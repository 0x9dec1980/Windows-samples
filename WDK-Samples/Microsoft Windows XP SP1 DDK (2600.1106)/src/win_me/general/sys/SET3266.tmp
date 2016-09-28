/****************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

	testdrv.c

Abstract:

	This is where you put routines to add to the WDM template driver. 

	NOTE: notice the OpenDriver routine. This is where you fill
	in pointers to any routines of your own that you want called from
	the template driver. See vector.h and vector.c for more info. All of
	these pointers are initialized to NULL, so just fill in what you
	need. Also you can fill in the driver name and version here.

Environment:

	Kernel mode only

Notes:

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

	Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.

Revision History:

	10-30-96 : created

Author:

	Tom Green

****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wdm.h>
#include "testdrv.h"
#include "debugwdm.h"


/************************************************************************/
/*						InitDriver										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Initializes pointers to routines to call in driver specific files.	*/
/*  Do any other initialization needed for the device specific part		*/
/*  of driver.															*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	VOID																*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
OpenDriver(VOID)
{
	VectorDriverEntry	= TESTDRV_DriverEntry;
	VectorDispatch		= TESTDRV_Dispatch;
	VectorUnload		= TESTDRV_Unload;
	VectorAddDevice		= TESTDRV_AddDevice;
	VectorRemoveDevice	= TESTDRV_RemoveDevice;
	VectorCreate		= TESTDRV_Create;
	VectorClose			= TESTDRV_Close;
	VectorWrite			= TESTDRV_Write;
	VectorRead			= TESTDRV_Read;
	
	DriverName			= DRIVER_NAME;
	DriverVersion		= DRIVER_VERSION;

	return STATUS_SUCCESS;
} // OpenDriver


/************************************************************************/
/*						CloseDriver										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Undo things done in OpenDriver. This routine is called when the		*/
/*  driver unloads.														*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	VOID																*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
CloseDriver(VOID)
{
	// probably don't need to make these vectors NULL, but why not
	// could prevent my computer from exploding, you never know
	VectorDriverEntry	= NULL;
	VectorDispatch		= NULL;
	VectorUnload		= NULL;
	VectorAddDevice		= NULL;
	VectorRemoveDevice	= NULL;
	VectorCreate		= NULL;
	VectorClose			= NULL;
	VectorWrite			= NULL;
	VectorRead			= NULL;

	DriverName			= "";
	DriverVersion		= "";
} // CloseDriver


/************************************************************************/
/*						TESTDRV_DriverEntry								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  DriverEntry routine. Called when driver is loaded. Driver shell		*/
/*  takes care of setting up pointers for drive entry points.			*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DriverObject - pointer to a driver object							*/
/*																		*/
/*	RegistryPath - pointer to a unicode string representing the path	*/
/*				   to driver-specific key in the registry				*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
TESTDRV_DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS	ntStatus = STATUS_SUCCESS;
	
	DEBUG_LOG_PATH("enter TESTDRV_DriverEntry");

	DEBUG_LOG_PATH("exit  TESTDRV_DriverEntry");

	return ntStatus;
} // TESTDRV_DriverEntry


/************************************************************************/
/*						TESTDRV_Dispatch								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*	Process the IRPs sent to this device. In this case IOCTLs not		*/
/*  handled in the DRVSHELL_Dispatch routine. There may be a lot		*/
/*  of redundant pulling stuff out of IRP etc. but this is for			*/
/*  IOCTLs, so we are going for clarity, not speed. Make sure you set	*/
/*  IoStatus fields before leaving.										*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS. Return STATUS_INVALID_PARAMETER if we don't handle IOCTL.	*/
/*																		*/
/************************************************************************/
NTSTATUS
TESTDRV_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION	irpStack;
	PVOID				ioBuffer;
	ULONG				outputBufferLength;
	ULONG				ioControlCode;
	PCHAR				strBuff;

	DEBUG_LOG_PATH("enter TESTDRV_Dispatch");

	// Get a pointer to the current location in the Irp. This is where
	// the function codes and parameters are located.
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	// Get the pointer to the input/output buffer and it's length
	ioBuffer			= Irp->AssociatedIrp.SystemBuffer;
	outputBufferLength	= irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ioControlCode		= irpStack->Parameters.DeviceIoControl.IoControlCode;

	switch(ioControlCode)
	{
	case GET_DRIVER_INFO:
		// here's a little sample code for handling IOCTLs, this one
		// just expects a buffer filled with text returned, feel free to return
		// any info you want
		DEBUG_LOG_PATH("GET_DRIVER_INFO");

		// make sure buffer contains null terminator
		((PCHAR) ioBuffer)[0] = '\0';

		// get a temporaru buffer for formatting string
		strBuff = (PCHAR) Debug_MemAlloc(NonPagedPool, 0x100, FALSE);

		if(strBuff == NULL)
		{
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			Irp->IoStatus.Information = 0;
			break;
		}
			
		// driver name and version
		sprintf(strBuff, "\n\n\nDriver:	 %s\n\nVersion: %s\n\n", DRIVER_NAME, DRIVER_VERSION);

		// make sure it fits in buffer
		if(strlen(strBuff) < outputBufferLength)
			strcat(ioBuffer, strBuff);

		Irp->IoStatus.Information = strlen(ioBuffer);

		Debug_MemFree(strBuff);
		break;
	default:
		// not one we handle here
		DEBUG_LOG_PATH("Driver doesn't handle this IOCTL");
		ntStatus = STATUS_INVALID_PARAMETER;
		break;
	}
	
	DEBUG_LOG_PATH("exit  TESTDRV_Dispatch");

	return ntStatus;
} // TESTDRV_Dispatch


