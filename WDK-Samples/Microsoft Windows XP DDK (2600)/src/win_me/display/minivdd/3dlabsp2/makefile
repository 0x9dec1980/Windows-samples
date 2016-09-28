##############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1999
#	All Rights Reserved.
#
#	Makefile for Permedia2 MiniVDD directory
#                                                                          
#############################################################################

VXDNAME	= GLINTMVD
SOURCES = glintmvd.asm i2c.c ddkernel.c devctrl.c hardware.c video.c 
SOURCES	= $(SOURCES) vesa.c vesaddc.c writecmb.c mtrr.asm init.c reglog.c

BOARD	= GLINT

ASM			= $(MASM_ROOT)\bin\ml

ADDRC		= $(DDKROOT)\bin\win_me\bin\adrc2vxd

MYMAPSYM	= $(C32_ROOT)\bin\mapsym

MYRC		= $(DDKROOT)\bin\win_me\bin16\rc

MYCC		= $(C32_ROOT)\bin\cl

LINK32		= $(C32_ROOT)\bin\link

VERSIONHDR	= $(DDKROOT)\inc\win_me\inc16\version.h
RCINCLUDE	= $(DDKROOT)\inc\win_me\inc16

VXDDIR = ..

MYAFLAGS	= -DDX5 -D$(BOARD) -coff -DBLD_COFF -nologo -W2 -Zd -c  -Cx -DMASM6 -Sg -DMINIVDD=1 -DWIN98 -DPOWER_STATE_MANAGE
MYAFLAGS  = $(MYAFLAGS) -I$(VXDDIR)\..\inc -I$(VXDDIR)\..\..\mini\3dlabsp2\inc 
MYAFLAGS  = $(MYAFLAGS) -I$(DDKROOT)\inc\win_me
OVERRIDE_AVXDFLAGS = $(MYAFLAGS)

MYCFLAGS	= -D_X86_ -DBLD_COFF -G5fyz -bzalign -W3 -Gs -nologo -Zp1 -Zl -c -Zi -DWIN98 -DPOWER_STATE_MANAGE
MYCFLAGS	= $(MYCFLAGS) -D$(BOARD) -DWANTVXDWRAPS -DWIN32 -DMINIVDD -DIS_32 -Oi 
MYCFLAGS    = $(MYCFLAGS) -I$(VXDDIR)\..\inc
MYCFLAGS	= $(MYCFLAGS) -I$(VXDDIR)\..\..\mini\3dlabsp2\inc 
MYCFLAGS	= $(MYCFLAGS) -I$(DDKROOT)\inc\win_me -D$(C32_ROOT)\include

OVERRIDE_CVXDFLAGS = $(MYCFLAGS) -DDDFLIP_INTERVALN -DUSE_HARDWARECURSOR

VXDLIBS = $(DDKROOT)\lib\win_me\vxdwraps.clb 
!include ..\mini_vdd.mk

