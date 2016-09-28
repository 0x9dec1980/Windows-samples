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

StartCDecl VWIN32_DIOCCompletionRoutine@4

        mov ebx, [esp+4] ; Get hEvent
        VxDCall VWIN32_DIOCCompletionRoutine
        ret 4

EndCDecl VWIN32_DIOCCompletionRoutine@4

END



