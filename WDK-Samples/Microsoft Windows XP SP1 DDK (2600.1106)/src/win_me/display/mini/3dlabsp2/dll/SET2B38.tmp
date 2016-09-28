/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: Direct Draw SAMPLE CODE *
 *                           ***************************************
 *
 * Module Name: ddinit.c
 *
 * This module contains the Direct Draw Initialization code for the 32-bit portion
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#define INITGUID
#include "precomp.h"

static VOID GetDDHALInfo(LPP2THUNKEDDATA pThisDisplay, DDHALINFO *pHALInfo);

static BYTE ropList95[] =
{
    (SRCCOPY >> 16),
};
static DWORD rops[DD_ROP_SPACE] = { 0 };


/*
 * Function:    InitDDHAL
 * Description: do the initialisation of the Direct Draw HAL. Basically fills in
 *              HAL and Surface Callbacks.
 */

VOID 
InitDDHAL(LPP2THUNKEDDATA pThisDisplay)
{
    // Fill in the HAL Callback pointers    
    memset(&pThisDisplay->DDHALCallbacks, 0, sizeof(DDHAL_DDCALLBACKS));
    pThisDisplay->DDHALCallbacks.dwSize = sizeof(DDHAL_DDCALLBACKS);
    pThisDisplay->DDHALCallbacks.WaitForVerticalBlank   = 
                            (LPDDHAL_WAITFORVERTICALBLANK)DdWaitForVerticalBlank;
    pThisDisplay->DDHALCallbacks.CanCreateSurface       = 
                            (LPDDHAL_CANCREATESURFACE)DdCanCreateSurface;
    pThisDisplay->DDHALCallbacks.GetScanLine            = 
                            (LPDDHAL_GETSCANLINE)DdGetScanLine;
    pThisDisplay->DDHALCallbacks.CreateSurface          = 
                            (LPDDHAL_CREATESURFACE)DdCreateSurface;
    pThisDisplay->DDHALCallbacks.FlipToGDISurface       = 
                            (LPDDHAL_FLIPTOGDISURFACE)DdFlipToGDISurface;
    
    // Fill in the HAL Callback flags   
    pThisDisplay->DDHALCallbacks.dwFlags =  DDHAL_CB32_WAITFORVERTICALBLANK |
                                            DDHAL_CB32_GETSCANLINE | 
                                            DDHAL_CB32_CANCREATESURFACE |
                                            DDHAL_CB32_CREATESURFACE |
                                            DDHAL_CB32_FLIPTOGDISURFACE;


    // Fill in the Surface Callback pointers
    memset(&pThisDisplay->DDSurfCallbacks, 0, sizeof(DDHAL_DDSURFACECALLBACKS));
    pThisDisplay->DDSurfCallbacks.dwSize = sizeof(DDHAL_DDSURFACECALLBACKS);
    pThisDisplay->DDSurfCallbacks.DestroySurface    = (LPDDHALSURFCB_DESTROYSURFACE)DdDestroySurface;
    pThisDisplay->DDSurfCallbacks.Flip              = (LPDDHALSURFCB_FLIP)DdFlip;
    pThisDisplay->DDSurfCallbacks.Lock              = (LPDDHALSURFCB_LOCK)DdLock;
    pThisDisplay->DDSurfCallbacks.GetBltStatus      = (LPDDHALSURFCB_GETBLTSTATUS)DdGetBltStatus;
    pThisDisplay->DDSurfCallbacks.GetFlipStatus     = (LPDDHALSURFCB_GETFLIPSTATUS)DdGetFlipStatus;
    pThisDisplay->DDSurfCallbacks.Blt               = (LPDDHALSURFCB_BLT)DdBlt;
    pThisDisplay->DDSurfCallbacks.SetColorKey       = (LPDDHALSURFCB_SETCOLORKEY)DdSetColorKey;

    pThisDisplay->DDSurfCallbacks.dwFlags = DDHAL_SURFCB32_DESTROYSURFACE |
                                            DDHAL_SURFCB32_FLIP     |
                                            DDHAL_SURFCB32_LOCK     |
                                            DDHAL_SURFCB32_BLT |
                                            DDHAL_SURFCB32_GETBLTSTATUS |
                                            DDHAL_SURFCB32_GETFLIPSTATUS |
                                            DDHAL_SURFCB32_SETCOLORKEY;
    // Fill in the DDHAL Informational caps
    GetDDHALInfo(pThisDisplay, &pThisDisplay->ddhi32);
    
    return;
    
}// InitDDHAL()

