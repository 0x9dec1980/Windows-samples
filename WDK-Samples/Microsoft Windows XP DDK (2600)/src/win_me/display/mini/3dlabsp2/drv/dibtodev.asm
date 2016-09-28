;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: dibtodev.ASM
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

    .xlist
DOS5 = 1			;so we don't get INC BP in <cBegin>
incDrawMode = 1
CODE32BPP = 1

; if both of these flags are set then we can do the accelerated mono blit else not
MONO_PATTERN= PATTERNMONO		;flag to say that mono part came from mono source data
MONO_VALID	= MONOVALID			;flag to say that a brush's mono part is valid

    include glint.inc
    include gdidefs.inc
	.list
;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------
BS_SOLID        equ     0
BS_HOLLOW       equ     1
BS_HATCHED      equ     2
BS_PATTERN      equ     3

GLINTDEPTH8		equ		0
GLINTDEPTH16	equ		1
GLINTDEPTH32	equ		2

DD_UNDEFINED_OFFSCREEN  equ 0   ; Value which signifies GLInfo.dwCurrentOffscreen is undefined.

CLIPSRCTODST macro
	call	ClipSrcToDest           ;Clip Source
    jc      BS_Done                 ;exit if null blt.
endm

EXCLUDESWCURSOR macro
local NoSWCursor
ifdef CODE32BPP
	mov		eax,GLInfo.dwCursorType	;running with a software cursor?
	test	eax,0ffffh
else
	cmp		wCursorType,0			;running with a software cursor?
endif
	jne		NoSWCursor         		;nope, no need to exclude hdw cursor
    call    D2D_CursorExclude
NoSWCursor:
endm


.586


_INIT   segment

PUBLIC  BiCompJumpTable
BiCompJumpTable dd      D2D_RGB             ; BI_RGB      
                dd      D2D_RLE8            ; BI_RLE8     
                dd      D2D_RLE4            ; BI_RLE4     
                dd      D2D_BitFields       ; BI_BITFIELDS

BI_COMPRESSION_MAX  equ BI_BITFIELDS

_INIT   ends

;----------------------------------------------------------------------------
;			       GLINT
;----------------------------------------------------------------------------

_BLIT32	segment page


	BEST_ALIGN

PLABEL Jmp_DIB_DIBToDevice
DestinationIsMemory:
    pop     edi
    pop     esi
    pop     ds
    mov     sp,bp					; transfer only the bottom 16 bits
    pop     ebp

	StackMangle32To16				; convert the 32:32 ret addr back to 16:16
	jmp	    DIB_DibToDevice

;----------------------------------------------------------------------------
; BltSpecial_GLINT
;----------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  DIBtoDev16
	StackMangle16To32
DIBtoDevParms	STRUCT
	_ebp		    DWORD	0
	_retLo		    DWORD	0
	_retHi		    DWORD	0
    
	lpTranslate	    DWORD	0
	lpBitmapInfo    DWORD	0
	lpDIBits	    DWORD	0
	lpDrawMode	    DWORD	0
	lpClipRect	    DWORD	0
	cScans		    WORD	0
	iScan   	    WORD	0
	DestYOrg	    WORD	0
	DestXOrg	    WORD	0
	lpDestDev	    DWORD	0
DIBtoDevParms	ENDS

; Temporarily reuse these locals at start of day
ClipMinXY   equ DestX
ClipMaxXY   equ DestY

