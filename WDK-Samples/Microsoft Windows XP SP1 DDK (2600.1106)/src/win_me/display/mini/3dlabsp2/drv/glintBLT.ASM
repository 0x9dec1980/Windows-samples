;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: GLINTBLT.ASM
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
	call	BltCursorExclude		;Do clipping
    jc      BS_Done					;exit if null blt.
NoSWCursor:
endm


.586


_INIT   segment

; Some function call switches
    globalD MemCopyFn,              0
    globalD FillRectFn,             0
    globalD GreyFillRectFn,         0
    globalD ROPFillRectFn,          0
    globalD CopyBltFn,              0
    globalD ROPCopyBltFn,           0
    globalD PatCopyColorBrushBltFn, 0
    globalD PatCopyMonoBrushBltFn,  0
    globalD ImageUploadFn,          0
    globalD ImageDownloadFn,        0
    globalD MonoImageDownloadFn,    0
    globalD MonoROPImageDownloadFn, 0
    globalD ROPColorBrushBltFn,     0
    globalD ROPMonoBrushBltFn,      0
    globalD ROPImageDownloadFn,     0
    globalD DestMemoryFn,           0


; Some cached render commands
	globalD FillRectCmd,0
	globalD P2FillRectCmd,0
	globalD ROPFillRectCmd,0
	globalD BrushFillRectCmd,0
	globalD P2BrushFillRectCmd,0
	globalD CopyBltCmd,0
	globalD CopyBltSlowCmd,0
	globalD SolidMonoBrushCmd,0
	globalD ROPMonoBrushCmd,0
	globalD ImageUploadCmd,0
	globalD P2MonoROPImageDownloadCmd,0

	globalD DD_CurrentContext,0
	globalD DD_CurrentColour,0
	globalD DD_Disconnect,0

; Space for upload functions
PIU_FirstDWORD  dd  OFFSET PIU_UploadFirst4Bytes
                dd  OFFSET PIU_UploadFirst3Bytes
                dd  OFFSET PIU_UploadFirst2Bytes
                dd  OFFSET PIU_UploadFirst1Byte
PIU_LastDWORD   dd  OFFSET PIU_UploadLast4Bytes
                dd  OFFSET PIU_UploadLast1Byte
                dd  OFFSET PIU_UploadLast2Bytes
                dd  OFFSET PIU_UploadLast3Bytes

PIU_SingleDWORD dd  OFFSET PIU_Done
                dd  OFFSET PIU_Upload1Byte
                dd  OFFSET PIU_Upload2Bytes
                dd  OFFSET PIU_Upload3Bytes
                dd  OFFSET PIU_Upload4Bytes

	;jump table for image downloads
PID_lastWords	dd	PID_AllWordsDone, PID_1More, PID_2More, PID_3More, PID_4More, PID_5More
				dd	PID_6More, PID_7More, PID_8More, PID_9More, PID_10More


;same bits as in fbReadMode

ROP_REQ_SRC  =     0200h                   ; source read required
ROP_REQ_DST  =     0400h                   ; dest read required

; Enabled Logical Ops
__GLINT_LOGICOP_CLEAR           equ  1                          ; 0
__GLINT_LOGICOP_AND             equ  3+ROP_REQ_SRC+ROP_REQ_DST  ; S&D
__GLINT_LOGICOP_AND_REVERSE     equ  5+ROP_REQ_SRC+ROP_REQ_DST  ; S&~D
__GLINT_LOGICOP_COPY            equ  0+ROP_REQ_SRC              ; S (7+ROP_REQ_SRC)
__GLINT_LOGICOP_AND_INVERTED    equ  9+ROP_REQ_SRC+ROP_REQ_DST  ; ~S&D
__GLINT_LOGICOP_NOOP            equ 11+ROP_REQ_DST              ; D
__GLINT_LOGICOP_XOR             equ 13+ROP_REQ_SRC+ROP_REQ_DST  ; S^D
__GLINT_LOGICOP_OR              equ 15+ROP_REQ_SRC+ROP_REQ_DST  ; S|D
__GLINT_LOGICOP_NOR             equ 17+ROP_REQ_SRC+ROP_REQ_DST  ; ~(S|D)
__GLINT_LOGICOP_EQUIV           equ 19+ROP_REQ_SRC+ROP_REQ_DST  ; ~(S^D)
__GLINT_LOGICOP_INVERT          equ 21+ROP_REQ_DST              ; ~D
__GLINT_LOGICOP_OR_REVERSE      equ 23+ROP_REQ_SRC+ROP_REQ_DST  ; S|~D
__GLINT_LOGICOP_COPY_INVERT     equ 25+ROP_REQ_SRC              ; ~S
__GLINT_LOGICOP_OR_INVERT       equ 27+ROP_REQ_SRC+ROP_REQ_DST  ; ~S|D
__GLINT_LOGICOP_NAND            equ 29+ROP_REQ_SRC+ROP_REQ_DST  ; ~(S&D)
__GLINT_LOGICOP_SET             equ 31                          ; 1
                                
;/*
; * decompose all rops into GLiNT logic ops if the dest is the screen.
; * Sometimes we do this in a few stages. The following defines mimic
; * the definitions found in the ROP3 table in the books. The idea
; * is to minimize errors in constructing the ropTable array below by
; * allowing me to copy the reverse Polish notation more or less in
; * tact.
; */
unset   equ __GLINT_LOGICOP_CLEAR
P       equ __GLINT_LOGICOP_COPY
S       equ P
DPna    equ __GLINT_LOGICOP_AND_INVERTED
DSna    equ DPna
DPa     equ __GLINT_LOGICOP_AND
DSa     equ DPa
PDa     equ DPa
SDa     equ DPa
PDna    equ __GLINT_LOGICOP_AND_REVERSE
SDna    equ PDna
DPno    equ __GLINT_LOGICOP_OR_INVERT
DSno    equ DPno
DPo     equ __GLINT_LOGICOP_OR
DSo     equ DPo
PDo     equ DPo
SDo     equ DPo
PDno    equ __GLINT_LOGICOP_OR_REVERSE
SDno    equ PDno
D       equ __GLINT_LOGICOP_NOOP
Dn      equ __GLINT_LOGICOP_INVERT
Pn      equ __GLINT_LOGICOP_COPY_INVERT
Sn      equ Pn
DPan    equ __GLINT_LOGICOP_NAND
DSan    equ DPan
PDan    equ DPan
SDan    equ DPan
DPon    equ __GLINT_LOGICOP_NOR
DSon    equ DPon
PDon    equ DPon
SDon    equ DPon
DPxn    equ __GLINT_LOGICOP_EQUIV
DSxn    equ DPxn
PDxn    equ DPxn
SDxn    equ DPxn
DPx     equ __GLINT_LOGICOP_XOR
DSx     equ DPx
PDx     equ DPx
SDx     equ DPx
set     equ __GLINT_LOGICOP_SET
                 
;/*               
; * if we have to first combine the source and pattern before downloading
; * to GLiNT we use the engine to do it using EngBitBlt. So these are the
; * chosen rop3s which combine the source with the pattern. We blt into
; * a temporary bitmap and use this to download.
; */
SPa     equ 030h
PSa     equ SPa
SPan    equ 03Fh
PSan    equ SPan
SPna    equ 00Ch
PSna    equ 030h

SPo     equ 0FCh
PSo     equ SPo
SPon    equ 003h
PSon    equ SPon
SPno    equ 0CFh
PSno    equ 0F3h

SPx     equ 03Ch
PSx     equ SPx
SPxn    equ 0C3h
PSxn    equ SPxn
SPnx    equ SPxn
PSnx    equ SPxn


; we set up a jump table for the different rop3's. Each entry contains
; a category and a set of 1 (, 2 or 3) GLINT logic ops. In the main blt
; routine we switch on the category to figure out what routine to call.
; We pass the GLINT logic op straight in without having to do any further
; conversion. By keeping each entry in the table down to 8 bytes it
; takes up 2K of data. That's not too much. The benefit is that in each
; routine we call we don't have to do any checking to see whether the
; rop really needs pattern or source. I've done some pre-processing on
; some of the rops to decompose them into forms which allow us to use
; the hardware in a series of steps. e.g. pattern fill followed by
; source download. If anything doesn't fit into a defined category then
; we go back to the engine.
;

; categories
BLACKNESS_BLT       equ BlacknessBlt
WHITENESS_BLT       equ WhitenessBlt
DEST_BLT            equ BS_Done
DESTINVERT_BLT      equ DestInvertBlt
PATCOPY_BLT         equ PatCopyBlt
SRCCOPY_BLT         equ SrcCopyBlt
PAT_FILL_1_BLT      equ ROPBrushBlt       
SRC_FILL_1_BLT      equ ROP2SrcCopyBlt       

PAT_SRC_2_BLT       equ Jmp_DIB_BitBlt       
PAT_SRC_PAT_3_BLT   equ Jmp_DIB_BitBlt       

SRC_PAT_2_BLT       equ Jmp_DIB_BitBlt       
SRC_PAT_SRC_3_BLT   equ Jmp_DIB_BitBlt       

ENG_DOWNLOAD_2_BLT  equ Jmp_DIB_BitBlt       
ENGINE_BITBLT       equ Jmp_DIB_BitBlt       

ropTable dd BLACKNESS_BLT,      unset   ;,  0,      0      ;/* 00 BLACKNESS */    
         dd SRC_PAT_2_BLT,      SDo     ;,    DPon,   0      ;/* 01 */           
         dd SRC_PAT_2_BLT,      DSna    ;,   DPna,   0      ;/* 02 */    
         dd SRC_PAT_2_BLT,      S		;,      PDon,   0      ;/* 03 */    
         dd SRC_PAT_2_BLT,      SDna	;,   DPna,   0      ;/* 04 */    
         dd PAT_FILL_1_BLT,     DPon	;,   0,      0      ;/* 05 */    
         dd SRC_PAT_2_BLT,      DSxn	;,   PDon,   0      ;/* 06 */    
         dd SRC_PAT_2_BLT,      DSa		;,    PDon,   0      ;/* 07 */    
         dd SRC_PAT_2_BLT,      DSa		;,    DPna,   0      ;/* 08 */    
         dd SRC_PAT_2_BLT,      DSx		;,    PDon,   0      ;/* 09 */    
         dd PAT_FILL_1_BLT,     DPna	;,   0,      0      ;/* 0A */    
         dd SRC_PAT_2_BLT,      SDna	;,   PDon,   0      ;/* 0B */    
         dd SRC_PAT_2_BLT,      S		;,      DPna,   0      ;/* 0C */    
         dd SRC_PAT_2_BLT,      DSna	;,   PDon,   0      ;/* 0D */    
         dd SRC_PAT_2_BLT,      DSon	;,   PDon,   0      ;/* 0E */    
         dd PAT_FILL_1_BLT,     Pn		;,     0,      0      ;/* 0F */    
         dd SRC_PAT_2_BLT,      DSon	;,   PDa,    0      ;/* 10 */    
         dd SRC_FILL_1_BLT,     DSon	;,   0,      0      ;/* 11 */    
         dd PAT_SRC_2_BLT,      DPxn	;,   SDon,   0      ;/* 12 */    
         dd PAT_SRC_2_BLT,      DPa		;,    SDon,   0      ;/* 13 */    
         dd ENG_DOWNLOAD_2_BLT, PSx		;,    SDno,   0      ;/* 14 */    
         dd ENG_DOWNLOAD_2_BLT, PSa		;,    DSon,   0      ;/* 15 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 16 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 17 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 18 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 19 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 1A */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 1B */    
         dd PAT_SRC_PAT_3_BLT,  DPa		;,    SDo,    PDx    ;/* 1C */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 1D */    
         dd SRC_PAT_2_BLT,      DSo		;,    PDx,    0      ;/* 1E */    
         dd SRC_PAT_2_BLT,      DSo		;,    PDan,   0      ;/* 1F */    
         dd SRC_PAT_2_BLT,      DSna	;,   PDa,    0      ;/* 20 */    
         dd PAT_SRC_2_BLT,      DPx		;,    SDon,   0      ;/* 21 */    
         dd SRC_FILL_1_BLT,     DSna	;,   0,      0      ;/* 22 */    
         dd PAT_SRC_2_BLT,      PDna	;,   SDon,   0      ;/* 23 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 24 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 25 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 26 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 27 */    
         dd ENG_DOWNLOAD_2_BLT, PSx		;,    DSa,    0      ;/* 28 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 29 */    
         dd ENG_DOWNLOAD_2_BLT, PSa		;,    DSna,   0      ;/* 2A */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 2B */    
         dd SRC_PAT_SRC_3_BLT,  DSo		;,    PDa,    SDx    ;/* 2C */    
         dd SRC_PAT_2_BLT,      SDno	;,   PDx,    0      ;/* 2D */    
         dd PAT_SRC_PAT_3_BLT,  DPx		;,    SDo,    PDx    ;/* 2E */    
         dd SRC_PAT_2_BLT,      SDno	;,   PDan,   0      ;/* 2F */    
         dd SRC_PAT_2_BLT,      S		;,      PDna,   0      ;/* 30 */    
         dd PAT_SRC_2_BLT,      DPna	;,   SDon,   0      ;/* 31 */    
         dd SRC_PAT_SRC_3_BLT,  SDo		;,    PDo,    SDx    ;/* 32 */    
         dd SRC_FILL_1_BLT,     Sn		;,     0,      0      ;/* 33 */    
         dd SRC_PAT_SRC_3_BLT,  DSa		;,    PDo,    SDx    ;/* 34 */    
         dd SRC_PAT_SRC_3_BLT,  DSxn	;,   PDo,    SDx    ;/* 35 */    
         dd PAT_SRC_2_BLT,      DPo		;,    SDx,    0      ;/* 36 */    
         dd PAT_SRC_2_BLT,      DPo		;,    SDan,   0      ;/* 37 */    
         dd PAT_SRC_PAT_3_BLT,  DPo		;,    SDa,    PDx    ;/* 38 */    
         dd PAT_SRC_2_BLT,      PDno	;,   SDx,    0      ;/* 39 */    
         dd SRC_PAT_SRC_3_BLT,  DSx		;,    PDo,    SDx    ;/* 3A */    
         dd PAT_SRC_2_BLT,      PDno	;,   SDan,   0      ;/* 3B */    
         dd SRC_PAT_2_BLT,      S		;,      PDx,    0      ;/* 3C */    
         dd SRC_PAT_SRC_3_BLT,  DSon	;,   PDo,    SDx    ;/* 3D */    
         dd SRC_PAT_SRC_3_BLT,  DSna	;,   PDo,    SDx    ;/* 3E */    
         dd SRC_PAT_2_BLT,      S		;,      PDan,   0      ;/* 3F */    
         dd SRC_PAT_2_BLT,      SDna	;,   PDa,    0      ;/* 40 */    
         dd ENG_DOWNLOAD_2_BLT, PSx		;,    DSon,   0      ;/* 41 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 42 */    
         dd SRC_PAT_SRC_3_BLT,  DSan	;,   PDa,    SDxn   ;/* 43 */    
         dd SRC_FILL_1_BLT,     SDna	;,   0,      0      ;/* 44 */    
         dd ENG_DOWNLOAD_2_BLT, PSna	;,   DSon,   0      ;/* 45 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 46 */    
         dd PAT_SRC_PAT_3_BLT,  DPx		;,    SDa,    PDxn   ;/* 47 */    
         dd PAT_SRC_2_BLT,      DPx		;,    SDa,    0      ;/* 48 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 49 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 4A */    
         dd SRC_PAT_2_BLT,      DSno	;,   PDx,    0      ;/* 4B */    
         dd PAT_SRC_2_BLT,      DPan	;,   SDa,    0      ;/* 4C */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 4D */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 4E */    
         dd SRC_PAT_2_BLT,      DSno	;,   PDan,   0      ;/* 4F */    
         dd PAT_FILL_1_BLT,     PDna	;,   0,      0      ;/* 50 */    
         dd ENG_DOWNLOAD_2_BLT, SPna	;,   DSon,   0      ;/* 51 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 52 */    
         dd SRC_PAT_SRC_3_BLT,  DSx		;,    PDa,    SDxn   ;/* 53 */    
         dd ENG_DOWNLOAD_2_BLT, PSo		;,    SDna,   0      ;/* 54 */    
         dd DESTINVERT_BLT,     Dn		;,     0,      0      ;/* 55 */    
         dd ENG_DOWNLOAD_2_BLT, PSo		;,    DSx,    0      ;/* 56 */    
         dd ENG_DOWNLOAD_2_BLT, PSo		;,    DSan,   0      ;/* 57 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 58 */    
         dd ENG_DOWNLOAD_2_BLT, PSno	;,   DSx,    0      ;/* 59 */    
         dd PAT_FILL_1_BLT,     DPx		;,    0,      0      ;/* 5A */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 5B */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 5C */    
         dd ENG_DOWNLOAD_2_BLT, PSno	;,   DSan,   0      ;/* 5D */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 5E */    
         dd PAT_FILL_1_BLT,     DPan	;,   0,      0      ;/* 5F */    
         dd SRC_PAT_2_BLT,      DSx		;,    PDa,    0      ;/* 60 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 61 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 62 */    
         dd PAT_SRC_2_BLT,      DPno	;,   SDx,    0      ;/* 63 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 64 */    
         dd ENG_DOWNLOAD_2_BLT, SPno	;,   DSx,    0      ;/* 65 */    
         dd SRC_FILL_1_BLT,     DSx		;,    0,      0      ;/* 66 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 67 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 68 */    
         dd SRC_PAT_2_BLT,      DSx		;,    PDxn,   0      ;/* 69 */    
         dd ENG_DOWNLOAD_2_BLT, PSa		;,    DSx,    0      ;/* 6A */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 6B */    
         dd PAT_SRC_2_BLT,      DPa		;,    SDx,    0      ;/* 6C */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 6D */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 6E */    
         dd SRC_PAT_2_BLT,      DSxn	;,   PDan,   0      ;/* 6F */    
         dd SRC_PAT_2_BLT,      DSan	;,   PDa,    0      ;/* 70 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 71 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 72 */    
         dd PAT_SRC_2_BLT,      DPno	;,   SDan,   0      ;/* 73 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 74 */    
         dd ENG_DOWNLOAD_2_BLT, SPno	;,   DSan,   0      ;/* 75 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 76 */    
         dd SRC_FILL_1_BLT,     DSan	;,   0,      0      ;/* 77 */    
         dd SRC_PAT_2_BLT,      DSa		;,    PDx,    0      ;/* 78 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 79 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 7A */    
         dd PAT_SRC_2_BLT,      DPxn	;,   SDan,   0      ;/* 7B */    
         dd SRC_PAT_SRC_3_BLT,  DSno	;,   PDa,    SDx    ;/* 7C */    
         dd ENG_DOWNLOAD_2_BLT, PSxn	;,   DSan,   0      ;/* 7D */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 7E */    
         dd ENG_DOWNLOAD_2_BLT, PSa		;,    DSan,   0      ;/* 7F */    
         dd ENG_DOWNLOAD_2_BLT, PSa		;,    DSa,    0      ;/* 80 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 81 */    
         dd ENG_DOWNLOAD_2_BLT, PSx		;,    DSna,   0      ;/* 82 */    
         dd SRC_PAT_SRC_3_BLT,  DSno	;,   PDa,    SDxn   ;/* 83 */    
         dd PAT_SRC_2_BLT,      DPxn	;,   SDa,    0      ;/* 84 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 85 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 86 */    
         dd SRC_PAT_2_BLT,      DSa		;,    PDxn,   0      ;/* 87 */    
         dd SRC_FILL_1_BLT,     DSa		;,    0,      0      ;/* 88 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 89 */    
         dd ENG_DOWNLOAD_2_BLT, SPno	;,   DSa,    0      ;/* 8A */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 8B */    
         dd PAT_SRC_2_BLT,      DPno	;,   SDa,    0      ;/* 8C */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 8D */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 8E */    
         dd SRC_PAT_2_BLT,      DSan	;	,   PDan,   0      ;/* 8F */    
         dd SRC_PAT_2_BLT,      DSxn	;,   PDa,    0      ;/* 90 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 91 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 92 */    
         dd PAT_SRC_2_BLT,      PDa		;,    SDxn,   0      ;/* 93 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 94 */    
         dd ENG_DOWNLOAD_2_BLT, PSa		;,    DSxn,   0      ;/* 95 */    
         dd SRC_PAT_2_BLT,      DSx		;,    PDx,    0      ;/* 96 DPSxx == PDSxx */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 97 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 98 */    
         dd SRC_FILL_1_BLT,     DSxn	;,   0,      0      ;/* 99 */    
         dd ENG_DOWNLOAD_2_BLT, PSna	;,   DSx,    0      ;/* 9A */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 9B */    
         dd PAT_SRC_2_BLT,      PDna	;,   SDx,    0      ;/* 9C */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 9D */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* 9E */    
         dd SRC_PAT_2_BLT,      DSx		;,    PDan,   0      ;/* 9F */    
         dd PAT_FILL_1_BLT,     DPa		;,    0,      0      ;/* A0 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* A1 */    
         dd ENG_DOWNLOAD_2_BLT, PSno	;,   DSa,    0      ;/* A2 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* A3 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* A4 */    
         dd PAT_FILL_1_BLT,     PDxn	;,   0,      0      ;/* A5 */    
         dd ENG_DOWNLOAD_2_BLT, SPna	;,   DSx,    0      ;/* A6 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* A7 */    
         dd ENG_DOWNLOAD_2_BLT, PSo		;,    DSa,    0      ;/* A8 */    
         dd ENG_DOWNLOAD_2_BLT, PSo		;,    DSxn,   0      ;/* A9 */    
         dd DEST_BLT,           D		;,      0,      0      ;/* AA */    
         dd ENG_DOWNLOAD_2_BLT, PSo		;,    DSno,   0      ;/* AB */    
         dd SRC_PAT_SRC_3_BLT,  DSx		;,    PDa,    SDx    ;/* AC */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* AD */    
         dd ENG_DOWNLOAD_2_BLT, SPna	;,   DSo,    0      ;/* AE */    
         dd PAT_FILL_1_BLT,     DPno	;,   0,      0      ;/* AF */    
         dd SRC_PAT_2_BLT,      DSno	;,   PDa,    0      ;/* B0 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* B1 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* B2 */    
         dd PAT_SRC_2_BLT,      DPan	;,   SDan,   0      ;/* B3 */    
         dd SRC_PAT_2_BLT,      SDna	;,   PDx,    0      ;/* B4 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* B5 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* B6 */    
         dd PAT_SRC_2_BLT,      DPx		;,    SDan,   0      ;/* B7 */    
         dd PAT_SRC_PAT_3_BLT,  DPx		;,    SDa,    PDx    ;/* B8 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* B9 */    
         dd ENG_DOWNLOAD_2_BLT, PSna	;,   DSo,    0      ;/* BA */    
         dd SRC_FILL_1_BLT,     DSno	;,   0,      0      ;/* BB */    
         dd SRC_PAT_SRC_3_BLT,  DSan	;,   PDa,    SDx    ;/* BC */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* BD */    
         dd ENG_DOWNLOAD_2_BLT, PSx		;,    DSo,    0      ;/* BE */    
         dd ENG_DOWNLOAD_2_BLT, PSa		;,    DSno,   0      ;/* BF */    
         dd SRC_PAT_2_BLT,      S		;,      PDa,    0      ;/* C0 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* C1 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* C2 */    
         dd SRC_PAT_2_BLT,      S		;,      PDxn,   0      ;/* C3 */    
         dd PAT_SRC_2_BLT,      PDno	;,   SDa,    0      ;/* C4 */    
         dd SRC_PAT_SRC_3_BLT,  DSx		;,    PDo,    SDxn   ;/* C5 */    
         dd PAT_SRC_2_BLT,      DPna	;,   SDx,    0      ;/* C6 */    
         dd PAT_SRC_PAT_3_BLT,  DPo		;,    SDa,    PDxn   ;/* C7 */    
         dd PAT_SRC_2_BLT,      DPo		;,    SDa,    0      ;/* C8 */    
         dd PAT_SRC_2_BLT,      PDo		;,    SDxn,   0      ;/* C9 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* CA */    
         dd SRC_PAT_SRC_3_BLT,  DSa		;,    PDo,    SDxn   ;/* CB */    
         dd SRCCOPY_BLT,        S		;,      0,      0      ;/* CC SRCCOPY */    
         dd PAT_SRC_2_BLT,      DPon	;,   SDo,    0      ;/* CD */    
         dd PAT_SRC_2_BLT,      DPna	;,   SDo,    0      ;/* CE */    
         dd SRC_PAT_2_BLT,      S		;,      DPno,   0      ;/* CF */    
         dd SRC_PAT_2_BLT,      SDno	;,   PDa,    0      ;/* D0 */    
         dd PAT_SRC_PAT_3_BLT,  DPx		;,    SDo,    PDxn   ;/* D1 */    
         dd SRC_PAT_2_BLT,      DSna	;,   PDx,    0      ;/* D2 */    
         dd SRC_PAT_SRC_3_BLT,  DSo		;,    PDa,    SDxn   ;/* D3 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* D4 */    
         dd ENG_DOWNLOAD_2_BLT, PSan	;,   DSan,   0      ;/* D5 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* D6 */    
         dd ENG_DOWNLOAD_2_BLT, PSx		;,    DSan,   0      ;/* D7 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* D8 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* D9 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* DA */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* DB */    
         dd PAT_SRC_2_BLT,      PDna	;,   SDo,    0      ;/* DC */    
         dd SRC_FILL_1_BLT,     SDno	;,   0,      0      ;/* DD */    
         dd PAT_SRC_2_BLT,      DPx		;,    SDo,    0      ;/* DE */    
         dd ENG_DOWNLOAD_2_BLT, DPan	;,   SDo,    0      ;/* DF */    
         dd SRC_PAT_2_BLT,      DSo		;,    PDa,    0      ;/* E0 */    
         dd SRC_PAT_2_BLT,      DSo		;,    PDxn,   0      ;/* E1 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* E2 DSPDxax : XXX S3 special case*/    s this */
         dd PAT_SRC_PAT_3_BLT,  DPa		;,    SDo,    PDxn   ;/* E3 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* E4 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* E5 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* E6 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* E7 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* E8 */    
         dd ENGINE_BITBLT,      0		;,      0,      0      ;/* E9 */    
         dd ENG_DOWNLOAD_2_BLT, PSa		;,    DSo,    0      ;/* EA */    
         dd ENG_DOWNLOAD_2_BLT, PSx		;,    DSno,   0      ;/* EB */    
         dd PAT_SRC_2_BLT,      DPa		;,    SDo,    0      ;/* EC */    
         dd PAT_SRC_2_BLT,      DPxn	;,   SDo,    0      ;/* ED */    
         dd SRC_FILL_1_BLT,     DSo		;,    0,      0      ;/* EE */    
         dd SRC_PAT_2_BLT,      SDo		;,    DPno,   0      ;/* EF */    
         dd PATCOPY_BLT,        P		;,      0,      0      ;/* F0 PATCOPY */    
         dd SRC_PAT_2_BLT,      DSon	;,   PDo,    0      ;/* F1 */    
         dd SRC_PAT_2_BLT,      DSna	;,   PDo,    0      ;/* F2 */    
         dd SRC_PAT_2_BLT,      S		;,      PDno,   0      ;/* F3 */    
         dd SRC_PAT_2_BLT,      SDna	;,   PDo,    0      ;/* F4 */    
         dd PAT_FILL_1_BLT,     PDno	;,   0,      0      ;/* F5 */    
         dd SRC_PAT_2_BLT,      DSx		;,    PDo,    0      ;/* F6 */    
         dd SRC_PAT_2_BLT,      DSan	;,   PDo,    0      ;/* F7 */    
         dd SRC_PAT_2_BLT,      DSa		;,    PDo,    0      ;/* F8 */    
         dd SRC_PAT_2_BLT,      DSxn	;,   PDo,    0      ;/* F9 */    
         dd PAT_FILL_1_BLT,     DPo		;,    0,      0      ;/* FA */    
         dd SRC_PAT_2_BLT,      DSno	;,   PDo,    0      ;/* FB */    
         dd SRC_PAT_2_BLT,      S		;,      PDo,    0      ;/* FC */    
         dd SRC_PAT_2_BLT,      SDno	;,   PDo,    0      ;/* FD */    
         dd ENG_DOWNLOAD_2_BLT, PSo		;,    DSo,    0      ;/* FE */    
         dd WHITENESS_BLT,      set		;,    0,      0      ;/* FF WHITENESS */    

