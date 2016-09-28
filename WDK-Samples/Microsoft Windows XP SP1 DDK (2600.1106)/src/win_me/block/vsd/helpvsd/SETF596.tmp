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

;
; disk vsd trace entry format
;

VTE	struc

VTE_type	db	?		; entry type. see below for equates.
VTE_iop		dd	?		; IOP pointer
VTE_srb_func	db	?		; srb function
VTE_srb_stat	db	?		; srb status
VTE_scsi_stat	db	?		; scsi status
VTE_sense_key	db	?		; sense key
VTE_adsense	db	?		; additional sense code
VTE_ioctl_func	dw	?		; ioctl function
VTE_func	db	?		; IOP function.
VTE_unit	db	?		; DEVICE unit number.
VTE	ends

VTE_NUM_ENTRIES equ     512
VTE_TABLE_SIZE	equ	(VTE_NUM_ENTRIES + 1) * size VTE

VTE_REQ		  	equ	1
VTE_CALLDOWN		equ	2
VTE_COMP		equ	3
VTE_CALL_UP	  	equ	4
VTE_RESUBMIT	  	equ	5
VTE_WRITE_CB	  	equ	6
VTE_ERROR	  	equ	7
VTE_UNRECOV  		equ	8
VTE_ISSUE_SENSE  	equ	09h
VTE_SENSE_EVENT  	equ	0ah
VTE_SENSE_VALID		equ	0bh
VTE_INTERNAL	  	equ	0ch
VTE_RETRY_SENSE		equ	0dh
VTE_PARSE_SENSE	  	equ	0eh
VTE_START_UNIT	  	equ	0fh
VTE_START_EVENT  	equ	10h
VTE_CALLUP_WITH_ERR	equ	11h
VTE_XLATE_SENSE		equ	12h
VTE_COMPUTE_CB		equ	13h

VTE_MAX_TYPE	  equ   13h
