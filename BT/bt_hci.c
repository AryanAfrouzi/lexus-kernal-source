/*
 * USB BT module driver - 1.1
 *
 */

/* Change history */
/*	2012.11.09	Ver0.2 -> 0.3	Change : DEBUG_BTC define -> undef */
/*								Add : DEBUG_BTC_ERR for Error log */
/*	2013.04.09	Ver0.3 -> 0.4	Change : interruptの受信コールバックでエラーでも、*/
/*										 クラスドライバ内部のエラー変数に情報を格納しない。*/
/*	2013.04.19	Ver0.4 -> 0.5	Change : read時のEPIPEエラーをEAGAINエラーにマッピング */
/*										                 write時のEPIPEエラーをENOMEMエラーにマッピング */
/*	2013.05.02	Ver0.5 -> 0.6	Change : bt_hci_read_int  異常ルートのMutex Unlock漏れ改修 */
/*										                 						*/
/*	2013.06.18	Ver0.6 -> 0.7	Change : Watchdogリセット問題の対策に関連して改修(DN) */
/*										 デバイス情報更新時の排他制御修正			*/
/*	2013.06.19	Ver0.7 -> 0.8	Change : 排他制御について再改修，リターンコードの見直し(DN) */
/*										 カーネルログ出力の一部修正			*/
/*	2013.07.11	Ver0.8 -> 0.9	Change :  BCCMD応答ロスト不具合への対応，エラー状態クリア箇所の見直し */
/*										 */
/*	2013.10.22	Ver0.9 -> 1.0	Change :  spin_lock関連使用APIをspin_lock_irqsave/spin_unlock_irqrestoreに統一 */
/*										 probe処理時のログ出力強化 */
/*	2013.12.02	Ver1.0 -> 1.1	Change :  bulk/interrupt受信、bulk送信処理をブロッキング動作を前提に改修 */
/*										 */

#ifdef DEBUG_BTC
#undef DEBUG_BTC
#endif

#define DEBUG_BTC_ERR

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>

#if 1 /* retry */
#include <linux/delay.h>
#endif

/* for HCI mode	*/
#define	USB_REQ_SEND_ENCAPSULATED_COMMAND	0x00
#define BT_HCI_HCI_COMMAND_TIMEOUT  (10000)
#define O_HCI_ACLDATA	(04000000)	/* ACLデータモード */


static __u8 min_int_in_interval = 1;

/* Define these values to match your devices */
#define USB_BT_HCI_VENDOR_ID	0x0a12
#define USB_BT_HCI_PRODUCT_ID	0x0001

/* table of devices that work with this driver */
static struct usb_device_id bt_hci_table[] = {
	{ USB_DEVICE(USB_BT_HCI_VENDOR_ID, USB_BT_HCI_PRODUCT_ID) },
	{ }					/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, bt_hci_table);


/* Get a minor range for your devices from the usb maintainer */
#define USB_BT_HCI_MINOR_BASE	200

/* our private defines. if this grows any larger, use your own .h file */
#define MAX_TRANSFER		(1024)	// more than 255 for HCI
#define WRITES_IN_FLIGHT	1
/* arbitrarily chosen */

/* Structure to hold all of our device specific stuff */
struct usb_bt_hci {
	struct usb_device	*udev;			/* the usb device for this device */
	struct usb_interface	*interface;		/* the interface for this device */
	struct semaphore	limit_sem;		/* limiting the number of writes in progress */
	struct usb_anchor	submitted;		/* in case we need to retract our submissions */
							/* BULK Endpoint */
	struct urb		*bulk_in_urb;		/* the urb to read data with */
	unsigned char           *bulk_in_buffer;	/* the buffer to receive data */
	size_t			bulk_in_size;		/* the size of the receive buffer */
	size_t			bulk_in_filled;		/* number of bytes in the buffer */
	size_t			bulk_in_copied;		/* already copied to user space */
	__u8 			bulk_in_endpointAddr;	/* the address of the bulk in endpoint */
	__u8			bulk_out_endpointAddr;	/* the address of the bulk out endpoint */
							/* INTERRUPT Endpoint */
	struct urb		*int_in_urb;		/* the urb to read data with */
	unsigned char           *int_in_buffer;		/* the buffer to receive data */
	size_t			int_in_size;		/* the size of the receive buffer */
	size_t			int_in_filled;		/* number of bytes in the buffer */
	size_t			int_in_copied;		/* already copied to user space */
	__u8 			int_in_endpointAddr;	/* the address of the int in endpoint */
        __u8                    int_in_interval;
	__u8			int_out_endpointAddr;	/* the address of the bulk out endpoint */
	int			int_read_errors;			/* the last request tanked int read */
	int			bulk_read_errors;			/* the last request tanked bulk read */
	int			bulk_write_errors;			/* the last request tanked bulk write */
	int			open_count;		/* count the number of openers */
	bool			ongoing_read_bulk;	/* a read is going on (BULK IN)*/
	bool			ongoing_read_int;	/* a read is going on (INT IN)*/
	spinlock_t		err_lock;			/* lock for errors */
/* 2013.06.17 排他修正 start	*/
	spinlock_t		devinfo_lock;		/* lock for device-info  */
/* 2013.06.17 排他修正 end		*/
	struct kref		kref;
	struct mutex		io_mutex;		/* synchronize I/O with disconnect */
	struct mutex		io_mutex_acl_w;		/* synchronize I/O with disconnect(write) */
	struct mutex		io_mutex_acl_r;		/* synchronize I/O with disconnect(read) */
	struct completion	bulk_in_completion;	/* to wait for an ongoing read */
	struct completion	int_in_completion;	/* to wait for an ongoing read */
	struct completion	bulk_out_completion;	/* to wait for an ongoing write (ADD) */
};
#define to_bt_hci_dev(d) container_of(d, struct usb_bt_hci, kref)

static struct usb_driver bt_hci_driver;
static void bt_hci_draw_down(struct usb_bt_hci *dev);
/* static void bt_hci_draw_down_bulk(struct usb_bt_hci *dev); */
/* static void bt_hci_draw_down_int(struct usb_bt_hci *dev); */
static ssize_t bt_hci_read_bulk(struct file *file, char *buffer, size_t count, loff_t *ppos);
static ssize_t bt_hci_read_int(struct file *file, char *buffer, size_t count, loff_t *ppos);
static void bt_hci_read_int_callback(struct urb *urb);
static ssize_t bt_hci_bulk_write(struct file *file, const char *user_buffer, size_t count, loff_t *ppos);
static ssize_t bt_hci_int_write(struct file *file, const char *user_buffer, size_t count, loff_t *ppos);

