;****************************************************************************
;                                                                           *
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
; PURPOSE.                                                                  *
;                                                                           *
; Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
;                                                                           *
;****************************************************************************

;******************************************************************************
;
;   Title:    DMASYS.INC - includes for Virtual DMA driver
;
;   Version:  2.00
;
;==============================================================================

;
; DMA related ports
;
DMA_P0		equ	0087h		; DMA page register for Channel 0
DMA_P1		equ	0083h		; DMA page register for Channel 1
DMA_P2		equ	0081h		; DMA page register for Channel 2
DMA_P3		equ	0082h		; DMA page register for Channel 3
DMA_P5		equ	008Bh		; DMA page register for Channel 5
DMA_P6		equ	0089h		; DMA page register for Channel 6
DMA_P7		equ	008Ah		; DMA page register for Channel 7

DMA_Page_Min	equ	DMA_P2
DMA_P4		equ	008Fh
DMA_Page_Max	equ	DMA_P4

DMA_B0		equ	0000h		; DMA base register for Channel 0
DMA_C0		equ	0001h		; DMA count register for Channel 0
DMA_B1		equ	0002h		; DMA base register for Channel 1
DMA_C1		equ	0003h		; DMA count register for Channel 1
DMA_B2		equ	0004h		; DMA base register for Channel 2
DMA_C2		equ	0005h		; DMA count register for Channel 2
DMA_B3		equ	0006h		; DMA base register for Channel 3
DMA_C3		equ	0007h		; DMA count register for Channel 3
DMA_B4		equ	00C0h		; DMA base register for Channel 4
DMA_C4		equ	00C2h		; DMA count register for Channel 4
DMA_B5		equ	00C4h		; DMA base register for Channel 5
DMA_C5		equ	00C6h		; DMA count register for Channel 5
DMA_B6		equ	00C8h		; DMA base register for Channel 6
DMA_C6		equ	00CAh		; DMA count register for Channel 6
DMA_B7		equ	00CCh		; DMA base register for Channel 7
DMA_C7		equ	00CEh		; DMA count register for Channel 7

DMA_Ctrl_1	equ	DMA_B0
DMA_Ctrl_1_Max	equ	000Fh

DMA_Ctrl_2	equ	DMA_B4
DMA_Ctrl_2_Max	equ	00DFh

DMA1_Status	equ	0008h
DMA1_SoftReq	equ	0009h
DMA1_SingleMask equ	000Ah
DMA1_Mode	equ	000Bh
DMA1_CLR_FF	equ	000Ch		; clear flip-flop cmd for channels 0-3
DMA1_Reset	equ	000Dh
DMA1_ResetMask	equ	000Eh
DMA1_Mask	equ	000Fh

DMA2_Status	equ	00D0h
DMA2_SoftReq	equ	00D2h
DMA2_SingleMask equ	00D4h
DMA2_Mode	equ	00D6h
DMA2_CLR_FF	equ	00D8h		; clear flip-flop cmd for channels 5-7
DMA2_Reset	equ	00DAh
DMA2_ResetMask	equ	00DCh
DMA2_Mask	equ	00DEh

;
;   PS/2 specific ports
;
DMA_XFN 	equ	0018h
DMA_EXE 	equ	001Ah

;
;   PS/2 extended commands
;

Set_IO_Adr	equ	00h
Set_Mem_Adr	equ	20h
Get_Mem_Adr	equ	30h
Set_Count	equ	40h
Get_Count	equ	50h
Get_Status	equ	60h
Set_Mode	equ	70h
Set_Arbus	equ	80h
Set_Chn_Mask	equ	90h
Reset_Chn_Mask	equ    0A0h
Master_Clear	equ    0D0h


;------------------------------------------------------------------------------
; I/O ports for EISA
;
;==============================================================================
;==
;== Channel Interrupt Status Register: (READ) (40Ah)
;==
;== 76543210
;== xxxxxxxx---> Interrupt on channel 0
;== |||||||----> Interrupt on channel 1
;== ||||||-----> Interrupt on channel 2
;== |||||------> Interrupt on channel 3
;== ||||-------> reserved
;== |||--------> Interrupt on channel 5
;== ||---------> Interrupt on channel 6
;== |----------> Interrupt on channel 7
;==
;== Chaining Mode Status Register: (READ) (4D4h)
;==
;== 76543210
;== xxxxxxxx---> Channel 0 enabled
;== |||||||----> Channel 1 enabled
;== ||||||-----> Channel 2 enabled
;== |||||------> Channel 3 enabled
;== ||||-------> reserved
;== |||--------> Channel 5 enabled
;== ||---------> Channel 6 enabled
;== |----------> Channel 7 enabled
;==
;== Chaining Mode Register: (WRITE) (40Ah & 4D4h)
;==
;== 76543210
;== xxxxxxxx
;== ||||||||---> Channel 0-3 selected
;== ||||||-----> 0-Disable, 1-Enable, 2-illegal, 3-Programming complete
;== ||||
;== ||||-------> reserved
;==
;==============================================================================
DMA_E_IS	equ	40Ah			; Channel interrupt status register (read only)
DMA_E_CS	equ	4D4h			; Chaining mode status register (read only)

