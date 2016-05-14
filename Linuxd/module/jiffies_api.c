#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>     
#include <linux/proc_fs.h>
#include <linux/errno.h>  
#include <linux/workqueue.h>
#include <linux/preempt.h>
#include <linux/interrupt.h> 
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <asm/hardirq.h>

#define JIT_ASYNC_LOOPS 5
#define LIMIT			(PAGE_SIZE-128)	
#define SCHEDULER_QUEUE ((task_queue *) 1)

enum jit_files {
	JIT_BUSY,
	JIT_SCHED,
	JIT_QUEUE,
	JIT_SCHEDTO
};

struct jit_data {
	struct timer_list timer;
	struct tasklet_struct tlet;
	int hi; 
	wait_queue_head_t wait;
	unsigned long prevjiffies;
	unsigned char *buf;
	int loops;
};

int delays = HZ; 
int tdelay = 10;
static long delay = 1;
module_param(tdelay, int, 0);
module_param(delays, int, 0);
module_param(delay, long, 0);

MODULE_AUTHOR("Alessandro Rubini");
MODULE_LICENSE("Dual BSD/GPL");

static struct work_struct jiq_work;
static struct timer_list jiq_timer;
static struct clientdata {
	int len;
	char *buf;
	unsigned long jiffies;
	long delay;
} jiq_data;

/*****************************jit***********************************/
int jit_now(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	struct timeval tv1;
	struct timespec tv2;
	unsigned long j1;
	u64 j2;

	j1 = jiffies;
	j2 = get_jiffies_64();
	do_gettimeofday(&tv1);
	tv2 = current_kernel_time();

	len = 0;
	len += sprintf(buf,"0x%08lx 0x%016Lx %10d.%06d\n%40d.%09d\n",j1, j2,(int) tv1.tv_sec, (int) tv1.tv_usec, (int) tv2.tv_sec, (int) tv2.tv_nsec);
	*start = buf;
	
	return len;
}

int jit_sleeps(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	unsigned long j0, j1; 
	wait_queue_head_t wait;

	init_waitqueue_head (&wait);
	j0 = jiffies;
	j1 = j0 + delays;

	switch((long)data) {
		case JIT_BUSY:
			while (time_before(jiffies, j1))
				cpu_relax();
			break;
			
		case JIT_SCHED:
			while (time_before(jiffies, j1)) 
				schedule();
			break;
			
		case JIT_QUEUE:
			wait_event_interruptible_timeout(wait, 0, delays);
			break;
			
		case JIT_SCHEDTO:
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout (delays);
			break;
	}
	
	j1 = jiffies; 
	len = sprintf(buf, "%9lu %9lu\n", j0, j1);
	*start = buf;
	
	return len;
}

void timer_fn(unsigned long arg)
{
	struct jit_data *data = (struct jit_data *)arg;
	unsigned long j = jiffies;
	
	data->buf += sprintf(data->buf, "%9lu  %3lu     %d    %6d   %d   %s\n", j, j - data->prevjiffies, in_interrupt() ? 1 : 0, current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->timer.expires += tdelay;
		data->prevjiffies = j;
		add_timer(&data->timer);
	} else {
		wake_up_interruptible(&data->wait);
	}
}

int jit_timer(char *buf, char **start, off_t offset, int len, int *eof, void *unused_data)
{
	struct jit_data *data;
	char *buf2 = buf;
	unsigned long j = jiffies;

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	init_timer(&data->timer);
	init_waitqueue_head(&data->wait);

	buf2 += sprintf(buf2, "   time   delta  inirq    pid   cpu command\n");
	buf2 += sprintf(buf2, "%9lu  %3lu     %d    %6d   %d   %s\n",j, 0L, in_interrupt() ? 1 : 0, current->pid, smp_processor_id(), current->comm);

	data->prevjiffies = j;
	data->buf = buf2;
	data->loops = JIT_ASYNC_LOOPS;
	data->timer.data = (unsigned long)data;
	data->timer.function = timer_fn;
	data->timer.expires = j + tdelay; 
	add_timer(&data->timer);

	wait_event_interruptible(data->wait, !data->loops);
	if (signal_pending(current))
		return -ERESTARTSYS;
	
	buf2 = data->buf;
	kfree(data);
	*eof = 1;
	
	return buf2 - buf;
}

