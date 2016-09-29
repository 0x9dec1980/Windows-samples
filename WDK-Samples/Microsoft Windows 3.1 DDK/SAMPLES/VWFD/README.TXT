
VWFD.386

This VxD, when installed on a system running Windows in enhanced mode,
allows DOS applications to determine if they are running in a window
or full-screen.  This is accomplished via the API that VWFD supplies
TSTWF.ASM is a sample DOS program that uses the VWFD API. 


To install this VxD, copy it to your Windows SYSTEM directory and 
add the following line to your SYSTEM.INI.

[386enh]

    ...
    ...

    device=VWFD.386
