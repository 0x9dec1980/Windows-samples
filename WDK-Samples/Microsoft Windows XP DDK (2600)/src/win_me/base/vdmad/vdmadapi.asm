PAGE 58,132
;******************************************************************************
TITLE vdmadapi.asm -
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1989
;
;   Title:	vdmadapi.asm -
;
;   Version:	1.00
;
;   Date:	15-Sep-1989
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   15-Sep-1989 RAP
;
;==============================================================================

	.386p

.XLIST
	include VMM.INC
	include DEBUG.INC
	include VDMAD.INC
	include DMASYS.INC
	include OPTTEST.INC
.LIST
.sall


VxD_DATA_SEG

EXTRN DMA_Max_Physical:DWORD
EXTRN DMA_Buff_Table:BYTE

ifdef NEC_98
Vect_Ch_Flag	db	?		
endif

original_PM_4B_EIP  dd	?
original_PM_4B_CS   dw	?
ifndef NO_EXIT
original_40_7B	    db	?
endif ;NO_EXIT
Virt_47Bh	    db	?
save_4B_vector	    dd	-1

VxD_DATA_ENDS


VxD_LOCKED_DATA_SEG
EXTRN VDMAD_Machine_Type:BYTE
VxD_LOCKED_DATA_ENDS


VxD_ICODE_SEG

;******************************************************************************
;
;   VDMAD_API_Device_Init
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_API_Device_Init

ifdef	NEC_98
	mov	al, ds:[5adh]
else
	mov	al, ds:[47Bh]
endif

ifndef NO_EXIT
	mov	[original_40_7B], al
endif ;NO_EXIT
	mov	[Virt_47Bh], al

	mov	ecx, ds:[4Bh*4] 	    ; save original 4Bh vector
	mov	[save_4B_vector], ecx

ifdef	NEC_98
	mov Vect_Ch_Flag,0		; Init flag to check changing int 4b Vect NEC 911002
	movzx	ecx, word ptr ds:[4Bh*4+2]	;
	cmp	ecx, 0060h		; if segment = DEFAULT?
	je	short hook_4B	   	;   Yes, jump due to not chain
	mov Vect_Ch_Flag,1		; set flag that INT 4Bh chain
else;	!NEC_98
	IF_NOT_MCA_JMP <short check_vector>
	test	al, DMA_Chain_4B	    ;Q: chain bit set?
	jnz	short hook_4B		    ;	Y:

; check current value segment for not E000 or F000
;
check_vector:
	and	[Virt_47Bh], NOT DMA_Chain_4B ; force the bit off
	shr	ecx, 16 		    ; shift segment to low word
	jecxz	short hook_4B		    ; jump, if no segment
	cmp	ecx, 0F000h		    ;Q: segment of system ROM?
	je	short hook_4B		    ;	Y: don't chain
	cmp	ecx, 0E000h		    ;Q: segment of system ROM?
	je	short hook_4B		    ;	Y: don't chain
	or	[Virt_47Bh], DMA_Chain_4B   ; chaining required
endif;	NEC_98

hook_4B:
	mov	esi, OFFSET32 VDMAD_V86_DMA_Services

	mov	eax, 4Bh
	xor	edx, edx
	VMMCall Allocate_V86_Call_Back
IFDEF DEBUG
	jnc	short @F
	Debug_Out "ERROR: VDMAD could not allocate V86 call back"
@@:
ENDIF
	jc	short di_BPI_Fatal
	mov	ds:[4Bh*4], eax

	mov	eax, 4Bh
	VMMcall Get_PM_Int_Vector
	mov	[original_PM_4B_CS], cx
	mov	[original_PM_4B_EIP], edx

	mov	esi, OFFSET32 VDMAD_PM_DMA_Services
	xor	edx, edx
	VMMcall Allocate_PM_Call_Back
IFDEF DEBUG
	jnc	short VMADD10
	Debug_Out "ERROR: VDMAD could not allocate PM call back"
VMADD10:
ENDIF
	jc	short di_BPI_Fatal
	mov	ecx, eax
	movzx	edx, ax
	shr	ecx, 10h
	mov	eax, 4Bh
	VMMcall Set_PM_Int_Vector

ifdef	NEC_98
	or	byte ptr ds:[5adh], DMA_Services_Avail OR DMA_Chain_4B
else
	or	byte ptr ds:[47Bh], DMA_Services_Avail OR DMA_Chain_4B
endif
	ret

