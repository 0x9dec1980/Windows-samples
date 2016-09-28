PAGE 58,132
;******************************************************************************
TITLE vdmadps2.asm -
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1989
;
;   Title:	vdmadps2.asm -
;
;   Version:	1.00
;
;   Date:	17-Aug-1989
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   17-Aug-1989 RAP
;
;==============================================================================

	.386p

;******************************************************************************
;			      I N C L U D E S
;******************************************************************************

.XLIST
	include VMM.INC

	include VDMAD.INC
	include DMASYS.INC

	include Debug.INC
.LIST


VxD_IDATA_SEG

; IO ports specific to PS2's with extended DMA capabilities, these ports are
; only trapped, if PS2 with micro channel is detected
;
ifndef NEC_98 ; NEC need not this code for PS/2.
Begin_VxD_IO_Table VDMAD_PS2_IO_Table
	VxD_IO	DMA_XFN, VDMAD_RegFunc
	VxD_IO	DMA_EXE, VDMAD_ExecFunc
End_VxD_IO_Table VDMAD_PS2_IO_Table
endif ; NEC_98 ; NEC need not this code for PS/2.

VxD_IDATA_ENDS


VxD_DATA_SEG

EXTRN VDMAD_CB_Offset:DWORD
EXTRN VDMAD_Check_TC:DWORD
EXTRN DMA_consts:BYTE
EXTRN DMA_Channels:BYTE
EXTRN DMA_Ctrl1:BYTE
EXTRN DMA_Ctrl2:BYTE

VxD_DATA_ENDS

VxD_LOCKED_DATA_SEG
extrn VDMAD_Machine_Type:byte
VxD_LOCKED_DATA_ENDS

VxD_LOCKED_CODE_SEG
IFDEF DEBUG
extrn VDMAD_Validate_Handle:near
ENDIF
VxD_LOCKED_CODE_ENDS

ifndef NEC_98 ; NEC need not this code for PS/2.
VxD_ICODE_SEG

;******************************************************************************
;
;   VDMAD_PS2_Device_Init
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

BeginProc VDMAD_PS2_Device_Init

; hook PS2 specific DMA ports
;
	mov	edi, OFFSET32 VDMAD_PS2_IO_Table
	VMMCall Install_Mult_IO_Handlers    ; Install the PS2 port traps

; assign PS2 specific Set_Phys_State service handler

	mov	eax, @@VDMAD_Set_Phys_State
	mov	esi, OFFSET32 VDMAD_Set_PS2_Phys_State
	VMMCall Hook_Device_Service
	ret

EndProc VDMAD_PS2_Device_Init


VxD_ICODE_ENDS
endif ; NEC_98 ; NEC need not this code for PS/2.

VxD_Pageable_Code_Seg

BeginDoc
;******************************************************************************
;
; VDMAD_Set_IO_Address
;
; DESCRIPTION:
;	Given a channel number, and the IO port base, sets it. It will be
;	used in the programmed IO mode.
;
; ENTRY:
;	EAX = channel number
;	EDX = IO port
;
; EXIT:
;	None.
;
; USES:
;	None.
;==============================================================================
EndDoc
BeginProc VDMAD_Set_IO_Address, Service

	push	eax
	and	eax,3
	call	VDMAD_Get_DMA_Handle	; EAX -> DMA handle
	mov	[eax.IO_Port],dx
	pop	eax
	ret

EndProc VDMAD_Set_IO_Address

VxD_Pageable_Code_Ends

VxD_CODE_SEG

EXTRN VDMAD_Bad_Read:NEAR
EXTRN VDMAD_Notify_all:NEAR
EXTRN VDMAD_NotifyOwner:NEAR
EXTRN VDMAD_Get_DMA_Handle:NEAR

