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
	title	aer - helper vsd IOS asyncronous event routines
	name	helpvsd.vxd
;===========================================================================
;
;   Module:
;		This module handles all asyncronous event communications with
;		IOS.  Asyncronous events are not normal requests.  These are
;		specific function calls generated from IOS, not external
;		sources
;
;===========================================================================
;
;   Functional Description: - we pick up the asyncronous event packet (AEP)
;   -----------------------   and dispatch it according to the AEP_FUNC
;			
;============================================================================

;============================================================================
;				I N C L U D E S
;============================================================================
.386p

.xlist
	include vmm.inc

	include	blockdev.inc
	include	bpb.inc

	include ilb.inc 	;IOS linkage block

	include aep.inc 	;Async event packet

	include ddb.inc 	;Driver information block
	include dcb.inc 	;DCB

	include isp.inc 	;IOS Service Block

	include	scsidefs.inc	;SCSI definitions
	include scsi.inc
	include	scsiport.inc

	include	debug.inc	; general debug macros

	include vsdinfo.inc	; general vsd information
.list

;============================================================================
;			  D A T A   S E G M E N T
;============================================================================

;============================================================================
;	                 P A G E A B L E   D A T A
;============================================================================

VXD_PAGEABLE_DATA_SEG

; helpvsd_device_count keeps track of the number of devices that we are being
; called for

public hlpvsd_device_count
	hlpvsd_device_count	db 0

VXD_PAGEABLE_DATA_ENDS

;============================================================================
;			E X T E R N A L   C O D E
;============================================================================

VXD_LOCKED_CODE_SEG

; code in helpvsd.asm
	extrn	vsd_ilb:dword		; IOS linkage block

; code in helpreq.asm
	extrn	HLP_request:near	; main request entry point

; output strings for the Async_Request handler

IFDEF PRINTF

invalid_aep	db	'VSDAER:   invalid aep follows', CAR_RET, LINE_FEED, BELL
		db	'%z', CAR_RET, LINE_FEED, CAR_RET, LINE_FEED, 0
table_full	db	'VSDAER: Error - DCB Call Down Table full', CAR_RET, LINE_FEED, BELL, 0

ENDIF

;==============================================================================
; 			     M A I N   C O D E
;==============================================================================

;==============================================================================
;
; Help_Async_Request()
;
;	Help_Async_Request basically jumps to the appropriate routine 	
;	indicated by the AER command in the Async Event Packet (AEP).	
;									
;
; Entry: pointer to AEP on stack
;
; Exit:
;	AEP_result set to success/fail
;
; Destroys:     all
;
;==============================================================================

VSD_aer_frame	struc

	dd	?		; callers 32 bit near return address.
b_a_aep_off	dd	?		;

VSD_aer_frame	ends

beginproc Help_Async_Request

	TRAP

;
; set up the pointer to the stack frame, point to the aep, obtain the
; function code from it, call the appropriate function handler, and
; return to our caller
;
	mov	ebx, [esp.b_a_aep_off]

	; assume success

	mov	[ebx.AEP_result], AEP_SUCCESS

	mov	si,[ebx.AEP_Func]	  ; pick up the aep's function code.

	cmp	si, AEP_CONFIG_DCB	  ; is it a config call?
	je	vsd_cfg_device		  ; if so, jump

	cmp	si, AEP_BOOT_COMPLETE	  ; is it init complete?
	je	vsd_init_complete	  ; if so, jump


;
; report that the function code is not supported by this VSD
;

IFDEF PRINTF

	push	size aep_header		; store the size to display.
	push	es			; store the pointer
	push	bx			; to the aep.
	push	cs			; display the
	push	OFFSET32 invalid_aep	; "invalid aep"
      	call	cs:[ILB_dprintf_rtn]	; message on the
	add	sp,4+6			; debug terminal.


ENDIF

	mov	[ebx.AEP_result],AEP_FAILURE; set result code to indicate error.

	ret

endproc Help_Async_Request

VXD_LOCKED_CODE_ENDS

;==============================================================================
;		 	 P A G E A B L E   C O D E
;==============================================================================


; all of the AEP routines that we are interested in handling are low frequency,
; and they are not called at interrupt time, so we will keep them in pageable
; code segment to reduce the working set size

