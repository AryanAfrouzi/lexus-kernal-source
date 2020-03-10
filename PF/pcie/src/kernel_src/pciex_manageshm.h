/**
 * @file pciex_manageshm.h
 * @brief Definitions, structure and function declarations for main task.
 * @version $Name: PCIEX_r2_C01 $ (revision $Revision: 1.4 $)
 * @author $Author: tkojima $
 * @par copyright
 * (c) 2009 ADIT Corporation
 */

#ifndef PCIEX_MANAGESHM_H
#define PCIEX_MANAGESHM_H

/***********************************************************************
 *   definitions
 ***********************************************************************/
/**
 * @addtogroup InternalDefinitions
 */
/*@{*/
#include "pciex_types.h"
#include "pciex_common.h"
#include "triton_ep.h"
#include "pciex.h"

/** Event flag name for waiting for request completion */
#define PCIEX_CMP_FLGNAME  "ADPEXCP"

/** Unit name of self side */
#define PCIEX_SELF_UNIT_NAME 'a'

/** Unit name of peer side */
#define PCIEX_PEER_UNIT_NAME 'b'

/** Physical device name(self side) */
#define PCIEX_SELF_PHYDEVNAME  (UB *)"pciea"

/** Physical device name(peer side) */
#define PCIEX_PEER_PHYDEVNAME  (UB *)"pcieb"

/** Means no shared memory block is created */
#define PCIEX_INDEX_NONALLOC  -1

/** User id max limit */
#define PCIEX_SHMMAXUSRNUM          65535

/** Direct MSI request */
#define PCIEX_DIRECT_MSI_REQ  0x80000000

/** Timeout for exit ack */
#define PCIEX_EXIT_ACK_TMOUT  500
/*@}*/
/***********************************************************************
 *   constants
 ***********************************************************************/

/***********************************************************************
 *  structures
 ***********************************************************************/
/**
 * @addtogroup InternalStructures
 */
/*@{*/
/**
 * @typedef PCIEX_SHM_ACCESSINFO
 * @par DESCRIPTION
 * Structure for read or write memory.
 */
/*@}*/
typedef struct _PCIEX_SHM_ACCESSINFO
{
    UW subunitnum; /**< subunit number */
    UW offset;     /**< offset from shared memory base address */
    UB *p_buffer;  /**< read or write data with buffer this pointer indicates */
    UW data_size;  /**< data size */
} PCIEX_SHM_ACCESSINFO;

/***********************************************************************
 *  functions
 ***********************************************************************/

// [kd] - Defination of the file which will called by the Atom side DD.
void pciex_issue_msi(UW* p_msi, UH msgdata);
ER pciex_log_readData (struct triton_ep_read_write *p_devReq, struct triton_ep_dev *ptdev);
ER pciex_log_writeData (struct triton_ep_read_write *p_devReq, struct triton_ep_dev *ptdev);
ER pciex_get_shm_info_peer (struct triton_ep_dev * ptdev, PCIEX_SHM_INFO *p_shminfo_req);
ER pciex_create_shm_peer (struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_shm_req);
ER pciex_release_shm_peer (struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_shm_req);
ER pciex_attach_shm_peer (struct triton_ep_dev * ptdev, PCIEX_SHM_REQ *p_shm_req);
ER pciex_detach_shm_peer (struct triton_ep_dev * ptdev, PCIEX_SHM_REQ  *p_shm_req);
ER pciex_lock_shm_peer (struct triton_ep_dev * ptdev, PCIEX_LOCK_REQ *p_req);
ER pciex_unlock_shm_peer (struct triton_ep_dev * ptdev, PCIEX_LOCK_REQ *p_req);
ER pciex_read_shm (struct triton_ep_dev *ptdev, PCIEX_SHM_ACCESSINFO *p_ainfo);
ER pciex_write_shm (struct triton_ep_dev *ptdev, PCIEX_SHM_ACCESSINFO *p_ainfo);
ER pciex_read_memory (struct triton_ep_dev *ptdev, VP address, VP p_buffer, UW data_size, PCIEX_ADDRESS_CONV_FLAG flag);
ER pciex_write_memory (struct triton_ep_dev *ptdev, VP address, VP p_buffer, UW data_size, PCIEX_ADDRESS_CONV_FLAG flag);
ER pciex_issue_req (struct triton_ep_dev *ptdev, ID waitflg, TMO tmout);

/***********************************************************************
 *  global variables
 ***********************************************************************/

#endif /*  PCIEX_MANAGESHM_H */
