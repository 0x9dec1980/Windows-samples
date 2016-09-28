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

LINK16 = link  
CL16   = cl
RC16   = rc 

# Standard Windows make file.  The make utility compares the
# creation date of the file to the left of the colon with the file(s)
# to the right of the colon.  If the file(s) on the right are newer
# then the file on the left, Make will execute all of the command lines
# following this line that are indented by at least one tab or space.
# Any valid MS-DOS command line may be used.

# This line allows NMAKE to work as well

all: dosacc.exe winacc.exe


# Update the resource if necessary

winacc.res: winacc.rc winacc.h
    $(RC16) -r winacc.rc


# Update the object file if necessary

winacc.obj: winacc.c winacc.h
    $(CL16) -c -AS -Gsw -Oas -W3 -Zpei winacc.c


# Update the executable file if necessary, and if so, add the resource back in.

winacc.exe: winacc.obj winacc.def winacc.res
    $(LINK16) /CO/NOD winacc,,, libw slibcew, winacc.def
    $(RC16) winacc.res


# If the .res file is new and the .exe file is not, update the resource.
# Note that the .rc file can be updated without having to either 
# compile or link the file.

#winacc.exe:: winacc.res
#   $(RC16) winacc.res

dosacc.obj: dosacc.asm vdevice.inc
    ml -c -Cx -Fodosacc.obj dosacc.asm

dosacc.exe: dosacc.obj
    $(LINK16) dosacc.obj;
