
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    INCOMPLETE2.C

Abstract:

    Device driver to control the toaster device. This version contains in 
    addition to the features present in INCOMPLETE1.C, create, close, read
    & write handlers, IRP Queuing support and IRP counting & synchronization
    scheme (RemoveLocks) to protect the device from getting deleted while in use.

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
#define _DRIVER_NAME_ "Incomplete2"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, ToasterAddDevice)
#pragma alloc_text (PAGE, ToasterCreate)
#pragma alloc_text (PAGE, ToasterClose)
#pragma alloc_text (PAGE, ToasterDispatchIoctl)
#pragma alloc_text (PAGE, ToasterDispatchIO)
#pragma alloc_text (PAGE, ToasterReadWrite)
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
    NTSTATUS            status = STATUS_SUCCESS;

    ToasterDebugPrint(TRACE, "Entered DriverEntry of "_DRIVER_NAME_"  version "
                                                         "built on " __DATE__" at "__TIME__ "\n");

    DriverObject->MajorFunction[IRP_MJ_PNP]            = ToasterDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = ToasterDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ToasterSystemControl;
    DriverObject->DriverExtension->AddDevice           = ToasterAddDevice;
    DriverObject->DriverUnload                         = ToasterUnload;

    //
    // These dispatch points are required if you want an application or
    // another driver to talk to your driver.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = ToasterCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = ToasterClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ToasterDispatchIO;
    DriverObject->MajorFunction[IRP_MJ_READ]           = ToasterDispatchIO;
    DriverObject->MajorFunction[IRP_MJ_WRITE]           = ToasterDispatchIO;

    return status;
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
    POWER_STATE             powerState;

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

    //
    // We will hold all requests until we are started.
    // On W2K we will not get any I/O until the entire device
    // is started. On Win9x this may be required.
    //

    fdoData->QueueState = HoldRequests;

    fdoData->Self = deviceObject;
    fdoData->NextLowerDriver = NULL;

    InitializeListHead(&fdoData->NewRequestsQueue);
    KeInitializeSpinLock(&fdoData->QueueLock);

    //
    // Initialize the remove event to Not-Signaled
    //

    KeInitializeEvent(&fdoData->RemoveEvent,
                      SynchronizationEvent,
                      FALSE);
    //
    // Initialize the stop event to Signaled:
    // there are no Irps that prevent the device from being
    // stopped. This event will be set when the OutstandingIO
    // will become 1.
    //

    KeInitializeEvent(&fdoData->StopEvent,
                      SynchronizationEvent,
                      TRUE);



    fdoData->OutstandingIO = 1; // biased to 1.  Transition to zero during
                                // remove device means IO is finished.
                                // Transition to 1 means the device can be
                                // stopped.

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
    PDEVICE_CAPABILITIES    deviceCapabilities;
    ULONG                   requestCount;
    PPNP_DEVICE_STATE       deviceState;

    PAGED_CODE ();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    stack = IoGetCurrentIrpStackLocation (Irp);

    ToasterDebugPrint(TRACE, "FDO %s \n",
                PnPMinorFunctionString(stack->MinorFunction));
    ToasterIoIncrement (fdoData);

    if (Deleted == fdoData->DevicePnPState) {

        //
        // Since the device is removed, we will not hold any IRPs.
        // We just fail it.
        //
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE ;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        ToasterIoDecrement(fdoData);
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
            status = ToasterStartDevice (fdoData, Irp);
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completion routine with MORE_PROCESSING_REQUIRED.
        //
        break;


    case IRP_MN_QUERY_STOP_DEVICE:

        //
        // First, don't allow any requests to be passed down.
        //

        SET_NEW_PNP_STATE(fdoData, StopPending);
        fdoData->QueueState = HoldRequests;

        ToasterDebugPrint(INFO, "Holding requests...\n");

        //
        // Then, wait for the existing ones to be finished
        // (since we expect to give up our resources, we
        // can't allow such requests). First, we need to decrement
        // this very operation (and to keep in mind not to decrement
        // after the wait).
        //

        ToasterIoDecrement(fdoData);

        KeWaitForSingleObject(
           &fdoData->StopEvent,
           Executive, // Waiting for reason of a driver
           KernelMode, // Waiting in kernel mode
           FALSE, // No alert
           NULL); // No timeout


        //
        // We must set Irp->IoStatus.Status to STATUS_SUCCESS before
        // passing it down.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (fdoData->NextLowerDriver, Irp);

        return status;

        break;

   case IRP_MN_CANCEL_STOP_DEVICE:

        status = ToasterSendIrpSynchronously(fdoData->NextLowerDriver,Irp);
        if(NT_SUCCESS(status))
        {

            fdoData->QueueState = AllowRequests;

            RESTORE_PREVIOUS_PNP_STATE(fdoData);

            ASSERT(fdoData->DevicePnPState == Started);
            //
            // Process the queued requests
            //

            ToasterProcessQueuedRequests(fdoData);
        } else {
            //
            // Somebody below us failed cancel-stop.
            // This is a fatal error.
            //
            ASSERTMSG("Cancel stop failed. Fatal error!", FALSE);
            ToasterDebugPrint(WARNING, "Failure statute = 0x%x\n", status);
        }
        break;


    case IRP_MN_STOP_DEVICE:

        //
        // After the stop IRP has been sent to the lower driver object, the
        // driver must not send any more IRPs down that touch the device until
        // another START has occurred.  For this reason we are holding IRPs
        // and waiting for the outstanding requests to be finished when
        // QUERY_STOP is received.
        // IRP_MN_STOP_DEVICE doesn't change anything in this behavior
        // (we continue to hold IRPs until a IRP_MN_START_DEVICE is issued).
        //


        //
        // Mark the device as stopped.
        //

        SET_NEW_PNP_STATE(fdoData, Stopped);


        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (fdoData->NextLowerDriver, Irp);

        ToasterIoDecrement(fdoData);

        return status;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        //
        // If we can allow removal of the device, we should set the QueueState
        // to HoldRequests so further requests will be queued. This is required
        // so that we can process queued up requests in cancel-remove just in
        // case somebody else in the stack fails the query-remove.
        //


        fdoData->QueueState = HoldRequests;

        SET_NEW_PNP_STATE(fdoData, RemovePending);

        ToasterDebugPrint(INFO, "Query - remove holding requests...\n");

        ToasterIoDecrement(fdoData);


        //
        // Wait for all the requests to be completed
        //

        KeWaitForSingleObject(
            &fdoData->StopEvent,
            Executive, // Waiting for reason of a driver
            KernelMode, // Waiting in kernel mode
            FALSE, // No alert
            NULL); // No timeout


        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);

        status = IoCallDriver (fdoData->NextLowerDriver, Irp);
        return status;

        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        //
        // We need to reset the QueueState flag to ProcessRequest,
        // since the device resume its normal activities.
        //

        status = ToasterSendIrpSynchronously(fdoData->NextLowerDriver,Irp);

        if(NT_SUCCESS(status))
        {
            fdoData->QueueState = AllowRequests;

            RESTORE_PREVIOUS_PNP_STATE(fdoData);

            //
            // Process the queued requests that arrived between
            // QUERY_REMOVE and CANCEL_REMOVE.
            //

            ToasterProcessQueuedRequests(fdoData);


        } else {
            //
            // Nobody can fail this IRP. This is a fatal error.
            //
            ASSERTMSG("Cancel remove failed. Fatal error!", FALSE);
            ToasterDebugPrint(WARNING, "Failure status = 0x%x\n", status);
        }

        break;

   case IRP_MN_SURPRISE_REMOVAL:

        //
        // The device has been unexpectedly removed from the machine
        // and is no longer available for I/O ("surprise" removal).
        // We must return device and memory resources,
        // disable interfaces. We will defer failing any outstanding
        // request to IRP_MN_REMOVE.
        //

        fdoData->QueueState = FailRequests;

        //
        // Fail all the pending request. Since the QueueState is FailRequests
        // ToasterProcessQueuedRequests will simply flush the queue,
        // completing each IRP with STATUS_NO_SUCH_DEVICE 
        //

        ToasterProcessQueuedRequests(fdoData);

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
        ToasterIoDecrement(fdoData);

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
            // This means we are here after query-remove.
            // So first stop the device, disable the interface,
            // return resources, and fail all the pending request,.
            //

            fdoData->QueueState = FailRequests;

            //
            // Fail all the pending request. Since the QueueState is FailRequests
            // ToasterProcessQueuedRequests will simply flush the queue,
            // completing each IRP with STATUS_NO_SUCH_DEVICE 
            //

            ToasterProcessQueuedRequests(fdoData);

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
        // We need two decrements here, one for the increment in
        // ToasterPnpDispatch, the other for the 1-biased value of
        // OutstandingIO. Also, we need to wait that all the requests
        // are served.
        //

        requestCount = ToasterIoDecrement (fdoData);

        //
        // The requestCount is a least one here (is 1-biased)
        //
        ASSERT(requestCount > 0);

        requestCount = ToasterIoDecrement (fdoData);

        KeWaitForSingleObject (
                &fdoData->RemoveEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL);


        //
        // Send on the remove IRP.
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

    default:
        //
        // Pass down all the unhandled Irps.
        //

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (fdoData->NextLowerDriver, Irp);
        ToasterIoDecrement(fdoData);

        return status;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    ToasterIoDecrement(fdoData);

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
ToasterReadWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Performs read/write to the toaster device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

    NT status code


