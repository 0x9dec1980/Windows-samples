
VDIALOG.386


This VxD demonstrates how to serialize I/O to a device under enhanced
mode Windows. To find out what this means exactly, install this VxD
by putting a "device=" line in the [386enh] section of your SYSTEM.INI.
Restart Windows, and try running the accompanying sample programs, WINACC
and DOSACC. These two application programs simply try to perform an
IN instruction to a predetermined, but undefined port. The VDIALOG
VxD will protect multiple VM's from accessing the port (either reading
or writing), and display a dialog box to resolve the contention.

*) Open a DOS box.
*) Execute DOSACC.EXE in that DOS box.
*) Switch back to Windows.  DO NOT DESTROY THE DOS BOX (EG: Don't "exit".)
*) Execute WINACC.EXE.
*) Admire the dialog box that has appeared.  If no contention box has
   appeared, you've probably done something wrong.  Make sure you've
   added the "device=" line for the VDIALOG device in your system.

