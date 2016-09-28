;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: text.asm
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
	.286c
	.xlist
	DOS5 = 1			;so we don't get INC BP in <cBegin>
incDrawMode = 1
incFont = 1

include glint.inc
include	gdidefs.inc

	.list
;----------------------------------------------------------------------------
; D A T A
;----------------------------------------------------------------------------
_TEXT segment
	assumes cs,_TEXT
_TEXT ends

_INIT segment 
    globalW wRenderText, 2

ifdef GDI_BYPASS
; Set up the GDI interception table
GDI_ExtTextOut_Intercept	dw	OFFSET GDI_ExtTextOut
							dw	SEG GDI_ExtTextOut
							db	"ExtTextOut",0
GDI_ExtTextOut_InterceptCmd	dw	INTERCEPT_VALIDATE			; Validate
							dw	3							; From GDI Offset 3
							dw	OFFSET GDIETO_ValidateStart	; Start _TEXT Offset
							dw	OFFSET GDIETO_ValidateEnd	; End _TEXT Offset
							dw	INTERCEPT_MOVE				; Move
							dw	0							; From GDI Offset 0
							dw	OFFSET GDIETO_RelocateStart	; Start _TEXT Offset
							dw	OFFSET GDIETO_RelocateEnd	; End _TEXT Offset
							dw	INTERCEPT_RETURN			; Place the return jump
							dw	OFFSET GDIETO_ReturnJump	; at this offset in _TEXT
							dw  7							; to offset 7 in GDI
							dw	INTERCEPT_END				; Finished relocation
endif ; GDI_BYPASS


_INIT ends

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT segment
	assumes cs,_TEXT

.486
Strblt PROC FAR PASCAL PUBLIC
	pop	cx			;Save caller's return address
	pop	bx
	xor	ax,ax
	push	ax			;Push null for lp_dx
	push	ax
	push	ax			;Push null for lp_opaque_rect
	push	ax
	push	ax			;Push null for options
	push	bx			;Restore return address
	push	cx
    jmp     ExtTextOut16
Strblt ENDP

PLABEL  ExtTextOut
    jmp     _TEXT32:ExtTextOut16
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop


ifdef GDI_BYPASS
;-----------------------------------------------------------------------------------------
; GDI_ExtTextOut
;   This updates the DC extent regions.
;   This routine assumes rectangle is on the stack and ds is GDI's ds
;-----------------------------------------------------------------------------------------
GDI_ExtentChecking PROC NEAR PASCAL PUBLIC USES si di, pDC:WORD, pRect:WORD
	mov		si, pDC
	mov		bp, pRect
	mov		dx, [bp].left
	mov     di, [bp].top
	mov     bx, [bp].right
	mov     cx, [bp].bottom
	test    bx, bx						; I guess extents need to be unclipped!
	jne		@f
	mov		bx, 07fffh
@@:
	test    cx, cx						; I guess extents need to be unclipped!
	jne		@f
	mov		cx, 07fffh
@@:
	mov		al, [si].DC_ExtentFlags		; Find out which regions to update
	shr     al, 1
	and     al, 5
	or      [si].DC_ExtentFlags, al		; and mark them as updated.

	test	al, 4
	je		GDI_ExtentCheckingSecondRegion
	push	ax
	mov		ax, [si].DC_ExtentBottom1
	cmp		ax, dx
	jle		@f
	mov		[si].DC_ExtentBottom1, dx
@@:
	mov		ax, [si].DC_ExtentRight1
	cmp		ax, di
	jle		@f
	mov		[si].DC_ExtentRight1, di
@@:
	mov		ax, [si].DC_ExtentTop1
	cmp		ax, bx
	jge		@f
	mov		[si].DC_ExtentTop1, bx
@@:
	mov		ax, [si].DC_ExtentLeft1
	cmp		ax, cx
	jge		@f
	mov		[si].DC_ExtentLeft1, cx
@@:
	pop		ax
GDI_ExtentCheckingSecondRegion:
	test	al, 1
	je		GDI_ExtentCheckingDone
	mov		ax, [si].DC_ExtentBottom2
	cmp		ax, dx
	jle		@f
	mov		[si].DC_ExtentBottom2, dx
@@:
	mov		ax, [si].DC_ExtentRight2
	cmp		ax, di
	jle		@f
	mov		[si].DC_ExtentRight2, di
