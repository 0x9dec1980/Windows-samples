/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name:  p2d3dhw.c
*
* Content:      Hardware dependent functions exclusively for Direct3D 
*               Please note that in all the cases we map the Operations
*               into the Permedia2 registers. However we do it only in
*               the software register copy for that context. 
*               The actual registers get loaded when the corresponding 
*               context is activated, which happens when a rendering 
*               operation is performed in that context.
*
*               These functions can be divided into two categories, 
*                   - functions related to the initialization a validated 
*                     rendering context. 
*                   - functions related to state changes within a rendering context.
*               Please note that SetupPermediaRenderTarget falls into both.
*                           
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"


#if DEBUG

void DbgNotHandledRenderStates(DWORD renderState, DWORD dwRSVal);
#define DBG_NOTHANDLEDRENDERSTATES(renderState, dwRSVal) \
                            DbgNotHandledRenderStates(renderState, dwRSVal)
VOID    DecodeBlend( LONG Level, DWORD i );
#define DECODEBLEND(a, b) DecodeBlend(a, b);

#else   //#if DEBUG

#define DBG_NOTHANDLEDRENDERSTATES(renderState, dwRSVal) 
#define DECODEBLEND(a, b)
 
#endif  //#if DEBUG

/*
 * Prototypes of Local Functions
 */
static void 
DisableTexturePermedia(PERMEDIA_D3DCONTEXT* pContext);
static void 
EnableTexturePermedia(PERMEDIA_D3DCONTEXT* pContext);
static HRESULT  
__HWSetupStageStates(PERMEDIA_D3DCONTEXT *pContext, 
                     LPP2FVFOFFSETS      lpP2FVFOff);

/********************************************************************************
 * Functions related to the Initialization of a Validated Rendering Context     *
 ********************************************************************************/

/**
 * Function:    HWInitContext
 * Description: Given a valid context, maps it to the contents of the
 *              appropriate Permedia 2 registers.
 */

VOID 
HWInitContext(PERMEDIA_D3DCONTEXT* pContext)
{
    PP2REG_SOFTWARE_COPY    pSoftPermedia = &pContext->SoftCopyP2Regs;
    LPP2THUNKEDDATA         ppdev = pContext->pThisDisplay;

    DBG_D3D((10,"Entering InitPermediaContext"));

    // Setup initial state of Permedia 2 registers for this D3D context

    //=========================================================================
    // Initialize our software copy of some registers for their default values 
    //=========================================================================

    // Setup the default & constant ( Z Buffer) LB settings
    //  this will be updated into the chip in SetupPermediaRenderTarget
    pSoftPermedia->LBReadMode.WindowOrigin          = __PERMEDIA_TOP_LEFT_WINDOW_ORIGIN;
    pSoftPermedia->LBReadMode.DataType              = __PERMEDIA_LBDEFAULT;     // default
    pSoftPermedia->LBReadMode.ReadSourceEnable      = __PERMEDIA_DISABLE;
    pSoftPermedia->LBReadMode.ReadDestinationEnable = __PERMEDIA_DISABLE;
    pSoftPermedia->LBReadMode.PatchMode             = 0;

    // Setup the default & constant FB settings
    //  this will be updated into the chip in SetupPermediaRenderTarget
    pSoftPermedia->FBReadMode.ReadSourceEnable      = __PERMEDIA_DISABLE;
    pSoftPermedia->FBReadMode.ReadDestinationEnable = __PERMEDIA_DISABLE;
    pSoftPermedia->FBReadMode.DataType              = __PERMEDIA_FBDATA;
                                                    // Top Left for D3D origin
    pSoftPermedia->FBReadMode.WindowOrigin          = __PERMEDIA_TOP_LEFT_WINDOW_ORIGIN;
    pSoftPermedia->FBReadMode.PatchMode             = 0;
    pSoftPermedia->FBReadMode.PackedData            = 0;
    pSoftPermedia->FBReadMode.RelativeOffset        = 0;

    // Setup the default & constant Alpha Blend Mode settings
    //  this will be updated into the chip in SetupPermediaRenderTarget
    pSoftPermedia->AlphaBlendMode.AlphaBlendEnable  = 0;
    pSoftPermedia->AlphaBlendMode.SourceBlend       = __PERMEDIA_BLEND_FUNC_ONE;
    pSoftPermedia->AlphaBlendMode.DestinationBlend  = __PERMEDIA_BLEND_FUNC_ZERO;
    pSoftPermedia->AlphaBlendMode.NoAlphaBuffer     = 0;
    pSoftPermedia->AlphaBlendMode.ColorOrder        = COLOR_MODE;
    pSoftPermedia->AlphaBlendMode.BlendType         = 0;
    pSoftPermedia->AlphaBlendMode.ColorConversion   = 1;
    pSoftPermedia->AlphaBlendMode.AlphaConversion   = 1;

    // Setup the default & constant  Dither Mode settings
    //  this will be updated into the chip in SetupPermediaRenderTarget
    pSoftPermedia->DitherMode.ColorOrder            = COLOR_MODE;
    pSoftPermedia->DitherMode.XOffset               = DITHER_XOFFSET;
    pSoftPermedia->DitherMode.YOffset               = DITHER_YOFFSET;
    pSoftPermedia->DitherMode.UnitEnable            = __PERMEDIA_ENABLE;
    pSoftPermedia->DitherMode.ForceAlpha            = 0;

    //=========================================================================
    //  Find out info for memory widths
    //=========================================================================

    pContext->ulPackedPP = PARTIAL_PRODUCTS(ppdev->cxMemory);

    DBG_D3D((4, "ScreenWidth %d, ScreenHeight %d, Bytes/Pixel %d PackedPP = %04x",
                ppdev->cxMemory, ppdev->cyMemory, 
                ppdev->ddpfDisplay.dwRGBBitCount >> 3, pContext->ulPackedPP));
    //=========================================================================
    // Initialize hardware registers to their default values 
    //=========================================================================
    // ----------------- Render and Depth Buffer setup ----------------------

    // Render buffer Write Mode setup
    pSoftPermedia->FBWriteMode.UnitEnable   = __PERMEDIA_ENABLE;

    // Render buffer Write Masks (write to all bits in the pixel)
    pSoftPermedia->FBSoftwareWriteMask      = __PERMEDIA_ALL_WRITEMASKS_SET;
    pSoftPermedia->FBHardwareWriteMask      = __PERMEDIA_ALL_WRITEMASKS_SET;

    // Depth Buffer Write mode (initially allow LB Writes)
    pSoftPermedia->LBWriteMode.WriteEnable          = __PERMEDIA_DISABLE;

    // Depth comparisons
    pSoftPermedia->DepthMode.WriteMask              = __PERMEDIA_ENABLE;
    pSoftPermedia->DepthMode.CompareMode            =
                                __PERMEDIA_DEPTH_COMPARE_MODE_LESS_OR_EQUAL;
    pSoftPermedia->DepthMode.NewDepthSource         = __PERMEDIA_DEPTH_SOURCE_DDA;
    pSoftPermedia->DepthMode.UnitEnable             = __PERMEDIA_DISABLE;


    // ----------------- Texture units setup -----------------------------

    // Enable texture address unit, disable perspective correction
    pSoftPermedia->TextureAddressMode.Enable                = 1;
    pSoftPermedia->TextureAddressMode.PerspectiveCorrection = 0;
    pSoftPermedia->TextureAddressMode.DeltaFormat           = 0;

    // Enable texture color mode unit, set modulation blending, no specular
    // as defaults
    pSoftPermedia->TextureColorMode.TextureEnable   = 1;
    pSoftPermedia->TextureColorMode.ApplicationMode = _P2_TEXTURE_MODULATE;
    pSoftPermedia->TextureColorMode.TextureType     = 0;
    pSoftPermedia->TextureColorMode.KdDDA           = 0;
    pSoftPermedia->TextureColorMode.KsDDA           = 0;

    // Enable texture mapping unit, set frame buffer size as default texture
    // map size (to be oevrriden in EnableTexturePermedia)
    pSoftPermedia->TextureMapFormat.PackedPP        = pContext->ulPackedPP;
    pSoftPermedia->TextureMapFormat.WindowOrigin    =
                                __PERMEDIA_TOP_LEFT_WINDOW_ORIGIN; //top left
    pSoftPermedia->TextureMapFormat.SubPatchMode    = 0;
    pSoftPermedia->TextureMapFormat.TexelSize       = 1;

    // Setup Textura data format (to be oevrriden in EnableTexturePermedia)
    pSoftPermedia->TextureDataFormat.TextureFormat  = 1;
    pSoftPermedia->TextureDataFormat.NoAlphaBuffer  = 1;
    pSoftPermedia->TextureDataFormat.ColorOrder     = COLOR_MODE;

    // Setup default texture map base address (in video memory)
    pSoftPermedia->TextureBaseAddress.Addr          = 0;
    pSoftPermedia->TextureBaseAddress.Access        = 0;

    // Setup texture reading defaults: Repeat s,t wrapping, 256x256 texture
    // no texture filtering set up.
    pSoftPermedia->TextureReadMode.PackedData       = 0;
    pSoftPermedia->TextureReadMode.FilterMode       = 0;
    pSoftPermedia->TextureReadMode.Height           = 8;
    pSoftPermedia->TextureReadMode.Width            = 8;
    pSoftPermedia->TextureReadMode.pad1             = 0;
    pSoftPermedia->TextureReadMode.pad2             = 0;
    pSoftPermedia->TextureReadMode.TWrapMode        = _P2_TEXTURE_REPEAT;
    pSoftPermedia->TextureReadMode.SWrapMode        = _P2_TEXTURE_REPEAT;
    pSoftPermedia->TextureReadMode.Enable           = 1;

    // Disable Texture LUT unit for palettized textures
    //pSoftPermedia->TexelLUTMode.Enable              = __PERMEDIA_DISABLE;

    // -------------- Other rendering units setup ----------------

    // Setup defaults of YUV units used for chromakey testing
    pSoftPermedia->YUVMode.Enable                   = __PERMEDIA_DISABLE;
    pSoftPermedia->YUVMode.TestMode                 = PM_YUVMODE_CHROMATEST_DISABLE;
    pSoftPermedia->YUVMode.TestData                 = PM_YUVMODE_TESTDATA_INPUT;
    pSoftPermedia->YUVMode.RejectTexel              = FALSE;
    pSoftPermedia->YUVMode.TexelDisableUpdate       = FALSE;

    // Chromakey values initially black
    pSoftPermedia->dwChromaUpperBound   = 0x00000000;
    pSoftPermedia->dwChromaLowerBound   = 0x00000000;
    pSoftPermedia->dwAlphaMapUpperBound = 0xFFFFFFFF;
    pSoftPermedia->dwAlphaMapLowerBound = 0x11000000;

    // Default Fog color is white
    pSoftPermedia->FogColor             = 0xFFFFFFFF;

    // Fog setup
    pSoftPermedia->FogMode.FogEnable    = 1;

    // Stencil mode setup
    pSoftPermedia->StencilMode.DPFail           = __PERMEDIA_STENCIL_METHOD_KEEP;
    pSoftPermedia->StencilMode.DPPass           = __PERMEDIA_STENCIL_METHOD_KEEP;
    pSoftPermedia->StencilMode.UnitEnable       = __PERMEDIA_DISABLE;
    pSoftPermedia->StencilMode.StencilSource    =
                                        __PERMEDIA_STENCIL_SOURCE_TEST_LOGIC;

    // Host out unit , disable read backs
    pSoftPermedia->dwFilterMode                         = __PERMEDIA_DISABLE;

    // ----------------- Rasterization setup -----------------------------

    // Setup Rasterizer units defaults
    pSoftPermedia->dwRasterizerMode                     = 0;

    // Setup a step of -1, as this doesn't change very much
    pSoftPermedia->dY                                   = 0xFFFF0000;

    // Setup for Gourand shaded colour model, and enable unit
    pSoftPermedia->ColorDDAMode.UnitEnable              = 1;
    pSoftPermedia->ColorDDAMode.ShadeMode               = 1;

    // Disable stippling unit
    pSoftPermedia->dwAreaStippleMode                    = 0x00;

    // Setup the Delta setup chip for rasterization
    pSoftPermedia->DeltaMode.TargetChip                 = 2;
    pSoftPermedia->DeltaMode.SpecularTextureEnable      = 0;
    // The below changes to normalize in the perspective case
    // It must not be on in the non-perspective case as the bad Q's will
    // get used in the normalisation.
    pSoftPermedia->DeltaMode.TextureParameterMode       = 1;
    pSoftPermedia->DeltaMode.TextureEnable              = 1;
    pSoftPermedia->DeltaMode.DiffuseTextureEnable       = 0;

    pSoftPermedia->DeltaMode.FogEnable                  = 1;
    pSoftPermedia->DeltaMode.SmoothShadingEnable        = 1;
    pSoftPermedia->DeltaMode.DepthEnable                = 0;
    pSoftPermedia->DeltaMode.SubPixelCorrectionEnable   = 1;
    pSoftPermedia->DeltaMode.DiamondExit                = 1;
    pSoftPermedia->DeltaMode.NoDraw                     = 0;
    pSoftPermedia->DeltaMode.ClampEnable                = 0;
    pSoftPermedia->DeltaMode.FillDirection              = 0;
#ifndef P2_CHIP_CULLING
    pSoftPermedia->DeltaMode.BackfaceCull               = 0;
#else
    pSoftPermedia->DeltaMode.BackfaceCull               = 1;
#endif
    pSoftPermedia->DeltaMode.ColorOrder                 = COLOR_MODE;

    DBG_D3D((10,"Exiting InitPermediaContext"));

} // HWInitContext



