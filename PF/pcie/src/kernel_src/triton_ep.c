#include <linux/pci.h>
#include <linux/fs.h>			       /* file stuff */
#include <linux/kernel.h>		       /* printk() */
#include <linux/errno.h>		       /* error codes */
#include <linux/module.h>		       /* THIS_MODULE */
#include <linux/cdev.h>			       /* char device stuff */
#include <linux/mod_devicetable.h>
#include <linux/types.h>
#include <linux/poll.h>
#include <linux/unistd.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <asm/uaccess.h>                /* copy_to_user() */
#include <asm/system.h>
#include <linux/mutex.h>

#include "device_file.h"
#include "triton_ep.h"
#include "pciex_common.h"
#include "pciex_manageshm.h"
#include "pciex_types.h"
#include "pciex.h"

//#define TIMER 1
#if 0 // For real MSI
    #ifdef TIMER
        struct timer_list atom_timer;
        #define BLINK_DELAY HZ/50
    #endif
#endif // For real MSI

//#define WRITE_TIMEOUT 10*HZ
#define WRITE_TIMEOUT 2*HZ

// For starping out log, the below macro is added.
//#define ENABLE_LOG

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wipro Technologies");

struct triton_ep_block struct_10MB;
static dev_t devid;

unsigned int dev_open = 0;
unsigned int dev_close = 0;
unsigned int open_count = 0;
unsigned int data_readable = 0;

// To debug the MSI issue.
unsigned int read_complete_count = 0;
unsigned int triton_write_count = 0;
unsigned int write_count = 0;
unsigned read_count = 0;

static struct triton_ep_dev *g_tdev = NULL;

DEFINE_MUTEX(pcie_mutex);

/**
 * @fn static irqreturn_t triton_ep_interrupt(int irq, void *dev_id)
 * @brief
 * ISR which catches the MSI coming from EP side.
 * @param[in] irq - irq number
 * @param[in] dev_id Driver/Unit information
 * @return IRQ_HANDLED     Completed successfully.
 */
static irqreturn_t triton_ep_interrupt(int irq, void *dev_id)
{
#if 0
   struct triton_ep_dev *priv = (struct triton_ep_dev *) dev_id;
   //BUG_ON (1);
   printk(KERN_INFO "MSI recieved..\n");
   //mdelay(1000);
   /* code for notifying the pool */
   wake_up_interruptible(&priv->wait);
   return IRQ_HANDLED;
#endif
    struct triton_ep_dev *tdev = NULL;
   	unsigned int *admin = NULL;

   	tdev = g_tdev;
   	admin = (unsigned int *)(tdev->region_user.vaddr+ 0x3e0000);

#ifdef ENABLE_LOG
   	printk(KERN_INFO "MSI recieved..\n");
#endif

   	// It checks whether the data written by Atom is read by M3 or not!
   	if (admin[2] == 0x00000002)
   	{
#ifdef ENABLE_LOG
   		printk(KERN_INFO "Read completed by M3.\n");
   		printk(KERN_INFO "Read complete count = %d\n", read_complete_count++);  // To debug the MSI issue.
#endif
   		admin[2] = 0x00000000;
   		complete(&tdev->write_complete);
   	}

   	if (admin[4] == 0x00000001)
   	{
#ifdef ENABLE_LOG
   		printk(KERN_INFO "Data written by Triton.\n");
   		printk(KERN_INFO "M3 write count = %d\n", triton_write_count++);  // To debug the MSI issue.
#endif
   		admin[4] = 0x00000000;
   		data_readable = 1;
   		wake_up_interruptible(&tdev->wait);
   	}
   	return IRQ_HANDLED;
}

#if 0 // For real MSI
// The below function is the alternative of MSI. Implemented on the basis of timer.
#ifdef TIMER
static void atom_timer_func (unsigned int data)
{
    struct triton_ep_dev *tdev = NULL;
	unsigned int *admin = NULL;

	tdev = g_tdev;
	admin = (unsigned int *)(tdev->region_user.vaddr+ 0x3e0000);

	// It checks whether the data written by Atom is read by M3 or not!
	if (admin[1] == 0x00000002)
	{
		printk(KERN_INFO "Read completed by M3.\n");
		admin[1] = 0x00000000;
		complete(&tdev->write_complete);
	}

	if (admin[3] == 0x00000001)
	{
		printk(KERN_INFO "Time to wake interuptible, Something request is written by Triton.\n");
		admin[3] = 0x00000000;
		data_readable = 1;
		wake_up_interruptible(&tdev->wait);
	}

	// Reset the timer again.
	init_timer(&atom_timer);
	atom_timer.function = atom_timer_func;
	atom_timer.data = (unsigned long)0;
	atom_timer.expires = jiffies + BLINK_DELAY;
	add_timer(&atom_timer);
}
#endif
#endif // For real MSI

