;****************************************************************************
;                                                                           *
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
; PURPOSE.                                                                  *
;                                                                           *
; Copyright 1993-95  Microsoft Corporation.  All Rights Reserved.           *
;                                                                           *
;****************************************************************************

PAGE 58,132
;******************************************************************************
TITLE CONTROL - ControlDispatch for VxD
;******************************************************************************
;
    .386p

;******************************************************************************
;                I N C L U D E S
;******************************************************************************

    .xlist
    include vmm.inc
    include debug.inc
    .list

; the following equate makes the VXD dynamically loadable.
ASYNCW32_DYNAMIC   EQU 1
ASYNCW32_DEVICE_ID EQU 19ABH

;============================================================================
;        V I R T U A L   D E V I C E   D E C L A R A T I O N
;============================================================================

DECLARE_VIRTUAL_DEVICE ASYNCW32, 1, 0, ASYNCW32_Control, ASYNCW32_DEVICE_ID, \
                        UNDEFINED_INIT_ORDER

VxD_LOCKED_CODE_SEG

;===========================================================================
;
;   PROCEDURE: ASYNCW32_Control
;
;   DESCRIPTION:
;    Device control procedure for the ASYNCW32 VxD
;
;   ENTRY:
;    EAX = Control call ID
;
;   EXIT:
;    If carry clear then
;        Successful
;    else
;        Control call failed
;
;   USES:
;    EAX, EBX, ECX, EDX, ESI, EDI, Flags
;
;============================================================================

BeginProc ASYNCW32_Control
    Control_Dispatch SYS_DYNAMIC_DEVICE_INIT, ASYNCW32_Dynamic_Init, sCall
    Control_Dispatch SYS_DYNAMIC_DEVICE_EXIT, ASYNCW32_Dynamic_Exit, sCall
    Control_Dispatch W32_DEVICEIOCONTROL,     ASYNCW32_DeviceIOControl, sCall, <ecx, ebx, edx, esi>
    clc
    ret
EndProc ASYNCW32_Control

VxD_LOCKED_CODE_ENDS

    END

