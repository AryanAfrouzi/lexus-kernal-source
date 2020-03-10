/**
 * @file pciex_manageshm.c
 * @brief Functions relates to the main task.
 * @version $Name: PCIEX_r2_C01 $ (revision $Revision: 1.7 $)
 * @author $Author: madachi $
 * @par copyright
 * (c) 2009 ADIT Corporation
 */

// [kd] - Include Linux specific headers.
#include <linux/pci.h>
#include <linux/fs.h>			/* file stuff */
#include <linux/kernel.h>		/* printk() */
#include <linux/errno.h>		/* error codes */
#include <linux/module.h>		/* THIS_MODULE */
#include <linux/cdev.h>			/* char device stuff */
#include <asm/uaccess.h>		/* copy_to_user() */
#include <linux/mod_devicetable.h>
#include <linux/types.h>
#include <linux/poll.h>
#include <linux/unistd.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/slab.h>

#include "pciex_common.h"
#include "pciex.h"
#include "pciex_manageshm.h"
#include "pciex_trace.h"
#include "pciex_types.h"
#include "triton_ep.h"


/***********************************************************************
 *   definitions
 ***********************************************************************/
#ifdef HAS_TRACE
/*--------------------------------------------------------------------------*/
/* set Trace Data CLASS-define (needed if HAS_TRACE is set)                 */
/*--------------------------------------------------------------------------*/
//[kd] - #define UTIL_DRV_UHCLASS   (TR_COMP_DRV_INTERFACE + PCIEX_FILENAME_MANAGESHM)
#endif /* HAS_TRACE */

/***********************************************************************
 *   constants
 ***********************************************************************/

/***********************************************************************
 *  structures
 ***********************************************************************/

/***********************************************************************
 *  functions
 ***********************************************************************/

/***********************************************************************
 *  global variables
 ***********************************************************************/

ER pciex_memcpy( VP dst, VP src, UINT size )
{
    ER err;
    UW dst_align;
    UW src_align;
    UINT i;
    UINT copy_size;

    /* for fear of exception occurrence, avoid using memcpy if address is not aligned */
    if ( ( dst == NULL )
      || ( src == NULL )
      || ( size == 0   ) )
    {
        err = -1;
        TR_ERROR( PCIEX_FUNCNAME_MEMCPY, 0x00, err );
    }
    else
    {
        dst_align = (UW)dst & ( sizeof(INT) - 1 );
        src_align = (UW)src & ( sizeof(INT) - 1 );

        if ( ( dst_align == 0 )
          && ( src_align == 0 ) )
        {
            /* both address are aligned. */
            memcpy( dst, src, size );
        }
        else
        {
            if ( dst_align == src_align )
            {
                copy_size = sizeof(INT) - dst_align;
                for ( i = 0; i < copy_size; i++ )
                {
                    /* copy data 1 by 1 byte. */
                    *( (UB *)((UW)dst + i) ) = *( (UB *)((UW)src + i) );
                }
                /* both address are aligned. */
                memcpy( (VP)((UW)dst + copy_size), (VP)((UW)src + copy_size), size - copy_size );
            }
            else
            {
                /* copy data 1 by 1 byte. */
                for ( i = 0; i < size; i++ )
                {
                    *( (UB *)((UW)dst + i) ) = *( (UB *)((UW)src + i) );
                }
            }
        }
        err = 0;
    }
    return err;
}

/**
 * @fn pciex_issue_msi( UW* p_msi, UH msgdata )
 * @brief
 * This function issues MSI to peer.
 * @param[in] p_msi Pointer to msi region
 * @param[in] msgdata Message data
 * @return void
 */
void pciex_issue_msi(UW* p_msi, UH msgdata)
{
    p_msi[ msgdata - 1 ] = (UW)msgdata;
}

/**
 * @fn ER pciex_log_readData (struct triton_ep_read_write *p_devReq, struct triton_ep_dev *ptdev)
 * @brief
 * This function handles read request to logical device.
 * @param[in] p_devReq Device request.
 * @param[in] ptdev Unit information.
 * @return 0    Completed successfully.
 * @return EFAULT   Error.
 * @return ENOEXEC  error.
  * @return others Internal error.
 */
