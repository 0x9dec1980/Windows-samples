/*static char *SCCSID = "@(#)scsidefs.h   1.18 90/07/18";*/
/****************************************************************************\
*
*	Copyright (c) Microsoft Corporation 1990		
*									
\****************************************************************************/
/****************************************************************************\
*
*	General SCSI Definitions
*
\****************************************************************************/

#define	CDB_6_byte		6	/* Length of 6 byte CDB	   	    */
#define	CDB_10_byte  		10	/* Length of 10 byte CDB   	    */
#define	CDB_12_byte  		12	/* Length of 12 byte CDB   	    */

#define	SCSI_Test_Unit_Ready	0x00	/* Test Unit Ready		    */
#define	SCSI_Rezero_Unit	0x01	/* CD-ROM rezero unit		    */
#define	SCSI_Rewind		0x01	/* Tape Rewind			    */
#define	SCSI_Request_Sense	0x03	/* Request Sense Command   	    */
#define	SCSI_Format_Unit	0x04  /* format for floppy				*/
#define	SCSI_Read_Blk_Limits	0x05	/* Read Block Limits  	   	    */
#define	SCSI_Req_Aux_Sense	0x06	/* Request Auxiliary Sense 	    */
#define	SCSI_Read_6		0x08	/* SCSI 6 byte Read	    	    */
#define	SCSI_Write_6		0x0A	/* SCSI 6 byte Write	    	    */
#define	SCSI_Write_Filemarks	0x10	/* Tape Write Filemarks	   	    */
#define	SCSI_Space		0x11	/* Tape Space		    	    */
#define	SCSI_Inquiry		0x12	/* Inquiry command	    	    */
#define	SCSI_Recover_Buffer	0x14 	/* Tape Recover Buffer		    */
#define	SCSI_Mode_Select	0x15	/* Mode Select		    	    */
#define	SCSI_Reserve_Unit	0x16	/* Tape Reserve Unit	    	    */
#define	SCSI_Release_Unit	0x17	/* Tape Release Unit	    	    */
#define	SCSI_Erase		0x19	/* Tape Erase		    	    */
#define	SCSI_Mode_Sense		0x1A	/* Mode Sense		    	    */
#define	SCSI_Start_Stop_Unit	0x1B	/* Start/Stop Unit		    */
#define	SCSI_Load_Unload	0x1B	/* Device Load/Unload Media  	    */
#define	SCSI_Lock_Unlock	0x1E	/* Lock/Unlock drive door	    */
#define	SCSI_Read_Fmt_Cap   0x23	/* Read Capacity	    	    */
#define	SCSI_Read_Capacity	0x25	/* Read Capacity	    	    */
#define  SCSI_Read_10		0x28	/* SCSI 10 byte Read 	   	    */
#define	SCSI_Write_10		0x2A	/* SCSI 10 byte Write 	   	    */
#define	SCSI_Seek_10		0x2B	/* SCSI 10 byte Seek		    */
#define	SCSI_Locate		0x2B	/* Tape Locate		    	    */
#define	SCSI_Write_Verify_10	0x2E	/* SCSI 10 byte Write w/Verify	    */
#define	SCSI_Verify_10		0x2F	/* SCSI 10 byte Verify 	   	    */
#define	SCSI_Read_Sub_Chan	0x42	/* Read Sub-Channel (CD-ROM)	    */
#define	SCSI_Read_TOC		0x43	/* Read Table of Contents	    */
#define	SCSI_Play_MSF		0x47	/* Play Audio - MSF format	    */
#define	SCSI_Pause_Resume	0x4B	/* Pause/Resume Audio Play	    */

/*
** SCSI Status Codes
*/

#define	SCSI_CHECK_CONDITION	0x02	/* SCSI Check condition	    	    */
#define	SCSI_TARGET_BUSY	0x08	/* SCSI Busy status	    	    */
#define  SCSI_GOOD_STATUS	 	0x00	/* no error reported		    */
#define  SCSI_CONDITION_MET   	 0x04    /* Condtion met, Good status	    */
#define  SCSI_INTERMEDIATE_GOOD 	0x14   	/* Good status during likned CDBs */
#define  SCSI_RESERVATION_CONFLICT 	0x18 		/* Reservation conflict		    */
#define  SCSI_QUEUE_FULL		0x28	/* Queue for the device is full	    */

/*
** sense key values on check condition - note that this field is
** only the low nibble of the byte
*/

#define SCSI_SENSE_KEY_MASK	0x0f	/* mask to extract sense key nibble */

#define	SCSI_SENSE_NO_SENSE	0x00	/* No error occured		    */
#define	SCSI_RECOVERED_ERROR	0x01	/* Recovered Error	    	    */
#define	SCSI_NOT_READY		0x02	/* Device not ready		    */
#define	SCSI_MEDIUM_ERROR	0x03	/* Medium error detected	    */
#define	SCSI_HARDWARE_ERROR	0x04	/* Hardware error detected	    */
#define	SCSI_ILLEGAL_REQUEST	0x05	/* Illegal request	    	    */
#define	SCSI_UNIT_ATTENTION	0x06	/* Unit Attention	    	    */
#define	SCSI_ABORTED_ERROR	0x0B	/* Aborted error detected 	    */

