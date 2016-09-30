/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cldata.c

Abstract:

    This module contains all the global data used by the cirrus driver.

Environment:

    Kernel mode

Revision History:


--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "cirrus.h"

#include "cmdcnst.h"

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif

//---------------------------------------------------------------------------
//
//        The actual register values for the supported modes are in chipset-specific
//        include files:
//
//                mode64xx.h has values for CL6410 and CL6420
//                mode542x.h has values for CL5422, CL5424, and CL5426
//                mode543x.h has values for CL5430-CL5439 (Alpine chips)
//
#include "mode6410.h"
#include "mode6420.h"
#include "mode542x.h"
#ifdef _PPC_
#include "ppc543x.h"
#else
#include "mode543x.h"
#endif


//
// This structure describes to which ports access is required.
//

VIDEO_ACCESS_RANGE VgaAccessRange[] = {
{
#ifdef _ALPHA_
    REAL_VGA_BASE_IO_PORT, 0x00000000,           // 64-bit linear base address
                                                 // of range
    VGA_START_BREAK_PORT - REAL_VGA_BASE_IO_PORT + 1, // # of ports
#else
    VGA_BASE_IO_PORT, 0x00000000,                // 64-bit linear base address
                                                 // of range
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1, // # of ports
#endif
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
    MEM_VGA, 0x00000000,
    MEM_VGA_SIZE,
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
#ifdef _ALPHA_
    REAL_VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - REAL_VGA_BASE_IO_PORT + 1,
#else
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
#endif
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
#ifdef _ALPHA_
    REAL_VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - REAL_VGA_BASE_IO_PORT + 1,
#else
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
#endif
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



USHORT MODESET_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    EOD
};

USHORT MODESET_2K_WIDE[] = {
    OWM,                             // stretch scans to 2k
    CRTC_ADDRESS_PORT_COLOR,
    2,
    0x0013,
    0x321B,

    EOD
};

#ifndef _X86

//
// For MIPS NEC machine only
//

//
// CR13 determine the display memory scanline width.
// Over 1KB memory per scanline required for 24bpp mode.
//

USHORT MODESET_640x3_WIDE[] = {
    OW,                             // stretch scans to 640 * 3 bytes.
    CRTC_ADDRESS_PORT_COLOR,
    0xF013, //0x0813, //0xF013,                         // CR13 = 0xf0

    EOD
};

#endif


//---------------------------------------------------------------------------
//
// Memory map table -
//
// These memory maps are used to save and restore the physical video buffer.
//

MEMORYMAPS MemoryMaps[] = {

//               length      start
//               ------      -----
    {           0x08000,    0x10000},   // all mono text modes (7)
    {           0x08000,    0x18000},   // all color text modes (0, 1, 2, 3,
    {           0x20000,    0x00000},   // all VGA graphics modes
    {           0x00000,    0x00000}
};

//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//

VIDEOMODE ModesVGA[] = {

#ifdef _X86_     // No text or 16-color modes on non-Intel platforms
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
  0,                 // montype is 'dont care' for text modes
  0, 0, 0,           // montype is 'dont care' for text modes
  TRUE,              // hardware cursor enabled for this mode
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
  CL6410 | CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
  crt | panel,
  FALSE,              // ModeValid default is always off
  { 3,3,3},          // int10 BIOS modes
  { CL6410_80x25Text_crt, CL6410_80x25Text_panel,
   CL6420_80x25Text_crt, CL6420_80x25Text_panel,
   CL542x_80x25Text, CL543x_80x25Text, 0 },
},

//
// Color text mode 3, 640x350, 8x14 char cell (EGA).
//
{  VIDEO_MODE_COLOR,  // flags that this mode is a color mode, but not graphics
  4,                 // four planes
  1,                 // one bit of colour per plane
  80, 25,            // 80x25 text resolution
  640, 350,          // 640x350 pixels on screen
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  0, 0,              // only support one frequency, non-interlaced
  0,                 // montype is 'dont care' for text modes
  0, 0, 0,           // montype is 'dont care' for text modes
  TRUE,              // hardware cursor enabled for this mode
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
   CL6410 | CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
   crt | panel,
   FALSE,              // ModeValid default is always off
  { 3,3,3},             // int10 BIOS modes
   { CL6410_80x25_14_Text_crt, CL6410_80x25_14_Text_panel,
     CL6420_80x25_14_Text_crt, CL6420_80x25_14_Text_panel,
     CL542x_80x25_14_Text, CL543x_80x25_14_Text, 0 },
},
//
//
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000,
  60, 0,              // 60hz, non-interlaced
  3,                  // montype
  0x1203, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL6410 | CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
  crt | panel,
  FALSE,                      // ModeValid default is always off
  { 0x12,0x12,0x12},          // int10 BIOS modes
  { CL6410_640x480_crt, CL6410_640x480_panel,
   CL6420_640x480_crt, CL6420_640x480_panel,
   CL542x_640x480_16, CL543x_640x480_16, 0 },
},