ER pciex_log_readData (struct triton_ep_read_write *p_devReq, struct triton_ep_dev *ptdev)
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    PCIEX_SHM *p_shmb = NULL;
    PCIEX_SHM_ACCESSINFO ainfo;    /* initialization as follows */
    //INT unit_num = 0;
    ER err = -ENOEXEC;
    //ER err = E_NOSPT;
    UB subunitnum = 0;

    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *)ptdev->region_admin.vaddr;

    if (ptdev->dev_status!= PCIEX_STS_NORMAL )
    {
        /* Driver's not ready */
        TR_ERROR( PCIEX_FUNCNAME_LOG_READDATA, 0x00, ptdev->dev_status);
	 return -EFAULT;
        //return E_SYS; /* ======== leave function ======== */
    }

    /* arrangement clear */
    memset( (VP)&ainfo, 0, sizeof(PCIEX_SHM_ACCESSINFO) );

    /* [kd] - need to understand */
    //subunitnum = (UB)( ( ( p_devReq->devid ) - GDI_devid( p_drvinfo->individual[unit_num].gdi ) ) - 1 );

    p_shmb = &p_admin->shmadmin.shm[subunitnum];
    if ( p_shmb->is_allocated == FALSE )
    {
        /* shared memory block not created */
        TR_ERROR( PCIEX_FUNCNAME_LOG_READDATA, 0x01, subunitnum );
	 return -EFAULT;
        //return E_NOEXS; /* ======== leave function ======== */
    }
    if ( p_devReq->start >= 0 )  /* device-specific data  */
    {
        if ( p_devReq->size == 0 )
        {
            /* return shared memory block size */
	     err = 0;
            //err = E_OK;
            p_devReq->asize = (INT)p_shmb->size;
        }
        else
        {
            /* access offset & size check */
            if ( ( p_devReq->start < (INT)p_shmb->size )
              && ( ( p_devReq->start + p_devReq->size ) <= (INT)p_shmb->size ) )
            {
                ainfo.subunitnum = subunitnum;
                ainfo.offset = (UW)p_devReq->start;
                ainfo.p_buffer = p_devReq->buf;
                ainfo.data_size = (UW)p_devReq->size;
                err = pciex_read_shm (ptdev, &ainfo );
                p_devReq->asize = p_devReq->size;
            }
            else
            {
                //[kd] -TR_ERROR( PCIEX_FUNCNAME_LOG_READDATA, 0x02, E_PAR );
		  TR_ERROR( PCIEX_FUNCNAME_LOG_READDATA, 0x02, EINVAL );
		  return -EINVAL;
                //return E_PAR; /* ======== leave function ======== */
            }
        }
    }
    else
    {
        TR_ERROR( PCIEX_FUNCNAME_LOG_READDATA, 0x03, p_devReq->start );
	 return -EINVAL;
        //return E_PAR; /* ======== leave function ======== */
    }

    return err;
}

/**
 * @fn ER pciex_log_writeData (struct triton_ep_read_write *p_devReq, struct triton_ep_dev *ptdev)
 * @brief
 * This function handles write request to logical device.
 * @param[in] p_devReq Device request.
 * @param[out] ptdev Unit information.
 * @return 0    Completed successfully.
 * @return EFAULT   Error.
 * @return ENOEXEC   Error.
  * @return others Internal error.
 */
ER pciex_log_writeData (struct triton_ep_read_write *p_devReq, struct triton_ep_dev *ptdev)
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_SHM *p_shmb = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    PCIEX_SHM_ACCESSINFO ainfo;    /* initialization as follows */
    //INT unit_num = 0;
    ER err = -ENOEXEC;
    // [kd ]ER err = E_NOSPT;
    UB subunitnum = 0;

    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;

    printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);
    p_admin = (PCIEX_ADMIN_REGION *)ptdev->region_admin.vaddr;

    if (ptdev->dev_status!= PCIEX_STS_NORMAL )
    {
        /* Driver's not ready */
	//printk ("Driver not ready - %d\n", p_drvinfo->individual[unit_num].drvsts);
	return -EFAULT;
        TR_ERROR( PCIEX_FUNCNAME_LOG_WRITEDATA, 0x00, ptdev->dev_status);
        //return E_SYS; /* ======== leave function ======== */
    }

    /* arrangement clear */
    memset( (VP)&ainfo, 0, sizeof(PCIEX_SHM_ACCESSINFO) );

   /* [kd] - Need to understand this. */
    //subunitnum = (UB)( ( ( p_devReq->devid ) - GDI_devid( p_drvinfo->individual[unit_num].gdi ) ) - 1 );

    printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);
    p_shmb = &p_admin->shmadmin.shm[subunitnum];

#if 0
    if ( p_shmb->is_allocated == FALSE )
    {
        /* shared memory block not created */
	 //printk("Shared memory block not created\n");
        TR_ERROR( PCIEX_FUNCNAME_LOG_WRITEDATA, 0x01, subunitnum );
        //return E_NOEXS; /* ======== leave function ======== */
        return -EFAULT;
    }
