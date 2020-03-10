#ifndef _TRITON_EP_H_
#define _TRITON_EP_H_

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

#include <asm/uaccess.h>
#include <asm/system.h>

#include "pciex_types.h"
#include "pciex_common.h"

#define MODULE_NAME                     "triton_ep"
#define TRITON_EP_MAJOR_NUM	 248
#define TRITON_EP_MINOR_NUM      0
#define DATA_AREA_LEN_1MB         1048576*4
#define ADMIN_AREA_LEN_128KB    131072
#define DATA_AREA_LEN_10MB      0x800000  //10485760


// definition for triton
#define PCI_VENDOR_ID_TRITON 	0x1033
#define PCI_DEVICE_ID_TRITON	0x0190

#define BAR0                      0
#define BAR1                      1
#define BAR2                      2
#define BAR3                      3
#define BAR4                      4



#define GET_SHM_INFO     _IOR(TRITON_EP_MAJOR_NUM, 1, int)
#define SHM_CREATE	       _IOR(TRITON_EP_MAJOR_NUM, 2, int)
#define SHM_RELEASE	       _IOR(TRITON_EP_MAJOR_NUM, 3, int)
#define SHM_ATTACH	       _IOR(TRITON_EP_MAJOR_NUM, 4, int)
#define SHM_DETACH	       _IOR(TRITON_EP_MAJOR_NUM, 5, int)
#define SHM_LOCK	       _IOR(TRITON_EP_MAJOR_NUM, 6, int)
#define SHM_UNLOCK	       _IOR(TRITON_EP_MAJOR_NUM, 7, int)
#define SHM_READ	       _IOR(TRITON_EP_MAJOR_NUM, 8, int)
#define SHM_WRITE	       _IOR(TRITON_EP_MAJOR_NUM, 9, int)
#define GET_SHM_PTR		_IOR(TRITON_EP_MAJOR_NUM, 10, int)


struct triton_ep_read_write {
    unsigned char *buf;
    int start;
    int size;
    int asize;
};

struct triton_ep_block {
    unsigned int    paddr;
    unsigned int    vaddr;
    unsigned int    len;
    int                  sem_id;
};

struct triton_ep_dev {
    /* ref. to pci device struct */
    struct pci_dev                *pdev;
    /* character device struct */
    struct cdev                    *cdev;
    /* address of admin area */
    struct triton_ep_block     region_admin;
    /* current user area */
    struct triton_ep_block     region_user;
    /*sync params*/
        /*wait flag*/
        /*time out value*/
    /*completion*/
    struct completion            complete;
    /*array of user info??*/
    /*device status - ready, busy*/
    PCIEX_STS                     dev_status;
    wait_queue_head_t         wait;
    int                      vma_count;
    /* Rest to go */
};


#endif //_TRITON_EP_H_