//
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000,
  72, 0,              // 72hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL542x,
  crt,
  FALSE,                      // ModeValid default is always off
  { 0,0,0x12},                // int10 BIOS modes
  { NULL, NULL,
   NULL, NULL,
   CL542x_640x480_16, NULL, 0 },
},

// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000,
  75, 0,              // 75hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL754x | CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                      // ModeValid default is always off
  { 0,0,0x12},                // int10 BIOS modes
  { NULL, NULL,
   NULL, NULL,
   NULL, CL543x_640x480_16, 0 },
},


//
// Beginning of SVGA modes
//

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000,
  56, 0,              // 56hz, non-interlaced
  3,                  // montype
  0x1203, 0xA4, 0,    // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL6410 | CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0x6a,0x6a,0x6a},       // int10 BIOS modes
  { CL6410_800x600_crt, NULL,
   CL6420_800x600_crt, NULL,
   CL542x_800x600_16, CL543x_800x600_16, 0 },
},

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000,
  60, 0,              // 60hz, non-interlaced
  4,                  // montype
  0x1203, 0x01A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0x6a,0x6a},          // int10 BIOS modes
  { NULL, NULL,
   CL6420_800x600_crt, NULL,
   CL542x_800x600_16, CL543x_800x600_16, 0 },
},

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000,
  72, 0,              // 72hz, non-interlaced
  5,                  // montype
  0x1203, 0x02A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL754x | CL542x | CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0,0x6a},             // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_16, CL543x_800x600_16, 0 },
},

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000,
  75, 0,              // 75hz, non-interlaced
  5,                  // montype
  0x1203, 0x03A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0,0x6a},             // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_16, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  60, 0,              // 60hz, non-interlaced
  5,                  // montype
  0x1203, 0x10A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL754x | CL542x | CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  70, 0,              // 70hz, non-interlaced
  6,                  // montype
  0x1203, 0x20A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL754x | CL542x | CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
   CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  72, 0,              // 72hz, non-interlaced
  7,                  // montype
  0x1203, 0x30A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL542x | CL543x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  75, 0,              // 75hz, non-interlaced
  7,                  // montype
  0x1203, 0x40A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL754x | CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_16, 0 },
},

//
// 1024x768 interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  45, 1,              // 45hz, interlaced
  4,                  // montype
  0x1203, 0xA4, 0,    // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0x37,0x5d},       // int10 BIOS modes
  { NULL, NULL,
   CL6420_1024x768_crt, NULL,
   CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},

//
// 1280x1024 interlaced 16 colors.
// Assumes 1meg required. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 160, 64,
  1280, 1024, 256, 0x40000,
  45, 1,              // 45Hz, interlaced
  5,                  // montype
  0x1203, 0xA4, 0,    // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL754x | CL542x | CL543x | CL5434 | CL5434_6,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6c},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1280x1024_16, CL543x_1280x1024_16, MODESET_1K_WIDE},
},

#endif   //def _X86_

