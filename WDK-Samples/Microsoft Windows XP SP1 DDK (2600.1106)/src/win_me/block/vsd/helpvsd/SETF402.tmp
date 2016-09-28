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
	title	helpreq - helper vsd VxD initialization code
	name	helpvsd.vxd
;===========================================================================
;
;   Module:
;	    This module contains code that will handle normal request path
;	    IOPs.  This module contains the main calldown entry point for the
;	    VSD.
;	    It also contains code to generate a new request based on an
;	    incoming request
;
;===========================================================================
;
;   Functional Description: - When we receive a request at the HLP_request entry
;   -----------------------   point, we pick up the IOP, and examine it, to
;			      determine if we want to do something with it.
;			      If we don't understand it, we will call the next
;			      layer below with the packet.
;
;			      If we want to submit a new command in response
;			      to the incoming command (e.g. read a partition
;			      table in response to a COMPUTE_GEOM request) this
;			      module contains all the code necessary to alloc
;			      a new packet, set it up, and call in the request.
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

	include sgd.inc		; Scatter Gather descriptors
	include	srb.inc
	include scsiport.inc	; General Scsi definitions
	include scsi.inc

	include debug.inc	; used for debug macros like DEBUG_OUT

	include med.inc		; required for iodebug.inc
	include iodebug.inc     ; various IOS assertions (like AssertIOP)

	include ior.inc		; command packet definitions
	include iop.inc

	include opttest.inc     ; memory test functions

	include ilb.inc		; used for internal requests

	include isp.inc		; needed for internal requests

	include vsddefs.inc	; local definitions of constants

	.list

ERROR_GEN_FAILURE	EQU	31

;============================================================================
;			E X T E R N A L   D A T A
;============================================================================

VXD_LOCKED_DATA_SEG

; data in helpvsd.asm

	extrn vsd_ilb:dword	; linkage block to call IOS for internal
				; requests, memory allocation, etc.

VXD_LOCKED_DATA_ENDS

;============================================================================
;			E X T E R N A L   C O D E
;============================================================================

VXD_LOCKED_CODE_SEG

; code in vsdco.asm

	extrn   HLP_callback:near ; entry point for the callback of requests


;============================================================================
;			    M A I N   C O D E
;============================================================================


PAGE
SUBTTL HLP_request

;==============================================================================
;
; HLP_request() - Main request entry point in the helper VSD.  This routine
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

; stack frame for accessing the incoming IOP

HLPR_Stack_Frame struc
	VSDR_EBP_SAVE    dd ?
	VSDR_Ret_Address dd ?
	VSDR_IOP_Ptr_Off dd ?
HLPR_Stack_Frame ends

; dispatch table for dispatching incoming IOPs based on function code

