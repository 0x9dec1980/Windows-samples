/******************************Module*Header**********************************\
*
*                           ***************************************
*                           * Permedia 2: Direct 3D   SAMPLE CODE *
*                           ***************************************
*
* Module Name: p2d3dpt.c
*
* Content:    Direct3D hw point rasterization code.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"


//
// Prototypes for local functions
//

static void 
P2DrawFVFPoint( PERMEDIA_D3DCONTEXT *pContext, 
                LPD3DTLVERTEX       lpV0, 
                LPP2FVFOFFSETS      lpFVFOff);


/*
 * Function:    P2DrawFVFPoint
 * Description: Hardare render a single point coming from a FVF vertex
 *              Primitive rendering at this stage is dependent upon the 
 *              current value/setting of texturing, perspective correction, 
 *              fogging, gouraud/flat shading, and specular highlights.
 */

static VOID
P2DrawFVFPoint( PERMEDIA_D3DCONTEXT  *pContext,
                LPD3DTLVERTEX        lpV0, 
                LPP2FVFOFFSETS       lpFVFOff)
{
    LPP2THUNKEDDATA ppdev       = pContext->pThisDisplay;
    DWORD           dwFlags     = pContext->Flags;
    ULONG           ulRenderCmd = pContext->RenderCommand;
    DWORD           dwColorOffs,dwSpecularOffs,dwTexOffs;
    D3DCOLOR        dwColor, dwSpecular;
    D3DVALUE        fKs, fS, fT, fQ;

    DBG_D3D((10,"Entering P2DrawFVFPoint"));

    // Set point rendering mode
    RENDER_POINT(ulRenderCmd);

    // Get FVF structure offsets
    __SetFVFOffsets(&dwColorOffs,&dwSpecularOffs,&dwTexOffs,lpFVFOff);

    WAIT_FOR_FREE_ENTRIES(ppdev, 40);
    LOAD_INPUT_FIFO_TAG_DATA(ppdev, RasterizerMode, BIAS_NEARLY_HALF);
    // Get vertex color value (FVF based)
    if (dwColorOffs)
    {
        dwColor = FVFCOLOR(lpV0, dwColorOffs)->color;
        if (FAKE_ALPHABLEND_MODULATE & pContext->FakeBlendNum)
        {
            dwColor  |= 0xFF000000;
        }
    }
    else
    {
        // must set default in case no D3DFVF_DIFFUSE
        dwColor = 0xFFFFFFFF;
    }

    // Get vertex specular value (FVF based) if necessary
    if ((dwFlags & (CTXT_HAS_SPECULAR_ENABLED | CTXT_HAS_FOGGING_ENABLED))
        && (dwSpecularOffs != 0))
    {
        dwSpecular = FVFSPEC(lpV0, dwSpecularOffs)->specular;
    }

    if ((dwFlags & CTXT_HAS_TEXTURE_ENABLED) && (dwTexOffs != 0))
    {
        // Get s,t texture coordinates (FVF based)
        fS = FVFTEX(lpV0,dwTexOffs)->tu;
        fT = FVFTEX(lpV0,dwTexOffs)->tv;

        // Scale s,t coordinate values
        fS *= pContext->DeltaWidthScale;
        fT *= pContext->DeltaHeightScale;

        // Apply perpspective corrections if necessary
        if (dwFlags & CTXT_HAS_PERSPECTIVE_ENABLED)
        {
            fQ = lpV0->rhw;
            fS *= fQ;
            fT *= fQ;
        }
        else
        {
            fQ = 1.0;
        }

        // Send points s,t,q,ks (conditionaly),x,y,z values
        if ((dwFlags & CTXT_HAS_SPECULAR_ENABLED) && (dwSpecularOffs != 0))
        {
            fKs = RGB256_TO_LUMA(RGB_GETRED(dwSpecular),
                                 RGB_GETGREEN(dwSpecular),
                                 RGB_GETBLUE(dwSpecular));

            SEND_VERTEX_STQ_KS_XYZ(ppdev, __Permedia2TagV0FloatS, fS, fT, fQ, 
                                            fKs, lpV0->sx, lpV0->sy, lpV0->sz);
        } 
        else 
        {
            SEND_VERTEX_STQ_XYZ(ppdev, __Permedia2TagV0FloatS, fS, fT, fQ, 
                                                lpV0->sx, lpV0->sy, lpV0->sz);
        }
    }
    else // not textured point
    {
        // If specular is enabled, change the colours
        if ((dwFlags & CTXT_HAS_SPECULAR_ENABLED) && (dwSpecularOffs != 0))
        {
            CLAMP8888(dwColor, dwColor, dwSpecular);
        }

        // Send lines x,y,z values
        SEND_VERTEX_XYZ(ppdev, __Permedia2TagV0FloatS, lpV0->sx, lpV0->sy, lpV0->sz);
    }

    // If fog is set, send the appropriate value
    if ((dwFlags & CTXT_HAS_FOGGING_ENABLED) && (dwSpecularOffs != 0))
    {
        SEND_VERTEX_FOG(ppdev, __Permedia2TagV0FixedF, RGB_GET_GAMBIT_FOG(dwSpecular));
    }

    // Send appropriate color depending on Gouraud , Mono, & Alpha
    if (dwFlags & CTXT_HAS_GOURAUD_ENABLED)
    {
        // Gouraud shading
        if (RENDER_MONO)
        {
            SEND_VERTEX_RGB_MONO_P2(ppdev, __Permedia2TagV0FixedS, dwColor);
        }
        else
        {
            if (dwFlags & CTXT_HAS_ALPHABLEND_ENABLED)
            {
                if (pContext->FakeBlendNum & FAKE_ALPHABLEND_ONE_ONE)
                {
                    dwColor &= 0xFFFFFF;  // supress color's alpha value
                }
            }
            SEND_VERTEX_RGBA_P2(ppdev, __Permedia2TagV0FixedS, dwColor);
        }
    }
    else        // Flat shading
    {
        if (RENDER_MONO)
        {
            // Get constant color from the blue channel
            DWORD BlueChannel = RGBA_GETBLUE(dwColor);
            LOAD_INPUT_FIFO_TAG_DATA(ppdev, ConstantColor,
                RGB_MAKE(BlueChannel, BlueChannel, BlueChannel));
        }
        else
        {
            if (pContext->FakeBlendNum & FAKE_ALPHABLEND_ONE_ONE)
            {
                dwColor &= 0xFFFFFF;
            }
            LOAD_INPUT_FIFO_TAG_DATA(ppdev, 
                                     ConstantColor,
                                     RGBA_MAKE(RGBA_GETBLUE(dwColor),
                                               RGBA_GETGREEN(dwColor), 
                                               RGBA_GETRED(dwColor), 
                                               RGBA_GETALPHA(dwColor)));
        }
    }

    LOAD_INPUT_FIFO_TAG_DATA(ppdev, DrawLine01, ulRenderCmd);
    LOAD_INPUT_FIFO_TAG_DATA(ppdev, RasterizerMode, 0);

    DBG_D3D((10,"Exiting P2DrawFVFPoint"));

} // P2DrawFVFPoint


/*
 * Function:    HWSetPointFunc
 * Description: Select the appropiate point rendering function depending on the
 *              current point sprite mode  set for the current context
 */

D3DFVFDRAWPNTFUNCPTR 
HWSetPointFunc(PERMEDIA_D3DCONTEXT  *pContext, 
               LPP2FVFOFFSETS       lpP2FVFOff)
{
        return (D3DFVFDRAWPNTFUNCPTR)P2DrawFVFPoint;
}