DIBtoDev32	PROC FAR PASCAL PUBLIC		;remember that USES would override "ret" code...
	LOCAL	DestX:DWORD,
            DestY:DWORD,
            DestLeft:DWORD,
            DestRight:DWORD,
            DestTop:DWORD,
            DestBottom:DWORD,
            realDestLeft:DWORD,
            dwDelta:DWORD,
            SubFn1:DWORD,
            SubFn2:DWORD

	and		ebp,0ffffh
	push	ds
	push	esi					;save 32 bit versions of these
	push	edi					;

    mov     eax,_INIT
    mov     ds, eax
	assumes	ds,_INIT
	mov	    edx,RemappedDataSegment         ;Load Data segment for later access
    assumes ds,nothing		
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    xor     esi, esi
    xor     edi, edi
	lds		si, [ebp].lpDestDev
    mov     ClipMinXY, edi                  ;zero minimum clip
    mov     ebx, DWORD PTR ds:[esi].deWidth ;get Width | Height << 16
    mov     eax, dword ptr ds:[esi].deFlags ;is destination the screen?
    mov     ClipMaxXY, ebx                  ;Set Max clip

	cmp	    ds:[esi].deType,TYPE_DIBENG     ;is this a DIBENGINE surface?
    jne     Jmp_DIB_DIBToDevice

    mov     ecx,dword ptr ds:[esi].deDriverReserved  ; Get Minidriver flags
    test    eax,BUSY                        ;disabled ?
    jnz     Jmp_DIB_DIBToDevice             ;if so, punt

	push	edx                             ;Push DataSegment onto stack

    ; Default clip is to the bitmap. Override with specified clip
    cmp     [ebp].lpClipRect, 0
    je      @f
    lds     si, [ebp].lpClipRect
    mov     ebx, ds:[esi+0]                 ; Top Left Clip
    mov     edx, ds:[esi+4]                 ; Bottom Right Clip
    mov     ClipMinXY, ebx
    mov     ClipMaxXY, edx
@@:
	pop		ds                              ;get Data Segment
    assumes ds,_INIT

    test    eax,VRAM                        ;is destination the screen?
    je      DestinationIsMemory		        
    test    ecx,ACCELERATED                 ;is destination accelerated?
    je      DestinationIsMemory

    lfs     di, [ebp].lpBitmapInfo

; Work out rectangle of interest in destination. We start with Bitmap Width 
; and at line iScan for cScan lines,then we apply the clipping region 
    mov     ecx, ClipMinXY
    xor     esi, esi
    movsx   ebx, [ebp].DestYOrg
    mov     edx, ecx                   
    movsx   eax, [ebp].DestXOrg              ; Left
    shr     edx, 16                         ; Top of clip
    mov     si, [ebp].iScan
    add     ebx, esi                        ; right
    and     ecx, 0ffffh                     ; Left of clip
    cmp     ebx, edx
    jge     @f
    mov     ebx, edx
@@:
    cmp     eax, ecx
    jge     @f
    mov     eax, ecx
@@:
    mov     DestLeft, eax
    mov     DestTop, ebx              
    mov     realDestLeft, eax

    mov     ecx, ClipMaxXY
    movsx   ebx, [ebp].DestYOrg
    mov     edx, ecx                   
    movsx   eax, [ebp].DestXOrg
    shr     edx, 16                         ; Bottom of clip
    mov     DestX, eax                      ; Start at left
    mov     si, [ebp].iScan
    add     ebx, esi
    and     ecx, 0ffffh                     ; Right of clip
    mov     si, [ebp].cScans
    add     ebx, esi                        ; bottom
    add     eax, fs:[edi].biWidth           ; right
    mov     DestY, ebx                      ; Start at bottom of rectangle.
    cmp     ebx, edx
    jle     @f
    mov     ebx, edx
@@:
    cmp     eax, ecx
    jle     @f
    mov     eax, ecx
@@:
    mov     DestRight, eax
    mov     DestBottom, ebx
    dec     DestY                           ; Bottom line not drawn. Advance to first drawn line

    cmp     DestLeft, eax                   ; Check for complete exclusion
    jge     D2D_Done     
    cmp     DestTop, ebx
    jge     D2D_Done

; DestLeft, DestRight, DestTop, DestBottom now contain dest region of interest
; DestX, DestY contain current destination position (top left).

    les     si, [ebp].lpDIBits

    mov     eax, fs:[edi].biCompression
    cmp     eax, BI_COMPRESSION_MAX         ; Max value we recognise
    ja      Jmp_DIB_DIBToDevice             ; Pass to DIB if it exceeds this.

    lea     ebx, [eax*4+BiCompJumpTable]
    jmp     [ebx]

	BEST_ALIGN
PLABEL D2D_RGB
    ; This is a download with colour conversion. As a first pass we just download the
    ; data with translation. Later we need to ensure packed downloads
    cmp     fs:[edi].biBitCount, 8
    je      @f
    ;int 1
    jmp     Jmp_DIB_DIBToDevice
