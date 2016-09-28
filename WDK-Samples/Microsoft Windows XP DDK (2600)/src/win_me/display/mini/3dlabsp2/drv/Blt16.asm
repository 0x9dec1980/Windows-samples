;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: BLT16.ASM
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

.xlist
DOS5 = 1
include glint.inc
.list
.586
;----------------------------------------------------------------------------
; MACROS
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT   segment
	assumes cs,_TEXT

PLABEL  BitBlt
    jmp     _BLIT32:BitBlt16
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

PLABEL  DibToDevice
    jmp     _BLIT32:DIBtoDev16
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop



ScanLR PROC FAR PASCAL PUBLIC USES esi edi, lpDevice:DWORD, XPos:WORD, YPos:WORD,
                                    dwPhysColor:DWORD, Style:WORD
    invoke  DIB_ScanLR, lpDevice, XPos, YPos, dwPhysColor, Style

    ret
ScanLR ENDP


DibBlt PROC FAR PASCAL PUBLIC USES ds,
    	lpBitmap:DWORD,	fGet:WORD, iStart:WORD, cScans:WORD, lpDIBits:DWORD,
        lpBitmapInfo:DWORD, lpDrawModeA:DWORD, lpTranslate:DWORD

    ; The DIB engine doesnt seem to call BeginAccess and EndAccess - so Sync the chip here

	and		ebp,0ffffh
    call    FarBeginAccess

    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
    assumes es, nothing
    assumes fs, nothing
    assumes gs, nothing

    push    lpBitmap
    push    fGet
    push    iStart
    push    cScans
    push    lpDIBits
    push    lpBitmapInfo
    push    lpDrawModeA
    push    lpTranslate
    push    wPalettized
	call	DIB_DibBltExt
    ret
DibBlt ENDP


; The DIB engine messes up copies between two Offscreen Bitmaps in its BitmapBits
; function - it assumes as both delta's are the same, it can just copy (delta*height)
; bytes from source to destination!
; In this case, we want to call the BitBlt function, and end up calling our standard
; Copyblt functions

BitmapBits PROC FAR PASCAL PUBLIC,
    	lpDevice:DWORD,	fFlags:DWORD, dwCount:DWORD, lpBits:DWORD
    LOCAL hBitmap:DIBENGINE, bWidth:WORD, bHeight:WORD

    mov     ax, WORD PTR fFlags
    cmp     ax, 4                   ; DBB_COPY
    jne     BitmapBits_SetGet

    xor     eax, eax
    mov     ecx, 00CC0020h          ; SRCCOPY
    les     bx, lpDevice
    push    lpDevice                ; lpDestDev
    push    eax                     ; DestX = DestY = 0
    push    lpBits                  ; lpSrcDev
    push    eax                     ; SrcX = SrcY = 0
    push    es:[bx].deWidth         ; Dest Width
    push    es:[bx].deHeight        ; Dest Height
    push    ecx                     ; ROP
    push    eax                     ; lpBrush
    push    eax                     ; lpDrawMode
    call    BitBlt16                ; Do the blt
    ret

PLABEL BB_SetGet_Breakdown
    jmp     BB_SetGet_DIBEngine             ; or go to DIB
    int     1
    xor     edx, edx
    mov     ax, WORD PTR dwCount            ; Number of bytes to upload
    mov     cx, ds:[bx].deWidth             ; Width per scanline
    mov     dx, WORD PTR dwCount+2          ; Number of bytes to upload
    div     cx
    mov     bHeight, ax                     ; Number of entire scanlines to do.
    mov     eax, dwCount                    ; Number of bytes to upload
    test    dx, dx                          ; Any incomplete?
    je      BB_SetGetBreakdownContinues     ; Continue if not
    jmp     BB_SetGet_DIBEngine             ; or go to DIB
    

PLABEL BitmapBits_SetGet
    cmp     ax, 8                   ; DBB_SETWITHFILLER
    je      BitmapBits_DIBEngine
    push    ds

