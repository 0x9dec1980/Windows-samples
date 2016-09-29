;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1992
;
;   Title:      VMIOD.INC
;
;   Version:    3.10
;
;   Date:       
;
;   Author:     
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV                 DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   10-Mar-92   bjm New device ID.
;==============================================================================

;
; This is the virtual device ID for this VxD.  It is a unique number assigned
; to this device.  For more information on VxD IDs, please see the VXDID.TXT
; file.
;
VMIOD_Dev_ID equ 7febh
