/******************************Module*Header*********************************\
*
*                           *************************************
*                           * Permedia 2: Direct 3D SAMPLE CODE *
*                           *************************************
*
* Module Name: d3dcntxt.h
*
*  Content: D3D Context management related definitions and macros
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef _DLL_D3DCNTXT_H_
#define _DLL_D3DCNTXT_H_

#include "p2regs.h"

//-----------------------------------------------------------------------------
//               Known Issues in current version of the D3D driver
//-----------------------------------------------------------------------------
//
// Stencil support is not yet completed
//
// Some games may have some issues when running under a Permedia 2 since this
// hw has some limitations, namely:
//   1) Alpha blending modes available
//   2) Alpha channel interpolation is not possible
//   3) There is no mip mapping support
//   4) Texture filtering is applied only to textures being magnified
//
//  Also, the fill rules of the Delta setup unit don't follow exactly the D3D
//  fill rules, though in practice this shouldn't be a much of a problem for
//  most apps.


//-----------------------------------------------------------------------------
//                      Global enabling/disabling definitions
//-----------------------------------------------------------------------------
// Set to 1 to enable stencil buffer support in the driver
#define D3D_STENCIL         1

// Code stubs to implement a T&L driver. Since the P2 does not support this
// in hw, this symbol should always be set to zero.
#define D3DDX7_TL           0

// Code stubs to implement mip mapping, Since the P2 does not support this 
// natively in hw, this symbols should always be set to zero. Only shows
// how/where to grab DDI info to implement it.
#define D3D_MIPMAPPING      0

// This code shows how to add stateblock support into your DX7 driver. It is
// functional code, so this symbols should be set to one.
#define D3D_STATEBLOCKS     1

//-----------------------------------------------------------------------------
//                         DX6 FVF Support declarations
//-----------------------------------------------------------------------------
typedef struct _P2TEXCOORDS
{
    D3DVALUE    tu;
    D3DVALUE    tv;
} P2TEXCOORDS, *LPP2TEXCOORDS;

typedef struct _P2COLOR 
{
    D3DCOLOR    color;
} P2COLOR, *LPP2COLOR;

typedef struct _P2SPECULAR 
{
    D3DCOLOR    specular;
} P2SPECULAR, *LPP2SPECULAR;

typedef struct _P2PSIZE
{
    D3DVALUE    psize;
} P2PSIZE, *LPP2PSIZE;

typedef struct _P2FVFOFFSETS
{ 
    DWORD       dwColOffset;
    DWORD       dwSpcOffset;
    DWORD       dwTexOffset;
    DWORD       dwTexBaseOffset;
    DWORD       dwStride;
} P2FVFOFFSETS, *LPP2FVFOFFSETS;

// track appropriate pointers to fvf vertex components
__inline void __SetFVFOffsets (DWORD            *lpdwColorOffs, 
                               DWORD            *lpdwSpecularOffs, 
                               DWORD            *lpdwTexOffs, 
                               LPP2FVFOFFSETS   lpP2FVFOff)
{
    if (lpP2FVFOff == NULL) 
    {   // Default non-FVF case , we just set up everything as for a D3DTLVERTEX
        *lpdwColorOffs    = offsetof( D3DTLVERTEX, color);
        *lpdwSpecularOffs = offsetof( D3DTLVERTEX, specular);
        *lpdwTexOffs      = offsetof( D3DTLVERTEX, tu);
    } 
    else 
    {   // Use the offsets info to setup the corresponding fields
        *lpdwColorOffs    = lpP2FVFOff->dwColOffset;
        *lpdwSpecularOffs = lpP2FVFOff->dwSpcOffset;
        *lpdwTexOffs      = lpP2FVFOff->dwTexOffset;
    }
}

//Size of maximum FVF that we can get. Used for temporary storage
typedef BYTE P2FVFMAXVERTEX[ 3 * sizeof( D3DVALUE ) +    // Position coordinates
                             5 * 4                  +    // D3DFVF_XYZB5
                                 sizeof( D3DVALUE ) +    // FVF_TRANSFORMED
                             3 * sizeof( D3DVALUE ) +    // Normals
                                 sizeof( DWORD )    +    // RESERVED1
                                 sizeof( DWORD )    +    // Diffuse color
                                 sizeof( D3DCOLOR ) +    // Specular color
                                 sizeof( D3DVALUE ) +    // Point sprite size
                             4 * 8 * sizeof( D3DVALUE )  // 8 sets of 4D texture coordinates
                           ];

#define FVFTEX( lpVtx , dwOffs )     ((LPP2TEXCOORDS)((LPBYTE)(lpVtx) + dwOffs))
#define FVFCOLOR( lpVtx, dwOffs )    ((LPP2COLOR)((LPBYTE)(lpVtx) + dwOffs))
#define FVFSPEC( lpVtx, dwOffs)      ((LPP2SPECULAR)((LPBYTE)(lpVtx) + dwOffs))
#define FVFPSIZE( lpVtx, dwOffs)     ((LPP2PSIZE)((LPBYTE)(lpVtx) + dwOffs))


//-----------------------------------------------------------------------------
//                    Conversion, math and culling macros
//-----------------------------------------------------------------------------
/*
 * This loses one bit of accuracy, but adds and clamps without ifs.
 * We first mask all channels with 0xfe.  This leaves the lsb of
 * each channel clear, so when the terms are added, any carry goes
 * into the new highest bit.  Now all we have to do is generate a
 * mask for any channels that have overflowed.  So we shift right
 * and eliminate everything but the overflow bits, so each channel
 * contains either 0x00 or 0x01.  Subtracting each channel from 0x80
 * produces 0x7f or 0x80.  We just shift this left once and mask to
 * give 0xfe or 0x00.  (We could eliminate the final mask here, but
 * it would introduce noise into the low-bit of every channel..)
 */
