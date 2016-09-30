/*++

Copyright (c) 1993-4  Microsoft Corporation

Module Name:

    atapi.c

Abstract:

    This is the miniport driver for ATAPI IDE controllers.

Author:

    Mike Glass (MGlass)
    Chuck Park (ChuckP)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "miniport.h"
#include "atapi.h"               // includes scsi.h

//
// Device extension
//

typedef struct _HW_DEVICE_EXTENSION {

    //
    // Current request on controller.
    //

    PSCSI_REQUEST_BLOCK CurrentSrb;

    //
    // Base register locations
    //

    PIDE_REGISTERS_1 BaseIoAddress1[2];
    PIDE_REGISTERS_2 BaseIoAddress2[2];

    //
    // Interrupt level
    //

    ULONG InterruptLevel;

    //
    // Data buffer pointer.
    //

    PUSHORT DataBuffer;

    //
    // Data words left.
    //

    ULONG WordsLeft;

    //
    // Number of channels being supported by one instantiation
    // of the device extension. Normally (and correctly) one, but
    // with so many broken PCI IDE controllers being sold, we have
    // to support them.
    //

    ULONG NumberChannels;

    //
    // Identify data for device
    //

    IDENTIFY_DATA FullIdentifyData;
    IDENTIFY_DATA2 IdentifyData[4];

    //
    // Indicates device is present
    //

    BOOLEAN DevicePresent[4];

    //
    // Indicates that ATAPI commands can be used.
    //

    BOOLEAN Atapi[4];

    //
    // Indicates whether device interrupts as DRQ is set after
    // receiving Atapi Packet Command
    //

    BOOLEAN InterruptDRQ[4];

    //
    // Indicates whether this is a tape device.
    //

    BOOLEAN TapeDevice[4];

    //
    // Indicates expecting an interrupt
    //

    BOOLEAN ExpectingInterrupt;

    //
    // Indicate last tape command was DSC Restrictive.
    //

    BOOLEAN RDP;

    //
    // Driver is being used by the crash dump utility or ntldr.
    //

    BOOLEAN DriverMustPoll;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// Logical unit extension
//

typedef struct _HW_LU_EXTENSION {
   ULONG Reserved;
} HW_LU_EXTENSION, *PHW_LU_EXTENSION;


BOOLEAN
IssueIdentify(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG Channel,
    IN UCHAR Command
    )

/*++

Routine Description:

    Issue IDENTIFY command to a device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.
    Command - Either the standard (EC) or the ATAPI packet (A1) IDENTIFY.

Return Value:

    TRUE if all goes well.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1[Channel] ;
    PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2[Channel];
    ULONG                waitCount = 10000;
    ULONG                i,j;
    UCHAR                statusByte;
    UCHAR                signatureLow,
                         signatureHigh;

    //
    // Select device 0 or 1.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)((DeviceNumber << 4) | 0xA0));

    //
    // Check that the status register makes sense.
    //

    GetBaseStatus(baseIoAddress1, statusByte);

    if (Command == IDE_COMMAND_IDENTIFY) {

        //
        // Mask status byte ERROR bits.
        //

        statusByte &= ~(IDE_STATUS_ERROR | IDE_STATUS_INDEX);

        DebugPrint((1,
                    "IssueIdentify: Checking for IDE. Status (%x)\n",
                    statusByte));

        //
        // Check if register value is reasonable.
        //

        if (statusByte != IDE_STATUS_IDLE) {

            //
            // Reset the controller.
            //

            AtapiSoftReset(baseIoAddress1,DeviceNumber);

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                   (UCHAR)((DeviceNumber << 4) | 0xA0));

            WaitOnBusy(baseIoAddress2,statusByte);

            signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
            signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

            if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                //
                // Device is Atapi.
                //

                return FALSE;
            }

            DebugPrint((1,
                        "IssueIdentify: Resetting controller.\n"));

            ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_RESET_CONTROLLER );
            ScsiPortStallExecution(500 * 1000);
            ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);


            do {

                //
                // Wait for Busy to drop.
                //

                ScsiPortStallExecution(100);
                GetStatus(baseIoAddress2, statusByte);

            } while ((statusByte & IDE_STATUS_BUSY) && waitCount--);

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                   (UCHAR)((DeviceNumber << 4) | 0xA0));

            //
            // Another check for signature, to deal with one model Atapi that doesn't assert signature after
            // a soft reset.
            //

            signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
            signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

            if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                //
                // Device is Atapi.
                //

                return FALSE;
            }

            statusByte &= ~IDE_STATUS_INDEX;

            if (statusByte != IDE_STATUS_IDLE) {

                //
                // Give up on this.
                //

                return FALSE;
            }

        }

    } else {

        DebugPrint((1,
                    "IssueIdentify: Checking for ATAPI. Status (%x)\n",
                    statusByte));

    }

    //
    // Load CylinderHigh and CylinderLow with number bytes to transfer.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh, (0x200 >> 8));
    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,  (0x200 & 0xFF));

    for (j = 0; j < 2; j++) {

        //
        // Send IDENTIFY command.
        //

        WaitOnBusy(baseIoAddress2,statusByte);

        ScsiPortWritePortUchar(&baseIoAddress1->Command, Command);

        //
        // Wait for DRQ.
        //

        for (i = 0; i < 4; i++) {

            WaitForDrq(baseIoAddress2, statusByte);

            if (statusByte & IDE_STATUS_DRQ) {

                //
                // Read status to acknowledge any interrupts generated.
                //

                GetBaseStatus(baseIoAddress1, statusByte);

                //
                // One last check for Atapi.
                //


                signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                    //
                    // Device is Atapi.
                    //

                    return FALSE;
                }

                break;
            }

            if (Command == IDE_COMMAND_IDENTIFY) {

                //
                // Check the signature. If DRQ didn't come up it's likely Atapi.
                //

                signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                    //
                    // Device is Atapi.
                    //

                    return FALSE;
                }
            }

            WaitOnBusy(baseIoAddress2,statusByte);
        }

        if (i == 4 && j == 0) {

            //
            // Device didn't respond correctly. It will be given one more chances.
            //

            DebugPrint((1,
                        "IssueIdentify: DRQ never asserted (%x). Error reg (%x)\n",
                        statusByte,
                         ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1)));

            AtapiSoftReset(baseIoAddress1,DeviceNumber);

            GetStatus(baseIoAddress2,statusByte);

            DebugPrint((1,
                       "IssueIdentify: Status after soft reset (%x)\n",
                       statusByte));

        } else {

            break;

        }
    }

    //
    // Check for error on really stupid master devices that assert random
    // patterns of bits in the status register at the slave address.
    //

    if ((Command == IDE_COMMAND_IDENTIFY) && (statusByte & IDE_STATUS_ERROR)) {
        return FALSE;
    }

    DebugPrint((1,
               "IssueIdentify: Status before read words %x\n",
               statusByte));

    //
    // Suck out 256 words. After waiting for one model that asserts busy
    // after receiving the Packet Identify command.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    ReadBuffer(baseIoAddress1,
               (PUSHORT)&deviceExtension->FullIdentifyData,
               256);

    ScsiPortMoveMemory(&deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber],&deviceExtension->FullIdentifyData,sizeof(IDENTIFY_DATA2));

    if (deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber].GeneralConfiguration & 0x20 &&
        Command != IDE_COMMAND_IDENTIFY) {

        //
        // This device interrupts with the assertion of DRQ after receiving
        // Atapi Packet Command
        //

        deviceExtension->InterruptDRQ[(Channel * 2) + DeviceNumber] = TRUE;

        DebugPrint((1,
                    "IssueIdentify: Device interrupts on assertion of DRQ.\n"));

    } else {

        DebugPrint((1,
                    "IssueIdentify: Device does not interrupt on assertion of DRQ.\n"));
    }

    if (((deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber].GeneralConfiguration & 0xF00) == 0x100) &&
        Command != IDE_COMMAND_IDENTIFY) {

        //
        // This is a tape.
        //

        deviceExtension->TapeDevice[(Channel * 2) + DeviceNumber] = TRUE;

        DebugPrint((1,
                    "IssueIdentify: Device is a tape drive.\n"));

    } else {

        DebugPrint((1,
                    "IssueIdentify: Device is not a tape drive.\n"));
    }

    //
    // Work around for some IDE and one model Atapi that will present more than
    // 256 bytes for the Identify data.
    //

    for (i = 0; i < 0x10000; i++) {

        GetStatus(baseIoAddress2,statusByte);

        if (statusByte & IDE_STATUS_DRQ) {

            //
            // Suck out any remaining bytes and throw away.
            //

            ScsiPortReadPortUshort(&baseIoAddress1->Data);

        } else {

            break;

        }
    }

    DebugPrint((1,
               "IssueIdentify: Status after read words (%x)\n",
               statusByte));

    return TRUE;

} // end IssueIdentify()


BOOLEAN
SetDriveParameters(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG Channel
    )

/*++

Routine Description:

    Set drive parameters using the IDENTIFY data.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.

Return Value:

    TRUE if all goes well.


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1[Channel];
    PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2[Channel];
    PIDENTIFY_DATA2      identifyData   = &deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber];
    ULONG i;
    UCHAR statusByte;

    DebugPrint((1,
               "SetDriveParameters: Number of heads %x\n",
               identifyData->NumberOfHeads));

    DebugPrint((1,
               "SetDriveParameters: Sectors per track %x\n",
                identifyData->SectorsPerTrack));

    //
    // Set up registers for SET PARAMETER command.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)((DeviceNumber << 4) |
                           (identifyData->NumberOfHeads - 1)));

    ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                           (UCHAR)identifyData->SectorsPerTrack);

    //
    // Send SET PARAMETER command.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->Command,
                           IDE_COMMAND_SET_DRIVE_PARAMETERS);

    //
    // Wait for up to 30 milliseconds for ERROR or command complete.
    //

    for (i=0; i<30 * 1000; i++) {

        GetStatus(baseIoAddress2, statusByte);

        if (statusByte & IDE_STATUS_ERROR) {
            return FALSE;
        } else if ((statusByte & ~IDE_STATUS_INDEX ) == IDE_STATUS_IDLE) {
            break;
        } else {
            ScsiPortStallExecution(10);
        }
    }

    //
    // Check for timeout.
    //

    if (i == 30 * 1000) {
        return FALSE;
    } else {
        return TRUE;
    }

} // end SetDriveParameters()


BOOLEAN
AtapiResetController(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    )

/*++

Routine Description:

    Reset IDE controller and/or Atapi device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    Nothing.


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG                numberChannels  = deviceExtension->NumberChannels;
    PIDE_REGISTERS_1 baseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2;
    BOOLEAN result = FALSE;
    ULONG i,j;
    UCHAR statusByte;

    DebugPrint((2,"AtapiResetController: Reset IDE\n"));

    //
    // Check if request is in progress.
    //

    if (deviceExtension->CurrentSrb) {

        //
        // Complete outstanding request with SRB_STATUS_BUS_RESET.
        //

        ScsiPortCompleteRequest(deviceExtension,
                                            deviceExtension->CurrentSrb->PathId,
                                            deviceExtension->CurrentSrb->TargetId,
                                            deviceExtension->CurrentSrb->Lun,
                                (ULONG)SRB_STATUS_BUS_RESET);

        //
        // Clear request tracking fields.
        //

        deviceExtension->CurrentSrb = NULL;
        deviceExtension->WordsLeft = 0;
        deviceExtension->DataBuffer = NULL;

        //
        // Indicate ready for next request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);
    }

    //
    // Clear expecting interrupt flag.
    //

    deviceExtension->ExpectingInterrupt = FALSE;


    for (j = 0; j < numberChannels; j++) {

        baseIoAddress1 = deviceExtension->BaseIoAddress1[j];
        baseIoAddress2 = deviceExtension->BaseIoAddress2[j];

        //
        // Do special processing for ATAPI and IDE disk devices.
        //

        for (i = 0; i < 2; i++) {

            //
            // Check if device present.
            //

            if (deviceExtension->DevicePresent[i + (j * 2)]) {

                //
                // Check for ATAPI disk.
                //

                if (deviceExtension->Atapi[i + (j * 2)]) {

                    //
                    // Issue soft reset and issue identify.
                    //

                    GetStatus(baseIoAddress2,statusByte);
                    DebugPrint((1,
                                "AtapiResetController: Status before Atapi reset (%x).\n",
                                statusByte));

                    AtapiSoftReset(baseIoAddress1,i);

                    GetStatus(baseIoAddress2,statusByte);

                    if (statusByte == 0x0) {

                        IssueIdentify(HwDeviceExtension,
                                      i,
                                      j,
                                      IDE_COMMAND_ATAPI_IDENTIFY);
                    } else {

                        DebugPrint((1,
                                   "AtapiResetController: Status after soft reset %x\n",
                                   statusByte));
                    }

                } else {

                    //
                    // Write IDE reset controller bits.
                    //

                    IdeHardReset(baseIoAddress2,result);

                    if (!result) {
                        return FALSE;
                    }

                    //
                    // Set disk geometry parameters.
                    //

                    if (!SetDriveParameters(HwDeviceExtension,
                                            i,
                                            j)) {

                        DebugPrint((1,
                                   "AtapiResetController: SetDriveParameters failed\n"));
                    }
                }
            }
        }
    }

    return TRUE;

} // end AtapiResetController()


ULONG
MapError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine maps ATAPI and IDE errors to specific SRB statuses.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PIDE_REGISTERS_2     baseIoAddress2  = deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
    UCHAR errorByte;
    UCHAR srbStatus;
    UCHAR scsiStatus;

    //
    // Read the error register.
    //

    errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);
    DebugPrint((1,
               "MapError: Error register is %x\n",
               errorByte));

    if (deviceExtension->Atapi[Srb->TargetId]) {

        switch (errorByte >> 4) {
        case SCSI_SENSE_NO_SENSE:

            DebugPrint((1,
                       "ATAPI: No sense information\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_RECOVERED_ERROR:

            DebugPrint((1,
                       "ATAPI: Recovered error\n"));
            scsiStatus = 0;
            srbStatus = SRB_STATUS_SUCCESS;
            break;

        case SCSI_SENSE_NOT_READY:

            DebugPrint((1,
                       "ATAPI: Device not ready\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_MEDIUM_ERROR:

            DebugPrint((1,
                       "ATAPI: Media error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_HARDWARE_ERROR:

            DebugPrint((1,
                       "ATAPI: Hardware error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_ILLEGAL_REQUEST:

            DebugPrint((1,
                       "ATAPI: Illegal request\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_UNIT_ATTENTION:

            DebugPrint((1,
                       "ATAPI: Unit attention\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_DATA_PROTECT:

            DebugPrint((1,
                       "ATAPI: Data protect\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_BLANK_CHECK:

            DebugPrint((1,
                       "ATAPI: Blank check\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        default:

            DebugPrint((1,
                       "ATAPI: Invalid sense information\n"));
            scsiStatus = 0;
            srbStatus = SRB_STATUS_ERROR;
            break;
        }

    } else {

        scsiStatus = 0;

        switch (errorByte & 0x0F) {
        case IDE_ERROR_MEDIA_CHANGE_REQ:

            DebugPrint((1,
                       "ATAPI: Media change\n"));
            srbStatus = SRB_STATUS_ERROR;
            break;

        case IDE_ERROR_COMMAND_ABORTED:

            DebugPrint((1,
                       "ATAPI: Command abort\n"));
            srbStatus = SRB_STATUS_ABORTED;
            break;

        case IDE_ERROR_END_OF_MEDIA:

            DebugPrint((1,
                       "ATAPI: End of media\n"));
            srbStatus = SRB_STATUS_ERROR;
            break;

        case IDE_ERROR_ILLEGAL_LENGTH:

            DebugPrint((1,
                       "ATAPI: Illegal length\n"));
            srbStatus = SRB_STATUS_INVALID_REQUEST;
            break;

        case IDE_ERROR_BAD_BLOCK:

            DebugPrint((1,
                       "ATAPI: Bad block\n"));
            srbStatus = SRB_STATUS_ERROR;
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_MEDIUM_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

            break;

        case IDE_ERROR_ID_NOT_FOUND:
            DebugPrint((1,
                       "ATAPI: Id not found\n"));
            srbStatus = SRB_STATUS_ERROR;
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_MEDIUM_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

            break;

        case IDE_ERROR_DATA_ERROR:
        case IDE_ERROR_MEDIA_CHANGE:

        default:

            DebugPrint((1,
                       "ATAPI: Unknown error\n"));
            srbStatus = SRB_STATUS_ERROR;
            break;
        }
    }

    //
    // Set SCSI status to indicate a check condition.
    //

    Srb->ScsiStatus = scsiStatus;

    return srbStatus;

} // end MapError()


BOOLEAN
AtapiHwInitialize(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE - if initialization successful.
    FALSE - if initialization unsuccessful.

--*/

