;   (C) Copyright MICROSOFT Corp., 1991
;
;   Author:     Neil Sandlin
;

        name    sample
;
;
        include itimer.inc


_TEXT   segment word public 'CODE'
        assume cs:_TEXT,ds:_DATA

sample  proc    far
        mov     ax, _DATA
        mov     ds, ax
        mov     es, ax            

        mov     ax, 1600h               ; check for enhanced windows
        int     2fh                     ; hello windows?
        test    al, 7fh                 ; significant bits
        jnz     short running_enhanced        ; ok
        jmp     sample_exit
running_enhanced:

;-----------------------------------------------------------------------
;       set up interval timer
;-----------------------------------------------------------------------

        mov     si, offset Count        ; point to count
        mov     dx, ITIMER_COUNT        ; count register
        mov     cx, 4                   ; 4 bytes to process
sc_loop:
        lodsb                           ; get byte into al
        out     dx, al                  ; send it on out
        loop    sc_loop                 ; do it again

        mov     al, ITIMER_RUNNING OR ITIMER_NO_RUPT
        mov     dx, ITIMER_STATE
        out     dx, al                  ; start timer

sample_exit:        
        mov     ax, 4C00h               ; exit
        int     21h                     ; 

sample  endp
_TEXT   ends

;*------------------------------- Data ---------------------------------*

_DATA   segment word public 'DATA'

Count   dd      2000h

_DATA   ends

        end     sample          

