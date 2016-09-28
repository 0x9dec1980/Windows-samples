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

DCB_cd_table_depth	EQU	10

DCB_cd_entry	STRUC
DCB_cd_io_address	DD	?
DCB_cd_flags	DD	?
DCB_cd_ddb	DD	?
DCB_cd_next	DD	?
DCB_cd_expan_off	DW	?
DCB_cd_layer_flags	DB	?
DCB_cd_lgn	DB	?
DCB_cd_entry	ENDS

DCB_COMMON	STRUC
DCB_physical_dcb	DD	?
DCB_expansion_length	DD	?
DCB_ptr_cd	DD	?
DCB_next_dcb	DD	?
DCB_next_logical_dcb	DD	?
DCB_drive_lttr_equiv	DB	?
DCB_unit_number	DB	?
DCB_TSD_Flags	DW	?
DCB_vrp_ptr	DD	?
DCB_dmd_flags	DD	?
DCB_device_flags	DD	?
DCB_device_flags2	DD	?
DCB_Partition_Start	DD	?
DCB_track_table_ptr	DD	?
DCB_bds_ptr	DD	?
DCB_Reserved1	DD	?
DCB_Reserved2	DD	?
DCB_apparent_blk_shift	DB	?
DCB_partition_type	DB	?
DCB_sig	DW	?
DCB_device_type	DB	?
DCB_Exclusive_VM	DD	?
DCB_disk_bpb_flags	DB	?
DCB_cAssoc	DB	?
DCB_Sstor_Host	DB	?
DCB_user_drvlet	DW	?
DCB_Reserved3	DW	?
DCB_Reserved4	DD	?
DCB_COMMON	ENDS

DCB_BLOCKDEV	STRUC
DCB_BDD_Next	DD	?
DCB_BDD_BD_Major_Version	DB	BDD_BD_Major_Ver
DCB_BDD_BD_Minor_Version	DB	BDD_BD_Minor_Ver
DCB_BDD_Device_SubType	DB	?
DCB_BDD_Int_13h_Number	DB	?
DCB_BDD_flags	DD	?
DCB_BDD_Name_Ptr	DD	?
DCB_apparent_sector_cnt	DD	2 DUP (?)
DCB_apparent_blk_size	DD	?
DCB_apparent_head_cnt	DD	?
DCB_apparent_cyl_cnt	DD	?
DCB_apparent_spt	DD	?
DCB_BDD_Sync_Cmd_Proc	DD	?
DCB_BDD_Command_Proc	DD	?
DCB_BDD_Hw_Int_Proc	DD	0
DCB_BDP_Cmd_Queue_Ascending	DD	?
DCB_BDP_Cmd_Queue_Descending	DD	?
DCB_BDP_Current_Flags	DD	?
DCB_BDP_Int13_Param_Ptr	DD	?
DCB_BDP_Current_Command	DD	?
DCB_BDP_Current_Position	DD	2 DUP (?)
DCB_BDP_Reserved	DD	5 DUP (?)
DCB_fastdisk_bdd	DD	?
DCB_BLOCKDEV	ENDS

DCB	STRUC
DCB_cmn	DB	SIZE DCB_COMMON DUP (?)
DCB_max_xfer_len	DD	?
DCB_actual_sector_cnt	DD	2 DUP (?)
DCB_actual_blk_size	DD	?
DCB_actual_head_cnt	DD	?
DCB_actual_cyl_cnt	DD	?
DCB_actual_spt	DD	?
DCB_next_ddb_dcb	DD	?
DCB_dev_node	DD	?
DCB_bus_type	DB	?
DCB_bus_number	DB	?
DCB_queue_freeze	DB	?
DCB_max_sg_elements	DB	?
DCB_io_pend_count	DB	?
DCB_lock_count	DB	?
DCB_SCSI_VSD_FLAGS	DW	?
DCB_scsi_target_id	DB	?
DCB_scsi_lun	DB	?
DCB_scsi_hba	DB	?
DCB_max_sense_data_len	DB	?
DCB_srb_ext_size	DW	?
DCB_inquiry_flags	DB	8 DUP (?)
DCB_vendor_id	DB	8 DUP (?)
DCB_product_id	DB	16 DUP (?)
DCB_rev_level	DB	4 DUP (?)
DCB_port_name	DB	8 DUP (?)
DCB_current_unit	DB	?
DCB_blocked_iop	DD	?
DCB_vol_unlock_timer	DD	?
DCB_access_timer	DB	?
DCB_Vol_Flags	DB	?
DCB_q_algo	DB	?
DCB_unit_on_ctl	DB	?
DCB_Port_Specific	DD	?
DCB_spindown_timer	DD	?
DCB_bdd	DB	SIZE DCB_BLOCKDEV DUP (?)
DCB	ENDS

