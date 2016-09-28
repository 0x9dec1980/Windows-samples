/***************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

	drvshell.c

Abstract:

	Template driver for WDM 

Environment:

	kernel mode only

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

#include <wdm.h>
#include <stdio.h>
#include "debugwdm.h"



// we will support 256 devices, keep track of open slots here
#define NUM_DEVICE_SLOTS		256

LOCAL BOOLEAN Slots[NUM_DEVICE_SLOTS];




/************************************************************************/
/*						DriverEntry										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/* Installable driver initialization entry point.						*/
/* This entry point is called directly by the I/O system.				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DriverObject - pointer to the driver object							*/
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
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	static BOOLEAN		initd = FALSE;
	PDEVICE_OBJECT		pDeviceObject = NULL;
	PDEVICE_EXTENSION	deviceExtension;

	// BUG BUG - why does DriverEntry get entered twice if we load driver at start
	// and then plug in PnP device that uses this driver. It is the same instance
	// of the driver, but you get a different driver object
	if(!initd)
	{
		// initialize calls to new driver routines, etc.
		ntStatus = OpenDriver();
	
		if(!NT_SUCCESS(ntStatus))
		{
			Debug_KdPrint(("exit DriverEntry: OpenDriver call failed (%x)\n", ntStatus));
			return ntStatus;
		}
	
		// initialize diagnostic stuff (history, tracing, error logging)
		ntStatus = Debug_OpenWDMDebug();

		if(!NT_SUCCESS(ntStatus))
		{
			Debug_KdPrint(("exit DriverEntry: Debug_OpenWDMDebug call failed (%x)\n", ntStatus));
			return ntStatus;
		}
	}

	// chicken before the egg problem, can't call Debug_Path until we initialize (above)
	DEBUG_LOG_PATH("enter DriverEntry");
	
	Debug_KdPrint(("DriverObject = %0x08x\n", DriverObject));
	Debug_KdPrint(("DeviceObject = 0x%08x\n", DriverObject->DeviceObject));

	// Create dispatch points for device control, create, close, etc.

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DRVSHELL_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_CREATE]		   = DRVSHELL_Create;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]		   = DRVSHELL_Close;
	DriverObject->MajorFunction[IRP_MJ_PNP]	   		   = DRVSHELL_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_POWER]	   	   = DRVSHELL_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_WRITE]		   = DRVSHELL_Write;
	DriverObject->MajorFunction[IRP_MJ_READ]		   = DRVSHELL_Read;
	DriverObject->DriverUnload						   = DRVSHELL_Unload;
	DriverObject->DriverExtension->AddDevice		   = DRVSHELL_PnPAddDevice;
	
	// call device specific DriverEntry if there is one
	if(VectorDriverEntry != NULL)
		ntStatus = (*VectorDriverEntry)(DriverObject, RegistryPath);

	if(!initd)
	{
		// let's create a device object named the same as the driver
		ntStatus = DRVSHELL_CreateDeviceObject(DriverObject, &pDeviceObject, DriverName);

		// check for error, remove the device if needed
    	if(!NT_SUCCESS(ntStatus))
    	{
			DEBUG_LOG_PATH("Failed to create device object");
			DRVSHELL_RemoveDevice(DriverObject->DeviceObject);
		}
		else
		{
			// make sure we don't mess up when the object is removed
			// and write past the end of our slot array

			// Get a pointer to the device extension
			deviceExtension = pDeviceObject->DeviceExtension;

			// set to invalid slot value
			deviceExtension->Instance = NUM_DEVICE_SLOTS;
		}

		initd = TRUE;
	}


	// log an error if we got one
	DEBUG_LOG_ERROR(ntStatus);

	DEBUG_LOG_PATH("exit  DriverEntry");

	Debug_KdPrint(("status = (%x)\n", ntStatus));
	
	return ntStatus;
} // DriverEntry


