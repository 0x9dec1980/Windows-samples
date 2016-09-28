;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: ACCESS.ASM
; *
; * This module contains the Direct Draw Initialization code for the 16-bit portion
; * and one Direct Draw callback DestroyDriver16. It also contains code to switch
; * back to the GDI context.
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
	.xlist
DOS5=1
	include glint.inc
	.list

;----------------------------------------------------------------------------
; D A T A
;----------------------------------------------------------------------------
_INIT   segment
	globalB 	DIBAccessCallFlag,0	;
_INIT   ends

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT   segment
	assumes cs,_TEXT

;----------------------------------------------------------------------------
; NearBeginAccess
;----------------------------------------------------------------------------
NearBeginAccess PROC NEAR C PUBLIC
	mov		bx, es						;store in registers rather than on the stack
	mov		dx, ds
    mov     ax,_INIT
    mov     es, ax
	assumes	es,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    es,ax
    test    GLInfo.GlintBoardStatus,GLINT_DMA_COMPLETE		; dma in progress?

    mov     ax,GLInfo.wRegSel				; then point at the Glint registers
    mov     ds,ax

    assumes ds,Glint
    jnz      NBANoDMA                    ; j if not doing a dma

	test	GLInfo.GlintBoardStatus,GLINT_INTR_CONTEXT
	jz		NBAWaitForNonIntDMA

; Wait for the DMA queue to empty
	push	edx
NBAWaitForIntDMA:
	mov		ecx, GLInfo.frontIndex
	mov		edx, GLInfo.backIndex
	cmp		ecx, edx 
	jnz		NBAWaitForIntDMA
	pop		edx

; Wait for the DMA count to hit zero
NBAWaitForNonIntDMA:
	mov		cx, word ptr DMACount
	test	cx,cx
	jz		clearDmaFlagAndCont
    loop    $							; wait in proportion to how much there is left

    jmp     NBAWaitForNonIntDMA             ; try again

clearDmaFlagAndCont:
	or		GLInfo.GlintBoardStatus, GLINT_DMA_COMPLETE	; clear the flag since it's over now.
NBANoDMA:
;;; "WaitFifo 2" won't work here since if the 2D side uses disconnect then the macro is empty!
	cmp     InFIFOSpace,2
	jb      NBANoDMA
    mov     FilterMode,00010000000000b ;enable tag out
    mov     Sync, 0
    mov     FilterMode,0                ;disable tag out
ifdef DEBUG
NBATestErrorFlags:
    mov     ecx, ErrorFlags
    and     ecx, NOT 2
    je      NBAWaitSync
    cmp     GLInfo.wRenderChipID, PERMEDIA2_ID
    je      @f
    cmp     GLInfo.wRenderChipID, TIPERMEDIA2_ID
    jne     NBAInterrupt
@@:
    ; On P2 we could get a fifo underrun error on first frame. This is OK.
    test    ecx, 010h
    je      NBAInterrupt
    test    GLInfo.dwFlags, GMVF_P2FIFOUNDERRUN
    jne     NBAInterrupt
    mov     ErrorFlags, 010h                ; Clear flag
    jmp     NBATestErrorFlags               ; Try again.

NBAInterrupt:
    int     1
    mov     ErrorFlags, ecx
endif
NBAWaitSync:
    cmp     OutFIFOWords, 1
    jb      NBAWaitSync
    cmp     GPFifo1, 188h
    jne     NBAWaitSync
	mov		ds, dx
	mov		es, bx
    assumes ds,nothing
    assumes es,nothing
    ret
NearBeginAccess ENDP
        
;----------------------------------------------------------------------------
; FarBeginAccess
;----------------------------------------------------------------------------
FarBeginAccess PROC FAR C PUBLIC
    call	NearBeginAccess
    ret
FarBeginAccess ENDP

;----------------------------------------------------------------------------
; BeginAccess(PDevice,Left,Top,Right,Bottom,aFlags)
;
; GRAB surface semaphore
;
; RELEASE surface semaphore
; RETURN(Flags)
; 
;----------------------------------------------------------------------------
BeginAccess PROC FAR PASCAL PUBLIC USES ds, lpDevice:DWORD, Left:WORD, Top:WORD,
                                    Right:WORD, Bottom:WORD, aFlags:WORD
    call    NearBeginAccess
	assumes ds,nothing
   	test	aFlags,CURSOREXCLUDE	;is this a request to exclude cursor?
	jz	    BAExit			;nope, don't do this
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
	cmp	    wCursorType,0   	;running with a software cursor?
	jne	    BAExit			;nope, no need to call DIB engine
	mov	    DIBAccessCallFlag,'A'	;set the DIBAccessCallFlag
	arg	    lpDevice		;call the DIB engine
	arg	    Left			;
	arg	    Top			;
	arg	    Right			;
	arg	    Bottom			;
	mov	    ax,aFlags		;
	and	    ax,CURSOREXCLUDE	;(only pass the CURSOREXCLUDE order)
	arg	    ax			;
	cCall	DIB_BeginAccess 	;
;
BAExit:
	mov	    ax,aFlags
    ret
BeginAccess ENDP
;----------------------------------------------------------------------------
; EndAccess(PDevice,aFlags)
;
; GRAB surface semaphore
;
; RETURN(aFlags)
;----------------------------------------------------------------------------
EndAccess PROC FAR PASCAL PUBLIC USES ds, lpDevice:DWORD, aFlags:WORD
.386
	assumes ds,nothing
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax

;
;We need to call the DIB Engine's EndAccess call to unexclude the cursor if
;we called its BeginAccess call:
;
	cmp	    DIBAccessCallFlag,0	;
	je	    EAExit			;nothing to do!
	test	aFlags,CURSOREXCLUDE	;Unexclude cursor?
	jz	    EAExit			;no
	push	lpDevice		;
	push	CURSOREXCLUDE		;
	call	DIB_EndAccess		;Let DIB Engine unexclude cursor.
	mov	    DIBAccessCallFlag,0	;clear the flag
;
EAExit:
	mov	    ax,aFlags
    ret
EndAccess ENDP

_TEXT   ends
end
