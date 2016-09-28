/******************************Module*Header**********************************\
 *
 *                           *****************************************
 *                           * Permedia 2: D3D and DDraw SAMPLE CODE *
 *                           *****************************************
 *
 * Module Name: p2cntxt.c
 *
 * This module contains the Context Switching code for Permedia 2. 
 * It should be noted that the 16bit - GDI portion does not save any contexts
 * It simply restores to the GDI context - i.e., any registers saved by D3D
 * and/or DDraw are restored. 
 * To start without DDraw or D3D active, we run in GDI context. Thereafter when 
 * Ddraw or D3D does a rendering operation we save the necessary registers
 * initialized by GDI. We then load the appropriate registers and do the rendering.
 * Depending on the type of rendering operation by Direct Draw we have atmost two
 * sets of registers to be saved. These are represented by D3D_OR_DDRAW_CONTEXT0 and
 * D3D_OR_DDRAW_CONTEXT1. The registers to be saved when D3D needs to render is 
 * represented by D3D_OR_DDRAW_CONTEXT2. D3D_OR_DDRAW_CONTEXT0 is a proper subset of
 * D3D_OR_DDRAW_CONTEXT1. Also D3D_OR_DDRAW_CONTEXT1 is a proper subset of D3D_OR_DDRAW_CONTEXT2.
 * So if the last saved register context was D3D_OR_DDRAW_CONTEXT1 and, D3D needs
 * to do a rendering operation we need to just save the registers in the set
 * (D3D_OR_DDRAW_CONTEXT2 - D3D_OR_DDRAW_CONTEXT1). If the last saved register context
 * was D3D_OR_DDRAW_CONTEXT2 and Direct Draw needs to do a render operation which requires
 * D3D_OR_DDRAW_CONTEXT1 to be saved, we then restore the registers in
 * (D3D_OR_DDRAW_CONTEXT2 - D3D_OR_DDRAW_CONTEXT1).
 * 
 * In each case we keep track of which D3D_OR_DDRAW_CONTEXTN was the last saved context.
 * When GDI needs to render the approriate stored D3D_OR_DDRAW_CONTEXTN context is restored.
 * So essentially the saved context D3D_OR_DDRAW_CONTEXTN denotes the minimal register set
 * which needs to be restored when a D3d/DDraw ---> GDI switch occurs.
 * 
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#include "precomp.h"

/*
 * Inline to set Register Mode, Scissor Mode and FBWriteMode registers to some default
 * values while switching to a Direct Draw / Direct3D context.
 */
_inline void P2DDRegInit(LPP2THUNKEDDATA  pThisDisplay)
{
    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 4); 
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, RasterizerMode,  __PERMEDIA_DISABLE);    //Rasterizer
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ScissorMode,     __PERMEDIA_DISABLE);    //Scissor/Stipple
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWriteMode,     __PERMEDIA_ENABLE);     //FB Write
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBPixelOffset,           0);  
}

/*
 * Function:    P2DDorD3DContextInit
 * Description: Saves the appropriate Permedia 2 registers if needed while switching 
 *              while switching between (GDI, DDRAW, D3D) <---> (GDI, DDRAW, D3D)
 *              These saved registers are the minimal set required to restore to GDI
 *              context.
 */
