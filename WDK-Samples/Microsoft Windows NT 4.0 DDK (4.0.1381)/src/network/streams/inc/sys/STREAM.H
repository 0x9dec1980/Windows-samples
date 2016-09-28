/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stream.h

Abstract:

    This module declares the data types and functions available to
    STREAMS drivers and modules.

Author:

    Eric Chin (ericc)           August 9, 1991

Revision History:

    Who          When          What
    --------   --------   -----------------------------------------------------
    sampa      09-19-91   pulled kernel level stuff from stream.h
    mikemas    01-18-92   removed redefinition of STREAMS functions and
                          reorganized file.
    mikemas    01-20-92   integrated private\inc\stream.h into this file.
                          moved to streams\inc\stream.h from ntstream.h

--*/

/******************************************************************
 *
 *  SpiderSTREAMS - STREAMS Package
 *
 *  Copyright 1987  Spider Systems Limited
 *
 *  STREAM.H
 *
 *    Main STREAMS definitions
 *
 ******************************************************************/

/*
 *   /usr/projects/spare/PBRAIN/SCCS/pbrainG/dev/stremul/include/sys/114/s.stream.h
 *  @(#)stream.h    1.36
 *
 *  Last delta created  12:37:24 12/4/91
 *  This file extracted 16:45:56 12/23/91
 *
 *  Modifications:
 *
 */


#ifndef _SYS_STREAM_INCLUDED
#define _SYS_STREAM_INCLUDED



/********************************************************/
/*          Streams message and queue types     */
/********************************************************/

/*
 * Message block descriptor
 */
struct msgb {
    struct msgb    *b_next;     /* next message on queue */
    struct msgb    *b_prev;     /* previous message on queue */
    struct msgb    *b_cont;     /* next message block of message */
    unsigned char  *b_rptr;     /* first unread data byte in buffer */
    unsigned char  *b_wptr;     /* first unwritten data byte in buffer */
    struct datab   *b_datap;    /* data block */
    unsigned char   b_rsvd;     /* in SVR4, this is b_band */
    unsigned char   b_pad1;
    unsigned short  b_flag;     /* message flag field */
    long            b_pad2;
};


/*
 * Message flags.  These are interpreted by the Stream Head.
 */
#define MSGMARK     0x01        /* marked block - for I_ATMARK */
#define MSGNLOOP    0x02        /* don't loop message to write queue */
#define MSGDELIM    0x04        /* message is delimited */


/*
 * structure containing function to call and argument when freeing
 * an extended buffer
 */
struct free_rtn {
    void (*free_func)(char *);  /* free routine of user */
    char *free_arg;             /* argument to free routine */
};


/*
 *  Data block descriptor
 *
 */
struct datab {
    struct datab   *db_freep;   /* used internally */
    unsigned char  *db_base;    /* first byte of buffer */
    unsigned char  *db_lim;     /* last byte + 1 of buffer */
    unsigned long   db_ref;     /* count of msgs pointing to block */
    unsigned char   db_type;    /* message type */
    unsigned char   db_class;   /* used internally. In SVR4, is db_iswhat */
    KSPIN_LOCK      db_lock;    /* protects db_ref */

                                /* unused SVR4 fields, for future expansion */
    unsigned int    db_rsvd1;   /* db_size */
    char           *db_rsvd2;   /* db_msgaddr */
    long            db_filler;  /* reserved for future use */

                                /* unique to SpiderSTREAMS */
    struct free_rtn db_discard; /* call routine & arg to free buffer */
};


struct queue;    // define queue name so the MIPS compiler will recognize it
                 // in the following definition as being the same as the
                 // queue structure definition that will follow later.


/*
 * Queue Information Structure
 *
 */
struct qinit {                                  /* declares a queue */
    int               (*qi_putp)(               /*  put procedure */
                            struct queue *q,
                            struct msgb *mp
                        );
    int               (*qi_srvp)(               /*  service procedure */
                            struct queue *
                        );
    int               (*qi_qopen)(              /*  open procedure */
                            struct queue *,
                            dev_t *,
                            int,
                            int,
                            void *
                        );
    int               (*qi_qclose)(             /*  close procedure */
                            struct queue *,
                            int,
                            void *
                        );
    int               (*qi_qadmin)();           /*  unused */
    struct module_info *qi_minfo;               /*  module information */
    struct module_stat *qi_mstat;               /*  module statistics */
    int                 qi_mplevel;             /*  level of parallelism */
};


