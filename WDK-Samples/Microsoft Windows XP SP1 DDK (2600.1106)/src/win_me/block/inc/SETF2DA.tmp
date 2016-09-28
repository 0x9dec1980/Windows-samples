/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    scsi.h

Abstract:

    These are the structures and defines that are used in the
    SCSI port and class drivers.

Authors:

    John Freeman (johnfr) 28-Mar-90
    Andre Vachon (andrev) 06-Jun-90
    Mike Glass (mglass)
    Jeff Havens (jhavens)
    Forrest Foltz (forrestf)
    Bill Parry (billpa)

Revision History:

    11-Nov-92	forrestf	Added read capacity CDB
    22-Jan-93  billpa   Added request sense CDB

--*/

#ifndef _SCSI_H_
#define _SCSI_H_ 1

#include <basedef.h>
#include <srb.h>

#pragma pack(1)

//
// Define SCSI maximum configuration parameters.
//

#define SCSI_MAXIMUM_TARGETS 8
#define SCSI_MAXIMUM_LOGICAL_UNITS 8

#define MAXIMUM_CDB_SIZE 12

//
// Command Descriptor Block. Passed by SCSI controller chip over the SCSI bus
//

typedef struct _SUBCHAN_REC {
        UCHAR Reserved0 : 1;
        UCHAR Msf : 1;
        UCHAR Reserved1 : 3;
        UCHAR LogicalUnitNumber : 3;
} SUBCHAN_REC;

typedef struct _SUBCHAN_SUBQ_REC {
        UCHAR Reserved2 : 6;
        UCHAR SubQ : 1;
        UCHAR Reserved3 : 1;
} SUBCHAN_SUBQ_REC;

typedef struct _MODE_SNS_DBD_REC {

        UCHAR Reserved1 : 3;
        UCHAR Dbd : 1;
        UCHAR Reserved2 : 1;
        UCHAR LogicalUnitNumber : 3;
} MODE_SNS_DDB_REC;

typedef struct _MODE_SNS_PC_REC {

        UCHAR PageCode : 6;
        UCHAR Pc : 2;
} MODE_SNS_PC_REC;

typedef struct _MODE_SEL_PF_REC {
        UCHAR Reserved1 : 4;
        UCHAR MS_PF: 1;
        UCHAR MSEL_LogicalUnitNumber : 3;
} MODE_SEL_PF_REC;

