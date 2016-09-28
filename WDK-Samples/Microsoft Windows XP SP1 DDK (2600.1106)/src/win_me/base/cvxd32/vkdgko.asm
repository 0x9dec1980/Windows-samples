;****************************************************************************
;                                                                           *
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
; PURPOSE.                                                                  *
;                                                                           *
; Copyright 1993-95  Microsoft Corporation.  All Rights Reserved.           *
;                                                                           *
;****************************************************************************

include mylocal.inc

StartCDecl	VKD_Get_Kbd_Owner

		push	ebx
		VxDCall	VKD_Get_Kbd_Owner
		mov	eax, ebx
		pop	ebx
		ret

EndCDecl	VKD_Get_Kbd_Owner

END


