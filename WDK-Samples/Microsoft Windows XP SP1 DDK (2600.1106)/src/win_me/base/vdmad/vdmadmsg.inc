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

;******************************************************************************
;
;   Title:      vdmadmsg.inc
;
;   Version:	1.00
;
;==============================================================================


VxD_MESSAGE_SEG

BeginMsg
;
; These 2 messages are for the same error.  The error occurs when a requested
; DMA transfer that needs to be buffered exceeds the allocated size of the
; DMA buffer.  The 1st message is given for DOS VM's and the 2nd is given as
; an exit message, if a Windows application is the one attempting a DMA
; operation.
;
SQZD_MSG_START <MSG_Buffer_Too_Small>
SQZD_MSG <"The DMA buffer is too small.", 13, 10>
SQZD_MSG <'Set "DMABUFFERSIZE=%u" in SYSTEM.INI in the [386Enh] section', 13, 10, 13, 10>
SQZD_MSG <"and restart your computer.", 0>
SQZD_MSG_END   <MSG_Buffer_Too_Small>

; SQZD_MSG_START <MSG_Fatal_Buffer_Too_Small>
; SQZD_MSG <"The DMA buffer is too small.", 13, 10>
; SQZD_MSG <'Set "DMABUFFERSIZE=%u" in SYSTEM.INI in the [386Enh] section', 13, 10>
; SQZD_MSG <" in SYSTEM.INI in the [386Enh] section", 13, 10>
; SQZD_MSG <"and restart your computer.", 0>
; SQZD_MSG_END <MSG_Fatal_Buffer_Too_Small>

EndMsg

VxD_MESSAGE_ENDS