@@:
    mov     eax, DestBottom
    sub     eax, DestTop
    dec     eax
    jne     Jmp_DIB_DIBToDevice

    mov     eax, fs:[edi].biWidth
    add     eax, 3                          ; round up
    and     eax, 0fffffffch
    mov     dwDelta, eax

    ; advance edi to colour information in BITMAPINFO
    mov     ecx, fs:[edi].biClrUsed         ; Number of colours to translate
    test    ecx, ecx
    jne     @f
    mov     cx, fs:[edi].biBitCount         ; If 0, then all are important.
    mov     eax, 1
    shl     eax, cl
    mov     ecx, eax
@@:
    add     edi, bmiColors
    call    D2D_Generate256ColorLUT
    
    EXCLUDESWCURSOR
	DISPLAY_CONTEXT
    DISABLE_FIFO_CHECKING

	; Inform other display driver routines that registers will change.
	;
	mov     DD_CurrentContext, DD_UNDEFINED_CTXT

	; Setup Permedia registers
	;
    mov     LogicalOpMode,0             ; Don't use constantFBWriteData.
    mov     dY, 0ffff0000h              ; Draw from top down
	mov     eax, [ebp].lpDestDev
	cmp     GLInfo.dwCurrentOffscreen, eax
	je      @f
	xor		ebx,ebx
	lfs     bx, [ebp].lpDestDev
	mov     GLInfo.dwCurrentOffscreen, eax
	mov     edx, fs:[ebx].OSB_PixelOffset
	mov     FBWindowBase, edx
@@:
    mov     eax, DestLeft
    mov     ecx, DestBottom
    mov     ebx, DestRight
    mov     edx, ecx
	shl		eax,16					    ; Convert left dest x-coord to Fixed point.
    sub     edx, DestTop                ; count is Bottom - Top.
	shl		ebx,16					    ; Convert right dest x-coord to Fixed point.
    dec     ecx                         ; Dont want to render on the bottom pixel
    shl     ecx, 16
	mov		StartXDom,eax
	mov		StartXSub,ebx
	mov		StartY,ecx
    mov     Count,edx
	mov		Render,1040h			    ; Start rendering. Trapezoid, SyncOnHostData

    mov     eax, DestY
    mov     ebx, DestLeft
    sub     eax, DestBottom
    mov     edx, dwDelta
    inc     eax                         ; dest bottom is exclusive.
    sub     ebx, DestX
    mul     edx
    add     eax, ebx                    ; Work out pixel offset into bitmap
    add     esi, eax                    ; point to first pixel in bitmap

    mov     ecx, DestBottom
    sub     ecx, DestTop

D2DRGB8_NextScanline:
    mov     eax, DestRight
    lea     edi, GPFifo5
    sub     eax, DestLeft
    push    ecx
    mov     ecx, eax
    shl     eax, 16
    push    esi
	sub		eax,0ff02h				        ; Dec hold count & add color tag (0xfe)
    jl      D2DRGB8_Done                    ; exit if zero words
    mov     GPFifo4, eax
    cmp     ecx, 4
    jl      D2DRGB8_LessThan4DWORDS
    push    ebp
    jmp     D2DRGB8_MoreThan4DWORDS

BEST_ALIGN
D2DRGB8_MoreThan4DWORDS:
    push    ecx
    lea     ecx, Temp1KSpace

    mov     eax, es:[esi]
    mov     ebp, eax
    xor     ebx, ebx
    mov     bl, ah
    and     ebp, 0ffh
    shr     eax, 16
    xor     edx, edx
    mov     ebp, ds:[ecx + 4*ebp]          ; Convert to current format
    mov     dl, ah
    and     eax, 0ffh
    mov     ebx, ds:[ecx + 4*ebx]          ; Convert to current format
    mov     eax, ds:[ecx + 4*eax]          ; Convert to current format
    mov     edx, ds:[ecx + 4*edx]          ; Convert to current format
    pop     ecx
    mov     [edi +  0], ebp
    mov     [edi +  4], ebx
    mov     [edi +  8], eax
    mov     [edi + 12], edx
    add     esi, 4
    add     edi, 16
    sub     ecx, 4
    and     edi, 0ffffefffh
    cmp     ecx, 4
    jae     D2DRGB8_MoreThan4DWORDS
    pop     ebp
