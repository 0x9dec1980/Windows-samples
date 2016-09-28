;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: polygon.asm
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
    .xlist
CODE32BPP = 1
    include glint.inc
    incLogical = 1
    incDrawMode = 1
    incOutput = 1
    include gdidefs.inc
	.list
;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------
ROP2_BLACKNESS  equ     00h
ROP2_COPYPEN    equ     0ch
ROP2_WHITENESS  equ     0fh

ROP_REQ_SRC  =     0200h                   ; source read required
ROP_REQ_DST  =     0400h                   ; dest read required

_INIT segment 

    globalD     SolidPolygonCmd, 48h
    globalD     ROPPolygonCmd, 40h

PUBLIC Temp1KSpace
Temp1KSpace     db  1000 dup (0)
Temp1KSpace1000 db  24 dup (0)              ; Label for nearing the end of 1K

_INIT ends


_OUTPUT32   segment
assumes cs,_OUTPUT32

PLABEL P_CallDIBEngine
    pop     edi
    pop     esi
    pop     ds
    mov     sp,bp					; transfer only the bottom 16 bits
    pop     ebp

	StackMangle32To16				; convert the 32:32 ret addr back to 16:16

	jmp	DIB_Output

PLABEL  GlintPolygon

	StackMangle16To32

GlintPolygonParms	STRUCT
	_ebp		DWORD	0
	_retLo		DWORD	0
	_retHi		DWORD	0

	lpClipRect	DWORD	0
	lpDrawMode	DWORD	0
	lpPBrush	DWORD	0
	lpPPen		DWORD	0
	lpPoints	DWORD	0
	wCount  	WORD	0
	wStyle  	WORD	0
	lpDestDev	DWORD	0
GlintPolygonParms	ENDS

; These locals are shared between HW and Span code.
Swapped     equ Rop
PolygonCmd  equ SpanCount

GlintPolygon32	PROC FAR PASCAL PUBLIC		;remember that USES would override "ret" code...
    LOCAL   ptFirst:DWORD, ptLast:DWORD, ptTop:DWORD, bcol:DWORD, Rop:DWORD,
        LastL:DWORD, LastR:DWORD, SpanCount:DWORD, SpanToDoCount:DWORD, SpanPtr:DWORD,
        dwLeft:DWORD, dwTop:DWORD, dwRight:DWORD, dwBottom:DWORD,
        npntL:DWORD, npntR:DWORD, pntL:DWORD, pntR:DWORD, dxdyL:DWORD, dxdyR:DWORD,
        DrawTrap:DWORD, CRTrap:DWORD, CLTrap:DWORD, FinPoly:DWORD

	and		ebp,0ffffh
	push	ds
	push	esi					            ;save 32 bit versions of these
	push	edi					            ;

    assumes ds,nothing
    assumes es,nothing		
    assumes fs,nothing
    assumes gs,nothing

    xor     edi, edi
    lds     di,[ebp].lpDestDev                      ;ds:di-->dest pdevice
    mov     eax, DWORD PTR ds:[edi].deFlags
    mov     DrawTrap, OFFSET P_StartHardwareTrapezoid
    test    eax,VRAM
    jz      P_CallDIBEngine                         ;out if busy
    test    eax,BUSY
    jnz     P_CallDIBEngine                         ;out if busy

	cmp	    ds:[edi].deType,TYPE_DIBENG             ;is this a DIBENGINE surface?
    jne     P_CallDIBEngine

    test    ds:[edi].deDriverReserved,ACCELERATED   ;is destination accelerated?
    jne     @f                              ;no.
    mov     DrawTrap, OFFSET P_StartSpanTrapezoid
@@:

; We dont accelerate any kinds of pen.
    mov     eax, [ebp].lpPPen
    xor     esi, esi
	test    eax, eax
	je      @f
    lds     si, [ebp].lpPPen
	mov     eax, DWORD PTR ds:[si].dpPenStyle
	cmp     ax,LS_NOLINE
	je      @f
    jmp     P_CallDIBEngine
@@:
	;
	;       Process lpPBrush.
	;
    mov     eax, [ebp].lpPBrush
    xor     esi, esi
	test    eax, eax
	je      DoneBrush
	lds     si,[ebp].lpPBrush
	mov     ax,ds:[esi].dp8BrushStyle
    cmp     ax,BS_SOLID
    jne     P_BrushNotSolid
    mov     al, ds:[esi].dp8BrushFlags
    test    al, COLORSOLID
    jne     P_SetupSolidBrush
    ; Dithered brush. Spans dont seem to handle this case (they draw solid
    ; lines rather than brushes)
    jmp     P_SetupSpanBrush
P_BrushNotSolid:
	cmp     ax,BS_HOLLOW
    je      P_PolygonFinished

P_SetupSpanBrush:
    ; HardwareTrapezoid can only handle solid filled polygons.
    mov     DrawTrap, OFFSET P_StartSpanTrapezoid
    jmp     DoneBrush
P_SetupSolidBrush:
	mov     eax,dword ptr ds:[esi].dp8BrushBits
	mov     bcol,eax
DoneBrush:

;
; Process lpPoints. We need to decide which point to start drawing with and
; if we can actually handle the polygon or not.
;  
	lds     si,[ebp].lpPoints
	mov     edi, DWORD PTR [ebp].wCount
    and     edi, 0ffffh
	cmp     edi,(3+1)
	jb      PolygonError                    ; Polygon has < 3 points
    mov     ptFirst, esi
	dec     edi
	shl     edi,2
	add     edi,esi
    mov     eax, ds:[edi]                   ; Check for first point = last point
    cmp     eax, ds:[esi]
    jne     @f
    sub     edi, 4                          ; remove replicated point from list
