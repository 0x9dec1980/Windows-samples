/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        pnp.c

    Abstract:
        This module contains code to handle PnP and Power IRPs.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Apr-2000

    Revision History:
--*/

#include "pch.h"
#define MODULE_ID                       1

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, HbtnPnp)
  #pragma alloc_text(PAGE, HbtnPower)
  #pragma alloc_text(PAGE, SendSyncIrp)
#endif

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | HbtnPnp |
            Plug and Play dispatch routine for this driver.

    @parm   IN PDEVICE_OBJECT | DevObj | Pointer to the device object.
    @parm   IN PIRP | Irp | Pointer to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS EXTERNAL
HbtnPnp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    TRACEPROC("HbtnPnp", 1)
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    PDEVICE_EXTENSION devext;

    PAGED_CODE();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    TRACEENTER(("(DevObj=%p,Irp=%p,IrpSp=%p,Minor=%s)\n",
                DevObj, Irp, irpsp,
                LookupName(irpsp->MinorFunction, PnpMinorIrpNames)));

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);
    status = IoAcquireRemoveLock(&devext->RemoveLock, Irp);
    if (!NT_SUCCESS(status))
    {
        //
        // Someone sent us another plug and play IRP after removed
        //
        LogError(ERRLOG_DEVICE_REMOVED,
                 status,
                 UNIQUE_ERRID(0x10),
                 NULL,
                 NULL);
        TRACEWARN(("received IRP after device was removed.\n"));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        BOOLEAN fRemove = FALSE;
        BOOLEAN fForward = FALSE;

        switch (irpsp->MinorFunction)
        {
            case IRP_MN_START_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE:
                TRACEASSERT(!(devext->dwfHBtn & HBTNF_DEVICE_STARTED));
                //
                // Forward the IRP down the stack
                //
                status = SendSyncIrp(devext->LowerDevObj, Irp, TRUE);
                if (NT_SUCCESS(status))
                {
                    status = OemStartDevice(devext, Irp);
                    if (NT_SUCCESS(status))
                    {
                        devext->dwfHBtn |= HBTNF_DEVICE_STARTED;
                    }
                }
                else
                {
                    LogError(ERRLOG_LOWERDRV_IRP_FAILED,
                             status,
                             UNIQUE_ERRID(0x20),
                             NULL,
                             NULL);
                    TRACEERROR(("failed to forward IRP.\n"));
                }

                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;

            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_SURPRISE_REMOVAL:
                fRemove = TRUE;
                //
                // Fall through.
                //
            case IRP_MN_STOP_DEVICE:
            case IRP_MN_QUERY_REMOVE_DEVICE:
                //
                // After the start IRP has been sent to the lower driver
                // object, the bus may NOT send any more IRPS down ``touch''
                // until another START has occured.  Whatever access is
                // required must be done before Irp passed on.
                //
                if (devext->dwfHBtn & HBTNF_DEVICE_STARTED)
                {
                    KIRQL OldIrql;
                    PLIST_ENTRY plist;
                    PIRP PendingIrp;

                    status = OemRemoveDevice(devext, Irp);
                    devext->dwfHBtn &= ~HBTNF_DEVICE_STARTED;
                    KeAcquireSpinLock(&devext->SpinLock, &OldIrql);
                    while (!IsListEmpty(&devext->PendingIrpList))
                    {

                        plist = devext->PendingIrpList.Flink;
                        PendingIrp =
                            CONTAINING_RECORD(plist, IRP, Tail.Overlay.ListEntry);
                        KeReleaseSpinLock(&devext->SpinLock, OldIrql);
                        if (!IoCancelIrp(PendingIrp))
                        {
                            TRACEWARN(("Failed to cancel read request IRP.\n"))
                        }
                        KeAcquireSpinLock(&devext->SpinLock, &OldIrql);
                    }
                    KeReleaseSpinLock(&devext->SpinLock, OldIrql);
                }

                //
                // We don't need a completion routine so fire and forget.
                // Set the current stack location to the next stack location and
                // call the next device object.
                //
                fForward = TRUE;
                Irp->IoStatus.Status = status;
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                status = SendSyncIrp(devext->LowerDevObj, Irp, TRUE);
                if (NT_SUCCESS(status))
                {
                    PDEVICE_CAPABILITIES devcaps;

                    devcaps = irpsp->Parameters.DeviceCapabilities.Capabilities;
                    if (devcaps != NULL)
                    {
                        SYSTEM_POWER_STATE i;

                        //
                        // This device is built-in to the system, so it should
                        // be impossible to surprise remove this device, but
                        // we will handle it anyway.
                        //
                        devcaps->SurpriseRemovalOK = TRUE;

                        //
                        // While the underlying serial bus might be able to
                        // wake the machine from low power (via wake on ring),
                        // the tablet cannot.
                        //
                        devcaps->SystemWake = PowerSystemUnspecified;
                        devcaps->DeviceWake = PowerDeviceUnspecified;
                        devcaps->WakeFromD0 =
                                devcaps->WakeFromD1 =
                                devcaps->WakeFromD2 =
                                devcaps->WakeFromD3 = FALSE;
                        devcaps->DeviceState[PowerSystemWorking] =
                                PowerDeviceD0;
                        for (i = PowerSystemSleeping1;
                             i < PowerSystemMaximum;
                             i++)
                        {
                            devcaps->DeviceState[i] = PowerDeviceD3;
                        }
                    }
                }
                else
                {
                    LogError(ERRLOG_LOWERDRV_IRP_FAILED,
                             status,
                             UNIQUE_ERRID(0x30),
                             NULL,
                             NULL);
                    TRACEERROR(("failed to forward IRP.\n"));
                }
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;

            default:
                fForward = TRUE;
                break;
        }

        if (fForward)
        {
            IoSkipCurrentIrpStackLocation(Irp);
            TRACEENTER((".IoCallDriver(DevObj=%p,Irp=%p)\n",
                        devext->LowerDevObj, Irp));
            status = IoCallDriver(devext->LowerDevObj, Irp);
            TRACEEXIT((".IoCallDriver=%x\n", status));
        }

        if (fRemove)
        {
            //
            // Wait for the remove lock to free.
            //
            IoReleaseRemoveLockAndWait(&devext->RemoveLock, Irp);
        }
        else
        {
            IoReleaseRemoveLock(&devext->RemoveLock, Irp);
        }
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //HbtnPnp

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | HbtnPower | The power dispatch routine for this driver.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS EXTERNAL
HbtnPower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    TRACEPROC("HbtnPower", 1)
    NTSTATUS status;
    PDEVICE_EXTENSION devext;

    PAGED_CODE();

    TRACEENTER(("(DevObj=%p,Irp=%p,Minor=%s)\n",
                DevObj, Irp,
                LookupName(IoGetCurrentIrpStackLocation(Irp)->MinorFunction,
                           PowerMinorIrpNames)));

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);
    status = IoAcquireRemoveLock(&devext->RemoveLock, Irp);
    if (!NT_SUCCESS(status))
    {
        //
        // Someone sent us another power IRP after removed
        //
        LogError(ERRLOG_DEVICE_REMOVED,
                 status,
                 UNIQUE_ERRID(0x40),
                 NULL,
                 NULL);
        TRACEERROR(("received IRP after device was removed.\n"));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
      #ifdef PERFORMANCE_TEST
        ULONGLONG BeforeTime, AfterTime;
        PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
      #endif
        IoSkipCurrentIrpStackLocation(Irp);

        TRACEENTER((".PoCallDriver(DevObj=%p,Irp=%p)\n",
                    devext->LowerDevObj, Irp));
      #ifdef PERFORMANCE_TEST
        BeforeTime = KeQueryInterruptTime();
      #endif
        status = PoCallDriver(devext->LowerDevObj, Irp);
      #ifdef PERFORMANCE_TEST
        AfterTime = KeQueryInterruptTime();
        AfterTime -= BeforeTime;
        AfterTime /= 10000;
        LOGDBGMSG((ERRLOG_DEBUG_INFORMATION,
                   status,
                   "%d:PowerIrp(%s,%x,%x)",
                   (ULONG)AfterTime,
                   LookupName(irpsp->MinorFunction, PowerMinorIrpNames),
                   irpsp->Parameters.Power.Type,
                   irpsp->Parameters.Power.State));
      #endif
        TRACEEXIT((".PoCallDriver=%x\n", status));

        IoReleaseRemoveLock(&devext->RemoveLock, Irp);
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //HbtnPower

/*++
    @doc    INTERNAL

    @func   NTSTATUS | SendSyncIrp |
            Send an IRP synchronously down the stack.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to the IRP.
    @parm   IN BOOLEAN | fCopyToNext | if TRUE, copy the irpsp to next location.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS INTERNAL
SendSyncIrp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN BOOLEAN        fCopyToNext
    )
{
    TRACEPROC("SendSyncIrp", 2)
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
    KEVENT event;

    PAGED_CODE();

    TRACEENTER(("(DevObj=%p,Irp=%p,fCopyToNext=%x,MajorFunc=%s)\n",
                DevObj, Irp, fCopyToNext,
                LookupName(irpsp->MajorFunction, MajorIrpNames)));

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
    if (fCopyToNext)
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    IoSetCompletionRoutine(Irp, IrpCompletion, &event, TRUE, TRUE, TRUE);
    if (irpsp->MajorFunction == IRP_MJ_POWER)
    {
        TRACEENTER((".PoCallDriver(DevObj=%p,Irp=%p)\n", DevObj, Irp));
        status = PoCallDriver(DevObj, Irp);
        TRACEEXIT((".IoCallDriver=%x\n", status));
    }
    else
    {
        TRACEENTER((".IoCallDriver(DevObj=%p,Irp=%p)\n", DevObj, Irp));
        status = IoCallDriver(DevObj, Irp);
        TRACEEXIT((".IoCallDriver=%x\n", status));
    }

    if (status == STATUS_PENDING)
    {
        status = KeWaitForSingleObject(&event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
    }

    if (NT_SUCCESS(status))
    {
        status = Irp->IoStatus.Status;
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //SendSyncIrp

/*++
    @doc    INTERNAL

    @func   NTSTATUS | IrpCompletion | Completion routine for all IRPs.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O request packet.
    @parm   IN PKEVENT | Event | Points to the event to notify.

    @rvalue Always returns STATUS_MORE_PROCESSING_REQUIRED.
--*/

NTSTATUS INTERNAL
IrpCompletion(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PKEVENT        Event
    )
{
    TRACEPROC("IrpCompletion", 2)

    TRACEENTER(("(DevObj=%p,Irp=%p,Event=%p)\n", DevObj, Irp, Event));

    UNREFERENCED_PARAMETER(DevObj);

    KeSetEvent(Event, 0, FALSE);

    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    TRACEEXIT(("=%x\n", STATUS_MORE_PROCESSING_REQUIRED));
    return STATUS_MORE_PROCESSING_REQUIRED;
}       //IrpCompletion
