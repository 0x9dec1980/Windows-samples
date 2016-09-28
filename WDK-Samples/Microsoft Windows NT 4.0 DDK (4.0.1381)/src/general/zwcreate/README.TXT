This sample demonstrates how to use ZwCreateFile to obtain a handle to
a file in a kernel-mode device driver.  It also shows how to read the
contents of this file using ZwReadFile.

The device driver sample, ZWCFTEST.SYS, is built using the Windows NT
DDK.  A Free or a Checked version of the driver can be built, but a
checked version of the driver is required for source level
debugging.  It is also recommended to build the driver on the Host
machine.  This is not a requirement, but does make it easier for WinDbg 
to find the source files for source level debugging as the directory path 
to the source code is stored in the device driver SYS file.

Once the kernel-mode device driver is built, copy it to
%SystemRoot%\System32\Drivers directory of the Target machine.  Be
sure to also copy it to the appropriate 'Symbols\Sys' directory of
the Host machine for debugging purposes.

Copy ZWCREATE.INI to the Target machine and type the following:

regini zwcreate.ini

This will configure the registry for ZWCFTEST.SYS.

Next, a file called INFO.DAT must be copied to
%windir%\system32\drivers of the Target machine.  This is the file
that will be opened by ZWCFTEST.SYS.  It can be any type of file, but
an ASCII file is easy to spot when debugging:

copy readme.txt %windir%\system32\drivers\info.dat

Start WinDbg on the Host machine.

Reboot the Target machine.  Note, even thought ZWCFTEST.SYS is
manually started, we must reboot at least this one time to update the
registry's cache.

Logon with Administrator priveleges on the Target machine.

Before starting ZWCFTEST, a breakpoint needs to be set in the kernel
debugger for this routine.  This can be accomplished either manually,
or via the registry.  To set a breakpoint manually, break in to the
debugger by hitting the SysReq key on the Target machine, or by
hitting Ctrl-C in the Command window of the kernel debugger on the
Host machine.  Type 'bp zwcftest!driverentry' (case insensitive)
followed by the enter key.  Since the device driver is not yet
loaded, the breakpoint cannot be instantiated.  Now type a 'g'
followed by the enter key to give control back to the Target machine.

An alternative way to set the breakpoint is by editing the registry.
This is only valid if the device driver was built as a Checked
version (which defines DBG).  To do this, start REGEDT32 and select
the window marked HKEY_LOCAL_MACHINE.  From there, click down to
SYSTEM\CurrentControlSet\Services\ZwCFTest\Parameters and edit the
BreakOnEntry subkey to be a non-zero value.  Now, when ZWCFTEST is
started, it automatically breaks in to the debugger.  To see how this
is accomplished, examine the source code in DriverEntry.  Note, when
BreakOnEntry is non-zero, a debugger MUST be connected to the Target
machine when ZWCFTEST is started.  If a debugger is not connected,
the Target machine will appear to lock-up and cannot be used until a
debugger is connected, or  until its power is recycled.  Also, when
BreakOnEntry is used to break into the debugger, the actual code that
is broken into is DbgBreakPoint.  To enter ZWCFTEST's source code,
hit F10 twice to execute the return from DbgBreakPoint into
ZWCFTEST.SYS.

Start ZWCFTEST.SYS on the Target machine by typing:

net start zwcftest

At this point you should see the cursor stop on the Target machine
which indicates that the debugger has control of the Target machine.
Also, the source window for ZWCREATE.C should appear as long as
ZWCREATE.C is in the same directory path as when the file was built
AND a Checked version of the file was built.  If BreakOnEntry was
used to break in to the debugger, F10 will have to be hit twice
before the source window appears.

The file to be opened (L"\\SystemRoot\\System32\\Drivers\\Info.dat")
is split up into a path prefix (L"\\SystemRoot\\System32\\Drivers")
and a DEFAULT_FILE_NAME (L"info.dat").  These are combined into one
filename in ZwCFOpenAndReadFile.  This is also where the file handle
is obtained and the actual reading of the file occurs.  If the file
to be read is found elsewhere than under %SystemRoot%, then
PathPrefix must include \\DosDevices followed by the directory path
(e.g. L"\\DosDevices\\D:\\MyDir\\MySubDir").  This is only valid as
long as the Start value for ZWCFTEST is a 2 or a 3.  If the Start
value is a 0 or a 1, then an ArcName must be used to define the path
unless it is under %SystemRoot%.