/************************************************************************/
/*						DRVSHELL_Dispatch								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*	Process the IRPs sent to this device. In this case IOCTLs and		*/
/*  PNP_POWER IOCTLs													*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
DRVSHELL_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION	irpStack, nextStack;
	PDEVICE_EXTENSION	deviceExtension;
	PVOID				ioBuffer;
	ULONG				outputBufferLength;
	ULONG				inputBufferLength;
	ULONG				ioControlCode;


	DEBUG_LOG_PATH("enter DRVSHELL_Dispatch");

	// set return values to something known
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	// Get a pointer to the current location in the Irp. This is where
	// the function codes and parameters are located.
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	// Get a pointer to the device extension
	deviceExtension = DeviceObject->DeviceExtension;
	
	// Get the pointer to the input/output buffer and it's length
	ioBuffer		   = Irp->AssociatedIrp.SystemBuffer;
	outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;

	// make entry in IRP history table
	DEBUG_LOG_IRP_HIST(DeviceObject, Irp, irpStack->MajorFunction, ioBuffer, outputBufferLength);
	
	switch(irpStack->MajorFunction)
	{
	case IRP_MJ_DEVICE_CONTROL:
		
		DEBUG_LOG_PATH("IRP_MJ_DEVICE_CONTROL");

		ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

		switch(ioControlCode)
		{
		case GET_DRIVER_LOG:
			DEBUG_LOG_PATH("GET_DRIVER_LOG");

			// make sure buffer contains null terminator
			((PCHAR) ioBuffer)[0] = '\0';
			Irp->IoStatus.Information = Debug_DumpDriverLog(DeviceObject, ioBuffer, outputBufferLength);

			break;

		case GET_IRP_HIST:
			DEBUG_LOG_PATH("GET_IRP_HIST");

			// make sure buffer contains null terminator
			((PCHAR) ioBuffer)[0] = '\0';
			Irp->IoStatus.Information = Debug_ExtractIRPHist(ioBuffer, outputBufferLength);
			
			break;

		case GET_PATH_HIST:
			DEBUG_LOG_PATH("GET_PATH_HIST");

			// make sure buffer contains null terminator
			((PCHAR) ioBuffer)[0] = '\0';
			Irp->IoStatus.Information = Debug_ExtractPathHist(ioBuffer, outputBufferLength);
			
			break;

		case GET_ERROR_LOG:
			DEBUG_LOG_PATH("GET_ERROR_LOG");

			// make sure buffer contains null terminator
			((PCHAR) ioBuffer)[0] = '\0';
			Irp->IoStatus.Information = Debug_ExtractErrorLog(ioBuffer, outputBufferLength);
			
			break;
			
		case GET_ATTACHED_DEVICES:
			DEBUG_LOG_PATH("GET_ATTACHED_DEVICES");

			// make sure buffer contains null terminator
			((PCHAR) ioBuffer)[0] = '\0';
			Irp->IoStatus.Information = Debug_ExtractAttachedDevices(DeviceObject->DriverObject, ioBuffer, outputBufferLength);

			break;

		case SET_IRP_HIST_SIZE:
			if(inputBufferLength != sizeof(ULONG))
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
			else
				ntStatus = Debug_SizeIRPHistoryTable(*((ULONG *) ioBuffer));
			break;

		case SET_PATH_HIST_SIZE:
			if(inputBufferLength != sizeof(ULONG))
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
			else
				ntStatus = Debug_SizeDebugPathHist(*((ULONG *) ioBuffer));
			break;

		case SET_ERROR_LOG_SIZE:
			if(inputBufferLength != sizeof(long))
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
			else
				ntStatus = Debug_SizeErrorLog(*((ULONG *) ioBuffer));
			break;

		default:
			// call into vectored routine to try and handle unknown IOCTLS
			// this routine should return STATUS_INVALID_PARAMETER if
			// this is not handled, and this will be passed on by falling
			// out of switch
			DEBUG_LOG_PATH("DRVSHELL doesn't handle this IOCTL");
			if(VectorDispatch)
				ntStatus = (*VectorDispatch)(DeviceObject, Irp);

			break;
		}

		// these IOCTLs will complete unless the status is pending
		Irp->IoStatus.Status = ntStatus;

		// complete I/O if status is not pending
		if(ntStatus != STATUS_PENDING)
			DRVSHELL_CompleteIO(DeviceObject, Irp, irpStack->MajorFunction,
								ioBuffer, outputBufferLength);
		else
			DEBUG_LOG_PATH("STATUS_PENDING");

		break;
		
	case IRP_MJ_INTERNAL_DEVICE_CONTROL:		

		// pass through to driver below
				
		DEBUG_LOG_PATH("IRP_MJ_INTERNAL_DEVICE_CONTROL");
		
		nextStack = IoGetNextIrpStackLocation(Irp);

		DEBUG_ASSERT("nextStack pointer is NULL", nextStack != NULL);

		RtlCopyMemory(nextStack, irpStack, sizeof(IO_STACK_LOCATION));
		
		IoMarkIrpPending(Irp);

		ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

		DEBUG_LOG_PATH("Passed PnP Irp down");

		Debug_KdPrint(("status = (%x)\n", ntStatus));

		// BUG BUG - what should happen here
		break;
		
	case IRP_MJ_PNP:
	
		DEBUG_LOG_PATH("IRP_MJ_PNP_POWER");
	
		switch(irpStack->MinorFunction)
		{
			case IRP_MN_START_DEVICE:

				DEBUG_LOG_PATH("IRP_MN_START_DEVICE");
				deviceExtension->Stopped = FALSE;
				break;
			
			case IRP_MN_STOP_DEVICE:

				DEBUG_LOG_PATH("IRP_MN_STOP_DEVICE");
				deviceExtension->Stopped = TRUE;
				break;

			case IRP_MN_REMOVE_DEVICE:

				DEBUG_LOG_PATH("IRP_MN_REMOVE_DEVICE");

				ntStatus = DRVSHELL_RemoveDevice(DeviceObject);

				break;

			default:

				DEBUG_LOG_PATH("PNP IOCTL not handled");

				break;
				
		}

		// All PNP POWER messages get passed to StackDeviceObject.

		nextStack = IoGetNextIrpStackLocation(Irp);

		DEBUG_ASSERT("nextStack pointer is NULL", nextStack != NULL);

		RtlCopyMemory(nextStack, irpStack, sizeof(IO_STACK_LOCATION));
		
		IoMarkIrpPending(Irp);
		break;
	default:
		// not an IOCTL we handle
		ntStatus = STATUS_INVALID_PARAMETER;
		DRVSHELL_CompleteIO(DeviceObject, Irp, irpStack->MajorFunction,
							ioBuffer, outputBufferLength);

	}

	// log an error if we got one
	DEBUG_LOG_ERROR(ntStatus);

	DEBUG_LOG_PATH("exit  DRVSHELL_Dispatch");

	Debug_KdPrint(("status = (%x)\n", ntStatus));
	
	return ntStatus;
} // DRVSHELL_Dispatch


/************************************************************************/
/*						DRVSHELL_Create									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*	Process the IRPs sent to this device for Create calls				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
DRVSHELL_Create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION	irpStack;
	PDEVICE_EXTENSION	deviceExtension;
	PVOID				ioBuffer;

	DEBUG_LOG_PATH("enter DRVSHELL_Create");
	
	// set return values to something known
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	// Get a pointer to the current location in the Irp. This is where
	// the function codes and parameters are located.
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	// Get a pointer to the device extension
	deviceExtension = DeviceObject->DeviceExtension;

	// Get the pointer to the input/output buffer and it's length
	ioBuffer = Irp->AssociatedIrp.SystemBuffer;

	// make entry in IRP history table
	DEBUG_LOG_IRP_HIST(DeviceObject, Irp, irpStack->MajorFunction, ioBuffer, 0);

	// call device specific create if there is one
	if(VectorCreate != NULL)
		ntStatus = (*VectorCreate)(DeviceObject, Irp);
	
	// complete IO and make entry in IRP history table
	if(ntStatus != STATUS_PENDING)
		DRVSHELL_CompleteIO(DeviceObject, Irp, irpStack->MajorFunction, ioBuffer, 0);
	else
		DEBUG_LOG_PATH("STATUS_PENDING");

	// log an error if we got one
	DEBUG_LOG_ERROR(ntStatus);

	DEBUG_LOG_PATH("exit  DRVSHELL_Create");

	Debug_KdPrint(("status = (%x)\n", ntStatus));
	
	return ntStatus;
} // DRVSHELL_Create


/************************************************************************/
/*						DRVSHELL_Close									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*	Process the IRPs sent to this device for Close calls				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
DRVSHELL_Close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION	irpStack;
	PDEVICE_EXTENSION	deviceExtension;
	PVOID				ioBuffer;

	DEBUG_LOG_PATH("enter DRVSHELL_Close");
	
	// set return values to something known
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	// Get a pointer to the current location in the Irp. This is where
	// the function codes and parameters are located.
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	// Get a pointer to the device extension
	deviceExtension = DeviceObject->DeviceExtension;

	// Get the pointer to the input/output buffer
	ioBuffer = Irp->AssociatedIrp.SystemBuffer;

	// make entry in IRP history table
	DEBUG_LOG_IRP_HIST(DeviceObject, Irp, irpStack->MajorFunction, ioBuffer, 0);

	// call device specific close if there is one
	if(VectorClose != NULL)
		ntStatus = (*VectorClose)(DeviceObject, Irp);
	
	// complete IO and make entry in IRP history table
	if(ntStatus != STATUS_PENDING)
		DRVSHELL_CompleteIO(DeviceObject, Irp, irpStack->MajorFunction, ioBuffer, 0);
	else
		DEBUG_LOG_PATH("STATUS_PENDING");

	// log an error if we got one
	DEBUG_LOG_ERROR(ntStatus);

	DEBUG_LOG_PATH("exit  DRVSHELL_Close");

	Debug_KdPrint(("status = (%x)\n", ntStatus));
	
	return ntStatus;
} // DRVSHELL_Close


/************************************************************************/
/*						DRVSHELL_Write									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*	Process the IRPs sent to this device for Write calls				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
DRVSHELL_Write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION	irpStack;
	PDEVICE_EXTENSION	deviceExtension;
	PVOID				ioBuffer;
	PMDL				pMdl;
	ULONG				bufferLength = 0;
	
	DEBUG_LOG_PATH("enter DRVSHELL_Write");
	
	// Get a pointer to the current location in the Irp. This is where
	// the function codes and parameters are located.
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	// Get a pointer to the device extension
	deviceExtension = DeviceObject->DeviceExtension;

	// Get the pointer to the input/output buffer, depending on buffered or direct IO
	if(DeviceObject->Flags & DO_BUFFERED_IO)
	{
		ioBuffer = Irp->AssociatedIrp.SystemBuffer;

		// check for NULL pointer
		if(ioBuffer != NULL)
			// Get the length
			bufferLength = irpStack->Parameters.Write.Length;
	}
	else if(DeviceObject->Flags & DO_DIRECT_IO)
	{
		pMdl = Irp->MdlAddress;
		
		// check for NULL pointer
		if(pMdl == NULL)
			ioBuffer = NULL;
		else
		{
			// lock pages
			MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
			// get system address
			ioBuffer = MmGetSystemAddressForMdl(pMdl);

			// Get the length
			bufferLength = MmGetMdlByteCount(Irp->MdlAddress);
		}
	}

	// make entry in IRP history table
	DEBUG_LOG_IRP_HIST(DeviceObject, Irp, irpStack->MajorFunction, ioBuffer, bufferLength);

	// if our buffer is NULL, or it is 0 in length the we have an error
	if(ioBuffer == NULL || bufferLength == 0)
	{
		ntStatus = STATUS_INVALID_BUFFER_SIZE;
		DEBUG_LOG_PATH("STATUS_INVALID_BUFFER_SIZE");	
	}	
	else
	{
		// call device specific write if there is one
		if(VectorWrite != NULL)
			ntStatus = (*VectorWrite)(DeviceObject, Irp);
		// BUG BUG - not sure what to do here if they don't have a routine, return success for now
		else
		{
			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = 0;
		}
	}
	
	// complete IO and make entry in IRP history table
	if(ntStatus != STATUS_PENDING)
		DRVSHELL_CompleteIO(DeviceObject, Irp, irpStack->MajorFunction, ioBuffer, bufferLength);
	else
		DEBUG_LOG_PATH("STATUS_PENDING");

	// unlock Mdl pages if needed
	if((DeviceObject->Flags & DO_DIRECT_IO) && pMdl)
		MmUnlockPages(pMdl);

	// log an error if we got one
	DEBUG_LOG_ERROR(ntStatus);

	DEBUG_LOG_PATH("exit  DRVSHELL_Write");

	Debug_KdPrint(("status = (%x)\n", ntStatus));
	
	return ntStatus;
} // DRVSHELL_Write


/************************************************************************/
/*						DRVSHELL_Read									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*	Process the IRPs sent to this device for Read calls					*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DeviceObject - pointer to a device object							*/
/*																		*/
/*	Irp          - pointer to an I/O Request Packet						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
DRVSHELL_Read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION	irpStack;
	PDEVICE_EXTENSION	deviceExtension;
	PVOID				ioBuffer;
	PMDL				pMdl;
	ULONG				bufferLength = 0;
	
	DEBUG_LOG_PATH("enter DRVSHELL_Read");
	
	// Get a pointer to the current location in the Irp. This is where
	// the function codes and parameters are located.
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	// Get a pointer to the device extension
	deviceExtension = DeviceObject->DeviceExtension;

	// Get the pointer to the input/output buffer,
	// depending on buffered or direct IO
	if(DeviceObject->Flags & DO_BUFFERED_IO)
	{
		ioBuffer = Irp->AssociatedIrp.SystemBuffer;

		// check for NULL pointer
		if(ioBuffer != NULL)
			// Get the length
			bufferLength = irpStack->Parameters.Read.Length;
	}
	else if(DeviceObject->Flags & DO_DIRECT_IO)
	{
		pMdl = Irp->MdlAddress;

		// check for NULL pointer
		if(pMdl == NULL)
			ioBuffer = NULL;
		else
		{
			// lock pages
			MmProbeAndLockPages(pMdl, KernelMode, IoModifyAccess);
			// get system address
			ioBuffer = MmGetSystemAddressForMdl(pMdl);

			// Get the length
			bufferLength = MmGetMdlByteCount(Irp->MdlAddress);
		}
	}
	
	// make entry in IRP history table
	DEBUG_LOG_IRP_HIST(DeviceObject, Irp, irpStack->MajorFunction, ioBuffer, bufferLength);

	// if our buffer is NULL, or it is 0 in length the we have an error
	if(ioBuffer == NULL || bufferLength == 0)
	{
		ntStatus = STATUS_INVALID_BUFFER_SIZE;
		DEBUG_LOG_PATH("STATUS_INVALID_BUFFER_SIZE");
	}
	else
	{
		// call device specific read if there is one
		if(VectorRead != NULL)
			ntStatus = (*VectorRead)(DeviceObject, Irp);
		// BUG BUG - not sure what to do here if they don't have a routine, return success for now
		else
		{
			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = 0;
		}
	}
	
	// complete IO and make entry in IRP history table
	if(ntStatus != STATUS_PENDING)
		DRVSHELL_CompleteIO(DeviceObject, Irp, irpStack->MajorFunction, ioBuffer, bufferLength);
	else
		DEBUG_LOG_PATH("STATUS_PENDING");

	// unlock Mdl pages if needed
	if((DeviceObject->Flags & DO_DIRECT_IO) && pMdl)
		MmUnlockPages(pMdl);

	// log an error if we got one
	DEBUG_LOG_ERROR(ntStatus);

	DEBUG_LOG_PATH("exit  DRVSHELL_Read");

	Debug_KdPrint(("status = (%x)\n", ntStatus));
	
	return ntStatus;
} // DRVSHELL_Read


/************************************************************************/
/*						DRVSHELL_Unload									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*	Process unloading driver											*/
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
DRVSHELL_Unload(IN PDRIVER_OBJECT DriverObject)
{
	DEBUG_LOG_PATH("enter DRVSHELL_Unload");

	// BUG BUG - why doesn't this routine ever get called

	// call driver specific routine to shut down driver and release
	// resources allocated by OpenDriver
	CloseDriver();

	// shut down debugging and release resources
	Debug_CloseWDMDebug();

	// can't call DebugLogPath anymore
	Debug_KdPrint(("exit  DRVSHELL_Unload\n"));
} // DRVSHELL_Unload


