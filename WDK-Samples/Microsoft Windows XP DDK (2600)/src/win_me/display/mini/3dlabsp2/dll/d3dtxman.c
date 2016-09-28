/******************************Module*Header**********************************\
*
*                           *************************
*                           * Direct 3D SAMPLE CODE *
*                           *************************
*
* Module Name: d3dtxman.c
*
* Content:  D3D Texture manager
*
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights Reserved.
\*****************************************************************************/
#include "precomp.h"

static DWORD   P2TMcount = 0;
static ULONG ulVidMemAllocate(LPP2THUNKEDDATA    ppdev,
                              DWORD              dwWidth,
                              DWORD              dwHeight,
                              DWORD              dwRGBBitCount,                 
                              LPVIDMEM           pvmHeap,
                              LPVIDMEM           *ppvmHeap);

static void TextureHeapHeapify(PTextureHeap,DWORD);
static BOOL TextureHeapAdd(PTextureHeap,PPERMEDIA_D3DTEXTURE);

static PPERMEDIA_D3DTEXTURE TextureHeapExtractMin(PTextureHeap);
static PPERMEDIA_D3DTEXTURE TextureHeapExtractMax(PTextureHeap pTextureHeap);

// Free the LRU texture 
static BOOL TextureCacheManagerFreeTextures(PTextureCacheManager,DWORD, DWORD);
static void TextureCacheManagerEvictTextures(PTextureCacheManager);

//-----------------------------------------------------------------------------
//
// void TextureHeapHeapify
//
//-----------------------------------------------------------------------------
static void TextureHeapHeapify(PTextureHeap pTextureHeap, 
                               DWORD        k)
{
    while(TRUE) 
    {
        DWORD smallest;
        DWORD l = lchild(k);
        DWORD r = rchild(k);
        if(l < pTextureHeap->m_next)
            if(TextureCost(pTextureHeap->m_data_p[l]) <
                             TextureCost(pTextureHeap->m_data_p[k]))
                smallest = l;
            else
                smallest = k;
        else
            smallest = k;
        if(r < pTextureHeap->m_next)
            if(TextureCost(pTextureHeap->m_data_p[r]) <
                             TextureCost(pTextureHeap->m_data_p[smallest]))
                smallest = r;
        if(smallest != k) 
        {
            PPERMEDIA_D3DTEXTURE t = pTextureHeap->m_data_p[k];
            pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[smallest];
            pTextureHeap->m_data_p[k]->m_dwHeapIndex = k;
            pTextureHeap->m_data_p[smallest] = t;
            t->m_dwHeapIndex = smallest;
            k = smallest;
        }
        else
            break;
    }
}

//-----------------------------------------------------------------------------
//
// bool TextureHeapAdd
//
//-----------------------------------------------------------------------------
static BOOL TextureHeapAdd(PTextureHeap         pTextureHeap, 
                           PPERMEDIA_D3DTEXTURE lpD3DTexI)
{
    PPERMEDIA_D3DTEXTURE    *p;
    ULONGLONG               Cost;
    DWORD                   k;

    if(pTextureHeap->m_next == pTextureHeap->m_size) 
    {
        pTextureHeap->m_size = pTextureHeap->m_size * 2 - 1;
        p = (PPERMEDIA_D3DTEXTURE *)
                D3DSHAREDALLOC(sizeof(PPERMEDIA_D3DTEXTURE)*pTextureHeap->m_size);

        if(p == 0)
        {
            DBG_D3D((0,"Failed to allocate memory to grow heap."));
            pTextureHeap->m_size = (pTextureHeap->m_size + 1) / 2; // restore size
            return FALSE;
        }
        memcpy( p + 1, pTextureHeap->m_data_p + 1, 
                (sizeof(PPERMEDIA_D3DTEXTURE) * (pTextureHeap->m_next - 1)));
        D3DSHAREDFREE( pTextureHeap->m_data_p);
        pTextureHeap->m_data_p = p;
    }
    Cost = TextureCost(lpD3DTexI);
    for(k = pTextureHeap->m_next; k > 1; k = parent(k))
        if(Cost < TextureCost(pTextureHeap->m_data_p[parent(k)])) 
        {
            pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[parent(k)];
            pTextureHeap->m_data_p[k]->m_dwHeapIndex = k;
        }
        else
            break;
    pTextureHeap->m_data_p[k] = lpD3DTexI;
    lpD3DTexI->m_dwHeapIndex = k;
    ++pTextureHeap->m_next;
    return TRUE;
}

