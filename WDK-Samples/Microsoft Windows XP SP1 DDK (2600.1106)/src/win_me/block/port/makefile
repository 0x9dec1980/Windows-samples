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

BUILD_TYPE=block
!INCLUDE $(BASEDIR)\inc\win_me\master.mk

DEVICE = PORT

OBJDIR = obj

all: $(OBJDIR)\$(DEVICE).pdr

OBJS   = $(OBJDIR)\port.obj $(OBJDIR)\portaer.obj $(OBJDIR)\portreq.obj $(OBJDIR)\portisr.obj

######### Definitions for the debug level #########

!ifdef DEBUG
DDEBUG =-DDEBLEVEL=1 -DDEBUG
!else
DDEBUG =-DDEBLEVEL=0
!endif

########## Definitions for the Assembler ##########

ASM     = ml
AFLAGS  = -nologo -coff -DBLD_COFF $(AFLAGS) -DIS_32 -nologo -W3 -Zd -c -Cx -DMASM6
AFLAGS  = $(AFLAGS) -DINITLOG $(DDEBUG) -I..\inc -I$(INC32)
AFLAGS  = $(AFLAGS) -DDEBUG_TRACE=1
ASMENV  = ML

########## Definitions for linker #############

LFLAGS  = /VXD /NOD

########## Base inference rules ###################

$(OBJDIR):
        @if not exist $(OBJDIR)\nul md $(OBJDIR)

.asm{$(OBJDIR)}.obj:
	set $(ASMENV)=$(AFLAGS)
	$(ASM) -Fo$*.obj $< 

############### VxD device section ####################

$(OBJDIR)\$(DEVICE).map: $(OBJDIR)\$(DEVICE).pdr

$(OBJDIR)\$(DEVICE).pdr : $(OBJDIR) $(OBJS)
!ifndef PASS0ONLY
    @echo link -OUT:$@
    link @<<$(OBJDIR)\$(DEVICE).lnk
$(LFLAGS) 
/OUT:$(OBJDIR)\$(DEVICE).pdr
/MAP:$(OBJDIR)\$(DEVICE).map
$(OBJS)
<<

    -mapsym -s -o $(OBJDIR)\$(DEVICE).sym $(OBJDIR)\$(DEVICE).map > nul
!endif

clean:
    -@if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
    -@if exist $(OBJDIR)\*.pdr del $(OBJDIR)\*.pdr
    -@if exist $(OBJDIR)\*.map del $(OBJDIR)\*.map
    -@if exist $(OBJDIR)\*.sym del $(OBJDIR)\*.sym
    -@if exist $(OBJDIR)\nul   rd  $(OBJDIR)
    -@if exist *.bak del *.bak
