# Nmake macros for building Windows 32-Bit apps

!include <ntwin32.mak>

INCLUDE=$(INCLUDE);$(BASEDIR)\inc;$(BASEDIR)\src\storage\inc

all: spti.exe

spti.obj: spti.c
    $(cc) $(cflags) $(cdebug) $(cvars) spti.c

spti.exe: spti.obj
    $(link) $(linkdebug) $(conflags) -out:spti.exe spti.obj $(conlibs)
