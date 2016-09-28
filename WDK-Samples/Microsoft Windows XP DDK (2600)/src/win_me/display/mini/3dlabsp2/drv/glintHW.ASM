;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: GLINTHW.ASM
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
    .xlist
    incDevice = 1                   ;Include control for gdidefs.inc
    incDrawMode = 1                 ;Include DRAWMODE structure
    include glint.inc
    include gdidefs.inc
    include valmode.inc
    .list
;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------
GET386API       equ     1684h           ;Get API entry point from VxD.
VDD             equ     000Ah           ;id of Virtual Display Driver.
STOP_IO_TRAP    equ     4000h           ; stop io trapping
START_IO_TRAP	equ	4007h		; re-start io trapping

;----------------------------------------------------------------------------
; D A T A
;----------------------------------------------------------------------------
_INIT segment

SCREENSTRADDLESMEMORY equ 0

GLInfo GlintInfo <> 

GLGamma GlintGamma <0,>

globalD dwFBReadMode ,0
globalD dwFBWriteMode,0
globalD dwRenderCommand,0
globalD dwRasterizerMode,38Dh
globalW ScreenSelector,0                ;selector for the screen
globalD RemappedDataSegment,0           ;Far Pointer to GLInfo

PUBLIC ValidateModeTable
ValidateModeTable   dw   320,  200
                    dw   320,  240
                    dw   512,  384
                    dw   640,  400
                    dw   640,  480
                    dw   800,  600
                    dw  1024,  768
                    dw  1152,  870
                    dw  1280, 1024
                    dw  1600, 1200
                    dw     0,    0      ; Terminating mode
                    dw     0,    0      ; Spare for debug purposes

_INIT ends

;----------------------------------------------------------------------------
; S T R U C T U R E S
;----------------------------------------------------------------------------
FOURTHMEG = 0
HALFMEG	  = 1
ONEMEG	  = 2*HALFMEG
TWOMEG	  = 2*ONEMEG
THREEMEG  = 3*ONEMEG
FOURMEG   = 4*ONEMEG
FIVEMEG   = 5*ONEMEG
SIXMEG    = 6*ONEMEG
SEVENMEG  = 7*ONEMEG
EIGHTMEG  = 8*ONEMEG

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT segment
assumes cs,_TEXT
        .386

;----------------------------------------------------------------------------
; PhysicalEnable
;----------------------------------------------------------------------------
PhysicalEnable	PROC NEAR C
        assumes ds,_INIT
        assumes es,nothing
        assumes gs,nothing
        assumes fs,nothing


;----------------------------------------------------------------------------
; Hook up with the vdd.
;
; The MiniVDD requires us to register ourselves with it in order to activate
; its ability to virtualize graphics mode DOS apps in a window or in the
; background.  The VDD will also allocate some memory so that it can perform
; this virtualization correctly.  Let's setup this call and pass down the
; proper information to the MiniVDD.
;----------------------------------------------------------------------------
PLABEL PE_HookUpWithVDD

    test    GLInfo.dwCurrentContext,0ffffh
    jz      @f                      ; j if no context yet
    DISPLAY_CONTEXT
@@:
	mov	    eax,VDD_DRIVER_REGISTER ;this is function code for VDD PM API
	mov     ebx,GLInfo.dwDeviceHandle 	    ;VDD PM API needs this
	mov	    di,_TEXT		        ;send ES:DI --> routine that VDD calls
	mov	    es,di			        ;to set us back into Windows mode
	mov	    di,CodeOFFSET ResetHiResMode
	xor	    edx,edx                 ;assume 4 plane virtualization should be done
    test    GLInfo.dwFlags,GMVF_TRYTOVIRTUALISE4PLANEVGA
    jnz     @f

    not     edx                     ;disable 4 plane virtualization
@@:
	call	dword ptr VDDEntryPoint

	mov	eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov     ebx,GLInfo.dwDeviceHandle 	;VDD PM API needs this
    mov     ecx, MINIVDD_INITIALISEMODE
    mov     edx, GLInfo.DesktopDisplayInfo.ddiDevNodeHandle
    mov     si,DataOFFSET GLInfo
	call	dword ptr VDDEntryPoint ;
    cmp     eax,0                   ; did the mode change succeed?
    jne     PE_Fail                 ; j if not

