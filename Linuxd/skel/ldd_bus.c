#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>

#include "ldd_bus.h"		

MODULE_AUTHOR("Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

int sculld_major 	=  SCULLD_MAJOR;
int sculld_devs 	=  SCULLD_DEVS;	
int sculld_qset 	=  SCULLD_QSET;
int sculld_order 	=  SCULLD_ORDER;
module_param(sculld_major, int, 0);
module_param(sculld_devs, int, 0);
module_param(sculld_qset, int, 0);
module_param(sculld_order, int, 0);

struct sculld_dev *sculld_devices; 
struct ldd_driver sculld_driver = {
	.version 	= 	"$Revision: 1.21 $",
	.module 	= 	THIS_MODULE,
	.driver 	= 	{
		.name 	= 	"sculld",
	},
};

void sculld_vma_open(struct vm_area_struct *vma)
{
	struct sculld_dev *dev = vma->vm_private_data;
	
	dev->vmas++;
}

void sculld_vma_close(struct vm_area_struct *vma)
{
	struct sculld_dev *dev = vma->vm_private_data;
	
	dev->vmas--;
}

struct page *sculld_vma_nopage(struct vm_area_struct *vma, unsigned long address, int *type)
{
	unsigned long offset;
	struct sculld_dev *ptr, *dev = vma->vm_private_data;
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

	if (!pageptr) 
		goto out; 

	get_page(page);
	
	if (type)
		*type = VM_FAULT_MINOR;
	
out:
	up(&dev->sem);
	return page;
}

struct vm_operations_struct sculld_vm_ops = {
	.open 		=   sculld_vma_open,
	.close 		=   sculld_vma_close,
	.nopage 	=   sculld_vma_nopage,
};

int sculld_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct inode *inode = filp->f_dentry->d_inode;

	if (sculld_devices[iminor(inode)].order)
		return -ENODEV;

	vma->vm_ops = &sculld_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	vma->vm_private_data = filp->private_data;
	sculld_vma_open(vma);
	
	return 0;
}

int sculld_trim(struct sculld_dev *dev)
{
	struct sculld_dev *next, *dptr;
	int qset = dev->qset;  
	int i;

	if (dev->vmas)
		return -EBUSY;

	for (dptr = dev; dptr; dptr = next) { 
		if (dptr->data) {
			for (i = 0; i < qset; i++)
				if (dptr->data[i])
					free_pages((unsigned long)(dptr->data[i]), dptr->order);

			kfree(dptr->data);
			dptr->data=NULL;
		}
		
		next=dptr->next;
		
		if (dptr != dev) 
			kfree(dptr); 
	}
	
	dev->size = 0;
	dev->qset = sculld_qset;
	dev->order = sculld_order;
	dev->next = NULL;
	
	return 0;
}

int sculld_open (struct inode *inode, struct file *filp)
{
	struct sculld_dev *dev;

	dev = container_of(inode->i_cdev, struct sculld_dev, cdev);

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible (&dev->sem))
			return -ERESTARTSYS;
		
		sculld_trim(dev); 
		up(&dev->sem);
	}

	filp->private_data = dev;

	return 0;         
}

int sculld_release (struct inode *inode, struct file *filp)
{
	return 0;
}

struct sculld_dev *sculld_follow(struct sculld_dev *dev, int n)
{
	while (n--) {
		if (dev->next == NULL) {
			dev->next = kmalloc(sizeof(struct sculld_dev), GFP_KERNEL);
			if (dev->next == NULL)
				return NULL;
			memset(dev->next, 0, sizeof(struct sculld_dev));
		}
		
		dev = dev->next;
		continue;
	}
	
	return dev;
}

ssize_t sculld_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct sculld_dev *dev = filp->private_data; 
	struct sculld_dev *dptr;
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

	dptr = sculld_follow(dev, item);
	if (dptr->data == NULL)
		goto nothing; 
	
	if (dptr->data[s_pos] == NULL)
		goto nothing;

	if (count > quantum - q_pos)
		count = quantum - q_pos; 

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
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

