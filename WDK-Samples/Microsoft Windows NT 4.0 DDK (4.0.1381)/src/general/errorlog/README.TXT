This sample kernel-mode driver, SimLog, demonstrates how to use the
IO Manager's error logging functions and how to compose the message
file and how to setup the registry appropriately.

How to use the sample:

. Build the sample driver

. Copy the .sys file to the target machine's Winnt\System32\Drivers
  directory.

. Copy the SimLog.ini file to the target machine and run Regini.exe

	Regini Simlog.ini

  This will setup the registry.

. Reboot the target machine so that the registry change can take effect.

. At cmd prompt, run "Net start simlog" to start the driver.

. Review the Event Viewer.

To unload the driver run "net stop simlog" at the cmd prompt.


A few notes about using the event logging functions.

1. The registry setup.

See simlog.ini for the registry entry that needs to be setup in order
for the event logging to work correctly. The string "EventMessageFile"
points to where the message files are. If the driver is to use the
standard error code provide by the IO Manager (see chapter 16 of the
Kernel-mode Driver Design Guide), EventMessageFile should point to:

%SystemRoot%\System32\IoLogMsg.dll

If, like in this sample, a driver defined error message is be used. The
path should be:

%SystemRoot%\System32\Drivers\SimLog.Sys

or whereever the driver file is. Multiple message files can be specified
and seperated by semi-colones. For example:

%SystemRoot%\System32\Drivers\SimLog.Sys;%SystemRoot%\System32\IoLogMsg.dll

2. The message file

If a driver needs to use a driver defined error message, the driver
writer makes a .mc file. When the message compiler compiles
the message file, it will generate the .h file, the .rc file and the
msgXXXXX.bin file. In this example, when simlog.mc is compiled,
simlog.h, simlog.rc and msg00001.bin get generated. See makefile.inc
and the sources file for the necessary changes to be made.

3. The error code

Whether an error code is driver defined or system defined, it should
not be used as a returned status by a driver routine. The driver should
return an appropriate status code defined by Windows NT. The error code
is put in the field ErrorCode in the error log entry.

4. The insertion strings

In the message text defined in the .mc file, %1 is reserved by
the IO Manager. If IoAllocateErrorLogEntry is called with a device,
the name of the device will be inserted into the message at %1.
Otherwise, the place of %1 will be left empty. In either case, the
insertion strings from the driver's error log entry starts at %2.
In other words, the first insertion string goes to %2, the second
to %3 and so on.

-End
