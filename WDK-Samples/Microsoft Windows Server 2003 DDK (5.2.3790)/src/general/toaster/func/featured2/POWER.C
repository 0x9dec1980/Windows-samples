/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    power.c

Abstract:

    The power management related processing.

    The Power Manager uses IRPs to direct drivers to change system
    and device power levels, to respond to system wake-up events,
    and to query drivers about their devices. All power IRPs have
    the major function code IRP_MJ_POWER.

    Most function and filter drivers perform some processing for
    each power IRP, then pass the IRP down to the next lower driver
    without completing it. Eventually the IRP reaches the bus driver,
    which physically changes the power state of the device and completes
    the IRP.

    When the IRP has been completed, the I/O Manager calls any
    IoCompletion routines set by drivers as the IRP traveled
    down the device stack. Whether a driver needs to set a completion
    routine depends upon the type of IRP and the driver's individual
    requirements.

    The power policy of this driver is simple. The device enters the
    device working state D0 when the system enters the system
    working state S0. The device enters the lowest-powered sleeping state
    D3 for all the other system power states (S1-S5).

Environment:

    Kernel mode

Revision History:

     Based on Adrian J. Oney's power management boilerplate code 
     
--*/

#include "toaster.h"
#include "power.h"

#if defined(EVENT_TRACING)
#include "power.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ToasterDispatchPower)
#pragma alloc_text(PAGE, ToasterDispatchPowerDefault)
#pragma alloc_text(PAGE, ToasterDispatchSetPowerState)
#pragma alloc_text(PAGE, ToasterDispatchQueryPowerState)
#pragma alloc_text(PAGE, ToasterDispatchSystemPowerIrp)
#pragma alloc_text(PAGE, ToasterDispatchDeviceQueryPower)
#pragma alloc_text(PAGE, ToasterDispatchDeviceSetPower)
#pragma alloc_text(PAGE, ToasterQueuePassiveLevelPowerCallbackWorker)
#pragma alloc_text(PAGE, ToasterPowerBeginQueuingIrps)
#pragma alloc_text(PAGE, ToasterCallbackHandleDeviceQueryPower)
#pragma alloc_text(PAGE, ToasterCallbackHandleDeviceSetPower)
#pragma alloc_text(PAGE, ToasterCanSuspendDevice)
#endif // ALLOC_PRAGMA

//
// The code for this file is split into two sections, biolerplate code and
// device programming code.
//
// The Biolerplate code comes first. The device programming code comes last in
// the file.
//

//
// Begin Biolerplate Code
// ----------------------
//

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

    ToasterDebugPrint(TRACE, "FDO %s IRP:0x%p %s %s\n",
                  DbgPowerMinorFunctionString(stack->MinorFunction),
                  Irp,
                  DbgSystemPowerString(fdoData->SystemPowerState),
                  DbgDevicePowerString(fdoData->DevicePowerState));

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

    //
    // If the device is not stated yet, just pass it down.
    //

    if (NotStarted == fdoData->DevicePnPState ) {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        status = PoCallDriver(fdoData->NextLowerDriver, Irp);
        ToasterIoDecrement (fdoData);
        return status;
    }


    //
    // Check the request type.
    //

    switch(stack->MinorFunction) {

        case IRP_MN_SET_POWER:

            //
            // The Power Manager sends this IRP for one of the
            // following reasons:
            // 1) To notify drivers of a change to the system power state.
            // 2) To change the power state of a device for which
            //    the Power Manager is performing idle detection.
            // A driver sends IRP_MN_SET_POWER to change the power
            // state of its device if it's a power policy owner for the
            // device.
            //

            status = ToasterDispatchSetPowerState(DeviceObject, Irp);

            break;


        case IRP_MN_QUERY_POWER:

            //
            // The Power Manager sends a power IRP with the minor
            // IRP code IRP_MN_QUERY_POWER to determine whether it
            // can safely change to the specified system power state
            // (S1-S5) and to allow drivers to prepare for such a change.
            // If a driver can put its device in the requested state,
            // it sets status to STATUS_SUCCESS and passes the IRP down.
            //

            status = ToasterDispatchQueryPowerState(DeviceObject, Irp);
            break;

        case IRP_MN_WAIT_WAKE:

            //
            // The minor power IRP code IRP_MN_WAIT_WAKE provides
            // for waking a device or waking the system. Drivers
            // of devices that can wake themselves or the system
            // send IRP_MN_WAIT_WAKE. The system sends IRP_MN_WAIT_WAKE
            // only to devices that always wake the system, such as
            // the power-on switch.
            //
            status = ToasterDispatchWaitWake(DeviceObject, Irp);
            break;

        case IRP_MN_POWER_SEQUENCE:

            //
            // A driver sends this IRP as an optimization to determine
            // whether its device actually entered a specific power state.
            // This IRP is optional. Power Manager cannot send this IRP.
            //

        default:
            //
            // Pass it down
            //
            status = ToasterDispatchPowerDefault(DeviceObject, Irp);
            ToasterIoDecrement(fdoData);
            break;
    }

    return status;
}