/**
 * Function:    SetupPermediaRenderTarget
 * Description: Sets up the correct surface characteristics (format, stride, etc) 
 *              of the render buffer and the depth buffer in the Permedia 
 *              registers software copy
 */

VOID 
SetupPermediaRenderTarget(PERMEDIA_D3DCONTEXT* pContext)
{
    PPERMEDIA_D3DTEXTURE    pSurfRender, pSurfZBuffer;
    P2_SURFACE_FORMAT       P2RenderSurfaceFormat;
    PP2REG_SOFTWARE_COPY    pSoftPermedia   = &pContext->SoftCopyP2Regs;
    LPP2THUNKEDDATA         ppdev           = pContext->pThisDisplay;
    
    DBG_D3D((4,"Entering SetupPermediaRenderTarget"));

    pSurfRender = 
        TextureHandleToPtr(pContext->RenderSurfaceHandle, pContext);

    ASSERTDD((pSurfRender), "SetupPermediaRenderTarget: invalid RenderSurface !");

    pSoftPermedia->FBReadMode.PackedPP = PARTIAL_PRODUCTS(pSurfRender->wWidth);
    pContext->PixelOffset = pSoftPermedia->dwFBPixelOffset = 
        (DWORD)(((UINT_PTR)(pSurfRender->fpVidMem) - ppdev->dwScreenFlatAddr)>>(pSurfRender->ddpfSurface.dwRGBBitCount>>4));

    // Record the surface information
    // If there is a Z Buffer, then we must setup the Partial products to be
    // the same as those chosen when it was allocated.

    if (0 != pContext->ZBufferHandle)
    {
        pSurfZBuffer = 
            TextureHandleToPtr(pContext->ZBufferHandle, pContext);

        ASSERTDD((pSurfZBuffer), "SetupPermediaRenderTarget: invalid pSurfZBuffer !");

        pSoftPermedia->LBReadMode.PackedPP = PARTIAL_PRODUCTS(pSurfZBuffer->wWidth);
        
        //actually check dwStencilBitMask 
        if (0==pSurfZBuffer->ddpfSurface.dwBBitMask)
        {
            pSoftPermedia->LBReadFormat.DepthWidth = 0;                 // 16 bits
            pSoftPermedia->LBReadFormat.StencilWidth = 0;               // No Stencil
            pSoftPermedia->DeltaMode.DepthFormat = 1;   //PM_DELTAMODE_DEPTHWIDTH_16
        }
        else
        {
            pSoftPermedia->LBReadFormat.DepthWidth = 3;                 // 15 bits
            pSoftPermedia->LBReadFormat.StencilWidth = 3;               // 1 Stencil
            pSoftPermedia->DeltaMode.DepthFormat = 0;   //PM_DELTAMODE_DEPTHWIDTH_15
        }
        pSoftPermedia->LBWriteFormat = pSoftPermedia->LBReadFormat;
        pSoftPermedia->LBWindowBase.Addr = (DWORD)(((UINT_PTR)pSurfZBuffer->fpVidMem - ppdev->dwScreenFlatAddr)>> 1);
    }
    else
    {   // No Z Buffer, just stuff the same Partial products as the desktop.
        pSoftPermedia->LBReadMode.PackedPP = pContext->ulPackedPP;
    }

    GetP2SurfaceFormat(&pSurfRender->ddpfSurface, &P2RenderSurfaceFormat);

    // DitherMode and AlphaBlendMode both depend on the surface pixel format
    // being correct.
    pSoftPermedia->DitherMode.ColorFormat =
    pSoftPermedia->AlphaBlendMode.ColorFormat = P2RenderSurfaceFormat.dwP2Format;
    pSoftPermedia->DitherMode.ColorFormatExtension = 
    pSoftPermedia->AlphaBlendMode.ColorFormatExtension =
                        P2RenderSurfaceFormat.dwP2FormatExt;
    pSoftPermedia->FBReadPixel = P2RenderSurfaceFormat.dwP2PixelSize;

    DBG_D3D((4," \t FBReadMode      = 0x%x", pSoftPermedia->dwFBReadMode));
    DBG_D3D((4," \t DeltaMode       = 0x%x", pSoftPermedia->dwDeltaMode));
    DBG_D3D((4," \t LBWriteFormat   = 0x%x", pSoftPermedia->dwLBWriteFormat));
    DBG_D3D((4," \t LBWindowBase    = 0x%x", pSoftPermedia->dwLBWindowBase));
    DBG_D3D((4," \t LBReadMode      = 0x%x", pSoftPermedia->dwLBReadMode));
    DBG_D3D((4," \t LBReadFormat    = 0x%x", pSoftPermedia->dwLBReadFormat));
    DBG_D3D((4," \t DitherMode      = 0x%x", pSoftPermedia->dwDitherMode));
    DBG_D3D((4," \t AlphaBlendMode  = 0x%x", pSoftPermedia->dwAlphaBlendMode));
    DBG_D3D((4," \t FBReadPixel     = 0x%x", pSoftPermedia->FBReadPixel));
    // The AlphaBlending may need to be changed.
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHABLEND;

    // Dirty the Z Buffer (the new target may not have one)
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
    pContext->dwDirtyFlags &= ~CONTEXT_DIRTY_RENDERTARGET;

    DBG_D3D((4,"Exiting SetupPermediaRenderTarget"));
} // SetupPermediaRenderTarget

/********************************************************************************
 * Functions related to the related to state changes within a Rendering Context *
 ********************************************************************************/

/*
 *
 * Function:    DisableTexturePermedia
 * Description: Disable texturing in Permedia2
 *
 */

static void 
DisableTexturePermedia(PERMEDIA_D3DCONTEXT* pContext)
{
    DWORD                   *pFlags         = &pContext->Flags;
    PERMEDIA_D3DTEXTURE     *pTexture       = NULL;
    PP2REG_SOFTWARE_COPY    pSoftPermedia  = &pContext->SoftCopyP2Regs;
    LPP2THUNKEDDATA         pThisDisplay    = pContext->pThisDisplay;

    DBG_D3D((10,"Entering DisableTexturePermedia"));

    pContext->FakeBlendNum &= ~FAKE_ALPHABLEND_MODULATE;
    
    // The textures have been turned off, so...
    ASSERTDD(pContext->CurrentTextureHandle == 0,
        "DisableTexturePermedia expected zero texture handle");

    DBG_D3D((4, "Disabling Texturing"));
    
    // Turn off texture address generation
    pSoftPermedia->TextureAddressMode.Enable = 0;
    // Turn off texture reads
    pSoftPermedia->TextureReadMode.Enable = 0;
    
    // Turn off textures
    pSoftPermedia->TextureColorMode.TextureEnable = 0;

    // Set the texture base address to 0
    // (turning off the 'AGP' bit in the process)
    // Also stop TexelLUTTransfer messages
    pSoftPermedia->dwTextureBaseAddress = 0;
    //pSoftPermedia->dwTexelLUTTransfer   = 0;

    // Set current texture to 0
    pContext->CurrentTextureHandle = 0;
    *pFlags &= ~CTXT_HAS_TEXTURE_ENABLED;
    RENDER_TEXTURE_DISABLE(pContext->RenderCommand);
    
    // If textures were in copy mode, we may have fiddled with the DDA,
    // to improve performance.
    if ((unsigned int)pSoftPermedia->TextureColorMode.ApplicationMode ==
        _P2_TEXTURE_COPY) 
    {
        if (*pFlags & CTXT_HAS_GOURAUD_ENABLED) 
        {
            pSoftPermedia->DeltaMode.SmoothShadingEnable = 1;
            pSoftPermedia->ColorDDAMode.UnitEnable = 1;
        }
        else 
        {
            pSoftPermedia->DeltaMode.SmoothShadingEnable = 0;
            pSoftPermedia->ColorDDAMode.UnitEnable = 1;
        }
    }

    if (pContext->bCanChromaKey == TRUE) 
    {
        // Turn off Chroma Keying
        
        pSoftPermedia->YUVMode.TestMode = PM_YUVMODE_CHROMATEST_DISABLE;
        pSoftPermedia->YUVMode.Enable = __PERMEDIA_DISABLE;
        pContext->bCanChromaKey = FALSE;
    }

    pContext->bStateValid = TRUE;

    DBG_D3D((10,"Exiting DisableTexturePermedia"));

    return;
} // DisableTexturePermedia

/*
 *
 * Function:    EnableTexturePermedia
 * Description: Enable texturing in Permedia2
 *
 */

