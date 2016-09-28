#############################################################################
#       Copyright (C) Microsoft Corporation 1991-1997
#       All Rights Reserved.
#
#       MINI project makefile include
#
#############################################################################

DRVNAME     = GLINT


DESCRIPTION = DIB Engine based Permedia2 driver.

DX5=1
DX6=1
L16DEF  = /CODEVIEW
DEF = /DGLINT
DEF16 =  -DPERMEDIA_ROUTINES -DCONTROL_ROUTINES -DDIRECT3D 
!ifdef DX5
DEF = $(DEF) /DDX5
!endif
!ifdef DX6
DEF = $(DEF) /DDX6
!endif

DRVDEF  = GLINT.def



OVERRIDE_RCHEADERS = 1
VERSIONHDR	= $(DDKROOT)\inc\win_me\inc16\version.h
RCINCLUDE	= $(DDKROOT)\inc\win_me\inc16

OVERRIDE_TOOLS = 1
ASMENV		= ML
ASM         = $(MASM_ROOT)\bin\ml
MYMAPSYM    = $(C16_ROOT)\bin\mapsym
MYRC		= $(C16_ROOT)\bin16\rc
MYCC        = $(C16_ROOT)\bin16\cl
LINK16      = $(C16_ROOT)\bin16\link

ASMINCLUDES = /I.. /I..\..\inc /I$(DXDDKROOT)\inc /I$(DXDDKROOT)\..\minivdd\inc
ASMINCLUDES = $(ASMINCLUDES) /I$(DDKROOT)\inc\win_me 
ASMINCLUDES = $(ASMINCLUDES) /I$(DDKROOT)\inc\win_me\inc16 

OVERRIDE_A16FLAGS =  /nologo /c /Zi /Zd /Zm /Cp /D$(DRVNAME) $(ASMINCLUDES)

CINCLUDES = /I$(DDKROOT)\inc\win_me\inc16 
CINCLUDES = $(CINCLUDES) /I$(DDKROOT)\inc\win_me 
CINCLUDES = $(CINCLUDES) /I..\..\inc /I$(DXDDKROOT)\inc /I$(DXDDKROOT)\..\minivdd\inc

OVERRIDE_C16FLAGS = /Fc /c /W3 /Ox /G3 /ND_DATA /DDIRECT3D /DW95_DDRAW /NT_TEXT /ALu /Ox /Zi /DIS_16 /D$(DRVNAME) /DUSE_HARDWARECURSOR $(CINCLUDES)

OVERRIDE_L16FLAGS = /NOE/NOD/LI/MAP/AL:4096/NOPACKC /CODEVIEW

OVERRIDE_MYLIB = $(DDKROOT)\lib\win_me;$(C16_ROOT)\lib
OVERRIDE_DRVLIBS = libw.lib dibeng.lib ldllcew.lib
#############################################################################
#
#   16bit driver files
#
#############################################################################

SOURCES =	diblink.asm disable.asm enable.asm getresid.asm polyscan.asm polyline.asm \
			init.asm contxt16.asm GDIInter.asm cursor.asm glintblt.asm object.asm \
			inkready.asm palette.asm sswitch.asm glinthw.asm polygon.asm \
			access.asm text.asm text32.asm output.asm blt16.asm dibtodev.asm \
			pointer.c  heap.c drv2.c gldd.c debug.c


## include common mini driver include file
DISPLAY_MINI_DIR=..\..
!include $(DISPLAY_MINI_DIR)\mini.mk

