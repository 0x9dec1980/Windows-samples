;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: glint.inc
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/


; This is a little nasty. For performance reasons we need to declare the
; data area differently if we are in 16 bit segment or a 32 bit segment.
; This means we have to separate our code files for 32 bit and 16 bit code.
; If you dont do this, then access to the data segment will either be with
; word address offsets or with dword offsets for both 32 bit and 16 bit segments.
; Thus in one of the 2 segment types you will always need prefixes to access the data.

; Should be defined in newer minivdd.inc
DEVICE_IS_NOT_VGA   equ 800h

.586
ifdef CODE32BPP
_INIT segment page use32 'DATA'
_INIT ends
else

_INIT segment page use16 'DATA'
_INIT ends
endif

	include cmacros.inc
	include	dibeng.inc
	include	macros.inc
    include brdconf.inc
    include minivdd.inc
    include ddctxt.inc

.586
_TEXT segment word public use16 'CODE'

; These are our external procedures in the 16 bit segment.
ifndef CODE32BPP
BitBlt16					PROTO FAR PASCAL, lpDestDev:DWORD, DestxOrg:WORD, DestyOrg:WORD, lpSrcDev:DWORD, SrcxOrg:WORD, SrcyOrg:WORD, xExt:WORD, yExt:WORD, Rop:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD
DIBtoDev16					PROTO FAR PASCAL, lpDestDev:DWORD, DestxOrg:WORD, DestyOrg:WORD, lpSrcDev:DWORD, SrcxOrg:WORD, SrcyOrg:WORD, xExt:WORD, yExt:WORD, Rop:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD
ExtTextOut16				PROTO FAR PASCAL, lpDestDev:DWORD, DestxOrg:WORD, DestyOrg:WORD, lpSrcDev:DWORD, SrcxOrg:WORD, SrcyOrg:WORD, xExt:WORD, yExt:WORD, Rop:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD
GlintPolygon                PROTO FAR PASCAL, lpDestDev:DWORD, wStyle:WORD, wCount:WORD, lpPoints:DWORD, lpPPen:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD, lpClipRect:DWORD
endif
SetRAMDAC_far		        PROTO FAR C         ;in PALETTE.ASM
InitialisePointer			PROTO FAR C

ClearCursorCache            PROTO FAR C         ; in pointer.c 

BeginAccess                 PROTO FAR PASCAL, lpDevice:DWORD, Left:WORD, Top:WORD, Right:WORD, Bottom:WORD, Flags:WORD
NearBeginAccess             PROTO NEAR C
FarBeginAccess              PROTO FAR C
EndAccess                   PROTO FAR PASCAL, lpDevice:DWORD, Flags:WORD
Far_Context_Change          PROTO FAR C
Context_Change              PROTO NEAR C
RestoreGDIContext			PROTO FAR C

GlintPolyScanLine           PROTO FAR PASCAL, lpDestDev:DWORD, wStyle:WORD, wCount:WORD, lpPoints:DWORD, lpPPen:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD, lpClipRect:DWORD     
GlintPolyLine               PROTO FAR PASCAL, lpDestDev:DWORD, wStyle:WORD, wCount:WORD, lpPoints:DWORD, lpPPen:DWORD, lpPBrush:DWORD, lpDrawMode:DWORD, lpClipRect:DWORD     
GlintDrawPolyLine           PROTO NEAR PASCAL

HookInt2Fh                  PROTO NEAR C        ;Hook into multiplexed interrupt
SetPaletteTranslate         PROTO FAR PASCAL    ;in palette.asm
SetRAMDAC_far               PROTO FAR C         ;in palette.asm
PhysicalEnable              PROTO NEAR C        ;Enable routine
RestoreInt2Fh               PROTO NEAR C        ;Restore multiplexed interrupt
GetScreenSettings           PROTO NEAR C

SetDriverInfo               PROTO FAR C, reset:WORD
GLDD_Disable                PROTO FAR C, lpDevice:DWORD

; Heap manipulation functions in heap.c
EnableOffscreenHeap			PROTO FAR C, xScreen:WORD, yScreen:WORD
DisableOffscreenHeap		PROTO FAR C
DeleteOffscreenBitmap		PROTO FAR C, osb:DWORD
CreateOffscreenBitmap		PROTO FAR C, osb:DWORD
ConsiderMovingToOffscreen	PROTO FAR C, osb:DWORD
ifdef DEBUG
DebugDIB                    PROTO FAR C, Level:DWORD, osb:DWORD
endif

