/****************************************************************************
*
*       Copyright (c) Microsoft Corporation 1992
*
*       All rights reserved
*
\****************************************************************************/
/****************************************************************************
*
* DCB (Device Control Block) Data Structure
*
\****************************************************************************/

/*
** define the call down table
*/

#define DCB_cd_table_depth      10      // allow a layering of ten deep

typedef struct  _DCB_cd_entry { /* */
        PVOID   DCB_cd_io_address;      // addr of request routine
        ULONG   DCB_cd_flags;           // demand bits - as defined below
        ULONG   DCB_cd_ddb;             // driver's DDB pointer
        ULONG   DCB_cd_next;            // pointer to next cd entry
        USHORT  DCB_cd_expan_off;       // offset of expansion area
        UCHAR   DCB_cd_layer_flags;     // flags for layer's use
        UCHAR   DCB_cd_lgn;             // load group number
} DCB_cd_entry, *pDCB_cd_entry;

/*
** union fields follow
*/



/*
** fields common to physical and logical disk DCB's 
*/

typedef struct  _DCB_COMMON { /* */

        ULONG   DCB_physical_dcb;       /* DCB for physical device */
        ULONG   DCB_expansion_length;   /* total length of IOP extension filled */
                                        /* in by IOS (excludes IOP size) */
        /*
         * Link fields follow
         */

        PVOID   DCB_ptr_cd;             /* pointer to calldown list */
        ULONG   DCB_next_dcb;           /* link to next DCB */
        ULONG   DCB_next_logical_dcb;   /* pointer to next logical dcb */
                                        /* for physical device */

        BYTE    DCB_drive_lttr_equiv;   /* drive number (A: = 0, etc.) */
                                        /* set up by iosserv during logical */
                                        /* device associate processing. */

        BYTE    DCB_unit_number;        /* either physical drive number */
                                        /* (sequential drive number or'd */
                                        /* with 80h) or unit number within */
                                        /* tsd. set up by iosbid for disk */
                                        /* physical dcb's. set up by tsdpart*/
                                        /* for disk logical dcb's. set up by*/
                                        /* tsdaer for cdrom physical dcb's. */

        USHORT  DCB_TSD_Flags;          /* Flags for TSD */

        /*
         * Volume Tracking fields follow
         */

        ULONG   DCB_vrp_ptr;            /* pointer to VRP for this DCB */

        ULONG   DCB_dmd_flags;          /* demand bits of the topmost layer */

        ULONG   DCB_device_flags;       /* was BDD_Flags */
        ULONG   DCB_device_flags2;      /* second set of general purpose flags*/

        ULONG   DCB_Partition_Start;    /* partition start sector */

        ULONG   DCB_track_table_ptr;    /* pointer for the track table buffer */
                                        /* for ioctls */

        ULONG   DCB_bds_ptr;            /* DOS BDS corresp. to this DCB */
                                        /* (logical DCB's only) */
        ULONG   DCB_Reserved1;          /* reserved - MBZ */
        //ULONG   DCB_Reserved2;          /* reserved - MBZ */
        ULONG   DCB_pEid;               /* IDE ID block */

        BYTE    DCB_apparent_blk_shift; /* log of apparent_blk_size */
        BYTE    DCB_partition_type;     /* partition type */
        USHORT  DCB_sig;                /* padding and signature */
        BYTE    DCB_device_type;        /* Device Type */
        ULONG   DCB_Exclusive_VM;       /* handle for exclusive access to this device */
        UCHAR   DCB_disk_bpb_flags;     /* bpb flags see defines below */
        UCHAR   DCB_cAssoc;             /* count of logical drives */
                                        /* associated with this logical DCB */
        UCHAR   DCB_Sstor_Host;         /* This field indicates a sstor host volume */

        USHORT DCB_user_drvlet;         /* contains the userdriveletter settings else ff*/
        USHORT DCB_Reserved3;           /* reserved - MBZ */
        
        //#ifndef OnNow
        //ULONG   DCB_Reserved4;          /* reserved - MBZ */
        //#else OnNow
        BYTE    DCB_fACPI;              /* Are we on ACPI subtree ? */
        BYTE    DCB_fSpinDownIssued;    /* Has a spindown issued ? */
        BYTE    DCB_bPowerState;
        BYTE    DCB_bEidLength;
        //#endif


} DCB_COMMON, *PDCB_COMMON;

