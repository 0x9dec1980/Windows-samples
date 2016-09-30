RAM Disk Driver
---------------

This sample demonstrates a software only Windows NT Device Driver.  This driver
is provided for educational purposes only.  Generally the Windows NT cache
manager does a much better job of optimizing memory usage than using a RAM disk.


Building and Installing
-----------------------

    1)  Set the environment variables by running the SDK SETENV.BAT and the DDK
        SETENV.BAT.

    2)  Run BUILD.EXE with the -cef options.

    3)  The driver will be built in the OBJ\I386 directory, copy it to
        %WINDIR%\SYSTEM32\DRIVERS.

    4)  Edit RAMDISK.INI, change parameters to match your preferences.

    5)  Run REGINI.EXE with RAMDISK.INI as an argument.

    6)  Reboot and enjoy.


Configuring the RAM disk
------------------------

The RAM disk can be configured using the registry.  The RAMDISK.INI file
contains the default values for all the configurable parameters.  A brief
description of each parameter follows:

Name                Type        Default Value
====                ====        =============

BreakOnEntry        REG_DWORD   0

    Boolean value which determines whether a break point will be generated
    during the DriverEntry routine.  It has no effect in a free build of the
    driver.

DebugLevel          REG_DWORD   0

    This value specifies the level of diagnostic messages produced.  Larger
    values result in more verbose messages.  It has no effect in a free build
    of the driver.

DiskSize            REG_DWORD   0x100000 (1,048,576 1Mb)

    The size of the RAM disk in bytes.

DriveLetter         REG_SZ      Z:

    The drive letter associated with the RAM disk.

RootDirEntries      REG_DWORD   512

    The number of entries in the root directory.

SectorsPerCluster   REG_DWORD   2

    The granularity of the allocation.
