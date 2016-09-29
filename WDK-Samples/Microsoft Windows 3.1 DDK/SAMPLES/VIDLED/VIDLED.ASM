PAGE 58,132
;******************************************************************************
TITLE VIdleD 
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1992
;
;   Title:      VIdleD.ASM 
;
;   Version:    3.10
;
;   Date:       21-Feb-1992
;
;   Author:     Bernie McIlroy
;
;   Description: This VxD demonstrates how to use the Call_When_Idle function.
;                WARNING: This VxD issues a Trace_Out call on every idle
;                call-back.  This VxD may impair system performance.
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
        INCLUDE VIdleD.Inc
        .LIST


;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VIdleD, 3, 0ah, VIdleD_Control, VIdleD_Dev_ID,, ,


;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
;   VIdleD_Device_Init
;
;   DESCRIPTION:
;       This routine installs the handler that will be called when
;       the system is idle.
;
;
;==============================================================================

BeginProc VIdleD_Sys_Critical_Init

IFDEF DEBUG
        Trace_Out "VIdleD installed"
ENDIF
        mov      esi, OFFSET32 VIdled_Idle_Handler
        VMMCall  Call_When_Idle

        clc
        ret
EndProc VIdleD_Sys_Critical_Init

VxD_ICODE_ENDS


VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VIdleD_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation.
;
;
;==============================================================================

BeginProc VIdleD_Control

        clc
        Control_Dispatch Sys_Critical_Init, VIdleD_Sys_Critical_Init
        ret

EndProc VIdleD_Control



;******************************************************************************
;
;   VIdleD_Idle_Handler
;
;   DESCRIPTION:
;
;       This function gets control when Windows detects an idle condition
;       in the system.
;
;   ENTRY:
;       EBX = System VM handle
;       EBP = Client register structure.  Return with carry set to pass the
;             call to the next handler.  Return with carry clear to "eat"
;             the call-back and indicate the System VM is not idle.
;
;
;==============================================================================
BeginProc VIdleD_Idle_Handler
    Trace_Out "VIdleD: System idle detected."
    or     [ebp.Client_Flags], CF_Mask ; set the client carry flag
    ret
EndProc VIdleD_Idle_Handler


VxD_LOCKED_CODE_ENDS


        END
