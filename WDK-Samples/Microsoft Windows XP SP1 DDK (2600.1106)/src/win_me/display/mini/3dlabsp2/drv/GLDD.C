/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: Direct Draw SAMPLE CODE *
 *                           ***************************************
 *
 * Module Name: gldd.c
 *
 * This module contains the Direct Draw Initialization code for the 
 * 16-bit portion and one Direct Draw callback DestroyDriver16. It also 
 * contains code to switch back to the GDI context.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/


#define WIN95_THUNKING

#include "glint.h"
#pragma warning(disable:4005)
#include <ddrawi.h>
#pragma warning(default:4005)
#include "glddif.h"
#include "linalloc.h"
#include "glddtk.h"
#include "ramdac.h"
#include "p2string.h"
#include "debug.h"


/********************************************/
/* External Functions Prototypes            */
/********************************************/
extern WORD FAR PASCAL DIB_Control( LPVOID  lpDevice, 
                                    UINT    fn, 
                                    LPVOID  pin,
                                    LPVOID  pout);
extern VOID _cdecl DisableOffscreenHeap();
extern BOOL _cdecl EnableOffscreenHeap(WORD cxScreen, 
                                       WORD cyScreen);

/********************************************/
/* External Variables                       */
/********************************************/
extern LPGLINTINFO PASCAL pGLInfo;//Glint Information from main driver

/********************************************/
/* Prototypes of functions defined here     */
/********************************************/

static DWORD __loadds FAR PASCAL DestroyDriver16(LPDDHAL_DESTROYDRIVERDATA pddd);

VOID _cdecl RestoreGDIContext();
static void SetRGBABitMasks(LPDDHALMODEINFO   pThisMode);
static void BuildPixelFormat(LPDDHALMODEINFO pmode, 
                             LPDDPIXELFORMAT pdpf);
static void FillHalModeInfo(LPDDHALMODEINFO     pThisMode,
                            DWORD               Width, 
                            DWORD               Height, 
                            DWORD               Bpp,
                            WORD               MaxRefreshRate);
static int  BuildHALModes();
static LPDDHALMODEINFO SetupSharedData();

/********************************************/
/* Global variables                         */
/********************************************/
P2THUNKEDDATA             DriverData; //Global driver data
LPP2THUNKEDDATA           pDriverData;//Global driver data
static DWORD              dwDriverDataInitialized = 0;

/*
 * a sample of video memory usage
 *
 * - the first chunk can be used for anything but overlays and textures
 *   (offscreen memory only)
 * - the second chunk can be used for anything but plain offscreen surfaces
 *   or texture surfaces (overlay memory only)
 * - the third chunk can be used for anything but plain offscreen surfaces
 *   or overlay surfaces (texture memory only)
 *
 * if the allocations fails, note that any chunk can be used to allocatedd
 * any piece of memory (that is way the caps bits are ZERO in the
 * alternate caps field)
 *
 */


/********************************************/
/* Misc Definitions                         */
/********************************************/
#define QUERYESCSUPPORT     8   //defined because we can't include GDI defns 
                                //and GDIDEFS.INC

static int                  NumHALModes = 0;
static DDHALMODEINFO        ddmodeInfo[50] = {0};

#define FOURCC_YUV422       0x32595559

static DWORD fourCC[] =
{
    FOURCC_YUV422,
};

/********************************************/
/* Callback arrays                          */
/********************************************/
/*
 * callbacks from the DIRECTDRAW object
 */
