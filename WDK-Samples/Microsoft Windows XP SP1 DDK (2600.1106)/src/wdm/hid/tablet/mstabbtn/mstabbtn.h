/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        MSTabBtn.h

    Abstract:
        Contains OEM specific definitions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Apr-2000

    Revision History:
--*/

#ifndef _MSTABBTN_H
#define _MSTABBTN_H

//
// Constants
//
#define HBTNF_INTERRUPT_CONNECTED       0x80000000
#define HBTNF_DEBOUNCE_TIMER_SET        0x40000000

#define OEM_VENDOR_ID                   0x3647          //"MST"
#define OEM_PRODUCT_ID                  0x0000
#define OEM_VERSION_NUM                 1
#define OEM_BUTTON_DEBOUNCE_TIME        10              //10msec

#define REPORTID_BTN                    1
#define REPORTID_KBD                    2

#define SASF_DELETE                     0x01
#define SASF_LEFT_CTRL                  0x02
#define SASF_LEFT_ALT                   0x04
#define SASF_CAD                        (SASF_DELETE |          \
                                         SASF_LEFT_CTRL |       \
                                         SASF_LEFT_ALT)

//
// Button ports
//
#define PORT_BUTTONSTATUS(devext)       \
        (PUCHAR)((devext)->OemExtension.IORes.u.Port.Start.QuadPart)
#define BUTTON_STATUS_MASK              0x1f
#define BUTTON_INTERRUPT_MASK           0x80
#define BUTTON_VALID_BITS               (BUTTON_STATUS_MASK |   \
                                         BUTTON_INTERRUPT_MASK)

//
// Macros
//
#define READBUTTONSTATE(devext)         (ULONG)(~READ_PORT_UCHAR(           \
                                                PORT_BUTTONSTATUS(devext)) &\
                                                BUTTON_VALID_BITS)

//
// Type definitions
//

//
// This must match the report descriptor, so make sure it is byte align.
//
#include <pshpack1.h>
typedef struct _OEM_BTN_REPORT
{
    ULONG dwButtonStates;
} OEM_BTN_REPORT, *POEM_BTN_REPORT;

typedef struct _OEM_KBD_REPORT
{
    UCHAR bKeys;
    UCHAR abReserved[3];
} OEM_KBD_REPORT, *POEM_KBD_REPORT;

typedef struct _HID_INPUT_REPORT
{
    UCHAR  ReportID;
    union
    {
        OEM_BTN_REPORT BtnReport;
        OEM_KBD_REPORT KbdReport;
    };
} HID_INPUT_REPORT, *PHID_INPUT_REPORT;
#include <poppack.h>

typedef struct _OEM_EXTENSION
{
    CM_PARTIAL_RESOURCE_DESCRIPTOR IORes;//button port resource
    CM_PARTIAL_RESOURCE_DESCRIPTOR IRQRes;//button IRQ resource
    PKINTERRUPT    InterruptObject;     //location of the interrupt object
    LARGE_INTEGER  DebounceTime;
    KTIMER         DebounceTimer;
    KDPC           TimerDpc;
    ULONG          dwLastButtonState;
    ULONG          dwLastButtons;
    ULONG          dwSASButtonID;
} OEM_EXTENSION, *POEM_EXTENSION;

//
// Global Data Declarations
//
extern UCHAR gReportDescriptor[];
extern ULONG gdwcbReportDescriptor;
extern HID_DESCRIPTOR gHidDescriptor;
extern PWSTR gpwstrManufacturerID;
extern PWSTR gpwstrProductID;
extern PWSTR gpwstrSerialNumber;

#endif  //ifndef _MSTABBTN_H
