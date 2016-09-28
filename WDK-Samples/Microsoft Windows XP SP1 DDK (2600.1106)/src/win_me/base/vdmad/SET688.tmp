#****************************************************************************
#                                                                           *
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
# PURPOSE.                                                                  *
#                                                                           *
# Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
#                                                                           *
#****************************************************************************

BUILD_TYPE=base
!INCLUDE $(BASEDIR)\inc\win_me\master.mk

# sample make file for building VDMAD virtual device for Windows

NAME = VDMAD

OBJDIR = obj

#       Definitions for the debug level 

!ifdef DEBUG
DDEBUG  =-DDEBLEVEL=1 -DDEBUG
!else
DDEBUG  =-DDEBLEVEL=0
!endif

#       Definitions for MASM 6 Assembler

ASM    = ml
AFLAGS = -nologo -coff -DBLD_COFF -DIS_32 -W2 -c -Cx -Zd -DMASM6 -I$(INC32)
ASMENV = ML
LFLAGS = /NOLOGO /VXD /NOD

#       MASM 6 only inference rules

.asm{$(OBJDIR)}.obj:
    set $(ASMENV)=$(AFLAGS)
    $(ASM) -Fo$*.obj $< 

all : $(OBJDIR)\$(NAME).vxd

$(OBJDIR):
    @if not exist $(OBJDIR)\nul md $(OBJDIR)

OBJS = $(OBJDIR)\dmaeisa.obj  \
       $(OBJDIR)\vdmad.obj    \
       $(OBJDIR)\vdmadapi.obj \
       $(OBJDIR)\vdmadio.obj  \
       $(OBJDIR)\vdmadmsg.obj \
       $(OBJDIR)\vdmadps2.obj \
       $(OBJDIR)\vdmadarb.obj

$(OBJDIR)\vdmadmsg.obj: vdmadmsg.asm

$(OBJDIR)\vdmad.obj: vdmad.asm

$(OBJDIR)\vdmadio.obj: vdmadio.asm

$(OBJDIR)\vdmadapi.obj: vdmadapi.asm

$(OBJDIR)\vdmadarb.obj: vdmadarb.asm

$(OBJDIR)\dmaeisa.obj: dmaeisa.asm

$(OBJDIR)\vdmadps2.obj: vdmadps2.asm

$(OBJDIR)\$(NAME).vxd: $(OBJDIR) $(OBJS)
!ifndef PASS0ONLY
		@echo link -OUT:$@
        link @<<$(OBJDIR)\$(NAME).lnk /DEF:<<$(OBJDIR)\$(NAME).def
$(LFLAGS) 
-ignore:4078 -ignore:4039
/OUT:$(OBJDIR)\$(NAME).vxd
/MAP:$(OBJDIR)\$(NAME).map
$(OBJS)
<<
VXD $(NAME)
DESCRIPTION 'VDMAD Sample VxD for Microsoft Windows'
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
    $(NAME)_DDB @1
<<
     -mapsym -s -o $(OBJDIR)\$(NAME).sym $(OBJDIR)\$(NAME).map > nul
!endif


clean:
    -@if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
    -@if exist $(OBJDIR)\*.vxd del $(OBJDIR)\*.vxd
    -@if exist $(OBJDIR)\*.exp del $(OBJDIR)\*.exp
    -@if exist $(OBJDIR)\*.lib del $(OBJDIR)\*.lib
    -@if exist $(OBJDIR)\*.map del $(OBJDIR)\*.map
    -@if exist $(OBJDIR)\*.sym del $(OBJDIR)\*.sym
    -@if exist $(OBJDIR)\nul   rd  $(OBJDIR)
    -@if exist *.bak del *.bak
