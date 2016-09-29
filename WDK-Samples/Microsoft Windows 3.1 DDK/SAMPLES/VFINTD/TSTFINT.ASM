;   (C) Copyright MICROSOFT Corp., 1990
;
        name    vfintd
;
;       This is a small test app which calls the VxD API for the 
;       VFINTD device.

        include vfintd.inc
;
cr      equ     0dh
lf      equ     0ah

_DATA   segment word public 'DATA'
hellow  db      'VxD Version = '
vfintdx db      4 dup (?)
        db      cr,lf
hellowl equ     $-hellow

no_support_msg db 'Can not function.  Are we in a DOS box under Windows?  ',\
                  'Is VFINTD installed?', cr,lf
no_support_msg_len equ $ - no_support_msg

VxDFunc dd      ?
_DATA   ends



_TEXT   segment word public 'CODE'
        assume cs:_TEXT,ds:_DATA

;*--------------------------------------------------------------------*
;       This routine will get the VxD Device API entry point via 
;       an INT 2F call. Then, calling that entry point, it will 
;       retrieve the version number of the VxD, and more importantly
;       set the focus for floppy interrupts.
;

vfintd proc    far
        mov     ax,_DATA
        mov     ds,ax

        mov     bx,VFINTD_Dev_ID        ; get VxD ID
        mov     ax,1684h                ; Get Device API Entry Point
        int     2fh
        mov     ax,es
        or      ax,di                   ; installed?
        jz      no_support              ; no

        mov     WORD PTR [VxDFunc], di
        mov     WORD PTR [VxDFunc+2], es

        mov     ax,0                    ; get version
        call    VxDFunc

        mov     bx,offset vfintdx      
        call    hexasc

        mov     dx,offset hellow        ;Print VxD version number
        mov     cx,hellowl
        mov     bx,1                    ;stdout
        mov     ah,40h                  ;write
        int     21h
        
        mov     ax,100h                 ; Set focus
        call    VxDFunc
        jmp     short ByeBye

no_support:
        mov     dx,offset no_support_msg  ;Print error message
        mov     cx,no_support_msg_len
        mov     bx,1                    ;stdout
        mov     ah,40h                  ;write
        int     21h
        
ByeBye:
        mov     ax,4c00h
        int     21h
vfintd endp


;--------------------------------------------------------------------
;
;       hexasc
;
;       convert hex to ascii
;
;--------------------------------------------------------------------
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

        end     vfintd

