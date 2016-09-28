/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright 1993-95  Microsoft Corporation.  All Rights Reserved.           *
*                                                                           *
****************************************************************************/

#ifdef _VKD_H

typedef DWORD HHK;  // HotKey handle

MAKE_HEADER(HVM,_cdecl,VKD_Get_Kbd_Owner, (VOID))
MAKE_HEADER(DWORD,_stdcall,VKD_Get_Shift_State, (HVM hVM))
MAKE_HEADER(HHK,_stdcall,VKD_Define_Hot_Key, (DWORD dwCode,       \
                                              DWORD dwShiftState, \
                                              DWORD dwOp,         \
                                              DWORD hkCallBack,   \
                                              DWORD dwRefData,    \
                                              DWORD NotifyDelay))
MAKE_HEADER(VOID,_stdcall,VKD_Remove_Hot_Key, (HHK hHotKey))
MAKE_HEADER(DWORD,_stdcall,VKD_Filter_Keyboard_Input, (VOID))

#define VKD_Get_Kbd_Owner               PREPEND(VKD_Get_Kbd_Owner)
#define VKD_Define_Hot_Key              PREPEND(VKD_Define_Hot_Key)
#define VKD_Remove_Hot_Key              PREPEND(VKD_Remove_Hot_Key)

#endif // _VKD_H
