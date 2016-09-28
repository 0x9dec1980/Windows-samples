/***************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

	debugwdm.c

Abstract:

	Debug routines for WDM template driver 

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


#include <wdm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debugwdm.h"


// data structures, macros, and data that the outside world doesn't need to know about

// amount of data to save from IRP buffer
#define IRP_DATA_SIZE		0x08

// signature to write at end of allocated memory block
#define MEM_ALLOC_SIGNATURE	(ULONG) 'CLLA'

// signature to write at end of freed memory block
#define MEM_FREE_SIGNATURE	(ULONG) 'EERF'

// size for temporary string formatting buffers
#define TMP_STR_BUFF_SIZE	0x100

// initial number of entries in tables and logs
#define DEFAULT_LOG_SIZE	64L


// data structures for debug stuff

// entry for IRP history table for IRPs going in and out
typedef struct IRPHistory
{
	LARGE_INTEGER			timeStamp;
	PDEVICE_OBJECT			pDeviceObject;
	PIRP					pIrp;
	ULONG					majorFunction;
	UCHAR					irpDataCount;
	UCHAR					irpData[IRP_DATA_SIZE];
} IRPHist, *PIRPHist;

// entry for execution tracing
typedef struct PATHHistory
{
	LARGE_INTEGER			timeStamp;
	PCHAR					pPath;
} PATHHist, *PPATHHist;

// entry for error log
typedef struct ERRORLog
{
	LARGE_INTEGER			timeStamp;
	NTSTATUS				Status;
} ERRLog, *PERRLog;

// this is for translating a code into an ASCII string
typedef struct Code2Ascii
{
	LONG					Code;
	PCHAR					Str;
} Code2Ascii;


// local data for debug file

// IRP history table components
LOCAL  PIRPHist				IRPHistoryTable		= NULL;
LOCAL  ULONG				IRPHistoryIndex		= 0L;
GLOBAL ULONG				IRPHistorySize		= 0L;

// Debug path storage
LOCAL  PPATHHist	 		DebugPathHist		= NULL;
LOCAL  ULONG		 		DebugPathIndex		= 0L;
GLOBAL ULONG		 		DebugPathSize		= 0L;

// Error log components
LOCAL  PERRLog		 		ErrorLog			= NULL;
LOCAL  ULONG				ErrorLogIndex		= 0L;
GLOBAL ULONG				ErrorLogSize		= 0L;

// memory allocation stats
LOCAL  ULONG				MemoryAllocated		= 0L;
LOCAL  ULONG				MemAllocFailCnt		= 0L;
LOCAL  ULONG				MemAllocCnt			= 0L;
LOCAL  ULONG				MemFreeFailCnt		= 0L;
LOCAL  ULONG				MemFreeCnt			= 0L;
LOCAL  ULONG				MaxMemAllocated		= 0L;

// this is for translating NT status codes into ASCII strings
LOCAL  Code2Ascii NTErrors[] =
{
	STATUS_SUCCESS,						"STATUS_SUCCESS",
	STATUS_PENDING,						"STATUS_PENDING",
	STATUS_DEVICE_BUSY,					"STATUS_DEVICE_BUSY",
	STATUS_INSUFFICIENT_RESOURCES,		"STATUS_INSUFFICIENT_RESOURCES",
	STATUS_INVALID_DEVICE_REQUEST,		"STATUS_INVALID_DEVICE_REQUEST",
	STATUS_DEVICE_NOT_READY,			"STATUS_DEVICE_NOT_READY",
	STATUS_INVALID_BUFFER_SIZE,			"STATUS_INVALID_BUFFER_SIZE"
};

LOCAL  ULONG				NumNTErrs = sizeof(NTErrors) / sizeof(Code2Ascii);
LOCAL  CHAR					UnknownStatus[80];

// this is for translating IOCTL codes into ASCII strings
LOCAL  Code2Ascii IoctlCodes[] =
{
	IRP_MJ_CREATE,						"CREATE",
	IRP_MJ_CREATE_NAMED_PIPE,			"CNPIPE",
	IRP_MJ_CLOSE,						"CLOSE ",
	IRP_MJ_READ,						"READ  ",
	IRP_MJ_WRITE,						"WRITE ",
	IRP_MJ_QUERY_INFORMATION,			"QRYINF",
	IRP_MJ_SET_INFORMATION,				"SETINF",
	IRP_MJ_QUERY_EA,					"QRYEA ",
	IRP_MJ_SET_EA,						"SETEA ",
	IRP_MJ_FLUSH_BUFFERS,				"FLSBUF",
	IRP_MJ_QUERY_VOLUME_INFORMATION,	"QRYVOL",
	IRP_MJ_SET_VOLUME_INFORMATION,		"SETVOL",
	IRP_MJ_DIRECTORY_CONTROL,			"DIRCTL",
	IRP_MJ_FILE_SYSTEM_CONTROL,			"SYSCTL",
	IRP_MJ_DEVICE_CONTROL,				"DEVCTL",
	IRP_MJ_INTERNAL_DEVICE_CONTROL,		"INDVCT",
	IRP_MJ_SHUTDOWN,					"SHTDWN",
	IRP_MJ_LOCK_CONTROL,				"LOKCTL",
	IRP_MJ_CLEANUP,						"CLNUP ",
	IRP_MJ_CREATE_MAILSLOT,				"MAILSL",
	IRP_MJ_QUERY_SECURITY,				"QRYSEC",
	IRP_MJ_SET_SECURITY,				"SETSEC",
	IRP_MJ_SYSTEM_CONTROL,				"SYSCTL",
	IRP_MJ_DEVICE_CHANGE,				"DEVCHG",
	IRP_MJ_QUERY_QUOTA,					"QRYQUO",
	IRP_MJ_SET_QUOTA,					"SETQUO",
	IRP_MJ_POWER,						"POWER ",
	IRP_MJ_PNP,							"PNP   ",
	IRP_MJ_MAXIMUM_FUNCTION,			"MAXFNC"
};

LOCAL ULONG					NumIoctl = sizeof(IoctlCodes) / sizeof(Code2Ascii);
LOCAL CHAR					UnknownIoctl[80];


/************************************************************************/
/*						Debug_OpenWDMDebug								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocate resources and init history tables and logs.				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  VOID																*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
Debug_OpenWDMDebug(VOID)
{
	NTSTATUS		ntStatus = STATUS_SUCCESS;

	// make sure driver name and version got filled in O.K.
	if(!DriverName)
		DriverName = "DRVSHELL";

	if(!DriverVersion)
		DriverVersion = "0.99";

	// allocate tables and logs
	ntStatus = Debug_SizeIRPHistoryTable(DEFAULT_LOG_SIZE);
	if(!NT_SUCCESS(ntStatus))
	{
		Debug_CloseWDMDebug();
		return ntStatus;
	}

	ntStatus = Debug_SizeDebugPathHist(DEFAULT_LOG_SIZE);
	if(!NT_SUCCESS(ntStatus))
	{
		Debug_CloseWDMDebug();
		return ntStatus;
	}

	ntStatus = Debug_SizeErrorLog(DEFAULT_LOG_SIZE);
	if(!NT_SUCCESS(ntStatus))
	{
		Debug_CloseWDMDebug();
		return ntStatus;
	}
	
	return ntStatus;	
} // Debug_OpenWDMDebug


/************************************************************************/
/*						Debug_CloseWDMDebug								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Free up resources used for history tables and logs.					*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  VOID																*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_CloseWDMDebug(VOID)
{
	if(DebugPathHist)
	{
		Debug_MemFree(DebugPathHist);
		DebugPathHist	= NULL;
		DebugPathSize	= 0L;
	}

	if(IRPHistoryTable)
	{
		Debug_MemFree(IRPHistoryTable);
		IRPHistoryTable	= NULL;
		IRPHistorySize	= 0L;
	}

	if(ErrorLog)
	{
		Debug_MemFree(ErrorLog);
		ErrorLog		= NULL;
		ErrorLogSize	= 0L;
	}
} // Debug_CloseWDMDebug


/************************************************************************/
/*						Debug_SizeIRPHistoryTable						*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocate IRP history table											*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  size - number of entries in table									*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
Debug_SizeIRPHistoryTable(ULONG size)
{
	NTSTATUS		ntStatus = STATUS_SUCCESS;

	// see if they are trying to set the same size
	if(size == IRPHistorySize)
		return ntStatus;

	// get rid of old history table if we got one
	if(IRPHistoryTable)
		Debug_MemFree(IRPHistoryTable);

	IRPHistoryTable	= NULL;
	IRPHistoryIndex	= 0L;
	IRPHistorySize	= 0L;

	if(size != 0L)
	{
		IRPHistoryTable = Debug_MemAlloc(NonPagedPool, sizeof(IRPHist) * size, TRUE);
		if(IRPHistoryTable == NULL)
			return STATUS_INSUFFICIENT_RESOURCES;
	}

	IRPHistorySize = size;

	return ntStatus;
} // Debug_SizeIRPHistoryTable


/************************************************************************/
/*						Debug_SizeDebugPathHist							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocate path history												*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  size - number of entries in history									*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
Debug_SizeDebugPathHist(ULONG size)
{
	NTSTATUS		ntStatus = STATUS_SUCCESS;

	// see if they are trying to set the same size
	if(size == DebugPathSize)
		return ntStatus;

	// get rid of old path history if we got one
	if(DebugPathHist)
		Debug_MemFree(DebugPathHist);

	DebugPathHist	= NULL;
	DebugPathIndex	= 0L;
	DebugPathSize	= 0L;

	if(size != 0L)
	{
		DebugPathHist = Debug_MemAlloc(NonPagedPool, sizeof(PATHHist) * size, TRUE);
		if(DebugPathHist == NULL)
			return STATUS_INSUFFICIENT_RESOURCES;
	}

	DebugPathSize = size;

	return ntStatus;
} // Debug_SizeDebugPathHist


/************************************************************************/
/*						Debug_SizeErrorLog								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocate error log													*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  size - number of entries in error log								*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
Debug_SizeErrorLog(ULONG size)
{
	NTSTATUS		ntStatus = STATUS_SUCCESS;

	// see if they are trying to set the same size
	if(size == ErrorLogSize)
		return ntStatus;

	// get rid of old error log if we got one
	if(ErrorLog)
		Debug_MemFree(ErrorLog);
	ErrorLog		= NULL;
	ErrorLogIndex	= 0L;
	ErrorLogSize	= 0L;

	if(size != 0L)
	{
		ErrorLog = Debug_MemAlloc(NonPagedPool, sizeof(ERRLog) * size, TRUE);
		// make sure we actually allocated some memory
		if(ErrorLog == NULL)
			return STATUS_INSUFFICIENT_RESOURCES;
	}

	ErrorLogSize = size;

	return ntStatus;
} // Debug_SizeErrorLog


/************************************************************************/
/*						Debug_LogIrpHist								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Logs IRP history. These are timestamped and put in a				*/
/*  circular buffer for extraction later.								*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  pDeviceObject - pointer to device object.							*/
/*																		*/
/*  pIrp          - pointer to IRP.										*/
/*																		*/
/*  majorFunction - major function of IRP.								*/
/*																		*/
/*  ioBuffer      - buffer for data passed in and out of driver.		*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_LogIrpHist(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp,
				 IN ULONG majorFunction, IN PVOID ioBuffer, ULONG bufferLen)
{
	PIRPHist pIrpHist;
	
	// get pointer to current entry in IRP history table
	pIrpHist = &IRPHistoryTable[IRPHistoryIndex++];
	
	// point to the next entry in the IRP history table
	IRPHistoryIndex %= IRPHistorySize;

	// get time stamp
	pIrpHist->timeStamp = KeQueryPerformanceCounter(NULL);

	// save IRP, device object, major function and first 8 bytes of data in buffer
	pIrpHist->pDeviceObject = pDeviceObject;
	pIrpHist->pIrp = pIrp;
	pIrpHist->majorFunction = majorFunction;

	// copy any data if we have it
	pIrpHist->irpDataCount = (UCHAR) min(IRP_DATA_SIZE, bufferLen);
	if(bufferLen)
		RtlCopyMemory(pIrpHist->irpData, (PUCHAR) ioBuffer, pIrpHist->irpDataCount);
} // Debug_LogIrpHist


/************************************************************************/
/*						Debug_LogPath									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Logs execution path through code. These are timestamped and put		*/
/*  in a circular buffer for extraction later. Kernel print routines 	*/
/*  are also called.													*/
/*																		*/
/*  DANGER DANGER Will Robinson - the argument to this must be a 		*/
/*  const char pointer,													*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  pPath - Pointer to const char array that contains description of	*/
/*          of path.													*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_LogPath(IN CHAR *pPath)
{
	PPATHHist	pPHist;

	// get pointer to current entry in path history
	pPHist = &DebugPathHist[DebugPathIndex++];

	// point to the next entry in path trace
	DebugPathIndex %= DebugPathSize;

	// get time stamp
	pPHist->timeStamp = KeQueryPerformanceCounter(NULL);

	// save path string
	pPHist->pPath = pPath;

	// now call kernel print routines
	Debug_KdPrint(("%s\n", pPath));
	
} // Debug_LogPath


/************************************************************************/
/*						Debug_LogError									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Logs NTSTATUS type errors. These are timestamped and put in a		*/
/*  circular buffer for extraction later.								*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  ntStatus - NTSTATUS error to log.									*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_LogError(IN NTSTATUS ntStatus)
{
	PERRLog	pErrLog;

	// no error, so don't log
	if(NT_SUCCESS(ntStatus))
		return;

	// get pointer to current entry in error log
	pErrLog = &ErrorLog[ErrorLogIndex++];

	// point to the next entry in error log
	ErrorLogIndex %= ErrorLogSize;

	// get time stamp
	pErrLog->timeStamp = KeQueryPerformanceCounter(NULL);

	// save status
	pErrLog->Status = ntStatus;
} // Debug_LogError


/************************************************************************/
/*						Debug_Trap										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Trap. Causes execution to halt after logging message.				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  pTrapCause - pointer to char array that contains description		*/
/*				 of cause of trap.										*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_Trap(IN PCHAR pTrapCause)
{
	// log the path
	DEBUG_LOG_PATH("Debug_Trap: ");

	// kernel debugger print
	Debug_KdPrint(("Debug_Trap: "));

	// log the path
	DEBUG_LOG_PATH(pTrapCause);

	// kernel debugger print
	Debug_KdPrint(("%s\n",pTrapCause));

	// halt execution
	DEBUG_TRAP();
} // Debug_TRAP


/************************************************************************/
/*						Debug_Assert									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Assertion routine.													*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  This should not be called directly. Use DEBUG_ASSERT macro.			*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_Assert(IN PVOID FailedAssertion, IN PVOID FileName, IN ULONG LineNumber,
			 IN PCHAR Message)
{
	// just call the NT assert function and stop in the debugger.
    //RtlAssert(FailedAssertion, FileName, LineNumber, Message);
    Debug_Trap(Message);
} // Debug_Assert


/************************************************************************/
/*						Debug_MemAlloc									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocates block of memory. Stores length of block and a				*/
/*  signature ULONG for keeping track of amount of memory allocated		*/
/*  and checking for bogus calls to Debug_MemFree. The signature		*/
/*  can also be used to determine if someone has written past the		*/
/*	end of the block. Memory is zero filled if bZeroFill flag is set.	*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	PoolType      - Pool to allocate memory from.						*/
/*																		*/
/*  NumberOfBytes - Number of bytes to allocate.						*/
/*																		*/
/*  bZeroFill     - set to true if you want memory zero filled			*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	PVOID - pointer to allocated memory.								*/
/*																		*/
/************************************************************************/
PVOID
Debug_MemAlloc(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes, BOOLEAN bZeroFill)
{
	PULONG	pMem;

	// allocate memory plus a little extra for our own use
	pMem = ExAllocatePool(PoolType, NumberOfBytes + (2 * sizeof(ULONG)));

	// see if we actually allocated any memory
	if(pMem)
	{
		// keep track of how much we allocated
		MemoryAllocated += NumberOfBytes;

		// see if we have a new maximum
		if(MemoryAllocated > MaxMemAllocated)
			MaxMemAllocated = MemoryAllocated;

		// store number of bytes allocated at start of memory allocated
		*pMem++ = NumberOfBytes;

		// now we are pointing at the memory allocated for caller
		// put signature word at end

		// get new pointer that points to end of buffer - ULONG
		pMem = (PULONG) (((PUCHAR) pMem) + NumberOfBytes);

		// write signature
		*pMem = MEM_ALLOC_SIGNATURE;

		// get back pointer to return to caller
		pMem = (PULONG) (((PUCHAR) pMem) - NumberOfBytes);

		// let's zero memory if requested
		if(bZeroFill)
			RtlZeroMemory(pMem, NumberOfBytes);

		// log stats
		MemAllocCnt++;
	}
	else
		// failed, log stats
		MemAllocFailCnt++;
		
	return (PVOID) pMem;
} // Debug_MemAlloc