#endif
    printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);
    if ( p_devReq->start >= 0 )  /* device-specific data  */
    {
        if ( p_devReq->size == 0 )
        {
            err = 0;
	     // err = E_OK;
            p_devReq->asize = (INT)p_shmb->size;
        }
        else
        {
            /* access offset & size check */
            /*if ( ( p_devReq->start < (INT)p_shmb->size )
              && ( ( p_devReq->start + p_devReq->size ) <= (INT)p_shmb->size ) )
            {*/
                ainfo.subunitnum = subunitnum;
                ainfo.offset = (UW)p_devReq->start;
                ainfo.p_buffer = p_devReq->buf;
                ainfo.data_size = (UW)p_devReq->size;
                printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);
                err = pciex_write_shm (ptdev, &ainfo );
                printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);
                p_devReq->asize = p_devReq->size;
            /*}
            else
            {
                //printk("Size check failed and cant access offset\n");
		  return -EINVAL;
                TR_ERROR( PCIEX_FUNCNAME_LOG_WRITEDATA, 0x02, EINVAL );
                //return E_PAR; //======== leave function ========
            }*/
        }
    }
    else
    {
        //printk("No device specific data\n");
	 return -EINVAL;
        TR_ERROR( PCIEX_FUNCNAME_LOG_WRITEDATA, 0x03, p_devReq->start );
        //return E_PAR; /* ======== leave function ======== */
    }

    printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);
    return err;
}


/**
 * @fn ER pciex_get_shm_info_peer(struct triton_ep_dev *ptdev, PCIEX_SHM_INFO *p_req )
 * @brief
 * Gets shared memory information in opposite side.
 * @param[in] ptdev Unit information.
 * @param[out] p_req pointer to buffer to store shared memory information.
 * @return 0    Completed successfully.
 * @return EINVAL   Parameter error.
 * @return others Internal error.
 */
ER pciex_get_shm_info_peer(struct triton_ep_dev *ptdev, PCIEX_SHM_INFO *p_req )
{
    PCIEX_ADMIN_REGION *p_admin = NULL;
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    //INT unit_num = 0;
    //[kd] - ER err = E_OK;
    ER err = 0;

    if ( NULL == p_req || NULL == ptdev)
    {
        //[kd] - TR_ERROR( PCIEX_FUNCNAME_GET_SHM_INFO_PEER, 0x00, E_PAR );
        //return E_PAR; /* ======== leave function ======== */
        TR_ERROR( PCIEX_FUNCNAME_GET_SHM_INFO_PEER, 0x00, EINVAL );
        return -EINVAL;
    }

    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *)ptdev->region_admin.vaddr;

    /* set message packet -> admin region ( memory transaction ) */
    p_admin->msgpkt.msgtype = PESHM_MSG_GET_SHM_INFO;
    p_admin->msgpkt.msgdata.getshm_info.getshm_info_req = *p_req;
    //p_admin->msgpkt.msgdata.getshm_info.ret = E_OK;
    p_admin->msgpkt.msgdata.getshm_info.ret = 0;

    err = pciex_issue_req (ptdev, 0, 0);
    if (err < 0)  //if ( err < E_OK )
    {
        TR_ERROR( PCIEX_FUNCNAME_GET_SHM_INFO_PEER, 0x01, err );
        return err; /* ======== leave function ======== */
    }

    /* add base address of user memory on self-side to returned offset */
    p_admin->msgpkt.msgdata.getshm_info.getshm_info_req.address += ptdev->region_user.vaddr;
    /* set result */
    *p_req = p_admin->msgpkt.msgdata.getshm_info.getshm_info_req;

    return p_admin->msgpkt.msgdata.getshm_info.ret;
}


/**
 * @fn ER pciex_create_shm_peer(struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_req )
 * @brief
 * Creates memory block in peer side.
 * @param[out] ptdev Unit information.
 * @param[out] p_req pointer to buffer to request structure.
 * @return 0    Completed successfully.
 * @return EINVAL   Parameter error.
 * @return others Internal error.
 */
ER pciex_create_shm_peer(struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_req )
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    //INT unit_num = 0;
    ER err = 0;
    //[kd] - ER err = E_OK;

   if ( NULL == p_req || NULL == ptdev)
    {
        TR_ERROR( PCIEX_FUNCNAME_CREATE_SHM_PEER, 0x00, EINVAL );
        return -EINVAL;
    }

    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *) ptdev->region_admin.vaddr;

    /* set message packet -> admin region ( memory transaction ) */
    p_admin->msgpkt.msgtype = PESHM_MSG_CREATE;
    p_admin->msgpkt.msgdata.create.shm_req = *p_req;
    p_admin->msgpkt.msgdata.create.ret = 0;
    //[kd] - p_admin->msgpkt.msgdata.create.ret = E_OK;

    // As of now passing 0 as the 2nd and 3rd parameter of pciex_msi_req. Change required.
    err = pciex_issue_req (ptdev, 0, 0);
    if (err < 0)  //if ( err < E_OK )
    {
        TR_ERROR( PCIEX_FUNCNAME_CREATE_SHM_PEER, 0x01, err );
        return err; /* ======== leave function ======== */
    }

    /* add base address of user memory on self-side to returned offset */
    p_admin->msgpkt.msgdata.create.shm_req.address += ptdev->region_user.vaddr;

    /* set result */
    *p_req = p_admin->msgpkt.msgdata.create.shm_req;

    /* convert device name */
    //pciex_replace_devname( p_req->device_name );

    return p_admin->msgpkt.msgdata.create.ret;
}


