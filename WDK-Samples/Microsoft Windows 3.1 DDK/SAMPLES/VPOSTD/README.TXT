
VPOSTD.386


VPOSTD.386, POSTHOST.DLL, POSTAPP.EXE, and POSTDOS.EXE
======================================================

This .ZIP file contains source demonstrating how a VxD can 
cause a message to be posted to a Windows application.  This
example also demonstrates how to implement an API for Real
and Protect mode to a VxD... among other things.

There are four separate programs for this example:

    VPOSTD.386  :   The VxD that coordinates activities.

        VPOSTD.ASM  :   Main body of VxD.
        VPOSTD.INC  :   Header file for VPOSTD.ASM.
        VPOSTD.DEF  :   Definition file for the VPOSTD VxD.
        VPOSTD.LNK  :   Link file for building the VxD.


    POSTHOST.DLL:   The 'host' .DLL that VPOSTD.386 calls for
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


    POSTDOS.EXE :   DOS program that calls VPOSTD.386 to post
                    messages to POSTAPP.EXE through POSTHOST.DLL.

        POSTDOS.C   :   Main body of POSTDOS.EXE.


The MAKEFILE included will build all of these.  Note that this
is a makefile for NMK or NMAKE.


To run this example, you need to do the following (if you goof
something up, it'll tell you about it):

    o   Install VPOSTD.386:
        --  copy VPOSTD.386 to your WINDOWS\SYSTEM directory
        --  put 'device=VPOSTD.386' in your SYSTEM.INI file under
            the [386Enh] section
        --  reboot Windows

    o   Run POSTAPP.EXE.  This will cause POSTHOST.DLL to be loaded.

    o   Now you can run POSTDOS.EXE in any DOS box--windowed or
        full screen.  Note that a System Modal dialog box is used.


Refer to the source code for details on how this all works.


Enjoy, Curt.
5/4/91