;----------------------------------------------------------------------------
;At this point:
; EAX contains VDD_DRIVER_REGISTER if the call failed.
;
;----------------------------------------------------------------------------
	cmp	eax,VDD_DRIVER_REGISTER ;did the call fail?
	je	PE_Fail	;yes, just use values that we've got.
        mov     bx,GLInfo.wFBSel
        mov     ScreenSelector,bx               ; save the allocated segments
        mov     eax,GLInfo.ddFBSize
        mov     GLInfo.dwVideoMemorySize,eax
                                                                                  
        mov     ax, 0                           ; Set up our register pointer for C code  
        mov     WORD PTR GLInfo.pRegs, ax       ; Write the offset               
        mov     ax,GLInfo.wRegSel       
        mov     WORD PTR GLInfo.pRegs+2, ax     ; Write the segment pointer
        mov     es,ax
        assumes es,Glint

        mov     eax,dwRasterizerMode
        mov     RasterizerMode,eax
        mov     eax,FBReadMode
        and     eax,0777o
        mov     dwFBReadMode,eax        ;save stride stuff
        mov     eax,1                   ;basic write mode
        xor     ecx,ecx                 ;render without fast fill
        test    GLInfo.dwFlags,GMVF_FFON ;is fast fill on?
        je      @f                      ;j if not
        mov     cl,1000b                ;enable fast fill
        mov     bx,wBpp                 ;check bits per pix
@@:
        mov     eax,FBWriteMode         ; set by VDD
        and     eax,110b                ; block size
        shl     eax,3
        or      ecx,eax                 ; include in render command
        shr     eax,3
        or      al,1
;        WaitFifo 1
;        cmp     bx,32                 ;true color?
;        jne     @f
;        or      cl,100000b            ;set block size 32
;        or      al,100b
;@@:
PE_SetWriteMode:
        mov     dwFBWriteMode,eax
        mov     FBWriteMode,eax
        mov     dwRenderCommand,ecx

; check for delta
        test    GLInfo.dwFlags,GMVF_DELTA
        jz      @f
        WaitFifo 1
        xor     eax,eax
        mov     DeltaMode,eax
@@:

; Set up Offscreen Bitmap code

        mov     DD_CurrentContext, DD_UNDEFINED_CTXT
        mov     GLInfo.dwCurrentOffscreen, 0
        invoke  EnableOffscreenHeap, wScreenWidth, wScreenHeight

; set up pointer code

        invoke  InitialisePointer

ifdef GDI_BYPASS
; set up GDI Interception

        test    GLInfo.dwFlags,GMVF_INTCPTGDI	; Should GDI calls be intercepted?
        jz      @f
        invoke  InterceptGDI					; yes
@@:
endif

; set the bitblt functions up properly
        invoke  InitialiseBitBlt

PLABEL PE_SaveState
;
;Call the VDD to order it to save the current register state.  This will
;hopefully assure that the Windows VM has the running state saved:
;
	mov	eax,VDD_SAVE_DRIVER_STATE
	mov     ebx,GLInfo.dwDeviceHandle 	;
	call	dword ptr VDDEntryPoint ;

@@:	clc				;Indicate success
	ret

PLABEL PE_Fail
	stc				;indicate failure
	ret
PhysicalEnable	endp


;----------------------------------------------------------------------------
; ResetHiResMode
;   This function is called by the VDD to restore the graphics adapter 
;   into graphics mode when switching to the system vm.  Note that this
;   function is in a page-locked segment.
;   
;   Called when reentering the windows vm.
;----------------------------------------------------------------------------
ResetHiResMode PROC FAR SYSCALL 
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
    assumes es,nothing
    assumes gs,nothing
	assumes fs,nothing
	push	esi
	push	edi

    call    PhysicalEnable          ; reset the chip etc

	cmp	    wBpp,8
	ja	    short RHR_CallVDD
	mov	    cx,256
	xor	    ax,ax
	push	ds
	lds	    si, DWORD PTR GLInfo.lpColorTableOff		;ds:[si]-->color table
	call	SetRAMDAC_far		;set up initial palette
	pop	    ds

PLABEL RHR_CallVDD
	les	    si,GLInfo.lpDriverPDevice
