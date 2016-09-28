;===========================================================================
	page	,132
	title	oem - OEM routines to initialize and set spindle speed
	name	vsdoem.asm
;===========================================================================
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
; PURPOSE.
;
; Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.
;
;   Module:
;		This module contains the OEM routines to initialize a 3mode
;		floppy drive, and to set the floppy drive spindle speed.
;
;   Version:  0.001
;
;   Date:     June 18, 1995
;
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

.386p

;============================================================================
;				I N C L U D E S
;============================================================================

.xlist
	include vmm.inc
	include	blockdev.inc
	include ilb.inc 	;IOS linkage block
	include aep.inc 	;Async event packet
	include ddb.inc 	;Driver information block
	include dcb.inc 	;DCB
	include isp.inc 	;IOS Service Block
	include debug.inc	;general debug macros
	include iodebug.inc
	include vsdinfo.inc	;general vsd information
.list


;============================================================================
;
; SYNC_DRIVES
;
; If the OEM machine includes two 3 mode drives, and both drives cannot
; maintain separate spindle speed settings, then the SYNC_DRIVES flag should
; be set.
;
;============================================================================

ifdef	CPQ_1
	SYNC_DRIVES equ     1
endif

ifdef	CPQ_2
	SYNC_DRIVES equ     1
endif

ifdef	CPQ_4
	SYNC_DRIVES equ     1
endif

;==============================================================================
;			   L O C K E D	 C O D E
;==============================================================================

VxD_LOCKED_CODE_SEG

	extrn	NEC_out:near		    ; in vsdutil.asm
	bMikado	dw	?
VXD_LOCKED_CODE_ENDS

;==============================================================================
;		 	 P A G E A B L E   C O D E
;==============================================================================

VXD_PAGEABLE_CODE_SEG

;==============================================================================
;
; Init3modeDrive() -
;
;   This routine is called when the AER received an AEP_CONFIG_DCB request.
;   This request is sent by IOS during HSFLOP.PDR initialization time, the
;   AEP packet points to a DCB_Floppy data structure.  The 3mode drivers needs
;   to set the DCB_3mode_Flags field at the end of the structure.  Currently
;   the following two flags are defined:
;
;   DCB_IS_3MODE_DRIVE:  This bit should be set if the drive is a 3mode drive
;
;   DCB_SET_RPM_ON_ACCESS:  This bit should be set if the drive spindle speed
;			    should be set on every access.
;
; Entry: ebx -> Async Event Packet (AEP)
;	 esi -> Floppy DCB
;
; Exit:
;
; Destroys:     none
;
;==============================================================================

beginproc Init3modeDrive

	;
	; Get Machine ID
	;

ifdef DEBUG1
int 1
int 3
endif

	mov	ax, 0E800h
	push	dword ptr 15h
	VMMcall	Exec_Vxd_Int
	jc	lblINo15
	mov	ax, bx
        jmp	lblIdone
    lblINo15:
        mov	dx, 0C7Ch
        in	al, dx
        xor	ah, ah
    lblIdone:
	.IF	ax == 0114h
	mov	bMikado, 1h
	.ELSE
	mov	bMikado, 0h
	.ENDIF

	;
	; Initialize 3mode flags in DCB.  DCB_IS_3MODE_DRIVE bit is zeroed out.
	; Note that this is not strictly necessary because IOS zeros out the
	; DCB when it creates it.
	;

	xor	ax,ax
	mov	[esi].DCB_3mode_Flags,ax

	;
	; At this point the driver should identify if the floppy is
	; a 3mode drive or not.  For this model, we will simply check
	; if it's a 1.44MB floppy by calling BIOS function Read Drive
	; Parameters.
	;

	push	dx
	mov	dl,byte ptr [esi].DCB_unit_number
	mov	ah,8h
	push	dword ptr 13h
	VMMcall	Exec_VxD_Int
	pop	dx
	jc	short i3d_ret			; Call failed
	and	bl,0fh
	cmp	bl,04h				; Is drive 1.44Mbyte?
	jnz	short i3d_ret			; no, skip it

	;
	; Set the DCB_IS_3MODE_DRIVE bit
	;

	or	[esi].DCB_3mode_Flags, DCB_IS_3MODE_DRIVE

	;
	; Set the other bit
	;

