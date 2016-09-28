/******************************Module*Header***********************************\
 *                                                                            *
 *                           ***************************************          *
 *                           * Permedia 3: Direct Draw SAMPLE CODE *          *
 *                           ***************************************          *
 *                                                                            *
 * Module Name: gldd.c                                                        *
 *                                                                            *
 * This module contains the Direct Draw Initialization code for the           *
 * 16-bit portion and one Direct Draw callback DestroyDriver16. It also       *
 * contains code to switch back to the GDI context.                           *
 *                                                                            *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.              *
 * Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.       *
 *                                                                            *
\******************************************************************************/


#define WIN95_THUNKING

#include "glint.h"
#pragma warning(disable:4005)
#include <ddrawi.h>
#pragma warning(default:4005)
#include "glddif.h"
#include "linalloc.h"
#include "glddtk.h"
#include "ramdac.h"
#include "p3string.h"
#include "debug.h"


//
// Global GDI context variables
//

extern DWORD PASCAL     dwFBReadMode;
extern DWORD PASCAL     dwFBWriteMode;
extern DWORD PASCAL     dwRasterizerMode;


//
// External Functions Prototypes
//

extern WORD FAR PASCAL 
DIB_Control(
    LPVOID  lpDevice, 
    UINT    fn, 
    LPVOID  pin,
    LPVOID  pout);

extern VOID _cdecl 
DisableOffscreenHeap();

extern BOOL _cdecl 
EnableOffscreenHeap(
    WORD    cxScreen, 
    LONG    cyScreen);


//
// External Variables
//

extern LPGLINTINFO PASCAL   pGLInfo;    // Glint Information from main driver


//
// Prototypes of functions defined here
//

static DWORD __loadds FAR PASCAL 
DestroyDriver16(
    LPDDHAL_DESTROYDRIVERDATA   pddd);

VOID _cdecl 
RestoreGDIContext();

static void 
SetRGBABitMasks(
    LPDDHALMODEINFO     pThisMode);

static void 
BuildPixelFormat(
    LPDDHALMODEINFO     pMode, 
    LPDDPIXELFORMAT     pdpf);

static void 
FillHalModeInfo(
    LPDDHALMODEINFO     pThisMode,
    DWORD               Width, 
    DWORD               Height, 
    DWORD               Bpp,
    WORD                MaxRefreshRate);

static int  
BuildHALModes();

static LPDDHALMODEINFO 
SetupSharedData();


/******************************************************************************\
 *                                                                            *
 * Global variables                                                           *
 *                                                                            *
\******************************************************************************/

P3_THUNKEDDATA          DriverData;     // Global driver data
// P3_THUNKEDDATA         *pDriverData;    // Global driver data
static DWORD            dwDriverDataInitialized = 0;
static VIDMEM           VidMem[2];


/******************************************************************************\
 *                                                                            *
 * a sample of video memory usage                                             *
 *                                                                            *
 * - the first chunk can be used for anything but overlays and textures       *
 *   (offscreen memory only)                                                  *
 * - the second chunk can be used for anything but plain offscreen surfaces   *
 *   or texture surfaces (overlay memory only)                                *
 * - the third chunk can be used for anything but plain offscreen surfaces    *
 *   or overlay surfaces (texture memory only)                                *
 *                                                                            *
 * if the allocations fails, note that any chunk can be used to allocatedd    *
 * any piece of memory (that is way the caps bits are ZERO in the             *
 * alternate caps field)                                                      *
 *                                                                            *
\******************************************************************************/
                                                                             

/******************************************************************************\
 *                                                                            *
 * Misc Definitions                                                           *
 *                                                                            *
\******************************************************************************/

#define QUERYESCSUPPORT     8   // defined because we can't include GDI defns 
                                // and GDIDEFS.INC

static int                  NumHALModes = 0;
static DDHALMODEINFO        ddmodeInfo[50] = {0};

#define FOURCC_YUV422       0x32595559

static DWORD fourCC[] =
{
    FOURCC_YUV422,
};

/******************************************************************************\
 *                                                                            *
 * Callback arrays                                                            *
 *                                                                            *
\******************************************************************************/

static DDHAL_DDCALLBACKS cbDDCallbacks =
{
    sizeof(DDHAL_DDCALLBACKS),
    0,                  // dwFlags
    0,                  // DestroyDriver
    NULL,               // CreateSurface32
    NULL,               // SetColorKey
    NULL,               // SetMode
    NULL,               // WaitForVerticalBlank
    NULL,               // CanCreateSurface32
    NULL,               // CreatePalette
    NULL,               // GetScanLine
    NULL,               // SetExclusiveMode
    NULL                // FlipToGDISurface
};