lock	and	es:[si].deFlags,not BUSY

;
;Call the VDD to order it to save the current register state.  This will
;hopefully assure that the Windows VM has the running state saved:
;
	mov	eax,VDD_SAVE_DRIVER_STATE
	mov     ebx,GLInfo.dwDeviceHandle 	;
	call	dword ptr VDDEntryPoint ;
	pop	edi
	pop	esi
	ret

ResetHiResMode endp

subttl		Validate Mode Entry Point
page +
cProc		ValidateMode1,<FAR,PUBLIC>
;
	parmD	lpValMode
;
cBegin
.386
;
;Entry:
;	lpValMode = DISPVALMODE structure
;Exit:
;	AX = VALMODE_YES (0), VALMODE_MAYBE (1), VALMODE_NO (2)
;
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
	push	esi			;save 32 bit versions of ESI & EDI
	push	edi			;
    push    ebx
    push    ecx
    push    es
;
;
;
public	VMGetModeInfo
VMGetModeInfo:
	les	di,lpValMode
    xor     eax, eax
    xor     ecx, ecx
    mov     ax,es:[di].dvmXRes
    mov     cx,es:[di].dvmYRes

    xor     bx, bx

VM_NextResolution:
    mov     dx, [ValidateModeTable + bx]        ; Get X
    add     bx, 4
    test    dx, dx                              ; Reached the end of list?
    je      VMErrorExit
    cmp     ax, dx                              ; Is the X resolution the same?
    jne     VM_NextResolution
    mov     dx, [ValidateModeTable - 2 + bx]    ; Get Y
    cmp     cx, dx                              ; Is the Y resolution the same?
    jne     VM_NextResolution

    mov     bx, es:[di].dvmBpp
    cmp     bx, 8
    je      VM_ValidResolution
    cmp     bx, 16
    je      VM_ValidResolution
    cmp     bx, 32
    je      VM_ValidResolution
    test    GLInfo.dwFlags, GMVF_EXPORT24BPP    ; Are we exporting 24bpp modes?
    je      VMErrorExit
    cmp     bx, 24
    je      VM_ValidResolution                  ; If so, accept 24 bpp
    jmp     VMErrorExit                         ; Fail all other depths

VM_ValidResolution:
    cmp     GLInfo.wRenderFamily, PERMEDIA_ID
    jne     VM_CheckMemorySize
    cmp     GLInfo.wRenderChipID, PERMEDIA_ID     ; Dont support 24bpp on P1
    je      VM_LimitP1Resolutions
    cmp     GLInfo.wRenderChipID, TIPERMEDIA_ID
    je      VM_LimitP1Resolutions

    ; Eliminate resolutions not supported on a P2
    cmp     ax, 1600                            ; Support up to 1600x1200 8 and 16bpp
    ja      VMErrorExit
    cmp     bx, 16
    jle     VM_CheckMemorySize
    cmp     ax, 1280                            ; Support up to 1280x1024 24bpp and 32bpp
    ja      VMErrorExit
    jmp     VM_CheckMemorySize

VM_LimitP1Resolutions:
    ; Rule out resolutions not supported on a Permedia
    cmp     ax, 1600                            ; Support up to 1600x1200 8bpp
    ja      VMErrorExit
    cmp     bx, 8                               ; Accept resolution if 8bpp
    je      VM_CheckMemorySize
    cmp     bx, 16                              
    je      @f
    cmp     bx, 32
    jne     VMErrorExit
@@:                 
    cmp     ax, 1280
    ja      VMErrorExit                         ; Support up to 1280x1024 16bpp and 32bpp
VM_CheckMemorySize:
    imul    ecx
    mov     bx,es:[di].dvmBpp
    cmp     bx,8                                ;make bytes
    je      @f
    add     eax,eax
    cmp     bx,16
    je      @f
    add     eax,eax
@@: cmp     eax,GLInfo.dwVideoMemorySize
    ja      VMErrorExit

    cmp     GLInfo.wRenderFamily, GLINT_ID
    jne     VM_AcceptMode

    ; Max size of a Glint screen is currently 4Mb
    cmp     eax,400000h
    ja      VMErrorExit

