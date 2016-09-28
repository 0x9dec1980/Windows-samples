/*++
    Copyright (c) 2000,2002 Microsoft Corporation

    Module Name:
        pnp.c

    Abstract:
        This module contains code to handle PnP and Power IRPs.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Mar-2000

    Revision History:
--*/

#include "pch.h"
#define MODULE_ID                       1

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, HpenPnp)
  #pragma alloc_text(PAGE, HpenPower)
  #pragma alloc_text(PAGE, SendSyncIrp)
#endif

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | HpenPnp |
            Plug and Play dispatch routine for this driver.

    @parm   IN PDEVICE_OBJECT | DevObj | Pointer to the device object.
    @parm   IN PIRP | Irp | Pointer to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS EXTERNAL
HpenPnp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    TRACEPROC("HpenPnp", 1)
    NTSTATUS status;
    PDEVICE_EXTENSION devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();
    TRACEENTER(("(DevObj=%p,Irp=%p,IrpSp=%p,Minor=%s)\n",
                DevObj, Irp, irpsp,
                LookupName(irpsp->MinorFunction, PnpMinorIrpNames)));

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
        TRACEWARN(("received IRP after device was removed (status=%x).\n",
                   status));
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
                TRACEASSERT(!(devext->dwfHPen & HPENF_DEVICE_STARTED));
                //
                // Forware the IRP down the stack.
                //
                status = SendSyncIrp(devext->LowerDevObj, Irp, TRUE);
                if (NT_SUCCESS(status))
                {
                    status = OemStartDevice(devext, Irp);
                    if (NT_SUCCESS(status))
                    {
                        devext->dwfHPen |= HPENF_DEVICE_STARTED;
                    }
                }
                else
                {
                    LogError(ERRLOG_LOWERDRV_IRP_FAILED,
                             status,
                             UNIQUE_ERRID(0x20),
                             NULL,
                             NULL);
                    TRACEERROR(("failed to forward IRP (status=%x).\n",
                                status));
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
                if (devext->dwfHPen & HPENF_DEVICE_STARTED)
                {
                    devext->dwfHPen &= ~HPENF_DEVICE_STARTED;
                    status = OemStopDevice(devext, Irp);
                }

                if (NT_SUCCESS(status))
                {
                    fForward = TRUE;
                }
                else
                {
                    Irp->IoStatus.Status = status;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                }
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
                        // the digitizer cannot.
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
                    TRACEERROR(("failed to forward IRP (status=%x).\n",
                                status));
                }
                Irp->IoStatus.Status = status;
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
}       //HpenPnp

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | HpenPower | The power dispatch routine for this driver.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS EXTERNAL
HpenPower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    TRACEPROC("HpenPower", 1)
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
        TRACEERROR(("received IRP after device was removed (status=%x).\n",
                    status));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
        POWER_STATE_TYPE PowerType = irpsp->Parameters.Power.Type;
        POWER_STATE NewPowerState = irpsp->Parameters.Power.State;
        BOOLEAN fSkipCalldown = FALSE;
      #ifdef PERFORMANCE_TEST
        ULONGLONG BeforeTime, AfterTime;
      #endif

        switch (irpsp->MinorFunction)
        {
            case IRP_MN_SET_POWER:
                //
                // We only handle DevicePowerState IRPs that change
                // power states.
                //
                if ((PowerType == DevicePowerState) &&
                    (NewPowerState.DeviceState != devext->PowerState))
                {
                    TRACEINFO(1, ("power state change (%s->%s)\n",
                                  LookupName(devext->PowerState,
                                             PowerStateNames),
                                  LookupName(NewPowerState.DeviceState,
                                             PowerStateNames)));
                    switch (NewPowerState.DeviceState)
                    {
                        case PowerDeviceD0:
                            //
                            // Transitioning from a low D state to D0.
                            //
                          #ifdef PERFORMANCE_TEST
                            TRACEINFO(0, ("Begin:SetD0\n"));
                            BeforeTime = KeQueryInterruptTime();
                          #endif
                            status = SendSyncIrp(devext->LowerDevObj,
                                                 Irp,
                                                 TRUE);
                          #ifdef PERFORMANCE_TEST
                            AfterTime = KeQueryInterruptTime();
                            AfterTime -= BeforeTime;
                            AfterTime /= 10000;
                            LOGDBGMSG((ERRLOG_DEBUG_INFORMATION,
                                       status,
                                       "%d:SetD0",
                                       (ULONG)AfterTime));
                          #endif
                            if (NT_SUCCESS(status))
                            {
                              #ifdef PERFORMANCE_TEST
                                TRACEINFO(0, ("Begin:WakeDevice\n"));
                                BeforeTime = KeQueryInterruptTime();
                              #endif
                                OemWakeupDevice(devext);
                              #ifdef PERFORMANCE_TEST
                                AfterTime = KeQueryInterruptTime();
                                AfterTime -= BeforeTime;
                                AfterTime /= 10000;
                                LOGDBGMSG((ERRLOG_DEBUG_INFORMATION,
                                           0,
                                           "%d:WakeDevice",
                                           (ULONG)AfterTime));
                              #endif
                                devext->PowerState = NewPowerState.DeviceState;
                                devext->dwfHPen &= ~HPENF_DIGITIZER_STANDBY;
                                PoSetPowerState(DevObj,
                                                PowerType,
                                                NewPowerState);
                            }
                            else
                            {
                                LogError(ERRLOG_LOWERDRV_IRP_FAILED,
                                         status,
                                         UNIQUE_ERRID(0x50),
                                         NULL,
                                         NULL);
                                TRACEERROR(("failed to forward IRP (status=%x).\n",
                                            status));
                            }
                            Irp->IoStatus.Status = status;
                            IoReleaseRemoveLock(&devext->RemoveLock, Irp);
                            IoCompleteRequest(Irp, IO_NO_INCREMENT);
                            fSkipCalldown = TRUE;
                            break;

                        case PowerDeviceD1:
                        case PowerDeviceD2:
                        case PowerDeviceD3:
                          #ifdef PERFORMANCE_TEST
                            TRACEINFO(0, ("Begin:StandbyDevice\n"));
                            BeforeTime = KeQueryInterruptTime();
                          #endif
                            OemStandbyDevice(devext);
                          #ifdef PERFORMANCE_TEST
                            AfterTime = KeQueryInterruptTime();
                            AfterTime -= BeforeTime;
                            AfterTime /= 10000;
                            LOGDBGMSG((ERRLOG_DEBUG_INFORMATION,
                                       0,
                                       "%d:StandbyDevice",
                                       (ULONG)AfterTime));
                          #endif
                            devext->PowerState = NewPowerState.DeviceState;
                            devext->dwfHPen |= HPENF_DIGITIZER_STANDBY;
                            PoSetPowerState(DevObj, PowerType, NewPowerState);
                            IoReleaseRemoveLock(&devext->RemoveLock, Irp);

                            Irp->IoStatus.Status = STATUS_SUCCESS;
                            IoSkipCurrentIrpStackLocation(Irp);

                            TRACEENTER((".PoCallDriver(DevObj=%p,Irp=%p)\n",
                                        devext->LowerDevObj, Irp));
                            status = PoCallDriver(devext->LowerDevObj, Irp);
                            TRACEEXIT((".PoCallDriver=%x\n", status));

                            fSkipCalldown = TRUE;
                            break;
                    }
                }
                break;

            case IRP_MN_WAIT_WAKE:
            case IRP_MN_QUERY_POWER:
                break;

            default:
                TRACEERROR(("unsupported power IRP (%s)\n",
                            LookupName(irpsp->MinorFunction,
                                       PowerMinorIrpNames)));
                break;
        }

        if (!fSkipCalldown)
        {
            IoSkipCurrentIrpStackLocation(Irp);
            TRACEENTER((".PoCallDriver(DevObj=%p,Irp=%p)\n",
                        devext->LowerDevObj, Irp));
          #ifdef PERFORMANCE_TEST
            TRACEINFO(0, ("Begin:CallDownPowerIrp\n"));
            BeforeTime = KeQueryInterruptTime();
          #endif
            status = PoCallDriver(devext->LowerDevObj, Irp);
          #ifdef PERFORMANCE_TEST
            AfterTime = KeQueryInterruptTime();
            AfterTime -= BeforeTime;
            AfterTime /= 10000;
            LOGDBGMSG((ERRLOG_DEBUG_INFORMATION,
                       status,
                       "%d:CallDownPowerIrp(%s,%x,%x)",
                       (ULONG)AfterTime,
                       LookupName(irpsp->MinorFunction, PowerMinorIrpNames),
                       irpsp->Parameters.Power.Type,
                       irpsp->Parameters.Power.State));
          #endif
            TRACEEXIT((".PoCallDriver=%x\n", status));

            IoReleaseRemoveLock(&devext->RemoveLock, Irp);
        }
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //HpenPower

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
    NTSTATUS status = Irp->IoStatus.Status;

    TRACEENTER(("(DevObj=%p,Irp=%p,Event=%p,Status=%x)\n",
                DevObj, Irp, Event, status));

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
