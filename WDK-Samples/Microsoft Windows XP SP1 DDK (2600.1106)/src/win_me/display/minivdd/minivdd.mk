#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991
#       All Rights Reserved.
#                                                                          
#       common makefile for a Mini-VDD Port
#
#       OPTIONAL MACROS
#
#               VXDDIR          $(SRCDIR)\.. as default.
#
##########################################################################

!ifndef VERDIR
!error
!endif

!ifndef VXDNAME
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

!ifndef VXDDIR
VXDDIR = $(SRCDIR)\..
!endif

SOURCES = $(SOURCES:  = )
SOURCES = $(SOURCES:  = )
SOURCES = $(SOURCES:  = )
SOURCES = $(SOURCES:  = )
SOURCES = $(SOURCES:  = )
VXDOBJS = $(SOURCES:.cpp=.obj)
VXDOBJS = $(VXDOBJS:.c=.obj)
VXDOBJS = $(VXDOBJS:.asm=.obj)
VXDOBJS = $(VXDOBJS:.rc=.res)
VXDOBJS = $(VXDOBJS:.thk=.obj)

IS_32       = TRUE
IS_OEM      = TRUE
WANT_C1032  = TRUE
MASM6       = TRUE
!if EXIST($(VXDDIR)\check.mk)
!include $(VXDDIR)\check.mk
!endif

!ifndef ASM
ASM             = $(MASM_ROOT)\bin\ml
!endif 

!ifndef ADDRC
ADDRC           = $(DXDDKROOT)\bin16\adrc2vxd
!endif

!ifndef MYMAPSYM
MYMAPSYM        = $(SDKROOT)\bin\mapsym
!endif

!ifndef MYRC
MYRC            = $(DXDDKROOT)\bin16\rc
!endif




!ifndef ASMENV
ASMENV          = ML
!endif

!ifndef MYCC
MYCC        = $(C32_ROOT)\bin\cl
!endif

!ifndef LINK32
LINK32      = $(C32_ROOT)\bin\link
!endif

!ifndef VXDFILE
VXDFILE=$(VXDNAME).vxd
!endif

!ifndef VXDDEF
VXDDEF  = $(VXDNAME).def
!endif

!if EXIST($(SRCDIR)\$(VXDNAME).def)
VXDDEF  = $(SRCDIR)\$(VXDNAME).def
!endif


!ifndef VXDRC
!if EXIST($(SRCDIR)\$(VXDNAME).rc)
VXDRC   = $(SRCDIR)\$(VXDNAME).rc
!endif
!endif


VXDMAP  = $(VXDNAME).map
VXDSYM  = $(VXDNAME).sym
VXDRES  = $(VXDNAME).res

TARGETS = $(VXDFILE)

#
#  Make sure environment variables are properly set up
#
INCLUDE = $(SRCDIR)
!ifdef  VXDINCLUDE
INCLUDE = $(INCLUDE);$(VXDINCLUDE)
!endif
INCLUDE = $(INCLUDE);$(DXDDKROOT)\inc
INCLUDE = $(INCLUDE);$(DDKROOT)\inc32
INCLUDE = $(INCLUDE);$(DDKROOT)\inc

!ifdef C32_ROOT
INCLUDE = $(INCLUDE);$(C32_ROOT)\include
!endif


LIB=$(DDKROOT)\lib 

!ifdef DXDDKROOT
!ifndef VERSIONHDR
VERSIONHDR=$(DDKROOT)\inc16\version.h
RCINCLUDE=$(DXDDKROOT)\inc
!endif
!else
!error DXDDKROOT not defined
!endif

!IFNDEF DESCRIPTION
DESCRIPTION = $(VXDNAME) Virtual Device  (Version 4.0)
!ENDIF # DESCRIPTION

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

!ifdef OVERRIDE_AVXDFLAGS
AFLAGS   = $(OVERRIDE_AVXDFLAGS)
!else
AFLAGS   = $(AFLAGS) -DSTANDARD_CCALL -DIS_32
AFLAGS   = $(AFLAGS) -coff -DBLD_COFF -nologo -W2 -Zd -c -Cx -DMASM6 -Sg /Fl
AFLAGS   = $(AFLAGS) -DVGA -DVGA31 -DMINIVDD=1 -DWIN40COMPAT=1
!endif

!ifdef OVERRIDE_CVXDFLAGS
CFLAGS   = $(OVERRIDE_CVXDFLAGS)
!else
CFLAGS   = $(CFLAGS) -nologo -Zd -Zp -Gs -c -DIS_32 -bzalign -Zl -DVGA -DVGA31 -DMINIVDD=1 -DWIN40COMPAT=1 
!endif

!ifdef OVERRIDE_LVXDFLAGS
LFLAGS   = $(OVERRIDE_LVXDFLAGS)
!else
LFLAGS   = $(LFLAGS) -nologo -nodefaultlib -align:0x200 -ignore:4039 -ignore:4078 /vxd /NOD
!endif