static DDHAL_DDCALLBACKS cbDDCallbacks =
{
    sizeof( DDHAL_DDCALLBACKS ),
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

/*
 * callbacks from the DIRECTDRAWSURFACE object
 */
static DDHAL_DDSURFACECALLBACKS cbDDSurfaceCallbacks =
{
    sizeof( DDHAL_DDSURFACECALLBACKS ),
    0,              // dwFlags
    NULL,           // DestroySurface (32 Bit)
    NULL,           // Flip (32 Bit)
    NULL,           // SetClipList
    NULL,           // Lock (32 Bit)
    NULL,           // Unlock
    NULL,           // Blt (32 Bit)
    NULL,           // SetColorKey (32 Bit)
    NULL,           // AddAttachedSurface
    NULL,           // GetBltStatus (32 Bit)
    NULL,           // GetFlipStatus (32 Bit)
    NULL,           // UpdateOverlay
    NULL,           // SetOverlayPosition
    NULL,           // reserved4
    NULL            // SetPalette
};

static DDHAL_DDPALETTECALLBACKS cbDDPaletteCallbacks =
{
    sizeof ( DDHAL_DDPALETTECALLBACKS ),
    0,
    NULL,
    NULL
};

/*
 * Permedia 2 device dependant definitions
 */

#define LOAD_P2_REG(Register, Value) \
    VideoPortWriteRegisterUlong((ULONG FAR *)&(((PREGISTERS)(pGLInfo->pRegs))->Glint.Register), Value)  

#define READ_P2_REG(Register)       \
    VideoPortReadRegisterUlong((ULONG FAR *)&(((PREGISTERS)(pGLInfo->pRegs))->Glint.Register)) 
  
#define WAIT_FOR_INPUT_FIFO_ENTRIES(n) \
        while (READ_P2_REG(InFIFOSpace) < n);
/*
 * Function:    vWaitDMAComplete
 * Description: Waits till the DMA engine is idle
 */
static VOID 
vWaitDMAComplete()
{
    ULONG count;

    if (!(pGLInfo->GlintBoardStatus & GLINT_DMA_COMPLETE)) 
    {
        if (pGLInfo->GlintBoardStatus & GLINT_INTR_CONTEXT) 
        { 
            /* do any VBLANK wait, wait Q to empty and last DMA to complete */ 
            while (pGLInfo->frontIndex != pGLInfo->backIndex); 
        }
        
        while ((count = READ_P2_REG(DMACount)) > 0)
        {
            if (count < 32)
                count = 1;      
            else                                
                count <<= 1;                    
            while (--count > 0);            
        }
        pGLInfo->GlintBoardStatus |= GLINT_DMA_COMPLETE;    
    }
}

/*
 * Function:    vSyncWithPermedia
 * Description: Waits till the DMA is idle and makes sure that the Input FIFO 
 *              is completely flushed out.
 */

static VOID
vSyncWithPermedia()
{
    vWaitDMAComplete();
    WAIT_FOR_INPUT_FIFO_ENTRIES(3);
    LOAD_P2_REG(FilterMode, 0x400);
    LOAD_P2_REG(Sync, 0x000);
    LOAD_P2_REG(FilterMode, 0x000);

    do {
        while (READ_P2_REG(OutFIFOWords) == 0);
    } while (READ_P2_REG(GPFifo[0]) != 0x188);
}

static void 
SetRGBABitMasks(LPDDHALMODEINFO   pThisMode)
{        
    switch (pThisMode->dwBPP)
    {
        case 8:
            pThisMode->dwRBitMask = 0;
            pThisMode->dwGBitMask = 0;
            pThisMode->dwBBitMask = 0;
            pThisMode->dwAlphaBitMask = 0;
        break;

        case 16:
            pThisMode->dwRBitMask = 0x7c00L;
            pThisMode->dwGBitMask = 0x03e0L;
            pThisMode->dwBBitMask = 0x001fL;
            pThisMode->dwAlphaBitMask = 0x8000L;
        break;

        case 32:
            pThisMode->dwRBitMask = 0x00ff0000L;
            pThisMode->dwGBitMask = 0x0000ff00L;
            pThisMode->dwBBitMask = 0x000000ffL;
            pThisMode->dwAlphaBitMask = 0xff000000L;
        break;
    }
}

/*
 * Function:    BuildPixelFormat
 * Description: Generate a pixel format structure based on the mode
 *              This example works only with RGB surfaces
 */
static void 
BuildPixelFormat(LPDDHALMODEINFO pmode,
                 LPDDPIXELFORMAT pdpf )
{
    pdpf->dwSize        = sizeof( DDPIXELFORMAT );
    pdpf->dwFourCC      = 0;
    pdpf->dwFlags       = DDPF_RGB;
    pdpf->dwRGBBitCount = pmode->dwBPP;

    if(pmode->dwBPP == 8 )
    {
        pdpf->dwFlags |= DDPF_PALETTEINDEXED8;
    }

    pdpf->dwRBitMask        = pmode->dwRBitMask;;
    pdpf->dwGBitMask        = pmode->dwGBitMask;;
    pdpf->dwBBitMask        = pmode->dwBBitMask;;
    pdpf->dwRGBAlphaBitMask = pmode->dwAlphaBitMask;;

} /* BuildPixelFormat */

/*
 * Function:    FillHalModeInfo
 * Description: Fills an entry in the DDHALMODEINFO table
 */
static void 
FillHalModeInfo(LPDDHALMODEINFO     pThisMode,
                DWORD               Width, 
                DWORD               Height, 
                DWORD               Bpp,
                WORD                MaxRefreshRate)
{
    DWORD dwTotalScanLines;

    pThisMode->dwWidth        = Width;              
    pThisMode->dwHeight       = Height;                 
    pThisMode->lPitch         = Width * (Bpp / 8);          
    pThisMode->dwBPP          = Bpp;
    if (Bpp == 8)
        pThisMode->wFlags     = DDMODEINFO_PALETTIZED;          
    pThisMode->wRefreshRate   = MaxRefreshRate;
    pThisMode->wFlags         |= DDMODEINFO_MAXREFRESH;
    
    SetRGBABitMasks(pThisMode);

    if ((pThisMode->dwWidth >= 320)&&(pThisMode->dwHeight >= 240))
    {   
        dwTotalScanLines = pGLInfo->ddFBSize/pThisMode->lPitch;
        if (dwTotalScanLines > (pThisMode->dwHeight*4))
        {
            pThisMode->wFlags   |= DDMODEINFO_STEREO;           
        }
    }
}

/*
 * Function:    BuildHALModes
 * Description: Generates the DDHALMODEINFO table
 */
static int 
BuildHALModes()
{
    int numModes = 0;

    // Clear out all the modes.
    memset(&ddmodeInfo[0], 0, (sizeof(ddmodeInfo) / sizeof(ddmodeInfo[0])));

    FillHalModeInfo(&ddmodeInfo[numModes], 320, 200, 8, 120);   numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 320, 200, 16, 120);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 320, 200, 32, 120);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 320, 240, 8, 120);   numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 320, 240, 16, 120);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 320, 240, 32, 120);  numModes++;
    //FillHalModeInfo(&ddmodeInfo[numModes], 512, 384, 8, 120);   numModes++;
    //FillHalModeInfo(&ddmodeInfo[numModes], 512, 384, 16, 120);    numModes++;
    //FillHalModeInfo(&ddmodeInfo[numModes], 512, 384, 32, 120);    numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 640, 400, 8, 120);   numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 640, 400, 16, 120);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 640, 400, 32, 120);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 640, 480, 8, 100);   numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 640, 480, 16, 100);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 640, 480, 32, 100);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 800, 600, 8, 100);   numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 800, 600, 16, 100);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 800, 600, 32, 100);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1024, 768, 8, 100);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1024, 768, 16, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1024, 768, 32, 100); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1152, 870, 8, 85);   numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1152, 870, 16, 85);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1152, 870, 32, 85);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1280, 1024, 8, 85);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1280, 1024, 16, 85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1280, 1024, 32, 85); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1600, 1200, 8, 75);  numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1600, 1200, 16, 75); numModes++;
    FillHalModeInfo(&ddmodeInfo[numModes], 1600, 1200, 32, 75); numModes++;

    return numModes;
}   /* BuildHALModes */


