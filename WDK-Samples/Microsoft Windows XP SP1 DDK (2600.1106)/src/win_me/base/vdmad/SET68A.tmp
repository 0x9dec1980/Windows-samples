ARBIT = 1

PAGE 58,132
;******************************************************************************
TITLE vdmad.asm -
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1988-1991
;
;   Title:	vdmad.asm -
;
;   Version:	2.01
;
;   Date:	4-Feb-1991
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   15-Nov-1988 RAP Started complete rewrite
;   04-Feb-1991 RAP fixed misc bugs
;
;==============================================================================

	.386p

;==============================================================================
;;    rpMemTst = 1	  ; this equate conditionally generates code which
			; registers a VKD hot key "Ctrl+Alt+M" to force
			; page fragmentation by allocating a bunch of pages,
			; then deallocating every other one.  This was used to
			; better test buffered DMA.
;==============================================================================

;******************************************************************************
;			      I N C L U D E S
;******************************************************************************

.XLIST
	include VMM.INC
	include VPICD.INC

	Create_VDMAD_Service_Table EQU 1      ; VDMAD service table created
	include VDMAD.INC
	include DMASYS.INC

	include shell.inc
	include Debug.INC

IFDEF rpMemTst
%OUT including VKD.INC to declare a test hot key - used to fragment memory
	include VKD.INC
ENDIF

	include sysinfo.INC
        include MSGMACRO.INC
	include VDMADMSG.INC
	include CONFIGMG.INC
	include OptTest.Inc
	include pageswap.inc
	include pagefile.inc

ifdef	NEC_98
	include v86mmgr.Inc
endif

.LIST


BeginDoc
;******************************************************************************
;
;   VDMAD Virtual DMA Device
;
;   VDMAD virtualizes DMA I/O for standard DMA channels for all VM's.  By
;   default it handles all programmed I/O for the DMA controllers and
;   arbitrates I/O to the physical DMA ports so that more than one VM can
;   be using the same DMA channels at the same time.  In some cases the
;   default handling of DMA channels is not desirable.	To handle these cases,
;   VDMAD provides a number of services to allow another Windows/386 device
;   to take control of the virtualization of specific DMA channels.
;
;   VDMAD also provides some services which can be used by Bus Master devices
;   which have their own DMA controllers.  These devices still need to be able
;   to lock and unlock DMA regions in memory and determine the physical
;   addresses of these regions.  Bus Master devices can also make use of the
;   buffer services, if they can't otherwise scatter gather a linear region
;   which isn't physically contiguous.
;
;   VDMAD services available for Bus Master use are:
;
;		VDMAD_Create_Handle
;		VDMAD_Lock_DMA_Region
;		VDMAD_Unlock_DMA_Region
;		VDMAD_Scatter_Lock
;		VDMAD_Scatter_Unlock
;		VDMAD_Request_Buffer
;		VDMAD_Release_Buffer
;		VDMAD_Copy_To_Buffer
;		VDMAD_Copy_From_Buffer
;
;******************************************************************************
EndDoc


;******************************************************************************
;		 V I R T U A L	 D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VDMAD, 4, 0, VDMAD_Control, VDMAD_Device_ID, VDMAD_Init_Order

VxD_PAGEABLE_CODE_SEG

EXTRN VDMAD_PnP_New_DevNode:NEAR

VxD_PAGEABLE_CODE_ENDS


MinBufferSizeK	    equ 16	    ; default buffer of 16Kb (4 pages)
MinBufferSizeBytes  equ MinBufferSizeK * 400h
MinBufferSizePages  equ MinBufferSizeK / 4

;******************************************************************************
;******************************************************************************
; Initialization data


VxD_IDATA_SEG

EXTRN VDMAD_IO_Table:BYTE
ifdef	NEC_98
EXTRN VDMAD_IO_Table_Old:BYTE
endif
EXTRN VDMAD_Ini_Buffer_Size:BYTE
ifndef	NEC_98
EXTRN VDMAD_EISA_Size_Ini:BYTE
EXTRN VDMAD_MCA_Size_Ini:BYTE
EXTRN VDMAD_Ini_XT_Buffer:BYTE
endif
EXTRN VDMAD_Max_DMA_pg_Ini:BYTE

DMA_MinReq_SizeK    dd	0   ; 0 indicates none set.  Size is in Kbytes

VDMAD_SysCrit_Done  db	0

VxD_IDATA_ENDS

VxD_CODE_SEG

IFDEF ARBIT
EXTRN VDMADArb_Unmask_Channel:NEAR
EXTRN VDMADArb_Alloc_Channel:NEAR
EXTRN VDMADArb_Unalloc_Channel:NEAR
ENDIF

EXTRN VDMAD_NoCheck:NEAR
EXTRN VDMAD_IO_Check_TC:NEAR

VxD_CODE_ENDS


;******************************************************************************
;******************************************************************************
; Global data

VxD_PAGEABLE_DATA_SEG

EXTRN	VDMAD_Devnode_Handle:DWORD

VxD_PAGEABLE_DATA_ENDS

VxD_DATA_SEG

ifndef	NEC_98
EXTRN DMA_EISA_Ext_Modes:BYTE
endif

VDMAD_Copy_Exception Exception_Handler_Struc<,OFFSET32 VDMAD_Fault_Start,OFFSET32 VDMAD_Fault_End,OFFSET32 VDMAD_Copy_Fault>

VxD_DATA_ENDS


VxD_LOCKED_DATA_SEG

	PUBLIC VDMAD_CB_Offset
VDMAD_CB_Offset     dd	0

ifndef	NEC_98
EXTRN page_ports:BYTE
EXTRN base_ports:BYTE
EXTRN count_ports:BYTE
EXTRN EISA_high_page_ports:WORD
EXTRN EISA_high_count_ports:WORD
else
EXTRN page_ports:WORD
EXTRN base_ports:WORD
EXTRN count_ports:WORD
endif


	PUBLIC DMA_Channels
DMA_Channels	    label dword
    DMA_Channel_Data <0, OFFSET32 VDMAD_Call_Def>  ; Channel 0
    DMA_Channel_Data <1, OFFSET32 VDMAD_Call_Def>  ; Channel 1
    DMA_Channel_Data <2, OFFSET32 VDMAD_Call_Def>  ; Channel 2
    DMA_Channel_Data <3, OFFSET32 VDMAD_Call_Def>  ; Channel 3
DMA_Channels_2	    label dword
    DMA_Channel_Data <4, 0>                        ; Channel 4
    DMA_Channel_Data <5, OFFSET32 VDMAD_Call_Def>  ; Channel 5
    DMA_Channel_Data <6, OFFSET32 VDMAD_Call_Def>  ; Channel 6
    DMA_Channel_Data <7, OFFSET32 VDMAD_Call_Def>  ; Channel 7


DMA_Channels_Available	equ 8

	PUBLIC	DMA_Ctrl1, DMA_Ctrl2
DMA_Ctrl1	label	dword
    DMA_Controller_State	<>

DMA_Ctrl2	label	dword
    DMA_Controller_State	<>

.errnz DMA_Ctrl2 - DMA_Ctrl1 - (SIZE DMA_Controller_State)


	PUBLIC DMA_consts
DMA_consts label byte
DMA1 Controller_Const <OFFSET32 DMA_Channels, \
		       DMA1_Mode, DMA1_SingleMask, DMA1_SoftReq, DMA1_CLR_FF, \
		       0, 0>

DMA2 Controller_Const <OFFSET32 DMA_Channels_2, \
		       DMA2_Mode, DMA2_SingleMask, DMA2_SoftReq, DMA2_CLR_FF, \
		       0, 0>

DMA_FREE_TIMEOUT    EQU     60000	; 1 minute

NUM_DMA_BUFFS	    EQU     8

PUBLIC DMA_Buff_Table
PUBLIC VDMAD_Free_Buff_Timeout

DMA_Buff_Table db (SIZE DMA_Buff_Struc * NUM_DMA_BUFFS) dup (0)
End_DMA_Buff_Table LABEL BYTE

VDMAD_Free_Buff_Timeout dd  0

	PUBLIC	DMA_Max_Physical
; DMA_Max_Physical is the highest allowable DMA page address, based on the 
; bus architecture.  It is supposed to be a "less than" value, that is, DMA
; should be limited to address below this value.
ifdef NEC_98
DMA_Max_Physical    dd	1000h       ; Assume max value for starters 
else ;ifdef NEC_98
DMA_Max_Physical    dd	100000h     ; Assume max value for starters 
endif ;ifdef NEC_98


	PUBLIC VDMAD_Check_TC
VDMAD_Check_TC	    dd	0
VDMAD_Hw_Int_Next   dd	0	    ; address of next routine to receive the
				    ; VPICD Hardware Int notification; 0 if none
VDMAD_Hw_Int_Filter dd	0	    ; address of DMA check routine, if DMA in
				    ; progress, else same as VDMAD_Hw_Int_Next

DMA_disabled	dd  0		    ; bits 0-7 set if channel's DMA_disable_cnt
				    ;	non-zero
DMA_TC_event	dd  0		    ; event handle

	PUBLIC VDMAD_DMA1_status, VDMAD_DMA2_status

VDMAD_DMA1_status   db	0
VDMAD_DMA2_status   db	0

        PUBLIC VDMAD_Machine_Type
VDMAD_Machine_Type  db	ISA_Machine

	PUBLIC VDMAD_Rsvd_Mask
VDMAD_Rsvd_Mask db     00h			; Mask of reserved DMA channels

VxD_LOCKED_DATA_ENDS

;******************************************************************************
;******************************************************************************
; Initialization code

VxD_ICODE_SEG

IFDEF ARBIT
EXTRN VDMAD_PnP_Init:NEAR
EXTRN VDMADArb_Init_Complete:NEAR
ENDIF
ifndef	NEC_98
EXTRN VDMAD_PS2_Device_Init:NEAR
endif
EXTRN VDMAD_API_Device_Init:NEAR
ifndef	NEC_98
EXTRN VDMAD_EISA_Init:NEAR
endif

;******************************************************************************
;
;   VDMAD_Set_Req_Buff_SizeK
;
;   DESCRIPTION:
;	Called to set the minimum requested buffer size during sys crit init.
;
;   ENTRY:
;	EAX = Minimum requested size in K
;	NOTE:  If EAX = 0 then no minimum size buffer will be requested.
;
;   EXIT:
;	EAX may be modified
;
;   USES:
;	EAX, Flags
;
;==============================================================================

BeginProc VDMAD_Set_Req_Buff_SizeK

	cmp	eax, [DMA_MinReq_SizeK]     ; Will ignore 0 size if no min
	jbe	VSRBS_Exit		    ; has been set yet

	cmp	eax, MinBufferSizeK
	jae	not_too_small
	mov	eax, MinBufferSizeK
not_too_small:
	mov	[DMA_MinReq_SizeK], eax
VSRBS_Exit:
	ret

EndProc VDMAD_Set_Req_Buff_SizeK


;******************************************************************************
;
;   VDMAD_Sys_Crit_Init
;
;   DESCRIPTION:    Check INI file for a VDMAD buffer size specification
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Sys_Crit_Init

IFDEF ARBIT
        call    VDMAD_PnP_Init
ENDIF

	mov	[VDMAD_SysCrit_Done], VMM_TRUE

	in	al, DMA1_Status 	; Read DMA controller status regs
ifndef	NEC_98
	in	al, DMA2_Status 	;   to clear any old TC conditions
else;	NEC_98
	push	edx
	mov	dx, DMA2_Status
	in	al, dx
	pop	edx
endif;	!NEC_98

ifndef	NEC_98
;
; Set up machine type
;
	push	ebx
	VMMCall Get_Machine_Info

	test	ebx, GMIF_MCA		    ; Q: Micro channel?
	jz	SHORT not_PS2DMA	    ;	N:

	mov	eax, VMM_TRUE
	xor	esi, esi
	mov	edi, OFFSET32 VDMAD_MCA_Size_Ini
	VMMCall Get_Profile_Boolean
	or	eax, eax
	jz	short not_PS2DMA	    ; jump if forced to non-PS2

	mov	[VDMAD_Machine_Type], MCA_Machine

; channel 4 is available on PS2's, so assign default handler proc
;
	mov	[DMA_Channels_2.call_back], OFFSET32 VDMAD_Call_Def
	call	VDMAD_PS2_Device_Init

not_PS2DMA:
	test	ebx, GMIF_EISA		    ; Q: EISA machine?
	jz	SHORT not_EISADMA	    ;	N:

	mov	eax, VMM_TRUE
	xor	esi, esi
	mov	edi, OFFSET32 VDMAD_EISA_Size_Ini
	VMMCall Get_Profile_Boolean
	or	eax, eax
	jz	short not_EISADMA	    ; jump if forced to non-EISA

	mov	[VDMAD_Machine_Type], EISA_Machine
	call	VDMAD_EISA_Init

not_EISADMA:
        test    ebx, GMIF_PCXT              ; XT with accelerator?
        jz      SHORT not_PCXT              ;   N:
        mov     [VDMAD_Machine_Type], PCXT_Machine

not_PCXT:    
endif;	!NEC_98

	call	VDMAD_API_Device_Init

ifdef	NEC_98 ; support to change N/H for NEC.
	call	VDMAD_Init_Bank		;
endif;	NEC_98

ifndef	NEC_98
; 
; Now that we know the machine type, we can establish the upper limit
; on the DMA buffer location.  First see if a limit is specified in 
; system.ini
;
        mov     eax, [DMA_Max_Physical]      ; default of 4Gb, or value
                                             ;   from Reserve_Buffer_Space
        xor     esi, esi
        mov     edi, OFFSET32 VDMAD_Max_DMA_pg_Ini
        VMMCall Get_Profile_Hex_Int
        
        cmp     [VDMAD_Machine_Type], EISA_Machine
        je      sci_check_xt                 ; check for 1Mb limit
;
; ISA and MCA support a max of 16Mb, so make sure we aren't over that limit
;
        cmp     eax, 1000h               
        jbe     sci_check_xt                ; doesn't exceed 16Mb, okay
        mov     eax, 1000h                  ; limit it to 16Mb

sci_check_xt:
        push    eax                         ; save current limit    
	mov	eax, FALSE
	mov	edi, OFFSET32 VDMAD_Ini_XT_Buffer
	VMMCall Get_Profile_Boolean
	or	eax, eax		    ;Q: locate buffer in 1st Mb
        pop     eax                         ; recover current limit
	jz	short sci_got_max	    ;	N:
        cmp     eax, 100h                   ;Q: max restricted to less than 1Mb?
        jbe     sci_got_max                 ;	Y: leave it alone
        mov     eax, 100h                   ;	N: change max to 1Mb

sci_got_max:
	mov	[DMA_Max_Physical], eax     
else ;NEC_98
        mov     eax, [DMA_Max_Physical]
        xor     esi, esi
        mov     edi, OFFSET32 VDMAD_Max_DMA_pg_Ini
        VMMCall Get_Profile_Hex_Int
	mov	[DMA_Max_Physical], eax     
endif;	!NEC_98

;
; Now set up the DMA buffer
;
	xor	eax, eax
	xor	esi, esi
	mov	edi, OFFSET32 VDMAD_Ini_Buffer_Size
	VMMCall Get_Profile_Decimal_Int
	call	VDMAD_Set_Req_Buff_SizeK

;
; Initialize global DMA controller/channel state.  For EISA machines, we'll
; reset them a little later, when we check each channel for byte or word
; count state.
;
	mov	[DMA_Ctrl1.CTL_flags], 0
ifndef	NEC_98
	mov	[DMA_Ctrl2.CTL_flags], DMA_2nd_Ctlr OR DMA_Word_Ctlr
else
	mov	[DMA_Ctrl2.CTL_flags], 0
endif

mask_all_channels:
	mov	al, 1111b		; mask all channels of both controllers
	mov	[DMA_Ctrl1.CTL_mask], al
	mov	[DMA_Ctrl2.CTL_mask], al

IFDEF allow_partial_virtualization
	mov	[DMA_Ctrl1.CTL_mode_set], 1111b  ; no mode set in controller 1
	mov	[DMA_Ctrl2.CTL_mode_set], 1111b  ; no mode set in controller 1

ENDIF

	out	DMA1_CLR_FF, al 	; clear flop flop
ifndef	NEC_98
	out	DMA2_CLR_FF, al 	; clear flop flop
else;	NEC_98
	push	edx
	mov	dx, DMA2_CLR_FF
	out	dx, al
	pop	edx
endif;	!NEC_98

ifndef	NEC_98
	IF_MCA_JMP  <read_ps2_chnls>
endif

	xor	edi, edi
	mov	ecx, OFFSET32 DMA_Channels

init_chnls:
ifndef	NEC_98
	movzx	edx, page_ports[edi]
else
	movzx	edx, page_ports[edi*2]
endif
	in	al, dx
	movzx	eax, al
	mov	word ptr [ecx.cur_addr+2], ax
	mov	word ptr [ecx.pgm_addr+2], ax
ifndef	NEC_98
	movzx	edx, base_ports[edi]
else
	movzx	edx, base_ports[edi*2]
endif
	in	al, dx
	mov	byte ptr [ecx.cur_addr], al
	mov	byte ptr [ecx.pgm_addr], al
	in	al, dx
	mov	byte ptr [ecx.cur_addr+1], al
	mov	byte ptr [ecx.pgm_addr+1], al
ifndef	NEC_98
	movzx	edx, count_ports[edi]
else
	movzx	edx, count_ports[edi*2]
endif
	xor	eax, eax
	in	al, dx
	mov	ah, al
	in	al, dx
	xchg	al, ah
	mov	[ecx.cur_count], eax
	mov	[ecx.pgm_count], eax

	mov	[ecx.mode], DMA_single_mode
	mov	[ecx.ext_mode], 0               ; assume byte transfers
ifndef	NEC_98
	IF_EISA_JMP <SHORT get_EISA_high_ports>
	bt	edi, 2				; channel 4 - 7 ?
	jnc	short set_bytesize_port
        jmp     short set_wordsize_port
;
; Read in top eight bits of address and count on EISA systems
;
get_EISA_high_ports:
        cmp     edi,4                           ; no channel 4, so skip it
        je      set_bytesize_port

        movzx   edx, EISA_high_page_ports[edi*2]
        in      al,dx   
        mov     byte ptr [ecx.cur_addr+3], al
        mov     byte ptr [ecx.pgm_addr+3], al 
        movzx   edx, EISA_high_count_ports[edi*2]
        in      al,dx
        mov     byte ptr [ecx.cur_count+2], al
        mov     byte ptr [ecx.pgm_count+2], al
;
; Check for 16-bit mode setting on EISA ports, by reading extended modes
; array set up earlier
;
        mov     al, DMA_EISA_Ext_Modes[edi]     ; get defined mode
        and     al, DMA_EM_Chan_Size            ; isolate the channel bits
        cmp     al, DMA_EM_16bit_wc             ; word count transfers?
        je      set_wordsize_flag               ; Y: go set it up
;
; Transfers on this channel are byte count.  If it's controller 1, we're
; already initialized correctly.  If not, we have to switch the flag
; for this channel
;
        bt      edi, 2                          ; first controller?
        jnc     set_bytesize_port               ; Y: done
        sub     edi, 4                          ; adjust index
        btr     [DMA_Ctrl2.CTL_Flags], edi      ; clear word count flag
        add     edi, 4                          ; get back channel #
        jmp     short set_bytesize_port
; 
; Transfers on this channel are word count.  If it's controller 2, we're
; already initialized correctly.  If not, we have to switch the flag
; for this channel.
;
set_wordsize_flag:
        bt      edi, 2                          ; first controller?
        jc      set_wordsize_port               ; N: done
        bts     [DMA_Ctrl1.CTL_Flags], edi      ; set word count flag
        
set_wordsize_port:
;
; The low 17 bits of address we read in reflect a word-oriented address.
; We need to adjust the address to reflect a byte orientation.  The top
; 7 bits of the page segment, and the 8 bits of the EISA high page, 
; reflect the byte-oriented address, so we don't want to change them.
; We know cur_addr and pgm_addr are the same now, so we only need to
; check for bit 15 set once.
;
	mov	[ecx.ext_mode], _16_bit_xfer
	shl	word ptr [ecx.cur_addr],1       ; convert to byte addr
        clc                                     ; make sure carry clear
	rcl	word ptr [ecx.pgm_addr],1       ; convert to byte, save high bit
        jnc     set_bytesize_port               ; no high bit, done
        or      byte ptr [ecx.cur_addr+2], 1    ; set bit 16 of stored addr.
        or      byte ptr [ecx.pgm_addr+2], 1        
endif;	!NEC_98
set_bytesize_port:
	inc	edi
	add	ecx, SIZE DMA_Channel_Data
	cmp	edi, DMA_Channels_Available
	jb	init_chnls
	jmp	short chnls_done