/*
** RMM uses DCB_Reserved2 field. This should be mutually exclusive to 
** Pmode IDE driver controlled DCB which should never be controlled by RMM.
*/

#define DCB_Reserved2 DCB_pEid


/*
** blockdev compatible fields in physical disk DCB's
*/

typedef struct  _DCB_BLOCKDEV { /* */

        /* The initial set of fields below should not be re-ordered or modified in
         * anyway as the offset and the size of fileds have to match those
         * in BLOCKDEV.H for COMPATIBITLITY!!!
         */

        ULONG   DCB_BDD_Next;
        BYTE    DCB_BDD_BD_Major_Version;       /* INIT <BDD_BD_Major_Ver>                        */
        BYTE    DCB_BDD_BD_Minor_Version;       /* INIT <BDD_BD_Minor_Ver>                        */
        BYTE  DCB_BDD_Device_SubType;
        BYTE  DCB_BDD_Int_13h_Number;
        ULONG DCB_BDD_flags;                                    /* was BDD_Flags                                                  */
        ULONG DCB_BDD_Name_Ptr;
        ULONG   DCB_apparent_sector_cnt[2];/* num. of secs as seen by tsd and above */
        ULONG   DCB_apparent_blk_size;  /* blk size of dev. as seen by tsd and above */
        ULONG   DCB_apparent_head_cnt;  /* num. of heads as seen by tsd and above */
        ULONG DCB_apparent_cyl_cnt;     /* num. of cyls as seen by tsd and above */
        ULONG   DCB_apparent_spt;       /* num. of secs/trk as seen by tsd and above */
        ULONG DCB_BDD_Sync_Cmd_Proc;
        ULONG DCB_BDD_Command_Proc;
        ULONG DCB_BDD_Hw_Int_Proc;              /* INIT <0> */
        ULONG   DCB_BDP_Cmd_Queue_Ascending;
        ULONG   DCB_BDP_Cmd_Queue_Descending;
        ULONG   DCB_BDP_Current_Flags;
        ULONG   DCB_BDP_Int13_Param_Ptr;
        ULONG   DCB_BDP_Current_Command;
        ULONG   DCB_BDP_Current_Position[2];
        ULONG   DCB_BDP_Reserved[5];
        ULONG   DCB_fastdisk_bdd;         /* set for DCBs created when a fastdisk regs */
                                                                             /* with the blockdev BDD for it else 0 */

        /*
         *  End BlockDev compatibility
         */

} DCB_BLOCKDEV, *PDCB_BLOCKDEV;

/*
** define the device control block (dcb) for physical devices (i.e., disks)
*/