di_BPI_Fatal:
	VMMcall Fatal_Memory_Error

EndProc VDMAD_API_Device_Init


VxD_ICODE_ENDS


VxD_CODE_SEG

EXTRN VDMAD_Get_DMA_Handle:NEAR
ifdef	NEC_98
EXTRN VDMAD_Initialize_DMA:NEAR
endif

ifndef NO_EXIT
;******************************************************************************
;
;   VDMAD_API_System_Exit
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_API_System_Exit

	mov	al, [original_40_7B]
	Begin_Touch_1st_Meg
ifdef	NEC_98
	mov	ds:[5adh], al
else
	mov	ds:[47Bh], al
endif
	End_Touch_1st_Meg

;
; restore original 4B vector
;
	mov	eax, [save_4B_vector]
	inc	eax			;Q: vector saved? (= -1)
	jz	short @F		;   N: skip restore
	dec	eax
	Begin_Touch_1st_Meg
	mov	ds:[4Bh*4], eax 	; restore 4Bh vector to original
	End_Touch_1st_Meg
@@:

ifdef	NEC_98
	call	VDMAD_Initialize_DMA	; re-initialize DMA before ending
endif

	ret

EndProc VDMAD_API_System_Exit

endif ;NO_EXIT

;******************************************************************************
;
;   VDMAD_PM_DMA_Services
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_PM_DMA_Services

	mov	eax, [ebp.Client_EAX]
	cmp	ah, 81h 		;Q: DMA service?
	jne	SHORT PM_unknown	;   N:
	VMMCall Simulate_Iret
	CallRet SHORT VDMAD_DMA_Services

PM_unknown:
ifdef	NEC_98
	cmp	Vect_Ch_Flag,1		; if INT 4Bh is chained?
	je	SHORT PM_chain		;   Yes, jmp
else
	test	[Virt_47Bh], DMA_Chain_4B ;Q: chain?
	jnz	SHORT PM_chain		;	 Y:
endif
	VMMjmp	Simulate_Iret

PM_chain:
	movzx	ecx, [original_PM_4B_CS]
	mov	edx, [original_PM_4B_EIP]
	VMMjmp	Simulate_Far_Jmp	; jump to previous 4B handler

EndProc VDMAD_PM_DMA_Services


;******************************************************************************
;
;   VDMAD_V86_DMA_Services
;
;   DESCRIPTION:    DMA services for V86
;
;   ENTRY:	    CLIENT_AH = major function #
;		    CLIENT_AL = minor function #
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_V86_DMA_Services

	mov	eax, [ebp.Client_EAX]
	cmp	ah, 81h 		;Q: DMA service?
	jne	SHORT v86_unknown	;   N:
	VMMCall Simulate_Iret
	CallRet SHORT VDMAD_DMA_Services

v86_unknown:
ifdef	NEC_98
	cmp	Vect_Ch_Flag,1		; if INT 4Bh is chained?
	je	SHORT v86_chain		;   Yes, return carry clear to consume int
else
	test	[Virt_47Bh], DMA_Chain_4B ;Q: chain?
	jnz	SHORT v86_chain 	;	 Y:
endif
	VMMjmp	Simulate_Iret

v86_chain:
	mov	ecx, [save_4B_vector]
	movzx	edx, cx
	shr	ecx, 16
	VMMjmp	Simulate_Far_Jmp	; jump to previous 4B handler

EndProc VDMAD_V86_DMA_Services


;******************************************************************************
;
;   VDMAD_DMA_Services
;
;   DESCRIPTION:    Common handler of DMA services called from V86 and PM
;
;   ENTRY:	    AL = function #
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_DMA_Services

FirstService = 2
MaxService   = 0Ch

	cmp	al, MaxService
	ja	SHORT bad_service
	sub	al, FirstService
	jb	SHORT bad_service
	movzx	eax, al
	shl	eax, 2
	and	[ebp.Client_Flags], NOT CF_Mask ; assume okay
IFDEF DEBUG
;
; put pseudo random garbage in AX, to discourage people from assume that some-
; thing in it is of any value!
;
	push	eax
	VMMCall Get_System_Time
	or	eax, ebp
	dec	eax
	mov	[ebp.Client_AX], ax
	pop	eax
ENDIF
	jmp	cs:[DMA_services+eax]

bad_service:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], DMA_Func_Not_Supported
	ret	; carry cleared by OR above

