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


    TITLE    DEFIOCTL - DEFAULT FILE SYSTEM IOCTL Routines
    NAME    DEFIOCTL
    .386

    page    ,132

        extrn   GetIOP:near
        extrn   RetIOP:near
        extrn   CallStrat2:near

MASM    =    1        ; indicate MASM environment for includes
    include vmm.inc
    include error.inc
    include sgd.inc
    include chain.inc
    include ifs.inc
    include blockdev.inc
    include ior.inc
    include resource.inc
    include macro.inc
    include debug.inc
    include opttest.inc


    .xlist
    .xcref

    ASSUMPTIONS
    VxD_LOCKED_CODE_SEG

;***********************************************************************
;   PassIoctl - Take an ioreq packet for an ioctl, and convert it
;               to and IOCTL IOP
;
;   entry:  ioreq on stack
;
;   exit: ioreq_Status set according to results
;
;   uses: eax
;
;***********************************************************************


BeginProc PassIoctl
    ;ioreq structure on stack frame, need restab pointer,
    ;and drive number for eject ioctl

    push    ebx
    push    ecx
    push    edx
    push    esi
    push    edi
    push    ebp

    mov     ebp,28[esp]        ; (ebp) = ioreq strucure
    push    ebp

    mov     edi,[ebp].ir_rh         ;get restable
    testmem [edi].Res_Lock,DFS_RESOURCE_LOCKED

    jnz     passioctl_check_lock

passioctl_do_ioctl:
    call    GetIOP                  ;ebx returns pointer to allocated IOR

    ;fill the IOR structure
    mov     [ebx].IOR_func,IOR_GEN_IOCTL
    or      [ebx].IOR_flags,IORF_SYNC_COMMAND OR IORF_16BIT_IOCTL

    mov     esi,edi         ;save restab
    mov     edi,ebx         ;save IOP pointer

    mov     ebx,[ebp].ir_aux2.aux_ptr
    push    ebx                             ;save client_pointer

    mov     [edi].IOR_ioctl_client_params,ebx

    mov     ax,[ebx].Client_BX
    mov     [edi].IOR_ioctl_drive,ax
    dec     al                      ;change to 0 based drive number
    mov     [edi].IOR_vol_designtr,al

    movzx   eax,[ebx].Client_CX
    mov     [edi].IOR_ioctl_control_param,eax

    testmem [ebp].ir_options, IOCTL_PKT_LINEAR_ADDRESS
                    ; look for 32 bit IOCTL
    jz      pioc10

    mov     eax, [ebp].ir_data
    mov     [edi].IOR_ioctl_buffer_ptr, eax

    clrflag [edi.IOR_flags],IORF_16BIT_IOCTL
    jmp     pioc15


pioc10:


    mov     ax, (Client_DS SHL 8) + Client_DX
    VMMcall Map_Flat
    mov     [edi].IOR_ioctl_buffer_ptr,eax

pioc15:

    mov     ax,[ebx].Client_AX
    mov     [edi].IOR_ioctl_function,ax

    mov     eax,[ebp].ir_rh
    mov     [edi].IOR_req_vol_handle,eax

    mov     eax,edi
    push    edi

    mov     edi,esi
    call    CallStrat2
    pop     edi

    ;BUGBUG edi should be unaffected by callstrat2, and should contain the
    ;IOR pointer, but somebody hoses him at the moment in IOS

    pop     ebx                     ;ebx contains client pointer

    pop     ebp                     ;ebp contains ioreq structure

    mov     eax,[edi].IOR_ioctl_return
    mov     [ebx].Client_AX,ax      ;return the ioctl status in ax

    cmp     [edi].IOR_status, IORS_ERROR_DESIGNTR
                    ; Q: error
    jae     pioc20            ; Y: continue
    xor     eax, eax        ; N: ax = success

pioc20: mov word ptr [ebp].ir_error,ax

    mov     ebx,edi
    call    RetIOP

passioctl_done:
    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebx
    ret


passioctl_check_lock:
    ;check if lock on RESOURCE is for this vm
    push    ebx
    VMMcall Get_Cur_VM_Handle

    cmp     ebx,[edi].Res_VMHandle
    pop     ebx

    je      passioctl_do_ioctl

    ;lock incorrect for this handle
    TRACE_OUT    "VDEF: failing invalid I/O for drive being formatted"
    ;set error codes
    pop    ebp    ; realign stack frame

    mov     word ptr [ebp].ir_error,ERROR_ACCESS_DENIED
    jmp     passioctl_done

EndProc PassIoctl

    VxD_LOCKED_CODE_ENDS
    end
