/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        pch.h

    Abstract:
        Pre-compile C header file.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Mar-2000

    Revision History:
--*/

#ifndef _PCH_H
#define _PCH_H

#define COMPONENT_NAME          "HPEN"
#define INTERNAL
#define EXTERNAL

#if DBG
#ifndef DEBUG
  #define DEBUG
#endif
#ifndef TRACING
  #define TRACING
#endif
#endif

#ifdef DEBUG
  #define PERFORMANCE_TEST
#endif
//#define _TIMESTAMP_

#include <wdm.h>
#include <ntddser.h>
#include <hidport.h>
#include "wacompen.h"
#include <hidpen.h>
#include <serial.h>
#include <errlog.h>
#include <trace.h>
#define INC_HID_NAMES
#define INC_SERIAL_NAMES
#include <tracedat.h>

#endif  //ifndef _PCH_H
