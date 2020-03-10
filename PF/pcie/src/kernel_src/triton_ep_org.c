#include <linux/pci.h>
#include <linux/fs.h>			       /* file stuff */
#include <linux/kernel.h>		       /* printk() */
#include <linux/errno.h>		       /* error codes */
#include <linux/module.h>		       /* THIS_MODULE */
#include <linux/cdev.h>			/* char device stuff */
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
//#include <linux/wrapper.h>


#include <asm/uaccess.h>                 /* copy_to_user() */
#include <asm/system.h>

#include "device_file.h"
#include "triton_ep.h"
#include "pciex_common.h"
#include "pciex_manageshm.h"
#include "pciex_types.h"
#include "pciex.h"

#define TIMER 1

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Wipro Technologies");

struct triton_ep_block struct_10MB;

// Backup - start
#ifdef TIMER
struct timer_list atom_timer;
#define BLINK_DELAY HZ/5
#endif
// Backup - end


static struct triton_ep_dev *g_tdev = NULL;

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
   struct triton_ep_dev *priv = (struct triton_ep_dev *) dev_id;

   printk(KERN_INFO "MSI recieved..\n");
   /* code for notifying the pool */
   wake_up_interruptible(&priv->wait);
   return IRQ_HANDLED;
}

// Timer function. //Backup - start
#ifdef TIMER
static void atom_timer_func (unsigned int data)
{
    struct triton_ep_dev *tdev = NULL;
	unsigned int *admin = NULL;

	tdev = g_tdev;
	admin = (unsigned int *)(tdev->region_user.vaddr+ 0x3e0000);

	printk(KERN_INFO "Timer called.\n");
	// For temporary release.
	if (admin[3] == 0x00000001)
	{
		printk(KERN_INFO "Time to wake interuptible, Something is being written by Triton.\n");
		wake_up_interruptible(&tdev->wait);
		// Set the value of admin[1] as 0.
		admin[3] = 0x00000000;
	}
	init_timer(&atom_timer);
	atom_timer.function = atom_timer_func;
	atom_timer.data = (unsigned long)0;
	atom_timer.expires = jiffies + BLINK_DELAY;
	add_timer(&atom_timer);
}
#endif
//Backup - end

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
    unsigned int *msi = NULL;
    unsigned int *user2 = NULL;
    int i = 0;

    tdev = g_tdev;
    src = (char *) tdev->region_user.vaddr;

    if (copy_to_user(buf, src, count))
    {
	printk(KERN_ERR "copy_to_user failed.\n");
	return -EFAULT;
    }
    msi = (unsigned int *)(tdev->region_user.vaddr+ 0x3e0000);
    user2 = (unsigned int *)(struct_10MB.vaddr);
    //user2 = (unsigned int *)(tdev->region_user.vaddr+ 0x100000);

    for (i = 0 ; i < 3 ; i++)
    {
    	printk("User2[%d] 0x%08X\n", i, user2[i]);
    }
    printk("Admin[0] 0x%08X\n", msi[0]);
    printk("Admin[1] 0x%08X\n", msi[1]);
    printk("Admin[2] 0x%08X\n", msi[2]);
    //printk("Admin[3] 0x%08X\n", msi[3]);
    printk(KERN_INFO "read successful\n");
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

#if 1
    struct triton_ep_dev *tdev = NULL;
    char *dst;
    unsigned int *admin = NULL;
    unsigned int *user2 = NULL;
    //unsigned int data1 = 0x00000001;
    //int i = 0;

    tdev = g_tdev;
    dst = (char *) tdev->region_user.vaddr;

    if (copy_from_user(dst, buf, count))
    {
        printk(KERN_ERR "copy_from_user failed.\n");
        return -EFAULT;
    }

    //msi = (unsigned int *) (tdev->region_admin.vaddr);
    admin = (unsigned int *)(tdev->region_user.vaddr+ 0x3e0000);
    user2 = (unsigned int *)(struct_10MB.vaddr);
    /*for (i = 0 ; i < 20 ; i++)
    {
    	msi [i] = data1;
    	data1 = data1 + 1;
    }*/
    admin[0] = 0x00000001;
    admin[2] = count;

    user2[1] = 0xdeadbeef;
    /*msi[1] = 0x00000002;
    msi[2] = 0x00000003;
    msi[3] = 0x00000004;*/

    // For temporary release
    /*while (1)
    {
    	if (admin[1] == 0x00000002)
    	{
    	    printk(KERN_INFO "Response recieved from Triton for Atom`s write.\n");
    	    // Set the value of admin[1] as 0.
    	    admin[1] = 0x00000000;
    	    break;
    	}
    }*/

    printk(KERN_INFO "write successful\n");
    return count;
