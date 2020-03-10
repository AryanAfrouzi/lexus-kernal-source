/**
 * @file pciex_controlmacro.h
 * @brief Definitions, structure and function declarations for control task.
 * @version $Name: PCIEX_r2_C01 $ (revision $Revision: 1.2 $)
 * @author $Author: madachi $
 * @par copyright
 * (c) 2009 ADIT Corporation
 */

#ifndef PCIEX_TRACE_H
#define PCIEX_TRACE_H

#include "pciex_types.h" //[kd]

#ifdef HAS_TRACE /* use Trace */

/**********************************************************************/
/* set Trace unique file number (needed if HAS_TRACE is set)          */
/**********************************************************************/
#include "util_trace.h"

typedef enum
{
    PCIEX_FILENAME_CONTROLMACRO = 0
   ,PCIEX_FILENAME_DMAC64
   ,PCIEX_FILENAME_MAIN
   ,PCIEX_FILENAME_LIB
   ,PCIEX_FILENAME_MANAGESHM
   ,PCIEX_FILENAME_UTIL
} PCIEX_FILENAME_ENUM;

typedef enum
{
    PCIEX_FUNCNAME_MAIN                  = 0x01 /* pciex_xx_main */
   ,PCIEX_FUNCNAME_GETCONFIG             = 0x02 /* pciex_get_configuration */
   ,PCIEX_FUNCNAME_FINISH_TSK            = 0x03 /* pciex_finish_task */
   ,PCIEX_FUNCNAME_MEMCPY                = 0x04 /* pciex_memcpy */
   ,PCIEX_FUNCNAME_DMA_INIT              = 0x05 /* pciex_DMA_init */
   ,PCIEX_FUNCNAME_DMA_GET_CHNUM         = 0x06 /* pciex_DMA_get_chnum */
   ,PCIEX_FUNCNAME_DMA_START             = 0x07 /* pciex_DMA_start */
   ,PCIEX_FUNCNAME_DMA_WAIT              = 0x08 /* pciex_DMA_wait */
   ,PCIEX_FUNCNAME_ALLOC_BUFFER          = 0x09 /* pciex_alloc_buffer */
   ,PCIEX_FUNCNAME_INIT_INT              = 0x0a /* pciex_initint */
   ,PCIEX_FUNCNAME_INIT_ERRINT           = 0x0b /* pciex_initerrint */
   ,PCIEX_FUNCNAME_UNDEF_INT             = 0x0c /* pciex_undefint */
   ,PCIEX_FUNCNAME_INTHDL_TSK            = 0x0d /* pciex_inthdltsk */
   ,PCIEX_FUNCNAME_DEL_INTHDL_TSK        = 0x0e /* pciex_del_inthdltsk */
   ,PCIEX_FUNCNAME_INIT_MACRO            = 0x0f /* pciex_initmacro */
   ,PCIEX_FUNCNAME_RC_INIT_MACRO         = 0x10 /* pciex_rc_initmacro */
   ,PCIEX_FUNCNAME_EP_INIT_MACRO         = 0x11 /* pciex_ep_initmacro */
   ,PCIEX_FUNCNAME_MSI_HANDLER           = 0x12 /* pciex_msi_handler */
   ,PCIEX_FUNCNAME_REQ_HANDLER           = 0x13 /* pciex_req_handler */
   ,PCIEX_FUNCNAME_HANDLE_RCVERR         = 0x14 /* pciex_handle_rcverr */
   ,PCIEX_FUNCNAME_CONFIG_RC             = 0x15 /* pciex_config_rc */
   ,PCIEX_FUNCNAME_NOTIFY_DRICET_MSI     = 0x16 /* pciex_notify_directmsi */
   ,PCIEX_FUNCNAME_NOTIFY_CONFIG_COMP    = 0x17 /* pciex_notify_configcomp */
   ,PCIEX_FUNCNAME_NOTIFY_BUS_ERR        = 0x18 /* pciex_notify_buserror */
   ,PCIEX_FUNCNAME_NOTIFY_EXIT           = 0x19 /* pciex_notify_exit */
   ,PCIEX_FUNCNAME_WAIT_PEER_READY       = 0x1a /* pciex_wait_peer_ready */
   ,PCIEX_FUNCNAME_SET_TBASEAD_REQ       = 0x1b /* pciex_set_TargetBaseAddressReq */
   ,PCIEX_FUNCNAME_SET_NCONNECT_WINDOW   = 0x1c /* pciex_set_NconnectWindow */
   ,PCIEX_FUNCNAME_SET_NCNCT_WINDOW_SEG  = 0x1d /* pciex_set_NconnectWindowSeg */
   ,PCIEX_FUNCNAME_SET_MSI_SETTING       = 0x1e /* pciex_set_MSISetting */
   ,PCIEX_FUNCNAME_WAIT_RESET            = 0x1f /* pciex_wait_reset */
   ,PCIEX_FUNCNAME_WAIT_LINKUP           = 0x20 /* pciex_wait_LinkUp */
   ,PCIEX_FUNCNAME_WAIT_NEGOTIATION      = 0x21 /* pciex_wait_Negotiation */
   ,PCIEX_FUNCNAME_WAIT_SLAVE_READY      = 0x22 /* pciex_wait_SlaveReady */
   ,PCIEX_FUNCNAME_WAIT_SLAVE_NOT_READY  = 0x23 /* pciex_wait_SlaveNotReady */
   ,PCIEX_FUNCNAME_WAIT_CONFIG           = 0x24 /* pciex_wait_Config */
   ,PCIEX_FUNCNAME_WAIT_MSI_ENABLE       = 0x25 /* pciex_wait_MSI_enable */
   ,PCIEX_FUNCNAME_SET_TBASEADDRESS      = 0x26 /* pciex_set_TargetBaseAddress */
   ,PCIEX_FUNCNAME_RC_DEVCONFIG          = 0x27 /* pciex_rc_dev_configuration */
   ,PCIEX_FUNCNAME_WRITE_TBASEADD_TO_EP  = 0x28 /* pciex_write_TargetBaseAddress_toEP */
   ,PCIEX_FUNCNAME_WRITE_COMMAND_TO_EP   = 0x29 /* pciex_write_Command_toEP */
   ,PCIEX_FUNCNAME_GET_ROOTPORT_INFO     = 0x2a /* pciex_get_rootport_info */
   ,PCIEX_FUNCNAME_READ_CONFIG_H         = 0x2b /* pciex_read_config_h */
   ,PCIEX_FUNCNAME_READ_CONFIG_W         = 0x2c /* pciex_read_config_w */
   ,PCIEX_FUNCNAME_WRITE_CONFIG_H        = 0x2d /* pciex_write_config_h */
   ,PCIEX_FUNCNAME_WRITE_CONFIG_W        = 0x2e /* pciex_write_config_w */
   ,PCIEX_FUNCNAME_READ_CONFIG           = 0x2f /* pciex_read_config */
   ,PCIEX_FUNCNAME_WRITE_CONFIG          = 0x30 /* pciex_write_config */
   ,PCIEX_FUNCNAME_WAIT                  = 0x31 /* pciex_wait */
   ,PCIEX_FUNCNAME_START                 = 0x32 /* pciex_start */
   ,PCIEX_FUNCNAME_STOP                  = 0x33 /* pciex_stop */
   ,PCIEX_FUNCNAME_MAIN_TASK             = 0x34 /* pciex_MainTask */
   ,PCIEX_FUNCNAME_INIT_DRVINFBLOCK      = 0x35 /* pciex_init_drvinfblock */
   ,PCIEX_FUNCNAME_INIT_ADMINS           = 0x36 /* pciex_init_admins */
   ,PCIEX_FUNCNAME_DEF_DEVICE            = 0x37 /* pciex_def_device */
   ,PCIEX_FUNCNAME_UNDEF_DEVICE          = 0x38 /* pciex_undef_device */
   ,PCIEX_FUNCNAME_OPEN_FN               = 0x39 /* pciex_open_fn */
   ,PCIEX_FUNCNAME_CLOSE_FN              = 0x3a /* pciex_close_fn */
   ,PCIEX_FUNCNAME_EVENT_FN              = 0x3b /* pciex_event_fn */
   ,PCIEX_FUNCNAME_START_MAIN_TASK       = 0x3c /* pciex_start_main_task */
   ,PCIEX_FUNCNAME_SETUP_DRVINFBLK_CMN   = 0x3d /* pciex_setup_drvinfblock_common */
   ,PCIEX_FUNCNAME_SETUP_DRVINFBLK_IND   = 0x3e /* pciex_setup_drvinfblock_individual */
   ,PCIEX_FUNCNAME_ALLOC_MSI             = 0x3f /* pciex_alloc_memmsi */
   ,PCIEX_FUNCNAME_ALLOC_SHMADMIN        = 0x40 /* pciex_alloc_shmadmin */
   ,PCIEX_FUNCNAME_PHY_READDATA          = 0x41 /* pciex_phy_readData */
   ,PCIEX_FUNCNAME_PHY_READDATA_SELF     = 0x42 /* pciex_phy_readData_self */
   ,PCIEX_FUNCNAME_PHY_READDATA_PEER     = 0x43 /* pciex_phy_readData_peer */
   ,PCIEX_FUNCNAME_PHY_WRITEDATA         = 0x44 /* pciex_phy_writeData */
   ,PCIEX_FUNCNAME_PHY_WRITEDATA_SELF    = 0x45 /* pciex_phy_writeData_self */
   ,PCIEX_FUNCNAME_PHY_WRITEDATA_PEER    = 0x46 /* pciex_phy_writeData_peer */
   ,PCIEX_FUNCNAME_LOG_READDATA          = 0x47 /* pciex_log_readData */
   ,PCIEX_FUNCNAME_LOG_WRITEDATA         = 0x48 /* pciex_log_writeData */
   ,PCIEX_FUNCNAME_GET_SHM_INFO_SELF     = 0x49 /* pciex_get_shm_info_self */
   ,PCIEX_FUNCNAME_GET_SHM_INFO_PEER     = 0x4a /* pciex_get_shm_info_peer */
   ,PCIEX_FUNCNAME_CREATE_SHM_SELF       = 0x4b /* pciex_create_shm_self */
   ,PCIEX_FUNCNAME_CREATE_SHM_PEER       = 0x4c /* pciex_create_shm_peer */
   ,PCIEX_FUNCNAME_RELEASE_SHM_SELF      = 0x4d /* pciex_release_shm_self */
   ,PCIEX_FUNCNAME_RELEASE_SHM_PEER      = 0x4e /* pciex_release_shm_peer */
   ,PCIEX_FUNCNAME_ATTACH_SHM_SELF       = 0x4f /* pciex_attach_shm_self */
   ,PCIEX_FUNCNAME_ATTACH_SHM_PEER       = 0x50 /* pciex_attach_shm_peer */
   ,PCIEX_FUNCNAME_DETACH_SHM_SELF       = 0x51 /* pciex_detach_shm_self */
   ,PCIEX_FUNCNAME_DETACH_SHM_PEER       = 0x52 /* pciex_detach_shm_peer */
   ,PCIEX_FUNCNAME_LOCK_SHM_SELF         = 0x53 /* pciex_lock_shm_self */
   ,PCIEX_FUNCNAME_LOCK_SHM_PEER         = 0x54 /* pciex_lock_shm_peer */
   ,PCIEX_FUNCNAME_UNLOCK_SHM_SELF       = 0x55 /* pciex_unlock_shm_self */
   ,PCIEX_FUNCNAME_UNLOCK_SHM_PEER       = 0x56 /* pciex_unlock_shm_peer */
   ,PCIEX_FUNCNAME_DIRECT_MSI            = 0x57 /* pciex_direct_msi */
   ,PCIEX_FUNCNAME_RECONFIGURATION       = 0x58 /* pciex_reconfiguration */
   ,PCIEX_FUNCNAME_READ_SHM              = 0x59 /* pciex_read_shm */
   ,PCIEX_FUNCNAME_WRITE_SHM             = 0x5a /* pciex_write_shm */
   ,PCIEX_FUNCNAME_READ_MEMORY           = 0x5b /* pciex_read_memory */
   ,PCIEX_FUNCNAME_WRITE_MEMORY          = 0x5c /* pciex_write_memory */
   ,PCIEX_FUNCNAME_ISSUE_REQ             = 0x5d /* pciex_issue_req */
   ,PCIEX_FUNCNAME_EXIT_NOTIFICATION     = 0x5e /* pciex_issue_exit_notification */
   ,PCIEX_FUNCNAME_ALLOCATE_SHM          = 0x5f /* pciex_allocate_shm */
   ,PCIEX_FUNCNAME_FREE_SHM              = 0x60 /* pciex_free_shm */
   ,PCIEX_FUNCNAME_ISSUE_DEVNAME         = 0x61 /* pciex_issue_devname */
   ,PCIEX_FUNCNAME_ISSUE_USERID          = 0x62 /* pciex_issue_userid */
   ,PCIEX_FUNCNAME_GET_SHMB_INDEX        = 0x63 /* pciex_get_shmb_index */
   ,PCIEX_FUNCNAME_LOCK_SHMB             = 0x64 /* pciex_lock_shmb */
   ,PCIEX_FUNCNAME_UNLOCK_SHMB           = 0x65 /* pciex_unlock_shmb */
   ,PCIEX_FUNCNAME_ISSUE_SHMRENEW_EVT    = 0x66 /* pciex_issue_shmrenew_event */
   ,PCIEX_FUNCNAME_SHM                   = 0x67 /* pciex_allocate_shared_memory */
   ,PCIEX_FUNCNAME_NOTIFY_FAIL_ALLOC     = 0x68 /* pciex_notify_fail_allocate */

} PCIEX_TRACE_FUNC_NAME;

