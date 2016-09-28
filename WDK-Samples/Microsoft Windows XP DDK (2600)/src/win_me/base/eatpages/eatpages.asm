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

PAGE 58,132
;******************************************************************************
TITLE EATPAGES - Steals Free pages
;******************************************************************************
;
;   Title:      EATPAGES.ASM - Steals Free pages
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

;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device EATPAGES, 3, 10, EATPAGES_Control, Undefined_Device_ID

;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_IDATA_SEG

pageinfo        DemandInfoStruc <0>
numpages        dd      0

VxD_IDATA_ENDS

VxD_DATA_SEG

PageListHead    dd      ?

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
;       allocate half of them, and keep track of the allocations by
;	linking them through each other, storing the head of the list
;	into PageListHead.
;
;==============================================================================

BeginProc EATPAGES_Init_Complete

;----------------------------------------------------------------------------
;       Get free page information
;----------------------------------------------------------------------------
	mov	esi, OFFSET32 pageinfo
        VMMCall _GetDemandPageInfo,<esi, 0>

	xor	edi, edi			; EDI = 0 (end of list)
        mov     esi, [esi.DIFree_Count]         ;number of free pages
        shr     esi, 1                          ;divide by two
	jz	di_next				; Wow, talk about low memory!

;----------------------------------------------------------------------------
;       Allocate half of all free pages
;----------------------------------------------------------------------------
di_loop:
        VMMCall _PageAllocate,<1,PG_SYS,0,0,0,-1,0,PageLocked>
        or      eax, eax                        ;failed?
        jz      short di_next                   ;ok, done
	mov	[eax], edi			; Hook into the list
	xchg	edi, eax			; New head of list
	inc	numpages			; Bookkeeping
	dec	esi				; Last page?
        jnz	di_loop                         ; N: Keep going

di_next:
	mov	PageListHead, edi		;Save list head
IFDEF DEBUG
	mov	ecx, numpages
        Trace_Out "EATPAGES installed - #ecx pages stolen"
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
        mov     ecx, PageListHead               ; Get head of list
        jecxz	chse_ex				; List is empty

se_loop:
	push	dword ptr [ecx]			; Save pointer to next page
        VMMCall _PageFree,<ecx,0>		; Free this page
        pop     ecx				; Recover pointer to next page
	or	ecx, ecx			; Finished?
        jnz     se_loop                         ; N: keep freeing

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