/**
 * @fn static ssize_t triton_ep_read (struct file *filep, char __user *buf,
                                      size_t count, loff_t *ppos)
 * @brief
 * Entry point for read operation.
 * @param[in] filep  file pointer structure
 * @param[in] buf User buffer.
 * @param[in] count Bytes to read
 * @param[in] ppos Present pointer location
 * @return int count     Completed successfully.
 * @return EFAULT - Error scenario
 */
static ssize_t triton_ep_read (struct file *filep, char __user *buf,
                               size_t count, loff_t *ppos)
{
    struct triton_ep_dev *tdev = NULL;
    char *src;
    unsigned int *admin = NULL;

    tdev = g_tdev;
    src = (char *) tdev->region_user.vaddr + 0x200000;
    admin = (unsigned int *)(tdev->region_user.vaddr+ 0x3e0000);

    if (copy_to_user(buf, src, count))
    {
    	printk(KERN_ERR "copy_to_user failed.\n");
    	return -EFAULT;
    }

    // Set the read complete bit.
    admin[5] = 0x00000002;
    //printk(KERN_INFO "admin[5] = %d\n", admin[5]);

    mutex_lock(&pcie_mutex);
    /*if (mutex_is_locked(&pcie_mutex) == 0)
    {
    	printk(KERN_INFO "Mutex not locked -1\n");
    }*/
    // Issue MSI
    admin[0] = 0x00000001;
    mutex_unlock(&pcie_mutex);
    /*if (mutex_is_locked(&pcie_mutex) != 0)
    {
     	printk(KERN_INFO "Mutex locked -1\n");
    }*/

#ifdef ENABLE_LOG
    printk(KERN_INFO "Read Count = %d\n", read_count++);   // To debug the MSI issue.
    printk(KERN_INFO "read successful\n");
#endif
    return count;
}


/**
 * @fn static ssize_t triton_ep_write (struct file *filep, const char __user *buf,
                                       size_t count, loff_t *ppos)
 * @brief
 * Entry point for write operation.
 * @param[in] filep  file pointer structure
 * @param[in] buf User buffer.
 * @param[in] count Bytes to write
 * @param[in] ppos Present pointer location
 * @return int count     Completed successfully.
 * @return EFAULT - Error scenario
 */
static ssize_t triton_ep_write (struct file *filep, const char __user *buf,
                                size_t count, loff_t *ppos)
{
    struct triton_ep_dev *tdev = NULL;
    char *dst;
    unsigned int *admin = NULL;
    unsigned long time_left;

    tdev = g_tdev;
    dst = (char *) tdev->region_user.vaddr;
    admin = (unsigned int *)(tdev->region_user.vaddr+ 0x3e0000);

    if (copy_from_user(dst, buf, count))
    {
        printk(KERN_ERR "copy_from_user failed.\n");
        return -EFAULT;
    }

    admin[3] = count; /* Need to set before setting admin[1] */
    admin[1] = 0x00000001;
    // Test Print
    //printk(KERN_INFO "set admin[1] to 1, read it from SHM = %d\n", admin[1]);

    mutex_lock(&pcie_mutex);
    /*if (mutex_is_locked(&pcie_mutex) == 0)
    {
       	printk(KERN_INFO "Mutex not locked -2\n");
    }*/
    // Issue the MSI
    admin[0] = 0x00000001;
    mutex_unlock(&pcie_mutex);
    /*if (mutex_is_locked(&pcie_mutex) != 0)
    {
       	printk(KERN_INFO "Mutex locked -2\n");
    }*/

    //printk(KERN_INFO "Write Length = %d\n", admin[3]);

    time_left = wait_for_completion_timeout(&tdev->write_complete, WRITE_TIMEOUT);
    //Print just for debug
#ifdef ENABLE_LOG
    printk(KERN_INFO "time_left = %ld\n", time_left);
    printk(KERN_INFO "Write Count = %d\n", write_count++);   // To debug the MSI issue.
#endif
    if (time_left == 0)
    {
    	count = -EIO;
    	return count;
    }
    else
    {
#ifdef ENABLE_LOG
        printk(KERN_INFO "write successful\n");
#endif
        return count;
    }
}

