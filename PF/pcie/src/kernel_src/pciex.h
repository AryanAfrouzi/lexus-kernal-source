/**
 * @file pciex.h
 * @brief Interface definitions.
 * @version $Name: PCIEX_r2_C01 $ (revision $Revision: 1.5 $)
 * @author $Author: madachi $
 * @par copyright
 * (c) 2009 ADIT Corporation
 */

#ifndef PCIEX_H
#define PCIEX_H

#include "pciex_types.h" //[kd]

/***********************************************************************
 *   definitions
 ***********************************************************************/
/**
 * @addtogroup InterfaceDefinitions
 */

/*@{*/
/** Number of subunits of PCIex driver */
#define PCIEX_SHMSUBUNITNM          255

/** Maximum number of shared memory block */
#define PCIEX_SHMBLOCKNUM           (PCIEX_SHMSUBUNITNM -1)


/* Event flag */
/** event flag wait pattern for notification (all) */
#define PCIEX_NOTIFY_EVENT_PATTERN   0xff

/** event flag wait pattern for notification (renew) */
#define PCIEX_NOTIFY_EVENT_RENEW     0x01

/** event flag wait pattern for notification (bus error) */
#define PCIEX_NOTIFY_EVENT_BSERR     0x02


/** event flag wait pattern for completition (complete) */
#define PCIEX_CMP_WAIT    0x00000001

/** event flag wait pattern for completition (abort) */
#define PCIEX_CMP_ABORT   0x00000002

/** event flag wait pattern for exit ack */
#define PCIEX_EXIT_ACK    0x00000004

/* MSI */
/** size of MSI region */
#define PCIEX_MSI_RCVWIN_SIZE       32

/** number of MSI */
#define PCIEX_MSI_ARRAY_NUM         (PCIEX_MSI_RCVWIN_SIZE / sizeof(UW))

/** MSI message data (request) */
#define PESHM_SHM_REQ     0x0001

/** MSI message data (complition) */
#define PESHM_SHM_REQCMP  0x0002

/** MSI message data (direct MSI message) */
#define PESHM_DIRECT_MSI  0x0003

/** MSI message data (notify exit) */
#define PESHM_NOTIFY_EXIT 0x0004

/** MSI message data (ack of notify exit) */
#define PESHM_NOTIFY_EXIT_ACK 0x0005

/** MSI message data (driver ready) */
#define PESHM_DRIVER_READY 0x0006

/** number of user to attach one shared memory block */
/*@}*/
#define NUM_OF_USR 32

/***********************************************************************
 *   constants
 ***********************************************************************/
/**
 * @addtogroup InterfaceEnums
 */

/*@{*/

/**
 * @typedef PCIEX_ATTR_DATA
 * @par DESCRIPTION
 * Attributes.
 */
typedef enum _PCIEX_ATTR_DATA
{
     DN_GET_SHM_INFO        = -200    /**< (R) Get shared memory information */
    ,DN_GET_RENEW_INFO      = -202    /**< (R) Get renew information */

    ,DN_SHM_CREATE          = -300    /**< (W) Create  shared memory block */
    ,DN_SHM_RELEASE         = -301    /**< (W) Release shared memory block */
    ,DN_SHM_ATTACH          = -302    /**< (W) Attach  shared memory block */
    ,DN_SHM_DETACH          = -303    /**< (W) Detach  shared memory block */
    ,DN_SHM_LOCK            = -304    /**< (W) Lock    shared memory block */
    ,DN_SHM_UNLOCK          = -305    /**< (W) Unlock  shared memory block */

    ,DN_GET_COMM_TIMEOUT    = -400    /**< (R) Get timeout value for waiting completion */
    ,DN_SET_COMM_TIMEOUT    = -401    /**< (W) Set timeout value for waiting completion */
    ,DN_REG_NOTIFY_ID       = -402    /**< (W) Register message buffer ID for notification */
    ,DN_DIRECT_MSI          = -403    /**< (W) Request data sending via MSI */
    ,DN_RECONFIG            = -404    /**< (W) Request reconfiguration */
} PCIEX_ATTR_DATA;

/**
 * @typedef PCIEX_TARGET
 * @par DESCRIPTION
 * Target board to issue command.
 */
typedef enum _PCIEX_TARGET
{
    PCIEX_TARGET_SELF = 0     /**< Self side */
   ,PCIEX_TARGET_PEER         /**< Opposite side */
} PCIEX_TARGET;

/**
 * @typedef PCIEX_MBF_MSG_TYPE
 * @par DESCRIPTION
 * Message type for notification.
 */
