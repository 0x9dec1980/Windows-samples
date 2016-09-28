This sample demonstrates how to create a new device object in a
dispatch routine and how a Win32 program gets a handle to this new
device object.  This sample is a boilerplate that can easily be
converted to allow multiple device objects to be created with each
attempt at opening \\.\MySymbolicLink.  This sample creates an
initial master device object (\Device\Reparse) and a single symbolic
link (\DosDevices\RP) to this device object.  This sample only
creates one additional new device object, \Device\Reparse1, but could
be easily modified to create more device objects.  This avoids
creating a large number of symbolic links for each device object,
MySymbolicLink1, MySymbolicLink2, MySymbolicLink3, ....,
MySymbolicLinkN.  This is accomplished by 1) creating a new device
object in the IRP_MJ_CREATE dispatch routine, 2) modifying the
FileObject's FileName to reflect this new device object's name, and
3) return STATUS_REPARSE.

The device driver sample, REPARSE.SYS, is built using the Windows NT
DDK.  A Free or a Checked version of the driver can be built, but a
checked version of the driver is required for source level
debugging.  It is also recommended to build the driver on the Host
machine.  This is not a requirement, but does make it easier to find
the source files for source level debugging.

Once the kernel-mode device driver is built, copy it to
%SystemRoot%\System32\Drivers directory of the Target machine.  Be
sure to also copy it to the appropriate 'Symbols\Sys' directory of
the Host machine for debugging purposes.

Copy REPARSE.INI to the Target machine and type the following:

regini reparse.ini

This will configure the registry for REPARSE.SYS.

Start WinDbg on the Host machine.  Press F5 and wait for the Target
machine to connect.

Reboot the Target machine.  Note, even though REPARSE.SYS is
manually started, we must reboot at least this one time to update the
registry's cache.  Start the Target machine in Debug mode.

Logon with Administrator priveleges on the Target machine.

Before starting REPARSE, a breakpoint needs to be set in the kernel
debugger for this routine.  This can be accomplished either manually,
or via the registry.  To set a breakpoint manually, break in to the
debugger by hitting the SysReq key on the Target machine, or by
hitting Ctrl-C in the Command window of the kernel debugger on the
Host machine.  Type,

bp reparse!driverentry (case insensitive)

Since the device driver is not yet loaded, the breakpoint cannot be
instantiated.  Now type a 'g' followed by the enter key (or press F5)
to give control back to the Target machine.

An alternative way to set the breakpoint is to edit the registry. 
This is only valid if the device driver was built as a Checked
version (which defines DBG).  To do this, start REGEDT32 and select
the window marked HKEY_LOCAL_MACHINE.  From there, click down to
SYSTEM\CurrentControlSet\Services\Reparse\Parameters and edit the
BreakOnEntry subkey to be a non-zero value.  Now, when REPARSE is
started, it automatically breaks in to the debugger in the
DriverEntry routine.  To see how this is accomplished, examine the
source code in DriverEntry.  Note, when BreakOnEntry is non-zero, a
debugger MUST be connected to the Target machine when REPARSE is
started.  If a debugger is not connected, the Target machine will
appear to lock-up and cannot be used until a debugger is connected,
or  until its power is recycled.  Also, when BreakOnEntry is used to
break into the debugger, the actual code that is broken into is
DbgBreakPoint.  To enter REPARSE's source code, hit F10 twice to
execute the return from DbgBreakPoint into REPARSE.SYS.

Start REPARSE.SYS on the Target machine by typing the following at a
command prompt,

net start reparse

or by selecting the Control Panel applette, Devices.  Scroll down to
Reparse and click on Start.

At this point the Target machine appears to lockup which indicates
that the debugger has control of the Target machine. Also, the source
window for REPARSE.C should appear as long as REPARSE.C is in the
same directory path as when the file was built AND a Checked version
of the file was built. It is not possible to perform source level
debugging on a Free version of REPARSE.SYS.  If BreakOnEntry was used
to break in to the debugger, F10 will have to be hit twice before the
source window appears.

Once the kernel debugger has control of the Target machine, break
points can be set in various locations in REPARSE.C as deemed
necessary.  Once all breakpoints have been set, return control to the
Target machine by typing F5 in the Command Window of WinDbg.  The
Target machine no longer appears to be locked up.  Now run the Win32
application, RPTEST.EXE.  This test program attempts to obtain a
handle to \\.\RP which is a symbolic link that was created by
REPARSE.SYS.