#define FULL_PARALLEL   -1      /* fully parallelised */
#define STREAM_PARALLEL -2      /* streams parallel: NOT SUPPORTED */


/*
 * Module information structure
 */
struct module_info {
    unsigned short  mi_idnum;   /* module id number */
    char           *mi_idname;  /* module name */
    long            mi_minpsz;  /* min packet size accepted */
    long            mi_maxpsz;  /* max packet size accepted */
    unsigned long   mi_hiwat;   /* hi-water mark */
    unsigned long   mi_lowat;   /* lo-water mark */
};


/*
 * Module statistics structure (not used in current implementation)
 */
struct module_stat {
    long    ms_pcnt;      /* count of calls to put proc */
    long    ms_scnt;      /* count of calls to service proc */
    long    ms_ocnt;      /* count of calls to open proc */
    long    ms_ccnt;      /* count of calls to close proc */
    long    ms_acnt;      /* count of calls to admin proc */
    char   *ms_xptr;      /* pointer to private statistics */
    short   ms_xsize;     /* length of private statistics buffer */
};


/*
 * Streamtab (used in cdevsw and fmodsw to point to module or driver)
 */
struct streamtab {
    struct qinit *st_rdinit;   /* defines read QUEUE */
    struct qinit *st_wrinit;   /* defines write QUEUE */
    struct qinit *st_muxrinit; /* for multiplexing drivers only */
    struct qinit *st_muxwinit; /* for multiplexing drivers only */
};


struct mpctl;                                   /* for ANSI c */


/*
 * data queue
 */
struct queue {                                  /* internal representation */
    struct queue       *q_next;                 /*  next in stream */
    struct qinit       *q_qinfo;                /*  queue procedures, limits */
    struct msgb        *q_first;                /*  first data block */
    struct msgb        *q_last;                 /*  last data block */
    struct queue       *q_link;                 /*  next queue for scheduling */
    char               *q_ptr;                  /*  private data */
    unsigned long       q_count;                /*  bytes of data on queue */
    unsigned long       q_flag;                 /*  queue state */
    long                q_minpsz;               /*  min packet size accepted */
    long                q_maxpsz;               /*  max packet size accepted */
    unsigned long       q_hiwat;                /*  queue high water mark */
    unsigned long       q_lowat;                /*  queue low water mark */

                                                /*  unique to SpiderSTREAMS */
    struct queue       *q_other;                /*  other queue of pair */
    struct queue       *q_can;                  /*  queue canput() looks at */
    struct queue       *q_back;                 /*  queue to back-enable */
    struct mpctl       *q_mpctl;                /*  multiprocessor queue info */
    int                 q_active;               /*  # srv/put procs running */
    KEVENT              q_disableproc;          /*  for disable_procs() */
    char               *q_strmptr;              /*  at stream head only */

                                                /*  unused SVR4 fields */
    void               *q_rsvd1;                /*  q_bandp */
    unsigned char       q_rsvd2;                /*  q_nband */
    unsigned char       q_pad1[3];              /*  reserved for future use */
    long                q_pad2[2];              /*  reserved for future use */
};


/*
 * Typedefs for important structures
 */
typedef struct msgb   mblk_t;
typedef struct datab  dblk_t;
typedef struct free_rtn frtn_t;
typedef struct queue  queue_t;


/*
 * Priority definitions for block allocation.
 */
#define NPRI        3   /* Number of priority values */

#define BPRI_LO     0
#define BPRI_MED    1
#define BPRI_HI     2


/*
 * Queue flags
 */
#define QSTATE      0x000F    /* Queue state mask (see below for values) */
#define QREADR      0x0010    /* This is the reader (first) Q */
#define QSERV       0x0020    /* This queue has a service proc */
#define QEND        0x0040    /* This queue is this stream side end */
#define QENABL      0x0100    /* Queue is already enabled to run */
#define QNOENB      0x0200    /* Don't enable Q via putq */
#define QUSE        0x0400    /* Queue is used */
#define QDISABL     0x0800    /* Disable procs can be called */

/*
 * Queue state values (in LS 4 bits of flags)
 */
#define QPRIMED     0x0000    /* Queue was read when empty */
#define QEMPTY      0x0001    /* Queue empty but not primed */
#define QACTIVE     0x0002    /* Queue not empty and not blocked */
#define QBLOCKED    0x0003    /* Queue blocked (canput failed) */
#define QFULL       QBLOCKED  /* Just so it's defined */

