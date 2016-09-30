!include <ntwin32.mak>

OBJS=instdrv.obj

all: instdrv.exe

.c.obj:
    $(cc) $(cdebug) $(cflags) $(cvarsmt) $*.c

instdrv.exe: instdrv.obj
    $(link) $(linkdebug) $(conflags) -out:instdrv.exe instdrv.obj $(conlibsmt) advapi32.lib
