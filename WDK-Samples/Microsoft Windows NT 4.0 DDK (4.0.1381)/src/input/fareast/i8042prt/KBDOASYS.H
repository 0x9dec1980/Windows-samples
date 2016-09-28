/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990, 1991, 1992, 1993  Microsoft Corporation

Module Name:

    kbdoasys.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the Fujitsu FMV oyayubi-shift keyboard device.

Author:

    96/07/26 v-kazuta (Fujitsu)
    96/09/15 hideyukn

Revision History:

--*/

#ifndef _FJKBD_H_
#define _FJKBD_H_

//
// FMV oyayubi-shift keyboard internal input mode value.
//
#define FMV_NOROMAN_ALPHA_CAPSON     0x01
#define FMV_NOROMAN_ALPHA_CAPSOFF    0x02
#define FMV_NOROMAN_HIRAGANA         0x03
#define FMV_NOROMAN_KATAKANA         0x04
#define FMV_ROMAN_ALPHA_CAPSON       0x05
#define FMV_ROMAN_ALPHA_CAPSOFF      0x06
#define FMV_ROMAN_HIRAGANA           0x07
#define FMV_ROMAN_KATAKANA           0x08

//
// Following functions are FMV oyayubi-shift keyboard use only.
//
NTSTATUS
I8042SetIMEStatusForOasys(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

ULONG
I8042QueryIMEStatusForOasys(
    IN PKEYBOARD_IME_STATUS KeyboardIMEStatus
    );

VOID
I8xKeyboardInitiateIoForOasys(
    IN PVOID Context
    );
#endif // _FJKBD_H_