SelectorAccessRights        PROTO FAR PASCAL   ;in KERNEL.EXE

GlobalAlloc                 PROTO FAR SYSCALL
GlobalLock                  PROTO FAR SYSCALL
GlobalUnlock                PROTO FAR SYSCALL
GlobalFree                  PROTO FAR SYSCALL
GlobalSmartPageLock         PROTO FAR SYSCALL

GetPrivateProfileInt        PROTO FAR SYSCALL    ; Kernel!GetProfileInt
GetModuleHandle             PROTO FAR SYSCALL
GetProcAddress              PROTO FAR SYSCALL
LoadLibrary                 PROTO FAR SYSCALL
AllocCSToDSAlias            PROTO FAR SYSCALL   ;get a DS alias for CS
FreeSelector                PROTO FAR SYSCALL   ;frees up selectors
ResetHiResMode              PROTO FAR SYSCALL

DIB_BitBlt                  PROTO FAR PASCAL  :DWORD,:WORD,:WORD,:DWORD,:WORD,:WORD,:WORD,:WORD,:DWORD,:DWORD,:DWORD 
DIB_ColorInfo               PROTO FAR PASCAL     ;in DIBENG.DLL 
DIB_Control                 PROTO FAR PASCAL     ;in DIBENG.DLL 
DIB_CheckCursorExt          PROTO FAR PASCAL     ;in DIBENG.DLL 
DIB_Control                 PROTO FAR PASCAL     ;in DIBENG.DLL 
DIB_Disable                 PROTO FAR PASCAL     ;in DIBENG.DLL 
DIB_Enable                  PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_EnumDFonts              PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_EnumObjExt		        PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_Inquire                 PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_Output                  PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_Pixel                   PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_RealizeObjectExt	    PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_Strblt                  PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_ScanLR                  PROTO FAR PASCAL  :DWORD,:WORD,:WORD,:DWORD,:WORD
DIB_DeviceMode              PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_ExtTextOut              PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_GetCharWidth            PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_DeviceBitmap            PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_FastBorder              PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_SetAttribute            PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_DibBltExt		        PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_CreateDIBitmap          PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_DibToDevice             PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_SetPaletteExt	        PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_GetPaletteExt	        PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_SetPaletteTranslateExt  PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_GetPaletteTranslateExt  PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_UpdateColorsExt 	    PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_StretchBlt              PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_StretchDIBits           PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_SelectBitmap            PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_BitmapBits              PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_ReEnable                PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_BeginAccess             PROTO FAR PASCAL     ;in DIBENG.DLL
DIB_EndAccess               PROTO FAR PASCAL     ;in DIBENG.DLL

_TEXT ends

_TEXT32 segment page public use32 'CODE'

; These are our external procedures in the 32 bit segment.
	ifdef CODE32BPP
	ExtTextOut32			PROTO FAR PASCAL
	endif
InitialiseFontCache         PROTO FAR PASCAL, CacheMode:DWORD         ;in text.asm
_TEXT32 ends

_OUTPUT32 segment page public use32 'CODE'

_OUTPUT32 ends

_BLIT32 segment page public use32 'CODE'

; This is the place we store our data segment
	ifdef CODE32BPP
	BitBlt32				PROTO FAR PASCAL
	CursorExclude           PROTO FAR PASCAL, xPos:DWORD, yPos:DWORD, xWidth:DWORD, yHeight:DWORD
	endif
InitialiseBitBlt            PROTO FAR C        ;in glintblt.asm
_BLIT32 ends

_INTERCEPT segment para public use16 'CODE'
InterceptGDI                PROTO FAR C        ; in GDIInter.asm
_INTERCEPT ends

_INIT segment
EXTERNDEF bReEnable:BYTE
EXTERNDEF DD_Disconnect:DWORD
EXTERNDEF DD_CurrentContext:DWORD
EXTERNDEF DD_CurrentColour:DWORD
EXTERNDEF DIBAccessCallFlag:BYTE
EXTERNDEF dwDDContext:DWORD
EXTERNDEF dwFBReadMode:DWORD
EXTERNDEF dwFBWriteMode:DWORD
EXTERNDEF dwRasterizerMode:DWORD
EXTERNDEF dwRenderCommand:DWORD
EXTERNDEF GLInfo:GlintInfo
EXTERNDEF OurVMHandle:WORD		        ;in VGA.ASM
EXTERNDEF RepaintAddr:DWORD
EXTERNDEF ScreenSelector:WORD           ;selector for the screen
EXTERNDEF VDDEntryPoint:DWORD		    ;in VGA.ASM
EXTERNDEF dwBppMask:DWORD
EXTERNDEF dwDpi:DWORD
EXTERNDEF dwEnabled:DWORD
EXTERNDEF dwPalettized:DWORD
EXTERNDEF dwWaitFifoSpace:DWORD
EXTERNDEF _lpDDHAL_SetInfo:DWORD
EXTERNDEF rop2Table:DWORD
EXTERNDEF pGLInfo:DWORD

