/******************************Module*Header**********************************\
 *
 *                           *************************************
 *                           * Permedia 2: Direct 3D SAMPLE CODE *
 *                           *************************************
 *
 * Module Name:  d3dresrc.c
 *
 *               Contains Resource management functions for Texture Handles and Palettes 
 *               refer to d3dtxman.c for managed textures
 * 
 *               Textures and Palettes are referenced in the DrawPrimitives2 command
 *               stream using handles. 
 *               A given Texture which is a type of Direct Draw surface is associated 
 *               with a Texture Handle via the CreateSurfaceEx callback. Since we do
 *               not have access to the LPDDRAWI_DDRAWSURFACE_LCL of the Texture Surface
 *               beyond this point, we need to cache all the information associated
 *               with the Texture and use it whenever we are referenced by the 
 *               corresponding handle in the DrawPrimitives2 command stream.
 *
 *               As for Palettes, the content of the Color Table associated with a 
 *               Palette Handle is informed via D3DDP2OP_UPDATEPALETTE token in the
 *               DrawPrimitives2 command stream. The D3DDP2OP_SETPALETTE token in the
 *               DrawPrimitives2 command stream associates a Palette (Handle) with a
 *               Texture (Handle).
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/


#include "precomp.h"

extern TextureCacheManager P2TextureManager;

//
//Local Functions
//


static VOID  
InitD3DTextureWithDDSurfInfo(PPERMEDIA_D3DTEXTURE       pTexture, 
                             LPDDRAWI_DDRAWSURFACE_LCL  lpSurf, 
                             LPP2THUNKEDDATA            ppdev);

static BOOL
SetTextureSlot(LPVOID                       pDDLcl,
               LPDDRAWI_DDRAWSURFACE_LCL    lpDDSLcl,
               PPERMEDIA_D3DTEXTURE         pTexture);

static PPERMEDIA_D3DTEXTURE
GetTextureSlot(LPVOID   pDDLcl, 
               DWORD    dwSurfaceHandle);

// Handles table
// each entry is a DWLIST structure (*dwSurfaceList,*dwPaletteList;pDDLcl)
static DWLIST  HandleList[MAX_DDRAWLCL_NUM] = {0}; 


/**
 *
 * Function:    GetSurfaceHandleList
 * Description: Get the handle list which is associated to a specific 
 *              PDD_DIRECTDRAW_LOCAL pDDLcl. It is called from  
 *              D3DContextCreate to get the handle list associated to the 
 *              pDDLcl with which the context is being created.
 */

LPDWLIST 
GetSurfaceHandleList(LPVOID pDDLcl)
{
    DWORD       i;
    LPDWLIST    lpSurfaceHandleList = NULL;

    DBG_D3D((10,"Entering GetSurfaceHandleList"));

    ASSERTDD(NULL != pDDLcl, "GetSurfaceHandleList get NULL==pDDLcl"); 
    for (i = 0; i < MAX_DDRAWLCL_NUM;i++)
    {
        if (HandleList[i].pDDLcl == pDDLcl)
        {
            DBG_D3D((4,"Getting pHandleList=%08lx for pDDLcl %x",
                &HandleList[i],pDDLcl));
            lpSurfaceHandleList = &HandleList[i];
            break;
        }
    }

    DBG_D3D((10,"Exiting GetSurfaceHandleList lpHandleList = 0x%x", lpSurfaceHandleList));

    return lpSurfaceHandleList;   //No surface handle available yet
} // GetSurfaceHandleList

/**
 * Function:    CreateSurfaceHandle
 * Description: allocate a new surface handle for Direct Draw Local
 *
 * return value
 *
 *      DD_OK   -- no error
 *      DDERR_OUTOFMEMORY -- allocation of texture handle failed
 *
 */

