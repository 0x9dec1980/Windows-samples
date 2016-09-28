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
    title    VDEFDDA
;===========================================================================
;
;   Module:   VDEFDDA - Contains the default FSDs int 25/26 functions
;                     - Calls requests down to IOS
;
;===========================================================================

;===========================================================================
;                  P U B L I C S
;===========================================================================

;===========================================================================
;                  I N C L U D E S
;===========================================================================

.xlist        
    include vmm.inc
    include iostk.inc

MASM=1
    include ifs.inc
    include opttest.inc
    include blockdev.inc
    include ior.inc
    include chain.inc
    include resource.inc
    include ilb.inc
    include debug.inc
    include vrp.inc
    include ios.inc
    include error.inc
    
.list    

;===========================================================================
;                  E X T E R N S
;===========================================================================

VxD_LOCKED_DATA_SEG

    extrn    VDEF_ilb:dword             
    extrn    ResChain:dword

VxD_LOCKED_DATA_ENDS

VxD_LOCKED_CODE_SEG

    extrn    CallStrat2:near
    extrn    GetIOP:near
    extrn    RetIOP:near

;==========================================================================
;
;    Procedure Name    : AbsReadWrite
;
;    Input        : ioreq on stack with the foll. 
;                    
;               ir_rh    = handle to the volume resource
;               ir_flags = 0 for int 25h (read)
;                    = 1 for int 26h (write)
;               ir_pos   = sector number to start operation at
;               ir_length= number of sectors to transfer
;               ir_data  = address of data buffer
;
;
;    Output        : ir_error contains errorcode if error
;
;    Uses        : eax, ecx, edx, Flags
;
;=========================================================================

BeginProc    AbsReadWrite

    push    ebp
;    mov    ebp, [esp+4]
    mov    ebp, [esp+8]
    push    ebx
    push    esi
    push    edi

    xor    ax, ax
    mov    [ebp].ir_error, ax
    cmp    [ebp].ir_flags, DIO_SET_LOCK_CACHE_STATE
    je    arw30
    
    mov    edi, [ebp].ir_rh    ; (edi) = Resource Tab
    
    call    MemPgLock
    call    GetIOP

    ;
    ; (ebx) = IOR
    ; (edi) = ResTab
    ;
    ; Init the relevant fields in the IOR
    ;

    mov    eax, [ebp].ir_length
    mov    [ebx].IOR_xfer_count, eax

    mov    eax, [ebp].ir_pos
    mov    [ebx].IOR_start_addr, eax

    mov    eax, [ebp].ir_data
    mov    [ebx].IOR_buffer_ptr, eax

    call    SatisfyDevCrit

    SetFlag    [ebx].IOR_flags, IORF_SYNC_COMMAND
    
    cmp    [ebp].ir_flags, DIO_ABS_READ_SECTORS
                    ; Q: is this a read
    je    arw10            ; Y: continue. Note that the IOR_func
                    ;    is set to IOR_READ by default
                    ;    in getIOP

ifdef    DEBUG

    cmp    [ebp].ir_flags, DIO_ABS_WRITE_SECTORS
    DEBUG_OUTnz    "vdef: Bad Parms to AbsReadWrite"
    mov    ax, ERROR_INVALID_PARAMETER
    jnz    arw20

endif    

    mov    [ebx].IOR_func, IOR_WRITE
    mov    eax, ebx        ; (eax) = IOR - input to CallStrat2
arw10:    call    CallStrat2

    xor    ax, ax            ; assume success
    movzx    ecx, [ebx].IOR_status
    cmp    ecx, IORS_ERROR_DESIGNTR
                    ; Q: any errors
    ja    arw100            ; Y: map the error

arw20:    mov    [ebp].ir_error, ax

    call    RetIOP
    call    MemPgUnlock

arw30:    pop    edi
    pop    esi
    pop    ebx
    pop    ebp
    ret

    ;
    ; (ecx) = error from IOS
    ; (ebx) = IOR
    ; 

arw100:    
    mov    eax, ecx
    movzx    ecx, [ebx].IOR_func
    VxdCall    IOSMapIORSToI24
    jmp    short arw20

EndProc        AbsReadWrite

;===========================================================================
;
;    Procedure Name    : MemPgLock
;
;    Input        : (ebp) = ioreq structure
;                it's a read or write request, so
;                     ir_data - ptr to data buffer
;                     ir_length - length of area to lock
;              (edi) = ResTab
;
;
;    Ouput        : Locks the user buffer pages 
;
;    Uses        : ALL except ebp
;
;===========================================================================

