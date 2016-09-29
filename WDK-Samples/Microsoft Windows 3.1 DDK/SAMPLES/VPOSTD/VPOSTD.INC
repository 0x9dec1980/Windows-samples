;** vpostd.inc
;*
;*  DESCRIPTION:
;*      This include file contains misc. equates that VPOSTD uses.
;*
;*  HISTORY:
;*      2/27/91     cjp     wrote it
;*
;*
;** cjp *

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
