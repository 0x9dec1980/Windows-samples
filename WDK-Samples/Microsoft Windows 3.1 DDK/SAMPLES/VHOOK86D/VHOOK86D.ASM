PAGE 58,132
;******************************************************************************
Title VHOOK86D.ASM
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1992
;
;   Title:      VHOOK86D.ASM
;
;   Version:    3.10
;
;   Date:       14-Jan-1992
;
;   Author:     Todd Cole
;
;
; Description:
;      This is a simple VxD that will hook the V86 INT chain.
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV                 DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   29-Feb-1992 bjm General cleanup.
;==============================================================================
;******************************************************************************

	.386p

	.xlist
	include vmm.inc
	include debug.inc
	.list


;******************************************************************************
; declare virtual device
;==============================================================================

Declare_Virtual_Device VHook86D, 3, 0ah, VHook86D_Control, Undefined_Device_ID, \
		       Undefined_Init_Order,,


Hooked_Int                  equ      2fh  ; This is the interrupt we want
                                          ; to hook.
		     
;******************************************************************************
; Initialization Code
;==============================================================================

VXD_ICODE_SEG

BeginProc VHook86D_Sys_Crit_Init

    Trace_Out "VHook86D: Sys Critical Init."

   	mov     eax, Hooked_Int
   	mov     esi, OFFSET32 VHook86D_V86_Int_Handler
   	VMMCall Hook_V86_Int_Chain

    clc
    ret

EndProc VHook86D_Sys_Crit_Init

VXD_ICODE_ENDS


;******************************************************************************
; Code
;------------------------------------------------------------------------------
VXD_CODE_SEG


;******************************************************************************
; Control dispatch proc
;==============================================================================
BeginProc VHook86D_Control

   	Control_Dispatch Sys_Critical_Init, VHook86D_Sys_Crit_Init
   	clc
   	ret

EndProc VHook86D_Control


;--------------------------------------------------------------------
; VHook86D_V86_Int_Handler
;--------------------------------------------------------------------
BeginProc VHook86D_V86_Int_Handler

    pushad
   	mov     ax,[ebp.Client_AX]
   	mov     bx,[ebp.Client_BX]
   	mov     cx,[ebp.Client_CX]
   	mov     dx,[ebp.Client_DX]
   	mov     si,[ebp.Client_SI]
   	mov     di,[ebp.Client_DI]
   	Trace_Out "VHook86D: Got INT.  AX=#AX  BX=#BX  CX=#CX  DX=#DX  SI=#SI  DI=#DI"
    popad
	
   	stc   ; don't consume the interrupt
   	ret

EndProc VHook86D_V86_Int_Handler

;==============================================================================

VXD_CODE_ENDS

   	END