ifndef NEC_98 ; NEC need not this code for PS/2.
;******************************************************************************
;
;   VDMAD_Read_PS2_Mode
;
;   DESCRIPTION:    Read current mode setting for channel
;
;   ENTRY:	    EAX = channel #
;		    ESI -> DMA_Channel_Data
;
;   EXIT:
;
;   USES:	    EAX, flags
;
;==============================================================================
BeginProc VDMAD_Read_PS2_Mode

	or	al, Set_Mode
	out	DMA_XFN, al
	IO_Delay
	in	al, DMA_EXE
	mov	[esi.ext_mode], al

    ;
    ; copy undocumented autoinit bit
    ;
	push	ecx
	mov	cl, al
	and	cl, PS2_AutoInit
	shl	cl, 3
	and	ah, NOT DMA_AutoInit
	or	ah, cl
	pop	ecx

	test	al, Transfer_data	    ;Q: verify op?
	jz	short init_no_transfer	    ;	Y:
	test	al, Write_Mem		    ;Q: write memory?
	jnz	short init_PS2_write	    ;	Y:
	or	ah, DMA_type_read	    ;	N:
	jmp	short init_PS2_read
init_PS2_write:
	or	ah, DMA_type_write
init_PS2_read:
init_no_transfer:
	mov	[esi.mode], ah
	ret

EndProc VDMAD_Read_PS2_Mode


;******************************************************************************
;
;   VDMAD_PS2_Read_Channels
;
;   DESCRIPTION:    read current modes for all channels
;
;   ENTRY:	    EBX = Handle of VM being initialized
;
;   EXIT:
;
;   USES:	    EAX, ECX, EDI, flags
;
;==============================================================================
BeginProc VDMAD_PS2_Read_Channels

	mov	ecx, 8
	xor	edi, edi
	mov	esi, OFFSET32 DMA_Channels

init_chnls:

	mov	eax, edi
	call	VDMAD_Read_PS2_Mode

;
; Read in the current address
;        
        mov     eax, edi
        or      al, Get_Mem_Adr                 ; read address
        out     DMA_XFN, al
        IO_Delay
        in      al, DMA_EXE                     ; low byte
        ror     eax, 8
        in      al, DMA_EXE                     ; middle byte
        ror     eax, 8
        in      al, DMA_EXE                     ; high byte
        ror     eax, 16                         ; realign on bit 0 boundary
        mov     [esi.cur_addr], eax             ; store result
        mov     [esi.pgm_addr], eax
;        
; Read in the current count
;
        mov     eax, edi
        or      al, Get_Count                   ; read count
        out     DMA_XFN, al
        IO_Delay
        in      al, DMA_EXE                     ; low byte
        shl     ax, 8
        in      al, DMA_EXE                     ; high byte
        xchg    ah, al                          ; realign word
        mov     [esi.cur_count], eax            ; store result
        mov     [esi.pgm_count], eax

	inc	edi
	add	esi, SIZE DMA_Channel_Data
	loop	init_chnls

	clc
	ret

EndProc VDMAD_PS2_Read_Channels


;******************************************************************************
;
;   VDMAD_Set_PS2_Phys_State
;
;   DESCRIPTION:    PS2 specific version of VDMAD_Set_Phys_State, this version
;		    is only used if a PS2 with micro channel is detected at
;		    init time, then its address is written over the address
;		    of VDMAD_Set_Phys_State in the VDMAD service table.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM handle
;		    DL	= mode
;		    DH	= extended mode
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================

BeginProc VDMAD_Set_PS2_Phys_State, ASYNC_SERVICE

	pushad

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

    ;
    ; verify that mode and ext mode are properly in sync
    ;
	mov	ch, dl			    ; build a new mode in ch
    ;
    ; first copy undocumented auto-init bit
    ;
	mov	cl, dh
	and	cl, PS2_AutoInit
	shl	cl, 3
	and	ch, NOT DMA_AutoInit
	or	ch, cl
    ;
    ; copy memory access bits
    ;
	and	ch, NOT (DMA_type_write+DMA_type_read)
	test	dh, Transfer_data	    ;Q: verify op?
	jz	short sps_no_transfer	    ;	Y:
	test	dh, Write_Mem		    ;Q: write memory?
	jnz	short sps_PS2_write	    ;	Y:
	or	ch, DMA_type_read	    ;	N:
	jmp	short sps_PS2_read