ssize_t sculld_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct sculld_dev *dev = filp->private_data;
	struct sculld_dev *dptr;
	int quantum = PAGE_SIZE << dev->order;
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

	dptr = sculld_follow(dev, item);
	if (dptr->data == NULL) {
		dptr->data = kmalloc(qset * sizeof(void *), GFP_KERNEL);
		if (dptr->data == NULL)
			goto nomem;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	
	if (dptr->data[s_pos] == NULL) {
		dptr->data[s_pos] = (void *)__get_free_pages(GFP_KERNEL, dptr->order);
		if (dptr->data[s_pos] == NULL)
			goto nomem;
		memset(dptr->data[s_pos], 0, PAGE_SIZE << dptr->order);
	}

	if (count > quantum - q_pos)
		count = quantum - q_pos; 
	
	if (copy_from_user(dptr->data[s_pos]+q_pos, buf, count)) {
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

loff_t sculld_llseek (struct file *filp, loff_t off, int whence)
{
	struct sculld_dev *dev = filp->private_data;
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

int sculld_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, ret = 0, tmp;

	if (_IOC_TYPE(cmd) != SCULLD_IOC_MAGIC) 
		return -ENOTTY;
	
	if (_IOC_NR(cmd) > SCULLD_IOC_MAXNR) 
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	
	if (err)
		return -EFAULT;

	switch(cmd) {
		case SCULLD_IOCRESET:
			sculld_qset = SCULLD_QSET;
			sculld_order = SCULLD_ORDER;
			break;

		case SCULLD_IOCSORDER: 
			ret = __get_user(sculld_order, (int __user *) arg);
			break;
			
		case SCULLD_IOCTORDER: 
			sculld_order = arg;
			break;

		case SCULLD_IOCGORDER: 
			ret = __put_user(sculld_order, (int __user *) arg);
			break;

		case SCULLD_IOCQORDER: 
			return sculld_order;

		case SCULLD_IOCXORDER: 
			tmp = sculld_order;
			ret = __get_user(sculld_order, (int __user *) arg);
			if (ret == 0)
				ret = __put_user(tmp, (int __user *) arg);
			break;

		case SCULLD_IOCHORDER:
			tmp = sculld_order;
			sculld_order = arg;
			return tmp;

		case SCULLD_IOCSQSET:
			ret = __get_user(sculld_qset, (int __user *) arg);
			break;

		case SCULLD_IOCTQSET:
			sculld_qset = arg;
			break;

		case SCULLD_IOCGQSET:
			ret = __put_user(sculld_qset, (int __user *)arg);
			break;

		case SCULLD_IOCQQSET:
			return sculld_qset;

		case SCULLD_IOCXQSET:
			tmp = sculld_qset;
			ret = __get_user(sculld_qset, (int __user *)arg);
			if (ret == 0)
				ret = __put_user(tmp, (int __user *)arg);
			break;

		case SCULLD_IOCHQSET:
			tmp = sculld_qset;
			sculld_qset = arg;
			return tmp;

		default:  
			return -ENOTTY;
	}

	return ret;
}

static void sculld_do_deferred_op(void *p)
{
	struct async_work *stuff = (struct async_work *) p;
	
	aio_complete(stuff->iocb, stuff->result, 0);
	kfree(stuff);
}

static int sculld_defer_op(int write, struct kiocb *iocb, char __user *buf, size_t count, loff_t pos)
{
	struct async_work *stuff;
	int result;

	if (write) {
		result = sculld_write(iocb->ki_filp, buf, count, &pos);
	} else {
		result = sculld_read(iocb->ki_filp, buf, count, &pos);
	}
	
	if (is_sync_kiocb(iocb))
		return result;

	stuff = kmalloc (sizeof (*stuff), GFP_KERNEL);
	if (stuff == NULL)
		return result;
	
	stuff->iocb = iocb;
	stuff->result = result;
	INIT_WORK(&stuff->work, sculld_do_deferred_op, stuff);
	schedule_delayed_work(&stuff->work, HZ/100);
	
	return -EIOCBQUEUED;
}

static ssize_t sculld_aio_read(struct kiocb *iocb, char __user *buf, size_t count, loff_t pos)
{
	return sculld_defer_op(0, iocb, buf, count, pos);
}

static ssize_t sculld_aio_write(struct kiocb *iocb, const char __user *buf, size_t count, loff_t pos)
{
	return sculld_defer_op(1, iocb, buf, count, pos);
}

struct file_operations sculld_fops = {
	.owner 		=	THIS_MODULE,
	.read 		=	sculld_read,
	.write 		=   sculld_write,
	.llseek 	=	sculld_llseek,
	.ioctl 		=   sculld_ioctl,
	.mmap 		=	sculld_mmap,
	.open 		=	sculld_open,
	.release 	=   sculld_release,
	.aio_read 	=   sculld_aio_read,
	.aio_write	=   sculld_aio_write,
};

static void sculld_setup_cdev(struct sculld_dev *dev, int index)
{
	int err, devno;

	devno = MKDEV(sculld_major, index);
	cdev_init(&dev->cdev, &sculld_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &sculld_fops;
	
	err = cdev_add (&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

static int ldd_match(struct device *dev, struct device_driver *driver)
{
	return !strncmp(dev->bus_id, driver->name, strlen(driver->name));
}

static int ldd_hotplug(struct device *dev, char **envp, int num_envp, char *buffer, int buffer_size)
{
	envp[0] = buffer;
	if (snprintf(buffer, buffer_size, "LDDBUS_VERSION=%s", "$Revision: 1.9 $") >= buffer_size)
		return -ENOMEM;
	
	envp[1] = NULL;
	
	return 0;
}
	
struct bus_type ldd_bus_type = {
	.name 		= 	"ldd",
	.match 		= 	ldd_match,
	.hotplug  	= 	ldd_hotplug,
};

static void ldd_bus_release(struct device *dev)
{
	printk(KERN_DEBUG "lddbus release\n");
}

struct device ldd_bus = {
	.bus_id   	= 	"ldd0",
	.release  	= 	ldd_bus_release
};

static void ldd_dev_release(struct device *dev)
{ 
	return;
}

static ssize_t show_version(struct device_driver *driver, char *buf)
{
	struct ldd_driver *ldriver;

	ldriver = container_of(driver, struct ldd_driver, driver);
	sprintf(buf, "%s\n", ldriver->version);
	
	return strlen(buf);
}

static ssize_t sculld_show_dev(struct device *ddev, char *buf)
{
	struct sculld_dev *dev = ddev->driver_data;

	return print_dev_t(buf, dev->cdev.dev);
}
static DEVICE_ATTR(dev, S_IRUGO, sculld_show_dev, NULL);

static void sculld_register_dev(struct sculld_dev *dev, int index)
{
	sprintf(dev->devname, "sculld%d", index);
	dev->ldev.name = dev->devname;
	
	dev->ldev.driver = &sculld_driver;	
	dev->ldev.driver.driver.bus = &ldd_bus_type;
	dev->ldev.driver.version_attr.attr.name = "version";
	dev->ldev.driver.version_attr.attr.owner = dev->ldev.driver.module;
	dev->ldev.driver.version_attr.attr.mode = S_IRUGO;
	dev->ldev.driver.version_attr.show = show_version;
	dev->ldev.driver.version_attr.store = NULL;
	driver_register(&dev->ldev.driver.driver);
	driver_create_file(&dev->ldev.driver.driver, &dev->ldev.driver.version_attr);
	
	dev->ldev.dev.bus = &ldd_bus_type;
	dev->ldev.dev.parent = &ldd_bus;
	dev->ldev.dev.driver_data = dev;	
	dev->ldev.dev.release = ldd_dev_release;
	strncpy(dev->ldev.dev.bus_id, dev->ldev.name, BUS_ID_SIZE);
	device_register(&dev->ldev.dev);
	device_create_file(&dev->ldev.dev, &dev_attr_dev);
}

void sculld_unregister_dev(struct sculld_dev *dev)
{
	driver_unregister(&dev->ldev.driver.driver);
	device_unregister(&dev->ldev.dev);
}
		
static ssize_t show_bus_version(struct bus_type *bus, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", "$Revision: 1.9 $");
}

static BUS_ATTR(version, S_IRUGO, show_bus_version, NULL);

#ifdef SCULLD_USE_PROC 
void sculld_proc_offset(char *buf, char **start, off_t *offset, int *len)
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

int sculld_read_procmem(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int i, j, order, qset, len = 0;
	int limit = count - 80;
	struct sculld_dev *d;

	*start = buf;
	for(i = 0; i < sculld_devs; i++) {
		d = &sculld_devices[i];
		if (down_interruptible(&d->sem))
			return -ERESTARTSYS;
		
		qset = d->qset;  
		order = d->order;
		len += sprintf(buf+len,"\nDevice %i: qset %i, order %i, sz %li\n", i, qset, order, (long)(d->size));

		for (; d; d = d->next) { 
			len += sprintf(buf+len,"  item at %p, qset at %p\n",d,d->data);
			sculld_proc_offset (buf, start, &offset, &len);
			if (len > limit)
				goto out;
			
			if (d->data && !d->next) 
				for (j = 0; j < qset; j++) {
					if (d->data[j])
						len += sprintf(buf+len,"    % 4i:%8p\n",j,d->data[j]);
					
					sculld_proc_offset (buf, start, &offset, &len);
					
					if (len > limit)
						goto out;
				}
		}

out:
		up (&sculld_devices[i].sem);
		if (len > limit)
			break;
	}
	
	*eof = 1;
	
	return len;
}
#endif 

static int __init ldd_bus_init(void)
{
	int result, i;
	dev_t dev;

	result = bus_register(&ldd_bus_type);
	if (result) {
		printk(KERN_NOTICE "Unable to register bus\n");
		return result;
	}
	
	if (bus_create_file(&ldd_bus_type, &bus_attr_version))
		printk(KERN_NOTICE "Unable to create version attribute\n");
	
	result = device_register(&ldd_bus);
	if (result) {
		printk(KERN_NOTICE "Unable to register ldd0\n");
		return result;
	}

	if (sculld_major) {
		dev = MKDEV(sculld_major, 0);
		result = register_chrdev_region(dev, sculld_devs, "sculld");
	} else {
		result = alloc_chrdev_region(&dev, 0, sculld_devs, "sculld");
		sculld_major = MAJOR(dev);
	}
	
	if (result < 0) {
		printk(KERN_NOTICE "Unable to register chrdev_region\n");
		return result;
	}

	register_ldd_driver(&sculld_driver);
	
	sculld_devices = kmalloc(sculld_devs * sizeof(struct sculld_dev), GFP_KERNEL);
	if (sculld_devices == NULL) {
		result = -ENOMEM;
		unregister_chrdev_region(dev, sculld_devs);
		return result;
	}
	memset(sculld_devices, 0, sculld_devs*sizeof (struct sculld_dev));
	
	for (i = 0; i < sculld_devs; i++) {
		sculld_devices[i].order = sculld_order;
		sculld_devices[i].qset = sculld_qset;
		sema_init (&sculld_devices[i].sem, 1);
		sculld_setup_cdev(sculld_devices + i, i);
		sculld_register_dev(sculld_devices + i, i);
	}

#ifdef SCULLD_USE_PROC 
	create_proc_read_entry("sculldmem", 0, NULL, sculld_read_procmem, NULL);
#endif

	return 0; 
}

static void ldd_bus_exit(void)
{
	int i;

#ifdef SCULLD_USE_PROC
	remove_proc_entry("sculldmem", NULL);
#endif

	for (i = 0; i < sculld_devs; i++) {
		sculld_unregister_dev(&sculld_devices[i];
		cdev_del(&sculld_devices[i].cdev);
		sculld_trim(sculld_devices + i);
	}
	
	kfree(sculld_devices);
	unregister_ldd_driver(&sculld_driver);
	unregister_chrdev_region(MKDEV (sculld_major, 0), sculld_devs);

	device_unregister(&ldd_bus);
	bus_unregister(&ldd_bus_type);
}

module_init(ldd_bus_init);
module_exit(ldd_bus_exit);
