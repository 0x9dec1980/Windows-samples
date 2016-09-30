!include <ntwin32.mak>

all: dditest.exe

.c.obj:
  $(cc) $(cdebug) $(cflags) $(cvars) -I$(BASEDIR)\inc $*.c


dditest.exe: dditest.obj wndtest.obj
  $(link) $(linkdebug) $(conflags) -out:dditest.exe $**  $(guilibs)