DWORD CreateSurfaceHandle(LPP2THUNKEDDATA            ppdev,
                          LPVOID                     pDDLcl,
                          LPDDRAWI_DDRAWSURFACE_LCL  lpDDSLcl)
{
    PPERMEDIA_D3DTEXTURE    pTexture;
    DWORD                   dwSurfaceHandle;
    PERMEDIA_D3DCONTEXT     *pCntxt;

    dwSurfaceHandle = lpDDSLcl->lpSurfMore->dwSurfaceHandle;
    ASSERTDD((0 != dwSurfaceHandle), "CreateSurfaceHandle (NULL == dwSurfaceHandle)"); 


    if ((0 == lpDDSLcl->lpGbl->fpVidMem) && 
        (DDSCAPS_SYSTEMMEMORY & lpDDSLcl->ddsCaps.dwCaps))
    {
        // this is a system memory destroy notification
        // so go ahead free the slot for this surface if we have it
        RemoveSurfaceHandle(pDDLcl, dwSurfaceHandle);
        return DD_OK;
    }

    pTexture = GetTextureSlot(pDDLcl,dwSurfaceHandle);
    if (NULL == pTexture)
    {
        pTexture =(PERMEDIA_D3DTEXTURE*)D3DMALLOCZ(sizeof(PERMEDIA_D3DTEXTURE));

        if (NULL != pTexture) 
        {
            if (!SetTextureSlot(pDDLcl,lpDDSLcl,pTexture))
            {
                D3DFREE(pTexture);
                return DDERR_OUTOFMEMORY;
            }
            DBG_D3D((0,"CreateSurfaceHandle: lpDDLcl=0x%x surfaceHandle=0x%x pTexture=0x%x ManagedTexture=0x%x",
                        pDDLcl, dwSurfaceHandle, pTexture, 
                        (lpDDSLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)));
        }
        else
        {
            DBG_D3D((0,"ERROR: Couldn't allocate Texture data mem"));
            return DDERR_OUTOFMEMORY;
        } 
    }

    InitD3DTextureWithDDSurfInfo(pTexture,lpDDSLcl,ppdev);

    if (pTexture->dwCaps & DDSCAPS_TEXTURE)
    {
        pCntxt = (PERMEDIA_D3DCONTEXT *)HandleList[pTexture->HandleListIndex].lpD3DContext;

        while (pCntxt != NULL)
        {
            if (pCntxt->CurrentTextureHandle == dwSurfaceHandle)
            {
                pCntxt->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
            }
            pCntxt = pCntxt->NextD3DContext;
        }
    }

    return DD_OK;
}//CreateSurfaceHandle

/**
 * Function:    RemoveSurfaceHandle
 * Description: Deletes the surfaceHandle from the given DirectDrawLocal's
 *              Surface Handle List.
 */

VOID
RemoveSurfaceHandle(LPVOID   lpDDLcl, 
                    DWORD    dwSurfaceHandle)
{
    PPERMEDIA_D3DTEXTURE    pTexture;

    if (dwSurfaceHandle)
    {
        if (pTexture = GetTextureSlot(lpDDLcl, dwSurfaceHandle))
        {
            if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
            {
                TextureCacheManagerRemove(&P2TextureManager,pTexture);
                D3DFREE(pTexture->fpVidMem);
            }
            DBG_D3D((0,"RemoveSurfaceHandle: lpDDLcl=0x%x surfaceHandle=0x%x ManagedTexture=0x%x",
                            lpDDLcl, dwSurfaceHandle, (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)));
            HandleList[pTexture->HandleListIndex].dwSurfaceList[dwSurfaceHandle]=0;
            D3DFREE(pTexture);
        }
    }
}//RemoveSurfaceHandle

/**
 * Function:    RemoveManagedSurface
 * Description: This is same as RemoveSurfaceHandle except that the
 *              dwSurfaceHandle is known to refer to a Managed Surface.
 *              Since this managed texture can be associated with any of the
 *              D3D Contexts corresponding to lpDDLcl we need
 *              to mark all D3D Contexts associated with lpDDLcl with 
 *              CONTEXT_DIRTY_TEXTURE
 */

VOID
RemoveManagedSurface(LPVOID   lpDDLcl, 
                     DWORD    dwSurfaceHandle)
{
    PPERMEDIA_D3DTEXTURE    pTexture;
    DWORD               i;
    PERMEDIA_D3DCONTEXT *pContext;

    if (dwSurfaceHandle)
    {
        if (pTexture = GetTextureSlot(lpDDLcl, dwSurfaceHandle))
        {
            DBG_D3D((0,"RemoveManagedSurface: lpDDLcl=0x%x surfaceHandle=0x%x ",
                            lpDDLcl, dwSurfaceHandle));
            TextureCacheManagerRemove(&P2TextureManager,pTexture);
        }
        for (i = 0; i < MAX_DDRAWLCL_NUM; i++)
        {
            if (HandleList[i].pDDLcl == lpDDLcl)
            {
                pContext = (PERMEDIA_D3DCONTEXT *)HandleList[i].lpD3DContext;
                while (pContext != NULL)
                {
                    pContext->dwDirtyFlags  |= CONTEXT_DIRTY_TEXTURE;
                    pContext                = pContext->NextD3DContext;
                }
                break;
            }
        }
    }
}//RemoveManagedSurface

/*
 * Function:    DirtyContextWithThisRenderTarget
 * Description: Traverses all the D3D contexts for the given lpDDLcl 
 *              and marks the ones with the RenderTarget handle
 *              equal to dwSurfaceHandle with CONTEXT_DIRTY_RENDERTARGET
 */
