PAGE 58,132
;******************************************************************************
TITLE VXD - GPTrap
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1991
;
;   Title:      GPTRAP - executes INT1 on invalid GP fault
;
;   Version:    3.00
;
;   Date:       1-Jun-1991 
;
;   Author:     Neil Sandlin
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
        .LIST


;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device GPTrap, 3, 0, GPTrap_Control, Undefined_Device_ID ,,,


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_LOCKED_DATA_SEG

OldGPFaultHandler dd    ?

VxD_LOCKED_DATA_ENDS




;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
;   GPTrap_Sys_Critical_Init
;
;   DESCRIPTION:
;       This routine sets up the GP fault handler. Note that this
;       must be called at Sys_Critical_Init in order for it to be
;       hooked AFTER the standard GP fault handler. This way, we
;       only get control when a BAD gp fault happens.
;
;
;==============================================================================

BeginProc GPTrap_Sys_Critical_Init


        mov     esi, OFFSET32 MyGPFaultHandler
        mov     eax, 0dh                        ; gp faults
        VMMCall Hook_PM_Fault
        mov     [OldGPFaultHandler], esi

        clc
        ret
EndProc GPTrap_Sys_Critical_Init

VxD_ICODE_ENDS


;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   GPTrap_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation. 
;
;
;==============================================================================

BeginProc GPTrap_Control

        Control_Dispatch Sys_Critical_Init, GPTrap_Sys_Critical_Init
        clc
        ret

EndProc GPTrap_Control

;******************************************************************************
;
;   MyGPFaultHandler
;
;   DESCRIPTION:
;       This is the actual gp fault handler. It does only a debug
;       break to get control to the debugger.
;
;==============================================================================

BeginProc MyGPFaultHandler

        int     1                       ;DEBUG BREAK
        jmp     DWORD PTR [OldGPFaultHandler]

EndProc MyGPFaultHandler


VxD_LOCKED_CODE_ENDS


        END