PUBLIC rop2Table
rop2Table   dd 0            ; Unused (Rop2 from 1-16!)
            dd unset        ;/* 00 BLACKNESS */    
            dd DSon	        ;/* 11 */    
            dd DSna	        ;/* 22 */    
            dd Sn		    ;/* 33 */    
            dd SDna	        ;/* 44 */    
            dd Dn		    ;/* 55 */    
            dd DSx		    ;/* 66 */    
            dd DSan	        ;/* 77 */    
            dd DSa		    ;/* 88 */    
            dd DSxn	        ;/* 99 */    
            dd D		    ;/* AA */    
            dd DSno	        ;/* BB */    
            dd S		    ;/* CC SRCCOPY */    
            dd SDno	        ;/* DD */    
            dd DSo		    ;/* EE */    
            dd set		    ;/* FF WHITENESS */    

; Tables of mask values used by PermediaCopyBlt routines.
;
pcb_leftMasks	label	DWORD
		dd	000000000h
		dd	0ffffff00h
		dd	0ffff0000h
		dd	0ff000000h

pcb_rightMasks	label	DWORD
		dd	000000000h
		dd	0000000ffh
		dd	00000ffffh
		dd	000ffffffh

fifoSpace	dd	0

; Enabled Logical Ops
SopDtoDopS db 0
    db  __GLINT_LOGICOP_CLEAR           AND 0FFh,0  ; 0
    db  __GLINT_LOGICOP_AND             AND 0FFh,0  ; S&D
    db  __GLINT_LOGICOP_AND_INVERTED    AND 0FFh,0  ; ~S&D
    db  __GLINT_LOGICOP_NOOP            AND 0FFh,0  ; D
    db  __GLINT_LOGICOP_AND_REVERSE     AND 0FFh,0  ; S&~D
    db  __GLINT_LOGICOP_COPY            AND 0FFh,0  ; S (7+ROP_REQ_SRC)
    db  __GLINT_LOGICOP_XOR             AND 0FFh,0  ; S^D
    db  __GLINT_LOGICOP_OR              AND 0FFh,0  ; S|D
    db  __GLINT_LOGICOP_NOR             AND 0FFh,0  ; ~(S|D)
    db  __GLINT_LOGICOP_EQUIV           AND 0FFh,0  ; ~(S^D)
    db  __GLINT_LOGICOP_COPY_INVERT     AND 0FFh,0  ; ~S
    db  __GLINT_LOGICOP_OR_INVERT       AND 0FFh,0  ; ~S|D
    db  __GLINT_LOGICOP_INVERT          AND 0FFh,0  ; ~D
    db  __GLINT_LOGICOP_OR_REVERSE      AND 0FFh,0  ; S|~D
    db  __GLINT_LOGICOP_NAND            AND 0FFh,0  ; ~(S&D)
    db  __GLINT_LOGICOP_SET             AND 0FFh,0  ; 1


_INIT   ends

;----------------------------------------------------------------------------
;			       GLINT
;----------------------------------------------------------------------------

_BLIT32	segment page

CursorExclude PROC FAR PASCAL, xPos:DWORD, yPos:DWORD, xWidth:DWORD, yHeight:DWORD
	and		ebp,0ffffh
    push    ds
    push    gs
    push    esi
    push    edi
    mov     eax,_INIT
    mov     ds, eax
	assumes	ds,_INIT
	mov	    eax,RemappedDataSegment
	mov	    ds,eax
;
;We need to set the BUSY bit in the destination PDevice structure so that
;the DIB engine doesn't try to asynchronously draw the cursor while we're
;using the hardware to BLT.
;
    xor     ebx, ebx
	lgs	    bx,GLInfo.lpDriverPDevice
    mov     esi, xWidth
	mov     ecx,xPos                 
    mov     edi, yHeight
	mov     edx,yPos
	or	    gs:[ebx].deFlags,BUSY

	dec	    esi  	                ;Make the extents inclusive of the
	dec	    edi		                ;last point
	add     esi,ecx                 ;Set right
	add     edi,edx                 ;Set bottom

	push    GLInfo.lpDriverPDevice         ;PDevice
	push    cx                      ;Left
	push    dx                      ;Top
	push    si                      ;Right
	push    di                      ;Bottom
	push    word ptr CURSOREXCLUDE           ;Flags
	call	BeginAccess 	        ;returns with flags in ax.

	mov	    DIBAccessCallFlag,'B'	;set the DIBAccessCallFlag
    pop     edi
    pop     esi
	pop	    gs		                ;restore GS --> Data
    pop     ds
	ret
CursorExclude  ENDP 


	BEST_ALIGN

PLABEL Jmp_DIB_BitBlt
    pop     edi
    pop     esi
    pop     ds
    mov     sp,bp					; transfer only the bottom 16 bits
    pop     ebp

	StackMangle32To16				; convert the 32:32 ret addr back to 16:16
	
	jmp	DIB_BitBlt

;----------------------------------------------------------------------------
; InitialiseBitBlt - Set up all the function pointers for blt operations
;----------------------------------------------------------------------------
InitialiseBitBlt PROC FAR C PUBLIC

    push    eax
    push    ds

    mov     eax,_INIT
    mov     ds, eax
	assumes	ds,_INIT
	mov	    eax,RemappedDataSegment
	mov	    ds,eax

    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    ; Generic setup using DIB engine for all functions
    mov     MemCopyFn,              OFFSET MemCopyInit
    mov     FillRectFn,             OFFSET Jmp_DIB_BitBlt    
    mov     GreyFillRectFn,         OFFSET Jmp_DIB_BitBlt    
    mov     ROPFillRectFn,          OFFSET Jmp_DIB_BitBlt
    mov     PatCopyColorBrushBltFn, OFFSET Jmp_DIB_BitBlt
    mov     PatCopyMonoBrushBltFn,  OFFSET Jmp_DIB_BitBlt
    mov     CopyBltFn,				OFFSET Jmp_DIB_BitBlt    
    mov     ROPCopyBltFn,           OFFSET Jmp_DIB_BitBlt    
    mov     MonoImageDownloadFn,    OFFSET Jmp_DIB_BitBlt    
    mov     MonoROPImageDownloadFn, OFFSET Jmp_DIB_BitBlt    
    mov     ImageDownloadFn,        OFFSET Jmp_DIB_BitBlt    
    mov     ImageUploadFn,          OFFSET Jmp_DIB_BitBlt    
    mov     ROPColorBrushBltFn,     OFFSET Jmp_DIB_BitBlt
    mov     ROPMonoBrushBltFn,      OFFSET Jmp_DIB_BitBlt
    mov     ROPImageDownloadFn,     OFFSET Jmp_DIB_BitBlt    
    mov     DestMemoryFn,           OFFSET Jmp_DIB_BitBlt    

	mov		FillRectCmd,			48h
	mov		P2FillRectCmd,			6000C8h
	mov	    BrushFillRectCmd,		848h
	mov		ROPFillRectCmd, 		40h
	mov		SolidMonoBrushCmd,		41h
	mov		ROPMonoBrushCmd,		41h
	mov		CopyBltCmd,				40h
	mov		CopyBltSlowCmd,			40h    
	mov		ImageUploadCmd,			40h    
    mov     P2MonoROPImageDownloadCmd, 6008C0h

	cmp		byte ptr wBpp,24                ; 24bpp is special as it doesnt have a generally
	je		IB_24bpp                        ; usable block fill. Should only take this jump on P2

    ; Generic setup for all chips
    mov     FillRectFn,             OFFSET FillRect    
    mov     GreyFillRectFn,         OFFSET FillRect    
    mov     ROPFillRectFn,          OFFSET ROPFillRect
    mov     CopyBltFn,				OFFSET glintCopyBlt    
    mov     ROPCopyBltFn,           OFFSET glintCopyBlt    

    mov     eax, GLInfo.dwRenderChipID
    cmp     eax, GLINT500TX_ID
    je      IB_GLINTTX
    cmp     eax, GLINTMX_ID
    je      IB_GLINTTX
    cmp     eax, PERMEDIA_ID
    je      IB_PERMEDIA
    cmp     eax, TIPERMEDIA_ID
    je      IB_PERMEDIA
    cmp     eax, PERMEDIA2_ID
    je      IB_PERMEDIA2
    cmp     eax, TIPERMEDIA2_ID
    je      IB_PERMEDIA2

    jmp IB_Done

IB_GLINTTX:
; We are either a TX or an MX
    mov     ImageDownloadFn,        OFFSET glintImageDownloadTX
	mov		CopyBltCmd,				40048h
   	mov		ROPFillRectCmd, 		40048h

    jmp IB_Done

IB_PERMEDIA:
    ; Assume it is a Permedia!
    ;
    mov     PatCopyMonoBrushBltFn,  OFFSET permediaSolidMonoBrush
    mov     ROPMonoBrushBltFn,      OFFSET permediaROPMonoBrush
    mov     ImageDownloadFn,        OFFSET permediaImageDownload
    mov     ImageUploadFn,          OFFSET permediaImageUpload

    cmp		byte ptr wBpp,32
    je		IB_Done				; No optimization possible for 32bit.

    ; Use packed ROP function for 8 and 16bpp
    mov     ROPFillRectFn,          OFFSET permediaROPFillRect

	cmp		byte ptr wBpp,8
	jne		IBP_16bpp

    ; 8Bpp Depth
    ;
    mov     PatCopyColorBrushBltFn, OFFSET permediaBrushFillRect8

	cmp		wScreenWidth,800
	je		@f		            ; Fast routines don't work for 800x600.

	mov     ROPCopyBltFn,			OFFSET permediaROPCopyBlt8
	mov     CopyBltFn,				OFFSET permediaCopyBlt8Bpp
	jmp		IB_Done

@@:	mov     CopyBltFn,			    OFFSET permediaCopyBlt
    jmp     IB_Done

    ; 16Bpp Depth
    ;
IBP_16bpp:
	mov     ROPCopyBltFn,			OFFSET permediaROPCopyBlt16
	cmp		wScreenWidth,800
	je		@f         	        ; Fast routines don't work for 800x600.

    mov     CopyBltFn,			    OFFSET permediaCopyBlt16Bpp
	jmp		IB_Done

@@:	mov     CopyBltFn,			    OFFSET permediaCopyBlt
    jmp IB_Done

IB_PERMEDIA2:
    ; Assume it is a P2
    ;
	mov	    P2BrushFillRectCmd,		6228C8h

    mov     FillRectFn,             OFFSET P2FillRect    
    mov     PatCopyMonoBrushBltFn,  OFFSET permediaSolidMonoBrush
    mov     ROPMonoBrushBltFn,      OFFSET permediaROPMonoBrush
    mov     ImageUploadFn,          OFFSET permediaImageUpload
    mov     MonoImageDownloadFn,    OFFSET P2MonoROPImageDownload    
    mov     MonoROPImageDownloadFn, OFFSET P2MonoROPImageDownload   
    
    ; The DIB engine is faster at download if we are write combined. 
    test    GLInfo.dwFlags, GMVF_FRAME_BUFFER_IS_WC
    jne     @f
    mov     ImageDownloadFn,        OFFSET permediaImageDownload
@@:
	cmp		GLInfo.dw2D_DMA_Phys, 0		; Has a DMA buffer been allocated
	je		@f
    mov     ImageUploadFn,          OFFSET P2ImageUpload
@@:

    ; Use packed ROP function for 8 and 16bpp
    mov     ROPFillRectFn,          OFFSET permediaROPFillRect
    mov     CopyBltFn,			    OFFSET P2CopyBlt

	cmp		byte ptr wBpp,8
	je		IB_P2_8bpp
	cmp		byte ptr wBpp,16
	je		IB_P2_16bpp
    cmp		byte ptr wBpp,32
    je		IB_P2_32bpp

    jmp     IB_Done


IB_P2_8bpp:
    ; Use packed ROP function for 8 and 16bpp
    mov     ROPFillRectFn,          OFFSET permediaROPFillRect
    mov     CopyBltFn,			    OFFSET P2CopyBlt


	mov     ROPCopyBltFn,			OFFSET P2ROPCopyBlt8
    mov     PatCopyColorBrushBltFn, OFFSET P2BrushFillRect8
	mov	    P2BrushFillRectCmd,		6020C8h ; No sync on bitmask at 8bpp
    jmp     IB_Done

IB_P2_16bpp:
    ; Use packed ROP function for 8 and 16bpp
	mov     ROPCopyBltFn,			OFFSET P2ROPCopyBlt16
    mov     ROPFillRectFn,          OFFSET permediaROPFillRect
    mov     CopyBltFn,			    OFFSET P2CopyBlt

    mov     PatCopyColorBrushBltFn, OFFSET P2BrushFillRect16
    jmp     IB_Done

IB_P2_32bpp:
    mov     PatCopyColorBrushBltFn, OFFSET P2BrushFillRect32
    jmp     IB_Done

IB_24bpp:
; Standard block fill can only be used for shades of grey in 24bpp. Special functions
; for some other common operations.
    mov     FillRectFn,             OFFSET FillRect24bpp
    mov     GreyFillRectFn,         OFFSET FillRect
    mov     CopyBltFn,			    OFFSET P2CopyBlt24bpp
    mov     ROPCopyBltFn,           OFFSET glintCopyBlt    
    jmp     IB_Done

IB_Done:
    pop     ds
    pop     eax
    ret
InitialiseBitBlt    endp


;----------------------------------------------------------------------------
; BltSpecial_GLINT
;----------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  BitBlt16

	StackMangle16To32

    ;lpDestDev:DWORD,                ;--> to destination bitmap descriptor
    ;DestxOrg:WORD,                  ;Destination origin - x coordinate
    ;DestyOrg:WORD,                  ;Destination origin - y coordinate
    ;lpSrcDev:DWORD,                 ;--> to source bitmap descriptor
    ;SrcxOrg:WORD,                   ;Source origin - x coordinate
    ;SrcyOrg:WORD,                   ;Source origin - y coordinate
    ;xExt:WORD,                      ;x extent of the BLT
    ;yExt,WORD,                      ;y extent of the BLT
    ;Rop:DWORD,                      ;Raster operation descriptor
    ;lpPBrush:DWORD,                 ;--> to a physical brush (pattern)
	;lpDrawMode:DWORD				;--> to a drawmode

BitBltParms	STRUCT
	_ebp		DWORD	0
	_retLo		DWORD	0
	_retHi		DWORD	0

	lpDrawMode	DWORD	0
	lpPBrush	DWORD	0
	Rop			DWORD	0
	yExt		WORD	0
	xExt		WORD	0
	SrcyOrg		WORD	0
	SrcxOrg		WORD	0
	lpSrcDev	DWORD	0
	DestyOrg	WORD	0
	DestxOrg	WORD	0
	lpDestDev	DWORD	0
BitBltParms	ENDS

; Used by P2MonoROPImageDownload
dwSegmentOffset         equ nTemp1
dwLinesInSegment        equ nTemp2
dwLinesLeftInSegment    equ nTemp3
dwFirstInNextSegment    equ nTemp4

; Used by PermediaCopyBlt
wgx0 equ nTemp1
wgx1 equ nTemp2
wgx2 equ nTemp3
wgx3 equ nTemp4
wdx  equ nTemp5

; Used by PermediaImageUpload
Fn1  equ pTemp1              
Fn2  equ pTemp2

BitBlt32	PROC FAR PASCAL PUBLIC		;remember that USES would override "ret" code...
	LOCAL	DestHeight:DWORD,
		    RopFunction:DWORD,     
		    dwSrcWords:DWORD,
		    dwPMask0:DWORD,
		    dwPMask1:DWORD,
		    nDwords:DWORD,
		    holdTag:DWORD,
		    wLeft16:DWORD,
            dwDelta:DWORD,
            nTemp1:DWORD,
            nTemp2:DWORD,
            nTemp3:DWORD,
            nTemp4:DWORD,
            nTemp5:DWORD,
            pTemp1:DWORD,
            pTemp2:DWORD

	and		ebp,0ffffh
	push	ds
	push	esi					;save 32 bit versions of these
	push	edi					;

    mov     eax,_INIT
    mov     ds, eax
	assumes	ds,_INIT
	mov	    edx,RemappedDataSegment
    assumes ds,_INIT			;BUT: ds doesn't get set up until we know
    assumes es,nothing			;that we'll handle this, not the dib engine
    assumes fs,nothing
    assumes gs,nothing

	xor		ebx,ebx
	xor		esi,esi
	lfs		si, [ebp].lpDestDev			    ;
    mov     eax,dword ptr fs:[esi].deFlags  ;is destination the screen?
    mov		bl,bptr [ebp].Rop+2		        ;Get the raster op code.

	cmp	    fs:[esi].deType,TYPE_DIBENG     ;is this a DIBENGINE surface?
    jne     Jmp_DIB_BitBlt

    mov     ecx,dword ptr fs:[esi].deDriverReserved  ; Get Minidriver flags
    test    eax,BUSY                        ;disabled ?
    jnz     Jmp_DIB_BitBlt                  ;if so, punt

	push	edx
	pop		ds
    test    eax,VRAM                        ;is destination the screen?
    je      DestinationIsMemory		        
    test    ecx,ACCELERATED                 ;is destination accelerated?
    je      DestinationIsMemory		        

PLABEL DestinationIsScreen
	;pair the next two and therefor getthe second for free
    test    eax,PALETTE_XLAT         ;is there a translate to perform ?
    lea     ebx, [ropTable + ebx * 8] ; ebx points to ROP entry (8 bytes per entry)

	;this one is a long (unpairable) jump
    jne     XLAT_BitBlt             ;If so, call the special translation proc.
    
    jmp     [ebx]                   ; Jump to jump point for this ROP

	BEST_ALIGN

PLABEL  WhitenessBlt
    mov		eax, 0ffffffffh         ;We have whiteness logic op
    jmp     [GreyFillRectFn]

	BEST_ALIGN

PLABEL  ROP2SrcCopyBlt
	; We either have a ROP Image Download or screen to screen
	xor		edi,edi
	les		di, [ebp].lpSrcDev
    mov     eax, [ebx+4]                    ;Get Glint Logical Op
    mov     ecx, dword ptr es:[edi].deFlags ;ecx = src bitmap flags.
    mov	    RopFunction,eax                 ;save Glint ROP function locally

	cmp	    es:[edi].deType,TYPE_DIBENG ;is this a DIBENGINE surface?
    jne     ROP2SrcCopyBlt_NotDIBENG

    test    ecx,VRAM
    jz      ROP2SrcCopyBlt_NotOffscreen     ;j if source is not accelerated
    mov     ecx, dword ptr es:[edi].deDriverReserved;ecx = src driver flags.
    test    ecx,ACCELERATED
    jz      ROP2SrcCopyBlt_NotOffscreen     ;j if source is not accelerated
	jmp		[ROPCopyBltFn]                  ;Screen to screen blt

ROP2SrcCopyBlt_NotOffscreen:
    mov     dl, es:[edi].deBitsPixel        ;Get bitmap bpp
    mov     bl, byte ptr wBpp									
    cmp     dl,1                            ;mono source?
    je      DoROPMonoImageDownload          ;Call the correct Mono Fn if so
    cmp     dl,bl                           ;Is the bitmap the same depth as the screen?
    jne     Jmp_DIB_BitBlt                  ;Get the DIB Engine to do it if not
    jmp     [ROPImageDownloadFn]	        ;do the download

PLABEL	DoROPMonoImageDownload
    jmp     [MonoROPImageDownloadFn]        ;Call the correct Mono Fn if so

PLABEL ROP2SrcCopyBlt_NotDIBENG
	cmp	    es:[edi].bmType,0               ;is this a PBITMAP surface?
    jne     Jmp_DIB_BitBlt                  ;if not, pass to the DIB
    mov     dl, es:[edi].bmBitsPixel        ;Get bitmap bpp
    cmp     dl, 1
    je      ROP2SrcCopyBlt_NotOffscreen     ;accept all monochrome PBITMAPS
ifdef DEBUG
    cmp     es:[edi].bmSegmentIndex,0
    je      @f
    int     1
@@:
endif
    cmp     es:[edi].bmSegmentIndex,0       ;only single segment colour PBITMAPS
    jne     Jmp_DIB_BitBlt
    jmp     ROP2SrcCopyBlt_NotOffscreen

	BEST_ALIGN

PLABEL  SrcCopyBlt
	; We either have a source copy Image Download or screen to screen
	
	xor		edi,edi
	les		di, [ebp].lpSrcDev
    mov     ecx,dword ptr es:[edi].deFlags  ;cx = src pdevice flags.
    mov	    RopFunction,ROP_REQ_SRC         ;save Glint ROP function locally
	cmp	    es:[edi].deType,TYPE_DIBENG     ;is this a DIBENGINE surface?
    jne     SrcCopyBlt_NotDIBENG

    test    ecx,VRAM
    jz      SrcCopyBlt_NotOffscreen         ;j if source is not accelerated
    mov     ecx,dword ptr es:[edi].deDriverReserved      ;cx = src pdevice flags.
    test    ecx,ACCELERATED
    jz      SrcCopyBlt_NotOffscreen         ;j if source is not accelerated
	jmp		[CopyBltFn]                     ;Screen to screen blt

SrcCopyBlt_NotOffscreen:
    mov     dl, es:[edi].deBitsPixel        ;Get bitmap bpp
    mov     bl, byte ptr wBpp
    cmp     dl,1                            ;mono source?
    je      DoMonoImageDownload             ;Call the correct Mono Fn if so
    cmp     dl,bl                           ;Is the bitmap the same depth as the screen?
    jne     Jmp_DIB_BitBlt                  ;Get the DIB Engine to do it if not
    jmp     [ImageDownloadFn]		        ;do the download

PLABEL	DoMonoImageDownload
    jmp     [MonoImageDownloadFn]           ;Call the correct Mono Fn if so

PLABEL SrcCopyBlt_NotDIBENG
	cmp	    es:[edi].bmType,0               ;is this a PBITMAP surface?
    jne     Jmp_DIB_BitBlt                  ;if not, pass to the DIB
    mov     dl, es:[edi].bmBitsPixel        ;Get bitmap bpp
    cmp     dl, 1
    je      SrcCopyBlt_NotOffscreen         ;accept all monochrome PBITMAPS
ifdef DEBUG
    cmp     es:[edi].bmSegmentIndex,0
    je      @f
    int     1
@@:
endif
    cmp     es:[edi].bmSegmentIndex,0       ;only colour if single segment
    jne     Jmp_DIB_BitBlt
    jmp     SrcCopyBlt_NotOffscreen

	BEST_ALIGN
PLABEL  BlacknessBlt
    xor     eax, eax                ;We have Blackness logic op
    jmp     [GreyFillRectFn]

	BEST_ALIGN
PLABEL  PatCopyBlt
    ; Check for a solid brush, mono brush and color brush

	xor		edi,edi
	mov		dl,byte ptr wBpp
    les     di,[ebp].lpPBrush             ;ds:si points to the physical brush
    mov     bl,byte ptr es:[edi].dp8BrushStyle;
    mov     cl,es:[edi].dp8BrushFlags;bl style, cl flag
    mov     eax,dword ptr es:[edi].dp8BrushBits ;V get color
									;
    cmp     bl,BS_SOLID             ;solid brush
    jne     DoPatCopyBrushBlt       ;Goto brush blt if not
    test    cl,COLORSOLID           ;solid color?
    je      DoPatCopyBrushBlt

PLABEL  DoSolidPatternBlt
    cmp     dl,24					;Replicate colour to 32 bits
    jae     DoPatCopyFillRect       ;skip replicate if 24 or 32 bits
    cmp     dl, 8
    jne     @f
    mov     ah, al                  ;8bpp is now replicated to 16bpp
@@:
    mov     ecx,eax                 ;double up from 16 bits to 32 bits
	and		ecx,0ffffh
    shl     eax, 16
    or		eax, ecx
PLABEL DoPatCopyFillRect
    jmp     [FillRectFn]		

	BEST_ALIGN

PLABEL DoPatCopyBrushBlt			;es:di = lpPBrush
    cmp     bl,BS_PATTERN           ;pattern brush?
    jne     DoBrushBltHollowHatched
	and		cl, MONO_VALID or MONO_PATTERN
	cmp 	cl, MONO_VALID or MONO_PATTERN			;is the mono part of the brush instantiated?
	jnz		@f
	jmp	    [PatCopyMonoBrushBltFn]
@@:
	jmp	    [PatCopyColorBrushBltFn]

DoBrushBltHollowHatched:
    cmp     bl,BS_HOLLOW
    je      BS_Done
    jmp     Jmp_DIB_BitBlt          ;let DIB handle it if brush is HOLLOW or HATCHED

	BEST_ALIGN

PLABEL ROPBrushBlt
    ; Hopefully we now have some kind of ROP brush with a source.
    ; Check for a solid brush, mono brush and color brush
	xor		edi,edi
    mov     eax, [ebx+4]            ;Get Glint Logical Op
    les     di,[ebp].lpPBrush       ;ds:si points to the physical brush
    mov	    RopFunction,eax         ;save Glint ROP function locally
    mov     cl,es:[edi].dp8BrushFlags;bl style, cl flag
    mov     ebx,dword ptr es:[edi].dp8BrushStyle;
	mov		dl,byte ptr wBpp
    mov     eax,dword ptr es:[edi].dp8BrushBits ; get color
									;
    cmp     bl,BS_SOLID             ;solid brush
    jne     DoROPBrushBlt           ;Goto brush blt if not
    test    cl,COLORSOLID           ;solid color?
    je      DoROPBrushBlt

