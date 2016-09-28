;/******************************Module*Header**********************************\
; *
; *                           ***************************************
; *                           * Permedia 2: miniVDD SAMPLE CODE   *
; *                           ***************************************
; *
; * Module Name: mtrr.asm
; *              Assembler support file for writecmb.c
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

	.586

	.xlist
include 	    VMM.INC

	.list

VxD_CODE_SEG


;**************************************************************************	
;       Function:	int HasCPUID()
;
;       Purpose:        Check for existence of CPUID instruction.
;                       If exists execute CPUID with eax==0.
;
;       Inputs:         none
;                       
;       Output:         
;                       
;       Returns:        0 - no CPUID instruction supported
;                       1 - CPUID instruction supported
;
;		Notes:			Original code from www.cyrix.com
;
;**************************************************************************

BeginProc HasCPUID, PUBLIC, CCALL
	push	ebx

	pushfd                  ; get extended flags
	pop     eax
	mov     ebx, eax        ; save current flags

	xor     eax, 200000h    ; toggle bit 21
	push    eax             ; put new flags on stack
	popfd                   ; flags updated now in flags

	pushfd                  ; get extended flags
	pop     eax             
	xor     eax, ebx        ; if bit 21 r/w then eax <> 0

	je      no_cpuid        ; can't toggle id bit (21) no cpuid here

	mov     eax, 1          ; cpuid supported

	jmp     done_cpuid_sup

no_cpuid:
	mov     eax, 0          ; cpuid not supported

done_cpuid_sup:

	pop		ebx
	ret
EndProc HasCPUID

BeginProc GetCPUID, PUBLIC, CCALL
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	mov	eax, 1
	cpuid
	mov	eax, edx

   	pop 	edi
	pop 	esi
	pop 	edx
	pop 	ecx
	pop 	ebx

	ret
EndProc GetCPUID


;
; Interface to RDMSR instruction:
;

BeginProc ReadMSR, PUBLIC, CCALL

ArgVar regno,		DWORD
ArgVar lowPart,		DWORD
ArgVar highPart,	DWORD

EnterProc
	pushad
	mov	ecx, regno
	rdmsr
	mov	ecx, DWORD PTR lowPart
	mov	DWORD PTR [ecx], eax
	mov	ecx, DWORD PTR highPart
	mov	DWORD PTR [ecx], edx
   	popad
LeaveProc
Return

EndProc ReadMSR


;
; Interface to the WRMSR instruction.
;

BeginProc WriteMSR, PUBLIC, CCALL

ArgVar regno,		DWORD
ArgVar lowPart,		DWORD
ArgVar highPart,	DWORD

EnterProc
    pushad
	mov	ecx, DWORD PTR lowPart
	mov 	eax, DWORD PTR [ecx]
	mov 	ecx, DWORD PTR HighPart
	mov 	edx, DWORD PTR [ecx]
	mov	ecx, regno
	wrmsr
    popad
LeaveProc
Return

EndProc WriteMSR

VxD_CODE_ENDS

end
