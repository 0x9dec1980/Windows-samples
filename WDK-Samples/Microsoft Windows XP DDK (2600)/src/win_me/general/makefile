
# filter empty NMAKE command option
MKFLAGS = $(MAKEFLAGS: =)
!if "$(MKFLAGS)" != ""
MKFLAGS =-$(MAKEFLAGS)
!endif


DIRS = EXE SYS

$(DIRS): $(@R)\nul
        cd $@
        $(MAKE) $(MKFLAGS) $(DO)
        cd ..

clean:
!ifndef LINKONLY
        $(MAKE) DO="clean"
!endif
