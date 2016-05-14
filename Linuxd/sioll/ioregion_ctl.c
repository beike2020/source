#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>	
#include <linux/fs.h>		
#include <linux/errno.h>	
#include <linux/delay.h>	
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <asm/io.h>

#define SHORT_NR_PORTS	8	/* use 8 ports by default */
#define NR_TIMEVAL 512 

static int major = 0;	
static int use_mem = 0;	
static unsigned long base = 0x378;
static int irq = -1;
static int probe = 0;	
static int wq = 0;	
static int tasklet = 0;	
static int share = 0;	
module_param(major, int, 0);
module_param(use_mem, int, 0);
module_param(base, long, 0);
module_param(irq, int, 0);
module_param(probe, int, 0);
module_param(wq, int, 0);
module_param(tasklet, int, 0);
module_param(share, int, 0);

MODULE_AUTHOR ("Alessandro Rubini");
MODULE_LICENSE("Dual BSD/GPL");

unsigned long short_base = 0;
volatile int  short_irq = -1;
unsigned long short_buffer = 0;
unsigned long volatile short_head;
volatile unsigned long short_tail;
struct timeval tv_data[NR_TIMEVAL]; 
volatile struct timeval *tv_head=tv_data;
volatile struct timeval *tv_tail=tv_data;
static struct work_struct short_wq;
int short_wq_count = 0;

enum short_modes {
	SHORT_DEFAULT=0, 
	SHORT_PAUSE, 
	SHORT_STRING, 
	SHORT_MEMORY
};

void short_do_tasklet(unsigned long);
DECLARE_TASKLET(short_tasklet, short_do_tasklet, 0);

DECLARE_WAIT_QUEUE_HEAD(short_queue);

static inline void short_incr_bp(volatile unsigned long *index, int length)
{
	unsigned long new = *index + length;
	
	barrier();
	*index = (new >= (short_buffer + PAGE_SIZE)) ? short_buffer : new;
}

ssize_t short_io_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int count0;
	DEFINE_WAIT(wait);

	while (short_head == short_tail) {
		prepare_to_wait(&short_queue, &wait, TASK_INTERRUPTIBLE);
		if (short_head == short_tail)
			schedule();
		
		finish_wait(&short_queue, &wait);
		if (signal_pending (current))
			return -ERESTARTSYS;
	} 

	count0 = short_head - short_tail;
	if (count0 < 0) 
		count0 = short_buffer + PAGE_SIZE - short_tail;
	
	if (count0 < count) 
		count = count0;

	if (copy_to_user(buf, (char *)short_tail, count))
		return -EFAULT;
	
	short_incr_bp (&short_tail, count);
	
	return count;
}

ssize_t short_io_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int written = 0, odd = *f_pos & 1;
	unsigned long port = short_base; 
	void *address = (void *) short_base;

	if (use_mem) {
		while (written < count)
			iowrite8(0xff*((++written + odd)&1), address);
	} else {
		while (written < count)
			outb(0xff*((++written + odd)&1), port);
	}

	*f_pos += count;
	
	return written;
}

struct file_operations short_io_fops = {
	.owner	 = THIS_MODULE,
	.read	 = short_io_read,
	.write	 = short_io_write,
	.open	 = short_open,
	.release = short_release,
};

int short_open (struct inode *inode, struct file *filp)
{
	extern struct file_operations short_io_fops;

	if (iminor (inode) & 0x80)
		filp->f_op = &short_io_fops; 
	
	return 0;
}