; Set up a dummy DIB surface that points at the buffer we wish to fill.
    lds     bx, lpDevice;
    xor     eax, eax
    mov     hBitmap.deType, TYPE_DIBENG
    mov     hBitmap.deDriverReserved, eax       ; zero deDriverReserved
    mov     hBitmap.deBeginAccess, eax          ; zero BeginAccess
    mov     hBitmap.deEndAccess, eax            ; zero EndAccess


    mov     dx, WORD PTR ds:[bx].dePlanes
    mov     ax, ds:[bx].deWidth                 ; width in Pixels
    xor     ecx, ecx
    mov     WORD PTR hBitmap.dePlanes, dx
    mov     cl, dh                              ; get BPP
    shr     cx, 3                               ; reduce to 1 (8bpp) 2(16bpp) 3(24bpp) and 4(32bpp)
    mov     hBitmap.deWidth, ax
    mov     bWidth, ax
    mul     cx                                  ; turn width into Bytes
    mov     hBitmap.deWidthBytes, ax
    mov     cx, ds:[bx].deHeight
    mov     bHeight, cx
    add     ax, 3                               ; round up to nearest dword
    mov     hBitmap.deHeight, cx
    and     ax, NOT 3
    mov     hBitmap.deDeltaScan, eax            ; Update the remaining field
	cmp		ax, hBitmap.deWidthBytes
	jnz		BB_SetGet_Breakdown 

    mul     ecx                                 ; Calculate return value
    cmp     eax, dwCount                        ; compare it with buffer size
    ja      BB_SetGet_Breakdown                 ; See if we can break the operation down
BB_SetGetBreakdownContinues:
    push    eax                                 ; save return value.
    mov     WORD PTR hBitmap.deBits+2, 0
    mov     ecx, ds:[bx].delpPDevice
    mov     ax, WORD PTR lpBits
    mov     hBitmap.delpPDevice, ecx
    mov     WORD PTR hBitmap.deBits, ax
    mov     ecx, ds:[bx].deBitmapInfo
    mov     dx, ds:[bx].deFlags
    mov     hBitmap.deBitmapInfo,ecx
    mov     ax, WORD PTR lpBits+2
    and     dx, MINIDRIVER + PALETTIZED
    mov     WORD PTR hBitmap.deVersion, 0400h
    mov     WORD PTR hBitmap.deFlags, dx
    mov     WORD PTR hBitmap.deBits + 4, ax

    lea     cx, hBitmap
    xor     eax, eax

    cmp     WORD PTR fFlags, 1      ; DBB_Set
    jne     BB_Get
    push    lpDevice                ; lpDestDev
    push    eax                     ; DestX = DestY = 0
    push    ss
    push    cx                      ; lpSrcDev (hBitmap)
    jmp     BB_SetGetGo            
BB_Get:
    push    ss                      ; lpSrcDev (hBitmap)
    push    cx
    push    eax                     ; DestX = DestY = 0
    push    lpDevice                ; lpDestDev
BB_SetGetGo:
    push    eax                     ; SrcX = SrcY = 0
    push    bWidth                  ; Dest Width
    push    bHeight                 ; Dest Height
    push    DWORD PTR 00CC0020h     ; ROP
    push    eax                     ; lpBrush
    push    eax                     ; lpDrawMode
    call    BitBlt16                ; Do the blt

    pop     ax                      ; Number of bits copied
    pop     dx
    pop     ds
    ret

BB_SetGet_DIBEngine:
    pop     ds
BitmapBits_DIBEngine:
    ; The DIB engine doesnt seem to call BeginAccess and EndAccess - so Sync the chip here
    call    FarBeginAccess

    push    lpDevice
    push    fFlags
    push    dwCount
    push    lpBits
	call	DIB_BitmapBits
    ret
BitmapBits ENDP



_TEXT   ends

end