DMA_services	label dword
	dd  OFFSET32 VDMAD_API_GetVersion   ;[2]
	dd  OFFSET32 VDMAD_API_Lock	    ;[3]
	dd  OFFSET32 VDMAD_API_Unlock	    ;[4]
	dd  OFFSET32 VDMAD_API_SLock	    ;[5]
	dd  OFFSET32 VDMAD_API_SUnlock	    ;[6]
	dd  OFFSET32 VDMAD_API_RequestBuf   ;[7]
	dd  OFFSET32 VDMAD_API_ReleaseBuf   ;[8]
	dd  OFFSET32 VDMAD_API_CopyToBuf    ;[9]
	dd  OFFSET32 VDMAD_API_CopyFromBuf  ;[A]
	dd  OFFSET32 VDMAD_API_DisableAuto  ;[B]
	dd  OFFSET32 VDMAD_API_EnableAuto   ;[C]

bad_flags:
	mov	[ebp.Client_AL], DMA_NonZero_Reserved_Flags
flag_bad:
	or	[ebp.Client_Flags], CF_Mask
	ret	; carry cleared by OR above

bad_addr:
	mov	[ebp.Client_AL], DMA_Invalid_Region
	jmp	flag_bad

EndProc VDMAD_DMA_Services


;******************************************************************************
;
;   VDMAD_API_GetVersion
;
;   DESCRIPTION:    Get Version information
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 02h
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			Client_AH = 1	   (Version 1.00 of specification)
;			Client_AL = 0
;			Client_BX = 3	   (Microsoft Windows/386 product #)
;			Client_CX = 200h   (Version 2.00 of implementation)
;			Client_SI:DI = maximum size of DMA buffer available
;			Client_DX = flags
;				    Bit 0 = 1 if PC/XT bus architecture
;				    Bit 1 = 1 if physical buffer/remp region in
;					      first 1Mb of memory
;				    Bit 2 = 1 if automatic remap supported
;				    Bit 3 = 1 if all memory physically contiguous
;
;   USES:	    EAX, Flags
;
;	The implementation version # (CX) was changed to 2.00 for Windows
;	3.1 because some vendors shipped different 3.0 VMDAD devices with
;	bug fixes and used implementation version numbers as high as 1.FFh.
;
;==============================================================================
BeginProc VDMAD_API_GetVersion
	test	[ebp.Client_DX], 0FFFFh ;Q: any flag bits set
	jnz	bad_flags		;   Y:

	mov	[ebp.Client_AX], 100h	; spec version 1.0
	mov	[ebp.Client_BX], 3	; Microsoft OEM #
	mov	[ebp.Client_CX], 200h	; MS version 2.00

	;
	;   Always return at least 128K-1 page for maximum size since we can
	;   now dynamically grow the buffer.  If the primary buffer is larger
	;   then report that.
	;
	mov	edx, [DMA_Buff_Table.DB_Size_Pages]
	mov	eax, 1FE00h
	shl	edx, 12
	cmp	eax, edx
	ja	UseMinSize
	mov	eax, edx
UseMinSize:
	mov	[ebp.Client_DI], ax
	shr	eax, 16
	mov	[ebp.Client_SI], ax

	xor	eax,eax 				; Assume not XT machine
ifndef	NEC_98
        cmp     [VDMAD_Machine_Type], PCXT_Machine      ; now check
        jne     not_PCXT                                ; 
	inc	eax					; set bit 1 if yes

not_PCXT:
endif

;
; set bit 1, if buffer in first 1Mb
;
	cmp	[DMA_Max_Physical], 100h
	ja	short not_1mb
	or	al, 10b
not_1mb:
	mov	[ebp.Client_DX], ax
	clc
	ret
EndProc VDMAD_API_GetVersion

;******************************************************************************
;
;   VDMAD_API_Lock
;
;   DESCRIPTION:    Attempt to lock a DMA region and optionally allocate a
;		    DMA buffer, if the region is not DMAable.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 03h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 1 = 1 if data should be copied into buffer
;					(ignored if bit 2 = 1)
;				Bit 2 = 1 if buffer should NOT be allocated
;					if lock would otherwise fail
;				Bit 3 = 1 if automatic remap should NOT be
;					attempted (fail, if region not contiguous)
;				Bit 4 = 1 if region must not overlap a 64Kb
;					boundary
;				Bit 5 = 1 if region must not overlap a 128Kb
;					boundary
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			Memory locked
;			Physical address field of the DDS contains the starting
;			  physical address of the region or buffer
;			The buffer ID field contains the ID of the allocated
;			  buffer, or 0 if no buffer
;		    Carry set in Client_Flags
;			Memory is not locked
;			Region size field of the DDS contains the maximum
;			  contiguous length in bytes.
;			Client_AL = 01h (DMA_Not_Contiguous)
;				    02h (DMA_Not_Aligned)
;				    03h (DMA_Lock_Failed)
;				    04h (DMA_No_Buffer)
;				    05h (DMA_Buffer_Too_Small)
;				    06h (DMA_Buffer_In_Use)
;				    07h (DMA_Invalid_Region)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_Lock
	test	[ebp.Client_DX], 1111111111000001b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	jc	bad_addr
	mov	ecx, [edi.DDS_size]
	jecxz	short al_get_phys
ifndef	NEC_98
;
; Some stupid CD-ROM device driver tries to lock 1000h bytes at 9F00:1E0
; So in V86 mode we try check if someone is trying to lock a mem region
; starting above lastvmpage (but below a0h) and crossing the a0000h linear
; address. If we detect such a lock we clip the lock to lock only upto
; a0000h
;
	test	[ebx].cb_vm_status, VMSTAT_PM_EXEC	; V86 mode ?
	jnz	@F					; no, no hacks

	movzx	eax, [edi.DDS_Seg]
	shl	eax, 4
	add	eax, [edi.DDS_Linear]		; eax start addr of region

	mov	edx, eax			; eax -> start addr of region
	shr	edx, 12				; lin to page
	cmp	dx, 0a0h			; above a0? (maybe in UMB)
	jae	@F				; yes, no hacks

	push	eax				; save start addr of region
	VMMCall	_GetLastV86Page
	cmp	edx, eax			; below lastvmpage ?
	pop	eax				; restore start addr of region
	jbe	@F				; yes, no hacks

	mov	edx, eax
	add	eax, ecx			; add len
	cmp	eax, 0a0000h			; does it cross a0000h
	jbe	@F				; no, no hacks

	mov	ecx, 0a0000h			; clip the length
	sub	ecx, edx			; so that the region does not
						;  cross a0000h
@@:
endif
	mov	[edi.DDS_bufferID], 0
	mov	[edi.DDS_physical], 0

	mov	dl, [ebp.Client_DL]
	and	dl, DMA_Align_64K OR DMA_Align_128K
	shr	dl, DMA_Align_64K_bit
.erre DMA_Align_128K_bit GT DMA_Align_64K_bit

	VxDCall VDMAD_Lock_DMA_Region
	jc	SHORT al_lock_failed
al_got_phys:
	mov	[edi.DDS_physical], edx
	jmp	SHORT al_exit

al_get_phys:
	push	eax				    ; reserve dword on stack
	mov	edx, esp			    ; EDX -> reserved dword
	mov	ecx, esi
	shr	ecx, 12 			    ; convert to lin pg #
	VMMCall _CopyPageTable <ecx, 1, edx, 0>     ; get single page table entry
	pop	edx				    ; edx = page table entry
	xor	ecx, ecx			    ; nothing locked
	test	dl, P_PRES			    ;Q: page present?
	jz	short al_lock_failed		    ;	N: fail call
	and	dx, 0F000h			    ; clear flag bytes
	and	esi, 0FFFh
	or	edx, esi			    ; edx = phys address
	jmp	al_got_phys

al_lock_failed:
	xchg	ecx, [edi.DDS_size]		    ; reload region size in ecx
						    ;	& update DDS with lockable size
	test	[ebp.Client_DL], DMA_No_Alloc_Buf   ;Q: get buffer?
	jnz	SHORT al_fail_call		    ;	N:

	VxDCall VDMAD_Request_Buffer		    ;	Y:
	jc	SHORT al_fail_call

	mov	[edi.DDS_size], ecx		    ; got buffer, so restore size
	mov	[edi.DDS_bufferID], bx
	mov	[edi.DDS_physical], edx

	test	[ebp.Client_DL], DMA_Buf_Copy	    ;Q: copy into buffer?
	jz	SHORT al_exit			    ;	N:

	xor	edi, edi
	VxDCall VDMAD_Copy_To_Buffer
	jnc	SHORT al_exit

al_fail_call:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], al

