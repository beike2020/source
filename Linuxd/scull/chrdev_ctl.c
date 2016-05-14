#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	
#include <linux/slab.h>		
#include <linux/fs.h>		
#include <linux/errno.h>	
#include <linux/types.h>	
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/system.h>		
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/tty.h>
#include <asm/atomic.h>
#include <linux/list.h>

#include "chrdev_ctl.h"	

int scull_major =   SCULL_MAJOR;
int scull_minor =   0;
int scull_nr_devs = SCULL_NR_DEVS;	
int scull_quantum = SCULL_QUANTUM;
int scull_qset =    SCULL_QSET;
module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_nr_devs, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);
module_param(scull_qset, int, S_IRUGO);

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

struct scull_dev *scull_devices;	

/****************************scull_pipe start**************************/
struct scull_pipe {
	wait_queue_head_t inq, outq;       				/* read and write queues */
	char *buffer, *end;                				/* begin of buf, end of buf */
	int buffersize;                    				/* used in pointer arithmetic */
	char *rp, *wp;                     				/* where to read, where to write */
	int nreaders, nwriters;            				/* number of openings for r/w */
	struct fasync_struct *async_queue; 				/* asynchronous readers */
	struct semaphore sem;              				/* mutual exclusion semaphore */
	struct cdev cdev;                  				/* Char device structure */
};

static struct scull_pipe *scull_p_devices;
dev_t scull_p_devno;								/* Our first device number */

static int scull_p_nr_devs = SCULL_P_NR_DEVS;		/* number of pipe devices */
int scull_p_buffer =  SCULL_P_BUFFER;				/* buffer size */
module_param(scull_p_nr_devs, int, 0);			/* FIXME check perms */
module_param(scull_p_buffer, int, 0);

static int scull_p_open(struct inode *inode, struct file *filp)
{
	struct scull_pipe *dev;

	dev = container_of(inode->i_cdev, struct scull_pipe, cdev);
	filp->private_data = dev;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	
	if (dev->buffer == NULL) {
		dev->buffer = kmalloc(scull_p_buffer, GFP_KERNEL);
		if (dev->buffer == NULL) {
			up(&dev->sem);
			return -ENOMEM;
		}
	}
	
	dev->buffersize = scull_p_buffer;
	dev->end = dev->buffer + dev->buffersize;
	dev->rp = dev->wp = dev->buffer; 

	if (filp->f_mode & FMODE_READ)
		dev->nreaders++;
	
	if (filp->f_mode & FMODE_WRITE)
		dev->nwriters++;
	
	up(&dev->sem);

	return nonseekable_open(inode, filp);
}

static int scull_p_release(struct inode *inode, struct file *filp)
{
	struct scull_pipe *dev = filp->private_data;

	scull_p_fasync(-1, filp, 0);
	down(&dev->sem);
	
	if (filp->f_mode & FMODE_READ)
		dev->nreaders--;
	
	if (filp->f_mode & FMODE_WRITE)
		dev->nwriters--;
	
	if (dev->nreaders + dev->nwriters == 0) {
		kfree(dev->buffer);
		dev->buffer = NULL; 
	}
	
	up(&dev->sem);
	
	return 0;
}

static ssize_t scull_p_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_pipe *dev = filp->private_data;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	while (dev->rp == dev->wp) { 
		up(&dev->sem); 
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		
		PDEBUG("\"%s\" reading: going to sleep\n", current->comm);
		
		if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp)))
			return -ERESTARTSYS; 
		
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}
	
	if (dev->wp > dev->rp) {
		count = min(count, (size_t)(dev->wp - dev->rp));
	} else {
		count = min(count, (size_t)(dev->end - dev->rp));
	}
	
	if (copy_to_user(buf, dev->rp, count)) {
		up(&dev->sem);
		return -EFAULT;
	}
	
	dev->rp += count;
	if (dev->rp == dev->end)
		dev->rp = dev->buffer; 
	
	up(&dev->sem);
	wake_up_interruptible(&dev->outq);
	PDEBUG("\"%s\" did read %li bytes\n",current->comm, (long)count);
	
	return count;
}

