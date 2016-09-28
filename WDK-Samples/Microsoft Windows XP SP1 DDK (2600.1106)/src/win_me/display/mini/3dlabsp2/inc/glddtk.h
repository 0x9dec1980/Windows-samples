/******************************Module*Header**********************************\
 *
 *                           *******************************
 *                           * Permedia 2: GDI SAMPLE CODE *
 *                           *******************************
 *
 * Module Name: glddtk.h
 *
 * This module contains definitions for the Thunked Data shared between 16bit
 * and 32 bit portions of the Permedia 2 display driver.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#ifndef _INC_GLDDTK_H_
#define _INC_GLDDTK_H_

#pragma warning( disable: 4704)

#define MAX_TEXTURE_FORMAT 15

#define D3D_OR_DDRAW_CONTEXT0   1
#define D3D_OR_DDRAW_CONTEXT1   2
#define D3D_OR_DDRAW_CONTEXT2   3

//#define UNOPTIMIZED_CNTXTSWITCH 1

typedef struct _P2_REGISTER_CONTEXT
{
    DWORD   dwRasterizerMode;
    DWORD   dwScissorMode;
    DWORD   dwLogicalOpMode;
    DWORD   dwFBReadPixel;
    DWORD   dwFBWriteMode;
    DWORD   dwFBWindowBase;
    DWORD   dwFBReadMode;
    DWORD   dwFBWriteConfig;
    DWORD   dwTextureAddressMode;
    DWORD   dwTextureColorMode;
    DWORD   dwTextureReadMode;
    DWORD   dwTextureDataFormat;
    DWORD   dwTextureBaseAddress;
    DWORD   dwTextureMapFormat;
    DWORD   dwSStart;
    DWORD   dwTStart;
    DWORD   dwdSdx;
    DWORD   dwdTdyDom;
    DWORD   dwAlphaBlendMode; // From here only for D3D
    DWORD   dwAreaStippleMode; 
    DWORD   dwColorDDAMode;
    DWORD   dwFBHardwareWriteMask;
    DWORD   dwScreenSize;
    DWORD   AreaStipplePattern[8];
} P2_REGISTER_CONTEXT, *PP2_REGISTER_CONTEXT;

typedef struct tagThunkedData
{
    DWORD           control;
    DWORD           ramdac;
    DWORD           lpMMReg;

    DWORD           dwScreenFlatAddr;   // Virt. Addr of start of screen memory   
    DWORD           dwLocalBuffer;      // Virt. Addr of start of LB memory

    // Screen settings
    DWORD           dwScreenWidth;
    DWORD           dwScreenHeight;
    DWORD           cxMemory;
    DWORD           cyMemory;

    // Virtual address of start of screen
    DWORD           dwScreenStart;
    DWORD           bPixShift;
    DWORD           bBppShift;
    DWORD           dwBppMask;
    DWORD           dwScreenModeCookie;

    DWORD           bD3DActive;
    DWORD           bReset;             // Reset flag
    DWORD           bSetupThisDisplay;	// Has this display been intialised at least once?

    // Globals that were hanging around in gldd32.
    // Some effort should be made to tidy up their use at some point...
    DWORD           bCanDoStereo;
    DWORD           bDdStereoMode;
    DWORD           dwNewDDSurfaceOffset;

    // The glintPacking array has to be one larger than the number of different
    // pixel depths that the display can support...
    DWORD           lbDepth;            // Depth buffer depth of this card
    DDPIXELFORMAT   ddpfDisplay;        // Current pixel format of display
    LPPERMEDIA2INFO pPermediaInfo;      // Shared information between the 16 bit 32 bit portions
    FPPERMEDIA2REG  pPermedia2;         // Pointer to the actual glint registers.
    DWORD           bIsAGPCapable;      // ChipConfig register for this card.    
    DWORD	    hInstance;          // HINSTANCE of gldd32.dll

    // Context Information
    DWORD                       dwD3DorDDContext;   
    P2_REGISTER_CONTEXT         P2RegisterContext;
    DWORD                       dwCurrentD3DContext;
    // DirectDraw callbacks
    DDHAL_DDCALLBACKS	        DDHALCallbacks;
    DDHAL_DDSURFACECALLBACKS    DDSurfCallbacks;

    // D3D Callbacks
    DWORD                       lpD3DGlobalDriverData;
    DWORD		        lpD3DHALCallbacks;
    DWORD                       lpD3DParseUnknownCommand;
    DDSURFACEDESC               TextureFormats[MAX_TEXTURE_FORMAT];

    DWORD           dwDXVersion;

    // These have to live here, as we could be running on 2 different cards
    // on two different displays...!
    DWORD           dwMemStart;	            // Start of the managed memory
    DWORD           dwMemEnd;		    // End of the managed memory

    DWORD           dwGARTLin;		    // Linear address of Base of AGP Memory
    DWORD           dwGARTDev;		    // High Linear address of Base of AGP Memory

    //VidMemHeap
    DWORD           dwNumHeaps;
    VIDMEM	    vidMem[4];
    // HAL info structure.
    DDHALINFO       ddhi32;

} P2THUNKEDDATA, *LPP2THUNKEDDATA;

#endif //#ifndef _INC_GLDDTK_H_
