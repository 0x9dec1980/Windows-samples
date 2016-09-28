////////////////////////////////////////////////////////////////////////
// Program Name: Control.h
// Programmer: Mark Overby (MarkOv)
// Description: This is the header file which will define the custom
//              IOCTL's that we will use.
////////////////////////////////////////////////////////////////////////

// =====================================================================
// INCLUDES AND GLOBALS
// =====================================================================

#include <devioctl.h>

#define IOCTL_DO_NOTHING \
    CTL_CODE (FILE_DEVICE_UNKNOWN, 0xAA0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_HOLD_REQUEST \
    CTL_CODE (FILE_DEVICE_UNKNOWN, 0xAA1, METHOD_BUFFERED, FILE_ANY_ACCESS)