static void 
EnableTexturePermedia(PERMEDIA_D3DCONTEXT* pContext)
{
    DWORD                   *pFlags         = &pContext->Flags;
    PERMEDIA_D3DTEXTURE     *pTexture       = NULL;
    PP2REG_SOFTWARE_COPY    pSoftPermedia  = &pContext->SoftCopyP2Regs;
    LPP2THUNKEDDATA         pThisDisplay    = pContext->pThisDisplay;
    PERMEDIA_D3DPALETTE     *pPalette       = NULL;
    LPPALETTEENTRY          lpColorTable    = NULL;// array of palette entries
    PP2_MANAGED_TEXTURE     pP2ManagedTexture;
    P2_SURFACE_FORMAT       P2SurfaceFormat;

    DBG_D3D((10,"Entering EnableTexturePermedia"));

    pContext->FakeBlendNum &= ~FAKE_ALPHABLEND_MODULATE;

    // A texture has been turned on so ...
    ASSERTDD(pContext->CurrentTextureHandle != 0,
        "EnableTexturePermedia expected non zero texture handle");

    // We must be texturing so...
    pTexture = TextureHandleToPtr(pContext->CurrentTextureHandle, pContext);
    
    if (pTexture) 
    {
        DWORD cop = pContext->TssStates[D3DTSS_COLOROP];
        DWORD ca1 = pContext->TssStates[D3DTSS_COLORARG1];
        DWORD ca2 = pContext->TssStates[D3DTSS_COLORARG2];
        DWORD aop = pContext->TssStates[D3DTSS_ALPHAOP];
        DWORD aa1 = pContext->TssStates[D3DTSS_ALPHAARG1];

        // Current is the same as diffuse in stage 0
        if (ca2 == D3DTA_CURRENT)
            ca2 = D3DTA_DIFFUSE;

        GetP2SurfaceFormat(&pTexture->ddpfSurface,&P2SurfaceFormat);

        if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
        {
            pP2ManagedTexture = &pTexture->P2ManagedTexture;

            if ((FLATPTR)NULL==pP2ManagedTexture->fpVidMem)
            {
                DBG_D3D((0,"EnableTexturePermedia: TextureCacheManagerAllocNode: surfaceHandle=0x%x lpDDLcl=0x%x",
                                pContext->CurrentTextureHandle, pContext->pDDLcl));
                TextureCacheManagerAllocNode(pContext,pTexture);
                if ((FLATPTR)NULL==pP2ManagedTexture->fpVidMem)
                {
                    DBG_D3D((0,"EnableTexturePermedia unable to allocate memory from heap"));
                    pContext->CurrentTextureHandle = 0;
                    if ((pTexture) && (!(pTexture->dwCaps & DDSCAPS_VIDEOMEMORY))) //i.e., System Memory 
                        pContext->bStateValid = FALSE;
                    else 
                        pContext->bStateValid = TRUE;
                    return;
                }
                pP2ManagedTexture->dwFlags |= P2_SURFACE_NEEDUPDATE;
            }
            TextureCacheManagerTimeStamp(pContext->pTextureManager,pTexture);
            if (pP2ManagedTexture->dwFlags & P2_SURFACE_NEEDUPDATE)
            {
                LoadManagedTextureToVidMem(pThisDisplay, pTexture);
            }
        }        
        // If it is a palette indexed texture, we simply follow the chain
        // down from the surface to it's palette and pull out the LUT values
        // from the PALETTEENTRY's in the palette.
        if (pTexture->ddpfSurface.dwRGBBitCount == 8) 
        {
            pPalette = 
                    PaletteHandleToPtr(pTexture->dwPaletteHandle,pContext);
            if (NULL != pPalette)
            {
                //some apps are not setting their alpha correctly with palette
                //then it's up to palette to tell us
                pTexture->bAlpha = pPalette->dwFlags & DDRAWIPAL_ALPHA;
            }
        }

        if ((ca2 == D3DTA_DIFFUSE && ca1 == D3DTA_TEXTURE) &&
             cop == D3DTOP_MODULATE &&
             (aa1 == D3DTA_TEXTURE && aop == D3DTOP_LEGACY_ALPHAOVR)) 
        {
            // if this is legacy modulation then we take the texture alpha
            // only if the texure format has it
            if (pTexture->bAlpha)
                pContext->FakeBlendNum |= FAKE_ALPHABLEND_MODULATE;
        }
        else if ((ca2 == D3DTA_DIFFUSE && ca1 == D3DTA_TEXTURE) &&
             cop == D3DTOP_MODULATE &&
             (aa1 == D3DTA_TEXTURE && aop == D3DTOP_SELECTARG1)) 
        {
            // if this is DX6 modulation then we take the texture alpha
            // no matter what ( it will be xFF if it doesn't exist)
            pContext->FakeBlendNum |= FAKE_ALPHABLEND_MODULATE;
        }

        // Enable Texture Address calculation
        pSoftPermedia->TextureAddressMode.Enable = 1;
            
        // Enable Textures
        pSoftPermedia->TextureColorMode.TextureEnable = 1;
        if (*pFlags & CTXT_HAS_SPECULAR_ENABLED)
        {
            pSoftPermedia->DeltaMode.SpecularTextureEnable = 1;
            pSoftPermedia->TextureColorMode.KsDDA = 1; 
            pSoftPermedia->TextureColorMode.ApplicationMode |= _P2_TEXTURE_SPECULAR;
        } 
        else 
        {
            pSoftPermedia->DeltaMode.SpecularTextureEnable = 0;
            pSoftPermedia->TextureColorMode.KsDDA = 0; 
            pSoftPermedia->TextureColorMode.ApplicationMode &= ~_P2_TEXTURE_SPECULAR;
        }

        // Set Partial products for texture (assume no mipmapping).
        pSoftPermedia->TextureMapFormat.PackedPP = pTexture->MipLevels[0].ulPackedPP;

        pSoftPermedia->TextureMapFormat.TexelSize = P2SurfaceFormat.dwP2PixelSize;

        pSoftPermedia->TextureMapFormat.SubPatchMode = 0;


        // Set texture size
        DBG_D3D((4,"     Texture Width: 0x%x", 
                 pTexture->MipLevels[0].logWidth));
        DBG_D3D((4,"     Texture Height: 0x%x", 
                 pTexture->MipLevels[0].logHeight));

        pSoftPermedia->TextureReadMode.Width = 
                                       pTexture->MipLevels[0].logWidth;
        pSoftPermedia->TextureReadMode.Height = 
                                       pTexture->MipLevels[0].logHeight;

        pSoftPermedia->TextureReadMode.Enable = 1;
        pContext->DeltaWidthScale = (float)pTexture->wWidth * (1 / 2048.0f);
        pContext->DeltaHeightScale = (float)pTexture->wHeight * (1 / 2048.0f);

        pContext->MaxTextureXf = (float)(2048 / pTexture->wWidth);
        pContext->MaxTextureYf = (float)(2048 / pTexture->wHeight);

        myFtoui(&pContext->MaxTextureXi, pContext->MaxTextureXf);
        pContext->MaxTextureXi -= 1;
        myFtoui(&pContext->MaxTextureYi, pContext->MaxTextureYf);
        pContext->MaxTextureYi -= 1;

        *pFlags |= CTXT_HAS_TEXTURE_ENABLED;
        RENDER_TEXTURE_ENABLE(pContext->RenderCommand);
        
        DBG_D3D((4,"     Texture Format: 0x%x", P2SurfaceFormat.dwP2Format));
        DBG_D3D((4,"     Texture Format Extension: 0x%x", P2SurfaceFormat.dwP2FormatExt));

        pSoftPermedia->TextureDataFormat.TextureFormat = P2SurfaceFormat.dwP2Format;
        pSoftPermedia->TextureDataFormat.TextureFormatExtension = 
                                                        P2SurfaceFormat.dwP2FormatExt;

        if (pTexture->bAlpha) 
        {
            pSoftPermedia->TextureDataFormat.NoAlphaBuffer = 0;
        } 
        else 
        {
            pSoftPermedia->TextureDataFormat.NoAlphaBuffer = 1;
        }

        // If we are copying textures, there is no need for colour data
        // to be generated, so we turn off the DDA
        if (((unsigned int)pSoftPermedia->TextureColorMode.ApplicationMode) == 
                                                              _P2_TEXTURE_COPY)
        {
            pSoftPermedia->ColorDDAMode.UnitEnable = 0;
            DBG_D3D((4, "    Disabling DDA"));
        }
        else
        {
            pSoftPermedia->ColorDDAMode.UnitEnable = 1;
            DBG_D3D((4, "    Enabling DDA"));
        }
        
        // Load the texture base address BEFORE the TexelLUTTransfer message 
        // to ensure we load the LUT from the right sort of memory (AGP or not)
        // Always set the base address at the root texture (not the miplevels 
        // if there are any)
        DBG_D3D((4, "Setting texture base address to 0x%08X", 
                 pTexture->MipLevels[0].PixelOffset));

        pSoftPermedia->dwTextureBaseAddress = pTexture->MipLevels[0].PixelOffset;

        // If it is a palette indexed texture, we simply follow the chain
        // down from the surface to it's palette and pull out the LUT values
        // from the PALETTEENTRY's in the palette.
        if (P2SurfaceFormat.dwP2Format == PERMEDIA_8BIT_PALETTEINDEX) 
        {

            if (NULL != pPalette)
            {
                int i;
                lpColorTable = pPalette->ColorTable;
                

                SWITCH_TO_CONTEXT(pContext, pThisDisplay);
                if (pPalette->dwFlags & DDRAWIPAL_ALPHA)
                {
                    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 129);
                    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTIndex, 0);
                    for (i = 0; i < 128; i++)
                    {
                        LOAD_INPUT_FIFO_TAG_DATA(   pThisDisplay, 
                                                    TexelLUTData, 
                                                    *(DWORD*)lpColorTable);
                        lpColorTable++;
                    }
                    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 129);
                    for (i = 128; i < 256; i++)
                    {
                        LOAD_INPUT_FIFO_TAG_DATA(   pThisDisplay, 
                                                    TexelLUTData, 
                                                    *(DWORD*)lpColorTable);
                        lpColorTable++;
                    }
                    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTIndex, 0);
                }
                else
                {
                    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 129);
                    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTIndex, 0);
                    for (i = 0; i < 128; i++)
                    {
                        LOAD_INPUT_FIFO_TAG_DATA(   pThisDisplay, 
                                                    TexelLUTData, 
                                                    CHROMA_UPPER_ALPHA(*(DWORD*)lpColorTable));
                        lpColorTable++;
                    }
                    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 129);
                    for (i = 128; i < 256; i++)
                    {
                        LOAD_INPUT_FIFO_TAG_DATA(   pThisDisplay, 
                                                    TexelLUTData, 
                                                    CHROMA_UPPER_ALPHA(*(DWORD*)lpColorTable));
                        lpColorTable++;
                    }
                    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTIndex, 0);
                }
                SYNC_AND_EXIT_FROM_CONTEXT(pContext, pThisDisplay);    

                pSoftPermedia->dwTexelLUTMode = __PERMEDIA_ENABLE;

                DBG_D3D((4,"Texel LUT pPalette->dwFlags=%08lx bAlpha=%d", 
                    pPalette->dwFlags,pTexture->bAlpha));

                pSoftPermedia->dwTexelLUTIndex = 0;
            }
            else
            {
                DBG_D3D((0, "NULL == pPalette in EnableTexturePermedia"
                    "dwPaletteHandle=%08lx dwSurfaceHandle=%08lx",
                    pTexture->dwPaletteHandle,
                    pContext->CurrentTextureHandle)); 
            }
        } 
        else
        {
            pSoftPermedia->dwTexelLUTMode = __PERMEDIA_DISABLE;
        }

        if ((pTexture->dwFlags & DDRAWISURF_HASCKEYSRCBLT)
            && (pContext->RenderStates[D3DRENDERSTATE_COLORKEYENABLE])) 
        {
            DWORD LowerBound = pTexture->dwKeyLow;
            DWORD UpperBound = pTexture->dwKeyHigh;
            DWORD dwLowIndexColor;

            pContext->bCanChromaKey = TRUE;
            
            DBG_D3D((4,"    Can Chroma Key the texture"));
            // Enable Chroma keying for the texture
            // ..and set the correct colour

            // Evaluate the new chroma key value.  Shouldn't be too expensive,
            // as it is only bit shifts and a couple of tests.
            // We also change only when the texture map has changed.
            DBG_D3D((4, "dwColorSpaceLow = 0x%08X", LowerBound));
            DBG_D3D((4, "dwColorSpaceHigh = 0x%08X", UpperBound));

            if (NULL != pPalette) 
            {
                //if (pPrivateData->SurfaceFormat.Format == PERMEDIA_4BIT_PALETTEINDEX)
                //{
                //    LowerBound &= 0x0F;
                //}
                //else
                {
                    LowerBound &= 0xFF;
                }
                lpColorTable = pPalette->ColorTable;

                // ChromaKeying for 4/8 Bit textures is done on the looked up 
                // color, not the index. This means using a range is 
                // meaningless and we have to lookup the color from the 
                // palette.  Make sure the user doesn't force us to access 
                // invalid memory.
                dwLowIndexColor = *(DWORD*)(&lpColorTable[LowerBound]);
                if (pPalette->dwFlags & DDRAWIPAL_ALPHA)
                {
                    LowerBound = UpperBound = dwLowIndexColor;
                }
                else
                {
                    LowerBound = CHROMA_LOWER_ALPHA(dwLowIndexColor);
                    UpperBound = CHROMA_UPPER_ALPHA(dwLowIndexColor);
                }
                DBG_D3D((4,"PaletteHandle=%08lx Lower=%08lx ChromaColor=%08lx"
                    "lpColorTable=%08lx dwFlags=%08lx",
                    pTexture->dwPaletteHandle, LowerBound, dwLowIndexColor,
                    lpColorTable, pPalette->dwFlags));
            }
            else 
            {
                GetP2ChromaLevelBounds( &P2SurfaceFormat, 
                                        &LowerBound, 
                                        &UpperBound);
            }

            DBG_D3D((4,"LowerBound Selected: 0x%x", LowerBound));
            DBG_D3D((4,"UpperBound Selected: 0x%x", UpperBound));

            pSoftPermedia->TextureDataFormat.AlphaMap =  
                                          PM_TEXDATAFORMAT_ALPHAMAP_DISABLE;
            pSoftPermedia->dwChromaUpperBound = UpperBound;
            pSoftPermedia->dwChromaLowerBound = LowerBound;

            pSoftPermedia->YUVMode.TestMode = 
                                           PM_YUVMODE_CHROMATEST_FAILWITHIN;
        }
        else
        {
            DBG_D3D((2,"    Can't Chroma Key the texture"));
            pContext->bCanChromaKey = FALSE;
            pSoftPermedia->TextureDataFormat.AlphaMap = __PERMEDIA_DISABLE;
            pSoftPermedia->YUVMode.TestMode = PM_YUVMODE_CHROMATEST_DISABLE;
        }
        

        // Restore the filter mode from the mag filter.
        if (pContext->bMagFilter) 
        {
            pSoftPermedia->TextureReadMode.FilterMode = 1;
        }
        else 
        {
            pSoftPermedia->TextureReadMode.FilterMode = 0;
        }

        // If the texture is a YUV texture we need to change the color order
        // of the surface and turn on the YUV->RGB conversoin
        if ((pTexture->ddpfSurface.dwFlags & DDPF_FOURCC)&&
            (pTexture->ddpfSurface.dwFourCC == FOURCC_YUV422))
        {
            pSoftPermedia->YUVMode.Enable = __PERMEDIA_ENABLE;
            pSoftPermedia->TextureDataFormat.ColorOrder = INV_COLOR_MODE;
        }
        else 
        {
            pSoftPermedia->YUVMode.Enable = __PERMEDIA_DISABLE;
            pSoftPermedia->TextureDataFormat.ColorOrder = COLOR_MODE;
        }   
    }
    else 
    {
        DBG_D3D((0,"Invalid Texture handle (%d)!, doing nothing", 
                 pContext->CurrentTextureHandle));
        pContext->CurrentTextureHandle = 0;
    }

    if ((pTexture) && (!(pTexture->dwCaps & DDSCAPS_VIDEOMEMORY)))  //In System Memory
        pContext->bStateValid = FALSE;
    else 
        pContext->bStateValid = TRUE;

    DBG_D3D((10,"Exiting EnableTexturePermedia"));

} // EnableTexturePermedia



/*
 * Function:    SetupP2RenderStates
 * Description: Handle a single render state change
 */

