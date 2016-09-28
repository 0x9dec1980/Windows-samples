*****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
*                                                                           *
*****************************************************************************

To call system VxDs from VxDs written in C, service wrappers can be used.

Wrappers are provided for some of the services in the following 
Windows 95 VxDs:

CONFIGMG - Config Manager Virtual Device
IOS      - I/O Supervisor Virtual Device
SHELL    - Shell Virtual Device
VCOMM    - COMM Virtual Device
VMM      - Virtual Machine Manager (Mother of all VxDs)
VMMREG   - Registry Services Virtual Device
VPOWERD  - Advanced Power Management (APM) Virtual Device
VXDLDR   - VxD Loader Virtual Device

The MAKEFILE is provided to demonstrate how wrappers are built
into a library which can then be linked with the C VxD object
files.

The MAKEFILE (as is) will build a new copy of VXDWRAPS.CLB (a COFF
version of all the provided wrappers) a copy of VXDWRAPS.CLB can
be found in the LIB directory.

This MAKEFILE is only known to work with the MS Visual C++ 2.0 (or 
better) 32-bit COFF librarian.

The LOCAL.INC file must be included in any wrapper that is created.
The appropriate .inc file (which defines the desired service) must be
included (either in local.inc or the created wrapper source).
The wrapper can then be compiled using the desired SEGNUM value, and
is then ready to be linked in w/ the C VxD that calls it.

WRAPPERS.TXT contains a list of all provided wrappers.
