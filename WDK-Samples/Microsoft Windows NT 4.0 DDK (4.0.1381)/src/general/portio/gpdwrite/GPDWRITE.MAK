# Nmake macros for building Windows 32-Bit apps

!include <ntwin32.mak>

all: gpdwrite.exe

gpdwrite.obj: gpdwrite.c
    $(cc) $(cflags) $(cvars) $(cdebug) -I.. $*.c

gpdwrite.exe: gpdwrite.obj
    $(link) $(linkdebug) $(conflags) -out:$*.exe $*.obj $(conlibs)
