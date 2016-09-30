/******************************Module*Header*******************************\
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1993-1995 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

#define DBG_3DDDI   0

/******************************Public*Structure****************************\
* GDIINFO ggdiDefault
*
* This contains the default GDIINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for an 8bpp palette device.
*       Some fields are overwritten for different colour depths.
\**************************************************************************/

GDIINFO ggdiDefault = {
    0x3500,                 // ulVersion (our driver is version 3.5.00) // !!!
    DT_RASDISPLAY,          // ulTechnology
    0,                      // ulHorzSize (filled in later)
    0,                      // ulVertSize (filled in later)
    0,                      // ulHorzRes (filled in later)
    0,                      // ulVertRes (filled in later)
    0,                      // cBitsPixel (filled in later)
    0,                      // cPlanes (filled in later)
    20,                     // ulNumColors (palette managed)
    0,                      // flRaster (DDI reserved field)

    0,                      // ulLogPixelsX (filled in later)
    0,                      // ulLogPixelsY (filled in later)

    TC_RA_ABLE,             // flTextCaps -- If we had wanted console windows
                            //   to scroll by repainting the entire window,
                            //   instead of doing a screen-to-screen blt, we
                            //   would have set TC_SCROLLBLT (yes, the flag is
                            //   bass-ackwards).

    0,                      // ulDACRed (filled in later)
    0,                      // ulDACGreen (filled in later)
    0,                      // ulDACBlue (filled in later)

    0x0024,                 // ulAspectX
    0x0024,                 // ulAspectY
    0x0033,                 // ulAspectXY (one-to-one aspect ratio)

    1,                      // xStyleStep
    1,                      // yStyleSte;
    3,                      // denStyleStep -- Styles have a one-to-one aspect
                            //   ratio, and every 'dot' is 3 pixels long

    { 0, 0 },               // ptlPhysOffset
    { 0, 0 },               // szlPhysSize

    256,                    // ulNumPalReg

    // These fields are for halftone initialization.  The actual values are
    // a bit magic, but seem to work well on our display.

    {                       // ciDevice
       { 6700, 3300, 0 },   //      Red
       { 2100, 7100, 0 },   //      Green
       { 1400,  800, 0 },   //      Blue
       { 1750, 3950, 0 },   //      Cyan
       { 4050, 2050, 0 },   //      Magenta
       { 4400, 5200, 0 },   //      Yellow
       { 3127, 3290, 0 },   //      AlignmentWhite
       20000,               //      RedGamma
       20000,               //      GreenGamma
       20000,               //      BlueGamma
       0, 0, 0, 0, 0, 0     //      No dye correction for raster displays
    },

    0,                       // ulDevicePelsDPI (for printers only)
    PRIMARY_ORDER_CBA,       // ulPrimaryOrder
    HT_PATSIZE_4x4_M,        // ulHTPatternSize
    HT_FORMAT_8BPP,          // ulHTOutputFormat
    HT_FLAG_ADDITIVE_PRIMS,  // flHTFlags
    0,                       // ulVRefresh
    0,                       // ulDesktopHorzRes
    0,                       // ulDesktopVertRes
    0,                       // ulBltAlignment
};

/******************************Public*Structure****************************\
* DEVINFO gdevinfoDefault
*
* This contains the default DEVINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for an 8bpp palette device.
*       Some fields are overwritten for different colour depths.
\**************************************************************************/

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       FIXED_PITCH | FF_DONTCARE, L"Courier"}

DEVINFO gdevinfoDefault = {
    (GCAPS_OPAQUERECT       |
     GCAPS_DITHERONREALIZE  |
     GCAPS_PALMANAGED       |
     GCAPS_ALTERNATEFILL    |
     GCAPS_WINDINGFILL      |
     GCAPS_MONO_DITHER      |
     GCAPS_COLOR_DITHER     |
     GCAPS_BEZIERS          |
     GCAPS_ASYNCMOVE),          // NOTE: Only enable ASYNCMOVE if your code
                                //   and hardware can handle DrvMovePointer
                                //   calls at any time, even while another
                                //   thread is in the middle of a drawing
                                //   call such as DrvBitBlt.

                                                // flGraphicsFlags
    SYSTM_LOGFONT,                              // lfDefaultFont
    HELVE_LOGFONT,                              // lfAnsiVarFont
    COURI_LOGFONT,                              // lfAnsiFixFont
    0,                                          // cFonts
    BMF_8BPP,                                   // iDitherFormat
    8,                                          // cxDither
    8,                                          // cyDither
    0                                           // hpalDefault (filled in later)
};

/******************************Public*Structure****************************\
* DFVFN gadrvfn[]
*
* Build the driver function table gadrvfn with function index/address
* pairs.  This table tells GDI which DDI calls we support, and their
* location (GDI does an indirect call through this table to call us).
*
* Why haven't we implemented DrvSaveScreenBits?  To save code.
*
* When the driver doesn't hook DrvSaveScreenBits, USER simulates on-
* the-fly by creating a temporary device-format-bitmap, and explicitly
* calling DrvCopyBits to save/restore the bits.  Since we already hook
* DrvCreateDeviceBitmap, we'll end up using off-screen memory to store
* the bits anyway (which would have been the main reason for implementing
* DrvSaveScreenBits).  So we may as well save some working set.
\**************************************************************************/

#if MULTI_BOARDS

// Multi-board support has its own thunks...

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) MulEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) MulCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) MulDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) MulEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) MulDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) MulAssertMode         },
    {   INDEX_DrvMovePointer,           (PFN) MulMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) MulSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) MulDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) MulSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) MulCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) MulBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) MulTextOut            },
    {   INDEX_DrvGetModes,              (PFN) MulGetModes           },
    {   INDEX_DrvStrokePath,            (PFN) MulStrokePath         },
    {   INDEX_DrvFillPath,              (PFN) MulFillPath           },
    {   INDEX_DrvPaint,                 (PFN) MulPaint              },
    {   INDEX_DrvRealizeBrush,          (PFN) MulRealizeBrush       },
    {   INDEX_DrvDestroyFont,           (PFN) MulDestroyFont        },
    // Note that DrvStretchBlt is not supported for multi-boards
    // Note that DrvCreateDeviceBitmap is not supported for multi-boards
    // Note that DrvDeleteDeviceBitmap is not supported for multi-boards
    // Note that DrvEscape is not supported for multi-boards
};

