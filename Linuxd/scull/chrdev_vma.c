#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>  
#include <linux/slab.h>   
#include <linux/fs.h>      
#include <linux/errno.h>    
#include <linux/types.h>    
#include <linux/mm.h>
#include <linux/kdev_t.h>
#include <asm/page.h>
#include <linux/cdev.h>
#include <linux/device.h>

static int simple_major = 0;
module_param(simple_major, int, 0);

MODULE_AUTHOR("Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

#define MAX_SIMPLE_DEV 2
static struct cdev SimpleDevs[MAX_SIMPLE_DEV];

static int simple_open (struct inode *inode, struct file *filp)
{
	return 0;
}

static int simple_release(struct inode *inode, struct file *filp)
{
	return 0;
}

void simple_vma_open(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE "Simple VMA open, virt %lx, phys %lx\n", vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

void simple_vma_close(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE "Simple VMA close.\n");
}

static struct vm_operations_struct simple_remap_vm_ops = {
	.open 		=	simple_vma_open,
	.close 		= 	simple_vma_close,
};

static int simple_remap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	vma->vm_ops = &simple_remap_vm_ops;
	simple_vma_open(vma);
	
	return 0;
}

static struct file_operations simple_remap_ops = {
	.owner   	= 	THIS_MODULE,
	.open    	= 	simple_open,
	.release 	= 	simple_release,
	.mmap    	= 	simple_remap_mmap,
};

struct page *simple_vma_nopage(struct vm_area_struct *vma, unsigned long address, int *type)
{
	struct page *pageptr;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long physaddr = address - vma->vm_start + offset;
	unsigned long pageframe = physaddr >> PAGE_SHIFT;

	printk(KERN_NOTICE "---- Nopage, off %lx phys %lx\n", offset, physaddr);
	printk(KERN_NOTICE "VA is %p\n", __va (physaddr));
	printk(KERN_NOTICE "Page at %p\n", virt_to_page (__va (physaddr)));
	printk(KERN_NOTICE "Page frame %ld\n", pageframe);
	
	if (!pfn_valid(pageframe))
		return NOPAGE_SIGBUS;
	
	pageptr = pfn_to_page(pageframe);
	printk(KERN_NOTICE "page->index = %ld mapping %p\n", pageptr->index, pageptr->mapping);
	
	get_page(pageptr);
	
	if (type)
		*type = VM_FAULT_MINOR;
	
	return pageptr;
}

static struct vm_operations_struct simple_nopage_vm_ops = {
	.open =   simple_vma_open,
	.close =  simple_vma_close,
	.nopage = simple_vma_nopage,
};

static int simple_nopage_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	//set flag: io vmap and can't swap out.
	if (offset >= __pa(high_memory) || (filp->f_flags & O_SYNC))
		vma->vm_flags |= VM_IO;
	
	vma->vm_flags |= VM_RESERVED;
	vma->vm_ops = &simple_nopage_vm_ops;
	simple_vma_open(vma);
	
	return 0;
}

static struct file_operations simple_nopage_ops = {
	.owner   	= 	THIS_MODULE,
	.open    	= 	simple_open,
	.release 	= 	simple_release,
	.mmap    	= 	simple_nopage_mmap,
};

static void simple_setup_cdev(struct cdev *dev, int minor, struct file_operations *fops)
{
	int err, devno = MKDEV(simple_major, minor);
    
	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	dev->ops = fops;
	
	err = cdev_add (dev, devno, 1);
	if (err)
		printk (KERN_NOTICE "Error %d adding simple%d", err, minor);
}

static int simple_init(void)
{
	int result;
	dev_t dev;

	if (simple_major){
		dev = MKDEV(simple_major, 0);
		result = register_chrdev_region(dev, 2, "simple");
	} else {
		result = alloc_chrdev_region(&dev, 0, 2, "simple");
		simple_major = MAJOR(dev);
	}
	
	if (result < 0) {
		printk(KERN_WARNING "Unable to get major %d\n", simple_major);
		return result;
	}

	//only apply to map reserve page[ 640k - 1M] and I/O memory.
	simple_setup_cdev(SimpleDevs, 0, &simple_remap_ops);

	//only apply to map RAW.
	simple_setup_cdev(SimpleDevs + 1, 1, &simple_nopage_ops);
	
	return 0;
}

static void simple_cleanup(void)
{
	cdev_del(SimpleDevs);
	cdev_del(SimpleDevs + 1);
	
	unregister_chrdev_region(MKDEV(simple_major, 0), 2);
}

module_init(simple_init);
module_exit(simple_cleanup);