D2DRGB8_LessThan4DWORDS:
    lea     edx, Temp1KSpace
    cmp     ecx, 2
    jl      D2DRGB8_LessThan2DWORDS
    xor     eax, eax
    mov     al, es:[esi +  0]
    xor     ebx, ebx
    mov     bl, es:[esi +  1]
    mov     eax, ds:[edx + 4*eax]          ; Convert to current format
    mov     ebx, ds:[edx + 4*ebx]          ; Convert to current format
    mov     [edi +  0], eax
    mov     [edi +  4], ebx
    add     esi, 2
    add     edi, 8
    sub     ecx, 2
D2DRGB8_LessThan2DWORDS:
    cmp     ecx, 1
    jl      D2DRGB8_ScanlineDone
    xor     eax, eax
    mov     al, es:[esi +  0]
    mov     eax, ds:[edx + 4*eax]          ; Convert to current format
    mov     [edi +  0], eax
D2DRGB8_ScanlineDone:
    pop     esi
    pop     ecx
    add     esi, dwDelta                    ; advance to next scanline
    dec     ecx
    jne     D2DRGB8_NextScanline

D2DRGB8_Done:
    mov     LogicalOpMode, 20h
    mov     dY, 10000h
    jmp     D2D_UnexcludeCursor

	BEST_ALIGN
PLABEL D2D_RLE4
    jmp     Jmp_DIB_DIBToDevice
	BEST_ALIGN
PLABEL D2D_BitFields
    jmp     Jmp_DIB_DIBToDevice

	BEST_ALIGN
PLABEL D2D_RLE8

    ; advance edi to colour information in BITMAPINFO
    mov     ecx, fs:[edi].biClrUsed         ; Number of colours to translate
    test    ecx, ecx
    jne     @f
    mov     cx, fs:[edi].biBitCount         ; If 0, then all are important.
    mov     eax, 1
    shl     eax, cl
    mov     ecx, eax
@@:
    add     edi, bmiColors
    call    D2D_Generate256ColorLUT
    
    EXCLUDESWCURSOR
	DISPLAY_CONTEXT
    DISABLE_FIFO_CHECKING

	; Inform other display driver routines that registers will change.
	;
	mov     DD_CurrentContext, DD_UNDEFINED_CTXT

	; Setup Permedia registers
	;
    mov     LogicalOpMode,0             ; Don't use constantFBWriteData.
    mov     dY, 0ffff0000h              ; Draw from top down
	mov     eax, [ebp].lpDestDev
	cmp     GLInfo.dwCurrentOffscreen, eax
	je      @f
	xor		ebx,ebx
	lfs     bx, [ebp].lpDestDev
	mov     GLInfo.dwCurrentOffscreen, eax
	mov     edx, fs:[ebx].OSB_PixelOffset
	mov     FBWindowBase, edx
@@:
; We could improve speed here by using packed transfers. However, this routine
; doesnt get called in WinBench - so there is no point! Its entire reason for
; existance is to bypass a bug in the DIBEngine.
    mov     eax, DestLeft
    mov     ecx, DestBottom
    mov     ebx, DestRight
    mov     edx, ecx
	shl		eax,16					    ; Convert left dest x-coord to Fixed point.
    sub     edx, DestTop                ; count is Bottom - Top.
	shl		ebx,16					    ; Convert right dest x-coord to Fixed point.
    dec     ecx                         ; Dont want to render on the bottom pixel
    shl     ecx, 16
	mov		StartXDom,eax
	mov		StartXSub,ebx
	mov		StartY,ecx
    mov     Count,edx
	mov		Render,1040h			    ; Start rendering. Trapezoid, SyncOnHostData
    jmp     D2DRLE8_GetNextDataItem

D2DRLE8_RestartDownload:
; We are asked to restart the download command. This time we know Count is in Excess and
;  StartXSub is valid, so less to download and work out.
; 
    mov     eax, DestLeft
    mov     ecx, DestBottom
	shl		eax,16					    ; Convert left dest x-coord to Fixed point.
    dec     ecx                         ; Dont want to render on the bottom pixel
    shl     ecx, 16
	mov		StartXDom,eax
	mov		StartY,ecx
	mov		Render,1040h			    ; Start rendering. Trapezoid, SyncOnHostData
    jmp     D2DRLE8_GetNextDataItem

