rem Sample setenv.bat

rem Please note that the paths in this batch file are included as examples only.
rem It will be necessary for you to modify the path names so that they correctly
rem reference the platforms and tools on your build system.
rem To avoid difficulties with long directory names, substitute the DOS directory
rem names in the paths, e.g. PROGRA~1 instead of Program Files.

rem Set C32_ROOT to reference the root of the VC installation and ensure that both
rem %C32_ROOT%\Bin and the folder containing the shared Visual Studio binaries are
rem listed in the PATH.

rem Example:

rem Example For Visual C++ 5.0:

rem set MSVC5DIR=C:\PROGRA~1\DevStudio
rem set C32_ROOT=%MSVC5DIR%\VC
rem set PATH=%PATH%;%C32_ROOT%\Bin;%MSVC5DIR%\SharedIDE\bin


rem Example For Visual C++ 6.0:

set MSVC60DIR=C:\PROGRA~1\MICROS~1
set C32_ROOT=%MSVC60DIR%\VC98
set PATH=%PATH%;%C32_ROOT%\Bin;%MSVC60DIR%\Common\MSDev98\bin

rem Example For Microsoft Visual Studio:

rem set VSTUDIODIR=C:\PROGRA~1\MICROS~4
rem set C32_ROOT=%VSTUDIODIR%\VC98
rem set PATH=%PATH%;%C32_ROOT%\Bin;%VSTUDIODIR%\MSDev98\bin


rem Set DDKROOT to reference the root of the Windows 98 DDK installation, e.g.
set DDKROOT=C:\NTDDK

rem Set C16_ROOT to reference the root of the 16 bit compiler installation, e.g.
set C16_ROOT=%DDKROOT%\bin\win_me
set PATH=%PATH%;%C16_ROOT%\bin16

rem Set MASM_ROOT to reference the root of the MASM 6.11 installation, e.g.
rem if you have masm installed then:   set MASM_ROOT=C:\MASM611

rem if you do not have masm installed
set MASM_ROOT=%DDKROOT%\bin\win_me

rem Set DXDDKROOT to reference the root of the DirectX 7 DDK, e.g.
set DXDDKROOT=C:\NTDDK

rem Set DXSDKROOT to reference the root of the DirectX 7 SDK installation, e.g.
set DXSDKROOT=C:\NTDDK

rem Set SDKROOT to reference the root of the Platform SDK installation, e.g.
set SDKROOT=C:\NTDDK