typedef union _CDB {

    //
    // Generic 6-Byte CDB
    //

    struct _CDB6GENERIC {
       UCHAR  OperationCode;
       UCHAR  Immediate : 1;
       UCHAR  CommandUniqueBits : 4;
       UCHAR  LogicalUnitNumber : 3;
       UCHAR  CommandUniqueBytes[3];
       UCHAR  Link : 1;
       UCHAR  Flag : 1;
       UCHAR  Reserved : 4;
       UCHAR  VendorUnique : 2;
    } CDB6GENERIC, *PCDB6GENERIC;

    //
    // Standard 6-byte CDB
    //

    struct _CDB6READWRITE {
        UCHAR OperationCode;
        UCHAR LogicalBlockMsb1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR LogicalBlockMsb0;
        UCHAR LogicalBlockLsb;
        UCHAR TransferBlocks;
        UCHAR Control;
    } CDB6READWRITE, *PCDB6READWRITE;


    //
    // SCSI Inquiry CDB
    //

    struct _CDB6INQUIRY {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR PageCode;
        UCHAR IReserved;
        UCHAR AllocationLength;
        UCHAR Control;
    } CDB6INQUIRY, *PCDB6INQUIRY;

    //
    // SCSI Format CDB
    //

    struct _CDB6FORMAT {
        UCHAR OperationCode;
        UCHAR FormatControl : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR FReserved1;
        UCHAR InterleaveMsb;
        UCHAR InterleaveLsb;
        UCHAR FReserved2;
    } CDB6FORMAT, *PCDB6FORMAT;

    //
    // SCSI Send Diagnostic CDB
    //

    struct _CDB6SENDDIAG {
	UCHAR OperationCode;
	UCHAR UnitOfl : 1;	/* Allow medium writes during diagnostics */
	UCHAR DevOfl : 1;	/* Allow diags that may alter all LUNs on this device */
	UCHAR SelfTest : 1;
	UCHAR Reserved1 : 1;
	UCHAR PF : 1;		/* Parameters conform to page standard (SCSI-II) */
	UCHAR LogicalUnitNumber : 3;
	UCHAR Reserved2;
	UCHAR ParamListLenMsb;
	UCHAR ParamListLenLsb;
	UCHAR Control;
    } CDB6SENDDIAG, *PCDB6SENDDIAG;

    //
    // SCSI Receive Diagnostic CDB
    //

    struct _CDB6RECDIAG {
	UCHAR OperationCode;
	UCHAR Reserved1 : 5;
	UCHAR LogicalUnitNumber : 3;
	UCHAR Reserved;
	UCHAR AllocationLenMsb;
	UCHAR AllocationLenLsb;
	UCHAR Control;
    } CDB6RECDIAG, *PCDB6RECDIAG;

    //
    // Standard 10-byte CDB

    struct _CDB10 {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR LogicalBlockByte0;
        UCHAR LogicalBlockByte1;
        UCHAR LogicalBlockByte2;
        UCHAR LogicalBlockByte3;
        UCHAR Reserved2;
        UCHAR TransferBlocksMsb;
        UCHAR TransferBlocksLsb;
        UCHAR Control;
    } CDB10, *PCDB10;

    struct _READ_CAPACITY {
	UCHAR OperationCode;
	UCHAR RelAdr : 1;
	UCHAR Reserved1 : 4;
	UCHAR LogicalUnitNumber : 3;
        UCHAR LogicalBlockByte0;
        UCHAR LogicalBlockByte1;
        UCHAR LogicalBlockByte2;
        UCHAR LogicalBlockByte3;
	UCHAR Reserved2;
	UCHAR Reserved3;
	UCHAR PMI : 1;
	UCHAR Reserved4 : 7;
	UCHAR Control;
    } CDB10READ_CAPACITY, *PCDB10READ_CAPACITY;

    struct _CDB10READWRITE {
	UCHAR OperationCode;
	UCHAR RelAdr : 1;
	UCHAR Reserved1 : 2;
	UCHAR FUA : 1;		/* Force unit access */
	UCHAR DPO : 1;
	UCHAR LogicalUnitNumber : 3;
	UCHAR LogicalBlockByte0;
	UCHAR LogicalBlockByte1;
	UCHAR LogicalBlockByte2;
	UCHAR LogicalBlockByte3;
	UCHAR Reserved2;
	UCHAR TransferBlocksMSB;
	UCHAR TransferBlocksLSB;
	UCHAR Control;
    } CDB10READWRITE, *PCDB10READWRITE;


    //
    // Request Sense CDB
    //

    struct _REQ_SENSE {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR Reserved2;
        UCHAR Reserved3;
        UCHAR TransferLength;
        UCHAR Control;
    } REQ_SENSE, *PREQ_SENSE;

    //
    // CD Rom Audio CDBs
    //

    struct _PAUSE_RESUME {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR Reserved2[6];
        UCHAR Action;
        UCHAR Control;
    } PAUSE_RESUME, *PPAUSE_RESUME;

    //
    // Read Table of Contents
    //

#ifndef _H2INC

    struct _READ_TOC {
        UCHAR OperationCode;
        UCHAR Reserved0 : 1;
        UCHAR Msf : 1;
        UCHAR Reserved1 : 3;
        UCHAR LogicalUnitNumber : 3;
        UCHAR Reserved2[4];
        UCHAR StartingTrack;
        UCHAR AllocationLength[2];
        UCHAR Control : 6;
        UCHAR Format : 2;
    } READ_TOC, *PREAD_TOC;

#else // _H2INC

    struct _READ_TOC {
        UCHAR OperationCode;
        SUBCHAN_REC TOC_rec0;
        UCHAR Reserved2[4];
        UCHAR StartingTrack;
        UCHAR AllocationLength[2];
        UCHAR Control;
    } READ_TOC, *PREAD_TOC;

#endif // _H2INC

	 struct _READ_CD_DA {
		UCHAR OperationCode;
		UCHAR CD_DA_Lun;
		UCHAR LogicalStartAddress[4];
		UCHAR CD_DA_TransferLength[4];
		UCHAR CD_DA_SubCode;
		UCHAR CD_DA_Control;
	 } READ_CD_DA, *PREAD_CD_DA;


    struct _PLAY_AUDIO_MSF {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR Reserved2;
        UCHAR StartingM;
        UCHAR StartingS;
        UCHAR StartingF;
        UCHAR EndingM;
        UCHAR EndingS;
        UCHAR EndingF;
        UCHAR Control;
    } PLAY_AUDIO_MSF, *PPLAY_AUDIO_MSF;

		// play logical block address structure

    struct _PLAY_AUDIO_LBA {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR LbaStart[4];
        UCHAR Extent[4];
        UCHAR LBA_Reserved2;
        UCHAR Control;
    } PLAY_AUDIO_LBA, *PPLAY_AUDIO_LBA;

        // play logical block address structure (10 byte)

    struct _PLAY_LBA10 {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR Lba10_Start[4];
        UCHAR Lba10_Reserved2;
        UCHAR Lba10_Extent[2];
        UCHAR Control;
    } PLAY_LBA10, *PPLAY_LBA10;


    //
    // Read SubChannel Data
    //

#ifdef _H2INC
    struct _SUBCHANNEL {
        UCHAR OperationCode;
        SUBCHAN_REC subq_rec0;
        SUBCHAN_SUBQ_REC subq_rec1;
        UCHAR Format;
        UCHAR SUB_Reserved4[2];
        UCHAR SUB_TrackNumber;
        UCHAR AllocationLength[2];
        UCHAR Control;
    } SUBCHANNEL, *PSUBCHANNEL;

#else //!_H2INC


    struct _SUBCHANNEL {
        UCHAR OperationCode;
        UCHAR Reserved0 : 1;
        UCHAR Msf : 1;
        UCHAR Reserved1 : 3;
        UCHAR LogicalUnitNumber : 3;
        UCHAR Reserved2 : 6;
        UCHAR SubQ : 1;
        UCHAR Reserved3 : 1;
        UCHAR Format;
        UCHAR Reserved4[2];
        UCHAR TrackNumber;
        UCHAR AllocationLength[2];
        UCHAR Control;
    } SUBCHANNEL, *PSUBCHANNEL;

#endif //_H2INC

    //
    // Mode sense
    //

#ifndef _H2INC

     struct _MODE_SENSE {
        UCHAR OperationCode;
        UCHAR Reserved1 : 3;
        UCHAR Dbd : 1;
        UCHAR Reserved2 : 1;
        UCHAR LogicalUnitNumber : 3;
        UCHAR PageCode : 6;
        UCHAR Pc : 2;
        UCHAR Reserved3;
        UCHAR AllocationLength;
        UCHAR Control;
    } MODE_SENSE, *PMODE_SENSE;

#else // _H2INC

   struct _MODE_SENSE {
        UCHAR OperationCode;
        MODE_SNS_DDB_REC md_sns_rec0;
        MODE_SNS_PC_REC md_sns_rec1;
        UCHAR MS_Reserved3;
        UCHAR MS_AllocationLength;
        UCHAR Control;
    } MODE_SENSE, *PMODE_SENSE;

#endif // _H2INC

	struct _MODE_SENSE10 {
		UCHAR OperationCode;
		UCHAR MS10Lun;
		UCHAR PageCode;
		UCHAR MS10Reserved[4];
		UCHAR MS10AllocationLen[2];
		UCHAR MS10Reserved2;
    } MODE_SENSE10, *PMODE_SENSE10;

    //
    // Mode select
    //


#ifdef _H2INC

    struct _MODE_SELECT {
        UCHAR OperationCode;
        MODE_SEL_PF_REC md_sel_rec0;
        UCHAR MSEL_Reserved2[2];
        UCHAR ParameterListLength;
        UCHAR Control;
    } MODE_SELECT, *PMODE_SELECT;

#else // _H2INC

    struct _MODE_SELECT {
        UCHAR OperationCode;
        UCHAR SPBit : 1;
        UCHAR Reserved1 : 3;
        UCHAR PFBit : 1;
        UCHAR LogicalUnitNumber : 3;
        UCHAR Reserved2[2];
        UCHAR ParameterListLength;
        UCHAR Control;
    } MODE_SELECT, *PMODE_SELECT;

#endif // _H2INC

    struct _LOCATE {
        UCHAR OperationCode;
        UCHAR Reserved1 : 1;
        UCHAR CPBit     : 1;
        UCHAR Reserved2 : 6;
        UCHAR Reserved3;
        UCHAR LogicalBlockAddress[4];
        UCHAR Reserved4;
        UCHAR Partition;
        UCHAR Control;
    } LOCATE, *PLOCATE;

    struct _LOGSENSE {
        UCHAR OperationCode;
        UCHAR Unknown1;
        UCHAR PageCode : 6;
        UCHAR PCBit    : 2;
        UCHAR Unknown2;
        UCHAR Unknown3;
        UCHAR ParameterPointer[2];  // [0]=MSB, [1]=LSB
        UCHAR AllocationLength[2];  // [0]=MSB, [1]=LSB
        UCHAR Control;
    } LOGSENSE, *PLOGSENSE;

    struct _PRINT {
        UCHAR OperationCode;
        UCHAR Reserved : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR TransferLength[3];
        UCHAR Control;
    } PRINT, *PPRINT;


#ifdef _H2INC
    struct _SEEK {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR SeekLBA[4];
        UCHAR Reserved2[3];
        UCHAR Control;
    } SEEK, *PSEEK;

#else // _H2INC

    struct _SEEK {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR LogicalBlockAddress[4];
        UCHAR Reserved2[3];
        UCHAR Control;
    } SEEK, *PSEEK;

#endif // _H2INC

    struct _START_STOP {
        UCHAR OperationCode;
        UCHAR Immediate: 1;
        UCHAR Reserved1 : 4;
        UCHAR LogicalUnitNumber : 3;
        UCHAR Reserved2[2];
        UCHAR Start : 1;
        UCHAR LoadEject : 1;
        UCHAR Reserved3 : 6;
        UCHAR Control;
    } START_STOP, *PSTART_STOP;

    struct _MEDIA_REMOVAL {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR Reserved2[2];
        UCHAR Prevent;
        UCHAR Control;
    } MEDIA_REMOVAL, *PMEDIA_REMOVAL;

    //
    // Tape CDBs
    //

    struct _PARTITION {
        UCHAR OperationCode;
        UCHAR Immediate : 1;
        UCHAR Sel: 1;
        UCHAR PartitionSelect : 6;
        UCHAR Reserved1[3];
        UCHAR Control;
    } PARTITION, *PPARTITION;

    struct _WRITE_TAPE_MARKS {
        UCHAR OperationCode;
        UCHAR Immediate : 1;
        UCHAR WriteSetMarks: 1;
        UCHAR Reserved : 3;
        UCHAR LogicalUnitNumber : 3;
        UCHAR TransferLength[3];
        UCHAR Control;
    } WRITE_TAPE_MARKS, *PWRITE_TAPE_MARKS;

    struct _SPACE_TAPE_MARKS {
        UCHAR OperationCode;
        UCHAR Control  : 3;
        UCHAR Reserved : 5;
        UCHAR NumMarksMSB ;
        UCHAR NumMarks;
        UCHAR NumMarksLSB;
        union {
            UCHAR value;
            struct {
                UCHAR Link : 1;
                UCHAR Flag : 1;
                UCHAR Reserved : 4;
                UCHAR VendorUnique : 2;
            } Fields;
        } Byte6;
    } SPACE_TAPE_MARKS, *PSPACE_TAPE_MARKS;

    //
    // Read tape position
    //

    struct _READ_POSITION {
        UCHAR Operation;
        UCHAR BlockType:1;
        UCHAR Reserved1:4;
        UCHAR Lun:3;
        UCHAR Reserved2[7];
        UCHAR Control;
    } READ_POSITION, *PREAD_POSITION;

    //
    // ReadWrite for Tape
    //

    struct _CDB6READWRITETAPE {
        UCHAR OperationCode;
        UCHAR VendorSpecific : 5;
        UCHAR Reserved : 3;
        UCHAR TransferLenMSB;
        UCHAR TransferLen;
        UCHAR TransferLenLSB;
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved1 : 4;
        UCHAR VendorUnique : 2;
     } CDB6READWRITETAPE, *PCDB6READWRITETAPE;

#ifndef _H2INC
} CDB, *PCDB;
#else
} CDB_6, *PCDB;
#endif

//
// Command Descriptor Block constants.
//

