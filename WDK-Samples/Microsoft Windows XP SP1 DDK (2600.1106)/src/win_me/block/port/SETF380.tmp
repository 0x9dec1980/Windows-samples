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
	title	portreq - port driver VxD initialization code
	name	port.pdr
;===========================================================================
;
;   Module:
;	    This module contains code that will handle normal request path
;	    IOPs.  This module contains the main calldown entry point for the
;	    port driver.
;
;===========================================================================
;
;   Functional Description: - When we receive a request at the HLP_request entry
;   -----------------------   point, we pick up the IOP, and examine it, to
;			      determine if we want to do something with it.
;			      If we don't understand it, we will call the next
;			      layer below with the packet.
;
;============================================================================

;============================================================================
;				I N C L U D E S
;============================================================================
.386p

	.xlist
	include vmm.inc
	include blockdev.inc    ; needed for IOP, and IOR definitions
	include dcb.inc         ;DCB

	include	ddb.inc	   	; ddb definition
	include	portddb.inc	; port driver specific ddb definition

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

;============================================================================
;			    M A I N   C O D E
;============================================================================


PAGE
SUBTTL Port_request

;==============================================================================
;
; Port_request() - Main request entry point in the port driver.  This routine
;		  looks at the incoming IOP, and dispatches to an internal
;		  routine if we want to do anything with the packet.
;		  Otherwise, we just pass it on.
;
; Entry:  pointer to IOP on stack
;
; Exit:
;
; Destroys:  C call compatible, can only destroy eax, ecx, edx
;
;==============================================================================

BeginProc	Port_request

ArgVar	IOP_ptr
	
	EnterProc
        mov     ebx, IOP_Ptr         ; point to the iop.

	AssertIOP <ebx>

; sanity check

.ERRNZ  IOR_READ   - 0
.ERRNZ  IOR_WRITE  - 1
.ERRNZ  IOR_VERIFY - 2

	;
	; in this sample driver, we only support read, write and verify
        ; commands.
	;

	cmp	[ebx].IOP_ior.IOR_func, IOR_VERIFY
	ja	port_r_not_io

        mov     edi,[ebx.IOP_calldown_ptr]      ; point to calldown entry
        mov     edi,[edi.DCB_cd_ddb]            ; get the ddb.

	AssertDDB <edi>

        mov     esi,[ebx.IOP_physical_dcb]      ; point to physical dcb.

	AssertDCB <esi>

	;
	; queue/sort this request on DCB queue.
	;

	push	esi			   ; *DCB
	push	ebx			   ; *IOP
        call    [port_ilb].ILB_enqueue_iop ; queue the request.
	add	esp, 4+4

        ;
        ; see if request is already processing.  if not, dequeue next.
        ;

	bts	[edi].DDB_port_flags, DDB_BF_ACTIVE_BIT
        jc      Port_r_exit

;
;  Entry point for starting requests from completion handler.
;

    public  Port_Start_Request
Port_Start_Request:

	AssertDCB <esi>
	
	push	esi	      		; *DCB
        call    [Port_ilb].ILB_dequeue_iop 
	add	esp, 4			; get a request

        or      eax, eax                ; check if queue empty
        jz      Port_r_exit	        ; if so, jump

	mov	ebx, eax
	AssertIOP <ebx>
        mov     ecx,[ebx.IOP_calldown_ptr]      ; point to calldown entry
        movsx   ecx, [ecx.DCB_cd_expan_off]     ; get the start of our expan.

        ;
        ; find and fill in IOP extension
        ;

        ; esi => DCB    
        ; ebx => IOP
        ; ecx = expansion offset
        ; edi => DDB

        add     ebx, ecx                ; point ebx to IOP expansion area

	;
	; if we had expansion space in the IOP, ebx would now point to it.
	;

;	call 	Port_Start_Hardware

Port_r_exit:
	LeaveProc
        Return

Port_r_not_io:

	;
	; call back the request with error
	;

	mov	[ebx].IOP_ior.IOR_status, IORS_INVALID_COMMAND
        mov     eax, [ebx].IOP_callback_ptr
        sub     eax, size IOP_CallBack_Entry    ;point to next available
                                                ;callback entry
        mov     [ebx].IOP_callback_ptr, eax     ;update CallBack Pointer

        ;IOP pointer is passed on the stack

        push    ebx                     ;IOP's offset
        call    dword ptr [eax]         ;make the call
        add     esp, 4                  ;restore stack
	jmp	Port_r_exit

EndProc	Port_request   


VXD_LOCKED_CODE_ENDS

end
