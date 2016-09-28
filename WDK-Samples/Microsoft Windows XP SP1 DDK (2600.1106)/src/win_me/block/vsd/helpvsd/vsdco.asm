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
	title	vsdco - helper vsd callback code
	name	hlpvsd.vxd
;===========================================================================
;
;   Module:
;		This module handles the normal callback of requests from the
;		layer below us.
;
;===========================================================================
;
;   Functional Description: - When we receive a request in the HLP_callback
;   -----------------------   entry point, we want to determine if we want to
;			      do something with it.
;
;			      If we don't, we just call the next layer up.
;			
;============================================================================
.386p

;============================================================================
;				I N C L U D E S
;============================================================================

.xlist

	include vmm.inc

	include	blockdev.inc	; general packet definitions
	include sgd.inc
	include ior.inc
	include iop.inc

	include	srb.inc
	include scsiport.inc	; SCSI packet definitions
	include scsi.inc

	include	debug.inc	; general debug macros

	include	med.inc		; needed for iodebug.inc
	include	iodebug.inc	; IOS assertion macros

	include	bpb.inc		; needed for dcb.inc
	include	dcb.inc

	include opttest.inc

.list

;============================================================================
;			E X T E R N A L   C O D E
;============================================================================

VXD_LOCKED_CODE_SEG

;============================================================================
;			    M A I N   C O D E
;============================================================================


;==============================================================================
;
; HLP_callback() - IOP completion callback entry into the helper VSD
;
;
; Entry: pointer to IOP on the stack
;
; Exit:
;
; Destroys:     keeps c callable + ebx, can destroy eax, ecx, edx
;
;==============================================================================

VSDR_Stack_Frame struc
	VSDR_Ret_Address dd ?
	VSDR_edi	 dd ?
	VSDR_esi	 dd ?
	VSDR_ebx	 dd ?
	VSDR_IOP_Ptr_Off dd ?
VSDR_Stack_Frame ends

beginproc HLP_callback

	; set up stack frame

	push	edi
	push	esi
	push	ebx

	mov	esi,[esp.VSDR_IOP_Ptr_Off]	; pick up pointer to the IOP

	AssertIOP <esi>

	TRAP

	; check for an error, if there is one jmp to the error handler
	; note: for SCSI devices, if this VSD is the layer above SCSIPORT.vxd
	; the IOR_status field is invalid ... The SRB status should be
	; checked instead

	cmp	[esi].IOP_ior.IOR_status, IORS_ERROR_DESIGNTR
	jae	error_found

	; check the function code, or the SRB to see if we need to do
	; something with the callback, if not, just fall through, and
	; continue the callback

	; no errors found, just callback the request to the next layer

	public	vsdcb_callback
vsdcb_callback:
	call	do_ior_callback

vsdcb_exit:

	;restore the stack

	pop	ebx
	pop	esi
	pop	edi

	ret

error_found:

	TRAP
	; we have an error in the command, parse the error

	call	VSD_Error_Handler		; yes, process the error.

vsdcb_done:

	; a command was handled by the error handler, or the appropriate
	; callback parser.
	;

	; esi is IOP

	AssertIOP <esi>

	TRAP

	; return code in al:
	; 0 => request complete ( callback the IOP)
	; 1 => resubmit iop in esi (calldown the IOP)
	; 2 => exit (no callback or calldown)
	;

	or	al, al		; done?
	je	vsdcb_callback	; if so, call back

	cmp	al, 2		; exit?
	je	vsdcb_exit	; if so, jump


ifdef debug
	cmp	al, 1		; check for invalid return code
	DEBUG_OUTne	"HELPERVSD: illegal callback return code! big trouble!"
endif

	AssertIOP<esi>

	; setup up the callback entry to the next available callback, leave
	; our callback as it was ...

	add	[esi].IOP_callback_ptr, size IOP_callback_entry

	; pick up the calldown entry, and call the next layer

	mov	eax, [esi].IOP_calldown_ptr	; get current calldown structure
	mov	eax, [eax].DCB_cd_next 		; get next calldown structure

ifdef DEBUG
	or 	eax,eax				; make sure the calldown is ok
	DEBUG_OUTz	"HELPVSD: calldown pointer is trashed for IOP #esi"
endif ;DEBUG

	mov	[esi].IOP_calldown_ptr, eax	; store in IOP

	push	esi				; place IOP on stack
	call	[eax].DCB_CD_IO_Address		; call next layer
	add	esp, 4				; restore stack
	jmp	vsdcb_exit

endproc HLP_callback

;==============================================================================
;
; vsd_error_handler() - handle any errors
;
;
; Entry: esi is IOP
;
; Exit: al set to 0 to indicate callback
;
; Destroys:     eax
;
;==============================================================================

beginproc vsd_error_handler

	TRAP

	xor	al, al
	ret

endproc vsd_error_handler

;==============================================================================
;
; do_ior_callback() - calls back the next layer with completion callback
;
;
; Entry: esi - IOP
;
; Exit:
;
; Destroys:     edi
;
;==============================================================================

beginproc	do_ior_callback

	AssertIOP	<esi>

	TRAP

	;Call next level in CallBack Stack

	mov	edi, [esi].IOP_callback_ptr
	sub	edi, size IOP_CallBack_Entry	;point to next available
						;callback entry
	mov	[esi].IOP_callback_ptr, edi	;update CallBack Pointer

	;IOP pointer is passed on the stack

	push	esi			;IOP's offset
	call	dword ptr [edi]		;make the call
	add	esp, 4			;restore stack

	ret

endproc	do_ior_callback



VXD_LOCKED_CODE_ENDS

end
