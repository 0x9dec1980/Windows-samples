/******************************Module*Header**********************************\
*
*                           *****************************************
*                           * Permedia 2: Direct Draw   SAMPLE CODE *
*                           *****************************************
*
* Module Name: ddhalcb.c
*
*       Contains Direct Draw HAL Callback functions
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/


#include "precomp.h"

extern LPP2THUNKEDDATA     pDriverData;

/*
 * Function:    DdCanCreateSurface
 * Description: This entry point is called after parameter validation but before
 *              any object creation. You can decide here if it is possible for
 *              you to create this surface.  For example, if the person is trying
 *              to create an overlay, and you already have the maximum number of
 *              overlays created, this is the place to fail the call.
 *              You also need to check if the pixel format specified can be supported.
 */

DWORD CALLBACK DdCanCreateSurface( LPDDHAL_CANCREATESURFACEDATA pccsd)
{
    LPP2THUNKEDDATA pThisDisplay;
    LPDDSURFACEDESC lpDDS=pccsd->lpDDSurfaceDesc;

    if (pccsd->lpDD->dwReserved3) 
        pThisDisplay = (LPP2THUNKEDDATA)pccsd->lpDD->dwReserved3;
    else 
        pThisDisplay = pDriverData; 

    
    DBG_DD((2,"DDraw:DdCanCreateSurface"));
    
    if(!(lpDDS->dwFlags & DDSD_LINEARSIZE))
    {
        // rectangular surface
        // Reject all widths larger than we can create partial products for.
        
        if ((lpDDS->dwWidth > (ULONG)(2048)) ||
            (lpDDS->dwHeight > (ULONG)(2408)))// Max resolution supported is 2048x2048 
        {
            DBG_DD((1,"DDraw:DdCanCreateSurface: Surface rejected: Width: %ld", lpDDS->dwWidth));
            pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
            return DDHAL_DRIVER_HANDLED;
        }
    }
    
    // We only support 16bits & 15bits (for stencils) Z-Buffer on PERMEDIA
    if ((lpDDS->ddsCaps.dwCaps & DDSCAPS_ZBUFFER) &&
        (lpDDS->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        DWORD dwBitDepth;
        
        // verify where the right z buffer bit depth is
        if (DDSD_ZBUFFERBITDEPTH & lpDDS->dwFlags)
            dwBitDepth = lpDDS->dwZBufferBitDepth;
        else
            dwBitDepth = lpDDS->ddpfPixelFormat.dwZBufferBitDepth;
        
        // Notice we have to check for a BitDepth of 16 even if a stencil 
        // buffer is present. dwZBufferBitDepth in this case will be the 
        // sum of the z buffer and the stencil buffer bit depth.
        if (dwBitDepth == 16)
        {
            pccsd->ddRVal = DD_OK;
        }
        else
        {
            DBG_DD((2,"DDraw:DdCanCreateSurface: ERROR: "
                       "Depth buffer not 16Bits! - %d", dwBitDepth));
            
            pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
            
        }
        return DDHAL_DRIVER_HANDLED;
    }

    // pccsd->bIsDifferentPixelFormat tells us if the pixel format of the
    // surface being created matches that of the primary surface.  It can be
    // true for Z buffer and alpha buffers, so don't just reject it out of
    // hand...
 
    if (pccsd->bIsDifferentPixelFormat)
    {
        DBG_DD((3,"  Pixel Format is different to primary"));

        if(lpDDS->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
        {
            DBG_DD((3, "  FourCC requested (%4.4hs, 0x%08lx)", (LPSTR) 
                        &lpDDS->ddpfPixelFormat.dwFourCC,
                        lpDDS->ddpfPixelFormat.dwFourCC ));

            switch (lpDDS->ddpfPixelFormat.dwFourCC)
            {
                case FOURCC_YUV422:
                    DBG_DD((3,"  Surface requested is YUV422"));
                    //if (ppdev->iBitmapFormat == BMF_8BPP)
                    if (pThisDisplay->ddpfDisplay.dwRGBBitCount == 16)
                    {
                        lpDDS->ddpfPixelFormat.dwYUVBitCount = 16;
                        pccsd->ddRVal = DD_OK;
                    } 
                    else
                    {
                        pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                    }
                return DDHAL_DRIVER_HANDLED;

                default:
                    DBG_DD((3,"  ERROR: Invalid FOURCC requested, refusing"));
                    pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                return DDHAL_DRIVER_HANDLED;
            }
        }
        else if((lpDDS->ddsCaps.dwCaps & DDSCAPS_TEXTURE))
        {

            if (bCheckTextureFormat(&pccsd->lpDDSurfaceDesc->ddpfPixelFormat))
            {
                // texture surface is in one or our supported texture formats
                DBG_DD((3, "  Texture Surface - OK" ));
                pccsd->ddRVal = DD_OK;
                return DDHAL_DRIVER_HANDLED;
            }
            else
            {
                // we don't support this kind of texture format
                DBG_DD((3, "  ERROR: Texture Surface - NOT OK" ));
                pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                return DDHAL_DRIVER_HANDLED;
            }
        } 
        pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
        return DDHAL_DRIVER_HANDLED;
    }
    
    pccsd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;

}

/*
 * Function:    DdCreateSurface
 *
 * Description: This function is called by DirectDraw if a new surface is created. 
 *              If the driver has its own memory manager, here is the place to 
 *              allocate the videomemory or to fail the call. Note that we return 
 *              DDHAL_DRIVER_NOTHANDLED here to indicate that we do not manage 
 *              the heap. fpVidMem is set to DDHAL_PLEASEALLOC_BLOCKSIZE, and the 
 *              DDraw memory manager wll allocate the memory for us.
 *  
 *              Note that the Permedia chip requires a partial product
 *              to be setup for each surface.  They also limit the widths to a multiple
 *              of 32 for the Partial Products to work.  The below code adjusts the
 *              surfaces to meet this requirement.  Note that if we are using a
 *              rectangular allocation scheme, the surface is already OK as the desktop
 *              is a good width anyway.  This code also handles YUV 16 Bit colour space
 *              compressed format (FOURCC_YUV422) which will always be 16 bits, regardless
 *              of the desktop resolution/requested depth.
 * Important Note: Please do not touch lpCreateSurface->lpDDSurfaceDesc
 *                 The information in it is redundant and this field is not required.
 */

DWORD CALLBACK 
DdCreateSurface(LPDDHAL_CREATESURFACEDATA lpCreateSurface)
{
    LPP2THUNKEDDATA             pThisDisplay;
    LPDDRAWI_DDRAWSURFACE_LCL   lpSurfaceLocal;
    LPDDRAWI_DDRAWSURFACE_GBL   lpSurfaceGlobal;
    BOOL                        bYUV = FALSE;
    DWORD                       lclSurfIndex;
    DDPIXELFORMAT               *pPixFormat;
    ULONG                       lPitch;

    if (lpCreateSurface->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)lpCreateSurface->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }


    DBG_DD((2, "DdCreateSurface called"));

    //
    // See if any of these surfaces are Z buffers. If they are, ensure that the
    // pitch is a valid LB width. The minimum partial product is 32 words or
    // 32 pixels on Permedia 2
    //
    for( lclSurfIndex=0;lclSurfIndex<(int)lpCreateSurface->dwSCnt;lclSurfIndex++ )
    {

        lpSurfaceLocal  = lpCreateSurface->lplpSList[lclSurfIndex];

        if (!(lpSurfaceLocal->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
        {
            lpSurfaceGlobal = lpSurfaceLocal->lpGbl;    
            bYUV            = FALSE;

            // Either the surface is a texture or it has a valid pixel format.
            if (lpSurfaceLocal->dwFlags & DDRAWISURF_HASPIXELFORMAT)
            {
                pPixFormat = &lpSurfaceLocal->lpGbl->ddpfSurface;
            }
            else
            {
                pPixFormat = &pThisDisplay->ddpfDisplay;
            }

            //
            // Width is in pixels/texels
            //
            lPitch = PITCH_IN_PIXELS(lpSurfaceGlobal->wWidth);

            if ( pPixFormat->dwFlags & DDPF_FOURCC )
            {   //
                // The surface is a YUV format surface or we fail 
                //
                switch ( lpSurfaceGlobal->ddpfSurface.dwFourCC )
                {
                    case FOURCC_YUV422:
                        DBG_DD((3,"  Surface is YUV422 - Adjusting"));
                        lpSurfaceGlobal->ddpfSurface.dwYUVBitCount = 16;
                        lPitch = lPitch << 1;
                    break;

                    default:
                        //
                        // We should never get here, as CanCreateSurface will
                        // validate the YUV format for us.
                        //
                        ASSERTDD(0, "Trying to create an invalid YUV surface!");
                    break;
                }
            }
            else
            {   
                if (lpSurfaceGlobal->ddpfSurface.dwFlags & DDPF_ZBUFFER) 
                {   // Z buffer is always 16bpp. see CanCreateSurface
                    lPitch = lPitch << 1;
                }
                else
                {   
                    if (pPixFormat->dwRGBBitCount == 24)
                        lPitch = lPitch * 3;
                    else 
                        lPitch = lPitch << (pPixFormat->dwRGBBitCount >> 4);
                }
            }

            lpSurfaceGlobal->lPitch         = lPitch;
            DBG_DD((5,"  Surface Pitch is: 0x%x",  lpSurfaceGlobal->lPitch));

            //
            // Whenever we encounter a case where the memory needed for
            // a surface is not of size (lpSurfaceGlobal->lPitch * lpSurfaceGlobal->wHeight)
            // we need to ask the runtime allocator to use lpSurfaceGlobal->dwBlockSizeX
            // and lpSurfaceGlobal->dwBlockSizeY and allocate memory of size
            // (lpSurfaceGlobal->dwBlockSizeX * lpSurfaceGlobal->dwBlockSizeY) 
            // This is done by filling in lpSurfaceGlobal->dwBlockSizeX and
            // lpSurfaceGlobal->dwBlockSizeY and setting 
            // lpSurfaceGlobal->fpVidMem = DDHAL_PLEASEALLOC_BLOCKSIZE;
            //
            // In general the memory block size is not know if the surface has FOURCC
            // pixel format. In these case we are forced to use the above.
            // In the case of Permedia2 there is an additional constraint that 
            // that minimum height of a surface needs to be 32 pixels.
            //
            if ( pPixFormat->dwFlags & DDPF_FOURCC )
            {   
                lpSurfaceGlobal->dwBlockSizeX   = lPitch * (DWORD)(lpSurfaceGlobal->wHeight);
                lpSurfaceGlobal->dwBlockSizeY   = 1;
                lpSurfaceGlobal->fpVidMem       = DDHAL_PLEASEALLOC_BLOCKSIZE;
            }
            else
            {   // Its an RGB surface
                if ( lpSurfaceGlobal->wHeight < 32 )
                {
                    lpSurfaceGlobal->dwBlockSizeX   = (lPitch << 5);//effectively  (lPitch * 32)
                    lpSurfaceGlobal->dwBlockSizeY   = 1;
                    lpSurfaceGlobal->fpVidMem       = DDHAL_PLEASEALLOC_BLOCKSIZE;
                }
            }
            if (lpSurfaceLocal->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
            {
                lpSurfaceGlobal->fpVidMem       = (FLATPTR)D3DMALLOCZ(lPitch * (DWORD)(lpSurfaceGlobal->wHeight));
                lpCreateSurface->ddRVal         = DD_OK;
                return DDHAL_DRIVER_HANDLED;
            }
        }
#ifdef DDFLIP_INTERVALN
        lpSurfaceLocal->dwReserved1 = (ULONG_PTR)INVALID_VBINTERRUPT_COUNT;
#endif // #ifdef DDFLIP_INTERVALN

    }//for( lclSurfIndex=0;lclSurfIndex<(int)pcsd->dwSCnt;lclSurfIndex++ )
    lpCreateSurface->ddRVal = DD_OK;

    return DDHAL_DRIVER_NOTHANDLED;

}// DdCreateSurface()

/*
 * Function;        DdWaitForVerticalBlank
 * Description:     returns the vertical blank status of the device.
 */

DWORD CALLBACK 
DdWaitForVerticalBlank( LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
    LPP2THUNKEDDATA pThisDisplay;

    if (lpWaitForVerticalBlank->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)lpWaitForVerticalBlank->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }

    switch(lpWaitForVerticalBlank->dwFlags)
    {
        case DDWAITVB_I_TESTVB:
            //
            // Just a request for current VBLANK status
            //
            lpWaitForVerticalBlank->ddRVal = DD_OK;
            lpWaitForVerticalBlank->bIsInVB = IN_VRETRACE(pThisDisplay);
        return DDHAL_DRIVER_HANDLED;

        case DDWAITVB_BLOCKBEGIN:
            /* 
            * if blockbegin is requested we wait until the vertical retrace
            * is over, and then wait for the display period to end.
            */
            while(IN_VRETRACE(pThisDisplay));
            while(IN_DISPLAY(pThisDisplay));
            lpWaitForVerticalBlank->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;

        case DDWAITVB_BLOCKEND:
            /* 
            * if blockend is requested we wait for the vblank interval to end.
            */
            if( IN_VRETRACE(pThisDisplay) )
            {
                while( IN_VRETRACE(pThisDisplay) );
            }
            else
            {
                while(IN_DISPLAY(pThisDisplay));
                while(IN_VRETRACE(pThisDisplay));
            }
            lpWaitForVerticalBlank->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;
    }

    return DDHAL_DRIVER_NOTHANDLED;
}

/*
 * Function;        DdGetScanLine
 * Description:     returns the number of the current physical scan line.
 */

DWORD CALLBACK DdGetScanLine( LPDDHAL_GETSCANLINEDATA lpGetScanLine)
{
    LPP2THUNKEDDATA pThisDisplay;

    if (lpGetScanLine->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)lpGetScanLine->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }

    DBG_DD(( 2, "DDraw:GetScanLine"));

    //  If a vertical blank is in progress the scan line is 
    //  indeterminant. If the scan line is indeterminant we return
    //  the error code DDERR_VERTICALBLANKINPROGRESS.
    //  Otherwise we return the scan line and a success code

    if( IN_VRETRACE(pThisDisplay) )
    {
        lpGetScanLine->ddRVal = DDERR_VERTICALBLANKINPROGRESS;
        lpGetScanLine->dwScanLine = 0;
    }
    else
    {
        lpGetScanLine->dwScanLine = CURRENT_VLINE(pThisDisplay);
        lpGetScanLine->ddRVal = DD_OK;
    }
    return DDHAL_DRIVER_HANDLED;

}

/*
 * Function;        DdFlipToGDISurface
 * Description:     This function is called by DirectDraw when it flips 
 *                  to the surface on which GDI can write to.
 */
  
DWORD CALLBACK
DdFlipToGDISurface(LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurface)
{
    LPP2THUNKEDDATA pThisDisplay;

    if (lpFlipToGDISurface->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)lpFlipToGDISurface->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }

    DBG_DD((6, "DDraw::DdFlipToGDISurface called"));

    pThisDisplay->dwNewDDSurfaceOffset=0xffffffff;

    if (pThisDisplay->bDdStereoMode == TRUE)
    {
        HWDisableStereoMode(pThisDisplay);    
    }

    lpFlipToGDISurface->ddRVal=DD_OK;

    //
    //  we return NOTHANDLED, then the ddraw runtime takes
    //  care that we flip back to the primary...
    //
    return (DDHAL_DRIVER_NOTHANDLED);
}

/*
 * Function:    UpdateNonLocalHeap32
 * Description: Receives the address of the AGP Heap and updates the chip.
 */
DWORD CALLBACK 
DdUpdateNonLocalHeap(LPDDHAL_UPDATENONLOCALHEAPDATA plhd)
{
    LPP2THUNKEDDATA pThisDisplay;

    if (plhd->lpDD->dwReserved3) 
    {
        pThisDisplay = (LPP2THUNKEDDATA)plhd->lpDD->dwReserved3;
    }
    else 
    {
        pThisDisplay = pDriverData; 
    }

    DISPDBG((3,"DdUpdateNonLocalHeap32 - for Heap 0x%x", plhd->dwHeap));

    pThisDisplay->dwGARTLin = (DWORD)plhd->fpGARTLin;
    pThisDisplay->dwGARTDev = (DWORD)plhd->fpGARTDev;

    HWLoadAGPTextureBaseAddress(pThisDisplay);

    DISPDBG((3,"GartLin: 0x%x, GartDev: 0x%x", plhd->fpGARTLin, plhd->fpGARTDev));
    plhd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
} // UpdateNonLocalHeap32()