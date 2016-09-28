DRVNAME				= GLINT
DRVNAME32			= gldd32
OVERRIDE_RCHEADERS	= 1


VERSIONHDR			= $(DDKROOT)\inc\win_me\inc16\version.h
RCINCLUDE			= $(DDKROOT)\inc\win_me\inc16\

DRVLIBS32	= 

SOURCES32 = debug.c \
			dllMain.c \
			ddhalcb.c \
			ddinit.c \
			ddsurfcb.c \
			ddutil.c   \
			p2ddblt.c \
			p2hw.c	\
			p2hwmisc.c \
			p2cntxt.c \
			p2d3dln.c \
			p2d3dpt.c \
			p2d3dtri.c \
			p2d3dhw.c \
 			d3d.c	  \
 			d3dutil.c \
 			d3dtxman.c \
 			d3dstate.c \
 			d3dresrc.c \
 			d3ddp2.c
	

	

#Specify tools here
OVERRIDE_TOOLS	= 1
ASM				= $(MASM_ROOT)\bin\ml
ASMENV			= ML


MYMAPSYM		= $(DDKROOT)\bin\win_me\bin\mapsym
MYCC			= $(C32_ROOT)\bin\cl
LINK32			= $(C32_ROOT)\bin\link
MYRC                    = $(MSDEVDIR)\bin\rc


#specify C32 flags here
MYC32_FLAGS		= -W3 -c -DWIN32 -D_X86_ -Oi -Zi  -DBUILDDLL -DIS_32 -bzalign -DP2WIN9X -DDDFLIP_INTERVALN

!if "$(VERDIR)" == "debug32"
MYC32_FLAGS  = $(MYC32_FLAGS) -DDEBUG -Fc
!endif

CINCLUDES = /I.. /I..\..\inc    
CINCLUDES = $(CINCLUDES) /I$(DXDDKROOT)\inc  /I$(C32_ROOT)\include /I$(DDKROOT)\inc\win_me

OVERRIDE_C32FLAGS = $(MYC32_FLAGS) $(CINCLUDES)

#specify masm flags here
OVERRIDE_A32FLAGS = 

#specify path to libs for the linker
OVERRIDE_MYLIB = $(DDKROOT)\lib\win_me;$(DXSDKROOT)\lib;$(C32_ROOT)\lib;

## include common mini driver include file
DISPLAY_MINI_DIR=..\..
!include $(DISPLAY_MINI_DIR)\mini.mk