#elif DBG

// On Checked builds, or when we have to synchronize access, thunk
// everything through Dbg calls...

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DbgEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DbgCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DbgDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DbgEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DbgDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DbgAssertMode         },
    {   INDEX_DrvMovePointer,           (PFN) DbgMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) DbgSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) DbgDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) DbgSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) DbgCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) DbgBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) DbgTextOut            },
    {   INDEX_DrvGetModes,              (PFN) DbgGetModes           },
    {   INDEX_DrvStrokePath,            (PFN) DbgStrokePath         },
    {   INDEX_DrvFillPath,              (PFN) DbgFillPath           },
    {   INDEX_DrvPaint,                 (PFN) DbgPaint              },
    {   INDEX_DrvRealizeBrush,          (PFN) DbgRealizeBrush       },
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DbgCreateDeviceBitmap },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DbgDeleteDeviceBitmap },
    {   INDEX_DrvDestroyFont,           (PFN) DbgDestroyFont        },
    {   INDEX_DrvStretchBlt,            (PFN) DbgStretchBlt         },
    {   INDEX_DrvEscape,                (PFN) DbgEscape             },
};

#else

// On Free builds, directly call the appropriate functions...

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes           },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath           },
    {   INDEX_DrvPaint,                 (PFN) DrvPaint              },
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush       },
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap },
    {   INDEX_DrvDestroyFont,           (PFN) DrvDestroyFont        },
    {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt         },
    {   INDEX_DrvEscape,                (PFN) DrvEscape             },
};

#endif

ULONG gcdrvfn = sizeof(gadrvfn) / sizeof(DRVFN);

/******************************Public*Routine******************************\
* BOOL DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

BOOL DrvEnableDriver(
ULONG          iEngineVersion,
ULONG          cj,
DRVENABLEDATA* pded)
{
    // Engine Version is passed down so future drivers can support previous
    // engine versions.  A next generation driver can support both the old
    // and new engine conventions if told what version of engine it is
    // working with.  For the first version the driver does nothing with it.

    // Fill in as much as we can.

    if (cj >= sizeof(DRVENABLEDATA))
        pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
        pded->c = gcdrvfn;

    // DDI version this driver was targeted for is passed back to engine.
    // Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
        pded->iDriverVersion = DDI_DRIVER_VERSION;

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DrvDisableDriver
*
* Tells the driver it is being disabled. Release any resources allocated in
* DrvEnableDriver.
*
\**************************************************************************/

VOID DrvDisableDriver(VOID)
{
    return;
}

/******************************Public*Routine******************************\
* DHPDEV DrvEnablePDEV
*
* Initializes a bunch of fields for GDI, based on the mode we've been asked
* to do.  This is the first thing called after DrvEnableDriver, when GDI
* wants to get some information about us.
*
* (This function mostly returns back information; DrvEnableSurface is used
* for initializing the hardware and driver components.)
*
\**************************************************************************/

DHPDEV DrvEnablePDEV(
DEVMODEW*   pdm,            // Contains data pertaining to requested mode
PWSTR       pwszLogAddr,    // Logical address
ULONG       cPat,           // Count of standard patterns
HSURF*      phsurfPatterns, // Buffer for standard patterns
ULONG       cjCaps,         // Size of buffer for device caps 'pdevcaps'
ULONG*      pdevcaps,       // Buffer for device caps, also known as 'gdiinfo'
ULONG       cjDevInfo,      // Number of bytes in device info 'pdi'
DEVINFO*    pdi,            // Device information
PWSTR       pwszDataFile,   // Name of data file
PWSTR       pwszDeviceName, // Device name
HANDLE      hDriver)        // Kernel driver handle
{
    PDEV*   ppdev;

    // Future versions of NT had better supply 'devcaps' and 'devinfo'
    // structures that are the same size or larger than the current
    // structures:

    if ((cjCaps < sizeof(GDIINFO)) || (cjDevInfo < sizeof(DEVINFO)))
    {
        DISPDBG((0, "DrvEnablePDEV - Buffer size too small"));
        goto ReturnFailure0;
    }

    // Allocate a physical device structure.  Note that we definitely
    // rely on the zero initialization:

    ppdev = (PDEV*) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(PDEV));
    if (ppdev == NULL)
    {
        DISPDBG((0, "DrvEnablePDEV - Failed LocalAlloc"));
        goto ReturnFailure0;
    }

    ppdev->hDriver = hDriver;

    // Get the current screen mode information.  Set up device caps and
    // devinfo:

    if (!bInitializeModeFields(ppdev, (GDIINFO*) pdevcaps, pdi, pdm))
    {
        DISPDBG((0, "DrvEnablePDEV - Failed bInitializeModeFields"));
        goto ReturnFailure1;
    }

    // Initialize palette information.

    if (!bInitializePalette(ppdev, pdi))
    {
        DISPDBG((0, "DrvEnablePDEV - Failed bInitializePalette"));
        goto ReturnFailure1;
    }

    // Initialize device standard patterns.

    if (!bInitializePatterns(ppdev, min(cPat, HS_DDI_MAX)))
    {
        DISPDBG((0, "DrvEnablePDEV - Failed bInitializePatterns"));
        goto ReturnFailure1;
    }

    // Set the phsurfPatterns array to handles to each of the standard
    // patterns that were just created.

    RtlCopyMemory(phsurfPatterns, ppdev->ahbmPat,
                  ppdev->cPatterns * sizeof(HBITMAP));

    return((DHPDEV) ppdev);

ReturnFailure1:
    DrvDisablePDEV((DHPDEV) ppdev);

ReturnFailure0:
    DISPDBG((0, "Failed DrvEnablePDEV"));

    return(0);
}

/******************************Public*Routine******************************\
* VOID DrvDisablePDEV
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
* Note that this function will be called when previewing modes in the
* Display Applet, but not at system shutdown.  If you need to reset the
* hardware at shutdown, you can do it in the miniport by providing a
* 'HwResetHw' entry point in the VIDEO_HW_INITIALIZATION_DATA structure.
*
* Note: In an error, we may call this before DrvEnablePDEV is done.
*
\**************************************************************************/

VOID DrvDisablePDEV(
DHPDEV  dhpdev)
{
    PDEV*           ppdev;

    ppdev = (PDEV*) dhpdev;

    vUninitializePatterns(ppdev);
    vUninitializePalette(ppdev);
    LocalFree(ppdev);
}

