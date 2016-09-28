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

PortDDB	struct
DDB_header	db	size DDB dup (?) ; header - see ddb.inc

;
; port driver private fields go here
;

DDB_port_private	db 1 dup (?)	
DDB_irq_number		db ?
DDB_irq_handle		dd ?
DDB_port_flags		dd ?
DDB_base_ioa		dw ?

PortDDB	ends

;
; DDB_port_flags flags follow
;

DDB_BF_IRQ_SHARED	equ 1
DDB_BF_ACTIVE_BIT	equ 1
DDB_BF_ACTIVE	equ 1 SHL DDB_BF_ACTIVE_BIT 

;
; various equates
;

MAX_DRIVES		equ 2
