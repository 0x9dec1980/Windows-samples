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
#   to use this makefile to build the 32bit side of a Windows95 mini driver
#   define the needed macros and include this file.
#
#   NOTE your driver does not need a .RC file, or .DEF file
#   defaults will be used.
#
#   REQUIRED MACROS
#
#       DRVNAME32       the basename of your driver (ie vga)
#       SOURCES32       the list of files needed to build your driver
#       VERDIR          location of the intermedite files, and final driver
#
#   OPTIONAL MACROS
#
#       DESCRIPTION     this becomes the descripton in the version resource.
#       DRVFILE         final name for the .DRV file, defaults to $(DRVNAME).drv
#       DRVDEF          the .def file to use, if not specifed a default will be used
#       DRVLIBS         any extra libs you need
#       DLLBASE         base of the 32bit dll
#       DLLENTRY        entry point of the 32bit dll
#       DLLEXPORT       export list
#	DEFRULESUPPLIED Defined=1 if rule for builing .DEF file is in target
#			directory makefile.
#	MINIDIR		default to be $(SRCDIR)\.. unless specified otherwise
#	VC5		Default is undefined.  If defined, nmake uses VC5.0 to compile.
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
!ifdef SOURCES32
SOURCES = $(SOURCES32)
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

IS_32       = TRUE
IS_OEM      = TRUE
WANT_C1032  = TRUE
MASM6       = TRUE

!ifndef OVERRIDE_TOOLS


!include $(MINIDIR)\..\check.mk
ASM         = $(MASM_ROOT)\bin\ml
MYMAPSYM    = $(SDKROOT)\bin\mapsym

ASMENV		= ML
MYCC        = $(C32_ROOT)\bin\cl
LINK32      = $(C32_ROOT)\bin\link

!ifdef	SHARED_ROOT
MYRC        = $(SHARED_ROOT)\bin\rc
!else
!ifdef 	C32_ROOT
MYRC        = $(C32_ROOT)\bin\rc
!else
!error
!endif
!endif

!endif
# endif OVERRIDE_TOOLS

!ifndef DLLBASE
DLLBASE = 0xb00b0000
!endif

!ifndef DLLENTRY
DLLENTRY = DllMain@12
!endif

!ifndef DLLEXPORT
DLLEXPORT = DriverInit
!endif

!ifdef DRVNAME32
DRVNAME=$(DRVNAME32)
!endif

!ifdef DRVFILE32
DRVFILE=$(DRVFILE32)
!else
DRVFILE=$(DRVNAME).dll
!endif

!ifdef DRVDEF32
DRVDEF=$(DRVDEF32)
!else
DRVDEF=mini32.def
!endif

!if EXIST($(SRCDIR)\$(DRVDEF))
DRVDEF  = $(SRCDIR)\$(DRVDEF)
!endif

DRVMAP  = $(DRVFILE:.dll=.map)
DRVSYM  = $(DRVFILE:.dll=.sym)
!ifdef  DLL32SYM2
DRVSYM2 = $(DLL32SYM2)
!else
DRVSYM2 = $(DRVFILE:.dll=)dd.sym
!endif
DRVRES  = $(DRVNAME).res

DRVLIBS = $(DRVLIBS32) kernel32.lib user32.lib advapi32.lib \
          ddraw.lib winmm.lib libc.lib thunk32.lib

TARGETS = $(DRVFILE)
.SUFFIXES: .thk


#
#  Make sure environment variables are properly set up
#
INCLUDE = $(SRCDIR)
!ifdef	SRCDIR2
INCLUDE = $(INCLUDE);$(SRCDIR2)
!endif

!ifdef DRVINCLUDE32
INCLUDE = $(INCLUDE);$(DRVINCLUDE32)
!endif

INCLUDE = $(INCLUDE);$(MINIDIR)\inc 
INCLUDE = $(INCLUDE);$(DXDDKROOT)\inc
INCLUDE = $(INCLUDE);$(C32_ROOT)\include
INCLUDE = $(INCLUDE);$(SDKROOT)\include
INCLUDE = $(INCLUDE);$(DDKROOT)\inc16

!ifdef OVERRIDE_MYLIB
LIB = $(OVERRIDE_MYLIB)
!else
MYLIB = $(SRCDIR)
MYLIB = $(MYLIB);$(DXDDKROOT)\lib
MYLIB = $(MYLIB);$(SDKROOT)\lib
MYLIB = $(MYLIB);$(DDKROOT)\lib
MYLIB = $(MYLIB);$(C32_ROOT)\lib
LIB = $(MYLIB)
!endif


##
##  Cflags, Aflags, Lflags
##
## -bzalign =
## -c   = Suppress linking
## -Cx  = preseve case in publics, externs
## -Fc  = Generate .COD files (combined assembly and C) for debugging
## -Fl  = Generate assembly listing
## -G5  = Optimize code to favor pentium
## -Gf  = pool identical strings
## -Gs  = Control Stack Probes
## -nologo = omit header text
## -Ox  = same as /Ob1 /Og /Oi, /Ot, /Oy, /Gs
##    /Ob1 = expand inline functions into equivalent code
##    /Og  = Enable global optimizations
##    /Oi  = generate intrinsic functions
##    /Ot  = generate fast sequences of machine code
##    /Oy  = omit frame pointer
##    /Gs  = control stack probe
## -Ow  = no aliasing (volatile pointers) in functions but maybe across function boundaries
## -Ot  = generate fast sequences of machine code
## -QIfdiv- makes the compiler use real fdiv, not a function call
## -Sg  = 
## -W2  = Warning level 2
## -W3  = Warning level 3
## -Zd  = Object files only contain line-number and public symbol info
## -Zl  = Omit default library name from .OBJ
## -Zp  = Structure members packed on 1 byte boundaries
##
!if "$(VERDIR)" == "debug32"
CFLAGS = -Zd
!endif

