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

IOP_CALLBACK_TABLE_DEPTH	EQU	6

IOP_callback_entry	STRUC
IOP_CB_address	DD	?
IOP_CB_ref_data	DD	?
IOP_callback_entry	ENDS

IOP	STRUC
IOP_physical	DD	?
IOP_physical_dcb	DD	?
IOP_original_dcb	DD	?
IOP_timer	DW	?
IOP_timer_orig	DW	?
IOP_calldown_ptr	DD	?
IOP_callback_ptr	DD	?
IOP_voltrk_private	DD	?
ifdef	WIN31COMPAT
IOP_VM_Handle	DD	?
else
IOP_Thread_Handle	DD	?
endif
IOP_srb	DD	?
IOP_reserved	DD	2 DUP (?)
IOP_callback_table	DB	SIZE IOP_callback_entry * IOP_CALLBACK_TABLE_DEPTH DUP (?)
IOP_format_head	DB	?
IOP_format_xfer_rate	DB	?
IOP_format_track	DW	?
IOP_format_num_sectors	DD	?
IOP_ior	DB	SIZE IOR DUP (?)
IOP	ENDS
