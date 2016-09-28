/*++
    Copyright (c) 2000,2002 Microsoft Corporation

    Module Name:
        oempen.c

    Abstract:
        Contains OEM common functions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Mar-2000

    Revision History:
--*/

#include "pch.h"
#define MODULE_ID                       4

//
// Type definitions
//
typedef struct _READ_WORKITEM
{
    PIO_WORKITEM      WorkItem;
    PIRP              Irp;
    PHID_INPUT_REPORT HidReport;
} READ_WORKITEM, *PREAD_WORKITEM;

//
// Local function prototype.
//
NTSTATUS INTERNAL
ReadReportCompletion(
    IN PDEVICE_OBJECT    DevObj,
    IN PIRP              Irp,
    IN PHID_INPUT_REPORT HidReport
    );

NTSTATUS INTERNAL
ProcessResyncBuffer(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS INTERNAL
ProcessInputData(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp,
    IN PHID_INPUT_REPORT HidReport
    );

VOID INTERNAL
ReadMoreBytes(
    IN PDEVICE_OBJECT DevObj,
    IN PREAD_WORKITEM ReadWorkItem
    );

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, OemAddDevice)
  #pragma alloc_text(PAGE, OemStartDevice)
  #pragma alloc_text(PAGE, OemStopDevice)
#endif  //ifdef ALLOC_PRAGMA

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemAddDevice | OEM specific AddDevice code.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns NT status code.
--*/

