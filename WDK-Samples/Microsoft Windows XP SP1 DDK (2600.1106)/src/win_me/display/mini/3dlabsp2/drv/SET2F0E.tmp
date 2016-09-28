;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: polyline.asm
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

ROP_REQ_SRC  =     0200h                   ; source read required
ROP_REQ_DST  =     0400h                   ; dest read required

_INIT   segment
PL_clipped     dw      0
_INIT   ends

;----------------------------------------------------------------------------
;			       GLINT
;----------------------------------------------------------------------------
_TEXT   segment
assumes cs,_TEXT

PLABEL PL_CallDIBEngine
        pop     edi
        pop     esi
        pop     ds
        lea     sp,[bp]
        pop     bp
	jmp	DIB_Output


GlintPolyLine PROC FAR PASCAL PUBLIC USES ds, lpDestDev:DWORD, wStyle:WORD, wCount:WORD,         
    lpPoints:DWORD, lpPPen:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD, lpClipRect:DWORD     
    LOCAL pen_color:DWORD,dwRenderIt:DWORD, Rop:DWORD,
        wLeft:WORD,wTop:WORD, wRight:WORD,wBottom:WORD
assume  ds:Nothing
assume  es:Nothing

	push	esi			;save 32 bit versions of these
	push	edi			;
    lfs     bx,lpDestDev            ;fs:bx-->dest pdevice

	cmp	    fs:[bx].deType,TYPE_DIBENG             ;is this a DIBENGINE surface?
    jne     PL_CallDIBEngine

    mov     ax, fs:[bx].deFlags

    test    ax,VRAM
    jz      PL_CallDIBEngine
    test    ax,BUSY
    jnz     PL_CallDIBEngine
    test    fs:[bx].deDriverReserved,ACCELERATED    ;is destination accelerated?
    je      PL_CallDIBEngine        ;no.
    mov     ax,_INIT
    mov     gs, ax
	assumes	gs,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    gs,ax
    assumes ds,nothing
    assumes es,nothing
    assumes fs,nothing

    xor     eax, eax
    mov     Rop, eax
	mov     PL_clipped,ax

	;
	;       Process lpPPen.
	;
	cmp     lpPPen,0
	je      DonePen
	lds     si,lpPPen
	mov     ax,ds:[si].dpPenStyle
	cmp     ax,LS_NOLINE
	jne     @f
	jmp     LineDone
@@: 
    cmp     ax,LS_SOLID
	jne     PL_CallDIBEngine
	mov     eax,ds:[si].dpPenColor
	mov     pen_color,eax
DonePen:

	;
	;       Process lpDrawMode.
	;
	lds     si,lpDrawMode
	assume  ds:Nothing
    xor     eax, eax
	mov     ax,ds:[si].DRAWMODE.Rop2
	dec     ax                      ; Convert to index
	and     ax,0000fh               ; Ensure 0 <= ax < 16
	cmp     ax,ROP2_COPYPEN
	je      PL_RopIsOK
    xor     ebx,ebx
    cmp     ax,0
    je      PL_RopOverride
    not     ebx
    cmp     ax,0Fh
	je     PL_RopOverride

PL_UseROP:
    mov     ebx, [rop2Table + 4 + eax*4]    ; Get FBReadMode and LogicalopMode
    mov     Rop, ebx                        ; Indicate that a ROP is taking place.

    mov     ax,GLInfo.wRegSel
    mov     fs,ax
    assumes fs,Glint

    DISPLAY_CONTEXT

    mov     edx, dwFBReadMode
    xor     ecx, ecx
    mov     cl, bl                          ; ecx now contains Logicalop
    and     ebx, ROP_REQ_DST                ; ebx now contains FBReadMode modifier
    or      ebx, edx                        ; ebx is now FBReadMode
    mov     edx, pen_color
    mov     LogicalOpMode, ecx
    mov     ColorDDAMode, 1
    mov     FBReadMode, ebx
    mov     ConstantColor, edx
    jmp     PL_RopIsOK
    assumes fs,nothing


PL_RopOverride:
        mov     pen_color,ebx
PL_RopIsOK:
        ;
        ;       Find visible rectangle if not clipped
        ;
        cmp     wCursorType,0           ;soft cursor?
        jne     PY_NoVis                ;skip if not
        cmp     lpClipRect,0            ;clipped?
        jne     PY_NoVis                ;j if so
        lds     si,lpPoints
        xor     ax,ax
        mov     wRight,ax               ;set large region
        mov     wBottom,ax
        dec     ax
        mov     wLeft,ax
        mov     wTop,ax
        mov     cx,wCount               ;all points