/************************************************************************/
/*						DRVSHELL_PnPAddDevice							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Attach new device to driver											*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DriverObject - pointer to the driver object							*/
/*																		*/
/*	DeviceObject - pointer to the device object							*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
DRVSHELL_PnPAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	ULONG				DeviceInstance;
	CHAR				DeviceName[80];
	PDEVICE_OBJECT		pDeviceObject = NULL;
	PDEVICE_EXTENSION	deviceExtension;

	DEBUG_LOG_PATH("enter DRVSHELL_PnPAddDevice");

	Debug_KdPrint(("DriverObject = %0x08x\n", DriverObject));
	Debug_KdPrint(("DeviceObject = 0x%08x\n", DeviceObject));

	// let's make a name for our device
	strcpy(DeviceName, DriverName);

	// let's get an instance
	for(DeviceInstance = 0; DeviceInstance < NUM_DEVICE_SLOTS; DeviceInstance++)
	{
		if(Slots[DeviceInstance] == FALSE)
			break;
	}

	// check if we didn't have any empty slots
	if(DeviceInstance == NUM_DEVICE_SLOTS)
	{
		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
		goto AddDevice_End;
	}

	// make an instance
	sprintf(DeviceName, "%s%03d", DriverName, DeviceInstance);

	ntStatus = DRVSHELL_CreateDeviceObject(DriverObject, &pDeviceObject, DeviceName);

	// check for error, remove the device if needed or attach it
    if(NT_SUCCESS(ntStatus))
    {
		// Get a pointer to the device extension
		deviceExtension = DeviceObject->DeviceExtension;

		// Attach to the PDO
		deviceExtension->StackDeviceObject =
			IoAttachDeviceToDeviceStack(pDeviceObject, DeviceObject);

		deviceExtension->Instance = DeviceInstance;
		Slots[DeviceInstance] = TRUE;


		// reset device initializating flag
		pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	}
	else
    {
		DEBUG_LOG_PATH("Failed to create device object");
		DRVSHELL_RemoveDevice(DeviceObject);
	}

AddDevice_End:

	// log an error if we got one
	DEBUG_LOG_ERROR(ntStatus);

	DEBUG_LOG_PATH("exit  DRVSHELL_PnPAddDevice");

	Debug_KdPrint(("status = (%x)\n", ntStatus));

	return ntStatus;
} // DRVSHELL_PnPAddDevice


/************************************************************************/
/*						DRVSHELL_CompleteIO								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Complete IO request and log IRP										*/
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
DRVSHELL_CompleteIO(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp,
					IN ULONG majorFunction, IN PVOID ioBuffer, IN ULONG bufferLen)
{
	PDEVICE_EXTENSION	deviceExtension;

	DEBUG_LOG_PATH("enter DRVSHELL_CompleteIO");

	// get pointer to device extension
	deviceExtension = (PDEVICE_EXTENSION) pDeviceObject->DeviceExtension;

	// log IRP count and bytes processed in device extension
	deviceExtension->IRPCount++;
	deviceExtension->ByteCount = RtlLargeIntegerAdd(deviceExtension->ByteCount,
									RtlConvertUlongToLargeInteger(
										pIrp->IoStatus.Information));
	
	// make entry in IRP history table
	DEBUG_LOG_IRP_HIST(pDeviceObject, pIrp, majorFunction, ioBuffer, bufferLen);

	// if we got here, must want to complete request on IRP
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	DEBUG_LOG_PATH("exit  DRVSHELL_CompleteIO");
} // DRVSHELL_CompleteIO