PLABEL  DoROPSolidPatternBlt
    cmp     dl,24					;Replicate colour to 32 bits  
    jae     DoROPFillRect           ;skip replicate if 32 bits    
    cmp     dl, 8
    jne     @f
    mov     ah, al                  ;8bpp is now replicated to 16bpp
@@:
    mov     ecx,eax                 ;double up from 16 bits to 32 bits
	and		ecx,0ffffh
    shl     eax, 16
    or		eax, ecx
PLABEL DoROPFillRect
    jmp     [ROPFillRectFn]			;

	BEST_ALIGN

PLABEL DoROPBrushBlt			    ;es:di = lpPBrush
    cmp     bl,BS_PATTERN           ;pattern brush?
    jne     DoROPBrushBltHollowHatched
	and		cl, MONO_VALID or MONO_PATTERN
	cmp 	cl, MONO_VALID or MONO_PATTERN
	jnz		@f
	jmp	    [ROPMonoBrushBltFn]
@@:
	jmp	    [ROPColorBrushBltFn]

DoROPBrushBltHollowHatched:
    cmp     bl,BS_HOLLOW
    je      BS_Done
    jmp     Jmp_DIB_BitBlt          ;let DIB handle it if brush is HOLLOW or HATCHED
	BEST_ALIGN

PLABEL DestInvertBlt
    mov     eax, [ebx+4]                    ;Get Glint Logical Op
    mov	    RopFunction,eax                 ;save Glint ROP function locally
    jmp     [ROPFillRectFn]			        

	BEST_ALIGN

PLABEL DestinationIsMemory
    cmp     bl, 0cch                        ;Is it a SRCCOPY Logicalop
    jne     DM_UnsupportedROP
    xor     edi, edi
	les		di, [ebp].lpSrcDev
    test    eax,PALETTE_XLAT                ;is there a translate to perform ?
    jnz     DM_Translate                    ;Call the DIB engine if there is

	cmp	    es:[edi].deType,TYPE_DIBENG     ;is this a DIBENGINE surface?
    jne     Jmp_DIB_BitBlt

    mov     ecx, dword ptr es:[edi].deFlags ;cx = src pdevice flags.
    test    ecx,VRAM
    jz      DM_SourceIsMemory               ;j if source is not accelerated
    mov     ecx, dword ptr es:[edi].deDriverReserved ;cx = src pdevice flags.
    mov     ebx, GLInfo.dwBpp               ;get depth of screen
    test    ecx,ACCELERATED
    jz      DM_SourceIsMemory               ;j if source is not accelerated
    cmp     bl, fs:[esi].deBitsPixel        ;are we uploading to the same depth?
    jne     DM_DestDifferentDepth
    jmp     [ImageUploadFn]                 ;Call the upload function

DM_DestDifferentDepth:
DM_Translate:
DM_SourceIsMemory:
DM_UnsupportedROP:
    jmp     [DestMemoryFn]

; There seems to be some trouble when the DIB Engine gets called
; to do a translate from one VRAM surface to another. Therefore, just
; for this call we do the requisite sync, mark the surface as not VRAM
; then call DIB Bitblt, then remark surface as VRAM.

	BEST_ALIGN
PLABEL XLAT_BitBlt
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    mov     eax, [ebp].lpSrcDev+2
    test    eax,0ffffh
    je      Jmp_DIB_BitBlt

	xor		edi,edi
	les		di, [ebp].lpSrcDev
    mov     esi, dword ptr es:[edi].deFlags
    test    esi, VRAM
    je      Jmp_DIB_BitBlt
    ; Only get here if the source is VRAM
    and     esi, 07fffh
    mov     es:[edi].deFlags, si

    ; Do a sync to simulate what would have happened if VRAM was set.
    push    [ebp].lpDestDev               ;--> to destination bitmap descriptor
    push    [ebp].DestxOrg                ;Destination origin - x coordinate
    push    [ebp].DestyOrg                ;Destination origin - y coordinate
    push    [ebp].lpSrcDev                ;--> to source bitmap descriptor
    push    [ebp].SrcxOrg                 ;Source origin - x coordinate
    push    [ebp].SrcyOrg                 ;Source origin - y coordinate
    push    [ebp].xExt                    ;x extent of the BLT
    push    [ebp].yExt                    ;y extent of the BLT
    push    [ebp].Rop                     ;Raster operation descriptor
    push    [ebp].lpPBrush                ;--> to a physical brush (pattern)
	push 	[ebp].lpDrawMode			  ;--> to a drawmode
    call    DIB_BitBlt
    
	xor		edi,edi
	les		di, [ebp].lpSrcDev
    or      es:[edi].deFlags, 8000h
	jmp		BS_Done


;--------------------------------------------------------------------------
; glintImageDownloadTX
;
; Packed image download - should be used for TX chips.
;
;--------------------------------------------------------------------------
	BEST_ALIGN

PLABEL glintImageDownloadTX
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    CLIPSRCTODST
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
    ;ENABLE_DISCONNECT ds
    DISABLE_FIFO_CHECKING

    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

    mov     eax, [ebp].lpDestDev
	xor		ebx,ebx
    cmp     eax, GLInfo.dwCurrentOffscreen
    je      TXID_SameOffscreen
    lfs     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, fs:[ebx].OSB_PixelOffset
    mov     FBWindowBase, edx
TXID_SameOffscreen:

	mov     ebx, RopFunction       ;restore saved function
    mov     edx, dwFBReadMode      ;get read mode
    mov     cx, wScreenHeight      ;Work out ScissorMaxXY
    and     bh, ROP_REQ_DST SHR 8  ;Remove source read (we only need dest reads)
    or      dh, bh                 ;add source enables to read mode (edx)
    shl     ecx, 16
    mov     cx, wScreenWidth

    mov     ScissorMode, 1         ;User Scissor
    mov     ScissorMaxXY,ecx       ;Min is set up later
    mov     FBReadMode,edx         ;set read mode

    movzx   ecx,[ebp].SrcxOrg            ;Get source x
    movzx   eax,[ebp].DestxOrg           ;X coords
    mov     bx,[ebp].xExt
    and     cx, wBppMask           ;Work out the extra source alignment pixels 
    mov     ScissorMinXY, eax      ;Min XY needs to be set to X start
    add     ebx, eax
    sub     eax, ecx               ;subtract alignment pixels from eax
    sub     [ebp].SrcxOrg, cx            ;subtract them from the source too
    add     [ebp].xExt, cx               ;Add alignment pixels to width
    shl     ebx,16
    shl     eax,16
    mov     StartXDom,eax
    mov     StartXSub,ebx

    mov     ax,[ebp].DestyOrg				;Y coords
    movzx   ebx,[ebp].yExt
    shl     eax,16
    mov     StartY,eax
    mov     Count,ebx
    mov     Render,41048h              ;Variable spans, Trapezoid, SyncOnHostData 

PLABEL DownloadImageData

	xor		esi,esi
    les     si, [ebp].lpSrcDev          ;es:si is PBITMAP structure
    cmp     es:[esi].deType, TYPE_DIBENG
    jne     TXID_SourcePBITMAP
    mov     eax, es:[esi].deDeltaScan   ;Scanline width
    lfs     edi, es:[esi].deBits        ;fs:edi is pointer to data
    jmp     TXID_SourceInESEDI
TXID_SourcePBITMAP:
    xor     eax, eax
    mov     ax, es:[esi].bmWidthBytes
    xor     edi, edi
    lfs     di, es:[esi].bmBits         ;fs:edi is pointer to data
TXID_SourceInESEDI:
    mov     dwDelta, eax                ; preserve for later use
	xor		ebx,ebx
    mov		bx, [ebp].SrcyOrg           ;First Scanline
    mul     ebx                         ;eax is the offset of the first scanline
    mov     cl, GLInfo.bPixelToBytesShift

    mov		bx, [ebp].SrcxOrg           ; pixel start
    shl     ebx, cl                     ; convert to bytes
    mov     [ebp].SrcxOrg, bx                         

    add     eax, ebx                    ; ax is now the offset of the first pixel
    add     edi, eax                    ; fs:edi now points at the first pixel
    mov     bx, [ebp].xExt              ; Convert xExt into words
    add     bx, wBppMask                ; Add on rounding factor
    mov     cl, GLInfo.bPixelToWordShift; get Pixel to Byte shift
    shr     bx, cl
    mov     [ebp].xExt, bx              ; Save number of words
	jmp		short DownloadImageData_NextScan;skip the nops
    
	BEST_ALIGN

PLABEL DownloadImageData_NextScan
    ; yExt = number of scans that need to be downloaded
    mov     si, [ebp].xExt          ; Get number of bytes to download
    push    edi                     ; Save Bitmap Pointer
    mov     eax, esi
    shl     eax, 16                 ; Work out Hold count
    sub     eax, 0ff02h             ; Subtract one from count and add in the Color Tag (0xfe)
    mov     GPFifo1, eax            ; Send Count/Tag to Glint

	test	edi, 4
	jz		@f
    mov     eax, fs:[edi]            
    mov     GPFifo1, eax         
    add     edi, 4
    dec		esi						;sub     si, 1
@@:	         
	lea		eax, GPFifo1
    sub     esi, 4                   ; Have we got < 4 words to download
    jb      DownloadImageData_LastWords

PLABEL DownloadImageData_Next16Words
	mov		ebx,fs:[edi +  0]
	mov		edx,fs:[edi + 04]
	mov		ecx,fs:[edi + 08]
	mov		[eax +  0],ebx
	mov		ebx,fs:[edi + 12]
	mov		[eax +  4],edx
	mov		[eax +  8],ecx
	mov		[eax + 12],ebx

    add     edi, 16
    sub     esi, 4
    jae     DownloadImageData_Next16Words

PLABEL DownloadImageData_LastWords
    add     esi, 4
    cmp     esi, 2
    jb      @f
	
	mov		ebx,fs:[edi + 0]
	mov		ecx,fs:[edi + 4]
	mov		[eax + 0],ebx
	mov		[eax + 4],ecx

    add     edi, 8
    sub     esi, 2
@@:
    cmp     esi, 1
    jne     @f
    mov     ecx, fs:[edi]           ; Get 1 word
    mov     [eax + 8], ecx          ; Send 1 word to Glint         
@@:
PLABEL DownloadImageData_AllWordsDone
    pop     edi                         ; Restore the pointer to start of scan
    add     edi, dwDelta                ; Point to start of next scan
    dec     [ebp].yExt			        ; Reduce the count to do
    jnz      DownloadImageData_NextScan

    ; We have now downloaded all scans    

PLABEL DownloadImageData_Done
    ; Resore the FP register state

    xor     eax,eax
    mov     edx, dwFBReadMode               ;get read mode
    mov     ScissorMode, eax
    mov     FBReadMode, edx

	jmp	BSXUnexcludeCursor


;--------------------------------------------------------------------------
; permediaImageDownload
;
; Image download - specially optimized for Permedia chip.
;
; Operates by setting up the Permedia chip to draw a rectangle of
; appropriate size and waiting for the host processor to provide each
; pixel.  If the source pixels are 8 or 16 bits wide they are packed
; into a 32bit word.
;
; For efficiency the code widens the rectangular area until both
; left and right edges are 32bit aligned and uses hardware to shift
; and mask the data.  To promote the more efficient PCI-burst mode
; transfer, data is written to the chip with incrementing addresses.
;
; Note: Care must be taken to ensure NO address outside of the source
; buffer is accessed.  If this is not done then a General Protection
; Fault can result.  In practice this means paying close attention
; to what is read for the very first and very last pixels.
;
; Assumptions:	src = host memory.
;				dest = screen memory.
;				Src and dest are same depth.
;				No logical Ops required.
;				No palette translation is required.
;--------------------------------------------------------------------------
		BEST_ALIGN

PLABEL permediaImageDownload

	assumes ds,_INIT
	assumes es,nothing
	assumes fs,nothing
	assumes gs,nothing

    CLIPSRCTODST
    EXCLUDESWCURSOR
	DISPLAY_CONTEXT
    DISABLE_FIFO_CHECKING

    ; Enable PCI Disconnect mode.
    ; Avoids the need to poll chip by calling WaitFifo.
    ;
    ;ENABLE_DISCONNECT ds

	; Inform other display driver routines that registers will change.
	;
	mov     DD_CurrentContext, DD_UNDEFINED_CTXT

	; Setup Permedia registers
	;
    mov     LogicalOpMode,0             ; Don't use constantFBWriteData.
	mov     eax, [ebp].lpDestDev
	cmp     GLInfo.dwCurrentOffscreen, eax
	je      @f
	xor		ebx,ebx
	lfs     bx, [ebp].lpDestDev
	mov     GLInfo.dwCurrentOffscreen, eax
	mov     edx, fs:[ebx].OSB_PixelOffset
	mov     FBWindowBase, edx
@@:
    ; Load common values into registers
    ;
    mov     esi,dword ptr [ebp].SrcxOrg
    mov     edi,dword ptr [ebp].DestxOrg
    mov     ecx,dword ptr [ebp].xExt
	and		esi,0ffffh
	and		edi,0ffffh
	and		ecx,0ffffh

	; Set Permedia to mask surplus bytes at each end of a row
	; Essential if the pixel size is 8 or 16 bits instead of 32.
	;
	mov		eax,edi
	shl		eax,16
	or		eax,edi
	add		eax,ecx					; eax = (DestXorg << 16) | (DestXorg+xExt)
	mov		PackedDataLimits,eax	; Set up automatic masking

	; Calculate correct FBReadMode settings according to pixel depth.
	;
	mov     edx,dwFBReadMode		; Get partial products (saved at startup)
	and     edx,NOT(ROP_REQ_DST OR ROP_REQ_SRC); Disable src & dest reads

	mov		al,byte ptr wBpp		; Get pixel depth (low 8 bits)
	cmp		al,8
	jne		Pid1

	; 8 Bit per pixel packed word aligned setup
	;
	mov		ebx,esi
	lea		eax, [esi + ecx + 3]	;Calculate right x-coord rounding up to next word boundary
	shr		ebx,2					; Round down to previous word boundary.
	shr		eax,2
	sub		eax,ebx
	mov		dwSrcWords,eax			; Store number of src words to transfer.

	mov		eax,edi   				; Calculate relative offset between src & dest.
	and		al,3
	mov		ebx,esi
	and		bl,3
	sub		al,bl
	and		ax,7
	shl		eax,20					; Format result for FBReadMode reg.
	or		edx,eax
	or		edx,00080000h			; Set Packed data bit.

	mov		ebx,edi
	shr		ebx,2					; Round down to previous word boundary.
	lea		eax, [edi + ecx + 3]	;Calculate right x-coord rounding up to next word boundary.
	shr		eax,2

	jmp		Pid3

	BEST_ALIGN
Pid1:	cmp		al,16
	jne		Pid2

	; 16 Bit per pixel packed word aligned setup
	;
	mov		ebx,esi
	shr		ebx,1					; Round down to previous word boundary.
	lea		eax, [esi + ecx + 1]	; calculate right x-coord rounding up to next word boundary
	shr		eax,1
	sub		eax,ebx
	mov		dwSrcWords,eax			; Store number of src words to transfer.

	mov		eax,edi   				; Calculate relative offset between src & dest.
	and		al,1
	mov		ebx,esi
	and		bl,1
	sub		al,bl
	and		eax,7
	shl		eax,20					; Format result for FBReadMode reg.
	or		edx,eax
	or		edx,00080000h			; Set Packed data bit.

	mov		ebx,edi
	lea		eax, [edi + ecx + 1]	;Calculate right x-coord, rounding upwards to next word boundary
	shr		ebx,1					; Round down to previous word boundary.
	shr		eax,1

	jmp		Pid3

	BEST_ALIGN
Pid2:	; 32 Bit per pixel setup
	;
	mov		eax,ecx					; Calculate right x-coord.
	mov		dwSrcWords,eax			; Store number of src words to transfer.

	mov		eax,edi   				; Get left x-coord.
	mov		ebx,edi
	add		eax,ecx					; Calculate right x-coord.

Pid3:	; edx	= required FBReadMode
	; bx	= left dest x-coord word.
	; ax	= right dest x-coord word.
	; 
	mov     FBReadMode,edx			; Set read mode

    ; Calculate number of words to download per line.
    ; Note: It is crucial to get this correct as otherwise the Permedia chip
    ; will not be in agreement with the host as to how many words to expect.
    ; This can cause all sorts of unexpected crashes and hangs.
    ; Remember Permedia expects (StartXSub - StartXDom) words when in packed mode.

	sub		eax,ebx					; Calculate number of dest words per line.

    ; Depending on alignments and required relative offsets between src and dest
    ; it is possible for the source to fit in one more or one less word than the
    ; dest.  The code therefore has to choose the bigger of the two when deciding
    ; how many words need downloading.
    ; Note: this can lead to situations where a word beyond the end of the src
    ; buffer is read. But that is checked later on.

    cmp     eax,dwSrcWords            ; Dest width >= src width?
    jae     @f                      ; yes: use dest width
                                    ; No:  use src width instead
    mov     eax,dwSrcWords
@@:
	mov		[ebp].xExt,ax					; Save number of words to transfer per line.

	lea		ecx, [ebx + eax]
	shl		ebx,16					; Convert left dest x-coord to Fixed point.
	shl		ecx,16					; Convert right dest x-coord to Fixed point.
	mov		StartXDom,ebx
	mov		StartXSub,ecx

	mov		eax,dword ptr [ebp].DestyOrg				; Y-coords
	mov		ebx,dword ptr [ebp].yExt
	shl		eax,16
	and		ebx,0ffffh
	mov		StartY,eax
	mov		eax,1040h				; Trapezoid, SyncOnHostData
	mov		Count,ebx

	mov		Render,eax				; Start rendering.

	xor		esi,esi
    les     si, [ebp].lpSrcDev          ;es:si is PBITMAP structure
    cmp     es:[esi].deType, TYPE_DIBENG
    jne     PID_SourcePBITMAP
    mov     eax, es:[esi].deDeltaScan   ;Scanline width
    lfs     edi, es:[esi].deBits        ;fs:edi is pointer to data
    jmp     PID_SourceInESEDI
PID_SourcePBITMAP:
    xor     eax, eax
    mov     ax, es:[esi].bmWidthBytes
    xor     edi, edi
    lfs     di, es:[esi].bmBits         ;fs:edi is pointer to data
PID_SourceInESEDI:
    mov     dwDelta, eax                ; preserve for later use

	mov		ebx,dword ptr [ebp].SrcyOrg	; First Scanline
	and		ebx,0ffffh
	mul     ebx                         ; eax is the byte offset of the first scanline
	mov     cl,GLInfo.bPixelToBytesShift
	; Then get 16 bits with a 32 bit load
	mov		ebx,dword ptr [ebp].SrcxOrg	; Pixel start
	and		ebx,0ffffh
	shl     ebx,cl					    ; Convert to bytes
	and		bl,0FCh					    ; Round down to word boundary
	add     eax,ebx					    ; eax is now the offset of the first pixel
	add     edi,eax                     ; fs:edi now points at the first word to transfer
	jmp	short PID_NextScan			    ; skip those nops

	BEST_ALIGN
PLABEL PID_NextScan
	; Registers that must not be changed in middle of main loop.
    ; edx    = inter line byte offset.
    ; fs:edi = Address of first pixel on a line.
	;
    mov     ecx,dword ptr [ebp].xExt ; Get number of dwords to be tranferred per line
    mov     esi,edi                 ; Get working copy of 1st pixel address.
	and		ecx,0ffffh

	mov     eax,ecx
	shl		eax,16					; Work out hold count
	sub		eax,0ff02h				; Dec hold count & add color tag (0xfe)
	test	esi,4					; Odd word?
	mov		GPFifo1,eax				; Send Count/Tag to Permedia
	jz		PID_Even				; no
									; yes: deal with it.
	mov     eax,fs:[esi]			; Get word from bitmap
	add     esi,4					; Point to next word
	mov     GPFifo2,eax				; Send it to Permedia
	dec     ecx  					; Keep count of remaining words

PID_Even:
	cmp     ecx,10					; Are there <= 5 words to download?
	jle		PID_LastWords			; yes
									; no
	push	edi
	push	ebx
	push	ebp
	jmp		PID_Next10Words	;skip those nops

	BEST_ALIGN
PLABEL PID_Next10Words
	mov		eax,fs:[esi +  0]
	mov		edi,fs:[esi +  4]
	mov		ebx,fs:[esi +  8]
	mov		ebp,fs:[esi + 12]
	mov		edx,fs:[esi + 16]
	mov		[GPFifo1],eax
	mov		[GPFifo2],edi
	mov		[GPFifo3],ebx
	mov		[GPFifo4],ebp
	mov		[GPFifo5],edx
	mov		eax,fs:[esi + 20]
	mov		edi,fs:[esi + 24]
	mov		ebx,fs:[esi + 28]
	mov		ebp,fs:[esi + 32]
	mov		edx,fs:[esi + 36]
	mov		[GPFifo6],eax
	mov		[GPFifo7],edi
	mov		[GPFifo8],ebx
	mov		[GPFifo9],ebp
	mov		[GPFifo10],edx
	sub     ecx,10
  	add     esi,40
	cmp		ecx,10
	jg		PID_Next10Words
	pop		ebp
	pop		ebx
	pop		edi

PLABEL PID_LastWords
	jmp		[PID_lastWords + 4 * ecx]
	; NOTE: The reason for this apparently inefficient code is to prevent a
	; General Protection Fault.  If the last pixel of the source bitmap is
	; also the last byte of allocated memory a fatal GPF will arise if the
	; download attempts to read past the end of the source bitmap.  Without
	; the following code this would occur as a result of the X-coord rounding.
	;

PID_10More:
	mov		eax,fs:[esi]			; Get 1 word
	add		esi,4
	mov		GPFifo1,eax				; Send 1 word
PID_9More:
	mov		eax,fs:[esi]			; Get 1 word
	add		esi,4
	mov		GPFifo2,eax				; Send 1 word
PID_8More:
	mov		eax,fs:[esi]			; Get 1 word
	add		esi,4
	mov		GPFifo3,eax				; Send 1 word
PID_7More:
	mov		eax,fs:[esi]			; Get 1 word
	add		esi,4
	mov		GPFifo4,eax				; Send 1 word
PID_6More:
	mov		eax,fs:[esi]			; Get 1 word
	add		esi,4
	mov		GPFifo5,eax				; Send 1 word
PID_5More:
	mov		eax,fs:[esi]			; Get 1 word
	add		esi,4
	mov		GPFifo6,eax				; Send 1 word
PID_4More:
	mov		eax,fs:[esi]			; Get 1 word
	add		esi,4
	mov		GPFifo7,eax				; Send 1 word
PID_3More:
	mov		eax,fs:[esi]			; Get 1 word
	add		esi,4
	mov		GPFifo8,eax				; Send 1 word
PID_2More:
	mov		eax,fs:[esi]			; Get 1 word
	add		esi,4
	mov		GPFifo9,eax				; Send 1 word
PID_1More:
	mov		eax,dwSrcWords
	cmp		ax,[ebp].xExt           ; Need a dummy word?
	jb		@f                      ; Yes: skip it
                                    ; No: read it
	mov		eax,fs:[esi]			; Get 1 word
@@:	mov		GPFifo10,eax			; Send 1 word

PLABEL PID_AllWordsDone
    add     edi,dwDelta             ; Point to 1st pixel of next line
	dec     [ebp].yExt              ; Reduce the count to do & update SF
	jnz      PID_NextScan

PLABEL PID_Done
    ; Restore Permedia registers to default values

    mov     LogicalOpMode,20h       ; UseConstantFBWriteData.
    mov     edx, dwFBReadMode       ; Get cached read mode value.
    mov     FBReadMode, edx

	jmp	BSXUnexcludeCursor


;--------------------------------------------------------------------------
; permediaImageUpload
;
; Image upload - specially optimized for Permedia chip.
;
; Operates by setting up the Permedia chip to draw a rectangle of
; appropriate size and waiting for the host processor to read each
; pixel. If the source pixels are 8 or 16 bits wide they are packed
; into a 32bit word.
;
; Assumptions:	src = screen memory.
;				dest = host memory.
;				Src and dest are same depth.
;				No logical Ops required.
;				No palette translation is required.
;--------------------------------------------------------------------------
		BEST_ALIGN

PLABEL P2ImageUpload

	assumes ds,_INIT
	assumes es,nothing
	assumes fs,nothing
	assumes gs,nothing

    mov     RopFunction, ROP_REQ_SRC+ROP_REQ_DST        ; To ensure Src and Dst are clipped
    CLIPSRCTODST
    EXCLUDESWCURSOR
	DISPLAY_CONTEXT
    DISABLE_FIFO_CHECKING

	; Inform other display driver routines that registers will change.
	;
	mov     DD_CurrentContext, DD_UNDEFINED_CTXT

	; Setup Permedia registers
	;
    mov     LogicalOpMode,0             ; Don't use constantFBWriteData.
    mov     FilterMode, 0600h

    ; Ensure we are using the correct offscreen surface
	mov     eax, [ebp].lpSrcDev
	cmp     GLInfo.dwCurrentOffscreen, eax
	je      @f
	xor		ebx,ebx
	lfs     bx, [ebp].lpSrcDev
	mov     GLInfo.dwCurrentOffscreen, eax
	mov     edx, fs:[ebx].OSB_PixelOffset
	mov     FBWindowBase, edx
@@:

    ; Load common values into registers
    ;
    mov     edi,dword ptr [ebp].SrcxOrg
    mov     esi,dword ptr [ebp].SrcyOrg
    mov     ebx,dword ptr [ebp].xExt
	and		edi,0ffffh
	and		esi,0ffffh
	and		ebx,0ffffh

	; Calculate correct FBReadMode settings according to pixel depth.
	;
    mov     FBWriteMode, 0
	mov     edx,dwFBReadMode		        ; Get partial products (saved at startup)
    mov     cl, GLInfo.bPixelToWordShift
    or      edx, 00088000h+ROP_REQ_DST      ; We want to read the destination, use packed + color
    add     ebx, edi                        ; ebx -> right edge
    shl     esi, 16                         ; turn Y into 16.16
    mov     FBReadMode, edx                 ; load FBReadmode into the chip
    shr     edi, cl                         ; convert left edge pixels into DWORDs
    mov     StartY, esi             
    mov     edx,dword ptr [ebp].yExt
    add     ebx, dwBppMask                  ; round up right edge
    shr     ebx, cl                         ; convert right edge pixels into words
    mov     eax, ebx
    shl     ebx, 16                         ; convert into 16.16 coords
    sub     eax, edi                        ; eax is number of words per scanline to upload
    shl     edi, 16                         ; convert into 16.16 coords
    mov     StartXSub, ebx
    mov     StartXDom, edi                  ; load these into registers
    and     edx, 0ffffh
    mov     dwSrcWords, eax                 ; preserve number to upload per scan
    mov     ebx, ImageUploadCmd
    mov     Count, edx                      ; height
	mov		Render,ebx				        ; Start rendering.

    mov     edx,dword ptr [ebp].yExt
    and     edx, 0ffffh
    mul     edx                             ; eax = (dwSrcWords * yExt) DWORDS of data
    mov     nDwords, eax                    ; Total words to DMA

