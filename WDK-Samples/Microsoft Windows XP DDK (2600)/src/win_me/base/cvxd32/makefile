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
!INCLUDE $(BASEDIR)\inc\win_me\master.mk

DEVICE = CVXDSAMP

OBJDIR = obj

all: $(OBJDIR)\$(DEVICE).vxd $(OBJDIR)\con_samp.exe

CFLAGS     = -nologo -DWIN32 -DCON -Di386 -D_X86_ -D_NTWIN -W3 -Gs -D_DEBUG -Zi -I$(INC32)
CVXDFLAGS  = -nologo -Zdp -Gs -c -DIS_32 -Zl -DDEBLEVEL=1 -DDEBUG -I$(INC32)
LFLAGS     = -machine:i386 -debug:notmapped,full -debugtype:cv -subsystem:console kernel32.lib 
AFLAGS     = -nologo -coff -DBLD_COFF -DIS_32 -W2 -Zd -c -Cx -DMASM6 -DDEBLEVEL=1 -DDEBUG -I$(INC32)

########## Base inference rules ###################

$(OBJDIR):
        @if not exist $(OBJDIR)\nul md $(OBJDIR)

$(OBJDIR)\cvxdsamp.obj: cvxdsamp.c
        cl $(CVXDFLAGS) -Fo$@ %s

$(OBJDIR)\cvxdctrl.obj: cvxdctrl.asm
        ml $(AFLAGS) -Fo$@ %s

$(OBJDIR)\vkdgko.obj: vkdgko.asm
        ml -nologo -coff -I$(INC32) -DBLD_COFF -W2 -c -Cx -DMASM6 -DSEGNUM=1 -Fo$@ %s

$(OBJDIR)\cvxdsamp.vxd: $(OBJDIR) $(OBJDIR)\cvxdsamp.obj $(OBJDIR)\cvxdctrl.obj $(OBJDIR)\vkdgko.obj \
                  cvxdsamp.def
        echo >NUL @<<cvxdsamp.crf
-MACHINE:i386 -DEBUG:NONE -PDB:NONE -ignore:4078 -ignore:4039
-DEF:cvxdsamp.def -OUT:$(OBJDIR)\cvxdsamp.vxd -MAP:$(OBJDIR)\cvxdsamp.map -LIBPATH:$(LIB32)
-VXD $(DDKROOT)\lib\win_me\vxdwraps.clb $(OBJDIR)\cvxdsamp.obj $(OBJDIR)\cvxdctrl.obj $(OBJDIR)\vkdgko.obj
<<
!ifndef PASS0ONLY
		@echo link -OUT:$@
        link -nologo @cvxdsamp.crf
        cd $(OBJDIR)
        -mapsym cvxdsamp > nul
        cd $(MAKEDIR)
!endif

$(OBJDIR)\con_samp.obj: con_samp.c 
        cl $(CFLAGS) -c -Fo$@ %s

$(OBJDIR)\con_samp.exe: $(OBJDIR) $(OBJDIR)\con_samp.obj
        echo >NUL @<<con_samp.crf
$(LFLAGS)
-PDB:NONE -OUT:$(OBJDIR)\con_samp.exe -MAP:$(OBJDIR)\con_samp.map $(OBJDIR)\con_samp.obj 
<<
!ifndef PASS0ONLY
		@echo link -OUT:$@
        link -nologo @con_samp.crf
!endif

clean:
        -@if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
        -@if exist $(OBJDIR)\*.vxd del $(OBJDIR)\*.vxd
        -@if exist $(OBJDIR)\*.exe del $(OBJDIR)\*.exe
        -@if exist $(OBJDIR)\*.exp del $(OBJDIR)\*.exp
        -@if exist $(OBJDIR)\*.map del $(OBJDIR)\*.map
        -@if exist $(OBJDIR)\*.sym del $(OBJDIR)\*.sym
        -@if exist $(OBJDIR)\*.lib del $(OBJDIR)\*.lib
        -@if exist $(OBJDIR)\nul   rd  $(OBJDIR)
        -@if exist *.pdb del *.pdb
        -@if exist *.bak del *.bak
        
