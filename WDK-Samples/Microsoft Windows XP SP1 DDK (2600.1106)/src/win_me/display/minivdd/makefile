
!include ..\check.mk


!ifndef DO
DO=retail 
!endif

!if "$(BUILD)" == "ALL" || "$(BUILD)" == "all"
DO=retail 
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
!if EXIST(retail)
            echo Y | del retail\*.* > nul
            rmdir retail
!endif
        $(MAKE) DO="clean"

debug:
        $(MAKE) DO="debug"

retail:
        $(MAKE) DO="retail"