; Now we have to upload (dwSrcWords * yExt) DWORDS of data via DMA. We want to
; kick off a DMA for the entire size of our buffer to start with. If the buffer can
; contain the entire upload, all is well and good. If it can't, we have to wrap when
; the count gets to zero.
; Another factor which complicates things is that we want to process each scanline when
; we have enough data read into the buffer.
; dwSrcWords    - words per scanline
; nDwords       - words left to DMA in next DMA operations
; nTemp1        - unprocessed words in DMA buffer
; nTemp2        - known valid words in DMA buffer
; nTemp3        - Bytes to copy per scanline 
; pTemp1        - Address of start of unread data
; pTemp2        - Address of first unDMA'ed data

PLABEL P2IU_UploadData
    mov     ebx, nDwords
    mov     eax, GLInfo.dw2D_DMA_Phys       ; First of all, kick off the initial DMA upload
    cmp     ebx, DMA_BUFFER_2D_SIZE/4
    jbe     @f
    mov     ebx, DMA_BUFFER_2D_SIZE/4
@@:
    mov     OutDMAAddress, eax
    mov     OutDMACount, ebx
    mov     eax, dword ptr [ebp].SrcxOrg    ; Get Src X
    sub     nDwords, ebx                    ; nDwords is now number to DMA in next buffer
    mov     cl, GLInfo.bPixelToBytesShift
    and     eax, 0ffffh
	shl		eax, cl							; Src X -> BYTES
    mov     nTemp1, ebx                     ; Numver of words being DMA'ed
	and		eax, 3							; Number of extra bytes
    mov     pTemp2, DMA_BUFFER_2D           ; Start of next DMA buffer
	add		eax, DMA_BUFFER_2D						; Point to first valid data
    mov     nTemp2, 0
    mov     pTemp1, eax			            ; Start of this DMA buffer

    xor     esi, esi
	lfs     si, [ebp].lpDestDev             ; load up Destination surface pointer
    les     edi, fs:[esi].deBits            ; load up destination data base address
    mov     eax, fs:[esi].deDeltaScan       ; Number to advance between scanlines
    mov     edx, dword ptr [ebp].DestyOrg
    mov     dwDelta, eax
    and     edx, 0ffffh
    mul     edx                             ; work out offset to the first scanline
    add     edi, eax                        ; edi now points at the first scanline

    mov     eax, dword ptr [ebp].DestxOrg   ; Get Dest X
    mov     ebx, dword ptr [ebp].xExt       ; Get Width
    and     eax, 0ffffh
    shl     eax, cl                         ; DestX to bytes
    and     ebx, 0ffffh
    shl     ebx, cl                         ; Width to bytes
    mov     ecx, DWORD PTR [ebp].yExt       ; get number of scanlines to do
    add     edi, eax                        ; es:edi points to first dest byte
    and     ecx, 0ffffh                     ; ecx = scanlines
    mov     nTemp3, ebx                     ; Store number bytes to copy
    jmp     P2IU_UploadScanline

	BEST_ALIGN

P2IU_RestartDMA2ndHalf:
    cmp     pTemp1, DMA_BUFFER_2D + (DMA_BUFFER_2D_SIZE/2)
    jae     P2IU_CheckScanline              ; Not yet finished with data. Retry later

    mov     eax, GLInfo.dw2D_DMA_Phys       ; Kick off in second half
    mov     pTemp2, DMA_BUFFER_2D
    add     eax, (DMA_BUFFER_2D_SIZE/2)
    jmp     P2IU_ActuallyRestartDMA

	BEST_ALIGN
P2IU_RestartDMA:
    ; We want to wait for half of the DMA buffer to become free. 
    cmp     pTemp2, DMA_BUFFER_2D           ; Waiting for the first half?
    jne     P2IU_RestartDMA2ndHalf
    cmp     pTemp1, DMA_BUFFER_2D + (DMA_BUFFER_2D_SIZE/2)
    jb      P2IU_CheckScanline              ; Not yet finished with data. Retry later
    mov     eax, GLInfo.dw2D_DMA_Phys       ; Kick off in first half
    mov     pTemp2, DMA_BUFFER_2D + (DMA_BUFFER_2D_SIZE/2)

P2IU_ActuallyRestartDMA:
    mov     ebx, nDwords

    cmp     ebx, DMA_BUFFER_2D_SIZE/8       ; Comparing number of DWORDS
    jbe     @f
    mov     ebx, DMA_BUFFER_2D_SIZE/8
@@:
    mov     OutDMAAddress, eax
    mov     OutDMACount, ebx
    mov     nTemp1, ebx
    sub     nDwords, ebx                    ; nDwords is now number to DMA in next buffer
    jmp     P2IU_CheckScanline              ; with nDwords of -1

	BEST_ALIGN
PLABEL P2IU_UploadScanline

    push    ecx                             ; save count
    cmp     nDwords, 0                      ; Finished initiating all DMAs?
    je      P2IU_CheckScanline              ; Skip next buffer test if so.
    cmp     nTemp1, 0                       ; Are we waiting to restart DMA?
    je      P2IU_RestartDMA

P2IU_CheckScanline:
    mov     ecx, dwSrcWords                 ; Get number of source words per scanline
    ; Wait for one scanline of data, then do the upload.
    cmp     nTemp2, ecx                     ; have we got enough words to proceed?
    jae     P2IU_GotScanline
P2IU_WaitScanline:
    mov     edx, nTemp1
    mov     ebx, OutDMACount
    sub     edx, ebx                        ; Work out the number of words DMA'ed so far
    add     nTemp2, edx
    mov     nTemp1, ebx
    cmp     nDwords, 0                      ; Finished initiating all DMAs?
    je      @f                              ; Skip the end test if so
    test    ebx, ebx                        ; Got to end of DMA (OutDMACount == 0)?
    je      P2IU_RestartDMA
@@:
    cmp     nTemp2, ecx
    jae     P2IU_GotScanline

    mov     ebx, nTemp2
    mov     eax, ecx
    sub     eax, ebx                        ; Get number of words to wait for
    shl     eax, 3                          ; *8 for loops to wait
@@:
    ; We need to delay a while before checking again.
    dec     eax
    jz      P2IU_WaitScanline    
    jmp     @b

P2IU_GotScanline:
    sub     nTemp2, ecx                     ; Reduce the number valid by the number in the scanline

    mov     esi, pTemp1
    lea     edx, [esi + ecx*4]              ; Work out the address of the start of the next scanline
    mov     ecx, nTemp3

    cmp     edx, DMA_BUFFER_2D + DMA_BUFFER_2D_SIZE
    jae     PIU_SplitScanline

    mov     pTemp1, edx                     ; Store it
    call    MemCopy

P2IU_UploadScanDone:
    pop     ecx                             ; restore number of scanlines to do
    add     edi, dwDelta                    ; advance to the next scanline
    dec     ecx
    jnz     P2IU_UploadScanline
    jmp     PIU_Done

PIU_SplitScanline:
    mov     ecx, DMA_BUFFER_2D + DMA_BUFFER_2D_SIZE
    sub     ecx, esi

    push    edi                             ; save destination address
    push    ecx                             ; save count of first half
    test    ecx, ecx
    je      @f
    call    MemCopy
@@:
    pop     eax                             ; restore count of first half
    mov     ecx, nTemp3                     ; Get count of total number of bytes
    add     edi, eax                        ; Increase destination as appropriate
    sub     ecx, eax                        ; work out number of bytes to copy

    mov     ebx, dwSrcWords
    lea     edx, [esi - DMA_BUFFER_2D_SIZE]
    lea     edx, [edx + ebx * 4]
    mov     esi, DMA_BUFFER_2D
    mov     pTemp1, edx

    test    ecx, ecx
    je      @f
    call    MemCopy
@@:
    pop     edi                             ; restore destination address
    jmp     P2IU_UploadScanDone

;--------------------------------------------------------------------------
; permediaImageUpload
;
; Image upload - specially optimized for Permedia chip.
;
; Operates by setting up the Permedia chip to draw a rectangle of
; appropriate size and waiting for the host processor to read each
; pixel. If the source pixels are 8 or 16 bits wide they are packed
; into a 32bit word.
;
; Assumptions:	src = screen memory.
;				dest = host memory.
;				Src and dest are same depth.
;				No logical Ops required.
;				No palette translation is required.
;--------------------------------------------------------------------------
		BEST_ALIGN

PLABEL permediaImageUpload

	assumes ds,_INIT
	assumes es,nothing
	assumes fs,nothing
	assumes gs,nothing

    mov     RopFunction, ROP_REQ_SRC+ROP_REQ_DST        ; To ensure Src and Dst are clipped
    CLIPSRCTODST
    EXCLUDESWCURSOR
	DISPLAY_CONTEXT
    DISABLE_FIFO_CHECKING

	; Inform other display driver routines that registers will change.
	;
	mov     DD_CurrentContext, DD_UNDEFINED_CTXT

	; Setup Permedia registers
	;
    mov     LogicalOpMode,0             ; Don't use constantFBWriteData.
    mov     FilterMode, 0600h

    ; Ensure we are using the correct offscreen surface
	mov     eax, [ebp].lpSrcDev
	cmp     GLInfo.dwCurrentOffscreen, eax
	je      @f
	xor		ebx,ebx
	lfs     bx, [ebp].lpSrcDev
	mov     GLInfo.dwCurrentOffscreen, eax
	mov     edx, fs:[ebx].OSB_PixelOffset
	mov     FBWindowBase, edx
@@:

    ; Load common values into registers
    ;
    mov     edi,dword ptr [ebp].SrcxOrg
    mov     esi,dword ptr [ebp].SrcyOrg
    mov     ebx,dword ptr [ebp].xExt
	and		edi,0ffffh
	and		esi,0ffffh
	and		ebx,0ffffh

	; Calculate correct FBReadMode settings according to pixel depth.
	;
    mov     FBWriteMode, 0
	mov     edx,dwFBReadMode		        ; Get partial products (saved at startup)
    mov     cl, GLInfo.bPixelToWordShift
    or      edx, 00088000h+ROP_REQ_DST      ; We want to read the destination, use packed + color
    add     ebx, edi                        ; ebx -> right edge
    shl     esi, 16                         ; turn Y into 16.16
    mov     FBReadMode, edx                 ; load FBReadmode into the chip
    shr     edi, cl                         ; convert left edge pixels into DWORDs
    mov     StartY, esi             
    mov     edx,dword ptr [ebp].yExt
    add     ebx, dwBppMask                  ; round up right edge
    shr     ebx, cl                         ; convert right edge pixels into words
    mov     eax, ebx
    shl     ebx, 16                         ; convert into 16.16 coords
    sub     eax, edi                        ; eax is number of words per scanline to upload
    shl     edi, 16                         ; convert into 16.16 coords
    mov     StartXSub, ebx
    mov     StartXDom, edi                  ; load these into registers
    and     edx, 0ffffh
    mov     dwSrcWords, eax                 ; preserve number to upload per scan
    mov     ebx, ImageUploadCmd
    mov     Count, edx                      ; height
	mov		Render,ebx				        ; Start rendering.

    ; Now we have to upload (dwSrcWords * yExt) DWORDS of data
PLABEL PIU_UploadData
    mov     ebx, dword ptr [ebp].SrcxOrg    ; Get Src X
    xor     esi, esi
	lfs     si, [ebp].lpDestDev             ; load up Destination surface pointer
    mov     cl, GLInfo.bPixelToBytesShift
    and     ebx, 0ffffh
    les     edi, fs:[esi].deBits            ; load up destination data base address
    mov     eax, fs:[esi].deDeltaScan       ; Number to advance between scanlines
    mov     edx, dword ptr [ebp].DestyOrg
    mov     dwDelta, eax
    and     edx, 0ffffh
    mul     edx                             ; work out offset to the first scanline
    add     edi, eax                        ; edi now points at the first scanline

    mov     eax, dword ptr [ebp].DestxOrg   ; Get Dest X
    shl     ebx, cl                         ; SrcX to bytes
    and     eax, 0ffffh
    shl     eax, cl                         ; DestX to bytes
    mov     edx, dword ptr [ebp].xExt       ; get width
    add     edi, eax                        ; es:edi points to first dest byte 
    and     edx, 0ffffh
    shl     edx, cl                         ; convert into bytes
    add     edx, ebx                        ; add Srcx -> right byte
    and     ebx, 3                          ; get initial number of bytes
    and     edx, 3                          ; get final number of bytes
    mov     esi, [PIU_FirstDWORD + ebx*4]
    mov     ecx, [PIU_LastDWORD + edx*4]
    mov     Fn1, esi
    mov     Fn2, ecx

    mov     eax, DWORD PTR [ebp].yExt                 ; get number of scanlines to do
    mov     ecx, dwSrcWords
    and     eax, 0ffffh
	mov		FIFODiscon, 3
    ; Wait for any data, then do the upload. We assume here that uploading is slower than
    ; the chip can supply the data
@@:
    cmp     OutFIFOWords, 1
    jb      @b
    sub	    ecx, 2
    jae     PIU_UploadScanline

PLABEL PIU_SingleWordPerScan                            
    ; in this rare case we just have to decide how many bytes to write and jump
    ; to the appropriate routine
    ; ebx is initial number of bytes, edx is final number of bytes, eax is number of scanlines
    sub     edx, ebx                        ; work out number
    ja      @f                              ; Is this crossing a 4 byte boundary?
    add     edx, 4                          ; If so, add 4 to get real number.
@@:
    mov     ecx, ebx
    shl     ecx, 3                          ; work out shift to get data into least sig bytes
    jmp     [PIU_SingleDWORD + edx*4]

	BEST_ALIGN
PLABEL PIU_UploadScanline
    push    eax                             ; save number of scanlines to do
    push    ecx                             ; save number of central words per scanline
    push    edi                             ; push address of first byte
    mov     eax, es:[edi]                   ; Read from destination to get into cache.
    jmp     [Fn1]

	BEST_ALIGN
PIU_UploadFirst3Bytes:
    mov     eax, GPFifo1
    mov     es:[edi], ah
    shr     eax, 16
    mov     es:[edi+1], ax
    add     edi, 3
    jmp     PIU_UploadCenterDWORDS

	BEST_ALIGN
PIU_UploadFirst2Bytes:
    mov     eax, GPFifo1
    shr     eax, 16
    mov     es:[edi], ax
    add     edi, 2
    jmp     PIU_UploadCenterDWORDS

	BEST_ALIGN
PIU_UploadFirst1Byte:
    mov     eax, GPFifo1
    shr     eax, 24
    mov     es:[edi], al
    add     edi, 1
    jmp     PIU_UploadCenterDWORDS

	BEST_ALIGN
PIU_UploadFirst4Bytes:
    inc     ecx                             ; Just load an extra word in the center section
    jmp     PIU_UploadCenterDWORDS

	BEST_ALIGN
PIU_UploadCenterDWORDS:
    cmp     ecx, 4
    jb      PIU_UploadCenterLessThan4
    push    ebp
PIU_UploadCenterNext4DWORDS:
    mov     ebp, es:[edi+16]                   ; Read from dest of next batch to bring into cache.
PIU_UploadCenterLast4DWORDS:
    mov     eax, GPFifo2
    mov     ebx, GPFifo3
    mov     edx, GPFifo4
    mov     esi, GPFifo5
    mov     es:[edi], eax
    mov     es:[edi+4], ebx     
    mov     es:[edi+8], edx     
    mov     es:[edi+12], esi
    add     edi, 16
    sub     ecx, 4
    cmp     ecx, 4
    ja      PIU_UploadCenterNext4DWORDS
    je      PIU_UploadCenterLast4DWORDS     ; If we are at the last DWORD, dont cache next block.
    pop     ebp

    BEST_ALIGN         
PIU_UploadCenterLessThan4:
    cmp     ecx, 2
    jb      PIU_UploadCenterLessThan2
    mov     eax, GPFifo6
    mov     ebx, GPFifo7
    mov     es:[edi], eax
    mov     es:[edi+4], ebx     
    add     edi, 8
    sub     ecx, 2
PIU_UploadCenterLessThan2:
    cmp     ecx, 1
    jb      PIU_UploadCenterDone
    mov     eax, GPFifo7
    mov     es:[edi], eax
    add     edi, 4
PIU_UploadCenterDone:
    jmp     [Fn2]

    BEST_ALIGN         
PIU_UploadLast4Bytes:
    mov     eax, GPFifo8
    mov     es:[edi], eax
    jmp     PIU_UploadScanDone

    BEST_ALIGN         
PIU_UploadLast1Byte:
    mov     eax, GPFifo8
    mov     es:[edi], al
    jmp     PIU_UploadScanDone

    BEST_ALIGN         
PIU_UploadLast2Bytes:
    mov     eax, GPFifo8
    mov     es:[edi], ax
    jmp     PIU_UploadScanDone

    BEST_ALIGN         
PIU_UploadLast3Bytes:
    mov     eax, GPFifo8
    mov     es:[edi], ax
    shr     eax, 16
    mov     es:[edi+2], al
    jmp     PIU_UploadScanDone

    BEST_ALIGN         
PIU_UploadScanDone:
    pop     edi                             ; restore address of first byte
    pop     ecx                             ; restore number of central words per scanline
    pop     eax                             ; restore number of scanlines to do
    add     edi, dwDelta                    ; advance to the next scanline
    dec     eax
    jnz     PIU_UploadScanline

PLABEL PIU_Done
    ; Restore Permedia registers to default values

ifdef DEBUG
    mov     eax, OutFIFOWords
    test    eax, eax
    je      @f
    int     1
@@:
    mov     eax, ErrorFlags
    test    eax, eax
    je      @f
    int     1
@@:
endif
	mov		FIFODiscon, 1
    mov     LogicalOpMode,20h       ; UseConstantFBWriteData.
    mov     edx, dwFBReadMode       ; Get cached read mode value.
    mov     eax, dwFBWriteMode
    mov     FBReadMode, edx
    mov     FBWriteMode, eax
	jmp	BSXUnexcludeCursor


    BEST_ALIGN         
PLABEL PIU_Upload4Bytes
    mov     ebx, GPFifo1
    mov     es:[edi], ebx                      ; write out data
    add     edi, dwDelta                    ; advance to the next scanline
    dec     eax
    ja      PIU_Upload3Bytes
    jmp     PIU_Done

    BEST_ALIGN         
PLABEL PIU_Upload1Byte
    mov     ebx, GPFifo1
    shr     ebx, cl                         ; Shift data into LSB
    mov     es:[edi], bl                       ; write out data
    add     edi, dwDelta                    ; advance to the next scanline
    dec     eax
    jnz     PIU_Upload1Byte
    jmp     PIU_Done

    BEST_ALIGN         
PLABEL PIU_Upload2Bytes
    mov     ebx, GPFifo1
    shr     ebx, cl                         ; Shift data into LSB
    mov     es:[edi], bx                       ; write out data
    add     edi, dwDelta                    ; advance to the next scanline
    dec     eax
    jnz     PIU_Upload2Bytes
    jmp     PIU_Done

    BEST_ALIGN         
PLABEL PIU_Upload3Bytes
    mov     ebx, GPFifo1
    shr     ebx, cl                         ; Shift data into LSB
    mov     es:[edi], bx                       ; write out data
    shr     ebx, 16
    mov     es:[edi+2], bl                     ; write out data
    add     edi, dwDelta                    ; advance to the next scanline
    dec     eax
    jnz     PIU_Upload3Bytes
    jmp     PIU_Done

;--------------------------------------------------------------------------
; glintCopyBlt
;
; Basic screen to screen bitblt - should be used for SX and TX.
; Some checks for TX Span blt inaccuracies.
;
;--------------------------------------------------------------------------
	BEST_ALIGN

PLABEL	glintCopyBlt	near

	assumes ds,_INIT
	assumes es,nothing
	assumes fs,nothing
	assumes gs,nothing

    CLIPSRCTODST
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
    ;DISABLE_DISCONNECT ds

    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

    mov     eax, [ebp].lpDestDev
    cmp     eax, GLInfo.dwCurrentOffscreen
    je      CB_SameOffscreen
    les     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, es:[bx].OSB_PixelOffset

	GetFifoCount ebx
	WaitFifoIfNeeded 1,ebx
	
    mov     FBWindowBase, edx
	jmp		@f

	BEST_ALIGN

CB_SameOffscreen:

	GetFifoCount ebx
@@:
    WaitFifoIfNeeded 12, ebx

; some standard setups
	xor		eax,eax
	mov		bx,[ebp].xExt 			;get the width into BX
	mov     cx,[ebp].yExt           ;get the height into CX
	mov		al,byte ptr RopFunction ;restore saved function
	mov     LogicalOpMode,eax		;set mode

;
;At this point:
;       BX has X-extent.
;       CX has Y-extent.
;
    mov     Count,ecx               ;y extent
    mov     edi,dwFBReadMode        ;get read mode
    mov     eax,RopFunction			;eax already loaded
    xor     al,al
    or      di,ax
    test    ah,ROP_REQ_SRC SHR 8
    jz      CBNoSource              ;j if no source
    xor     eax, eax                ;Zero source if we are logicop ~D as we
    cmp     bptr [ebp].Rop+2,055h   ;have converted to ~S so that TX spans work
    je      CBLoadSourceOffset
    mov     ax,[ebp].SrcyOrg        ;find source offset in y
    sub     ax,[ebp].DestyOrg
    mov     si,[ebp].SrcxOrg        ;find source offset in x
    sub     si,[ebp].DestxOrg
	shl		eax,16
	sar		eax,16
    mov		edx,dword ptr wScreenWidth        ;width of screen
	and		edx,0ffffh
    mul     edx
	shl		esi,16                  ;include x offset
	sar		esi,16
    add     eax,esi
; Check for off screen bitmap blts from one bitmap to another
    mov     edx, [ebp].lpDestDev
    cmp     edx, [ebp].lpSrcDev
    je      CBLoadSourceOffset
	xor		esi,esi
    les     si, [ebp].lpDestDev
    sub     eax, es:[esi].OSB_PixelOffset
    les     si, [ebp].lpSrcDev
    add     eax, es:[esi].OSB_PixelOffset
PLABEL CBLoadSourceOffset
    mov     FBSourceOffset,eax
PLABEL CBNoSource
    mov     FBReadMode,edi          ;set read mode as required

	mov		edx, CopyBltCmd			; Get the render command
    cmp     GLInfo.wRenderFamily, GLINT_ID
	jne		CBWorkOutDirection
; Due to some wierd 'features' in the TX and MX, we have to turn off spans for some cases.
	cmp		eax, 0					; Is -31 <= source offset < 0 
	jge		@f
	cmp		eax, -31
	jl		@f
	mov		edx, CopyBltSlowCmd		; Get the render command without spans	
@@:
	cmp		[ebp].SrcyOrg, 0
	ja		CBWorkOutDirection
	cmp		[ebp].SrcxOrg, 3
	ja		CBWorkOutDirection		
	mov		edx, CopyBltSlowCmd		; Get the render command without spans

CBWorkOutDirection:

;
;Now we construct the block move command taking into account the direction of
;the move. eax contains the source offset still:
;
	dec		cx						; Decrease Y limit so we render on the right line
	mov		di, [ebp].DestxOrg			; Get X left
	mov		si, [ebp].DestyOrg			; Get Y org (Y top) 
	shl		ecx,16
	shl		esi,16
	add		ebx, edi					; Work out X right
	shl     edi,16						; Shift to Glint coords
	shl     ebx,16
; eax = sourceoffset
; edi = X left, ebx = X right, cx = Y limits, edx = render command, si = Y Top
	cmp		eax, 0
	jl		CBSourceOffsetNegative

; Use left to right and top to bottom
	mov     StartXDom,edi
	mov     StartXSub,ebx
	mov		StartY, esi

	mov		Render,edx					;go for it
    mov     edx, dwFBReadMode               ;get read mode
    mov     FBReadMode, edx
    mov     LogicalOpMode, 20h

    ENABLE_FIFO_CHECKING

	jmp		BSXUnexcludeCursor			;and return to caller

CBSourceOffsetNegative:

; Use right to left and bottom to top
	add		ecx, esi					; work out Y bottom 
	mov     StartXDom,ebx
	mov     StartXSub,edi
	mov		dY,0ffff0000h				; -1
	mov		StartY, ecx

	mov	    Render,edx						;go for it

    mov     edx, dwFBReadMode               ;get read mode
    mov     FBReadMode, edx
    mov     LogicalOpMode, 20h
	mov		dY, 10000h					; +1

    ENABLE_FIFO_CHECKING

	jmp		BSXUnexcludeCursor			;and return to caller

;--------------------------------------------------------------------------
; permediaCopyBlt8Bpp
;
; Screen to screen bitblt on 8bit framebuffer - Optimized for Permedia.
;
; If Src and Dest x-offsets are 32 bit aligned to each other the framebuffer
; depth is temporarily set to 32bits giving approx x4 speedup over a packed copy.
; If Src and Dest x-offsets are 16 bit aligned to each other the framebuffer
; depth is temporarily set to 16bits giving approx x2 speedup over a packed copy.
; If none of these conditions are true then a normal packed copy is performed.
;
; Note: This algorithm temporarily halves or quarters the horizontal resolution
; by changing the partial products.  For this reason 800x600 cannot be
; supported because there are no partial products for 400 or 200.
;
; Assumptions:	Framebuffer is 8Bpp.
;				src = screen memory.
;				dest = screen memory.
;				src and dest are same depth.
;				No palette translation is required.
;				Resolution is not 800x600.
;--------------------------------------------------------------------------
		BEST_ALIGN

PLABEL	permediaCopyBlt8Bpp	near

		assumes ds,_INIT
		assumes es,nothing
		assumes fs,nothing
		assumes gs,nothing

		mov		eax,dword ptr [ebp].SrcxOrg		; I only use the low word from here
		sub		eax,dword ptr [ebp].DestxOrg	; Again only the bottom 16 bits are used
		test	al,1					; Are Src and Dest suitably aligned?
		jnz		permediaCopyBlt			; No: use standard slow (x1) routine.

