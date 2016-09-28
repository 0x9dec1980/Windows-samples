
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    INCOMPLETE1.C

Abstract:

    Device driver to control the toaster device. This is a simplified version of
    the toaster function driver. This incomplete version is provided to help
    you understand the basic functionalities of a driver. It just has minimal things
    required to build and get installed in the system. One should just use this
    code for learning purposes only. After you understand the basics, you can
    study the featured version of the driver and base your driver code off of 
    that.


Environment:

    Kernel mode


Revision History:

    Eliyas Yakub - 10-Oct-2002

--*/
#include "toaster.h"

//
// Global debug error level
//

ULONG DebugLevel = INFO;
#define _DRIVER_NAME_ "Incomplete1"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, ToasterAddDevice)
#pragma alloc_text (PAGE, ToasterDispatchPnp)
#pragma alloc_text (PAGE, ToasterStartDevice)
#pragma alloc_text (PAGE, ToasterUnload)
#pragma alloc_text(PAGE,ToasterSystemControl)
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    ToasterDebugPrint(TRACE, "Entered DriverEntry of "_DRIVER_NAME_" version "
                                                         "built on " __DATE__" at "__TIME__ "\n");

    DriverObject->MajorFunction[IRP_MJ_PNP]            = ToasterDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = ToasterDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ToasterSystemControl;
    DriverObject->DriverExtension->AddDevice           = ToasterAddDevice;
    DriverObject->DriverUnload                         = ToasterUnload;

    return STATUS_SUCCESS;
}


NTSTATUS
ToasterAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    The Plug & Play subsystem is handing us a brand new PDO, for which we
    (by means of INF registration) have been asked to provide a driver.

    We need to determine if we need to be in the driver stack for the device.
    Create a function device object to attach to the stack
    Initialize that device object
    Return status success.

    Remember: We can NOT actually send ANY non-pnp IRPS to the given driver
    stack, UNTIL we have received an IRP_MN_START_DEVICE.

Arguments:

    DeviceObject - pointer to a device object.

    PhysicalDeviceObject -  pointer to a device object created by the
                            underlying bus driver.

Return Value:

    NT status code.

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PFDO_DATA               fdoData;

    PAGED_CODE();

    ToasterDebugPrint(TRACE, "AddDevice PDO (0x%x)\n", PhysicalDeviceObject);

    //
    // Create a function device object.
    //

    status = IoCreateDevice (DriverObject,
                             sizeof (FDO_DATA),
                             NULL,  // No Name
                             FILE_DEVICE_UNKNOWN,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &deviceObject);


    if (!NT_SUCCESS (status)) {
        //
        // returning failure here prevents the entire stack from functioning,
        // but most likely the rest of the stack will not be able to create
        // device objects either, so it is still OK.
        //
        return status;
    }

    ToasterDebugPrint(INFO, "AddDevice FDO (0x%x)\n", deviceObject);

    //
    // Initialize the device extension.
    //

    fdoData = (PFDO_DATA) deviceObject->DeviceExtension;

    //
    // Set the initial state of the FDO
    //

    INITIALIZE_PNP_STATE(fdoData);


    fdoData->UnderlyingPDO = PhysicalDeviceObject;
    fdoData->Self = deviceObject;
    fdoData->NextLowerDriver = NULL;

    deviceObject->Flags |= DO_POWER_PAGABLE;
    deviceObject->Flags |= DO_BUFFERED_IO;


    //
    // Attach our driver to the device stack.
    // The return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //

    fdoData->NextLowerDriver = IoAttachDeviceToDeviceStack (deviceObject,
                                                       PhysicalDeviceObject);
    if(NULL == fdoData->NextLowerDriver) {

        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Tell the Plug & Play system that this device will need an interface
    //

    status = IoRegisterDeviceInterface (
                PhysicalDeviceObject,
                (LPGUID) &GUID_DEVINTERFACE_TOASTER,
                NULL,
                &fdoData->InterfaceName);

    if (!NT_SUCCESS (status)) {
        ToasterDebugPrint(ERROR,
            "AddDevice: IoRegisterDeviceInterface failed (%x)\n", status);
        IoDetachDevice(fdoData->NextLowerDriver);
        IoDeleteDevice (deviceObject);
        return status;
    }

    //
    // Clear the DO_DEVICE_INITIALIZING flag.
    // Note: Do not clear this flag until the driver has set the
    // device power state and the power DO flags.
    //

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return status;

}