; This is the place we store our data segment
EXTERNDEF RemappedDataSegment:DWORD

EXTERNDEF Temp1KSpace:BYTE


; Intercept table information
EXTERNDEF GDIDataSegment:WORD
EXTERNDEF GDI_ExtTextOut_Intercept:WORD
EXTERNDEF GDI_ExtTextOut_InterceptCmd:WORD

EXTERNDEF GLGamma:GlintGamma

_INIT ends

TEXT_CACHED_IN_OFFSCREEN    equ 03d01h
TEXT_CACHED_IN_MEMORY       equ 03d02h


; TEXTURE Unit defines
P_8BIT_TEXELS       equ			     0h
P_16BIT_TEXELS		equ			 80000h
P_32BIT_TEXELS		equ			100000h
P_4BIT_TEXELS		equ			180000h
P_24BIT_TEXELS		equ			200000h


;----------------------------------------------------------------------------
; Offscreen Bitmap Information
;----------------------------------------------------------------------------

OFFSCREEN_BITMAP   STRUCT
    OSB_DibEngine       db SIZE DIBENGINE dup (?)	; Standard DIB engine structure

    OSB_PixelOffset     dd  0						; Pixel Offset for this bitmap
    OSB_Heap            dd  0						; far heap ptr
    OSB_iUniq			dd  0
    OSB_cBlt			dd  0
OFFSCREEN_BITMAP    ENDS


; Definitions for DIBEngine.deDriverReserved
ACCELERATED equ 0000000000000001b       ;free DIB engine flag

;----------------------------------------------------------------------------
; Brush Information
;----------------------------------------------------------------------------

Cached_Brush8 STRUCT
    CB8_DIB_Brush8      db SIZE DIB_Brush8 dup (?)  ; Standard DIB engine structure
    CB8_Flags           dd 0
Cached_Brush8 ENDS

CB_COLOR_REALIZED   equ 00000001b
CB_MONO_REALISED    equ 00000010b
CB_DIB_REALIZED     equ 10000000b


;----------------------------------------------------------------------------
; Size overrides for 32->16 bit things
;----------------------------------------------------------------------------
wRegSel             equ WORD PTR dwRegSel
wRenderFamily       equ WORD PTR dwRenderFamily
wRamDacType         equ WORD PTR dwRamDacType
wFBSel              equ WORD PTR dwFBSel
wDisconCtrl         equ WORD PTR dwDisconCtrl
wRenderChipID       equ WORD PTR dwRenderChipID
wBpp                equ WORD PTR GLInfo.dwBpp                
wBppMask            equ WORD PTR dwBppMask            
wCursorType         equ WORD PTR GLInfo.dwCursorType         
wDpi                equ WORD PTR dwDpi                
wEnabled            equ WORD PTR dwEnabled            
wPalettized         equ WORD PTR dwPalettized         
wScreenHeight       equ WORD PTR GLInfo.dwScreenHeight       
wScreenWidth        equ WORD PTR GLInfo.dwScreenWidth        
wScreenWidthBytes   equ WORD PTR GLInfo.dwScreenWidthBytes   
wWaitFifoSpace      equ WORD PTR dwWaitFifoSpace   

;----------------------------------------------------------------------------
; I N T E R C E P T Information
;----------------------------------------------------------------------------

INTERCEPT_END       = 0
INTERCEPT_VALIDATE  = 1
INTERCEPT_RETURN    = 2
INTERCEPT_MOVE      = 3

