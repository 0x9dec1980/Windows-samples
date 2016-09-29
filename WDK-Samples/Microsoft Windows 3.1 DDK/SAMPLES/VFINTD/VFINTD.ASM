PAGE 58,132
;******************************************************************************
TITLE VFINTD - 
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1990
;
;   Title:      VFINTD.ASM - 
;
;   Version:    3.00
;
;   Date:       Oct-10-1990
;
;   Author:     Neil Sandlin
;
;   This is a sample VxD which allows an application to "trap" floppy
;   interrupts, even though this "global" interrupt would normally be
;   reflected into whatever VM is running.
;
;   PLEASE NOTE: This is not an official Microsoft product. This is sample
;   code that was written to illustrate possible applications of functions 
;   that are documented in the Windows Device Development Kit. Microsoft 
;   does not guarantee the accuracy or reliability of this code.
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV                 DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;
;==============================================================================

        .386p

;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

        .XLIST
        INCLUDE VMM.Inc
        INCLUDE Debug.Inc
        INCLUDE VPICD.Inc
        INCLUDE vfintd.inc
        .LIST


;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VFINTD, 3, 0, VFINTD_Control, VFINTD_Dev_ID ,,  \
                                    VFINTD_API_Proc, VFINTD_API_Proc


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_LOCKED_DATA_SEG

VFINTD_Owner    dd  0

VFINTD_IRQ_Desc VPICD_IRQ_Descriptor <6,VPICD_Opt_Read_Hw_IRR, \
                          OFFSET32 VFINTD_Hw_INT,,OFFSET32 VFINTD_EOI>

VxD_LOCKED_DATA_ENDS


;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
;   VFINTD_Device_Init
;
;   DESCRIPTION:
;
;       This function is called during enhanced mode startup. The only
;       thing we do here is inform the VPICD that we want to virtualize
;       the floppy IRQ. From then on, hardware interrupts and EOI's will
;       flow through our own routines.
;
;
;==============================================================================

BeginProc VFINTD_Device_Init

IFDEF DEBUG
        mov     eax,VFINTD_Dev_ID
        Trace_Out "Floppy IRQ router: dev=#ax"
ENDIF
        
    	mov	edi, OFFSET32 VFINTD_IRQ_Desc
     	VxDcall VPICD_Virtualize_IRQ

        clc
        ret
EndProc VFINTD_Device_Init

VxD_ICODE_ENDS



;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VFINTD_Destroy_VM
;
;   DESCRIPTION:
;
;       This function is called when a VM is destroyed. It checks to see
;       if the VM being destroyed has the focus. If so, it releases the
;       focus.
;
;
;==============================================================================

BeginProc VFINTD_Destroy_VM
        cmp     [VFINTD_Owner], ebx         ;the same vm?
        jnz     short VFINTD_dvmexit
        xor     eax,eax
        mov     [VFINTD_Owner], eax

IFDEF DEBUG
    Trace_Out "VFINTD: Owner=#eax"
ENDIF

VFINTD_dvmexit:
        clc
        ret
EndProc VFINTD_Destroy_VM

;******************************************************************************
;
;   VFINTD_Control
;
;   DESCRIPTION:
;
;       This routine is the main entry from VMM. All message traffic
;       comes through this routine. 
;
;
;==============================================================================

BeginProc VFINTD_Control

        Control_Dispatch Device_Init, VFINTD_Device_Init
        Control_Dispatch Destroy_VM, VFINTD_Destroy_VM
        clc
        ret

EndProc VFINTD_Control


;VxD_LOCKED_CODE_ENDS


;VxD_CODE_SEG


;******************************************************************************
;			       A P I
;******************************************************************************


BeginDoc
;******************************************************************************
;
;   VFINTD_API_Proc
;
;   DESCRIPTION:
;
;       This is the exported API procedure that is callable from VM's. 
;       An application needs only to use INT 2Fh, AX=1684h, BX=device ID
;       and a call back address is returned. Then, when the
;       address is called, eventually it ends up here.
;
;
;==============================================================================
EndDoc

BeginProc VFINTD_API_Proc

	and	[ebp.Client_Flags], NOT CF_Mask ; clear VM's carry flag

	cmp	[ebp.Client_AX], 0              ; check out value in VM's AX
	jne	SHORT VFINTD_API_Test_Set
	mov	[ebp.Client_AX], 300h           ; API - Get Version Number
	jmp SHORT VFINTD_exit

VFINTD_API_Test_Set:
	cmp	[ebp.Client_AX], 100h           ; VM requesting floppy focus
	jne	SHORT VFINTD_API_Test_Release
	mov 	[VFINTD_Owner], ebx		    ; set focus
	jmp SHORT VFINTD_exit

VFINTD_API_Test_Release:
	cmp	[ebp.Client_AX], 200h           ; VM releasing focus
	jne	SHORT VFINTD_API_Invalid
    xor eax,eax
    mov [VFINTD_Owner], eax             ; release focus
	jmp SHORT VFINTD_exit

VFINTD_API_Invalid:
	or	[ebp.Client_Flags], CF_Mask

VFINTD_exit:
    mov eax,[VFINTD_Owner]

IFDEF DEBUG
    Trace_Out "VFINTD: Owner=#eax"
ENDIF

	ret

EndProc VFINTD_API_Proc

VxD_CODE_ENDS



VxD_LOCKED_CODE_SEG


;******************************************************************************
;          H A R D W A R E   I N T E R R U P T   R O U T I N E S
;******************************************************************************

;******************************************************************************
;
;   VFINTD_Hw_Int
;
;   DESCRIPTION:
;
;       This is the routine that gets control on a floppy interrupt.
;       If the variable VFINTD_Owner contains a VM control block address,
;       then the interrupt is reflected into that VM.
;
;
;==============================================================================

BeginProc VFINTD_Hw_Int, High_Freq

        cmp     [VFINTD_Owner], 0           ; has focus been set?
        jz      short VFINTD_Hw_exit
        mov     ebx, [VFINTD_Owner]         ; load the owner VM

VFINTD_Hw_exit:
        VxDjmp  VPICD_Set_Int_Request

EndProc VFINTD_Hw_Int



;******************************************************************************
;
;   VFINTD_EOI
;
;   DESCRIPTION:
;
;       This routine is required by VPICD. It performs default 
;       processing.
;
;
;==============================================================================
BeginProc VFINTD_EOI, High_Freq

	VxDCall VPICD_Phys_EOI
	VxDjmp	VPICD_Clear_Int_Request

EndProc VFINTD_EOI


VxD_LOCKED_CODE_ENDS

        END