al_exit:
	clc
	ret

EndProc VDMAD_API_Lock

;******************************************************************************
;
;   VDMAD_API_Unlock
;
;   DESCRIPTION:    Unlock a previously locked region or release an
;		    automatically allocated DMA buffer.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 04h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 1 = 1 if data should be copied out of buffer
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			memory unlocked
;		    Carry set in Client_Flags
;			Client_AL = 08h (DMA_Region_Not_Locked)
;			Client_AL = 0Ah (DMA_Invalid_Buffer)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_Unlock
	test	[ebp.Client_DX], NOT DMA_Buf_Copy   ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	jc	bad_addr
	mov	ecx, [edi.DDS_size]

	movzx	ebx, [edi.DDS_bufferID]
	or	ebx, ebx			    ;Q: buffer allocated?
	jz	SHORT aul_unlock		    ;	N: unlock the region

	test	[ebp.Client_DL], DMA_Buf_Copy	    ;Q: copy from buffer?
	jz	SHORT aul_no_copy		    ;	N: just release it

	xor	edi, edi
	VxDCall VDMAD_Copy_From_Buffer		    ;	Y: copy from the buffer
	jc	SHORT aul_inv_buf		    ; jump if error
aul_no_copy:
	VxDCall VDMAD_Release_Buffer
	jnc	SHORT aul_exit			    ; done, if no error