static int scull_getwritespace(struct scull_pipe *dev, struct file *filp)
{
	while (spacefree(dev) == 0) { 
		DEFINE_WAIT(wait);
		up(&dev->sem);
		
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		
		PDEBUG("\"%s\" writing: going to sleep\n",current->comm);
		
		prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
		if (spacefree(dev) == 0)
			schedule();
		
		finish_wait(&dev->outq, &wait);
		if (signal_pending(current))
			return -ERESTARTSYS; 
		
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}
	
	return 0;
}	

static int spacefree(struct scull_pipe *dev)
{
	if (dev->rp == dev->wp)
		return dev->buffersize - 1;
	
	return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) - 1;
}

static ssize_t scull_p_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int result;
	struct scull_pipe *dev = filp->private_data;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	result = scull_getwritespace(dev, filp);
	if (result)
		return result; 

	count = min(count, (size_t)spacefree(dev));
	
	if (dev->wp >= dev->rp)
		count = min(count, (size_t)(dev->end - dev->wp));
	else 
		count = min(count, (size_t)(dev->rp - dev->wp - 1));
	
	PDEBUG("Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);

	if (copy_from_user(dev->wp, buf, count)) {
		up (&dev->sem);
		return -EFAULT;
	}
	
	dev->wp += count;
	if (dev->wp == dev->end)
		dev->wp = dev->buffer; 
	
	up(&dev->sem);
	wake_up_interruptible(&dev->inq);  

	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
	
	PDEBUG("\"%s\" did write %li bytes\n",current->comm, (long)count);
	
	return count;
}

static unsigned int scull_p_poll(struct file *filp, poll_table *wait)
{
	struct scull_pipe *dev = filp->private_data;
	unsigned int mask = 0;

	down(&dev->sem);
	poll_wait(filp, &dev->inq,  wait);
	poll_wait(filp, &dev->outq, wait);

	if (dev->rp != dev->wp)
		mask |= POLLIN | POLLRDNORM;
	
	if (spacefree(dev))
		mask |= POLLOUT | POLLWRNORM;	
	
	up(&dev->sem);
	
	return mask;
}