@@:
	mov     ptLast,edi                      ; save away last point in array.

    xor     edx, edx
    xor     ecx, ecx
    mov     ptTop, esi                      ; First point is top so far!
InitialSameY:
    cmp     esi,edi
	ja      P_PolygonFinished               ; Reached end of list - all on the same Y
	mov     ebx,ds:[esi+(size PTTYPE)]
    mov     eax,ds:[esi]
	add     esi,(size PTTYPE)
    or      edx, eax                        ; Keep track of what bits accessed
    and     eax,0ffff0000h
    or      edx, ebx                        ; Keep track of what bits accessed
    and     ebx,0ffff0000h
	sub     eax,ebx
	je      InitialSameY                    ; Y position is the same
    jl      ChangingToDown

ChangingToUp:
    inc     ecx                             ; Increase direction count
GoingUp:
    cmp     esi, edi
    jge     ReachedEndUp
    mov     eax, ebx                        ; eax = ds:[esi]
    mov     ebx, ds:[esi+(size PTTYPE)]
    add     esi, 4
    or      edx, ebx                        ; Keep track of what bits accessed
    and     ebx,0ffff0000h
    sub     eax, ebx
    jge     GoingUp
    ; Fall through to:
ChangingToDown:
    mov     eax, ptTop                      ; Get address of last top point
    mov     eax, [eax]
    cmp     eax, ds:[esi-4]                 ; Compare last highest with this high point
    jle     @f
    lea     eax, [esi-4]                    ; Update highest point
    mov     ptTop, eax
@@:     
    inc     ecx                             ; increase direction count
GoingDown:
    cmp     esi, edi
    jge     ReachedEndDown
    mov     eax, ebx                        ; eax = ds:[esi]
    mov     ebx, ds:[esi+(size PTTYPE)]
    add     esi, 4
    or      edx, ebx                        ; Keep track of what bits accessed
    and     ebx,0ffff0000h
    sub     eax, ebx
    jle     GoingDown
    jmp     ChangingToUp

ReachedEndUp:
    mov     eax, ptTop                      ; Get address of last top point
    mov     eax, [eax]
    cmp     eax, ds:[esi]                   ; Compare last highest with this high point
    jle     @f
    lea     eax, [esi]                      ; Update highest point
    mov     ptTop, eax
@@:     
    ; ebx contains the last point in the list
    mov     esi,ptFirst                     ; recall first point
    cmp     ecx, 3                          
    jl      PolygonAccepted                 ; <3 edges we can handle.
    jg      PolygonRejected                 ; >3 edges we can handle.
    ; 3 directions - we have to check a little more
    mov     eax, [esi]
    and     eax, 0ffff0000h
    cmp     eax, ebx                        ; Compare last point with first point
    jle     PolygonAccepted                 ; First point is above last point
PolygonRejected:
    jmp     P_CallDIBEngine                ; First point below last -> Reject
    
ReachedEndDown:
    ; ebx contains the last point in the list
    mov     esi,ptFirst                     ; recall first point
    cmp     ecx, 3                          
    jl      PolygonAccepted                 ; <3 edges we can handle.
    jg      PolygonRejected                 ; >3 edges we can handle.
    ; 3 directions - we have to check a little more
    mov     eax, [esi]
    and     eax, 0ffff0000h
    cmp     eax, ebx                        ; Compare last point with first point
    jge     PolygonAccepted                 ; First point is below last point
    jmp     P_CallDIBEngine                ; First point above last -> Reject

PolygonAccepted:
    ; edx contains all the points or'ed together. Check to see if the polygon is inside area
    ; if we go into hardware code
    mov     eax,_INIT
    mov     ds, eax
	assumes	ds,_INIT
	mov	    eax,RemappedDataSegment
	mov	    ds,eax

    jmp     [DrawTrap]

PLABEL P_StartSpanTrapezoid
; Span specific configuration
    mov     DrawTrap, P_SpanTrapezoid
    mov     CRTrap, P_SpanContRight
    mov     CLTrap, P_SpanContLeft
    mov     FinPoly, P_SpanFinishPolygon

    ; set up the clipping region. X limits are in 16.16 format. Y in 32.0 format
    mov     ebx, GLInfo.dwScreenWidth
    xor     eax, eax
    mov     ecx, GLInfo.dwScreenHeight
    mov     dwLeft, eax
    shl     ebx, 16
    mov     dwTop, eax
    mov     dwBottom, ecx
    mov     dwRight, ebx
;
;       Process lpClipRect.
;
    cmp     [ebp].lpClipRect,0
    je      @f
    lfs     si,[ebp].lpClipRect
    mov     eax, DWORD PTR fs:[esi].left    ; Get left + (top << 16)
    mov     ecx, DWORD PTR fs:[esi].right   ; Get right + (bottom << 16)
    mov     ebx, eax
    mov     edx, ecx
    shl     eax, 16
    sar     ebx, 16
    mov     dwLeft, eax
    shl     ecx, 16
    mov     dwTop, ebx
    sar     edx, 16
    mov     dwRight, ecx
    mov     dwBottom, edx