/*
 * Function:    setupRops
 * Description: Sets up the ROP table
 */
static void 
setupRops(  LPBYTE  proplist, 
            LPDWORD proptable, 
            int     cnt )
{
    int         i;
    DWORD       idx, bit, rop;

    for(i=0; i<cnt; i++)
    {
        rop = proplist[i];
        idx = rop / 32; 
        bit = 1L << ((DWORD)(rop % 32));
        proptable[idx] |= bit;
    }

} /* setupRops */


/*
 * Function:    GetDDHALInfo
 * Description: Sets up Caps bits and retrieves the Direct 3D callback function pointers.
 */
static VOID 
GetDDHALInfo(LPP2THUNKEDDATA    pThisDisplay, 
             DDHALINFO          *pHALInfo)
{
    int             i;

    DBG_DD(( 5, "DDraw:GetDDHalInfo"));

    memset( pHALInfo, 0, sizeof(DDHALINFO) );
    pHALInfo->dwSize = sizeof(DDHALINFO);

    
    // The most basic DirectDraw functionality
    // All Capabilities for rendering/transfer operations (for example Blts), 
    // indicated here are only for only surfaces in video memory, or  
    // between surfaces in video memory only.
    pHALInfo->ddCaps.dwCaps =   DDCAPS_BLT |            //hw capable of blt operations.
                                DDCAPS_BLTQUEUE |       //hw capable of asynchronous blt operations.
                                DDCAPS_BLTCOLORFILL |   //hw capable of color fill with bltter
                                DDCAPS_READSCANLINE |   //hw can return the current scan line.
                                DDCAPS_3D |             //hw has 3D acceleration.
                                DDCAPS_GDI |            //hw is shared with GDI.
                                DDCAPS_ALPHA |          //hw supports alpha surfaces
                                DDCAPS_BLTDEPTHFILL |   //hw capable of depth filling Z-buffers with bltter
                                DDCAPS_BLTSTRETCH |     //hw capable of stretching during blt operations
                                DDCAPS_COLORKEY |       //hw supports color key
                                DDCAPS_CANBLTSYSMEM;    //hw is capable of bltting to or from system memory
    if (pThisDisplay->pPermediaInfo->dwBpp != 8)
    {
        pHALInfo->ddCaps.dwCaps |= DDCAPS_BLTFOURCC;    //hw capable of color space conversions during blt operation.
    }

    pHALInfo->ddCaps.ddsCaps.dwCaps =   DDSCAPS_OFFSCREENPLAIN | 
                                        DDSCAPS_PRIMARYSURFACE |
                                        DDSCAPS_FLIP |
                                        DDSCAPS_ALPHA |
                                        DDSCAPS_3DDEVICE |
                                        DDSCAPS_TEXTURE |
                                        DDSCAPS_ZBUFFER;

    pHALInfo->ddCaps.dwZBufferBitDepths = DDBD_16;  // Z Buffer is only 16 Bits

    // bit depths supported for alpha   
    pHALInfo->ddCaps.dwAlphaBltConstBitDepths       = DDBD_2 | DDBD_4 | DDBD_8;
    pHALInfo->ddCaps.dwAlphaBltPixelBitDepths       = DDBD_1 | DDBD_8;
    pHALInfo->ddCaps.dwAlphaBltSurfaceBitDepths     = DDBD_1 |  DDBD_2 | DDBD_4 | DDBD_8;

    // ROPS supported 
    setupRops( ropList95, rops, sizeof(ropList95)/sizeof(ropList95[0]));
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwRops[i] = rops[i];
    }

    /*
     * Notes from ddraw.h:
     * DDSCAPS_OFFSCREENPLAIN : Indicates that this surface is any offscreen surface 
     *                          that is not an overlay, texture, zbuffer, front buffer, 
     *                          back buffer, or alpha surface.  It is used to identify 
     *                          plain vanilla surfaces.
     * DDSCAPS_PRIMARYSURFACE : Indicates that this surface is the primary surface. 
     *                          The primary surface represents what the user is seeing at the moment.
     * DDSCAPS_FLIP           : Indicates that this surface is a part of a surface flipping structure.
     *                          When it is passed to CreateSurface the DDSCAPS_FRONTBUFFER and
     *                          DDSCAP_BACKBUFFER bits are not set. They are set by CreateSurface
     *                          on the resulting creations. The dwBackBufferCount field in the
     *                          DDSURFACEDESC structure must be set to at least 1 in order for
     *                          the CreateSurface call to succeed.  The DDSCAPS_COMPLEX capability
     *                          must always be set with creating multiple surfaces through CreateSurface.
     * DDSCAPS_ALPHA          : Indicates that this surface contains alpha-only information.
     *                          (To determine if a surface is RGBA/YUVA, the pixel format must be interrogated.)
     * DDSCAPS_3DDEVICE       : Indicates that a surface may be a destination for 3D rendering.  This
     *                          bit must be set in order to query for a Direct3D Device Interface
     *                          from this surface.
     * DDSCAPS_ZBUFFER        : Indicates that this surface is a z buffer. A z buffer does not contain
     *                          displayable information.  Instead it contains bit depth information that is
     *                          used to determine which pixels are visible and which are obscured.
     */

    pHALInfo->ddCaps.dwCaps2  = DDCAPS2_STEREO              | 
                                DDCAPS2_NOPAGELOCKREQUIRED  | 
                                DDCAPS2_CANMANAGETEXTURE    | 
                                DDCAPS2_WIDESURFACES;
    pHALInfo->ddCaps.dwSVCaps = DDSVCAPS_STEREOSEQUENTIAL;

