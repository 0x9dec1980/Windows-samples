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

DVT_NAME_LEN	EQU	16
DVT_DATE_LEN	EQU	8
DVT_TIME_LEN	EQU	8
DVT_FILE_LEN	EQU	8
DVT_REV_LEN	EQU	4
DVT_FC_LEN	EQU	4
DVT_FNAME_LEN	EQU	12

DVT	STRUC
DVT_reserved_1	DW	?
DVT_next_dvt	DD	?
DVT_device_cnt	DW	?
DVT_aer	DD	?
DVT_ddb	DD	?
DVT_ddb_init	DD	?
DVT_ascii_name	DB	DVT_NAME_LEN DUP (?)
DVT_create_date	DB	DVT_DATE_LEN DUP (?)
DVT_create_time	DB	DVT_TIME_LEN DUP (?)
DVT_rev_level	DB	DVT_REV_LEN DUP (?)
DVT_feature_code	DD	?
DVT_if_requirements	DW	?
DVT_bus_type	DB	?
DVT_reference_data	DD	?
DVT_first_drive	DB	?
DVT_current_lgn	DB	?
DVT_LoadHandle	DD	?
DVT_scsi_max_target	DB	?
DVT_scsi_max_lun	DB	?
DVT_entry_point	DD	?
DVT_init_count	DB	?
DVT_reserved	DD	2 DUP (?)
DVT	ENDS
DVT_BT_ESDI	EQU	00H
DVT_BT_SCSI	EQU	01H
DVT_BT_FLOPPY	EQU	02H
DVT_BT_SMART	EQU	03H
DVT_BT_ABIOS	EQU	04H
DVT_FC_SCAN_DOWN	EQU	04H
DVT_FC_IO_FOR_INQ_AEP	EQU	40H
DVT_FC_HALF_SEC	EQU	2000H
DVT_FC_1_SEC	EQU	2000H
DVT_FC_2_SECS	EQU	4000H
DVT_FC_4_SECS	EQU	8000H
DVT_FC_DYNALOAD	EQU	10000H
DVT_FC_NEED_PRELOAD	EQU	20000H
DVT_FC_NEED_PRE_POST_LOAD	EQU	40000H