void
SetupP2RenderStates(PERMEDIA_D3DCONTEXT   *pContext,
                    DWORD                 dwRSType,
                    DWORD                 dwRSVal)
{
    PP2REG_SOFTWARE_COPY    pSoftPermedia   = &pContext->SoftCopyP2Regs;
    DWORD                   *pFlags         = &pContext->Flags;
    LPP2THUNKEDDATA         ppdev           = pContext->pThisDisplay;

    DBG_D3D((10,"Entering SetupP2RenderStates"));

    // Process each specific renderstate
    switch (dwRSType) 
    {

        case D3DRENDERSTATE_TEXTUREMAPBLEND:
            DBG_D3D((8, "ChangeState: Texture Blend Mode 0x%x "
                                      "(D3DTEXTUREBLEND)", dwRSVal));
            switch (dwRSVal) 
            {
                case D3DTBLEND_DECALALPHA:
                    pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_DECAL;
                break;
                case D3DTBLEND_MODULATE:
                    pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
                break;
                case D3DTBLEND_MODULATEALPHA:
                    pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
                break;
                case D3DTBLEND_COPY:
                case D3DTBLEND_DECAL:
                    pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_COPY;
                break;
            }

            pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;          // May need to change DDA
        break;

        case D3DRENDERSTATE_TEXTUREADDRESS:
            DBG_D3D((8, "ChangeState: Texture address 0x%x "
                        "(D3DTEXTUREADDRESS)", dwRSVal));
            switch (dwRSVal) 
            {
                case D3DTADDRESS_CLAMP:
                    pSoftPermedia->TextureReadMode.TWrapMode =
                                                      _P2_TEXTURE_CLAMP;
                    pSoftPermedia->TextureReadMode.SWrapMode =
                                                      _P2_TEXTURE_CLAMP;
                break;
                case D3DTADDRESS_WRAP:
                    pSoftPermedia->TextureReadMode.TWrapMode =
                                                      _P2_TEXTURE_REPEAT;
                    pSoftPermedia->TextureReadMode.SWrapMode =
                                                      _P2_TEXTURE_REPEAT;
                break;
                case D3DTADDRESS_MIRROR:
                    pSoftPermedia->TextureReadMode.TWrapMode =
                                                      _P2_TEXTURE_MIRROR;
                    pSoftPermedia->TextureReadMode.SWrapMode =
                                                      _P2_TEXTURE_MIRROR;
                break;
                default:
                    DBG_D3D((2, "Illegal value passed to ChangeState "
                                " D3DRENDERSTATE_TEXTUREADDRESS = %d",
                                                        dwRSVal));
                    // set a fallback value
                    pSoftPermedia->TextureReadMode.TWrapMode =
                                                      _P2_TEXTURE_REPEAT;
                    pSoftPermedia->TextureReadMode.SWrapMode =
                                                      _P2_TEXTURE_REPEAT;
                break;
            }

        break;

        case D3DRENDERSTATE_TEXTUREADDRESSU:
                DBG_D3D((8, "ChangeState: Texture address 0x%x "
                            "(D3DTEXTUREADDRESSU)", dwRSVal));
            switch (dwRSVal) {
                case D3DTADDRESS_CLAMP:
                    pSoftPermedia->TextureReadMode.SWrapMode =
                                                      _P2_TEXTURE_CLAMP;
                break;
                case D3DTADDRESS_WRAP:
                    pSoftPermedia->TextureReadMode.SWrapMode =
                                                      _P2_TEXTURE_REPEAT;
                break;
                case D3DTADDRESS_MIRROR:
                    pSoftPermedia->TextureReadMode.SWrapMode =
                                                      _P2_TEXTURE_MIRROR;
                break;
                default:
                    DBG_D3D((2, "Illegal value passed to ChangeState "
                                " D3DRENDERSTATE_TEXTUREADDRESSU = %d",
                                                          dwRSVal));
                    // set a fallback value
                    pSoftPermedia->TextureReadMode.SWrapMode =
                                                      _P2_TEXTURE_REPEAT;
                break;
            }
        break;

        case D3DRENDERSTATE_TEXTUREADDRESSV:
            DBG_D3D((8, "ChangeState: Texture address 0x%x "
                        "(D3DTEXTUREADDRESSV)", dwRSVal));
            switch (dwRSVal) 
            {
                case D3DTADDRESS_CLAMP:
                    pSoftPermedia->TextureReadMode.TWrapMode =
                                                      _P2_TEXTURE_CLAMP;
                break;
                case D3DTADDRESS_WRAP:
                    pSoftPermedia->TextureReadMode.TWrapMode =
                                                      _P2_TEXTURE_REPEAT;
                break;
                case D3DTADDRESS_MIRROR:
                    pSoftPermedia->TextureReadMode.TWrapMode =
                                                      _P2_TEXTURE_MIRROR;
                break;
                default:
                    DBG_D3D((2, "Illegal value passed to ChangeState "
                                " D3DRENDERSTATE_TEXTUREADDRESSV = %d",
                                                       dwRSVal));
                    // set a fallback value
                    pSoftPermedia->TextureReadMode.TWrapMode =
                                                      _P2_TEXTURE_REPEAT;
                break;
            }

        break;

        case D3DRENDERSTATE_TEXTUREHANDLE:
            DBG_D3D((8, "ChangeState: Texture Handle 0x%x",dwRSVal));
            if (dwRSVal != pContext->CurrentTextureHandle)
            {
                pContext->CurrentTextureHandle = dwRSVal;
                pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
            }
        break;

        case D3DRENDERSTATE_WRAPU:
            DBG_D3D((8, "ChangeState: Wrap_U "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal)
            {
                *pFlags |= CTXT_HAS_WRAPU_ENABLED;
            }
            else
            {
                *pFlags &= ~CTXT_HAS_WRAPU_ENABLED;
            }
        break;


        case D3DRENDERSTATE_WRAPV:
            DBG_D3D((8, "ChangeState: Wrap_V "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal)
            {
                *pFlags |= CTXT_HAS_WRAPV_ENABLED;
            }
            else
            {
                *pFlags &= ~CTXT_HAS_WRAPV_ENABLED;
            }
        break;

        case D3DRENDERSTATE_ZWRITEENABLE:
            DBG_D3D((8, "ChangeState: Z Write Enable "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                // Local Buffer Write mode
                if (!(*pFlags & CTXT_HAS_ZWRITE_ENABLED))
                {
                    DBG_D3D((8, "   Enabling Z Writes"));
                    *pFlags |= CTXT_HAS_ZWRITE_ENABLED;
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
                }
            }
            else
            {
                if (*pFlags & CTXT_HAS_ZWRITE_ENABLED)
                {
                    DBG_D3D((8, "   Disabling Z Writes"));
                    *pFlags &= ~CTXT_HAS_ZWRITE_ENABLED;
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
                }
            }
        break;


        case D3DRENDERSTATE_LASTPIXEL:
            // True for last pixel on lines
            DBG_D3D((8, "ChangeState: Last Pixel "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal)
            {
                *pFlags |= CTXT_HAS_LASTPIXEL_ENABLED;
            }
            else
            {
                *pFlags &= ~CTXT_HAS_LASTPIXEL_ENABLED;
            }
        break;

        case D3DRENDERSTATE_TEXTUREMAG:
            DBG_D3D((8, "ChangeState: Texture magnification "
                        "(D3DTEXTUREFILTER) 0x%x",dwRSVal));
            switch(dwRSVal) 
            {
                case D3DFILTER_NEAREST:
                case D3DFILTER_MIPNEAREST:
                    pContext->bMagFilter = FALSE;
                    pSoftPermedia->TextureReadMode.FilterMode = 0;
                break;
                case D3DFILTER_LINEAR:
                case D3DFILTER_MIPLINEAR:
                case D3DFILTER_LINEARMIPNEAREST:
                case D3DFILTER_LINEARMIPLINEAR:
                    pContext->bMagFilter = TRUE;
                    pSoftPermedia->TextureReadMode.FilterMode = 1;
                break;
                default:
                break;
            }
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
        break;

        case D3DRENDERSTATE_TEXTUREMIN:
            DBG_D3D((8, "ChangeState: Texture minification "
                        "(D3DTEXTUREFILTER) 0x%x",dwRSVal));
            switch(dwRSVal) 
            {
                case D3DFILTER_NEAREST:
                case D3DFILTER_MIPNEAREST:
                    pContext->bMinFilter = FALSE;
                break;
                case D3DFILTER_MIPLINEAR:
                case D3DFILTER_LINEAR:
                case D3DFILTER_LINEARMIPNEAREST:
                case D3DFILTER_LINEARMIPLINEAR:
                    pContext->bMinFilter = TRUE;
                break;
                default:
                break;
            }
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
        break;

        case D3DRENDERSTATE_SRCBLEND:
            DBG_D3D((8, "ChangeState: Source Blend (D3DBLEND):"));
            DECODEBLEND(4, dwRSVal);
            switch (dwRSVal) 
            {
                case D3DBLEND_ZERO:
                    pSoftPermedia->AlphaBlendMode.SourceBlend =
                                      __PERMEDIA_BLEND_FUNC_ZERO;
                break;
                case D3DBLEND_ONE:
                    pSoftPermedia->AlphaBlendMode.SourceBlend =
                                      __PERMEDIA_BLEND_FUNC_ONE;
                break;
                case D3DBLEND_SRCALPHA:
                    pSoftPermedia->AlphaBlendMode.SourceBlend =
                                      __PERMEDIA_BLEND_FUNC_SRC_ALPHA;
                break;
                default:
                    DBG_D3D((2,"Invalid Source Blend! - %d",
                                                  dwRSVal));
                break;
            }

            // If alpha is on, we may need to validate the chosen blend
            if (*pFlags & CTXT_HAS_ALPHABLEND_ENABLED) 
                pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHABLEND;

        break;

        case D3DRENDERSTATE_DESTBLEND:
            DBG_D3D((8, "ChangeState: Destination Blend (D3DBLEND):"));
            DECODEBLEND(4, dwRSVal);
            switch (dwRSVal) 
            {
                case D3DBLEND_ZERO:
                    pSoftPermedia->AlphaBlendMode.DestinationBlend =
                                 __PERMEDIA_BLEND_FUNC_ZERO;
                break;
                case D3DBLEND_ONE:
                    pSoftPermedia->AlphaBlendMode.DestinationBlend =
                                 __PERMEDIA_BLEND_FUNC_ONE;
                break;
                case D3DBLEND_INVSRCALPHA:
                    pSoftPermedia->AlphaBlendMode.DestinationBlend =
                                 __PERMEDIA_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                break;
                default:
                    DBG_D3D((2,"Invalid Dest Blend! - %d", dwRSVal));
                break;
            }

            // If alpha is on, we may need to validate the chosen blend
            if (*pFlags & CTXT_HAS_ALPHABLEND_ENABLED) 
                pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHABLEND;

        break;

        case D3DRENDERSTATE_CULLMODE:
            DBG_D3D((8, "ChangeState: Cull Mode "
                        "(D3DCULL) 0x%x",dwRSVal));
            pContext->CullMode = (D3DCULL) dwRSVal;
#ifdef P2_CHIP_CULLING
            switch(dwRSVal) 
            {
                case D3DCULL_NONE:
                pSoftPermedia->DeltaMode.BackfaceCull = 0;
                break;

                case D3DCULL_CCW:
                RENDER_NEGATIVE_CULL(pContext->RenderCommand);
                pSoftPermedia->DeltaMode.BackfaceCull = 1;
                break;

                case D3DCULL_CW:
                RENDER_POSITIVE_CULL(pContext->RenderCommand);
                pSoftPermedia->DeltaMode.BackfaceCull = 1;
                break;
            }
#endif
        break;

        case D3DRENDERSTATE_ZFUNC:
            DBG_D3D((8, "ChangeState: Z Compare function "
                        "(D3DCMPFUNC) 0x%x",dwRSVal));
            switch (dwRSVal) 
            {
                case D3DCMP_NEVER:
                    pSoftPermedia->DepthMode.CompareMode =
                                 __PERMEDIA_DEPTH_COMPARE_MODE_NEVER;
                break;
                case D3DCMP_LESS:
                    pSoftPermedia->DepthMode.CompareMode =
                                 __PERMEDIA_DEPTH_COMPARE_MODE_LESS;
                break;
                case D3DCMP_EQUAL:
                    pSoftPermedia->DepthMode.CompareMode =
                                 __PERMEDIA_DEPTH_COMPARE_MODE_EQUAL;
                break;
                case D3DCMP_LESSEQUAL:
                    pSoftPermedia->DepthMode.CompareMode =
                                 __PERMEDIA_DEPTH_COMPARE_MODE_LESS_OR_EQUAL;
                break;
                case D3DCMP_GREATER:
                    pSoftPermedia->DepthMode.CompareMode =
                                 __PERMEDIA_DEPTH_COMPARE_MODE_GREATER;
                break;
                case D3DCMP_NOTEQUAL:
                    pSoftPermedia->DepthMode.CompareMode =
                                 __PERMEDIA_DEPTH_COMPARE_MODE_NOT_EQUAL;
                break;
                case D3DCMP_GREATEREQUAL:
                    pSoftPermedia->DepthMode.CompareMode =
                                 __PERMEDIA_DEPTH_COMPARE_MODE_GREATER_OR_EQUAL;
                break;
                case D3DCMP_ALWAYS:
                    pSoftPermedia->DepthMode.CompareMode =
                                 __PERMEDIA_DEPTH_COMPARE_MODE_ALWAYS;
                break;
                default:
                    pSoftPermedia->DepthMode.CompareMode =
                                 __PERMEDIA_DEPTH_COMPARE_MODE_LESS_OR_EQUAL;
                break;
            }
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
        break;

        case D3DRENDERSTATE_DITHERENABLE:
            DBG_D3D((8, "ChangeState: Dither Enable "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                pSoftPermedia->DitherMode.DitherEnable = DITHER_ENABLE;
            }
            else
            {
                pSoftPermedia->DitherMode.DitherEnable = 0;
            }
        break;

        case D3DRENDERSTATE_COLORKEYENABLE:
            DBG_D3D((8, "ChangeState: ColorKey Enable "
                        "(BOOL) 0x%x",dwRSVal));
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
        break;

        case D3DRENDERSTATE_ALPHABLENDENABLE:
            DBG_D3D((8, "ChangeState: Blend Enable "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                if (!(*pFlags & CTXT_HAS_ALPHABLEND_ENABLED))
                {
                    // Set the blend enable flag in the render context struct
                    *pFlags |= CTXT_HAS_ALPHABLEND_ENABLED;
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHABLEND;
                }
            }
            else
            {
                if (*pFlags & CTXT_HAS_ALPHABLEND_ENABLED)
                {
                    // Turn off blend enable flag in render context struct
                    *pFlags &= ~CTXT_HAS_ALPHABLEND_ENABLED;
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHABLEND;
                }
            }
        break;

        case D3DRENDERSTATE_FOGENABLE:
            DBG_D3D((8, "ChangeState: Fog Enable "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                    *pFlags |= CTXT_HAS_FOGGING_ENABLED;
                    RENDER_FOG_ENABLE(pContext->RenderCommand);
            }
            else
            {
                    *pFlags &= ~CTXT_HAS_FOGGING_ENABLED;
                    RENDER_FOG_DISABLE(pContext->RenderCommand);
            }
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
        break;

        case D3DRENDERSTATE_FOGCOLOR:
            DBG_D3D((8, "ChangeState: Fog Color "
                        "(D3DCOLOR) 0x%x",dwRSVal));
            {
                BYTE red, green, blue, alpha;

                red = (BYTE)RGBA_GETRED(dwRSVal);
                green = (BYTE)RGBA_GETGREEN(dwRSVal);
                blue = (BYTE)RGBA_GETBLUE(dwRSVal);
                alpha = (BYTE)RGBA_GETALPHA(dwRSVal);
                DBG_D3D((4,"FogColor: Red 0x%x, Green 0x%x, Blue 0x%x",
                                                     red, green, blue));
                pSoftPermedia->FogColor = RGBA_MAKE(blue, green, red, alpha);
            }
        break;

        case D3DRENDERSTATE_SPECULARENABLE:
            DBG_D3D((8, "ChangeState: Specular Lighting "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal)
            {
                *pFlags |= CTXT_HAS_SPECULAR_ENABLED;
            } 
            else
            {
                *pFlags &= ~CTXT_HAS_SPECULAR_ENABLED;
            }
        break;

        case D3DRENDERSTATE_FILLMODE:
            DBG_D3D((8, "ChangeState: Fill Mode 0x%x",dwRSVal));
            pContext->FillMode = dwRSVal;
            WAIT_FOR_FREE_ENTRIES(ppdev, 1);
            switch (dwRSVal) 
            {
                case D3DFILL_POINT:
                    DBG_D3D((4, "RM = Point"));
                    // Restore the RasterizerMode
                    pSoftPermedia->dwRasterizerMode = 0;
                    break;
                case D3DFILL_WIREFRAME:
                    DBG_D3D((4, "RM = Wire"));
                    // Add nearly a half in the delta case for lines
                    // (lines aren't biased on a delta).
                    pSoftPermedia->dwRasterizerMode = BIAS_NEARLY_HALF;
                    break;
                case D3DFILL_SOLID:
                    DBG_D3D((4, "RM = Solid"));
                    // Restore the RasterizerMode
                    pSoftPermedia->dwRasterizerMode = 0;
                    break;
                default:
                    // Illegal value
                    DBG_D3D((4, "RM = Nonsense"));
                    pContext->FillMode = D3DFILL_SOLID;
                    // Restore the RasterizerMode
                    pSoftPermedia->dwRasterizerMode = 0;
                    break;
            }
        break;

        case D3DRENDERSTATE_TEXTUREPERSPECTIVE:
            DBG_D3D((8, "ChangeState: Texture Perspective "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                pSoftPermedia->TextureAddressMode.PerspectiveCorrection = 1;
                pSoftPermedia->DeltaMode.TextureParameterMode = 2; // Normalise
                *pFlags |= CTXT_HAS_PERSPECTIVE_ENABLED;
            }
            else
            {
                pSoftPermedia->TextureAddressMode.PerspectiveCorrection = 0;
                pSoftPermedia->DeltaMode.TextureParameterMode = 1; // Clamp
                *pFlags &= ~CTXT_HAS_PERSPECTIVE_ENABLED;
            }

        break;

        case D3DRENDERSTATE_ZENABLE:
            DBG_D3D((8, "ChangeState: Z Enable "
                        "(TRUE) 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                if ( (!(*pFlags & CTXT_HAS_ZBUFFER_ENABLED)) &&
                     (pContext->ZBufferHandle) )
                {
                    // Local Buffer Write mode
                    DBG_D3D((4, "       Enabling Z Buffer"));

                    *pFlags |= CTXT_HAS_ZBUFFER_ENABLED;
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
                }
            }
            else
            {
                if (*pFlags & CTXT_HAS_ZBUFFER_ENABLED)
                {
                    DBG_D3D((4, "  Disabling Z Buffer"));
                    *pFlags &= ~CTXT_HAS_ZBUFFER_ENABLED;
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
                }
            }
        break;

        case D3DRENDERSTATE_SHADEMODE:
            DBG_D3D((8, "ChangeState: Shade mode "
                        "(D3DSHADEMODE) 0x%x",dwRSVal));
            switch(dwRSVal) 
            {
                case D3DSHADE_PHONG:
                case D3DSHADE_GOURAUD:
                    if (!(*pFlags & CTXT_HAS_GOURAUD_ENABLED))
                    {
                        pSoftPermedia->ColorDDAMode.ShadeMode = 1;// Set DDA to gouraud                       
                        pSoftPermedia->DeltaMode.SmoothShadingEnable = 1;
                        *pFlags |= CTXT_HAS_GOURAUD_ENABLED;
                        // If we are textureing, some changes may need to be made
                        if (pContext->CurrentTextureHandle != 0)
                            pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
                    }
                    break;
                case D3DSHADE_FLAT:
                    if (*pFlags & CTXT_HAS_GOURAUD_ENABLED)
                    {
                        pSoftPermedia->ColorDDAMode.ShadeMode = 0;// Set DDA to flat
                        pSoftPermedia->DeltaMode.SmoothShadingEnable = 0;
                        *pFlags &= ~CTXT_HAS_GOURAUD_ENABLED;
                        // If we are textureing, some changes may need to be made
                        if (pContext->CurrentTextureHandle != 0) 
                            pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
                    }
                break;
            }
        break;


        case D3DRENDERSTATE_PLANEMASK:
            DBG_D3D((8, "ChangeState: Plane Mask "
                        "(ULONG) 0x%x",dwRSVal));
            pSoftPermedia->FBHardwareWriteMask = (DWORD)dwRSVal;
        break;

        case D3DRENDERSTATE_MONOENABLE:
            DBG_D3D((8, "ChangeState: Mono Raster enable "
                        "(BOOL) 0x%x", dwRSVal));
            if (dwRSVal)
            {
                *pFlags |= CTXT_HAS_MONO_ENABLED;
            }
            else
            {
                *pFlags &= ~CTXT_HAS_MONO_ENABLED;
            }
        break;

#if D3D_STENCIL
    //
    // Stenciling Render States
    //
        case D3DRENDERSTATE_STENCILENABLE:
            DBG_D3D((8, "ChangeState: Stencil Enable "
                        "(ULONG) 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                pSoftPermedia->StencilMode.UnitEnable = __PERMEDIA_ENABLE;
            }
            else
            {
                pSoftPermedia->StencilMode.UnitEnable = __PERMEDIA_DISABLE;
            }
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
        break;

        case D3DRENDERSTATE_STENCILFAIL:
            DBG_D3D((8, "ChangeState: Stencil Fail Method "
                        "(ULONG) 0x%x",dwRSVal));
            switch (dwRSVal) 
            {
                case D3DSTENCILOP_KEEP:
                    pSoftPermedia->StencilMode.SFail =
                                             __PERMEDIA_STENCIL_METHOD_KEEP;
                break;
                case D3DSTENCILOP_ZERO:
                    pSoftPermedia->StencilMode.SFail =
                                             __PERMEDIA_STENCIL_METHOD_ZERO;
                break;
                case D3DSTENCILOP_REPLACE:
                    pSoftPermedia->StencilMode.SFail =
                                             __PERMEDIA_STENCIL_METHOD_REPLACE;
                break;
                case D3DSTENCILOP_INCRSAT:
                case D3DSTENCILOP_INCR:
                    pSoftPermedia->StencilMode.SFail =
                                             __PERMEDIA_STENCIL_METHOD_INCR;
                break;
                case D3DSTENCILOP_DECR:
                case D3DSTENCILOP_DECRSAT:
                    pSoftPermedia->StencilMode.SFail =
                                             __PERMEDIA_STENCIL_METHOD_DECR;
                break;
                case D3DSTENCILOP_INVERT:
                    pSoftPermedia->StencilMode.SFail =
                                             __PERMEDIA_STENCIL_METHOD_INVERT;
                break;
                default:
                    DBG_D3D((2, " Unrecognized stencil method 0x%x",
                                                           dwRSVal));
                break;
            }
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
        break;

        case D3DRENDERSTATE_STENCILZFAIL:
            DBG_D3D((8, "ChangeState: Stencil Pass Depth Fail Method "
                        "(ULONG) 0x%x",dwRSVal));
            switch (dwRSVal) 
            {
                case D3DSTENCILOP_KEEP:
                pSoftPermedia->StencilMode.DPFail =
                                         __PERMEDIA_STENCIL_METHOD_KEEP;
                break;
                case D3DSTENCILOP_ZERO:
                pSoftPermedia->StencilMode.DPFail =
                                         __PERMEDIA_STENCIL_METHOD_ZERO;
                break;
                case D3DSTENCILOP_REPLACE:
                pSoftPermedia->StencilMode.DPFail =
                                         __PERMEDIA_STENCIL_METHOD_REPLACE;
                break;
                case D3DSTENCILOP_INCRSAT:
                case D3DSTENCILOP_INCR:
                pSoftPermedia->StencilMode.DPFail =
                                         __PERMEDIA_STENCIL_METHOD_INCR;
                break;
                case D3DSTENCILOP_DECR:
                case D3DSTENCILOP_DECRSAT:
                pSoftPermedia->StencilMode.DPFail =
                                         __PERMEDIA_STENCIL_METHOD_DECR;
                break;
                case D3DSTENCILOP_INVERT:
                pSoftPermedia->StencilMode.DPFail =
                                         __PERMEDIA_STENCIL_METHOD_INVERT;
                break;
                default:
                DBG_D3D((2, " Unrecognized stencil method 0x%x",
                                                       dwRSVal));
                break;
            }
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
        break;

        case D3DRENDERSTATE_STENCILPASS:
            DBG_D3D((8, "ChangeState: Stencil Pass Method "
                        "(ULONG) 0x%x",dwRSVal));
            switch (dwRSVal) 
            {
                case D3DSTENCILOP_KEEP:
                    pSoftPermedia->StencilMode.DPPass =
                                 __PERMEDIA_STENCIL_METHOD_KEEP;
                break;
                case D3DSTENCILOP_ZERO:
                    pSoftPermedia->StencilMode.DPPass =
                                 __PERMEDIA_STENCIL_METHOD_ZERO;
                break;
                case D3DSTENCILOP_REPLACE:
                    pSoftPermedia->StencilMode.DPPass =
                                 __PERMEDIA_STENCIL_METHOD_REPLACE;
                break;
                case D3DSTENCILOP_INCRSAT:
                case D3DSTENCILOP_INCR:
                    pSoftPermedia->StencilMode.DPPass =
                                 __PERMEDIA_STENCIL_METHOD_INCR;
                break;
                case D3DSTENCILOP_DECR:
                case D3DSTENCILOP_DECRSAT:
                    pSoftPermedia->StencilMode.DPPass =
                                 __PERMEDIA_STENCIL_METHOD_DECR;
                break;
                case D3DSTENCILOP_INVERT:
                    pSoftPermedia->StencilMode.DPPass =
                                 __PERMEDIA_STENCIL_METHOD_INVERT;
                break;
                default:
                    DBG_D3D((2, " Unrecognized stencil method 0x%x",
                                                        dwRSVal));
                break;
            }
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
        break;

        case D3DRENDERSTATE_STENCILFUNC:
            DBG_D3D((8, "ChangeState: Stencil Comparison Function "
                        "(ULONG) 0x%x",dwRSVal));
            switch (dwRSVal) 
            {
                case D3DCMP_NEVER:
                    pSoftPermedia->StencilMode.CompareFunction =
                                 __PERMEDIA_STENCIL_COMPARE_MODE_NEVER;
                break;
                case D3DCMP_LESS:
                    pSoftPermedia->StencilMode.CompareFunction =
                                 __PERMEDIA_STENCIL_COMPARE_MODE_LESS;
                break;
                case D3DCMP_EQUAL:
                    pSoftPermedia->StencilMode.CompareFunction =
                                 __PERMEDIA_STENCIL_COMPARE_MODE_EQUAL;
                break;
                case D3DCMP_LESSEQUAL:
                    pSoftPermedia->StencilMode.CompareFunction =
                                 __PERMEDIA_STENCIL_COMPARE_MODE_LESS_OR_EQUAL;
                break;
                case D3DCMP_GREATER:
                    pSoftPermedia->StencilMode.CompareFunction =
                                 __PERMEDIA_STENCIL_COMPARE_MODE_GREATER;
                break;
                case D3DCMP_NOTEQUAL:
                    pSoftPermedia->StencilMode.CompareFunction =
                                 __PERMEDIA_STENCIL_COMPARE_MODE_NOT_EQUAL;
                break;
                case D3DCMP_GREATEREQUAL:
                    pSoftPermedia->StencilMode.CompareFunction =
                                 __PERMEDIA_STENCIL_COMPARE_MODE_GREATER_OR_EQUAL;
                break;
                case D3DCMP_ALWAYS:
                    pSoftPermedia->StencilMode.CompareFunction =
                                 __PERMEDIA_STENCIL_COMPARE_MODE_ALWAYS;
                break;
                default:
                    DBG_D3D((2, " Unrecognized stencil comparison function 0x%x",
                                                               dwRSVal));
                break;
            }
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
        break;

        case D3DRENDERSTATE_STENCILREF:
            DBG_D3D((8, "ChangeState: Stencil Reference Value "
                        "(ULONG) 0x%x",dwRSVal));
            pSoftPermedia->StencilData.ReferenceValue =
                                         ( dwRSVal & 0x0001 );
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
        break;

        case D3DRENDERSTATE_STENCILMASK:
            DBG_D3D((8, "ChangeState: Stencil Compare Mask "
                        "(ULONG) 0x%x",dwRSVal));
            pSoftPermedia->StencilData.CompareMask =
                                        ( dwRSVal & 0x0001 );
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
        break;

        case D3DRENDERSTATE_STENCILWRITEMASK:
            DBG_D3D((8, "ChangeState: Stencil Write Mask "
                        "(ULONG) 0x%x",dwRSVal));
            pSoftPermedia->StencilData.WriteMask =
                                        ( dwRSVal & 0x0001 );
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
        break;
#endif // D3D_STENCIL

        //
        // Stippling
        //
        case D3DRENDERSTATE_STIPPLEDALPHA:
            DBG_D3D((8, "ChangeState: Stippled Alpha "
                        "(BOOL) 0x%x",dwRSVal));
            if (dwRSVal)
            {
                if (!(*pFlags & CTXT_HAS_ALPHASTIPPLE_ENABLED))
                {
                    // Force a new start on the Alpha pattern
                    pContext->LastAlpha = 16;

                    *pFlags |= CTXT_HAS_ALPHASTIPPLE_ENABLED;
                    if (pContext->bKeptStipple == TRUE)
                    {
                        RENDER_AREA_STIPPLE_DISABLE(pContext->RenderCommand);
                    }
                }
            }
            else
            {
                if (*pFlags & CTXT_HAS_ALPHASTIPPLE_ENABLED)
                {
                    // If Alpha Stipple is being turned off, turn the normal
                    // stipple back on, and enable it.
                    int i;

                    pSoftPermedia->AreaStipplePattern.startIndex    = 0;
                    pSoftPermedia->AreaStipplePattern.count         = 8;  
                    for (i = 0; i < 8; i++)
                    {
                        pSoftPermedia->AreaStipplePattern.Pattern[i] = 
                                            (DWORD)pContext->CurrentStipple[i];
                    }

                    *pFlags &= ~CTXT_HAS_ALPHASTIPPLE_ENABLED;

                    if (pContext->bKeptStipple == TRUE)
                    {
                        RENDER_AREA_STIPPLE_ENABLE(pContext->RenderCommand);
                    }
                }
            }
        break;

        case D3DRENDERSTATE_STIPPLEENABLE:
            DBG_D3D((8, "ChangeState: Stipple Enable "
                        "(BOOL) 0x%x", dwRSVal));
            if (dwRSVal)
            {
                if (!(*pFlags & CTXT_HAS_ALPHASTIPPLE_ENABLED))
                {
                    RENDER_AREA_STIPPLE_ENABLE(pContext->RenderCommand);
                }
                pContext->bKeptStipple = TRUE;
            }
            else
            {
                RENDER_AREA_STIPPLE_DISABLE(pContext->RenderCommand);
                pContext->bKeptStipple = FALSE;
            }
        break;

        case D3DRENDERSTATE_SCENECAPTURE:
            // This state pass TRUE or FALSE to replace the functionality
            // of D3DHALCallbacks->SceneCapture(), Permedia2 Hardware doesn't
            // need begin/end scene information, therefore it's a NOOP here.
            if (dwRSVal)
                TextureCacheManagerResetStatCounters(pContext->pTextureManager);

            DBG_D3D((8,"D3DRENDERSTATE_SCENECAPTURE=%x", (DWORD)dwRSVal));
        break;

        case D3DRENDERSTATE_ANTIALIAS:
        case D3DRENDERSTATE_LINEPATTERN:
        case D3DRENDERSTATE_ALPHATESTENABLE:
        case D3DRENDERSTATE_ALPHAREF:
        case D3DRENDERSTATE_ALPHAFUNC:
        case D3DRENDERSTATE_MIPMAPLODBIAS:
        case D3DRENDERSTATE_ROP2:
        case D3DRENDERSTATE_ZVISIBLE:// From DX6 onwards this is an obsolete 
                                     // render state. The D3D runtime does not 
                                     // support it anymore so drivers 
                                     // don't need to implement it
        case D3DRENDERSTATE_SUBPIXEL:
        case D3DRENDERSTATE_SUBPIXELX:
        case D3DRENDERSTATE_CLIPPING:
        case D3DRENDERSTATE_LIGHTING:
        case D3DRENDERSTATE_EXTENTS:
        case D3DRENDERSTATE_AMBIENT:
        case D3DRENDERSTATE_FOGVERTEXMODE:
        case D3DRENDERSTATE_COLORVERTEX:
        case D3DRENDERSTATE_LOCALVIEWER:
        case D3DRENDERSTATE_NORMALIZENORMALS:
        case D3DRENDERSTATE_COLORKEYBLENDENABLE:
        case D3DRENDERSTATE_DIFFUSEMATERIALSOURCE:
        case D3DRENDERSTATE_SPECULARMATERIALSOURCE:
        case D3DRENDERSTATE_AMBIENTMATERIALSOURCE:
        case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE:
        case D3DRENDERSTATE_VERTEXBLEND:
        case D3DRENDERSTATE_CLIPPLANEENABLE:
        break;


        case D3DRENDERSTATE_WRAP0:
        case D3DRENDERSTATE_WRAP1:
        case D3DRENDERSTATE_WRAP2:
        case D3DRENDERSTATE_WRAP3:
        case D3DRENDERSTATE_WRAP4:
        case D3DRENDERSTATE_WRAP5:
        case D3DRENDERSTATE_WRAP6:
        case D3DRENDERSTATE_WRAP7:
            DBG_D3D((8, "ChangeState: Wrap(%x) "
                        "(BOOL) 0x%x",(dwRSType - D3DRENDERSTATE_WRAPBIAS ),dwRSVal));
            pContext->dwWrap[dwRSType - D3DRENDERSTATE_WRAPBIAS] = dwRSVal;
            break;

        default:
            if ((dwRSType >= D3DRENDERSTATE_STIPPLEPATTERN00) && 
                (dwRSType <= D3DRENDERSTATE_STIPPLEPATTERN07))
            {
                DBG_D3D((8, "ChangeState: Loading Stipple0x%x with 0x%x",
                                        dwRSType - D3DRENDERSTATE_STIPPLEPATTERN00,
                                        (DWORD)dwRSVal));

                pContext->CurrentStipple[(dwRSType - D3DRENDERSTATE_STIPPLEPATTERN00)] =
                                                             (BYTE)dwRSVal;

                if (!(*pFlags & CTXT_HAS_ALPHASTIPPLE_ENABLED))
                {
                    // Flat-Stippled Alpha is not on, so use the 
                    // current stipple pattern
                    pSoftPermedia->AreaStipplePattern.startIndex = (dwRSType - D3DRENDERSTATE_STIPPLEPATTERN00);
                    pSoftPermedia->AreaStipplePattern.count      = 1;
                    pSoftPermedia->AreaStipplePattern.Pattern[0] = (DWORD)dwRSVal;
                }
            }
            else
            {
                DBG_D3D((2, "ChangeState: Unhandled opcode = %d", dwRSType));
            }
        break;
    } // switch (dwRSType of renderstate)

    DBG_NOTHANDLEDRENDERSTATES(dwRSType, dwRSVal);

    DBG_D3D((10,"Exiting SetupP2RenderStates"));
} //SetupP2RenderStates


/*
 * Function:    __HWSetupStageStates
 * Description: Processes the state changes related to the DX7 texture stage 
 *              states in the current rendering context
 */

static HRESULT  
__HWSetupStageStates(PERMEDIA_D3DCONTEXT *pContext, 
                     LPP2FVFOFFSETS      lpP2FVFOff)
{
    PP2REG_SOFTWARE_COPY    pSoftPermedia   = &pContext->SoftCopyP2Regs;
    DWORD                   dwFlags         = pContext->Flags;
    LPP2THUNKEDDATA         ppdev           = pContext->pThisDisplay;

    DBG_D3D((10,"Entering __HWSetupStageStates"));

    
    // If we are to texture map our primitives
    if (pContext->TssStates[D3DTSS_TEXTUREMAP])
    {
        DWORD dwMag = pContext->TssStates[D3DTSS_MAGFILTER];
        DWORD dwMin = pContext->TssStates[D3DTSS_MINFILTER];
        DWORD dwMip = pContext->TssStates[D3DTSS_MIPFILTER];
        DWORD dwCop = pContext->TssStates[D3DTSS_COLOROP];
        DWORD dwCa1 = pContext->TssStates[D3DTSS_COLORARG1];
        DWORD dwCa2 = pContext->TssStates[D3DTSS_COLORARG2];
        DWORD dwAop = pContext->TssStates[D3DTSS_ALPHAOP];
        DWORD dwAa1 = pContext->TssStates[D3DTSS_ALPHAARG1];
        DWORD dwAa2 = pContext->TssStates[D3DTSS_ALPHAARG2];
        DWORD dwTau = pContext->TssStates[D3DTSS_ADDRESSU];
        DWORD dwTav = pContext->TssStates[D3DTSS_ADDRESSV];
        DWORD dwTxc = pContext->TssStates[D3DTSS_TEXCOORDINDEX];

        DBG_D3D((6,"Setting up w TSS:"
                   "dwCop=%x dwCa1=%x dwCa2=%x dwAop=%x dwAa1=%x dwAa2=%x "
                   "dwMag=%x dwMin=%x dwMip=%x dwTau=%x dwTav=%x dwTxc=%x",
                   dwCop, dwCa1, dwCa2, dwAop, dwAa1, dwAa2,
                   dwMag, dwMin, dwMip, dwTau, dwTav, dwTxc));

        // Choose texture coord to use
        __SelectFVFTexCoord( lpP2FVFOff, dwTxc);

        // Current is the same as diffuse in stage 0
        if (dwCa2 == D3DTA_CURRENT)
            dwCa2 = D3DTA_DIFFUSE;
        if (dwAa2 == D3DTA_CURRENT)
            dwAa2 = D3DTA_DIFFUSE;

        // Check if we need to disable texturing 
        if (dwCop == D3DTOP_DISABLE || 
            (dwCop == D3DTOP_SELECTARG2 && dwCa2 == D3DTA_DIFFUSE && 
             dwAop == D3DTOP_SELECTARG2 && dwAa2 == D3DTA_DIFFUSE))
        {
            //Please don't clear pContext->TssStates[D3DTSS_TEXTUREMAP] though
           pContext->CurrentTextureHandle = 0;
            DBG_D3D((10,"Exiting __HWSetupStageStates , texturing disabled"));
            return DD_OK;
        }

        // setup the address mode
        switch (dwTau) 
        {
            case D3DTADDRESS_CLAMP:
                pSoftPermedia->TextureReadMode.SWrapMode = _P2_TEXTURE_CLAMP;
            break;
            case D3DTADDRESS_WRAP:
                pSoftPermedia->TextureReadMode.SWrapMode = _P2_TEXTURE_REPEAT;
            break;
            case D3DTADDRESS_MIRROR:
                pSoftPermedia->TextureReadMode.SWrapMode = _P2_TEXTURE_MIRROR;
            break;
            default:
                DBG_D3D((2, "Illegal value passed to TSS U address mode = %d"
                                                                      ,dwTau));
                pSoftPermedia->TextureReadMode.SWrapMode = _P2_TEXTURE_REPEAT;
            break;
        }
        switch (dwTav) 
        {
            case D3DTADDRESS_CLAMP:
                pSoftPermedia->TextureReadMode.TWrapMode = _P2_TEXTURE_CLAMP;
            break;
            case D3DTADDRESS_WRAP:
                pSoftPermedia->TextureReadMode.TWrapMode = _P2_TEXTURE_REPEAT;
            break;
            case D3DTADDRESS_MIRROR:
                pSoftPermedia->TextureReadMode.TWrapMode = _P2_TEXTURE_MIRROR;
            break;
            default:
                DBG_D3D((2, "Illegal value passed to TSS V address mode = %d"
                                                                      ,dwTav));
                pSoftPermedia->TextureReadMode.TWrapMode = _P2_TEXTURE_REPEAT;
            break;
        }

        // Enable-disable wrapping flags for U & V       
        if (pContext->dwWrap[dwTxc] &  D3DWRAPCOORD_0)
        {
            pContext->Flags |= CTXT_HAS_WRAPU_ENABLED;
        }
        else
        {
            pContext->Flags &= ~CTXT_HAS_WRAPU_ENABLED;
        }

        if (pContext->dwWrap[dwTxc] &  D3DWRAPCOORD_1)
        {
            pContext->Flags |= CTXT_HAS_WRAPV_ENABLED;
        }
        else
        {
            pContext->Flags &= ~CTXT_HAS_WRAPV_ENABLED;
        }

        // Setup the equivalent texture filtering state
        if (dwMip == D3DTFP_NONE) 
        {
            // We can only take care of magnification filtering on the P2
            if (dwMag == D3DTFG_LINEAR)
            {
                pContext->bMagFilter = TRUE; // D3DFILTER_LINEAR;
            }
            else if (dwMag == D3DTFG_POINT)
            {
                pContext->bMagFilter = FALSE; // D3DFILTER_NEAREST;
            }
        }
        else if (dwMip == D3DTFP_POINT) 
        {
            if (dwMin == D3DTFN_POINT) 
            {
                pContext->bMagFilter = FALSE; // D3DFILTER_MIPNEAREST;
            }
            else if (dwMin == D3DTFN_LINEAR) 
            {
                pContext->bMagFilter = TRUE; // D3DFILTER_MIPLINEAR;
            }
        }
        else 
        { // dwMip == D3DTFP_LINEAR
            if (dwMin == D3DTFN_POINT) 
            {
                pContext->bMagFilter = TRUE; // D3DFILTER_LINEARMIPNEAREST;
            }
            else if (dwMin == D3DTFN_LINEAR) 
            {
                pContext->bMagFilter = TRUE; // D3DFILTER_LINEARMIPLINEAR;
            }
        }

        // Setup the equivalent texture blending state
        // Check if we need to decal
        if ((dwCa1 == D3DTA_TEXTURE && dwCop == D3DTOP_SELECTARG1) &&
             (dwAa1 == D3DTA_TEXTURE && dwAop == D3DTOP_SELECTARG1)) 
        {
            // D3DTBLEND_COPY;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_COPY;
        }
        // check if we the app modified the TSS for decaling after first
        // setting it up for modulating via the legacy renderstates
        // this is a Permedia2 specific optimization.
        else if ((dwCa1 == D3DTA_TEXTURE && dwCop == D3DTOP_SELECTARG1) &&
             (dwAa1 == D3DTA_TEXTURE && dwAop == D3DTOP_LEGACY_ALPHAOVR)) 
        {
            // D3DTBLEND_COPY;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_COPY;
        }
        // Check if we need to modulate & pass texture alpha
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) &&
                  dwCop == D3DTOP_MODULATE &&
                 (dwAa1 == D3DTA_TEXTURE && dwAop == D3DTOP_SELECTARG1)) 
        {
            // D3DTBLEND_MODULATE;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
        }
        // Check if we need to modulate & pass diffuse alpha
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) &&
                  dwCop == D3DTOP_MODULATE &&
                 (dwAa2 == D3DTA_DIFFUSE && dwAop == D3DTOP_SELECTARG2)) 
        {
            // D3DTBLEND_MODULATE;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
        }
        // Check if we need to do legacy modulate
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) &&
                  dwCop == D3DTOP_MODULATE &&
                 (dwAa1 == D3DTA_TEXTURE && dwAop == D3DTOP_LEGACY_ALPHAOVR)) 
        {
            // D3DTBLEND_MODULATE;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
        }
        // Check if we need to decal alpha
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) && 
                  dwCop == D3DTOP_BLENDTEXTUREALPHA &&
                 (dwAa2 == D3DTA_DIFFUSE && dwAop == D3DTOP_SELECTARG2)) 
        {
            // D3DTBLEND_DECALALPHA;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_DECAL;
        }
        // Check if we need to modulate alpha
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) && 
                  dwCop == D3DTOP_MODULATE &&
                 (dwAa2 == D3DTA_DIFFUSE && dwAa1 == D3DTA_TEXTURE) && 
                  dwAop == D3DTOP_MODULATE) 
        {
            // D3DTBLEND_MODULATEALPHA;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
        }
        else
        {
            DBG_D3D((0,"Trying to setup a state we don't understand!"));
        }

        pContext->CurrentTextureHandle = pContext->TssStates[D3DTSS_TEXTUREMAP];
    }
    else
    {
        // No texturing
        pContext->CurrentTextureHandle = 0;
    }

    pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;

    DBG_D3D((10,"Exiting __HWSetupStageStates"));

    return DD_OK;
} //__HWSetupStageStates

