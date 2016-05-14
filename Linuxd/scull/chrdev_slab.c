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
#include <linux/aio.h>
#include <linux/mm.h>		
#include <asm/uaccess.h>
#include <asm/pgtable.h>

#include "chrdev_slab.h"		

int scullc_major 	= SCULLC_MAJOR;
int scullc_devs 	= SCULLC_DEVS;	
int scullc_qset 	= SCULLC_QSET;
int scullc_quantum 	= SCULLC_QUANTUM;
module_param(scullc_major, int, 0);
module_param(scullc_devs, int, 0);
module_param(scullc_qset, int, 0);
module_param(scullc_quantum, int, 0);

MODULE_AUTHOR("Alessandro Rubini");
MODULE_LICENSE("Dual BSD/GPL");

struct scullc_dev *scullc_devices; 
kmem_cache_t *scullc_cache;
struct async_work {
	struct kiocb *iocb;
	int result;
	struct work_struct work;
};

void scullc_cleanup(void);

int scullc_trim(struct scullc_dev *dev)
{
	struct scullc_dev *next, *dptr;
	int qset = dev->qset;   
	int i;

	if (dev->vmas) 
		return -EBUSY;

	for (dptr = dev; dptr; dptr = next) { 
		if (dptr->data) {
			for (i = 0; i < qset; i++) {
				if (dptr->data[i])
					kmem_cache_free(scullc_cache, dptr->data[i]);
			}

			kfree(dptr->data);
			dptr->data=NULL;
		}
		next=dptr->next;

		if (dptr != dev) 
			kfree(dptr); 
	}
	
	dev->size = 0;
	dev->qset = scullc_qset;
	dev->quantum = scullc_quantum;
	dev->next = NULL;
	
	return 0;
}

struct scullc_dev *scullc_follow(struct scullc_dev *dev, int n)
{
	while (n--) {
		if (dev->next == NULL) {
			dev->next = kmalloc(sizeof(struct scullc_dev), GFP_KERNEL);
			memset(dev->next, 0, sizeof(struct scullc_dev));
		}
		
		dev = dev->next;
		continue;
	}
	
	return dev;
}

int scullc_open (struct inode *inode, struct file *filp)
{
	struct scullc_dev *dev; 

	dev = container_of(inode->i_cdev, struct scullc_dev, cdev);

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible (&dev->sem))
			return -ERESTARTSYS;
		
		scullc_trim(dev);
		up(&dev->sem);
	}

	filp->private_data = dev;

	return 0;  
}

int scullc_release (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t scullc_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scullc_dev *dev = filp->private_data;
	struct scullc_dev *dptr;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset; 
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	
	if (*f_pos > dev->size) 
		goto nothing;
	
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;
	
	item = ((long) *f_pos) / itemsize;
	rest = ((long) *f_pos) % itemsize;
	s_pos = rest / quantum; 
	q_pos = rest % quantum;

	dptr = scullc_follow(dev, item);

	if (!dptr->data || !dptr->data[s_pos])
		goto nothing; 
		
	if (count > quantum - q_pos)
		count = quantum - q_pos; 

	if (copy_to_user(buf, dptr->data[s_pos]+q_pos, count)) {
		retval = -EFAULT;
		goto nothing;
	}
	
	up(&dev->sem);
	*f_pos += count;
	return count;

nothing:
	up(&dev->sem);
	return retval;
}

ssize_t scullc_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scullc_dev *dev = filp->private_data;
	struct scullc_dev *dptr;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM; 

	if (down_interruptible (&dev->sem))
		return -ERESTARTSYS;

	item = ((long) *f_pos) / itemsize;
	rest = ((long) *f_pos) % itemsize;
	s_pos = rest / quantum; 
	q_pos = rest % quantum;

	dptr = scullc_follow(dev, item);
	
	if (dptr->data == NULL) {
		dptr->data = kmalloc(qset * sizeof(void *), GFP_KERNEL);
		if (dptr->data == NULL)
			goto nomem;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	
	if (dptr->data[s_pos] == NULL) {
		dptr->data[s_pos] = kmem_cache_alloc(scullc_cache, GFP_KERNEL);
		if (dptr->data[s_pos] == NULL)
			goto nomem;
		memset(dptr->data[s_pos], 0, scullc_quantum);
	}
	
	if (count > quantum - q_pos)
		count = quantum - q_pos; 
	
	if (copy_from_user (dptr->data[s_pos]+q_pos, buf, count)) {
		retval = -EFAULT;
		goto nomem;
	}
	
	*f_pos += count;
 
	if (dev->size < *f_pos)
		dev->size = *f_pos;
	
	up(&dev->sem);
	return count;

nomem:
	up(&dev->sem);
	return retval;
}

