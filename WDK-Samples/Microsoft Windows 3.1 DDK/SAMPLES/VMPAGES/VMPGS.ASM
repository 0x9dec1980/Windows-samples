        name    vmpgs
;****************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1991
;
;   Title:      VMPGS.ASM
;
;   Version:    3.00
;
;   Date:       22-May-1991 
;
;   Author:     Neil Sandlin
;
;       
;       This small DOS application will call the VMPAGES API which
;       returns the current count of 4k pages assigned to this VM.
;       In order to see anything interesting happening, you need to
;       run WDEB386 (or any other debugger that catches INT 1) and
;       step through the code following the INT 1.
;
;       The VMPAGES API is as follows:
;
;       call    VMPAGES_API_Callback
;
;       entry:  
;
;            dx:ax =  linear address 
;            es:di => 8 BYTE buffer to hold count information. See the
;                     Virtual Device Adaptation Guide in the Windows
;                     Device Driver Kit (DDK) for more information 
;                     (under GetVMPgCount).
;
;       exit:
;
;
;****************************************************************************
 
        include VMPAGES.INC
_DATA   segment word public 'DATA'

Apientry dd     ?

VMCountBuff dd    2 dup (0)


cr      equ     0dh
lf      equ     0ah

msg1  db      cr,lf,'Not running Windows enhanced mode.',cr,lf
msg1l equ     $-msg1

msg2  db      cr,lf,'The VMPAGES VxD is not installed.',cr,lf
msg2l equ     $-msg2

_DATA   ends


_TEXT   segment word public 'CODE'
        assume cs:_TEXT,ds:_DATA

;*--------------------------------------------------------------------*

vmpgs   proc    far
        mov     ax,_DATA
        mov     ds,ax

        mov     ax,1600h                ; enhanced mode?
        int     2fh                     ; api call
        test    al,7fh                  ; enhance mode running?
        jz      not_running_enhanced    ; no

        mov     ax,1684h                ; Get Device API call
        mov     bx,VMPAGES_Dev_ID          ; for the VMPAGES VxD
        int     2fh                     ; do enhanced api
        mov     WORD PTR Apientry,di    ; save the callback address
        mov     WORD PTR Apientry+2,es

        mov     ax,es                   ; is VMPAGES installed?
        or      ax,di
        jz      VMPAGES_not_installed      ; if not, split


        int     1
        push    ds
        pop     es
        mov     di, OFFSET VMCountBuff
        call    DWORD PTR Apientry      ; call VMPAGES

	    ; Our VMCountBuff now contains the info.  However, this example
	    ; does nothing with this information.

        jmp     short exit

not_running_enhanced:
        mov     dx,offset msg1          ; load 
        mov     cx,msg1l
        jmp     short write_msg         ; go output

VMPAGES_not_installed:
        mov     dx,offset msg2          ; load 
        mov     cx,msg2l

write_msg:
        mov     bx,1                    ; stdout
        mov     ah,40h                  ; DOS write
        int     21h

exit:
        mov     ax,4c00h                ; exit to DOS
        int     21h
vmpgs endp


_TEXT   ends


;*--------------------------------------------------------------------*


        end     vmpgs
