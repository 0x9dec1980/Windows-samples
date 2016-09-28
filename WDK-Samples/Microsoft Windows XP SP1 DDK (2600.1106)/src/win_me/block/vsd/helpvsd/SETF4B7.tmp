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

;****************************************************************
;                                                             	*
; vsdscsi.inc - SCSI Disk Vendor Specific Driver Include file   *
;                                                             	*
;****************************************************************

Read_data_flags 	EQU SRB_FLAGS_DATA_IN + SRB_FLAGS_ADAPTER_CACHE_ENABLE  + SRB_FLAGS_DISABLE_SYNCH_TRANSFER
Write_data_flags EQU SRB_FLAGS_DATA_OUT + SRB_FLAGS_ADAPTER_CACHE_ENABLE  + SRB_FLAGS_DISABLE_SYNCH_TRANSFER
Verify_data_flags 	EQU SRB_FLAGS_NO_DATA_TRANSFER
Req_sns_flags 	EQU SRB_FLAGS_DATA_IN
Tur_data_flags 	EQU SRB_FLAGS_NO_DATA_TRANSFER
Start_unit_flags 	EQU SRB_FLAGS_NO_DATA_TRANSFER
Lock_unlock_flags 	EQU SRB_FLAGS_NO_DATA_TRANSFER
Compute_geom_flags 	EQU SRB_FLAGS_NO_DATA_TRANSFER
Start_unit_flags EQU SRB_FLAGS_NO_DATA_TRANSFER
Default_flags	EQU	SRB_FLAGS_NO_DATA_TRANSFER


;Values for SCSI_VSD_Flags in DCB

WriteV_OK	EQU	0001h		;Write Verify command supported

VSD_MAX_RETRIES	EQU 	5

; Max request sense size

VSD_REQ_SENSE_SIZE	EQU 0Eh

; ioctl defines

CDROM_IOCTL_QUERY_MSB	EQU 0CFh

; function code for non-query cdrom_ioctls

CDROM_IOCTL_MSB	EQU	0CDh
