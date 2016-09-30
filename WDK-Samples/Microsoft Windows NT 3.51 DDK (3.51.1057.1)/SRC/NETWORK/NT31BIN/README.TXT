Running NDIS miniport drivers on Windows NT 3.1
-----------------------------------------------

NDIS miniport driver binaries built on and for Windows NT 3.5 can
also be run on Windows NT 3.1, provided the following steps are
taken:

1) The driver cannot directly make any system calls.  It can only
   use the functions, macros, and types explicitly exported by
   ndis.h.  There are certain system functions declared in ndis.h,
   but these are prefaced by a warning not to call them directly.
   Failure to heed this warning will prevent the driver from running
   on Windows NT 3.1.

2) The sources file used to build the driver must include the
   following line:

        DRIVER_ALIGNMENT=0x200

   Windows NT 3.5 allows image sections to be aligned on 32-byte
   boundaries, but Windows NT 3.1 requires them to be aligned on
   512-byte boundaries.  If you do not add this line to your sources
   file, Windows NT 3.1 will refuse to load your driver, reporting an
   invalid image format error.

3) When the driver is installed, the new NDIS.SYS for 3.1 supplied
   in the DDK must also be installed.  The NDIS.SYS that shipped with
   3.1 does not support miniports.  The new NDIS.SYS supports
   miniports and also fully supports all 3.1-compatible full MAC
   drivers.  (Note that full MAC drivers built on and for 3.5 will
   not run on 3.1, even with the new NDIS.SYS installed.)

Running NDIS miniport drivers on Chicago
----------------------------------------

x86 NDIS miniport driver binaries built on and for Windows NT 3.5 can
also be run on Chicago, provided the driver does not directly make
any system calls.  It can only use the functions, macros, and types
explicitly exported by ndis.h.  There are certain system functions
declared in ndis.h, but these are prefaced by a warning not to call
them directly.  Failure to heed this warning will prevent the driver
from running on Chicago.
