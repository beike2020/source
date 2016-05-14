#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/kernel.h> 
#include <linux/fs.h>	  
#include <linux/errno.h> 
#include <linux/delay.h>  
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>

// Register offsets
#define SP_DATA    		0x00
#define SP_STATUS  		0x01
#define SP_CONTROL 		0x02
#define SP_NPORTS     	3

// Status register bits.
#define SP_SR_BUSY 		0x80
#define SP_SR_ACK		0x40
#define SP_SR_PAPER		0x20
#define SP_SR_ONLINE	0x10
#define SP_SR_ERR		0x08

// Control register.
#define SP_CR_IRQ		0x10
#define SP_CR_SELECT	0x08
#define SP_CR_INIT		0x04
#define SP_CR_AUTOLF	0x02
#define SP_CR_STROBE	0x01

// Minimum space before waking up a writer.
#define SP_MIN_SPACE	PAGE_SIZE/2
#define SHORTP_NR_PORTS 3
#define TIMEOUT 		5*HZ  

static int major = 0; 
static unsigned long base = 0x378;
static int irq = -1;
static int delay = 0;
module_param(major, int, 0);
module_param(base, long, 0);
module_param(irq, int, 0);
module_param(delay, int, 0);

unsigned long shortp_base = 0;
static int shortp_irq = -1;
static int shortp_delay;
static struct timeval shortp_tv; 
static unsigned long shortp_in_buffer = 0;
static unsigned long volatile shortp_in_head;
static volatile unsigned long shortp_in_tail;
static unsigned char *shortp_out_buffer = NULL;
static volatile unsigned char *shortp_out_head, *shortp_out_tail;
static struct semaphore shortp_out_sem;
static struct workqueue_struct *shortp_workqueue;
static struct timer_list shortp_timer;
static spinlock_t shortp_out_lock;
volatile static int shortp_output_active;

static void shortp_cleanup(void);
static void shortp_timeout(unsigned long unused);
static void shortp_do_work(void *);

static DECLARE_WORK(shortp_work, shortp_do_work, NULL);
static DECLARE_WAIT_QUEUE_HEAD(shortp_in_queue);
static DECLARE_WAIT_QUEUE_HEAD(shortp_out_queue);
static DECLARE_WAIT_QUEUE_HEAD(shortp_empty_queue);

MODULE_AUTHOR ("Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

static inline void shortp_roll_inspace(volatile unsigned long *index, int delta)
{
	unsigned long new = *index + delta;
	
	barrier ();  
	if( new >= (shortp_in_buffer + PAGE_SIZE)) 
		new = shortp_in_buffer;

	*index = new;
}

static inline void shortp_roll_outspace(volatile unsigned char **bp, int incr)
{
	unsigned char *new = (unsigned char *) *bp + incr;

	barrier ();  
	if (new >= (shortp_out_buffer + PAGE_SIZE))
		new -= PAGE_SIZE;
	
	*bp = new;
}

static inline int shortp_get_outspace(void)
{
	int space;

	if (shortp_out_head >= shortp_out_tail) {
		space = PAGE_SIZE - (shortp_out_head - shortp_out_buffer);
		return (shortp_out_tail == shortp_out_buffer) ? space - 1 : space;
	} else {
		return (shortp_out_tail - shortp_out_head) - 1;
	}
}

static int shortp_open(struct inode *inode, struct file *filp)
{
	return 0;
}


static int shortp_release(struct inode *inode, struct file *filp)
{
	wait_event_interruptible(shortp_empty_queue, shortp_output_active == 0);

	return 0;
}

static ssize_t shortp_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int count0;
	DEFINE_WAIT(wait);

	while (shortp_in_head == shortp_in_tail) {
		prepare_to_wait(&shortp_in_queue, &wait, TASK_INTERRUPTIBLE);
		if (shortp_in_head == shortp_in_tail)
			schedule();
		
		finish_wait(&shortp_in_queue, &wait);
		
		if (signal_pending (current))  
			return -ERESTARTSYS; 
	}

	count0 = shortp_in_head - shortp_in_tail;
	if (count0 < 0) 
		count0 = shortp_in_buffer + PAGE_SIZE - shortp_in_tail;
	
	if (count0 < count)
		count = count0;

	if (copy_to_user(buf, (char *)shortp_in_tail, count))
		return -EFAULT;
	
	shortp_roll_inspace(&shortp_in_tail, count);
	
	return count;
}

