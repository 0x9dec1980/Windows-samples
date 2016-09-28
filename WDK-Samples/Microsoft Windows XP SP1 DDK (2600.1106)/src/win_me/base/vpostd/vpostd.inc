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

;** vpostd.inc
;*
;*  DESCRIPTION:
;*      This include file contains misc. equates that VPOSTD uses.
;*

VPOSTD_VERMAJ   equ     3
VPOSTD_VERMIN   equ     0

;
; This is the virtual device ID for this VxD.  It is a unique number assigned
; to this device.  For more information on VxD IDs, please see the VXDID.TXT
; file.
;
VPOSTD_DEV_ID   equ     7fe8h

;;  This is our Control Block (CB) structure; one per VM.

VPOSTD_CB_DATA STRUC

VPOSTD_CB_Client_DX     dw  ?
VPOSTD_CB_Client_CX     dw  ?

VPOSTD_CB_DATA ENDS


;** EOF: vpostd.inc **
