/*++

Copyright (C) 1995-1996  Microsoft Corporation

Module Name:

    scsicdwo.h

Abstract:

    Header file and interface for the CD Writer (Write Once) class driver.

Author:

    Tom McGuire (tommcg)

Environment:

    Kernel mode for driver and user mode for interface to driver.  Must
    include either <ntddk.h> (kernel mode) or <winioctl.h> (user mode)
    before including this header file.

--*/

#ifndef _SCSICDWO_
#define _SCSICDWO_

#define FILE_DEVICE_CD_WRITER           0xCD    // unique in devioctl.h

#define CDWO_SENSE_MAX_LENGTH           32
#define CDWO_CDB_MAX_LENGTH             16
#define CDWO_TEXT_ID_LENGTH             28
#define CDWO_STANDARD_TIMEOUT           30      // 30 seconds default
#define CDWO_MAX_RETRY                  2       // try once + retry twice = 3 tries total


#pragma pack( 1 )   // for CDWRITER_PARAMETERS and WRITER_SENSE_DATA

typedef struct _CDWRITER_PARAMETERS CDWRITER_PARAMETERS, *PCDWRITER_PARAMETERS;

struct _CDWRITER_PARAMETERS {

    IN  ULONG SizeOfCdWriterParameters;

    OUT UCHAR ScsiPortNumber;
    OUT UCHAR ScsiPathId;
    OUT UCHAR ScsiTargetId;
    OUT UCHAR ScsiLun;

    OUT ULONG BufferAlign;
    OUT ULONG BufferMaxSize;
    OUT ULONG BufferMaxPages;

    OUT union {

        UCHAR DeviceId[ CDWO_TEXT_ID_LENGTH ];

        struct {
            UCHAR VendorId[ 8 ];
            UCHAR ProductId[ 16 ];
            UCHAR RomRevision[ 4 ];
            };
        };

    OUT UCHAR Terminator;

    };


typedef struct _WRITER_SENSE_DATA WRITER_SENSE_DATA, *PWRITER_SENSE_DATA;

struct _WRITER_SENSE_DATA {

    union {

        UCHAR Byte[ CDWO_SENSE_MAX_LENGTH ];

        struct {

            UCHAR ErrorCode         :7;                 // byte 0
            UCHAR Valid             :1;

            UCHAR ReservedByte1;                        // byte 1

            UCHAR SenseKey          :4;                 // byte 2
            UCHAR ReservedBit       :1;
            UCHAR IncorrectLength   :1;
            UCHAR ReservedBits      :2;

            UCHAR InformationBytes[ 4 ];                // byte 3..6

            UCHAR AdditionalSenseLength;                // byte 7

            UCHAR CommandSpecificInformation[ 4 ];      // byte 8..11

            UCHAR AdditionalSenseCode;                  // byte 12

            UCHAR AdditionalSenseCodeQualifier;         // byte 13

            };
        };
    };


#pragma pack()      // restore normal packing


typedef struct _CDWRITER_CONTROL CDWRITER_CONTROL, *PCDWRITER_CONTROL;

struct _CDWRITER_CONTROL {

    IN     ULONG             SizeOfCdWriterControl;
    IN     UCHAR             CdbData[ CDWO_CDB_MAX_LENGTH ];
    IN     UCHAR             CdbLength;
    IN     BOOLEAN           WriteToDevice;
    IN     USHORT            TimeoutSeconds;
    IN OUT ULONG             TransferLength;
    IN OUT PVOID             TransferBuffer;            // must be aligned
       OUT LONG              NtStatus;
       OUT UCHAR             ScsiStatus;
       OUT UCHAR             SrbStatus;
       OUT UCHAR             SenseLength;
       OUT WRITER_SENSE_DATA SenseData;

    };



//
//  IOCTL_CDWRITER_CONTROL takes InputBuffer of CDWRITER_CONTROL and
//  performs in/out operations on the parameters in the CDWRITER_CONTROL
//  structure.
//

#define IOCTL_CDWRITER_CONTROL                      \
            CTL_CODE(                               \
                FILE_DEVICE_CD_WRITER,              \
                0x871,                              \
                METHOD_NEITHER,                     \
                FILE_READ_DATA | FILE_WRITE_DATA    \
                )


//
//  IOCTL_CDWRITER_QUERY_PARAMETERS writers a CDWRITER_PARAMETER structure
//  to the caller's OutputBuffer parameter and sets the OutputBufferLength.
//

#define IOCTL_CDWRITER_QUERY_PARAMETERS             \
            CTL_CODE(                               \
                FILE_DEVICE_CD_WRITER,              \
                0x872,                              \
                METHOD_NEITHER,                     \
                FILE_ANY_ACCESS                     \
                )


#endif /* _SCSICDWO_ */