NTSTATUS
ToasterDispatchPowerDefault(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    If a driver does not support a particular power IRP,
    it must nevertheless pass the IRP down the device stack
    to the next-lower driver. A driver further down the stack
    might be prepared to handle the IRP and must have the
    opportunity to do so.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    NTSTATUS         status;
    PFDO_DATA        fdoData;

    //
    // Drivers must call PoStartNextPowerIrp while the current
    // IRP stack location points to the current driver.
    // This routine can be called from the DispatchPower routine
    // or from the IoCompletion routine. However, PoStartNextPowerIrp
    // must be called before IoCompleteRequest, IoSkipCurrentIrpStackLocation,
    // and PoCallDriver. Calling the routines in the other order might
    // cause a system deadlock.
    //

    PoStartNextPowerIrp(Irp);

    IoSkipCurrentIrpStackLocation(Irp);

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

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
ToasterDispatchSetPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Processes IRP_MN_SET_POWER.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

    ToasterDebugPrint(TRACE, "Entered ToasterDispatchSetPowerState\n");

    return (stack->Parameters.Power.Type == SystemPowerState) ?
        ToasterDispatchSystemPowerIrp(DeviceObject, Irp) :
        ToasterDispatchDeviceSetPower(DeviceObject, Irp);
}


NTSTATUS
ToasterDispatchQueryPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Processes IRP_MN_QUERY_POWER.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

    ToasterDebugPrint(TRACE, "Entered ToasterDispatchQueryPowerState\n");

    return (stack->Parameters.Power.Type == SystemPowerState) ?
        ToasterDispatchSystemPowerIrp(DeviceObject, Irp) :
        ToasterDispatchDeviceQueryPower(DeviceObject, Irp);
}


NTSTATUS
ToasterDispatchSystemPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Processes IRP_MN_SET_POWER and IRP_MN_QUERY_POWER
   for the system power Irp (S-IRP).

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    POWER_STATE_TYPE    type = stack->Parameters.Power.Type;
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    SYSTEM_POWER_STATE  newSystemState;

    ToasterDebugPrint(TRACE, "Entered ToasterDispatchSystemPowerIrp\n");

    newSystemState = stack->Parameters.Power.State.SystemState;

    //
    // Here we update our cached away system power state.
    //
    if (stack->MinorFunction == IRP_MN_SET_POWER) {
        fdoData->SystemPowerState = newSystemState;
        ToasterDebugPrint(INFO, "\tSetting the system state to %s\n",
                    DbgSystemPowerString(fdoData->SystemPowerState));
    }

    //
    // Send the IRP down
    //
    IoMarkIrpPending(Irp);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(
        Irp,
        (PIO_COMPLETION_ROUTINE) ToasterCompletionSystemPowerUp,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );

    PoCallDriver(fdoData->NextLowerDriver, Irp);

    return STATUS_PENDING;
}


