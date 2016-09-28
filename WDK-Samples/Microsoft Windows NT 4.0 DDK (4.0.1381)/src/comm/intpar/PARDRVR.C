/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    pardrvr.c

Abstract:

    This module contains the code that does the non-initialization work
    of the parallel driver.

Environment:

    Kernel mode

Revision History :

    The code is set up to be mostly interrupt driven.  Some printers
    won't interrupt when initialized, so the is structured so that
    when it starts a request, it will determine if a previous initialize
    was done - and if so did it complete correctly.

    In addition, code has been adjusted so that an interrupt can't be
    aquired for port, then we will drive the port using a timer dpc
    that will attempt to push X characters out while polling the port.
    If a port isn't ready after Y microseconds then the timer will be
    resubmitted to try again later.

Access to device extension variables

    The access on these variables may be serialized :
    TimerCount, Initialized, Initializing, CountBuffer, Command,
    CompletingIoControl.

    TimerCount is useful for the management of timeout; when the driver
    makes an operation on the controller, it sets a timeout, if the interrupt
    genrated by ACK doesn't arrive the timer routine complete the
    operation with a failure result.
    TimerCount is modified by : InitializeDevice, ManageTimeOut,
    InitiateIo, ISR.

    Initialized is true when the device is initialized.
    Initialized is modified by : InitializeDevice, ISR, InitiateIo.

    Initializing is TRUE when an initialization of the device is going on.
    Initializing is modified by : InitializeDevice, ISR,
    ManageTimeOut.

    CountBuffer is != 0 when a writing is going on, it is the number of
    characters the driver has still to send to the controller to end the
    operation.
    CountBuffer is modified by : Initialize, ManageTimeOut, ISR,
    StartCriticalFunctions.

    Command is the command the driver is servicing.
    Command is modified by : StartCriticalFunctions, ManageIoDevice.

    CompletingIoControl it is useful to synchronize Dpc and TimerRoutine
    when IoControl is completing.

--*/

//
// Note that we include ntddser that we can use the serials
// timeout structure and ioctl to set the timeout for a write.
//

#include <stddef.h>
#include "ntddk.h"
#include "ntddpar.h"
#include "ntddser.h"
#include "par.h"
#include "parlog.h"

static const PHYSICAL_ADDRESS ParPhysicalZero = {0};

#define PAR_INIT_TIMEOUT_VALUE 2

#define VALID_FLAGS PARALLEL_INIT & ( PARALLEL_INIT | PARALLEL_AUTOFEED )

typedef enum _PAR_DPC_ACTION {
    ParCompleteWrite,
    ParCompleteSetIoctl,
    ParUnknownError,
    ParPoweredOff,
    ParNotConnected,
    ParOffline,
    ParPaperEmpty,
    ParBusy,
    ParDebug,
    ParCancel
    } PAR_DPC_ACTION, *PPAR_ACTION;

typedef struct _CONTROL_AREA {
    PPAR_DEVICE_EXTENSION Extension;
    UCHAR Status;
    UCHAR Control;
    PIRP Irp;
} CONTROL_AREA, *PCONTROL_AREA;

typedef struct _PAR_SYNCH_COUNT {
    PPAR_DEVICE_EXTENSION Extension;
    ULONG OldCount;
} PAR_SYNCH_COUNT,*PPAR_SYNCH_COUNT;


//
// Declarations for all internal and external routines used by this module
//



//
//Macros definitions
//

//
// Busy, PE
//

#define PAR_PAPER_EMPTY( Status ) ( \
            (Status & PAR_STATUS_PE) )

//
// Busy, not select, not error
//

#define PAR_OFF_LINE( Status ) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            !(Status & PAR_STATUS_SLCT) )

//
// error, ack, not busy
//

#define PAR_POWERED_OFF( Status ) ( \
            ((Status & PAR_STATUS_NOT_ERROR) ^ PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_NOT_ACK) ^ PAR_STATUS_NOT_ACK) && \
            (Status & PAR_STATUS_NOT_BUSY))

//
// not error, not busy, not select
//

#define PAR_NOT_CONNECTED( Status ) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            (Status & PAR_STATUS_NOT_BUSY) &&\
            !(Status & PAR_STATUS_SLCT) )

//
// not error, not busy
//

#define PAR_OK(Status) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_PE) ^ PAR_STATUS_PE) && \
            (Status & PAR_STATUS_NOT_BUSY) )

//
// not error, not busy, selected.
//
#define PAR_ONLINE(Status) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            (Status & PAR_STATUS_NOT_BUSY) && \
            ((Status & PAR_STATUS_PE) ^ PAR_STATUS_PE) && \
            (Status & PAR_STATUS_SLCT) )

//
// busy, select, not error
//

#define PAR_POWERED_ON(Status) ( \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            (Status & PAR_STATUS_SLCT) && \
            (Status & PAR_STATUS_NOT_ERROR))

//
// busy, not error
//

#define PAR_BUSY(Status) (\
             (( Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
             ( Status & PAR_STATUS_NOT_ERROR ) )

//
// selected
//

#define PAR_SELECTED(Status) ( \
            ( Status & PAR_STATUS_SLCT ) )

//
// No cable attached.
//
#define PAR_NO_CABLE(Status) ( \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            (Status & PAR_STATUS_NOT_ACK) &&                          \
            (Status & PAR_STATUS_PE) &&                               \
            (Status & PAR_STATUS_SLCT) &&                             \
            (Status & PAR_STATUS_NOT_ERROR))

//
// autofeed
//

#define PAR_AUTOFEED( Control ) (\
            ( Control & PAR_CONTROL_AUTOFD ) )