//
//
// VGA Color graphics,        640x480 256 colors. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000,
  60, 0,              // 60hz, non-interlaced
  3,                  // montype
  0x1203, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  CL542x | CL543x | CL5434 | CL5434_6,
#else
  CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
#endif
  crt | panel,
  FALSE,                // ModeValid default is always off
  { 0,0x2e,0x5f},       // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_256, MODESET_1K_WIDE },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_256a60, SNI_640x480_256a60, MODESET_1K_WIDE},
#else
  { NULL, NULL,
    CL6420_640x480_256color_crt, CL6420_640x480_256color_panel,
    CL542x_640x480_256, CL543x_640x480_256, MODESET_1K_WIDE },
#endif
},

//
//
// VGA Color graphics,        640x480 256 colors. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000,
  72, 0,              // 72hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  0,
#elif _MIPS_
  CL542x | CL543x | CL5434 | CL5434_6,
#else
  CL754x | CL542x,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5f},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_256a72, SNI_640x480_256a72, MODESET_1K_WIDE },
#else
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_256, CL543x_640x480_256, MODESET_1K_WIDE },
#endif
},

//
//
// VGA Color graphics,        640x480 256 colors. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000,
  75, 0,              // 75hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _MIPS_
  0,
#else
  CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x5f},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_256_75, MODESET_1K_WIDE },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_256, MODESET_1K_WIDE },
#endif
},

//
// 800x600 256 colors. 1K scan line requires 1 MEG
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000,
  56, 0,              // 56hz, non-interlaced
  3,                  // montype
  0x1203, 0xA4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  0,
#elif _MIPS_
  CL542x,
#else
  CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0x30,0x5c},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_256a56, NULL, MODESET_1K_WIDE },
#else
  { NULL, NULL,
    CL6420_800x600_256color_crt, NULL,
    CL542x_800x600_256, CL543x_800x600_256, MODESET_1K_WIDE },
#endif
},

//
// 800x600 256 colors. 1K scan line requires 1 MEG
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000,
  60, 0,              // 60hz, non-interlaced
  4,                  // montype
  0x1203, 0x01A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  CL542x | CL543x | CL5434 | CL5434_6,
#else
  CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
#endif
  crt | panel8x6,
  FALSE,                   // ModeValid default is always off
  { 0,0x30,0x5c},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256, MODESET_1K_WIDE },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_256a60, SNI_800x600_256a60, MODESET_1K_WIDE },
#else
  { NULL, NULL,
    CL6420_800x600_256color_crt, NULL,
    CL542x_800x600_256, CL543x_800x600_256, MODESET_1K_WIDE },
#endif
},

//
// 800x600 256 colors. 1K scan line requires 1 MEG
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000,
  72, 0,              // 72hz, non-interlaced
  5,                  // montype
  0x1203, 0x02A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  CL542x | CL543x | CL5434 | CL5434_6,
#else
  CL754x | CL542x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0,0x5c},             // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256_72, MODESET_1K_WIDE },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_256a72, SNI_800x600_256a72, MODESET_1K_WIDE },
#else
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_256, CL543x_800x600_256, MODESET_1K_WIDE },
#endif
},

//
// 800x600 256 colors. 1K scan line requires 1 MEG
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000,
  75, 0,              // 75hz, non-interlaced
  5,                  // montype
  0x1203, 0x03A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _MIPS_
  0,
#else
  CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                   // ModeValid default is always off
  { 0,0,0x5c},             // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256_75, MODESET_1K_WIDE },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256, MODESET_1K_WIDE },
#endif
},
//
// 1024x768 non-interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  60, 0,              // 60hz, non-interlaced
  5,                  // montype
  0x1203, 0x10A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  CL542x | CL543x | CL5434 | CL5434_6,
#else
  CL754x | CL542x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x60},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024_256, 0 },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_256a60, SNI_1024x768_256a61, NULL},
#else
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
#endif
},