aul_inv_buf:
	mov	[ebp.Client_AL], DMA_Invalid_Buffer
	jmp	SHORT aul_error

aul_unlock:
	VxDCall VDMAD_Unlock_DMA_Region
	jnc	SHORT aul_exit
	mov	[ebp.Client_AL], DMA_Region_Not_Locked

aul_error:
	or	[ebp.Client_Flags], CF_Mask
aul_exit:
	clc
	ret
EndProc VDMAD_API_Unlock

;******************************************************************************
;
;   VDMAD_API_SLock
;
;   DESCRIPTION:    This service is provided so that hardware devices that
;		    support automatic scatter/gather can determine the actual
;		    physical regions of and lock an entire linear address range
;		    in one call.  It can return a list of physical regions that
;		    make up the linear region, or it can return a copy of the
;		    page map entries that are spanned by the linear region.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 05h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 6 = 1 if page table entries should be copied
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			memory locked
;			The # used field in the DDS contains the number of
;			physical regions or page table entries filled in.
;			If bit 6 of DX was set, then BX is the offset into the
;			first page for the start of the region
;		    Carry set in Client_Flags
;			Client_AL = 03h (DMA_Lock_Failed)
;				    07h (DMA_Invalid_Region)
;				    09h (DMA_Table_Too_Small)
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_API_SLock
	test	[ebp.Client_DX], NOT (DMA_Get_PgTable OR DMA_Allow_NPs)
					    ;Q: any flag bits set
	jnz	bad_flags		    ;	Y:

	call	VDMAD_Get_DDS_ptrs	; (esi) = linear address to lock
					; (edi) = DDS structure
	jc	bad_addr
ifndef	NEC_98
ifdef TEKRAMHACK	; this is NOT defined, bug was blown off for Win95

; The "TekRam" scsi real-mode aspi driver it locks and unlocks
; a buffer in his device driver's global data segment with random
; large size values when doing verify track ioctls (and the random number gets
; bigger as the track gets bigger).  If we return error to him (say, because
; the large range needs too many scatter gather descriptor to fit in his
; parameter packet), he dies.  So to work around him we clip off any requests
; from v86 mode which straddle First_VM_Page at First_VM_Page.
; This will not fix the problem if the driver is loaded high, but at least
; it fixes the problem some of the time and gives a workaround that PSS can
; suggest.

	Assert_VM_Handle ebx
	TestMem [ebx].CB_VM_Status,VMSTAT_PM_EXEC	; V86 mode ?
	jnz	asl_noclip				; no, no hacks

	VMMCall _GetFirstV86Page		; (eax) = First_VM_Page
	shl	eax,PAGESHIFT			; (eax) = linear address
	add	eax,[ebx].CB_High_Linear	; (eax) = high linear of that
	sub	eax,esi 			; (eax) = bytes below 1st VM
	jc	asl_noclip			; if no bytes below, skip clip

	mov	ecx,[edi].DDS_size		; (ecx) = bytes to lock
	cmp	ecx,eax 			; is last byte on other side?
	jbe	asl_noclip			; if on same size, skip clip

	mov	[edi].DDS_size,eax		; clip off lock at First_VM_Page
