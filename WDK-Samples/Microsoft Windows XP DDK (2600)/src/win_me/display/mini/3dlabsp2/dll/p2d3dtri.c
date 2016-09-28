/******************************Module*Header**********************************\
*
*                           *************************************
*                           * Permedia 2: Direct 3D SAMPLE CODE *
*                           *************************************
*
* Module Name: p2d3dtri.c
*
*  Content:    Direct3D hw triangle rasterization code.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"


/*
 * Function:    P2DrawFVFSolidTri
 * Description: Hardware render a single triangle coming from three FVF vertices
 */

static VOID
P2DrawFVFSolidTri(PERMEDIA_D3DCONTEXT   *pContext,
                  LPD3DTLVERTEX         lpV0, 
                  LPD3DTLVERTEX         lpV1,
                  LPD3DTLVERTEX         lpV2, 
                  LPP2FVFOFFSETS        lpFVFOff)
{
    LPP2THUNKEDDATA ppdev       = pContext->pThisDisplay;
    DWORD           dwFlags     = pContext->Flags;
    ULONG           ulRenderCmd = pContext->RenderCommand;
    DWORD           dwColorOffs, dwSpecularOffs, dwTexOffs;
    D3DCOLOR        dwColor0, dwColor1, dwColor2;
    D3DCOLOR        dwSpec0, dwSpec1, dwSpec2;
    D3DVALUE        fS0, fS1, fS2, fT0, fT1, fT2, fQ0, fQ1, fQ2;
    D3DVALUE        fKs0, fKs1, fKs2;

    DBG_D3D((10,"Entering P2DrawFVFSolidTri"));

    // Set triangle rendering mode
    RENDER_TRAPEZOID(ulRenderCmd);

    WAIT_FOR_FREE_ENTRIES(ppdev, 120);

    // Get FVF structure offsets
    __SetFVFOffsets(&dwColorOffs,&dwSpecularOffs,&dwTexOffs,lpFVFOff);

    // Get vertex color value (FVF based)
    if (dwColorOffs)
    {
        dwColor0  = FVFCOLOR(lpV0, dwColorOffs)->color;
        dwColor1  = FVFCOLOR(lpV1, dwColorOffs)->color;
        dwColor2  = FVFCOLOR(lpV2, dwColorOffs)->color;
        if (FAKE_ALPHABLEND_MODULATE & pContext->FakeBlendNum)
        {
            dwColor0  |= 0xFF000000;
            dwColor1  |= 0xFF000000;
            dwColor2  |= 0xFF000000;
        }
    }
    else
    {
        // must set default in case no D3DFVF_DIFFUSE
        dwColor0  = 0xFFFFFFFF;
        dwColor1  = 0xFFFFFFFF;
        dwColor2  = 0xFFFFFFFF;
    }
    // Get vertex specular value (FVF based) if necessary
    if ((dwFlags & (CTXT_HAS_SPECULAR_ENABLED | CTXT_HAS_FOGGING_ENABLED))
        && (dwSpecularOffs!=0))
    {
        dwSpec0   = FVFSPEC(lpV0, dwSpecularOffs)->specular;
        dwSpec1   = FVFSPEC(lpV1, dwSpecularOffs)->specular;
        dwSpec2   = FVFSPEC(lpV2, dwSpecularOffs)->specular;
    }

    if ( (dwFlags & CTXT_HAS_TEXTURE_ENABLED) && (dwTexOffs != 0) )
    {
         // Get s,t texture coordinates (FVF based)
        fS0 = FVFTEX(lpV0,dwTexOffs)->tu; 
        fT0 = FVFTEX(lpV0,dwTexOffs)->tv;
        
        fS1 = FVFTEX(lpV1,dwTexOffs)->tu; 
        fT1 = FVFTEX(lpV1,dwTexOffs)->tv;
        
        fS2 = FVFTEX(lpV2,dwTexOffs)->tu; 
        fT2 = FVFTEX(lpV2,dwTexOffs)->tv;

        // The hw requires us to keep the texture coordinates centered around 0
        // and avoid exceeding the texel wrapping limit.
        RECENTER_TEX_COORDS(pContext->MaxTextureXf, 
                            pContext->MaxTextureXi, fS0, fS1, fS2);
        RECENTER_TEX_COORDS(pContext->MaxTextureYf, 
                            pContext->MaxTextureYi, fT0, fT1, fT2);

        // Wrap texture coordinates if necessary
        WRAP(fS,dwFlags & CTXT_HAS_WRAPU_ENABLED);
        WRAP(fT,dwFlags & CTXT_HAS_WRAPV_ENABLED);

        // Scale s,t coordinate values
        fS0 *= pContext->DeltaWidthScale;
        fS1 *= pContext->DeltaWidthScale;
        fS2 *= pContext->DeltaWidthScale;
        fT0 *= pContext->DeltaHeightScale;
        fT1 *= pContext->DeltaHeightScale;
        fT2 *= pContext->DeltaHeightScale;

        // Apply perspective corrections if necessary
        if (dwFlags & CTXT_HAS_PERSPECTIVE_ENABLED)
        {
            fQ0 = lpV0->rhw; 
            fQ1 = lpV1->rhw;
            fQ2 = lpV2->rhw;

            fS0 *= fQ0;
            fT0 *= fQ0;

            fS1 *= fQ1;
            fT1 *= fQ1;

            fS2 *= fQ2;
            fT2 *= fQ2;
        }
        else
        {
            fQ0 = fQ1 = fQ2 = 1.0;
        }

        // Send lines s,t,q,ks (conditionaly),x,y,z values
        if ((dwFlags & CTXT_HAS_SPECULAR_ENABLED) && (dwSpecularOffs!=0))
        {
            fKs0   = RGB256_TO_LUMA(RGB_GETRED(dwSpec0),
                                        RGB_GETGREEN(dwSpec0),
                                        RGB_GETBLUE(dwSpec0));
            if (dwFlags & CTXT_HAS_GOURAUD_ENABLED)
            {
                fKs1 = RGB256_TO_LUMA(RGB_GETRED(dwSpec1),
                                        RGB_GETGREEN(dwSpec1),
                                        RGB_GETBLUE(dwSpec1));
                fKs2 = RGB256_TO_LUMA(RGB_GETRED(dwSpec2),
                                        RGB_GETGREEN(dwSpec2),
                                        RGB_GETBLUE(dwSpec2));
            }   
            else 
            {
                fKs2 = fKs1 = fKs0; 
            }

            SEND_VERTEX_STQ_KS_XYZ(ppdev, __Permedia2TagV0FloatS, fS0, fT0, fQ0, fKs0,
                                                  lpV0->sx, lpV0->sy, lpV0->sz);
            SEND_VERTEX_STQ_KS_XYZ(ppdev, __Permedia2TagV1FloatS, fS1, fT1, fQ1, fKs1,
                                                  lpV1->sx, lpV1->sy, lpV1->sz);
            SEND_VERTEX_STQ_KS_XYZ(ppdev, __Permedia2TagV2FloatS, fS2, fT2, fQ2, fKs2,
                                                  lpV2->sx, lpV2->sy, lpV2->sz);
        }
        else
        {
            SEND_VERTEX_STQ_XYZ(ppdev, __Permedia2TagV0FloatS, fS0, fT0, fQ0,
                                                lpV0->sx, lpV0->sy, lpV0->sz);
            SEND_VERTEX_STQ_XYZ(ppdev, __Permedia2TagV1FloatS, fS1, fT1, fQ1,
                                                lpV1->sx, lpV1->sy, lpV1->sz);
            SEND_VERTEX_STQ_XYZ(ppdev, __Permedia2TagV2FloatS, fS2, fT2, fQ2,
                                                lpV2->sx, lpV2->sy, lpV2->sz);
        }

    }
    else     // not textured triangle
    {
        // If specular is enabled, change the colours
        if ((dwFlags & CTXT_HAS_SPECULAR_ENABLED) && (dwSpecularOffs!=0))
        {
            CLAMP8888(dwColor0, dwColor0, dwSpec0);
            CLAMP8888(dwColor1, dwColor1, dwSpec1);
            CLAMP8888(dwColor2, dwColor2, dwSpec2);
        }

        // Send triangles x,y,z values
        SEND_VERTEX_XYZ(ppdev, __Permedia2TagV0FloatS, lpV0->sx, lpV0->sy, lpV0->sz);
        SEND_VERTEX_XYZ(ppdev, __Permedia2TagV1FloatS, lpV1->sx, lpV1->sy, lpV1->sz);
        SEND_VERTEX_XYZ(ppdev, __Permedia2TagV2FloatS, lpV2->sx, lpV2->sy, lpV2->sz);
    }

    // If fog is set, send the appropriate values
    if ((dwFlags & CTXT_HAS_FOGGING_ENABLED) && (dwSpecularOffs!=0))
    {
        SEND_VERTEX_FOG(ppdev, __Permedia2TagV0FixedF, RGB_GET_GAMBIT_FOG(dwSpec0));
        SEND_VERTEX_FOG(ppdev, __Permedia2TagV1FixedF, RGB_GET_GAMBIT_FOG(dwSpec1));
        SEND_VERTEX_FOG(ppdev, __Permedia2TagV2FixedF, RGB_GET_GAMBIT_FOG(dwSpec2));
    }

    // Set alpha stippling if required by context
    if (dwFlags & CTXT_HAS_ALPHASTIPPLE_ENABLED)
    {
        SET_STIPPLED_ALPHA(ppdev, (RGBA_GETALPHA(lpV0->color) >> 4) );
    }

    // Send appropriate color depending on Gouraud , Mono, & Alpha
    if (dwFlags & CTXT_HAS_GOURAUD_ENABLED)
    {
        // Gouraud shading
        if (RENDER_MONO)
        {
            SEND_VERTEX_RGB_MONO(ppdev, __Permedia2TagV0FixedS, dwColor0);
            SEND_VERTEX_RGB_MONO(ppdev, __Permedia2TagV1FixedS, dwColor1);
            SEND_VERTEX_RGB_MONO(ppdev, __Permedia2TagV2FixedS, dwColor2);
        }
        else
        {
            if (dwFlags & CTXT_HAS_ALPHABLEND_ENABLED)
            {
                if (pContext->FakeBlendNum & FAKE_ALPHABLEND_ONE_ONE)
                {
                    dwColor0 &= 0xFFFFFF;     // supress color's alpha value
                    dwColor1 &= 0xFFFFFF;
                    dwColor2 &= 0xFFFFFF;
                }
                SEND_VERTEX_RGBA(ppdev, __Permedia2TagV0FixedS, dwColor0);
                SEND_VERTEX_RGBA(ppdev, __Permedia2TagV1FixedS, dwColor1);
                SEND_VERTEX_RGBA(ppdev, __Permedia2TagV2FixedS, dwColor2);
            }
            else
            {
                SEND_VERTEX_RGB(ppdev, __Permedia2TagV0FixedS, dwColor0);
                SEND_VERTEX_RGB(ppdev, __Permedia2TagV1FixedS, dwColor1);
                SEND_VERTEX_RGB(ppdev, __Permedia2TagV2FixedS, dwColor2);
            }
        }
    }
    else    // Flat shading
    {
        if (RENDER_MONO)
        {
            // Get constant color from the blue channel
            DWORD BlueChannel = RGBA_GETBLUE(dwColor0);
            LOAD_INPUT_FIFO_TAG_DATA(ppdev, 
                                     ConstantColor,
                                     RGB_MAKE(BlueChannel, 
                                              BlueChannel, 
                                              BlueChannel));
        }
        else
        {
            if (pContext->FakeBlendNum & FAKE_ALPHABLEND_ONE_ONE)
            {
                dwColor0 &= 0xFFFFFF;
            }

            LOAD_INPUT_FIFO_TAG_DATA(ppdev, 
                                     ConstantColor,
                                     RGBA_MAKE(RGBA_GETBLUE(dwColor0),
                                               RGBA_GETGREEN(dwColor0),
                                               RGBA_GETRED(dwColor0), 
                                               RGBA_GETALPHA(dwColor0)));
        }
    }

    LOAD_INPUT_FIFO_TAG_DATA(ppdev, DrawTriangle, ulRenderCmd); 

    DBG_D3D((10,"Exiting P2DrawFVFSolidTri"));

} // P2DrawFVFSolidTri

