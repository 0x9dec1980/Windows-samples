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


    TITLE  DEFDATA - DATA SEGMENT for default file system
    NAME   DEFDATA
    .386

MASM = 1;
include vmm.inc
include macro.inc
include chain.inc
include resource.inc

DCXXX    struc
  XXFWD dd DCXXX PTR 0      ; forward pointer
  XXBAK dd DCXXX PTR 0      ; backward pointer
DCXXX    ends

VxD_LOCKED_DATA_SEG

    Public    ResChain
    .errnz    Res_Chain
ResChain DCXXX    <OFFS ResChain,OFFS ResChain>        ; list is empty now

    public    gdeltaIOR
gdeltaIOR   dd  ?

VxD_LOCKED_DATA_ENDS

END