LOG_DCB	STRUC
DCB_cmn	DB	SIZE DCB_COMMON DUP (?)
LOG_DCB	ENDS

DCB_disk	STRUC
DCB_filler_1	DB	SIZE DCB DUP (?)
DCB_write_precomp	DW	?
DCB_disk_tsd_private	DD	?
DCB_disk	ENDS

DCB_floppy	STRUC
DCB_filler_3	DB	SIZE DCB_disk DUP (?)
DCB_3mode_Flags	DW	?
DCB_floppy	ENDS
DCB_IS_3MODE_DRIVE	EQU	0001H
DCB_SET_RPM_ON_ACCESS	EQU	0002H

DCB_disk_geometry	STRUC
sector_cnt	DD	2 DUP (?)
blk_size	DD	?
head_cnt	DD	?
cyl_cnt	DD	?
spt	DD	?
DCB_disk_geometry	ENDS

.errnz DCB_apparent_sector_cnt - DCB_apparent_sector_cnt[0] - sector_cnt[0]
.errnz DCB_apparent_blk_size - DCB_apparent_sector_cnt[0] - blk_size
.errnz DCB_apparent_head_cnt - DCB_apparent_sector_cnt[0] - head_cnt
.errnz DCB_apparent_cyl_cnt - DCB_apparent_sector_cnt[0] - cyl_cnt
.errnz DCB_apparent_spt - DCB_apparent_sector_cnt[0] - spt

.errnz DCB_actual_sector_cnt - DCB_actual_sector_cnt[0] - sector_cnt[0]
.errnz DCB_actual_blk_size - DCB_actual_sector_cnt[0] - blk_size
.errnz DCB_actual_head_cnt - DCB_actual_sector_cnt[0] - head_cnt
.errnz DCB_actual_cyl_cnt - DCB_actual_sector_cnt[0] - cyl_cnt
.errnz DCB_actual_spt - DCB_actual_sector_cnt[0] - spt


DCBF_DISK_BPB_USEFAKE	EQU	00000001H
CDROM_MAX_TOC	EQU	804
CDROM_MAX_MODE_BUF	EQU	20

DCB_cdrom	STRUC
DCB_filler_2	DB	SIZE DCB DUP (?)
DCB_cdrom_Partition_Start	DD	?
DCB_cdrom_Partition_End	DD	?
DCB_cd_ls_ft	DB	?
DCB_TOC	DD	CDROM_MAX_TOC / 4 + 1 DUP (?)
	.errnz	(DCB_TOC / 4) * 4 - DCB_TOC