static int scull_p_fasync(int fd, struct file *filp, int mode)
{
	struct scull_pipe *dev = filp->private_data;

	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

#ifdef SCULL_DEBUG
static void scullp_proc_offset(char *buf, char **start, off_t *offset, int *len)
{
	if (*offset == 0)
		return;
	
	if (*offset >= *len) {	
		*offset -= *len;
		*len = 0;
	} else {			
		*start = buf + *offset;
		*offset = 0;
	}
}

static int scull_read_p_mem(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int i, len;
	struct scull_pipe *p;

	*start = buf;
	len = sprintf(buf, "Default buffersize is %d\n", scull_p_buffer);
	
	for(i = 0; i < scull_p_nr_devs && len <= PAGE_SIZE - 200; i++) {
		p = &scull_p_devices[i];
		if (down_interruptible(&p->sem))
			return -ERESTARTSYS;
		
		len += sprintf(buf+len, "\nDevice %d: %p\n", i, p);
		len += sprintf(buf+len, "   Buffer: %p to %p (%d bytes)\n", p->buffer, p->end, p->buffersize);
		len += sprintf(buf+len, "   rp %p   wp %p\n", p->rp, p->wp);
		len += sprintf(buf+len, "   readers %d   writers %d\n", p->nreaders, p->nwriters);

		up(&p->sem);
		scullp_proc_offset(buf, start, &offset, &len);
	}
	
	*eof = (len <= PAGE_SIZE - 200);
	
	return len;
}
#endif

struct file_operations scull_pipe_fops = {
	.owner =	THIS_MODULE,
	.llseek =	no_llseek,
	.read =		scull_p_read,
	.write =	scull_p_write,
	.poll =		scull_p_poll,
	.ioctl =	scull_ioctl,
	.open =		scull_p_open,
	.release =	scull_p_release,
	.fasync =	scull_p_fasync,
};

static void scull_p_setup_cdev(struct scull_pipe *dev, int index)
{
	int err, devno;

	devno = scull_p_devno + index;
	
	cdev_init(&dev->cdev, &scull_pipe_fops);
	dev->cdev.owner = THIS_MODULE;
	
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scullpipe%d", err, index);
}

int scull_p_init(dev_t firstdev)
{
	int i, result;

	result = register_chrdev_region(firstdev, scull_p_nr_devs, "scullp");
	if (result < 0) {
		printk(KERN_NOTICE "Unable to get scullp region, error %d\n", result);
		return 0;
	}
	scull_p_devno = firstdev;
	
	scull_p_devices = kmalloc(scull_p_nr_devs * sizeof(struct scull_pipe), GFP_KERNEL);
	if (scull_p_devices == NULL) {
		unregister_chrdev_region(firstdev, scull_p_nr_devs);
		return 0;
	}
	memset(scull_p_devices, 0, scull_p_nr_devs * sizeof(struct scull_pipe));

	for (i = 0; i < scull_p_nr_devs; i++) {
		init_waitqueue_head(&(scull_p_devices[i].inq));
		init_waitqueue_head(&(scull_p_devices[i].outq));
		init_MUTEX(&scull_p_devices[i].sem);
		scull_p_setup_cdev(scull_p_devices + i, i);
	}
	
#ifdef SCULL_DEBUG
	create_proc_read_entry("scullpipe", 0, NULL, scull_read_p_mem, NULL);
#endif

	return scull_p_nr_devs;
}

void scull_p_cleanup(void)
{
	int i;

#ifdef SCULL_DEBUG
	remove_proc_entry("scullpipe", NULL);
#endif

	if (scull_p_devices == NULL)
		return; 

	for (i = 0; i < scull_p_nr_devs; i++) {
		cdev_del(&scull_p_devices[i].cdev);
		kfree(scull_p_devices[i].buffer);
	}
	kfree(scull_p_devices);
	scull_p_devices = NULL; 
	
	unregister_chrdev_region(scull_p_devno, scull_p_nr_devs);
}

/****************************scull_pipe end**************************/

/***************************scull_access satrt*************************/
static dev_t scull_a_firstdev; 
static struct scull_dev scull_s_device;
static struct scull_dev scull_u_device;
static struct scull_dev scull_w_device;
static atomic_t scull_s_available = ATOMIC_INIT(1);
static int scull_u_count;	
static int scull_w_count;	
static uid_t scull_u_owner;	
static uid_t scull_w_owner;
static spinlock_t scull_u_lock = SPIN_LOCK_UNLOCKED;
static spinlock_t scull_w_lock = SPIN_LOCK_UNLOCKED;
static DECLARE_WAIT_QUEUE_HEAD(scull_w_wait);

static int scull_s_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = &scull_s_device; 
	
	if (!atomic_dec_and_test (&scull_s_available)) {
		atomic_inc(&scull_s_available);
		return -EBUSY; 
	}

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);
	
	filp->private_data = dev;
	
	return 0;     
}

static int scull_s_release(struct inode *inode, struct file *filp)
{ 
	atomic_inc(&scull_s_available);
	
	return 0;
}

struct file_operations scull_sngl_fops = {
	.owner 		=	THIS_MODULE,
	.llseek 	=	scull_llseek,
	.read 		=   scull_read,
	.write 		=   scull_write,
	.ioctl 		=   scull_ioctl,
	.open 		=   scull_s_open,
	.release 	=   scull_s_release,
};

static int scull_u_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = &scull_u_device; 

	spin_lock(&scull_u_lock);

	if (scull_u_count && (scull_u_owner != current->uid) && 
	   (scull_u_owner != current->euid) && !capable(CAP_DAC_OVERRIDE)) { 
		spin_unlock(&scull_u_lock);
		return -EBUSY;  
	}

	if (scull_u_count == 0)
		scull_u_owner = current->uid; 

	scull_u_count++;
	spin_unlock(&scull_u_lock);

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);
	
	filp->private_data = dev;
	
	return 0; 
}

