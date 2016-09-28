/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    common.h

Abstract:

    Common Header

Environment:

    user mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

  10/17/99: Created Keith S. Garner

--*/

#ifndef _UNICODE
#define _UNICODE
#endif 

#ifndef UNICODE
#define UNICODE
#endif 

#include <tchar.h>    // Make program ansi AND unicode safe
#include <windows.h>  // Most Windows functions
#include <commctrl.h> // Used for TreeView controls
#include <setupapi.h> // Used for SetupDiXxx functions
#include <cfgmgr32.h> // Used for CM_Xxxx functions
#include <regstr.h>   // Extract Registry Strings
#include "resource.h" // Resource Values.

//
//  Define the value DBG if _DEBUG is defined. 
//  _DEBUG is defined within the Visual C IDE.
//
#ifdef _DEBUG 
   #ifndef DBG
      #define DBG 1
   #endif
#endif 

//
// Functions exported by SetupDi.c
//
BOOL StateChange(DWORD NewState, DWORD SelectedItem, HDEVINFO hDevInfo);
BOOL EnumAddDevices(BOOL ShowHidden, HWND hwndTree, HDEVINFO hDevInfo);

//
// Functions exported by Main.c
//
BOOL GetClassImageIndex(LPGUID ClassGuid, PINT Index);

//
// Functions exported by config.c
//
BOOL IsClassHidden( GUID * ClassGuid );
BOOL IsDisableable ( DWORD SelectedItem, HDEVINFO hDevInfo );
BOOL IsDisabled ( DWORD SelectedItem, HDEVINFO hDevInfo );
BOOL GetStatus( DEVINST DevInst, DWORD * Status, DWORD * Problem );
BOOL ConstructDeviceName( HDEVINFO  DeviceInfoSet,
                          PSP_DEVINFO_DATA  DeviceInfoData,
                          PVOID       Buffer,
                          PULONG  Length);

//
// Functions exported by debug.c
//
#if DBG

#define DebugPrint(_x_) __DbgOut__ _x_

void __DbgOut__(LPTSTR lpszFormat, ...);
void DisplayError(TCHAR * ErrorName);

#else 

#define DebugPrint(_x_) 
#define DisplayError(_x_)  

#endif //DBG