ifndef	NEC_98
read_ps2_chnls:
	call	VDMAD_PS2_Read_Channels
endif

;
; Allocate portion of each VM's control block for DMA information
;
chnls_done:
	push	ebx
	VMMCall _Allocate_Device_CB_Area, <<SIZE DMA_CB_DATA>, 0>
	test	eax, eax
	jnz	SHORT DMAD_CB_OK
	Debug_Out "VDMAD ERROR:  Could not alloc control block data area space"
	VMMcall Fatal_Memory_Error

DMAD_CB_OK:
	mov	[VDMAD_CB_Offset], eax
	pop	ebx

;
; hook DMA I/O ports
;
ifndef	NEC_98
	mov	edi, OFFSET32 VDMAD_IO_Table  ; Table of ports
	VMMCall Install_Mult_IO_Handlers    ; Install the port traps
else
	push	ax
	push	dx
	mov	dx, DMA2_P_MODE
	in	ax, dx	; #0-#3 machine -> bit 0=1, #0-#7 machine -> bit 0=0
	pop	dx
	and	ax, 00000001b
	jnz	@f
	mov	edi, OFFSET32 VDMAD_IO_Table  ; Table of ports
	VMMCall Install_Mult_IO_Handlers    ; Install the port traps
	jmp	short	IO_Trap_Done
@@:
	mov	edi, OFFSET32 VDMAD_IO_Table_Old  ; Table of ports
	VMMCall Install_Mult_IO_Handlers    ; Install the port traps
IO_Trap_Done:
	pop	ax
endif

;
; register VPICD Hardware Int filter proc

	pushfd
	cli
	mov	esi, OFFSET32 VDMAD_jmp_filter
	VxDCall VPICD_Call_When_Hw_Int
	mov	[VDMAD_Hw_Int_Next], esi    ; save procedure to chain to
	mov	[VDMAD_Hw_Int_Filter], esi  ; no filter so far
	mov	[VDMAD_Check_TC], OFFSET32 VDMAD_NoCheck
	popfd

	mov	esi, OFFSET32 VDMAD_Copy_Exception
	VMMcall Install_Exception_Handler

ifndef	NEC_98 ; Pop too many.
	pop	ebx
endif
	clc
	ret

EndProc VDMAD_Sys_Crit_Init

ifdef	NEC_98
;*****************************************************************************
;
;   VDMAD_Init_Bank
;
;   DESCRIPTION:
;	Only used for stinking H98 model
;	Change DMA into 24it address mode, and 16Mb boundary.
;
;   ENTRY:
;
;   EXIT:
;
;   USED:
;      
;*****************************************************************************
BeginProc VDMAD_Init_Bank

	push	eax
	push	ecx
	push	edx
	push	edi

	MOV	DX,DMA_439		; set port address
	in	AL,dx			; get DMA mode
	JMP	$+2			; wait for I/O
	JMP	$+2			; 
	AND	AL,NOT MD2		; set DMA 24bit addressing
	OUT	DX,AL			;
	JMP	$+2			; wait for I/O
	JMP	$+2			;
	mov	al,00h			;
	out	DMA1_1BH,al		; master clear
	JMP	$+2			; wait for I/O
	JMP	$+2			;
	push	edx
	mov	dx,DMA2_1BH
	out	dx,al			; master clear
	pop	edx
	JMP	$+2			; wait for I/O
	JMP	$+2			;

	mov	al,40h			;
	out	DMA1_STS,al		; master clear
	JMP	$+2			; wait for I/O
	JMP	$+2			;
	push	edx
	mov	dx,DMA2_STS
	out	dx,al			; master clear
	pop	edx
	JMP	$+2			; wait for I/O
	JMP	$+2			;

	mov	ecx,4			; set loop counter
	mov	al,0ch			; DMA boundary paramer
					;   with channel number
Init_29H:
	out	DMA1_P_MODE,al		; set DMA boundary to 16MB
	JMP	$+2			; wait for I/O
	JMP	$+2			;
	push	edx
	mov	dx,DMA2_P_MODE
	out	dx,al			; set DMA boundary to 16MB
	pop	edx
	JMP	$+2			; wait for I/O
	JMP	$+2			;
	inc	al			; set next channel number
	loop	Init_29H		; Init next channel

	call	VDMAD_Init_Sub		; master clear
					;   & init bank registers
        pop     edi			;
	pop	edx			;
	pop	ecx			;
        pop     eax			;
        ret				;
EndProc VDMAD_Init_Bank
endif;	NEC_98

IFDEF rpMemTst
;******************************************************************************
;
;   VDMAD_Device_Init
;
;   DESCRIPTION:
;
;   ENTRY:	    EBX is SYS VM handle
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Device_Init

%OUT special setup of Ctrl+Alt+M is memory fragmenter hot key

ifndef	NEC_98
	mov	ax, 32h 		    ; 'M' key
else
	mov	ax, 2Fh 		    ; 'M' key
endif
	ShiftState <SS_Either_Ctrl + SS_Either_Alt + SS_Toggle_mask>, <SS_Ctrl + SS_Alt>
	mov	cl, CallOnPress
	mov	esi, OFFSET32 Fragment_Memory ; call-back routine
	xor	edx, edx		    ; no reference data
	VxDCall VKD_Define_Hot_Key
	Trace_Out "Ctrl+Alt+M is memory fragmenter hot key"
	clc
	ret

EndProc VDMAD_Device_Init
ENDIF


BeginDoc
;******************************************************************************
;
;   VDMAD_Reserve_Buffer_Space
;
;   DESCRIPTION:
;	This service allows other devices that are going to handle DMA to
;	make sure that VDMAD allocates a buffer large enough for any transfers
;	that they might require.  It also allows a device to specify a maximum
;	physical address that would be valid for the device's DMA requests
;	(such as 1Mb for an XT.)  During the Init_Complete phase of
;	initialization, VDMAD will allocate the DMA buffer using all of the
;	contraints specified by other devices.	i.e. the buffer will be at
;	least as big as the largest size specified by the calls to this
;	service, and it will be allocate below the lowest maximum physical
;	addresses specified.
;
;	This service is only available before Init_Complete.
;
;   ENTRY:	    EAX = # of pages requested
;		    ECX = maximum physical address that can be included in a
;			  DMA transfer; 0, if no limit.
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Reserve_Buffer_Space, SERVICE

	cmp	[VDMAD_SysCrit_Done], VMM_TRUE
	je	SHORT vres_bad
	pushad
	shl	eax, 2			; convert from pages to Kb
	call	VDMAD_Set_Req_Buff_SizeK

	jecxz	short no_max_change
	inc	ecx
	shr	ecx, 12
	cmp	ecx, [DMA_Max_Physical]
	jae	short no_max_change
	mov	[DMA_Max_Physical], ecx
no_max_change:
	popad
	ret

vres_bad:
	Debug_Out "ERROR:  VDMAD_Reserve_Buffer_Space called after Sys_Critical_Init"
	Fatal_Error
EndProc VDMAD_Reserve_Buffer_Space


;******************************************************************************
;
;   VDMAD_Init_Complete
;
;   DESCRIPTION:
;	At Init_Complete time we will force the DMA buffer to be allocated
;	if a DMABUFFERSIZE entry was found in System.Ini.  This procedure
;	then jumps to VDMADArb_Init_Complete to initialize the arbitrator.
;	We also pre-allocate it if we are paging through dos so we don't
;	try to allocate it at a bad time later.
;
;   ENTRY:
;	EBX = System VM handle
;
;   EXIT:
;	Carry always clear (cleared by VDMADArb_Init_Complete)
;
;   USES:
;	Can use any reg and flags.
;
;==============================================================================

BeginProc VDMAD_Init_Complete

	mov	ecx, [DMA_MinReq_SizeK]
	test	ecx, ecx
	jnz	vic_prealloc

	VxDCall PageSwap_Get_Version		; (bl) = how we are paging
	jc	VDMADArb_Init_Complete		; jump to exit if no pageswap
	cmp	bl,PF_Paging_DOS		; dos pager?
	jnz	VDMADArb_Init_Complete		; jump if not dos pager

	mov	ecx, MinBufferSizeK		; Always allocate min buf size

;	If we get here, we need to preallocate the dma buffer

vic_prealloc:
	shl	ecx, 10 			; Convert Kb to bytes
	xor	esi, esi			; Linear address (bogus)
	VxDcall VDMAD_Request_Buffer		; This allocates the buffer
	Debug_OutC "VDMAD ERROR:  Could not allocate buffer at Init_Complete!"
	jc	VDMADArb_Init_Complete		; If error, just prey!

	VxDcall VDMAD_Release_Buffer		; Release the buffer
	jmp	VDMADArb_Init_Complete		; And init the arbitrator

EndProc VDMAD_Init_Complete


VxD_ICODE_ENDS


;******************************************************************************
;******************************************************************************

IFDEF rpMemTst
VxD_DATA_SEG
	fragmentList dd 100 DUP(0)
VxD_DATA_ENDS
ENDIF

VxD_CODE_SEG

ifndef NO_EXIT
EXTRN VDMAD_API_System_Exit:NEAR
endif ;NO_EXIT
ifndef	NEC_98
EXTRN VDMAD_PS2_Read_Channels:NEAR
endif

IFDEF rpMemTst
BeginProc Fragment_Memory

	pushfd
	pushad
	mov	ecx, 100
	mov	edi, OFFSET32 fragmentList
	cmp	dword ptr [edi], 0	    ;Q: already called once?
	je	short do_allocs 	    ;	N:

	Trace_Out "freeing fragmentation pages"
free_pages:
	mov	eax, [edi]
	or	eax, eax		    ;Q: valid handle?
	jz	short no_page		    ;	N:
	push	ecx
	VMMCall _PageFree <eax, 0>
	pop	ecx
	xor	eax, eax
no_page:
	cld
	stosd
	loop	free_pages
	jmp	short fm_exit

do_allocs:
	push	ecx
	VMMCall _PageAllocate <1, PG_SYS, 0, 0, 0, 0, 0, PageLocked>
	cld
	stosd				    ; save handle
	pop	ecx
	loop	do_allocs

	mov	ecx, 50
	mov	edi, OFFSET32 fragmentList
free_odd_pages:
	add	edi, 4
	mov	eax, [edi]
	push	ecx
	VMMCall _PageFree <eax, 0>
	pop	ecx
	xor	eax, eax
	cld
	stosd				    ; clear handle
	loop	free_odd_pages
	Trace_Out "allocated fragmentation pages"

fm_exit:
	popad
	popfd
	ret

EndProc Fragment_Memory
ENDIF