/*
 * Function:    SetupSharedData
 * Description: Called to fill in DriverData with information, ready for when
 *              DriverInit gets called on the 32 bit side.  This data is 
 *              required DriverInit gets called on the 32 bit side. This data
 *              required so that the 32 bit side can properly initialise.
 *              This function is called again when there is a mode change, 
 *              as most of the things it fills in will change at this point.
 */
static LPDDHALMODEINFO 
SetupSharedData()
{
    LPDDHALMODEINFO pmode;
    int             i;

    // Control points to the RAMDAC
    // lpGLInfo is a pointer to the DisplayDriver state.
    DriverData.control          = (FLATPTR) pGLInfo->dwpRegisters;
    DriverData.pPermediaInfo    = (LPPERMEDIA2INFO)(((DWORD)pGLInfo->dwDSBase) + 
                                                    ((DWORD)pGLInfo & 0xffff));
    DriverData.bIsAGPCapable     = (((PREGISTERS)pGLInfo->pRegs)->Glint.ChipConfig) & PM_CHIPCONFIG_AGPCAPABLE;

    DriverData.dwLocalBuffer    = 
    DriverData.dwScreenFlatAddr = (FLATPTR) pGLInfo->dwpFrameBuffer;

    // Initialise the modes, and fill in the pixel format of the display
    for(pmode = ddmodeInfo, i = NumHALModes; i--; pmode++)
    {
        if( (pmode->dwHeight    == (DWORD)pGLInfo->dwScreenHeight) &&
            (pmode->dwWidth     == (DWORD)pGLInfo->dwScreenWidth) &&
            (pmode->dwBPP       == (DWORD)pGLInfo->dwBpp) )
        break;
    }
    BuildPixelFormat( pmode, &DriverData.ddpfDisplay );

    if (pmode->wFlags & DDMODEINFO_STEREO)
        DriverData.bCanDoStereo = TRUE;
    else
        DriverData.bCanDoStereo = FALSE;

    // Setup the display size information
    // dwScreenWidth, dwScreenHeight = current resolution
    // cxMemory = Pixels across for one scanline (not necessarily the same as the screen width)
    // cyMemory = Scanline height of the memory
    // dwScreenStart = First visible line of display
    DriverData.dwScreenWidth    = pGLInfo->dwScreenWidth;
    DriverData.dwScreenHeight   = pGLInfo->dwScreenHeight;
    DriverData.cxMemory         = pGLInfo->dwScreenWidth;
    DriverData.cyMemory         = pGLInfo->ddFBSize / 
                                    (DriverData.dwScreenWidth <<  (DriverData.ddpfDisplay.dwRGBBitCount >> 4));

    DriverData.dwScreenStart    = DriverData.dwScreenFlatAddr  + pGLInfo->dwScreenBase;

    // Usefull constants used during blits.
    // = 2,1,0 for 32,16,8 depth.  Shifts needed to calculate bytes/pixel
    DriverData.bPixShift        = (BYTE)DriverData.ddpfDisplay.dwRGBBitCount >> 4; 
    // = 0,1,2 for 32/16/8.
    DriverData.bBppShift        = 2 - DriverData.bPixShift;
    // = 3,1,0 for 8,16,32 bpp
    DriverData.dwBppMask        = 3 >> DriverData.bPixShift;

    return (pmode);
}


