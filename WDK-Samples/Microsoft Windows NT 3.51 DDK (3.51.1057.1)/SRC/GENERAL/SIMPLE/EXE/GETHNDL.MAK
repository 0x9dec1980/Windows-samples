# Nmake macros for building Windows 32-Bit apps

!include <ntwin32.mak>

all: gethndl.exe

gethndl.obj: gethndl.c
    $(cc) $(cflags) $(cvars) $(cdebug) gethndl.c

gethndl.exe: gethndl.obj
    $(link) $(linkdebug) $(conflags) -out:gethndl.exe gethndl.obj $(conlibs)
