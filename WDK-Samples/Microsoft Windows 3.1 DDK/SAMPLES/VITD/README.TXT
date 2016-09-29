
VITD.386

This VxD provides a simulation of a hardware interval timer for virtual
machines. To install it, build VITD.386, place the final VITD.386 in
the Windows SYSTEM directory, and add a "device=VITD.386" line in the 
[386enh] section of the SYSTEM.INI and restart Windows.

The accompanying sample applications and header files provide documentation
on using the virtual interval timer.
