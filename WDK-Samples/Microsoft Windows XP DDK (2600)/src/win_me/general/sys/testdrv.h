/****************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

	testdrv.h

Abstract:

	This header file is for the specific driver file where you add
	routines to the WDM template driver

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

#ifndef _TESTDRV_
#define _TESTDRV_

// driver name and version
#define DRIVER_NAME				"TESTDRV"
#define DRIVER_VERSION			"1.00"

// the following define determines the IO method used, buffered or direct
// for reads and writes
#define DRIVER_IO_METHOD		DO_DIRECT_IO
// #define IO_METHOD	DO_BUFFERED_IO


// device extension for specific driver instance, used to store needed data

typedef struct _TESTDRV_DEV_EXTENSION
{
	BOOLEAN				Open;		// device open flag
	PUCHAR				Buffer;		// buffer for read/write in-memory loopback
	ULONG				BufferLen;	// length of buffer
} TESTDRV_DEV_EXTENSION, *PTESTDRV_DEV_EXTENSION;


// prototypes

NTSTATUS
OpenDriver(VOID);

VOID
CloseDriver(VOID);

NTSTATUS
TESTDRV_DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

NTSTATUS
TESTDRV_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

VOID
TESTDRV_Unload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS
TESTDRV_AddDevice(IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
TESTDRV_RemoveDevice(IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
TESTDRV_Create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
TESTDRV_Close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
TESTDRV_Write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
TESTDRV_Read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#endif // _TESTDRV_