VSD_cmd_tbl:
	.errnz IOR_READ - ($-VSD_cmd_tbl) / 4	; sanity check
	dd      offset32 Setup_Unsupported_Cmd	; procedure for handling reads

	.errnz IOR_WRITE - ($ - VSD_cmd_tbl) / 4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for handling writes

	.errnz IOR_VERIFY - ($ - VSD_cmd_tbl) / 4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for verifies

	.errnz IOR_CANCEL - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for cancel function

	.errnz IOR_WRITEV - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for write verifies

	.errnz IOR_MEDIA_CHECK - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for determing media
						; status

	.errnz IOR_MEDIA_CHECK_RESET - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procefure for determing status
						; and resetting the device

	.errnz IOR_LOAD_MEDIA - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for setting up a
						; media load function

	.errnz IOR_EJECT_MEDIA - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for setting up an
						; eject media function

	.errnz IOR_LOCK_MEDIA - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for setting up a
						; lock function

	.errnz IOR_UNLOCK_MEDIA - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for setting up an
						; unlock media function

	.errnz IOR_REQUEST_SENSE - ($ -VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for determing media
						; error sensing (SCSI only)

	.errnz IOR_COMPUTE_GEOM - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for a compute
						; geometry call

	.errnz IOR_GEN_IOCTL - ($ - VSD_cmd_tbl) /4
	dd      offset32 Setup_Unsupported_Cmd	; procedure for generic IOCTLs

VSD_CMD_TBL_LEN equ ($ - offset32 VSD_cmd_tbl) / 4


beginproc HLP_request

	push    ebp             ;save frame

	mov     ebp,esp         ;set to entry frame

	push    edi             ;save for C callable
	push    esi

	; pick up the incoming IOP

	mov     ebx, dword ptr [ebp].VSDR_IOP_Ptr_Off
	AssertIOP <ebx>		; make sure it is an IOP

	TRAP

	;
	; insert our callback in the callback stack as default for all requests
	; If we end up handling this request, and we want to call it back as
	; complete, we will remove this entry before calling back

	mov     eax, [ebx.IOP_callback_ptr]                    ; set Callback
	mov     [eax.IOP_cb_address],offset32 HLP_callback     ; pointer

	; use the calldown pointer as reference data in the callback entry
	; so that we can find the offset in the IOP to our scratch section

	mov     edx, [ebx.IOP_calldown_ptr]                    ; get CD ptr
	mov     [eax.IOP_cb_ref_data], edx                     ; set reference

	; add a callback entry to the callback stack, for the next layer

	add     [ebx.IOP_callback_ptr],size IOP_callBack_entry ; move down

	; pick up the DCB for use in the request handlers

	mov     esi,[ebx].IOP_physical_dcb      ;ESI -> DCB
	AssertDCB <esi>

	; for helper CDVSD, we want to see if the SRB_VALID flag is set.
	; if it is, we want to ignore the function code, and check the
	; Scsi Request Block (SRB) to see if the function code in the
	; scsi command block is a function code that we interpret differently
	; for the CD drive that we support

	TestMem	[ebx].IOP_ior.IOR_flags, IORF_SRB_VALID
	TRAPnz

	jnz	hlpr_check_SRB

	; pick up the function code

	movzx   eax, [ebx].IOP_ior.IOR_func

	; at this point we call a dispatch routine for any functions that
	; we support

	; check if function is above the range that we support

	cmp     eax, VSD_CMD_TBL_LEN
	jae     hlpr_calldown		; we don't understand it, just let the
					; command continue

	;call the appropriate function handler.
	; at this point:
	; ebx = IOP
	; esi = DCB

	AssertIOP <ebx>
	AssertDCB <esi>

	call    [eax * 4 + offset32 VSD_cmd_tbl]

public hlpr_check_if_handled
hlpr_check_if_handled:

	TRAP

	; this is the entry point for check_srb, and continuation of commands
	; that used the normal dispatch table

	; ebx is pointer to IOP

	AssertIOP <ebx>

	; esi is pointer to DCB

	AssertDCB <esi>

	; al is a dispatch return code, defined as follows:
	;   al = 0, continue to calldown the request
	;   al = 1, callback to previous layer
	;   al = 2, return without calling back or continuing.  Used when the
	;           handler scheduled an event to do the actual work

	or      al,al
	jnz     hlpr_cmd_complete

	; Call next level in CallDown Stack

hlpr_calldown:
	TRAP

	AssertIOP<ebx>

	mov     eax, [ebx].IOP_calldown_ptr     ; get call down address
	mov     eax, [eax].DCB_cd_next          ; get next calldown entry

ifdef DEBUG
	or      eax, eax			; sanity check, make sure the
						; calldown is ok

	; print a warning message in debug code if we are in trouble

	DEBUG_OUTz	"HELPERVSD: calldown is trashed on IOP #ebx"	
endif ;DEBUG

	mov     [ebx].IOP_calldown_ptr, eax     ; reset calldown pointer

	push    ebx                             ; place IOP on stack
	call    [eax].DCB_CD_IO_Address         ; call next layer
	add     esp, 4                          ; restore stack

hlpr_exit:
	pop     esi				; restore the stack frame
	pop     edi

	pop     ebp

	ret


hlpr_cmd_complete:

	; at this point, we know that the command should not be called down,
	; check to see if we should call the command back, or just exit

	; ebx should be IOP

	AssertIOP <ebx>

	TRAP

	; al = 1 indicates callback, al = 2 indicates just return

	dec     al
	jnz     hlpr_eat_callback		; jump if this was not callback

	; callback the  layer above us with this command, as we implemented
	; the entire functionality for it in this layer.

	mov     eax, [ebx].IOP_callback_ptr	; pick up the callback pointer

	sub     eax, 2 * size IOP_callback_entry; override the callback that
						; was automaticaly setup at the
						; start of HLP_request
						; and pick up the callback ptr
						; that was set just prior to
						; calling HLP_request

	mov     [ebx].IOP_callback_ptr, eax     ; update CallBack Pointer in IOP

	push    ebx                     ;IOP as stack argument
	call    dword ptr [eax]         ;make the call to the layer above us
	add     esp, 4                  ;restore stack

	jmp     hlpr_exit		; we're done!

hlpr_eat_callback:

	; on entry here, we should be trying to just exit this routine without
	; a callback or a calldown.  Verify that the dispatch return code is
	; valid first.

	; al should be 1 here

	TRAP

	dec     al
	jz      hlpr_exit               ;request is ok, just return

	; request code has been mangled.  print a warning message, and error
	; the incoming command as invalid.  We should never get here.

	; set the return status as callback

	mov	al, 1

	DEBUG_OUT       "CDVSD: illegal return code! Big Trouble!"

	; indicate that there was a serious error in this command, and call
	; it back.

	mov     [ebx].IOP_ior.IOR_status, IORS_INVALID_COMMAND ; set the status

	; if this was an ioctl, error the ioctl return as well

	cmp     [ebx].IOP_ior.IOR_func, IOR_GEN_IOCTL
	jne     hlpr_cmd_complete		; not an ioctl, just return

	mov     [ebx].IOP_ior.IOR_ioctl_return, ERROR_GEN_FAILURE
	jmp     hlpr_cmd_complete

endproc HLP_request

;==============================================================================
;
; hlpr_check_SRB() - look in the SRB for a command code that we want to change
;
; Entry: ebx - IOP
;	 esi - DCB
;
; Exit:
;
; Destroys:     none
;
;==============================================================================

beginproc hlpr_check_SRB

	AssertIOP <ebx>
	AssertDCB <esi>

	TRAP

	; the flags indicate that this IOP has a valid SRB, so, pick it up

	mov	edx, [ebx].IOP_srb

	; verify that edi is indeed an SRB

	AssertSRB <edx>

	; pick up the CDB command code, and index into the SRB table
	; to see if we support it

	mov	al, byte ptr [edi].Cdb[0]

	; indicate calldown, continue the calldown

	xor	al, al

	jmp	hlpr_check_if_handled

endproc hlpr_check_SRB

;==============================================================================
;
; Setup_Unsupported_Cmd() - Setup routine for commands we don't understand
;
; Entry: ebx is IOP
;
; Exit: al set to 0 to indicate continued calldown
;
; Destroys:     eax
;
;==============================================================================

beginproc Setup_Unsupported_Cmd

	; indicate continued calldown of the request

	xor	al, al
	ret

endproc Setup_Unsupported_Cmd


PAGE
SUBTTL setup_internal_async_request

;==============================================================================
;
; setup_internal_async_request() - Allocates a new IOP, and sets up a new
;                                  SRB for an async internal request
;
; Entry: ebx is current IOP
;        esi is DCB
;
; Exit: carry set - No memory available, request fails
;
;       carry clear - success, parameters as follows
;       edi is new SRB
;       ebx is new IOP
;               IOP_ior.IOR_requestor_usage[0] = handle of old IOP
;
; Destroys:     ebx, edi
;
;==============================================================================

beginproc setup_internal_async_request

	AssertIOP <ebx>

	TRAP

	push    ebx

	push    [ebx].IOP_calldown_ptr  ; save the calldown pointer from the current request

	; get a new IOP
	call    Vsd_GetIOP

	pop     eax             ;pick up the calldown pointer

	or      edi, edi        ; no IOP - This should never happen
	TRAPz
	stc                     ; presuppose failure
	jz      siar_no_mem


	AssertIOP <edi>
	AssertDCB <esi>

	; set the calldown pointer in the new IOP, used to find the new SRB!
	mov     [edi].IOP_calldown_ptr, eax

	; set the callback reference data as this IOP
	mov     [edi].IOP_ior.IOR_req_req_handle, edi


	; set function as pass through
	mov     [edi].IOP_ior.IOR_func, IOR_SCSI_PASS_THROUGH

	; set DCB so that meet-io-criteria has something to do
	mov     [edi].IOP_original_dcb, esi

	; pickup SRB for request
	mov     ebx, edi
	call    vsd_setup_srb

	AssertIOP <ebx>
	AssertDCB <esi>
	AssertSRB <edi>

	; pick up the old IOP
	pop     [ebx].IOP_ior.IOR_requestor_usage[0]
	AssertIOP <[ebx].IOP_ior.IOR_requestor_usage[0]>

	; set the lun, etc
	mov     al,[esi].DCB_SCSI_LUN
	shl     al, 5
	mov     byte ptr [edi].Cdb[1],al

	clc

siar_no_mem:
	ret

endproc setup_internal_async_request

;==============================================================================
;
; vsd_ret_iop() - releases an IOP
;
; Entry: - ebx is DCB
;        - edi is IOP
;
; Exit: IOP is dealloced
;
; Destroys:     none
;
;==============================================================================

beginproc vsd_ret_iop

	AssertDCB <ebx>
	AssertIOP <edi>

	TRAP

	sub     edi, [ebx].DCB_expansion_length ; sub expansion length
	sub     esp, size ISP_mem_dealloc       ;
	mov     esi, esp                        ; release
	mov     [esi].ISP_mem_ptr_da, edi       ;
	mov     [esi].ISP_func, ISP_dealloc_mem ; IOP
						;
	push    esi                             ;
	call    [vsd_ilb.ILB_Service_rtn]       ;

	add     esp, size ISP_mem_dealloc + 4   ; clean up the stack and arg

	ret


endproc vsd_ret_iop


PAGE
SUBTTL Vsd_GetIOP

;==============================================================================
;
; Vsd_GetIOP() - Allocates a new IOP for internal requests
;
;
; Entry: esi - DCB logical
;
;
; Exit: new IOP ptr in edi
;
; Destroys:     ecx,eax,edi
;
;==============================================================================

beginproc Vsd_GetIOP

	push    esi

	TRAP

	mov     esi, [esi].DCB_physical_dcb

	mov     ecx, [esi].DCB_expansion_length ; get expansion length
	add     ecx, IOP_ior
	sub     esp, size ISP_iop_create        ; allocate isp from the stack
	mov     edi, esp                        ; point to the gotten isp

	mov     [edi.ISP_delta_to_ior],ecx      ; set delta to IOR
	add     cx, size IOR + 2 * size SGD     ; get size of pkt
	mov     [edi.ISP_IOP_size], cx          ; create
	mov     [edi.ISP_i_c_flags], 0          ; indicate blocking ok.
	mov     [edi.ISP_func], ISP_create_iop  ; an
						; the
	push    edi                             ; specified
	call    [vsd_ilb.ILB_Service_rtn]       ; size

	mov     edi, [edi].ISP_IOP_ptr          ; EDI -> IOP (linear) alloc-ed

	add     esp,size ISP_IOP_create + 4     ; cleanup the stack and arg

	mov     eax, edi
	or      edi, edi                        ; check for no mem
	TRAPz
	jz      vsd_getiop_exit

	add     edi, [esi].DCB_expansion_length ; delta to IOP

	xor     eax, eax

	mov     [edi].IOP_ior.IOR_next, eax
	mov     [edi].IOP_ior.IOR_req_vol_handle, eax   ; zero vrp field
	mov     [edi].IOP_ior.IOR_xfer_count, eax       ; make xfer count 0
	mov     [edi].IOP_ior.IOR_flags, (IORF_HIGH_PRIORITY \
		 OR IORF_BYPASS_VOLTRK OR IORF_VERSION_002 or IORF_BYPASS_QUEUE \
		 or IORF_SRB_VALID)

	pop     esi

	mov     al, [esi].DCB_drive_lttr_equiv  ;pick up the drive letter
	mov     [edi].IOP_ior.IOR_vol_designtr, al  ; set unit # in request.\

	; set the IOR_sgd_lin_phys pointer
	mov     eax, edi
	add     eax, size IOR + IOP_ior
	mov     [edi].IOP_ior.IOR_sgd_lin_phys, eax

vsd_getiop_exit:
	ret

endproc vsd_getiop

;=========================================================================
;
; vsd_internal_request - build IOP/IOR and issue request to IOS. note that
;                        request will bypass vsd unless special entry
;                        point below is called.
;
;       INPUT:  AL = IOR function code
;               EBX = logical DCB
;
;       OUTPUT: AL = IOR status
;
;=========================================================================

BeginProc       vsd_internal_request

	pushad

	TRAP

	mov     esi, [ebx].DCB_physical_dcb

	;bugbug rr - use getiop

	mov     ecx, [esi].DCB_expansion_length ; get expansion length
	add     ecx, IOP_ior
	sub     esp, size ISP_iop_create        ; allocate isp from the stack
	mov     edi, esp                        ; point to the gotten isp

	mov     [edi.ISP_delta_to_ior],ecx      ; set delta to IOR
	add     cx, size IOR                    ; get size of pkt
	mov     [edi.ISP_IOP_size], cx          ; create
	mov     [edi.ISP_i_c_flags], 0          ; no flags
	mov     [edi.ISP_func], ISP_create_iop  ; an iop of
						; the
	push    edi                             ; specified
	call    [vsd_ilb.ILB_Service_rtn]       ; size

	mov     edi, [edi].ISP_IOP_ptr          ; EDI -> IOP (linear) alloc-ed

	add     esp,size ISP_IOP_create + 4     ; cleanup the stack and arg

	;
	; if alloc failed, exit with error
	;

	mov     ecx, eax
	mov     eax, IORS_MEMORY_ERROR
	or      edi, edi
	TRAPz
	jz      vsd_ir_exit

	add     edi, [esi].DCB_expansion_length ; delta to IOP

	xor     edx, edx

	  mov   [edi].IOP_ior.IOR_req_vol_handle, edx   ; zero vrp field

	mov     [edi].IOP_ior.IOR_xfer_count, edx       ; make xfer count 0

	mov     [edi].IOP_ior.IOR_func, cx              ; set function code

	mov     [edi].IOP_ior.IOR_flags, (IORF_HIGH_PRIORITY OR IORF_SYNC_COMMAND\
		 OR IORF_BYPASS_VOLTRK OR IORF_VERSION_002 or IORF_BYPASS_QUEUE \
		 or IORF_SRB_VALID)

	mov     edx, offset32 HLP_request	    ; use this value of edx
						    ; for sending a request
						    ; that goes directly to
						    ; the layer beneath us

	xor     edx, edx			    ; use this value of edx
						    ; to send a request to
						    ; all layers

	mov     al, [esi].DCB_drive_lttr_equiv      ; get the unit number

	mov     [edi].IOP_ior.IOR_vol_designtr, al  ; set unit # in request.

	;
	; edi => IOP
	; ebx => DCB
	; edx => our calldown entry.  Indicate calldown after us ...
	;

	call    [vsd_ilb].ILB_internal_request  ; Since it is a sync. cmd
						; when we return I/O is done

	movzx   eax, [edi].IOP_ior.IOR_status   ; pick up status

	sub     edi, [ebx].DCB_expansion_length ; sub expansion length
	sub     esp, size ISP_mem_dealloc       ;
	mov     esi, esp                        ; release
	mov     [esi].ISP_mem_ptr_da, edi       ;
	mov     [esi].ISP_func, ISP_dealloc_mem ; IOP
						;
	push    esi                             ;
	call    [vsd_ilb.ILB_Service_rtn]       ;

	add     esp, size ISP_mem_dealloc + 4   ; clean up the stack and arg

vsd_ir_exit:
	mov     [esp].Pushad_EAX, eax           ; store status in frame
	popad
	ret

EndProc vsd_internal_request

;==============================================================================
;
; vsd_setup_srb() - takes an IOP, and prepares an SRB  returns SRB pointer
;                   in edi
;
; Entry: IOP in ebx
;
; Exit: edi is SRB
;       Srb base information is set, and all relevant IOP pointers are set
;
; Destroys:     eax, edi, edx
;
;==============================================================================

beginproc vsd_setup_srb

	push    edx

	TRAP

	mov     eax, [ebx].IOP_calldown_ptr
	movsx   edi, [eax].DCB_cd_expan_off
	add     edi, ebx

	; ebx => IOP
	; edi => SRB area
	; esi => dcb

	;Put IOP pointer into SRB
	mov     [edi].SrbIopPointer,ebx

	;
	; since the length field is defined as Length, which is asm directive
	; we will just assume that Length is the first word in the struct.
	;
	mov     word ptr [edi], size SCSI_REQUEST_BLOCK  ; set size of srb
	mov     [edi].Function, SRB_FUNCTION_EXECUTE_SCSI; set function type
	mov     [edi].SrbStatus, SRB_STATUS_PENDING      ; set srb status
	mov     [ebx].IOP_srb, edi                       ; set SRB ptr
	xor     eax, eax
	mov     [edi].OriginalRequest, eax
	mov     [edi].SrbNextSrb, eax                    ; bugbug
	mov     [edi].NextSrb, eax
	mov     [edi].SrbRetryCount, al                  ; no retries, yet.
	mov     [edi].QueueTag, al                       ; bugbug bgp-
	mov     [edi].QueueAction, al                    ; are these
	mov     [edi].DataBuffer, eax                    ; necessary?


	AssertDCB <esi>
	;setup the SRB target address numbers

	mov     al, [esi].DCB_bus_number                 ; set bus number
	mov     [edi].PathId, al                         ;

	mov     al, [esi].DCB_scsi_target_id ; set target ID and
	mov     [edi].TargetId, al                       ; lun
	mov     al, [esi].DCB_scsi_lun                   ;
	mov     [edi].Lun, al                            ;

	;
	; set miniport extension
	;

	mov     edx, edi
	add     edx, size PORT_SRB
	mov     [edi].SrbExtension, edx

	;
	; set sense info buffer pointer
	;

	movzx   eax, [esi].DCB_srb_ext_size
	add     edx, eax
	mov     [edi].SenseInfoBuffer, edx
	mov     [edi].SenseInfoBufferLength, VSD_REQ_SENSE_SIZE
	pop     edx

	ret

endproc vsd_setup_srb

;==============================================================================
;
; vsd_send_internal_request() - send down an async internal request
; vsd_send_criteria_request() - send down a request after calling criteria
;
;
; Entry: ebx - IOP
;		IOP_ior.IOR_callback - should be set to the callback routine
;				       for this async request
;        esi - DCB
;        edi - SRB
;
; Exit:
;       command called in,
;       edi - SRB
;       ebx - IOP
;       esi - DCB
;
; Destroys:     eax, ecx, edx  (also see exit for register switches)
;
;==============================================================================

beginproc vsd_send_criteria_request

	; call the criteria code to allow IOS to fix up any memory alignment
	; constraints

	AssertIOP <ebx>
	AssertDCB <esi>

	TRAP

	push    ebx
	call    [vsd_ilb.ILB_int_io_criteria_rtn]

	add     esp, 4

	AssertIOP <ebx>
	AssertDCB <esi>

	; status in al if zero, we are ok, if not, we must double buffer
	or      al,al
	jz      vscr_no_double_buffer

	; read needs to be double buffered, set flag
	or      [ebx].IOP_ior.IOR_flags, IORF_DOUBLE_BUFFER

vscr_no_double_buffer:
	; command is set up, call the command in

	AssertIOP <ebx>
	AssertSRB <edi>
	AssertDCB <esi>

public vsd_send_internal_request
vsd_send_internal_request:


	TRAP

	; this is the entry point to send a request that does not require
	; the criteria code to be called.  For example, and request that does
	; not involve a data buffer, also the send in point for requests that
	; used the criteria code already.

	; esi -> DCB
	; ebx -> IOP

	AssertDCB       <esi>
	AssertIOP       <ebx>

	;mov    edx, offset32 HLP_request       ; indicate call layer after us
	xor     edx, edx			; or indicate call everybody

	mov     [edi].SrbStatus,SRB_STATUS_PENDING      ; reset SRB status

	mov     cx, [ebx].IOP_Timer_Orig                ; reset timer so the
	mov     [ebx].IOP_Timer, cx			; command won't timeout

	; rearrange the parameters for the call in to IOS

	; what we're looking for when we call IOS is:
	; edi => IOP
	; edx => zero, => start from top of calldown table.
	;


	xchg    edi,esi         ;save SRB in esi
	xchg    ebx,edi


	AssertIOP <edi>
	AssertDCB <ebx>
	AssertSRB <esi>

	; set up the req_req_handle as a pointer to the IOP, this pointer will
	; be on the stack, when the procedure set in IOP_ior.IOR_callback
	; is called back upon completion of the command

	mov     [edi].IOP_ior.IOR_req_req_handle, edi	

	call    [vsd_ilb].ILB_internal_request  ; call it down

	; we're done, just return
	ret

endproc vsd_send_criteria_request

VXD_LOCKED_CODE_ENDS

end
