/*++

Copyright (c) 1990, 1991, 1992, 1993  Microsoft Corporation

Module Name:

    kbdoasys.c

Abstract:

    Fujitsu FMV oyayubi-shift (oasys) keyboard device.

Author:

    96/09/15 hideyukn

Revision History:

--*/

#include <ntddk.h>
#include <windef.h>
#include <imm.h>
#include "i8042prt.h"
#include "i8042log.h"

VOID
I8xKeyboardInitiateIoForOasys(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called synchronously from I8xKeyboardInitiateWrapper and
    the ISR to initiate an I/O operation for the keyboard device.

Arguments:

    Context - Pointer to the device object.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject;
    KEYBOARD_SET_PACKET keyboardPacket;
    NTSTATUS status;

    I8xPrint((2, "I8042PRT-I8xKeyboardInitiateIoForOasys: enter\n"));

    //
    // Get the device extension.
    //

    deviceObject = (PDEVICE_OBJECT) Context;
    deviceExtension = deviceObject->DeviceExtension;

    //
    // Set the timeout value.
    //

    deviceExtension->TimerCount = I8042_ASYNC_TIMEOUT;

    //
    // Get the current set request packet to work on.
    //

    keyboardPacket = deviceExtension->KeyboardExtension.CurrentOutput;

    if (deviceExtension->KeyboardExtension.CurrentOutput.State
        == SendFirstByte){

        I8xPrint((
            0,
            "I8042PRT-I8xKeyboardInitiateIoForOasys: send first byte 0x%x\n",
             keyboardPacket.FirstByte
            ));

        //
        // Send the first byte of a 3-byte command sequence to the
        // keyboard controller, asynchronously.
        //

        I8xPutByteAsynchronous(
             (CCHAR) DataPort,
             deviceExtension,
             keyboardPacket.FirstByte
             );

    } else if (deviceExtension->KeyboardExtension.CurrentOutput.State
        == SendFinalByte) {

        //
        // Send the second byte of a 3-byte command sequence to the
        // keyboard controller, asynchronously.
        //

        I8xPutByteAsynchronous(
             (CCHAR) DataPort,
             deviceExtension,
             (UCHAR)0x8c
             );

    } else if(deviceExtension->KeyboardExtension.CurrentOutput.State
        == SendLastByte) {

        I8xPrint((
            0,
            "I8042PRT-I8xKeyboardInitiateIoForOasys: send last byte 0x%x\n",
             keyboardPacket.LastByte
            ));

        //
        // Send the last byte of a command sequence to the keyboard
        // controller, asynchronously.
        //

        I8xPutByteAsynchronous(
             (CCHAR) DataPort,
             deviceExtension,
             keyboardPacket.LastByte
             );

    } else { 

        I8xPrint((0, "I8042PRT-I8xKeyboardInitiateIoForOasys: INVALID REQUEST\n"));

        //
        // Queue a DPC to log an internal driver error.
        //

        KeInsertQueueDpc(
            &deviceExtension->ErrorLogDpc,
            (PIRP) NULL,
            (PVOID) (ULONG) I8042_INVALID_INITIATE_STATE);

        ASSERT(FALSE);
    }

    I8xPrint((2, "I8042PRT-I8xKeyboardInitiateIoForOasys: exit\n"));

    return;
}

VOID
I8xKeyboardInitiateWrapperForOasys(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called from StartIo synchronously.  It sets up the
    CurrentOutput and ResendCount fields in the device extension, and
    then calls I8xKeyboardInitiateIo to do the real work.

Arguments:

    Context - Pointer to the context structure containing the first and
        last bytes of the send sequence.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;

    //
    // Get a pointer to the device object from the context argument.
    //

    deviceObject = ((PKEYBOARD_INITIATE_CONTEXT) Context)->DeviceObject;

    //
    // Set up CurrentOutput state for this operation.
    //

    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;

    deviceExtension->KeyboardExtension.CurrentOutput.State = SendFirstByte;
    deviceExtension->KeyboardExtension.CurrentOutput.FirstByte =
        ((PKEYBOARD_INITIATE_CONTEXT) Context)->FirstByte;
    deviceExtension->KeyboardExtension.CurrentOutput.LastByte =
        ((PKEYBOARD_INITIATE_CONTEXT) Context)->LastByte;

    //
    // We're starting a new operation, so reset the resend count.
    //

    deviceExtension->KeyboardExtension.ResendCount = 0;

    //
    // Initiate the keyboard I/O operation.  Note that we were called
    // using KeSynchronizeExecution, so I8xKeyboardInitiateIo is also
    // synchronized with the keyboard ISR.
    //
    I8xKeyboardInitiateIoForOasys((PVOID) deviceObject);
}