loff_t scullc_llseek (struct file *filp, loff_t off, int whence)
{
	struct scullc_dev *dev = filp->private_data;
	long newpos;

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

int scullc_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, ret = 0, tmp;

	if (_IOC_TYPE(cmd) != SCULLC_IOC_MAGIC) 
		return -ENOTTY;
	
	if (_IOC_NR(cmd) > SCULLC_IOC_MAXNR) 
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}
	
	if (err)
		return -EFAULT;

	switch(cmd) {
		case SCULLC_IOCRESET:
			scullc_qset = SCULLC_QSET;
			scullc_quantum = SCULLC_QUANTUM;
			break;

		case SCULLC_IOCSQUANTUM:
			ret = __get_user(scullc_quantum, (int __user *) arg);
			break;

		case SCULLC_IOCTQUANTUM: 
			scullc_quantum = arg;
			break;

		case SCULLC_IOCGQUANTUM: 
			ret = __put_user(scullc_quantum, (int __user *) arg);
			break;

		case SCULLC_IOCQQUANTUM: 
			return scullc_quantum;

		case SCULLC_IOCXQUANTUM: 
			tmp = scullc_quantum;
			ret = __get_user(scullc_quantum, (int __user *) arg);
			if (ret == 0)
				ret = __put_user(tmp, (int __user *) arg);
			break;

		case SCULLC_IOCHQUANTUM: 
			tmp = scullc_quantum;
			scullc_quantum = arg;
			return tmp;

		case SCULLC_IOCSQSET:
			ret = __get_user(scullc_qset, (int __user *) arg);
			break;

		case SCULLC_IOCTQSET:
			scullc_qset = arg;
			break;

		case SCULLC_IOCGQSET:
			ret = __put_user(scullc_qset, (int __user *)arg);
			break;

		case SCULLC_IOCQQSET:
			return scullc_qset;

		case SCULLC_IOCXQSET:
			tmp = scullc_qset;
			ret = __get_user(scullc_qset, (int __user *)arg);
			if (ret == 0)
				ret = __put_user(tmp, (int __user *)arg);
			break;

		case SCULLC_IOCHQSET:
			tmp = scullc_qset;
			scullc_qset = arg;
			return tmp;

		default: 
			return -ENOTTY;
	}

	return ret;
}

void scullc_vma_open(struct vm_area_struct *vma)
{
	struct scullc_dev *dev = vma->vm_private_data;

	dev->vmas++;
}

void scullc_vma_close(struct vm_area_struct *vma)
{
	struct scullc_dev *dev = vma->vm_private_data;

	dev->vmas--;
}

struct page *scullc_vma_nopage(struct vm_area_struct *vma, unsigned long address, int *type)
{
	unsigned long offset;
	struct scullc_dev *ptr, *dev = vma->vm_private_data;
	struct page *page = NOPAGE_SIGBUS;
	void *pageptr = NULL; 

	down(&dev->sem);
	offset = (address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT);
	if (offset >= dev->size) 
		goto out; 

	offset >>= PAGE_SHIFT;

	for (ptr = dev; ptr && offset >= dev->qset;) {
		ptr = ptr->next;
		offset -= dev->qset;
	}
	
	if (ptr && ptr->data) 
		pageptr = ptr->data[offset];
	
	if (pageptr == NULL) 
		goto out; 

	page = virt_to_page(pageptr);
	get_page(page);
	
	if (type)
		*type = VM_FAULT_MINOR;
	
out:
	up(&dev->sem);
	return page;
}

struct vm_operations_struct scullc_vm_ops = {
	.open =     scullc_vma_open,
	.close =    scullc_vma_close,
	.nopage =   scullc_vma_nopage,
};

int scullc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct inode *inode = filp->f_dentry->d_inode;

	if (scullc_devices[iminor(inode)].quantum != PAGE_SIZE)
		return -ENODEV;

	vma->vm_ops = &scullc_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	vma->vm_private_data = filp->private_data;
	scullc_vma_open(vma);
	
	return 0;
}

static void scullc_do_deferred_op(void *p)
{
	struct async_work *stuff = (struct async_work *) p;
	
	aio_complete(stuff->iocb, stuff->result, 0);
	kfree(stuff);
}

static int scullc_defer_op(int write, struct kiocb *iocb, char __user *buf, size_t count, loff_t pos)
{
	struct async_work *stuff;
	int result;

	if (write) {
		result = scullc_write(iocb->ki_filp, buf, count, &pos);
	} else {
		result = scullc_read(iocb->ki_filp, buf, count, &pos);
	}
	
	if (is_sync_kiocb(iocb))
		return result;

	stuff = kmalloc (sizeof (*stuff), GFP_KERNEL);
	if (stuff == NULL)
		return result; 
	stuff->iocb = iocb;
	stuff->result = result;
	
	INIT_WORK(&stuff->work, scullc_do_deferred_op, stuff);
	schedule_delayed_work(&stuff->work, HZ/100);
	
	return -EIOCBQUEUED;
}

