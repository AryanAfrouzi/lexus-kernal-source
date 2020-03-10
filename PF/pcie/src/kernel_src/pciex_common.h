/**
 * @file pciex_common.h
 * @brief Definitions, structures, and functions in common.
 * @version $Name: PCIEX_r2_C01 $ (revision $Revision: 1.7 $)
 * @author $Author: madachi $
 * @par copyright
 * (c) 2009 ADIT Corporation
 */

#ifndef PCIEX_COMMON_H
#define PCIEX_COMMON_H

#include "pciex_types.h" //[kd]
//#include "pciex_manageshm.h" //[kd]

/***********************************************************************
 *   definitions
 ***********************************************************************/
/**
 * @addtogroup InternalDefinitions
 */
/*@{*/
/** PCI express Root Complex mode */
#define PCIEX_RC 0

/** PCI express End Point mode */
#define PCIEX_EP 1

/** The mode which this PCIex driver works as */
#define PCIEX_COMPILE_TARGET PCIEX_EP
/* #define PCIEX_COMPILE_TARGET PCIEX_RC */

/*
 * If you want to enable DMA to transfer data, define PCIEX_ENABLE_DMA
 * below. If not, commnet out below definition.
 */
#define PCIEX_ENABLE_DMA
#define PCIEX_USE_REGISTER_MODE_DMA

#ifdef PCIEX_ENABLE_DMA
//#include "DMA_axi.h"
#endif

/*
 * If you want to enable TD(TLP Digest) field of TLP, define PCIEX_ENABLE_TD
 * below. If not, commnet out below definition.
 */
#define PCIEX_ENABLE_TD

/** Number of units */
#define PCIEX_NUM_OF_UNIT    2

/** Unit number of self side */
#define PCIEX_SELF_UNIT_NUM  0

/** Unit number of peer side */
#define PCIEX_PEER_UNIT_NUM  1

/** Physical device name(common) */
#define PCIEX_PHYDEVNAME       (UB *)"pcie"

/** The name of the main task */
#define PCIEX_MAIN_TSK_NAME    (UB *)"ADPEXMN"

/** The name of the control task */
#define PCIEX_CTRL_TSK_NAME    (UB *)"ADPEXCTL"

/** Wait time for finishing task  (wait PCIEX_RELWAI_DELAY * #PCIEX_FINWAI_TIMEOUT ms for finishing task) */
#define PCIEX_RELWAI_DELAY            (10)

/** Loop number of finishing task (wait #PCIEX_RELWAI_DELAY * PCIEX_FINWAI_TIMEOUT ms for finishing task) */
#define PCIEX_FINWAI_TIMEOUT          (500)

/** Maximum limit of interrupt level */
#define PCIEX_INT_LVL_MAX     15

/** Minimum limit of interrupt level */
#define PCIEX_INT_LVL_MIN     0

/** Maximum limit of core number */
#define PCIEX_CORE_NUM_MAX     TLCID_3

/** Minimum limit of core number */
#define PCIEX_CORE_NUM_MIN     TLCID_SMP

/** Minimum limit of request queue size */
#define PCIEX_MAXREQ_MIN       1

/** DMA transfer size(byte) */
#define PCIEX_DMA_TRANSFER_SIZE   64

#define DRVCONF_CACHELINE_SIZE      32

#define PCIEX_CHKCACHEALIGN(p_buf,size) \
            (((((UW)(p_buf)) & (DRVCONF_CACHELINE_SIZE - 1)) == 0) && \
             ((((UW)(p_buf)+(size)) & (DRVCONF_CACHELINE_SIZE - 1)) == 0))

/** NConnect start address */
#define PCIEX_NCADDR_START          0x60000000

/** Window virtual address */
#define PCIEX_WIN_VADDRESS          JV_PCIEX_WINBASE

/** Segment align */
#define PCIEX_SEGMENT_ALIGN         0x00400000

/** PCI start adrress for RC */
#define PCIEX_PCIADDR_RC_START 0x0F000000

/** PCI start adrress for EP */
#define PCIEX_PCIADDR_EP_START PCIEX_NCADDR_START

/** Message data of MSI */
#define PCIEX_MSI_BASEDATA          0x5000

/** Segment value of window0 */
#define PCIEX_RC_WIN0SEG_MSW        0x00000000

/** Segment value of window1 */
#define PCIEX_RC_WIN1SEG_MSW        0x00000000