PY_FindMinMax:
        mov     ax,[si].PTTYPE.xcoord   ;x
        cmp     ax,wRight
        jbe     @f                      ;j if < right
        mov     wRight,ax               ;set new right
@@:     cmp     ax,wLeft
        jae     @f                      ;j if >= left
        mov     wLeft,ax
@@:     mov     ax,[si].PTTYPE.ycoord   ;check y
        cmp     ax,wBottom
        jbe     @f                      ;j if < bottom
        mov     wBottom,ax              ;set new bottom
@@:     cmp     ax,wTop
        jae     @f                      ;j if >= Top
        mov     wTop,ax
@@:     add     si,(size PTTYPE)
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
	mov	DIBAccessCallFlag,'L'	;set the DIBAccessCallFlag
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
	mov     PL_clipped,1
DoneClipRect:
        mov     cx,wCount
        lds     si,lpPoints
        mov     eax,pen_color
        call    GlintDrawPolyLine
    mov     ebx, Rop
    mov     eax,dwRasterizerMode
    WaitFifo 4
    mov     dXDom, 0
    mov     dY, 10000h
    mov     RasterizerMode,eax
    mov     ScissorMode,2           ;screen scissor only
    test    ebx, ebx
    je      @f
    mov     eax, dwFBReadMode
    mov     LogicalOpMode, 20h
    mov     ColorDDAMode, 0
    mov     FBReadMode, eax
@@:

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
	cmp	DIBAccessCallFlag,0	    ;did we call DIB eng to exclude cursor?
	je	LineDone   		        ;nope, skip the following!
	push	lpDestDev		    ;
	push	CURSOREXCLUDE		;
	call	DIB_EndAccess		;Let DIB Engine unexclude cursor.
	mov	DIBAccessCallFlag,0	    ;clear the flag
LineDone:
        mov     ax,1
        pop     edi
        pop     esi
        ret
GlintPolyLine   ENDP

GlintDrawPolyLine PROC near PASCAL PUBLIC
        WaitFifo 5
        ;
        ; set block color in case
        ;
        mov     bx,wBpp                 ;get depth
        cmp     bl,32
        je      @f                      ;skip duplicate if 32 bits
        movzx   eax,ax
        mov     dx,ax                   ;double up 16 bits
        shl     edx,16
        or      eax,edx
        cmp     bl,8
        jne     @f
        and     eax,00FF00FFh
        mov     edx,eax
        shl     edx,8
        or      eax,edx
@@:     mov     FBBlockColor,eax        ;just in case
        mov     FBWriteData,eax            ; set fill color
        mov     eax,dwRasterizerMode
        or      al,20h
        mov     RasterizerMode,eax
        dec     cx                      ;cx now number of lines
first_point_again:
        WaitFifo 6
        push    cx                             ; save count
        mov     ax,word ptr [si+4]             ; get x2
        sub     ax,word ptr [si]               ; - x1
        jz      DPLVertical
        mov     bx,ax                           ; save dx
        jns     short @f                        ; j if pos
        neg     ax                              ; adx
@@:     mov     cx,word ptr [si+6]             ; get y2
        sub     cx,word ptr [si+2]             ; - y1
        jz      DPLHorizontal
        mov     dx,cx                           ; dy
        jns     short @f                        ; ady
        neg     cx
@@:     mov     di,word ptr [si]               ; get start x
        shl     edi,16
        mov     StartXDom,edi
        mov     di,word ptr [si+2]             ; start y
        shl     edi,16
        mov     StartY,edi
        cmp     ax,cx                           ; adx >= ady ?
        jl      major_y                         ; j if major is y
        jnz     short @f
        cmp     ax,0
        jnz     short @f                        ; lines is null.. step
        pop     cx
        add     si,4
        loop    first_point_again
        ret
@@:     cmp     bx,0
        mov     edi,10000h                      ; set pos x
        jg      short @f
        mov     edi,0ffff0000h                  ; set neg
@@:     mov     dXDom,edi
        movsx   ebx,ax                          ; save adx
        shl     edx,16
        mov     eax,edx
        cdq
        idiv    ebx
        mov     dY,eax                     ; set dy
        mov     Count,ebx                  ; count
        jmp     do_render                 ; and do the line
DPLCHorizontal:
        WaitFifo 5
DPLHorizontal:
        mov     ax,word ptr [si]                ;start x
 if (CHIP_SX OR CHIP_TX)
        cmp     ax,word ptr [si+4]              ;right to left ?
        jl      @f                              ;j if not
        inc     ax                              ;force draw of right point
        shl     eax,16
        mov     StartXDom,eax
        mov     ax,word ptr [si+4]              ;end x
        inc     ax                              ;dont draw left point
        shl     eax,16
        mov     StartXSub,eax
        jmp     short DPLHY
