/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: devctrl.h
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

typedef DIOCPARAMETERS *LPDIOC;

DWORD _stdcall VXD3DL_DeviceIOControl(DWORD, DWORD, DWORD, LPDIOC);
DWORD _stdcall VXD3DL_GetVersion(DWORD, DWORD, LPDIOC);
DWORD _stdcall VXD3DL_MemoryRequest(DWORD, DWORD, LPDIOC);
DWORD _stdcall VXD3DL_16To32Pointer(DWORD, DWORD, LPDIOC);
DWORD _stdcall VXD3DL_I2C(DWORD, DWORD, LPDIOC);

DWORD ( _stdcall *VXD3DL_Proc[] )(DWORD, DWORD, LPDIOC) = 
{
    VXD3DL_GetVersion,
    VXD3DL_MemoryRequest,
    VXD3DL_16To32Pointer,
    VXD3DL_I2C
};

#define MAX_VXD3DL_API (sizeof(VXD3DL_Proc)/sizeof(DWORD))
