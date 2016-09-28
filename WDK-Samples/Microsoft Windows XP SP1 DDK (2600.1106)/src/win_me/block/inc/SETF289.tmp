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

ICE_TYPE_DRIVER	EQU	0
ICE_TYPE_MAP	EQU	1
ICE_TYPE_DEVICE	EQU	2
ICE_LAYER_TSD	EQU	0
ICE_LAYER_VSD_1	EQU	1
ICE_LAYER_VSD_2	EQU	2
ICE_LAYER_VSD_3	EQU	3
ICE_LAYER_VSD_4	EQU	4
ICE_LAYER_VSD_5	EQU	5
ICE_LAYER_VSD_6	EQU	6
ICE_LAYER_VSD_7	EQU	7
ICE_LAYER_VSD_8	EQU	8
ICE_LAYER_VSD_9	EQU	9
ICE_LAYER_PSD	EQU	10
ICE_LAYER_BID	EQU	11
ICE_ISA	EQU	0001H
ICE_EISA	EQU	0002H
ICE_MCA	EQU	0004H

ICE_header	STRUC
ICE_table_len	DD	?
ICE_header	ENDS

ICE	STRUC
ICE_len	DW	?
ICE_type	DW	?
ICE_platform	DW	?
ICE	ENDS

ICE_driver	STRUC
ICE_filler_1	DW	?
ICE_filler_2	DW	?
ICE_filler_3	DW	?
ICE_drv_ascii_name	DB	DVT_NAME_LEN DUP (?)
ICE_drv_create_date	DB	DVT_DATE_LEN DUP (?)
ICE_drv_create_time	DB	DVT_TIME_LEN DUP (?)
ICE_drv_rev_level	DB	DVT_REV_LEN DUP (?)
ICE_drv_file_name	DB	DVT_FNAME_LEN DUP (?)
ICE_drv_layer	DW	?
ICE_drv_length	DB	?
ICE_drv_option	DB	64 DUP (?)
ICE_driver	ENDS

ICE_map	STRUC
ICE_filler_4	DW	?
ICE_filler_5	DW	?
ICE_filler_6	DW	?
ICE_map_device_name	DB	6 DUP (?)
ICE_map_inquiry_flags	DB	8 DUP (?)
ICE_map_vendor_id	DB	8 DUP (?)
ICE_map_product_id	DB	16 DUP (?)
ICE_map_dev_rev_level	DB	4 DUP (?)
ICE_map_device_type	DB	?
ICE_map_ascii_name	DB	DVT_NAME_LEN DUP (?)
ICE_map_create_date	DB	DVT_DATE_LEN DUP (?)
ICE_map_create_time	DB	DVT_TIME_LEN DUP (?)
ICE_map_drv_rev_level	DB	DVT_REV_LEN DUP (?)
ICE_map	ENDS

ICE_device	STRUC
ICE_filler_7	DW	?
ICE_filler_8	DW	?
ICE_filler_9	DW	?
ICE_dev_device_name	DB	6 DUP (?)
ICE_dev_inquiry_flags	DB	8 DUP (?)
ICE_dev_vendor_id	DB	8 DUP (?)
ICE_dev_product_id	DB	16 DUP (?)
ICE_dev_rev_level	DB	4 DUP (?)
ICE_dev_device_type	DB	?
ICE_device	ENDS
IOCONFIG_TBL_START	EQU	<"$$$IOCONFIG$$start$$of$$table$$$">
IOCONFIG_TBL_END	EQU	<"$$$IOCONFIG$$$end$$of$$$table$$$">
