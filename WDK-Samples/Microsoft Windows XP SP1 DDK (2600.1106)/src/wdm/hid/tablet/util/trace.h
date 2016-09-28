/*++
    Copyright (c) 1998-2001 Microsoft Corporation

    Module Name:
        trace.h

    Abstract:
        This module contains definitions of the trace functions

    Author:
        Michael Tsang (MikeTs) 24-Sep-1998

    Environment:
        Kernel mode

    Revision History:
--*/

#ifndef _TRACE_H
#define _TRACE_H

//
// Constants
//
#ifdef TRACING

#define MSGTYPE_MASK            0x00000003
#define MSGTYPE_NONE            0x00000000
#define MSGTYPE_INFO            0x00000001
#define MSGTYPE_WARN            0x00000002
#define MSGTYPE_ERR             0x00000003

#define TRACELEVEL_MASK         0x0000000f
#define MODULE_MASK             0xfffffff0

// gdwfTrace flags
#define TF_IN_PROGRESS          0x00000001

#endif  //ifdef TRACING

//
// Macros
//
#ifdef MODULE_ID
  #undef MODULE_ID
#endif
#ifdef DEBUG
  #define TRAP()                DbgBreakPoint()
#else
  #define TRAP()
#endif
#ifdef TRACING
  #define TRACEINIT(t,v)        giTraceLevel = (t);                     \
                                giVerboseLevel = (v);
  #define TRACEPROC(s,n)        static PSZ ProcName = (s);              \
                                static int ProcLevel = (n);
  #define TRACEENTER(p)         {                                       \
                                    if (IsTraceProcOn(MODULE_ID,        \
                                                      ProcName,         \
                                                      ProcLevel,        \
                                                      TRUE))            \
                                    {                                   \
                                        gdwfTrace |= TF_IN_PROGRESS;    \
                                        DbgPrint p;                     \
                                        gdwfTrace &= ~TF_IN_PROGRESS;   \
                                    }                                   \
                                }
  #define TRACEEXIT(p)          {                                       \
                                    if (IsTraceProcOn(MODULE_ID,        \
                                                      ProcName,         \
                                                      ProcLevel,        \
                                                      FALSE))           \
                                    {                                   \
                                        gdwfTrace |= TF_IN_PROGRESS;    \
                                        DbgPrint p;                     \
                                        gdwfTrace &= ~TF_IN_PROGRESS;   \
                                    }                                   \
                                }
  #define TRACEMSG(t,n,p)       if (IsTraceMsgOn(ProcName, (t), (n)))   \
                                {                                       \
                                    DbgPrint p;                         \
                                }
  #define TRACEINFO(n,p)        TRACEMSG(MSGTYPE_INFO, (n), p)
  #define TRACEWARN(p)          TRACEMSG(MSGTYPE_WARN, 1, p)
  #define TRACEERROR(p)         TRACEMSG(MSGTYPE_ERR, 0, p)
  #define TRACEASSERT(x)        if (!(x))                               \
                                {                                       \
                                    TRACEERROR(("Assert failed in "     \
                                                "file %s at line %d.\n",\
                                                __FILE__, __LINE__));   \
                                }
#else
  #define TRACEINIT(t,v)
  #define TRACEPROC(s,n)
  #define TRACEENTER(p)
  #define TRACEEXIT(p)
  #define TRACEMSG(t,n,p)
  #define TRACEINFO(n,p)
  #define TRACEWARN(p)
  #define TRACEERROR(p)
  #define TRACEASSERT(x)
#endif

#ifdef TRACING

//
// Type definitions
//
typedef struct _NAMETABLE
{
    ULONG dwCode;
    PSZ   pszName;
} NAMETABLE, *PNAMETABLE;

//
// Exported data
//
extern int   giTraceLevel;
extern int   giVerboseLevel;
extern ULONG gdwfTrace;

//
// Exported function prototypes
//
BOOLEAN
IsTraceProcOn(
    IN ULONG   dwModuleID,
    IN PSZ     pszProcName,
    IN int     iProcLevel,
    IN BOOLEAN fEnter
    );

BOOLEAN
IsTraceMsgOn(
    IN PSZ   pszProcName,
    IN ULONG dwMsgType,
    IN int   iVerboseLevel
    );

PSZ
LookupName(
    IN ULONG      dwCode,
    IN PNAMETABLE NameTable
    );

#endif  //ifdef TRACING

#endif  //ifndef _TRACE_H
