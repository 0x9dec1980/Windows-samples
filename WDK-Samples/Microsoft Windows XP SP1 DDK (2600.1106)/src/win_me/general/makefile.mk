#****************************************************************************
# This makefile is intended to be used as a boilerplate for any DDK projects*
# requiring the building of both a SYS and an Application (EXE) to be used  *
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

!INCLUDE $(BASEDIR)\inc\win_me\master.mk

RC          = rc.exe
CL          = cl -nologo

#       Definitions for the debug level

STD_DEFINES = -D_X86_=1 -Di386=1 -DSTD_CALL

C_DEFINES   = $(STD_DEFINES) $(C_DEFINES)

C_DEFINES   = $(C_DEFINES) -DCONDITION_HANDLING=1 -DNU_UP=1 -DNT_INST=0 -DWIN32=100 -D_NT1X=100 -DWINNT=1 -D_WIN32_WINNT=0x0400 -DWIN32_LEAN_AND_MEAN=1
CFLAGS      = -DWIN32 -D_IDWBUILD /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GF
LFLAGS      = /nologo -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -INCREMENTAL:NO -FORCE:MULTIPLE -RELEASE -FULLBUILD -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 -NODEFAULTLIB -version:4.00 -osversion:4.00 -PDB:NONE -MERGE:.rdata=.text -optidata
LPATHFLAGS  = /LIBPATH:$(DDKROOT)\lib /LIBPATH:$(DDKROOT)\lib\win_me /LIBPATH:$(C32_ROOT)\lib
RCFLAGS     = -l 409 -r

CFLAGS      = $(CFLAGS) $(C_DEFINES) $(INCFLAGS) $(CDEBUG)
LFLAGS      = $(LFLAGS) $(LDEBUG)
RCFLAGS     = $(RCFLAGS) $(C_DEFINES)

!ifdef DEBUG
CDEBUG      = -DDBG=1 -DFPO=0 /Z7 /Od /Oi /Oy-
LDEBUG      = -debug:notmapped,FULL -debugtype:both
!else
CDEBUG      = -DFPO=1 /Oxs /Oy
LDEBUG      = -debug:notmapped,MINIMAL -debugtype:coff
!endif

