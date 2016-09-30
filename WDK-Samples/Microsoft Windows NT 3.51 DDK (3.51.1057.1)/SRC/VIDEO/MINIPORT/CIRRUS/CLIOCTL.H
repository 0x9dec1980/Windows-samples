//---------------------------------------------------------------------------
//
//  file: CLIOCTL.H
// 
// (c) Copyright 1993, Cirrus Logic, Inc.
// All rights reserved.
//
//  date: 1 July 1993
//---------------------------------------------------------------------------
// The maximum GDI ESCAPE value defined in WINGDI.H is 4110(decimal). So here
// we pick an arbitrary value of...

#define CIRRUS_PRIVATE_ESCAPE 0x5000

//---------------------------------------------------------------------------
//
// The following macro(CTL_CODE) is defined in WINIOCTL.H. That file states
// that functions 2048-4095 are reserved for "customers". So I picked an 
// arbitrary value of 0x900=2304. 
//
#define IOCTL_CIRRUS_PRIVATE_BIOS_CALL \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CIRRUS_GET_CAPABILITIES  \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CIRRUS_SET_DISPLAY_PITCH \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)


//---------------------------------------------------------------------------
// Structure for miniport to indicate to display driver the capabilities
// of the chip.  The flag currently indicates HW Cursor and BLT Engine
// support.
//
// Also included is the size of memory, and the top of available offscreen
// memory. (Actually it's top+1).
//
typedef struct {            
   ULONG size;              // size of this structure
   ULONG fl;                // see bit description below
   ULONG ulChipId;          // Chip ID read from CR27[7:2] - e.g CL5434 = 0x2A
   ULONG ulMemSize;         // Size of memory in bytes=end of HW cursor buffers
   ULONG ulOffscreenMemTop; // Offset of 1st byte of unusable video memory
                            // [1st byte of cursor buffers on all but 754x]
                            // [1st byte of split screen buffer on 754x]
} CIRRUS_CAPABILITIES, *PCIRRUS_CAPABILITIES;

#define CL_ALLOW_HW_CURSOR 0x01     // Flag to enable HW Cursor in capabilities
#define CL_BLT_SUPPORT 0x02         // Flag set if chip has BLT Engine support
#define CL_ALLOW_OPAQUE_TEXT 0x04   // Flag to enable HW Cursor in capabilities
#define CL_LINEAR_MODE 0x08         // Flag set if addressing mode is linear

//---------------------------------------------------------------------------
//
// this is the structure used to pass arguments to the CIRRUS_PRIVATE_ESCAPE
// call done in DrvEscape(). The size of this struct limits the size of the
// returned arguments also. See the DrvEscape() function in enable.c (in the
// display driver DLL) for more information.
//
// NOTE: to enable the definition of these parameters, the following sequence
//       is recommended!
//
// #define ENABLE_BIOS_ARGUMENTS    // put this before the include
// #include "clioctl.h"


#ifdef ENABLE_BIOS_ARGUMENTS

typedef struct _VIDEO_X86_BIOS_ARGUMENTS {
    ULONG Eax;
    ULONG Ebx;
    ULONG Ecx;
    ULONG Edx;
    ULONG Esi;
    ULONG Edi;
    ULONG Ebp;
} VIDEO_X86_BIOS_ARGUMENTS, *PVIDEO_X86_BIOS_ARGUMENTS;

#endif

//---------------------------------------------------------------------------
