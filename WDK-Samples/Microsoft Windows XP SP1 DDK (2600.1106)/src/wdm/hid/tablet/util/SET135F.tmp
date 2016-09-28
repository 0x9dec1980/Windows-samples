/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        errlog.h

    Abstract:
        Contains error log related definitions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 29-Jan-2001

    Revision History:
--*/

#ifndef _ERRLOG_H
#define _ERRLOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "errcodes.h"

//
// Constants
//
#ifdef DEBUG
  #define INFO                          ERRLOG_DEBUG_INFORMATION
  #define WARN                          ERRLOG_DEBUG_WARNING
  #define ERR                           ERRLOG_DEBUG_ERROR
#endif

//
// Macros
//
#ifdef DEBUG
  #define LOGDBGMSG(x)                  LogDbgMsg x
#else
  #define LOGDBGMSG(x)
#endif
#define UNIQUE_ERRID(id)                (((MODULE_ID) << 16) | (id))

//
// Global variables
//
extern PDRIVER_OBJECT gDriverObj;

//
// Function prototypes
//
VOID
LogError(
    IN NTSTATUS ErrorCode,
    IN NTSTATUS NTStatus,
    IN ULONG    UniqueID OPTIONAL,
    IN PWSTR    String1  OPTIONAL,
    IN PWSTR    String2  OPTIONAL
    );

#ifdef DEBUG
VOID __cdecl
LogDbgMsg(
    IN NTSTATUS ErrorCode,
    IN NTSTATUS NTStatus OPTIONAL,
    IN PSZ      pszFormat,
    ...
    );
#endif

#endif  //ifndef _ERRLOG_H