@@:		shl		eax,16					;sign extend ax into eax
		sar		eax,16
		mov		wdx,eax

		; If necessary perform Permedia context switch.
		;

        CLIPSRCTODST
        EXCLUDESWCURSOR
	    DISPLAY_CONTEXT
        ;DISABLE_DISCONNECT ds

		; Inform other display driver routines that registers may have changed.
		;
	    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

        ; Inform other routines that the current FBWindowBase cache needs recalculating
        ;
	    mov     GLInfo.dwCurrentOffscreen, DD_UNDEFINED_OFFSCREEN

		mov		edi,RopFunction				; Restore saved function
		mov		eax,edi
		and		edi,(ROP_REQ_SRC OR ROP_REQ_DST)
		xor		ah,ah
		mov		ecx,dword ptr [ebp].yExt

		; Setup Permedia registers
		;
		GetFifoCount esi
		WaitFifoIfNeeded 8, esi	; Need 8 registers before 'pcb8_Render' label.
		mov		fifoSpace,esi

		mov     LogicalOpMode,eax			; Set mode
		and		ecx,0ffffh
		mov		Count,ecx					; Set number of lines to render.
		xor		eax,eax

		test	edi, ROP_REQ_SRC				; Does the Rop use the source?
		jz		pcb8_1						; No: use sourceOffset = 0
											; Yes: calculate sourceOffset
		;fetch the interesting 16 bits more quickly
		mov		eax,dword ptr [ebp].SrcyOrg
		mov		esi,wdx
		sub		ax,[ebp].DestyOrg
		mov		edx,dword ptr wScreenWidth
		and		edx,0ffffh
		shl		eax,16
		sar		eax,16
		mul		edx
		add		eax,esi						; eax=(SrcyOrg-DestyOrg)*wScreenWidth + (SrcxOrg-DestxOrg)

		mov		edx,[ebp].lpDestDev
		cmp		edx,[ebp].lpSrcDev				; Is src and dest one and the same?
		je		pcb8_1						; Yes: no adjustment necessary
											; No: adjust offsets
		xor		esi,esi
		les		si,[ebp].lpDestDev
		sub		eax,es:[esi].OSB_PixelOffset
		les		si,[ebp].lpSrcDev
		add		eax,es:[esi].OSB_PixelOffset

		; Select framebuffer depth and partial products according to alignment.
		;
pcb8_1:	xor		ebx,ebx
		lfs     bx, [ebp].lpDestDev
	    mov     edx, fs:[ebx].OSB_PixelOffset

    	test	wdx,3						; Are src & dest 32 bit aligned?
		jz		@f							; yes: use fast (x4) algorithm.
											; no: use medium (x2) algorithm.
		; 16 bit aligned buffers.
		;
        sar     edx,1                               ; Adjust pixel offset
		sar		eax,1								; Convert byte offset to word offset
		mov		FBReadPixel, GLINTDEPTH16			; Set framebuffer depth to 16bpp
		or		edi,GLInfo.dwPartialProducts2		; Get halved partial products.
		jmp		pcb8_2

		; 32 bit aligned buffers.
		;
@@:     sar     edx,2                               ; Adjust pixel offset
		sar		eax,2								; Convert byte offset to dword offset
		mov		FBReadPixel, GLINTDEPTH32           ; Set framebuffer depth to 32bpp
		or		edi,GLInfo.dwPartialProducts4		; Get quartered partial products.

pcb8_2:	mov     FBWindowBase, edx
    	mov		FBReadMode,edi						; Set partial products and buffer read.
		mov		FBSourceOffset,eax

		test	eax,eax						; FBSourceOffset -Ve?
		jl		pcb8_bottotop				; Yes: copy from bottom to top
											; No:  copy from top to bottom
		; Copy from Top to Bottom
		;
		mov		ebx,10000H					; dy = +1
		;Only interested in the bottom 16 bits on this next load...
		mov		ecx,dword ptr [ebp].DestyOrg			; y  = Top
		shl		ecx,16
		jmp		@f
		BEST_ALIGN
pcb8_bottotop:
		; Copy from Bottom to Top
		;
		;load 32 instead of 16 for speed
		mov		ecx,dword ptr [ebp].DestyOrg
		mov		ebx,0FFFF0000H				; dy = -1
		add		ecx,dword ptr [ebp].yExt	;don't care about the high word
		dec		ecx
		shl		ecx,16						; y = Bottom

@@:		mov		dY,ebx
		mov		StartY,ecx

		; Y Coords are now correctly set up.
		; It now remains to set up the X Coords.

		xor		edi,edi						; Assume masks do not need merging.

		test	wdx,3						; Test alignment
		jz		pcb8_32bit					; 32 bit
											; 16 bit
		;load 32 faster than 16...
		mov		edx,dword ptr [ebp].DestxOrg
		and		edx,0ffffh
		mov		eax,edx
		shr		eax,1
		inc		edx
		mov		ebx,edx
		shr		ebx,1
		add		dx,[ebp].xExt
		mov		ecx,edx
		shr		ecx,1
		dec		edx
		shr		edx,1

		;fast 16 bit fetch
		mov		esi,dword ptr [ebp].SrcxOrg
		cmp		[ebp].DestxOrg,si
		jge		@f

		mov		wgx0,eax
		mov		wgx1,ebx
		mov		wgx2,edx
		mov		wgx3,ecx
 
		mov		dwPMask0,0ff00ff00H
		mov		dwPMask1,000ff00ffH
		jmp		pcb8_Render

@@:		mov		wgx0,ecx
		mov		wgx1,edx
		mov		wgx2,ebx
		mov		wgx3,eax

		mov		dwPMask0,000ff00ffH
		mov		dwPMask1,0ff00ff00H
		jmp		pcb8_Render

		BEST_ALIGN
pcb8_32bit:
		; 32 bit alignment

		mov		edx,dword ptr [ebp].DestxOrg		; Load ax,bx,cx,dx with x-offsets
		and		edx,0ffffh
		mov		eax,edx
		shr		eax,2
		add		edx,3
		mov		ebx,edx
		shr		ebx,2
		add		dx,[ebp].xExt
		mov		ecx,edx
		shr		ecx,2
		sub		edx,3
		shr		edx,2

		;fast 16 bit fetch
		mov		esi,dword ptr [ebp].SrcxOrg
		cmp		[ebp].DestxOrg,si					; R to L  or  L to R?
		jge		pcb8_rtol

		; 32bit Fast left to right copy
		;
		mov		wgx0,eax						; Set x-offset order
		mov		wgx1,ebx
		mov		wgx2,edx
		mov		wgx3,ecx

		cmp		ebx,edx						; do masks need merging?
		jbe		@f
		inc		edi							; set mask merge flag
@@:
		movzx	ecx,[ebp].DestxOrg					; Look up appropriate masks.
		mov		edx,ecx
		and		ecx,3
		;add in the bottom 16 bits + garbage
		add		edx,dword ptr [ebp].xExt
		and		edx,3
		mov		eax,pcb_leftMasks[ecx*4]
		mov		dwPMask0,eax
		mov		eax,pcb_rightMasks[edx*4]
		mov		dwPMask1,eax
		jmp		pcb8_Render

		BEST_ALIGN
pcb8_rtol:
		; 32bit Fast right to left copy
		;
		mov		wgx0,ecx
		mov		wgx1,edx
		mov		wgx2,ebx
		mov		wgx3,eax

		cmp		edx,ebx						; Do masks need merging?
		sbb		edi,edi						; If dx < bx di = -1 else di = 0

		;only the bottom 16 bits here matter
		mov		ecx,dword ptr [ebp].DestxOrg					; Look up appropriate masks.
		mov		edx,ecx
		and		ecx,3
		;again, only the bottom 16 bits matter
		add		edx,dword ptr [ebp].xExt
		and		edx,3
		mov		eax,pcb_leftMasks[ecx*4]
		mov		dwPMask1,eax
		mov		eax,pcb_rightMasks[edx*4]
		mov		dwPMask0,eax


pcb8_Render:
		; Assumptions:	if (di != 0) then masks need merging
		;				dwPMask[01] contains suitable masks
		;				wgx[0-3] contain x-coords
		;				Hardware write mask is available.
		;				Software write mask bits are all 1's.

		mov		edx,fifoSpace
		test	edi,edi					; Can masks be combined?
		jz		pcb8_complexRender		; no:
										; yes:
		; The blt does not cross a word boundary.  Therefore the start and end of a scan line
		; are inside the same word.  The masks must be combined.
		;

		mov		eax,dwPMask0
		and		eax,dwPMask1

        WaitFifoIfNeeded 5, edx   ; 5 now and 4 after 'pcb8_finish' label

		mov		FBHardwareWriteMask,eax

		mov		eax,wgx0
		mov		ebx,wgx3
		mov		ecx,CopyBltCmd
		shl		eax,16
		shl		ebx,16
		mov		StartXDom,eax
		mov		StartXSub,ebx
		mov		Render,ecx					    ; Start rendering.
		mov		FBHardwareWriteMask,0ffffffffh	; Restore mask.
		jmp		pcb8_finish                ; Virtually done

		BEST_ALIGN
pcb8_complexRender:
		; Multiple word render required.  Task is split into left, middle and right strips.

		mov		eax,wgx0
		cmp		eax,wgx1						; Does the left strip need drawing?
		je		pcb8_middleStrip			; no: try the middle strip
											; yes:
        WaitFifoIfNeeded 5, edx     ; 5 now and 4 after 'pcb8_finish' label

		mov		eax,dwPMask0
		mov		ebx,wgx0
		mov		ecx,wgx1
		mov		FBHardwareWriteMask,eax
		shl		ebx,16
		shl		ecx,16
		mov		StartXDom,ebx
		mov		eax,CopyBltCmd
		mov		StartXSub,ecx
		mov		Render,eax						; Start rendering.
		mov		FBHardwareWriteMask,0ffffffffh	; Restore mask.

pcb8_middleStrip:

		mov		eax,wgx1
		mov		ebx,wgx2
		cmp		eax,ebx						; Does the middle strip need drawing?
		je		pcb8_endStrip				; no: try the last strip
											; yes:
        WaitFifoIfNeeded 3, edx ; 3 now and 4 after 'pcb8_finish' label

		shl		eax,16
		shl		ebx,16
		mov		StartXDom,eax
		mov		StartXSub,ebx
		mov		eax,CopyBltCmd
		mov		Render,eax					; Start rendering.

pcb8_endStrip:
		mov		eax,wgx2
		mov		ebx,wgx3
		cmp		eax,ebx						; Does the right strip need drawing?
		je		pcb8_finish 				; no: finished
											; yes:
		mov		ecx,dwPMask1
		shl		eax,16
		shl		ebx,16

        WaitFifoIfNeeded 5, edx     ; 5 now and 4 after 'pcb8_finish' label

		mov		FBHardwareWriteMask,ecx
		mov		StartXDom,eax
		mov		StartXSub,ebx
		mov		eax,CopyBltCmd
		mov		Render,eax						; Start rendering.
		mov		FBHardwareWriteMask,0ffffffffh	; Restore mask.

pcb8_finish:                                ; Normal exit point
		WaitFifoIfNeeded 4, edx
		mov		eax,dwFBReadMode
		mov		FBReadPixel, GLINTDEPTH8	; Restore framebuffer depth.
		mov		FBReadMode,eax				; Restore partial products
        mov     LogicalOpMode, 20h
        mov     dY, 010000h


        ENABLE_FIFO_CHECKING

		jmp		BSXUnexcludeCursor			; Return to caller



;--------------------------------------------------------------------------
; permediaCopyBlt16Bpp
;
; Screen to screen bitblt on 16bit framebuffer - Optimized for Permedia.
;
; If Src and Dest x-offsets are 32 bit aligned to each other the framebuffer
; depth is temporarily set to 32bits giving approx x2 speedup over a packed copy.
; If not then a normal packed copy is performed.
;
; Assumptions:	Framebuffer is 16Bpp.
;				src = screen memory.
;				dest = screen memory.
;				src and dest are same depth.
;				No palette translation is required.
;				Resolution is not 800x600.
;--------------------------------------------------------------------------
		BEST_ALIGN
PLABEL	permediaCopyBlt16Bpp	near

		assumes ds,_INIT
		assumes es,nothing
		assumes fs,nothing
		assumes gs,nothing

if 0
		TEMPORARILY DISABLED

		mov		ax,[ebp].SrcxOrg
		sub		ax,[ebp].DestxOrg
		test	ax,1						; Are Src and Dest suitably aligned?
		jz		@f							; Yes: use faster algorithm.
endif
		jmp		permediaCopyBlt				; No: use standard slow (x1) routine.

if 0
		; If necessary perform Permedia context switch.
		;
        CLIPSRCTODST
        EXCLUDESWCURSOR
	    DISPLAY_CONTEXT

		; Inform other display driver routines that registers will change.
		;
	    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

		; SOME CODE SHOULD GO IN HERE!

		jmp		BSXUnexcludeCursor		; Return to caller
endif


;--------------------------------------------------------------------------
; permediaCopyBlt
;
; Screen to screen packed bitblt for Permedia.
; This routine is usually called when its faster bretheren
; (permediaCopyBlt8Bpp and permediaCopyBlt16Bpp) cannot be used.
;--------------------------------------------------------------------------
	BEST_ALIGN

PLABEL	permediaCopyBlt	near

	assumes ds,_INIT
	assumes es,nothing
	assumes fs,nothing
	assumes gs,nothing


    CLIPSRCTODST
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
    ;DISABLE_DISCONNECT ds

    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

    mov     ecx, [ebp].lpDestDev

	GetFifoCount eax

    cmp     ecx, GLInfo.dwCurrentOffscreen
    je      PCB_SameOffscreen
	xor		ebx,ebx
    les     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, ecx
    mov     edx, es:[ebx].OSB_PixelOffset

	WaitFifoIfNeeded 1,eax
    mov     FBWindowBase, edx								; - 1

PCB_SameOffscreen:

; some standard setups
	mov		bx,[ebp].xExt 				;get the width into BX
	mov     cx,[ebp].yExt               ;get the height into CX

    WaitFifoIfNeeded 13,eax
	xor		eax,eax
	mov		al,byte ptr RopFunction;restore saved function
	mov     LogicalOpMode,eax		;set mode				; - 1

;
;At this point:
;       BX has X-extent.
;       CX has Y-extent.
;
    mov     Count,ecx               ;y extent				; - 2
    mov     edi,dwFBReadMode        ;get read mode
	;get 16 bits but faster
	mov		ecx, dword ptr wBppMask
    mov     eax,RopFunction
    xor     al,al
    or      edi,eax
    test    ah,ROP_REQ_SRC SHR 8
    jz      PCB_NoSource              ;j if no source
    xor     eax, eax                ;Zero source if we are logicop ~D as we
    cmp     bptr [ebp].Rop+2,055h         ;have converted to ~S so that TX spans work
    je      PCB_LoadSourceOffset
	;then 2 disguised 16 bit loads
    mov     esi,dword ptr [ebp].SrcxOrg              ;find source offset in x
	mov		edx,dword ptr [ebp].DestxOrg
	not		ecx						; cx = ~mask
    mov     eax,dword ptr [ebp].SrcyOrg              ;find source offset in y
	and		esi, ecx
	and		edx, ecx
	;really only subtract the bottom 16 bits here...
    sub     eax,dword ptr [ebp].DestyOrg
    sub     esi,edx
	shl		eax,16
	sar		eax,16
    mov		edx,dword ptr wScreenWidth        ;width of screen
	and		edx,0ffffh
    mul     edx
	shl		esi,16
	sar		esi,16                  ;include x offset
    add     eax,esi
	;disguised 16 bit load
    mov     edx, dword ptr [ebp].DestxOrg
	not		ecx						; cx = mask
	;another 16 bit load here
    mov     esi, dword ptr [ebp].SrcxOrg
    and     edx, ecx
    and     esi, ecx
    sub     edx, esi
    and     edx, 7
    shl     edx, 20
    or      edi, edx                ; or in offset bits
; Check for off screen bitmap blts from one bitmap to another
    mov     ecx, [ebp].lpDestDev
    cmp     ecx, [ebp].lpSrcDev
    je      PCB_LoadSourceOffset
	xor		esi,esi
    les     si, [ebp].lpDestDev
    sub     eax, es:[esi].OSB_PixelOffset
    les     si, [ebp].lpSrcDev
    add     eax, es:[esi].OSB_PixelOffset
PLABEL PCB_LoadSourceOffset
    mov     FBSourceOffset,eax								; - 3
PLABEL PCB_NoSource
    or      edi, 80000h
    mov     FBReadMode,edi          ;set read mode as required -4

;
;Now we construct the block move command taking into account the direction of
;the move. eax contains the source offset still:
;
	mov		cl, GLInfo.bPixelToWordShift
	;get 16 bits plus rubbish
	mov		edi, dword ptr [ebp].DestxOrg			; Get X left
	and		edi,0ffffh					;dump the rubbish
	;16 bits quickly please
	mov		esi, dword ptr [ebp].DestyOrg			; Get Y org (Y top)
	shl		esi,16
	add		ebx, edi					; Work out X right
    shr     edi, cl
    add     ebx, 7
    shr     ebx, cl
	shl     edi,16					; Shift to Glint coords
	;get 16 bits quickly
	mov		ecx, dword ptr [ebp].yExt
	and		ecx,0ffffh
	shl     ebx,16
	dec		ecx						; Decrease Y limit so we render on the right line
	shl		ecx,16
	mov		edx, CopyBltCmd			; Get the render command

; eax = sourceoffset
; edi = X left, ebx = X right, cx = Y limits, edx = render command, si = Y Top
	cmp		eax, 0
	jl		PCB_SourceOffsetNegative

; Use left to right and top to bottom
	mov     StartXDom,edi									; - 5
	mov     StartXSub,ebx									; - 6
	mov		StartY, esi										; - 7

	;then two more disguised 16 bit loads
    mov     eax, dword ptr [ebp].DestxOrg
    mov     ecx, dword ptr [ebp].xExt
    mov     ebx, eax
    add     ebx, ecx
    shl     eax, 16
	and		ebx,0ffffh
    or		eax, ebx
    mov     PackedDataLimits, eax							; - 8

	mov		Render,edx					;go for it			; - 9
    mov     edx, dwFBReadMode               ;get read mode
    mov     FBReadMode, edx
    mov     LogicalOpMode, 20h          					; - 10

    ENABLE_FIFO_CHECKING

	jmp		BSXUnexcludeCursor			;and return to caller

	BEST_ALIGN

PCB_SourceOffsetNegative:

; Use right to left and bottom to top
	add		ecx, esi					; work out Y bottom 
	mov     StartXDom,ebx									; - 5
	mov     StartXSub,edi									; - 6
	mov		dY,0ffff0000h	    			; -1				; - 7
	mov		StartY, ecx										; - 8

	;two 16 bit loads really
    mov     eax, dword ptr [ebp].DestxOrg
    mov     ecx, dword ptr [ebp].xExt
	and		eax,0ffffh
	and		ecx,0ffffh
    mov     ebx, eax
    add     ebx, ecx
    shl     ebx, 16
    or		ebx, eax
    mov     PackedDataLimits, ebx							; - 9

	mov	    Render,edx					;go for it			; - 10
    mov     edx, dwFBReadMode               ;get read mode
    mov     FBReadMode, edx									; - 11
    mov     LogicalOpMode, 20h								; - 12
	mov		dY, 10000h					; +1	; - 13

    ENABLE_FIFO_CHECKING

	jmp		BSXUnexcludeCursor			;and return to caller
;--------------------------------------------------------------------------
; P2CopyBlt
;
; Screen to screen packed bitblt for P2.
;--------------------------------------------------------------------------
	BEST_ALIGN

PLABEL	P2CopyBlt	near

	assumes ds,_INIT
	assumes es,nothing
	assumes fs,nothing
	assumes gs,nothing


    CLIPSRCTODST
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
    ;DISABLE_DISCONNECT ds

    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

    mov     ecx, [ebp].lpDestDev

	GetFifoCount eax

    cmp     ecx, GLInfo.dwCurrentOffscreen
    je      P2CB_SameOffscreen
	xor		ebx,ebx
    les     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, ecx
    mov     edx, es:[ebx].OSB_PixelOffset

	WaitFifoIfNeeded 1,eax
    mov     FBWindowBase, edx								; - 1

P2CB_SameOffscreen:

; some standard setups
	mov		bx,[ebp].xExt 				;get the width into BX
	mov     cx,[ebp].yExt               ;get the height into CX

    WaitFifoIfNeeded 13,eax
	xor		eax,eax
	mov		al,byte ptr RopFunction;restore saved function
	mov     LogicalOpMode,eax		;set mode				; - 1

;
;At this point:
;       BX has X-extent.
;       CX has Y-extent.
;
    mov     edi,dwFBReadMode        ;get read mode
    mov     eax,RopFunction
    xor     al,al                   ;Clear out ROP, leaving source/dest reads
    mov     Count,ecx               ;y extent
    or      edi, eax                ;OR reads into read mode
    or      edi, 80000h             ;Also or in packed bit
    mov     FBReadMode,edi          ;set read mode as required -4

	;get 16 bits but faster
	mov		ecx, dword ptr wBppMask
    test    ah,ROP_REQ_SRC SHR 8
    jz      P2CB_NoSource           ;j if no source
    xor     eax, eax                ;Zero source if we are logicop ~D as we
    cmp     bptr [ebp].Rop+2,055h         ;have converted to ~S so that TX spans work
    je      P2CB_LoadSourceOffset
	;then 2 disguised 16 bit loads
    mov     esi,dword ptr [ebp].SrcxOrg              ;find source offset in x
	mov		edx,dword ptr [ebp].DestxOrg
	not		ecx						; cx = ~mask
    mov     eax,dword ptr [ebp].SrcyOrg              ;find source offset in y
	and		esi, ecx
	and		edx, ecx
	;really only subtract the bottom 16 bits here...
    sub     eax,dword ptr [ebp].DestyOrg
    sub     esi,edx
	shl		eax,16
	sar		eax,16
    mov		edx,dword ptr wScreenWidth        ;width of screen
	and		edx,0ffffh
    mul     edx
	shl		esi,16
	sar		esi,16                  ;include x offset
    add     eax,esi
	;disguised 16 bit load
    mov     edx, dword ptr [ebp].DestxOrg
	not		ecx						; cx = mask
	;another 16 bit load here
    mov     esi, dword ptr [ebp].SrcxOrg
    and     edx, ecx
    and     esi, ecx
    sub     edx, esi
    mov     ecx, [ebp].lpDestDev
    and     edx, 7
    shl     edx, 29                 ; In the right place for packed limits register
	xor		esi,esi
; Check for off screen bitmap blts from one bitmap to another
    cmp     ecx, [ebp].lpSrcDev
    je      P2CB_LoadSourceOffset
    les     si, [ebp].lpDestDev
    sub     eax, es:[esi].OSB_PixelOffset
    les     si, [ebp].lpSrcDev
    add     eax, es:[esi].OSB_PixelOffset
PLABEL P2CB_LoadSourceOffset
    mov     FBSourceOffset,eax								; - 3
PLABEL P2CB_NoSource

;
;Now we construct the block move command taking into account the direction of
;the move. eax contains the source offset still:
;
	mov		cl, GLInfo.bPixelToWordShift
	;get 16 bits plus rubbish
	mov		edi, dword ptr [ebp].DestxOrg			; Get X left
	and		edi,0ffffh					;dump the rubbish
	;16 bits quickly please
	mov		esi, dword ptr [ebp].DestyOrg			; Get Y org (Y top)
	shl		esi,16
	add		ebx, edi					; Work out X right
    shr     edi, cl
    add     ebx, 7
    shr     ebx, cl
	shl     edi,16					; Shift to Glint coords
	;get 16 bits quickly
	mov		ecx, dword ptr [ebp].yExt
	and		ecx,0ffffh
	shl     ebx,16
	dec		ecx						; Decrease Y limit so we render on the right line
	shl		ecx,16

; eax = sourceoffset
; edi = X left, ebx = X right, cx = Y limits, edx = offset bits, si = Y Top
	cmp		eax, 0
	jl		P2CB_SourceOffsetNegative

; Use left to right and top to bottom
	mov     StartXDom,edi									; - 5
	mov     StartXSub,ebx									; - 6
	mov		StartY, esi										; - 7

	;then two more disguised 16 bit loads
    mov     eax, dword ptr [ebp].DestxOrg
    mov     ecx, dword ptr [ebp].xExt
    mov     ebx, eax
    add     ebx, ecx
    shl     eax, 16
	and		ebx,0ffffh
    or		eax, ebx
	mov		edi, CopyBltCmd			; Get the render command
    or      eax, edx                ; add in the offset field
    mov     PackedDataLimits, eax							; - 8

	mov		Render,edi					;go for it			; - 9
    mov     edx, dwFBReadMode               ;get read mode
    mov     FBReadMode, edx
    mov     LogicalOpMode, 20h          					; - 10

    ENABLE_FIFO_CHECKING

	jmp		BSXUnexcludeCursor			;and return to caller

	BEST_ALIGN

P2CB_SourceOffsetNegative:

; Use right to left and bottom to top
	add		ecx, esi					; work out Y bottom 
	mov     StartXDom,ebx				
	mov     StartXSub,edi				
	mov		dY,0ffff0000h	    		
	mov		StartY, ecx					

	;two 16 bit loads really
    mov     eax, dword ptr [ebp].DestxOrg
    mov     ecx, dword ptr [ebp].xExt
	and		eax,0ffffh
	and		ecx,0ffffh
    mov     ebx, eax
    add     ebx, ecx
    shl     ebx, 16
    or		eax, ebx
	mov		edi, CopyBltCmd			    ; Get the render command
    or      eax, edx                    ; add in the offset field
    mov     PackedDataLimits, eax							; - 9

	mov	    Render,edi					;go for it			; - 10
    mov     edx, dwFBReadMode               ;get read mode
    mov     FBReadMode, edx									; - 11
    mov     LogicalOpMode, 20h								; - 12
	mov		dY, 10000h					; +1	; - 13

    ENABLE_FIFO_CHECKING

	jmp		BSXUnexcludeCursor			;and return to caller
;--------------------------------------------------------------------------
; P2CopyBlt24bpp
;
; Screen to screen packed bitblt for Permedia at 24bpp.
;--------------------------------------------------------------------------
	BEST_ALIGN

PLABEL	P2CopyBlt24bpp	near

	assumes ds,_INIT
	assumes es,nothing
	assumes fs,nothing
	assumes gs,nothing


    CLIPSRCTODST
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
    ;DISABLE_DISCONNECT ds

    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

    mov     ecx, [ebp].lpDestDev

	GetFifoCount eax

    cmp     ecx, GLInfo.dwCurrentOffscreen
    je      P2CB24_SameOffscreen
	xor		ebx,ebx
    les     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, ecx
    mov     edx, es:[ebx].OSB_PixelOffset

	WaitFifoIfNeeded 1,eax
    mov     FBWindowBase, edx								; - 1