sps_PS2_write:
	or	ch, DMA_type_write
sps_PS2_read:
	and	ch, NOT DMA_type_verify
sps_no_transfer:
IFDEF DEBUG
	cmp	ch, dl			    ;Q: modes in sync?
	Debug_OutNZ "VDMAD_Set_PS2_Phys_State mode and ext-mode not in sync"
ENDIF
	mov	dl, ch			    ; use calculated mode

IFDEF DEBUG
	; dump these in opposite order, because .S displays them backwards
	mov	ebx, [eax.region_size]
	dec	ebx
	DMA_Q_OUT "      cnt=#eax  mode=#bl", ebx, edx
	movzx	ebx, [eax.xfer_page]
	shl	ebx, 16
	mov	bx, [eax.xfer_base]
	DMA_Q_OUT "Set Phys State chn=#al  adr=#ebx", [eax.channel_num]
ENDIF
	mov	edi, eax		; EDI is now the DMA handle
	mov	ebx, edx		; BL = new mode & flags, BH = extended mode

IFDEF DEBUG
	cmp	[edi.locked_pages], 0	;Q: region locked?
	jne	short region_ok_2	;   Y:
	cmp	[edi.buffer_id], 0	;Q: buffer assigned?
	jne	short region_ok_2	;   Y:
	Debug_Out "VDMAD: Attempted to start DMA without locking a region"
	Fatal_Error

region_ok_2:
ENDIF

; use extended commands to program the DMA
	mov	eax, [edi.channel_num]
	or	al, Set_Mem_Adr
	out	DMA_XFN, al		; out command byte
	IO_Delay
IFDEF DEBUG
	mov	edx, eax
ENDIF
	movzx	eax, [edi.xfer_base]
	out	DMA_EXE, al		; out low base byte
	IO_Delay
	mov	al, ah
	out	DMA_EXE, al		; out high base byte
	IO_Delay
	mov	ax, [edi.xfer_page]
	out	DMA_EXE, al		; out page byte
	IO_Delay

;
; output old mode
;
	mov	eax, [edi.channel_num]
	mov	edx, eax
	and	al, DMA_chan_sel
	and	bl, NOT DMA_chan_sel
	or	al, bl			; add channel # to MODE command
	Cntrl_Const_Offset edx
	movzx	edx, [edx+DMA_consts.DMA_mode_port]
	out	dx, al			; write new mode
	IO_Delay

;
; output extended mode
;
	mov	eax, [edi.channel_num]
	or	al, Set_Mode
	out	DMA_XFN, al
	IO_Delay
	mov	al, bh
	out	DMA_EXE, al		; out new extended mode

;
; output count
;
	mov	edx, [edi.region_size]
	dec	edx

	test	bh, _16_bit_xfer	;Q: word transfer on this channel?
	jz	short spps2_byte_xfer	;   N: use # of bytes
	shr	edx, 1			;   Y: convert to # of words
spps2_byte_xfer:
	mov	eax, [edi.channel_num]
	or	al, Set_Count
	out	DMA_XFN, al		; out command byte
	IO_Delay
	mov	eax, edx
	out	DMA_EXE, al		; out low count byte
	IO_Delay
	mov	al, ah
	out	DMA_EXE, al		; out high count byte
	IO_Delay

;
; handle progammed I/O mode in PS2 extended DMA
;
	test	bh, Programmed_IO	;Q: programmed I/O mode?
	jz	short not_IO_mode	;   N:
	mov	eax, [edi.channel_num]

; get I/O from controller/channel data

	movzx	ebx, [edi.IO_port]

spps_ctrl_1:
	or	al, Set_IO_Adr
	out	DMA_XFN, al		; out command byte
	IO_Delay
	mov	eax, ebx
	DMA_Q_OUT "VDMAD: Set I/O address #ax"
	out	DMA_EXE, al		; out low count byte
	IO_Delay
	mov	al, ah
	out	DMA_EXE, al		; out high count byte
	IO_Delay
not_IO_mode:
	popad
	ret

EndProc VDMAD_Set_PS2_Phys_State

