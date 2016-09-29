
VKXD.386

This is a tiny little VxD that lets you use F9 as the alt-enter 
combination. To install it, put the file VKXD.386 in your Windows
SYSTEM directory, and add the following line in the [386enh] section 
of your SYSTEM.INI:

    device=VKXD.386

Restart Windows and Voila! The F9 key will switch your DOS applications
from full-screen to windowed mode.

Note: Under Windows version 3.1, the ALT-ENTER keypress sequence will
cause actions to be taken that did not happen in 3.0 (eg: in the Program
Manager, pressing ALT-ENTER when a program icon is selected will bring up
the "program item properties" dialog box).  This VxD is given
as an example, and is not intended to be a "perfect" implementation of
a window to full-screen toggle key.
