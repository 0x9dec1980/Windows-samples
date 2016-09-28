;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: sswitch.asm
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
; E Q U A T E S
;----------------------------------------------------------------------------
STOP_IO_TRAP      equ   4000h           ;Stop io trapping
ENTER_CRIT_REG    equ   4003h           ;Enter Critical Section notification.
EXIT_CRIT_REG     equ   4004h           ;Exit Critical Section notification.
START_IO_TRAP     equ   4007h           ;Restart io trapping
REPAINT_EXPORT_INDEX    equ     275
PREVENT_SWITCH    equ   10000000b       ;Don't allow switch (DOS 3.x, 4.x)
INT_2F_HOOKED     equ   00000001b       ;Have hooked int 2Fh
FLAGS_ON_STACK    equ   4               ;iret --> offset(0), seg(2), flags(4)
CARRY_FLAG        equ   00000001b
DEVICECOLORMATCH_EXPORT_INDEX   equ     449

;----------------------------------------------------------------------------
; D A T A
;----------------------------------------------------------------------------
_INIT segment

globalW DeviceFlags,0

SwitchTable     label   word
    ; Int 2F (AX value: handler function) pairs
    dw      4001h,  DevToBackground
    dw      4002h,  DevToForeground
SwitchTableSize equ     2               ; UPDATE this if more entries are added

SwitchControl   db      0               ;Switch control flags
;----------------------------------------------------------------------------
; Repaint variables.
;----------------------------------------------------------------------------
bRepaintDisable db      0h              ;ok to call user to repaint
RepaintPending  db      0               ;repaint is pending.
public RepaintAddr
RepaintAddr     dd      0
szUSER          db      'USER',0

public GetColorAddr
GetColorAddr    dd      0
szGDI           db      'GDI',0

_INIT   ends
;----------------------------------------------------------------------------
; C O D E
;----------------------------------------------------------------------------
_TEXT   segment
assumes cs,_TEXT
        .386
PrevInt2Fh	dd	0		;Previous int 2Fh vector

;----------------------------------------------------------------------------
; UserRepaintDisable
;
; USER calls this function to tell the display driver whether it expects
; the repaint call to be postponed or not.
;
; If RepaintDisable is TRUE then the display driver should not send
; repaint requests to USER immediately but wait till it is enabled again.
; USER will enable repaint by seting RepaintDisable to FALSE.
;----------------------------------------------------------------------------
cProc   UserRepaintDisable,<FAR,PASCAL,PUBLIC>
        parmB   disable                 ;TRUE to Disable/FALSE to enable
cBegin
    mov     ax,_INIT
    mov     ds, ax
	assumes	ds,_INIT
	mov	    ax,WORD PTR RemappedDataSegment
	mov	    ds,ax
    mov	al,disable		;get the value
    mov     bRepaintDisable,al      ;save it
    or      al,al                   ;being enabled again
    jnz	short @f		;no.
    cmp	RepaintPending,0ffh	;pending repaint ?
    jnz     short @f                ;no.
    call    DTF_RepaintScreen       ;call repaint
    mov     RepaintPending,0        ;done with pending call
@@:
cEnd