typedef struct  _DCB { /* */

        DCB_COMMON      DCB_cmn;

        ULONG   DCB_max_xfer_len;       /* maximum transfer length */

        /*
         *      Actual geometry data follows
         */

        ULONG   DCB_actual_sector_cnt[2];/* number of sectors as seen below  */
                                        /* the tsd.                         */
        ULONG   DCB_actual_blk_size;    /* actual block size of the device  */
                                        /* as seen below the tsd.           */
        ULONG   DCB_actual_head_cnt;    /* number of heads as seen below    */
                                        /* the tsd.                         */
        ULONG   DCB_actual_cyl_cnt;     /* number of cylinders as seen      */
                                        /* below the tsd.                   */
        ULONG     DCB_actual_spt;         /* number of sectors per track as   */
                                        /* seen below the tsd.              */

        PVOID   DCB_next_ddb_dcb;                 /* link to next DCB on DDB chain */

        PVOID     DCB_dev_node;                   /* pointer to dev node for this device */
        BYTE    DCB_bus_type;           /* Type of BUS, see below           */

        BYTE    DCB_bus_number;           /* channel (cable) within adapter   */
        UCHAR   DCB_queue_freeze;  /* queue freeze depth counter */

        UCHAR   DCB_max_sg_elements;    /* max # s/g elements.  set initially */
                                          /* by port, but may be MORE RESTRICTIVELY */
                                                                                           /* updated by other layers */
        UCHAR   DCB_io_pend_count; /* indicates number of requests pending */
                                   /* for this DCB (VOLUME TRACKING LAYER USE ONLY) */
        UCHAR   DCB_lock_count;    /* depth counter for LOCK MEDIA commands */
                                   /* (VOLUME TRACKING LAYER USE ONLY) */
        /*
         * SCSI fields follow
         */

        USHORT  DCB_SCSI_VSD_FLAGS;     /* Flags for SRB builder            */
        
        BYTE    DCB_scsi_target_id;              /* SCSI target ID */
        BYTE    DCB_scsi_lun;                                           /* SCSI logical unit number         */
        BYTE    DCB_scsi_hba;                    /* adapter number relative to port driver */
        BYTE    DCB_max_sense_data_len; /* Maximum sense Length             */
        USHORT  DCB_srb_ext_size;         /* miniport srb extension length    */

        BYTE    DCB_inquiry_flags[8];   /* Device Inquiry Flags             */
        BYTE    DCB_vendor_id[8];       /* Vendor ID string                 */

        BYTE    DCB_product_id[16];     /* Product ID string                */

        BYTE    DCB_rev_level[4];       /* Product revision level           */

        BYTE    DCB_port_name[8];

        UCHAR   DCB_current_unit;       /* used to emulate multiple logical devices */
                                        /* with a single physical device */
        ULONG   DCB_blocked_iop;        /* pointer to requests for an inactive */
                                          /* volume (VOLUME TRACKING LAYER USE ONLY) */
        ULONG   DCB_vol_unlock_timer;   /* unlock timer handle */
        UCHAR   DCB_access_timer;       /* used to measure time between accesses */
        UCHAR   DCB_Vol_Flags;          /* Flags for Volume Tracking          */
                                        /* volume tracking use only */
        BYTE    DCB_q_algo;             /* queuing algorithm index - see    */
                                        /* values below.                    */

        BYTE    DCB_unit_on_ctl;        /* relative device number on ctlr (0-based) */
        ULONG   DCB_Port_Specific;      /* bytes for PORT DRIVER use        */

        ULONG   DCB_spindown_timer;      /* timer for drive spin down      */

        DCB_BLOCKDEV    DCB_bdd;
 
} DCB, *PDCB;


/* ASM

;; changing the DCB size will break all precompiled utilities.

.errnz SIZE DCB NE 299

*/


/*
** define the device control block (dcb) for logical disk devices
*/

typedef struct  _LOG_DCB { /* */

        DCB_COMMON      DCB_cmn;

} LOG_DCB, *PLOG_DCB;

/*
** define the device control block (dcb) for disk and cd-rom device types
*/

//
//  NOTE: Any changes in this structure must also be reflected in cfr.h
//

// shk BUGBUG Note that this same structure is used for floppies too...
// see iosbid.asm in inquiry_type_table.
typedef struct  _DCB_disk { /* */

        DCB     DCB_filler_1;           /* standard dcb header              */

        USHORT  DCB_write_precomp;      /* Write Precompensation            */

   ULONG   DCB_disk_tsd_private;   /* private area for tsd.  used to  */
                                                                                                         /* store b: BDS ptr for single flp */
} DCB_disk, *pDCB_disk;

//
// To support Japanese 3mode floppy driver this is defined to add an extra
// field to the end of DCB_disk.
//
//

typedef struct  _DCB_floppy { /* */

        DCB_disk  DCB_filler_3;         /* standard dcb header */

        USHORT  DCB_3mode_Flags;        /* 3mode floppy flag */

} DCB_floppy, *pDCB_floppy;

#define DCB_IS_3MODE_DRIVE      0x0001  /* floppy drive supports 3mode access */
#define DCB_SET_RPM_ON_ACCESS   0x0002  /* set spindle speed on every access */

/*
** DCB_disk_geometry structure access into either the actual or apparent
** DCB geometry fields
*/

typedef struct _DCB_disk_geometry { /* */
        ULONG   sector_cnt[2];/* number of sectors */
        ULONG   blk_size;       /* actual block size of the device  */
                                        /* as seen below the tsd.           */
        ULONG   head_cnt;       /* number of heads as seen below    */
                                        /* the tsd.                         */
        ULONG   cyl_cnt;        /* number of cylinders as seen      */
                                        /* below the tsd.                   */
        ULONG   spt;            /* number of sectors per track as   */
                                        /* seen below the tsd.              */
} DCB_disk_geometry, *pDCB_disk_geometry;

/* ASM

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

*/

/*
** DCB_disk_bpb_flags definitions follow
*/

#define DCBF_DISK_BPB_USEFAKE   0x00000001

