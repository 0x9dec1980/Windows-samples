;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: palette.asm
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
        .xlist
DOS5 = 1
        include glint.inc
        include ramdac.inc
        .list

RAMDAC_DELAY macro
        test    InFIFOSpace,0
        test    InFIFOSpace,0
        endm

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT   segment
    assumes cs,_TEXT
    .486

;---------------------------------------------------------------------------
; SetRAMDAC
;   Initialize the DAC to the values stored in a color table
;   composed of triples.
; Entry:
;       ax  - index to 1st palette entry
;       cx  - count of indices to program
;    ds:si --> array of RGBs
;---------------------------------------------------------------------------
SetRAMDAC PROC NEAR C PUBLIC
        push    es
        push    fs
        mov     bx,_INIT
        mov     es, bx
	    assumes	es,_INIT
	    mov	    bx,WORD PTR RemappedDataSegment
	    mov	    es, bx
        mov     bx,GLInfo.wRegSel
        mov     fs,bx
        assumes fs,Glint

        mov     di,offset ExtVC
        cmp     GLInfo.wRamDacType, RGB525_RAMDAC
        jne     SetPaletteTVP3026 

        ; Set up VSControl on external dac
        cmp     GLInfo.dwRenderChipID, TIPERMEDIA2_ID
        jne     @f
        mov     ebx, 1f1h
        mov     fs:[5800h], ebx
        add     di, 01000h          ; point to ext registers.
@@:
        mov     fs:[di]._rgb525_regs.palAddrWr.reg,eax
        RAMDAC_DELAY
        add     ax,ax           ;ax * 4
        add     ax,ax
        add     si,ax           ;pt at color
SetPalColRGB525:
        mov     al,[si+2]
        mov     fs:[di]._rgb525_regs.palData.reg,eax
        RAMDAC_DELAY
        mov     al,[si+1]
        mov     fs:[di]._rgb525_regs.palData.reg,eax
        RAMDAC_DELAY
        mov     al,[si]
        mov     fs:[di]._rgb525_regs.palData.reg,eax
        RAMDAC_DELAY
       add     si,4                   ; dummy for flags
        loop    SetPalColRGB525

        ; reset VSControl on external dac
        cmp     GLInfo.dwRenderChipID, TIPERMEDIA2_ID
        jne     @f
        mov     ebx, 1f7h
        mov     fs:[5800h], ebx
@@:
        pop     fs
        pop     es
        assumes es,nothing
        ret

SetPaletteTVP3026:
	    assumes	es,_INIT
        mov     bx, GLInfo.wCursorControl
        mov     dx, bx
        and     dx, 07fffh                  ; Remove top bit to disable interrupt
        mov     GLInfo.wCursorControl, dx
        mov     fs:[di]._tvp3026_regs.pciAddrWr.reg,eax
        add     ax,ax                       ;ax * 4
        add     ax,ax
        add     si,ax                       ;pt at color
        xor     eax, eax
SetPalColTVP3026:
        mov     al,[si+2]
        mov     fs:[di]._tvp3026_regs.palData.reg,eax
        mov     al,[si+1]
        mov     fs:[di]._tvp3026_regs.palData.reg,eax
        mov     al,[si]
        mov     fs:[di]._tvp3026_regs.palData.reg,eax
        add     si,4                        ; dummy for flags
        loop    SetPalColTVP3026

        mov     GLInfo.wCursorControl, bx   ; restore interrupt flag if required.
        pop     fs
        pop     es
        assumes es,nothing
        ret

SetRAMDAC  endp

SetRAMDAC_far	PROC FAR C PUBLIC
	call	SetRAMDAC
	ret
SetRAMDAC_far	endp

;---------------------------------------------------------------------------
; SetPalette
;  Set the hardware palette from GDI.  This palette is also copied into
;  the screen DIB Color table.
; Entry:
;       start       - first palette index to set
;       count       - count of palette indices
;       lpPalette   -->array of RGBQUADs containing colors
;---------------------------------------------------------------------------
cProc   SetPalette1,<FAR,PUBLIC,PASCAL,NODATA>,<si,di>
    parmW   wStartIndex             ;starting index in the Palette
    parmW   wNumEntries             ;no of indexes in the Palette
    parmD   lpPalette               ;long pointer to color Palette
cBegin
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    cmp     wBpp, 8                             ; Only do this at 8bpp.
    jne     SP_Exit

	push	wStartIndex		                    ;Let the DIB Engine move the
	push	wNumEntries		                    ; palette into the color table.
	push	lpPalette
	push	GLInfo.lpDriverPDevice
	call	DIB_SetPaletteExt

	les	    si,GLInfo.lpDriverPDevice
	test	es:[si].deFlags,BUSY	            ;Is the screen busy?
	jnz	    SP_Exit			                    ;Yes, can't touch the hardware.
	lds	    si, DWORD PTR GLInfo.lpColorTableOff;ds:si-->device color table
	mov	    ax,wStartIndex
	mov	    cx,wNumEntries
	call	SetRAMDAC		                    ;Set new colors into the h/w palette.