NTSTATUS INTERNAL
OemAddDevice(
    IN PDEVICE_EXTENSION DevExt
    )
{
    TRACEPROC("OemAddDevice", 2)
    NTSTATUS status;

    PAGED_CODE ();
    TRACEENTER(("(DevExt=%p)\n", DevExt));

    status = STATUS_SUCCESS;

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemAddDevice

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemStartDevice | Start pen tablet device.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
OemStartDevice(
     IN PDEVICE_EXTENSION DevExt,
     IN PIRP              Irp
    )
{
    TRACEPROC("OemStartDevice", 2)
    NTSTATUS status;
    PIO_STACK_LOCATION irpspNext = IoGetNextIrpStackLocation(Irp);
    NTSTATUS PrevStatus = Irp->IoStatus.Status;
    ULONG_PTR PrevInfo = Irp->IoStatus.Information;

    PAGED_CODE();
    TRACEENTER(("(DevExt=%p,Irp=%p)\n", DevExt, Irp));
    TRACEASSERT(!(DevExt->dwfHPen & HPENF_DEVICE_STARTED));

    RtlZeroMemory(irpspNext, sizeof(*irpspNext));
    irpspNext->MajorFunction = IRP_MJ_CREATE;
    status = SendSyncIrp(DevExt->LowerDevObj, Irp, FALSE);
    if (NT_SUCCESS(status))
    {
        DevExt->dwfHPen |= HPENF_SERIAL_OPENED;
        Irp->IoStatus.Status = PrevStatus;
        Irp->IoStatus.Information = PrevInfo;
        status = OemInitSerialPort(DevExt);
        if (NT_SUCCESS(status))
        {
            status = OemInitDevice(DevExt);
        }
    }
    else
    {
        LogError(ERRLOG_OPEN_SERIAL_FAILED,
                 status,
                 UNIQUE_ERRID(0x10),
                 NULL,
                 NULL);
        TRACEERROR(("Failed to send Create IRP to serial port (status=%x).\n",
                    status));
    }

    if (!NT_SUCCESS(status))
    {
        OemStopDevice(DevExt, Irp);
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemStartDevice

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemStopDevice | Uninitialize the device.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
OemStopDevice(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    TRACEPROC("OemStopDevice", 2)
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    BOOLEAN fFailed = FALSE;

    PAGED_CODE();
    TRACEENTER(("(DevExt=%p,Irp=%p)\n", DevExt, Irp));
    TRACEASSERT(!(DevExt->dwfHPen & HPENF_DEVICE_STARTED));

    if (DevExt->dwfHPen & HPENF_SERIAL_INITED)
    {
        DevExt->dwfHPen &= ~HPENF_SERIAL_INITED;
        status = SerialSyncSendIoctl(IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS,
                                     DevExt->LowerDevObj,
                                     &DevExt->OemData.PrevSerialSettings,
                                     sizeof(DevExt->OemData.PrevSerialSettings),
                                     NULL,
                                     0,
                                     TRUE,
                                     &iosb);
        if (!NT_SUCCESS(status))
        {
            fFailed = TRUE;
            LogError(ERRLOG_RESTORE_SERIAL_FAILED,
                     status,
                     UNIQUE_ERRID(0x20),
                     NULL,
                     NULL);
            TRACEWARN(("failed to restore COM settings (status=%x).\n", status));
        }
    }

    if (DevExt->dwfHPen & HPENF_SERIAL_OPENED)
    {
        PIO_STACK_LOCATION irpspNext;

        DevExt->dwfHPen &= ~HPENF_SERIAL_OPENED;
        irpspNext = IoGetNextIrpStackLocation(Irp);
        RtlZeroMemory(irpspNext, sizeof(*irpspNext));
        irpspNext->MajorFunction = IRP_MJ_CLEANUP;
        status = SendSyncIrp(DevExt->LowerDevObj, Irp, FALSE);
        if (!NT_SUCCESS(status))
        {
            fFailed = TRUE;
            TRACEERROR(("Failed to send a Cleanup IRP to the serial port (status=%x).\n",
                        status));
        }

        irpspNext = IoGetNextIrpStackLocation(Irp);
        RtlZeroMemory(irpspNext, sizeof(*irpspNext));
        irpspNext->MajorFunction = IRP_MJ_CLOSE;
        status = SendSyncIrp(DevExt->LowerDevObj, Irp, FALSE);
        if (!NT_SUCCESS(status))
        {
            fFailed = TRUE;
            TRACEERROR(("Failed to send a Close IRP to the serial port (status=%x).\n",
                        status));
        }
    }

    if (fFailed)
    {
        status = STATUS_UNSUCCESSFUL;
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemStopDevice

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemReadReport | Send the read request down to
            serial.sys,

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O Request Packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS INTERNAL
OemReadReport(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    TRACEPROC("OemReadReport", 2)
    NTSTATUS status;
    PHID_INPUT_REPORT HidReport = (PHID_INPUT_REPORT)Irp->UserBuffer;
    KIRQL OldIrql;

    TRACEENTER(("(DevExt=%p,Irp=%p)\n", DevExt, Irp));

    KeAcquireSpinLock(&DevExt->SpinLock, &OldIrql);
    status = ProcessResyncBuffer(DevExt, Irp);
    KeReleaseSpinLock(&DevExt->SpinLock, OldIrql);

    if (!NT_SUCCESS(status))
    {
        //
        // If we don't have enough bytes in the resync buffer or the packet
        // in the resync buffer is invalid, send an IRP down to read some
        // more.
        //
        status = SerialAsyncReadWriteData(TRUE,
                                          DevExt,
                                          Irp,
                                          HidReport->Report.RawInput,
                                          sizeof(OEM_INPUT_REPORT),
                                          ReadReportCompletion,
                                          HidReport);
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemReadReport

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemWriteReport | Handle write report requests.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue Returns STATUS_NOT_SUPPORTED.
--*/

NTSTATUS INTERNAL
OemWriteReport(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP Irp
    )
{
    TRACEPROC("OemWriteReport", 2)
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    TRACEENTER(("(DevExt=%p,Irp=%p)\n", DevExt, Irp));

    LogError(ERRLOG_NOT_SUPPORTED,
             status,
             UNIQUE_ERRID(0x30),
             NULL,
             NULL);
    TRACEWARN(("WriteReport is unsupported.\n"));

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemWriteReport

/*++
    @doc    INTERNAL

    @func   NTSTATUS | ReadReportCompletion | Completion routine for ReadReport.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O request packet.
    @parm   IN PHID_INPUT_REPORT | HidReport | Points to input data packet.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns NT status code.
--*/

NTSTATUS INTERNAL
ReadReportCompletion(
    IN PDEVICE_OBJECT    DevObj,
    IN PIRP              Irp,
    IN PHID_INPUT_REPORT HidReport
    )
{
    TRACEPROC("ReadReportCompletion", 2)
    NTSTATUS status = Irp->IoStatus.Status;
    PDEVICE_EXTENSION devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    TRACEENTER(("(DevObj=%p,Irp=%p,HidReport=%p,Status=%x)\n",
                DevObj, Irp, HidReport, status));

    if (status == STATUS_CANCELLED)
    {
        TRACEWARN(("ReadReport IRP was cancelled.\n"));
        status = STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(status))
    {
        TRACEWARN(("failed to read input data packet (status=%x).\n",
                   status));
        status = STATUS_SUCCESS;
    }
    else
    {
        status = ProcessInputData(devext, Irp, HidReport);
    }

    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    if (status != STATUS_MORE_PROCESSING_REQUIRED)
    {
        IoReleaseRemoveLock(&devext->RemoveLock, Irp);
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //ReadReportCompletion

/*++
    @doc    INTERNAL

    @func   NTSTATUS | ProcessResyncBuffer |
            Process input data from the resync buffer.
            Note that this function must be called at IRQL==DISPATCH_LEVEL

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns STATUS_MORE_PROCESSING_REQUIRED
                      (We want the IRP back).
--*/

NTSTATUS INTERNAL
ProcessResyncBuffer(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    TRACEPROC("ProcessResyncBuffer", 2)
    NTSTATUS status = STATUS_DATA_ERROR;
    PHID_INPUT_REPORT HidReport = (PHID_INPUT_REPORT)Irp->UserBuffer;

    TRACEENTER(("(DevExt=%p,Irp=%p,Len=%d,status=%x,xData=%x,yData=%x)\n",
                DevExt, Irp, DevExt->OemData.BytesInBuff,
                DevExt->OemData.ResyncData[0].InputReport.bStatus,
                DevExt->OemData.ResyncData[0].InputReport.wXData,
                DevExt->OemData.ResyncData[0].InputReport.wYData));

    TRACEASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    while (DevExt->OemData.BytesInBuff >= sizeof(OEM_INPUT_REPORT))
    {
        if (OemIsResyncDataValid(DevExt))
        {
            status = OemNormalizeInputData(DevExt,
                                           &DevExt->OemData.ResyncData[0]);
            if (NT_SUCCESS(status))
            {
                HidReport->ReportID = REPORTID_PEN;
#ifdef _TIMESTAMP_
                HidReport->TimeStamp = DevExt->OemData.LastCounter;
#endif
                RtlCopyMemory(HidReport->Report.RawInput,
                              &DevExt->OemData.ResyncData[0],
                              sizeof(OEM_INPUT_REPORT));
                Irp->IoStatus.Information = sizeof(HID_INPUT_REPORT);
            }

            DevExt->OemData.BytesInBuff -= sizeof(OEM_INPUT_REPORT);
            if (DevExt->OemData.BytesInBuff > 0)
            {
                RtlMoveMemory(&DevExt->OemData.ResyncData[0],
                              &DevExt->OemData.ResyncData[1],
                              DevExt->OemData.BytesInBuff);
            }

            if (NT_SUCCESS(status))
            {
                break;
            }
        }
    }

    TRACEEXIT(("=%x (status=%x,xData=%x,yData=%x)\n",
               status,
               HidReport->Report.InputReport.bStatus,
               HidReport->Report.InputReport.wXData,
               HidReport->Report.InputReport.wYData));
    return status;
}       //ProcessResyncBuffer

/*++
    @doc    INTERNAL

    @func   NTSTATUS | ProcessInputData |
            OEM specific code to process input data.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O request packet.
    @parm   IN PHID_INPUT_REPORT | HidReport | Points to hid report packet.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns STATUS_MORE_PROCESSING_REQUIRED
                     (We want the IRP back).
--*/

NTSTATUS INTERNAL
ProcessInputData(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp,
    IN PHID_INPUT_REPORT HidReport
    )
{
    TRACEPROC("ProcessInputData", 2)
    NTSTATUS status;
    KIRQL OldIrql;

    TRACEENTER(("(DevExt=%p,Irp=%p,HidReport=%p,Len=%d,status=%x,xData=%x,yData=%x)\n",
                DevExt, Irp, HidReport, Irp->IoStatus.Information,
                HidReport->Report.InputReport.bStatus,
                HidReport->Report.InputReport.wXData,
                HidReport->Report.InputReport.wYData));

    KeAcquireSpinLock(&DevExt->SpinLock, &OldIrql);
    if ((DevExt->OemData.BytesInBuff == 0) &&
        (Irp->IoStatus.Information == sizeof(OEM_INPUT_REPORT)) &&
        IsValidPacket(&HidReport->Report))
    {
        status = OemNormalizeInputData(DevExt, &HidReport->Report);
        if (NT_SUCCESS(status))
        {
            HidReport->ReportID = REPORTID_PEN;
#ifdef _TIMESTAMP_
            HidReport->TimeStamp = DevExt->OemData.LastCounter;
#endif
            Irp->IoStatus.Information = sizeof(HID_INPUT_REPORT);
        }
    }
    else
    {
        //
        // Either resync buffer already has something in it or packet is
        // partial or invalid, so append data to resync buffer and process
        // it again.
        //
        RtlMoveMemory((PUCHAR)&DevExt->OemData.ResyncData[0] +
                      DevExt->OemData.BytesInBuff,
                      &HidReport->Report,
                      Irp->IoStatus.Information);
        DevExt->OemData.BytesInBuff += (ULONG)Irp->IoStatus.Information;
        TRACEASSERT(DevExt->OemData.BytesInBuff <=
                    sizeof(DevExt->OemData.ResyncData));
        status = ProcessResyncBuffer(DevExt, Irp);
    }

    if (!NT_SUCCESS(status))
    {
        status = IoAcquireRemoveLock(&DevExt->RemoveLock, Irp);
        if (!NT_SUCCESS(status))
        {
            LogError(ERRLOG_DEVICE_REMOVED,
                     status,
                     UNIQUE_ERRID(0x40),
                     NULL,
                     NULL);
            TRACEERROR(("device was removed (status=%x).\n", status));
        }
        else
        {
            PREAD_WORKITEM ReadWorkItem;

            //
            // No valid data packet, send another IRP down to read more.
            //
            ReadWorkItem = ExAllocatePoolWithTag(NonPagedPool,
                                                 sizeof(READ_WORKITEM),
                                                 HPEN_POOL_TAG);
            if (ReadWorkItem != NULL)
            {
                ReadWorkItem->WorkItem = IoAllocateWorkItem(DevExt->DevObj);
                if (ReadWorkItem != NULL)
                {
                    ReadWorkItem->Irp = Irp;
                    ReadWorkItem->HidReport = HidReport;
                    IoQueueWorkItem(ReadWorkItem->WorkItem,
                                    ReadMoreBytes,
                                    DelayedWorkQueue,
                                    ReadWorkItem);

                    status = STATUS_MORE_PROCESSING_REQUIRED;
                }
                else
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    LogError(ERRLOG_INSUFFICIENT_RESOURCES,
                             status,
                             UNIQUE_ERRID(0x50),
                             NULL,
                             NULL);
                    TRACEERROR(("failed to allocate read work item.\n"));
                }
            }
            else
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                LogError(ERRLOG_INSUFFICIENT_RESOURCES,
                         status,
                         UNIQUE_ERRID(0x60),
                         NULL,
                         NULL);
                TRACEERROR(("failed to allocate read work item data.\n"));
            }
        }
    }
    KeReleaseSpinLock(&DevExt->SpinLock, OldIrql);

    TRACEEXIT(("=%x (status=%x,xData=%x,yData=%x)\n",
               status,
               HidReport->Report.InputReport.bStatus,
               HidReport->Report.InputReport.wXData,
               HidReport->Report.InputReport.wYData));
    return status;
}       //ProcessInputData

/*++
    @doc    INTERNAL

    @func   NTSTATUS | ReadMoreBytes | Read more bytes to resync packet.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PREAD_WORKITEM | ReadWorkItem | Points to the read work item.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue None.
--*/

VOID INTERNAL
ReadMoreBytes(
    IN PDEVICE_OBJECT DevObj,
    IN PREAD_WORKITEM ReadWorkItem
    )
{
    TRACEPROC("ReadMoreBytes", 2)
    PDEVICE_EXTENSION devext;
    ULONG BytesToRead;
    KIRQL OldIrql;

    TRACEENTER(("(DevObj=%p,ReadWorkItem=%p)\n", DevObj, ReadWorkItem));

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    KeAcquireSpinLock(&devext->SpinLock, &OldIrql);
    IoReleaseRemoveLock(&devext->RemoveLock, ReadWorkItem->Irp);
    BytesToRead = sizeof(OEM_INPUT_REPORT) -
                  devext->OemData.BytesInBuff%sizeof(OEM_INPUT_REPORT);
    KeReleaseSpinLock(&devext->SpinLock, OldIrql);

    TRACEINFO(3, ("Read %d more bytes\n", BytesToRead));
    SerialAsyncReadWriteData(TRUE,
                             devext,
                             ReadWorkItem->Irp,
                             ReadWorkItem->HidReport->Report.RawInput,
                             BytesToRead,
                             ReadReportCompletion,
                             ReadWorkItem->HidReport);
    IoFreeWorkItem(ReadWorkItem->WorkItem);
    ExFreePool(ReadWorkItem);

    TRACEASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    TRACEEXIT(("!\n"));
    return;
}       //ReadMoreBytes
