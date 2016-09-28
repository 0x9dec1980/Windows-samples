;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: GDIInter.ASM
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

;
; This function is used to intercept GDI calls. The idea is to write a far jump
; instruction to the first 5 bytes of the GDI call to trap. This jumps into
; our own special routine, which can decide to handle the call entirely, without
; any GDI interaction, or alternativly it needs to do the 5 or so bytes of
; instructions which get zapped by the jump and then jump back to the next instruction.
; 
;----------------------------------------------------------------------------
ifdef GDI_BYPASS

.486
.xlist

include glint.inc
include Toolhelp.inc

.list
;----------------------------------------------------------------------------
; D A T A
;----------------------------------------------------------------------------
_INIT segment

; Table which defines which functions to intercept and command information.
InterceptTable  dw  GDI_ExtTextOut_Intercept 
                dw  GDI_ExtTextOut_InterceptCmd
                dw  0
                dw  0

GDIIntercepted  dw  0

GDIModuleName   db  "GDI",0

GDIDataSegment  dw  0

if 0
TooltalkModuleName db "Toolhelp.dll",0
TooltalkModuleFn db "SystemHeapInfo",0
endif

_INIT ends

_INTERCEPT segment
	assumes cs,_INTERCEPT

InterceptGDI    PROC FAR C
    LOCAL   hGDIModule:WORD, Function:DWORD
.586
    assumes ds, _INIT 

if 0
    , SystemHeapInfo:SYSHEAPINFO
    lea     ax, TooltalkModuleName
    push    ds
    push    ax
    call    LoadLibrary
    mov     hGDIModule, ax
    
    lea     ax, TooltalkModuleFn
    push    hGDIModule                  ; push GDI module handle
    push    ds                          ; push data segment
    push    ax                          ; push offset of string
    call    GetProcAddress              ; Get proc address
    mov     Function, eax

    lea     ax, SystemHeapInfo
    mov     DWORD PTR SystemHeapInfo, SIZE SYSHEAPINFO
    push    ss
    push    ax
    call    Function
endif

    cmp     GDIIntercepted, 0
    jne     IGDI_Done
    mov     GDIIntercepted, 1

    lea     ax, GDIModuleName
    push    ds
    push    ax
    call    GetModuleHandle
    test    ax, ax
    je      IGDI_Done
    mov     hGDIModule, ax              ; preserve GDI module handle

    or      ax, 1                       ; ensure it is a ring 3 segment offset
    mov     es, ax                      ; put it into segment register
    cmp     WORD PTR es:[0], 0454eh     ; Is this a NE header?
    jne     IGDI_Done                   ; exit if not
    cmp     WORD PTR es:[03eh],0400h    ; Is this a Win95 (4.00) GDI?
    jne     IGDI_Done                   ; exit if not
    mov     bx, es:[8]                  ; Get offset to data segment
    mov     ax, es:[bx + 8]             ; get data segment
    or      ax, 1                       ; turn into valid ring 3 segment
    mov     GDIDataSegment, ax

    lea     si, InterceptTable
IGDI_NextFunction:
    mov     ax, [si]                    ; ax points to intercept information
    add     ax, 4
    push    si
    push    hGDIModule                  ; push GDI module handle
    push    ds                          ; push data segment
    push    ax                          ; push offset of string
    call    GetProcAddress              ; Get proc address
    pop     si

    test    ax, ax
    je      IGDI_Done

    ; eax now contains the segment:offset of the required function to intercept
    mov     Function, eax

    ; Set the access rights to the GDI selector so we can write to them
    push    si
    mov     ax, WORD PTR Function+2 ;Get GDI segment
	push    ax                      ;Selector push for Set Attribute call
	push    ax                      ;Selector for Get Attribute call
	xor     ax,ax
	push    ax                      ;Get attribute
	push    ax                      ;Ignored for Get Attribute
	cCall   SelectorAccessRights    ;Returns attibutes in AX
    and     ax,NOT 1ah              ;clear required bits
	or      ax,12h                  ;set writable data selector
	mov     bx,1
	push    bx                      ;Set attribute
	push    ax                      ;Attribute
	cCall   SelectorAccessRights    ;Updates attributes
    pop     si

    ; Set the access rights to the code selector so we can write to it
    push    si
    mov     di, [si]                ; Get code segment
    mov     ax, [di+2]
	push    ax                      ;Selector push for Set Attribute call
	push    ax                      ;Selector for Get Attribute call
	xor     ax,ax
	push    ax                      ;Get attribute
	push    ax                      ;Ignored for Get Attribute
	cCall   SelectorAccessRights    ;Returns attibutes in AX
    and     ax,NOT 1ah              ;clear required bits
	or      ax,12h                  ;set writable data selector
	mov     bx,1
	push    bx                      ;Set attribute
	push    ax                      ;Attribute
	cCall   SelectorAccessRights    ;Updates attributes
    pop     si

    ; Run through the command list modifying our code as required

    mov     di, [si]                    ; Get code segment
    mov     ax, [di+2]
    mov     bx, WORD PTR Function+2     ; Get GDI segment
    mov     di, [si+2]                  ; ds:di is start of command list
    mov     es, ax                      ; es is our code segment
    mov     gs, bx                      ; gs is GDI code segment

