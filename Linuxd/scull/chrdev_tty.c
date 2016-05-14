#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <asm/uaccess.h>

#define DRIVER_VERSION 			"v2.0"
#define DRIVER_AUTHOR 			"Greg Kroah-Hartman <greg@kroah.com>"
#define DRIVER_DESC 			"Tiny TTY driver"
#define SERIAL_DRIVER_DESC 		"Tiny serial driver"
MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_DESCRIPTION( SERIAL_DRIVER_DESC );
MODULE_LICENSE("GPL");

#define DELAY_TIME				HZ * 2		
#define TINY_DATA_CHARACTER		't'
#define TINY_TTY_MAJOR			240			
#define TINY_TTY_MINORS			4		
#define TINY_SERIAL_MINORS		1			
#define UART_NR					1			
#define TINY_SERIAL_NAME		"ttytiny"

#define MCR_DTR					0x01
#define MCR_RTS					0x02
#define MCR_LOOP				0x04
#define MSR_CTS					0x08
#define MSR_CD					0x10
#define MSR_RI					0x20
#define MSR_DSR					0x40
#define RELEVANT_IFLAG(iflag) 	((iflag) & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))

struct tiny_serial {
	struct tty_struct	*tty;			/* pointer to the tty for this device */
	int			open_count;				/* number of times this port has been opened */
	struct semaphore	sem;			/* locks this structure */
	struct timer_list	*timer;
	int			msr;					/* MSR shadow */
	int			mcr;					/* MCR shadow */
	struct serial_struct	serial;
	wait_queue_head_t	wait;
	struct async_icount	icount;
};

static struct tty_driver *tiny_tty_driver;
static struct tiny_serial *tiny_table[TINY_TTY_MINORS];
static struct timer_list *timers;

static void tiny_timers(unsigned long timer_data)
{
	struct tiny_serial *tiny = (struct tiny_serial *)timer_data;
	struct tty_struct *tty;
	int i;
	char data[1] = {TINY_DATA_CHARACTER};
	int data_size = 1;

	if (!tiny)
		return;

	tty = tiny->tty;
	for (i = 0; i < data_size; ++i) {
		if (tty->flip.count >= TTY_FLIPBUF_SIZE)
			tty_flip_buffer_push(tty);
		tty_insert_flip_char(tty, data[i], TTY_NORMAL);
	}
	tty_flip_buffer_push(tty);

	tiny->timer->expires = jiffies + DELAY_TIME;
	add_timer(tiny->timer);
}

static int tiny_open(struct tty_struct *tty, struct file *file)
{
	struct tiny_serial *tiny;
	struct timer_list *timer;
	int index;

	tty->driver_data = NULL;
	index = tty->index;
	tiny = tiny_table[index];
	if (tiny == NULL) {
		tiny = kmalloc(sizeof(*tiny), GFP_KERNEL);
		if (tiny == NULL)
			return -ENOMEM;

		init_MUTEX(&tiny->sem);
		tiny->open_count = 0;
		tiny->timer = NULL;
		tiny_table[index] = tiny;
	}

	down(&tiny->sem);
	tty->driver_data = tiny;
	tiny->tty = tty;
	++tiny->open_count;
	
	if (tiny->open_count == 1) {
		if (tiny->timer == NULL) {
			timer = kmalloc(sizeof(*timer), GFP_KERNEL);
			if (timer == NULL) {
				up(&tiny->sem);
				return -ENOMEM;
			}
			tiny->timer = timer;
		}
		
		tiny->timer->data = (unsigned long )tiny;
		tiny->timer->expires = jiffies + DELAY_TIME;
		tiny->timer->function = tiny_timers;
		add_timer(tiny->timer);
	}
	up(&tiny->sem);
	
	return 0;
}

static void tiny_close(struct tty_struct *tty, struct file *file)
{
	struct tiny_serial *tiny = tty->driver_data;

	if (tiny) {
		down(&tiny->sem);
		if (!tiny->open_count) 
			up(&tiny->sem);

		--tiny->open_count;
		if (tiny->open_count <= 0) 
			del_timer(tiny->timer);
		
		up(&tiny->sem);
	}
}	