//
// 1024x768 non-interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  70, 0,              // 70hz, non-interlaced
  6,                  // montype
  0x1203, 0x20A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  CL542x | CL543x | CL5434 | CL5434_6,
#else
  CL754x | CL542x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x60},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024_256_70, 0 },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_256a70, SNI_1024x768_256a70, 0 },
#else
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
#endif
},

//
// 1024x768 non-interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  72, 0,              // 72hz, non-interlaced
  7,                  // montype
  0x1203, 0x30A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x,
#elif _MIPS_
  CL543x | CL5434 | CL5434_6,
#else
  CL542x | CL543x,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x60},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024_256_72, 0 },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    NULL, SNI_1024x768_256a73, 0 },
#else
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
#endif
},

//
// 1024x768 non-interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  75, 0,              // 75hz, non-interlaced
  7,                  // montype
  0x1203, 0x40A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  CL543x | CL5434 | CL5434_6,
#else
  CL754x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x60},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024_256_75, 0 },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    NULL, SNI_1024x768_256a75, 0 },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_16, 0 },
#endif
},

//
// 1024x768 interlaced 256 colors.
// Assumes 1Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  45, 1,              // 45hz, interlaced
  4,                  // montype
  0x1203, 0xA4, 0,    // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  CL542x,
#else
  CL6420 | CL754x | CL542x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0x38,0x60},       // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024_256_43I, 0 },
#elif _MIPS_
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_256a87, NULL, 0 },
#else
  { NULL, NULL,
    CL6420_1024x768_256color_crt, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
#endif
},

//
// 1280x1024 interlaced 256 colors.
// Assumes 2Meg.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000,
  45, 1,              // 45hz, interlaced
  5,                  // montype
  0x1203, 0xA4, 0,    // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  0,
#else
  CL754x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6D},       // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280_256_43I, MODESET_2K_WIDE },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, MODESET_2K_WIDE },
#endif
},

//
// 1280x1024 non-interlaced 256 colors.
// Assumes 2meg required.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000,
  60, 0,              // 60Hz, non-interlaced
  0,                  // montype
  0x1203, 0xA4, 0x1000, // montype
  FALSE,              // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _MIPS_
  0,
#else
  CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6D},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280_256, MODESET_2K_WIDE },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, MODESET_2K_WIDE },
#endif
},

//
// 1280x1024 non-interlaced 256 colors.
// Assumes 2meg required.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000,
  71, 0,              // 71Hz, non-interlaced
  0,                  // montype
  0x1203, 0xA4, 0x2000, // montype
  FALSE,              // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  0,
#elif _MIPS_
  0,
#else
  CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6D},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
//
// We need a CL543x_1280x1024 mode for the PPC (must be added
// to ppc543x.h).  Until we get this, I'll put the code for
// 60hz mode.
//
#ifdef _PPC_
    NULL, CL543x_1280_256, MODESET_2K_WIDE },
#else
    NULL, CL543x_1280x1024_16, MODESET_2K_WIDE },
#endif
},

//
// 1280x1024 non-interlaced 256 colors.
// Assumes 2meg required.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000,
  75, 0,              // 75Hz, non-interlaced
  0,                  // montype
  0x1203, 0xA4, 0x3000, // montype
  FALSE,              // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  0,
#elif _MIPS_
  0,
#else
  CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x6D},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
//
// We need a CL543x_1280x1024 mode for the PPC (must be added
// to ppc543x.h).  Until we get this, I'll put the code for
// 60hz mode.
//
#ifdef _PPC_
    NULL, CL543x_1280_256, MODESET_2K_WIDE },
#else
    NULL, CL543x_1280x1024_16, MODESET_2K_WIDE },
#endif
},

//
// The Cirrus Display Driver now supports broken rasters,
// so I have enabled support for broken rasters in the
// miniport.
//
// Eventually we will probably want to add additional
// (equivalent) modes which don't require broken rasters.
//
// To get back to these modes, make the wbytes field
// equal to 2048, set the memory requirements field
// appropriately (1 meg for 640x480x64k, 2 meg for
// 800x600x64).
//
// Finally for non broken rasters we need to the
// stretch from NULL to MODESET_2K_WIDE.
//