--*/

{
    PFDO_DATA   fdoData;
    NTSTATUS    status;

    PAGED_CODE();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterDebugPrint(TRACE, "ReadWrite called\n");

    //
    // Perform read/write operation here
    //
    status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0; // fill in the correct length
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;

}

NTSTATUS
ToasterCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

   Dispatch routine to handle Create commands for the toaster device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PFDO_DATA    fdoData;

    PAGED_CODE ();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterDebugPrint(TRACE, "Create \n");

    //
    // Since we don't access the hardware to process create, we don't have to
    // worry about about the current device state and whether or not to queue
    // this request.
    //

    ToasterIoIncrement (fdoData);

    if (Deleted == fdoData->DevicePnPState) {

        //
        // Since the device is removed, we will not hold any IRPs.
        // We just fail it.
        //
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE ;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        ToasterIoDecrement(fdoData);
        return STATUS_NO_SUCH_DEVICE ;
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    ToasterIoDecrement(fdoData);


    return STATUS_SUCCESS;
}


NTSTATUS
ToasterClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

   Dispatch routine to handle Close for the toaster device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PFDO_DATA    fdoData;
    NTSTATUS     status;
    TOASTER_INTERFACE_STANDARD busInterface;

    PAGED_CODE ();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterDebugPrint(TRACE, "Close \n");

    ToasterIoIncrement (fdoData);

    status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    ToasterIoDecrement(fdoData);


    return status;
}


