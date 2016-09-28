;===========================================================================
	page	,132
	title	aer - IOS asyncronous event routines
	name	vsdaer.asm
;===========================================================================
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
; PURPOSE.
;
; Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.
;
;   Module:
;		This module handles all asyncronous event communications with
;		IOS.  Asyncronous events are not normal requests.  These are
;		specific function calls originate from IOS, not external
;		sources
;
;   Version:  0.001
;
;   Date:     July 18, 1995
;
;===========================================================================
;
;   Change log:
;
;     DATE    REVISION			DESCRIPTION
;   --------  --------	-------------------------------------------------------
;   3/02/95   Original
;
;===========================================================================
;
;   Functional Description: - we pick up the asyncronous event packet (AEP)
;   -----------------------   and dispatch it according to the AEP_FUNC
;			
;============================================================================
.386p

;============================================================================
;				I N C L U D E S
;============================================================================

.xlist
	include vmm.inc
	include vmmreg.inc
	include	blockdev.inc
	include ilb.inc 	;IOS linkage block
	include aep.inc 	;Async event packet
	include ddb.inc 	;Driver information block
	include dcb.inc 	;DCB
	include isp.inc 	;IOS Service Block
	include defs.inc
	include debug.inc	;general debug macros
	include vsdinfo.inc	;general vsd information 
.list


;============================================================================
;			 L O C K E D   D A T A
;============================================================================

VXD_LOCKED_DATA_SEG

; Move the "Driver Identification String" to VSDinit.asm


VXD_LOCKED_DATA_ENDS

;============================================================================
;	                 P A G E A B L E   D A T A
;============================================================================

VXD_PAGEABLE_DATA_SEG

Public	fDriverLoaded	; Insert 95-07-21(M9 Update)
fDriverLoaded	dw	0   ; 0=initialization succeeded, -1=failed

VXD_PAGEABLE_DATA_ENDS

;==============================================================================
; 			     M A I N   C O D E
;==============================================================================

VXD_LOCKED_CODE_SEG

;==============================================================================
;
; _3mode_Async_Request()
;
;	3mode_Async_Request basically jumps to the appropriate routine
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

beginproc _3mode_Async_Request

	;
	; set up the pointer to the stack frame, point to the aep, obtain
	; the function code from it, call the appropriate function handler,
	; and return to our caller
	;

	mov	ebx, [esp.b_a_aep_off]

	; assume success

	mov	[ebx.AEP_result], AEP_SUCCESS

	mov	si,[ebx.AEP_Func]	;pick up the aep's function code.

	cmp	si, AEP_INITIALIZE	;is it to initialize?
	je	vsd_initialize		;yes, jump

;;****** Insert 95-07-20(M9 Update)
	cmp	si, AEP_BOOT_COMPLETE	;is this boot-complete?
	je	vsd_boot_complete

        push    eax			; Just safe
        mov     ax,fDriverLoaded                ; Get init state
        or      ax,ax
        pop     eax
        jnz     vsd_init_failed                 ; Init failed.
;;****** End of Insert 

	cmp	si, AEP_CONFIG_DCB	;is it to configure DCB?
	je	vsd_config_dcb		;yes, jump

	cmp	si, AEP_CHANGE_RPM	;is it to change spindle speed
	je	vsd_change_rpm

;;; Moved after AEP_INITIALIZE
;;	cmp	si, AEP_BOOT_COMPLETE	;is this boot-complete?
;;	je	vsd_boot_complete
;;;

;= Insert (M9 Update) ================================================================
	cmp	si, AEP_QUERY_RPM	; is it to get spindle speed
	je	vsd_query_rpm
	
	cmp	si, AEP_CONFIG_3MODE	; is it to get 3mode info
	je	vsd_config_3mode
;= End of Insert (M9 Update) =========================================================

vsd_init_failed:

	mov	[ebx.AEP_result],AEP_FAILURE	; return failure
	ret

endproc _3mode_Async_Request

VXD_LOCKED_CODE_ENDS

;==============================================================================
;		 	 P A G E A B L E   C O D E
;==============================================================================

;
; all of the AEP routines that we are interested in handling are low frequency,
; and they are not called at interrupt time, so we will keep them in pageable
; code segment to reduce the working set size
;

VXD_PAGEABLE_CODE_SEG

;
; These two routines are written by OEM in the VSDOEM.ASM file.
;

extrn	Init3modeDrive:near
extrn	SetSpindleSpeed:near
;******** Insert 95-07-12(M9 Update)
extrn	Get3modeInfo:near
extrn	GetSpindleSpeed:near
;******** Endof insert 95-07-12(M9 Update)


;============================================================================
;
; vsd_initialize() -
;
;   This routine is called when the AER received the AEP_INITIALIZE request.
;   It checks the registry to see if this driver is selected by the installer
;   to remain resident.  If not, fail initialization and be unloaded by IOS.
;
; Entry: ebx -> Async Event Packet (AEP)
;
; Exit:
;
; Destroys:     none
;
;============================================================================

beginproc vsd_initialize

;******** 95-07-20(M9 Update)
;******* Move the registry chacking routine to VSDinit.asm

	mov	[ebx.AEP_result], AEP_SUCCESS

	ret

endproc vsd_initialize


;============================================================================
;
; vsd_boot_complete() -
;
;   This routine is called when this driver's initialization is complete.
;   The result from this call determines if the driver will remain loaded,
;   or be unloaded.
;
; Entry: ebx -> Async Event Packet (AEP)
;
; Exit:
;
; Destroys:     none
;
;============================================================================