static ssize_t scullc_aio_read(struct kiocb *iocb, char __user *buf, size_t count, loff_t pos)
{
	return scullc_defer_op(0, iocb, buf, count, pos);
}

static ssize_t scullc_aio_write(struct kiocb *iocb, const char __user *buf, size_t count, loff_t pos)
{
	return scullc_defer_op(1, iocb, (char __user *) buf, count, pos);
}

struct file_operations scullc_fops = {
	.owner 		=   THIS_MODULE,
	.llseek 	=   scullc_llseek,
	.read 		=	scullc_read,
	.write 		=   scullc_write,
	.ioctl 		=   scullc_ioctl,
	.mmap 		=   scullc_mmap,
	.open 		=	scullc_open,
	.release 	=   scullc_release,
	.aio_read 	=	scullc_aio_read,
	.aio_write 	=	scullc_aio_write,
};

static void scullc_setup_cdev(struct scullc_dev *dev, int index)
{
	int err, devno = MKDEV(scullc_major, index);
    
	cdev_init(&dev->cdev, &scullc_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scullc_fops;
	
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

#ifdef SCULLC_USE_PROC 
void scullc_proc_offset(char *buf, char **start, off_t *offset, int *len)
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

int scullc_read_procmem(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int i, j, quantum, qset, len = 0;
	int limit = count - 80; 
	struct scullc_dev *d;

	*start = buf;
	for(i = 0; i < scullc_devs; i++) {
		d = &scullc_devices[i];
		
		if (down_interruptible (&d->sem))
			return -ERESTARTSYS;

		qset = d->qset;  
		quantum = d->quantum;
		len += sprintf(buf+len,"\nDevice %i: qset %i, quantum %i, sz %li\n", i, qset, quantum, (long)(d->size));

		for (; d; d = d->next) {
			len += sprintf(buf+len,"  item at %p, qset at %p\n", d, d->data);
			scullc_proc_offset (buf, start, &offset, &len);
			
			if (len > limit)
				goto out;

			if (d->data && !d->next) 
				for (j = 0; j < qset; j++) {
					if (d->data[j])
						len += sprintf(buf+len,"    % 4i:%8p\n", j, d->data[j]);
					
					scullc_proc_offset (buf, start, &offset, &len);
					
					if (len > limit)
						goto out;
				}
		}

out:
		up(&scullc_devices[i].sem);
		if (len > limit)
			break;
	}
	
	*eof = 1;
	
	return len;
}
#endif 

int scullc_init(void)
{
	int result, i;
	dev_t dev;
	
	if (scullc_major) {
		dev = MKDEV(scullc_major, 0);
		result = register_chrdev_region(dev, scullc_devs, "scullc");
	} else {
		result = alloc_chrdev_region(&dev, 0, scullc_devs, "scullc");
		scullc_major = MAJOR(dev);
	}
	
	if (result < 0)
		return result;

	scullc_devices = kmalloc(scullc_devs * sizeof(struct scullc_dev), GFP_KERNEL);
	if (scullc_devices == NULL) {
		result = -ENOMEM;
		goto fail_malloc;
	}
	memset(scullc_devices, 0, scullc_devs * sizeof(struct scullc_dev));
	
	for (i = 0; i < scullc_devs; i++) {
		scullc_devices[i].quantum = scullc_quantum;
		scullc_devices[i].qset = scullc_qset;
		sema_init (&scullc_devices[i].sem, 1);
		scullc_setup_cdev(scullc_devices + i, i);
	}

	scullc_cache = kmem_cache_create("scullc", scullc_quantum, 0, SLAB_HWCACHE_ALIGN, NULL, NULL);
	if (scullc_cache == NULL) {
		scullc_cleanup();
		return -ENOMEM;
	}

#ifdef SCULLC_USE_PROC 
	create_proc_read_entry("scullcmem", 0, NULL, scullc_read_procmem, NULL);
#endif

	return 0; 

fail_malloc:
	unregister_chrdev_region(dev, scullc_devs);
	return result;
}

void scullc_cleanup(void)
{
	int i;

#ifdef SCULLC_USE_PROC
	remove_proc_entry("scullcmem", NULL);
#endif

	for (i = 0; i < scullc_devs; i++) {
		cdev_del(&scullc_devices[i].cdev);
		scullc_trim(scullc_devices + i);
	}
	
	kfree(scullc_devices);

	if (scullc_cache)
		kmem_cache_destroy(scullc_cache);
	
	unregister_chrdev_region(MKDEV(scullc_major, 0), scullc_devs);
}

module_init(scullc_init);
module_exit(scullc_cleanup);
