# Nmake macros for building Windows 32-Bit apps

!include <ntwin32.mak>

all: rptest.exe

rptest.obj: rptest.c
    $(cc) $(cflags) $(cvars) $(cdebug) rptest.c

rptest.exe: rptest.obj
    $(link) $(conflags) -out:rptest.exe rptest.obj $(conlibs)
