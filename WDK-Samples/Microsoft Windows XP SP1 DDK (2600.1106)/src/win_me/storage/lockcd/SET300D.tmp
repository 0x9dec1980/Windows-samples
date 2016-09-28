#****************************************************************************
# This makefile is intended to be used as a boilerplate for any DDK projects*
# requiring the building of both a VxD and an Application (EXE) to be used  *
# together. It assumes you have configured SETENV.BAT and executed it       *
#                                                                           *
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
# PURPOSE.                                                                  *
#                                                                           *
# Copyright 1993-2000  Microsoft Corporation.  All Rights Reserved.         *
#                                                                           *
#****************************************************************************

BUILD_BITS=32
BUILD_TYPE=base
LOCAL_CLEAN=1

# Set some local settings specific to this app
APP         = LOCKCD
DEVICE      = LDVXD
OBJS        = bin\$(APP).obj
VXDOBJS     = bin\ldioctl.obj bin\ldios.obj bin\$(DEVICE).obj
INCFLAGS    = -I.\INC -I$(DDKROOT)\inc\win_me -I$(DDKROOT)\inc\win_me\inc16
ASMINCFLAGS = -I..\INC $(INCFLAGS)
RCINCFLAGS  = -I$(DDKROOT)\inc\win_me -I$(DDKROOT)\inc\win_me\inc16
LPATHFLAGS  = /LIBPATH:$(DDKROOT)\lib /LIBPATH:$(DDKROOT)\lib\win_me /LIBPATH:$(C32_ROOT)\lib
CFLAGS      = -nologo -DWIN32 -DCON -Di386 -D_X86_ -DIS_32 -W3 -Gs -D_DEBUG -Zp -I$(C32_ROOT)\include $(INCFLAGS) -I$(BASEDIR)\src\win_me\block\inc
CVXDFLAGS   = -nologo -Zdp -Gs -c -DIS_32 -Zl -DDEBLEVEL=1 -DWANTVXDWRAPS $(INCFLAGS) -I$(BASEDIR)\src\win_me\block\inc
RCFLAGS     = -x -v -r
LFLAGS      = /nologo /machine:i386 /debug:notmapped,full /PDB:NONE /debugtype:cv /subsystem:console $(LPATHFLAGS) kernel32.lib 
LVXDFLAGS   = /nologo /MACHINE:i386 /DEBUG:NONE /PDB:NONE /NOD  /ignore:4078 $(LPATHFLAGS) kernel32.lib libc.lib /vxd
AFLAGS      = /nologo /coff /DBLD_COFF /DIS_32 /W2 /Zd /c /Cx /DMASM6 /DDEBLEVEL=1 /DDEBUG $(ASMINCFLAGS)

# Older resource compiler that creates 8-bit characters
MYRC        = $(DDKROOT)\bin\win_me\bin16\rc.exe

# Should find this one in the path to your Visual Studio installation.
# Generates UNICODE format .RES
WIN32RC     = rc.exe

#LIB=$(DDKROOT)\lib 

all: direxist bin\$(APP).exe bin\$(DEVICE).vxd

direxist:
        -@if not exist bin\nul md bin

# Makefile directives for building the VxD

bin\ldioctl.obj: vxd\ldioctl.c
        cl $(CVXDFLAGS) -Fo$@ %s

bin\ldios.obj: vxd\ldios.asm
        $(MASM_ROOT)\bin\ml $(AFLAGS) -Fo$@ %s

bin\$(DEVICE).obj: vxd\$(DEVICE).asm
        $(MASM_ROOT)\bin\ml $(AFLAGS) -DSEGNUM=1 -Fo$@ %s

bin\$(DEVICE).res: 
        $(MYRC) $(RCINCFLAGS) $(RCFLAGS) -fo $@ $(DEVICE).rc
        
bin\$(DEVICE).vxd: $(VXDOBJS) bin\$(DEVICE).res
        link @<<$(DEVICE).lnk /def:$(DEVICE).def
$(LVXDFLAGS)
/out:bin\$(DEVICE).vxd
/map:bin\$(DEVICE).map
$(VXDOBJS)
vxdwraps.clb
<<

        $(C16_ROOT)\bin\adrc2vxd.exe $@ bin\$(DEVICE).res
        mapsym -s -o bin\$(DEVICE).sym bin\$(DEVICE).map
        del bin\$(DEVICE).exp
        del bin\$(DEVICE).lib

# Makefile directives for building the application itself

bin\$(APP).res:
        $(WIN32RC) $(RCFLAGS) -fo $@ $(APP).rc
        
bin\$(APP).obj: app\ld.c
        cl $(CFLAGS) -c -Fo$@ %s

bin\$(APP).exe: bin\$(APP).obj bin\$(APP).res
        link @<<$(APP).crf
$(LFLAGS)
-OUT:bin\$(APP).exe -MAP:bin\$(APP).map bin\$(APP).obj bin\$(APP).res
<<

######
clean:
        -@if exist bin\*.obj del bin\*.obj
        -@if exist bin\*.vxd del bin\*.vxd
        -@if exist bin\*.exe del bin\*.exe
        -@if exist bin\*.map del bin\*.map
        -@if exist bin\*.sym del bin\*.sym
        -@if exist bin\*.res del bin\*.res
        -@if exist bin\*.lib del bin\*.lib
        -@if exist bin\*.exp del bin\*.exp
        -@if exist bin\R*    del bin\R*
        -@if exist bin\nul rd  bin
        -@if exist *.lst del *.lst
        -@if exist *.pdb del *.pdb
        -@if exist *.bak del *.bak