BOOLEAN
ParInitializeDevice(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked to initialize the parallel port drive.
    It performs the following actions:

        o   Sets the timer, because an interrupt is expected on the last
            StoreControl

        o   Send INIT to the driver and if the device is online, it sends
            SLIN

    This routine is invoked by ParInitialize and InterruptServiceRoutine, and
    its execution is synchronized with ISR execution.

Arguments:

    Context - Really the device extension.

Return Value:

    Always FALSE.

--*/

{

    PPAR_DEVICE_EXTENSION extension = Context;

    ParDump(
        PARINITDEV,
        ("PARALLEL: In ParInitializeDevice - device status is %x\n",
          GetStatus(extension->Controller))
        );

    if ((!extension->Initialized) ||
        ((extension->Command == ParSetInformation) &&
         !extension->CompletingIoControl)) {

        UCHAR irqValue = ((extension->UsingATimer)?(0):
                                                   (PAR_CONTROL_IRQ_ENB));

        extension->Initializing = TRUE;

        //
        // Clear the register.
        //

        if (GetControl(extension->Controller) &
            PAR_CONTROL_NOT_INIT) {

            StoreControl(
                extension->Controller,
                (UCHAR)(PAR_CONTROL_WR_CONTROL | irqValue)
                );
            KeStallExecutionProcessor(60);

        }

        if (extension->AutoFeed) {

            StoreControl(
                extension->Controller,
                (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_NOT_INIT |
                        PAR_CONTROL_AUTOFD | irqValue)
                );

        } else {

            StoreControl(
                extension->Controller,
                (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_NOT_INIT |
                        irqValue)
                );

        }

        extension->TimerCount = PAR_INIT_TIMEOUT_VALUE;

    }

    return FALSE;

}

VOID
ParTimerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the driver's timer routine.  It is invoked once every second.
    The value of the timeout counter is set in the Interrupt Service Routine
    (ISR) and decremented by the ParManageTimeOut function, invoked by this
    routine.  If the current operation has timed out, then it is the function
    of this routine to complete the current packet with an error status and
    get the next operation started.

    A timeout may occur under any of the following conditions:

        o   The device is powered off.

        o   The device is disconnected.

        o   The device is off-line.

        o   PE?

Arguments:

    DeviceObject - Pointer to the device object associated with this device.

    Context - Unused context parameter.

Return value:

    None.

--*/

{

    PPAR_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(Context);

    //
    // the test isn't in a critical region, because it can loose a round
    // It is useful to avoid the useless ParSynchronizeExecution call.
    //

    if (extension->TimerCount != -1) {

        KIRQL cancelIrql;

        ParDump(
            PARTIMEOUT,
            ("PARALLEL: In ParTimerRoutine - about to sync with ISR\n")
            );
        IoAcquireCancelSpinLock(&cancelIrql);
        ParSynchronizeExecution(
            extension,
            ParManageTimeOut,
            DeviceObject
            );
        IoReleaseCancelSpinLock(cancelIrql);


    }

    if (extension->StormKnocksOutInterrupts) {

        extension->StormKnocksOutInterrupts = FALSE;

        ParLogError(
            DeviceObject->DriverObject,
            DeviceObject,
            extension->OriginalController,
            ParPhysicalZero,
            0,
            0,
            0,
            41,
            STATUS_SUCCESS,
            PAR_INTERRUPT_STORM
            );

    }

}

BOOLEAN
ParManageTimeOut(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine sets the status of the device in the extension when
    an interrupt was expected didn't arrive and timeout was reached.

    This function is synchronized with the ISR.

Arguments:

    Context - Really a pointer to the device object.

Return value:

    Always FALSE.

--*/

{

    PDEVICE_OBJECT deviceObject = Context;
    PPAR_DEVICE_EXTENSION extension = deviceObject->DeviceExtension;

    ParDump(
        PARTIMEOUT,
        ("PARALLEL: In ParManageTimeout\n")
        );

    //
    // Reset the storm interrupt count back to zero on each timer tick.
    //

    extension->UnexpectedCount = 0;

    if (extension->TimerCount < 0) {

        //
        // Nothing to do.
        //
        return FALSE;

    } else {

        //
        // We can assume that we are called with the cancel spinlock
        // held.  Check to see if we have a current irp, and if so
        // was it canceled.
        //

        if (deviceObject->CurrentIrp) {

            if (deviceObject->CurrentIrp->Cancel) {

                ParDump(
                    PARTIMEOUT,
                    ("PARALLEL: In ParManageTimeout - irp %x has been cancelled\n",
                     deviceObject->CurrentIrp)
                    );
                extension->TimerCount = -1;
                IoRequestDpc(
                    deviceObject,
                    deviceObject->CurrentIrp,
                    (PVOID)ParCancel
                    );
                return FALSE;

            }

        }

        if (extension->TimerCount > 0) {

            //
            // We are going to call the isr routine.  We do this
            // so that if the device went on/off line during IO
            // we can start up the IO again.  The driver will NOT
            // put a character out to the device if it is off line.
            //

            ParInterruptServiceRoutine(
                extension->Interrupt,
                extension->DeviceObject
                );
            if (extension->TimerCount != -1) {

                ASSERT(extension->TimerCount);
                extension->TimerCount--;

            }
            return FALSE;

        } else {

            UCHAR deviceStatus = GetStatus(extension->Controller);
            PAR_DPC_ACTION action;

            extension->TimerCount = -1;

            ParDump(
                PARTIMEOUT,
                ("PARALLEL: In timeout status of device is %x\n",
                 deviceStatus)
                );

            //
            // CountBuffer is = 0 if no packet is being writtten or a Dpc
            // is completing a request
            //

            if (extension->Initializing) {

                extension->Initializing = FALSE;

                //
                // This is a patch for the Centronic Interface for the
                // laser printer that hasn't INIT pin, so it doesn't
                // generate an interrupt when it is initialized.
                //

                if (PAR_OK(deviceStatus)) {

                    ParDump(
                        PARTIMEOUT,
                        ("PARALLEL - ParManageTimeout: In centronics patch\n")
                        );
                    extension->Initialized = TRUE;

                    if ((extension->Command == ParSetInformation) &&
                        !extension->CompletingIoControl ) {

                          extension->CompletingIoControl = TRUE;
                          IoRequestDpc(
                              deviceObject,
                              deviceObject->CurrentIrp,
                              (PVOID)ParCompleteSetIoctl
                              );

                    }

                    return FALSE;

                }

            }

            if ((extension->CountBuffer != 0) ||
                ((extension->Command == ParSetInformation ) &&
                 !extension->CompletingIoControl)) {

                extension->CompletingIoControl = TRUE;

                if (PAR_POWERED_OFF(deviceStatus) ||
                    PAR_NO_CABLE(deviceStatus)) {

                    ParDump(
                        PARTIMEOUT,
                        ("PARALLEL - ParManageTimeout: powered off\n")
                        );
                    extension->Initialized = FALSE;
                    action = ParPoweredOff;

                } else if (PAR_NOT_CONNECTED(deviceStatus)) {

                    ParDump(
                        PARTIMEOUT,
                        ("PARALLEL - ParManageTimeout: not connected\n")
                        );
                    extension->Initialized = FALSE;
                    action = ParNotConnected;

                } else if (PAR_OFF_LINE(deviceStatus)) {

                    ParDump(
                        PARTIMEOUT,
                        ("PARALLEL - ParManageTimeout: off line\n")
                        );
                    action = ParOffline;

                } else if (PAR_PAPER_EMPTY(deviceStatus)) {

                    ParDump(
                        PARTIMEOUT,
                        ("PARALLEL - ParManageTimeout: paper empty\n")
                        );
                    action = ParPaperEmpty;

                } else if (PAR_BUSY(deviceStatus)) {

                    ParDump(
                        PARTIMEOUT,
                        ("PARALLEL - ParManageTimeout: busy\n")
                        );
                    action = ParBusy;

                } else {

                    //
                    // This code should never be executed.
                    //

                    ParDump(
                        PARTIMEOUT,
                        ("PARALLEL - ParManageTimeout: shouldn't be here\n")
                        );
                    extension->Initialized = FALSE;
                    action = ParUnknownError;

                }

                IoRequestDpc(
                    deviceObject,
                    deviceObject->CurrentIrp,
                    (PVOID)action
                    );

            }

        }

    }

    return FALSE;
}

BOOLEAN
ParSetTimerStart(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked via KeSynchronize.  It is used to
    set the value that we should start counting down with
    after every successful IO operation.

Arguments:

    Context - Really a pointer to the irp.

Return Value:

    Always False.

--*/

{

    PPAR_DEVICE_EXTENSION extension =
        IoGetCurrentIrpStackLocation((PIRP)Context)
            ->DeviceObject->DeviceExtension;

    PSERIAL_TIMEOUTS New =
        ((PSERIAL_TIMEOUTS)(((PIRP)Context)->AssociatedIrp.SystemBuffer));

    ParDump(
        PARDISPATCH,
        ("Parallel: SET TIME OUT to %d seconds\n",
          extension->TimerStart)
        );
    extension->TimerStart = New->WriteTotalTimeoutConstant / 1000;

    return FALSE;
}

NTSTATUS
ParDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the main dispatch routine for the parallel port driver.
    It is given a pointer to the IRP for the current request and
    it determines what to do with it. If the request is valid and doen't
    have any parameter errors, then it is placed into the work queue.
    Otherwise it is not completed and an appropriate error is returned.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PPAR_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

    ParDump(
        PARDISPATCH,
        ("PARALLEL: In main dispatch with IRP: %x\n",
         Irp)
        );
    Irp->IoStatus.Information=0L;
    switch(irpSp->MajorFunction) {

        case IRP_MJ_CLOSE: {

            ParDump(
                PARDISPATCH,
                ("PARALLEL: About to close the port for extension: %x\n",
                 extension)
                );
            //
            // Before we close we want to wait until the busy
            // path is false.  We don't want any paths of
            // execution running around while we're closed.
            //

            while (extension->BusyPath) {

                KeDelayExecutionThread(
                    KernelMode,
                    FALSE,
                    &extension->BusyDelayAmount
                    );

            }
            break;


        }
        case IRP_MJ_CREATE:

            ParDump(
                PARDISPATCH,
                ("PARALLEL: About to open the port for extension: %x\n",
                 extension)
                );
            //
            // Let's make sure they aren't trying to create a directory.
            // This is a silly, but what's a driver to do!?
            //

            if (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) {

                status = STATUS_NOT_A_DIRECTORY;

            }

            break;

        case IRP_MJ_WRITE:

            ParDump(
                PARDISPATCH,
                ("PARALLEL: Starting write IRP: %x for extension: %x\n",
                 Irp,
                 extension)
                );
            if ((irpSp->Parameters.Write.ByteOffset.HighPart != 0) ||
                (irpSp->Parameters.Write.ByteOffset.LowPart != 0)) {

                status = STATUS_INVALID_PARAMETER;

            } else {

                if (irpSp->Parameters.Write.Length != 0) {

                    status = STATUS_PENDING;
                }

            }

            break;

        case IRP_MJ_DEVICE_CONTROL:

            ParDump(
                PARDISPATCH,
                ("PARALLEL: Starting device controlIRP: %x for extension: %x\n",
                 Irp,
                 extension)
                );
            switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

                case IOCTL_PAR_SET_INFORMATION :

                    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                        1) {

                        status = STATUS_BUFFER_TOO_SMALL;

                    } else {

                        PPAR_SET_INFORMATION irpBuffer =
                            Irp->AssociatedIrp.SystemBuffer;

                        //
                        // INIT is required, AUTOFEED is optional
                        //

                        if (!(irpBuffer->Init & PARALLEL_INIT) ||
                             (irpBuffer->Init & ~VALID_FLAGS)) {

                            status = STATUS_INVALID_PARAMETER;

                        } else {

                            status = STATUS_PENDING;
                        }

                    }

                    break;

                case IOCTL_PAR_QUERY_INFORMATION :

                    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof(PAR_QUERY_INFORMATION)) {

                        status = STATUS_BUFFER_TOO_SMALL;

                    } else {

                        status = STATUS_PENDING;
                    }

                    break;

                case IOCTL_SERIAL_SET_TIMEOUTS: {

                    PSERIAL_TIMEOUTS NewTimeouts =
                        ((PSERIAL_TIMEOUTS)(Irp->AssociatedIrp.SystemBuffer));

                    ParDump(
                        PARDISPATCH,
                        ("PARALLEL: Got a set timeouts irp: %x for extension: %x\n",
                         Irp,
                         extension)
                        );

                    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                        sizeof(SERIAL_TIMEOUTS)) {

                        status = STATUS_BUFFER_TOO_SMALL;
                        break;

                    } else if (NewTimeouts->WriteTotalTimeoutConstant < 2000) {

                        status = STATUS_INVALID_PARAMETER;
                        break;

                    }

                    ParSynchronizeExecution(
                        extension,
                        ParSetTimerStart,
                        Irp
                        );

                    break;

                }
                case IOCTL_SERIAL_GET_TIMEOUTS:

                    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof(SERIAL_TIMEOUTS)) {

                        status = STATUS_BUFFER_TOO_SMALL;
                        break;

                    }

                    //
                    // We don't need to synchronize the read.
                    //

                    RtlZeroMemory(
                        Irp->AssociatedIrp.SystemBuffer,
                        sizeof(SERIAL_TIMEOUTS)
                        );
                    Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);
                    ((PSERIAL_TIMEOUTS)Irp->AssociatedIrp.SystemBuffer)->
                        WriteTotalTimeoutConstant =
                        extension->TimerStart * 1000;

                    break;

                default :

                    status = STATUS_INVALID_PARAMETER;
                    break;

            }

            break;

    }

    Irp->IoStatus.Status = status;

    if (status == STATUS_PENDING) {

        IoMarkIrpPending(Irp);
        IoStartPacket(
            DeviceObject,
            Irp,
            (PULONG)NULL,
            ParCancelRequest
            );

    } else {

        ParDump(
            PARIRPCOMPLETE,
            ("PARALLEL: About to complete IRP in dispatch: %x\n",Irp)
            );
        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );

    }

    return status;

}