/*
 * Function:    P2DrawFVFPointTri
 * Description: Render a triangle with FVF vertexes when the point fillmode is active
 */

static void 
P2DrawFVFPointTri(PERMEDIA_D3DCONTEXT   *pContext, 
                  LPD3DTLVERTEX         lpV0, 
                  LPD3DTLVERTEX         lpV1,
                  LPD3DTLVERTEX         lpV2, 
                  LPP2FVFOFFSETS        lpFVFOff)
{
    D3DFVFDRAWPNTFUNCPTR       pPoint;

    DBG_D3D((10,"Entering P2DrawFVFPointTri"));

    pPoint = HWSetPointFunc(pContext, lpFVFOff);
    (*pPoint)(pContext, lpV0, lpFVFOff);
    (*pPoint)(pContext, lpV1, lpFVFOff);
    (*pPoint)(pContext, lpV2, lpFVFOff);

    DBG_D3D((10,"Exiting P2DrawFVFPointTri"));

} // P2DrawFVFPointTri

/*
 * Function:    P2DrawFVFWireTri
 * Description: Draws a Triangle Wire Frame
 */
static void 
P2DrawFVFWireTri(PERMEDIA_D3DCONTEXT    *pContext, 
                 LPD3DTLVERTEX          lpV0, 
                 LPD3DTLVERTEX          lpV1,
                 LPD3DTLVERTEX          lpV2, 
                 LPP2FVFOFFSETS         lpFVFOff)
{
    DBG_D3D((10,"Entering P2DrawFVFWireTri"));

    P2DrawFVFLine(pContext, lpV0, lpV1, lpV0, lpFVFOff);
    P2DrawFVFLine(pContext, lpV1, lpV2, lpV0, lpFVFOff);
    P2DrawFVFLine(pContext, lpV2, lpV0, lpV0, lpFVFOff);

    DBG_D3D((10,"Exiting P2DrawFVFWireTri"));
}

/*
 * Function:    HWSetTriangleFunc
 * Description: Select the appropiate triangle rendering function depending on the
 *              current fillmode set for the current context
 */

D3DFVFDRAWTRIFUNCPTR 
HWSetTriangleFunc(PERMEDIA_D3DCONTEXT *pContext)
{

    if ( pContext->FillMode == D3DFILL_SOLID )
        return (D3DFVFDRAWTRIFUNCPTR)P2DrawFVFSolidTri;
    else
    {
        if ( pContext->FillMode == D3DFILL_WIREFRAME )
            return (D3DFVFDRAWTRIFUNCPTR)P2DrawFVFWireTri;
        else
            // if it isn't solid nor line it must be a point filled triangle
            return (D3DFVFDRAWTRIFUNCPTR)P2DrawFVFPointTri;
    }
}