VOID
DirtyContextWithThisRenderTarget(LPVOID   lpDDLcl, 
                                 DWORD    dwSurfaceHandle)
{
    DWORD               i;
    PERMEDIA_D3DCONTEXT *pContext;

    for (i = 0; i < MAX_DDRAWLCL_NUM; i++)
    {
        if (HandleList[i].pDDLcl == lpDDLcl)
        {
            pContext = (PERMEDIA_D3DCONTEXT *)HandleList[i].lpD3DContext;
            while (pContext != NULL)
            {
                if (pContext->RenderSurfaceHandle == dwSurfaceHandle)
                {
                    pContext->dwDirtyFlags  |= CONTEXT_DIRTY_RENDERTARGET;
                }
                pContext                = pContext->NextD3DContext;
            }
            break;
        }
    }
}

/**
 * Function:    MarkManagedSurfaceNeedsUpdate
 * Description: Marks the Managed Surface as needing update if the 
 *              corresponding Video Memory part is already created.
 */

VOID
MarkManagedSurfaceNeedsUpdate(LPVOID   lpDDLcl, 
                              DWORD    dwSurfaceHandle)
{
    PPERMEDIA_D3DTEXTURE    pTexture;

    if (dwSurfaceHandle)
    {
        if (pTexture = GetTextureSlot(lpDDLcl, dwSurfaceHandle))
        {
            if (pTexture->P2ManagedTexture.fpVidMem != (FLATPTR)NULL)
                pTexture->P2ManagedTexture.dwFlags|= P2_SURFACE_NEEDUPDATE;
        }
    }
}//MarkManagedSurfaceNeedsUpdate

/**
 * Function:    LockManagedSurface
 * Description: This is called when a DirectDrawLock is called on a
 *              managed surface. We simply mark the surface as needing
 *              an update. Note that from Direct Draw's perspective
 *              the system memory copy of the surface is seen.
 *              Hence if any direct writes to this system memory copy occur,
 *              they get reflected when this texture is used next, via
 *              an update of the VideoMemory copy.
 */

BOOL
LockManagedSurface(LPVOID   lpDDLcl, 
                   DWORD    dwSurfaceHandle)
{
    PPERMEDIA_D3DTEXTURE        pTexture;

    if (dwSurfaceHandle)
    {
        if ((pTexture = GetTextureSlot(lpDDLcl, dwSurfaceHandle)) != NULL)
        {
            pTexture->P2ManagedTexture.dwFlags |= P2_SURFACE_NEEDUPDATE;
            return TRUE;
        }
    }
    return FALSE;
}//LockManagedSurface

/**
 *
 * Function:    DestroyAllSurfaceAndPaletteHandles
 *
 * Description: Free all the associated surface handle and palette 
 *              memory pools associated with a given DD local object.
 *
 */

static VOID 
DestroyAllSurfaceAndPaletteHandles(LPDWLIST lpHandleListEntry)
{
    LPVOID                  pPalette;
    DWORD                   handle;

    DBG_D3D((10,"Entering DestroyAllSurfaceAndPaletteHandles"));

    if (NULL != lpHandleListEntry->dwSurfaceList)
    {
        for (handle = 1; handle < PtrToUlong(lpHandleListEntry->dwSurfaceList[0]); handle++)
        {
            RemoveSurfaceHandle(lpHandleListEntry->pDDLcl, handle);
        }

        D3DFREE(lpHandleListEntry->dwSurfaceList);
        lpHandleListEntry->dwSurfaceList = NULL;
    }


    if (NULL != lpHandleListEntry->dwPaletteList)
    {
        DBG_D3D((4,"Releasing dwPaletteList %x for pDDLcl %x",
            lpHandleListEntry->dwPaletteList,lpHandleListEntry->pDDLcl));

        for (handle = 1; handle < PtrToUlong(lpHandleListEntry->dwPaletteList[0]); handle++)
        {
            pPalette = (LPVOID)lpHandleListEntry->dwPaletteList[handle];
            if (NULL != pPalette)
                D3DFREE(pPalette);
        }

        D3DFREE(lpHandleListEntry->dwPaletteList);
        lpHandleListEntry->dwPaletteList = NULL;
    }

    DBG_D3D((10,"Exiting DestroyAllSurfaceAndPaletteHandles"));
} // DestroyAllSurfaceAndPaletteHandles

/**
 *
 * Function:    RemoveDirectDrawLocal
 *
 * Description: Free all the associated surface handle and palette 
 *              memory pools associated with a given DD local object.
 *
 */

VOID
RemoveDirectDrawLocal(LPVOID lpDDLcl)
{
    DWORD                   i;

    ASSERTDD(NULL != lpDDLcl, "RemoveDirectDrawLocal get NULL==lpDDLcl"); 
    for (i = 0; i < MAX_DDRAWLCL_NUM; i++)
    {
        if (HandleList[i].pDDLcl == lpDDLcl)
        {
            DestroyAllSurfaceAndPaletteHandles(&HandleList[i]);
            HandleList[i].pDDLcl = NULL;
            break;
        }
    }
}//RemoveDirectDrawLocal