static int scull_u_release(struct inode *inode, struct file *filp)
{
	spin_lock(&scull_u_lock);
	scull_u_count--; 
	spin_unlock(&scull_u_lock);
	
	return 0;
}

struct file_operations scull_user_fops = {
	.owner 		=	THIS_MODULE,
	.llseek 	=	scull_llseek,
	.read 		=	scull_read,
	.write 		=	scull_write,
	.ioctl 		=	scull_ioctl,
	.open 		=	scull_u_open,
	.release 	=	scull_u_release,
};

static inline int scull_w_available(void)
{
	return scull_w_count == 0 || scull_w_owner == current->uid ||
		   scull_w_owner == current->euid || capable(CAP_DAC_OVERRIDE);
}

static int scull_w_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = &scull_w_device; 

	spin_lock(&scull_w_lock);
	while (!scull_w_available()) {
		spin_unlock(&scull_w_lock);
		
		if (filp->f_flags & O_NONBLOCK) 
			return -EAGAIN;
		
		if (wait_event_interruptible(scull_w_wait, scull_w_available()))
			return -ERESTARTSYS; 
		
		spin_lock(&scull_w_lock);
	}
	
	if (scull_w_count == 0)
		scull_w_owner = current->uid; 
	
	scull_w_count++;
	spin_unlock(&scull_w_lock);

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);
	
	filp->private_data = dev;
	
	return 0;          
}

static int scull_w_release(struct inode *inode, struct file *filp)
{
	int temp;

	spin_lock(&scull_w_lock);
	scull_w_count--;
	temp = scull_w_count;
	spin_unlock(&scull_w_lock);

	if (temp == 0)
		wake_up_interruptible_sync(&scull_w_wait); 
	
	return 0;
}

struct file_operations scull_wusr_fops = {
	.owner 		=	THIS_MODULE,
	.llseek 	=	scull_llseek,
	.read 		=	scull_read,
	.write 		=	scull_write,
	.ioctl 		=	scull_ioctl,
	.open 		=	scull_w_open,
	.release 	=	scull_w_release,
};

struct scull_listitem {
	struct scull_dev device;
	dev_t key;
	struct list_head list;
    
};

static LIST_HEAD(scull_c_list);
static spinlock_t scull_c_lock = SPIN_LOCK_UNLOCKED;

static struct scull_dev scull_c_device;   

static struct scull_dev *scull_c_lookfor_device(dev_t key)
{
	struct scull_listitem *lptr;

	list_for_each_entry(lptr, &scull_c_list, list) {
		if (lptr->key == key)
			return &(lptr->device);
	}

	lptr = kmalloc(sizeof(struct scull_listitem), GFP_KERNEL);
	if (lptr == NULL)
		return NULL;
	memset(lptr, 0, sizeof(struct scull_listitem));
	
	lptr->key = key;
	scull_trim(&(lptr->device));
	init_MUTEX(&(lptr->device.sem));
	list_add(&lptr->list, &scull_c_list);

	return &(lptr->device);
}

static int scull_c_open(struct inode *inode, struct file *filp)
{
	dev_t key;
	struct scull_dev *dev;
 
	if (!current->signal->tty) { 
		PDEBUG("Process \"%s\" has no ctl tty\n", current->comm);
		return -EINVAL;
	}
	
	key = tty_devnum(current->signal->tty);

	spin_lock(&scull_c_lock);
	dev = scull_c_lookfor_device(key);
	spin_unlock(&scull_c_lock);

	if (dev == NULL)
		return -ENOMEM;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);
	
	filp->private_data = dev;
	
	return 0;          
}