/** Retry counter of RC to wait for EP be ready */
#define PCIEX_RC_WAIT_EP_UP_RETRY   100000

/** PIO/DMA transfer threshold array number for read */
#define PCIEX_PIO_DMA_READ     0

/** PIO/DMA transfer threshold array number for write */
#define PCIEX_PIO_DMA_WRITE    1

/** Wait time after reset off(msec) */
#define PCIEX_WAIT_RESET_OFF (100)

/** Sleep time for retry (msec) */
#define PCIEX_WAITSLEEPTIME (1)

/** Retry counter for waiting reset (-1 means there's no wait limit) Tkernel delays task for sleeptime+1 msec */
#define PCIEX_WAIRSTRETRY (10000 / (PCIEX_WAITSLEEPTIME + 1))

/** Retry counter for waiting link up (-1 means there's no wait limit) Tkernel delays task for sleeptime+1 msec */
#define PCIEX_WAILUPRETRY  (100000 / (PCIEX_WAITSLEEPTIME + 1))

/** Retry counter for waiting negotiation (-1 means there's no wait limit) Tkernel delays task for sleeptime+1 msec */
#define PCIEX_WAINEGRETRY  (1000 / (PCIEX_WAITSLEEPTIME + 1))

/** Retry counter for waiting slave ready (-1 means there's no wait limit) Tkernel delays task for sleeptime+1 msec */
#define PCIEX_WAISRDYRETRY (100000 / (PCIEX_WAITSLEEPTIME + 1))

/** Retry counter for waiting slave NOT ready (-1 means there's no wait limit) Tkernel delays task for sleeptime+1 msec */
#define PCIEX_WAISNRDYRETRY (10000 / (PCIEX_WAITSLEEPTIME + 1))

/** Retry counter for waiting configuration (-1 means there's no wait limit) Tkernel delays task for sleeptime+1 msec */
#define PCIEX_WAICFGRETRY  (1000 / (PCIEX_WAITSLEEPTIME + 1))

/** Retry counter for waiting MSI enable (-1 means there's no wait limit) Tkernel delays task for sleeptime+1 msec */
#define PCIEX_WAIMSIRETRY  (1000 / (PCIEX_WAITSLEEPTIME + 1))

/** Local buffer size */
#define PCIEX_LOCAL_BUF    256

#define PCIEX_TLP_LENGTH_CFG  0x00000001

/** Macro for string copy */
#define STRCPY strcpy

/** Macro for string cut */
#define STRCAT strcat

/** Macro for string compare */
#define STRCMP strcmp

/** Macro for integer to string */
#define ITOS   pciex_itos

/** Macro for alphabet to integer */
#define ATOI   pciex_atoi

/** The number of main task */
#define PCIEX_MAIN_TASK_NUM   (0)

/** The number of control task */
#define PCIEX_CTRL_TASK_NUM   (1)

/** Number of tasks */
#define PCIEX_TASK_MAX        (2)

/** Array number of MSI interrupt */
#define PCIEX_INT_MSI         (0)

/** Array number of error interrupt */
#define PCIEX_INT_ERR         (1)

/** Number of interrupts */
#define PCIEX_INT_MAX         (2)

/** Number of DMA channel use */
#define PCIEX_DMACH_NUM       (2)

/** The parameter name of shared memory size in DEVCONF */
#define DRVCONF_PCIEX_SHMSIZE           ((UB *)"PCIEX_SHM_SIZE")

/** Shared memory size parameter number in DEVCONF */
#define DRVCONF_PCIEX_SHMSIZE_NUM       (1)

/** The parameter name of shared memory maximum block size in DEVCONF */
#define DRVCONF_PCIEX_SHM_MAXBLOCKSIZE      ((UB *)"PCIEX_SHM_MAX_BLOCKSIZE")

/** Maximum block size parameter number in DEVCONF */
#define DRVCONF_PCIEX_SHM_MAXBLOCKSIZE_NUM (1)

/** The parameter name of task priorities in DEVCONF */
#define DRVCONF_PCIEX_STR_DRV_TSK_PRI ((UB *)"PCIEX_TSK_PRI")

/** The parameter name of task stack size in DEVCONF */
#define DRVCONF_PCIEX_STR_DRV_STK_SIZ ((UB *)"PCIEX_STK_SIZ")

/** The parameter name of communication timeout in DEVCONF */
#define DRVCONF_PCIEX_DRV_TMO        ((UB *)"PCIEX_COMM_TMO")

