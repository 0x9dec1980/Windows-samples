#############################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1991
#   All Rights Reserved.
#
#   MINI project makefile include
#
#   USAGE:
#
#   to use this makefile to build the 16bit side of a Windows95 mini driver
#   define the needed macros and include this file.
#
#   NOTE your driver does not need a .RC file
#
#   REQUIRED MACROS
#
#       DRVNAME         the basename of your driver (ie vga)
#       SOURCES         the list of files needed to build your driver
#       VERDIR          location of the intermedite files, and final driver
#
#   OPTIONAL MACROS
#
#       DESCRIPTION     this becomes the descripton in the version resource.
#       DRVFILE         final name for the .DRV file, defaults to $(DRVNAME).drv
#       DRVDEF          the .def file to use, if not specifed a default will be used
#       DRVLIBS         any extra libs you need
#       DRVRESFILE      Optional driver resource file
#	DEFRULESUPPLIED Defined=1 if rule for builing .DEF file is in target
#			directory makefile.
#	MINIDIR		default to be $(SRCDIR)\.. unless specified otherwise
#	MASM6		= TRUE or MASM6 undefined to force to use masm611c
#			= FALSE to use masm5
#
#############################################################################

!ifndef VERDIR
!error
!endif

!ifndef DRVNAME
!error
!endif

!IF [set INCLUDE=;]
!ENDIF

!IF [set LIB=;]
!ENDIF

!if [cd $(VERDIR)]     ## change the current directory
!endif

SRCDIR = ..
OBJDIR = .

!ifndef	MINIDIR
MINIDIR = $(SRCDIR)\..
!endif
SOURCES = $(SOURCES:  = )
SOURCES = $(SOURCES:  = )
SOURCES = $(SOURCES:  = )
SOURCES = $(SOURCES:  = )
SOURCES = $(SOURCES:  = )
DRVOBJS = $(SOURCES:.cpp=.obj)
DRVOBJS = $(DRVOBJS:.c=.obj)
DRVOBJS = $(DRVOBJS:.asm=.obj)
DRVOBJS = $(DRVOBJS:.rc=.res)
DRVOBJS = $(DRVOBJS:.thk=.obj)

IS_16       = TRUE
IS_OEM      = TRUE
!ifndef MASM6
MASM6       = TRUE
!endif


!ifndef OVERRIDE_TOOLS
!include $(MINIDIR)\..\check.mk
ASM         = $(MASM_ROOT)\bin\ml
MYMAPSYM    = $(SDKROOT)\bin\mapsym
MYRC		= $(DXDDKROOT)\bin16\rc
!endif


!ifndef OVERRIDE_TOOLS
ASMENV		= ML
MYCC        = $(C16_ROOT)\bin16\cl
LINK16      = $(C16_ROOT)\bin16\link
!endif

!ifndef DRVFILE
DRVFILE=$(DRVNAME).drv
!endif

!ifndef DRVDEF
DRVDEF  = $(DRVNAME).def
!endif

!if EXIST($(SRCDIR)\$(DRVDEF))
DRVDEF  = $(SRCDIR)\$(DRVDEF)
!endif

DRVMAP  = $(DRVNAME).map
DRVSYM  = $(DRVFILE:.drv=.sym)
DRVSYM  = $(DRVSYM:.DLL=.sym)
DRVSYM  = $(DRVSYM:.dll=.sym)
!ifndef DRVSYM2
DRVSYM2 = $(DRVFILE:.drv=)d.sym
!endif
DRVRES  = $(DRVNAME).res


TARGETS = $(DRVFILE)
.SUFFIXES: .thk

#
#  Make sure environment variables are properly set up
#

INCLUDE = $(SRCDIR)
!ifdef	SRCDIR2
INCLUDE = $(INCLUDE);$(SRCDIR2)
!endif

!ifdef DRVINCLUDE
INCLUDE = $(INCLUDE);$(DRVINCLUDE)
!endif

INCLUDE = $(INCLUDE);$(DDKROOT)\inc\win_me\inc16
INCLUDE = $(INCLUDE);$(DDKROOT)\inc\win_me

INCLUDE = $(INCLUDE);$(MINIDIR)\inc
INCLUDE = $(INCLUDE);$(DXDDKROOT)\inc16
INCLUDE = $(INCLUDE);$(DXDDKROOT)\inc
INCLUDE = $(INCLUDE);$(SDKROOT)\Inc16
INCLUDE = $(INCLUDE);$(SDKROOT)\Include

!ifdef OVERRIDE_DRVLIBS
DRVLIBS = $(OVERRIDE_DRVLIBS)
!else
DRVLIBS = $(DRVLIBS) \
          kernel.lib \
          dibeng.lib \
          ldllcew.lib
!endif