/************************************************************************/
/*						TESTDRV_Unload									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*	Process unloading driver. Undo what we did in TESTDRV_DriverEntry.	*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DriverObject - pointer to a driver object							*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
TESTDRV_Unload(IN PDRIVER_OBJECT DriverObject)
{
	DEBUG_LOG_PATH("enter TESTDRV_Unload");

	DEBUG_LOG_PATH("exit  TESTDRV_Unload");
} // TESTDRV_Unload


/************************************************************************/
/*						TESTDRV_AddDevice								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Add stuff to device extension specific to your driver.				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object of new device				*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS. Return STATUS_SUCCESS if it went O.K. You would probably	*/
/*            return STATUS_INSUFFICIENT_RESOURCES if alloc failed.		*/
/*																		*/
/************************************************************************/
NTSTATUS
TESTDRV_AddDevice(IN PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS				ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION		pDeviceExtension;
	PTESTDRV_DEV_EXTENSION	pTestdrvDevExtension;

	DEBUG_LOG_PATH("enter TESTDRV_AddDevice");

	pDeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	
	// this is where you allocate memory and add to and initialize device extension
	pTestdrvDevExtension = (PVOID) Debug_MemAlloc(NonPagedPool, sizeof(TESTDRV_DEV_EXTENSION), TRUE);

	// see if we allocated memory O.K.
	if(pTestdrvDevExtension == NULL)
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
	// got the memory, now initialize our device specific extension
	else
	{
		pTestdrvDevExtension->Open		= FALSE;
		pTestdrvDevExtension->Buffer	= NULL;
		pTestdrvDevExtension->BufferLen	= 0L;
	}
	
	// setup pointer in device extension
	pDeviceExtension->DeviceSpecificExt = (PVOID) pTestdrvDevExtension;

    // set flags for IO method for read/write
    // this define is in the header file for this file
	DeviceObject->Flags |= DRIVER_IO_METHOD;

	DEBUG_LOG_PATH("exit  TESTDRV_AddDevice");

	return ntStatus;
} // TESTDRV_AddDevice


/************************************************************************/
/*						TESTDRV_RemoveDevice							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Undo everyting done in AddDevice routine (free resources)			*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object of device to remove		*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS. Return STATUS_SUCCESS if it went O.K.						*/
/*																		*/
/************************************************************************/
NTSTATUS
TESTDRV_RemoveDevice(IN PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS				ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION		pDeviceExtension;
	PTESTDRV_DEV_EXTENSION	pTestdrvDevExtension;

	DEBUG_LOG_PATH("enter TESTDRV_RemoveDevice");

	// get pointer to device extension
	pDeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	
	// Get a pointer to the device specific extension
	pTestdrvDevExtension = (PTESTDRV_DEV_EXTENSION) pDeviceExtension->DeviceSpecificExt;

	// this is where you would free resources allocated in AddDevice routine
	if(pTestdrvDevExtension != NULL)
	{
		// let's free up the in-memory loopback read/write buffer if there is one
		if(pTestdrvDevExtension->Buffer != NULL)
		{
			Debug_MemFree(pTestdrvDevExtension->Buffer);
			pTestdrvDevExtension->Buffer	= NULL;
			pTestdrvDevExtension->BufferLen = 0L;
		}

		// free up device specific device extension
		Debug_MemFree((PVOID) pDeviceExtension->DeviceSpecificExt);
	}

	DEBUG_LOG_PATH("exit  TESTDRV_RemoveDevice");

	return ntStatus;
} // TESTDRV_RemoveDevice