asl_noclip:
endif
endif
	xchg	[edi.DDS_linear], esi
	xor	ecx, ecx
	xchg	cx, [edi.DDS_seg]	    ; cx:esi holds original linear adr

	mov	al, [ebp.Client_DL]
	and	al, DMA_Get_PgTable OR DMA_Allow_NPs
.erre DMA_Get_PgTable_bit GT DMA_SL_Get_PgTable_bit
	shr	al, DMA_Get_PgTable_bit - DMA_SL_Get_PgTable_bit
.errnz DMA_Allow_NPs_bit - DMA_Get_PgTable_bit - 1
.errnz DMA_SL_Allow_NPs_bit - DMA_SL_Get_PgTable_bit - 1
	push	esi			    ; save original linear address
	VxDCall VDMAD_Scatter_Lock
	jbe	short asl_00		    ; jump if C (lock failed) or
					    ; Z (whole region locked) set
	or	al, 100b		    ; don't mark locked pages as dirty
	VxDCall VDMAD_Scatter_Unlock
	mov	[edi.DDS_used], dx	    ; set # needed
	stc

asl_00:
	pop	[edi.DDS_linear]
	mov	[edi.DDS_seg], cx	    ; restore original linear
	jc	short asl_error
	test	[ebp.Client_DX], DMA_Get_PgTable
	jz	short asl_exit
	and	si, 0FFFh
	mov	[ebp.Client_BX], si	    ; return offset into first page
	jmp	short asl_exit

asl_error:
	or	[ebp.Client_Flags], CF_Mask
	movzx	edx, [edi.DDS_used]
	mov	[ebp.Client_AL], DMA_Lock_Failed
	cmp	dx, [edi.DDS_avail]
	jbe	short asl_exit
	mov	[ebp.Client_AL], DMA_Table_Too_Small

asl_exit:
	clc
	ret

EndProc VDMAD_API_SLock

;******************************************************************************
;
;   VDMAD_API_SUnlock
;
;   DESCRIPTION:    Unlock a previous scatter/gather locked region.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 06h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			memory unlocked
;		    Carry set in Client_Flags
;			Client_AL = 08h (DMA_Region_Not_Locked)
;
;   USES:
;
;==============================================================================
;
; This comment left over from Win 3.0 development:
;   define new bit to flag that pages should not be marked as dirty
; This requires a change to published API.
;


BeginProc VDMAD_API_SUnlock
	test	[ebp.Client_DX], NOT (DMA_Get_PgTable OR DMA_Allow_NPs)
					    ;Q: any flag bits set
	jnz	bad_flags		    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	jc	bad_addr
ifndef	NEC_98
ifdef TEKRAMHACK	; this is NOT defined, bug was blown off for Win95

; The "TekRam" scsi real-mode aspi driver it locks and unlocks
; a buffer in his device driver's global data segment with random
; large size values when doing verify track ioctls (and the random number gets
; bigger as the track gets bigger).  If we return error to him (say, because
; the large range needs too many scatter gather descriptor to fit in his
; parameter packet), he dies.  So to work around him we clip off any requests
; from v86 mode which straddle First_VM_Page at First_VM_Page.
;

	Assert_VM_Handle ebx
	TestMem [ebx].CB_VM_Status,VMSTAT_PM_EXEC	; V86 mode ?
	jnz	asu_noclip				; no, no hacks

	VMMCall _GetFirstV86Page		; (eax) = First_VM_Page
	shl	eax,PAGESHIFT			; (eax) = linear address
	add	eax,[ebx].CB_High_Linear	; (eax) = high linear of that
	sub	eax,esi 			; (eax) = bytes below 1st VM
	jc	asu_noclip			; if no bytes below, skip clip

	mov	ecx,[edi].DDS_size		; (ecx) = bytes to lock
	cmp	ecx,eax 			; is last byte on other side?
	jbe	asu_noclip			; if on same size, skip clip

	mov	[edi].DDS_size,eax		; clip off lock at First_VM_Page
asu_noclip:
endif
endif
	xchg	[edi.DDS_linear], esi
	xor	cx, cx
	xchg	cx, [edi.DDS_seg]	    ; cx:esi holds original linear adr

	mov	al, [ebp.Client_DL]
	and	al, DMA_Get_PgTable OR DMA_Allow_NPs
.erre DMA_Get_PgTable_bit GT DMA_SL_Get_PgTable_bit
	shr	al, DMA_Get_PgTable_bit - DMA_SL_Get_PgTable_bit