@@:

    ; Initialise span information
    mov     SpanPtr, OFFSET Temp1KSpace
    mov     SpanCount, 0

    jmp     P_BreakIntoTrapezoids

PLABEL P_StartHardwareTrapezoid

; check edx for limits of HW rendering
    mov     eax, 0fC00f800h                ; Permedia HW range is 0 <= x <= 2047, 0 <= y <= 1024
    cmp     GLInfo.dwRenderFamily, GLINT_ID
    jne     @f
    mov     eax, 0f800f800h                ; TX HW range is -2048 <= x <= 2047, -2048 <= y <= 2047
@@:
    test    edx, eax
    jne     P_StartSpanTrapezoid            ; Use spans if we are outside area
; Polygon is Within Limits:

; Work out the render command to use. At 24bpp on P2 we need to ensure Block Fill is disabled
    mov     eax, SolidPolygonCmd
    cmp     GLInfo.dwBpp, 24
    jne     P_RenderCommandCorrect
    cmp     al, ah
    jne     @f
    mov     ebx, eax
    shr     ebx, 8
    cmp     al, ah
    je      P_RenderCommandCorrect
@@:
    and     eax, 0fffffff7h
P_RenderCommandCorrect:
    xor     ebx, ebx
    mov     Rop, ebx
    mov     PolygonCmd, eax

; Process lpDrawMode. We cannot accelerate any Dest rops if there are lines involved - 
; as we will end up drawing some pixels twice.
; Also convert Blackness and Whiteness into solid fills.
    xor     esi, esi
	lfs     si,[ebp].lpDrawMode
	mov     eax, DWORD PTR fs:[esi].DRAWMODE.Rop2
	dec     eax                             ; Convert to index
	and     eax,0000fh                      ; Ensure 0 <= eax < 16
	cmp     eax,ROP2_COPYPEN
	je      P_RopIsOK
    xor     ebx,ebx
    cmp     eax,ROP2_BLACKNESS
    je      P_RopOverride
    not     ebx
    cmp     eax,ROP2_WHITENESS
	je      P_RopOverride
; Not Blackness, Whiteness or Copy
; Check to see if we are drawing a line around the polygon
	cmp     [ebp].lpPPen,0
	je      P_UseROP
	lfs     si,[ebp].lpPPen
	mov     ebx,DWORD PTR fs:[esi].dpPenStyle
	cmp     bx,LS_NOLINE
	je      P_UseROP
    jmp     P_CallDIBEngine
P_UseROP:
    mov     ebx, [rop2Table + 4 + eax*4]    ; Get FBReadMode and LogicalopMode
    mov     edx, dwFBReadMode
    mov     eax, ROPPolygonCmd
    mov     Rop, ebx                        ; Indicate that a ROP is taking place.
    xor     ecx, ecx
    mov     cl, bl                          ; ecx now contains Logicalop
    and     ebx, ROP_REQ_DST                ; ebx now contains FBReadMode modifier
    or      ebx, edx                        ; ebx is now FBReadMode
    mov     edx, bcol
    mov     PolygonCmd, eax
    mov     LogicalOpMode, ecx
    mov     ColorDDAMode, 1
    mov     FBReadMode, ebx
    mov     ConstantColor, edx
    jmp     P_RopIsOK
P_RopOverride:
    mov     bcol,ebx
    xor     eax, eax
    mov     [ebp].lpPPen, eax               ; If we are blackness or whiteness, there is no point in drawing outline
P_RopIsOK:

; Hardware specific configuration
    mov     DrawTrap, P_HWTrapezoid
    mov     CRTrap, P_HWContRight
    mov     CLTrap, P_HWContLeft
    mov     FinPoly, P_HWFinishPolygon

	cmp	    GLInfo.dwCursorType,0   	    ;running with a software cursor?
    je      RemoveSWCursor
SWCursorRemoved:

;
;We need to set the BUSY bit in the destination PDevice structure so that
;the DIB engine doesn't try to asynchronously draw the cursor while we're
;using the hardware to BLT.
;

    DISPLAY_CONTEXT
    xor     esi, esi
	lfs	    si, GLInfo.lpDriverPDevice
    mov     DD_CurrentContext, DD_UNDEFINED_CTXT
	or	    fs:[esi].deFlags,BUSY

    mov     eax, [ebp].lpDestDev
    cmp     eax, GLInfo.dwCurrentOffscreen
    je      @f
    lfs     si, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, fs:[esi].OSB_PixelOffset
	WaitFifo 1
    mov     FBWindowBase, edx
@@:

;
;       Process lpClipRect.
;
    cmp     [ebp].lpClipRect,0
    je      P_DoneClipRect
    lfs     si,[ebp].lpClipRect
    mov     ScissorMode,3
    mov     eax, DWORD PTR fs:[esi].left    ; Get left + (top << 16)
    mov     ebx, DWORD PTR fs:[esi].right   ; Get right + (bottom << 16)
    mov     ScissorMinXY,eax
    mov     ScissorMaxXY,ebx
P_DoneClipRect:

    mov     ebx,GLInfo.dwBpp                ; get depth
	mov     eax,bcol
    cmp     bl,16
    ja      P_ReplicatedColor               ; skip duplicate if 32 bits
    je      @f                              ; skip 8->16 if at 16bpp
    mov     ah,al
@@:
    mov     ebx, eax
    and     eax, 0ffffh
    shl     ebx, 16
    or      eax, ebx
