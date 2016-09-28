This sample consists of a kernel-mode device driver and a Win32
application that communicates with the device driver.  The purpose of
this sample is to demonstrate how using ExallocatePoolWithTag can
assist developers in tracking down memory leaks in their code.

The device driver sample, EAPWT.SYS, must be built using the
windows NT DDK.  The file must be built from a Checked Build
Environment.  This is because the 'tag' that is associated with the
pool allocation is only used in Checked versions of the operating
system.

The Win32 aplication, found in the UserApp subdirectory, is made
using 'nmake'.  After making MEMLEAK.EXE, copy it to the Target
machine.

Once the kernel-mode device driver is built, copy it to
%SystemRoot%\System32\Drivers directory of the Target machine.  Be
sure to also copy it to the appropriate 'Symbols\Sys' directory of
the Host machine for debugging purposes.

Copy EAPWT.INI to the Target machine and type the following:

regini eapwt.ini

This will configure the registry for EAPWT.SYS.

Start WinDbg on the Host machine.

Reboot the Target machine.  Note, even thought EAPWT.SYS is manually
started, we must reboot at least this one time to update the
registry's cache.

Logon with Administrator priveleges on the Target machine.

Start EAPWT.SYS on the Target machine by typing:

net start eapwt

run MEMLEAK.EXE on the Target machine.

Type 'a' ten times.

Break in to the debugger.  Type,

!poolused 1

You should see some output very similar to the following:

KDx86> !poolused 1
   Sorting by Tag

  Pool Used:
            NonPaged                    Paged
 Tag    Allocs    Frees     Diff     Used   Allocs    Frees     Diff     Used
 Adap        3        0        3      608        0        0        0        0
 Afd        12        9        3      384        0        0        0        0
 BusH       27       15       12     1088        0        0        0        0
 CM          1        1        0        0     1091      814      277  1232544
 CMkb        0        0        0        0     1915     1870       45     9760
 CMnb        0        0        0        0        8        0        8      768
 CMpa        8        0        8      768        0        0        0        0
 .
 .
 .
 Vpb         7        0        7      896        0        0        0        0
 Wait      456      454        2      512        0        0        0        0
 XTag       10        0       10    81920        0        0        0        0
 call        1        0        1      832        0        0        0        0


Notice that XTag (our tag name) has 10 allocations accounted for it. 
Type 'g' to continue.

On the Target machine, type 'a' one more time and break in to the
debugger again.  Type,

!poolused 1

KDx86> !poolused 1
   Sorting by Tag

  Pool Used:
            NonPaged                    Paged
 Tag    Allocs    Frees     Diff     Used   Allocs    Frees     Diff     Used
 Adap        3        0        3      608        0        0        0        0
 Afd        12        9        3      384        0        0        0        0
 BusH       27       15       12     1088        0        0        0        0
 CM          1        1        0        0     1091      814      277  1232544
 CMkb        0        0        0        0     1915     1870       45     9760
 CMnb        0        0        0        0        8        0        8      768
 CMpa        8        0        8      768        0        0        0        0
 .
 .
 .
 Vpb         7        0        7      896        0        0        0        0
 Wait      491      489        2      512        0        0        0        0
 XTag       11        0       11    90112        0        0        0        0
 call        1        0        1      832        0        0        0        0


Notice that XTag now has an 11th allocation as expected.  Type 'g' to
continue.

On the Target machine, type 'd' ten times.  Break in to the debugger
again.  Type,

!poolused 1

KDx86> !poolused 1
   Sorting by Tag

  Pool Used:
            NonPaged                    Paged
 Tag    Allocs    Frees     Diff     Used   Allocs    Frees     Diff     Used
 Adap        3        0        3      608        0        0        0        0
 Afd        12        9        3      384        0        0        0        0
 BusH       27       15       12     1088        0        0        0        0
 CM          1        1        0        0     1091      814      277  1232544
 CMkb        0        0        0        0     1915     1870       45     9760
 CMnb        0        0        0        0        8        0        8      768
 CMpa        8        0        8      768        0        0        0        0
 .
 .
 .
 Vpb         7        0        7      896        0        0        0        0
 Wait      586      583        3      672        0        0        0        0
 XTag       11       10        1     8192        0        0        0        0
 call        1        0        1      832        0        0        0        0

Now notice that XTag has 11 allocations, but it also has 10
Deallocations with a difference of 1.  Type 'g' to continue.

On the Target machine, type one more 'd'.  Break in to the debugger
again.  Type,

!poolused 1

KDx86> !poolused 1
   Sorting by Tag

  Pool Used:
            NonPaged                    Paged
 Tag    Allocs    Frees     Diff     Used   Allocs    Frees     Diff     Used
 Adap        3        0        3      608        0        0        0        0
 Afd        12        9        3      384        0        0        0        0
 BusH       27       15       12     1088        0        0        0        0
 CM          1        1        0        0     1091      814      277  1232544
 CMkb        0        0        0        0     1915     1870       45     9760
 CMnb        0        0        0        0        8        0        8      768
 CMpa        8        0        8      768        0        0        0        0
 .
 .
 .
 Vpb         7        0        7      896        0        0        0        0
 Wait      631      628        3      672        0        0        0        0
 XTag       11       11        0        0        0        0        0        0
 call        1        0        1      832        0        0        0        0

All of XTag's allocations have been properly deallocated.

By using ExAllocatePoolWithTag in combination with a Checked build of
the OS, it is possible to verify how many allocations and
deallocations have been made by a device driver.