static int scull_c_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct file_operations scull_priv_fops = {
	.owner 		=	THIS_MODULE,
	.llseek 	=   scull_llseek,
	.read 		=	scull_read,
	.write 		=	scull_write,
	.ioctl 		=	scull_ioctl,
	.open 		=	scull_c_open,
	.release 	=	scull_c_release,
};

static struct scull_adev_info {
	char *name;
	struct scull_dev *sculldev;
	struct file_operations *fops;
} scull_access_devs[] = {
	{ "scullsingle", &scull_s_device, &scull_sngl_fops },
	{ "sculluid", &scull_u_device, &scull_user_fops },
	{ "scullwuid", &scull_w_device, &scull_wusr_fops },
	{ "sullpriv", &scull_c_device, &scull_priv_fops }
};

#define SCULL_N_ADEVS 4

static void scull_access_setup (dev_t devno, struct scull_adev_info *devinfo)
{
	struct scull_dev *dev;
	int err;

	dev = devinfo->sculldev;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	init_MUTEX(&dev->sem);
	
	cdev_init(&dev->cdev, devinfo->fops);
	kobject_set_name(&dev->cdev.kobj, devinfo->name);
	dev->cdev.owner = THIS_MODULE;

	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_NOTICE "Error %d adding %s\n", err, devinfo->name);
		kobject_put(&dev->cdev.kobj);
	} else {
		printk(KERN_NOTICE "%s registered at %x\n", devinfo->name, devno);
	}
}

int scull_access_init(dev_t firstdev)
{
	int result, i;

	result = register_chrdev_region (firstdev, SCULL_N_ADEVS, "sculla");
	if (result < 0) {
		printk(KERN_WARNING "Device number registration failed\n");
		return 0;
	}
	scull_a_firstdev = firstdev;

	for (i = 0; i < SCULL_N_ADEVS; i++)
		scull_access_setup (firstdev + i, scull_access_devs + i);
	
	return SCULL_N_ADEVS;
}

void scull_access_cleanup(void)
{
	struct scull_listitem *lptr, *next;
	int i;

	for (i = 0; i < SCULL_N_ADEVS; i++) {
		struct scull_dev *dev = scull_access_devs[i].sculldev;
		cdev_del(&dev->cdev);
		scull_trim(scull_access_devs[i].sculldev);
	}

	list_for_each_entry_safe(lptr, next, &scull_c_list, list) {
		list_del(&lptr->list);
		scull_trim(&(lptr->device));
		kfree(lptr);
	}

	unregister_chrdev_region(scull_a_firstdev, SCULL_N_ADEVS);
	
	return;
}
/***************************scull_access end*************************/

int scull_trim(struct scull_dev *dev)
{
	int i;
	struct scull_qset *next, *dptr;

	for (dptr = dev->data; dptr; dptr = next) { 
		if (dptr->data != NULL) {
			for (i = 0; i < dev->qset; i++)
				kfree(dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		
		next = dptr->next;
		kfree(dptr);
	}
	
	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->data = NULL;
	
	return 0;
}

#ifdef SCULL_DEBUG 

int scull_read_procmem(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int i, j, len = 0;
	int limit = count - 80; 

	for (i = 0; i < scull_nr_devs && len <= limit; i++) {
		struct scull_dev *d = &scull_devices[i];
		struct scull_qset *qs = d->data;
		
		if (down_interruptible(&d->sem))
			return -ERESTARTSYS;
		
		len += sprintf(buf+len,"\nDevice %i: qset %i, q %i, sz %li\n", i, d->qset, d->quantum, d->size);

		for (; qs && len <= limit; qs = qs->next) { 
			len += sprintf(buf + len, "  item at %p, qset at %p\n", qs, qs->data);
			if (qs->data && !qs->next) {
				for (j = 0; j < d->qset; j++) {
					if (qs->data[j])
						len += sprintf(buf + len,"    % 4i: %8p\n", j, qs->data[j]);
				}
			}
		}
		
		up(&scull_devices[i].sem);
	}
	
	*eof = 1;
	
	return len;
}

static void *scull_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= scull_nr_devs)
		return NULL;   
	
	return scull_devices + *pos;
}

