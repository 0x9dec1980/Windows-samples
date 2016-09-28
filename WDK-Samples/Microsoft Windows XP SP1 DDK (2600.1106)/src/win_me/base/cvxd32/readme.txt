*****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright 1993-95  Microsoft Corporation.  All Rights Reserved.           *
*                                                                           *
*****************************************************************************

This sample demonstrates:

        - How to write a VxD in C.
        - How to communicate with VxDs from WIN32 (using
                DeviceIoControl interface).
        - How to write wrappers for VxD services for which there
                are currently no wrappers provided.


This sample is comprised of the following files:

CON_SAMP.C      - Application front end that will dynamically load
                  the C VxD sample, call it, and display data
                  returned from the C VxD.

CVXDSAMP.C      - Sample VxD written in C. This VxD supports the
                  WIN32 DeviceIOControl interface which is used by 
                  WIN32 to communicate with VxDs. This VxD can be
                  loaded both dynamically and statically.

CVXDCTRL.ASM    - Part of sample VxD which contains the Real Mode
                  init segment (which is 16-bit) and the
                  SystemControl message dispatcher.

MAKEFILE        - Makefile for building the C VxD sample using both
                  the MS Visual C++ 2.0 Linker and Link386.

CVXDSAMP.DEF    - Module Definition file used by both the MS Visual 
                  C++ 2.0 Linker and Link386. The MSVC20 linker will
                  display warnings during linking, these are expected
                  and can be ignored for this sample.

CVXDSAMP.LNK    - Link file used by Link386.

VKDGKO.ASM      - Wrapper for VKD_Get_Kbd_Owner.

MYVKD.H         - Private conversion of VKD.INC file to a .H file.

MYLOCAL.INC     - Private version of LOCAL.INC (found in BASE\VXDWRAPS)
                  which is used to build the created wrapper for
                  linking with C VxD object.

VKDWRAPS.H      - Include file that defines VKD wrappers interface. 
                  This file is included by the CVXDSAMP.C file. 