NTSTATUS
ToasterDispatchPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The plug and play dispatch routines.

    Most of these requests the driver will completely ignore.
    In all cases it must pass on the IRP to the lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PFDO_DATA               fdoData;
    PIO_STACK_LOCATION      stack;
    NTSTATUS                status = STATUS_SUCCESS;
    PPNP_DEVICE_STATE       deviceState;

    PAGED_CODE ();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    stack = IoGetCurrentIrpStackLocation (Irp);

    ToasterDebugPrint(TRACE, "FDO %s \n",
                PnPMinorFunctionString(stack->MinorFunction));

    if (Deleted == fdoData->DevicePnPState) {

        //
        // Since the device is removed, we will not hold any IRPs.
        // We just fail it.
        //
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE ;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE ;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        // First pass the IRP down.
        //

        status = ToasterSendIrpSynchronously(fdoData->NextLowerDriver, Irp);
        if (NT_SUCCESS (status)) {
            //
            // Lower drivers have finished their start operation, so now
            // we can finish ours.
            //
            status = IoSetDeviceInterfaceState(&fdoData->InterfaceName, TRUE);

            if (NT_SUCCESS (status)){
                SET_NEW_PNP_STATE(fdoData, Started);
            } else {
                ToasterDebugPrint(ERROR, "IoSetDeviceInterfaceState failed: 0x%x\n",
                                        status);
            }
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completion routine with MORE_PROCESSING_REQUIRED.
        //

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;

   case IRP_MN_SURPRISE_REMOVAL:

        SET_NEW_PNP_STATE(fdoData, SurpriseRemovePending);

        //
        // Disable the device interface.
        //

        status = IoSetDeviceInterfaceState(&fdoData->InterfaceName, FALSE);

        if (!NT_SUCCESS (status)) {
            ToasterDebugPrint(ERROR, 
                "IoSetDeviceInterfaceState failed: 0x%x\n", status);
        }

        //
        // We must set Irp->IoStatus.Status to STATUS_SUCCESS before
        // passing it down.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (fdoData->NextLowerDriver, Irp);

        return status;

   case IRP_MN_REMOVE_DEVICE:

        //
        // The Plug & Play system has dictated the removal of this device.  We
        // have no choice but to detach and delete the device object.
        // (If we wanted to express an interest in preventing this removal,
        // we should have failed the query remove IRP).
        //


        if(SurpriseRemovePending != fdoData->DevicePnPState)
        {
            //
            // Disable the Interface
            //
            status = IoSetDeviceInterfaceState(&fdoData->InterfaceName, FALSE);

            if (!NT_SUCCESS (status)) {
                ToasterDebugPrint(ERROR,
                        "IoSetDeviceInterfaceState failed: 0x%x\n", status);
            }

        }

        SET_NEW_PNP_STATE(fdoData, Deleted);

        //
        // We need to send the remove down the stack before we detach,
        // but we don't need to wait for the completion of this operation
        // (and to register a completion routine).
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (fdoData->NextLowerDriver, Irp);

        //
        // Detach the FDO from the device stack
        //
        IoDetachDevice (fdoData->NextLowerDriver);

        //
        // Free up interface memory
        //

        RtlFreeUnicodeString(&fdoData->InterfaceName);
        IoDeleteDevice (fdoData->Self);

        return status;

    case IRP_MN_QUERY_STOP_DEVICE:


        SET_NEW_PNP_STATE(fdoData, StopPending);

        //
        // We must set Irp->IoStatus.Status to STATUS_SUCCESS before
        // passing it down.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

   case IRP_MN_CANCEL_STOP_DEVICE:

        RESTORE_PREVIOUS_PNP_STATE(fdoData);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:
       
        SET_NEW_PNP_STATE(fdoData, Stopped);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        SET_NEW_PNP_STATE(fdoData, RemovePending);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        RESTORE_PREVIOUS_PNP_STATE(fdoData);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    default:
        //
        // Pass down all the unhandled Irps.
        //
        break;
    }

    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (fdoData->NextLowerDriver, Irp);
    return status;
    
}