#if 0
#define TR_FATAL( a, b, c )     TR_F(  ( UTIL_DRV_UHCLASS, "%4u%4u%4u", (a), (b), (c) ) )
#define TR_ERROR( a, b, c )     TR_E(  ( UTIL_DRV_UHCLASS, "%4u%4u%4u", (a), (b), (c) ) )
#define TR_SYSMIN( a, b, c )    TR_SM( ( UTIL_DRV_UHCLASS, "%4u%4u%4u", (a), (b), (c) ) )
#define TR_SYSTEM( a, b, c )    TR_S(  ( UTIL_DRV_UHCLASS, "%4u%4u%4u", (a), (b), (c) ) )
#define TR_COMPONENT( a, b, c ) TR_C(  ( UTIL_DRV_UHCLASS, "%4u%4u%4u", (a), (b), (c) ) )
#define TR_USER1( a, b, c )     TR_U1( ( UTIL_DRV_UHCLASS, "%4u%4u%4u", (a), (b), (c) ) )
#define TR_USER2( a, b, c )     TR_U2( ( UTIL_DRV_UHCLASS, "%4u%4u%4u", (a), (b), (c) ) )
#define TR_USER3( a, b, c )     TR_U3( ( UTIL_DRV_UHCLASS, "%4u%4u%4u", (a), (b), (c) ) )
#define TR_USER4( a, b, c )     TR_U4( ( UTIL_DRV_UHCLASS, "%4u%4u%4u", (a), (b), (c) ) )
#endif

