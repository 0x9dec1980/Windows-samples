# Nmake macros for building Windows 32-Bit apps

!include <ntwin32.mak>

all: smpi.exe

smpi.obj: smpi.c
    $(cc) $(cflags) $(cvars) smpi.c

smpi.exe: smpi.obj
    $(link) $(conflags) -out:smpi.exe smpi.obj $(conlibs)