@@:
	mov		ax, [si].DC_ExtentTop2
	cmp		ax, bx
	jge		@f
	mov		[si].DC_ExtentTop2, bx
@@:
	mov		ax, [si].DC_ExtentLeft2
	cmp		ax, cx
	jge		@f
	mov		[si].DC_ExtentLeft2, cx
@@:
GDI_ExtentCheckingDone:
	ret
GDI_ExtentChecking ENDP


;-----------------------------------------------------------------------------------------
; GDI_ExtTextOut
;	This is a GDI bypass routine. We need to decide whether we want to process the call
; without involving GDI, or if we want to pass the thing to GDI. 
;-----------------------------------------------------------------------------------------
GDI_ExtTextOut PROC FAR PASCAL PUBLIC USES ds esi edi, hdc:WORD, x:WORD, y:WORD, fuOptions:WORD, lprc:DWORD, lpString:DWORD, cbCount:WORD, lpDx:DWORD
        LOCAL   ScreenRect:RECT, ClipRect:RECT, lpClipRect:DWORD, lpDevice:DWORD
	assumes	ds,nothing
	assumes	es,nothing
	assumes	fs,nothing
	assumes	gs,nothing

	cmp		cbCount, 0						; Have we got any characters to draw?
	jne		GDIETO_ReturnToGDI				; Let GDI handle it if not

	mov		ax, fuOptions
	test	ax, ETO_OPAQUE
	je		GDIETO_ReturnToGDI				; Let GDI handle if we are not opaquing

 	mov		ax, WORD PTR lprc+2
	test	ax, ax
	je		GDIETO_ReturnToGDI				; No clip, no Opaque

	mov	    ax,WORD PTR Text_DataSegment			
	push	ax					
	pop		gs					
	assumes ds,nothing
	assumes	es,nothing
	assumes	fs,nothing
	assumes gs, _INIT

	cmp	wCursorType,0   	                ;running with a software cursor?
    je      GDIETO_ReturnToGDI              ;GDI can deal with it if so.

    mov     ax, GDIDataSegment
    mov     bx, hdc
    push    ax
    pop     ds
    mov     si, ds:[bx]                     ; Get pointer to DC
    mov     ax, [si].DC_Type                ; Do some validation of the DC
    and     ax, 05fffh                      ; and off the 8000h and 2000h bits
    cmp     ax, 4f4dh                       ; compare it with DC type
    jne     GDIETO_ReturnToGDI
; We now assume the hdc was valid. Check to see if its a DIBeng PDEVICE
    mov     eax, [si].DC_PDevice 
    les     di, [si].DC_PDevice             ; Get PDevice
    mov     lpDevice, eax                   ; Save PDevice for future use
    mov     ax, es:[di].deType
    cmp     ax, TYPE_DIBENG                 ; Is it a DIBEngine PDevice?
    jne     GDIETO_ReturnToGDI              ; Give back to GDI if not

	test	es:[di].deFlags,VRAM  	        ;is this screen memory?
	jz		GDIETO_ReturnToGDI	            ;no, let the DIB Engine handle it.
	test	es:[di].deDriverReserved,ACCELERATED  	        ;is this accelerated?
	jz		GDIETO_ReturnToGDI	            ;no, let the DIB Engine handle it.
    test    es:[di].deFlags,BUSY
    jnz     GDIETO_ReturnToGDI              ;out if busy

    les     di, lprc                        ; Get pointer to rectangle

    mov     ax, [si].DC_Scr_X               ; Get window origin X
    mov     bx, [si].DC_Scr_Y               ; Get window origin Y
    mov     cx, es:[di].left                ; Get left of rectangle
    mov     dx, es:[di].right               ; Get right of rectangle
    cmp     cx, dx                          ; do we need to swap our X's?
    jle     @f
    xchg    cx, dx
@@:
    add     cx, ax                          ; convert rect to screen coords
    add     dx, ax                          ; convert rect to screen coords
    mov     ScreenRect.left, cx
    mov     ScreenRect.right, dx

    mov     cx, es:[di].top                 ; Get left of rectangle
    mov     dx, es:[di].bottom              ; Get right of rectangle
    cmp     cx, dx                          ; do we need to swap our Y's?
    jle     @f
    xchg    cx, dx