/*
 * Function:    __HandleDirtyPermediaState
 * Description: Handle any changes in the rendering context with respect to
 *              a change in a texture; an alpha blendmode change or 
 *              change in the handling of Z buffers.  
 */

void 
__HandleDirtyPermediaState(PERMEDIA_D3DCONTEXT  *pContext,
                           LPP2FVFOFFSETS       lpP2FVFOff)
{
    PP2REG_SOFTWARE_COPY    pSoftPermedia = &pContext->SoftCopyP2Regs;
    LPP2THUNKEDDATA         ppdev         = pContext->pThisDisplay;

    DBG_D3D((10,"Entering __HandleDirtyPermediaState"));

    // We need to keep this ordering of evaluation on the P2

    // --------------Have the texture or the stage states changed ? -----------

    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_TEXTURE)
    {
        DBG_D3D((4,"preparing to handle CONTEXT_DIRTY_TEXTURE"));
        // Choose between legacy Texture Handle or TSS
        if (pContext->dwDirtyFlags & CONTEXT_DIRTY_MULTITEXTURE)
        {
            pContext->dwDirtyFlags &= ~CONTEXT_DIRTY_MULTITEXTURE;
            //Setup TSS state AND textures
            if (__HWSetupStageStates(pContext, lpP2FVFOff) == DD_OK)
            {
                // if this FVF has no tex coordinates at all disable texturing
                if (lpP2FVFOff->dwTexBaseOffset == 0)
                {
                    pContext->CurrentTextureHandle = 0;
                    DBG_D3D((2,"No texture coords present in FVF "
                               "to texture map primitives"));
                }
            }
            else
            {
                pContext->CurrentTextureHandle = 0;
                DBG_D3D((0,"TSS Setup failed"));
            }
        }
        else
        {   
            // select default texture coordinate index
             __SelectFVFTexCoord( lpP2FVFOff, 0);
        }
    }

    // --------------Has the state of the LB changed ? ------------------------

    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_ZBUFFER)
    {
        DBG_D3D((4,"CONTEXT_DIRTY_ZBUFFER handled"));
        pContext->dwDirtyFlags &= ~CONTEXT_DIRTY_ZBUFFER;

        if ((pContext->Flags & CTXT_HAS_ZBUFFER_ENABLED) && 
            (pContext->ZBufferHandle))
        {
            if (pContext->Flags & CTXT_HAS_ZWRITE_ENABLED)
            {
                if (__PERMEDIA_DEPTH_COMPARE_MODE_NEVER ==
                    (int)pSoftPermedia->DepthMode.CompareMode)
                {
                    pSoftPermedia->LBReadMode.ReadDestinationEnable =
                                                             __PERMEDIA_DISABLE;
                    pSoftPermedia->LBWriteMode.WriteEnable  = __PERMEDIA_DISABLE;
                }
                else
                {
                    pSoftPermedia->LBReadMode.ReadDestinationEnable =
                                                             __PERMEDIA_ENABLE;
                    pSoftPermedia->LBWriteMode.WriteEnable  = __PERMEDIA_ENABLE;
                }
                pSoftPermedia->DepthMode.WriteMask          = __PERMEDIA_ENABLE;
            } 
            else 
            {
                pSoftPermedia->LBReadMode.ReadDestinationEnable =
                                                         __PERMEDIA_ENABLE;
                pSoftPermedia->LBWriteMode.WriteEnable      = __PERMEDIA_DISABLE;
                pSoftPermedia->DepthMode.WriteMask          = __PERMEDIA_DISABLE;
            }

            // We are Z Buffering 

            // Enable Z test
            pSoftPermedia->DepthMode.UnitEnable             = __PERMEDIA_ENABLE;

            // Tell delta we are Z Buffering.
            pSoftPermedia->DeltaMode.DepthEnable = 1;
        }
        else
        {
            // We are NOT Z Buffering

            // Disable Writes
            pSoftPermedia->LBWriteMode.WriteEnable          = __PERMEDIA_DISABLE;

            // Disable Z test
            pSoftPermedia->DepthMode.UnitEnable             = __PERMEDIA_DISABLE;
            pSoftPermedia->DepthMode.WriteMask              = __PERMEDIA_DISABLE;

            // No reads, no writes
            pSoftPermedia->LBReadMode.ReadDestinationEnable = __PERMEDIA_DISABLE;
            // Tell delta we aren't Z Buffering.
            pSoftPermedia->DeltaMode.DepthEnable            = 0;
        }

        if (__PERMEDIA_ENABLE == pSoftPermedia->StencilMode.UnitEnable)
        {
            pSoftPermedia->LBReadMode.ReadDestinationEnable = __PERMEDIA_ENABLE;

            pSoftPermedia->LBWriteMode.WriteEnable          = __PERMEDIA_ENABLE;
        }

    } // if CONTEXT_DIRTY_ZBUFFER

    // ----------------Has the alphablend type changed ? ----------------------


    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_ALPHABLEND)
    {
        DWORD dwBlendMode;
        // Only clear when we have an alphablend dirty context
        pContext->FakeBlendNum &= ~FAKE_ALPHABLEND_ONE_ONE;

        pContext->dwDirtyFlags &= ~CONTEXT_DIRTY_ALPHABLEND;

        // Verify that requested blend mode is HW supported
        dwBlendMode = 
            (((DWORD)pSoftPermedia->AlphaBlendMode.SourceBlend) |
             ((DWORD)pSoftPermedia->AlphaBlendMode.DestinationBlend) << 4);

        DBG_D3D((4,"CONTEXT_DIRTY_ALPHABLEND handled: Blend mode = %08lx",
                                                              dwBlendMode));

        switch (dwBlendMode) {

            // In this case, we set the bit to the QuickDraw mode
            case __PERMEDIA_BLENDOP_ONE_AND_INVSRCALPHA:
                DBG_D3D((4,"Blend Operation is PreMult"));
                pSoftPermedia->AlphaBlendMode.BlendType = 1;
            break;
            // This is the standard blend
            case __PERMEDIA_BLENDOP_SRCALPHA_AND_INVSRCALPHA:
                DBG_D3D((4,"Blend Operation is Blend"));
                pSoftPermedia->AlphaBlendMode.BlendType = 0;
            break;
            case ((__PERMEDIA_BLEND_FUNC_ZERO << 4) | 
                   __PERMEDIA_BLEND_FUNC_SRC_ALPHA):
                // we substitute the SrcBlend = SrcAlpha DstBlend = 1
                // with the 1,0 mode since we really dont' support
                // it, just so apps perform reasonably
                pSoftPermedia->AlphaBlendMode.AlphaBlendEnable = 0;

            case ((__PERMEDIA_BLEND_FUNC_ONE << 4) 
                 | __PERMEDIA_BLEND_FUNC_ZERO):

            case __PERMEDIA_BLENDOP_ONE_AND_ZERO:
            // This is code for 'no blending'
                DBG_D3D((4,"Blend Operation is validly None"));
            break;
            case ((__PERMEDIA_BLEND_FUNC_ONE << 4) | 
                   __PERMEDIA_BLEND_FUNC_SRC_ALPHA):
                // we substitute the SrcBlend = SrcAlpha DstBlend = 1
                // with the 1,1 mode since we really dont' support
                // it, just so apps perform reasonably
            case __PERMEDIA_BLENDOP_ONE_AND_ONE:
                DBG_D3D((4,"BlendOperation is 1 Source, 1 Dest"));
                pSoftPermedia->AlphaBlendMode.BlendType = 1;
                pContext->FakeBlendNum |= FAKE_ALPHABLEND_ONE_ONE;
            break;
            default:
                DBG_D3D((2,"Blend Operation is invalid! BlendOp == %x",
                                                              dwBlendMode));
                // This is a fallback blending mode 
                dwBlendMode = __PERMEDIA_BLENDOP_ONE_AND_ZERO;
            break;
        }


        if ((pContext->Flags & CTXT_HAS_ALPHABLEND_ENABLED) && 
            (dwBlendMode != __PERMEDIA_BLENDOP_ONE_AND_ZERO))
        {
            // Set the AlphaBlendMode register on Permedia
            pSoftPermedia->AlphaBlendMode.AlphaBlendEnable = 1;
            // Turn on destination read in the FBReadMode register
            pSoftPermedia->FBReadMode.ReadDestinationEnable = 1;
        }
        else
        {
            // Set the AlphaBlendMode register on Permedia
            pSoftPermedia->AlphaBlendMode.AlphaBlendEnable = 0;
            // Turn off the destination read in FbReadMode register
            pSoftPermedia->FBReadMode.ReadDestinationEnable = 0;

            // if not sending alpha, turn alpha to 1
            pSoftPermedia->dwAStart = PM_BYTE_COLOR(0xFF);
        }
    }

    // --------------------Has the texture handle changed ? -------------------

    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_TEXTURE)
    {
        pContext->dwDirtyFlags &= ~CONTEXT_DIRTY_TEXTURE;
        DBG_D3D((4,"CONTEXT_DIRTY_TEXTURE handled"));
        if (pContext->CurrentTextureHandle == 0)
            DisableTexturePermedia(pContext);
        else
            EnableTexturePermedia(pContext);
    }

    DBG_D3D((10,"Exiting __HandleDirtyPermediaState"));

} // __HandleDirtyPermediaState