DMA_E_CH1	equ	40Ah		; ChainMode for channels 0-3 (write only)
DMA_E_CH2	equ	4D4h		; ChainMode for channels 5-7 (write only)

;==============================================================================
;==
;== DMA Extended Mode Register: (WRITE) (40Bh & 4D6h)
;==
;== 76543210
;== xxxxxxxx
;== ||||||||---> Channel 0-3 selected
;== ||||||-----> 0-8/byte, 1-16/word, 2-32/byte, 3-16/byte
;== ||||-------> 0-ISA, 1-"A", 2-"B", 3-"C" (Burst)
;== ||---------> T-C is an input for this channel
;==	|----------> Stop register disabled
;==
;==============================================================================
DMA_E_EM1	equ	40Bh		; Extended Mode for channels 0-3
DMA_E_EM2	equ	4D6h		; Extended Mode for channels 5-7

DMA_EM_Chan_Mask    equ   11b
DMA_EM_Chan_Size    equ 1100b
DMA_EM_8bit	    equ 0000b		; byte transfer with byte adr & cnt
DMA_EM_16bit_wc     equ 0100b		; word transfer with word adr & cnt (ISA)
DMA_EM_32bit	    equ 1000b		; dword transfer with byte adr & cnt
DMA_EM_16bit	    equ 1100b		; word transfer with byte adr & cnt

DMA_E_Hoff	equ	400h

DMA_E_P0	equ	487h		; HiPage0 (Bits: <31.24>)
DMA_E_P1	equ	483h		; HiPage1
DMA_E_P2	equ	481h		; HiPage2
DMA_E_P3	equ	482h		; HiPage3
DMA_E_P5	equ	48Bh		; HiPage5
DMA_E_P6	equ	489h		; HiPage6
DMA_E_P7	equ	48Ah		; HiPage7

.errnz DMA_E_P0 - DMA_P0 - DMA_E_Hoff
.errnz DMA_E_P1 - DMA_P1 - DMA_E_Hoff
.errnz DMA_E_P2 - DMA_P2 - DMA_E_Hoff
.errnz DMA_E_P3 - DMA_P3 - DMA_E_Hoff
.errnz DMA_E_P5 - DMA_P5 - DMA_E_Hoff
.errnz DMA_E_P6 - DMA_P6 - DMA_E_Hoff
.errnz DMA_E_P7 - DMA_P7 - DMA_E_Hoff

DMA_E_C0	equ	401h		; HiCount0 (Bits: <23.16>)
DMA_E_C1	equ	403h		; HiCount1
DMA_E_C2	equ	405h		; HiCount2
DMA_E_C3	equ	407h		; HiCount3
DMA_E_C5	equ	4C6h		; HiCount5
DMA_E_C6	equ	4CAh		; HiCount6
DMA_E_C7	equ	4CEh		; HiCount7

.errnz DMA_E_C0 - DMA_C0 - DMA_E_Hoff
.errnz DMA_E_C1 - DMA_C1 - DMA_E_Hoff
.errnz DMA_E_C2 - DMA_C2 - DMA_E_Hoff
.errnz DMA_E_C3 - DMA_C3 - DMA_E_Hoff
.errnz DMA_E_C5 - DMA_C5 - DMA_E_Hoff
.errnz DMA_E_C6 - DMA_C6 - DMA_E_Hoff
.errnz DMA_E_C7 - DMA_C7 - DMA_E_Hoff

