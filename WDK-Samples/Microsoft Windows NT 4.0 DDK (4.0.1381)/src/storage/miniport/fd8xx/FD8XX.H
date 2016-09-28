/*++

Copyright (c) 1991, 1992  Microsoft Corporation
Copyright (c) 1992  Future Domain Corporation

Module Name:

    fd8xx.h

Abstract:

    Definitions for the Future Domain TMC-8XX SCSI controllers.

Author:

    Bob Rinne (bobri - Microsoft)           June 1991
    Roger Stoller (v-rogs - Future Domain)  August 1992

Revision History:

--*/

#include "scsi.h"

//
// Size of memory mapped area used.
//
#define FD8XX_ADDRESS_SIZE  0x2000

//
// I/O Memory Map Interface.  Offsets from BaseAddress referrences.
//
#define WRITE_CONTROL   0x1C00
#define READ_STATUS     0x1C00
#define WRITE_SCSI      0x1E00
#define READ_SCSI       0x1E00

//
// Macros for accessing the control memory of the FD8XX.
//
#define FD8XX_SET_CONTROL(BASE, VALUE) \
          ScsiPortWriteRegisterUchar((BASE + WRITE_CONTROL), (VALUE))

#define FD8XX_READ_STATUS(BASE) \
          ScsiPortReadRegisterUchar(BASE + READ_STATUS)

#define FD8XX_READ_ALTERNATE_STATUS(BASE, OFFSET) \
          ScsiPortReadRegisterUchar(BASE + (READ_STATUS + OFFSET))

#define FD8XX_WRITE_DATA(BASE, VALUE) \
          ScsiPortWriteRegisterUchar((BASE + WRITE_SCSI), (UCHAR) (VALUE))

#define FD8XX_READ_DATA(BASE) \
          ScsiPortReadRegisterUchar(BASE + READ_SCSI)

#define FD8XX_READ_ALTERNATE_DATA(BASE, OFFSET) \
          ScsiPortReadRegisterUchar(BASE + READ_SCSI + OFFSET)

//
// Control port output codes.
//
#define C_RESET         0x01    // Assert SCSI reset line
#define C_SELECT        0x02    // Assert SCSI select line
#define C_BUSY          0x04    // Assert SCSI busy line
#define C_ATTENTION     0x08    // Assert SCSI attention line
#define C_ARBITRATION   0x10    // Start arbitration
#define C_PARITY_ENABLE 0x20    // Parity enable
#define C_INT_ENABLE    0x40    // Enable interrupt
#define C_BUS_ENABLE    0x80    // Bus enable

//
// Input status bit definitions.
//
#define S_BUSY          0x01    // Busy line from SCSI bus
#define S_MESSAGE       0x02    // Message line from SCSI bus
#define S_IO            0x04    // Input/Output line from SCSI bus
#define S_CD            0x08    // Command/Data line from SCSI bus
#define S_REQUEST       0x10    // Request line from SCSI bus
#define S_SELECT        0x20    // Select line from SCSI bus
#define S_PARITY        0x40    // Parity error status
#define S_ARB_COMPLETE  0x80    // Arbitration complete

//
// Useful status combinations.
//
#define BP_BUS_FREE     0
#define BP_COMMAND      ( S_BUSY | S_CD | S_REQUEST )
#define BP_MESSAGE_IN   ( S_BUSY | S_MESSAGE | S_IO | S_CD | S_REQUEST )
#define BP_MESSAGE_OUT  ( S_BUSY | S_MESSAGE | S_CD | S_REQUEST )
#define BP_DATA_IN      ( S_BUSY | S_IO | S_REQUEST )
#define BP_DATA_OUT     ( S_BUSY | S_REQUEST )
#define BP_STATUS       ( S_BUSY | S_IO | S_CD | S_REQUEST )
#define BP_RESELECT     ( S_SELECT | S_IO )

//
// Status mask for bus phase.  This is everything except Parity and
// Arbitration complete.
//
#define S_PHASE_MASK    ((UCHAR) 0x3F)

#define FD8XX_READ_PHASE(BASE)  ((UCHAR) (FD8XX_READ_STATUS(BASE) & S_PHASE_MASK))

//
// Interrupt definition.
//
#define FD8XX_IDT_VECTOR    5

//
// SCSI source identifier for host system.
//
#define SCSI_INITIATOR_ID   7

//
// Various timeout values (in microseconds).
//
#define REQUEST_SPIN_WAIT    750000 // Wait for target to assert REQUEST
#define RESET_HOLD_TIME          25 // Time to hold RESET line to reset bus
#define SELECTION_DELAY      200000 // Wait for target to assert BUSY
#define RESELECTION_WAIT     200000 // Wait for target to de-assert BUSY
#define ARBITRATION_DELAY      1000 // Wait for arbitration complete
#define FD8xx_TIMER_VALUE     10000 // Time for timer to fire


//
// Various limits...
//

//
// Number of adapters this driver will support.
//
#define MAX_ADAPTERS        4

//
// Actually there is no limit to the transfer size, but since the I/O area
// has to be mapped into memory for the transfer we restrict it to the
// number below.
//
#define MAX_TRANSFER_LENGTH ( -1 )

//
// Size of the memory-mapped SCSI data register on the adapter.
//
#define MAX_BUFFER_LENGTH   0x200
#define MAX_HEAD_LENGTH     0x10
#define MAX_TAIL_LENGTH     0x01

//
// Number of times to attempt arbitration before giving up.
//
#define MAX_ARB_ATTEMPTS    150000

#define min(l, r)   (((l) < (r)) ? (l) : (r))
