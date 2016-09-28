The INSTRDRV sample demonstrates how to use the Win32 Service Control
Manager APIs to install, start, stop, and delete a kernel-mode device
driver.  It consists of a Win32 console application ("INSTSRV") and a
simple NT kernel-mode device driver ("SIMPLDRV").

Instructions for use:

    1. Type "nmake /f instdrv.mak" to compile and link the
       INSTDRV executable program.

    2. Run the NTDDK\SETENV.BAT script to set up a DDK build
       environment.  If you are running a target/host machine
       debug setup then you'll want to create a "checked"
       environment so you can watch the debug info generated
       by the driver, otherwise specify a "free" build
       environment.

    3. Compile and link the driver by typing "build -cef". The
       built driver (SIMPLDRV.SYS) will reside in
       <ddk_root>\lib\*\*.

    4. Once INSTDRV.EXE and SIMPLDRV.SYS have been built,
       run the INSTDRV.EXE program.