/*
 * Finding related queues
 */
#define OTHERQ(Q)   ((Q)->q_other)
#define WR(Q)       ((Q)->q_other)
#define RD(Q)       ((Q)->q_other)


/*
 *  IOCTL structure - this structure is the format of the M_IOCTL message type.
 *
 */
struct iocblk {
    int             ioc_cmd;        /* ioctl command type */
    unsigned short  ioc_uid;        /* effective uid of user { *ioc_cr } */
    unsigned short  ioc_gid;        /* effective gid of user { in SVR4 } */
    unsigned int    ioc_id;         /* ioctl id */
    unsigned int    ioc_count;      /* count of bytes in data field */
    int             ioc_error;      /* error code */
    int             ioc_rval;       /* return value  */
};
                                    /* unprovided SVR4 fields */
                                    /* ioc_filler[4] */


/*
 * Link structure for I_LINK and I_UNLINK ioctl's
 *
 */
struct linkblk {
    queue_t *l_qtop;   /* lowest level write queue of upper stream */
    queue_t *l_qbot;   /* highest level write queue of lower stream */
    int      l_index;  /* link index for lower stream */
};
                       /* unprovided SVR4 fields */
                       /* l_pad[5] */


/*
 * Options structure for M_SETOPTS message.  This is sent upstream
 * by driver to set stream head options.
 */
struct stroptions {
    unsigned long   so_flags;      /* options to set */
    short           so_readopt;    /* read option */
    unsigned short  so_wroff;      /* write offset */
    long            so_minpsz;     /* minimum read packet size */
    long            so_maxpsz;     /* maximum read packet size */
    unsigned long   so_hiwat;      /* read queue high water mark */
    unsigned long   so_lowat;      /* read queue low water mark */
    unsigned char   so_band;       /* update water marks for this band */
};


/*
 * enumeration for strqset/strqget
 */
typedef enum qfields {
    QHIWAT = 0,         /* q_hiwat or qb_hiwat */
    QLOWAT = 1,         /* q_lowat or qb_lowat */
    QMAXPSZ = 2,        /* q_maxpsz */
    QMINPSZ = 3,        /* q_minpsz */
    QCOUNT = 4,         /* q_count or qb_count */
    QFIRST = 5,         /* q_first or qb_first */
    QLAST = 6,          /* q_last or qb_last */
    QFLAG = 7,          /* q_flag or qb_flag */
    QBAD = 8
} qfields_t;


/*
 * Flags for stream options set message
 */
#define SO_ALL      077     /* set all options */
#define SO_READOPT  01      /* set read option */
#define SO_WROFF    02      /* set write offset */
#define SO_MINPSZ   04      /* set min packet size */
#define SO_MAXPSZ   010     /* set max packet size */
#define SO_HIWAT    020     /* set high water mark */
#define SO_LOWAT    040     /* set low water mark */
#define SO_MREADON  0100    /* enable M_READ generation */
#define SO_MREADOFF 0200    /* disable M_READ generation */
#define SO_NDELON   0400    /* select non-STREAMS tty O_NDELAY semantics */
#define SO_NDELOFF  01000   /* select STREAMS O_NDELAY semantics */
#define SO_BAND     02000   /* set water marks in a band (unsupported) */
#define SO_ISTTY    04000   /* acting as a controlling terminal */
#define SO_ISNTTY   010000  /* not acting as a controlling terminal */
#define SO_TOSTOP   020000  /* stop on background writes (unsupported) */
#define SO_TONSTOP  040000  /* no stop on background writes (unsupported) */


/*
 * SpiderStreams Specific flag - to enable the sending down of an
 * I_CLOSE ioctl just before calling a drivers close routine
 */
#define SO_I_CLOSE  0100000 /* send I_CLOSE */


/*
 * Data and protocol messages (regular priority)
 */
#define M_DATA      00  /* regular data */
#define M_PROTO     01  /* protocol control */
#define M_DELAY     014 /* real-time xmit delay (1 param) */


/*
 * Control messages (regular priority)
 */
#define M_BREAK     010 /* line break */
#define M_PASSFP    011 /* pass file pointer */
#define M_SIG       013 /* generate process signal */
#define M_CTL       015 /* device-specific control message */
#define M_IOCTL     016 /* ioctl; set/get params */
#define M_SETOPTS   020 /* set various stream head options */
#define M_RSE       021 /* reserved for internal use */


/*
 * Control messages (high priority; go to head of queue)
 */
