#****************************************************************************
#                                                                           *
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
# PURPOSE.                                                                  *
#                                                                           *
# Copyright 1993-95  Microsoft Corporation.  All Rights Reserved.           *
#                                                                           *
#****************************************************************************

BUILD_TYPE=base
LOCAL_CLEAN=1
!INCLUDE $(BASEDIR)\inc\win_me\master.mk

DEVICE = ASYNCW32

OBJDIR = obj

all: $(OBJDIR)\con_aw32.exe $(OBJDIR)\$(DEVICE).vxd

OBJS        = $(OBJDIR)\asyncw32.obj \
              $(OBJDIR)\sysctrl.obj \
              $(OBJDIR)\wrappers.obj

CFLAGS      = -DWIN32 -DCON -Di386 -D_X86_ -D_NTWIN -W3 -Gs -D_DEBUG -Zi -I$(INC32)
CVXDFLAGS   = -Zdp -Gs -c -DIS_32 -Zl -DDEBLEVEL=1 -DDEBUG -I$(INC32)
LFLAGS      = -machine:i386 -debug:notmapped,full -debugtype:cv -subsystem:console kernel32.lib 
LVXDFLAGS   = -MACHINE:i386 -DEBUG:NONE -PDB:NONE -NOD

########## Definitions for the Assembler ##########
AFLAGS      = -nologo -coff -DBLD_COFF -DIS_32 -W2 -Zd -c -Cx -DMASM6 -DDEBLEVEL=1 -DDEBUG -I$(INC32)
DESCRIPTION = DOS386 Async W32 Devioctl Sample Device
ASMENV      = ML

CL          = cl -nologo

########## Base inference rules ###################

$(OBJDIR):
        @if not exist $(OBJDIR)\nul md $(OBJDIR)

.c{$(OBJDIR)\}.obj:
        $(CL) $(CFLAGS) -Fo$@ %s

.asm{$(OBJDIR)}.obj:
    set $(ASMENV)=$(AFLAGS)
    ml -Fo$*.obj $< 

$(OBJDIR)\asyncw32.obj: asyncw32.c
        $(CL) $(CVXDFLAGS) -Fo$@ %s

$(OBJDIR)\wrappers.obj: wrappers.asm
        ml -nologo -coff -I$(INC32) -DBLD_COFF -W2 -c -Cx -DMASM6 -DSEGNUM=1 -Fo$@ %s

$(OBJDIR)\$(DEVICE).res: 
        $(RC16B) -nologo -r -i$(INC16);$(INC32) -fo $@ <<$(DEVICE).rc
#include <version.h>
#define Not_VxD
#include <vmm.h>

#define VER_FILETYPE			VFT_VXD
#ifdef $(DEVICE)_DEVICE_ID
#define VER_FILESUBTYPE 		$(DEVICE)_DEVICE_ID
#else
#define VER_FILESUBTYPE 		UNDEFINED_DEVICE_ID
#endif
#define VER_FILEDESCRIPTION_STR 	"$(DESCRIPTION)"
#define VER_INTERNALNAME_STR		"$(DEVICE)"
#define VER_LEGALCOPYRIGHT_YEARS	"1995"
#define VER_ORIGINALFILENAME_STR	"$(DEVICE).VXD"

#include <common.ver>
<<

$(OBJDIR)\$(DEVICE).vxd: $(OBJDIR) $(OBJS) $(OBJDIR)\$(DEVICE).res
!ifndef PASS0ONLY
		@echo link -OUT:$@
        link -nologo @<<$(DEVICE).lnk /def:<<$(DEVICE).def
$(LVXDFLAGS) /vxd /libpath:$(LIB32)
-ignore:4078 -ignore:4039
/out:$(OBJDIR)\$(DEVICE).vxd
/map:$(OBJDIR)\$(DEVICE).map
$(OBJS)
$(DDKROOT)\lib\win_me\vxdwraps.clb
<<
VXD $(DEVICE) DYNAMIC
DESCRIPTION '$(DESCRIPTION)'
SEGMENTS
    _LPTEXT     CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _LTEXT      CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _LDATA      CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _TEXT       CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _DATA       CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    CONST       CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _TLS        CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _BSS        CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _ITEXT      CLASS 'ICODE'   DISCARDABLE
    _IDATA      CLASS 'ICODE'   DISCARDABLE
    _PTEXT      CLASS 'PCODE'   NONDISCARDABLE
    _PDATA      CLASS 'PDATA'   NONDISCARDABLE SHARED
    _STEXT      CLASS 'SCODE'   RESIDENT
    _SDATA      CLASS 'SCODE'   RESIDENT
    _DBOSTART   CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
    _DBOCODE    CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
    _DBODATA    CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
    _16ICODE    CLASS '16ICODE' PRELOAD DISCARDABLE
    _RCODE      CLASS 'RCODE'
EXPORTS
    $(DEVICE)_DDB @1
<<
        adrc2vxd.exe $@ $(OBJDIR)\$(DEVICE).res
        -mapsym -s -o $(OBJDIR)\$(DEVICE).sym $(OBJDIR)\$(DEVICE).map > nul
        del $(OBJDIR)\$(DEVICE).exp
        del $(OBJDIR)\$(DEVICE).lib
!endif

$(OBJDIR)\con_aw32.exe: $(OBJDIR) $(OBJDIR)\con_aw32.obj
!ifndef PASS0ONLY
		@echo link -OUT:$@
        link -nologo @<<con_aw32.crf
$(LFLAGS)
-PDB:NONE -OUT:$(OBJDIR)\con_aw32.exe -MAP:$(OBJDIR)\con_aw32.map $(OBJDIR)\con_aw32.obj
<<
!endif

clean:
        -@if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
        -@if exist $(OBJDIR)\*.vxd del $(OBJDIR)\*.vxd
        -@if exist $(OBJDIR)\*.exe del $(OBJDIR)\*.exe
        -@if exist $(OBJDIR)\*.map del $(OBJDIR)\*.map
        -@if exist $(OBJDIR)\*.sym del $(OBJDIR)\*.sym
        -@if exist $(OBJDIR)\*.res del $(OBJDIR)\*.res
        -@if exist $(OBJDIR)\*.lib del $(OBJDIR)\*.lib
        -@if exist $(OBJDIR)\*.exp del $(OBJDIR)\*.exp
        -@if exist $(OBJDIR)\nul   rd  $(OBJDIR)
        -@if exist *.pdb del *.pdb
        -@if exist *.ilk del *.ilk
        -@if exist *.exe del *.exe
        -@if exist *.bak del *.bak