static int tiny_write(struct tty_struct *tty, const unsigned char *buffer, int count)
{
	struct tiny_serial *tiny = tty->driver_data;
	int i,retval = -EINVAL;

	if (!tiny)
		return -ENODEV;

	down(&tiny->sem);
	if (!tiny->open_count) {
		up(&tiny->sem);
		return retval;
	}

	for (i = 0; i < count; ++i)
		printk("%02x ", buffer[i]);
	printk("\n");
		
	up(&tiny->sem);
	
	return retval;
}

static int tiny_write_room(struct tty_struct *tty) 
{
	struct tiny_serial *tiny = tty->driver_data;
	int room = -EINVAL;

	if (tiny == NULL)
		return -ENODEV;

	down(&tiny->sem);
	if (!tiny->open_count){
		up(&tiny->sem);
		return room;
	}
	
	room = 255;
	up(&tiny->sem);
	
	return room;
}

static void tiny_set_termios(struct tty_struct *tty, struct termios *old_termios)
{
	unsigned int cflag;

	cflag = tty->termios->c_cflag;
	if (old_termios) {
		if ((cflag == old_termios->c_cflag) && (RELEVANT_IFLAG(tty->termios->c_iflag) ==  RELEVANT_IFLAG(old_termios->c_iflag))) {
			printk(KERN_DEBUG " - nothing to change...\n");
			return;
		}
	}

	switch (cflag & CSIZE) {
		case CS5:
			printk(KERN_DEBUG " - data bits = 5\n");
			break;
			
		case CS6:
			printk(KERN_DEBUG " - data bits = 6\n");
			break;
			
		case CS7:
			printk(KERN_DEBUG " - data bits = 7\n");
			break;
			
		default:
		case CS8:
			printk(KERN_DEBUG " - data bits = 8\n");
			break;
	}
	
	if (cflag & PARENB) {
		if (cflag & PARODD)
			printk(KERN_DEBUG " - parity = odd\n");
		else
			printk(KERN_DEBUG " - parity = even\n");
	} else {
		printk(KERN_DEBUG " - parity = none\n");
	}

	if (cflag & CSTOPB) {
		printk(KERN_DEBUG " - stop bits = 2\n");
	} else {
		printk(KERN_DEBUG " - stop bits = 1\n");
	}
	
	if (cflag & CRTSCTS) {
		printk(KERN_DEBUG " - RTS/CTS is enabled\n");
	} else {
		printk(KERN_DEBUG " - RTS/CTS is disabled\n");
	}
	
	if (I_IXOFF(tty)) {
		printk(KERN_DEBUG " - INBOUND XON/XOFF is enabled, XON = %2x, XOFF = %2x", START_CHAR(tty), STOP_CHAR(tty));
	} else {
		printk(KERN_DEBUG" - INBOUND XON/XOFF is disabled");
	}
	
	if (I_IXON(tty)) {
		printk(KERN_DEBUG" - OUTBOUND XON/XOFF is enabled, XON = %2x, XOFF = %2x", START_CHAR(tty), STOP_CHAR(tty));
	} else {
		printk(KERN_DEBUG" - OUTBOUND XON/XOFF is disabled");
	}
	
	printk(KERN_DEBUG " - baud rate = %d", tty_get_baud_rate(tty));
}

static int tiny_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	struct tiny_serial *tiny;
	off_t begin = 0;
	int length = 0;
	int i;

	length += sprintf(page, "tinyserinfo:1.0 driver:%s\n", DRIVER_VERSION);
	for (i = 0; i < TINY_TTY_MINORS && length < PAGE_SIZE; ++i) {
		tiny = tiny_table[i];
		if (tiny == NULL)
			continue;

		length += sprintf(page+length, "%d\n", i);
		if ((length + begin) > (off + count))
			goto done;
		
		if ((length + begin) < off) {
			begin += length;
			length = 0;
		}
	}
	
	*eof = 1;
	