ifdef	NEC_98; code for NEC.
;*****************************************************************************
;   VDMAD_Init_Sub - reset DMAC & initialize Bank address registers
;
;   ENTRY: Protected Mode Ring 0
;          DS = DGROUP
;
;   EXIT:  Protected Mode Ring 0
;          none
;
;   USED:  al,ecx,dx,flags
;   STACK: 0
;*****************************************************************************
BeginProc VDMAD_Init_Sub
	mov	ecx,4			; set loop counter
	mov	al,00h			; initial value for Bank
	mov	dx,DMA_P1		; bank reg (Ch#1)

VDMAD_Init_BankRegs:
	out	dx,al			; init Bank register
	inc	edx			;
	inc	edx			; bank reg for next ch
	loop	VDMAD_Init_BankRegs	; initialize next channel

	mov	ecx,3			; set loop counter
	mov	dx,DMA_P5		; bank reg (Ch#5)

VDMAD_Init_BankRegs_2:
	out	dx,al			; init Bank register
	inc	edx			;
	inc	edx			; bank reg for next ch
	loop	VDMAD_Init_BankRegs_2	; initialize next channel

	ret

EndProc VDMAD_Init_Sub

;*****************************************************************************
;   VDMAD_Initialize_DMA - Re-Initialize for end of MS-WINDOWS/386 session
;
;   ENTRY: Protected Mode Ring 0
;          DS = DGROUP
;
;   EXIT:  Protected Mode Ring 0
;          none
;
;   USED:  none
;   STACK: 16 bytes
;*****************************************************************************
BeginProc VDMAD_Initialize_DMA

	push	eax			;
	push	ecx			;
	push	edx			;
	pushfd				;

	cli
	mov	al,00h			;
	out	DMA1_1BH,al		;master clear
	JMP	$+2			; wait for I/O
	JMP	$+2			;
	push	edx
	mov	dx,DMA2_1BH
	out	dx,al			;master clear
	pop	edx
	JMP	$+2			; wait for I/O
	JMP	$+2			;

	mov	al,40h			;
	out	DMA1_STS,al		;master clear
	JMP	$+2			; wait for I/O
	JMP	$+2			;
	push	edx
	mov	dx,DMA2_STS
	out	dx,al			;master clear
	pop	edx
	JMP	$+2			; wait for I/O
	JMP	$+2			;

	mov	dx,DMA_439		;
	in	al,dx			;read mode
	or	al,MD2			;DMA 20bit addressing
	out	dx,al			;set mode

	and	al,MD0			;strip MD0 bit
					;MD0: 0->normal mode
					;     1->high-reso mode
	shl	al,2			;now it's changed to
					;DMA boundary parameter!

	mov	ecx,4			;set loop counter

VDMAD_Sys_ReInit_29H:
	out	DMA1_P_MODE,al		;set DMA boundary
	JMP	$+2			; wait for I/O
	JMP	$+2			;
	push	edx
	mov	dx,DMA2_P_MODE
	out	dx,al			;set DMA boundary
	pop	edx
	JMP	$+2			; wait for I/O
	JMP	$+2			;

	inc	al			;add channel number
	loop	VDMAD_Sys_ReInit_29H	;set parm to next CH
	call	VDMAD_Init_Sub		;master clear
					; & init bank registers
	mov	al,03h			;dma 20bit

	popfd				;
	pop	edx			;
	pop	ecx			;
	pop	eax			;
	ret				;
EndProc VDMAD_Initialize_DMA
endif;	NEC_98

;******************************************************************************
;
;   VDMAD_Control
;
;   DESCRIPTION:    dispatch control messages to the correct handlers
;
;   ENTRY:
;
;   EXIT:	    Carry clear if no error
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Control

	Control_Dispatch Sys_Critical_Init,  VDMAD_Sys_Crit_Init
IFDEF rpMemTst
	Control_Dispatch Device_Init,	     VDMAD_Device_Init
ENDIF
	Control_Dispatch Create_VM,	     <SHORT VDMAD_Init_VM_CB>
IFDEF rpMemTst
	Control_Dispatch VM_Terminate,	     VDMAD_Terminate
ENDIF
ifndef	NEC_98	; BUGBUG: Need consideration.
	Control_Dispatch VM_Not_Executeable, <DEBFAR VDMAD_VM_Not_Executeable>
else
	Control_Dispatch VM_Not_Executeable, VDMAD_VM_Not_Executeable
endif
ifndef NO_EXIT
	Control_Dispatch Sys_Critical_Exit,  VDMAD_API_System_Exit
endif ;NO_EXIT
IFDEF ARBIT
	Control_Dispatch PNP_NEW_DEVNODE,    VDMAD_PnP_New_DevNode
	Control_Dispatch Init_Complete, VDMAD_Init_Complete
ENDIF
IFDEF DEBUG
	Control_Dispatch VM_Suspend,	     <SHORT VDMAD_VM_Suspend>
ENDIF
        IsDebugOnlyLoaded VDMAD_Ctrl_Exit
	Control_Dispatch Debug_Query,	     VDMAD_Query

VDMAD_Ctrl_Exit:
	clc
	ret

EndProc VDMAD_Control


;******************************************************************************
;
;   VDMAD_Init_VM_CB
;
;   DESCRIPTION:    Initialize virtual DMA state for new VM.
;
;   ENTRY:	    EBX = Handle of VM being initialized
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Init_VM_CB

	Assert_VM_Handle ebx		; Debugging
	mov	esi, ebx		; set esi to point to DMA data in
	add	esi, [VDMAD_CB_Offset]	; the VM's control block

	xor	eax, eax
	mov	word ptr [esi.DMA_flipflop], ax     ; clear VM's flipflop state

	clc
	ret

EndProc VDMAD_Init_VM_CB


IFDEF DEBUG
;******************************************************************************
;
;   VDMAD_VM_Suspend
;
;   DESCRIPTION:    check for any DMA transfers that are active for this VM
;
;   ENTRY:	    EBX = VM Handle
;
;   EXIT:	    Carry clear
;
;   USES:	    EAX, ECX, Flags
;
;==============================================================================
BeginProc VDMAD_VM_Suspend

	mov	eax, OFFSET32 DMA_Channels
	mov	ecx, 8
vms_lp:
	cmp	[eax.owner_VM], ebx
	jne	short vms_next
	call	Test_Global_DMA     ;don't whine about DMA to/from global mem
	jc	short vms_next
	Debug_Out 'VM #ebx is being suspended with an active DMA transfer'
vms_next:
	add	eax, SIZE DMA_Channel_Data
	loop	vms_lp
	clc
	ret

EndProc VDMAD_VM_Suspend
ENDIF

;******************************************************************************
;
;   VDMAD_Abort_Transfer
;
;   DESCRIPTION:    Free allocated DMA buffer, or unlock DMA region and
;		    physically mask channel.
;
;   ENTRY:	    EAX = DMA Handle
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================
BeginProc VDMAD_Abort_Transfer

; virtually & physically mask the channel again

	push	eax
	push	esi
	mov	esi, OFFSET32 DMA_Ctrl1
	mov	eax, [eax.channel_num]
	btr	eax, 2				; Q: controller 2?
	jnc	SHORT at_ctrl_set
	mov	esi, OFFSET32 DMA_Ctrl2
at_ctrl_set:
	bts	dword ptr [esi.CTL_mask], eax	; virtually mask the channel
	pop	esi
	pop	eax

	VxDCall VDMAD_Mask_Channel		; physically mask the channel
;
; Release memory being held for this transfer
;
	cmp	[eax.buffer_id],0	;Q: buffer assigned?
	je	short at_unlock 	;   N:
	call	VDMAD_Free_Buffer
	jmp	short at_end

at_unlock:
	cmp	[eax.locked_pages], 0	;Q: pages locked?
	je	short at_end		;   N:
	stc
	call	VDMAD_Free_Region	;   Y: unlock them

at_end:
	ret

EndProc VDMAD_Abort_Transfer


;******************************************************************************
;
;   VDMAD_VM_Not_Executeable
;
;   DESCRIPTION:    Abort any DMA transfers that are in progress for this VM,
;		    since it is no longer running.
;
;   ENTRY:	    EBX is VM handle
;
;   EXIT:	    Carry clear
;
;   USES:	    EAX, ECX, EDX, Flags
;
;==============================================================================

BeginProc VDMAD_VM_Not_Executeable

	Assert_VM_Handle ebx		; Debugging

;
; abort any DMA transfers that are active for this VM
;
	mov	eax, OFFSET32 DMA_Channels
	mov	ecx, 8
vmne_lp:
	cmp	[eax.owner_VM], ebx
	jne	short vmne_next

	call	Test_Global_DMA 	;Q: DMA to/from global memory?
	jnc	short vmne_abort	;   N: go abort transfer

	mov	edx, ebx		;   Y: assign DMA transfer to SYS VM
	VMMcall Get_Sys_VM_Handle
	xchg	edx, ebx
	mov	[eax.owner_VM], edx
	cmp	[eax.buffer_id], 0	;Q: channel owns buffer?
	je	short @F		;   N:
	push	eax
	movzx	eax, [eax.buffer_id]
	mov	DMA_Buff_Table[eax-1].DB_Owner, edx ; Y: reassign buffer owner also
	pop	eax
@@:
	cmp	[eax.disable_cnt], 0	;Q: translation disabled?
	jne	short vmne_next 	;   Y: addr is physical
	push	esi			;   N: convert hi liner to sys VM
	mov	esi, [eax.region_base]
	sub	esi, [ebx.CB_High_Linear]
	add	esi, [edx.CB_High_Linear]
	mov	[eax.region_base], esi
	pop	esi
	jmp	SHORT vmne_next

vmne_abort:
	call	VDMAD_Abort_Transfer

vmne_next:
	add	eax, SIZE DMA_Channel_Data
	loop	vmne_lp

;
; Check if the VM owns the buffer also.  We'll free it, because we really have
; to, but it could be dangerous, if it was claimed thru VDS, because the
; transfer could still be in progress, also it could be in use by a global
; bus master (VDS aware) device.  There just isn't a right answer here.
;

	mov	eax, OFFSET DMA_Buff_Table
vne_loop:
	cmp	[eax.DB_owner], ebx
	jne	vne_skip_buff

	Debug_Out 'VDMAD: freeing DMA buffer at VM_Not_Executeable', DEBLEVELMAX
	push	ebx
	mov	ebx, eax
	sub	ebx, OFFSET32 DMA_Buff_Table-1
	VxDCall VDMAD_Release_Buffer
	pop	ebx

vne_skip_buff:
	add	eax, SIZE DMA_Buff_Struc
	cmp	eax, OFFSET32 End_DMA_Buff_Table
	jb	vne_loop

	clc
	ret

EndProc VDMAD_VM_Not_Executeable


;******************************************************************************
;
;   VDMAD_Get_DMA_Handle
;
;   DESCRIPTION:    Get a DMA handle for a given channel
;
;   ENTRY:	    EAX is channel #
;
;   EXIT:	    Carry clear
;			EAX is DMA handle
;		    Carry set
;			invalid channel #
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Get_DMA_Handle

	cmp	eax, DMA_Channels_Available
	jae	SHORT bad_channel
	imul	eax, SIZE DMA_Channel_Data
	add	eax, OFFSET32 DMA_Channels    ; eax is DMA handle
	stc

bad_channel:
	cmc
	ret

EndProc VDMAD_Get_DMA_Handle


;******************************************************************************
;
;   VDMAD_Get_DMA_Handle_For_Ctrl
;
;   DESCRIPTION:    Given a controller data pointer and a channel # (0-3),
;		    return a pointer to the channel's DMA_Channel_Data
;		    structure.
;
;   ENTRY:	    ECX = channel # (0-3)
;		    ESI -> DMA_Controller_State
;
;   EXIT:	    EAX -> DMA_Channel_Data
;
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Get_DMA_Handle_For_Ctrl

IFDEF DEBUG
	cmp	ecx, 3
	jbe	short chn_ok
	Debug_Out "VDMAD_Get_DMA_Handle_For_Ctrl called with DMA channel > 3!"
chn_ok:
	cmp	esi, OFFSET32 DMA_Ctrl1
	jz	short ctrl_ok
	cmp	esi, (OFFSET32 DMA_Ctrl1) + (SIZE DMA_Controller_State)
	jz	short ctrl_ok
	Debug_Out "VDMAD_Get_DMA_Handle_For_Ctrl called with bad DMA ctrl pointer!"
ctrl_ok:
ENDIF
	mov	eax, ecx
	cmp	esi, OFFSET32 DMA_Ctrl1 	; Controller # 1?
	jbe	short on_ctrl_1
	add	al,4
on_ctrl_1:
	imul	eax, SIZE DMA_Channel_Data
	add	eax, OFFSET32 DMA_Channels	; eax is DMA handle

	ret

EndProc VDMAD_Get_DMA_Handle_For_Ctrl


;******************************************************************************
;
;   Test_Global_DMA
;
;   DESCRIPTION:    Determine if a programmed DMA address is to global
;		    memory.
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    CY set if DMA to/from global memory
;
;   USES:	    flags
;
;==============================================================================

BeginProc Test_Global_DMA

	push	eax
	push	ecx
	push	edx

	cmp	[eax.pgm_addr], 0A0000h 	; adapter region is global
	jae	SHORT trg_make_global

	mov	edx, [eax.pgm_count]
ifndef	NEC_98
	test	[eax.ext_mode], _16_bit_xfer
	jz	SHORT trg_byte_cnt
	shl	edx, 1
	inc	edx
trg_byte_cnt:
endif
	inc	edx				; edx = DMA size in bytes

	VMMcall _TestGlobalV86Mem,<[eax.pgm_addr],edx,0>    ; is it global?
	cmp	eax, 1
	jz	SHORT trg_make_global

IFDEF DEBUG
	or	eax, eax
	jz	SHORT trg_no_overlap
	Debug_Out "DMA buffer overlap: global/local or instance (#EAX)"
trg_no_overlap:
ENDIF
	clc					; is local
	jmp	SHORT trg_ret

trg_make_global:				; is global
	stc

trg_ret:
	pop	edx
	pop	ecx
	pop	eax
	ret

EndProc Test_Global_DMA

;******************************************************************************
;
;   VDMAD_Read_DMA_Status
;
;   DESCRIPTION:    Read the DMA controller status registers and update
;		    status shadow register/bytes.
;
;   ENTRY:	    None.
;
;   EXIT:	    AH = DMA2_Status, AL = DMA1_Status
;
;   USES:	    EAX, flags
;
;==============================================================================

BeginProc VDMAD_Read_DMA_Status

	push	ecx

	in	al, DMA1_Status 		; get/save ctrl 1 status
	or	[VDMAD_DMA1_status], al
	mov	cl, [DMA_Ctrl1.CTL_status]	; update virtual status to
	and	cl, 0Fh 			;   include new TC bits but
	or	cl, al				;   just use current request
	mov	[DMA_Ctrl1.CTL_status], cl	;   bits

ifndef	NEC_98
	in	al, DMA2_Status 		; likewise for ctrl 2
else;	NEC_98
	push	edx
	mov	dx, DMA2_Status
	in	al, dx
	pop	edx
endif;	!NEC_98
	or	[VDMAD_DMA2_Status], al
	mov	cl, [DMA_Ctrl2.CTL_status]
	and	cl, 0Fh
	or	cl, al
	mov	[DMA_Ctrl2.CTL_status], cl

	movzx	eax, word ptr [VDMAD_DMA1_Status]
	.errnz	VDMAD_DMA2_Status - VDMAD_DMA1_Status - 1

	pop	ecx
	ret

EndProc VDMAD_Read_DMA_Status

;******************************************************************************
; SERVICES
;******************************************************************************

BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Version
;
;   DESCRIPTION:
;	Returns the version of the Virtual DMA Device
;
;   ENTRY:
;	None
;
;   EXIT:
;	AH = Major version number
;	AL = Minor version number
%OUT BUGBUG: doc incorrect, we're not returning buffer size, nor have we ever
;	ECX = Buffer size in bytes (0, if not allocated; a buffer will always
;		be allocated, but it doesn't happen until Init_Complete)
;	Carry flag clear
;
;   USES:
;	EAX, Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Get_Version, SERVICE

	mov	eax, 400h
	clc
	ret

EndProc VDMAD_Get_Version

BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Max_Phys_Page
;
;   DESCRIPTION:
;	Returns the maximum addressable page by the DMA controller.
;       This value is either determined programmatically or overriddden
;       using a SYSTEM.INI switch.
;
;   ENTRY:	    NOTHING
;
;   EXIT:	    EAX = max addressable DMA page
;
;   USES:	    flags, EAX
;
;==============================================================================
EndDoc

BeginProc VDMAD_Get_Max_Phys_Page, SERVICE

        mov     eax, [DMA_Max_Physical]      ; default of 4Gb, or value
        ret

EndProc VDMAD_Get_Max_Phys_Page

BeginDoc
;******************************************************************************
;
;   VDMAD_Virtualize_Channel
;
;   DESCRIPTION:
;	This service allows another Windows/386 device to claim ownership
;	of a standard DMA channel.  The new owner registers a call-back
;	routine that will be called whenever the virtual state of the channel
;	is changed as a result of I/O done in a VM.  In some cases a device
;	doesn't want to allow a VM to perform DMA to a channel at all (they
;	will handle programming based on a private API, etc. instead of
;	virtualized hardware I/O), so it is possible to pass a 0 to specify
;	a null call-back routine.  VDMAD will continue to trap the I/O for
;	the channel, but won't ever change the physical state of the channel
;	as a result of any VM I/O.
;
;   ENTRY:	    EAX is Channel #
;		    ESI is I/O Call-back procedure (0 = none)
;
;   EXIT:	    Carry set if channel is already owned
;		    ELSE
;		      EAX is DMA handle
;
;   USES:	    EAX, EDX, flags
;
;   CALL BACK:	    ENTRY:
;			EAX = DMA handle
;			EBX = VM handle
;			Proc can modify EAX, EBX, ECX, EDX, ESI, EDI, and flags
;
;		    EXIT
;			none
;
;==============================================================================
EndDoc
BeginProc VDMAD_Virtualize_Channel, SERVICE

IFDEF DEBUG
	push	ebx
	mov	ebx, eax
ENDIF
	call	VDMAD_Get_DMA_Handle
IFDEF DEBUG
	jnc	SHORT channel_valid
	cmp	[eax.channel_num], ebx	;Q: valid channel?
	je	SHORT channel_valid	;   Y:
	Debug_Out "VDMAD:  invalid channel requested #EBX"

;       fatal error trap removed so that debug and retail have the
;       same behavior.
;
;       Fatal_Error

channel_valid:
	pop	ebx
ENDIF

	cmp	[eax.call_back], OFFSET32 VDMAD_Call_Def
	je	short not_virtualized	; jump if not virtualized already
	Debug_Out "VDMAD:  channel #EBX already virtualized"
	stc
	jmp	short virt_exit

not_virtualized:
IFDEF ARBIT
        push    eax
        mov     eax, [eax.channel_num]
        call    VDMADArb_Alloc_Channel
        pop     eax
ENDIF
	mov	[eax.call_back], esi	; assign new call-back proc, or 0
	clc

virt_exit:
	ret

EndProc VDMAD_Virtualize_Channel

BeginDoc
;******************************************************************************
;
;   VDMAD_Unvirtualize_Channel
;
;   DESCRIPTION:
;       This service allows a VxD to release its virtualization of
;       a standard DMA channel.  The previously registered call back will
;       not be called again. The channel is available for virtualization
;       by other VxDs. VDMAD will continue to trap the I/O for
;	the channel, but won't ever change the physical state of the channel
;	as a result of any VM I/O.
;
;   ENTRY:          EAX is DMA handle
;
;   EXIT:           Carry set if DMA handle is invalid
;
;   USES:	    EAX, EDX, flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Unvirtualize_Channel, SERVICE

        mov     edx, eax
        mov     eax, [edx.channel_num]
        call    VDMAD_Get_DMA_Handle
        jc      SHORT VUC_Invalid
        cmp     eax, edx
        jz      SHORT VUC_Valid
VUC_Invalid:
        Debug_Out "VDMAD:  invalid channel handle #EDX for Unvirtualize"
        stc
        ret
VUC_Valid:
        mov     eax, [edx.channel_num]
IFDEF ARBIT
        call    VDMADArb_Unalloc_Channel
ENDIF

IFDEF DEBUG
        cmp     [edx.call_back], OFFSET32 VDMAD_Call_Def
        jne     short VUC_virtualized   ; jump if virtualized
        Trace_Out "VDMAD:  channel #EBX already not virtualized"
VUC_virtualized:
ENDIF
        mov     [edx.call_back], OFFSET32 VDMAD_Call_Def
        mov     [edx.callbacks], 0
	clc
	ret
EndProc VDMAD_Unvirtualize_Channel


BeginDoc
;******************************************************************************
;
;   VDMAD_Set_Channel_Callbacks
;
;   DESCRIPTION:
;       Sets extended virtualization callbacks for a given channel.
;
;   ENTRY:          EAX is DMA handle
;                   EBX is callbacks AND mask
;                   ECX is callbacks OR mask
;      Option:
;          VDMAD_Callbacks_ReadNotify:  
;             Extended read notifications.  Callbacks are sent
;             by VDMAD when the count, page or address registers
;             are read.
;
;   EXIT:           callbacks types set
;
;   USES:	    EAX, EDX, flags
;
;==============================================================================
EndDoc

BeginProc VDMAD_Set_Channel_Callbacks, SERVICE

	Validate_DMA_Handle

ifdef DEBUG
        push    ecx
        and     ecx, VDMAD_Callbacks_Valid
        cmp     ecx, [esp]
        jz      SHORT @F
        Trace_Out "VDMAD: Set_Channel_Callbacks, invalid #ECX"
@@:
        pop     ecx
endif

        and     [eax.callbacks], bx
        or      [eax.callbacks], cx
	ret

EndProc VDMAD_Set_Channel_Callbacks

BeginDoc
;******************************************************************************
;
;   VDMAD_Disable_Translation
;
;   DESCRIPTION:    This service disables the automatic translation done for
;		    the standard DMA channels.	It is necessary, if a V86 app
;		    or driver, or a PM app uses the DMA services thru INT 4Bh
;		    to determine actual physical addresses for DMA transfers.
;		    A disable count is maintained, so a matching call to
;		    VDMAD_Enable_Translation is required for each call to this
;		    service to re-enable translation.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;
;   EXIT:	    Carry clear
;			automatic translation is disable for the channel
;		    Carry set
;			the disable count overflowed
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Disable_Translation, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	push	ecx
	mov	ecx, [eax.channel_num]
	bts	[DMA_disabled], ecx	; set channel disabled flag

	mov	ecx, [eax.disable_cnt]	; maintain disable call count
	add	ecx, 1
	jc	SHORT disable_exit
	mov	[eax.disable_cnt], ecx
disable_exit:
	pop	ecx
	ret

EndProc VDMAD_Disable_Translation


BeginDoc
;******************************************************************************
;
;   VDMAD_Enable_Translation
;
;   DESCRIPTION:    This decrements the disable count associated with a
;		    standard DMA channel.  If the disable count goes to 0, then
;		    automatic translation is re-enabled.
;		    See VDMAD_Disable_Translation for further information.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;
;   EXIT:	    Carry clear
;			service completed successfully
;			Z-flag clear, if automatic translation is re-enabled
;		    Carry set
;			attempt to enable when translation already enabled
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Enable_Translation, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	push	ecx
	mov	ecx, [eax.disable_cnt]	; maintain channel disable count
	sub	ecx, 1
	jc	SHORT enable_exit
	mov	[eax.disable_cnt], ecx
	jnz	SHORT enable_exit	; jump if still disabled
	mov	ecx, [eax.channel_num]
	btr	[DMA_disabled], ecx	; clear disabled bit
	clc
enable_exit:
	pop	ecx
	ret

EndProc VDMAD_Enable_Translation

BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Region_Info
;
;   DESCRIPTION:    Get information about the current region assigned to a DMA
;		    handle.  This information can be used by a handler to
;		    call following services:
;
;			VDMAD_Unlock_DMA_Region
;			VDMAD_Release_Buffer
;			VDMAD_Copy_To_Buffer
;			VDMAD_Copy_From_Buffer
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    BL	= buffer id
;		    BH	= pages locked (0 = FALSE, else VMM_TRUE)
;		    ESI = region linear
;		    ECX = size in bytes
;
;   USES:	    EBX, ECX, ESI
;
;==============================================================================
EndDoc
BeginProc VDMAD_Get_Region_Info, SERVICE

	Validate_DMA_Handle
	movzx	ebx, [eax.buffer_id]
	mov	bh, [eax.locked_pages]
	mov	esi, [eax.region_base]
	mov	ecx, [eax.region_size]
	ret

EndProc VDMAD_Get_Region_Info


BeginDoc
;******************************************************************************
;
;   VDMAD_Set_Region_Info
;
;   DESCRIPTION:    Set information about the current region assigned to a DMA
;		    handle.  This service must be called before calling
;		    VDMAD_Set_Phys_State.
;
;   ENTRY:	    EAX = DMA handle
;		    BL	= buffer id (can be 0 -- Indicates none)
;		    BH	= pages locked (0 = FALSE, else VMM_TRUE)
;		    ESI = region linear
;		    ECX = size in bytes
;		    EDX = physical address for transfer
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Set_Region_Info, ASYNC_SERVICE

IFDEF DEBUG
	Validate_DMA_Handle
	test	bl, bl
	jz	@f
	Validate_Buffer_ID
@@:
ENDIF

	mov	[eax.buffer_id], bl
	mov	[eax.locked_pages], bh
	mov	[eax.region_base], esi
	mov	[eax.region_size], ecx

IFDEF DEBUG
; record largest transfer request
	cmp	ecx, [eax.largest_request]
	jbe	SHORT setr_D00
	mov	[eax.largest_request], ecx
setr_D00:
ENDIF

	push	edx

ifndef	NEC_98
; IF PS2, then extended command is used to program the address in 3 bytes
; (address lines A0-A23).  If ISA or EISA, we have to check whether the 
; channel is in byte or word count mode.  In word count mode,
; we have to adjust for AT word DMA scheme
;
	IF_MCA_JMP  <SHORT skip_buf_word_adjust>
        
        test    [eax.ext_mode], _16_bit_xfer    ; word count mode?
        jz      skip_buf_word_adjust

set_word_mode:
	shr	edx, 1			;   Y: page = address lines A17-A23
					;     edx=  <0>A31..A17|A16..A1
	ror	edx, 16 		;		A16..A1|<0>A31..A17
	shl	dx, 1			;		A16..A1|A31..A17<0>
	ror	edx, 16 		;	    A31..A17<0>|A16..A1
					;      base = address lines A1-A16
skip_buf_word_adjust:
endif;	!NEC_98

	mov	[eax.xfer_base], dx
	shr	edx, 16
	mov	[eax.xfer_page], dx
	pop	edx
	ret

EndProc VDMAD_Set_Region_Info


BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Virt_State
;
;   DESCRIPTION:
;	This service allows a channel owner to determine the current virtual
;	state of the channel.  The virtual state consists of all the
;	information necessary to physically program the DMA channel for a
;	DMA transfer (linear address of target region, byte length of region,
;	mode of transfer, and state of mask bit and software request bit)
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM handle
;
;   EXIT:	    If translation is enabled
;		      ESI = high linear address of the user's DMA region
;			  (high linear is used so that the DMA can proceed
;			   even if a different VM is actually running at the
;			   time of the transfer)
;		    Else
;		      ESI = physical byte address programmed (shifted left 1,
;			    for word ports)
;		    ECX = count in bytes
;		    DL	= mode (same as 8237 mode byte with channel # removed
;				and DMA_masked & DMA_requested set as
;				appropriate:
;				    DMA_masked	    channel masked and not ready
;						    for a transfer
;				    DMA_requested   software request flag set)
;		    DH	= extended mode (ignored on non-PS2 machines that don't
;					 have extended DMA capabilities)
;
;   USES:	    ESI, ECX, EDX, flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Get_Virt_State, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	mov	esi, OFFSET32 DMA_Ctrl1 ; point to controller 1 data
	mov	ecx, [eax.channel_num]
	push	eax
	cmp	cl, 4			;Q: channel of 2nd controller?
	jb	short gvs_ctrl_1	;   N:
	sub	cl, 4			;   Y:
	add	esi, SIZE DMA_Controller_State	; point to controller 2 data
gvs_ctrl_1:
	movzx	edx, [eax.mode]
	and	dl, NOT DMA_chan_sel

	mov	dh, [eax.ext_mode]

IFDEF allow_partial_virtualization
	bt	[esi.CTL_mode_set], cx	;Q: mode been set?
	jnc	short gvs_mode_set	;   Y:
	or	edx, 80000000h		;   N: set high bit in edx
gvs_mode_set:
ENDIF

	movzx	eax, [esi.CTL_mask]
	bt	eax, ecx		;Q: channel masked?
	jnc	short gvx_not_masked	;   N:
	or	dl, DMA_masked		;   Y: set flag bit
gvx_not_masked:
	movzx	eax, [esi.CTL_request]
	bt	eax, ecx		;Q: channel requested?
	jnc	short gvx_not_requested ;   N:
	or	dl, DMA_requested	;   Y: set flag bit
gvx_not_requested:
	pop	eax			; get DMA handle
	mov	esi, [eax.cur_addr]
	mov	ecx, [eax.cur_count]
	cmp	[eax.disable_cnt], 0	;Q: translation disabled?
	jne	short gvx_ret_phys	;   Y: return phys address (esi)

	add	esi, [ebx.CB_High_Linear]

gvx_ret_phys:
ifndef	NEC_98
	test	dh, _16_bit_xfer	;Q: word transfer on this channel?
	jz	short gvx_byte_cnt	;   N:
	shl	ecx, 1			;   Y: convert word cnt to byte cnt
	inc	ecx
gvx_byte_cnt:
endif
	inc	ecx			; return actual # of bytes

	; dump these in opposite order, because .S displays them backwards
	DMA_Q_OUT "      cnt=#eax  mode=#bx", ecx, edx
	DMA_Q_OUT "Get Virt State chn=#al  adr=#ebx", [eax.channel_num], esi
	ret

EndProc VDMAD_Get_Virt_State


BeginDoc
;******************************************************************************
;
;   VDMAD_Set_Virt_State
;
;   DESCRIPTION:
;	Modify the virtual state of a DMA channel.  This is service is used
;	when a channel owner wants to change the virtual state of a channel
;	from how the VM programmed it.	This might be used to split a DMA
;	request into smaller pieces, etc.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM handle
;		    If translation is enabled
;		      ESI = high linear address of the user's DMA region
;			  (high linear is used so that the DMA can proceed
;			   even if a different VM is actually running at the
;			   time of the transfer)
;		    Else
;		      ESI = physical byte address programmed (shifted left 1,
;			    for word ports)
;		    ECX = count in bytes
;		    DL	= mode (same as 8237 mode byte with channel # removed
;				and DMA_masked & DMA_requested set as
;				appropriate:
;				    DMA_masked	    channel masked and not ready
;						    for a transfer
;				    DMA_requested   software request flag set)
;		    DH	= extended mode (ignored on non-PS2 machines that don't
;					 have extended DMA capabilities)
;
;   EXIT:	    NOTHING
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Set_Virt_State, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	pushad

	; dump these in opposite order, because .S displays them backwards
	DMA_Q_OUT "      cnt=#eax  mode=#bx", ecx, edx
	DMA_Q_OUT "Set Virt State chn=#al  adr=#ebx", [eax.channel_num], esi
	mov	edi, OFFSET32 DMA_Ctrl1
	mov	ecx, [eax.channel_num]
	cmp	cl, 4			;Q: channel of 2nd controller?
	jb	short svs_ctrl_1	;   N:
	sub	cl, 4			;   Y:
	add	edi, SIZE DMA_Controller_State	; point to controller 2 data
svs_ctrl_1:

;
; update mask flag
;
	movzx	eax, [edi.CTL_mask]
	test	dl, DMA_masked		;Q: channel masked?
	jz	short svs_not_masked	;   N:
	bts	eax, ecx
	jmp	short svs_upd_mask
svs_not_masked:
	btr	eax, ecx
svs_upd_mask:
	mov	[edi.CTL_mask], al

;
; update requested flag
;
	movzx	eax, [edi.CTL_request]
	test	dl, DMA_requested	;Q: channel requested?
	jz	short svs_not_reqd	;   N:
	bts	eax, ecx
	jmp	short svs_upd_req
svs_not_reqd:
	btr	eax, ecx
svs_upd_req:
	mov	[edi.CTL_request], al

;
; update mode bytes
;

IFDEF allow_partial_virtualization
	BROKEN, jnz skip_virt_mode doesn't set EDI = DMA handle
	test	edx, 80000000h		;Q: set mode?
	jnz	short skip_virt_mode	;   N:
	btr	[edi.CTL_mode_set], cx	; indicate that mode has been set
ENDIF

	mov	edi, [esp.Pushad_EAX]	; EDI = DMA handle
	and	dl, NOT DMA_chan_sel
IFDEF DEBUG
ifndef	NEC_98
	IF_NOT_MCA_JMP	<short skip_PS2_debug>
endif
    ;
    ; verify that modes are in sync
    ;
	mov	ah, dl
    ;
    ; first copy undocumented auto-init bit
    ;
	mov	al, dh
	and	al, PS2_AutoInit
	shl	al, 3
	and	ah, NOT DMA_AutoInit
	or	ah, al
    ;
    ; copy memory access bits
    ;
	and	ah, NOT (DMA_type_write+DMA_type_read)
	test	dh, Transfer_data	    ;Q: verify op?
	jz	short svs_no_transfer	    ;	Y:
	test	dh, Write_Mem		    ;Q: write memory?
	jnz	short svs_PS2_write	    ;	Y:
	or	ah, DMA_type_read	    ;	N:
	jmp	short svs_PS2_read
svs_PS2_write:
	or	ah, DMA_type_write
svs_PS2_read:
	and	ah, NOT DMA_type_verify
svs_no_transfer:
	cmp	ah, dl
	Debug_OutNZ "VDMAD_Set_Virt_State with mode and ext-mode out of sync"
ifndef	NEC_98
skip_PS2_debug:
endif
ENDIF
	mov	[edi.mode], dl
	mov	[edi.ext_mode], dh
skip_virt_mode:

;
; update address
;
	mov	eax, [esp.Pushad_ESI]	; high linear address
	cmp	[edi.disable_cnt], 0	;Q: translation disabled?
	jne	short svx_set_phys	;   Y: high linear bias not included
	sub	eax, [ebx.CB_High_Linear] ; N: remove high linear bias
IFDEF DEBUG
; make sure that really was a high linear address
        jnc     svx_set_phys
        Debug_Out "VDMAD: Invalid high linear address in Set_Virt_State"        
ENDIF

svx_set_phys:
	mov	[edi.cur_addr], eax

;
; update count
;
	mov	eax, [esp.Pushad_ECX]	; byte length
	dec	eax
ifndef	NEC_98
	test	dh, _16_bit_xfer	;Q: word transfer on this channel?
	jz	short svx_byte_cnt	;   N:
	shr	eax, 1			;   Y: convert byte cnt to word cnt

svx_byte_cnt:
endif

IFDEF DEBUG
; make sure size doesn't exceed max: 64K for ISA and MCA (128K for 
; word count) and 16Mb for EISA
ifndef	NEC_98
        IF_EISA_JMP <short svx_check_EISA_max>
endif

        cmp     eax, 0FFFFh             ; 64K or less transfer?
        ja      svx_bad_size            ;   N: display message
                                        ; fallthrough will work okay
svx_check_EISA_max:
        cmp     eax, 0FFFFFFh           ; 16M or less transfer?
        jbe     svx_size_ok

svx_bad_size:
        Debug_Out "VDMAD: Transfer size exceeds maximum in Set_Virt_State"

svx_size_ok:
ENDIF
	mov	[edi.cur_count], eax
	mov	[edi.pgm_count], eax

	popad
	ret

EndProc VDMAD_Set_Virt_State

BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Virt_Count
;
;   DESCRIPTION:
;	Returns the counter value for the given DMA handle.  
;       Note that this is the counter value from the virtual DMA 
;       state, not the physical DMA counter value.
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    ECX = count in bytes
;
;   USES:	    ECX, flags
;
;==============================================================================
EndDoc

BeginProc VDMAD_Get_Virt_Count, SERVICE

	Validate_DMA_Handle

	mov	ecx, [eax.cur_count]

ifndef NEC_98
	test	[eax.ext_mode], _16_bit_xfer
        jz      SHORT gvc_byte_cnt

        ;
        ; convert to bytes
        ;

        shl     ecx, 1
        inc     ecx

gvc_byte_cnt:
endif ;ifndef NEC_98
        inc     ecx

        ret

EndProc VDMAD_Get_Virt_Count

BeginDoc
;******************************************************************************
;
;   VDMAD_Set_Virt_Count
;
;   DESCRIPTION:
;	Set the counter value for the given DMA handle.  
;       Note that this is the counter value for the virtual DMA 
;       state, not the physical DMA counter value.
;
;   ENTRY:	    EAX = DMA handle
;                   ECX = count in bytes
;
;   EXIT:	    Nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc

BeginProc VDMAD_Set_Virt_Count, SERVICE

	Validate_DMA_Handle

        push    ecx
        jecxz   SHORT svc_byte_cnt

        dec     ecx

ifndef NEC_98
	test	[eax.ext_mode], _16_bit_xfer
        jz      SHORT svc_byte_cnt
        shr     ecx, 1
endif ;ifndef NEC_98

svc_byte_cnt:
        mov     [eax.cur_count], ecx
        pop     ecx

        ret

EndProc VDMAD_Set_Virt_Count

BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Virt_Address
;
;   DESCRIPTION:
;	Returns the current address value for the given DMA handle.  
;       Note that this is the current address value of the virtual DMA 
;       state.  This is the value that is used when software in a virtual
;	machine reads and writes the io ports.  Thus depending on whether
;	the app has used the VDS services to turn translation off or not,
;	this address will be interpreted as either a phsical address or
;	a linear address.  Note that most apps do NOT turn off translation
;	so in most cases this address will be a high linear address, not
;	a physical address.  Changing this address via VDMAD_Set_Virt_Address
;	will change the values returned from io port reads of the DMA 
;	address registers.
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    ECX = current address
;
;   USES:	    ECX, flags
;
;==============================================================================
EndDoc

BeginProc VDMAD_Get_Virt_Address, SERVICE

	Validate_DMA_Handle

	    mov	ecx, [eax.cur_addr]
	
        ret

EndProc VDMAD_Get_Virt_Address

BeginDoc
;******************************************************************************
;
;   VDMAD_Set_Virt_Address
;
;   DESCRIPTION:
;	Set the current address value for the given DMA handle.  
;       Note that this address is used to calculate the return values for
;	port reads of the DMA current address register.  In most cases
;	this address is a high linear address.  In some cases when the
;	app has used VDS services to disable address translation, then
;	this address is a physical address.
;
;   ENTRY:	    EAX = DMA handle
;                   ECX = current address
;
;   EXIT:	    Nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc

BeginProc VDMAD_Set_Virt_Address, SERVICE

	Validate_DMA_Handle

        mov     [eax.cur_addr], ecx

        ret

EndProc VDMAD_Set_Virt_Address


BeginDoc
;******************************************************************************
;
;   VDMAD_Set_Phys_State
;
;   DESCRIPTION:
;	This service programs the DMA controller state for a channel.  All
;	that it needs to know is the desired mode.  The location and size
;	of the buffer is taken from the information passed to the service
;	VDMAD_Set_Region_Info which must be called previously.
;
;       This routine is for ISA systems.  If we are running on an MCA or
;       EISA machine, this routine is hooked out by a routine specialized
;       for those architecture.  See VDMAD_Set_PS2_Phys_State and 
;       VDMAD_Set_EISA_Phys_State.
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
EndDoc
BeginProc VDMAD_Set_Phys_State, ASYNC_SERVICE

	pushad

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

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
	jne	short region_ok 	;   Y:
	cmp	[edi.buffer_id], 0	;Q: buffer assigned?
	jne	short region_ok 	;   Y:
	Debug_Out "VDMAD: Attempted to start DMA without locking a region"

;	Fatal_Error

region_ok:
ENDIF
ifndef	NEC_98
	movzx	eax, [edi.xfer_base]
	mov	edx, [edi.channel_num]
	movzx	edx, [edx.base_ports]
	call	Out_DMA_Word		; write new base value

	movzx	eax, [edi.xfer_page]
	mov	edx, [edi.channel_num]
	movzx	edx, [edx.page_ports]
	out	dx, al			; write new page value

	mov	eax, [edi.region_size]
	dec	eax
	mov	edx, [edi.channel_num]
;
; Not EISA, so determine if byte or word transfer by channel #
;
	cmp	edx, 4			;Q: controller 2?
	jb	short sp_byte_xfer	;   N: use # of bytes
	shr	eax, 1			;   Y: convert to # of words

sp_byte_xfer:
	movzx	edx, [edx.count_ports]
	call	Out_DMA_Word		; write new count value
endif;	!NEC_98

IFDEF allow_partial_virtualization
	test	ebx, 80000000h		;Q: set mode?
	jnz	short skip_phys_mode	;   N:
ENDIF

	mov	eax, [edi.channel_num]
ifndef NEC_98
	mov	edx, eax
	and	al, DMA_chan_sel
	and	bl, NOT DMA_chan_sel
	or	al, bl			; add channel # to MODE command
	Cntrl_Const_Offset edx
	movzx	edx, [edx+DMA_consts.DMA_mode_port]
	out	dx, al			; write new mode
else;	NEC_98
	Cntrl_Const_Offset eax
	movzx	edx, [eax+DMA_consts.DMA_flipflop_port]
	out	dx, al			; clear the flip flop to write low byte

VDMAD_Set_Phys_mode_out:
	IO_Delay			; wait for I/O

	mov	eax, [edi.channel_num]	; get ch#
	mov	edx, eax		; copy ch# to EDX
	and	al, DMA_chan_sel	; 
	and	bl, NOT DMA_chan_sel	; 
	or	al, bl			; add channel # to MODE command
	Cntrl_Const_Offset edx		;
	movzx	edx, [edx+DMA_consts.DMA_mode_port] ;
	out	dx, al			; write new mode

	movzx	eax, [edi.xfer_page]	; get page
	mov	edx, [edi.channel_num]	; get ch#
	shl	edx, 1
	movzx	edx, [edx.page_ports]	; get page port address
	out	dx, al			; write new page value

VDMAD_Set_Phys_base:
	movzx	eax, [edi.xfer_base]	; 
	mov	edx, [edi.channel_num]	;
	push	ecx			;
	mov	ecx, edx		;
	shl	edx, 1
	movzx	edx, [edx.base_ports]	;
	call	Out_DMA_Word		; write new base value

	mov	eax, [edi.region_size]	;
	dec	eax			;
	mov	edx, [edi.channel_num]	; get ch#
	mov	ecx, edx		; set ch# to ECX
	shl	edx, 1
	movzx	edx, [edx.count_ports]	; get count port
	call	Out_DMA_Word		; write new count value
	pop	ecx			;
endif;	!NEC_98

skip_phys_mode:
	popad
	ret

EndProc VDMAD_Set_Phys_State

BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Phys_Count
;
;   DESCRIPTION:
;	Retrieves the current counter for the given DMA channel.
;
;       This is an asynchronous service.
;
;   ENTRY:	    EAX = DMA handle
;                   EBX = VM handle
;
;   EXIT:	    ECX = current dma channel count
;
;   USES:	    flags, ECX
;
;==============================================================================
EndDoc

BeginProc VDMAD_Get_Phys_Count, ASYNC_SERVICE

	Validate_DMA_Handle

        push    edx
        push    edi

        mov     edi, eax               
        mov     eax, [edi.channel_num]
ifndef	NEC_98
        movzx   edx, count_ports[eax]
else
        movzx   edx, count_ports[eax*2]
endif
        call    In_DMA_Count
        mov     ecx, eax
        mov     eax, edi

        pop     edi
        pop     edx

        ret

EndProc VDMAD_Get_Phys_Count

BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Phys_Status
;
;   DESCRIPTION:
;	Retrieves the status bytes of the DMA controllers.
;
;       This is an asynchronous service.
;
;   ENTRY:	    NOTHING
;
;   EXIT:	    AL = controller 1 status byte
;                   AH = controller 2 status byte
;
;   USES:	    flags, EAX
;
;==============================================================================
EndDoc

BeginProc VDMAD_Get_Phys_Status, ASYNC_SERVICE

        pushfd
        cli
        call    VDMAD_Read_DMA_Status
        popfd
        ret

EndProc VDMAD_Get_Phys_Status

BeginDoc
;******************************************************************************
;
;   VDMAD_Mask_Channel
;
;   DESCRIPTION:
;	This service physically masks a channel so that it will not attempt
;	any further DMA transfers.
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Mask_Channel, SERVICE

	Validate_DMA_Handle

	pushad

	pushfd
	cli

; Clear status mask bit for this channel.  If all status mask bits are
; are 0, reset the Terminal Count check procedures.

	movzx	ecx, [DMA_Ctrl1.CTL_status_mask]
	mov	ch, [DMA_Ctrl2.CTL_status_mask]
	jecxz	mask_channel
	push	ebx
	xor	ebx, ebx
	mov	edx, [eax.channel_num]
	bts	ebx, edx
	shl	ebx, 4
	shr	bl, 4
	not	ebx
	and	ecx, ebx
	pop	ebx
	mov	[DMA_Ctrl1.CTL_status_mask], cl
	mov	[DMA_Ctrl2.CTL_status_mask], ch
	jnz	SHORT mask_channel

; All status mask bits are 0, reset filter proc(s).

	push	eax
	mov	eax, [VDMAD_Hw_Int_Next]
	mov	[VDMAD_Hw_Int_Filter], eax
	pop	eax
	mov	[VDMAD_Check_TC], OFFSET32 VDMAD_NoCheck

	xor	esi, esi
	xchg	[DMA_TC_event], esi
	VMMCall Cancel_Global_Event

mask_channel:
	popfd

mc_phys_mask_entry:

	;
	; entry point for VDMAD_Phys_Mask_Channel
	;

	Validate_DMA_Handle

	DMA_Q_OUT 'Mask channel #al', [eax.channel_num]

	mov	[eax.owner_VM], 0
	mov	eax, [eax.channel_num]
	mov	edx, eax

	Cntrl_Const_Offset edx
	and	al, DMA_chan_sel
	or	al, 100b
	movzx	edx, [edx+DMA_consts.DMA_single_mask_p]
	out	dx, al			; write mask command
	IO_Delay

	popad
	ret

EndProc VDMAD_Mask_Channel


BeginDoc
;******************************************************************************
;
;   VDMAD_Phys_Mask_Channel
;
;   DESCRIPTION:    This service is used by VxD clients to simply mask the
;		    chl without checking for terminal count.  This service is
;		    asynchronous.
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    nothing
;
;   USES:	    nothing
;
;==============================================================================
EndDoc
BeginProc	VDMAD_Phys_Mask_Channel, async_service

	pushad				; set up stack for jump to common
	jmp	mc_phys_mask_entry	; jump to common code

EndProc	VDMAD_Phys_Mask_Channel


BeginDoc
;******************************************************************************
;
;   VDMAD_UnMask_Channel
;
;   DESCRIPTION:
;	This service physically unmasks a channel so that DMA transfers can
;	proceed.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_UnMask_Channel, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	pushad

	DMA_Q_OUT 'Unmask channel #al', [eax.channel_num]

	mov	edx, eax		; edx = DMA handle

	pushfd
	cli

	call	VDMAD_Read_DMA_Status

        ;
        ; The following mask clearing hack is for a race condition
        ; with applications where the TC status is not seen before
        ; the channel is unmasked again with auto-init mode.
        ; This is not an optimal solution but resolves the problems
        ; without much additional code.   Optimally, the code path
        ; to clear the Hw_Int_Filter when masking the channel
        ; should be invoked.
        ;

	xor	ecx, ecx				; not autoinit, set bit
	mov	eax, [edx.channel_num]			;   in status_mask
	bts	ecx, eax				;   for this channel
	shl	ecx, 4					;   and assign filter
	shr	cl, 4					;   proc(s)

        not     ecx
        and     [DMA_Ctrl1.CTL_status_mask], cl
        and     [DMA_Ctrl2.CTL_status_mask], ch
        not     ecx

	test	byte ptr [edx.mode], DMA_autoinit	; we don't care about
	jnz	SHORT umc_no_TC_chk			;   TC with autoinit

	or	[DMA_Ctrl1.CTL_status_mask], cl
	or	[DMA_Ctrl2.CTL_status_mask], ch

	not	ecx					; clr channel's status
	and	word ptr [VDMAD_DMA1_Status], cx	;   bit in shadow regs
	.errnz	VDMAD_DMA2_Status-VDMAD_DMA1_Status-1	;   so we don't think
							;   it's already done

	mov	esi, OFFSET32 VDMAD_HW_Check_TC
	mov	[VDMAD_Hw_Int_Filter], esi

	mov	esi, OFFSET32 VDMAD_IO_Check_TC
	mov	[VDMAD_Check_TC], esi

umc_no_TC_chk:
	popfd

umc_phys_unmask_entry:

	;
	; entry point for VDMAD_Phys_Unmask_Channel
	;

	mov	[edx.owner_VM], ebx

	mov	eax, [edx.channel_num]
        call    VDMADArb_Unmask_Channel                 ; Tell arbitrator
	mov	edx, eax

	Cntrl_Const_Offset edx
	and	al, DMA_chan_sel
	movzx	edx, [edx+DMA_consts.DMA_single_mask_p]

	out	dx, al			; write mask command
	IO_Delay

	popad
	ret

EndProc VDMAD_UnMask_Channel

BeginDoc
;******************************************************************************
;
;   VDMAD_Phys_Unmask_Channel
;
;   DESCRIPTION:    This service is used by VxD clients to simply unmask the
;		    chl without checking for terminal count.  This service is
;		    asynchronous.
;
;   ENTRY:	    EDX = DMA handle
;		    EBX = Current VM Handle
;
;   EXIT:	    nothing
;
;   USES:	    nothing
;
;==============================================================================
EndDoc
BeginProc	VDMAD_Phys_Unmask_Channel, async_service


	pushad				; set up stack for jump to common
	jmp	umc_phys_unmask_entry	; jump to common code

EndProc	VDMAD_Phys_Unmask_Channel

;******************************************************************************
;
;   In_DMA_Count
;
;   DESCRIPTION:    Input a count from a DMA port as 2 bytes, low then high.
;		    In case of EISA m/c., get the HIGH count also
;		    First the routine resets the DMA controller's flipflop
;		    so that the controller will be ready to send the low
;		    byte first.
;
;   ENTRY:	    DX is port address
;		    EDI = DMA handle
;
;   EXIT:	    EAX is the DMA word.
;
;   USES:	    flags
;
;==============================================================================
BeginProc IN_DMA_Count
ifndef	NEC_98	; BUGBUG: Need consideration.
        pushfd
        cli
endif
	push	edx

	mov	eax, [edi.channel_num]
	Cntrl_Const_Offset eax
	movzx	edx, [eax+DMA_consts.DMA_flipflop_port]

	out	dx, al			; clear the flip flop to read low byte
	IO_Delay

	pop	edx

; The DMA count register isn't latched so it's read at least twice to
; make sure the hi byte doesn't change while reading the low byte.

	push	ecx

	call	Read_DMA_Count
idc_rd_again:
	mov	ecx, eax
	call	Read_DMA_Count
	cmp	ah, ch
	jne	SHORT idc_rd_again

	pop	ecx
ifndef	NEC_98	; BUGBUG: Need consideration.
        popfd
endif
	ret

EndProc In_DMA_Count

BeginProc Read_DMA_Count

	push	edx

	in	al, dx			; low byte first
	IO_Delay
	xchg	al, ah
	in	al, dx			; now high byte
	xchg	al, ah			; restore AX

ifndef	NEC_98
	IF_NOT_EISA_JMP <SHORT idw_no_high_cnt>
	add	dx, DMA_E_Hoff
	shl	eax,16
	in	al, dx			; get EISA hi count byte
	rol	eax,16

idw_no_high_cnt:
endif
	pop	edx
	ret

EndProc Read_DMA_Count


;******************************************************************************
;
;   Out_DMA_Word
;
;   DESCRIPTION:    Output a word to a DMA port as 2 bytes, low then high.
;		    First the routine resets the DMA controller's flipflop
;		    so that the controller will be ready to receive the low
;		    byte first.
;
;   ENTRY:	    AX is word to send
;		    DX is port address
;		    EDI = DMA handle
;
;   EXIT:
;
;   USES:	    flags
;
;==============================================================================
BeginProc Out_DMA_Word
ifndef	NEC_98	; BUGBUG: Need consideration.
        pushfd
        cli
endif
	push	eax
	push	edx

	mov	eax, [edi.channel_num]
	Cntrl_Const_Offset eax
	movzx	edx, [eax+DMA_consts.DMA_flipflop_port]
	out	dx, al			; clear the flip flop to write low byte
	IO_Delay

	pop	edx
	pop	eax

	out	dx, al			; low byte first
	IO_Delay
	xchg	al, ah
	out	dx, al			; now high byte
	xchg	al, ah			; restore AX
ifndef	NEC_98	; BUGBUG: Need consideration.
        popfd
endif
	ret

EndProc Out_DMA_Word

;******************************************************************************
;
;   VDMAD_Dirty_Region
;
;   DESCRIPTION:    Since DMA does not go thru the processor, it does not
;		    set the DIRTY bit in the page tables.  Our memory
;		    manager uses the DIRTY bit for determining paging
;		    candidates, etc., so we need to go thru all pages and
;		    access them so that they will be marked as dirty.  We
;		    do this by:
;
;			1)  ebp <- starting linear address rounded down to
;				   first byte of first page
;			2)  read & write byte at ebp
;			3)  inc ebp to next linear page address
;			4)  loop back to (2), if ebp <= ending linear address
;
;   ENTRY:	    ESI = start page # (linear)
;		    ECX = # of pages in region
;
;   EXIT:	    none
;
;   USES:	    EAX, Flags
;
;==============================================================================
BeginProc VDMAD_Dirty_Region

	or	al, -1
	push	esi
	push	ecx
	shl	esi, 12 		; convert linear pg # to address