//
// VGA Color graphics,        640x480 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000,
  60, 0,              // 60hz, non-interlaced
  3,                  // montype
  0x1203, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  0,
#else
  CL754x | CL542x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x64},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, MODESET_2K_WIDE },
#else
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_64k, CL543x_640x480_64k, MODESET_2K_WIDE },
#endif
},

//
//
// VGA Color graphics,        640x480 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000,
  72, 0,              // 72hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  0,
#elif _MIPS_
  0,
#else
  CL542x,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x64},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_64k, NULL, MODESET_2K_WIDE },
},

//
//
// VGA Color graphics,        640x480 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000,
  75, 0,              // 75hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  0,
#else
  CL754x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x64},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k_75, MODESET_2K_WIDE },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, MODESET_2K_WIDE },
#endif
},

//
//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  56, 0,              // 56hz, non-interlaced
  4,                  // montype
  0x1203, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  0,
#elif _MIPS_
  0,
#else
  CL754x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x65},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
},

//
//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  60, 0,              // 60hz, non-interlaced
  4,                  // montype
  0x1203, 0x01A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  0,
#else
  CL754x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x65},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, },
#endif
},

//
//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  72, 0,              // 72hz, non-interlaced
  5,                  // montype
  0x1203, 0x02A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _MIPS_
  0,
#else
  CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x65},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k_72, NULL },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
#endif
},

//
//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  75, 0,              // 75hz, non-interlaced
  5,                  // montype
  0x1203, 0x03A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _MIPS_
  0,
#else
  CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x65},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k_75, NULL },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
#endif
},

//
//
//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  45, 1,              // 45hz, interlaced
  5,                  // montype
  0x1203, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _PPC_
  CL543x | CL5434 | CL5434_6,
#elif _MIPS_
  0,
#else
  CL754x | CL543x | CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x74},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024_64k_43I, 0 },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
#endif
},

//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  60, 0,              // 60hz, non-interlaced
  5,                  // montype
  0x1203, 0x10A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _MIPS_
  0,
#else
  CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x74},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024_64k, 0 },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
#endif
},

//
//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  70, 0,              // 70hz, non-interlaced
  6,                  // montype
  0x1203, 0x20A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _MIPS_
  0,
#else
  CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x74},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024_64k_70, 0 },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
#endif
},

//
//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  75, 0,              // 75hz, non-interlaced
  7,                  // montype
  0x1203, 0x40A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _MIPS_
  0,
#else
  CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x74},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024_64k_75, 0 },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
#endif
},

//
// NOTE: We can not get into the following mode, because
// we don't have a MODESET_4K_WIDE.  (I don't think we
// can make one).  I'll have to talk to MikeH and see if
// he knows of a way to tell the card to use 4K scan
// lines.
//

#if 0
//
// 1280x1024 interlaced 64k colors.
// Assumes 4meg required.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 160, 64,
  1280, 1024, 4096, 0x400000,
  45, 1,              // 45Hz, interlaced
  0,                  // montype
  0x1203, 0xA4, 0x0000, // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
#ifdef _MIPS_
  0,
#else
  CL5434 | CL5434_6,
#endif
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0,0x75},          // int10 BIOS modes
#ifdef _PPC_
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280_256_43I, MODESET_2K_WIDE },
#else
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, MODESET_2K_WIDE },
#endif
},
#endif

#ifdef _MIPS_
//
// True Color graphics,        640x480 16M colors. 1K scan line
//
// For MIPS NEC machine only
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 640 * 3, 0x100000,
  1, 0,              // 60hz, non-interlaced
  3,                  // montype
  0, 0, 0,
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL542x,
  crt,
  FALSE,                // ModeValid default is always off
  { 0,0x2e,0x5f},       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_16M, NULL, MODESET_640x3_WIDE },
},
#endif

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
