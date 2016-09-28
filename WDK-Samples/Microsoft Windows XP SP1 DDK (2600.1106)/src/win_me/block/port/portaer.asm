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

;===========================================================================
	page	,132
	title	aer - port driver IOS asynchronous event routines
	name	port.386
;===========================================================================
;
;   Module:
;		This module handles all asynchronous event communications with
;		IOS.  Asynchronous events are not normal requests.  These are
;		specific function calls generated from IOS, not external
;		sources
;
;===========================================================================
;
;   Functional Description: - we pick up the asynchronous event packet (AEP)
;   -----------------------   and dispatch it according to the AEP_FUNC
;			
;============================================================================

;============================================================================
;				I N C L U D E S
;============================================================================
.386p

.xlist
	include vmm.inc

	include	blockdev.inc
	include	bpb.inc

	include ilb.inc 	;IOS linkage block

	include aep.inc 	;Async event packet

	include ddb.inc 	;Driver information block
	include	portddb.inc	;port driver specific DDB
	include dcb.inc 	;DCB

	include isp.inc 	;IOS Service Block

	include	scsidefs.inc	;SCSI definitions
	include scsi.inc
	include	srb.inc
	include	scsiport.inc

	include	debug.inc	; general debug macros
	include	configmg.inc	; used to process the dev node
	include	vpicd.inc	; interface to VPICD
	include portinfo.inc	; general port driver information
	include	opttest.inc	; optimized test, or, and and macros
	include	iodebug.inc	; dragon debug stuff
.list

;============================================================================
;			  D A T A   S E G M E N T
;============================================================================

;============================================================================
;	                 P A G E A B L E   D A T A
;============================================================================

VXD_PAGEABLE_DATA_SEG

; port_device_count keeps track of the number of devices that we are being
; called for

public port_device_count
	port_device_count	db 0

VXD_PAGEABLE_DATA_ENDS

;============================================================================
;			E X T E R N A L   C O D E
;============================================================================

VXD_LOCKED_CODE_SEG

; code in port.asm
	extrn	port_ilb:dword		; IOS linkage block

; code in portreq.asm
	extrn	Port_request:near	; main request entry point

; code in portisr.asm

	extrn	Port_irq_handler:near	; ISR

;==============================================================================
; 			     M A I N   C O D E
;==============================================================================

;==============================================================================
;
; Port_Async_Request()
;
;	Help_Async_Request basically jumps to the appropriate routine 	
;	indicated by the AER command in the Async Event Packet (AEP).	
;									
;
; Entry: pointer to AEP on stack
;
; Exit:
;	AEP_result set to success/fail
;
; Destroys:     all
;
;==============================================================================


BeginProc Port_Async_Request, esp

ArgVar AEPPtr, DWORD

	TRAP
	EnterProc

;
; set up the pointer to the stack frame, point to the aep, obtain the
; function code from it, call the appropriate function handler, and
; return to our caller
;
	mov	ebx, AEPPtr

	; assume success

	mov	[ebx.AEP_result], AEP_SUCCESS

	mov	si,[ebx.AEP_Func]	  ; pick up the aep's function code.

	cmp	si, AEP_INITIALIZE 	  ; is it an intialize call?
	je	Port_initialize		  ; if so, jump

	cmp	si, AEP_DEVICE_INQUIRY 	  ; is it an inquiry call?
	je	Port_device_inquiry	  ; if so, jump

	cmp	si, AEP_CONFIG_DCB	  ; is it a config call?
 	je	Port_cfg_device		  ; if so, jump

 	cmp	si, AEP_IOP_TIMEOUT	  ; is it a timeout call?
	je	Port_iop_timeout	  ; if so, jump

	cmp	si, AEP_BOOT_COMPLETE	  ; is it init complete?
	je	Port_init_complete	  ; if so, jump

;
; report that the function code is not supported by this port driver
;

	mov	[ebx.AEP_result],AEP_FAILURE; set result code to indicate error.

	LeaveProc
	Return

EndProc Port_Async_Request

VXD_LOCKED_CODE_ENDS

;==============================================================================
;		 	 P A G E A B L E   C O D E
;==============================================================================


; all of the AEP routines that we are interested in handling are low frequency,
; and they are not called at interrupt time, so we will keep them in pageable
; code segment to reduce the working set size

VXD_PAGEABLE_CODE_SEG

