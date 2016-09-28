.386p

TITLE LDIOS.ASM - LDVXD test CD-ROM VxD
;*****************************************************************************
;
;	Copyright (c) 1997-2000  Microsoft Corporation
;
;	Module Name:
;
;	   ldios.asm
;
;	Abstract:
;
;   	This is the VxD, LDVXD.VXD, that provides the interface to CDVSD.VXD 
;   	used by the LOCKCD.EXE CD locking demo.
;   	This module contains code to issue I/O requests to the system that 
;		do not have C callable interfaces.
;
;	Author:
;
;   	original Redbook audio CD test code by Len Smale (formerly RBIOS.ASM)
;
;	Environment:
;
;   	Kernel mode only
;
;
;	Revision History:
;    	4/19/2000 Modifications to add use of drive locking mechanisms by 
;	Kevin Burrows
;*****************************************************************************

	.xlist
MASM    = 1
	include	vmm.inc
	include sgd.inc
	include ilb.inc
	include blockdev.inc
	include	dcb.inc
	include ior.inc
	include iop.inc
	include vrp.inc
	include isp.inc
	include ios.inc
	include opttest.inc
	include debug.inc
	include	iodebug.inc

	.list

;===========================================================================
;			      P U B L I C S
;===========================================================================



	VxD_LOCKED_DATA_SEG

	extrn 	_LDVXD_ilb:dword
	VxD_LOCKED_DATA_ENDS

	VxD_LOCKED_CODE_SEG
	
;**************************************************************************
; LD_SendRequest
;--------------------------------------------------------------------------
beginproc	LD_SendRequest, SCALL, esp

    ArgVar              pDcb,     dword
    ArgVar              pIop,     dword
    ArgVar              fnSendLockCommand, dword

     EnterProc
     SaveReg <ad>                ; preserve registers
     mov      esi, pIop
     lea      esi, [esi].IOP_ior
     mov      edi, pDcb
     lea      edi, [edi].DCB_bdd
     mov      ecx, fnSendLockCommand  ; set up for a lock
     or       ecx, ecx			 
     jz       sr0                ; Must be a conventional call
     call     ecx                ; Call IOS's locked entry point 
     jmp      sr1
sr0: VxDcall  IOS_SendCommand    ; call the conventional IOS_SendCommand
sr1: RestoreReg  <ad>            ; restore registers
     LeaveProc
     Return
 	
endproc LD_SendRequest

;**************************************************************************
; LD_LockDrive -kburrows
;--------------------------------------------------------------------------
beginproc       LD_LockDrive, SCALL, esp

    ArgVar      dwDrive, dword

    EnterProc
    SaveReg         <ECX, EDX>          ; preserve registers
    push             dwDrive            ; pass the drive to lock via the stack
    VxDCall         _IOS_LockDrive      ; call IOS to lock the specified drive
    add              esp, 4             ; restore the stack
    RestoreReg      <EDX, ECX>          ; restore registers
    LeaveProc
    Return
endproc LD_LockDrive

;**************************************************************************
; LD_UnlockDrive -kburrows
;--------------------------------------------------------------------------
beginproc       LD_UnlockDrive, SCALL, esp

    ArgVar         fnSendLockCommand, dword

    EnterProc
    SaveReg      <ECX, EDX>             ; preserve registers
    mov           ecx,fnSendLockCommand ; get the locked SendCommand function
    or            ecx, ecx              ; validate it
    jz            @f                    ; skip the call if function is invalid
    push          ecx                   ; IOS wants the function on the stack
    VxDCall      _IOS_UnlockDrive       ; call IOS to unlock the drive
    add           esp, 4                ; restore the stack
@@: RestoreReg   <EDX, ECX>             ; restore registers
    LeaveProc
    Return
endproc LD_UnlockDrive

;**************************************************************************
; LD_RefreshLock -kburrows
;--------------------------------------------------------------------------
beginproc       LD_RefreshLock, SCALL, esp

     ArgVar     fnSendLockCommand, dword

     EnterProc
     SaveReg    <ad>                    ; preserve all registers
     xor         esi, esi               ; clear the IOR pointer
     mov         ecx, fnSendLockCommand ; get the locked SendCommand function
     or          ecx, ecx               ; validate it
     jz          @f                     ; skip the call if function is invalid
     call        ecx                    ; call IOS's locked  entry point 
@@:  RestoreReg <ad>                    ; restore all registers
     LeaveProc
     Return
 	
endproc LD_RefreshLock


;**************************************************************************
; LD_IsDriveLocked -kburrows
;--------------------------------------------------------------------------
beginproc       LD_IsDriveLocked, SCALL, esp

    ArgVar      dwDrive, dword

    EnterProc
    SaveReg    <ECX, EDX>               ; preserve registers
    push        dwDrive                 ; pass the drive to query via the stack
    VxDCall    _IOS_IsDriveLocked       ; call IOS to query the lock status
    add         esp, 4                  ; restore the stack
    RestoreReg <EDX, ECX>               ; restore registers
    LeaveProc
    Return
endproc LD_IsDriveLocked


;**************************************************************************
; LD_MapIORResult
;--------------------------------------------------------------------------
beginproc	LD_MapIORResult, SCALL, esp

    ArgVar		IorResult, dword
	EnterProc
	xor		eax, eax			; assume success
	mov		ecx, IorResult
	cmp		ecx, IORS_ERROR_DESIGNTR	; Q: any errors
	jb		MEDone				; Y: map the error
	VxDCall IOSMapIORSToI21				; eax = new error code
MEDone:	
 	LeaveProc
	Return
 	
endproc LD_MapIORResult
 
VxD_LOCKED_CODE_ENDS

END	