ifdef	SYNC_DRIVES
        test    [esi].DCB_3mode_Flags, DCB_IS_3MODE_DRIVE       ; flag set?
        jz      @f
	or	[esi].DCB_3mode_Flags, DCB_SET_RPM_ON_ACCESS
@@:
endif

i3d_ret:

	ret

endproc Init3modeDrive


;==============================================================================
;
; SetSpindleSpeed() -
;
;   The actual routine to change the spindle speed between 300 and 360 rpm.
;
; Entry: ebx -> Async Event Packet
;	  al == ffh: request 360 rpm for 1.2 and 1.232MB
;	     == 00h: request 300 rpm for 2.88,1.44MB and 720KB
;	 esi -> Floppy DCB ; 95-06-20(M9 update)
; 	        You can get the drive number from DCB_unit_number.
;
; Exit:
;
; Destroys: all but ebp
;
;==============================================================================

beginproc SetSpindleSpeed

	push	eax
	push	ebx
	push	ecx
	push	edx
	push	edi
	push	esi
	pushfd

;ifdef DEBUG1
;int 1
;int 3
;endif

	sti


ifdef	DEBUG
	push	ax
	cmp	al,0
	jz	@f
	mov	al,6h
@@:
;	VerPrintf       1, <"3 MODE : Change spindle request for 3%lx0 rpm\n">,<al>
	pop	ax
endif
	;push	ax
	;mov	al,  0c0h				; 500kbs
	;mov	dx, PORT_Input_And_XferRate	; 3f7

	;out	dx, al				; set controller xfr rate

	;pop	ax

ifdef	CPQ_1

	;-------------------------------------------------------------------
	; CPQ_1:  DeskPro series, Prolinea
	;
	;	  National FDC (8477, 87311, 8478, 87322,...)
	;-------------------------------------------------------------------
	;
	; Write mode command to FDC
	;
	;    byte 0: 01h
	;    byte 1: 02h
	;    byte 2: 00h
	;    byte 3: Spindle byte
	;	     00h: if 360rpm
	;	     C0h: if 300rpm
	;    byte 4: 00h
	;
	;-------------------------------------------------------------------

	cli

	and	al,1100000b	; get spindle info
	xor	al,1100000b	; if 360rpm, then al=00h
				; if 300rpm, then al=C0h
	mov	ah,al
	mov	al,0
	shl	eax,16
	mov	ax,201h		; set mode command
	mov	bl,0		; set fifth byte
	mov	cx,5
	call	NEC_out
ifdef	DEBUG
	mov	dl,1		;error code
endif
	jc	ChangeSpindle_ret

endif	; CPQ_1

ifdef	CPQ_2

	;-------------------------------------------------------------------
	; CPQ_2:  Prolinea MT series
	;
	;	  SMC's FDC (FDC37C661, FDC37C662,...)
	;-------------------------------------------------------------------
	;
	; 1) Move configuration mode to FDC twice write 55h to port 3f0h
	; 2) Set spindle rotation index 03h: bit3 and bit4
	; 3) Exit configuration mode
	;
	;-------------------------------------------------------------------

;ifdef DEBUG1
;int 1
;int 3
;endif

	cli
	;jmp	ChangeSpindle_ret

	mov	ah,al
	and	ah,10000b	; mask bit4
	xor	ah,10000b	; if 360rpm, AH=00h
				; if 300rpm, AH=10h
	mov	al, 55h
	mov	dx, PORT_StatusRegA ; 3f0
	out	dx, al		; Enter configuration mode
	out	dx, al

	mov	al, 03h		; set config register
	out	dx, al

	inc	dx
	in	al, dx		; read 3f1h
	and	al, 11100111b	; clear bit4 and 3
	or	al, ah
	out	dx, al		; write 3f1h

	mov	al, 0aah
	dec	dx
	out	dx, al		; exit configuration mode

