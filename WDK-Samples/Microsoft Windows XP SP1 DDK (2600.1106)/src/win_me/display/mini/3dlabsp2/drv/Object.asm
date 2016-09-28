;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: object.asm
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
	.286c
	.xlist
	DOS5 = 1			;so we don't get INC BP in <cBegin>
incLogical = 1

include glint.inc
include	gdidefs.inc

OBJ_PBITMAP = 5

	.list
;----------------------------------------------------------------------------
; D A T A
;----------------------------------------------------------------------------
_INIT segment 

PUBLIC RealizeJumpTable, DeleteBitmapFn, DeleteBrushFn, CreateBrushFn, CreateBitmapFn
DeleteBitmapFn		dw	OFFSET RO_DeleteBitmap 	        ;-5 - Delete Bitmap
					dw	OFFSET RO_DIBEngine		        ;-4 - Delete ?
					dw	OFFSET RO_DIBEngine		        ;-3 - Delete Font
DeleteBrushFn		dw	OFFSET RO_DIBEngine 	        ;-2 - Delete Brush
					dw	OFFSET RO_DIBEngine		        ;-1 - Delete Pen
RealizeJumpTable	dw	OFFSET RO_DIBEngine		        ; 0 - ?	
					dw	OFFSET RO_DIBEngine		        ; 1 - Pen
CreateBrushFn		dw	OFFSET RO_DIBEngine     	    ; 2 - Brush
					dw	OFFSET RO_DIBEngine		        ; 3 - Font
					dw	OFFSET RO_DIBEngine		        ; 4 - ?
CreateBitmapFn		dw	OFFSET RO_CreateBitmap	        ; 5 - Bitmap


_INIT ends

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT segment
	assumes cs,_TEXT

.586

InitialiseRealizeObject PROC FAR C PUBLIC USES ds esi edi
    ; Set up the RealizeJumpTable

    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax

    cmp     wBpp, 8
    je      @f
    mov     DeleteBrushFn, RO_DIBEngine
    mov     CreateBrushFn, RO_DIBEngine
    ret
@@:
    mov     DeleteBrushFn, RO_CreateBrush8bpp
    mov     CreateBrushFn, RO_DeleteBrush
    ret


InitialiseRealizeObject ENDP

CreateDitherBrush8bpp4x4 PROC NEAR PASCAL PUBLIC, 
    ; eax = color
    ; ds:si = destination for brush
    LOCAL   Col:DWORD

    mov     Col, eax
    xor     dx, dx
    xor     cx, cx

    ret
CreateDitherBrush8bpp4x4 ENDP



RealizeObject PROC FAR PASCAL PUBLIC USES ds esi edi,
	lpDevice:DWORD,	wStyle:WORD, lpInObj:DWORD,	lpOutObj:DWORD, lpTextXForm:DWORD
;   lpDevice    PDEVICE to create object for
;   wStyle      Type of object to create (eg OBJ_PBITMAP)
;   lpInObj     Address of an LOGPEN, LOGBRUSH, LOGFONT, or PBITMAP structure		
;   lpOutObj    Address of physical object structure or NULL for structure size
;   lpTextXForm

    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax

	mov		bx, wStyle
	mov		si, bx
	lea		bx, [si+bx+RealizeJumpTable]  
	jmp		[bx]

PLABEL RO_CreateBitmap
    mov     ax, WORD PTR lpOutObj+2         ; Get segment of output structure
    test    ax, ax
    je      RO_SizeBitmap
;   We are trying to create a bitmap.
	assumes	ds,_INIT
    mov     eax, lpOutObj
    add     eax, SIZE OFFSCREEN_BITMAP
    push    lpDevice
    push    wStyle
    push    lpInObj
    push    eax                             ; Dib Engine offset into memory
    push    lpTextXForm
    push    GLInfo.lpDriverPDevice
    call    DIB_RealizeObjectExt
    test    eax,eax
    je      RO_Failed
	invoke	CreateOffscreenBitmap, lpOutObj
	mov		ax, 8000h						; return success
	assumes	ds,nothing
PLABEL RO_Failed
    ret

PLABEL RO_SizeBitmap
;   We are trying to find the size of a physical bitmap structure.
	assumes	ds,_INIT
    push    lpDevice
    push    wStyle
    push    lpInObj
    push    lpOutObj
    push    lpTextXForm
    push    GLInfo.lpDriverPDevice
    call    DIB_RealizeObjectExt
    test    eax,eax
    je      RO_Failed
    add     eax, SIZE OFFSCREEN_BITMAP
    mov     edx, eax
    shr     edx, 16
	assumes	ds,nothing
    ret

