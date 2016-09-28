// Simple driver that demonstrates dynamically loading and unloading

#include "ntddk.h"

#define NT_DEVICE_NAME      L"\\Device\\Ldunld"
#define DOS_DEVICE_NAME     L"\\DosDevices\\LOADTEST"

NTSTATUS
LdUnldOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
LdUnldClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
LdUnldUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{

    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status;
    UNICODE_STRING uniNtNameString;
    UNICODE_STRING uniWin32NameString;

    KdPrint( ("LDUNLD: Entered the Load/Unload driver!\n") );

    //
    // Create counted string version of our device name.
    //

    RtlInitUnicodeString( &uniNtNameString, NT_DEVICE_NAME );

    //
    // Create the device object
    //

    status = IoCreateDevice(
                 DriverObject,
                 0,                     // We don't use a device extension
                 &uniNtNameString,
                 FILE_DEVICE_UNKNOWN,
                 0,                     // No standard device characteristics
                 FALSE,                 // This isn't an exclusive device
                 &deviceObject
                 );

    if ( NT_SUCCESS(status) )
    {

        //
        // Create dispatch points for create/open, close, unload.
        //

        DriverObject->MajorFunction[IRP_MJ_CREATE] = LdUnldOpen;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = LdUnldClose;
        DriverObject->DriverUnload = LdUnldUnload;

        KdPrint( ("LDUNLD: just about ready!\n") );

        //
        // Create counted string version of our Win32 device name.
        //
    
        RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );
    
        //
        // Create a link from our device name to a name in the Win32 namespace.
        //
        
        status = IoCreateSymbolicLink( &uniWin32NameString, &uniNtNameString );

        if (!NT_SUCCESS(status))
        {
            KdPrint( ("LDUNLD: Couldn't create the symbolic link\n") );

            IoDeleteDevice( DriverObject->DeviceObject );
        }
        else
        {
            KdPrint( ("LDUNLD: All initialized!\n") );
        }
    }
    else
    {
        KdPrint( ("LDUNLD: Couldn't create the device\n") );
    }
    return status;
}

NTSTATUS
LdUnldOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    //
    // No need to do anything.
    //

    //
    // Fill these in before calling IoCompleteRequest.
    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    KdPrint( ("LDUNLD: Opened!!\n") );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

NTSTATUS
LdUnldClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    //
    // No need to do anything.
    //

    //
    // Fill these in before calling IoCompleteRequest.
    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    KdPrint( ("LDUNLD: Closed!!\n") );

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

VOID
LdUnldUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    UNICODE_STRING uniWin32NameString;

    //
    // All *THIS* driver needs to do is to delete the device object and the
    // symbolic link between our device name and the Win32 visible name.
    //
    // Almost every other driver ever witten would need to do a
    // significant amount of work here deallocating stuff.
    //

    KdPrint( ("LDUNLD: Unloading!!\n") );
    
    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );

    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //
    
    IoDeleteSymbolicLink( &uniWin32NameString );

    //
    // Finally delete our device object
    //

    IoDeleteDevice( DriverObject->DeviceObject );
}