NTSTATUS
ToasterCompletionSystemPowerUp(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
/*++

Routine Description:

   The completion routine for Power Up S-IRP.
   It queues a corresponding D-IRP.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Not used  - context pointer

Return Value:

   NT status code

--*/
{
    PFDO_DATA   fdoData = (PFDO_DATA) Fdo->DeviceExtension;
    NTSTATUS    status = Irp->IoStatus.Status;

    ToasterDebugPrint(TRACE, "Entered ToasterCompletionSystemPowerUp\n");

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        ToasterIoDecrement(fdoData);
        return STATUS_CONTINUE_COMPLETION;
    }

    ToasterQueueCorrespondingDeviceIrp(Irp, Fdo);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



VOID
ToasterQueueCorrespondingDeviceIrp(
    IN PIRP SIrp,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

   This routine gets the D-State for a particular S-State
   from DeviceCaps and generates a D-IRP.

Arguments:

   Irp - pointer to an S-IRP.

   DeviceObject - pointer to a device object.

Return Value:

   NT status code

--*/

{
    NTSTATUS            status;
    POWER_STATE         state;
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(SIrp);

    ToasterDebugPrint(TRACE, "Entered ToasterQueueCorrespondingDeviceIrp\n");

    //
    // Get the device state to request.
    //
    status = ToasterGetPowerPoliciesDeviceState(
        SIrp,
        DeviceObject,
        &state.DeviceState
        );

    if (NT_SUCCESS(status)) {
        //
        // Note: Win2k's PoRequestPowerIrp can take an FDO,
        // but Win9x's requires the PDO.
        //
        ASSERT(!fdoData->PendingSIrp);
        fdoData->PendingSIrp = SIrp;
        
        status = PoRequestPowerIrp(
            fdoData->UnderlyingPDO,
            stack->MinorFunction,
            state,
            ToasterCompletionOnFinalizedDeviceIrp,
            fdoData,
            NULL
            );
    }

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(SIrp);
        SIrp->IoStatus.Status = status;
        IoCompleteRequest(SIrp, IO_NO_INCREMENT);
        ToasterIoDecrement(fdoData);
    }
}


VOID
ToasterCompletionOnFinalizedDeviceIrp(
    IN  PDEVICE_OBJECT              DeviceObject,
    IN  UCHAR                       MinorFunction,
    IN  POWER_STATE                 PowerState,
    IN  PVOID    PowerContext,
    IN  PIO_STATUS_BLOCK            IoStatus
    )
/*++

Routine Description:

   Completion routine for D-IRP.

Arguments:


Return Value:

   NT status code

--*/
{
    //PFDO_DATA   fdoData = (PFDO_DATA) PowerContext->DeviceObject->DeviceExtension;
    PFDO_DATA   fdoData = (PFDO_DATA) PowerContext;
    PIRP        sIrp = fdoData->PendingSIrp;

    ToasterDebugPrint(TRACE, "Entered ToasterCompletionOnFinalizedDeviceIrp\n");

    if(sIrp) {
        //
        // Here we copy the D-IRP status into the S-IRP
        //
        sIrp->IoStatus.Status = IoStatus->Status;

        //
        // Release the IRP
        //
        PoStartNextPowerIrp(sIrp);
        IoCompleteRequest(sIrp, IO_NO_INCREMENT);

        //
        // Cleanup
        //
        //ExFreePool(PowerContext);
        fdoData->PendingSIrp = NULL;
        ToasterIoDecrement(fdoData);
    }
}


NTSTATUS
ToasterDispatchDeviceQueryPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    Handles IRP_MN_QUERY_POWER for D-IRP

Arguments:

    DeviceObject - pointer to a device object.

    Irp - pointer to an I/O Request Packet.

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    DEVICE_POWER_STATE  deviceState = stack->Parameters.Power.State.DeviceState;
    NTSTATUS            status;

    ToasterDebugPrint(TRACE, "Entered ToasterDispatchDeviceQueryPower\n");

    //
    // Here we check to see if it's OK for our hardware to be suspended. Note
    // that our driver may have requests that would cause us to fail this
    // check. If so, we need to begin queuing requests after succeeding this
    // call (otherwise we may succeed such an IRP *after* we've said we can
    // power down).
    //
    if (deviceState == PowerDeviceD0) {

        //
        // Note - this driver does not queue IRPs if the S-to-D state mapping
        //        specifies that the device will remain in D0 during standby.
        //        For some devices, this could be a problem. For instance, if
        //        an audio card were in a machine where S1->D0, it might not
        //        want to stay "active" during standby (that could be noisy).
        //
        //        Ideally, a driver would be able to use the ShutdownType field
        //        in the D-IRP to distinguish these cases. Unfortunately, this
        //        field cannot be trusted for D0 IRPs. A driver can get the same
        //        information however by maintaining a pointer to the current
        //        S-IRP in its device extension. Of course, such a driver must
        //        be very very careful if it also does idle detection (which is
        //        not shown here).
        //
        status = STATUS_SUCCESS;

    } else {

        status = ToasterQueuePassiveLevelPowerCallback(
            DeviceObject,
            Irp,
            IRP_NEEDS_FORWARDING,
            ToasterCallbackHandleDeviceQueryPower
            );

        if (STATUS_PENDING == status) {

            return status;
        }
    }

    return ToasterFinalizeDevicePowerIrp(
        DeviceObject,
        Irp,
        IRP_NEEDS_FORWARDING,
        status
        );
}