/******************************Public*Routine******************************\
* VOID DrvCompletePDEV
*
* Store the HPDEV, the engines handle for this PDEV, in the DHPDEV.
*
\**************************************************************************/

VOID DrvCompletePDEV(
DHPDEV dhpdev,
HDEV   hdev)
{
    ((PDEV*) dhpdev)->hdevEng = hdev;
}

/******************************Public*Routine******************************\
* HSURF DrvEnableSurface
*
* Creates the drawing surface, initializes the hardware, and initializes
* driver components.  This function is called after DrvEnablePDEV, and
* performs the final device initialization.
*
\**************************************************************************/

HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    PDEV*       ppdev;
    HSURF       hsurf;
    SIZEL       sizl;
    DSURF*      pdsurf;
    VOID*       pvTmpBuffer;

    ppdev = (PDEV*) dhpdev;

    /////////////////////////////////////////////////////////////////////
    // First enable all the subcomponents.
    //
    // Note that the order in which these 'Enable' functions are called
    // may be significant in low off-screen memory conditions, because
    // the off-screen heap manager may fail some of the later
    // allocations...

    if (!bEnableHardware(ppdev))
        goto ReturnFailure;

    if (!bEnableOffscreenHeap(ppdev))
        goto ReturnFailure;

    if (!bEnableRx(ppdev))
        goto ReturnFailure;

    if (!bEnablePointer(ppdev))
        goto ReturnFailure;

    if (!bEnableText(ppdev))
        goto ReturnFailure;

    if (!bEnableBrushCache(ppdev))
        goto ReturnFailure;

    if (!bEnablePalette(ppdev))
        goto ReturnFailure;

    /////////////////////////////////////////////////////////////////////
    // Now create our private surface structure.
    //
    // Whenever we get a call to draw directly to the screen, we'll get
    // passed a pointer to a SURFOBJ whose 'dhpdev' field will point
    // to our PDEV structure, and whose 'dhsurf' field will point to the
    // following DSURF structure.
    //
    // Every device bitmap we create in DrvCreateDeviceBitmap will also
    // have its own unique DSURF structure allocated (but will share the
    // same PDEV).  To make our code more polymorphic for handling drawing
    // to either the screen or an off-screen bitmap, we have the same
    // structure for both.

    pdsurf = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(DSURF));
    if (pdsurf == NULL)
    {
        DISPDBG((0, "DrvEnableSurface - Failed pdsurf LocalAlloc"));
        goto ReturnFailure;
    }

    ppdev->pdsurfScreen = pdsurf;           // Remember it for clean-up
    pdsurf->poh     = ppdev->pohScreen;     // The screen is a surface, too
    pdsurf->dt      = DT_SCREEN;            // Not to be confused with a DIB
    pdsurf->sizl.cx = ppdev->cxScreen;
    pdsurf->sizl.cy = ppdev->cyScreen;
    pdsurf->ppdev   = ppdev;

    /////////////////////////////////////////////////////////////////////
    // Next, have GDI create the actual SURFOBJ.
    //
    // Since we can map the entire framebuffer linearly into main memory
    // (i.e., we didn't have to go through a 64k aperture), it is
    // beneficial to create the surface via EngCreateBitmap, giving GDI a
    // pointer to the framebuffer bits.

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    // Device-managed surface:

    hsurf = EngCreateDeviceSurface((DHSURF) pdsurf, sizl, ppdev->iBitmapFormat);
    if (hsurf == 0)
    {
        DISPDBG((0, "DrvEnableSurface - Failed EngCreateDeviceSurface"));
        goto ReturnFailure;
    }

    ppdev->hsurfScreen = hsurf;             // Remember it for clean-up
    ppdev->bEnabled = TRUE;                 // We'll soon be in graphics mode

    /////////////////////////////////////////////////////////////////////
    // Now associate the surface and the PDEV.
    //
    // We have to associate the surface we just created with our physical
    // device so that GDI can get information related to the PDEV when
    // it's drawing to the surface (such as, for example, the length of
    // styles on the device when simulating styled lines).

    if (!EngAssociateSurface(hsurf, ppdev->hdevEng, ppdev->flHooks))
    {
        DISPDBG((0, "DrvEnableSurface - Failed EngAssociateSurface"));
        goto ReturnFailure;
    }

    // Create our generic temporary buffer, which may be used by any
    // component.  Because this may get swapped out of memory any time
    // the driver is not active, we want to minimize the number of pages
    // it takes up.  We use 'VirtualAlloc' to get an exactly page-aligned
    // allocation (which 'LocalAlloc' will not do):

    pvTmpBuffer = VirtualAlloc(NULL, TMP_BUFFER_SIZE, MEM_RESERVE | MEM_COMMIT,
                               PAGE_READWRITE);
    if (pvTmpBuffer == NULL)
    {
        DISPDBG((0, "DrvEnableSurface - Failed VirtualAlloc"));
        goto ReturnFailure;
    }

    ppdev->pvTmpBuffer = pvTmpBuffer;

    DISPDBG((5, "Passed DrvEnableSurface"));

    return(hsurf);

ReturnFailure:
    DrvDisableSurface((DHPDEV) ppdev);

    DISPDBG((0, "Failed DrvEnableSurface"));

    return(0);
}

/******************************Public*Routine******************************\
* VOID DrvDisableSurface
*
* Free resources allocated by DrvEnableSurface.  Release the surface.
*
* Note that this function will be called when previewing modes in the
* Display Applet, but not at system shutdown.  If you need to reset the
* hardware at shutdown, you can do it in the miniport by providing a
* 'HwResetHw' entry point in the VIDEO_HW_INITIALIZATION_DATA structure.
*
* Note: In an error case, we may call this before DrvEnableSurface is
*       completely done.
*
\**************************************************************************/

VOID DrvDisableSurface(
DHPDEV dhpdev)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) dhpdev;

    // Note: In an error case, some of the following relies on the
    //       fact that the PDEV is zero-initialized, so fields like
    //       'hsurfScreen' will be zero unless the surface has been
    //       sucessfully initialized, and makes the assumption that
    //       EngDeleteSurface can take '0' as a parameter.

    vDisablePalette(ppdev);
    vDisableBrushCache(ppdev);
    vDisableText(ppdev);
    vDisablePointer(ppdev);
    vDisableRx(ppdev);
    vDisableOffscreenHeap(ppdev);
    vDisableHardware(ppdev);

    VirtualFree(ppdev->pvTmpBuffer, 0, MEM_RELEASE);
    EngDeleteSurface(ppdev->hsurfScreen);
    LocalFree(ppdev->pdsurfScreen);
}