static DDHAL_DDSURFACECALLBACKS cbDDSurfaceCallbacks =
{
    sizeof(DDHAL_DDSURFACECALLBACKS),
    0,                  // dwFlags
    NULL,               // DestroySurface (32 Bit)
    NULL,               // Flip (32 Bit)
    NULL,               // SetClipList
    NULL,               // Lock (32 Bit)
    NULL,               // Unlock
    NULL,               // Blt (32 Bit)
    NULL,               // SetColorKey (32 Bit)
    NULL,               // AddAttachedSurface
    NULL,               // GetBltStatus (32 Bit)
    NULL,               // GetFlipStatus (32 Bit)
    NULL,               // UpdateOverlay
    NULL,               // SetOverlayPosition
    NULL,               // reserved4
    NULL                // SetPalette
};

static DDHAL_DDPALETTECALLBACKS cbDDPaletteCallbacks =
{
    sizeof (DDHAL_DDPALETTECALLBACKS),
    0,
    NULL,
    NULL
};


//
// Permedia 3 device dependant definitions
//

#define LOAD_P3_REG(Register, Value) \
    VideoPortWriteRegisterUlong((ULONG FAR *)&(((PREGISTERS)(pGLInfo->pRegs))->Glint.Register), Value)

#define READ_P3_REG(Register)       \
    VideoPortReadRegisterUlong((ULONG FAR *)&(((PREGISTERS)(pGLInfo->pRegs))->Glint.Register)) 

#define WAIT_FOR_INPUT_FIFO_ENTRIES(n) \
        while (READ_P3_REG(InFIFOSpace) < n);

/******************************************************************************\
 *                                                                            *
 * Function:    vWaitDMAComplete                                              *
 * Description: Waits till the DMA engine is idle                             *
 *                                                                            *
\******************************************************************************/

static VOID 
vWaitDMAComplete()
{
    ULONG count;

    if (! (pGLInfo->GlintBoardStatus & GLINT_DMA_COMPLETE)) {
        
        if (pGLInfo->GlintBoardStatus & GLINT_INTR_CONTEXT) { 
            
            //
            //  do any VBLANK wait, wait Q to empty and last DMA to complete
            //

            while (pGLInfo->frontIndex != pGLInfo->backIndex); 
        }

        while ((count = READ_P3_REG(DMACount)) > 0) {
            
            if (count < 32) {
                count = 1;
            } else {
                count <<= 1;                    
            }

            while (--count > 0);            
        }

        pGLInfo->GlintBoardStatus |= GLINT_DMA_COMPLETE;    
    }
}


/******************************************************************************\
 *                                                                            *
 * Function:    vSyncWithPermedia                                             *
 * Description: Waits till the DMA is idle and makes sure that the Input FIFO *
 *              is completely flushed out.                                    *
 *                                                                            *
\******************************************************************************/

static VOID
vSyncWithPermedia()
{
    vWaitDMAComplete();
    
    WAIT_FOR_INPUT_FIFO_ENTRIES(3);
    
    LOAD_P3_REG(FilterMode, 0x400);
    LOAD_P3_REG(Sync, 0x000);
    LOAD_P3_REG(FilterMode, 0x000);

    do {
        
        while (READ_P3_REG(OutFIFOWords) == 0);
    } while (READ_P3_REG(GPFifo[0]) != 0x188);
}


static void 
SetRGBABitMasks(
    LPDDHALMODEINFO     pThisMode)
{        
    switch (pThisMode->dwBPP) {
    
        case 8:
            pThisMode->dwRBitMask = 0;
            pThisMode->dwGBitMask = 0;
            pThisMode->dwBBitMask = 0;
            pThisMode->dwAlphaBitMask = 0;
            break;
    
        case 16:
            pThisMode->dwRBitMask = 0x0000F800L;
            pThisMode->dwGBitMask = 0x000007E0L;
            pThisMode->dwBBitMask = 0x0000001FL;
            // pThisMode->dwAlphaBitMask = 0x8000L;
            pThisMode->dwAlphaBitMask = 0x0000L;
            break;
    
        case 32:
            pThisMode->dwRBitMask = 0x00ff0000L;
            pThisMode->dwGBitMask = 0x0000ff00L;
            pThisMode->dwBBitMask = 0x000000ffL;
            // pThisMode->dwAlphaBitMask = 0xff000000L;
            pThisMode->dwAlphaBitMask = 0x00000000L;
            break;
    }
}