#define CDB6GENERIC_LENGTH                   6
#define CDB10GENERIC_LENGTH                  10

#define CDB_6_byte                           6
#define CDB_10_byte                          10

#define SETBITON                             1
#define SETBITOFF                            0
//
// Mode Sense/Select page constants.
//

#define MODE_PAGE_ERROR_RECOVERY        0x01
#define MODE_PAGE_DISCONNECT            0x02
#define MODE_PAGE_FORMAT_DEVICE         0x03
#define MODE_PAGE_RIGID_GEOMETRY        0x04
#define MODE_PAGE_FLEXIBILE             0x05
#define MODE_PAGE_VERIFY_ERROR          0x07
#define MODE_PAGE_CACHING               0x08
#define MODE_PAGE_PERIPHERAL            0x09
#define MODE_PAGE_CONTROL               0x0A
#define MODE_PAGE_MEDIUM_TYPES          0x0B
#define MODE_PAGE_NOTCH_PARTITION       0x0C
#define MODE_SENSE_RETURN_ALL           0x3f
#define MODE_SENSE_CURRENT_VALUES       0x00
#define MODE_SENSE_CHANGEABLE_VALUES    0x40
#define MODE_SENSE_DEFAULT_VAULES       0x80
#define MODE_SENSE_SAVED_VALUES         0xc0
#define MODE_PAGE_DEVICE_CONFIG         0x10
#define MODE_PAGE_MEDIUM_PARTITION      0x11
#define MODE_PAGE_CDVD_CAPS				0X2A

//
// SCSI CDB operation codes
//

#define SCSIOP_TEST_UNIT_READY     0x00
#define SCSIOP_REZERO_UNIT         0x01
#define SCSIOP_REWIND              0x01
#define SCSIOP_REQUEST_BLOCK_ADDR  0x02
#define SCSIOP_REQUEST_SENSE       0x03
#define SCSIOP_FORMAT_UNIT         0x04
#define SCSIOP_READ_BLOCK_LIMITS   0x05
#define SCSIOP_REASSIGN_BLOCKS     0x07
#define SCSIOP_READ6               0x08
#define SCSIOP_RECEIVE             0x08
#define SCSIOP_WRITE6              0x0A
#define SCSIOP_PRINT               0x0A
#define SCSIOP_SEND                0x0A
#define SCSIOP_SEEK6               0x0B
#define SCSIOP_TRACK_SELECT        0x0B
#define SCSIOP_SLEW_PRINT          0x0B
#define SCSIOP_SEEK_BLOCK          0x0C
#define SCSIOP_PARTITION           0x0D
#define SCSIOP_READ_REVERSE        0x0F
#define SCSIOP_WRITE_FILEMARKS     0x10
#define SCSIOP_FLUSH_BUFFER        0x10
#define SCSIOP_SPACE               0x11
#define SCSIOP_INQUIRY             0x12
#define SCSIOP_VERIFY6             0x13
#define SCSIOP_RECOVER_BUF_DATA    0x14
#define SCSIOP_MODE_SELECT         0x15
#define SCSIOP_RESERVE_UNIT        0x16
#define SCSIOP_RELEASE_UNIT        0x17
#define SCSIOP_COPY                0x18
#define SCSIOP_ERASE               0x19
#define SCSIOP_MODE_SENSE          0x1A
#define SCSIOP_START_STOP_UNIT     0x1B
#define SCSIOP_STOP_PRINT          0x1B
#define SCSIOP_LOAD_UNLOAD         0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC  0x1C
#define SCSIOP_SEND_DIAGNOSTIC     0x1D
#define SCSIOP_MEDIUM_REMOVAL      0x1E
#define SCSIOP_READ_CAPACITY       0x25
#define SCSIOP_READ                0x28
#define SCSIOP_WRITE               0x2A
#define SCSIOP_SEEK                0x2B
#define SCSIOP_LOCATE              0x2B
#define SCSIOP_WRITE_VERIFY        0x2E
#define SCSIOP_VERIFY              0x2F
#define SCSIOP_SEARCH_DATA_HIGH    0x30
#define SCSIOP_SEARCH_DATA_EQUAL   0x31
#define SCSIOP_SEARCH_DATA_LOW     0x32
#define SCSIOP_SET_LIMITS          0x33
#define SCSIOP_READ_POSITION       0x34
#define SCSIOP_COMPARE             0x39
#define SCSIOP_COPY_COMPARE        0x3A
#define SCSIOP_WRITE_DATA_BUFF     0x3B
#define SCSIOP_READ_DATA_BUFF      0x3C
#define SCSIOP_READ_LONG 		     0x3E
#define SCSIOP_CHANGE_DEFINITION   0x40
#define SCSIOP_READ_SUB_CHANNEL    0x42
#define SCSIOP_READ_TOC            0x43
#define SCSIOP_READ_HEADER         0x44
#define SCSIOP_PLAY_AUDIO_10       0x45
#define SCSIOP_PLAY_AUDIO_MSF      0x47
#define SCSIOP_PLAY_TRACK_INDEX    0x48
#define SCSIOP_PLAY_TRACK_RELATIVE 0x49
#define SCSIOP_PAUSE_RESUME        0x4B
#define SCSIOP_LOG_SELECT          0x4C
#define SCSIOP_LOG_SENSE           0x4D
#define SCSIOP_MODE_SELECT10	   0x55
#define SCSIOP_MODE_SENSE10		   0x5A
#define SCSIOP_PLAY_AUDIO          0xA5
#define SCSIOP_READ12			   0xA8
#define SCSIOP_READCD			   0xBE

#define	SCSIOP_READ_CDDA	   0xD8

//
// If the IMMED bit is 1, status is returned as soon
// as the operation is initiated. If the IMMED bit
// is 0, status is not returned until the operation
// is completed.
//

#define CDB_RETURN_ON_COMPLETION   0
#define CDB_RETURN_IMMEDIATE       1

//
// CDB Force media access used in extended read and write commands.
//

#define CDB_FORCE_MEDIA_ACCESS 0x08

//
// Denon CD ROM operation codes
//

#define SCSIOP_DENON_EJECT_DISC    0xE6
#define SCSIOP_DENON_STOP_AUDIO    0xE7
#define SCSIOP_DENON_PLAY_AUDIO    0xE8
#define SCSIOP_DENON_READ_TOC      0xE9
#define SCSIOP_DENON_READ_SUBCODE  0xEB

//
// SCSI Bus Messages
//

#define SCSIMESS_ABORT                0x06
#define SCSIMESS_ABORT_WITH_TAG       0x0D
#define SCSIMESS_BUS_DEVICE_RESET     0X0C
#define SCSIMESS_CLEAR_QUEUE          0X0E
#define SCSIMESS_COMMAND_COMPLETE     0X00
#define SCSIMESS_DISCONNECT           0X04
#define SCSIMESS_EXTENDED_MESSAGE     0X01
#define SCSIMESS_IDENTIFY             0X80
#define SCSIMESS_IDENTIFY_WITH_DISCON 0XC0
#define SCSIMESS_IGNORE_WIDE_RESIDUE  0X23
#define SCSIMESS_INITIATE_RECOVERY    0X0F
#define SCSIMESS_INIT_DETECTED_ERROR  0X05
#define SCSIMESS_LINK_CMD_COMP        0X0A
#define SCSIMESS_LINK_CMD_COMP_W_FLAG 0X0B
#define SCSIMESS_MESS_PARITY_ERROR    0X09
#define SCSIMESS_MESSAGE_REJECT       0X07
#define SCSIMESS_NO_OPERATION         0X08
#define SCSIMESS_HEAD_OF_QUEUE_TAG    0X21
#define SCSIMESS_ORDERED_QUEUE_TAG    0X22
#define SCSIMESS_SIMPLE_QUEUE_TAG     0X20
#define SCSIMESS_RELEASE_RECOVERY     0X10
#define SCSIMESS_RESTORE_POINTERS     0X03
#define SCSIMESS_SAVE_DATA_POINTER    0X02
#define SCSIMESS_TERMINATE_IO_PROCESS 0X11

//
// SCSI Extended Message operation codes
//

#define SCSIMESS_MODIFY_DATA_POINTER  0X00
#define SCSIMESS_SYNCHRONOUS_DATA_REQ 0X01
#define SCSIMESS_WIDE_DATA_REQUEST    0X03

//
// SCSI Extended Message Lengths
//

#define SCSIMESS_MODIFY_DATA_LENGTH   5
#define SCSIMESS_SYNCH_DATA_LENGTH    3
#define SCSIMESS_WIDE_DATA_LENGTH     2

