/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        serial.c

    Abstract:
        Functions to talk to the serial port.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 23-Mar-2000

    Revision History:
--*/

#include "pch.h"
#define MODULE_ID                       3

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, SerialSyncSendIoctl)
  #pragma alloc_text(PAGE, SerialSyncReadWritePort)
#endif /* ALLOC_PRAGMA */

/*++
    @doc    INTERNAL

    @func   NTSTATUS | SerialSyncSendIoctl |
            Performs a synchronous ioctl request to the serial port.

    @parm   IN ULONG | IoctlCode | ioctl code.
    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PVOID | InBuffer OPTIONAL | Points to the input buffer.
    @parm   IN ULONG | InBufferLen | Specifies the size of the input buffer.
    @parm   OUT PVOID | OutBuffer OPTIONAL | Points to the output buffer.
    @parm   IN ULONG | OutBufferLen | Specifies the size of the output buffer.
    @parm   IN BOOLEAN | fInternal | If TRUE, an internal ioctl is sent.
    @parm   OUT PIO_STATUS_BLOCK | Iosb | Points to the io status block.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
SerialSyncSendIoctl(
    IN ULONG          IoctlCode,
    IN PDEVICE_OBJECT DevObj,
    IN PVOID          InBuffer OPTIONAL,
    IN ULONG          InBufferLen,
    OUT PVOID         OutBuffer OPTIONAL,
    IN ULONG          OutBufferLen,
    IN BOOLEAN        fInternal,
    OUT PIO_STATUS_BLOCK Iosb
    )
{
    TRACEPROC("SerialSyncSendIoctl", 3)
    NTSTATUS status = STATUS_SUCCESS;
    KEVENT event;
    PIRP irp;

    PAGED_CODE();

    TRACEENTER(("(Ioctl=%s,DevObj=%p,InBuff=%p,InLen=%d,OutBuff=%p,OutLen=%d,fInternal=%x,Iosb=%p)\n",
                LookupName(IoctlCode,
                           fInternal? SerialInternalIoctlNames: SerialIoctlNames),
                DevObj, InBuffer, InBufferLen, OutBuffer, OutBufferLen,
                fInternal, Iosb));

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IoctlCode,
                                        DevObj,
                                        InBuffer,
                                        InBufferLen,
                                        OutBuffer,
                                        OutBufferLen,
                                        fInternal,
                                        &event,
                                        Iosb);
    if (irp != NULL)
    {
        status = IoCallDriver(DevObj, irp);
        if (status == STATUS_PENDING)
        {
            status = KeWaitForSingleObject(&event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);
        }

        if (status == STATUS_SUCCESS)
        {
            status = Iosb->Status;
        }
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(ERRLOG_INSUFFICIENT_RESOURCES,
                 status,
                 UNIQUE_ERRID(0x10),
                 NULL,
                 NULL);
        TRACEERROR(("failed to build ioctl IRP.\n"));
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //SerialSyncSendIoctl

/*++
    @doc    INTERNAL

    @func   NTSTATUS | SerialAsyncReadWriteData |
            Read/Write data from/to the Serial Port asynchornously.

    @parm   IN BOOLEAN | fRead | If TRUE, the access is a Read.
    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O Request Packet.
    @parm   IN PUCHAR | Buffer | Points to the data buffer.
    @parm   IN ULONG | BuffLen | Specifies the data buffer length.
    @parm   IN PIO_COMPLETION_ROUTINE | CompletionRoutine |
            Points to the completion callback routine.
    @parm   IN PVOID | Context | Callback context.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
SerialAsyncReadWriteData(
    IN BOOLEAN                fRead,
    IN PDEVICE_EXTENSION      DevExt,
    IN PIRP                   Irp,
    IN PUCHAR                 Buffer,
    IN ULONG                  BuffLen,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID                  Context
    )
{
    TRACEPROC("SerialAsyncReadWriteData", 3)
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;

    TRACEENTER(("(fRead=%x,DevExt=%p,Irp=%p,Buff=%p,BuffLen=%d,pfnCompletion=%p,Context=%p)\n",
                fRead, DevExt, Irp, Buffer, BuffLen, CompletionRoutine,
                Context));

    TRACEASSERT(Buffer != NULL);
    TRACEASSERT(BuffLen > 0);

    Irp->AssociatedIrp.SystemBuffer = Buffer;
    irpsp = IoGetNextIrpStackLocation(Irp);
    RtlZeroMemory(irpsp, sizeof(*irpsp));
    irpsp->Parameters.Read.Length = BuffLen;
    irpsp->Parameters.Read.ByteOffset.QuadPart = 0;
    irpsp->MajorFunction = fRead? IRP_MJ_READ: IRP_MJ_WRITE;
    IoSetCompletionRoutine(Irp,
                           CompletionRoutine,
                           Context,
                           TRUE,
                           TRUE,
                           TRUE);

    status = IoCallDriver(DevExt->LowerDevObj, Irp);

    TRACEEXIT(("=%x\n", status));
    return status;
}       //SerialAsyncReadWriteData

/*++
    @doc    INTERNAL

    @func   NTSTATUS | SerialAsyncReadWritePort | Read/Write data from/to
            the Serial Port asynchronously.

    @parm   IN BOOLEAN | fRead | If TRUE, the access is a Read.
    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PUCHAR | Buffer | Points to the data buffer.
    @parm   IN ULONG | BuffLen | Specifies the data buffer length.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
SerialAsyncReadWritePort(
    IN BOOLEAN           fRead,
    IN PDEVICE_EXTENSION DevExt,
    IN PUCHAR            Buffer,
    IN ULONG             BuffLen
    )
{
    TRACEPROC("SerialAsyncReadWritePort", 3)
    NTSTATUS status;
    PIRP irp;
    LARGE_INTEGER StartingOffset = RtlConvertLongToLargeInteger(0);

    PAGED_CODE();

    TRACEENTER(("(fRead=%x,DevExt=%p,Buff=%p,BuffLen=%d)\n",
                fRead, DevExt, Buffer, BuffLen));

    TRACEASSERT(Buffer != NULL);
    TRACEASSERT(BuffLen > 0);

    irp = IoBuildAsynchronousFsdRequest(fRead? IRP_MJ_READ: IRP_MJ_WRITE,
                                        DevExt->LowerDevObj,
                                        Buffer,
                                        BuffLen,
                                        &StartingOffset,
                                        NULL);
    if (irp != NULL)
    {
        status = IoAcquireRemoveLock(&DevExt->RemoveLock, irp);
        if (NT_SUCCESS(status))
        {
            IoSetCompletionRoutine(irp,
                                   AsyncReadWriteCompletion,
                                   DevExt,
                                   TRUE,
                                   TRUE,
                                   TRUE);
            TRACEENTER((".IoCallDriver(DevObj=%p,Irp=%p)\n",
                        DevExt->LowerDevObj, irp));
            status = IoCallDriver(DevExt->LowerDevObj, irp);
            TRACEEXIT((".IoCallDriver=%x\n", status));

            if (!NT_SUCCESS(status))
            {
                LogError(ERRLOG_SERIAL_ACCESS_FAILED,
                         status,
                         UNIQUE_ERRID(0x20),
                         NULL,
                         NULL);
                TRACEERROR(("failed to send async read/write irp to COM (status=%x).\n",
                            status));
            }
        }
        else
        {
            IoFreeIrp(irp);
            TRACEWARN(("Failed to acquire remove lock (status=%x).\n", status));
        }
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(ERRLOG_INSUFFICIENT_RESOURCES,
                 status,
                 UNIQUE_ERRID(0x30),
                 NULL,
                 NULL);
        TRACEERROR(("failed to allocate asynchronous IRP.\n"));
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //SerialAsyncReadWritePort

/*++
    @doc    INTERNAL

    @func   NTSTATUS | AsyncReadWriteCompletion | Completion routine for
            AsyncReadWritePort.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O request packet.
    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns NT status code.
--*/

