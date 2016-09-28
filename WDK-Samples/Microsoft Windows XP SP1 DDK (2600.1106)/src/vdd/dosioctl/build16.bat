@echo off
set OldPath=%Path%
set Path=%BASEDIR%\src\vdd\dosioctl\bin16;%Path%

cd dosapp
nmake

cd ..\dosdrvr
nmake

cd ..

Path=%OldPath%
set OldPath=
