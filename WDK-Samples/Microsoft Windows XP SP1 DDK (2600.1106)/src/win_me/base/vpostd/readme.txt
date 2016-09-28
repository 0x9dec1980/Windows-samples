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

Note that Windows 4.0 provides a Shell_PostMessage VxD service which 
obviates the need for much of the codepresented here. It remains for 
illustrative purposes.

VPOSTD.VXD

VPOSTD.VXD, POSTHOST.DLL, POSTAPP.EXE, and POSTDOS.EXE
======================================================

This directory contains source demonstrating how a VxD can 
cause a message to be posted to a Windows application.  This
example also demonstrates how to implement an API for Real
and Protect mode to a VxD... among other things.

There are four separate programs for this example:

    VPOSTD.VXD  :   The VxD that coordinates activities.

        VPOSTD.ASM  :   Main body of VxD.
        VPOSTD.INC  :   Header file for VPOSTD.ASM.
        VPOSTD.DEF  :   Definition file for the VPOSTD VxD.
        VPOSTD.LNK  :   Link file for building the VxD.


    POSTHOST.DLL:   The 'host' .DLL that VPOSTD.VXD calls for
                    posting messages.

        POSTHOST.C  :   Main body of POSTHOST.DLL.
        POSTHOST.H  :   Public header file for POSTHOST.DLL.
        POSTHOST.DEF:   Definition file for POSTHOST.DLL.
        POSTHOST.LNK:   Link file for building POSTHOST.DLL.
        LIBENTRY.ASM:   Entry stub for .DLL's.

    POSTAPP.EXE :   The Windows app who receives messages.

        POSTAPP.C   :   Main body of POSTAPP.EXE.
        POSTAPP.H   :   Header file for POSTAPP.C.
        POSTAPP.RC  :   Resource file for POSTAPP.EXE.
        POSTAPP.DEF :   Definition file for POSTAPP.EXE.
        POSTAPP.ICO :   Stupid icon for POSTAPP.EXE.
        POSTAPP.LNK :   Link file for building POSTAPP.EXE.

    POSTDOS.EXE :   DOS program that calls VPOSTD.VXD to post
                    messages to POSTAPP.EXE through POSTHOST.DLL.

        POSTDOS.C   :   Main body of POSTDOS.EXE.


The MAKEFILE included will build all of these.  Note that this
is a makefile for NMK or NMAKE.

To run this example, you need to do the following (if you goof
something up, it'll tell you about it):

    o   Install VPOSTD.VXD:
        --  copy VPOSTD.VXD to your WINDOWS\SYSTEM directory
        --  put 'device=VPOSTD.VXD' in your SYSTEM.INI file under
            the [386Enh] section
        --  reboot Windows

    o   Run POSTAPP.EXE.  This will cause POSTHOST.DLL to be loaded.

    o   Now you can run POSTDOS.EXE in any DOS box--windowed or
        full screen.  Note that a System Modal dialog box is used.

Refer to the source code for details on how this all works.