su_dirty_mem:
	and	byte ptr [esi], al	; dirty page
	add	esi, P_SIZE		;
	loop	su_dirty_mem
	pop	ecx
	pop	esi
	ret

EndProc VDMAD_Dirty_Region


BeginDoc
;******************************************************************************
;
;   VDMAD_Lock_DMA_Region
;
;   DESCRIPTION:
;	This service attempts to lock a region of memory for a DMA transfer.
;	It is called before a DMA transfer is started (before the physical
;	state is set for a channel and before it is unmasked.)
;
;	It first verifies that the region is mapped to contiguous pages of
;	physical memory.
;
;	Then it determines whether the region will result in a DMA bank (page)
;	wrap
;	    On AT class machines each channel has a base address register
;	    and a page address register.  The base address register is
;	    incremented after each byte or word transfered.  If the
;	    increment of this 16 bit register results in the roll over
;	    from FFFFh to 0, then the transfer wraps to the start of
;	    the DMA bank because the page register is not updated.
;	    Normally DOS watches for this condition and adjusts INT 13h
;	    parameters to split transfers to avoid this wrap, but DOS
;	    doesn't know anything about the difference between linear
;	    and physical addresses under Windows/386, so VDMAD checks
;	    again to prevent wrap from occurring undesirably.
;
;	If all of these checks are okay, then the service calls the memory
;	manager to lock the physical pages.
;
;   NOTE:
;	This routine does not check to see if the region is within some
;	physical maximum constraint.  If the region is lockable, then it locks
;	the memory, and it is up to the caller to check to see if the physical
;	region is acceptable.  If the region is not acceptable, then the caller
;	should unlock the region and perform a buffered DMA transfer.
;
;   ENTRY:	    ESI = linear address of actual DMA region
;		    ECX = # of bytes in DMA region
;		    DL	= 1b, if region must be aligned on 64K page boundary
;			= 10b, if region must be aligned on 128K page boundary
;
;   EXIT:	    Carry set, if lock failed
;			ECX = # of bytes that are lockable in the region
;			      (starting from ESI)
;			AL = 1 (DMA_Not_Contiguous), region not contiguous
;			   = 2 (DMA_Not_Aligned), region crossed physical
;			       alignment boundary
;			   = 3 (DMA_Lock_Failed), unable to lock pages
;		    ELSE
;			EDX = physical address of the DMA region
;			the region has been locked
;
;   USES:	    EAX, ECX, EDX, Flags
;
;
;
;==============================================================================
EndDoc
BeginProc VDMAD_Lock_DMA_Region, SERVICE

	pushfd
	pushad
	mov	ebp, esp