!ifndef OVERRIDE_MYLIB
MYLIB = $(MYLIBDIR)
MYLIB = $(MYLIB);$(SRCDIR)
MYLIB = $(MYLIB);$(DXDDKROOT)\lib
MYLIB = $(MYLIB);$(DDKROOT)\lib\win_me
MYLIB = $(MYLIB);$(DXDDKROOT)\lib16
MYLIB = $(MYLIB);$(SDKROOT)\lib
MYLIB = $(MYLIB);$(DDKROOT)\lib
LIB =$(MYLIB)
!else

LIB =$(OVERRIDE_MYLIB)

!endif

#
#  Cflags, Aflags, and Lflags
#
#  /Al = large model
#  /Af = far data pointers
#  /Aw = ss != ds, ds not loaded on function call
#  /Au = load DS segment register on each FAR call (use this if /Aw causes problems)
#  /c  = exclude linker
#  /Cp = preserve case of user identifiers
#  /Fc = generate .COD files (combined source and assembly)
#  /G3 = gendrate optimized code for 386
#  /GD = protected mode win entry/exit code
#  /Gs = control stack probes
#  /nologo = exclude header text
#  /Oa = assume no aliasing (volatile pointers) in functions or across functions
#  /Ot = favor code spped
#  /Ox = maximum optimziations
#     /Ob = expand inline functions
#     /O2 = maximize speed
#     /Oc = local common expressions
#     /Oe = register allocation
#     /Og = global common expressions
#     /Oi = generate intrinsic functions as inline code
#     /Ol = loop optimizations
#     /On = disable unsafe optimizations
#     /Oo = peep hole optimizations in assembler generation
#     /Ot = favor code speed
#  -NT_TEXT = nmake code segment
#  /u  = remove all defined macros
#  /W3 = warning level 3
#  /Zd = generate only line number and public symbol debugger info
#  /Zi = Generate complete debugging info
#  /Zm = Enable masm 5.10 compatibility
#  /Zp = pack structure members on 1 byte boundary
#

!ifdef OVERRIDE_L16FLAGS
L16FLAGS = $(OVERRIDE_L16FLAGS)
!else
L16FLAGS = /NOE/NOD/NOPACKCODE/NOPACKFUNCTIONS/LI/MAP/AL:16 $(L16DEF)
!endif

!ifdef OVERRIDE_C16FLAGS
CFLAGS = $(OVERRIDE_C16FLAGS)
!else
CFLAGS = /Alfw /nologo /GD /W3 /c /Zp /Zi /G3s /u /Oxat -NT _TEXT $(DEF) $(DEF16) $(COPT) -DWIN95 -DBUILD_DDDDK -DMSBUILD=1 -DIS_16
!endif

!ifdef OVERRIDE_A16FLAGS
AFLAGS = $(OVERRIDE_A16FLAGS)
!else
AFLAGS   = /D?QUIET /nologo /c /Zi /Zd /Zm /Cp -DMSBUILD=1 -DIS_16 $(DEF)
!endif

!if "$(VERDIR)" == "debug"
CFLAGS   = $(CFLAGS) -DDEBUG -Fc
AFLAGS   = $(AFLAGS) -DDEBUG
!endif

#############################################################################
#
# figure out the 32bit driver name and pass it on the cmd line
#
#############################################################################
!ifdef SOURCES32
!ifndef DRVFILE32
!ifdef  DRVNAME32
DRVFILE32=$(DRVNAME32).dll
!else
DRVFILE32=$(DRVNAME).dll
!endif
!endif
CFLAGS = $(CFLAGS) -DDD32_DLLNAME=\"$(DRVFILE32)\"
!endif

CFLAGS = $(CFLAGS) -DDRVNAME=\"$(DRVNAME)\"


#############################################################################
#
# build rules
#
#############################################################################

!if "$(MASM6)" == "TRUE"

{$(MINIDIR)\common}.asm.obj:
        @$(ASM) @<<
$(AFLAGS)
-Fo$*.obj $<
<<

!else

{$(MINIDIR)\common}.asm.obj:
        @$(ASM) $(AFLAGS) $(MINIDIR)\common\$*.asm $*.obj;

!endif

{$(MINIDIR)\common}.c.obj:
        @$(MYCC) @<<
$(CFLAGS)
-Fo$*.obj $<
<<

!ifdef SRCDIR2
{$(SRCDIR)\$(SRCDIR2)}.asm.obj:
        @$(ASM) @<<
$(AFLAGS)
-Fo$*.obj $<
<<

{$(SRCDIR)\$(SRCDIR2)}.c.obj:
        @$(MYCC) @<<
$(CFLAGS)
-Fo$*.obj $<
<<

{$(SRCDIR)\$(SRCDIR2)}.cpp.obj:
        @$(MYCC) @<<
