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


# sample make file for building vdialog virtual device for Windows

NAME = VDIALOG

OBJDIR = obj

#       Definitions for the debug level 

!ifdef DEBUG
DDEBUG  =-DDEBLEVEL=1 -DDEBUG
!else
DDEBUG  =-DDEBLEVEL=0
!endif

#       Definitions for MASM 6 Assembler

ASM    = ml
AFLAGS = -nologo -coff -DBLD_COFF -DIS_32 -W2 -c -Cx -Zm -DMASM6 -I$(INC32) $(DDEBUG)
ASMENV = ML
LFLAGS = /NOLOGO /VXD /NOD


all : $(OBJDIR)\$(NAME).vxd $(OBJDIR)\dosacc.exe $(OBJDIR)\winacc.exe

$(OBJDIR):
        @if not exist $(OBJDIR)\nul md $(OBJDIR)

$(OBJDIR)\vdialog.obj: vdialog.asm
	set $(ASMENV)=$(AFLAGS)
	$(ASM) -Fo$*.obj vdialog.asm
	@set $(ASMENV)=

OBJS =  $(OBJDIR)\vdialog.obj 

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

# NOTE: this make file uses 16-bit tools!

# supply the location of a 16-bit linker, compiler, and resource compiler

#LINK16 = link  
#CL16   = cl
#RC16   = rc 

# Standard Windows make file.  The make utility compares the
# creation date of the file to the left of the colon with the file(s)
# to the right of the colon.  If the file(s) on the right are newer
# then the file on the left, Make will execute all of the command lines
# following this line that are indented by at least one tab or space.
# Any valid MS-DOS command line may be used.

# This line allows NMAKE to work as well

# Update the resource if necessary

$(OBJDIR)\winacc.res: winacc.rc winacc.h
    @set INCLUDE=$(INC16);$(DDKROOT)\media\inc
    $(RC16) -nologo -r -fo$(OBJDIR)\winacc.res winacc.rc


# Update the object file if necessary

$(OBJDIR)\winacc.obj: winacc.c winacc.h
    @set INCLUDE=$(INC16);$(DDKROOT)\media\inc
	@echo cl -Fo$* winacc.c
    $(CL16) -nologo -c -AS -Gsw -Oas -W3 -Zpei -Fo$* winacc.c

# Update the executable file if necessary, and if so, add the resource back in.

$(OBJDIR)\winacc.exe: $(OBJDIR) $(OBJDIR)\winacc.obj winacc.def $(OBJDIR)\winacc.res
!ifndef PASS0ONLY
    @set path=$(BIN16);%PATH%
	@echo link -OUT:$@
	@cd $(OBJDIR)
    $(LINK16) /NOLOGO/CO/NOD winacc,,, $(LIB16)\libw $(LIB16)\slibcew, ..\winacc.def
    $(RC16) -nologo winacc.res
	@cd ..
!endif

# If the .res file is new and the .exe file is not, update the resource.
# Note that the .rc file can be updated without having to either 
# compile or link the file.

#winacc.exe:: winacc.res
#   $(RC16) winacc.res

$(OBJDIR)\dosacc.obj: dosacc.asm vdevice.inc
    ml -nologo -c -Cx -Fo$(OBJDIR)\dosacc.obj dosacc.asm

$(OBJDIR)\dosacc.exe: $(OBJDIR) $(OBJDIR)\dosacc.obj
!ifndef PASS0ONLY
	@echo link -OUT:$@
	@cd $(OBJDIR)
    $(LINK16) /NOLOGO dosacc.obj;
	@cd ..
!endif


clean:
    -@if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
    -@if exist $(OBJDIR)\*.vxd del $(OBJDIR)\*.vxd
    -@if exist $(OBJDIR)\*.exp del $(OBJDIR)\*.exp
    -@if exist $(OBJDIR)\*.lib del $(OBJDIR)\*.lib
    -@if exist $(OBJDIR)\*.map del $(OBJDIR)\*.map
    -@if exist $(OBJDIR)\*.sym del $(OBJDIR)\*.sym
    -@if exist $(OBJDIR)\*.exe del $(OBJDIR)\*.exe
    -@if exist $(OBJDIR)\*.res del $(OBJDIR)\*.res
    -@if exist $(OBJDIR)\nul   rd  $(OBJDIR)
    -@if exist *.pdb del *.pdb
    -@if exist *.bak del *.bak