BeginProc    MemPgLock

    mov    eax,[ebp].ir_length    ; (eax) = secs to transfer

    mov    esi, [edi].Res_Vrp
    mul    [esi].VRP_block_size    ; (eax) = bytes to transfer

    mov    edx,PAGEMARKDIRTY    ; Assume read operation

    cmp    [ebp].ir_flags, 0    ; Q: is this a read
    je    mpl10            ; Y: continue

    xor    edx, edx        ; N: write op, no need to dirty pages

mpl10:    or    edx,PAGEMAPGLOBAL    ; Need global alias

    mov    esi,[ebp].ir_data    ; (esi) = address to lock
    add    eax,esi
    dec    eax            ; (eax) = address of last byte
    shr    esi,12            ; (esi) = page holding 1st byte
    shr    eax,12            ; (eax) = page holding last byte
    sub    eax,esi
    inc    eax            ; (eax) = # of pages to lock

    mov    [ebp].ir_fsd.df_PLCnt,eax
    
    VMMcall    _LinPageLock, <esi, eax, edx>

    and    eax,eax
    jz    short mpl90        ; the lock failed

    mov    edx,[ebp].ir_data    ; (edx) = caller's non-global address
    and    edx,PAGESIZE - 1    ; (edx) = page offset
    or    eax,edx            ; (eax) = global address
    mov    [ebp].ir_data,eax    ; Pull the ol' switcheroo
    shr    eax,12            ; (eax) = logical page number
    mov    [ebp].ir_fsd.df_PLNum,eax 
                    ; Save the page number

    SetFlag    [ebp].ir_fsd.df_Flag,DFLG_MPLOCK        
                    ; mark mem pages locked
    ret

mpl90:    SetFlag    [ebp].ir_fsd.df_Flag,DFLG_DBLBFR
    ret

EndProc        MemPgLock

;==========================================================================
;
;    Procedure Name    : MemPgUnlock
;
;    Input        : (ebp) = ioreq struc
;
;    Output        : Unlocks user buffer if locked
;
;    Uses        : ALL except ebp
;
;==========================================================================

BeginProc    MemPgUnlock

    TestMem    [ebp].ir_fsd.df_Flag,DFLG_MPLOCK
    jz    short mpux        ; nothing locked

    mov    esi,[ebp].ir_fsd.df_PLNum
    mov    eax,[ebp].ir_fsd.df_PLCnt
    VMMcall    _LinPageUnLock, <esi, eax, PAGEMAPGLOBAL>

ifdef     DEBUG

    or    eax, eax
    TRAPz

endif
    xor    [ebp].ir_fsd.df_Flag,DFLG_MPLOCK    ; free lock mark
mpux:    ret

EndProc        MemPgUnlock

;==========================================================================
;
;    Routine Name    : SatisfyDevCrit
;
;    ENTRY        : (edi) = Resource table (RESTAB)
;            : (ebx) = ptr to allocated IOR
;              (ebp) = IOREQ
;
;    EXIT        : IOR's SGDs are filled in according to the demand
;              flags specifed in the VRP
;
;    USES        : ALL except ebp, edi, ebx
;
;==========================================================================

BeginProc    SatisfyDevCrit

    TestMem    [ebp].ir_fsd.df_Flag,DFLG_DBLBFR
                    ; Q: did we fail to lock user buffer
    jnz    sdc10            ; Y: do ios dbl buffer

    push    edi
    push    ebx

    mov    eax, [edi].Res_Vrp    ; (eax) = pointer to VRP        
    mov    [ebx].IOR_req_vol_handle, eax
    push    ebx
    call    [VDEF_ilb].ILB_io_criteria_rtn
    or    eax, eax
    pop    eax
    pop    ebx
    pop    edi
    jnz    sdc10
    ret

sdc10:    SetFlag    [ebx].IOR_flags, IORF_DOUBLE_BUFFER
    ret    

EndProc    SatisfyDevCrit

ifdef    TESTING

;============================================================================
;
;    Procedure Name    : DoAbsRW
;
;    This procedure handles Absolute Disk Read requests
;    from real mode.
;
;    ENTRY    (EBP) = Client_Regs pointer
;        (Client_AL) = drive (0 = A, 1 = B, etc.)
;        (Client_BX) = buffer or DISKIO offset
;        (Client_DS) = buffer or DISKIO segment
;        (Client_CX) = number of sectors to read or 0FFFFh
;        (Client_DX) = first sector to read or ignored
;        (eax) = 0 for read 1 for write
;    EXIT    (Client_Flags) carry set
;            (Client_AX) = error code
;        Flags not popped from Client stack!
;
;    If (Client_CX) = 0FFFFh, then the client's (DS:BX) points
;    to a DISKIO structure which contains the parameters (excluding
;    the drive) for the request.
;
;    Note that INT 25h does not pop the client flags off the
;    client's stack.
;
;============================================================================