#define M_IOCACK    0201    /* acknowledge ioctl */
#define M_IOCNAK    0202    /* negative ioctl acknowledge */
#define M_PCPROTO   0203    /* priority proto message */
#define M_PCSIG     0204    /* generate process signal */
#define M_FLUSH     0206    /* flush your queues */
#define M_STOP      0207    /* stop transmission immediately */
#define M_START     0210    /* restart transmission after stop */
#define M_HANGUP    0211    /* line disconnect */
#define M_ERROR     0212    /* fatal error used to set u.u_error */
#define M_READ      0213    /* sent on read call, if no messages waiting */
#define M_STOPI     0214    /* stop input request */
#define M_STARTI    0215    /* start input request */
#define M_PCRSE     0216    /* reserved for internel use */


/*
 * Queue message class definitions.
 */
#define QNORM       0       /* normal messages */
#define QPCTL       0200    /* priority cntrl messages */


/*
 * Define for 2-byte M_ERROR message
 */
#define NOERROR     255

/************************************************************/
/*        Miscellaneous parameters and flags            */
/************************************************************/

/*
 * Stream head default high/low water marks
 */

#define STRHIGH     5120
#define STRLOW      1024


/*
 * Block allocation parameters
 */
#define QBSIZE      65      /* min size for block allocation retries */
#define MAXBSIZE    4096    /* max block size */
#define MAXIOCBSZ   1024    /* max ioctl data block size */


/*
 * Values for stream flag in open to indicate module open, clone open;
 * return value for failure.
 */
#define MODOPEN     1   /* open as a module */
#define CLONEOPEN   2   /* open for clone, pick own minor device */
#define OPENFAIL    -1  /* returned for open failure */


/*
 * Value for packet size that denotes infinity
 */
#define INFPSZ      -1


/*
 * Flags for flushq()
 */
#define FLUSHALL    1   /* flush all messages */
#define FLUSHDATA   0   /* don't flush control messages */


/*
 * Default ioctl/close timeout (seconds)
 */

#define STRTIMOUT   15


/************************************************************************/
/*    Definitions of Streams macros and function interfaces.            */
/************************************************************************/

/*
 * extract queue class of message block
 */
#define queclass(bp)    (bp->b_datap->db_type & QPCTL)


/*
 * Align address on next lower word boundary
 */
#define straln(a)   (char *)((long)(a) & ~(sizeof(int)-1))



/*
 * NT-specific definitions for STREAMS Drivers
 */
typedef struct {
    KSPIN_LOCK    spinlock;
    KIRQL         oldlevel;
} lock_t;


/*
 *  STREAMS <-> TDI interface variable types
 */

#define SHTDI_ADDRESS_COMPARE_BIND    0
#define SHTDI_ADDRESS_COMPARE_RECEIVE 1

typedef struct {
    long ADDR_length;           // length of broadcast address for this type
    USHORT AddressType;         // Tdi address type
    BOOLEAN DirectedRouted;     // TRUE if a directed message will go farther
                                // than a broadcast or multicast message.
    PVOID ADDR_ptr;             // pointer to broadcast address.  NULL if there
                                // isn't one.
    BOOLEAN (*AddressCompare)(char *, int, char *, int, int);
                                // Function to call to compare two addresses to
                                // see if the provider treats them as equal.
                                // returns TRUE if the addresses are equal.
} STREAMS_TDI_ADDRESS_INFO, *PSTREAMS_TDI_ADDRESS_INFO;

typedef struct {
    long SERV_type;
    struct streamtab * Partner;
    long PSERV_type;
    long OPT_length;            // length of option string to pass to turn
                                // on TO_REUSE_ADDR
    PVOID OPT_ptr;              // ptr to option string
    BOOLEAN ReuseAddr;          // TRUE if TO_REUSE_ADDR is on by default
    int AddressInfoCount;       // Number of address info structures in the
                                // AddressInfo array.

    PSTREAMS_TDI_ADDRESS_INFO AddressInfo;
                                // Pointer to the AddressInfo array.  There is
                                // one for each address type the provider
                                // supports.

} STREAMS_TDI_INFO, *PSTREAMS_TDI_INFO;



/*
 * Definitions for Unix system call and kernel variable emulation
 *
 */

//
// In SVR4, HZ is defined in <sys/param.h> and lbolt declared in <sys\systm.h>.
//
// However, on NT, we define it here since the value of HZ is intimately tied
// to the function, StrmQuerySecondsSince1970Time().
//
#define HZ        100
#define lbolt     StrmQueryLbolt()