static void bt_hci_delete(struct kref *kref)
{
	struct usb_bt_hci *dev = to_bt_hci_dev(kref);

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_delete called\n");
#endif

 	if (dev->bulk_in_urb) {
#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_delete free bulk_in_urb\n");
#endif
		usb_free_urb(dev->bulk_in_urb);
	}
 	if (dev->int_in_urb) {
#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_delete free int_in_urb\n");
#endif
		usb_free_urb(dev->int_in_urb);
	}
	usb_put_dev(dev->udev);
        if (dev->bulk_in_buffer) {
		kfree(dev->bulk_in_buffer);
	}
        if (dev->int_in_buffer) {
		kfree(dev->int_in_buffer);
	}
	kfree(dev);
}

static int bt_hci_open(struct inode *inode, struct file *file)
{
	struct usb_bt_hci *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;
	struct mutex *p_io_mutex;

	subminor = iminor(inode);

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_open called minor:%d %x \n", subminor, file->f_flags);
#endif

	interface = usb_find_interface(&bt_hci_driver, subminor);
	if (!interface) {
#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_open error interface:%08x \n", (int)interface);
#endif
		err("%s - error, can't find device for minor %d",
		     __func__, subminor);
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);
	if (!dev) {
#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_open error dev:%08x \n", (int)dev);
#endif
		retval = -ENODEV;
		goto exit;
	}

	if (file->f_flags & O_HCI_ACLDATA) {
		// ACL
		p_io_mutex = &dev->io_mutex_acl_w;
	}
	else {
		// HCI Command
		p_io_mutex = &dev->io_mutex;
	}

	/* increment our usage count for the device */
	kref_get(&dev->kref);

	/* lock the device to allow correctly handling errors
	 * in resumption */
	mutex_lock(p_io_mutex);

	if (!dev->open_count++) {
#if 1 /* retry */
		{
			int i = 0;
//			struct timespec req;

			for( i = 0; i < 10; i++ ) {
				retval = usb_autopm_get_interface(interface);
				if (retval) {
#ifdef DEBUG_BTC
					printk(KERN_INFO"bt_hci_open usb_autopm_get_interface error(rty):%d\n", retval);
#endif

					msleep(500);

//					req.tv_sec  = 0;
//					req.tv_nsec = 300000000; /* 300ms */
//					nanosleep(&req, NULL);
				}
				else {
#ifdef DEBUG_BTC
					printk(KERN_INFO"bt_hci_open usb_autopm_get_interface success\n");
#endif
					break;
				}
			}
		}
#endif

		if (retval) {
#ifdef DEBUG_BTC
			printk(KERN_INFO"bt_hci_open usb_autopm_get_interface error:%d\n", retval);
#endif
			dev->open_count--;
			mutex_unlock(p_io_mutex);
			kref_put(&dev->kref, bt_hci_delete);
			goto exit;
		}
	}
	/* else { //uncomment this block if you want exclusive open
		retval = -EBUSY;
		dev->open_count--;
		mutex_unlock(p_io_mutex);
		kref_put(&dev->kref, bt_hci_delete);
		goto exit;
	} */
	if (dev->bulk_in_urb) {
		init_completion(&dev->bulk_in_completion);
		init_completion(&dev->bulk_out_completion);
		
		dev->bulk_read_errors = 0;
		dev->bulk_write_errors = 0;
	}
	/* submit read for interrpt in */
	if (dev->int_in_urb ) {
		if (!(file->f_flags & O_HCI_ACLDATA)) {

			dev->int_in_filled = 0;
			dev->int_read_errors = 0;
			usb_fill_int_urb(dev->int_in_urb, dev->udev,
				usb_rcvintpipe(dev->udev,
					dev->int_in_endpointAddr),
				dev->int_in_buffer,
				dev->int_in_size,
				bt_hci_read_int_callback,
				dev,
				dev->int_in_interval);
#ifdef DEBUG_BTC
	        printk(KERN_INFO"bt_hci_open submit int_urb \n");
#endif
#if 0
	int rv;
			/* do it */
			rv = usb_submit_urb(dev->int_in_urb, GFP_KERNEL);
#ifdef DEBUG_BTC
	    printk(KERN_INFO"bt_hci_open submit int_urb rv = %d\n",rv);
#endif
			if (rv < 0) {
				err("%s - failed submitting read urb, error %d",
				__func__, rv);
				dev->int_in_filled = 0;
				retval = (rv == -ENOMEM) ? rv : -EIO;
			}
#endif
		}

		init_completion(&dev->int_in_completion);
	}
	

	/* save our object in the file's private structure */
	file->private_data = dev;
	mutex_unlock(p_io_mutex);

exit:
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_open returned retval:%d \n", retval);
#endif
	return retval;
}

static int bt_hci_release(struct inode *inode, struct file *file)
{
	struct usb_bt_hci *dev;
	struct mutex *p_io_mutex;
	/* 2013.06.17 排他修正 start */
	struct usb_interface *interface;
	unsigned long flags;
	/* 2013.06.17 排他修正 end   */

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_release called\n");
#endif

	dev = (struct usb_bt_hci *)file->private_data;
	if (dev == NULL)
		return -ENODEV;

	if (file->f_flags & O_HCI_ACLDATA) {
		// ACL
		p_io_mutex = &dev->io_mutex_acl_w;

#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_release: ACL \n");
#endif
	}
	else {
		// HCI Command
		p_io_mutex = &dev->io_mutex;

#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_release: HCI \n");
#endif
	}

	/* allow the device to be autosuspended */

	/* 2013.06.19 排他修正2 start */
	mutex_lock(p_io_mutex);

	spin_lock_irqsave(&dev->devinfo_lock, flags);
	interface = dev->interface;
	spin_unlock_irqrestore(&dev->devinfo_lock, flags);

	if (!--dev->open_count && interface) {
		usb_autopm_put_interface(interface);
	}
	mutex_unlock(p_io_mutex);
	/* 2013.06.19 排他修正2 end   */

	/* decrement the count on our device */
	kref_put(&dev->kref, bt_hci_delete);
	return 0;
}

// static int bt_hci_flush(struct file *file, fl_owner_t id)
// {
	// struct usb_bt_hci *dev;
	// int res;
	// struct mutex *p_io_mutex;

// #ifdef DEBUG_BTC
	// printk(KERN_INFO"bt_hci_flush called\n");
// #endif

	// dev = (struct usb_bt_hci *)file->private_data;
	// if (dev == NULL)
		// return -ENODEV;

	// if (file->f_flags & O_HCI_ACLDATA) {
		// // ACL
		// p_io_mutex = &dev->io_mutex_acl_w;

