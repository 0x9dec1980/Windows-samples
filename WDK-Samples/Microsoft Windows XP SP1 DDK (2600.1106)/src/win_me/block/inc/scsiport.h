/******************************************************************************

    (C) Copyright MICROSOFT Corp., 1992

    Title:	scsiport.h

    Version:	1.00

    Date:	29-Sep-1992

 ------------------------------------------------------------------------------

    Change log:

     DATE    REV WHO             DESCRIPTION
  -------- ----- --- ----------------------------------------------------------
  09-28-92  1.00 FCF 

******************************************************************************/
	 		    
			   
#ifndef _SCSIPORT_H_
#define _SCSIPORT_H_

#include <basedef.h>	 

#define SCSIPORT_Init_Order	UNDEFINED_INIT_ORDER

/* ASM

    SCSIPORT_VMAJ	EQU	3
    SCSIPORT_VMIN	EQU	1

*/

typedef struct _PVT { /* */
	ULONG	Port_export_tables;
	ULONG	Port_miniport_init;
} PVT;


#ifndef _H2INC

typedef struct _PORT_SRB { /* */
	 SCSI_REQUEST_BLOCK  BaseSrb;
	 PVOID 		    SrbIopPointer;         // offset 64
	 SCSI_REQUEST_BLOCK *SrbNextSrb;	      // offset 68

//
// SrbNextActiveSrb allows us to keep a list of all active SRBs (SRBs that
// have been submitted to a miniport and are still under it's control).
//

	 SCSI_REQUEST_BLOCK *SrbNextActiveSrb;	// offset 72
	UCHAR 		    SrbRetryCount;			// offset 76
	UCHAR           Filler[3];				   // offset 77
} PORT_SRB;

#endif

/* ASM

PORT_SRB	STRUC
BaseSrb	DB	SIZE _SCSI_REQUEST_BLOCK DUP (?)
SrbIopPointer	DD	?
SrbNextSrb	DD	?
SrbNextActiveSrb	DD	?
SrbRetryCount	DB	?
Filler	DB	3 DUP (?)
PORT_SRB	ENDS

*/

//
// Maximum number of SCSI targets on a single bus
//

#define MAX_SCSITARGS		8
#define MAX_ADAPTER_BUSES  	8

#endif /* _SCSIPORT_H_ */


//
// ScsiportGlobalFlags definitions
//

#define SCSIPORT_GF_EMUL_SG 0x01	 // scatter/gather is emulated


