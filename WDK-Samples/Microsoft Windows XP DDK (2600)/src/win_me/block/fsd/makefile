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
#  Makefile for VDEF (default FSD)
#
#############################################################################

BUILD_TYPE=block
!INCLUDE $(BASEDIR)\inc\win_me\master.mk

DEVICE = VDEF

OBJDIR = obj

all: $(OBJDIR)\$(DEVICE).VXD

OBJS   = $(OBJDIR)\fsmount.obj  \
         $(OBJDIR)\defdata.obj  \
         $(OBJDIR)\defaer.obj   \
         $(OBJDIR)\vdefios.obj  \
         $(OBJDIR)\deffcn.obj   \
         $(OBJDIR)\vxdinit.obj  \
         $(OBJDIR)\defioctl.obj \
         $(OBJDIR)\vdefdda.obj

########## Definitions for the Assembler ##########

AFLAGS = -nologo -coff -DBLD_COFF -DIS_32 -W3 -Zd -c -Cx -DMASM6 -DINITLOG 
AFLAGS = $(AFLAGS) -DSAFE=3 -DDOS_VXD=1 -DWIN32_APIS -DDEBLEVEL=1 -DDEBUG -I$(INC32)
ASMENV = ML

########## Base inference rules ###################

$(OBJDIR):
        @if not exist $(OBJDIR)\nul md $(OBJDIR)

.asm{$(OBJDIR)}.obj:
    set $(ASMENV)=$(AFLAGS)
    ml -Fo$*.obj $< 

############### VxD device section ####################

$(OBJDIR)\$(DEVICE).vxd : $(OBJDIR) $(OBJS)
!ifndef PASS0ONLY
	@echo link -OUT:$@
    link @<<$(OBJDIR)\$(DEVICE).lnk /DEF:<<$(DEVICE).def
   -ignore:4078 -ignore:4039
/VXD /NOD /OUT:$(OBJDIR)\$(DEVICE).vxd /MAP:$(OBJDIR)\$(DEVICE).map
$(OBJS)
<<
VXD $(DEVICE)
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
    mapsym -s -o $(OBJDIR)\$(DEVICE).sym $(OBJDIR)\$(DEVICE).map
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