;******************************************************************************
;
;   VDMAD_RegFunc
;
;   DESCRIPTION:    Handle trapped I/O to the PS/2's extended register DMA port
;
;   ENTRY:	    AL = Byte to be output (if output)
;		    EBX = VM handle
;		    EDX = Port address
;		    ECX = 0 for byte input, 4 for byte output, others emulated
;
;   EXIT:	    AL = Byte input (if input)
;
;   USES:	    Everything
;
;==============================================================================
BeginProc VDMAD_RegFunc

	call	[VDMAD_Check_TC]
	Dispatch_Byte_IO VDMAD_Bad_Read, Fall_Through

VDMAD_SetReg:
	mov	esi, OFFSET32 DMA_Ctrl1 ; set esi to point to DMA data
	movzx	edx, al
	and	eax, Channel_Mask
	cmp	al, 4			;Q: 2nd controller?
	jb	short rf_1st_ctl	;   N:
	add	esi, SIZE DMA_Controller_State	; point to 2nd controller data
rf_1st_ctl:
	mov	ecx, edx
	and	ecx, Function_Mask
	shr	ecx, 2
	jmp	cs:[ecx+FuncHandlers]

FuncHandlers	label dword
	dd	OFFSET32 PS2_DMA_Set_IO_Adr	  ; 0
	dd	OFFSET32 PS2_DMA_Bad_Func	  ; 1
	dd	OFFSET32 PS2_DMA_Set_Mem_Adr	  ; 2
	dd	OFFSET32 PS2_DMA_Get_Mem_Adr	  ; 3
	dd	OFFSET32 PS2_DMA_Set_Count	  ; 4
	dd	OFFSET32 PS2_DMA_Get_Count	  ; 5
	dd	OFFSET32 PS2_DMA_Get_Status	  ; 6
	dd	OFFSET32 PS2_DMA_Set_Mode	  ; 7
	dd	OFFSET32 PS2_DMA_Set_Arbus	  ; 8
	dd	OFFSET32 PS2_DMA_Set_Chn_Mask	  ; 9
	dd	OFFSET32 PS2_DMA_Reset_Chn_Mask   ; A
	dd	OFFSET32 PS2_DMA_Bad_Func	  ; B
	dd	OFFSET32 PS2_DMA_Bad_Func	  ; C
	dd	OFFSET32 PS2_DMA_Master_Clear	  ; D
	dd	OFFSET32 PS2_DMA_Bad_Func	  ; E
	dd	OFFSET32 PS2_DMA_Bad_Func	  ; F

PS2_DMA_Bad_Func:
IFDEF DEBUG
	Debug_Out "VDMAD:  Illegal PS/2 extended DMA function #al"
ENDIF
	ret
EndProc VDMAD_RegFunc

;******************************************************************************
;
;   VDMAD_ExecFunc
;
;   DESCRIPTION:    Handle trapped I/O to the PS/2's extended function DMA port
;
;   ENTRY:	    AL = Byte to be output (if output)
;		    EBX = VM handle
;		    EDX = Port address
;		    ECX = 0 for byte input, 4 for byte output, others emulated
;
;   EXIT:	    AL = Byte input (if input)
;
;   USES:	    Everything
;
;==============================================================================
BeginProc VDMAD_ExecFunc

	call	[VDMAD_Check_TC]
	mov	esi, ebx
	add	esi, [VDMAD_CB_OFFSET]	    ; esi points to DMA data
	Dispatch_Byte_IO Fall_Through, <SHORT PS2_Put_DMA_Byte>

;
; Get next available DMA data byte
;
PS2_Get_DMA_Byte:
	movzx	edx, [esi.DMA_PS2_cmd]
	and	edx, Function_Mask
	cmp	edx, Set_Arbus		    ;Q: writing arbus?
	je	short read_arbus

	movzx	ebx, [esi.DMA_bytePtr]
	mov	al, byte ptr [ebx+esi.DMA_data]
	jmp	PS2_Inc_Byte_ptr