/*
** maximum size of a Table of Contents for a CDROM
*/

#define CDROM_MAX_TOC 804

/*
** maximum size of a mode sense buffer for CDROM
*/
#define CDROM_MAX_MODE_BUF 20

typedef struct  _DCB_cdrom { /* */

        DCB     DCB_filler_2;           /* standard dcb header                */

        ULONG   DCB_cdrom_Partition_Start;    /* partition start sector      */
        ULONG   DCB_cdrom_Partition_End;      /* partition end sector         */
    UCHAR       DCB_cd_ls_ft;           /* first track number in the last sess*/

        ULONG   DCB_TOC[CDROM_MAX_TOC / 4 + 1]; /* cdrom toc buffer           */

/* ASM
        .errnz  (DCB_TOC / 4) * 4 - DCB_TOC
*/

        ULONG   DCB_cd_mode_sense_buf[CDROM_MAX_MODE_BUF /4 + 1]; /* cdrom mode */

    UCHAR   DCB_cd_first_session;       /* index number of first session      */
        UCHAR   DCB_cd_last_session;    /* index of last session on disc      */

        ULONG   DCB_play_resume_start;  /* re-start address when paused(LBA)  */
        ULONG   DCB_play_resume_stop;   /* end of re-started play (LBA)       */
        ULONG   DCB_play_status;        /* last command play status           */
        ULONG   DCB_cd_device_flags;    /* flags indicating the audio support
                                          capabilities of the device          */

        UCHAR   DCB_cd_fs_lt;           /* last track in the first session    */
        UCHAR DCB_cd_bobbit_pt; /* indicates the track where a CDPLUS disc is */
                                                                   /* chopped off to prevent data tracks from showing */
                                                                        /* up in the table of contents                              */

        ULONG   DCB_cd_fs_lo;           /* lead out address of first session  */
        ULONG   DCB_cd_last_session_start  ; /* LBA start address of data area in
                                  the last session on a multi-session disc    */

   ULONG   DCB_cd_current_block_size;  /* current block size selected by the
                                                   mode command               */

        UCHAR   DCB_cd_vol_map[8];              /* current volume / channel mapping */
        ULONG   DCB_cd_current_command; /* current command                    */

        ULONG   DCB_cd_queue_head;      /* head of current command queue      */

    UCHAR   DCB_cd_grp1_time[2];    /* group 1 command timeouts        */
    UCHAR   DCB_cd_grp2_time[2];    /* group 2 command timeouts        */

	ULONG	DCB_cd_dvd_copy_flags;  /* dvd copy protection bytes */

} DCB_cdrom, *PDCB_cdrom;



/*
** DCB_play_status definitions
*/
#define DCB_cdaudio_playing     0x0001  // indicates audio play in progress
#define DCB_cdaudio_paused              0x0002  // indicates audio operation is paused
#define DCB_cdaudio_vol_set     0x0004  // default volume has been set for disc

/*
** DCB_cd_device_flags definitions
*/

#define DCB_cd_supports_audio_play 0x0001
#define DCB_cd_supports_data_only  0x0000
#define DCB_cd_multi_session_valid 0x0002       // indicates a multi session
                                                // disc in the drive
#define DCB_CD_CHANGEABLE_BLOCK    0x0004       // indicates block size changeable

#define DCB_CD_FAKE_SCSI2                 0x0008        // indicates SW emulated SCSI 2
#define DCB_CD_CDPLUS_DISC                        0x0010 // indicates a CDPLUS disc
#define DCB_CD_FAKE_SCSI1                         0x0020 // indicates force of SCSI 1 load
                                                                                     // behaviour
#define DCB_CD_USE_ATAPI_READCD 0x0040  // indicates that this is an atapi
                                                                                    // device that should use the readcd
                                                                                        // command instead of the read_10
                                                                                        // command

#define DCB_CD_CHANGER_DEVICE           0x0080  // indicates that this drive is
                                                                                    // part of a CD changer

#define DCB_CD_ISDVD                0x0100  // indicates that this drive is
                                            // a DVD device

#define DCB_CD_ISMMC                0x0200  // indicates an MMC 2 compliant
                                            // device

#define DCB_CD_ISAUDIO                          0x0400  // indicates an audio capable
                                                                                        // CD device

#define DCB_CD_ISDVD_MEDIA                      0x0800  // indicates DVD media present 

