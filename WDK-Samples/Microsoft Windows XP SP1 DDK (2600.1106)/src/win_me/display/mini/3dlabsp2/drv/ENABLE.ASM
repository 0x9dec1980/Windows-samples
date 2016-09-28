;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: ENABLE.ASM
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

        .xlist
DOS5 = 1
        include glint.inc
        incDevice = 1
        include gdidefs.inc
        .list

;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------
VDD		        equ     000Ah           ;id of Virtual Display Driver.
DRV_VERSION	= 0400h
STOP_IO_TRAP    = 4000h		        ; stop io trapping
START_IO_TRAP   = 4007h			; re-start io trapping

;----------------------------------------------------------------------------
; M A C R O S
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; D A T A
;----------------------------------------------------------------------------
_INIT   segment
    globalD pGLInfo,0                        ;Far Pointer to GLInfo
    globalW wDIBPdevSize,0
    globalW	wLastValidRes,0
    globalW	wLastValidBpp,0
    globalW	wLastValidMode,0
    globalW	E_HasInitialisedHW,0
    globalW	E_Padding,0
    globalD dwEnabled,0
_INIT   ends
;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT   segment
assumes cs,_TEXT
        .386
;----------------------------------------------------------------------------
; Color Table used for palettized 8 bpp format.
;----------------------------------------------------------------------------
DIB8ColorTable  label   dword
;               blue  green red   flags
 DIBColorEntry <0000, 0000, 0000, 0000      >  ; 0
 DIBColorEntry <0000, 0000, 080h, 0000      >  ; 1
 DIBColorEntry <0000, 080h, 0000, 0000      >  ; 2
 DIBColorEntry <0000, 080h, 080h, 0000      >  ; 3
 DIBColorEntry <080h, 0000, 0000, 0000      >  ; 4
 DIBColorEntry <080h, 0000, 080h, 0000      >  ; 5
 DIBColorEntry <080h, 080h, 0000, 0000      >  ; 6
 DIBColorEntry <0C0h, 0C0h, 0C0h, MAPTOWHITE>  ; 7  Light Grey
 DIBColorEntry <0C0h, 0dch, 0C0h, NONSTATIC+MAPTOWHITE>  ; Money Green
 DIBColorEntry <0F0h, 0CAh, 0A6h, NONSTATIC+MAPTOWHITE>  ; cool blue
REPT 236
 DIBColorEntry <0000, 0000, 0000, NONSTATIC >  ; Palette Manager colors
ENDM
 DIBColorEntry <0F0h, 0FBh, 0FFh, NONSTATIC+MAPTOWHITE>  ; off white
 DIBColorEntry <0A4h, 0A0h, 0A0h, NONSTATIC+MAPTOWHITE>  ; med grey
 DIBColorEntry <080h, 080h, 080h, MAPTOWHITE>  ; 8  Dark Grey
 DIBColorEntry <0000, 0000, 0FFh, 0000      >  ; 9
 DIBColorEntry <0000, 0FFh, 0000, MAPTOWHITE>  ; a
 DIBColorEntry <0000, 0FFh, 0FFh, MAPTOWHITE>  ; b
 DIBColorEntry <0FFh, 0000, 0000, 0000      >  ; c
 DIBColorEntry <0FFh, 0000, 0FFh, 0000      >  ; d
 DIBColorEntry <0FFh, 0FFh, 0000, MAPTOWHITE>  ; e
 DIBColorEntry <0FFh, 0FFh, 0FFh, MAPTOWHITE>  ; f
DIB8ColorTableSize = $ - DIB8ColorTable

