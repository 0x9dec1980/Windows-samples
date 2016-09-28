/*++

Copyright (c) 1991, 1992  Microsoft Corporation
Copyright (c) 1992  Future Domain Corporation

Module Name:

    fd8xx.c

Abstract:

    This is the miniport driver for the Future Domain TMC-8XX SCSI Adapters.

Environment:

    kernel mode only

Notes:

--*/

#include "miniport.h"
#include "stdarg.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "fd8xx.h"      // includes scsi.h

//
// Logical Unit states.
//
typedef enum _LU_STATE {

    LS_UNDETERMINED,
    LS_ARBITRATE,
    LS_SELECT,
    LS_IDENTIFY,
    LS_MSG_SPECIAL,
    LS_COMMAND,
    LS_DATA,
    LS_DISCONNECTED,
    LS_ABORT,
    LS_STATUS,
    LS_MSG_IN,
    LS_COMPLETE

} LU_STATE, *PLU_STATE;

//
// Logical Unit extension.  The BOOLEANS in this structure will be initialized
// to FALSE by the port driver.  It will zero the structure when allocated.
// Therefore there is no explicit initialization of these fields.
//
typedef struct _SPECIFIC_LU_EXTENSION {

    LU_STATE            LuState;            // State information.
    ULONG               SavedDataPointer;   // Current data pointer.
    ULONG               SavedDataLength;    // Current data lenght.
    PSCSI_REQUEST_BLOCK ActiveLuRequest;    // Active Srb for this LUN.
    USHORT              OverRunCount;       // Initialized to zero, counts up
    UCHAR               AbortBeingAttempted;// Abort active on this LU.
    BOOLEAN             NoDisconnectActive; // No disconnect flag on SRB.
    BOOLEAN             HandShakeAllData;   // Dribbling drive fix.
    BOOLEAN             SixByteCDBActive;   // Potential tape access.

} SPECIFIC_LU_EXTENSION, *PSPECIFIC_LU_EXTENSION;

//
// Device extension
//
typedef struct _SPECIFIC_DEVICE_EXTENSION {

    PUCHAR              BaseAddress;        // Memory map address of adapter.
    ULONG               CurDataPointer;     // Current pointer for active LUN.
    ULONG               CurDataLength;      // Bytes left to xfer to this LUN.
    PSPECIFIC_LU_EXTENSION  SavedLu;        // Saved LUN during interrupts.
    PSPECIFIC_LU_EXTENSION  ActiveLu;       // Currently active LUN.
    UCHAR               PathId;             // Relates to SCSI bus.
    UCHAR               ControlRegister;    // Current val of fd8xx control reg.
    BOOLEAN             ExpectingInterrupt; // Bookkeeping for IRQ configuration.
    BOOLEAN             NotifiedConfigurationError; // Event is logged.
    USHORT              ContinueTimer;      // Indicator to continue polling
    USHORT              TimerCaughtInterrupt; // Number times timer caught IRQ
    UCHAR               InitiatorId;        // target id for host adapter
    BOOLEAN             ConfiguredWithoutInterrupts; // Intentially configured w/o IRQ
    BOOLEAN             SelectingTarget;

} SPECIFIC_DEVICE_EXTENSION, *PSPECIFIC_DEVICE_EXTENSION;


//
// Function declarations
//

ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    );

ULONG
Fd8xxFindAdapter(
    IN PVOID                                Context,
    IN PVOID                                AdaptersFound,
    IN PVOID                                BusInformation,
    IN PCHAR                                ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION  ConfigInfo,
    OUT PBOOLEAN                            Again
    );

BOOLEAN
Fd8xxInitialize(
    IN PVOID Context
    );