;==============================================================================
;
; Port_device_inquiry() - determine whether there is a peripheral at the unit
;			number indicated in the DCB.
;
;
; Entry: ebx -> Async Event Packet (AEP)
;
; Exit:
;
; Destroys:     none
;
;==============================================================================

BeginProc Port_initialize

	TRAP

	EnterProc

	; ebx points to AEP

	;
	; first create a DDB for the adapter.  If it does not exist, we
	; will deallocate it later
	;

	sub	esp,size ISP_ddb_create		; allocate isp from the stack.

	mov	edi,esp				; point to the ISP stack frame

	mov	[edi].ISP_func,ISP_create_ddb  	; construct the isp for
	mov	[edi].ISP_ddb_size,size PortDDB ; ddb service.
	mov	[edi].ISP_ddb_flags, 0		; show no flags

	push	edi				; put ISP on stack

	call	[port_ilb.ILB_Service_rtn]   	; call IOS Service Routine

	mov	edi,[edi.ISP_ddb_ptr]		; edi-> new DDB

	add	esp,size ISP_ddb_create + 4 	; clean up the stack and arg

	;
	; call routine to parse the hardware parameters of the adapter.
	; these parameters come from Chicago's configuration manager, and
	; are determined by hardware detection routines.
	;

        call    Port_scan_inp_params 		; process devnode parameters.

	;
	; now determine if there is an adapter at the location specified
	; in the DDB, and initialize it if present.
	;

;	call	Port_initialize_adapter
	jc	Port_i_failure			; jump if failure

	;
	; now set the IRQ handler
	;

	call	Port_set_irq_handler

	;
	; all set.  note that AEP_SUCCESS was preset by calling routine.
	;

Port_i_exit:
	LeaveProc
	Return

Port_i_failure:

	;
	; deallocate the DDB and show failure
	;

	sub 	esp, size ISP_ddb_dealloc	;
	mov	eax, esp 			; release
        mov     [eax].ISP_ddb_d_ddb, edi	;
	mov	ecx, [port_ilb].ILB_dvt		;
        mov     [eax].ISP_ddb_d_dvt, ecx	;
	mov	[eax].ISP_func, ISP_DEALLOC_DDB	; DDB
						;
	push	eax				;
	call	[port_ilb.ILB_Service_rtn]	;

	add	esp, size ISP_ddb_dealloc + 4	; clean up the stack and arg

	mov	[ebx.AEP_result], AEP_FAILURE	; show failure
	jmp	Port_i_exit

EndProc Port_initialize


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; 	Port_Set_IRQ_Handler 	   
;		Try to register adapter's IRQ with VPICD.
;
;	INPUT: EDI-> DDB   
;
;	OUTPUT:
;
;     	DESTROYS: all but edi
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc	Port_Set_IRQ_Handler

	;
	; call VPICD to register IRQ
	;

        sub     esp, size VPICD_IRQ_Descriptor  ; space for IRQ pkt
        mov     [esp].VID_Hw_Int_Proc, offset32 Port_IRQ_Handler
						; set IRQ handler

	mov	[esp].VID_HW_Int_Ref, edi	; *DDB = reference data
        mov     [esp].VID_Options, VPICD_Opt_Can_Share OR VPICD_OPT_REF_DATA
					        ; set up options
	movzx	eax, [edi].DDB_irq_number	; set up IRQ level
        mov     [esp].VID_IRQ_Number, ax        ;
                
        mov     [esp].VID_IRET_Time_Out, 500    ; 500 ms virt timeout

	TestMem	[edi].DDB_Port_flags, DDB_BF_IRQ_SHARED
	jnz	Port_s_i_set_irq

	ClrFlag	[esp].VID_Options, VPICD_Opt_Can_Share

Port_s_i_set_irq:

        mov     [esp].VID_Virt_Int_Proc,0       ; zero unapplicable
        mov     [esp].VID_EOI_Proc, 0           ; fields
        mov     [esp].VID_Mask_Change_Proc, 0   ; 
        mov     [esp].VID_IRET_Proc, 0          ;

	mov	edx, edi			; save *DDB
        mov     edi, esp                        ; hook the IRQ
        cli                                     ; ints off until
                                                ; handle saved
        VxDCall VPICD_Virtualize_IRQ            ;
	mov	edi, edx			; restore *DDB
        TRAPc

	;
	; since we may receive data buffers that are instanced, we
	; must disable our IRQ's when swapping, since we could 
	; reenter the instance swapper.
	;
	; eax = VPICD handle

	mov	[edi].DDB_irq_handle, eax	; save handle
	VxDCall	VPICD_Auto_Mask_At_Inst_Swap	; disable irq when
						; instance swapping
	sti					; ints safe now
        VxDCall VPICD_Physically_Unmask         ; unmask the irq

        add     esp, size VPICD_IRQ_Descriptor  ; rel space for IRQ pkt

	ret