.errnz DMA_Allow_NPs_bit - DMA_Get_PgTable_bit - 1
.errnz DMA_SL_Allow_NPs_bit - DMA_SL_Get_PgTable_bit - 1
	VxDCall VDMAD_Scatter_Unlock

	mov	[edi.DDS_seg], cx	    ; restore original linear
	mov	[edi.DDS_linear], esi
	jnc	short asu_exit
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], DMA_Region_Not_Locked

asu_exit:
	clc
	ret

EndProc VDMAD_API_SUnlock

;******************************************************************************
;
;   VDMAD_API_RequestBuf
;
;   DESCRIPTION:    Request a DMA region to perform DMA transfers to/from.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 07h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 1 = 1 if data should be copied into buffer
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			Physical address field of the DDS contains the starting
;			physical address of the buffer.  The Buffer ID field
;			contains the ID of the allocated buffer
;		    Carry set in Client_Flags
;			Client_AL = 04h (DMA_No_Buffer)
;				    05h (DMA_Buffer_Too_Small)
;				    06h (DMA_Buffer_In_Use)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_RequestBuf
	test	[ebp.Client_DX], NOT DMA_Buf_Copy   ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	jc	bad_addr
	mov	ecx, [edi.DDS_size]
	VxDCall VDMAD_Request_Buffer
	jc	short req_buf_err

	mov	[edi.DDS_bufferID], bx
	mov	[edi.DDS_physical], edx

	test	[ebp.Client_DX], DMA_Buf_Copy	;Q: copy into buffer?
	jz	short req_buf_exit		;   N:

	xor	edi, edi
	VxDCall VDMAD_Copy_To_Buffer
	jnc	short req_buf_exit

req_buf_err:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], al

req_buf_exit:
	clc
	ret
EndProc VDMAD_API_RequestBuf

;******************************************************************************
;
;   VDMAD_API_ReleaseBuf
;
;   DESCRIPTION:    Release a previously requested DMA buffer.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 08h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 1 = 1 if data should be copied out of buffer
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			buffer released
;		    Carry set in Client_Flags
;			Client_AL = 0Ah (DMA_Invalid_Buffer)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_ReleaseBuf
	test	[ebp.Client_DX], NOT DMA_Buf_Copy   ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	jc	bad_addr

	movzx	ebx, [edi.DDS_bufferID]

	test	[ebp.Client_DX], DMA_Buf_Copy
	jz	short rel_buf_no_copy
	mov	ecx, [edi.DDS_size]
	xor	edi, edi
	VxDCall VDMAD_Copy_From_Buffer
	jc	short rel_buf_err
rel_buf_no_copy:
	VxDCall VDMAD_Release_Buffer
	jnc	short rel_buf_exit

rel_buf_err:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], DMA_Invalid_Buffer

rel_buf_exit:
	clc
	ret
EndProc VDMAD_API_ReleaseBuf

;******************************************************************************
;
;   VDMAD_API_CopyToBuf
;
;   DESCRIPTION:    Copy from the original DMA region into the allocated DMA
;		    buffer.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 09h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;		    Client_ES:DI -> DMA Descriptor Structure
;		    Client_BX:CX = starting offset in DMA buffer to copy
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			copy successful
;		    Carry set in Client_Flags
;			Client_AL = 0Ah (DMA_Invalid_Buffer)
;				    0Bh (DMA_Copy_Out_Range)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_CopyToBuf
	test	[ebp.Client_DX], 1111111111111111b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	jc	bad_addr
	movzx	ebx, [edi.DDS_bufferID]
	mov	ecx, [edi.DDS_size]
	mov	di, [ebp.Client_BX]
	shl	edi, 16
	mov	di, [ebp.Client_CX]
	VxDCall VDMAD_Copy_To_Buffer
	jnc	short copyto_exit

	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], al

copyto_exit:
	clc
	ret
EndProc VDMAD_API_CopyToBuf

;******************************************************************************
;
;   VDMAD_API_CopyFromBuf
;
;   DESCRIPTION:    Copy from the allocated DMA buffer into the original DMA
;		    region.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 0Ah
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;		    Client_ES:DI -> DMA Descriptor Structure
;		    Client_BX:CX = starting offset in DMA buffer to copy
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			copy successful
;		    Carry set in Client_Flags
;			Client_AL = 0Ah (DMA_Invalid_Buffer)
;				    0Bh (DMA_Copy_Out_Range)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_CopyFromBuf
	test	[ebp.Client_DX], 1111111111111111b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	jc	bad_addr
	movzx	ebx, [edi.DDS_bufferID]
	mov	ecx, [edi.DDS_size]
	mov	di, [ebp.Client_BX]
	shl	edi, 16
	mov	di, [ebp.Client_CX]
	VxDCall VDMAD_Copy_From_Buffer
	jnc	short copyfrom_exit

	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], al

