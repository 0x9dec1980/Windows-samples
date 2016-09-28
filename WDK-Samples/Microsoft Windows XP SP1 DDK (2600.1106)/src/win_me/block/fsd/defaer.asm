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

    page   ,132
    title  vDefAER - Dragon FlatFat virtual device async routines
    name   vDefAER
;
; this module contains the Pagefiles's asynchronous event routine
;

.386p

.xlist
MASM    =       1               ;indicate MASM environment
    include vmm.inc
    include aep.inc
    include drp.inc
    include ilb.inc
    include vdefinfo.inc
.list

VxD_IDATA_SEG

;Driver registration packet

    public     Drv_Reg_Pkt

Drv_Reg_Pkt  DRP {EyeCatcher, DRP_FSD,,,VDEFName,VDEFRev,VDEFFeature,VDEF_IF}

VxD_IDATA_ENDS

VXD_LOCKED_DATA_SEG

Public VDEF_ilb              ; define pagefile ilb
VDEF_ilb    ILB    <>      ;

VXD_LOCKED_DATA_ENDS

VXD_LOCKED_CODE_SEG

aer_table    label    dword

    dd    offset32 initialize        ; 0 initialize.
    dd    offset32 event_unknown        ; 1 reserved.
    dd    offset32 event_unknown        ; 2 boot complete.
    dd    offset32 config_device        ; 3 configure physical device.

aer_cmd_max    EQU    (($-aer_table)/4)-1


;************************************************************************
;                                                                       *
; VDEF_async_event - Asynchronous Event Handler                         *
;                                                                       *
;    PageAsyncRequest basically jump to the appropriate routine         *
;    indicated by the AER command in the Async Event Packet (AEP).      *
;                                                                       *
;    Entry:    Far pointer to AEP on stack                              *
;                                                                       *
;    Exit:    ebx->AEP                                                  *
;                                                                       *
;************************************************************************

AER_stack_frame struc

AER_return    dd    ?
AER_aep_offset    dd    ?

AER_stack_frame ends

        public    VDEF_async_event
VDEF_async_event    proc    near

    mov    ebx,[esp.AER_aep_offset]        ; pick up the aep ptr

    mov    [ebx.AEP_result],0        ; show no error yet.

    ret                    ; return to our caller.

unknown_event:

    call    event_unknown            ; process the error condition.

    ret                    ; return to our caller.

VDEF_async_event    endp

;************************************************************************
;                                                                       *
; event_unknown                                                         *
;                                                                       *
;    Entry:    BX -> Async Event Packet                                 *
;                                                                       *
;    Exit:                                                              *
;                                                                       *
;************************************************************************

event_unknown    proc    near

    mov    [ebx.AEP_result],-1        ; show that an error occured
    ret                    ; and return to our caller.

event_unknown    endp


;************************************************************************
;                                                                       *
; initialize the Pagefiler -  currently a no-op routine                 *
;                                                                       *
;    on entry ebx points to the aep                                     *
;                                                                       *
;************************************************************************

        public    initialize
initialize    proc    near
    ret
initialize    endp


;************************************************************************
;                                                                       *
; config_device                                                         *
;                                                                       *
;    Entered on AER from IOS with AEP. Get DCB ptr from AEP and         *
;                                                                       *
;    INPUT: EBX -> AEP                                                  *
;                                                                       *
;************************************************************************    

        public    config_device
config_device    proc    near


    ret                    ; return to our caller.

config_device    endp

;

VXD_LOCKED_CODE_ENDS

    end
