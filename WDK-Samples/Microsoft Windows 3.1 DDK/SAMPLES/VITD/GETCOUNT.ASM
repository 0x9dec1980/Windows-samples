;   (C) Copyright MICROSOFT Corp., 1991
;
;   Author:     Neil Sandlin
;

        name    dosit
;
;
        include dosit.inc
        include itimer.inc


ITIMER_COUNT equ 30h
ITIMER_STATE equ 31h


_TEXT   segment word public 'CODE'
        assume cs:_TEXT,ds:_DATA


dosit   proc    far
        mov     ax, _DATA
        mov     ds, ax
        mov     es, ax            

        mov     ax, 1600h               ; check for enhanced windows
        int     2fh                     ; hello windows?
        test    al, 7fh                 ; significant bits
        jnz     short running_enhanced        ; ok

        Writel  errmsg1                 ; display error
        jmp     dosit_exit


running_enhanced:
        Writel  cnthdr
        
        mov     di, offset Count        ; put it here
        Call    Get_Count

        mov     cx, 4                   ; output doubleword
        mov     si, offset Count        ; source
        mov     di, offset cntmsg       ; destination
        call    hextrans                ; make it ascii
        Writel  cntmsg


dosit_exit:        
        mov     ax, 4C00h               ; exit
        int     21h                     ; 

dosit   endp



;************************************************************************
;
;       Set_Count
;
;************************************************************************

Set_Count proc  near
        mov     dx, ITIMER_COUNT        ; count register
        mov     cx, 4                   ; 4 bytes to process
sc_loop:
        lodsb                           ; get byte into al
        out     dx, al                  ; send it on out
        loop    sc_loop                 ; do it again
        ret
Set_Count endp



;************************************************************************
;
;       Get_Count
;
;************************************************************************

Get_Count proc  near
        mov     dx, ITIMER_COUNT        ; count register
        mov     cx, 4                   ; 4 bytes to process
gc_loop:
        in      al, dx                  ; get a byte
        stosb                           ; save it in our area
        loop    gc_loop                 ; do it again
        ret
Get_Count endp


;************************************************************************
;
;       hextrans
;
;       This subroutine formats hex values into ASCII
;
;       ENTRY:
;               CX    = # of bytes to convert
;               DS:SI-> input hex value (in memory)
;               ES:DI-> output area
;
;       USES:
;               AX, SI, DI
;************************************************************************

hextrans proc    near            
                                
        push    bx
        push    cx

        cmp     cx, 0           ; must be higher
        jna     hextexit        ; nope
        add     si, cx          ; point to end of value
        dec     si              ; now pointing at last byte
hext1:
        push    cx
        lodsb                   ; get hex byte
        sub     si, 2           ; make this a decrement
        
        mov     bx, ax          ; save for next nibble
        mov     cx, 4           ; isolate next four bits
        shr     ax, cl          ; get top nibble
        and     ax, 0fh

        cvt_nibble              ; make it ascii
        stosb                   ; save it in destination
        mov     ax, bx          ; retrieve original byte
        and     ax, 0fh
        cvt_nibble              ; make it ascii
        stosb                   ; save it in destination
        pop     cx
        loop    hext1

hextexit:
        pop     cx
        pop     bx
        ret                     ; back to caller

hextrans endp

_TEXT   ends

;*------------------------------- Data ---------------------------------*

_DATA   segment word public 'DATA'

Count   dd      800h

errmsg1 db      'This program must run in a DOS box under Enhanced Windows.'
        db      cr, lf
errmsg1l equ     $-errmsg1

cnthdr  db      'Interval Timer Count = '
cnthdrl equ     $-cnthdr

cntmsg  db      8 dup (?)
        db      8 dup (bs)
cntmsgl equ     $-cntmsg


_DATA   ends




        end     dosit          