// #ifdef DEBUG_BTC
		// printk(KERN_INFO"bt_hci_flush: ACL \n");
// #endif

		// /* wait for io to stop */
		// mutex_lock(p_io_mutex);
		// bt_hci_draw_down_bulk(dev);
	// }
	// else {
		// // HCI Command
		// p_io_mutex = &dev->io_mutex;

// #ifdef DEBUG_BTC
		// printk(KERN_INFO"bt_hci_flush: HCI \n");
// #endif

		// /* wait for io to stop */
		// mutex_lock(p_io_mutex);
		// bt_hci_draw_down_int(dev);
	// }

	// /* read out errors, leave subsequent opens a clean slate */
	// spin_lock_irq(&dev->err_lock);
	// res = dev->errors ? (dev->errors == -EPIPE ? -EPIPE : -EIO) : 0;
	// dev->errors = 0;
	// spin_unlock_irq(&dev->err_lock);

	// mutex_unlock(p_io_mutex);

	// return res;
// }

static void bt_hci_read_bulk_callback(struct urb *urb)
{
	struct usb_bt_hci *dev;
	unsigned long flags;

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_bulk_callback called\n");
	printk(KERN_INFO"bt_hci_read_bulk_callback actual read = %d\n",urb->actual_length );
#endif

	dev = urb->context;

	spin_lock_irqsave(&dev->err_lock, flags);
	/* sync/async unlink faults aren't errors */
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			err("%s - nonzero read bulk status received: %d",
			    __func__, urb->status);

		dev->bulk_read_errors = urb->status;
		err("%s - errors received: %d", __func__, dev->bulk_read_errors);
	
	} else {
		dev->bulk_in_filled = urb->actual_length;
		dev->bulk_read_errors = 0;
	}
	dev->ongoing_read_bulk = 0;
	spin_unlock_irqrestore(&dev->err_lock, flags);

	complete(&dev->bulk_in_completion);
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_bulk_callback fin status:%d, actual_length:%d\n",urb->status,urb->actual_length);
#endif
}

static void bt_hci_read_int_callback(struct urb *urb)
{
	struct usb_bt_hci *dev;
	unsigned long flags;
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_int_callback called\n");
#endif

	dev = urb->context;

	spin_lock_irqsave(&dev->err_lock, flags);
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			err("%s - nonzero read int status received: %d",
			    __func__, urb->status);

		dev->int_read_errors = urb->status;
		err("%s - errors received: %d", __func__, dev->int_read_errors);

#ifdef DEBUG_BTC_ERR
		printk(KERN_INFO"bt_hci_read_int_callback error %d\n", dev->int_read_errors);
#endif
	} else {
		dev->int_in_filled = urb->actual_length;
		dev->int_read_errors = 0;
#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_read_int_callback actual_length = %d\n",urb->actual_length);
#endif
	}
	dev->ongoing_read_int = 0;
	spin_unlock_irqrestore(&dev->err_lock, flags);
	complete(&dev->int_in_completion);
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_int_callback fin\n");
#endif
}

static int bt_hci_do_read_io_bulk(struct usb_bt_hci *dev, size_t count)
{
	// BT Module Complete proc

	int rv;
	unsigned long flags;

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_do_read_io_bulk called\n");
#endif

	dev->bulk_in_copied = 0;
	dev->bulk_in_filled = 0;
	/* dev->errors = 0; */

	/* prepare a read */
	usb_fill_bulk_urb(dev->bulk_in_urb,
			dev->udev,
			usb_rcvbulkpipe(dev->udev,
				dev->bulk_in_endpointAddr),
			dev->bulk_in_buffer,
			min(max(dev->bulk_in_size, count),(size_t)MAX_TRANSFER),
			bt_hci_read_bulk_callback,
			dev);
	/* tell everybody to leave the URB alone */
	spin_lock_irqsave(&dev->err_lock, flags);
	dev->ongoing_read_bulk = 1;
	spin_unlock_irqrestore(&dev->err_lock, flags);

	/* do it */
	rv = usb_submit_urb(dev->bulk_in_urb, GFP_KERNEL);
	if (rv < 0) {
		err("%s - failed submitting read urb, error %d",
			__func__, rv);
		dev->bulk_in_filled = 0;
		rv = (rv == -ENOMEM) ? rv : -EIO;
		
		spin_lock_irqsave(&dev->err_lock, flags);
		dev->ongoing_read_bulk = 0;
		spin_unlock_irqrestore(&dev->err_lock, flags);
	}

#ifdef DEBUG_BTC
	printk(KERN_INFO"read_bulk usb_submit_urb\n");
#endif
	
	return rv;
}

static ssize_t bt_hci_read(struct file *file, char *buffer, size_t count,
			 loff_t *ppos)
{
	struct usb_bt_hci *dev;
	int rv = 0;

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read called,flg:%x \n", file->f_flags);
#endif

#if 0 /* MeeGo */
	if( ( file->f_flags & O_NONBLOCK ) == O_NONBLOCK ) {
		file->f_flags = file->f_flags & ~O_NONBLOCK;

		printk(KERN_INFO"bt_hci_read O_NONBLOCK down:0x%08x\n", file->f_flags);
	}
#endif

	dev = (struct usb_bt_hci *)file->private_data;

	/* open時のフラグでHCIイベントのreadとACLデータのread切り替え */
	if (file->f_flags & O_HCI_ACLDATA) {
		/* ACLデータ = BULK IN */
		if (dev->bulk_in_urb) {
			rv = bt_hci_read_bulk(file, buffer, count, ppos);
		} else {
			err("%s - no bulk_in_urb", __func__);

			/* 2013.06.19 エラーコード修正 start */
			rv = -ENOENT;
			/* 2013.06.19 エラーコード修正 end */
		}
	} else {
		/* HCIイベント = INT IN */
		 if (dev->int_in_urb) {
			rv = bt_hci_read_int(file, buffer, count, ppos);
		} else {
			err("%s - no int_in_urb", __func__);

			/* 2013.06.19 エラーコード修正 start */
			rv = -ENOENT;
			/* 2013.06.19 エラーコード修正 end */
		}
	}

	return rv;

}

