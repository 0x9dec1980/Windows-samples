/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    monopriv.h

Abstract:

    This module contains the PRIVATE (driver-only) definitions for the
    code that implements the mono device driver.

Environment:

    Kernel mode

Revision History:

    03-22-93 : created

--*/


//
// Indices into & count of the above array
//

#define MONO_VIDEO_BUFFER  0
#define MONO_CRTC_REG      1
#define MONO_MODE_CTL_REG  2

#define MONO_NUMBER_RESOURCE_ENTRIES 3



//
// The following is the debug print macro- when we are building checked
// drivers "DBG" will be defined (by the \ddk\setenv.cmd script), and we
// will see debug messages appearing on the KD screen on the host debug
// machine. When we build free drivers "DBG" is not defined, and calls
// to pMonoKdPrint are removed.
//

#ifdef DBG
#define pMonoKdPrint(arg) DbgPrint arg
#else
#define pMonoKdPrint(arg)
#endif



//
// A structure representing the instance information associated with
// a particular device
//

typedef struct _MONO_DEVICE_EXTENSION
{
    KEVENT         SyncEvent;
    INTERFACE_TYPE InterfaceType;
    ULONG          BusNumber;
    PUSHORT        VideoMemory;
    PUCHAR         CRTCRegisters;
    PUCHAR         ModeControlRegister;
    ULONG          Xpos;
    ULONG          Ypos;

} MONO_DEVICE_EXTENSION, *PMONO_DEVICE_EXTENSION;



//
// A structure decribing a particular device resource
//

typedef struct _MONO_RESOURCE
{
    PHYSICAL_ADDRESS  PhysicalAddress;
    ULONG             Length;
    ULONG             AddressSpace;
    ULONG             RangeSharable;

}   MONO_RESOURCE, *PMONO_RESOURCE;



//
// A global that we keep around for use by the MonoDbgPrint function
// (when other drivers call MonoDbgPrint we need to be able to access
// the info stored in the device extension).
//

PMONO_DEVICE_EXTENSION GlobalDeviceExtension = NULL;



//
// A variable which determines the verboseness of the messages printed by
// MonoDbgPrint.
//

ULONG MonoDbgLevel = 3;



//
// Resources used by the monochrome video adapter
//

MONO_RESOURCE MonoResources[] =
{
    { 0x000b0000, 0x00000000,   // the video buffer
      0x00001000,               // length
      0,                        // in memory space
      1 },                      // shared with other drivers/devices (vga)

    { 0x000003b4, 0x00000000,   // the 6845 index & data regs
      0x00000002,               // length
      1,                        // in I/O space
      1 },                      // shared with other drivers/devices (vga)

    { 0x000b03b8, 0x00000000,   // mode control register
      0x00000001,               // length
      1,                        // in I/O space
      1 }                       // shared with other drivers/devices (vga)
};
