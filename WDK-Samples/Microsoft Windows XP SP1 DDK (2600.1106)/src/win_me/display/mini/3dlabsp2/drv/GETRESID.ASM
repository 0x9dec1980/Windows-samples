;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: GETRESID.ASM
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

	.xlist
	include glint.inc
	.list
;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT   segment
assumes cs,_TEXT
;----------------------------------------------------------------------------
; GetDriverResourceID
;----------------------------------------------------------------------------
cProc	GetDriverResourceID,<FAR,WIN,PASCAL,PUBLIC>
        parmW  wResID
        parmD  lpResType
cBegin
	assumes	ds,_INIT
	mov	ax,wResID		;Get res id into ax.
	cmp	ax,3			;Is it the fonts resource?
	jne	short @f		;no. leave it alone.
        cmp     wDpi,96			;is dpi = 96?
	je	short @f		;yes. leave it alone.
	mov	ax,2003			;no. remap it to 2003.
@@:	xor	dx,dx		        ;dx must be zero.                
cEnd GetDriverResourceID
_TEXT   ends
end