/**
 * @fn ER pciex_release_shm_peer(struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_req )
 * @brief
 * Release memory block in opposite side.
 * @param[out] ptdev Unit information.
 * @param[out] p_req pointer to buffer to request structure.
 * @return 0     Completed successfully.
 * @return EINVAL    Error.
 * @return others Internal error.
 */
ER pciex_release_shm_peer(struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_req )
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    //INT unit_num = 0;
    ER err = 0;
    //[kd] - ER err = E_OK;

    if ( NULL == p_req || NULL == ptdev)
    {
        TR_ERROR( PCIEX_FUNCNAME_RELEASE_SHM_PEER, 0x00, EINVAL );
        return -EINVAL;
    }

    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *) ptdev->region_admin.vaddr;

    /* set message packet -> admin region ( memory transaction ) */
    p_admin->msgpkt.msgtype = PESHM_MSG_RELEASE;
    p_admin->msgpkt.msgdata.release.shm_req = *p_req;
    //[kd] - p_admin->msgpkt.msgdata.release.ret = E_OK;
    p_admin->msgpkt.msgdata.release.ret = 0;

    err = pciex_issue_req( ptdev, 0, 0);
    if (err < 0)  //if ( err < E_OK )
    {
        TR_ERROR( PCIEX_FUNCNAME_RELEASE_SHM_PEER, 0x01, err );
        return err; /* ======== leave function ======== */
    }

    /* set result */
    *p_req = p_admin->msgpkt.msgdata.release.shm_req;

    return p_admin->msgpkt.msgdata.release.ret;
}


/**
 * @fn ER pciex_attach_shm_peer(struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_req )
 * @brief
 * Attach memory block in peer side.
 * @param[out] ptdev Unit information.
 * @param[out] p_req pointer to buffer to request structure.
 * @return 0     Completed successfully.
 * @return EINVAL    Error.
 * @return others Internal error.
 */
ER pciex_attach_shm_peer(struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_req )
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    //INT unit_num = 0;
    //[kd] - ER err = E_OK;
    ER err = 0;

    if ( NULL == p_req || NULL == ptdev)
    {
        TR_ERROR( PCIEX_FUNCNAME_ATTACH_SHM_PEER, 0x00, EINVAL );
        return -EINVAL;
    }

    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *) ptdev->region_admin.vaddr;

    /* set message packet -> admin region ( memory transaction ) */
    p_admin->msgpkt.msgtype = PESHM_MSG_ATTACH;
    p_admin->msgpkt.msgdata.attach.shm_req = *p_req;
    //[kd] - p_admin->msgpkt.msgdata.attach.ret = E_OK;
    p_admin->msgpkt.msgdata.attach.ret = 0;

    err = pciex_issue_req(ptdev, 0,0 );
    if (err < 0)  //if ( err < E_OK )
    {
        TR_ERROR( PCIEX_FUNCNAME_ATTACH_SHM_PEER, 0x01, err );
        return err; /* ======== leave function ======== */
    }

    /* add base address of user memory on self-side to returned offset */
    p_admin->msgpkt.msgdata.create.shm_req.address += ptdev->region_user.vaddr;
    /* convert device name */
    //pciex_replace_devname( p_admin->msgpkt.msgdata.attach.shm_req.device_name );

    /* set result */
    *p_req = p_admin->msgpkt.msgdata.attach.shm_req;

    return p_admin->msgpkt.msgdata.attach.ret;
}


/**
 * @fn ER pciex_detach_shm_peer(struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_req )
 * @brief
 * Detach memory block in peer side.
 * @param[out] ptdev Unit information.
 * @param[out] p_req pointer to buffer to request structure.
 * @return 0     Completed successfully.
 * @return EINVAL    Error.
 * @return others Internal error.
 */
ER pciex_detach_shm_peer(struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_req )
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    //INT unit_num = 0;
    //[kd] - ER err = E_OK;
    ER err = 0;

    if ( NULL == p_req || NULL == ptdev)
    {
        TR_ERROR( PCIEX_FUNCNAME_DETACH_SHM_PEER, 0x00, EINVAL );
        return -EINVAL;
    }

    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *) ptdev->region_admin.vaddr;

    /* set message packet -> admin region ( memory transaction ) */
    p_admin->msgpkt.msgtype = PESHM_MSG_DETACH;
    p_admin->msgpkt.msgdata.detach.shm_req = *p_req;
    //[kd] - p_admin->msgpkt.msgdata.detach.ret = E_OK;
    p_admin->msgpkt.msgdata.detach.ret = 0;

    err = pciex_issue_req(ptdev, 0, 0);
    if (err < 0)  //if ( err < E_OK )
    {
        TR_ERROR( PCIEX_FUNCNAME_DETACH_SHM_PEER, 0x01, err );
        return err; /* ======== leave function ======== */
    }

    /* set result */
    *p_req = p_admin->msgpkt.msgdata.detach.shm_req;

    return p_admin->msgpkt.msgdata.detach.ret;
}


