hpgl2pen.pcd: hpgl2pen.txt
    plotgpc hpgl2pen.txt hpgl2pen.pcd
    copy hpgl2pen.pcd $(BASEDIR)\lib\$(TARGET_DIRECTORY)\$(DDKBUILDENV)