IFDEF DEBUG
	test	edx, 11b		;Q: alignment constraint?
	jz	short xfer_ok		;   N:
	movzx	edx, si
	add	edx, ecx
	test	[ebp.Pushad_EDX], 10b
	jz	short xfer_not_128K
	shr	edx, 1			; check wrap in 128Kb
xfer_not_128K:
	dec	edx
	cmp	edx, 10000h		;Q: transfer will result in a wrap?
	jb	short xfer_ok		;   N:
	Trace_Out "VDMAD warning: linear address of DMA transfer causes a wrap"
xfer_ok:
ENDIF

;
; set up for call to VDMAD_Scatter_Lock
;
	sub	esp, SIZE Extended_DDS_Struc + 2 * 4	; struc + 1 region entry
	mov	edi, esp		; EDI -> Extended_DDS_Struc
	mov	[edi.DDS_size], ecx
	mov	[edi.DDS_linear], esi

	xor	eax, eax
	mov	dword ptr [edi.DDS_seg], eax
	mov	dword ptr [edi.DDS_avail], eax	; zero DDS_avail & DDS_used
	mov	byte ptr [edi.DDS_avail], 1

    ; eax = 0, means we want physical regions
	VMMCall Get_Cur_VM_Handle
	VxDCall VDMAD_Scatter_Lock
	jc	short lk_failed

	mov	edx, dword ptr [edi.SIZE Extended_DDS_Struc]	; phys addr
	mov	ecx, dword ptr [edi.SIZE Extended_DDS_Struc+4]	; locked length

	pushfd
	test	byte ptr [ebp.Pushad_EDX], 11b	;Q: alignment constraint?
	jz	short lk_alignment_ok		;   N:
	mov	esi, 10000h
	test	byte ptr [ebp.Pushad_EDX], 10b	;Q: 128Kb wrap?
	jz	short lk_xfer_not_128K		;   N:
	shl	esi, 1				;   Y: check wrap in 128Kb
lk_xfer_not_128K:
	movzx	eax, dx
	add	eax, ecx
	dec	eax
	cmp	eax, esi		;Q: transfer will result in a wrap?
if DEBLEVEL GE DEBLEVELMAX
	Debug_OutAE 'DMA warning: locked region violates alignment constraint'
endif
	jae	short lk_bad_alignment	;   Y:

lk_alignment_ok:
	popfd
	jz	short lk_done		; jump if all locked

	mov	[ebp.Pushad_ECX], ecx	; return lockable length

	mov	al, DMA_Not_Contiguous	; guess at error return
	cmp	[edi.DDS_used], 1	;Q: not contiguous (needed more than 1 entry)?
	jg	short lk_unlock 	;   Y: unlock and return error
	mov	al, DMA_Lock_Failed	;   N: unlock and return failed
lk_unlock:
	movzx	eax, al
	mov	[ebp.Pushad_EAX], eax	; stuff error value
	mov	esi, [edi.DDS_linear]
	lea	eax, [esi+ecx+0FFFh]
	shr	esi, 12 		; first page #
	shr	eax, 12 		; page # of first page after region
	sub	eax, esi		; # of pages
	VMMCall _LinPageUnLock <esi, eax, 0>
	stc
	jmp	short lk_return

lk_bad_alignment:			; esi = alignment limit; eax = esi + overage
	popfd
	sub	esi, eax
	dec	esi			; -(# of bytes past alignment)
	add	esi, ecx		; # of lockable bytes
	mov	[ebp.Pushad_ECX], esi	; return lockable length
	mov	al, DMA_Not_Aligned	; unlock and return error
	jmp	lk_unlock		; ecx = locked length

lk_failed:
	xor	eax, eax
	mov	[ebp.Pushad_ECX], eax	; can't lock anything
	mov	al, DMA_Lock_Failed
	mov	[ebp.Pushad_EAX], eax
	stc
	jmp	short lk_return

lk_done:
	mov	[ebp.Pushad_EDX], edx
	clc

lk_return:
	pushfd
	pop	eax
	mov	[ebp + SIZE Pushad_Struc], eax	; replace saved flags
	mov	esp, ebp		; free alloc'ed stack buffer
	popad
	popfd
	ret

EndProc VDMAD_Lock_DMA_Region


BeginDoc
;******************************************************************************
;
;   VDMAD_Unlock_DMA_Region
;
;   DESCRIPTION:
;	This service unlocks the DMA region previously locked to a channel.
;	It is called after a DMA transfer is complete and the channel has
;	been masked.  So that the controller will not attempt any further
;	transfers to the programmed address.
;
;   ENTRY:	    ESI = linear address of actual DMA region
;		    ECX = # of bytes in DMA region
;
;   EXIT:	    Carry clear
;			memory unlocked
;		    Carry set
;			error
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Unlock_DMA_Region, SERVICE

	clc				; flag that pages should be marked dirty
	jecxz	short udr_exit
	pushad

VDMAD_Do_Unlock:			; alternate entry from
	pushfd				;   VDMAD_Unlock_DMA_Region_No_Dirty

	dec	ecx
	add	ecx, esi
	shr	esi, 12 		; start page # (linear)
	shr	ecx, 12 		; last page #
	sub	ecx, esi
	inc	ecx			; # of pages in region

	popfd				;Q: dirty pages?
	jc	short @F		;   N: called from VDMAD_Unlock_DMA_Region_No_Dirty
	call	VDMAD_Dirty_Region	;   Y:
@@:
	VMMCall _LinPageUnLock <esi, ecx, 0>
	sub	eax, 1			; set carry, if error
	popad
udr_exit:
	ret

EndProc VDMAD_Unlock_DMA_Region


;******************************************************************************
;
;   VDMAD_Unlock_DMA_Region_No_Dirty
;
;   DESCRIPTION:    Same as VDMAD_Unlock_DMA_Region, except that it doesn't
;		    mark each page in the region as dirty.
;
;   ENTRY:	    ESI = linear address of actual DMA region
;		    ECX = # of bytes in DMA region
;
;   EXIT:	    Carry clear
;			memory unlocked
;		    Carry set
;			error
;
;   USES:	    flags
;
;==============================================================================
BeginProc VDMAD_Unlock_DMA_Region_No_Dirty, SERVICE

	pushad
	stc
	jmp	VDMAD_Do_Unlock

EndProc VDMAD_Unlock_DMA_Region_No_Dirty


;------------------------------------------------------------------------------
IFDEF rpMemTst

VxD_DATA_SEG
Test_DDS Extended_DDS_Struc <28023h, 40335h, 0, 0, 28, 0>
	dd	32 DUP(?)
VxD_DATA_ENDS

BeginProc VDMAD_Terminate
	int 1
	xor	eax, eax
	mov	edi, OFFSET32 Test_DDS
	VxDCall VDMAD_Scatter_Lock
	jc	short rp_lock_failed
	VxDCall VDMAD_Scatter_Unlock
rp_lock_failed:
	clc
	ret
EndProc VDMAD_Terminate
ENDIF
;------------------------------------------------------------------------------


VxD_DATA_SEG
Scatter_Lock_Proc   dd	OFFSET32 VDMAD_Scatter_Page_Lock
VxD_DATA_ENDS

;******************************************************************************
;
;   VDMAD_Scatter_Page_Lock
;
;   DESCRIPTION:    Used to process list of page table entries.  It attempts
;		    to lock a page.
;
;   ENTRY:	    ESI = linear page #
;		    EBP -> temp dword on stack for _CopyPageTable call
;
;   EXIT:	    If lock failed
;		       Carry set
;		    Else
;		       Z-flag clear and EAX = page entry
;
;   USES:	    EAX, ECX, EDX, Flags
;
;==============================================================================
BeginProc VDMAD_Scatter_Page_Lock

	VMMCall _LinPageLock <esi, 1, 0>
	or	eax, eax		;Q: Lock done?
	jz	short vspl_lock_failed	;   N:

	VMMCall _CopyPageTable <esi, 1, ebp, 0>
	mov	eax, [ebp]
	and	eax, NOT (P_SIZE - 1)
	or	al, 1			; set bit 0 for present
	ret

vspl_lock_failed:
	xor	eax, eax
	stc
	ret

EndProc VDMAD_Scatter_Page_Lock

;******************************************************************************
;
;   VDMAD_Scatter_Page_Lock_w_holes
;
;   DESCRIPTION:    Used to process list of page table entries.  It first
;		    checks to see if the page is currently present, if not
;		    then it zeros the page entry in the DDS table, else it
;		    attempts to lock a page.
;
;   ENTRY:	    ESI = linear page #
;		    EBP -> temp dword on stack for _CopyPageTable call
;
;   EXIT:	    If lock failed
;			Carry set
;		    Elsif page present then
;			Z-flag clear and EAX = page entry
;		    Else
;			Z-flag set and EAX = 0
;
;   USES:	    EAX, ECX, EDX, Flags
;
;==============================================================================
BeginProc VDMAD_Scatter_Page_Lock_w_holes

	VMMCall _CopyPageTable <esi, 1, ebp, 0>
	mov	eax, [ebp]
	test	al, P_PRES		;Q: page present?
	jz	short vsplh_hole	;   N:

	VMMCall _LinPageLock <esi, 1, 0>
	or	eax, eax		;Q: Lock done?
	jz	short vsplh_lock_failed ;   N:

	VMMCall _CopyPageTable <esi, 1, ebp, 0>
	mov	eax, [ebp]
	and	eax, NOT (P_SIZE - 1)
	or	al, 1			; set bit 0 for present
	ret

vsplh_hole:
	xor	eax, eax		; clear EAX & Carry & set Z-flag
	ret

vsplh_lock_failed:
	xor	eax, eax
	stc
	ret

EndProc VDMAD_Scatter_Page_Lock_w_holes


BeginDoc
;******************************************************************************
;
;   VDMAD_Scatter_Lock
;
;   DESCRIPTION:    This service attempts to lock all pages mapped to a DMA
;		    region and return the actual physical addresses of the
;		    pages.
;
;   ENTRY:	    EBX = VM Handle
;		    AL = flags
;			 = 0, if the DDS table should be filled with physical
;			    addresses and sizes of the physical regions that
;			    make up the DMA region
;			 Bit 0 (DMA_SL_Get_PgTable_bit), set if the DDS table
;			    should be filled with the actual page table entries
;			 Bit 1 (DMA_SL_Allow_NPs_bit), set if not present page
;			    should not be locked
;			    (NOTE: ignored, if Bit 0 NOT set)
;		    EDI -> extended DDS (DMA Descriptor Structure)
;
;   EXIT:	    Carry clear
;			Z-flag set
;			    whole region was locked successfully
;			Z-flag clear
;			    partial region locked
;		    Carry set
;			nothing locked
;
;		    EDX = # of table entries needed to describe whole region
;		    DDS_size = # of bytes locked
;		    DDS table has been updated
;		    if request was for page table copy (AL=1 OR 3), then
;			ESI = offset into first page for start of the region
;
;   USES:	    EDX, ESI, Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Scatter_Lock, SERVICE

	Assert_VM_Handle ebx		; Debugging

	pushad

IFDEF DEBUG
; make sure the flag is valid
        and     al, NOT (DMA_SL_Get_PgTable + DMA_SL_Allow_NPs)
        jz      sl_flags_ok
        Debug_Out "VDMAD: Invalid flags passed to Scatter_Lock"
sl_flags_ok:
ENDIF
	movzx	eax, [edi.DDS_sel]
	or	eax, eax
	jz	short no_selector
	VMMcall _SelectorMapFlat, <ebx, eax, 0>
	cmp	eax, -1
	je	vsl_bad_sel

no_selector:
	mov	esi, [edi.DDS_linear]
	add	esi, eax
	mov	edx, esi		; edx = base linear address
	mov	ecx, [edi.DDS_size]
	or	ecx, ecx
	jnz	short non_0_size
	jmp	vsl_size0
non_0_size:
	dec	ecx
	add	ecx, esi
	jc	vsl_sizehuge
	shr	esi, 12 		; start page # (linear)
	shr	ecx, 12 		; last page #
	sub	ecx, esi
	inc	ecx			; # of pages in region

	test	byte ptr [esp.Pushad_EAX], DMA_SL_Get_PgTable
					;Q: copy page table request?
	je	phys_region_list	;   N:

;------------------------------------------------------------------------------
; lock region and return table of physical pages locked
					; ecx = # of pages in region
					; esi = linear page # of first page
					; edi -> EDDS
	mov	eax, edx
	and	eax, P_SIZE - 1
	mov	[esp.Pushad_ESI], eax
	mov	[esp.Pushad_EDX], ecx	; max required # of entries in table
	mov	[edi.DDS_used], 0
	movzx	ebx, [edi.DDS_avail]	; ebx = # of available entries
	or	ebx, ebx		;Q: any entries in the DDS table?
	jz	vsl_nothing		;   N:
	mov	[Scatter_Lock_Proc], OFFSET32 VDMAD_Scatter_Page_Lock
	test	byte ptr [esp.Pushad_EAX], DMA_SL_Allow_NPs
					;Q: allow np pages?
	jne	short all_present_req	;   N:
	mov	[Scatter_Lock_Proc], OFFSET32 VDMAD_Scatter_Page_Lock_w_holes
all_present_req:
	push	edi
	push	ecx
	push	ebp
	sub	esp, 4			; reserve 1 dword for page table copy
	mov	ebp, esp
	add	edi, SIZE Extended_DDS_Struc
vslp_lock_pages:
	push	ecx
	call	[Scatter_Lock_Proc]
	jc	short vslp_lock_failed
	dec	ebx			;Q: page entry available?
	js	short vslp_table_overflow ; N:
	cld
	stosd
	pop	ecx
	inc	esi
	loop	vslp_lock_pages

vslp_loop_done:
	add	esp, 4
	pop	ebp
	pop	edx			; original ecx page cnt
	pop	edi
	jecxz	short vslp_complete
	sub	edx, ecx		; # of pages locked
	jbe	vsl_nothing
	shl	edx, 12 		; # of bytes locked
	mov	eax, edx
	mov	edx, [esp.Pushad_ESI]	; offset into 1st page for start of region
	sub	eax, edx		; reduce size by offset into 1st page
vslp_nolock:
	mov	[edi.DDS_size], eax
vslp_complete:
	jmp	vsl_complete

vslp_table_overflow:			; can't record page in EDDS table, so
	VMMCall _LinPageUnLock <esi, 1, 0> ; unlock last page and return partial
					;    lock
vslp_lock_failed:
	pop	ecx
	jmp	vslp_loop_done

;------------------------------------------------------------------------------
; lock region and return list of sub-regions (physical address & size)
phys_region_list:
					; ecx = # of pages in region
					; esi = linear page # of first page
					; edi -> EDDS
	mov	[esp.Pushad_ESI], 0
	mov	[esp.Pushad_EDX], ecx	; max required # of entries in table
	mov	[edi.DDS_used], 0
	movzx	ebx, [edi.DDS_avail]	; ebx = # of available regions
	or	ebx, ebx		;Q: any entries in the DDS table?
	jz	vsl_nothing		;   N:
	push	edi
	push	ecx
	push	ebp
	push	[edi.DDS_size]
	sub	esp, 3 * 4		; reserve 3 dwords for page table copy

vsl_region_size equ dword ptr [ebp+12]
locked_bytes	equ dword ptr [ebp+8]
next_cons_page	equ dword ptr [ebp+4]
page_copy	equ dword ptr [ebp]

	mov	ebp, esp
	add	edi, (SIZE Extended_DDS_Struc) - 4
	mov	eax, edx
	and	eax, P_SIZE - 1
	neg	eax
	mov	[edi+8], eax		; reduce first region lock size
	mov	[locked_bytes], eax
	mov	[next_cons_page], -1
vsl_lock_pages:
	push	ecx
	VMMCall _LinPageLock <esi, 1, 0>
	or	eax, eax		;Q: Lock done?
	jz	vsl_lock_failed
	VMMCall _CopyPageTable <esi, 1, ebp, 0>
	mov	eax, [page_copy]
	and	eax, NOT (P_SIZE - 1)	; clear flag bits
	cmp	eax, [next_cons_page]	;Q: consecutive page?
	je	short vsl_add_page	;   Y:
	add	edi, 4
	dec	ebx			;Q: region entry available?
	js	short vsl_table_overflow;   N:
	cld
	stosd
	cmp	[next_cons_page], -1	;Q: first page locked?
	je	short vsl_1st_page	;   Y:
	mov	dword ptr [edi], 0	;   N: init region size to 0
	jmp	short vsl_add_page
vsl_1st_page:
	mov	edx, [edi]
	neg	edx			; offset of start of region in 1st page
	add	[edi-4], edx		; adjust base of first region
vsl_add_page:
	mov	edx, P_SIZE
	add	[edi], edx
	add	eax, edx
	add	[locked_bytes], edx
	mov	[next_cons_page], eax	; next consecutive page
	pop	ecx
	inc	esi
	loop	vsl_lock_pages
	mov	edx, [locked_bytes]
	sub	edx, [vsl_region_size]	; # of extra bytes locked
	sub	[edi], edx		; reduce size of last region
vsl_loop_done:
	mov	eax, [locked_bytes]
	add	esp, 4 * 4		; discard 3 temp dwords & region size
	pop	ebp
	pop	edx
	pop	edi
	jecxz	short vsl_complete
	sub	edx, ecx		; # of pages locked
	jbe	short vsl_nothing
	mov	[edi.DDS_size], eax

vsl_complete:
	movzx	eax, [edi.DDS_avail]
	sub	eax, ebx		; eax = # of regions filled in
	mov	[edi.DDS_used], ax
	mov	[esp.Pushad_EDX], eax	; save # of entries needed
	or	ecx, ecx		; clear carry, & set Z, if complete
	jmp	short scatter_lock_exit

vsl_table_overflow:			; can't record page in EDDS table, so
	VMMCall _LinPageUnLock <esi, 1, 0> ; unlock last page and return partial
					;    lock
vsl_lock_failed:
	pop	ecx
	jmp	short vsl_loop_done

vsl_bad_sel:
IFDEF DEBUG
	movzx	eax, [edi.DDS_sel]
	Debug_Out 'WARNING: VDMAD_Scatter_Lock with bad region selector #ax'
ENDIF
	jmp	short vsl_nothing

vsl_size0:
	Debug_Out 'WARNING: VDMAD_Scatter_Lock with size of 0'
	jmp	short vsl_nothing

vsl_sizehuge:
	Debug_Out 'WARNING: VDMAD_Scatter_Lock with end of region > 4Gb'
	jmp	short vsl_nothing

vsl_nothing:
	mov	[edi.DDS_size], 0
	stc

scatter_lock_exit:
	popad
	ret
EndProc VDMAD_Scatter_Lock


BeginDoc
;******************************************************************************
;
;   VDMAD_Scatter_Unlock
;
;   DESCRIPTION:    This service attempts to unlock all pages locked by a
;		    previous call to VDMAD_Scatter_Lock
;
;   ENTRY:	    EBX = VM Handle
;		    AL = flags
;			 = 0, if the DDS table is filled with physical
;			    addresses and sizes of the physical regions that
;			    make up the DMA region
;			 Bit 0 (DMA_SL_Get_PgTable_bit), set if the DDS table
;			    is filled with the actual page table entries
;			 Bit 1 (DMA_SL_Allow_NPs_bit), set if not present page
;			    holes exist in the region
;			    (NOTE: ignored, if Bit 0 NOT set)
;			 Bit 2 (DMA_SL_Dont_Dirty_bit), set if pages should
;			    NOT be marked as dirty
;			    (NOTE: if bits 0 and 1 are set, and 2 is clear,
;			     then not present pages are not marked)
;		    EDI -> extended DDS (DMA Descriptor Structure)
;			(If Bits 0 and 1 are set, then the table at the end
;			 of the DDS is not required, to unlock the previously
;			 locked pages; otherwise the table isn't used and
;			 caller need not necessary maintain the table after
;			 the lock call.)
;
;   EXIT:	    Carry clear
;			Lock counts have been decremented.  If no other VxD's
;			had pages locked, then the pages have been unlocked.
;		    Carry set
;			The memory was not locked.
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Scatter_Unlock, SERVICE

	Assert_VM_Handle ebx		; Debugging

	pushad

IFDEF DEBUG
; validate flag bits
        and     al, NOT (DMA_SL_Get_PgTable + DMA_SL_Allow_NPs + DMA_SL_Dont_Dirty)
        jz      su_flags_ok
        Debug_Out "VDMAD: Invalid flags passed to Scatter_Unlock"
su_flags_ok:
ENDIF     
	movzx	eax, [edi.DDS_sel]
	or	eax, eax
	jz	short vsu_no_selector
	VMMcall _SelectorMapFlat, <ebx, eax, 0>

vsu_no_selector:
	mov	esi, [edi.DDS_linear]
	add	esi, eax
	mov	eax, [esp.Pushad_EAX]
	and	al, DMA_SL_Get_PgTable OR DMA_SL_Allow_NPs
	cmp	al, DMA_SL_Get_PgTable OR DMA_SL_Allow_NPs
					;Q: page table copy with np holes?
	je	short unlock_scan	;   Y:
	mov	ecx, [edi.DDS_size]
	dec	ecx
	add	ecx, esi		; ecx = last byte of region
	shr	esi, 12 		; start page # (linear)
	shr	ecx, 12 		; last page #
	sub	ecx, esi
	inc	ecx			; # of pages in region
	test	byte ptr [esp.Pushad_EAX], DMA_SL_Dont_Dirty	;Q: dirty page?
	jnz	short su_dont_dirty				;   N:
	call	VDMAD_Dirty_Region
su_dont_dirty:
	VMMCall _LinPageUnLock <esi, ecx, 0>
	sub	eax, 1			; carry set, if failed (eax = 0)
su_exit:
	popad
	ret

unlock_scan:
	mov	ebp, esi
	and	ebp, NOT P_SIZE-1	; linear addr of first page of region
	shr	esi, 12 		; start page # (linear)
	movzx	ecx, [edi.DDS_used]
	lea	edi, [edi+(SIZE Extended_DDS_Struc)]
;
; Since DMA does not go thru the processor, it does not set the DIRTY bit
; in the page tables.  Our memory manager uses the DIRTY bit for determining
; paging candidates, etc., so we need to go thru all pages and access them
; so that they will be marked as dirty.  We do this by:
;
;   1)	ebp <- starting linear address rounded down to first byte of page (esi)
;   2)	if page (esi) is present then read & write byte at ebp
;   3)	inc ebp to next linear page address & esi to next linear page #
;   4)	loop back to (2), if pages left in table
;
unlock_lp:
	test	byte ptr [edi], P_PRES	;Q: page present?
	jz	short skip_unlock	;   N:
	test	byte ptr [esp.Pushad_EAX], DMA_SL_Dont_Dirty	;Q: dirty page?
	jnz	short su_dont_dirty_page			;   N:
	and	byte ptr [ebp], 0FFh				;   Y:
