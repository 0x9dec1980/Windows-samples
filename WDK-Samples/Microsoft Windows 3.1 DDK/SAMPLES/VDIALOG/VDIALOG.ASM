PAGE 58,132
;******************************************************************************
TITLE VDIALOG - 
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1990
;
;   Title:      VDIALOG.ASM - 
;
;   Version:    3.00
;
;   Date:       8-Oct-1990 
;
;   Author:     Neil Sandlin
;
;
;       The purpose of this sample VxD is to demonstrate the basic
;       use of the Install_IO_Handler and Shell_Resolve_Contention
;       function calls. The Install_IO_Handler call enables this
;       VxD to trap IN and OUT's to a port from any VM. The
;       Shell_Resolve_Contention call is then used to display a
;       dialog box when another VM attempts to use a port that
;       is already assigned.
;
;       This VxD also demonstrates the use of Enable_Local_Trapping
;       and Disable_Local_Trapping to allow the owner of the port in
;       question to freely do I/O to the port without overhead.
;
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
        INCLUDE Shell.inc
        INCLUDE Vdevice.inc
        .LIST

;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VDIALOG, 3, 0, VDIALOG_Control, Undefined_Device_ID ,,,


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************


VxD_LOCKED_DATA_SEG

Device_Name db  "VDIALOG ",0
VDIALOG_Owner     dd  ?


VxD_LOCKED_DATA_ENDS




;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
;   VDIALOG_Device_Init
;
;   DESCRIPTION:
;
;       This routine is called during Windows startup. It needs to 
;       install the I/O handler for our device, and set up the system
;       VM as the default owner.
;
;
;==============================================================================

BeginProc VDIALOG_Device_Init

        mov     edx, vdev_addr
        mov     esi, OFFSET32 My_VDIALOG_Hook
        VMMCall Install_IO_Handler

        xor     eax, eax
        mov     VDIALOG_Owner, eax              ; no current owner
        
IFDEF DEBUG
        Trace_Out "VDIALOG installed"
ENDIF

        clc
        ret

EndProc VDIALOG_Device_Init

VxD_ICODE_ENDS


VxD_CODE_SEG

;******************************************************************************
;
;   VDIALOG_Destroy_VM
;
;   DESCRIPTION:
;
;       This routine is called when a VM is destroyed. We need to check
;       to see if the VM in question is the current owner of the device.
;
;==============================================================================
BeginProc VDIALOG_Destroy_VM

        cmp     ebx, VDIALOG_Owner              ; Destroying device owner?
        jnz     short VDM_Exit

        xor     eax, eax
        mov     VDIALOG_Owner, eax              ; no current owner
        
VDM_Exit:
        clc
        ret

EndProc VDIALOG_Destroy_VM

VxD_CODE_ENDS


;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VDIALOG_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation.
;
;==============================================================================
BeginProc VDIALOG_Control

        Control_Dispatch Device_Init, VDIALOG_Device_Init
        Control_Dispatch Destroy_VM, VDIALOG_Destroy_VM
        clc
        ret

EndProc VDIALOG_Control

VxD_LOCKED_CODE_ENDS


VxD_CODE_SEG

;*****************************************************************************
;
;   My_VDIALOG_Hook
;
;   DESCRIPTION:
;
;       This routine is called for every I/O access to our port. First,
;       it checks to see if the VM issuing the I/O is the current owner.
;       If not, it displays a dialog box so the user can decide what to
;       do.
;
;*****************************************************************************

BeginProc My_VDIALOG_Hook

;---------------------------------------------------------------------
;  Resolve Contention 
;---------------------------------------------------------------------
        pushad                                  ; save regs
        mov     eax, VDIALOG_Owner              ; get previous owner
        cmp     eax, ebx                        ; same as current owner?
        jz      short process_io                ; yes, just handle it
        or      eax, eax                        ; was there an old owner?
        jz      short new_owner                 ; no

        mov     esi, OFFSET32 Device_Name
        VxDCall Shell_Resolve_Contention

        jc      short dont_process              ; hmmm, couldn't resolve

        cmp     ebx, VDIALOG_Owner              ; if contention winner is
                                                ; the current owner,
        je      short dont_process              ; then we shouldn't process

new_owner:

IFDEF DEBUG
        Trace_Out "VDIALOG: New Owner #EBX"
ENDIF

        mov     edx, vdev_addr                  ; our port address 
        VMMCall Disable_Local_Trapping          ; give winner free access        

        xchg    ebx, VDIALOG_Owner              ; save new owner, get old
        or      ebx, ebx                        ; no old owner?
        jz      short process_io                ; no, just process
        VMMCall Enable_Local_Trapping           ; old owner now locked out

process_io:
        popad

;---------------------------------------------------------------------
;  Handle I/O 
;---------------------------------------------------------------------
        Dispatch_Byte_IO Fall_Through, <SHORT VDIALOG_Out>
        in      al,dx                           ; do real in
        ret

VDIALOG_Out:
        out     dx,al                           ; do real out
        ret

dont_process:
        popad                                   ; restore regs
        mov     al, 0Eh                         ; indicate error to sample
                                                ; apps
IFDEF DEBUG
        Trace_Out "VDIALOG: I/O inhibited for VM #EBX"
ENDIF

        ret     

EndProc My_VDIALOG_Hook


VxD_CODE_ENDS


        END
