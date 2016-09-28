/*++
    Copyright (c) 1998-2001 Microsoft Corporation

    Module Name:
        tracedat.h

    Abstract:
        This module contains definitions of the trace name tables.

    Author:
        Michael Tsang (MikeTs) 24-Sep-1998

    Environment:
        Kernel mode

    Revision History:
--*/

#ifndef _TRACEDAT_H
#define _TRACEDAT_H

#ifdef TRACING
//
// Exported data
//
extern NAMETABLE MajorIrpNames[];
extern NAMETABLE PnpMinorIrpNames[];
extern NAMETABLE PowerMinorIrpNames[];
extern NAMETABLE PowerStateNames[];
extern NAMETABLE QueryIDTypeNames[];
#ifdef INC_HID_NAMES
extern NAMETABLE HidIoctlNames[];
#endif
#ifdef INC_SERIAL_NAMES
extern NAMETABLE SerialIoctlNames[];
extern NAMETABLE SerialInternalIoctlNames[];
#endif
#endif  //ifdef TRACING

#endif  //ifndef _TRACEDAT_H