/** Communication timeout parameter number in DEVCONF */
#define DRVCONF_PCIEX_DRV_TMO_NUM    (1)

/** The parameter name of shared memory address in DEVCONF */
#define DRVCONF_PCIEX_SHM        ((UB *)"PCIEX_SHM_ADR")

/** Shared memory address parameter number in DEVCONF */
#define DRVCONF_PCIEX_SHM_NUM    (1)

/** The parameter name of task core numbers in DEVCONF */
#define DRVCONF_PCIEX_TASKCORE         ((UB *)"PCIEX_TASKCORE")

/** The parameter name of interrupt levels in DEVCONF */
#define DRVCONF_PCIEX_INT_LVL          ((UB *)"PCIEX_INT_LVL")

/** Interrupt level parameter number in DEVCONF */
#define DRVCONF_PCIEX_INT_LVL_NUM      (PCIEX_INT_MAX)

/** The parameter name of interrupt core numbers in DEVCONF */
#define DRVCONF_PCIEX_INTCORE          ((UB *)"PCIEX_INTCORE")

/** Interrupt core parameter number in DEVCONF */
#define DRVCONF_PCIEX_INTCORE_NUM      (PCIEX_INT_MAX)

/** The parameter name of interrupt stack size in DEVCONF */
#define DRVCONF_PCIEX_INTSTK_SIZ       ((UB *)"PCIEX_INTSTK_SIZ")

/** Interrupt stack size parameter number in DEVCONF */
#define DRVCONF_PCIEX_INTSTK_SIZ_NUM   (PCIEX_INT_MAX)

/** The parameter name of DMA channel in DEVCONF */
#define DRVCONF_PCIEX_DMACH       ((UB *)"PCIEX_DMACH")

/** The parameter name of max PIO size in DEVCONF */
#define DRVCONF_PCIEX_MAX_PIO_RW       ((UB *)"PCIEX_MAX_PIO_RW")

/** Max PIO size name parameter number in DEVCONF */
#define DRVCONF_PCIEX_MAX_PIO_RW_NUM   (2)

/** The parameter name of max request queue size in DEVCONF */
#define DRVCONF_PCIEX_MAXREQ       ((UB *)"PCIEX_MAX_REQ")

/** Input parameter number in calling main function */
#define PCIEX_INPUT_PARAM          (1)

#define PCIEX_1STBE_ENABLEALL 0x0F

#define PCIEX_RC_BUSNUM      0
#define PCIEX_RC_DEVICENUM   0
#define PCIEX_RC_FUNCNUM     0
#define PCIEX_EP_BUSNUM      1
#define PCIEX_EP_DEVICENUM   0
#define PCIEX_EP_FUNCNUM     0
#define PCIEX_SWTAG          0x10
#define PCIEX_MSGCTL_INITVAL 0x0092
/*@}*/

/***********************************************************************
 *   constants
 ***********************************************************************/
/**
 * @addtogroup InternalEnums
 */
/*@{*/
/**
 * @typedef PCIEX_MODE
 * @par DESCRIPTION
 * PCI express mode.
 */
typedef enum _PCIEX_MODE
{
    PCIEX_MODE_RC = 0     /**< Root Complex mode */
   ,PCIEX_MODE_EP         /**< End Point mode */
} PCIEX_MODE;

/**
 * @typedef PCIEX_STS
 * @par DESCRIPTION
 * Driver status.
 */
typedef enum _PCIEX_STS
{
    PCIEX_STS_STARTUP = 0     /**< Startup */
   ,PCIEX_STS_NORMAL          /**< Normal operation */
   ,PCIEX_STS_SUSPEND         /**< Suspendion mode */
   ,PCIEX_STS_BUSERROR        /**< Bus error */
   ,PCIEX_STS_CTIMEOUT        /**< Communication timeout */
   ,PCIEX_STS_RECONFIG        /**< Reconfiguration */
   ,PCIEX_STS_WAIT_EXIT       /**< Wait exit */
} PCIEX_STS;

/**
 * @typedef PCIEX_ADDRESS_CONV_FLAG
 * @par DESCRIPTION
 * Use this bit flag to indicate which buffer needs locking(LockSpace).
 */
typedef enum _PCIEX_ADDRESS_CONV_FLAG
{
     PCIEX_CONV_NON = 0x00000000    /**< Doesn't need converting */
    ,PCIEX_CONV_SRC = 0x00000001    /**< SRC needs converting */
    ,PCIEX_CONV_DST = 0x00000002    /**< DST needs converting */
} PCIEX_ADDRESS_CONV_FLAG;

