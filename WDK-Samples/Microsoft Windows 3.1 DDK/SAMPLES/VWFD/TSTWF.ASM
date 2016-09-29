        name    tstwf
;****************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1990
;
;   Title:      TSTWF.ASM
;
;   Version:    3.00
;
;   Date:       15-Nov-1990 
;
;   Author:     Neil Sandlin
;
;       
;       This small DOS application will call the VWFD API which
;       returns the state of a VM: Windowed or Full screen.
;       The API's used by this application are:
;
;       INT 2Fh, AX=1600h - determine if enhanced mode Windows is running
;       INT 2Fh, AX=1684h - get VxD API callback address
;
;       The VWFD API is as follows:
;
;       call    VWFD_API_Callback
;
;       entry:
;           BX=VMID of the VM to test  
;
;       exit:
;           AX=0 if VM is full screen
;
;****************************************************************************
 
        include VWFD.INC

_DATA   segment word public 'DATA'

Apientry dd     ?

cr      equ     0dh
lf      equ     0ah

msg1  db      cr,lf,'Full Screen.',cr,lf
msg1l equ     $-msg1

msg2  db      cr,lf,'In a Window.',cr,lf
msg2l equ     $-msg2

msg3  db      cr,lf,'Not running Windows enhanced mode.',cr,lf
msg3l equ     $-msg3

msg4  db      cr,lf,'VWFD is not installed.',cr,lf
msg4l equ     $-msg4

msg5  db      cr,lf,'Specified VM does not exist.',cr,lf
msg5l equ     $-msg5
_DATA   ends



_TEXT   segment word public 'CODE'
        assume cs:_TEXT,ds:_DATA

;*--------------------------------------------------------------------*

tstwf   proc    far
        mov     ax,_DATA
        mov     ds,ax

        mov     ax,1600h                ; enhanced mode?
        int     2fh                     ; api call
        test    al,7fh                  ; enhance mode running?
        jz      not_running_enhanced    ; no

        mov     ax,1684h                ; Get Device API call
        mov     bx,VWFD_Dev_ID          ; for the VWFD VxD
        int     2fh                     ; do enhanced api
        mov     WORD PTR Apientry,di    ; save the callback address
        mov     WORD PTR Apientry+2,es

        mov     ax,es                   ; is VWFD installed?
        or      ax,di
        jz      vwfd_not_installed      ; if not, split

        mov     bx, 2                   ; check for this VMID
        call    DWORD PTR Apientry      ; call VWFD
                                        ; if AX=0, we're running 
                                        ; fullscreen

        jc      api_error
        or      ax,ax                   ; anything on?
        jz      full_screen

; windowed
        mov     dx,offset msg2          ; 
        mov     cx,msg2l
        jmp     short write_msg

full_screen:        
        mov     dx,offset msg1          ; load 
        mov     cx,msg1l
        jmp     short write_msg         ; go output

not_running_enhanced:
        mov     dx,offset msg3          ; load 
        mov     cx,msg3l
        jmp     short write_msg         ; go output

vwfd_not_installed:
        mov     dx,offset msg4          ; load 
        mov     cx,msg4l
        jmp     short write_msg         ; go output

api_error:
        mov     dx,offset msg5          ; load 
        mov     cx,msg5l

write_msg:
        mov     bx,1                    ; stdout
        mov     ah,40h                  ; DOS write
        int     21h

        mov     ax,4c00h                ; exit to DOS
        int     21h
tstwf endp


_TEXT   ends


;*--------------------------------------------------------------------*

        end     tstwf