@@:
    add     cx, bx                          ; convert rect to screen coords
    add     dx, bx                          ; convert rect to screen coords
    mov     ScreenRect.top, cx
    mov     ScreenRect.bottom, dx

	test    [si].DC_ExtentFlags,0ah         ; Are we doing extent checking?
	je      @f
; This routine assumes rectangle is on the stack and ds is GDI's ds
    lea     cx, ScreenRect
	push    si								; DC
	push    cx								; offset of rect
    call    GDI_ExtentChecking
@@:

    mov     ecx, [si].DC_bkColor            ; Get background color

    movzx   edi, [si].DC_ClipRegion         ; Get region handle
    lea     edi, [edi + 010000h]            ; Get handle address
    mov     edi, [edi]                      ; get Region address
    cmp     [edi].RGN_Count, 0              ; have we got a cliprect?
    je      GDIETO_OpaqueNoWindowClip
    mov     ebx, [edi].RGN_StructSize       ; Get size of Clip structure
    cmp     ebx, 02ah                       ; is there just a single region?
    jg      GDIETO_OpaqueMultipleWindowClip

    mov     eax, DWORD PTR [edi].RGN_BoundLeft
    mov     DWORD PTR ClipRect, eax
    mov     eax, DWORD PTR [edi].RGN_BoundRight
    mov     DWORD PTR ClipRect+4, eax
    lea     bx, ScreenRect
    lea     ax, ClipRect
    push    lpDevice
    push    ss
    push    bx                              ; lpOpaqueRect from user
    push    ss
    push    ax                              ; Cliprect from window region
    push    ecx                             ; Color
    call    OpaqueRect32
GDIETO_Exit:
    ret

GDIETO_OpaqueMultipleWindowClip:
    ; This is where things get a little more complex!
    ; We need to call the Opaque routine for each rectangle we need to opaque.

    mov     edx, lpDevice

    push    bp

    lea     bp, ClipRect

    add     ebx, edi                        ; ebx = offset of end of clip region
    add     edi, 01eh                       ; edi points to num X coords
GDIETO_OpaqueNextY:
    mov     si, [edi]                       ; si is count of X coords
    mov     ax, [edi+2]                     ; Get cliprect top
    mov     [bp].top, ax                    ; update our local cliprect
    mov     ax, [edi+4]                     ; Get cliprect bottom
    mov     [bp].bottom, ax
    add     edi, 6                          ; edi = first X coord
    push    ebx                             ; save last addr
GDIETO_OpaqueNextX:
    mov     ax, [edi]                       ; Get left
    mov     [bp].left, ax                   ; Update local clip rect
    mov     ax, [edi+2]                     ; Get right
    mov     [bp].right, ax
    add     edi, 4                          ; point to next coord

; OpaqueRect corrupts pretty much all the registers! save those we need
    push    ds
    push    edi
    push    si
    push    ecx
    push    edx

    lea     ax, [bp+8]                      ; ScreenRect is just above Cliprect
    push    edx                             ; lpDevice
    push    ss
    push    ax                              ; lpOpaqueRect from user
    push    ss
    push    bp                              ; Cliprect from window region
    push    ecx                             ; Color
    call    OpaqueRect32                    ; draw the rectangle

    pop     edx
    pop     ecx
    pop     si
    pop     edi
    pop     ds
    sub     si, 2
    jg      GDIETO_OpaqueNextX

    add     edi, 2                          ; Skip over extra word
    pop     ebx
    cmp     edi, ebx
    jb      GDIETO_OpaqueNextY
    pop     bp
    jmp     GDIETO_Exit
    
GDIETO_OpaqueNoWindowClip:
; Presumabaly the window is not mapped if we havnt got any window clip information.
; therefore, we pass the call to GDI to deal with however it sees fit.
GDIETO_ReturnToGDI:
	pop		edi
	pop		esi
    pop     ds
    add     sp, 24               ; remove locals
	pop		bp
GDIETO_RelocateStart:
	mov		ax, 1234h
GDIETO_RelocateEnd:
GDIETO_ValidateStart:
	inc		bp
	push	bp
	mov		bp, sp
GDIETO_ValidateEnd:
GDIETO_ReturnJump:
	nop
	nop
	nop
	nop
	nop
GDI_ExtTextOut ENDP
endif ; GDI_BYPASS

_TEXT ends


end
