/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    eapwt.c

Abstract:

    This is a ExAllocatePoolWithTag sample driver.

Author:

    Paul Sanders Apr 18, 1995.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/


//
// Include files.
//

#include <ntddk.h>          // various NT definitions
#include <ntiologc.h>

#include <string.h>

#include "eapwt.h"

#if DBG
ULONG   EAPWTDebugLevel = 0;
#endif


NTSTATUS
DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    )
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

    It fills in the dispatch entry points in the driver object.  Then
    EAPWTInitialize is called to create the device object and complete
    the initialization.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if this disk is initialized; an error otherwise.

--*/

{
    NTSTATUS        ntStatus;
    UNICODE_STRING  paramPath;
    static  WCHAR   SubKeyString[] = L"\\Parameters";

    //
    // The registry path parameter points to our key, we will append
    // the Parameters key and look for any additional configuration items
    // there.  We add room for a trailing NUL for those routines which
    // require it.

    paramPath.MaximumLength = RegistryPath->Length + sizeof(SubKeyString);
    paramPath.Buffer = ExAllocatePool(PagedPool, paramPath.MaximumLength);

    if (paramPath.Buffer != NULL)
    {
        RtlMoveMemory(
            paramPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);

        RtlMoveMemory(
            &paramPath.Buffer[RegistryPath->Length / 2], SubKeyString,
            sizeof(SubKeyString));

        paramPath.Length = paramPath.MaximumLength;
    }
    else
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if DBG
    {
        //
        // We use this to query into the registry as to whether we
        // should break at driver entry.
        //
    
        RTL_QUERY_REGISTRY_TABLE    paramTable[3];
        ULONG                       zero = 0;
    
        ULONG                       debugLevel = 0;
        ULONG                       shouldBreak = 0;
    
        RtlZeroMemory(&paramTable[0], sizeof(paramTable));
    
        paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name = L"BreakOnEntry";
        paramTable[0].EntryContext = &shouldBreak;
        paramTable[0].DefaultType = REG_DWORD;
        paramTable[0].DefaultData = &zero;
        paramTable[0].DefaultLength = sizeof(ULONG);
        
        paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name = L"DebugLevel";
        paramTable[1].EntryContext = &debugLevel;
        paramTable[1].DefaultType = REG_DWORD;
        paramTable[1].DefaultData = &zero;
        paramTable[1].DefaultLength = sizeof(ULONG);
        
        if (!NT_SUCCESS(RtlQueryRegistryValues(
            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
            paramPath.Buffer, &paramTable[0], NULL, NULL)))
        {
            shouldBreak = 0;
            debugLevel = 0;
        }
        
        EAPWTDebugLevel = debugLevel;
        
        if (shouldBreak)
        {
            DbgBreakPoint();
        }
    }
#endif

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = EAPWTCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = EAPWTCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = EAPWTDeviceControl;
    DriverObject->DriverUnload = EAPWTUnloadDriver;

    ntStatus = EAPWTInitialize(DriverObject, &paramPath);

    //
    // We don't need that path anymore.
    //

    if (paramPath.Buffer)
    {
        ExFreePool( paramPath.Buffer );
    }

    return ntStatus;
}

NTSTATUS
EAPWTInitialize(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  ParamPath
    )

/*++

Routine Description:
    This routine is called at initialization time by DriverEntry().

    It creates and initializes a device object.

    The filename to be opened may be configured using the registry.  

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    ParamPath - a pointer to the Parameters key under our key in the Services
        section of the registry.

Return Value:
    STATUS_SUCCESS if this disk is initialized; an error otherwise.

--*/