#define bcopy(src, dest, bcount)    RtlMoveMemory(          \
                                        (PVOID) (dest),     \
                                        (PVOID) (src),      \
                                        (ULONG) (bcount)    \
                                        )

#define bzero(addr, bcount)         RtlZeroMemory(          \
                                        (PVOID) (addr),     \
                                        (ULONG) (bcount)    \
                                        )


//
// SAMESTR() is not defined in the SVR4 STREAMS Programmer's Guide, but is
// a macro in SVR4's <sys/streams.h>.  In SpiderSTREAMS it is a function.
//
int
SAMESTR(
    IN queue_t *q
    );


/*
 *  Definitions for the STREAMS error logging facility
 */
#include <ntiologc.h>

    /*
     *  Maximum amount of data (binary dump data plus insertion strings) that
     *  can be added to an error log entry.
     */
#define STRM_MAX_ERROR_LOG_DATA_SIZE     \
    ( (ERROR_LOG_MAXIMUM_SIZE - sizeof(IO_ERROR_LOG_PACKET) + 4) & 0xFFFFFFFC )



//
// NT STREAMS Extensions to the Regular SVR4 STREAMS Utilities
//

typedef struct _U_UERROR {
    LIST_ENTRY p_list;
    struct _KTHREAD * ThreadHandle;
    char u_error;

} U_UERROR, *PU_UERROR;


LARGE_INTEGER
StrmConvertCentisecondsToRelativeTimeout(
    unsigned long Centiseconds
    );


NTSTATUS
StrmDeregisterDriver(
    IN PDRIVER_OBJECT driverobject,
    IN struct streamtab *streamtab
    );

NTSTATUS
StrmDeregisterModule(
    IN PDRIVER_OBJECT driverobject,
    IN struct streamtab *streamtab
    );

dev_t
StrmFindDevno(
    IN struct streamtab *stab
    );

char
StrmGetError(
    void
    );

time_t
StrmQueryLbolt(
    void
    );

time_t
StrmQuerySecondsSince1970Time(
    void
    );

NTSTATUS
StrmRegisterDriver(
    IN PDRIVER_OBJECT driverobject,
    IN struct streamtab *streamtab,
    IN char *subysname OPTIONAL,
    IN PSTREAMS_TDI_INFO ptdiinfo OPTIONAL
    );

NTSTATUS
StrmRegisterModule(
    IN PDRIVER_OBJECT driverobject,
    IN struct streamtab *streamtab,
    IN char *subysname OPTIONAL
    );

VOID
StrmSetError(
    char
    );

int
suser(
    void
    );

NTSTATUS
StrmLogEvent(
    IN PDRIVER_OBJECT DriverObject OPTIONAL,
    IN NTSTATUS EventCode,
    IN ULONG UniqueEventValue,
    IN USHORT NumStrings,
    IN PCHAR *Strings OPTIONAL,
    IN ULONG DataSize,
    IN PVOID Data OPTIONAL
    );

NTSTATUS
StrmWaitForMultipleObjects(
    IN queue_t *Queue,
    IN CCHAR Count,
    IN PVOID Object[],
    IN WAIT_TYPE WaitType,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    IN PKWAIT_BLOCK WaitBlockArray OPTIONAL
    );

NTSTATUS
StrmWaitForMutexObject(
    IN queue_t *Queue,
    IN PKMUTEX Mutex,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

NTSTATUS
StrmWaitForSingleObject(
    IN queue_t *Queue,
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );


NTSTATUS
StrmOpenRegKey(
    PHANDLE HandlePtr,
    PUCHAR KeyName
    );

NTSTATUS
StrmGetRegValue(
    HANDLE KeyHandle,
    PUCHAR ValueName,
    PUCHAR ValueData,
    ULONG ValueLength,
    PULONG ValueType
    );



/*
 * General STREAMS functions exported for use by drivers.
 *
 *     Some of these functions have been renamed and redefined with additional
 *     parameters for debugging purposes. Macros which call the new functions
 *     appear in place of the old ones.
 */

int
adjmsg(
    IN mblk_t *mp,
    IN int len
    );

mblk_t *
allocb(
    IN int size,
    IN unsigned int pri
    );

queue_t *
backq(
    IN queue_t *q
    );

int
bufcall(
    IN int size,
    IN int pri,
    IN int (*func)(long),
    IN long arg
    );

int
canput(
    IN queue_t *q
    );

int
canputnext(
    IN queue_t *q
    );

mblk_t *
copyb(
    IN mblk_t *bp
    );

mblk_t *
copymsg(
    IN mblk_t *bp
    );

//
// Test if data block type is one of the data messages (i.e. not a
// control message).
//
#define datamsg(type)   ((type) == M_DATA || (type) == M_PROTO || \
                         (type) == M_PCPROTO || (type) == M_DELAY)

