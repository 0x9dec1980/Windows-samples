Generic Port I/O Driver

Included with this file:
     a Windows NT device driver to provide simple access to 8/16/32 bit IO
     ports using IOCTL calls.

     some simple test programs which read from ports and write to them. 

This sample is based on a driver submitted by Robert R. Howell, which in turn
was based on the adlib driver in the DDK.  We cleaned up the comments, tested
dynamic loading/unloading and added registry support.  Any questions regarding
this sample should be directed to Microsoft.

This driver provides an example of a minimal driver.  Neither it, nor the
sample programs are intended to be used in a production environment.  Rather,
they are intended as a skeleton driver for those devices which do not use
interrupts or DMA.

===============================================================
BUILDING THE DRIVER AND EXAMPLES:
---------------------------------------------------------------
To compile the device driver:
 
    1) Run the "SETENV.BAT" which should be in the MSTOOLS directory.  This 
       sets basic environment variables needed by NT.  The program does not 
       require any arguments. 

    2) Run the "SETENV.BAT" which should be in the DDK directory.  This sets 
       the environment variables needed by the DDK programs.  It does require 
       command line arguments.  It will print out instructions if run without
       arguments.

    3) Run "BUILD".  This invokes the Microsoft make routines.
       They produce log files called BUILD.WRN, and BUILD.LOG.
       The WRN file will contain warnings that no components have been
       extracted from certain libraries, even if the build succeeds.
       If it does succeed the driver will be named genport.sys and
       can be found in the I386 subdirectory created by BUILD.

    4) Copy the genport.sys file from the obj\I386 subdirectory created by
       BUILD to the \WINNT\SYSTEM32\DRIVERS directory where all the NT device
       drivers are stored.  If the system responds  "ACCESS DENIED" then a
       previous version of the file is currently in use, and NT will not let
       you replace it.  You can unload it by typing "net stop genport".

---------------------------------------------------------------------------
To compile the NT test programs:

    2) Run "SETENV.BAT" which is in the MSTOOLS directory.  It sets the 
       environment variables needed by the SDK programs.

    3) Run "NMAKE -f gptest.mak".  It will build two programs:  gpdread and
       gpdwrite.  If you just type the program name without any parameters,
       they will print instructions.

------------------------------------------------------------------------
To install the Generic Port I/O driver (GENPORT) use the REGINI utility to
create the entries listed in the genport.ini file.

--------------------------------------------------------
To use the IOCTL directly, see the comments in the example programs.

They
      1) Open "\\.\GpdDev" using CreateFile.
      2) Read from the port by placing the relative address (0, 1, 2, .. )
         into a buffer, calling an IOCTL, then reading a buffer.
      3) Write to the port by placing the relative address and a 
         value into a buffer then calling an IOCTL. 
      4) Close the device with  CloseHandle.
