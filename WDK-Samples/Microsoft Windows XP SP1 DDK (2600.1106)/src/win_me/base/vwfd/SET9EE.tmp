;****************************************************************************
;                                                                           *
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
; PURPOSE.                                                                  *
;                                                                           *
; Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
;                                                                           *
;****************************************************************************

PAGE 58,132
;******************************************************************************
TITLE VWFD 
;******************************************************************************
;
;   Title:      VWFD.ASM 
;
;   Version:    3.00
;
;==============================================================================

        .386p

;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

        .XLIST
        INCLUDE VMM.Inc
        INCLUDE Debug.Inc
        INCLUDE MINIVDD.Inc
        INCLUDE VWFD.Inc
        .LIST


VWFD_CB_DATA STRUC

VWFD_State  dd  ?

VWFD_CB_DATA ENDS


;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VWFD, 3, 0, VWFD_Control, VWFD_Dev_ID,, \
                                    VWFD_API_PROC , VWFD_API_Proc


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_LOCKED_DATA_SEG
        ALIGN   4

VWFD_CB_Offset  dd  0
VDD_VMType_Proc dd  0

VxD_LOCKED_DATA_ENDS


VxD_ICODE_SEG

;******************************************************************************
;
;   VWFD_Sys_Crit_Init
;
;   DESCRIPTION:
;       This routine gets some "VM BLOCK" memory.
;   
;
;==============================================================================

BeginProc VWFD_Sys_Crit_Init

        push    ebx
        VMMCall _Allocate_Device_CB_Area, <<SIZE VWFD_CB_DATA>, 0>
        test    eax, eax
        jnz     SHORT VWFD_CB_OK
        Debug_Out "VWFD: Allocate_Device_CB_Area failed"
        VMMCall Fatal_Memory_Error

VWFD_CB_OK:
        mov     [VWFD_CB_Offset], eax
        pop     ebx
        
        clc
        ret


EndProc VWFD_Sys_Crit_Init

;******************************************************************************
;
;   VWFD_Device_Init
;
;   DESCRIPTION:
;       This routine hooks the service that specifies the windowed
;       state of a virtual machine.
;   
;
;==============================================================================

BeginProc VWFD_Device_Init

        GetVxDServiceOrdinal eax, VDD_Set_VMType   ; hook this service
        mov     esi, OFFSET32 VWFD_Set_VMType     ; with this little front end
        VMMCall Hook_Device_Service         ; grab it
        jc      SHORT not_hooked            ; huh?
        mov     [VDD_VMType_Proc], esi
  
not_hooked:
        clc
        ret


EndProc VWFD_Device_Init

VxD_ICODE_ENDS



;******************************************************************************


VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VWFD_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation. 
;
;
;==============================================================================

BeginProc VWFD_Control

        Control_Dispatch Sys_Critical_Init, VWFD_Sys_Crit_Init
        Control_Dispatch Device_Init, VWFD_Device_Init
        clc
        ret

EndProc VWFD_Control

;******************************************************************************
;
;   VWFD_Set_VMType
;
;   DESCRIPTION:
;       This routine keeps track of the windowed state of each VM.
;   
;
;==============================================================================

BeginProc VWFD_Set_VMType

        pushad
        mov     esi, ebx                    ; vm block
        add     esi, [VWFD_CB_Offset]       ; point to our area
        mov     [esi.VWFD_State], eax       ; save the current state
        popad
        jmp     [VDD_VMType_Proc]           ; chain to real handler

EndProc VWFD_Set_VMType


VxD_LOCKED_CODE_ENDS


VxD_CODE_SEG

BeginDoc
;******************************************************************************
;
;   VWFD_API_Proc
;
;   DESCRIPTION:
;
;       This is the exported API procedure that is callable from VM's. 
;       An application needs only to use INT 2Fh, AX=1684h, BX=device ID
;       and a call back address is returned. Then, when the
;       address is called, eventually it ends up here. 
;
;   ENTRY:
;       ebp -> Client data area
;       ebx -> Current VMCB
;
;
;==============================================================================
EndDoc

BeginProc VWFD_API_Proc

        add     ebx, [VWFD_CB_Offset]          ; point to our area
        mov     edx, [ebx.VWFD_State]          ; get the state

        and     [ebp.Client_Flags], NOT CF_Mask ; clear VM's carry flag

        mov     [ebp.Client_AX], dx             ;
        shr     edx, 16
        mov     [ebp.Client_DX], dx

        ret

EndProc VWFD_API_Proc

VxD_CODE_ENDS

        END

