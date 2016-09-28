;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: output.asm
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
    .xlist
    include glint.inc
    incLogical = 1
    incDrawMode = 1
    incOutput = 1
    include gdidefs.inc
    .list

_INIT segment 

_INIT ends

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT   segment
    assumes cs,_TEXT
	.586

; Jump table. A few of the entries are reused - as the table is not the entire size of
; the possible options. If this is the case, then a further check should be done after
; the jump. 

ROP2toPATROP3Table  dd  0           ; Unused (Rop2 from 1-16!)
                    dd  00000042h   ;/* 00 unset BLACKNESS */    
                    dd  00050000h   ;/* 05 DPon	 */    
                    dd  000a0000h   ;/* 0a DPna	 */    
                    dd  000f0000h   ;/* 0f Pn	 */    
                    dd  00500000h   ;/* 50 PDna	 */    
                    dd  00550009h   ;/* 55 Dn	 DSTINVERT*/    
                    dd  005A0049h   ;/* 5a DPx	 PATINVERT */    
                    dd  005f0000h   ;/* 5f DPan	 */    
                    dd  00a00000h   ;/* a0 DPa	 */    
                    dd  00a50000h   ;/* a5 DPxn	 */    
                    dd  00aa0000h   ;/* aa D	 */    
                    dd  00af0000h   ;/* af DPno	 */    
                    dd  00F00021h   ;/* f0 P	 PATCOPY */    
                    dd  00F50000h   ;/* f5 PDno	 */    
                    dd  00Fa0000h   ;/* fa DPo	 */    
                    dd  00FF0062h   ;/* ff set	 WHITENESS */

PUBLIC OutputJumpTable, OS_SCANLINES_FN, OS_POLYSCAN_FN, OS_RECTANGLE_FN
PUBLIC OS_BEGINSCAN_FN, OS_ENDSCAN_FN, OS_WINDPOLY_FN, OS_ALTPOLY_FN
OutputJumpTable dw      Jmp_Error
                dw      Jmp_DIB             ; 1: OS_POLYBEZIER
                dw      Jmp_Error
                dw      Jmp_DIB             ; 3: OS_ARC
OS_SCANLINES_FN dw      Jmp_DIB             ; 4: OS_SCANLINES
OS_POLYSCAN_FN  dw      GlintPolyScanLine   ; 5: OS_POLYSCANLINE
OS_RECTANGLE_FN dw      GlintRectangle      ; 6: OS_RECTANGLE
                dw      Jmp_DIB             ; 7: OS_ELLIPSE                 39: OS_CHORD
                dw      Jmp_DIB             ; 8: OS_MARKER
                dw      Jmp_Error       
                dw      Jmp_Error
                dw      Jmp_Error
                dw      Jmp_Error
                dw      Jmp_Error
                dw      Jmp_Error
                dw      Jmp_Error
OS_BEGINSCAN_FN dw      Jmp_DIB             ; 80: OS_BEGINNSCAN
OS_ENDSCAN_FN   dw      Jmp_DIB             ; 81: OS_ENDNSCAN
                dw      GlintPolyLine       ; 18: OS_POLYLINE
                dw      Jmp_Error
OS_WINDPOLY_FN  dw      Jmp_DIB             ; 20: OS_TRAPEZOID/OS_WINDPOLYGON
                dw      Jmp_Error
OS_ALTPOLY_FN   dw      Jmp_Polygon        ; 22: OS_ALTPOLYGON/OS_POLYGON
;OS_ALTPOLY_FN   dw      Jmp_DIB             ; 22: OS_ALTPOLYGON/OS_POLYGON
                dw      Jmp_DIB             ; 23: OS_PIE                    55: OS_CIRCLE
                dw      Jmp_DIB             ; 24: OS_POLYMARKER
                dw      Jmp_Error
                dw      Jmp_Error
                dw      Jmp_Error
                dw      Jmp_Error
                dw      Jmp_Error
                dw      Jmp_Error
                dw      Jmp_Error

Output PROC FAR PASCAL PUBLIC     
    mov     bx,sp
    mov     bx,ss:[bx+26]           ;get wStyle
    and     bx, 01fh
    add     bx, bx
    lea     bx, [bx+OutputJumpTable]
    jmp     cs:[bx]

Jmp_Polygon:
    jmp     GlintPolygon
Jmp_Error:
    int     1
Jmp_DIB:
    jmp     DIB_Output
Output      ENDP


ifdef DEBUG
GlintNULL PROC FAR PASCAL PUBLIC USES ds, lpDestDev:DWORD, wStyle:WORD, wCount:WORD,         
    lpPoints:DWORD, lpPPen:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD, lpClipRect:DWORD     
    mov     ax, 1
    ret
GlintNULL ENDP
endif

RECTPOINTLIST	STRUCT
	Point1  	DWORD	0
	Point2      DWORD	0
	Point3	    DWORD	0
	Point4		DWORD	0
	Point5	    DWORD	0
RECTPOINTLIST	ENDS

; This stub function needs to convert a rectangle call into a bitblt and a lines call
; to provide the same functionality as the rectangle call would have normally.
; We want to check on each case whether we have to do anything.
GlintRectangle PROC FAR PASCAL PUBLIC USES ds es si di, lpDestDev:DWORD, wStyle:WORD, wCount:WORD,         
    lpPoints:DWORD, lpPPen:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD, lpClipRect:DWORD
    LOCAL Lines:RECTPOINTLIST
    
    
    lds     si, lpPBrush
    cmp     ds:[si].dp1BrushStyle, BS_HOLLOW
    je      Rect_BrushDone