read_arbus:
	mov	al, [esi.DMA_PS2_cmd]
	out	DMA_XFN, al
	in	al, DMA_EXE
	jmp	PS2_write_done

;
; Put the data byte into Base, Page or Count field in DMA controller data
; based upon the setup from the last executed CMD written
; to the extended register port

PS2_Put_DMA_Byte:
	cmp	[esi.DMA_writing], VMM_TRUE     ;Q: writing?
	jne	bad_write		    ;	N:

	movzx	edx, [esi.DMA_PS2_cmd]
	and	edx, Function_Mask

	cmp	edx, Set_Mem_Adr	    ;Q: writing memory address?
	je	short PS2_New_Adr	    ;	Y:
	cmp	edx, Set_Count		    ;Q: writing count?
	je	SHORT PS2_New_Cnt	    ;	Y:
	cmp	edx, Set_IO_Adr 	    ;Q: writing I/O address?
	je	SHORT PS2_New_IO_Adr	    ;	Y:
	cmp	edx, Set_Mode		    ;Q: writing mode?
	je	SHORT PS2_New_Mode	    ;	Y:
	cmp	edx, Set_Arbus		    ;Q: writing arbus?
	jne	short bad_write 	    ;	N:

	mov	ah, al
	mov	al, [esi.DMA_PS2_cmd]
	DMA_Q_OUT "VDMAD: Setting Arbus #ax (value/cmd)"
	out	DMA_XFN, al
	mov	al, ah
	out	DMA_EXE, al
	jmp	SHORT PS2_write_done
;
; Update the channel's mode field
;
PS2_New_Mode:
	call	PS2_Get_Chl_Data_Ptr
	mov	[edi.ext_mode], al

;
; modify simulated 8237 mode with verify, read, write operation type
;
	mov	ah, [edi.mode]
    ;
    ; copy undocumented autoinit bit
    ;
	mov	cl, al
	and	cl, PS2_AutoInit
	shl	cl, 3
	and	ah, NOT DMA_AutoInit
	or	ah, cl

	and	ah, NOT (DMA_type_write+DMA_type_read)
	test	al, Transfer_data	    ;Q: verify op?
	jz	short no_transfer	    ;	Y:
	test	al, Write_Mem		    ;Q: write memory?
	jnz	short PS2_write 	    ;	Y:
	or	ah, DMA_type_read	    ;	N:
	jmp	short PS2_read
PS2_write:
	or	ah, DMA_type_write
PS2_read:
no_transfer:
	mov	[edi.mode], ah

	jmp	short PS2_write_done
;
; Update the channel's I/O address field
;
PS2_New_IO_Adr:
	call	PS2_Get_Chl_Data_Ptr
	lea	edi, [edi.IO_port]
	jmp	short PS2_update_field

;
; Update the channel's count field
;
PS2_New_Cnt:
	call	PS2_Get_Chl_Data_Ptr
	lea	edi, [edi.cur_count]
PS2_update_field:
	movzx	ebx, [esi.DMA_bytePtr]
	mov	byte ptr [ebx+edi], al	    ; store count or I/O port byte
	mov	byte ptr [ebx+edi+4], al
	.errnz	pgm_count - cur_count - 4
	jmp	short PS2_Inc_Byte_Ptr

;
; Update the channel's base or page fields
;
PS2_New_Adr:
	call	PS2_Get_Chl_Data_Ptr
	movzx	ebx, [esi.DMA_bytePtr]
	lea	edi, [edi.cur_addr]
	mov	byte ptr [ebx+edi], al	    ; store adr byte into adrs
	mov	byte ptr [ebx+edi+4], al
	.errnz	pgm_addr - cur_addr - 4

;
; Increment the virtual byte ptr for the controller
;
PS2_Inc_Byte_Ptr:
	inc	ebx
	cmp	bl, [esi.DMA_dataBytes]     ;Q: beyond available?
	jb	short next_ok		    ;	N:
	xor	ebx, ebx		    ;	Y: reset pointer
next_ok:
	mov	[esi.DMA_bytePtr], bl	    ; save next byte pointer
PS2_write_done:
	ret