void tasklet_fn(unsigned long arg)
{
	struct jit_data *data = (struct jit_data *)arg;
	unsigned long j = jiffies;
	
	data->buf += sprintf(data->buf, "%9lu  %3lu     %d    %6d   %d   %s\n", j, j - data->prevjiffies, in_interrupt() ? 1 : 0, current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->prevjiffies = j;
		if (data->hi)
			tasklet_hi_schedule(&data->tlet);
		else
			tasklet_schedule(&data->tlet);
	} else {
		wake_up_interruptible(&data->wait);
	}
}

int jit_tasklet(char *buf, char **start, off_t offset, int len, int *eof, void *arg)
{
	struct jit_data *data;
	char *buf2 = buf;
	unsigned long j = jiffies;
	long hi = (long)arg;

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	init_waitqueue_head (&data->wait);

	buf2 += sprintf(buf2, "   time   delta  inirq    pid   cpu command\n");
	buf2 += sprintf(buf2, "%9li  %3li     %i    %6i   %i   %s\n", j, 0L, in_interrupt() ? 1 : 0, current->pid, smp_processor_id(), current->comm);

	data->prevjiffies = j;
	data->buf = buf2;
	data->loops = JIT_ASYNC_LOOPS;
	
	tasklet_init(&data->tlet, tasklet_fn, (unsigned long)data);
	data->hi = hi;
	if (hi)
		tasklet_hi_schedule(&data->tlet);
	else
		tasklet_schedule(&data->tlet);

	wait_event_interruptible(data->wait, !data->loops);

	if (signal_pending(current))
		return -ERESTARTSYS;
	
	buf2 = data->buf;
	kfree(data);
	*eof = 1;
	
	return buf2 - buf;
}

/*****************************jit***********************************/

/*****************************jiq***********************************/

static DECLARE_WAIT_QUEUE_HEAD (jiq_wait);

static int jiq_show(void *ptr)
{
	struct clientdata *data = ptr;
	int len = data->len;
	char *buf = data->buf;
	unsigned long j = jiffies;

	if (len > LIMIT) { 
		wake_up_interruptible(&jiq_wait);
		return 0;
	}

	if (len == 0) {
		len = sprintf(buf, "    time  delta preempt   pid cpu command\n");
		len += sprintf(buf+len, "%9lu  %4lu     %3d %5d %3d %s\n", j, j - data->jiffies, preempt_count(), current->pid, smp_processor_id(),current->comm);
	} else {
		len = 0;
		len += sprintf(buf+len, "%9lu  %4lu     %3d %5d %3d %s\n", j, j - data->jiffies, preempt_count(), current->pid, smp_processor_id(),current->comm);
	}
	
	data->len += len;
	data->buf += len;
	data->jiffies = j;
	
	return 1;
}

static void jiq_show_tasklet(unsigned long ptr)
{
	if (jiq_show ((void *) ptr))
		tasklet_schedule(&jiq_tasklet);
}

static DECLARE_TASKLET(jiq_tasklet, jiq_show_tasklet, (unsigned long)&jiq_data);

static void jiq_workqueue(void *ptr)
{
	struct clientdata *data = (struct clientdata *) ptr;
    
	if (!jiq_show(ptr))
		return;
    
	if (data->delay)
		schedule_delayed_work(&jiq_work, data->delay);
	else
		schedule_work(&jiq_work);
}

static int jiq_read_workqueue(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	DEFINE_WAIT(wait);
	
	jiq_data.len = 0;                
	jiq_data.buf = buf;             
	jiq_data.jiffies = jiffies;      
	jiq_data.delay = 0;
    
	prepare_to_wait(&jiq_wait, &wait, TASK_INTERRUPTIBLE);
	schedule_work(&jiq_work);
	schedule();
	finish_wait(&jiq_wait, &wait);

	*eof = 1;
	return jiq_data.len;
}

