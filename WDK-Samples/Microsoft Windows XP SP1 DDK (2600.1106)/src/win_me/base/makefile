
# filter empty NMAKE command option
MKFLAGS = $(MAKEFLAGS: =)
!if "$(MKFLAGS)" != ""
MKFLAGS =-$(MAKEFLAGS)
!endif


DIRS =  asyncw32    \
        cvxd32      \
        eatpages    \
        generic     \
        vdialog     \
        vdmad       \
        vfintd      \
        vmirqd      \
        vpostd      \
        vwfd        \
        vxdwraps

$(DIRS): $(@R)\nul
        cd $@
        $(MAKE) $(MKFLAGS) $(DO)
        cd ..

clean:
        $(MAKE) DO="clean"
