;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: DIBLINK.ASM
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
DIBLINK macro name,target,extra
name    PROC FAR PASCAL PUBLIC
	assumes	ds,nothing
	assumes	es,nothing
	assumes	fs,nothing
	assumes	gs,nothing
ifnb <extra>
    mov     ax,_INIT
    mov     es, ax
	assumes	es,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    es,ax
	pop	    ecx
	push	extra
	push	ecx
endif
	jmp	target
name    ENDP
	endm

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT   segment
	assumes cs,_TEXT

;DIBLINK BitBlt,		    	DIB_BitBlt
;DIBLINK ColorInfo,		        DIB_ColorInfo
;DIBLINK Control,		    	DIB_Control
;DIBLINK  Disable,		    	DIB_Disable
;DIBLINK  Enable,		        DIB_Enable
DIBLINK EnumDFonts,		    	DIB_EnumDFonts
DIBLINK EnumObj,		    	DIB_EnumObjExt,		        GLInfo.lpDriverPDevice
;DIBLINK Output,				DIB_Output
DIBLINK Pixel,			    	DIB_Pixel
;DIBLINK RealizeObject,			DIB_RealizeObjectExt,	    GLInfo.lpDriverPDevice
;DIBLINK Strblt,			    DIB_Strblt
;DIBLINK ScanLR,		    	DIB_ScanLR
DIBLINK DeviceMode,		    	DIB_DeviceMode
;DIBLINK ExtTextOut,		    DIB_ExtTextOut
DIBLINK GetCharWidth,			DIB_GetCharWidth
DIBLINK DeviceBitmap,			DIB_DeviceBitmap
DIBLINK FastBorder,		    	DIB_FastBorder
DIBLINK SetAttribute,			DIB_SetAttribute
;DIBLINK DibBlt,			    DIB_DibBltExt,		        wPalettized
DIBLINK CreateDIBitmap,		    DIB_CreateDIBitmap
;DIBLINK DibToDevice,			DIB_DibToDevice
;DIBLINK SetPalette			    DIB_SetPaletteExt,	        GLInfo.lpDriverPDevice
DIBLINK GetPalette,		        DIB_GetPaletteExt,	        GLInfo.lpDriverPDevice
DIBLINK SetPaletteTranslate,	DIB_SetPaletteTranslateExt, GLInfo.lpDriverPDevice
DIBLINK GetPaletteTranslate,	DIB_GetPaletteTranslateExt, GLInfo.lpDriverPDevice
DIBLINK UpdateColors,			DIB_UpdateColorsExt,	    GLInfo.lpDriverPDevice
DIBLINK StretchBlt,		    	DIB_StretchBlt
DIBLINK StretchDIBits,			DIB_StretchDIBits
;DIBLINK SelectBitmap,			DIB_SelectBitmap
;DIBLINK BitmapBits,		    DIB_BitmapBits
;DIBLINK  ReEnable,			    DIB_ReEnable
;DIBLINK SetCursor,			    Cursor_SetCursor,	        GLInfo.lpDriverPDevice
;DIBLINK MoveCursor,			Cursor_MoveCursor,	        GLInfo.lpDriverPDevice
;DIBLINK BeginAccess,			DIB_BeginAccess
;DIBLINK EndAccess,		        DIB_EndAccess


; I wish I could do this algorithmicly - but its the way the DIB does it - and we need
; to be consistent.
PLABEL ColorInfo16bppTranslate
    db  000h, 010h, 018h, 020h, 028h, 030h, 038h, 040h
    db  048h, 050h, 058h, 060h, 068h, 070h, 078h, 080h
    db  088h, 090h, 098h, 0a0h, 0a8h, 0b0h, 0b8h, 0c0h
    db  0c8h, 0d0h, 0d8h, 0e0h, 0e8h, 0f0h, 0f8h, 0ffh