P_ReplicatedColor:
 	mov     FBWriteData,eax                 ; set fixed color
    mov     FBBlockColor,eax        ;just in case

P_BreakIntoTrapezoids:


;       /* Initialize left and right points (current) to top point. */
;        npnt[LEFT]  = ptTop;
;        npnt[RIGHT] = ptTop;

    mov     eax, [ebp].lpPoints+2
	mov     esi,ptTop
	mov     npntL,esi
	mov     npntR,esi
    push    eax
    pop     es

;
;        while (1) {

P_NewPolygon:

;            // npnt[] is always the valid point to draw from
;            do {

P_FindFirstLeft:
	mov     esi,npntL                           ; pnt[LEFT] = npnt[LEFT];  
	mov     pntL,esi
    lea     edi, [esi - 4]                      ; npnt[LEFT] = pnt[LEFT] - 1;
	cmp     edi, ptFirst                        ; if (npnt[LEFT] < ptFirst)
	jge     @f
	mov     edi,ptLast                          ; npnt[LEFT] = ptend;
@@:
; Check for Special case of flat based polygon, need to break now as polygon is finished
	cmp     edi,npntR                           ; if (npnt[LEFT] == npnt[RIGHT])
	jz      P_DonePolygon

	mov     eax,es:[esi]                        ; }  while (pnt[LEFT]->y == npnt[LEFT]->y);
	mov     npntL,edi
	mov     ebx,es:[edi]
    mov     ecx, eax
    mov     edx, ebx
    sar     eax, 16
    sar     ebx, 16
	cmp     eax, ebx
	jz      P_FindFirstLeft

    and     ecx, 0ffffh
    and     edx, 0ffffh
; Work out the Delta. eax = pnt[LEFT]->y, ebx = npnt[LEFT]->y, ecx = pnt[LEFT]->x, edx = npnt[LEFT]->x

    sub     edx, ecx                            ; dx[LEFT] = (npnt[LEFT]->x - pnt[LEFT]->x) << 16;
    sub     ebx, eax                            ; dy[LEFT] = (npnt[LEFT]->y - pnt[LEFT]->y);
    shl     edx, 16

;            // Need to ensure we round delta down. divide rounds towards zero
    test    edx, edx                            ; if (dx[LEFT] < 0)
    jns     @f
	sub     edx, ebx                            ; dx[LEFT] -= dy[LEFT] - 1;
	inc     edx
@@:
    mov     eax, edx
	cdq                                         ; gdx[LEFT] = dx[LEFT] / dy[LEFT];
	idiv    ebx
    mov     dxdyL, eax

P_FindFirstRight:
	mov     esi,npntR                           ; pnt[RIGHT] = npnt[RIGHT];  
	mov     pntR,esi
    lea     edi, [esi + 4]                      ; npnt[RIGHT] = pnt[RIGHT] + 1;
	cmp     edi, ptLast                         ; if (npnt[RIGHT] < ptFirst)
	jle     @f
	mov     edi,ptFirst                         ; npnt[RIGHT] = ptFirst;
@@:
	mov     eax,es:[esi]                         ; }  while (pnt[RIGHT]->y == npnt[RIGHT]->y);
	mov     npntR,edi
	mov     ebx,es:[edi]

    mov     ecx, eax
    mov     edx, ebx
    sar     eax, 16
    sar     ebx, 16
	cmp     eax, ebx
	jz      P_FindFirstRight

    and     ecx, 0ffffh
    and     edx, 0ffffh
; Work out the Delta. eax = pnt[RIGHT]->y, ebx = npnt[RIGHT]->y, ecx = pnt[RIGHT]->x, edx = npnt[RIGHT]->x

    sub     edx, ecx                            ; dx[RIGHT] = (npnt[RIGHT]->x - pnt[RIGHT]->x) << 16;
    sub     ebx, eax                            ; dy[RIGHT] = (npnt[RIGHT]->y - pnt[RIGHT]->y);
    shl     edx, 16

;            // Need to ensure we round delta down. divide rounds towards zero
    test    edx, edx                            ; if (dx[RIGHT] < 0)
    jns     @f
	sub     edx, ebx                            ; dx[RIGHT] -= dy[RIGHT] - 1;
	inc     edx
@@:
    mov     eax, edx
	cdq                                         ; gdx[RIGHT] = dx[RIGHT] / dy[RIGHT];
	idiv    ebx
    mov     dxdyR, eax

    jmp     [DrawTrap]

PLABEL P_HWTrapezoid
    mov     eax, dxdyL
    mov     ebx, dxdyR
	mov     dXDom,eax                           ; LD_GLINT_FIFO(__GlintTagdXDom, gdx[LEFT]);
	mov     dXSub,ebx                           ; LD_GLINT_FIFO(__GlintTagdXSub, gdx[RIGHT]);

	mov     esi,pntL
	mov     edi,pntR
	mov     eax,es:[esi]                        ; eax = L: (y<<16) + x 
	mov     ebx,es:[edi]                        ; ebx = R: (y<<16) + x
	shl     eax, 16
    mov     edx, ebx                            ; edx = R: (y<<16) + x
    shl     ebx, 16
    or      eax, 07fffh                         ; Or in Bias
	mov     StartXDom,eax                       ; LD_GLINT_FIFO(__GlintTagStartXDom, pnt[LEFT]->x << 16);
    or      ebx, 07fffh                         ; Or in Bias
	mov     StartXSub,ebx                       ; LD_GLINT_FIFO(__GlintTagStartXSub, pnt[RIGHT]->x << 16);
    and     edx, 0ffff0000h                     ; Zero the fractional bits
	mov     StartY,edx                          ; LD_GLINT_FIFO(__GlintTagStartY, pnt[RIGHT]->y >> 16);
