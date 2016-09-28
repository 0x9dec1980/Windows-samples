
;===========================================================================
	page	,132
	title	init - 3mode floppy vsd initialization code
	name	vsdinit.asm
;===========================================================================
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
; PURPOSE.
;
; Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.
;
;   Module:
;	      Contains code to intializae the 3mode vsd as a Virtual Device
;
;   Version:  0.001
;
;   Date:     March 2, 1995
;
;
;===========================================================================
;
;   Change log:
;
;     DATE    REVISION			DESCRIPTION
;   --------  --------	-------------------------------------------------------
;   3/02/95   Original
;
;===========================================================================
;
;   Functional Description: - During device init, we register with IOS to
;   -----------------------   determine whether we stay resident or not
;
;============================================================================
.386p

;============================================================================
;				I N C L U D E S
;============================================================================

.xlist
	include vmm.inc
	include vmmreg.inc
	include defs.inc
	include drp.inc 	; Device registration packet
	include ilb.inc 	; IOS linkage block
        include ios.inc
	include aep.inc
	include debug.inc
	include vsdinfo.inc	; VSD Identification Equates
.list

;============================================================================
;		   I N I T I A L I Z A T I O N  D A T A
;============================================================================

VXD_IDATA_SEG

;-------------------------------------------------------------------------
;
; Driver Registry Packet
;
; This packet identifies us for IOS.  See vsdinfo.inc for definitions and
; descriptions of the parameters
;

Drv_Reg_Pkt DRP <EyeCatcher, \  	    ; standard DRP signature string
		 DRP_VSD_8,  \  	    ; Dragon layer of 3mode drivers
		 OFFSET32 _3mode_Async_Request, \   ; AER entry point
		 OFFSET32 vsd_ilb, \ 	    ; IOS linkage block
		 VSD_NAME, \ 		    ; driver name
		 VSD_REV, \ 		    ; driver revision number
		 VSD_FEATURE, \ 	    ; feature code
		 VSD_IF>		    ; I/F requirements

;============================================================================
; 			E X T E R N A L   D A T A
;============================================================================


VXD_IDATA_ENDS

;============================================================================
; 			  P U B L I C   D A T A
;============================================================================

VXD_LOCKED_DATA_SEG

;; Move from VSDaer.asm 95-07-21(M9 Update) Start
;;
;============================================================================
;
; Driver Identification String
;
; The installer of 3mode VSDs writes the signature string of the selected
; driver in the registry, under the key...
;
;     HKLM\System\CurrentControlSet\Control\3mode\3modeDrv
;
; During initialization, in response to AEP_INITIALIZE, the 3mode VSD
; queries the registry setting and compares it against 3mode VSD_NAME
; defined in VSDINFO.INC.  If they match, the driver should return success
; to remain resident.  If they don't, the driver must fail the
; initialization and IOS will unload it.  No more than one 3mode VSD
; driver should ever be loaded.
;
;===========================================================================

public	RegNode 	    ; registry path name
public	RegKey		    ; registry key name
public	RegHandle	    ; opened registry query handle
public	RegStringValue	    ; returned registry key value
public	RegValueType	    ; returned value type
public	_3modeVSDSig	    ; driver identification string

;;RegNode 	db	'system\currentcontrolset\services\class\fdc\0000',0
RegNode 	db	'system\currentcontrolset\control\3mode',0
RegKey		db	'3modeDrv',0
RegHandle	dd	0
RegStringValue	db	VSD_NAME_LEN+1 dup (?)
RegValueType	dd	?
_3modeVSDSig	db	VSD_NAME,0	; defined in VSDINFO.INC

;; Move from VSDaer.asm 95-07-21(M9 Update) end

; The IOSUBSYSTEM linkage block is used for internal calls to IOS

public	vsd_ilb
vsd_ilb		ILB	<>	;IO Subsystem linkage block

VXD_LOCKED_DATA_ENDS

