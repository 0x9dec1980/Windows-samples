;===========================================================================
	page	,132
	title	util - Utility and helper routines for port access
	name	vsdutil.asm
;===========================================================================
;
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
; PURPOSE.
;
; Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.
;
;   Module:
;		This module contains utility and helper routines to
;		access port.
;
;   Version:  0.001
;
;   Date:     March 2, 1995
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
	include debug.inc	;general debug macros
	include iodebug.inc	;general debug macros
	include vsdinfo.inc	;general vsd information
.list

;==============================================================================
;		 	 P A G E A B L E   C O D E
;==============================================================================


;==============================================================================
;			  L O C K E D	 C O D E
;==============================================================================

VXD_LOCKED_CODE_SEG

;==============================================================================
;
; NEC_in
;
; ENTRY: cl = num bytes to read
; EXIT:  eax = lo return
;	 ebx = high return
;	 NC = no error
;	 CY = error
; USES:  ecx
;
;==============================================================================

beginproc NEC_in

ifdef DEBUG
	pushfd
	or      cl, cl
	jnz     short ni05
	VerPrintf	1, <"    ERROR: NEC_in - 0 bytes\n">
ni05:   popfd
	mov     eax, -1
	mov     ebx, eax
endif
	sti
	and     ecx, 0FFh

ni10:	mov	dx, PORT_StatusReg   ; 3f4
	push    ecx
	mov     ecx, 20000h
	push    eax

ni20:   in      al, dx
	and     al, 11000000b
	cmp     al, 11000000b
	jz      short ni30
	loop    ni20
	pop     eax

	pop     ecx
	VerPrintf	1, <"    ERROR: NEC_in - couldn't read all bytes: %ld\n">, <cl>
	stc
	ret

	; read it
ni30:   pop     eax
	shld    ebx, eax, 8
	shl     eax, 8
	inc     dx
	in      al, dx
	pop     ecx
	loop    ni10

	clc
ni40:   ret

endproc NEC_in


;==============================================================================
;
; NEC_out
;
;
; ENTRY: eax = low byte(s) to out
;	 ebx = high byte(s) to out
;	 cl = number of bytes to out
; EXIT:  NC = no error
;	 CY = error
; USES:  ecx
;
;==============================================================================

beginproc NEC_out

ifdef DEBUG
	pushfd
	or      cl, cl
	jnz     short no05
	VerPrintf	1, <"    ERROR: NEC_out - 0 bytes\n">
	TRAP
no05:   popfd
endif

	; clear the vm interrupts flag
	sti
	and     ecx, 0FFh

no10:	mov	dx, PORT_StatusReg   ; 3f4
	push    ecx
	mov     ecx, 20000h
	push    eax

no20:   in      al, dx
	and     al, 11000000b
	cmp     al, 10000000b
	jz      short no30
	; BUGBUG - give up time slices if n tries in loop?
	loop    no20
	pop     eax

	pop     ecx
	VerPrintf	1, <"    ERROR: NEC_out - couldn't write all bytes: %ld\n">, <cl>
	; shk TRAP
	stc
	jmp     short no40

no30:   pop     eax
	inc     dx
	out     dx, al
	shrd    eax, ebx, 8
	shr     ebx, 8

	pop     ecx
	loop    no10

	clc
no40:   ret

endproc NEC_out


;==============================================================================
;
; NEC_delay() - Small delay for successive outs
;
; Entry:
;
; Exit:
;
; Destroys:     none
;
;==============================================================================

beginproc NEC_delay

	push    cs
	push    offset32 ndret
	retf
ndret:
	ret

endproc NEC_delay

VXD_LOCKED_CODE_ENDS

end
