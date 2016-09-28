PAGE 58,132
;******************************************************************************
TITLE LDVXD.ASM - LDVXD test CD-ROM VxD
;******************************************************************************
;
;
;	Copyright (c) 1997  Microsoft Corporation
;
;	Module Name:
;
;	   ldvxd.asm
;
;	Abstract:
;
;   	This is the VxD, LDVXD.VXD, that provides the interface to CDVSD.VXD 
;   	used by the LOCKCD.EXE drive locking demo.
;   	This module contains the main driver skeleton.
;	It handles driver load/unload, debug commands, and routing IOCTLs.
;
;	Author:
;
;   	original Redbook audio CD test code by Len Smale (formerly RBVXD.ASM)
;
;	Environment:
;
;   	Kernel mode only
;
;
;	Revision History:
;    	4/19/2000 Modifications to add use of drive locking mechanisms by 
;	Kevin Burrows 
	
	.386p

;******************************************************************************
;                       L O C A L   C O N S T A N T S
;******************************************************************************

	.XLIST
LDVXD_Dynamic                equ     1

	INCLUDE vmm.inc
	INCLUDE VWIN32.inc
	INCLUDE Vxdldr.inc
	INCLUDE Debug.Inc
	include ios.inc
	include aep.inc
	include drp.inc
	include ilb.inc
	include LDIF.INC
	
	LDVXDName 		EQU	<'LDVXD LOCK CD IF'> 	;Must be 16 chars, yuck!
	LDVXDRev		EQU	00H
	LDVXDFeature	EQU	00H
	LDVXD_IF		EQU	00H

AER_stack_frame struc
AER_return	dd	?
AER_aep_offset	dd	?
AER_stack_frame ends


LDVXD_Device_Id              equ     Undefined_Device_Id
	.LIST

;******************************************************************************
;               V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device LDVXD, 3, 1, LDVXD_Control, LDVXD_Device_Id, Undefined_Init_Order

VxD_LOCKED_DATA_SEG

LDVXD_Iotbl  label dword                       ; DeviceIoControl service table
    dd    offset32 LDVXD_CloseHandle           ; -1 -- DIOC_CLOSEHANDLE
    dd    offset32 LDVXD_GetVersion            ;  0 -- DIOC_GETVERSION
    dd    offset32 LDVXD_GetVxdVersion         ;  1 -- GETAPPVERSION
LDVXD_NumServices equ ($-LDVXD_Iotbl)/4

Public	_LDVXD_ilb
_LDVXD_ilb	ILB	<>

VxD_LOCKED_DATA_ENDS

VxD_IDATA_SEG

;Driver registration packet
Drv_Reg_Pkt     DRP {EyeCatcher, 
                     DRP_FSD,
                     ,
                     ,
                     LDVXDName,
                     LDVXDRev,
                     LDVXDFeature, 
                     LDVXD_IF}

VxD_IDATA_ENDS

VxD_ICODE_SEG

;******************************************************************************
;
;   LDVXD_Device_Init
;
;------------------------------------------------------------------------------

BeginProc LDVXD_Device_Init
	Vxdcall IOS_Get_Version
	jc	short LDVXD_Device_Init_Exit ;no IOS present bail out.
	call	LDVXD_IOS_Reg
LDVXD_Device_Init_Exit:
	ret

EndProc LDVXD_Device_Init

;******************************************************************************
;
;   LDVXD_IOS_Reg
;
;   DESCRIPTION:
;       attach to IOS and register to obtain ILB entry points
;
;   ENTRY:
;	EBX = System VM Handle
;
;   EXIT:
;
;   USES:
;	All registers and flags
;
;==============================================================================


BeginProc LDVXD_IOS_Reg

	;
	; Set up DRP packet for IOS driver registration and call IOS
	;

	mov     [Drv_Reg_Pkt].DRP_LGN, DRP_TSD

	mov     [Drv_Reg_Pkt].DRP_AER,offset32 LDVXD_async_event
	mov     [Drv_Reg_Pkt].DRP_ILB,offset32 _LDVXD_ilb

	push    offset32 Drv_Reg_Pkt    ;packet (DRP)
	VXDCall IOS_Register            ;call registration
	add     esp,04                  ;Clean up stack
	movzx	eax, [Drv_Reg_Pkt].DRP_reg_result
	cmp		eax, DRP_REMAIN_RESIDENT
	jne		RegDoneFail
	 ; compare resulting in zero, so carry clear
	TRAPc
	ret
	
RegDoneFail:
	stc		
	ret

EndProc LDVXD_IOS_Reg

VxD_ICODE_ENDS

VXD_LOCKED_CODE_SEG

;************************************************************************
;									
; LDVXD_async_event - Asynchronous Event Handler
;
;	PageAsyncRequest basically jump to the appropriate routine
;	indicated by the AER command in the Async Event Packet (AEP).
;
;	Entry:	Far pointer to AEP on stack
;
;	Exit:	ebx->AEP
;

LDVXD_async_event	proc	near
	mov	ebx,[esp.AER_aep_offset]    ; pick up the aep ptr
	mov	[ebx.AEP_result],0			; show no error yet.
	ret								; return to our caller.
LDVXD_async_event	endp

VXD_LOCKED_CODE_ENDS

VxD_CODE_SEG