static void *scull_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >= scull_nr_devs)
		return NULL;
	
	return scull_devices + *pos;
}

static void scull_seq_stop(struct seq_file *s, void *v)
{
	return ;
}

static int scull_seq_show(struct seq_file *s, void *v)
{
	struct scull_dev *dev = (struct scull_dev *) v;
	struct scull_qset *d;
	int i;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	
	seq_printf(s, "\nDevice %d: qset %d, quantum %d, size %ld\n", (int)(dev - scull_devices), dev->qset, dev->quantum, dev->size);

	for (d = dev->data; d != NULL; d = d->next) { 
		seq_printf(s, "  item at %p, qset at %p\n", d, d->data);

		if (d->data && !d->next) { 
			for (i = 0; i < dev->qset; i++) {
				if (d->data[i])
					seq_printf(s, "    % 4d: %8p\n", i, d->data[i]);
			}
		}
	}
	
	up(&dev->sem);
	
	return 0;
}
	
static struct seq_operations scull_seq_ops = {
	.start = scull_seq_start,
	.next  = scull_seq_next,
	.stop  = scull_seq_stop,
	.show  = scull_seq_show
};

static int scull_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &scull_seq_ops);
}

static struct file_operations scull_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = scull_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};
	
static void scull_create_proc(void)
{
	struct proc_dir_entry *rentry;
	struct proc_dir_entry *entry;
	
	rentry = create_proc_read_entry("scullmem", 0, NULL, scull_read_procmem, NULL);
	if(rentry == NULL)
		return ;
	
	entry = create_proc_entry("scullseq", 0, NULL);
	if (entry)
		entry->proc_fops = &scull_proc_ops;
}

static void scull_remove_proc(void)
{
	remove_proc_entry("scullmem", NULL);
	remove_proc_entry("scullseq", NULL);
}

#endif 

int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev; 

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev; 

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
		
		scull_trim(dev);
		up(&dev->sem);
	}
	
	return 0;         
}

int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct scull_qset *scull_follow(struct scull_dev *dev, int n)
{
	struct scull_qset *qs = dev->data;

	if (qs == NULL) {
		qs = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if (qs == NULL)
			return NULL;  
		memset(qs, 0, sizeof(struct scull_qset));
	}

	while (n--) {
		if (qs->next == NULL) {
			qs->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			if (qs->next == NULL)
				return NULL;  
			memset(qs->next, 0, sizeof(struct scull_qset));
		}
		
		qs = qs->next;
		continue;
	}
	
	return qs;
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data; 
	struct scull_qset *dptr;	
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset; 
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	
	if (*f_pos >= dev->size)
		goto out;
	
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; 
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (!dptr || !dptr->data || !dptr->data[s_pos])
		goto out; 

	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}
	
	*f_pos += count;
	retval = count;

out:
	up(&dev->sem);
	return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; 
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (dptr == NULL)
		goto out;
	
	if (dptr->data == NULL) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (dptr->data == NULL)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	
	if (dptr->data[s_pos] == NULL) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (dptr->data[s_pos] == NULL)
			goto out;
	}
	
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	
	*f_pos += count;
	retval = count;

	if (dev->size < *f_pos)
		dev->size = *f_pos;

out:
	up(&dev->sem);
	return retval;
}