#if 0
static long triton_ep_ioctl_shm_get_len_client (struct triton_ep_dev *ptdev, unsigned long arg)
{
	unsigned int *length;
    unsigned int shm_len = 0;
    long rc = 0;
    struct triton_ep_dev *tdev = NULL;
    unsigned int *admin = NULL;

    if (NULL == ptdev)
    {
       return -ENOMEM;
    }

   	length = ( unsigned int*) arg;
   	tdev = g_tdev;

   	admin = (unsigned int *)(tdev->region_user.vaddr+ 0x3e0000);

    // Call the function which gets the length of shared memory /
    shm_len = (unsigned int)admin[2];
    printk(KERN_INFO "In shm_get_len_client - Data length is %d\n", shm_len);

     if (copy_to_user((void __user *)length, (const void *)&shm_len, sizeof(int)))
     {
	    printk(KERN_ERR "copy_to_user failed.\n");
	    rc = -EFAULT;
     }

	 return rc;
}
#endif

static long triton_ep_ioctl_shm_get_len (struct triton_ep_dev *ptdev, unsigned long arg)
{
	unsigned int *length;
    unsigned int shm_len = 0;
    long rc = 0;
    struct triton_ep_dev *tdev = NULL;
    unsigned int *admin = NULL;

    if (NULL == ptdev)
    {
       return -ENOMEM;
    }

   	length = ( unsigned int*) arg;
   	tdev = g_tdev;

   	admin = (unsigned int *)(tdev->region_user.vaddr+ 0x3e0000);

    // Call the function which gets the length of shared memory /
    shm_len = (unsigned int)admin[6];
#ifdef ENABLE_LOG
    printk(KERN_INFO "In shm_get_len - Data length is %d\n", shm_len);
#endif

     if (copy_to_user((void __user *)length, (const void *)&shm_len, sizeof(int)))
     {
	    printk(KERN_ERR "copy_to_user failed.\n");
	    rc = -EFAULT;
     }

	 return rc;
}

// The below function is not required now.
// But will be required in Phase 2 when SHM management will be placed
// with proper modifications.
/**
 * @fn static long triton_ep_ioctl_shm_read (struct triton_ep_dev *ptdev, unsigned long arg)
 * @brief
 * Function for ioctl read operation.
 * @param[in] ptdev  Driver/Unit information
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return EFAULT - Error scenario
 */
static long triton_ep_ioctl_shm_read (struct triton_ep_dev *ptdev, unsigned long arg)
{
    struct triton_ep_read_write *pshmread;
    long rc = 0;
    char *src;

    if (NULL == ptdev)
    {
        return -ENOMEM;
    }

    src = (char *) ptdev->region_user.vaddr;
    pshmread = (struct triton_ep_read_write*) arg;

    rc = pciex_log_readData (pshmread, ptdev);

    if (0 == rc)
    {
        if (NULL != pshmread->buf)
        {
    	     if (copy_to_user(pshmread->buf, src, pshmread->size))
            {
	         printk(KERN_ERR "copy_to_user failed.\n");
	         rc = -EFAULT;
            }
        }
    }

    return rc;
}

// The below function is not required now.
// But will be required in Phase 2 when SHM management will be placed
// with proper modifications.
/**
 * @fn static long triton_ep_ioctl_shm_write (struct triton_ep_dev *ptdev, unsigned long arg)
 * @brief
 * Function for ioctl write operation.
 * @param[in] ptdev  Driver/Unit information
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return EFAULT - Error scenario
 * @return ENOMEM -Error scenario
 */
static long triton_ep_ioctl_shm_write (struct triton_ep_dev *ptdev, unsigned long arg)
{
    struct triton_ep_read_write *pshmwrite;
    long rc = 0;
    char *dst;

    if (NULL == ptdev)
    {
        return -ENOMEM;
    }

    dst = (char *) ptdev->region_user.vaddr;
    pshmwrite = (struct triton_ep_read_write*) arg;

    if (NULL != pshmwrite->buf)
    {
        if (copy_from_user(dst, pshmwrite->buf, pshmwrite->size))
        {
            printk(KERN_ERR "copy_from_user failed.\n");
            return -EFAULT;
        }
    }

    rc = pciex_log_writeData (pshmwrite, ptdev);

    return rc;
}

