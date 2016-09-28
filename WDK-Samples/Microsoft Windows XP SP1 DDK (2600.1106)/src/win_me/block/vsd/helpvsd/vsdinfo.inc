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

;=========================================================================
; VSDINFO.INC
;
;=========================================================================	

; VSD Info Equates
	HelpVSDName 	EQU	<'SCSI1 CDHELP VSD'> 	;Must be 16 chars
	HelpVSDRev	EQU	00H
	HelpVSDFeature	EQU	00H			
	HelpVSD_IF	EQU	00H

; Helper VSD version equates

HELPVSD_MAJOR_VERSION	EQU	4
HELPVSD_MINOR_VERSION	EQU	0

; Strings for company identification

OUR_COMPANY_NAME_LO	equ	' CEN'		; NEC designation
OUR_COMPANY_NAME_HI	equ	'    '		; trailing spaces