CFLAGS = $(CFLAGS) -Zp -Gs -c -DIS_32 -bzalign -Zl
AFLAGS = $(AFLAGS) -DIS_32 -nologo -W2 -Zd -c -Cx -DMASM6 -DMSBUILD=1 -Sg
AFLAGS = $(AFLAGS) -coff -DBLD_COFF

!ifdef OVERRIDE_C32FLAGS
CFLAGS = $(OVERRIDE_C32FLAGS)
!else
CFLAGS  = $(CFLAGS:-W2=) -W3 -Oxwt -nologo -QIfdiv- -G5f $(DEF) $(DEF32) -DMSBUILD=1 -DWIN32 -DWIN95 -DBUILD_DDDDK
CFLAGS  = $(CFLAGS) -DDRVNAME=\"$(DRVNAME)\"
!if "$(VERDIR)" == "debug32"
CFLAGS  = $(CFLAGS) -DDEBUG -Fc
!endif
!endif

!ifdef OVERRIDE_A32FLAGS
AFLAGS = $(OVERRIDE_A32FLAGS)
!else
!if "$(VERDIR)" == "debug32"
AFLAGS  = $(AFLAGS) -DDEBUG -Fl
!endif
!endif


!ifdef OVERRIDE_L32FLAGS
LFLAGS32 = $(OVERRIDE_L32FLAGS)
!else
LFLAGS32 = -merge:.rdata=.text -merge:.bss=.data -nodefaultlib -out:$(DRVFILE)
LFLAGS32 = $(LFLAGS32) -def:$(DRVDEF) -map:$(DRVMAP) -base:$(DLLBASE) -dll
LFLAGS32 = $(LFLAGS32) -machine:i386 -subsystem:windows,4.0 -entry:$(DLLENTRY)
LFLAGS32 = $(LFLAGS32) -export:$(DLLEXPORT) -pdb:none -shared
LFLAGS32 = $(LFLAGS32) $(L32DEF)
!if "$(VERDIR)" == "debug32"
LFLAGS32 = $(LFLAGS32) -debug:full
!else
LFLAGS32 = $(LFLAGS32) -debug:none
!endif
!endif

#############################################################################
#
# build rules
#
#############################################################################

##!message INCLUDE=$(INCLUDE)
##!message MYCC=$(MYCC)

{$(MINIDIR)\common}.asm.obj:
        @$(ASM) @<<
$(AFLAGS)
-Fo$*.obj $<
<<

{$(MINIDIR)\common}.c.obj:
        @$(MYCC) @<<
$(CFLAGS)
-Fo$*.obj $<
<<

{$(SRCDIR)}.asm.obj:
        @$(ASM) @<< 
$(AFLAGS)
-Fo$*.obj $<
<<

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
        @$(SDKROOT)\bin\thunk.exe $(THUNK32_FLAGS) $(SRCDIR)\$*.thk -o $*.asm
        @$(ASM) -D?LARGE -D?QUIET -Zp4 -DIS_32 -c -Cx -DMASM6 $(AFLAGS) $*.asm -Fo$*.obj

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


############################################################################
#
#   build the driver
#
#############################################################################

##-def:$(DRVDEF)

$(DRVFILE): $(DRVOBJS) $(DRVRES) $(DRVDEF)
        $(LINK32) @<<
$(LFLAGS32)
$(DRVOBJS)
$(DRVLIBS)
$(DRVRES)
<<
        $(MYMAPSYM) $(DRVMAP)
        copy $(DRVSYM) $(DRVSYM2)
!if "$(VERDIR)" == "debug32"
!if EXIST($(MINIDIR)\..\debug)
		copy $(DRVFILE) $(MINIDIR)\..\debug\$(DRVFILE)
		copy $(DRVSYM2) $(MINIDIR)\..\debug\$(DRVSYM2)
!endif
!else
!if EXIST($(MINIDIR)\..\retail)
		copy $(DRVFILE) $(MINIDIR)\..\retail\$(DRVFILE)
		copy $(DRVSYM2) $(MINIDIR)\..\retail\$(DRVSYM2)
!endif
!endif

!ifndef DEFRULESUPPLIED

#############################################################################
#
#   build a default .def file for a 32bit DLL
#
#############################################################################
$(DRVDEF):
        copy << $(DRVDEF)
LIBRARY $(DRVFILE:.dll=)

DESCRIPTION '$(DESCRIPTION)'

SECTIONS
        .text  READ EXECUTE SHARED
        .rdata READ EXECUTE SHARED
        .bss   READ WRITE SHARED
        .data  READ WRITE SHARED
        .edata READ WRITE SHARED
        .idata READ WRITE SHARED
        .rsrc  READ SHARED

;;EXPORTS
;;        DriverInit

<<

!endif