; Decode the data in es:esi and send to the chip

BEST_ALIGN
D2DRLE8_GetNextDataItem:
    xor     eax, eax
    mov     ax, es:[esi]                ; Get first word of input
    add     esi, 2                      ; advance to next word
    test    al, al                      ; al != 0 -> Encoded mode
    jne     D2DRLE8_EncodedMode
    cmp     ah, 3                       ; ah >= 3 -> Absolute Mode
    jae     D2DRLE8_AbsoluteMode
    cmp     ah, 0                       ; End of Line
    je      D2DRLE8_EndOfLine
    cmp     ah, 1                       ; End of Bitmap
    je      D2DRLE8_Done
    jmp     D2DRLE8_Delta

BEST_ALIGN
D2DRLE8_EncodedMode:
; al is number of items of colour index ah
    xor     ecx, ecx
    mov     cl, al                      ; get number of data items
    shr     eax, 8                      ; get index
    call    D2DRLE8_DownloadIndex
    jmp     D2DRLE8_GetNextDataItem
    
BEST_ALIGN
D2DRLE8_AbsoluteMode:
; ah is number of data items. This will be padded to a WORD aligment in memory
    xor     ecx, ecx
    mov     cl, ah                      ; get number of data items
    mov     edi, ecx                    ; edi is our count register
@@:
    mov     ecx, 1                      ; Count of number to download
    xor     eax, eax
    mov     al, es:[esi]                ; Index to download
    inc     esi                         ; advance to next pixel
    call    D2DRLE8_DownloadIndex
    dec     edi                         ; Done all the bytes?
    jne     @b

    inc     esi
    and     esi, 0fffffffeh             ; round up esi to next word
    jmp     D2DRLE8_GetNextDataItem

BEST_ALIGN
D2DRLE8_DownloadIndex:
; This subroutine downloads the colour for the index in eax, ecx times.
; It advances DestX
    mov     ebx, DestY
    cmp     ebx, DestBottom             ; Are we below the cliprect?
    jge     D2DRLE8_OutsideClip
    mov     ebx, DestX
    cmp     ebx, DestRight
    jge     D2DRLE8_OutsideClip
    lea     edx, [ebx + ecx]
    cmp     edx, DestLeft
    jle     D2DRLE8_OutsideClip
    mov     DestX, edx                  ; Advance X
    cmp     ebx, DestLeft
    jge     @f
; Clipped to the left. Reduce Count
    mov     ebx, DestLeft               ; new StartDestX. Needed if we cut right clip.
    mov     ecx, edx
    sub     ecx, ebx                    ; Count = EndDestX - DestLeft
@@:
    cmp     edx, DestRight
    jle     @f
; Clipped to the right. Reduce count
    mov     ecx, DestRight
    sub     ecx, ebx                    ; Count = DestRight - StartDestX
@@:
    lea     ebx, Temp1KSpace
    mov     eax, [ebx + 4*eax]          ; Convert to current format
    dec     ecx                         ; Use colour message if just one to download
    jne     @f
    mov     Color, eax
    retn
@@:                                     ; Use Hold tag if more than one to download
    lea     edx, GPFifo2
    shl     ecx, 16                     ; turn into hold tag
    or      ecx, 0feh                   ; or in the count
    mov     GPFifo1, ecx                ; download hold tag
@@:
    mov     [edx], eax                  ; download data
    add     edx, 4                      ; increase pointer
    sub     ecx, 10000h
    jae     @b
    retn

D2DRLE8_OutsideClip:
    add     DestX, ecx                  ; Advance X and return. 
    retn
    
BEST_ALIGN
D2DRLE8_EndOfLine:
    mov     eax, DestY
    mov     edx, DestX                  ; need this to compare against later
    dec     eax
    cmp     eax, DestTop                ; Have we advanced past the top of clip?
    jl      D2DRLE8_Done                ; If so, we have finished

    mov     ecx, DestRight
    movsx   ebx, [ebp].DestXOrg
    mov     DestY, eax                  ; eax reused on a restart
    mov     DestX, ebx

    cmp     eax, DestBottom             ; Have we reached clip area yet?
    jge     D2DRLE8_GetNextDataItem     ; if not, go get new data

    mov     ebx, realDestLeft
    inc     eax                         ; DestBottom is exclusive
    cmp     ebx, DestLeft
    je      @f