;============================================================================
;	                 P A G E A B L E   D A T A
;============================================================================

VXD_PAGEABLE_DATA_SEG

extrn	fDriverLoaded:word

VXD_PAGEABLE_DATA_ENDS


;============================================================================
;	           D E V I C E   D E C L A R A T I O N
;============================================================================

VXD_LOCKED_CODE_SEG

; use the drive registration packet as initialization data

ifdef	CPQ_1
DECLARE_VIRTUAL_DEVICE COMPAQ1,\
	VSD_MAJOR_VERSION, \
	VSD_MINOR_VERSION,\
	_3modeVSD_Control,,\
	UNDEFINED_INIT_ORDER\
	,,,Drv_Reg_Pkt
endif
ifdef	CPQ_2
DECLARE_VIRTUAL_DEVICE COMPAQ2,\
	VSD_MAJOR_VERSION, \
	VSD_MINOR_VERSION,\
	_3modeVSD_Control,,\
	UNDEFINED_INIT_ORDER\
	,,,Drv_Reg_Pkt
endif
ifdef	CPQ_3
DECLARE_VIRTUAL_DEVICE COMPAQ3,\
	VSD_MAJOR_VERSION, \
	VSD_MINOR_VERSION,\
	_3modeVSD_Control,,\
	UNDEFINED_INIT_ORDER\
	,,,Drv_Reg_Pkt
endif
ifdef	CPQ_4
DECLARE_VIRTUAL_DEVICE COMPAQ4,\
	VSD_MAJOR_VERSION, \
	VSD_MINOR_VERSION,\
	_3modeVSD_Control,,\
	UNDEFINED_INIT_ORDER\
	,,,Drv_Reg_Pkt
endif
ifdef	CPQ_5
DECLARE_VIRTUAL_DEVICE COMPAQ5,\
	VSD_MAJOR_VERSION, \
	VSD_MINOR_VERSION,\
	_3modeVSD_Control,,\
	UNDEFINED_INIT_ORDER\
	,,,Drv_Reg_Pkt
endif
ifdef	CPQ_x
DECLARE_VIRTUAL_DEVICE COMPAQX,\
	VSD_MAJOR_VERSION, \
	VSD_MINOR_VERSION,\
	_3modeVSD_Control,,\
	UNDEFINED_INIT_ORDER\
	,,,Drv_Reg_Pkt
endif

;============================================================================
;			E X T E R N A L   C O D E
;============================================================================

extrn	_3mode_Async_Request:near

;============================================================================
;			    M A I N   C O D E
;============================================================================


;============================================================================
;
; _3modeVSD_Control() -
;
;     Device control procedure for the 3mode VSD.  Dispatches all Windows
;     VxD messages.
;
; Exit:
;
;     If carry clear then
;	  Successful
;     else
;	  Control call failed
;
; Uses:
;     eax, ebx, ecx, edx, esi, edi, flags
;
;============================================================================

_3modeVSD_Control PROC NEAR

	Control_Dispatch SYS_DYNAMIC_DEVICE_INIT, _3modeVSD_Device_Init
	clc
	ret

_3modeVSD_Control ENDP

VXD_LOCKED_CODE_ENDS


;============================================================================
;	   D E V I C E   I N I T I A L I Z A T I O N   C O D E
;============================================================================

VXD_ICODE_SEG

;============================================================================
;									
; _3modeVSD_Device_Init() -
;									
;	VSD_Init registers with the IOS and based on the return code from
;	IOS leaves all, some or none of the driver resident.  During the
;	registration call, IOS will call the VSD's Async Event Routine (AER).
;
; Entry:
;
;     ebx = System VM handle (not used)
;     edx = Reference data from real mode init portion
;
; Exit:
;
;     If successful then
;	  Carry flag is clear
;     else
;	  Carry flag is set to indicate an error - device not initialized
;
;============================================================================

beginproc _3modeVSD_Device_Init