NTSTATUS
ParQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to query the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always returns an end of file of 0.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS status = STATUS_SUCCESS;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);

    ParDump(
        PARDISPATCH,
        ("PARALLEL: In query information file with Irp: %x\n",
         Irp)
        );
    Irp->IoStatus.Information = 0L;
    if (irpSp->Parameters.QueryFile.FileInformationClass ==
        FileStandardInformation) {

        PFILE_STANDARD_INFORMATION buf = Irp->AssociatedIrp.SystemBuffer;

        buf->AllocationSize = RtlConvertUlongToLargeInteger(0ul);
        buf->EndOfFile = buf->AllocationSize;
        buf->NumberOfLinks = 0;
        buf->DeletePending = FALSE;
        buf->Directory = FALSE;
        Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);

    } else if (irpSp->Parameters.QueryFile.FileInformationClass ==
               FilePositionInformation) {

        ((PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->
            CurrentByteOffset = RtlConvertUlongToLargeInteger(0ul);
        Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);

    } else {

        status = STATUS_INVALID_PARAMETER;

    }

    Irp->IoStatus.Status = status;
    ParDump(
        PARIRPCOMPLETE,
        ("PARALLEL: About to complete IRP: %x\n",Irp)
        );
    IoCompleteRequest(
        Irp,
        IO_NO_INCREMENT
        );

    return status;

}