/*@}*/

/***********************************************************************
 *  structures
 ***********************************************************************/
/**
 * @addtogroup InternalStructures
 */
/*@{*/

#if 0
typedef struct
{
        struct  t_devreq *next; /* I: Link to request packet (NULL: termination)        */
        VP      exinf;  /* X: Extended information       */
        ID      devid;  /* I: Target device ID  */
        INT     cmd:4;  /* I: Request command   */
        BOOL    abort:1;        /* I: TRUE if abort request     */
        BOOL    nolock:1;       /* I: TRUE if lock (making resident) not needed */
        INT     rsv:26; /* I: reserved (always 0)       */
        //T_TSKSPC      tskspc; /* I: Task space of requesting task     */
        INT     start;  /* I: Starting data number      */
        INT     size;   /* I: Request size      */
        VP      buf;    /* I: IO buffer address */
        INT     asize;  /* O: Size of result    */
        //ER    error;  /* O: Error result      */
/* Implementation-dependent information may be added beyond this point. */
} T_DEVREQ;
#endif


/**
 * @typedef PCIEX_DEVCONF_PARAM
 * @par DESCRIPTION
 * DEVCONF parameters.
 */
typedef struct _PCIEX_DEVCONF_PARAM
{
    INT taskpri[PCIEX_TASK_MAX];                /**< Task priorities */
    INT stksiz[PCIEX_TASK_MAX];                 /**< Stack size */
    INT taskcore[PCIEX_TASK_MAX];               /**< Task core numbers */
#ifdef PCIEX_ENABLE_DMA
    INT dmach[PCIEX_DMACH_NUM];                 /**< DMA channel number */
    INT piorw[DRVCONF_PCIEX_MAX_PIO_RW_NUM];    /**< PIO read/write size */
#endif
    INT intstksz[PCIEX_INT_MAX];                /**< Interrupt stack size */
    INT intlevel[PCIEX_INT_MAX];                /**< Interrupt level */
    INT intcore[PCIEX_INT_MAX];                 /**< Interrupt core number */
    INT maxreqq[PCIEX_NUM_OF_UNIT];             /**< Max queued request*/
    INT shm_size;                               /**< Shared memory size */
    INT shm_max_blocksize;                      /**< Max block size */
    INT reqcmp_tmout;                           /**< Communication timeout */
    INT mempaddr;                               /**< Shared memory start address */
    ID  msgpkt_semid;                           /**< Semaphore ID for message packet */
    ID  shmadmin_semid;                         /**< Semaphore ID for shared memory admin region */
} PCIEX_DEVCONF_PARAM;

#ifdef PCIEX_ENABLE_DMA
/**
 * @typedef PCIEX_DMA_INFO
 * @par DESCRIPTION
 * DMA information.
 */
typedef struct _PCIEX_DMA_INFO
{
    ID semid;                      /**< Semaphore id for DMA use */
    ID evtflg;                     /**< Event flag id for DMA completion */
    struct {
        BOOL is_used;              /**< Flag if this DMA channel is used or not */
        VP allocvadr;              /**< Allocated virtual address of DMA buffer */
        VP alignvadr;              /**< Alligned virtual address of DMA buffer */
        //DMA_channel_t *ch;         /**< DMA channel information */
        //DMA_transfer_t transfer;   /**< DMA transfer information */
    }dma_ch[PCIEX_DMACH_NUM];
} PCIEX_DMA_INFO;
#endif

/**
 * @typedef PCIEX_DRV_INFO_COMMON
 * @par DESCRIPTION
 * Common driver information in units.
 */
typedef struct _PCIEX_DRV_INFO_COMMON
{
    VP pciex_config_base;               /**< Base register address of PCIex */
    PCIEX_MODE mode;                    /**< PCI mode(RootComplex or EndPoint) */
    UINT pciex_int_mode[PCIEX_INT_MAX]; /**< Interrupt mode of PCIex */
    ID mbf_id;                          /**< Message buffer id for notification */
    ID reqcmp_evflg;                    /**< EventFlag id for waiting req completion */
    ID intflgid;                        /**< EventFlag id for interruption */
    TMO reqcmp_tmout;                   /**< Timeout value for waiting req completion */
    UW shm_max_blocksize;               /**< Shared memory block max size */
    UW pciex_int_num[PCIEX_INT_MAX];    /**< PCIex interrupt number */
#ifdef PCIEX_ENABLE_DMA
    PCIEX_DMA_INFO dma;                 /**< DMA channel information */
#endif
} PCIEX_DRV_INFO_COMMON;