done:
	if (off >= (length + begin))
		return 0;
	
	*start = page + (off-begin);
	
	return (count < begin+length-off) ? count : begin + length-off;
}

static int tiny_tiocmget(struct tty_struct *tty, struct file *file)
{
	struct tiny_serial *tiny = tty->driver_data;
	unsigned int result = 0;
	unsigned int msr = tiny->msr;
	unsigned int mcr = tiny->mcr;

	result = ((mcr & MCR_DTR)  ? TIOCM_DTR  : 0) | ((mcr & MCR_RTS)  ? TIOCM_RTS  : 0) |
             ((mcr & MCR_LOOP) ? TIOCM_LOOP : 0) |	
             ((msr & MSR_CTS)  ? TIOCM_CTS  : 0) | ((msr & MSR_CD)   ? TIOCM_CAR  : 0) |  
             ((msr & MSR_RI)   ? TIOCM_RI   : 0) | ((msr & MSR_DSR)  ? TIOCM_DSR  : 0);	

	return result;
}

static int tiny_tiocmset(struct tty_struct *tty, struct file *file, unsigned int set, unsigned int clear)
{
	struct tiny_serial *tiny = tty->driver_data;
	unsigned int mcr = tiny->mcr;

	if (set & TIOCM_RTS)
		mcr |= MCR_RTS;
	
	if (set & TIOCM_DTR)
		mcr |= MCR_RTS;

	if (clear & TIOCM_RTS)
		mcr &= ~MCR_RTS;
	
	if (clear & TIOCM_DTR)
		mcr &= ~MCR_RTS;

	tiny->mcr = mcr;
	
	return 0;
}

static struct tty_operations serial_ops = {
	.open 		= 	tiny_open,
	.close 		= 	tiny_close,
	.write 		= 	tiny_write,
	.write_room = 	tiny_write_room,
	.set_termios= 	tiny_set_termios,
};

static int tiny_ioctl_tiocgserial(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct tiny_serial *tiny = tty->driver_data;

	if (cmd == TIOCGSERIAL) {
		struct serial_struct tmp;

		if (arg == NULL)
			return -EFAULT;

		memset(&tmp, 0, sizeof(tmp));
		tmp.type			= tiny->serial.type;
		tmp.line			= tiny->serial.line;
		tmp.port			= tiny->serial.port;
		tmp.irq				= tiny->serial.irq;
		tmp.flags			= ASYNC_SKIP_TEST | ASYNC_AUTO_IRQ;
		tmp.xmit_fifo_size	= tiny->serial.xmit_fifo_size;
		tmp.baud_base		= tiny->serial.baud_base;
		tmp.close_delay		= 5*HZ;
		tmp.closing_wait	= 30*HZ;
		tmp.custom_divisor	= tiny->serial.custom_divisor;
		tmp.hub6			= tiny->serial.hub6;
		tmp.io_type			= tiny->serial.io_type;

		if (copy_to_user((void __user *)arg, &tmp, sizeof(struct serial_struct)))
			return -EFAULT;
		
		return 0;
	}
	
	return -ENOIOCTLCMD;
}

static int tiny_ioctl_tiocmiwait(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct tiny_serial *tiny = tty->driver_data;

	if (cmd == TIOCMIWAIT) {
		DECLARE_WAITQUEUE(wait, current);
		struct async_icount cnow;
		struct async_icount cprev;

		cprev = tiny->icount;
		while (1) {
			add_wait_queue(&tiny->wait, &wait);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			remove_wait_queue(&tiny->wait, &wait);

			if (signal_pending(current))
				return -ERESTARTSYS;

			cnow = tiny->icount;
			if (cnow.rng == cprev.rng && cnow.dsr == cprev.dsr &&
			    cnow.dcd == cprev.dcd && cnow.cts == cprev.cts)
				return -EIO; 
			
			if (((arg & TIOCM_RNG) && (cnow.rng != cprev.rng)) ||
			    ((arg & TIOCM_DSR) && (cnow.dsr != cprev.dsr)) ||
			    ((arg & TIOCM_CD)  && (cnow.dcd != cprev.dcd)) ||
			    ((arg & TIOCM_CTS) && (cnow.cts != cprev.cts)) ) {
				return 0;
			}
			
			cprev = cnow;
		}

	}
	return -ENOIOCTLCMD;
}