/************************************************************************/
/*						Debug_MemFree									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Frees memory allocated in call to Debug_MemAlloc. Checks for		*/
/*  signature ULONG at the end of allocated memory to make sure			*/
/*  this is a valid block to free.										*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	pMem - pointer to allocated block to free							*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID. Traps if it is an invalid block.								*/
/*																		*/
/************************************************************************/
VOID
Debug_MemFree(IN PVOID pMem)
{
	PULONG	pTmp = (PULONG) pMem;
	ULONG	buffSize;
	
	// point at size ULONG at start of buffer, and address to free
	pTmp--;

	// get the size of memory allocated by caller
	buffSize = *pTmp;

	// point at signature and make sure it's O.K.
	((PCHAR) pMem) += buffSize;

	if(*((PULONG) pMem) == MEM_ALLOC_SIGNATURE)
	{
		// let's go ahead and get rid of signature in case we get called
		// with this pointer again and memory is still paged in
		*((PULONG) pMem) = MEM_FREE_SIGNATURE;
		
		// adjust amount of memory allocated
		MemoryAllocated -= buffSize;
		// free real pointer
		ExFreePool(pTmp);

		// log stats
		MemFreeCnt++;
	}
	else
	{
		// not a real allocated block, or someone wrote past the end
		MemFreeFailCnt++;
		DEBUG_TRAP();
	}
	
} // Debug_MemFree


