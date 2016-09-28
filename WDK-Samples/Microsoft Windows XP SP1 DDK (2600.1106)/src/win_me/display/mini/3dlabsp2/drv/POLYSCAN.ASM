;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: polyscan.asm
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
    .xlist
    DOS5 = 1			;so we don't get INC BP in <cBegin>
    include glint.inc
    incLogical = 1
    incDrawMode = 1
    incPolyScanline = 1
    include gdidefs.inc
    .list
;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------
ROP2_COPYPEN    equ     0ch

_INIT   segment
PSL_clipped     dw      0
_INIT   ends

;----------------------------------------------------------------------------
;			       GLINT
;----------------------------------------------------------------------------
_TEXT segment
assumes cs,_TEXT
.486

PLABEL PS_CallDIBEngine
        pop     edi
        pop     esi
        pop     ds
        lea     sp,[bp]
        pop     bp
	jmp	DIB_Output


GlintPolyScanLine PROC FAR PASCAL PUBLIC USES ds, lpDestDev:DWORD, wStyle:WORD, wCount:WORD,         
    lpPoints:DWORD, lpPPen:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD, lpClipRect:DWORD     
    LOCAL pen_color:DWORD,dwRenderIt:DWORD,
        wLeft:WORD,wTop:WORD, wRight:WORD,wBottom:WORD
assume  ds:Nothing
assume  es:Nothing

	push	esi			;save 32 bit versions of these
	push	edi			;
    lfs     bx,lpDestDev            ;fs:bx-->dest pdevice

	cmp	    fs:[bx].deType,TYPE_DIBENG             ;is this a DIBENGINE surface?
    jne     PS_CallDIBEngine

    mov     ax, fs:[bx].deFlags

    test    ax,VRAM
    jz      PS_CallDIBEngine
    test    ax,BUSY
    jnz     PS_CallDIBEngine
    test    fs:[bx].deDriverReserved,ACCELERATED    ;is destination accelerated?
    je      PS_CallDIBEngine        ;no.
    mov     ax,_INIT
    mov     gs, ax
	assumes	gs,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    gs,ax
    assumes ds,nothing
    assumes es,nothing
    assumes fs,nothing
    assumes gs,_INIT

	mov     PSL_clipped,0

	;
	;       Process lpPPen.
	;
	cmp     lpPPen,0
	je      DonePen
	lds     si,lpPPen
	mov     ax,ds:[si].dpPenStyle
	cmp     ax,LS_NOLINE
	jne     @f
	mov     lpPPen,0
	jmp     DonePen
@@:     cmp     ax,LS_SOLID
	jne     PS_CallDIBEngine
	mov     eax,ds:[si].dpPenColor
	mov     pen_color,eax
DonePen:

	;
	;       Process lpPBrush.
	;
	cmp     lpPBrush,0
	je      DoneBrush
	lds     si,lpPBrush
	assume  ds:Nothing
	mov     ax,ds:[si].dp8BrushStyle
	cmp     ax,BS_HOLLOW
	jne     @f
	mov     lpPBrush,0
	jmp     DoneBrush
@@: cmp     ax,BS_SOLID
	jne     PS_CallDIBEngine
    mov     al, ds:[si].dp8BrushFlags          ; Check for Dithered spans. We dont
    test    al, COLORSOLID                      ; yet support this operation
    je      PS_CallDIBEngine
	mov     eax,dword ptr ds:[si].dp8BrushBits
	mov     pen_color,eax
DoneBrush:

	;
	;       Process lpDrawMode.
	;
	lds     si,lpDrawMode
	assume  ds:Nothing
	mov     ax,ds:[si].DRAWMODE.Rop2
	dec     ax                      ; Convert to index
	and     ax,0000fh               ; Ensure 0 <= ax < 16
	cmp     ax,ROP2_COPYPEN
	je      PS_RopIsOK
        xor     ebx,ebx
        cmp     ax,0
        je      PS_RopOverride
        not     ebx
        cmp     ax,0Fh
	jne     PS_CallDIBEngine