P2CB24_SameOffscreen:

    WaitFifoIfNeeded 13,eax
    mov     edi,dwFBReadMode                ;get read mode
    mov     ebx, DWORD PTR [ebp].yExt       ;get height
	xor		edx,edx
    or      edi, ROP_REQ_SRC + 80000h       ;Source reads and packed
	mov     LogicalOpMode,edx 		        ;set mode
    and     ebx, 0ffffh                     ;remove rubish from height
    mov     FBReadMode,edi                  ;set read mode as required -4
    mov     Count,ebx                       ;y extent

    ; RelativeOffset (esi) = (Dx - DstX - Sx + SrcX) & 7
    ; SourceOffset = SrcX - DstX + screenWidth * (Sy - Dy)

    mov     edx,dword ptr [ebp].SrcxOrg     ; find Sx   (top 16 bits undefined)
	mov		edi,dword ptr [ebp].DestxOrg    ; And Dx    (top 16 bits undefined)
    mov     esi, dword ptr [ebp].DestyOrg   ; Dy
    mov     eax, dword ptr [ebp].SrcyOrg    ; Sy
    mov     ebx, edx
    mov     ecx, edi
    and     eax, 0ffffh                     ; clear out rubish from top 16 bits
    and     esi, 0ffffh                     ; clear out rubish from top 16 bits
    sub     eax, esi                        ; eax = Sy - Dy (Sy, Dy no longer needed)
    and     ebx, 0FFFCh                     ; Round Sx down to 4 pixel boundary. SrcX
    mov     esi, edi                        ; esi = Dx
    and     ecx, 0FFFCh                     ; Round Dx down to 4 pixel boundary. DstX
    add     esi, ebx                        ; esi = DX + SrcX
    sub     ebx, ecx                        ; ebx = SrcX - DstX (SrcX no longer needed)
    sub     esi, edx                        ; esi = DX - Sx + SrcX (Sx no longer needed)
    mov     edx, GLInfo.dwScreenWidth
    sub     esi, ecx                        ; esi = DX - DstX - Sx + SrcX
    mul     edx                             ; eax = screenWidth * (Sy - Dy)
    shl     esi, 29                         ; esi = RelOffset ready to & PackedDataLimits
    add     eax, ebx                        ; eax = SourceOffset

; Check for off screen bitmap blts from one bitmap to another
    mov     ebx, [ebp].lpDestDev
    cmp     ebx, [ebp].lpSrcDev
    je      P2CB24_LoadSourceOffset
    push    esi
	xor		esi,esi
    les     si, [ebp].lpDestDev
    sub     eax, es:[esi].OSB_PixelOffset
    les     si, [ebp].lpSrcDev
    add     eax, es:[esi].OSB_PixelOffset
    pop     esi
P2CB24_LoadSourceOffset:
    mov     FBSourceOffset, eax

;
;Now we construct the block move command taking into account the direction of
;the move. eax contains the source offset still:
;

; eax = SourceOffset, ecx = DstX, esi = offset, edi = Dx

    cmp     eax, 0
    jl      P2CB24_SourceOffsetNegative

; Set up left to right and top to bottom copy
    mov     ebx, edi
    mov     edx, DWORD PTR [ebp].xExt           ; xExt in bottom 16 bits
    shl     edi, 16                             ; Dx in top 16 bits
    and     edx, 0ffffh                         ; Clear top bitsd of edx
    add     edi, ebx                            ; edi = (Dx << 16) + Dx
    add     edi, edx                            ; edi = (Dx + xExt << 16) + Dx. 
    or      edi, esi                            ; Add in offset
    mov     PackedDataLimits, edi

; eax = SourceOffset, ecx = DstX, edx = xExt
    add     edx, ecx
    shl     ecx, 14                             ; ecx = StartXDom / 4
    mov     edi, dword ptr [ebp].DestyOrg - 2   ; Sy << 16 (ignore fractional bits)
    shl     edx, 14                             ; edx = StartXSub / 4
    mov     ebx, ecx
    mov     esi, edx
    add     ecx, ecx                            ; ecx = StartXDom * 2 / 4
    add     edx, edx                            ; edx = StartXSub * 2 / 4
    add     ecx, ebx                            ; ecx = StartXDom * 3 / 4 = Packed XDom
    add     edx, esi                            ; edx = StartXSub * 3 / 4 = Packed XSub
    mov     StartY, edi

; eax = SourceOffset, ecx = StartXDom, edx = StartXSub, esi = offset, edi = Packed Limits
    mov     ebx, CopyBltCmd
    add     edx, 030000h                        ; Add 3 for worst case blt

    mov     StartXDom, ecx
    mov     StartXSub, edx
    mov     Render, ebx

; Reset the chip back to standard information
    mov     edx, dwFBReadMode               ;get read mode
    mov     LogicalOpMode, 20h          					; - 10
    mov     FBReadMode, edx

	jmp		BSXUnexcludeCursor			;and return to caller

	BEST_ALIGN

P2CB24_SourceOffsetNegative:
; Set up as if left to right and top to bottom copy
    mov     ebx, edi
    mov     edx, DWORD PTR [ebp].xExt - 2       ; xExt in top 16 bits
    shl     edi, 16                             ; Dx in top 16 bits
    and     edx, 0ffff0000h                     ; Clear bottom bits of edx
    add     edi, ebx                            ; edi = (Dx << 16) + Dx
    add     edi, edx                            ; edi = (Dx << 16) + Dx + xExt. 
    or      edi, esi                            ; Add in offset -> Packed data limits

; eax = SourceOffset, ecx = DstX, edx = xExt
    shr     edx, 16
    add     edx, ecx
    shl     ecx, 14                             ; ecx = StartXDom / 4
    mov     PackedDataLimits, edi
    shl     edx, 14                             ; edx = StartXSub / 4
    mov     ebx, ecx
    mov     edi, dword ptr [ebp].DestyOrg - 2   ; Sy << 16 (ignore fractional bits)
    add     ecx, ecx                            ; ecx = StartXDom * 2 / 4
    mov     esi, dword ptr [ebp].yExt - 2       ; yExt << 16
    add     ecx, ebx                            ; ecx = StartXDom * 3 / 4 = Packed XDom
    sub     edi, 010000h                        ; Subtract 1 from Start Y
    mov     ebx, edx
    add     edi, esi
    add     edx, edx                            ; edx = StartXSub * 2 / 4
    mov     StartY, edi
    add     edx, ebx                            ; edx = StartXSub * 3 / 4 = Packed XSub
	mov		dY,0ffff0000h	    		        ; Negative Y
    add     edx, 030000h                        ; Add 3 for worst case blt
    mov     ebx, CopyBltCmd

    mov     StartXDom, edx
    mov     StartXSub, ecx
    mov     Render, ebx

; Reset the chip back to standard information
    mov     edx, dwFBReadMode               ;get read mode
    cmp     eax, 0
    ja      @f
	mov		dY,0ffff0000h	    		        ; Back to standard Positive Y
@@:    
    mov     LogicalOpMode, 20h          					; - 10
    mov     FBReadMode, edx
	mov		dY,010000h	    		        ; Back to standard Positive Y


	jmp		BSXUnexcludeCursor			;and return to caller

;--------------------------------------------------------------------------
; permediaROPCopyBlt
; permedia can only handle ROPS with packed blts if the alignment of
; the source and dest are aligned to 32 bits. Check for this, then punt to
; the appropriate procedure. 
;--------------------------------------------------------------------------
		BEST_ALIGN
PLABEL permediaROPCopyBlt8

		;two hidden 16 bit quantities
		mov		eax,dword ptr [ebp].SrcxOrg
		sub		eax,dword ptr [ebp].DestxOrg
		test	ax,3					; Are Src and Dest 32bit aligned?
		jz		@f				        ; Yes: use fast algorithm.
                                        ; no: use slow method.
        jmp     glintCopyBlt

@@:	    jmp		permediaCopyBlt8Bpp

;--------------------------------------------------------------------------
; permediaROPCopyBlt
; permedia can only handle ROPS with packed blts if the alignment of
; the source and dest are aligned to 32 bits. Check for this, then punt to
; the appropriate procedure. 
;--------------------------------------------------------------------------
		BEST_ALIGN
PLABEL P2ROPCopyBlt8

		;two hidden 16 bit quantities
		mov		eax,dword ptr [ebp].SrcxOrg
		sub		eax,dword ptr [ebp].DestxOrg
		test	ax,3					; Are Src and Dest 32bit aligned?
		jz		@f				        ; Yes: use fast algorithm.
        jmp     glintCopyBlt

@@:	    jmp		P2CopyBlt

;--------------------------------------------------------------------------
		BEST_ALIGN
PLABEL permediaROPCopyBlt16

		;two hidden 16 bit quantities
		mov		eax,dword ptr [ebp].SrcxOrg
		sub		eax,dword ptr [ebp].DestxOrg
		test	ax,1					; Are Src and Dest 32bit aligned?
		jz		@f				        ; Yes: use faster algorithm.
                                        ; no: use slower method.
        jmp     glintCopyBlt

@@:	    jmp		permediaCopyBlt

;--------------------------------------------------------------------------
		BEST_ALIGN
PLABEL P2ROPCopyBlt16

		;two hidden 16 bit quantities
		mov		eax,dword ptr [ebp].SrcxOrg
		sub		eax,dword ptr [ebp].DestxOrg
		test	ax,1					; Are Src and Dest 32bit aligned?
		jz		@f				        ; Yes: use faster algorithm.
                                        ; no: use slower method.
        jmp     glintCopyBlt

@@:	    jmp		P2CopyBlt



;--------------------------------------------------------------------------
; permediaSolidMonoBrush
; First use block fill to draw the background, then use the stipple unit to
; draw the foreground.
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL permediaSolidMonoBrush           ;es:di = lpPBrush 

    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds
    
    mov     DD_CurrentContext, DD_SOLIDBLT_CTXT

    mov     eax, [ebp].lpDestDev

	GetFifoCount edi		;doesn't affect the zero flag (or any others)

    cmp     eax, GLInfo.dwCurrentOffscreen
    je      pSMB_SameOffscreen
	xor		ebx,ebx
    lfs     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, fs:[ebx].OSB_PixelOffset

	WaitFifoIfNeeded 1,edi
    
	mov     FBWindowBase, edx
pSMB_SameOffscreen:

	; The next three intructions are only really loading the bottom 16 bits of their regs!
    mov     ecx, dword ptr [ebp].yExt                ; ecx = count
    mov     eax, dword ptr [ebp].DestxOrg ; Get xStart
    mov     ebx, dword ptr [ebp].xExt     ; Get Width
    add     ebx, eax                  ; bx = xEnd
	and		ecx,0ffffh
    shl     eax, 16                 ; Convert to 16.16
    shl     ebx, 16                 ; Convert to 16.16
    
ifdef SOME_DISCONNECT
	WaitFifoIfNeeded 7,edi
	mov		fifoSpace,edi			;and record how much is left
endif

    mov     Count, ecx              ; calculated earlier
    mov     StartXDom, eax
    mov     StartXSub, ebx

	xor		edi,edi
	xor		esi,esi
    les     di, [ebp].lpDrawMode
	;Only 16 bits really fetched for this next one too
    mov     ebx, dword ptr [ebp].DestyOrg            ; get yStart
    shl     ebx, 16                 ; Shift by 16
    mov     al, byte ptr wBpp
    mov     StartY, ebx

    mov     edx, es:[edi].bkColor	; Get background color and replicate
    cmp     al, 24
    jae     pSMB_BGReplicated
	and		edx,0ffffh				;ensure top word is zero
    cmp     al, 8
    jne     @f
    mov     dh, dl
@@:
    mov     eax, edx
    shl     edx, 16
    or		edx, eax
pSMB_BGReplicated:
    mov     eax, FillRectCmd        ; has fast fill etc for a solid fill
    mov     FBBlockColor,edx
    mov     FBWriteData,edx			
    mov     Render,eax              ; Render a rectangle in background color
    mov     edx, es:[edi].TextColor  ; Get foreground color
    cmp     esi, 24                  ; Replicate it
    jae     pSMB_FGReplicated
	and		edx,0ffffh
    cmp     esi, 8
    jne     @f
    mov     dh, dl
@@:
    mov     eax, edx
    shl     edx, 16
	and		eax, 0ffffh
    or		edx, eax
pSMB_FGReplicated:
	
	WaitFifoIfNeededVarReg 12,fifoSpace,edi

    mov     FBBlockColor,edx
	xor		edi,edi
    les     di, [ebp].lpPBrush
    add     di, dp8BrushMono        ;es:di points to mono data
    mov     FBWriteData,edx			;
	mov		DD_CurrentColour,edx

	mov		AreaStippleMode, 060001h
	mov		eax, es:[edi]
	mov		ebx, es:[edi+4]
	mov		ecx, es:[edi+8]
	mov		edx, es:[edi+12]
	mov		AreaStipplePattern0, eax
	mov		AreaStipplePattern1, ebx
	mov		AreaStipplePattern2, ecx
	mov		AreaStipplePattern3, edx
	mov		eax, es:[edi+16]
	mov		ebx, es:[edi+20]
	mov		ecx, es:[edi+24]
	mov		edx, es:[edi+28]
	mov		AreaStipplePattern4, eax
	mov		AreaStipplePattern5, ebx
	mov		AreaStipplePattern6, ecx
	mov		AreaStipplePattern7, edx

    mov     edx, SolidMonoBrushCmd  ;has fast fill etc
    mov     Render,edx              ; Render a rectangle in background color


	jmp		BSXUnexcludeCursor			;and return to caller


;--------------------------------------------------------------------------
; permediaROPMonoBrush
; Use the stipple unit to generate forground and background colours.
; The Logicop unit can then combine the results.
; NOTE: This should not be called for operations which dont involve the source.
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL permediaROPMonoBrush           ;es:di = lpPBrush 

    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds

	GetFifoCount esi

    ; Check to see if color changed or context changed.
    cmp     DD_CurrentContext, DD_ROPMONOBRUSH_CTXT
    je      pRMB_DontChangeContext
	
	WaitFifoIfNeeded 1,esi

    mov     DD_CurrentContext, DD_ROPMONOBRUSH_CTXT
	mov		AreaStippleMode, 0160001h
pRMB_DontChangeContext:

	xor		edi,edi
    mov     eax, [ebp].lpDestDev
    cmp     eax, GLInfo.dwCurrentOffscreen
    je      pRMB_SameOffscreen
	xor		ebx,ebx
    lfs     bx, [ebp].lpDestDev
    mov     edx, fs:[ebx].OSB_PixelOffset
    mov     GLInfo.dwCurrentOffscreen, eax
	
	WaitFifoIfNeeded 1,esi

    mov     FBWindowBase, edx
pRMB_SameOffscreen:

    les     di, [ebp].lpDrawMode

	WaitFifoIfNeeded 9,esi

    mov     PTextureReadMode, 0         ;Turn off texture read mode to load Texel0
    mov     ColorDDAMode, 1				;enable CDDA

    mov     edx, es:[edi].bkColor		; Get background color
    mov     ecx, dword ptr [ebp].yExt	; ecx = count
    mov     eax, es:[edi].TextColor		; Get foreground color

    mov     Texel0,edx
    mov     ConstantColor, eax

	;Next two instructions are really 16 bits loads
    mov     eax, dword ptr [ebp].DestxOrg ; Get xStart
    mov     ebx, dword ptr [ebp].xExt     ; Get Width
    and     ecx, 0ffffh
    add     ebx, eax                ; bx = xEnd
    shl     eax, 16                 ; Convert to 16.16
    mov     Count, ecx              ; calculated earlier
    shl     ebx, 16                 ; Convert to 16.16
    mov     StartXDom, eax
    mov     StartXSub, ebx

	; Really a 16 bit load, this next one...
    mov     ebx, dword ptr [ebp].DestyOrg      ; get yStart
	mov		eax,RopFunction			;restore saved function
    shl     ebx, 16                 ; Shift by 16
    mov     edx,dwFBReadMode        ;get read mode
	mov     LogicalOpMode,eax		;set mode
    and     eax, ROP_REQ_DST
    les     di, [ebp].lpPBrush
    or      edx,eax
    mov     StartY, ebx
    mov     FBReadMode,edx          ;set read mode as required

DP8BM = 14							;offset into brush structure of dp8BushMono

	mov		eax, es:[edi + DP8BM]
	mov		ebx, es:[edi + DP8BM+4]
	mov		ecx, es:[edi + DP8BM+8]
	mov		edx, es:[edi + DP8BM+12]

	WaitFifoIfNeeded 8,esi

	mov		AreaStipplePattern0, eax
	mov		AreaStipplePattern1, ebx
	mov		AreaStipplePattern2, ecx
	mov		AreaStipplePattern3, edx
	mov		eax, es:[edi + DP8BM+16]
	mov		ebx, es:[edi + DP8BM+20] 
	mov		ecx, es:[edi + DP8BM+24]
	mov		edx, es:[edi + DP8BM+28]
	mov		AreaStipplePattern4, eax
	mov		AreaStipplePattern5, ebx
	mov		AreaStipplePattern6, ecx
	mov		AreaStipplePattern7, edx

	WaitFifoIfNeeded 5,esi

    mov     edx, ROPMonoBrushCmd    ; has fast fill etc
    mov     eax,dwFBReadMode        ; get read mode

    mov     Render,edx              ; Render a rectangle in background color

    mov     ColorDDAMode,0          ; disable CDDA
    mov     LogicalOpMode, 20h
    mov     PTextureReadMode, DEFAULT_P2TEXTUREREADMODE

    mov     FBReadMode,eax          ; set read mode as required
	jmp		BSXUnexcludeCursor		;and return to caller


;--------------------------------------------------------------------------
; P2MonoROPImageDownload
; Download Monochrome brush to P2 whilst performing a ROP with the background.
; The code aligns source reads to 32 bits and uses the scissor clip in P2 to discard
; the leftmost pixels
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL P2MonoROPImageDownload           ;es:di = lpPBrush 

    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    EXCLUDESWCURSOR
    DISPLAY_CONTEXT

    mov     eax, [ebp].lpDestDev
	xor		edi,edi
    mov     DD_CurrentContext, DD_UNDEFINED_CTXT
    cmp     eax, GLInfo.dwCurrentOffscreen
    je      P2MRID_SameOffscreen
	xor		ebx,ebx
    lfs     bx, [ebp].lpDestDev
    mov     edx, fs:[ebx].OSB_PixelOffset
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     FBWindowBase, edx
P2MRID_SameOffscreen:

    les     di, [ebp].lpDrawMode

    mov     PTextureReadMode, 0             ;Turn off texture read mode to load Texel0
    mov     ColorDDAMode, 1				    ;enable CDDA
    mov     ScissorMode, 3
    mov     ScissorMaxXY, 07ff07ffh

    ; Check for specific rop/colour combinations so we can avoid using the Background
    ; Colour. It is faster to draw if the background is not required
    ; xor/or with black = no op
	mov		eax,RopFunction			        ;restore saved function
    mov     ebx,dwRasterizerMode
    mov     edx, es:[edi].bkColor		    ;Get P2 foreground color
    mov     ecx, es:[edi].TextColor		    ;Get P2 background color
    cmp     eax, __GLINT_LOGICOP_COPY
    je      P2MRID_UseBackground
    cmp     eax, __GLINT_LOGICOP_XOR
    je      P2MRID_Xor
    cmp     eax, __GLINT_LOGICOP_OR
    jne     P2MRID_UseBackground
P2MRID_Xor:
    and     ecx, 0ffffffh
    and     edx, 0ffffffh
    test    ecx, ecx                        ; is background zero?
    je      P2MRID_NoBackground
    test    edx, edx                        ; is foreground not zero?
    jne     P2MRID_UseBackground
    or      ebx, 2                          ; Invert bitmask
    mov     edx, ecx                        ; mov background color into foreground
    jmp     P2MRID_NoBackground

P2MRID_UseBackground:
    mov     Texel0,ecx                      ;Load Background colour
    or      ebx, 40h                        ;ForceBackgroundColour

P2MRID_NoBackground:
    mov     RasterizerMode, ebx
	mov     LogicalOpMode,eax		        ;set mode
    mov     ebx,dwFBReadMode                ;get read mode
    and     eax, ROP_REQ_DST
    mov     ConstantColor, edx              ;Load foreground colour
    or      ebx,eax
   	xor		esi,esi
    mov     FBReadMode,ebx                  ;set read mode as required

    les     si, [ebp].lpSrcDev              ;es:si is PBITMAP structure

    xor     eax, eax
    mov     ax, [ebp].SrcxOrg
    xor     ecx, ecx
    mov     cx, es:[esi].deWidth
    xor     ebx, ebx
    mov     bx, [ebp].SrcyOrg
    xor     edx, edx
    mov     dx, es:[esi].deHeight
    sub     ecx, eax                        ; work out width of bitmap remaining
    jle     P2MRID_DownloadDone
    sub     edx, ebx                        ; work out height of bitmap remaining
    jle     P2MRID_DownloadDone
    xor     eax, eax
    mov     ax, [ebp].xExt
    xor     ebx, ebx
    mov     bx, [ebp].yExt
    cmp     eax, ecx
    jle     @f
    mov     eax, ecx
@@:
    cmp     ebx, edx
    jle     @f
    mov     ebx, edx
@@:
    shl     ebx, 16                         ; ebx = height << 16
    or      ebx, eax                        ; ebx = height << 16 | width

    mov     eax,dword ptr [ebp].DestyOrg    ;X << 16 | Y coords
    ror     eax, 16                         ;P2 needs Y << 16 | X 
    mov     ecx,dword ptr [ebp].SrcyOrg     ;Src X << 16 | Y coords
    mov     ScissorMinXY, eax
    ror     ecx, 16
    mov     edx, P2MonoROPImageDownloadCmd  ;has direction for P2 and SyncOnBitMask bit
    and     ecx, 01fh                       ;Need to reduce X by anough pixels to align source read
    sub     ax, cx                          ;reduce X. Dont trash Y though (use 16bits!)
    add     ebx, ecx                        ;increase width
    mov     RectangleOrigin,eax
    mov     ecx, ebx
    mov     RectangleSize,ebx
    add     ebx, 01fh                       ; round up width
    mov     Render,edx                      ;do it
    shr     ecx, 16                         ;work out number of scanlines
    and     ebx, 0ffffh                     ;remove Y from top
    shr     ebx, 5                          ;convert to number of dwords per scanline
    mov     DestHeight, ecx                 ;save height 
    mov     dwSrcWords, ebx                 ;

; We now need to download the monochrome data to the chip. This could be in one of 2 formats
; either PBITMAP or DIBENG. PBITMAP could be multisegmented!

    mov     eax,dword ptr [ebp].SrcyOrg     ;Src X << 16 | Y coords
    mov     ebx, eax
    shr     ebx, 16                         ;ebx = Src X
    and     ebx, 0ffe0h                     ;ebx = src X aligned to 32 bits
    and     eax, 0ffffh                     ;eax = Src Y
    shr     ebx, 3                          ;ebx = byte offset into source

    cmp     es:[esi].deType, TYPE_DIBENG
    jne     P2MRID_SourcePBITMAP
    mov     ecx, es:[esi].deDeltaScan       ;Scanline width
    lfs     edi, es:[esi].deBits            ;fs:edi is pointer to data
    xor     edx, edx
    mov     dwDelta, ecx                    ; preserve for later use
    mov     dwSegmentOffset, edx
    dec     edx
    mov     dwLinesInSegment, edx
    mov     dwLinesLeftInSegment, edx
    jmp     P2MRID_SourceInESEDI
P2MRID_SourcePBITMAPSingleSegment:
    xor     ecx, ecx
    mov     cx, es:[esi].bmWidthBytes
    xor     edi, edi
    lfs     di, es:[esi].bmBits             ; fs:edi is pointer to data
    xor     edx, edx
    mov     dwDelta, ecx                    ; preserve for later use
    mov     dwSegmentOffset, edx
    dec     edx
    mov     dwLinesInSegment, edx
    mov     dwLinesLeftInSegment, edx
    jmp     P2MRID_SourceInESEDI
P2MRID_SourcePBITMAP:
    xor     ecx, ecx
    mov     cx, es:[esi].bmSegmentIndex
    test    ecx, ecx
    je      P2MRID_SourcePBITMAPSingleSegment
    xor     edx, edx
    mov     dx, es:[esi].bmScanSegment
    mov     dwSegmentOffset, ecx
    mov     dwLinesInSegment, edx
    xor     edi, edi
    mov     di, WORD PTR es:[esi].bmBits    ; get offset
    xor     ecx, ecx
    mov     cx, WORD PTR es:[esi].bmBits+2  ; get segment
@@:
    cmp     eax, edx
    jb      P2MRID_SourcePBITMAPCorrectSegment
    sub     eax, edx                        ; reduce Y by one segment
    add     ecx, dwSegmentOffset            ; advance segment
    xor     edi, edi
    jmp     @b
P2MRID_SourcePBITMAPCorrectSegment:
    sub     edx, eax                        ; work out lines left in this segment
    mov     dwLinesLeftInSegment, edx       
    mov     fs, ecx                         ; set up segment
    xor     ecx, ecx
    mov     cx, es:[esi].bmWidthBytes
    mov     dwDelta, ecx                    ; preserve for later use
P2MRID_SourceInESEDI:
    mov     dwFirstInNextSegment, ebx       ; preserve start of next segment
    mov     ecx, dwDelta
    mul     ecx                             ; y * dwDelta
    add     eax, ebx                        ; + x
    add     edi, eax                        ; advance to correct source address
    mov     esi, edi

P2MRID_NextSegment:
    mov     ecx, DestHeight
    cmp     ecx, dwLinesLeftInSegment
    jbe     @f
    mov     ecx, dwLinesLeftInSegment
@@:
    sub     DestHeight, ecx
    sub     dwLinesLeftInSegment, ecx
P2MRID_NextScanline:
    push    esi                             ; preserve start address of scanline
    push    ecx                             ; and scanline count
    mov     ecx, dwSrcWords
    mov     eax, ecx
    dec     eax
    shl     eax, 16
    lea     edi, GPFifo5
    or      eax, 0dh                        ; BitmaskPattern tag
    push    ebp
    mov     GPFifo4, eax                    ; Bitmask hold tag for scanline
    cmp     ecx, 4
    jl      P2MRID_LessThan4DWORDS
    jmp     P2MRID_MoreThan4DWORDS
BEST_ALIGN
P2MRID_MoreThan4DWORDS:
    mov     eax, fs:[esi +  0]
    mov     ebx, fs:[esi +  4]
    mov     edx, fs:[esi +  8]
    mov     ebp, fs:[esi + 12]
    mov     [edi +  0], eax
    mov     [edi +  4], ebx
    mov     [edi +  8], edx
    mov     [edi + 12], ebp
    add     esi, 16
    add     edi, 16
    sub     ecx, 4
    cmp     ecx, 4
    jge     P2MRID_MoreThan4DWORDS
P2MRID_LessThan4DWORDS:
    pop     ebp
    cmp     ecx, 2
    jl      P2MRID_LessThan2DWORDS
    mov     eax, fs:[esi +  0]
    mov     ebx, fs:[esi +  4]
    mov     [edi +  0], eax
    mov     [edi +  4], ebx
    add     esi, 8
    add     edi, 8
    sub     ecx, 2
P2MRID_LessThan2DWORDS:
    cmp     ecx, 1
    jl      P2MRID_ScanlineDone
    mov     eax, fs:[esi +  0]
    mov     [edi +  0], eax