// The below function is not required now.
// But will be required in Phase 2 when SHM management will be placed
// with proper modifications.
/**
 * @fn static long triton_ep_ioctl_shm_info (struct triton_ep_dev *ptdev, unsigned long arg)
 * @brief
 * Function for ioctl "get SHM info" operation.
 * @param[in] ptdev  Driver/Unit information
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static long triton_ep_ioctl_shm_info (struct triton_ep_dev *ptdev, unsigned long arg)
{
    PCIEX_SHM_INFO  *pshminfo_req;
    long rc = 0;

    if (NULL == ptdev)
    {
        return -ENOMEM;
    }

    pshminfo_req = (PCIEX_SHM_INFO *) arg;
    rc = pciex_get_shm_info_peer(ptdev, pshminfo_req);

    return rc;
}

// The below function is not required now.
// But will be required in Phase 2 when SHM management will be placed
// with proper modifications.
/**
 * @fn static long triton_ep_ioctl_shm_create (struct triton_ep_dev *ptdev, unsigned long arg)
 * @brief
 * Function for ioctl "create SHM" operation.
 * @param[in] ptdev  Driver/Unit information
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static long triton_ep_ioctl_shm_create (struct triton_ep_dev *ptdev, unsigned long arg)
{
    PCIEX_SHM_REQ  *pshmcreate_req;
    long rc = 0;

    if (NULL == ptdev)
    {
        return -ENOMEM;
    }

    pshmcreate_req = (PCIEX_SHM_REQ *) arg;
    rc = pciex_create_shm_peer(ptdev, pshmcreate_req);

    return rc;
}

// The below function is not required now.
// But will be required in Phase 2 when SHM management will be placed
// with proper modifications.
/**
 * @fn static long triton_ep_ioctl_shm_release (struct triton_ep_dev *ptdev, unsigned long arg)
 * @brief
 * Function for ioctl "release SHM" operation.
 * @param[in] ptdev  Driver/Unit information
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static long triton_ep_ioctl_shm_release (struct triton_ep_dev *ptdev, unsigned long arg)
{
    PCIEX_SHM_REQ  *pshmrelease_req;
    long rc = 0;

    if (NULL == ptdev)
    {
        return -ENOMEM;
    }

    pshmrelease_req = (PCIEX_SHM_REQ *)arg;
    rc = pciex_release_shm_peer(ptdev, pshmrelease_req);

    return rc;
}

// The below function is not required now.
// But will be required in Phase 2 when SHM management will be placed
// with proper modifications.
/**
 * @fn static long triton_ep_ioctl_shm_attach (struct triton_ep_dev *ptdev, unsigned long arg)
 * @brief
 * Function for ioctl "attach SHM" operation.
 * @param[in] ptdev  Driver/Unit information
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static long triton_ep_ioctl_shm_attach (struct triton_ep_dev *ptdev, unsigned long arg)
{
    PCIEX_SHM_REQ  *pshmattach_req;
    long rc = 0;

    if (NULL == ptdev)
    {
        return -ENOMEM;
    }

    pshmattach_req = (PCIEX_SHM_REQ*) arg;
    rc = pciex_attach_shm_peer(ptdev, pshmattach_req);

    return rc;
}

// The below function is not required now.
// But will be required in Phase 2 when SHM management will be placed
// with proper modifications.
/**
 * @fn static long triton_ep_ioctl_shm_detach (struct triton_ep_dev *ptdev, unsigned long arg)
 * @brief
 * Function for ioctl "detach SHM" operation.
 * @param[in] ptdev  Driver/Unit information
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static long triton_ep_ioctl_shm_detach (struct triton_ep_dev *ptdev, unsigned long arg)
{
    PCIEX_SHM_REQ  *pshmdetach_req;
    long rc = 0;

    if (NULL == ptdev)
    {
        return -ENOMEM;
    }

    pshmdetach_req = (PCIEX_SHM_REQ*) arg;
    rc = pciex_detach_shm_peer (ptdev, pshmdetach_req);

    return rc;
}

// The below function is not required now.
// But will be required in Phase 2 when SHM management will be placed
// with proper modifications.
/**
 * @fn static long triton_ep_ioctl_shm_lock (struct triton_ep_dev *ptdev, unsigned long arg)
 * @brief
 * Function for ioctl "lock SHM" operation.
 * @param[in] ptdev  Driver/Unit information
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static long triton_ep_ioctl_shm_lock (struct triton_ep_dev *ptdev, unsigned long arg)
{
    PCIEX_LOCK_REQ  *pshmlock_req;
    long rc = 0;

    if (NULL == ptdev)
    {
        return -ENOMEM;
    }

    pshmlock_req = (PCIEX_LOCK_REQ*) arg;
    rc = pciex_lock_shm_peer (ptdev, pshmlock_req);

    return rc;
}

// The below function is not required now.
// But will be required in Phase 2 when SHM management will be placed
// with proper modifications.
/**
 * @fn static long triton_ep_ioctl_shm_unlock (struct triton_ep_dev *ptdev, unsigned long arg)
 * @brief
 * Function for ioctl "lock SHM" operation.
 * @param[in] ptdev  Driver/Unit information
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static long triton_ep_ioctl_shm_unlock (struct triton_ep_dev *ptdev, unsigned long arg)
{
    PCIEX_LOCK_REQ  *pshmunlock_req;
    long rc = 0;

    if (NULL == ptdev)
    {
        return -ENOMEM;
    }

    pshmunlock_req = (PCIEX_LOCK_REQ*) arg;
    rc = pciex_unlock_shm_peer (ptdev, pshmunlock_req);

    return rc;
}


/**
 * @fn static long triton_ep_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
 * @brief
 * Entry point for various ioctl operations.
 * @param[in] filep fileoperation pointer
 * @param[in] cmd denotes which ioctl operation needs to perform
 * @param[in] arg - Holding pointer of the info provided by user
 * @return 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static long triton_ep_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{

    long rc = 0;
    struct triton_ep_dev *ptdev = NULL;

    if (NULL == filep)
    {
        return -ENOMEM;
    }

    ptdev = (struct triton_ep_dev *)filep->private_data;

    switch (cmd)
    {
        case GET_SHM_INFO:
#ifdef ENABLE_LOG
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside GET_SHM_INFO\n");
#endif
            rc = triton_ep_ioctl_shm_info (ptdev, arg);
            break;
        case SHM_CREATE:
#ifdef ENABLE_LOG
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_CREATE\n");
#endif
            rc = triton_ep_ioctl_shm_create (ptdev, arg);
	     break;
        case SHM_RELEASE:
#ifdef ENABLE_LOG
	        printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_RELEASE\n");
#endif
            rc = triton_ep_ioctl_shm_release (ptdev, arg);
	        break;
        case SHM_ATTACH:
#ifdef ENABLE_LOG
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_ATTACH\n");
#endif
            rc = triton_ep_ioctl_shm_attach (ptdev, arg);
            break;
        case SHM_DETACH:
#ifdef ENABLE_LOG
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_DETACH\n");
#endif
            rc = triton_ep_ioctl_shm_detach (ptdev, arg);
            break;
        case SHM_LOCK:
#ifdef ENABLE_LOG
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_LOCK\n");
#endif
            rc = triton_ep_ioctl_shm_lock (ptdev, arg);
            break;
        case SHM_UNLOCK:
#ifdef ENABLE_LOG
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_UNLOCK\n");
#endif
            rc = triton_ep_ioctl_shm_unlock (ptdev, arg);
            break;
        case SHM_READ:
#ifdef ENABLE_LOG
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_READ\n");
#endif
            rc = triton_ep_ioctl_shm_read (ptdev, arg);
            break;
        case SHM_WRITE:
#ifdef ENABLE_LOG
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_WRITE\n");
#endif
            rc = triton_ep_ioctl_shm_write (ptdev, arg);
            break;
        case SHM_GET_LEN:
#ifdef ENABLE_LOG
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_GET_LEN\n");
#endif
            rc = triton_ep_ioctl_shm_get_len(ptdev, arg);
            break;
        default:
            printk( KERN_NOTICE "PCIEX-DRIVER: Not supported command\n");
            break;

        return rc;
    }

#ifdef ENABLE_LOG
    printk(KERN_INFO "ioctl successful\n");
#endif
    return 0;
}


/**
 * @fn static int triton_ep_open(struct inode *inode, struct file *filep)
 * @brief
 * Entry point for device open operation.
 * @param[in] filep  file pointer structure
 * @param[in] inode information regarding to inode.
 * @return int 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static int triton_ep_open(struct inode *inode, struct file *filep)
{
    int ret = 0;
#ifdef ENABLE_LOG
    printk(KERN_INFO "Within open.\n");
#endif

    if (dev_open == 0 && open_count == 0)
    {
		//unsigned int irq; /* Wipro: removed as "irq" is defined as in triton private data structure(triton_ep_dev) */
		struct triton_ep_dev *tdev = NULL;

		if (NULL == inode || NULL == filep)
		{
			printk (KERN_ERR "NULL pointer in open\n");
			return -ENOMEM;
		}