/******************************************************************************\
 *                                                                            *
 * Function:    BuildPixelFormat                                              *
 * Description: Generate a pixel format structure based on the mode           *
 *              This example works only with RGB surfaces                     *
 *                                                                            *
\******************************************************************************/

static void 
BuildPixelFormat(
    LPDDHALMODEINFO     pMode,
    LPDDPIXELFORMAT     pdpf)
{
    pdpf->dwSize        = sizeof(DDPIXELFORMAT);
    pdpf->dwFourCC      = 0;
    pdpf->dwFlags       = DDPF_RGB;
    pdpf->dwRGBBitCount = pMode->dwBPP;

    if (pMode->dwBPP == 8 ) {
        pdpf->dwFlags |= DDPF_PALETTEINDEXED8;
    }

    pdpf->dwRBitMask        = pMode->dwRBitMask;
    pdpf->dwGBitMask        = pMode->dwGBitMask;
    pdpf->dwBBitMask        = pMode->dwBBitMask;
    pdpf->dwRGBAlphaBitMask = pMode->dwAlphaBitMask;

} // End of BuildPixelFormat()


/******************************************************************************\
 *                                                                            *
 * Function:    FillHalModeInfo                                               *
 * Description: Fills an entry in the DDHALMODEINFO table                     *
 *                                                                            *
\******************************************************************************/

static void 
FillHalModeInfo(
    LPDDHALMODEINFO     pThisMode,
    DWORD               Width, 
    DWORD               Height, 
    DWORD               Bpp,
    WORD                MaxRefreshRate)
{
#if STEREO_MODE
    DWORD               dwTotalScanLines;
#endif

    pThisMode->dwWidth        = Width;              
    pThisMode->dwHeight       = Height;                 
    pThisMode->lPitch         = Width * (Bpp / 8);          
    pThisMode->dwBPP          = Bpp;
    if (Bpp == 8) {
        pThisMode->wFlags     = DDMODEINFO_PALETTIZED;
    }
    pThisMode->wRefreshRate   = MaxRefreshRate;
    pThisMode->wFlags         |= DDMODEINFO_MAXREFRESH;

    SetRGBABitMasks(pThisMode);

#if STEREO_MODE
    //
    // Stereo mode is not supported on P3 right now
    //
    if ((pThisMode->dwWidth >= 320) && (pThisMode->dwHeight >= 240)) {

        dwTotalScanLines = pGLInfo->ddFBSize/pThisMode->lPitch;

        if (dwTotalScanLines > (pThisMode->dwHeight*4)) {
            pThisMode->wFlags   |= DDMODEINFO_STEREO;           
        }
    }
#endif
}


/******************************************************************************\
 *                                                                            *
 * Function:    AllocD3DDMABuffer                                             *
 * Description: Find one DMA buufer fro the D3D driver                        *
 *                                                                            *
\******************************************************************************/

BOOL
AllocD3DDMABuffer(
    LPGLINTINFO         pGLInfo)
{
    unsigned short      i;

    //
    // If DMA for D3D has already been allocated
    //

    if (pGLInfo->dw3DDMABufferSize) {
        return (TRUE);
    }

    //
    // Look for a free DMA buffer allocated by the MVD
    //

    for (i = 0; i < pGLInfo->wDMABufferCount; i++) {

        if (! (pGLInfo->wDMABufferAlloc & (1 << i))) {

            pGLInfo->dw3DDMABufferSize = pGLInfo->dwDMABufferSize;
            pGLInfo->dw3DDMABufferPhys = pGLInfo->dwDMABufferPhys[i];
            pGLInfo->dw3DDMABufferVirt = pGLInfo->dwDMABufferVirt[i];

            //
            // Mark it as used
            //

            pGLInfo->wDMABufferAlloc  |= (1 << i);

            return (TRUE);
        }
    }

    //
    // No DMA buffer is available for the D3D driver
    //

    return (FALSE);
}



/******************************************************************************\
 *                                                                            *
 * Function:    FindDisplayMode                                               *
 * Description: Find the HAL display mode matching the pGLInfo                *
 *                                                                            *
\******************************************************************************/

static LPDDHALMODEINFO 
FindDisplayMode()
{
    LPDDHALMODEINFO         pMode = NULL;
    int                     i;

    //
    // Initialize the modes, and fill in the pixel format of the display
    //

    for (pMode = ddmodeInfo, i = 0; i < NumHALModes; i++, pMode++) {

        if ((pMode->dwHeight == (DWORD)pGLInfo->dwScreenHeight) &&
            (pMode->dwWidth  == (DWORD)pGLInfo->dwScreenWidth) &&
            (pMode->dwBPP    == (DWORD)pGLInfo->dwBpp)) {
            
            return (pMode);
        }
    }

    //
    // No matching mode is found
    //

    return (NULL);
}