@@:
 endif
        shl     eax,16
        mov     StartXDom,eax
        mov     ax,word ptr [si+4]              ;end x
        shl     eax,16
        mov     StartXSub,eax
DPLHY:
        mov     Count,1                         ;1 line only
        mov     ax,word ptr [si+2]              ;at y
        shl     eax,16
        mov     StartY,eax
        xor     eax,eax
        test    PL_clipped,-1
        jnz     @f
        mov     eax,dwRenderCommand             ;has fast fill etc
@@:     or      al,40h
        mov     Render,eax                      ; do it
        mov     FBBlockColor, eax               ; Rubbish to avoid MX crash
        add     si,4                            ; pt to next pair
        pop     cx                              ; restore count of points
        loop    jmp_first_point_again               ;and next
        ret
DPLCVertical:
        WaitFifo 6
DPLVertical:
        mov     ax,word ptr [si]                ;start x
        shl     eax,16
        mov     StartXDom,eax
        xor     eax,eax
        mov     dXDom,eax                       ;deltas
        mov     ax,word ptr [si+6]              ;end y
        sub     ax,word ptr [si+2]              ;-start
        mov     bx,word ptr [si+2]              ;start y
        mov     edx,10000h                      ;pos dy
        jns     @f                              ;pos?
        neg     ax                              ;swap ends
        mov     edx,0ffff0000h
@@:     mov     Count,eax
        mov     dY,edx
        shl     ebx,16
        mov     StartY,ebx
        mov     Render,00h              ;do it
        mov     FBBlockColor, eax               ; Rubbish to avoid MX crash
        add     si,4                            ; pt to next pair
        pop     cx                              ; restore count of points
        loop    jmp_first_point_again               ;and next
        ret
jmp_first_point_again:
        jmp     first_point_again
major_y:
        cmp     dx,0
        mov     edi,10000h                      ; set pos x
        jg      short @f
        mov     edi,0ffff0000h                  ; set neg
@@:     mov     dY,edi
        movsx   ecx,cx                          ; save adx
        shl     ebx,16
        mov     eax,ebx
        cdq
        idiv    ecx
        mov     dXDom,eax                  ; set dy
        mov     Count,ecx                  ; count
do_render:                                  ; and do the line
        mov     Render,0h                  ; start it
        mov     FBBlockColor, eax               ; Rubbish to avoid MX crash
        add     si,4                           ; pt to next pair
        pop     cx                             ; restore count of points
        loop    next_point
        ret
next_point:
        push    cx                             ; save count
        WaitFifo 3
        mov     ax,word ptr [si+4]             ; get x2
        sub     ax,word ptr [si]               ; - x1
        jz      DPLCVertical
        mov     bx,ax                           ; save dx
        jns     short @f                        ; j if pos
        neg     ax                              ; adx
@@:     mov     cx,word ptr [si+6]             ; get y2
        sub     cx,word ptr [si+2]             ; - y1
        jz      DPLCHorizontal
        mov     dx,cx                           ; dy
        jns     short @f                        ; ady
        neg     cx
@@:     cmp     ax,cx                           ; adx >= ady ?
        jl      major_y1                        ; j if major is y
        jnz     short @f
        cmp     ax,0
        jnz     short @f                        ; lines is null.. step
        pop     cx
        add     si,4
        loop    next_point
        jmp     done_all_points
@@:     cmp     bx,0
        mov     edi,10000h                      ; set pos x
        jg      short @f
        mov     edi,0ffff0000h                  ; set neg
@@:     mov     dXDom,edi
        movsx   ebx,ax                          ; save adx
        shl     edx,16
        mov     eax,edx
        cdq
        idiv     ebx
        mov     dY,eax                     ; set dy
        mov     ContinueNewLine,ebx        ; count
        mov     FBBlockColor, eax               ; Rubbish to avoid MX crash
        jmp     short do_continue               ; and do the line
major_y1:
        cmp     dx,0
        mov     edi,10000h                      ; set pos x
        jg      short @f
        mov     edi,0ffff0000h                  ; set neg
@@:     mov     dY,edi
        movsx   ecx,cx                          ; save ady
        shl     ebx,16
        mov     eax,ebx
        cdq
        idiv     ecx
        mov     dXDom,eax                  ; set dy
        mov     ContinueNewLine,ecx        ; count
        mov     FBBlockColor, eax               ; Rubbish to avoid MX crash
do_continue:
        pop     cx
        add     si,4
        loop    jmp_next_point
done_all_points:
        ret
jmp_next_point:
        jmp     next_point
GlintDrawPolyLine endp

_TEXT   ends
end