PS_RopOverride:
        mov     pen_color,ebx
PS_RopIsOK:
        ;
        ;       Find visible rectangle if not clipped
        ;
        cmp     wCursorType,0           ;soft cursor?
        jne     PY_NoVis                ;skip if not
        cmp     lpClipRect,0            ;clipped?
        jne     PY_NoVis                ;j if so
        lds     si,lpPoints             ;pts to SCAN structures
        xor     ax,ax
        mov     wRight,ax               ;set large region
        mov     wBottom,ax
        dec     ax
        mov     wLeft,ax
        mov     wTop,ax
        mov     cx,wCount               ;all points
PY_FindMinMax:
        push    cx                      ;save count
        mov     ax,[si].SCAN.scnPntTop   ;check y
        cmp     ax,wBottom
        jbe     @f                      ;j if < bottom
        mov     wBottom,ax              ;set new bottom
@@:     cmp     ax,wTop
        jae     @f                      ;j if >= Top
        mov     wTop,ax
@@:     mov     ax,[si].SCAN.scnPntBottom    ;check y
        cmp     ax,wBottom
        jbe     @f                      ;j if < bottom
        mov     wBottom,ax              ;set new bottom
@@:     cmp     ax,wTop
        jae     @f                      ;j if >= Top
        mov     wTop,ax
@@:
        mov     cx,[si].scnPntCnt
        shr     cx,1                    ;count of pairs
        lea     si,[si].SCAN.scnPntX    ;pt to x pairs
NextPointMM:
        mov     ax,[si]
        cmp     ax,wRight
        jbe     @f                      ;j if < right
        mov     wRight,ax               ;set new right
@@:     cmp     ax,wLeft
        jae     @f                      ;j if >= left
        mov     wLeft,ax
@@:     add     si,2
        mov     ax,[si]
        cmp     ax,wRight
        jbe     @f                      ;j if < right
        mov     wRight,ax               ;set new right
@@:     cmp     ax,wLeft
        jae     @f                      ;j if >= left
        mov     wLeft,ax
@@:     add     si,2
        loop    NextPointMM
        add     si,2
        pop     cx
        loop    PY_FindMinMax
PY_NoVis:
	cmp	wCursorType,0   	;running with a software cursor?
	jne	PY_UsingHWCursor	;nope, no need to exclude hdw cursor
    lds     si, lpDestDev
    mov     ax, ds:[si].deFlags
    test    ax, OFFSCREEN
    jne     PY_UsingHWCursor
    test    ax, VRAM
    je      PY_UsingHWCursor
    
	push	gs			;save GS --> Data
	push    lpDestDev               ;PDevice
    cmp     lpClipRect,0            ;clipped?
    je      PY_UseMinMax            ;j if not
    lds     si,lpClipRect           ;pt to rect
	push    [si].left               ;Left
	push    [si].top                ;Top
	push    [si].right              ;Right
	push    [si].bottom             ;Bottom
    jmp     @f
PY_UseMinMax:
    push    wLeft
    push    wTop
    push    wRight
    push    wBottom
@@:	push    CURSOREXCLUDE           ;Flags
	call	BeginAccess 	;returns with flags in ax.
	pop	gs			;restore GS --> Data
	mov	DIBAccessCallFlag,'S'	;set the DIBAccessCallFlag
PY_UsingHWCursor:
	lfs	bx,GLInfo.lpDriverPDevice
;
;We need to set the BUSY bit in the destination PDevice structure so that
;the DIB engine doesn't try to asynchronously draw the cursor while we're
;using the hardware to BLT.
;
	or	fs:[bx].deFlags,BUSY
    mov     ax,GLInfo.wRegSel
    mov     fs,ax
    assumes fs,Glint

    DISPLAY_CONTEXT
    ;DISABLE_DISCONNECT fs

    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

    mov     eax, lpDestDev
    cmp     eax, GLInfo.dwCurrentOffscreen
    je      @f
    les     bx, lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, es:[bx].OSB_PixelOffset
	WaitFifo 1
    mov     FBWindowBase, edx