/******************************************************************************\
 *                                                                            *
 * Function:    BuildHALModes                                                 *
 * Description: Generates the DDHALMODEINFO table                             *
 *                                                                            *
\******************************************************************************/

static int 
BuildHALModes()
{
    int     numModes = 0;

    //
    // Clear out all the modes.
    //

    memset(&ddmodeInfo[0], 0, (sizeof(ddmodeInfo) / sizeof(ddmodeInfo[0])));

    // FillHalModeInfo(&ddmodeInfo[numModes],  320,  200,  8, 100); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes],  320,  200, 16, 100); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes],  320,  200, 32, 100); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes],  320,  240,  8, 100); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes],  320,  240, 16, 100); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes],  320,  240, 32, 100); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes], 512, 384,  8, 120); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes], 512, 384, 16, 120); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes], 512, 384, 32, 120); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes],  640,  400,  8, 100); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes],  640,  400, 16, 100); numModes++;
    // FillHalModeInfo(&ddmodeInfo[numModes],  640,  400, 32, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes],  640,  480,  8, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes],  640,  480, 16, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes],  640,  480, 32, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes],  800,  600,  8, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes],  800,  600, 16, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes],  800,  600, 32, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1024,  768,  8, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1024,  768, 16, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1024,  768, 32, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1152,  864,  8,  85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1152,  864, 16,  85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1152,  864, 32,  85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1280, 1024,  8,  85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1280, 1024, 16,  85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1280, 1024, 32,  85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1600, 1200,  8,  85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1600, 1200, 16,  85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1600, 1200, 32,  85); numModes++;

    return (numModes);
}   // End of BuildHALModes()


/******************************************************************************\
 *                                                                            *
 * Function:    SetupSharedData                                               *
 * Description: Called to fill in DriverData with information, ready for when *
 *              DriverInit gets called on the 32 bit side.  This data is      *
 *              required DriverInit gets called on the 32 bit side. This data *
 *              required so that the 32 bit side can properly initialise.     *
 *              This function is called again when there is a mode change,    *
 *              as most of the things it fills in will change at this point.  *
 *                                                                            *
\******************************************************************************/