static ssize_t shortp_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int space, written = 0;
	unsigned long flags;
	
	if (down_interruptible(&shortp_out_sem))
		return -ERESTARTSYS;

	while (written < count) {
		space = shortp_get_outspace();
		if (space <= 0) {
			if (wait_event_interruptible(shortp_out_queue, (space = shortp_get_outspace()) > 0))
				goto out;
		}

		if ((space + written) > count)
			space = count - written;
		
		if (copy_from_user((char *)shortp_out_head, buf, space)) {
			up(&shortp_out_sem);
			return -EFAULT;
		}
		
		shortp_roll_outspace(&shortp_out_head, space);
		buf += space;
		written += space;

		spin_lock_irqsave(&shortp_out_lock, flags);
		if (!shortp_output_active){
			shortp_output_active = 1;
			shortp_timer.expires = jiffies + TIMEOUT;
			add_timer(&shortp_timer);
			queue_work(shortp_workqueue, &shortp_work);
		}
		
		spin_unlock_irqrestore(&shortp_out_lock, flags);
	}

out:
	*f_pos += written;
	up(&shortp_out_sem);
	return written;
}

static unsigned int shortp_poll(struct file *filp, poll_table *wait)
{
    return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
}

static void shortp_do_work(void *unused)
{
	int written;
	unsigned long flags;
	unsigned char cr;
	
	if ((inb(shortp_base + SP_STATUS) & SP_SR_BUSY) == 0) {
		printk(KERN_INFO "shortprint: waiting for printer busy\n");
		printk(KERN_INFO "Status is 0x%x\n", inb(shortp_base + SP_STATUS));
		while ((inb(shortp_base + SP_STATUS) & SP_SR_BUSY) == 0) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(10*HZ); 
		}
	}
	
	spin_lock_irqsave(&shortp_out_lock, flags);

	if (shortp_out_head == shortp_out_tail) { 
		shortp_output_active = 0;
		wake_up_interruptible(&shortp_empty_queue);
		del_timer(&shortp_timer);  
	} else {
		cr = inb(shortp_base + SP_CONTROL);
		mod_timer(&shortp_timer, jiffies + TIMEOUT);
		outb_p(*shortp_out_tail, shortp_base+SP_DATA);
		shortp_roll_outspace(&shortp_out_tail, 1);

		if (shortp_delay)
			udelay(shortp_delay);
		
		outb_p(cr | SP_CR_STROBE, shortp_base+SP_CONTROL);
		if (shortp_delay)
			udelay(shortp_delay);
		outb_p(cr & ~SP_CR_STROBE, shortp_base+SP_CONTROL);
	}

	if (((PAGE_SIZE + shortp_out_tail - shortp_out_head) % PAGE_SIZE) > SP_MIN_SPACE) 
		wake_up_interruptible(&shortp_out_queue);

	spin_unlock_irqrestore(&shortp_out_lock, flags);

	written = sprintf((char *)shortp_in_head, "%08u.%06u\n", (int)(shortp_tv.tv_sec % 100000000), (int)(shortp_tv.tv_usec));
	shortp_roll_inspace(&shortp_in_head, written);
	wake_up_interruptible(&shortp_in_queue); 
}

static irqreturn_t shortp_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	if (!shortp_output_active) 
		return IRQ_NONE;

	do_gettimeofday(&shortp_tv);
	queue_work(shortp_workqueue, &shortp_work);
	
	return IRQ_HANDLED;
}

