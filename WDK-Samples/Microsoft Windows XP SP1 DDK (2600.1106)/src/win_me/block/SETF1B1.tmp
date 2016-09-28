
# filter empty NMAKE command option
MKFLAGS = $(MAKEFLAGS: =)
!if "$(MKFLAGS)" != ""
MKFLAGS =-$(MAKEFLAGS)
!endif


DIRS = fsd  \
       port \
       vsd  \
       wnaspi32

$(DIRS): $(@R)\nul
        cd $@
        $(MAKE) $(MKFLAGS) $(DO)
        cd ..

clean:
        $(MAKE) DO="clean"
