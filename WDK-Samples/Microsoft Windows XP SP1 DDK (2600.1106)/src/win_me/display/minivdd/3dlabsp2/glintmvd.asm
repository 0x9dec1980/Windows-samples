;/******************************Module*Header**********************************\
; *
; *                           ***************************************
; *                           * Permedia 2: miniVDD SAMPLE CODE   *
; *                           ***************************************
; *
; * Module Name: glintmvd.asm
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

page            ,132
title		Glint Mini-VDD Support Functions
.386p
;
; General comments on VGA support
;
; This file contains the PERMEDIA specific VGA support and provides the additional functionality
; required to support windowed DOS 4 plane graphics mode windows.
;
; Usually this is implemented by reserving an off-screen portion of the framebuffer as a
; scratch pad for converting between 4 plane graphics and the HiRes packed pixel format of the
; visible screen.  Unfortunately, the PERMEDIA VGA core can only reach the first 512K of the
; framebuffer and only 64K when non 640 pixel per line modes are in effect (because banking is
; not active in these modes).  Therefore the VGA core cannot be guaranteed access to the
; off-screen area of all Hi-Res modes.  To solve this the VGA core is allocated the first 64Kbytes
; of the framebuffer and all Windows Hi-Res modes should start 64Kbytes beyond the framebuffer start.
;

.xlist
include 	    VMM.INC
include 	    MINIVDD.INC
include         CONFIGMG.INC            ; config manager
include         VPICD.INC               ; interrupts
include         VWIN32.INC              ; Win32 services
include         brdconf.inc
include 		glint.inc

.list
;
;
; The PerVMData structure.  This structure represents the reserved space in
; in the so-called CB data structure.  The CB data structure is created for
; each VM started in the system.  It can be used therefore to save states
; that need remembering on a per-VM basis.  The PERMEDIA requires the
; settings of two of its extension registers to be remembered.
;
PerVMData	struc
	VM_Mode640State	    db	?
	VM_VGAControlState  db  ?
	GCRReg6State	db	?		;needed for screwy BIOS check
	SeqReg2State	db	?		;needed for screwy BIOS check
PerVMData	ends
.errnz	size PerVMData MOD 4


;
subttl		Virtual Device Declaration
page +
Declare_Virtual_Device	GLINTMVD, 				\
			3,					\
			1,					\
			MiniVDD_Control,					\
            Undefined_Device_ID,               \
			VDD_Init_Order, 			\
			,					\
			,					\
			,
;
;-----------------------
subttl		Initialization Data
page +
VxD_IDATA_SEG
VxD_IDATA_ENDS
;
;-----------------------
subttl		Locked Data Area
page +
VxD_LOCKED_DATA_SEG
VxD_LOCKED_DATA_ENDS
;
;-----------------------
subttl		General Data Area
page +
VxD_DATA_SEG

public	C WindowsVMHandle
WindowsVMHandle 	    dd	?

public	PlanarStateID, MessageModeID
PlanarStateID		    dd	?
MessageModeID		    dd	?

public ForcedPlanarCBData, MessageModeCBData
ForcedPlanarCBData	    db	size PerVMData dup(0)
MessageModeCBData	    db	size PerVMData dup(0)

public	ReentryFlag
ReentryFlag		db	0		;FFH if virtualization reentered

public	OurCBDataPointer
OurCBDataPointer	    dd	?

HWInitialised           dd  0

public	C DisplayInfoStructure
DisplayInfoStructure	db	size DISPLAYINFO dup(0)

;

VxD_DATA_ENDS
;
;-----------------------
VxD_ICODE_SEG
;
;
subttl		MiniVDD Sys Critical Initialization
page +
public	MiniVDD_Sys_Critical_Init
BeginProc MiniVDD_Sys_Critical_Init
;
;NOTE:	This routine is ONLY called if we're being statically loaded from
;	SYSTEM.INI.  It isn't called (and doesn't need to be called) if
;	the Main VDD is dynamically loading us.
;
;
MVSI_Exit:
	clc				;return success (always)
	ret				;
EndProc MiniVDD_Sys_Critical_Init



;
;
subttl		MiniVDD Dynamic Initialization
page +
public	MiniVDD_Dynamic_Init
BeginProc MiniVDD_Dynamic_Init
;
;Entry:
;	EBX contains the VM handle of the Windows VM.
;Exit:
;	CY is set if MiniVDD initialization failed.
;		-- This means that the MiniVDD won't be loaded.
;		-- This also means that the guy who tried to load
;		   us (*VDD) will get back an error code.
;
;NOTE: MiniVDD_Dynamic_Init MUST be called only during the Device_Init
;      phase of system initialization.	We make a bunch of service calls
;      to the Main VDD (*VDD) that are located in its VxD_ICODE_Seg which
;      will be discarded after the Device_Init phase of system initialization.
;
	mov	WindowsVMHandle,ebx	        ; Save the Windows VM handle

;
; Allocate space in the per-VM CB data structure.  This will be used by the mini-VDD
; to store per-VM data relating to states of registers.
;
	VMMCall _Allocate_Device_CB_Area,<<size PerVMData>,0>
	mov	    OurCBDataPointer,eax	; Save offset of our VxD's area
	or	    eax,eax 		        ; Successful?
	jz	    MVDI_ErrorExit		    ; No: Something is wrong... don't load MiniVDD.
                                    ; Yes

; Get the display config from the main VDD.
; this gives us a handle to the device node for whatever chip the driver
; has been associated with

    lea     eax, DisplayInfoStructure
    mov     ecx,DISPLAYINFO_SIZE
    VxDCall VDD_Get_DISPLAYINFO

    mov     eax, DWORD PTR DisplayInfoStructure.diInfoFlags
    test    eax, DEVICE_IS_NOT_VGA
    jne     MVDI_ExitOK

; Store the DevNode of the chip we are running on. (for Registry reads/writes)
    mov     eax, DisplayInfoStructure.diDevNodeHandle
    invoke  InitialiseDynamicInit, eax
    xor     ebx, ebx
    mov     HWInitialised, ebx
;
; Hook the INT10h interrupt so that we can intercept VESA VBE/DDC monitor recognition
; requests.  The code has to catch this call early because Windows attempts to
; recognize the monitor at bootup before it has setup the vesaSupport routine.
;
    mov     eax,10h                             ; Int number to be hooked.
    mov     esi,OFFSET32 MiniVDD_Int10Catcher   ; Routine to hook up.
    VMMCall Hook_V86_Int_chain                  ; Hook it. (CF set if hook failed)

;
public	MVDI_GetSpecialVMs
MVDI_GetSpecialVMs:
;
; There are two special VM states that the "main" VDD establishes.  The
; first is the planar state which is simply a state that the "main" VDD
; restores to establish the VGA 4 plane mode.  When we restore the states
; at MiniVDD_RestoreRegisters, we check for the special identifier which
; flags that we're restoring this state.  If we find that we are restoring
; the planar state, we have the opportunity to special case the register
; state restore.
;
; Similarly, there is a special state called "Message Mode".  This is the
; state when the Shell VxD is displaying a full-screen message (usually
; with a blue background) telling the user of a serious anomaly in the
; Windows running state.
;
	VxDCall VDD_Get_Special_VM_IDs	; go get special VM information
	mov	PlanarStateID,esi	        ; save returned identifiers
	mov	MessageModeID,edi
;

MVDI_ExitOK:
	clc				                ; Return success
	jmp	short @f

MVDI_ErrorExit:
	stc                             ; Return failure
@@:
	ret

EndProc MiniVDD_Dynamic_Init

VxD_ICODE_ENDS


;-----------------------
;
VxD_LOCKED_CODE_SEG
;
;

Begin_Control_Dispatch	MiniVDD
	Control_Dispatch Sys_Critical_Init   MiniVDD_Sys_Critical_Init
	Control_Dispatch Device_Init,	     MiniVDD_Dynamic_Init
	Control_Dispatch Sys_Dynamic_Device_Init, MiniVDD_Dynamic_Init
	Control_Dispatch Sys_VM_Terminate,   MiniVDD_Sys_VM_Terminate
	Control_Dispatch W32_DeviceIoControl, VXD3DL_DeviceIOControl, sCall, <ecx, ebx, edx, esi>
	Control_Dispatch PNP_NEW_DEVNODE,    MiniVDD_PNP_NEW_DEVNODE
End_Control_Dispatch MiniVDD


;------------------------------------------------------------------------------
public MiniVDD_Int10Catcher
BeginProc MiniVDD_Int10Catcher, DOSVM
;
; On Entry: eax = int number (should be 10h)
;           ebx = current Virtual Machine handle
;           ebp = pointer to client register structure
;
; On Exit:  CF cleared if routine handled the int10h request.
;

    pushad
    cmp     eax,10h                 ; Handle an Int 10h?
    jne     I10_PassIt              ; No
                                    ; Yes
    mov     eax, [ebp].Client_EAX
    cmp     ax,4F15h                ; Is it a VESA VBE/DDC call?
    je      I10_HandleIt            ; Yes
    cmp     ax,4F10h                ; Is it a VESA DPMS call?
    je      I10_HandleIt            ; Yes
                                    ; No
I10_PassIt:
    popad
    stc                             ; Signal request not handled
    ret

I10_HandleIt:                       ; Handle the request here.

    invoke FindDevNodeForCurrentlyActiveVGA

ifdef POWER_STATE_MANAGE
;   add test for zero return (some other card is the VGA)
    test    eax, eax
    je      I10_PassIt
endif

    invoke VesaSupport, eax, ebp
    test    eax, eax
    jnz     @f

    popad
    stc
    ret

@@:
    popad
    clc
    ret

EndProc MiniVDD_Int10Catcher


subttl		Collect States From Sequencer OUT's
page +
public	MiniVDD_VirtualizeSequencerOut
BeginProc MiniVDD_VirtualizeSequencerOut, DOSVM
;
;Entry:
;	AL contains the value to output to Sequencer.
;	EBX contains the VM handle of the VM doing the OUT.
;	ECX contains the index to output.
;	EDX --> Sequencer Data Register.
;Exit:
;	If it isn't one of ours, return NC.
;	If it is one of ours, return CY.
;
;There seems to be some problem on Permedia when switching between mode 12 and
;mode 3 whilst in a window. For some reason we get 06 in Sequencer Register 4
;rather than 02 in text modes. I found this code in one of the reference drivers
;which fixes the problem - although it seems like their reason is different!

;The 3Dlabs ROM BIOS does screwy things to Sequencer Register 4 during
;Font changes.  It sets it to an 06H which is "planar" state instead of 02H
;which is "text mode" state.  We must therefore attempt to fix this in the
;Main VDD's saved state so that it never restores an 06H during a CRTC mode
;change.
;
	cmp	ReentryFlag,0		        ;are we being reentered?
	jne	MVSONotOneofOurs	        ;nope, continue
	mov	ReentryFlag,0ffh	        ;prevent ourselves from being reentered
	cmp	cl,04h			            ;is it Sequencer Register 04H?
	jne	MVSOVirtualExtensions       ;nope, continue
	cmp	ebx,WindowsVMHandle	        ;is VM doing the OUT the Windows VM?
	je	MVSOVirtualExtensions       ;yes, we're not interested
	cmp	al,06h			            ;is he trying to get screwy with us?
	jne	MVSOVirtualExtensions       ;nope, just let it go through
;
;We are trying to set Sequencer Register 04H to 06H.  We must do some more
;investigation to see if we're in text mode or graphics mode.  If we're in
;text mode (indicated by GCR Register 06H, bit 0 being 0), we must do the
;fixup.
;
	push	ebx			            ;save this register
	add	ebx,OurCBDataPointer	    ;make EBX --> CB data area
	mov	ah,[ebx].GCRReg6State	    ;get state of GCR register 6
	mov	ch,[ebx].SeqReg2State	    ;get state of Sequencer register 2
	pop	ebx			                ;(restore EBX --> VM handle)
	cmp	ch,03h			            ;in a text mode (planes 0 & 1 mapped)?
	mov	ch,0			            ;(clear this without disturbing flags)
	jne	MVSOVirtualExtensions       ;
	test	ah,01h			        ;in graphics mode?
	jnz	MVSOVirtualExtensions       ;in graphics mode, nothing to fix

;
public	MVSOFixScrewyBIOS
MVSOFixScrewyBIOS:
;
;OK, we're setting Sequencer Register 04H to an 06H and we're in text mode.
;This means that the BIOS is doing the screwy scrolling trick.	We must
;therefore simulate I/O to the Sequencer Register from this VM
;
	push	eax			
	push	ebx			
	push	ecx			
	push	edx			
	mov	eax,0204h		
	mov	ecx,WORD_OUTPUT 
	dec	dl			                ;EDX --> Sequencer Index Register
	VMMCall Simulate_VM_IO
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	VxDCall VDD_Get_VM_Info 	    ;get CRTC owner VM handle in EDI
	cmp	ebx,edi 		            ;is CRTC controlled by caller?
	je	MVSOPhysicalOUT 	        ;yes, go do physical OUT
	jmp	MVSOWeHandledIt 	        ;nope, don't do physical OUT!
;
public	MVSOVirtualExtensions  
MVSOVirtualExtensions:
	mov	edi,ebx 		            ;get our CB data pointer in EDX
	add	edi,OurCBDataPointer	    ;
	cmp	cl,02h			            ;do we need to collect register 2 state?
	jne	@F			                ;nope, continue
	mov	[edi].SeqReg2State,al	    ;save the state
	jmp	MVSONotOneOfOurs	        ;and send it back to the main VDD
@@:	cmp	cl,5h			            ;is it one of ours?
	jne	MVSONotOneofOurs	        ;nope, let caller handle it
	mov	[edi].VM_VGAControlState,al ;save the value virtually
	VxDCall VDD_Get_VM_Info 	    ;get CRTC owner VM handle in EDI
	cmp	ebx,edi 		            ;is CRTC controlled by caller?
	jne	MVSOWeHandledIt 	        ;nope, we just virtualized it

; We only want to update this register if it is not being virtualised when we are in HiRes mode
    push    eax                     ; Preserve registers across call
    push    ecx
    push    edx
    push    ebp
    invoke  FindPrimarypDev
	mov 	esi, eax
    pop     ebp                     ; Restore preserved regs
    pop     edx
    pop     ecx
    pop     eax

    test    esi, esi
    je      MVSOPhysicalOUT
    mov     edi,[esi].pGLInfo       ; get pointer to shared area
    test    edi, edi
    je      MVSOPhysicalOUT

    cmp     [edi].dwHiResMode, 0    ; Dont write to hardware if we are 
    jne     MVSOWeHandledIt         ; in Hi res mode              

;
public	MVSOPhysicalOUT
MVSOPhysicalOUT:
	dec	dl			                ;EDX --> Sequencer Index Register
	mov	ch,al					    ;get value for data register in CH
	in	al,dx					    ;get current value of index register
	ror	eax,8					    ;save it in high byte of EAX
	mov	ax,cx					    ;get index in AL, data in AH
	out	dx,ax					    ;set index and data to hardware
	rol	eax,8					    ;get saved index back in AL
	out	dx,al					    ;and restore it to the hardware
;
public	MVSOWeHandledIt
MVSOWeHandledIt:
	stc				                ;indicate that we handled it
	jmp	MVSOExit		            ;and go return the value
;
MVSONotOneofOurs:
	clc				                ;clear CY so caller will handle it
;
MVSOExit:
	mov	ReentryFlag,0		        ;free the semaphore
	ret				;
EndProc MiniVDD_VirtualizeSequencerOut
;
;
subttl		Return States Requested By Sequencer IN's
page +
public	MiniVDD_VirtualizeSequencerIn
BeginProc MiniVDD_VirtualizeSequencerIn, DOSVM
;
;Entry:
;	EBX contains the VM handle of the VM doing the IN.
;	ECX contains the index to read.
;	EDX --> Sequencer Data Register.
;Exit:
;	AL contains the value to return to the caller.
;	If it isn't one of ours, preserve the flags.
;	If it is one of ours, return the value and set CY flag.
;
	pushf				            ;save the flags over this
	cmp	cl,5h			            ;is it a VGA extension register?
	jne	MVSINotOneofOurs	        ;nope, leave
;
public	MVSIItsOneofOurs
MVSIItsOneofOurs:
	popf				            ;fix the Stack
	VxDCall VDD_Get_VM_Info 	    ;get CRTC owner VM handle in EDI
	cmp	ebx,edi 		            ;is CRTC controlled by caller?
	je	MVSIPhysicalIN		        ;yes, go do the physical IN
;
public	MVSIVirtualIN
MVSIVirtualIN:
	add	ebx,OurCBDataPointer	    ;now ECX --> place to retrieve from
	mov	al,[ebx].VM_VGAControlState	;return the value from this register
	stc				                ;indicate that we handled it
	jmp	MVSIExit		            ;and go return the value
;
public	MVSIPhysicalIN
MVSIPhysicalIN:
	dec	dl			                ;EDX --> Sequencer Index Register
	in	al,dx			            ;get current value of this register
	mov	ah,al			            ;save it in AH
	mov	al,cl			            ;get index to read
	out	dx,al			            ;set it to hardware
	inc	dl			                ;EDX --> Sequencer Data Register
	in	al,dx			            ;get the data into AL
	dec	dl			                ;EDX --> Sequencer Index Register
	xchg	al,ah			        ;save data just read in AH
					                ;get saved index in AL
	out	dx,al			            ;restore saved value of index register
	mov	al,ah			            ;and return physical data in AL
	stc				                ;indicate that we handled it
	jmp	MVSIExit		            ;and go return the value
;
MVSINotOneofOurs:
	popf				            ;restore flags so caller will handle it
;
MVSIExit:
	ret				;
EndProc MiniVDD_VirtualizeSequencerIn
;
;
subttl		Virtualize GCR OUT's
page +
public	MiniVDD_VirtualizeGCROut
BeginProc MiniVDD_VirtualizeGCROut, DOSVM
;Entry:
;	AL contains the value to output to GCR.
;	EBX contains the VM handle of the VM doing the OUT.
;	ECX contains the index to output.
;	EDX --> GCR Data Register.
;Exit:
;	If it isn't one of ours, return NC.
;	If it is one of ours, return CY.
;
;Function:
;   We need to keep track of GCR 6 to fix BIOS and virtualize GCR 9

	cmp	ReentryFlag,0		        ;are we being reentered?
	jne	MVGONotOneofOurs	        ;nope, continue
	mov	ReentryFlag,0ffh	        ;prevent ourselves from being reentered

public	MVGOVirtualExtensions
MVGOVirtualExtensions:
	mov	edi,ebx 		            ;get our CB data pointer in EDX
	add	edi,OurCBDataPointer	    ;
	cmp	cl,06h			            ;do we need to collect register 6 state?
	jne	@F			                ;nope, continue
	mov	[edi].GCRReg6State,al	    ;save the state
	jmp	MVGONotOneOfOurs	        ;and send it back to the main VDD
@@:	cmp	cl,09h			            ;is it one of ours?
	jne	MVGONotOneofOurs	        ;nope, let caller handle it
	mov	[edi].VM_Mode640State,al    ;save the value virtually
	VxDCall VDD_Get_VM_Info 	    ;get CRTC owner VM handle in EDI
	cmp	ebx,edi 		            ;is CRTC controlled by caller?
	jne	MVGOWeHandledIt 	        ;nope, we just virtualized it
;
public	MVGOPhysicalOUT
MVGOPhysicalOUT:
	dec	dl			                ;EDX --> Sequencer Index Register
	mov	ch,al					    ;get value for data register in CH
	in	al,dx					    ;get current value of index register
	ror	eax,8					    ;save it in high byte of EAX
	mov	ax,cx					    ;get index in AL, data in AH
	out	dx,ax					    ;set index and data to hardware
	rol	eax,8					    ;get saved index back in AL
	out	dx,al					    ;and restore it to the hardware
;
public	MVGOWeHandledIt
MVGOWeHandledIt:
	stc				                ;indicate that we handled it
	jmp	MVGOExit		            ;and go return the value
;
MVGONotOneofOurs:
	clc				                ;clear CY so caller will handle it
;
MVGOExit:
	mov	ReentryFlag,0		        ;free the semaphore
	ret				;
EndProc MiniVDD_VirtualizeGCROut


subttl		Return States Requested By GCR IN's
page +
public	MiniVDD_VirtualizeGCRIn
BeginProc MiniVDD_VirtualizeGCRIn, DOSVM
;
;Entry:
;	EBX contains the VM handle of the VM doing the IN.
;	ECX contains the index to read.
;	EDX --> GCR Data Register.
;Exit:
;	AL contains the value to return to the caller.
;	If it isn't one of ours, preserve the flags.
;	If it is one of ours, return the value and set CY flag.
;
	pushf				            ;save the flags over this
	cmp	cl,9h			            ;is it a VGA extension register?
	jne	MVGINotOneofOurs	        ;nope, leave
;
public	MVGIItsOneofOurs
MVGIItsOneofOurs:
	popf				            ;fix the Stack
	VxDCall VDD_Get_VM_Info 	    ;get CRTC owner VM handle in EDI
	cmp	ebx,edi 		            ;is CRTC controlled by caller?
	je	MVGIPhysicalIN		        ;yes, go do the physical IN
;
public	MVGIVirtualIN
MVGIVirtualIN:
	add	ebx,OurCBDataPointer	    ;now ECX --> place to retrieve from
	mov	al,[ebx].VM_Mode640State    ;return the value from this register
	stc				                ;indicate that we handled it
	jmp	MVGIExit		            ;and go return the value
;
public	MVGIPhysicalIN
MVGIPhysicalIN:
	dec	dl			                ;EDX --> Sequencer Index Register
	in	al,dx			            ;get current value of this register
	mov	ah,al			            ;save it in AH
	mov	al,cl			            ;get index to read
	out	dx,al			            ;set it to hardware
	inc	dl			                ;EDX --> Sequencer Data Register
	in	al,dx			            ;get the data into AL
	dec	dl			                ;EDX --> Sequencer Index Register
	xchg	al,ah			        ;save data just read in AH
					                ;get saved index in AL
	out	dx,al			            ;restore saved value of index register
	mov	al,ah			            ;and return physical data in AL
	stc				                ;indicate that we handled it
	jmp	MVGIExit		            ;and go return the value
;
MVGINotOneofOurs:
	popf				            ;restore flags so caller will handle it
;
MVGIExit:
	ret				;
EndProc MiniVDD_VirtualizeGCRIn


if 0
subttl		Virtualize DAC OUT's
page +
public	MiniVDD_VirtualizeDACOut
BeginProc MiniVDD_VirtualizeDACOut, DOSVM
;Entry:
;	AL contains the value to output to DAC.
;	EBX contains the VM handle of the VM doing the OUT.
;	ECX contains the index to output.
;	EDX --> DAC Data Register.
;Exit:
;	If it isn't one of ours, return NC.
;	If it is one of ours, return CY.
;
;Function:

	cmp	ReentryFlag,0		        ;are we being reentered?
	jne	MVGONotOneofOurs	        ;nope, continue
	mov	ReentryFlag,0ffh	        ;prevent ourselves from being reentered
if 0

public	MVDOVirtualExtensions
MVDOVirtualExtensions:
	mov	edi,ebx 		            ;get our CB data pointer in EDX
	add	edi,OurCBDataPointer	    ;
	cmp	cl,06h			            ;do we need to collect register 6 state?
	jne	@F			                ;nope, continue
	mov	[edi].GCRReg6State,al	    ;save the state
	jmp	MVDONotOneOfOurs	        ;and send it back to the main VDD
@@:	cmp	cl,09h			            ;is it one of ours?
	jne	MVDONotOneofOurs	        ;nope, let caller handle it
	mov	[edi].VM_Mode640State,al    ;save the value virtually
	VxDCall VDD_Get_VM_Info 	    ;get CRTC owner VM handle in EDI
	cmp	ebx,edi 		            ;is CRTC controlled by caller?
	jne	MVDOWeHandledIt 	        ;nope, we just virtualized it
;
public	MVDOPhysicalOUT
MVDOPhysicalOUT:
	dec	dl			                ;EDX --> Sequencer Index Register
	mov	ch,al					    ;get value for data register in CH
	in	al,dx					    ;get current value of index register
	ror	eax,8					    ;save it in high byte of EAX
	mov	ax,cx					    ;get index in AL, data in AH
	out	dx,ax					    ;set index and data to hardware
	rol	eax,8					    ;get saved index back in AL
	out	dx,al					    ;and restore it to the hardware
;
public	MVDOWeHandledIt
MVDOWeHandledIt:
	stc				                ;indicate that we handled it
	jmp	MVDOExit		            ;and go return the value
;
endif
MVDONotOneofOurs:
	clc				                ;clear CY so caller will handle it
;
MVDOExit:
	mov	ReentryFlag,0		        ;free the semaphore
	ret				;
EndProc MiniVDD_VirtualizeDACOut


subttl		Return States Requested By GCR IN's
page +
public	MiniVDD_VirtualizeDACIn
BeginProc MiniVDD_VirtualizeDACIn, DOSVM
;
;Entry:
;	EBX contains the VM handle of the VM doing the IN.
;	ECX contains the index to read.
;	EDX --> GCR Data Register.
;Exit:
;	AL contains the value to return to the caller.
;	If it isn't one of ours, preserve the flags.
;	If it is one of ours, return the value and set CY flag.
;
	pushf				            ;save the flags over this
	cmp	ecx,256			            ;is it a DAC register?
	jge	MVDINotOneofOurs	        ;nope, leave
if 0
;
public	MVDIItsOneofOurs
MVDIItsOneofOurs:
	popf				            ;fix the Stack
;
public	MVDIVirtualIN
MVDIVirtualIN:
	add	ebx,OurCBDataPointer	    ;now ECX --> place to retrieve from
	mov	al,[ebx].VM_Mode640State    ;return the value from this register
	stc				                ;indicate that we handled it
	jmp	MVDIExit		            ;and go return the value
;
public	MVDIPhysicalIN
MVDIPhysicalIN:
	dec	dl			                ;EDX --> Sequencer Index Register
	in	al,dx			            ;get current value of this register
	mov	ah,al			            ;save it in AH
	mov	al,cl			            ;get index to read
	out	dx,al			            ;set it to hardware
	inc	dl			                ;EDX --> Sequencer Data Register
	in	al,dx			            ;get the data into AL
	dec	dl			                ;EDX --> Sequencer Index Register
	xchg	al,ah			        ;save data just read in AH
					                ;get saved index in AL
	out	dx,al			            ;restore saved value of index register
	mov	al,ah			            ;and return physical data in AL
	stc				                ;indicate that we handled it
	jmp	MVDIExit		            ;and go return the value
;
endif
MVDINotOneofOurs:
	popf				            ;restore flags so caller will handle it
;
MVDIExit:
	ret				;
EndProc MiniVDD_VirtualizeDACIn
endif

subttl		PNP_NEW_DEVNODE handler
page +
public	MiniVDD_PNP_NEW_DEVNODE
BeginProc MiniVDD_PNP_NEW_DEVNODE, RARE

    push    ebx
    push    ebp
    push    edi
    push    esi

    invoke  PNP_NewDevnode, eax, ebx, edx

    ; We dont really care if this doesnt work. Always return success
MVPNP_ReturnSuccess:
    mov     eax, CR_SUCCESS
    clc
    jmp     @f
MVPNP_ReturnFailure:
    mov     eax, CR_INVALID_DEVNODE
    stc
@@:
    pop     esi
    pop     edi
    pop     ebp
    pop     ebx

    ret                                     ; return to VWIN32
EndProc MiniVDD_PNP_NEW_DEVNODE



subttl		InitialisePrimaryDispatchTable
page +
public	_InitialisePrimaryDispatchTable
BeginProc _InitialisePrimaryDispatchTable, RARE

    pushad

    lea     eax, DisplayInfoStructure
    mov     ecx,DISPLAYINFO_SIZE
    VxDCall VDD_Get_DISPLAYINFO

;
;The "master" VDD (VDD.386) contains all of the routines and procedures
;that are necessary to virtualize a standard VGA environment on high
;resolution super-VGA cards.  This "mini-VDD" provides some simple
;services which provide support which are peculiar to the chipset
;that we're providing support for.
;
;We must register the entry point addresses of our chipset-dependent
;routines with the "master" VDD.  To do this, we call a service in the
;"master" VDD, which returns the address of a dispatch table of the
;which is an array of the format:
;
;		dd	Address of support routine in this mini-VDD.
;			This address will be initialized to 0 by the "master"
;			VDD.  If we need to implement this functionality,
;			we fill in the address of our routine.
;
;Then, when the "master" VDD needs to call this function, it'll see if we've
;filled in the address field in for the function's entry and will call our
;routine if the address is filled in.  Otherwise, the VDD will skip the call
;and continue on its way without calling us.
;
;The following function calls the "master" VDD and returns a pointer to the
;dispatch table documented above in EDI and the number of functions
;supported in ECX. If the number of functions returned by the "master" VDD
;isn't equal to what we think it is, we return an error and don't allow
;Windows to continue running.
;
	VxDCall VDD_Get_Mini_Dispatch_Table
	cmp	    ecx,NBR_MINI_VDD_FUNCTIONS	;perform a cursory version check
	jb	    IPDT_ErrorExit			    ;oops, versions don't match!
;
;Fill in the addresses of all the functions that we need to handle in this
;mini-VDD in the table provided by the "master" VDD whose address is
;contained in EDI.  Note that if you do not need to support a function,
;you do not need to fill in the dispatch table entry for that function.
;If you do not fill in an entry, the "master" VDD won't try to call
;this mini-VDD to support the function.  It'll just handle it in a
;default manner.
;

ifdef POWER_STATE_MANAGE
    cmp     ecx, NBR_MINI_VDD_FUNCTIONS_41
    jb      @f
	MiniVDDDispatch SET_ADAPTER_POWER_STATE     , SetAdapterPowerState
	MiniVDDDispatch GET_ADAPTER_POWER_STATE_CAPS, GetAdapterPowerStateCaps

	MiniVDDDispatch TURN_VGA_OFF                , TurnVGAOff
	MiniVDDDispatch TURN_VGA_ON                 , TurnVGAOn
@@:
endif

    MiniVDDDispatch GET_VDD_BANK                , GetVDDBank
    MiniVDDDispatch SET_VDD_BANK                , SetVDDBank
    MiniVDDDispatch RESET_BANK                  , ResetBank
	MiniVDDDispatch SET_LATCH_BANK              , SetLatchBank
    MiniVDDDispatch SAVE_REGISTERS              , SaveRegisters
	MiniVDDDispatch RESTORE_REGISTERS           , RestoreRegisters
    MiniVDDDispatch ACCESS_VGA_MEMORY_MODE      , AccessVGAMemoryMode
    MiniVDDDispatch SAVE_FORCED_PLANAR_STATE    , SaveForcedPlanarState
    MiniVDDDispatch SAVE_MESSAGE_MODE_STATE     , SaveMsgModeState
    MiniVDDDispatch PRE_CRTC_MODE_CHANGE        , PreCRTCModeChange
    MiniVDDDispatch POST_CRTC_MODE_CHANGE       , PostCRTCModeChange
    MiniVDDDispatch GET_TOTAL_VRAM_SIZE         , GetTotalVRAMSize
    MiniVDDDispatch GET_CURRENT_BANK_WRITE      , GetCurrentBankWrite
    MiniVDDDispatch GET_CURRENT_BANK_READ       , GetCurrentBankRead
	MiniVDDDispatch GET_BANK_SIZE               , GetBankSize
;    MiniVDDDispatch SET_BANK                    , SetBank
;    MiniVDDDispatch SAVE_LATCHES                , SaveLatches
;    MiniVDDDispatch RESTORE_LATCHES             , RestoreLatches
;    MiniVDDDispatch ACCESS_LINEAR_MEMORY_MODE   , AccessLinearMemoryMode

	MiniVDDDispatch REGISTER_DISPLAY_DRIVER     , RegisterDisplayDriver
	MiniVDDDispatch RESET_LATCH_BANK            , ResetLatchBank
	MiniVDDDispatch PRE_HIRES_TO_VGA            , PreHiResToVGA
	MiniVDDDispatch POST_HIRES_TO_VGA           , PostHiResToVGA
	MiniVDDDispatch PRE_VGA_TO_HIRES            , PreVGAToHiRes
	MiniVDDDispatch POST_VGA_TO_HIRES           , PostVGAToHiRes
	MiniVDDDispatch ENABLE_TRAPS                , EnableTraps
	MiniVDDDispatch DISABLE_TRAPS               , DisableTraps
	MiniVDDDispatch MAKE_HARDWARE_NOT_BUSY      , MakeHardwareNotBusy
;;	MiniVDDDispatch DISPLAY_DRIVER_DISABLING    , DisplayDriverDisabling ;;
    MiniVDDDispatch VESA_SUPPORT                , VesaSupport
    MiniVDDDispatch VESA_CALL_POST_PROCESSING   , VesaPostProcessing
	MiniVDDDispatch VIRTUALIZE_SEQUENCER_OUT    , VirtualizeSequencerOut
	MiniVDDDispatch VIRTUALIZE_GCR_OUT          , VirtualizeGCROut
	MiniVDDDispatch VIRTUALIZE_SEQUENCER_IN     , VirtualizeSequencerIn
	MiniVDDDispatch VIRTUALIZE_GCR_IN           , VirtualizeGCRIn
if 0
	MiniVDDDispatch VIRTUALIZE_DAC_OUT          , VirtualizeDACOut
	MiniVDDDispatch VIRTUALIZE_DAC_IN           , VirtualizeDACIn
endif

    popad
    mov     eax, 1
    ret

IPDT_ErrorExit:
    popad
    xor     eax, eax
    ret

EndProc _InitialisePrimaryDispatchTable


subttl		InitialiseSecondaryDispatchTable
page +
public	_InitialiseSecondaryDispatchTable
BeginProc _InitialiseSecondaryDispatchTable, RARE

    pushad

    lea     eax, DisplayInfoStructure
    mov     ecx,DISPLAYINFO_SIZE
    VxDCall VDD_Get_DISPLAYINFO

	VxDCall VDD_Get_Mini_Dispatch_Table
	cmp     ecx,NBR_MINI_VDD_FUNCTIONS	;perform a cursory version check
	jb	    ISDT_ErrorExit			;oops, versions don't match!

;;	MiniVDDDispatch DISPLAY_DRIVER_DISABLING    , DisplayDriverDisabling ;;

    cmp     ecx, NBR_MINI_VDD_FUNCTIONS_41
    jb      @f

;;
	MiniVDDDispatch SET_ADAPTER_POWER_STATE     , SetAdapterPowerState
	MiniVDDDispatch GET_ADAPTER_POWER_STATE_CAPS, GetAdapterPowerStateCaps

;;
	MiniVDDDispatch TURN_VGA_OFF                , TurnVGAOff
	MiniVDDDispatch TURN_VGA_ON                 , TurnVGAOn
@@:

	MiniVDDDispatch REGISTER_DISPLAY_DRIVER     , RegisterDisplayDriver

ISDT_ExitOK:
    popad
    mov     eax, 1
    ret
ISDT_ErrorExit:
    popad
    xor     eax, eax
    ret
EndProc _InitialiseSecondaryDispatchTable


;
; Function to check if 4-Plane VGA virtualization should take place.
;
; On Exit:  CY = 1 if 4 Plane VGA Virtualization is enabled.
;              = 0 if not.
Is4PlaneOK proc

    pushad
    invoke  FindPrimarypDev
    test    eax, eax
    jz      @f

	mov 	esi, eax
	mov		esi, [esi].pGLInfo              ; Get pointer to shared area.
	test	esi, esi						; Is pGLInfo valid yet?
	je		@f      						; no
                                            ; yes
    assume  esi:ptr _GlintInfo
    test    [esi].dwFlags, GMVF_VIRTUALISE4PLANEVGA     ; Should the MiniVDD support 4-plane virtualization?
    assume  esi:nothing
    jz      @f                              ; no
                                            ; Yes
    stc         ; Return that it IS OK to do 4 plane virtualization.
    popad
    ret

@@: clc         ; Signal NO 4 plane virtualization
    popad
    ret

endproc Is4PlaneOK



ifndef DDK_DX5_BETA1
;
;
; Functions to handle Kernel mode DirectDraw calls.
;

subttl		MiniVDD Get IRQ Info
page +
public	_MiniVDD_GetIRQInfo
BeginProc _MiniVDD_GetIRQInfo
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddGetIRQInfo, eax, edi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_GetIRQInfo

subttl		MiniVDD Flip Overlay
page +
public	_MiniVDD_FlipOverlay
BeginProc _MiniVDD_FlipOverlay
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddFlipOverlay, eax, esi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_FlipOverlay

subttl		MiniVDD Lock
page +
public	_MiniVDD_Lock
BeginProc _MiniVDD_Lock
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddLock, eax, esi, edi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_Lock

subttl		MiniVDD SetState
page +
public	_MiniVDD_SetState
BeginProc _MiniVDD_SetState
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddSetState, eax, esi, edi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_SetState

subttl		MiniVDD Get Field Polarity
page +
public	_MiniVDD_GetFieldPolarity
BeginProc _MiniVDD_GetFieldPolarity
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddGetFieldPolarity, eax, esi, edi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_GetFieldPolarity

subttl		MiniVDD Skip Next field
page +
public	_MiniVDD_SkipNextField
BeginProc _MiniVDD_SkipNextField
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddSkipNextField, eax, esi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_SkipNextField

subttl		MiniVDD Is our IRQ
page +
public	_MiniVDD_IsOurIRQ
BeginProc _MiniVDD_IsOurIRQ
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddIsOurIRQ, eax

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_IsOurIRQ

subttl		MiniVDD Enable IRQ
page +
public	_MiniVDD_EnableIRQ
BeginProc _MiniVDD_EnableIRQ
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddEnableIRQ, eax, esi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_EnableIRQ

subttl		MiniVDD Flip Video Port
page +
public	_MiniVDD_FlipVideoPort
BeginProc _MiniVDD_FlipVideoPort
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddFlipVideoPort, eax, esi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_FlipVideoPort

subttl		MiniVDD Get Current AutoFlip
page +
public	_MiniVDD_GetCurrentAutoflip
BeginProc _MiniVDD_GetCurrentAutoflip
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddGetCurrentAutoflip, eax, esi, edi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_GetCurrentAutoflip

subttl		MiniVDD Get Previous AutoFlip
page +
public	_MiniVDD_GetPreviousAutoflip
BeginProc _MiniVDD_GetPreviousAutoflip
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	invoke vddGetPreviousAutoflip, eax, esi, edi

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _MiniVDD_GetPreviousAutoflip
endif ; DDDK_DX5_BETA1

subttl		Allocate Selector
page +
public	_AllocateSelector
BeginProc _AllocateSelector, RARE
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	mov		eax, [ebp + 8]				; Get Virtual Address
	mov		ecx, [ebp + 12]				; Get number of pages
    mov     edx,10010011b               ; set present, DPL == undefined
									    ; app selector, Data
	mov	ebx,10010000b					; set GRAN and AVL bit on
	VMMCall _BuildDescriptorDWORDs,<eax,ecx,edx,ebx,0>
	VMMCall _Allocate_LDT_Selector,<WindowsVMHandle,edx,eax,1,0>

	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	pop		ebp
    ret
EndProc _AllocateSelector

public	_FreeSelector
BeginProc _FreeSelector, RARE
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	mov		eax, [ebp + 8]				; Get Selector
	VMMCall _Free_LDT_Selector,<WindowsVMHandle,eax,0>
	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _FreeSelector

public	_SelectorMapFlat
BeginProc _SelectorMapFlat, RARE
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	mov		eax, [ebp + 8]				; Get Selector
	VMMCall _SelectorMapFlat,<WindowsVMHandle,eax,0>

	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop		ebp
    ret
EndProc _SelectorMapFlat

if 0
public	_GetDisplayInfoStructure
BeginProc _GetDisplayInfoStructure, RARE
	push	ebp
	mov		ebp, esp
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

    mov     eax,OFFSET32 DisplayInfoStructure
    mov     ecx,SIZE DISPLAYINFO
    VxDCall VDD_Get_DISPLAYINFO

	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	pop		ebp
    ret
EndProc _GetDisplayInfoStructure
endif

subttl		Register Display Driver Dependent Data
page +
public	MiniVDD_RegisterDisplayDriver
BeginProc MiniVDD_RegisterDisplayDriver, RARE
;
;	The only client registers that are reserved (and cannot be used
;to pass data in) are EAX (which contains the function code which the
;display driver uses to call the "main" VDD) and EBX (which contains the
;Windows VM handle).  When we get called by the "main" VDD, we'll be
;passed a flag in Client_AL which will be set to non-zero if the VFLATD
;VxD is being used for banked support or zero if VFLATD is NOT involved in
;this session.
;
;Entry:
;	EBP --> the Client Register Structure (documented in VMM.INC)
;       CRS.DS:SI->GlintInfo structure

    invoke  RegisterDisplayDriver, ebp
    ret

EndProc MiniVDD_RegisterDisplayDriver

;-----------------------
subttl		Calculate and Save Banking Register Values
page +
public	MiniVDD_GetVDDBank
BeginProc MiniVDD_GetVDDBank
;
; In order to virtualize VGA graphics mode applications in a window on the
; Windows desktop, the VDD requires the Mini-VDD to reserve at least 32K of
; off-screen video memory for the exclusive use of the VDD.  Although it's best
; to allocate a full 64K of off-screen video memory, the VDD can work with a
; minimum of 32K.  This function is called by the "main" VDD when the
; display driver registers with it.  This function must calculate the
; correct bank of the off-screen memory and the offset within that bank of
; the VDD reserved memory.  Note that it's best if the offset value returned
; is zero (that is, you start on a bank boundary).
;
; Note: The "main" VDD assumes that the hardware banks are 64K in length.
;
; Entry:
;	EBX contains the Windows VM handle (which must be preserved).
;	ECX contains the byte offset of where in video memory the VDD
;	    memory is to be placed (this will be the first byte of
;	    off-screen memory).
; Exit:
;	AH  = Page offset from start of bank of virtualization area.
;	EBX = preserved.
;	EDX = size of VDD virtualization area.
;
;   EDX == ECX if VGA 4 plane graphics cannot be supported in the background.
;
; The PERMEDIA VGA core cannot hope to reserve an area of off-screen Hi-Res framebuffer
; memory beyond the visible portion of the framebuffer because it simply cannot address
; that high.  Therefore the lower 64K of framebuffer memory is used for VGA virtualization
; exclusively and the Hi-Res PERMEDIA modes must skip over this bit of the framebuffer.
; As a result VGA virtualization ALWAYS uses bank 0 and starts at offset 0.
;
    call    Is4PlaneOK          ; Is it OK to do 4-Plane virtualisation?
    jc      @f                  ; Yes
                                ; No
    mov     edx,ecx             ; Indicate 4 plane VGA not supported.
    jmp     short MV_GVBEnd

@@: mov     edx,32*1024         ; VDD virtualization area = 32K
    xor     ah,ah               ; Start from offset 0

MV_GVBEnd:
	ret

EndProc MiniVDD_GetVDDBank


;-----------------------
subttl		Banking Handling Code for VGA Virtualization
page +
public	MiniVDD_SetVDDBank
BeginProc MiniVDD_SetVDDBank
;
; This routine is called when we need to virtualize something in the
; off-screen region of the video memory.  The current state of the
; banking registers are saved and then set to the off-screen memory region.
;
; Entry:
;	EBX contains the MemC owner's VM handle.
;   EBP points to Window VM's client registers.
; Exit:
;	Save any registers that you use.
;

	push	eax
	push	edx

	mov	    edx, 3CEh
	in	    al, dx      			; Get current Graphic reg index
	ror	    eax,8		            ; Save it in top byte of EAX

	mov	    al,9			        ; Index the Mode640Reg (banking register)
	out 	dx,al
	inc 	dl
	in	    al,dx			        ; Get current Mode640Reg contents
    and     al,0C0h                 ; Select bank 0
	out	    dx,al
	dec	    dl
	rol	    eax,8			        ; Restore previous Graphic index reg
	out	    dx,al

	pop	edx			                ; restore saved registers
	pop	eax
	ret

EndProc MiniVDD_SetVDDBank

;-----------------------
public	MiniVDD_ResetBank
BeginProc MiniVDD_ResetBank
;
; This routine is called when the VDD has finished using the off-screen memory
; and wants the original bank to be restored.
; Note: This code won't restore a saved value if the current bank setting has
; been changed from the expected value.
;
;Entry:
;	EBX contains the VM handle.
;Exit:
;	Save anything that you use.
;

	ret

EndProc MiniVDD_ResetBank
;-------------------


public	MiniVDD_SetLatchBank
BeginProc MiniVDD_SetLatchBank
;
;When virtualizing the VGA 4 plane mode, we have to save and restore the
;latches occasionally.	This routine allows you to set to an off-screen
;bank (in this case and in most cases, the VDD bank) in order to prepare
;for restoring the VGA latches.  This routine is NOT called for saving
;the latches since this is done by simply using the standard VGA CRTC
;register 22H which all super-VGA's possess (we hope).
;
;Entry:
;	Nothing assumed.
;Exit:
;	Save anything that you use.
;

	ret

EndProc MiniVDD_SetLatchBank




public	MiniVDD_ResetLatchBank
BeginProc MiniVDD_ResetLatchBank
;
;This routine reverses the latch save that we did prior to restoring the
;latches.  Just restore the states that you saved.
;
;Entry:
;	Nothing assumed.
;Exit:
;	Save anything that you use.
;

	ret

EndProc MiniVDD_ResetLatchBank




subttl		Save and Restore Routines for Extension Registers
page +
public	MiniVDD_SaveRegisters
BeginProc MiniVDD_SaveRegisters, DOSVM
;
;This routine is called by the VDD whenever it is called upon to save
;the register state.  The "main" VDD will save all of the states of the
;"normal" VGA registers (CRTC, Sequencer, GCR, Attribute etc.) and will
;then call this routine to allow the mini-VDD to save register states
;that are hardware dependent.  These registers will be restored during the
;routine MiniVDD_RestoreRegisters.  Typically, only a few registers
;relating to the memory access mode of the board are necessary to save
;and restore.  Specific registers that control the hardware BLTer probably
;need not be saved.  The register state is saved in the mini-VDD's
;"CB data area" which is a per-VM state saving area.  The mini-VDD's
;CB data area was allocated at Device_Init time.
;
;Entry:
;	EBX contains the VM handle for which these registers are being saved.
;Exit:
;	You must preserve EBX, EDX, EDI, and ESI.
;

	ret

EndProc MiniVDD_SaveRegisters

;
;
public	MiniVDD_RestoreRegisters
BeginProc MiniVDD_RestoreRegisters, DOSVM
;
;This routine is called whenever the "main" VDD is called upon to restore
;the register state.  The "main" VDD will restore all of the states of the
;"normal" VGA registers (CRTC, Sequencer, GCR, Attribute etc.) and will
;then call this routine to allow the mini-VDD to restore register states
;that are hardware dependent.  These registers were saved during the
;routine MiniVDD_SaveRegisterState.  Typically, only a few registers
;relating to the memory access mode of the board are necessary to save
;and restore.  Specific registers that control the hardware BLTer probably
;need not be saved.  The register state is saved in the mini-VDD's
;"CB data area" which is a per-VM state saving area.  The mini-VDD's
;CB data area was allocated at Device_Init time.
;
;Entry:
;	ESI contains the VM handle for the MemC owner VM.
;	ECX contains the VM handle for the CRTC owner VM.
;Exit:
;	You must preserve EBX and EDX.	The caller preserves everything else.
;
;A short explanation of the terms "CRTC owner" and "MemC owner" is in order.
;The CRTC owner VM is the VM that owns the screen mode.  If you're running
;on the Windows desktop, then the Windows VM is the CRTC owner.  If you're
;running a full-screen DOS box, then that DOS box's VM is the CRTC owner.
;If you're running a DOS box in a window, then the CRTC owner is Windows
;but the MemC owner is the DOS VM.  What significance does this have?
;Well, when you restore the register state of a DOS VM running in a
;Window, it means that you're getting ready to VIRTUALIZE the VGA by
;using the off-screen memory.  Your VGA hardware must be setup to write
;to this memory in EXACTLY THE SAME WAY AS IF THE VGA WAS RUNNING IN
;NATIVE VGA MODE.  But...  you also must preserve the appearance of the
;Windows screen which is being displayed on the VISIBLE screen.  Thus,
;we have the screen looking like it's running in Windows HiRes packed
;pixel mode from the user's perspective, but the CPU sees the video
;memory as a 4 plane VGA.  Thus, we present this routine with both
;the CRTC owner and the MemC owner's VM handles.  Therefore, you can
;restore those states from the CRTC owner that preserve the appearance
;of the screen while restoring those states from the MemC owner that
;control how the CPU sees the video memory.
;
    ; Analyse the CRTC and MEMC owners and set VGAControlReg accordingly.

    cmp     ecx,WindowsVMHandle     ; Is the CRTC owner the Windows VM?
    jne     MVRR1                   ; No
                                    ; Yes
    cmp     esi,WindowsVMHandle     ; Is the MEMC owner the windows VM?
    je      MVRR1                   ; Yes
                                    ; No
    call    Is4PlaneOK              ; Is it OK to do 4-Plane virtualisation?
    jnc     MVRR1                   ; No
                                    ; Yes
    ; Windows is getting ready to virtualize the VGA using off-screen memory.
    ; The following code sets VGAControlReg to allow VGA like accesses to the
    ; VGA core framebuffer while still displaying the Hi-Res windows screen.

    push    ebx
    push    edx

    mov     dx,03C4h
    in      al,dx                   ; Get current Sequencer index value
    mov     ah,al                   ; Save it
    mov     al,5                    ; Select VGAControlReg
    out     dx,al
    inc     dl
    in      al,dx                   ; Get current value
    or      al,3                    ; Enable VGA like access to framebuffer
    out     dx,al
    dec     dl
    mov     al,ah                   ; Restore previous Sequencer index setting
    out     dx,al

    pop     edx
    pop     ebx

MVRR1:
	ret

EndProc MiniVDD_RestoreRegisters

;
;
public	MiniVDD_AccessVGAMemoryMode
BeginProc MiniVDD_AccessVGAMemoryMode

    ret

EndProc MiniVDD_AccessVGAMemoryMode


subttl		Save Forced Planar State
page +
public	MiniVDD_SaveForcedPlanarState
BeginProc MiniVDD_SaveForcedPlanarState, RARE
;
;This routine allows the MiniVDD to copy virtualized states and save them in
;a special Forced Planar state structure.  When this routine is called, the
;Main VDD has executed a totally virtualized mode change to the planar state
;(ie: Mode 12H).  Therefore, no screen changes are evident but the states have
;been saved in the CB data structure for the Windows VM (since the virtualized
;mode change was done when the Windows VM was the "Currently executing VM".
;Thus, when this routine is called, we have the planar state for all
;of the Extension Registers saved in the CB data structure for the
;Windows VM.  We simply copy this state into the special Planar State
;data structure.
;
;Entry:
;	Nothing assumed.
;Exit:
;	Save everything that you use!
;
;The source of our state save copy is the Windows VM's CB data structure:
;

	ret

EndProc MiniVDD_SaveForcedPlanarState



subttl		Save Message Mode State For Extended Registers
page +
public	MiniVDD_SaveMsgModeState
BeginProc MiniVDD_SaveMsgModeState, RARE
;
;This routine allows the MiniVDD to copy virtualized states and save them in
;a special Message Mode state structure.  When this routine is called, the
;Main VDD has executed a totally virtualized mode change to the blue-screen
;Message Mode state (ie: Mode 3 in SBCS Windows and Mode 12H in Far East
;Windows).  Therefore, no screen changes are evident but the states have
;been saved in the CB data structure for the Windows VM (since the virtualized
;mode change was done when the Windows VM was the "Currently executing VM".
;Thus, when this routine is called, we have the Message Mode state for all
;of the Extension Registers saved in the CB data structure for the
;Windows VM.  We simply copy this state into the special Message Mode state
;data structure.
;
;Entry:
;	EBX --> Windows VM.
;	DS --> Windows VM's BIOS data area.
;Exit:
;	Save everything that you use!
;

	ret

EndProc MiniVDD_SaveMsgModeState




; These services could be use for Softice integration
;subttl		Get the Version number
;page +
;public GLINTMVD_Get_Version
;BeginProc GLINTMVD_Get_Version, SERVICE
;    invoke  EnableVGA, pGlintDev
;	mov eax, 1
;	clc
;    ret
;EndProc GLINTMVD_Get_Version
;
;subttl		Go to VGA
;page +
;public GLINTMVD_ToVGA
;BeginProc GLINTMVD_ToVGA, SERVICE
;    mov eax, 1
;	clc
;    ret
;EndProc GLINTMVD_ToVGA
;
;subttl		Go from VGA
;page +
;public GLINTMVD_FromVGA
;BeginProc GLINTMVD_FromVGA, SERVICE
;	mov eax, 1
;	clc
;    ret
;EndProc GLINTMVD_FromVGA

;
;
subttl		Prepare to Enter a Standard VGA Mode from HiRes Mode
page +
public	MiniVDD_PreHiResToVGA
BeginProc MiniVDD_PreHiResToVGA, DOSVM
;
;This routine will be called if we're running VGA.DRV, SUPERVGA.DRV, or
;S3.DRV and applies to all drivers.
;
;When the VDD is about to switch from Windows HiRes mode to any of the
;standard VGA modes (for real -- not virtualized), this routine will be
;called.  You need to determine exactly what your hardware requires in
;order to accomplish this task.  For example, you should disable trapping
;on registers that the mini-VDD is specifically handling (such as 4AE8H
;in the case of the S3 chipset), make sure that the hardware is "ready"
;to undergo a mode transition (make sure that your hardware's graphics
;engine isn't in the middle of an operation that can't be interrupted)
;etc.  If your hardware does not return to a standard VGA mode via
;a call to INT 10H, function 0, you should also make sure to do whatever
;it takes to restore your hardware to a standard VGA mode at this time.
;Try not to touch video memory during your transition to standard VGA as
;this will disturb the reliability of the system.
;
;
;	simply enable the pass thru

    invoke  EnableVGA, 0        ; for primary display   ; 
	ret
EndProc MiniVDD_PreHiResToVGA
;
;
public	MiniVDD_PostHiResToVGA
BeginProc MiniVDD_PostHiResToVGA, DOSVM
;
;This routine will be called if we're running VGA.DRV, SUPERVGA.DRV, or
;S3.DRV and applies to all drivers.
;
;This routine is called after the ROM BIOS call is made to place the hardware
;back in the standard VGA state.  You should reenable trapping and do any
;other post-processing that your hardware might need:
;
;
	ret				;
EndProc MiniVDD_PostHiResToVGA
;
;
subttl		Prepare to Enter HiRes Mode From a Standard VGA Mode
page +
public	MiniVDD_PreVGAToHiRes
BeginProc MiniVDD_PreVGAToHiRes, DOSVM
;
;We are notified that the VDD is about to call the display driver to
;switch into HiRes mode from a full screen VGA DOS box.  We can do
;anything necessary to prepare for this.
;
;Entry:
;	EBX contains the Windows VM handle.
;Exit:
;	Nothing assumed.
MSVH_Exit:
        clc                             ; done
        ret
EndProc MiniVDD_PreVGAToHiRes

;
;
public	MiniVDD_PostVGAToHiRes
BeginProc MiniVDD_PostVGAToHiRes, DOSVM
;
;We are notified that the VDD is done setting the hardware state back
;to HiRes mode.  
;
;Entry:
;	EBX contains the Windows VM handle.
;Exit:
;	Nothing assumed.
;
;
MPVHExit:
    invoke  ReinitialiseMode
	ret				;
EndProc MiniVDD_PostVGAToHiRes




BeginProc MiniVDD_Sys_VM_Terminate, RARE

    invoke  DisableHardware

    clc
	ret				;
EndProc MiniVDD_Sys_VM_Terminate
;

public _MapFlat
BeginProc _MapFlat, RARE
	push	ebp
	mov		ebp, esp
	push	edi
	push	esi

	mov		ah, [ebp + 8]				; Get Segment Offset
	mov		al, [ebp + 12]				; Get Selector Offset
    VMMcall Map_Flat

	pop		esi
	pop		edi
	pop		ebp
    ret
EndProc _MapFlat

public MiniVDD_VesaSupport
BeginProc MiniVDD_VesaSupport, RARE
;
; This function is called whenever a VESA INT 10 call
; is made..
; We are looking for DPMS support calls, but only if we
; are the active display.

    pushad

ifdef POWER_STATE_MANAGE
;   VesaSupport needs DevNode not SysVMHandle as arg1
    invoke finddevnodeforcurrentlyactivevga
    test    eax, eax            ; is a Permedia the VGA?
    je      @f
    invoke  VesaSupport, eax, ebp
;;  invoke  VesaSupport, ebx, ebp

endif

    test    eax, eax
    je      @f
    popad
    stc
    ret
@@:
    popad
    clc
    ret
EndProc MiniVDD_VesaSupport

public MiniVDD_TurnVGAOn
BeginProc MiniVDD_TurnVGAOn, RARE

    pushad
;;
    mov     edx, [esp+4+32]    ; get DevNode (+32 for pushad)
    invoke  TurnVGAOn, edx, ebp
    test    eax, eax
    je      @f
    popad
    stc
    ret
@@:
    popad
    clc
    ret
EndProc MiniVDD_TurnVGAOn

public MiniVDD_TurnVGAOff
BeginProc MiniVDD_TurnVGAOff, RARE

    pushad
;;
    mov     edx, [esp+4+32]    ; get DevNode (+32 for pushad)
    invoke  TurnVGAOff, edx, ebp
    test    eax, eax
    je      @f
    popad
    stc
    ret
@@:
    popad
    clc
    ret
EndProc MiniVDD_TurnVGAOff


;
public MiniVDD_VesaPostProcessing
BeginProc MiniVDD_VesaPostProcessing, RARE
;
    ret
EndProc MiniVDD_VesaPostProcessing

;
;
subttl		Enable and Disable BLTer Register Trapping
page +
public	MiniVDD_EnableTraps
BeginProc MiniVDD_EnableTraps, DOSVM
;
;As stated elsewhere, the VDD needs to set traps on the BLTer registers used
;by the display driver whenever it is using the off-screen memory to
;virtualize VGA graphics mode in the background or in a window.  The reason
;the VDD does this is to receive notification on the first access to a
;BLTer register AFTER returning to the Windows VM.  Then, the VDD switches
;the state of the hardware back to that appropriate for running Windows
;HiRes mode.  In this routine, all you need to do is call the VMM service
;Enable_Global_Trapping for each of the ports that you registered at
;MiniVDD_Device_Init.  Then, the VDD will receive notification at the
;proper time and will switch states properly.
;
;Entry:
;	Nothing assumed.
;Exit:
;	Just Preserve EBX, and ESI.
;
;
MERTExit:
	mov	edx,3dah
	VMMCall Enable_Global_Trapping
	ret				;
EndProc MiniVDD_EnableTraps
;
;
public	MiniVDD_DisableTraps
BeginProc MiniVDD_DisableTraps, DOSVM
;
;See comment at EnableTraps.
;
;Entry:
;	Nothing assumed.
;Exit:
;	Just Preserve EBX and ESI.
;
;
MDRTExit:
	mov	edx,3dah
	VMMCall Disable_Global_Trapping
	ret				;
EndProc MiniVDD_DisableTraps
;
;
subttl		Set Hardware to a Not Busy State
page +
public	MiniVDD_MakeHardwareNotBusy
BeginProc MiniVDD_MakeHardwareNotBusy, DOSVM
;
;Quite often, we need to make sure that the hardware state is not busy
;before changing the MemC mode from the Windows HiRes state to VGA state
;or vice-versa.  This routine allows you to do this (to the best of your
;ability).  You should try to return a state where the hardware BLTer
;isn't busy.
;
;Entry:
;	EAX contains the CRTC owner's VM handle.
;	EBX contains the currently running VM handle.
;	ECX contains the MemC owner's VM handle.
;	EDX contains the CRTC index register.
;Exit:
;	You must save all registers that you destroy except for EAX & ECX.
;
MHNBExit:
	ret				;
EndProc MiniVDD_MakeHardwareNotBusy
;
;
subttl		Display Driver Is Being Disabled Notification
page +

if 0  ;; not used  ;;

public	MiniVDD_DisplayDriverDisabling
BeginProc	MiniVDD_DisplayDriverDisabling, RARE
;
;The display driver is in its Disable routine and is about to set the
;hardware back into VGA text mode.  Since this could either mean that
;the Windows session is ending or that some Windows application is switching
;to a VGA mode to display something full screen (such as MediaPlayer), we
;need to disable our MiniVDD_RestoreRegisters code because we're liable
;to restore a Windows HiRes state when we shouldn't!  Thus, clear the
;DisplayEnabledFlag to prevent this:
;
    invoke  FindDevNodeForCurrentlyActiveVGA
    invoke  FindpDevFromDevNodeTable, eax
    invoke  EnableVGA, eax        ;
	ret				;
EndProc MiniVDD_DisplayDriverDisabling

endif ;0  ;; not used  ;;

;
;


public	MiniVDD_PreCRTCModeChange
BeginProc MiniVDD_PreCRTCModeChange, RARE
;
;This routine allows the display driver to call us before it does the
;mode set using INT 10H.  This MiniVDD is going to use the routine to
;Entry:
;	Nothing assumed.
;Exit:
;	Nothing assumed.

	ret

EndProc MiniVDD_PreCRTCModeChange


subttl		Update any enhanced registers after the mode set.
page +
public	MiniVDD_PostCRTCModeChange
BeginProc MiniVDD_PostCRTCModeChange, RARE

    ret

EndProc MiniVDD_PostCRTCModeChange

subttl		Return Total Memory Size on Card
page +
public	MiniVDD_GetTotalVRAMSize
BeginProc MiniVDD_GetTotalVRAMSize
;
;Entry:
;	EBX contains the Current VM Handle (which is also the CRTC owner's
;	VM Handle).
;	EBP --> VM's Client Registers.
;Exit:
;	CY is returned upon success.
;	All registers (except ECX) must be preserved over the call.
;	ECX will contain the total VRAM size in bytes.
;

	mov	ecx,512*1024	; Total VGA framebuffer size
	stc				    ;
	ret

EndProc MiniVDD_GetTotalVRAMSize


public	MiniVDD_GetCurrentBankWrite
BeginProc MiniVDD_GetCurrentBankWrite
;
;Entry:
;	EBX contains the VM Handle (Always the "CurrentVM").
;	EBP --> Client Register structure for VM
;Exit:
;	CY is returned upon success.
;	All registers (except EDX & EAX) must be preserved over the call.
;	EDX will contain the current write bank (bank A) set in hardware.
;

    xor edx,edx
	stc				;return success to caller
	ret				;

EndProc MiniVDD_GetCurrentBankWrite



public	MiniVDD_GetCurrentBankRead
BeginProc MiniVDD_GetCurrentBankRead
;
;Entry:
;	EBX contains the VM Handle (Always the "CurrentVM").
;	EBP --> Client Register structure for VM
;Exit:
;	CY is returned upon success.
;	All registers (except EDX & EAX) must be preserved over the call.
;	EDX will contain the current write bank (bank A) set in hardware.
;

    xor edx,edx
	stc				;return success to caller
	ret				;

EndProc MiniVDD_GetCurrentBankRead



public	MiniVDD_GetBankSize
BeginProc MiniVDD_GetBankSize
;
;Entry:
;	EBX contains the VM Handle (Always the "CurrentVM").
;	ECX contains the BIOS mode number that we're currently in.
;	EBP --> Client Register structure for VM
;Exit:
;	CY is returned upon success.
;	All registers (except EDX & EAX) must be preserved over the call.
;	EDX will contain the current bank size.
;	EAX will contain the physical address of the memory aperture or
;		zero to indicate VRAM at A000H.
;
;Our bank size is ALWAYS 64K.
;
	mov	edx,64*1024		;
	xor	eax,eax 		; return VRAM is at A000H
	stc				    ; return success to caller
	ret

EndProc MiniVDD_GetBankSize



public	MiniVDD_SetBank
BeginProc MiniVDD_SetBank
;
;Entry:
;	EAX contains the read bank to set.
;	EDX contains the write bank to set.
;	EBX contains the VM Handle (Always the "CurrentVM").
;	EBP --> Client Register structure for VM
;Exit:
;	CY is returned upon success.
;	All registers must be preserved over the call.
;	EDX will contain the current write bank (bank A) set in hardware.
;
;
    ; This routine is called to set the VGA offscreen bank.
    ; Since the PERMEDIA always uses bank 0 for this the code 'knows'
    ; which bank to use when the time comes to use it.  Therefore there
    ; is nothing to do here except report success.

	stc
	ret

EndProc MiniVDD_SetBank



subttl          Interrupt handlers

public MiniVDD_DMA_Event
BeginProc       MiniVDD_DMA_Event,HIGH_FREQ
;
;       Use VWIN32 to fire the event
;
        mov     eax,edx                 ; get event handle
        VxDcall _VWIN32_SetWin32Event   ; signal it
        ret
EndProc MiniVDD_DMA_Event

public  _MiniVDD_Hw_Int
BeginProc       _MiniVDD_Hw_Int,HIGH_FREQ

;
; This handler is called by the VPIC device when our interrupt occurs
; We must check if we have generated the interrupt, and if so clear it,
; pass it to the correct VM and continue.
;


; the reference data dword (in edx) is the pDev for the card that issued
; the interrupt.  Pass it to the handler.

	    invoke HandleInterrupt, edx
		test eax, eax
		jz MHWI_NotOurInterrupt

MHWI_EndInterrupt:
        clc
        ret

MHWI_NotOurInterrupt:
        stc
        ret

EndProc _MiniVDD_Hw_Int

ifdef POWER_STATE_MANAGE
subttl          Power State Management





;*********************************************************************
; SetAdapterPowerState(devnode,PowerState) sets the power specified
; power state by saving or restoring the device state (code in video.c)
;*********************************************************************

public		MiniVDD_SetAdapterPowerState
BeginProc	MiniVDD_SetAdapterPowerState

	jmp	SetAdapterPowerState

EndProc		MiniVDD_SetAdapterPowerState

;*********************************************************************
; GetAdapterPowerStateCaps(devnode) returns the power states supported
; by the display adapter
;*********************************************************************

public		MiniVDD_GetAdapterPowerStateCaps
BeginProc	MiniVDD_GetAdapterPowerStateCaps

;	mov	eax, esp+4	; devnode (if you need it)

	VMMCall	Get_VMM_Version
    .if ( !carry? && eax >= 045ah )
	; set hibernate capability
	mov	eax, CM_POWERSTATE_HIBERNATE	
    .else
	; WIN98 and earlier versions do not handle hibernate
	xor	eax, eax
    .endif

	or	eax, CM_POWERSTATE_D0 + \
		     CM_POWERSTATE_D1 + \
		     CM_POWERSTATE_D2 + \
		     CM_POWERSTATE_D3

	ret

EndProc		MiniVDD_GetAdapterPowerStateCaps

endif

VxD_LOCKED_CODE_ENDS

end