/*
 * Function:    StorePermediaLODLevel
 * Description: Maps the width, height, stride and start offset
 *              of given LOD in a texture to their equivalent
 *              Permedia2 internal format. These would be used
 *              to program Permedia2 when the texture is used.
 */

VOID 
StorePermediaLODLevel(LPP2THUNKEDDATA           ppdev, 
                      PERMEDIA_D3DTEXTURE       *pTexture, 
                      LPDDRAWI_DDRAWSURFACE_LCL pSurf, 
                      int                       LOD)
{
    DDPIXELFORMAT       *pPixFormat;
    P2_SURFACE_FORMAT   P2SurfaceFormat;

    DBG_D3D((10,"Entering StorePermediaLODLevel"));

    if (pSurf->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        pPixFormat = &pSurf->lpGbl->ddpfSurface;
    }
    else
    {
        pPixFormat = &pSurf->lpGbl->lpDD->vmiData.ddpfDisplay;
    }

    GetP2SurfaceFormat(pPixFormat, &P2SurfaceFormat);    

    // if it's any surface type that's not created by driver
    // certainly there's no need to texture it
    // Get the BYTE Offset to the texture map

    if (pTexture->dwCaps & DDSCAPS_VIDEOMEMORY)
    {
        if (pTexture->dwCaps & DDSCAPS_NONLOCALVIDMEM)
        {
            pTexture->MipLevels[LOD].PixelOffset = 
                GetP2AGPMemoryOffset(ppdev, pSurf->lpGbl->fpVidMem, P2SurfaceFormat.dwP2PixelSize);
        }
        else 
        {
            pTexture->MipLevels[LOD].PixelOffset = 
                GetP2VideoMemoryOffset(ppdev, pSurf->lpGbl->fpVidMem, P2SurfaceFormat.dwP2PixelSize);
        }
    }
    DBG_D3D((4,"Storing LOD: %d, Pitch: %d, Width: %d PixelOffset=%08lx", 
                LOD, pSurf->lpGbl->lPitch, 
                pSurf->lpGbl->wWidth,pTexture->MipLevels[LOD].PixelOffset));    
    pTexture->MipLevels[LOD].ulPackedPP     = PARTIAL_PRODUCTS(pTexture->wWidth);
    pTexture->MipLevels[LOD].logWidth       = log2((int)pSurf->lpGbl->wWidth);
    pTexture->MipLevels[LOD].logHeight      = log2((int)pSurf->lpGbl->wHeight);

    if ((pTexture->ddpfSurface.dwFlags & DDPF_ALPHAPIXELS)&&
                                 (pTexture->ddpfSurface.dwRGBAlphaBitMask))
        pTexture->bAlpha = TRUE;
    else
        pTexture->bAlpha = FALSE;

    DBG_D3D((10,"Exiting StorePermediaLODLevel"));

} // StorePermediaLODLevel

