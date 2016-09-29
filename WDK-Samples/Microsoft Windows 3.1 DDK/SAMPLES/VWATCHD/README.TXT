
VWATCHD.386

VwatchD is a Virtual Device that demonstrates the basic structure of
VxD. TEST.EXE contains the DOS application that calls the VwatchD
V86 API entry.

To Install VwatchD.386, copy the file VwatchD.386 to Windows\system 
directory. Specify "device=VwatchD.386" in system.ini [386ehn]
section. 

A debug terminal needs to be setup to see the debugging output
by VwatchD. Also Win386.EXE debug version should be running. The
debug version of Win386.exe can be found in DDK source directory
VxD\tools\debug.
