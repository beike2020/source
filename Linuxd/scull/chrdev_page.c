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
#include <linux/vmalloc.h>
#include <linux/mm.h>		
#include <asm/uaccess.h>
#include <asm/pgtable.h>

#include "chrdev_page.h"

int scullp_major 	=  SCULLP_MAJOR;
int scullp_devs 	=  SCULLP_DEVS;	
int scullp_qset	 	=  SCULLP_QSET;
int scullp_order 	=  SCULLP_ORDER;

module_param(scullp_major, int, 0);
module_param(scullp_devs, int, 0);
module_param(scullp_qset, int, 0);
module_param(scullp_order, int, 0);

MODULE_AUTHOR("Alessandro Rubini");
MODULE_LICENSE("Dual BSD/GPL");

struct scullp_dev *scullp_devices; 
struct async_work {
	struct kiocb *iocb;
	int result;
	struct work_struct work;
};

void scullp_cleanup(void);

int scullp_trim(struct scullp_dev *dev)
{
	struct scullp_dev *next, *dptr;
	int qset = dev->qset; 
	int i;

	if (dev->vmas) 
		return -EBUSY;

	for (dptr = dev; dptr; dptr = next) { 
		if (dptr->data) {
			for (i = 0; i < qset; i++) {
				if (dptr->data[i])
					free_pages((unsigned long)(dptr->data[i]), dptr->order);		//vfree(dptr->data[i]);
			}
			
			kfree(dptr->data);
			dptr->data=NULL;
		}
		
		next=dptr->next;
		if (dptr != dev) 
			kfree(dptr); 
	}
	
	dev->size = 0;
	dev->qset = scullp_qset;
	dev->order = scullp_order;
	dev->next = NULL;
	
	return 0;
}

struct scullp_dev *scullp_follow(struct scullp_dev *dev, int n)
{
	while (n--) {
		if (dev->next == NULL) {
			dev->next = kmalloc(sizeof(struct scullp_dev), GFP_KERNEL);
			memset(dev->next, 0, sizeof(struct scullp_dev));
		}
		
		dev = dev->next;
		continue;
	}
	
	return dev;
}

int scullp_open (struct inode *inode, struct file *filp)
{
	struct scullp_dev *dev; 

	dev = container_of(inode->i_cdev, struct scullp_dev, cdev);

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
		
		scullp_trim(dev); 
		up(&dev->sem);
	}

	filp->private_data = dev;

	return 0;          
}

int scullp_release (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t scullp_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scullp_dev *dev = filp->private_data; 
	struct scullp_dev *dptr;
	int quantum = PAGE_SIZE << dev->order;
	int qset = dev->qset;
	int itemsize = quantum * qset; 
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	if (down_interruptible (&dev->sem))
		return -ERESTARTSYS;
	
	if (*f_pos > dev->size) 
		goto nothing;
	
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;
	
	item = ((long) *f_pos) / itemsize;
	rest = ((long) *f_pos) % itemsize;
	s_pos = rest / quantum; 
	q_pos = rest % quantum;

	dptr = scullp_follow(dev, item);

	if (!dptr->data || !dptr->data[s_pos])
		goto nothing; 
		
	if (count > quantum - q_pos)
		count = quantum - q_pos; 

	if (copy_to_user (buf, dptr->data[s_pos]+q_pos, count)) {
		retval = -EFAULT;
		goto nothing;
	}
	
	up(&dev->sem);
	*f_pos += count;
	return count;

nothing:
	up (&dev->sem);
	return retval;
}

ssize_t scullp_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scullp_dev *dev = filp->private_data;
	struct scullp_dev *dptr;
	int quantum = PAGE_SIZE << dev->order;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM; 

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	item = ((long) *f_pos) / itemsize;
	rest = ((long) *f_pos) % itemsize;
	s_pos = rest / quantum; 
	q_pos = rest % quantum;

	dptr = scullp_follow(dev, item);
	if (dptr->data == NULL) {
		dptr->data = kmalloc(qset * sizeof(void *), GFP_KERNEL);
		if (dptr->data == NULL)
			goto nomem;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	
	if (dptr->data[s_pos] == NULL) {
		dptr->data[s_pos] = (void *)__get_free_pages(GFP_KERNEL, dptr->order);		//dptr->data[s_pos] = (void *)vmalloc(PAGE_SIZE << dptr->order);
		if (dptr->data[s_pos] == NULL)
			goto nomem;
		memset(dptr->data[s_pos], 0, PAGE_SIZE << dptr->order);
	}
	
	if (count > quantum - q_pos)
		count = quantum - q_pos; 
	
	if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count)) {
		retval = -EFAULT;
		goto nomem;
	}
	
	*f_pos += count;
 
	if (dev->size < *f_pos)
		dev->size = *f_pos;
	
	up (&dev->sem);
	return count;

