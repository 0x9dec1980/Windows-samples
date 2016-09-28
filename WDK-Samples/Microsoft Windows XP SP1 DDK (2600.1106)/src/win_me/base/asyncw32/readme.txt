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

This sample demonstrates how to communicate asynchronously with a 
VxD from WIN32, using DeviceIoControl interface.


This sample is comprised of the following files:

CON_AW32.C      - Application front end that will dynamically load
                  the C VxD sample, call it, and display data
                  returned from the C VxD.

ASYNCW32.C      - Sample VxD written in C. This VxD supports the
                  WIN32 DeviceIOControl interface which is used by 
                  WIN32 to communicate with VxDs. This VxD can be
                  loaded both dynamically and statically. The VxD
                  will communicate asynchronously with CON_AW32.EXE

SYSCTRL.ASM     - Part of sample VxD which contains the Real Mode
                  init segment (which is 16-bit) and the
                  SystemControl message dispatcher.

MAKEFILE        - Makefile for building the C VxD sample. This sample
                  was built using the latest Microsoft Visual C++
                  tools as well as Microsoft Masm 6.11c. The MSVC 
                  linker will display several LNK4078 warnings during 
                  linking, these are expected and can be ignored.

WRAPPERS.ASM    - Wrapper for VWIN32_DIOCCompletionRoutine.

MYLOCAL.INC     - Private version of LOCAL.INC (found in BASE\VXDWRAPS)
                  which is used to build the created wrapper for
                  linking with C VxD object.