PLABEL RO_DeleteBitmap
	assumes	ds,_INIT

    mov     eax, lpOutObj                   ; Compare deleted bitmap with the
    cmp     GLInfo.dwCurrentOffscreen, eax       ; current cached offscreen pointer
    jne     @f
    mov     GLInfo.dwCurrentOffscreen, 0         ; If we have just deleted the current one ...
@@:
	invoke	DeleteOffscreenBitmap, eax
	; fall through to DIB Engine.
    jmp     RO_DIBEngine


PLABEL RO_SizeBrush8bpp
; This is simple: Just return the size of our Brush structure.
    mov     eax, SIZE Cached_Brush8
    ret

PLABEL RO_CreateBrush8bpp
    mov     ax, WORD PTR lpOutObj+2         ; Get segment of output structure
    test    ax, ax
    je      RO_SizeBrush8bpp
; Realise the brush. All we have to do is mark it unrealised by us and the DIB, and leave the
; realisation to the Bit code.
    les     di, lpInObj                     ; Get LOGBRUSH structure
    lds     si, lpOutObj
    mov     ax, es:[di].lbStyle
    mov     ds:[si].dp8BrushBpp, 8
    mov     ds:[si].dp8BrushStyle, ax
    cmp     ax, BS_SOLID
    je      RO_CreateSolidBrush8bpp
    cmp     ax, BS_PATTERN
    je      RO_CreatePatternBrush8bpp
    cmp     ax, BS_HATCHED
    je      RO_CreateHatchedBrush8bpp
;RO_CreateHollowBrush8bpp:                  ; We dont care about this entrypoint
    int     1
    jmp     RO_DIBEngine

PLABEL RO_CreateSolidBrush8bpp
    mov     eax, es:[di].lbColor
    mov     ds:[si].dp8BrushFlags, COLORSOLID+PRIVATEDATA
    test    eax, 0ff000000h
    je      RO_CreateDitherBrush8bpp
    ; Update color and replicate it.
    mov     ah, al                  ;8bpp is now replicated to 16bpp
    mov     ecx,eax                 ;double up from 16 bits to 32 bits
	and		ecx,0ffffh
    shl     eax, 16
    or		ecx, eax
    mov     dword ptr ds:[si].dp8BrushBits, ecx
    mov     ax, 8002h
    ret

PLABEL RO_CreateDitherBrush8bpp
    mov     ds:[si].dp8BrushFlags, PRIVATEDATA
    lea     si, [si + dp8BrushBits]
    ; eax = color
    ; ds:si = destination for brush
    invoke  CreateDitherBrush8bpp4x4
    mov     ax, 8000h
    ret
        
PLABEL RO_CreatePatternBrush8bpp
    mov     ds:[si].dp8BrushFlags, MONOVALID+PRIVATEDATA
    mov     ax, 8000h
    ret

PLABEL RO_CreateHatchedBrush8bpp
    mov     ds:[si].dp8BrushFlags, MONOVALID+PRIVATEDATA
    mov     ax, 8000h
    ret

PLABEL RO_DeleteBrush
; This is simple. Just decide if the brush has been realised by the DIB Engine or not, and call
; the DIB if it has. 2 cases, if we dont have any private data or if we have given it to the DIB
    lds     si, lpInObj
    test    ds:[si].dp8BrushFlags, PRIVATEDATA
    je      RO_DIBEngine
; Theoretically we do not need to do anything if we havnt realised through the DIB.
RO_DeleteBrushExit:
    mov     ax, 8000h
    ret
    

PLABEL RO_DIBEngine
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
    mov     eax, GLInfo.lpDriverPDevice
    pop     edi
    pop     esi
    pop     ds
	assumes	ds,nothing
    pop     bp
	pop	    ecx
	push	eax
    push    ecx
	jmp		DIB_RealizeObjectExt

RealizeObject ENDP

SelectBitmap PROC FAR PASCAL PUBLIC,
    	lpDevice:DWORD,	lpPrevBitmap:DWORD, lpBitmap:DWORD, fFlags:DWORD

    les     bx, lpBitmap
    cmp     es:[bx].deType, TYPE_DIBENG
    jne     SelectBitmap_DIBEngine

    test    es:[bx].deFlags, VRAM
    jne     SelectBitmap_DIBEngine  ; Bitmap is already in Offscreen. 

    push    edi
    push    esi

    invoke  ConsiderMovingToOffscreen, lpBitmap

    pop     esi
    pop     edi

SelectBitmap_DIBEngine:
    pop     bp
	jmp		DIB_SelectBitmap
SelectBitmap ENDP


_TEXT ends

end