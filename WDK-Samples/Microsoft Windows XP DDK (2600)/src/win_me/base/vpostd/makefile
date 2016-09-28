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

#
#   Makefile for VPOSTD.VXD, POSTHOST.DLL, POSTAPP.EXE, and POSTDOS.EXE
#
#   This is a sample demonstrating how to PostMessage from a VxD.
#

NAME = VPOSTD

OBJDIR = obj

# supply the tools location information below

#LINK16	= link
#CL16	= cl
#RC16	= rc

#
# for POSTDOS.EXE build
#

!IFDEF DEBUG_POSTDOS

DAS =   ml -nologo -c -Zm -Zi
DCC =   $(CL16) -nologo -c -Zi
DLN =   $(LINK16) /NOLOGO/CO
DZZ =   POSTDOS: ***** DEBUG ***** DEBUG ***** DEBUG ***** DEBUG *****

!ELSE

DAS =   ml -nologo -c -Zm 
DCC =   $(CL16) -nologo -c -Fo$@
DLN =   $(LINK16) /NOLOGO
DZZ =   POSTDOS: **** RELEASE *** RELEASE *** RELEASE *** RELEASE ****

!ENDIF

DSRCS   =   postdos.c
DOBJS   =   $(OBJDIR)\postdos.obj

#
# for POSTAPP.EXE build
#

!IFDEF DEBUG_POSTAPP

AAS =   ml -DIS_32 -W2 -c -Cx -Zm -Zi -DMASM6 -DDEBLEVEL=1 -DDEBUG
ACC =   $(CL16) -nologo -c -DDEBUG -AM -G2sw -Owd -W3 -Ziped -Fo$*.obj
ALN =   $(LINK16) /NOLOGO/NOD/NOE/A:16/CO 
ARC =   $(RC16B) -nologo -DDEBUG
AZZ =   POSTAPP: ***** DEBUG ***** DEBUG ***** DEBUG ***** DEBUG *****

!ELSE

AAS =   ml -DIS_32 -W2 -c -Cx -Zm -DMASM6 -DDEBLEVEL=0 
ACC =   $(CL16) -nologo -c -DRELEASE -AM -G2sw -Ows -W3 -Zpe -Fo$*.obj
ALN =   $(LINK16) /NOLOGO/NOD/NOE/A:16 
ARC =   $(RC16B) -nologo -DRELEASE
AZZ =   POSTAPP: **** RELEASE *** RELEASE *** RELEASE *** RELEASE ****

!ENDIF

AMISC   =   postapp.def     postapp.lnk
AINCS   =   postapp.h
ASRCS   =   postapp.c
AOBJS   =   $(OBJDIR)\postapp.obj


#
# for POSTHOST.DLL build
#

!IFDEF DEBUG_POSTHOST

LAS =   ml -nologo -c -Zm -Zi
LCC =   $(CL16) -nologo -c -DDEBUG -Alnw -G2sw -Owd -W3 -Ziped -Fo$*.obj
LLN =   $(LINK16) /NOLOGO/NOD/NOE/A:16/CO 
LRC =   $(RC16B) -nologo -DDEBUG
LZZ =   POSTHOST: ***** DEBUG ***** DEBUG ***** DEBUG ***** DEBUG *****

!ELSE

LAS =   ml -nologo -c -Zm 
LCC =   $(CL16) -nologo -c -DRELEASE -Alnw -G2sw -Ows -W3 -Zpe -Fo$*.obj
LLN =   $(LINK16) /NOLOGO/NOD/NOE/A:16 
LRC =   $(RC16B) -nologo -DRELEASE
LZZ =   POSTHOST: **** RELEASE *** RELEASE *** RELEASE *** RELEASE ****

!ENDIF

LMISC   =   posthost.def    posthost.lnk

LINCS   =   posthost.h
LSRCS   =   posthost.c      libentry.asm
LOBJS   =   $(OBJDIR)\posthost.obj    $(OBJDIR)\libentry.obj

#
# for VPOSTD.VXD build
#

!IFDEF DEBUG_VPOSTD

ASM    = ml
AFLAGS = -nologo -coff -DBLD_COFF -DIS_32 -W2 -c -Cx -Zm -DMASM6 -DDEBUG -I$(INC32)
ASMENV = ML
LFLAGS = /NOLOGO /VXD /NOD
VRC =   rc -v -DDEBUG
VZZ =   VPOSTD: ***** DEBUG ***** DEBUG ***** DEBUG ***** DEBUG *****

!ELSE

ASM    = ml
AFLAGS = -nologo -coff -DBLD_COFF -DIS_32 -W2 -c -Cx -Zm -DMASM6 -DRELEASE -I$(INC32)
ASMENV = ML
LFLAGS = /NOLOGO /VXD /NOD
VRC =   rc -v -DRELEASE
VZZ =   VPOSTD: **** RELEASE *** RELEASE *** RELEASE *** RELEASE ****

!ENDIF

VMISC   =   vpostd.def  $(OBJDIR)\vpostd.lnk
VINCS   =   vpostd.inc
VSRCS   =   vpostd.asm
VOBJS   =   $(OBJDIR)\vpostd.obj

#
# now the rules
#

all: $(OBJDIR)\$(NAME).vxd $(OBJDIR)\postapp.exe $(OBJDIR)\postdos.exe