int
disable_procs(
    IN queue_t *q
    );

mblk_t *
dupb(
    IN mblk_t *bp
    );

mblk_t *
dupmsg(
    IN mblk_t *bp
    );

int
enable_procs(
    IN queue_t *q
    );

void
enableok(
    IN queue_t *q
    );

mblk_t *
esballoc(
    IN unsigned char *base,
    IN int size,
    IN int pri,
    IN frtn_t *fr_rtn
    );

int
esbbcall(
    IN int pri,
    IN int (*func)(long),
    IN long arg
    );

void
flushq(
    IN queue_t *q,
    IN int flag
    );

void
freeb(
    IN mblk_t *bp
    );

void
freemsg(
    IN mblk_t *bp
    );

int (*
getadmin(
    IN unsigned short mid
    ))();

unsigned short
getmid(
    IN char *name
    );

mblk_t *
getq(
    IN queue_t *q
    );

/*
 * void
 * init_lock(
 *    IN lock_t *lockp
 *    );
 */

#define init_lock(lockp)    KeInitializeSpinLock(&((lockp)->spinlock))


int
insq(
    IN queue_t *q,
    IN mblk_t *emp,
    IN mblk_t *mp
    );

void
linkb(
    IN mblk_t *mp,
    IN mblk_t *bp
    );

int
msgdsize(
    IN mblk_t *bp
    );

void
noenable(
    IN queue_t *q
    );

/*
 * int
 * p_lock (
 *    IN lock_t *lockp,
 *    IN int level
 *    );
 */

#define p_lock(lockp, ignored)              \
            ( KeAcquireSpinLock(            \
	          &((lockp)->spinlock),     \
		  &((lockp)->oldlevel)      \
		  ),                        \
	      ((int) (lockp)->oldlevel)     \
	    )

int
pullupmsg(
    IN mblk_t *mp,
    IN int len
    );

int
put(
    IN queue_t *q,
    IN mblk_t *mp
    );

int
putbq(
    IN queue_t *q,
    IN mblk_t *bp
    );

int
putctl(
    IN queue_t *q,
    IN int type
    );

int
putctl1(
    IN queue_t *q,
    IN int type,
    IN int param
    );

int
putnext(
    IN queue_t *q,
    IN mblk_t *mp
    );

int
putnextctl(
    IN queue_t *q,
    IN int type
    );

int
putnextctl1(
    IN queue_t *q,
    IN int type,
    IN int p
    );

int
putq(
    IN queue_t *q,
    IN mblk_t *bp
    );

void
qenable(
    IN queue_t *q
    );

void
qreply(
    IN queue_t *q,
    IN mblk_t *bp
    );

int
qsize(
    IN queue_t *qp
    );

mblk_t *
rmvb(
    IN mblk_t *mp,
    IN mblk_t *bp
    );

void
rmvq(
    IN queue_t *q,
    IN mblk_t *mp
    );

int
strlog(
    IN short mid,
    IN short sid,
    IN char level,
    IN unsigned short  flags,
    IN char *fmt,
    ...
    );

int
strqget(
    IN queue_t *q,
    IN qfields_t what,
    IN unsigned char band,
    IN long *valp
    );

int
strqset(
    IN queue_t *q,
    IN qfields_t what,
    IN unsigned char band,
    IN long *val
    );

int
testb(
    IN int size,
    IN unsigned int pri
    );

int
timeout(
    IN int (*ftn)(char *),
    IN char *arg,
    IN long tim
    );

int
unbufcall(
    IN int id
    );

mblk_t *
unlinkb(
    IN mblk_t *mp
    );

int
untimeout(
    IN int seq
    );

/* void
 * v_lock (
 *     IN lock_t *lockp,
 *     IN int level
 *     );
 *
 */

#define v_lock(lockp, level)   \
            KeReleaseSpinLock(&((lockp)->spinlock), (KIRQL) (level))



//
// BUGBUG - this include needs to be removed for the final product.
//
#include <debugapi.h>



#endif  // _SYS_STREAM_INCLUDED