#define DCB_CD_ISDA     			0x1000	// indicates a CD device capable of reading
                                            // Redbook audio with READ-CD command
#define DCB_CD_ISMESN				0x2000	// indicates that a device supports the MESN
											// protocol
#define DCB_CD_ISMESN_BIT			13

/*
** define the algorithm indices for DCB_q_algo
*/

#define  DCB_q_fi_fo            0x00            /* first in, first out              */
#define  DCB_q_sort             0x01            /* special algorithm for disk       */

/*
** define the device type codes for use with DCB_device_type
*/

#define  DCB_type_disk           0x00   /* All Direct Access Devices -- non-removable */
#define  DCB_type_tape           0x01   /* Sequencial Access Devices        */
#define  DCB_type_printer        0x02   /* Printer Device                   */
#define  DCB_type_processor      0x03   /* Processor type device            */
#define  DCB_type_worm           0x04   /* Write Once Read Many Device      */
#define  DCB_type_cdrom          0x05   /* CD ROM Device                    */
#define  DCB_type_scanner        0x06   /* Scanner Device                   */
#define  DCB_type_optical_memory 0x07   /* some Optical disk                */
#define  DCB_type_changer        0x08   /* Changer device e.g. juke box     */
#define  DCB_type_comm           0x09   /* Communication devices            */
#define  DCB_type_floppy         0x0A   /* devices like floppy */
#ifdef  NEC_98
#define  DCB_type_optical_nec    0x84   /* NEC 5.25" optical disk           */
#endif

/*
** define Volume characteristics
*/


#define  DCB_INQ_DATA_LENGTH    sizeof DCB_inquiry_flags + sizeof DCB_vendor_id + sizeof DCB_product_id + sizeof DCB_rev_level

/*
** define the bus interface type codes for use with DCB_bus_type
*/

#define  DCB_BUS_ESDI           0x00    /* ESDI BUS                         */
#define  DCB_BUS_SCSI           0x01    /* SCSI BUS                         */
#define  DCB_BUS_NEC                    0x02    /* NEC BUS                          */
#define  DCB_BUS_SMART          0x03    /* SMART BUS                */
#define  DCB_BUS_ABIOS          0x04    /* ABIOS BUS                */

/*
** define the flags for use with DCB_vol_flags
*/

#define  DCB_VF_INHIBIT_LOCKING  0x01 /* Indicates locking is temporarily inhibited */
#define  DCB_VF_INHIBIT_IO              0x02 /* Indicates i/o is temporarily inhibited */
                                                                                                          /* indicates event sched in voltrk */
#define  DCB_VF_INHIBIT_GEOM_RECOMPUTE  0x04 /* indicates geometry recompute */
                                                                                     /* is temporarily inhibited */
#define  DCB_VF_INHIBIT_GEOM_RECOMPUTE_BIT 2
#define  DCB_VF_UNLOCK_SCHED_BIT                4    /* indicates unlock event scheduled */
#define  DCB_VF_UNLOCK_SCHED            (1 << DCB_VF_UNLOCK_SCHED_BIT)

#define  DCB_VF_NEED_PHYS_RECOMP_BIT    5       /* indicates that a mapper drive */
                                                                                    /* needs a compute geom sent to  */
                                                                                    /* physical drive with same # as */
                                                                                    /* the given logical                         */
#define  DCB_VF_NEED_PHYS_RECOMP        (1 << DCB_VF_NEED_PHYS_RECOMP_BIT)
#define  DCB_VF_PROT_NEC_DRIVE_BIT       6   /* indicates that this DCB has a */
                                             /* corresponding prot phys NEC driver*/
#define  DCB_VF_PROT_NEC_DRIVE          (1 << DCB_VF_PROT_NEC_DRIVE_BIT)




/*
** define the flags for use with DCB_tsd_flags
*/

#define  DCB_TSD_INVALID_PARTITION  1   /* Don't trust the BPB              */
#define  DCB_TSD_BAD_MBR            1   /* Bad MBR - physical device only   */

#define  DCB_TSD_USER_LETTERS_CHECKED_BIT  1    /* user drive letters checked */
#define         DCB_TSD_USER_LETTERS_CHECKED 1 << DCB_TSD_USER_LETTERS_CHECKED_BIT

#define  DCB_TSD_FIRST_USER_CHECK_BIT  2        /* user drive letters checked first time */
#define         DCB_TSD_FIRST_USER_CHECK 1 << DCB_TSD_FIRST_USER_CHECK_BIT