beginproc vsd_boot_complete

	;
	; Return result of vsd_initialze()
	;

	push	eax
	mov	ax, fDriverLoaded
	mov	[ebx.AEP_result], ax
	pop	eax
	ret

endproc vsd_boot_complete


;==============================================================================
;
; vsd_config_dcb() -
;
;   This routine is called when the AER received an AEP_CONFIG_DCB request.
;   This request is sent by IOS during HSFLOP.PDR initialization time, the
;   AEP packet points to a DCB_Floppy data structure.  The 3mode driver needs
;   to set the DCB_3mode_Flags field at the end of the structure.  Currently
;   the following two flags are defined:
;
;   DCB_is_3mode_drive:  This bit should be set if the drive is a 3mode drive
;
;   DCB_set_spd_everytime:  This bit should be set if the drive spindle speed
;			    should be set on every access.
;
; Entry: ebx -> Async Event Packet (AEP)
;
; Exit:
;
; Destroys:     none
;
;==============================================================================

beginproc vsd_config_dcb

	;
	; Address the DCB
	;

	push	esi
	mov	esi, [ebx].AEP_d_c_dcb		; pick up address of the DCB

	;
	; Only handle physical DCB
	;

	test	[esi].DCB_device_flags, DCB_DEV_PHYSICAL
	jz	short dcb_config_ret

	;
	; Only handle floppy drive DCB
	;

	cmp	[esi].DCB_Device_Type,DCB_type_floppy
	jne	short dcb_config_ret

	call	Init3modeDrive			; in vsdoem.asm

dcb_config_ret:

	pop	esi
	ret

endproc vsd_config_dcb


;==============================================================================
;
; vsd_change_rpm() -
;
;   Call the OEM-supplied routine to actually change the spindle speed.
;
; Entry: ebx -> Async Event Packet
;
; Exit:
;
; Destroys:
;
;==============================================================================

beginproc vsd_change_rpm

	push	eax
;	Modify the AEP_rc. New AEP_rc has DCB field. 95-06-20(M9 Update)
	push	esi
	mov	esi, [ebx].AEP_rc_dcb

	;
	; Get speed parameter from AEP, in AEP_rc_speed if...
	;
	;   AEP_RC_360_RPM bit (0x0001) is set:    360 rpm (al = ffh)
	;   AEP_RC_300_RPM bit (0x0002) is set:    300 rpm (al = 00h)
	;

	mov	al,0ffh 			; assume 360 rpm
	test	word ptr [ebx].AEP_rc_speed,AEP_RC_300_RPM
	jz	@f
	mov	al,0				; set to 300 rpm

@@:
	call	SetSpindleSpeed 		; in vsdoem.asm

	;
	; Always return success, there is not much HSFLOP can do if this
	; call fails.
	;

	mov	[ebx].AEP_result, AEP_SUCCESS

	pop     esi			; 95-06-20(M9 Update)
	pop	eax
	ret

endproc vsd_change_rpm

;= Insert M9 Update =================================================================
;==============================================================================
;
; vsd_query_rpm() -
;
;   Call the OEM-supplied routine to actually get the now spindle speed.
;
; Entry: ebx -> Async Event Packet
;
; Exit: [AEP_ri_speed] <- now spindle speed
;
; Destroys:
;

;==============================================================================
beginproc	vsd_query_rpm
	push	eax
	push	ecx
	push	esi

	mov	esi, [ebx].AEP_ri_dcb

	mov	ax,0ffffh

	call	GetSpindleSpeed 		; in vsdoem.asm
	; Get speed parameter from AEP, in AEP_rc_speed if...
	;
	;   AEP_RC_360_RPM bit (0x0001) is set:    360 rpm (al = ffh)
	;   AEP_RC_300_RPM bit (0x0002) is set:    300 rpm (al = 00h)
	;
	cmp	ah,0			; If AH=0 supporting the
	jnz	vqr_NotSupport		; Get spindle

	mov	cx, AEP_RC_360_RPM		; assume 360 rpm
	cmp	al, 0ffh			; AH= 0 supportting the
	jz	@f				; 	get spindle
	mov	cx, AEP_RC_300_RPM		; set to 300 rpm
@@:
	mov	[ebx].AEP_ri_speed, cx		; Set return value

vqr_NotSupport:

	mov	[ebx].AEP_result, AEP_SUCCESS

	pop     esi
	pop	ecx
	pop	eax
	ret
endproc		vsd_query_rpm
	
;==============================================================================
; vsd_config_3mode() -
;
;   Call the OEM-supplied routine to actually get 3mode info.
;
; Entry: ebx -> Async Event Packet
;
; Exit: [AEP_3c_wait] <- Wait time when after change spindle.
;
; Destroys:

;==============================================================================
beginproc	vsd_config_3mode
	;
	; Address the DCB
	;

	push	esi
	mov	esi, [ebx].AEP_3c_dcb		; pick up address of the DCB

	;
	; Only handle floppy drive DCB
	;
	cmp	[esi].DCB_Device_Type,DCB_type_floppy
	jne	short config_3mode_ret

	call	Get3modeInfo			; in vsdoem.asm

	mov	[ebx].AEP_3c_wait, ax		; Set the wait time to AEP

config_3mode_ret:

	pop	esi
	ret
endproc		vsd_config_3mode
;= End of Insert M9 Update =================================================================

VXD_PAGEABLE_CODE_ENDS

end