;            // Work out number of scanlines to render
	mov     esi,npntL
	mov     edi,npntR
	mov     eax,es:[esi]                        ; Assume npnt[LEFT]->y < npnt[RIGHT]->y
	mov     ebx,es:[edi]
	cmp     eax, ebx
	jl      @f
    mov     eax, ebx                            ; npnt[LEFT]->y > npnt[RIGHT]->y
@@:
    sub     eax, edx                            ; subtract top Y from next Y
    sar     eax, 16                             ; Convert back to normal format
    mov     ebx, PolygonCmd
    mov     Count, eax                          ; LD_GLINT_FIFO(__GlintTagCount, count);
    mov     Render, ebx                         ; LD_GLINT_FIFO(__GlintTagRender, renderBits);
    ; Fall through to

; With lots of luck, top trapezoid should be drawn now! Repeatedly draw more trapezoids
; until points are equal If y values are equal, then we can start again from scratch.

;            while (npnt[LEFT] != npnt[RIGHT] && npnt[LEFT]->y != npnt[RIGHT]->y) {

PLABEL P_NextTrapezoid
	mov     esi,npntL                           ; If the nPoints are equal, then we must have
	mov     edi,npntR                           ; reached the end of the polygon
	cmp     esi,edi
	je      P_DonePolygon
	mov     eax,es:[esi]
	mov     ebx,es:[edi]
    and     eax, 0ffff0000h
    and     ebx, 0ffff0000h
	cmp     eax, ebx
	je      P_NewPolygon                        ; If Y values are equal, treat as new polygon
    jg      P_ContinueLeft                      ; npnt[LEFT]->y > npnt[RIGHT]->y - Continue Left

PLABEL P_ContinueRight                          ; Reached npnt[LEFT]. npnt[RIGHT] is still ok
    mov     esi,npntL                           ; pnt[LEFT] = npnt[LEFT];  
	mov     pntL,esi
    lea     edi, [esi - 4]                      ; npnt[LEFT] = pnt[LEFT] - 1;
	cmp     edi, ptFirst                        ; if (npnt[LEFT] < ptFirst)
	jge     @f
	mov     edi,ptLast                          ; npnt[LEFT] = ptend;
@@:
; Check for Special case of flat based polygon, need to break now as polygon is finished
	cmp     esi,npntR                           ; if (npnt[LEFT] == npnt[RIGHT])
	jz      P_DonePolygon

	mov     eax,es:[esi]                        ; }  while (pnt[LEFT]->y == npnt[LEFT]->y);
	mov     npntL,edi
	mov     ebx,es:[edi]
    mov     ecx, eax
    mov     edx, ebx
    sar     eax, 16
    sar     ebx, 16
	cmp     eax, ebx
	jz      P_ContinueRight

    and     ecx, 0ffffh
    and     edx, 0ffffh
; Work out the Delta. eax = pnt[LEFT]->y, ebx = npnt[LEFT]->y, ecx = pnt[LEFT]->x, edx = npnt[LEFT]->x

    sub     edx, ecx                            ; dx[LEFT] = (npnt[LEFT]->x - pnt[LEFT]->x) << 16;
    sub     ebx, eax                            ; dy[LEFT] = (npnt[LEFT]->y - pnt[LEFT]->y);
    shl     edx, 16

;            // Need to ensure we round delta down. divide rounds towards zero
    test    edx, edx                            ; if (dx[LEFT] < 0)
    jns     @f
	sub     edx, ebx                            ; dx[LEFT] -= dy[LEFT] - 1;
	inc     edx
@@:
    mov     eax, edx
	cdq                                         ; gdx[LEFT] = dx[LEFT] / dy[LEFT];
	idiv    ebx
    mov     dxdyL, eax
    jmp     [CRTrap]

PLABEL P_HWContRight
    ;       ebx = dyL
    mov     eax, dxdyL
	mov     dXDom,eax                           ; LD_GLINT_FIFO(__GlintTagdXDom, gdx[LEFT]);

	mov     esi,pntL
	mov     eax,es:[esi]                        ; eax = L: (y<<16) + x 
    mov     ecx, eax                            ; ecx = L: (y<<16) + x 
	shl     eax, 16
    and     ecx, 0ffff0000h
    or      eax, 07fffh                         ; Or in Bias
	mov     StartXDom,eax                       ; LD_GLINT_FIFO(__GlintTagStartXDom, pnt[LEFT]->x << 16);

; Work out number of scanlines to render
	mov     esi,npntL
	mov     edi,npntR
	mov     eax,es:[esi]                        ; Assume npnt[LEFT]->y < npnt[RIGHT]->y
	mov     edx,es:[edi]                        ; -> count = dy (ebx)
	cmp     eax, edx
	jl      @f
    mov     ebx, edx
    sub     ebx, ecx                            ; npnt[LEFT]->y > npnt[RIGHT]->y.
    sar     ebx, 16                             ; count = npnt[RIGHT]->y - pnt[LEFT]->y;
@@:
    mov     ContinueNewDom, ebx                 ; LD_GLINT_FIFO(__GlintTagContinueNewDom, count);   
	jmp     P_NextTrapezoid                     ; Continue with next trapezoid