#define DCB_TSD_BID_SET_GEOM     0x0008 /* all geometry set by BID */

#define  DCB_TSD_USER_SET_BIT  4        /* user set the a drive letter range */
#define         DCB_TSD_USER_SET 1 << DCB_TSD_USER_SET_BIT

#define DCB_TSD_NO_USER_INT13    0x0020 /* user int 13 disabled */

#define DCB_TSD_PROTMODE_INT_13_BIT     6 /* protmode only int 13 */
#define DCB_TSD_PROTMODE_INT_13  1 << DCB_TSD_PROTMODE_INT_13_BIT 

#define DCB_TSD_NONEXIST_PARTITION_BIT  7 /* reserved drive letter */
#define DCB_TSD_NONEXIST_PARTITION       1 << DCB_TSD_NONEXIST_PARTITION_BIT

#define DCB_TSD_MBPB_PBR         0x0100 /* recommndd bpb set from pbr by tsd*/
                                        /* from int 41 or int 46            */

#define DCB_TSD_RBPB_INIT_BIT    0x09   /* bit shift for recommendd bpb initialized by tsd*/
#define DCB_TSD_RBPB_INIT               (1 << DCB_TSD_RBPB_INIT_BIT)
                                                                                               /* recommendd bpb initialized by tsd*/
#define DCB_TSD_MBPB_INIT        0x0400 /* media bpb initialized by tsd     */
#define DCB_TSD_APPARENT_PBR     0x0800 /* apparent set from primary pbr by */
#define DCB_TSD_APPARENT_ROM     0x1000 /* apparent set from rom data by tsd*/
#define DCB_TSD_ACTUAL_SET       0x2000 /* actual set from apparent by tsd  */
#define DCB_TSD_ACTUAL_PRE_SET   0x4000 /* actual geom set prior to tsd     */
#define DCB_TSD_APPARENT_PRE_SET 0x8000 /* apparent geom set prior to tsd   */
                                        /* tsd                              */

//      or      ds:[si.DCB_tsd_flags],DCB_TSD_APPARENT_PBR
//      or      ds:[si.DCB_tsd_flags],DCB_TSD_ROM_GEOM
//      or      ds:[si.DCB_tsd_flags],DCB_TSD_AUTO_GEOM

/*
** define the device characteristics flags for use with DCB_device_flags
*/                                                                                       

/* indicates drive supports spindown command */

#define DCB_DEV_SPINDOWN_SUPPORTED_BIT 31
#define DCB_DEV_SPINDOWN_SUPPORTED (1 << DCB_DEV_SPINDOWN_SUPPORTED_BIT)

/* indicates drive is currently spun down */

#define DCB_DEV_SPUN_DOWN_BIT 30
#define DCB_DEV_SPUN_DOWN       (1 << DCB_DEV_SPUN_DOWN_BIT)

/* port driver use only - indicates I/O has been received for this DCB */

#define DCB_DEV_IO_ACTIVE_BIT 29
#define DCB_DEV_IO_ACTIVE       (1 << DCB_DEV_IO_ACTIVE_BIT)

/* this device has been configured as an int 13 drive */

#define DCB_DEV_INT13_CONFIGURED_BIT 28
#define DCB_DEV_INT13_CONFIGURED (1 << DCB_DEV_INT13_CONFIGURED_BIT)

/* This DCB cannot handle ASPI or CAM requests */

#define DCB_DEV_NO_ASPICAM_BIT 27
#define DCB_DEV_NO_ASPICAM (1 << DCB_DEV_NO_ASPICAM_BIT)

/* this device is going away soon */

#define DCB_DEV_REMOVAL_PENDING_BIT 26
#define DCB_DEV_REMOVAL_PENDING (1 << DCB_DEV_REMOVAL_PENDING_BIT)

/* logical drives for this int 13 device are controlled by the mapper */

#define DCB_DEV_RMM_INT13_BIT 25
#define DCB_DEV_RMM_INT13 (1 << DCB_DEV_RMM_INT13_BIT)

/* this device has been processed by a TSD */

#define DCB_DEV_TSD_PROCESSED_BIT 24
#define DCB_DEV_TSD_PROCESSED   (1 << DCB_DEV_TSD_PROCESSED_BIT)