P2MRID_ScanlineDone:
    pop     ecx
    pop     esi
    add     esi, dwDelta                    ; advance to next scanline
    dec     ecx
    jne     P2MRID_NextScanline
    cmp     DestHeight, 0
    je      P2MRID_DownloadDone
    mov     eax, fs
    mov     ebx, dwLinesInSegment
    add     eax, dwSegmentOffset
    mov     dwLinesLeftInSegment, ebx
    mov     esi, dwFirstInNextSegment       ; esi points to first word in next segment
    mov     fs, eax
    jmp     P2MRID_NextSegment

P2MRID_DownloadDone:
    mov     eax,dwFBReadMode                ;get read mode
    mov     ebx,dwRasterizerMode
    mov     ColorDDAMode,0                  ;disable CDDA
    mov     ScissorMode,0                   ;disable scissor
    mov     LogicalOpMode, 20h
    mov     PTextureReadMode, DEFAULT_P2TEXTUREREADMODE
    mov     RasterizerMode, ebx
    mov     FBReadMode,eax                  ;set read mode as required
	jmp		BSXUnexcludeCursor		        ;and return to caller

;--------------------------------------------------------------------------
; ROPFillRect
;
; ROP screen rectangle fill - should be used for all chips.
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  ROPFillRect
    ; eax contains replicated colour

    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    push    eax
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds
    pop     eax

;   eax = colour (replicated)


    mov     edx, [ebp].lpDestDev

	GetFifoCount esi

    cmp     edx, GLInfo.dwCurrentOffscreen
    je      RFR_SameOffscreen
	xor		ebx,ebx
    lfs     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, edx
    mov     edx, fs:[ebx].OSB_PixelOffset

	WaitFifoIfNeeded 1,esi

    mov     FBWindowBase, edx
RFR_SameOffscreen:

    mov     ecx,RopFunction
    and     ecx, ROP_REQ_DST         ;We just want the Destination read flag
    jne     RFR_DoIt
; No destination read - a Solid fill will do just as well
    cmp     RopFunction, __GLINT_LOGICOP_COPY
    je      @f
    not     eax                     ; Must be a source invert
@@:
    jmp     [FillRectFn]

	BEST_ALIGN
RFR_DoIt:
    mov     DD_CurrentContext, DD_ROPFILLRECT_CTXT
; The TX Span operations require a source pixel is produced for each pixel, but
; in this type of operation, the source pixel is constant. Therefore we can invert the
; logical op to read the source pixel rather than the dest pixel and keep the dest constant.
; Some ROPs however are not commutative, so use a table to look up the appropriate
; ROP to perform

	WaitFifoIfNeeded 11,esi

    mov     FBData, eax             ;Save the color in the destination register
    mov     FBSourceOffset, 0

	xor		ebx,ebx
	mov		bl,byte ptr RopFunction	;restore saved function
    mov     edx,dwFBReadMode        ;get read mode
    shr     cx, 1                   ;Convert the read dst to a read src
    mov     bl, [SopDtoDopS + ebx]  ;Convert the ROP
    or      dx,cx                   ;or it into the read mode
	mov     LogicalOpMode,ebx		;set mode
    mov     FBReadMode,edx          ;set read mode as required

	;The next two are really 16 bit loads in disguise again
    mov     eax,dword ptr [ebp].DestxOrg            ;X coords
    mov     ebx,dword ptr [ebp].xExt				;width
	xor		edi, edi				;zero edi
    add     ebx, eax                  ;bx is right edge
    shl     eax,16					;X -> 16.16 format
    mov     esi,dword ptr [ebp].DestyOrg             ;Y coords
    mov     StartXDom,eax			;
    mov		di,[ebp].yExt					;edi -> height
    shl     ebx,16					;Width -> 16.16 format
    shl     esi,16					;Y -> 16.16 format

    mov     StartXSub,ebx			;
    mov     ecx, ROPFillRectCmd     ;has fast fill etc
    mov     Count,edi				;
    mov     StartY,esi				;
    mov     Render,ecx              ; do it

    mov     edx,dwFBReadMode        ;get read mode
	mov     LogicalOpMode,20h		;set mode
    mov     FBReadMode,edx          ;set read mode as required

	jmp	BSXUnexcludeCursor			

;--------------------------------------------------------------------------
; permediaBrushFillRect8
;
; Code to blt brushes onto the screen for Permedia at 8bpp.
; The fastest way to get a pattern replicated all over the screen is to use
; Block Fills. Set up dy to 8 - thus drawing a scanline at a time. It will take
; 8 passes to draw all the scanlines.
; As we only have a 32 bit block fill mask, we need 2 passes to draw each
; scanline. However, if on the first pass we block fill the entire scanline,
; we can check on the second pass to see if the colour has changed, and avoid
; the second pass if this is the case. If we do need a second pass, use Mono
; image download to mask out the first pass data. 
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  permediaBrushFillRect8
    ; eax contains replicated colour
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds

    ; Check to see if color changed or context changed.
    mov     DD_CurrentContext, DD_SOLIDBLT_CTXT

    mov     eax, [ebp].lpDestDev

	GetFifoCount esi

    cmp     eax, GLInfo.dwCurrentOffscreen
    je      pBFR_SameOffscreen
	xor		ebx,ebx
    lfs     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, fs:[ebx].OSB_PixelOffset
	
	WaitFifoIfNeeded 1, esi
    
	mov     FBWindowBase, edx
pBFR_SameOffscreen:

	xor		edi,edi
    les     di, [ebp].lpPBrush
	mov		al, es:[edi].dp8BrushBpp
	cmp		al,8
	jnz		Jmp_DIB_BitBlt
    add     edi, dp8BrushBits        ;es:edi points to mono data

; es:edi = pointer to data

ifdef SOME_DISCONNECT
    WaitFifoIfNeeded 7,esi
	mov		fifoSpace,esi			;and how much is left
endif

    mov     dY, 80000h
    mov     ScissorMode, 3
    mov     ScissorMaxXY, 07ff07ffh

	;get 16 bits the fast way
    mov     eax, dword ptr [ebp].DestxOrg
	and		eax,0ffffh
    mov     ScissorMinXY, eax
    mov     ebx, eax
    add     bx, [ebp].xExt
    and     eax, NOT 7               ; Align start to screen
    mov     ecx, ebx
    sub     ecx, eax                  ; cx is width of area being rendered.
    shl     eax, 16
    add     ecx, 31                  ; Add 31 to round up
    shl     ebx, 16
    mov     StartXDom, eax
    shr     ecx, 5                   ; Convert to number of bitmasks per scanline
    mov     StartXSub, ebx
	mov		dwSrcWords, ecx			 ; hold on to that value in ecx for a moment please

    xor     ebx, ebx
    mov		edx,dword ptr [ebp].yExt
	and		edx,0ffffh
    mov     ebx, edx                ;ax = count
    and     dl, 7                   ;dl = surplus lines. 
    add     ebx, 7
    shr     ebx, 3                  ;ax = count to use for dl surplus lines.
    mov     Count, ebx
    mov     DestHeight, ebx         ; Save away the height
	mov		eax, ebx

	;16 bit load in disguise
    mov     ebx, dword ptr [ebp].DestyOrg      ;Get destination Y
    mov     esi, ebx
    and     ebx, 7                  ;get rotational offset
    shl     esi, 16
    mov     dh, bl                  ;dh is current offset
	lea		edi, [edi + 8 * ebx]    ;edi is now at the correct offset to start rendering.

	push	edx
	mul		ecx
	shl		ecx,16
	mov		nDwords, eax			;eax = width * height
	dec		eax
	mov		wLeft16, ecx
	shl		eax,16
    mov     ecx, 8
	mov		al, 0dh					;eax = ((((width * height) - 1) << 16) | 0dh)
	pop		edx
	mov		holdTag, eax
    jmp     pBFR_NextScanline

	BEST_ALIGN

pBFR_NextScanline:
    mov     eax, FillRectCmd        ;has fast fill etc
	WaitFifoIfNeededVarReg 4,fifoSpace, ebx
    mov     StartY, esi
    mov     ebx, es:[edi]           ;Get color
    mov     FBBlockColor,ebx        
    mov     FBWriteData,ebx			
    mov     DD_CurrentColour, ebx
    mov     Render,eax              ;do first pass

    mov     eax, es:[edi+4]         ;Get second pass color
    cmp     ebx, eax                ;If colors are equal, we don't need a second pass.
    je      pBFR_SecondPassDone
    mov     ebx, BrushFillRectCmd   ;has fast fill, Sync on Bitmask etc
    mov     DD_CurrentColour, eax

    WaitFifoIfNeededVar 3,fifoSpace

    mov     FBBlockColor,eax        
    mov     FBWriteData,eax			
    mov     Render,ebx              ; do second pass
    mov     DD_CurrentColour, eax

ifdef SOME_DISCONNECTS
    push    ecx						; starts as 8
endif
    push    edx

    mov     ebx, holdTag
	mov		eax, nDwords
    mov     edx, 0f0f0f0fh          ; Bitmask to download

ifdef SOME_DISCONNECTS
	mov		ecx,fifoSpace
    WaitFifoIfNeeded 1,ecx
endif

    mov     GPFifo1, ebx
	xor		ebx, ebx				; since ebx is free now, I'll point
    sub     eax, 8					;  into the 4K funny region with it.
    jl      pBFR_LessThan8
pBFR_Next8:
    WaitFifoIfNeeded 8,ecx
    mov     [GPFifo1 + ebx + 00], edx
    mov     [GPFifo1 + ebx + 04], edx
    mov     [GPFifo1 + ebx + 08], edx
    mov     [GPFifo1 + ebx + 12], edx
    mov     [GPFifo1 + ebx + 16], edx
    mov     [GPFifo1 + ebx + 20], edx
    mov     [GPFifo1 + ebx + 24], edx
    mov     [GPFifo1 + ebx + 28], edx
	add		ebx, 32
	and		ebx, 0fffh				;keep ebx within the range
    sub     eax, 8
    jae     pBFR_Next8
pBFR_LessThan8:
	test	al, 4
	jz		pBFR_LessThan4
    WaitFifoIfNeeded 4,ecx
    mov     [GPFifo1 + ebx + 00], edx
    mov     [GPFifo1 + ebx + 04], edx
    mov     [GPFifo1 + ebx + 08], edx
    mov     [GPFifo1 + ebx + 12], edx
	add		ebx, 16
pBFR_LessThan4:
	test	al, 2
	jz		pBFR_LessThan2
    WaitFifoIfNeeded 2,ecx
    mov     [GPFifo1 + ebx + 00], edx
    mov     [GPFifo1 + ebx + 04], edx
	add		ebx, 8
pBFR_LessThan2:
	test	al, 1
	jz		pBFR_Done
    WaitFifoIfNeeded 1,ecx
    mov     [GPFifo1 + ebx + 00], edx
pBFR_Done:    
    pop     edx
ifdef SOME_DISCONNECTS
	mov		fifoSpace,ecx
    pop     ecx
endif
pBFR_SecondPassDone:
    add     edi, 8                  ; Next line of data
    add     esi, 010000h            ; Next Scanline
    dec     dl                      ; decrease surplus line count
    jne     @f

	push	ecx
	mov		ebx, holdTag
	mov		ecx, wLeft16
	mov		eax, DestHeight
	sub		ebx, ecx
	mov		ecx, dwSrcWords
	mov		holdTag, ebx
	mov		ebx, nDwords
	dec		eax
	sub		ebx,ecx
	pop		ecx
	mov		nDwords, ebx

    WaitFifoIfNeededVar 1,fifoSpace
    mov     Count, eax
@@:
    inc     dh
    cmp     dh, 8
    jne     @f
    sub     edi, 64                  ; put di back to start of brush.
@@:     
    dec     cl
    jne     pBFR_NextScanline

	mov		eax,fifoSpace
    WaitFifoIfNeeded 2,eax

    mov     ScissorMode, 2
    mov     dY, 10000h
	jmp	BSXUnexcludeCursor			;U

if 0
;--------------------------------------------------------------------------
; P2BrushFillRect8
;
; Code to blt brushes onto the screen for Permedia at 8bpp.
; The fastest way to get a pattern replicated all over the screen is to use
; Block Fills. Set up dy to 8 - thus drawing a scanline at a time. It will take
; 8 passes to draw all the scanlines.
; The 64 bit block colour can contain an entire brush scanline
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  P2BrushFillRect8
    ; eax contains replicated colour
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds

    ; Check to see if color changed or context changed.
    mov     DD_CurrentContext, DD_SOLIDBLT_CTXT

    mov     eax, [ebp].lpDestDev

	GetFifoCount esi

    cmp     eax, GLInfo.dwCurrentOffscreen
    je      P2BFR_SameOffscreen
	xor		ebx,ebx
    lfs     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, fs:[ebx].OSB_PixelOffset
	
	WaitFifoIfNeeded 1, esi
    
	mov     FBWindowBase, edx
P2BFR_SameOffscreen:

	xor		edi,edi
    les     di, [ebp].lpPBrush
	mov		al, es:[edi].dp8BrushBpp
	cmp		al,8
	jnz		Jmp_DIB_BitBlt
    add     edi, dp8BrushBits           ;es:edi points to mono data

; es:edi = pointer to data

    mov     dY, 80000h
    mov     ScissorMode, 3
    mov     ScissorMaxXY, 07ff07ffh

	;get 16 bits the fast way
    mov     eax, dword ptr [ebp].DestxOrg
    mov		edx,dword ptr [ebp].yExt
	and		eax,0ffffh
	and		edx,0ffffh
    mov     ScissorMinXY, eax
    mov     esi, edx                    ;ax = count
    mov     ebx, eax
    and     edx, 7                      ;dl = surplus lines. 
    add     ebx, DWORD PTR [ebp].xExt   ; Add extent. Ignore top bits for now.
    add     esi, 7
    and     eax, NOT 7                  ; Align start to screen
    shr     esi, 3                      ;ax = count to use for dl surplus lines.
    shl     eax, 16
    mov     DestHeight, esi
    shl     ebx, 16
    mov     Count, esi
    mov     StartXDom, eax
    mov     StartXSub, ebx

	;16 bit load in disguise
    mov     ebx, dword ptr [ebp].DestyOrg   ;Get destination Y
    mov     ecx, 8
    mov     esi, ebx
    and     ebx, 7                      ;get rotational offset
    shl     esi, 16                     ;Y will be loaded every scan
	lea		edi, [edi + 8 * ebx]        ;edi is now at the correct offset to start rendering.
    mov     dh, bl                      ;dh is current offset

    jmp     P2BFR_NextScanline

	BEST_ALIGN

P2BFR_NextScanline:
    mov     eax, es:[edi]           ;Get first color
    mov     ebx, es:[edi+4]         ;Get second color
    mov     FBBlockColorUpper,eax       
    mov     FBBlockColorLower,ebx
           
    mov     eax, P2BrushFillRectCmd ;has fast fill etc
    mov     StartY, esi
    mov     Render,eax              ; do first pass

    add     edi, 8                  ; Next line of data
    add     esi, 010000h            ; Next Scanline
    dec     dl                      ; decrease surplus line count
    jne     @f

	mov		eax, DestHeight
	dec		eax
    mov     Count, eax              ; Reduce the count for the rest of the brush
@@:
    inc     dh
    cmp     dh, 8
    jne     @f
    sub     edi, 64                  ; put di back to start of brush.
@@:     
    dec     cl
    jne     P2BFR_NextScanline

	mov		eax,fifoSpace
    WaitFifoIfNeeded 2,eax

    mov     ScissorMode, 2
    mov     dY, 10000h
	jmp	BSXUnexcludeCursor			; U
endif

;--------------------------------------------------------------------------
; P2BrushFillRect8
;
; Code to blt brushes onto the screen for P2 at 8bpp.
; The fastest way to get a pattern replicated all over the screen is to use
; Block Fills. We can use the texture LUT to contain our brush data, we
; just need to load the LUT and hit go!
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  P2BrushFillRect8
    ; eax contains replicated colour
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    EXCLUDESWCURSOR
    DISPLAY_CONTEXT

    ; Check to see if color changed or context changed.
    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

    mov     eax, [ebp].lpDestDev

    cmp     eax, GLInfo.dwCurrentOffscreen
    je      P2BFR8_SameOffscreen
	xor		ebx,ebx
    lfs     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, fs:[ebx].OSB_PixelOffset
	mov     FBWindowBase, edx
P2BFR8_SameOffscreen:

	xor		esi,esi
    les     si, [ebp].lpPBrush
	mov		al, es:[esi].dp16BrushBpp
	cmp		al,8
	jnz		Jmp_DIB_BitBlt
    add     esi, dp8BrushBits               ;es:esi points to brush data

    ;First load brush into TexelLUT. Our brush is already screen aligned, and this is
    ;exactly what we need.

    mov     ecx, 4
    lea     edi, GPFifo4
    push    ebp
    mov     GPFifo1, 098h                   ;TexelLUTIndex                
    mov     GPFifo2, 0h                     ;TexelLUTIndex Value                
    mov     GPFifo3, 0f0099h                ;TexelLUTData*32 - 2 DWORDS * 8 = 16 words of data               
@@:
    mov     eax, es:[esi +  0]              ;Read 2 scanlines of the brush
    mov     ebx, es:[esi +  4]
    mov     edx, es:[esi +  8]
    mov     ebp, es:[esi + 12]
    mov     [edi +  0], eax                 ;Burst write 2 scanlines to GPFIFO
    mov     [edi +  4], ebx
    mov     [edi +  8], edx
    mov     [edi + 12], ebp
    add     esi, 16
    add     edi, 16
    dec     ecx                             ;decrease scanline count
    jne     @b
    pop     ebp                             ;Restore base pointer

    xor     eax, eax
    mov     TextureAddressMode, eax
    mov     TextureColorMode, 07h           ;COPY_TEXTURE + ENABLE                
    mov     TextureLUTMode, 0803h           ;4 PIXELS/ENTRY + DIRECT + ENABLE      
    mov     PTextureReadMode, 01000660ah    ;8x8 REPEAT + PACKED

    mov     ScissorMode, 3
	; Really these are two 16 bit loads
    mov     eax,dword ptr [ebp].DestyOrg    ;X << 16 | Y coords
    mov     ebx,dword ptr [ebp].yExt		;width << 16 | height
    ror     eax, 16                         ;P2 needs Y << 16 | X 
    mov     ecx, P2BrushFillRectCmd         ;has fast fill + direction for P2
    ror     ebx, 16                         ;P2 needs height << 16 | width
    mov     edx, eax
    mov     ScissorMinXY, eax               ;We need left clip, but we have top too.
    add     edx, ebx                        ;work out right and bottom clip
    and     eax, 0fffffff8h                 ;render from brush alignment - 8 pixels
    add     ebx, 7                          ;Possibly extra 7 pixels width
    mov     ScissorMaxXY, edx 
    mov     RectangleOrigin,eax			
    mov     RectangleSize,ebx			
    mov     Render,ecx                      ; do it

    xor     eax, eax
    mov     ScissorMode, 2
    mov     dY, 10000h

    mov     TextureColorMode, eax                
    mov     TextureLUTMode, eax                 
    mov     TextureAddressMode, DEFAULT_P2TEXTUREADDRESSMODE
    mov     PTextureReadMode, DEFAULT_P2TEXTUREREADMODE   

	jmp	BSXUnexcludeCursor

;--------------------------------------------------------------------------
; P2BrushFillRect16
;
; Code to blt brushes onto the screen for P2 at 16bpp.
; The fastest way to get a pattern replicated all over the screen is to use
; Block Fills. We can use the texture LUT to contain our brush data, we
; just need to load the LUT and hit go!
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  P2BrushFillRect16
    ; eax contains replicated colour
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    EXCLUDESWCURSOR
    DISPLAY_CONTEXT

    ; Check to see if color changed or context changed.
    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

    mov     eax, [ebp].lpDestDev

    cmp     eax, GLInfo.dwCurrentOffscreen
    je      P2BFR16_SameOffscreen
	xor		ebx,ebx
    lfs     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, fs:[ebx].OSB_PixelOffset
	mov     FBWindowBase, edx
P2BFR16_SameOffscreen:

	xor		esi,esi
    les     si, [ebp].lpPBrush
	mov		al, es:[esi].dp16BrushBpp
	cmp		al,16
	jnz		Jmp_DIB_BitBlt
    add     esi, dp8BrushBits               ;es:esi points to brush data

    ;First load brush into TexelLUT. Our brush is already screen aligned, and this is
    ;exactly what we need.

    mov     ecx, 8
    lea     edi, GPFifo4
    push    ebp
    mov     GPFifo1, 098h                   ;TexelLUTIndex                
    mov     GPFifo2, 0h                     ;TexelLUTIndex Value                
    mov     GPFifo3, 1f0099h                ;TexelLUTData*32 - 4 DWORDS * 8 = 32 words of data               
@@:
    mov     eax, es:[esi +  0]              ;Read a scanline of the brush
    mov     ebx, es:[esi +  4]
    mov     edx, es:[esi +  8]
    mov     ebp, es:[esi + 12]
    mov     [edi +  0], eax                 ;Burst write a scanline to GPFIFO
    mov     [edi +  4], ebx
    mov     [edi +  8], edx
    mov     [edi + 12], ebp
    add     esi, 16
    add     edi, 16
    dec     ecx                             ;decrease scanline count
    jne     @b
    pop     ebp                             ;Restore base pointer

    xor     eax, eax
    mov     TextureAddressMode, eax
    mov     TextureColorMode, 07h           ;COPY_TEXTURE + ENABLE                
    mov     TextureLUTMode, 0403h           ;2 PIXELS/ENTRY + DIRECT + ENABLE      
    mov     PTextureReadMode, 01000660ah    ;8x8 REPEAT + PACKED

    mov     ScissorMode, 3
    mov     BitMaskPattern, 0f0f0f0f0h      ;Draw 4 pixels, miss 4

	; Really these are two 16 bit loads
    mov     eax,dword ptr [ebp].DestyOrg    ;X << 16 | Y coords
    mov     ebx,dword ptr [ebp].yExt		;width << 16 | height
    ror     eax, 16                         ;P2 needs Y << 16 | X 
    mov     ecx, P2BrushFillRectCmd         ;has fast fill + direction for P2
    ror     ebx, 16                         ;P2 needs height << 16 | width
    mov     edx, eax
    mov     ScissorMinXY, eax               ;We need left clip, but we have top too.
    add     edx, ebx                        ;work out right and bottom clip
    and     eax, 0fffffff8h                 ;render from brush alignment - 8 pixels
    add     ebx, 7                          ;Possibly extra 7 pixels width
    mov     ScissorMaxXY, edx 
    mov     RectangleOrigin,eax			
    mov     RectangleSize,ebx			
    mov     Render,ecx                      ; do it
    add     eax, 4                          ; repeat render 4 pixels to right
    mov     TextureLUTMode, 040bh           ;2 PIXELS/ENTRY + DIRECT + ENABLE + Offset 2    
    mov     RectangleOrigin, eax
    mov     Render,ecx                      ; do it

    xor     eax, eax
    mov     ScissorMode, 2
    mov     dY, 10000h

    mov     TextureColorMode, eax                
    mov     TextureLUTMode, eax                 
    mov     TextureAddressMode, DEFAULT_P2TEXTUREADDRESSMODE
    mov     PTextureReadMode, DEFAULT_P2TEXTUREREADMODE   

	jmp	BSXUnexcludeCursor

;--------------------------------------------------------------------------
; P2BrushFillRect32
;
; Code to blt brushes onto the screen for P2 at 16bpp.
; The fastest way to get a pattern replicated all over the screen is to use
; Block Fills. We can use the texture LUT to contain our brush data, we
; just need to load the LUT and hit go!
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  P2BrushFillRect32
    ; eax contains replicated colour
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds

    ; Check to see if color changed or context changed.
    mov     DD_CurrentContext, DD_UNDEFINED_CTXT

    mov     eax, [ebp].lpDestDev

    cmp     eax, GLInfo.dwCurrentOffscreen
    je      P2BFR32_SameOffscreen
	xor		ebx,ebx
    lfs     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, fs:[ebx].OSB_PixelOffset
	mov     FBWindowBase, edx
P2BFR32_SameOffscreen:

	xor		esi,esi
    les     si, [ebp].lpPBrush
	mov		al, es:[esi].dp16BrushBpp
	cmp		al,32
	jnz		Jmp_DIB_BitBlt
    add     esi, dp8BrushBits               ;es:esi points to brush data

    ;First load brush into TexelLUT. Our brush is already screen aligned, and this is
    ;exactly what we need.

    mov     ecx, 16
    lea     edi, GPFifo4
    push    ebp
    mov     GPFifo1, 098h                   ;TexelLUTIndex                
    mov     GPFifo2, 0h                     ;TexelLUTIndex Value                
    mov     GPFifo3, 3f0099h                ;TexelLUTData*32 - 8 DWORDS * 8 = 64 words of data               
@@:
    mov     eax, es:[esi +  0]              ;Read a scanline of the brush
    mov     ebx, es:[esi +  4]
    mov     edx, es:[esi +  8]
    mov     ebp, es:[esi + 12]
    mov     [edi +  0], eax                 ;Burst write a scanline to GPFIFO
    mov     [edi +  4], ebx
    mov     [edi +  8], edx
    mov     [edi + 12], ebp
    add     esi, 16
    add     edi, 16
    dec     ecx                             ;decrease scanline count
    jne     @b
    pop     ebp                             ;Restore base pointer

    xor     eax, eax
    mov     TextureColorMode, 07h           ;COPY_TEXTURE + ENABLE                
    mov     PTextureReadMode, 01000660ah    ;8x8 REPEAT + PACKED
    mov     TextureAddressMode, eax

    mov     ScissorMode, 3
    mov     BitMaskPattern, 0c0c0c0c0h      ;Draw 2 pixels, miss 6

	; Really these are two 16 bit loads
    mov     eax,dword ptr [ebp].DestyOrg    ;X << 16 | Y coords
    mov     ebx,dword ptr [ebp].yExt		;width << 16 | height
    ror     eax, 16                         ;P2 needs Y << 16 | X 
    mov     ecx, P2BrushFillRectCmd         ;has fast fill + direction for P2
    ror     ebx, 16                         ;P2 needs height << 16 | width
    mov     edx, eax
    mov     ScissorMinXY, eax               ;We need left clip, but we have top too.
    add     edx, ebx                        ;work out right and bottom clip
    and     eax, 0fffffff8h                 ;render from brush alignment - 8 pixels
    add     ebx, 7                          ;Possibly extra 7 pixels width
    mov     ScissorMaxXY, edx 
    mov     RectangleSize,ebx			
    mov     TextureLUTMode, 0003h           ; 1 PIXEL/ENTRY + DIRECT + ENABLE      
    mov     RectangleOrigin,eax			
    mov     Render,ecx                      ; do it
    add     eax, 2                          ; repeat render 4 pixels to right
    mov     TextureLUTMode, 000bh           ; 2 PIXELS/ENTRY + DIRECT + ENABLE + Offset 2    
    mov     RectangleOrigin, eax
    mov     Render,ecx                      ; do it
    add     eax, 2                          ; repeat render 4 pixels to right
    mov     TextureLUTMode, 0013h           ; 2 PIXELS/ENTRY + DIRECT + ENABLE + Offset 4    
    mov     RectangleOrigin, eax
    mov     Render,ecx                      ; do it
    add     eax, 2                          ; repeat render 4 pixels to right
    mov     TextureLUTMode, 001bh           ; 2 PIXELS/ENTRY + DIRECT + ENABLE + Offset 6    
    mov     RectangleOrigin, eax
    mov     Render,ecx                      ; do it

    xor     eax, eax
    mov     ScissorMode, 2
    mov     dY, 10000h

    mov     TextureColorMode, eax                
    mov     TextureLUTMode, eax                 
    mov     TextureAddressMode, DEFAULT_P2TEXTUREADDRESSMODE
    mov     PTextureReadMode, DEFAULT_P2TEXTUREREADMODE   

	jmp	BSXUnexcludeCursor