#ifdef ENABLE_LOG
		printk(KERN_INFO "Open successful\n");
#endif
		tdev = g_tdev;
		tdev->irq = tdev->pdev->irq;

#if 1
		if((ret = request_irq(tdev->irq, triton_ep_interrupt, 0, MODULE_NAME, tdev)) < 0)
		{
			printk(KERN_NOTICE "Problem acquiring the IRQ, request_irq returned %d\n", ret);
		}
#endif

		/* store tdev for use in read, write methods etc */
		filep->private_data = (void*) g_tdev;

		//Init the poll head.
		init_waitqueue_head(&tdev->wait);

		// Init the write_complete.
		init_completion(&tdev->write_complete);

#if 0
		// Lets do the timer work.
	#ifdef TIMER
		init_timer(&atom_timer);
		atom_timer.function = atom_timer_func;
		atom_timer.data = (unsigned long)0;
		atom_timer.expires = jiffies + BLINK_DELAY;
		add_timer(&atom_timer);
	#endif
#endif

		dev_open = 1;
    }
    open_count++;
    return ret;
}


/**
 * @fn static int triton_ep_release(struct inode *inode, struct file *filep)
 * @brief
 * Entry point for device release operation.
 * @param[in] filep  file pointer structure
 * @param[in] inode information regarding to inode.
 * @return int 0     Completed successfully.
 * @return ENOMEM - Error scenario
 */
