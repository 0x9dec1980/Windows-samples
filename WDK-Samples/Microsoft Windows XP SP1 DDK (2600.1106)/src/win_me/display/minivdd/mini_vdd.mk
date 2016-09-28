#############################################################################
#
#   VERDIR is defined, build a retail or debug version
#
#############################################################################

!ifdef VERDIR
!include	..\minivdd.mk
#############################################################################
#
#   VERDIR is not defined, set the goals and recurse
#
#############################################################################

!else   ## VERDIR

!if "$(BUILD)" == "ALL" || "$(BUILD)" == "all"
goal:   debug.a retail.a 
!else
goal:   debug.a 
!endif

retail debug: 
!ifdef SOURCES
        @echo ***** Building $* version of $(VXDNAME)
        @if not exist $*\nul mkdir $*
        @$(MAKE) /nologo VERDIR=$*
!else
        @echo ***** not building $* version of $(VXDNAME)
!endif

retail.a debug.a: 
!ifdef SOURCES
        @echo ***** Building $* version of $(VXDNAME)
        @if not exist $*\nul mkdir $*
        @$(MAKE) /nologo VERDIR=$*
!else
        @echo ***** not building $* version of $(VXDNAME)
!endif

depend:
        @echo depend not supported yet

clean:
!if EXIST(debug)
        echo Y | del debug\*.* > nul
        rmdir debug
!endif
!if EXIST(retail)
        echo Y | del retail\*.* > nul
	rmdir retail
!endif

!endif