;--------------------------------------------------------------------------
; PermediaROPFillRect
;
; ROP screen rectangle fill - for Permedia chips only
;
;--------------------------------------------------------------------------
	BEST_ALIGN

PLABEL  permediaROPFillRect
    ; eax contains replicated colour
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    push    eax
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds
    pop     eax

;   eax = colour (replicated)

    ; Check to see if color changed or context changed.
    mov     DD_CurrentContext, DD_ROPFILLRECT_CTXT
    mov     edx, [ebp].lpDestDev

	GetFifoCount ecx

    cmp     edx, GLInfo.dwCurrentOffscreen
    je      pRFR_SameOffscreen
    lfs     bx, [ebp].lpDestDev
    mov     GLInfo.dwCurrentOffscreen, edx
    mov     edx, fs:[bx].OSB_PixelOffset

	WaitFifoIfNeeded 1,ecx
    
	mov     FBWindowBase, edx
pRFR_SameOffscreen:

	WaitFifoIfNeeded 11,ecx
	mov		fifoSpace,ecx

    mov     FBSourceData, eax       ;Save the color in the source register
    mov     ecx,RopFunction          ;ax contains ROP fn and Read bits
    xor     ebx, ebx
    mov     bl, cl
    and     ecx, ROP_REQ_DST         ;We just want the Destination read flag
    mov     edx,dwFBReadMode        ;get read mode
	mov     LogicalOpMode,ebx		;set logical op mode
    or      edx,ecx                 ;or it into the read mode
    or      edx, 80000h             ;set packed bit on Permedia
    mov     FBReadMode,edx          ;set read mode as required
    mov     cl, GLInfo.bPixelToWordShift   ;Shift factor (2 for 8bpp, 0 for 32bpp)

    mov     esi,dword ptr [ebp].DestyOrg       ;Y coords
    mov     eax,dword ptr [ebp].DestxOrg       ;X coords
    mov     ebx,dword ptr [ebp].xExt			;width
    shl     esi,16					;Y -> 16.16 format
    mov		edi,dword ptr [ebp].yExt					;edi -> height
    mov     StartY,esi				;
    add     ebx, eax                ;bx is right edge
	and		edi,0ffffh
    shl     eax,16					;X -> 16.16 format
    mov     edx, eax
    shr     eax, cl                 ;reduce StartXDom for packing 
    mov     StartXDom,eax			;
	mov		dx,bx
    shl     ebx,16					;Width -> 16.16 format
    shr     ebx,cl                  ;reduce StartXSub for packing
    mov     Count,edi				;
    mov     ecx, ROPFillRectCmd     ;has fast fill etc
    add     ebx, 010000h            ;add one to get limits correct.
    mov     StartXSub,ebx			;
    mov     PackedDataLimits,edx
    mov     Render,ecx              ; do it

    mov     edx,dwFBReadMode        ;get read mode
	mov     LogicalOpMode,20h		;set logical op mode
    mov     FBReadMode,edx          ;set read mode as required
	jmp	BSXUnexcludeCursor			;


;--------------------------------------------------------------------------
; P2FillRect
;
; P2 Screen fill - uses condensed setup.
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  P2FillRect
    ; eax contains replicated colour
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    push    eax
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds
    pop     eax


;   eax = colour (replicated)

	GetFifoCount esi

    ; Check to see if color changed or context changed.
    cmp     DWORD PTR DD_CurrentContext, DD_SOLIDBLT_CTXT
	jne		P2FR_ChangeColour

    cmp     DD_CurrentColour, eax
    je      P2FR_DontChangeColour
P2FR_ChangeColour:
    mov     DD_CurrentContext, DD_SOLIDBLT_CTXT
	mov     DD_CurrentColour, eax

	WaitFifoIfNeeded 1,esi
    
	mov     FBBlockColor,eax        ;Passed in eax
P2FR_DontChangeColour:

    mov     eax, [ebp].lpDestDev
    cmp     eax, GLInfo.dwCurrentOffscreen
    je      P2FR_SameOffscreen
	xor		ebx,ebx
    mov     GLInfo.dwCurrentOffscreen, eax
    lfs     bx, [ebp].lpDestDev
    mov     edx, fs:[ebx].OSB_PixelOffset

	WaitFifoIfNeeded 1,esi

    mov     FBWindowBase, edx

P2FR_SameOffscreen:
	;Really these are two 16 bit loads
    mov     eax,dword ptr [ebp].DestyOrg    ;X << 16 | Y coords
    mov     ebx,dword ptr [ebp].yExt		;width << 16 | height
    mov     ecx, P2FillRectCmd              ;has fast fill + direction for P2
    ror     eax, 16                         ;P2 needs Y << 16 | X 
    ror     ebx, 16                         ;P2 needs height << 16 | width 

    WaitFifoIfNeeded 3,esi

    mov     RectangleOrigin,eax			
    mov     RectangleSize,ebx			
    mov     Render,ecx                      ; do it
;	Don't Fall through to BSXUnexcludeCursor, jump the nops
	jmp		BSXUnexcludeCursor			

;--------------------------------------------------------------------------
; FillRect24bpp
;
; Basic screen fill at 24bpp where we cannot use block fills unless the
; colour is grey. Decide which routine to call.
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  FillRect24bpp
    ; eax contains replicated colour
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    ; Check for a shade of grey
    cmp     al, ah                          ; compare lower and middle bytes of colour
    jne     FillRectNoBlockFill             ; Jump to non block fill function if not equal
    mov     ebx, eax
    shr     ebx, 8
    cmp     bl, bh                          ; compare upper and middle bytes of colour
    jne     FillRectNoBlockFill             ; Jump to non block fill function if not equal

    shl     eax, 8                          ; Replicate colour into top byte (all 4 bytes
    mov     al, ah                          ; need to be the same)
    jmp     [GreyFillRectFn]                ; All bytes equal. Go to block fill function

;--------------------------------------------------------------------------
; FillRectNoBlockFill
;
; Basic screen fill without using block fill.
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  FillRectNoBlockFill
    push    eax
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds
    pop     eax

;   eax = colour (replicated)

    mov     ecx, [ebp].lpDestDev
    cmp     ecx, GLInfo.dwCurrentOffscreen
    je      FRNBF_SameOffscreen
	xor		ebx,ebx
    mov     GLInfo.dwCurrentOffscreen, ecx
    lfs     bx, [ebp].lpDestDev
    mov     edx, fs:[ebx].OSB_PixelOffset

    mov     FBWindowBase, edx
FRNBF_SameOffscreen:

	mov     FBWriteData,eax                 ;Colour passed in eax

    mov     ebx,dword ptr [ebp].yExt		;ebx = width << 16 | height
    mov     eax,dword ptr [ebp].DestyOrg    ;eax = X << 16 | Y coords
    mov     edi, ebx                        ;edi = width << 16 | height
    mov     edx, eax                        ;edx = X << 16 | Y coords
    add     ebx, eax                        ;X+width << 16 | Y+height
    and     edi, 0ffffh                     ;height
    shl     edx, 16                         ;Y << 16
    mov     ecx, 40h                        ;Trapezoid. No block fill

    mov     StartXDom,eax			        ;Ignore fraction bits
    mov     StartXSub,ebx                   ;Ignore fraction bits
    mov     StartY,edx
    mov     Count,edi				
    mov     Render,ecx                      ; do it
	jmp		BSXUnexcludeCursor			

;--------------------------------------------------------------------------
; FillRect
;
; Basic screen fill - can be used for all chips.
;
;--------------------------------------------------------------------------
	BEST_ALIGN
PLABEL  FillRect
    ; eax contains replicated colour
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    push    eax
    EXCLUDESWCURSOR
    DISPLAY_CONTEXT
;    DISABLE_DISCONNECT ds
    pop     eax

;   eax = colour (replicated)

	GetFifoCount esi

    ; Check to see if color changed or context changed.
    cmp     DWORD PTR DD_CurrentContext, DD_SOLIDBLT_CTXT
	jne		gSB_ChangeColour

    cmp     DD_CurrentColour, eax
    je      gSB_DontChangeColour
gSB_ChangeColour:
    mov     DD_CurrentContext, DD_SOLIDBLT_CTXT
	mov     DD_CurrentColour, eax

	WaitFifoIfNeeded 1,esi
    
	mov     FBBlockColor,eax        ;Passed in eax

gSB_DontChangeColour:

    mov     eax, [ebp].lpDestDev
    cmp     eax, GLInfo.dwCurrentOffscreen
    je      gSB_SameOffscreen
	xor		ebx,ebx
    mov     GLInfo.dwCurrentOffscreen, eax
    lfs     bx, [ebp].lpDestDev
    mov     edx, fs:[ebx].OSB_PixelOffset

	WaitFifoIfNeeded 1,esi

    mov     FBWindowBase, edx
gSB_SameOffscreen:

    mov     ebx,dword ptr [ebp].yExt		;ebx = width << 16 | height
    mov     eax,dword ptr [ebp].DestyOrg    ;eax = X << 16 | Y coords
    mov     edi, ebx                        ;edi = width << 16 | height
    mov     edx, eax                        ;edx = X << 16 | Y coords
    add     ebx, eax                        ;X+width << 16 | Y+height
    and     edi, 0ffffh                     ;height
    shl     edx, 16                         ;Y << 16
    mov     ecx, FillRectCmd                ;has fast fill etc

    WaitFifoIfNeeded 5,esi

    mov     StartXDom,eax			        ;Ignore fraction bits
    mov     StartXSub,ebx                   ;Ignore fraction bits
    mov     StartY,edx
    mov     Count,edi				
    mov     Render,ecx                      ; do it
	jmp		BSXUnexcludeCursor			

	BEST_ALIGN
PLABEL BS_Done
	pop		edi					
	pop		esi					
	pop		ds
	mov		esp, ebp					; Then do the "leave" by hand.
	mov		eax, 1
	pop		ebp
	retf	sizeof BitBltParms - 12		; not the 8 bytes of return addr and 4 of ebp

	BEST_ALIGN
PLABEL BSXUnexcludeCursor
	mov		ebx, GLInfo.dwCursorType            ; running with a software cursor?
	mov		eax, 1
	test	ebx,ebx
	je		BS_not_Done		            ; nope, no need to worry about hw cursor
	pop		edi					
	pop		esi					
	pop		ds
	mov		esp, ebp					; Then do the "leave" by hand.
	pop		ebp
	retf	sizeof BitBltParms - 12		; not the 8 bytes of return addr and 4 of ebp

BS_not_Done:
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
	test	al,al						;did we call DIB eng to exclude cursor?
	je		BS_Done 					;nope, skip the following!
	push	GLInfo.lpDriverPDevice				;
	push	word ptr CURSOREXCLUDE		;
	call	DIB_EndAccess				;Let DIB Engine unexclude cursor.
	mov		DIBAccessCallFlag,0			;clear the flag
	jmp		BS_Done


;----------------------------------------------------------------------------
; ClipSrcToDest
;----------------------------------------------------------------------------
	BEST_ALIGN
PLABEL	ClipSrcToDest	near
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    push    ds
    lds     bx,[ebp].lpDestDev
	xor		eax,eax
    mov     ax,[bx].deHeight
    mov     DestHeight,eax
	; The next two are really only loading 16 bits each
    mov     esi,dword ptr [ebp].xExt                 ;X extent will be used a lot
    mov     edi,dword ptr [ebp].yExt                 ;Y extent will be used a lot
    test    RopFunction,ROP_REQ_SRC
    je      PFB_ChkNullBlt

    cmp     [ebp].lpSrcDev,0            ;Is there a src?
    je      PFB_ChkNullBlt				;no.
    lds     bx,[ebp].lpSrcDev           ;yes.
;----------------------------------------------------------------------------
; Input clipping.  The source device must be clipped to the device
; limits.  The destination X and Y, and the extents have been clipped
; by GDI and are positive numbers (0-7FFFh).  The source X and Y could
; be negative.  The clipping code will have to check constantly for
; negative values.
;----------------------------------------------------------------------------
PLABEL  PFB_InputClipX
    mov     ax,[ebp].SrcxOrg              ;Will need source X org
    mov     bx,[bx].deWidth         ;Maximum allowable is width_bits-1
	;Then check the top bit of ax without specifying a 16 bit register
    test    ah,ah                   ;Any left edge overhang?
    jns     PFB_InputClipRightEdge  ;  No, left edge is on the surface

;----------------------------------------------------------------------------
; The source origin is off the left hand edge of the device surface.
; Move both the source and destination origins right by the amount of
; the overhang and also remove the overhang from the extent.
;
; There is no need to check for the destination being moved off the
; right hand edge of the device's surface since the extent would go
; zero or negative were that to happen.
;----------------------------------------------------------------------------
    add     si,ax                 ;Subtract overhang from X extent
    js      PFB_NullBlt             ;Wasn't enough, nothing to BLT
    sub     [ebp].DestxOrg,ax       ;Move destination left
    xor     eax,eax                 ;Set new source X origin
    mov     [ebp].SrcxOrg,ax

;----------------------------------------------------------------------------
; The left hand edge has been clipped.  Now clip the right hand edge.
; Since both the extent and the source origin must be positive numbers
; now, any sign change from adding them together can be ignored if the
; comparison to bmWidth is made as an unsigned compare (maximum result
; of the add would be 7FFFh+7FFFh, which doesn't wrap past zero).
;----------------------------------------------------------------------------
PLABEL  PFB_InputClipRightEdge
    add     eax,esi                 ;Compute right edge + 1
    sub     ax,bx                   ;Compute right edge overhang
    jbe     PFB_InputClipSaveXExt   ;No overhang
    sub     si,ax                   ;Subtract overhang from X extent
    js      PFB_NullBlt             ;Wasn't enough, nothing to BLT

PLABEL  PFB_InputClipSaveXExt
    mov     [ebp].xExt,si                 ;Save new X extent

;----------------------------------------------------------------------------
; Now clip the Y coordinates.  The procedure is the same and all the
; above about positive and negative numbers still holds true.
;----------------------------------------------------------------------------
PLABEL  PFB_InputClipY
    mov     eax,dword ptr [ebp].SrcyOrg              ;Will need source Y org
    mov     ebx,[ebp].lpSrcDev
	and		eax,0ffffh
	and		ebx,0ffffh
    mov     bx,[ebx].deHeight        ;Maximum allowable is height-1
	;Test the high bit of ax without specifying a 16 bit register...
    test    ah,ah                   ;Any top edge overhang?
    jns     PFB_InputClipBtmEdge    ;  No, top is on the surface

;----------------------------------------------------------------------------
; The source origin is off the top edge of the device surface.  Move
; both the source and destination origins down by the amount of the
; overhang, and also remove the overhang from the extent.
;
; There is no need to check for the destination being moved off the
; bottom of the device's surface since the extent would go zero or
; negative were that to happen.
;----------------------------------------------------------------------------
    add     di,ax                   ;Subtract overhang from Y extent
    js      PFB_NullBlt             ;Wasn't enough, nothing to BLT
    sub     [ebp].DestyOrg,ax             ;Move destination down
    xor     eax,eax                   ;Set new source Y origin
    mov     [ebp].SrcyOrg,ax

;----------------------------------------------------------------------------
; The top edge has been clipped. Now clip the bottom edge. Since both
; the extent and the source origin must be positive numbers now, any
; sign change from adding them together can be ignored if the
; comparison to bmWidth is made as an unsigned compare (maximum result
; of the add would be 7FFFh+7FFFh, which doesn't wrap thru 0).
;----------------------------------------------------------------------------
PLABEL  PFB_InputClipBtmEdge
    add     eax,edi                   ;Compute bottom edge + 1
    sub     ax,bx                   ;Compute bottom edge overhang
    jbe     PFB_InputClipSaveYExt   ;No overhang
    sub     di,ax                   ;Subtract overhang from Y extent
    jns     PFB_InputClipSaveYExt

PLABEL  PFB_NullBlt
    pop     ds
    stc
    retn

	BEST_ALIGN
PLABEL  PFB_InputClipSaveYExt
    mov     [ebp].yExt,di                 ;Save new Y extent

PLABEL  PFB_ChkNullBlt
    test    esi,0ffffh
    jz      PFB_NullBlt             ;X extent is 0
    test    edi,0ffffh
    jz      PFB_NullBlt             ;Y extent is 0

PLABEL PFB_Done
    pop     ds
	clc	
	retn

;----------------------------------------------------------------------------
; Cursor Exclusion
; A union of both rectangles must be performed to determine the
; exclusion area.
; Currently:
;       SI = X extent
;       DI = Y extent
;----------------------------------------------------------------------------
	BEST_ALIGN
PLABEL	BltCursorExclude	near
    push    ds

	xor		ebx,ebx
	xor		eax,eax

	lds	    bx,GLInfo.lpDriverPDevice           ; Indicate hardware is busy so that SW cursor
	or	    ds:[ebx].deFlags,BUSY               ; doesnt get updated.

    lds     bx, [ebp].lpDestDev
    mov     eax, dword ptr ds:[ebx].deFlags
    test    eax, OFFSCREEN
    jne     BCS_DestOK
    test    eax, VRAM
    je      BCS_DestOK

; Dest is VRAM and not Offscreen -> must be the screen! Exclude the cursor
    mov     eax, dword ptr [ebp].DestxOrg
    mov     ebx, dword ptr [ebp].DestyOrg
    mov     ecx, dword ptr [ebp].xExt
    mov     edx, dword ptr [ebp].yExt
	and		eax,0ffffh
	and		ebx,0ffffh
	and		ecx,0ffffh
	and		edx,0ffffh
    invoke  CursorExclude, eax, ebx, ecx, edx

BCS_DestOK:
    mov     eax, DWORD PTR [ebp].lpSrcDev+2 ; Get segment
    test    eax, 0ffffh                  ; Is it zero?
    je      BCS_SrcOK               ; No source surface.
; Check the source surface now
    mov     ds, ax                  ; ds = segment
    mov     ebx, [ebp].lpSrcDev   ; bx = offset 
	and		ebx,0ffffh
    mov     ax, ds:[ebx].deFlags
    test    eax, OFFSCREEN
    jne     BCS_SrcOK
    test    eax, VRAM
    je      BCS_SrcOK

; Src is VRAM and not Offscreen -> must be the screen! Exclude the cursor
    mov     eax, dword ptr [ebp].SrcxOrg
    mov     ebx, dword ptr [ebp].SrcyOrg
    mov     ecx, dword ptr [ebp].xExt
    mov     edx, dword ptr [ebp].yExt
	and		eax,0ffffh
	and		ebx,0ffffh
	and		ecx,0ffffh
	and		edx,0ffffh
    invoke  CursorExclude, eax, ebx, ecx, edx

BCS_SrcOK:
    pop     ds
    retn


;----------------------------------------------------------------------------
; MemCopy
; A fast memory to memory copy routine. Copies ecx bytes from ds:esi to es:edi
;----------------------------------------------------------------------------

PLABEL	MemCopyInit	near
; On first call, decide which of the accelerated routines we should call, then fix
; up MemCopyFn appropriatly
    push    ecx                     ; Save count

; Check for CPUID instruction:
    pushfd                          ; save EFLAGS
    pop     eax                     ; get EFLAGS in eax
    mov     ebx, eax                ; copy to ebx
    xor     eax, 200000h            ; flip ID bit in EFLAGS
    push    eax                     ; save new EFLAGS value on stack
    popfd                           ; replace current EFLAGS value
    pushfd                          ; get new EFLAGS
    pop     eax                     ; store new EFLAGS in EAX
    xor     eax, ebx                ; can’t toggle ID bit,
    je      MemCopy_UseDst          ; No CPUID Instruction

    mov     eax, 0
    cpuid
    cmp     ebx, 0756e6547h         ; Check for GenuineIntel
    jne     MemCopy_UseDst
    cmp     ecx, 049656e69h
    jne     MemCopy_UseDst
    cmp     edx, 06c65746eh
    jne     MemCopy_UseDst

    mov     eax, 1
    cpuid
if 0
    test    edx, 00800000h          ; Check for MMX
    jne     MemCopy_UseMMX
endif
    shr     eax, 16                 ; Move processor family into al
    cmp     al, 6                   ; Is it a P6?
    je      MemCopy_UseSrc          ; Aligned Source reads seem faster on a P6
    jmp     MemCopy_UseDst          ; Aligned Dest writes are faster on a P5
    
MemCopy_UseMMX:                     ; No MMX support yet

MemCopy_UseSrc:
    pop     ecx
    mov     MemCopyFn, OFFSET MemCopySrc
    jmp     MemCopySrc
MemCopy_UseDst:
    pop     ecx
    mov     MemCopyFn, OFFSET MemCopyDst
    jmp     MemCopyDst


    BEST_ALIGN
PLABEL	MemCopy
    jmp     [MemCopyFn]

PLABEL	MemCopyDst
    mov     edx, edi                ; Destination aligned
    jmp MemCopyGo

    BEST_ALIGN
PLABEL	MemCopySrc                  ; Source Aligned
    mov     edx, esi
MemCopyGo:
    push    esi
    push    edi
    push    ebp
    mov     al, es:[edi]
    test    edx, 7
    je      MemCopy_Aligned8Bytes
    test    edx, 3
    je      MemCopy_AlignedDWORD
    test    edx, 1
    je      MemCopy_AlignedWORD
    mov     al, [esi+0]
    mov     es:[edi+0], al
    inc     esi
    inc     edi
    inc     edx
    dec     ecx
MemCopy_AlignedWORD:
    test    edx, 3
    je      MemCopy_AlignedDWORD
    cmp     ecx, 2
    jb      MemCopy_Last1
    mov     ax, [esi+0]
    mov     es:[edi+0], ax
    add     esi, 2
    add     edi, 2
    add     edx, 2
    sub     ecx, 2 
MemCopy_AlignedDWORD:
    test    edx, 7
    je      MemCopy_Aligned8Bytes
    cmp     ecx, 4
    jb      MemCopy_Last3
    mov     eax, [esi+0]
    mov     es:[edi+0], eax
    add     esi, 4
    add     edi, 4
    add     edx, 4
    sub     ecx, 4
MemCopy_Aligned8Bytes:
    test    edx, 15
    je      MemCopy_Aligned16Bytes
    cmp     ecx, 8
    jb      MemCopy_Last7
    mov     eax, [esi+0]
    mov     ebx, [esi+4]
    mov     es:[edi+0], eax
    mov     es:[edi+4], ebx
    add     esi, 8
    add     edi, 8
    add     edx, 8
    sub     ecx, 8
MemCopy_Aligned16Bytes:
    test    edx, 31
    je      MemCopy_CacheAligned
    cmp     ecx, 16
    jb      MemCopy_Last15
    mov     eax, [esi+ 0]
    mov     ebx, [esi+ 4]
    mov     edx, [esi+ 8]
    mov     ebp, [esi+12]
    mov     es:[edi+ 0], eax
    mov     es:[edi+ 4], ebx
    mov     es:[edi+ 8], edx
    mov     es:[edi+12], ebp
    add     esi, 16
    add     edi, 16
    sub     ecx, 16
    jmp     MemCopy_CacheAligned

    BEST_ALIGN
MemCopy_Next32:
    mov     ebp, es:[edi+ 0]
    mov     eax, [esi+ 0]
    mov     ebx, [esi+ 4]
    mov     edx, [esi+ 8]
    mov     ebp, [esi+12]
    mov     es:[edi+ 0], eax
    mov     es:[edi+ 4], ebx
    mov     es:[edi+ 8], edx
    mov     es:[edi+12], ebp
    mov     eax, [esi+16]
    mov     ebx, [esi+20]
    mov     edx, [esi+24]
    mov     ebp, [esi+28]
    mov     es:[edi+16], eax
    mov     es:[edi+20], ebx
    mov     es:[edi+24], edx
    mov     es:[edi+28], ebp
    add     esi, 32
    add     edi, 32
MemCopy_CacheAligned:
    sub     ecx, 32
    jge     MemCopy_Next32
    add     ecx, 32
    jz      MemCopy_Finished     
MemCopy_Last31:
    mov     ebp, es:[edi+ 0]
    cmp     ecx, 16
    jb      MemCopy_Last15
    mov     eax, [esi+ 0]
    mov     ebx, [esi+ 4]
    mov     edx, [esi+ 8]
    mov     ebp, [esi+12]
    mov     es:[edi+ 0], eax
    mov     es:[edi+ 4], ebx
    mov     es:[edi+ 8], edx
    mov     es:[edi+12], ebp
    add     esi, 16
    add     edi, 16
    sub     ecx, 16
    jz      MemCopy_Finished
MemCopy_Last15:
    cmp     ecx, 8
    jb      MemCopy_Last7
    mov     eax, [esi+ 0]
    mov     ebx, [esi+ 4]
    mov     es:[edi+ 0], eax
    mov     es:[edi+ 4], ebx
    add     esi, 8
    add     edi, 8
    sub     ecx, 8
    jz      MemCopy_Finished     
MemCopy_Last7:
    cmp     ecx, 4
    jb      MemCopy_Last3
    mov     eax, [esi+ 0]
    mov     es:[edi+ 0], eax
    add     esi, 4
    add     edi, 4
    sub     ecx, 4
    jz      MemCopy_Finished
MemCopy_Last3:
    cmp     ecx, 2
    jb      MemCopy_Last1
    mov     ax, [esi+ 0]
    mov     es:[edi+ 0], ax
    add     esi, 2
    add     edi, 2
    sub     ecx, 2
    jz      MemCopy_Finished
MemCopy_Last1:
    cmp     ecx, 1
    jb      MemCopy_Finished
    mov     al, [esi+ 0]
    mov     es:[edi+ 0], al
MemCopy_Finished:
    pop     ebp
    pop     edi
    pop     esi
    retn


	BEST_ALIGN
BitBlt32	ENDP


_BLIT32   ends
end