VOID
P2DDorD3DContextInit(LPP2THUNKEDDATA  pThisDisplay, ULONG ContextToSave)
{
    PP2_REGISTER_CONTEXT pRegContext = &pThisDisplay->P2RegisterContext;

    DBG_DD(( 4, "Enter P2DDorD3DContextInit Current=0x%x ToSave=0x%x", pThisDisplay->dwD3DorDDContext, ContextToSave));

    if (pThisDisplay->dwD3DorDDContext == ContextToSave)
    {
        if (pThisDisplay->dwD3DorDDContext != D3D_OR_DDRAW_CONTEXT2)
            pThisDisplay->dwCurrentD3DContext = 0;
        return ;
    }

    pThisDisplay->dwCurrentD3DContext = 0;
    vSyncWithPermedia(pThisDisplay);
    while (bDrawEngineBusy(pThisDisplay));

    if (ContextToSave == D3D_OR_DDRAW_CONTEXT0)
    {
        if (pThisDisplay->dwD3DorDDContext < D3D_OR_DDRAW_CONTEXT0)
        {
            pRegContext->dwRasterizerMode   = READ_PERMEDIA2_REGISTER(pThisDisplay, RasterizerMode);
            pRegContext->dwScissorMode      = READ_PERMEDIA2_REGISTER(pThisDisplay, ScissorMode);
            pRegContext->dwLogicalOpMode    = READ_PERMEDIA2_REGISTER(pThisDisplay, LogicalOpMode);
            pRegContext->dwFBReadPixel      = READ_PERMEDIA2_REGISTER(pThisDisplay, FBReadPixel);
            pRegContext->dwFBWriteMode      = READ_PERMEDIA2_REGISTER(pThisDisplay, FBWriteMode);
            pRegContext->dwFBWindowBase     = READ_PERMEDIA2_REGISTER(pThisDisplay, FBWindowBase);
            pRegContext->dwFBReadMode       = READ_PERMEDIA2_REGISTER(pThisDisplay, FBReadMode);
            pRegContext->dwFBWriteConfig    = READ_PERMEDIA2_REGISTER(pThisDisplay, FBWriteConfig);

            DBG_DD(( 4, "\t RasterizerMode 0x%x",       pRegContext->dwRasterizerMode));
            DBG_DD(( 4, "\t ScissorMode 0x%x",          pRegContext->dwScissorMode));
            DBG_DD(( 4, "\t LogicalOpMode 0x%x",        pRegContext->dwLogicalOpMode));
            DBG_DD(( 4, "\t FBReadPixel 0x%x",          pRegContext->dwFBReadPixel));
            DBG_DD(( 4, "\t FBWriteMode 0x%x",          pRegContext->dwFBWriteMode));
            DBG_DD(( 4, "\t FBWindowBase 0x%x",         pRegContext->dwFBWindowBase));
            DBG_DD(( 4, "\t FBReadMode 0x%x",           pRegContext->dwFBReadMode));

            pThisDisplay->dwD3DorDDContext = D3D_OR_DDRAW_CONTEXT0;
        }
        else
        {
            if (pThisDisplay->dwD3DorDDContext == D3D_OR_DDRAW_CONTEXT2)
            {   
                WAIT_FOR_FREE_ENTRIES(pThisDisplay, 33);
                // Delta
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DeltaMode,               0);  
                // Rasterizer
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, RasterizerMode,  __PERMEDIA_DISABLE);    //Rasterizer
                // Scissor/Stipple
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ScissorMode,     __PERMEDIA_DISABLE);    //Scissor/Stipple
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStippleMode,         0);
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern0,     pRegContext->AreaStipplePattern[0]);
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern1,     pRegContext->AreaStipplePattern[1]);
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern2,     pRegContext->AreaStipplePattern[2]);
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern3,     pRegContext->AreaStipplePattern[3]);
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern4,     pRegContext->AreaStipplePattern[4]);
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern5,     pRegContext->AreaStipplePattern[5]);
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern6,     pRegContext->AreaStipplePattern[6]);
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern7,     pRegContext->AreaStipplePattern[7]);
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStippleMode,         pRegContext->dwAreaStippleMode);
                // local buffer
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBReadMode,              0);  
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBWriteMode,             0);  
                // stencil/depth
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StencilMode,             0);  
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DepthMode,               0);  
                // texture address
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdx,                    0);  
                // texture read
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTIndex,           0);  
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTMode ,           0);  
                // yuv
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, YUVMode,                 0); 
                // local buffer write
                // frame buffer read
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBPixelOffset,           0);  
                // color DDA
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ColorDDAMode,            pRegContext->dwColorDDAMode);  
                // Texture/Fog/Blend
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AlphaBlendMode,          pRegContext->dwAlphaBlendMode); 
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FogMode,                 0);  
                // Color Format
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode,              0); 
                // Logic Ops
                // Frame Buffer Write
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBHardwareWriteMask,     pRegContext->dwFBHardwareWriteMask);  
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWriteMode,     __PERMEDIA_ENABLE);     //FB Write
                // Host Out
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FilterMode,              0);  

                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXDom,                   0);  
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXSub,                   0);  
                LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,                      0x10000); 

                pThisDisplay->dwD3DorDDContext = D3D_OR_DDRAW_CONTEXT1;
                vSyncWithPermedia(pThisDisplay);
                return;
            }
        }
        P2DDRegInit(pThisDisplay);
        vSyncWithPermedia(pThisDisplay);
        return;
    }

    if (ContextToSave == D3D_OR_DDRAW_CONTEXT1)
    {
        if (pThisDisplay->dwD3DorDDContext < D3D_OR_DDRAW_CONTEXT0)
        {
            pRegContext->dwRasterizerMode   = READ_PERMEDIA2_REGISTER(pThisDisplay, RasterizerMode);
            pRegContext->dwScissorMode      = READ_PERMEDIA2_REGISTER(pThisDisplay, ScissorMode);
            pRegContext->dwLogicalOpMode    = READ_PERMEDIA2_REGISTER(pThisDisplay, LogicalOpMode);
            pRegContext->dwFBReadPixel      = READ_PERMEDIA2_REGISTER(pThisDisplay, FBReadPixel);
            pRegContext->dwFBWriteMode      = READ_PERMEDIA2_REGISTER(pThisDisplay, FBWriteMode);
            pRegContext->dwFBWindowBase     = READ_PERMEDIA2_REGISTER(pThisDisplay, FBWindowBase);
            pRegContext->dwFBReadMode       = READ_PERMEDIA2_REGISTER(pThisDisplay, FBReadMode);
            pRegContext->dwFBWriteConfig    = READ_PERMEDIA2_REGISTER(pThisDisplay, FBWriteConfig);

            DBG_DD(( 4, "\t RasterizerMode 0x%x",       pRegContext->dwRasterizerMode));
            DBG_DD(( 4, "\t ScissorMode 0x%x",          pRegContext->dwScissorMode));
            DBG_DD(( 4, "\t LogicalOpMode 0x%x",        pRegContext->dwLogicalOpMode));
            DBG_DD(( 4, "\t FBReadPixel 0x%x",          pRegContext->dwFBReadPixel));
            DBG_DD(( 4, "\t FBWriteMode 0x%x",          pRegContext->dwFBWriteMode));
            DBG_DD(( 4, "\t FBWindowBase 0x%x",         pRegContext->dwFBWindowBase));
            DBG_DD(( 4, "\t FBReadMode 0x%x",           pRegContext->dwFBReadMode));

        }

        if (pThisDisplay->dwD3DorDDContext < D3D_OR_DDRAW_CONTEXT1)
        {
            pRegContext->dwTextureAddressMode   = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureAddressMode);
            pRegContext->dwTextureColorMode     = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureColorMode);
            pRegContext->dwTextureReadMode      = READ_PERMEDIA2_REGISTER(pThisDisplay, FxTextureReadMode);
            pRegContext->dwTextureDataFormat    = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureDataFormat);
            pRegContext->dwTextureBaseAddress   = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureBaseAddress);
            pRegContext->dwTextureMapFormat     = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureMapFormat);
            pRegContext->dwSStart               = READ_PERMEDIA2_REGISTER(pThisDisplay, SStart);
            pRegContext->dwTStart               = READ_PERMEDIA2_REGISTER(pThisDisplay, TStart);
            pRegContext->dwdSdx                 = READ_PERMEDIA2_REGISTER(pThisDisplay, dSdx);
            pRegContext->dwdTdyDom              = READ_PERMEDIA2_REGISTER(pThisDisplay, dTdyDom);

            DBG_DD(( 4, "\t TextureAddressMode 0x%x",   pRegContext->dwTextureAddressMode));
            DBG_DD(( 4, "\t TextureColorMode 0x%x",     pRegContext->dwTextureColorMode));
            DBG_DD(( 4, "\t TextureReadMode 0x%x",      pRegContext->dwTextureReadMode));
            DBG_DD(( 4, "\t TextureDataFormat 0x%x",    pRegContext->dwTextureDataFormat));
            DBG_DD(( 4, "\t TextureBaseAddress 0x%x",   pRegContext->dwTextureBaseAddress));
            DBG_DD(( 4, "\t TextureMapFormat 0x%x",     pRegContext->dwTextureMapFormat));
            DBG_DD(( 4, "\t SStart 0x%x",               pRegContext->dwSStart));
            DBG_DD(( 4, "\t TStart 0x%x",               pRegContext->dwTStart));
            DBG_DD(( 4, "\t dSdx 0x%x",                 pRegContext->dwdSdx));
            DBG_DD(( 4, "\t dTdyDom 0x%x",              pRegContext->dwdTdyDom));
        }

        if (pThisDisplay->dwD3DorDDContext == D3D_OR_DDRAW_CONTEXT2)
        {   
            WAIT_FOR_FREE_ENTRIES(pThisDisplay, 33);
            // Delta
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DeltaMode,               0);  
            // Rasterizer
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, RasterizerMode,  __PERMEDIA_DISABLE);    //Rasterizer
            // Scissor/Stipple
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ScissorMode,     __PERMEDIA_DISABLE);    //Scissor/Stipple
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStippleMode,         0);
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern0,     pRegContext->AreaStipplePattern[0]);
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern1,     pRegContext->AreaStipplePattern[1]);
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern2,     pRegContext->AreaStipplePattern[2]);
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern3,     pRegContext->AreaStipplePattern[3]);
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern4,     pRegContext->AreaStipplePattern[4]);
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern5,     pRegContext->AreaStipplePattern[5]);
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern6,     pRegContext->AreaStipplePattern[6]);
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStipplePattern7,     pRegContext->AreaStipplePattern[7]);
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStippleMode,         pRegContext->dwAreaStippleMode);
            // local buffer
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBReadMode,              0);  
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBWriteMode,             0);  
            // stencil/depth
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StencilMode,             0);  
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DepthMode,               0);  
            // texture address
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdx,                    0);  
            // texture read
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTIndex,           0);  
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTMode ,           0);  
            // yuv
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, YUVMode,                 0); 
            // local buffer write
            // frame buffer read
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBPixelOffset,           0);  
            // color DDA
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ColorDDAMode,            pRegContext->dwColorDDAMode);  
            // Texture/Fog/Blend
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AlphaBlendMode,          pRegContext->dwAlphaBlendMode); 
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FogMode,                 0);  
            // Color Format
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode,              0); 
            // Logic Ops
            // Frame Buffer Write
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBHardwareWriteMask,     pRegContext->dwFBHardwareWriteMask);  
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWriteMode,     __PERMEDIA_ENABLE);     //FB Write
            // Host Out
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FilterMode,              0);  

            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXDom,                   0);  
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXSub,                   0);  
            LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,                      0x10000); 
            
            pThisDisplay->dwD3DorDDContext = D3D_OR_DDRAW_CONTEXT1;
            vSyncWithPermedia(pThisDisplay);
            return;
        }
        pThisDisplay->dwD3DorDDContext = D3D_OR_DDRAW_CONTEXT1;
        P2DDRegInit(pThisDisplay);
        vSyncWithPermedia(pThisDisplay);
        return;
    }

    if (ContextToSave == D3D_OR_DDRAW_CONTEXT2)
    {
        if (pThisDisplay->dwD3DorDDContext < D3D_OR_DDRAW_CONTEXT0)
        {
            pRegContext->dwRasterizerMode   = READ_PERMEDIA2_REGISTER(pThisDisplay, RasterizerMode);
            pRegContext->dwScissorMode      = READ_PERMEDIA2_REGISTER(pThisDisplay, ScissorMode);
            pRegContext->dwLogicalOpMode    = READ_PERMEDIA2_REGISTER(pThisDisplay, LogicalOpMode);
            pRegContext->dwFBReadPixel      = READ_PERMEDIA2_REGISTER(pThisDisplay, FBReadPixel);
            pRegContext->dwFBWriteMode      = READ_PERMEDIA2_REGISTER(pThisDisplay, FBWriteMode);
            pRegContext->dwFBWindowBase     = READ_PERMEDIA2_REGISTER(pThisDisplay, FBWindowBase);
            pRegContext->dwFBReadMode       = READ_PERMEDIA2_REGISTER(pThisDisplay, FBReadMode);
            pRegContext->dwFBWriteConfig    = READ_PERMEDIA2_REGISTER(pThisDisplay, FBWriteConfig);

            DBG_DD(( 4, "\t RasterizerMode 0x%x",       pRegContext->dwRasterizerMode));
            DBG_DD(( 4, "\t ScissorMode 0x%x",          pRegContext->dwScissorMode));
            DBG_DD(( 4, "\t LogicalOpMode 0x%x",        pRegContext->dwLogicalOpMode));
            DBG_DD(( 4, "\t FBReadPixel 0x%x",          pRegContext->dwFBReadPixel));
            DBG_DD(( 4, "\t FBWriteMode 0x%x",          pRegContext->dwFBWriteMode));
            DBG_DD(( 4, "\t FBWindowBase 0x%x",         pRegContext->dwFBWindowBase));
            DBG_DD(( 4, "\t FBReadMode 0x%x",           pRegContext->dwFBReadMode));
        }

        if (pThisDisplay->dwD3DorDDContext < D3D_OR_DDRAW_CONTEXT1)
        {
            pRegContext->dwTextureAddressMode   = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureAddressMode);
            pRegContext->dwTextureColorMode     = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureColorMode);
            pRegContext->dwTextureReadMode      = READ_PERMEDIA2_REGISTER(pThisDisplay, FxTextureReadMode);
            pRegContext->dwTextureDataFormat    = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureDataFormat);
            pRegContext->dwTextureBaseAddress   = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureBaseAddress);
            pRegContext->dwTextureMapFormat     = READ_PERMEDIA2_REGISTER(pThisDisplay, TextureMapFormat);
            pRegContext->dwSStart               = READ_PERMEDIA2_REGISTER(pThisDisplay, SStart);
            pRegContext->dwTStart               = READ_PERMEDIA2_REGISTER(pThisDisplay, TStart);
            pRegContext->dwdSdx                 = READ_PERMEDIA2_REGISTER(pThisDisplay, dSdx);
            pRegContext->dwdTdyDom              = READ_PERMEDIA2_REGISTER(pThisDisplay, dTdyDom);

            DBG_DD(( 4, "\t TextureAddressMode 0x%x",   pRegContext->dwTextureAddressMode));
            DBG_DD(( 4, "\t TextureColorMode 0x%x",     pRegContext->dwTextureColorMode));
            DBG_DD(( 4, "\t TextureReadMode 0x%x",      pRegContext->dwTextureReadMode));
            DBG_DD(( 4, "\t TextureDataFormat 0x%x",    pRegContext->dwTextureDataFormat));
            DBG_DD(( 4, "\t TextureBaseAddress 0x%x",   pRegContext->dwTextureBaseAddress));
            DBG_DD(( 4, "\t TextureMapFormat 0x%x",     pRegContext->dwTextureMapFormat));
            DBG_DD(( 4, "\t SStart 0x%x",               pRegContext->dwSStart));
            DBG_DD(( 4, "\t TStart 0x%x",               pRegContext->dwTStart));
            DBG_DD(( 4, "\t dSdx 0x%x",                 pRegContext->dwdSdx));
            DBG_DD(( 4, "\t dTdyDom 0x%x",              pRegContext->dwdTdyDom));
        }

        pRegContext->dwAlphaBlendMode       = READ_PERMEDIA2_REGISTER(pThisDisplay, AlphaBlendMode);
        pRegContext->dwAreaStippleMode      = READ_PERMEDIA2_REGISTER(pThisDisplay, AreaStippleMode);
        pRegContext->dwColorDDAMode         = READ_PERMEDIA2_REGISTER(pThisDisplay, ColorDDAMode);
        pRegContext->dwFBHardwareWriteMask  = READ_PERMEDIA2_REGISTER(pThisDisplay, FBHardwareWriteMask);
        pRegContext->AreaStipplePattern[0]  = READ_PERMEDIA2_REGISTER(pThisDisplay, AreaStipplePattern0);
        pRegContext->AreaStipplePattern[1]  = READ_PERMEDIA2_REGISTER(pThisDisplay, AreaStipplePattern1);
        pRegContext->AreaStipplePattern[2]  = READ_PERMEDIA2_REGISTER(pThisDisplay, AreaStipplePattern2);
        pRegContext->AreaStipplePattern[3]  = READ_PERMEDIA2_REGISTER(pThisDisplay, AreaStipplePattern3);
        pRegContext->AreaStipplePattern[4]  = READ_PERMEDIA2_REGISTER(pThisDisplay, AreaStipplePattern4);
        pRegContext->AreaStipplePattern[5]  = READ_PERMEDIA2_REGISTER(pThisDisplay, AreaStipplePattern5);
        pRegContext->AreaStipplePattern[6]  = READ_PERMEDIA2_REGISTER(pThisDisplay, AreaStipplePattern6);
        pRegContext->AreaStipplePattern[7]  = READ_PERMEDIA2_REGISTER(pThisDisplay, AreaStipplePattern7);

        DBG_DD(( 4, "\t AlphaBlendMode 0x%x",       pRegContext->dwAlphaBlendMode));
        DBG_DD(( 4, "\t AreaStippleMode 0x%x",      pRegContext->dwAreaStippleMode));
        DBG_DD(( 4, "\t ColorDDAMode 0x%x",         pRegContext->dwColorDDAMode));
        DBG_DD(( 4, "\t FBHardwareWriteMask 0x%x",  pRegContext->dwFBHardwareWriteMask));

        P2DDRegInit(pThisDisplay);
        pThisDisplay->dwD3DorDDContext = D3D_OR_DDRAW_CONTEXT2;
        vSyncWithPermedia(pThisDisplay);
       return;
    }

    return;
}

