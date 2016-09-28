Using the VGA performance counters example

Overview:

This example is provided as a supplement to the VGA driver provided in
the DDK to demonstrate the addition of performance counters to a device
driver. The counter example will count the VGA driver's TextOut and 
BitBlt operations and provide this data to the performance section of the
registry by way of the VGACTRS.DLL. The easiest way to see these counters
is by running Perfmon after they have been installed. 

Building and Installing:

Before working with this sample, please refer to the DDK Programmer's Guide
for more details on performance counters.

    1.  Design the counter objects and counter names for the driver.
        One driver may have multiple counter objects, counters and 
        instances of counter objects as is appropriate. See Perfmon's
        counters for examples of each of these cases. 

        (This step has already been accomplished)


    2.  Create the driver name installation file and include file.

        These are the example files: 

            vgactrs\vgactrnm.h      ; include file for counter offsets
            vgactrs\vgactrs.ini     ; name and explain texd definitions

    3.  Add data collection functions, variables and interfaces to the
        device driver (see "tips" below).

        Replace the following files in the VGA driver example with
        these files and re-build the VGA.DLL file. Replace the current
        VGA.DLL in the system32 directory with this one.

            vgacode\driver\enable.c    ; interface initialization code
            vgacode\driver\bitblt.c    ; bitblt counter increment
            vgacode\driver\textout.c   ; textout counter increment

    4.  Create the Counter DLL routines (Open, Collect and Close) and
        any requisite data structures (see "tips" below).

        Build the vgactrs directory which will produce the vgactrs.dll
        and copy the .dll into the system32 directory.

    5.  Update the registry entries. For testing this can be done 
        manually, however, for "production", these modifications should
        be done by Setup when the driver and the counter DLL's are 
        installed. The registry  entries include AT LEAST:

                For loading of your driver's performance counter 
                DLL and routines, make the following modifications 
                to the registry using regedt32:
            
        MACHINE
            SYSTEM
                CurrentControlSet
                    Services
                        VGA
                            Performance
                                :Library=vgactrs.dll
                                :Open=OpenVgaPerformanceData
                                :Collect=CollectVgaPerformanceData
                                :Close=CloseVgaPerformanceData

                For the event logger to recognize your driver's
                error messages and display the corresponding text,
                make the following modifications to the registry using
                regedt32:

        MACHINE
            SYSTEM
                CurrentControlSet
                    Services
                        EventLog
                            Application
                                VgaCtrs
                                    :EventMessageFile=
                                        %systemroot%\system32\vgactrs.dll
                                    :TypesSupported=0x07
                

    6.  Load the counter names and explain text into the registry
        using the LODCTR utility and the corresponding .INI file.

            > lodctr vgactrs.ini

At this point all the software is installed and the system will need to be
rebooted for the changes to take effect (more specifically for the new
VGA driver and counter .DLL to be loaded).

Be sure to reboot between VGACTRS.DLL changes if it is necessary to 
recompile the Counter DLL since SCREG.EXE will keep the old version 
opened until the system is rebooted. 


List of files supplied in this example:

README.TXT
    this file.

VGACODE
    directory of modified VGA driver files. These files should replace
    the corresponding files in the VGA driver example in order to build
    a VGA driver that provides performance statistics.

    \enable.c
        modified VGA file that contains the initialization of a mapped
        memory section for use by the VGACTRS.DLL to read the values of
        the performance counters collected  by the VGA driver.

    \bitblt.c
        modified VGA file that increments the bitblt counter every time
        the bitblt routine is called

    \textout.c
        modified VGA file that increments the textout counter every time
        the textout routine is called.

VGACTRS
    the directory containing the extensible performance counter routines 
    and initialization files for the VGA driver counters.

    \datavga.h
        data structure and offset constant definitions for the data 
        block returned to the registry by the VGA extensible counter 
        routines.

    \datavga.c
        the declaration and initialization of the static data template
        used to format the counter data for return to the registry

    \makefile
        the generic makefile used to build the example code.

    \makefile.inc
        the addition to the standard makefile that compiles the 
        message file when necessary

    \perfmsg.h
        macros and constants used for formatting and passing messages
        to the event logging facility

    \perfutil.c
        utility routines used by the counter functions

    \perfutil.h
        utility routine prototypes and constant defintions used by the
        counter routines.

    \perfvga.c
        Counter routines used by the registry to open and collect data
        from the VGA driver.

    \sources
        list of files and environment variables to use when compiling
        source code

    \vgactrnm.h
        symbolic constants used by extensible counter routines and 
        registry initialization files that indicate the relative value
        of the counter and object names and explain text.

    \vgactrs.mc
        message definition file for logging error events to the event
        logger

    \vgactrs.ini
        explain text and counter name definition file used by lodctr 
        utility to add these entries to the registry.