typedef	struct	Request_Sense { /* */
BYTE		RSB_ErrCode;  				/* Error code 70h or 71h */
BYTE 		RSB_Segment;    			/* Segment Number */
BYTE 		RSB_Sense_key;     		/* Sense Key */
ULONG 	RSB_Information;			/* Information Byte */
BYTE 		RSB_Add_sns_len;			/* Addtional Sense Length */
ULONG 	RSB_Cmd_specific_info;	/* Command specific information */
BYTE		RSB_Add_sns_code;			/* Addtional Sense Code */
BYTE		RSB_Add_sns_qualifier;	/* Additional Sense qualifier */
BYTE		RSB_FRU_Code;				/* Field Replaceable Unit Code */
USHORT	RSB_Sns_key_Specific;	/* Sense Key Specific */
BYTE		RSB_Addtl_Sense_Bytes;  /* Additional sense info */
} far *pRequest_Sense;

/* Additional sense code definitions follow */

#define	SSC_LUN_NOT_READY 0x04  		/* LUN is not ready */
#define SSC_WRONG_FORMAT  0x30          /* incompatbile format */

/* sense code qualifers for LUN NOT READY follow */

#ifdef  NEC_98
  #ifdef        MO
        #define SSQ_MAN_INTERV_REQUIRED 0x00  /* Manual intervention    required */
        #define SSQ_FORMAT_IN_PROG      0x00  /* Format in progress */
  #else
        #define SSQ_MAN_INTERV_REQUIRED 0x03  /* Manual intervention    required */
        #define SSQ_FORMAT_IN_PROG      0x04  /* Format in progress */
  #endif;       MO
#else
#define	SSQ_MAN_INTERV_REQUIRED 0x03  /* Manual intervention required */
#define	SSQ_FORMAT_IN_PROG      0x04  /* Format in progress */
#endif; NEC_98

/*
** Structure overlay for first 8 bytes of inquiry data
*/

typedef	struct	InqFlags { /* */

	BYTE	INQ_Dev_Type; 		/* device type and peripheral qualifier */
	BYTE	INQ_Dev_Type_Mod;	/* removable media and device type modifier */
	BYTE	INQ_Version;		/* device version */
	BYTE	INQ_Response_Data;	/* AEN, TRMIOP, and response data */
	BYTE	INQ_Additional_Len;	/* additional inquiry length (n-4) */
	BYTE	INQ_Reserved_1;		/* reserved - MBZ */
	BYTE	INQ_Reserved_2;		/* reserved - MBZ */
	BYTE	INQ_Dev_Flags;		/* device flags - see below */

} far *pInqFlags;

/*
** Inquiry type modifier flags
*/
#define	INQ_mod_removable 0x80 /* device has removable media */

/*
** Inquiry device flag definitions
*/

#define	INQ_Rel_Adr			0x80;	/* device supports relative addressing */
#define	INQ_Wide_32			0x40;	/* device supports 32 bit data xfers */
#define	INQ_Wide_16			0x20;	/* device supports 16 bit data xfers */
#define	INQ_Synch_Supp			0x10;	/* device supports synchronous xfer */
#define	INQ_Linked_Cmd			0x08;	/* device supports linked commands */
#define	INQ_Cmd_Queue			0x02;	/* device supports command queueing */
#define	INQ_Sft_Reset			0x01;	/* device supports soft resets */



/* Note that the following are 5 bit values in the enquiry info 1st byte */

/*
** Standard SCSI definition types
 */

/* Here we use DASD for non-removable rotating media as we have a separate
 * entry for floppies in our DRAGON model.
 */
#define SCSI_DASD_TYPE				0x00
#define SCSI_SEQUENTIAL_TYPE		0x01
#define SCSI_PRINTER_TYPE			0x02
#define SCSI_PROCESSOR_TYPE		0x03
#define SCSI_WORM_TYPE				0x04
#define SCSI_CDROM_TYPE				0x05
#define SCSI_SCANNER_TYPE			0x06
#define SCSI_OPTICAL_TYPE			0x07
#define SCSI_MEDIACHANGER_TYPE	0x08
#define SCSI_COMMUNICATIONS_TYPE	0x09
#define SCSI_UNKNOWN_TYPE			0x1F


/* we start number from high to low -- 1Fh means unknown, so we use 1Eh as
**	our first type.
 */
/*
** DRAGON SCSI definition types
 */

#define SCSI_FLOPPY_TYPE			0x1E
#define REAL_MAPPER_TYPE			0x1D

#ifdef	NEC_98
#define SCSI_OPTICAL_NEC_TYPE			0x1C
#endif