typedef enum _PCIEX_MBF_MSG_TYPE
{
    PCIEX_CONFIG_COMP = 0    /**< The result of configuration */
   ,PCIEX_DIRECT_MSI         /**< Direct MSI message */
   ,PCIEX_BUS_ERROR          /**< PCI bus error */
   ,PCIEX_EXIT_OF_PEER       /**< Notification of the termination of the peer */
   ,PCIEX_FAIL_ALLOCATE      /**< Notification of allocate fail for the shared memory */
} PCIEX_MBF_MSG_TYPE;

/**
 * @typedef PCIEX_MESSAGE_TYPE
 * @par DESCRIPTION
 * Message type for notification.
 */
/*@}*/
typedef enum _PCIEX_MESSAGE_TYPE
{
    PESHM_MSG_NONE = 0
   ,PESHM_MSG_CREATE          /**< Create  shared memory block */
   ,PESHM_MSG_RELEASE         /**< Release shared memory block */
   ,PESHM_MSG_ATTACH          /**< Attach  shared memory block */
   ,PESHM_MSG_DETACH          /**< Detach  shared memory block */
   ,PESHM_MSG_LOCK            /**< Lock    shared memory block */
   ,PESHM_MSG_UNLOCK          /**< Unlock  shared memory block */
   ,PESHM_MSG_SHMRENEW        /**< Request to issue renew event */
   ,PESHM_MSG_GET_SHM_INFO    /**< Get shared memory information */
} PCIEX_MESSAGE_TYPE;

/***********************************************************************
 *   structures
 ***********************************************************************/
/**
 * @addtogroup InterfaceStructs
 */
/*@{*/

/**
 * @typedef PCIEX_SHM_ID
 * @par DESCRIPTION
 * Structure to identify shared memory block.
 */
typedef struct _PCIEX_SHM_ID
{
    ID           shm_id;    /**< Shared memory id */
    PCIEX_TARGET target;    /**< Target wheare the shared memory block is allocated */
} PCIEX_SHM_ID;

/**
 * @typedef PCIEX_USRINFO
 * @par DESCRIPTION
 * User information to attach shared memory block.
 */
typedef struct _PCIEX_USRINFO
{
    UW           uid;          /**< User id */
    ID           event_flg;    /**< Event flag id for notification */
    PCIEX_TARGET target;       /**< Target where the user exists */
} PCIEX_USRINFO;

/**
 * @typedef PCIEX_SHM
 * @par DESCRIPTION
 * Structure for shared memory block management.
 */
typedef struct _PCIEX_SHM
{
    ID      shm_id;                         /**< Shared memory id */
    UW      base_address;                   /**< Start address of the shared memory block */
    UW      size;                           /**< Size of the the shared memory block */
    UW      num_of_ref;                     /**< The number of references */
    ID      semid;                          /**< Semaphore id for this structure */
    BOOL    is_allocated;                   /**< Flag if this shared memory block is created or not */
    BOOL    is_locked;                      /**< Flag if this shared memory block is locked  or not */
    ID      lock_semid;                     /**< Semaphore for the lock mechanism */
    UW      lock_uid;                       /**< User id of that locked    this shared memory block */
    UW      alloc_uid;                      /**< User id of that allocated this shared memory block */
    PCIEX_USRINFO user_info[NUM_OF_USR];    /**< User information to attach this shared memory block */
} PCIEX_SHM;

/**
 * @typedef PCIEX_SHM_REQ
 * @par DESCRIPTION
 * Structure to request Create/Release/Attach/Detach.
 */
typedef struct _PCIEX_SHM_REQ
{
    PCIEX_SHM_ID st_shm_id;            /**< Shared memory id */
    B       device_name[3];    /**< Device name to access the shared memory block */
    UW      address;                   /**< Start address of the shared memory block */
    UW      size;                      /**< Size of the the shared memory block */
    UW      user_id;                   /**< User ID */
    ID      event_flg;                 /**< Event flag id for notification */
} PCIEX_SHM_REQ;

/**
 * @typedef PCIEX_LOCK_REQ
 * @par DESCRIPTION
 * Structure to request Lock/Unlock.
 */
typedef struct _PCIEX_LOCK_REQ
{
    PCIEX_SHM_ID st_shm_id;    /**< Shared memory id */
    UW user_id;                /**< User ID */
} PCIEX_LOCK_REQ;

/**
 * @typedef PCIEX_SHM_INFO
 * @par DESCRIPTION
 * Structure to get shared memory information.
 */
