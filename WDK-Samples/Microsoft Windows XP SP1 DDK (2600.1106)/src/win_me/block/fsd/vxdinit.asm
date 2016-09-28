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
; Module - vxdinit.asm
;
; function - this is the virtual device initialization code for the default
;            file system
;
;******************************************************************************

    .386p

    .xlist
    INCLUDE VMM.Inc
    INCLUDE Debug.Inc
    INCLUDE V86MMGR.Inc
    INCLUDE SHELL.Inc
MASM=1

    include ios.inc
    include drp.inc
    include macro.inc
    include ifs.inc
    include ifsmgr.inc

    .LIST

Create_VDEF_Service_Table EQU True

    include vdef.inc


;******************************************************************************
;         V I R T U A L     D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VDEF, 3, 0, VDEF_Control, Undefined_Device_ID, \
               FSD_Init_Order

;******************************************************************************
;                E Q U A T E S
;******************************************************************************


VxD_IDATA_SEG

    extrn    Drv_Reg_Pkt:byte

VxD_IDATA_ENDS

VXD_LOCKED_DATA_SEG

    extrn    VDEF_ilb:dword              ; define pagefile ilb
    extrn   VDEF_async_event:near             ; async event proc

VXD_LOCKED_DATA_ENDS

;******************************************************************************
;            I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG

;******************************************************************************
;
;   VDEF_Sys_Critical_Init
;
;   DESCRIPTION:
;       VDEF does not require any system critical initialization
;
;   ENTRY:
;    EBX = System VM Handle
;
;   EXIT:
;    Carry clear (can not fail)
;
;   USES:
;    none
;
;==============================================================================

BeginProc VDEF_Sys_Critical_Init

;   Done!  Return with carry clear to indicate success.

    clc
    ret

EndProc VDEF_Sys_Critical_Init


;******************************************************************************
;
;   VDEF_Device_Init
;
;   DESCRIPTION:
;       registers with redirector and IOS
;       first checks to make sure both are present ...
;       BUGBUG - watch for re-entrancy, right now, we asssume no problems ...
;                so the critical section is not claimed
;
;   ENTRY:
;    EBX = Current VM Handle
;
;   EXIT:
;    Carry clear to indicate success
;
;   USES:
;
;==============================================================================

        extrn Mount_Def:near

BeginProc VDEF_Device_Init

    VxDcall    IFSMgr_Get_Version

    ;no redirector present, bail out if carry
    jc      vdix

    Vxdcall IOS_Get_Version

    jc    short vdix      ;no IOS present bail out.

    ;register with vredir
    ;register as default FSD
    VxDcall    IFSMgr_RegisterMount <offs Mount_Def , IFSMGRVERSION, 1>
    cld

    ;register with IOS
    call    VDEF_IOS_Reg
vdix:
    ret


EndProc VDEF_Device_Init


;******************************************************************************
;
;   VDEF_Init_Complete
;
;   DESCRIPTION:
;    We have to delay until this point before calling the V86MMGR to
;    allocate mapping regions.
;
;   ENTRY:
;    EBX = System VM Handle
;
;   EXIT:
;       carry is clear
;
;   USES:
;    All registers and flags
;
;==============================================================================

BeginProc VDEF_Init_Complete
    clc
    ret

EndProc VDEF_Init_Complete

;******************************************************************************
;
;   VDEF_IOS_reg
;
;   DESCRIPTION:
;       attck to IOS and register to obtain ILB entry points
;
;   ENTRY:
;    EBX = System VM Handle
;
;   EXIT:
;       carry is clear
;
;   USES:
;    All registers and flags
;
;==============================================================================


BeginProc VDEF_IOS_Reg

    ;
    ; Set up DRP packet for IOS driver registration and call IOS
    ;

    mov     [Drv_Reg_Pkt].DRP_LGN, DRP_TSD

    mov     [Drv_Reg_Pkt].DRP_AER,offset32 VDEF_async_event
    mov     [Drv_Reg_Pkt].DRP_ILB,offset32 VDEF_ilb

    push    offset32 Drv_Reg_Pkt    ;packet (DRP)
    VXDCall IOS_Register            ;call registration
    add     esp,04                  ;Clean up stack

    clc                             ; indicate no error
    ret

