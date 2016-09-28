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

BELL	EQU	07H
LINE_FEED	EQU	0aH
CAR_RET	EQU	0dH
COM1_PORT	EQU	03f8H
COM2_PORT	EQU	02f8H
COM_DAT	EQU	00H
COM_IEN	EQU	01H
COM_IER	EQU	02H
COM_LCR	EQU	03H
COM_MCR	EQU	04H
COM_LSR	EQU	05H
COM_MSR	EQU	06H
COM_DLL	EQU	00H
COM_DLM	EQU	01H

w_s	STRUC
lobyte	DB	?
hibyte	DB	?
w_s	ENDS

dw_s	STRUC
loword	DW	?
hiword	DW	?
dw_s	ENDS

dw_ss	STRUC
lo	DW	?
hi	DW	?
dw_ss	ENDS

dd_s	STRUC
lodword	DD	?
hidword	DD	?
dd_s	ENDS

FarPtr	STRUC
offst16	DW	?
segmt	DW	?
FarPtr	ENDS

FarPtr32	STRUC
offst	DD	?
segmt32	DW	?
pad32	DW	?
FarPtr32	ENDS