int short_release (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t short_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int retval = count, minor = iminor (filp->f_dentry->d_inode);
	unsigned long port = short_base + (minor&0x0f);
	void *address = (void *) short_base + (minor&0x0f);
	int mode = (minor&0x70) >> 4;
	unsigned char *kbuf, *ptr;

	kbuf = kmalloc(count, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;
	ptr = kbuf;

	if (use_mem)
		mode = SHORT_MEMORY;
	
	switch(mode) {
	    case SHORT_DEFAULT:
			while (count--) {
				*(ptr++) = inb(port);
				rmb();
			}
			break;

	    case SHORT_PAUSE:
			while (count--) {
				*(ptr++) = inb_p(port);
				rmb();
			}
			break;
		
	    case SHORT_STRING:
			insb(port, ptr, count);
			rmb();
			break;

	    case SHORT_MEMORY:
			while (count--) {
				*ptr++ = ioread8(address);
				rmb();
			}
			break;
		
	    default: 
			retval = -EINVAL;
			break;
	}
	
	if ((retval > 0) && copy_to_user(buf, kbuf, retval))
		retval = -EFAULT;
	
	kfree(kbuf);
	
	return retval;
}

ssize_t short_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int retval = count, minor = iminor(filp->f_dentry->d_inode);
	unsigned long port = short_base + (minor&0x0f);
	void *address = (void *) short_base + (minor&0x0f);
	int mode = (minor&0x70) >> 4;
	unsigned char *kbuf, *ptr;

	kbuf = kmalloc(count, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;
	
	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;
	
	ptr = kbuf;

	if (use_mem)
		mode = SHORT_MEMORY;

	switch(mode) {
		case SHORT_DEFAULT:
			while (count--) {
				outb(*(ptr++), port);
				wmb();
			}
			break;
		
		case SHORT_PAUSE:
			while (count--) {
				outb_p(*(ptr++), port);
				wmb();
			}
			break;

		case SHORT_STRING:
			outsb(port, ptr, count);
			wmb();
			break;

		case SHORT_MEMORY:
			while (count--) {
				iowrite8(*ptr++, address);
				wmb();
			}
			break;

		default: 
			retval = -EINVAL;
			break;
	}
	
	kfree(kbuf);
	
	return retval;
}

unsigned int short_poll(struct file *filp, poll_table *wait)
{
	return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
}

struct file_operations short_fops = {
	.owner	 = THIS_MODULE,
	.read	 = short_read,
	.write	 = short_write,
	.poll	 = short_poll,
	.open	 = short_open,
	.release = short_release,
};

static inline void short_incr_tv(volatile struct timeval **tvp)
{
	if (*tvp == (tv_data + NR_TIMEVAL - 1)) {
		*tvp = tv_data;	
	} else {
		(*tvp)++;
	}
}

void short_do_tasklet (unsigned long unused)
{
	int savecount = short_wq_count, written;
	short_wq_count = 0; 

	written = sprintf((char *)short_head, "bh after %6i\n", savecount);
	short_incr_bp(&short_head, written);

	do {
		written = sprintf((char *)short_head, "%08u.%06u\n", (int)(tv_tail->tv_sec % 100000000), (int)(tv_tail->tv_usec));
		short_incr_bp(&short_head, written);
		short_incr_tv(&tv_tail);
	} while (tv_tail != tv_head);

	wake_up_interruptible(&short_queue); 
}

void short_irq_probe(void)
{
	int count = 0;
	
	do {
		unsigned long mask;
		
		mask = probe_irq_on();
		outb_p(0x10,short_base+2); 
		outb_p(0x00,short_base);   
		outb_p(0xFF,short_base);   
		outb_p(0x00,short_base+2);
		udelay(5);  
		short_irq = probe_irq_off(mask);
		if (short_irq == 0) { 
			printk(KERN_INFO "short: no irq reported by probe\n");
			short_irq = -1;
		}
	} while (short_irq < 0 && count++ < 5);
	
	if (short_irq < 0)
		printk("short: probe failed %i times, giving up\n", count);
}

irqreturn_t irq_selfprobe_fn(int irq, void *dev_id, struct pt_regs *regs)
{
	if (short_irq == 0) 
		short_irq = irq;
	
	if (short_irq != irq) 
		short_irq = -irq; 
	
	return IRQ_HANDLED;
}

void short_irq_selfprobe(void)
{
	int trials[] = {3, 5, 7, 9, 0};
	int tried[]  = {0, 0, 0, 0, 0};
	int i, count = 0;

	for (i = 0; trials[i]; i++){
		tried[i] = request_irq(trials[i], irq_selfprobe_fn, SA_INTERRUPT, "short probe", NULL);
		printk("short: request_irq-%d = %d\n", i, tried[i]);
	}
	
	do {
		short_irq = 0; 
		outb_p(0x10,short_base+2); 
		outb_p(0x00,short_base);
		outb_p(0xFF,short_base); 
		outb_p(0x00,short_base+2);
		udelay(5); 

		if (short_irq == 0) 
			printk(KERN_INFO "short: no irq reported by probe\n");
	} while (short_irq <=0 && count++ < 5);

	for (i = 0; trials[i]; i++) {
		if (tried[i] == 0)
			free_irq(trials[i], NULL);
	}
	
	if (short_irq < 0)
		printk("short: probe failed %i times, giving up\n", count);
}

irqreturn_t short_waitqueue_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct timeval tv;
	int written;

	do_gettimeofday(&tv);
	written = sprintf((char *)short_head,"%08u.%06u\n", (int)(tv.tv_sec % 100000000), (int)(tv.tv_usec));
	short_incr_bp(&short_head, written);
	wake_up_interruptible(&short_queue); 
	
	return IRQ_HANDLED;
}