su_dont_dirty_page:
	push	ecx
	VMMCall _LinPageUnLock <esi, 1, 0>
	pop	ecx
skip_unlock:
	inc	esi
	add	edi, 4
	add	ebp, P_SIZE
	loop	unlock_lp
	clc
	jmp	su_exit

EndProc VDMAD_Scatter_Unlock


BeginDoc
;******************************************************************************
;
;   VDMAD_Request_Buffer
;
;   DESCRIPTION:    This device reserves the DMA buffer for a DMA transfer.
;
;   ENTRY:	    ESI = linear address of actual DMA region (NOT REALLY USED!)
;		    ECX = # of bytes in DMA region
;
;   EXIT:	    Carry clear
;			EBX = buffer ID
;			EDX = the physical address of the buffer
;
;		    Carry set
;			AL = 5 (DMA_Buffer_Too_Small), region request is
;			       too large for buffer
;			   = 6 (DMA_Buffer_In_Use), buffer already in use
;
;   USES:	    EAX, EBX, Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Request_Buffer, SERVICE

	push	ecx
	push	esi

	add	ecx, 0FFFh
	shr	ecx, 12 			; # of pages necessary

	mov	dl, DMA_Buffer_Too_Small	; DL = Correct error to return
	xor	esi, esi			; ESI = Best so far
	mov	eax, OFFSET DMA_Buff_Table
RB_Find_Buff_Loop:
	mov	ebx, [eax.DB_Size_Pages]
	cmp	ecx, ebx			; Q: VDMAD buffer big enough?
	ja	RB_Not_Large_Enough		;    N: Keep looking
	TestMem [eax.DB_State], Buffer_In_Use	; Q: Already in use?
	jz	RB_Have_Buffer			;    N: Great!
;
;   Set the possible error return to "In Use" EVEN IF a future attempt to
;   grow a buffer fails.
;
	mov	dl, DMA_Buffer_In_Use
	jmp	RB_Skip
;
;   Found a buffer, but it's not big enough.
;
RB_Not_Large_Enough:
	TestMem [eax.DB_State], Buffer_In_Use	; Q: Already in use?
	jnz	RB_Skip 			;    Y: Don't try this one

	test	esi, esi			; Q: Any "best so far"?
	jz	RB_Best_So_Far			;    N: This one's the best
	cmp	ebx, [esi.DB_Size_Pages]	;    Y: Q: New one bigger?
	jbe	RB_Skip 			;	   N: Skip this
RB_Best_So_Far:
	mov	esi, eax			; Remember this one to grow
RB_Skip:
	add	eax, SIZE DMA_Buff_Struc
	cmp	eax, OFFSET32 End_DMA_Buff_Table
	jb	RB_Find_Buff_Loop

	test	esi, esi			; Q: Did we find any to grow?
	jnz	RB_Alloc_Buffer 		;    Y: Try to alloc and grow
						;    N: Return "In use error"
	mov	al, DMA_Buffer_In_Use
RB_Exit_Error:
	pop	esi
	pop	ecx
	stc
	ret

;
;   Great!  Found a suitable buffer.  Handle is in EAX.
;
RB_Have_Buffer:
	SetFlag [eax.DB_State], Buffer_In_Use
	VMMCall Get_Cur_VM_Handle
	mov	[eax.DB_Owner], ebx
	mov	edx, [eax.DB_Phys]

	mov	ebx, eax
	sub	ebx, OFFSET32 DMA_Buff_Table-1

	cmp	ebx, 1				; Q: Is this the primary buff?
	jne	RB_Set_Time_Out 		;    N: Try to free it later

RB_Exit_Success:
	pop	esi
	pop	ecx
	clc
	ret

RB_Set_Time_Out:
	xor	esi, esi
	cmp	[VDMAD_Free_Buff_Timeout], esi	; Q: Time-Out already sched?
	jne	RB_Exit_Success 		;    Y: Done

	mov	eax, DMA_FREE_TIMEOUT
	mov	esi, OFFSET32 VDMAD_Time_Out
	VMMcall Set_Global_Time_Out
	mov	[VDMAD_Free_Buff_Timeout], esi
	jmp	RB_Exit_Success



;
;   Allocate the DMA buffer.  Handle is in ESI.  Correct error code to return
;   in DL
;
    PUBLIC RB_Alloc_Buffer  ; Make debugging easier
RB_Alloc_Buffer:
	pushad

	mov	eax, MinBufferSizePages
	cmp	ecx, eax
	ja	RB_SizeOK
	mov	ecx, eax
