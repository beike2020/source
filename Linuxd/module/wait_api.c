#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h> 
#include <linux/kernel.h> 
#include <linux/fs.h>     
#include <linux/types.h>  
#include <linux/wait.h> 
#include <linux/completion.h>

MODULE_LICENSE("Dual BSD/GPL");

static int complete_major = 0;
static int sleepy_major = 0;
static int flag = 0;

static DECLARE_COMPLETION(comp);
static DECLARE_WAIT_QUEUE_HEAD(wq);

ssize_t complete_read (struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) going to sleep\n", current->pid, current->comm);
	wait_for_completion(&comp);
	printk(KERN_DEBUG "awoken %i (%s)\n", current->pid, current->comm);
	
	return 0; 
}

ssize_t complete_write (struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) awakening the readers...\n", current->pid, current->comm);
	complete(&comp);
	
	return count; 
}

ssize_t sleepy_read (struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) going to sleep\n", current->pid, current->comm);
	wait_event_interruptible(wq, flag != 0);
	flag = 0;
	printk(KERN_DEBUG "awoken %i (%s)\n", current->pid, current->comm);
	
	return 0; 
}

ssize_t sleepy_write (struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) awakening the readers...\n", current->pid, current->comm);
	flag = 1;
	wake_up_interruptible(&wq);
	
	return count; 
}

struct file_operations complete_fops = {
	.owner 		= 	THIS_MODULE,
	.read 		=  	complete_read,
	.write 		= 	complete_write,
};

struct file_operations sleepy_fops = {
	.owner 		= 	THIS_MODULE,
	.read 		=  	sleepy_read,
	.write 		= 	sleepy_write,
};

int wait_init(void)
{
	int result;
	
	result = register_chrdev(complete_major, "complete", &complete_fops);
	if (result < 0)
		return result;

	if (complete_major == 0)
		complete_major = result; 

	result = register_chrdev(sleepy_major, "sleepy", &sleepy_fops);
	if (result < 0)
		return result;

	if (sleepy_major == 0)
		sleepy_major = result; 
		
	return 0;
}

void wait_cleanup(void)
{
	unregister_chrdev(complete_major, "complete");
	unregister_chrdev(sleepy_major, "sleepy");
}

module_init(wait_init);
module_exit(wait_cleanup);

