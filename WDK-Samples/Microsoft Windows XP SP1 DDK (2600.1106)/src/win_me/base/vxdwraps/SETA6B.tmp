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

BUILD_BITS=32
BUILD_TYPE=base
!INCLUDE $(BASEDIR)\inc\win_me\master.mk


# filter empty NMAKE command option
MKFLAGS = $(MAKEFLAGS: =)
!if "$(MKFLAGS)" != ""
MKFLAGS =-$(MAKEFLAGS)
!endif


DIRLIST = _mtrr _pccard _cmd _ios _pci _ntkern _vpicd _vdmad _vxdldr _vpowerd \
	_shell _vmmreg _vwin32 _vtd _acpi _vcomm _vmm _configmg


target: $(DIRLIST) 

$(DIRLIST):
        cd $(@:_=)
        $(MAKE) $(MKFLAGS) $(DO)
        cd ..

clean:
    $(MAKE) DO="clean"
