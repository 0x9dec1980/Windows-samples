/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        pch.h

    Abstract:
        Pre-compile C header file.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Apr-2000

    Revision History:
--*/

#ifndef _PCH_H
#define _PCH_H

#define COMPONENT_NAME          "HBTN"
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

#include <wdm.h>
#include <hidport.h>
#include "mstabbtn.h"
#include <hidbtn.h>
#include <errlog.h>
#include <trace.h>
#define INC_HID_NAMES
#include <tracedat.h>

#endif  //ifndef _PCH_H
