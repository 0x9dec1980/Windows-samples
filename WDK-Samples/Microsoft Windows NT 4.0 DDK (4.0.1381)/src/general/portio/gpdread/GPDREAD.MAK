# Nmake macros for building Windows 32-Bit apps

!include <ntwin32.mak>

all: gpdread.exe

gpdread.obj: gpdread.c
    $(cc) $(cflags) $(cvars) $(cdebug) -I.. $*.c

gpdread.exe: gpdread.obj
    $(link) $(linkdebug) $(conflags) -out:$*.exe $*.obj $(conlibs)
