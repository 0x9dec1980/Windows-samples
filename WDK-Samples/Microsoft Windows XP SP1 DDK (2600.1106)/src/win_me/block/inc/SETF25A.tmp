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

DDB	STRUC
DDB_phys_addr	DD	?
DDB_Next_DDB	DD	?
DDB_Next_DDB_init	DD	?
DDB_dcb_ptr	DD	?
DDB_number_buses	DB	?
DDB_ios_flags	DB	?
DDB_sig	DW	?
DDB_dvt	DD	?
DDB_devnode_ptr	DD	?
DDB_reserved	DD	?
DDB	ENDS
DDB_IOS_FL_PENDING_REMOVAL	EQU	1
DDB_IOS_FL_PAGE_ALLOCED	EQU	2
DDB_IOS_FL_FIRST_ENUM_DONE_BIT	EQU	2
DDB_IOS_FL_PLEASE_DESTROY	EQU	4
