
!include ..\check.mk


!ifndef DO
DO=retail retail32
!endif

!if "$(BUILD)" == "ALL" || "$(BUILD)" == "all"
DO=retail retail32
!endif

DIRS = 3dlabsp2


$(DIRS): $(@R)\nul
        cd $@
        $(MAKE) $(DO)
        cd ..

clean:
!if EXIST(debug)
        echo Y | del debug\*.* > nul
            rmdir debug
!endif
!if EXIST(debug32)
        echo Y | del debug32\*.* > nul
            rmdir debug32
!endif
!if EXIST(retail)
        echo Y | del retail\*.*
            rmdir retail
!endif
!if EXIST(retail32)
        echo Y | del retail32\*.* > nul
            rmdir retail32
!endif
        $(MAKE) DO="clean"

debug:
        $(MAKE) DO="debug debug32"

retail:
        $(MAKE) DO="retail retail32"