/******************************Public*Routine******************************\
* VOID DrvAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

VOID DrvAssertMode(
DHPDEV  dhpdev,
BOOL    bEnable)
{
    PDEV* ppdev;

    ppdev = (PDEV*) dhpdev;

    ppdev->bEnabled = bEnable;

    if (!bEnable)
    {
        //////////////////////////////////////////////////////////////
        // Disable - Switch to full-screen mode

        vAssertModePalette(ppdev, FALSE);

        vAssertModeBrushCache(ppdev, FALSE);

        vAssertModeText(ppdev, FALSE);

        vAssertModePointer(ppdev, FALSE);

        vAssertModeRx(ppdev, FALSE);

        vAssertModeOffscreenHeap(ppdev, FALSE);

        // Let's hope the mode-set back to graphics mode doesn't fail,
        // because we have no way of reporting an error back to GDI:

        bAssertModeHardware(ppdev, FALSE);
    }
    else
    {
        //////////////////////////////////////////////////////////////
        // Enable - Switch back to graphics mode

        // We have to enable every subcomponent in the reverse order
        // in which it was disabled:

        bAssertModeHardware(ppdev, TRUE);

        vAssertModeOffscreenHeap(ppdev, TRUE);

        vAssertModeRx(ppdev, TRUE);

        vAssertModePointer(ppdev, TRUE);

        vAssertModeText(ppdev, TRUE);

        vAssertModeBrushCache(ppdev, TRUE);

        vAssertModePalette(ppdev, TRUE);
    }
}

/******************************Public*Routine******************************\
* ULONG DrvGetModes
*
* Returns the list of available modes for the device.
*
\**************************************************************************/

ULONG DrvGetModes(
HANDLE      hDriver,
ULONG       cjSize,
DEVMODEW*   pdm)
{
    DWORD cModes;
    DWORD cbOutputSize;
    PVIDEO_MODE_INFORMATION pVideoModeInformation;
    PVIDEO_MODE_INFORMATION pVideoTemp;
    DWORD cOutputModes = cjSize / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    DWORD cbModeSize;
    VIDEO_MODE_INFORMATION DefaultMode;

    cModes = getAvailableModes(hDriver,
                            (PVIDEO_MODE_INFORMATION *) &pVideoModeInformation,
                            &cbModeSize);
    if (cModes == 0)
    {
        DISPDBG((0, "DrvGetModes failed to get mode information"));
        return(0);
    }

    if (pdm == NULL)
    {
        cbOutputSize = cModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else
    {
        //
        // Now copy the information for the supported modes back into the
        // output buffer
        //

        cbOutputSize = 0;

        pVideoTemp = pVideoModeInformation;

        do
        {
            if (pVideoTemp->Length != 0)
            {
                if (cOutputModes == 0)
                {
                    break;
                }

                //
                // Zero the entire structure to start off with.
                //

                memset(pdm, 0, sizeof(DEVMODEW));

                //
                // Set the name of the device to the name of the DLL.
                //

                memcpy(pdm->dmDeviceName, DLL_NAME, sizeof(DLL_NAME));

                pdm->dmSpecVersion = DM_SPECVERSION;
                pdm->dmDriverVersion = DM_SPECVERSION;

                //
                // We currently do not support Extra information in the driver
                //

                pdm->dmDriverExtra = DRIVER_EXTRA_SIZE;

                pdm->dmSize = sizeof(DEVMODEW);
                pdm->dmBitsPerPel = pVideoTemp->NumberOfPlanes *
                                    pVideoTemp->BitsPerPlane;
                pdm->dmPelsWidth = pVideoTemp->VisScreenWidth;
                pdm->dmPelsHeight = pVideoTemp->VisScreenHeight;
                pdm->dmDisplayFrequency = pVideoTemp->Frequency;

                if (pVideoTemp->AttributeFlags & VIDEO_MODE_INTERLACED)
                {
                    pdm->dmDisplayFlags |= DM_INTERLACED;
                }

                //
                // Go to the next DEVMODE entry in the buffer.
                //

                cOutputModes--;

                pdm = (LPDEVMODEW) ( ((ULONG)pdm) + sizeof(DEVMODEW) +
                                                   DRIVER_EXTRA_SIZE);

                cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

            }

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                (((PUCHAR)pVideoTemp) + cbModeSize);


        } while (--cModes);
    }

    LocalFree(pVideoModeInformation);

    return(cbOutputSize);
}

/******************************Public*Routine******************************\
* BOOL bSetModeAndWarmupHardware
*
* Sets the requested actual mode and initializes the hardware to a known
* state.
*
\**************************************************************************/

BOOL bSetModeAndWarmupHardware(
PDEV*   ppdev)
{
    BYTE*   pjBase;
    DWORD   ReturnedDataLength;
    ULONG   ulReturn;
    ULONG   ulAccess;
    HW_DATA HwData;

    pjBase = ppdev->pjBase;

    // Call the miniport via a public IOCTL to set the graphics mode.

    if (!DeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_CURRENT_MODE,
                         &ppdev->ulMode,            // Input
                         sizeof(DWORD),
                         NULL,                      // Output
                         0,
                         &ReturnedDataLength,
                         NULL))
    {
        DISPDBG((0, "bSetModeAndWarmupHardware - Failed VIDEO_SET_CURRENT_MODE"));
        goto ReturnFalse;
    }

    // Get the MGA's linear offset using a private IOCTL:

    if (!DeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MTX_QUERY_HW_DATA,
                         NULL,                              // Input
                         0,
                         &HwData,                           // Output
                         sizeof(HW_DATA),
                         &ReturnedDataLength,
                         NULL))
    {
        DISPDBG((0, "bEnableHardware -- failed MTX_QUERY_HW_DATA"));
        return(FALSE);
    }

    ppdev->ulYDstOrg = HwData.YDstOrg;

    ppdev->HopeFlags = 0;

    if (ppdev->iBitmapFormat == BMF_8BPP)
        ulAccess = 0;
    else if (ppdev->iBitmapFormat == BMF_16BPP)
        ulAccess = 1;
    else
        ulAccess = 2;

    CHECK_FIFO_SPACE(pjBase, 5);
    CP_WRITE(pjBase, DWG_MACCESS, ulAccess);
    CP_WRITE(pjBase, DWG_SHIFT,   0);
    CP_WRITE(pjBase, DWG_YDSTORG, ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_PLNWT,   ppdev->ulPlnWt);
    CP_WRITE(pjBase, DWG_PITCH,   ppdev->cxMemory);

    CP_WRITE_REGISTER(pjBase + HST_OPMODE,
        CP_READ_REGISTER(pjBase + HST_OPMODE) | 0x01000000);

    vResetClipping(ppdev);

    // At this point, the RAMDAC should be okay, but it looks
    // like it's not quite ready to accept data, particularly
    // on VL boards.  Adding a delay seems to fix things.

    Sleep(100);

    return(TRUE);

