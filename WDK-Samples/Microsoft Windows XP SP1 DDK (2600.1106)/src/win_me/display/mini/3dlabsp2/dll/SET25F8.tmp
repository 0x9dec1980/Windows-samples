/******************************Module*Header**********************************\
*
*                           *************************************
*                           * Permedia 2: Direct 3D SAMPLE CODE *
*                           *************************************
*
* Module Name: d3ddp2.c
*
*  Content:    Direct3D DX7 DrawPrimitives 2 callback function
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

//Proto main to get by a linker error.
int main(int argc, char ** arv)
{
    return 0;
}

//-----------------------------------------------------------------------------
//
// DX6 allows driver-level acceleration of the new vertex-buffer API. It 
// allows data and commands, indices and statechanges to be contained in 
// two separate DirectDraw surfaces. The DirectDraw surfaces can reside 
// in system, AGP, or video memory depending on the type of allocation
// requested by the user  The interface is designed to accomodate legacy
// ExecuteBuffer applications with no driver impact. This allows higher 
// performance on both legacy applications as well as the highest 
// possible performance through the vertex buffer API.
//
//-----------------------------------------------------------------------------

#define STARTVERTEXSIZE (sizeof(D3DHAL_DP2STARTVERTEX))

// Macros for updating properly our instruction pointer to our next instruction
// in the command buffer
#define NEXTINSTRUCTION(ptr, type, num, extrabytes)                            \
        NEXTINSTRUCTION_S(ptr, sizeof(type), num, extrabytes)

#define NEXTINSTRUCTION_S(ptr, typesize, num, extrabytes)                      \
    ptr = (LPD3DHAL_DP2COMMAND)((LPBYTE)ptr + sizeof(D3DHAL_DP2COMMAND) +      \
                                ((num) * (typesize)) + (extrabytes))

#define IS_OP_WITHIN_CMDBUFFER(lpCBStart, lpCBEnd, lpOpStart, sizeofOpType, numOps, extraBytes) \
    ((lpOpStart + (numOps * sizeofOpType) + (extraBytes - 1)) < lpCBEnd) &&                     \
     (lpCBStart <= lpOpStart))


#define IS_VALID_VERTEX_INDEX( pDP2Data, iIndex)                            \
    (((LONG)iIndex >= 0) && ((LONG)iIndex <(LONG)pDP2Data->dwVertexLength))

// Macros for accessing vertexes in the vertex buffer based on an index or on
// a previous accessed vertex
#define LP_FVF_VERTEX(lpBaseAddr, wIndex, P2FVFOffs)                           \
         (LPD3DTLVERTEX)((LPBYTE)(lpBaseAddr) + (wIndex) * (P2FVFOffs).dwStride)

#define LP_FVF_NXT_VTX(lpVtx, P2FVFOffs )                                      \
         (LPD3DTLVERTEX)((LPBYTE)(lpVtx) + (P2FVFOffs).dwStride)


// Forward declaration of utility functions
static DWORD 
__CheckFVFRequest(DWORD             dwFVF, 
                  LPP2FVFOFFSETS    lpP2FVFOff);

static VOID  
__Clear(PERMEDIA_D3DCONTEXT *pContext,
        DWORD               dwFlags,
        DWORD               dwFillColor,
        D3DVALUE            dvFillDepth,
        DWORD               dwFillStencil,
        LPD3DRECT           lpRects,
        DWORD               dwNumRects);

static VOID  
__TextureBlt(PERMEDIA_D3DCONTEXT    *pContext,
             D3DHAL_DP2TEXBLT       *lpdp2texblt);

static VOID  
__SetRenderTarget(PERMEDIA_D3DCONTEXT   *pContext,
                  DWORD                 hRenderTarget,
                  DWORD                 hZBuffer);


/**
 * Function:    D3DDrawPrimitives2
 * Description: The D3DDrawPrimitives2 callback is filled in by drivers  
 *              which directly support the rendering primitives using 
 *              the new DDI. If this entry is left as NULL, the API will 
 *              be emulated through DX5-level HAL interfaces.
 *
 * Parameters:
 *
 *      lpdp2d   This structure is used when D3DDrawPrimitives2 is called 
 *               to draw a set of primitives using a vertex buffer. The
 *               surface specified by the lpDDCommands in 
 *               D3DHAL_DRAWPRIMITIVES2DATA contains a sequence of 
 *               D3DHAL_DP2COMMAND structures. Each D3DHAL_DP2COMMAND 
 *               specifies either a primitive to draw, a state change to
 *               process, or a re-base command.
 *
 */