nomem:
	up (&dev->sem);
	return retval;
}

loff_t scullp_llseek (struct file *filp, loff_t off, int whence)
{
	struct scullp_dev *dev = filp->private_data;
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

int scullp_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, ret = 0, tmp;

	if (_IOC_TYPE(cmd) != SCULLP_IOC_MAGIC) 
		return -ENOTTY;
	
	if (_IOC_NR(cmd) > SCULLP_IOC_MAXNR) 
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}
	
	if (err)
		return -EFAULT;

	switch(cmd) {
		case SCULLP_IOCRESET:
			scullp_qset = SCULLP_QSET;
			scullp_order = SCULLP_ORDER;
			break;

		case SCULLP_IOCSORDER: 
			ret = __get_user(scullp_order, (int __user *) arg);
			break;
			
		case SCULLP_IOCTORDER: 
			scullp_order = arg;
			break;

		case SCULLP_IOCGORDER: 
			ret = __put_user (scullp_order, (int __user *) arg);
			break;

		case SCULLP_IOCQORDER: 
			return scullp_order;

		case SCULLP_IOCXORDER: 
			tmp = scullp_order;
			ret = __get_user(scullp_order, (int __user *) arg);
			if (ret == 0)
				ret = __put_user(tmp, (int __user *) arg);
			break;

		case SCULLP_IOCHORDER: 
			tmp = scullp_order;
			scullp_order = arg;
			return tmp;

		case SCULLP_IOCSQSET:
			ret = __get_user(scullp_qset, (int __user *) arg);
			break;

		case SCULLP_IOCTQSET:
			scullp_qset = arg;
			break;

		case SCULLP_IOCGQSET:
			ret = __put_user(scullp_qset, (int __user *)arg);
			break;

		case SCULLP_IOCQQSET:
			return scullp_qset;

		case SCULLP_IOCXQSET:
			tmp = scullp_qset;
			ret = __get_user(scullp_qset, (int __user *)arg);
			if (ret == 0)
				ret = __put_user(tmp, (int __user *)arg);
			break;

		case SCULLP_IOCHQSET:
			tmp = scullp_qset;
			scullp_qset = arg;
			return tmp;

		default:  
			return -ENOTTY;
	}

	return ret;
}

void scullp_vma_open(struct vm_area_struct *vma)
{
	struct scullp_dev *dev = vma->vm_private_data;
	
	dev->vmas++;
}

void scullp_vma_close(struct vm_area_struct *vma)
{
	struct scullp_dev *dev = vma->vm_private_data;
	
	dev->vmas--;
}

struct page *scullp_vma_nopage(struct vm_area_struct *vma, unsigned long address, int *type)
{
	unsigned long offset;
	struct scullp_dev *ptr, *dev = vma->vm_private_data;
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
	
	page = virt_to_page(pageptr);													//page = vmalloc_to_page(pageptr);
	get_page(page);
	
	if (type)
		*type = VM_FAULT_MINOR;
	
out:
	up(&dev->sem);
	return page;
}

struct vm_operations_struct scullp_vm_ops = {
	.open =     scullp_vma_open,
	.close =    scullp_vma_close,
	.nopage =   scullp_vma_nopage,
};

int scullp_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct inode *inode = filp->f_dentry->d_inode;

	if (scullp_devices[iminor(inode)].order)
		return -ENODEV;

	vma->vm_ops = &scullp_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	vma->vm_private_data = filp->private_data;
	scullp_vma_open(vma);
	
	return 0;
}

static void scullp_do_deferred_op(void *p)
{
	struct async_work *stuff = (struct async_work *) p;
	
	aio_complete(stuff->iocb, stuff->result, 0);
	kfree(stuff);
}

static int scullp_defer_op(int write, struct kiocb *iocb, char __user *buf, size_t count, loff_t pos)
{
	struct async_work *stuff;
	int result;

	if (write) {
		result = scullp_write(iocb->ki_filp, buf, count, &pos);
	} else {
		result = scullp_read(iocb->ki_filp, buf, count, &pos);
	}

	if (is_sync_kiocb(iocb))
		return result;

	stuff = kmalloc (sizeof(*stuff), GFP_KERNEL);
	if (stuff == NULL)
		return result;
	
	stuff->iocb = iocb;
	stuff->result = result;
	
	INIT_WORK(&stuff->work, scullp_do_deferred_op, stuff);
	schedule_delayed_work(&stuff->work, HZ/100);
	
	return -EIOCBQUEUED;
}