/************************************************************************/
/*						TESTDRV_Create									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handle a create call on device.										*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS.															*/
/*																		*/
/************************************************************************/
NTSTATUS
TESTDRV_Create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS				ntStatus = STATUS_SUCCESS;
	PTESTDRV_DEV_EXTENSION	pTestdrvDevExtension;
	PDEVICE_EXTENSION		pDeviceExtension;
	
	DEBUG_LOG_PATH("enter TESTDRV_Create");

	// get pointer to device extension
	pDeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	
	// Get a pointer to the device specific extension
	pTestdrvDevExtension = (PTESTDRV_DEV_EXTENSION) pDeviceExtension->DeviceSpecificExt;

	// if device is open, don't open again
	if(pTestdrvDevExtension->Open)
	{
		ntStatus = STATUS_DEVICE_BUSY;
		Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
	}
	else
		pTestdrvDevExtension->Open = TRUE;

	DEBUG_LOG_PATH("exit  TESTDRV_Create");

	return ntStatus;
} // TESTDRV_Create


/************************************************************************/
/*						TESTDRV_Close									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handle a close call on device.										*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS.															*/
/*																		*/
/************************************************************************/
NTSTATUS
TESTDRV_Close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS				ntStatus = STATUS_SUCCESS;
	PTESTDRV_DEV_EXTENSION	pTestdrvDevExtension;
	PDEVICE_EXTENSION		pDeviceExtension;
	
	DEBUG_LOG_PATH("enter TESTDRV_Close");

	// get pointer to device extension
	pDeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	
	// Get a pointer to the device specific extension
	pTestdrvDevExtension = (PTESTDRV_DEV_EXTENSION) pDeviceExtension->DeviceSpecificExt;

	// let's free up the in-memory loopback read write/buffer if there is one
	if(pTestdrvDevExtension->Buffer != NULL)
	{
		Debug_MemFree(pTestdrvDevExtension->Buffer);
		pTestdrvDevExtension->Buffer	= NULL;
		pTestdrvDevExtension->BufferLen = 0L;
	}

	// mark device as not opened
	pTestdrvDevExtension->Open = FALSE;
	
	DEBUG_LOG_PATH("exit  TESTDRV_Close");

	return ntStatus;
} // TESTDRV_Close