;;******* Insert 95-07-21(M9 update)
	call	Check3modeRegistry
	cmp	fDriverLoaded, AEP_FAILURE	; fDriverLoaded = -1
	je	vsd_d_i_abort
;;****** End f Insert 95-07-21(M9 update)

	push	OFFSET32 Drv_Reg_Pkt	;packet (DRP)
        VxDCall	IOS_Register		;call registration
	add	esp,04			;Clean up stack
	
	cmp	Drv_Reg_Pkt.DRP_reg_result,DRP_REMAIN_RESIDENT
	je	short vsd_d_i_done

	cmp	Drv_Reg_Pkt.DRP_reg_result,DRP_MINIMIZE
	jne	short vsd_d_i_abort

vsd_d_i_minimize:

	TRAP
	jmp	short vsd_d_i_done

vsd_d_i_abort:

	TRAP
	jmp	short vsd_d_i_failed

vsd_d_i_done:

	xor	eax, eax		; clear CF
	ret

vsd_d_i_failed:

	TRAP
	stc
	ret

endproc _3modeVSD_Device_Init

;;******* Insert 95-07-21(M9 update)
;============================================================================
;									
; Check3modeRegistry() -
;									
;	This routine check the 3mode registry.
;     		HKLM\System\CurrentControlSet\Control\3mode\3modeDrv
;
; Entry:	none
;
; Exit:
;
;     If successful then
;	mov	fDriverLoaded, AEP_SUCCESS
;     else
;	mov	fDriverLoaded, AEP_FAILURE	
;
;============================================================================
beginproc	Check3modeRegistry
	push	esi
	push	edi
	push	eax
	push	ecx
	push	edx

	mov	eax, offset32 RegNode		; registry node name
	mov	edx, offset32 RegHandle 	; return reg handle
	VMMCall _RegOpenKey, <HKEY_LOCAL_MACHINE, eax, edx>
	cmp	eax, ERROR_SUCCESS		; were we successful?
	je	@F				; yes, continue

	jmp	vsd_i_failed			; no, couldn't find our node

@@:
	;
	; Read the registry key
	;

	push	ebx				; save AEP
	mov	byte ptr RegStringValue, 0	; zero terminate return string
	mov	eax, RegHandle			; our registry handle
	mov	ebx, offset32 RegKey		; key to query
	mov	ecx, offset32 RegValueType	; returns value type
	mov	edx, offset32 RegStringValue	; returns the string
	mov	edi, VSD_NAME_LEN
	VMMCall _RegQueryValueEx, <eax, ebx, 0, ecx, edx, edi>
	pop	ebx				; ebx <- AEP

	push	eax				; save query result
	mov	eax, RegHandle			; our registry handle
	VMMCall _RegCloseKey, <eax>		; close the registry
	pop	eax				; restore query result

	cmp	eax,ERROR_SUCCESS		; were we successful?
	je	@F				; yes, continue

	jmp	vsd_i_failed			; no, key value is not found

@@:
	;
	; Match registry key against our driver signature
	;

	mov	esi, offset32 RegStringValue	; registry key value
	mov	edi, offset32 _3modeVSDSig	; our driver signature
	mov	ecx, VSD_NAME_LEN
;;********** Insert 95-07-20(M9 Update) Just for safe
	cld
;;********** End of insert 95-07-20(M9 Update)
	repe	cmpsb		
;;********** Mod 95-07-20(M9 Update)  BUGFIX
	je	vsd_i_exit
;;	or	ecx, ecx			; identical?
;;	jz	@F				; yes, we are it!
;;*********** Mod 95-07-20(M9 Update) 

vsd_i_failed:
	mov	fDriverLoaded, AEP_FAILURE	; fDriverLoaded = -1

vsd_i_exit:
	pop	edx
	pop	ecx
	pop	eax
	pop	edi
	pop	esi

	ret

endproc		Check3modeRegistry
;;******* End of insert 95-07-21(M9 update)


VXD_ICODE_ENDS

end