/*
 * Function:    LoadManagedTextureToVidMem
 * Description: Loads the Managed Texture copy from system memory
 *              to Video Memory.
 */

void LoadManagedTextureToVidMem(LPP2THUNKEDDATA      pThisDisplay,
                                PERMEDIA_D3DTEXTURE  *pTexture)
{
    RECTL               rect;
    PP2_MANAGED_TEXTURE pP2ManagedTexture;
    P2_SURFACE_FORMAT   P2SurfaceFormat;

    rect.left   = rect.top = 0;
    rect.right  = pTexture->wWidth;
    rect.bottom = pTexture->wHeight;

    GetP2SurfaceFormat(&pTexture->ddpfSurface,&P2SurfaceFormat);

    pP2ManagedTexture           = &pTexture->P2ManagedTexture;
    pP2ManagedTexture->dwFlags  &= ~P2_SURFACE_NEEDUPDATE;

    pTexture->MipLevels[0].PixelOffset = 
        GetP2VideoMemoryOffset(pThisDisplay, pP2ManagedTexture->fpVidMem, P2SurfaceFormat.dwP2PixelSize);

    pTexture->MipLevels[0].ulPackedPP     = PARTIAL_PRODUCTS(pTexture->wWidth);
    pTexture->MipLevels[0].logWidth       = log2((int)pTexture->wWidth);
    pTexture->MipLevels[0].logHeight      = log2((int)pTexture->wHeight);

    SysMemToVidMemPackedDownload(pThisDisplay,
                                 pTexture->fpVidMem,
                                 pTexture->lPitch,
                                 &rect,
                                 pP2ManagedTexture->fpVidMem,
                                 pTexture->wWidth,
                                 &rect,
                                 pTexture->ddpfSurface.dwRGBBitCount);

    if ((pTexture->ddpfSurface.dwFlags & DDPF_ALPHAPIXELS)&&
                                 (pTexture->ddpfSurface.dwRGBAlphaBitMask))
        pTexture->bAlpha = TRUE;
    else
        pTexture->bAlpha = FALSE;

    DBG_D3D((10, "Copy from %08lx to %08lx w=%08lx h=%08lx p=%08lx b=%08lx",
        pTexture->fpVidMem,pP2ManagedTexture->fpVidMem,pTexture->wWidth,
        pTexture->wHeight,pTexture->lPitch,pTexture->ddpfSurface.dwRGBBitCount));
}