{
    return TRUE;
} // end AtapiHwInitialize()


BOOLEAN
FindDevices(
    IN PVOID HwDeviceExtension,
    IN BOOLEAN AtapiOnly,
    IN ULONG   Channel
    )

/*++

Routine Description:

    This routine is called from AtapiFindController to identify
    devices attached to an IDE controller.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    AtapiOnly - Indicates that routine should return TRUE only if
        an ATAPI device is attached to the controller.

Return Value:

    TRUE - True if devices found.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1[Channel];
    PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2[Channel];
    BOOLEAN              deviceResponded = FALSE,
                         skipSetParameters = FALSE;
    ULONG                waitCount = 10000;
    ULONG                deviceNumber;
    ULONG                i;
    UCHAR                signatureLow,
                         signatureHigh;
    UCHAR                statusByte;

    //
    // Clear expecting interrupt flag and current SRB field.
    //

    deviceExtension->ExpectingInterrupt = FALSE;
    deviceExtension->CurrentSrb = NULL;

    //
    // Search for devices.
    //

    for (deviceNumber = 0; deviceNumber < 2; deviceNumber++) {

        //
        // Select the device.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                               (UCHAR)((deviceNumber << 4) | 0xA0));

        //
        // Check here for some SCSI adapters that incorporate IDE emulation.
        //

        GetStatus(baseIoAddress2, statusByte);
        if (statusByte == 0xFF) {
            continue;
        }


        signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
        signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

        if (signatureLow == 0x14 && signatureHigh == 0xEB) {

            //
            // ATAPI signature found.
            // Issue the ATAPI identify command if this
            // is not for the crash dump utility.
            //

atapiIssueId:

            if (!deviceExtension->DriverMustPoll) {

                //
                // Issue ATAPI packet identify command.
                //

                if (IssueIdentify(HwDeviceExtension,
                                  deviceNumber,
                                  Channel,
                                  IDE_COMMAND_ATAPI_IDENTIFY)) {

                    //
                    // Indicate ATAPI device.
                    //

                    DebugPrint((1,
                               "AtapiHwInitialize: Device %x is ATAPI\n",
                               deviceNumber));

                    deviceExtension->Atapi[deviceNumber + (Channel * 2)] = TRUE;
                    deviceExtension->DevicePresent[deviceNumber + (Channel * 2)] = TRUE;

                    deviceResponded = TRUE;

                } else {

                    //
                    // Indicate no working device.
                    //

                    DebugPrint((1,
                               "AtapiHwInitialize: Device %x not responding\n",
                               deviceNumber));

                    deviceExtension->DevicePresent[deviceNumber + (Channel * 2)] = FALSE;
                }

            }

        } else {

            //
            // Issue IDE Identify. If an Atapi device is actually present, the signature
            // will be asserted, and the drive will be recognized as such.
            //

            if (IssueIdentify(HwDeviceExtension,
                              deviceNumber,
                              Channel,
                              IDE_COMMAND_IDENTIFY)) {
                //
                // IDE drive found.
                //


                DebugPrint((1,
                           "AtapiHwInitialize: Device %x is IDE\n",
                           deviceNumber));

                deviceExtension->DevicePresent[deviceNumber + (Channel * 2)] = TRUE;

                if (!AtapiOnly) {
                    deviceResponded = TRUE;
                }

                //
                // Indicate IDE - not ATAPI device.
                //

                deviceExtension->Atapi[deviceNumber + (Channel * 2)] = FALSE;


            } else {

                //
                // Look to see if an Atapi device is present.
                //

                AtapiSoftReset(baseIoAddress1,deviceNumber);

                WaitOnBusy(baseIoAddress2,statusByte);

                signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {
                    goto atapiIssueId;
                }
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if (deviceExtension->DevicePresent[i + (Channel * 2)] && !(deviceExtension->Atapi[i + (Channel * 2)]) && deviceResponded) {

            //
            // This hideous hack is to deal with ESDI devices that return
            // garbage geometry in the IDENTIFY data.
            // This is ONLY for the crashdump environment as
            // these are ESDI devices.
            //

            if (deviceExtension->IdentifyData[i].SectorsPerTrack ==
                    0x35 &&
                deviceExtension->IdentifyData[i].NumberOfHeads ==
                    0x07) {

                DebugPrint((1,
                           "AtapiHwInitialize: Found nasty Compaq ESDI!\n"));

                //
                // Change these values to something reasonable.
                //

                deviceExtension->IdentifyData[i].SectorsPerTrack =
                    0x34;
                deviceExtension->IdentifyData[i].NumberOfHeads =
                    0x0E;
            }

            if (deviceExtension->IdentifyData[i].SectorsPerTrack ==
                    0x35 &&
                deviceExtension->IdentifyData[i].NumberOfHeads ==
                    0x0F) {

                DebugPrint((1,
                           "AtapiHwInitialize: Found nasty Compaq ESDI!\n"));

                //
                // Change these values to something reasonable.
                //

                deviceExtension->IdentifyData[i].SectorsPerTrack =
                    0x34;
                deviceExtension->IdentifyData[i].NumberOfHeads =
                    0x0F;
            }


            if (deviceExtension->IdentifyData[i].SectorsPerTrack ==
                    0x36 &&
                deviceExtension->IdentifyData[i].NumberOfHeads ==
                    0x07) {

                DebugPrint((1,
                           "AtapiHwInitialize: Found nasty UltraStor ESDI!\n"));

                //
                // Change these values to something reasonable.
                //

                deviceExtension->IdentifyData[i].SectorsPerTrack =
                    0x3F;
                deviceExtension->IdentifyData[i].NumberOfHeads =
                    0x10;
                skipSetParameters = TRUE;
            }


            if (!skipSetParameters) {

                //
                // Select the device.
                //

                ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                       (UCHAR)((i << 4) | 0xA0));

                GetStatus(baseIoAddress2, statusByte);

                if (statusByte & IDE_STATUS_ERROR) {

                    //
                    // Reset the device.
                    //

                    DebugPrint((1,
                                "FindDevices: Resetting controller before SetDriveParameters.\n"));

                    ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_RESET_CONTROLLER );
                    ScsiPortStallExecution(500 * 1000);
                    ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);

                    do {

                        //
                        // Wait for Busy to drop.
                        //

                        ScsiPortStallExecution(100);
                        GetStatus(baseIoAddress2, statusByte);

                    } while ((statusByte & IDE_STATUS_BUSY) && waitCount--);

                }

                DebugPrint((1,
                            "FindDevices: Status before SetDriveParameters: (%x) \n",
                            statusByte));

                //
                // Use the IDENTIFY data to set drive parameters.
                //

                if (!SetDriveParameters(HwDeviceExtension,i,Channel)) {

                    DebugPrint((1,
                               "AtapHwInitialize: Set drive parameters for device %d failed\n",
                               i));

                    //
                    // Don't use this device as writes could cause corruption.
                    //

                    deviceExtension->DevicePresent[i + Channel] = FALSE;
                    continue;

                }

                //
                // Indicate that a device was found.
                //

                if (!AtapiOnly) {
                    deviceResponded = TRUE;
                }
            }
        }
    }

    //
    // Make sure master device is selected on exit.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect, 0xA0);

    //
    // Reset the controller. This is a feeble attempt to leave the ESDI
    // controllers in a state that ATDISK driver will recognize them.
    // The problem in ATDISK has to do with timings as it is not reproducible
    // in debug. The reset should restore the controller to its poweron state
    // and give the system enough time to settle.
    //

    if (!deviceResponded) {

        ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_RESET_CONTROLLER );
        ScsiPortStallExecution(50 * 1000);
        ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);
    }

    return deviceResponded;

} // end FindDevices()


ULONG
AtapiParseArgumentString(
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

    if (!String) {
        return 0;
    }
    if (!KeyWord) {
        return 0;
    }

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


ULONG
AtapiFindController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
/*++

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - Address of adapter count
    ArgumentString - Used to determine whether driver is client of ntldr or crash dump utility.
    ConfigInfo - Configuration information structure describing HBA
    Again - Indicates search for adapters to continue

Return Value:

    ULONG

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PULONG               adapterCount    = (PULONG)Context;
    PUCHAR               ioSpace;
    BOOLEAN              atapiOnly;
    ULONG                i,j;
    ULONG                irq;
    ULONG                portBase;
    UCHAR                statusByte;
    UCHAR                cdb[12];

    //
    // The following table specifies the ports to be checked when searching for
    // an IDE controller.  A zero entry terminates the search.
    //

    CONST ULONG AdapterAddresses[5] = {0x1F0, 0x170, 0x1e8, 0x168, 0};

    //
    // The following table specifies interrupt levels corresponding to the
    // port addresses in the previous table.
    //

    CONST ULONG InterruptLevels[5] = {14, 15, 11, 10, 0};

    if (!deviceExtension) {
        return SP_RETURN_ERROR;
    }

    //
    // Check to see if this is a special configuration environment.
    //

    portBase = irq = 0;
    if (ArgumentString) {

        irq = AtapiParseArgumentString(ArgumentString, "Interrupt");
        if (irq ) {

            //
            // Both parameters must be present to proceed
            //

            portBase = AtapiParseArgumentString(ArgumentString, "BaseAddress");
            if (!portBase) {

                //
                // Try a default search for the part.
                //

                irq = 0;
            }
        }
    }

    //
    // Scan though the adapter address looking for adapters.
    //

    while (AdapterAddresses[*adapterCount] != 0) {

        for (i = 0; i < 4; i++) {

            //
            // Zero device fields to ensure that if earlier devices were found,
            // but not claimed, the fields are cleared.
            //

            deviceExtension->Atapi[i] = 0;
            deviceExtension->DevicePresent[i] = 0;
            deviceExtension->TapeDevice[i] = 0;
        }

        //
        // Get the system physical address for this IO range.
        //

        if (portBase) {
            ioSpace = ScsiPortGetDeviceBase(HwDeviceExtension,
                                            ConfigInfo->AdapterInterfaceType,
                                            ConfigInfo->SystemIoBusNumber,
                                            ScsiPortConvertUlongToPhysicalAddress(portBase),
                                            8,
                                            TRUE);
        } else {
            ioSpace = ScsiPortGetDeviceBase(HwDeviceExtension,
                                            ConfigInfo->AdapterInterfaceType,
                                            ConfigInfo->SystemIoBusNumber,
                                            ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount]),
                                            8,
                                            TRUE);
        }

        //
        // Update the adapter count.
        //

        (*adapterCount)++;

        //
        // Check if ioSpace accessible.
        //

        if (!ioSpace) {
            continue;
        }

        statusByte = ScsiPortReadPortUchar(&((PATAPI_REGISTERS_2)ioSpace)->AlternateStatus);
        if (statusByte & IDE_STATUS_BUSY) {

            //
            // Could be the TEAC in a thinkpad. Their dos driver puts it in a sleep-mode that
            // warm boots don't clear.
            //

            do {
                ScsiPortStallExecution(1000);
                statusByte = ScsiPortReadPortUchar(&((PATAPI_REGISTERS_1)ioSpace)->Command);
                DebugPrint((3,
                            "AtapiFindController: First access to status %x\n",
                            statusByte));
            } while ((statusByte & IDE_STATUS_BUSY) && ++i < 100);
        }

        //
        // Select master.
        //

        ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->DriveSelect, 0xA0);

        //
        // Check if card at this address.
        //

        ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->CylinderLow, 0xAA);

        //
        // Check if indentifier can be read back.
        //

        if ((statusByte = ScsiPortReadPortUchar(&((PIDE_REGISTERS_1)ioSpace)->CylinderLow)) != 0xAA) {

            DebugPrint((2,
                        "AtapiFindController: Identifier read back from Master (%x)\n",
                        statusByte));

            //
            // Work around Compaq embedded IDE controller strangeness with several devices.
            //

            if (statusByte != 0x01) {

                //
                // Select slave.
                //

                ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->DriveSelect, 0xB0);

                //
                // See if slave is present.
                //

                ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->CylinderLow, 0xAA);

                if ((statusByte = ScsiPortReadPortUchar(&((PIDE_REGISTERS_1)ioSpace)->CylinderLow)) != 0xAA) {

                    DebugPrint((2,
                                "AtapiFindController: Identifier read back from Slave (%x)\n",
                                statusByte));

                    //
                    // Same compaq work-around
                    //

                    if (statusByte != 0x01) {

                        //
                        //
                        // No controller at this base address.
                        //

                        ScsiPortFreeDeviceBase(HwDeviceExtension,
                                               ioSpace);

                        continue;
                    }
                }
            }
        }

        //
        // Record base IO address.
        //

        deviceExtension->BaseIoAddress1[0] = (PIDE_REGISTERS_1)(ioSpace);

        //
        // An adapter has been found request another call.
        //

        *Again = TRUE;

        //
        // Fill in the access array information.
        //

        if (portBase) {
            (*ConfigInfo->AccessRanges)[0].RangeStart = ScsiPortConvertUlongToPhysicalAddress(portBase);
        } else {
            (*ConfigInfo->AccessRanges)[0].RangeStart =
                ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount - 1]);
        }

        (*ConfigInfo->AccessRanges)[0].RangeLength = 8;
        (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

        //
        // Indicate the interrupt level corresponding to this IO range.
        //

        if (irq) {
            ConfigInfo->BusInterruptLevel = irq;
        } else {
            ConfigInfo->BusInterruptLevel = InterruptLevels[*adapterCount - 1];
        }

        if (ConfigInfo->AdapterInterfaceType == MicroChannel) {
            ConfigInfo->InterruptMode = LevelSensitive;
        } else {
            ConfigInfo->InterruptMode = Latched;
        }

        //
        // Get the system physical address for the second IO range.
        //

        if (portBase) {
            ioSpace = ScsiPortGetDeviceBase(HwDeviceExtension,
                                            ConfigInfo->AdapterInterfaceType,
                                            ConfigInfo->SystemIoBusNumber,
                                            ScsiPortConvertUlongToPhysicalAddress(portBase + 0x206),
                                            2,
                                            TRUE);
        } else {
            ioSpace = ScsiPortGetDeviceBase(HwDeviceExtension,
                                            ConfigInfo->AdapterInterfaceType,
                                            ConfigInfo->SystemIoBusNumber,
                                            ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount - 1] + 0x206),
                                            2,
                                            TRUE);
        }

        deviceExtension->BaseIoAddress2[0] = (PIDE_REGISTERS_2)(ioSpace);

        deviceExtension->NumberChannels = 1;

        //
        // Indicate only one bus.
        //

        ConfigInfo->NumberOfBuses = 1;

        //
        // Check for a current version of NT.
        //

        if (ConfigInfo->Length >= sizeof(PORT_CONFIGURATION_INFORMATION)) {

            //
            // Indicate only two devices can be attached to the adapter.
            //

            ConfigInfo->MaximumNumberOfTargets = 2;
        }

        //
        // Indicate maximum transfer length is 64k.
        //

        ConfigInfo->MaximumTransferLength = 0x10000;

        DebugPrint((1,
                   "AtapiFindController: Found IDE at %x\n",
                   deviceExtension->BaseIoAddress1[0]));

        //
        // For Daytona, the atdisk driver gets the first shot at the
        // primary and secondary controllers.
        //

        if (*adapterCount - 1 < 2) {

            //
            // Determine whether this driver is being initialized by the
            // system or as a crash dump driver.
            //

            if (ArgumentString) {

                if (AtapiParseArgumentString(ArgumentString, "dump") == 1) {
                    DebugPrint((1,
                               "AtapiFindController: Crash dump\n"));
                    atapiOnly = FALSE;
                    deviceExtension->DriverMustPoll = TRUE;
                } else {
                    DebugPrint((1,
                               "AtapiFindController: Atapi Only\n"));
                    atapiOnly = TRUE;
                    deviceExtension->DriverMustPoll = FALSE;
                }
            } else {

                DebugPrint((1,
                           "AtapiFindController: Atapi Only\n"));
                atapiOnly = TRUE;
                deviceExtension->DriverMustPoll = FALSE;
            }

        } else {
            atapiOnly = FALSE;
        }

        //
        // Search for devices on this controller.
        //

        if (FindDevices(HwDeviceExtension,
                        atapiOnly,
                        0)) {

            //
            // Claim primary or secondary ATA IO range.
            //

            if (portBase) {
                switch (portBase) {
                case 0x170:
                    ConfigInfo->AtdiskSecondaryClaimed = TRUE;
                    break;
                case 0x1f0:
                    ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                    break;
                default:
                    break;
                }
            } else {
                if (*adapterCount == 1) {
                    ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                } else if (*adapterCount == 2) {
                    ConfigInfo->AtdiskSecondaryClaimed = TRUE;
                }
            }

            return(SP_RETURN_FOUND);
        }
    }

    //
    // The entire table has been searched and no adapters have been found.
    // There is no need to call again and the device base can now be freed.
    // Clear the adapter count for the next bus.
    //

    *Again = FALSE;
    *(adapterCount) = 0;

    return(SP_RETURN_NOT_FOUND);

} // end AtapiFindController()



BOOLEAN
FindBrokenController(
    IN PVOID  DeviceExtension,
    IN PUCHAR VendorID,
    IN ULONG  VendorIDLength,
    IN PUCHAR DeviceID,
    IN ULONG  DeviceIDLength,
    IN OUT PULONG FunctionNumber,
    IN OUT PULONG SlotNumber,
    IN ULONG  BusNumber,
    OUT PBOOLEAN LastSlot
    )

/*++

Routine Description:

    Walk PCI slot information looking for Vendor and Product ID matches.

Arguments:

Return Value:

    TRUE if card found.

--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = DeviceExtension;
    ULONG               rangeNumber = 0;
    ULONG               pciBuffer;
    ULONG               slotNumber;
    ULONG               functionNumber;
    ULONG               status;
    PCI_SLOT_NUMBER     slotData;
    PPCI_COMMON_CONFIG  pciData;
    UCHAR               vendorString[5];
    UCHAR               deviceString[5];

    pciData = (PPCI_COMMON_CONFIG)&pciBuffer;

    slotData.u.AsULONG = 0;

    //
    // Look at each device.
    //

    for (slotNumber = *SlotNumber;
         slotNumber < 32;
         slotNumber++) {

        slotData.u.bits.DeviceNumber = slotNumber;

        //
        // Look at each function.
        //

        for (functionNumber= *FunctionNumber;
             functionNumber < 8;
             functionNumber++) {

            slotData.u.bits.FunctionNumber = functionNumber;

            if (!ScsiPortGetBusData(DeviceExtension,
                                    PCIConfiguration,
                                    BusNumber,
                                    slotData.u.AsULONG,
                                    pciData,
                                    sizeof(ULONG))) {

                //
                // Out of PCI data.
                //

                *LastSlot = TRUE;
                return FALSE;
            }

            if (pciData->VendorID == PCI_INVALID_VENDORID) {

                //
                // No PCI device, or no more functions on device
                // move to next PCI device.
                //

                break;
            }

            //
            // Translate hex ids to strings.
            //

            sprintf(vendorString, "%04x", pciData->VendorID);
            sprintf(deviceString, "%04x", pciData->DeviceID);

            DebugPrint((1,
                       "FindBrokenController: Bus %x Slot %x Function %x Vendor %s Product %s\n",
                       BusNumber,
                       slotNumber,
                       functionNumber,
                       vendorString,
                       deviceString));

            //
            // Compare strings.
            //

            if (strnicmp(vendorString,
                        VendorID,
                        VendorIDLength) ||
                strnicmp(deviceString,
                        DeviceID,
                        DeviceIDLength)) {

                //
                // Not our PCI device. Try next device/function
                //

                continue;
            }

            *FunctionNumber = functionNumber;
            *SlotNumber     = slotNumber;
            return TRUE;

        }   // next PCI function

        *FunctionNumber = 0;

    }   // next PCI slot

    *LastSlot = TRUE;
    return FALSE;
} // end FindBrokenController


ULONG
AtapiFindPCIController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
/*++

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - Address of adapter count
    BusInformation -
    ArgumentString - Used to determine whether driver is client of ntldr or crash dump utility.
    ConfigInfo - Configuration information structure describing HBA
    Again - Indicates search for adapters to continue

Return Value:

    ULONG

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PULONG               adapterCount    = (PULONG)Context;
    ULONG                channel         = 0;
    static ULONG         functionNumber,
                         slotNumber,
                         controllers;
    ULONG                i,j;
    PUCHAR               ioSpace;
    BOOLEAN              atapiOnly,
                         lastSlot,
                         controllerFound = FALSE,
                         deviceFound = FALSE;
    UCHAR                statusByte;

    //
    // The following table specifies the ports to be checked when searching for
    // an IDE controller.  A zero entry terminates the search.
    //

    CONST ULONG AdapterAddresses[5] = {0x1F0, 0x170, 0x1e8, 0x168, 0};

    //
    // The following table specifies interrupt levels corresponding to the
    // port addresses in the previous table.
    //

    CONST ULONG InterruptLevels[5] = {14, 15, 11, 10, 0};

    if (!deviceExtension) {
        return SP_RETURN_ERROR;
    }

    //
    // Gronk PCI config space looking for the broken PCI IDE controllers that have only
    // one FIFO for both channels.
    // Don't do this. It's incorrect and nasty. It has to be done to work around these
    // broken parts, no other reason can justify this.
    //

    for (i = controllers; i < BROKEN_ADAPTERS; i++) {

        //
        // Determine if both channels are enabled and have devices.
        //

        lastSlot = FALSE;

        if (FindBrokenController(deviceExtension,
                                 BrokenAdapters[i].VendorId,
                                 BrokenAdapters[i].VendorIdLength,
                                 BrokenAdapters[i].DeviceId,
                                 BrokenAdapters[i].DeviceIdLength,
                                 &functionNumber,
                                 &slotNumber,
                                 ConfigInfo->SystemIoBusNumber,
                                 &lastSlot)) {

            slotNumber++;
            functionNumber = 0;
            controllerFound = TRUE;

            DebugPrint((1,
                        "Found broken PCI IDE controller: VendorId %s, DeviceId %s\n",
                        BrokenAdapters[i].VendorId,
                        BrokenAdapters[i].DeviceId));

            if (AdapterAddresses[*adapterCount] != 0) {

                for (j = 0; j < 2; j++) {

                    //
                    // Get the system physical address for this IO range.
                    //

                    ioSpace = ScsiPortGetDeviceBase(HwDeviceExtension,
                                                    ConfigInfo->AdapterInterfaceType,
                                                    ConfigInfo->SystemIoBusNumber,
                                                    ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount]),
                                                    8,
                                                    TRUE);

                    //
                    // Update the adapter count.
                    //

                    (*adapterCount)++;

                    //
                    // Check if ioSpace accessible.
                    //

                    if (!ioSpace) {
                        continue;
                    }

                    //
                    // Select master.
                    //

                    ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->DriveSelect, 0xA0);

                    //
                    // Check if card at this address.
                    //

                    ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->CylinderLow, 0xAA);

                    //
                    // Check if indentifier can be read back.
                    //

                    if ((statusByte = ScsiPortReadPortUchar(&((PIDE_REGISTERS_1)ioSpace)->CylinderLow)) != 0xAA) {

                        DebugPrint((2,
                                    "AtapiFindPciController: Identifier read back from Master (%x)\n",
                                    statusByte));


                        //
                        // Select slave.
                        //

                        ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->DriveSelect, 0xB0);

                        //
                        // See if slave is present.
                        //

                        ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->CylinderLow, 0xAA);

                        if ((statusByte = ScsiPortReadPortUchar(&((PIDE_REGISTERS_1)ioSpace)->CylinderLow)) != 0xAA) {

                            DebugPrint((2,
                                        "AtapiFindPciController: Identifier read back from Slave (%x)\n",
                                        statusByte));

                            //
                            //
                            // No controller at this base address.
                            //

                            ScsiPortFreeDeviceBase(HwDeviceExtension,
                                                   ioSpace);

                            //
                            // If the chip is there, but we couldn't find the primary channel, try the secondary.
                            // If we couldn't find a secondary, who cares.
                            //

                            if (j == 1) {

                                goto setStatusAndExit;

                            } else {
                                continue;
                            }
                        }
                    }

                    if (controllerFound) {

                        //
                        // Record base IO address.
                        //

                        deviceExtension->BaseIoAddress1[channel] = (PIDE_REGISTERS_1)(ioSpace);

                        //
                        // Fill in the access array information.
                        //

                        (*ConfigInfo->AccessRanges)[channel].RangeStart =
                                ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount - 1]);

                        (*ConfigInfo->AccessRanges)[channel].RangeLength = 8;
                        (*ConfigInfo->AccessRanges)[channel].RangeInMemory = FALSE;

                        //
                        // Indicate the interrupt level corresponding to this IO range.
                        //

                        if (channel == 0) {
                            ConfigInfo->BusInterruptLevel = InterruptLevels[*adapterCount - 1];
                            ConfigInfo->InterruptMode = Latched;
                        } else {
                            ConfigInfo->BusInterruptLevel2 = InterruptLevels[*adapterCount - 1];
                            ConfigInfo->InterruptMode2 = Latched;
                        }

                        //
                        // Get the system physical address for the second IO range.
                        //

                        ioSpace = ScsiPortGetDeviceBase(HwDeviceExtension,
                                                        ConfigInfo->AdapterInterfaceType,
                                                        ConfigInfo->SystemIoBusNumber,
                                                        ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount - 1] + 0x206),
                                                        2,
                                                        TRUE);

                        deviceExtension->BaseIoAddress2[channel] = (PIDE_REGISTERS_2)(ioSpace);

                        deviceExtension->NumberChannels = 2;

                        //
                        // Indicate only one bus.
                        //

                        ConfigInfo->NumberOfBuses = 1;

                        //
                        // Check for a current version of NT.
                        //

                        if (ConfigInfo->Length >= sizeof(PORT_CONFIGURATION_INFORMATION)) {

                            //
                            // Indicate four devices can be attached to the adapter, since we
                            // have to serialize access to the two channels.
                            //

                            ConfigInfo->MaximumNumberOfTargets = 4;
                        }

                        //
                        // Indicate maximum transfer length is 64k.
                        //

                        ConfigInfo->MaximumTransferLength = 0x10000;

                        DebugPrint((1,
                                   "AtapiFindPciController: Found broken IDE at %x\n",
                                   deviceExtension->BaseIoAddress1[channel]));

                        //
                        // Since we will always pick up this part, and not atdisk, so indicate.
                        //

                        atapiOnly = FALSE;

                        //
                        // Search for devices on this controller.
                        //

                        if (FindDevices(HwDeviceExtension,
                                    atapiOnly,
                                    channel++)){
                            deviceFound = TRUE;
                        }

                        //
                        // Claim primary or secondary ATA IO range.
                        //

                        if (*adapterCount == 1) {
                            ConfigInfo->AtdiskPrimaryClaimed = TRUE;

                        } else if (*adapterCount == 2) {
                            ConfigInfo->AtdiskSecondaryClaimed = TRUE;
                        }
                    }
                }
            }
        }

setStatusAndExit:

        if (lastSlot) {
            slotNumber = 0;
            functionNumber = 0;
        }

        controllers = i;

        if (controllerFound && deviceFound) {

            *Again = TRUE;
            return SP_RETURN_FOUND;
        }
    }


    //
    // The entire table has been searched and no adapters have been found.
    //

    *Again = FALSE;

    return SP_RETURN_NOT_FOUND;

} // end AtapiFindPCIController()



BOOLEAN
AtapiInterrupt(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This is the interrupt service routine for ATAPI IDE miniport driver.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE if expecting an interrupt.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_REQUEST_BLOCK srb              = deviceExtension->CurrentSrb;
    PATAPI_REGISTERS_1 baseIoAddress1;
    PATAPI_REGISTERS_2 baseIoAddress2;
    ULONG wordCount;
    ULONG status;
    ULONG i;
    UCHAR statusByte,interruptReason;
    BOOLEAN commandComplete = FALSE;

    if (srb) {
        baseIoAddress1 =    (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[srb->TargetId >> 1];
        baseIoAddress2 =    (PATAPI_REGISTERS_2)deviceExtension->BaseIoAddress2[srb->TargetId >> 1];
    } else {
        DebugPrint((2,
                    "AtapiInterrupt: CurrentSrb is NULL\n"));

        return FALSE;
    }

    //
    // Clear interrupt by reading status.
    //

    GetBaseStatus(baseIoAddress1, statusByte);

    DebugPrint((2,
                "AtapiInterrupt: Entered with status (%x)\n",
                statusByte));

    if (!(deviceExtension->ExpectingInterrupt)) {

        DebugPrint((3,
                    "AtapiInterrupt: Unexpected interrupt.\n"));
        return FALSE;
    }

    if (statusByte & IDE_STATUS_BUSY) {
        if (deviceExtension->DriverMustPoll) {

            //
            // Crashdump is polling and we got caught with busy asserted.
            // Just go away, and we will be polled again shortly.
            //

            DebugPrint((3,
                        "AtapiInterrupt: Hit BUSY while polling during crashdump.\n"));

            return TRUE;
        }

        //
        // Ensure BUSY is non-asserted.
        //

        for (i = 0; i < 10; i++) {
            GetBaseStatus(baseIoAddress1, statusByte);
            if (!(statusByte & IDE_STATUS_BUSY)) {
                break;
            }
            ScsiPortStallExecution(5000);
        }

        if (i == 10) {

            for (i = 0; i < 10; i++) {
                GetBaseStatus(baseIoAddress1, statusByte);
                if (!(statusByte & IDE_STATUS_BUSY)) {
                    break;
                }
                ScsiPortStallExecution(5000);
            }
            if (i == 10) {
                DebugPrint((1,
                            "AtapiInterrupt: BUSY on entry, resetting. status %x\n",
                            statusByte));

                AtapiResetController(HwDeviceExtension,srb->PathId);
                return TRUE;
            }
        }
    }


    //
    // Check for error conditions.
    //

    if (statusByte & IDE_STATUS_ERROR) {

        if (srb->Cdb[0] != SCSIOP_REQUEST_SENSE) {

            //
            // Fail this request.
            //

            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        }
    }

    //
    // check reason for this interrupt.
    //

    if (deviceExtension->Atapi[srb->TargetId]) {

        interruptReason = (ScsiPortReadPortUchar(&baseIoAddress1->InterruptReason) & 0x3);

    } else {

        if (statusByte & IDE_STATUS_DRQ) {

            if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

                interruptReason =  0x2;

            } else if (srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

                interruptReason = 0x0;

            } else {
                status = SRB_STATUS_ERROR;
                goto CompleteRequest;
            }

        } else if (statusByte & IDE_STATUS_BUSY) {

            return FALSE;

        } else {

            //
            // Command complete - verify.
            //

            interruptReason = 0x3;
        }
    }

    if (interruptReason == 0x1 && (statusByte & IDE_STATUS_DRQ)) {

        //
        // Write the packet.
        //

        DebugPrint((2,
                    "AtapiInterrupt: Writing Atapi packet.\n"));

        //
        // Send CDB to device.
        //

        WriteBuffer(baseIoAddress1,
                    (PUSHORT)srb->Cdb,
                    6);

        return TRUE;

    } else if (interruptReason == 0x0 && (statusByte & IDE_STATUS_DRQ)) {

        //
        // Write the data.
        //

        if (deviceExtension->Atapi[srb->TargetId]) {

            //
            // Pick up bytes to transfer and convert to words.
            //

            wordCount =
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountLow);

            wordCount |=
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountHigh) << 8;

            //
            // Covert bytes to words.
            //

            wordCount >>= 1;

            if (wordCount != deviceExtension->WordsLeft) {
                DebugPrint((3,
                           "AtapiInterrupt: %d words requested; %d words xferred\n",
                           deviceExtension->WordsLeft,
                           wordCount));
            }

            //
            // Verify this makes sense.
            //

            if (wordCount > deviceExtension->WordsLeft) {
                wordCount = deviceExtension->WordsLeft;
            }

        } else {

            //
            // IDE path. Check if words left is at least 256.
            //

            if (deviceExtension->WordsLeft < 256) {

               //
               // Transfer only words requested.
               //

               wordCount = deviceExtension->WordsLeft;

            } else {

               //
               // Transfer next block.
               //

               wordCount = 256;
            }
        }

        //
        // Ensure that this is a write command.
        //

        if (srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

           DebugPrint((3,
                      "AtapiInterrupt: Write interrupt\n"));

           WaitOnBusy(baseIoAddress2,statusByte);

           WriteBuffer(baseIoAddress1,
                      deviceExtension->DataBuffer,
                      wordCount);
        } else {

            DebugPrint((1,
                        "AtapiInterrupt: Int reason %x, but srb is for a write %x.\n",
                        interruptReason,
                        srb));

            //
            // Fail this request.
            //

            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        }


        //
        // Advance data buffer pointer and bytes left.
        //

        deviceExtension->DataBuffer += wordCount;
        deviceExtension->WordsLeft -= wordCount;

        //
        // Check for write command complete.
        //

        if (deviceExtension->WordsLeft == 0) {

            if (!deviceExtension->Atapi[srb->TargetId]) {

                //
                // Completion for IDE drives.
                //

                if (deviceExtension->WordsLeft) {

                    status = SRB_STATUS_DATA_OVERRUN;

                } else {

                    status = SRB_STATUS_SUCCESS;

                }

                goto CompleteRequest;

            }
        }

        return TRUE;

    } else if (interruptReason == 0x2 && (statusByte & IDE_STATUS_DRQ)) {


        if (deviceExtension->Atapi[srb->TargetId]) {

            //
            // Pick up bytes to transfer and convert to words.
            //

            wordCount =
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountLow);

            wordCount |=
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountHigh) << 8;

            //
            // Covert bytes to words.
            //

            wordCount >>= 1;

            if (wordCount != deviceExtension->WordsLeft) {
                DebugPrint((3,
                           "AtapiInterrupt: %d words requested; %d words xferred\n",
                           deviceExtension->WordsLeft,
                           wordCount));
            }

            //
            // Verify this makes sense.
            //

            if (wordCount > deviceExtension->WordsLeft) {
                wordCount = deviceExtension->WordsLeft;
            }

        } else {

            //
            // Check if words left is at least 256.
            //

            if (deviceExtension->WordsLeft < 256) {

               //
               // Transfer only words requested.
               //

               wordCount = deviceExtension->WordsLeft;

            } else {

               //
               // Transfer next block.
               //

               wordCount = 256;
            }
        }

        //
        // Ensure that this is a read command.
        //

        if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

           DebugPrint((3,
                      "AtapiInterrupt: Read interrupt\n"));

           WaitOnBusy(baseIoAddress2,statusByte);

           ReadBuffer(baseIoAddress1,
                      deviceExtension->DataBuffer,
                      wordCount);
        } else {

            DebugPrint((1,
                        "AtapiInterrupt: Int reason %x, but srb is for a read %x.\n",
                        interruptReason,
                        srb));

            //
            // Fail this request.
            //

            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        }


        //
        // Advance data buffer pointer and bytes left.
        //

        deviceExtension->DataBuffer += wordCount;
        deviceExtension->WordsLeft -= wordCount;

        //
        // Check for read command complete.
        //

        if (deviceExtension->WordsLeft == 0) {

            if (deviceExtension->Atapi[srb->TargetId]) {

                //
                // Work around to make many atapi devices return correct sector size
                // of 2048. Also certain devices will have sector count == 0x00, check
                // for that also.
                //

                if (srb->Cdb[0] == 0x25) {
                    deviceExtension->DataBuffer -= wordCount;
                    if (deviceExtension->DataBuffer[0] == 0x00) {

                        *((ULONG *) &(deviceExtension->DataBuffer[0])) = 0xFFFFFF7F;

                    }

                    *((ULONG *) &(deviceExtension->DataBuffer[2])) = 0x00080000;
                    deviceExtension->DataBuffer += wordCount;
                }
            } else {

                //
                // Completion for IDE drives.
                //


                if (deviceExtension->WordsLeft) {

                    status = SRB_STATUS_DATA_OVERRUN;

                } else {

                    status = SRB_STATUS_SUCCESS;

                }

                goto CompleteRequest;

            }
        }

        return TRUE;

    } else if (interruptReason == 0x3  && !(statusByte & IDE_STATUS_DRQ)) {

        //
        // Command complete.
        //

        if (deviceExtension->WordsLeft) {

            status = SRB_STATUS_DATA_OVERRUN;

        } else {

            status = SRB_STATUS_SUCCESS;

        }

CompleteRequest:


        if (status == SRB_STATUS_ERROR) {

            //
            // Map error to specific SRB status and handle request sense.
            //

            status = MapError(deviceExtension,
                              srb);
        }

        //
        // Wait for busy to drop.
        //

        for (i = 0; i < 5000; i++) {
            GetBaseStatus(baseIoAddress1,statusByte);
            if (statusByte & IDE_STATUS_BUSY) {
                ScsiPortStallExecution(10);
            } else {
                break;
            }
        }

        if (i == 5000) {

            //
            // reset the controller.
            //

            DebugPrint((1,
                        "AtapiInterrupt: Resetting due to BSY still up - %x\n",
                        statusByte));
            AtapiResetController(HwDeviceExtension,srb->PathId);
            return TRUE;
        }

        //
        // Check to see if DRQ is still up.
        //

        GetBaseStatus(baseIoAddress1,statusByte);

        if (statusByte & IDE_STATUS_DRQ) {

            for (i = 0; i < 5000; i++) {
                GetBaseStatus(baseIoAddress1,statusByte);
                if (statusByte & IDE_STATUS_DRQ) {
                    ScsiPortStallExecution(100);
                } else {
                    break;
                }
            }

            if (i == 5000) {

                //
                // reset the controller.
                //

                DebugPrint((1,
                            "AtapiInterrupt: Resetting due to DRQ still up - %x\n",
                            statusByte));
                AtapiResetController(HwDeviceExtension,srb->PathId);
                return TRUE;
            }

        }

        //
        // Clear interrupt expecting flag.
        //

        deviceExtension->ExpectingInterrupt = FALSE;

        //
        // Clear current SRB.
        //

        deviceExtension->CurrentSrb = NULL;

        //
        // Sanity check that there is a current request.
        //

        if (srb != NULL) {

            //
            // Set status in SRB.
            //

            srb->SrbStatus = (UCHAR)status;

            //
            // Check for underflow.
            //

            if (deviceExtension->WordsLeft) {

                //
                // Subtract out residual words.
                //

                srb->DataTransferLength -= deviceExtension->WordsLeft;
            }

            //
            // Indicate command complete.
            //

            ScsiPortNotification(RequestComplete,
                                 deviceExtension,
                                 srb);


        } else {

            DebugPrint((1,
                       "AtapiInterrupt: No SRB!\n"));
        }

        //
        // Indicate ready for next request.
        //

        ScsiPortStallExecution(200);

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);


        return TRUE;

    } else {

        //
        // Unexpected int.
        //

        DebugPrint((3,
                    "AtapiInterrupt: Unexpected interrupt. InterruptReason %x. Status %x.\n",
                    interruptReason,
                    statusByte));
        return FALSE;
    }

    return TRUE;

} // end AtapiInterrupt()


ULONG
IdeReadWrite(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine handles IDE read and writes.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PIDE_REGISTERS_2     baseIoAddress2  = deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
    ULONG                startingSector,i;
    UCHAR                statusByte,statusByte2;
    UCHAR                cylinderHigh,cylinderLow,drvSelect,sectorNumber;

    WaitOnBusy(baseIoAddress2,statusByte2);

    if (statusByte2 & IDE_STATUS_BUSY) {
        DebugPrint((1,
                    "IdeReadWrite: Returning BUSY status\n"));
        return SRB_STATUS_BUSY;
    }

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    //
    // Indicate expecting an interrupt.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    //
    // Set up sector count register. Round up to next block.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                           (UCHAR)((Srb->DataTransferLength + 0x1FF) / 0x200));

    //
    // Get starting sector number from CDB.
    //

    startingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

    DebugPrint((2,
               "IdeReadWrite: Starting sector is %x, Number of bytes %x\n",
               startingSector,
               Srb->DataTransferLength));

    //
    // Set up sector number register.
    //

    sectorNumber =  (UCHAR)((startingSector % deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) + 1);
    ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,sectorNumber);

    //
    // Set up cylinder low register.
    //

    cylinderLow =  (UCHAR)(startingSector / (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads));
    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,cylinderLow);

    //
    // Set up cylinder high register.
    //

    cylinderHigh = (UCHAR)((startingSector / (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)) >> 8);
    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,cylinderHigh);

    //
    // Set up head and drive select register.
    //

    drvSelect = (UCHAR)(((startingSector / deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
                      deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads) |((Srb->TargetId & 0x1) << 4) | 0xA0);
    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,drvSelect);

    DebugPrint((2,
               "IdeReadWrite: Cylinder %x Head %x Sector %x\n",
               startingSector /
               (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads),
               (startingSector /
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
               startingSector %
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack + 1));

    //
    // Check if write request.
    //

    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        //
        // Send read command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_READ);
    } else {


        //
        // Send write command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_WRITE);
        //
        // Wait for BSY and DRQ.
        //

        WaitOnBaseBusy(baseIoAddress1,statusByte);

        if (statusByte & IDE_STATUS_BUSY) {

            DebugPrint((1,
                        "IdeReadWrite 2: Returning BUSY status %x\n",
                        statusByte));
            return SRB_STATUS_BUSY;
        }

        for (i = 0; i < 1000; i++) {
            GetBaseStatus(baseIoAddress1, statusByte);
            if (statusByte & IDE_STATUS_DRQ) {
                break;
            }
            ScsiPortStallExecution(200);
        }

        if (!(statusByte & IDE_STATUS_DRQ)) {

            DebugPrint((1,
                       "IdeReadWrite: DRQ never asserted (%x) original status (%x)\n",
                       statusByte,
                       statusByte2));

            deviceExtension->WordsLeft = 0;

            //
            // Clear interrupt expecting flag.
            //

            deviceExtension->ExpectingInterrupt = FALSE;

            //
            // Clear current SRB.
            //

            deviceExtension->CurrentSrb = NULL;

            return SRB_STATUS_TIMEOUT;
        }

        //
        // Write next 256 words.
        //

        WriteBuffer(baseIoAddress1,
                    deviceExtension->DataBuffer,
                    256);

        //
        // Adjust buffer address and words left count.
        //

        deviceExtension->WordsLeft -= 256;
        deviceExtension->DataBuffer += 256;

    }

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeReadWrite()



ULONG
IdeVerify(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine handles IDE Verify.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PIDE_REGISTERS_2     baseIoAddress2  = deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
    ULONG                startingSector;

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    //
    // Indicate expecting an interrupt.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    //
    // Set up sector count register. Round up to next block.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                           (UCHAR)((Srb->DataTransferLength + 0x1FF) / 0x200));

    //
    // Get starting sector number from CDB.
    //

    startingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

    DebugPrint((2,
               "IdeVerify: Starting sector is %x, Number of bytes %x\n",
               startingSector,
               Srb->DataTransferLength));

    //
    // Set up sector number register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,
                           (UCHAR)((startingSector %
                           deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) + 1));

    //
    // Set up cylinder low register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,
                           (UCHAR)(startingSector /
                           (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)));

    //
    // Set up cylinder high register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,
                           (UCHAR)((startingSector /
                           (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)) >> 8));

    //
    // Set up head and drive select register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)(((startingSector /
                           deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads) |
                           ((Srb->TargetId & 0x1) << 4) | 0xA0));

    DebugPrint((2,
               "IdeVerify: Cylinder %x Head %x Sector %x\n",
               startingSector /
               (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads),
               (startingSector /
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
               startingSector %
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack + 1));


    //
    // Send verify command.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->Command,
                           IDE_COMMAND_VERIFY);

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeVerify()


ULONG
AtapiSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Send ATAPI packet command to device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PATAPI_REGISTERS_1   baseIoAddress1  = (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PATAPI_REGISTERS_2   baseIoAddress2 =  (PATAPI_REGISTERS_2)deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
    ULONG i;
    UCHAR statusByte,byteCountLow,byteCountHigh;

    DebugPrint((2,
               "AtapiSendCommand: Command %x to device %d\n",
               Srb->Cdb[0],
               Srb->TargetId));

    //
    // Make sure command is to ATAPI device.
    //

    if ((Srb->Lun != 0) ||
        !deviceExtension->Atapi[Srb->TargetId]) {

        //
        // Indicate no device found at this address.
        //

        return SRB_STATUS_SELECTION_TIMEOUT;
    }


    //
    // Select device 0 or 1.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)(((Srb->TargetId & 0x1) << 4) | 0xA0));

    //
    // Verify that controller is ready for next command.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    DebugPrint((2,
                "AtapiSendCommand: Entered with status %x\n",
                statusByte));

    //
    // If a tape drive has doesn't have DSC set and the last command is restrictive, don't send
    // the next command. See discussion of Restrictive Delayed Process commands in QIC-157.
    //

    if ((!(statusByte & IDE_STATUS_DSC)) && deviceExtension->TapeDevice[Srb->TargetId] && deviceExtension->RDP) {
        ScsiPortStallExecution(1000);
        DebugPrint((1,"AtapiSendCommand: DSC not set. %x\n",statusByte));
        return SRB_STATUS_BUSY;
    }

    if (IS_RDP(Srb->Cdb[0])) {

        deviceExtension->RDP = TRUE;
    } else {

        deviceExtension->RDP = FALSE;
    }

    if (statusByte & IDE_STATUS_DRQ) {

        DebugPrint((1,
                    "AtapiSendCommand: Entered with status (%x). Attempting to recover.\n",
                    statusByte));
        //
        // Try to drain the data that one preliminary device thinks that it has
        // to transfer. Hopefully this random assertion of DRQ will not be present
        // in production devices.
        //

        for (i = 0; i < 0x10000; i++) {

           GetStatus(baseIoAddress2, statusByte);

           if (statusByte & IDE_STATUS_DRQ) {

              ScsiPortReadPortUshort(&baseIoAddress1->Data);

           } else {

              break;
           }
        }

        if (i == 0x10000) {

            DebugPrint((1,
                        "AtapiSendCommand: DRQ still asserted.Status (%x)\n",
                        statusByte));

            AtapiSoftReset(baseIoAddress1,Srb->TargetId);

            DebugPrint((1,
                         "AtapiSendCommand: Issued soft reset to Atapi device. \n"));

            //
            // Re-initialize Atapi device.
            //

            IssueIdentify(HwDeviceExtension,
                          (Srb->TargetId & 0x1),
                          (Srb->TargetId >> 1),
                          IDE_COMMAND_ATAPI_IDENTIFY);

            //
            // Inform the port driver that the bus has been reset.
            //

            ScsiPortNotification(ResetDetected, HwDeviceExtension, 0);

            //
            // Clean up device extension fields that AtapiStartIo won't.
            //

            deviceExtension->ExpectingInterrupt = FALSE;

            return SRB_STATUS_BUS_RESET;

        }
    }

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    WaitOnBusy(baseIoAddress2,statusByte);

    //
    // Write transfer byte count to registers.
    //

    byteCountLow = (UCHAR)(Srb->DataTransferLength & 0xFF);
    byteCountHigh = (UCHAR)(Srb->DataTransferLength >> 8);

    if (Srb->DataTransferLength >= 0x10000) {
        byteCountLow = byteCountHigh = 0xFF;
    }

    ScsiPortWritePortUchar(&baseIoAddress1->ByteCountLow,byteCountLow);
    ScsiPortWritePortUchar(&baseIoAddress1->ByteCountHigh, byteCountHigh);

    //
    // Write 0 to features to ensure PIO.
    //

    ScsiPortWritePortUchar((PUCHAR)baseIoAddress1 + 1,0);


    if (deviceExtension->InterruptDRQ[Srb->TargetId]) {

        //
        // This device interrupts when ready to receive the packet.
        //
        // Write ATAPI packet command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_ATAPI_PACKET);

        DebugPrint((3,
                   "AtapiSendCommand: Wait for int. to send packet. Status (%x)\n",
                   statusByte));

        deviceExtension->ExpectingInterrupt = TRUE;

        return SRB_STATUS_PENDING;

    } else {

        //
        // Write ATAPI packet command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_ATAPI_PACKET);

        //
        // Wait for DRQ.
        //

        WaitOnBusy(baseIoAddress2, statusByte);
        WaitForDrq(baseIoAddress2, statusByte);

        if (!(statusByte & IDE_STATUS_DRQ)) {

            DebugPrint((1,
                       "AtapiSendCommand: DRQ never asserted (%x)\n",
                       statusByte));
            return SRB_STATUS_ERROR;
        }
    }

    //
    // Need to read status register.
    //

    GetBaseStatus(baseIoAddress1, statusByte);

    //
    // Send CDB to device.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    WriteBuffer(baseIoAddress1,
                (PUSHORT)Srb->Cdb,
                6);

    //
    // Indicate expecting an interrupt and wait for it.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    return SRB_STATUS_PENDING;

} // end AtapiSendCommand()

ULONG
IdeSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Program ATA registers for IDE disk transfer.

Arguments:

    HwDeviceExtension - ATAPI driver storage.
    Srb - System request block.

Return Value:

    SRB status (pending if all goes well).

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG status;
    ULONG i;

    DebugPrint((2,
               "IdeSendCommand: Command %x to device %d\n",
               Srb->Cdb[0],
               Srb->TargetId));

    switch (Srb->Cdb[0]) {
    case SCSIOP_INQUIRY:

       //
       // Filter out all TIDs but 0 and 1 since this is an IDE interface
       // which support up to two devices.
       //

       if ((Srb->Lun != 0) ||
           !deviceExtension->DevicePresent[Srb->TargetId]) {

           //
           // Indicate no device found at this address.
           //

           status = SRB_STATUS_SELECTION_TIMEOUT;
           break;

       } else {

           PINQUIRYDATA    inquiryData  = Srb->DataBuffer;
           PIDENTIFY_DATA2 identifyData = &deviceExtension->IdentifyData[Srb->TargetId];

           //
           // Zero INQUIRY data structure.
           //

           for (i = 0; i < Srb->DataTransferLength; i++) {
              ((PUCHAR)Srb->DataBuffer)[i] = 0;
           }

           //
           // Standard IDE interface only supports disks.
           //

           inquiryData->DeviceType = DIRECT_ACCESS_DEVICE;

           //
           // Fill in vendor identification fields.
           //

           for (i = 0; i < 20; i += 2) {
              inquiryData->VendorId[i] =
                  ((PUCHAR)identifyData->ModelNumber)[i + 1];
              inquiryData->VendorId[i+1] =
                  ((PUCHAR)identifyData->ModelNumber)[i];
           }

           //
           // Initialize unused portion of product id.
           //

           for (i = 0; i < 4; i++) {
              inquiryData->ProductId[12+i] = ' ';
           }

           //
           // Move firmware revision from IDENTIFY data to
           // product revision in INQUIRY data.
           //

           for (i = 0; i < 4; i += 2) {
              inquiryData->ProductRevisionLevel[i] =
                  ((PUCHAR)identifyData->FirmwareRevision)[i+1];
              inquiryData->ProductRevisionLevel[i+1] =
                  ((PUCHAR)identifyData->FirmwareRevision)[i];
           }

           status = SRB_STATUS_SUCCESS;
       }

       break;

    case SCSIOP_TEST_UNIT_READY:

        status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_READ_CAPACITY:

        //
        // Claim 512 byte blocks (big-endian).
        //

        ((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock = 0x20000;

        //
        // Calculate last sector.
        //


        i = (deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads *
             deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders *
             deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) - 1;

        ((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress =
            (((PUCHAR)&i)[0] << 24) |  (((PUCHAR)&i)[1] << 16) |
            (((PUCHAR)&i)[2] << 8) | ((PUCHAR)&i)[3];

        DebugPrint((1,
                   "IDE disk %x - #sectors %x, #heads %x, #cylinders %x\n",
                   Srb->TargetId,
                   deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack,
                   deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
                   deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders));


        status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_VERIFY:

            status = IdeVerify(HwDeviceExtension,Srb);
            break;

    case SCSIOP_READ:
    case SCSIOP_WRITE:

        status = IdeReadWrite(HwDeviceExtension,
                              Srb);

        break;

    default:

        DebugPrint((1,
                   "IdeSendCommand: Unsupported command %x\n",
                   Srb->Cdb[0]));

        status = SRB_STATUS_INVALID_REQUEST;

    } // end switch

    return status;

} // end IdeSendCommand()


BOOLEAN
AtapiStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine is called from the SCSI port driver synchronized
    with the kernel to start an IO request.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    TRUE

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG status;

    //
    // Determine which function.
    //

    switch (Srb->Function) {

    case SRB_FUNCTION_EXECUTE_SCSI:

        //
        // Sanity check. Only one request can be outstanding on a
        // controller.
        //

        if (deviceExtension->CurrentSrb) {

            DebugPrint((1,
                       "AtapiStartIo: Already have a request!\n"));
            Srb->SrbStatus = SRB_STATUS_BUSY;
            ScsiPortNotification(RequestComplete,
                                 deviceExtension,
                                 Srb);
            return FALSE;
        }

        //
        // Indicate that a request is active on the controller.
        //

        deviceExtension->CurrentSrb = Srb;

        //
        // Send command to device.
        //

        if (deviceExtension->Atapi[Srb->TargetId]) {

           status = AtapiSendCommand(HwDeviceExtension,
                                     Srb);

        } else if (deviceExtension->DevicePresent[Srb->TargetId]) {

           status = IdeSendCommand(HwDeviceExtension,
                                   Srb);
        } else {

            status = SRB_STATUS_SELECTION_TIMEOUT;
        }

        break;

    case SRB_FUNCTION_ABORT_COMMAND:

        //
        // Verify that SRB to abort is still outstanding.
        //

        if (!deviceExtension->CurrentSrb) {

            DebugPrint((1, "AtapiStartIo: SRB to abort already completed\n"));

            //
            // Complete abort SRB.
            //

            status = SRB_STATUS_ABORT_FAILED;

            break;
        }

        //
        // Abort function indicates that a request timed out.
        // Call reset routine. Card will only be reset if
        // status indicates something is wrong.
        // Fall through to reset code.
        //

    case SRB_FUNCTION_RESET_BUS:

        //
        // Reset Atapi and SCSI bus.
        //

        DebugPrint((1, "AtapiStartIo: Reset bus request received\n"));

        if (!AtapiResetController(deviceExtension,
                             Srb->PathId)) {

              DebugPrint((1,"AtapiStartIo: Reset bus failed\n"));

            //
            // Log reset failure.
            //

            ScsiPortLogError(
                HwDeviceExtension,
                NULL,
                0,
                0,
                0,
                SP_INTERNAL_ADAPTER_ERROR,
                5 << 8
                );

              status = SRB_STATUS_ERROR;

        } else {

              status = SRB_STATUS_SUCCESS;
        }

        break;

    default:

        //
        // Indicate unsupported command.
        //

        status = SRB_STATUS_INVALID_REQUEST;

        break;

    } // end switch

    //
    // Check if command complete.
    //

    if (status != SRB_STATUS_PENDING) {

        DebugPrint((2,
                   "AtapiStartIo: Srb %x complete with status %x\n",
                   Srb,
                   status));

        //
        // Clear current SRB.
        //

        deviceExtension->CurrentSrb = NULL;

        //
        // Set status in SRB.
        //

        Srb->SrbStatus = (UCHAR)status;

        //
        // Indicate command complete.
        //

        ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             Srb);

        //
        // Indicate ready for next request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);
    }

    return TRUE;

} // end AtapiStartIo()


ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )

/*++

Routine Description:

    Installable driver initialization entry point for system.

Arguments:

    Driver Object

Return Value:

    Status from ScsiPortInitialize()

--*/

