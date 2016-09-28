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

The RC file and makefile in this directory can be used to add
version information to a virtual device which can then be 
displayed in device manager.

The makefile is used within the 16-bit environment. The resource
compiler which is used is the RC.EXE which is found in the 
SDK\BINW16 directory. With the exception of VMM.H which is found
in the DDK\INC32 directory, all required include files are found 
in the DDK\INC16 and SDK\INC16 directories. 

Refer to DDK\INC16\COMMON.VER for more information on creating
the .RC file for adding version info to VxDs.

COMMON.VER and VERSION.H contain string defines that mention
Microsoft Corporation. These should be changed to whatever
company name is appropriate.