{
    UNICODE_STRING      ntUnicodeString;    // NT Device Name "\Device\EAPWT"
    UNICODE_STRING      Win32NameString;    // Win32 Name "\DosDevices\TagTest"
    
    PDEVICE_OBJECT      deviceObject = NULL;    // ptr to device object
    
    NTSTATUS            ntStatus;

    RtlInitUnicodeString( &ntUnicodeString, NT_DEVICE_NAME );
    
    ntStatus = IoCreateDevice(
        DriverObject,                   // Our Driver Object
        0,                     			// We don't use a device extension
        &ntUnicodeString,               // Device name "\Device\EAPWT"
        FILE_DEVICE_UNKNOWN,       		// Device type
        0,                              // Device characteristics
        FALSE,                          // Not an exclusive device
        &deviceObject );                // Returned ptr to Device Object

    if ( !NT_SUCCESS( ntStatus ) )
    {
        EAPWTDump(
            EAPWTERRORS, ("EAPWT: Couldn't create the device object\n"));
        goto EAPWTInitializeExit;
    }
    
    //
    // Allocate and initialize a Unicode String containing the Win32 name
    // for our device.
    //

    RtlInitUnicodeString( &Win32NameString, DOS_DEVICE_NAME );

    //
    // Create a symbolic link between our device name "\Device\EAPWT" and
    // the Win32 name (ie "\DosDevices\EAPWT").
    //

    ntStatus = IoCreateSymbolicLink(
                        &Win32NameString, &ntUnicodeString );

                        
EAPWTInitializeExit:

    if ( !NT_SUCCESS( ntStatus ) )
    {
        //
        // Delete everything that this routine has allocated.
        //
        
        if ( deviceObject != NULL )
        {
            IoDeleteDevice( deviceObject );
        }
    }

    return ntStatus;
}

NTSTATUS
EAPWTCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when the EAPWT is opened or
    closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_INVALID_PARAMETER if parameters are invalid,
    STATUS_SUCCESS otherwise.

--*/

{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    return STATUS_SUCCESS;
}

VOID
EAPWTUnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine is called by the I/O system to unload the driver.

    Any resources previously allocated must be freed.

Arguments:

    DriverObject - a pointer to the object that represents our driver.

Return Value:

    None
--*/

{
    PDEVICE_OBJECT      deviceObject = DriverObject->DeviceObject;

    UNICODE_STRING uniWin32NameString;

    if ( deviceObject != NULL )
    {
        IoDeleteDevice( deviceObject );
    }

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );


    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //
    
    IoDeleteSymbolicLink( &uniWin32NameString );

}


NTSTATUS
EAPWTDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

Arguments:

    DeviceObject - a pointer to the object that represents the device
        that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_SUCCESS if recognized I/O control code,
    STATUS_INVALID_DEVICE_REQUEST otherwise.

--*/

{
	PALLOCATE_POOL Buffer;
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            ntStatus;

    //
    // Set up necessary object and extension pointers.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Assume failure.
    //

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;

    //
    // Determine which I/O control code was specified.
    //

    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode )
    {
    
    case IOCTL_EAPWT_LEAK_ALLOCATE_POOL:
        //
        // Allocate some memory using ExAllocatePoolwithTag.
        //

        if ( irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof( ALLOCATE_POOL ) )
        {

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            
        }
		else
		{

			Buffer = Irp->AssociatedIrp.SystemBuffer;

			Buffer->Address = ExAllocatePoolWithTag ( NonPagedPool, 
					6 * 1024, Buffer->Tag );
					
			if ( Buffer->Address )
			{
			
				Irp->IoStatus.Status = STATUS_SUCCESS;
    	    	Irp->IoStatus.Information = sizeof( ALLOCATE_POOL );
			}
			else
			{

				Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

			}

        }
        break;

    case IOCTL_EAPWT_LEAK_DEALLOCATE_POOL: 
        //
        // Deallocate the memory that was previously allocated 
        //

        if ( irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof( PCHAR ) )
        {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        }
        else
        {

			Buffer = Irp->AssociatedIrp.SystemBuffer;

			ExFreePool ( Buffer->Address );

			Irp->IoStatus.Status = STATUS_SUCCESS;
        }
        break;


    default:
        //
        // The specified I/O control code is unrecognized by this driver.
        // The I/O status field in the IRP has already been set so just
        // terminate the switch.
        //

        EAPWTDump(
        EAPWTDIAG1,
            ("EAPWT: ERROR:  unrecognized IOCTL %x\n",
            irpSp->Parameters.DeviceIoControl.IoControlCode));

        break;
    }

    //
    // Finish the I/O operation by simply completing the packet and returning
    // the same status as in the packet itself.
    //

    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return ntStatus;
}