NTSTATUS INTERNAL
AsyncReadWriteCompletion(
    IN PDEVICE_OBJECT    DevObj,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DevExt
    )
{
    TRACEPROC("AsyncReadWriteCompletion", 2)
    NTSTATUS status = Irp->IoStatus.Status;

    TRACEENTER(("(DevObj=%p,Irp=%p,devext=%p,Status=%x)\n",
                DevObj, Irp, DevExt, status));

    if (status == STATUS_CANCELLED)
    {
        TRACEWARN(("AsyncReadWrite IRP was cancelled.\n"));
    }
    else if (!NT_SUCCESS(status))
    {
        TRACEWARN(("AsyncReadWrite irp failed (status=%x).\n", status));
    }

    IoReleaseRemoveLock(&DevExt->RemoveLock, Irp);
    IoFreeIrp(Irp);
    status = STATUS_MORE_PROCESSING_REQUIRED;

    TRACEEXIT(("=%x\n", status));
    return status;
}       //AsyncReadWriteCompletion

/*++
    @doc    INTERNAL

    @func   NTSTATUS | SerialSyncReadWritePort |
            Read/Write data from/to the Serial Port.

    @parm   IN BOOLEAN | fRead | If TRUE, the access is a Read.
    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PUCHAR | Buffer | Points to the data buffer.
    @parm   IN ULONG | BuffLen | Specifies the data buffer length.
    @parm   IN PLARGE_INTEGER | Timeout | Points to an optional timeout value
    @parm   OUT PULONG | BytesAccessed | Optionally returns number of bytes
            accessed.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
SerialSyncReadWritePort(
    IN BOOLEAN           fRead,
    IN PDEVICE_EXTENSION DevExt,
    IN PUCHAR            Buffer,
    IN ULONG             BuffLen,
    IN PLARGE_INTEGER    Timeout OPTIONAL,
    OUT PULONG           BytesAccessed OPTIONAL
    )
{
    TRACEPROC("SerialSyncReadWritePort", 3)
    NTSTATUS status;
    KEVENT event;
    PIRP irp;
    LARGE_INTEGER StartingOffset = RtlConvertLongToLargeInteger(0);
    IO_STATUS_BLOCK iosb;

    PAGED_CODE();

    TRACEENTER(("(fRead=%x,DevExt=%p,Buff=%p,BuffLen=%d,pTimeout=%p,pBytesAccessed=%p)\n",
                fRead, DevExt, Buffer, BuffLen, Timeout, BytesAccessed));

    TRACEASSERT(Buffer != NULL);
    TRACEASSERT(BuffLen > 0);

    if (BytesAccessed != NULL)
    {
        *BytesAccessed = 0;
    }
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(fRead? IRP_MJ_READ: IRP_MJ_WRITE,
                                       DevExt->LowerDevObj,
                                       Buffer,
                                       BuffLen,
                                       &StartingOffset,
                                       &event,
                                       &iosb);
    if (irp != NULL)
    {
        TRACEENTER((".IoCallDriver(DevObj=%p,Irp=%p)\n",
                    DevExt->LowerDevObj, irp));
        status = IoCallDriver(DevExt->LowerDevObj, irp);
        TRACEEXIT((".IoCallDriver=%x\n", status));

        if (status == STATUS_PENDING)
        {
            status = KeWaitForSingleObject(&event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           Timeout);
        }

        if (status == STATUS_SUCCESS)
        {
            status = iosb.Status;
            if (BytesAccessed != NULL)
            {
                *BytesAccessed = (ULONG)iosb.Information;
            }
        }
        else
        {
            if (status == STATUS_TIMEOUT)
            {
                IoCancelIrp(irp);
                KeWaitForSingleObject(&event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
            }

            LogError(ERRLOG_SERIAL_ACCESS_FAILED,
                     status,
                     UNIQUE_ERRID(0x40),
                     NULL,
                     NULL);
            TRACEWARN(("failed accessing COM (status=%x).\n", status));
        }
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(ERRLOG_INSUFFICIENT_RESOURCES,
                 status,
                 UNIQUE_ERRID(0x50),
                 NULL,
                 NULL);
        TRACEERROR(("failed to allocate synchronous IRP.\n"));
    }

    TRACEEXIT(("=%x (BytesAccessed=%d)\n", status, iosb.Information));
    return status;
}       //SerialSyncReadWritePort