VM_AcceptMode:
	mov	ax,VALMODE_YES		;return that we support the mode
	jmp	VMExit			    ;and we're done!
;
public	VMErrorExit
VMErrorExit:
	mov	ax,VALMODE_NO_NOMEM	;oops, we don't support the mode
;
VMExit:
    pop     es
    pop     ecx
    pop     ebx
	pop	    edi			;
	pop	    esi			;
.286c
cEnd
;

.586

MiniVDD_MemoryRequest	PROC FAR C PUBLIC USES ds di si, InputBuffer:DWORD, OutputBuffer:DWORD
	mov		eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov		ebx,GLInfo.dwDeviceHandle
	mov		esi,InputBuffer
	mov		edx,OutputBuffer
	mov		ecx,MINIVDD_MEMORYREQUEST
	call	dword ptr VDDEntryPoint
	ret
MiniVDD_MemoryRequest ENDP

MiniVDD_I2CRequest	PROC FAR C PUBLIC USES ds di si, InputBuffer:DWORD, OutputBuffer:DWORD
	mov		eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov		ebx,GLInfo.dwDeviceHandle
	mov		esi,InputBuffer
	mov		edx,OutputBuffer
	mov		edi,GLInfo.DesktopDisplayInfo.ddiDevNodeHandle	; Need the devnode to talk to the correct I2C
	mov		ecx,MINIVDD_I2CREQUEST
	call	dword ptr VDDEntryPoint
	ret
MiniVDD_I2CRequest ENDP

MiniVDD_VMIRequest	PROC FAR C PUBLIC USES ds di si, InputBuffer:DWORD, OutputBuffer:DWORD
	mov		eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov		ebx,GLInfo.dwDeviceHandle
	mov		esi,InputBuffer
	mov		edx,OutputBuffer
	mov		edi,GLInfo.DesktopDisplayInfo.ddiDevNodeHandle	; Need the devnode to talk to the correct I2C
	mov		ecx,MINIVDD_VMIREQUEST
	call	dword ptr VDDEntryPoint
	ret
MiniVDD_VMIRequest ENDP

MiniVDD_AllocateGlobalMemory    PROC FAR C PUBLIC USES ds di si, SizeToAllocate:DWORD
	mov	    eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov     ebx,GLInfo.dwDeviceHandle 	                    ;VDD PM API needs this
    mov     ecx, MINIVDD_ALLOCATEMEMORY
    mov     edx, SizeToAllocate
	call	dword ptr VDDEntryPoint 
    ret
MiniVDD_AllocateGlobalMemory ENDP

MiniVDD_FreeGlobalMemory    PROC FAR C PUBLIC USES ds di si, SelectorToFree:DWORD
	mov	    eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov     ebx,GLInfo.dwDeviceHandle 	                    ;VDD PM API needs this
    mov     ecx, MINIVDD_FREEMEMORY
    mov     edx, SelectorToFree
	call	dword ptr VDDEntryPoint 
    ret
MiniVDD_FreeGlobalMemory ENDP

MiniVDD_ReadRegistry PROC FAR C PUBLIC USES ds es di si, RegKey:DWORD, KeyValue:DWORD, KeySize:WORD, KeyType:WORD 
	mov	    eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov     ebx,GLInfo.dwDeviceHandle 	                    ;VDD PM API needs this
    mov     ecx, MINIVDD_READREGISTRY
    mov     dx, KeySize
    shl     edx, 16
    mov     dx, KeyType
    lds     di, KeyValue
    les     si, RegKey
	call	dword ptr VDDEntryPoint 
    ret
MiniVDD_ReadRegistry ENDP

MiniVDD_WriteRegistry PROC FAR C PUBLIC USES ds es di si, RegKey:DWORD, KeyValue:DWORD, KeySize:WORD, KeyType:WORD 
	mov	    eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov     ebx,GLInfo.dwDeviceHandle 	                    ;VDD PM API needs this
    mov     ecx, MINIVDD_WRITEREGISTRY
    mov     dx, KeySize
    shl     edx, 16
    mov     dx, KeyType
    lds     di, KeyValue
    les     si, RegKey
	call	dword ptr VDDEntryPoint 
    ret
MiniVDD_WriteRegistry ENDP


_TEXT ends

end