DWORD CALLBACK 
D3DDrawPrimitives2( LPD3DHAL_DRAWPRIMITIVES2DATA lpdp2d )
{
    LPD3DHAL_DP2COMMAND         lpIns, lpResumeIns;  
    LPD3DTLVERTEX               lpVertices, lpV0, lpV1, lpV2, lpV3;
    LPBYTE                      lpInsStart, lpPrim;
    PERMEDIA_D3DCONTEXT         *pContext;
    UINT                        i,j;
    WORD                        wCount, wIndex, wIndex1, wIndex2, wIndex3,
                                wFlags, wIndxBase;
    P2FVFOFFSETS                P2FVFOff;
    BOOL                        bFVFValidated = FALSE;
    D3DHAL_DP2TEXTURESTAGESTATE *lpRState;
    D3DFVFDRAWTRIFUNCPTR        pTriangle;
    D3DFVFDRAWPNTFUNCPTR        pPoint;
    DWORD                       dwEdgeFlags;
    LPP2THUNKEDDATA             ppdev;
    PP2REG_SOFTWARE_COPY        pSoftPermedia;
    LPBYTE                      lpCmdBufferStart, lpCmdBufferEnd;

    DBG_D3D((6,"Entering D3DDrawPrimitives2"));

    DBG_D3D((8,"  dwhContext        = %x",lpdp2d->dwhContext));
    DBG_D3D((8,"  dwFlags           = %x",lpdp2d->dwFlags));
    DBG_D3D((8,"  dwVertexType      = %d",lpdp2d->dwVertexType));
    DBG_D3D((8,"  dwCommandOffset   = %d",lpdp2d->dwCommandOffset));
    DBG_D3D((8,"  dwCommandLength   = %d",lpdp2d->dwCommandLength));
    DBG_D3D((8,"  dwVertexOffset    = %d",lpdp2d->dwVertexOffset));
    DBG_D3D((8,"  dwVertexLength    = %d",lpdp2d->dwVertexLength));

    // Retrieve permedia d3d context from context handle
    pContext = (PERMEDIA_D3DCONTEXT*)lpdp2d->dwhContext;


    // Check if we got a valid context
    if (pContext == NULL)
    {
        lpdp2d->dwErrorOffset   = 0;
        lpdp2d->ddrval          = D3DHAL_CONTEXT_BAD;
        DBG_D3D((0,"Context not valid in DrawPrimitives2; Exit")); 
        return DDHAL_DRIVER_HANDLED;
    }

    ppdev           = pContext->pThisDisplay;
    pSoftPermedia   = &pContext->SoftCopyP2Regs;
    
    // Get appropriate pointers to command buffer
    lpInsStart = (LPBYTE)(lpdp2d->lpDDCommands->lpGbl->fpVidMem);
    if (lpInsStart == NULL)
    {
        lpdp2d->dwErrorOffset   = 0;
        lpdp2d->ddrval          = DDERR_INVALIDPARAMS;
        DBG_D3D((0,"DX6 Command Buffer pointer is null; Exit"));
        return DDHAL_DRIVER_HANDLED;
    }

    // Check if vertex buffer resides in user memory or in a DDraw surface
    if (lpdp2d->dwFlags & D3DHALDP2_USERMEMVERTICES)
    {
        // Get appropriate pointer to vertices , memory is already secured
        lpVertices = (LPD3DTLVERTEX)((LPBYTE)lpdp2d->lpVertices + 
                                             lpdp2d->dwVertexOffset);
    } 
    else 
    {
        // Get appropriate pointer to vertices 
        lpVertices = (LPD3DTLVERTEX)((LPBYTE)lpdp2d->lpDDVertex->lpGbl->fpVidMem
                                                     + lpdp2d->dwVertexOffset);
    }

    lpCmdBufferStart    = lpInsStart + lpdp2d->dwCommandOffset;
    lpIns               = (LPD3DHAL_DP2COMMAND)lpCmdBufferStart;
    lpCmdBufferEnd      = lpCmdBufferStart + lpdp2d->dwCommandLength;
    lpdp2d->ddrval      = DD_OK;

    // Process commands while we haven't exhausted the command buffer
    while ((LPBYTE)lpIns < lpCmdBufferEnd)  
    {
        // Get pointer to first primitive structure past the D3DHAL_DP2COMMAND
        lpPrim = (LPBYTE)lpIns + sizeof(D3DHAL_DP2COMMAND);

        DBG_D3D((4,"D3DDrawPrimitive2: parsing instruction %d count = %d @ %x", 
                lpIns->bCommand, lpIns->wPrimitiveCount, lpIns));

        // If our next command involves some actual rendering, we have to make
        // sure that our rendering context is realized
        switch( lpIns->bCommand )
        {
            case D3DDP2OP_POINTS:
            case D3DDP2OP_LINELIST:
            case D3DDP2OP_INDEXEDLINELIST:
            case D3DDP2OP_INDEXEDLINELIST2:
            case D3DDP2OP_LINESTRIP:
            case D3DDP2OP_INDEXEDLINESTRIP:
            case D3DDP2OP_TRIANGLELIST:
            case D3DDP2OP_INDEXEDTRIANGLELIST:
            case D3DDP2OP_INDEXEDTRIANGLELIST2:
            case D3DDP2OP_TRIANGLESTRIP:
            case D3DDP2OP_INDEXEDTRIANGLESTRIP:
            case D3DDP2OP_TRIANGLEFAN:
            case D3DDP2OP_INDEXEDTRIANGLEFAN:
            case D3DDP2OP_LINELIST_IMM:
            case D3DDP2OP_TRIANGLEFAN_IMM:

            if (lpVertices == NULL)
            {
                lpdp2d->dwErrorOffset   = 0;
                lpdp2d->ddrval          = DDERR_CANTLOCKSURFACE;
                DBG_D3D((0,"DX6 Vertex Buffer pointer is null; Exit"));
                return DDHAL_DRIVER_HANDLED;
            }

            if (bFVFValidated == FALSE)
            {
                // Check if the FVF format being passed is valid. 
                if (__CheckFVFRequest(lpdp2d->dwVertexType, &P2FVFOff) != DD_OK)
                {
                    lpdp2d->dwErrorOffset   = 0; 
                    lpdp2d->ddrval          = D3DERR_INVALIDVERTEXFORMAT;                                       
                    DBG_D3D((0,"DrawPrimitives2 cannot handle FVF requested "));
                    return DDHAL_DRIVER_HANDLED;
                }
                bFVFValidated = TRUE;
            }
            // Update triangle rendering function
            pTriangle = HWSetTriangleFunc(pContext);
            pPoint    = HWSetPointFunc(pContext, &P2FVFOff);      

            // Handle State changes that may need to update the chip
            if (pContext->dwDirtyFlags & CONTEXT_DIRTY_RENDERTARGET)
            {
                SetupPermediaRenderTarget(pContext);
            }
            if (pContext->dwDirtyFlags)
            {
                // Handle the dirty states
                __HandleDirtyPermediaState(pContext, &P2FVFOff);
                pContext->dwReloadContext = RELOAD_CONTEXT;

                // DDK Requires that we allow system memory textures through up until
                // this point, at which we may fail the actual render.
                if (!pContext->bStateValid)
                {
                    DBG_D3D((0,"ERROR: Texture is non-valid memory, not rendering"));
                    goto ExitWithError;
                }
            }
            break;
        }

        // Execute the current command buffer command
        switch( lpIns->bCommand )
        {

        case D3DDP2OP_RENDERSTATE:
        {
            // Specifies a render state change that requires processing. 
            // The rendering state to change is specified by one or more 
            // D3DHAL_DP2RENDERSTATE structures following D3DHAL_DP2COMMAND.
        
            DBG_D3D((8,"D3DDP2OP_RENDERSTATE "
                    "state count = %d", lpIns->wStateCount));

            // Check we are in valid buffer memory
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2RENDERSTATE),
                                         lpIns->wStateCount,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            lpdp2d->ddrval = ProcessRenderStates(pContext,
                                                 lpIns->wStateCount,
                                                 (LPD3DSTATE) (lpPrim),
                                                 lpdp2d->lpdwRStates);

            if ( lpdp2d->ddrval != DD_OK )
            {
                DBG_D3D((2,"Error processing D3DDP2OP_RENDERSTATE"));
                goto ExitWithError;
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2RENDERSTATE, 
                            lpIns->wStateCount, 0);
        }
        break;

        case D3DDP2OP_TEXTURESTAGESTATE:
            // Specifies texture stage state changes, having wStateCount 
            // D3DNTHAL_DP2TEXTURESTAGESTATE structures follow the command
            // buffer. For each, the driver should update its internal 
            // texture state associated with the texture at dwStage to 
            // reflect the new value based on TSState.

            DBG_D3D((8,"D3DDP2OP_TEXTURESTAGESTATE"));

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2TEXTURESTAGESTATE),
                                         lpIns->wStateCount,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            lpRState = (D3DHAL_DP2TEXTURESTAGESTATE *)(lpPrim);
            for (i = 0; i < lpIns->wStateCount; i++)
            {
                if (0 == lpRState->wStage)
                {

                   pContext->dwDirtyFlags |= CONTEXT_DIRTY_MULTITEXTURE;

                   if ((lpRState->TSState >= D3DTSS_TEXTUREMAP) &&
                        (lpRState->TSState <= D3DTSS_TEXTURETRANSFORMFLAGS))
                   {
#if D3D_STATEBLOCKS
                        if (!pContext->bStateRecMode)
                        {
#endif //D3D_STATEBLOCKS
                            if (pContext->TssStates[lpRState->TSState] !=
                                                          lpRState->dwValue)
                            {
                                // Store value associated to this stage state
                                pContext->TssStates[lpRState->TSState] =
                                                             lpRState->dwValue;

                                // Perform any necessary preprocessing of it
                                PreProcessTextureStageState(pContext,
                                                            0,
                                                            lpRState->TSState,
                                                            lpRState->dwValue);

                                DBG_D3D((8,"TSS State Chg , Stage %d, "
                                           "State %d, Value %d",
                                        (LONG)lpRState->wStage, 
                                        (LONG)lpRState->TSState, 
                                        (LONG)lpRState->dwValue));
                                pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE; //AZN5
                            }
#if D3D_STATEBLOCKS
                        } 
                        else
                        {
                            if (pContext->pCurrSS != NULL)
                            {
                                DBG_D3D((6,"Recording RS %x = %x",
                                         lpRState->TSState, lpRState->dwValue));

                                // Recording the state in a stateblock
                                pContext->pCurrSS->u.uc.TssStates[lpRState->TSState] =
                                                                    lpRState->dwValue;
                                FLAG_SET(pContext->pCurrSS->u.uc.bStoredTSS,
                                         lpRState->TSState);
                            }
                        }
#endif //D3D_STATEBLOCKS
                   }
                   else
                   {
                        DBG_D3D((2,"Unhandled texture stage state %d value %d",
                            (LONG)lpRState->TSState, (LONG)lpRState->dwValue));
                   }
                }
                else
                {
                    DBG_D3D((0,"Texture Stage other than 0 received,"
                               " not supported in hw"));
                }
                lpRState ++;
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TEXTURESTAGESTATE, 
                            lpIns->wStateCount, 0); 
            break;

        case D3DDP2OP_VIEWPORTINFO:
            // Specifies the clipping rectangle used for guard-band 
            // clipping by guard-band aware drivers. The clipping 
            // rectangle (i.e. the viewing rectangle) is specified 
            // by the D3DHAL_DP2 VIEWPORTINFO structures following 
            // D3DHAL_DP2COMMAND

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2VIEWPORTINFO),
                                         lpIns->wStateCount,
                                         0))
            {
                goto ExitWithUnParsedError;
            }


            // We don't implement guard band clipping in this driver so
            // we just skip any of this data that might be sent to us
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2VIEWPORTINFO,
                            lpIns->wStateCount, 0); 
            break;

        case D3DDP2OP_WINFO:
            // Specifies the w-range for W buffering. It is specified
            // by one or more D3DHAL_DP2WINFO structures following
            // D3DHAL_DP2COMMAND.

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2WINFO),
                                         lpIns->wStateCount,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            // We dont implement a w-buffer in this driver so we just 
            // skip any of this data that might be sent to us 
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2WINFO,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_POINTS:

            DBG_D3D((8,"D3DDP2OP_POINTS"));
            // Point primitives in vertex buffers are defined by the 
            // D3DHAL_DP2POINTS structure. The driver should render
            // wCount points starting at the initial vertex specified 
            // by wFirst. Then for each D3DHAL_DP2POINTS, the points
            // rendered will be (wFirst),(wFirst+1),...,
            // (wFirst+(wCount-1)). The number of D3DHAL_DP2POINTS
            // structures to process is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND.

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2POINTS),
                                         lpIns->wPrimitiveCount,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                wIndex = ((D3DHAL_DP2POINTS*)lpPrim)->wVStart;
                wCount = ((D3DHAL_DP2POINTS*)lpPrim)->wCount;

                lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);

                // Check first & last vertex
                if (!(IS_VALID_VERTEX_INDEX(lpdp2d, wIndex) &&
                      IS_VALID_VERTEX_INDEX(lpdp2d, ((LONG)wIndex + wCount - 1))))
                {
                    goto ExitWithVertexBufferError;
                }

                for (j = 0; j < wCount; j++)
                {
                    (*pPoint)(pContext, lpV0, &P2FVFOff);
                    lpV0 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
                }

                lpPrim += sizeof(D3DHAL_DP2POINTS);
            }
            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2POINTS, 
                                   lpIns->wPrimitiveCount, 0);
            break;

        case D3DDP2OP_LINELIST:

            DBG_D3D((8,"D3DDP2OP_LINELIST"));

            // Non-indexed vertex-buffer line lists are defined by the 
            // D3DHAL_DP2LINELIST structure. Given an initial vertex, 
            // the driver will render a sequence of independent lines, 
            // processing two new vertices with each line. The number 
            // of lines to render is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND. The sequence of lines 
            // rendered will be 
            // (wVStart, wVStart+1),(wVStart+2, wVStart+3),...,
            // (wVStart+(wPrimitiveCount-1)*2), wVStart+wPrimitiveCount*2 - 1).

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2LINELIST),
                                         1,
                                         0))
            {
                goto ExitWithUnParsedError;
            }
            SWITCH_TO_CONTEXT(pContext, ppdev);

            wIndex = ((D3DHAL_DP2LINELIST*)lpPrim)->wVStart;

            lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);

            // Check first & last vertex
            if (!(IS_VALID_VERTEX_INDEX(lpdp2d, wIndex) &&
                  IS_VALID_VERTEX_INDEX(lpdp2d, ((LONG)wIndex + (2*lpIns->wPrimitiveCount) - 1))))
            {
                goto ExitWithVertexBufferError;
            }

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                P2DrawFVFLine(pContext, lpV0, lpV1, lpV0, &P2FVFOff);

                lpV0 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
                lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
            }
            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2LINELIST, 1, 0);
            break;

        case D3DDP2OP_INDEXEDLINELIST:

            DBG_D3D((8,"D3DDP2OP_INDEXEDLINELIST"));

            // The D3DHAL_DP2INDEXEDLINELIST structure specifies 
            // unconnected lines to render using vertex indices.
            // The line endpoints for each line are specified by wV1 
            // and wV2. The number of lines to render using this 
            // structure is specified by the wPrimitiveCount field of
            // D3DHAL_DP2COMMAND.  The sequence of lines 
            // rendered will be (wV[0], wV[1]), (wV[2], wV[3]),...
            // (wVStart[(wPrimitiveCount-1)*2], wVStart[wPrimitiveCount*2-1]).

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2INDEXEDLINELIST),
                                         lpIns->wPrimitiveCount,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            { 
                wIndex1 = ((D3DHAL_DP2INDEXEDLINELIST*)lpPrim)->wV1;
                wIndex2 = ((D3DHAL_DP2INDEXEDLINELIST*)lpPrim)->wV2;

                lpV1 = LP_FVF_VERTEX(lpVertices, wIndex1, P2FVFOff);
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2, P2FVFOff);

                // Must check each new vertex
                if (!(IS_VALID_VERTEX_INDEX(lpdp2d, wIndex1) &&
                      IS_VALID_VERTEX_INDEX(lpdp2d, wIndex2) ))
                {
                    goto ExitWithVertexBufferError;
                }
                P2DrawFVFLine(pContext, lpV1, lpV2, lpV1, &P2FVFOff);

                lpPrim += sizeof(D3DHAL_DP2INDEXEDLINELIST);
            }
            EXIT_FROM_CONTEXT(pContext, ppdev);

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDLINELIST, 
                                   lpIns->wPrimitiveCount, 0);
            break;

        case D3DDP2OP_INDEXEDLINELIST2:

            DBG_D3D((8,"D3DDP2OP_INDEXEDLINELIST2"));

            // The D3DHAL_DP2INDEXEDLINELIST structure specifies 
            // unconnected lines to render using vertex indices.
            // The line endpoints for each line are specified by wV1 
            // and wV2. The number of lines to render using this 
            // structure is specified by the wPrimitiveCount field of
            // D3DHAL_DP2COMMAND.  The sequence of lines 
            // rendered will be (wV[0], wV[1]), (wV[2], wV[3]),
            // (wVStart[(wPrimitiveCount-1)*2], wVStart[wPrimitiveCount*2-1]).
            // The indexes are relative to a base index value that 
            // immediately follows the command

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2INDEXEDLINELIST),
                                         lpIns->wPrimitiveCount,
                                         STARTVERTEXSIZE))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            // Access base index
            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                wIndex1 = ((D3DHAL_DP2INDEXEDLINELIST*)lpPrim)->wV1;
                wIndex2 = ((D3DHAL_DP2INDEXEDLINELIST*)lpPrim)->wV2;

                lpV1 = LP_FVF_VERTEX(lpVertices, (wIndex1+wIndxBase), P2FVFOff);
                lpV2 = LP_FVF_VERTEX(lpVertices, (wIndex2+wIndxBase), P2FVFOff);

                // Must check each new vertex
                if (!(IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex1 + wIndxBase)) &&
                      IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex2 + wIndxBase)) ))
                {
                    goto ExitWithVertexBufferError;
                }

                P2DrawFVFLine(pContext, lpV1, lpV2, lpV1, &P2FVFOff);

                lpPrim += sizeof(D3DHAL_DP2INDEXEDLINELIST);
            }

            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDLINELIST, 
                                   lpIns->wPrimitiveCount, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_LINESTRIP:

            DBG_D3D((8,"D3DDP2OP_LINESTRIP"));

            // Non-index line strips rendered with vertex buffers are
            // specified using D3DHAL_DP2LINESTRIP. The first vertex 
            // in the line strip is specified by wVStart. The 
            // number of lines to process is specified by the 
            // wPrimitiveCount field of D3DHAL_DP2COMMAND. The sequence
            // of lines rendered will be (wVStart, wVStart+1),
            // (wVStart+1, wVStart+2),(wVStart+2, wVStart+3),...,
            // (wVStart+wPrimitiveCount, wVStart+wPrimitiveCount+1).

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2LINESTRIP),
                                         1,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            wIndex = ((D3DHAL_DP2LINESTRIP*)lpPrim)->wVStart;

            lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);

            // Check first & last vertex
            if (!(IS_VALID_VERTEX_INDEX(lpdp2d, wIndex) &&
                  IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex + lpIns->wPrimitiveCount)) ))
            {
                goto ExitWithVertexBufferError;
            }

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                P2DrawFVFLine(pContext, lpV0, lpV1, lpV0, &P2FVFOff);

                lpV0 = lpV1;
                lpV1 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
            }

            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2LINESTRIP, 1, 0);
            break;

        case D3DDP2OP_INDEXEDLINESTRIP:

            DBG_D3D((8,"D3DDP2OP_INDEXEDLINESTRIP"));

            // Indexed line strips rendered with vertex buffers are 
            // specified using D3DHAL_DP2INDEXEDLINESTRIP. The number
            // of lines to process is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND. The sequence of lines 
            // rendered will be (wV[0], wV[1]), (wV[1], wV[2]),
            // (wV[2], wV[3]), ...
            // (wVStart[wPrimitiveCount-1], wVStart[wPrimitiveCount]). 
            // Although the D3DHAL_DP2INDEXEDLINESTRIP structure only
            // has enough space allocated for a single line, the wV 
            // array of indices should be treated as a variable-sized 
            // array with wPrimitiveCount+1 elements.
            // The indexes are relative to a base index value that 
            // immediately follows the command

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(WORD),
                                         (lpIns->wPrimitiveCount + 1),
                                         STARTVERTEXSIZE))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            // guard defensively against pathological commands
            if ( lpIns->wPrimitiveCount > 0 )
            {
                wIndex1 = ((D3DHAL_DP2INDEXEDLINESTRIP*)lpPrim)->wV[0];
                wIndex2 = ((D3DHAL_DP2INDEXEDLINESTRIP*)lpPrim)->wV[1];
                lpV1 = 
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex1+wIndxBase, P2FVFOff);

                //We need to check each vertex separately
                if (! IS_VALID_VERTEX_INDEX(lpdp2d, wIndex1 + wIndxBase))
                {
                    goto ExitWithVertexBufferError;
                }
            }

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            { 
                lpV1 = lpV2;
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2 + wIndxBase, P2FVFOff);

                if (! IS_VALID_VERTEX_INDEX(lpdp2d, wIndex2 + wIndxBase))
                {
                    goto ExitWithVertexBufferError;
                }
                P2DrawFVFLine(pContext, lpV1, lpV2, lpV1, &P2FVFOff);

                if ( i % 2 )
                {
                    wIndex2 = ((D3DHAL_DP2INDEXEDLINESTRIP*)lpPrim)->wV[1];
                } 
                else if ( (i+1) < lpIns->wPrimitiveCount )
                {
                    // advance to the next element only if we're not done yet
                    lpPrim += sizeof(D3DHAL_DP2INDEXEDLINESTRIP);
                    wIndex2 = ((D3DHAL_DP2INDEXEDLINESTRIP*)lpPrim)->wV[0];
                }
            }

            // Point to next D3DHAL_DP2COMMAND in the command buffer
            // Advance only as many vertex indices there are, with no padding!
            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, WORD, 
                            lpIns->wPrimitiveCount + 1, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_TRIANGLELIST:

            DBG_D3D((8,"D3DDP2OP_TRIANGLELIST"));

            // Non-indexed vertex buffer triangle lists are defined by 
            // the D3DHAL_DP2TRIANGLELIST structure. Given an initial
            // vertex, the driver will render independent triangles, 
            // processing three new vertices with each triangle. The
            // number of triangles to render is specified by the 
            // wPrimitveCount field of D3DHAL_DP2COMMAND. The sequence
            // of vertices processed will be  (wVStart, wVStart+1, 
            // vVStart+2), (wVStart+3, wVStart+4, vVStart+5),...,
            // (wVStart+(wPrimitiveCount-1)*3), wVStart+wPrimitiveCount*3-2, 
            // vStart+wPrimitiveCount*3-1).

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2TRIANGLELIST),
                                         1,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            wIndex = ((D3DHAL_DP2TRIANGLELIST*)lpPrim)->wVStart;

            lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
            lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);

            // Check first & last vertex
            if (!(IS_VALID_VERTEX_INDEX(lpdp2d, wIndex) &&
                  IS_VALID_VERTEX_INDEX(lpdp2d, ((LONG)wIndex + 3*lpIns->wPrimitiveCount - 1)) ))
            {
                goto ExitWithVertexBufferError;
            }
            
            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV0, lpV1, lpV2, &P2FVFOff);

                lpV0 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);
                lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
                lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
            }
            

            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TRIANGLELIST, 1, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLELIST:

            DBG_D3D((8,"D3DDP2OP_INDEXEDTRIANGLELIST"));

            // The D3DHAL_DP2INDEXEDTRIANGLELIST structure specifies 
            // unconnected triangles to render with a vertex buffer.
            // The vertex indices are specified by wV1, wV2 and wV3. 
            // The wFlags field allows specifying edge flags identical 
            // to those specified by D3DOP_TRIANGLE. The number of 
            // triangles to render (that is, number of 
            // D3DHAL_DP2INDEXEDTRIANGLELIST structures to process) 
            // is specified by the wPrimitiveCount field of 
            // D3DHAL_DP2COMMAND.

            // This is the only indexed primitive where we don't get 
            // an offset into the vertex buffer in order to maintain
            // DX3 compatibility. A new primitive 
            // (D3DDP2OP_INDEXEDTRIANGLELIST2) has been added to handle
            // the corresponding DX6 primitive.

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST),
                                         lpIns->wPrimitiveCount,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            { 
                wIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wV1;
                wIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wV2;
                wIndex3 = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wV3;
                wFlags  = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wFlags;


                lpV1 = LP_FVF_VERTEX(lpVertices, wIndex1, P2FVFOff);
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2, P2FVFOff);
                lpV3 = LP_FVF_VERTEX(lpVertices, wIndex3, P2FVFOff);

                // Must check each new vertex
                if (!(IS_VALID_VERTEX_INDEX(lpdp2d, wIndex1) &&
                      IS_VALID_VERTEX_INDEX(lpdp2d, wIndex2) &&
                      IS_VALID_VERTEX_INDEX(lpdp2d, wIndex3) ))
                {
                    goto ExitWithVertexBufferError;
                }
                if (!CULL_TRI(pContext,lpV1,lpV2,lpV3))
                {

                    if (pContext->FillMode == D3DFILL_POINT)
                    {
                        (*pPoint)( pContext, lpV1, &P2FVFOff);
                        (*pPoint)( pContext, lpV2, &P2FVFOff);
                        (*pPoint)( pContext, lpV3, &P2FVFOff);
                    } 
                    else if (pContext->FillMode == D3DFILL_WIREFRAME)
                    {
                        if ( wFlags & D3DTRIFLAG_EDGEENABLE1 )
                            P2DrawFVFLine( pContext,
                                              lpV1, lpV2, lpV1, &P2FVFOff);
                        if ( wFlags & D3DTRIFLAG_EDGEENABLE2 )
                            P2DrawFVFLine( pContext,
                                              lpV2, lpV3, lpV1, &P2FVFOff);
                        if ( wFlags & D3DTRIFLAG_EDGEENABLE3 )
                            P2DrawFVFLine( pContext,
                                              lpV3, lpV1, lpV1, &P2FVFOff);
                    }
                    else
                        (*pTriangle)(pContext, lpV1, lpV2, lpV3, &P2FVFOff);
                }

                lpPrim += sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST);
            }

            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDTRIANGLELIST, 
                                   lpIns->wPrimitiveCount, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLELIST2:

            DBG_D3D((8,"D3DDP2OP_INDEXEDTRIANGLELIST2 "));

            // The D3DHAL_DP2INDEXEDTRIANGLELIST2 structure specifies 
            // unconnected triangles to render with a vertex buffer.
            // The vertex indices are specified by wV1, wV2 and wV3. 
            // The wFlags field allows specifying edge flags identical 
            // to those specified by D3DOP_TRIANGLE. The number of 
            // triangles to render (that is, number of 
            // D3DHAL_DP2INDEXEDTRIANGLELIST structures to process) 
            // is specified by the wPrimitiveCount field of 
            // D3DHAL_DP2COMMAND.
            // The indexes are relative to a base index value that 
            // immediately follows the command

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST2),
                                         lpIns->wPrimitiveCount,
                                         STARTVERTEXSIZE))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            // Access base index here
            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            { 
                wIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLELIST2*)lpPrim)->wV1;
                wIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLELIST2*)lpPrim)->wV2;
                wIndex3 = ((D3DHAL_DP2INDEXEDTRIANGLELIST2*)lpPrim)->wV3;

                lpV1 = LP_FVF_VERTEX(lpVertices, wIndex1+wIndxBase, P2FVFOff);
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2+wIndxBase, P2FVFOff);
                lpV3 = LP_FVF_VERTEX(lpVertices, wIndex3+wIndxBase, P2FVFOff);

                // Must check each new vertex
                if (! (IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex1 + wIndxBase)) &&
                       IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex2 + wIndxBase)) &&
                       IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex3 + wIndxBase)) ))
                {
                    goto ExitWithVertexBufferError;
                }

                if (!CULL_TRI(pContext,lpV1,lpV2,lpV3)) 
                {
                    if (pContext->FillMode == D3DFILL_POINT)
                    {
                        (*pPoint)( pContext, lpV1, &P2FVFOff);
                        (*pPoint)( pContext, lpV2, &P2FVFOff);
                        (*pPoint)( pContext, lpV3, &P2FVFOff);
                    }
                    else if (pContext->FillMode == D3DFILL_WIREFRAME)
                    {
                            P2DrawFVFLine( pContext,
                                              lpV1, lpV2, lpV1, &P2FVFOff);
                            P2DrawFVFLine( pContext,
                                              lpV2, lpV3, lpV1, &P2FVFOff);
                            P2DrawFVFLine( pContext,
                                              lpV3, lpV1, lpV1, &P2FVFOff);
                    } 
                    else
                        (*pTriangle)(pContext, lpV1, lpV2, lpV3, &P2FVFOff);
                }

                lpPrim += sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST2);
            }

            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDTRIANGLELIST2, 
                                   lpIns->wPrimitiveCount, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_TRIANGLESTRIP:

            DBG_D3D((8,"D3DDP2OP_TRIANGLESTRIP"));

            // Non-index triangle strips rendered with vertex buffers 
            // are specified using D3DHAL_DP2TRIANGLESTRIP. The first 
            // vertex in the triangle strip is specified by wVStart. 
            // The number of triangles to process is specified by the 
            // wPrimitiveCount field of D3DHAL_DP2COMMAND. The sequence
            // of triangles rendered for the odd-triangles case will 
            // be (wVStart, wVStart+1, vVStart+2), (wVStart+1, 
            // wVStart+3, vVStart+2),.(wVStart+2, wVStart+3, 
            // vVStart+4),.., (wVStart+wPrimitiveCount-1), 
            // wVStart+wPrimitiveCount, vStart+wPrimitiveCount+1). For an
            // even number of , the last triangle will be .,
            // (wVStart+wPrimitiveCount-1, vStart+wPrimitiveCount+1,
            // wVStart+wPrimitiveCount).

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2TRIANGLESTRIP),
                                         1,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            // guard defensively against pathological commands
            if ( lpIns->wPrimitiveCount > 0 )
            {
                wIndex = ((D3DHAL_DP2TRIANGLESTRIP*)lpPrim)->wVStart;
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
                lpV1 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);

                // Check first and last vertex
                if (!(IS_VALID_VERTEX_INDEX(lpdp2d, wIndex)  &&
                      IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex + lpIns->wPrimitiveCount + 1)) ))
                {
                    goto ExitWithVertexBufferError;
                }
            }

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            { 
                if ( i % 2 )
                {
                    lpV0 = lpV1;
                    lpV1 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);
                }
                else
                {
                    lpV0 = lpV2;
                    lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
                }

                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV0, lpV1, lpV2, &P2FVFOff);
            }
            // Point to next D3DHAL_DP2COMMAND in the command buffer
            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TRIANGLESTRIP, 1, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLESTRIP:

            DBG_D3D((8,"D3DDP2OP_INDEXEDTRIANGLESTRIP"));

            // Indexed triangle strips rendered with vertex buffers are 
            // specified using D3DHAL_DP2INDEXEDTRIANGLESTRIP. The number
            // of triangles to process is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND. The sequence of triangles 
            // rendered for the odd-triangles case will be 
            // (wV[0],wV[1],wV[2]),(wV[1],wV[3],wV[2]),
            // (wV[2],wV[3],wV[4]),...,(wV[wPrimitiveCount-1],
            // wV[wPrimitiveCount],wV[wPrimitiveCount+1]). For an even
            // number of triangles, the last triangle will be
            // (wV[wPrimitiveCount-1],wV[wPrimitiveCount+1],
            // wV[wPrimitiveCount]).Although the 
            // D3DHAL_DP2INDEXEDTRIANGLESTRIP structure only has 
            // enough space allocated for a single line, the wV 
            // array of indices should be treated as a variable-sized 
            // array with wPrimitiveCount+2 elements.
            // The indexes are relative to a base index value that 
            // immediately follows the command


            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(WORD),
                                         (lpIns->wPrimitiveCount + 2),
                                         STARTVERTEXSIZE))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            // Access base index
            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            // guard defensively against pathological commands
            if ( lpIns->wPrimitiveCount > 0 )
            {
                wIndex  = ((D3DHAL_DP2INDEXEDTRIANGLESTRIP*)lpPrim)->wV[0];
                wIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLESTRIP*)lpPrim)->wV[1];

                // We need to check each vertex
                if (! (IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex + wIndxBase)) &&
                       IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex1 + wIndxBase)) ))
                {
                    goto ExitWithVertexBufferError;
                }
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex + wIndxBase, P2FVFOff);
                lpV1 = LP_FVF_VERTEX(lpVertices, wIndex1 + wIndxBase, P2FVFOff);
            }

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            { 
                wIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLESTRIP*)lpPrim)->wV[2];

                // We need to check each new vertex
                if (! IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex2+wIndxBase)) )
                {
                    goto ExitWithVertexBufferError;
                }

                if ( i % 2 )
                {
                    lpV0 = lpV1;
                    lpV1 = LP_FVF_VERTEX(lpVertices, wIndex2+wIndxBase, P2FVFOff);
                }
                else
                {
                    lpV0 = lpV2;
                    lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2+wIndxBase, P2FVFOff);
                }

                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV0, lpV1, lpV2, &P2FVFOff);

                // We will advance our pointer only one WORD in order 
                // to fetch the next index
                lpPrim += sizeof(WORD);
            }
 
            // Point to next D3DHAL_DP2COMMAND in the command buffer
            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, WORD , 
                            lpIns->wPrimitiveCount + 2, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_TRIANGLEFAN:

            DBG_D3D((8,"D3DDP2OP_TRIANGLEFAN"));

            // The D3DHAL_DP2TRIANGLEFAN structure is used to draw 
            // non-indexed triangle fans. The sequence of triangles
            // rendered will be (wVstart+1, wVStart+2, wVStart),
            // (wVStart+2,wVStart+3,wVStart), (wVStart+3,wVStart+4
            // wVStart),...,(wVStart+wPrimitiveCount,
            // wVStart+wPrimitiveCount+1,wVStart).

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2TRIANGLEFAN),
                                         1,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            wIndex = ((D3DHAL_DP2TRIANGLEFAN*)lpPrim)->wVStart;

            lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
            lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);

            // Check first & last vertex
            if (!(IS_VALID_VERTEX_INDEX(lpdp2d, wIndex)   &&
                  IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex + lpIns->wPrimitiveCount + 1)) ))
            {
                goto ExitWithVertexBufferError;
            }

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            {
                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV1, lpV2, lpV0, &P2FVFOff);

                lpV1 = lpV2;
                lpV2 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);
            }

            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TRIANGLEFAN, 1, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLEFAN:

            DBG_D3D((8,"D3DDP2OP_INDEXEDTRIANGLEFAN"));

            // The D3DHAL_DP2INDEXEDTRIANGLEFAN structure is used to 
            // draw indexed triangle fans. The sequence of triangles
            // rendered will be (wV[1], wV[2],wV[0]), (wV[2], wV[3],
            // wV[0]), (wV[3], wV[4], wV[0]),...,
            // (wV[wPrimitiveCount], wV[wPrimitiveCount+1],wV[0]).
            // The indexes are relative to a base index value that 
            // immediately follows the command

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(WORD),
                                         (lpIns->wPrimitiveCount + 2),
                                         STARTVERTEXSIZE))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            // guard defensively against pathological commands
            if ( lpIns->wPrimitiveCount > 0 )
            {
                wIndex  = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[0];
                wIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[1];
                lpV0 = LP_FVF_VERTEX(lpVertices, wIndex + wIndxBase, P2FVFOff);
                lpV1 = 
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex1 + wIndxBase, P2FVFOff);

                // We need to check each vertex
                if (!(IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex + wIndxBase)) &&
                      IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex1 + wIndxBase)) ))
                {
                    goto ExitWithVertexBufferError;
                }
            }

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            { 
                wIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[2];
                lpV1 = lpV2;
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2 + wIndxBase, P2FVFOff);

                // We need to check each vertex
                if (!IS_VALID_VERTEX_INDEX(lpdp2d, (wIndex2 + wIndxBase)) )
                {
                    goto ExitWithVertexBufferError;
                }
                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV1, lpV2, lpV0, &P2FVFOff);

                // We will advance our pointer only one WORD in order 
                // to fetch the next index
                lpPrim += sizeof(WORD);
            }

            // Point to next D3DHAL_DP2COMMAND in the command buffer
            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION(lpIns, WORD , 
                            lpIns->wPrimitiveCount + 2, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_LINELIST_IMM:

            DBG_D3D((8,"D3DDP2OP_LINELIST_IMM"));

            // Draw a set of lines specified by pairs of vertices 
            // that immediately follow this instruction in the
            // command stream. The wPrimitiveCount member of the
            // D3DHAL_DP2COMMAND structure specifies the number
            // of lines that follow. The type and size of the
            // vertices are determined by the dwVertexType member
            // of the D3DHAL_DRAWPRIMITIVES2DATA structure.

            // Primitives in an IMM instruction are stored in the
            // command buffer and are DWORD aligned
            lpPrim = (LPBYTE)((ULONG_PTR)(lpPrim + 3 ) & ~3 );

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         P2FVFOff.dwStride,
                                         (lpIns->wPrimitiveCount + 1),
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            // Get vertex pointers
            lpV0 = (LPD3DTLVERTEX)lpPrim;
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            {
                P2DrawFVFLine(pContext, lpV0, lpV1, lpV0, &P2FVFOff);

                lpV0 = lpV1;
                lpV1 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
            }

            // Realign next command since vertices are dword aligned
            // and store # of primitives before affecting the pointer
            wCount = lpIns->wPrimitiveCount;
            lpIns  = (LPD3DHAL_DP2COMMAND)(( ((ULONG_PTR)lpIns) + 3 ) & ~ 3);

            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION_S(lpIns, P2FVFOff.dwStride, wCount + 1, 0);

            break;

        case D3DDP2OP_TRIANGLEFAN_IMM:

            DBG_D3D((8,"D3DDP2OP_TRIANGLEFAN_IMM"));

            // Draw a triangle fan specified by pairs of vertices 
            // that immediately follow this instruction in the
            // command stream. The wPrimitiveCount member of the
            // D3DHAL_DP2COMMAND structure specifies the number
            // of triangles that follow. The type and size of the
            // vertices are determined by the dwVertexType member
            // of the D3DHAL_DRAWPRIMITIVES2DATA structure.

            // Verify the command buffer validity for the first structure
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(BYTE),
                                         0,
                                         sizeof(D3DHAL_DP2TRIANGLEFAN_IMM)))
            {
                goto ExitWithUnParsedError;
            }

            // Get Edge flags (we still have to process them)
            dwEdgeFlags = ((D3DHAL_DP2TRIANGLEFAN_IMM *)lpPrim)->dwEdgeFlags;
            lpPrim = (LPBYTE)lpPrim + sizeof(D3DHAL_DP2TRIANGLEFAN_IMM); 

            // Vertices in an IMM instruction are stored in the
            // command buffer and are DWORD aligned
            lpPrim = (LPBYTE)((ULONG_PTR)(lpPrim + 3 ) & ~3 );

            // Verify the rest of the command buffer
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         P2FVFOff.dwStride,
                                         (lpIns->wPrimitiveCount + 2),
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            SWITCH_TO_CONTEXT(pContext, ppdev);
            // Get vertex pointers
            lpV0 = (LPD3DTLVERTEX)lpPrim;
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
            lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);

            for (i = 0 ; i < lpIns->wPrimitiveCount ; i++)
            {

                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                {
                    if (pContext->FillMode == D3DFILL_POINT)
                    {
                        if (0 == i)
                        {
                            (*pPoint)( pContext, lpV0, &P2FVFOff);
                            (*pPoint)( pContext, lpV1, &P2FVFOff);
                        }
                        (*pPoint)( pContext, lpV2, &P2FVFOff);
                    } 
                    else if (pContext->FillMode == D3DFILL_WIREFRAME)
                    {
                        // dwEdgeFlags is a bit sequence representing the edge
                        // flag for each one of the outer edges of the 
                        // triangle fan
                        if (0 == i)
                        {
                            if (dwEdgeFlags & 0x0001)
                                P2DrawFVFLine( pContext, lpV0, lpV1, lpV0,
                                                  &P2FVFOff);

                            dwEdgeFlags >>= 1;
                        }

                        if (dwEdgeFlags & 0x0001)
                            P2DrawFVFLine( pContext, lpV1, lpV2, lpV0,
                                              &P2FVFOff);

                        dwEdgeFlags >>= 1;

                        if (i == (UINT)lpIns->wPrimitiveCount - 1)
                        {
                            // last triangle fan edge
                            if (dwEdgeFlags & 0x0001)
                                P2DrawFVFLine( pContext, lpV2, lpV0, lpV0,
                                                  &P2FVFOff);
                        }
                    }
                    else
                        (*pTriangle)(pContext, lpV1, lpV2, lpV0, &P2FVFOff);
                }

                lpV1 = lpV2;
                lpV2 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);
            }
 
            // Realign next command since vertices are dword aligned
            // and store # of primitives before affecting the pointer
            wCount = lpIns->wPrimitiveCount;
            lpIns  = (LPD3DHAL_DP2COMMAND)(( ((ULONG_PTR)lpIns) + 3 ) & ~ 3);

            EXIT_FROM_CONTEXT(pContext, ppdev);
            NEXTINSTRUCTION_S(lpIns, P2FVFOff.dwStride, 
                              wCount + 2, sizeof(D3DHAL_DP2TRIANGLEFAN_IMM));
            break;

        case D3DDP2OP_TEXBLT:
            // Inform the drivers to perform a BitBlt operation from a source
            // texture to a destination texture. A texture can also be cubic
            // environment map. The driver should copy a rectangle specified
            // by rSrc in the source texture to the location specified by pDest
            // in the destination texture. The destination and source textures
            // are identified by handles that the driver was notified with
            // during texture creation time. If the driver is capable of
            // managing textures, then it is possible that the destination
            // handle is 0. This indicates to the driver that it should preload
            // the texture into video memory (or wherever the hardware
            // efficiently textures from). In this case, it can ignore rSrc and
            // pDest. Note that for mipmapped textures, only one D3DDP2OP_TEXBLT
            // instruction is inserted into the D3dDrawPrimitives2 command stream.
            // In this case, the driver is expected to BitBlt all the mipmap
            // levels present in the texture.

            // Verify the command buffer validity
            if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                         lpCmdBufferEnd,
                                         lpPrim,
                                         sizeof(D3DHAL_DP2TEXBLT),
                                         lpIns->wStateCount,
                                         0))
            {
                goto ExitWithUnParsedError;
            }

            DBG_D3D((8,"D3DDP2OP_TEXBLT"));

            for ( i = 0; i < lpIns->wStateCount; i++)
            {
                __TextureBlt(pContext, (D3DHAL_DP2TEXBLT*)(lpPrim));
                lpPrim += sizeof(D3DHAL_DP2TEXBLT);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TEXBLT, lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_STATESET:
            {
                LPD3DHAL_DP2STATESET pStateSetOp = (LPD3DHAL_DP2STATESET)(lpPrim);
                DBG_D3D((8,"D3DDP2OP_STATESET"));
#if D3D_STATEBLOCKS
                for (i = 0; i < lpIns->wStateCount; i++, pStateSetOp++)
                {
                    switch (pStateSetOp->dwOperation)
                    {
                        case D3DHAL_STATESETBEGIN  :
                            __BeginStateSet(pContext,pStateSetOp->dwParam);
                        break;

                        case D3DHAL_STATESETEND    :
                            __EndStateSet(pContext);
                        break;

                        case D3DHAL_STATESETDELETE :
                            __DeleteStateSet(pContext,pStateSetOp->dwParam);
                        break;

                        case D3DHAL_STATESETEXECUTE:
                            __ExecuteStateSet(pContext,pStateSetOp->dwParam);
                        break;

                        case D3DHAL_STATESETCAPTURE:
                            __CaptureStateSet(pContext,pStateSetOp->dwParam);
                        break;

                        default :
                            DBG_D3D((0,"D3DDP2OP_STATESET has invalid"
                                "dwOperation %08lx",pStateSetOp->dwOperation));
                        break;
                    }
                }
#endif //D3D_STATEBLOCKS
                // Update the command buffer pointer
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2STATESET, 
                                lpIns->wStateCount, 0);
            }
            break;

        case D3DDP2OP_SETPALETTE:
            // Attach a palette to a texture, that is , map an association
            // between a palette handle and a surface handle, and specify
            // the characteristics of the palette. The number of
            // D3DNTHAL_DP2SETPALETTE structures to follow is specified by
            // the wStateCount member of the D3DNTHAL_DP2COMMAND structure

            {
                D3DHAL_DP2SETPALETTE* lpSetPal =
                                            (D3DHAL_DP2SETPALETTE*)(lpPrim);

                DBG_D3D((8,"D3DDP2OP_SETPALETTE"));

                // Verify the command buffer validity
                if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                             lpCmdBufferEnd,
                                             lpPrim,
                                             sizeof(D3DHAL_DP2SETPALETTE),
                                             lpIns->wStateCount,
                                             0))
                {
                    goto ExitWithUnParsedError;
                }

                for (i = 0; i < lpIns->wStateCount; i++, lpSetPal++)
                {
                    __PaletteSet(pContext,
                                lpSetPal->dwSurfaceHandle,
                                lpSetPal->dwPaletteHandle,
                                lpSetPal->dwPaletteFlags );
                }
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETPALETTE, 
                                lpIns->wStateCount, 0);
            }
            break;

        case D3DDP2OP_UPDATEPALETTE:
            // Perform modifications to the palette that is used for palettized
            // textures. The palette handle attached to a surface is updated
            // with wNumEntries PALETTEENTRYs starting at a specific wStartIndex
            // member of the palette. (A PALETTENTRY (defined in wingdi.h and
            // wtypes.h) is actually a DWORD with an ARGB color for each byte.) 
            // After the D3DNTHAL_DP2UPDATEPALETTE structure in the command
            // stream the actual palette data will follow (without any padding),
            // comprising one DWORD per palette entry. There will only be one
            // D3DNTHAL_DP2UPDATEPALETTE structure (plus palette data) following
            // the D3DNTHAL_DP2COMMAND structure regardless of the value of
            // wStateCount.

            {
                D3DHAL_DP2UPDATEPALETTE* lpUpdatePal =
                                          (D3DHAL_DP2UPDATEPALETTE*)(lpPrim);

                DBG_D3D((8,"D3DDP2OP_UPDATEPALETTE"));

                // Verify the command buffer validity
                if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                             lpCmdBufferEnd,
                                             lpPrim,
                                             sizeof(D3DHAL_DP2UPDATEPALETTE),
                                             1,
                                             (lpUpdatePal->wNumEntries * sizeof(PALETTEENTRY))))
                {
                    goto ExitWithUnParsedError;
                }

                // We will ALWAYS have only 1 palette update structure + palette
                // following the D3DDP2OP_UPDATEPALETTE token
                ASSERTDD(1 == lpIns->wStateCount,
                         "1 != wStateCount in D3DDP2OP_UPDATEPALETTE");

                __PaletteUpdate(pContext,
                                        lpUpdatePal->dwPaletteHandle,
                                        lpUpdatePal->wStartIndex,
                                        lpUpdatePal->wNumEntries,
                                        (BYTE*)(lpUpdatePal+1) );

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2UPDATEPALETTE, 
                                1,
                                (DWORD)lpUpdatePal->wNumEntries * 
                                     sizeof(PALETTEENTRY));
            }
            break;

        case D3DDP2OP_SETRENDERTARGET:
            // Map a new rendering target surface and depth buffer in
            // the current context.  This replaces the old D3dSetRenderTarget
            // callback. 

            {
                D3DHAL_DP2SETRENDERTARGET* pSRTData;

                // Verify the command buffer validity
                if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                             lpCmdBufferEnd,
                                             lpPrim,
                                             sizeof(D3DHAL_DP2SETRENDERTARGET),
                                             lpIns->wStateCount,
                                             0))
                {
                    goto ExitWithUnParsedError;
                }

                // Get new data by ignoring all but the last structure
                pSRTData = (D3DHAL_DP2SETRENDERTARGET*)lpPrim +
                           (lpIns->wStateCount - 1);

                __SetRenderTarget(pContext,
                                          pSRTData->hRenderTarget,
                                          pSRTData->hZBuffer);

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETRENDERTARGET,
                                lpIns->wStateCount, 0);
            }
            break;

        case D3DDP2OP_CLEAR:
            // Perform hardware-assisted clearing on the rendering target,
            // depth buffer or stencil buffer. This replaces the old D3dClear
            // and D3dClear2 callbacks. 

            {
                D3DHAL_DP2CLEAR* pClear;
                // Verify the command buffer validity
                if (!(IS_OP_WITHIN_CMDBUFFER(lpCmdBufferStart,
                                             lpCmdBufferEnd,
                                             lpPrim,
                                             sizeof(RECT),
                                             lpIns->wStateCount,
                                             (sizeof(D3DHAL_DP2CLEAR) - sizeof(RECT))))
                {
                    goto ExitWithUnParsedError;
                }

                // Get new data by ignoring all but the last structure
                pClear = (D3DHAL_DP2CLEAR*)lpPrim;

                DBG_D3D((8,"D3DDP2OP_CLEAR dwFlags=%08lx dwColor=%08lx "
                           "dvZ=%08lx dwStencil=%08lx",
                           pClear->dwFlags,
                           pClear->dwFillColor,
                           (DWORD)(pClear->dvFillDepth*0x0000FFFF),
                           pClear->dwFillStencil));

                __Clear(pContext, 
                                pClear->dwFlags,        // in:  surfaces to clear
                                pClear->dwFillColor,    // in:  Color value for rtarget
                                pClear->dvFillDepth,    // in:  Depth value for
                                                        //      Z-buffer (0.0-1.0)
                                pClear->dwFillStencil,  // in:  value used to clear stencil
                                                        // in:  Rectangles to clear
                                (LPD3DRECT)((LPBYTE)pClear + 
                                         sizeof(D3DHAL_DP2CLEAR) -
                                         sizeof(RECT)),
                                (DWORD)lpIns->wStateCount); // in:  Number of rectangles
                NEXTINSTRUCTION(lpIns, RECT, lpIns->wStateCount, 
                                (sizeof(D3DHAL_DP2CLEAR) - sizeof(RECT))); 
            }
            break;

#if D3DDX7_TL
        case D3DDP2OP_SETMATERIAL:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETMATERIAL,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_SETLIGHT:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETLIGHT,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_CREATELIGHT:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2CREATELIGHT,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_SETTRANSFORM:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETTRANSFORM,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_ZRANGE:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2ZRANGE,
                            lpIns->wStateCount, 0);
            break;
