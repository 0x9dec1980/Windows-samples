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

    TITLE    FSMOUNT -- Mount and UnMount volumes
    page   ,132


    .386p
    .xlist

MASM6 EQU 1
MASM  EQU 1

    include vmm.inc
    include chain.inc       ; needed for resource.inc
    include resource.inc
    include error.inc
    include vrp.inc
    include macro.inc       ;needed for restab list management
    include ifs.inc         ;needed for ioreq structure definition
    include debug.inc       ;needed for the trap macros

    .list

    extrn    ResChain:dword
    extrn    gdeltaIOR:dword


    ASSUMPTIONS
    VxD_LOCKED_CODE_SEG

; code in DEFFCN.ASM
    extrn    DefEntry:near


;**    Mount_Def - Mount Default File System
;
;    Mount_Def is called by the network code each time there's a new
;    default device for us to mount.
;
;    The caller passes in an ioreq structure
;        ir_flags = 0 iff to mount
;        ir_flags = 1 iff to dismount
;        ir_rh = this will be the VRP for future reference
;        ir_aux1 = flat address of VRP
;
;    ENTRY    (TOS+4) = pointer to ioreq structure
;
;    EXIT    ir_error = eax
;        (eax) != 0 if error
;        (eax) = 0 if OK
;          if was mount:
;            ir_rh = pointer to provider specific information
;            ir_vfunc = flat address of entry point table
;
;    Uses    All except EBP

BeginProc Mount_Def

    push    ebx
    push    esi
    push    edi
    push    ebp

    mov    ebp,20[esp]            ; [ebp] = ioreq structure

    cmp    [ebp].ir_flags, IR_FSD_MOUNT_CHILD
    je    short mount10        ; am to mount child
    cmp    [ebp].ir_flags, IR_FSD_MOUNT
    jne    short mount20        ; am to dismount

mount10:

    call    mdfs            ; Perform actual mount
    mov    [ebp].ir_error,ax    ; set error response
    mov    dword ptr [ebp].ir_vfunc,OFFS DefEntry
    jmp    short mount30

;*    Dismount volume/device.

mount20:

    xor    eax, eax
    mov    [ebp].ir_error, ax

mount30:
    pop     ebp
    pop     edi
    pop     esi
    pop     ebx

    ret    

EndProc Mount_Def



;**    UnMountDef - unMount default file system
;
;    unMountDef is called by the network code when we want to stop using
;    the default device
;
;    The caller passes in an ioreq structure
;        ir_rh = this is the handle to free
;        ir_aux1 = flat address of VRP
;
;    ENTRY    (TOS+4) = pointer to ioreq structure
;
;    EXIT    ir_error = eax
;        eax = 0 
;
;    uses    eax, edi, esi, ecx

BeginProc UnMountDef, esp

    argvar    unmntIoreq, dword

    enterproc

    savereg    <ebp, edi, ebx>

    mov    ebp,unmntIoreq        ; [ebp] = ioreq structure

    mov    edi,[ebp].ir_rh
;    Now release the volume.

mnt30:    DCREM    Res_Chain,edi, ebx, ecx
    VMMCall    _HeapFree <edi, 0>
    xor    eax, eax
    mov    [ebp].ir_error,ax    ; set error return [nothing was done, though]

    restorereg    <ebx, edi, ebp>

    leaveproc

    return

EndProc UnMountDef


;**    mdfs - Mount File System
;
;
;    ENTRY      (ebp) = ioreq structure
;                  [ebp].ir_aux1 = VRP
;                  [ebp].ir_aux2 = drv id
;                  [ebp].ir_options = 1 if lock mount for this VM
;                     = 0 if plain mount.
;                  [ebp].ir_error = error code to be examined in dofail
;                    
;
;    EXIT    Yields CPU
;        (eax) = 0 iff no error
;          [ebp].ir_rh = RESTAB
;        (eax) = error code if error
;
;    USES    all


BeginProc  mdfs

    movzx    edi,[ebp].ir_options    ; (edi) = 1 if lock else 0
    mov    ebx, [ebp].ir_aux2.aux_ul ; (ebx) = drv_id

    mov    esi,OFFS ResChain      ; (esi) = first guy-1 in reschain
mdfs5:    mov    esi,Res_Chain.FWD[esi]    ; check next one in chain
    cmp    esi,OFFS ResChain
    je    short mdfs10        ; end of chain, this guy is new
    cmp    bl,[esi].Res_Drive
    jne    mdfs5            ; not a match

;
;    We've already mounted this resource.  Return an error and vrp in
;    ir_rh.

mdfs9:    mov    eax, edi
    mov    [esi].Res_Lock, al
    mov    ax, [ebp].ir_error
    mov    [esi].Res_ErrCode, ax
    mov    eax, [esi].Res_Vrp
    mov    [ebp].ir_rh, eax
    mov    eax, ERROR_VOLUME_EXISTS
    ret

;    We've don't have this resource mounted so we proceed.
;    First, allocate and setup a resource table as completely as we can.
;    We then add it to ResChain
;
;    (edi) = 1 if lock else 0
;    (ebx) = drv_id

mdfs10:    VMMCall    _HeapAllocate <size RESTAB, 0>
    or    eax, eax
    jz    mdfs100

    mov    ecx, edi
    mov    [eax].Res_Lock, cl

    mov    edi,[ebp].ir_aux1.aux_ptr ; (edi) = VRP

    ;
    ; set the delta for the RETIOP call
    ;
    mov    ecx, [edi].VRP_delta_to_ior
    mov    [gdeltaIOR], ecx

    ;
    ; set the mount/unmount entry point and the hvol field in the vrp .
    ;
    mov    [edi].VRP_fsd_entry, OFFS Mount_Def
    mov    [edi].VRP_fsd_hvol, eax

    ;
    ; initialize the remaining fields in the restab structure
    ;
    mov    [eax].Res_Vrp, edi
    mov    [eax].Res_Drive, bl
    VMMCall    Get_Cur_VM_Handle
    mov    [eax].Res_VMHandle, ebx
    mov    bx, [ebp].ir_error
    mov    [eax].Res_ErrCode, bx

    mov    [ebp].ir_rh,eax        ; set resource address for caller

;    Add to ResChain

    pushfd
    cli                ; volchain is walked at interrupt time
    mov    ebx,OFFS ResChain
    DCADDF    Res_Chain,ebx, eax, ecx
    popfd

    sub    eax,eax
    ret                ; done, no error

mdfs100:    TRAP                    ; hard error
    mov    eax,ERROR_ACCESS_DENIED
    ret

EndProc mdfs

    VxD_LOCKED_CODE_ENDS

    end
