
.\msg00001.bin : vgactrs.mc
    erase .\vgactrs.h
    erase .\msg00001.bin 
    erase .\vgactrs.rc
    mc -v -s vgactrs.mc

.\vgactrs.rc : vgactrs.mc
    erase .\vgactrs.h
    erase .\msg00001.bin 
    erase .\vgactrs.rc
    mc -v -s vgactrs.mc

.\vgactrs.h: vgactrs.mc
    erase .\vgactrs.h
    erase .\msg00001.bin 
    erase .\vgactrs.rc
    mc -v -s vgactrs.mc