#define CLAMP8888(result, color, specular) \
     result = (color & 0xfefefefe) + (specular & 0xfefefe); \
     result |= ((0x808080 - ((result >> 8) & 0x010101)) & 0x7f7f7f) << 1;

#define RGB256_TO_LUMA(r,g,b) (float)(((float)r * 0.001172549019608) + \
                                      ((float)g * 0.002301960784314) + \
                                      ((float)b * 0.000447058823529));

#define LONG_AT(flt) (*(long *)(&flt))
#define ULONG_AT(flt) (*(unsigned long *)(&flt))

//Triangle culling macro
#define CULL_TRI(pCtxt,p0,p1,p2)                                         \
    ((pCtxt->CullMode != D3DCULL_NONE) &&                                \
     (((p1->sx - p0->sx)*(p2->sy - p0->sy) <=                            \
       (p2->sx - p0->sx)*(p1->sy - p0->sy)) ?                            \
      (pCtxt->CullMode == D3DCULL_CCW)     :                             \
      (pCtxt->CullMode == D3DCULL_CW) ) )

__inline ULONG RGB888ToHWFmt(ULONG dwRGB888Color, ULONG ColorMask, ULONG RGB888Mask)
{
    unsigned long m;
    int s = 0;

    if (ColorMask)
        for (s = 0, m = ColorMask; !(m & RGB888Mask);  s++)
            m <<= 1;

    return ((dwRGB888Color >> s) & ColorMask);
}


//-----------------------------------------------------------------------------
//                              State Set overrides
//-----------------------------------------------------------------------------

#define IS_OVERRIDE(type)           ((DWORD)(type) > D3DSTATE_OVERRIDE_BIAS)
#define GET_OVERRIDE(type)          ((DWORD)(type) - D3DSTATE_OVERRIDE_BIAS)

#define MAX_STATE                   D3DSTATE_OVERRIDE_BIAS
#define DWORD_BITS                  32
#define DWORD_SHIFT                 5

#define VALID_STATE(type)           ((DWORD)(type) < 2*D3DSTATE_OVERRIDE_BIAS)

typedef struct _D3DStateSet 
{
    DWORD       bits[MAX_STATE >> DWORD_SHIFT];
} D3DStateSet;

#define STATESET_MASK(set, state)       \
        (set).bits[((state) - 1) >> DWORD_SHIFT]

#define STATESET_BIT(state)     (1 << (((state) - 1) & (DWORD_BITS - 1)))

#define STATESET_ISSET(set, state) \
        STATESET_MASK(set, state) & STATESET_BIT(state)

#define STATESET_SET(set, state) \
        STATESET_MASK(set, state) |= STATESET_BIT(state)

