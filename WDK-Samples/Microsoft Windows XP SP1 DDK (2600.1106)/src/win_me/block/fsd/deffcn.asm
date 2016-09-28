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

    TITLE    DEFFS - Default File System Routines
    NAME    DEFFS

    .386

    page    ,132

;**    Def Filesystem Routines

    .xlist
    .xcref
MASM    =    1        ; indicate MASM environment for includes
    include vmm.inc
    include error.inc

    Include ifs.inc
    include chain.inc
    Include debug.inc
    Include macro.inc
    include resource.inc
    .cref
    .list

    ASSUMPTIONS



    VxD_LOCKED_CODE_SEG

    EXTRN   PassIoctl:near
    EXTRN    AbsReadWrite:near
    EXTRN    UnMountDef:near

;*    Entry point Table
;
;    this is an entry point table used so that the redir
;    interface can find the FlatFat entry points, since they're loaded
;    as seperate packages.

    Public    DefEntry
DefEntry label    dword

    .errnz    vfn_version
    dw    IFS_VERSION
    .errnz    vfn_revision - vfn_version - 2
    db    IFS_REVISION
    .errnz    vfn_size - vfn_revision - 1
    db    NUM_VOLFUNC

    .errnz    VFN_DELETE
fatea    dd    OFFS DoFail
    .errnz    VFN_DIR - VFN_DELETE - 1
    dd    OFFS DoFail
    .errnz    VFN_FILEATTRIB - VFN_DIR - 1
    dd    OFFS DoFail
    .errnz    VFN_FLUSH - VFN_FILEATTRIB - 1
    dd    OFFS DoSucceed
    .errnz    VFN_GETDISKINFO - VFN_FLUSH - 1
    dd    OFFS DoFail
    .errnz    VFN_OPEN - VFN_GETDISKINFO - 1
    dd    OFFS DoFail
    .errnz    VFN_RENAME - VFN_OPEN - 1
    dd    OFFS DoFail
    .errnz    VFN_SEARCH - VFN_RENAME - 1
    dd    OFFS DoFail
    .errnz    VFN_QUERY - VFN_SEARCH - 1
    dd    OFFS DoFail
    .errnz    VFN_DISCONNECT - VFN_QUERY - 1
    dd    OFFS UnMountDef
    .errnz    VFN_UNCPIPEREQ - VFN_DISCONNECT - 1
    dd    OFFS DoFail

    .errnz  VFN_IOCTL16DRIVE - VFN_UNCPIPEREQ - 1
    dd      OFFS PassIoctl

    .errnz  VFN_GETDISKPARMS - VFN_IOCTL16DRIVE - 1
    dd      OFFS DoFail

    .errnz  VFN_FINDOPEN - VFN_GETDISKPARMS - 1
    dd      OFFS DoFail

    .errnz  VFN_DASDIO - VFN_FINDOPEN - 1
    dd    OFFS AbsReadWrite

    .errnz    ($-fatea)/4 - NUM_VOLFUNC

;        entry:
;                stack frame contains ioreq struct
;        uses:
;                eax
;        exit:
;                ir_error is set

DOFAILFRAME STRUC
    Dfsavregs       dd      2       dup     (?)
    Dfretaddr    dd    ?    
        DfIOREQ        dd      ?
DOFAILFRAME ENDS


BeginProc DoFail
    push    ebp
    push    edi
    mov    ebp,DfIOREQ[esp]    ; (ebp) = ioreq strucure

    mov    edi, [ebp].ir_rh    ; (edi) = restab
    movzx    eax, [edi].Res_ErrCode
    cmp    eax, ERROR_NOT_READY
    je    short df10

    cmp    eax, ERROR_VOLUME_LOCKED
    je    short df10

    mov     eax,ERROR_SHARING_VIOLATION;set failure code
    cmp     [edi].Res_Lock,DFS_RESOURCE_LOCKED ; are we locked?
    je      df10            ;yes set sharing violation

    ;if it is not a not ready or sharing violation, it is a general
    ;failure
    mov     eax,ERROR_GEN_FAILURE


df10:    mov     [ebp].ir_error,ax
    pop     edi
    pop     ebp
    ret
endproc DoFail

;*  DoSucceed
;
;   entry:
;           stack frame contains ioreq struct
;   uses:
;   exit:     ir_error is clear
;

DOSUCCEEDFRAME STRUC

    DSsavregs   dd      ?
    DSretaddr   dd      ?    
    DSIOREQ     dd      ?

DOSUCCEEDFRAME ENDS

BeginProc    DoSucceed

    push    ebp
    mov     ebp, DSIOREQ[esp]        ; (ebp) = ioreq strucure
    xor     eax, eax
    mov     [ebp].ir_error, ax
    pop     ebp
    ret
EndProc        DoSucceed

    VxD_LOCKED_CODE_ENDS
    END