NTSTATUS
ParSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to set the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always ignores the actual end of file since
    the query information code always returns an end of file of 0.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{

    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(DeviceObject);

    ParDump(
        PARDISPATCH,
        ("PARALLEL: In set information with IRP: %x\n",
         Irp)
        );
    Irp->IoStatus.Information = 0L;
    if (IoGetCurrentIrpStackLocation(Irp)->
            Parameters.SetFile.FileInformationClass !=
        FileEndOfFileInformation) {

        status = STATUS_INVALID_PARAMETER;

    }

    Irp->IoStatus.Status = status;
    ParDump(
        PARIRPCOMPLETE,
        ("PARALLEL: About to complete IRP: %x\n",Irp)
        );
    IoCompleteRequest(
        Irp,
        IO_NO_INCREMENT
        );

    return status;

}

VOID
ParCheckTheWeather(
    IN PPAR_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This routine checks to see if we've gotten too many interrupts
    within a certain amount of time.  If so we turn the device into
    a polling device.  This is to deal with interrupt storms.

Arguments:

    Extension - Pointer to the device extension.

Return Value:

    None.

--*/

{

    //
    // We've sometimes seen interrupt storms.  If had no work to do
    // and we are on an EISA or an ISA bus and we've exceeded the
    // threshold for interrupts that weren't ours, then turn off
    // interrupts, turn on the using a timer flag, and set a variable
    // that our one second timer routine will note to cause it to
    // put an event in the event log that this occured.

    if ((Extension->UnexpectedCount > PARALLEL_STORM_WATCH) &&
        (!Extension->UsingATimer) &&
        ((Extension->InterfaceType == Eisa) ||
         (Extension->InterfaceType == Isa))) {

        if (Extension->AutoFeed) {

            StoreControl(
                Extension->Controller,
                (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_NOT_INIT |
                        PAR_CONTROL_AUTOFD)
                );

        } else {

            StoreControl(
                Extension->Controller,
                (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_NOT_INIT)
                );

        }

        Extension->StormKnocksOutInterrupts = TRUE;
        Extension->UsingATimer = TRUE;

    }

}

BOOLEAN
ParInterruptServiceRoutine(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the interrupt service routine for the parallel port driver.
    An interrupt will occur under any of the following conditions:

        o   A character was received by the device

        o   The device's power was switched off

        o   In some printers PAPER_EMPTY and OFF_LINE don't generate
            interrupts, but they are read on the status when a character
            is received ( because the printer accepts one more character )

Arguments:

    InterruptObject - Pointer to the interrupt object used to connect
            to the interrupt vector for the parallel port

    Context - Really a Pointer to the device object for the parallel
             port device that is interrupting

Return Value:

    Returns TRUE if an interrupt was expected from the device;
    otherwise FALSE

--*/

{

    LONG saveTimer;
    PDEVICE_OBJECT deviceObject = Context;
    PPAR_DEVICE_EXTENSION extension = deviceObject->DeviceExtension;
    UCHAR deviceStatus;
    PAR_DPC_ACTION action;

    UNREFERENCED_PARAMETER(InterruptObject);

    deviceStatus = GetStatus(extension->Controller);

    ParDump(
        PARISR,
        ("PARALLEL: ISR - device status is %x\n",deviceStatus)
        );
    //
    // saveTimer == -1 implies that there is no current operation
    // in progress.
    //

    saveTimer = extension->TimerCount;
    action = ParUnknownError;
    extension->TimerCount = -1;

    if (extension->Initializing) {

        ParDump(
            PARISR,
            ("PARALLEL: ISR - In initializing code path\n")
            );
        extension->Initializing = FALSE;

        if (PAR_OK(deviceStatus)) {

            extension->Initialized = TRUE;
            if ((extension->Command == ParSetInformation) &&
                !extension->CompletingIoControl) {

                ParDump(
                    PARISR,
                    ("PARALLEL: ISR - Completing IO control\n")
                    );
                extension->CompletingIoControl = TRUE;
                IoRequestDpc(
                    deviceObject,
                    deviceObject->CurrentIrp,
                    (PVOID)ParCompleteWrite
                    );

            } else if ((extension->CountBuffer != 0) &&
                       (saveTimer != -1)) {

                ParDump(
                    PARISR,
                    ("PARALLEL: ISR Doing More writes - count:%d\n",
                     extension->CountBuffer)
                    );
                ParInitiateIo(deviceObject);

            }

            return TRUE;

        } else if ((extension->CountBuffer != 0 ) ||
                   (extension->Command == ParSetInformation) &&
                    !extension->CompletingIoControl) {

            if (PAR_POWERED_OFF(deviceStatus)) {

                //
                // For the real powered off case make sure we aren't
                // getting an interrupt storm from turning an HPxp
                // off on an MIO-X00 adapter.
                //
                // If it is just an simple turning off then bumping
                // the unexpected count up won't hurt.
                //

                extension->UnexpectedCount++;
                action = ParPoweredOff;
                ParCheckTheWeather(extension);

            } else if (PAR_NO_CABLE(deviceStatus)) {

                action = ParPoweredOff;

            } else if (PAR_OFF_LINE(deviceStatus) ||
                       PAR_PAPER_EMPTY(deviceStatus)) {

                //
                // Paper empty or device off line should let
                // the normal time out code handle the problem
                //

                extension->TimerCount = saveTimer;
                return TRUE;

            } else if (PAR_PAPER_EMPTY(deviceStatus)) {

                action = ParPaperEmpty;

            } else if (PAR_NOT_CONNECTED(deviceStatus)) {

                action = ParNotConnected;

            } else if (PAR_BUSY(deviceStatus)) {

                //
                // For the busy state we will just let the regular timeout code
                // handle it.
                //
                extension->TimerCount = saveTimer;
                return TRUE;

            }

            ParDump(
                PARISR | PARISRACTION,
                ("PARALLEL: ISR Completing request with bad status - ACTION: %d\n",
                 action)
                );
            IoRequestDpc(
                deviceObject,
                deviceObject->CurrentIrp,
                (PVOID)action
                );

            return TRUE;

        }

    } else {

        //
        // The port is not being initialized at this point.  Check to see
        // whether or not it has been initialized at all and drive the
        // remainder of the ISR based on the result.
        //

        if (extension->Initialized) {

            ParDump(
                PARISR,
                ("PARALLEL - ISR Inited path\n")
                );


            if (PAR_OK(deviceStatus) && PAR_ONLINE(deviceStatus) &&
               (saveTimer != -1)) {

                if (extension->CountBuffer > 0) {

                    ParDump(
                        PARISR,
                        ("PARALLEL - ISR Inited - more io - count:%d\n",
                         extension->CountBuffer)
                        );
                    ParInitiateIo(deviceObject);

                } else {

                    ParDump(
                        PARISR,
                        ("PARALLEL - ISR Inited - good io all done:\n")
                        );
                    IoRequestDpc(
                        deviceObject,
                        deviceObject->CurrentIrp,
                        (PVOID)ParCompleteWrite
                        );

                }

                return TRUE;

            } else if ((PAR_OK(deviceStatus)) && (saveTimer != -1) &&
                       (extension->CountBuffer == 0)) {

                //
                // The odd case of the device going off line just after
                // we give the hardware the very last byte.
                //

                ParDump(
                    PARISR,
                    ("PARALLEL - ISR Inited - good io all done:\n")
                    );
                IoRequestDpc(
                    deviceObject,
                    deviceObject->CurrentIrp,
                    (PVOID)ParCompleteWrite
                    );

                return TRUE;

            } else {

                if ((saveTimer != -1) &&
                    (PAR_PAPER_EMPTY(deviceStatus) ||
                     PAR_OFF_LINE(deviceStatus))) {

                    //
                    // Paper empty or device off line should let
                    // the normal time out code handle the problem
                    //

                    ParDump(
                        PARISRACTION,
                        ("PARALLEL: Isr found device to be off line or empty\n")
                        );
                    extension->TimerCount = saveTimer;
                    return TRUE;

                } else if (PAR_POWERED_OFF(deviceStatus) ||
                           PAR_NOT_CONNECTED(deviceStatus) ||
                           PAR_NO_CABLE(deviceStatus)) {

                    extension->Initialized = FALSE;

                    if (saveTimer != -1) {

                        if (PAR_POWERED_OFF(deviceStatus)) {

                            //
                            // For the real powered off case make sure we aren't
                            // getting an interrupt storm from turning an HPxp
                            // off on an MIO-X00 adapter.
                            //
                            // If it is just an simple turning off then bumping
                            // the unexpected count up won't hurt.
                            //

                            extension->UnexpectedCount++;
                            action = ParPoweredOff;
                            ParCheckTheWeather(extension);

                        } else if (PAR_NO_CABLE(deviceStatus)) {

                            action = ParPoweredOff;

                        }

                        if (PAR_NOT_CONNECTED(deviceStatus)) {

                            action = ParNotConnected;

                        }

                        ParDump(
                            PARISR,
                            ("PARALLEL - ISR Inited off/connected complete "
                             "irp - action %d\n",action)
                            );
                        IoRequestDpc(
                            deviceObject,
                            deviceObject->CurrentIrp,
                            (PVOID)action
                            );

                        return TRUE;

                    } else {

                        //
                        // Well we weren't doing an operation, but
                        // the device has changed state since we
                        // last looked.  So this interrupt might
                        // as well have been for us.
                        //

                        return TRUE;

                    }

                } else if ((PAR_BUSY(deviceStatus)) &&
                           (saveTimer != -1)) {

                    ParDump(
                        PARISR | PARBUSYPATH,
                        ("PARALLEL - ISR inited - busy interrupt?\n")
                        );

                    //
                    // Well we think that we have work to do, but for
                    // some reason we got an interrupt, yet the device
                    // thinks it's busy.
                    //
                    // Three things could have happened.
                    //
                    // 1) We are running this device off a timer
                    //    (NO interrupts are ever being used).
                    //    The driver is just calling the ISR to
                    //    force the work to be done.
                    //
                    // 2) We are on some kind of Jazz machine.  We
                    //    *THINK* that the part on the machine is
                    //    signaling an interrupt on just the device
                    //    going BUSY, which is bizarre.
                    //
                    // 3) Some printers (whose name shall go unmentioned
                    //    except that their initials are HP),
                    //    seem to give an interrupt to acknowledge
                    //    the character, *THEN* decide that they want
                    //    to go busy (I think this is bizarre too).
                    //
                    //
                    // For 1 we could just return, since the run off
                    // the timer code will calm down and try to run later
                    // if it gets too many "busys".
                    //
                    // For 2 we could also just return, because we
                    // get another acknowledge interrupt later.
                    //
                    // 3 is quite painful because WE ARE NOT GOING
                    // TO GET ANOTHER INTERRUPT FROM THE DEVICE.
                    //
                    // What we do to solve 3 is to queue off a dpc
                    // that will start a timer that when the timer
                    // fires will cause a dpc to synchrnoize with the
                    // isr and then call this isr.  If the busy bit
                    // has gone away then we will just act as if we
                    // got interrupted.  If the busy bit hasn't gone
                    // away, we just start off the dpc again.
                    //

                    extension->TimerCount = saveTimer;

                    //
                    // If this driver is using a timer to drive the
                    // writes then we should return that we haven't
                    // done anything yet with the io.
                    //

                    if (extension->UsingATimer) {

                        return FALSE;

                    } else {

                        if (!extension->BusyPath) {

                            extension->BusyPath = TRUE;

                            ParDump(
                                PARBUSYPATH,
                                ("Parallel: About to queue the start busy"
                                 "timer\n")
                                );
                            KeInsertQueueDpc(
                                &extension->StartBusyTimerDpc,
                                NULL,
                                NULL
                                );

                        }

                    }

                }

            }

            extension->TimerCount = saveTimer;
            return TRUE;

        } else {

            // not initialized

            if (PAR_POWERED_ON(deviceStatus)) {

                ParDump(
                    PARISR | PARBUSYPATH,
                    ("PARALLEL: ISR - not initialized / not on\n")
                    );
                extension->AutoFeed = FALSE;
                ParInitializeDevice(extension);
                return TRUE;

            }

        }

    }

    //
    // Well we couldn't find anything to do.  Restore the
    // timer value and return FALSE.
    //

    ParDump(
        PARISR,
        ("PARALLEL: ISR returning FALSE with a timer count of %d\n",
         extension->TimerCount)
        );
    extension->TimerCount = saveTimer;
    extension->UnexpectedCount++;

    ParCheckTheWeather(extension);
    return FALSE;

}

BOOLEAN
ParSynchCount(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is synchronized to work at device level (e.g. the
    ISR can't be running).  It is used to clear the count buffer.
    We do this so that when we completing a request prematurely, we
    don't have to refer to a variable that the ISR used to determine
    if it is doing work.  We clear that variable here.

Arguments:

    Context - Pointer to a struct that contains the extension and.
              a ulong to hold the old count.

Return Value:

    Always False.

--*/

{

    PPAR_SYNCH_COUNT synchCount = Context;

    synchCount->OldCount = synchCount->Extension->CountBuffer;
    synchCount->Extension->CountBuffer = 0;

    return FALSE;

}

VOID
ParDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID DeferredContext
    )

/*++

Routine Description:

    This is the DPC routine that complete driver's write operation.

Arguments:

    DeviceObject - Pointer to the device object

    Irp - Pointer to the current Irp

    DeferredContext - Type of action

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER(Dpc);

    ParDump(
        PARDPC,
        ("PARALLEL: In ParDpcRoutine - current irp is: %x\n",
          Irp)
        );

    if (Irp) {

        PPAR_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
        PAR_DPC_ACTION action = (PAR_DPC_ACTION)DeferredContext;
        PAR_SYNCH_COUNT synchCount;
        KIRQL cancelIrql;


        //
        // No matter what, this irp is being completed.
        // Kill the cancel routine.
        //

        IoAcquireCancelSpinLock(&cancelIrql);
        Irp->CancelRoutine = NULL;
        IoReleaseCancelSpinLock(cancelIrql);

        synchCount.Extension = extension;
        synchCount.OldCount = 0;
        ParSynchronizeExecution(
            extension,
            ParSynchCount,
            &synchCount
            );

        switch (action) {

            case ParCompleteWrite :

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = irpSp->Parameters.Write.Length -
                                            synchCount.OldCount;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - Complete Write\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                break;

            case ParCompleteSetIoctl :

                Irp->IoStatus.Status = STATUS_SUCCESS;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - Complete Set Ioctl\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                break;

            case ParPoweredOff :

                Irp->IoStatus.Status = STATUS_DEVICE_POWERED_OFF;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - powered off\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                break;

            case ParNotConnected :

                Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - not connected\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                break;

            case ParOffline :

                Irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
                Irp->IoStatus.Information = irpSp->Parameters.Write.Length -
                                            synchCount.OldCount;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - off line\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                break;

            case ParPaperEmpty :

                Irp->IoStatus.Status = STATUS_DEVICE_PAPER_EMPTY;
                Irp->IoStatus.Information = irpSp->Parameters.Write.Length -
                                            synchCount.OldCount;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - paper empty\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                break;

            case ParUnknownError :

                Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - unknown error\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                ParLogError(
                    DeviceObject->DriverObject,
                    DeviceObject,
                    extension->OriginalController,
                    ParPhysicalZero,
                    extension->IrpSequence,
                    irpSp->MajorFunction,
                    0,
                    38,
                    Irp->IoStatus.Status,
                    IO_ERR_NOT_READY
                    );
                break;

            case ParBusy :

                Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
                Irp->IoStatus.Information = irpSp->Parameters.Write.Length -
                                            synchCount.OldCount;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - device busy\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                break;

            case ParCancel:

                Irp->IoStatus.Status = STATUS_CANCELLED;
                Irp->IoStatus.Information = 0;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - cancel part of the case\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                break;


            default :

                Irp->IoStatus.Status = STATUS_DEVICE_DATA_ERROR;
                ParDump(
                    PARDPC,
                    ("PARALLEL: DPC - default part of the case\n"
                     "--------  STATUS/INFORMATON: %x/%x\n",
                     Irp->IoStatus.Status,
                     Irp->IoStatus.Information)
                    );
                ParLogError(
                    DeviceObject->DriverObject,
                    DeviceObject,
                    extension->OriginalController,
                    ParPhysicalZero,
                    extension->IrpSequence,
                    irpSp->MajorFunction,
                    0,
                    39,
                    Irp->IoStatus.Status,
                    IO_ERR_NOT_READY
                    );
                break;
        }

        IoStartNextPacket(
            DeviceObject,
            TRUE
            );
        ParDump(
            PARIRPCOMPLETE,
            ("PARALLEL: About to complete IRP: %x\n",Irp)
            );
        IoCompleteRequest(
            Irp,
            IO_PARALLEL_INCREMENT
            );

    }

}

BOOLEAN
ParStartCriticalFunctions(
    IN PVOID Context
    )

/*++

Routine Description :

    This routine starts the writing operations on the device.

Arguments :

    Context - Really a pointer to the device object for the parallel port
                    device

Return value :

    Always FALSE;

--*/

{
    PDEVICE_OBJECT deviceObject = Context;
    PIO_STACK_LOCATION irpSp =
        IoGetCurrentIrpStackLocation(deviceObject->CurrentIrp);
    PPAR_DEVICE_EXTENSION extension = deviceObject->DeviceExtension;

    extension->Command = ParWrite;

    //
    // Initialize the state machine for this packet so the remainder of the
    // driver can determine the number of characters that are to be written
    // to the device.
    //

    extension->CompletingIoControl = FALSE;
    extension->CountBuffer = irpSp->Parameters.Write.Length;

    //
    // patch for HP laserjet that hasn't INIT pin
    //

    if (!extension->Initialized) {     //begin HP patch

        UCHAR deviceStatus = GetStatus(extension->Controller);

        ParDump(
            PARCRIT,
            ("PARALLEL: Critical - In HP patch\n")
            );
        if (PAR_OK(deviceStatus)) {

            ParDump(
                PARCRIT,
                ("PARALLEL: Critical - It was initialized\n")
                );
            extension->Initialized = TRUE;

        }

    }                                          //end HP patch


    if (extension->Initialized) {

        ParDump(
            PARCRIT,
            ("PARALLEL: Critical - Sending a byte\n")
            );
        ParInitiateIo(deviceObject);

    } else {

        ParDump(
            PARCRIT,
            ("PARALLEL: Critical - reinitializing the device\n")
            );
       extension->AutoFeed = FALSE;
       ParInitializeDevice(extension);

    }

    //
    // If we are running off the timer then just
    // queue off the "polling" code.
    //

    if (extension->UsingATimer) {

        ParDump(
            PARCRIT,
            ("PARALLEL: Critical - queueing off the polling DPC\n")
            );
        KeInsertQueueDpc(
            &extension->PollingDpc,
            NULL,
            NULL
            );

    }

    return FALSE;

}

BOOLEAN
ParManageIoDevice(
     IN PVOID Context
     )

/*++

Routine Description :

    This routine starts the IoControl commands.

Arguments :

    Context - Really a pointer to a communication area between StartIo
              and ManageIoDevice

Return Value :

    Always FALSE

--*/
{
    PCONTROL_AREA ioControlArea = Context;
    PPAR_DEVICE_EXTENSION extension = ioControlArea->Extension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(ioControlArea->Irp);

    extension->CompletingIoControl = FALSE;

    if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_PAR_SET_INFORMATION) {

        PPAR_SET_INFORMATION irpBuffer =
            ioControlArea->Irp->AssociatedIrp.SystemBuffer;

        extension->Command = ParSetInformation;

        extension->AutoFeed = (BOOLEAN)
                             ((irpBuffer->Init & PARALLEL_AUTOFEED) != 0);

        ParInitializeDevice(extension);

    } else if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_PAR_QUERY_INFORMATION) {

        ioControlArea->Status = GetStatus(extension->Controller);
        ioControlArea->Control = GetControl(extension->Controller);
        extension->Command = ParQueryInformation;
        extension->CompletingIoControl = TRUE;

    }

    return FALSE;
}

VOID
ParStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine starts an I/O operation for the driver and
    then returns

Arguments:

    DeviceObject - Pointer to the device object of this device

    Irp - Pointer to the current IRP

Return Value:

    None

--*/

{
    PPAR_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Increment the following value so that we can have a "unique"
    // value to give to the error log routine.
    //

    extension->IrpSequence++;

    ParDump(
        PARDISPATCH,
        ("PARALLEL: In startio with IRP: %x\n",
         Irp)
        );
    if (irpSp->MajorFunction == IRP_MJ_WRITE) {

        ParDump(
            PARSTART,
            ("PARALLEL - startio - Starting off a write\n")
            );
        ParSynchronizeExecution(
            extension,
            ParStartCriticalFunctions,
            DeviceObject
            );

    } else if (irpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) {

        CONTROL_AREA ioControlArea;

        ioControlArea.Extension = extension;
        ioControlArea.Irp = Irp;

        ParDump(
            PARSTART,
            ("PARALLEL - startio - Starting off a device io control\n")
            );
        ParSynchronizeExecution(
            extension,
            ParManageIoDevice,
            &ioControlArea
            );

        if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_PAR_QUERY_INFORMATION) {

            PPAR_QUERY_INFORMATION irpBuffer = Irp->AssociatedIrp.SystemBuffer;

            ParDump(
                PARSTART,
                ("PARALLEL - startio - Starting off a query\n")
                );
            Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // Interpretating Status & Control
            //

            irpBuffer->Status = 0x0;

            if (PAR_POWERED_OFF(ioControlArea.Status) ||
                PAR_NO_CABLE(ioControlArea.Status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_POWER_OFF);

            } else if (PAR_PAPER_EMPTY(ioControlArea.Status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_PAPER_EMPTY);

            } else if (PAR_OFF_LINE(ioControlArea.Status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_OFF_LINE);

            } else if (PAR_NOT_CONNECTED(ioControlArea.Status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_NOT_CONNECTED);

            }

            if ( PAR_BUSY( ioControlArea.Status ) ) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_BUSY);

            }

            if (PAR_SELECTED(ioControlArea.Status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_SELECTED);

            }

            if (PAR_AUTOFEED(ioControlArea.Control)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_AUTOFEED);

            }

            Irp->IoStatus.Information = sizeof( PAR_QUERY_INFORMATION );
            IoStartNextPacket(
                DeviceObject,
                TRUE
                );
            ParDump(
                PARIRPCOMPLETE,
                ("PARALLEL: About to complete IRP: %x\n",Irp)
                );
            IoCompleteRequest(
                Irp,
                IO_NO_INCREMENT
                );

        }

    }

}

