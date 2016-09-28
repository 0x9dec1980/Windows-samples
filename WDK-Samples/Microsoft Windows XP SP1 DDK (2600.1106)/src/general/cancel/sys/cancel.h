#ifndef __CANCEL_H
#define __CANCEL_H

#include <ntddk.h>
#ifdef W2K
#include <csq.h> // Include this header file and link with csq.lib to use this sample on Win2K.
#endif 

//  Debugging macros

#if DBG
#define CSAMP_KDPRINT(_x_) \
                DbgPrint("CANCEL.SYS: ");\
                DbgPrint _x_;
#else

#define CSAMP_KDPRINT(_x_)

#endif



#define CSAMP_DEVICE_NAME_U     L"\\Device\\CANCELSAMP"
#define CSAMP_DOS_DEVICE_NAME_U L"\\DosDevices\\CancelSamp"
#define CSAMP_RETRY_INTERVAL    500*1000 //500 ms

typedef struct _INPUT_DATA{

    ULONG Data; //device data is stored here

} INPUT_DATA, *PINPUT_DATA;

typedef struct _DEVICE_EXTENSION{

    BOOLEAN ThreadShouldStop;
    
    // Irps waiting to be processed are queued here
    LIST_ENTRY   PendingIrpQueue;

    //  SpinLock to protect access to the queue
    KSPIN_LOCK QueueLock;

    IO_CSQ CancelSafeQueue;   

    // Time at which the device was last polled
    LARGE_INTEGER LastPollTime;  

    // Polling interval (retry interval)
    LARGE_INTEGER PollingInterval;

    KSEMAPHORE IrpQueueSemaphore;

    PETHREAD ThreadObject;

    
}  DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING registryPath
);

NTSTATUS
CsampCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CsampUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
CsampPollingThread(
    IN PVOID Context
);

NTSTATUS
CsampRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
CsampCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
CsampPollDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP    Irp
    );

VOID 
CsampInsertIrp (
    IN PIO_CSQ   Csq,
    IN PIRP              Irp
    );

VOID 
CsampRemoveIrp(
    IN  PIO_CSQ Csq,
    IN  PIRP    Irp
    );

PIRP 
CsampPeekNextIrp(
    IN  PIO_CSQ Csq,
    IN  PIRP    Irp,
    IN  PVOID  PeekContext
    );

VOID 
CsampAcquireLock(
    IN  PIO_CSQ Csq,
    OUT PKIRQL  Irql
    );

VOID 
CsampReleaseLock(
    IN PIO_CSQ Csq,
    IN KIRQL   Irql
    );

VOID 
CsampCompleteCanceledIrp(
    IN  PIO_CSQ             pCsq,
    IN  PIRP                Irp
    );

#endif