/* this is an equivlaent of IRS_DRV_SINGLE_FLOPPY. This indicates that */
/* the dmd_a_b_toggling is not set in the floppy dcb. I have defined this */
/* here as all the IRS flags are equivalents of the DCB_device_flags, lest */
/* it get used for some other purpose. */

#define DCB_DEV_SINGLE_FLOPPY   0x00800000

/* this device must be controlled by a real mode driver, and should not */
/* be taken over in protected mode */

#define DCB_DEV_REAL_MODE       0x00400000

/* current queing direction is descending - used by port drivers or IOS if */
/* DCB_SERIAL_CMD flag is set */

#define DCB_DEV_QUE_DESCEND     0x00200000

/* hardware indicated media change  */

#define DCB_DEV_MEDIA_CHANGED   0x00100000

/* bit indicates the disk geometry has become invalid. */

#define DCB_DEV_INVALID_GEOM    0x00080000

/* If this bit is set, it specifies not have the volume tracking layer do
 * the 2 second timing stuff for media change. This bit is typically set
 * if DCB_DEV_SYNC_MED_CHG_SUPPORT is set.
 */
#define DCB_DEV_INHIBIT_SW_TIMED_SUPPORT 0x00040000 /* drive is in read only mode */

/* drive has h/w support to indicate by interrupt that disk door has been
 * popped open, etc.
 */
#define DCB_DEV_ASYNC_MED_CHG_SUPPORT 0x00020000

/* drive has h/w sync med. chg supp. -- when u first access a drive with
 * media changed,  drive returns an error.
 */
#define DCB_DEV_SYNC_MED_CHG_SUPPORT 0x00010000

#define DCB_DEV_PHYSICAL        0x00008000      /* on = physical */
#define DCB_DEV_LOGICAL         0x00004000      /* on = logical */
#define DCB_DEV_LOGICAL_BIT     14

#define DCB_DEV_RMM             0x00002000      /* on = real mode mapper DCB*/
#define DCB_DEV_RMM_BIT 13

#define DCB_DEV_UNCERTAIN_MEDIA 0x00001000      /* media may have been changed  */
#define DCB_DEV_ROM_CFG_REQ     0x00000800      /* tsd must use rom chain data  */

#define DCB_DEV_MUST_CONFIGURE  0x00000400      /* set at create dcb time        */
                                                /* cleared after all layers */
                                                /* get the config aep for   */
                                                /* this DCB.                */

#define DCB_DEV_SSTOR_VOL       0x00000200      /* set if this is SSTOR volume */

#define DCB_DEV_PAGING_DEVICE   0x00000100  /* indicates that the paging drive */
                                                                                                        /* is on this DCB */

#define DCB_CHAR_DEVICE         0x00000080      /* indicates that this is a char dev*/
/*
 * The foll. DCB_DEV_I13_IN_PROGRESS flag is used in conjunction with RMM
 * and the global DCB. This flag is set when an int 13 in progress, in
 * iosint13.asm, and cleared when the int 13 is done.
 */

#define DCB_DEV_I13_IN_PROGRESS 0x00000040

#define DCB_DEV_CACHE           BDF_CACHE       /* = 0x00000020 */
#define DCB_DEV_SERIAL_CMD      BDF_Serial_Cmd  /* = 0x00000010 */
#define DCB_DEV_REMOTE          BDF_Remote      /* = 0x00000008 */
#define DCB_DEV_REMOVABLE       BDF_Removable   /* = 0x00000004 */
#define DCB_DEV_REMOVABLE_BIT 2
#define DCB_DEV_WRITEABLE       BDF_Writeable   /* = 0x00000002 */
#define DCB_DEV_INT13_DRIVE     BDF_Int13_Drive /* = 0x00000001 */

/*
** definitions of the DCB_device_flags2 field
*/

#define DCB_DEV2_THREAD_LOCK    0x00000001
#define DCB_DEV2_THREAD_LOCK_BIT        0

#define DCB_DEV2_ATAPI_DEVICE   0x00000002
#define DCB_DEV2_ATAPI_DEVICE_BIT       1

#define DCB_DEV2_DMF_DISK               0x00000004      /* indicates current media is DMF */
#define DCB_DEV2_DMF_DISK_BIT   2

#define DCB_DEV2_I13_COPY_PROT 0x00000008       /* indicates int 13 copy protection */
#define DCB_DEV2_I13_COPY_PROT_BIT 3            /* access to one of the floppies        */

