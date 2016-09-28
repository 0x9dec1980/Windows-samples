/******************************Module*Header**********************************\
*
*                           *******************************
*                           * Permedia 2: D3D SAMPLE CODE *
*                           *******************************
*
* Module Name: d3dstate.c
*
*       Contains code to translate D3D renderstates and texture stage
*       states into hardware specific settings.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

#if D3D_STATEBLOCKS

static P2StateSetRec *
FindStateSet(PERMEDIA_D3DCONTEXT *pContext,
             DWORD               dwHandle);

static VOID 
AddStateSetIndexTableEntry(PERMEDIA_D3DCONTEXT *pContext,
                           DWORD               dwNewHandle,
                           P2StateSetRec       *pNewSSRec);

static P2StateSetRec * 
CompressStateSet(PERMEDIA_D3DCONTEXT    *pContext,
                 P2StateSetRec          *pUncompressedSS);


#endif // #if D3D_STATEBLOCKS

//-----------------------------------------------------------------------------
//
// void __SelectFVFTexCoord
//
// This utility function sets the correct texture offset depending on the
// texturing coordinate set wished to be used from the FVF vertexes
//
//-----------------------------------------------------------------------------
void 
__SelectFVFTexCoord(LPP2FVFOFFSETS  lpP2FVFOff, 
                    DWORD           dwTexCoord)
{
    DBG_D3D((10,"Entering __SelectFVFTexCoord"));

    lpP2FVFOff->dwTexOffset = lpP2FVFOff->dwTexBaseOffset + 
                                dwTexCoord * 2 * sizeof(D3DVALUE);

    // verify the requested texture coordinate doesn't exceed the FVF 
    // vertex structure provided , if so go down to set 0 as a 
    // crash-avoiding alternative
    if (lpP2FVFOff->dwTexOffset >= lpP2FVFOff->dwStride)
        lpP2FVFOff->dwTexOffset = lpP2FVFOff->dwTexBaseOffset;

    DBG_D3D((10,"Exiting __SelectFVFTexCoord"));
} // __SelectFVFTexCoord


//-----------------------------------------------------------------------------
//
// HRESULT PreProcessTextureStageState
//
// Processes the state changes that must be done as soon as they arrive
//
//-----------------------------------------------------------------------------
void 
PreProcessTextureStageState(PERMEDIA_D3DCONTEXT *pContext, 
                            DWORD               dwStage, 
                            DWORD               dwState, 
                            DWORD               dwValue)
{
    DBG_D3D((10,"Entering PreProcessTextureStageState"));

    if (D3DTSS_ADDRESS == dwState)
    {
        pContext->TssStates[D3DTSS_ADDRESSU] = dwValue;
        pContext->TssStates[D3DTSS_ADDRESSV] = dwValue;
    }
    else if ((D3DTSS_TEXTUREMAP == dwState) && (0 != dwValue))
    {
        PPERMEDIA_D3DTEXTURE   pTexture=TextureHandleToPtr(dwValue, pContext);

        if (pTexture)
        {
            if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE) 
            {
                TextureCacheManagerIncNumTexturesSet(pContext->pTextureManager);
                if (pTexture->m_dwHeapIndex)
                    TextureCacheManagerIncNumSetTexInVid(pContext->pTextureManager);
            }
        }
    }

    DBG_D3D((10,"Exiting PreProcessTextureStageState"));
} // PreProcessTextureStageState

//-----------------------------------------------------------------------------
//
// void MapRenderStateToTextureStage0State
//
// Map Renderstate changes into the corresponding change in the Texture Stage
// State #0 .
//
//-----------------------------------------------------------------------------
void 
MapRenderStateToTextureStage0State(PERMEDIA_D3DCONTEXT   *pContext,
                                   DWORD                 dwRSType,
                                   DWORD                 dwRSVal)
{
    DBG_D3D((10,"Entering MapRenderStateToTextureStage0State"));

    // Process each specific renderstate
    switch (dwRSType)
    {
        case D3DRENDERSTATE_TEXTUREHANDLE:
            //Mirror texture related render states into TSS stage 0
            pContext->TssStates[D3DTSS_TEXTUREMAP] = dwRSVal;
        break;

        case D3DRENDERSTATE_TEXTUREMAPBLEND:
            switch (dwRSVal)
            {
                case D3DTBLEND_DECALALPHA:
                    //Mirror texture related render states into TSS stage 0
                    pContext->TssStates[D3DTSS_COLOROP]     =
                                                   D3DTOP_BLENDTEXTUREALPHA;
                    pContext->TssStates[D3DTSS_COLORARG1]   = D3DTA_TEXTURE;
                    pContext->TssStates[D3DTSS_COLORARG2]   = D3DTA_DIFFUSE;
                    pContext->TssStates[D3DTSS_ALPHAOP]     = D3DTOP_SELECTARG2;
                    pContext->TssStates[D3DTSS_ALPHAARG1]   = D3DTA_TEXTURE;
                    pContext->TssStates[D3DTSS_ALPHAARG2]   = D3DTA_DIFFUSE;
                break;
                case D3DTBLEND_MODULATE:
                    //Mirror texture related render states into TSS stage 0
                    pContext->TssStates[D3DTSS_COLOROP]     = D3DTOP_MODULATE;
                    pContext->TssStates[D3DTSS_COLORARG1]   = D3DTA_TEXTURE;
                    pContext->TssStates[D3DTSS_COLORARG2]   = D3DTA_DIFFUSE;
                    // a special legacy alpha operation is called for
                    // that depends on the format of the texture
                    pContext->TssStates[D3DTSS_ALPHAOP]     = D3DTOP_LEGACY_ALPHAOVR;
                    pContext->TssStates[D3DTSS_ALPHAARG1]   = D3DTA_TEXTURE;
                    pContext->TssStates[D3DTSS_ALPHAARG2]   = D3DTA_DIFFUSE;
                break;
                case D3DTBLEND_MODULATEALPHA:
                    //Mirror texture related render states into TSS stage 0
                    pContext->TssStates[D3DTSS_COLOROP]     = D3DTOP_MODULATE;
                    pContext->TssStates[D3DTSS_COLORARG1]   = D3DTA_TEXTURE;
                    pContext->TssStates[D3DTSS_COLORARG2]   = D3DTA_DIFFUSE;
                    pContext->TssStates[D3DTSS_ALPHAOP]     = D3DTOP_MODULATE;
                    pContext->TssStates[D3DTSS_ALPHAARG1]   = D3DTA_TEXTURE;;
                    pContext->TssStates[D3DTSS_ALPHAARG2]   = D3DTA_DIFFUSE;
                break;
                case D3DTBLEND_COPY:
                case D3DTBLEND_DECAL:
                    //Mirror texture related render states into TSS stage 0
                    pContext->TssStates[D3DTSS_COLOROP]     = D3DTOP_SELECTARG1;
                    pContext->TssStates[D3DTSS_COLORARG1]   = D3DTA_TEXTURE;
                    pContext->TssStates[D3DTSS_ALPHAOP]     = D3DTOP_SELECTARG1;
                    pContext->TssStates[D3DTSS_ALPHAARG1]   = D3DTA_TEXTURE;
                break;
                case D3DTBLEND_ADD:
                    //Mirror texture related render states into TSS stage 0
                    pContext->TssStates[D3DTSS_COLOROP]     = D3DTOP_ADD;
                    pContext->TssStates[D3DTSS_COLORARG1]   = D3DTA_TEXTURE;
                    pContext->TssStates[D3DTSS_COLORARG2]   = D3DTA_DIFFUSE;
                    pContext->TssStates[D3DTSS_ALPHAOP]     = D3DTOP_SELECTARG2;
                    pContext->TssStates[D3DTSS_ALPHAARG1]   = D3DTA_TEXTURE;
                    pContext->TssStates[D3DTSS_ALPHAARG2]   = D3DTA_DIFFUSE;
                break;
            }
        break;

        case D3DRENDERSTATE_BORDERCOLOR:
            //Mirror texture related render states into TSS stage 0
            pContext->TssStates[D3DTSS_BORDERCOLOR]     = dwRSVal;
        break;

        case D3DRENDERSTATE_MIPMAPLODBIAS:
            //Mirror texture related render states into TSS stage 0
            pContext->TssStates[D3DTSS_MIPMAPLODBIAS]   = dwRSVal;
        break;

        case D3DRENDERSTATE_ANISOTROPY:
            //Mirror texture related render states into TSS stage 0
            pContext->TssStates[D3DTSS_MAXANISOTROPY]   = dwRSVal;
        break;

        case D3DRENDERSTATE_TEXTUREADDRESS:
            //Mirror texture related render states into TSS stage 0
            pContext->TssStates[D3DTSS_ADDRESSU] =
            pContext->TssStates[D3DTSS_ADDRESSV]        = dwRSVal; 
        break;

        case D3DRENDERSTATE_TEXTUREADDRESSU:
            //Mirror texture related render states into TSS stage 0
            pContext->TssStates[D3DTSS_ADDRESSU]        = dwRSVal;
        break;

        case D3DRENDERSTATE_TEXTUREADDRESSV:
            //Mirror texture related render states into TSS stage 0
            pContext->TssStates[D3DTSS_ADDRESSV]        = dwRSVal;
        break;

        case D3DRENDERSTATE_TEXTUREMAG:
            switch(dwRSVal)
            {
                case D3DFILTER_NEAREST:
                    pContext->TssStates[D3DTSS_MAGFILTER] = D3DTFG_POINT;
                break;
                case D3DFILTER_LINEAR:
                case D3DFILTER_MIPLINEAR:
                case D3DFILTER_MIPNEAREST:
                case D3DFILTER_LINEARMIPNEAREST:
                case D3DFILTER_LINEARMIPLINEAR:
                    pContext->TssStates[D3DTSS_MAGFILTER] = D3DTFG_LINEAR;
                break;
                default:
                break;
            }
        break;

        case D3DRENDERSTATE_TEXTUREMIN:
            switch(dwRSVal)
            {
                case D3DFILTER_NEAREST:
                    pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_POINT;
                    pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_NONE;
                break;
                case D3DFILTER_LINEAR:
                    pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
                    pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_NONE;
                break;
                case D3DFILTER_MIPNEAREST:
                    pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_POINT;
                    pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_POINT;
                break;
                case D3DFILTER_MIPLINEAR:
                    pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
                    pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_POINT;
                break;
                case D3DFILTER_LINEARMIPNEAREST:
                    pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_POINT;
                    pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_LINEAR;
                break;
                case D3DFILTER_LINEARMIPLINEAR:
                    pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
                    pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_LINEAR;
                break;;
                default:
                break;
            }
        break;

        default:
            // All other renderstates don't have a corresponding TSS state so
            // we don't have to worry about mapping them.
        break;

    } // switch (dwRSType of renderstate)

    DBG_D3D((10,"Exiting MapRenderStateToTextureStage0State"));

} // MapRenderStateToTextureStage0State



//-----------------------------------------------------------------------------
//
// DWORD ProcessRenderStates
//
// Handle render state changes that arrive through the D3DDP2OP_RENDERSTATE
// token in the DP2 command stream.
//
//-----------------------------------------------------------------------------
DWORD 
ProcessRenderStates(PERMEDIA_D3DCONTEXT *pContext, 
                    DWORD               dwCount,
                    LPD3DSTATE          lpState,
                    LPDWORD             lpStateMirror)
{

    DWORD dwRSType, dwRSVal, i;

    DBG_D3D((10,"Entering ProcessRenderStates"));
    DBG_D3D((4, "ProcessRenderStates: Processing %d State changes", dwCount));

    // Loop through all renderstates passed in the DP2 command stream
    for (i = 0; i < dwCount; i++, lpState++)
    {
        dwRSType = (DWORD) lpState->drstRenderStateType;
        dwRSVal  = (DWORD) lpState->dwArg[0];

        DBG_D3D((8, "ProcessRenderStates state %d value = %d",
                                          dwRSType, dwRSVal));

        // Check validity of the render state
        if (!VALID_STATE(dwRSType))
        {
            DBG_D3D((0, "state 0x%08x is invalid", dwRSType));
            return DDERR_INVALIDPARAMS;
        }

        // Verify if state needs to be overrided or ignored
        if (IS_OVERRIDE(dwRSType))
        {
            DWORD override = GET_OVERRIDE(dwRSType);
            if (dwRSVal)
            {
                DBG_D3D((4, "in RenderState, setting override for state %d",
                                                                   override));
                STATESET_SET(pContext->overrides, override);
            }
            else
            {
                DBG_D3D((4, "in RenderState, clearing override for state %d",
                                                                    override));
                STATESET_CLEAR(pContext->overrides, override);
            }
            continue;
        }

        if (STATESET_ISSET(pContext->overrides, dwRSType))
        {
            DBG_D3D((4, "in RenderState, state %d is overridden, ignoring",
                                                                      dwRSType));
            continue;
        }

#if D3D_STATEBLOCKS
        if (!pContext->bStateRecMode)
        {
#endif D3D_STATEBLOCKS
            // Store the state in the context
            pContext->RenderStates[dwRSType] = dwRSVal;

            // Mirror value
            if ( lpStateMirror )
                lpStateMirror[dwRSType] = dwRSVal;


            SetupP2RenderStates(pContext, dwRSType, dwRSVal);
            // Mirror any change that happened in the render states into TSS 0
            MapRenderStateToTextureStage0State(pContext, dwRSType, dwRSVal);
#if D3D_STATEBLOCKS
        }
        else
        {
            if (pContext->pCurrSS != NULL)
            {
                DBG_D3D((6,"Recording RS %x = %x",dwRSType,dwRSVal));

                // Recording the state in a stateblock
                pContext->pCurrSS->u.uc.RenderStates[dwRSType] = dwRSVal;
                FLAG_SET(pContext->pCurrSS->u.uc.bStoredRS,dwRSType);
            }
        }
#endif D3D_STATEBLOCKS

    } // for (i)

    DBG_D3D((10,"Exiting ProcessRenderStates"));

    return DD_OK;
} // ProcessRenderStates

#if D3D_STATEBLOCKS
//-----------------------------------------------------------------------------
//
// P2StateSetRec *FindStateSet
//
// Find a state identified by dwHandle starting from pRootSS.
// If not found, returns NULL.
//
//-----------------------------------------------------------------------------
P2StateSetRec *
FindStateSet(PERMEDIA_D3DCONTEXT *pContext,
             DWORD               dwHandle)
{
    if (dwHandle <= pContext->dwMaxSSIndex)
        return pContext->pIndexTableSS[dwHandle - 1];
    else
    {
        DBG_D3D((2,"State set %x not found (Max = %x)",
                    dwHandle, pContext->dwMaxSSIndex));
        return NULL;
    }
}

//-----------------------------------------------------------------------------
//
// void DumpStateSet
//
// Dump info stored in a state set
//
//-----------------------------------------------------------------------------
#define ELEMS_IN_ARRAY(a) ((sizeof(a)/sizeof(a[0])))

void 
DumpStateSet(P2StateSetRec *pSSRec)
{
    DWORD i;

    DBG_D3D((0,"DumpStateSet %x, Id=%x bCompressed=%x",
                pSSRec,pSSRec->dwHandle,pSSRec->bCompressed));

    if (!pSSRec->bCompressed)
    {
        // uncompressed state set

        // Dump render states values
        for (i=0; i< MAX_STATE; i++)
        {
            DBG_D3D((0,"RS %x = %x",i, pSSRec->u.uc.RenderStates[i]));
        }

        // Dump TSS's values
        for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
        {
            DBG_D3D((0,"TSS %x = %x",i, pSSRec->u.uc.TssStates[i]));
        }

        // Dump RS bit masks
        for (i=0; i< ELEMS_IN_ARRAY(pSSRec->u.uc.bStoredRS); i++)
        {
            DBG_D3D((0,"bStoredRS[%x] = %x",i, pSSRec->u.uc.bStoredRS[i]));
        }

        // Dump TSS bit masks
        for (i=0; i< ELEMS_IN_ARRAY(pSSRec->u.uc.bStoredTSS); i++)
        {
            DBG_D3D((0,"bStoredTSS[%x] = %x",i, pSSRec->u.uc.bStoredTSS[i]));
        }

    }
    else
    {
        // compressed state set

        DBG_D3D((0,"dwNumRS =%x  dwNumTSS=%x",
                    pSSRec->u.cc.dwNumRS,pSSRec->u.cc.dwNumTSS));

        // dump compressed state
        for (i=0; i< pSSRec->u.cc.dwNumTSS + pSSRec->u.cc.dwNumRS; i++)
        {
            DBG_D3D((0,"RS/TSS %x = %x",
                        pSSRec->u.cc.pair[i].dwType, 
                        pSSRec->u.cc.pair[i].dwValue));
        }

    }

}

//-----------------------------------------------------------------------------
//
// void AddStateSetIndexTableEntry
//
// Add an antry to the index table. If necessary, grow it.
//-----------------------------------------------------------------------------
VOID 
AddStateSetIndexTableEntry(PERMEDIA_D3DCONTEXT *pContext,
                           DWORD               dwNewHandle,
                           P2StateSetRec       *pNewSSRec)
{
    DWORD           dwNewSize;
    P2StateSetRec   **pNewIndexTableSS;

    // If the current list is not large enough, we'll have to grow a new one.
    if (dwNewHandle > pContext->dwMaxSSIndex)
    {
        // New size of our index table
        // (round up dwNewHandle in steps of SSPTRS_PERPAGE)
        dwNewSize = ((dwNewHandle -1 + SSPTRS_PERPAGE) / SSPTRS_PERPAGE)
                      * SSPTRS_PERPAGE;

        // we have to grow our list
        pNewIndexTableSS = (P2StateSetRec **)D3DMALLOCZ(dwNewSize*sizeof(P2StateSetRec *));

        if (pContext->pIndexTableSS)
        {
            // if we already had a previous list, we must transfer its data
            memcpy(pNewIndexTableSS, 
                   pContext->pIndexTableSS,
                   pContext->dwMaxSSIndex*sizeof(P2StateSetRec *));
            
            //and get rid of it
            D3DFREE(pContext->pIndexTableSS);
        }

        // New index table data
        pContext->pIndexTableSS = pNewIndexTableSS;
        pContext->dwMaxSSIndex  = dwNewSize;
    }

    // Store our state set pointer into our access list
    pContext->pIndexTableSS[dwNewHandle - 1] = pNewSSRec;
}

//-----------------------------------------------------------------------------
//
// void CompressStateSet
//
// Compress a state set so it uses the minimum necessary space. Since we expect 
// some apps to make extensive use of state sets we want to keep things tidy.
// Returns address of new structure (ir old, if it wasn't compressed)
//
//-----------------------------------------------------------------------------
P2StateSetRec * 
CompressStateSet(PERMEDIA_D3DCONTEXT    *pContext,
                 P2StateSetRec          *pUncompressedSS)
{
    P2StateSetRec   *pCompressedSS;
    DWORD           i, dwSize, dwIndex, dwCount;

    // Create a new state set of just the right size we need

    // Calculate how large 
    dwCount = 0;
    for (i=0; i< MAX_STATE; i++)
    {
        if (IS_FLAG_SET(pUncompressedSS->u.uc.bStoredRS , i))
        {
            dwCount++;
        };
    }

    for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
    {
        if (IS_FLAG_SET(pUncompressedSS->u.uc.bStoredTSS , i))
        {
            dwCount++;
        };
    }

    // Create a new state set of just the right size we need
    // ANY CHANGE MADE TO THE P2StateSetRec structure MUST BE REFLECTED HERE!
    dwSize = 2*sizeof(DWORD) +                          // handle , flags
             2*sizeof(DWORD) +                          // # of RS & TSS
             2*dwCount*sizeof(DWORD);                   // compressed structure

    if (dwSize >= sizeof(P2StateSetRec))
    {
        // it is not efficient to compress, leave uncompressed !
        pUncompressedSS->bCompressed = FALSE;
        return pUncompressedSS;
    }

    pCompressedSS = (P2StateSetRec *)D3DMALLOCZ(dwSize);

    if (pCompressedSS)
    {
        // adjust data in new compressed state set
        pCompressedSS->bCompressed = TRUE;
        pCompressedSS->dwHandle = pUncompressedSS->dwHandle;

        // Transfer our info to this new state set
        pCompressedSS->u.cc.dwNumRS = 0;
        pCompressedSS->u.cc.dwNumTSS = 0;
        dwIndex = 0;

        for (i=0; i< MAX_STATE; i++)
        {
            if (IS_FLAG_SET(pUncompressedSS->u.uc.bStoredRS , i))
            {
                pCompressedSS->u.cc.pair[dwIndex].dwType = i;
                pCompressedSS->u.cc.pair[dwIndex].dwValue = 
                                    pUncompressedSS->u.uc.RenderStates[i];
                pCompressedSS->u.cc.dwNumRS++;
                dwIndex++;
            }
        }

        for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
        {
            if (IS_FLAG_SET(pUncompressedSS->u.uc.bStoredTSS , i))
            {
                pCompressedSS->u.cc.pair[dwIndex].dwType = i;
                pCompressedSS->u.cc.pair[dwIndex].dwValue = 
                                    pUncompressedSS->u.uc.TssStates[i];
                pCompressedSS->u.cc.dwNumTSS++;
                dwIndex++;
            }
        }

        // Get rid of the old(uncompressed) one
        D3DFREE(pUncompressedSS);
        return pCompressedSS;

    }
    else
    {
        DBG_D3D((0,"Not enough memory left to compress D3D state set"));
        pUncompressedSS->bCompressed = FALSE;
        return pUncompressedSS;
    }

}

//-----------------------------------------------------------------------------
//
// void __DeleteAllStateSets
//
// Delete any remaining state sets for cleanup purpouses
//
//-----------------------------------------------------------------------------
void 
__DeleteAllStateSets(PERMEDIA_D3DCONTEXT* pContext)
{
    P2StateSetRec *pSSRec;
    DWORD dwSSIndex;

    DBG_D3D((10,"Entering __DeleteAllStateSets"));

    if (pContext->pIndexTableSS)
    {
        for(dwSSIndex = 0; dwSSIndex < pContext->dwMaxSSIndex; dwSSIndex++)
        {
            if (pSSRec = pContext->pIndexTableSS[dwSSIndex])
            {
                D3DFREE(pSSRec);
            }
        }

        // free fast index table
        D3DFREE(pContext->pIndexTableSS);
    }

    DBG_D3D((10,"Exiting __DeleteAllStateSets"));
}

//-----------------------------------------------------------------------------
//
// void __BeginStateSet
//
// Create a new state set identified by dwParam and start recording states
//
//-----------------------------------------------------------------------------
void 
__BeginStateSet(PERMEDIA_D3DCONTEXT    *pContext, 
                DWORD                  dwParam)
{

    P2StateSetRec *pSSRec;

    DBG_D3D((10,"Entering __BeginStateSet dwParam=%08lx",dwParam));
    // Create a new state set
    pSSRec = (P2StateSetRec *)D3DMALLOCZ(sizeof(P2StateSetRec));
    if (!pSSRec)
    {
        DBG_D3D((0,"Run out of memory for additional state sets"));
        return;
    }

    // remember handle to current state set
    pSSRec->dwHandle = dwParam;
    pSSRec->bCompressed = FALSE;

    // Get pointer to current recording state set
    pContext->pCurrSS = pSSRec;

    // Start recording mode
    pContext->bStateRecMode = TRUE;

    DBG_D3D((10,"Exiting __BeginStateSet"));
}

//-----------------------------------------------------------------------------
//
// void __EndStateSet
//
// stop recording states - revert to executing them.
//
//-----------------------------------------------------------------------------
void 
__EndStateSet(PERMEDIA_D3DCONTEXT* pContext)
{
    DWORD           dwHandle;
    P2StateSetRec   *pNewSSRec;

    DBG_D3D((10,"Entering __EndStateSet"));

    if (pContext->pCurrSS)
    {
        dwHandle = pContext->pCurrSS->dwHandle;

        // compress the current state set
        // Note: after being compressed the uncompressed version is free'd.
        pNewSSRec = CompressStateSet(pContext, pContext->pCurrSS);

        AddStateSetIndexTableEntry(pContext, dwHandle, pNewSSRec);
    }

    // No state set being currently recorded
    pContext->pCurrSS = NULL;

    // End recording mode
    pContext->bStateRecMode = FALSE;


    DBG_D3D((10,"Exiting __EndStateSet"));
}

//-----------------------------------------------------------------------------
//
// void __DeleteStateSet
//
// Delete the recorder state ste identified by dwParam
//
//-----------------------------------------------------------------------------
void 
__DeleteStateSet(PERMEDIA_D3DCONTEXT    *pContext, 
                 DWORD                  dwParam)
{

    P2StateSetRec   *pSSRec;

    DBG_D3D((10,"Entering __DeleteStateSet dwParam=%08lx",dwParam));
    if (pSSRec = FindStateSet(pContext, dwParam))
    {
        // Clear index table entry
        pContext->pIndexTableSS[dwParam - 1] = NULL;

        // Now delete the actual state set structure
        D3DFREE(pSSRec);
    }

    DBG_D3D((10,"Exiting __DeleteStateSet"));
}

//-----------------------------------------------------------------------------
//
// void __ExecuteStateSet
//
//
//-----------------------------------------------------------------------------
void 
__ExecuteStateSet(PERMEDIA_D3DCONTEXT* pContext,
                  DWORD                dwParam)
{
    P2StateSetRec   *pSSRec;
    DWORD           i;

    DBG_D3D((10,"Entering __ExecuteStateSet dwParam=%08lx",dwParam));
    if (pSSRec = FindStateSet(pContext, dwParam))
    {

        if (!pSSRec->bCompressed)
        {
            // uncompressed state set

            // Execute any necessary render states
            for (i=0; i< MAX_STATE; i++)
            {
                if (IS_FLAG_SET(pSSRec->u.uc.bStoredRS , i))
                {
                    DWORD dwRSType, dwRSVal;

                    dwRSType = i;
                    dwRSVal = pSSRec->u.uc.RenderStates[dwRSType];

                    // Store the state in the context
                    pContext->RenderStates[dwRSType] = dwRSVal;

                    DBG_D3D((6,"__ExecuteStateSet RS %x = %x",
                                dwRSType, dwRSVal));

                    // Process it
                    SetupP2RenderStates(pContext, dwRSType, dwRSVal);
                    // Mirror any change that happened in the render states into TSS 0
                    MapRenderStateToTextureStage0State(pContext, dwRSType, dwRSVal);

                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHABLEND;
                }
            }

            // Execute any necessary TSS's
            for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
            {
                if (IS_FLAG_SET(pSSRec->u.uc.bStoredTSS , i))
                {
                    DWORD dwTSState, dwValue;

                    dwTSState = i;
                    dwValue = pSSRec->u.uc.TssStates[dwTSState];

                    DBG_D3D((6,"__ExecuteStateSet TSS %x = %x",
                                dwTSState, dwValue));

                    // Store value associated to this stage state
                    pContext->TssStates[dwTSState] = dwValue;

                    // Perform any necessary preprocessing of it
                    PreProcessTextureStageState(pContext, 0, dwTSState, dwValue);

                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
                }
            }
            // Execute any necessary state for lights, materials, transforms,
            // viewport info, z range and clip planes - here -
        }
        else
        {
            // compressed state set

            // Execute any necessary render states
            for (i=0; i< pSSRec->u.cc.dwNumRS; i++)
            {
                DWORD dwRSType, dwRSVal;

                dwRSType = pSSRec->u.cc.pair[i].dwType;
                dwRSVal = pSSRec->u.cc.pair[i].dwValue;

                // Store the state in the context
                pContext->RenderStates[dwRSType] = dwRSVal;

                DBG_D3D((6,"__ExecuteStateSet RS %x = %x",
                            dwRSType, dwRSVal));

                // Process it
                SetupP2RenderStates(pContext, dwRSType, dwRSVal);
                // Mirror any change that happened in the render states into TSS 0
                MapRenderStateToTextureStage0State(pContext, dwRSType, dwRSVal);

                pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
                pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER;
                pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHABLEND;
            }

            // Execute any necessary TSS's
            for (; i< pSSRec->u.cc.dwNumTSS + pSSRec->u.cc.dwNumRS; i++)
            {
                DWORD dwTSState, dwValue;

                dwTSState = pSSRec->u.cc.pair[i].dwType;
                dwValue = pSSRec->u.cc.pair[i].dwValue;

                DBG_D3D((6,"__ExecuteStateSet TSS %x = %x",
                            dwTSState, dwValue));

                // Store value associated to this stage state
                pContext->TssStates[dwTSState] = dwValue;

                // Perform any necessary preprocessing of it
                PreProcessTextureStageState(pContext, 0, dwTSState, dwValue);

                pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
            }

            // Execute any necessary state for lights, materials, transforms,
            // viewport info, z range and clip planes - here -

        }
    }

    DBG_D3D((10,"Exiting __ExecuteStateSet"));
}

//-----------------------------------------------------------------------------
//
// void __CaptureStateSet
//
//
//-----------------------------------------------------------------------------
void 
__CaptureStateSet(PERMEDIA_D3DCONTEXT*  pContext, 
                  DWORD                 dwParam)
{
    P2StateSetRec   *pSSRec;
    DWORD           i;

    DBG_D3D((10,"Entering __CaptureStateSet dwParam=%08lx",dwParam));
    if (pSSRec = FindStateSet(pContext, dwParam))
    {
        if (!pSSRec->bCompressed)
        {
            // uncompressed state set

            // Capture any necessary render states
            for (i=0; i< MAX_STATE; i++)
                if (IS_FLAG_SET(pSSRec->u.uc.bStoredRS , i))
                {
                    pSSRec->u.uc.RenderStates[i] = pContext->RenderStates[i];
                }

            // Capture any necessary TSS's
            for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
                if (IS_FLAG_SET(pSSRec->u.uc.bStoredTSS , i))
                {
                    pSSRec->u.uc.TssStates[i] = pContext->TssStates[i];
                }

            // Capture any necessary state for lights, materials, transforms,
            // viewport info, z range and clip planes - here -
        }
        else
        {
            // compressed state set

            // Capture any necessary render states
            for (i=0; i< pSSRec->u.cc.dwNumRS; i++)
            {
                DWORD dwRSType;

                dwRSType = pSSRec->u.cc.pair[i].dwType;
                pSSRec->u.cc.pair[i].dwValue = pContext->RenderStates[dwRSType];

            }

            // Capture any necessary TSS's
            for (; i< pSSRec->u.cc.dwNumTSS + pSSRec->u.cc.dwNumRS; i++)
                {
                    DWORD dwTSState;

                    dwTSState = pSSRec->u.cc.pair[i].dwType;
                    pSSRec->u.cc.pair[i].dwValue = pContext->TssStates[dwTSState];
                }

            // Capture any necessary state for lights, materials, transforms,
            // viewport info, z range and clip planes - here -

        }
    }

    DBG_D3D((10,"Exiting __CaptureStateSet"));
}
#endif //D3D_STATEBLOCKS