static int tiny_ioctl_tiocgicount(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct tiny_serial *tiny = tty->driver_data;

	if (cmd == TIOCGICOUNT) {
		struct async_icount cnow = tiny->icount;
		struct serial_icounter_struct icount;

		icount.cts	= cnow.cts;
		icount.dsr	= cnow.dsr;
		icount.rng	= cnow.rng;
		icount.dcd	= cnow.dcd;
		icount.rx	= cnow.rx;
		icount.tx	= cnow.tx;
		icount.frame	= cnow.frame;
		icount.overrun	= cnow.overrun;
		icount.parity	= cnow.parity;
		icount.brk	= cnow.brk;
		icount.buf_overrun = cnow.buf_overrun;

		if (copy_to_user((void __user *)arg, &icount, sizeof(icount)))
			return -EFAULT;
		
		return 0;
	}
	return -ENOIOCTLCMD;
}

static int tiny_ioctl(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		case TIOCGSERIAL:
			return tiny_ioctl_tiocgserial(tty, file, cmd, arg);
			
		case TIOCMIWAIT:
			return tiny_ioctl_tiocmiwait(tty, file, cmd, arg);
			
		case TIOCGICOUNT:
			return tiny_ioctl_tiocgicount(tty, file, cmd, arg);
	}

	return -ENOIOCTLCMD;
}