//
// SCSI extended message structure
//

typedef struct _SCSI_EXTENDED_MESSAGE {
    UCHAR InitialMessageCode;
    UCHAR MessageLength;
    UCHAR MessageType;
    union _EXTENDED_ARGUMENTS {

        struct {
            UCHAR Modifier[4];
        } Modify;

        struct {
            UCHAR TransferPeriod;
            UCHAR ReqAckOffset;
        } Synchronous;

        struct{
            UCHAR Width;
        } Wide;
    }ExtendedArguments;
}SCSI_EXTENDED_MESSAGE, *PSCSI_EXTENDED_MESSAGE;

//
// SCSI bus status codes.
//

#define SCSISTAT_GOOD                  0x00
#define SCSISTAT_CHECK_CONDITION       0x02
#define SCSISTAT_CONDITION_MET         0x04
#define SCSISTAT_BUSY                  0x08
#define SCSISTAT_INTERMEDIATE          0x10
#define SCSISTAT_INTERMEDIATE_COND_MET 0x14
#define SCSISTAT_RESERVATION_CONFLICT  0x18
#define SCSISTAT_COMMAND_TERMINATED    0x22
#define SCSISTAT_QUEUE_FULL            0x28

//
// Enable Vital Product Data Flag (EVPD)
// used with INQUIRY command.
//

#define CDB_INQUIRY_EVPD           0x01

//
// Defines for format CDB
//

#define LUN0_FORMAT_SAVING_DEFECT_LIST 0
#define USE_DEFAULTMSB  0
#define USE_DEFAULTLSB  0

#define START_UNIT_CODE 0x01
#define STOP_UNIT_CODE  0x00

//
// Inquiry buffer structure. This is the data returned from the target
// after it receives an inquiry.
//
// This structure may be extended by the number of bytes specified
// in the field AdditionalLength. The defined size constant only
// includes fields through ProductRevisionLevel.
//
// The NT SCSI drivers are only interested in the first 36 bytes of data.
//

#define INQUIRYDATABUFFERSIZE 36

#ifdef _H2INC

typedef struct _INQUIRYDATA {
    UCHAR DeviceType;
    UCHAR DeviceTypeModifier;
    UCHAR Versions;
    UCHAR ResponseDataFormat;
    UCHAR AdditionalLength;
    UCHAR Reserved[2];
    UCHAR SoftReset : 1;
    UCHAR CommandQueue : 1;
    UCHAR Reserved2 : 1;
    UCHAR LinkedCommands : 1;
    UCHAR Synchronous : 1;
    UCHAR Wide16Bit : 1;
    UCHAR Wide32Bit : 1;
    UCHAR RelativeAddressing : 1;
    UCHAR VendorId[8];
    UCHAR ProductId[16];
    UCHAR ProductRevisionLevel[4];
    UCHAR VendorSpecific[20];
    UCHAR Reserved3[40];
} INQUIRYDATA, *PINQUIRYDATA;

#else

typedef struct _INQUIRYDATA {
    UCHAR DeviceType : 5;
    UCHAR DeviceTypeQualifier : 3;
    UCHAR DeviceTypeModifier : 7;
    UCHAR RemovableMedia : 1;
    UCHAR Versions;
    UCHAR ResponseDataFormat;
    UCHAR AdditionalLength;
    UCHAR Reserved[2];
    UCHAR SoftReset : 1;
    UCHAR CommandQueue : 1;
    UCHAR Reserved2 : 1;
    UCHAR LinkedCommands : 1;
    UCHAR Synchronous : 1;
    UCHAR Wide16Bit : 1;
    UCHAR Wide32Bit : 1;
    UCHAR RelativeAddressing : 1;
    UCHAR VendorId[8];
    UCHAR ProductId[16];
    UCHAR ProductRevisionLevel[4];
    UCHAR VendorSpecific[20];
    UCHAR Reserved3[40];
} INQUIRYDATA, *PINQUIRYDATA;

#endif // _H2INC

//
// Read capacity buffer structure.  This is the data returned from the target
// after it receives an inquiry.
//

typedef struct _READCAPACITYDATA {
    UCHAR BlockCountByte0;
    UCHAR BlockCountByte1;
    UCHAR BlockCountByte2;
    UCHAR BlockCountByte3;
    UCHAR BlockLengthByte0;
    UCHAR BlockLengthByte1;
    UCHAR BlockLengthByte2;
    UCHAR BlockLengthByte3;
} READCAPACITYDATA, *PREADCAPACITYDATA;


//
// Definitions for diagnostic pages
//

//
// Definition for Rigid Disk Drive Geometry Page
//

typedef struct _DIAGPAGE4 {
    UCHAR PageCode  : 6;
    UCHAR Reserved1 : 1;
    UCHAR PS : 1;
    UCHAR NumCylindersMSB;
    UCHAR NumCylindersLSB;
    UCHAR NumHeads;
    UCHAR StartingPreCompMSB;
    UCHAR StartingPreCompLSB;
    UCHAR StartingCylRedWrCurrentMSB;
    UCHAR StartingCylRedWrCurrentLSB;
    UCHAR StepRateMSB;
    UCHAR StepRateLSB;
    UCHAR LandingZoneMSB;
    UCHAR LandingZoneLSB;
    UCHAR RPL : 2;		/* Spindle synch */
    UCHAR Reserved2 : 6;
    UCHAR RotationalOffset;
    UCHAR Reserved;
    UCHAR RotationRateMSB;
    UCHAR RotationRateLSB;
    UCHAR Reserved3;
    UCHAR Reserved4;
} DIAGPAGE4, *PDIAGPAGE4;



#define MAXIMUM_NUMBER_LUNS 8

// Inquiry defines. Used to interpret data returned from target as result
// of inquiry command.
//
// DeviceType field
//

#define DIRECT_ACCESS_DEVICE            0x00    // disks
#define SEQUENTIAL_ACCESS_DEVICE        0x01    // tapes
#define PRINTER_DEVICE                  0x02
#define PROCESSOR_DEVICE                0x03
#define WRITE_ONCE_READ_MULTIPLE_DEVICE 0x04
#define READ_ONLY_DIRECT_ACCESS_DEVICE  0x05    // CD_ROM
#define SCANNER_DEVICE                  0x06
#define LOGICAL_UNIT_NOT_PRESENT_DEVICE 0x7F

//
// DeviceTypeQualifier field
//

#define REMOVABLE_MASK  0x80

#define NOT_REMOVABLE   0x0         // disks
#define REMOVABLE       0x80        // CD-ROM

//
// Sense Data Format
//

#ifdef _H2INC

typedef struct _SENSE_DATA {
    UCHAR ErrorCode;
    UCHAR SegmentNumber;
    UCHAR SenseKey:4;
    UCHAR Reserved:1;
    UCHAR IncorrectLength:1;
    UCHAR EndOfMedia:1;
    UCHAR FileMark:1;
    UCHAR Information[4];
    UCHAR AdditionalSenseLength;
    UCHAR CommandSpecificInformation[4];
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR FieldReplaceableUnitCode;
    UCHAR SenseKeySpecific[3];
} SENSE_DATA, *PSENSE_DATA;

#else // !_H2INC

typedef struct _SENSE_DATA {
    UCHAR ErrorCode:7;
	 UCHAR Valid:1;
    UCHAR SegmentNumber;
    UCHAR SenseKey:4;
    UCHAR Reserved:1;
    UCHAR IncorrectLength:1;
    UCHAR EndOfMedia:1;
    UCHAR FileMark:1;
    UCHAR Information[4];
    UCHAR AdditionalSenseLength;
    UCHAR CommandSpecificInformation[4];
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR FieldReplaceableUnitCode;
    UCHAR SenseKeySpecific[3];
} SENSE_DATA, *PSENSE_DATA;

#endif

//
// Default request sense buffer size
//

#define SENSE_BUFFER_SIZE 18

//
// Sense codes
//

