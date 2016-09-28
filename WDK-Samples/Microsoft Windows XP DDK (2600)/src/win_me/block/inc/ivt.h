/*static char *SCCSID = "@(#)ivt.h   1.19 90/08/20";*/
/****************************************************************************\
*
*	Copyright (c) Microsoft Corporation 1990		
*									
\****************************************************************************/
/****************************************************************************\
*
*  ivt - ios vector table - contained in ios's code segment and provides
*			various usefull pointers
*
\****************************************************************************/

typedef struct IVT { /* */

	ULONG	IVT_ios_mem_virtual;	/* virtual addr of ios memory pool	    */
	PVOID	IVT_ios_mem_phys;	/* physical addr of ios memory pool */
	ULONG	IVT_first_dvt;		/* 32-bit offset of first dvt	    */	

} _IVT;