;----------------------------------------------------------------------------
; ScreenSwitchHook   INTERRUPT HANDLER
;
; Watches calls to the INT 2Fh multiplex interrupt chain, and traps calls
; to the driver to save or restore the state of the display hardware
; before a context switch.
;
; If a save/restore call is recognized, then it will be dispatched
; to the device dependent handler unless PREVENT_SWITCH is
; set in SwitchControl.
;
; Entry:
;       AH = multiplex number
;       AL = function code
; Returns:
;       'C' set if screen switch cannot occur (DOS's < 10.0)
;       'C' clear otherwise
;----------------------------------------------------------------------------
        assumes ds,nothing
        assumes es,nothing

PPROC ScreenSwitchHook far

        push    bp                      ;set up frame for altering flags
        mov     bp,sp
        push    ds
        pushad
        mov     bx,_INIT
        mov     ds, bx
	    assumes	ds,_INIT
	    mov	    bx,WORD PTR RemappedDataSegment
	    mov	    ds, bx

        mov     si, offset SwitchTable
        mov     cx, SwitchTableSize

SSH_Loop:
        cmp     ax, [si]
        je      SSH_HandleIt
        add     si,4
        loop    SSH_Loop

        ; The int 2F function is not for us to handle

        popad
        pop     ds
        pop     bp
        jmp     PrevInt2Fh              ; Pass to previous int 2F handler.

SSH_HandleIt:
        test    SwitchControl, PREVENT_SWITCH
        jnz     SSH_ExitError

        add     si,2                    ; Point to handler address.
        call    word ptr [si]
        popad
        pop     ds
        assumes es,nothing
        assumes ds,nothing
;----------------------------------------------------------------------------
; Note that BP is still on the stack (hence FLAGS_ON_STACK+2).
;----------------------------------------------------------------------------
        and     wptr [bp][FLAGS_ON_STACK][2],not CARRY_FLAG ;Show success.
        pop     bp
        iret

SSH_ExitError:
        popad
        pop     ds                              ;This is not a good time
        assumes ds,nothing                      ;to do a screen switch.
        or      wptr [bp][FLAGS_ON_STACK][2],CARRY_FLAG  ;Set the carry flag
        pop     bp                              ;and hopefully, the VDD will
        iret                                    ;try again later.
ScreenSwitchHook        endp

;----------------------------------------------------------------------------
; DevToForeground
; Performs any action necessary to restore the state of the display
; hardware prior to becoming the active context.
;----------------------------------------------------------------------------
PPROC DevToForeground   near

    assumes ds,_INIT
    assumes es,nothing
    assumes	gs,nothing
    assumes	fs,nothing

    ; Some applications such as AAPLAYER depend on the INT 2FH call to
    ; not only repaint the screen, but also reset the mode from low resolution
    ; back into the Windows HiRes mode.  Normally, we'd already have been
    ; switched back to HiRes mode by the time we've reached here.  However,
    ; if the app hasn't switched back in the normally accepted way,
    ; we'll still be in a standard (13H or below) VGA mode:
    ;
    ; Also, we might be called after the DPMS screen saver has ended..
    ; in this case, do nothing, since no state has changed.

    test    GLInfo.DisabledByGLDD, DRIVER_DISABLED
    je      @f     
    and     GLInfo.DisabledByGLDD, NOT DRIVER_DISABLED
    invoke  EnableOffscreenHeap, wScreenWidth, wScreenHeight
@@:

    ; invalidate the cursor cache in case of return from powerdown or hibernation
    invoke  ClearCursorCache

    mov     GLInfo.dwHiResMode, 1   ; Virtualize future VGA io requests.
    call    ResetHiResMode

PLABEL DTF_RepaintScreen
    mov     ax,wptr RepaintAddr[0]  ;We expect to always have the
    or      ax,wptr RepaintAddr[2]  ;  address, but let's be sure
    jz      short DTF_Exit          ;Nope.
    cmp     bRepaintDisable,0       ;Is repaint enabled ?
    je      short @f                ;yes.
    mov     RepaintPending,0ffh     ;no. repaint pending
    ret
@@: 
    call    RepaintAddr             ;Force repaint of all windows

    ; Disable screen blanking
    mov     ax,GLInfo.wRegSel       ;pt at glint
    mov     es,ax
    assumes es,Glint
    cmp     GLInfo.wRenderFamily, GLINT_ID
    jne     DTF_NotGlint
    mov     eax, GLInfo.dwVGADisable
    mov     VTGPolarity, eax
    jmp     DTF_Exit
DTF_NotGlint:
    mov     eax, GLInfo.dwVideoControl
    mov     VideoControl, eax
PLABEL DTF_Exit
    and     GLInfo.dwFlags, NOT GMVF_POWERDOWN
    ret
DevToForeground endp

;----------------------------------------------------------------------------
; DevToBackground
; Performs any action necessary to save the state of the display
; hardware prior to becoming an inactive context.
;----------------------------------------------------------------------------
PPROC DevToBackground   near
    assumes ds,_INIT
    assumes es,nothing
    assumes fs,nothing
    assumes gs,nothing

    ; Disable all the bitmaps in the Offscreen Heap - just in case something
    ; decides to use all the screen memory in DOS mode.

    invoke  DisableOffscreenHeap
    or      GLInfo.DisabledByGLDD, DRIVER_DISABLED

    ; Save away the state of any context just in case something resets the chip.
    mov     ax, CONTEXT_NONE
    call    Context_Change

    cmp     GLInfo.dwHiResMode,0            ;Switching to VGA?
    jne     DTB_InNormalHiresMode                      

	les	    si,GLInfo.lpDriverPDevice
	or	    es:[si].deFlags,BUSY

    jmp     DTB_Exit

DTB_InNormalHiresMode:
    mov     GLInfo.dwHiResMode, 0           ; Allow full access to all VGA extended registers
                                            ; if its a W3.1 FG to background.
    ; DPMS Screen saver should have kicked in. Make sure screen is blanked.
    or      GLInfo.dwFlags, GMVF_POWERDOWN
    mov     ax,GLInfo.wRegSel       ;pt at glint
    mov     es,ax
    assumes es,Glint
    cmp     GLInfo.wRenderFamily, GLINT_ID
    jne     DTB_NotGlint
    mov     VTGPolarity, 0bfh
    jmp     DTB_Exit
DTB_NotGlint:
    mov     VideoControl, 0

DTB_Exit:
    ret

DevToBackground endp

;----------------------------------------------------------------------------
; DisableSwitching
; This function is called whenever we need to prevent a screen switch
; from occuring.
;----------------------------------------------------------------------------
PPROC DisableSwitching near
        mov     ax,ENTER_CRIT_REG
        int     2fh
        or      SwitchControl,PREVENT_SWITCH
        ret
DisableSwitching endp

;----------------------------------------------------------------------------
; EnableSwitching
; This function is called whenever we can allow a screen group switch.
;----------------------------------------------------------------------------
PPROC EnableSwitching near
        assumes ds,_INIT
        assumes es,nothing
        mov     ax,EXIT_CRIT_REG
        int     2fh
        and     SwitchControl,not PREVENT_SWITCH
        ret
EnableSwitching endp
;----------------------------------------------------------------------------
; HookInt2Fh
;
; Installs a link in the 2Fh multiplex interrupt chain to watch for
; calls to the driver to save or restore the state of the display
; hardware before a context switch.
;
; This function is called whenever the driver recieves an enable call.
;----------------------------------------------------------------------------
HookInt2Fh PROC NEAR C PUBLIC
        assumes ds,_INIT
        assumes es,nothing
        push    bx
        push    cx
        push    dx
        push    ds
        push    es
        mov     ax,wptr [RepaintAddr][0]        ;Do we already have User's
        or      ax,wptr [RepaintAddr][2]        ;repaint address?
        jnz     HI2F_HookTheThang               ;Yes. Go hook the interrupt.
        mov     ax,DataOFFSET szUSER    ;No. Get User's module
        farPtr  module_name,ds,ax               ;handle.
        cCall   GetModuleHandle,<module_name>
        xchg    ax,bx
        mov     ax,REPAINT_EXPORT_INDEX
        cwd
        farPtr  func_number,dx,ax
        cCall   GetProcAddress,<bx,func_number> ;Get the address of User's
        mov     wptr [RepaintAddr][0],ax        ;repaint routine and save
        mov     wptr [RepaintAddr][2],dx        ;away for later use.

        mov     ax,DataOFFSET szGDI     ;Now lets get the address
        farPtr  module_name,ds,ax               ;of GDI's color matching
        cCall   GetModuleHandle,<module_name>   ;routine.
        xchg    ax,bx
        mov     ax,DEVICECOLORMATCH_EXPORT_INDEX
        cwd
        farPtr  func_number,dx,ax
        cCall   GetProcAddress,<bx,func_number>
        mov     wptr [GetColorAddr][0],ax       ;Save it away too. This
        mov     wptr [GetColorAddr][2],dx       ;is called by the DIB code.

HI2F_HookTheThang:
        cli                             ;Disable interrupts while hooking
        or      SwitchControl,INT_2F_HOOKED ;this puppy....
        xor     ax,ax
        push    ds                      ;save or Data segment
        mov     ax,CodeBASE             ;get the CS selector
        cCall   AllocCSToDSAlias,<ax>   ;get a data segment alias
        mov     ds,ax                   ;Now we can write into our code segment.
        push    ax                      ;save for later FreeSelector call
        assumes ds,Code                 ;do this to save address in CS
        mov     ax,3500h+2Fh            ;get the vector
        int     21h                     ;Ask DOS for the old vector.
        mov     wptr PrevInt2Fh[0],bx   ;Save off the previous vector.
        mov     wptr PrevInt2Fh[2],es
        mov     dx,CodeOFFSET ScreenSwitchHook ;This will be the new vector.
        mov     ax,CodeBASE
        mov     ds,ax
        assumes ds,nothing
        mov     ax,2500h+2Fh
        int     21h                     ;Ask DOS to install the new vector.
        cCall   FreeSelector            ;selector is on the stack
        pop     ds                      ;get back own data segment
        assumes ds,Data
        sti                             ;Turn interrupts back on.
HI2F_Done:
        pop     es
        assumes es,nothing
        pop     ds
        assumes ds,nothing
        pop     dx
        pop     cx
        pop     bx
        ret
HookInt2Fh      endp

;----------------------------------------------------------------------------
; RestoreInt2Fh
;
; If we installed ourselves into int 2Fh, we'll restore the previous
; vector.
; This function is called whenever the driver receives a disable call.
;----------------------------------------------------------------------------
RestoreInt2Fh PROC NEAR C PUBLIC
        assumes es,nothing
        mov     ax,_INIT
        mov     es, ax
	    assumes	es,_INIT
	    mov	    ax,WORD PTR RemappedDataSegment
	    mov	    es,ax
        test    SwitchControl,INT_2F_HOOKED
        jz      RI2F_Done
        cli
        and     SwitchControl,not INT_2F_HOOKED
        push    es
        push    ds
        mov     ax,CodeBASE             ; get the code address
        cCall   AllocCSToDSAlias,<ax>   ; get a data alias out of it
        mov     es,ax
        push    ax                      ; save for FreeSelector call
        assumes es,Code
        push    dx                      ; save
        lds     dx,PrevInt2Fh           ; get the saved vector
        mov     ax,252fh                ; Call DOS to set vector 2Fh.
        int     21h
        pop     dx                      ; restore
        sti
        cCall   FreeSelector            ; selector is on the stack
        pop     ds
        assumes ds,nothing
        pop     es
        assumes es,_INIT
RI2F_Done:
        ret
RestoreInt2Fh endp
_TEXT   ends
end
