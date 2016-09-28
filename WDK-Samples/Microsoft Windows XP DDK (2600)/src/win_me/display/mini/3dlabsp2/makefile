
!include ..\..\check.mk



all:
    cd drv
    $(C16_ROOT)\bin\nmake retail
    cd ..
    cd dll
    nmake retail32
    cd ..

clean:
    cd drv
    $(C16_ROOT)\bin\nmake clean
    cd ..
    cd dll
    nmake clean
    cd ..

debug:
    cd drv
    $(C16_ROOT)\bin\nmake 
    cd ..

debug32:
    cd dll
    nmake
    cd ..

retail:
    cd drv
    $(C16_ROOT)\bin\nmake retail
    cd ..

retail32:
    cd dll
    nmake retail32
    cd ..




