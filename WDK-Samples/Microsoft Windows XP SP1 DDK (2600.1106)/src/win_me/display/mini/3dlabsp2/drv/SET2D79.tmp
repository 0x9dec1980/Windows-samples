;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: DISABLE.ASM
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

	.xlist
	include glint.inc
	incDevice = 1
	include gdidefs.inc
    .list

;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------
STOP_IO_TRAP    = 4000h		        ; stop io trapping
START_IO_TRAP	= 4007h 		; re-start io trapping

;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT   segment
assumes cs,_TEXT
        .386
;----------------------------------------------------------------------------
; Disable
;   The display driver's physical disable routine is called to disable
;   graphics and enter a character mode of operation.
;----------------------------------------------------------------------------
cProc	Disable1,<FAR,PUBLIC,PASCAL,NODATA>,<esi,edi,es>
	parmD	lp_device
cBegin
    ; Disable all the bitmaps in the Offscreen Heap.
    invoke  DisableOffscreenHeap
        
	invoke  GLDD_Disable, lp_device

    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
    assumes es,nothing
    assumes gs,nothing
    assumes fs,nothing
    mov     ax,GLInfo.wRegSel
    mov     es,ax
    assumes es,Glint
	mov	VTGPolarity,0d5h		; enable VGA
	mov	wEnabled,0
	les	si,lp_device
lock	or	es:[si].deFlags,BUSY	;Set the BUSY bit
;
;Tell the VDD to start trapping the standard VGA ports again (such as the
;CRTC, Sequencer, and GCR registers).  This should be done before the change
;back to VGA text mode since the VDD needs to know about the text mode state
;that the Windows VM is going to be in after Disable is completed.
;

; only done for primary device
        cmp     GLInfo.dwDeviceHandle, 1
        jne     @f

	mov	ax,START_IO_TRAP
	int	2fh			;start i/o trapping
@@:

; 
; Tell MiniVDD we need the chip in VGA mode and remove interrupt
;
; I tried to do this in VDD_DRIVER_UNREGISTER (MiniVDD_DisplayDriverDisabling)
; but VDD_DRIVER_UNREGISTER doesn't call MiniVDD_DisplayDriverDisabling
; for secondary devices
	mov	eax,VDD_REGISTER_DISPLAY_DRIVER_INFO
	mov  	ebx,GLInfo.dwDeviceHandle 	;we need this for call
        mov     ecx, MINIVDD_ENABLEVGA
	call	VDDEntryPoint
;
;Call the VDD to inform it that we're disabling:
;
	mov	eax,VDD_DRIVER_UNREGISTER
					;pass the VDD API function code in EAX
	mov  	ebx,GLInfo.dwDeviceHandle 	;we need this for call
	call	VDDEntryPoint		;
;
;Now, set ourselves back to VGA text mode 3:
;
	mov	ax,3
	int	10h
;
;Unhook ourselves from the INT 2FH chain:
;
	call	RestoreInt2Fh
	mov	ax,-1			;Show success
cEnd
_TEXT   ends
end
