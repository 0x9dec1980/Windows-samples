/****************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

	drvshell.h

Abstract:

	This header file is used both by ring3 app and ring0 WDM template driver,
	hence the use of #ifdef DRIVER. Put IOCTLs called by ring3 application here.

Environment:

	Kernel mode & user mode

Notes:

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

	Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.

Revision History:

	10-25-96 : created

Author:

	Tom Green

****************************************************************************/

#ifndef _DRVSHELL_
#define _DRVSHELL_

#ifndef DRIVER

#include <winioctl.h>

#endif // DRIVER

// IOCTL info, needs to be visible for application

#define SHELLDRV_IOCTL_INDEX  0x0080


#define GET_DRIVER_LOG			CTL_CODE(FILE_DEVICE_UNKNOWN,		\
										 SHELLDRV_IOCTL_INDEX + 0,	\
										 METHOD_BUFFERED,			\
										 FILE_ANY_ACCESS)

#define GET_IRP_HIST			CTL_CODE(FILE_DEVICE_UNKNOWN,		\
										 SHELLDRV_IOCTL_INDEX + 1,	\
										 METHOD_BUFFERED,			\
										 FILE_ANY_ACCESS)

#define GET_PATH_HIST			CTL_CODE(FILE_DEVICE_UNKNOWN,		\
										 SHELLDRV_IOCTL_INDEX + 2,	\
										 METHOD_BUFFERED,			\
										 FILE_ANY_ACCESS)

#define GET_ERROR_LOG			CTL_CODE(FILE_DEVICE_UNKNOWN,		\
										 SHELLDRV_IOCTL_INDEX + 3,	\
										 METHOD_BUFFERED,			\
										 FILE_ANY_ACCESS)

#define GET_ATTACHED_DEVICES	CTL_CODE(FILE_DEVICE_UNKNOWN,		\
										 SHELLDRV_IOCTL_INDEX + 4,	\
										 METHOD_BUFFERED,			\
										 FILE_ANY_ACCESS)

#define SET_IRP_HIST_SIZE		CTL_CODE(FILE_DEVICE_UNKNOWN,		\
										 SHELLDRV_IOCTL_INDEX + 5,	\
										 METHOD_BUFFERED,			\
										 FILE_ANY_ACCESS)

#define SET_PATH_HIST_SIZE		CTL_CODE(FILE_DEVICE_UNKNOWN,		\
										 SHELLDRV_IOCTL_INDEX + 6,	\
										 METHOD_BUFFERED,			\
										 FILE_ANY_ACCESS)

#define SET_ERROR_LOG_SIZE		CTL_CODE(FILE_DEVICE_UNKNOWN,		\
										 SHELLDRV_IOCTL_INDEX + 7,	\
										 METHOD_BUFFERED,			\
										 FILE_ANY_ACCESS)

// we will handle this in test driver code for a sample of
// handling IOCTLs
#define GET_DRIVER_INFO			CTL_CODE(FILE_DEVICE_UNKNOWN,		\
										 SHELLDRV_IOCTL_INDEX + 8,	\
										 METHOD_BUFFERED,			\
										 FILE_ANY_ACCESS)

#ifdef DRIVER

// length of name for devices
#define NAME_MAX				0x80

// device extension for driver instance, used to store needed data

typedef struct _DEVICE_EXTENSION
{
	CHAR			 DeviceName[NAME_MAX];	// string name of device
	CHAR			 LinkName[NAME_MAX];	// string name of symbolic link
	PDEVICE_OBJECT	 PhysDeviceObject;		// physical device object
	PDEVICE_OBJECT	 StackDeviceObject;		// stack device object
	BOOLEAN			 Stopped;				// current state of device status
	ULONG			 IRPCount;				// number of IRPs completed with this device object
	LARGE_INTEGER	 ByteCount;				// number of bytes of data passed with this device object
	ULONG			 Instance;				// instance of device
	PVOID			 DeviceSpecificExt;		// pointer to extension things specific to device
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

NTSTATUS
DRVSHELL_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
DRVSHELL_Create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
DRVSHELL_Close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
DRVSHELL_Write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
DRVSHELL_Read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

VOID
DRVSHELL_Unload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS
DRVSHELL_PnPAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT DeviceObject);

VOID
DRVSHELL_CompleteIO(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp,
					IN ULONG majorFunction, IN PVOID ioBuffer, ULONG bufferLen);

NTSTATUS
DRVSHELL_RemoveDevice(IN PDEVICE_OBJECT pDeviceObject);


NTSTATUS
DRVSHELL_CreateDeviceObject(IN PDRIVER_OBJECT DriverObject, IN OUT PDEVICE_OBJECT *DeviceObject,
							IN PCHAR DeviceName);


// these routines must be in driver specific file

NTSTATUS
OpenDriver(VOID);

VOID
CloseDriver(VOID);

#endif // DRIVER
#endif // _DRVSHELL_

