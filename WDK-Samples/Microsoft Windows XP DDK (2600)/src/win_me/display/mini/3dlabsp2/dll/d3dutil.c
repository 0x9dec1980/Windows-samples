/******************************Module*Header**********************************\
*
*                           ***************************************
*                           * Permedia 2: Direct 3D   SAMPLE CODE *
*                           ***************************************
*
* Module Name: d3dutil.c
*
*       Contains Direct 3D utility functions.  
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

//-----------------------------------------------------------------------------
// Direct3D HAL Table.
//
// This table contains all of the HAL calls that this driver supports in the 
// D3DHAL_Callbacks structure. These calls pertain to device context, scene 
// capture, execution, textures, transform, lighting, and pipeline state. 
// None of this is emulation code. The calls take the form of a return code 
// equal to: HalCall(HalCallData* lpData). All of the information in this 
// table will be implementation specific according to the specifications of 
// the hardware.
//
//-----------------------------------------------------------------------------

#define PermediaTriCaps {                                   \
    sizeof(D3DPRIMCAPS),                                    \
    D3DPMISCCAPS_CULLCCW    |        /* miscCaps */         \
    D3DPMISCCAPS_CULLCW     |                               \
    D3DPMISCCAPS_CULLNONE   |                               \
    D3DPMISCCAPS_MASKPLANES |                               \
    D3DPMISCCAPS_MASKZ      |                               \
    D3DPMISCCAPS_LINEPATTERNREP,                            \
    D3DPRASTERCAPS_DITHER    |          /* rasterCaps */    \
    D3DPRASTERCAPS_PAT       |                              \
    D3DPRASTERCAPS_SUBPIXEL  |                              \
    D3DPRASTERCAPS_ZTEST     |                              \
    D3DPRASTERCAPS_FOGVERTEX |                              \
    D3DPRASTERCAPS_STIPPLE,                                 \
    D3DPCMPCAPS_NEVER        |                              \
    D3DPCMPCAPS_LESS         |                              \
    D3DPCMPCAPS_EQUAL        |                              \
    D3DPCMPCAPS_LESSEQUAL    |                              \
    D3DPCMPCAPS_GREATER      |                              \
    D3DPCMPCAPS_NOTEQUAL     |                              \
    D3DPCMPCAPS_GREATEREQUAL |                              \
    D3DPCMPCAPS_ALWAYS       |                              \
    D3DPCMPCAPS_LESSEQUAL,           /* zCmpCaps */         \
    D3DPBLENDCAPS_SRCALPHA |         /* sourceBlendCaps */  \
    D3DPBLENDCAPS_ONE,                                      \
    D3DPBLENDCAPS_INVSRCALPHA |      /* destBlendCaps */    \
    D3DPBLENDCAPS_ZERO        |                             \
    D3DPBLENDCAPS_ONE,                                      \
    0,                               /* alphatestCaps */    \
    D3DPSHADECAPS_COLORFLATRGB|      /* shadeCaps */        \
    D3DPSHADECAPS_COLORGOURAUDRGB |                         \
    D3DPSHADECAPS_SPECULARFLATRGB |                         \
    D3DPSHADECAPS_SPECULARGOURAUDRGB |                      \
    D3DPSHADECAPS_FOGFLAT        |                          \
    D3DPSHADECAPS_FOGGOURAUD     |                          \
    D3DPSHADECAPS_ALPHAFLATBLEND |                          \
    D3DPSHADECAPS_ALPHAFLATSTIPPLED,                        \
    D3DPTEXTURECAPS_PERSPECTIVE |   /* textureCaps */       \
    D3DPTEXTURECAPS_ALPHA       |                           \
    D3DPTEXTURECAPS_POW2        |                           \
    D3DPTEXTURECAPS_TRANSPARENCY,                           \
    D3DPTFILTERCAPS_NEAREST |       /* textureFilterCaps*/  \
    D3DPTFILTERCAPS_LINEAR,                                 \
    D3DPTBLENDCAPS_DECAL         |  /* textureBlendCaps */  \
    D3DPTBLENDCAPS_DECALALPHA    |                          \
    D3DPTBLENDCAPS_MODULATE      |                          \
    D3DPTBLENDCAPS_MODULATEALPHA |                          \
    D3DPTBLENDCAPS_COPY,                                    \
    D3DPTADDRESSCAPS_WRAP   |       /* textureAddressCaps */\
    D3DPTADDRESSCAPS_MIRROR |                               \
    D3DPTADDRESSCAPS_CLAMP  |                               \
    D3DPTADDRESSCAPS_INDEPENDENTUV,                         \
    8,                              /* stippleWidth */      \
    8                               /* stippleHeight */     \
}          

