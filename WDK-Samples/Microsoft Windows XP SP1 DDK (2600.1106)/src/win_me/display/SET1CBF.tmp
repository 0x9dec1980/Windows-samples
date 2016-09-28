
!include check.mk

!ifndef DO
all: retail 
!endif

!if "$(BUILD)" == "ALL" || "$(BUILD)" == "all"
all: retail
!endif



debug:
        cd mini
        $(MAKE) DO="debug debug32"
		cd ..\minivdd
		$(MAKE) DO="debug"
		cd ..

retail:
        cd mini
        $(MAKE) DO="retail retail32"
		cd ..\minivdd
		$(MAKE) DO="retail"
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
        cd mini
        $(MAKE) clean
		cd ..\minivdd
		$(MAKE) clean
		cd ..


