/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    debugapi.h

Abstract:

    STREAMS debugging declarations

Author:

    Mike Massa (mikemas)           March 7, 1992

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     03-07-92    created

Notes:

--*/

#if DBG


#ifndef _DEBUGAPI_INCLUDED_
#define _DEBUGAPI_INCLUDED_


//
// Definitions for STREAMS buffer tracing facility
///

//
// STREAMS Message Type Map Structure
//
typedef struct _strm_msg_type_map {
    unsigned char  MsgType;                  // mp->b_datap->db_type
    char          *MsgTypeString;            // human-readable msg type string
    BOOLEAN        HasSubType;               // Does the message have a subtype?
} STRM_MSG_TYPE_MAP, *PSTRM_MSG_TYPE_MAP;


//
// STREAMS Message SubType Map Structure
//
typedef struct _strm_msg_subtype_map {
    int           MsgSubType;                // mp->b_rptr by convention
    char         *MsgSubTypeString;          // human-readable msg type string
} STRM_MSG_SUBTYPE_MAP, *PSTRM_MSG_SUBTYPE_MAP;


//
// Debug APIs
//
#define allocb(sz, pri)   StrmAllocb(sz, pri, __LINE__, __FILE__)

mblk_t *
StrmAllocb(
    IN int              sz,
    IN unsigned int     pri,
    IN int              line,
    IN char            *file
    );

#define copyb(mp)         StrmCopyb(mp, __LINE__, __FILE__)

mblk_t *
StrmCopyb(
    IN mblk_t          *bp,
    IN int              line,
    IN char            *file
    );

#define copymsg(mp)       StrmCopymsg(mp, __LINE__, __FILE__)

mblk_t *
StrmCopymsg(
    IN mblk_t          *bp,
    IN int              line,
    IN char            *file
    );

#define dupb(mp)          StrmDupb(mp, __LINE__, __FILE__)

mblk_t *
StrmDupb(
    IN mblk_t          *bp,
    IN int              line,
    IN char            *file
    );

#define dupmsg(mp)        StrmDupmsg(mp, __LINE__, __FILE__)

mblk_t *
StrmDupmsg(
    IN mblk_t          *bp,
    IN int              line,
    IN char            *file
    );

#define esballoc(base, sz, pri, fr_rtnp)  StrmEsballoc(base, sz, pri, fr_rtnp, \
                               __LINE__, __FILE__)

mblk_t *
StrmEsballoc(
    IN unsigned char   *base,
    IN int              sz,
    IN int              pri,
    IN frtn_t          *fr_rtnp,
    IN int              line,
    IN char            *file
    );


#define freeb(mp)         StrmFreeb(mp, __LINE__, __FILE__)

void
StrmFreeb(
    IN mblk_t          *mp,
    IN int              line,
    IN char            *file
    );

#define freemsg(mp)       StrmFreemsg(mp, __LINE__, __FILE__)

void
StrmFreemsg(
    IN mblk_t          *mp,
    IN int              line,
    IN char            *file
    );


VOID
StrmAcquireSpinLock(
    IN  PKSPIN_LOCK   PSpinLock,
    OUT PKIRQL        POldIrql,
    IN  int           Line,
    IN  char         *File
    );

VOID
StrmReleaseSpinLock(
    IN PKSPIN_LOCK  PSpinLock,
    IN KIRQL        OldIrql,
    IN int          Line,
    IN char        *File
    );

VOID
StrmFlushMsgTraceTable(
    VOID
    );

int
StrmInsertMsgTypeMapEntries(
    IN PSTRM_MSG_TYPE_MAP    MsgTypeMapArray,
    IN unsigned long         NumEntries
    );

int
StrmInsertMsgSubTypeMapEntries(
    IN PSTRM_MSG_SUBTYPE_MAP MsgSubTypeMapArray,
    IN unsigned long         NumEntries
    );

VOID
StrmListOutstandingMsgBuffers(
    VOID
    );


#endif // _DEBUGAPI_INCLUDED_


#endif //DBG