#define SCSI_SENSE_NO_SENSE         0x00
#define SCSI_SENSE_RECOVERED_ERROR  0x01
#define SCSI_SENSE_NOT_READY        0x02
#define SCSI_SENSE_MEDIUM_ERROR     0x03
#define SCSI_SENSE_HARDWARE_ERROR   0x04
#define SCSI_SENSE_ILLEGAL_REQUEST  0x05
#define SCSI_SENSE_UNIT_ATTENTION   0x06
#define SCSI_SENSE_DATA_PROTECT     0x07
#define SCSI_SENSE_BLANK_CHECK      0x08
#define SCSI_SENSE_UNIQUE           0x09
#define SCSI_SENSE_COPY_ABORTED     0x0A
#define SCSI_SENSE_ABORTED_COMMAND  0x0B
#define SCSI_SENSE_EQUAL            0x0C
#define SCSI_SENSE_VOL_OVERFLOW     0x0D
#define SCSI_SENSE_MISCOMPARE       0x0E
#define SCSI_SENSE_RESERVED         0x0F

//
// Additional tape bit
//

#define SCSI_ILLEGAL_LENGTH         0x20
#define SCSI_EOM                    0x40
#define SCSI_FILE_MARK              0x80

//
// Additional Sense codes
//

#define SCSI_ADSENSE_NO_SENSE       0x00
#define SCSI_ADSENSE_MAN_INTERV     0x03
#define SCSI_ADSENSE_LUN_NOT_READY  0x04
#define SCSI_ADSENSE_ILLEGAL_COMMAND 0x20
#define SCSI_ADSENSE_ILLEGAL_BLOCK  0x21
#define SCSI_ADSENSE_INVALID_LUN    0x25
#define SCSI_ADSENSE_SELECT_TIMEOUT 0x45
#define SCSI_ADSENSE_TIMEOUT		0x5E
#define SCSI_ADSENSE_MUSIC_AREA     0xA0
#define SCSI_ADSENSE_DATA_AREA      0xA1
#define SCSI_ADSENSE_VOLUME_OVERFLOW 0xA7

#define SCSI_ADSENSE_NO_MEDIA_IN_DEVICE 0x3a
#define SCSI_ADWRITE_PROTECT        0x27
#define SCSI_ADSENSE_MEDIUM_CHANGED 0x28
#define SCSI_ADSENSE_BUS_RESET      0x29
#define SCSI_ADSENSE_TRACK_ERROR    0x14
#define SCSI_ADSENSE_SEEK_ERROR     0x15
#define SCSI_ADSENSE_REC_DATA_NOECC 0x17
#define SCSI_ADSENSE_REC_DATA_ECC   0x18
#define SCSI_ADSENSE_ILLEGAL_MODE   0x64
#define SCSI_ADSENSE_BAD_CDB        0x24
#define SCSI_ADSENSE_BAD_PARM_LIST  0x26
#define SCSI_ADSENSE_CANNOT_READ_MEDIUM  0x30

//
// Additional sense code qualifier
//

#define SCSI_SENSEQ_FORMAT_IN_PROGRESS 0x04
#define SCSI_SENSEQ_INIT_COMMAND_REQUIRED 0x02
#define SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED 0x03
#define SCSI_SENSEQ_BECOMING_READY 0x01
#define SCSI_SENSEQ_INCOMP_FORMAT  0x02

//
// SCSI IO Device Control Codes
//

#define FILE_DEVICE_SCSI 0x0000001b

#define IOCTL_SCSI_EXECUTE_IN   ((FILE_DEVICE_SCSI << 16) + 0x0011)
#define IOCTL_SCSI_EXECUTE_OUT  ((FILE_DEVICE_SCSI << 16) + 0x0012)
#define IOCTL_SCSI_EXECUTE_NONE ((FILE_DEVICE_SCSI << 16) + 0x0013)

//
// Read Capacity Data - returned in Big Endian format
//

typedef struct _READ_CAPACITY_DATA {
    ULONG LogicalBlockAddress;
    ULONG BytesPerBlock;
} READ_CAPACITY_DATA, *PREAD_CAPACITY_DATA;


//
// Read Block Limits Data - returned in Big Endian format
// This structure returns the maximum and minimum block
// size for a TAPE device.
//

typedef struct _READ_BLOCK_LIMITS {
    UCHAR Reserved;
    UCHAR BlockMaximumSize[3];
    UCHAR BlockMinimumSize[2];
} READ_BLOCK_LIMITS_DATA, *PREAD_BLOCK_LIMITS_DATA;

//
// Mode data structures.
//

//
// Define Mode parameter header.
//

typedef struct _MODE_PARAMETER_HEADER {
    UCHAR ModeDataLength;
    UCHAR MediumType;
    UCHAR DeviceSpecificParameter;
    UCHAR BlockDescriptorLength;
}MODE_PARAMETER_HEADER, *PMODE_PARAMETER_HEADER;

#define MODE_FD_SINGLE_SIDE     0x01
#define MODE_FD_DOUBLE_SIDE     0x02
#define MODE_FD_MAXIMUM_TYPE    0x1E

typedef struct _MODE10_PARAMETER_HEADER {
	UCHAR Mode10DataLength[2];
	UCHAR Mode10MediumType;
	UCHAR Reserved[3];
	UCHAR Mode10BlockDescriptorLength[2];
}MODE10_PARAMETER_HEADER, *PMODE10_PARAMETER_HEADER;

//
// Define the mode parameter block.
//

typedef struct _MODE_PARAMETER_BLOCK {
    UCHAR DensityCode;
    UCHAR NumberOfBlocks[3];
    UCHAR Reserved;
    UCHAR BlockLength[3];
}MODE_PARAMETER_BLOCK, *PMODE_PARAMETER_BLOCK;

//
// Define the Error Recovery Page
//

typedef struct _MODE_ERROR_RECOVERY_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
	 UCHAR ErrorRecoveryParam;
	 UCHAR ReadRetry;
	 UCHAR MERP_Reserved[4];
} MODE_ERROR_RECOVERY_PAGE, *PMODE_ERROR_RECOVERY_PAGE;


//
// Define Disconnect-Reconnect page.
//

typedef struct _MODE_DISCONNECT_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    UCHAR BufferFullRatio;
    UCHAR BufferEmptyRatio;
    UCHAR BusInactivityLimit[2];
    UCHAR BusDisconnectTime[2];
    UCHAR BusConnectTime[2];
    UCHAR MaximumBurstSize[2];
    UCHAR DataTransferDisconnect : 2;
    UCHAR Reserved2[3];
}MODE_DISCONNECT_PAGE, *PMODE_DISCONNECT_PAGE;

//
// Define mode caching page.
//

typedef struct _MODE_CACHING_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    UCHAR ReadDisableCache : 1;
    UCHAR MultiplicationFactor : 1;
    UCHAR WriteCacheEnable : 1;
    UCHAR Reserved2 : 5;
    UCHAR WriteRetensionPriority : 4;
    UCHAR ReadRetensionPriority : 4;
    UCHAR DisablePrefetchTransfer[2];
    UCHAR MinimumPrefectch[2];
    UCHAR MaximumPrefectch[2];
    UCHAR MaximumPrefectchCeil[2];
}MODE_CACHING_PAGE, *PMODE_CACHING_PAGE;

//
// Define mode flexible disk page.
//

typedef struct _MODE_FLEXIBLE_DISK_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    UCHAR TransferRate[2];
    UCHAR NumberOfHeads;
    UCHAR SectorsPerTrack;
    UCHAR BytesPerSector[2];
    UCHAR NumberOfCylinders[2];
    UCHAR StartWritePrecom[2];
    UCHAR StartReducedCurrent[2];
    UCHAR StepRate[2];
    UCHAR StepPluseWidth;
    UCHAR HeadSettleDelay[2];
    UCHAR MotorOnDelay;
    UCHAR MotorOffDelay;
    UCHAR Reserved2 : 5;
    UCHAR MotorOnAsserted : 1;
    UCHAR StartSectorNumber : 1;
    UCHAR TrueReadySignal : 1;
    UCHAR StepPlusePerCyclynder : 4;
    UCHAR Reserved3 : 4;
    UCHAR WriteCompenstation;
    UCHAR HeadLoadDelay;
    UCHAR HeadUnloadDelay;
    UCHAR Pin2Usage : 4;
    UCHAR Pin34Usage : 4;
    UCHAR Pin1Usage : 4;
    UCHAR Pin4Usage : 4;
    UCHAR MediumRotationRate[2];
    UCHAR Reserved4[2];
}MODE_FLEXIBLE_DISK_PAGE, *PMODE_FLEXIBLE_DISK_PAGE;

//
// Define mode format page.
//