static LPDDHALMODEINFO 
SetupSharedData()
{
    LPDDHALMODEINFO         pMode = NULL;
    LinearAllocatorInfo    *pLAInfo;
    DWORD                   dwChipConfig;

    //
    // Find the current display mode
    //

    if (! (pMode = FindDisplayMode())) {
        return (NULL);
    }

    //
    // Allocate a DMA buffer for the D3D driver use
    //

    if (! AllocD3DDMABuffer(pGLInfo)) {
        return (NULL);
    }

    //
    // Control points to the RAMDAC
    // lpGLInfo is a pointer to the DisplayDriver state.
    //

    DriverData.control = (FLATPTR) pGLInfo->dwpRegisters;

    DriverData.pGLInfo = (LPGLINTINFO)(((DWORD)pGLInfo->dwDSBase) +
                         ((DWORD)pGLInfo & 0xffff));

    //
    // Decicde whether the card support AGP or not
    //

    dwChipConfig = ((PREGISTERS)pGLInfo->pRegs)->Glint.ChipConfig; 
    if ((dwChipConfig & PM_CHIPCONFIG_AGP1XCAPABLE) ||
        (dwChipConfig & PM_CHIPCONFIG_AGP2XCAPABLE) ||
        (dwChipConfig & PM_CHIPCONFIG_AGP4XCAPABLE)) {

        DriverData.bCanAGP = TRUE;
    } else {
        DriverData.bCanAGP = FALSE;
    }

    //
    // Pass the DevNode to 32bit part so that it callback the MVD
    //

    DriverData.dwDevNode        = pGLInfo->dwDeviceHandle;

    //
    // Video memory address
    //

    DriverData.dwLocalBuffer    = 
    DriverData.dwScreenFlatAddr = (FLATPTR)pGLInfo->dwpFrameBuffer;
    // DriverData.dwScreenStart    = pGLInfo->dwOffscreenBase;

    //
    // Build the pixel format based on the display mode
    //

    BuildPixelFormat(pMode, &DriverData.ddpfDisplay);

    // if (pMode->wFlags & DDMODEINFO_STEREO) {
    //     DriverData.bCanDoStereo = TRUE;
    // } else {
    //     DriverData.bCanDoStereo = FALSE;
    // }

    //
    // Setup the display size information
    // dwScreenWidth, dwScreenHeight = current resolution
    // cxMemory = Pixels across for one scanline 
    //     (not necessarily the same as the screen width)
    // cyMemory = Scanline height of the memory
    // dwScreenStart = First visible line of display
    //

    DriverData.dwScreenWidth  = pGLInfo->dwScreenWidth;
    DriverData.dwScreenHeight = pGLInfo->dwScreenHeight;

    DriverData.cxMemory = pGLInfo->dwScreenWidth;
    DriverData.cyMemory = pGLInfo->ddFBSize / 
                          (DriverData.dwScreenWidth << 
                              (DriverData.ddpfDisplay.dwRGBBitCount >> 4));

    DriverData.dwScreenStart = DriverData.dwScreenFlatAddr + 
                               pGLInfo->dwScreenBase;

    //
    // Usefull constants used during blits.
    // 2,1,0 for 32,16,8 depth.  Shifts needed to calculate bytes/pixel
    //

    DriverData.bPixShift     = (BYTE)DriverData.ddpfDisplay.dwRGBBitCount >> 4; 
    
    //
    // 0,1,2 for 32/16/8.
    //

    DriverData.bBppShift     = 2 - DriverData.bPixShift;
    
    //
    // 3,1,0 for 8,16,32 bpp
    //

    DriverData.dwBppMask     = 3 >> DriverData.bPixShift;
    
    //
    // Set up the Heap Manager data structure
    //

    pLAInfo = &DriverData.LocalVideoHeap0Info;

    //
    // Set up overall heap start, end information
    //

    pLAInfo->dwMemStart = (FLATPTR)pGLInfo->dwpFrameBuffer +
                          pGLInfo->dwOffscreenBase;
    pLAInfo->dwMemEnd   = (FLATPTR)pGLInfo->dwpFrameBuffer +
                          pGLInfo->ddFBSize - 1;
    
    //
    // (Re)initialize the heap manager on the 16 bit side
    //

    _DX_LIN_InitializeHeapManager(pLAInfo);
    
    //
    // Round up the start pointer and round down the end pointer
    //

    pLAInfo->dwMemStart = (pLAInfo->dwMemStart + 3) & ~3;
    pLAInfo->dwMemEnd   = (pLAInfo->dwMemEnd + 3) & ~3;
    
    DPF("Heap Attributes:");
    DPF(
        "  Primary: %d (pitch: %d) x %ld", 
        wScreenWidth, pMode->lPitch, (DWORD)pGLInfo->dwScreenHeight);
    DPF("  Start of Heap Memory: 0x%lx", pLAInfo->dwMemStart);
    DPF("  End of Heap Memory: 0x%lx", pLAInfo->dwMemEnd);

    DPF("Allocating Off screen heap");

    return (pMode);
}


/******************************************************************************\
 *                                                                            *
 * Function:    SetDriverInfo                                                 *
 * Description: Called at escape time, or at mode change time.                *
 *              Prepares the HAL data and calls directdraw with it.           *
 *                                                                            *
\******************************************************************************/

