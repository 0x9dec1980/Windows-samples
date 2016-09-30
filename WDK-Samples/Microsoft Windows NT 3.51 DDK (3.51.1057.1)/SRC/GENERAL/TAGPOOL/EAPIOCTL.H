// eapioctl.h    Include file for ExAllocatePoolwithTag Example Driver
//
// Define the IOCTL codes we will use.  The IOCTL code contains a command
// identifier, plus other information about the device, the type of access
// with which the file must have been opened, and the type of buffering.
//
// Robert B. Nelson (Microsoft)     March 1, 1993

// Device type           -- in the "User Defined" range."
#define EAPWT_TYPE 40000

// The IOCTL function codes from 0x800 to 0xFFF are for customer use.

#define IOCTL_EAPWT_LEAK_ALLOCATE_POOL \
    CTL_CODE( EAPWT_TYPE, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_EAPWT_LEAK_DEALLOCATE_POOL \
    CTL_CODE( EAPWT_TYPE, 0x901, METHOD_BUFFERED, FILE_READ_ACCESS )

