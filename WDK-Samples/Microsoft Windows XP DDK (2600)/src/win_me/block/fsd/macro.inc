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

;**    DCADDF - Add Item to Front of List
;
;    DCADDF    offset,listreg,itemreg,scrreg
;
;        offset  = offset into structure of links to edit
;        listreg = address of list head node
;        itemreg = address of item to insert
;        scrreg  = scratch register to roach

DCADDF    MACRO    o,LR,IR,SR
    mov    SR,o.FWD[LR]
    mov    o.FWD[IR],SR
    mov    o.BAK[IR],LR
    mov    o.BAK[SR],IR
    mov    o.FWD[LR],IR
    ENDM


;**    DCREM - Remove Item from Double Link Chain
;
;    DCREM  offset,adrreg,scrreg1,scrreg2
;
;    If this is the only item in the chain then DCREM is a NOP.
;
;    offset  = offset into structure of links to edit
;    adrreg  = address of item to remove
;    scrreg? = two registers to scratch

DCREM    MACRO    o,ir,r2,r3
    mov    r2,o.FWD[ir]
    mov    r3,o.BAK[ir]
    mov    o.BAK[r2],r3
    mov    o.FWD[r3],r2
    ENDM


;**    DCMOVF - Move Item to the Front of the Chain
;
;    DCMOVF    offset,listreg,itemreg,scrreg,[scrreg2]
;
;        offset  = offset into structure of links to edit
;        listreg = address of list head node
;        itemreg = address of item to insert
;        scrreg  = scratch register to roach
;        scrreg2 = optional additional register to roach
;
;    CODEWORK - check users for supply of scratch registers

DCMOVF    MACRO    o,lr,ir,sr,sr2
  IFNB    <sr2>
    DCREM    o,ir,sr,sr2
   else
    push    lr
    DCREM    o,ir,lr,sr
    pop    lr
   endif
    DCADDF    o,lr,ir,sr
    ENDM


;**    DCMOVB - Move Item to the Back of the Chain
;
;    DCMOVB    offset,listreg,itemreg,scrreg
;
;        offset  = offset into structure of links to edit
;        listreg = address of list head node
;        itemreg = address of item to insert
;        scrreg  = scratch register to roach

DCMOVB    MACRO  o,lr,ir,sr
    push    lr
    DCREM    o,ir,lr,sr
    pop    lr
    DCADDB    o,lr,ir,sr
    ENDM


ASSUMPTIONS macro
     assume    cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT
    endm


OFFS    equ    <offset FLAT:>