{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG                  adapterCount;
    ULONG                  i;
    ULONG                  isaStatus,mcaStatus,pciStatus,tmpStatus;

    DebugPrint((1,"\n\nATAPI IDE MiniPort Driver\n"));

    //
    // Zero out structure.
    //

    for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++) {
        ((PUCHAR)&hwInitializationData)[i] = 0;
    }

    //
    // Set size of hwInitializationData.
    //

    hwInitializationData.HwInitializationDataSize =
      sizeof(HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitializationData.HwInitialize = AtapiHwInitialize;
    hwInitializationData.HwResetBus = AtapiResetController;
    hwInitializationData.HwStartIo = AtapiStartIo;
    hwInitializationData.HwInterrupt = AtapiInterrupt;
    hwInitializationData.HwFindAdapter = AtapiFindPCIController;

    //
    // Specify size of extensions.
    //

    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize = sizeof(HW_LU_EXTENSION);

    //
    // Indicate PIO device.
    //

    hwInitializationData.MapBuffers = TRUE;

    //
    // The adapter count is used by the find adapter routine to track how
    // which adapter addresses have been tested.
    //

    adapterCount = 0;

    hwInitializationData.NumberOfAccessRanges = 4;
    hwInitializationData.AdapterInterfaceType = Isa;

    pciStatus = ScsiPortInitialize(DriverObject,
                                   Argument2,
                                   &hwInitializationData,
                                   &adapterCount);


    //
    // Indicate 2 access ranges and reset FindAdapter.
    //

    hwInitializationData.NumberOfAccessRanges = 2;
    hwInitializationData.HwFindAdapter = AtapiFindController;

    //
    // Indicate ISA bustype.
    //

    hwInitializationData.AdapterInterfaceType = Isa;

    //
    // Call initialization for ISA bustype.
    //

    isaStatus =  ScsiPortInitialize(DriverObject,
                                    Argument2,
                                    &hwInitializationData,
                                    &adapterCount);
    //
    // Set up for MCA
    //

    hwInitializationData.AdapterInterfaceType = MicroChannel;
    adapterCount = 0;

    mcaStatus =  ScsiPortInitialize(DriverObject,
                                    Argument2,
                                    &hwInitializationData,
                                    &adapterCount);

    tmpStatus = (pciStatus < isaStatus ? pciStatus : isaStatus);
    return(mcaStatus < tmpStatus ? mcaStatus : tmpStatus);

} // end DriverEntry()