static void shortp_timeout(unsigned long unused)
{
	unsigned long flags;
	unsigned char status;
   
	if (!shortp_output_active)
		return;
	
	spin_lock_irqsave(&shortp_out_lock, flags);
	status = inb(shortp_base + SP_STATUS);

	if ((status & SP_SR_BUSY) == 0 || (status & SP_SR_ACK)) {
		shortp_timer.expires = jiffies + TIMEOUT;
		add_timer(&shortp_timer);
		spin_unlock_irqrestore(&shortp_out_lock, flags);
		return;
	}

	spin_unlock_irqrestore(&shortp_out_lock, flags);
	shortp_interrupt(shortp_irq, NULL, NULL);
}
    
static struct file_operations shortp_fops = {
	.read 		=	shortp_read,
	.write 		=   shortp_write,
	.open 		=	shortp_open,
	.release 	= 	shortp_release,
	.poll 		=	shortp_poll,
	.owner	 	= 	THIS_MODULE
};

static int shortp_init(void)
{
	int result;

	shortp_base = base;
	shortp_irq = irq;
	shortp_delay = delay;

	if (!request_region(shortp_base, SHORTP_NR_PORTS, "shortprint")) {
		printk(KERN_INFO "shortprint: can't get I/O port address 0x%lx\n", shortp_base);
		return -ENODEV;
	}	

	result = register_chrdev(major, "shortprint", &shortp_fops);
	if (result < 0) {
		printk(KERN_INFO "shortp: can't get major number\n");
		release_region(shortp_base, SHORTP_NR_PORTS);
		return result;
	}
	
	if (major == 0)
		major = result; 

	shortp_in_buffer = __get_free_pages(GFP_KERNEL, 0); 
	shortp_in_head = shortp_in_tail = shortp_in_buffer;
	
	shortp_out_buffer = (unsigned char *)__get_free_pages(GFP_KERNEL, 0);
	shortp_out_head = shortp_out_tail = shortp_out_buffer;
	sema_init(&shortp_out_sem, 1);
	shortp_output_active = 0;
	spin_lock_init(&shortp_out_lock);
	
	init_timer(&shortp_timer);
	shortp_timer.function = shortp_timeout;
	shortp_timer.data = 0;
    
	shortp_workqueue = create_singlethread_workqueue("shortprint");

	if (shortp_irq < 0) {
		switch(shortp_base) {
		    case 0x378: 
				shortp_irq = 7; 
				break;
				
		    case 0x278: 
				shortp_irq = 2; 
				break;
				
		    case 0x3bc: 
				shortp_irq = 5; 
				break;
		}
	}

	result = request_irq(shortp_irq, shortp_interrupt, 0, "shortprint", NULL);
	if (result) {
		printk(KERN_INFO "Can't get assigned irq %i\n", shortp_irq);
		shortp_irq = -1;
		shortp_cleanup ();
		return result;
	}

	outb(SP_CR_IRQ | SP_CR_SELECT | SP_CR_INIT, shortp_base + SP_CONTROL);

	return 0;
}

static void shortp_cleanup(void)
{
	if (shortp_irq >= 0) {
		outb(0x0, shortp_base + SP_CONTROL);  
		free_irq(shortp_irq, NULL);
	}

	unregister_chrdev(major, "shortprint");
	release_region(shortp_base,SHORTP_NR_PORTS);

	if (shortp_output_active)
		del_timer_sync (&shortp_timer);
	
	flush_workqueue(shortp_workqueue);
	destroy_workqueue(shortp_workqueue);

	if (shortp_in_buffer)
		free_page(shortp_in_buffer);
	
	if (shortp_out_buffer)
		free_page((unsigned long) shortp_out_buffer);
}

module_init(shortp_init);
module_exit(shortp_cleanup);
