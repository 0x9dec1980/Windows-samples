        name    dosacc
;****************************************************************************
;   (C) Copyright MICROSOFT Corp., 1990
;
;   Date:       8-Oct-1990 
;
;   Author:     Neil Sandlin
;
;
;       DOSACC
;
;       This DOS application demonstrates the functionality of the
;       VDIALOG VxD.
;
;
;****************************************************************************
 
        include vdevice.inc

_DATA   segment word public 'DATA'

value   db      ?

accmsg  db      cr,lf,'Value read from port '
accmsga dd      ?
        db      ':'
accmsgb dd      ?
        db      cr,lf
accmsgl equ     $-accmsg


_DATA   ends

_TEXT   segment word public 'CODE'
        assume cs:_TEXT,ds:_DATA

;*--------------------------------------------------------------------*

dosacc  proc    far
        mov     ax,_DATA
        mov     ds,ax

        mov     ax, vdev_addr
        mov     bx, offset accmsga
        call    hexasc

        mov     ax, -1
        mov     dx, vdev_addr
        in      al, dx                  ; read the port 
        mov     bx, offset accmsgb
        call    hexasc
        
        mov     WORD PTR accmsgb, 2020h

; Write message

        Writel  accmsg                          


        mov     ax,4c00h                ; exit to DOS
        int     21h
dosacc  endp

;************************************************************************
;
;       HEXASC
;
;       This subroutine formats hex values into ASCII
;
;************************************************************************

hexasc  proc    near            ; converts word to hex ASCII
                                ; call with AX = value,
                                ; DS:BX = address for string
                                ; returns AX, BX destroyed

        push    cx              ; save registers
        push    dx

        mov     dx,4            ; initialize character counter
hexasc1:
        mov     cx,4            ; isolate next four bits
        rol     ax,cl
        mov     cx,ax
        and     cx,0fh
        add     cx,'0'          ; convert to ASCII
        cmp     cx,'9'          ; is it 0-9?
        jbe     hexasc2         ; yes, jump
        add     cx,'A'-'9'-1    ; add fudge factor for A-F

hexasc2:                        ; store this character
        mov     [bx],cl
        inc     bx              ; bump string pointer

        dec     dx              ; count characters converted
        jnz     hexasc1         ; loop, not four yet

        pop     dx              ; restore registers
        pop     cx
        ret                     ; back to caller

hexasc  endp


_TEXT   ends


;*--------------------------------------------------------------------*


        end     dosacc

