#   Window's Sample Display Drivers.
#
#   High resolution resources

goal: config.bin   fonts.bin colortab.bin

config.bin:    config.asm
    mlx /Zm /c config.asm
    link config ,config;
    exe2bin config
    erase config.exe
    erase config.obj

colortab.bin: colortab.asm
    mlx /Zm /c colortab.asm
    link colortab ,colortab;
    exe2bin colortab
    erase colortab.exe
    erase colortab.obj

fonts.bin: fonts.asm
    mlx /Zm /c fonts.asm
    link fonts;
    exe2bin fonts
    erase fonts.exe
    erase fonts.obj