/************************************************************************/
/*						Debug_ExtractAttachedDevices					*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Formats and places attached device info into a buffer.				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	pDriverObject - pointer to driver object.							*/
/*																		*/
/*	pBuffer       - pointer to buffer to fill with IRP history.			*/
/*																		*/
/*  buffSize      - size of pBuffer.									*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_ExtractAttachedDevices(PDRIVER_OBJECT pDriverObject, PCHAR pBuffer, ULONG buffSize)
{
	PCHAR				strBuff;
	PDEVICE_EXTENSION	deviceExtension;
	PDEVICE_OBJECT		pDeviceObject;
	BOOLEAN				bDev = FALSE;

	// make sure we have a pointer and a number of bytes
	if(pBuffer == NULL || buffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	strBuff = Debug_MemAlloc(NonPagedPool, TMP_STR_BUFF_SIZE, FALSE);

	if(strBuff == NULL)
		return 0L;

	// title
	sprintf(strBuff, "\n\n\nAttached Devices\n\n");

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto ExtractDevices_End;
	else
		strcat(pBuffer, strBuff);

	// columns	
	sprintf(strBuff, "Device              Device Obj  IRPs Complete   Byte Count\n\n");

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto ExtractDevices_End;
	else
		strcat(pBuffer, strBuff);

	// get the first device object
	pDeviceObject = pDriverObject->DeviceObject;

	// march through linked list of devices
	while(pDeviceObject)
	{
		// found at least one device
		bDev = TRUE;

		// Get a pointer to the device extension
		deviceExtension = pDeviceObject->DeviceExtension;
		sprintf(strBuff, "%-17s   0x%08x  0x%08x      0x%08x%08x\n", &deviceExtension->DeviceName[8],
				pDeviceObject, deviceExtension->IRPCount,
				deviceExtension->ByteCount.HighPart,
				deviceExtension->ByteCount.LowPart);

		// make sure it fits in buffer
		if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
			goto ExtractDevices_End;
		else
			strcat(pBuffer, strBuff);

		pDeviceObject = pDeviceObject->NextDevice;
	}

	// if we don't have any devices, say so, but this should never happen (I think)
	if(!bDev)
	{
		sprintf(strBuff, "No attached devices\n");

		// make sure it fits in buffer
		if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
			goto ExtractDevices_End;
		else
			strcat(pBuffer, strBuff);
	}

ExtractDevices_End:

	Debug_MemFree(strBuff);
	return strlen(pBuffer);	
} // Debug_ExtractAttachedDevices


/************************************************************************/
/*						Debug_ExtractIRPHist							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Formats and places IRP history info into a buffer.					*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	pBuffer  - pointer to buffer to fill with IRP history.				*/
/*																		*/
/*  buffSize - size of pBuffer.											*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_ExtractIRPHist(PCHAR pBuffer, ULONG buffSize)
{
	ULONG		index, size;
	PIRPHist	pIrpHist;
	PCHAR		strBuff;
	BOOLEAN		bHist = FALSE;
	
	// make sure we have a pointer and a number of bytes
	if(pBuffer == NULL || buffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	strBuff = Debug_MemAlloc(NonPagedPool, TMP_STR_BUFF_SIZE, FALSE);

	if(strBuff == NULL)
		return 0L;

	// title
	sprintf(strBuff, "\n\n\nIRP History\n\n");

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto IRPHist_End;
	else
		strcat(pBuffer, strBuff);

	// see if error log is on
	if(IRPHistorySize == 0L)
	{
		sprintf(strBuff, "IRP History is disabled\n");

		// make sure it fits in buffer
		if((strlen(pBuffer) + strlen(strBuff)) < buffSize)
			strcat(pBuffer, strBuff);
		goto IRPHist_End;
	}

	// columns	
	sprintf(strBuff, "Time Stamp          Device Obj  IRP         Func    Data\n\n");

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto IRPHist_End;
	else
		strcat(pBuffer, strBuff);

	index = IRPHistoryIndex;

	for(size = 0; size < IRPHistorySize; size++)
	{
		// get pointer to current entry in IRP history table
		pIrpHist = &IRPHistoryTable[index++];

		// parse timestamp and IRP history and write to buffer
		if(pIrpHist->timeStamp.LowPart)
		{
			UCHAR	dataCount;
			char	dataBuff[10];

			// we have at least one entry
			bHist = TRUE;

			sprintf(strBuff, "0x%08x%08x  0x%08x  0x%08x  %s  ",
					pIrpHist->timeStamp.HighPart, pIrpHist->timeStamp.LowPart,
					pIrpHist->pDeviceObject, pIrpHist->pIrp, Debug_TranslateIoctl(pIrpHist->majorFunction));


			// add data bytes if we got them
			for(dataCount = 0; dataCount < pIrpHist->irpDataCount; dataCount++)
			{
				sprintf(dataBuff, "%02x ", pIrpHist->irpData[dataCount]);
				strcat(strBuff, dataBuff);
			}

			sprintf(dataBuff, "\n");

			strcat(strBuff, dataBuff);

			// make sure it fits in buffer
			if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
				goto IRPHist_End;
			else
				strcat(pBuffer, strBuff);
		}
		
		// point to the next entry in the IRP history table
		index %= IRPHistorySize;
	}

	// if we don't have history, say so, but this should never happen (I think)
	if(!bHist)
	{
		sprintf(strBuff, "No IRP history\n");

		// make sure it fits in buffer
		if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
			goto IRPHist_End;
		else
			strcat(pBuffer, strBuff);
	}

IRPHist_End:

	Debug_MemFree(strBuff);
	return strlen(pBuffer);	
} // Debug_ExtractIRPHist


/************************************************************************/
/*						Debug_ExtractPathHist							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Formats and places path history info into buffer.					*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	pBuffer  - pointer to buffer to fill with path history.				*/
/*																		*/
/*  buffSize - size of pBuffer.											*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_ExtractPathHist(PCHAR pBuffer, ULONG buffSize)
{
	ULONG		index, size;
	PPATHHist	pPHist;
	PCHAR		strBuff;
	BOOLEAN		bHist = FALSE;
	
	// make sure we have a pointer and a number of bytes
	if(pBuffer == NULL || buffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	strBuff = Debug_MemAlloc(NonPagedPool, TMP_STR_BUFF_SIZE, FALSE);

	if(strBuff == NULL)
		return 0L;

	// title
	sprintf(strBuff, "\n\n\nExecution Path History\n\n");

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto PathHist_End;
	else
		strcat(pBuffer, strBuff);
		
	// see if path history is on
	if(DebugPathSize == 0L)
	{
		sprintf(strBuff, "Path History is disabled\n");

		// make sure it fits in buffer
		if((strlen(pBuffer) + strlen(strBuff)) < buffSize)
			strcat(pBuffer, strBuff);
		goto PathHist_End;
	}

	// columns	
	sprintf(strBuff, "Time Stamp          Path\n\n");

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto PathHist_End;
	else
		strcat(pBuffer, strBuff);
		
	index = DebugPathIndex;

	for(size = 0; size < DebugPathSize; size++)
	{
		// get pointer to current entry in path history
		pPHist = &DebugPathHist[index++];

		// parse timestamp and path and write to buffer. Check for NULL entries
		if(pPHist->timeStamp.LowPart)
		{
			// at least we have one entry
			bHist = TRUE;
			
			sprintf(strBuff, "0x%08x%08x  %s\n", pPHist->timeStamp.HighPart, 
					pPHist->timeStamp.LowPart, pPHist->pPath);

			// make sure it fits in buffer
			if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
				goto PathHist_End;
			else
				strcat(pBuffer, strBuff);
		}
		
		// point to the next entry in path trace
		index %= DebugPathSize;
	}

	// if we don't have history, say so, but this should never happen (I think)
	if(!bHist)
	{
		sprintf(strBuff, "No execution path history\n");

		// make sure it fits in buffer
		if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
			goto PathHist_End;
		else
			strcat(pBuffer, strBuff);
	}

PathHist_End:

	Debug_MemFree(strBuff);
	return strlen(pBuffer);	
} // Debug_ExtractPathHist


/************************************************************************/
/*						Debug_ExtractErrorLog							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Formats and places error log info into buffer.						*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	pBuffer  - pointer to buffer to fill with IRP history.				*/
/*																		*/
/*  buffSize - size of pBuffer.											*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_ExtractErrorLog(PCHAR pBuffer, ULONG buffSize)
{
	ULONG		index, size;
	PERRLog		pErrLog;
	PCHAR		strBuff;
	BOOLEAN		bErrors = FALSE;
	
	// make sure we have a pointer and a number of bytes
	if(pBuffer == NULL || buffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	strBuff = Debug_MemAlloc(NonPagedPool, TMP_STR_BUFF_SIZE, FALSE);

	if(strBuff == NULL)
		return 0L;

	// title
	sprintf(strBuff, "\n\n\nError Log\n\n");

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto ErrorLog_End;
	else
		strcat(pBuffer, strBuff);
		
	// see if error log is on
	if(ErrorLogSize == 0L)
	{
		sprintf(strBuff, "Error Log is disabled\n");

		// make sure it fits in buffer
		if((strlen(pBuffer) + strlen(strBuff)) < buffSize)
			strcat(pBuffer, strBuff);

		goto ErrorLog_End;
	}

	// columns	
	sprintf(strBuff, "Time Stamp          Error\n\n");

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto ErrorLog_End;
	else
		strcat(pBuffer, strBuff);
		
	index = ErrorLogIndex;

	for(size = 0; size < ErrorLogSize; size++)
	{
		// get pointer to current entry in error log
		pErrLog = &ErrorLog[index++];

		// parse timestamp and error and write to buffer
		if(pErrLog->timeStamp.LowPart)
		{
			// we have at least one error
			bErrors = TRUE;
			
			sprintf(strBuff, "0x%08x%08x  %s\n", pErrLog->timeStamp.HighPart, 
					pErrLog->timeStamp.LowPart, Debug_TranslateStatus(pErrLog->Status));

			// make sure it fits in buffer
			if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
				goto ErrorLog_End;
			else
				strcat(pBuffer, strBuff);
		}
		
		// point at next entry
		index %= ErrorLogSize;
	}

	// if we don't have errors, say so
	if(!bErrors)
	{
		sprintf(strBuff, "No errors in log\n");

		// make sure it fits in buffer
		if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
			goto ErrorLog_End;
		else
			strcat(pBuffer, strBuff);
	}

ErrorLog_End:

	Debug_MemFree(strBuff);
	return strlen(pBuffer);
} // Debug_ExtractErrorLog


/************************************************************************/
/*						Debug_DumpDriverLog								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Dumps all history and logging to buffer.							*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  pDeviceObject - pointer to device object.							*/
/*																		*/
/*	pBuffer       - pointer to buffer to fill with IRP history.			*/
/*																		*/
/*  buffSize      - size of pBuffer.									*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_DumpDriverLog(PDEVICE_OBJECT pDeviceObject, PCHAR pBuffer, ULONG buffSize)
{
	PCHAR		strBuff;

	// make sure we have a pointer and a number of bytes
	if(pBuffer == NULL || buffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	strBuff = Debug_MemAlloc(NonPagedPool, TMP_STR_BUFF_SIZE, FALSE);

	if(strBuff == NULL)
		return 0L;

	// driver name and version, memory allocated
	sprintf(strBuff, "\n\n\nDriver:	 %s\n\nVersion: %s\n\nMemory Allocated:          0x%08x\nMaximum Memory Allocated:  0x%08x\n",
			DriverName, DriverVersion, MemoryAllocated, MaxMemAllocated);

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto DriverLog_End;
	else
		strcat(pBuffer, strBuff);
		
	// memory allocation stats
	sprintf(strBuff, "MemAlloc Count:            0x%08x\nMemFree Count:             0x%08x\nMemAlloc Fail Count:       0x%08x\nMemFree Fail Count:        0x%08x\n",
			MemAllocCnt, MemFreeCnt, MemAllocFailCnt, MemFreeFailCnt);

	// make sure it fits in buffer
	if((strlen(pBuffer) + strlen(strBuff)) >= buffSize)
		goto DriverLog_End;
	else
		strcat(pBuffer, strBuff);
		
	// get attached devices
	Debug_ExtractAttachedDevices(pDeviceObject->DriverObject, pBuffer, buffSize);

	// get IRP history
	Debug_ExtractIRPHist(pBuffer, buffSize);

	// get execution path history
	Debug_ExtractPathHist(pBuffer, buffSize);

	// get error log
	Debug_ExtractErrorLog(pBuffer, buffSize);

DriverLog_End:

	Debug_MemFree(strBuff);
	return strlen(pBuffer);
} // Debug_DumpDriverLog


/************************************************************************/
/*						Debug_TranslateStatus							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Translates NTSTATUS into ASCII string.								*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  ntStatus - NTSTATUS code.											*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	PCHAR - pointer to error string.									*/
/*																		*/
/************************************************************************/
PCHAR
Debug_TranslateStatus(NTSTATUS ntStatus)
{
	ULONG	err;

	for(err = 0; err < NumNTErrs; err++)
	{
		if(ntStatus == NTErrors[err].Code)
			return NTErrors[err].Str;
	}

	// fell through, not an error we handle
	sprintf(UnknownStatus, "Unknown error 0x%08x", ntStatus);

	return UnknownStatus;
} // Debug_TranslateStatus