; We restarted for part of a scanline. Reset DestLeft and restart
    mov     DestLeft, ebx
    mov     DestBottom, eax
    jmp     D2DRLE8_RestartDownload       ; start new primitive
@@:

    sub     ecx, edx                    ; Do we have more pixels to download
    jg      @f                          ; on last scanline
    jmp     D2DRLE8_GetNextDataItem
@@:                                     ; Need to abort and restart on next line.
    mov     DestBottom, eax             ; reset bottom of image
    jmp     D2DRLE8_RestartDownload       ; start new primitive

BEST_ALIGN
D2DRLE8_Delta:
; We need to abort the current download and restart it with a new left position.
; This is done by changine DestLeft and restarting the download
; However, we need to restart again when we reach the end of that scanline with
; the old value of DestLeft.
    xor     eax, eax
    mov     ax, es:[esi]                ; Get the delta
    add     esi, 2
    xor     ebx, ebx
    mov     bl, ah                      ; Y Offset
    and     eax, 0ffh                   ; X Offset

    mov     ecx, DestX
    lea     eax, [ecx + eax]            ; New X Position
    mov     DestX, eax                  ; Advance in X

    test    ebx, ebx
    jne     @f
; Y delta is zero -> Check to see if delta interrupts the current download area
; If it doesnt, we dont have to restart download
    cmp     eax, DestLeft               ; Is end to the left of clip?
    jle     D2DRLE8_GetNextDataItem
    cmp     ecx, DestRight              ; Or the start to the right?
    jge     D2DRLE8_GetNextDataItem
@@:

    mov     ecx, DestY
    sub     ecx, ebx                    ; Advance Y
    mov     DestY, ecx

    cmp     ecx, DestBottom             ; Have we reached clip region yet?
    jge     D2DRLE8_GetNextDataItem     ; If not, just do get more data.

    inc     ecx                         ; DestY is inclusive, DestBottom is exclusive
    mov     DestBottom, ecx             ; We can change the bottom value!
    cmp     eax, DestRight              ; if DestX is right of DestRight, then
    jl      @f                          ; advance top scanline and just restart
    dec     ecx
    cmp     ecx, DestTop                ; Have we advanced past the top of clip?
    jle     D2DRLE8_Done                ; If so, we have finished
    mov     DestBottom, ecx             ; We can change the top value!
    jmp     D2DRLE8_RestartDownload
@@:
    cmp     ecx, DestTop                ; Have we advanced past the top of clip?
    jle     D2DRLE8_Done                ; If so, we have finished
; Need to restart at DestX, DestY for one scanline.
    mov     ecx, DestLeft
    cmp     eax, ecx                    ; Is DestX left of DestLeft?
    jle     @f                          ; if so, we dont have to restart single line
    mov     DestLeft, eax               ; Update new temporary DestLeft
@@:
    jmp     D2DRLE8_RestartDownload
    
BEST_ALIGN
D2DRLE8_Done:
    mov     LogicalOpMode, 20h
    mov     dY, 10000h
    jmp     D2D_UnexcludeCursor

BEST_ALIGN
PLABEL D2D_UnexcludeCursor
	mov		ebx, GLInfo.dwCursorType        ; running with a software cursor?
	mov		eax, 1
	test	ebx,ebx
	je		D2D_ReplaceSWCursor		        ; nope, no need to worry about hw cursor
PLABEL D2D_Done
    mov     ax, [ebp].cScans                ; return value - number of scans copied.
    xor     edx, edx
	pop		edi					
	pop		esi					
	pop		ds
	mov		esp, ebp					    ; Then do the "leave" by hand.
	pop		ebp
	retf	sizeof DIBtoDevParms - 12	    ; not the 8 bytes of return addr and 4 of ebp

	BEST_ALIGN
