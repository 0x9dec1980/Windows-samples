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
#
#       Makefile for vsd device
#
##########################################################################

BUILD_TYPE=block
!INCLUDE $(BASEDIR)\inc\win_me\master.mk

DEVICE  = HELPVSD

OBJDIR  = obj

all : $(OBJDIR)\$(DEVICE).vxd

OBJS    = $(OBJDIR)\helpvsd.obj $(OBJDIR)\AER.obj $(OBJDIR)\VSDCO.obj $(OBJDIR)\helpreq.obj

######### Definitions for the debug level #########

!ifdef DEBUG
DDEBUG =-DDEBLEVEL=1 -DDEBUG
!else
DDEBUG =-DDEBLEVEL=0
!endif

########## Definitions for the Assembler ##########

ASM    = ml
AFLAGS = -nologo -coff -DBLD_COFF -DIS_32 -nologo -W3 -Zd -c -Cx -DMASM6 -DINITLOG $(DDEBUG)
AFLAGS = $(AFLAGS) -I..\..\inc -I$(INC32)
ASMENV = ML

########## Definitions for linker #############

LFLAGS  = /VXD /NOD

########## Base inference rules ###################

$(OBJDIR):
        @if not exist $(OBJDIR)\nul md $(OBJDIR)

.asm{$(OBJDIR)}.obj:
    set $(ASMENV)=$(AFLAGS)
    $(ASM) -Fo$*.obj $< 

############### VxD device section ####################

$(OBJDIR)\$(DEVICE).map: $(OBJDIR)\$(DEVICE).vxd

$(OBJDIR)\$(DEVICE).vxd : $(OBJDIR) $(OBJS)
!ifndef PASS0ONLY
	@echo link -OUT:$@
    link @<<$(OBJDIR)\$(DEVICE).lnk /DEF:<<$(DEVICE).def
$(LFLAGS)
-ignore:4078 -ignore:4039
/OUT:$(OBJDIR)\$(DEVICE).vxd
/MAP:$(OBJDIR)\$(DEVICE).map
$(OBJS)
<<
VXD $(DEVICE) DYNAMIC
DESCRIPTION 'DOS386 $(DEVICE) Device  (Version 4.0)'

SEGMENTS
        _LPTEXT         CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _LTEXT          CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _LDATA          CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _TEXT           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _DATA           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        CONST           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _TLS            CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _BSS            CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _ITEXT          CLASS 'ICODE'   DISCARDABLE
        _IDATA          CLASS 'ICODE'   DISCARDABLE
        _PTEXT          CLASS 'PCODE'   NONDISCARDABLE
        _PDATA          CLASS 'PDATA'   NONDISCARDABLE SHARED
        _STEXT          CLASS 'SCODE'   RESIDENT
        _SDATA          CLASS 'SCODE'   RESIDENT
        _DBOSTART       CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
        _DBOCODE        CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
        _DBODATA        CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
        _16ICODE        CLASS '16ICODE' PRELOAD DISCARDABLE
        _RCODE          CLASS 'RCODE'

EXPORTS
        $(DEVICE)_DDB @1
<<
    mapsym -s -o $(OBJDIR)\$(DEVICE).sym $(OBJDIR)\$(DEVICE).map > nul
!endif

clean:
    -@if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
    -@if exist $(OBJDIR)\*.vxd del $(OBJDIR)\*.vxd
    -@if exist $(OBJDIR)\*.lib del $(OBJDIR)\*.lib
    -@if exist $(OBJDIR)\*.exp del $(OBJDIR)\*.exp
    -@if exist $(OBJDIR)\*.map del $(OBJDIR)\*.map
    -@if exist $(OBJDIR)\*.sym del $(OBJDIR)\*.sym
    -@if exist $(OBJDIR)\nul   rd  $(OBJDIR)
    -@if exist *.bak del *.bak