//=============================================================================
//
// In the new DX7 DDI we don't have the Texture Create/Destroy/Swap calls
// anymore, so now we need a mechanism for generating texture handles. This
// is done by the runtime, which will associate a surface handle for each 
// surface created with the DD local object, and will get our D3DCreateSurfaceEx
// callback called. 
//
// Since this creation can very well happen before we create a D3D context, we
// need to keep track of this association, and when we do get called to create
// a D3D context, we will now be given the relevant DD local object pointer to
// resolve which handles are ours (and to which private texture structures we
// need to use).
//
// This mechanism is also used to associate a palette to a texture
//
//=============================================================================

/**
 *
 * Function:    SetTextureSlot
 *
 * Description: In the handle list element corresponding to this local DD object, 
 *              store or update the pointer to the pTexture associated to the  
 *              surface handle from the lpDDSLcl surface.
 *
 */

static BOOL
SetTextureSlot(LPVOID                       pDDLcl,
               LPDDRAWI_DDRAWSURFACE_LCL    lpDDSLcl,
               PPERMEDIA_D3DTEXTURE         pTexture)
{
    int   i,j= -1;
    DWORD dwSurfaceHandle;

    DBG_D3D((4,"Entering SetTextureSlot"));

    ASSERTDD(NULL != pDDLcl && NULL != lpDDSLcl && NULL != pTexture,
                                    "SetTextureSlot invalid input");
    dwSurfaceHandle = lpDDSLcl->lpSurfMore->dwSurfaceHandle;

    // Find the handle list element associated with the local DD object,
    // if there's none then select an empty one to be used
    for (i = 0; i < MAX_DDRAWLCL_NUM;i++)
    {
        if (pDDLcl == HandleList[i].pDDLcl)
        {
            break;  // found the right slot
        }
        else
        if (0 == HandleList[i].pDDLcl && -1 == j)
        {
            j=i;    // first empty slot !
        }
    }

    // If we overrun the existing handle list elements, we need to
    // initialize an existing empty slot or return an error.
    if (i >= MAX_DDRAWLCL_NUM)
    {
        if (-1 != j)
        {
            //has an empty slot for this process, so use it
            i = j;  
            HandleList[j].pDDLcl = pDDLcl;
            ASSERTDD(NULL == HandleList[j].dwSurfaceList,"in SetTextureSlot");
        }
        else
        {
            //all process slots has been used, fail
            DBG_D3D((0,"SetTextureSlot failed with pDDLcl=%x "
                       "dwSurfaceHandle=%08lx pTexture=%x",
                       pDDLcl,dwSurfaceHandle,pTexture));
            return FALSE;
        }
    }

    ASSERTDD(i < MAX_DDRAWLCL_NUM, "in SetTextureSlot");

    if ( NULL == HandleList[i].dwSurfaceList ||
        dwSurfaceHandle >= PtrToUlong(HandleList[i].dwSurfaceList[0]))
    {
        // dwSurfaceHandle numbers are going to be ordinal numbers starting
        // at one, so we use this number to figure out a "good" size for
        // our new list.
        DWORD newsize = ((dwSurfaceHandle + LISTGROWSIZE) / LISTGROWSIZE)
                                                              * LISTGROWSIZE;
        PPERMEDIA_D3DTEXTURE *newlist= (PPERMEDIA_D3DTEXTURE *)
            D3DMALLOCZ((sizeof(PPERMEDIA_D3DTEXTURE)*newsize));
        DBG_D3D((4,"Growing pDDLcl=%x's SurfaceList[%x] size to %08lx",
                   pDDLcl,newlist,newsize));

        if (NULL == newlist)
        {
            DBG_D3D((0,"SetTextureSlot failed to increase "
                       "HandleList[%d].dwSurfaceList",i));
            return FALSE;
        }

        memset(newlist,0,newsize);

        // we had a formerly valid surfacehandle list, so we now must 
        // copy it over and free the memory allocated for it
        if (NULL != HandleList[i].dwSurfaceList)
        {
            memcpy(newlist,HandleList[i].dwSurfaceList,
                PtrToUlong(HandleList[i].dwSurfaceList[0]) * 
                sizeof(PPERMEDIA_D3DTEXTURE));
            D3DFREE(HandleList[i].dwSurfaceList);
            DBG_D3D((4,"Freeing pDDLcl=%x's old SurfaceList[%x]",
                       pDDLcl,HandleList[i].dwSurfaceList));
        }

        HandleList[i].dwSurfaceList = newlist;
         //store size in dwSurfaceList[0]
        *(DWORD*)HandleList[i].dwSurfaceList = newsize;
    }

    // Store a pointer to the pTexture associated to this surface handle
    HandleList[i].dwSurfaceList[dwSurfaceHandle] = pTexture;
    pTexture->HandleListIndex = i; //store index here to facilitate search
    DBG_D3D((4,"Set pDDLcl=%x Handle=%08lx pTexture = %x",
                pDDLcl, dwSurfaceHandle, pTexture));

    DBG_D3D((10,"Exiting SetTextureSlot"));

    return TRUE;
} // SetTextureSlot