DCINFO   STRUCT
    DC_Unknown0            dw  0
    DC_Type                dw  0
    DC_Unknown4            dw  0
    DC_Unknown6            dw  0
    DC_Unknown8            dw  0
    DC_Unknown10           dw  0
    DC_Unknown12           dw  0
    DC_Unknown14           dw  0
    DC_Unknown16           dw  0
    DC_Unknown18           dw  0
    DC_PParentDevice       dd  0
    DC_Unknown24           dw  0
    DC_Unknown26           dw  0
    DC_Unknown28           dw  0
    DC_Unknown30           dw  0
    DC_Unknown32           dw  0
    DC_Unknown34           dw  0
    DC_ClipRegion          dw  0
    DC_Unknown38           dw  0
    DC_Unknown40           dw  0
    DC_Unknown42           dw  0
    DC_Unknown44           dw  0
    DC_Unknown46           dw  0
    DC_PDevice             dd  0
    DC_Unknown52           dw  0
    DC_Unknown54           dw  0
    DC_Unknown56           dw  0
    DC_Unknown58           dw  0
    DC_Unknown60           dw  0
    DC_Unknown62           dw  0
    DC_Unknown64           dw  0
    DC_Unknown66           dw  0
    DC_LPFONT              dd  0
    DC_Unknown72           dw  0
    DC_Scr_X               dw  0
    DC_Scr_Y               dw  0
;DrawMode structure starts here
  DC_Rop2		           dw	0	;The 16-bit encoded Logical op		    
  DC_bkMode	               dw	0	;Background Mode (for text only)	    
  DC_bkColor	           dd	0	;Physical background Color		        
  DC_TextColor	           dd	0	;Physical text (forground) color	    
  DC_TBreakExtra	       dw	0	; total pixles to stuff into a line	
  DC_BreakExtra	           dw	0	; div(TBreakExtra, BreakCount)		
  DC_BreakErr	           dw	0	; running error term			        
  DC_BreakRem	           dw	0	; mod(TBreakExtra, BreakCount)		    
  DC_BreakCount	           dw	0	; count of breaks in the line		
  DC_CharExtra	           dw	0	; extra pixles to stuff after each char (used to space out a font)
  DC_LbkColor	           dd	0	;Logical background color		        
  DC_LTextColor	           dd	0	;Logical Text (forground) color 	
  DC_ICMCXform	           dd	0	;Transform for DIC image color matcher	
  DC_StretchBltMode        dw  0   ;Stretch Blt mode           
  DC_eMiterLimit           dd  0   ;Miter Limit (single precision IEEE float)   
;DrawMode structure ends here
    DC_Unknown120          dw  0
    DC_DrawFlags           db  0    
    DC_Unknown123          db  0
    DC_Unknown124          dw  0
    DC_Unknown126          dw  0
    DC_Unknown128          dw  0
    DC_Unknown130          dw  0
    DC_Unknown132          dw  0
    DC_Unknown134          dw  0
    DC_Unknown136          dw  0
    DC_Unknown138          dw  0
    DC_Unknown140          dw  0
    DC_Unknown142          dw  0
    DC_Unknown144          dw  0
    DC_Unknown146          dw  0
    DC_Unknown148          dw  0
    DC_Unknown150          dw  0
    DC_Unknown152          dw  0
    DC_Unknown154          dw  0
    DC_Unknown156          dw  0
    DC_Unknown158          dw  0
    DC_Unknown160          dw  0
    DC_Unknown162          dw  0
    DC_Unknown164          dw  0
    DC_Unknown166          dw  0
    DC_Unknown168          dw  0
    DC_Unknown170          dw  0
    DC_Unknown172          dw  0
    DC_Unknown174          dw  0
    DC_Unknown176          dw  0
    DC_Unknown178          dw  0
    DC_Unknown180          dw  0
    DC_Unknown182          dw  0
    DC_Unknown184          dw  0
    DC_Unknown186          dw  0
    DC_Unknown188          dw  0
    DC_Unknown190          dw  0
    DC_Unknown192          dw  0
    DC_Unknown194          dw  0
    DC_Unknown196          dw  0
    DC_Unknown198          dw  0
    DC_Unknown200          dw  0
    DC_Unknown202          dw  0
    DC_Unknown204          dw  0
    DC_Unknown206          dw  0
    DC_Unknown208          dw  0
    DC_Unknown210          dw  0
    DC_Unknown212          dw  0
    DC_Unknown214          dw  0
    DC_Unknown216          dw  0
    DC_Unknown218          dw  0
    DC_Unknown220          dw  0
    DC_Unknown222          dw  0
    DC_Unknown224          dw  0
    DC_ExtentBottom1       dw  0
    DC_ExtentRight1        dw  0
    DC_ExtentTop1          dw  0
    DC_ExtentLeft1         dw  0
    DC_ExtentBottom2       dw  0
    DC_ExtentRight2        dw  0
    DC_ExtentTop2          dw  0
    DC_ExtentLeft2         dw  0
    DC_Unknown242          dw  0
    DC_Unknown244          dw  0
    DC_Unknown246          dw  0
    DC_Unknown248          dw  0
    DC_ExtentFlags         db  0    ; If bit 3 is set, update first extent region
									; Bit 2 set indicates first has been updated
									; If bit 1 is set, update second extent region
									; Bit 0 set indicates second has been updated
    DC_Unknown251          db  0
    DC_Unknown252          dw  0
    DC_Unknown254          dw  0
    DC_Unknown256          dw  0
    DC_Unknown258          dw  0
    DC_Unknown260          dw  0
    DC_Unknown262          dw  0
    DC_Unknown264          dw  0
    DC_Unknown266          dw  0
    DC_Unknown268          dw  0
