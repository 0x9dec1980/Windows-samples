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

.386p

;===========================================================================
    title    VDEFIOS
;===========================================================================
;
;   Module:   VDEFIOS - Allocates and deallocates IOPs for the FS
;                     - Calls requests down to IOS
;
;===========================================================================

;===========================================================================
;                  I N C L U D E S
;===========================================================================

    .xlist
MASM    = 1
    include vmm.inc
    include sgd.inc
    include ilb.inc
    include blockdev.inc
    include ior.inc
    include vrp.inc
    include isp.inc
    include ios.inc
    include chain.inc
    include debug.inc
    include resource.inc
    include macro.inc
    include opttest.inc
    .list

;===========================================================================
;                  P U B L I C S
;===========================================================================



    VxD_LOCKED_DATA_SEG

    extrn     VDEF_ilb:dword
    VxD_LOCKED_DATA_ENDS

    VxD_LOCKED_CODE_SEG
    ASSUMPTIONS

;
;###########################################################################
;
;    Routine Name    : GetIOP
;
;    ENTRY        : (edi) = volume pointer
;    
;    EXIT        : (ebx) = ptr to allocated IOR
;
;    Description:
;        Obtains the pointer to the VRP(Vloume Request Packet) from
;    the RESTAB (edi). Determines the size VRP_max_req_size and allocates
;    an IOP. Then initializes the IOR fileds in the IOP and passes a
;    pointer to the IOR back to the caller.
;
;    USES    eax, ebx, esi
;
;############################################################################
;
        
    align    4
BeginProc GetIOP

    pushfd

    mov    esi, [edi].Res_Vrp            ; esi = pointer to VRP    
    mov    bx, [esi].VRP_max_req_size    ; ebx = size to allocate
    mov    ax, [esi].VRP_max_sgd
    .errnz    size SGD - 8
    shl    ax, 3
    add    bx, ax
    mov    eax, [esi].VRP_delta_to_ior
    
     sub    esp,size ISP_IOP_create        ; allocate isp from the stack.

    mov    esi,esp                ; point to the gotten isp.

    mov    [esi.ISP_IOP_size], bx
    mov    [esi.ISP_delta_to_ior], eax
    mov    [esi.ISP_i_c_flags], ISP_M_FL_MUST_SUCCEED
    mov    [esi.ISP_func],ISP_CREATE_IOP    ; create
    push    esi                ; specified size.
    call    [VDEF_ilb].ILB_service_rtn      ;

    mov    ebx,[esi].ISP_IOP_ptr        ; point to the created rlh.

    add    esp,size ISP_IOP_create+4     ; cleanup the stack and
                         ; discard the isp.

                                                ; eax is delta to IOR
                                                ; ebx is IOP ptr

    add    ebx, eax            ; ebx = lin ptr to IOR

                        ; save delta to IOR in
                        ; reserved_client
    mov    [ebx].IOR_private_client, eax

    ;
    ; Now fill in the default linear pointer to the sgd in the IOR.
    ;

    mov    eax, ebx
    add    eax, size IOR
    mov    [ebx].IOR_sgd_lin_phys, eax
    
; Set initial values in IOR

    sub    eax,eax
    .errnz    size IOR_func-2
    .errnz    size IOR_Status-2
    .errnz    IOR_Status-IOR_func-2        ; 2 words  make dword
    mov    [ebx].IOR_func,ax
    mov    [ebx].IOR_start_addr+4,eax
    mov    [ebx].IOR_xfer_count,eax
    mov    [ebx].IOR_num_sgds, al
    mov    [ebx].IOR_Flags, IORF_VERSION_002 OR IORF_HIGH_PRIORITY OR IORF_BYPASS_VOLTRK


    mov    eax, [edi].Res_Vrp        ; eax = pointer to VRP    

    popfd                ; to reduce duplicated code in callers?
    clc
    ret

endproc GetIOP

;###########################################################################
;
;    Routine Name    : RetIOP
;
;    ENTRY        : ebx = pointer to previously allocated IOR
;
;    EXIT        : deallocates the memory.
;
;###########################################################################

BeginProc RetIOP

    pushfd
    sub    ebx, [ebx].IOR_private_client    ; get ptr to IOP
    sub     esp, size ISP_mem_dealloc    ;
    mov    esi, esp             ; release
        mov     [esi].ISP_mem_ptr_da, ebx    ;
    mov    [esi].ISP_func, ISP_dealloc_mem    ; IOP
    push    esi                ;
    call    [VDEF_ilb].ILB_service_rtn      ;

    add    esp, size ISP_mem_dealloc + 4    ; clean up the stack.
    popfd
    ret
    
EndProc RetIOP


;###########################################################################
;
;    Routine Name    : CallStrat2
;
;       Function:       : CallStrat2 fills in a few common parameters
;                         for the IOR and calls it down to IOS
;
;    ENTRY        : eax = IOR
;                         edi = RESTAB
;
;    EXIT        : Yields CPU
;
;       USES            : all but ebp
;
;###########################################################################


beginproc CallStrat2

    push    ebp
    mov     esi, eax                ;save the IOR

    mov     al, Res_Drive[edi]      ;use the drive from the Restab
    mov     [esi].IOR_vol_designtr, al

    mov     edi,Res_Vrp[edi]        ; (edi) = vrp

    SetFlag    [esi].IOR_flags, IORF_LOGICAL_START_SECTOR
                    ; assume removable media

    TestMem    [edi].VRP_demand_flags, VRP_dmd_removable_supp
                    ; Q: is this removable media
    jz    cs10            ; N: do not add partition offset

    ClrFlag    [esi].IOR_flags, IORF_LOGICAL_START_SECTOR

    mov    eax, [edi].Vrp_Partition_Offset
    add    [esi].IOR_start_addr, eax
    
cs10:    mov     [esi].IOR_req_vol_Handle, edi   ;hkn insert handle = vrp

    mov    [esi].IOR_next, 0    ; clear the next field
    
    mov    edi, [edi].VRP_device_handle

        ;call the IOR down
    VxDcall IOS_SendCommand

    cld
    pop     ebp
    ret

endproc CallStrat2


    VxD_LOCKED_CODE_ENDS

    end
