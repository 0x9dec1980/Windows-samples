/****************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

	debugwdm.h

Abstract:

	This header file is for debug and diagnostics for the WDM template driver

Environment:

	Kernel mode only

Notes:

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

	Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.

Revision History:

	10-20-96 : created

Author:

	Tom Green

****************************************************************************/

#ifndef _DEBUGWDM_
#define _DEBUGWDM_

#include "vector.h"
#include "drvshell.h"

// this makes it easy to hide static vars, make visible for debug
#if DBG
#define LOCAL
#define GLOBAL
#else
#define LOCAL	static
#define GLOBAL
#endif


// need these for creating macros for speed in logging and history

extern GLOBAL ULONG					IRPHistorySize;
extern GLOBAL ULONG		 			DebugPathSize;
extern GLOBAL ULONG					ErrorLogSize;


#if DBG

#define Debug_KdPrint(_x_)	DbgPrint("%s: ",DriverName);	\
										 DbgPrint _x_ ;
#define DEBUG_TRAP()		DbgBreakPoint()

#else

#define Debug_KdPrint(_x_)

#define DEBUG_TRAP()		DbgBreakPoint()

#endif


// assert macro

#define DEBUG_ASSERT(msg, exp)								\
    if(!(exp))												\
        Debug_Assert(#exp, __FILE__, __LINE__, msg)


// these macros are for logging things, avoid subroutine call
// if they are disabled (number of entries = 0)

#define DEBUG_LOG_IRP_HIST(dobj, pirp, majfunc, buff, bufflen)		\
	if(IRPHistorySize)												\
		Debug_LogIrpHist(dobj, pirp, majfunc, buff, bufflen)

#define DEBUG_LOG_PATH(path)										\
	if(DebugPathSize)												\
		Debug_LogPath(path)

#define DEBUG_LOG_ERROR(status)										\
	if(ErrorLogSize)												\
		Debug_LogError(status)

// prototypes

NTSTATUS
Debug_OpenWDMDebug(VOID);

VOID
Debug_CloseWDMDebug(VOID);

NTSTATUS
Debug_SizeIRPHistoryTable(ULONG size);

NTSTATUS
Debug_SizeDebugPathHist(ULONG size);

NTSTATUS
Debug_SizeErrorLog(ULONG size);

VOID
Debug_LogIrpHist(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp,
				 IN ULONG majorFunction, IN PVOID ioBuffer, ULONG bufferLen);

VOID
Debug_LogPath(IN PCHAR pPath);

VOID
Debug_LogError(IN NTSTATUS ntStatus);

VOID
Debug_Trap(IN PCHAR pTrapCause);

VOID
Debug_Assert(IN PVOID FailedAssertion, IN PVOID FileName, IN ULONG LineNumber,
			 IN PCHAR Message);

PVOID
Debug_MemAlloc(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes, BOOLEAN bZeroFill);

VOID
Debug_MemFree(IN PVOID pMem);

ULONG
Debug_ExtractAttachedDevices(PDRIVER_OBJECT pDriverObject, PCHAR pBuffer, ULONG buffSize);

ULONG
Debug_ExtractIRPHist(PCHAR pBuffer, ULONG buffSize);

ULONG
Debug_ExtractPathHist(PCHAR pBuffer, ULONG buffSize);

ULONG
Debug_ExtractErrorLog(PCHAR pBuffer, ULONG buffSize);

ULONG
Debug_DumpDriverLog(PDEVICE_OBJECT pDeviceObject, PCHAR pBuffer, ULONG buffSize);

PCHAR
Debug_TranslateStatus(NTSTATUS ntStatus);

PCHAR
Debug_TranslateIoctl(LONG ioctl);

#ifdef _X86_
VOID
Debug_StartPentiumCycleCounter(PLARGE_INTEGER cycleCount);

VOID
Debug_StopPentiumCycleCounter(PLARGE_INTEGER cycleCount);
#endif

#endif // _DEBUGWDM_
