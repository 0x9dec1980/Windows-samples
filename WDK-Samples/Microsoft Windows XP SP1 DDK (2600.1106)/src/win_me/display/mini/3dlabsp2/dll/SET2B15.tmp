/******************************Module*Header**********************************\
*
*       ****************************************
*       * Permedia 2: Direct Draw  SAMPLE CODE *
*       ****************************************
*
* Module Name: dd.h
*
*       Contains Prototypes for Direct Draw related functions
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef _DLL_DD_H_
#define _DLL_DD_H_


#define DDSurf_BitDepth(lpLcl) \
    ( (lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? \
      (lpLcl->lpGbl->ddpfSurface.dwRGBBitCount) : \
      (lpLcl->lpGbl->lpDD->vmiData.ddpfDisplay.dwRGBBitCount) \
    )

#define DDSurf_AlphaBitDepth(lpLcl) \
    ( (lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? \
      (lpLcl->lpGbl->ddpfSurface.dwAlphaBitDepth) : \
      (lpLcl->lpGbl->lpDD->vmiData.ddpfDisplay.dwAlphaBitDepth) \
    )

#define DDSurf_RGBAlphaBitMask(lpLcl) \
    ( (lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? \
      (lpLcl->lpGbl->ddpfSurface.dwRGBAlphaBitMask) : \
      (lpLcl->lpGbl->lpDD->vmiData.ddpfDisplay.dwRGBAlphaBitMask) \
    )

#define DDSurf_PixelFormat(lpLcl)   \
    ( (lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? \
      (&lpLcl->lpGbl->ddpfSurface):                  \
      (&lpLcl->lpGbl->lpDD->vmiData.ddpfDisplay)     \
    )

_inline DWORD GetRenderDirection(FLATPTR    fpSrcVidMem, 
                                 FLATPTR    fpDestVidMem,
                                 RECTL      *rSrc, 
                                 RECTL      *rDst)
{
    if (fpDestVidMem == fpSrcVidMem)
    {
        if (rSrc->top < rDst->top)
        {
            return 0;
        }
        else if (rSrc->top > rDst->top)
        {
            return 1;
        }
        else if(rSrc->left < rDst->left)
        {
            return 0;
        }
        else 
            return 1;
            
    }
    return  1;
}

_inline BOOL GetXMirror(LPDDHAL_BLTDATA    lpBlt, 
                        DWORD              dwRenderDirection) 
{
    if (lpBlt != NULL)
    {
        if(lpBlt->dwFlags & DDBLT_DDFX)
        {
            return (lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT);
        } 
    }
    else
    {
        if (dwRenderDirection==0)
        {
            return (TRUE);
        }
    }
    return FALSE;
}

_inline BOOL GetYMirror(LPDDHAL_BLTDATA    lpBlt, 
                        DWORD              dwRenderDirection) 
{
    if (lpBlt != NULL)
    {
        if(lpBlt->dwFlags & DDBLT_DDFX)
        {
            return (lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN);
        } 
    }
    else
    {
        if (dwRenderDirection==0)
        {
            return (TRUE);
        }
    }
    return FALSE;
}

#ifdef DDFLIP_INTERVALN

#define INVALID_VBINTERRUPT_COUNT     0xFFFFFFFF
#define DDFLIP_INTERVAL1              0x01000000L

#define FLIP_INTERVAL_MASK          (DDFLIP_INTERVAL1|DDFLIP_INTERVAL2|DDFLIP_INTERVAL3|DDFLIP_INTERVAL4)

#endif //#ifdef DDFLIP_INTERVALN

/*
 * definitions in ddutil.c
 */
BOOL 
bCheckTextureFormat(LPDDPIXELFORMAT lpddpf);
BOOL 
bComparePixelFormat(LPDDPIXELFORMAT lpddpf1, 
                    LPDDPIXELFORMAT lpddpf2);
/*
 * DirectDraw Hal callback functions implemented in this driver
 * definitions in ddhalcb.c
 */
extern DWORD CALLBACK DdCanCreateSurface( LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurfaceData);
extern DWORD CALLBACK DdCreateSurface( LPDDHAL_CREATESURFACEDATA lpCreateSurfaceData);
extern DWORD CALLBACK DdWaitForVerticalBlank( LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank);
extern DWORD CALLBACK DdGetScanLine( LPDDHAL_GETSCANLINEDATA lpGetScanLine);
extern DWORD CALLBACK DdFlipToGDISurface(LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurface);
extern DWORD CALLBACK DdUpdateNonLocalHeap(LPDDHAL_UPDATENONLOCALHEAPDATA plhd);
/*
 * DirectDraw Surface callback functions implemented in this driver
 * definitions in ddsurfcb.c
 */