static ssize_t bt_hci_read_bulk(struct file *file, char *buffer, size_t count,
			 loff_t *ppos)
{
	// BT Module Complete proc

	struct usb_bt_hci *dev;
	int rv;
	bool ongoing_io;
	/* 2013.06.17 排他修正 start */
	struct usb_interface *interface;
	unsigned long flags;
	/* 2013.06.17 排他修正 end   */

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_bulk called\n");
#endif

	dev = (struct usb_bt_hci *)file->private_data;

	/* if we cannot read at all, return EOF */
	if (!dev->bulk_in_urb || !count) {
#ifdef DEBUG_BTC
	        printk(KERN_INFO"bt_hci_read_bulk EOF\n");
#endif
		return 0;
	}

	/* no concurrent readers */
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_bulk lock\n");
#endif
	rv = mutex_lock_interruptible(&dev->io_mutex_acl_r);
	if (rv < 0){
#ifdef DEBUG_BTC
	        printk(KERN_INFO"bt_hci_read_bulk no concurrent readers\n");
#endif
		return rv;
	}
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_bulk locked\n");
#endif

	/* 2013.06.17 排他修正2 start */
	spin_lock_irqsave(&dev->devinfo_lock, flags);
	interface = dev->interface;
	spin_unlock_irqrestore(&dev->devinfo_lock, flags);

	if (!interface) {		/* disconnect() was called */
		rv = -ENODEV;
#ifdef DEBUG_BTC
	        printk(KERN_INFO"bt_hci_read_bulk no interface\n");
#endif
		goto exit;
	}
	/* 2013.06.19 排他修正2 end */

	rv = 0;
	while (count > 0 && rv >= 0) {
		size_t	available = dev->bulk_in_filled - dev->bulk_in_copied;
		/*
		 * step.1 copy outstanding data, if exist
		 */
		if (available > 0) {
			/*
			 * outstanding read data is left
			 */
			size_t	chunk = min(available, count);
			if (copy_to_user(buffer, dev->bulk_in_buffer + dev->bulk_in_copied, chunk)) {
				err(
			"%s copy_to_user addr=%p count=%d size=%d",
					__func__,
					buffer, count, chunk);

				rv = -EFAULT;
			} else {
				rv += chunk;
				count -= chunk;
				
				dev->bulk_in_copied += chunk;
			}
			break;	// return user data
			/*
			 * if continue, read append
			 * continuous data
			 */
		}
		/*
		 * step.2 read
		 */
		spin_lock_irqsave(&dev->err_lock, flags);
		ongoing_io = dev->ongoing_read_bulk;
		spin_unlock_irqrestore(&dev->err_lock, flags);

		if (!ongoing_io) {
			rv = bt_hci_do_read_io_bulk(dev, count);
			continue;
		}
		//step2 error check
		rv = dev->bulk_read_errors;
		if (dev->bulk_read_errors) {
			printk(KERN_INFO"bt_hci_read_bulk error %d\n",rv);
			/* any error is reported once */
			dev->bulk_read_errors = 0;
			/* to preserve notifications about reset */
			/* EPIPE時はreadsize=0に付け替える(EPIPEの無視) */
			rv = (rv == -EPIPE) ? 0 : -EIO;
			/* rv = (rv == -EPIPE) ? rv : -EIO; */
			/* no data to deliver */
			dev->bulk_in_filled = 0;
			/* report it */
			break;
		}	

		/*
		 * step.3 wait completion
		 */
		if (file->f_flags & O_NONBLOCK) {
			rv = -EAGAIN;
		} else {
			rv = wait_for_completion_interruptible(
				&dev->bulk_in_completion);
			if (rv < 0) {
				printk(KERN_INFO"bt_hci_read_bulk wfci error rv = %d\n", rv);
				break;
			}
			//step3 error check
			rv = dev->bulk_read_errors;
			if (dev->bulk_read_errors) {
				printk(KERN_INFO"bt_hci_read_bulk error %d\n",rv);
				/* any error is reported once */
				dev->bulk_read_errors = 0;
				/* to preserve notifications about reset */
				/* EPIPE時はreadsize=0に付け替える(EPIPEの無視) */
				rv = (rv == -EPIPE) ? 0 : -EIO;
				/* rv = (rv == -EPIPE) ? rv : -EIO; */
				/* no data to deliver */
				dev->bulk_in_filled = 0;
				/* report it */
				break;
			}
		}
	}
	
exit:
	mutex_unlock(&dev->io_mutex_acl_r);
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_bulk fin rv = %d\n",rv);
#endif
	return rv;
}