typedef struct _MODE_FORMAT_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    UCHAR TracksPerZone[2];
    UCHAR AlternetSectorsPerZone[2];
    UCHAR AlternetTracksPerZone[2];
    UCHAR SectorsPerTrack[2];
    UCHAR BytesPerPhysicalSector[2];
    UCHAR Interleave[2];
    UCHAR TrackSkewFactor[2];
    UCHAR CylinderSkewFactor[2];
    UCHAR Reserved2 : 4;
    UCHAR SurfaceFirst : 1;
    UCHAR RemovableMedia : 1;
    UCHAR HardSectorFormating : 1;
    UCHAR SoftSectorFormating : 1;
    UCHAR Reserved3[2];
}MODE_FORMAT_PAGE, *PMODE_FORMAT_PAGE;

//
// Define rigid disk driver geometry page.
//

typedef struct _MODE_RIGID_GEOMETRY_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    UCHAR NumberOfCylinders[2];
    UCHAR NumberOfHeads;
    UCHAR StartWritePrecom[2];
    UCHAR StartReducedCurrent[2];
    UCHAR DriveStepRate[2];
    UCHAR LandZoneCyclinder[2];
    UCHAR RotationalPositionLock : 2;
    UCHAR Reserved2 : 6;
    UCHAR RotationOffset;
    UCHAR Reserved3;
    UCHAR RoataionRate[2];
    UCHAR Reserved4[2];
}MODE_RIGID_GEOMETRY_PAGE, *PMODE_RIGID_GEOMETRY_PAGE;

//
// Defines for Log Sense Pages
//

#define LOGSENSEPAGE0                        0x00
#define LOGSENSEPAGE2                        0x02
#define LOGSENSEPAGE3                        0x03
#define LOGSENSEPAGE30                       0x30
#define LOGSENSEPAGE31                       0x31

//
// Defined Log Sense Page Header
//

typedef struct _LOG_SENSE_PAGE_HEADER {

   UCHAR PageCode : 6;
   UCHAR Reserved1 : 2;
   UCHAR Reserved2;
   UCHAR Length[2];           // [0]=MSB ... [1]=LSB

} LOG_SENSE_PAGE_HEADER, *PLOG_SENSE_PAGE_HEADER;


//
// Defined Log Sense Parameter Header
//

typedef struct _LOG_SENSE_PARAMETER_HEADER {

   UCHAR ParameterCode[2];    // [0]=MSB ... [1]=LSB
   UCHAR LPBit     : 1;
   UCHAR Reserved1 : 1;
   UCHAR TMCBit    : 2;
   UCHAR ETCBit    : 1;
   UCHAR TSDBit    : 1;
   UCHAR DSBit     : 1;
   UCHAR DUBit     : 1;
   UCHAR ParameterLength;

} LOG_SENSE_PARAMETER_HEADER, *PLOG_SENSE_PARAMETER_HEADER;


//
// Defined Log Page Information - statistical values, accounts
// for maximum parameter values that is returned for each page
//

typedef struct _LOG_SENSE_PAGE_INFORMATION {

   union {

       struct {
          UCHAR Page0;
          UCHAR Page2;
          UCHAR Page3;
          UCHAR Page30;
          UCHAR Page31;
       } PageData ;

       struct {
          LOG_SENSE_PARAMETER_HEADER Parm1;
          UCHAR TotalRewrites[2];
          LOG_SENSE_PARAMETER_HEADER Parm2;
          UCHAR TotalErrorCorrected[3];
          LOG_SENSE_PARAMETER_HEADER Parm3;
          UCHAR NotApplicable[2];    // Always 0
          LOG_SENSE_PARAMETER_HEADER Parm4;
          UCHAR TotalBytesProcessed[4];
          LOG_SENSE_PARAMETER_HEADER Parm5;
          UCHAR TotalUnrecoverableErrors[2];
          LOG_SENSE_PARAMETER_HEADER Parm6;
          UCHAR RewritesLastReadOp[2];
       } Page2 ;

       struct {
          LOG_SENSE_PARAMETER_HEADER Parm1;
          UCHAR TotalRereads[2];
          LOG_SENSE_PARAMETER_HEADER Parm2;
          UCHAR TotalErrorCorrected[3];
          LOG_SENSE_PARAMETER_HEADER Parm3;
          UCHAR TotalCorrectableECCC3[2];
          LOG_SENSE_PARAMETER_HEADER Parm4;
          UCHAR TotalBytesProcessed[4];
          LOG_SENSE_PARAMETER_HEADER Parm5;
          UCHAR TotalUnrecoverableErrors[2];
          LOG_SENSE_PARAMETER_HEADER Parm6;
          UCHAR RereadsLastWriteOp[2];
       } Page3 ;

       struct {
          LOG_SENSE_PARAMETER_HEADER Parm1;
          UCHAR CurrentGroupsWritten[3];
          LOG_SENSE_PARAMETER_HEADER Parm2;
          UCHAR CurrentRewrittenFrames[2];
          LOG_SENSE_PARAMETER_HEADER Parm3;
          UCHAR CurrentGroupsRead[3];
          LOG_SENSE_PARAMETER_HEADER Parm4;
          UCHAR CurrentECCC3Corrections[2];
          LOG_SENSE_PARAMETER_HEADER Parm5;
          UCHAR PreviousGroupsWritten[3];
          LOG_SENSE_PARAMETER_HEADER Parm6;
          UCHAR PreviousRewrittenFrames[2];
          LOG_SENSE_PARAMETER_HEADER Parm7;
          UCHAR PreviousGroupsRead[3];
          LOG_SENSE_PARAMETER_HEADER Parm8;
          UCHAR PreviousECCC3Corrections[2];
          LOG_SENSE_PARAMETER_HEADER Parm9;
          UCHAR TotalGroupsWritten[4];
          LOG_SENSE_PARAMETER_HEADER Parm10;
          UCHAR TotalRewritteFrames[3];
          LOG_SENSE_PARAMETER_HEADER Parm11;
          UCHAR TotalGroupsRead[4];
          LOG_SENSE_PARAMETER_HEADER Parm12;
          UCHAR TotalECCC3Corrections[3];
          LOG_SENSE_PARAMETER_HEADER Parm13;
          UCHAR LoadCount[2];
       } Page30 ;

       struct {
          LOG_SENSE_PARAMETER_HEADER Parm1;
          UCHAR RemainingCapacityPart0[4];
          LOG_SENSE_PARAMETER_HEADER Parm2;
          UCHAR RemainingCapacityPart1[4];
          LOG_SENSE_PARAMETER_HEADER Parm3;
          UCHAR MaximumCapacityPart0[4];
          LOG_SENSE_PARAMETER_HEADER Parm4;
          UCHAR MaximumCapacityPart1[4];
       } Page31 ;

   } LogSensePage;


} LOG_SENSE_PAGE_INFORMATION, *PLOG_SENSE_PAGE_INFORMATION;


//
// Defined Log Sense Parameter Format - statistical values, accounts
// for maximum parameter values that is returned
//

typedef struct _LOG_SENSE_PARAMETER_FORMAT {

   LOG_SENSE_PAGE_HEADER       LogSenseHeader;
   LOG_SENSE_PAGE_INFORMATION  LogSensePageInfo;

} LOG_SENSE_PARAMETER_FORMAT, *PLOG_SENSE_PARAMETER_FORMAT;

//
// Define read write recovery page
//

typedef struct _MODE_READ_WRITE_RECOVERY_PAGE {

    UCHAR PageCode : 6;
    UCHAR Unknown1 : 2;
    UCHAR PageLength;
    UCHAR Unknown2;
    UCHAR ReadRetryCount;
    UCHAR Unknown3;
    UCHAR Unknown4;
    UCHAR Unknown5;
    UCHAR Unknown6;
    UCHAR WriteRetryCount;
    UCHAR Unknown7;
    UCHAR Unknown8;
    UCHAR Unknown9;

} MODE_READ_WRITE_RECOVERY_PAGE, *PMODE_READ_WRITE_RECOVERY_PAGE;

//
// Define Device Configuration Page
//

typedef struct _MODE_DEVICE_CONFIGURATION_PAGE {

    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PS : 1;
    UCHAR PageLength;
    UCHAR ActiveFormat : 5;
    UCHAR CAFBit : 1;
    UCHAR CAPBit : 1;
    UCHAR Reserved2 : 1;
    UCHAR ActivePartition;
    UCHAR WriteBufferFullRatio;
    UCHAR ReadBufferEmptyRatio;
    UCHAR WriteDelayTime[2];
    UCHAR REW : 1;
    UCHAR RBO : 1;
    UCHAR SOCF : 2;
    UCHAR AVC : 1;
    UCHAR RSmk : 1;
    UCHAR BIS : 1;
    UCHAR DBR : 1;
    UCHAR GapSize;
    UCHAR Reserved3 : 3;
    UCHAR SEW : 1;
    UCHAR EEG : 1;
    UCHAR EODdefined : 3;
    UCHAR BufferSize[3];
    UCHAR DCAlgorithm;
    UCHAR Reserved4;

} MODE_DEVICE_CONFIGURATION_PAGE, *PMODE_DEVICE_CONFIGURATION_PAGE;

