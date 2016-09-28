;----------------------------------------------------------------------------
; MACROS.INC
; Copyright (c) 1992 Microsoft Corporation
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------
fptr	equ	fword ptr
wptr	equ	word  ptr
bptr    equ     byte  ptr

;----------------------------------------------------------------------------
; S T R U C S
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".
;----------------------------------------------------------------------------
LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

FARPOINTER      struc
off     dw      ?
sel     dw      ?
FARPOINTER      ends

FPOINTER        struc
foff    dd      ?
fsel    dw      ?
FPOINTER        ends

;----------------------------------------------------------------------------
; M A C R O S
;----------------------------------------------------------------------------


;----------------------------------------------------------------------------
; abs_ax
;	takes absolute value of AX
; Entry:
;	AX	= integer
; Returns:
;	AX	= abs(AX)
;----------------------------------------------------------------------------
abs_ax	macro
	cwd
	xor	ax,dx
	sub	ax,dx
	endm

;----------------------------------------------------------------------------
; min_ax
;	returns min of AX and REG
; Entry:
;	AX	= integer
;	REG	= general purpose register containing an integer
; Returns:
;	AX	= min(AX,REG)
;----------------------------------------------------------------------------
min_ax  macro   REG
ifdif <REG>,<0>
        sub     ax,REG
endif
	cwd
	and	ax,dx
ifdif <REG>,<0>
        add     ax,REG
endif
	endm

;----------------------------------------------------------------------------
; max_ax
;	returns max of AX and REG
; Entry:
;	AX	= integer
;	REG	= general purpose register containing an integer
; Returns:
;	AX	= max(AX, REG)
;----------------------------------------------------------------------------
max_ax  macro   REG
ifdif <REG>,<0>
        sub     ax,REG
endif
	cwd
	not	dx
        and     ax,dx
ifdif <REG>,<0>
        add     ax,REG
endif
	endm

;----------------------------------------------------------------------------
; jmpnext
; jmpnext stop
;
; jmpnext is used in the generation of fall through chains.  It
; generates the opcode used to swallow the next two bytes of object
; code (cmp ax,immediate word), and performs error checking to
; ensure that only two bytes of object code exist between any
; use of jmpnext.
;
; The chain is terminated by use of an optional parameter to jmpnext.
; If this optional field is non-blank, the chain is terminated.
;
; usage
;
;	dog:
;	      mov    al,34
;	     jmpnext
;
;	foo:
;	     mov     al,0
;	     jmpnext
;
;	bar:
;	     mov     al,1
;	     jmpnext stop			;End of the chain
;----------------------------------------------------------------------------
??ji	=	0			;;Initial index value

jmpnext macro e
jn %??ji,%(??ji+1),e			;;Set next label
endm

jn macro i,j,e
.sall
??ji&i:
.xall
ifb <e> 				;;If not the end of the chain
	db	03Dh			;;cmp ax, next two bytes
errn$	??ji&j,+2			;;next lable must be two bytes away
endif
??ji=j					;;increment counter
endm

;----------------------------------------------------------------------------
; DOUT
;
; Writes some debugging info to the standard aux device.
;----------------------------------------------------------------------------
ifdef DEBUG
        extrn OutputDebugString:FAR

DOUT    macro   text
        local   string_buffer

_DATA segment
string_buffer label byte
        db      "&text&",13,10,0
_DATA ends

        pusha
        push    es
        push    DataBASE
        push    DataOFFSET string_buffer
        call    OutputDebugString
        pop     es
        popa
        endm
else

DOUT    macro   text
        endm
endif
;----------------------------------------------------------------------------
; WriteAux
; WriteDebug
;
; Writes some debugging info to the standard aux device.
;----------------------------------------------------------------------------
WriteAux    macro x
ifdef DEBUGAUX
            DOUT <x>
endif
            endm

WriteDebug  macro x
            DOUT <x>
            endm

;----------------------------------------------------------------------------
; Public Label
; example:
;   PLABEL sort
;
; generates:
;   sort:
;   public sort
;----------------------------------------------------------------------------
PLABEL	macro n
n:
public n
endm

;----------------------------------------------------------------------------
; Public Proc
; example:
;   PPROC sort near
;
; generates:
;   proc sort near
;   public sort
;----------------------------------------------------------------------------
PPROC	macro n,nf
n proc nf
public n
endm