#endif //D3DDX7_TL

        default:

            ASSERTDD((pContext->pThisDisplay->lpD3DParseUnknownCommand),
                     "D3D DX6 ParseUnknownCommand callback == NULL");

            // Call the ParseUnknown callback to process 
            // any unidentifiable token
            lpdp2d->ddrval = ((PFND3DPARSEUNKNOWNCOMMAND)(pContext->pThisDisplay->lpD3DParseUnknownCommand))
                                ((VOID **) lpIns , (VOID **) &lpResumeIns);
            if ( lpdp2d->ddrval == DD_OK)
            {
                // Resume buffer processing after D3DParseUnknownCommand
                // was succesful in processing an unknown command
                lpIns = lpResumeIns;
                break;
            }

            DBG_D3D((2,"unhandled opcode (%d)- returning "
                        "D3DERR_COMMAND_UNPARSED @ addr %x",
                        lpIns->bCommand,lpIns));
            goto ExitWithError;
        } // switch

    } //while

    // any necessary housekeeping can be done here before leaving
    DBG_D3D((6,"Exiting D3DDrawPrimitives2"));
    return DDHAL_DRIVER_HANDLED;

ExitWithVertexBufferError:
    EXIT_FROM_CONTEXT(pContext, ppdev);

ExitWithUnParsedError:
    lpdp2d->ddrval          = D3DERR_COMMAND_UNPARSED;