typedef struct _PCIEX_SHM_INFO
{
    PCIEX_TARGET target;    /**< Target to request */
    UW address;             /**< Start address of the shared memory */
    UW remaining_size;      /**< Remaining size of the shared memory */
} PCIEX_SHM_INFO;

/**
 * @typedef PCIEX_MSG_CONFIG_COMP
 * @par DESCRIPTION
 * Message structure to notify configuration result.
 */
typedef struct _PCIEX_MSG_CONFIG_COMP
{
    ER result;    /**< The result of configuration */
} PCIEX_MSG_CONFIG_COMP;

/**
 * @typedef PCIEX_MSG_ERROR_RES
 * @par DESCRIPTION
 * Message structure to notify error response.
 */
typedef struct _PCIEX_MSG_ERROR_RES
{
    ER result;    /**< The code of error */
} PCIEX_MSG_ERROR_RES;

/**
 * @typedef PCIEX_MSG_DIRECT_MSI
 * @par DESCRIPTION
 * Message structure to notify direct MSI message.
 */
typedef struct _PCIEX_MSG_DIRECT_MSI
{
    UB direct_msi;    /**< Direct MSI message */
} PCIEX_MSG_DIRECT_MSI;

/**
 * @typedef PCIEX_MBF_MSG
 * @par DESCRIPTION
 * Message structure for notification.
 */
typedef struct _PCIEX_MBF_MSG
{
    PCIEX_MBF_MSG_TYPE msg_type;             /**< Message type */
    union _msg_data
    {
        PCIEX_MSG_CONFIG_COMP   cfgcmp;      /**< The result of configuration */
        PCIEX_MSG_ERROR_RES     errres;      /**< The result of error */
        PCIEX_MSG_DIRECT_MSI    direct_msi;  /**< Direct MSI message */
    } msg_data;
} PCIEX_MBF_MSG;

/**
 * @typedef PCIEX_MSGD_CREATE
 * @par DESCRIPTION
 * Message packet structure to create shared memory block.
 */
typedef struct _PCIEX_MSGD_CREATE        /* PESHM_MSG_CREATE */
{
    PCIEX_SHM_REQ shm_req;    /**< arguments */
    ER ret;                   /**< return code */
} PCIEX_MSGD_CREATE;

/**
 * @typedef PCIEX_MSGD_RELEASE
 * @par DESCRIPTION
 * Message packet structure to release shared memory block.
 */
typedef struct _PCIEX_MSGD_RELEASE       /* PESHM_MSG_RELEASE */
{
    PCIEX_SHM_REQ shm_req;    /**< arguments */
    ER ret;                   /**< return code */
} PCIEX_MSGD_RELEASE;

/**
 * @typedef PCIEX_MSGD_ATTACH
 * @par DESCRIPTION
 * Message packet structure to attach shared memory block.
 */
typedef struct _PCIEX_MSGD_ATTACH        /* PESHM_MSG_ATTACH */
{
    PCIEX_SHM_REQ shm_req;    /**< arguments */
    ER ret;                   /**< return code */
} PCIEX_MSGD_ATTACH;

/**
 * @typedef PCIEX_MSGD_DETACH
 * @par DESCRIPTION
 * Message packet structure to detach shared memory block.
 */
typedef struct _PCIEX_MSGD_DETACH        /* PESHM_MSG_DETACH */
{
    PCIEX_SHM_REQ shm_req;    /**< arguments */
    ER ret;                   /**< return code */
} PCIEX_MSGD_DETACH;

/**
 * @typedef PCIEX_MSGD_LOCK
 * @par DESCRIPTION
 * Message packet structure to lock shared memory block.
 */
typedef struct _PCIEX_MSGD_LOCK          /* PESHM_MSG_LOCK */
{
    PCIEX_LOCK_REQ lock_req;    /**< arguments */
    ER ret;                     /**< return code */
} PCIEX_MSGD_LOCK;

/**
 * @typedef PCIEX_MSGD_UNLOCK
 * @par DESCRIPTION
 * Message packet structure to unlock shared memory block.
 */
typedef struct _PCIEX_MSGD_UNLOCK        /* PESHM_MSG_UNLOCK */
{
    PCIEX_LOCK_REQ unlock_req;    /**< arguments */
    ER ret;                       /**< return code */
} PCIEX_MSGD_UNLOCK;

/**
 * @typedef PCIEX_MSGD_GETSHM_INFO
 * @par DESCRIPTION
 * Message packet structure to get shared memory information.
 */
