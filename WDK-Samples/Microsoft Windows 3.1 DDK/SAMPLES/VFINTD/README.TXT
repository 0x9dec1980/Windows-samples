
VFINTD.386

Summary:
--------

Normally, hardware interrupts in Windows enhanced mode are 
classified either as "global" or as "owned". Global interrupts 
will typically be reflected into whichever virtual machine is 
currently running, while owned interrupts will be reflected into 
the machine that owns that interrupt. For example, the floppy
device is one that should be made global, because any virtual
machine should be able to use it and receive interrupts from it.
On the other hand, an application that exclusively handles the
communication with a non-standard device would become confused 
if interrupts from the device were routed to some other virtual 
machine. For this reason, these interrupts should be "owned".

Some software developers may want to be able to "capture" global 
interrupts temporarily in one virtual machine, effectively 
making the interrupt "owned". VFINTD.386 is a sample VxD which 
does just that. With this VxD installed in an enhanced mode Windows 
system, an application can issue the appropriate API calls to force 
all interrupts coming from the floppy device to be reflected into 
the app's VM, regardless of which VM is currently running.


Background:
-----------

The PIC (Programmable Interrupt Controller) contains an IMR
(Interrupt Mask Register) which defines which hardware interrupts 
are currently enabled and which are masked. You can display the 
contents of the IMR with DEBUG. For example, from the DOS prompt, 
issue the commands:
    
    debug
    i21

This will display the value of port 21 on your machine, which is
the IMR for the hardware interrupts IRQ0 - IRQ7. The format of the
IMR is such that for each bit (0-7) that is on, the corresponding 
IRQ (Interrupt ReQuest) is disabled. So, a typical IMR value might 
be x'98', which indicates that IRQ3, IRQ4, and IRQ7 are all masked, 
meaning those interrupts are disabled.

        x'98' = b'10011000', masked IRQ's = '7..43...'

  NOTE: On AT systems, there are two hardware PIC's, and thus two 
   IMR's which correspond to the fifteen possible IRQ's (IRQ 9 is 
   cascaded to IRQ2, tying up the usage of one IRQ). This point
   is only mentioned here for completeness, but is not necessary to
   understand the overall picture.

When a DOS device driver or TSR installs an interrupt handler for
a particular IRQ, it sets up the interrupt vector and unmasks the
corresponding bit in the IMR. When Windows is running, DOS drivers 
and TSRs are normally shared by all virtual machines. Thus, if an 
interrupt occurs while an arbitrary VM is running, Windows would not 
need to perform a virtual machine task switch in order to access the 
interrupt handler, because it resides in the address space of each 
virtual machine.

On the other hand, if an interrupt handler for a previously masked
IRQ is installed WITHIN a virtual machine, and an interrupt occurs 
while another virtual machine is executing, Windows would have to 
make a task switch in order for that local interrupt handler to be 
accessible and executable.


Windows Default Behavior:
-------------------------

In Windows enhanced mode, the virtualization of the PICs is done
by the VPICD. If no other action is taken with regard to an IRQ,
then VPICD will handle interrupts according to the following 
default behavior:

When Windows enhanced mode boots, VPICD examines the value of
the IMR's in the system to determine which interrupts are masked. 
If an interrupt was not masked (was enabled), Windows must assume 
that there is already a valid interrupt handler installed. These 
IRQ's are then flagged as "global", so that Windows can avoid the 
overhead of task switching to have them serviced. The remaining
IRQ's, which were masked (disabled), are flagged as "local" or 
"owned". Interrupts for these IRQ's will only be reflected into
the particular virtual machine that unmasks them.


Changing the Default Behavior:
------------------------------

It is possible to change the default behavior of interrupt handling
described above by installing a VxD which virtualizes the IRQ. This
is done with the VxDCall "VPICD_Virtualize_IRQ". What this call 
does is to replace the VPICD default callback routines with ones
supplied in the VxD. For example, the VCD (Virtual COMM Device)
does this to control interrupts from comm devices regardless of the 
state of the IMR at Windows load time.

The VFINTD sample VxD also uses this function to allow an app to 
"capture" the interrupts on IRQ6 (floppy device), which is normally 
global. For example, a backup program that needs to directly access 
the floppy controller hardware to improve performance would "lose" 
interrupts to other virtual machines unless action was taken to 
change the global nature of IRQ6. The VFINTD can be used in this
case to capture the "focus" of the floppy device and the interrupts 
it generates.


Using VFINTD:
-------------

There are two pieces to this sample, the VxD and a sample DOS app. 
All the files that have the VFINTD file name are a part of the VxD. 
The DOS app has the name TSTFINT. The only purpose of the DOS app is 
to capture the floppy interrupts. A release of this focus is not 
done in the sample app, but this functionality is provided in the
VxD.

To install the VxD in your machine, put the line:
    device=VFINTD.386

in the [386enh] section of your system.ini. Then, put that file in
your Windows SYSTEM directory. Once the machine is booted, you can
issue a "TSTFINT" from a DOS VM to capture the floppy interrupts. 
TSTFINT will print the VxD's version number to indicate that it has 
successfully communicated with the VxD.

This sample has been provided to demonstrate the technique of 
handling the virtualization of a particular IRQ. It would make a
good base for other VxDs that need to perform similar functions.

