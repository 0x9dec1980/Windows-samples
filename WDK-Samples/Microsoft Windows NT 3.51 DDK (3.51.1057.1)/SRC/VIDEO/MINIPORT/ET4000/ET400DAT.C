/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    et400dat.c

Abstract:

    This module contains all the global data used by the et4000 driver.

Environment:

    Kernel mode

Revision History:


--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "et4000.h"

#include "cmdcnst.h"

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif

//
// This structure describes to which ports access is required.
//

VIDEO_ACCESS_RANGE VgaAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,                // 64-bit linear base address
                                                 // of range
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1, // # of ports
    1,                                           // range is in I/O space
    1,                                           // range should be visible
    0                                            // range should be shareable
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},
{
    CRTCB_IO_PORT_BASE, 0x00000000,       // 64-bit linear base address
    CRTCB_IO_PORT_LEN,                    // # of ports
    1,                                    // range is in I/O space
    1,                                    // range should be visible
    0                                     // range should be shareable
},
{
    0x000A0000, 0x00000000,
    0x00020000,
    0,
    1,
    0
}
};


//
// Validator Port list.
// This structure describes all the ports that must be hooked out of the V86
// emulator when a DOS app goes to full-screen mode.
// The structure determines to which routine the data read or written to a
// specific port should be sent.
//

EMULATOR_ACCESS_ENTRY VgaEmulatorAccessEntries[] = {

    //
    // Traps for byte OUTs.
    //

    {
        0x000003b0,                   // range start I/O address
        0xC,                         // range length
        Uchar,                        // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                        // does not support string accesses
        (PVOID)VgaValidatorUcharEntry // routine to which to trap
    },

    {
        0x000003c0,                   // range start I/O address
        0x20,                         // range length
        Uchar,                        // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                        // does not support string accesses
        (PVOID)VgaValidatorUcharEntry // routine to which to trap
    },

    //
    // Traps for word OUTs.
    //

    {
        0x000003b0,
        0x06,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
    },

    {
        0x000003c0,
        0x10,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
    },

    //
    // Traps for dword OUTs.
    //

    {
        0x000003b0,
        0x03,
        Ulong,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUlongEntry
    },

    {
        0x000003c0,
        0x08,
        Ulong,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUlongEntry
    }

};


//
// Used to trap only the sequncer and the misc output registers
//

VIDEO_ACCESS_RANGE MinimalVgaValidatorAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
    1,
    1,        // <- enable range IOPM so that it is not trapped.
    0
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},
{
    VGA_BASE_IO_PORT + MISC_OUTPUT_REG_WRITE_PORT, 0x00000000,
    0x00000001,
    1,
    0,
    0
},
{
    VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, 0x00000000,
    0x00000002,
    1,
    0,
    0
}
};

//
// Used to trap all registers
//

VIDEO_ACCESS_RANGE FullVgaValidatorAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
    1,
    0,        // <- disable range in the IOPM so that it is trapped.
    0
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    0,
    0
}
};

USHORT ET4K_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    EOD
};

// This is the only value that avoids broken rasters (at least they're not
// broken within the visible portion of the bitmap)
USHORT ET4K_1928_WIDE[] = {
    OW,                             // stretch scans to 1928
    CRTC_ADDRESS_PORT_COLOR,
    0xF113,

    EOD
};

//
// Memory map table -
//
// These memory maps are used to save and restore the physical video buffer.
//

MEMORYMAPS MemoryMaps[] = {

//               length      start
//               ------      -----
    {           0x08000,    0xB0000},   // all mono text modes (7)
    {           0x08000,    0xB8000},   // all color text modes (0, 1, 2, 3,
    {           0x10000,    0xA0000},   // all VGA graphics modes
};

//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//

VIDEOMODE ModesVGA[] = {

//
// Standard VGA modes.
//

//
// Mode index 0
// Color text mode 3, 720x400, 9x16 char cell (VGA).
//

{
  VIDEO_MODE_COLOR,  // flags that this mode is a color mode, but not graphics
  4,                 // four planes
  1,                 // one bit of colour per plane
  80, 25,            // 80x25 text resolution
  720, 400,          // 720x400 pixels on screen
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  0, 0,              // only support one frequency, non-interlaced
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
  FALSE,             // Mode is not available by default
  0x3,               // int 10 modesset value
  NULL,              // scan line stretching option
},

//
// Mode index 1.
// Color text mode 3, 640x350, 8x14 char cell (EGA).
//

{ VIDEO_MODE_COLOR, 4, 1, 80, 25,
  640, 350, 160, 0x10000, 0, 0, NoBanking, MemMap_CGA,
  FALSE,
  0x3,
  NULL,
},

//
//
// Mode index 2
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x12,
  NULL,
},