EndProc	Port_Set_IRQ_Handler 


;========================================================================								    *
;								    
; Port_Scan_Inp_Params                                 
; 	Parse the devnode adapter configuration information.
;
;								    
; 	Entry: 	EBX = AEP packet
;		EDI = DDB packet         	                            
;
;	Exit:  	DDB filled in from registry data
;
;      	Uses:	eax, ecx
;								    
;========================================================================
BeginProc	Port_Scan_Inp_Params, esp

LocalVar	prdResDes, dword
LocalVar	CurResDesOrLogConf, dword
LocalVar	ForResource, dword
LocalVar	pResourceID, dword

	EnterProc
	SaveReg	<esi, edx>
	
        mov	esi, [ebx.AEP_bi_i_hdevnode]	; get *device node
	
	;
	; get first/next configuration entry
	;

	lea	eax, CurResDesOrLogConf		; space for *config

	VxDCall	_CONFIGMG_Get_First_Log_Conf <eax, esi, ALLOC_LOG_CONF>
						; get config entry

	cmp	eax, CR_SUCCESS			; success?
	TRAPne
	jne	Port_sip_end			; if not, just exit.

	;
	; process the logical configuration element
	;

	mov	ForResource, ResType_All	; any resource

Port_sip_config_loop:
	lea	eax, prdResDes			; pointer to new resource des	
	lea	edx, pResourceID		; id for new resource des
	VxDCall	_CONFIGMG_Get_Next_Res_Des < eax, CurResDesOrLogConf, ForResource, edx, 0>

	cmp	eax, CR_SUCCESS			; good status?
	jne 	Port_sip_end			; if not, jump
						
	mov	eax, prdResDes	       		;
	mov	CurResDesOrLogConf, eax		;
	mov	prdResDes, eax

	mov	eax, pResourceID		; get type.

	cmp	eax, ResType_IRQ		; type IRQ?
	jne	Port_sip_check_port		; if not, jump.

	mov	eax, prdResDes			; get pointer to descriptor

        mov     dx, [eax].IRQD_Alloc_Num
        mov     [edi].DDB_irq_number,dl 	; Set IRQ to return value in AX

	TestMem	[eax].IRQ_Des_s.IRQD_Flags, fIRQD_Share
	jz	Port_sip_config_loop		; if unshared, jump

	;
	; show shared
	;

	SetFlag	[edi].DDB_Port_flags, DDB_BF_IRQ_SHARED
	jmp	Port_sip_config_loop		; loop for next
	
Port_sip_check_port:
	cmp	eax, ResType_IO			; type port?
	TRAPne
 	jne	Port_sip_config_loop		; loop for next
	
	mov	eax, prdResDes			; get pointer to descriptor
	mov	si, [eax].IOD_Alloc_Base	; get start of range

	mov	[edi].DDB_base_ioa,si		; Set base I/O register address
	jmp	Port_sip_config_loop		; jump for next resource des.
	
Port_sip_end:
	RestoreReg	<edx, esi>
	LeaveProc
	Return

EndProc	Port_Scan_Inp_Params


;==============================================================================
;
; Port_device_inquiry() - determine whether there is a peripheral at the unit
;			number indicated in the DCB.
;
;
; Entry: ebx -> Async Event Packet (AEP)
;
; Exit:
;
; Destroys:     none
;
;==============================================================================

BeginProc Port_device_inquiry

	TRAP

	EnterProc

	;ebx points to AEP

	mov	esi, [ebx].AEP_i_d_dcb		; get *DCB
	AssertDCB <esi>				;

	movzx	ecx,[esi.DCB_unit_on_ctl] 	; spindle number.

	;
	; at this point, we determine if a device exists at the unit number
        ; in ecx.
	;
	; if so, we return AEP_SUCCESS in the AEP. 
	; if not, we return AEP_FAILURE.
	; if not, and we are sure there are no more, we return 
	; AEP_NO_MORE_DEVICES.
	;

	cmp	ecx, MAX_DRIVES - 1		; zero based
	ja	Port_di_no_more_devices

	;
	; determine if the drive is present
	;

