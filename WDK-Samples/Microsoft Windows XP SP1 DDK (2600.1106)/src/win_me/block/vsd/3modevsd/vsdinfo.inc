;	SCCSID = @(#)VSDINFO.INC	12.5 95/03/02
;;========================================================================;;
;; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY  ;;
;; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE    ;;
;; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR  ;;
;; PURPOSE.                                                               ;;
;;									  ;;
;;	Copyright (c) Microsoft Corporation 1990, 1995			  ;;
;;									  ;;
;;	All rights reserved						  ;;
;;									  ;;
;;========================================================================;;
;
;=========================================================================
; VSDINFO.INC
;
;
; Revisions
;   20-Mar-1995 [bent]	initial implementation
;
;=========================================================================	


;=========================================================================
;
;=========================================================================

PORT_StatusRegA 	    EQU      3f0h
PORT_StatusRegB 	    EQU      3f1h
PORT_OutputReg		    EQU      3f2h
PORT_StatusReg		    EQU      3f4h
PORT_DataReg		    EQU      3f5h
PORT_Input_And_XferRate     EQU      3f7h

;=========================================================================
;
; 3mode VSD signature string
;
; This string uniquely identified this driver, and must match the string
; stored in the .INF file which is written to the registry by the 3mode
; driver installer.
;
; This string must be 16 bytes long, in the format of 'xxxxxxxx 3MFD nn',
; where...
;
;   xxxxxxxx is the OEM name, eg, 'Compaq  '.  Unused positions should be
;   blank-padded.  Names longer than 8 characters must be truncated.
;
;   3MFD stands fo 3-Mode Floppy Driver, and must not be changed.
;
;   nn is the driver ID as a 2-digit hex number.
;
;=========================================================================

	VSD_NAME_LEN	EQU 16	    ; must not change

ifdef CPQ_1

	;
	; CPQ_1:  Deskpro series, Prolinea series except following
	; models..(?).	National's FDC family (8477, 87311, 8478, 87332,...)
	;

	VSD_NAME	EQU <'Compaq   3MFD 01'>
endif

ifdef CPQ_2

	;
	; CPQ_2:  Prolinea MT series.  SMC's FDC family (FDC37C661, FDC37C662)
	;

	VSD_NAME	EQU <'Compaq   3MFD 02'>
endif

ifdef CPQ_3

	;
	; CPQ_3:  Contura 400 series.  VLSI's FDC family (VL82C144-02, ...)
	;

	VSD_NAME	EQU <'Compaq   3MFD 03'>
endif

ifdef CPQ_4

	;
	; CPQ_4:  Prolinea III.  SMC FDC37C665
	;

	VSD_NAME	EQU <'Compaq   3MFD 04'>
endif

ifdef CPQ_x	;???

	;
	; CPQ_x:  Contura Aero series.	VLSI VL82C142
	;

	VSD_NAME	EQU <'Compaq   3MFD 0x'>
endif


;-------------------------------------------------------------------------
;
; 3Mode VSD driver registration packet (DRP) definitions
;
; Used in VSDINIT.ASM.	See DDK documentation for details.
;

	VSD_REV 	EQU 00H
	VSD_FEATURE	EQU 00H
	VSD_IF		EQU 00H

;-------------------------------------------------------------------------
;
; Standard 3Mode VSD driver version defintion
;

	VSD_MAJOR_VERSION   EQU     4
	VSD_MINOR_VERSION   EQU     0
