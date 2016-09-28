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

;
; The default FSD will allocate a RESTAB for each mount if one already does
; not exist for this drive and VRP.
;
RESTAB    struc

  Res_Chain     db  (size DCHDR) dup (?); doubly linked list resources
  Res_Vrp       dd  ?        ; pointer to volume request parameters
  Res_VMHandle  dd  ?        ; current vm handle
  Res_Drive     db  ?        ; drive ID, 0=A
  Res_Lock      db  ?
  Res_ErrCode   dw  ?        ; error code to be returned

RESTAB    ends

DFS_RESOURCE_LOCKED     equ     1