#ifdef DDFLIP_INTERVALN
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_FLIPINTERVAL; 
#endif //#ifdef DDFLIP_INTERVALN
    /*
     * Notes from ddraw.h
     * DDCAPS2_NOPAGELOCKREQUIRED   : Driver neither requires nor prefers surfaces to be pagelocked 
     *                                when performing blts involving system memory surfaces
     * DDCAPS2_CANMANAGETEXTURE     : Driver supports management of video memory, if this flag is ON,
     *                                driver manages the texture if requested with 
     *                                DDSCAPS2_TEXTUREMANAGE on DirectX manages the texture if this 
     *                                flag is OFF and surface has DDSCAPS2_TEXTUREMANAGE on
     */

    // Special effects caps 
    pHALInfo->ddCaps.dwFXCaps = DDFXCAPS_BLTSTRETCHY |
                                DDFXCAPS_BLTSTRETCHX |
                                DDFXCAPS_BLTSTRETCHYN |
                                DDFXCAPS_BLTSTRETCHXN |
                                DDFXCAPS_BLTSHRINKY |
                                DDFXCAPS_BLTSHRINKX |
                                DDFXCAPS_BLTSHRINKYN |
                                DDFXCAPS_BLTSHRINKXN |
                                DDFXCAPS_BLTMIRRORUPDOWN |
                                DDFXCAPS_BLTMIRRORLEFTRIGHT |
                                DDFXCAPS_BLTALPHA |
                                DDFXCAPS_BLTFILTER;
    /* 
     * Notes from ddraw.h
     * DDFXCAPS_BLTSHRINKX      : DirectDraw supports arbitrary shrinking of a surface along the
     *                            x axis (horizontal direction) for blts.
     * DDFXCAPS_BLTSHRINKY      : same as DDFXCAPS_BLTSHRINKX except it is for y axis
     * DDFXCAPS_BLTSHRINKYN     : DirectDraw supports integer shrinking (1x,2x,) of a surface
     *                            along the y axis (vertical direction) for blts
     * DDFXCAPS_BLTSHRINKXN     : same as DDFXCAPS_BLTSHRINKYN except it is for x axis
     * DDFXCAPS_BLTSTRETCHX     : DirectDraw supports arbitrary stretching of a surface along the
     *                            x axis (horizontal direction) for blts.
     * DDFXCAPS_BLTSTRETCHY     : same as DDFXCAPS_BLTSTRETCHX except it is for y axis
     * DDFXCAPS_BLTSTRETCHXN    : DirectDraw supports integer stretching (1x,2x,) of a surface
     *                            along the x axis (horizontal direction) for blts.
     * DDFXCAPS_BLTSTRETCHYN    : same as DDFXCAPS_BLTSTRETCHXN except it is for y axis
     * DDFXCAPS_BLTMIRRORUPDOWN : Supports mirroring top to bottom in blt.
     * DDFXCAPS_BLTMIRRORLEFTRIGHT: Supports mirroring left to right in blt.
     * DDFXCAPS_BLTALPHA        : Driver can do alpha blending for blits.
     * DDFXCAPS_BLTFILTER       : Driver can do surface-reconstruction filtering for warped blits.
     */

    // ColorKey caps
    pHALInfo->ddCaps.dwCKeyCaps         = DDCKEYCAPS_SRCBLT | 
                                          DDCKEYCAPS_SRCBLTCLRSPACE;
    if (pThisDisplay->pPermediaInfo->dwBpp != 8)
    {
        pHALInfo->ddCaps.dwCKeyCaps     |= DDCKEYCAPS_SRCBLTCLRSPACEYUV;
    }
    /*
     * Notes from ddraw.h
     * DDCKEYCAPS_SRCBLT        : Supports transparent blting using the color key for 
     *                            the source with this surface for RGB colors.
     * DDCKEYCAPS_SRCBLTCLRSPACE: Supports transparent blting using a color space 
     *                            for the source with this surface for RGB colors.
     * DDCKEYCAPS_SRCBLTCLRSPACEYUV: Supports transparent blting using a color space for 
     *                            the source with this surface for YUV colors.
     */

    // Now fill in the caps bits indicating  
    // System Memory to Video Memory transfer operations
    pHALInfo->ddCaps.dwSVBCaps          = DDCAPS_BLT;
    pHALInfo->ddCaps.dwSVBCKeyCaps      = 0;
    pHALInfo->ddCaps.dwSVBFXCaps        = 0;

    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwSVBRops[i] = rops[i];
    }

    if (pThisDisplay->bIsAGPCapable)
    {
        DBG_DD((1, "GetDDHALInfo: P2 AGP board - supports NONLOCALVIDMEM"));

        pHALInfo->ddCaps.dwCaps2        |= DDCAPS2_NONLOCALVIDMEM | 
                                           DDCAPS2_NONLOCALVIDMEMCAPS;
        pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM | 
                                           DDSCAPS_NONLOCALVIDMEM;
    }
    else
    {
        DBG_DD((1,"GetDDHALInfo: P2 Board is NOT AGP"));
    }

    // Now fill in the caps bits indicating  
    // Video Memory to System Memory transfer operations
    pHALInfo->ddCaps.dwVSBCaps          = 0;
    pHALInfo->ddCaps.dwVSBCKeyCaps      = 0;
    pHALInfo->ddCaps.dwVSBFXCaps        = 0;
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwVSBRops[i]   = 0;
    }

    // Now fill in the caps bits indicating  
    // System Memory to System Memory transfer operations
    pHALInfo->ddCaps.dwSSBCaps          = 0;
    pHALInfo->ddCaps.dwSSBCKeyCaps      = 0;
    pHALInfo->ddCaps.dwSSBFXCaps        = 0;
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwSSBRops[i]   = 0;
    }


    // For DX5 and beyond we support this new informational callback.
    pHALInfo->GetDriverInfo = DdGetDriverInfo;
    pHALInfo->dwFlags |= DDHALINFO_GETDRIVERINFOSET;

    // now setup D3D callbacks 
    D3DHALCreateDriver( pThisDisplay, 
                        (LPD3DHAL_GLOBALDRIVERDATA*)
                            &pHALInfo->lpD3DGlobalDriverData,
                        (LPD3DHAL_CALLBACKS*) 
                            &pHALInfo->lpD3DHALCallbacks);
    
    if(pHALInfo->lpD3DGlobalDriverData == (ULONG_PTR)NULL)
    {
        // no D3D available - kill caps we set before
        pHALInfo->ddCaps.dwCaps &=  
            ~(DDCAPS_3D | DDCAPS_BLTDEPTHFILL);
        pHALInfo->ddCaps.ddsCaps.dwCaps &= 
            ~(DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER);
    }
}