/*
 *
 * Function:    GetTextureSlot
 * Description: Find the pointer to the PPERMEDIA_D3DTEXTURE associated to the 
 *              dwSurfaceHandle corresponding to the given local DD object
 *
 */

static PPERMEDIA_D3DTEXTURE
GetTextureSlot(LPVOID   pDDLcl, 
               DWORD    dwSurfaceHandle)
{
    DWORD                   i;
    PPERMEDIA_D3DTEXTURE    pTexture = NULL;

    DBG_D3D((10,"Entering GetTextureSlot"));

    if (dwSurfaceHandle)
    {
        for (i = 0; i < MAX_DDRAWLCL_NUM; i++)
        {
            if (HandleList[i].pDDLcl == pDDLcl)
            {
                if (HandleList[i].dwSurfaceList &&
                    PtrToUlong(HandleList[i].dwSurfaceList[0]) > dwSurfaceHandle )
                {
                    pTexture = HandleList[i].dwSurfaceList[dwSurfaceHandle];
                    break;
                }
                else
                    break;
            }
        }
    }
    DBG_D3D((10,"Exiting GetTextureSlot pTexture = 0x%x", pTexture));
    return pTexture;    //Not found
} // GetTextureSlot

/**
 *
 * Function:    AddContextToHandleList
 * Description: Add the Direct3D context to the DirectDrawLocal Handle List. 
 *
 */

VOID
AddContextToHandleList(PERMEDIA_D3DCONTEXT *pContext)
{
    if (pContext->pHandleList->lpD3DContext == NULL)
    {
        pContext->pHandleList->lpD3DContext = (LPVOID)pContext;
        pContext->NextD3DContext            = NULL;
    }
    else
    {
        pContext->NextD3DContext            = pContext->pHandleList->lpD3DContext;
        pContext->pHandleList->lpD3DContext = (LPVOID)pContext;
    }
}//AddContextToHandleList

/**
 *
 * Function:    DeleteContextFromHandleList
 * Description: Delete the Direct3D context from the DirectDrawLocal Handle List. 
 *
 */

VOID
DeleteContextFromHandleList(PERMEDIA_D3DCONTEXT *pContext)
{
    LPDWLIST            lpHandleList    = pContext->pHandleList;
    PERMEDIA_D3DCONTEXT *pCntxt         = (PERMEDIA_D3DCONTEXT *)lpHandleList->lpD3DContext;
    PERMEDIA_D3DCONTEXT *pNextCntxt     = (PERMEDIA_D3DCONTEXT *)pCntxt->NextD3DContext;

    if (pCntxt == pContext)
    {   // First context in the handle list
        lpHandleList->lpD3DContext = pNextCntxt;
        return;
    }
    while (pNextCntxt != pContext)
    {
        ASSERTD3D((pNextCntxt != NULL), "DeleteContextFromHandleList pNextCntxt == NULL");
        pCntxt      = pCntxt->NextD3DContext;
        pNextCntxt  = pNextCntxt->NextD3DContext;
    }
    pCntxt->NextD3DContext = pNextCntxt->NextD3DContext;
}//DeleteContextFromHandleList

/**
 * Function:    TextureHandleToPtr
 * Description: Find the texture associated to a given texture handle
 */

PPERMEDIA_D3DTEXTURE 
TextureHandleToPtr(UINT_PTR thandle, PERMEDIA_D3DCONTEXT* pContext)
{
    PPERMEDIA_D3DTEXTURE pTexture = NULL;

    DBG_D3D((4,"Entering TextureHandleToPtr handle=0x%x pContext=0x%x",thandle, pContext ));
    //  only a DX7 context can get here
    ASSERTDD((NULL != pContext->pHandleList), "pHandleList==NULL in TextureHandleToPtr");

    if (pContext->pHandleList->dwSurfaceList != NULL)
    {
        if ((PtrToUlong(pContext->pHandleList->dwSurfaceList[0]) > thandle) && 
            (0 != thandle))
        {
            pTexture = pContext->pHandleList->dwSurfaceList[(DWORD)thandle];
        }
    }

    DBG_D3D((4,"Leaving TextureHandleToPtr pTexture=0x%x",pTexture));
    return pTexture;               
} // TextureHandleToPtr



/**
 * Function:    InitD3DTextureWithDDSurfInfo
 * Description: Caches the texture information from DirectDrawSurfaceLocal
 *              so that it can be retrieved later with its texture handle.
 */