static ssize_t scullp_aio_read(struct kiocb *iocb, char __user *buf, size_t count, loff_t pos)
{
	return scullp_defer_op(0, iocb, buf, count, pos);
}

static ssize_t scullp_aio_write(struct kiocb *iocb, const char __user *buf, size_t count, loff_t pos)
{
	return scullp_defer_op(1, iocb, (char __user *) buf, count, pos);
}

struct file_operations scullp_fops = {
	.owner 		=   THIS_MODULE,
	.llseek 	=   scullp_llseek,
	.read 		=	scullp_read,
	.write 		=   scullp_write,
	.ioctl 		=   scullp_ioctl,
	.mmap 		=	scullp_mmap,
	.open 		=	scullp_open,
	.release 	=   scullp_release,
	.aio_read 	=  	scullp_aio_read,
	.aio_write 	=	scullp_aio_write,
};

static void scullp_setup_cdev(struct scullp_dev *dev, int index)
{
	int err, devno = MKDEV(scullp_major, index);
    
	cdev_init(&dev->cdev, &scullp_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scullp_fops;
	
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

#ifdef SCULLP_USE_PROC 
void scullp_proc_offset(char *buf, char **start, off_t *offset, int *len)
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

int scullp_read_procmem(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int i, j, order, qset, len = 0;
	int limit = count - 80; 
	struct scullp_dev *d;

	*start = buf;
	for(i = 0; i < scullp_devs; i++) {
		d = &scullp_devices[i];
		
		if (down_interruptible (&d->sem))
			return -ERESTARTSYS;

		qset = d->qset;  
		order = d->order;
		len += sprintf(buf+len,"\nDevice %i: qset %i, order %i, sz %li\n", i, qset, order, (long)(d->size));

		for (; d; d = d->next) { 
			len += sprintf(buf+len,"  item at %p, qset at %p\n", d, d->data);
			scullp_proc_offset (buf, start, &offset, &len);
			
			if (len > limit)
				goto out;

			if (d->data && !d->next) 
				for (j = 0; j < qset; j++) {
					if (d->data[j])
						len += sprintf(buf+len,"    % 4i:%8p\n", j, d->data[j]);
					
					scullp_proc_offset (buf, start, &offset, &len);
					
					if (len > limit)
						goto out;
				}
		}

out:
		up(&scullp_devices[i].sem);
		
		if (len > limit)
			break;
	}
	
	*eof = 1;
	
	return len;
}
#endif 

int scullp_init(void)
{
	int result, i;
	dev_t dev;
	
	if (scullp_major) {
		dev = MKDEV(scullp_major, 0);
		result = register_chrdev_region(dev, scullp_devs, "scullp");
	} else {
		result = alloc_chrdev_region(&dev, 0, scullp_devs, "scullp");
		scullp_major = MAJOR(dev);
	}
	
	if (result < 0)
		return result;

	scullp_devices = kmalloc(scullp_devs * sizeof(struct scullp_dev), GFP_KERNEL);
	if (scullp_devices == NULL) {
		result = -ENOMEM;
		goto fail_malloc;
	}
	memset(scullp_devices, 0, scullp_devs * sizeof(struct scullp_dev));
	
	for (i = 0; i < scullp_devs; i++) {
		scullp_devices[i].order = scullp_order;
		scullp_devices[i].qset = scullp_qset;
		sema_init(&scullp_devices[i].sem, 1);
		scullp_setup_cdev(scullp_devices + i, i);
	}

#ifdef SCULLP_USE_PROC 
	create_proc_read_entry("scullpmem", 0, NULL, scullp_read_procmem, NULL);
#endif
	return 0;

fail_malloc:
	unregister_chrdev_region(dev, scullp_devs);
	return result;
}

void scullp_cleanup(void)
{
	int i;

#ifdef SCULLP_USE_PROC
	remove_proc_entry("scullpmem", NULL);
#endif

	for (i = 0; i < scullp_devs; i++) {
		cdev_del(&scullp_devices[i].cdev);
		scullp_trim(scullp_devices + i);
	}
	
	kfree(scullp_devices);
	
	unregister_chrdev_region(MKDEV(scullp_major, 0), scullp_devs);
}

module_init(scullp_init);
module_exit(scullp_cleanup);