static int triton_ep_release(struct inode *inode, struct file *filep)
{
#ifdef ENABLE_LOG
	printk(KERN_INFO "Within release\n");
#endif
	if (dev_open == 1 && open_count == 1)
	{
		//struct triton_ep_dev *priv = NULL; /* Wipro: Not used in this function */
		//unsigned int irq; /* Wipro: removed as "irq" is defined as in triton private data structure(triton_ep_dev) */

		if (NULL == inode || NULL == filep)
		{
			 printk (KERN_ERR "NULL pointer in release\n");
			return -ENOMEM;
		}
   		// Wipro: Commented and moved to triton_en_remove function because of pcidev is freed
   		// as free_irq points to NULL pointer

		//irq = priv->irq;

#ifdef ENABLE_LOG
		printk(KERN_INFO "release successful\n");
#endif

	//#if 1
		/* release IRQ */
		//free_irq (priv->irq, priv);
	//#endif

	#ifdef TIMER
		del_timer(&atom_timer);
	#endif
		dev_open = 0;
		open_count = 0;
    }
	else if (dev_open == 1 && open_count > 1)
	{
		open_count--;
	}
    return 0;
}

static void triton_vma_open(struct vm_area_struct *vma)
{
#ifdef ENABLE_LOG
	printk(KERN_INFO "vma_open start\n");
#endif
	struct triton_ep_dev *tdev = vma->vm_private_data;
	tdev->vma_count++;
#ifdef ENABLE_LOG
	printk(KERN_INFO "vma_open end\n");
#endif
}

static void triton_vma_close(struct vm_area_struct *vma)
{
#ifdef ENABLE_LOG
	printk(KERN_INFO "vma_close start\n");
#endif
	struct triton_ep_dev *tdev = vma->vm_private_data;
	tdev->vma_count--;
#ifdef ENABLE_LOG
	printk(KERN_INFO "vma_close end\n");
#endif
}

static int triton_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
#ifdef ENABLE_LOG
	printk(KERN_INFO "vma_fault start\n");
#endif
	page = virt_to_page(struct_10MB.vaddr);
	get_page(page);
	vmf->page = page;
#ifdef ENABLE_LOG
	printk(KERN_INFO "vma_fault end\n");
#endif
	return 0;
}

static const struct vm_operations_struct triton_vm_ops = {
	.open =  triton_vma_open,
	.close = triton_vma_close,
	.fault = triton_vma_fault,
};

static int uio_mmap_physical(struct vm_area_struct *vma)
{
	struct triton_ep_dev *tdev = vma->vm_private_data;
#ifdef ENABLE_LOG
	printk(KERN_INFO "mmap_physical start\n");
#endif

	vma->vm_flags |= VM_IO | VM_RESERVED;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	printk(KERN_INFO "mmap_physical end\n");

	return remap_pfn_range(vma,
			               vma->vm_start,
			               struct_10MB.paddr >> PAGE_SHIFT,
			               vma->vm_end - vma->vm_start,
			               vma->vm_page_prot);
}

static int triton_mmap_logical(struct vm_area_struct *vma)
{
#ifdef ENABLE_LOG
	printk(KERN_INFO "mmap_logical start\n");
#endif
	vma->vm_flags |= VM_RESERVED;
	vma->vm_ops = &triton_vm_ops;
	triton_vma_open(vma);
#ifdef ENABLE_LOG
	printk(KERN_INFO "mmap_logical end\n");
#endif
	return 0;
}


// Not required.
static int triton_ep_mmap(struct file *filep, struct vm_area_struct *vma)
{
	struct triton_ep_dev *tdev = filep->private_data;
	unsigned long requested_pages, actual_pages;

#ifdef ENABLE_LOG
	printk(KERN_INFO "triton_mmap start\n");
#endif

	if (vma->vm_end < vma->vm_start)
	{
#ifdef ENABLE_LOG
		printk(KERN_INFO "triton_mmap -1\n");
#endif
		return -EINVAL;
	}

	vma->vm_private_data = tdev;

	requested_pages = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
	actual_pages = ((struct_10MB.paddr & ~PAGE_MASK)
			+ struct_10MB.len + PAGE_SIZE -1) >> PAGE_SHIFT;
	if (requested_pages > actual_pages)
	{
#ifdef ENABLE_LOG
		printk(KERN_INFO "triton_mmap -2\n");
#endif
		return -EINVAL;
	}

#ifdef ENABLE_LOG
	printk(KERN_INFO "triton_mmap end\n");
#endif
    //return triton_mmap_logical(vma);
	return uio_mmap_physical(vma);
}


