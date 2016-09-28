/******************************Module*Header**********************************\
*
*                           *************************************
*                           * Permedia 2: Direct 3D SAMPLE CODE *
*                           *************************************
*
* Module Name: p2d3dln.c
*
* Content:    Direct3D hw line rasterization code.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"



/*
 * Function:    P2DrawFVFLine
 * Description: Hardare render a single line coming from two FVF vertices
 *              Primitive rendering at this stage is dependent upon the 
 *              current value/setting of texturing, perspective correction, 
 *              fogging, gouraud/flat shading, and specular highlights.
 */

VOID
P2DrawFVFLine(  PERMEDIA_D3DCONTEXT *pContext, 
                LPD3DTLVERTEX       lpV0, 
                LPD3DTLVERTEX       lpV1,
                LPD3DTLVERTEX       lpVFlat,
                LPP2FVFOFFSETS      lpFVFOff)
{
    DWORD           dwFlags     = pContext->Flags;
    ULONG           ulRenderCmd = pContext->RenderCommand;
    LPP2THUNKEDDATA ppdev       = pContext->pThisDisplay;
    DWORD           dwColorOffs,dwSpecularOffs,dwTexOffs;
    D3DCOLOR        dwColor0, dwColor1;
    D3DCOLOR        dwSpec0, dwSpec1;
    D3DVALUE        fS0, fS1, fT0, fT1, fQ0, fQ1;
    D3DVALUE        fKs0, fKs1;

    DBG_D3D((10,"Entering P2DrawFVFLine"));

    // Set line rendering mode
    RENDER_LINE(ulRenderCmd);

    // Fix up the biasing in delta because delta doesn't do it for us for lines.
    WAIT_FOR_FREE_ENTRIES(ppdev, 40);
    LOAD_INPUT_FIFO_TAG_DATA(ppdev, RasterizerMode, BIAS_NONE);

    // Get FVF structure offsets
    __SetFVFOffsets(&dwColorOffs,&dwSpecularOffs,&dwTexOffs,lpFVFOff);

    // Get vertex color value (FVF based)
    if (dwColorOffs)
    {
        dwColor0  = FVFCOLOR(lpV0, dwColorOffs)->color;
        dwColor1  = FVFCOLOR(lpV1, dwColorOffs)->color;
        if (FAKE_ALPHABLEND_MODULATE & pContext->FakeBlendNum)
        {
            dwColor0  |= 0xFF000000;
            dwColor1  |= 0xFF000000;
        }
    }
    else
    {
        // must set default in case no D3DFVF_DIFFUSE
        dwColor0  = 0xFFFFFFFF;
        dwColor1  = 0xFFFFFFFF;
    }

    // Get vertex specular value (FVF based) if necessary
    if ((dwFlags & (CTXT_HAS_SPECULAR_ENABLED | CTXT_HAS_FOGGING_ENABLED))
        && (dwSpecularOffs!=0))
    {
        dwSpec0   = FVFSPEC(lpV0, dwSpecularOffs)->specular;
        dwSpec1   = FVFSPEC(lpV1, dwSpecularOffs)->specular;
    }

    if ((dwFlags & CTXT_HAS_TEXTURE_ENABLED) && (dwTexOffs != 0))
    {
         // Get s,t texture coordinates (FVF based)
        fS0 = FVFTEX(lpV0,dwTexOffs)->tu;
        fT0 = FVFTEX(lpV0,dwTexOffs)->tv;
        fS1 = FVFTEX(lpV1,dwTexOffs)->tu;
        fT1 = FVFTEX(lpV1,dwTexOffs)->tv;

        // Wrap texture coordinates if necessary
        WRAP_LINE(fS, dwFlags & CTXT_HAS_WRAPU_ENABLED);
        WRAP_LINE(fT, dwFlags & CTXT_HAS_WRAPV_ENABLED);

        // Scale s,t coordinate values
        fS0 *= pContext->DeltaWidthScale;
        fS1 *= pContext->DeltaWidthScale;
        fT0 *= pContext->DeltaHeightScale;
        fT1 *= pContext->DeltaHeightScale;

        // Apply perpspective corrections if necessary
        if (dwFlags & CTXT_HAS_PERSPECTIVE_ENABLED)
        {
            fQ0 = lpV0->rhw;
            fQ1 = lpV1->rhw;
            fS0 *= fQ0;
            fT0 *= fQ0;
            fS1 *= fQ1;
            fT1 *= fQ1;
        }
        else
        {
            fQ0 = fQ1 = 1.0;
        }

        // Send lines s,t,q,ks (conditionaly),x,y,z values
        if ((dwFlags & CTXT_HAS_SPECULAR_ENABLED) && (dwSpecularOffs!=0))
        {
            fKs0 = RGB256_TO_LUMA(RGB_GETRED(dwSpec0),
                                    RGB_GETGREEN(dwSpec0),
                                    RGB_GETBLUE(dwSpec0));
            
            if (dwFlags & CTXT_HAS_GOURAUD_ENABLED) 
            {
                fKs1 = RGB256_TO_LUMA(RGB_GETRED(dwSpec1),
                                        RGB_GETGREEN(dwSpec1),
                                        RGB_GETBLUE(dwSpec1));
            } 
            else
            {
                fKs1 = fKs0;
            }

            SEND_VERTEX_STQ_KS_XYZ(ppdev, __Permedia2TagV0FloatS, fS0, fT0, fQ0, fKs0,
                                                lpV0->sx, lpV0->sy, lpV0->sz);
            SEND_VERTEX_STQ_KS_XYZ(ppdev, __Permedia2TagV1FloatS, fS1, fT1, fQ1, fKs1,
                                                lpV1->sx, lpV1->sy, lpV1->sz);
        }
        else
        {
            SEND_VERTEX_STQ_XYZ(ppdev, __Permedia2TagV0FloatS, fS0, fT0, fQ0,
                                                lpV0->sx, lpV0->sy, lpV0->sz);
            SEND_VERTEX_STQ_XYZ(ppdev, __Permedia2TagV1FloatS, fS1, fT1, fQ1,
                                                lpV1->sx, lpV1->sy, lpV1->sz);
        }
    }
    else    // not textured line
    {
        // If specular is enabled, change the colours
        if ((dwFlags & CTXT_HAS_SPECULAR_ENABLED) && (dwSpecularOffs!=0))
        {
            CLAMP8888(dwColor0, dwColor0, dwSpec0 );
            CLAMP8888(dwColor1, dwColor1, dwSpec1 );
        }

        // Send lines x,y,z values
        SEND_VERTEX_XYZ(ppdev, __Permedia2TagV0FloatS, lpV0->sx, lpV0->sy, lpV0->sz);
        SEND_VERTEX_XYZ(ppdev, __Permedia2TagV1FloatS, lpV1->sx, lpV1->sy, lpV1->sz);
    }

    // If fog is set, send the appropriate values
    if ((dwFlags & CTXT_HAS_FOGGING_ENABLED) && (dwSpecularOffs!=0))
    {
        SEND_VERTEX_FOG(ppdev, __Permedia2TagV0FixedF, RGB_GET_GAMBIT_FOG(dwSpec0));
        SEND_VERTEX_FOG(ppdev, __Permedia2TagV1FixedF, RGB_GET_GAMBIT_FOG(dwSpec1));
    }

    // Send appropriate color depending on Gouraud , Mono, & Alpha
    if (dwFlags & CTXT_HAS_GOURAUD_ENABLED)
    {
        // Gouraud shading
        if (RENDER_MONO)
        {
            SEND_VERTEX_RGB_MONO_P2(ppdev, __Permedia2TagV0FixedS, dwColor0);
            SEND_VERTEX_RGB_MONO_P2(ppdev, __Permedia2TagV1FixedS, dwColor1);
        }
        else
        {
            if (dwFlags & CTXT_HAS_ALPHABLEND_ENABLED)
            {
                if (pContext->FakeBlendNum & FAKE_ALPHABLEND_ONE_ONE)
                {
                    dwColor0 &= 0xFFFFFF;     // supress color's alpha value
                    dwColor1 &= 0xFFFFFF;
                }
            }
            SEND_VERTEX_RGBA_P2(ppdev, __Permedia2TagV0FixedS, dwColor0);
            SEND_VERTEX_RGBA_P2(ppdev, __Permedia2TagV1FixedS, dwColor1);
        }
    }
    else   // Flat shading
    {
        // Get vertex color value (FVF based) from the right vertex!
        if (dwColorOffs)
        {
            dwColor0  = FVFCOLOR(lpVFlat, dwColorOffs)->color;
            if (FAKE_ALPHABLEND_MODULATE & pContext->FakeBlendNum)
            {
                dwColor0  |= 0xFF000000;
            }
        }
        else
        {
            // must set default in case no D3DFVF_DIFFUSE
            dwColor0  = 0xFFFFFFFF;
        }

        
        if (RENDER_MONO)
        {
            // Get constant color from the blue channel
            DWORD BlueChannel;
            BlueChannel = RGBA_GETBLUE(dwColor0);
            LOAD_INPUT_FIFO_TAG_DATA(ppdev, ConstantColor,
                            RGB_MAKE(BlueChannel, BlueChannel, BlueChannel));
        }
        else
        {
            if (pContext->FakeBlendNum & FAKE_ALPHABLEND_ONE_ONE)
            {
                dwColor0 &= 0xFFFFFF;
            }
            LOAD_INPUT_FIFO_TAG_DATA(ppdev, ConstantColor,
                            RGBA_MAKE(RGBA_GETBLUE(dwColor0), 
                                      RGBA_GETGREEN(dwColor0),
                                      RGBA_GETRED(dwColor0), 
                                      RGBA_GETALPHA(dwColor0)));
        }
    }

    LOAD_INPUT_FIFO_TAG_DATA(ppdev, DrawLine01, ulRenderCmd);
    LOAD_INPUT_FIFO_TAG_DATA(ppdev, RasterizerMode, 0);

    DBG_D3D((10,"Exiting P2DrawFVFLine"));
} // P2DrawFVFLine


