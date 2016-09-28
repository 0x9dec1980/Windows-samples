.386p

;  Define NO_SEGMENTS so that VMM.INC won't declare every one of the possible
;  segments.  If we didn't do this, then every object file will get these
;  segment definitions.  Because we're building libraries with tons of the
;  object files, the segment definitions cause the library to grow rapidly.
;
;  The end result is if we don't do this, then the file will be ten times as
;  large as it needs to be.
;
;  The disadvantage is that we must explicitly declare the segments below based
;  on SEGNUM.  But, the segments are well-known, so this isn't much of a
;  problem.
NO_SEGMENTS     EQU     TRUE

include basedef.inc
include vmm.inc
include debug.inc
include shell.inc
include vmmreg.inc
include vxdldr.inc
include configmg.inc
include vcomm.inc
include vpicd.inc
include vdmad.inc
include ios.inc
include vpowerd.inc
include vwin32.inc
include ifsmgr.inc
include vtd.inc
include ntkern.inc
include	pci.inc
include mtrr.inc
include acpivxd.inc
include pccard.inc

.errndef        SEGNUM

IFE     SEGNUM-0

_LTEXT      SEGMENT DWORD PUBLIC FLAT 'LCODE'
_LTEXT      ENDS
        SEGB    TEXTEQU <_LTEXT SEGMENT>
        SEGE    TEXTEQU <_LTEXT ENDS>

ELSEIFE SEGNUM-1

_LTEXT      SEGMENT DWORD PUBLIC FLAT 'LCODE'
_LTEXT      ENDS
        SEGB    TEXTEQU <_LTEXT SEGMENT>
        SEGE    TEXTEQU <_LTEXT ENDS>

ELSEIFE SEGNUM-2

_ITEXT      SEGMENT DWORD PUBLIC FLAT 'ICODE'
_ITEXT      ENDS
        SEGB    TEXTEQU <_ITEXT SEGMENT>
        SEGE    TEXTEQU <_ITEXT ENDS>

ELSEIFE SEGNUM-3

_PTEXT      SEGMENT DWORD PUBLIC FLAT 'PCODE'
_PTEXT      ENDS
        SEGB    TEXTEQU <_PTEXT SEGMENT>
        SEGE    TEXTEQU <_PTEXT ENDS>

ELSEIFE SEGNUM-4

_STEXT      SEGMENT DWORD PUBLIC FLAT 'SCODE'
_STEXT      ENDS
        SEGB    TEXTEQU <_STEXT SEGMENT>
        SEGE    TEXTEQU <_STEXT ENDS>

ELSEIFE SEGNUM-5

_DB1CODE    SEGMENT DWORD PUBLIC FLAT 'DBOCODE'
_DB1CODE    ENDS
        SEGB    TEXTEQU <_DB1CODE SEGMENT>
        SEGE    TEXTEQU <_DB1CODE ENDS>

ELSEIFE SEGNUM-6

PNP         SEGMENT DWORD PUBLIC FLAT 'PNPCODE'
PNP         ENDS
        SEGB    TEXTEQU <PNP SEGMENT>
        SEGE    TEXTEQU <PNP ENDS>

ELSEIFE SEGNUM-7

LOCKABLE         SEGMENT DWORD PUBLIC FLAT 'LOCKABLECODE'
LOCKABLE         ENDS
        SEGB    TEXTEQU <LOCKABLE SEGMENT>
        SEGE    TEXTEQU <LOCKABLE ENDS>

ELSE
        %OUT    Wrong SEGNUM
        .err

ENDIF

BEGSEG                  MACRO
SEGB
ENDM

ENDSEG                  MACRO
SEGE
ENDM

@RETNAME                MACRO   Name, Option
                        LOCAL   txt
%IFIDNI	<Option>, <FASTCALL>
	prefix	TEXTEQU	<@>
ELSE
	prefix	TEXTEQU	<_>
ENDIF
IFE     SEGNUM-0

       txt     TEXTEQU @CATSTR(%prefix,Name)

ELSEIFE     SEGNUM-1

       txt     TEXTEQU @CATSTR(%prefix,LCODE_,Name)

ELSEIFE SEGNUM-2

       txt     TEXTEQU @CATSTR(%prefix,ICODE_,Name)

ELSEIFE SEGNUM-3

       txt     TEXTEQU @CATSTR(%prefix,PCODE_,Name)

ELSEIFE SEGNUM-4

       txt     TEXTEQU @CATSTR(%prefix,SCODE_,Name)

ELSEIFE SEGNUM-5

       txt     TEXTEQU @CATSTR(%prefix,DCODE_,Name)

ELSEIFE SEGNUM-6

       txt     TEXTEQU @CATSTR(%prefix,CCODE_,Name)

ELSE

       txt     TEXTEQU @CATSTR(%prefix,KCODE_,Name)

ENDIF

                        EXITM   txt
ENDM


StartCDecl              MACRO   Name, Option

    ??_ccall = 1
    ??_pcall = 0
    ??_scall = 0
    ??_numargs = 0
    ??_numlocals = 0
    ??_numlocalsymbols = 0 
    ??_procflags = 0
    ??_esp = 0
    ??_frame = 0
    ??_taframe = 0
    ??_fastcall = 0
    ??_aframe = 0
    ??_cleanoff = 0
    ??_w32svc = 0
    ??_might_block = 0 

    ??_test_cld = DFS_TEST_CLD

    ife ?_PCODE
       ??_might_block = DFS_TEST_BLOCK
    endif

    if ??_esp

    ??_basereg equ <esp+??_pushed>
    ??_initaframe = 4

    else

    ??_basereg equ <ebp>
    ??_initaframe = 8

    endif

    @Caller equ <dword ptr [??_basereg+??_initaframe-4]>

    ??_cleanoff = ??_pcall or ??_scall

    ??_dfs = ??_test_cld + ??_might_block
			
PUBLIC                  @RETNAME(Name, Option)
BEGSEG
@RETNAME(Name,Option) 	PROC NEAR


ENDM

EndCDecl                MACRO   Name, Option
if ??_procflags AND ??_pf_Left
if ??_fleave
if ??_esp
    ??_pushed = ??_pushed - ??_frame
endif
endif
endif
ifdifi  <Flag>,<NOCHECK>
    if ??_pushed ne 0
        %out Warning: stack not balanced in Name
    endif
    if ??_procflags AND ??_pf_Check
        ife ??_procflags AND ??_pf_Entered
            %out Warning: ArgVar/LocalVar without EnterProc in Name
        endif
        ife ??_procflags AND ??_pf_Left
            %out Warning: ArgVar/LocalVar without LeaveProc in Name
        endif
        ife ??_procflags AND ??_pf_Returned
            %out Warning: ArgVar/LocalVar without Return in Name
        endif
    endif
endif
ifdifi  <Flag>,<KEEPFRAMEVARS>
    ?count = 0
    rept    ??_numlocalsymbols
        ?count = ?count + 1
        ?invprg <?LOC>,%?count
    endm
endif
@RETNAME(Name, Option)	ENDP
ENDSEG
ENDM

MakeCDecl               MACRO   Name
StartCDecl              Name
                        VxDJmp  Name
EndCDecl                Name
ENDM

MakeStdCall             MACRO   Name, Param
StartCDecl              Name&@&Param
                        VxDJmp  Name
EndCDecl                Name&@&Param
ENDM

MakeFastCall		MACRO	Name, Param
StartCDecl		Name&@&Param, FASTCALL
			VxDJmp	Name
EndCDecl		Name&@&Param, FASTCALL
ENDM