static VOID  
InitD3DTextureWithDDSurfInfo(PPERMEDIA_D3DTEXTURE           pTexture, 
                               LPDDRAWI_DDRAWSURFACE_LCL    lpSurf, 
                               LPP2THUNKEDDATA              ppdev)
{
    DBG_D3D((10,"Entering lpSurf=%08lx %08lx",lpSurf,lpSurf->lpGbl->fpVidMem));
    


    // Need to remember the sizes and the log of the sizes of the maps
    pTexture->fpVidMem                  = lpSurf->lpGbl->fpVidMem;
    pTexture->lPitch                    = lpSurf->lpGbl->lPitch;
    pTexture->wWidth                    = (WORD)(lpSurf->lpGbl->wWidth);
    pTexture->wHeight                   = (WORD)(lpSurf->lpGbl->wHeight);

    if (lpSurf->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        pTexture->ddpfSurface = lpSurf->lpGbl->ddpfSurface;
    }
    else
    {
        pTexture->ddpfSurface = ppdev->ddpfDisplay;
    }

    pTexture->m_dwBytes                 = pTexture->wHeight * pTexture->lPitch; 
    pTexture->dwFlags                   = lpSurf->dwFlags ; 
    pTexture->dwCaps                    = lpSurf->ddsCaps.dwCaps;
    pTexture->dwCaps2                   = lpSurf->lpSurfMore->ddsCapsEx.dwCaps2;
    pTexture->P2ManagedTexture.dwFlags  |= P2_SURFACE_NEEDUPDATE; 

    if (DDRAWISURF_HASCKEYSRCBLT & pTexture->dwFlags)
    {
         pTexture->dwKeyLow     = lpSurf->ddckCKSrcBlt.dwColorSpaceLowValue;
         pTexture->dwKeyHigh    = lpSurf->ddckCKSrcBlt.dwColorSpaceHighValue;
         DBG_D3D((4, "ColorKey exists (%08lx %08lx) on surface %d",
                                             pTexture->dwKeyLow,
                                             pTexture->dwKeyHigh,
                                             lpSurf->lpSurfMore->dwSurfaceHandle));
    }

    if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
        return;
#if D3D_MIPMAPPING
    // Verify if texture has mip maps atteched
    if (lpSurf->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        LPDDRAWI_DDRAWSURFACE_LCL lpNextSurf;
        int LOD;

        lpNextSurf = lpSurf;
        LOD = 0;

        pTexture->bMipMap = TRUE;

        // Calculate the number of mipmap levels (if this is a mipmap)
        pTexture->iMipLevels = (DWORD)((pTexture->wWidth > pTexture->wHeight) ?
                                                  log2((int)pTexture->wWidth) :
                                             log2((int)pTexture->wHeight)) + 1;

        // Walk the chain of surfaces and find all of the mipmap levels
        for (LOD = 0; LOD < pTexture->iMipLevels; LOD++)
        {
            DBG_D3D((4, "Loading texture LOD:%d, Ptr:0x%x",
                        LOD, lpNextSurf->lpGbl->fpVidMem));

            // Store the offsets for each of the mipmap levels
            StorePermediaLODLevel(ppdev, pTexture, lpNextSurf, LOD);

            // Is there another surface in the chain?
            if (lpNextSurf->lpAttachList)
            {
                lpNextSurf = lpNextSurf->lpAttachList->lpAttached;
                if (lpNextSurf == NULL)
                    break;
            }
            else 
                break;
        }

        // This isn't really a MipMap if LOD is 0
        if (LOD == 0)
        {
            DBG_D3D((4, "Texture was not a mipmap - only 1 level"));
            pTexture->bMipMap = FALSE;
            pTexture->iMipLevels = 1;
        }
        else
        {
            // Fill in the remaining levels with the smallest LOD
            // (this is for applications that haven't bothered to
            // pass us all of the LOD's).
            if (LOD < (pTexture->iMipLevels - 1))
            {
                int iLastLOD = LOD;

                DBG_D3D((4,"Filling in missing mipmaps!"));

                for (;LOD < MAX_MIP_LEVELS; LOD++)
                {
                    pTexture->MipLevels[LOD] = pTexture->MipLevels[iLastLOD];
                }
            }
        }
    }
    else 
#endif //D3D_MIPMAPPING
    {
        // NOT A MIPMAP, simply store away the offset of level 0
        pTexture->bMipMap = FALSE;
        pTexture->iMipLevels = 1;
        StorePermediaLODLevel(ppdev, pTexture, lpSurf, 0);
    }

    DBG_D3D((10,"Exiting InitD3DTextureWithDDSurfInfo"));
} // InitD3DTextureWithDDSurfInfo