static ssize_t bt_hci_read_int(struct file *file, char *buffer, size_t count,
			 loff_t *ppos)
{
	struct usb_bt_hci *dev;
	int rv;
	bool ongoing_io;
	/* 2013.06.17 排他修正 start */
	struct usb_interface *interface;
	unsigned long flags;
	/* 2013.06.17 排他修正 end   */

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_int called\n");
#endif

	dev = (struct usb_bt_hci *)file->private_data;

	/* if we cannot read at all, return EOF */
	if (!dev->int_in_urb || !count) {
#ifdef DEBUG_BTC_ERR
		printk(KERN_INFO"bt_hci_read_int EOF\n");
#endif
		return 0;
	}
	/* no concurrent readers */
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_int lock\n");
#endif
	rv = mutex_lock_interruptible(&dev->io_mutex);
	if (rv < 0){
#ifdef DEBUG_BTC_ERR
	        printk(KERN_INFO"bt_hci_read_bulk no concurrent readers\n");
#endif
		return rv;
	}
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_int locked\n");
#endif

	/* 2013.06.19 排他修正2 start */
	spin_lock_irqsave(&dev->devinfo_lock, flags);
	interface = dev->interface;
	spin_unlock_irqrestore(&dev->devinfo_lock, flags);

	if (!interface) {		/* disconnect() was called */
		/* ADD 2013.05.02 Start */
		mutex_unlock(&dev->io_mutex);
		/* ADD 2013.05.02 End */
		rv = -ENODEV;
#ifdef DEBUG_BTC_ERR
	        printk(KERN_INFO"bt_hci_read_int no interface\n");
#endif
		/* 2013.06.17 エラーコード修正 start */
		return rv;
		/* 2013.06.17 エラーコード修正 end */
	}
	/* 2013.06.19 排他修正2 end */
	rv = 0;

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_int dev->int_in_filled(1st) = %d\n",dev->int_in_filled);
#endif
	/* Data is not ready */
	if (dev->int_in_filled == 0) {
		spin_lock_irqsave(&dev->err_lock, flags);
		ongoing_io = dev->ongoing_read_int;
		spin_unlock_irqrestore(&dev->err_lock, flags);
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_int onging io = %d\n", ongoing_io);
#endif
		if (ongoing_io) { /* already submitted urb */
			/* nonblocking IO shall not wait */
			if (file->f_flags & O_NONBLOCK) {
#ifdef DEBUG_BTC
				printk(KERN_INFO"bt_hci_read_int NONBLOCK EAGAIN\n");
#endif
				rv = -EAGAIN;
				goto exit;
			}
		} else {
		
			/* errors must be reported */
			/* Add ver.0.9 Start */
			rv = dev->int_read_errors;
			if (rv < 0) {
				/* any error is reported once */
				dev->int_read_errors = 0;
				/* to preserve notifications about reset */
				/* EPIPE時はreadsize=0に付け替える(EPIPEの無視) */
				rv = (rv == -EPIPE) ? 0 : -EIO;
				/* no data to deliver */
				dev->int_in_filled = 0;
				/* report it */
				goto exit;
			}
			/* Add ver.0.9 End */
		
			/* submit urb */
			spin_lock_irqsave(&dev->err_lock, flags);
			dev->ongoing_read_int = 1;
			spin_unlock_irqrestore(&dev->err_lock, flags);
			rv = usb_submit_urb(dev->int_in_urb, GFP_KERNEL);
#ifdef DEBUG_BTC
	    printk(KERN_INFO"bt_hci_read_int submit int_urb rv = %d\n",rv);
#endif
			if (rv < 0) {
				err("%s - failed submitting read urb, error %d",
				__func__, rv);
				dev->int_in_filled = 0;
				rv = (rv == -ENOMEM) ? rv : -EIO;
		
				spin_lock_irqsave(&dev->err_lock, flags);
				dev->ongoing_read_int = 0;
				spin_unlock_irqrestore(&dev->err_lock, flags);
				goto exit;
			}

#ifdef DEBUG_BTC
			printk(KERN_INFO"read_int usb_submit_urb\n");
#endif
	
			/* nonblocking IO shall not wait */
			if (file->f_flags & O_NONBLOCK) {
#ifdef DEBUG_BTC
				printk(KERN_INFO"bt_hci_read_int NONBLOCK EAGAIN\n");
#endif
				rv = -EAGAIN;
				goto exit;
			}
#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_read_int wait_for_completion_interruptible\n");
#endif
			rv = wait_for_completion_interruptible(&dev->int_in_completion);
			if (rv < 0) {
#ifdef DEBUG_BTC_ERR
				printk(KERN_INFO"bt_hci_read_int wfci error rv = %d\n", rv);
#endif
				goto exit;
			}
#ifdef DEBUG_BTC
			printk(KERN_INFO"bt_hci_read_int completion_interruptible fin\n");
#endif

        	}
	}

	/* errors must be reported */
	rv = dev->int_read_errors;
	if (rv < 0) {
#ifdef DEBUG_BTC_ERR
		printk(KERN_INFO"bt_hci_read_int error %d\n",rv);
#endif
		/* any error is reported once */
		dev->int_read_errors = 0;
		/* to preserve notifications about reset */
		/* EPIPE時はreadsize=0に付け替える(EPIPEの無視) */
		rv = (rv == -EPIPE) ? 0 : -EIO;
		/* rv = (rv == -EPIPE) ? rv : -EIO; */
		/* no data to deliver */
		dev->int_in_filled = 0;
		/* report it */
		goto exit;
	}
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_int dev->int_in_filled(2nd) = %d\n",dev->int_in_filled);
#endif
	if (dev->int_in_filled > 0) {
		size_t available = dev->int_in_filled - dev->int_in_copied;
		size_t chunk = min(available, count);
#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_read_int filled %d\n",dev->int_in_filled);
		printk(KERN_INFO"bt_hci_read_int copied %d\n",dev->int_in_copied);
		printk(KERN_INFO"bt_hci_read_int available, count %d, %d -> %d\n",available,count,chunk);
#endif
		if (copy_to_user(buffer,
			 dev->int_in_buffer + dev->int_in_copied,
			 chunk)) {
			 
			 err(
			"%s copy_to_user addr=%p count=%d size=%d",
					__func__,
					buffer, count, chunk);
					
			rv = -EFAULT ;
		} else {
			rv = chunk ;
		}
		/* all data transfer ? */
		if ( available == chunk ) {
		  dev->int_in_filled = 0;
		  dev->int_in_copied = 0;
		} else {
			/* data remain */
		  	dev->int_in_copied += chunk;
		}
	}
exit:
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_read_int fin %d\n",rv);
#endif
	mutex_unlock(&dev->io_mutex);
	return rv;
}


static void bt_hci_write_bulk_callback(struct urb *urb)
{
	struct usb_bt_hci *dev;
	unsigned long flags;

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_write_bulk_callback called\n");
#endif

	dev = urb->context;

	/* sync/async unlink faults aren't errors */
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			err("%s - nonzero write bulk status received: %d",
			    __func__, urb->status);

		spin_lock_irqsave(&dev->err_lock, flags);
		dev->bulk_write_errors = urb->status;
		err("%s - errors received: %d", __func__, dev->bulk_write_errors);
		spin_unlock_irqrestore(&dev->err_lock, flags);
	} else {
		dev->bulk_write_errors = 0;
	}

	/* free up our allocated buffer */
	usb_free_coherent(urb->dev, urb->transfer_buffer_length,
//	usb_buffer_free(urb->dev, urb->transfer_buffer_length,
			urb->transfer_buffer, urb->transfer_dma);
	up(&dev->limit_sem);
	
	/* ADD */
	complete(&dev->bulk_out_completion);

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_write_bulk_callback fin error = %d\n", dev->bulk_write_errors);
#endif
}

static ssize_t bt_hci_write(struct file *file, const char *user_buffer,
			  size_t count, loff_t *ppos)
{
	struct usb_bt_hci *dev;
	int retval = 0;
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_write called,flg:0x%x\n", file->f_flags);
#endif

	dev = (struct usb_bt_hci *)file->private_data;

#if 0 /* MeeGo */
	if( ( file->f_flags & O_NONBLOCK ) == O_NONBLOCK ) {
		file->f_flags = file->f_flags & ~O_NONBLOCK;
#ifdef DEBUG_BTC_ERR
		printk(KERN_INFO"bt_hci_write O_NONBLOCK down:0x%08x\n", file->f_flags);
#endif
	}
#endif

	/* open時のフラグでHCIコマンドのwriteとACLデータのwrite切り替え */
	if (file->f_flags & O_HCI_ACLDATA) {
		/* ACLデータ = BULK OUT */
		retval = bt_hci_bulk_write(file, user_buffer, count, ppos);
	} else {
		/* HCIコマンド = CONTROL OUT */
		/* INTERRUPT転送のwrite側をコントロール転送のwriteに使っているので */
		/* 関数名はbt_hci_int_writeにしている。 */
		retval = bt_hci_int_write(file, user_buffer, count, ppos);
	}

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_write retval = %d\n", retval);
#endif
	return retval;
}