#ifdef never

//
// Define Medium Partition Page
//

typedef struct _MODE_MEDIUM_PARTITION_PAGE {

    UCHAR PageCode : 6;
    UCHAR Unknown1 : 2;
    UCHAR PageLength;
    UCHAR UnknownBits1 : 1;
    UCHAR UnknownBits2 : 7;
    UCHAR AdditionalPartitionDefined;
    UCHAR UnknownBits3 : 3;
    UCHAR PSUMBit      : 2;
    UCHAR IDPBit       : 1;
    UCHAR UnknownBits4 : 2;
    UCHAR UnknownBits5 : 2;
    UCHAR UnknownBits6 : 6;
    UCHAR Unknown2;
    UCHAR Unknown3;
    UCHAR AdditionalPartitionSize[2];

} MODE_MEDIUM_PARTITION_PAGE, *PMODE_MEDIUM_PARTITION_PAGE;

//
// Define Control Mode Page
//

typedef struct _MODE_CONTROL_MODE_PAGE {

    UCHAR PageCode : 6;
    UCHAR Unknown1 : 2;
    UCHAR PageLength;
    UCHAR Reserved1;
    UCHAR Unknown2;
    UCHAR Unknown3;
    UCHAR Unknown4;
    UCHAR Unknown5;
    UCHAR Unknown6;

} MODE_CONTROL_MODE_PAGE, *PMODE_CONTROL_MODE_PAGE;

//
// Mode parameter list block descriptor -
// set the block length for reading/writing
//
//

#define MODE_SELECT_PFBIT                    0x10
#define MODE_BLOCK_DESC_LENGTH               8

typedef struct _MODE_PARM_READ_WRITE {

   MODE_PARAMETER_HEADER  ParameterListHeader;  // List Header Format
   MODE_PARAMETER_BLOCK   ParameterListBlock;   // List Block Descriptor

} MODE_PARM_READ_WRITE_DATA, *PMODE_PARM_READ_WRITE_DATA;

//
// Mode parameter list header and medium partition page -
// used in creating partitions
//

typedef struct _MODE_MEDIUM_PART_PAGE {

   MODE_PARAMETER_HEADER       ParameterListHeader;
   MODE_MEDIUM_PARTITION_PAGE  MediumPartPage;

} MODE_MEDIUM_PART_PAGE, *PMODE_MEDIUM_PART_PAGE;


//
// Mode parameters for retrieving tape or media information
//

typedef struct _MODE_TAPE_MEDIA_INFORMATION {

   MODE_PARAMETER_HEADER       ParameterListHeader;
   MODE_PARAMETER_BLOCK        ParameterListBlock;
   MODE_MEDIUM_PARTITION_PAGE  MediumPartPage;

} MODE_TAPE_MEDIA_INFORMATION, *PMODE_TAPE_MEDIA_INFORMATION;

#endif

//
// Mode parameter list header and device configuration page -
// used in retrieving device configuration information
//

typedef struct _MODE_DEVICE_CONFIG_PAGE {

   MODE_PARAMETER_HEADER       ParameterListHeader;
   MODE_DEVICE_CONFIGURATION_PAGE  DeviceConfigPage;

} MODE_DEVICE_CONFIG_PAGE, *PMODE_DEVICE_CONFIG_PAGE;


//
// CDROM audio control (0x0E)
//

#define CDB_AUDIO_PAUSE 0
#define CDB_AUDIO_RESUME 1

#define CDB_DEVICE_START 0x11
#define CDB_DEVICE_STOP 0x10

#define CDB_EJECT_MEDIA 0x10
#define CDB_LOAD_MEDIA 0x01

#define CDB_SUBCHANNEL_HEADER      0x00
#define CDB_SUBCHANNEL_BLOCK       0x01

#define CDROM_AUDIO_CONTROL_PAGE   0x0E
#define CDROM_AUDIO_CURRENT_VALUES 0x00
#define MODE_SELECT_IMMEDIATE      0x04

#define CDB_USE_MSF                0x01

typedef struct _PORT_OUTPUT {
    UCHAR ChannelSelection;
    UCHAR Volume;
} PORT_OUTPUT, *PPORT_OUTPUT;


#ifndef _H2INC

typedef struct _AUDIO_OUTPUT {
    UCHAR CodePage;
    UCHAR ParameterLength;
    UCHAR Immediate;
    UCHAR Reserved[2];
    UCHAR LbaFormat;
    UCHAR LogicalBlocksPerSecond[2];
    PORT_OUTPUT PortOutput[4];
} AUDIO_OUTPUT, *PAUDIO_OUTPUT;


#else // _H2INC

typedef struct _AUDIO_OUTPUT {
    MODE_PARAMETER_HEADER MPHeader;
    MODE_PARAMETER_BLOCK MPBlock;
    UCHAR CodePage;
    UCHAR ParameterLength;
    UCHAR Immediate;
    UCHAR Reserved[2];
    UCHAR LbaFormat;
    UCHAR LogicalBlocksPerSecond[2];
    PORT_OUTPUT PortOutput[4];
} AUDIO_OUTPUT, *PAUDIO_OUTPUT;

#endif // _H2INC

typedef struct _DEVICE_CAPABILITIES {
    UCHAR PageCode;
    UCHAR PageLen;
    UCHAR ReadCaps;
    UCHAR WriteCaps;
    UCHAR CDCaps1;
    UCHAR CDCaps2;
    UCHAR LoadMech;
    UCHAR ChangeCaps;
    UCHAR DCReserved[2];
    UCHAR VolumeLvls[2];
    UCHAR DevBuffer[2];
    UCHAR DCReserved2[6];
    UCHAR CopyMgmtRev[2];
    UCHAR DCReserved3[2];
} DEVICE_CAPABILITIES, *PDEVICE_CAPABILITIES;

typedef struct _DVD_ERROR_RECOVERY_PAGE {
	UCHAR PageCode;
	UCHAR PageLen;
	UCHAR ErrRecoveryParam;
	UCHAR ReadRetry;
	UCHAR WriteRetry;
	UCHAR VerifySpan;
	UCHAR Grp1TO[2];
	UCHAR Grp2TO[2];
} DVD_ERROR_RECOVERY_PAGE, *PDVD_ERROR_RECOVERY_PAGE;

/*
** Definitions of the ReadCaps (Read capabilities) flags
*/

#define RC_READ_CDR         0x01
#define RC_READ_CDE         0x02
#define RC_READ_CDRFIXEDPKT 0x04
#define RC_READ_DVD         0x08
#define RC_READ_DVDR        0x10
#define RC_READ_DVDRAM      0x20

/*
** Definitions of the WriteCaps (Write capabilities) flags
*/

#define WC_WRITE_CDR        0x01
#define WC_WRITE_CDE        0x02
#define WC_WRITE_TESTMODE   0x04
#define WC_WRITE_DVDR       0x10
#define WC_WRITE_DVDRAM     0x20

/*
** Definitions of CDCaps1 (CD device capabilities) flags
*/

#define CDC1_PLAY_AUDIO		0x01

/*
** Definitions of CDCaps2 (CD device capabilities) flags
*/

#define CDC2_CD_DA  		0x01
#define CDC2_DAACCU  		0x02

/*
** TOC structure definitions
*/

#define CDAUDIO_LEAD_OUT_TRACK  0xaa

typedef struct _TOC_HEADER {
	 UCHAR DataLength[2];
	 UCHAR FirstTrack;
	 UCHAR LastTrack;
} TOC_HEADER;

/* multi-session table of contents unions ... */

#define FirstSession FirstTrack
#define LastSession LastTrack

/* multi-session info */
#define SESSION_INFO_MAX_BUFFER_SIZE	0x800		

#define TOC_CONTROL_READ_SESSION_INFO 0x40
#define TOC_CONTROL_READ_ALL_SUBCODE  0x80

typedef struct _TOC_TD_ADR_CTRL_REC {
        UCHAR Control:4;
        UCHAR ADR:4;
} TOC_TD_ADR_CTRL_REC;