/************************************************************************/
/*						TESTDRV_Write									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handle a write call on device.										*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS.															*/
/*																		*/
/************************************************************************/
NTSTATUS
TESTDRV_Write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS				ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION		irpStack;
	PVOID					ioBuffer;
	ULONG					bufferLength;
	PTESTDRV_DEV_EXTENSION	pTestdrvDevExtension;
	PDEVICE_EXTENSION		pDeviceExtension;
	LARGE_INTEGER			cycleCount;

	DEBUG_LOG_PATH("enter TESTDRV_Write");

	// get pointer to device extension
	pDeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	
	// Get a pointer to the device specific extension
	pTestdrvDevExtension = (PTESTDRV_DEV_EXTENSION) pDeviceExtension->DeviceSpecificExt;

	// allocate a buffer to hold the data to write
	// we will loop this back on read
	if(pTestdrvDevExtension->Buffer == NULL)
	{

		// Get a pointer to the current location in the Irp. This is where
		// the function codes and parameters are located.
		irpStack = IoGetCurrentIrpStackLocation(Irp);
		
		// Get the pointer to the input/output buffer,
		// depending on buffered or direct IO
		if(DeviceObject->Flags & DO_BUFFERED_IO)
		{
			ioBuffer = Irp->AssociatedIrp.SystemBuffer;

			// Get the length
			bufferLength = irpStack->Parameters.Write.Length;
		}
		// assume pages are already locked by shell routine
		else if(DeviceObject->Flags & DO_DIRECT_IO)
		{
			ioBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
			
			// Get the length
			bufferLength = MmGetMdlByteCount(Irp->MdlAddress);
		}
		
		pTestdrvDevExtension->Buffer = Debug_MemAlloc(NonPagedPool, bufferLength, FALSE);

		// save the length of the buffer
		pTestdrvDevExtension->BufferLen = bufferLength;
		
		// if pBuffer is NULL, then we couldn't allocate memory
		if(pTestdrvDevExtension->Buffer == NULL)
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		else
		{
			// just for fun, let's cycle count our RtlCopyMemory routine
#ifdef _X86_
			Debug_StartPentiumCycleCounter(&cycleCount);
#endif

			// lets copy buffer sent on write into our buffer
			// for loopback to read
			RtlCopyMemory(pTestdrvDevExtension->Buffer, ioBuffer, bufferLength);

			// stop the cycle count
#ifdef _X86_
			Debug_StopPentiumCycleCounter(&cycleCount);

			Debug_KdPrint(("RtlCopyMemory copied 0x%08x bytes in 0x%08x cycles\n",
						   bufferLength, cycleCount.LowPart));
#endif			
			Irp->IoStatus.Information = bufferLength;
		}
	}
	// must have a buffer allocated already, so device is busy
	else
		ntStatus = STATUS_DEVICE_BUSY;
	
	Irp->IoStatus.Status = ntStatus;
	
	DEBUG_LOG_PATH("exit  TESTDRV_Write");

	return ntStatus;
} // TESTDRV_Write


/************************************************************************/
/*						TESTDRV_Read									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handle a read call on device.										*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS.															*/
/*																		*/
/************************************************************************/
NTSTATUS
TESTDRV_Read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS				ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION		irpStack;
	PVOID					ioBuffer;
	ULONG					outputBufferLength;
	PTESTDRV_DEV_EXTENSION	pTestdrvDevExtension;
	PDEVICE_EXTENSION		pDeviceExtension;
	
	DEBUG_LOG_PATH("enter TESTDRV_Read");

	// get pointer to device extension
	pDeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	
	// Get a pointer to the device specific extension
	pTestdrvDevExtension = (PTESTDRV_DEV_EXTENSION) pDeviceExtension->DeviceSpecificExt;

	// see if we have a buffer
	if(pTestdrvDevExtension->Buffer != NULL)
	{
		// we have data, so copy to buffer for read

		// Get a pointer to the current location in the Irp. This is where
		// the function codes and parameters are located.
		irpStack = IoGetCurrentIrpStackLocation(Irp);
		
		// Get the pointer to the input/output buffer,
		// depending on buffered or direct IO
		if(DeviceObject->Flags & DO_BUFFERED_IO)
		{
			ioBuffer = Irp->AssociatedIrp.SystemBuffer;
			
			// Get the length
			outputBufferLength = irpStack->Parameters.Read.Length;
		}
		// assume pages are already locked by shell routine
		else if(DeviceObject->Flags & DO_DIRECT_IO)
		{
			ioBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);

			// Get the length
			outputBufferLength = MmGetMdlByteCount(Irp->MdlAddress);
		}
		

		// figure out how many bytes to copy
		outputBufferLength = min(outputBufferLength, pTestdrvDevExtension->BufferLen);
		
		// copy the data
		RtlCopyMemory(ioBuffer, pTestdrvDevExtension->Buffer, outputBufferLength);

		Irp->IoStatus.Information = outputBufferLength;
		
		// free up buffer allocated in write routine
		Debug_MemFree(pTestdrvDevExtension->Buffer);
	
		// set the buffer to NULL, so we allocate a new one next
		// time in the write routine. This will help ring out memory
		// allocation routines
		pTestdrvDevExtension->Buffer = NULL;
	
	}
	else
		ntStatus = STATUS_DEVICE_NOT_READY;

	Irp->IoStatus.Status = ntStatus;
	
	DEBUG_LOG_PATH("exit  TESTDRV_Read");

	return ntStatus;
} // TESTDRV_Read