DCINFO   ENDS

RGNINFO STRUCT
    RGN_Unknown0            dw  0 ;BF0
    RGN_Type                dw  0
    RGN_Unknown4            dw  0 ;E2E8
    RGN_Unknown6            dw  0 ;E
    RGN_Unknown8            dw  0 ;0
    RGN_Unknown10           dw  0 ;CCCC
    RGN_Unknown12           dw  0 ;0
    RGN_StructSize          dd  0
    RGN_Count               dw  0 ;2
    RGN_Unknown20           dw  0 ;2
    RGN_BoundLeft           dw  0
    RGN_BoundTop            dw  0
    RGN_BoundRight          dw  0
    RGN_BoundBottom         dw  0

    RGN_Unknown30           dw  0 ;2
    RGN_Clip1Top            dw  0
    RGN_Clip1Bottom         dw  0
    RGN_Clip1Left           dw  0
    RGN_Clip1Right          dw  0
    RGN_Unknown40           dw  0 ;2

    RGN_Unknown42           dw  0 ;2
    RGN_Clip2Left           dw  0
    RGN_Clip2Top            dw  0
    RGN_Clip2Right          dw  0
    RGN_Clip2Bottom         dw  0
    RGN_Unknown52           dw  0 ;2
RGNINFO ENDS

;----------------------------------------------------------------------------
; M A C R O S
;----------------------------------------------------------------------------
WaitFifo macro n
	local enough, more
ifdef SOME_DISCONNECT
	ifdef CODE32BPP
		cmp     dwWaitFifoSpace, 0
		je      enough
		sub     dwWaitFifoSpace, n
		jae     more
		mov     dwWaitFifoSpace, 0
	else
		cmp     wWaitFifoSpace, 0
		je      enough
		sub     wWaitFifoSpace, n
		jae     more
		mov     wWaitFifoSpace, 0
	endif
	more:
		cmp     InFIFOSpace,n
		jb      more
	enough:
endif
    endm

GetFifoCount macro Variable:REQ
local enough
ifdef SOME_DISCONNECT
    mov     Variable, 07fffh
    cmp     dwWaitFifoSpace, 0
    je      enough
    mov     Variable, InFIFOSpace
enough:
endif
    endm

WaitFifoIfNeeded macro Number:REQ, Variable:REQ
 local enough, more
ifdef SOME_DISCONNECT
    sub     Variable, Number
    jge     enough
more:
    mov     Variable, InFIFOSpace
    sub     Variable, Number
    jl      more
enough:
endif
    endm

WaitFifoIfNeededVar macro Number:REQ, Variable:REQ
 local enough, more
ifdef SOME_DISCONNECT
    sub     Variable, Number
    jge     enough
	push	eax
more:
    mov     eax, InFIFOSpace
    sub     eax, Number
    jl      more
	mov		Variable, eax
	pop		eax
enough:
endif
    endm

WaitFifoIfNeededVarReg macro Number:REQ, Variable:REQ, Reg:REQ
 local enough, more
ifdef SOME_DISCONNECT
    sub     Variable, Number
    jns     enough
more:
    mov     Reg, InFIFOSpace
    sub     Reg, Number
    jl      more
	mov		Variable, Reg
enough:
endif
    endm

BEST_ALIGN	macro
		align	16
		endm

ifdef CODE32BPP

	StackMangle16To32	macro
		pop		eax				;fetch the 32 bits (16:16) of return address
		mov		ebx,eax
		and		eax,0ffffh		;clear the high word of the offset
		shr		ebx,16
		push	ebx
		push	eax
	endm

	StackMangle32To16	macro
		pop		eax					; unmangle the stack...
		pop		ebx					; now we have the 64 bit ret addr
		shl		ebx,16				; get the segment in the high word
		or		eax,ebx				; now stick them together
		push	eax					; and put it back on the stack as 16:16
	endm