#define STATESET_CLEAR(set, state) \
        STATESET_MASK(set, state) &= ~STATESET_BIT(state)

#if D3D_STATEBLOCKS
//-----------------------------------------------------------------------------
//                     State sets structure definitions
//-----------------------------------------------------------------------------
#define FLAG DWORD
#define FLAG_SIZE (8*sizeof(DWORD))

typedef struct _P2StateSetRec 
{
    DWORD               dwHandle;
    DWORD               bCompressed;

    union 
    {
        struct 
        {
            // Stored state block info (uncompressed)
            DWORD       RenderStates[MAX_STATE];
            DWORD       TssStates[D3DTSS_TEXTURETRANSFORMFLAGS+1];

            FLAG        bStoredRS[(MAX_STATE + FLAG_SIZE)/ FLAG_SIZE];
            FLAG        bStoredTSS[(D3DTSS_TEXTURETRANSFORMFLAGS + FLAG_SIZE) / FLAG_SIZE];
        } uc;
        struct 
        {
            // Stored state block info (compressed)
            DWORD       dwNumRS;
            DWORD       dwNumTSS;
            struct 
            {
                DWORD   dwType;
                DWORD   dwValue;
            } pair[1];
        } cc;
    } u;

} P2StateSetRec;

#define SSPTRS_PERPAGE (4096/sizeof(P2StateSetRec *))

#define FLAG_SET(flag, number)     \
    flag[ (number) / FLAG_SIZE ] |= (1 << ((number) % FLAG_SIZE))

#define IS_FLAG_SET(flag, number)  \
    (flag[ (number) / FLAG_SIZE ] & (1 << ((number) % FLAG_SIZE) ))
#endif //D3D_STATEBLOCKS

//------------------------------------------------------------------------------
//from dx95type.h TODO: remove this comment
#define MAX_D3DRENDER_STATE        (D3DRENDERSTATE_CLIPPLANEENABLE + 1) // from d3dtypes.h
#define MAX_D3DTEXTURESTAGESTATE   (D3DTSS_TEXTURETRANSFORMFLAGS + 1)   // from d3dtypes.h


//-----------------------------------------------------------------------------
//                       Context indexing structure
//-----------------------------------------------------------------------------
#define MAX_DDRAWLCL_NUM 200

//-----------------------------------------------------------------------------
//                       Context validation macros
//-----------------------------------------------------------------------------
#define RC_MAGIC_DISABLE 0xd3d00000
#define RC_MAGIC_NO 0xd3d00100

// Defines for the dwDirtyFlags field of our context
#define CONTEXT_DIRTY_ALPHABLEND          2
#define CONTEXT_DIRTY_ZBUFFER             4
#define CONTEXT_DIRTY_TEXTURE             8
#define CONTEXT_DIRTY_MULTITEXTURE       16
#define CONTEXT_DIRTY_RENDERTARGET       32

//-----------------------------------------------------------------------------
//                    Context rendering state tracking
//-----------------------------------------------------------------------------
// Flags to keep track of various rendering states or conditions in a D3D context
// these are tracked in the Hdr.Flags field
#define CTXT_HAS_GOURAUD_ENABLED      (1 << 0)
#define CTXT_HAS_ZBUFFER_ENABLED      (1 << 1)
#define CTXT_HAS_SPECULAR_ENABLED     (1 << 2)
#define CTXT_HAS_FOGGING_ENABLED      (1 << 3)
#define CTXT_HAS_PERSPECTIVE_ENABLED  (1 << 4)
#define CTXT_HAS_TEXTURE_ENABLED      (1 << 5)
#define CTXT_HAS_ALPHABLEND_ENABLED   (1 << 6)
#define CTXT_HAS_MONO_ENABLED         (1 << 7)
#define CTXT_HAS_WRAPU_ENABLED        (1 << 8)
#define CTXT_HAS_WRAPV_ENABLED        (1 << 9)
    // Use the alpha value to calculate a stipple pattern
#define CTXT_HAS_ALPHASTIPPLE_ENABLED (1 << 10)
#define CTXT_HAS_ZWRITE_ENABLED       (1 << 11)
    // Enable last point on lines
#define CTXT_HAS_LASTPIXEL_ENABLED    (1 << 12)