ReturnFalse:

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bAssertModeHardware
*
* Sets the appropriate hardware state when entering or leaving graphics
* mode or full-screen.
*
\**************************************************************************/

BOOL bAssertModeHardware(
PDEV* ppdev,
BOOL  bEnable)
{
    ULONG   ulNewFileSize;
    DWORD   ReturnedDataLength;
    ULONG   ulReturn;

    if (bEnable)
    {
        // The MGA miniport requires that the screen must be reenabled
        // and reinitialized to a clean state.  This should not be done
        // for more than one board when supporting multiple boards:

        if (IBOARD(ppdev) == 0)
        {
            // Re-enable the MGA's screen via a private IOCTL:

            if (!DeviceIoControl(ppdev->hDriver,
                                 IOCTL_VIDEO_MTX_INITIALIZE_MGA,
                                 NULL,
                                 0,
                                 &ulNewFileSize,
                                 sizeof(ULONG),
                                 &ReturnedDataLength,
                                 NULL))
            {
                DISPDBG((0, "bAssertModeHardware - Failed VIDEO_MTX_INITAILIZE_MGA"));
                goto ReturnFalse;
            }

            // The miniport should also build a new mode table, via a
            // private IOCTL:

            if (!DeviceIoControl(ppdev->hDriver,
                                 IOCTL_VIDEO_MTX_INIT_MODE_LIST,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &ReturnedDataLength,
                                 NULL))
            {
                DISPDBG((0, "bAssertModeHardware - Failed VIDEO_MTX_INIT_MODE_LIST"));
                goto ReturnFalse;
            }
        }

        if (!bSetModeAndWarmupHardware(ppdev))
        {
            DISPDBG((0, "bAssertModeHardware - Failed bSetModeAndWarmupHardware"));
            goto ReturnFalse;
        }
    }
    else
    {
        // Wait for all pending accelerator operations to finish:

        CHECK_FIFO_SPACE(ppdev->pjBase, FIFOSIZE);

        // Call the kernel driver to reset the device to a known state.
        // NTVDM will take things from there.  One reset will affect
        // all boards:

        if (IBOARD(ppdev) == 0)
        {
            if (!DeviceIoControl(ppdev->hDriver,
                                 IOCTL_VIDEO_RESET_DEVICE,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &ulReturn,
                                 NULL))
            {
                DISPDBG((0, "bAssertModeHardware - Failed reset IOCTL"));
                goto ReturnFalse;
            }
        }
    }

    DISPDBG((5, "Passed bAssertModeHardware"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bAssertModeHardware"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bEnableHardware
*
* Puts the hardware in the requested mode and initializes it.
*
* Note: Should be called before any access is done to the hardware from
*       the display driver.
*
\**************************************************************************/

BOOL bEnableHardware(
PDEV*   ppdev)
{
    VIDEO_PUBLIC_ACCESS_RANGES      VideoPublicAccessRanges;
    ULONG                           ReturnedDataLength;

    // Get the coprocessor address range using a public IOCTL:

    if (!DeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                         NULL,                              // Input
                         0,
                         (VOID*) &VideoPublicAccessRanges,  // Output
                         sizeof(VideoPublicAccessRanges),
                         &ReturnedDataLength,
                         NULL))
    {
        DISPDBG((0, "bEnableHardware -- failed QUERY_PUBLIC_ACESS_RANGES"));
        return(FALSE);
    }

    ppdev->pjBase = (BYTE*) VideoPublicAccessRanges.VirtualAddress;

    ///////////////////////////////////////////////////////////////////

    // Now we can set the mode, unlock the accelerator, and reset the
    // clipping:

    if (!bSetModeAndWarmupHardware(ppdev))
        goto ReturnFalse;

    DISPDBG((0, "Memory: %li x %li  YDstOrg: %li",
        ppdev->cxMemory, ppdev->cyMemory, ppdev->ulYDstOrg));

    DISPDBG((5, "Passed bEnableHardware"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bEnableHardware"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* VOID vDisableHardware
*
* Undoes anything done in bEnableHardware.
*
* Note: In an error case, we may call this before bEnableHardware is
*       completely done.
*
\**************************************************************************/

VOID vDisableHardware(
PDEV*   ppdev)
{
    VIDEO_MEMORY    VideoMemory;
    ULONG           ReturnedDataLength;

    VideoMemory.RequestedVirtualAddress = ppdev->pjBase;

    if (!DeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                         &VideoMemory,                  // Input
                         sizeof(VideoMemory),
                         NULL,                          // Output
                         0,
                         &ReturnedDataLength,
                         NULL))
    {
        DISPDBG((0, "vDisableHardware -- failed FREE_PUBLIC_ACCESS_RANGES"));
    }
}

/******************************Public*Routine******************************\
* BOOL bInitializeOffscreenFields
*
\**************************************************************************/

BOOL bInitializeOffscreenFields(
PDEV*                   ppdev,
VIDEO_MODE_INFORMATION* pVideoModeInformation)
{
    VIDEO_NUM_OFFSCREEN_BLOCKS  NumOffscreenBlocks;
    OFFSCREEN_BLOCK*            pOffscreenBlock;
    ULONG                       ReturnedDataLength;
    ULONG                       cjOffscreenBlock;
    LONG                        i;

    // Ask the MGA miniport about the number of offscreen areas available
    // for our selected mode, using a private IOCTL:

    if (!DeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MTX_QUERY_NUM_OFFSCREEN_BLOCKS,
                         pVideoModeInformation,             // Input
                         sizeof(VIDEO_MODE_INFORMATION),
                         &NumOffscreenBlocks,               // Output
                         sizeof(VIDEO_NUM_OFFSCREEN_BLOCKS),
                         &ReturnedDataLength,
                         NULL))
    {
        DISPDBG((0, "bInitializeOffscreenFields -- failed QUERY_NUM_OFFSCREEN_BLOCKS"));
        goto ReturnFalse;
    }

    cjOffscreenBlock = NumOffscreenBlocks.NumBlocks
                     * NumOffscreenBlocks.OffscreenBlockLength;
    pOffscreenBlock = (OFFSCREEN_BLOCK*) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                                    cjOffscreenBlock);
    if (pOffscreenBlock == NULL)
    {
        DISPDBG((0, "bInitializeOffscreenFields -- failed pOffscreenBlock LocalAlloc"));
        goto ReturnFalse;
    }

    // Ask the MGA miniport to fill in the available offscreen areas using
    // a private IOCTL:

    if (!DeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MTX_QUERY_OFFSCREEN_BLOCKS,
                         pVideoModeInformation,             // Input
                         sizeof(VIDEO_MODE_INFORMATION),
                         pOffscreenBlock,                   // Output
                         cjOffscreenBlock,
                         &ReturnedDataLength,
                         NULL))
    {
        DISPDBG((0, "bInitializeOffscreenFields -- failed QUERY_OFFSCREEN_BLOCKS"));
        LocalFree(pOffscreenBlock);
        goto ReturnFalse;
    }

    LocalFree(pOffscreenBlock);

    ppdev->cyMemory = ppdev->cyScreen;

    for (i = NumOffscreenBlocks.NumBlocks; i != 0; i--, pOffscreenBlock++)
    {
        DISPDBG((1, "Offscreen blocks:"));
        DISPDBG((1, "  (%li, %li) at (%li, %li) Type: %li Planes: %lx ZOffset: %li",
                    pOffscreenBlock->Width,  pOffscreenBlock->Height,
                    pOffscreenBlock->XStart, pOffscreenBlock->YStart,
                    pOffscreenBlock->Type,   pOffscreenBlock->SafePlanes,
                    pOffscreenBlock->ZOffset));

        // The miniport seems to be giving us garbage for some fields:

        if ((pOffscreenBlock->YStart == (ULONG) ppdev->cyScreen) &&
            (pOffscreenBlock->Width  == (ULONG) ppdev->cxScreen))
        {
            ppdev->cyMemory = ppdev->cyScreen + pOffscreenBlock->Height;
        }
    }

    // !!! The miniport should be changed to never reserve space for 'Z'
    //     or the back buffer -- we want to do that ourselves.  Right now,
    //     it does so for the only 3d enabled mode it thinks we can do,
    //     namely 5-5-5 on a 4MB Impression Plus:

    if ((ppdev->ulBoardId == MGA_PCI_4M) &&
        (ppdev->flGreen == 0x3e0))
    {
        // The total count of scans is the floor of 4MB divided by the
        // screen stride, less one to account for a possible ulYDstOrg that
        // we don't yet know:

        ppdev->cyMemory = (4096 * 1024) / (ppdev->cxMemory * 2);
    }

    return(TRUE);

ReturnFalse:

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bSelectMode
*
* Negotiates the video mode with the miniport.
*
\**************************************************************************/

BOOL bSelectMode(
HANDLE                  hDriver,
DEVMODEW*               pdm,                    // Requested mode
VIDEO_MODE_INFORMATION* pVideoModeInformation,  // Returns requested mode
ULONG*                  pulBoardId)             // Returns MGA board ID
{
    ULONG                           cModes;
    PVIDEO_MODE_INFORMATION         pVideoBuffer;
    PVIDEO_MODE_INFORMATION         pVideoModeSelected;
    PVIDEO_MODE_INFORMATION         pVideoTemp;
    BOOL                            bSelectDefault;
    VIDEO_MODE_INFORMATION          VideoModeInformation;
    VIDEO_PUBLIC_ACCESS_RANGES      VideoPublicAccessRanges;
    ULONG                           cbModeSize;
    DWORD                           ReturnedDataLength;
    ULONG                           ulBoardId;
    ULONG                           cDefaultBitsPerPel;

    if (!DeviceIoControl(hDriver,
                         IOCTL_VIDEO_MTX_QUERY_BOARD_ID,
                         NULL,                      // Input
                         0,
                         &ulBoardId,
                         sizeof(ULONG),
                         &ReturnedDataLength,
                         NULL))
    {
        DISPDBG((0, "bSelectMode -- failed MTX_QUERY_BOARD_ID"));
        goto ReturnFailure0;
    }

    // Use the driver's lowest pixel depth for the default mode:

    *pulBoardId = ulBoardId;

    if ((ulBoardId == MGA_PRO_4M5) || (ulBoardId == MGA_PRO_4M5_Z))
    {
        cDefaultBitsPerPel = 24;
    }
    else
    {
        cDefaultBitsPerPel = 8;
    }

    // Call the miniport to get mode information:

    cModes = getAvailableModes(hDriver, &pVideoBuffer, &cbModeSize);
    if (cModes == 0)
        goto ReturnFailure0;

    // Determine if we are looking for a default mode:

    if ( ((pdm->dmPelsWidth)    ||
          (pdm->dmPelsHeight)   ||
          (pdm->dmBitsPerPel)   ||
          (pdm->dmDisplayFlags) ||
          (pdm->dmDisplayFrequency)) == 0)
    {
        bSelectDefault = TRUE;
    }
    else
    {
        bSelectDefault = FALSE;
    }

    // Now see if the requested mode has a match in that table.

    pVideoModeSelected = NULL;
    pVideoTemp = pVideoBuffer;

    if ((pdm->dmPelsWidth        == 0) &&
        (pdm->dmPelsHeight       == 0) &&
        (pdm->dmBitsPerPel       == 0) &&
        (pdm->dmDisplayFrequency == 0))
    {
        DISPDBG((1, "Default mode requested"));
    }
    else
    {
        DISPDBG((1, "Requested mode..."));
        DISPDBG((1, "   Screen width  -- %li", pdm->dmPelsWidth));
        DISPDBG((1, "   Screen height -- %li", pdm->dmPelsHeight));
        DISPDBG((1, "   Bits per pel  -- %li", pdm->dmBitsPerPel));
        DISPDBG((1, "   Frequency     -- %li", pdm->dmDisplayFrequency));
    }

    while (cModes--)
    {
        if (pVideoTemp->Length != 0)
        {
            DISPDBG((2, "   Checking against miniport mode:"));
            DISPDBG((2, "      Screen width  -- %li", pVideoTemp->VisScreenWidth));
            DISPDBG((2, "      Screen height -- %li", pVideoTemp->VisScreenHeight));
            DISPDBG((2, "      Bits per pel  -- %li", pVideoTemp->BitsPerPlane *
                                                      pVideoTemp->NumberOfPlanes));
            DISPDBG((2, "      Frequency     -- %li", pVideoTemp->Frequency));

            if (((bSelectDefault) &&
                 (pVideoTemp->BitsPerPlane == cDefaultBitsPerPel)) ||
                ((pVideoTemp->VisScreenWidth  == pdm->dmPelsWidth) &&
                 (pVideoTemp->VisScreenHeight == pdm->dmPelsHeight) &&
                 (pVideoTemp->BitsPerPlane *
                  pVideoTemp->NumberOfPlanes  == pdm->dmBitsPerPel)) &&
                 (pVideoTemp->Frequency       == pdm->dmDisplayFrequency))
            {
                pVideoModeSelected = pVideoTemp;
                DISPDBG((1, "...Found a mode match!"));
                break;
            }
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + cbModeSize);

    }

    // If no mode has been found, return an error:

    if (pVideoModeSelected == NULL)
    {
        DISPDBG((1, "...Couldn't find a mode match!"));
        goto ReturnFailure1;
    }

    // We have chosen the one we want.  Save it in a stack buffer and
    // get rid of allocated memory before we forget to free it.

    *pVideoModeInformation = *pVideoModeSelected;
    LocalFree(pVideoBuffer);

    return(TRUE);

ReturnFailure1:

    LocalFree(pVideoBuffer);

ReturnFailure0:

    DISPDBG((0, "Failed bSelectMode"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bInitializeModeFields
*
* Initializes a bunch of fields in the pdev, devcaps (aka gdiinfo), and
* devinfo based on the requested mode.
*
\**************************************************************************/

BOOL bInitializeModeFields(
PDEV*     ppdev,
GDIINFO*  pgdi,
DEVINFO*  pdi,
DEVMODEW* pdm)
{
    ULONG                           cModes;
    PVIDEO_MODE_INFORMATION         pVideoBuffer;
    PVIDEO_MODE_INFORMATION         pVideoModeSelected;
    PVIDEO_MODE_INFORMATION         pVideoTemp;
    BOOL                            bSelectDefault;
    VIDEO_MODE_INFORMATION          VideoModeInformation;
    VIDEO_PUBLIC_ACCESS_RANGES      VideoPublicAccessRanges;
    ULONG                           cbModeSize;
    DWORD                           ReturnedDataLength;
    ULONG                           ulBoardId;
    ULONG                           cDefaultBitsPerPel;

    // Tell the miniport what mode we want:

    if (!bSelectMode(ppdev->hDriver,
                     pdm,
                     &VideoModeInformation,
                     &ppdev->ulBoardId))
    {
        DISPDBG((0, "bInitializeModeFields -- failed bSelectMode"));
        goto ReturnFalse;
    }

    ppdev->pjScreen     = NULL;     // The MGA driver has no easily
                                    //   accessible frame buffer.  We just
                                    //   keep this around for source code
                                    //   compatibility with our other drivers.

    ppdev->ulMode       = VideoModeInformation.ModeIndex;
    ppdev->cxScreen     = VideoModeInformation.VisScreenWidth;
    ppdev->cyScreen     = VideoModeInformation.VisScreenHeight;
    ppdev->cxMemory     = VideoModeInformation.VideoMemoryBitmapWidth;

    ppdev->flRed        = VideoModeInformation.RedMask;
    ppdev->flGreen      = VideoModeInformation.GreenMask;
    ppdev->flBlue       = VideoModeInformation.BlueMask;

    ppdev->flHooks      = (HOOK_BITBLT     |
                           HOOK_TEXTOUT    |
                           HOOK_FILLPATH   |
                           HOOK_COPYBITS   |
                           HOOK_STROKEPATH |
                           HOOK_PAINT      |
                           HOOK_STRETCHBLT);

    if (!bInitializeOffscreenFields(ppdev, &VideoModeInformation))
    {
        DISPDBG((0, "bInitializeModeFields -- failed bInitializeOffscreenFields"));
        goto ReturnFalse;
    }

#if DBG_3DDDI
    if ((VideoModeInformation.VisScreenWidth == 1024) &&
        (VideoModeInformation.BitsPerPlane > 16))
    {
        VideoModeInformation.VisScreenHeight = 256;
        ppdev->cyScreen = VideoModeInformation.VisScreenHeight;
    }
#endif

    // Fill in the GDIINFO data structure with the default 8bpp values:

    *pgdi = ggdiDefault;

    // Now overwrite the defaults with the relevant information returned
    // from the kernel driver:

    pgdi->ulHorzSize        = VideoModeInformation.XMillimeter;
    pgdi->ulVertSize        = VideoModeInformation.YMillimeter;

    pgdi->ulHorzRes         = VideoModeInformation.VisScreenWidth;
    pgdi->ulVertRes         = VideoModeInformation.VisScreenHeight;
    pgdi->ulDesktopHorzRes  = VideoModeInformation.VisScreenWidth;
    pgdi->ulDesktopVertRes  = VideoModeInformation.VisScreenHeight;

    pgdi->cBitsPixel        = VideoModeInformation.BitsPerPlane;
    pgdi->cPlanes           = VideoModeInformation.NumberOfPlanes;
    pgdi->ulVRefresh        = VideoModeInformation.Frequency;

    pgdi->ulDACRed          = VideoModeInformation.NumberRedBits;
    pgdi->ulDACGreen        = VideoModeInformation.NumberGreenBits;
    pgdi->ulDACBlue         = VideoModeInformation.NumberBlueBits;

    pgdi->ulLogPixelsX      = pdm->dmLogPixels;
    pgdi->ulLogPixelsY      = pdm->dmLogPixels;

    // Fill in the devinfo structure with the default 8bpp values:

    *pdi = gdevinfoDefault;

    if (VideoModeInformation.BitsPerPlane == 8)
    {
        ppdev->cjPelSize        = 1;
        ppdev->cjHwPel          = 1;
        ppdev->lDelta           = ppdev->cxMemory;
        ppdev->iBitmapFormat    = BMF_8BPP;
        ppdev->ulPlnWt          = plnwt_MASK_8BPP;
        ppdev->pfnFillPatNative = vFillPat8bpp;
        ppdev->cPaletteShift    = 8 - pgdi->ulDACRed;
    }
    else if ((VideoModeInformation.BitsPerPlane == 16) ||
             (VideoModeInformation.BitsPerPlane == 15))
    {
        ppdev->cjPelSize        = 2;
        ppdev->cjHwPel          = 2;
        ppdev->lDelta           = 2 * ppdev->cxMemory;
        ppdev->iBitmapFormat    = BMF_16BPP;
        pgdi->ulNumColors       = (ULONG) -1;
        pgdi->ulNumPalReg       = 0;
        pgdi->ulHTOutputFormat  = HT_FORMAT_16BPP;
        ppdev->ulPlnWt          = plnwt_MASK_16BPP;
        ppdev->pfnFillPatNative = vFillPat16bpp;

        pdi->iDitherFormat      = BMF_16BPP;
        pdi->flGraphicsCaps    &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }
    else
    {
        ASSERTDD((VideoModeInformation.BitsPerPlane == 32) ||
                 (VideoModeInformation.BitsPerPlane == 24),
                 "This driver supports only 8, 16, 24, and 32bpp");

        // The miniport may think it's 32bpp, but we're going to tell GDI
        // that it's 24bpp.  We do this so that out bitmap transfers will
        // be more efficient, and compatible bitmaps will be smaller.
        //
        // Note that we also have to fudge the results returned from
        // 'GetModes' if we're going to do this.

        pgdi->cBitsPixel        = 24;

        ppdev->cjPelSize        = 3;
        ppdev->cjHwPel          = 4;
        ppdev->lDelta           = 4 * ppdev->cxMemory;
        ppdev->iBitmapFormat    = BMF_24BPP;
        pgdi->ulNumColors       = (ULONG) -1;
        pgdi->ulNumPalReg       = 0;
        pgdi->ulHTOutputFormat  = HT_FORMAT_24BPP;
        ppdev->ulPlnWt          = plnwt_MASK_24BPP;
        ppdev->pfnFillPatNative = vFillPat24bpp;

        pdi->iDitherFormat      = BMF_24BPP;
        pdi->flGraphicsCaps    &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }

    DISPDBG((5, "Passed bInitializeModeFields"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bInitializeModeFields"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* DWORD getAvailableModes
*
* Calls the miniport to get the list of modes supported by the kernel driver,
* and returns the list of modes supported by the diplay driver among those
*
* returns the number of entries in the videomode buffer.
* 0 means no modes are supported by the miniport or that an error occured.
*
* NOTE: the buffer must be freed up by the caller.
*
\**************************************************************************/

DWORD getAvailableModes(
HANDLE                   hDriver,
PVIDEO_MODE_INFORMATION* modeInformation,       // Must be freed by caller
DWORD*                   cbModeSize)
{
    ULONG                   ulTemp;
    VIDEO_NUM_MODES         modes;
    PVIDEO_MODE_INFORMATION pVideoTemp;

    //
    // Get the number of modes supported by the mini-port
    //

    if (!DeviceIoControl(hDriver,
            IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
            NULL,
            0,
            &modes,
            sizeof(VIDEO_NUM_MODES),
            &ulTemp,
            NULL))
    {
        DISPDBG((0, "getAvailableModes - Failed VIDEO_QUERY_NUM_AVAIL_MODES"));
        return(0);
    }

    *cbModeSize = modes.ModeInformationLength;

    //
    // Allocate the buffer for the mini-port to write the modes in.
    //

    *modeInformation = (PVIDEO_MODE_INFORMATION)
                        LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                   modes.NumModes *
                                   modes.ModeInformationLength);

    if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
    {
        DISPDBG((0, "getAvailableModes - Failed LocalAlloc"));
        return 0;
    }

    //
    // Ask the mini-port to fill in the available modes.
    //

    if (!DeviceIoControl(hDriver,
            IOCTL_VIDEO_QUERY_AVAIL_MODES,
            NULL,
            0,
            *modeInformation,
            modes.NumModes * modes.ModeInformationLength,
            &ulTemp,
            NULL))
    {

        DISPDBG((0, "getAvailableModes - Failed VIDEO_QUERY_AVAIL_MODES"));

        LocalFree(*modeInformation);
        *modeInformation = (PVIDEO_MODE_INFORMATION) NULL;

        return(0);
    }

    //
    // Now see which of these modes are supported by the display driver.
    // As an internal mechanism, set the length to 0 for the modes we
    // DO NOT support.
    //

    ulTemp = modes.NumModes;
    pVideoTemp = *modeInformation;

    //
    // Mode is rejected if it is not one plane, or not graphics, or is not
    // one of 8, 15, 16 or 32 bits per pel.
    //

    while (ulTemp--)
    {
        if ((pVideoTemp->NumberOfPlanes != 1 ) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
            ((pVideoTemp->BitsPerPlane != 8) &&
             (pVideoTemp->BitsPerPlane != 15) &&
             (pVideoTemp->BitsPerPlane != 16) &&
             (pVideoTemp->BitsPerPlane != 32)))
        {
            DISPDBG((2, "Rejecting miniport mode:"));
            DISPDBG((2, "   Screen width  -- %li", pVideoTemp->VisScreenWidth));
            DISPDBG((2, "   Screen height -- %li", pVideoTemp->VisScreenHeight));
            DISPDBG((2, "   Bits per pel  -- %li", pVideoTemp->BitsPerPlane *
                                                   pVideoTemp->NumberOfPlanes));
            DISPDBG((2, "   Frequency     -- %li", pVideoTemp->Frequency));

            pVideoTemp->Length = 0;
        }

        if (pVideoTemp->BitsPerPlane == 32)
        {
            // The MGA is a little weird.  The miniport reports back 32bpp
            // modes, but the display driver tells GDI that it's 24bpp.
            // We could change the miniport to report 24bpp, but it's just
            // as easy to do it here:

            pVideoTemp->BitsPerPlane = 24;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return(modes.NumModes);
}