#endif

#if 0
    struct triton_ep_read_write pshmwrite;
    long rc = 0;
    void *dst;
    unsigned int *msi = NULL;
    struct triton_ep_dev *ptdev = NULL;
    ptdev = (struct triton_ep_dev *)filep->private_data;
//    msi = (unsigned int *) ptdev->region_admin.vaddr;

//    msi[0] = 0x00000001;

    ptdev->dev_status = PCIEX_STS_NORMAL;

    printk(KERN_INFO "Within write\n");
    pshmwrite.size = count;
    pshmwrite.start = 0;
    pshmwrite.buf = buf;

    printk(KERN_INFO "before writedata\n");
    rc = pciex_log_writeData (&pshmwrite, ptdev);
    printk(KERN_INFO "after writedata\n");
    if (0 != rc)
    {
    	printk(KERN_ERR "Write failed\n");
    	return -EFAULT;
    }
    else
    {
    	printk(KERN_ERR "Write successful\n");
    	return count;
    }
#endif
}

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
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside GET_SHM_INFO" );
            rc = triton_ep_ioctl_shm_info (ptdev, arg);
            break;
        case SHM_CREATE:
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_CREATE" );
            rc = triton_ep_ioctl_shm_create (ptdev, arg);
	     break;
        case SHM_RELEASE:
	    printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_RELEASE" );
           rc = triton_ep_ioctl_shm_release (ptdev, arg);
	    break;
        case SHM_ATTACH:
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_ATTACH" );
            rc = triton_ep_ioctl_shm_attach (ptdev, arg);
            break;
        case SHM_DETACH:
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_DETACH" );
            rc = triton_ep_ioctl_shm_detach (ptdev, arg);
            break;
        case SHM_LOCK:
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_LOCK" );
            rc = triton_ep_ioctl_shm_lock (ptdev, arg);
            break;
        case SHM_UNLOCK:
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_UNLOCK" );
            rc = triton_ep_ioctl_shm_unlock (ptdev, arg);
            break;
        case SHM_READ:
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_READ" );
            rc = triton_ep_ioctl_shm_read (ptdev, arg);
            break;
        case SHM_WRITE:
            printk( KERN_NOTICE "PCIEX-DRIVER: Inside SHM_WRITE" );
            rc = triton_ep_ioctl_shm_write (ptdev, arg);
            break;
        default:
            printk( KERN_NOTICE "PCIEX-DRIVER: Not supported command" );
            break;

        return rc;
    }

    printk(KERN_INFO "ioctl successful\n");
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
    unsigned int irq;
    struct triton_ep_dev *tdev = NULL;

    if (NULL == inode || NULL == filep)
    {
    	printk (KERN_ERR "NULL pointer in open\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "open successful\n");
    tdev = g_tdev;
    irq = tdev->pdev->irq;

#if 0
    if((ret = request_irq(irq, triton_ep_interrupt, 0, MODULE_NAME, tdev)) < 0)
    {
        printk(KERN_NOTICE "Problem acquiring the IRQ, request_irq returned %d\n", ret);
    }
#endif

    /* store tdev for use in read, write methods etc */
    filep->private_data = (void*) g_tdev;

    //Init the poll head.
    init_waitqueue_head(&tdev->wait);

    // Lets do the timer work. Backup - start
#ifdef TIMER
    init_timer(&atom_timer);
    atom_timer.function = atom_timer_func;
    atom_timer.data = (unsigned long)0;
    atom_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&atom_timer);
    // Backup - end
#endif

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
    struct triton_ep_dev *priv = NULL;
    unsigned int irq;

    if (NULL == inode || NULL == filep)
    {
    	 printk (KERN_ERR "NULL pointer in release\n");
        return -ENOMEM;
    }

    priv = (struct triton_ep_dev *) filep->private_data;
    irq = priv->pdev->irq;

    printk(KERN_INFO "release successful\n");

#if 0
    /* release IRQ */
    free_irq (irq, priv);
#endif

    // Backup - Start
  #ifdef TIMER
      del_timer(&atom_timer);
  #endif
      // Backup - End
    return 0;
}