/************************************************************************/
/*						Debug_TranslateIoctl							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Translates IOCTL into ASCII string.									*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  ioctl - ioctl code.													*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	PCHAR - pointer to error string.									*/
/*																		*/
/************************************************************************/
PCHAR
Debug_TranslateIoctl(LONG ioctl)
{
	ULONG	index;

	// it's kind of silly to search at this point, but just in case
	// they change the actual IOCTLs we will be covered
	for(index = 0; index < NumIoctl; index++)
	{
		if(ioctl == IoctlCodes[index].Code)
			return IoctlCodes[index].Str;
	}

	// fell through, not an error we handle
	sprintf(UnknownIoctl, "0x%04x", ioctl);

	return UnknownIoctl;
} // Debug_TranslateIoctl


#ifdef _X86_
/************************************************************************/
/*					Debug_StartPentiumCycleCounter						*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Uses pentium RDTSC instruction to get current cycle count since		*/
/*  chip was powered. This records a staring cycle count, and then		*/
/*  call the stop routine to get a cycle count.							*/
/*																		*/
/*  DANGER DANGER Will Robinson - these routines will only work when	*/
/*  used on a Pentium processor. Also, these probably should be macros	*/
/*  so subroutine calls don't skew the results. They should be close	*/
/*  enough for profiling though.										*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  cycleCount - this is where we will store the starting cycle count.	*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_StartPentiumCycleCounter(PLARGE_INTEGER cycleCount)
{
	LARGE_INTEGER	startCount;

	_asm
	{
		// the RDTSC instruction is not really documented,
		// so assemblers may not support it, so use _emit to
		// create instruction
		_emit	0Fh
		_emit	31h
		mov		startCount.HighPart, edx
		mov		startCount.LowPart,  eax
	}

	// here is our starting count
	*cycleCount = startCount;
} // Debug_StartPentiumCycleCounter

/************************************************************************/
/*					Debug_StopPentiumCycleCounter						*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Uses pentium RDTSC instruction to get current cycle count since		*/
/*  chip was powered. This routine returns the number of cycles			*/
/*  since the strat routine was called.									*/
/*																		*/
/*  DANGER DANGER Will Robinson - these routines will only work when	*/
/*  used on a Pentium processor. Also, these probably should be macros	*/
/*  so subroutine calls don't skew the results. They should be close	*/
/*  enough for profiling though.										*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  cycleCount - this is where we will store the cycle count since 		*/
/*               call to start routine. On entry it should contain		*/
/*               a starting cycle count obtained with a call to the		*/
/*               start routine.											*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_StopPentiumCycleCounter(PLARGE_INTEGER cycleCount)
{
	LARGE_INTEGER	endCount;

	_asm
	{
		// the RDTSC instruction is not really documented,
		// so assemblers may not support it, so use _emit to
		// create instruction
		_emit	0Fh
		_emit	31h
		mov		endCount.HighPart, edx
		mov		endCount.LowPart,  eax
	}

	// figure up cycle count by subtracting starting count from ending count
	*cycleCount = RtlLargeIntegerSubtract(endCount, *cycleCount);
} // Debug_StopPentiumCycleCounter

#endif
