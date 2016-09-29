
VHOOK86D.386

This is a simple VxD that will hook the V86 INT chain by using the
Hook_V86_Int_Chain service.  For more information on the
Hook_V86_Int_Chain service, please consult the Virtual Device Adaptation
Guide of the DDK.

WARNING: This VxD, by default, will hook INT 2fh.
         This VxD may impair system performance.  It is intended
         only to be a demonstration.