// [kd] - for the linux
#define TR_FATAL( a, b, c )     printk("FATAL: %s: %d - %4u %4u %4u", __FILE__, __LINE__, a, b, c )
#define TR_ERROR( a, b, c )     printk("ERROR: %s: %d - %4u %4u %4u", __FILE__, __LINE__, a, b, c )
#define TR_SYSMIN( a, b, c )    printk("SYSMIN: %s: %d - %4u %4u %4u", __FILE__, __LINE__, a, b, c )
#define TR_SYSTEM( a, b, c )    printk("SYSTEM: %s: %d - %4u %4u %4u", __FILE__, __LINE__, a, b, c )
#define TR_COMPONENT( a, b, c ) printk("COMPONENT: %s: %d - %4u %4u %4u", __FILE__, __LINE__, a, b, c )
#define TR_USER1( a, b, c )     printk("USER1: %s: %d - %4u %4u %4u", __FILE__, __LINE__, a, b, c )
#define TR_USER2( a, b, c )     printk("USER2: %s: %d - %4u %4u %4u", __FILE__, __LINE__, a, b, c )
#define TR_USER3( a, b, c )     printk("USER3: %s: %d - %4u %4u %4u", __FILE__, __LINE__, a, b, c )
#define TR_USER4( a, b, c )     printk("USER4: %s: %d - %4u %4u %4u", __FILE__, __LINE__, a, b, c )

#else

#define TR_FATAL( a, b, c )
#define TR_ERROR( a, b, c )
#define TR_SYSMIN( a, b, c )
#define TR_SYSTEM( a, b, c )
#define TR_COMPONENT( a, b, c )
#define TR_USER1( a, b, c )
#define TR_USER2( a, b, c )
#define TR_USER3( a, b, c )
#define TR_USER4( a, b, c )

#endif /* HAS_TRACE */

#endif /* PCIEX_TRACE_H */

