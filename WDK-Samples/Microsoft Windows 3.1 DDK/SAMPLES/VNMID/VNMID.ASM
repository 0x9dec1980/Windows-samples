PAGE 58,132
;******************************************************************************
TITLE VNMID - 
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1990
;
;   Title:      VNMID.ASM - 
;
;   Version:    3.00
;
;   Date:       8-Oct-1990 
;
;   Author:     Neil Sandlin
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
        .LIST

;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VNMID, 3, 0, VNMID_Control, Undefined_Device_ID ,,,


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_LOCKED_DATA_SEG

NMI_Chain   dd      ?

VxD_LOCKED_DATA_ENDS


;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
;   VNMID_Device_Init
;
;   DESCRIPTION:
;       This routine installs the NMI handler.
;
;
;==============================================================================

BeginProc VNMID_Device_Init

IFDEF DEBUG
        Trace_Out "VNMID installed."
ENDIF
        
        VMMcall Get_NMI_Handler_Addr            ; get current handler
        mov     [NMI_Chain], esi
   
        mov     esi, OFFSET32 VNMID_Handler       ; setup up our NMI handler
        VMMcall Set_NMI_Handler_Addr
   
        mov     esi, OFFSET32 VNMID_Event_Handler ; setup our event handler
        VMMcall Hook_NMI_Event

        clc
        ret

EndProc VNMID_Device_Init

VxD_ICODE_ENDS



;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VNMID_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation.
;
;
;==============================================================================

BeginProc VNMID_Control

        Control_Dispatch Device_Init, VNMID_Device_Init
        clc
        ret

EndProc VNMID_Control


;******************************************************************************
;
;   VNMID_Handler
;
;   DESCRIPTION:
;
; This handler is called at NMI time, so what can be done here is
; limited. The following comes from the "Virtual Device Adaptation Guide":
;
; Notice that you NMI interrupt handler can only touch local data in the
; device's VxD_LOCKED_DATA_SEG. It cannot touch memory in a VM handle,
; V86 memory, or any other memory. It also cannot call ANY services,
; INCLUDING services that can be called during normal hardware interrupts.
; Because an NMI can occur at any time, it is difficult to do much of
; anything during interrupt time that is guaranteed not to reenter a
; non-reentrant procedure or affect a data structure.
;
;
;******************************************************************************
BeginProc VNMID_Handler

;       Code inserted here would be executed in the event of an NMI

        jmp     [NMI_Chain]                     ; chain to next 

EndProc VNMID_Handler
   

VxD_LOCKED_CODE_ENDS


VxD_CODE_SEG

;******************************************************************************
;
;   VNMID_Event_Handler
;
;   DESCRIPTION:
;
;   This event handler is called sometime after the interrupt handler
;   was called, when VMM has had a chance to get everything in order.
;   Here, services can be called as required.
;
;   ENTRY:
;
;
;******************************************************************************
   
   
BeginProc VNMID_Event_Handler

IFDEF DEBUG
        Trace_Out "VNMID: Event received"
ENDIF

        clc
        ret

EndProc VNMID_Event_Handler


VxD_CODE_ENDS

        END