/*
 * Function:    SetDriverInfo
 * Description: Called at escape time, or at mode change time.  
 *              Prepares the HAL data and calls directdraw with it.
 */ 

BOOL 
SetDriverInfo( BOOL reset )
{
    DDHALINFO           ddhi;
    LPDDHALMODEINFO     pmode;
    BOOL                bSuccess;
    LPVIDMEM            pVm;

    if (!reset)
    {   //This is the first create call
        //disable the 2D Rectangular Offscreen heap 
        DisableOffscreenHeap();
    }

    // reset == TRUE if there has been a mode change.
    DriverData.bReset               = reset;

    // Setup the information that is shared with the 32 bit side
    pmode = SetupSharedData();

    pGLInfo->DisabledByGLDD         |= D3D_DISABLED;

    // Copy the HAL callbacks
    cbDDCallbacks                   = DriverData.DDHALCallbacks;

    // Remember to fill in the one 16 bit callback - destroydriver
    cbDDCallbacks.DestroyDriver     = DestroyDriver16;

    // Copy the Surface callbacks
    cbDDSurfaceCallbacks            = DriverData.DDSurfCallbacks;

    // initialize the DDHALINFO struct
    memset( &ddhi, 0, sizeof( ddhi ) );

    // Get the HAL info data that was filled in the 32-Bit side
    ddhi                            = DriverData.ddhi32;
    ddhi.dwSize                     = sizeof( ddhi );
    ddhi.lpPDevice                  = (void*)pGLInfo->lpDriverPDevice;
    ddhi.hInstance                  = (DWORD)DriverData.hInstance;

    DPF("hInstance: 0x%lx", (DWORD)ddhi.hInstance);
    DPF("lpPDevice: 0x%lx", (DWORD)ddhi.lpPDevice);
    /*
     * describe the current primary surface
     * (ptr, dimensions, mode)
     */
    ddhi.vmiData.fpPrimary          = (FLATPTR) pGLInfo->dwpFrameBuffer + pGLInfo->dwScreenBase;
    ddhi.dwModeIndex                = 0;
    ddhi.vmiData.dwDisplayHeight    = (DWORD)pGLInfo->dwScreenHeight;
    ddhi.vmiData.dwDisplayWidth     = (DWORD)pGLInfo->dwScreenWidth;
    ddhi.vmiData.lDisplayPitch      = (DWORD)pGLInfo->dwScreenWidthBytes;

    /*
     * mode information
     */
    ddhi.dwModeIndex            = pmode - ddmodeInfo;
    ddhi.dwNumModes             = NumHALModes;
    ddhi.lpModeInfo             = ddmodeInfo;
    ddhi.vmiData.ddpfDisplay    = DriverData.ddpfDisplay;

    if (ddhi.dwModeIndex >= (DWORD)NumHALModes) 
    {
        // Fail initialisation in this case.
        if (pGLInfo->DisabledByGLDD)
            pGLInfo->DisabledByGLDD &= ~D3D_DISABLED;
        // Enable the 2D Recangular offscreen heap manager
        EnableOffscreenHeap((WORD)pGLInfo->dwScreenWidth, 
                            (WORD)pGLInfo->dwScreenHeight);
        return 0;
    }

    // Store the amount of memory being subtracted (bytes)
    DriverData.dwMemStart       = (FLATPTR)pGLInfo->dwpFrameBuffer + pGLInfo->dwOffscreenBase ;
    DriverData.dwMemEnd         = (FLATPTR)pGLInfo->dwpFrameBuffer + pGLInfo->ddFBSize - 1 ;
    // Ensure the pointers are DWORD aligned
    DriverData.dwMemStart       = (DriverData.dwMemStart + 3) & ~3;
    DriverData.dwMemEnd         = (DriverData.dwMemEnd + 3) & ~3;

    DPF("Heap Attributes:");
    DPF("  Primary: %d (pitch: %d) x %d", wScreenWidth, pmode->lPitch, (DWORD)pGLInfo->dwScreenHeight);
    DPF("  Start of Heap Memory: 0x%lx", DriverData.dwMemStart);
    DPF("  End of Heap Memory: 0x%lx", DriverData.dwMemEnd);

    // The linear allocator works on a global structure called pDriverData
    pDriverData                 = &DriverData;

    // Setup the YUV modes for Video playback acceleration.
    ddhi.ddCaps.dwNumFourCCCodes    = sizeof( fourCC ) / sizeof( fourCC[0] );
    ddhi.lpdwFourCC                 = fourCC;


    /*
     * set up the pointer to the first available video memory after
     * the primary surface
     */

    pVm = &DriverData.vidMem[0];

    pVm->dwFlags            = VIDMEM_ISLINEAR;
    pVm->fpStart            = DriverData.dwMemStart;
    pVm->fpEnd              = DriverData.dwMemEnd;
    pVm->ddsCapsAlt.dwCaps  = 0;

    DPF("Allocating Off screen heap");

    // We must be on DX2,3 so report a dummy heap
    if (DriverData.dwDXVersion == 0)
        pVm->ddsCaps.dwCaps     = 0;                        
    else
        pVm->ddsCaps.dwCaps     = DDSCAPS_OFFSCREENPLAIN;   

    ++pVm;

    // Is this an AGP P2 board
    if (DriverData.bIsAGPCapable)
    {
        DPF("Initialised AGP Heap for P2");
        pVm->dwFlags            =   VIDMEM_ISNONLOCAL | 
                                    VIDMEM_ISLINEAR | 
                                    VIDMEM_ISWC;
        pVm->fpStart            =   0;
        pVm->fpEnd              =   (((DWORD)1024 * (DWORD)1024) * (DWORD)8) - (DWORD)1;   
                                        // 8 MB of addressable AGP memory
        pVm->ddsCaps.dwCaps     =   DDSCAPS_3DDEVICE | 
                                    DDSCAPS_FRONTBUFFER |
                                    DDSCAPS_BACKBUFFER | 
                                    DDSCAPS_ZBUFFER;
        pVm->ddsCapsAlt.dwCaps  =   DDSCAPS_3DDEVICE | 
                                    DDSCAPS_FRONTBUFFER |
                                    DDSCAPS_BACKBUFFER | 
                                    DDSCAPS_ZBUFFER;
        ++pVm;
    }

    // Fill in the number of heaps.
    DriverData.dwNumHeaps           = 
    ddhi.vmiData.dwNumHeaps         = pVm - &DriverData.vidMem[0];
    
    /*
     * Fill in your heap info
     *
     * The first array in the structure is pass 1 memory.  DDRAW
     * will run through this list in order to try to allocate memory.
     * The caps specified are surface types that CANNOT be allocated in
     * the pool.
     *
     * The second array is pass 2 memory.   DDRAW will run through this
     * list in order to allocate memory if pass1 fails. Again, The caps
     * specified are surface types that CANNOT be allocated in the pool.
     */
    ddhi.vmiData.pvmList            = &DriverData.vidMem[0];

    // required alignments of the scanlines of each kind of memory 
    ddhi.vmiData.dwOffscreenAlign   = sizeof( DWORD );
    ddhi.vmiData.dwOverlayAlign     = sizeof( DWORD );
    ddhi.vmiData.dwTextureAlign     = sizeof( DWORD );
    ddhi.vmiData.dwAlphaAlign       = sizeof( DWORD );
    // Z buffer needs to be aligned to 32 words at least, as that is the minimum
    // partial product that glint supports.
    ddhi.vmiData.dwZBufferAlign     = sizeof( DWORD ) << 5;

    /*
     * callback functions
     */
    ddhi.lpDDCallbacks              = &cbDDCallbacks;
    ddhi.lpDDSurfaceCallbacks       = &cbDDSurfaceCallbacks;
    ddhi.lpDDPaletteCallbacks       = &cbDDPaletteCallbacks;

    // Call DirectDraw to create the driver object
    bSuccess = ((LPDDHAL_SETINFO)(pGLInfo->lpDDHAL_SetInfo))(&ddhi, reset);

    if (!bSuccess) 
    {
        // Fail initialisation in this case.
        if (pGLInfo->DisabledByGLDD)
            pGLInfo->DisabledByGLDD &= ~D3D_DISABLED;

        // Enable the 2D Recangular offscreen heap manager
        EnableOffscreenHeap((WORD)pGLInfo->dwScreenWidth, 
                            (WORD)pGLInfo->dwScreenHeight);
    }

    return bSuccess;
} /* SetDriverInfo */