NTSTATUS
ToasterDispatchDeviceSetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles IRP_MN_SET_POWER for D-IRP

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    POWER_STATE_TYPE    type = stack->Parameters.Power.Type;
    POWER_STATE         state = stack->Parameters.Power.State;
    NTSTATUS            status;
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  sIrpstack;

    ToasterDebugPrint(TRACE, "Entered ToasterDispatchDeviceSetPower\n");

    //
    // On Win2K and beyond, to facilitate fast-resume from suspended state,
    // if the set-power IRP is to get back to PowerSystemWorking (S0) state, we will
    // complete the S0 before acting on the DeviceIrp. This is because
    // the system waits for all the S0 IRPs sent system wide to complete before resuming the
    // any usermode process activity. Since powering up the device and restoring device
    // context takes longer, it's not necessary to hold the S-IRP and as a result 
    // slow down resume for non-critical devices.
    //
    if(fdoData->PendingSIrp) {
        sIrpstack = IoGetCurrentIrpStackLocation(fdoData->PendingSIrp);
        
        if(sIrpstack->MinorFunction == IRP_MN_SET_POWER &&
            stack->Parameters.Power.State.SystemState == PowerSystemWorking) {
            
            ToasterDebugPrint(TRACE, "\t ****Completing S0 IRP\n");
            fdoData->PendingSIrp->IoStatus.Status = STATUS_SUCCESS;
            PoStartNextPowerIrp(fdoData->PendingSIrp);
            IoCompleteRequest(fdoData->PendingSIrp, IO_NO_INCREMENT);
            fdoData->PendingSIrp = NULL;
            ToasterIoDecrement(fdoData);            
        } 
    } 
    
    if (state.DeviceState < fdoData->DevicePowerState) { // adding power

        IoMarkIrpPending(Irp);

        IoCopyCurrentIrpStackLocationToNext(Irp);

        IoSetCompletionRoutine(
            Irp,
            (PIO_COMPLETION_ROUTINE) ToasterCompletionDevicePowerUp,
            NULL,
            TRUE,
            TRUE,
            TRUE
            );

        PoCallDriver(fdoData->NextLowerDriver, Irp);
        return STATUS_PENDING;

    } else {

        //
        // We are here if we are entering a deeper sleep or entering a state
        // we are already in.
        //
        // As non-D0 IRPs are not alike (some may be for hibernate, shutdown,
        // or sleeping actions), we present these to our state machine.
        //
        // All D0 IRPs are alike though, and we don't want to touch our hardware
        // on a D0->D0 transition. However, we must still present them to our
        // state machine in case we succeeded a Query-D call (which may begin
        // queueing future requests) and the system has sent an S0 IRP to cancel
        // that preceeding query.
        //
        status = ToasterQueuePassiveLevelPowerCallback(
            DeviceObject,
            Irp,
            IRP_NEEDS_FORWARDING,
            ToasterCallbackHandleDeviceSetPower
            );

        if (STATUS_PENDING == status) {

            return status;
        }

        ASSERT(!NT_SUCCESS(status));

        return ToasterFinalizeDevicePowerIrp(
            DeviceObject,
            Irp,
            IRP_NEEDS_FORWARDING,
            status
            );
    }
}


NTSTATUS
ToasterCompletionDevicePowerUp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID NotUsed
    )
