#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h> 
#include <linux/fs.h>	 
#include <linux/errno.h>  
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/uaccess.h>

int isa_major = 0;
module_param(isa_major, int, 0);
MODULE_AUTHOR("Alessandro Rubini");
MODULE_LICENSE("Dual BSD/GPL");

/*
 * The devices access the 640k-1M memory.
 * minor 0 uses ioread8/iowrite8
 * minor 1 uses ioread16/iowrite16
 * minor 2 uses ioread32/iowrite32
 * minor 3 uses memcpy_fromio()/memcpy_toio()
 */
#define ISA_BASE	0xA0000
#define ISA_MAX		0x100000 
#define VIDEO_MAX	0xC0000 
#define VGA_BASE	0xb8000

static void __iomem *io_base;
enum isa_modes {
	M_8=0, M_16, M_32, M_MEMCPY
};

int isa_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int isa_release(struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t isa_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int retval;
	void __iomem *add;
	unsigned char *kbuf, *ptr;
	unsigned long isa_addr = ISA_BASE + *f_pos;
	int mode = iminor(filp->f_dentry->d_inode);

	if (isa_addr + count > ISA_MAX) 
		count = ISA_MAX - isa_addr;
	
	if (count < 0)
		return 0;

	kbuf = kmalloc(count, GFP_KERNEL);
	if (kbuf == NULL)
		return -ENOMEM;
	
	ptr = kbuf;
	retval = count;
	add = (void __iomem *)(io_base + *f_pos);
	
	if (mode == M_32 && ((isa_addr | count) & 3))
		mode = M_16;
	
	if (mode == M_16 && ((isa_addr | count) & 1))
		mode = M_8;

	switch(mode) {
		case M_8: 
			while (count) {
				*ptr = ioread8(add);
				add++;
				count--;
				ptr++;
			}
			break;

		case M_16: 
			while (count >= 2) {
				*(u16 *)ptr = ioread16(add);
				add += 2;
				count -= 2;
				ptr += 2;
			}
			break;

		
		case M_32: 
			while (count >= 4) {
				*(u32 *)ptr = ioread32(add);
				add += 4;
				count -= 4;
				ptr += 4;
			}
			break;

		case M_MEMCPY:
			memcpy_fromio(ptr, add, count);
			break;

		default:
			return -EINVAL;
	}
	
	if ((retval > 0) && copy_to_user(buf, kbuf, retval))
		retval = -EFAULT;
	
	kfree(kbuf);
	*f_pos += retval;
	
	return retval;
}

ssize_t isa_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int retval;
	void __iomem *add;
	unsigned char *kbuf, *ptr;
	unsigned long isa_addr = ISA_BASE + *f_pos;
	int mode = iminor(filp->f_dentry->d_inode);

	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;

	if (isa_addr + count > ISA_MAX) 
		count = ISA_MAX - isa_addr;

	if (count < 0)
		return 0;

	kbuf = kmalloc(count, GFP_KERNEL);
	if (kbuf == NULL)
		return -ENOMEM;

	if (copy_from_user(kbuf, buf, count)) {
		kfree(kbuf);
		return -EFAULT;
	}
	
	ptr = kbuf;
	retval = count;
	add = (void __iomem *)(io_base + *f_pos);
	
	if (mode == M_32 && ((isa_addr | count) & 3))
		mode = M_16;
	
	if (mode == M_16 && ((isa_addr | count) & 1))
		mode = M_8;

	switch(mode) {
		case M_8: 
			while (count) {
				iowrite8(*ptr, add);
				add++;
				count--;
				ptr++;
			}
			break;

		case M_16: 
			while (count >= 2) {
				iowrite8(*(u16 *)ptr, add);
				add += 2;
				count -= 2;
				ptr += 2;
			}
			break;

		case M_32: 
			while (count >= 4) {
				iowrite8(*(u32 *)ptr, add);
				add += 4;
				count -= 4;
				ptr += 4;
			}
			break;

		case M_MEMCPY:
			memcpy_toio(add, ptr, count);
			break;

		default:
			return -EINVAL;
	}
	
	*f_pos += retval;
	kfree(kbuf);
	
	return retval;
}

unsigned int isa_poll(struct file *filp, poll_table *wait)
{
    return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
}

struct file_operations isa_fops = {
	.open 		=	isa_open,
	.release 	=  	isa_release,
	.read 		=	isa_read,
	.write 		=	isa_write,
	.poll 		=	isa_poll,
	.owner 		= 	THIS_MODULE
};

int isa_init(void)
{
	int result = 0;
	
	result = register_chrdev(isa_major, "isa", &isa_fops);
	if (result < 0) 
		return result;
	
	if (isa_major == 0)
		isa_major = result;

	io_base = ioremap(ISA_BASE, ISA_MAX - ISA_BASE);
	
	return 0;
}

void isa_cleanup(void)
{
	iounmap(io_base);
	unregister_chrdev(isa_major, "isa");
}

module_init(isa_init);
module_exit(isa_cleanup);
