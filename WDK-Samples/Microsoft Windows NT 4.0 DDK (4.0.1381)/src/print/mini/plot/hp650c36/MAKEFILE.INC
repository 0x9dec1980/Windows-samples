hp650c36.pcd: hp650c36.txt
    plotgpc hp650c36.txt hp650c36.pcd
    copy hp650c36.pcd $(BASEDIR)\lib\$(TARGET_DIRECTORY)\$(DDKBUILDENV)