endif	; CPQ_2

ifdef	CPQ_3

	;-------------------------------------------------------------------
	; CPQ_3:  Contura 400
	;
	;	  VLSI's FDC (VL82C144-02, ...)
	;-------------------------------------------------------------------
	;
	; Write mode command to FDC
	;
	;    byte 0: 01h
	;    byte 1: 02h
	;    byte 2: 00h
	;    byte 3: Spindle byte
	;	     00h: if 360rpm
	;	     C0h: if 300rpm
	;    byte 4: 00h
	;
	;-------------------------------------------------------------------

	cli
	mov	ah,al
	and	ah,01000000b
	xor	ah,01000000b	; if 360rpm, AH=00h
				; if 300rpm, AH=40h
	mov	dx,0ech		; set index register
	mov	al,49h		; config reg
	out	dx,al

	inc	dx
	in	al, dx		; read FCR
	and	al, not 40h	; clear bit6
	or	al, ah
	out	dx, al		; set spindle mode

	mov	al,0ffh		; mask data
	dec	dx
	out	dx,al		; mask
	sti

endif	; CPQ_3

ifdef	CPQ_4

	;-------------------------------------------------------------------
	; CPQ_4:  Prolinea III
	;
	;	  SMC's FDC (FDC37C665)
	;-------------------------------------------------------------------
	;
	; 1) Move configuration mode to FDC twice write 55h to port 3f0h
	; 2) Set spindle rotation index 05h: bit3 and bit4
	; 3) Exit configguration mode
	;
	;-------------------------------------------------------------------


	cli
	mov	cx, bMikado
	cmp	cx, 1h		; MIKADO == TRUE
	jz	MIKADO

	;
	; For Old ProLinea
	;
	and	al,1100000b	; get spindle info
	xor	al,1100000b	; if 360rpm, then al=00h
				; if 300rpm, then al=C0h
	mov	ah,al
	mov	al,0
	shl	eax,16
	mov	ax,201h		; set mode command
	mov	bl,0		; set fifth byte
	mov	cx,5
	call	NEC_out
ifdef	DEBUG
	mov	dl,1		;error code
endif
	jc	ChangeSpindle_ret


	;
	; For MIKADO
	;
    MIKADO:
	mov	ah,al
	and	ah,11000b	; if 360rpm, AH=18h
				; if 300rpm, AH=00h
	mov	al, 55h
	mov	dx, PORT_StatusRegA ; 3f0
	out	dx, al		; Enter configuration mode
	out	dx, al

	mov	al, 05h		; set config register
	out	dx, al

	inc	dx
	in	al, dx		; read 3f1h
	and	al, 11100111b	; clear bit4 and 3
	or	al, ah
	out	dx, al		; write 3f1h

	mov	al, 0aah
	dec	dx
	out	dx, al		; exit configuration mode

endif	; CPQ_4


ifdef	CPQ_5

	;-------------------------------------------------------------------
	; CPQ_5:  Contura AERO series
	;
	;	  National FDC (8477, 87311, 8478, 87322,...)
	;-------------------------------------------------------------------
	;
	; Write mode command to FDC
	;
	;    byte 0: 01h
	;    byte 1: 00h
	;    byte 2: 00h
	;    byte 3: Spindle byte
	;	     00h: if 360rpm
	;	     C0h: if 300rpm
	;    byte 4: 00h
	;
	;-------------------------------------------------------------------

;ifdef DEBUG1
;int 1
;endif

	cli

	and	al,1100000b	; get spindle info
	xor	al,1100000b	; if 360rpm, then al=00h
				; if 300rpm, then al=C0h
	mov	ah,al
	mov	al,0
	shl	eax,16
	mov	ax,0001h		; set mode command
	mov	bl,0		; set fifth byte
	mov	cx,5
	call	NEC_out
ifdef	DEBUG
	mov	dl,1		;error code
endif
	jc	ChangeSpindle_ret

endif	; CPQ_5