int scull_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, tmp;
	int retval = 0;
    
	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) 
		return -ENOTTY;
	
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) 
		eturn -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}
	
	if (err) 
		return -EFAULT;

	switch(cmd) {
		case SCULL_IOCRESET:
			scull_quantum = SCULL_QUANTUM;
			scull_qset = SCULL_QSET;
			break;

		case SCULL_IOCSQUANTUM: 
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			retval = __get_user(scull_quantum, (int __user *)arg);
			break;

		case SCULL_IOCTQUANTUM: 
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			scull_quantum = arg;
			break;

		case SCULL_IOCGQUANTUM: 
			retval = __put_user(scull_quantum, (int __user *)arg);
			break;

		case SCULL_IOCQQUANTUM:
			return scull_quantum;

		case SCULL_IOCXQUANTUM: 
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp = scull_quantum;
			retval = __get_user(scull_quantum, (int __user *)arg);
			if (retval == 0)
				retval = __put_user(tmp, (int __user *)arg);
			break;

		case SCULL_IOCHQUANTUM: 
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp = scull_quantum;
			scull_quantum = arg;
			return tmp;

		case SCULL_IOCSQSET:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			retval = __get_user(scull_qset, (int __user *)arg);
			break;

		case SCULL_IOCTQSET:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			scull_qset = arg;
			break;

		case SCULL_IOCGQSET:
			retval = __put_user(scull_qset, (int __user *)arg);
			break;

		case SCULL_IOCQQSET:
			return scull_qset;

		case SCULL_IOCXQSET:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp = scull_qset;
			retval = __get_user(scull_qset, (int __user *)arg);
			if (retval == 0)
				retval = put_user(tmp, (int __user *)arg);
			break;

		case SCULL_IOCHQSET:
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp = scull_qset;
			scull_qset = arg;
			return tmp;

		case SCULL_P_IOCTSIZE:
			scull_p_buffer = arg;
			break;

		case SCULL_P_IOCQSIZE:
			return scull_p_buffer;

		default:  
			return -ENOTTY;
	}
	
	return retval;
}

loff_t scull_llseek(struct file *filp, loff_t off, int whence)
{
	struct scull_dev *dev = filp->private_data;
	loff_t newpos;

	switch(whence) {
		case 0: 
			newpos = off;
			break;
			
		case 1: 
			newpos = filp->f_pos + off;
			break;

		case 2: 
			newpos = dev->size + off;
			break;

		default: 
			return -EINVAL;
	}
	
	if (newpos < 0) 
		return -EINVAL;
	
	filp->f_pos = newpos;
	
	return newpos;
}

struct file_operations scull_fops = {
	.owner =    THIS_MODULE,
	.llseek =   scull_llseek,
	.read =     scull_read,
	.write =    scull_write,
	.ioctl =    scull_ioctl,
	.open =     scull_open,
	.release =  scull_release,
};

void scull_cleanup_module(void)
{
	int i;
	dev_t devno;

	devno = MKDEV(scull_major, scull_minor)

	if (scull_devices) {
		for (i = 0; i < scull_nr_devs; i++) {
			scull_trim(scull_devices + i);
			cdev_del(&scull_devices[i].cdev);
		}
		kfree(scull_devices);
	}

#ifdef SCULL_DEBUG 
	scull_remove_proc();
#endif

	unregister_chrdev_region(devno, scull_nr_devs);

	scull_p_cleanup();
	scull_access_cleanup();
}

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err, devno;

	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	init_MUTEX(dev->sem);

	devno = MKDEV(scull_major, scull_minor + index);
	
	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_fops;
	
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

/**********************************************************
 * API as follow:
 * 	mknod("/dev/scull", S_IFCHR|0666, makedev(scull_major, 0));
 * 	scull_fd = open("/dev/scull", O_RDWR);
 *
**********************************************************/
int scull_init_module(void)
{
	int result, i;
	dev_t dev = 0;

	if (scull_major) {
		dev = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(dev, scull_nr_devs, "scull");
	} else {
		result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");
		scull_major = MAJOR(dev);
	}
	
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", scull_major);
		return result;
	}

	scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
	if (!scull_devices) {
		result = -ENOMEM;
		goto fail; 
	}
	memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

	for (i = 0; i < scull_nr_devs; i++) 
		scull_setup_cdev(&scull_devices[i], i);

	dev = MKDEV(scull_major, scull_minor + scull_nr_devs);
	dev += scull_p_init(dev);
	dev += scull_access_init(dev);

#ifdef SCULL_DEBUG 
	scull_create_proc();
#endif

	return 0; 

fail:
	scull_cleanup_module();
	
	return result;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);
