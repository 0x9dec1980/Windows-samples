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
TITLE CONTROL - ControlDispatch for VxD in C
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
CVXDSAMP_DYNAMIC EQU 1

CVXD_DEVICE_ID EQU 18ABH

ifdef _VXD_SERVICES
; ****** Define the VxD services that can be called from other VxDs
Create_CVXD_Service_Table = 1

; The VxD services provided for other VxDs to call
Begin_Service_Table CVXD
CVXD_Service     _CVXD_Get_Version, VxD_LOCKED_CODE
End_Service_Table CVXD
endif

;============================================================================
;        V I R T U A L   D E V I C E   D E C L A R A T I O N
;============================================================================

DECLARE_VIRTUAL_DEVICE    CVXDSAMP, 1, 0, CVXD_Control, CVXD_DEVICE_ID, \
                        UNDEFINED_INIT_ORDER, CVXD_VM, CVXD_PM

VxD_LOCKED_CODE_SEG

ifdef _VXD_SERVICES
extrn _CVXD_Get_Version:near
endif

extrn _CVXD_VMAPI@12:near
extrn _CVXD_PMAPI@12:near

;===========================================================================
;
;   PROCEDURE: CVXD_Control
;
;   DESCRIPTION:
;    Device control procedure for the CVXD VxD
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

BeginProc CVXD_Control
    Control_Dispatch SYS_DYNAMIC_DEVICE_INIT, CVXD_Dynamic_Init, sCall
    Control_Dispatch SYS_DYNAMIC_DEVICE_EXIT, CVXD_Dynamic_Exit, sCall
    Control_Dispatch W32_DEVICEIOCONTROL,     CVXD_W32_DeviceIOControl, sCall, <ecx, ebx, edx, esi>
    clc
    ret
EndProc CVXD_Control

; *************************************************************************
; this routine handles api calls made from V86 mode programs.  For the 
; purpose of this examples it is assumed that the function number is passed
; in eax and any additional parameters are passed in ebx and ecx.
; The C code will do a SWITCH on the eax value to determine which routine to
; call and what parameters to pass.  It is assumed that all functions return
; a value in eax.
BeginProc CVXD_VM
    scall   CVXD_VMAPI, <[ebp].Client_EAX, [ebp].Client_EBX, [ebp].Client_ECX>
    mov    [ebp].Client_EAX,eax    ; put return code
    ret
EndProc   CVXD_VM



; *************************************************************************
; this routine handles api calls made from protected mode programs.  For the 
; purpose of this examples it is assumed that the function number is passed
; in eax and any additional parameters are passed in ebx and ecx.
; The C code will do a SWITCH on the eax value to determine which routine to
; call and what parameters to pass.  It is assumed that all functions return
; a value in eax.
BeginProc CVXD_PM
    scall   CVXD_PMAPI, <[ebp].Client_EAX, [ebp].Client_EBX, [ebp].Client_ECX>
    mov     [ebp].Client_EAX,eax
    ret
EndProc   CVXD_PM

VxD_LOCKED_CODE_ENDS

;
; Real Mode Init segment: If this VxD is statically loaded a message
; will appear in Real Mode before windows is started.
;
VxD_REAL_INIT_SEG

Load_Success_Msg  DB  'CVXDSAMP: Real Mode Init called',13,10
Load_Msg_Len      EQU $ - Load_Success_Msg

BeginProc CVXD_Real_Init

    mov    ah, 40h
    mov    bx, 1
    mov    cx, Load_Msg_Len
    mov    dx, OFFSET Load_Success_Msg
    int    21h

    xor    bx, bx
    xor    si, si
    xor    edx, edx
    mov    ax, Device_Load_Ok
    ret

EndProc CVXD_Real_Init

VxD_REAL_INIT_ENDS

    END CVXD_Real_Init