bad_write:
	Debug_Out "VDMAD: Attempted to write to port 1Ah"
	ret

EndProc VDMAD_ExecFunc

;******************************************************************************
;
;   PS2_Get_Chl_Data_Ptr
;
;   DESCRIPTION:    Get pointer to 8237 channel data
;
;   ENTRY:	    EBX is VM handle
;
;   EXIT:	    EDI points to DMA channel data
;
;   USES:
;
;==============================================================================
BeginProc PS2_Get_Chl_Data_Ptr

	push	eax
	mov	edi, ebx
	add	edi, [VDMAD_CB_Offset]
	movzx	eax, [edi.DMA_PS2_cmd]
	and	al, Channel_Mask
	call	VDMAD_Get_DMA_Handle
	mov	edi, eax
	pop	eax
	ret

EndProc PS2_Get_Chl_Data_Ptr
endif ; NEC_98


;******************************************************************************
;
;   PS2_DMA_Set_IO_Adr
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function by setting
;		    up state in PS/2 specific data for subsequent writes to
;		    the extended function port
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_DMA_Set_IO_Adr

	and	al, 3
	call	VDMAD_Get_DMA_Handle
	movzx	eax, [eax.IO_port]	; eax = current port addr
	mov	cl, 2
	mov	ch, VMM_TRUE
	CallRet PS2_Queue_DMA_Data

EndProc PS2_DMA_Set_IO_Adr

ifndef NEC_98 ; NEC need not this code for PS/2.
;******************************************************************************
;
;   PS2_DMA_Set_Mem_Adr
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function by setting
;		    up state in PS/2 specific data for subsequent writes to
;		    the extended function port
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_DMA_Set_Mem_Adr

	mov	ch, VMM_TRUE
	jmp	short PS_DMA_GetSet_Mem_Adr

EndProc PS2_DMA_Set_Mem_Adr

;******************************************************************************
;
;   PS2_DMA_Get_Mem_Adr
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_DMA_Get_Mem_Adr

	mov	ch, FALSE
PS_DMA_GetSet_Mem_Adr:
	and	al, 3
	call	VDMAD_Get_DMA_Handle
	mov	eax, [eax.cur_addr]	    ; eax = current addr
	mov	cl, 3
	CallRet PS2_Queue_DMA_Data

EndProc PS2_DMA_Get_Mem_Adr

;******************************************************************************
;
;   PS2_DMA_Set_Count
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function by setting
;		    up state in PS/2 specific data for subsequent writes to
;		    the extended function port
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_DMA_Set_Count

	mov	ch, VMM_TRUE
	jmp	short PS_DMA_GetSet_Count

EndProc PS2_DMA_Set_Count

;******************************************************************************
;
;   PS2_DMA_Get_Count
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_DMA_Get_Count

	mov	ch, FALSE
PS_DMA_GetSet_Count:
	and	al, 3
	call	VDMAD_Get_DMA_Handle
	mov	eax, [eax.cur_count]	    ; eax = current count
	mov	cl, 2
	CallRet PS2_Queue_DMA_Data

EndProc PS2_DMA_Get_Count

;******************************************************************************
;
;   PS2_DMA_Get_Status
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_DMA_Get_Status

	xor	al, al
	xchg	al, [esi.CTL_status]
	mov	cl, 1
	mov	ch, FALSE
	CallRet PS2_Queue_DMA_Data

EndProc PS2_DMA_Get_Status

;******************************************************************************
;
;   PS2_DMA_Set_Mode
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc PS2_DMA_Set_Mode

	and	al, 3
	call	VDMAD_Get_DMA_Handle
	movzx	eax, [eax.ext_mode]	; eax = current mode
	mov	cl, 1
	mov	ch, VMM_TRUE
	CallRet PS2_Queue_DMA_Data

EndProc PS2_DMA_Set_Mode

;******************************************************************************
;
;   PS2_DMA_Set_Arbus
;
;   DESCRIPTION:    Virtually handle Arbus programming
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc PS2_DMA_Set_Arbus

	mov	esi, ebx
	add	esi, [VDMAD_CB_Offset]	; esi points to DMA data
	mov	[esi.DMA_PS2_cmd], dl	; save command byte
	mov	[esi.DMA_writing], VMM_TRUE
	ret