ULONG
I8042ConversionStatusForOasys(
    IN ULONG fOpen,
    IN ULONG ConvStatus)

/*++

Routine Description:

    This routine convert ime open/close status and ime converion mode to
    FMV oyayubi-shift keyboard device internal input mode.

Arguments:


Return Value:

    FMV oyayubi-shift keyboard's internal input mode.

--*/
{
    ULONG ImeMode = 0;

    if (fOpen) {
        if (ConvStatus & IME_CMODE_ROMAN) {
            if (ConvStatus & IME_CMODE_ALPHANUMERIC) {
                //
                // Alphanumeric, roman mode.
                //
                ImeMode = FMV_ROMAN_ALPHA_CAPSON;
            } else if (ConvStatus & IME_CMODE_KATAKANA) {
                //
                // Katakana, roman mode.
                //
                ImeMode = FMV_ROMAN_KATAKANA;
            } else if (ConvStatus & IME_CMODE_NATIVE) {
                //
                // Hiragana, roman mode.
                //
                ImeMode = FMV_ROMAN_HIRAGANA;
            } else {
                ImeMode = FMV_ROMAN_ALPHA_CAPSON;
            }
        } else {
            if (ConvStatus & IME_CMODE_ALPHANUMERIC) {
                //
                // Alphanumeric, no-roman mode.
                //
                ImeMode = FMV_NOROMAN_ALPHA_CAPSON;
            } else if (ConvStatus & IME_CMODE_KATAKANA) {
                //
                // Katakana, no-roman mode.
                //
                ImeMode = FMV_NOROMAN_KATAKANA;
            } else if (ConvStatus & IME_CMODE_NATIVE) {
                //
                // Hiragana, no-roman mode.
                //
                ImeMode = FMV_NOROMAN_HIRAGANA;
            } else {
                ImeMode = FMV_NOROMAN_ALPHA_CAPSON;
            }
        }
    } else {
        //
        // Ime close. In this case, internal mode is always this value.
        // (the both LED off roman and kana)
        //
        ImeMode = FMV_NOROMAN_ALPHA_CAPSON;
    }

    return ImeMode;
}

ULONG
I8042QueryIMEStatusForOasys(
    IN PKEYBOARD_IME_STATUS KeyboardIMEStatus
    )
{
    ULONG InternalMode;

    //
    // Map to IME mode to hardware mode.
    //
    InternalMode = I8042ConversionStatusForOasys(
                KeyboardIMEStatus->ImeOpen,
                KeyboardIMEStatus->ImeConvMode
                );

    return InternalMode;
}

NTSTATUS
I8042SetIMEStatusForOasys(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension;
    KEYBOARD_INITIATE_CONTEXT keyboardInitiateContext;
    PKEYBOARD_IME_STATUS KeyboardIMEStatus;
    ULONG InternalMode;
    LARGE_INTEGER deltaTime;

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Get pointer to KEYBOARD_IME_STATUS buffer.
    //
    KeyboardIMEStatus = (PKEYBOARD_IME_STATUS)(Irp->AssociatedIrp.SystemBuffer);

    //
    // Map IME mode to keyboard hardware mode.
    //
    InternalMode = I8042QueryIMEStatusForOasys(KeyboardIMEStatus);

    //
    // Set up the context structure for the InitiateIo wrapper.
    //
    keyboardInitiateContext.DeviceObject = DeviceObject;
    keyboardInitiateContext.FirstByte = 0xF0;
    keyboardInitiateContext.LastByte  = (UCHAR)InternalMode;

    //
    // Call the InitiateIo wrapper synchronously.  The wrapper
    // stores the context parameters in the device extension,
    // and then initiates the I/O operation, all synchronized
    // with the keyboard ISR.
    //
    KeSynchronizeExecution(
        deviceExtension->KeyboardInterruptObject,
        (PKSYNCHRONIZE_ROUTINE) I8xKeyboardInitiateWrapperForOasys,
        (PVOID) &keyboardInitiateContext
        );

    //
    // Start the 1-second command timer. InitiateIo changed
    // the TimerCount already.
    //
    deltaTime.LowPart = (ULONG)(-10 * 1000 * 1000);
    deltaTime.HighPart = -1;

    (VOID)KeSetTimer(
               &deviceExtension->CommandTimer,
               deltaTime,
               &deviceExtension->TimeOutDpc
               );

    return (STATUS_SUCCESS);
}