/**
 * @fn ER pciex_lock_shm_peer(struct triton_ep_dev * ptdev, PCIEX_LOCK_REQ *p_req )
 * @brief
 * Lock memory block in peer side.
 * @param[out] ptdev Unit information.
 * @param[out] p_req pointer to buffer to request structure.
 * @return 0     Completed successfully.
 * @return EINVAL    Error.
 * @return others Internal error.
 */
ER pciex_lock_shm_peer(struct triton_ep_dev * ptdev, PCIEX_LOCK_REQ *p_req )
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    //INT unit_num = 0;
    //[kd] - ER err = E_OK;
    ER err = 0;

    if ( NULL == p_req || NULL == ptdev)
    {
        TR_ERROR( PCIEX_FUNCNAME_LOCK_SHM_PEER, 0x00, EINVAL );
        return -EINVAL;
    }

    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *)ptdev->region_admin.vaddr;

    /* set message packet -> admin region ( memory transaction ) */
    p_admin->msgpkt.msgtype = PESHM_MSG_LOCK;
    p_admin->msgpkt.msgdata.lock.lock_req = *p_req;
    //[kd] - p_admin->msgpkt.msgdata.lock.ret = E_OK;
    p_admin->msgpkt.msgdata.lock.ret = 0;

    err = pciex_issue_req(ptdev, 0, 0);
    if (err < 0)  //if ( err < E_OK )
    {
        TR_ERROR( PCIEX_FUNCNAME_LOCK_SHM_PEER, 0x01, err );
        return err; /* ======== leave function ======== */
    }

    return p_admin->msgpkt.msgdata.lock.ret;
}


/**
 * @fn ER pciex_unlock_shm_peer(struct triton_ep_dev * ptdev, PCIEX_LOCK_REQ *p_req )
 * @brief
 * Unlock memory block in peer side.
 * @param[out] ptdev Unit information.
 * @param[out] p_req pointer to buffer to request structure.
 * @return 0     Completed successfully.
 * @return EINVAL    Parameter error.
 * @return others Internal error.
 */
ER pciex_unlock_shm_peer(struct triton_ep_dev * ptdev, PCIEX_LOCK_REQ *p_req )
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    //INT unit_num = 0;
    //[kd] - ER err = E_OK;
    ER err = 0;

    if ( NULL == p_req || NULL == ptdev)
    {
        TR_ERROR( PCIEX_FUNCNAME_UNLOCK_SHM_PEER, 0x00, EINVAL );
        return -EINVAL;
    }

    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *)ptdev->region_admin.vaddr;

    /* set message packet -> admin region ( memory transaction ) */
    p_admin->msgpkt.msgtype = PESHM_MSG_UNLOCK;
    p_admin->msgpkt.msgdata.unlock.unlock_req = *p_req;
    //[kd] - p_admin->msgpkt.msgdata.unlock.ret = E_OK;
    p_admin->msgpkt.msgdata.unlock.ret = 0;

    err = pciex_issue_req(ptdev, 0, 0);
    if (err < 0)  //if ( err < E_OK )
    {
        TR_ERROR( PCIEX_FUNCNAME_UNLOCK_SHM_PEER, 0x01, err );
        return err; /* ======== leave function ======== */
    }

    return p_admin->msgpkt.msgdata.unlock.ret;
}


/**
 * @fn ER pciex_read_shm(struct triton_ep_dev *ptdev, PCIEX_SHM_ACCESSINFO *p_ainfo )
 * @brief
 * Reads shared memory block.
 * @param[in] ptdev Unit information.
 * @param[in] p_ainfo Access information.
 * @return 0     Completed successfully.
 * @return EINVAL    Error.
 */
ER pciex_read_shm(struct triton_ep_dev *ptdev, PCIEX_SHM_ACCESSINFO *p_ainfo )
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    PCIEX_SHM *p_shmb = NULL;
    //INT unit_num = 0;
    UW base_address = 0;
    PCIEX_ADDRESS_CONV_FLAG flag = PCIEX_CONV_NON;

    if ( p_ainfo->p_buffer == NULL )
    {
        //[kd] - TR_ERROR( PCIEX_FUNCNAME_READ_SHM, 0x00, E_PAR );
        //return E_PAR; /* ======== leave function ======== */
	 TR_ERROR( PCIEX_FUNCNAME_READ_SHM, 0x00, EINVAL );
        return -EINVAL;
    }
    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *)ptdev->region_admin.vaddr;

    p_shmb = &p_admin->shmadmin.shm[p_ainfo->subunitnum];

    /* convert address */
    //if ( unit_num == PCIEX_SELF_UNIT_NUM )
    //{
    //    base_address = p_shmb->base_address;
    //}
    //else
    //{
        base_address = ptdev->region_user.vaddr + ( p_shmb->base_address - p_admin->shmadmin.start_address );
        flag = PCIEX_CONV_SRC;
    //}
    pciex_read_memory (ptdev, (VP)( base_address + p_ainfo->offset ), (VP)p_ainfo->p_buffer, p_ainfo->data_size, flag );

    //[kd] - return E_OK;
    return 0;
}