static ssize_t bt_hci_bulk_write(struct file *file, const char *user_buffer,
			  size_t count, loff_t *ppos)
{
	struct usb_bt_hci *dev;
	int retval = 0;
	struct urb *urb = NULL;
	char *buf = NULL;
	size_t writesize = min(count, (size_t)MAX_TRANSFER);
	/* 2013.06.17 排他修正 start */
	struct usb_interface *interface;
	unsigned long flags;
	/* 2013.06.17 排他修正 end   */

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_write_bulk called\n");
#endif

	dev = (struct usb_bt_hci *)file->private_data;

	/* verify that we actually have some data to write */
	if (count == 0)
		goto exit;

	/*
	 * limit the number of URBs in flight to stop a user from using up all
	 * RAM
	 */
	if (!(file->f_flags & O_NONBLOCK)) {
		if (down_interruptible(&dev->limit_sem)) {
			retval = -ERESTARTSYS;
			goto exit;
		}
	} else {
		if (down_trylock(&dev->limit_sem)) {
			retval = -EAGAIN;
			goto exit;
		}
	}

	spin_lock_irqsave(&dev->err_lock, flags);
	retval = dev->bulk_write_errors;
	if (retval < 0) {
		/* any error is reported once */
		dev->bulk_write_errors = 0;
		/* to preserve notifications about reset */
		/* 2013.04.19 EPIPE時はENOMEMに付け替える(EPIPE時は上位のリトライ処理を期待) */
		retval = (retval == -EPIPE) ? -ENOMEM : -EIO;
		/* retval = (retval == -EPIPE) ? retval : -EIO; */
	}
	spin_unlock_irqrestore(&dev->err_lock, flags);
	if (retval < 0)
		goto error;

	/* create a urb, and a buffer for it, and copy the data to the urb */
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		retval = -ENOMEM;
		goto error;
	}

	buf = usb_alloc_coherent(dev->udev, writesize, GFP_KERNEL,
//	buf = usb_buffer_alloc(dev->udev, writesize, GFP_KERNEL,
			       &urb->transfer_dma);
	if (!buf) {
		retval = -ENOMEM;
		goto error;
	}

	if (copy_from_user(buf, user_buffer, writesize)) {
		retval = -EFAULT;
		goto error;
	}

	/* this lock makes sure we don't submit URBs to gone devices */
	mutex_lock(&dev->io_mutex_acl_w);

	/* 2013.06.19 排他修正2 start */
	spin_lock_irqsave(&dev->devinfo_lock, flags);
	interface = dev->interface;
	spin_unlock_irqrestore(&dev->devinfo_lock, flags);

	if (!interface) {		/* disconnect() was called */
		mutex_unlock(&dev->io_mutex_acl_w);
		retval = -ENODEV;
		goto error;
	}
	/* 2013.06.19 排他修正2 end */

	/* initialize the urb properly */
	usb_fill_bulk_urb(urb, dev->udev,
			  usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
			  buf, writesize, bt_hci_write_bulk_callback, dev);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	usb_anchor_urb(urb, &dev->submitted);

	/* send the data out the bulk port */
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_write_bulk submit_urb \n");
#endif
	retval = usb_submit_urb(urb, GFP_KERNEL);
	mutex_unlock(&dev->io_mutex_acl_w);
	if (retval) {
		err("%s - failed submitting write urb, error %d", __func__,
		    retval);
		
		/* 2013.04.19 EPIPE時はENOMEMに付け替える(EPIPE時は上位のリトライ処理を期待) */
		retval = (retval == -EPIPE) ? -ENOMEM : retval;
		
		goto error_unanchor;
	}

#ifdef DEBUG_BTC
	printk(KERN_INFO"write_bulk usb_submit_urb\n");
#endif
	
	/*
	 * release our reference to this urb, the USB core will eventually free
	 * it entirely
	 */
	usb_free_urb(urb);

	/* waiting for write sequence is completed */
	retval = wait_for_completion_interruptible(&dev->bulk_out_completion);
	if (retval < 0) {
		printk(KERN_INFO"bt_hci_bulk_write wfci error retval = %d\n", retval);
		goto error;
	}
	
	spin_lock_irqsave(&dev->err_lock, flags);
	retval = dev->bulk_write_errors;
	if (retval < 0) {
		/* any error is reported once */
		dev->bulk_write_errors = 0;
		/* to preserve notifications about reset */
		/* EPIPE時はENOMEMに付け替える(EPIPE時は上位のリトライ処理を期待) */
		retval = (retval == -EPIPE) ? -ENOMEM : -EIO;
	}
	spin_unlock_irqrestore(&dev->err_lock, flags);
	if (retval < 0)
		goto error;
	

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_write_bulk fin writesize = %d\n", writesize);
#endif
	return writesize;

error_unanchor:
	usb_unanchor_urb(urb);
error:

	if (urb) {
		usb_free_coherent(dev->udev, writesize, buf, urb->transfer_dma);
//		usb_buffer_free(dev->udev, writesize, buf, urb->transfer_dma);
		usb_free_urb(urb);
	}
	up(&dev->limit_sem);

exit:
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_write_bulk error retval = %d\n", retval);
#endif
	return retval;
}

/*
 * HCIでは、Interrupt転送のOUT側がないので、
 * controlエンドポイントへの書き込みは、Interrupt転送のデバイスのwrite側を使って
 * 行うものとする。
 * そのため関数名はbt_hci_int_writeにしている。
 */
