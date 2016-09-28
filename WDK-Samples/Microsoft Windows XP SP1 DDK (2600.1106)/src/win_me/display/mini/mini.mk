#############################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1991
#   All Rights Reserved.
#
#   MINI display driver makefile include
#
#   USAGE:
#
#   to use this makefile to build a Windows95 mini driver
#   define the needed macros and include this file.
#
#   NOTE your driver does not need a .RC file
#
#   REQUIRED MACROS
#
#       DRVNAME         the basename of your driver (ie vga)
#       SOURCES         the list of files needed to build your driver
#
#   OPTIONAL MACROS
#
#       DESCRIPTION     this becomes the descripton in the version resource.
#       DRVFILE         final name for the .DRV file, defaults to $(DRVNAME).drv
#       DRVDEF          the .def file to use, if not specifed a default will be used
#       DRVLIBS         any extra libs you need
#       DRVRESFILE      a .rc file
#	NOINCBINS	Do not include config.bin, fonts.bin, and colortab.bin
#
#############################################################################

!ifndef DRVNAME
!error DRVNAME not defined
!endif

!ifndef DESCRIPTION
DESCRIPTION=$(DRVNAME) Windows 95 Display Driver
!endif


!ifdef DXDDKROOT

!ifndef OVERRIDE_RCHEADERS
VERSIONHDR=$(DDKROOT)\inc16\version.h
RCINCLUDE=$(DXDDKROOT)\inc
!endif

!else
!error DXDDKROOT not defined
!endif


!ifndef DISPLAY_MINI_DIR
DISPLAY_MINI_DIR=.. 
!endif

#############################################################################
#
#   VERDIR is defined, build a 16 or 32 bit driver
#
#############################################################################

!ifdef VERDIR

!if DEFINED(IS_32) || "$(VERDIR)" == "debug32" || "$(VERDIR)" == "retail32"
!include $(DISPLAY_MINI_DIR)\mini32.mk
!else
!include $(DISPLAY_MINI_DIR)\mini16.mk
!endif


#############################################################################
#
# build a res file with the correct version info
#
#############################################################################

!if EXIST($(SRCDIR)\$(DRVNAME).rc)
$(DRVRES): $(VERSIONHDR)
        $(MYRC) -fo$*.res -r -i$(RCINCLUDE) $(SRCDIR)\$(DRVNAME).rc
!else

$(DRVRES): $(VERSIONHDR)
        $(MYRC) -fo$*.res -r -i$(RCINCLUDE) <<$(DRVNAME).rc
!ifdef DRVRESFILE
!if EXIST($(SRCDIR)\$(DRVRESFILE))
        #include "$(SRCDIR)\$(DRVRESFILE)"
!else
        #include "$(SRCDIR)\$(DRVNAME).rch"
!endif
!endif

#include <version.h>


#define VER_FILETYPE                    VFT_DRV
#define VER_FILESUBTYPE                 VFT2_DRV_DISPLAY
#define VER_FILEDESCRIPTION_STR         "$(DESCRIPTION)"
#define VER_INTERNALNAME_STR            "$(DRVNAME)"
#define VER_ORIGINALFILENAME_STR        "$(DRVFILE)"
#define VER_LEGALCOPYRIGHT_YEARS        "1992-" VER_COPYRIGHT_CURRENT_YEAR

#include <common.ver>

//
// we need to include the OEM resources to get LapLink to work
//

!ifndef NOINCBINS
1       oembin      PRELOAD     "$(SRCDIR)\\..\\res\\config.bin"
fonts   oembin      PRELOAD     "$(SRCDIR)\\..\\res\\fonts.bin"
2       oembin                  "$(SRCDIR)\\..\\res\\colortab.bin"
!endif

<<KEEP
!endif


#############################################################################
#
#   VERDIR is not defined, set the goals and recurse
#
#############################################################################

!else   ## VERDIR

!if "$(BUILD)" == "ALL" || "$(BUILD)" == "all"
goal:   debug.a debug32.a retail.a retail32.a
!else
goal:   debug.a debug32.a
!endif

retail32 debug32:
!ifdef SOURCES32
        @echo ***** Building $* version of $(DRVNAME)
        @if not exist $*\nul mkdir $*
        @$(MAKE) /nologo VERDIR=$*
!else
        @echo ***** not building $* version of $(DRVNAME)
!endif

retail debug: 
!ifdef SOURCES
        @echo ***** Building $* version of $(DRVNAME)
        @if not exist $*\nul mkdir $*
        @$(MAKE) /nologo VERDIR=$*
!else
        @echo ***** not building $* version of $(DRVNAME)
!endif

retail32.a debug32.a:  
!ifdef SOURCES32
        @echo ***** Building $* version of $(DRVNAME)
        @if not exist $*\nul mkdir $*
        @$(MAKE) /nologo VERDIR=$*
!else
        @echo ***** not building $* version of $(DRVNAME)
!endif

retail.a debug.a: 
!ifdef SOURCES
        @echo ***** Building $* version of $(DRVNAME)
        @if not exist $*\nul mkdir $*
        @$(MAKE) /nologo VERDIR=$*
!else
        @echo ***** not building $* version of $(DRVNAME)
!endif

depend:
        @echo depend not supported yet

clean:
!if EXIST(debug)
    echo Y | del debug\*.* > nul
        rmdir debug
!endif
!if EXIST(debug32)
    echo Y | del debug32\*.* > nul
        rmdir debug32
!endif
!if EXIST(retail)
    echo Y | del retail\*.* > nul
        rmdir retail
!endif
!if EXIST(retail32)
    echo Y | del retail32\*.* > nul
        rmdir retail32
!endif
!endif