static void triton_vma_open(struct vm_area_struct *vma)
{
	printk(KERN_INFO "vma_open start\n");
	struct triton_ep_dev *tdev = vma->vm_private_data;
	tdev->vma_count++;
	printk(KERN_INFO "vma_open end\n");
}

static void triton_vma_close(struct vm_area_struct *vma)
{
	printk(KERN_INFO "vma_close start\n");
	struct triton_ep_dev *tdev = vma->vm_private_data;
	tdev->vma_count--;
	printk(KERN_INFO "vma_close end\n");
}

static int triton_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	//struct triton_ep_dev *tdev = vma->vm_private_data;
	struct page *page;
	//unsigned long offset;
	printk(KERN_INFO "vma_fault start\n");
	page = virt_to_page(struct_10MB.vaddr);
	get_page(page);
	vmf->page = page;
	printk(KERN_INFO "vma_fault end\n");
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
	/*int mi = uio_find_mem_index(vma);
	if (mi < 0)
		return -EINVAL;*/
	printk(KERN_INFO "mmap_physical start\n");

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
	printk(KERN_INFO "mmap_logical start\n");
	vma->vm_flags |= VM_RESERVED;
	vma->vm_ops = &triton_vm_ops;
	triton_vma_open(vma);
	printk(KERN_INFO "mmap_logical end\n");
	return 0;
}


// Not required.
static int triton_ep_mmap(struct file *filep, struct vm_area_struct *vma)
{
	struct triton_ep_dev *tdev = filep->private_data;
	unsigned long requested_pages, actual_pages;

	printk(KERN_INFO "triton_mmap start\n");

	if (vma->vm_end < vma->vm_start)
	{
		printk(KERN_INFO "triton_mmap -1\n");
		return -EINVAL;
	}

	vma->vm_private_data = tdev;

	/*mi = uio_find_mem_index(vma);
	if (mi < 0)
		return -EINVAL;*/

	requested_pages = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
	actual_pages = ((struct_10MB.vaddr & ~PAGE_MASK)
			+ struct_10MB.len + PAGE_SIZE -1) >> PAGE_SHIFT;
	if (requested_pages > actual_pages)
	{
		printk(KERN_INFO "triton_mmap -2\n");
		return -EINVAL;
	}

	/*if (idev->info->mmap) {
		ret = idev->info->mmap(idev->info, vma);
		return ret;
	}*/

	/*switch (idev->info->mem[mi].memtype) {
		case UIO_MEM_PHYS:
			return uio_mmap_physical(vma);
		case UIO_MEM_LOGICAL:
		case UIO_MEM_VIRTUAL:
			return uio_mmap_logical(vma);
		default:
			return -EINVAL;
	}*/
	printk(KERN_INFO "triton_mmap end\n");
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

    /*if (NULL == wait || NULL == filep)
    {
    	printk (KERN_ERR "NULL pointer in poll\n");
        return -ENOMEM;
    }*/

    printk(KERN_INFO "within poll\n");

    priv = (struct triton_ep_dev *) filep->private_data;

    if(!priv->pdev->irq)
       return -EIO;

    poll_wait(filep, &priv->wait, wait);

    //if(condition to check device is readable)
    mask |= POLLIN | POLLRDNORM;
    /*if(condition to check device is writable)
     mask |= POLLOUT | POLLWRNORM;*/
    printk(KERN_INFO "poll successful\n");
    return mask;
}


/* File Operation structure - Contains all the device driver
                                          entry points.
*/
static struct file_operations triton_ep_fops =
{
    .owner             = THIS_MODULE,
    .read               = triton_ep_read,
    .write               = triton_ep_write,
    .unlocked_ioctl  = triton_ep_ioctl,
    .open               = triton_ep_open,
    .release            = triton_ep_release,
    .mmap             = triton_ep_mmap,
    .poll	             = triton_ep_poll
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
    int rc, devno;
    struct triton_ep_dev *tdev = NULL;
    //uint32_t reg32 = 0;

    if (NULL == pdev || NULL == entry)
    {
    	printk (KERN_ERR "NULL pointer in probe\n");
        return -ENOMEM;
    }

    devno = MKDEV(TRITON_EP_MAJOR_NUM, TRITON_EP_MINOR_NUM);

    tdev = kzalloc(sizeof(struct triton_ep_dev), GFP_KERNEL);
    if(!tdev)
    {
        printk(KERN_ERR "unable to allocate triton ep dev \n");
        return -ENOMEM;
    }

