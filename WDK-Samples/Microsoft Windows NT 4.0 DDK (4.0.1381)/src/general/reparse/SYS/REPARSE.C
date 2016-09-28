/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    reparse.c

Abstract:

    This is a STATUS_REPARSE sample driver.

Author:

    Paul Sanders May 4, 1995.

Environment:

    Kernel mode only.

Notes:

Revision History:

    Cleanup code for submission to DDK - Nov 21, 1995

--*/


//
// Include files.
//

#include <ntddk.h>          // various NT definitions

#include "reparse.h"

#if DBG
ULONG   ReparseDebugLevel = 0;
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
    ReparseInitialize is called to create the device object and complete
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
        RtlCopyMemory(
            paramPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);

        RtlCopyMemory(
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
        
        ReparseDebugLevel = debugLevel;
        
        if (shouldBreak)
        {
            DbgBreakPoint();
        }
    }
#endif

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ReparseCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ReparseClose;
    DriverObject->DriverUnload = ReparseUnloadDriver;

    ntStatus = ReparseInitialize(DriverObject, &paramPath);

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
ReparseInitialize(
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
    UNICODE_STRING      ntUnicodeString;    // NT Device Name "\Device\Reparse"
    UNICODE_STRING      Win32NameString;    // Win32 Name "\DosDevices\Rp"
    
    PDEVICE_OBJECT      deviceObject = NULL;    // ptr to device object
    PLOCAL_DEVICE_EXTENSION deviceExtension;
    
    NTSTATUS            ntStatus;

    RtlInitUnicodeString( &ntUnicodeString, NT_DEVICE_NAME );
    
    ntStatus = IoCreateDevice(
        DriverObject,                   // Our Driver Object
        sizeof (PLOCAL_DEVICE_EXTENSION),
        &ntUnicodeString,               // Device name "\Device\Reparse"
        FILE_DEVICE_UNKNOWN,       		// Device type
        0,                              // Device characteristics
        FALSE,                          // Not an exclusive device
        &deviceObject );                // Returned ptr to Device Object

    if ( !NT_SUCCESS( ntStatus ) )
    {
        ReparseDump(
            REPARSEERRORS, ("Reparse: Couldn't create the device object\n"));
        goto ReparseInitializeExit;
    }

    //
    // Configure the device extension for this device
    //

    deviceExtension = (PLOCAL_DEVICE_EXTENSION) deviceObject->DeviceExtension;

    RtlZeroMemory ( deviceExtension, sizeof (PLOCAL_DEVICE_EXTENSION));

    deviceExtension->SlaveDeviceObject = deviceExtension->MasterDeviceObject = deviceObject;
    
    //
    // Allocate and initialize a Unicode String containing the Win32 name
    // for our device.
    //

    RtlInitUnicodeString( &Win32NameString, DOS_DEVICE_NAME );

    //
    // Create a symbolic link between our device name "\Device\Reparse" and
    // the Win32 name (ie "\DosDevices\Rp").
    //

    ntStatus = IoCreateSymbolicLink(
                        &Win32NameString, &ntUnicodeString );

                        
ReparseInitializeExit:

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
ReparseCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when RP is opened.

    This routine is entered twice.  During the first pass, a new device 
    object, \Device\Reparse1, is created, Irp->IoStatus.Status is
    set to STATUS_REPARSE, and the FileName in the FileObject is changed
    to the new device objects name.  This informs the I/O Manger to
    reparse the open for the new device object.  This will cause this 
    routine to be called a second time.  During the second pass, simply
    return STATUS_SUCCESS.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_INVALID_PARAMETER if parameters are invalid,
    STATUS_SUCCESS otherwise.

--*/

{
    NTSTATUS        ntStatus;
    PLOCAL_DEVICE_EXTENSION deviceExtension;
    PLOCAL_DEVICE_EXTENSION slaveDeviceExtension;
    PDEVICE_OBJECT  slaveDeviceObject = NULL;    // ptr to device object
    PVOID           ntUnicodeStringBuffer;    // NT Device Name "\Device\Reparse1"
    UNICODE_STRING  ntUnicodeString;

    deviceExtension = (PLOCAL_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (DeviceObject != deviceExtension->MasterDeviceObject)

        {

        // 
        // A new device object exists.
        // Cleanup from the first pass and return STATUS_SUCCESS.
        // 

        ReparseDump (
            REPARSEDIAG2,
            ( "ReparseCreate (Path 2): DeviceObject = 0x%lx\n", DeviceObject ) );

        ReparseDump (
            REPARSEDIAG2,
            ( "ReparseCreate (Path 2): Freeing Pool: 0x%lx\n", deviceExtension->Pool ) );

        ExFreePool ( deviceExtension->Pool );

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        ReparseDump (
            REPARSEDIAG2,
            ( "ReparseCreate (Path 2): Complete Irp (0x%lx) - STATUS_SUCCESS\n",
            Irp ) );

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_SUCCESS;

        }
    else
        {

        // 
        // Create a new device object, \Device\Reparse1.
        // 

        ReparseDump (
            REPARSEDIAG2,
            ( "ReparseCreate (Path 1): DeviceObject = 0x%lx\n", DeviceObject ) );

        RtlInitUnicodeString( &ntUnicodeString, NT_NEW_DEVICE_NAME );
    
        ntUnicodeStringBuffer = ExAllocatePoolWithTag (
            NonPagedPool,
            ntUnicodeString.MaximumLength,
            'srpR' );

        if ( ntUnicodeStringBuffer == NULL)
        {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        ReparseDump (
            REPARSEDIAG2,
            ( "ReparseCreate (Path 1): ntUnicodeStringBuffer pool = 0x%lx\n",
            ntUnicodeStringBuffer ) );

        ntStatus = IoCreateDevice(
            DeviceObject->DriverObject,     // Our Driver Object
            sizeof (PLOCAL_DEVICE_EXTENSION),
            &ntUnicodeString,               // Device name "\Device\????"
            FILE_DEVICE_UNKNOWN,       		// Device type
            0,                              // Device characteristics
            TRUE,                           // An exclusive device
            &slaveDeviceObject );                // Returned ptr to Device Object

        if ( !NT_SUCCESS( ntStatus ) )
        {
            ReparseDump(
                REPARSEERRORS, ("ReparseCreate (Path 1): Couldn't create the new device object\n"));

    	    Irp->IoStatus.Status = ntStatus;
    	    return ntStatus;

        }

        //
        // Tweak the Master Device Object to have knowledge of this new
        // Device Object
        //

        deviceExtension->SlaveDeviceObject = slaveDeviceObject;
        
        //
        // Configure the device extension for this slave device
        //

        slaveDeviceExtension = (PLOCAL_DEVICE_EXTENSION) slaveDeviceObject->DeviceExtension;

        RtlZeroMemory ( slaveDeviceExtension, sizeof (PLOCAL_DEVICE_EXTENSION));

        slaveDeviceExtension->MasterDeviceObject = deviceExtension->MasterDeviceObject;

        slaveDeviceExtension->SlaveDeviceObject = slaveDeviceObject;

        slaveDeviceExtension->Pool = ntUnicodeStringBuffer;
    
        //
        // Clear this bit to allow CreateFile to open this device.
        // This is normally done by the I/O Manager in DriverEntry, but
        // must be done manually in dispatch routines.
        //

        slaveDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // Copy the Unicode string into ntUnicodeStringBuffer
        //

        RtlCopyMemory ( ntUnicodeStringBuffer,
            ntUnicodeString.Buffer,
            ntUnicodeString.MaximumLength );
    
        Irp->IoStatus.Status = STATUS_REPARSE;
        Irp->IoStatus.Information = 0;

        Irp->Tail.Overlay.OriginalFileObject->FileName.Length = ntUnicodeString.Length;
        Irp->Tail.Overlay.OriginalFileObject->FileName.MaximumLength = ntUnicodeString.MaximumLength;
        Irp->Tail.Overlay.OriginalFileObject->FileName.Buffer = ntUnicodeStringBuffer;
    
        ReparseDump (
            REPARSEDIAG2,
            ( "ReparseCreate (Path 1): Complete Irp (0x%lx) - STATUS_REPARSE\n",
            Irp ) );

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
        return STATUS_REPARSE;
    }
}

NTSTATUS
ReparseClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when RP is closed.

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
    
    ReparseDump (
        REPARSEDIAG2,
        ( "ReparseClose: Complete Irp (0x%lx) - STATUS_SUCCESSn",
        Irp ) );

    IoCompleteRequest ( Irp, IO_NO_INCREMENT );
    
    return STATUS_SUCCESS;

}

VOID
ReparseUnloadDriver(
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
    PLOCAL_DEVICE_EXTENSION deviceExtension;
    UNICODE_STRING uniWin32NameString;

    deviceExtension = deviceObject->DeviceExtension;

    if ( deviceExtension->MasterDeviceObject != NULL )
    {
        IoDeleteDevice( deviceExtension->MasterDeviceObject );
    }

    if ( deviceExtension->SlaveDeviceObject != NULL )
    {
        IoDeleteDevice( deviceExtension->SlaveDeviceObject );
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
