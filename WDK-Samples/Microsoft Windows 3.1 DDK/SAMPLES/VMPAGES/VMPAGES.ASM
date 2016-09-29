PAGE 58,132
;******************************************************************************
TITLE VMPAGES 
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1991
;
;   Title:      VMPAGES.ASM 
;
;   Version:    3.00
;
;   Date:       22-May-1991 
;
;   Author:     Neil Sandlin
;
;   Description: This VxD demonstrates how to export a VxD service, 
;                GetVMPgCount, to an application.
;
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV                 DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;
;==============================================================================

        .386p

;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

        .XLIST
        INCLUDE VMM.Inc
        INCLUDE Debug.Inc
        INCLUDE VMPAGES.Inc
        .LIST


;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VMPAGES, 3, 0, VMPAGES_Control, VMPAGES_Dev_ID,, \
                                    VMPAGES_API_PROC , VMPAGES_API_Proc



VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VMPAGES_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation.  However, in this example,
;       there's no code because we don't want any call-back calls.
;
;
;==============================================================================

BeginProc VMPAGES_Control

        clc
        ret

EndProc VMPAGES_Control


VxD_LOCKED_CODE_ENDS


VxD_CODE_SEG

BeginDoc
;******************************************************************************
;
;   VMPAGES_API_Proc
;
;   DESCRIPTION:
;
;       This is the exported API procedure that is callable from VM's. 
;       An application needs only to use INT 2Fh, AX=1684h, BX=device ID
;       and a call back address is returned. Then, when the
;       address is called, eventually it ends up here. 
;
;
;==============================================================================
EndDoc

BeginProc VMPAGES_API_Proc

        and     [ebp.Client_Flags], NOT CF_Mask ; clear VM's carry flag

        Client_Ptr_Flat edi, ES, DI             ; point to buffer

        VMMCall _GetVMPgCount, <ebx, 0>
        mov     [edi], eax
        mov     [edi+4], edx

        ret

EndProc VMPAGES_API_Proc

VxD_CODE_ENDS

        END