DCB_cd_mode_sense_buf	DD	CDROM_MAX_MODE_BUF /4 + 1 DUP (?)
DCB_cd_first_session	DB	?
DCB_cd_last_session	DB	?
DCB_play_resume_start	DD	?
DCB_play_resume_stop	DD	?
DCB_play_status	DD	?
DCB_cd_device_flags	DD	?
DCB_cd_fs_lt	DB	?
DCB_cd_bobbit_pt	DB	?
DCB_cd_fs_lo	DD	?
DCB_cd_last_session_start	DD	?
DCB_cd_current_block_size	DD	?
DCB_cd_vol_map	DB	8 DUP (?)
DCB_cd_current_command	DD	?
DCB_cd_queue_head	DD	?
DCB_cd_reserved	DD	?
DCB_cdrom	ENDS
DCB_cdaudio_playing	EQU	0001H
DCB_cdaudio_paused	EQU	0002H
DCB_cdaudio_vol_set	EQU	0004H
DCB_cd_supports_audio_play	EQU	0001H
DCB_cd_supports_data_only	EQU	0000H
DCB_cd_multi_session_valid	EQU	0002H
DCB_CD_CHANGEABLE_BLOCK	EQU	0004H
DCB_CD_FAKE_SCSI2	EQU	0008H
DCB_CD_CDPLUS_DISC	EQU	0010H
DCB_CD_FAKE_SCSI1	EQU	0020H
DCB_CD_USE_ATAPI_READCD	EQU	0040H
DCB_q_fi_fo	EQU	00H
DCB_q_sort	EQU	01H
DCB_type_disk	EQU	00H
DCB_type_tape	EQU	01H
DCB_type_printer	EQU	02H
DCB_type_processor	EQU	03H
DCB_type_worm	EQU	04H
DCB_type_cdrom	EQU	05H
DCB_type_scanner	EQU	06H
DCB_type_optical_memory	EQU	07H
DCB_type_changer	EQU	08H
DCB_type_comm	EQU	09H
DCB_type_floppy	EQU	0AH
ifdef  NEC_98
DCB_type_optical_nec	EQU	84H
endif
DCB_INQ_DATA_LENGTH	EQU	SIZE DCB_inquiry_flags+SIZE DCB_vendor_id+SIZE DCB_product_id+SIZE DCB_rev_level
DCB_BUS_ESDI	EQU	00H
DCB_BUS_SCSI	EQU	01H
DCB_BUS_NEC	EQU	02H
DCB_BUS_SMART	EQU	03H
DCB_BUS_ABIOS	EQU	04H
DCB_VF_INHIBIT_LOCKING	EQU	01H
DCB_VF_INHIBIT_IO	EQU	02H
DCB_VF_INHIBIT_GEOM_RECOMPUTE	EQU	04H
DCB_VF_INHIBIT_GEOM_RECOMPUTE_BIT	EQU	2
DCB_VF_UNLOCK_SCHED_BIT	EQU	4
DCB_VF_UNLOCK_SCHED	EQU	(1 SHL DCB_VF_UNLOCK_SCHED_BIT)
DCB_VF_NEED_PHYS_RECOMP_BIT	EQU	5
DCB_VF_NEED_PHYS_RECOMP	EQU	(1 SHL DCB_VF_NEED_PHYS_RECOMP_BIT)
DCB_VF_PROT_NEC_DRIVE_BIT	EQU	6
DCB_VF_PROT_NEC_DRIVE	EQU	(1 SHL DCB_VF_PROT_NEC_DRIVE_BIT)
DCB_TSD_INVALID_PARTITION	EQU	1
DCB_TSD_BAD_MBR	EQU	1
DCB_TSD_USER_LETTERS_CHECKED_BIT	EQU	1
DCB_TSD_USER_LETTERS_CHECKED	EQU	1 SHL DCB_TSD_USER_LETTERS_CHECKED_BIT
DCB_TSD_FIRST_USER_CHECK_BIT	EQU	2
DCB_TSD_FIRST_USER_CHECK	EQU	1 SHL DCB_TSD_FIRST_USER_CHECK_BIT
DCB_TSD_BID_SET_GEOM	EQU	0008H
DCB_TSD_USER_SET_BIT	EQU	4
DCB_TSD_USER_SET	EQU	1 SHL DCB_TSD_USER_SET_BIT
DCB_TSD_NO_USER_INT13	EQU	0020H
DCB_TSD_PROTMODE_INT_13_BIT	EQU	6
DCB_TSD_PROTMODE_INT_13	EQU	1 SHL DCB_TSD_PROTMODE_INT_13_BIT
DCB_TSD_NONEXIST_PARTITION_BIT	EQU	7
DCB_TSD_NONEXIST_PARTITION	EQU	1 SHL DCB_TSD_NONEXIST_PARTITION_BIT
DCB_TSD_MBPB_PBR	EQU	0100H
DCB_TSD_RBPB_INIT_BIT	EQU	09H
DCB_TSD_RBPB_INIT	EQU	(1 SHL DCB_TSD_RBPB_INIT_BIT)
DCB_TSD_MBPB_INIT	EQU	0400H
DCB_TSD_APPARENT_PBR	EQU	0800H
DCB_TSD_APPARENT_ROM	EQU	1000H
DCB_TSD_ACTUAL_SET	EQU	2000H
DCB_TSD_ACTUAL_PRE_SET	EQU	4000H
DCB_TSD_APPARENT_PRE_SET	EQU	8000H
DCB_DEV_SPINDOWN_SUPPORTED_BIT	EQU	31
DCB_DEV_SPINDOWN_SUPPORTED	EQU	(1 SHL DCB_DEV_SPINDOWN_SUPPORTED_BIT)
DCB_DEV_SPUN_DOWN_BIT	EQU	30
DCB_DEV_SPUN_DOWN	EQU	(1 SHL DCB_DEV_SPUN_DOWN_BIT)
DCB_DEV_IO_ACTIVE_BIT	EQU	29
DCB_DEV_IO_ACTIVE	EQU	(1 SHL DCB_DEV_IO_ACTIVE_BIT)
DCB_DEV_INT13_CONFIGURED_BIT	EQU	28
DCB_DEV_INT13_CONFIGURED	EQU	(1 SHL DCB_DEV_INT13_CONFIGURED_BIT)
DCB_DEV_NO_ASPICAM_BIT	EQU	27
DCB_DEV_NO_ASPICAM	EQU	(1 SHL DCB_DEV_NO_ASPICAM_BIT)
DCB_DEV_REMOVAL_PENDING_BIT	EQU	26
DCB_DEV_REMOVAL_PENDING	EQU	(1 SHL DCB_DEV_REMOVAL_PENDING_BIT)
DCB_DEV_RMM_INT13_BIT	EQU	25
DCB_DEV_RMM_INT13	EQU	(1 SHL DCB_DEV_RMM_INT13_BIT)
DCB_DEV_TSD_PROCESSED_BIT	EQU	24
DCB_DEV_TSD_PROCESSED	EQU	(1 SHL DCB_DEV_TSD_PROCESSED_BIT)
DCB_DEV_SINGLE_FLOPPY	EQU	00800000H
DCB_DEV_REAL_MODE	EQU	00400000H
DCB_DEV_QUE_DESCEND	EQU	00200000H
DCB_DEV_MEDIA_CHANGED	EQU	00100000H
DCB_DEV_INVALID_GEOM	EQU	00080000H
DCB_DEV_INHIBIT_SW_TIMED_SUPPORT	EQU	00040000H
DCB_DEV_ASYNC_MED_CHG_SUPPORT	EQU	00020000H
DCB_DEV_SYNC_MED_CHG_SUPPORT	EQU	00010000H
DCB_DEV_PHYSICAL	EQU	00008000H
DCB_DEV_LOGICAL	EQU	00004000H
DCB_DEV_LOGICAL_BIT	EQU	14
DCB_DEV_RMM	EQU	00002000H
DCB_DEV_RMM_BIT	EQU	13
DCB_DEV_UNCERTAIN_MEDIA	EQU	00001000H
DCB_DEV_ROM_CFG_REQ	EQU	00000800H
DCB_DEV_MUST_CONFIGURE	EQU	00000400H
DCB_DEV_SSTOR_VOL	EQU	00000200H
DCB_DEV_PAGING_DEVICE	EQU	00000100H
DCB_CHAR_DEVICE	EQU	00000080H
DCB_DEV_I13_IN_PROGRESS	EQU	00000040H
DCB_DEV_CACHE	EQU	<BDF_CACHE>
DCB_DEV_SERIAL_CMD	EQU	<BDF_Serial_Cmd>
DCB_DEV_REMOTE	EQU	<BDF_Remote>
DCB_DEV_REMOVABLE	EQU	<BDF_Removable>
DCB_DEV_REMOVABLE_BIT	EQU	2
DCB_DEV_WRITEABLE	EQU	<BDF_Writeable>
DCB_DEV_INT13_DRIVE	EQU	<BDF_Int13_Drive>
DCB_DEV2_THREAD_LOCK	EQU	00000001H
DCB_DEV2_THREAD_LOCK_BIT	EQU	0
DCB_DEV2_ATAPI_DEVICE	EQU	00000002H
DCB_DEV2_ATAPI_DEVICE_BIT	EQU	1
DCB_DEV2_DMF_DISK	EQU	00000004H
DCB_DEV2_DMF_DISK_BIT	EQU	2
DCB_DEV2_I13_COPY_PROT	EQU	00000008H
DCB_DEV2_I13_COPY_PROT_BIT	EQU	3
DCB_DEV2_FAIL_FORMAT	EQU	00000010H
DCB_DEV2_FAIL_FORMAT_BIT	EQU	4
DCB_DEV2_FLOPTICAL	EQU	00000020H
DCB_DEV2_FLOPTICAL_BIT	EQU	5
ifdef	NEC_98
DCB_DEV2_98_PARTITION	EQU	80000000H
DCB_DEV2_98_PARTITION_BIT	EQU	31
endif
DCB_dmd_srb_cdb	EQU	00000001H
DCB_dmd_rsrv_1	EQU	00000002H
DCB_dmd_logical	EQU	00000004H
DCB_dmd_physical	EQU	00000008H
DCB_dmd_small_memory	EQU	00000010H
DCB_dmd_rsrv_2	EQU	00000020H
DCB_dmd_rsrv_3	EQU	00000040H
DCB_dmd_rsrv_4	EQU	00000080H
DCB_dmd_not_512	EQU	00000100H
DCB_dmd_word_align	EQU	00000200H
DCB_dmd_dword_align	EQU	00000400H
DCB_dmd_phys_sgd	EQU	00000800H
DCB_dmd_phys_sgd_ptr	EQU	00001000H
DCB_dmd_do_a_b_toggling	EQU	00002000H
DCB_dmd_query_remov	EQU	00004000H
DCB_dmd_request_sns	EQU	00008000H
DCB_dmd_lock_unlock_media	EQU	00010000H
DCB_dmd_load_eject_media	EQU	00020000H
DCB_dmd_clear_media_chg	EQU	00040000H
DCB_dmd_serialize_bit	EQU	19
DCB_dmd_serialize	EQU	1 SHL DCB_dmd_serialize_bit
DCB_dmd_prot_mode	EQU	00100000H
DCB_dmd_no_xclusive	EQU	00200000H
DCB_dmd_pageability	EQU	00400000H
DCB_dmd_rsrv	EQU	DCB_dmd_rsrv_1+DCB_dmd_rsrv_2+DCB_dmd_rsrv_3+DCB_dmd_rsrv_4