$(CFLAGS) 
-Fo$*.obj $<
<<
!endif

!if "$(MASM6)" == "TRUE"

{$(SRCDIR)}.asm.obj:
        @$(ASM) @<< 
$(AFLAGS)
-Fo$*.obj $<
<<

!else

{$(SRCDIR)}.asm.obj:
        @$(ASM) $(AFLAGS) $(SRCDIR)\$*.asm $*.obj;

!endif

{$(SRCDIR)}.c.obj:
        @$(MYCC) @<<
$(CFLAGS)
-Fo$*.obj $<
<<

{$(SRCDIR)}.cpp.obj:
        @$(MYCC) @<<
$(CFLAGS) 
-Fo$*.obj $<
<<

{$(SRCDIR)}.thk.obj:
        @$(SDKROOT)\bin\thunk.exe $(THUNK16_FLAGS) $(SRCDIR)\$*.thk -o $*.asm
        @$(ASM) -D?LARGE -D?QUIET -Zp4 -DIS_16 -c -Cx -DMASM6 $*.asm -Fo$*.obj



############################################################################
#
#   build the driver
#
#############################################################################
$(DRVFILE): $(DRVOBJS) $(DRVRES) $(DRVDEF)
	$(LINK16) $(L16FLAGS) @<<$(DRVNAME).lnk
$(DRVOBJS: = +^
)
$(DRVFILE)
$(DRVMAP)
$(DRVLIBS:  = +^
)
$(DRVDEF);
<<KEEP
        $(MYRC) -b $(DRVRES) $(DRVFILE)
	copy $(DRVMAP) display.map
        -$(MYMAPSYM) /n  display
	del display.map
!if EXIST(DRVSYM2)
	del $(DRVSYM2)
!endif
	rename display.sym $(DRVSYM2)
!if EXIST($(MINIDIR)\..\$(VERDIR))
	copy $(DRVFILE) $(MINIDIR)\..\$(VERDIR)\$(DRVFILE)
	copy $(DRVSYM2) $(MINIDIR)\..\$(VERDIR)\$(DRVSYM2)
!if EXIST($(SRCDIR)\inf)
	copy $(SRCDIR)\inf\*.* $(MINIDIR)\..\$(VERDIR)
!endif
!endif

!ifndef DEFRULESUPPLIED

#############################################################################
#
#   build a default .def file
#
#############################################################################
$(DRVDEF):
	copy << $(DRVDEF)
LIBRARY DISPLAY

DESCRIPTION 'DISPLAY : 100, 96, 96 : $(DESCRIPTION)'

EXETYPE WINDOWS

;;DATA PRELOAD MOVEABLE SINGLE
;;CODE PRELOAD MOVEABLE DISCARDABLE

DATA PRELOAD FIXED SINGLE
CODE PRELOAD FIXED

HEAPSIZE 64

SEGMENTS
    _TEXT       PRELOAD                       SHARED
    _INIT       PRELOAD  MOVEABLE DISCARDABLE SHARED

EXPORTS
;;;;WEP
    ValidateMode
;   ValidateDesk
    BitBlt              @1
    ColorInfo           @2
    Control             @3
    Disable             @4
    Enable              @5
    EnumDFonts          @6
    EnumObj             @7
    Output              @8
    Pixel               @9
    RealizeObject       @10
    StrBlt              @11
    ScanLR              @12
;   DeviceMode          @13
    ExtTextOut          @14
    GetCharWidth        @15
    DeviceBitmap        @16
;   FastBorder          @17
    SetAttribute        @18
    DibBlt              @19
    CreateDIBitmap      @20
    DibToDevice         @21
    SetPalette          @22
    GetPalette          @23
    SetPaletteTranslate @24
    GetPaletteTranslate @25
    UpdateColors        @26
    StretchBlt          @27
    StretchDIBits       @28
    SelectBitmap        @29
    BitmapBits          @30
    ReEnable            @31
    Inquire             @101
    SetCursor           @102
    MoveCursor          @103
    CheckCursor         @104

IMPORTS
    AllocCSToDSAlias            = KERNEL.170
    AllocDSToCSAlias            = KERNEL.171
    KernelsScreenSel            = KERNEL.174
    AllocSelector               = KERNEL.175
    FreeSelector                = KERNEL.176
    PrestoChangoSelector        = KERNEL.177
    __WinFlags                  = KERNEL.178
    GetSelectorBase             = KERNEL.186
    SetSelectorBase             = KERNEL.187
    SetSelectorLimit            = KERNEL.189
    __A000h                     = KERNEL.174
    __B800h                     = KERNEL.182
    __C000h                     = KERNEL.195
    GlobalSmartPageLock         = KERNEL.230
<<

!endif