/**
 * @fn ER pciex_write_shm (struct triton_ep_dev *ptdev, PCIEX_SHM_ACCESSINFO *p_ainfo)
 * @brief
 * Writes shared memory block.
 * @param[out] ptdev Unit information.
 * @param[in] p_ainfo Access information.
 * @return 0     Completed successfully.
 * @return EINVAL    Error.
 */
ER pciex_write_shm (struct triton_ep_dev *ptdev, PCIEX_SHM_ACCESSINFO *p_ainfo)
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    PCIEX_SHM *p_shmb = NULL;
    PCIEX_ADDRESS_CONV_FLAG flag = PCIEX_CONV_NON;
    //INT unit_num = 0;
    PCIEX_TARGET target = PCIEX_TARGET_SELF;
    UW base_address = 0;
    ER err = 0;
    //[kd] - ER err = E_OK;

    printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);

    if ( p_ainfo->p_buffer == NULL )
    {
        TR_ERROR( PCIEX_FUNCNAME_WRITE_SHM, 0x00, EINVAL);
	 return -EINVAL;
        //[kd] - TR_ERROR( PCIEX_FUNCNAME_WRITE_SHM, 0x00, E_PAR );
        // return E_PAR; /* ======== leave function ======== */
    }
    //p_drvinfo = p_unitinfo->p_drvinfo;
    //unit_num = p_unitinfo->unit_num;
    p_admin = (PCIEX_ADMIN_REGION *)ptdev->region_admin.vaddr;

    p_shmb = &p_admin->shmadmin.shm[p_ainfo->subunitnum];

    /* convert address */
    //if ( unit_num == PCIEX_SELF_UNIT_NUM )
    //{
    //    base_address = p_shmb->base_address;
    //    target = PCIEX_TARGET_SELF;
    //}
    //else
    //{

        printk(KERN_INFO "ptdev->region_user.vaddr = %u  p_shmb->base_address = %u, p_admin->shmadmin.start_addres = %u\n", ptdev->region_user.vaddr, p_shmb->base_address, p_admin->shmadmin.start_address);
        base_address = ptdev->region_user.vaddr + ( p_shmb->base_address - p_admin->shmadmin.start_address );
        target = PCIEX_TARGET_PEER;
        flag = PCIEX_CONV_DST;
    //}

        printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);
    pciex_write_memory (ptdev, (VP)( base_address + p_ainfo->offset ), (VP)p_ainfo->p_buffer, p_ainfo->data_size, flag );
    printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);

    /* issue renew event to control task on self side */
    /* [kd] - Event notification to self side is presently not supported.

    err = pciex_issue_shmrenew_event( p_unitinfo, unit_num, (UW)p_shmb->shm_id );
    if ( err < E_OK )
    {
        TR_ERROR( PCIEX_FUNCNAME_WRITE_SHM, 0x01, err );
        return err; // ======== leave function ========
    }
    */

    /* issue renew event to control task on peer */
    /* set message packet -> admin region ( memory transaction ) */
    p_admin = (PCIEX_ADMIN_REGION *) ptdev->region_admin.vaddr;
    p_admin->msgpkt.msgtype = PESHM_MSG_SHMRENEW;
    p_admin->msgpkt.msgdata.renew.st_shm_id.shm_id = p_shmb->shm_id;
    p_admin->msgpkt.msgdata.renew.st_shm_id.target = target;
    p_admin->msgpkt.msgdata.renew.ret = 0;

    printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);

    err = pciex_issue_req(ptdev, 0, 0);
    printk(KERN_INFO "%s %u --------------------\n", __FILE__, __LINE__);

    if ( err < 0 )
    {
        TR_ERROR( PCIEX_FUNCNAME_WRITE_SHM, 0x02, err );
        return err; /* ======== leave function ======== */
    }

    return 0;
}

/**
 * @fn ER pciex_read_memory (struct triton_ep_dev *ptdev, VP address, VP p_buffer, UW data_size, PCIEX_ADDRESS_CONV_FLAG flag )
 * @brief
 * Reads memory.
 * @param[in] ptdev Unit information.
 * @param[in] address Target memory address.
 * @param[out] p_buffer Pointer to user buffer.
 * @param[in] data_size Data size.
 * @param[in] flag This flag shows whether vsaddr and vdaddr need lockking.
 * @return 0     Completed successfully.
 */