static ssize_t bt_hci_int_write(struct file *file, const char *user_buffer,
			  size_t count, loff_t *ppos)
{
	struct usb_bt_hci *dev;
	int retval = 0;
	char *buf = NULL;
	int result;
	size_t writesize = min(count, (size_t)MAX_TRANSFER);
        struct usb_interface *interface;
        struct usb_host_interface *iface_desc;
	__u8 request;
	__u8 requesttype;
	__u8 value;
	/* 2013.06.19 排他修正2 start */
	__u16 index = 0;
	/* 2013.06.19 排他修正2 end   */
	/* 2013.06.17 排他修正 start */
	unsigned long flags;
	/* 2013.06.17 排他修正 end   */

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_int_write called\n");
#endif

	dev = (struct usb_bt_hci *)file->private_data;

	/* verify that we actually have some data to write */
	if (count == 0) {
		return(0);
	}

	buf = kmalloc(writesize, GFP_KERNEL);
	if (!buf) {
		retval = -ENOMEM;
		goto error;
	}

	if (copy_from_user(buf, user_buffer, writesize)) {
		kfree(buf);
		retval = -EFAULT;
		goto error;
	}


#ifdef SANKOU
	/* SEND_ENCAPSULATED_COMMAND	*/
	tUsbDevReq.bmRequestType = USB_RQTP_CS_OUT_IF; // 0x21
	tUsbDevReq.bRequest		 = USB_REQ_SEND_ENCAPSULATED_COMMAND;
	tUsbDevReq.wValue		 = 0x00;
	tUsbDevReq.wIndex		 = (WORD)ptUsbMode->byCcIfNo;
	tUsbDevReq.wLength		 = 0;

#endif
	/* bInterfaceNumberを得るために、interfaceをとる */

		/* 2013.06.19 排他修正2 start */
		spin_lock_irqsave(&dev->devinfo_lock, flags);
        interface = dev->interface;
		if(interface) {
			iface_desc = interface->cur_altsetting;
			index = iface_desc->desc.bInterfaceNumber;
		}
		spin_unlock_irqrestore(&dev->devinfo_lock, flags);

		if (!interface) {		/* disconnect() was called */
			kfree(buf);
			retval = -ENODEV;
			goto error;
		}
		/* 2013.06.19 排他修正2 end */

		requesttype = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE; // 0x21
		request = USB_REQ_SEND_ENCAPSULATED_COMMAND; // 0x00
		value = 0;

        result = usb_control_msg (dev->udev,
                                  usb_sndctrlpipe(dev->udev, 0),
                                  request,
                                  requesttype,
                                  value,
                                  index,
                                  buf,
                                  writesize,
                                  BT_HCI_HCI_COMMAND_TIMEOUT);
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_int_write usb_control_msg result = %d\n", result);
#endif
        if (result < 0) {
                err("BT_HCI control request failed result = %d", result);
                retval = result;
				
				/* 2013.04.19 EPIPE時はENOMEMに付け替える(EPIPE時は上位のリトライ処理を期待) */
				retval = (retval == -EPIPE) ? -ENOMEM : retval;
				
        } else {
		retval = writesize;
	}
	kfree(buf);
error:
	return retval;
}

static const struct file_operations bt_hci_fops = {
	.owner =	THIS_MODULE,
	.read =		bt_hci_read,
	.write =	bt_hci_write,
	.open =		bt_hci_open,
	.release =	bt_hci_release,
//	.flush =	bt_hci_flush,
};

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver bt_hci_class = {
	/* .name =		"usb/bt_hci_cmd", */
	.name =		"usb_bt_hci_cmd",
	.fops =		&bt_hci_fops,
	.minor_base =	USB_BT_HCI_MINOR_BASE,
};

static int bt_hci_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	struct usb_bt_hci *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	size_t buffer_size;
	int i;
	int retval = -ENOMEM;
        int found = 0;
	/* 2013.06.17 排他修正 start */
	unsigned long flags;
	/* 2013.06.17 排他修正 end   */

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_probe called\n");
#endif
	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		err("Out of memory");
		goto error;
	}
	kref_init(&dev->kref);
	sema_init(&dev->limit_sem, WRITES_IN_FLIGHT);
	mutex_init(&dev->io_mutex);
	mutex_init(&dev->io_mutex_acl_r);
	mutex_init(&dev->io_mutex_acl_w);
	spin_lock_init(&dev->err_lock);
	/* 2013.06.17 排他修正 start */
	spin_lock_init(&dev->devinfo_lock);
	/* 2013.06.17 排他修正 end */
	init_usb_anchor(&dev->submitted);
	init_completion(&dev->bulk_in_completion);
	init_completion(&dev->int_in_completion);
	init_completion(&dev->bulk_out_completion);
	
	dev->udev = usb_get_dev(interface_to_usbdev(interface));

	/* 2013.06.17 排他修正 start */
	spin_lock_irqsave(&dev->devinfo_lock, flags);
	dev->interface = interface;
	spin_unlock_irqrestore(&dev->devinfo_lock, flags);
	/* 2013.06.17 排他修正 end */

	/* set up the endpoint information */
	/* use only the first bulk-in and bulk-out endpoints */
	iface_desc = interface->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;
#ifdef DEBUG_BTC
		printk(KERN_INFO"bt_hci_probe: Attr[%d]:%x\n",i,endpoint->bmAttributes);
#endif
		if (!dev->bulk_in_endpointAddr &&
		    usb_endpoint_is_bulk_in(endpoint)) {
#ifdef DEBUG_BTC
			printk(KERN_INFO"bt_hci_probe: found bulk_in\n");
#endif
			/* we found a bulk in endpoint */
			found ++;
			buffer_size = MAX_TRANSFER;
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
#ifdef DEBUG_BTC
			printk(KERN_INFO"bt_hci_probe dev = 0x%x, endpointaddr = 0x%x, buffer_size = %d\n",(int)dev, (int)(dev->bulk_in_endpointAddr), buffer_size);
#endif
			dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
			if (!dev->bulk_in_buffer) {
				err("Could not allocate bulk_in_buffer");
				goto error;
			}
			dev->bulk_in_urb = usb_alloc_urb(0, GFP_KERNEL);
			if (!dev->bulk_in_urb) {
				err("Could not allocate bulk_in_urb");
				goto error;
			}
		}

		if (!dev->bulk_out_endpointAddr &&
		    usb_endpoint_is_bulk_out(endpoint)) {
#ifdef DEBUG_BTC
			printk(KERN_INFO"bt_hci_probe: found bulk_out\n");
#endif
			/* we found a bulk out endpoint */
			found ++;
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
#ifdef DEBUG_BTC
			printk(KERN_INFO"bt_hci_probe dev = 0x%x, endpointaddr = 0x%x\n",(int)dev, (int)(dev->bulk_out_endpointAddr));
#endif
		}

		if (!dev->int_in_endpointAddr &&
		    usb_endpoint_is_int_in(endpoint)) {
#ifdef DEBUG_BTC
			printk(KERN_INFO"bt_hci_probe: found int_in\n");
#endif
			/* we found a int in endpoint */
			found ++;
			buffer_size = MAX_TRANSFER;
			dev->int_in_size = buffer_size;
			dev->int_in_endpointAddr = endpoint->bEndpointAddress;
			dev->int_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
			if (!dev->int_in_buffer) {
				err("Could not allocate int_in_buffer");
				goto error;
			}
			dev->int_in_urb = usb_alloc_urb(0, GFP_KERNEL);
			if (!dev->int_in_urb) {
				err("Could not allocate int_in_urb");
				goto error;
			}
		        dev->int_in_interval = min_int_in_interval > endpoint->bInterval ? min_int_in_interval : endpoint->bInterval;
#ifdef DEBUG_BTC
			printk(KERN_INFO"bt_hci_probe dev = 0x%x, endpointaddr = 0x%x, buffer_size = %d\n",(int)dev, (int)(dev->int_in_endpointAddr), buffer_size);
			printk(KERN_INFO"bt_hci_probe: int_interval %d,%d\n",min_int_in_interval,endpoint->bInterval);
#endif

		}
		if (!dev->int_out_endpointAddr &&
		    usb_endpoint_is_int_out(endpoint)) {
#ifdef DEBUG_BTC
			printk(KERN_INFO"bt_hci_probe: found int_out\n");
#endif
			/* we found a int out endpoint */
			found ++;
			dev->int_out_endpointAddr = endpoint->bEndpointAddress;
		}

	}
	if (found == 0) {
		char	buf[300];
		int	offset = 0;

		offset = snprintf(buf, sizeof(buf),
			"Could not find endpoints ");
		for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
			endpoint = &iface_desc->endpoint[i].desc;

			offset += snprintf(buf + offset,
				sizeof(buf) - offset,
				"(%d) epaddr=%d attib=%x ",
				i,
				endpoint->bEndpointAddress,
				endpoint->bmAttributes);
		}
		err("%s", buf);
		goto error;
	}

	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);

	/* we can register the device now, as it is ready */
	retval = usb_register_dev(interface, &bt_hci_class);
	if (retval) {
		/* something prevented us from registering this driver */
		err("Not able to get a minor for this device.");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	/* let the user know what node this device is now attached to */
	dev_info(&interface->dev,
		 "USB BT_HCI v1.0 device now attached to bt_hci-%d",
		 interface->minor);
	return 0;

error:
	if (dev)
		/* this frees allocated memory */
		kref_put(&dev->kref, bt_hci_delete);
	return retval;
}