DISKIO    STRUC
    diStartSector    dd    ?    ; First sector to read/write
    diSectors        dw    ?    ; Number of sectors to transfer
    diBuffer        dd    ?    ; Pointer to client's buffer
DISKIO    ENDS

BeginProc    DoAbsRW

    push    eax

    Client_Ptr_Flat ebx, DS, BX    ; (EBX) = buffer address
    movzx    edx,[ebp].Client_DX    ; (EDX) = starting sector (maybe)
    movzx    eax,[ebp].Client_AL    ; (EAX) = drive number
    movzx    ecx,[ebp].Client_CX    ; (ECX) = sector count
    cmp    cx,0FFFFh        ; User passed in DISKIO structure?
    jne    short ri25a        ;  no, skip ahead
    mov    edx,[ebx].diStartSector    ; (EDX) = starting sector
    movzx    ecx,[ebx].diSectors    ; (ECX) = sector count
    mov    edi,[ebx].diBuffer    ; (EDI) = 16:16 buffer address
    VMMcall    Get_Cur_VM_Handle    ; (EBX) = VM Handle
    shld    esi,edi,20        ; (ESI) = segment*16 and garbage
    and    esi,0FFFF0h        ; (ESI) = segment*16
    and    edi,0FFFFh        ; (EDI) = offset
    add    esi,edi            ; (ESI) = 20-bit address
    add    esi,[ebx].CB_High_Linear; (ESI) = high linear address
    mov    ebx,esi            ; (EBX) = buffer address

;    We have our parameters:
;    (EAX) = drive number
;    (EBX) = buffer address
;    (ECX) = count of sectors
;    (EDX) = starting sector
;    Now map the drive to a ResTab.

ri25a:    mov    edi,OFFSET32 ResChain    ; (EDI) = head of Resource chain
ri25b:    mov    edi,[edi].Res_Chain.FWD    ; (EDI) = next ResTab
    cmp    edi,OFFSET32 ResChain    ; End of chain?
    je    short ri25e        ;  yes, error
    cmp    [edi].Res_Drive,al    ; Right resource found?
    jne    short ri25b        ;  no, keep looking

;    We have our volume:
;    (EBX) = buffer address
;    (ECX) = count of sectors
;    (EDX) = starting sector
;    (EDI) = ResTab pointer
;    Now build our ioreq packet.

    pop    eax

    sub    esp,SIZE ioreq        ; Make room on stack
    .errnz    (SIZE ioreq AND 3)    ; Must be dword-aligned
    mov    esi,esp            ; (DS:ESI) = ioreq pointer
    push    eax
    push    ecx            ; Push number of sectors
    push    edi            ; Push VolTab pointer
    cld
    mov    edi,esi            ; (ES:EDI) = ioreq pointer
    xor    eax,eax            ; (EAX) = 0
    mov    ecx,SIZE ioreq / 4    ; (ECX) = count of dwords in packet
    rep stosd            ; Zero the packet
    pop    [esi].ir_rh        ; Set VolTab pointer
    pop    [esi].ir_length        ; Set number of sectors
    pop    eax
    mov    [esi].ir_flags, al
    mov    [esi].ir_pos,edx    ; Set starting sector
    mov    [esi].ir_data,ebx    ; Set buffer address
    push    esi
    call    AbsReadWrite
    pop    esi
    movzx    eax,[esi].ir_error    ; (EAX) = error code
    add    esp,SIZE ioreq        ; Punt the ioreq packet
    and    [ebp].Client_Flags,NOT 1; Assume no error (BUGBUG: equate???)
    or    eax,eax            ; Any error?
    jz    short ri25d        ;  no, done
ri25c:    mov    [ebp].Client_AX,ax    ; Put code in client register
    or    [ebp].Client_Flags,1    ; Set client carry (BUGBUG: equate???)
ri25d:    mov    eax,[ebp].Client_EFlags    ; (EAX) = client's flags
    VMMcall    Simulate_Push        ; Put copy on stack
    clc                ; Accept the interrupt
    ret

;    We're not handling this drive.  Chain on to DOS.

ri25e:    pop    eax
    stc                ; Didn't accept the interrupt
    ret

EndProc    DoAbsRW


BeginProc    TestInt25

    TRAP
    xor    eax, eax
    jmp    DoAbsRW

EndProc        TestInt25

BeginProc    TestInt26

    TRAP
    mov    eax, 1
    jmp    DoAbsRW

EndProc        TestInt26

ENDIF    ; TESTING


VxD_LOCKED_CODE_ENDS

    end