ColorInfo PROC FAR PASCAL PUBLIC USES ds di si, lpDestDev:DWORD, dwColorin:DWORD, lpPColor:DWORD
    assumes ds, nothing
    assumes es, nothing
    assumes fs, nothing
    assumes gs, nothing

    lds     si, lpDestDev
    mov     eax, dwColorin
    mov     bx, WORD PTR lpPColor+2
    mov     cl, ds:[si].deBitsPixel

    test    eax, eax
    je      CI_BlackOrWhite
    cmp     eax, 0ffffffh
    je      CI_BlackOrWhite

    test    bx, bx
    je      CI_PhysicalToLogical

    cmp     cl, 16
    je      CI16LogicalToPhysical
    cmp     cl, 32
    je      CI32LogicalToPhysical
    cmp     cl, 8
    je      CI8LogicalToPhysical

PLABEL CI8LogicalToPhysical
PLABEL CI8PhysicalToLogical
; These never seem to get called!
;    int 1		;except in colour depths greater than 8 bits!!

PLABEL Jmp_DIB
    push    lpDestDev
    push    dwColorin
    push    lpPColor
    call    DIB_ColorInfo
    ret

CI_PhysicalToLogical:
    cmp     cl, 8
    je      CI8PhysicalToLogical
    cmp     cl, 16
    je      CI16PhysicalToLogical
    cmp     cl, 32
    je      CI32PhysicalToLogical
    jmp     Jmp_DIB
    

CI_BlackOrWhite:
    test    bx, bx
    je      @f
    lds     si, lpPColor
    mov     edx, eax
    or      eax, 40000000h
    mov     DWORD PTR ds:[si], eax
    shr     edx, 16
    ret
@@:
    and     eax, 0ffffffh
    mov     edx, eax
    shr     edx, 16
    ret

PLABEL CI16LogicalToPhysical
    lds     si, lpPColor

    xor     dx, dx
    xor     bx, bx

    mov     ecx, eax
    shr     ecx, 16                     ; RGB -> XXR. Get Red component
    sub     cx, 4                       ; Round it down
    jae     @f
    mov     cx, 0
@@:                                     ; CX = Corrected XXR
    shr     cx, 3
    mov     bx, cx
    mov     dl, BYTE PTR cs:[OFFSET ColorInfo16bppTranslate + bx]
    shl     edx, 16

    mov     bx, ax
    sub     bx, 00400h                  ; Round it down
    jae     @f
    mov     bx, 0
@@:                                     ; di = Corrected XGX
    shr     bx, 11
    mov     dh, BYTE PTR cs:[OFFSET ColorInfo16bppTranslate + bx]
    shl     bx, 5                       ; move green to new position
    or      cx, bx                      ; RGB -> XGR

    mov     bx, ax
    xor     bh, bh
    sub     bx, 4
    jae     @f
    mov     bx, 0
@@:                                     ; di = Corrected XXB
    shr     bx, 3
    mov     dl, BYTE PTR cs:[OFFSET ColorInfo16bppTranslate + bx]
    shl     bx, 10                      ; move to new position
    or      cx, bx                      ; RGB -> BGR

    mov     eax, edx
    shr     edx, 16
    mov     DWORD PTR ds:[si], ecx
    ret

PLABEL CI16PhysicalToLogical
    mov     bx, ax
    and     bx, 0001fh
    xor     dx, dx
    mov     dl, BYTE PTR cs:[OFFSET ColorInfo16bppTranslate + bx]
    mov     bx, ax
    shl     edx, 16
    and     bx, 07c00h
    shr     bx, 10
    mov     dl, BYTE PTR cs:[OFFSET ColorInfo16bppTranslate + bx]
    mov     bx, ax
    and     bx, 003e0h
    shr     bx, 5
    mov     dh, BYTE PTR cs:[OFFSET ColorInfo16bppTranslate + bx]
    mov     eax, edx
    shr     edx, 16
    ret

PLABEL CI32LogicalToPhysical
    lds     si, lpPColor
    mov     dx, WORD PTR dwColorin + 2
    mov     cl, dl
    mov     dl, al
    mov     al, cl
    mov     WORD PTR ds:[si+2], dx
    mov     WORD PTR ds:[si], ax

    mov     ax, WORD PTR dwColorin
    mov     dx, WORD PTR dwColorin + 2
    ret

PLABEL CI32PhysicalToLogical
    mov     dx, WORD PTR dwColorin + 2
    mov     bl, al
    xor     dh, dh
    mov     al, dl
    mov     dl, bl
    ret

ColorInfo ENDP

_TEXT   ends

end
