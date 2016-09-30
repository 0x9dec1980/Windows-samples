!include <ntwin32.mak>

all: dosdev.exe

.c.exe:
  $(cc) $(cflags) $(cdebug) $(cvars) -I$(BASEDIR)\src\input\inc $*.c
  $(link) $(ldebug) $(conlflags) -out:$*.exe $*.obj $(conlibs) user32.lib advapi32.lib
