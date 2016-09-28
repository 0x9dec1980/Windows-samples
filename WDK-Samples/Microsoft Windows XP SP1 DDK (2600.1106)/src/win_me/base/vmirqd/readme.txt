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

VMIRQD

This VxD demonstrates the virtualizing of a hardware interrupt (IRQ).
You have to have a secondary monitor on your system to use this
VxD. To install this, you need to put the following lines in
your SYSTEM.INI file:


DebugMono=TRUE
VMIRQD_IRQ=x
device=vmirqd.vxd

where nnnn is the IRQ number to monitor. 

The technique use will only work for UNVIRTUALIZED IRQs. Most of
the hardware interrupts that Windows knows about (keyboard, timer,
mouse) use IRQs that are already virtualized, and thus can not be 
monitored by this VxD. But for a simple demonstration of this 
program, try IRQ 6, the floppy controller IRQ. This IRQ was 
not virtualized in the retail release of Windows 3.0 and 3.1.
