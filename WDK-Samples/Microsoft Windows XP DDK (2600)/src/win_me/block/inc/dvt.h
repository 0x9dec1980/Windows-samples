/*static char *SCCSID = "@(#)dvt.h   1.21 90/08/28";*/
/****************************************************************************\
*
*	Copyright (c) Microsoft Corporation 1990
*
\****************************************************************************/
/****************************************************************************\
*
*  dvt - driver vector table - ios creates one for itself (which is pointed to
*	 by the ivt and then one for each driver during driver registration.
*	 the dvt's for the drivers are all chained from ios's dvt.
*
*	 the dvt mostly contains useful pointers.
*
\****************************************************************************/

#define DVT_NAME_LEN		16
#define DVT_DATE_LEN		8
#define DVT_TIME_LEN		8
#define DVT_FILE_LEN		8
#define DVT_REV_LEN		4
#define DVT_FC_LEN		4
#define DVT_FNAME_LEN		12   /* file name of 8.3 format, no \0 */

//
//  NOTE: Any changes in this structure must also be reflected in cfr.h
//

typedef struct DVT { /* */

	USHORT	DVT_reserved_1;/* reserved for expansion to 32 bit	*/
	ULONG 	DVT_next_dvt;	/* 16-bit offset to next dvt			*/
	USHORT	DVT_device_cnt;/* count of devices added/accepted  */
	PVOID		DVT_aer;			/* Driver's Async Event Routine     */
	ULONG		DVT_ddb;			/* first ddb for this dvt				*/
	ULONG		DVT_ddb_init;	/* first ddb for this dvt/ice			*/

	/* note that iosreg.asm assumes that the following 12 fields*/
	/* remain in order					      							*/

	CHAR		DVT_ascii_name[DVT_NAME_LEN];
	CHAR		DVT_create_date[DVT_DATE_LEN];
	CHAR		DVT_create_time[DVT_TIME_LEN];
	CHAR		DVT_rev_level[DVT_REV_LEN];
	ULONG		DVT_feature_code;		/* feature code 		    				*/
	USHORT	DVT_if_requirements;	/* i/f Requirements		    			*/
	BYTE		DVT_bus_type;			/* I/O bus type							*/
	ULONG		DVT_reference_data;  /* data passed in DRP upon reg      */
	CHAR		DVT_first_drive;		/* unit number of first drive valid */
											/* only for block devices	    		*/
	CHAR		DVT_current_lgn;		/* current load group number 			*/
	ULONG		DVT_LoadHandle;		/* contains the Vxd's load handle if*/
											/* we loaded the driver else 0	   */
	CHAR		DVT_scsi_max_target; /* max target supported for SCSI		*/
	CHAR		DVT_scsi_max_lun;		/* max LUN supported for SCSI 		*/
	PVOID	   DVT_entry_point;     /* entry point into driver (NT MINIPORT ONLY) */
	UCHAR	   DVT_init_count;	  	/* # of successful AEP_INITIALIZE calls */
											/* (port drivers only)					*/
	PVOID		DVT_reserved[2];		/* reserved - MBZ							*/

} _DVT;

/*
** I/O Bus Types
*/

#define DVT_BT_ESDI		0x00  	/* ESDI or ESDI emulator */
#define DVT_BT_SCSI		0x01  	/* SCSI or SCSI emulator */
#define DVT_BT_FLOPPY	0x02  	/* NEC FLOPPY or FLOPPY emulator */
#define DVT_BT_SMART		0x03  	/* smart device */
#define DVT_BT_ABIOS		0x04  	/* ABIOS or ABIOS emulator */

/*
** Feature Code Definitions - must match DRP feature code definitions in drp.h
*/

#define DVT_FC_SCAN_DOWN   0x04    /*  on = bios scans targets from high to low*/
#define DVT_FC_IO_FOR_INQ_AEP 0x40 /*  on = PD needs to send I/O through IOP  */
										  	  /*  in response to an INQUIRY AEP.  Results */
										     /*  in CONFIGURE AEP for INQUIRY DCB.       */
         
#define DVT_FC_HALF_SEC    0x2000  /*  on = notify driver every half second    */
#define DVT_FC_1_SEC       0x2000  /*  on = notify driver every second         */
#define DVT_FC_2_SECS      0x4000  /*  on = notify driver every two seconds    */
#define DVT_FC_4_SECS      0x8000  /*  on = notify driver every four seconds   */
#define DVT_FC_DYNALOAD   0x10000  /*  on = driver was dynaloaded	by IOS    	 */
#define DVT_FC_NEED_PRELOAD   0x20000  /*  on = driver needs to hook I/O even  */
													/* before the port driver.  AEP_config_dcb */
                                       /* will be received before PD when set */
#define DVT_FC_NEED_PRE_POST_LOAD   0x40000  
													/* same as above, except that the drive */
													/* will receive 2 config_dcb calls for */
													/* each DCB, 1 before the port driver, */
													/* and 1 after layers before its load */
													/* group have been initialized.  Note */
													/* that care must be taken not to insert */
													/* TWICE in the same DCB.  				*/