irqreturn_t short_waitqueue_shareinterrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int value, written;
	struct timeval tv;

	value = inb(short_base);
	if (!(value & 0x80))
		return IRQ_NONE;
	outb(value & 0x7F, short_base);

	do_gettimeofday(&tv);
	written = sprintf((char *)short_head, "%08u.%06u\n", (int)(tv.tv_sec % 100000000), (int)(tv.tv_usec));
	short_incr_bp(&short_head, written);
	wake_up_interruptible(&short_queue); 
	
	return IRQ_HANDLED;
}

irqreturn_t short_tasklet_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	do_gettimeofday((struct timeval *) tv_head); 
	short_incr_tv(&tv_head);
	tasklet_schedule(&short_tasklet);
	short_wq_count++; 
	
	return IRQ_HANDLED;
}

irqreturn_t short_sharequeue_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	do_gettimeofday((struct timeval *) tv_head);
	short_incr_tv(&tv_head);
	schedule_work(&short_wq);
	short_wq_count++; 
	
	return IRQ_HANDLED;
}

int short_init(void)
{
	int result;

	short_base = base;
	short_irq = irq;

	if (!use_mem) {
		if (!request_region(short_base, SHORT_NR_PORTS, "short")) {
			printk(KERN_INFO "short: can't get I/O port address 0x%lx\n", short_base);
			return -ENODEV;
		}
	} else {
		if (!request_mem_region(short_base, SHORT_NR_PORTS, "short")) {
			printk(KERN_INFO "short: can't get I/O mem address 0x%lx\n", short_base);
			return -ENODEV;
		}
		short_base = (unsigned long)ioremap(short_base, SHORT_NR_PORTS);
	}

	result = register_chrdev(major, "short", &short_fops);
	if (result < 0) {
		printk(KERN_INFO "short: can't get major number\n");
		release_region(short_base,SHORT_NR_PORTS);  
		return result;
	} 

	if (major == 0) 
		major = result;

	short_buffer = __get_free_pages(GFP_KERNEL, 0); 
	short_head = short_tail = short_buffer;

	INIT_WORK(&short_wq, (void (*)(void *))short_do_tasklet, NULL);

	if (short_irq < 0 && probe == 1)
		short_irq_probe();

	if (short_irq < 0 && probe == 2)
		short_irq_selfprobe();

	if (short_irq < 0) {
		switch(short_base) {
		    case 0x378: 
				short_irq = 7; 
				break;
				
		    case 0x278: 
				short_irq = 2; 
				break;
				
		    case 0x3bc: 
				short_irq = 5; 
				break;
		}
	}

	if (short_irq >= 0 && share > 0) {
		result = request_irq(short_irq, short_waitqueue_shareinterrupt, SA_SHIRQ | SA_INTERRUPT, "short", short_waitqueue_shareinterrupt);
		if (result) {
			printk(KERN_INFO "Can't get assigned irq %i\n", short_irq);
			short_irq = -1;
		} else { 
			outb(0x10, short_base+2);
		}
		return 0; 
	}

	if (short_irq >= 0) {
		result = request_irq(short_irq, short_waitqueue_interrupt, SA_INTERRUPT, "short", NULL);
		if (result) {
			printk(KERN_INFO "Can't get assigned irq %i\n", short_irq);
			short_irq = -1;
		} else { 
			outb(0x10,short_base+2);
		}
	}

	if (short_irq >= 0 && (wq + tasklet) > 0) {
		free_irq(short_irq,NULL);
		result = request_irq(short_irq, tasklet ? short_tasklet_interrupt : short_sharequeue_interrupt, SA_INTERRUPT,"short-bh", NULL);
		if (result) {
			printk(KERN_INFO "short-bh: can't get assigned irq %i\n", short_irq);
			short_irq = -1;
		}
	}

	return 0;
}

void short_cleanup(void)
{
	if (short_irq >= 0) {
		outb(0x0, short_base + 2);   
		if (!share) { 
			free_irq(short_irq, NULL);
		} else {
			free_irq(short_irq, short_waitqueue_shareinterrupt);
		}
	}

	if (tasklet) {
		tasklet_disable(&short_tasklet);
	} else {
		flush_scheduled_work();
	}
	
	unregister_chrdev(major, "short");
	
	if (use_mem) {
		iounmap((void __iomem *)short_base);
		release_mem_region(short_base, SHORT_NR_PORTS);
	} else {
		release_region(short_base,SHORT_NR_PORTS);
	}
	
	if (short_buffer) 
		free_page(short_buffer);
}

module_init(short_init);
module_exit(short_cleanup);