RB_SizeOK:
	mov	edi, ecx		    ; Save size in EDI
					    ; convert size to # of pages
	mov	eax, ecx		    ; (examples:       3    7	  11
					    ;		      12K  28K	  44K
	dec	eax			    ; # pages - 1     10b  111b  1011b
	bsr	cx, ax			    ; max power of 2   1    2	   3
	inc	cl			    ; shift cnt        2    3	   4
	mov	eax, 1
	shl	eax, cl 		    ; mask + 1	     100b 1000b 10000b
	dec	eax			    ; mask	      11b  111b  1111b
					    ; alignment       16K  32K	  64K
	and	eax, 1Fh		    ; maximum alignment is 128K
	push	edx			    ; Reserve dword on stack
	mov	edx, esp		    ; Phys addr saved here.

	VMMCall _PageAllocate <edi,PG_SYS,0,eax,0,[DMA_Max_Physical],edx,PageUseAlign+PageContig+PageFixed>
	test	eax, eax
	Debug_OutZ "VDMAD ERROR:  Could not alloc DMA buffer"
	jz	SHORT RB_No_Buffer_No_Way

	pop	[esi.DB_Phys]		    ; Get new phys address
	mov	[esi.DB_Size_Pages], edi    ; And size
	mov	edx, [esi.DB_Linr]	    ; Get old lin addr
	mov	[esi.DB_Linr], eax	    ; Set new one
	test	edx, edx
	jz	SHORT RB_No_Old_Buffer
	VMMcall _PageFree, <edx, 0>
RB_No_Old_Buffer:
	popad
	mov	eax, esi		    ; Move handle to EAX
	jmp	RB_Have_Buffer

RB_No_Buffer_No_Way:
	Debug_Out "VDMAD ERROR:  Unable to allocate buffer of #DI pages"
	pop	ecx				; Toss phys addr
	popad					; Huck pushad regs
	mov	al, dl				; Get correct error code
	jmp	RB_Exit_Error

EndProc VDMAD_Request_Buffer


;******************************************************************************
;
;   VDMAD_Time_Out
;
;   DESCRIPTION:
;	Walk the list of buffers and free any that haven't been used for
;	a while.
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Time_Out


	VMMcall Get_Last_Updated_System_Time
	xor	ecx, ecx			; Const 0
	xor	esi, esi			; Non-zero if any buff remain
	mov	ebx, OFFSET32 DMA_Buff_Table + SIZE DMA_Buff_Struc
VTO_Loop:
	cmp	[ebx.DB_Size_Pages], ecx
	je	VTO_Next_One
	TestMem [ebx.DB_State], Buffer_In_Use
	jnz	VTO_Kept_One
	mov	edx, eax
	sub	edx, [ebx.DB_Last_Time]
	cmp	edx, 2*60*1000
	jb	VTO_Kept_One
	Trace_Out "VDMAD INFO:  Freeing extra buffer #EBX"

	pushad
	mov	edi, ebx
	VMMcall _PageFree, <[ebx.DB_Linr], 0>
	mov	ecx, SIZE DMA_Buff_Struc / 4
	xor	eax, eax
	cld
	rep stosd
	popad
	jmp	VTO_Next_One

VTO_Kept_One:
	inc	esi
VTO_Next_One:
	add	ebx, SIZE DMA_Buff_Struc
	cmp	ebx, OFFSET32 End_DMA_Buff_Table
	jb	VTO_Loop

	test	esi, esi
	jz	VTO_Set_Handle

	mov	eax, DMA_FREE_TIMEOUT
	mov	esi, OFFSET32 VDMAD_Time_Out
	VMMcall Set_Global_Time_Out

VTO_Set_Handle:
	mov	[VDMAD_Free_Buff_Timeout], esi
	ret

EndProc VDMAD_Time_Out


BeginDoc
;******************************************************************************
;
;   VDMAD_Release_Buffer
;
;   DESCRIPTION:
;	Release the VDMAD buffer assigned to a DMA channel from a previous
;	VDMAD_Request_Buffer call.  This routine exits from a critical section
;	and the DMA buffer will now be available for other users.  Any data
;	in the buffer is not automatically copied, so VDMAD_Copy_From_Buffer
;	must be called if the data is important.
;
;   ENTRY:	    EBX = Buffer id
;
;   EXIT:	    Carry clear
;			buffer released
;		    Carry set
;			bad ID
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Release_Buffer, SERVICE

	push	eax
	push	ebx

	Convert_ID_To_Handle ebx, VDMAD_RB_Error

	xor	eax, eax
	TestMem [ebx.DB_State], Buffer_In_Use	;Q: in use?
	Debug_OutE "VDMAD:  Buffer operation called when it isn't in use"
	jz	VDMAD_RB_Error

	mov	[ebx.DB_State], eax
	mov	[ebx.DB_Owner], eax
	VMMcall Get_Last_Updated_System_Time
	mov	[ebx.DB_Last_Time], eax

	clc
release_exit:
	pop	ebx
	pop	eax
	ret

VDMAD_RB_Error:
	stc
	jmp	release_exit

EndProc VDMAD_Release_Buffer

;******************************************************************************
;
;    VDMAD_Partial_DMA
;
;    DESCRIPTION:
;	This is called when an attempt is made to read the count or address reg.
;	It reads the word count register, compares it with the VMs word count
;	and if the word count register is smaller, then it copies the difference
;	from the DMA buffer to VMs buffer if required and updates the word
;	count register for that VM.
;
;    ENTRY:		ESI = DMA controller data in global data
;			ECX = channel # (0-3)
;			EBX = VM handle
;			EDX = Port address.
;
;    EXIT:		None.
;
;    USES:		everything except ESI, ECX
;
;******************************************************************************

BeginProc VDMAD_Partial_DMA

; if DMA is not requested or unmasked, then get out.

	bt	DWORD PTR [esi.CTL_request], ecx
	jc	SHORT vpd_upd_count

	bt	DWORD PTR [esi.CTL_mask], ecx
	jc	SHORT vpd_done

vpd_upd_count:
	call	VDMAD_Get_DMA_Handle_For_Ctrl	; eax = DMA_Handle

	mov	edi,eax			; edi = DMA_Handle (of the channel)
	call	In_DMA_Count		; get current count in eax

	test	[edi.mode], DMA_AutoInit	; calc AutoInit DMA count/addr
	jnz	SHORT vpd_upd_ai_cnt		;   differently


VDMAD_Partial_Complete	LABEL	NEAR	; VDMAD_DMA_Complete entry point

	mov	edx, [edi.cur_count]
	push	eax				; save new count
; Now we will increment the two counts and force them to their size in the
; DMA controller.  This way, if TC was reached, then the registers will contain
; zero. If TC was reached before, then edx will be zero and eax will always
; be greater than or equal to edx and hence no copy. If TC is reached now, then
; eax will be zero and there will be a copy. If TC is not reached in either
; case, then "and" operations are nops.
	inc	edx
	inc	eax
	mov	ebx, 0FFFFFFh
ifndef	NEC_98
	IF_EISA_JMP <SHORT vpd_eisa_1>
endif
	shr	ebx, 8
vpd_eisa_1:
	and	eax, ebx
	and	edx, ebx
	pop	ebx				; retrieve new count
	cmp	eax, edx			; compare new count,old count
	jae	SHORT vpd_done			; wrong readout, no upd,copy

	mov	[edi.cur_count], ebx		; update word count.

ifndef	NEC_98
	test	[edi.ext_mode], _16_bit_xfer	; word xfer channel?
	jz	short vpd_byte_xfer
	shl	eax,1
	shl	edx,1
vpd_byte_xfer:
endif
	add	[edi.cur_addr], edx		; update address
	sub	[edi.cur_addr], eax

	cmp	[edi.buffer_id], 0		; don't copy if not buffered
	jz	SHORT vpd_done

	test	[edi.mode],DMA_type_write	; only copy if write
	jz	SHORT vpd_done

	push	ecx				; active buffered memory write,
	push	esi				;   copy latest data to VM's
	call	VDMAD_Partial_Copy		;   memory.
	pop	esi
	pop	ecx

	jmp	SHORT vpd_done

; Auto init DMA has a different calculation in that the count and address
; continue after TC with the last programmed values.  We don't support
; auto init DMA to/from the DMA buffer so there is no need to copy data.

vpd_upd_ai_cnt:
	mov	[edi.cur_count], eax	; virt count = physical

	sub	eax, [edi.pgm_count]	; diff between programmed & current
	neg	eax			;   count is the diff between pgm'd
	bt	[esi.CTL_flags], ecx	;   and current address
	jnc	SHORT vpd_cnt_bytes
	shl	eax, 1
vpd_cnt_bytes:
	add	eax, [edi.pgm_addr]
	mov	[edi.cur_addr], eax

vpd_done:
	ret

EndProc VDMAD_Partial_DMA

;*****************************************************************************
;
; VDMAD_Partial_Copy:
;
; Description:
;	Copy data from DMA buffer to VM's memory.  The amount copied is
;	the difference between the current and last values of the DMA
;	count register.
; Entry:
;	EAX = Current DMA count in bytes
;	EDX = Last DMA count in bytes
;	EDI = DMA_Handle of the channel.
; Exit:
;	EAX, EBX, ECX, EDI, ESI
; Uses:
;	Everything
;*****************************************************************************

BeginProc VDMAD_Partial_Copy

	movzx	ebx, [edi.buffer_id]
	;;;; ~~~????? mov     bh, [edi.locked_pages]
	mov	esi, [edi.region_base]		; linear address
	mov	edi, [edi.region_size]

; region_size must be always the size of the DMA requested.

	sub	edi, edx			; offset in buffer
	mov	ecx, edx
	sub	ecx, eax			; size of copy
	VxdCall	VDMAD_Copy_From_Buffer
	jnc	short Partial_Copy_Done

	DMA_Q_OUT "PDMA: Failed copy"	

Partial_Copy_Done:

	ret
EndProc VDMAD_Partial_Copy

;******************************************************************************
;
;   Verify_Copy_Params
;
;   DESCRIPTION:    Verify parameters for VDMAD_Copy_To_Buffer and
;		    VDMAD_Copy_From_Buffer
;
;   ENTRY:	    EBX = buffer id
;		    EDI = offset within buffer for start of copy
;		    ECX = size
;
;   EXIT:	    Carry clear
;			params okay
;			EAX -> Buffer handle
;		    Carry set
;			AL = error code
;
;   USES:
;
;==============================================================================

BeginProc Verify_Copy_Params

	Convert_ID_To_Handle eax, ct_invalid_buffer

	push	edi
	add	edi, ecx
	jc	short ct_invalid
	dec	edi
	shr	edi, 12
	cmp	edi, [eax.DB_Size_Pages]
	jae	short ct_invalid
	pop	edi
	clc
	ret

ct_invalid:
	pop	edi
	Debug_Out 'VDMAD: invalid starting offset or size for buffer copy'
	mov	al, DMA_Copy_Out_Range
	stc
	ret

ct_invalid_buffer:
	mov	al, DMA_Invalid_Buffer
	stc
	ret


EndProc Verify_Copy_Params



BeginDoc
;******************************************************************************
;
;   VDMAD_Copy_To_Buffer
;
;   DESCRIPTION:
;	This service allows another device to copy data into the VDMAD buffer
;	from the actual DMA region associated with the buffer.	This service
;	is called after VDMAD_Request_Buffer and before starting a memory
;	read transfer.
;
;   ENTRY:	    EBX = buffer id
;		    ESI = region linear
;		    EDI = offset within buffer for start of copy
;		    ECX = size
;
;   EXIT:	    Carry clear
;			data copied from DMA region into buffer
;		    Carry set
;			AL = 0Ah (DMA_Invalid_Buffer) - invalid buffer
;				 id supplied
;			   = 0Bh (DMA_Copy_Out_Range) - (ESI + ECX) is
;				 greater than buffer size
;
;   USES:	    EAX, flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Copy_To_Buffer, SERVICE

	call	Verify_Copy_Params
	jc	SHORT copy_exit

	pushad
	add	edi, [eax.DB_Linr]
	jmp	short transfer_region

EndProc VDMAD_Copy_To_Buffer

BeginDoc
;******************************************************************************
;
;   VDMAD_Copy_From_Buffer
;
;   DESCRIPTION:
;	This service allows another device to copy data from the VDMAD buffer
;	to the actual DMA region associated with the buffer.  This service
;	is called after VDMAD_Request_Buffer, after a memory write transfer
;	and before VDMAD_Release_Buffer.
;
;   ENTRY:	    EBX = buffer id
;		    ESI = region linear
;		    EDI = offset within buffer for start of copy
;		    ECX = size
;
;   EXIT:	    Carry clear
;			data copied from buffer into DMA region
;		    Carry set
;			AL = 0Ah (DMA_Invalid_Buffer) - invalid buffer
;				 id supplied
;			   = 0Bh (DMA_Copy_Out_Range) - (ESI + ECX) is
;				 greater than buffer size
;
;   USES:	    EAX, flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Copy_From_Buffer, SERVICE

	call	Verify_Copy_Params
	jc	SHORT copy_exit

	pushad
	xchg	esi, edi
	add	esi, [eax.DB_Linr]

transfer_region:
	cld
	mov	eax, ecx
	shr	ecx, 2			; # of dwords to transfer

VDMAD_Fault_Start label near
	rep	movsd
	mov	ecx, eax
	and	ecx, 3
	rep	movsb			; transfer any extra bytes
VDMAD_Fault_End label near
	popad
copy_exit:
	ret

IFDEF DEBUG
BeginProc VDMAD_Copy_Fault

	Trace_Out "VDMAD_Copy_To/From_Buffer exception: usually caused by app not setting base"
	jmp	VDMAD_Fault_End

EndProc VDMAD_Copy_Fault
ELSE
VDMAD_Copy_Fault EQU VDMAD_Fault_End
ENDIF

EndProc VDMAD_Copy_From_Buffer


BeginDoc
;******************************************************************************
;
;   VDMAD_Get_EISA_Adr_Mode
;
;   DESCRIPTION:    Get EISA extended mode - the hardware doesn't allow for
;		    reading the extended mode for a channel, so VDMAD defaults
;		    to the ISA defaults (channels 0-3 are byte channels and
;		    5-7 are word channels with word addresses and counts)  An
;		    INI switch can specify an alternate setting.
;
;   ENTRY:	    EAX = Channel # (0..7) or
;			  DMA Handle
;
;   EXIT:	    CL = 0 - 8-bit I/O, with count in bytes
;		    CL = 1 - 16-bit I/O, with count in words and adr shifted
;		    CL = 2 - 32-bit I/O, with count in bytes
;		    CL = 3 - 16-bit I/O, with count in bytes
;
;   USES:	    ECX, Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Get_EISA_Adr_Mode, SERVICE

ifndef	NEC_98
	push	eax
	cmp	eax, 7
	jbe	short get_w_ch_num

	Validate_DMA_Handle
	mov	eax, [eax.channel_num]

get_w_ch_num:
	mov	cl, [eax+DMA_EISA_Ext_Modes]	; EISA_Ext_Modes default
	and	cl, DMA_EM_Chan_Size		; to ISA values, so we can
	shr	cl, 2				; always allow this service

.errnz DMA_EM_Chan_Size xor 1100b	; assumming size is in bits 2 & 3
	pop	eax
else;	NEC_98
	mov	cl,1
endif;	!NEC_98
	ret

EndProc VDMAD_Get_EISA_Adr_Mode


BeginDoc
;******************************************************************************
;
;   VDMAD_Set_EISA_Adr_Mode
;
;   DESCRIPTION:    Set EISA extended mode
;
;   ENTRY:	    EAX = Channel # (0..7) or
;			  DMA Handle
;		    CL = 0 - 8-bit I/O, with count in bytes
;		    CL = 1 - 16-bit I/O, with count in words and adr shifted
;		    CL = 2 - 32-bit I/O, with count in bytes
;		    CL = 3 - 16-bit I/O, with count in bytes
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Set_EISA_Adr_Mode, SERVICE

ifndef	NEC_98
	IF_NOT_EISA_JMP <SHORT seam_exit>   ; don't change if not EISA machine

	push	eax
	push	ecx
	cmp	eax, 7
	jbe	short set_w_ch_num

	Validate_DMA_Handle

	mov	eax, [eax.channel_num]

set_w_ch_num:
	Trace_Out "VDMAD_Set_EISA_Adr_Mode #al, #cl"
	shl	cl, 2
.errnz DMA_EM_Chan_Size xor 1100b	; assumming size is in bits 2 & 3
	and	cl, DMA_EM_Chan_Size
	mov	[eax+DMA_EISA_Ext_Modes], cl

    ; update ext_mode field
	imul eax, SIZE DMA_Channel_Data
	add	eax, OFFSET32 DMA_Channels	; eax is DMA handle
    mov [eax].ext_mode,0            ; assume byte addressing 
    cmp cl,DMA_EM_16bit_wc
    jne @F
    mov [eax].ext_mode,_16_bit_xfer
@@:    
	pop	ecx
	pop	eax
seam_exit:
endif;	!NEC_98
	ret

EndProc VDMAD_Set_EISA_Adr_Mode


;******************************************************************************
; routines to check and deal with DMA terminal counts at hardware int time
;******************************************************************************

;******************************************************************************
;
;   VDMAD_jmp_filter
;
;   DESCRIPTION:    indirect jump to current VDMAD filter or
;		    to the next Hw Int filter registered through VPICD
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_jmp_filter

	jmp	[VDMAD_Hw_Int_Filter]

EndProc VDMAD_jmp_filter

;******************************************************************************
;
;   VDMAD_HW_Check_TC
;
;   DESCRIPTION:    Check for a channel reaching Terminal Count in either
;		    controller.  If one does, then schedule a global event
;		    to complete the transfer.
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_HW_Check_TC

	push	eax
	pushfd
	cli					; don't wanna be reentered

	call	VDMAD_Read_DMA_Status		; ah = DMA2_Status, al = DMA1

	DMA_Q_OUT 'VDMAD_HW_Check_TC 1/2:#AX'

	and	al, [DMA_Ctrl1.CTL_status_mask] ; do we care about any of
	and	ah, [DMA_Ctrl2.CTL_status_mask] ;   the channels at TC?
	or	al, ah
	jz	SHORT chk_tc_exit

	cmp	[DMA_TC_event], 0		; event already scheduled?
	jnz	SHORT chk_tc_exit

	push	esi				; No, schedule one now
	push	edx
	mov	esi, OFFSET32 VDMAD_TC_Event
	xor	edx, edx			; event has no reference data
	VMMCall Schedule_Global_Event
	mov	[DMA_TC_event], esi
	pop	edx
	pop	esi

chk_tc_exit:
	popfd
	pop	eax
	jmp	[VDMAD_Hw_Int_Next]	; pass control to next filter

EndProc VDMAD_HW_Check_TC


;******************************************************************************
;
;   VDMAD_TC_Event
;
;   DESCRIPTION:    Handle completing a DMA transfer.
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_TC_Event

IFDEF DEBUG
	cmp	[DMA_TC_event], 0
	jnz	short vc1_event
	Debug_Out "VDMAD_TC_Event: DMA_TC_event already 0!"
vc1_event:
ENDIF
	pushfd					; avoid race conditions with
	cli					;   interrupt routine
	xor	eax, eax
	mov	[DMA_TC_event], eax
	xchg	al, [VDMAD_DMA1_status]
	xchg	ah, [VDMAD_DMA2_status]
	popfd

	push	eax				; al = ctrl 1 status
	mov	esi, OFFSET32 DMA_Ctrl1
	call	VDMAD_Check_Ctrl_TC

	pop	eax
	mov	al, ah				; al = ctrl 2 status
	mov	esi, OFFSET32 DMA_Ctrl2
	CallRet SHORT VDMAD_Check_Ctrl_TC

EndProc VDMAD_TC_Event


;******************************************************************************
;
;   VDMAD_Check_Ctrl_TC
;
;   DESCRIPTION:    Check for a channel reaching Terminal Count in an
;		    individual controller.
;
;   ENTRY: AL  =  current controller status
;	   EBX =  Current VM Handle
;	   ESI -> DMA_Ctrl1 or DMA_Ctrl2
;
;   EXIT:
;
;   USES:  EAX, ECX
;
;==============================================================================

BeginProc VDMAD_Check_Ctrl_TC

	and	al, [esi.CTL_status_mask]	; TC on any channels under
	jz	SHORT chk_ctl_exit		;   our control?

	movzx	eax, al

chk_ctl_next:
	bsf	ecx, eax			; get 1st/nxt channel #
	jz	SHORT chk_ctl_exit
	btr	eax, ecx			; clear this bit for next time
	push	eax

	call	VDMAD_Get_DMA_Handle_For_Ctrl	; EAX = DMA handle
	DMA_Q_OUT "DMA TC on chn #al", [eax.channel_num]

	bts	dword ptr [esi.CTL_mask], ecx	; 'virtually' mask the channel
						;   cause the HW does at TC
	Assert_Cur_VM_Handle ebx
	cmp	ebx, [eax.owner_VM]		; Q: does the current VM own
	jne	SHORT chk_ctl_call_event	;    the DMA?

	call	VDMAD_DMA_Complete		; Y: complete it here & now
	pop	eax
	jmp	SHORT chk_ctl_next

chk_ctl_call_event:

IFDEF DEBUG
	cmp	[eax.owner_VM], 0
	jne	SHORT @f
	Debug_Out "VDMAD: Owner_VM already 0 on chnl #cl"
	jmp	SHORT chk_ctl_event_done
@@:
ENDIF
	pushad					; N: call pri event to cmplt
	mov	edx, eax			;    the DMA in owner's VM.
	mov	eax, Time_Critical_Boost
	mov	ebx, [edx.owner_VM]
	xor	ecx, ecx
	mov	esi, OFFSET32 VDMAD_Complete_Event
	VMMcall Call_Priority_VM_Event
	popad
chk_ctl_event_done:
	pop	eax
	jmp	SHORT chk_ctl_next

chk_ctl_exit:
	ret

EndProc VDMAD_Check_Ctrl_TC

;******************************************************************************
;******************************************************************************

;******************************************************************************
;
;   VDMAD_Call_Def
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

BeginProc VDMAD_Call_Def

	VxDJmp	VDMAD_Default_Handler

EndProc VDMAD_Call_Def


BeginDoc
;******************************************************************************
;
;   VDMAD_Default_Handler
;
;   DESCRIPTION:
;	Default DMA channel I/O call back routine.  This routine receives
;	notifications of virtual state changes and handles setting up the
;	physical state to start DMA transfers.
;
;	    get virtual state
;	    If channel virtually unmasked then
;		lock region
;		If lock fails then
;		    request buffer
;		    If memory read opeartion then
;			copy data to buffer
;		set phyical state
;		physically unmask channel
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM handle
;
;   EXIT:	    nothing
;
;   USES:	    anything
;
;==============================================================================
EndDoc
BeginProc VDMAD_Default_Handler, SERVICE

	Assert_Cur_VM_Handle ebx	; Debugging
	Validate_DMA_Handle

	mov	ecx, [eax.channel_num]
	DMA_Q_OUT "DMA I/O  handle #eax   VM handle #ebx"

	VxDCall VDMAD_Get_Virt_State	; ESI = linear address
					; ECX = count
					; DL/DH = mode/flags

	test	dl, DMA_requested	;Q: channel requested?
	jnz	short dh_setup_buffer	;   Y:

	test	dl, DMA_masked		;Q: channel masked?
	jnz	dh_masked		;   Y: must have just been masked, so
					;      free the buffer/region

dh_setup_buffer:
	cmp	[eax.locked_pages], 0	;Q: existing lock?
	jne	short dh_chk_region	;   Y: check for same region?
	cmp	[eax.buffer_id], 0	;Q: buffer currently assigned?
	je	short dh_lock		;   N: attempt to lock the new region

dh_chk_region:
	cmp	esi, [eax.region_base]	;   Y: Q: same region?
	jne	short dh_unlock_first	;	    N:
	cmp	ecx, [eax.region_size]	;	    Y: Q: really?
	je	dh_lock_done		;	    Y: no change in region

dh_unlock_first:			; unlock a previously locked region first
	cmp	[eax.buffer_id], 0	;Q: buffer currently assigned?
	jne	short dh_free_buf	;   N:

	DMA_Q_OUT "unlock old region"
	test	[eax.mode], DMA_type_read	; (clears CY--make region dirty)
	jz	SHORT dh_reg_free
	stc					; (don't dirty the region)
dh_reg_free:
	call	VDMAD_Free_Region
	jmp	short dh_lock

dh_free_buf:
	DMA_Q_OUT "release old buffer"
	call	VDMAD_Free_Buffer

dh_lock:				; attempt to lock the new region
	DMA_Q_OUT "lock new region"
;
; It may be desirable to enter critical section before setting physical state.
; This would prevent an app that directly programs DMA from stomping on
; a VM that is using DMA through the OS.  This hasn't proved to be a problem
; so far, so we don't do it now.
;
	mov	edi, [eax.channel_num]
	bt	[DMA_disabled], edi	;Q: translation disabled?
	jc	short dh_attempt_lock	;   Y: we can go with it

	mov	edi, esi
	sub	edi, [ebx.CB_High_Linear]
	cmp	edi, 0A0000h		;Q: adr in adapter region?
	jae	short dh_attempt_lock	;   Y: try to lock first
	push	eax
	lea	edi, [edi+ecx]		; (edi) = ending byte + 1 (low linear)
	VMMCall _GetFirstV86Page
	shl	eax, PAGESHIFT		; (eax) = first vm linear address
	cmp	edi, eax		;Q: adr below 1st VM page
	pop	eax
	jbe	short dh_attempt_lock	;   Y: try to lock first

	test	BYTE PTR [eax.mode], DMA_autoinit
	jz	SHORT dh_not_autoinit

	call	VDMAD_Attempt_Lock	; Q: Can we lock it?
	jnc	short dh_lock_done	;    Y: Good!

	call	VDMAD_Force_Contig	; Q: Can we force memory contig?
	jc	short dh_just_die	;    N: Bad, ugly problem

	call	VDMAD_Attempt_Lock	; This should never fail now!
	jnc	short dh_lock_done	; WEIRD!!!!!!!

dh_just_die:
	mov	ecx,[eax.channel_num]
	Debug_Out "Autoinit: Lock failed on channel #cl"
	VMMCall	Crash_Cur_VM		; will not return

dh_attempt_lock:
	call	VDMAD_Attempt_Lock	; Q: Can we lock it?
	jnc	short dh_lock_done	;    Y: Good!

dh_not_autoinit:
					; EAX = DMA handle, ESI = linear adr
					; EBX = VM Handle
					; ECX = region byte size
	Assert_Cur_VM_Handle ebx	; Debugging
	call	VDMAD_Grab_Buffer

IFDEF allow_partial_virtualization
	test	edx, 80000000h		;Q: mode been set?
	jz	short chk_read_op	;   Y:
					;   N: flag copy out of buffer
	push	eax
	movzx	eax, [eax.buffer_id]
	SetFlag DMA_Buff_Table[eax-1].DB_State, Buffer_Needs_Copying
	pop	eax
	jmp	short dh_copy_into_buf
chk_read_op:
ENDIF

	test	dl, DMA_type_read	;Q: read transfer?
	jz	short dh_write_mode	;   N:

dh_copy_into_buf:
	pushad
	xor	edi, edi
	VxDCall VDMAD_Get_Region_Info
	VxDCall VDMAD_Copy_To_Buffer	; copy data into VDMAD buffer
	popad
	jmp	short dh_lock_done

dh_write_mode:
	; flag that buffer needs to be copied into VM's region after DMA
	push	eax
	movzx	eax, [eax.buffer_id]
	or	DMA_Buff_Table[eax-1].DB_State, Buffer_Needs_Copying
	pop	eax

dh_lock_done:
	VxDCall VDMAD_Set_Phys_State

	test	dl, DMA_masked		;Q: unmask channel?
	jnz	short dh_exit		;   N:
	VxDCall VDMAD_UnMask_Channel	;   Y: do it!
	jmp	short dh_exit

dh_masked:
	DMA_Q_OUT "DMA complete (VM masking) chn #al", [eax.channel_num]
	call	VDMAD_DMA_Complete

dh_exit:
	ret

EndProc VDMAD_Default_Handler


;******************************************************************************
;
;   VDMAD_Force_Contig
;
;   DESCRIPTION:
;	This function attempts to force a region of VM address space to
;	be contiguous.
;
;   ENTRY:
;	EBX = VM mandle
;	ESI = Linear address
;	ECX = Size
;
;   EXIT:
;	Carry clear if worked.
;
;   USES:
;	Flags
;
;==============================================================================

BeginProc VDMAD_Force_Contig, ESP, RARE

	LocalVar    npages, DWORD
	LocalVar    pageno, DWORD
	EnterProc
	SaveReg <ad>

	Assert_VM_Handle ebx
	sub	esi, [ebx.CB_High_Linear]
	dec	ecx
	add	ecx, esi		; ecx = last byte of region
	shr	esi, PAGESHIFT		; start page # (linear)
	mov	[pageno], esi
	shr	ecx, PAGESHIFT		; last page #
	sub	ecx, esi
	inc	ecx			; # of pages in region
	mov	[npages], ecx


;	Allocate a new bunch of pages that are fixed and have the proper
;	alignment and contiguity requirements.


	mov	edx, ecx		    ; convert DB_Size to # of pages
	mov	eax, edx		    ; (examples:       3    7	  11
					    ;		      12K  28K	  44K
	dec	eax			    ; # pages - 1     10b  111b  1011b
	bsr	cx, ax			    ; max power of 2   1    2	   3
	inc	cl			    ; shift cnt        2    3	   4
	mov	eax, 1
	shl	eax, cl 		    ; mask + 1	     100b 1000b 10000b
	dec	eax			    ; mask	      11b  111b  1111b
					    ; alignment       16K  32K	  64K

	VMMCall _PageAllocate <edx,PG_SYS,0,eax,0,[DMA_Max_Physical],0,PageUseAlign+PageContig+PageFixed>
	test	eax, eax
	Debug_OutZ "VDMAD ERROR: Can't alloc memory for auto-init DMA"
	jz	VDMAD_FC_Failed

;	Copy the data from the old pages to the new ones.  Note that the
;	pages might be swappable so this could yield.  This code is broken
;	if some other thread modifies the pages being replaced after we
;	copy, but before we replace the pages.

	mov	edi,eax 			; (edi) = base of new pages
	mov	esi,pageno 			; (esi) = base page # of old pages
	shl	esi,PAGESHIFT			; (esi) = base of old pages
	mov	ecx,npages			; (ecx) = pages to copy
	shl	ecx,PAGESHIFT - 2		; (ecx) = dwords to copy
	rep	movsd				; copy old pages to new

	mov	esi,eax 			; (esi) = saved new base address

;	Map the new pages over the old ones

	shr	eax,PAGESHIFT			; (eax) = page allocated
	VMMCall _LinMapIntoV86 <eax, ebx, pageno, npages, 0> ; map the new memory in

;	Free the high mapping of the new pages.  The pages themselves won't be
;	freed by this because they are still mapped into the VM.

	SaveReg <eax>
	VMMCall _PageFree <esi, 0>
	RestoreReg <eax>

	sub	eax, 1				; Carry set if EAX = 0
VDMAD_FC_Exit:
	RestoreReg <ad>
	LeaveProc
	Return

VDMAD_FC_Failed:
	stc
	jmp	VDMAD_FC_Exit

EndProc VDMAD_Force_Contig


;******************************************************************************
;
;   VDMAD_Attempt_Lock
;
;   DESCRIPTION:    attempt to lock a DMA region to a DMA handle
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;		    ESI = linear address of actual DMA region
;		    ECX = # of bytes in DMA region
;
;   EXIT:	    Carry clear if memory locked
;
;   USES:	    Flags
;
;==============================================================================
BeginProc VDMAD_Attempt_Lock

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle
	pushad

	mov	edx, [eax.channel_num]
	bt	[DMA_disabled], edx	;Q: translation disabled?
	mov	edx, esi		;      assume true - copy phys adr
	jc	short no_lock		;   Y: don't lock - esi = phys

ifndef	NEC_98
	xor	edx, edx
        test    [VDMAD_Machine_Type], EISA_Machine OR MCA_Machine
        jnz     no_alignment            ; alignment not req'd. for EISA & MCA

	mov	edx, [eax.channel_num]
	cmp	dl, 4
	mov	dl, 1
	jb	short dh_byte_channel
	shl	edx, 1

dh_byte_channel:
no_alignment:
endif;	!NEC_98
	VxDCall VDMAD_Lock_DMA_Region	; EDX = physical address
	jc	SHORT attempt_failed
	push	ecx
	add	ecx,edx			; max physical address + 1
	shr	ecx,12			; convert to pages
	cmp	ecx,[DMA_Max_Physical]
	pop	ecx
	jb	SHORT no_lock
	VxDCall	VDMAD_Unlock_DMA_Region
	stc
	jmp	short attempt_failed
no_lock:
	mov	ebx, 0FF00h		; pages locked & no buffer
	VxdCall VDMAD_Set_Region_Info
	clc

attempt_failed:
	popad
	ret

EndProc VDMAD_Attempt_Lock

;******************************************************************************
;
;   VDMAD_Free_Region
;
;   DESCRIPTION:    unlock the DMA region locked to a DMA handle
;
;   ENTRY:	    EAX = DMA handle
;		    Carry clear, if region pages should be marked as dirty
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================
BeginProc VDMAD_Free_Region

	Validate_DMA_Handle
	pushad
	pushfd

	mov	edx, [eax.channel_num]
	bt	[DMA_disabled], edx	;Q: translation disabled?
	jc	short fr_reset_region	;   Y: don't unlock - just reset region

	VxDCall VDMAD_Get_Region_Info
	popfd
	pushfd
	jc	short fr_no_dirty	; jump, if pages shouldn't be dirtied
	VxDCall VDMAD_Unlock_DMA_Region

fr_reset_region:
	popfd
	xor	ebx, ebx
	xor	esi, esi
	xor	ecx, ecx
	xor	edx, edx
	VxDCall VDMAD_Set_Region_Info

	popad
	ret

fr_no_dirty:
	VxDCall VDMAD_Unlock_DMA_Region_No_Dirty
	jmp	fr_reset_region

EndProc VDMAD_Free_Region


;******************************************************************************
;
;   VDMAD_Grab_Buffer
;
;   DESCRIPTION:    Allocate the DMA buffer and assign it to a DMA handle
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;		    ESI = linear adr
;		    ECX = region byte size
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================
BeginProc VDMAD_Grab_Buffer

	Assert_Cur_VM_Handle ebx		; Debugging
	Validate_DMA_Handle
	pushad

try_grab:
	VxDCall VDMAD_Request_Buffer
	jnc	buffer_grabbed

	VMMcall Get_Cur_VM_Handle

	cmp	al, DMA_Buffer_Too_Small
IFDEF DEBUG
	jne	SHORT JustBusy
	pushfd
	pushad
	mov	eax, [esp.Pushad_EAX]
	mov	edx, [eax.channel_num]
	Debug_Out "VDMAD: buffer too small, requested $#ecx bytes for channel #dl, handle #eax"
	popad
	popfd
JustBusy:
ENDIF
	je	VDMAD_In_Your_Face

;	If the buffer is currently in use, sleep for 100ms and try again.
;	Hopefully an interrupt will come in which will free up the buffer.

	Trace_Out "VDMAD:  buffer in use"
	push	eax
	mov	eax,100
	VMMCall Time_Slice_Sleep
	pop	eax
        jmp     try_grab                        ; try again

buffer_grabbed:
	mov	eax, [esp.Pushad_EAX]
	VxdCall VDMAD_Set_Region_Info
	popad

	DMA_Q_OUT "DMA buffer assigned to #eax in VM #ebx"
	ret

EndProc VDMAD_Grab_Buffer


;******************************************************************************
;
;   VDMAD_In_Your_Face
;
;   DESCRIPTION:
;
;   ENTRY:
;	EBX = Current VM handle
;	ECX = Size of request
;
;   EXIT:
;
;   USES:
;
;==============================================================================

VxD_RARE_CODE_SEG

BeginProc VDMAD_In_Your_Face

	Assert_Cur_VM_Handle ebx

        add     ecx, 1023
	shr	ecx, 10 			; Convert bytes to Kb

	VMMCall Test_Sys_VM_Handle		; Q: System VM?
	je	SHORT VDMAD_IYF_Fatal		;    Y: Really, really ugly!

	VxDcall _CONFIGMG_Set_Private_Problem <[VDMAD_Devnode_Handle], ecx, 0>

	mov	eax,GSDVME_NukeNoMsg
	xor	edx, edx
	xor	ecx,ecx
	inc	ecx
	VMMCall GetSetDetailedVMError		; Set Error

	VMMcall End_Critical_Section		; Get out of crit section

	VMMcall Crash_Cur_VM
      ;;  xor	  ecx, ecx		  ; Can't continue exec in this VM
      ;;  mov	  eax, 5000		  ; Wait for up to 5 seconds to close
      ;;  VMMcall Close_VM		  ; Won't ever return!


VDMAD_IYF_Fatal:
        PUSH_SPRINTF <MSG_Buffer_Too_Small>, ecx; Put the message on the stack
						; EAX -> message on stack
	xchg	esi, eax
        Fatal_Error <esi>

EndProc VDMAD_In_Your_Face

VxD_RARE_CODE_ENDS


;******************************************************************************
;
;   VDMAD_Free_Buffer
;
;   DESCRIPTION:    Release the DMA buffer from a DMA handle
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================
BeginProc VDMAD_Free_Buffer

	Validate_DMA_Handle
	pushad

	movzx	ebx, [eax.buffer_id]
	VxDCall VDMAD_Release_Buffer

	xor	ebx, ebx
	xor	esi, esi
	xor	ecx, ecx
	xor	edx, edx
	VxDcall VDMAD_Set_Region_Info

	popad
	ret

EndProc VDMAD_Free_Buffer


;******************************************************************************
;
;   VDMAD_DMA_Complete
;
;   DESCRIPTION:    Complete DMA transfer.
;			If buffered then
;			    If writing to memory then
;				copy memory from buffer to DMA region
;			    free buffer
;			else
;			    unlock DMA region
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================

	PUBLIC	VDMAD_Complete_Event
VDMAD_Complete_Event	LABEL	NEAR	; Priority Event entry point

	mov	eax, edx		; reference data is DMA handle

BeginProc VDMAD_DMA_Complete

	Validate_DMA_Handle
	cmp	[eax.owner_VM],0
	je	short dh_complete
;
; Set default update value for count register
;         
        push    edx
        mov     edx,0FFFFFFh            ; 24-bit count for EISA
ifndef	NEC_98
        IF_EISA_JMP <short vdc_eisa_1>
endif
        shr     edx,8                   ; convert to 16-bit count for ISA

vdc_eisa_1:
	cmp	[eax.buffer_id],0	;Q: buffer assigned?
	je	short dh_unlock 	;   N:
	pushad
	movzx	ecx, [eax.buffer_id]
	TestMem DMA_Buff_Table[ecx-1].DB_State, Buffer_Needs_Copying
	jz	short dh_free_buffer	;   N:

	mov	ecx,[eax.channel_num]	; ecx = channel number
	mov	esi,OFFSET32 DMA_Ctrl1	; point esi to global Ctrl 1 state info
	btr	ecx,2			; is channel for 2nd controller?
	jnc	short first_ctrl
	add	esi,SIZE DMA_Controller_State
first_ctrl:
	mov	edi,eax	
	mov	eax,edx
	call	VDMAD_Partial_Complete

dh_free_buffer:
	popad
	mov	[eax.cur_count],edx
	call	VDMAD_Free_Buffer
	jmp	short dh_mask

dh_unlock:
	mov	[eax.cur_count],edx
	cmp	[eax.locked_pages], 0	;Q: pages locked?
	je	short dh_mask		;   N:

	test	[eax.mode], DMA_type_read	; (clears CY--make region dirty)
	jz	SHORT dh_free_reg
	stc					; (don't dirty the region)
dh_free_reg:
	call	VDMAD_Free_Region	;   Y: unlock them
;
; physically mask the channel again
;
dh_mask:
	VxDCall VDMAD_Mask_Channel	; EAX = DMA handle
        pop     edx

dh_complete:
	ret

EndProc VDMAD_DMA_Complete


;==============================================================================


IFDEF DEBUG
;******************************************************************************
;
;   VDMAD_Validate_Handle
;
;   DESCRIPTION:    check for a valid DMA handle
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    EAX = DMA handle
;
;   USES:	    nothing
;
;==============================================================================
BeginProc VDMAD_Validate_Handle

	pushfd
	push	eax
	push	edx
	sub	eax, OFFSET32 DMA_Channels
	jb	short bad_handle
	cmp	eax, (SIZE DMA_Channel_Data) * 7
	ja	short bad_handle
	mov	edx, SIZE DMA_Channel_Data
	idiv	dl
	or	ah, ah
	jz	short handle_ok
bad_handle:
	Debug_Out "VDMAD: invalid DMA handle #EAX"
	Fatal_Error

handle_ok:
	pop	edx
	pop	eax
	popfd
	ret

EndProc VDMAD_Validate_Handle


;******************************************************************************
;
;   VDMAD_Validate_Buff_ID
;
;   DESCRIPTION:
;
;   ENTRY:
;	BL = Buffer ID
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Validate_Buff_ID

	pushfd
	pushad
	movzx	eax, bl
	dec	eax
	mov	ecx, SIZE DMA_Buff_Struc
	xor	edx, edx
	idiv	ecx
	test	edx, edx
	jnz	VVBI_Invalid
	cmp	eax, NUM_DMA_BUFFS
	jb	VVBI_OK
VVBI_Invalid:
	Debug_Out "VDMAD ERROR:  Invalid buffer ID #bl"

VVBI_OK:
	popad
	popfd
	ret

EndProc VDMAD_Validate_Buff_ID

ENDIF

VxD_LOCKED_CODE_ENDS


;******************************************************************************
;	   D E B U G G I N G   C O D E	 F O R	 A L L	 V E R S
;******************************************************************************

VxD_DEBUG_ONLY_CODE_SEG


;******************************************************************************
;
;   VDMAD_Query
;
;   DESCRIPTION:    Display current VDMAD status information to the debugging
;		    terminal
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Query

	Trace_Out "VDMAD state"
	Trace_Out " "
	Trace_Out " Handle      Size     Physical    Linear      Time     Owner VM   State"
	mov	edi, OFFSET32 DMA_Buff_Table
VQ_Buff_Loop:
	Trace_Out "#edi   ", NO_EOL
	mov	eax, [edi.DB_Size_Pages]
	test	eax, eax
	jnz	VQ_ShowState
	Trace_Out "(Unallocated)"
	jmp	VQ_Next_Buffer
VQ_ShowState:
	mov	ebx, [edi.DB_Phys]
	mov	ecx, [edi.DB_Linr]
	mov	edx, [edi.DB_Last_Time]
	Trace_Out "#eax   #ebx   #ecx   #edx   ", NO_EOL
	mov	eax, [edi.DB_Owner]
	mov	ebx, [edi.DB_State]
	Trace_Out "#eax   #ebx"
VQ_Next_Buffer:
	add	edi, SIZE DMA_Buff_Struc
	cmp	edi, OFFSET32 End_DMA_Buff_Table
	jb	VQ_Buff_Loop

	Trace_Out " "
	mov	eax, [VDMAD_CB_Offset]
	Trace_Out "CB offset          #eax"
	mov	eax, [VDMAD_Hw_Int_Filter]
	mov	ebx, [VDMAD_Hw_Int_Next]
	cmp	eax, ebx		;Q: assigned filter proc?
	je	short no_hw_int_hook	;   N:
	Trace_Out "Hw Int filter      ?eax"
	Trace_Out "Next Hw Int filter ?ebx"
	mov	al, [VDMAD_DMA1_status]
	Trace_Out "First DMA status   #al"
	mov	al, [VDMAD_DMA2_status]
	Trace_Out "Second DMA status  #al"
	jmp	short vq_1

no_hw_int_hook:
	Trace_Out "VPICD HW INT notification not hooked"
vq_1:

	Trace_Out " "
	Trace_Out "ESC to quit, or any char to see controller states:"
	VMMcall In_Debug_Chr
	jz	query_exit

	Trace_Out "Controller          1                 2"
	mov	al, [DMA_Ctrl1.CTL_status]
	mov	bl, [DMA_Ctrl2.CTL_status]
	Trace_Out "status             #al                #bl"
	mov	al, [DMA_Ctrl1.CTL_mask]
	mov	bl, [DMA_Ctrl2.CTL_mask]
	Trace_Out "mask               #al                #bl"
	mov	al, [DMA_Ctrl1.CTL_request]
	mov	bl, [DMA_Ctrl2.CTL_request]
	Trace_Out "request            #al                #bl"

	Trace_Out '                adr     cnt   mod em      adr     cnt   mod em'

	mov	ecx, 4
	xor	eax, eax
show_chn_state:
	push	ecx
	Trace_Out "channel #al   ",/noeol

	push	eax
	call	VDMAD_Get_DMA_Handle

	mov	ebx, [eax.cur_addr]
	mov	edi, [eax.cur_count]
	mov	dl, [eax.mode]
	mov	cl, [eax.ext_mode]
	Trace_Out "#ebx #edi #dl #cl",/noeol

	pop	eax

IFDEF allow_partial_virtualization
	bt	[DMA_Ctrl1.CTL_mode_set], ax
	jnc	short q_mode1_set
	Trace_Out "-",/noeol
	jmp	short show_ctrl2
q_mode1_set:
	Trace_Out "V",/noeol
show_ctrl2:
ENDIF

	push	eax
	add	al, 4
	call	VDMAD_Get_DMA_Handle

	mov	ebx, [eax.cur_addr]
	mov	edi, [eax.cur_count]
	mov	dl, [eax.mode]
	mov	cl, [eax.ext_mode]

	pop	eax

IFDEF allow_partial_virtualization
	Trace_Out "  #ebx #edi #dl #cl",/noeol
	bt	[DMA_Ctrl2.CTL_mode_set], ax
	jnc	short q_mode2_set
	Trace_Out "-"
	jmp	short next_chn
q_mode2_set:
	Trace_Out "V"
next_chn:
ELSE
	Trace_Out "  #ebx #edi #dl #cl"
ENDIF

	inc	eax
	pop	ecx
	dec	ecx
	jecxz	short @f
	jmp	show_chn_state

@@:
	Trace_Out " "
	Trace_Out "ESC to quit, or any char to see channel states:"
	VMMcall In_Debug_Chr
	jz	query_exit

	mov	esi, OFFSET32 DMA_Channels
	mov	ecx, 8
sc_loop:
	call	VDMAD_ShowChannel_State
	add	esi, SIZE DMA_Channel_Data
	loop	sc_loop


	Trace_Out " "
	Trace_Out "ESC to quit, or any char to see CurVM states:"
	VMMcall In_Debug_Chr
	jz	query_exit

	VMMcall Get_Cur_VM_Handle
	Trace_Out "Virtual DMA State for CurVM #EBX"
	Trace_Out " "
	mov	esi, ebx
	add	esi, [VDMAD_CB_OFFSET]

	mov	al, [esi.DMA_flipflop]
	mov	bl, [esi.DMA_flipflop+1]
	Trace_Out "flipflop:  ctrl 1 #AL   ctrl 2 #BL"

ifndef	NEC_98
	IF_NOT_MCA_JMP	<short skip_PS2_status>
	mov	al, [esi.DMA_PS2_cmd]
	Trace_Out 'PS2:DMA_PS2_cmd    #al'
	mov	al, [esi.DMA_writing]
	Trace_Out 'PS2:DMA_writing    #al'
	mov	al, [esi.DMA_bytePtr]
	Trace_Out 'PS2:DMA_bytePtr    #al'
	mov	al, [esi.DMA_dataBytes]
	Trace_Out 'PS2:DMA_dataBytes  #al'
	mov	eax, [esi.DMA_data]
	Trace_Out 'PS2:DMA_data       #eax'
skip_PS2_status:
endif

query_exit:
	ret
EndProc VDMAD_Query

;******************************************************************************
;
;   VDMAD_ShowChannel_State
;
;   DESCRIPTION:
;
;   ENTRY:	    ESI points to DMA_Channel_Data struc
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_ShowChannel_State

IFDEF DEBUG
	mov	eax, [esi.largest_request]
	add	eax, 1023
	shr	eax, 10 		; # of Kb
	VMMCall Debug_Convert_Hex_Decimal
	mov	ebx, eax
ELSE
	xor	ebx, ebx
ENDIF

	mov	eax, [esi.channel_num]
	Trace_Out "DMA channel ##al  max=#bxKb  callback = ",/noeol
	mov	eax, [esi.call_back]
	or	eax, eax		;Q: call back assigned?
	jz	short no_call_back	;   N:
	Trace_Out "?EAX"
	jmp	short show_channel_state

no_call_back:
	Trace_Out "none"
show_channel_state:

	movzx	eax, [esi.locked_pages]
	or	eax, eax		;Q: locked region?
	jz	short no_region 	;   N:
	Trace_Out "    region locked"
	jmp	short show_region
no_region:
	movzx	eax, [esi.buffer_id]
	or	eax, eax		;Q: buffer assigned?
	jz	short no_buffer 	;   N:
	Trace_Out "    buffer id    #ax"

show_region:
	mov	eax, [esi.region_base]
	mov	ebx, [esi.region_size]
	Trace_Out "    region adr   #eax   region size  #ebx"
	mov	ax, [esi.xfer_page]
	mov	bx, [esi.xfer_base]
	Trace_Out "    DMA page     #ax       DMA base     #bx"
	mov	eax, [esi.owner_VM]
	Trace_Out "    owner VM     #eax"

no_buffer:
	ret

EndProc VDMAD_ShowChannel_State


VxD_DEBUG_ONLY_CODE_ENDS


END