#if DEBUG
/*
 * Function:    DbgNotHandledRenderStates
 * Description: Debug output for render states not handled by the driver.
 */
void DbgNotHandledRenderStates(DWORD renderState, DWORD dwRSVal)
{
      
    switch(renderState)
    {   
        case D3DRENDERSTATE_ANTIALIAS:
            DBG_D3D((8, "ChangeState: AntiAlias 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_LINEPATTERN:
            DBG_D3D((8, "ChangeState: Line Pattern "
                        "(D3DLINEPATTERN) 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;
            
        case D3DRENDERSTATE_ALPHAREF:
            DBG_D3D((8, "ChangeState: Alpha Reference "
                        "(D3DFIXED) 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_ALPHAFUNC:
            DBG_D3D((8, "ChangeState: Alpha compare function "
                        "(D3DCMPFUNC) 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_ALPHATESTENABLE:
            DBG_D3D((8, "ChangeState: Alpha Test Enable "
                        "(BOOL) 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_MIPMAPLODBIAS:
            DBG_D3D((8, "ChangeState: Mipmap LOD Bias "
                        "(INT) 0x%x", dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_ROP2:
            DBG_D3D((8, "ChangeState: ROP (D3DROP2) 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_ZVISIBLE:
            // From DX6 onwards this is an obsolete render state. 
            // The D3D runtime does not support it anymore so drivers 
            // don't need to implement it
            DBG_D3D((8, "ChangeState: Z Visible 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;
        case D3DRENDERSTATE_SUBPIXEL:
            DBG_D3D((8, "ChangeState: SubPixel Correction "
                        "(BOOL) 0x%x", dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_SUBPIXELX:
            DBG_D3D((8, "ChangeState: SubPixel Correction (xOnly) "
                        "(BOOL) 0x%x", dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;
        case D3DRENDERSTATE_CLIPPING:
            DBG_D3D((8, "ChangeState: Clipping 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_LIGHTING:
            DBG_D3D((8, "ChangeState: Lighting 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_EXTENTS:
            DBG_D3D((8, "ChangeState: Extents 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_AMBIENT:
            DBG_D3D((8, "ChangeState: Ambient 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_FOGVERTEXMODE:
            DBG_D3D((8, "ChangeState: Fog Vertex Mode 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_COLORVERTEX:
            DBG_D3D((8, "ChangeState: Color Vertex 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_LOCALVIEWER:
            DBG_D3D((8, "ChangeState: LocalViewer 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_NORMALIZENORMALS:
            DBG_D3D((8, "ChangeState: Normalize Normals 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_COLORKEYBLENDENABLE:
            DBG_D3D((8, "ChangeState: Colorkey Blend Enable 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_DIFFUSEMATERIALSOURCE:
            DBG_D3D((8, "ChangeState: Diffuse Material Source 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_SPECULARMATERIALSOURCE:
            DBG_D3D((8, "ChangeState: Specular Material Source 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_AMBIENTMATERIALSOURCE:
            DBG_D3D((8, "ChangeState: Ambient Material Source 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE:
            DBG_D3D((8, "ChangeState: Emmisive Material Source 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_VERTEXBLEND:
            DBG_D3D((8, "ChangeState: Vertex Blend 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_CLIPPLANEENABLE:
            DBG_D3D((8, "ChangeState: Clip Plane Enable 0x%x",dwRSVal));
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;

        case D3DRENDERSTATE_SCENECAPTURE:
            // This state pass TRUE or FALSE to replace the functionality
            // of D3DHALCallbacks->SceneCapture(), Permedia2 Hardware doesn't
            // need begin/end scene information, therefore it's a NOOP here.
            DBG_D3D((4, "    **Not Currently Handled**"));;
        break;
    }
}

/*
 * Function:    DecodeBlend
 * Description: produce debug output for D3D blend modes
 */

VOID 
DecodeBlend( LONG Level, DWORD i )
{
    switch ((D3DBLEND)i)
    {
    case D3DBLEND_ZERO:
        DISPDBG((Level, "  ZERO"));
        break;
    case D3DBLEND_ONE:
        DISPDBG((Level, "  ONE"));
        break;
    case D3DBLEND_SRCCOLOR:
        DISPDBG((Level, "  SRCCOLOR"));
        break;
    case D3DBLEND_INVSRCCOLOR:
        DISPDBG((Level, "  INVSRCCOLOR"));
        break;
    case D3DBLEND_SRCALPHA:
        DISPDBG((Level, "  SRCALPHA"));
        break;
    case D3DBLEND_INVSRCALPHA:
        DISPDBG((Level, "  INVSRCALPHA"));
        break;
    case D3DBLEND_DESTALPHA:
        DISPDBG((Level, "  DESTALPHA"));
        break;
    case D3DBLEND_INVDESTALPHA:
        DISPDBG((Level, "  INVDESTALPHA"));
        break;
    case D3DBLEND_DESTCOLOR:
        DISPDBG((Level, "  DESTCOLOR"));
        break;
    case D3DBLEND_INVDESTCOLOR:
        DISPDBG((Level, "  INVDESTCOLOR"));
        break;
    case D3DBLEND_SRCALPHASAT:
        DISPDBG((Level, "  SRCALPHASAT"));
        break;
    case D3DBLEND_BOTHSRCALPHA:
        DISPDBG((Level, "  BOTHSRCALPHA"));
        break;
    case D3DBLEND_BOTHINVSRCALPHA:
        DISPDBG((Level, "  BOTHINVSRCALPHA"));
        break;
    }
}  /* DecodeBlend */
#endif