//-----------------------------------------------------------------------------
//
// PPERMEDIA_D3DTEXTURE TextureHeapExtractMin
//
//-----------------------------------------------------------------------------
static PPERMEDIA_D3DTEXTURE TextureHeapExtractMin(PTextureHeap pTextureHeap)
{
    PPERMEDIA_D3DTEXTURE lpD3DTexI = pTextureHeap->m_data_p[1];
    --pTextureHeap->m_next;
    pTextureHeap->m_data_p[1] = pTextureHeap->m_data_p[pTextureHeap->m_next];
    pTextureHeap->m_data_p[1]->m_dwHeapIndex = 1;
    TextureHeapHeapify(pTextureHeap,1);
    lpD3DTexI->m_dwHeapIndex = 0;
    return lpD3DTexI;
}

//-----------------------------------------------------------------------------
//
// PPERMEDIA_D3DTEXTURE TextureHeapExtractMax
//
//-----------------------------------------------------------------------------
static PPERMEDIA_D3DTEXTURE TextureHeapExtractMax(PTextureHeap pTextureHeap)
{
    // When extracting the max element from the heap, we don't need to
    // search the entire heap, but just the leafnodes. This is because
    // it is guaranteed that parent nodes are cheaper than the leaf nodes
    // so once you have looked through the leaves, you won't find anything
    // cheaper. 
    // NOTE: (lchild(i) >= m_next) is true only for leaf nodes.
    // ALSO NOTE: You cannot have a rchild without a lchild, so simply
    //            checking for lchild is sufficient.
    unsigned                max = pTextureHeap->m_next - 1;
    ULONGLONG               maxcost = 0;
    unsigned                i;
    ULONGLONG               Cost;
    PPERMEDIA_D3DTEXTURE    lpD3DTexI;

    for( i = max; lchild(i) >= pTextureHeap->m_next; --i)
    {
        Cost = TextureCost(pTextureHeap->m_data_p[i]);
        if(maxcost < Cost)
        {
            maxcost = Cost;
            max = i;
        }
    }
    lpD3DTexI = pTextureHeap->m_data_p[max];
    TextureHeapDel(pTextureHeap,max);
    return lpD3DTexI;
}

//-----------------------------------------------------------------------------
//
// void TextureHeapDel
//
//-----------------------------------------------------------------------------
void TextureHeapDel(PTextureHeap    pTextureHeap, 
                    DWORD           k)
{
    ULONGLONG               Cost;
    PPERMEDIA_D3DTEXTURE    lpD3DTexI = pTextureHeap->m_data_p[k];

    --pTextureHeap->m_next;
    Cost = TextureCost(pTextureHeap->m_data_p[pTextureHeap->m_next]);
    if(Cost < TextureCost(lpD3DTexI))
    {
        while(k > 1)
        {
            if(Cost < TextureCost(pTextureHeap->m_data_p[parent(k)]))
            {
                pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[parent(k)];
                pTextureHeap->m_data_p[k]->m_dwHeapIndex = k;
            }
            else
                break;
            k = parent(k);
        }
        pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[pTextureHeap->m_next];
        pTextureHeap->m_data_p[k]->m_dwHeapIndex = k;
    }
    else
    {
        pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[pTextureHeap->m_next];
        pTextureHeap->m_data_p[k]->m_dwHeapIndex = k;
        TextureHeapHeapify(pTextureHeap,k);
    }
    lpD3DTexI->m_dwHeapIndex = 0;
}

//-----------------------------------------------------------------------------
//
// void TextureHeapUpdate
//
//-----------------------------------------------------------------------------
void TextureHeapUpdate(PTextureHeap pTextureHeap, 
                       DWORD        k,
                       DWORD        priority, 
                       DWORD        ticks) 
{
    PPERMEDIA_D3DTEXTURE lpD3DTexI = pTextureHeap->m_data_p[k];
    ULONGLONG Cost;
#ifdef _X86_
    _asm
    {
        mov     edx, 0;
        shl     edx, 31;
        mov     eax, priority;
        mov     ecx, eax;
        shr     eax, 1;
        or      edx, eax;
        mov     DWORD PTR Cost + 4, edx;
        shl     ecx, 31;
        mov     eax, ticks;
        shr     eax, 1;
        or      eax, ecx;
        mov     DWORD PTR Cost, eax;
    }
#else
    Cost = ((ULONGLONG)priority << 31) + ((ULONGLONG)(ticks >> 1));
#endif
    if(Cost < TextureCost(lpD3DTexI))
    {
        while(k > 1)
        {
            if(Cost < TextureCost(pTextureHeap->m_data_p[parent(k)]))
            {
                pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[parent(k)];
                pTextureHeap->m_data_p[k]->m_dwHeapIndex = k;
            }
            else
                break;
            k = parent(k);
        }
        lpD3DTexI->m_dwPriority = priority;
        lpD3DTexI->m_dwTicks = ticks;
        lpD3DTexI->m_dwHeapIndex = k;
        pTextureHeap->m_data_p[k] = lpD3DTexI;
    }
    else
    {
        lpD3DTexI->m_dwPriority = priority;
        lpD3DTexI->m_dwTicks = ticks;
        TextureHeapHeapify(pTextureHeap,k);
    }
}