ExitWithError:
    lpdp2d->dwErrorOffset   = (DWORD)((LPBYTE)lpIns-(LPBYTE)lpInsStart); 

    // any necessary housekeeping can be done here before leaving
    DBG_D3D((6,"Exiting D3DDrawPrimitives2"));
    return DDHAL_DRIVER_HANDLED;

} // D3DDrawPrimitives2

/**
 *
 * Function:    __CheckFVFRequest
 * Descrition:  This utility function verifies that the requested FVF format 
 *              makes sense and computes useful offsets into the data and 
 *              a stride between succesive vertices.
 */

static DWORD 
__CheckFVFRequest(DWORD dwFVF, 
                  LPP2FVFOFFSETS lpP2FVFOff)
{
    UINT iTexCount; 

    DBG_D3D((10,"Entering __CheckFVFRequest"));

    memset(lpP2FVFOff, 0, sizeof(P2FVFOFFSETS));

    if ( (dwFVF & (D3DFVF_RESERVED0 | D3DFVF_RESERVED1 | D3DFVF_RESERVED2 |
         D3DFVF_NORMAL)) ||
         ((dwFVF & (D3DFVF_XYZ | D3DFVF_XYZRHW)) == 0) )
    {
        // can't set reserved bits, shouldn't have normals in
        // output to rasterizers, and must have coordinates
        return DDERR_INVALIDPARAMS;
    }

    lpP2FVFOff->dwStride = sizeof(D3DVALUE) * 3;

    if (dwFVF & D3DFVF_XYZRHW)
    {
        lpP2FVFOff->dwStride += sizeof(D3DVALUE);
    }

    if (dwFVF & D3DFVF_DIFFUSE)
    {
        lpP2FVFOff->dwColOffset = lpP2FVFOff->dwStride;
        lpP2FVFOff->dwStride += sizeof(D3DCOLOR);
    }

    if (dwFVF & D3DFVF_SPECULAR)
    {
        lpP2FVFOff->dwSpcOffset = lpP2FVFOff->dwStride;
        lpP2FVFOff->dwStride  += sizeof(D3DCOLOR);
    }



    iTexCount = (dwFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

    if (iTexCount >= 1)
    {
        lpP2FVFOff->dwTexBaseOffset = lpP2FVFOff->dwStride;
        lpP2FVFOff->dwTexOffset = lpP2FVFOff->dwTexBaseOffset;

        if (0xFFFF0000 & dwFVF)
        {
            //expansion of FVF, these 16 bits are designated for up to 
            //8 sets of texture coordinates with each set having 2bits
            //Normally a capable driver has to process all coordinates
            //However, code below only show correct parsing w/o really
            //observing all the texture coordinates.In reality,this would 
            //result in incorrect result.
            UINT i,numcoord;
            DWORD extrabits = (dwFVF >> 16);

            for (i = 0; i < iTexCount; i++)
            {
                switch((extrabits & 0x3))
                {
                case    1:
                    // one more D3DVALUE for 3D textures
                    numcoord = 3;
                    break;
                case    2:
                    // two more D3DVALUEs for 4D textures
                    numcoord = 4;
                    break;
                case    3:
                    // one less D3DVALUE for 1D textures
                    numcoord = 1;
                    break;
                default:
                    // i.e. case 0 regular 2 D3DVALUEs
                    numcoord = 2;
                    break;
                }
                extrabits = extrabits >> 2;
                DBG_D3D((0,"Expanded TexCoord set %d has a offset %8lx",
                           i,lpP2FVFOff->dwStride));
                lpP2FVFOff->dwStride += sizeof(D3DVALUE) * numcoord;
            }
            DBG_D3D((0,"Expanded dwVertexType=0x%08lx has %d Texture Coords "
                       "with total stride=0x%08lx",
                       dwFVF, iTexCount, lpP2FVFOff->dwStride));
        }
        else
            lpP2FVFOff->dwStride   += iTexCount * sizeof(D3DVALUE) * 2;
    } 
    else
    {
        lpP2FVFOff->dwTexBaseOffset = 0;
        lpP2FVFOff->dwTexOffset = 0;
    }

    DBG_D3D((10,"Exiting __CheckFVFRequest"));
    return DD_OK;
} // __CheckFVFRequest

/**
 * Function:    __TextureBlt
 * Description: Transfer a texture from system memory into AGP or video memory
 */

static VOID 
__TextureBlt(PERMEDIA_D3DCONTEXT    *pContext,
             D3DHAL_DP2TEXBLT       *lpdp2texblt)
{
    PPERMEDIA_D3DTEXTURE    dsttex,srctex;
    RECTL                   rDest;
    LPP2THUNKEDDATA         ppdev = pContext->pThisDisplay;

    DBG_D3D((10,"Entering __TextureBlt"));

    if (0 == lpdp2texblt->dwDDSrcSurface)
    {
        DBG_D3D((0,"Inavlid handle TexBlt from %08lx to %08lx",
            lpdp2texblt->dwDDSrcSurface,lpdp2texblt->dwDDDestSurface));
        return;
    }
    srctex=TextureHandleToPtr(lpdp2texblt->dwDDSrcSurface,pContext);
    ASSERTDD((srctex), "__TextureBlt: invalid Texture Handle !");
    if (0 == lpdp2texblt->dwDDDestSurface)
    {
        PPERMEDIA_D3DTEXTURE pTexture=srctex;
        P2_MANAGED_TEXTURE  *pManagedTexture = &pTexture->P2ManagedTexture;

        if (!(pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
        {
            DBG_D3D((0,"Must be a managed texture to do texture preload"));
            return;
        }
        if ((FLATPTR)NULL == pManagedTexture->fpVidMem)
        {
            DBG_D3D((0,"__TextureBlt: TextureCacheManagerAllocNode: surfaceHandle=0x%x",lpdp2texblt->dwDDSrcSurface));
            TextureCacheManagerAllocNode(pContext,pTexture);
            if ((FLATPTR)NULL == pManagedTexture->fpVidMem)
            {
                DBG_D3D((0,"EnableTexturePermedia unable to allocate memory from heap"));
                return;
            }
            pManagedTexture->dwFlags |= P2_SURFACE_NEEDUPDATE;
        }
        TextureCacheManagerTimeStamp(pContext->pTextureManager,pTexture);
        if (pManagedTexture->dwFlags & P2_SURFACE_NEEDUPDATE)
        {
            LoadManagedTextureToVidMem(ppdev, pTexture);
        }
        return;
    }
    else
    {
        dsttex=TextureHandleToPtr(lpdp2texblt->dwDDDestSurface,pContext);
    }

    if (NULL != dsttex && NULL != srctex)
    {
        rDest.left      = lpdp2texblt->pDest.x;
        rDest.top       = lpdp2texblt->pDest.y;
        rDest.right     = rDest.left + lpdp2texblt->rSrc.right - lpdp2texblt->rSrc.left;
        rDest.bottom    = rDest.top + lpdp2texblt->rSrc.bottom - lpdp2texblt->rSrc.top;

        DBG_D3D((4,"TexBlt from %x to %x",srctex,dsttex));

        /*
         * Please refer to function GetDDHALInfo in ddinit.c 
         * We notice that ddCaps.dwVSBCaps = 0 and ddCaps.dwSSBCaps = 0
         * Also since we are guaranteed that the Source and Destination Surfaces have
         * the same pixel formats, and since the Source and Destination rectangles are
         * identical, we need to worry about plain BLTs and not worry about stretching/mirroring
         * This is for VideoMemory to VideoMemory 
         *  and        SystemMemory to VideMemory blts.
         */


        if ((srctex->dwCaps & DDSCAPS_VIDEOMEMORY) &&
            (!(srctex->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)))
        {

            ASSERTDD((dsttex->dwCaps & DDSCAPS_VIDEOMEMORY), 
                        "__TextureBlt: Dest Texture not in VideoMemory");

            if (dsttex->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
            {
                /* VidMem -> Video Memory(Managed Texture) */
                SysMemToSysMemSurfaceCopy(ppdev,
                                          srctex->fpVidMem,
                                          srctex->lPitch,
                                          srctex->ddpfSurface.dwRGBBitCount,
                                          dsttex->fpVidMem,
                                          dsttex->lPitch,
                                          dsttex->ddpfSurface.dwRGBBitCount, 
                                          &lpdp2texblt->rSrc, 
                                          &rDest);
            }
            /*
             * VideoMemory to VideoMemory Blts 
             * where Source and Destination surfaces are not managed surfaces.
             */
            else if (srctex->dwCaps & DDSCAPS_NONLOCALVIDMEM)
            {
                if (dsttex->dwCaps & DDSCAPS_NONLOCALVIDMEM)
                {
                    /* Non-Local VidMem -> Non-Local VidMem */
                    SysMemToSysMemSurfaceCopy(ppdev,
                                              srctex->fpVidMem,
                                              srctex->lPitch,
                                              srctex->ddpfSurface.dwRGBBitCount,
                                              dsttex->fpVidMem,
                                              dsttex->lPitch,
                                              dsttex->ddpfSurface.dwRGBBitCount, 
                                              &lpdp2texblt->rSrc, 
                                              &rDest);
                }
                else
                {
                    /* Non-Local VidMem -> Local VidMem */
                    VidMemToVidMemStretchCopyBlt(ppdev, 
                                                 srctex->fpVidMem,
                                                 dsttex->fpVidMem,
                                                 &srctex->ddpfSurface,
                                                 &dsttex->ddpfSurface,
                                                 &lpdp2texblt->rSrc, 
                                                 &rDest,
                                                 srctex->wWidth,
                                                 dsttex->wWidth,
                                                 1,
                                                 1,
                                                 FALSE,
                                                 FALSE);
                }
            }
            else 
            {
                if (!(dsttex->dwCaps & DDSCAPS_NONLOCALVIDMEM))
                {
                    /* Local VidMem -> Local VidMem */
                    DWORD dwRenderDirection;

                    dwRenderDirection = GetRenderDirection(srctex->fpVidMem,
                                                           dsttex->fpVidMem,
                                                           &lpdp2texblt->rSrc,
                                                           &rDest);

                    VidMemToVidMemStretchCopyBlt(ppdev, 
                                                 srctex->fpVidMem,
                                                 dsttex->fpVidMem,
                                                 &srctex->ddpfSurface,
                                                 &dsttex->ddpfSurface,
                                                 &lpdp2texblt->rSrc, 
                                                 &rDest,
                                                 srctex->wWidth,
                                                 dsttex->wWidth,
                                                 0,
                                                 dwRenderDirection,
                                                 GetXMirror(NULL, dwRenderDirection),
                                                 GetYMirror(NULL, dwRenderDirection));

                }
                else
                {
                    ASSERTDD((0), "__TextureBlt: Local VidMem -> Non-Local VidMem !");
                }
            }
        }
        else if ((dsttex->dwCaps2 & DDSCAPS2_TEXTUREMANAGE) ||
                 (dsttex->dwCaps  & DDSCAPS_NONLOCALVIDMEM))
        {   
            /*
             * This takes care of the cases 
             *  Video Memory(Managed Texture) -> Video Memory(Managed Texture) 
             *  System Memory -> Video Memory(Managed Texture) 
             *  System Memory -> Video Memory(Non-Local)
             */
            SysMemToSysMemSurfaceCopy(ppdev,
                                      srctex->fpVidMem,
                                      srctex->lPitch,
                                      srctex->ddpfSurface.dwRGBBitCount,
                                      dsttex->fpVidMem,
                                      dsttex->lPitch,
                                      dsttex->ddpfSurface.dwRGBBitCount, 
                                      &lpdp2texblt->rSrc, 
                                      &rDest);
            if (dsttex->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
            {
                if (pContext->CurrentTextureHandle == lpdp2texblt->dwDDDestSurface)
                {
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
                }
                dsttex->P2ManagedTexture.dwFlags |= P2_SURFACE_NEEDUPDATE;
            }
        }
        else
        {
            /* System Memory -> Video Memory(Local) */
            SysMemToVidMemPackedDownload(ppdev, 
                                         srctex->fpVidMem,
                                         srctex->lPitch,
                                         &lpdp2texblt->rSrc,
                                         dsttex->fpVidMem,
                                         dsttex->wWidth,
                                         &rDest,
                                         srctex->ddpfSurface.dwRGBBitCount);
        }
        dsttex->dwPaletteHandle = srctex->dwPaletteHandle;
    }

    DBG_D3D((10,"Exiting __TextureBlt"));
    return;
}   //__TextureBlt

/**
 * Function:    __SetRenderTarget
 * Description: Set new render and z buffer target surfaces
 */
            
static VOID  
__SetRenderTarget(PERMEDIA_D3DCONTEXT   *pContext,
                  DWORD                 hRenderTarget,
                  DWORD                 hZBuffer)
{
    DBG_D3D((10,"Entering __SetRenderTarget Target=%d Z=%d",
                                        hRenderTarget,hZBuffer));
    // Call a function to initialise registers that will setup the rendering
    pContext->RenderSurfaceHandle = hRenderTarget;
    pContext->ZBufferHandle = hZBuffer;
    SetupPermediaRenderTarget(pContext);

    DBG_D3D((10,"Exiting __SetRenderTarget"));

    return;
} // __SetRenderTarget


/**
 * Function:    __Clear
 * Description: Clears selectively the frame buffer, z buffer and stencil buffer 
 *              for the D3D Clear2 callback and for the D3DDP2OP_CLEAR command token.
 */

static VOID  
__Clear(PERMEDIA_D3DCONTEXT *pContext,
        DWORD               dwFlags,        // in:  surfaces to clear
        DWORD               dwFillColor,    // in:  Color value for rtarget
        D3DVALUE            dvFillDepth,    // in:  Depth value for
                                            //      Z-buffer (0.0-1.0)
        DWORD               dwFillStencil,  // in:  value used to clear stencil buffer
        LPD3DRECT           lpRects,        // in:  Rectangles to clear
        DWORD               dwNumRects)     // in:  Number of rectangles
{
    int                 i;
    RECTL               *pRect;
    LPP2THUNKEDDATA     ppdev = pContext->pThisDisplay;

    if (D3DCLEAR_TARGET & dwFlags)
    {
        DWORD   a,r,g,b;

        PPERMEDIA_D3DTEXTURE    pSurfRender = 
            TextureHandleToPtr(pContext->RenderSurfaceHandle, pContext);
        ASSERTDD((pSurfRender), "D3DDP2OP_CLEAR: invalid RenderSurfaceHandle !");

        // Translate into HW specific format
        a = RGB888ToHWFmt(dwFillColor,
                          pSurfRender->ddpfSurface.dwRGBAlphaBitMask, 
                          0x80000000);
        r = RGB888ToHWFmt(dwFillColor,
                          pSurfRender->ddpfSurface.dwRBitMask, 
                          0x00800000);
        g = RGB888ToHWFmt(dwFillColor,
                          pSurfRender->ddpfSurface.dwGBitMask, 
                          0x00008000);
        b = RGB888ToHWFmt(dwFillColor,
                          pSurfRender->ddpfSurface.dwBBitMask, 
                          0x00000080);

        dwFillColor = a | r | g | b;

        DBG_D3D((8,"D3DDP2OP_CLEAR convert to %08lx with Mask %8lx %8lx %8lx",
                   dwFillColor,
                   pSurfRender->ddpfSurface.dwRBitMask,
                   pSurfRender->ddpfSurface.dwGBitMask,
                   pSurfRender->ddpfSurface.dwBBitMask));

        pRect = (RECTL*)lpRects;

        // Do clear for each Rect that we have
        for (i = dwNumRects; i > 0; i--)
        {
            VidMemFastFill(ppdev, 
                           pSurfRender->fpVidMem,
                           pRect,
                           dwFillColor,
                           pSurfRender->ddpfSurface.dwRGBBitCount,
                           pSurfRender->wWidth,
                           0xFFFFFFFF);
            pRect++;
        }
    }

    if (((D3DCLEAR_ZBUFFER
#if D3D_STENCIL
        | D3DCLEAR_STENCIL
#endif  //D3D_STENCIL
        ) & dwFlags) 
        && (0 != pContext->ZBufferHandle))
    {
        DWORD                   dwZbufferClearValue = 0x0000FFFF; //no stencil case
        DWORD                   dwWriteMask;
        PPERMEDIA_D3DTEXTURE    pSurfZBuffer = 
                                    TextureHandleToPtr(pContext->ZBufferHandle, pContext);
        ASSERTDD((pSurfZBuffer), "D3DDP2OP_CLEAR: invalid ZBufferHandle !");

#if D3D_STENCIL
        //actually check dwStencilBitMask
        if (0 == pSurfZBuffer->ddpfSurface.dwBBitMask)
        {
            dwWriteMask = 0xFFFFFFFF;   //all 16bits are for Z
            dwZbufferClearValue = (DWORD)(dvFillDepth*0x0000FFFF);
        }
        else
        {
            dwWriteMask = 0;
            dwZbufferClearValue = (DWORD)(dvFillDepth*0x00007FFF);

            if (D3DCLEAR_ZBUFFER & dwFlags)
                dwWriteMask |= 0x7FFF7FFF;

            if (D3DCLEAR_STENCIL & dwFlags)
            {
                dwWriteMask |= 0x80008000;
                if (0 != dwFillStencil)
                {
                    dwZbufferClearValue |= 0x8000;  //or stencil bit
                }
            }
        }
#endif  //D3D_STENCIL

        pRect = (RECTL*)lpRects;

        for (i = dwNumRects; i > 0; i--)
        {                
            VidMemFastFill(ppdev, 
                           pSurfZBuffer->fpVidMem,
                           pRect,
                           dwZbufferClearValue,
                           16,
                           pSurfZBuffer->wWidth,
                           dwWriteMask);
            pRect++;
        }
    }
    
    return;

} // __Clear