/*++

Routine Description:

   The completion routine for Power Up D-IRP.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Not used  - context pointer

Return Value:

   NT status code

--*/
{
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    NTSTATUS            status = Irp->IoStatus.Status;
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    POWER_STATE_TYPE    type = stack->Parameters.Power.Type;

    ToasterDebugPrint(TRACE, "Entered ToasterCompletionDevicePowerUp\n");

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        ToasterIoDecrement(fdoData);
        return STATUS_CONTINUE_COMPLETION;
    }

    ASSERT(stack->MajorFunction == IRP_MJ_POWER);
    ASSERT(stack->MinorFunction == IRP_MN_SET_POWER);

    status = ToasterQueuePassiveLevelPowerCallback(
        DeviceObject,
        Irp,
        IRP_ALREADY_FORWARDED,
        ToasterCallbackHandleDeviceSetPower
        );

    if (STATUS_PENDING != status) {

        ASSERT(!NT_SUCCESS(status));

        ToasterFinalizeDevicePowerIrp(
            DeviceObject,
            Irp,
            IRP_ALREADY_FORWARDED,
            status
            );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}


NTSTATUS
ToasterFinalizeDevicePowerIrp(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction,
    IN  NTSTATUS            Result
    )
/*++

Routine Description:

   This is the final step in D-IRP handling.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an D-IRP.

   Direction -  Whether to forward the D-IRP down or not.
                This depends on whether the system is powering
                up or down.

   Result  -
Return Value:

   NT status code

--*/
{
    NTSTATUS status;
    PFDO_DATA fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterDebugPrint(TRACE, "Entered ToasterFinalizeDevicePowerIrp\n");

    if (Direction == IRP_ALREADY_FORWARDED || (!NT_SUCCESS(Result))) {

        //
        // In either of these cases it is now time to complete the IRP.
        //
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = Result;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        ToasterIoDecrement(fdoData);
        return Result;
    }

    //
    // Here we update our result. Note that ToasterDefaultPowerHandler calls
    // PoStartNextPowerIrp for us.
    //
    Irp->IoStatus.Status = Result;
    status = ToasterDispatchPowerDefault(DeviceObject, Irp);
    ToasterIoDecrement(fdoData);
    return status;
}


NTSTATUS
ToasterQueuePassiveLevelPowerCallback(
    IN  PDEVICE_OBJECT                      DeviceObject,
    IN  PIRP                                Irp,
    IN  IRP_DIRECTION                       Direction,
    IN  PFN_QUEUE_SYNCHRONIZED_CALLBACK     Callback
    )
