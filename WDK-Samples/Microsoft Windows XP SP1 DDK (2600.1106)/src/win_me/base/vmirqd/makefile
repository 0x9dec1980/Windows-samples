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

# sample make file for building vmirqd virtual device for Windows

NAME = VMIRQD

OBJDIR = obj

#       Definitions for the debug level 

!ifdef DEBUG
DDEBUG  =-DDEBLEVEL=1 -DDEBUG
!else
DDEBUG  =-DDEBLEVEL=0
!endif

#       Definitions for MASM 6 Assembler

ASM    = ml
AFLAGS = -nologo -coff -DBLD_COFF -DIS_32 -W2 -c -Cx -Zm -DMASM6 $(DDEBUG) -I$(INC32)
ASMENV = ML
LFLAGS = /NOLOGO /VXD /NOD

#       MASM 6 only inference rules

.asm{$(OBJDIR)}.obj:
    set $(ASMENV)=$(AFLAGS)
    $(ASM) -Fo$*.obj $< 

all: $(OBJDIR)\$(NAME).vxd

$(OBJDIR):
    @if not exist $(OBJDIR)\nul md $(OBJDIR)

$(OBJDIR)\vmirqd.obj : vmirqd.asm

OBJS =  $(OBJDIR)\vmirqd.obj 

$(OBJDIR)\$(NAME).vxd: $(OBJDIR) $(NAME).def $(OBJS)
!ifndef PASS0ONLY
		@echo link -OUT:$@
        link @<<$(OBJDIR)\$(NAME).lnk
$(LFLAGS) 
-ignore:4078 -ignore:4039
/OUT:$(OBJDIR)\$(NAME).vxd
/MAP:$(OBJDIR)\$(NAME).map
/DEF:$(NAME).def
$(OBJS)
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