VXD_PAGEABLE_CODE_SEG

;==============================================================================
;
; vsd_cfg_device() - Examines a DCB that is passed in to determine if we want
;		     to work with this device
;
;
; Entry: ebx -> Async Event Packet (AEP)
;
; Exit:
;
; Destroys:     none
;
;==============================================================================

beginproc vsd_cfg_device

	TRAP

	;ebx points to AEP

	push	esi				; save reg

	;Address the DCB

	mov	esi, [ebx].AEP_d_c_dcb		; pick up address of the DCB

	test	[esi].DCB_device_flags, DCB_dev_logical OR DCB_dev_rmm
	jnz	Vcd_ret			 	; if not physical DCB or RMM

	;Check to see if it is a device which should be handled by this VSD

	cmp	[esi].DCB_Bus_Type,DCB_Bus_SCSI
	jne	Vcd_ret			; Not our device (not a SCSI device)
	cmp	[esi].DCB_Device_Type,DCB_Type_CDROM
	jne	Vcd_Ret			; Not our device (not a SCSI CD-ROM)

Our_Device:

	; check to see if this device is from the company we're trying to
	; make work ...

	cmp	dword ptr [esi].DCB_vendor_id[0], OUR_COMPANY_NAME_LO
	jne	Vcd_Ret
	cmp	dword ptr [esi].DCB_vendor_id[4], OUR_COMPANY_NAME_HI
	jne	Vcd_Ret

	; check product_id (DCB_product_id) and revision levels here if required

	; we have a device.  Increase our device count

	inc	[hlpvsd_device_count]	; show one more device

	mov	eax,[esi].DCB_dmd_flags;(eax)=demand bits

	; change any demand flags that we support (none)

	; for a scsi-1 device, indicate now that we look like SCSI-2
	TRAP

	;put VSD info into calldown stack

vsd_cfg_insert:

	; save registers

	push	esi
	push	edi

	; set up parameters to allocate a packet for calling the ISP functions

	; eax is still the DCB_demand_flags

	mov	esi,edi
	sub	esp,size ISP_calldown_insert	;allocate ios service pkt (from stack)
	mov	edi,esp				;point to the gotten isp.

	mov	[edi].ISP_func,ISP_insert_calldown	; set the functions

	mov	[edi].ISP_i_cd_flags,eax		; indicate demands

	mov	[edi].ISP_i_cd_dcb, esi			; insert on this DCB

	mov	[edi].ISP_i_cd_req,OFFSET32 HLP_request	; our request entry

	mov	al, [ebx.AEP_lgn]		; get load group number
	mov	[edi].ISP_i_cd_lgn, al	        ; store in ISP

	mov	[edi].ISP_i_cd_ddb,0		; we don't look at ddbs

	; if we require scratch space in every IOP, we can add it here
	
	xor	eax, eax

ifdef SCRATCH
	add	eax, size SCRATCH_AREA
endif ;SCRATCH

	mov	[edi].ISP_i_cd_expan_len, ax

	push	edi				;ptr to packet. it is edi!
	call	cs:[vsd_ilb.ILB_Service_rtn] 	;make ios call
	add	esp,size ISP_calldown_insert+4	;cleanup the stack and return
	pop	edi
	pop	esi
	
vcd_ret:

	pop	esi			; restore reg
	ret				; return to our caller.

endproc vsd_cfg_device


;==============================================================================
;
; VSD_init_complete() - Called at initialization complete time.  If we want
;			to unload our driver (i.e. no devices found) We fail
;			this call.  Otherwise, we just return success
;
;
; Entry: ebx -> Async Event Packet
;
; Exit:
;
; Destroys:     all but ebp
;
;==============================================================================

beginproc VSD_init_complete

	TRAP

	cmp	[hlpvsd_device_count], 0		; any devices?
	jne	VSD_i_c_exit				; if, jump

	TRACE_OUT	"HELPVSD: no NEC devices in the system, unloading "
	mov	[ebx].AEP_result, AEP_FAILURE		; unload driver

VSD_i_c_exit:
	ret

endproc VSD_init_complete

VXD_PAGEABLE_CODE_ENDS

end