/*++

Routine Description:

    This routine queues a passive level callback taking the Irp and Direction.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an D-IRP.

   Direction -  Whether to forward the D-IRP down or not.
                This depends on whether the system is powering
                up or down.

   Callback - Function to invoke when at PASSIVE_LEVEL.

Return Value:

   NT status code (STATUS_PENDING on success, in which case the Irp will be
                   marked pending. Error codes otherwise)

--*/
{
    PIO_WORKITEM            item;
    PWORKER_THREAD_CONTEXT  context;
    PFDO_DATA               fdoData;

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    //
    // We must wait for the pending I/Os to finish
    // before powering down the device. But we can't wait
    // while handling a power IRP because it can deadlock
    // the system. So let us queue a worker callback
    // item to do the wait and complete the irp.
    //
    context = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(WORKER_THREAD_CONTEXT),
        TOASTER_POOL_TAG
        );

    if (NULL == context) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Note - Windows 98 doesn't support IoWorkItem's, it only supports the
    //        ExWorkItem version. We use the Io versions here because the
    //        Ex versions can cause system crashes during unload (ie the driver
    //        can unload in the middle of an Ex work item callback)
    //
    item = IoAllocateWorkItem(DeviceObject);

    if (NULL == item) {

        ExFreePool(context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Irp = Irp;
    context->DeviceObject= DeviceObject;
    context->IrpDirection = Direction;
    context->Callback = Callback;
    context->WorkItem = item;

    IoMarkIrpPending(Irp);

    IoQueueWorkItem(
        item,
        ToasterQueuePassiveLevelPowerCallbackWorker,
        DelayedWorkQueue,
        context
        );

    return STATUS_PENDING;
}


VOID
ToasterCallbackHandleDeviceQueryPower(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction
    )
/*++

Routine Description:

   This routine handles the actual query-power state changes for the device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an D-IRP.

   Direction -  Whether to forward the D-IRP down or not.
                This depends on whether the system is powering
                up or down.
Return Value:

   NT status code

--*/
{
    DEVICE_POWER_STATE deviceState;
    PIO_STACK_LOCATION stack;
    NTSTATUS status;

    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation(Irp);

    deviceState = stack->Parameters.Power.State.DeviceState;
    ASSERT(PowerDeviceD0 != deviceState);

    status = ToasterPowerBeginQueuingIrps(
        DeviceObject,
        2,              // one for the S-IRP and one for the D-IRP;
        TRUE            // Query for state change.
        );

    //
    // Finish the power IRP operation.
    //
    ToasterFinalizeDevicePowerIrp(DeviceObject, Irp, Direction, status);
}


VOID
ToasterQueuePassiveLevelPowerCallbackWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This routine is the callback for ToasterQueuePassiveLevelPowerCallback.

Arguments:

   DeviceObject - pointer to a device object.

   Context - Context containing Irp and Direction.

Return Value:

   Nothing

--*/
{
    PWORKER_THREAD_CONTEXT  context;

    PAGED_CODE();

    context = (PWORKER_THREAD_CONTEXT) Context;

    context->Callback(
        context->DeviceObject,
        context->Irp,
        context->IrpDirection
        );

    //
    // Cleanup before exiting from the worker thread.
    //
    IoFreeWorkItem(context->WorkItem);
    ExFreePool((PVOID)context);
}


NTSTATUS
ToasterPowerBeginQueuingIrps(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  ULONG               IrpIoCharges,
    IN  BOOLEAN             Query
    )
/*++

Routine Description:

    This routine causes new Irp requests to be queued and existing IRP requests
    to be finished.

Arguments:

   DeviceObject - pointer to a device object.

   IrpIoCharges - Number of RfDriverIoIncrement calls charged against the
                  caller of this function.

   Query - TRUE if the driver should be queried as to whether the queuing
           should occur.

Return Value:

   NT status code

--*/
{
    NTSTATUS status;
    PFDO_DATA fdoData;
    ULONG chargesRemaining;

    PAGED_CODE();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    fdoData->QueueState = HoldRequests;

    //
    // Wait for the I/O in progress to be finish. The Stop event gets set when
    // the counter drops to One. Our caller might have had Io counts charged
    // against his IRPs too, so we also adjust for those.
    //
    chargesRemaining = IrpIoCharges;
    while(chargesRemaining--) {

        ToasterIoDecrement(fdoData);
    }

    KeWaitForSingleObject(
        &fdoData->StopEvent,
        Executive,   // Waiting for reason of a driver
        KernelMode,  // Waiting in kernel mode
        FALSE,       // No alert
        NULL         // No timeout
        );

    //
    // We might be forced to queue if the machine is running out of power.
    // We only ask if asking is optional.
    //
    if (Query) {

        //
        // Here's where we'd see if we could stop using our device. We'll
        // pretend to be successful instead.
        //
        status = ToasterCanSuspendDevice(DeviceObject);

        if (!NT_SUCCESS(status)) {

            //
            // Open the locks and process anything that actually got
            // queued.
            //
            fdoData->QueueState = AllowRequests;
            ToasterProcessQueuedRequests(fdoData);
        }

    } else {

        status = STATUS_SUCCESS;
    }

    //
    // Increment the counter back to take into account the S-IRP and D-IRP
    // currently in progress.
    //
    chargesRemaining = IrpIoCharges;
    while(chargesRemaining--) {

        ToasterIoIncrement(fdoData);
    }

    return status;
}


//
// Begin device-specific code
// --------------------------
//

VOID
ToasterCallbackHandleDeviceSetPower(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction
    )
/*++

Routine Description:

   This routine performs the actual power changes to the device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an D-IRP.

   Direction -  Whether to forward the D-IRP down or not.
                This depends on whether the system is powering
                up or down.
Return Value:

   NT status code

--*/
{
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    POWER_ACTION        newDeviceAction;
    DEVICE_POWER_STATE  newDeviceState, oldDeviceState;
    POWER_STATE         newState;
    NTSTATUS            status = STATUS_SUCCESS;

    ToasterDebugPrint(TRACE, "Entered ToasterCallbackHandleDeviceSetPower\n");

    //
    // Update our state.
    //
    newState =  stack->Parameters.Power.State;
    newDeviceState = newState.DeviceState;
    oldDeviceState = fdoData->DevicePowerState;
    fdoData->DevicePowerState = newDeviceState;

    ToasterDebugPrint(INFO, "\tSetting the device state to %s\n",
                            DbgDevicePowerString(fdoData->DevicePowerState));

    if (newDeviceState == oldDeviceState) {

        //
        // No change, there is just one thing we need to do before we leave.
        // The device may be queuing due to a prior QueryPower, and the Power
        // Manager sends a SetPower(S0->D0) command.
        //
        if (newDeviceState == PowerDeviceD0) {

            fdoData->QueueState = AllowRequests;
            ToasterProcessQueuedRequests(fdoData);
        }

        ToasterFinalizeDevicePowerIrp(
            DeviceObject,
            Irp,
            Direction,
            STATUS_SUCCESS
            );

        return;
    }

    if (oldDeviceState == PowerDeviceD0) {

        status = ToasterPowerBeginQueuingIrps(
            DeviceObject,
            2,              // One for the S IRP, one for the D IRP.
            FALSE           // Do not query for state change.
            );

        ASSERT(NT_SUCCESS(status));
    }

    //
    // This is why we are going to sleep (note - this field is not dependable
    // if the D state is PowerDeviceD0).
    //
    newDeviceAction = stack->Parameters.Power.ShutdownType;

    if (newDeviceState > oldDeviceState) {

        //
        // We are entering a deeper sleep state. Save away the appropriate
        // state and update our hardware. Note that this particular driver does
        // not care to distinguish Hibernates from shutdowns or standbys. If we
        // did the logic would also have to examine newDeviceAction.
        //
        PoSetPowerState(DeviceObject, DevicePowerState, newState);

        status = STATUS_SUCCESS;

    } else if (newDeviceState < oldDeviceState) {

        //
        // We are entering a lighter sleep state. Restore the appropriate amount
        // of state to our hardware.
        //
        PoSetPowerState(DeviceObject, DevicePowerState, newState);

        //
        // We might fail this IRP if our hardware is missing.
        //
        status = STATUS_SUCCESS;
    }

    if (newDeviceState == PowerDeviceD0) {

        //
        // Our hardware is now on again. Here we empty our existing queue of
        // requests and let in new ones. Note that if this is a D0->D0 (ie
        // no change) we will unblock our queue, which may have been blocked
        // processing our Query-D IRP.
        //
        fdoData->QueueState = AllowRequests;
        ToasterProcessQueuedRequests(fdoData);
    }

    ToasterFinalizeDevicePowerIrp(
        DeviceObject,
        Irp,
        Direction,
        status
        );
}


NTSTATUS
ToasterGetPowerPoliciesDeviceState(
    IN  PIRP                SIrp,
    IN  PDEVICE_OBJECT      DeviceObject,
    OUT DEVICE_POWER_STATE *DevicePowerState
    )
/*++

Routine Description:

    This routine retrieves the appropriate D state for a given S-IRP.

Arguments:

   SIRP - S-IRP to retrieve D-IRP for

   DeviceObject - pointer to a device object

   DevicePowerState - D-IRP to request for passed in S-IRP

Return Value:

   STATUS_SUCCESS if D-IRP is to be requested, failure otherwise.

--*/
{
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(SIrp);
    SYSTEM_POWER_STATE  systemState = stack->Parameters.Power.State.SystemState;
    DEVICE_POWER_STATE  deviceState;
    ULONG               wakeSupported;

    //
    // Read the D-IRP out of the S->D mapping array we captured in QueryCap's.
    // We can choose deeper sleep states than our mapping (with the appropriate
    // caveats if we have children), but we can never choose lighter ones, as
    // what hardware stays on in a given S-state is a function of the
    // motherboard wiring.
    //
    // Also note that if a driver rounds down it's D-state, it must ensure that
    // such a state is supported. A driver can do this by examining the
    // DeviceD1 and DeviceD2 flags (Win2k, XP, ...), or by examining the entire
    // S->D state mapping (Win98). Check the ToasterAdjustCapabilities routine.
    //

    if (systemState == PowerSystemWorking) {

        *DevicePowerState = PowerDeviceD0;
        return STATUS_SUCCESS;

    } else if (fdoData->WakeState != WAKESTATE_ARMED) {

        *DevicePowerState = PowerDeviceD3;
        return STATUS_SUCCESS;
    }

    //
    // We are armed for wake. Pick the best state that supports wakeup. First
    // check the S-state. If system-wake doesn't support wake, we'll fail it.
    // We don't do this check for shutdown or hibernate to make the users life
    // more pleasant.
    //
    if ((systemState <= PowerSystemSleeping3) &&
        (systemState > fdoData->DeviceCaps.SystemWake)) {

        //
        // Force an attempt to go to the next S-state.
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Pick the deepest D state that supports wake, but don't choose a state
    // lighter than that specified in the S->D mapping.
    //
    for(deviceState = PowerDeviceD3;
        deviceState >= fdoData->DeviceCaps.DeviceState[systemState];
        deviceState--) {

        switch(deviceState) {

            case PowerDeviceD0:
                wakeSupported = fdoData->DeviceCaps.WakeFromD0;
                break;

            case PowerDeviceD1:
                wakeSupported = fdoData->DeviceCaps.WakeFromD1;
                break;

            case PowerDeviceD2:
                wakeSupported = fdoData->DeviceCaps.WakeFromD2;
                break;

            case PowerDeviceD3:
                wakeSupported = fdoData->DeviceCaps.WakeFromD3;
                break;

            default:
                ASSERT(0);
                wakeSupported = FALSE;
                break;
        }

        if (wakeSupported) {

            *DevicePowerState = deviceState;
            return STATUS_SUCCESS;
        }
    }

    //
    // We exhausted our wakeable D-states. Fail the request unless we're
    // hibernating or shutting down.
    //
    if (systemState <= PowerSystemSleeping3) {

        return STATUS_UNSUCCESSFUL;
    }

    //
    // We won't bother waking the machine, we'll just do D3.
    //
    *DevicePowerState = PowerDeviceD3;
    return STATUS_SUCCESS;
}


NTSTATUS
ToasterCanSuspendDevice(
    IN PDEVICE_OBJECT   DeviceObject
    )
/*++

Routine Description:

    This routine determines whether a device powerdown request should be
    succeeded or failed.

Arguments:

    DeviceObject - pointer to a device object.

Return Value:

    NTSTATUS code

--*/
{
    return STATUS_SUCCESS;
}



PCHAR
DbgPowerMinorFunctionString (
    UCHAR MinorFunction
    )
{
    switch (MinorFunction)
    {
        case IRP_MN_SET_POWER:
            return "IRP_MN_SET_POWER";
        case IRP_MN_QUERY_POWER:
            return "IRP_MN_QUERY_POWER";
        case IRP_MN_POWER_SEQUENCE:
            return "IRP_MN_POWER_SEQUENCE";
        case IRP_MN_WAIT_WAKE:
            return "IRP_MN_WAIT_WAKE";

        default:
            return "unknown_power_irp";
    }
}

PCHAR
DbgSystemPowerString(
    IN SYSTEM_POWER_STATE Type
    )
{
    switch (Type)
    {
        case PowerSystemUnspecified:
            return "PowerSystemUnspecified";
        case PowerSystemWorking:
            return "PowerSystemWorking";
        case PowerSystemSleeping1:
            return "PowerSystemSleeping1";
        case PowerSystemSleeping2:
            return "PowerSystemSleeping2";
        case PowerSystemSleeping3:
            return "PowerSystemSleeping3";
        case PowerSystemHibernate:
            return "PowerSystemHibernate";
        case PowerSystemShutdown:
            return "PowerSystemShutdown";
        case PowerSystemMaximum:
            return "PowerSystemMaximum";
        default:
            return "UnKnown System Power State";
    }
 }

PCHAR
DbgDevicePowerString(
    IN DEVICE_POWER_STATE Type
    )
{
    switch (Type)
    {
        case PowerDeviceUnspecified:
            return "PowerDeviceUnspecified";
        case PowerDeviceD0:
            return "PowerDeviceD0";
        case PowerDeviceD1:
            return "PowerDeviceD1";
        case PowerDeviceD2:
            return "PowerDeviceD2";
        case PowerDeviceD3:
            return "PowerDeviceD3";
        case PowerDeviceMaximum:
            return "PowerDeviceMaximum";
        default:
            return "UnKnown Device Power State";
    }
}