BOOL 
SetDriverInfo(
    BOOL                    bReset)
{
    DDHALINFO               ddhi;
    LPDDHALMODEINFO         pMode;
    BOOL                    bSuccess = FALSE;
    LPVIDMEM                pVm;
 
    //
    // This is the first create call
    // disable the 2D Rectangular Offscreen heap 
    //

    if (! bReset) {  
        
        DisableOffscreenHeap();
    }

    //
    // Poor man's exception handling
    //

    do {

        //
        // bReset == TRUE if there has been a mode change.
        //

        DriverData.bResetMode = bReset;

        //
        // Setup the information that is shared with the 32 bit side
        //

        pMode = SetupSharedData();
        if (! pMode) {
            break;
        }

        pGLInfo->DisabledByGLDD |= D3D_DISABLED;

        //
        // Copy the HAL callbacks
        //

        cbDDCallbacks = DriverData.DDHALCallbacks;

        //
        // Remember to fill in the only 16 bit callback - DestroyDriver16
        //

        cbDDCallbacks.dwFlags       &= ~DDHAL_CB32_DESTROYDRIVER;
        cbDDCallbacks.DestroyDriver  = DestroyDriver16;

        //
        // Copy the Surface callbacks
        //

        cbDDSurfaceCallbacks = DriverData.DDSurfCallbacks;

        //
        // Initialize the DDHALINFO struct
        //

        memset(&ddhi, 0, sizeof(ddhi));

        //
        // Copy the HAL info data that was filled in the 32-Bit side
        //

        ddhi           = DriverData.ddhi32; 

        //
        // Set up DDHALINFO structure
        //

        ddhi.dwSize    = sizeof(ddhi);
        ddhi.lpPDevice = (void *)pGLInfo->lpDriverPDevice;

        //
        // (!!!) HINSTANCE is 16bit on Win16 and 32bit in Win32
        //

        ddhi.hInstance = DriverData.hInstance;

        DPF("hInstance: 0x%lx", (DWORD)ddhi.hInstance);
        DPF("lpPDevice: 0x%lx", (DWORD)ddhi.lpPDevice);

        //
        // Describe the current primary surface
        // (ptr, dimensions, mode)
        //

        ddhi.vmiData.fpPrimary       = (FLATPTR)pGLInfo->dwpFrameBuffer + 
                                       pGLInfo->dwScreenBase;

        ddhi.vmiData.dwDisplayHeight = (DWORD)pGLInfo->dwScreenHeight;
        ddhi.vmiData.dwDisplayWidth  = (DWORD)pGLInfo->dwScreenWidth;
        ddhi.vmiData.lDisplayPitch   = (DWORD)pGLInfo->dwScreenWidthBytes;

        //
        // Mode information
        //

        ddhi.dwModeIndex             = pMode - ddmodeInfo;
        ddhi.dwNumModes              = NumHALModes;
        ddhi.lpModeInfo              = ddmodeInfo;
        ddhi.vmiData.ddpfDisplay     = DriverData.ddpfDisplay;

        //
        // Setup the YUV modes for Video playback acceleration.
        //

        ddhi.ddCaps.dwNumFourCCCodes = sizeof( fourCC ) / sizeof( fourCC[0] );
        ddhi.lpdwFourCC              = fourCC;

        //
        // (1) P3's local video memory is managed by the driver
        // (2) Only P3's AGP heap is managed by the DD runtime
        //

        if (DriverData.bCanAGP) {

            pVm = &VidMem[0];
            
            DPF("Initialized AGP Heap for P3");

            pVm->dwFlags           = VIDMEM_ISNONLOCAL |
                                     VIDMEM_ISLINEAR   |
                                     VIDMEM_ISWC;

            pVm->ddsCaps.dwCaps    = DDSCAPS_OVERLAY        |
                                     DDSCAPS_OFFSCREENPLAIN |
                                     DDSCAPS_FRONTBUFFER    |
                                     DDSCAPS_BACKBUFFER     |
                                     DDSCAPS_ZBUFFER        |
                                     DDSCAPS_3DDEVICE;

            pVm->ddsCapsAlt.dwCaps = DDSCAPS_OVERLAY        |
                                     DDSCAPS_OFFSCREENPLAIN |
                                     DDSCAPS_FRONTBUFFER    |
                                     DDSCAPS_BACKBUFFER     |
                                     DDSCAPS_ZBUFFER        |
                                     DDSCAPS_3DDEVICE;

            pVm->fpStart           = 0;

            //
            // 32MB of AGP memory, mem will be committed when used for surfaces
            //
    
            pVm->fpEnd             = (DWORD)32*(DWORD)1024*(DWORD)1024;   

            //
            // fpEnd is regarded by runtime as inclusive
            //

            pVm->fpEnd            -= 1;   

            ++pVm;
        
            //
            // Report AGP heap back to the DD runtime
            //

            ddhi.vmiData.dwNumHeaps = 1;
            ddhi.vmiData.pvmList    = &VidMem[0];

        } else {

            //
            // Report no heap back to the DD runtime
            //

            ddhi.vmiData.dwNumHeaps = 0;
            ddhi.vmiData.pvmList    = NULL;
        }

        //
        // Required alignments of the scanlines of each kind of memory 
        //

        ddhi.vmiData.dwOffscreenAlign = sizeof(DWORD);
        ddhi.vmiData.dwOverlayAlign   = sizeof(DWORD);
        ddhi.vmiData.dwTextureAlign   = sizeof(DWORD);
        ddhi.vmiData.dwAlphaAlign     = sizeof(DWORD);

        //
        // Z buffer needs to be aligned to 32 words at least, as that is 
        // the minimum partial product that glint supports.
        //

        ddhi.vmiData.dwZBufferAlign   = sizeof(DWORD) << 5;

        //
        // Callback functions
        //

        ddhi.lpDDCallbacks            = &cbDDCallbacks;
        ddhi.lpDDSurfaceCallbacks     = &cbDDSurfaceCallbacks;
        ddhi.lpDDPaletteCallbacks     = &cbDDPaletteCallbacks;

        //
        // Call DirectDraw to create the driver object
        //

        bSuccess = ((LPDDHAL_SETINFO)(pGLInfo->lpDDHAL_SetInfo))(&ddhi, bReset);

    } while (FALSE);

    if (! bSuccess) {

        //
        // Fail initialisation in this case.
        //

        if (pGLInfo->DisabledByGLDD) {
            pGLInfo->DisabledByGLDD &= ~D3D_DISABLED;
        }

        //
        // Enable the 2D Recangular offscreen heap manager
        //

        EnableOffscreenHeap(
            (WORD)pGLInfo->dwScreenWidth, 
            pGLInfo->dwScreenHeight);
    }

    return (bSuccess);

} // End of SetDriverInfo()


