
#ifndef __TK_TYPEDEF_H__
#define __TK_TYPEDEF_H__

#ifdef __cplusplus
extern "C" {
#endif



/*
 * General-purpose data type  
 */
typedef signed char     B;          /* Signed 8 bit integer */
typedef short           H;          /* Signed 16 bit integer */
typedef int             W;          /* Signed 32 bit integer */
typedef unsigned char   UB;         /* Unsigned 8 bit integer */
typedef unsigned short  UH;         /* Unsigned 16 bit integer */
typedef unsigned int    UW;         /* Unsigned 32 bit integer */

typedef signed char     VB;         /* Nonuniform type 8 bit data */
typedef short           VH;         /* Nonuniform type 16 bit data */
typedef int             VW;         /* Nonuniform type 32 bit data */
typedef void            *VP;        /* Nonuniform type data pointer */

typedef volatile B      _B;         /* Volatile statement attached */
typedef volatile H      _H;
typedef volatile W      _W;
typedef volatile UB     _UB;
typedef volatile UH     _UH;
typedef volatile UW     _UW;

typedef int             INT;        /* Processor bit width signed integer */
typedef unsigned int    UINT;       /* Processor bit width unsigned integer */

typedef INT             ID;         /* ID general */
typedef INT             MSEC;       /* Time general (millisecond) */

typedef void            (*FP)();    /* Function address general */
typedef INT             (*FUNCP)(); /* Function address general */

#define LOCAL           static      /* Local symbol definition */
#define EXPORT                      /* Global symbol definition */
#define IMPORT          extern      /* Global symbol reference */

/*
 * Boolean value 
 *  Defined as TRUE = 1, but it is always true when not 0.
 *  Thus, comparison such as bool = TRUE are not permitted.
 *  Should be as per bool !=FALSE.
 */
typedef INT             BOOL;
#define TRUE            1           /* True */
#define FALSE           0           /* False */

/*
 * TRON character code
 */
typedef UH              TC;         /* TRON character code */
#define TNULL           ((TC)0)     /* End of TRON code character string */

/*
 * Data type in which meaning is defined in T-Kernel/OS specification
 */
typedef INT     FN;     /* Function code */
typedef INT     RNO;        /* Rendezvous number */
typedef UINT        ATR;        /* Object/handler attribute */
typedef INT     ER;     /* Error code */
typedef INT     PRI;        /* Priority */
typedef INT     TMO;        /* Time out setting */
typedef UINT        RELTIM;     /* Relative time */

typedef struct systim {         /* System time */
    W   hi;         /* Upper 32 bits */
    UW  lo;         /* Lower 32 bits */
} SYSTIM;

/*
 * Common constant
 */
//#if !defined __NETBSD_LIBC_USE__ && !LFS_SUPPORT
//#define NULL        0       /* Invalid address */
//#endif /* #if !defined __NETBSD_LIBC_USE__ && !LFS_SUPPORT */

#define TA_NULL     0U      /* No special attributes indicated */
#define TMO_POL     0       /* Polling */
#define TMO_FEVR    (-1)        /* Permanent wait */

/* ------------------------------------------------------------------------ */

/*
 * 64 bits value
 */
typedef struct dw {
#if BIGENDIAN
    W   hi; /* Upper 32 bits */
    UW  lo; /* Lower 32 bits */
#else
    UW  lo; /* Lower 32 bits */
    W   hi; /* Upper 32 bits */
#endif
} DW;

#ifdef __cplusplus
}
#endif
#endif /* __TK_TYPEDEF_H__ */