DMA_E_S00	equ	4E0h		; Stop channel 0 (Bits: <7:2> )
DMA_E_S01	equ	4E1h		; Stop channel 0 (Bits: <15.8> )
DMA_E_S02	equ	4E2h		; Stop channel 0 (Bits: <23.16> )
DMA_E_S10	equ	4E4h		; Stop channel 1 (Bits: <7:2> )
DMA_E_S11	equ	4E5h		; Stop channel 1 (Bits: <15.8> )
DMA_E_S12	equ	4E6h		; Stop channel 1 (Bits: <23.16> )
DMA_E_S20	equ	4E8h		; Stop channel 2 (Bits: <7:2> )
DMA_E_S21	equ	4E9h		; Stop channel 2 (Bits: <15.8> )
DMA_E_S22	equ	4EAh		; Stop channel 2 (Bits: <23.16> )
DMA_E_S30	equ	4ECh		; Stop channel 3 (Bits: <7:2> )
DMA_E_S31	equ	4EDh		; Stop channel 3 (Bits: <15.8> )
DMA_E_S32	equ	4EEh		; Stop channel 3 (Bits: <23.16> )
DMA_E_S50	equ	4F4h		; Stop channel 5 (Bits: <7:2> )
DMA_E_S51	equ	4F5h		; Stop channel 5 (Bits: <15.8> )
DMA_E_S52	equ	4F6h		; Stop channel 5 (Bits: <23.16> )
DMA_E_S60	equ	4F8h		; Stop channel 6 (Bits: <7:2> )
DMA_E_S61	equ	4F9h		; Stop channel 6 (Bits: <15.8> )
DMA_E_S62	equ	4FAh		; Stop channel 6 (Bits: <23.16> )
DMA_E_S70	equ	4FCh		; Stop channel 7 (Bits: <7:2> )
DMA_E_S71	equ	4FDh		; Stop channel 7 (Bits: <15.8> )
DMA_E_S72	equ	4FEh		; Stop channel 7 (Bits: <23.16> )

;------------------------------------------------------------------------------
; record structure of controller state information (2 records kept globally)

DMA_Controller_State STRUC
CTL_flags	    dd	?
CTL_status	    db	?
CTL_mask	    db	?
CTL_request	    db	?
CTL_status_mask     db	?

IFDEF allow_partial_virtualization
CTL_mode_set	    dw	?
		    dw	?	    ; padding
ENDIF

DMA_Controller_State ENDS

.errnz SIZE DMA_Controller_State MOD 4

;
; Flag equates for DMA_flags
;

DMA_Ch0_Word	    equ 00000001b	; set if 1st chan on ctrl in word mode
DMA_Ch0_Word_bit    equ        0
DMA_Ch1_Word	    equ 00000010b	; set if 2nd chan on ctrl in word mode
DMA_Ch1_Word_bit    equ       1
DMA_Ch2_Word	    equ 00000100b	; set if 3rd chan on ctrl in word mode
DMA_Ch2_Word_bit    equ      2
DMA_Ch3_Word	    equ 00001000b	; set if 4th chan on ctrl in word mode
DMA_Ch3_Word_bit    equ     3

DMA_2nd_Ctlr	    equ 10000000b
DMA_2nd_Ctlr_bit    equ 7

DMA_Word_Ctlr	    equ DMA_Ch0_Word OR DMA_Ch1_Word OR DMA_Ch2_Word OR DMA_Ch3_Word

;------------------------------------------------------------------------------
; record structure of constant information for a DMA controller (kept globally)

Controller_Const    STRUC
DMA_base_handle     dd	?		; DMA handle for channel 0
DMA_mode_port	    db	?		; port address for setting mode
DMA_single_mask_p   db	?		; port address for setting a channel mask
DMA_request_port    db	?		; port address for requesting DMA
DMA_flipflop_port   db	?		; port address for resetting flipflop
;DMA_TC_checker      dd  ?		 ; routine for checking for TC
;DMA_HW_TC_checker   dd  ?		 ; HW int routine for checking for TC
		    dd	?
		    dd	?
Controller_Const    ENDS


;------------------------------------------------------------------------------
; structure of DMA information kept in each VM's control block

DMA_CB_DATA	STRUC
DMA_data	dd  0		    ; if reading, then bytes that can be read
DMA_PS2_cmd	db  0		    ; last commmand byte
DMA_writing	db  0		    ; 0 - reading, 1 - writing port 1Ah
DMA_bytePtr	db  0		    ; byte position in read or write
DMA_dataBytes	db  0		    ; # of bytes readable or writable
DMA_flipflop	db  2 DUP (0)	    ; virtualized ctrl flipflop state for VM
DMA_CB_DATA ENDS


;------------------------------------------------------------------------------
; structure of information kept for each physical DMA channel

