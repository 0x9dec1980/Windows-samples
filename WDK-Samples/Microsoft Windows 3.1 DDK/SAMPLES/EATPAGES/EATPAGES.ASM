PAGE 58,132
;******************************************************************************
TITLE EATPAGES - Steals Free pages
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1991
;
;   Title:      EATPAGES.ASM - Steals Free pages
;
;   Version:    3.00
;
;   Date:       1-Jun-1991 
;
;   Author:     Neil Sandlin
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
        .LIST

pageitem        STRUC
pagehandle      dd      ?
pageitem        ENDS


;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device EATPAGES, 3, 0, EATPAGES_Control, Undefined_Device_ID ,,,



;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_IDATA_SEG

pageinfo        DemandInfoStruc <0>
numpages        dd      0

VxD_IDATA_ENDS


VxD_DATA_SEG

pagelist        dd      ?

VxD_DATA_ENDS


;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
;   EATPAGES_Init_Complete
;
;   DESCRIPTION:
;       This routine is called during system boot. The sole function
;       is to find out how many free pages there are in the system,
;       allocate half of them, and keep track of the allocations using
;       the linked list services. 
;
;
;==============================================================================

BeginProc EATPAGES_Init_Complete


;----------------------------------------------------------------------------
;       Create linked list
;----------------------------------------------------------------------------
        mov     eax, LF_Alloc_Error             ;handle out of memory
        mov     ecx, SIZE PageItem              ;ECX = size of each list node
        VMMCall List_Create
        mov     [PageList], esi                 ;save list handle

;----------------------------------------------------------------------------
;       Get free page information
;----------------------------------------------------------------------------
        xor     eax, eax
        mov     esi, OFFSET32 pageinfo
        VMMCall _GetDemandPageInfo,<esi, eax>

        mov     ecx, [esi.DIFree_Count]         ;number of free pages
        shr     ecx, 1                          ;divide by two
        mov     esi, [pagelist]                 ;point to linked list           

;----------------------------------------------------------------------------
;       Allocate half of all free pages
;----------------------------------------------------------------------------
di_loop:
        push    ecx                             ;pageallocate uses ecx
        VMMCall _PageAllocate,<1,PG_SYS,0,0,0,-1,0,PageLocked>
        pop     ecx                             ;restore
        or      eax, eax                        ;failed?
        jz      short di_next                   ;ok, done
        mov     edi, eax                        ;save handle

        VMMCall List_Allocate                   ;get new node in eax
        jc      short di_listerr                ;woops, ran out of memory
        VMMCall List_Attach_Tail                ;put it at the end

        inc     [numpages]                      ;another stolen
        mov     [eax.pagehandle], edi           ;save memory handle
        loop    di_loop                         ;and again
        jmp     short di_next

di_listerr:                                     ;out of list memory
        mov     eax, edi                        ;reload handle
        VMMCall _PageFree,<eax,0>               ;and free page

di_next:
        
        
IFDEF DEBUG
        mov     ecx, [numpages]
        Trace_Out "EATPAGES installed - #ECX pages stolen"
ENDIF
        clc
        ret

EndProc EATPAGES_Init_Complete

VxD_ICODE_ENDS

VxD_CODE_SEG

;******************************************************************************
;
;   EATPAGES_System_Exit
;
;   DESCRIPTION:
;       This routine frees the pages that were allocated at Init_Complete.
;
;
;==============================================================================

BeginProc EATPAGES_System_Exit

;----------------------------------------------------------------------------
;       Free all allocated pages
;----------------------------------------------------------------------------
        mov     esi, [PageList]                 ;get list handle
        VMMCall List_Get_First
        jz      short chse_ex                   ;probably won't jump
        
se_loop:
        push    eax
        mov     eax, [eax.pagehandle]           ;get handle
        VMMCall _PageFree,<eax,0>
        pop     eax

        push    eax                             ;save current node pointer
        VMMCall List_Get_Next                   ;get next one
        mov     edx, eax                        ;save next pointer
        pop     eax                             ;restore current
        
        VMMCall List_Remove                     ;take current off list
        VMMCall List_Deallocate                 ;and get rid of it

        mov     eax, edx                        ;restore next pointer
        or      eax, eax                        ;last one?
        jnz     se_loop                         ;yes, all deallocated

chse_ex:
        clc
        ret

EndProc EATPAGES_System_Exit

VxD_CODE_ENDS


;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   EATPAGES_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's.
;
;
;==============================================================================

BeginProc EATPAGES_Control

        Control_Dispatch Init_Complete, EATPAGES_Init_Complete
        Control_Dispatch System_Exit, EATPAGES_System_Exit
        clc
        ret

EndProc EATPAGES_Control

VxD_LOCKED_CODE_ENDS

        END