;	call	sniff_for_drive

	or	eax, eax
	jnz	Port_di_no_device

	;
	; fill in any necessary DCB parameters and return success
	;

	mov	[ebx].AEP_result, AEP_SUCCESS

Port_di_exit:
	LeaveProc
	Return


Port_di_no_device:
	mov	[ebx].AEP_result, AEP_FAILURE	; no drive at this ID
	jmp	Port_di_exit

Port_di_no_more_devices:
	mov	[ebx].AEP_result, AEP_NO_MORE_DEVICES	; all done scanning
	jmp	Port_di_exit

EndProc Port_device_inquiry


;==============================================================================
;
; Port_cfg_device() - Examines a DCB that is passed in to determine if we want
;		     to work with this device
;
;
; Entry: ebx -> Async Event Packet (AEP)
;
; Exit:
;
; Destroys:     none
;
;==============================================================================

BeginProc Port_cfg_device

	TRAP

	;ebx points to AEP
	;Address the DCB

	mov	esi, [ebx].AEP_d_c_dcb		; pick up address of the DCB

	inc	[port_device_count]		; show one more device

	;put port driver info into calldown stack

Port_cfg_insert:

	; save registers

	push	esi
	push	edi

	; set up parameters to allocate a packet for calling the ISP functions

	mov	esi,edi
	sub	esp,size ISP_calldown_insert	;allocate ios service pkt (from stack)
	mov	edi,esp				;point to the gotten isp.

	mov	[edi].ISP_func,ISP_insert_calldown	; set the functions

	mov	[edi].ISP_i_cd_flags, DCB_dmd_small_memory
							; indicate demands

	mov	[edi].ISP_i_cd_dcb, esi			; insert on this DCB

	mov	[edi].ISP_i_cd_req,OFFSET32 Port_Request; our request entry

	mov	al, [ebx.AEP_lgn]		; get load group number
	mov	[edi].ISP_i_cd_lgn, al	        ; store in ISP

        mov     eax,[ebx.AEP_ddb]          	; get DDB address
	mov	[edi].ISP_i_cd_ddb, eax		; and set in ISP

	; if we require scratch space in every IOP, we can add it here
	
	xor	eax, eax

ifdef SCRATCH
	add	eax, size SCRATCH_AREA
endif ;SCRATCH

	mov	[edi].ISP_i_cd_expan_len, ax

	push	edi				;ptr to packet. it is edi!
	call	[Port_ilb.ILB_Service_rtn] 	;make ios call
	add	esp,size ISP_calldown_insert+4	;cleanup the stack and return
	pop	edi
	pop	esi
	
vcd_ret:

	ret				; return to our caller.

EndProc Port_cfg_device


;==============================================================================
;
; Port_init_complete() - Called at initialization complete time.  If we want
;			to unload our driver (i.e. no devices found) We fail
;			this call.  Otherwise, we just return success
;
;
; Entry: ebx -> Async Event Packet
;
; Exit:
;
; Destroys:     all but ebp
;
;==============================================================================

BeginProc Port_init_complete

	TRAP

	cmp	[port_device_count], 0			; any devices?
	jne	Port_i_c_exit				; if, jump

	TRACE_OUT	"Port: no devices in the system, unloading "
	mov	[ebx].AEP_result, AEP_FAILURE		; unload driver

Port_i_c_exit:
	ret

EndProc Port_init_complete

VXD_PAGEABLE_CODE_ENDS

VXD_LOCKED_CODE_SEG

;==============================================================================
;
; Port_iop_timeout() - called when IOS detects a request which has timed
;		       out (may or may not be our request).
;
; Entry: ebx -> Async Event Packet
;
; Exit:
;
; Destroys:     all but ebp
;
;==============================================================================

BeginProc Port_iop_timeout

	TRAP
	
	;
	; this routine should pick up the IOP from the AEP, determine if 
	; the IOP is one which we have in process, and if it is, should
	; reset the hardware as appropriate, and either retry or call back
	; the request with error.
	;
	; This routine then
	; should return AEP_SUCCESS if the IOP was ours, or AEP_FAILURE
	; if not.

	Return

EndProc Port_iop_timeout

VXD_LOCKED_CODE_ENDS

end
