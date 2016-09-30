# Nmake macros for building Windows 32-Bit apps

!include <ntwin32.mak>

all: macaddr.exe

macaddr.obj: macaddr.c
    $(cc) $(cflags) $(cvars) $(cdebug) -I$(BASEDIR)\src\network\inc $*.c

macaddr.exe: macaddr.obj
    $(link) $(linkdebug) $(conflags) -out:$*.exe $*.obj $(conlibs)
