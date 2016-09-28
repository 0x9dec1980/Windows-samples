/*++
    Copyright (c) 2000,2002 Microsoft Corporation

    Module Name:
        mutohpen.h

    Abstract:
        Contains OEM specific definitions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Mar-2000

    Revision History:
--*/

#ifndef _MUTOHPEN_H
#define _MUTOHPEN_H

//
// Constants
//
#define OEM_VENDOR_ID           0x3429          //"MAI"

#define HPENF_SERIAL_OPENED     0x80000000
#define HPENF_SERIAL_INITED     0x40000000

// OEM serial communication parameters
#define OEM_SERIAL_BAUDRATE     19200
#define OEM_SERIAL_WORDLEN      8
#define OEM_SERIAL_PARITY       NO_PARITY
#define OEM_SERIAL_STOPBITS     STOP_BIT_1

#define OEM_PEN_MAX_X           8600
#define OEM_PEN_MAX_Y           6480

#define NORMALIZE_DATA(w)       ((USHORT)((((w) & 0x7f00) >> 1) |           \
                                          ((w) & 0x007f)))
#define IsValidPacket(p)        (((p)->InputReport.bStatus & INSTATUS_SYNC) && \
                                 !((p)->InputReport.bStatus &                  \
                                   (INSTATUS_RESERVED1 | INSTATUS_RESERVED2 |  \
                                    INSTATUS_TEST_DATA)) &&                    \
                                 !((p)->InputReport.wXData & INDATA_SYNC) &&   \
                                 !((p)->InputReport.wYData & INDATA_SYNC))

//
// This must match with hardware, so make sure it is byte aligned.
//
#include <pshpack1.h>
typedef struct _OEM_INPUT_REPORT
{
    union
    {
        struct
        {
            UCHAR  bStatus;
            USHORT wXData;
            USHORT wYData;
        } InputReport;
        UCHAR RawInput[5];
    };
} OEM_INPUT_REPORT, *POEM_INPUT_REPORT;

typedef struct _HID_INPUT_REPORT
{
    UCHAR            ReportID;
    OEM_INPUT_REPORT Report;
#ifdef _TIMESTAMP_
    LARGE_INTEGER    TimeStamp;
#endif
} HID_INPUT_REPORT, *PHID_INPUT_REPORT;

// bStatus bit values
#define INSTATUS_PEN_TIP_DOWN           0x01
#define INSTATUS_SIDE_SW_ENABLED        0x02
#define INSTATUS_RESERVED1              0x04
#define INSTATUS_TEST_DATA              0x08
#define INSTATUS_RESERVED2              0x10
#define INSTATUS_IN_PROXIMITY           0x20
#define INSTATUS_SW_CHANGED             0x40
#define INSTATUS_SYNC                   0x80
#define INDATA_SYNC                     0x8080
#include <poppack.h>

typedef struct _OEM_DATA
{
    SERIAL_BASIC_SETTINGS PrevSerialSettings;
    OEM_INPUT_REPORT      ResyncData[2];//resync data buffer
    ULONG                 BytesInBuff;  //number of bytes in the resync buffer
    USHORT                wFirmwareDate;
    USHORT                wFirmwareYear;
    USHORT                wProductID;
    USHORT                wFirmwareVer;
    USHORT                wCorrectionRev;
#ifdef _TIMESTAMP_
    LARGE_INTEGER         LastCounter;
#endif
#ifdef DEBUG
    OEM_INPUT_REPORT      LastReport;
    ULONGLONG             LastReportTime;
    ULONGLONG             StartTime;
    USHORT                wMaxX;
    USHORT                wMaxY;
    ULONG                 dwcSamples;
    ULONG                 dwcLostBytes;
#endif
} OEM_DATA, *POEM_DATA;

//
// Global Data Declarations
//
extern UCHAR gReportDescriptor[];
extern ULONG gdwcbReportDescriptor;
extern HID_DESCRIPTOR gHidDescriptor;
extern PWSTR gpwstrManufacturerID;
extern PWSTR gpwstrProductID;
extern PWSTR gpwstrSerialNumber;

#endif  //ifndef _MUTOHPEN_H