NTSTATUS
ToasterDispatchIoctl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

    Handle user mode DeviceIoControl requests.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status= STATUS_SUCCESS;
    PFDO_DATA               fdoData;

    PAGED_CODE ();

    ToasterDebugPrint(TRACE, "Ioctl called\n");

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
ToasterDispatchIO(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

    This routine is called by: 
    1) The I/O manager to dispath read, write & IOCTL requests. 
    2) This is called by ToasterProcessQueuedRequest routine to
        redispatch all queued IRPs. This is done whenever the QueueState
        changes for HoldRequests to AllowRequests.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status= STATUS_SUCCESS;
    PFDO_DATA               fdoData;

    PAGED_CODE ();
    
    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterIoIncrement (fdoData);
    
    if (HoldRequests == fdoData->QueueState) {

        return ToasterQueueRequest(fdoData, Irp);

    } else if (Deleted == fdoData->DevicePnPState) {

        //
        // Since the device is removed, we will not hold any IRPs.
        // We just fail it.
        //
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE ;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        ToasterIoDecrement(fdoData);
        return STATUS_NO_SUCH_DEVICE ;
    }


    irpStack = IoGetCurrentIrpStackLocation (Irp);
    
    switch (irpStack->MajorFunction) {
        case IRP_MJ_READ:
        case IRP_MJ_WRITE:
            status =  ToasterReadWrite(DeviceObject, Irp);    
            break;
        case IRP_MJ_DEVICE_CONTROL:
            status =  ToasterDispatchIoctl(DeviceObject, Irp);
            break;
        default:
            ASSERTMSG(FALSE, "ToasterDispatchIO invalid IRP");
            status = STATUS_UNSUCCESSFUL;
            Irp->IoStatus.Status = status;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);     
            break;
    }               

    ToasterIoDecrement(fdoData);
    return status;
}