//
//
// Mode index 2a
// Standard VGA Color graphics mode 0x12, 640x480 16 colors. 72Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 72, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x12,
  NULL,
},

//
// Beginning of SVGA modes
//

//
// Mode index 3
// 800x600 16 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x29,
  NULL,
},

//
// Mode index 3a
// 800x600 16 colors. 72 hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 72, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x29,
  NULL,
},

//
// Mode index 3b
// 800x600 16 colors. 56 hz for 8514/a monitors... (fixed freq)
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 56, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x29,
  NULL,
},

//
// Mode index 4
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x37,
  NULL,
},

//
// Mode index 4a
// 1024x768 non-interlaced 16 colors. 70hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 70, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x37,
  NULL
},

//
// Mode index 4b
// 1024x768 non-interlaced 16 colors. Interlaced (45 hz)
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 45, 1, NormalBanking, MemMap_VGA,
  FALSE,
  0x37,
  NULL
},

//
// Mode index 6
// 640x480x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

//
// Mode index 6a
// 640x480x256 72 Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

//
// Mode index 6b (tomzak)
// 640x480x256 75 Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 75, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

//
// Mode index 6c (tomzak)
// 640x480x256 90 Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 90, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

// NOTE: 800x600 modes need 1Meg until we support broken rasters

//
// Mode index 7
// 800x600x256 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// Mode index 7a
// 800x600x256 72Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// Mode index 7b
// 800x600x256  56Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 56, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// Mode index 7c
// 800x600x256  75Hz (tomzak)
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 75, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// Mode index 7d
// 800x600x256  90Hz (tomzak)
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 90, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// Mode index 8
// 1024x768x256 60
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// Mode index 8a 70 hz
// 1024x768x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 70, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// Mode index 8b 45Hz (Interlaced)
// 1024x768x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 45, 1, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// Mode index 8c 72 hz (tomzak)
// 1024x768x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// Mode index 8d 75 hz (tomzak)
// 1024x768x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 75, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// Mode index 9
// 640x480x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1928, 0x100000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1928_WIDE
},

//
// Mode index 9a 72hz
// 640x480x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1928, 0x100000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1928_WIDE
},

//
// Mode index 10
// 800x600x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  800, 600, 800*2, 0x100000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

//
// Mode index 10a 72 Hz
// 800x600x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  800, 600, 800*2, 0x100000, 72, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

//
// Mode index 11
// 1024x768x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  1024, 768, 1024*2, 0x200000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// Mode index 11a 70 Hz
// 1024x768x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  1024, 768, 1024*2, 0x200000, 70, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},


//
// Mode index 12
// 1280x1024x256 60
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1280, 1024, 1280, 0x200000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x3F,
  NULL
},

//
// Mode index 12a 70 hz (tomzak)
// 1280x1024x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1280, 1024, 1280, 0x200000, 70, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x3F,
  NULL
},

//
// Mode index 12b 45Hz (Interlaced)
// 1280x1024x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1280, 1024, 1280, 0x200000, 45, 1, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x3F,
  NULL
},

//
// Mode index 12c 75 hz (tomzak)
// 1280x1024x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1280, 1024, 1280, 0x200000, 75, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x3F,
  NULL
},

//
// Mode index 13 60 Hz
// 640x480 24bpp
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 640*3, 0x100000, 60, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x2E,
  NULL
},

//
// Mode index 14
// 800x600 24bpp
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  800, 600, 800*3, 0x200000, 60, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

};

ULONG NumVideoModes = sizeof(ModesVGA) / sizeof(VIDEOMODE);


//
//
// Data used to set the Graphics and Sequence Controllers to put the
// VGA into a planar state at A0000 for 64K, with plane 2 enabled for
// reads and writes, so that a font can be loaded, and to disable that mode.
//

// Settings to enable planar mode with plane 2 enabled.
//

USHORT EnableA000Data[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0204,     // Read Map = plane 2
    0x0005, // Graphics Mode = read mode 0, write mode 0
    0x0406, // Graphics Miscellaneous register = A0000 for 64K, not odd/even,
            //  graphics mode
    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0402, // Map Mask = write to plane 2 only
    0x0404, // Memory Mode = not odd/even, not full memory, graphics mode
    0x0300,  // end sync reset
    EOD
};

//
// Settings to disable the font-loading planar mode.
//

USHORT DisableA000Color[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0004, 0x1005, 0x0E06,

    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0302, 0x0204, 0x0300,  // end sync reset
    EOD

};

#if defined(ALLOC_PRAGMA)
#pragma data_seg()
#endif