PLABEL SP_Exit
    xor     ax,ax                               ;show success
    cwd
cEnd

;---------------------------------------------------------------------------
; SetGammaRamp1
;   Program RAMDAC color table based on given GammaRamp table from GDI.
; Entry:
;       lpGammaRamp  - array of color table for gamma control.
;---------------------------------------------------------------------------

cProc    SetGammaRamp1,<FAR,PUBLIC,PASCAL,NODATA>,<bx,cx,si,di,ds,es>
    parmD   lpGammaRamp

    localV  LocalGammaRamp, 256*4
cBegin
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    es,ax                               ; es = RemappedDataSegment

    xor     ax,ax
    cmp     es:wBpp, 8                          ; Only do this at not 8bpp.
    jz      DSGR_Exit

    lds     si, lpGammaRamp                     ; ds:si = lpGammaRamp
    mov     ax,_INIT
    mov     es, ax
    mov     di, OFFSET GLGamma.GammaRampTable   ; es:di = GLGamma.GammaRampTable

    mov     cx, 256*3*2/4
    rep     movs dword ptr es:[di], ds:[si]     ; copy lpGammaRamp to
                                                ;     GLInfo.GammaRampTable
    mov     ax, 1
    mov     es:GLGamma.ValidGammaTable, ax      ; GLGamma.ValidGammaTable = 0x1;

    lds     si, lpGammaRamp                     ; ds:si = lpGammaRamp
    mov     ax, ss
    mov     es, ax
    lea     di, LocalGammaRamp                  ; es:di = LocalGammaRamp

    mov     cx, 256

DSGR_SetLoop:
    mov     dx, [si][000h][1]                   ; Red
    mov     ah, [si][200h][1]                   ; Green
    mov     al, [si][400h][1]                   ; Blue

    mov     es:[di][2], dx
    mov     es:[di][0], ax                      ; xRGB

    add     si, 2
    add     di, 4

    dec     cx
    jnz     DSGR_SetLoop

    mov     ax, ss
    mov     ds, ax
    lea     si, LocalGammaRamp                  ; ds:si = LocalGammaRamp
    xor     ax, ax                              ; index = 0.
    mov     cx, 256                             ; number of entries = 256.

    call    SetRAMDAC                           ; upload it to RAMDAC
   
DSGR_Success:
    mov     ax, 1                               ; Set ret value as 1.
DSGR_Exit:
    cwd
cEnd

;---------------------------------------------------------------------------
; GetGammaRamp1
; Entry:
;       lpGammaRamp  - array of color table for gamma control.
;---------------------------------------------------------------------------

cProc    GetGammaRamp1,<FAR,PUBLIC,PASCAL,NODATA>,<bx,cx,si,di,ds,es>
    parmD   lpGammaRamp
cBegin
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    es, ax                              ; es = RemappedDataSegment

    xor     ax, ax
    cmp     es:wBpp, 8                          ; Only do this at not 8bpp.
    jz      DGGR_Exit

    mov     ax, ds:GLGamma.ValidGammaTable      ; GammaRamp is already exist ?
    cmp     ax, 1
    jnz     DGGR_Identity                       ; if no, jump to set idet. ramp

    les     di, lpGammaRamp                     ; es:di = lpGammaRamp
    mov     si, OFFSET GLGamma.GammaRampTable   ; ds:si = GLGamma.GammaRampTable

    mov     cx, 256*3*2/4
    rep     movs dword ptr es:[di], ds:[si]     ; copy GLInfo.GammaRampTable to
                                                ;      lpGammaRamp

    jmp     DGGR_Success

DGGR_Identity:

    ; setup identity gamma ramp in buffer.

    lds     si, lpGammaRamp                     ; ds:si = lpGammaRamp
    mov     cx, 0

DGGR_GetLoop:

    mov     ds:[si][000h][0], 0
    mov     ds:[si][000h][1], cl
    mov     ds:[si][200h][0], 0
    mov     ds:[si][200h][1], cl
    mov     ds:[si][400h][0], 0
    mov     ds:[si][400h][1], cl

    add     si, 2
    inc     cx
    cmp     cx, 256
    jnz     DGGR_GetLoop

DGGR_Success:
    mov     ax, 1
DGGR_Exit:
    cwd
cEnd

_TEXT   ends
end