/**
 * Function:    ClearManagedSurface
 * Description: Fills the rectangular area described by rDest of the 
 *              managed surface given by pDestLcl with dwColor.
 */

VOID 
ClearManagedSurface(LPDDRAWI_DDRAWSURFACE_LCL pDestLcl,
                    RECTL                     *rDest, 
                    DWORD                     dwColor)
{
    BYTE                    *pDestStart;
    LONG                    i;
    LONG                    lByteWidth  = rDest->right - rDest->left;
    LONG                    lHeight     = rDest->bottom - rDest->top;
    FLATPTR                 fpVidMem    = pDestLcl->lpGbl->fpVidMem;
    LONG                    lPitch      = pDestLcl->lpGbl->lPitch;
    PPERMEDIA_D3DTEXTURE    pTexture;

    DBG_DD(( 5, "DDraw: ClearManagedSurface"));

    // Calculate the start pointer for the dest
    pDestStart   = (BYTE*)(fpVidMem + (rDest->top * lPitch));
    // Clear depending on depth
    switch (DDSurf_BitDepth(pDestLcl)) 
    {
        case 8:
            pDestStart += rDest->left;
            while (--lHeight >= 0) 
            {
                for (i=0;i<lByteWidth;i++)
                    pDestStart[i]=(BYTE)dwColor;
                pDestStart += lPitch;
            }
        break;
            // fall through
        case 16:
            pDestStart += rDest->left*2;
            while (--lHeight >= 0) 
            {
                LPWORD  lpWord=(LPWORD)pDestStart;
                for (i=0;i<lByteWidth;i++)
                    lpWord[i]=(WORD)dwColor;
                pDestStart += lPitch;
            }
        break;

        case 32:
            pDestStart += rDest->left*4;
            while (--lHeight >= 0) 
            {
                LPDWORD lpDWord=(LPDWORD)pDestStart;
                for (i=0;i<lByteWidth;i++)
                    lpDWord[i]=(WORD)dwColor;
                pDestStart += lPitch;
            }
        break;
    }
    if ((pTexture = GetTextureSlot((LPVOID)pDestLcl->lpSurfMore->lpDD_lcl, 
                                   (DWORD)pDestLcl->lpSurfMore->dwSurfaceHandle)) != NULL)
    {
        pTexture->P2ManagedTexture.dwFlags |= P2_SURFACE_NEEDUPDATE;
    }
}

/**
 * Function:    PaletteHandleToPtr
 * Description: Given a palette handle, the pointer to the palette table is
 *              returned.
 */

PPERMEDIA_D3DPALETTE 
PaletteHandleToPtr(UINT_PTR phandle, PERMEDIA_D3DCONTEXT* pContext)
{
    PPERMEDIA_D3DPALETTE pPalette = NULL;

    DBG_D3D((4,"Entering PaletteHandleToPtr phandle=0x%x pContext=0x%x",phandle,pContext));

    ASSERTDD(NULL != pContext->pHandleList,
               "pHandleList==NULL in PaletteHandleToPtr");

    if ( (NULL != pContext->pHandleList->dwPaletteList) &&
         (PtrToUlong(pContext->pHandleList->dwPaletteList[0]) > phandle) &&
         (0 != phandle)
       )
    {
        pPalette = pContext->pHandleList->dwPaletteList[(DWORD)phandle];
    }

    DBG_D3D((4,"Leaving PaletteHandleToPtr pPalette=0x%x",pPalette));
    return pPalette;               
} // PaletteHandleToPtr

/**
 * Function:    __PaletteSet
 * Description: Associates a palette described by dwPaletteHandle with the 
 *              texture described by dwSurfaceHandle.
 */

