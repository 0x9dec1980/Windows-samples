;   (C) Copyright MICROSOFT Corp., 1991
;
;   Author:     Neil Sandlin
;

        name    dosit
;
;
        include dosit.inc
        include itimer.inc



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

        Writel  errmsg                 ; display error
        jmp     dosit_exit


running_enhanced:
;-----------------------------------------------------------------------
;       install handler
;-----------------------------------------------------------------------
        push    es
        mov     ah, 35h
        mov     al, ITIMER_VECTOR
        int     21h                     ; get old vector
        mov     WORD PTR cs:oldint,bx   ; save old vector here
        mov     WORD PTR cs:oldint+2,es
        pop     es

        push    ds
        mov     dx, offset handler
        push    cs                      ; get current code segment
        pop     ds
        mov     ah, 25h
        mov     al, ITIMER_VECTOR       ; vector to hook
        int     21h                     ; hook that vector
        pop     ds

;-----------------------------------------------------------------------
;       set up interval timer
;-----------------------------------------------------------------------


        mov     si, offset Count        ; point to count
        call    Set_Count
        mov     WORD PTR Count, 0       ; clear it out
        mov     WORD PTR Count+2, 0     ; clear it out

        mov     Timer_rupt, 0           ; indicate no interrupt yet
        mov     al, ITIMER_RUNNING OR ITIMER_ENHANCED
        mov     dx, ITIMER_STATE
        out     dx, al                  ; start timer

;-----------------------------------------------------------------------
;       wait for timer completion
;-----------------------------------------------------------------------

        Writel  cnthdr
        
cntloop:
        push    cx
        mov     di, offset Count        ; put it here
        Call    Get_Count

        mov     cx, 4                   ; output doubleword
        mov     si, offset Count        ; source
        mov     di, offset cntmsg       ; destination
        call    hextrans                ; make it ascii
        Writel  cntmsg

        mov     cx, 0ffffh              ; spin a while
chugloop: loop  chugloop                ; waste system resources
        mov     cx, 0ffffh              ; spin a while
chugloop2: loop  chugloop2              ; waste system resources

        pop     cx

        cmp     BYTE PTR Timer_rupt, 0ffh  ; get the interrupt?
        jz      short dosit_rupt        ; yes

        jmp     cntloop

        jmp     dosit_exit

dosit_rupt:
        mov     di, offset Count        ; put it here
        Call    Get_Count

        mov     cx, 4                   ; output doubleword
        mov     si, offset Count        ; source
        mov     di, offset cntmsg       ; destination
        call    hextrans                ; make it ascii
        Writel  cntmsg
        Writel  ruptmsg                 ; display

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


;**********************************************************************

handler proc    far
        push    ds
        push    ax
        mov     ax, _DATA
        mov     ds, ax

        mov     Timer_rupt, 0ffh        ; we got the 'rupt

        pop     ax
        pop     ds
        iret
handler endp
        

_TEXT   ends


;*------------------------------- Data ---------------------------------*

_DATA   segment word public 'DATA'

oldint  dd      ?
Count   dd      2000h

errmsg  db      'This program must run in a DOS box under Enhanced Windows.'
        db      cr, lf
errmsgl equ     $-errmsg

cnthdr  db      'Interval Timer Count = '
cnthdrl equ     $-cnthdr

cntmsg  db      8 dup (?)
        db      8 dup (bs)
cntmsgl equ     $-cntmsg

ruptmsg db      cr, lf, 'Interrupt from interval timer received!'
ruptmsgl equ     $-ruptmsg

Timer_rupt db   ?

_DATA   ends


        end     dosit          
