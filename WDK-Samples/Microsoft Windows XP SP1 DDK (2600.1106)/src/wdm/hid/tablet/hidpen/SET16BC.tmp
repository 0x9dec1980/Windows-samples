/*++
    Copyright (c) 2000,2002 Microsoft Corporation

    Module Name:
        serial.h

    Abstract:
        Contains definitions of all Serial Port related functions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Mar-2000

    Revision History:
--*/

#ifndef _SERIAL_H
#define _SERIAL_H

//
// Constants
//

// Serial Port FIFO Control Register bits for Receiver Trigger Level
#define SERIAL_IOC_FCR_RCVR_TRIGGER_01_BYTE     0
#define SERIAL_IOC_FCR_RCVR_TRIGGER_04_BYTES    SERIAL_IOC_FCR_RCVR_TRIGGER_LSB
#define SERIAL_IOC_FCR_RCVR_TRIGGER_08_BYTES    SERIAL_IOC_FCR_RCVR_TRIGGER_MSB
#define SERIAL_IOC_FCR_RCVR_TRIGGER_14_BYTES    (SERIAL_IOC_FCR_RCVR_TRIGGER_LSB |\
                                                 SERIAL_IOC_FCR_RCVR_TRIGGER_MSB)

//
// Function prototypes
//

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
    );

NTSTATUS INTERNAL
SerialAsyncReadWriteData(
    IN BOOLEAN                fRead,
    IN PDEVICE_EXTENSION      DevExt,
    IN PIRP                   Irp,
    IN PUCHAR                 Buffer,
    IN ULONG                  BuffLen,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID                  Context
    );

NTSTATUS INTERNAL
SerialAsyncReadWritePort(
    IN BOOLEAN           fRead,
    IN PDEVICE_EXTENSION DevExt,
    IN PUCHAR            Buffer,
    IN ULONG             BuffLen
    );

NTSTATUS INTERNAL
AsyncReadWriteCompletion(
    IN PDEVICE_OBJECT    DevObj,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS INTERNAL
SerialSyncReadWritePort(
    IN BOOLEAN           fRead,
    IN PDEVICE_EXTENSION DevExt,
    IN PUCHAR            Buffer,
    IN ULONG             BuffLen,
    IN PLARGE_INTEGER    Timeout OPTIONAL,
    OUT PULONG           BytesAccessed OPTIONAL
    );

#endif  //ifndef _SERIAL_H