/*
 * Function:    DestroyDriver16
 * Description: Direct Draw HAL Callback function.
 */
static DWORD __loadds FAR PASCAL 
DestroyDriver16( LPDDHAL_DESTROYDRIVERDATA pddd )
{
    // Reset the pointer to the DDRAW callback
    ((LPDDHAL_SETINFO)(pGLInfo->lpDDHAL_SetInfo)) = NULL;
    
    pddd->ddRVal = DD_OK;

    if (pGLInfo->DisabledByGLDD)
        pGLInfo->DisabledByGLDD &= ~D3D_DISABLED;

    if (dwDriverDataInitialized)
    {
        vSyncWithPermedia();
        RestoreGDIContext();
    }

    // This flag says to the HAL that we have not setup the Global 32 bit
    // Info for this display card.  It uses it to setup DMA, chip specific data,
    // etc.
    DriverData.bSetupThisDisplay = FALSE;

    // Enable the 2D Recangular offscreen heap manager
    EnableOffscreenHeap((WORD)pGLInfo->dwScreenWidth, 
                        (WORD)pGLInfo->dwScreenHeight);

    // Reset the driver version to 0 so it will be filled in again.
    DriverData.dwDXVersion = 0;

    return DDHAL_DRIVER_HANDLED;

} /* DestroyDriver16 */


