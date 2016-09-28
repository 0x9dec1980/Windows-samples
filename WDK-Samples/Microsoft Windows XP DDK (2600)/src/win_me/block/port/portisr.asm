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

;===========================================================================
	page	,132
	title	portisr - port driver VxD initialization code
	name	port.pdr
;===========================================================================
;
;   Module:
;	    This module contains port driver's ISR.
;
;===========================================================================
;
;   Functional Description: - We receive the DDB for the adapter from VPICD,
;   -----------------------   and handle the interrupt.
;
;============================================================================

;============================================================================
;				I N C L U D E S
;============================================================================
.386p

	.xlist
	
	include vmm.inc
	include	ddb.inc	   	; ddb definition
	include	portddb.inc	; port driver specific ddb definition

	include blockdev.inc    ; needed for IOP, and IOR definitions
	include dcb.inc         ;DCB

	include sgd.inc		; Scatter Gather descriptors

	include	srb.inc
	include scsiport.inc	; General Scsi definitions
	include scsi.inc

	include debug.inc	; used for debug macros like DEBUG_OUT

	include med.inc		; required for iodebug.inc
	include iodebug.inc     ; various IOS assertions (like AssertIOP)

	include ior.inc		; command packet definitions
	include iop.inc

;;	include error.inc       ; ioctl error codes
	include opttest.inc     ; memory test functions

	include ilb.inc		; used for internal requests

	include isp.inc		; needed for internal requests

	include	vpicd.inc	; VPICD interface
	.list


;============================================================================
;			E X T E R N A L   D A T A
;============================================================================

VXD_LOCKED_DATA_SEG

; data in port.asm

	extrn port_ilb:dword	; linkage block to call IOS for internal
				; requests, memory allocation, etc.

VXD_LOCKED_DATA_ENDS

;============================================================================
;			E X T E R N A L   C O D E
;============================================================================

VXD_LOCKED_CODE_SEG

; code in portreq.asm

	extrn Port_Start_Request:near  ; request start routine

;============================================================================
;			    M A I N   C O D E
;============================================================================


PAGE
SUBTTL Port_irq_handler

;==============================================================================
;
; Port_irq_handler() - ISR for the port driver
;
; Entry:  edx points to the DDB
;
; Exit:
;
; Destroys:  all but ebp
;
;==============================================================================

BeginProc	Port_irq_handler

	EnterProc
	AssertDDB <edx>
	mov	edi, edx

	;
	; issue EOI after making sure interrupt is ours
	;

        mov   	eax, [edi].DDB_irq_handle
	VxDCall	VPICD_Phys_EOI

	;	
	; process the interrupt.  if the request has completed, call 
	; routine to start next request and call this one back up.
	;

;	call	Port_process_interrupt
	jc	Port_i_h_exit		; if request not done, exit

	call	Port_complete_request	; try to start more work

Port_i_h_done:
	clc

Port_i_h_exit:
	LeaveProc
	Return        

Port_i_h_unexpected_int:
	stc
	jmp	Port_i_h_exit

EndProc	Port_IRQ_Handler 


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                                       
; 	Port_Complete_Request
;		handle IOP completetion
;                                        
;       INPUT:  edi => ddb                              
;		esi => IOP
;
;	OUTPUT:	none
;
;	DESTROYS: 
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc	Port_Complete_Request

        ;
        ; try to start next request
        ;

	AssertDDB <edi>
	AssertIOP <esi>

	push	esi				; save completing IOP
        mov     esi, [esi].IOP_physical_dcb
        AssertDCB <esi>

        call    Port_Start_Request      	; start next I/O

        ;
        ; call back completing IOP.
        ;

	mov	ebx, [esp]			; recover completing IOP
	AssertIOP <ebx>

        mov     eax, [ebx].IOP_callback_ptr
        sub     eax, size IOP_CallBack_Entry    ;point to next available
                                                ;callback entry
        mov     [ebx].IOP_callback_ptr, eax     ;update CallBack Pointer

        ;IOP pointer is passed on the stack

        call    dword ptr [eax]         	;make the call
        add     esp, 4                  	;restore stack
	Return

EndProc	Port_Complete_Request 

VXD_LOCKED_CODE_ENDS

end