/**
 * @typedef PCIEX_DRV_INFO_INDIVIDUAL
 * @par DESCRIPTION
 * Particular driver information in units.
 */
typedef struct _PCIEX_DRV_INFO_INDIVIDUAL
{
    //GDI gdi;                   /**< T-Engine GDI */
    //T_IDEV idev;               /**< T-Engine T_IDEV */
    //T_DEVREQ *p_devreq;        /**< Current processing request */
    PCIEX_STS  drvsts;         /**< Ccurrent processing request */
    ID tskid;                  /**< Main Task id */
    ID semid;                  /**< Semaphore id for request process */
    UW popencnt;               /**< Physical open count */
    UW lopencnt;               /**< Logical  open count */
    UW user_memallocvaddr;     /**< Alloc address for PCI memory region */
    UW user_memvaddr;          /**< PCI memory region start address for user(Shared memory block) */
    UW user_mempaddr;          /**< PCI memory region start address for user(Shared memory block) */
    UW user_memsize;           /**< PCI memory region size for user(Shared memory block) */
    UW admin_memallocvaddr;    /**< Alloc address for PCI memory region */
    UW admin_memvaddr;         /**< PCI memory region start address for admin */
    UW admin_mempaddr;         /**< PCI memory region start address for admin */
    UW admin_memsize;          /**< PCI memory region size for admin */
} PCIEX_DRV_INFO_INDIVIDUAL;

/**
 * @typedef PCIEX_DRV_INFO
 * @par DESCRIPTION
 * Driver information.
 */
typedef struct _PCIEX_DRV_INFO
{
    PCIEX_DEVCONF_PARAM          conf;                             /**< Configiration parameters */
    PCIEX_DRV_INFO_COMMON        common;                           /**< Common information */
    PCIEX_DRV_INFO_INDIVIDUAL    individual[PCIEX_NUM_OF_UNIT];    /**< Individual information */
} PCIEX_DRV_INFO;

/**
 * @typedef PCIEX_UNIT_INFO
 * @par DESCRIPTION
 * Unit information.
 */
typedef struct _PCIEX_UNIT_INFO
{
    INT unit_num;                 /**< Unit number */
    PCIEX_DRV_INFO *p_drvinfo;    /**< Driver information */
} PCIEX_UNIT_INFO;

/*@}*/
/***********************************************************************
 *   functions
 ***********************************************************************/
#ifndef SYSTEM_PROGRAM
#if (PCIEX_COMPILE_TARGET == PCIEX_RC)
ER pciex_rc_main( INT ac, UB *av[] );
#else
ER pciex_ep_main( INT ac, UB *av[] );
#endif /* PCIEX_COMPILE_TARGET */
#endif /* SYSTEM_PROGRAM */
IMPORT void pciex_read_macro_reg8( VP p_address, UB *p_reg_value );
IMPORT void pciex_read_macro_reg16( VP p_address, UH *p_reg_value );
IMPORT void pciex_read_macro_reg32( VP p_address, UW *p_reg_value );
IMPORT void pciex_write_macro_reg8( VP p_address, UB reg_value );
IMPORT void pciex_write_macro_reg16( VP p_address, UH reg_value );
IMPORT void pciex_write_macro_reg32( VP p_address, UW reg_value );

IMPORT UW pciex_adjust_align( UW paddr, UW size );
IMPORT B *pciex_itos( INT n, B *p_s );
IMPORT INT pciex_atoi( const B *p_s );
IMPORT ER pciex_cre_sem1( void );
IMPORT ER pciex_get_configuration( PCIEX_DEVCONF_PARAM *conf );
IMPORT ER pciex_finish_task( ID taskID, BOOL rel_flag );
IMPORT UW pciex_calc_mask( UW val );
IMPORT ER pciex_memcpy( VP dst, VP src, UINT size );
#ifdef PCIEX_ENABLE_DMA
IMPORT VP pciex_cnv_physical_addr( VP vaddr );
#endif

IMPORT PCIEX_UNIT_INFO g_unitinfo[PCIEX_NUM_OF_UNIT];
IMPORT PCIEX_DRV_INFO g_drvinfo;

/***********************************************************************
 *  global variables
 ***********************************************************************/
#endif /* PCIEX_COMMON_H */