;; Delete 95-07-11(M9 update)
;;	;
;;	; We now need to set up a time-out call to turn the motor off
;;	;
;;
;;ChangeSpindle_Cont:
;;
;;	mov     eax, 1 * 500                    ; .5 second
;;	mov     edx, edi                        ; (ref. data ret to proc)
;;      mov     esi, offset32 SpindleOut
;;	VMMcall Set_Global_Time_Out
;;
ifdef	DEBUG
	VerPrintf	1, <"3 MODE : Change spindle success\n">
	jmp	@f
ChangeSpindle_ret:
	VerPrintf       1, <"3 MODE : Error code = %lx\n">,<dl>
@@:
else
ChangeSpindle_ret:
endif

	popfd
	pop     esi                             ; restore reg
	pop     edi

	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
SpindleOut:
	ret

endproc SetSpindleSpeed

;******** Insert 95-07-11(M9 update)
;==============================================================================
;
; Get3modeInfo() -
;
;   The actual routine to get the 3mode info about waiting time when after
;   Spindle change..
;
; Entry: ebx -> Async Event Packet
;	 esi -> Floppy DCB 
;
; Exit:  eax -> waiting time (msec). 500msec is recommand.
;
; Destroys: all but ebp
;
;==============================================================================
beginproc Get3modeInfo


ifdef CPQ_4

	mov	eax, 1 * 500 	; 0.5 sec 

endif
	ret
endproc	Get3modeInfo

;==============================================================================
;
; GetSpindleSpeed() -
;
;   The actual routine to Get the spindle speed between 300 and 360 rpm.
;
; Entry: ebx -> Async Event Packet
;	 esi -> Floppy DCB 
;
; Exit:  AH  == supporting get spindle, other value is not supporting the it.
; 	 al  -> Spindle speed
;	     == ffh: Now FDC status is  360 rpm for 1.2 and 1.232MB
;	     == 00h: Now FDC status is  300 rpm for 2.88,1.44MB and 720KB
;
; Destroys: all but ebp
;
;==============================================================================
beginproc GetSpindleSpeed
	push	ebx
	push	ecx
	push	edx
	push	edi
	push	esi
	pushfd

ifdef CPQ_4
	;-------------------------------------------------------------------
	; CPQ_4:  Prolinea III
	;
	;	  SMC's FDC (FDC37C665)
	;-------------------------------------------------------------------
	;
	; 1) Move configuration mode to FDC twice write 55h to port 3f0h
	; 2) Get spindle rotation index 05h: bit3 and bit4
	; 3) Exit configguration mode
	;
	;-------------------------------------------------------------------

	cli
	mov	cx, bMikado
	cmp	cx, 1h		; MIKADO == TRUE
	jz	get_sp_MIKADO
	jmp	GetSS_exit	

	;
	; For MIKADO
	;
    get_sp_MIKADO:
	mov	al, 55h
	mov	dx, PORT_StatusRegA ; 3f0
	out	dx, al		; Enter configuration mode
	out	dx, al

	jmp	@f
@@:
	mov	al, 05h		; set config register
	out	dx, al
	jmp	@f
@@:

	inc	dx
	in	al, dx		; read 3f1h

	and	al, 00011000b	; clear bit4 and 3
	mov	ah, al		; if 360rpm, AH=18h
				; if 300rpm, AH=00h
	mov	al, 0aah
	dec	dx
	out	dx, al		; exit configuration mode
	sti

	mov	cx, 0		; Assume 300rpm
	or	ah,ah
	jz	@f
	mov	cx, 00ffh	; set 360rpm
@@:
				; AH = 0 is supporting get spindle speed.
				; AL is spindle speed value.
	mov	ax, cx		; Set return Value to AX
endif

GetSS_exit:
	popfd
	pop     esi                             ; restore reg
	pop     edi

	pop	edx
	pop	ecx
	pop	ebx

	ret
endproc GetSpindleSpeed
;******** End of insert 95-07-11(M9 Update)

VXD_PAGEABLE_CODE_ENDS

end