BOOLEAN
ParInitiateIo(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine writes UCHARs to the parallel port device.
    It is called from StartCriticalFunctions and ISR.  In other
    words, it fully expects to be synchronized with the isr.

Arguments:

    DeviceObject - Pointer to the device object of the device

Return Value:

    Always FALSE.

--*/

{
    PPAR_DEVICE_EXTENSION extension;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PUCHAR irpBuffer;

    extension = DeviceObject->DeviceExtension;

    if (extension->Initialized) {

        //
        // We only want to write to the port when the device is online.
        //

        UCHAR deviceStatus;
        ULONG numberWritten = 0;
        ULONG maxNumberToWrite;

        if (extension->InterruptMode = Latched) {

            maxNumberToWrite = 256;

        } else {

            maxNumberToWrite = 32;

        }
        irp = DeviceObject->CurrentIrp;
        irpSp = IoGetCurrentIrpStackLocation(irp);
        irpBuffer = (PUCHAR)(irp->AssociatedIrp.SystemBuffer);
        irpBuffer = &irpBuffer[irpSp->Parameters.Write.Length -
                               extension->CountBuffer];
        do {

            deviceStatus = GetStatus(extension->Controller);

            if (PAR_ONLINE(deviceStatus)) {

                StoreData(
                    extension->Controller,
                    *irpBuffer
                    );

                irpBuffer++;
                extension->CountBuffer--;
                numberWritten++;

            } else {

                ParDump(
                    PARCRIT,
                    ("PARALLEL: Initiate IO - device is not on line, status: %x\n",
                     deviceStatus)
                    );

                break;

            }

        } while (extension->CountBuffer && (numberWritten < maxNumberToWrite));

    }
    extension->TimerCount = extension->TimerStart;
    return FALSE;
}

VOID
ParCancelRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel any request in the parallel driver.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    //
    // If this is the current request then just let normal
    // code detect that the current is cancelled.
    //
    // If this isn't the current request then remove the request
    // from the device queue and complete it.
    //

    if (Irp != DeviceObject->CurrentIrp) {

        KeRemoveEntryDeviceQueue(
            &DeviceObject->DeviceQueue,
            &Irp->Tail.Overlay.DeviceQueueEntry
            );

        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;

        IoReleaseCancelSpinLock(Irp->CancelIrql);

        ParDump(
            PARIRPCOMPLETE,
            ("PARALLEL: About to complete IRP: %x\n",Irp)
            );
        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );

    } else {

        ParDump(
            PARIRPCOMPLETE,
            ("PARALLEL: Can't complete (CANCEL) IRP: %x because it's current\n",
             Irp)
            );
        IoReleaseCancelSpinLock(Irp->CancelIrql);

    }

}