BOOLEAN
Fd8xxStartIo(
    IN PVOID               Context,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
Fd8xxDoReconnect(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );

VOID
Fd8xxDpcRunPhase(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );

VOID
Fd8xxTimer(
    IN PVOID Context
    );

BOOLEAN
Fd8xxInterrupt(
    IN PVOID Context
    );

BOOLEAN
Fd8xxResetBus(
    IN PVOID Context,
    IN ULONG PathId
    );

VOID
Fd8xxMoveMemoryUchar(
    PUCHAR Source,
    PUCHAR Dest,
    ULONG  Length
    );


#if DBG

//
// Globals and externals used for debugging.
//

//
// Fd8xxDebug affects which debug prints are enabled:
//       old  new
//            0x001 Aborts,Timeouts,Error conditions
//      0x001       Arbitration and selection
//      0x002       Command, message out, and status
//      0x004       Data transfer
//      0x008       Message in and status
//      0x010       Miniport entry points and completion
//      0x020       Initialization and interrupt
//      0x040       StatusCheck and WaitForRequest
//      0x080       Control register manipulation
//      0x100       Completion
//      0x200       Handshake all bytes set.
//
ULONG Fd8xxDebug = 0x0000;

#define FdDebugPrint(MASK, ARGS)    \
	if (MASK & Fd8xxDebug) {    \
	    ScsiDebugPrint ARGS;    \
	}

#else

#define FdDebugPrint(MASK, ARGS)

#endif


VOID
Fd8xxWriteControl(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR                      Value
    )

/*++

Routine Description:

    This routine sets the control register on the adapter and remembers
    the value set in the device extension.

Arguments:

    DeviceExtension - Device adapter context pointer.
    Value           - New value for the control register.

Return Value:

    None

--*/

{
    FdDebugPrint(0x04,
		 (0, "WriteControl: Device = %x, NewVal = %x\n",
		 DeviceExtension,
		 Value));

    DeviceExtension->ControlRegister = Value;

    FD8XX_SET_CONTROL(DeviceExtension->BaseAddress,
		      Value);
} // end Fd8xxWriteControl()


VOID
Fd8xxSetControl(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR                      Value
    )

/*++

Routine Description:

    This routine adds the control lines indicated to the current value
    for the adapter control register.

Arguments:

    DeviceExtension - Device adapter context pointer.
    Value           - The bits that are to be added (or'd in) to the
		      control register.

Return Value:

    None

--*/

{

    DeviceExtension->ControlRegister |= Value;

    FD8XX_SET_CONTROL(DeviceExtension->BaseAddress,
		      DeviceExtension->ControlRegister);
} // end Fd8xxSetControl()


VOID
Fd8xxClearControl(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR                      Mask
    )

/*++

Routine Description:

    This routine masks out the bits for the control register that are
    passed in Mask and sets the adapter control register to the new value.

Arguments:

    DeviceExtension - Device adapter context pointer.
    Mask            - The bits that are to be reset to zero in the control
		      register.

Return Value:

    None

--*/

{
    //
    // Negate Value to get a zero based mask.
    //
    Mask = ~(Mask);

    //
    // AND against the current contents of the control register.
    //
    DeviceExtension->ControlRegister &= Mask;

    FD8XX_SET_CONTROL(DeviceExtension->BaseAddress,
		      DeviceExtension->ControlRegister);
} // end Fd8xxClearControl()


BOOLEAN
Fd8xxWaitForRequestLine(
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Spin checking the status of the FD8XX adapter until it indicates that
    the SCSI REQUEST line is high.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    TRUE    - indicates that the SCSI REQUEST line was asserted in time.
    FALSE   - indicates timeout occurred while waiting for the SCSI REQUEST
	      line.

--*/

{
    PUCHAR  baseAddress = DeviceExtension->BaseAddress;
    ULONG   spinCount   = REQUEST_SPIN_WAIT;

    do {

	//
	// Check if TWO of the status register reads are identical.  This is
	// to validate that there was no "glitch" that caused some line to
	// temporarily go high.
	//
	while (FD8XX_READ_STATUS(baseAddress) !=
	       FD8XX_READ_ALTERNATE_STATUS(baseAddress, 1)) {

	    ScsiPortStallExecution(1);
	}

	if (FD8XX_READ_STATUS(baseAddress) & S_REQUEST) {

	    FdDebugPrint(0x04, (0, "Fd8xxWaitForRequestLine: REQUEST Line Asserted\n"));
	    return TRUE;
	}

	ScsiPortStallExecution(1);

    } while (spinCount--);

    FdDebugPrint(0x01, (0, "Fd8xxWaitForRequestLine: REQUEST Line Assertion TIMEOUT\n"));

    if (DeviceExtension->ActiveLu) {

	ScsiPortLogError(DeviceExtension,
			 DeviceExtension->ActiveLu->ActiveLuRequest,
			 DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
			 SP_REQUEST_TIMEOUT,
			 1);
    }

    return FALSE;
} // end Fd8xxWaitForRequestLine()


BOOLEAN
Fd8xxStatusCheck(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR Mask,
    IN UCHAR Compare,
    IN ULONG SpinMax
    )

/*++

Routine Description:

    Spin checking the status of the FD8XX adapter until it matches
    the desired value.

Arguments:

    DeviceExtension - Device adapter context pointer.
    Mask            - Bits to mask out of the value returned by adapter
		      status register.
    Compare         - Desired result of status register after is is "Mask"ed.
    SpinMax         - Number of times to test adapter's status register.

Return Value:

    TRUE indicates desired value was returned by the adapter status register.
    FALSE indicates timeout occurred and desired value was not returned.

--*/

{
    PUCHAR  baseAddress = DeviceExtension->BaseAddress;
    ULONG   retryCount  = 750000;

    do {

	//
	// Check if TWO of the status register reads are identical.  This is
	// to validate that there was no "glitch" that caused some line to
	// temporarily go high.
	//
	while (FD8XX_READ_STATUS(baseAddress) !=
	       FD8XX_READ_ALTERNATE_STATUS(baseAddress, 1)) {

	    ScsiPortStallExecution(1);
        if (--retryCount == 0) {
            FdDebugPrint(0x01,(0,"Fd8xxStatusCheck: Timed out waiting for equal status registers\n"));
            return FALSE;
        }
	}

	if ((UCHAR)(FD8XX_READ_STATUS(baseAddress) & Mask) == Compare) {

	    FdDebugPrint(0x04, (0, "Fd8xxStatusCheck: Status Registers Compared.\n"));
	    return TRUE;
	}

	ScsiPortStallExecution(1);

    } while (SpinMax--);

    FdDebugPrint(0x01,
		  (0, "Fd8xxStatusCheck: TIMEOUT Status=%x\n",
		  FD8XX_READ_STATUS(baseAddress)));

    return FALSE;
} // end Fd8xxStatusCheck()


VOID
Fd8xxDetermineNextState(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine determines the next logical state of this state machine
    based on the current SCSI bus phase.  It is typically called because
    from the current state, the next state can be one of a number of other
    states.  For example, after issuing the last command byte of COMMAND
    phase, the device may want to drive us into DATA_IN, DATA_OUT, STATUS,
    or MSG_IN (i.e., transfer data, report an error, or disconnect).

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{
    FdDebugPrint(0x100,
		  (0, "Fd8xxDetermineNextState: phase = %x ",
		  FD8XX_READ_PHASE(DeviceExtension->BaseAddress)));

    switch (FD8XX_READ_PHASE(DeviceExtension->BaseAddress)) {

    case BP_COMMAND:

	FdDebugPrint(0x100, (0, "LS_COMMAND\n"));
	DeviceExtension->ActiveLu->LuState = LS_COMMAND;
	break;

    case BP_DATA_IN:
    case BP_DATA_OUT:

	FdDebugPrint(0x100, (0, "LS_DATA\n"));
	DeviceExtension->ActiveLu->LuState = LS_DATA;
	break;

    case BP_MESSAGE_IN:

	FdDebugPrint(0x100, (0, "LS_MSG_IN\n"));
	DeviceExtension->ActiveLu->LuState = LS_MSG_IN;
	break;

    case BP_STATUS:

	FdDebugPrint(0x100, (0, "LS_STATUS\n"));
	DeviceExtension->ActiveLu->LuState = LS_STATUS;
	break;

    case BP_BUS_FREE:

	FdDebugPrint(0x100, (0, "LS_BUS_FREE\n"));

	if (DeviceExtension->ActiveLu->AbortBeingAttempted) {

	    DeviceExtension->ActiveLu->ActiveLuRequest->SrbStatus =
					SRB_STATUS_SUCCESS;
	} else {

	    DeviceExtension->ActiveLu->ActiveLuRequest->SrbStatus =
					SRB_STATUS_UNEXPECTED_BUS_FREE;
	}

	DeviceExtension->ActiveLu->LuState = LS_COMPLETE;
	break;

    case BP_MESSAGE_OUT:
    case BP_RESELECT:
    default:

	//
	// This will get handled in RunPhase.
	//
	FdDebugPrint(0x100, (0, "N/A\n"));
	break;
    }
    FdDebugPrint(0x100, (0, "Fd8xxDetermineNextState: End.\n"));
} // end Fd8xxDetermineNextState()


VOID
Fd8xxArbitrate(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Attempt to arbitrate for the SCSI bus.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{
    PUCHAR  baseAddress = DeviceExtension->BaseAddress;
    ULONG   attemptsRemaining = MAX_ARB_ATTEMPTS;

    while (DeviceExtension->ActiveLu->LuState == LS_ARBITRATE) {

	    if (FD8XX_READ_PHASE(baseAddress) == BP_BUS_FREE) {

            //
            // Wait BusFree delay;
            //

            ScsiPortStallExecution(4);

	        //
	        // Tell the adapter to initiate arbitration for the SCSI bus
	        // since it ia available.
	        //
	        Fd8xxClearControl(DeviceExtension,
	    		      C_BUS_ENABLE);
	        Fd8xxSetControl(DeviceExtension,
	    		    C_PARITY_ENABLE);
	        FD8XX_WRITE_DATA(baseAddress,
	    		    (1 << DeviceExtension->InitiatorId));
	        Fd8xxSetControl(DeviceExtension,
	    		    C_ARBITRATION);
	
	        if (Fd8xxStatusCheck(DeviceExtension,
	        		     S_ARB_COMPLETE,
	        		     S_ARB_COMPLETE,
	        		     ARBITRATION_DELAY)) {

	            //
	            // Disable adapter interrupts so selection doesn't cause an
	            // interrupt.  Then go to selection phase.
	            //
	            DeviceExtension->ActiveLu->LuState = LS_SELECT;
                return;

	        } else {

	            FdDebugPrint(0x2,
	    		      (0, "Fd8xxArbitrate: Arbitration FAILED! Status=%x\n",
	    		      FD8XX_READ_STATUS(baseAddress)));

	            Fd8xxClearControl(DeviceExtension,
	    		          C_ARBITRATION);

            }

        } else {

            Fd8xxClearControl(DeviceExtension,
    		          C_ARBITRATION);

            ScsiPortStallExecution(5);

	        if (FD8XX_READ_PHASE(baseAddress) & S_SELECT) {

	    	    FdDebugPrint(0x100, (0, "Fd8xxArbitrate: Being re-selected!\n"));

	    	    //
	    	    // This will cause the interrupt routine to be called TWICE
	    	    // for the same reason.  The first call will be from here.
	    	    // The second call will be from the NT OS since it will
	    	    // remember to call the interrupt routine due to the
	    	    // adapter IRQ line (which is tied to the SCSI SELECT line)
	    	    // going high.  The second call will be seen by the interrupt
	    	    // routine as SPURRIOUS!
	    	    //
	    	    DeviceExtension->SavedLu = DeviceExtension->ActiveLu;
	    	    DeviceExtension->ActiveLu = NULL;

	    	    Fd8xxDoReconnect(DeviceExtension);

	    	    if (DeviceExtension->SavedLu == NULL) {

	    	        //
	    	        // The bus was reset during the reconnect.
	    	        //
	    	        return;
	    	    }

	    	    DeviceExtension->ActiveLu = DeviceExtension->SavedLu;
	    	    DeviceExtension->SavedLu = NULL;

	    	    FdDebugPrint(0x100, (0, "Fd8xxArbitrate: Back to Arbitration.\n"));

	        } else if (!(attemptsRemaining--)) {

	    	    FdDebugPrint(0x01, (0, "Fd8xxArbitrate: Arbitration aborted. SrbStatus = %x\n",
                                     SRB_STATUS_TIMEOUT));


	    	    DeviceExtension->ActiveLu->LuState = LS_COMPLETE;
	    	    DeviceExtension->ActiveLu->ActiveLuRequest->SrbStatus =
	    	    					SRB_STATUS_TIMEOUT;
	        }
	    }
    }
} // end Fd8xxArbitrate()


VOID
Fd8xxSelect(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Perform selection process on SCSI bus.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{

    //
    // Raise Select line, keeping the BUSY line high.  Also, enable parity
    // and tell the adapter to drive the SCSI data lines to prepare for
    // outgoing ID bits.  Keep in mind that this also clears the
    // C_ARBITRATION bit.
    //
    DeviceExtension->SelectingTarget = TRUE;
    Fd8xxWriteControl(DeviceExtension,
		      (C_SELECT | C_BUSY | C_BUS_ENABLE | C_INT_ENABLE));
    Fd8xxSetControl(DeviceExtension,
		    C_PARITY_ENABLE);

    ScsiPortStallExecution(2);  // Delay 1200 nanoseconds (Bus-Settle + Bus-Clear).

    FD8XX_WRITE_DATA(DeviceExtension->BaseAddress,
		     (1 << DeviceExtension->InitiatorId) |
		     (1 << (DeviceExtension->ActiveLu->ActiveLuRequest->TargetId)));

    //
    // Raise attention to get to message out phase.
    //
    Fd8xxSetControl(DeviceExtension,
		    C_ATTENTION);

    ScsiPortStallExecution(1);  // Delay 90 nanoseconds (2 * Bus-Deskew).

    //
    // Clear BUSY
    //
    Fd8xxClearControl(DeviceExtension,
		      C_BUSY);

    ScsiPortStallExecution(1);  // Delay 400 nanoseconds (Bus-Settle).

    if (Fd8xxStatusCheck(DeviceExtension,
			 S_BUSY,
			 S_BUSY,
			 SELECTION_DELAY)) {

	ScsiPortStallExecution(1);  // Delay 90 nanoseconds (2 * Bus-Deskew).

	//
	// Selection is Ok.  Clear SELECT line.
	//
	Fd8xxClearControl(DeviceExtension,
			  C_SELECT);

	DeviceExtension->ActiveLu->LuState = LS_IDENTIFY;
	DeviceExtension->SelectingTarget = FALSE;
	return;
    }

    //
    // Selection failed.  Force SCSI bus back to Bus-Free.
    //
    DeviceExtension->SelectingTarget = FALSE;
    Fd8xxWriteControl(DeviceExtension,
		      (C_INT_ENABLE | C_PARITY_ENABLE));

    FdDebugPrint(0x01,
		  (0, "Fd8xxSelect: SELECT FAILED %x\n",
		  DeviceExtension->ActiveLu->ActiveLuRequest->TargetId));

    DeviceExtension->ActiveLu->LuState = LS_COMPLETE;
    DeviceExtension->ActiveLu->ActiveLuRequest->SrbStatus =
						SRB_STATUS_SELECTION_TIMEOUT;
} // end Fd8xxSelect()


VOID
Fd8xxSendIdentify(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Send an identify message on the SCSI bus if the target will accept it.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{
    PUCHAR              baseAddress = DeviceExtension->BaseAddress;
    PSCSI_REQUEST_BLOCK srb = DeviceExtension->ActiveLu->ActiveLuRequest;

    if (Fd8xxWaitForRequestLine(DeviceExtension) == FALSE) {

        FdDebugPrint(0x01,
        	      (0, "Fd8xxSendIdentify: IDENTIFY not sent %x\n",
        	       FD8XX_READ_STATUS(DeviceExtension->BaseAddress)));
	ScsiPortLogError(DeviceExtension,
			 DeviceExtension->ActiveLu->ActiveLuRequest,
			 DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
			 SP_REQUEST_TIMEOUT,
			 12);
	DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;
	return;
    }

    if (srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {

	Fd8xxWriteControl(DeviceExtension,
			  (C_INT_ENABLE | C_PARITY_ENABLE | C_BUS_ENABLE));

	ScsiPortStallExecution(1);  // Delay 90 nanoseconds (2 * Bus-Deskew).

	//
	// The target may wish to negotiate synchronous with us, in which
	// case we will reject the message and go on to COMMAND phase.
	//
	DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;

    } else {

	//
	// We still have an ABORT or RESET message to send, so keep the
	// ATTENTION line high.
	//
	Fd8xxWriteControl(DeviceExtension,
			  (C_PARITY_ENABLE | C_BUS_ENABLE |
			   C_ATTENTION | C_INT_ENABLE));

	DeviceExtension->ActiveLu->LuState = LS_MSG_SPECIAL;
    }

    //
    // Some old SCSI-I devices want to skip MESSAGE OUT phase and go
    // directly to COMMAND phase.  We'll just let them do what they want.
    // If the drive is really stupid, the error should get picked up in
    // the phase to which it transitions, so just return and get on with
    // life.
    //
    if (FD8XX_READ_PHASE(baseAddress) == BP_MESSAGE_OUT) {

	if (srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT) {

	    DeviceExtension->ActiveLu->NoDisconnectActive = TRUE;
	    FD8XX_WRITE_DATA(DeviceExtension->BaseAddress,
			     (SCSIMESS_IDENTIFY |
			     srb->Lun));
	} else {

	    FD8XX_WRITE_DATA(DeviceExtension->BaseAddress,
			     (SCSIMESS_IDENTIFY_WITH_DISCON |
			     srb->Lun));
	}

    } else {

	FdDebugPrint(0x01,
		      (0, "Fd8xxSendIdentify: IDENTIFY not sent %x\n",
		      FD8XX_READ_STATUS(DeviceExtension->BaseAddress)));

	Fd8xxClearControl(DeviceExtension, C_ATTENTION);
	ScsiPortStallExecution(1);  // Delay 90 nanoseconds (2 * Bus-Deskew).

	DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;
    }
} // end Fd8xxSendIdentify()


VOID
Fd8xxSendSpecialMessage(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Send an ABORT or RESET message on the SCSI bus if the target will accept
    it.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{
    PUCHAR              baseAddress = DeviceExtension->BaseAddress;
    PSCSI_REQUEST_BLOCK srb = DeviceExtension->ActiveLu->ActiveLuRequest;

    if (Fd8xxWaitForRequestLine(DeviceExtension) == FALSE) {

    	FdDebugPrint(0x01,
    		      (0, "Fd8xxSendSpecialMessage: Abort/Reset Message not sent %x\n"));
	ScsiPortLogError(DeviceExtension,
			 DeviceExtension->ActiveLu->ActiveLuRequest,
			 DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
			 SP_REQUEST_TIMEOUT,
			 13);
	DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;
	return;
    }

    Fd8xxClearControl(DeviceExtension,
		      C_ATTENTION);
    ScsiPortStallExecution(1);  // Delay 90 nanoseconds (2 * Bus-Deskew).

    if (srb->Function == SRB_FUNCTION_ABORT_COMMAND) {
        FdDebugPrint(0x01,
                    (0, "Fd8xxSendSpecialMessage: Abort Command Received.\n"));

	FD8XX_WRITE_DATA(DeviceExtension->BaseAddress,
			 SCSIMESS_ABORT);
    } else if (srb->Function == SRB_FUNCTION_RESET_DEVICE) {

        FdDebugPrint(0x01,
                    (0, "Fd8xxSendSpecialMessage: Reset Command Received.\n"));

	FD8XX_WRITE_DATA(DeviceExtension->BaseAddress,
			 SCSIMESS_BUS_DEVICE_RESET);
    } else {

	ScsiPortLogError(DeviceExtension,
			 srb,
			 srb->PathId,
			 srb->TargetId,
			 srb->Lun,
			 SP_PROTOCOL_ERROR,
			 2);

	FD8XX_WRITE_DATA(DeviceExtension->BaseAddress,
			 SCSIMESS_NO_OPERATION);
    }

    DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;
} // end Fd8xxSendSpecialMessage()


VOID
Fd8xxSendCDB(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Send the SCSI Command Descriptor Block (CDB) to the indicated target/lun.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{
    UCHAR   cdbLength = DeviceExtension->ActiveLu->ActiveLuRequest->CdbLength;
    PUCHAR  cdb = DeviceExtension->ActiveLu->ActiveLuRequest->Cdb;
    PUCHAR  baseAddress = DeviceExtension->BaseAddress;


    Fd8xxSetControl(DeviceExtension, C_BUS_ENABLE);

    while ((cdbLength-- != 0) &&
	   Fd8xxWaitForRequestLine(DeviceExtension) &&
	   (FD8XX_READ_PHASE(baseAddress) == BP_COMMAND)) {

        FdDebugPrint(0x80, (0, "Fd8xxSendCdb: Sending Command: %x.\n",
                     *cdb));

	FD8XX_WRITE_DATA(baseAddress,
			 *cdb++);
    }

    DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;

    //
    // Set up the running data pointer info for a possible data transfer.
    //
    DeviceExtension->CurDataPointer = DeviceExtension->ActiveLu->SavedDataPointer;
    DeviceExtension->CurDataLength = DeviceExtension->ActiveLu->SavedDataLength;

    FdDebugPrint(0x80, (0, "Fd8xxSendCdb: New phase=%x\n", FD8XX_READ_PHASE(baseAddress)));
} // end Fd8xxSendCDB()


VOID
Fd8xxCopyData(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Copy data byte-for-byte to and from adapter.  This is used to copy data
    from the FD8XX controller to system memory and vice-versa.  Each
    iteration of the outer-most "while" loop will transfer MAX_BUFFER_LENGTH
    bytes (i.e., 512 bytes - this is the size of the TMC-950 memory-mapped
    SCSI data register), or any fraction thereof.

    In theory, one should be able to use the x86 REP MOVB instruction (or
    other equivalent) to "blast" 512 bytes of data to this register.  The
    REQ/ACK handshaking is done by the TMC-950 chip.  In practice, there is
    one problem.  The SCSI data register is "locked in" with the host bus
    (ISA).  This host bus cannot be tied up for longer than a memory refresh
    cycle (15 useconds, I think).  After completing a REQ/ACK cycle, the
    950 chip frees the bus, then locks it again waiting for the assertion
    SCSI REQ line by the target.  If REQ is not asserted within a memory
    refresh cycle, the 950 chip times out and frees the host bus so that a
    memory refresh can take place.  This puts the 950 chip one byte "ahead"
    of the target, thus screwing up the rest of the block transfer.

    Future Domain refers to this problem as the "dribbling drive" because
    the target delays the assertion of the SCSI REQ line for longer than
    usual (probably because the firmware did some maintenance task between
    bytes like filling or draining its internal FIFO).  SCSI places no
    limit on how long a target has to re-assert the SCSI REQ line, so we must
    provide a work-around.

    After a fair amount of research and experimentation, we have learned that
    "manually" handshaking the first x number of bytes, the last x number of
    bytes, or a combination of the two, solves the problem.  The determination
    of x in either case is purely trial-and-error and is based on device
    characteristics.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{
    PUCHAR  baseAddress = DeviceExtension->BaseAddress;
    UCHAR   byteBucket = 0;
    UCHAR   wantedState;

    register ULONG   moveCount;
    register ULONG   headCount;
    register ULONG   tailCount;

    if (Fd8xxWaitForRequestLine(DeviceExtension) == FALSE) {

    	FdDebugPrint(0x01, (0, "Fd8xxCopyData: Timed out waiting for Request Line.\n"));
	ScsiPortLogError(DeviceExtension,
			 DeviceExtension->ActiveLu->ActiveLuRequest,
			 DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
			 SP_REQUEST_TIMEOUT,
			 14);
	DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;
	return;
    }

    wantedState = FD8XX_READ_PHASE(baseAddress);

    if (wantedState == BP_DATA_IN) {

	//
	// The target will drive the SCSI data lines in DATA_IN phase.
	//
	Fd8xxClearControl(DeviceExtension, C_BUS_ENABLE);
    } else {

	//
	// We will drive the SCSI data lines in DATA_OUT phase.
	//
	Fd8xxSetControl(DeviceExtension, C_BUS_ENABLE);
    }

    while (FD8XX_READ_PHASE(baseAddress) == wantedState) {

	if (DeviceExtension->CurDataLength == 0) {

	    //
	    // Overrun condition.  Read (or write) bytes until
	    // out of DATA phase.
	    //

	    FdDebugPrint(0x01, (0, "Fd8xxCopyData:overrun condition.\n "));

	    while ((FD8XX_READ_PHASE(baseAddress) == wantedState) &&
		    Fd8xxWaitForRequestLine(DeviceExtension)) {

		if (wantedState == BP_DATA_IN) {

		    //
		    // Throw the byte away since there's nowhere to put it.
		    //
		    byteBucket = FD8XX_READ_DATA(baseAddress);
		} else {

		    //
		    // Write a zero to the device.
		    //
		    FD8XX_WRITE_DATA(baseAddress, byteBucket);
		}
	    }

	    DeviceExtension->ActiveLu->ActiveLuRequest->SrbStatus =
						    SRB_STATUS_DATA_OVERRUN;
	    DeviceExtension->ActiveLu->OverRunCount++;
	    if (DeviceExtension->ActiveLu->OverRunCount >= 5) {
		FdDebugPrint(0x02, (0, "Fd8xxCopyData: Turned on handshake; Too many overruns.\n"));
		DeviceExtension->ActiveLu->HandShakeAllData = TRUE;
	    }
	    continue;
	}

	//
	// First, copy the first bytes (if any) with REQ/ACK handshaking...
	//
	// NOTE:  Some devices are REALLY slow when transferring inquiry
	// data.  So, we'll treat any transfer less than a block
	// as a SLOW transfer.  Also, all transfers performed when
	// the DISABLE_DISCONNECT bit was on in the SRB are done in
	// the slower, handshake per byte way.  Finally some devices
	// tend to "dribble" bytes and need to have a handshake all
	// the time.  These are found by a threshold of overruns being
	// encountered above.  The single OR on the HandShake and SixByte
	// booleans is on purpose.  This generates an or and conditional
	// jump where a double OR generates more code.
	//
	if (((DeviceExtension->ActiveLu->NoDisconnectActive) &&
	     (DeviceExtension->ActiveLu->OverRunCount != 0)) ||
	    (DeviceExtension->ActiveLu->HandShakeAllData |
	     DeviceExtension->ActiveLu->SixByteCDBActive)) {

	    headCount = DeviceExtension->CurDataLength;
	} else {
	    if (MAX_BUFFER_LENGTH > DeviceExtension->CurDataLength) {

		headCount = DeviceExtension->CurDataLength;
	    } else {

		if (DeviceExtension->CurDataLength & (MAX_BUFFER_LENGTH - 1)) {

		    //
		    // If the expected transfer is not a block multiple, do it
		    // all the slow way.  This will correct problems with NEC
		    // CD drives when doing NEC CD audio commands.
		    //
		    headCount = DeviceExtension->CurDataLength;
		} else {
		    headCount = min(MAX_HEAD_LENGTH,
				    DeviceExtension->CurDataLength);
		}
	    }
	}

	moveCount = 0;

	while (moveCount != headCount) {

	    if (Fd8xxWaitForRequestLine(DeviceExtension) == FALSE) {
		ScsiPortLogError(DeviceExtension,
				 DeviceExtension->ActiveLu->ActiveLuRequest,
				 DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
				 DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
				 DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
				 SP_REQUEST_TIMEOUT,
				 11);
		break;
	    }

	    if (FD8XX_READ_PHASE(baseAddress) == BP_DATA_IN) {
																										
		ScsiPortReadRegisterBufferUchar((baseAddress + READ_SCSI),
				      ((PUCHAR) DeviceExtension->CurDataPointer++),
				      1);

	    } else if (FD8XX_READ_PHASE(baseAddress) == BP_DATA_OUT) {

		ScsiPortWriteRegisterBufferUchar((baseAddress + WRITE_SCSI),
				       ((PUCHAR) DeviceExtension->CurDataPointer++),
				       1);
	    } else {

		//
		// Account for the number of bytes moved.
		//
		DeviceExtension->CurDataLength -= moveCount;
		goto Fd8xxCopyData_CheckNextPhase;
	    }
	    moveCount++;
	}

	DeviceExtension->CurDataLength -= moveCount;

	if (DeviceExtension->CurDataLength == 0) {
	    goto Fd8xxCopyData_CheckNextPhase;
	}

	//
	// Now, calculate the the middle chunk for transfer without hanshaking...
	//
	moveCount = min(
	    (DeviceExtension->CurDataLength & (MAX_BUFFER_LENGTH - 1)),
	    MAX_BUFFER_LENGTH);

	if (moveCount >= MAX_TAIL_LENGTH) {

	    moveCount -= MAX_TAIL_LENGTH;
	}

	if (wantedState == BP_DATA_IN) {

	    ScsiPortReadRegisterBufferUchar((baseAddress + READ_SCSI),
				      ((PUCHAR) DeviceExtension->CurDataPointer),
				      moveCount);
	} else {

	    ScsiPortWriteRegisterBufferUchar((baseAddress + WRITE_SCSI),
				       ((PUCHAR) DeviceExtension->CurDataPointer),
				       moveCount);
	}

	DeviceExtension->CurDataPointer += moveCount;
	DeviceExtension->CurDataLength -= moveCount;

	//
	// Finally, copy the last bytes (if any) with REQ/ACK handshaking...
	//
	tailCount = min(MAX_TAIL_LENGTH, DeviceExtension->CurDataLength);
	moveCount = 0;

	while ((moveCount != tailCount) &&
	       (FD8XX_READ_PHASE(baseAddress) != BP_BUS_FREE) &&
	       Fd8xxWaitForRequestLine(DeviceExtension)) {

	    if (FD8XX_READ_PHASE(baseAddress) == BP_DATA_IN) {

		ScsiPortReadRegisterBufferUchar((baseAddress + READ_SCSI),
				      ((PUCHAR) DeviceExtension->CurDataPointer++),
				      1);

	    } else if (FD8XX_READ_PHASE(baseAddress) == BP_DATA_OUT) {

		ScsiPortWriteRegisterBufferUchar((baseAddress + WRITE_SCSI),
				       ((PUCHAR) DeviceExtension->CurDataPointer++),
				       1);

	    } else {

		DeviceExtension->CurDataLength -= moveCount;
		goto Fd8xxCopyData_CheckNextPhase;
	    }
	    moveCount++;
	}

	DeviceExtension->CurDataLength -= moveCount;

	if ((FD8XX_READ_PHASE(baseAddress) == BP_BUS_FREE) ||
	    (Fd8xxWaitForRequestLine(DeviceExtension) == FALSE)) {

	    DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;
	    FdDebugPrint(0x04, (0, "NO REQUEST\n"));
	    return;
	}
    }

Fd8xxCopyData_CheckNextPhase:

    if (FD8XX_READ_PHASE(baseAddress) == BP_MESSAGE_IN) {

	//
	// The device most likely wants to disconnect.
	//
	DeviceExtension->ActiveLu->LuState = LS_MSG_IN;

    } else if (FD8XX_READ_PHASE(baseAddress) == BP_STATUS) {

	if (DeviceExtension->CurDataLength != 0) {

	    //
	    // We have an underrun condition.  Update the count of bytes transferred.
	    //
	    FdDebugPrint(0x01, (0, "Fd8xxCopyData: Underrun condition.Request Length %x, Residual %x.\n",
                            DeviceExtension->ActiveLu->ActiveLuRequest->DataTransferLength,
                            DeviceExtension->CurDataLength));

	    DeviceExtension->ActiveLu->ActiveLuRequest->SrbStatus =
						    SRB_STATUS_DATA_OVERRUN;
	    DeviceExtension->ActiveLu->ActiveLuRequest->DataTransferLength -=
		DeviceExtension->CurDataLength;
	} else if (FD8XX_READ_STATUS(baseAddress) & S_PARITY) {

	    //
	    // We have a parity error.
	    //
	    FdDebugPrint(0x01, (0, "Fd8xxCopyData: Parity Error!\n"));
	    DeviceExtension->ActiveLu->ActiveLuRequest->SrbStatus =
						    SRB_STATUS_PARITY_ERROR;
	    ScsiPortLogError(DeviceExtension,
			     DeviceExtension->ActiveLu->ActiveLuRequest,
			     DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
			     DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
			     DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
			     SP_BUS_PARITY_ERROR,
			     3);
	}

	DeviceExtension->ActiveLu->LuState = LS_STATUS;

    } else {

	DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;
	FdDebugPrint(0x04, (0, "Fd8xxCopyData: OTHER\n"));
	return;
    }
} // end Fd8xxCopyData()


VOID
Fd8xxStatus(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine will obtain the status from the target.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{
    UCHAR               status;
    UCHAR               srbStatus;
    PSCSI_REQUEST_BLOCK srb = DeviceExtension->ActiveLu->ActiveLuRequest;

    Fd8xxClearControl(DeviceExtension, C_BUS_ENABLE);

    if (Fd8xxWaitForRequestLine(DeviceExtension) == FALSE) {

	DeviceExtension->ActiveLu->LuState = LS_UNDETERMINED;
	ScsiPortLogError(DeviceExtension,
			 DeviceExtension->ActiveLu->ActiveLuRequest,
			 DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
			 SP_REQUEST_TIMEOUT,
			 17);
	return;
    }

    status = FD8XX_READ_DATA(DeviceExtension->BaseAddress);

    //
    // Save this away for the driver above.
    //
    srb->ScsiStatus = status;

    FdDebugPrint(0x04, (0, "Fd8xxStatus: Returned status byte=%x\n", status));

    switch (status) {

    case SCSISTAT_GOOD:
    case SCSISTAT_CONDITION_MET:
    case SCSISTAT_INTERMEDIATE:
    case SCSISTAT_INTERMEDIATE_COND_MET:

	srbStatus = SRB_STATUS_SUCCESS;
	break;

    case SCSISTAT_CHECK_CONDITION:
    case SCSISTAT_COMMAND_TERMINATED:

	srbStatus = SRB_STATUS_ERROR;
	break;

    case SCSISTAT_BUSY:
    case SCSISTAT_RESERVATION_CONFLICT:
    case SCSISTAT_QUEUE_FULL:
    default:

	srbStatus = SRB_STATUS_BUSY;
	break;
    }

    //
    // If some error condition already occurred (e.g., parity error), we'll
    // let that one take priority.
    //
    if (srb->SrbStatus == SRB_STATUS_PENDING)
	srb->SrbStatus = srbStatus;

    DeviceExtension->ActiveLu->LuState = LS_MSG_IN;
} // end Fd8xxStatus()


VOID
Fd8xxMessageIn(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine will receive the message from the target.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{
    PSPECIFIC_LU_EXTENSION  luExtension = DeviceExtension->ActiveLu;
    PSCSI_REQUEST_BLOCK     srb = luExtension->ActiveLuRequest;
    PUCHAR                  baseAddress = DeviceExtension->BaseAddress;
    UCHAR                   msg;

    Fd8xxClearControl(DeviceExtension,
		      C_BUS_ENABLE);

    luExtension->LuState = LS_UNDETERMINED;

    if (Fd8xxWaitForRequestLine(DeviceExtension) == FALSE) {

	ScsiPortLogError(DeviceExtension,
			 DeviceExtension->ActiveLu->ActiveLuRequest,
			 DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
			 DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
			 SP_REQUEST_TIMEOUT,
			 15);
	return;
    }

    msg = FD8XX_READ_DATA(baseAddress);

    if (msg == SCSIMESS_DISCONNECT) {

	FdDebugPrint(0x40, (0, "Fd8xxMessageIn: DISCONNECT\n"));

	luExtension->LuState = LS_DISCONNECTED;
	luExtension->SavedDataPointer = DeviceExtension->CurDataPointer;
	luExtension->SavedDataLength = DeviceExtension->CurDataLength;

	DeviceExtension->ActiveLu = NULL;

	if (DeviceExtension->ContinueTimer) {

	    //
	    // Still need the timer to insure interrupts.
	    //
	    DeviceExtension->ExpectingInterrupt = TRUE;
	    ScsiPortNotification(RequestTimerCall,
				 DeviceExtension,
				 Fd8xxTimer,
				 FD8xx_TIMER_VALUE);
	}

    } else if (msg == SCSIMESS_SAVE_DATA_POINTER) {

	FdDebugPrint(0x40, (0, "Fd8xxMessageIn: SAVE POINTERS\n"));

	luExtension->SavedDataPointer = DeviceExtension->CurDataPointer;
	luExtension->SavedDataLength = DeviceExtension->CurDataLength;

    } else if ((msg == SCSIMESS_COMMAND_COMPLETE) ||
	       (msg == SCSIMESS_LINK_CMD_COMP) ||
	       (msg == SCSIMESS_LINK_CMD_COMP_W_FLAG)) {

	FdDebugPrint(0x20, (0, "Fd8xxMessageIn: COMPLETION\n"));

	luExtension->LuState = LS_COMPLETE;

    } else if (msg == SCSIMESS_RESTORE_POINTERS) {

	FdDebugPrint(0x40, (0, "Fd8xxMessageIn: RESTORE POINTERS\n"));

	//
	// The Srb is already correct; just restore the adapter context
	// pointers.
	//
	DeviceExtension->CurDataPointer = luExtension->SavedDataPointer;
	DeviceExtension->CurDataLength = luExtension->SavedDataLength;

    } else if ((msg == SCSIMESS_NO_OPERATION) ||
	       (msg == SCSIMESS_MESSAGE_REJECT)) {

	if (srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {

	    //
	    // We may be talking to a stupid device that cannot handle an
	    // identify message.  If this is the case, there's no telling
	    // what it'll do next.  Just let it take us where it wants and
	    // hope we can recover.
	    //
	    FdDebugPrint(0x40, (0, "Fd8xxMessageIn: Identify "));

	} else {

	    //
	    // We were trying to tell the device to abort or reset and it
	    // puked on the message.  It's probably some old device that
	    // cannot support these messages.  So, just let the device take
	    // us where it wants.  If it's to cammand phase, we'll error this
	    // out since there are no command bytes are associated w/ these
	    // messages.  Hopefully, it'll at least take us back to bus free.
	    //
	    srb->SrbStatus = SRB_STATUS_MESSAGE_REJECTED;
	    FdDebugPrint(0x40, (0, "Fd8xxMessageIn: Abort/Reset.\n"));
	}

	FdDebugPrint(0x40, (0, "Fd8xxMessageIn: REJECT\n"));

    } else if (msg == SCSIMESS_EXTENDED_MESSAGE) {

	UCHAR   xMsgType;
	UCHAR   xMsgLength;

	FdDebugPrint(0x40, (0, "Fd8xxMessageIn: EXTENDED "));

	if (Fd8xxStatusCheck(DeviceExtension,
			     BP_MESSAGE_IN,
			     BP_MESSAGE_IN,
			     REQUEST_SPIN_WAIT)) {

	    xMsgLength = FD8XX_READ_DATA(baseAddress);
	    FdDebugPrint(0x08, (0, "length=%x ", xMsgLength));

	} else {

	    //
	    // This better NEVER happen!  If it does, this device is
	    // BRAINDEAD!!
	    //
	    goto Fd8xxMessageIn_PhaseSequenceFailure;
	}

	if (Fd8xxStatusCheck(DeviceExtension,
			     BP_MESSAGE_IN,
			     BP_MESSAGE_IN,
			     REQUEST_SPIN_WAIT)) {

	    xMsgType = FD8XX_READ_DATA(baseAddress);
	} else {

	    goto Fd8xxMessageIn_PhaseSequenceFailure;
	}

	xMsgLength--;    // Received one of the bytes.

	if (xMsgType == SCSIMESS_MODIFY_DATA_POINTER) {

	    LONG offset = 0;

	    FdDebugPrint(0x40, (0, "Fd8xxMessageIn: MODIFY Data Pointer "));
	    while (xMsgLength-- != 0) {

		if (Fd8xxStatusCheck(DeviceExtension,
				     BP_MESSAGE_IN,
				     BP_MESSAGE_IN,
				     REQUEST_SPIN_WAIT)) {

		    *(((PUCHAR) &(offset)) + (3 - xMsgLength)) =
						FD8XX_READ_DATA(baseAddress);
		} else {

		    goto Fd8xxMessageIn_PhaseSequenceFailure;
		}
	    }

	    //
	    // All message bytes for this message were read.  We can now
	    // go and update our pointers as indicated by the juse-read
	    // message.  Keep in mind that the value of offset is a twos-
	    // compliment value (i.e., negative).
	    //
	    FdDebugPrint(0x40, (0, "offset=%x\n", offset));
	    DeviceExtension->CurDataPointer += offset;
	    DeviceExtension->CurDataLength -= offset;

	} else {

	    goto Fd8xxMessageIn_NotImplemented;
	}

    } else {

Fd8xxMessageIn_NotImplemented:

	FdDebugPrint(0x40, (0, "Fd8xxMessageIn: Message NOT IMPLEMENTED\n"));

	Fd8xxSetControl(DeviceExtension, C_ATTENTION);
	ScsiPortStallExecution(1);  // Delay 90 nanoseconds (2 * Bus-Deskew).

	while (Fd8xxWaitForRequestLine(DeviceExtension) &&
	       (FD8XX_READ_PHASE(baseAddress) == BP_MESSAGE_IN)) {

	    //
	    // Ignore the rest of this message.
	    //
	    msg = FD8XX_READ_DATA(baseAddress);
	}

	if (FD8XX_READ_PHASE(baseAddress) == BP_MESSAGE_OUT) {

	    Fd8xxSetControl(DeviceExtension, C_BUS_ENABLE);
	    Fd8xxClearControl(DeviceExtension, C_ATTENTION);
	    ScsiPortStallExecution(1);  // Delay 90 nanoseconds.
	    FD8XX_WRITE_DATA(baseAddress, SCSIMESS_MESSAGE_REJECT);
	}

	Fd8xxClearControl(DeviceExtension, C_ATTENTION);
    }

    return;

Fd8xxMessageIn_PhaseSequenceFailure:

    srb->SrbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;
    ScsiPortLogError(DeviceExtension,
		     srb,
		     srb->PathId,
		     srb->TargetId,
		     srb->Lun,
		     SP_PROTOCOL_ERROR,
		     4);

    FdDebugPrint(0x01,
		(0,
		"Fd8xxMessageIn: Phase Sequence Error=%x\n",
		FD8XX_READ_DATA(baseAddress)));
    return;

} // end Fd8xxMessageIn()


VOID
Fd8xxNotifyCompletion(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine will perform any clean up operations for the Srb
    and notify the ScsiPort driver of completion.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None

--*/

{
    PSPECIFIC_LU_EXTENSION     luExtension = DeviceExtension->ActiveLu;
    PSCSI_REQUEST_BLOCK        srb = luExtension->ActiveLuRequest;

    if (srb != NULL) {
	if (srb->SrbStatus == SRB_STATUS_PENDING) {
	
	    srb->SrbStatus = SRB_STATUS_ERROR;
	    ScsiPortLogError(DeviceExtension,
		    luExtension->ActiveLuRequest,
		    luExtension->ActiveLuRequest->PathId,
		    luExtension->ActiveLuRequest->TargetId,
		    luExtension->ActiveLuRequest->Lun,
		    SP_INTERNAL_ADAPTER_ERROR,
		    5);
	}
    }

    luExtension->ActiveLuRequest = NULL;
    luExtension->NoDisconnectActive = FALSE;
    DeviceExtension->ActiveLu = NULL;

    //
    // Call notification routine.
    //
    if (srb != NULL) {
	ScsiPortNotification(RequestComplete,
			     (PVOID) DeviceExtension,
			     srb);
    }

    if (!(luExtension->AbortBeingAttempted)) {

	//
	// Adapter ready for next request.
	//
	ScsiPortNotification(NextRequest,
			     DeviceExtension,
			     NULL);
    }

} // end Fd8xxNotifyCompletion()


VOID
Fd8xxRunPhase(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine runs through the bus phases until some type of completion
    indication is received.  This can be when a message informing the host
    that the target will be disconnecting is received or when the SCSI bus
    goes to a free state.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None.

--*/

{
    PUCHAR  baseAddress = DeviceExtension->BaseAddress;
    ULONG   undetermined = 0;

    while (DeviceExtension->ActiveLu != NULL) {

	switch (DeviceExtension->ActiveLu->LuState) {

	case LS_UNDETERMINED:
	    Fd8xxDetermineNextState(DeviceExtension);

	    if (undetermined++ == REQUEST_SPIN_WAIT) {

		//
		// Some CD-ROM drives like to be SCSI bus hogs
		// and hold the SCSI bus in COMMAND phase while feching
		// data (they also conveniently skip message out phase
		// so you can't tell it that you allow disconnect)!!  So,
		// we better not time out if this happens.
		//
		if ((FD8XX_READ_PHASE(baseAddress) | S_REQUEST) !=
							    BP_COMMAND) {
		    FdDebugPrint(0x01, (0, "Fd8xxRunPhase: NO REQUEST\n"));

		    DeviceExtension->ActiveLu->LuState = LS_COMPLETE;
		    DeviceExtension->ActiveLu->ActiveLuRequest->SrbStatus =
					    SRB_STATUS_PHASE_SEQUENCE_FAILURE;

		    ScsiPortLogError(DeviceExtension,
			    DeviceExtension->ActiveLu->ActiveLuRequest,
			    DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
			    DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
			    DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
			    SP_PROTOCOL_ERROR,
			    16);
		}
	    }
	    break;

	case LS_ARBITRATE:
	    Fd8xxArbitrate(DeviceExtension);
	    break;

	case LS_SELECT:
	    Fd8xxSelect(DeviceExtension);
	    break;

	case LS_IDENTIFY:
	    Fd8xxSendIdentify(DeviceExtension);
	    break;

	case LS_MSG_SPECIAL:
	    Fd8xxSendSpecialMessage(DeviceExtension);
	    break;

	case LS_COMMAND:
	    Fd8xxSendCDB(DeviceExtension);
	    break;

	case LS_DATA:
	    Fd8xxCopyData(DeviceExtension);
	    break;

	case LS_STATUS:
	    Fd8xxStatus(DeviceExtension);
	    break;

	case LS_MSG_IN:
	    Fd8xxMessageIn(DeviceExtension);
	    break;

	case LS_COMPLETE:
	    Fd8xxNotifyCompletion(DeviceExtension);
	    break;

	default:

	    DeviceExtension->ActiveLu->LuState = LS_COMPLETE;
	    DeviceExtension->ActiveLu->ActiveLuRequest->SrbStatus =
				    SRB_STATUS_PHASE_SEQUENCE_FAILURE;

        FdDebugPrint(0x01,(0,"Fd8xxRunPhase: Protocol Error. TargetId = %x, Lun = %x\n",
		             DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
		             DeviceExtension->ActiveLu->ActiveLuRequest->Lun
                     ));

	    ScsiPortLogError(DeviceExtension,
		    DeviceExtension->ActiveLu->ActiveLuRequest,
		    DeviceExtension->ActiveLu->ActiveLuRequest->PathId,
		    DeviceExtension->ActiveLu->ActiveLuRequest->TargetId,
		    DeviceExtension->ActiveLu->ActiveLuRequest->Lun,
		    SP_PROTOCOL_ERROR,
		    6);
	    break;
	}
    }
} // end Fd8xxRunPhase()


VOID
Fd8xxReenableAdapterInterrupts(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine doesn't really do anything.  It is here to keep the
    SCSI port driver happy about what it can do with interrupts.
    The only time we should enable interrupts on selection is when we
    get a disconnect from a target.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None.

--*/

{
    //
    // Re-enable interrupts on selection.
    //
    //Fd8xxSetControl(DeviceExtension, C_INT_ENABLE);

} // end Fd8xxReenableAdapterInterrupts()


VOID
Fd8xxDoReconnect(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine handles reconnecting devices.  It basically plucks the
    target ID out of reselection phase, followed by the LUN out of
    message in phase (from the IDENTIFY message).  Then, after validating
    the target/lun combination, starts up the state machine again for
    the reconnecting device.

    This routine should be called with interrupts enabled.

Arguments:

    DeviceExtension - Device adapter context pointer.

Return Value:

    None.

--*/

{
    PUCHAR                  baseAddress = DeviceExtension->BaseAddress;
    PSPECIFIC_LU_EXTENSION  luExtension;
    USHORT                  busFreeTimeout;
    UCHAR                   target;         // bit value returned from target
    UCHAR                   targetID;       // numeric value of target ID.
    UCHAR                   lunID;          // numeric value of LUN ID.

    //
    // Wait for the target to drop busy, which is the TRUE beginning of
    // reselection phase.  If BUSY is not dropped within the indicated time,
    // abort the interrupt (and the reselect).
    //
    if (Fd8xxStatusCheck(DeviceExtension,
			 BP_RESELECT,
			 BP_RESELECT,
			 RESELECTION_WAIT) == FALSE) {

	FdDebugPrint(0x01,
		     (0, "Fd8xxDoReconnect: BUSY not dropped: Status=%x\n",
		     FD8XX_READ_STATUS(baseAddress)));

	goto Fd8xxDoReconnect_InvalidReselection;
    }

    //
    // Delay a bit to allow for something close to a bus settle time
    // before reading the target id bits.
    //
    ScsiPortStallExecution(1);

    //
    // Try to figure out who is reselecting.
    //
    target = (FD8XX_READ_DATA(baseAddress) & ~(1 << DeviceExtension->InitiatorId));

    //
    // Convert the bit oriented target into a number.
    //
    for (targetID = ((UCHAR) -1); target != 0; target >>= 1) {

	targetID++;
    }

    //
    // Answer the reselection by raising busy.  This should cause the target
    // to transition to MESSAGE IN phase so that we cen figure out which LUN
    // is associated with the reselecting target.
    //
    ScsiPortStallExecution(1);  // Delay 400 nanoseconds (Bus-Settle).
    Fd8xxSetControl(DeviceExtension, C_BUSY);

    if (Fd8xxStatusCheck(DeviceExtension,
			 BP_MESSAGE_IN,
			 BP_MESSAGE_IN,
			 REQUEST_SPIN_WAIT) == FALSE) {

	FdDebugPrint(0x01,
		     (0, "Fd8xxDoReconnect: REQ not raised: Status=%x\n",
		     FD8XX_READ_STATUS(baseAddress)));

	goto Fd8xxDoReconnect_InvalidReselection;
    }

    //
    // The target should be driving BUSY now, so we should de-assert it in
    // control register (keep in mind that BUSY is or-tied).
    //
    Fd8xxClearControl(DeviceExtension, C_BUSY);

    lunID = FD8XX_READ_DATA(baseAddress) & (SCSI_MAXIMUM_LOGICAL_UNITS - 1);
    FdDebugPrint(0x10, (0, "Fd8xxDoReconnect: Target ID = %x, Lun Id=%x ", targetID, lunID));

    if ((targetID >= SCSI_MAXIMUM_TARGETS) ||
	(lunID >= SCSI_MAXIMUM_LOGICAL_UNITS)) {

	FdDebugPrint(0x01,
		     (0, "Fd8xxDoReconnect: BAD Target/Lun: %x/%x\n",
		     targetID, lunID));

	goto Fd8xxDoReconnect_InvalidReselection;
    }

    //
    // Re-enable adapter interrupts.  Don't do this if we're being called
    // by the arbitration routine, because they will never have been turned
    // off in the first place, and we'll only confuse the port driver.
    //
    if (DeviceExtension->SavedLu == NULL) {

	ScsiPortNotification(CallDisableInterrupts,
			     DeviceExtension,
			     Fd8xxReenableAdapterInterrupts);
    }

    luExtension = ScsiPortGetLogicalUnit(DeviceExtension,
					 DeviceExtension->PathId,
					 targetID,
					 lunID);

    if (luExtension == NULL) {

	//
	// This is the pathological case.  This would indicate that when
	// the target ID bits were read there was a parity error or some
	// other problem that made it look like a reselection from a phantom
	// device.
	//
    FdDebugPrint(0x01, (0, "Fd8xxDoReconnect: Invalid reselection.\n"));
	ScsiPortLogError(DeviceExtension,
			 NULL,
			 0,
			 0,
			 0,
			 SP_INVALID_RESELECTION,
			 (0x07 << 16) | (targetID << 8) | lunID);
	Fd8xxResetBus(DeviceExtension, DeviceExtension->PathId);
	ScsiPortNotification(ResetDetected, DeviceExtension, NULL);
	return;
    }

    if (luExtension->ActiveLuRequest == NULL) {

	//
	// This reconnect is not expected.  There is no request for the
	// device.
	//
	ScsiPortLogError(DeviceExtension,
			 NULL,
			 0,
			 0,
			 0,
			 SP_INVALID_RESELECTION,
			 (0x17 << 16) | (targetID << 8) | lunID);
	Fd8xxResetBus(DeviceExtension, DeviceExtension->PathId);
	ScsiPortNotification(ResetDetected, DeviceExtension, NULL);
	return;
    }

    if ((luExtension->AbortBeingAttempted) &&
	(DeviceExtension->SavedLu != NULL)) {

	//
	// We're being called while trying to arbitrate for an abort.
	// The LUN we're trying to abort is reconnecting, so complete the
	// abort from here, and force the arbitration routine to exit.
	//
	DeviceExtension->SavedLu->LuState = LS_COMPLETE;
	DeviceExtension->SavedLu->ActiveLuRequest->SrbStatus = SRB_STATUS_SUCCESS;
	goto Fd8xxDoReconnect_AbortReconnect;
    }

    DeviceExtension->CurDataPointer = luExtension->SavedDataPointer;
    DeviceExtension->CurDataLength  = luExtension->SavedDataLength;
    DeviceExtension->ActiveLu = luExtension;

    luExtension->LuState = LS_UNDETERMINED;

    Fd8xxRunPhase(DeviceExtension);

    return;

Fd8xxDoReconnect_InvalidReselection:

    ScsiPortLogError(DeviceExtension,
		     NULL,
		     0,
		     0,
		     0,
		     SP_INVALID_RESELECTION,
		     (0x27 << 16) | (targetID << 8) | lunID);

    if (DeviceExtension->SavedLu == NULL) {

	ScsiPortNotification(CallDisableInterrupts,
			     DeviceExtension,
			     Fd8xxReenableAdapterInterrupts);
    }

Fd8xxDoReconnect_AbortReconnect:

    //
    // Attempt to send an abort message to the target.
    //
    Fd8xxSetControl(DeviceExtension, C_ATTENTION);
    ScsiPortStallExecution(1);  // Delay 90 nanoseconds (2 * Bus-Deskew).

    while (Fd8xxWaitForRequestLine(DeviceExtension) &&
	   (FD8XX_READ_PHASE(baseAddress) != BP_MESSAGE_OUT)) {

	FD8XX_READ_DATA(baseAddress);
    }

    Fd8xxSetControl(DeviceExtension, C_BUS_ENABLE);
    Fd8xxClearControl(DeviceExtension, C_ATTENTION);
    ScsiPortStallExecution(1);  // Delay 90 nanoseconds.

    if (FD8XX_READ_PHASE(baseAddress) == BP_MESSAGE_OUT) {

	FD8XX_WRITE_DATA(baseAddress, SCSIMESS_ABORT);
    }

    Fd8xxClearControl(DeviceExtension, C_BUS_ENABLE);

    busFreeTimeout = 2048;
    while ((FD8XX_READ_PHASE(baseAddress) & S_BUSY) && (busFreeTimeout--)) {

	if (FD8XX_READ_PHASE(baseAddress) & S_REQUEST) {

	    FD8XX_READ_DATA(baseAddress);
	} else {

	    ScsiPortStallExecution(100);
	}
    }

    if (FD8XX_READ_PHASE(baseAddress) != BP_BUS_FREE) {

	//
	// Reset SCSI bus.  Drastic measures for drastic times...
	//
	Fd8xxResetBus(DeviceExtension, DeviceExtension->PathId);
	ScsiPortNotification(ResetDetected, DeviceExtension, NULL);
    }
} // end Fd8xxDoReconnect()


VOID
Fd8xxTimer(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to monitor interrupts from the Fd8xx adapter.
    The ScsiPort support for a timer is used when a disconnect message is
    received to schedule this routine as a timer for later execution.  If
    this routine is invoked before the interrupt and the adapter is attempting
    to select, then it is assumed that the adapter is on a different IRQ then
    expected and operation will continue in a "polling" mode.  If the actual
    interrupt occurs before this routine is called, then everything is ok
    and this routine need not reschedule itself.

    Operation when called is to see if the actual interrupt routine serviced
    the interrupt.  If so, then mark the device extension such that this
    routine is not rescheduled and return.  If the actual interrupt routine
    has not serviced the interrupt and the adapter is indicating that a
    reselection is being attempted, then call the interrupt routine to service
    the interrupt and continue scheduling this routine.  Lastly, if the
    interrupt routine has not serviced the interrupt and the adapter is not
    indicating a reselection, schedule another timer.

Arguments:

    Context - Device adapter context pointer.

Return Value

    TRUE indicates that the interrupt was from this Fd8xx adapter,
    FALSE indicates that this interrupt was NOT from us.

--*/

{
    PSPECIFIC_DEVICE_EXTENSION deviceExtension = Context;
    BOOLEAN                    restartTimer = TRUE;

    if (deviceExtension->ExpectingInterrupt == FALSE) {

	//
	// The interrupt routine got the interrupt.  Decrement the number
	// of times to continue scheduling a timer routine.
	//
	deviceExtension->ContinueTimer--;
	return;
    }

    if (Fd8xxStatusCheck(deviceExtension, S_SELECT, S_SELECT, 1)) {

	//
	// If the interrupt routine does not claim this interrupt for some
	// reason, restart the timer.  If it does claim the interrupt, shut
	// off the timer.
	//
	restartTimer = Fd8xxInterrupt(Context) == FALSE ? TRUE : FALSE;

	//
	// Now determine if an event log entry should be made to inform
	// the system administrator that there is a possible configuration
	// error on this driver.
	//
	if ((restartTimer == FALSE) &&
	    (deviceExtension->NotifiedConfigurationError == FALSE)) {
	    deviceExtension->TimerCaughtInterrupt--;
	    if ((deviceExtension->TimerCaughtInterrupt == 0) &&
		(deviceExtension->ConfiguredWithoutInterrupts == FALSE)) {
		ScsiPortLogError(deviceExtension,
				 NULL,
				 0,
				 0,
				 0,
				 SP_IRQ_NOT_RESPONDING,
				 8);
		deviceExtension->NotifiedConfigurationError = TRUE;
	    }
	}
    }

    if (restartTimer) {

	//
	// Not selected.  Set timer for another call.
	//
	ScsiPortNotification(RequestTimerCall,
			     deviceExtension,
			     Fd8xxTimer,
			     FD8xx_TIMER_VALUE);
    }
}


BOOLEAN
Fd8xxInterrupt(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine handles the interrupts for the FD8XX.  The intention is to
    quickly determine the cause of the interrupt, clear the interrupt, and
    setup to process any SCSI command that may have been affected by the
    interrupt.

Arguments:

    Context - Device adapter context pointer.

Return Value:

    TRUE indicates that the interrupt was from this Fd8xx< adapter,
    FALSE indicates that this interrupt was NOT from us.

--*/

{
    PSPECIFIC_DEVICE_EXTENSION deviceExtension = Context;

    if ((Fd8xxStatusCheck(deviceExtension, S_SELECT, S_SELECT, 1) == FALSE) ||
	(deviceExtension->SelectingTarget)) {

	//
	// Spurious interrupt or for some other device.
	//
	FdDebugPrint(0x100,
		      (0, "Fd8xxInterrupt: Spurious Interrupt or NOT Selected: Status = %x, Last CtrlReg = %x\n",
		      FD8XX_READ_STATUS(deviceExtension->BaseAddress),
		      deviceExtension->ControlRegister));
	return FALSE;

    } else {

	//
	// We are being reselected by a target.
	//
	FdDebugPrint(0x10,
		    (0, "Fd8xxInterrupt: Selected: Status = %x, Last CtrlReg = %x",
		    FD8XX_READ_STATUS(deviceExtension->BaseAddress),
		    deviceExtension->ControlRegister));

	deviceExtension->ExpectingInterrupt = FALSE;
	//Fd8xxClearControl(deviceExtension, C_INT_ENABLE);

	//
	// Here's where the fun starts.  We tell the OS to call DoReconnect
	// with system interrupts enabled.  This way we're not transferring
	// gobs of data with the rest of the world blocked out.  We can be
	// friendly neighbors and do that sort of CPU hungry stuff while
	// letting others grab it when necessary.
	//
	ScsiPortNotification(CallEnableInterrupts,
			     deviceExtension,
			     Fd8xxDoReconnect);

	return TRUE;
    }
} // end Fd8xxInterrupt()


VOID
Fd8xxDpcRunPhase(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine is called by the port driver with interrupts enabled.
    It calls the normal run phase routine of the 8xx driver to process
    the request.

Arguments:

    DeviceExtension - context pointer for the start of run phase.

Return Value:

    None.

--*/

{

    Fd8xxRunPhase(DeviceExtension);

    ScsiPortNotification(CallDisableInterrupts,
			 DeviceExtension,
			 Fd8xxReenableAdapterInterrupts);

    //
    // Adapter ready for next request.
    //
    ScsiPortNotification(NextRequest,
			 DeviceExtension,
			 NULL);
}


VOID
Fd8xxStartExecution(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN PSPECIFIC_LU_EXTENSION     LuExtension,
    IN PSCSI_REQUEST_BLOCK        Srb
    )

/*++

Routine Description:

    This routine will start (and possibley complete) the execution of
    a SCSI request.

Arguments:

    DeviceExtension -   Device adapter context pointer.
    LuExtension -       The logical unit specific information.
    Srb -               The Srb command to execute.

Return Value:

    None.

--*/

{

    //
    // Setup the context for this adapter.
    //
    DeviceExtension->ActiveLu = LuExtension;

    //
    // Turn off selection interrupt.
    //
    //Fd8xxClearControl(DeviceExtension, C_INT_ENABLE);

    //
    // Check for potential tape access to insure handshake data transfer.
    // This is done here instead of in the deferred routine to save
    // two pointer loads (Srb and LuExtension).
    //
    if (Srb->CdbLength == 6) {
	LuExtension->SixByteCDBActive = TRUE;
    } else {
	LuExtension->SixByteCDBActive = FALSE;
    }

    //
    // Run the bus phase with other interrupts in the system enabled.
    //
    ScsiPortNotification(CallEnableInterrupts,
			 DeviceExtension,
			 Fd8xxDpcRunPhase);
} // end Fd8xxStartExecution()


VOID
Fd8xxAbort(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN PSPECIFIC_LU_EXTENSION     LuExtension,
    IN PSCSI_REQUEST_BLOCK        Srb
    )

/*++

Routine Description:

    Attempt to abort a command on a SCSI target.

Arguments:

    DeviceExtension -   The adapter specific information.
    LuExtension     -   The specific target/logical unit information.
    Srb -               The Srb command to execute.

Return Value:

    None.

--*/

{
    PUCHAR              baseAddress = DeviceExtension->BaseAddress;
    PSCSI_REQUEST_BLOCK srbBeingAborted = LuExtension->ActiveLuRequest;


    //
    // Make sure there is still a request to complete.  If so complete
    // it with an SRB_STATUS_ABORTED status.
    //
    if (srbBeingAborted == NULL) {

	//
	// If there is no request, then fail the abort.
	//
	Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;

	FdDebugPrint(0x01, (0, "Fd8xxAbort: Abort failed, No abort srb.\n"));

    } else {

        FdDebugPrint(0x01, (0, "Fd8xxAbort: Srb %x\n",
                     srbBeingAborted));

	if (LuExtension->LuState == LS_DISCONNECTED) {

	    //
	    // Attempt to send an abort message SRB into the state machine.
	    //

	    LuExtension->LuState = LS_ARBITRATE;
	    LuExtension->AbortBeingAttempted = TRUE;
	    DeviceExtension->ActiveLu = LuExtension;
	    Fd8xxArbitrate(DeviceExtension);

	    if ((LuExtension->LuState == LS_SELECT) ||
		(LuExtension->LuState == LS_COMPLETE)) {

		//
		// Finish off the abort.
		//

		Fd8xxRunPhase(DeviceExtension);
		LuExtension->AbortBeingAttempted = FALSE;

		//
		// Now force a completion on the SRB that is being aborted.
		//

		LuExtension->LuState = LS_COMPLETE;
		srbBeingAborted->SrbStatus = SRB_STATUS_ABORTED;

		LuExtension->ActiveLuRequest = srbBeingAborted;
		DeviceExtension->ActiveLu = LuExtension;
		Fd8xxNotifyCompletion(DeviceExtension);

	    } else {

		LuExtension->AbortBeingAttempted = FALSE;
		Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;

		FdDebugPrint(0x10, (0, "FdAbort failed.\n"));
	    }

	} else if (LuExtension->LuState != LS_COMPLETE) {

	    //
	    // The SCSI bus is most likely hung by this LUN, so reset it.
	    //
	    FdDebugPrint(0x01, (0, "Fd8xxAbort: Still connected to SCSI bus. Resetting.\n"));
	    Fd8xxResetBus(DeviceExtension, Srb->PathId);
	    ScsiPortNotification(ResetDetected, DeviceExtension, NULL);
	}
    }
} // end Fd8xxAbort()


BOOLEAN
Fd8xxResetBus(
    IN PVOID Context,
    IN ULONG PathId
    )

/*++

Routine Description:

    Reset Future Domain 8XX SCSI adapter (there is no action for this)
    and SCSI bus.

Arguments:

    Context - pointer to the device extension for the reset.
    PathId  - The bus for telling the port driver where the reset occurred.

Return Value:

    Nothing.

--*/

{
    PSPECIFIC_DEVICE_EXTENSION deviceExtension = Context;

    FdDebugPrint(0x01, (0, "FdResetBus: Reset Fd8xx and SCSI bus\n"));

    //
    // RESET SCSI bus.
    //
    Fd8xxSetControl(deviceExtension,
		    C_RESET);
    ScsiPortStallExecution(RESET_HOLD_TIME);
    Fd8xxClearControl(deviceExtension,
		      C_RESET);

    //
    // Complete all outstanding requests with SRB_STATUS_BUS_RESET.
    //
    ScsiPortCompleteRequest(deviceExtension,
			    (UCHAR) PathId,
			    (UCHAR) -1,
			    (UCHAR) -1,
			    SRB_STATUS_BUS_RESET);

    //
    // Find all luExtensions and zero them out.
    //
    if (deviceExtension->ActiveLu != NULL) {
	deviceExtension->ActiveLu->LuState = LS_COMPLETE;
	deviceExtension->ActiveLu->ActiveLuRequest = NULL;
	deviceExtension->ActiveLu->NoDisconnectActive = FALSE;
	deviceExtension->ActiveLu->AbortBeingAttempted = FALSE;
	deviceExtension->ActiveLu = NULL;
    }

    if (deviceExtension->SavedLu != NULL) {
	deviceExtension->SavedLu->LuState = LS_COMPLETE;
	deviceExtension->SavedLu->ActiveLuRequest = NULL;
	deviceExtension->SavedLu->NoDisconnectActive = FALSE;
	deviceExtension->SavedLu->AbortBeingAttempted = FALSE;
	deviceExtension->SavedLu = NULL;
    }

    return TRUE;
} // end Fd8xxResetBus()


BOOLEAN
Fd8xxStartIo(
    IN PVOID               Context,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine is called from the SCSI port driver synchronized
    with the kernel with a request to be executed.

Arguments:
    Context -   The adapter specific information.
    Srb -       The Srb command to execute.

Return Value:

    TRUE

--*/

{
    PSPECIFIC_DEVICE_EXTENSION  deviceExtension = Context;
    PSPECIFIC_LU_EXTENSION      luExtension;

    //
    // Determine the logical unit that this request is for.
    //
    deviceExtension->PathId = Srb->PathId;
    luExtension = ScsiPortGetLogicalUnit(deviceExtension,
					 deviceExtension->PathId,
					 Srb->TargetId,
					 Srb->Lun);
    Srb->SrbStatus = SRB_STATUS_PENDING;

    switch (Srb->Function) {

    case SRB_FUNCTION_ABORT_COMMAND:

	FdDebugPrint(0x01, (0, "Fd8xxStartIO: ABORT COMMAND.\n"));

	//
	// Abort request in progress.
	//
	// Fd8xxResetBus(deviceExtension, Srb->PathId);
	Fd8xxAbort(deviceExtension, luExtension, Srb);
	break;

    case SRB_FUNCTION_RESET_BUS:

	FdDebugPrint(0x01, (0, "Fd8xxStartIO: RESET BUS.\n"));

	//
	// Reset Fd8xx and SCSI bus.
	//
	Fd8xxResetBus(deviceExtension, Srb->PathId);
	Srb->SrbStatus = SRB_STATUS_SUCCESS;
	break;

    case SRB_FUNCTION_EXECUTE_SCSI:

	FdDebugPrint(0x08, (0, "Fd8xxStartIO: EXECUTE SCSI.\n"));

	//
	// Setup the context for this target/lun.
	//
	luExtension->ActiveLuRequest = Srb;
	luExtension->LuState = LS_ARBITRATE;
	luExtension->SavedDataPointer = (ULONG) Srb->DataBuffer;
	luExtension->SavedDataLength = Srb->DataTransferLength;

	//
	// Initiate a SCSI request.
	//
	Fd8xxStartExecution(deviceExtension, luExtension, Srb);
	return TRUE;

    default:

	Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
	ScsiPortNotification(RequestComplete,
			     (PVOID) deviceExtension,
			     Srb);
	break;

    }

    //
    // Adapter ready for next request.
    //
    ScsiPortNotification(NextRequest,
			 deviceExtension,
			 NULL);
    return TRUE;
} // end Fd8xxStartIo()


BOOLEAN
Fd8xxInitialize(
    IN PVOID Context
    )

/*++

Routine Description:

    Inititialize Fd8xx adapter.

Arguments:

    Context - Adapter object device extension.

Return Value:

    Status.

--*/

{
    PSPECIFIC_DEVICE_EXTENSION deviceExtension = Context;

    //
    // Reset Fd8xx and SCSI bus.
    //
    Fd8xxWriteControl(deviceExtension, C_RESET);
    ScsiPortStallExecution(RESET_HOLD_TIME);
    Fd8xxClearControl(deviceExtension, C_RESET);

    ScsiPortNotification(ResetDetected,
			 (PVOID) deviceExtension);

    Fd8xxSetControl(deviceExtension, C_INT_ENABLE);
    deviceExtension->ExpectingInterrupt = FALSE;
    deviceExtension->NotifiedConfigurationError = FALSE;
    deviceExtension->ContinueTimer = 5;
    deviceExtension->TimerCaughtInterrupt = 10;
    deviceExtension->ConfiguredWithoutInterrupts = FALSE;

    return TRUE;
} // end Fd8xxInitialize()


BOOLEAN
Fd8xxCheckBaseAddress(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN PUCHAR                     BaseAddress
    )

/*++

Routine Description:

    This routine will check to see if there is an adapter at the
    provided base address.  It does this by first reading and comparing
    all of the data area bytes on the adapter.  If these are all equal
    it then changes the first value and performs the compare again.
    All other data area values should change to be equal to the first
    value when the change is made.  If they are not equal any changes
    are restored.

Arguments:

    DeviceExtension - Necessary to attempt the reads.
    BaseAddress     - The address to attempt to use for the base of the
		      memory area

Return Value:

    TRUE    - There is an adapter present at the base address provided.
    FALSE   - There is no adapter present at the base address provided.

--*/

{
    UCHAR  testValue;
    UCHAR  oldValue;
    UCHAR  oldStatus;
    USHORT offset;

    if (BaseAddress == 0) {
        return FALSE;
    }

    oldValue = FD8XX_READ_DATA(BaseAddress);

    //
    // Perform a read test of the memory area of the adapter.
    //
    for (offset = 254; offset > 0; offset--) {

	testValue = FD8XX_READ_ALTERNATE_DATA(BaseAddress, offset);
	if (oldValue != testValue) {

	    FdDebugPrint(0x01, (0, "Fd8xxCheckBaseAddress: Read test failed.\n"));
	    return FALSE;
	}
    }

    FdDebugPrint(0x8, (0, "Fd8xxCheckBaseAddress: Read test passed.\n"));

    //
    // Card may be present in the system.  Perform a write test.
    // The write value is insured to be different than the previous
    // value by adding 1
    //
    oldValue++;

    oldStatus = FD8XX_READ_STATUS(BaseAddress);
    Fd8xxWriteControl(DeviceExtension,
		      (C_BUS_ENABLE | C_PARITY_ENABLE));

    FD8XX_WRITE_DATA(BaseAddress, oldValue);

    for (offset = 254; offset > 0; offset--) {

	testValue = FD8XX_READ_ALTERNATE_DATA(BaseAddress, offset);

	if (oldValue != testValue) {

	    oldValue--;    // Get the original value and restore it.
	    FD8XX_WRITE_DATA(BaseAddress, oldValue);
	    FdDebugPrint(0x01, (0, "Fd8xxCheckBaseAddress: Write test failed.\n"));
	    Fd8xxWriteControl(DeviceExtension, oldStatus);
	    return FALSE;
	}
    }

    FdDebugPrint(0x08, (0, "Fd8xxCheckBaseAddress: Write test passed.\n"));

    return TRUE;
} // end Fd8xxCheckBaseAddress()


ULONG
Fd8xxParseArgumentString(
    IN PCHAR String,
    IN PCHAR KeyWord
    )

/*++

Routine Description:

    This routine will parse the string for a match on the keyword, then
    calculate the value for the keyword and return it to the caller.

Arguments:

    String - The ASCII string to parse.
    KeyWord - The keyword for the value desired.

Return Values:

    Zero if value not found
    Value converted from ASCII to binary.

--*/

{
    PCHAR cptr;
    PCHAR kptr;
    ULONG value;
    ULONG stringLength = 0;
    ULONG keyWordLength = 0;
    ULONG index;

    //
    // Calculate the string length and lower case all characters.
    //
    cptr = String;
    while (*cptr) {

	if (*cptr >= 'A' && *cptr <= 'Z') {
	    *cptr = *cptr + ('a' - 'A');
	}
	cptr++;
	stringLength++;
    }

    //
    // Calculate the keyword length and lower case all characters.
    //
    cptr = KeyWord;
    while (*cptr) {

	if (*cptr >= 'A' && *cptr <= 'Z') {
	    *cptr = *cptr + ('a' - 'A');
	}
	cptr++;
	keyWordLength++;
    }

    if (keyWordLength > stringLength) {

	//
	// Can't possibly have a match.
	//
	return 0;
    }

    //
    // Now setup and start the compare.
    //
    cptr = String;

ContinueSearch:
    //
    // The input string may start with white space.  Skip it.
    //
    while (*cptr == ' ' || *cptr == '\t') {
	cptr++;
    }

    if (*cptr == '\0') {

	//
	// end of string.
	//
	return 0;
    }

    kptr = KeyWord;
    while (*cptr++ == *kptr++) {

	if (*(cptr - 1) == '\0') {

	    //
	    // end of string
	    //
	    return 0;
	}
    }

    if (*(kptr - 1) == '\0') {

	//
	// May have a match backup and check for blank or equals.
	//

	cptr--;
	while (*cptr == ' ' || *cptr == '\t') {
	    cptr++;
	}

	//
	// Found a match.  Make sure there is an equals.
	//
	if (*cptr != '=') {

	    //
	    // Not a match so move to the next semicolon.
	    //
	    while (*cptr) {
		if (*cptr++ == ';') {
		    goto ContinueSearch;
		}
	    }
	    return 0;
	}

	//
	// Skip the equals sign.
	//
	cptr++;

	//
	// Skip white space.
	//
	while ((*cptr == ' ') || (*cptr == '\t')) {
	    cptr++;
	}

	if (*cptr == '\0') {

	    //
	    // Early end of string, return not found
	    //
	    return 0;
	}

	if (*cptr == ';') {

	    //
	    // This isn't it either.
	    //
	    cptr++;
	    goto ContinueSearch;
	}

	value = 0;
	if ((*cptr == '0') && (*(cptr + 1) == 'x')) {

	    //
	    // Value is in Hex.  Skip the "0x"
	    //
	    cptr += 2;
	    for (index = 0; *(cptr + index); index++) {

		if (*(cptr + index) == ' ' ||
		    *(cptr + index) == '\t' ||
		    *(cptr + index) == ';') {
		     break;
		}

		if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
		    value = (16 * value) + (*(cptr + index) - '0');
		} else {
		    if ((*(cptr + index) >= 'a') && (*(cptr + index) <= 'f')) {
			value = (16 * value) + (*(cptr + index) - 'a' + 10);
		    } else {

			//
			// Syntax error, return not found.
			//
			return 0;
		    }
		}
	    }
	} else {

	    //
	    // Value is in Decimal.
	    //
	    for (index = 0; *(cptr + index); index++) {

		if (*(cptr + index) == ' ' ||
		    *(cptr + index) == '\t' ||
		    *(cptr + index) == ';') {
		     break;
		}

		if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
		    value = (10 * value) + (*(cptr + index) - '0');
		} else {

		    //
		    // Syntax error return not found.
		    //
		    return 0;
		}
	    }
	}

	return value;
    } else {

	//
	// Not a match check for ';' to continue search.
	//
	while (*cptr) {
	    if (*cptr++ == ';') {
		goto ContinueSearch;
	    }
	}

	return 0;
    }
}


BOOLEAN
Fd8xxFindSignature(
    PUCHAR romAddress,
    PUCHAR p1RomBiosId)

/*++

Routine Description:

    This routine searches for a P2 ROM BIOS signature within the rom address
    provided.

Arguments:

    romAddress - Location to search
    plRomBiosId - BIOS string to match

Return Values:

    TRUE if found
    FALSE otherwise

--*/

{
    USHORT  count = 64;         // Check first 64 bytes of ROM space.
    PUCHAR  ptr1 = romAddress;

    while (count--) {

	PUCHAR  ptr2 = ptr1++;
	PUCHAR  ptr3 = p1RomBiosId;
	UCHAR   c = ScsiPortReadRegisterUchar(ptr2);

	while (c == *ptr3++) {

	    ptr2++;
	    c = ScsiPortReadRegisterUchar(ptr2);

	    if (*ptr3 == '\0')
		return TRUE;

	    if (c == '\0')
		return FALSE;
	}
    }

    return FALSE;
}


BOOLEAN
Fd8xxVerifyPatriot1(
    PSPECIFIC_DEVICE_EXTENSION      DeviceExtension,
    PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )

/*++

Routine Descriptions:

    This routine verifies the presence of a P2 ROM based adapter.

Arguments:

    DeviceExtension - call context.
    ConfigInfo      - system provided configuration information.

Return Value:

    TRUE if found
    FALSE otherwise

--*/

{
    UCHAR   p1RomBiosId[] = "IBM F1 BIOS";
    BOOLEAN retval;

    if (Fd8xxFindSignature(DeviceExtension->BaseAddress, p1RomBiosId)) {

	//
	// We have a patriot-1 adapter installed.
	//
	retval = TRUE;

    } else {

	retval = FALSE;
    }

    return retval;
}


ULONG
Fd8xxFindAdapter(
    IN PVOID                                Context,
    IN PVOID                                AdaptersFound,
    IN PVOID                                BusInformation,
    IN PCHAR                                ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION  ConfigInfo,
    OUT PBOOLEAN                            Again
    )

/*++

Routine Description:

    This routine maps the memory area for the FD8XX adapter into
    the virtual address space for the system.  It then attempts to
    find the adapter and set the configuration information.  If
    the adapter is not present in the system (or cannot be found)
    this routine returnes an indication as such (i.e. FALSE).

Arguments:

    Context         - The device specific context for the call.
    AdaptersFound   - Passed through from the driver entry as additional
		      context for the call.
    BusInformation  - Unused.
    ArgumentString  - Points to the potential IRQ for this adapter.
    ConfigInfo      - Pointer to the configuration information structure to
		      be filled in.
    Again           - Returns back a request to call this function again.

Return Value:

    SP_RETURN_FOUND     - if an adapter is found.
    SP_RETURN_NOT_FOUND - if no adapter is found.

--*/

{
    USHORT                      curBaseAddress;
    USHORT                      adaptersFound;
    ULONG                       returnStatus = SP_RETURN_NOT_FOUND;
    PSPECIFIC_DEVICE_EXTENSION  deviceExtension = Context;
    ULONG baseAddresses[] =
    {
	0x000c8000,
	0x000ca000,
	0x000ce000,
	0x000de000,
	0x000e8000,
	0x000ea000,
	0
    };

     //
     // This must never be set to TRUE for this controller.  If we return
     // TRUE for this parameter, the SCSI port driver will not move to the
     // next entry in the registry for the next controller of this type.
     // Instead, it will call this routine again with the SAME registry
     // information (i.e., IRQ vector) causing the next adapter with a
     // DIFFERENT IRQ to fail to install.
     //
    *Again = FALSE;

    curBaseAddress = (USHORT) (*((PULONG) AdaptersFound) >> 16);
    adaptersFound = (USHORT) (*((PULONG) AdaptersFound));

    if ((baseAddresses[curBaseAddress] == 0) ||
	   (adaptersFound == MAX_ADAPTERS)) {

	return SP_RETURN_NOT_FOUND;
    }

    while ((baseAddresses[curBaseAddress] != 0) &&
	   (adaptersFound != MAX_ADAPTERS) &&
	   (returnStatus == SP_RETURN_NOT_FOUND)) {

	PUCHAR  baseAddress;

	FdDebugPrint(0x02,
			 (0, "Fd8xxFindAdapter: baseAddresses[%x] = %x, ",
			 curBaseAddress,
			 baseAddresses[curBaseAddress]));
	FdDebugPrint(0x02,
			 (0, "%x adapters found so far.\n",
			 adaptersFound));

	//
	// Map the TMC-950 chip into the virtual memory address space.
	// If ConfigInfo already has default information about this
	// controller, use it.  If not, then we derive our own.  This
	// is for Chicago compatibility.
	//
	if (ScsiPortConvertPhysicalAddressToUlong(
		(*ConfigInfo->AccessRanges)[0].RangeStart) != 0) {

	    deviceExtension->BaseAddress = baseAddress =
		ScsiPortGetDeviceBase(
		    deviceExtension,
		    ConfigInfo->AdapterInterfaceType,
		    ConfigInfo->SystemIoBusNumber,
		    (*ConfigInfo->AccessRanges)[0].RangeStart,
		    (*ConfigInfo->AccessRanges)[0].RangeLength,
		    (BOOLEAN) !((*ConfigInfo->AccessRanges)[0].RangeInMemory));
	} else {

	    deviceExtension->BaseAddress = baseAddress =
		ScsiPortGetDeviceBase(
		    deviceExtension,
		    ConfigInfo->AdapterInterfaceType,
		    ConfigInfo->SystemIoBusNumber,
		    ScsiPortConvertUlongToPhysicalAddress(baseAddresses[curBaseAddress]),
		    (FD8XX_ADDRESS_SIZE),     // NumberOfBytes
		    (BOOLEAN) FALSE);         // InIoSpace
	}
	FdDebugPrint(0x08,
		    (0, "Fd8xxFindAdapter: MmMapIoSpace address = %x ",
		    baseAddress));

	//
	// Determine if the card is really installed in the system.
	//
	if (Fd8xxCheckBaseAddress(deviceExtension, baseAddress) == FALSE) {

	    //
	    // We did not find an adapter at this baseAddress.  Free the
	    // mapped memory space and continue with the next address if
	    // there is one.
	    //
	    returnStatus = SP_RETURN_NOT_FOUND;
            if (baseAddress != 0) {
                ScsiPortFreeDeviceBase(deviceExtension,
                                       (PVOID) baseAddress);
            }

	} else {
        FdDebugPrint(0x01,(0,"Fd8xxFindAdapter: Adapter found at address %x\n",
                     baseAddress
                     ));

	    returnStatus = SP_RETURN_FOUND;

	    //
	    // If there was no previously configured value for the IRQ,
	    // look at the other way to configure the value or use a default.
	    //
	    if (ConfigInfo->BusInterruptLevel == 0) {

		ConfigInfo->BusInterruptLevel = FD8XX_IDT_VECTOR;

		if (ArgumentString != NULL) {
		    ULONG irq = Fd8xxParseArgumentString(ArgumentString, "irq");

		    if (irq == 0) {

			//
			// Check for the old way.
			//
			irq = *((PULONG)ArgumentString);
			if (irq > 15) {

			    //
			    // really was intended to be zero.
			    //
			    irq = 0;
			    deviceExtension->ConfiguredWithoutInterrupts = TRUE;
			} else {
			    irq = FD8XX_IDT_VECTOR;
			}
		    }

		    FdDebugPrint(0x01,
				(0,
				"Fd8xxFindAdapter: Using IRQ %d passed in.\n",
				irq));
		    ConfigInfo->BusInterruptLevel = irq;
		} else {

		    FdDebugPrint(0x01, (0, "Fd8xxFindAdapter: Using default IRQ.\n"));
		}
	    } else {

		FdDebugPrint(0x01,
			    (0,
			    "Fd8xxFindAdapter: Using IRQ previously set to %d. ",
			    ConfigInfo->BusInterruptLevel));
	    }

	    //
	    // If the user has not specified a target ID, fill in a default.
	    //
	    if (ConfigInfo->InitiatorBusId[0] == (CCHAR) SP_UNINITIALIZED_VALUE) {

		deviceExtension->InitiatorId = SCSI_INITIATOR_ID;
		ConfigInfo->InitiatorBusId[0] = SCSI_INITIATOR_ID;
	    } else {

		deviceExtension->InitiatorId = ConfigInfo->InitiatorBusId[0];
		FdDebugPrint(0x01,
			    (0,
			    "Fd8xxFindAdapter: Initiator ID set to %d ",
			    ConfigInfo->InitiatorBusId[0]));
	    }

	    //
	    // Fill in the access array information only if there are no
	    // default parameters already there.
	    //
	    if (ScsiPortConvertPhysicalAddressToUlong(
		    (*ConfigInfo->AccessRanges)[0].RangeStart) == 0) {

		//
		// There is no pre-assigned information in the ConfigInfo
		// structure derived by Chicago.  Thus, we can fill
		// our own derived information into the ConfigInfo structure.
		// Otherwise WE MUST NOT change the ConfigInfo structure, as
		// this will overwrite the Chicago-prestored information.
		//
		(*ConfigInfo->AccessRanges)[0].RangeStart =
		    ScsiPortConvertUlongToPhysicalAddress(baseAddresses[curBaseAddress]);
		(*ConfigInfo->AccessRanges)[0].RangeLength = FD8XX_ADDRESS_SIZE;
		(*ConfigInfo->AccessRanges)[0].RangeInMemory = TRUE;

		if (Fd8xxVerifyPatriot1(deviceExtension, ConfigInfo)) {

		    //
		    // We have a Patriot-I adapter installed in the system
		    // which scans from Target ID 7 to Target ID 0.
		    //
		    ConfigInfo->AdapterScansDown = TRUE;
		}
	    }

	    adaptersFound++;
	}

	ConfigInfo->ScatterGather = FALSE;
	ConfigInfo->Master = FALSE;
	ConfigInfo->MaximumTransferLength = (ULONG) MAX_TRANSFER_LENGTH;
	ConfigInfo->NumberOfBuses = 1;
	ConfigInfo->BufferAccessScsiPortControlled = TRUE;

	curBaseAddress++;

    }

    *((PULONG) AdaptersFound) = ((ULONG) curBaseAddress) << 16;
    *((PULONG) AdaptersFound) += (ULONG) adaptersFound;

    return returnStatus;
} // end Fd8xxFindAdapter()


BOOLEAN
Fd8xxAdapterState(
    IN PVOID    DeviceExtension,
    IN PVOID    AdaptersFound,
    IN BOOLEAN  SaveState
    )

/*++

Routine Description:

    Saves/restores adapter's real-mode configuration state.

Arguments:

    DeviceExtension - Adapter object device extension.
    AdaptersFound   - Passed through from DriverEntry as additional
		      context for the call.
    SaveState       - TRUE = Save adapter state, FALSE = restore state.

Return Value:

    The spec did not intend for this routine to have a return value.
    Whoever did the header file just forgot to change the BOOLEAN to
    a VOID.  We will just return FALSE to shot the compiler up.

--*/

{
    return FALSE;
}


ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )

/*++

Routine Description:

    Driver initialization entry point for system.

Arguments:

    DriverObject - The driver specific object pointer
    Argument2    - not used.

Return Value:

    Status from ScsiPortInitialize()

--*/

{
    HW_INITIALIZATION_DATA  hwInitializationData;
    ULONG                   i;

    FdDebugPrint(0x01, (0, "\nSCSI Future Domain 8XX Miniport Driver\n"));

    //
    // Zero out the hwInitializationData structure.
    //
    for (i = 0; i < sizeof(HW_INITIALIZATION_DATA); i++) {

	*(((PUCHAR)&hwInitializationData + i)) = 0;
    }

    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //
    hwInitializationData.HwInitialize   = Fd8xxInitialize;
    hwInitializationData.HwStartIo      = Fd8xxStartIo;
    hwInitializationData.HwInterrupt    = Fd8xxInterrupt;
    hwInitializationData.HwFindAdapter  = Fd8xxFindAdapter;
    hwInitializationData.HwResetBus     = Fd8xxResetBus;
    hwInitializationData.HwAdapterState = Fd8xxAdapterState;

    //
    // Indicate need buffer mapping but not physical addresses.
    //
    hwInitializationData.MapBuffers             = TRUE;
    hwInitializationData.NeedPhysicalAddresses  = FALSE;
    hwInitializationData.NumberOfAccessRanges   = 1;

    //
    // Specify size of device extension.
    //
    hwInitializationData.DeviceExtensionSize = sizeof(SPECIFIC_DEVICE_EXTENSION);

    //
    // Specify size of logical unit extension.
    //
    hwInitializationData.SpecificLuExtensionSize = sizeof(SPECIFIC_LU_EXTENSION);

    //
    // The fourth parameter below (i.e., "i") will show up as the
    // "AdaptersFound" parameter when FindAdapter() is called.
    //

    FdDebugPrint(0x08, (0, "Fd8xxDriverEntry: Trying ISA...\n"));
    hwInitializationData.AdapterInterfaceType = Isa;

    i = 0;
    return ScsiPortInitialize(DriverObject,
			      Argument2,
			      &hwInitializationData,
			      &(i));

} // end DriverEntry()