EndProc VDEF_IOS_Reg

VxD_ICODE_ENDS

;==============================================================================

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VDEF_Control
;
;   DESCRIPTION:
;    This is the main control procedure for the VDEF device.
;
;   ENTRY:
;    EAX = System control message
;    Other registers may contain parameters.  See DDK for details of each
;    system control call.
;
;   EXIT:
;    Standard system control exit (usually, carry set indicates error)
;
;   USES:
;    See individual procedures for details.
;
;==============================================================================

BeginProc VDEF_Control

    Control_Dispatch Sys_Critical_Init, VDEF_Sys_Critical_Init
    Control_Dispatch Device_Init, VDEF_Device_Init
    Control_Dispatch Init_Complete, VDEF_Init_Complete
;;    Control_Dispatch VM_Terminate, <SHORT VDEF_VM_Terminate>
    Control_Dispatch Sys_VM_Terminate, <SHORT VDEF_Sys_VM_Terminate>
;;    Control_Dispatch VM_Not_Executeable, <SHORT VDEF_VM_Not_Exec>
;;    Control_Dispatch Query_Destroy, VDEF_Query_Destroy
    Control_Dispatch Debug_Query, VDEF_Dump_Debug
    clc
    ret

EndProc VDEF_Control

VxD_LOCKED_CODE_ENDS


VxD_LOCKED_CODE_SEG

;******************************************************************************
;       VDEF_Dump_Debug
;
;Description:
;       This routine calls and debug dump information.  Currently just returns
;
; Entry:
;
; Exit:
;       carry is clear
;
; Uses:
;       none
;
;******************************************************************************


BeginProc VDEF_Dump_Debug
    clc
    ret
EndProc    VDEF_Dump_Debug


;******************************************************************************
;
;   VDEF_VM_Terminate
;   VDEF_Sys_VM_Terminate
;   VDEF_VM_Not_Exec
;
;   DESCRIPTION:
;    At present, VDEF doesn't provide any local I/O services.  Thus
;    these calls are nop's.
;
;   ENTRY:
;    EBX = Handle of VM being terminated/destroyed
;
;   EXIT:
;    Carry clear
;
;   USES:
;    All registers and flags
;
;==============================================================================

BeginProc VDEF_VM_Terminate

VDEF_VM_Not_Exec LABEL NEAR
VDEF_Sys_VM_Terminate LABEL NEAR

    clc
    ret

EndProc VDEF_VM_Terminate


;******************************************************************************
;
;   VDEF_Query_Destroy
;
;   DESCRIPTION:
;    At present, VDEF doesn't provide any local I/O services.  Thus
;    this calls is a nop.
;
;   ENTRY:
;    EBX = Handle of VM that is about to be nuked
;
;   EXIT:
;    If carry clear then
;        OK to nuke the VM
;    else
;        Can't close VM now -- Dialogue box will be displayed
;
;   USES:
;    EAX, ECX, EDI, ESI, Flags
;
;==============================================================================

BeginProc VDEF_Query_Destroy
    clc
    ret
EndProc VDEF_Query_Destroy



VxD_LOCKED_CODE_ENDS

;***    Real Mode Initialization Code
;
;    The following routine gets executed in real mode during the windows
;    initialization process.

VxD_REAL_INIT_SEG

;**    RealmodeInit - Realmode initialization routine
;
;
; ENTRY    cs=ds=es = VxD_REAL_INIT_SEG
;    si = environment segment
;    ax = VMM version number
;    bx = flags
;    edx = reference data from Int2F response or 0
;
; EXIT    ax = return code
;    bx = 0 or pointer to list of pages to exclude
;    si = 0 or pointer to list of instance data items
;    edx = reference data
;
; USES    all
;
BeginProc RealmodeInit

    xor    bx, bx
    xor    si, si
    mov    ax, Device_Load_Ok
    ret

EndProc RealmodeInit


VxD_REAL_INIT_ENDS


    end    RealmodeInit        ; point at the real mode init routine