//-----------------------------------------------------------------------------
//                     Context structure definitions
//-----------------------------------------------------------------------------

typedef struct _TextureCacheManager *PTextureCacheManager;

typedef struct _PERMEDIA_D3DCONTEXT 
{
    ULONG                   Flags;
    ULONG                   FillMode;
    P2REG_SOFTWARE_COPY     SoftCopyP2Regs;
    DWORD                   RenderCommand;

    // Stored surface info
    UINT_PTR                RenderSurfaceHandle;
    UINT_PTR                ZBufferHandle;
    ULONG                   ulPackedPP;  
    
    LPP2THUNKEDDATA         pThisDisplay;

    BOOL                    bCanChromaKey;
    BOOL                    bStateValid;    // Is the context valid for rendering?
    DWORD                   dwReloadContext;

    // State
    BOOL                    bKeptStipple;
    DWORD                   LastAlpha;
    BYTE                    CurrentStipple[8];

    DWORD                   RenderStates[MAX_D3DRENDER_STATE];
    DWORD                   TssStates[MAX_D3DTEXTURESTAGESTATE]; // D3D DX6 TSS States
    DWORD                   dwWrap[8]; // D3D DX6 Wrap flags

    DWORD                   dwDirtyFlags;

    // Texture filtering modes
    BOOL                    bMagFilter;     // Filter the magnified textures
    BOOL                    bMinFilter;     // Filter the minified textures

    // Misc. state
    D3DCULL                 CullMode;
    DWORD                   FakeBlendNum;

    // To overide states in rendering
    D3DStateSet             overrides;      
    
    // Texture data & sizes (for Permedia setup) of our CurrentTextureHandle
    FLOAT                   DeltaHeightScale;
    FLOAT                   DeltaWidthScale;
    DWORD                   MaxTextureXi;
    FLOAT                   MaxTextureXf;
    DWORD                   MaxTextureYi;
    FLOAT                   MaxTextureYf;

    DWORD                   CurrentTextureHandle;


#if D3D_STATEBLOCKS
    BOOL                    bStateRecMode;  // Toggle for executing or recording states
    P2StateSetRec           *pCurrSS;       // Ptr to SS currently being recorded
    P2StateSetRec           **pIndexTableSS;// Pointer to table of indexes
    DWORD                   dwMaxSSIndex;   // size of table of indexes
#endif

    DWORD                   PixelOffset;    // Offset in pixels to start of the rendering buffer

    DWORD                   LowerChromaColor;// Last lower bound chroma key for this context
    DWORD                   UpperChromaColor;// Last upper bound chroma key for this context

    PTextureCacheManager    pTextureManager;
    LPDWLIST                pHandleList;
    LPVOID                  pDDLcl;          // Local surface pointer used as a ID
    LPVOID                  NextD3DContext;
} PERMEDIA_D3DCONTEXT ;

// Defines used in the FakeBlendNum field of the P2 D3D context in order to
// make up for missing features in the hw that can be easily simulated
#define FAKE_ALPHABLEND_ONE_ONE     1
#define FAKE_ALPHABLEND_MODULATE    2


//-----------------------------------------------------------------------------
//  One special legacy texture op we can;t easily map into the new texture ops
//-----------------------------------------------------------------------------
#define D3DTOP_LEGACY_ALPHAOVR (0x7fffffff)



//-----------------------------------------------------------------------------
//                          Context switching
//-----------------------------------------------------------------------------
#define SWITCH_TO_CONTEXT(pContext, pThisDisplay) \
{                                           \
    STOP_SOFTWARE_CURSOR(pThisDisplay);     \
    DrawPrim2ContextRegSave(pContext);      \
    DrawPrim2ContextRegInit(pContext);      \
}

#define EXIT_FROM_CONTEXT(pContext, pThisDisplay) \
{                                           \
    START_SOFTWARE_CURSOR(pThisDisplay);    \
}

#define SYNC_AND_EXIT_FROM_CONTEXT(pContext, pThisDisplay) \
{                                           \
    DrawPrim2ContextRegRestore(pContext);   \
    START_SOFTWARE_CURSOR(pThisDisplay);    \
}

#define RELOAD_CONTEXT          0x0007

#endif //#ifndef _DLL_D3DCNTXT_H_
