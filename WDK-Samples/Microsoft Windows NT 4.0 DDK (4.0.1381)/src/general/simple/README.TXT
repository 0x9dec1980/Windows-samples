This sample is composed of two parts, the Win32 portion and the
kernel device driver portion.

The Win32 portion contains a file, GETHNDL.C, which attempts to
obtain a handle to the LDUNLD.SYS driver which is named LOADTEST.  The
executable is built using the Windows NT SDK.  First update the environment
and path by running <mstools>\setenv.bat.  Type "NMAKE -f GETHNDL.MAK" to
compile the Win32 program, GETHNDL.EXE.

The kernel driver portion contains the driver source code, LDUNLD.C
(which is short for LOAD/UNLOAD) and a text file which is used in
conjuction with your registry.  The driver is built using the Windows
NT DDK.  To build the driver type:

build -cef

Copy the built driver, LDUNLD.SYS to the <winnt_root>\system32\drivers
directory :

copy obj\i386\ldunld.sys <winnt_root>\system32\drivers

Now, update the registry :

regini ldunld.ini        ; add the driver to the registry

Reboot.

Execute GETHNDL.EXE -- should not get a handle
net start ldunld
Execute GETHNDL.EXE -- should get a handle
net stop ldunld     -- will only work with Administrator privileges
Execute GETHNDL.EXE -- should not get a handle

That's all there is to it.  You can net start the driver with User
privileges, but you won't be able to stop it.  There are also debug
print messages which appear in the kernel debugger, if you build a checked
version.
