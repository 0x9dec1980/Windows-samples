/******************************Module*Header**********************************\
*
*                           ***************************************
*                           * Permedia 2: Direct Draw SAMPLE CODE *
*                           ***************************************
*
* Module Name: ddsurfcb.c
*
*       Contains Direct Draw Surface Callbacks. 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

extern LPP2THUNKEDDATA     pDriverData;

/*
 * Function:    DdDestroySurface
 * Description: Direct Draw surface callback. This is called when a surface is 
 *              about to be destroyed.
 */
DWORD CALLBACK DdDestroySurface( LPDDHAL_DESTROYSURFACEDATA psdd)
{
    psdd->ddRVal = DD_OK;

    if(psdd->lpDDSurface->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
    {
        if (DDRAWISURF_INVALID & psdd->lpDDSurface->dwFlags)
        {
            // We get DdDestroySurface with DDRAWISURF_INVALID flag set when 
            // need to destroy the Video Memory portion of a Driver Managed surface. 
            // This happens during a mode switch or when an app gets or losses 
            // exclusive mode regardless without mode switching. The driver needs to 
            // destroy only the VideoMemory portion of the managed surface. The system
            // memory portion NOT destroyed. The VideoMemory portion is restored when the
            // texture is used again.
            RemoveManagedSurface((LPVOID)psdd->lpDDSurface->lpSurfMore->lpDD_lcl, 
                                 (DWORD)psdd->lpDDSurface->lpSurfMore->dwSurfaceHandle);
            psdd->ddRVal = DD_OK;
            return DDHAL_DRIVER_HANDLED;
        }
        RemoveSurfaceHandle((LPVOID)psdd->lpDDSurface->lpSurfMore->lpDD_lcl, 
                            (DWORD)psdd->lpDDSurface->lpSurfMore->dwSurfaceHandle);
        psdd->lpDDSurface->lpGbl->fpVidMem = 0;
        // indicate that driver takes care of the lost surface already
        return DDHAL_DRIVER_HANDLED;
    }
    RemoveSurfaceHandle((LPVOID)psdd->lpDDSurface->lpSurfMore->lpDD_lcl, 
                        (DWORD)psdd->lpDDSurface->lpSurfMore->dwSurfaceHandle);
    if (psdd->lpDDSurface->ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
    {   // Surface being destroyed is a D3D Render target
        // We need to identify the contexts for which this is the render target 
        // and mark them with CONTEXT_DIRTY_RENDERTARGET
        DirtyContextWithThisRenderTarget((LPVOID)psdd->lpDDSurface->lpSurfMore->lpDD_lcl, 
                                         (DWORD)psdd->lpDDSurface->lpSurfMore->dwSurfaceHandle);

    }

    return DDHAL_DRIVER_NOTHANDLED;
}

/*
 * Function:    DdFlip
 * Description: This callback is invoked whenever we are about to flip to from
 *              one surface to another. lpFlipData->lpSurfCurr is the surface 
 *              we were at, lpFlipData->lpSurfTarg is the one we are flipping to.
 *
 *              You should point the hardware registers at the new surface, and
 *              also keep track of the surface that was flipped away from, so
 *              that if the user tries to lock it, you can be sure that it is done
 *              being displayed
 *
 */

DWORD CALLBACK
DdFlip( LPDDHAL_FLIPDATA lpFlipData)
{
    LPP2THUNKEDDATA     pThisDisplay;
    DWORD               dwDDSurfaceOffset;
    HRESULT             ddrval;
    DWORD               dwFlipInterval;
    ULONG               dwCurrentIntrCount, dwExpectedIntrCount, dwInterruptCount;

    DBG_DD(( 3, "DDraw:Flip"));

    if (lpFlipData->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)lpFlipData->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }

    // is the previous Flip already done?
    // check if the current surface is already displayed
    ddrval = updateFlipStatus(pThisDisplay);
    if( ddrval )
    {   // ddrval != DD_OK
        lpFlipData->ddRVal = ddrval;
        return DDHAL_DRIVER_HANDLED;
    }
    // Note this check for DrawEngineBusy may not be necessary
    if (bDrawEngineBusy(pThisDisplay) == TRUE)
    {
        lpFlipData->ddRVal = DDERR_WASSTILLDRAWING;
        return DDHAL_DRIVER_HANDLED;
    }

#ifdef DDFLIP_INTERVALN
    if (dwFlipInterval = (lpFlipData->dwFlags & FLIP_INTERVAL_MASK)) 
    {
        if (dwFlipInterval != 0x01000000L)  
        {
            if (lpFlipData->lpSurfTarg->dwReserved1 == (ULONG_PTR)INVALID_VBINTERRUPT_COUNT)
            {
                lpFlipData->lpSurfTarg->dwReserved1 = (ULONG_PTR)ulHWGetVerticalBlankIntrCount(pThisDisplay);
                lpFlipData->ddRVal = DDERR_WASSTILLDRAWING;
                DBG_DD(( 2, "DDraw:Flip:INTERVALN SetupCurrentIntr=%d", lpFlipData->lpSurfTarg->dwReserved1));
                return DDHAL_DRIVER_HANDLED;
            }
            else 
            {
                dwCurrentIntrCount  = ulHWGetVerticalBlankIntrCount(pThisDisplay);
                dwInterruptCount    = (ULONG)lpFlipData->lpSurfTarg->dwReserved1;
                if(dwFlipInterval == DDFLIP_INTERVAL2)
                    dwExpectedIntrCount = (dwInterruptCount + 1) % MAX_P2INTERRUPT_COUNT;
                else if(dwFlipInterval == DDFLIP_INTERVAL3)
                    dwExpectedIntrCount = (dwInterruptCount + 2) % MAX_P2INTERRUPT_COUNT;
                else if(dwFlipInterval == DDFLIP_INTERVAL4)
                    dwExpectedIntrCount = (dwInterruptCount + 3) % MAX_P2INTERRUPT_COUNT;

                if (dwExpectedIntrCount >= dwInterruptCount)
                {
                    if (!((dwCurrentIntrCount >= dwExpectedIntrCount)||
                          (dwCurrentIntrCount < dwInterruptCount)))
                    {
                        lpFlipData->ddRVal = DDERR_WASSTILLDRAWING;
                        return DDHAL_DRIVER_HANDLED;
                    }
                }
                else
                {
                    if (!((dwCurrentIntrCount >= dwExpectedIntrCount)&&
                          (dwCurrentIntrCount < dwInterruptCount)))
                    {
                        lpFlipData->ddRVal = DDERR_WASSTILLDRAWING;
                        return DDHAL_DRIVER_HANDLED;
                    }
                }
            }
            if (dwExpectedIntrCount == dwCurrentIntrCount)
            {   //Make sure we are still not in the Vertical Blank Period
                if (IN_VRETRACE(pThisDisplay))
                {
                    lpFlipData->ddRVal = DDERR_WASSTILLDRAWING;
                    return DDHAL_DRIVER_HANDLED;
                }
            }

            DBG_DD(( 2, "DDraw:Flip:Executing INTERVALN =0x%x Intr=%d ExpectedIntr=%d CurrentIntr=%d", 
                        dwFlipInterval, dwInterruptCount, dwExpectedIntrCount, dwCurrentIntrCount));
            lpFlipData->lpSurfTarg->dwReserved1 = (ULONG_PTR)INVALID_VBINTERRUPT_COUNT;

        }
        else
        {
            vHWDisableVerticalBlankInterrupts(pThisDisplay);
        }

    }
    else
    {
        vHWDisableVerticalBlankInterrupts(pThisDisplay);
    }
#endif //#ifdef DDFLIP_INTERVALN
    
    SyncHardware(pThisDisplay); // Just a precaution
    // everything is OK, do the flip.
    // get offset for Permedia ScreenBase register
    dwDDSurfaceOffset= 
        lpFlipData->lpSurfTarg->lpGbl->fpVidMem - pThisDisplay->dwScreenFlatAddr;

    if (lpFlipData->dwFlags & DDFLIP_STEREO)   // will be stereo
    {
        DBG_DD((4,"DDraw:Flip:Stereo"));
        DBG_DD((5,"ScreenBase: %08lx", dwDDSurfaceOffset));

        if (lpFlipData->lpSurfTargLeft!=NULL)
        {
            DWORD dwDDLeftSurfaceOffset;
            dwDDLeftSurfaceOffset= 
                lpFlipData->lpSurfTargLeft->lpGbl->fpVidMem - pThisDisplay->dwScreenFlatAddr;
            HWLoadLeftSurfaceBaseAddress(pThisDisplay, dwDDLeftSurfaceOffset);
            DBG_DD((5,"ScreenBaseLeft: %08lx", dwDDLeftSurfaceOffset));
        }

        HWEnableStereoMode(pThisDisplay);
    } 
    else 
    {
        // append flip command to Permedia render pipeline
        // that makes sure that all buffers are flushed before
        // the flip occurs
        HWDisableStereoMode(pThisDisplay);
    }
  
    HWLoadNewFrameBaseAddress(pThisDisplay, dwDDSurfaceOffset);
    // remember new Surface Offset for GetFlipStatus
    pThisDisplay->dwNewDDSurfaceOffset=dwDDSurfaceOffset;
    lpFlipData->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdFlip 

/*
 * Function:    Lock
 * Description: This call is invoked to lock a DirectDraw Videomemory surface. 
 *              To make sure there are no pending drawing operations on the 
 *              surface, flush all drawing operations and wait for a flip 
 *              if it is still pending.
 *              This is called before the user reads/writes to the surface directly.
 *              "Lock" is really a misnomer, what lock does is basically tell the user
 *              that it is safe to access the surface directly.
 *              There are two things worry about:
 *              1. If the surface is in one to which a flip was just issued. Make
 *                 sure the flip is completed.
 *              2. All pending rendering operations to the surface is done.
 *              Note that since we don't keep track of rendering operations explicitly
 *              per surface, we just need to wait till all the rendering operations
 *              are done and the Permedia2 is idle.
 */

DWORD CALLBACK
DdLock( LPDDHAL_LOCKDATA lpLockData )
{
    LPP2THUNKEDDATA             pThisDisplay;
    LPDDRAWI_DDRAWSURFACE_LCL   lpLcl=lpLockData->lpDDSurface;
    LPDDRAWI_DDRAWSURFACE_GBL   lpGbl=lpLcl->lpGbl;
    HRESULT                     ddrval;

    DBG_DD(( 2, "DDraw:Lock"));

    if (lpLockData->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)lpLockData->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }

    // Check if the surface to be locked is the surface we have
    // just issued a flip to. If so wait for the flip to complete.
    if (lpGbl->fpVidMem == (pThisDisplay->dwScreenFlatAddr + 
                            pThisDisplay->dwNewDDSurfaceOffset))
    {
        ddrval = updateFlipStatus(pThisDisplay);
        if((ddrval = updateFlipStatus(pThisDisplay)) != DD_OK)
        {
            lpLockData->ddRVal = ddrval;
            return DDHAL_DRIVER_HANDLED;
        }
    }
    
    if (bDrawEngineBusy(pThisDisplay) == TRUE)
    {
        lpLockData->ddRVal = DDERR_WASSTILLDRAWING;
        return DDHAL_DRIVER_HANDLED;
    }
    // Hopefully all the commands in the FIFO are done and
    // the FIFO is empty. But as a precaution
    // send a flush and wait for outstanding operations 
    // before allowing surfaces to be locked. Also make sure
    // once again that the Graphics processor is idle.
    SyncHardware(pThisDisplay); 

    if (bDrawEngineBusy(pThisDisplay) == TRUE)  
    {
        lpLockData->ddRVal = DDERR_WASSTILLDRAWING;
        return DDHAL_DRIVER_HANDLED;
    }

    //
    //  If the user attempts to lock a managed surface, mark it as dirty
    //  and return. 
    //
    if (lpLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
    {

        DBG_DD(( 3, "DDraw:Lock %08lx %08lx",
                lpLcl->lpSurfMore->dwSurfaceHandle, lpGbl->fpVidMem));
        if (LockManagedSurface((LPVOID)lpLcl->lpSurfMore->lpDD_lcl,
                               (DWORD)lpLcl->lpSurfMore->dwSurfaceHandle) == TRUE)
        {
            if (lpLockData->bHasRect)
            {
                lpLockData->lpSurfData  = (LPVOID) (lpLcl->lpGbl->fpVidMem +
                                                    lpLcl->lpGbl->lPitch * lpLockData->rArea.top +
                                                    (lpLockData->rArea.left << (DDSurf_BitDepth(lpLcl)>>4)));
            }
            else
            {
                lpLockData->lpSurfData  = (LPVOID) (lpLcl->lpGbl->fpVidMem);
            }
            lpLockData->ddRVal      = DD_OK;
        }
        else
        {
            lpLockData->ddRVal      = DDERR_INVALIDPARAMS;
        }
        return DDHAL_DRIVER_HANDLED;
    }
    // Because we correctly set 'fpVidMem' to be the offset into our frame
    // buffer when we created the surface, DirectDraw will automatically take
    // care of adding in the user-mode frame buffer address if we return
    // DDHAL_DRIVER_NOTHANDLED:
 
    return DDHAL_DRIVER_NOTHANDLED;
    
} // DdLock 


/*
 * Function:    DdGetBltStatus
 * Description: This callback is invoked to get the current blit status or 
 *              to ask if the user can add the next blit.
 */

DWORD CALLBACK 
DdGetBltStatus(LPDDHAL_GETBLTSTATUSDATA lpGetBltStatus )
{
    LPP2THUNKEDDATA             pThisDisplay;
    
    DBG_DD(( 2, "DDraw:DdGetBltStatus"));

    if (lpGetBltStatus->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)lpGetBltStatus->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }

    lpGetBltStatus->ddRVal = DD_OK;
    
    if( lpGetBltStatus->dwFlags == DDGBS_CANBLT )
    {
        if ((lpGetBltStatus->lpDDSurface->lpGbl->fpVidMem - pThisDisplay->dwScreenFlatAddr) ==
                pThisDisplay->dwNewDDSurfaceOffset)
        {   // This is indeed the last surface which was flipped
            lpGetBltStatus->ddRVal = updateFlipStatus(pThisDisplay);
        }
    }
    else
    {
        if(bDrawEngineBusy(pThisDisplay) == TRUE)
        {
            lpGetBltStatus->ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

    return DDHAL_DRIVER_HANDLED;
    
} // DdGetBltStatus 

/*
 *
 * Function:    DdGetFlipStatus
 * Description: If the display has went through one refresh cycle since the flip
 *              occurred we return DD_OK.  If it has not went through one refresh
 *              cycle we return DDERR_WASSTILLDRAWING to indicate that this surface
 *              is still busy "drawing" the flipped page. We also return
 *              DDERR_WASSTILLDRAWING if the bltter is busy and the caller wanted
 *              to know if they could flip yet
 *
 */

DWORD CALLBACK 
DdGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatus )
{
    LPP2THUNKEDDATA             pThisDisplay;
    
    DBG_DD(( 2, "DDraw:DdGetFlipStatus"));

    if (lpGetFlipStatus->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)lpGetFlipStatus->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }

    lpGetFlipStatus->ddRVal = updateFlipStatus(pThisDisplay);

    if (lpGetFlipStatus->ddRVal == DD_OK)
    {
        if (bDrawEngineBusy(pThisDisplay) == TRUE)
        {
            lpGetFlipStatus->ddRVal = DDERR_WASSTILLDRAWING;
            return DDHAL_DRIVER_HANDLED;
        }
    }

    // There is no need to check for 
    // if( lpGetFlipStatus->dwFlags == DDGFS_CANFLIP )
    // and check if the drawEngine is busy since we already
    // do it in updateFlipStatus
    return DDHAL_DRIVER_HANDLED;    
} // DdGetFlipStatus 



/*
 * Function:    DdBlt
 * Description: Direct Draw Blt callback
 */

DWORD CALLBACK 
DdBlt( LPDDHAL_BLTDATA lpBlt )
{
    LPP2THUNKEDDATA             pThisDisplay;
    RECTL                       rSrc;
    RECTL                       rDest;
    DWORD                       dwFlags;
    HRESULT                     ddrval;
    LPDDRAWI_DDRAWSURFACE_LCL   pSrcLcl;
    LPDDRAWI_DDRAWSURFACE_LCL   pDestLcl;
    BOOL                        bMirror = 0;
    BOOL                        bStretched;
    
    DBG_DD((2,"DDraw: Blt"));

    if (lpBlt->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)lpBlt->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }

    pDestLcl    = lpBlt->lpDDDestSurface;
    pSrcLcl     = lpBlt->lpDDSrcSurface;
    dwFlags     = lpBlt->dwFlags;
    rSrc        = lpBlt->rSrc;
    rDest       = lpBlt->rDest;

    // First and foremost we don't want to Blt into a surface which is 
    // involved in a Flip, since this can cause tearing. So check this.
    if ((pDestLcl->lpGbl->fpVidMem - pThisDisplay->dwScreenFlatAddr) ==
            pThisDisplay->dwNewDDSurfaceOffset)
    {   // This is indeed the last surface which was flipped
        if ((ddrval = updateFlipStatus(pThisDisplay)) != DD_OK)
        {
            lpBlt->ddRVal = ddrval;
            return DDHAL_DRIVER_HANDLED;
        }
    }
   
    // For the future, drivers should ignore the DDBLT_ASYNC
    // flag, because its hardly used by applications and
    // nowadays drivers can queue up lots of blits, so that
    // the applications do not have to wait for it.

    if (DDSurf_BitDepth(pDestLcl)==24)
    {   
        DBG_DD((1,"DDraw: Blt destination is 24bpp"));
        return DDHAL_DRIVER_NOTHANDLED;  
    }

    if (dwFlags & DDBLT_ROP)
    {

        if ((lpBlt->bltFX.dwROP >> 16) != (SRCCOPY >> 16))
        {
            DBG_DD((1,"DDraw:Blt:BLT ROP not supported! bltFX.dwROP 0x%x", lpBlt->bltFX.dwROP));
            return DDHAL_DRIVER_NOTHANDLED;
        }
        if (pSrcLcl == NULL)
        {
            DBG_DD((1,"DDraw:Blt:BLT ROP pSrcLcl = NULL!"));
            return DDHAL_DRIVER_NOTHANDLED;
        }
        else if (DDSurf_BitDepth(pSrcLcl)==24)
        {
            DBG_DD((1,"DDraw:Blt:BLT Source is 24bpp!"));
            return DDHAL_DRIVER_NOTHANDLED;
        }

        DBG_DD((3,"DDBLT_ROP:  SRCCOPY"));
        // Operation is System -> Video memory blit, 
        // The only operation we support is a plain SRC Copy Blt.
        // No need to bother about color space conversion, etc.,
        // See function GetDDHALInfo in ddinit.c for 
        // dwSVBCaps, dwSVBCKeyCaps and dwSVBFXCaps for details
        if ((pSrcLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) && 
            (pDestLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
        {
            if ((pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)||
                (pDestLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM))
            {   //System Memory -> AGP Memory (Non Local Video Memory)
                DBG_DD((3,"DDBLT_ROP:  SRCCOPY:SysMemToSysMemSurfaceCopy"));
                SysMemToSysMemSurfaceCopy(pThisDisplay, 
                                          pSrcLcl->lpGbl->fpVidMem,
                                          pSrcLcl->lpGbl->lPitch,
                                          DDSurf_BitDepth(pSrcLcl),
                                          pDestLcl->lpGbl->fpVidMem,
                                          pDestLcl->lpGbl->lPitch,
                                          DDSurf_BitDepth(pDestLcl),
                                          &rSrc,
                                          &rDest);
                if (pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
                {
                    MarkManagedSurfaceNeedsUpdate((LPVOID)pDestLcl->lpSurfMore->lpDD_lcl, 
                                                  (DWORD)pDestLcl->lpSurfMore->dwSurfaceHandle);
                }
            }
            else
            {   // System Memory -> Video Memory
                DBG_DD((3,"SYSMEM->VIDMEM Blit (system to videomemory)"));
                SysMemToVidMemPackedDownload(pThisDisplay, 
                                             pSrcLcl->lpGbl->fpVidMem,
                                             pSrcLcl->lpGbl->lPitch,
                                             &rSrc,
                                             pDestLcl->lpGbl->fpVidMem, 
                                             pDestLcl->lpGbl->wWidth, 
                                             &rDest, 
                                             DDSurf_BitDepth(pDestLcl));
            }

            lpBlt->ddRVal = DD_OK;
            return DDHAL_DRIVER_HANDLED;
        }

        if (pSrcLcl->lpGbl->ddpfSurface.dwFlags & DDPF_FOURCC)
        {
            if ((pSrcLcl->lpGbl->ddpfSurface.dwFourCC == FOURCC_YUV422)&&
                (!(pDestLcl->lpGbl->ddpfSurface.dwFlags & DDPF_FOURCC)))
            {   // Operation is YUV->RGB conversion
                DBG_DD((3,"DDBLT_ROP:  SRCCOPY:VidMemYUVtoRGB"));
                VidMemYUVtoRGB(     pThisDisplay, 
                                    pSrcLcl, 
                                    pDestLcl, 
                                    &rSrc, 
                                    &rDest);
                lpBlt->ddRVal = DD_OK;
                return DDHAL_DRIVER_HANDLED;
            }
            else
            {
                DBG_DD((0,"Couldn't handle YUV to YUV blt"));
                lpBlt->ddRVal = DD_OK;
                return DDHAL_DRIVER_NOTHANDLED;
            }
        }


        ASSERTDD(DDSurf_BitDepth(pSrcLcl)==DDSurf_BitDepth(pDestLcl),
                 "Blt between surfaces of different"
                 "color depths are not supported");

        if ((dwFlags & DDBLT_DDFX)==DDBLT_DDFX)
        {
            bMirror=  (lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN) || 
                      (lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT);
        }
        bStretched=((rSrc.right - rSrc.left) != (rDest.right - rDest.left) || 
                    (rSrc.bottom - rSrc.top) != (rDest.bottom - rDest.top));

        // Is it a colorkey blt?
        if (dwFlags & DDBLT_KEYSRCOVERRIDE)
        {
            DBG_DD((3,"DDBLT_KEYSRCOVERRIDE"));

            // If the surface sizes don't match, then we are stretching.
            if (bStretched || bMirror)
            {
                DBG_DD((3,"DDBLT_ROP:  SRCCOPY:VidMemToVidMemStretchChromaBlt"));
                VidMemToVidMemStretchChromaBlt( pThisDisplay, 
                                                pSrcLcl, 
                                                pDestLcl, 
                                                &rSrc, 
                                                &rDest,
                                                lpBlt);
            }
            else
            {
                DBG_DD((3,"DDBLT_ROP:  SRCCOPY:VidMemToVidMemChromaBlt"));
                VidMemToVidMemChromaBlt(    pThisDisplay, 
                                            pSrcLcl, 
                                            pDestLcl, 
                                            &rSrc, 
                                            &rDest,
                                            lpBlt);
            }
            lpBlt->ddRVal = DD_OK;
            return DDHAL_DRIVER_HANDLED;
        }
        else
        { 
            // If the surface sizes don't match, then we are stretching.
            // Also the blits from Nonlocal- to Videomemory have to go through
            // the texture unit!
            if ( bStretched || bMirror || (pSrcLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM))
            {
                DWORD   dwRenderDirection;

                dwRenderDirection = GetRenderDirection(pSrcLcl->lpGbl->fpVidMem,
                                                       pDestLcl->lpGbl->fpVidMem,
                                                       &rSrc,
                                                       &rDest);
                DBG_DD((3,"DDBLT_ROP:  SRCCOPY:VidMemToVidMemStretchCopyBlt"));
                VidMemToVidMemStretchCopyBlt(pThisDisplay, 
                                             pSrcLcl->lpGbl->fpVidMem,
                                             pDestLcl->lpGbl->fpVidMem,
                                             DDSurf_PixelFormat(pSrcLcl),
                                             DDSurf_PixelFormat(pDestLcl),
                                             &rSrc, 
                                             &rDest,
                                             pSrcLcl->lpGbl->wWidth,
                                             pDestLcl->lpGbl->wWidth,
                                             (pSrcLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM),
                                             dwRenderDirection,
                                             GetXMirror(lpBlt, dwRenderDirection),
                                             GetYMirror(lpBlt, dwRenderDirection));
            }
            else
            {   //Plain old source copy blt.
                DBG_DD((3,"DDBLT_ROP:  SRCCOPY:VidMemtoVidMemPackedCopyBlt"));
                VidMemtoVidMemPackedCopyBlt(pThisDisplay, 
                                            pSrcLcl, 
                                            pDestLcl, 
                                            &rSrc, 
                                            &rDest);
            }
            lpBlt->ddRVal = DD_OK;
            return DDHAL_DRIVER_HANDLED;
        }
    }
    else if (dwFlags & DDBLT_COLORFILL)
    {
        DBG_DD((3,"DDBLT_COLORFILL: Color=0x%x", lpBlt->bltFX.dwFillColor));
        if (pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
                    DDSCAPS2_TEXTUREMANAGE)
        {   
            ClearManagedSurface(pDestLcl,
                                &rDest, 
                                lpBlt->bltFX.dwFillColor);
        }
        else
        {   // Direct Draw Surface which is not a managed texture
            VidMemFastFill(pThisDisplay, 
                           pDestLcl->lpGbl->fpVidMem,
                           &rDest,
                           lpBlt->bltFX.dwFillColor,
                           DDSurf_BitDepth(pDestLcl),
                           pDestLcl->lpGbl->wWidth,
                           0xFFFFFFFF);
         }
    }
    else if (dwFlags & DDBLT_DEPTHFILL)
    {   // Z Buffer clear
        DBG_DD((3,"DDBLT_DEPTHFILL:  Value=0x%x", lpBlt->bltFX.dwFillColor));
        VidMemFastFill(pThisDisplay, 
                       pDestLcl->lpGbl->fpVidMem,
                       &rDest,
                       lpBlt->bltFX.dwFillColor,
                       16,                      //Always 16 bits
                       pDestLcl->lpGbl->wWidth,
                       0xFFFFFFFF);
    }
    else
    {
        DBG_DD((1,"DDraw:Blt:Blt case not supported %08lx", dwFlags));
        return DDHAL_DRIVER_NOTHANDLED;
    }
    
    lpBlt->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdBlt ()