//-----------------------------------------------------------------------------
//
// HRESULT TextureCacheManagerInitialize
//
//-----------------------------------------------------------------------------
HRESULT TextureCacheManagerInitialize(PTextureCacheManager pTextureCacheManager)
{
    if (P2TMcount == 0)
    {
        pTextureCacheManager->tcm_ticks = 0;
        pTextureCacheManager->m_heap.m_next = 1;
        pTextureCacheManager->m_heap.m_size = 1024;
        pTextureCacheManager->m_heap.m_data_p = (PPERMEDIA_D3DTEXTURE *)
            D3DSHAREDALLOC((sizeof(PPERMEDIA_D3DTEXTURE))*(pTextureCacheManager->m_heap.m_size));

        if(pTextureCacheManager->m_heap.m_data_p == 0)
        {
            DBG_D3D((0,"Failed to allocate texture heap."));
            return E_OUTOFMEMORY;
        }
        memset(pTextureCacheManager->m_heap.m_data_p, 0, 
            sizeof(PPERMEDIA_D3DTEXTURE) * pTextureCacheManager->m_heap.m_size);
        P2TMcount++;
    }
    else
    {
        P2TMcount++;
    }
    return D3D_OK;
}

//-----------------------------------------------------------------------------
//
// VOID  TextureCacheManagerRelease
//
//-----------------------------------------------------------------------------
VOID TextureCacheManagerRelease(PTextureCacheManager pTextureCacheManager)
{
    P2TMcount--;

    if (0 == P2TMcount)
    {
        if (0 != pTextureCacheManager->m_heap.m_data_p)
        {
            TextureCacheManagerEvictTextures(pTextureCacheManager);
            D3DSHAREDFREE(pTextureCacheManager->m_heap.m_data_p);
            pTextureCacheManager->m_heap.m_data_p=NULL;
        }
    }
}

