; ******************************Module*Header**********************************\
; *
; *                           ***********************************
; *                           * Permedia 2:   SAMPLE CODE       *
; *                           ***********************************
; *
; * Module Name: contxt16.asm
; *
; * This module contains the context switching code. 
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
;----------------------------------------------------------------------------
    .xlist
    DOS5 = 1			;so we don't get INC BP in <cBegin>
    include glint.inc
    .list

;----------------------------------------------------------------------------
;			       GLINT
;----------------------------------------------------------------------------
_TEXT segment
assumes cs,_TEXT


;----------------------------------------------------------------------------
;			       Context_ReadRegisters
;   es:si   -   Template pointer
;   fs:0    -   Glint register pointer
;   gs:di   -   Saved Registers pointer
;----------------------------------------------------------------------------
cProc   Context_ReadRegisters,<NEAR,PUBLIC,PASCAL>
cBegin
.386
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,Glint
    assumes gs,nothing

    xor     eax, eax
    mov     cx, es:[si]             ; Get count of registers
    add     si, 2                   ; es:si points to first register

@@:
    mov     ax, es:[si]             ; Get register offset
    mov     ebx, fs:[eax]           ; Get the value of the register
    mov     gs:[di], ebx            ; Save register
    add     si, 2    
    add     di, 4
    loop    @b
cEnd

;----------------------------------------------------------------------------
;			       Context_WriteRegisters
;   es:si   -   Template pointer
;   fs:0    -   Glint register pointer
;   gs:di   -   Saved Registers pointer
;----------------------------------------------------------------------------
cProc   Context_WriteRegisters,<NEAR,PUBLIC,PASCAL>
cBegin
.386
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,Glint
    assumes gs,nothing

    xor     eax, eax
    mov     cx, es:[si]             ; Get count of registers
    add     si, 2                   ; es:si points to first register

@@:
    mov     ebx, gs:[di]            ; Get saved register
    mov     ax, es:[si]             ; Get register offset
    mov     fs:[eax], ebx           ; Write the value of the register
    add     si, 2    
    add     di, 4
    loop    @b
cEnd

;----------------------------------------------------------------------------
;			       Context_Locate
;       ds      -   _INIT Segment
;       ax      -   Context/Template handle
;   returns:
;       es:si   -   Context/Template pointer
;       ax      -   Template handle
;----------------------------------------------------------------------------
cProc   Context_Locate,<NEAR,PUBLIC,PASCAL>
cBegin
.386
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,Glint
    assumes gs,nothing

    mov     bx, GLInfo.pContext16   ; Get the selector for the context
    mov     es, bx
    xor     di, di


    cmp     ax, MAX_CONTEXTS_IN_BLOCK
    jb      CL_FoundContextBlock
@@:
    sub     ax, MAX_CONTEXTS_IN_BLOCK
    mov     bx, es:[di].pNextContext16
    mov     es, bx
    cmp     ax, MAX_CONTEXTS_IN_BLOCK
    jae     @b
CL_FoundContextBlock:
    mov     di, ax                  ; Convert ax into words
    add     di, di
    mov     ax, es:[di].CTemplate   ; Get template ID
    mov     si, es:[di].COffset     ; Get Offset
cEnd

;----------------------------------------------------------------------------
;			       Context_Change
;   ax contains the context handle
;----------------------------------------------------------------------------
Context_Change  PROC NEAR C PUBLIC USES si di
; sanity check.. just in case

        push    ds
        mov     bx,_INIT
        mov     ds, bx
	    assumes	ds,_INIT
	    mov	    bx,WORD PTR RemappedDataSegment
	    mov	    ds,bx
        push    es                      ; save seg regs
        push    fs
        push    gs
        push    ax                      ; save new context

        mov     DD_CurrentContext, DD_UNDEFINED_CTXT

; save the current context if required

        mov     ax,GLInfo.wRegSel       ; selector for glint
        mov     fs,ax                   ; to fs
        assumes fs,Glint

        call    NearBeginAccess             ; wait for dma etc. (sync is done)

; Restore the new context
        pop     ax                      ; Get the handle
        mov     WORD PTR GLInfo.dwCurrentContext,ax ; Save away new current context handle 

        cmp     ax, 0ffffh              ; Do we want to switch into a context?
        je      DontSwitch

		invoke  RestoreGDIContext	;  
		call    NearBeginAccess             ; wait for dma etc. (sync is done)

DontSwitch:
        pop     gs
        pop     fs
        pop     es
        pop     ds
        ret

Context_Change  ENDP

;----------------------------------------------------------------------------
;			       Context_Change
;   ax contains the context handle
;----------------------------------------------------------------------------
Far_Context_Change  PROC FAR C PUBLIC
    call Context_Change
    ret
Far_Context_Change  ENDP

_TEXT   ends
 end