@@:

	;
	;       Process lpClipRect.
	;
	cmp     lpClipRect,0
	je      DoneClipRect
        WaitFifo 3
        lds     si,lpClipRect
	mov     ax,[si].top
	shl     eax,16
	mov     ax,[si].left
        mov     ScissorMinXY,eax
	mov     bx,[si].bottom
	shl     ebx,16
	mov     bx,[si].right
        mov     ScissorMaxXY,ebx
        mov     ScissorMode,3
	mov     PSL_clipped,1
DoneClipRect:

	;
	;       Draw the scan lines.
	;

        ; initial setup for Glint (non-variables)
        WaitFifo 5
        xor     eax,eax
	mov     eax,pen_color
	mov     FBWriteData,eax        ; set fixed color
        mov     bx,wBpp                 ;get depth
        cmp     bl,32
        je      @f                      ;skip duplicate if 32 bits
        movzx   eax,ax
        mov     cx,ax                   ;double up 16 bits
        shl     ecx,16
        or      eax,ecx
        cmp     bl,8
        jne     @f
        and     eax,00FF00FFh
        mov     ecx,eax
        shl     ecx,8
        or      eax,ecx
@@:     mov     FBBlockColor,eax        ;just in case
        xor     eax,eax
        test    PSL_clipped,-1
        jnz     @f
        mov     eax,dwRenderCommand
@@:     or      al,40h
	mov     dwRenderIt,eax

        lds     si,lpPoints             ;pts to SCAN structures
        mov     cx,wCount               ;count of them

PLABEL PS_NewScan
        push    cx                      ;save count
        WaitFifo 2
        mov     ax,[si].SCAN.scnPntTop  ;start Y
        shl     eax,16
        mov     StartY,eax
        mov     ax,[si].SCAN.scnPntBottom ;find height
        sub     ax,[si].SCAN.scnPntTop
        movzx   eax,ax
        mov     Count,eax               ;count of scans
        mov     cx,[si].scnPntCnt
        shr     cx,1                    ;count of pairs
        lea     si,[si].SCAN.scnPntX    ;pt to x pairs
PLABEL PS_NextPair
        WaitFifo 3
        mov     ax,[si]
        inc     si
        inc     si
        shl     eax,16
        mov     StartXDom,eax           ;start X
        mov     ax,[si]
        inc     si
        inc     si
        shl     eax,16
        mov     StartXSub,eax           ;start X
        mov     eax,dwRenderIt
        mov     Render,eax              ;start the render
        dec     cx
        jg      PS_NextPair
        inc     si                      ;step past count
        inc     si
        pop     cx
        dec     cx
        jg      PS_NewScan

        WaitFifo 1
        mov     ScissorMode,2           ;screen scissor only

;
;We need to unset the BUSY bit in the destination PDevice structure which
;we previously set in order to prevent the DIB engine from asynchonously
;drawing the cursor
;
	lfs	bx,GLInfo.lpDriverPDevice		;
	and	fs:[bx].deFlags,NOT BUSY;
;
;When we get to this point, we've finished doing a BitBLT which used hardware
;acceleration.	If we previously called the DIB engine to exclude a software
;cursor owned by it, we must call the DIB engine to unexclude its software
;cursor.
;
	cmp	DIBAccessCallFlag,0	;did we call DIB eng to exclude cursor?
	je	ok       		;nope, skip the following!
	push	lpDestDev		;
	push	CURSOREXCLUDE		;
	call	DIB_EndAccess		;Let DIB Engine unexclude cursor.
	mov	DIBAccessCallFlag,0	;clear the flag

ok:     mov     ax,1
        pop     edi
        pop     esi
    ret
GlintPolyScanLine   ENDP

_TEXT   ends
end
