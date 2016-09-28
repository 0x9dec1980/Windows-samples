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

ifndef _SCSIPORT_H_
_SCSIPORT_H_	EQU	1
INCLUDE basedef.INC
SCSIPORT_Init_Order	EQU	<UNDEFINED_INIT_ORDER>

    SCSIPORT_VMAJ	EQU	3
    SCSIPORT_VMIN	EQU	1



PVT	STRUC
Port_export_tables	DD	?
Port_miniport_init	DD	?
PVT	ENDS
ifndef _H2INC

PORT_SRB	STRUC
BaseSrb	DB	SIZE SCSI_REQUEST_BLOCK DUP (?)
SrbIopPointer	DD	?
SrbNextSrb	DD	?
SrbNextActiveSrb	DD	?
SrbRetryCount	DB	?
Filler	DB	3 DUP (?)
PORT_SRB	ENDS
endif

PORT_SRB	STRUC
BaseSrb	DB	SIZE _SCSI_REQUEST_BLOCK DUP (?)
SrbIopPointer	DD	?
SrbNextSrb	DD	?
SrbNextActiveSrb	DD	?
SrbRetryCount	DB	?
Filler	DB	3 DUP (?)
PORT_SRB	ENDS


MAX_SCSITARGS	EQU	8
MAX_ADAPTER_BUSES	EQU	8
endif
SCSIPORT_GF_EMUL_SG	EQU	01H