#define DCB_DEV2_FAIL_FORMAT 0x00000010 /* indicates that we cannot allow format*/
#define DCB_DEV2_FAIL_FORMAT_BIT 4

#define DCB_DEV2_FLOPTICAL 0x00000020   /* indicates device is floptical */
#define DCB_DEV2_FLOPTICAL_BIT 5

#define DCB_DEV2_IDE_FLOPTICAL 0x00000040       /* indicates device is floptical */
#define DCB_DEV2_IDE_FLOPTICAL_BIT 6

#ifdef  NEC_98
#define DCB_DEV2_98_PARTITION 0x80000000
#define DCB_DEV2_98_PARTITION_BIT 31
#define DCB_DEV2_DISKX_VOL      0x40000000      /* for DISKX VOLUME */
#define DCB_DEV2_DISKX_VOL_BIT  30
#endif

/*
** define the demand bits for use with DCB_cd_flags
*/

#define DCB_dmd_srb_cdb         0x00000001      /* there must be an srb and cdb for */
                                        /* each ior.                        */
#define DCB_dmd_rsrv_1          0x00000002      /* reserved - must be zero.         */

#define DCB_dmd_logical         0x00000004      /* media address must be logical and*/
                                        /* dcb must be for a logical device.*/

#define DCB_dmd_physical        0x00000008      /* media adresses must be physical  */
                                        /* and dcb must be for a physical   */
                                        /* device.                          */

#define DCB_dmd_small_memory    0x00000010      /* data buffers must reside in      */
                                        /* in low 16M.                      */

#define DCB_dmd_rsrv_2          0x00000020      /* reserved - must be zero.         */

#define DCB_dmd_rsrv_3          0x00000040      /* reserved - must be zero.             */
#define DCB_dmd_rsrv_4          0x00000080      /* reserved - must be zero.         */

#define DCB_dmd_not_512         0x00000100      /* sector size on the media is not  */
                                        /* 512 bytes                        */

#define DCB_dmd_word_align      0x00000200      /* data buffers must be word        */
                                        /* aligned.                         */

#define DCB_dmd_dword_align     0x00000400      /* data buffers must be double word */
                                        /* aligned.                         */

#define DCB_dmd_phys_sgd        0x00000800      /* scatter/gather descriptors must  */
                                        /* contain physical addresses (they */
                                        /* will contain linear addresses if */
                                        /* this demand bit is not set).     */

#define DCB_dmd_phys_sgd_ptr    0x00001000      /* the pointer to the first         */
                                        /* scatter/gather descriptor        */
                                        /* (IOR_sgd_lin_phys) must be a     */
                                        /* physical address (it will be a   */
                                        /* linear addresses if this demand  */
                                        /* bit is not set).                 */

/* If foll. bit set, single floppy drive system -- voltrack needs to do
 * the toggling between A: and B: when accesses to made to these drive
 * letters.
 */
#define DCB_dmd_do_a_b_toggling         0x00002000

#define DCB_dmd_query_remov     0x00004000      /* lower level must be called to    */
                                        /* check if media is removeable/has */
                                        /* changed.                         */

#define DCB_dmd_request_sns      0x00008000      /* port driver cannot perform autosense  */
#define DCB_dmd_lock_unlock_media 0x00010000  /* media supports software locking */
#define DCB_dmd_load_eject_media 0x00020000  /* media supports electronic eject */
#define DCB_dmd_clear_media_chg 0x00040000  /* command to clear media change */
#define DCB_dmd_serialize_bit  19  /* layer requires request serialization */
#define DCB_dmd_serialize 1 << DCB_dmd_serialize_bit
#define DCB_dmd_prot_mode        0x00100000  /* override real mode checks for */
                                             /* this device i.e. ATAPI cdrom  */
#define DCB_dmd_no_xclusive      0x00200000  /* indicates exclusive access is */
                                             /* not required for abs write    */
#define DCB_dmd_pageability      0x00400000  /* indicates that access to this volume */
                                                                                                            /* can cause paging */

#define DCB_dmd_preload                  0x00800000  /* indicates that driver is preload */

#define DCB_dmd_rsrv    DCB_dmd_rsrv_1+DCB_dmd_rsrv_2+DCB_dmd_rsrv_3+DCB_dmd_rsrv_4