NTSTATUS
ToasterStartDevice (
    IN PFDO_DATA     FdoData,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Performs whatever initialization is needed to setup the
    device, namely connecting to an interrupt,
    setting up a DMA channel or mapping any I/O port resources.

Arguments:

   Irp - pointer to an I/O Request Packet.

   FdoData - pointer to a FDO_DATA structure

Return Value:

    NT status code


--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PIO_STACK_LOCATION      stack;

    stack = IoGetCurrentIrpStackLocation (Irp);

    PAGED_CODE();


    //
    // Enable the device interface. Return status
    // STATUS_OBJECT_NAME_EXISTS means we are enabling the interface
    // that was already enabled, which could happen if the device
    // is stopped and restarted for resource rebalancing.
    //
    status = IoSetDeviceInterfaceState(&FdoData->InterfaceName, TRUE);

    if (!NT_SUCCESS (status)){
        ToasterDebugPrint(ERROR, "IoSetDeviceInterfaceState failed: 0x%x\n",
                                status);
        return status;
    }

    SET_NEW_PNP_STATE(FdoData, Started);

    //
    // Mark the device as active and not holding IRPs
    //

    FdoData->QueueState = AllowRequests;

    //
    // The last thing to do is to process the pending IRPs.
    //

    ToasterProcessQueuedRequests(FdoData);

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
ToasterQueueRequest    (
    IN OUT PFDO_DATA FdoData,
    IN PIRP Irp
    )

/*++

Routine Description:

    Queues the Irp in the device queue. This routine will be called whenever
    the device receives IRP_MN_QUERY_STOP_DEVICE or IRP_MN_QUERY_REMOVE_DEVICE

Arguments:

    FdoData - pointer to the device's extension.

    Irp - the request to be queued.

Return Value:

    NT status code.

--*/
{

    KIRQL               oldIrql;
    NTSTATUS status = STATUS_PENDING;

    ToasterDebugPrint(TRACE, "Queuing Requests\n");

    //
    // Check whether we are allowed to queue requests.
    //

    ASSERT(HoldRequests == FdoData->QueueState);

    IoMarkIrpPending(Irp);
    
    KeAcquireSpinLock(&FdoData->QueueLock, &oldIrql);

    InsertTailList(&FdoData->NewRequestsQueue,
                        &Irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock(&FdoData->QueueLock, oldIrql);

    ToasterIoDecrement(FdoData);
    return status;
}



VOID
ToasterProcessQueuedRequests    (
    IN OUT PFDO_DATA FdoData
    )

/*++

Routine Description:

    Removes and processes the entries in the queue. If this routine is called
    when processing IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE
    or IRP_MN_START_DEVICE, the requests are passed to the next lower driver.
    If the routine is called when IRP_MN_REMOVE_DEVICE is received, the IRPs
    are completed with STATUS_NO_SUCH_DEVICE .


Arguments:

    FdoData - pointer to the device's extension (where is the held IRPs queue).


Return Value:

    VOID.

--*/
{

    KIRQL               oldIrql;
    PIRP                nextIrp;
    PLIST_ENTRY         listEntry;

    ToasterDebugPrint(TRACE, "Process or fail queued Requests\n");


    //
    // We need to dequeue all the entries in the queue, reset the cancel
    // routine for each of them and then process then:
    // - if the device is active, we will send them down
    // - else we will complete them with STATUS_NO_SUCH_DEVICE 
    //

    for(;;)
    {
        //
        // Acquire the queue lock before manipulating the list entries.
        //
        KeAcquireSpinLock(&FdoData->QueueLock, &oldIrql);

        if(IsListEmpty(&FdoData->NewRequestsQueue))
        {
            KeReleaseSpinLock(&FdoData->QueueLock, oldIrql);
            break;
        }

        //
        // Remove a request from the queue.
        //
        listEntry = RemoveHeadList(&FdoData->NewRequestsQueue);

        nextIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        //
        // Release the lock before we call out of the driver
        //

        KeReleaseSpinLock(&FdoData->QueueLock, oldIrql);

        if (FailRequests == FdoData->QueueState) {
            //
            // The device was removed, we need to fail the request
            //
            nextIrp->IoStatus.Information = 0;
            nextIrp->IoStatus.Status = STATUS_NO_SUCH_DEVICE ;
            IoCompleteRequest (nextIrp, IO_NO_INCREMENT);

        } else {
                //
                // Re-dispatch the IRP. 
                //
                if(STATUS_PENDING == ToasterDispatchIO(FdoData->Self, 
                                            nextIrp) ) {                    
                    // It looks like either the device is too busy to perform
                    // IO or the QueueStatus has changed to HoldRequest and
                    // the currentIrp has been queued back. So break out of
                    // the loop.
                    // Note that the assumption made above will not be true
                    // if this driver happens to send any I/Os to lower driver
                    // and IoCallDriver returns STATUS_PENDING.
                    // In such a scenario you probably have to rewrite the 
                    // dispatch routines to convey why status_pending was 
                    // returned. Based on that you should decide whether to 
                    // continue the loop or break out.
                    //
                    break;
                }
        }

    }

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
    PIO_STACK_LOCATION  stack;
    PFDO_DATA           fdoData;
    NTSTATUS            status;

    stack   = IoGetCurrentIrpStackLocation(Irp);
    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterDebugPrint(TRACE, "Entered ToasterDispatchPower\n");

    ToasterIoIncrement (fdoData);

    //
    // We don't queue power Irps, we'll only check if the
    // device was removed, otherwise we'll take appropriate
    // action and send it to the next lower driver. In general
    // drivers should not cause long delays while handling power
    // IRPs. If a driver cannot handle a power IRP in a brief time,
    // it should return STATUS_PENDING and queue all incoming
    // IRPs until the IRP completes.
    //

    if (Deleted == fdoData->DevicePnPState) {

        //
        // Even if a driver fails the IRP, it must nevertheless call
        // PoStartNextPowerIrp to inform the Power Manager that it
        // is ready to handle another power IRP.
        //

        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Status = status = STATUS_NO_SUCH_DEVICE ;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        ToasterIoDecrement (fdoData);
        return status;
    }

    PoStartNextPowerIrp(Irp);

    IoSkipCurrentIrpStackLocation(Irp);

    //
    // Drivers must use PoCallDriver, rather than IoCallDriver,
    // to pass power IRPs. PoCallDriver allows the Power Manager
    // to ensure that power IRPs are properly synchronized throughout
    // the system.
    //

    status = PoCallDriver(fdoData->NextLowerDriver, Irp);
    
    ToasterIoDecrement (fdoData);
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
    PIO_STACK_LOCATION      stack;

    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation (Irp);

    ToasterDebugPrint(TRACE, "FDO received WMI IRP\n");

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterIoIncrement (fdoData);

    if (fdoData->DevicePnPState == Deleted) {
        Irp->IoStatus.Status = status = STATUS_NO_SUCH_DEVICE ;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        ToasterIoDecrement (fdoData);
        return status;
    }

    //
    // We aren't interested in handling WMI irps, so send it down.
    // 
    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (fdoData->NextLowerDriver, Irp);
    ToasterIoDecrement (fdoData);
    return status;
}

LONG
ToasterIoIncrement    (
    IN  OUT PFDO_DATA   FdoData
    )

/*++

Routine Description:

    This routine increments the number of requests the device receives


Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    The value of OutstandingIO field in the device extension.


--*/

{

    LONG            result;


    result = InterlockedIncrement(&FdoData->OutstandingIO);

    //ToasterDebugPrint(TRACE, "ToasterIoIncrement %d\n", result);

    ASSERT(result > 0);
    //
    // Need to clear StopEvent (when OutstandingIO bumps from 1 to 2)
    //
    if (result == 2) {
        //
        // We need to clear the event
        //
        KeClearEvent(&FdoData->StopEvent);
    }

    return result;
}

LONG
ToasterIoDecrement    (
    IN  OUT PFDO_DATA  FdoData
    )

/*++

Routine Description:

    This routine decrements as it complete the request it receives

Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    The value of OutstandingIO field in the device extension.


--*/
{

    LONG            result;

    result = InterlockedDecrement(&FdoData->OutstandingIO);

    //ToasterDebugPrint(TRACE, "ToasterIoDecrement %d\n", result);

    ASSERT(result >= 0);

    if (result == 1) {
        //
        // Set the stop event. Note that when this happens
        // (i.e. a transition from 2 to 1), the type of requests we
        // want to be processed are already held instead of being
        // passed away, so that we can't "miss" a request that
        // will appear between the decrement and the moment when
        // the value is actually used.
        //

        KeSetEvent (&FdoData->StopEvent,
                    IO_NO_INCREMENT,
                    FALSE);

    }

    if (result == 0) {

        //
        // The count is 1-biased, so it can be zero only if an
        // extra decrement is done when a remove Irp is received
        //

        ASSERT(Deleted == FdoData->DevicePnPState);

        //
        // Set the remove event, so the device object can be deleted
        //

        KeSetEvent (&FdoData->RemoveEvent,
                    IO_NO_INCREMENT,
                    FALSE);

    }


    return result;
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
            
            KdPrint (("Incomplete2.sys: RtlStringCbVPrintfA failed %x \n", status));
            return;
        }
        if (DebugPrintLevel <= DebugLevel) {

            KdPrint (("Incomplete2.sys: %s", debugMessageBuffer));
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



