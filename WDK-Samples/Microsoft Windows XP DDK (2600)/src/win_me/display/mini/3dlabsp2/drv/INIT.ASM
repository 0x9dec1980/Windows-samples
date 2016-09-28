;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: init.asm
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
    .xlist
    include windows.inc
    include glint.inc
    include context.inc
    .list

;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------
NO_REFRESH equ MONITOR_DEVNODE_NOT_ACTIVE+MONITOR_INFO_NOT_VALID+MONITOR_INFO_DISABLED_BY_USER

;----------------------------------------------------------------------------
; D A T A
;----------------------------------------------------------------------------
_INIT segment
globalD dwWaitFifoSpace,0                ;Number of Fifos to wait for before relying
                                         ;on Disconnect.
globalD dwDpi,96                         ;Contains the dpi setting.
globalD dwBppMask,11B                    ;mask for pixels to 32 bit
globalD dwPalettized,0                   ;0 = not palettized, 1 = palettized
globalD VDDEntryPoint,0
globalW OurVMHandle,0
globalB bReEnable,0                     ;only set for re-enable call
_INIT ends

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
_TEXT segment
assumes cs,_TEXT
assumes ds,_INIT

;-----------------------------------------------------------------------
; DriverInit
;
; Windows display driver initialization.  Called at driver load time.
; Entry:
;       CX = size of heap
;       DI = module handle
;       DS = automatic data segment
;       ES:SI = address of command line (not used)
;-----------------------------------------------------------------------
DriverInit PROC FAR C PUBLIC USES si di
	push	_TEXT
	call	GlobalSmartPageLock

    mov     GLInfo.dwDeviceHandle, 0
    mov     RemappedDataSegment, _INIT
    mov     WORD PTR pGLInfo+2, _INIT
    mov     WORD PTR pGLInfo, OFFSET GLInfo
    mov     ax,1                            ; initialization successful.
    ret
    
DriverInit endp

;-----------------------------------------------------------------------
; GetScreenSettings
; Entry:
;   ds = Data Segment to use
; Exit:
;  wResolution = 1,2,3,4
;  wBpp = 8,15,16,24,32
;  wDpi = 96,120
;  wPalettized = 0 or 1
;-----------------------------------------------------------------------
GetScreenSettings PROC NEAR C PUBLIC

; Get the DisplayInfo structure from the VDD.  This structure contains
; the resolution, color depth, etc. from the registry.  Update wBpp and
; wResolution with data from this structure.
; The calling convention for this is:
; entry:
;   eax = VDD_GET_DISPLAY_CONFIG
;   ebx = OurVMHandle
;   ecx = size of DISPLAYINFO structure + minivdd specific additions
;   es:edi--> DISPLAYINFO structure
; exit:
;   eax =  VDD_GET_DISPLAY_CONFIG means vdd not present.  Assume some
;           basic settings.
;   eax = 0 means success.
;  eax = -1 means error reading registry.  Assume basic settings.

; Check if the user wants to override the registry.

    push    ds
    pop     es
    mov     GLInfo.dwBpp, 0ffffffffh            ; Invalid depth
	lea	    edi,GLInfo.DesktopDisplayInfo;es:edi-->DispInfo structure
	mov	    eax,VDD_GET_DISPLAY_CONFIG
	mov 	ebx,GLInfo.dwDeviceHandle
	mov	    ecx,size VDDDISPLAYINFO
	call	dword ptr VDDEntryPoint
	cmp	    eax,VDD_GET_DISPLAY_CONFIG  ;Did the call work?
	je	    GSS_ValidateBpp		        ;no.
	inc	    eax			                ;yes. Could vdd read the registry?
	jz	    GSS_ValidateBpp		        ;no.
    xor     esi, esi
	mov	    si,GLInfo.DesktopDisplayInfo.ddiXRes ;si = xRes
    mov     GLInfo.dwScreenWidth,esi
	mov	    si,GLInfo.DesktopDisplayInfo.ddiYRes ;si = yRes
    mov     GLInfo.dwScreenHeight,esi

	movzx	eax,GLInfo.DesktopDisplayInfo.ddiBpp       ;ax = Bpp
	mov     GLInfo.dwBpp,eax               ;update wBpp
    mov     ebx,11B                 ;mask for 8bpp
    mov     cl,2                    ;and shift
    cmp     ax,8
    je      @f                      ;j if 8 bpp
    shr     bx,1                    ;adjust for 16 bpp
    shr     cl,1
    cmp     ax,16
    je      @f                      ;j if so
    xor     ebx,ebx                 ;no mask for 32 bpp
    mov     cl,bl                   ;or shift
@@: 
    and     ebx, 0ffffh
    mov     dwBppMask,ebx
    mov     GLInfo.bPixelToWordShift,cl
	movzx   eax,GLInfo.DesktopDisplayInfo.ddiDPI           ;ax = DPI
    mov     bl, 2
    sub     bl, cl
    mov     GLInfo.bPixelToBytesShift,bl
	test	ax,ax			        ;is Dpi valid?
	jz	@f      		            ;no.
	mov  	dwDpi,eax                     ;yes. update wDpi
@@:
PLABEL GSS_ValidateBpp

; validate the Bpp.

    mov     ebx,GLInfo.dwScreenWidth
    mov     eax,GLInfo.dwBpp        ;get the unvalidated value.
    cmp     ax,4                    ;valid bpp ?
    je      GSS_BppOK                ;yes.
    cmp     ax,8                    ;valid bpp ?
    je      GSS_BppOK                ;yes.
    add     ebx,ebx                   ; *2 for width
    cmp     ax,16                   ;valid bpp ?
    je      GSS_BppOK                ;yes.
    add     ebx,ebx
    cmp     ax,32                   ;valid bpp ?
    je      GSS_BppOK                ;yes.

    ; Accept 24bpp on all boards for now. This may be called
    ; before we know what board we are on.
    sub     ebx, GLInfo.dwScreenWidth       ; ebx = PixelWidth * 4 -> PixelWidth * 3
    cmp     ax,24
    je      GSS_BppOK

GSS_BppInvalid:
	mov	    eax,0ffffffffh       			;nothing matched, fail init
GSS_BppOK:
    mov     GLInfo.dwBpp,eax
    mov     dwPalettized,0
    cmp     ax,8
    jne     @f
    test    GLInfo.dwFlags, GMVF_8BPPRGB
    jne     @f
    mov     dwPalettized,1
@@: 
    mov     GLInfo.dwScreenWidthBytes,ebx
    ret
GetScreenSettings ENDP

_TEXT   ends

end     DriverInit