/******************************************************************************\
 *                                                                            *
 * Function:    DestroyDriver16                                               *
 * Description: Direct Draw HAL Callback function.                            *
 *                                                                            *
\******************************************************************************/

static DWORD __loadds FAR PASCAL 
DestroyDriver16(
    LPDDHAL_DESTROYDRIVERDATA   pddd)
{
    //
    // Reset the pointer to the DDRAW callback
    //

    ((LPDDHAL_SETINFO)(pGLInfo->lpDDHAL_SetInfo)) = NULL;

    pddd->ddRVal = DD_OK;

    if (pGLInfo->DisabledByGLDD)
        pGLInfo->DisabledByGLDD &= ~D3D_DISABLED;

    if (dwDriverDataInitialized) {

        vSyncWithPermedia();
        RestoreGDIContext();
    }

    //
    // This flag says to the HAL that we have not setup the Global 32 bit
    // Info for this display card.  It uses it to setup DMA, chip specific data,
    // etc.
    //

    DriverData.dwSetupThisDisplay = FALSE;

    //
    // Enable the 2D Recangular offscreen heap manager
    //

    EnableOffscreenHeap(
        (WORD)pGLInfo->dwScreenWidth, 
        pGLInfo->dwScreenHeight);

    //
    // Reset the driver version to 0 so it will be filled in again.
    //

    DriverData.dwDXVersion = 0;

    return (DDHAL_DRIVER_HANDLED);

} // End of DestroyDriver16()


/******************************************************************************\
 *                                                                            *
 * Function:    Control1                                                      *
 * Description: Called whenever the Escape function is invoked                *
 * Sequence:    (1) DDraw uses QUERYESCSUPPORT to ask whether DCICOMMAND is   *
 *                  supported by the driver                                   *
 *              (2) DDraw uses DDNEWCALLBACKFNS to notify helper functions,   *
 *                  normally only DDHAL_SetInfo (in DDraw16.dll) is used      *
 *              (3) DDraw uses DDVERSIONINFO to pass down its version         *
 *              (4) DDraw uses DDGET32BITDRIVERNAME to get name of the 32bit  *
 *                  driver DLL (which will be loaded into the SHARED arena    *
 *              (5) DDraw uses DDCREATEDRIVEROBJECT to ask the driver to      *
 *                  fill out the 16-bit info that goes into DDHALINFO         *
 *                  (display modes, pixel format, HALDestroyDriver, etc)      * 
 *                                                                            *
\******************************************************************************/