!if "$(VERDIR)" == "debug"
AFLAGS  = $(AFLAGS) -DDEBUG -Fl
CFLAGS  = $(CFLAGS) -DDEBUG -Fc
LFLAGS =  $(LFLAGS) /DEBUG /DEBUGTYPE:BOTH
VXDRCFLAGS = $(VXDRCFLAGS) -DDEBUG
!endif

#############################################################################
#
# build rules
#
#############################################################################

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

{$(SRCDIR)}.rc.res:
	$(MYRC) $(VXDRCFLAGS) -r -I$(RCINCLUDE) -Fo$@B.res $@B.rc

############################################################################
#
#   build the driver
#
#############################################################################

$(VXDFILE): $(VXDOBJS) $(VXDRES) $(VXDDEF) 
	$(LINK32) $(LFLAGS) @<<
$(VXDOBJS)
$(VXDLIBS)
/DEF:$(VXDDEF)
/OUT:$(VXDNAME).vxd
<<
	$(ADDRC) $(VXDFILE) $(VXDRES)
!if EXIST($(VXDDIR)\..\$(VERDIR))
	    copy $(VXDFILE) $(VXDDIR)\..\$(VERDIR)\$(VXDFILE)
!endif




#############################################################################
#
# build rules for RES file
#
#############################################################################

!ifndef DESCRIPTION
DESCRIPTION=$(VXDNAME) Windows 95 Display Driver Minivdd
!endif

!IFNDEF VXDRC
$(VXDRES): $(VERSIONHDR)
	$(MYRC) $(VXDRCFLAGS) -fo$*.res -r -i $(RCINCLUDE) <<$(VXDNAME).rc
!IFDEF VER_FILEVERSION
#define VER_FILEVERSION $(VER_FILEVERSION)
!ENDIF

!IFDEF VER_FILEVERSION_STR
#define VER_FILEVERSION_STR "$(VER_FILEVERSION_STR)\0"
!ENDIF

#include <version.h>
#define VER_FILETYPE                    VFT_VXD
#ifndef UNDEFINED_DEVICE_ID
#define UNDEFINED_DEVICE_ID 0x00000
#endif
#ifdef $(VXDNAME)_DEVICE_ID
#define VER_FILESUBTYPE                 $(VXDNAME)_DEVICE_ID
#else
#define VER_FILESUBTYPE                 UNDEFINED_DEVICE_ID
#endif
#define VER_FILEDESCRIPTION_STR         "$(DESCRIPTION)"
#define VER_INTERNALNAME_STR            "$(VXDNAME)"
#define VER_LEGALCOPYRIGHT_YEARS        "1988-" VER_COPYRIGHT_CURRENT_YEAR
#define VER_ORIGINALFILENAME_STR        "$(VXDFILE)"

#include <common.ver>
<<$(KEEPFLAG)

!ELSE
$(VXDRES): $(VXDRC)     $(VERSIONHDR)
	$(MYRC) $(VXDRCFLAGS) -fo$*.res -r -i $(RCINCLUDE) $(VXDRC)
!ENDIF


!IFNDEF DDB_NAME
DDB_NAME = $(VXDNAME)_DDB
!ENDIF
$(VXDDEF):
		copy << $(VXDDEF)
VXD $(VXDNAME) DYNAMIC
DESCRIPTION '$(DESCRIPTION)'
SEGMENTS
	_LPTEXT         CLASS 'LCODE'   PRELOAD NONDISCARDABLE
	_LTEXT          CLASS 'LCODE'   PRELOAD NONDISCARDABLE
	_LDATA          CLASS 'LCODE'   PRELOAD NONDISCARDABLE
	_TEXT           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
	_DATA           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
	CONST           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
	_TLS            CLASS 'LCODE'   PRELOAD NONDISCARDABLE
	_BSS            CLASS 'LCODE'   PRELOAD NONDISCARDABLE
	_MSGTABLE               CLASS 'MCODE'   PRELOAD NONDISCARDABLE IOPL
	_MSGDATA                CLASS 'MCODE'   PRELOAD NONDISCARDABLE IOPL
	_IMSGTABLE      CLASS 'MCODE'   PRELOAD DISCARDABLE IOPL
	_IMSGDATA       CLASS 'MCODE'   PRELOAD DISCARDABLE IOPL
	_ITEXT          CLASS 'ICODE'   DISCARDABLE
	_IDATA          CLASS 'ICODE'   DISCARDABLE
	_PTEXT          CLASS 'PCODE'   NONDISCARDABLE
	_PDATA          CLASS 'PDATA'   NONDISCARDABLE SHARED
	_STEXT          CLASS 'SCODE'   RESIDENT
	_SDATA          CLASS 'SCODE'   RESIDENT
	_DBOCODE        CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
	_DBODATA        CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
	_16ICODE        CLASS '16ICODE' PRELOAD DISCARDABLE
	_RCODE          CLASS 'RCODE'
EXPORTS
	$(DDB_NAME) @1
<<