static D3DDEVICEDESC_V1 PermediaCaps = 
{
    sizeof(D3DDEVICEDESC_V1),                       /* dwSize */
    D3DDD_COLORMODEL           |                    /* dwFlags */
    D3DDD_DEVCAPS              |
    D3DDD_TRICAPS              |
    D3DDD_LINECAPS             |
    D3DDD_DEVICERENDERBITDEPTH |
    D3DDD_DEVICEZBUFFERBITDEPTH,
    D3DCOLOR_RGB /*| D3DCOLOR_MONO*/,              /* dcmColorModel */
    D3DDEVCAPS_FLOATTLVERTEX |                     /* devCaps */
    D3DDEVCAPS_DRAWPRIMITIVES2 |
    D3DDEVCAPS_DRAWPRIMITIVES2EX    |
#if D3DDX7_TL
    D3DDEVCAPS_HWTRANSFORMANDLIGHT  |
#endif //D3DDX7_TL
    D3DDEVCAPS_SORTINCREASINGZ  |
    D3DDEVCAPS_SORTEXACT |
    D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
    D3DDEVCAPS_EXECUTESYSTEMMEMORY |
    D3DDEVCAPS_TEXTUREVIDEOMEMORY,
    { sizeof(D3DTRANSFORMCAPS), 
      0 },                                         /* transformCaps */
    FALSE,                                         /* bClipping */
    { sizeof(D3DLIGHTINGCAPS), 
      0 },                                         /* lightingCaps */
    PermediaTriCaps,                               /* lineCaps */
    PermediaTriCaps,                               /* triCaps */
    DDBD_16 | DDBD_32,                             /* dwDeviceRenderBitDepth */
    DDBD_16,                                       /* Z Bit depths */
    0,                                             /* dwMaxBufferSize */
    0                                              /* dwMaxVertexCount */
};

//-----------------------------------------------------------------------------
// gD3DTextureFormats is a static structure which contains information 
// pertaining to pixel format, dimensions, bit depth, surface requirements, 
// overlays, and FOURCC codes of the supported texture formats.  These texture 
// formats will vary with the driver implementation according to the 
// capabilities of the hardware. 
//-----------------------------------------------------------------------------
DDSURFACEDESC gD3DTextureFormats [] = 
{
    // 1:5:5:5 ARGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB | DDPF_ALPHAPIXELS,      // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      16,                               // ddpfPixelFormat.dwRGBBitCount
      0x7c00,                           // ddpfPixelFormat.dwRBitMask
      0x03e0,                           // ddpfPixelFormat.dwGBitMask
      0x001f,                           // ddpfPixelFormat.dwBBitMask
      0x8000                            // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 5:6:5 RGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB,                         // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      16,                               // ddpfPixelFormat.dwRGBBitCount
      0xf800,                           // ddpfPixelFormat.dwRBitMask
      0x07e0,                           // ddpfPixelFormat.dwGBitMask
      0x001f,                           // ddpfPixelFormat.dwBBitMask
      0                                 // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 4:4:4:4 ARGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB | DDPF_ALPHAPIXELS,      // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      16,                               // ddpfPixelFormat.dwRGBBitCount
      0x0f00,                           // ddpfPixelFormat.dwRBitMask
      0x00f0,                           // ddpfPixelFormat.dwGBitMask
      0x000f,                           // ddpfPixelFormat.dwBBitMask
      0xf000                            // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 8:8:8 RGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB,                         // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      32,                               // ddpfPixelFormat.dwRGBBitCount
      0x00ff0000,                       // ddpfPixelFormat.dwRBitMask
      0x0000ff00,                       // ddpfPixelFormat.dwGBitMask
      0x000000ff,                       // ddpfPixelFormat.dwBBitMask
      0                                 // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 8:8:8:8 ARGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB | DDPF_ALPHAPIXELS,      // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      32,                               // ddpfPixelFormat.dwRGBBitCount
      0x00ff0000,                       // ddpfPixelFormat.dwRBitMask
      0x0000ff00,                       // ddpfPixelFormat.dwGBitMask
      0x000000ff,                       // ddpfPixelFormat.dwBBitMask
      0xff000000                        // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },
    // 8 bit palettized format
    {
    sizeof(DDSURFACEDESC),              // dwSize
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags
    0,                                  // dwHeight
    0,                                  // dwWidth
    0,                                  // lPitch
    0,                                  // dwBackBufferCount
    0,                                  // dwZBufferBitDepth
    0,                                  // dwAlphaBitDepth
    0,                                  // dwReserved
    NULL,                               // lpSurface
    { 0, 0 },                           // ddckCKDestOverlay
    { 0, 0 },                           // ddckCKDestBlt
    { 0, 0 },                           // ddckCKSrcOverlay
    { 0, 0 },                           // ddckCKSrcBlt
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize
      DDPF_RGB | DDPF_PALETTEINDEXED8,  // ddpfPixelFormat.dwFlags
      0,                                // ddpfPixelFormat.dwFourCC
      8,                                // ddpfPixelFormat.dwRGBBitCount
      0x00,                             // ddpfPixelFormat.dwRBitMask
      0x00,                             // ddpfPixelFormat.dwGBitMask
      0x00,                             // ddpfPixelFormat.dwBBitMask
      0x00                              // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps
    },

};

