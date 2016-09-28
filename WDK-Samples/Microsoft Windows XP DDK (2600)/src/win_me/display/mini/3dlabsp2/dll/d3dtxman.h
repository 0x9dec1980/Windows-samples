/******************************Module*Header**********************************\
*
*                           *************************************
*                           * Permedia 2: Direct 3D SAMPLE CODE *
*                           *************************************
*
* Module Name: d3dtxman.h
*
* Content:  D3D Texture manager definitions and macros.
*
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights Reserved.
\*****************************************************************************/
#ifndef _DLL_D3DTXMAN_H_
#define _DLL_D3DTXMAN_H_

#define parent(k) (k / 2)
#define lchild(k) (k * 2)
#define rchild(k) (k * 2 + 1)

__inline ULONGLONG TextureCost(PPERMEDIA_D3DTEXTURE pTexture)
{
#ifdef _X86_
    ULONGLONG retval;
    _asm
    {
        mov     ebx, pTexture;
        mov     eax, [ebx]PERMEDIA_D3DTEXTURE.m_dwPriority;
        mov     ecx, eax;
        shr     eax, 1;
        mov     DWORD PTR retval + 4, eax;
        shl     ecx, 31;
        mov     eax, [ebx]PERMEDIA_D3DTEXTURE.m_dwTicks;
        shr     eax, 1;
        or      eax, ecx;
        mov     DWORD PTR retval, eax;
    }
    return retval;
#else
    return ((ULONGLONG)pTexture->m_dwPriority << 31) + 
            ((ULONGLONG)(pTexture->m_dwTicks >> 1));
#endif
}

typedef struct _TextureHeap 
{
    DWORD   m_next;
    DWORD   m_size;
    PPERMEDIA_D3DTEXTURE *m_data_p;
} TextureHeap, *PTextureHeap;

void TextureHeapDel(PTextureHeap,DWORD);
void TextureHeapUpdate(PTextureHeap,DWORD,DWORD,DWORD); 

typedef struct _TextureCacheManager 
{    
    TextureHeap m_heap;
    unsigned int tcm_ticks;
    D3DDEVINFO_TEXTUREMANAGER m_stats;
}TextureCacheManager, *PTextureCacheManager;

    //remove all HW handles and release surface
void TextureCacheManagerRemove(PTextureCacheManager,PPERMEDIA_D3DTEXTURE);  
    
HRESULT TextureCacheManagerAllocNode(PERMEDIA_D3DCONTEXT*,PPERMEDIA_D3DTEXTURE);
HRESULT TextureCacheManagerInitialize(PTextureCacheManager);
VOID    TextureCacheManagerRelease(PTextureCacheManager);
    
__inline void TextureCacheManagerRemoveFromHeap(
    PTextureCacheManager pTextureCacheManager,
    PPERMEDIA_D3DTEXTURE lpD3DTexI) 
{ 
    TextureHeapDel(&pTextureCacheManager->m_heap,lpD3DTexI->m_dwHeapIndex); 
}
__inline void TextureCacheManagerUpdatePriority(
    PTextureCacheManager pTextureCacheManager,
    PPERMEDIA_D3DTEXTURE lpD3DTexI) 
{ 
    TextureHeapUpdate(&pTextureCacheManager->m_heap,
        lpD3DTexI->m_dwHeapIndex, 
        lpD3DTexI->m_dwPriority, lpD3DTexI->m_dwTicks); 
}
__inline void TextureCacheManagerIncTotSz(
    PTextureCacheManager pTextureCacheManager,
    DWORD dwSize)
{
    ++pTextureCacheManager->m_stats.dwTotalManaged;
    pTextureCacheManager->m_stats.dwTotalBytes += dwSize;
}
__inline void TextureCacheManagerDecTotSz(
    PTextureCacheManager pTextureCacheManager,
    DWORD dwSize)
{
    --pTextureCacheManager->m_stats.dwTotalManaged;
    pTextureCacheManager->m_stats.dwTotalBytes -= dwSize;
}
__inline void TextureCacheManagerIncNumSetTexInVid(
    PTextureCacheManager pTextureCacheManager)
{
    ++pTextureCacheManager->m_stats.dwNumUsedTexInVid;
}
__inline void TextureCacheManagerIncNumTexturesSet(
    PTextureCacheManager pTextureCacheManager)
{
    ++pTextureCacheManager->m_stats.dwNumTexturesUsed;
}
__inline void TextureCacheManagerResetStatCounters(
    PTextureCacheManager pTextureCacheManager)
{
    pTextureCacheManager->m_stats.bThrashing = 0;
    pTextureCacheManager->m_stats.dwNumEvicts = 0;
    pTextureCacheManager->m_stats.dwNumVidCreates = 0;
    pTextureCacheManager->m_stats.dwNumUsedTexInVid = 0;
    pTextureCacheManager->m_stats.dwNumTexturesUsed = 0;
}
__inline void TextureCacheManagerGetStats(
    PERMEDIA_D3DCONTEXT *pContext,
    LPD3DDEVINFO_TEXTUREMANAGER stats)
{
    memcpy(stats, &pContext->pTextureManager->m_stats, 
        sizeof(D3DDEVINFO_TEXTUREMANAGER));
}
void TextureCacheManagerTimeStamp(PTextureCacheManager,PPERMEDIA_D3DTEXTURE);
    
#endif //#ifndef _DLL_D3DTXMAN_H_