/*
 * Function:    DdGetDriverInfo
 * Description: callback for various new HAL features, post DX3.
 */

DWORD CALLBACK 
DdGetDriverInfo(LPDDHAL_GETDRIVERINFODATA lpData)
{
    LPP2THUNKEDDATA   pThisDisplay;
    DWORD           dwSize;

    DBG_DD(( 2, "DDraw:GetDriverInfo"));

    // Get a pointer to the chip we are on.
    pThisDisplay = (LPP2THUNKEDDATA)lpData->dwContext;
      
    // Default to 'not supported'
    lpData->ddRVal = DDERR_CURRENTLYNOTAVAIL;
    
    // fill in supported stuff  
    if (IsEqualIID(&lpData->guidInfo, &GUID_D3DCallbacks3)) 
    {
        D3DHAL_CALLBACKS3 D3DCB3;
        DBG_DD((3,"  GUID_D3DCallbacks3"));
        
        memset(&D3DCB3, 0, sizeof(D3DHAL_CALLBACKS3));
        D3DCB3.dwSize                       = sizeof(D3DHAL_CALLBACKS3);
        D3DCB3.lpvReserved                  = NULL; 
        D3DCB3.ValidateTextureStageState    = D3DValidateTextureStageState;
        D3DCB3.DrawPrimitives2              = D3DDrawPrimitives2;
        D3DCB3.dwFlags                      |= D3DHAL3_CB32_DRAWPRIMITIVES2           |
                                               D3DHAL3_CB32_VALIDATETEXTURESTAGESTATE |
                                               0;

        lpData->dwActualSize                = sizeof(D3DHAL_CALLBACKS3);
        dwSize=min(lpData->dwExpectedSize,sizeof(D3DHAL_CALLBACKS3));
        memcpy(lpData->lpvData, &D3DCB3, dwSize);
        lpData->ddRVal = DD_OK;
    } 
    else if (IsEqualIID(&lpData->guidInfo, &GUID_D3DExtendedCaps)) 
    {
        D3DHAL_D3DEXTENDEDCAPS D3DExtendedCaps;
        DBG_DD((3,"  GUID_D3DExtendedCaps"));
            
        memset(&D3DExtendedCaps, 0, sizeof(D3DExtendedCaps));
        dwSize=min(lpData->dwExpectedSize, sizeof(D3DExtendedCaps));

        lpData->dwActualSize                = dwSize;
        D3DExtendedCaps.dwSize              = dwSize;

        // number of (multi)textures we support simultaneusly for DX6
        D3DExtendedCaps.dwFVFCaps           = 1;

        D3DExtendedCaps.dwMinTextureWidth   = 1;
        D3DExtendedCaps.dwMinTextureHeight  = 1;
        D3DExtendedCaps.dwMaxTextureWidth   = 2048;
        D3DExtendedCaps.dwMaxTextureHeight  = 2048;

        D3DExtendedCaps.dwMinStippleWidth   = 8;
        D3DExtendedCaps.dwMaxStippleWidth   = 8;
        D3DExtendedCaps.dwMinStippleHeight  = 8;
        D3DExtendedCaps.dwMaxStippleHeight  = 8;

        D3DExtendedCaps.dwTextureOpCaps =
            D3DTEXOPCAPS_DISABLE                   |
            D3DTEXOPCAPS_SELECTARG1                |
            D3DTEXOPCAPS_SELECTARG2                |
            D3DTEXOPCAPS_MODULATE                  |
            D3DTEXOPCAPS_ADD                       |
            D3DTEXOPCAPS_BLENDDIFFUSEALPHA         |
            D3DTEXOPCAPS_BLENDTEXTUREALPHA         |
            0;

        D3DExtendedCaps.wMaxTextureBlendStages      = 1;
        D3DExtendedCaps.wMaxSimultaneousTextures    = 1;

        // Full range of the integer (non-fractional) bits of the 
        // post-normalized texture indices. If the 
        // D3DDEVCAPS_TEXREPEATNOTSCALEDBYSIZE bit is set, the 
        // device defers scaling by the texture size until after 
        // the texture address mode is applied. If it isn't set, 
        // the device scales the texture indices by the texture size 
        // (largest level-of-detail) prior to interpolation. 
        D3DExtendedCaps.dwMaxTextureRepeat = 2048;

        // In order to support stencil buffers in DX6 we need besides
        // setting these caps and handling the proper renderstates to
        // declare the appropriate z buffer pixel formats here in 
        // response to the GUID_ZPixelFormats and implement the
        // Clear2 callback. Also , we need to be able to create the
        // appropriate ddraw surfaces.
#if D3D_STENCIL
        D3DExtendedCaps.dwStencilCaps =  0                      |
                                        D3DSTENCILCAPS_KEEP     |
                                        D3DSTENCILCAPS_ZERO     |
                                        D3DSTENCILCAPS_REPLACE  |
                                        D3DSTENCILCAPS_INCRSAT  |
                                        D3DSTENCILCAPS_DECRSAT  |
                                        D3DSTENCILCAPS_INVERT;
#endif

#if D3DDX7_TL
        // In order to use hw accelerated T&L we must declare 
        // how many simultaneously active lights we can handle.
        D3DExtendedCaps.dwMaxActiveLights = 0;
#endif //D3DDX7_TL

        memcpy(lpData->lpvData, &D3DExtendedCaps, dwSize);
        lpData->ddRVal = DD_OK;
    } 
    else if (IsEqualIID(&lpData->guidInfo, &GUID_ZPixelFormats)) 
    {
        DDPIXELFORMAT ddZBufPixelFormat[2];
        DWORD         dwNumZPixelFormats;
        
        DBG_DD((3,"  GUID_ZPixelFormats"));


        memset(&ddZBufPixelFormat, 0, sizeof(ddZBufPixelFormat));

#if D3D_STENCIL
        dwSize = min(lpData->dwExpectedSize, 2*sizeof(DDPIXELFORMAT));
        lpData->dwActualSize = 2*sizeof(DDPIXELFORMAT) + sizeof(DWORD);
#else
        dwSize = min(lpData->dwExpectedSize, 1*sizeof(DDPIXELFORMAT));
        lpData->dwActualSize = 1*sizeof(DDPIXELFORMAT) + sizeof(DWORD);
#endif

        // If we didn't support stencils, we would only fill one 16-bit
        // Z Buffer format since that is all what the Permedia supports.
        // Drivers that implement stencil buffer support (like this one)
        // have to report here all Z Buffer formats supported since they 
        // have to support the Clear2 callback (or the D3DDP2OP_CLEAR
        // token)
        
#if D3D_STENCIL
        dwNumZPixelFormats  = 2;
#else
        dwNumZPixelFormats  = 1;
#endif
          
        ddZBufPixelFormat[0].dwSize             = sizeof(DDPIXELFORMAT);
        ddZBufPixelFormat[0].dwFlags            = DDPF_ZBUFFER;
        ddZBufPixelFormat[0].dwFourCC           = 0;
        ddZBufPixelFormat[0].dwZBufferBitDepth  = 16;
        ddZBufPixelFormat[0].dwStencilBitDepth  = 0;
        ddZBufPixelFormat[0].dwZBitMask         = 0xFFFF;
        ddZBufPixelFormat[0].dwStencilBitMask   = 0x0000;
        ddZBufPixelFormat[0].dwRGBZBitMask      = 0;

#if D3D_STENCIL
        ddZBufPixelFormat[1].dwSize             = sizeof(DDPIXELFORMAT);
        ddZBufPixelFormat[1].dwFlags            = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
        ddZBufPixelFormat[1].dwFourCC           = 0;
        // The sum of the z buffer bit depth AND the stencil depth 
        // should be included here
        ddZBufPixelFormat[1].dwZBufferBitDepth  = 16;
        ddZBufPixelFormat[1].dwStencilBitDepth  = 1;
        ddZBufPixelFormat[1].dwZBitMask         = 0x7FFF;
        ddZBufPixelFormat[1].dwStencilBitMask   = 0x8000;
        ddZBufPixelFormat[1].dwRGBZBitMask      = 0;
#endif

        memcpy(lpData->lpvData, &dwNumZPixelFormats, sizeof(DWORD));
        memcpy((LPVOID)((LPBYTE)(lpData->lpvData) + sizeof(DWORD)), 
                        &ddZBufPixelFormat, dwSize);
        
        lpData->ddRVal = DD_OK;
    } 
    else if (IsEqualIID(&(lpData->guidInfo), 
                &GUID_D3DParseUnknownCommandCallback)) 
    {
        DBG_DD((3,"  GUID_D3DParseUnknownCommandCallback"));
        pThisDisplay->lpD3DParseUnknownCommand = (DWORD)(lpData->lpvData);
        lpData->ddRVal = DD_OK;
    }
    if (IsEqualIID(&(lpData->guidInfo), &GUID_MiscellaneousCallbacks) )
    {
        DDHAL_DDMISCELLANEOUSCALLBACKS MISC_CB;
        
        DBG_DD((3,"  GUID_MiscellaneousCallbacks"));

        memset(&MISC_CB, 0, sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS));
        MISC_CB.dwSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);

        if (pThisDisplay->bIsAGPCapable)
        {
            MISC_CB.dwFlags = DDHAL_MISCCB32_UPDATENONLOCALHEAP;
            MISC_CB.UpdateNonLocalHeap = DdUpdateNonLocalHeap;
        }
        
        dwSize = min(sizeof(MISC_CB),lpData->dwExpectedSize);
        lpData->dwActualSize = sizeof(MISC_CB);
        memcpy(lpData->lpvData, &MISC_CB, dwSize );
        lpData->ddRVal = DD_OK;
    }
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_Miscellaneous2Callbacks) ) 
    {
        DDHAL_DDMISCELLANEOUS2CALLBACKS MISC2_CB;
        
        DBG_DD((3,"  GUID_Miscellaneous2Callbacks2"));
        
        memset(&MISC2_CB, 0, sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS));
        MISC2_CB.dwSize = sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS);
        
        MISC2_CB.dwFlags  = 0 
            | DDHAL_MISC2CB32_CREATESURFACEEX 
            | DDHAL_MISC2CB32_GETDRIVERSTATE
            | DDHAL_MISC2CB32_DESTROYDDLOCAL;

        MISC2_CB.GetDriverState     = D3DGetDriverState;
        MISC2_CB.CreateSurfaceEx    = D3DCreateSurfaceEx;
        MISC2_CB.DestroyDDLocal     = D3DDestroyDDLocal;

        lpData->dwActualSize = sizeof(MISC2_CB);
        dwSize = min(sizeof(MISC2_CB),lpData->dwExpectedSize);
        memcpy(lpData->lpvData, &MISC2_CB, dwSize);
        lpData->ddRVal = DD_OK;
    }
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_DDMoreSurfaceCaps) ) 
    {
        DDMORESURFACECAPS  DDMoreSurfaceCaps;
        DDSCAPSEX           ddsCapsEx, ddsCapsExAlt;

        DBG_DD((3,"  GUID_DDMoreSurfaceCaps"));

        // fill in everything until expectedsize...
        memset(&DDMoreSurfaceCaps, 0, sizeof(DDMoreSurfaceCaps));

        // Caps for heaps 2..n
        memset(&ddsCapsEx, 0, sizeof(ddsCapsEx));
        memset(&ddsCapsExAlt, 0, sizeof(ddsCapsEx));

        DDMoreSurfaceCaps.dwSize=lpData->dwExpectedSize;

        DBG_DD((3,"  stereo support: %ld", pThisDisplay->bCanDoStereo));
        if (pThisDisplay->bCanDoStereo)
        {
            
            DDMoreSurfaceCaps.ddsCapsMore.dwCaps2 = 
                DDSCAPS2_STEREOSURFACELEFT;
            DBG_DD((4, "DdGetDriverInfo: DDSCAPS2_STEREOSURFACELEFT set"));

        }
        lpData->dwActualSize = lpData->dwExpectedSize;

        dwSize = min(sizeof(DDMoreSurfaceCaps),lpData->dwExpectedSize);
        memcpy(lpData->lpvData, &DDMoreSurfaceCaps, dwSize);

        // now fill in other heaps...
        while (dwSize < lpData->dwExpectedSize)
        {
            memcpy( (PBYTE)lpData->lpvData+dwSize, 
                    &ddsCapsEx, 
                    sizeof(DDSCAPSEX));
            dwSize += sizeof(DDSCAPSEX);
            memcpy( (PBYTE)lpData->lpvData+dwSize, 
                    &ddsCapsExAlt, 
                    sizeof(DDSCAPSEX));
            dwSize += sizeof(DDSCAPSEX);
        }

        lpData->ddRVal = DD_OK;
    }
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_NonLocalVidMemCaps) ) 
    {
        DDNONLOCALVIDMEMCAPS DDNonLocalVidMemCaps;
        int                  i;

        DBG_DD((3,"  GUID_DDNonLocalVidMemCaps"));

        memset(&DDNonLocalVidMemCaps, 0, sizeof(DDNonLocalVidMemCaps));
        DDNonLocalVidMemCaps.dwSize =sizeof(DDNonLocalVidMemCaps);

        //fill in all supported nonlocal to videomemory blts
        //
        if (pThisDisplay->bIsAGPCapable)
        {
            DDNonLocalVidMemCaps.dwNLVBCaps = DDCAPS_BLT |
                                              DDCAPS_BLTSTRETCH |
                                              DDCAPS_BLTQUEUE |
                                              DDCAPS_COLORKEY |
                                              DDCAPS_ALPHA |
                                              DDCAPS_CANBLTSYSMEM;

            DDNonLocalVidMemCaps.dwNLVBCaps2 = 0;

            DDNonLocalVidMemCaps.dwNLVBCKeyCaps=DDCKEYCAPS_SRCBLT |
                                                DDCKEYCAPS_SRCBLTCLRSPACE;
                                            

            DDNonLocalVidMemCaps.dwNLVBFXCaps = DDFXCAPS_BLTALPHA |
                                                DDFXCAPS_BLTFILTER |
                                                DDFXCAPS_BLTSTRETCHY |
                                                DDFXCAPS_BLTSTRETCHX |
                                                DDFXCAPS_BLTSTRETCHYN |
                                                DDFXCAPS_BLTSTRETCHXN |
                                                DDFXCAPS_BLTSHRINKY |
                                                DDFXCAPS_BLTSHRINKX |
                                                DDFXCAPS_BLTSHRINKYN |
                                                DDFXCAPS_BLTSHRINKXN |
                                                DDFXCAPS_BLTMIRRORUPDOWN |
                                                DDFXCAPS_BLTMIRRORLEFTRIGHT;

            if (pThisDisplay->pPermediaInfo->dwBpp != 8)
            {
                DDNonLocalVidMemCaps.dwNLVBCaps     |= DDCAPS_BLTFOURCC;
                DDNonLocalVidMemCaps.dwNLVBCKeyCaps |=DDCKEYCAPS_SRCBLTCLRSPACEYUV;
            }

            for( i = 0; i < DD_ROP_SPACE; i++ )
                DDNonLocalVidMemCaps.dwNLVBRops[i] = rops[i];
        }

        lpData->dwActualSize =sizeof(DDNonLocalVidMemCaps);

        dwSize = min(sizeof(DDNonLocalVidMemCaps),lpData->dwExpectedSize);
        memcpy(lpData->lpvData, &DDNonLocalVidMemCaps, dwSize);
        lpData->ddRVal = DD_OK;
    } 
    // We always handled it.
    return DDHAL_DRIVER_HANDLED;

}   // GetDriverInfo