BOOLEAN
ParSynchronizeExecution(
    IN PPAR_DEVICE_EXTENSION Extension,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    This routine is used to abstract whether we are using
    an interrupt object or a simple spin lock.

Arguments:

    Extension - The device extension for the port.

    SynchronizeRoutine - A routine to call when the lock
                         is aquired.

    SynchronizeContext - What to pass to the synchronization
                         routine.

Return Value:

    Whatever the synchronization routine returns.

--*/

{

    KIRQL oldIrql;
    BOOLEAN returnValue;

    KeAcquireSpinLock(
        &Extension->PollingLock,
        &oldIrql
        );

    if (Extension->Interrupt) {

        //
        // If we've haven't dropped down to using a timer
        // then the acquiring the polling lock is a bit
        // of extra overhead, but hopefully not too harmful.
        // We did have to pass through this irql anyway.
        //

        if (!Extension->UsingATimer) {

            //
            // We have a small race where the isr is just about
            // to set this to true.  This is not really a big deal.
            // We will simply synchronize once using the interrupt
            // object and thereafter use the PollingLock.  Overall
            // we are always protected by the polling lock.
            //

            returnValue = KeSynchronizeExecution(
                              Extension->Interrupt,
                              SynchronizeRoutine,
                              SynchronizeContext
                              );

        } else {

            returnValue = SynchronizeRoutine(
                              SynchronizeContext
                              );

        }

    } else {

        returnValue = SynchronizeRoutine(
                          SynchronizeContext
                          );

    }

    KeReleaseSpinLock(
        &Extension->PollingLock,
        oldIrql
        );

    return returnValue;

}

VOID
ParPollingDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is invoked by the polling timer.  It will
    call the parsynchronizeexecution routine to call the
    real polling routine.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPAR_DEVICE_EXTENSION extension = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    ParSynchronizeExecution(
        extension,
        ParPolling,
        extension
        );

}

