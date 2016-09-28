;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: CURSOR.ASM
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

        .xlist
DOS5=1
    include glint.inc
	include windefs.inc
    include ramdac.inc
    .list

RAMDAC_DELAY macro
    test    InFIFOSpace,0
    endm

;----------------------------------------------------------------------------
; E Q U A T E S 
;----------------------------------------------------------------------------
SOFTWARECURSOR  equ     0
HARDWARECURSOR  equ     1

;----------------------------------------------------------------------------
; E X T E R N S
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT segment
assumes cs,_TEXT
        .386

;
;--------------------------------------------------------------------------
; CheckCursor
;--------------------------------------------------------------------------
cProc   CheckCursor,<FAR,PUBLIC,PASCAL,NODATA>
cBegin <nogen>
.386
    mov     ax,_INIT
    mov     es, ax
	assumes	es,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    es,ax
	assumes ds,nothing
	assumes fs,nothing
	assumes gs,nothing
	push    es
	push    si
	les     si,GLInfo.lpDriverPDevice
	test    es:[si].deFlags,BUSY
	pop     si
	pop     es
	jne	    @f
	cmp	    wCursorType,SOFTWARECURSOR
	jne	    @f
    test    GLInfo.dwFlags, GMVF_GCOP
    jne     @f
	arg	    <word ptr GLInfo.lpDriverPDevice+2>
	arg	    <word ptr GLInfo.lpDriverPDevice>
	cCall	DIB_CheckCursorExt
@@:	ret
.286c
cEnd <nogen>

;
;
subttl		Inquire Entry Point (Handled by DIB Engine)
page +
cProc		Inquire,<PUBLIC,FAR>
;
	parmD	lpCursorInfo
;
cBegin		<nogen>
.386
	jmp	DIB_Inquire
.286c
cEnd		<nogen>
;
;
cProc		VideoPortWriteRegisterUlong,<PUBLIC,NEAR,C>
;
	parmD	Pointer
	parmD	Value
;
cBegin		<nogen>
.386
    push    bp
    mov     bp, sp
    lfs     bx, Pointer
    mov     eax, Value
    mov     fs:[bx], eax
    pop     bp
    ret
.286c
cEnd		<nogen>

cProc		VideoPortReadRegisterUlong,<PUBLIC,NEAR,C>
;
	parmD	Pointer
;
cBegin		<nogen>
.386
    push    bp
    mov     bp, sp
    lfs     bx, Pointer
    mov     eax, fs:[bx]
    pop     bp
    ret
.286c
cEnd		<nogen>
;
;

;extern void
;writeMask(BYTE *what, volatile ULONG *where, BYTE trail)
;{
;	int i, j;
;	ULONG ul = 0;
;
;	for (i = 0; i < 32; ++i) {
;		for (j = 0; j < 4; j++) {
;			*(char *)&ul = *what++;
;			*where = ul;
;		}
;		*(char *)&ul = trail;
;		for (j = 0; j < 4; j++)
;			*where = ul;
;	}
;}

cProc	WriteCursorMask,<PUBLIC,NEAR,C>
	parmD	what
	parmD	where
	parmD	trail
cBegin		<nogen>
.386
	push	bp
	mov		bp, sp
	push	ds
	push	di

	les		bx, what
	lds		di, where
	mov		cx, 32
@@:
	mov		ax, es:[bx]			;get the first two of the four bytes for this line
	mov		[di], al			;write 1st
	mov		[di], ah			;write 2nd
	mov		ax, es:[bx + 2]		;then get the other two
	mov		[di], al			;write 3rd
	mov		[di], ah			;write 4th
	mov		al, byte ptr trail
	add		bx, 4
	mov		[di], al				;then four trailing writes per line
	mov		[di], al
	mov		[di], al
	mov		[di], al
	dec		cx					;and do all 32 lines...
	jnz		@b
	
	pop		di
	pop		ds
	pop		bp
	ret
.286c
cEnd		<nogen>



_TEXT   ends
end