VOID
ToasterUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE ();

    //
    // The device object(s) should be NULL now
    // (since we unload, all the devices objects associated with this
    // driver must be deleted.
    //
    ASSERT(DriverObject->DeviceObject == NULL);

    //
    // We should not be unloaded until all the devices we control
    // have been removed from our queue.
    //
    ToasterDebugPrint(TRACE, "unload\n");

    return;
}



NTSTATUS
ToasterSendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Sends the Irp down the stack and waits for it to complete.

Arguments:
    DeviceObject - pointer to the device object.

    Irp - pointer to the current IRP.

    NotImplementedIsValid -

Return Value:

    NT status code

--*/
{
    KEVENT   event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           ToasterDispatchPnpComplete,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    status = IoCallDriver(DeviceObject, Irp);

    //
    // Wait for lower drivers to be done with the Irp.
    // Important thing to note here is when you allocate
    // memory for an event in the stack you must do a
    // KernelMode wait instead of UserMode to prevent
    // the stack from getting paged out.
    //
    //

    if (status == STATUS_PENDING) {
       KeWaitForSingleObject(&event,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL
                             );
       status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
ToasterDispatchPnpComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:
    The lower-level drivers completed the pnp IRP.
    Signal this to whoever registered us.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Context - pointer to the event to be signaled.
Return Value:

    NT status code


--*/
{
    PKEVENT             event = (PKEVENT)Context;

    UNREFERENCED_PARAMETER (DeviceObject);

    //
    // If the lower driver didn't return STATUS_PENDING, we don't need to 
    // set the event because we won't be waiting on it. 
    // This optimization avoids grabbing the dispatcher lock and improves perf.
    //
    if (Irp->PendingReturned == TRUE) {
        KeSetEvent (event, IO_NO_INCREMENT, FALSE);
    }

    //
    // The dispatch routine will have to call IoCompleteRequest
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
ToasterDispatchPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The power dispatch routine.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PFDO_DATA           fdoData;
    NTSTATUS            status;

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterDebugPrint(TRACE, "Entered ToasterDispatchPower\n");

    PoStartNextPowerIrp(Irp);

    IoSkipCurrentIrpStackLocation(Irp);

    //
    // Drivers must use PoCallDriver, rather than IoCallDriver,
    // to pass power IRPs. PoCallDriver allows the Power Manager
    // to ensure that power IRPs are properly synchronized throughout
    // the system.
    //

    status = PoCallDriver(fdoData->NextLowerDriver, Irp);
    
    return status;
}


NTSTATUS
ToasterSystemControl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description

    We have just received a System Control IRP.

    Assume that this is a WMI IRP and
    call into the WMI system library and let it handle this IRP for us.

--*/
{
    PFDO_DATA               fdoData;
    NTSTATUS                status;

    PAGED_CODE();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterDebugPrint(TRACE, "FDO received WMI IRP\n");
    //
    // We aren't interested in handling WMI irps, so send it down.
    // 
    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (fdoData->NextLowerDriver, Irp);
    return status;
}

VOID
ToasterDebugPrint    (
    IN ULONG   DebugPrintLevel,
    IN PCCHAR  DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for the sample driver.

Arguments:

    DebugPrintLevel - print level between 0 and 3, with 3 the most verbose

Return Value:

    None.

 --*/
 {
#define     TEMP_BUFFER_SIZE        1024
    va_list    list;
    UCHAR      debugMessageBuffer[TEMP_BUFFER_SIZE];
    NTSTATUS status;
    
    va_start(list, DebugMessage);
    
    if (DebugMessage) {
        //
        // Using new safe string functions instead of _vsnprintf. This function takes
        // care of NULL terminating if the message is longer than the buffer.
        //
        status = RtlStringCbVPrintfA(debugMessageBuffer, sizeof(debugMessageBuffer), 
                                    DebugMessage, list);
        if(!NT_SUCCESS(status)) {
            
            KdPrint ((_DRIVER_NAME_": RtlStringCbVPrintfA failed %x \n", status));
            return;
        }
        if (DebugPrintLevel <= DebugLevel) {

            KdPrint ((_DRIVER_NAME_": %s", debugMessageBuffer));
        }        
        
    }
    va_end(list);

    return;

}

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
)
{
    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";
        default:
            return "unknown_pnp_irp";
    }
}



