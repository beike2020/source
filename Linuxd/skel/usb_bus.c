#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/smp_lock.h>
#include <linux/usb.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

#define USB_SKEL_VENDOR_ID	0xfff0
#define USB_SKEL_PRODUCT_ID	0xfff0
#define USB_SKEL_MINOR_BASE	192

static struct usb_driver skel_driver;

static struct usb_device_id skel_table [] = {
	{ USB_DEVICE(USB_SKEL_VENDOR_ID, USB_SKEL_PRODUCT_ID) },
	{ }					
};
MODULE_DEVICE_TABLE (usb, skel_table);

struct usb_skel {
	struct usb_device 	 *udev;								/* the usb device for this device */
	struct usb_interface *interface;						/* the interface for this device */
	unsigned char *	bulk_in_buffer;							/* the buffer to receive data */
	size_t			bulk_in_size;							/* the size of the receive buffer */
	__u8			bulk_in_endpointAddr;					/* the address of the bulk in endpoint */
	__u8			bulk_out_endpointAddr;					/* the address of the bulk out endpoint */
	struct kref		kref;
};

static void skel_delete(struct kref *krefs)
{	
	struct usb_skel *dev;

	dev = container_of(krefs, struct usb_skel, kref);
	usb_put_dev(dev->udev);
	kfree(dev->bulk_in_buffer);
	kfree(dev);
}

static int skel_open(struct inode *inode, struct file *file)
{
	struct usb_skel *dev;
	struct usb_interface *interfaces;
	int subminor;
	int retval = 0;

	subminor = iminor(inode);
	interfaces = usb_find_interface(&skel_driver, subminor);
	if (interfaces == NULL) {
		printk("Can't find device for minor %d", subminor);
		retval = -ENODEV;
		return retval;
	}

	dev = usb_get_intfdata(interfaces);
	if (dev == NULL) {
		retval = -ENODEV;
		return retval;
	}
	
	kref_get(&dev->kref);
	file->private_data = dev;

	return retval;
}

static int skel_release(struct inode *inode, struct file *file)
{
	struct usb_skel *dev;

	dev = (struct usb_skel *)file->private_data;
	if (dev == NULL)
		return -ENODEV;

	kref_put(&dev->kref, skel_delete);
	
	return 0;
}

static ssize_t skel_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
	struct usb_skel *dev;
	int retval = 0;

	dev = (struct usb_skel *)file->private_data;
	
	retval = usb_bulk_msg(dev->udev, usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr),
			      dev->bulk_in_buffer, min(dev->bulk_in_size, count), &count, HZ*10);
	if (retval == 0) {
		if (copy_to_user(buffer, dev->bulk_in_buffer, count)) {
			retval = -EFAULT;
		} else {
			retval = count;
		}
	}

	return retval;
}

static void skel_write_callback(struct urb *urb, struct pt_regs *regs)
{
	if (urb->status && !(urb->status == -ENOENT || urb->status == -ECONNRESET || urb->status == -ESHUTDOWN))
		printk("%s - nonzero write bulk status received: %d", __FUNCTION__, urb->status);

	usb_buffer_free(urb->dev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);
}

static ssize_t skel_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos)
{
	int result = 0;
	char *buf = NULL;
	struct usb_skel *dev;
	struct urb *urb = NULL;

	dev = (struct usb_skel *)file->private_data;

	if (count == 0)
		return count;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (urb == NULL) {
		result = -ENOMEM;
		goto error;
	}

	buf = usb_buffer_alloc(dev->udev, count, GFP_KERNEL, &urb->transfer_dma);
	if (buf == NULL) {
		result = -ENOMEM;
		goto error;
	}
	
	if (copy_from_user(buf, user_buffer, count)) {
		result = -EFAULT;
		goto error;
	}

	usb_fill_bulk_urb(urb, dev->udev, usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
			  buf, count, skel_write_callback, dev);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	result = usb_submit_urb(urb, GFP_KERNEL);
	if (result != 0) {
		printk("%s - failed submitting write urb, error %d", __FUNCTION__, result);
		goto error;
	}

	usb_free_urb(urb);
	return count;

error:
	usb_buffer_free(dev->udev, count, buf, urb->transfer_dma);
	usb_free_urb(urb);
	kfree(buf);
	return retval;
}

static struct file_operations skel_fops = {
	.owner 		=	THIS_MODULE,
	.read 	 	=	skel_read,
	.write 		=	skel_write,
	.open  		=	skel_open,
	.release 	=	skel_release,
};

static struct usb_class_driver skel_class = {
	.name 		= 	"usb/skel%d",
	.fops 		= 	&skel_fops,
	.mode 		= 	S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,
	.minor_base = 	USB_SKEL_MINOR_BASE,
};

static int usb_probe(struct usb_interface *interfaces, const struct usb_device_id *id)
{
	int i;
	int retval = -ENOMEM;
	struct usb_skel *dev = NULL;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;

	dev = kmalloc(sizeof(struct usb_skel), GFP_KERNEL);
	if (dev == NULL) {
		printk("Out of memory");
		goto error;
	}
	memset(dev, 0x00, sizeof (*dev));
	
	kref_init(&dev->kref);
	dev->udev = usb_get_dev(interface_to_usbdev(interfaces));
	dev->interface = interfaces;

	iface_desc = interfaces->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (!dev->bulk_in_endpointAddr &&(endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)) {
			dev->bulk_in_size = endpoint->wMaxPacketSize;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(endpoint->wMaxPacketSize, GFP_KERNEL);
			if (dev->bulk_in_buffer == NULL) {
				printk("Could not allocate bulk_in_buffer");
				goto error;
			}
		}

		if (!dev->bulk_out_endpointAddr && !(endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)) {
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
		}
	}
	
	if (!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr)) {
		printk("Could not find both bulk-in and bulk-out endpoints");
		goto error;
	}

	usb_set_intfdata(interfaces, dev);

	retval = usb_register_dev(interfaces, &skel_class);
	if (retval) {
		printk("Not able to get a minor for this device.");
		usb_set_intfdata(interfaces, NULL);
		goto error;
	}

	printk("USB Skeleton device now attached to USBSkel-%d", interfaces->minor);
	
	return 0;

error:
	if (dev)
		kref_put(&dev->kref, skel_delete);
	
	return retval;
}

static void usb_disconnect(struct usb_interface *interfaces)
{
	struct usb_skel *dev;
	int minor = interfaces->minor;

	lock_kernel();
	dev = usb_get_intfdata(interfaces);
	usb_set_intfdata(interfaces, NULL);
	usb_deregister_dev(interfaces, &skel_class);
	unlock_kernel();

	kref_put(&dev->kref, skel_delete);
	printk("USB Skeleton #%d now disconnected", minor);
}

static struct usb_driver skel_driver = {
	.owner 			= 	THIS_MODULE,
	.name 			= 	"skeleton",
	.id_table 		= 	skel_table,
	.probe 			= 	usb_probe,
	.disconnect 	= 	usb_disconnect,
};

static int __init usb_skel_init(void)
{
	return usb_register(&skel_driver);
}

static void __exit usb_skel_exit(void)
{
	usb_deregister(&skel_driver);
}

module_init (usb_skel_init);
module_exit (usb_skel_exit);