/************************************************************************/
/*						DRVSHELL_RemoveDevice							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Remove device														*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  pDeviceObject - pointer to device object.							*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
DRVSHELL_RemoveDevice(IN PDEVICE_OBJECT pDeviceObject)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION	deviceExtension;
	UNICODE_STRING		deviceLinkUnicodeString;
	ANSI_STRING			deviceLinkAnsiString;
	
	DEBUG_LOG_PATH("enter DRVSHELL_RemoveDevice");

	Debug_KdPrint(("DeviceObject = 0x%08x\n", pDeviceObject));

	// Get a pointer to the device extension
	deviceExtension = pDeviceObject->DeviceExtension;

	// detach device
	if(deviceExtension->StackDeviceObject)
		IoDetachDevice(deviceExtension->StackDeviceObject);

	Debug_KdPrint(("Delete SymLink (%s)\n", deviceExtension->LinkName));

	// delete symbolic link
	RtlInitAnsiString(&deviceLinkAnsiString, deviceExtension->LinkName);
	ntStatus = RtlAnsiStringToUnicodeString(&deviceLinkUnicodeString,
											&deviceLinkAnsiString, TRUE);
	IoDeleteSymbolicLink(&deviceLinkUnicodeString);
				
	// call device specific remove routine if there is one
	if(VectorRemoveDevice != NULL)
		ntStatus = (*VectorRemoveDevice)(pDeviceObject);

	Debug_KdPrint(("Delete Device (%s)\n", deviceExtension->DeviceName));
	
	// delete device object
	IoDeleteDevice(pDeviceObject);

	// free Uicode string
	RtlFreeUnicodeString(&deviceLinkUnicodeString);

	// clear out slot
	if(deviceExtension->Instance < NUM_DEVICE_SLOTS)
		Slots[deviceExtension->Instance] = FALSE;

	DEBUG_LOG_PATH("exit  DRVSHELL_RemoveDevice");

	return ntStatus;
} // DRVSHELL_RemoveDevice


/************************************************************************/
/*						DRVSHELL_CreateDeviceObject						*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Attach new device to driver											*/
/*																		*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DriverObject - pointer to the driver object							*/
/*																		*/
/*	DeviceObject - pointer to the device object							*/
/*																		*/
/*  DeviceName   - ASCII string name of device							*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
DRVSHELL_CreateDeviceObject(IN PDRIVER_OBJECT DriverObject, IN OUT PDEVICE_OBJECT *DeviceObject,
							IN PCHAR DeviceName)
{
	ANSI_STRING			devName;
	ANSI_STRING			linkName;
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	UNICODE_STRING		deviceNameUnicodeString;
	UNICODE_STRING		linkNameUnicodeString;
	PDEVICE_EXTENSION	deviceExtension;
	CHAR				DeviceLinkBuffer[NAME_MAX];
	CHAR				DeviceNameBuffer[NAME_MAX];


	DEBUG_LOG_PATH("enter DRVSHELL_CreateDeviceObject");

	strcpy(DeviceLinkBuffer, "\\DosDevices\\");
	strcpy(DeviceNameBuffer, "\\Device\\");
	
	// complete names of links and devices
	strcat(DeviceLinkBuffer, DeviceName);
	strcat(DeviceNameBuffer, DeviceName);
	
	// init ANSI string with our link and device names
	RtlInitAnsiString(&devName, DeviceNameBuffer);
	RtlInitAnsiString(&linkName, DeviceLinkBuffer);

	// convert to UNICODE string
	ntStatus = RtlAnsiStringToUnicodeString(&deviceNameUnicodeString, &devName, TRUE);
	ntStatus = RtlAnsiStringToUnicodeString(&linkNameUnicodeString, &linkName, TRUE);

	Debug_KdPrint(("Create Device (%s)\n", DeviceNameBuffer));

	// create the device object
	ntStatus = IoCreateDevice(DriverObject,
							  sizeof(DEVICE_EXTENSION),
							  &deviceNameUnicodeString,
							  FILE_DEVICE_UNKNOWN,
							  0,
							  FALSE,
							  DeviceObject);

	// created the device object O.K., create symbolic links, attach device object,
	// and fill in the device extension
	if(NT_SUCCESS(ntStatus))
	{

		// create symbolic links

		Debug_KdPrint(("Create SymLink (%s)\n", DeviceLinkBuffer));


		ntStatus = IoCreateUnprotectedSymbolicLink(&linkNameUnicodeString,
												   &deviceNameUnicodeString);

		// get pointer to device extension
		deviceExtension = (PDEVICE_EXTENSION) (*DeviceObject)->DeviceExtension;

		// let's zero out device extension
		RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

		// init device extension
		deviceExtension->Stopped = TRUE;

		// save our strings
		strcpy(deviceExtension->DeviceName, DeviceNameBuffer);
		strcpy(deviceExtension->LinkName, DeviceLinkBuffer);

		// save physical device object
		deviceExtension->PhysDeviceObject = *DeviceObject;

		// call vectored routine so you can add you own stuff to device extension
		if(VectorAddDevice != NULL)
			ntStatus = (*VectorAddDevice)(*DeviceObject);
	}

	// free Unicode strings
	RtlFreeUnicodeString(&deviceNameUnicodeString);
	RtlFreeUnicodeString(&linkNameUnicodeString);
	
	// log an error if we got one
	DEBUG_LOG_ERROR(ntStatus);

	DEBUG_LOG_PATH("exit  DRVSHELL_CreateDeviceObject");

	Debug_KdPrint(("status = (%x)\n", ntStatus));

	return ntStatus;
} // DRVSHELL_CreateDeviceObject