;******************************************************************************
;
;   LDVXD_Control
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
BeginProc LDVXD_Control

	Control_Dispatch SYS_DYNAMIC_DEVICE_INIT, LDVXD_Device_Init
	Control_Dispatch SYS_DYNAMIC_DEVICE_EXIT, LDVXD_Device_Exit
	Control_Dispatch W32_DeviceIocontrol,     LDVXD_Device_Ioctl
	Control_Dispatch Debug_Query,             LDVXD_DebugQuery

	clc
	ret
EndProc LDVXD_Control

;******************************************************************************
;
;   LDVXD_Device_Exit
;
;------------------------------------------------------------------------------

BeginProc LDVXD_Device_Exit

	; Do not permit unload because no easy way to deregister from IOS
	stc	
	;clc             ; continue with unload
	ret

EndProc LDVXD_Device_Exit

;******************************************************************************
;
;   LDVXD_Device_Ioctl
;    ENTRY:
;        ESI -> address of DIOC structure
;
;------------------------------------------------------------------------------

BeginProc LDVXD_Device_Ioctl
    mov   ecx, [esi+dwIoControlCode]      ; ECX = service index
    cmp         ecx, CDROM_IOCTL_LOCK_DRIVE
    je          LDVXD_ProcessLockRequest
    cmp         ecx, CDROM_IOCTL_UNLOCK_DRIVE
    je          LDVXD_ProcessLockRequest
    cmp         ecx, CDROM_IOCTL_IS_DRIVE_LOCKED
    je          LDVXD_ProcessLockRequest
    cmp         ecx, CDROM_IOCTL_REFRESH_LOCK
    je          LDVXD_ProcessLockRequest
    cmp		ecx, CDROM_IOCTL_READ_DA
    je		LDVXD_ReadDA
    cmp		ecx, CDROM_IOCTL_PROXY
    je		LDVXD_IoctlProxy
    inc   ecx                             ; add one to service index
    cmp   ecx, LDVXD_NumServices          ; is service in bounds?
    jae   devctrl_unsupported             ; if not, fail the call
    jmp   LDVXD_Iotbl[4*ecx]              ; yes. branch to server

;    ECX = -1 (DIOC_CLOSEHANDLE)
LDVXD_CloseHandle:
    jmp   devctrl_okay

;    ECX = 0 (DIOC_GETVERSION)
LDVXD_GetVersion:
    jmp   devctrl_okay                    ; satisfy check by VWIN32's
                                          ;   CreateFile handler

;    ECX = 1 (GETAPPVERSION)
;
;    Output:
;        0     (DWORD) version number of this VxD
LDVXD_GetVxdVersion:
    cmp   [esi+cbOutBuffer], 4            ; need at least 4 output bytes
    jb    @F                              ; fail call if not
    mov   edx, [esi+lpvOutBuffer]         ; point to output buffer
    test  edx, edx                        ; be sure not null
    jz    @F 
    movzx eax, word ptr LDVXD_DDB + DDB_Dev_Major_Version
    xchg  ah, al                          ; (fields reversed in DDB)
    mov   dword ptr [edx], eax            ; store version number
    mov   edx, [esi+lpcbBytesReturned]    ; get ptr to caller's variable
    test  edx, edx                        ; make sure not null
    jz    @F                              ;   ..
    mov   dword ptr [edx], 4              ; store # bytes returned
 @@:
    jmp   devctrl_okay                    ; done

LDVXD_ProcessLockRequest:
        sCall   LD_ProcessLockRequest <ecx, esi>
	jmp		devctrl_done


LDVXD_ReadDA:
	sCall	LD_ReadDA <esi>
	jmp		devctrl_done

LDVXD_IoctlProxy:
	sCall	LD_IoctlProxy <esi>
	jmp		devctrl_done

devctrl_okay:
    xor   eax, eax                        ; indicate success
devctrl_done:
	or		eax, eax
	jnz		devctrl_fail
    clc
    ret                                   ; return to VWIN32
    
devctrl_unsupported:
    mov   eax, 50                         ; ERROR_NOT_SUPPORTED
devctrl_fail:
    stc                                   ; indicate error
    ret                                   ; return to VWIN32

EndProc  LDVXD_Device_Ioctl

;******************************************************************************
;
;   LDVXD_DebugQuery
;
;------------------------------------------------------------------------------
BeginProc LDVXD_DebugQuery
    Trace_Out " "
    Trace_Out "*** LDVXD DEBUG INFORMATION ***"
    Trace_Out " "
    Trace_Out "[1] Option 1"
    Trace_Out "[2] Option 2"
    Trace_Out "[3] Option 3"
    Trace_Out " "
    Trace_Out "Please select an option: ", nocrlf

    VMMcall In_Debug_Chr

    Trace_Out " "
    Trace_Out " "
    jz   short DebugDone
    cmp  al, '1'
    jz   short DebugOption1
    cmp  al, '2'
    jz   short DebugOption2
    cmp  al, '3'
    jz   short DebugOption3
    Trace_Out "Invalid LDVXD debug option"
    Trace_Out " "

  DebugDone:
    clc
    ret

  DebugOption1:
    Trace_Out "LDVXD debug option 1"
    Trace_Out " "
    jmp  short DebugDone

  DebugOption2:
    Trace_Out "LDVXD debug option 2"
    Trace_Out " "
    jmp  short DebugDone

  DebugOption3:
    Trace_Out "LDVXD debug option 3"
    Trace_Out " "
    jmp  short DebugDone
EndProc LDVXD_DebugQuery

VxD_CODE_ENDS

	END