    /* Stroring the struct pdev */
    tdev->pdev = pdev;

    g_tdev = tdev;

    rc = pci_enable_device(pdev);
    if (rc < 0)
    {
        printk(KERN_ERR "failed to enable PCI device\n");
        goto tdev_cleanup;
    }

    if (!(pci_resource_flags(pdev, BAR0) & IORESOURCE_MEM))
    {
        printk(KERN_ERR "region #0 is not an MMIO resource, aborting\n");
        rc = -ENOMEM;
        goto pdev_cleanup;
    }

    if ((rc = pci_enable_msi(pdev)<0))
    {
        printk(KERN_ERR "pci_enable_msi failed with the error code %d\n", rc);
        goto pdev_cleanup;
    }
    /* At this point pdev->irq have the correct irq info */

    //temp
    if((rc = request_irq(pdev->irq, triton_ep_interrupt, 0, MODULE_NAME, tdev)) < 0)
    {
       printk(KERN_NOTICE "Problem acquiring the IRQ, request_irq returned %d\n", rc);
       goto msi_cleanup;
    }

    tdev->region_user.len = pci_resource_len(pdev, BAR0);
    if (tdev->region_user.len != DATA_AREA_LEN_1MB)
    {
        printk(KERN_ERR "data area length is not correct\n");
        rc = -ENODEV;
        goto irq_cleanup;
    }

    rc = pci_request_regions(pdev, MODULE_NAME);
    if (rc < 0)
    {
    	printk(KERN_ERR "failed to request regions\n");
    	goto irq_cleanup;
    }
    tdev->region_user.vaddr = (unsigned int) ioremap_nocache(pci_resource_start(pdev, BAR0), tdev->region_user.len);
    if(!tdev->region_user.vaddr)
    {
        printk(KERN_ERR "failed to get virtual address of data area\n");
        rc = -EIO;
        goto mem_region_cleanup;
    }

    printk (KERN_INFO "Address at probe = %u\n", tdev->region_user.vaddr);

    // Setting the admin area pointer which will be at the offset of 896 kb
    // from the user area base pointer.

    tdev->region_admin.vaddr = tdev->region_user.vaddr + 0x3e0000;
    tdev->region_admin.len = (128*1024);

    if (!(pci_resource_flags(pdev, BAR4) & IORESOURCE_MEM))
    {
        printk(KERN_ERR "region #4 is not an MMIO resource, aborting now\n");
        rc = -ENODEV;
        goto unmap_1mb;
    }

    struct_10MB.len= pci_resource_len(pdev, BAR4);
    if (struct_10MB.len!= DATA_AREA_LEN_10MB)
    {
        printk(KERN_ERR "10 MB area length is not correct\n");
        rc = -ENODEV;
        goto unmap_1mb;
    }

    struct_10MB.paddr = pci_resource_start(pdev, BAR4);
    struct_10MB.vaddr = (unsigned int) ioremap_nocache(pci_resource_start(pdev, BAR4),DATA_AREA_LEN_10MB);

    if(!struct_10MB.vaddr)
    {
        printk(KERN_ERR "failed to get virtual address of 10MB area\n");
        rc = -EIO;
        goto unmap_1mb;
    }

    tdev->cdev = kzalloc(sizeof(struct cdev), GFP_KERNEL);

    if(!tdev->cdev)
    {
        printk(KERN_ERR "failed to allocate cdev\n");
        rc = -ENOMEM;
        goto unmap_10mb;
    }

    cdev_init(tdev->cdev, &triton_ep_fops);
    tdev->cdev->owner = THIS_MODULE;
    tdev->cdev->ops   = &triton_ep_fops;
    rc = cdev_add(tdev->cdev, devno, 1);

    if(rc < 0)
    {
        printk(KERN_ERR "failed to add triton dev\n");
        goto cdev_cleanup;
    }

    printk(KERN_INFO "probe successful\n");

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

    //tdev = (struct triton_ep_dev *)pdev->private_data;

    cdev_del(tdev->cdev);
    kfree(tdev->cdev);
    iounmap((void *) struct_10MB.vaddr);
    iounmap((void *) tdev->region_user.vaddr);
    pci_release_regions(pdev);
    free_irq(pdev->irq, tdev);
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
    .remove	= triton_ep_remove
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

