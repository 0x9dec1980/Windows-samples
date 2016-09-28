/*static char *SCCSID = "@(#)config.h	13.35 90/07/18";*/
/****************************************************************************\
*
*
*	All rights reserved		
*									
\****************************************************************************/
/****************************************************************************\
*
* Module Name: CONFIG.H - built in configurable variables
*
\****************************************************************************/

#ifdef WIN31COMPAT
#define MEM_POOL_PAGES	   3	/* initial size of ios memory pool  */
#else
#define MEM_POOL_PAGES	   2			/* initial size of ios memory pool */
#endif
#define BIG_MEM_POOL_PAGES	16 		/* size of ios memory pool for big mem */

#define IOS_DOS_PAGER_MEM_SUP PAGE_SIZE * 14
											 	/* extra memory we allocate if paging thru DOS */

#define MAX_POOL_PAGES	   10h		/* max pool size bugbug */

//#define MEM_POOL_SIZE	   (MEM_POOL_PAGES << 12) - sizeof ( IDA)

#define PAGE_SIZE		      0x1000	/* size of single page (doesn't belong) */
#define PAGE_SHIFT		   12			/* shift for single page (doesn't belong) */
#define MAX_SG_ELEMENT_SIZE 0xFFFF /* max size of single phys scatter/gather element */

/*#define DEFAULT_PORT		COM1_PORT /* default debug terminal com port*/
/*#define DEFAULT_PORT		COM2_PORT default debug terminal com port*/
/*#define DEFAULT_PORT		COM1_PORT default debug terminal com port*/
					/* values are COM1_PORT or COM2_PORT*/

#define MAX_LOGICAL_DRIVES	26	/* maximum number of logical block  */
					/* devices supported by ios	    */

#ifdef	NEC_98
#define MAX_PHYSICAL_DRIVES	48h     /* maximum number of physical block */
					/* devices supported by ios	    */
#else
#define MAX_PHYSICAL_DRIVES	24	/* maximum number of physical block */
					/* devices supported by ios	    */
#endif

#define MAX_REG_UNITS_PER_DEVICE	16	/* maximum number of units that will */
					/* be recorded in the registry per adapter */

#define MAX_PHYS_FLOPPY_DRIVES	8 /* maximun number of physical floppy*/
				    /* devices supported by IOS		*/

#define MAX_IRQ 		32	/* maximum number of interrupts     */
#define MAX_IRQ_LEVEL		31	/* maximum interrupt level (0 based)*/

#define IOS_LOG_NAME	    "IOSLOG$"	/* device driver header name for    */
					/* ios logging services		    */

#define	LOG_BUF_CNT		10	/* number of log buffers to use	    */

#define	BIG_MEM_THRESHOLD 0xFFFFFF	/* threshold for small memory   */
#define	DEFAULT_BIG_MEM_PAGES 16	/* default # pages (64 k)   */

#define	BIG_IO_SIZE	   0FFFFh  /* size of chunks processed by dbl bfr code */


