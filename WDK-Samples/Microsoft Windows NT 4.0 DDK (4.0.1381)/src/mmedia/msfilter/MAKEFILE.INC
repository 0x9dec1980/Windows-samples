#==========================================================================;
#
#  Copyright (c) 1994  Microsoft Corporation.  All Rights Reserved.
#
#--------------------------------------------------------------------------;
#
#  makefile.inc
#
#  Description:
#      This file is used by makefile.def to rename the codec from a .dll
#      extension to a .acm extension.
#
#
#==========================================================================;

.SUFFIXES: .dll

.dll.acm:
	copy $** $@ >NUL