BOOLEAN
ParPolling(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to drive write requests on a device
    that doesn't isn't using interrupts.

Arguments:

    Context - really a pointer to the device extension

Return Value:

    None.

--*/

{

    PPAR_DEVICE_EXTENSION extension = Context;
    KIRQL oldIrql;
    ULONG callToIsr;
    ULONG ticks;
#define MAXTICKSTOWAIT    10000
#define MINTICKSTOWAIT    1
#define MAXTIMESCALLTOISR 10000
#define MAXMIKESINHERE    100000
    ULONG maxTimesCallToIsr = MAXTIMESCALLTOISR;
    ULONG maxTicksToWait = MAXTICKSTOWAIT;

    //
    // We try to call the isr maxTimesCallToIsr per each invocation
    // of this routine.
    //

    ParDump(
        PARPOLLREPORT,
        ("PARALLEL: Chars starting poll:  %d\n",
         extension->CountBuffer)
        );
    ParDump(
        PARPOLLREPORT,
        ("PARALLEL: Polling irp is:       %x\n",
         extension->DeviceObject->CurrentIrp)
        );

    //
    // If there isn't a current irp (like it got canceled somehow)
    // then simply get out.  This is only to write chars for the current
    // irp.
    //

    if (!extension->DeviceObject->CurrentIrp) {

        return FALSE;

    }

    for (
        callToIsr = 0;
        callToIsr < maxTimesCallToIsr;
        callToIsr++
        ) {

        //
        // We are willing to wait up to MAXTICKSTOWAIT uS "ticks" for each
        // call to the ISR to do something useful.
        //

        for (
            ticks = 0;
            ticks < maxTicksToWait;
            ticks++
            ) {

            if (ParInterruptServiceRoutine(
                    extension->Interrupt,
                    extension->DeviceObject
                    )) {

                //
                // We'll we got something done.  If the timercount
                // is -1 then means we are completing the operation
                // somehow.  If we are completing, let the normal
                // completion code do its work.
                //

                if (extension->TimerCount == -1) {

                    ParDump(
                        PARPOLLREPORT,
                        ("PARALLEL: Polling says we're done\n")
                        );
                    return FALSE;

                }

                //
                // Got did something useful.  Go for more.
                //
                // If this is the second time we did something then
                // adjust the amount of work we do in this routine
                // based on how long it took the call to work.
                // We do the second call because the first call might have
                // been influenced by the fact that this we sitting in the
                // timer queue for a while and this lets the device get
                // all caught up.
                //

                if (callToIsr == 1) {

                    if (ticks < MINTICKSTOWAIT) {

                        ticks = MINTICKSTOWAIT;

                    }

                    //
                    // The maximum amount of time we want to spend
                    // in here is MAXMIKESINHERE.  We divide it by
                    // the number of ticks it took for the second
                    // call to the isr.
                    //

                    maxTicksToWait = ticks;

                    maxTimesCallToIsr = MAXMIKESINHERE / maxTicksToWait;

                    if (maxTimesCallToIsr > MAXTIMESCALLTOISR) {

                        //
                        // If we are on a really fast machine, or
                        // we are on a really fast printer (or both) let's
                        // not use up our full "slice".  Let's get out
                        // after MAXTIMESCALLTOISR.
                        //

                        maxTimesCallToIsr = MAXTIMESCALLTOISR;

                    }

                }
                goto doNextCallToIsr;

            } else {

                //
                // Not ready yet.  Stall for a microsecond.
                //

                KeStallExecutionProcessor(1);

            }

        }

        //
        // We didn't write the character within the amount
        // of time allocated.  Break out of the outer loop
        //

        break;

doNextCallToIsr: ;

    }

    //
    // All done trying to write characters.  Queue a timer to invoke us
    // again.
    //

    KeSetTimer(
        &extension->PollingTimer,
        extension->PollingDelayAmount,
        &extension->PollingDpc
        );

    ParDump(
        PARPOLLREPORT,
        ("PARALLEL: To go at end of poll: %d\n"
         "--------- Ticks per char is:    %d\n"
         "--------- Calls to isr per:     %d\n",
         extension->CountBuffer,
         maxTicksToWait,
         maxTimesCallToIsr)

        );

    return FALSE;
#undef MAXTICKSTOWAIT
#undef MINTICKSTOWAIT
#undef MAXTIMESCALLTOISR
#undef MAXMIKESINHERE
}

VOID
ParStartBusyTimer(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is invoked by the dpc that is queued by the
    ISR when it finds that it has work to do yet the status
    is busy.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPAR_DEVICE_EXTENSION extension = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    ParDump(
        PARBUSYPATH,
        ("Parallel: In start busy timer dpc\n")
        );
    KeSetTimer(
        &extension->BusyTimer,
        extension->BusyDelayAmount,
        &extension->BusyTimerDpc
        );

}

VOID
ParBusyTimer(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is invoked by the dpc that is queued by when
    the busy timer fires.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPAR_DEVICE_EXTENSION extension = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    ParDump(
        PARBUSYPATH,
        ("Parallel: In busy timer dpc\n")
        );
    ParSynchronizeExecution(
        extension,
        ParBusyCallIsr,
        extension
        );

}

BOOLEAN
ParBusyCallIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked via KeSynchronize execution which
    was invoked by the dpc associated with the busy timer.
    Its sole purpose is to call the interrupt service routine.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always False.

--*/

{

    PPAR_DEVICE_EXTENSION extension = Context;

    extension->BusyPath = FALSE;

    ParDump(
        PARBUSYPATH,
        ("Parallel: In busy timer - about to call isr\n")
        );
    ParInterruptServiceRoutine(
        extension->Interrupt,
        extension->DeviceObject
        );

    return FALSE;

}