typedef struct _PCIEX_MSGD_GETSHM_INFO   /* PESHM_MSG_GET_SHM_INFO */
{
    PCIEX_SHM_INFO getshm_info_req;    /**< arguments */
    ER ret;                            /**< return code */
} PCIEX_MSGD_GETSHM_INFO;

/**
 * @typedef PCIEX_MSGD_DIRECT_MSI
 * @par DESCRIPTION
 * Message packet structure to get the result of direct MSI.
 */
typedef struct _PCIEX_MSGD_DIRECT_MSI    /* DN_DIRECT_MSI */
{
    ER ret;    /**< arguments */
} PCIEX_MSGD_DIRECT_MSI;

/**
 * @typedef PCIEX_MSGD_SHMRENEW
 * @par DESCRIPTION
 * Message packet structure to issue renew event.
 */
/*@}*/
typedef struct _PCIEX_MSGD_SHMRENEW      /* PESHM_MSG_SHMRENEW */
{
    PCIEX_SHM_ID st_shm_id;    /**< arguments */
    ER ret;                    /**< return code */
} PCIEX_MSGD_SHMRENEW;

/**
 * @addtogroup InterfaceUnions
 */
/*@{*/
/**
 * @typedef PCIEX_MESSAGE_DATA
 * @par DESCRIPTION
 * Message packet structure.
 */
/*@}*/
typedef union _PCIEX_MESSAGE_DATA
{
    PCIEX_MSGD_CREATE           create;         /**< Create  shared memory block */
    PCIEX_MSGD_RELEASE          release;        /**< Release shared memory block */
    PCIEX_MSGD_ATTACH           attach;         /**< Attach  shared memory block */
    PCIEX_MSGD_DETACH           detach;         /**< Detach  shared memory block */
    PCIEX_MSGD_LOCK             lock;           /**< Lock    shared memory block */
    PCIEX_MSGD_UNLOCK           unlock;         /**< Unlock  shared memory block */
    PCIEX_MSGD_GETSHM_INFO      getshm_info;    /**< Get shared memory information */
    PCIEX_MSGD_DIRECT_MSI       direct_msi;     /**< Direct MSI message */
    PCIEX_MSGD_SHMRENEW         renew;          /**< Request to issue renew event */
} PCIEX_MESSAGE_DATA;

/**
 * @addtogroup InterfaceStructs
 */
/*@{*/
/**
 * @typedef PCIEX_MSGPACKET
 * @par DESCRIPTION
 * Message Packet between RC and EP.
 */
typedef struct _PCIEX_MSGPACKET
{
    ID semid;                      /**< Semaphoe id for this region */
    PCIEX_MESSAGE_TYPE msgtype;    /**< Kind of message */
    PCIEX_MESSAGE_DATA msgdata;    /**< Request message */
} PCIEX_MSGPACKET;

/**
 * @typedef PCIEX_RENEW_INFO
 * @par DESCRIPTION
 * Structure to get renew information.
 */
typedef struct _PCIEX_RENEW_INFO
{
    UW                  *msi;      /**< pointer of msi            */
    PCIEX_MSGPACKET     *msg;      /**< pointer of message packet */
    ID                  flg_id;    /**< evnet flag ID to wait the result */
    TMO                 tmout;     /**< time out value to wait result */
} PCIEX_RENEW_INFO;

/**
 * @typedef PCIEX_SHMADMIN
 * @par DESCRIPTION
 * Structure to manage shared memory.
 */
typedef struct _PCIEX_SHMADMIN
{
    ID semid;                            /**< Semaphoe id for this region */
    INT last;                            /**< Shared memory block number to be last created */
    UW  user_id;                         /**< User id to be issued next */
    UW  start_address;                   /**< Start address of the shared memory */
    PCIEX_SHM shm[PCIEX_SHMBLOCKNUM];    /**< Data for shared memory block management */
} PCIEX_SHMADMIN;

/**
 * @typedef PCIEX_ADMIN_REGION
 * @par DESCRIPTION
 * Structure of all memory.
 */
/*@}*/
typedef struct _PCIEX_ADMIN_REGION
{
    UW msi[PCIEX_MSI_ARRAY_NUM];    /**< MSI region */
    PCIEX_SHMADMIN    shmadmin;     /**< Shared memory management region */
    PCIEX_MSGPACKET   msgpkt;       /**< Message packet region */
} PCIEX_ADMIN_REGION;

/***********************************************************************
 *  functions
 ***********************************************************************/

/***********************************************************************
 *  global variables
 ***********************************************************************/
#endif /*  PCIEX_H */