/*
 * Function:    DrawPrim2ContextRegInit
 * Description: Initializes the Permedia2 registers to setup a rendering context
 */
VOID
DrawPrim2ContextRegInit(PERMEDIA_D3DCONTEXT  *pContext)
{
    PP2REG_SOFTWARE_COPY    pSoftCopyP2Regs = &pContext->SoftCopyP2Regs;
    LPP2THUNKEDDATA         pThisDisplay    = pContext->pThisDisplay;
    DWORD                   i, offset;
    DWORD                   dwAlphaBlendSend;

    if ((pThisDisplay->dwCurrentD3DContext == (DWORD)pContext) &&
        (!pContext->dwReloadContext))
        return;

    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 60);
    // Delta
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DeltaMode,           pSoftCopyP2Regs->dwDeltaMode);
    // Rasterizer
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, RasterizerMode,      pSoftCopyP2Regs->dwRasterizerMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,                  pSoftCopyP2Regs->dY);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXDom,               0x0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXSub,               0x0);
    // Scissor/Stipple
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ScissorMode,         pSoftCopyP2Regs->dwScissorMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AreaStippleMode,     pSoftCopyP2Regs->dwAreaStippleMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, WindowOrigin,        pSoftCopyP2Regs->dwWindowOrigin);
    for (i = 0, offset = pSoftCopyP2Regs->AreaStipplePattern.startIndex; 
            i < pSoftCopyP2Regs->AreaStipplePattern.count; 
            i++)
    {
        LOAD_INPUT_FIFO_TAG_OFFSET_DATA(pThisDisplay,
                                        AreaStipplePattern0,
                                        pSoftCopyP2Regs->AreaStipplePattern.Pattern[i],
                                        offset++);
    }
    // local buffer
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBReadMode,          pSoftCopyP2Regs->dwLBReadMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBReadFormat,        pSoftCopyP2Regs->dwLBReadFormat);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBWriteFormat,       pSoftCopyP2Regs->dwLBWriteFormat);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBWriteMode,         pSoftCopyP2Regs->dwLBWriteMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBWindowBase,        pSoftCopyP2Regs->dwLBWindowBase);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LBSourceOffset,  pSoftCopyP2Regs->dwLBSourceOffset);
    // stencil/depth
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DepthMode,           pSoftCopyP2Regs->dwDepthMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StencilMode,         pSoftCopyP2Regs->dwStencilMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StencilData,         pSoftCopyP2Regs->dwStencilData);
    // texture address
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureAddressMode,  pSoftCopyP2Regs->dwTextureAddressMode);
    // texture read
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureReadMode,     pSoftCopyP2Regs->dwTextureReadMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTMode,        pSoftCopyP2Regs->dwTexelLUTMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureDataFormat,   pSoftCopyP2Regs->dwTextureDataFormat);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureMapFormat,    pSoftCopyP2Regs->dwTextureMapFormat);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureBaseAddress,  pSoftCopyP2Regs->dwTextureBaseAddress);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TexelLUTIndex,       0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AlphaMapUpperBound,  pSoftCopyP2Regs->dwAlphaMapUpperBound);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AlphaMapLowerBound,  pSoftCopyP2Regs->dwAlphaMapLowerBound);
    // yuv
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, YUVMode,             pSoftCopyP2Regs->dwYUVMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ChromaUpperBound,    pSoftCopyP2Regs->dwChromaUpperBound);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ChromaLowerBound,    pSoftCopyP2Regs->dwChromaLowerBound);
    // local buffer write
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadMode,          pSoftCopyP2Regs->dwFBReadMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWriteMode,         pSoftCopyP2Regs->dwFBWriteMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWindowBase,        pSoftCopyP2Regs->dwFBWindowBase);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadPixel,         pSoftCopyP2Regs->FBReadPixel);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWindowBase,        pSoftCopyP2Regs->dwFBWindowBase);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBSoftwareWriteMask, pSoftCopyP2Regs->dwFBSoftwareWriteMask);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBHardwareWriteMask, pSoftCopyP2Regs->dwFBHardwareWriteMask);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBPixelOffset,       pSoftCopyP2Regs->dwFBPixelOffset);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBSourceOffset,      pSoftCopyP2Regs->dwFBSourceOffset);
    // frame buffer read
    // color DDA
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ColorDDAMode,        pSoftCopyP2Regs->dwColorDDAMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AStart,          PM_BYTE_COLOR(0xFF));
    // Texture/Fog/Blend
    dwAlphaBlendSend    =   pSoftCopyP2Regs->dwAlphaBlendMode;
    if (FAKE_ALPHABLEND_ONE_ONE & pContext->FakeBlendNum)
    {
        dwAlphaBlendSend &= 0xFFFFFF01;
        dwAlphaBlendSend |= (__PERMEDIA_BLENDOP_ONE_AND_INVSRCALPHA << 1);
    }

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, AlphaBlendMode,      dwAlphaBlendSend);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FogMode,             pSoftCopyP2Regs->dwFogMode);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FogColor,            pSoftCopyP2Regs->FogColor);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureColorMode,    pSoftCopyP2Regs->dwTextureColorMode);
    // Color Format
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode,          pSoftCopyP2Regs->dwDitherMode);
    // Logic Ops
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LogicalOpMode,       0);
    // Frame Buffer Write
    // Host Out
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FilterMode,          pSoftCopyP2Regs->dwFilterMode);

    pThisDisplay->dwCurrentD3DContext = (DWORD)pContext;
    pContext->dwReloadContext         = 0;
    vSyncWithPermedia(pThisDisplay);

}