HRESULT 
__PaletteSet(PERMEDIA_D3DCONTEXT    *pContext,
             DWORD                  dwSurfaceHandle,
             DWORD                  dwPaletteHandle,
             DWORD                  dwPaletteFlags)
{
    PERMEDIA_D3DTEXTURE * pTexture;

    ASSERTDD(0 != dwSurfaceHandle, "dwSurfaceHandle==0 in D3DDP2OP_SETPALETTE");

    DBG_D3D((8,"SETPALETTE %d to %d", dwPaletteHandle, dwSurfaceHandle));

    pTexture = TextureHandleToPtr(dwSurfaceHandle, pContext);

    if (NULL == pTexture)
    {
        DBG_D3D((0,"__PaletteSet:NULL==pTexture Palette=%08lx Surface=%08lx", 
            dwPaletteHandle, dwSurfaceHandle));
        return DDERR_INVALIDPARAMS;   // invalid dwSurfaceHandle, skip it
    }

    pTexture->dwPaletteHandle = dwPaletteHandle;
    // need to make it into private data if driver created this surface
    if (pContext->CurrentTextureHandle == dwSurfaceHandle) 
        pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;

    if (0 == dwPaletteHandle)
    {
        return D3D_OK;  //palette association is OFF
    }

    // Check if we need to grow our palette list for this handle element
    if (NULL == pContext->pHandleList->dwPaletteList ||
        dwPaletteHandle >= PtrToUlong(pContext->pHandleList->dwPaletteList[0]))
    {
        DWORD newsize = ((dwPaletteHandle + 
                                LISTGROWSIZE)/LISTGROWSIZE)*LISTGROWSIZE;
        PPERMEDIA_D3DPALETTE *newlist = (PPERMEDIA_D3DPALETTE *)
                  D3DMALLOCZ(sizeof(PPERMEDIA_D3DPALETTE)*newsize);

        DBG_D3D((8,"Growing pDDLcl=%x's "
                   "PaletteList[%x] size to %08lx",
                  pContext->pDDLcl, newlist, newsize));

        if (NULL == newlist)
        {
            DBG_D3D((0,"D3DDP2OP_SETPALETTE Out of memory."));
            return DDERR_OUTOFMEMORY;
        }

        memset(newlist,0,newsize);

        if (NULL != pContext->pHandleList->dwPaletteList)
        {
            memcpy(newlist,pContext->pHandleList->dwPaletteList,
                   PtrToUlong(pContext->pHandleList->dwPaletteList[0]) *
                         sizeof(PPERMEDIA_D3DPALETTE));
            D3DFREE(pContext->pHandleList->dwPaletteList);
            DBG_D3D((8,"Freeing pDDLcl=%x's old PaletteList[%x]",
                       pContext->pDDLcl,
                       pContext->pHandleList->dwPaletteList));
        }

        pContext->pHandleList->dwPaletteList = newlist;
         //store size in dwSurfaceList[0]
        *(DWORD*)pContext->pHandleList->dwPaletteList = newsize;
    }

    // If we don't have a palette hanging from this palette list
    // element we have to create one. The actual palette data will
    // come down in the D3DDP2OP_UPDATEPALETTE command token.
    if (NULL == pContext->pHandleList->dwPaletteList[dwPaletteHandle])
    {
        pContext->pHandleList->dwPaletteList[dwPaletteHandle] = 
            (PERMEDIA_D3DPALETTE*)D3DMALLOCZ(sizeof(PERMEDIA_D3DPALETTE));
        if (NULL == pContext->pHandleList->dwPaletteList[dwPaletteHandle])
        {
            DBG_D3D((0,"D3DDP2OP_SETPALETTE Out of memory."));
            return DDERR_OUTOFMEMORY;
        }
    }

    // driver may store this dwFlags to decide whether 
    // ALPHA exists in Palette
    pContext->pHandleList->dwPaletteList[dwPaletteHandle]->dwFlags =
                                                            dwPaletteFlags;

    return DD_OK;

} // PaletteSet

/**
 * Function:    __PaletteUpdate
 * Description: Updates the entries of a palette attached to a texture
 *              in the given context
 */

HRESULT 
__PaletteUpdate(PERMEDIA_D3DCONTEXT *pContext,
                DWORD               dwPaletteHandle, 
                WORD                wStartIndex, 
                WORD                wNumEntries,
                BYTE                *pPaletteData)

{
    PERMEDIA_D3DPALETTE* pPalette;

    DBG_D3D((8,"UPDATEPALETTE %d (%d,%d) %d",
              dwPaletteHandle,
              wStartIndex,
              wNumEntries,
              pContext->CurrentTextureHandle));

    pPalette = PaletteHandleToPtr(dwPaletteHandle,pContext);

    if (NULL != pPalette)
    {
        ASSERTDD(256 >= wStartIndex + wNumEntries,
                 "wStartIndex+wNumEntries>256 in D3DDP2OP_UPDATEPALETTE");

        // Copy the palette & associated data
        pPalette->wStartIndex = wStartIndex;
        pPalette->wNumEntries = wNumEntries;

        memcpy((LPVOID)&pPalette->ColorTable[wStartIndex],
               (LPVOID)pPaletteData,
               (DWORD)wNumEntries*sizeof(PALETTEENTRY));

        // If we are currently texturing and the texture is using the
        // palette we just updated, dirty the texture flag so that
        // it set up with the right (updated) palette
        if (pContext->CurrentTextureHandle)
        {
            PERMEDIA_D3DTEXTURE * pTexture=
                TextureHandleToPtr(pContext->CurrentTextureHandle,pContext);
            if (pTexture)
            {
                if (pTexture->dwPaletteHandle == dwPaletteHandle)
                {
                    pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;
                    DBG_D3D((8,"UPDATEPALETTE DIRTY_TEXTURE"));
                }
            }
        }
    }
    else
    {
        return DDERR_INVALIDPARAMS;
    }
    return DD_OK;

} // __PaletteUpdate

