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
	title	port - port driver VxD initialization code
	name	port.pdr
;===========================================================================
;
;   Module:
;		Contains code to intializae the port driver as a Virtual Device
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
	include portinfo.inc	; port driver Identification Equates
	include	defs.inc
	include drp.inc 	; Device registration packet
	include ilb.inc 	; IOS linkage block
        include ios.inc
	include debug.inc
.list

;============================================================================
;		   I N I T I A L I Z A T I O N  D A T A
;============================================================================

VXD_IDATA_SEG

; DRIVER REGISTRATION PACKET.  This packet identifies us for IOS
; see portinfo.inc for definitions and descriptions of the parameters

; changing the feature code field (PortFeature) allows you to select
; different events that you wish to be notified of from IOS

; changing the load group (DRP_MISC_PD) indicates what layer you are
; in the architecture.  It is possible to select more than one

Drv_Reg_Pkt	DRP <EyeCatcher, DRP_MISC_PD, offset32 Port_Async_Request,offset32 port_ilb,PortName,PortRev,PortFeature,Port_IF>


;============================================================================
; 			E X T E R N A L   D A T A
;============================================================================


VXD_IDATA_ENDS

;============================================================================
; 			  P U B L I C   D A T A
;============================================================================

VXD_LOCKED_DATA_SEG

; The IOSUBSYSTEM linkage block is used for internal calls to IOS

public	port_ilb
port_ilb		ILB	<>	;IO Subsystem linkage block

VXD_LOCKED_DATA_ENDS

;============================================================================
;	           D E V I C E   D E C L A R A T I O N
;============================================================================

VXD_LOCKED_CODE_SEG

; use the drive registration packet as initialization data

DECLARE_VIRTUAL_DEVICE PORT,\
	PORT_MAJOR_VERSION, \
	PORT_MINOR_VERSION,\
	PORT_Control,,\
	UNDEFINED_INIT_ORDER\
	,,,Drv_Reg_Pkt

;============================================================================
;			E X T E R N A L   C O D E
;============================================================================

extrn	Port_Async_Request:near

;============================================================================
;			    M A I N   C O D E
;============================================================================


;============================================================================
;
;   PROCEDURE: PORT_Control
;
;   DESCRIPTION:
;	Device control procedure for the port driver. Dispatches all Windows
;	VxD messages.
;
;   EXIT:
;	If carry clear then
;	    Successful
;	else
;	    Control call failed
;
;   USES:
;	EAX, EBX, ECX, EDX, ESI, EDI, Flags
;
;============================================================================

PORT_Control PROC NEAR

	Control_Dispatch SYS_DYNAMIC_DEVICE_INIT, PORT_Device_Init
	clc
	ret

PORT_Control ENDP

VXD_LOCKED_CODE_ENDS

;============================================================================
;	   D E V I C E   I N I T I A L I Z A T I O N   C O D E
;============================================================================


VXD_ICODE_SEG

;============================================================================
;									
; PORT_Device_Init - Port Driver Initialization 				
;									
;	Port_Device_Init basically registers with the IOS and based on the	
;	return code from IOS leaves all, some or none of the		
;	driver resident.  During the registration call, IOS will
;	call the port driver's Async Event Routine (AER).		
;									
;   ENTRY:
;       EBX = System VM handle (not used)
;       EDX = Reference data from real mode init portion
;
;   EXIT:
;       If successful then
;           Carry flag is clear
;       else
;           Carry flag is set to indicate an error -- Device not initialized
;
;
;============================================================================

BeginProc PORT_Device_Init

	TRAP

	; call IOS to register.  Before returning: IOS will possibly call our
	; Async_Event routine with the following messages:
	; 	AEP_INITIALIZE
	;	AEP_INQUIRY
	;	AEP_CONFIG_DCB

	push	OFFSET32 Drv_Reg_Pkt	;packet (DRP)
        VxDCall	IOS_Register		;call registration
	add	esp,04			;Clean up stack

	; decide our status based on the information that IOS gives us
	
	cmp	Drv_Reg_Pkt.DRP_reg_result,DRP_REMAIN_RESIDENT; should we stay?
	je	short Port_Init_Done     ; yes: carry is clear from compare, just exit

	cmp	Drv_Reg_Pkt.DRP_reg_result,DRP_MINIMIZE ; should we minimize?
	je	short Port_Init_Done	; yes: we can't minimize any more than
					; normal, so just return with carry clear
					; indicating success

	stc
Port_Init_Done:
	ret

EndProc PORT_Device_Init

VXD_ICODE_ENDS

end
