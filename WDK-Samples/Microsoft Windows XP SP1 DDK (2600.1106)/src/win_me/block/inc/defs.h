/*static char *SCCSID = "@(#)defs.h	13.33 90/07/18";*/
/****************************************************************************\
*
*	Copyright (c) Microsoft Corporation 1990		
*									
\****************************************************************************/
/****************************************************************************\
*
* Module Name: DEFS.H - Common Definitions file
*
\****************************************************************************/

/* ascii terminal (ie debug terminal) control characters		   */

#define	BELL		0x07		/* bell character.		   */
#define	LINE_FEED	0x0a		/* line feed character.		   */
#define	CAR_RET		0x0d		/* carriage return character.	   */

/* debug terminal comm port addresses													*/

#define	COM1_PORT	0x03f8		/* com1's i/o address		   */
#define	COM2_PORT	0x02f8		/* com2's i/o address		   */

/* various constants for use with 8250 type uart's			   */

#define	COM_DAT		0x00		/*				   										*/
#define	COM_IEN		0x01		/* interrupt enable		   */
#define	COM_IER		0x02		/* interrupt ID			   */
#define	COM_LCR		0x03		/* line control registers	   */
#define	COM_MCR		0x04		/* modem control register	   */
#define	COM_LSR		0x05		/* line status register		   */
#define	COM_MSR		0x06		/* modem status regiser		   */
#define	COM_DLL		0x00		/* divisor latch least sig	   */
#define	COM_DLM		0x01		/* divisor latch most sig	   */

/* structure to define hibyte and lobyte within a word			   */

typedef struct w_s { /* */
	UCHAR		lobyte;
	UCHAR		hibyte;
};

/* structure to define hiword and loword within a double word		   */

typedef struct dw_s { /* */
	USHORT	loword;
	USHORT	hiword;
};

typedef struct dw_ss { /* */
	USHORT	lo;
	USHORT	hi;
};

typedef struct dd_s { /* */
	ULONG		lodword;
	ULONG		hidword;
};

/* structure to define offst16 and segmt within a 16:16 far pointer	   */

typedef struct	FarPtr { /* */
 	USHORT	offst16;
	USHORT	segmt;
};

/* structure to define offst and segmt32 within a 16:32 far pointer	   */

typedef struct FarPtr32 { /* */
	ULONG		offst;
	USHORT	segmt32;
	USHORT	pad32;
};