extern DWORD CALLBACK DdDestroySurface( LPDDHAL_DESTROYSURFACEDATA psdd);
extern DWORD CALLBACK DdFlip(LPDDHAL_FLIPDATA lpFlipData);
extern DWORD CALLBACK DdLock(LPDDHAL_LOCKDATA lpLockData);
extern DWORD CALLBACK DdGetBltStatus(LPDDHAL_GETBLTSTATUSDATA lpGetBltStatusData);
extern DWORD CALLBACK DdGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatusData);
extern DWORD CALLBACK DdBlt(LPDDHAL_BLTDATA lpBltData);
extern DWORD CALLBACK DdSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKeyData);

extern DWORD CALLBACK DdGetDriverInfo(LPDDHAL_GETDRIVERINFODATA);

/*
 * Permedia2 support functions for Direct Draw Initialization
 * definition in dllmain.c
 */
VOID InitDDHAL(LPP2THUNKEDDATA pThisDisplay);


/*
 * Low level Rendering functions
 * definitions in p2ddblt.c
 */
VOID 
SysMemToSysMemSurfaceCopy(LPP2THUNKEDDATA   pThisDisplay, 
                          FLATPTR           fpSrcVidMem,
                          DWORD             dwSrcPitch,
                          DWORD             dwSrcBitDepth,
                          FLATPTR           fpDestVidMem,
                          DWORD             dwDestPitch,
                          DWORD             dwDestBitDepth,
                          RECTL             *rSource,
                          RECTL             *rDest);

VOID
SysMemToVidMemPackedDownload(LPP2THUNKEDDATA    pThisDisplay, 
                             FLATPTR            fpSrcVidMem,
                             DWORD              dwSrcPitch,
                             RECTL              *rSrc, 
                             FLATPTR            fpDestVidMem,
                             DWORD              dwDestWidth,
                             RECTL              *rDst,
                             DWORD              dwDestBitDepth);

VOID    
VidMemFastFill(LPP2THUNKEDDATA  pThisDisplay, 
               FLATPTR          fpDestVidMem,
               RECTL            *rDest,
               DWORD            dwColor,
               DWORD            dwDestBitDepth,
               DWORD            dwDestWidth,
               DWORD            dwWriteMask);

VOID 
VidMemYUVtoRGB(LPP2THUNKEDDATA             pThisDisplay, 
               LPDDRAWI_DDRAWSURFACE_LCL   pSrcLcl, 
               LPDDRAWI_DDRAWSURFACE_LCL   pDestLcl, 
               RECTL                       *rSrc, 
               RECTL                       *rDst);

VOID 
VidMemtoVidMemPackedCopyBlt(LPP2THUNKEDDATA             pThisDisplay, 
                            LPDDRAWI_DDRAWSURFACE_LCL   pSrcLcl, 
                            LPDDRAWI_DDRAWSURFACE_LCL   pDestLcl, 
                            RECTL                       *rSrc, 
                            RECTL                       *rDst);

VOID 
VidMemToVidMemStretchCopyBlt(LPP2THUNKEDDATA    pThisDisplay, 
                             FLATPTR            fpSrcVidMem,
                             FLATPTR            fpDestVidMem,
                             DDPIXELFORMAT      *pSrcPixFormat,
                             DDPIXELFORMAT      *pDestPixFormat,
                             RECTL              *rSrc,
                             RECTL              *rDest,
                             DWORD              dwSrcWidth,
                             DWORD              dwDestWidth,
                             BOOL               bSrcInNonLocalVidMem,
                             DWORD              dwRenderDirection,
                             BOOL               bXMirror,
                             BOOL               bYMirror);

VOID 
VidMemToVidMemChromaBlt(LPP2THUNKEDDATA             pThisDisplay, 
                        LPDDRAWI_DDRAWSURFACE_LCL   pSrcLcl, 
                        LPDDRAWI_DDRAWSURFACE_LCL   pDestLcl, 
                        RECTL                       *rSrc, 
                        RECTL                       *rDst,
                        LPDDHAL_BLTDATA             lpBlt);

VOID
VidMemToVidMemStretchChromaBlt(LPP2THUNKEDDATA             pThisDisplay, 
                               LPDDRAWI_DDRAWSURFACE_LCL   pSrcLcl, 
                               LPDDRAWI_DDRAWSURFACE_LCL   pDestLcl, 
                               RECTL                       *rSrc, 
                               RECTL                       *rDst,
                               LPDDHAL_BLTDATA             lpBlt);

#endif /* #ifdef _DLL_DD_H_ */