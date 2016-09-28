Build & Run Instructions:
-------------------------


The MONO sample is composed of a simple monolithic export driver
and a Win32 test application. The Win32 app retrieves a handle to
the driver/device by calling CreateFile("\\.\MONO", ...), where MONO
is a Win32 alias (or "symbolic link") for \Device\Mono, and
subsequently sends it I/O requests (IOCTLs) to have it perform I/O
on it's behalf.


NOTE: MONO is intended to be a generic monolithic driver sample;
      it just happens that it was coded to support a monochrome
      video adapter.

      Microsoft does not recommend the use this sample as a
      template for video driver development. We encourage developers
      to code within the stated video model; doing so will insure
      compatibility & portability, lessen development time,
      and produce smaller and more supportable code.


As mentioned above, MONO is an export driver; it provides a
MonoDbgPrint() API which may be called by other kernel mode drivers
that include a prototype & link with MONO.LIB, e.g. by adding
"TARGETLIBS=$(BASEDIR)\lib\*\mono.lib" to the sources file.
(This can make single machine driver debugging a little more bearable.
Note that the load order of the driver can be varied by changing
the Start value in the registry, 0 being the earliest starting
value. See Chapter 16 of the Kernel-mode Driver Design Guide for
mode information on driver load ordering.)

There are currently three export drivers that ship with NT (i.e.
VIDEOPRT.SYS and SCSIPORT.SYS); these represent the port side of
the port/miniport driver model. The idea of this model is to split
the OS-specific functionality into a common port driver, while the
H/W specific functionality resides in the miniport driver. This
way, miniports remain portable across various platforms & systems,
and a single port driver can service several miniports on a particular
platform.

The Win32 portion contains a file, MONOTEST.C, which attempts to
obtain a handle to MONO & send it IOCTLs.  The executable is built
using the Windows NT SDK.  First update the environment and path
by running <mstools>\setenv.bat.  Then change to the directory
where you have the C source code and the makefile.  Type
"nmake /f monotest.mak" to compile the Win32 program, MONOTEST.EXE.

The kernel driver portion contains the driver source code, MONO.C
and a text file used to configure your registry so that the driver
can be loaded.  The driver is built using the Windows NT DDK.

To build the driver:


    1. Assuming you have run <sdk_root>\setenv.bat and
       <ddk_root>\setenv.bat, build the driver by typing:

               build -cef

       (If there are any errors have a look at the build.log, build.err,
        and build.wrn files to get an idea of what went wrong.)


    2. Copy the newly built driver, <ddk_root>\lib\*\MONO.SYS to the
       <nt_root>\system32\drivers\ directory, i.e.:

               copy \ntddk\lib\i386\free\mono.sys c:\winnt\system32\drivers\


    3. Update the registry by running regini.exe on the mapmem.ini
       file, i.e.:

               regini mono.ini

       This adds a MONO driver key under the HKEY_LOCAL_MACHINE\
       SYSTEM\CurrentControlSet\Services tree in the registry. You
       can verify this by starting REGEDIT.EXE and looking in the
       appropriate place.


    4. Reboot.


    5. Type:

               net start mono

       ...and then execute MONOTEST.EXE.