static void tiny_tx_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->info->xmit;
	int count;

	if (port->x_char) {
		pr_debug("wrote %2x", port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}
	
	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		tiny_stop_tx(port, 0);
		return;
	}

	count = port->fifosize >> 1;
	do {
		pr_debug("wrote %2x", xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		tiny_stop_tx(port, 0);
}

static void tiny_timer(unsigned long data)
{
	struct uart_port *port;
	struct tty_struct *tty;

	port = (struct uart_port *)data;
	if (port == NULL)
		return;
	
	if (port->info == NULL)
		return;
	
	tty = port->info->tty;
	if (tty == NULL)
		return;

	tty_insert_flip_char(tty, TINY_DATA_CHARACTER, 0);
	tty_flip_buffer_push(tty);

	timers->expires = jiffies + DELAY_TIME;
	add_timer(timers);

	tiny_tx_chars(port);
}

static int tiny_startup(struct uart_port *port)
{
	if (!timers) {
		timers = kmalloc(sizeof(*timers), GFP_KERNEL);
		if (!timers)
			return -ENOMEM;
	}
	
	timers->data = (unsigned long)port;
	timers->expires = jiffies + DELAY_TIME;
	timers->function = tiny_timer;
	add_timer(timers);
	
	return 0;
}

static void tiny_set_stermios(struct uart_port *port, struct termios *new, struct termios *old)
{
	int baud, quot, cflag;
	
	cflag = new->c_cflag;
	switch (cflag & CSIZE) {
		case CS5:
			printk(KERN_DEBUG " - data bits = 5\n");
			break;
			
		case CS6:
			printk(KERN_DEBUG " - data bits = 6\n");
			break;
			
		case CS7:
			printk(KERN_DEBUG " - data bits = 7\n");
			break;
			
		default: 
			printk(KERN_DEBUG " - data bits = 8\n");
			break;
	}

	if (cflag & PARENB){
		if (cflag & PARODD) {
			pr_debug(" - parity = odd\n");
		} else {
			pr_debug(" - parity = even\n");
		}
	} else {
		pr_debug(" - parity = none\n");
	}

	if (cflag & CSTOPB) {
		pr_debug(" - stop bits = 2\n");
	} else {
		pr_debug(" - stop bits = 1\n");
	}
	
	if (cflag & CRTSCTS) {
		pr_debug(" - RTS/CTS is enabled\n");
	} else {
		pr_debug(" - RTS/CTS is disabled\n");
	}
	
    baud = uart_get_baud_rate(port, new, old, 0, port->uartclk/16);
    quot = uart_get_divisor(port, baud);
	
}

static void tiny_shutdown(struct uart_port *port)
{
	del_timer(timers);
}

static const char *tiny_type(struct uart_port *port)
{
	return "tinytty";
}

static struct uart_ops tiny_ops = {
	.startup		=	tiny_startup,
	.shutdown		=	tiny_shutdown,
	.set_termios	=	tiny_set_stermios,
	.type			=	tiny_type,
};

static struct uart_port tiny_port = {
	.ops			=	&tiny_ops,
};

static struct uart_driver tiny_reg = {
	.owner			= 	THIS_MODULE,
	.driver_name	= 	TINY_SERIAL_NAME,
	.dev_name		= 	TINY_SERIAL_NAME,
	.major			= 	TINY_SERIAL_MAJOR,
	.minor			= 	TINY_SERIAL_MINORS,
	.nr				= 	UART_NR,
};

static int __init tiny_init(void)
{
	int retval;
	int i;

	tiny_tty_driver = alloc_tty_driver(TINY_TTY_MINORS);
	if (tiny_tty_driver == NULL)
		return -ENOMEM;

	tiny_tty_driver->owner    		= THIS_MODULE;
	tiny_tty_driver->driver_name 	= "tiny_tty";
	tiny_tty_driver->name   		= "ttty";
	tiny_tty_driver->devfs_name 	= "tts/ttty%d";
	tiny_tty_driver->major   		= TINY_TTY_MAJOR,
	tiny_tty_driver->type    		= TTY_DRIVER_TYPE_SERIAL,
	tiny_tty_driver->subtype 		= SERIAL_TYPE_NORMAL,
	tiny_tty_driver->flags 			= TTY_DRIVER_REAL_RAW | TTY_DRIVER_NO_DEVFS,
	tiny_tty_driver->init_termios 	= tty_std_termios;
	tiny_tty_driver->init_termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	tty_set_operations(tiny_tty_driver, &serial_ops);
	tiny_tty_driver->read_proc 		= tiny_read_proc;
	tiny_tty_driver->tiocmget  		= tiny_tiocmget;
	tiny_tty_driver->tiocmset  		= tiny_tiocmset;
	tiny_tty_driver->ioctl     		= tiny_ioctl;

	retval = tty_register_driver(tiny_tty_driver);
	if (retval) {
		printk(KERN_ERR "failed to register tiny tty driver");
		put_tty_driver(tiny_tty_driver);
		return retval;
	}

	for (i = 0; i < TINY_TTY_MINORS; ++i)
		tty_register_device(tiny_tty_driver, i, NULL);

	printk(KERN_INFO DRIVER_DESC " " DRIVER_VERSION);
	
	printk(KERN_INFO "Tiny serial driver loaded\n");
	retval = uart_register_driver(&tiny_reg);
	if (retval)
		return retval;

	result = uart_add_one_port(&tiny_reg, &tiny_port);
	if (retval)
		uart_unregister_driver(&tiny_reg);

	return retval;
}

static void __exit tiny_exit(void)
{
	struct tiny_serial *tiny;
	int i;

	for (i = 0; i < TINY_TTY_MINORS; ++i)
		tty_unregister_device(tiny_tty_driver, i);
	
	tty_unregister_driver(tiny_tty_driver);

	for (i = 0; i < TINY_TTY_MINORS; ++i) {
		tiny = tiny_table[i];
		if (tiny) {
			while (tiny->open_count){
				down(&tiny->sem);
				if (!tiny->open_count) 
					up(&tiny->sem);

				--tiny->open_count;
				if (tiny->open_count <= 0) 
					del_timer(tiny->timer);
				
				up(&tiny->sem);
			}

			del_timer(tiny->timer);
			kfree(tiny->timer);
			kfree(tiny);
			tiny_table[i] = NULL;
		}
	}

	uart_unregister_driver(&tiny_reg);
}

module_init(tiny_init);
module_exit(tiny_exit);