ER pciex_read_memory (struct triton_ep_dev *ptdev, VP address, VP p_buffer, UW data_size, PCIEX_ADDRESS_CONV_FLAG flag )
{
#if 0 //[kd] -DMA is not required on Atom side
#ifdef PCIEX_ENABLE_DMA
    //ER err = E_OK;
    INT err = 0;
    PCIEX_DMA_TRANS_INFO trans_info;

    /* Optimization of transfer performance */
    if ( (INT)data_size > p_unitinfo->p_drvinfo->conf.piorw[PCIEX_PIO_DMA_READ] )
    {
#ifdef PCIEX_USE_REGISTER_MODE_DMA
        /* check if DMA is available */
        err = pciex_DMA_get_chnum( p_unitinfo );
        if ( err >= 0 )
        {
            /* DMA is available */
            trans_info.dmach = (INT)err;
            trans_info.src_addr = address;
            trans_info.dst_addr = p_buffer;
            trans_info.data_size = data_size;
            trans_info.flag = flag;
            trans_info.direction = PCIEX_READ;
            err = pciex_DMA_start( p_unitinfo, &trans_info );
            if (err < 0) //if ( err < E_OK )
            {
                TR_ERROR( PCIEX_FUNCNAME_READ_MEMORY, 0x00, err );
                return err;    /* ======== leave function ======== */
            }
            err = pciex_DMA_wait( p_unitinfo, &trans_info );
            if (err < 0) //if ( err < E_OK )
            {
                TR_ERROR( PCIEX_FUNCNAME_READ_MEMORY, 0x01, err );
                return err;    /* ======== leave function ======== */
            }
#else
        /* check if DMA is available */
        trans_info.src_addr = address;
        trans_info.dst_addr = p_buffer;
        trans_info.data_size = data_size;
        trans_info.flag = flag;
        trans_info.direction = PCIEX_READ;
        err = pciex_DMA_get_chnum( p_unitinfo, &trans_info );
        if ( err >= 0 )
        {
            trans_info.dmach = (INT)err;
            /* DMA is available */
            err = pciex_DMA_start( p_unitinfo, &trans_info );
            if ( err < E_OK )
            {
                TR_ERROR( PCIEX_FUNCNAME_READ_MEMORY, 0x02, err );
                return err;    /* ======== leave function ======== */
            }
            err = pciex_DMA_wait( p_unitinfo, &trans_info );
            if ( err < E_OK )
            {
                TR_ERROR( PCIEX_FUNCNAME_READ_MEMORY, 0x03, err );
                return err;    /* ======== leave function ======== */
            }
#endif
        }
        else
        {
            /* DMA channel occupied */
            pciex_memcpy( p_buffer, address, data_size );
        }
    }
    else
    {
        pciex_memcpy( p_buffer, address, data_size );
    }
#else /* PCIEX_ENABLE_DMA */
    p_unitinfo = p_unitinfo; /* avoid LINT warning */
    flag = flag; /* avoid LINT warning */

    pciex_memcpy( p_buffer, address, data_size );
#endif /* PCIEX_ENABLE_DMA */
#endif //[kd] -

//[kd] -Need to have only the part with #else for PCIEX_ENABLE_DMA
    ptdev = ptdev; /* avoid LINT warning */
    flag = flag; /* avoid LINT warning */

    pciex_memcpy (p_buffer, address, data_size);

    return 0;
    //[kd] -return E_OK;
}

/**
 * @fn ER pciex_write_memory (struct triton_ep_dev *ptdev, VP address, VP p_buffer, UW data_size, PCIEX_ADDRESS_CONV_FLAG flag )
 * @brief
 * Writes memory.
 * @param[in] ptdev Unit information.
 * @param[in] address Target memory address.
 * @param[in] p_buffer Pointer to user buffer.
 * @param[in] data_size Data size.
 * @param[in] flag This flag shows whether vsaddr and vdaddr need lockking.
 * @return 0     Completed successfully.
 */
ER pciex_write_memory (struct triton_ep_dev *ptdev, VP address, VP p_buffer, UW data_size, PCIEX_ADDRESS_CONV_FLAG flag )
{
#if 0 //[kd] -DMA not required on the ATOM side.
#ifdef PCIEX_ENABLE_DMA
    ER err = E_OK;
    PCIEX_DMA_TRANS_INFO trans_info;

    /* Optimization of transfer performance */
    if ( (INT)data_size > p_unitinfo->p_drvinfo->conf.piorw[PCIEX_PIO_DMA_WRITE] )
    {
#ifdef PCIEX_USE_REGISTER_MODE_DMA
        /* check if DMA is available */
        err = pciex_DMA_get_chnum( p_unitinfo );
        if ( err >= 0 )
        {
            /* DMA is available */
            trans_info.dmach = (INT)err;
            trans_info.src_addr = p_buffer;
            trans_info.dst_addr = address;
            trans_info.data_size = data_size;
            trans_info.flag = flag;
            trans_info.direction = PCIEX_WRITE;
            err = pciex_DMA_start( p_unitinfo, &trans_info );
            if ( err < E_OK )
            {
                TR_ERROR( PCIEX_FUNCNAME_WRITE_MEMORY, 0x00, err );
                return err;    /* ======== leave function ======== */
            }
            err = pciex_DMA_wait( p_unitinfo, &trans_info );
            if ( err < E_OK )
            {
                TR_ERROR( PCIEX_FUNCNAME_WRITE_MEMORY, 0x01, err );
                return err;    /* ======== leave function ======== */
            }
#else
        /* check if DMA is available */
        trans_info.src_addr = p_buffer;
        trans_info.dst_addr = address;
        trans_info.data_size = data_size;
        trans_info.flag = flag;
        trans_info.direction = PCIEX_WRITE;
        err = pciex_DMA_get_chnum( p_unitinfo, &trans_info );
        if ( err >= 0 )
        {
            /* DMA is available */
            trans_info.dmach = (INT)err;
            err = pciex_DMA_start( p_unitinfo, &trans_info );
            if ( err < E_OK )
            {
                TR_ERROR( PCIEX_FUNCNAME_WRITE_MEMORY, 0x02, err );
                return err;    /* ======== leave function ======== */
            }
            err = pciex_DMA_wait( p_unitinfo, &trans_info );
            if ( err < E_OK )
            {
                TR_ERROR( PCIEX_FUNCNAME_WRITE_MEMORY, 0x03, err );
                return err;    /* ======== leave function ======== */
            }
#endif
        }
        else
        {
            /* not match align */
            pciex_memcpy( address, p_buffer, data_size );
        }
    }
    else
    {
        /* pio transfer */
        pciex_memcpy( address, p_buffer, data_size );
    }

#else /* PCIEX_ENABLE_DMA */
    p_unitinfo = p_unitinfo; /* avoid LINT warning */
    flag = flag; /* avoid LINT warning */

    pciex_memcpy( address, p_buffer, data_size );
#endif /* PCIEX_ENABLE_DMA */
#endif //[kd] -

//[kd] -Need to have only the code inside #else of PCIEX_ENABLE_DMA
    ptdev = ptdev; /* avoid LINT warning */
    flag = flag; /* avoid LINT warning */

    printk (KERN_INFO "Address at c2u = %u\n", address);
    if (copy_from_user( address, p_buffer, data_size) != 0)
    {
    	printk(KERN_ERR "copy_from_user failed\n");
    	return -EINVAL;
    }

    printk(KERN_INFO "final %s %u --------------------\n", __FILE__, __LINE__);
    return 0;
    //[kd] -return E_OK;
}

/**
 * @fn ER pciex_issue_req(struct triton_ep_dev * ptdev, ID waitflg, TMO tmout )
 * @brief
 * This function issues a request to the peer.
 * @param[in] ptdev Unit information.
 * @param[in] waitflg Event flag id to wait for the request completion.
 * @param[in] tmout Timeout value to wait for the request completion.
 * @return 0     Completed successfully.
 */
ER pciex_issue_req(struct triton_ep_dev * ptdev, ID waitflg, TMO tmout )
{
    //PCIEX_DRV_INFO *p_drvinfo = NULL;
    PCIEX_ADMIN_REGION *p_admin = NULL;
    UW flgptn = 0;
    //ER err = E_OK;
    ER err = 0;
    //p_drvinfo = p_unitinfo->p_drvinfo;
    p_admin = (PCIEX_ADMIN_REGION *) ptdev->region_admin.vaddr;

    pciex_issue_msi(p_admin->msi, PESHM_SHM_REQ);

    /* wait Root Complex response
    //Presently commented out later need to replaced. Ashwin is doing it. [kd]

    //err = tk_wai_flg( waitflg, ( PCIEX_CMP_WAIT | PCIEX_CMP_ABORT ), ( TWF_ORW | TWF_CLR ), &flgptn, tmout );

    if ( err == -1) //[kd] - presently -1, put it appropiately E_TMOUT
    {
        p_drvinfo->individual[PCIEX_SELF_UNIT_NUM].drvsts = PCIEX_STS_CTIMEOUT;
        p_drvinfo->individual[PCIEX_PEER_UNIT_NUM].drvsts = PCIEX_STS_CTIMEOUT;
    } */

    if ( ( flgptn & PCIEX_CMP_ABORT ) != 0 )
    {
        TR_ERROR( PCIEX_FUNCNAME_ISSUE_REQ, 0x00, E_ABORT );
        return -1;
        //[kd] - return E_ABORT; /* ======== leave function ======== */
    }

    return err;
}


/**
 * @fn pciex_replace_devname( B *p_str )
 * @brief
 * This function replaces device name.
 * @param[out] p_str Pointer to buffer to replace name.
 * @return void
 */
void pciex_replace_devname( B *p_str )
{
    if ( p_str[strlen( PCIEX_PHYDEVNAME )] == PCIEX_SELF_UNIT_NAME )
    {
        p_str[strlen( PCIEX_PHYDEVNAME )] = PCIEX_PEER_UNIT_NAME;
    }
    else
    {
        p_str[strlen( PCIEX_PHYDEVNAME )] = PCIEX_SELF_UNIT_NAME;
    }
}

/*@}*/