/**
 * @fn static unsigned int triton_ep_poll(struct file *filep, struct poll_table_struct *wait)
 * @brief
 * Entry point for polling operation.
 * @param[in] filep  file pointer structure
 * @param[in] wait poll_table_struct pointer.
 * @return 0     Completed successfully.
 * @return ENOMEM - Error scenario
 * @return EIO - Error scenario
 */
static unsigned int triton_ep_poll(struct file *filep, struct poll_table_struct *wait)
{
    struct triton_ep_dev *priv = NULL;
    unsigned int mask = 0;

#ifdef ENABLE_LOG
    printk(KERN_INFO "within poll\n");
#endif

    priv = (struct triton_ep_dev *) filep->private_data;

    if(!priv->pdev->irq)
       return -EIO;

    poll_wait(filep, &priv->wait, wait);

    if(data_readable == 1)
    {
        mask |= POLLIN;
        data_readable = 0;
    }
    /*if(condition to check device is writable)
     mask |= POLLOUT | POLLWRNORM;*/
#ifdef ENABLE_LOG
    printk(KERN_INFO "poll successful\n");
#endif
    return mask;
}


/* File Operation structure - Contains all the device driver
                                          entry points.
*/
static struct file_operations triton_ep_fops =
{
    .owner             = THIS_MODULE,
    .read              = triton_ep_read,
    .write             = triton_ep_write,
    .unlocked_ioctl    = triton_ep_ioctl,
    .open              = triton_ep_open,
    .release           = triton_ep_release,
    .mmap              = triton_ep_mmap,
    .poll	           = triton_ep_poll
};

/**
 * @fn static int __devinit triton_ep_probe(struct pci_dev *pdev,
			                                const struct pci_device_id *entry)
 * @brief
 * Probe function of the driver, which initialise most of the stuffs..
 */