PLABEL P_ContinueLeft                           ; Reached npnt[RIGHT]. npnt[LEFT] is still ok
	mov     esi,npntR                           ; pnt[RIGHT] = npnt[RIGHT];  
	mov     pntR,esi
    lea     edi, [esi + 4]                      ; npnt[RIGHT] = pnt[RIGHT] + 1;
	cmp     edi, ptLast                         ; if (npnt[RIGHT] < ptFirst)
	jle     @f
	mov     edi,ptFirst                         ; npnt[RIGHT] = ptFirst;
@@:
	mov     eax,es:[esi]                         ; }  while (pnt[RIGHT]->y == npnt[RIGHT]->y);
	mov     npntR,edi
	mov     ebx,es:[edi]

    mov     ecx, eax
    mov     edx, ebx
    sar     eax, 16
    sar     ebx, 16
	cmp     eax, ebx
	jz      P_ContinueLeft

    and     ecx, 0ffffh
    and     edx, 0ffffh
; Work out the Delta. eax = pnt[RIGHT]->y, ebx = npnt[RIGHT]->y, ecx = pnt[RIGHT]->x, edx = npnt[RIGHT]->x

    sub     edx, ecx                            ; dx[RIGHT] = (npnt[RIGHT]->x - pnt[RIGHT]->x) << 16;
    sub     ebx, eax                            ; dy[RIGHT] = (npnt[RIGHT]->y - pnt[RIGHT]->y);
    shl     edx, 16

;            // Need to ensure we round delta down. divide rounds towards zero
    test    edx, edx                            ; if (dx[RIGHT] < 0)
    jns     @f
	sub     edx, ebx                            ; dx[RIGHT] -= dy[RIGHT] - 1;
	inc     edx
@@:
    mov     eax, edx
	cdq                                         ; gdx[RIGHT] = dx[RIGHT] / dy[RIGHT];
	idiv    ebx
    mov     dxdyR, eax
    jmp     [CLTrap]

PLABEL P_HWContLeft
    ;       ebx = dyR
    mov     eax, dxdyR
	mov     dXSub,eax                           ; LD_GLINT_FIFO(__GlintTagdXSub, gdx[RIGHT]);

	mov     esi,pntR
	mov     eax,es:[esi]                        ; eax = R: (y<<16) + x 
    mov     ecx, eax                            ; ecx = R: (y<<16) + x 
	shl     eax, 16
    and     ecx, 0ffff0000h
    or      eax, 07fffh                         ; Or in Bias
	mov     StartXSub,eax                       ; LD_GLINT_FIFO(__GlintTagStartXDom, pnt[RIGHT]->x << 16);

; Work out number of scanlines to render
	mov     esi,npntR
	mov     edi,npntL
	mov     eax,es:[esi]                        ; Assume npnt[RIGHT]->y < npnt[LEFT]->y
	mov     edx,es:[edi]                        ; -> count = dyR (ebx)
	cmp     eax, edx
	jl      @f
    mov     ebx, edx
    sub     ebx, ecx                            ; npnt[RIGHT]->y > npnt[LEFT]->y.
    sar     ebx, 16                             ; count = npnt[LEFT]->y - pnt[RIGHT]->y;
@@:
    mov     ContinueNewSub, ebx                 ; LD_GLINT_FIFO(__GlintTagContinueNewDom, count);   
	jmp     P_NextTrapezoid                     ; Continue with next trapezoid

P_DonePolygon:
    jmp     [FinPoly]

PLABEL P_HWFinishPolygon
    mov     ebx, Rop
    xor     eax, eax
    mov     ScissorMode,2                       ; screen scissor only
    mov     dXDom, eax
    mov     dXSub, eax
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
	cmp	    DIBAccessCallFlag,0	                ;did we call DIB eng to exclude cursor?
	je	    @f       		                    ;nope, skip the following!
	push	[ebp].lpDestDev		
	push	WORD PTR CURSOREXCLUDE		
	call	DIB_EndAccess		                ;Let DIB Engine unexclude cursor.
	mov	DIBAccessCallFlag,0	                    ;clear the flag
@@:
; fall through to
PLABEL P_PolygonFinished
if 0
; The line drawing doesnt work correctly as the spec for a line is different to the spec
; for drawing the outline of a polygon. Thus, although this code works fine, lines are
; drawn in incorrect places - sometimes outside the cliprect.

; Do some checking to see if we need to call the line drawing code. We dont want to bother
; calling line code if there is no pen or the pen style is LS_NOLINE
	cmp     [ebp].lpPPen,0
	je      PolygonFinished
	lds     si,[ebp].lpPPen
	mov     ax,ds:[si].dpPenStyle
	cmp     ax,LS_NOLINE
	je      PolygonFinished

; Draw border. For some strange reason we are asked sometimes to draw beyond the bottom line
; of the bitmap. The lines code cannot handle this, so set up a cliprect if we havnt already got one.
    push    DWORD PTR [ebp].lpDestDev
    push     WORD PTR OS_POLYLINE
    push     WORD PTR [ebp].wCount
    push    DWORD PTR [ebp].lpPoints
    push    DWORD PTR [ebp].lpPPen
    push    DWORD PTR [ebp].lpPBrush
    push    DWORD PTR [ebp].lpDrawMode
	mov		eax, [ebp].lpClipRect
	test	eax, eax
	jne		@f