;----------------------------------------------------------------------------
; ReEnable
;   Called by GDI to change the display resolution dynamically. This function
;   will retrieve the new resolution value from the register, map it into
;   a resolution ID (0 = 320x200, 1=640x480, 2=800x600, 3=1024x768 will set
;   set wResolution to this value. It will then call Enable twice to set up
;   the GDIInfo block and the PDevice block appropriately.
;
;   The return value is FALSE if the function failed, else it is TRUE
;----------------------------------------------------------------------------
cProc ReEnable1,<FAR,PUBLIC,PASCAL,NODATA>,<esi,edi>
        parmD   lpPDevice               ;-> devices PDevice
        parmD   lpGDIInfo               ;-> GDI Info block
cBegin

    ; As the very first thing, clean up any existing heap allocations that we have.
    ; Everything needs to be uploaded back to host memory before we corrupt the
    ; frame store.
    invoke  DisableOffscreenHeap

    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing
	mov	ax,wBpp
	mov	wLastValidBpp,ax
	call	GetScreenSettings	    ; Get new res, etc.
    mov     bReEnable,1             ; this is a reenable call
@@:
;
; call enable to create a new PDevice and init the h/w.
;
    xor     eax,eax
    cCall   Enable1,<lpPDevice,0,eax,eax,eax>
	push	ax			;save results from Enable.
	pop	ax
@@:	test	ax,ax
	jz	RE_Fail
;
; call enable to fill in the new GDIINFO Block
;
    xor     eax,eax
    cCall   Enable1,<lpGDIInfo,1,eax,eax,eax>
	mov	ax,1
PLABEL RE_Exit
	mov	bReEnable,0
cEnd

PLABEL RE_Fail
	mov	ax,wLastValidBpp
	mov	wBpp,ax
    xor     eax,eax
    cCall   Enable1,<lpPDevice,0,eax,eax,eax>
	call	RepaintAddr
	xor	ax,ax
	jmp	RE_Exit

;----------------------------------------------------------------------------
; Enable
;   Called twice by GDI at boot time.  1st time to retrieve GDIINFO
;   structure, 2nd time to initialize video hardware.
;----------------------------------------------------------------------------
cProc   Enable1,<FAR,PUBLIC,PASCAL,NODATA>,<esi,edi>
    parmD   lpDevice                ;Physical device or GDIinfo destination
    parmW   style                   ;Style, Enable Device, or Inquire Info
    parmD   lpDeviceType            ;Device type (i.e FX80, HP7470, ...)
    parmD   lpOutputFile            ;DOS output file name (if applicable)
    parmD   lpStuff                 ;Device specific information
cBegin
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    cmp    word ptr VDDEntryPoint+2, 0
    jne     @f
    xor     di, di                  ;Clear these before the int 2f
    mov     es, di
    mov     ax,1684h                ;Int 2FH: GET DEVICE API ENTRY POINT
    mov     bx,VDD                  ;this is VxD ID for VDD
    int     2fh                     ;get the API Entry Point
    mov     word ptr VDDEntryPoint,di
    mov     word ptr VDDEntryPoint+2,es

	call	GetScreenSettings
@@:

    cmp     E_HasInitialisedHW, 0
    jne     E_HWInitialised

    mov     E_HasInitialisedHW, 1
; Call the VDD to fix the data segment and initialise GLInfo for us.

    mov     eax, _TEXT32
    mov     GLInfo.dwText32Selector, eax

; This is a null terminated list of addresses to store the new data segment.
    lea     edi, GLInfo.dwDMABufferPhys
    mov     WORD PTR [edi+0], OFFSET RemappedDataSegment
    mov     WORD PTR [edi+2], SEG RemappedDataSegment
    mov     WORD PTR [edi+4], 0
    mov     WORD PTR [edi+6], 0

    mov     ecx, MINIVDD_SHAREGLINFO
	mov	    eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov  	ebx,GLInfo.dwDeviceHandle 	                ; VDD PM API needs this
    mov     edx, GLInfo.DesktopDisplayInfo.ddiDevNodeHandle
    mov     si,OFFSET GLInfo                    ; pass it our pointer
	call	dword ptr VDDEntryPoint             ; hope it works
    inc     ax                                  ; ok?
    jz      E_ErrorExit                         ; ouch!!

    xor     eax, eax
    mov     ax, WORD PTR RemappedDataSegment    ; use new data segment
    mov     WORD PTR pGLInfo+2,ax               ; Update pGLInfo in data segment
    mov     WORD PTR pGLInfo, OFFSET GLInfo
    mov     ds, ax
    mov     RemappedDataSegment, eax            ; Update Remapped data segment in the new segment.
    mov     WORD PTR pGLInfo+2,ax               ; Update pGLInfo in remapped data segment
    mov     WORD PTR pGLInfo, OFFSET GLInfo
    mov     eax,GLInfo.ddFBSize                 ; for display size if driver not running
    mov     GLInfo.dwVideoMemorySize,eax

E_HWInitialised:
    ; Ensure the correct data segment has been set up
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax    

    test    style,1                 ;Get GDIINFO?
	jnz	E_GetGDIInfo		;yes.

;----------------------------------------------------------------------------
; Init the PDEVICE.
;----------------------------------------------------------------------------

; Ensure we are trying to run at a depth we support:
    mov     ax, wBpp
    cmp     ax, 8
    je      @f
    cmp     ax, 16
    je      @f
    cmp     ax, 32
    je      @f
    cmp     GLInfo.dwRenderFamily, PERMEDIA_ID
    jne     E_ErrorExit                         ; Only on P2 do we support 24bpp
    cmp     GLInfo.dwRenderChipID, PERMEDIA_ID
    je      E_ErrorExit                         ; Only on P2 do we support 24bpp
    cmp     GLInfo.dwRenderChipID, TIPERMEDIA_ID
    je      E_ErrorExit                         ; Only on P2 do we support 24bpp
    cmp     ax, 24
    je      @f                                  ; Accept 24bpp on P2 and newer
    jmp     E_ErrorExit
@@:

	mov	    eax,lpDevice
	mov	    GLInfo.lpDriverPDevice,eax	;save ptr to my pDevice.
    call    PhysicalEnable          ;Attempt to init the h/w.
	jc	E_ErrorExit		;did not work. Fail the call.
    cmp     bReEnable,0             ;re-enable being done ?
    jnz     short @f                ;yes, bypass int 2f call
    mov     ax,STOP_IO_TRAP
    int     2fh
@@: cCall   DIB_Enable,<lpDevice,style,lpDeviceType,lpOutputFile,lpStuff>
    cmp     wPalettized,0           ;Is the display palettized?
    je      short @f                ;no.
    xor     ax, ax
    mov     edx, lpDevice
    push    ax
    push    ax
    cCall   SetPaletteTranslate     ;initialize the palette trans. table
@@: les     di,lpDevice             ;--> dest: PDEVICE or GDIINFO struct.
    assumes es,nothing

; Build the driver's PDevice

    mov     es:[di].deType,'RP'
    mov     ax,wScreenWidth             ;ax = width of screen in pels.
    mov     es:[di].deWidth,ax          ;store it.
    mov     ax,wScreenHeight            ;ax = height of screen in scans.
    mov     es:[di].deHeight,ax         ;store it.
    movzx   eax,wScreenWidthBytes       ;ax = width of screen in bytes.
    mov     es:[di].deWidthBytes,ax     ;store off the byte screen width.
    mov     es:[di].deDeltaScan,eax     ;store off the delta scan width.
    mov     bx,wBpp                     ;bx = bpp.
    inc     bh                          ;bh = 1
    xchg    bl,bh                       ;bl = planes (always 1), bh = bpp
    mov     word ptr es:[di].dePlanes,bx;store off planes, bpp.
    xor     eax,eax                     ;offset to surface bits is zero.
    mov     es:[di].deReserved1,eax     ;make this zero.
    mov     es:[di].delpPDevice,eax     ;make this zero.
    mov     dword ptr es:[di].deBits,eax;store it.
    mov     ax,ScreenSelector           ;ax = selector to the surface bits.
    mov     word ptr es:[di].deBits+4,ax;store it.

    ; Set up the surface flags. For permedia on 1600x1200 we cannot
    ; accelerate. Therefore, mark the surface as not ACCELERATED
    mov     es:[di].deDriverReserved, ACCELERATED
    mov     ax, GLInfo.wRenderChipID
    cmp     ax, PERMEDIA_ID
    je      @f
    cmp     ax, TIPERMEDIA_ID
    jne     E_Continue
@@:
    ; On a Permedia1 or TIPermedia1
    cmp     wScreenWidth, 1280
    jle     E_Continue
    mov     es:[di].deDriverReserved, 0 ; 1600x1200 Permedia - not accelerated!
E_Continue:
    mov     ax,MINIDRIVER+VRAM
    cmp     wPalettized,0               ;Is the surface palettized?
    je      short @f                    ;no.
    or      ax,PALETTIZED               ;yes. Set the PALETTIZED bit too.
@@: mov     es:[di].deFlags,ax          ;store PDevice flags.
    mov     bx,di
    add     bx,wDIBPdevSize             ;es:bx -->surface's BitmapInfoHeader
    mov     word ptr es:[di].deBitmapInfo,bx  ;store it.
    mov     word ptr es:[di].deBitmapInfo+2,es;es = selector of BitmapInfoHeader
    mov     es:[di].deVersion,DRV_VERSION   ;Show that we are version 4.

; Fill in the appropriate OSB information
    mov     eax, GLInfo.dwScreenBase    ; Get Bytes to screen
    mov     ecx, GLInfo.dwBpp
    mov     esi, eax
    xor     edx, edx
    shr     cx, 3                       ; 1, 2, 3 or 4 bytes per pixel
    div     ecx                         ; number of pixels to screen
    mov     es:[di].OSB_Heap, 0         ; save Heap Ptr
    mov     es:[di].OSB_PixelOffset, eax; Pixel offset to screen address
    mov     dword ptr es:[di].deBits,esi;store framestore ptr.

; Copy the call back address of the Begin/End access functions

	mov	    ax,seg BeginAccess
	shl	    eax,16
	mov	    ax,offset BeginAccess
    mov     dword ptr es:[di].deBeginAccess,eax
	mov	    ax,seg EndAccess
	shl	    eax,16
	mov	    ax,offset EndAccess
    mov     dword ptr es:[di].deEndAccess,eax

; Now fill in the BitmapInfoHeader following the PDevice.

    mov     es:[bx].biSize,(size BitmapInfoHeader)  ;store header size.
    xor     eax, eax
    mov     ax,es:[di].deWidth              ;get pel width.
    mov     es:[bx].biWidth,eax             ;store it.
    mov     ax,es:[di].deHeight             ;get # of scans.
    mov     es:[bx].biHeight,eax            ;store it.
    mov     ax,wBpp                         ;ax = bpp.
    rol     eax,16                          ;eax.msw = bpp
    inc     ax                              ;eax.lsw = 1
    mov     dword ptr es:[bx].biPlanes,eax  ;store planes, bpp.
    xor     eax,eax
    mov     es:[bx].biCompression,eax       ;zero these for now.
    mov     es:[bx].biSizeImage,eax
    mov     es:[bx].biXPelsPerMeter,eax
    mov     es:[bx].biYPelsPerMeter,eax
    mov     es:[bx].biClrUsed,eax
    mov     es:[bx].biClrImportant,eax

; Now copy in the color table (if there is one). If this is a re-enable call,
; we will not need to copy the color table, the PDevice would already have
; one.

    mov     ax,wBpp                         ;ax = bpp.  Assume 8 bpp.
    mov     si,offset DIB8ColorTable        ;cs:si-->DIB8 Color table.
    mov     cx,DIB8ColorTableSize/4         ;cx = size of DIB8 color table.
@@: cmp     ax,8                            ;is bpp 8?
    jg      E_Install2FHandler              ;It is bigger. No color table.
    test    GLInfo.dwFlags, GMVF_8BPPRGB
    jne     E_Install2FHandler

PLABEL E_CopyColorTable
    lea     di,[bx].biClrImportant+4        ;es:di-->start of color table.
    mov     GLInfo.lpColorTableOff,di
    mov     GLInfo.lpColorTableSeg,es       ;remember address of color table.
    mov     GLInfo.pColorTable,0
    cmp     bReEnable,0                     ;ReEnable call ?
    jnz     short @f                        ;yes, colors are already there
    rep     movs dword ptr es:[di],cs:[si]
@@:
	mov	    cx,256
@@:	xor	    ax,ax
	push	ds
	lds	    si,DWORD PTR GLInfo.lpColorTableOff
	call	SetRAMDAC_far			;set up initial palette
	pop	    ds

PLABEL E_Install2FHandler
    cmp     bReEnable,0                     ;re-enable call ?
    jnz     short @f                        ;yes, no need of int 2f hooking
    test    GLInfo.DesktopDisplayInfo.ddiInfoFlags, DEVICE_IS_NOT_VGA
    jne     @f                              ;Dont hook if we are secondary.
    call    HookInt2Fh                      ;Hook into multiplexed interrupt
@@:
    cmp     GLInfo.lpDDHAL_SetInfo,0        ; DDRAW init done ?
    jz      @f
    invoke  SetDriverInfo, 1                ; reset data
@@:
    mov     eax,1
    mov	    dwEnabled,eax
    jmp     E_Exit

;----------------------------------------------------------------------------
; Return GDIINFO to GDI.
;----------------------------------------------------------------------------
PLABEL E_GetGDIInfo
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing
    cCall   DIB_Enable,<lpDevice,style,lpDeviceType,lpOutputFile,lpStuff>

    les     di,lpDevice             ;--> dest: GDIINFO struct.
    assumes es,nothing

    mov     es:[di].dpVersion,DRV_VERSION
    mov     es:[di].dpDEVICEsize, SIZE OFFSCREEN_BITMAP
    mov     ax,wScreenWidth        ; get width
    mov     bx,wScreenHeight        ; get height
    mov     es:[di].dpHorzRes, ax
    mov     es:[di].dpVertRes, bx
    mov     ax,wDpi                 ;Patch up LogPixels X and Y with wDpi.
    mov     es:[di].dpLogPixelsX,ax
    mov     es:[di].dpLogPixelsY,ax
    mov     es:[di].dpDCManage,DC_IgnoreDFNP
    or      es:[di].dpCaps1,C1_REINIT_ABLE+C1_COLORCURSOR;
    or      es:[di].dpPolygonals,PC_POLYGON+PC_RECTANGLE+PC_INTERIORS+PC_WINDPOLYGON; 
    or      es:[di].dpText,TC_OP_CHARACTER+TC_OP_STROKE+TC_CP_STROKE+TC_RA_ABLE;    +TC_VA_ABLE+TC_SF_X_YINDEP+TC_SA_DOUBLE+TC_SA_INTEGER+TC_SA_CONTIN+TC_EA_DOUBLE+TC_CR_90+TC_CR_ANY+TC_IA_ABLE+TC_UA_ABLE+TC_SO_ABLE

    mov     ax, WORD PTR GLInfo.dwFontCache16   ; Get Font Cache segment
	test	ax, ax
	je		@f
; Only export our text capabilities if we have some font cache
    or      es:[di].dpCaps1, C1_GLYPH_INDEX+C1_BYTE_PACKED
@@:

    mov     ax,es:[di].dpDEVICEsize
    mov     wDIBPdevSize,ax         ;save DIB Eng. PDevice size.

    mov     bx,wBpp
    mov     es:[di].dpBitsPixel, bx
    cmp     bx,8
    jg      E_BPPGreaterThan8
    test    GLInfo.dwFlags, GMVF_8BPPRGB
    jne     E_BPPGreaterThan8
                                                                                                    
PLABEL E_BPPIs8
    mov     es:[di].dpPlanes, 1
    mov     es:[di].dpBitsPixel, bx
    mov     ax,-1
    mov     es:[di].dpNumFonts,ax           ;# of fonts (a big value)
    mov     es:[di].dpNumBrushes,ax         ;# of brushes (a big value)
    mov     es:[di].dpNumPens,16            ;# of pens this driver realizes
    mov     es:[di].dpNumColors,20          ;# colors in color table
    mov     es:[di].dpNumPalReg,256
    mov     es:[di].dpPalReserved,20
    mov     es:[di].dpColorRes,18
    or      es:[di].dpRaster,RC_PALETTE+RC_DIBTODEV
    mov     ax,(size BitmapInfoHeader) + 256*4
    add     es:[di].dpDEVICEsize,ax         ;GDI will allocate this for us.
    mov     ax,SIZE GDIINFO                      ;return size of GDIINFO
    jmp     E_Exit

PLABEL E_BPPGreaterThan8
    mov     es:[di].dpPlanes, 1
    mov     es:[di].dpBitsPixel, bx
    mov     ax,-1
    mov     es:[di].dpNumFonts,ax           ;# of fonts this driver realizes
    mov     es:[di].dpNumPens,ax            ;# of pens this driver realizes
    mov     es:[di].dpNumBrushes,ax         ;# of brushes (a big value)
    mov     es:[di].dpNumColors,ax          ;# colors in color table
    inc     ax
    mov     es:[di].dpNumPalReg,ax
    mov     es:[di].dpPalReserved,ax
    mov     es:[di].dpColorRes,ax
    or      es:[di].dpRaster,RC_DIBTODEV
    or      es:[di].dpCaps1,C1_GAMMA_RAMP   ;GammaRamp support is only for
                                            ; greater than 8bpp mode.
    add     es:[di].dpDEVICEsize,(size BitmapInfoHeader) ;GDI will allocate
                                                         ; this for us.
    mov     ax,SIZE GDIINFO                   ;return size of GDIINFO

PLABEL E_Exit
cEnd

PLABEL E_ErrorExit
	xor	    ax,ax
	jmp	    E_Exit

_TEXT ends
end