static int jiq_read_workqueue_delayed(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	DEFINE_WAIT(wait);
	
	jiq_data.len = 0;                
	jiq_data.buf = buf;              
	jiq_data.jiffies = jiffies; 
	jiq_data.delay = delay;
    
	prepare_to_wait(&jiq_wait, &wait, TASK_INTERRUPTIBLE);
	schedule_delayed_work(&jiq_work, delay);
	schedule();
	finish_wait(&jiq_wait, &wait);
	*eof = 1;
	
	return jiq_data.len;
}

static void jiq_timedout(unsigned long ptr)
{
	jiq_show((void *)ptr);            
	wake_up_interruptible(&jiq_wait);  
}

static int jiq_timer_workqueue(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	jiq_data.len = 0;           
	jiq_data.buf = buf;
	jiq_data.jiffies = jiffies;

	init_timer(&jiq_timer);       
	jiq_timer.function = jiq_timedout;
	jiq_timer.data = (unsigned long)&jiq_data;
	jiq_timer.expires = jiffies + HZ; 

	jiq_show(&jiq_data);   
	add_timer(&jiq_timer);
	interruptible_sleep_on(&jiq_wait);  
	del_timer_sync(&jiq_timer); 
	*eof = 1;
	
	return jiq_data.len;
}

static int jiq_read_tasklet(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	jiq_data.len = 0;                
	jiq_data.buf = buf;             
	jiq_data.jiffies = jiffies;      

	tasklet_schedule(&jiq_tasklet);
	interruptible_sleep_on(&jiq_wait);    
	*eof = 1;
	
	return jiq_data.len;
}

/*****************************jit***********************************/

static int __init jiffies_init(void)
{
	INIT_WORK(&jiq_work, jiq_workqueue, &jiq_data);

	create_proc_read_entry("currentime", 0, NULL, jit_now,     NULL);
	create_proc_read_entry("jitbusy", 	 0, NULL, jit_sleeps,  (void *)JIT_BUSY);
	create_proc_read_entry("jitsched",	 0, NULL, jit_sleeps,  (void *)JIT_SCHED);
	create_proc_read_entry("jitqueue",	 0, NULL, jit_sleeps,  (void *)JIT_QUEUE);
	create_proc_read_entry("jitschedto", 0, NULL, jit_sleeps,  (void *)JIT_SCHEDTO);
	create_proc_read_entry("jitimer", 	 0, NULL, jit_timer, 	NULL);
	create_proc_read_entry("jitasklet",  0, NULL, jit_tasklet, 	NULL);
	create_proc_read_entry("jitasklethi",0, NULL, jit_tasklet, 	(void *)1);

	create_proc_read_entry("jiqwq", 0, NULL, jiq_read_workqueue, NULL);
	create_proc_read_entry("jiqwqdelay", 0, NULL, jiq_read_workqueue_delayed, NULL);
	create_proc_read_entry("jitimer", 0, NULL, jiq_timer_workqueue, NULL);
	create_proc_read_entry("jiqtasklet", 0, NULL, jiq_read_tasklet, NULL);

	return 0; 
}

static void jiffies_cleanup(void)
{
	remove_proc_entry("currentime", NULL);
	remove_proc_entry("jitbusy",    NULL);
	remove_proc_entry("jitsched",   NULL);
	remove_proc_entry("jitqueue",   NULL);
	remove_proc_entry("jitschedto", NULL);
	remove_proc_entry("jitimer",    NULL);
	remove_proc_entry("jitasklet",  NULL);
	remove_proc_entry("jitasklethi",NULL);

	remove_proc_entry("jiqwq", NULL);
	remove_proc_entry("jiqwqdelay", NULL);
	remove_proc_entry("jitimer", NULL);
	remove_proc_entry("jiqtasklet", NULL);
}

module_init(jiffies_init);
module_exit(jiffies_cleanup);