EndProc PS2_DMA_Set_Arbus


;******************************************************************************
;
;   PS2_DMA_Set_Chn_Mask
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_DMA_Set_Chn_Mask

	mov	ecx, eax
	and	cl, DMA_chan_sel
	bts	dword ptr [esi.CTL_mask], ecx
	mov	edx, 1
	CallRet VDMAD_NotifyOwner

EndProc PS2_DMA_Set_Chn_Mask

;******************************************************************************
;
;   PS2_DMA_Reset_Chn_Mask
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_DMA_Reset_Chn_Mask

	mov	ecx, eax
	and	cl, DMA_chan_sel
	btr	dword ptr [esi.CTL_mask], ecx
	mov	edx, 1
	CallRet VDMAD_NotifyOwner

EndProc PS2_DMA_Reset_Chn_Mask

;******************************************************************************
;
;   PS2_DMA_Master_Clear
;
;   DESCRIPTION:    Handle virtualizing the extended DMA function
;
;   ENTRY:	    EAX = channel #
;		    EBX = VM handle
;		    ESI points to controller data
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_DMA_Master_Clear

	Debug_Out "PS2 Master Clear"
	push	esi
	mov	esi, ebx
	add	esi, [VDMAD_CB_Offset]	; esi points to DMA data
	xor	eax, eax
	mov	[esi.DMA_PS2_cmd], al
	mov	[esi.DMA_writing], al
	mov	[esi.DMA_bytePtr], al
	mov	[esi.DMA_dataBytes], al
	mov	[esi.DMA_data], eax
	mov	word ptr [esi.DMA_flipflop], ax
	pop	esi

	mov	[DMA_Ctrl1.CTL_status], al
	mov	[DMA_Ctrl2.CTL_status], al
	mov	[DMA_Ctrl1.CTL_request], al
	mov	[DMA_Ctrl2.CTL_request], al

	mov	ecx, 8
	mov	eax, OFFSET32 DMA_Channels
mc_init_chnl:
	mov	[eax.mode], DMA_single_mode
	mov	[eax.ext_mode], 0
	cmp	cl, 4			; 8-5 init ctrl1, 4-1 init ctrl2
	ja	short @f
	mov	[eax.ext_mode], _16_bit_xfer
@@:	add	eax, SIZE DMA_Channel_Data
	loop	mc_init_chnl

	movzx	eax, [esi.CTL_mask]
	mov	[esi.CTL_mask], 0Fh	    ; mask all channels
	call	VDMAD_Notify_all	    ; notify chns 0-3

	add	esi, SIZE DMA_Controller_State ; point to 2nd controller's data
	movzx	eax, [esi.CTL_mask]
	mov	[esi.CTL_mask], 0Fh	    ; mask all channels
	call	VDMAD_Notify_all	    ; notify chns 4-7
	ret

EndProc PS2_DMA_Master_Clear
endif ; NEC_98


;******************************************************************************
;
;   PS2_Queue_DMA_Data
;
;   DESCRIPTION:    Queue DMA data into the PS/2 specific data fields.
;		    The data is then read from on subsequent
;		    virtualized reads from the extended function port
;
;   ENTRY:	    EAX is data  (1, 2 or 3 bytes)
;		    CL = # of bytes
;		    CH = write mode enable (TRUE or FALSE)
;		    EBX = VM handle
;		    EDX = command byte
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PS2_Queue_DMA_Data

	mov	esi, ebx
	add	esi, [VDMAD_CB_Offset]	; esi points to DMA data
	mov	[esi.DMA_PS2_cmd], dl	; save command byte
	mov	[esi.DMA_writing], ch
	mov	[esi.DMA_bytePtr], 0
	mov	[esi.DMA_dataBytes], cl
	mov	[esi.DMA_data], eax
	ret

EndProc PS2_Queue_DMA_Data

VxD_CODE_ENDS


END