#define CTRL_DATA_TRACK 0x04

typedef struct _TOC_TRACK_DESCRIPTOR {
        UCHAR Reserved1;
        TOC_TD_ADR_CTRL_REC toc_ctrl_rec;
        UCHAR TrackID;
        UCHAR ReservedForHell2;
        UCHAR StartAddr[4];
} TOC_TRACK_DESCRIPTOR;

typedef struct _TOC_SESSION_DESCRIPTOR {
	 	UCHAR	Session;
		TOC_TD_ADR_CTRL_REC toc_ses_ctrl_rec;
		UCHAR	TrackNo;
		UCHAR	Point;
		UCHAR	Min;
		UCHAR	Sec;
		UCHAR	Frame;
		UCHAR	Zero;
		UCHAR	PMin;
		UCHAR	PSec;
		UCHAR	PFrame;

} TOC_SESSION_DESCRIPTOR;

/* defintions for TOC_SESSION_DESCRIPTOR */

#define SESSION_ADR_TRACK_INFO	0x10

/* definitions for the Point field */
#define SESSION_DESC_FIRST_TRACK	0xa0
#define SESSION_DESC_LAST_TRACK		0xa1
#define SESSION_DESC_LEAD_OUT_ADDR	0xa2
#define SESSION_DESC_END_OF_TRACKS	0x99

//
// Multisession CDROM
//

#define GET_LAST_SESSION 0x01
#define GET_SESSION_DATA 0x02;

//
//
// Tape definitions
//

typedef struct _TAPE_POSITION_DATA {
    UCHAR Reserved1:2;
    UCHAR BlockPositionUnsupported:1;
    UCHAR Reserved2:3;
    UCHAR EndOfPartition:1;
    UCHAR BeginningOfPartition:1;
    UCHAR PartitionNumber;
    USHORT Reserved3;
    UCHAR FirstBlock[4];
    UCHAR LastBlock[4];
    UCHAR Reserved4;
    UCHAR NumberOfBlocks[3];
    UCHAR NumberOfBytes[4];
} TAPE_POSITION_DATA, *PTAPE_POSITION_DATA;

/*
** DRAGON CDROM definitions
 */

/*
 ** equates for determining which subchannel info to read
 */

#define CD_READ_ALL_SUBQ        0x00
#define CD_READ_POSITION_SUBQ   0x01
#define CD_READ_UPC_CODE_SUBQ   0x02
#define CD_READ_ISRC_SUBQ       0x03
#define CDROM_STANDARD_XFER     0x96

/*
 ** Sub Channel Data structure for SCSI - 2 devices
 */

typedef struct _UPC_REC { /* */
        UCHAR   Reserved2:7;
        UCHAR   UPCValid:1;	/* binary flag to indicate UPC code valid */
} UPC_REC;

typedef struct _ISRC_REC { /* */
        UCHAR   Reserved2:7;
        UCHAR   ISRCValid:1;	/* binary flag to indicate ISRC code valid */
} ISRC_REC;

typedef struct _CTRL_ADR_REC {
        UCHAR   Control :4;
        UCHAR   ADR : 4;
} CTRL_ADR_REC;

typedef struct _SubQ_Data { /* */
		UCHAR	Reserved1;
		UCHAR	Audio_Status;
		UCHAR	Data_Len[2];	/* Big Endian - size of following data */
		UCHAR	Data_Format;
        CTRL_ADR_REC Subq_Ctrl_ADR_rec;
		UCHAR	Track;
		UCHAR	Index;
		UCHAR	AbsAddr[4];	/* absolute address for the current operation */
		UCHAR	TrackRelAddr[4];/* address of sector relative to start of current track */
        UPC_REC subq_upc_rec;  /* indicates upc valid */
		UCHAR	UPCCode[31 - 17]; /* UPC code in Big Endian */
        ISRC_REC subq_isrc_rec; /* indicates ISRC valid */
		UCHAR	ISRC[47-33];
} SubQ_Data, * pSubQ_Data;


typedef struct _SubQ_pos { /* */
		UCHAR	Reserved1;
		UCHAR	pos_Audio_Status;
		UCHAR	pos_Data_Len[2];	/* Big Endian - size of following data */
		UCHAR	pos_Data_Format;
        CTRL_ADR_REC pos_Subq_Ctrl_ADR_rec;
		UCHAR	pos_Track;
		UCHAR	pos_Index;
		UCHAR	pos_AbsAddr[4];	/* absolute address for the current operation */
		UCHAR	pos_TrackRelAddr[4];/* address of sector relative to start of current track */
} SubQ_pos, * pSubQ_pos;

typedef struct _SubQ_UPC { /* */
		UCHAR	upc_Reserved1;
		UCHAR	upc_Audio_Status;
		UCHAR	upc_Data_Len[2];	/* Big Endian - size of following data */
		UCHAR	upc_Data_Format;
        CTRL_ADR_REC upc_Subq_Ctrl_ADR_rec;
		UCHAR	upc_Track;
		UCHAR	upc_Index;
        UPC_REC upc_subq_upc_rec;  /* indicates upc valid */
		UCHAR	upc_UPCCode[31 - 17]; /* UPC code in Big Endian */
} SubQ_UPC, * pSubQ_UPC;


typedef struct _SubQ_ISRC { /* */
		UCHAR	upc_Reserved1;
		UCHAR	upc_Audio_Status;
		UCHAR	upc_Data_Len[2];	/* Big Endian - size of following data */
		UCHAR	upc_Data_Format;
        CTRL_ADR_REC upc_Subq_Ctrl_ADR_rec;
		UCHAR	upc_Track;
		UCHAR	upc_Index;
        ISRC_REC upc_subq_isrc_rec; /* indicates ISRC valid */
		UCHAR	upc_ISRC[47-33];
} SubQ_ISRC, * pSubQ_ISRC;

/*
** subchannel equates
*/
#define	CDROM_AUDIO_SUB_CHAN_FORMAT_ALL		0x00
#define CDROM_AUDIO_SUB_CHAN_CUR_POS            0x01
#define CDROM_AUDIO_SUB_CHAN_UPC                0x02
#define CDROM_AUDIO_SUB_CHAN_ISRC               0x03

#define CDROM_Q_SUB_CHAN_DATA_LEN	12
#define CDROM_UPC_ISRC_DATA_LEN         14
#define CDROM_Q_INFO_LEN                9


#define CDROM_AUDIO_STATUS_NOT_SUPPORTED	0x00
#define CDROM_AUDIO_STATUS_PLAYING	0x11
#define CDROM_AUDIO_STATUS_PAUSED	0x12
#define CDROM_AUDIO_STATUS_DONE		0x13
#define CDROM_AUDIO_STATUS_ERROR	0x14
#define CDROM_AUDIO_STATUS_NO_STATUS	0x15

/*
 ** equates for sector sizes
 */

#define CDROM_COOKED_SECTOR_SIZE	2048
#define CDROM_RAW_SECTOR_SIZE		2340

#define CD_DEFAULT_BLOCK_SIZE CDROM_COOKED_SECTOR_SIZE

/* defines for addressing
 */

#define FRAMES_PER_SECOND		75
/*
** Audio Channel equates
*/

#define CDROM_AUDIO_LBA_INVALID		0x00
#define CDROM_AUDIO_IMED_RETURN		0x04
#define CDROM_AUDIO_SOTC		0x02

//
// Byte reversing macro for converting
// between big- and little-endian formats
//

#define REVERSE_BYTES(Destination, Source) {                \
    (Destination)->Byte3 = (Source)->Byte0;                 \
    (Destination)->Byte2 = (Source)->Byte1;                 \
    (Destination)->Byte1 = (Source)->Byte2;                 \
    (Destination)->Byte0 = (Source)->Byte3;                 \
}

//
// This structure is used to convert little endian
// ULONGs to SCSI CDB 4 byte big endians values.
//

typedef struct _FOUR_BYTE {
    UCHAR Byte0;
    UCHAR Byte1;
    UCHAR Byte2;
    UCHAR Byte3;
} FOUR_BYTE, *PFOUR_BYTE;

//
// This macro has the effect of Bit = log2(Data)
//

#define WHICH_BIT(Data, Bit) {                      \
    for (Bit = 0; Bit < 32; Bit++) {                \
        if ((Data >> Bit) == 1) {                   \
            break;                                  \
        }                                           \
    }                                               \
}

#pragma pack()

#endif /* _SCSI_H_ */