endif
;----------------------------------------------------------------------------
; GLINT SPECIFIC
;----------------------------------------------------------------------------

;DISPLAY_CONTEXT macro
; local NoSwitch
;ifdef CODE32BPP
;        cmp     GLInfo.dwCurrentContext,CONTEXT_DISPLAY_HANDLE  
;        je      NoSwitch
;        mov     eax, CONTEXT_DISPLAY_HANDLE
;        call    Far_Context_Change
;else
;		cmp     WORD PTR GLInfo.dwCurrentContext,CONTEXT_DISPLAY_HANDLE    ;; current ?
;        je      NoSwitch
;        mov     ax, CONTEXT_DISPLAY_HANDLE
;        call    Context_Change
;endif
;NoSwitch:
;endm
DISPLAY_CONTEXT macro
ifdef CODE32BPP
		mov     eax, CONTEXT_DISPLAY_HANDLE
		call    Far_Context_Change
else
		mov     eax, CONTEXT_DISPLAY_HANDLE
		call    Context_Change
endif
endm

SYNC_WITH_GLINT macro
 local SWG_WaitSync, SWG_WaitFifo
SWG_WaitFifo:
    cmp     InFIFOSpace,2
    jb      SWG_WaitFifo
    mov     FilterMode,00010000000000b ;enable tag out
    mov     Sync,0
SWG_WaitSync:
    cmp     OutFIFOWords,1
    jb      SWG_WaitSync
    cmp     GPFifo1,188h
    jne     SWG_WaitSync
ifdef CODE32BPP
    mov     dwWaitFifoSpace, 0
else
    mov     wWaitFifoSpace, 0
endif
endm

DISABLE_FIFO_CHECKING macro
ifdef CODE32BPP
    mov     dwWaitFifoSpace, 0
else
    mov     wWaitFifoSpace, 0
endif
endm

ENABLE_FIFO_CHECKING macro
  ifdef SOME_DISCONNECT
	local   NoDelta
		mov     dwWaitFifoSpace, 33
		test    GLInfo.dwFlags, GMVF_DELTA
		je      NoDelta
		mov     dwWaitFifoSpace, 80
	NoDelta:
  endif
endm


ifdef SOME_DISCONNECT

	ENABLE_DISCONNECT macro Segment:REQ
	local ED_Done
		ifdef CODE32BPP
			mov     ecx, 1
			cmp     DD_Disconnect, ecx				; Do we need to change minicontext?
			je      ED_Done
			mov     ebx, GLINT_REGISTERS_OFFSET
			mov		ebx, GLInfo.dwDisconCtrl
			mov		DWORD PTR Segment:[ebx], ecx		    ;turn on disconnect
			mov     DD_Disconnect, ecx
		else
			cmp     WORD PTR DD_Disconnect, 0		; Do we need to change minicontext?
			jne     ED_Done
			mov		bx, GLInfo.wDisconCtrl
			mov		DWORD PTR Segment:[bx], 1		;turn on disconnect
			mov     WORD PTR DD_Disconnect, 1
		endif
	ED_Done:
	endm

	DISABLE_DISCONNECT macro Segment:REQ
	local DD_Done
	ifdef CODE32BPP
		cmp     DD_Disconnect, 1				; Do we need to change minicontext?
		jne     DD_Done
		mov		ebx, GLInfo.dwDisconCtrl
		xor     ecx, ecx
		add     ebx, GLINT_REGISTERS_OFFSET
		mov		DWORD PTR ds:[ebx], ecx		    ;turn on disconnect
		mov     DD_Disconnect, ecx
	else
		cmp     WORD PTR DD_Disconnect, 1		; Do we need to change minicontext?
		jne     DD_Done
		mov		bx, GLInfo.wDisconCtrl
		xor     cx, cx
		mov		DWORD PTR Segment:[bx], 0		;turn on disconnect
		mov     WORD PTR DD_Disconnect, 0
	endif
	DD_Done:
	endm

else
	
	ENABLE_DISCONNECT macro	Segment:REQ
	endm

	DISABLE_DISCONNECT macro Segment:REQ
	endm

endif



ifdef DEBUG

DEBUGDIB macro Level:REQ, pDIB:REQ
    pushad
    invoke DebugDIB, Level, pDIB
    popad
endm

else

DEBUGDIB macro Level:REQ, pDIB:REQ
endm

endif