; Fix up the cliprect
	lds		si, [ebp].lpDestDev
	mov		bx, ds:[si].deWidth
	mov		dwTop, 0							; left and top coord
	mov		cx, ds:[si].deHeight
	shl		ecx, 16
	and		ebx, 0ffffh
	or		ecx, ebx
	mov		dwLeft, ecx							; right, bottom
	mov		eax, ss
	shl		eax, 16
	lea		ax, dwTop							; Form address of new structure.
@@:
    push    eax
    call    GlintPolyLine

PolygonFinished:
endif
    mov     ax,1
	jmp     @f
PolygonError:
    xor     ax,ax
@@:
	pop		edi					
	pop		esi					
	pop		ds
	mov		esp, ebp					        ; Then do the "leave" by hand.
	mov		eax, 1
	pop		ebp
	retf	sizeof GlintPolygonParms - 12		; not the 8 bytes of return addr and 4 of ebp

PLABEL P_SpanTrapezoid
	mov     esi,pntL
	mov     edi,pntR
	mov     eax,es:[esi]                        ; eax = L: (y<<16) + x 
	mov     ebx,es:[edi]                        ; ebx = R: (y<<16) + x
    shl     eax, 16                             ; eax is left
    mov     edx, ebx                            ; edx is Y
    shl     ebx, 16                             ; ebx is right
    or      eax, 07fffh                         ; Overall coordinate bias
    sar     edx, 16                             ; Convert Y to normal format
    or      ebx, 07fffh
; edx = Y, eax = Left(16.16), ebx = Right(16.16)

	mov     esi,npntL
	mov     edi,npntR
	mov     ecx,es:[esi]                        ; Assume npnt[LEFT]->y < npnt[RIGHT]->y
	mov     esi,es:[edi]
	cmp     ecx, esi
	jl      @f
    mov     ecx, esi                            ; npnt[LEFT]->y > npnt[RIGHT]->y
@@:
    sar     ecx, 16                             ; Convert back to normal format.
    sub     ecx, edx                            ; subtract top Y from next Y to get count
    mov     SpanToDoCount, ecx

P_TraverseTrapezoid:
    mov     edi, SpanPtr
    mov     Swapped, 0
P_NewSpan:  
    cmp     edx, dwBottom                       ; Are we beyond the bottom of the cliprect
    jl      @f                 
    jmp     P_SpanFinishPolygon
@@:
    cmp     eax, ebx
    jle     @f
    ; Span has just reversed direction.
    mov     esi, eax
    mov     ecx, dxdyL
    mov     eax, ebx
    xor     Swapped, 1
    mov     ebx, esi
    mov     esi, dxdyR
    mov     dxdyR, ecx
    mov     dxdyL, esi
@@:
    cmp     edx, dwTop                          ; Are we before the start?
    jl      @f
    cmp     eax, dwRight                        ; Are we all to the right?
    jge     @f
    cmp     ebx, dwLeft                         ; Or all to the left?
    jge     P_ContinueSpan                         
@@:                                             ; Just advance through this span
    inc     edx
    add     eax, dxdyL                          ; Advance span endpoints
    add     ebx, dxdyR
    jmp     P_NextSpan
P_ContinueSpan:
    mov     esi, edx
    shl     esi, 16
    inc     edx
    or      esi, 2                              ; Or in scnPntCnt
    mov     ecx, eax
    mov     DWORD PTR [edi], esi                ; Store scnPntCnt and scnPntTop
    cmp     ecx, dwLeft
    jge     @f
    mov     ecx, dwLeft                         ; Span starts left of clip region
@@:
    and     ecx, 0ffff0000h                     ; get scnPntX[0]
    mov     esi, ebx                            ; get scnPntX[1]
    cmp     ebx, dwRight
    jle     @f
    mov     esi, dwRight                        ; Span ends right of clip region
@@:
    and     esi, 0ffff0000h
    cmp     ecx, esi                            ; skip span if first point = last point
    je      @f
    or      ecx, edx
    shr     esi, 16
    mov     DWORD PTR [edi+4], ecx              ; Store scnPntBottom and scnPntX[0]
    or      esi, 020000h                        ; Or in scnPntCntToo
    mov     DWORD PTR [edi+8], esi              ; Store scnPntX[1] and scnPntCntToo
    add     edi, 12                             ; Advance in span space
    inc     SpanCount                           ; One more span drawn
@@:
    add     eax, dxdyL                          ; Advance span endpoints
    add     ebx, dxdyR
    cmp     edi, OFFSET Temp1KSpace1000         ; Have we got close to the end of the 1K space?
    jle     P_NextSpan
    call    P_FlushSpans
P_NextSpan:
    dec     SpanToDoCount                       ; One less!
    jne     P_NewSpan
; Should have done the whole trapezoid now. Save away state ready for continues
    cmp     Swapped, 0
    je      @f
    ; Swap spans back to the correct direction.
    mov     esi, eax
    mov     ecx, dxdyL
    mov     eax, ebx
    neg     Swapped
    mov     ebx, esi
    mov     esi, dxdyR
    mov     dxdyR, ecx
    mov     dxdyL, esi
@@:
    mov     SpanPtr, edi
    mov     LastL, eax
    mov     LastR, ebx
    jmp     P_NextTrapezoid

    
