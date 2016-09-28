This directory contains a sample DirectInput ring-3 force feedback
effect driver written in C, as well as a fully-commented INF file to
illustrate the registry keys used by force feedback.

Search for the string !!IHV!! for things that you definitely must
change before modifying this driver to suit your hardware.

Note that the code here is specifically optimized to be used in a DLL that
is a DirectInput force feedback effect driver and *nothing else*.  Do not
try to add additional interfaces or CLSIDs to the sample, because the
sample takes various shortcuts to make the code simpler.

The optimizations here are based on the assumption that management of a
force feedback device is global in scope, so no interfaces will require
per-instance data.  This is in general not true, which is why the code
is not suitable as a sample for a more general-purpose OLE in-proc server.

To test the sample driver (which doesn't do anything, but hey, if you
want to test it be my guest), follow these steps:

Step 1 - Build the binary

- If you are using the Platform SDK, make sure that the files

      c:\mssdk\include\dinput.h
      c:\mssdk\include\dinputd.h
      c:\mssdk\lib\dxguid.lib

  are the DirectX 5.0 versions of the files rather than the DirectX 3.0
  versions.
- Type "build -cz".
- Copy the generated FFDRV1.DLL to the same directory that contains
  a copy of the FFDRV1.INF file.

Step 2 - Installing the files

- Navigate to the directory that contains both ffdrv1.dll and ffdrv1.inf.
- Right-click ffdrv1.inf and choose Install.


Step 3 - Installing a joystick associated with the driver.

- Get a standard analog joystick and plug it in.  Make sure it works
  with the standard analog joystick driver.
- Open the Gaming Options control panel.
- Click the Add... button, then scroll down to the "XYZZY Force 
  Feedback Joystick" entry. 
- Select the joystick and click the Ok button to install it.

You can now make force feedback API calls, and the sample driver
will service the requests.  The sample driver does everything a
real driver would need to do, except that at the last minute, instead
of actually sending the command to a piece of hardware, it just prints
to the debugger terminal the command bytes it would have sent.