DMA_Channel_Data    STRUC
channel_num	    dd	0
call_back	    dd	0		; I/O call back
owner_VM	    dd	0		; VM handle
region_base	    dd	0		; linear address
region_size	    dd	0		; size in bytes
locked_pages	    db	0		; 0 = FALSE, else TRUE	(not actual #)
buffer_id	    db	0		; 0, if not assigned
xfer_page	    dw	0		; DMA transfer page
xfer_base	    dw	0		; DMA transfer base
callbacks	    dw	0		; callback flags
disable_cnt	    dd	0		; > 0 if auto translation disabled
cur_addr	    dd	0		; current virtual transfer address
pgm_addr	    dd	0		; app programmed virtual transfer addr
cur_count	    dd	0		; current transfer count
pgm_count	    dd	0		; app programmed transfer count
mode		    db	0
ext_mode	    db	0		; extended mode
IO_port 	    dw	0
IFDEF DEBUG
largest_request     dd	0
ENDIF
DMA_Channel_Data    ENDS

.errnz SIZE DMA_Channel_Data MOD 4


DMA_Buff_Struc STRUC
DB_Size_Pages	    dd	0
DB_Phys 	    dd	0
DB_Linr 	    dd	0
DB_State	    dd	0
DB_Owner	    dd	0
DB_Last_Time	    dd	0
DMA_Buff_Struc ENDS


Buffer_In_Use		equ 1
Buffer_Needs_Copying	equ 2
Buffer_Owns_CritSection equ 4

Convert_ID_To_Handle MACRO handlereg, errorlabel
	Validate_Buffer_ID
	movzx	handlereg, bl
	dec	handlereg
	.ERRNZ SIZE DMA_Buff_Struc - 11000b
	test	handlereg, 111b
	jnz	errorlabel
	add	handlereg, OFFSET32 DMA_Buff_Table
	ENDM

;
; Macro to convert channel # into byte offset into channel constants array
;
Cntrl_Const_Offset MACRO reg
	and	reg, 100b
.ERRNZ (SIZE Controller_Const) - 16	; assume 16 bytes
	shl	reg, 2			; offset into controller constants array
	ENDM

Validate_DMA_Handle MACRO
IFDEF DEBUG
ifndef MASM6
IF2
IFNDEF VDMAD_Validate_Handle
EXTRN VDMAD_Validate_Handle:NEAR
ENDIF
endif   ; MASM6
ENDIF

	call	VDMAD_Validate_Handle
ENDIF
	ENDM


Validate_Buffer_ID MACRO
IFDEF DEBUG
	call	VDMAD_Validate_Buff_ID
ENDIF
	ENDM




DMA_Set_Request equ 4


DMA_Services_Avail	equ 00100000b
DMA_Services_Avail_Bit	equ 5
DMA_Chain_4B		equ 00001000b
DMA_Chain_4B_Bit	equ 3


; machine types:

ISA_Machine	    equ 0
MCA_Machine	    equ 1
EISA_Machine	    equ 2
PCXT_Machine        equ 4               ; PC/XT with 386 expansion board

IF_ISA_JMP  MACRO label
ifndef MASM6
IF 2
IFNDEF VDMAD_Machine_Type
	EXTRN VDMAD_Machine_Type:BYTE
ENDIF
ENDIF
endif   ; MASM6
	cmp	[VDMAD_Machine_Type], ISA_Machine
	je	label
	    ENDM

IF_MCA_JMP  MACRO label
ifndef MASM6
IF 2
IFNDEF VDMAD_Machine_Type
	EXTRN VDMAD_Machine_Type:BYTE
ENDIF
ENDIF
endif   ; MASM6
	cmp	[VDMAD_Machine_Type], MCA_Machine
	je	label
	    ENDM

IF_EISA_JMP  MACRO label
ifndef MASM6
IF 2
IFNDEF VDMAD_Machine_Type
	EXTRN VDMAD_Machine_Type:BYTE
ENDIF
ENDIF
endif   ; MASM6
	cmp	[VDMAD_Machine_Type], EISA_Machine
	je	label
	    ENDM

IF_NOT_ISA_JMP	MACRO label
ifndef MASM6
IF 2
IFNDEF VDMAD_Machine_Type
	EXTRN VDMAD_Machine_Type:BYTE
ENDIF
ENDIF
endif   ; MASM6
	cmp	[VDMAD_Machine_Type], ISA_Machine
	jne	label
		ENDM

IF_NOT_MCA_JMP	MACRO label
ifndef MASM6
IF 2
IFNDEF VDMAD_Machine_Type
	EXTRN VDMAD_Machine_Type:BYTE
ENDIF
ENDIF
endif   ; MASM6
	cmp	[VDMAD_Machine_Type], MCA_Machine
	jne	label
		ENDM

IF_NOT_EISA_JMP  MACRO label
ifndef MASM6
IF 2
IFNDEF VDMAD_Machine_Type
	EXTRN VDMAD_Machine_Type:BYTE
ENDIF
ENDIF
endif   ; MASM6
	cmp	[VDMAD_Machine_Type], EISA_Machine
	jne	label
		ENDM


DMA_Q_OUT MACRO String, reg1, reg2
IFDEF DMA_TRACE
	Queue_Out <String>, reg1, reg2
ENDIF
	ENDM