; Draw the rectangle. Use bitblt to do the honours
    lds     si, lpPoints
    mov     eax, ds:[si+0]          ; Top Left
    mov     ebx, ds:[si+4]          ; Bottom Right
    cmp     eax, ebx                ; make sure TOP < BOTTOM
    jle     @f
    mov     edx, ebx
    mov     ebx, eax
    mov     eax, edx
@@:
    cmp     ax, bx                  ; make sure LEFT < RIGHT
    jle     @f
    mov     dx, bx
    mov     bx, ax
    mov     ax, dx
@@:
    ; Reduce witdh and height by one to render correct area.
    sub     ebx, 00010001h
    cmp     eax, ebx                ; Check to see if we have gone negative
    jge     Rect_BrushDone
    cmp     ax, bx                  ; Check to see if we have gone negative
    jge     Rect_BrushDone

    ; Default clip is to the screen
    cmp     lpClipRect, 0
    je      @f
    lds     si, lpClipRect
    mov     ecx, ds:[si+0]          ; Top Left Clip
    mov     edx, ds:[si+4]          ; Bottom Right Clip
    cmp     ecx, edx                ; zero clip region?
    je      Rect_Done               ; all done if so.
    jmp     Rect_GotClip
@@:
    lds     si, lpDestDev
    xor     ecx, ecx
    mov     edx, DWORD PTR ds:[si].deWidth; get Width | Height << 16
Rect_GotClip:

    ; Clipping. First check for complete exclusion
    cmp     ebx, ecx                ; Is rectangle entirly above clip?
    jl      Rect_Done
    cmp     eax, edx                ; Is rectangle entirly below clip region?
    jge     Rect_Done
    cmp     bx, cx                  ; Is rectangle entirly left of clip?
    jl      Rect_Done
    cmp     ax, dx                  ; Is rectangle entirly right of clip region?
    jge     Rect_Done

    ; Now intersect rectangles. We know they overlap.
    cmp     ax, cx                  ; Compare the lefts
    jge     @f                      ; Set left to clip
    mov     ax, cx
@@:
    cmp     bx, dx                  ; Compare the rights
    jle     @f                      ; Set right to clip
    mov     bx, dx
@@:

    ror     eax, 16                 ; rotate parameters ready to be pushed on the stack
    ror     ecx, 16                 ; rotate clip for comparasons.
    ror     ebx, 16
    ror     edx, 16

    cmp     ax, cx                  ; Compare the tops
    jge     @f                      ; Set top to clip
    mov     ax, cx
@@:
    cmp     bx, dx                  ; Compare the bottoms
    jle     @f                      ; Set bottom to clip
    mov     bx, dx
@@:

    ; Now set up the render
    lds     si, lpDrawMode
    sub     ebx, eax                ; convert bottom right to width+height
    xor     ecx, ecx

	mov     di, WORD PTR ds:[si].DRAWMODE.Rop2
    push    lpDestDev               ; lpDestDev
    shl     di, 2                   ; * 4
    push    eax                     ; DestX, DestY
    lea     si, [di + ROP2toPATROP3Table]
    push    ecx                     ; lpSrcDev
    mov     edx, cs:[si]
    push    ecx                     ; SrcX = SrcY = 0
    push    ebx                     ; Dest Width, Dest Height
    push    edx                     ; ROP
    push    lpPBrush                ; lpBrush
    push    lpDrawMode              ; lpDrawMode
    call    BitBlt16                ; Do the blt

Rect_BrushDone:

    lds     si, lpPPen
    cmp     ds:[si].dpPenStyle, LS_NOLINE
    je      Rect_PenDone

    ; Check for a zero clip region - as the DIB ENGINE lines code treats
    ; zero region sized clip regions as unclipped!
    cmp     lpClipRect, 0
    je      @f
    lds     si, lpClipRect
    mov     ecx, ds:[si+0]          ; Top Left Clip
    mov     edx, ds:[si+4]          ; Bottom Right Clip
    cmp     ecx, edx                ; zero clip region?
    je      Rect_Done               ; all done if so.
@@:

    lds     si, lpPoints

    mov     ebx, ds:[si+0]          ; Top Left
    mov     ecx, ds:[si+4]          ; Bottom Right
    cmp     ebx, ecx                ; make sure TOP < BOTTOM
    jle     @f
    mov     edx, ecx
    mov     ecx, ebx
    mov     ebx, edx
@@:
    cmp     bx, cx                  ; make sure LEFT < RIGHT
    jle     @f
    mov     dx, cx
    mov     cx, bx
    mov     bx, dx
@@:
    sub     ecx, 00010001h          ; Draw outline - inclusive points.
    mov     eax, ebx                ; eax = TL, ebx = TL, ecx = BR, edx = BR
    mov     edx, ecx
    mov     si, ax
    mov     ax, cx
    mov     cx, si                  ; eax = TR, ebx = TL, ecx = BL, edx = BR                         

    mov     Lines.Point1, eax
    mov     Lines.Point2, ebx
    mov     Lines.Point3, ecx
    mov     Lines.Point4, edx
    mov     Lines.Point5, eax

    lea     di, Lines.Point1
    push    lpDestDev
    push    WORD PTR OS_POLYLINE
    push    WORD PTR 5
    push    ss
    push    di                      ; lpPoints
    push    lpPPen
    push    lpPBrush
    push    lpDrawMode
	push    lpClipRect
    call    GlintPolyLine

Rect_PenDone:
      
Rect_Done:
    mov     ax, 1
    ret
GlintRectangle ENDP


_TEXT   ends
end