ULONG gD3DNumberOfTextureFormats = sizeof(gD3DTextureFormats) / sizeof(DDSURFACEDESC);

// Alpha Stipple patterns from Foley And Van Dam
DWORD FlatStipplePatterns[128] =
{
    //Pattern 0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        
    // Pattern 1
    0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00,

    // Pattern 2
    0xAA, 0x00, 0x22, 0x00, 0xAA, 0x00, 0x22, 0x00,

    // Pattern 3
    0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00,

    // Pattern 4
    0xAA, 0x44, 0xAA, 0x00, 0xAA, 0x44, 0xAA, 0x00,

    // Pattern 5
    0xAA, 0x44, 0xAA, 0x11, 0xAA, 0x44, 0xAA, 0x11,

    // Pattern 6
    0xAA, 0x55, 0xAA, 0x11, 0xAA, 0x55, 0xAA, 0x11,

    // Pattern 7
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,

    // Pattern 8
    0xEE, 0x55, 0xAA, 0x55, 0xEE, 0x55, 0xAA, 0x55,

    // Pattern 9
    0xEE, 0x55, 0xBB, 0x55, 0xEE, 0x55, 0xBB, 0x55,

    // Pattern 10
    0xFF, 0x55, 0xBB, 0x55, 0xFF, 0x55, 0xBB, 0x55,

    // Pattern 11
    0xFF, 0x55, 0xFF, 0x55, 0xFF, 0x55, 0xFF, 0x55,

    // Pattern 12
    0xFF, 0xdd, 0xFF, 0x55, 0xFF, 0xdd, 0xFF, 0x55,

    // Pattern 13
    0xFF, 0xdd, 0xFF, 0x77, 0xFF, 0xdd, 0xFF, 0x77,

    // Pattern 14
    0xFF, 0xFF, 0xFF, 0x77, 0xFF, 0xFF, 0xFF, 0x77,

    // Pattern 15
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/**
 * Function:    GetHardwareD3DCapabilities
 * Description: Returns the DirectD3D device capabilities for the specific
 *              hardware.
 */
D3DDEVICEDESC_V1 *
GetHardwareD3DCapabilities(LPP2THUNKEDDATA pThisDisplay)
{
#if D3D_MIPMAPPING
    // Add mipmapping cap bits to our texturing capabilities
    PermediaCaps.dpcTriCaps.dwTextureFilterCaps |= 
                                D3DPTFILTERCAPS_MIPNEAREST |
                                D3DPTFILTERCAPS_MIPLINEAR |
                                D3DPTFILTERCAPS_LINEARMIPNEAREST |
                                D3DPTFILTERCAPS_LINEARMIPLINEAR;

    PermediaCaps.dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_MIPMAPLODBIAS;
#endif

    // Can do packed 24 bit on P2.
    //PermediaCaps.dwDeviceRenderBitDepth |= DDBD_24;
    if (pThisDisplay->bIsAGPCapable)
        PermediaCaps.dwDevCaps |= D3DDEVCAPS_TEXTURENONLOCALVIDMEM;

    PermediaCaps.dwDevCaps |= D3DDEVCAPS_DRAWPRIMTLVERTEX;

    return (&PermediaCaps);
}