static int __devinit triton_ep_probe(struct pci_dev *pdev,
			                                     const struct pci_device_id *entry)
{
    int rc, devno, major;
    struct triton_ep_dev *tdev = NULL;

    if (NULL == pdev || NULL == entry)
    {
    	printk (KERN_INFO "PCIe: NULL pointer in probe\n");
        return -ENOMEM;
    }

    devno = MKDEV(TRITON_EP_MAJOR_NUM, TRITON_EP_MINOR_NUM);

    tdev = kzalloc(sizeof(struct triton_ep_dev), GFP_KERNEL);
    if(!tdev)
    {
        printk(KERN_INFO "PCIe: Unable to allocate triton ep dev \n");
        return -ENOMEM;
    }

    /* Stroring the struct pdev */
    tdev->pdev = pdev;

    g_tdev = tdev;

    rc = pci_enable_device(pdev);
    if (rc < 0)
    {
        printk(KERN_INFO "PCIe: Failed to enable PCI device\n");
        goto tdev_cleanup;
    }

    if (!(pci_resource_flags(pdev, BAR0) & IORESOURCE_MEM))
    {
        printk(KERN_INFO "PCIe: region #0 is not an MMIO resource, aborting\n");
        rc = -ENOMEM;
        goto pdev_cleanup;
    }


    pci_set_master(pdev);

    if ((rc = pci_enable_msi(pdev)<0))
    {
        printk(KERN_INFO "PCIe: pci_enable_msi failed with the error code %d\n", rc);
        goto pdev_cleanup;
    }
    /* At this point pdev->irq have the correct irq info */

    //temp
	//Init the poll head.
	//init_waitqueue_head(&tdev->wait);
#if 0
    if((rc = request_irq(pdev->irq, triton_ep_interrupt, 0, MODULE_NAME, tdev)) < 0)
    {
       printk(KERN_INFO "PCIe: Problem acquiring the IRQ, request_irq returned %d\n", rc);
       goto msi_cleanup;
    }
#endif

    tdev->region_user.len = pci_resource_len(pdev, BAR0);
    if (tdev->region_user.len != DATA_AREA_LEN_1MB)
    {
        printk(KERN_INFO "PCIe: Data area length is not correct\n");
        rc = -ENODEV;
        goto irq_cleanup;
    }

    rc = pci_request_regions(pdev, MODULE_NAME);
    if (rc < 0)
    {
    	printk(KERN_INFO "PCIe: Failed to request regions\n");
    	goto irq_cleanup;
    }
    tdev->region_user.vaddr = (unsigned int) ioremap_nocache(pci_resource_start(pdev, BAR0), tdev->region_user.len);
    if(!tdev->region_user.vaddr)
    {
        printk(KERN_INFO "PCIe: Failed to get virtual address of data area\n");
        rc = -EIO;
        goto mem_region_cleanup;
    }

    printk (KERN_INFO "PCIe: Address at probe = %u\n", tdev->region_user.vaddr);

    // Setting the admin area pointer which will be at the offset of 896 kb
    // from the user area base pointer.

    tdev->region_admin.vaddr = tdev->region_user.vaddr + 0x3e0000;
    tdev->region_admin.len = (128*1024);

    if (!(pci_resource_flags(pdev, BAR4) & IORESOURCE_MEM))
    {
        printk(KERN_INFO "PCIe: region #4 is not an MMIO resource, aborting now\n");
        rc = -ENODEV;
        goto unmap_1mb;
    }

    struct_10MB.len= pci_resource_len(pdev, BAR4);
    if (struct_10MB.len!= DATA_AREA_LEN_10MB)
    {
        printk(KERN_INFO "PCIe: 10 MB area length is not correct\n");
        rc = -ENODEV;
        goto unmap_1mb;
    }

    struct_10MB.paddr = pci_resource_start(pdev, BAR4);
    struct_10MB.vaddr = (unsigned int) ioremap_nocache(pci_resource_start(pdev, BAR4),DATA_AREA_LEN_10MB);

    if(!struct_10MB.vaddr)
    {
        printk(KERN_INFO "PCIe: Failed to get virtual address of 10MB area\n");
        rc = -EIO;
        goto unmap_1mb;
    }

    tdev->cdev = kzalloc(sizeof(struct cdev), GFP_KERNEL);

    if(!tdev->cdev)
    {
        printk(KERN_INFO "PCIe: Failed to allocate cdev\n");
        rc = -ENOMEM;
        goto unmap_10mb;
    }

    /* Wipro : Mar10'2013 Dynamic Allocation of Major Nummber */
    rc = alloc_chrdev_region(&devid, 0, 1, "triton_ep");
    if (rc) {
       printk(KERN_ERR "Triton PCIEX : failed to allocate char device region\n");
       goto unmap_10mb;
    }

    major = MAJOR(devid);

    cdev_init(tdev->cdev, &triton_ep_fops);
    tdev->cdev->owner = THIS_MODULE;
    tdev->cdev->ops   = &triton_ep_fops;
    rc = cdev_add(tdev->cdev, MKDEV(major, TRITON_EP_MINOR_NUM), 1);

    if(rc < 0)
    {
        printk(KERN_INFO "PCIe: Failed to add triton dev\n");
        goto cdev_cleanup;
    }

    printk(KERN_INFO "PCIe: probe successful\n");

out:
    return rc;
cdev_cleanup:
    kfree(tdev->cdev);
unmap_10mb:
    iounmap((void *) struct_10MB.vaddr);
unmap_1mb:
    iounmap((void *) tdev->region_user.vaddr);
mem_region_cleanup:
    pci_release_regions(pdev);
irq_cleanup:
    free_irq(pdev->irq, tdev);
msi_cleanup:
    pci_disable_msi(pdev);
pdev_cleanup:
    pci_disable_device(pdev);
tdev_cleanup:
    kfree(tdev);
    goto out;
}


/**
 * @fn static void __devexit triton_ep_remove(struct pci_dev *pdev)
 * @brief
 * Remove function of the driver, which performs the clean uyp activities.
 */
static void __devexit triton_ep_remove(struct pci_dev *pdev)
{
    struct triton_ep_dev *tdev;
    tdev = g_tdev;
    free_irq(tdev->irq, tdev);

    cdev_del(tdev->cdev);
    kfree(tdev->cdev);
    iounmap((void *) struct_10MB.vaddr);
    iounmap((void *) tdev->region_user.vaddr);
    unregister_chrdev_region(devid,1);
    pci_release_regions(pdev);
#if 0
    free_irq(pdev->irq, tdev);
#endif
    pci_disable_msi(pdev);
    pci_disable_device(pdev);
    kfree(tdev);
    printk(KERN_INFO "remove successful\n");
}

static struct pci_device_id triton_ep_ids[] =
{
    {
     PCI_VENDOR_ID_TRITON,
	 PCI_DEVICE_ID_TRITON,
	 PCI_ANY_ID,
	 PCI_ANY_ID,
	 0,0,0
    },
    { 0, }
};

static struct pci_driver triton_ep_driver = {
    .name		= MODULE_NAME,
    .id_table	= triton_ep_ids,
    .probe		= triton_ep_probe,
    .remove	    = triton_ep_remove
};

static int __init triton_ep_init(void)
{
    return pci_register_driver(&triton_ep_driver);
}

static void __exit triton_ep_exit(void)
{
    pci_unregister_driver(&triton_ep_driver);
}

module_init(triton_ep_init);
module_exit(triton_ep_exit);
