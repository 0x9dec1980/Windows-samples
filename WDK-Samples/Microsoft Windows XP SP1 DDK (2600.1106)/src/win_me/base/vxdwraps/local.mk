#****************************************************************************
#                                                                           *
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
# PURPOSE.                                                                  *
#                                                                           *
# Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
#                                                                           *
#****************************************************************************

AFLAGS  = -coff -W2 -c -Cx -I.. -DBLD_COFF -DIS_32 -DMASM6 -Sg

OBJS1   = $(FILES:z.asm=1.obj)
OBJS2   = $(FILES:z.asm=2.obj)
OBJS3   = $(FILES:z.asm=3.obj)
OBJS4   = $(FILES:z.asm=4.obj)
OBJS5   = $(FILES:z.asm=5.obj)
OBJS6   = $(FILES:z.asm=6.obj)
OBJS    = $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6)
LIBING  = $(OBJS: =&^
)
LIBING  = $(LIBING:&=)

target: $(DDKROOT)\lib\win_me\vxdwraps.clb

$(DDKROOT)\lib\win_me\vxdwraps.clb : always $(OBJS)
        if exist $(DDKROOT)\lib\win_me\vxdwraps.clb lib @<<vxdwraps.lnk
/out:$(DDKROOT)\lib\win_me\vxdwraps.clb
$(DDKROOT)\lib\win_me\vxdwraps.clb
$(LIBING)
<<
        if not exist $(DDKROOT)\lib\win_me\vxdwraps.clb lib @<<vxdwraps.lnk
/out:$(DDKROOT)\lib\win_me\vxdwraps.clb
$(LIBING)
<<

$(OBJS1): $(@:1.obj=z.asm)
        ml $(AFLAGS) -DSEGNUM=1 -Fo$*.obj $(@:1.obj=z.asm)

$(OBJS2): $(@:2.obj=z.asm)
        ml $(AFLAGS) -DSEGNUM=2 -Fo$*.obj $(@:2.obj=z.asm)

$(OBJS3): $(@:3.obj=z.asm)
        ml $(AFLAGS) -DSEGNUM=3 -Fo$*.obj $(@:3.obj=z.asm)

$(OBJS4): $(@:4.obj=z.asm)
        ml $(AFLAGS) -DSEGNUM=4 -Fo$*.obj $(@:4.obj=z.asm)

$(OBJS5): $(@:5.obj=z.asm)
        ml $(AFLAGS) -DSEGNUM=5 -Fo$*.obj $(@:5.obj=z.asm)

$(OBJS6): $(@:6.obj=z.asm)
        ml $(AFLAGS) -DSEGNUM=6 -Fo$*.obj $(@:6.obj=z.asm)

always:
	@rem echo pseudotarget