IGDI_NextCommand:
    mov     ax, [di]                    ; Get command
    add     di, 2                       ; DI points to next command or data
    cmp     ax, INTERCEPT_END          
    je      IGDI_FinishedCommands
    cmp     ax, INTERCEPT_VALIDATE
    je      IGDI_Validate
    cmp     ax, INTERCEPT_RETURN
    je      IGDI_Return
    cmp     ax, INTERCEPT_MOVE
    je      IGDI_Move
    jmp     IGDI_NextIntercept          ; Unrecognised command, so dont intercept.

IGDI_Validate:
    xor     eax, eax
    xor     ebx, ebx
    mov     ax, [di]                    ; Get GDI Fn Offset
    mov     bx, [di+2]                  ; Get Code Validation Start
    mov     cx, [di+4]                  ; Get Code Validation End
    add     ax, WORD PTR Function       ; get GDI code offset

@@:
    mov     dl, gs:[eax]                ; Get byte of GDI
    mov     dh, es:[ebx]                ; get byte of code
    cmp     dl, dh
    jne     IGDI_NextIntercept          ; Code differs, skip this intercept
    add     ax, 1
    add     bx, 1
    cmp     bx, cx
    jne     @b
    add     di, 6                       ; di points to next command
    jmp     IGDI_NextCommand            ; If we get here, code validated.
IGDI_Move:
    xor     eax, eax
    xor     ebx, ebx
    mov     ax, [di]                    ; Get GDI Fn Offset
    mov     bx, [di+2]                  ; Get Code Validation Start
    mov     cx, [di+4]                  ; Get Code Validation End
    add     ax, WORD PTR Function       ; get GDI code offset
@@:
    mov     dl, gs:[eax]                ; Get byte of GDI
    mov     es:[ebx], dl                ; write it to code
    add     ax, 1
    add     bx, 1
    cmp     bx, cx
    jne     @b
    add     di, 6                       ; di points to next command
    jmp     IGDI_NextCommand
IGDI_Return:
    xor     ecx, ecx
    mov     cx, [di]                    ; Get Offset into our code
    mov     ax, [di+2]                  ; Get Offset into GDI function
    add     ax, WORD PTR Function       ; get GDI code offset
    mov     bx, WORD PTR Function+2     ; Get segment to jump to
    mov     BYTE PTR es:[ecx], 0eah     ; Jump far
    mov     WORD PTR es:[ecx+1], ax     ; to offset
    mov     WORD PTR es:[ecx+3], bx     ; to segment
    add     di, 4
    jmp     IGDI_NextCommand

IGDI_FinishedCommands:
    ; place a far jump right at the start of the call
    mov     di, [si]                    
    mov     ax, [di]                    ; ax = offset of our function
    mov     bx, [di+2]                  ; bx = segment of our function
    mov     di, WORD PTR Function       ; gs:di = function to intercept
    mov     BYTE PTR gs:[di], 0eah      ; Jump far
    mov     WORD PTR gs:[di+1], ax      ; to offset
    mov     WORD PTR gs:[di+3], bx      ; and segment

IGDI_NextIntercept:
; reset the selector access rights

    ; reset the access rights to the GDI selector
    push    si
    mov     ax, WORD PTR Function+2 ;Get GDI segment
	push    ax                      ;Selector push for Set Attribute call
	push    ax                      ;Selector for Get Attribute call
	xor     ax,ax
	push    ax                      ;Get attribute
	push    ax                      ;Ignored for Get Attribute
	cCall   SelectorAccessRights    ;Returns attibutes in AX
    and     ax,NOT 1ah              ;clear required bits
	or      ax,1ah                  ;set writable data selector
	mov     bx,1
	push    bx                      ;Set attribute
	push    ax                      ;Attribute
	cCall   SelectorAccessRights    ;Updates attributes
    pop     si

    ; reset the access rights to the code selector
    push    si
    mov     di, [si]                ; Get code segment
    mov     ax, [di+2]
	push    ax                      ;Selector push for Set Attribute call
	push    ax                      ;Selector for Get Attribute call
	xor     ax,ax
	push    ax                      ;Get attribute
	push    ax                      ;Ignored for Get Attribute
	cCall   SelectorAccessRights    ;Returns attibutes in AX
    and     ax,NOT 1ah              ;clear required bits
	or      ax,1ah                  ;set writable data selector
	mov     bx,1
	push    bx                      ;Set attribute
	push    ax                      ;Attribute
	cCall   SelectorAccessRights    ;Updates attributes
    pop     si


    add     si, 4
    jmp     IGDI_NextFunction

IGDI_Done:
    ret
InterceptGDI    ENDP

_INTERCEPT ends

endif ; GDI_BYPASS

end