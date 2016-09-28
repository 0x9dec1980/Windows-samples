/***************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

	vector.h

Abstract:

	Header file for vector.c. Pointers to routines in device specific part of 
	driver. Includes pointers for driver name and version.

Environment:

	kernel mode only

Notes:

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

	Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.


Revision History:

	10-29-96 : created

Author:

	Tom Green

	
****************************************************************************/

#ifndef _VECTOR_
#define _VECTOR_


// typedefs for pointers to routines called from the driver shell

typedef NTSTATUS (*VECTOR_DRIVERENTRY)(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
typedef NTSTATUS (*VECTOR_DISPATCH)(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
typedef VOID     (*VECTOR_UNLOAD)(IN PDRIVER_OBJECT DriverObject);
typedef NTSTATUS (*VECTOR_ADDDEVICE)(IN PDEVICE_OBJECT DeviceObject);
typedef NTSTATUS (*VECTOR_REMOVEDEVICE)(IN PDEVICE_OBJECT DeviceObject);
typedef NTSTATUS (*VECTOR_CREATE)(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
typedef NTSTATUS (*VECTOR_CLOSE)(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
typedef NTSTATUS (*VECTOR_WRITE)(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
typedef NTSTATUS (*VECTOR_READ)(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);


// strage for pointers to routines called from the driver shell

extern VECTOR_DRIVERENTRY		VectorDriverEntry;
extern VECTOR_DISPATCH			VectorDispatch;
extern VECTOR_UNLOAD			VectorUnload;
extern VECTOR_ADDDEVICE			VectorAddDevice;
extern VECTOR_REMOVEDEVICE		VectorRemoveDevice;
extern VECTOR_CREATE			VectorCreate;
extern VECTOR_CLOSE				VectorClose;
extern VECTOR_WRITE				VectorWrite;
extern VECTOR_READ				VectorRead;


// pointers to the name and version of the driver

extern PCHAR					DriverName;
extern PCHAR					DriverVersion;

#endif // _VECTOR_