//-----------------------------------------------------------------------------
//
// BOOL TextureCacheManagerFreeTextures
//
//-----------------------------------------------------------------------------
static BOOL TextureCacheManagerFreeTextures(PTextureCacheManager    pTextureCacheManager,
                                            DWORD                   dwStage, 
                                            DWORD                   dwBytes)
{
    PPERMEDIA_D3DTEXTURE rc;
    unsigned             i;

    if(pTextureCacheManager->m_heap.m_next <= 1)
        return FALSE;
    for(i = 0; 
        pTextureCacheManager->m_heap.m_next > 1 && i < dwBytes; 
        i += rc->m_dwBytes)
    {
        // Find the LRU texture and remove it.
        rc = TextureHeapExtractMin(&pTextureCacheManager->m_heap);
        TextureCacheManagerRemove(pTextureCacheManager,rc);
        pTextureCacheManager->m_stats.dwLastPri = rc->m_dwPriority;
        ++pTextureCacheManager->m_stats.dwNumEvicts;
        DBG_D3D((2, "Removed texture with timestamp %u,%u (current = %u).", 
            rc->m_dwPriority, rc->m_dwTicks, pTextureCacheManager->tcm_ticks));
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
//
// HRESULT TextureCacheManagerAllocNode
//
//-----------------------------------------------------------------------------
HRESULT TextureCacheManagerAllocNode(PERMEDIA_D3DCONTEXT* pContext,
                                     PPERMEDIA_D3DTEXTURE pTexture)
{
    DWORD                   trycount                = 0;
    DWORD                   bytecount               = pTexture->m_dwBytes;
    PP2_MANAGED_TEXTURE     pManagedTexture         = &pTexture->P2ManagedTexture;
    PTextureCacheManager    pTextureCacheManager    = pContext->pTextureManager;
    LPDDRAWI_DIRECTDRAW_LCL lpDirectDrawLocal       = (LPDDRAWI_DIRECTDRAW_LCL)pContext->pDDLcl;

    // Attempt to allocate a texture.
    while((FLATPTR)NULL == pManagedTexture->fpVidMem)
    {
        ++trycount;
        pManagedTexture->fpVidMem=(FLATPTR)ulVidMemAllocate(pContext->pThisDisplay,
                                                            pTexture->wWidth,
                                                            pTexture->wHeight,
                                                            pTexture->ddpfSurface.dwRGBBitCount,
                                                            lpDirectDrawLocal->lpGbl->vmiData.pvmList,
                                                            &pManagedTexture->pvmHeap);

        DBG_D3D((8,"Got fpVidMem=%08lx",pManagedTexture->fpVidMem));
        if ((FLATPTR)NULL != pManagedTexture->fpVidMem)
        {   // No problem, there is enough memory. 
            pTexture->m_dwTicks = pTextureCacheManager->tcm_ticks;
            if(!TextureHeapAdd(&pTextureCacheManager->m_heap,pTexture))
            {          
                VidMemFree( pManagedTexture->pvmHeap->lpHeap,
                            pManagedTexture->fpVidMem);
                pManagedTexture->fpVidMem=(FLATPTR)NULL;
                DBG_D3D((0,"Out of memory"));
                return DDERR_OUTOFMEMORY;
            }
            pManagedTexture->dwFlags |= P2_SURFACE_NEEDUPDATE;
            break;
        }
        else
        {
            if (!TextureCacheManagerFreeTextures(
                pTextureCacheManager,0, bytecount))
            {
                DBG_D3D((0,"all Freed no further video memory available"));
                return DDERR_OUTOFVIDEOMEMORY;  //nothing left
            }
            bytecount <<= 1;
        }
    }
    if(trycount > 1)
    {
        DBG_D3D((8, "Allocated texture after %u tries.", trycount));
    }
    TextureCacheManagerIncTotSz(pTextureCacheManager,
        pTexture->m_dwBytes);
    ++pTextureCacheManager->m_stats.dwWorkingSet;
    pTextureCacheManager->m_stats.dwWorkingSetBytes 
        += (pTexture->m_dwBytes);
    ++pTextureCacheManager->m_stats.dwNumVidCreates;
    return D3D_OK;
}

//-----------------------------------------------------------------------------
//
// void TextureCacheManagerRemove
//
// remove all HW handles and release surface
//
//-----------------------------------------------------------------------------
void TextureCacheManagerRemove(PTextureCacheManager pTextureCacheManager,
                               PPERMEDIA_D3DTEXTURE pTexture)
{
    PP2_MANAGED_TEXTURE     pManagedTexture = &pTexture->P2ManagedTexture;

    if (pManagedTexture->fpVidMem)
    {
        VidMemFree(pManagedTexture->pvmHeap->lpHeap,pManagedTexture->fpVidMem);
        pManagedTexture->fpVidMem=(FLATPTR)NULL;
        TextureCacheManagerDecTotSz(pTextureCacheManager,
            pTexture->m_dwBytes);
        --pTextureCacheManager->m_stats.dwWorkingSet;
        pTextureCacheManager->m_stats.dwWorkingSetBytes -= 
            (pTexture->m_dwBytes);
    }

    if (pTexture->m_dwHeapIndex && pTextureCacheManager->m_heap.m_data_p)
        TextureHeapDel(&pTextureCacheManager->m_heap,
        pTexture->m_dwHeapIndex); 
}

//-----------------------------------------------------------------------------
//
// void TextureCacheManagerEvictTextures
//
//-----------------------------------------------------------------------------
static void TextureCacheManagerEvictTextures(PTextureCacheManager pTextureCacheManager)
{
    while(pTextureCacheManager->m_heap.m_next > 1)
    {
        PPERMEDIA_D3DTEXTURE lpD3DTexI = 
            TextureHeapExtractMin(&pTextureCacheManager->m_heap);
        TextureCacheManagerRemove(pTextureCacheManager,lpD3DTexI);
    }
    pTextureCacheManager->tcm_ticks = 0;
}

//-----------------------------------------------------------------------------
//
// void TextureCacheManagerTimeStamp
//
//-----------------------------------------------------------------------------
void TextureCacheManagerTimeStamp(PTextureCacheManager pTextureCacheManager,
                                  PPERMEDIA_D3DTEXTURE lpD3DTexI)
{
    TextureHeapUpdate(&pTextureCacheManager->m_heap,
        lpD3DTexI->m_dwHeapIndex, lpD3DTexI->m_dwPriority, 
        pTextureCacheManager->tcm_ticks);
    pTextureCacheManager->tcm_ticks += 2;
}

//---------------------------------------------------------------------------
//
// ULONG ulVidMemAllocate
//
// This function allocates "lWidth" by "lHeight" bytes of video memory
//
// Parameters:
//  ppdev----------PPDEV
//  lWidth---------Width of the memory to allocate
//  lHeight--------Height of the memory to allocate
//  dwRGBBitCount--
//  ppvmHeap-------Pointer to a video memory heap, local or non-local etc.
//
//--------------------------------------------------------------------------
static ULONG
ulVidMemAllocate(LPP2THUNKEDDATA    ppdev,
                 DWORD              dwWidth,
                 DWORD              dwHeight,
                 DWORD              dwRGBBitCount,
                 LPVIDMEM           pvmHeap,
                 LPVIDMEM           *ppvmHeap)
{
    ULONG               iHeap;
    ULONG               ulByteOffset;
    DWORD               dwDelta;
    LONG                lNewDelta;
    SURFACEALIGNMENT    alignment;  // DDRAW heap management allignment stru

    *ppvmHeap = NULL;

    memset(&alignment, 0, sizeof(alignment));

    //
    // Calculate lDelta and partical products based on lWidth
    // The permedia has surface width restrictions that must be met
    //
    dwDelta = (PITCH_IN_PIXELS(dwWidth) << (dwRGBBitCount >> 4));
    //
    // Set alignment requirements
    //   - must start at an pixel address
    //   - pitch needs to be lDelta
    //

    alignment.Linear.dwStartAlignment = 4; //always force 32bit alignment
    alignment.Linear.dwPitchAlignment = dwDelta;

    //
    // Loop through all the heaps to find available memory
    // Note: This ppdev->cHeap info was set in DrvGetDirectDrawInfo
    // when the driver is initialized
    //
    for ( iHeap = 0; iHeap < ppdev->dwNumHeaps; iHeap++ )
    {

        //
        // Since we are using DDRAW run time heap management code. It is
        // possible that the heap hasn't been initialized. For example, if
        // we fail in DrvEnableDirectDraw(), then the system won't initialize
        // the heap for us
        //
        if ( pvmHeap == NULL )
        {
            DBG_D3D((1, "Video memory hasn't been initialzied"));
            return 0;
        }

        //
        // AGP memory could be potentially used for device-bitmaps, with
        // two very large caveats:
        //
        // 1. No kernel-mode view is made for the AGP memory (would take
        //    up too many PTEs and too much virtual address space).
        //    No user-mode view is made either unless a DirectDraw
        //    application happens to be running.  Consequently, neither
        //    GDI nor the driver can use the CPU to directly access the
        //    bits.  (It can be done through the accelerator, however.)
        //
        // 2. AGP heaps never shrink their committed allocations.  The
        //    only time AGP memory gets de-committed is when the entire
        //    heap is empty.  And don't forget that committed AGP memory
        //    is non-pageable.  Consequently, if you were to enable a
        //    50 MB AGP heap for DirectDraw, and were sharing that heap
        //    for device bitmap allocations, after running a D3D game
        //    the system would never be able to free that 50 MB of non-
        //    pageable memory until every single device bitmap was deleted!
        //    Just watch your Winstone scores plummet if someone plays
        //    a D3D game first.
        //
        if ( !(pvmHeap->dwFlags & VIDMEM_ISNONLOCAL) )
        {
            //
            // Ask DDRAW heap management to allocate memory for us
            //
            ulByteOffset = (ULONG)HeapVidMemAllocAligned(pvmHeap,
                                                         dwDelta,
                                                         dwHeight,
                                                         &alignment,
                                                         &lNewDelta);
            

            if ( ulByteOffset != 0 )
            {
                ASSERTDD((dwDelta == (DWORD)lNewDelta),
                         "ulVidMemAllocate: deltas don't match");

                *ppvmHeap    = pvmHeap;

                //
                // We are done
                //
                return (ulByteOffset);
            }// if ( pdsurf != NULL )
        }// if (!(pvmHeap->dwFlags & VIDMEM_ISNONLOCAL))
        pvmHeap++;
    }// loop through all the heaps to see if we can find available memory

    return 0;
}// ulVidMemAllocate()