D2D_ReplaceSWCursor:
;
;We need to unset the BUSY bit in the destination PDevice structure which
;we previously set in order to prevent the DIB engine from asynchonously
;drawing the cursor
;
;	xor		ebx,ebx							;ebx has just been shown to be 0!!
	lfs		bx, GLInfo.lpDriverPDevice				
	mov		al, DIBAccessCallFlag		
	and		fs:[ebx].deFlags,NOT BUSY
;
;When we get to this point, we've finished doing a BitBLT which used hardware
;acceleration.	If we previously called the DIB engine to exclude a software
;cursor owned by it, we must call the DIB engine to unexclude its software
;cursor.
;
	test	al,al						    ;did we call DIB eng to exclude cursor?
	je		D2D_Done 					    ;nope, skip the following!
	push	GLInfo.lpDriverPDevice			
	push	word ptr CURSOREXCLUDE		    
	call	DIB_EndAccess				    ;Let DIB Engine unexclude cursor.
	mov		DIBAccessCallFlag,0			    ;clear the flag
	jmp		D2D_Done


; small subroutine to call the cursor exclude routine with the
; right parameters.
BEST_ALIGN
PLABEL D2D_CursorExclude
    mov     eax, DestLeft
    mov     ebx, DestTop
    mov     ecx, DestRight
    mov     edx, DestBottom
    sub     ecx, eax
    sub     edx, ebx
	invoke	CursorExclude, eax, ebx, ecx, edx
    ; reload these values as they may have been corrupted by call.
    les     si, [ebp].lpDIBits
    lfs     di, [ebp].lpBitmapInfo
    retn


; fs:edi points to 24bpp colour table
; ecx contains number of colours to convert
PLABEL D2D_Generate256ColorLUT
    lea     edx, Temp1KSpace
    cmp     GLInfo.dwBpp, 16
    ja      D2D_CCT_Copy
    je      D2D_CCT_24to16
D2D_CCT_24to8:
; Hopefully the OS has done the work and the lpTranslate is the table we require.
; Copy it to data segment translating into DWORDS
    cmp     [ebp].lpTranslate, 0
    jne     D2D_CCT_24to8_WithPalette
; Assume identity palette
    mov     eax, 0ffh
@@:
    mov     [edx+eax*4], eax
    dec     eax
    jne     @b
    retn
D2D_CCT_24to8_WithPalette:
    push    edi
    lgs     di, [ebp].lpTranslate
@@:
    mov     eax, gs:[edi]
    mov     ebx, gs:[edi+2]
    mov     [edx], eax
    mov     [edx+4], ebx
    add     edi, 4
    add     edx, 8
    sub     ecx, 2
    jg      @b
    pop     edi
    retn

; 2 colours simultaneously on a PentiumII will run at almost twice the
; speed of 1 colour at a time. On a Pentium it doesnt make any difference.
D2D_CCT_24to16:
    mov     eax, fs:[edi]
    mov     ebx, fs:[edi+4]
    sub     al, 4
    jae     @f
    xor     al, al
@@:
    sub     bl, 4
    jae     @f
    xor     bl, bl
@@:
    sub     ah, 4
    jae     @f
    xor     ah, ah
@@:
    sub     bh, 4
    jae     @f
    xor     bh, bh
@@:
    ror     eax, 10h
    ror     ebx, 10h
    sub     al, 4
    jae     @f
    xor     al, al
@@:
    sub     bl, 4
    jae     @f
    xor     bl, bl
@@:
    ror     eax, 10h
    ror     ebx, 10h
    shr     eax, 3
    shr     ebx, 3
    ror     eax, 5
    ror     ebx, 5
    shr     ax, 3
    shr     bx, 3
    ror     eax, 5
    ror     ebx, 5
    shr     ax, 3
    shr     bx, 3
    rol     eax, 10
    rol     ebx, 10
    mov     [edx], eax
    mov     [edx+4], ebx
    add     edi, 8
    add     edx, 8
    sub     ecx, 2
    jg      D2D_CCT_24to16
    retn

D2D_CCT_Copy:
    mov     eax, fs:[edi]
    mov     ebx, fs:[edi+4]
    mov     [edx], eax
    mov     [edx+4], ebx
    add     edi, 8
    add     edx, 8
    sub     ecx, 2
    jg      D2D_CCT_Copy
    retn

BEST_ALIGN
DIBtoDev32	ENDP


_BLIT32   ends
end