$(OBJDIR):
    @if not exist $(OBJDIR)\nul md $(OBJDIR)

# VPOSTD.vxd

$(OBJDIR)\$(NAME).vxd: $(OBJDIR) $(NAME).def $(VOBJS)
!ifndef PASS0ONLY
		@echo link -OUT:$@
        link @<<$(OBJDIR)\$(NAME).lnk
$(LFLAGS) 
-ignore:4078 -ignore:4039
/OUT:$(OBJDIR)\$(NAME).vxd
/MAP:$(OBJDIR)\$(NAME).map
/DEF:$(NAME).def
$(VOBJS)
<<
		-mapsym -s -o $(OBJDIR)\$(NAME).sym $(OBJDIR)\$(NAME).map > nul
!endif

$(OBJDIR)\vpostd.obj: vpostd.asm  vpostd.inc
		set $(ASMENV)=$(AFLAGS)
		$(ASM) -Fo$*.obj vpostd.asm
		@set $(ASMENV)=

vpostd.lst: vpostd.asm  vpostd.inc
                $(VAS) $*;

# POSTHOST.DLL


$(OBJDIR)\posthost.dll: $(OBJDIR) $(LOBJS) posthost.def
!ifndef PASS0ONLY	
				@echo link -OUT:$@
                $(LLN) @<<posthost.lnk
$(OBJDIR)\posthost.obj $(OBJDIR)\libentry.obj
$(OBJDIR)\posthost.dll
$(OBJDIR)\posthost.map
$(LIB16)\mdllcew $(LIB16)\libw
posthost.def;
<<
                $(BIN16)\implib $(OBJDIR)\posthost.lib posthost.def > nul
                $(LRC) $(OBJDIR)\posthost.dll
                @echo $(LZZ)
                @echo $(LZZ)
                @echo $(LZZ)
!endif

$(OBJDIR)\posthost.obj:   posthost.c posthost.h
				@set INCLUDE=$(INC16)
				@echo.
				@echo cl -Fo$@ posthost.c
                $(LCC) posthost.c

$(OBJDIR)\libentry.obj:   libentry.asm
                $(LAS) -Fo$*.obj libentry.asm

#
# POSTAPP.EXE
#

$(OBJDIR)\postapp.exe :: $(OBJDIR) $(AOBJS) postapp.def $(OBJDIR)\postapp.res $(OBJDIR)\posthost.dll
!ifndef PASS0ONLY
                @set PATH=$(BIN16);%PATH%
				@echo link -OUT:$@
                $(ALN) @<<postapp.lnk
$(OBJDIR)\postapp.obj
$(OBJDIR)\postapp.exe
$(OBJDIR)\postapp.map
$(OBJDIR)\posthost.lib $(LIB16)\mlibcew $(LIB16)\libw
postapp.def;
<<
                $(ARC) $(OBJDIR)\postapp.res $(OBJDIR)\postapp.exe
                @echo $(AZZ)
                @echo $(AZZ)
                @echo $(AZZ)
!endif

$(OBJDIR)\postapp.exe ::   $(OBJDIR) $(OBJDIR)\postapp.res
!ifndef PASS0ONLY	
                $(ARC) $(OBJDIR)\postapp.res $(OBJDIR)\postapp.exe
                @echo $(AZZ)
                @echo $(AZZ)
                @echo $(AZZ)
!endif

$(OBJDIR)\postapp.obj :   postapp.c postapp.h posthost.h
				@set INCLUDE=$(INC16)
				@echo cl -Fo$@ postapp.c
                $(ACC) postapp.c

$(OBJDIR)\postapp.res :   postapp.rc postapp.h postapp.ico
                $(ARC) -i $(INC16) -fo$@ -r postapp.rc


#
# POSTDOS.EXE
#

$(OBJDIR)\postdos.exe : $(DOBJS)
!ifndef PASS0ONLY	
				@echo link -OUT:$@
				@set LIB=$(LIB16);$(C16_ROOT)\lib
				cd $(OBJDIR)
                $(DLN) postdos;
				cd ..\..
                @echo $(DZZ)
                @echo $(DZZ)
                @echo $(DZZ)
!endif

$(OBJDIR)\postdos.obj :	postdos.c
						@set INCLUDE=$(INC16);$(INC32);$(C16_ROOT)\include
						@echo cl -Fo$(OBJDIR)\postdos.obj postdos.c
                        $(DCC) postdos.c

clean:
    -@if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
    -@if exist $(OBJDIR)\*.vxd del $(OBJDIR)\*.vxd
    -@if exist $(OBJDIR)\*.exp del $(OBJDIR)\*.exp
    -@if exist $(OBJDIR)\*.lib del $(OBJDIR)\*.lib
    -@if exist $(OBJDIR)\*.map del $(OBJDIR)\*.map
    -@if exist $(OBJDIR)\*.sym del $(OBJDIR)\*.sym
    -@if exist $(OBJDIR)\*.exe del $(OBJDIR)\*.exe
    -@if exist $(OBJDIR)\*.dll del $(OBJDIR)\*.dll
    -@if exist $(OBJDIR)\*.res del $(OBJDIR)\*.res
    -@if exist $(OBJDIR)\nul   rd  $(OBJDIR)
    -@if exist *.bak del *.bak
