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

;===========================================================================
    page    ,132
    title    iostk - vdef per io strucs and equates
    name    iostk
;===========================================================================
;
;   Module:   contains the struc defs and equates for the FSD local region
;          provided by IFSMGR on a per request basis
;
;===========================================================================


AbsRW    struc

  df_PLNum      DD  ?    ; page # of 1st logical page locked
  df_PLCnt      DD  ?    ; count of logical pages locked
  df_Flag       DD  ?    ; flags, see below for definitions

AbsRW    ends

DFLG_MPLOCK     EQU    00000001B    ; mem page locks in effect
DFLG_DBLBFR     EQU    00000010B    ; double buffer the request


; error codes to be returned in (al) on int 25/26

NO_RESPONSE     EQU    080h
SEEK_FAILED     EQU    040h
CNTRLR_FAILED   EQU    020h
DATA_ERROR      EQU    010h
DMA_FAILURE     EQU    008h
SEC_NOT_FOUND   EQU    004h
BAD_ADDRESS     EQU    002h
BAD_COMMAND     EQU    001h