copyfrom_exit:
	clc
	ret
EndProc VDMAD_API_CopyFromBuf

;******************************************************************************
;
;   VDMAD_API_DisableAuto
;
;   DESCRIPTION:    Disable automatic DMA remapping.
;
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 0Ch
;		    Client_BX = DMA channel #
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			enable successful
;		    Carry set in Client_Flags
;			Client_AL = 0Ch (DMA_Invalid_Channel)
;				    0Dh (DMA_Disable_Cnt_Overflow)
;
;   USES:	    EAX, Flags
;
;==============================================================================
BeginProc VDMAD_API_DisableAuto
	test	[ebp.Client_DX], 1111111111111111b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	movzx	eax, [ebp.Client_BX]
	call	VDMAD_Get_DMA_Handle
	jc	SHORT disable_bad_channel
	VxDCall VDMAD_Disable_Translation
	jnc	SHORT disable_exit
	mov	[ebp.Client_AL], DMA_Disable_Cnt_Overflow
	jmp	SHORT disable_error

disable_bad_channel:
	mov	[ebp.Client_AL], DMA_Invalid_Channel
disable_error:
	or	[ebp.Client_Flags], CF_Mask
disable_exit:
	clc
	ret
EndProc VDMAD_API_DisableAuto

;******************************************************************************
;
;   VDMAD_API_EnableAuto
;
;   DESCRIPTION:    This service must be called after calling
;		    VDMAD_API_DisableAuto to re-enable automatic DMA remapping.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 0Ch
;		    Client_BX = DMA channel #
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			enable successful
;		    Carry set in Client_Flags
;			Client_AL = 0Ch (DMA_Invalid_Channel)
;				    0Eh (DMA_Disable_Cnt_Underflow)
;
;   USES:	    EAX, Flags
;
;==============================================================================
BeginProc VDMAD_API_EnableAuto
	test	[ebp.Client_DX], 1111111111111111b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	movzx	eax, [ebp.Client_BX]
	call	VDMAD_Get_DMA_Handle
	jc	SHORT enable_bad_channel
	VxDCall VDMAD_Enable_Translation
	jnc	SHORT enable_exit
	mov	[ebp.Client_AL], DMA_Disable_Cnt_Underflow
	jmp	SHORT enable_error

enable_bad_channel:
	mov	[ebp.Client_AL], DMA_Invalid_Channel
enable_error:
	or	[ebp.Client_Flags], CF_Mask
enable_exit:
	clc
	ret
EndProc VDMAD_API_EnableAuto


;******************************************************************************
;
;   VDMAD_Get_DDS_ptrs
;
;   DESCRIPTION:    retrieve linear pointers to DDS and DMA region indentified
;		    in DDS
;
;   ENTRY:	    EBP -> Client frame
;
;   EXIT:	    EDI -> DDS
;		    ESI -> DMA region indentified in DDS
;
;   USES:	    EAX, Flags
;
;==============================================================================
BeginProc VDMAD_Get_DDS_ptrs

	Client_Ptr_Flat edi, ES, DI

	mov	esi, [edi.DDS_linear]
	movzx	eax, [edi.DDS_seg]
	or	eax, eax
	jz	short getDDS_no_DDS_xlat

	test	[ebp.Client_EFlags], VM_Mask
	jnz	short adr_valid
	push	ecx
	push	esi
	add	esi, [edi.DDS_size]
	dec	esi			; end_of_region = base + size - 1
	lsl	ecx, eax
	cmp	ecx, esi
	pop	esi
	pop	ecx
	jb	short getDDS_exit	; error: end of region > sel limit
adr_valid:
	push	dword ptr [ebp.Client_ES]
	mov	[ebp.Client_ES], ax
	Client_Ptr_Flat eax, ES
	add	esi, eax		; adjust edi with segment/selector base
	pop	dword ptr [ebp.Client_ES]

getDDS_no_DDS_xlat:
	clc
getDDS_exit:
	ret

EndProc VDMAD_Get_DDS_ptrs


VxD_CODE_ENDS

	END