/*
 * Function:    Control1
 * Description: Called whenever the Escape function is invoked
 */
                                        
WORD __export FAR PASCAL 
Control1(LPVOID  lpDevice, 
         UINT    fn, 
         LPVOID  pin, 
         LPVOID  pout)
{
    LPDCICMD    pcmd;
    WORD        Result;

    switch( fn )
    {
        case QUERYESCSUPPORT:
        switch(*((UINT FAR *)pin) )
        {
            case QUERYESCSUPPORT:
            return TRUE;
    
            case DCICOMMAND:
                Result = DD_HAL_VERSION;
            return Result;

            default:
                Result = DIB_Control(lpDevice, fn, pin, pout);
                DPF("DIB_Control QUERYESCSUPPORT for %d = %d", *((UINT FAR *)pin), Result);
            return(Result);
        }
        break;
        
        case DCICOMMAND:
            pcmd = (LPDCICMD)pin;
            if( pcmd == NULL )
            {
                DPF( "NULL PCMD!" );
                return FALSE;
            }
            /*
             * handle the request to create a driver
             */
            if( pcmd->dwCommand == DDNEWCALLBACKFNS )
            {
                LPDDHALDDRAWFNS     pfns;

                pfns    =   (LPDDHALDDRAWFNS) pcmd->dwParam1;
                // Keep a copy of the SetInfo function we use to call DirectDraw
                ((LPDDHAL_SETINFO)(pGLInfo->lpDDHAL_SetInfo)) = pfns->lpSetInfo;
                return TRUE;
            }
            else if( pcmd->dwCommand == DDGET32BITDRIVERNAME )
            {
                LPDD32BITDRIVERDATA p32dd = (LPDD32BITDRIVERDATA) pout;

                lstrcpy( p32dd->szName, "gldd32.dll");
                lstrcpy( p32dd->szEntryPoint, "DriverInit" );
                p32dd->dwContext = (DWORD)&DriverData;
                memset( &DriverData, 0, sizeof( P2THUNKEDDATA ) );
                dwDriverDataInitialized = 1;
                // Let the DX HAL know that this display is uninitialized
                DriverData.bSetupThisDisplay = FALSE;
                NumHALModes = BuildHALModes();
                // Prepare the DriverData with valid info for the 32bit side
                SetupSharedData();

                return TRUE;
            }
            else if( pcmd->dwCommand == DDCREATEDRIVEROBJECT )
            {
                *((DWORD FAR *) pout) = (DWORD) DriverData.hInstance;

                return SetDriverInfo(FALSE);
            }
            else if ( pcmd->dwCommand == DDVERSIONINFO )
            {
                LPDDVERSIONDATA pddvd  = (LPDDVERSIONDATA)pout;

                // Keep the DX HAL Version
                DriverData.dwDXVersion = pcmd->dwParam1;

                if( !pddvd )
                    return FALSE;

                pddvd->dwHALVersion = DD_RUNTIME_VERSION;
                return TRUE;
            } 

        break;

        case MOUSETRAILS:
        {
            int nTrails = *(int *)pin;
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
} /* Control */


/*
 * Function:    GLDD_Disable
 */
DWORD __loadds FAR GLDD_Disable( LPVOID lpDevice )
{
    DPF( "Disable" );
    return TRUE;

} /* Disable */


/*
 * Function:    RestoreGDIContext
 * Description: Restore the state of Permedia 2 registers to reflect the GDI context.
 *              Please note that this function should have the same functionality as
 *              as that of the function 
 *              VOID RestoreGDIContext(LPP2THUNKEDDATA pThisDisplay)
 *              in ..\dll\p2cntxt.c
 */

VOID _cdecl 
RestoreGDIContext()
{
    DWORD   context;

    if ((!dwDriverDataInitialized)||(DriverData.bSetupThisDisplay == FALSE))
    {
        return;
    }
    DriverData.dwCurrentD3DContext = 0;
    if (context = DriverData.dwD3DorDDContext)
    {
        PP2_REGISTER_CONTEXT pRegContext = &DriverData.P2RegisterContext;
        
        DriverData.dwD3DorDDContext = 0;

        if (context == D3D_OR_DDRAW_CONTEXT0)
        {
            WAIT_FOR_INPUT_FIFO_ENTRIES(8);
            // Rasterizer
            LOAD_P2_REG(RasterizerMode,         pRegContext->dwRasterizerMode);  
            // Scissor/Stipple
            LOAD_P2_REG(ScissorMode,            pRegContext->dwScissorMode);  
            // frame buffer read
            LOAD_P2_REG(FBReadPixel,            pRegContext->dwFBReadPixel);  
            LOAD_P2_REG(FBWriteMode,            pRegContext->dwFBWriteMode);  
            LOAD_P2_REG(FBWindowBase,           pRegContext->dwFBWindowBase);  
            LOAD_P2_REG(FBReadMode,             pRegContext->dwFBReadMode);  
            LOAD_P2_REG(FBWriteConfig,          pRegContext->dwFBWriteConfig); 
            // Logic Ops
            LOAD_P2_REG(LogicalOpMode,          pRegContext->dwLogicalOpMode); 
            return;
        }
        
        if (context == D3D_OR_DDRAW_CONTEXT1)
        {
            WAIT_FOR_INPUT_FIFO_ENTRIES(18);
            // Delta
            // Rasterizer
            LOAD_P2_REG(RasterizerMode,         pRegContext->dwRasterizerMode);  
            // Scissor/Stipple
            LOAD_P2_REG(ScissorMode,            pRegContext->dwScissorMode);  
            // local buffer
            // stencil/depth
            // texture address
            LOAD_P2_REG(TextureAddressMode,     pRegContext->dwTextureAddressMode);  
            // texture read
            LOAD_P2_REG(FxTextureReadMode,      pRegContext->dwTextureReadMode);  
            LOAD_P2_REG(TextureDataFormat,      pRegContext->dwTextureDataFormat);  
            LOAD_P2_REG(TextureBaseAddress,     pRegContext->dwTextureBaseAddress);  
            LOAD_P2_REG(TextureMapFormat,       pRegContext->dwTextureMapFormat);  
            LOAD_P2_REG(SStart,                 pRegContext->dwSStart);  
            LOAD_P2_REG(TStart,                 pRegContext->dwTStart);  
            LOAD_P2_REG(dSdx,                   pRegContext->dwdSdx);  
            LOAD_P2_REG(dTdyDom,                pRegContext->dwdTdyDom);  
            // yuv
            // local buffer write
            // frame buffer read
            LOAD_P2_REG(FBReadPixel,            pRegContext->dwFBReadPixel);  
            LOAD_P2_REG(FBWriteMode,            pRegContext->dwFBWriteMode);  
            LOAD_P2_REG(FBWindowBase,           pRegContext->dwFBWindowBase);  
            LOAD_P2_REG(FBReadMode,             pRegContext->dwFBReadMode);  
            LOAD_P2_REG(FBWriteConfig,          pRegContext->dwFBWriteConfig); 
            // color DDA
            // Texture/Fog/Blend
            LOAD_P2_REG(TextureColorMode,       pRegContext->dwTextureColorMode);  
            // Color Format
            // Logic Ops
            LOAD_P2_REG(LogicalOpMode,          pRegContext->dwLogicalOpMode); 
            // Frame Buffer Write
            // Host Out
            return ;
        }
        if (context == D3D_OR_DDRAW_CONTEXT2)
        {
            WAIT_FOR_INPUT_FIFO_ENTRIES(47);
            // Delta
            LOAD_P2_REG(DeltaMode,               0);  
            // Rasterizer
            LOAD_P2_REG(RasterizerMode,          pRegContext->dwRasterizerMode);  
            // Scissor/Stipple
            LOAD_P2_REG(ScissorMode,             pRegContext->dwScissorMode);  
            LOAD_P2_REG(AreaStippleMode,         0);
            LOAD_P2_REG(AreaStipplePattern0,     pRegContext->AreaStipplePattern[0]);
            LOAD_P2_REG(AreaStipplePattern1,     pRegContext->AreaStipplePattern[1]);
            LOAD_P2_REG(AreaStipplePattern2,     pRegContext->AreaStipplePattern[2]);
            LOAD_P2_REG(AreaStipplePattern3,     pRegContext->AreaStipplePattern[3]);
            LOAD_P2_REG(AreaStipplePattern4,     pRegContext->AreaStipplePattern[4]);
            LOAD_P2_REG(AreaStipplePattern5,     pRegContext->AreaStipplePattern[5]);
            LOAD_P2_REG(AreaStipplePattern6,     pRegContext->AreaStipplePattern[6]);
            LOAD_P2_REG(AreaStipplePattern7,     pRegContext->AreaStipplePattern[7]);
            LOAD_P2_REG(AreaStippleMode,         pRegContext->dwAreaStippleMode);
            // local buffer
            LOAD_P2_REG(LBReadMode,              0);  
            LOAD_P2_REG(LBWriteMode,             0);  
            // stencil/depth
            LOAD_P2_REG(DepthMode,               0);  
            LOAD_P2_REG(StencilMode,             0);  
            // texture address
            LOAD_P2_REG(TextureAddressMode,     pRegContext->dwTextureAddressMode);  
            LOAD_P2_REG(dTdx,                    0);  
            // texture read
            LOAD_P2_REG(FxTextureReadMode,      pRegContext->dwTextureReadMode);  
            LOAD_P2_REG(TextureDataFormat,      pRegContext->dwTextureDataFormat);  
            LOAD_P2_REG(TextureBaseAddress,     pRegContext->dwTextureBaseAddress);  
            LOAD_P2_REG(TextureMapFormat,       pRegContext->dwTextureMapFormat);  
            LOAD_P2_REG(SStart,                 pRegContext->dwSStart);  
            LOAD_P2_REG(TStart,                 pRegContext->dwTStart);  
            LOAD_P2_REG(dSdx,                   pRegContext->dwdSdx);  
            LOAD_P2_REG(dTdyDom,                pRegContext->dwdTdyDom);  
            LOAD_P2_REG(TextureLUTMode ,         0);  
            LOAD_P2_REG(TexelLUTIndex,           0);  
            // yuv
            LOAD_P2_REG(YUVMode,                 0);  
            // local buffer write
            // frame buffer read
            LOAD_P2_REG(FBPixelOffset,           0);  
            LOAD_P2_REG(FBReadPixel,            pRegContext->dwFBReadPixel);  
            LOAD_P2_REG(FBWriteMode,            pRegContext->dwFBWriteMode);  
            LOAD_P2_REG(FBWindowBase,           pRegContext->dwFBWindowBase);  
            LOAD_P2_REG(FBReadMode,             pRegContext->dwFBReadMode);  
            LOAD_P2_REG(FBWriteConfig,          pRegContext->dwFBWriteConfig); 
            // color DDA
            LOAD_P2_REG(ColorDDAMode,            pRegContext->dwColorDDAMode);  
            // Texture/Fog/Blend
            LOAD_P2_REG(TextureColorMode,       pRegContext->dwTextureColorMode);  
            LOAD_P2_REG(AlphaBlendMode,          pRegContext->dwAlphaBlendMode);  
            LOAD_P2_REG(FogMode,                 0);  
            // Color Format
            LOAD_P2_REG(DitherMode,              0);  
            // Logic Ops
            LOAD_P2_REG(LogicalOpMode,          pRegContext->dwLogicalOpMode); 
            // Frame Buffer Write
            LOAD_P2_REG(FBHardwareWriteMask,     pRegContext->dwFBHardwareWriteMask);
            // Host Out
            LOAD_P2_REG(FilterMode,              0);  

            LOAD_P2_REG(dXDom,                  0);  
            LOAD_P2_REG(dXSub,                  0);  
            LOAD_P2_REG(dY,                     0x10000);  
        }
    }
}