PLABEL P_FlushSpans
    cmp     SpanCount, 0
    je      @f
    pushad
    push    es
    mov     eax, SpanCount
    mov     ebx, ds
    shl     ebx, 16
    or      eax, OS_POLYSCANLINE SHL 16
    or      ebx, OFFSET Temp1KSpace
    push    DWORD PTR [ebp].lpDestDev
    push    eax
    push    ebx
    push    DWORD PTR 0
    push    DWORD PTR [ebp].lpPBrush
    push    DWORD PTR [ebp].lpDrawMode
    push    DWORD PTR 0
    call    GlintPolyScanLine
    pop     es
    popad
    mov     edi, OFFSET Temp1KSpace
    mov     SpanCount, 0
@@:
    retn

PLABEL      P_SpanContLeft
	mov     edi,pntR
	mov     ebx,es:[edi]                        ; ebx = R: (y<<16) + x
    mov     edx, ebx                            ; edx is Y
    shl     ebx, 16                             ; ebx is right
    sar     edx, 16                             ; Convert Y to normal format
    or      ebx, 07fffh

	mov     esi,npntL
	mov     edi,npntR
	mov     ecx,es:[esi]                        ; Assume npnt[LEFT]->y < npnt[RIGHT]->y
	mov     esi,es:[edi]
	cmp     ecx, esi
	jl      @f
    mov     ecx, esi                            ; npnt[LEFT]->y > npnt[RIGHT]->y
@@:
    sar     ecx, 16                             ; Convert back to normal format.
    sub     ecx, edx                            ; subtract top Y from next Y to get count
    mov     eax, LastL                          ; retrieve last left point.
    mov     SpanToDoCount, ecx
; edx = Y, eax = Left(16.16), ebx = Right(16.16)
    jmp     P_TraverseTrapezoid

PLABEL      P_SpanContRight
	mov     edi,pntL
	mov     eax,es:[edi]                        ; eax = L: (y<<16) + x
    mov     edx, eax                            ; edx is Y
    shl     eax, 16                             ; eax is left
    sar     edx, 16                             ; Convert Y to normal format
    or      eax, 07fffh

	mov     esi,npntL
	mov     edi,npntR
	mov     ecx,es:[esi]                        ; Assume npnt[LEFT]->y < npnt[RIGHT]->y
	mov     esi,es:[edi]
	cmp     ecx, esi
	jl      @f
    mov     ecx, esi                            ; npnt[LEFT]->y > npnt[RIGHT]->y
@@:
    sar     ecx, 16                             ; Convert back to normal format.
    sub     ecx, edx                            ; subtract top Y from next Y to get count
    mov     ebx, LastR                          ; retrieve last right point.
    mov     SpanToDoCount, ecx
; edx = Y, eax = Left(16.16), ebx = Right(16.16)
    jmp     P_TraverseTrapezoid

PLABEL      P_SpanFinishPolygon
    call    P_FlushSpans
	jmp     P_PolygonFinished                   ; Continue with next trapezoid

PLABEL RemoveSWCursor
;
;       Find visible rectangle if not clipped
;
    xor     esi, esi
    xor     eax, eax
    lfs     si, [ebp].lpDestDev
    mov     eax, DWORD PTR fs:[esi].deFlags
    test    eax, OFFSCREEN
    jne     SWCursorRemoved
    test    eax, VRAM
    je      SWCursorRemoved

    cmp     [ebp].lpClipRect,0                      ;clipped?
    je      PY_NoClip                               ;j if so

    lfs     si,[ebp].lpClipRect                     ;pt to rect
	push    [ebp].lpDestDev                         ;PDevice
	push    fs:[si].left                            ;Left
	push    fs:[si].top                             ;Top
	push    fs:[si].right                           ;Right
	push    fs:[si].bottom                          ;Bottom
PY_CallBeginAccess:
    push    WORD PTR CURSOREXCLUDE                  ;Flags
	call	BeginAccess 	                        ;returns with flags in ax.
	mov	    DIBAccessCallFlag, 'P'      	        ;set the DIBAccessCallFlag
    jmp     SWCursorRemoved

PY_NoClip:
    lfs     si,[ebp].lpPoints
    mov     ecx,DWORD PTR [ebp].wCount              ;all points
    xor     eax, eax
    mov     dwRight,eax                             ;set large region
    mov     dwBottom,eax
    and     ecx, 0ffffh
    dec     eax
    mov     dwLeft,eax
    mov     dwTop,eax
PY_FindMinMax:
    mov     eax,fs:[esi]                            ;x + (y << 16)
    mov     ebx, eax
    and     eax, 0ffffh                             ; x
    sar     ebx, 16                                 ; y
    add     si,(size PTTYPE)
    cmp     eax,dwRight
    jbe     @f                                      ;j if < right
    mov     dwRight,eax                              ;set new right
@@:
    cmp     eax,dwLeft
    jae     @f                                      ;j if >= left
    mov     dwLeft,eax
@@:
    cmp     ebx,dwBottom
    jbe     @f                                      ;j if < bottom
    mov     dwBottom,ebx                             ;set new bottom
@@:
    cmp     ebx,dwTop
    jae     @f                                      ;j if >= Top
    mov     dwTop,ebx
@@:
    dec     ecx
    jnz     PY_FindMinMax

	push    [ebp].lpDestDev                         ;PDevice
    push    WORD PTR dwLeft
    push    WORD PTR dwTop
    push    WORD PTR dwRight
    push    WORD PTR dwBottom
    jmp     PY_CallBeginAccess
GlintPolygon32    endp

_OUTPUT32   ends
end