static void bt_hci_disconnect(struct usb_interface *interface)
{
	struct usb_bt_hci *dev;
	int minor = interface->minor;
	/* 2013.06.17 排他修正 start */
	unsigned long flags;
	/* 2013.06.17 排他修正 end */

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_disconnect called\n");
#endif

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

	/* give back our minor */
	usb_deregister_dev(interface, &bt_hci_class);


	/* prevent more I/O from starting */
	/* 2013.06.17 排他修正 start */
	spin_lock_irqsave(&dev->devinfo_lock, flags);
	dev->interface = NULL;
	spin_unlock_irqrestore(&dev->devinfo_lock, flags);
	/* 2013.06.17 排他修正 end */
	
	usb_kill_anchored_urbs(&dev->submitted);

	/* decrement our usage count */
	kref_put(&dev->kref, bt_hci_delete);

	dev_info(&interface->dev, "USB BT_HCI #%d now disconnected", minor);
}

static void bt_hci_draw_down(struct usb_bt_hci *dev)
{
	int time;

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_draw_down called\n");
#endif

	time = usb_wait_anchor_empty_timeout(&dev->submitted, 1000);
	if (!time)
		usb_kill_anchored_urbs(&dev->submitted);
	if (dev->bulk_in_urb) {
		dev->ongoing_read_bulk = 0;
		usb_kill_urb(dev->bulk_in_urb);
	}
	if (dev->int_in_urb) {
		dev->ongoing_read_int = 0;
		usb_kill_urb(dev->int_in_urb);
	}
}

/*
static void bt_hci_draw_down_bulk(struct usb_bt_hci *dev)
{
	int time;

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_draw_down_bulk called\n");
#endif

	time = usb_wait_anchor_empty_timeout(&dev->submitted, 1000);
	if (!time)
		usb_kill_anchored_urbs(&dev->submitted);
	if (dev->bulk_in_urb) {
		dev->ongoing_read_bulk = 0;
		usb_kill_urb(dev->bulk_in_urb);
	}
}

static void bt_hci_draw_down_int(struct usb_bt_hci *dev)
{
	int time;

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_draw_down_int called\n");
#endif

	time = usb_wait_anchor_empty_timeout(&dev->submitted, 1000);
	if (!time)
		usb_kill_anchored_urbs(&dev->submitted);
	if (dev->int_in_urb) {
		dev->ongoing_read_int = 0;
		usb_kill_urb(dev->int_in_urb);
	}
}
*/

static int bt_hci_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct usb_bt_hci *dev = usb_get_intfdata(intf);

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_suspend called\n");
#endif

	if (!dev)
		return 0;
	bt_hci_draw_down(dev);
	return 0;
}

static int bt_hci_resume(struct usb_interface *intf)
{
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_resume called\n");
#endif

	return 0;
}

static int bt_hci_pre_reset(struct usb_interface *intf)
{
	struct usb_bt_hci *dev = usb_get_intfdata(intf);

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_pre_reset called\n");
#endif

	mutex_lock(&dev->io_mutex);
	bt_hci_draw_down(dev);

	return 0;
}

static int bt_hci_post_reset(struct usb_interface *intf)
{
	struct usb_bt_hci *dev = usb_get_intfdata(intf);

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_reset called\n");
#endif

	/* we are sure no URBs are active - no locking needed */
	dev->int_read_errors = -EPIPE;
	dev->bulk_read_errors = -EPIPE;
	dev->bulk_write_errors = -EPIPE;
	mutex_unlock(&dev->io_mutex);

	return 0;
}

static struct usb_driver bt_hci_driver = {
	.name =		"bt_hci",
	.probe =	bt_hci_probe,
	.disconnect =	bt_hci_disconnect,
	.suspend =	bt_hci_suspend,
	.resume =	bt_hci_resume,
	.pre_reset =	bt_hci_pre_reset,
	.post_reset =	bt_hci_post_reset,
	.id_table =	bt_hci_table,
#if 1 /* retry */
	.supports_autosuspend = 1,
#endif
};

static int __init usb_bt_hci_init(void)
{
	int result;

#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_init called\n");
#endif
	/* register this driver with the USB subsystem */
	result = usb_register(&bt_hci_driver);
	if (result)
		err("usb_register failed. Error number %d", result);

	return result;
}

static void __exit usb_bt_hci_exit(void)
{
#ifdef DEBUG_BTC
	printk(KERN_INFO"bt_hci_exit called\n");
#endif
	/* deregister this driver with the USB subsystem */
	usb_deregister(&bt_hci_driver);
}

module_init(usb_bt_hci_init);
module_exit(usb_bt_hci_exit);

MODULE_VERSION("v001_11");
MODULE_LICENSE("GPL");
