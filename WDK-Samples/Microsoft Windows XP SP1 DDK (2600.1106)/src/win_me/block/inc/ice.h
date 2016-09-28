/*static char *SCCSID = "@(#)ICE.H   1.30 90/08/28";*/
/****************************************************************************\
*
*	Copyright (c) Microsoft Corporation 1990		
*									
\****************************************************************************/
/****************************************************************************\
*
*
* ICE (I/O configuration entry)
*
*
\****************************************************************************/

/*
** record type for use with ICE_type
*/

#define	ICE_TYPE_DRIVER	0	/* entry for drivers			    */
#define	ICE_TYPE_MAP	1	/* entry which maps devices to vsd's        */
#define	ICE_TYPE_DEVICE	2	/* entry for devices which must be present  */

/*
** LADDR layer for use with ICE_layer
*/

#define	ICE_LAYER_TSD	0	/* driver is in type specific layer	    */
#define	ICE_LAYER_VSD_1	1	/* driver is in vendor enhancement layer 1  */
#define	ICE_LAYER_VSD_2	2	/* driver is in vendor enhancement layer 2  */
#define	ICE_LAYER_VSD_3	3	/* driver is in vendor enhancement layer 3  */
#define	ICE_LAYER_VSD_4	4	/* driver is in vendor enhancement layer 4  */
#define	ICE_LAYER_VSD_5	5	/* driver is in vendor enhancement layer 5  */
#define	ICE_LAYER_VSD_6	6	/* driver is in vendor enhancement layer 6  */
#define	ICE_LAYER_VSD_7	7	/* driver is in vendor enhancement layer 7  */
#define	ICE_LAYER_VSD_8	8	/* driver is in vendor enhancement layer 8  */
#define	ICE_LAYER_VSD_9	9	/* driver is in vendor enhancement layer 9  */
#define	ICE_LAYER_PSD	10	/* driver is in path selection layer	    */
#define	ICE_LAYER_BID	11	/* driver is in  bus interface layer	    */

/*
** platform mask for use with ICE_platform
*/

#define	ICE_ISA		0x0001	/* this entry applies to isa platforms	    */
#define	ICE_EISA	0x0002	/* this entry applies to eisa platforms	    */
#define	ICE_MCA		0x0004	/* this entry applies to mca platforms	    */

/*
** define the config table header in front of the ice's
*/

typedef struct ICE_header { /* */

	ULONG	ICE_table_len;		/* config table size including hdr  */

} _ICE_HEADER, *pICE_HEADER, far *pfICE_HEADER;

/*
** define the basic configuration entry
*/

typedef struct ICE { /* */

	USHORT	ICE_len;		/* length of this entry		    */
	USHORT	ICE_type;		/* entry type - see defines above   */
	USHORT	ICE_platform;		/* platform (machine) type mask	    */

} _ICE, *pICE, far *pfICE;

/*
** define the configuration entry for drivers
*/

typedef struct ICE_driver { /* */

	USHORT	ICE_filler_1;		/* length of this record	    */
	USHORT	ICE_filler_2;		/* record type - "ICE_TYPE_DRIVER"  */
	USHORT	ICE_filler_3;		/* platform (machine) type mask	    */
	CHAR	ICE_drv_ascii_name[DVT_NAME_LEN];	
	CHAR	ICE_drv_create_date[DVT_DATE_LEN];		
	CHAR	ICE_drv_create_time[DVT_TIME_LEN];		
	CHAR	ICE_drv_rev_level[DVT_REV_LEN];			
	CHAR	ICE_drv_file_name[DVT_FNAME_LEN];
	USHORT	ICE_drv_layer;		/* LADDR layer			    */
	CHAR	ICE_drv_length;		/* option str length (includes null)*/
	CHAR	ICE_drv_option[64];	/* option string		    */

} _ICE_driver, *pICE_driver, far *pfICE_driver;

/*
** define the configuration entry which maps devices to vsd's
*/

typedef struct ICE_map { /* */

	USHORT	ICE_filler_4;		/* length of this record	    */
	USHORT	ICE_filler_5;		/* record type - "ICE_TYPE_MAP"	*/
	USHORT	ICE_filler_6;		/* platform (machine) type mask	    */
	BYTE	ICE_map_device_name[6];	/* ascii name of device		    */
	BYTE	ICE_map_inquiry_flags[8]; /* Device Inquiry Flags	    */
	BYTE	ICE_map_vendor_id[8];	/* Vendor ID string		    */
	BYTE	ICE_map_product_id[16];	/* Product ID string		    */
	BYTE	ICE_map_dev_rev_level[4];	/* Product revision level	    */
	BYTE   	ICE_map_device_type;    /* Device Type			    */
	CHAR	ICE_map_ascii_name[DVT_NAME_LEN];	
	CHAR	ICE_map_create_date[DVT_DATE_LEN];		
	CHAR	ICE_map_create_time[DVT_TIME_LEN];		
	CHAR	ICE_map_drv_rev_level[DVT_REV_LEN];			

} _ICE_map, *pICE_map, far *pfICE_map;

/*
** define the configuration entry for devices which must be present
*/

typedef struct ICE_device { /* */

	USHORT	ICE_filler_7;		/* length of this record	    */
	USHORT	ICE_filler_8;		/* record type - "ICE_TYPE_DEVICE"  */
	USHORT	ICE_filler_9;		/* platform (machine) type mask	    */
	BYTE	ICE_dev_device_name[6];	/* ascii name of device		    */
	BYTE	ICE_dev_inquiry_flags[8]; /* Device Inquiry Flags	    */
	BYTE	ICE_dev_vendor_id[8];	/* Vendor ID string		    */
	BYTE	ICE_dev_product_id[16];	/* Product ID string		    */
	BYTE	ICE_dev_rev_level[4];	/* Product revision level	    */
	BYTE   	ICE_dev_device_type;    /* Device Type			    */

} _ICE_device, *pICE_device, far *pfICE_device;


/*
** 'Eye catchers' for the begging and end of IOCONFIG table
*/

#define IOCONFIG_TBL_START	"$$$IOCONFIG$$start$$of$$table$$$"
#define IOCONFIG_TBL_END	"$$$IOCONFIG$$$end$$of$$$table$$$"