WORD __export FAR PASCAL 
Control1(
    LPVOID      lpDevice, 
    UINT        fn, 
    LPVOID      pin, 
    LPVOID      pout)
{
    LPDCICMD    pcmd;
    WORD        Result;

    switch (fn) {
   
        case QUERYESCSUPPORT:
    
            switch (*((UINT FAR *)pin)) {
            
                case QUERYESCSUPPORT:
                    
                    return (TRUE);
        
                case DCICOMMAND:
                    
                    Result = DD_HAL_VERSION;
                    return (Result);
        
                default:
                    
                    Result = DIB_Control(lpDevice, fn, pin, pout);
                    DPF(
                        "DIB_Control QUERYESCSUPPORT for %d = %d", 
                        *((UINT FAR *)pin), 
                        Result);

                    return (Result);
            }
            break;
    
        //
        // Handle the DCICOMMAND sent down by DDraw.DLL
        //

        case DCICOMMAND:
    
            pcmd = (LPDCICMD)pin;

            if (! pcmd) {
                
                DPF("NULL PCMD!");
                return FALSE;
            }
            
            if (pcmd->dwCommand == DDNEWCALLBACKFNS) {
                
                LPDDHALDDRAWFNS     pfns;
    
                pfns = (LPDDHALDDRAWFNS)pcmd->dwParam1;
                
                //
                // Keep a copy of the SetInfo (in DDraw16.dll) so that 
                // we can use it to callback DirectDraw later.
                //

                ((LPDDHAL_SETINFO)(pGLInfo->lpDDHAL_SetInfo)) = pfns->lpSetInfo;
                
                return (TRUE);

            } 
            
            if (pcmd->dwCommand == DDVERSIONINFO) {
                
                LPDDVERSIONDATA pddvd  = (LPDDVERSIONDATA)pout;

                //
                // Keep the DX HAL Version
                //

                DriverData.dwDXVersion = pcmd->dwParam1;

                if (! pddvd) {
                    return (FALSE);
                }
    
                pddvd->dwHALVersion = DD_RUNTIME_VERSION;
                
                return (TRUE);
            }
                
            if (pcmd->dwCommand == DDGET32BITDRIVERNAME) {
            
                LPDD32BITDRIVERDATA     p32dd = (LPDD32BITDRIVERDATA)pout;

                dwDriverDataInitialized = 1;
                
                //
                // Let the DX HAL know that this display is uninitialized
                //

                DriverData.dwSetupThisDisplay = FALSE;
                NumHALModes = BuildHALModes();
                
                //
                // Prepare the DriverData with valid info for the 32bit side
                //

                SetupSharedData();
    
                //
                // Return the data to direct draw
                //

                lstrcpy(p32dd->szName, "perm3dd.dll");
                lstrcpy(p32dd->szEntryPoint, "DriverInit");
                p32dd->dwContext = (DWORD)&DriverData;

                return (TRUE);

            }
            
            if (pcmd->dwCommand == DDCREATEDRIVEROBJECT) {

                *((DWORD FAR *)pout) = (DWORD)DriverData.hInstance;
    
                return SetDriverInfo(FALSE);
            }
            
            break;
    
        case MOUSETRAILS:
            {
                int     nTrails = *(int *)pin;
                extern void FAR PASCAL Cursor_ChangeMouseTrails(int);
    
                Cursor_ChangeMouseTrails(nTrails);
            }
            break;

        default:
            
            DPF("Escape %d Unsupported", fn);
            break;
    
    }
    
    Result = DIB_Control(lpDevice, fn, pin, pout);
    DPF("DIB_Control Esc %d = %d", fn, Result);

    return(Result);

} // End of Control1()


/******************************************************************************\
 *                                                                            *
 * Function:    GLDD_Disable                                                  *
 *                                                                            *
\******************************************************************************/

DWORD __loadds FAR 
GLDD_Disable(
    LPVOID      lpDevice)
{
    DPF("Disable");
    return (TRUE);

} // End of Disable()


/******************************************************************************\
 *                                                                            *
 * Function:    RestoreGDIContext                                             *
 * Description: Restore the state of Permedia 2 registers to reflect the GDI  *
 *              context. Please note that this function should have the same  *
 *              functionality as as that of the function                      *
 *              VOID RestoreGDIContext(P3_THUNKEDDATA *pThisDisplay)          *
 *              in ..\dll\p2cntxt.c                                           *
 *                                                                            *
\******************************************************************************/

VOID _cdecl 
RestoreGDIContext()
{
    // DWORD   context;

    if ((! dwDriverDataInitialized) || 
        (DriverData.dwSetupThisDisplay == FALSE)) {

        return;
    }

    //
    // Restore the context ID correctly, otherwise 32 bit part will not
    // set up the context correctly 
    //

    pGLInfo->dwCurrentContext = CONTEXT_DISPLAY_HANDLE;

    WAIT_FOR_INPUT_FIFO_ENTRIES(8);

    //
    // Rasterizer
    //

    LOAD_P3_REG(RasterizerMode, dwRasterizerMode);  

    //
    // Scissor/Stipple
    //

    // LOAD_P3_REG(ScissorMode,    dwScissorMode);  

    //
    // Frame buffer read
    //

    // LOAD_P3_REG(FBReadPixel,    dwFBReadPixel);  
    LOAD_P3_REG(FBWriteMode,    dwFBWriteMode);  
    // LOAD_P3_REG(FBWindowBase,   dwFBWindowBase);  
    LOAD_P3_REG(FBReadMode,     dwFBReadMode);  
    // LOAD_P3_REG(FBWriteConfig,  dwFBWriteConfig); 

    //
    // Logic Ops
    //

    // LOAD_P3_REG(LogicalOpMode,  dwLogicalOpMode); 
}


//-----------------------------------------------------------------------------
//
// _DX_LIN_InitializeHeapManager
//
//     Request the 32 bit side to reset the heap manager
//
//-----------------------------------------------------------------------------

BOOL 
_DX_LIN_InitializeHeapManager(LinearAllocatorInfo* pAlloc)
{
    pAlloc->bResetLinAllocator = TRUE;

    return (TRUE);
    
} // _DX_LIN_InitializeHeapManager

