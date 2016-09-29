        PAGE    ,132

;   (C) Copyright MICROSOFT Corp., 1988-1991
;
;* LIBENTRY.ASM
;
;   Windows dynamic link library entry routine
;
;   This module generates a code segment called INIT_TEXT.  It
;   initialises the local heap if one exists and then calls the
;   C routine LibMain() which should have the form:
;
;   BOOL FAR PASCAL LibMain( HANDLE hModule, WORD wDataSeg,
;                               int cbHeap, LPSTR lpszCmdLine )
;   
;   The result of the call to LibMain is returned to Windows.  
;   The C routine should return TRUE if it completes initialisation
;   successfully, FALSE if some error occurs.
;
;*


include cmacros.inc

externFP <LibMain>              ; the C routine to be called

createSeg INIT_TEXT, INIT_TEXT, BYTE, PUBLIC, CODE
sBegin	INIT_TEXT
assumes CS,INIT_TEXT

?PLM=0                          ; 'C' naming
externA  <_acrtused>            ; ensures that Win DLL startup code is linked

?PLM=1                          ; 'PASCAL' naming
externFP <LocalInit>            ; Windows heap init routine

cProc   LibEntry, <PUBLIC,FAR>  ; entry point into DLL

cBegin
        push    di              ; handle of the module instance
        push    ds              ; the .DLL's data seg (don't assume==hModule)
        push    cx              ; heap size
        push    es              ; command line segment
        push    si              ; command line offset

        ; if we have some heap then initialize it
        jcxz    callc           ; jump if no heap specified

        ; call the Windows function LocalInit() to set up the heap
        ; LocalInit((LPSTR)start, WORD cbHeap);
        
        xor     ax, ax
        cCall   LocalInit < ds, ax, cx >
        test    ax, ax          ; did it do it ok ?
        jz      error           ; quit if it failed

        ; invoke the C routine to do any special initialization

callc:
        call    LibMain         ; invoke the 'C' routine (result in AX)
        jmp     exit            ; LibMain is responsible for stack clean up

error:
        pop     si              ; clean up stack on a LocalInit error
        pop     es
        pop     cx
        pop     ds
        pop     di

exit:

cEnd

sEnd	INIT_TEXT

end LibEntry
