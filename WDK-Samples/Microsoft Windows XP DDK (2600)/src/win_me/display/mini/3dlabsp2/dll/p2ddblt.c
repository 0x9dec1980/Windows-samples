/******************************Module*Header**********************************\
*
*       **************************************************
*       * Permedia 2: Direct Draw/Direct 3D  SAMPLE CODE *
*       **************************************************
*
* Module Name: p2ddblt.c
*
*       Contains low level blting functions for Permedia 2
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

/*
 * Function:    SysMemToSysMemSurfaceCopy
 * Description: Does a copy from System memory to System memory (either from 
 *              or to an AGP surface, or any other system memory surface)
 *
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
                          RECTL             *rDest)
{
    BYTE    *pSourceStart;
    BYTE    *pDestStart;
    BYTE     pixSource;
    BYTE    *pNewDest;
    BYTE    *pNewSource;
    INT     i;
    LONG    lByteWidth = rSource->right - rSource->left;
    LONG    lHeight    = rSource->bottom - rSource->top;

    DBG_DD(( 5, "DDraw:SysMemToSysMemSurfaceCopy"));

    // Calculate the start pointer for the source and the dest
    pSourceStart    = (BYTE*)(fpSrcVidMem + (rSource->top * dwSrcPitch));
    pDestStart      = (BYTE*)(fpDestVidMem + (rDest->top * dwDestPitch));
    
    STOP_SOFTWARE_CURSOR(pThisDisplay);
    vSyncWithPermedia(pThisDisplay); 

    // Be careful if the source is 4 bits deep.
    if(dwSrcBitDepth == 4)
    {
        // May have to handle horrible single pixel edges.  Check if we need to
        if (!((1 & (rSource->left ^ rDest->left)) == 1))
        {
            pSourceStart += rSource->left / 2;
            pDestStart += rDest->left / 2;
            lByteWidth /= 2;

            // Do we have to account for the odd pixel at the start?
            if (rSource->left & 0x1) 
            {
                lByteWidth--;
            }

            // If the end is odd then miss of the last nibble (do it later).
            if (rSource->right & 0x1) 
            {
                lByteWidth--;
            }

            while (--lHeight >= 0) 
            {
                // Potentially copy the left hand pixel
                if (rSource->left & 0x1) 
                {
                    *pDestStart &= 0x0F;
                    *pDestStart |= (*pSourceStart & 0xF0);

                    pNewDest = pDestStart + 1;
                    pNewSource = pSourceStart + 1;
                } 
                else 
                {
                    pNewDest = pDestStart;
                    pNewSource = pSourceStart;
                }

                // Byte copy the rest of the field
                memcpy(pNewDest, pNewSource, lByteWidth);

                // Potentially copy the right hand pixel
                if (rSource->right & 0x1) 
                {
                    *(pNewDest + lByteWidth) &= 0xF0;
                    *(pNewDest + lByteWidth) |= (*(pNewSource + lByteWidth) & 0xF);
                }

                pDestStart += dwDestPitch;
                pSourceStart += dwSrcPitch;
            }
        } 
        else 
        {   // Do it the hard way - copy single pixels one at a time

            pSourceStart += rSource->left / 2;
            pDestStart += rDest->left / 2;

            while (--lHeight >= 0) 
            {
                BOOL bOddSource = rSource->left & 0x1;
                BOOL bOddDest = rDest->left & 0x1;

                pNewDest = pDestStart;
                pNewSource = pSourceStart;

                for (i = 0; i < lByteWidth; i++) 
                {
                    if (bOddSource) 
                    {
                        pixSource = (*pNewSource & 0xF0) >> 4;
                        pNewSource++;
                    } 
                    else 
                    {
                        pixSource = (*pNewSource & 0x0F);
                    }

                    if (bOddDest) 
                    {
                        *pNewDest &= 0x0F;
                        *pNewDest |= pixSource << 4;
                        pNewDest++;
                    } 
                    else 
                    {
                        *pNewDest &= 0xF0;
                        *pNewDest |= pixSource;
                    }

                    bOddSource = !bOddSource;
                    bOddDest = !bOddDest;
                }

                // Step onto the next line
                pDestStart += dwDestPitch;
                pSourceStart += dwSrcPitch;
            }
        }
    }
    else // The simple 8, 16 or 24 bit copy
    {
        pSourceStart += (rSource->left * (dwSrcBitDepth >> 3));
        pDestStart += (rDest->left * (dwDestBitDepth >> 3));
        lByteWidth *= (dwSrcBitDepth >> 3);

        while (--lHeight >= 0)
        {
            memcpy(pDestStart, pSourceStart, lByteWidth);
            pDestStart += dwDestPitch;
            pSourceStart += dwSrcPitch;
        };
    }
    START_SOFTWARE_CURSOR(pThisDisplay);
}   // SysMemToSysMemSurfaceCopy 

/*
 * Function:    SysMemToVidMemPackedDownload
 * Description: Function to do a system memory to video memory blt.
 *              It is guaranteed by the calling function that the rSrc and rDest rectangles 
 *              are identical in size. Also all surfaces if allocated by DDraw are 
 *              DWORD (32bit) aligned. Also from the SVBXXX caps bits we set 
 *              (See function GetDDHALInfo in ddinit.c for dwSVBCaps, dwSVBCKeyCaps 
 *              and dwSVBFXCaps for details) the two surfaces have identical pixel
 *              formats. Surfaces created by DDraw in System Memory are always in 
 *              Packed Pixel Format (i.e., if 8bpp implies 4 pixels per DWORD,
 *              if 16bpp implies 2 pixels per DWORD)
 *              Permedia2 provides functionality to download bitmaps which are 8bpp
 *              or 16bpp one DWORD at a time. The source and destination address are DWORD
 *              aligned. Pixels on the right and left edges are discarded by setting the
 *              PackedDataLimits register. Also since both source and destination 
 *              addresses have to be DWORD aligned, the (left, top) pixel in the source
 *              and destination may not start at the identical by boundary of the DWORD.
 *              Hence we need to specify a (Pixel) Shift by which the source rectangle 
 *              needs to be moved either to the left or right with respect to the 
 *              destination.
 *              Note that since DDraw allocated surfaces are always aligned to a DWORD
 *              boundary, we don't have to worry about access voilations when we use
 *              DWORD aligned addresses for the source and destination rectangles.
 */

VOID
SysMemToVidMemPackedDownload(LPP2THUNKEDDATA    pThisDisplay, 
                             FLATPTR            fpSrcVidMem,
                             DWORD              dwSrcPitch,
                             RECTL              *rSrc, 
                             FLATPTR            fpDestVidMem,
                             DWORD              dwDestWidth,
                             RECTL              *rDst,
                             DWORD              dwDestBitDepth)
{
    ULONG   ulDstOffset;// DWORD aligned offset of (top,left) pixel of DestSurf 
    BYTE    *pSrcData; 
    LONG    lOffset;    // relative offset between src and dest 
    DWORD   ulPackedPP, ulFBReadPixelValue;
    LONG    lPackedWidth, lSrcPackedWidth, lDstPackedWidth;// packed width to download
    LONG    lScanLines;
    LONG    lDstLeft;
    LONG    lPitch = dwSrcPitch;

    if ((rSrc->left < 0 && rSrc->right < 0)|| 
        (rSrc->top < 0  && rSrc->bottom < 0))
        return; 

    switch (dwDestBitDepth)
    {
        case 8:
        {
            ulDstOffset = (fpDestVidMem - pThisDisplay->dwScreenFlatAddr) >> 0;
            pSrcData   = (BYTE *)(  (ULONG)fpSrcVidMem + 
                                    (ULONG)(rSrc->top * dwSrcPitch) + 
                                    (ULONG)rSrc->left);// Address of (top, left) pixel of Dest Surf
            lOffset    = ((rDst->left & 0x3)-(rSrc->left & 0x3)) & 0x7;
            ulFBReadPixelValue = 0; //From Spec.
            ulPackedPP = PARTIAL_PRODUCTS(dwDestWidth);
            lDstLeft        = (rDst->left >> 2);
            lSrcPackedWidth = ((rSrc->right + 3) >> 2) - (rSrc->left >> 2) ;
            lDstPackedWidth = ((rDst->right + 3) >> 2) - lDstLeft ;
        }
        break;
        case 16:
        {
            ulDstOffset = (fpDestVidMem - pThisDisplay->dwScreenFlatAddr) >> 1;
            pSrcData   = (BYTE *)(  (ULONG)fpSrcVidMem + 
                                    (ULONG)(rSrc->top * dwSrcPitch) + 
                                    (ULONG)(rSrc->left << 1));// Address of (top, left) pixel of Dest Surf
            lOffset    = ((rDst->left & 0x1)-(rSrc->left & 0x1)) & 0x7;
            ulFBReadPixelValue = 1;//From Spec.
            ulPackedPP = PARTIAL_PRODUCTS(dwDestWidth);
            lDstLeft        = (rDst->left >> 1);
            lSrcPackedWidth = ((rSrc->right + 1) >> 1) - (rSrc->left >> 1) ;
            lDstPackedWidth = ((rDst->right + 1) >> 1) - lDstLeft ;
        }
        break;
        case 32:
        {
            ulDstOffset = (fpDestVidMem - pThisDisplay->dwScreenFlatAddr)>>2;
            pSrcData   = (BYTE *)(  (ULONG)fpSrcVidMem + 
                                    (ULONG)(rSrc->top * dwSrcPitch) + 
                                    (ULONG)(rSrc->left << 2));// Address of (top, left) pixel of Dest Surf
            lOffset    = 0;
            ulFBReadPixelValue = 2;//From Spec.
            ulPackedPP = PARTIAL_PRODUCTS(dwDestWidth);
            lDstLeft        = rDst->left;
            lSrcPackedWidth = rSrc->right - rSrc->left;
            lDstPackedWidth = rDst->right - lDstLeft;
        }
        break;
        default:
        return;
    }

    pSrcData   = (BYTE *)((ULONG)(pSrcData) & ~0x3);    //DWORD aligned source address
    lScanLines = rDst->bottom - rDst->top;
    if (lSrcPackedWidth < lDstPackedWidth)
        lPackedWidth = lDstPackedWidth;
    else
        lPackedWidth = lSrcPackedWidth;

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    vSyncWithPermedia(pThisDisplay); 
    SymMemToVidMemPkdDwnldRegInit(pThisDisplay);

    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 12);                                            
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadPixel, ulFBReadPixelValue);                    

    // No logical ops in SYS->VIDMEM Blits
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LogicalOpMode, 0);                                         

    // Load up the partial products of image
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadMode,  (ulPackedPP |
                                                        PM_FBREADMODE_PACKEDDATA(__PERMEDIA_ENABLE)|
                                                        PM_FBREADMODE_RELATIVEOFFSET(lOffset)) );    

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWindowBase,  ulDstOffset);                                

    // Use left to right and top to bottom
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom,       INTtoFIXED(lDstLeft));                    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub,       INTtoFIXED(lDstLeft+lPackedWidth));       
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, PackedDataLimits,(PM_PACKEDDATALIMITS_OFFSET(lOffset) | 
                                                            (rDst->left << 16) | 
                                                            rDst->right));                           
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY,          INTtoFIXED(rDst->top));                 
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,              INTtoFIXED(1));                         
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Count,           (rDst->bottom - rDst->top));            
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Render,          (__RENDER_TRAPEZOID_PRIMITIVE | 
                                                             __RENDER_SYNC_ON_HOST_DATA));            
    // Now start sending the data
    if (lSrcPackedWidth < lDstPackedWidth)
    {
        while (lScanLines--)
        {
            WAIT_FOR_FREE_ENTRIES(pThisDisplay, 1);                                            
            LOAD_INPUT_FIFO_DATA(pThisDisplay, (__Permedia2TagColor | ((lPackedWidth-1) << 16)));
            LoadInputFifoNData(pThisDisplay, pSrcData, (lPackedWidth-1));
            WAIT_FOR_FREE_ENTRIES(pThisDisplay, 1);                                            
            LOAD_INPUT_FIFO_DATA(pThisDisplay, 0x00000000); //Dummy DWORD
            pSrcData = pSrcData + lPitch; 
        }
    }        
    else
    {
        while (lScanLines--)
        {
            WAIT_FOR_FREE_ENTRIES(pThisDisplay, 1);                                            
            LOAD_INPUT_FIFO_DATA(pThisDisplay, (__Permedia2TagColor | ((lPackedWidth-1) << 16)));
            LoadInputFifoNData(pThisDisplay, pSrcData, lPackedWidth);
            pSrcData = pSrcData + lPitch;    
        }
    }

    SymMemToVidMemPkdDwnldRegRestore(pThisDisplay);
    START_SOFTWARE_CURSOR(pThisDisplay);

} // SysMemToVidMemPackedDownload 

/*
 * Function:    VidMemFastFill
 * Description: Fills each each pixel in the surface with dwColor 
 *              when dwWriteMask == 0xffffffff
 *              Otherwise dwWriteMask describes the bit mask for
 *              the pixel values, for the number of pixels which
 *              which can fit in a DWORD. dwWriteMask effectively 
 *              becomes the stencil.
 */

VOID    VidMemFastFill(LPP2THUNKEDDATA  pThisDisplay, 
                       FLATPTR          fpDestVidMem,
                       RECTL            *rDest,
                       DWORD            dwColor,
                       DWORD            dwDestBitDepth,
                       DWORD            dwDestWidth,
                       DWORD            dwWriteMask)
{
    ULONG   ulPackedPP;
    ULONG   ulFBReadPixelValue; 
    DWORD   dwWindowBase;

    DBG_DD(( 5, "DDraw:PermediaFastClear"));

    ulPackedPP = PARTIAL_PRODUCTS(dwDestWidth);

    switch (dwDestBitDepth)
    {
        case 8:
            dwColor &= 0xFF;
            dwColor |= dwColor << 8;
            dwColor |= (dwColor << 16);
            ulFBReadPixelValue = 0;//From Spec.
            dwWindowBase = (fpDestVidMem - pThisDisplay->dwScreenFlatAddr) >> 0;
        break;
        case 16:
            dwColor &= 0xFFFF;
            dwColor |= (dwColor << 16);
            ulFBReadPixelValue = 1;//From Spec.
            dwWindowBase = (fpDestVidMem - pThisDisplay->dwScreenFlatAddr) >> 1;
        break;
        case 32:
            ulFBReadPixelValue = 2;//From Spec.
            dwWindowBase = (fpDestVidMem - pThisDisplay->dwScreenFlatAddr) >> 2;
        break;
        default:
        return;
    }

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    vSyncWithPermedia(pThisDisplay); 
    VidMemFastFillRegInit(pThisDisplay);

    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 20);                                            

    if (dwWriteMask != 0xFFFFFFFF)
    {
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBHardwareWriteMask, dwWriteMask);
    }

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadPixel, ulFBReadPixelValue);

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBBlockColor, dwColor);

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadMode,    
                                    (PM_FBREADMODE_PARTIAL(ulPackedPP)|
                                    PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE)));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LogicalOpMode, __PERMEDIA_CONSTANT_FB_WRITE);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWindowBase,  dwWindowBase);

    // Render the rectangle
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXDom, 0x0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXSub, 0x0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom, INTtoFIXED(rDest->left));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub, INTtoFIXED(rDest->right));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY,    INTtoFIXED(rDest->top));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,        INTtoFIXED(1));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Count,     (rDest->bottom - rDest->top));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Render, 
                                           (__RENDER_FAST_FILL_ENABLE | __RENDER_TRAPEZOID_PRIMITIVE));

    if (dwWriteMask != 0xFFFFFFFF)
    {
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBHardwareWriteMask, 0xFFFFFFFF);
    }

    VidMemFastFillRegRestore(pThisDisplay);
    START_SOFTWARE_CURSOR(pThisDisplay);
}   //VidMemFastFill

/*
 * Function:    VidMemYUVtoRGB
 * Description: Permedia2 YUV to RGB conversion blt
 */


VOID 
VidMemYUVtoRGB( LPP2THUNKEDDATA             pThisDisplay, 
                LPDDRAWI_DDRAWSURFACE_LCL   pSrcLcl, 
                LPDDRAWI_DDRAWSURFACE_LCL   pDestLcl, 
                RECTL                       *rSrc, 
                RECTL                       *rDest)
{
    ULONG               ulFBReadPixelValue; 
    DWORD               dwWindowBase, dwSourceOffset;
    LONG                lXScale;
    LONG                lYScale;
    DDPIXELFORMAT       *pPixFormat;
    P2_SURFACE_FORMAT   P2DestFormat;

    if (pDestLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        pPixFormat = &pDestLcl->lpGbl->ddpfSurface;
    }
    else
    {
        pPixFormat = &pDestLcl->lpGbl->lpDD->vmiData.ddpfDisplay;
    }
      
    lXScale = ((rSrc->right - rSrc->left) << 20) / (rDest->right - rDest->left);
    lYScale = ((rSrc->bottom - rSrc->top) << 20) / (rDest->bottom - rDest->top);

    GetP2SurfaceFormat(pPixFormat, &P2DestFormat);    
    ulFBReadPixelValue  = P2DestFormat.dwP2PixelSize;
    ASSERTDD(((ulFBReadPixelValue == 0)||(ulFBReadPixelValue == 1)||(ulFBReadPixelValue == 2)), "P2PixelSize Invalid");

    dwWindowBase        = GetP2VideoMemoryOffset(pThisDisplay, pDestLcl->lpGbl->fpVidMem, ulFBReadPixelValue);

    if (pSrcLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
    {
        dwSourceOffset = GetP2AGPMemoryOffset(pThisDisplay, pSrcLcl->lpGbl->fpVidMem, ulFBReadPixelValue);
    }
    else
    {
        dwSourceOffset = GetP2VideoMemoryOffset(pThisDisplay, pSrcLcl->lpGbl->fpVidMem, ulFBReadPixelValue);
    }

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    vSyncWithPermedia(pThisDisplay); 
    VidMemYUVtoRGBRegInit(pThisDisplay);

    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 29);                                            

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadPixel, ulFBReadPixelValue);
    
    //Since it is YUV Source its bitdepth is 16 
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode, 
        (P2DestFormat.dwP2ColorOrder << PM_DITHERMODE_COLORORDER) | 
        (P2DestFormat.dwP2Format << PM_DITHERMODE_COLORFORMAT) |
        (P2DestFormat.dwP2FormatExt << PM_DITHERMODE_COLORFORMATEXTENSION) |
        (1 << PM_DITHERMODE_ENABLE) |
        (1 << PM_DITHERMODE_DITHERENABLE));
    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWindowBase, dwWindowBase);
    
    // set no read of source.
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadMode, (PARTIAL_PRODUCTS(pDestLcl->lpGbl->wWidth)));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LogicalOpMode, __PERMEDIA_DISABLE);
    
    // set base of source
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureBaseAddress, dwSourceOffset);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureAddressMode,(1 << PM_TEXADDRESSMODE_ENABLE));
    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay,  TextureColorMode,
                        (1 << PM_TEXCOLORMODE_ENABLE) |
                        (_P2_TEXTURE_COPY << PM_TEXCOLORMODE_APPLICATION));
    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay,  TextureReadMode, 
                        PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                        PM_TEXREADMODE_FILTER(__PERMEDIA_ENABLE) |
                        PM_TEXREADMODE_WIDTH(11) |
                        PM_TEXREADMODE_HEIGHT(11) );
    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay,  TextureDataFormat, 
                        (PERMEDIA_YUV422 << PM_TEXDATAFORMAT_FORMAT) |
                        (PERMEDIA_YUV422_EXTENSION << PM_TEXDATAFORMAT_FORMATEXTENSION) |
                        (INV_COLOR_MODE << PM_TEXDATAFORMAT_COLORORDER));
    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay,  TextureMapFormat,    
                        (PARTIAL_PRODUCTS(pSrcLcl->lpGbl->wWidth)) | 
                        (1 << PM_TEXMAPFORMAT_TEXELSIZE) );
    
    // Turn on the YUV unit
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, YUVMode, 0x1);    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LogicalOpMode, 0);
    
    // set offset of source
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, SStart,    rSrc->left << 20);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TStart,    rSrc->top << 20);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdx,      (DWORD)(lXScale));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdyDom,   0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdx,      0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdyDom, (lYScale));
    
    
    // Render the rectangle
    //
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom, INTtoFIXED(rDest->left));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub, INTtoFIXED(rDest->right));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY,    INTtoFIXED(rDest->top));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,        INTtoFIXED(1));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Count,     (rDest->bottom - rDest->top));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Render,    (__RENDER_TRAPEZOID_PRIMITIVE | 
                                                        __RENDER_TEXTURED_PRIMITIVE));
       
    // Turn off units
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode, 0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, YUVMode, 0x0);

    VidMemYUVtoRGBRegRestore(pThisDisplay);
    START_SOFTWARE_CURSOR(pThisDisplay);
}

/*
 * Function:    VidMemtoVidMemPackedCopyBlt
 * Description: Does a blt rectangular region from a source surface to 
 *              to a rectangular region of identical size in the destination surface.
 *              The source and destination surfaces have the same pixel format,
 *              but may not be of the same size.
 */

VOID 
VidMemtoVidMemPackedCopyBlt(LPP2THUNKEDDATA             pThisDisplay, 
                            LPDDRAWI_DDRAWSURFACE_LCL   lpSrcLcl, 
                            LPDDRAWI_DDRAWSURFACE_LCL   lpDestLcl, 
                            RECTL                       *rSrc, 
                            RECTL                       *rDest)
{
    ULONG   ulFBReadPixelValue; 
    ULONG   ulDestBitDepth;
    LONG    lOffset;
    LONG    lSourceOffset, lWindowOffset;
    DWORD   dwWindowBase;
    DWORD   dwDestPitchInPixels;               
    DWORD   dwSourcePitchInPixels;
    ULONG   ulPixelMask;
    ULONG   ulPixelShift;
    
    DBG_DD(( 5, "DDraw:PermediaPackedCopyBlt"));

    if (((rSrc->top < 0) && (rSrc->bottom < 0))|| 
        ((rSrc->left < 0) && (rSrc->right < 0)))
        return;
    if (rSrc->top < 0) {
        rDest->top -= rSrc->top;
        rSrc->top = 0;
    }
    if (rSrc->left < 0) {
        rDest->left -= rSrc->left;
        rSrc->left = 0;
    }

    ulDestBitDepth = DDSurf_BitDepth(lpDestLcl);

    switch (ulDestBitDepth)
    {
        case 8:
            ulFBReadPixelValue = 0;//From Spec.
            lOffset         = ((rDest->left & 0x3)-(rSrc->left & 0x3)) & 0x7;
            dwWindowBase    = GetP2VideoMemoryOffset(pThisDisplay, lpDestLcl->lpGbl->fpVidMem, 0);
            lWindowOffset   = (lpSrcLcl->lpGbl->fpVidMem - lpDestLcl->lpGbl->fpVidMem) >> 0;
            ulPixelMask     = 3;
            ulPixelShift    = 2; 
            dwDestPitchInPixels = lpDestLcl->lpGbl->lPitch;  
            dwSourcePitchInPixels = lpSrcLcl->lpGbl->lPitch;
        break;
        case 16:
            ulFBReadPixelValue = 1;//From Spec.
            lOffset         = ((rDest->left & 0x1)-(rSrc->left & 0x1)) & 0x7;
            dwWindowBase    = GetP2VideoMemoryOffset(pThisDisplay, lpDestLcl->lpGbl->fpVidMem, 1);
            lWindowOffset   = (lpSrcLcl->lpGbl->fpVidMem - lpDestLcl->lpGbl->fpVidMem) >> 1;
            ulPixelMask     = 1;
            ulPixelShift    = 1; 
            dwDestPitchInPixels = lpDestLcl->lpGbl->lPitch >> 1;  
            dwSourcePitchInPixels = lpSrcLcl->lpGbl->lPitch >> 1;
      break;
        case 32:
            ulFBReadPixelValue = 2;//From Spec.
            lOffset         = 0;
            dwWindowBase    = GetP2VideoMemoryOffset(pThisDisplay, lpDestLcl->lpGbl->fpVidMem, 2);
            lWindowOffset   = (lpSrcLcl->lpGbl->fpVidMem - lpDestLcl->lpGbl->fpVidMem) >> 2;
            ulPixelMask     = 0;
            ulPixelShift    = 0; 
            dwDestPitchInPixels     = lpDestLcl->lpGbl->lPitch >> 2;  
            dwSourcePitchInPixels   = lpSrcLcl->lpGbl->lPitch >> 2;
        break;
        default:
        return;
    }

    lSourceOffset   = lWindowOffset + 
                        RECTS_PIXEL_OFFSET(rSrc, rDest,
                                           dwSourcePitchInPixels, dwDestPitchInPixels, 
                                           ulPixelMask ) + 
                        LINEAR_FUDGE(dwSourcePitchInPixels, dwDestPitchInPixels, rDest);

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    vSyncWithPermedia(pThisDisplay); 
    VMemToVMemPackedCopyBltRegInit(pThisDisplay);

    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 16);                                            

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadPixel, ulFBReadPixelValue);

    // set packed with offset
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWindowBase, dwWindowBase);

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadMode,  
                        PM_FBREADMODE_PARTIAL((PARTIAL_PRODUCTS(lpSrcLcl->lpGbl->wWidth))) |
                        PM_FBREADMODE_READSOURCE(__PERMEDIA_ENABLE)        |
                        PM_FBREADMODE_PACKEDDATA(__PERMEDIA_ENABLE)        |
                        PM_FBREADMODE_RELATIVEOFFSET(lOffset));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWriteConfig,   
                        PM_FBREADMODE_PARTIAL((PARTIAL_PRODUCTS(lpDestLcl->lpGbl->wWidth))) |
                        PM_FBREADMODE_READSOURCE(__PERMEDIA_ENABLE)        |
                        PM_FBREADMODE_PACKEDDATA(__PERMEDIA_ENABLE)        |
                        PM_FBREADMODE_RELATIVEOFFSET(lOffset));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LogicalOpMode, __PERMEDIA_DISABLE);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBSourceOffset, lSourceOffset);

    // Render the rectangle
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXDom,               0x0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dXSub,               0x0);

    if (lSourceOffset >= 0) 
    {
        // Use left to right and top to bottom
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom, 
                            INTtoFIXED(rDest->left >> ulPixelShift));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub, 
                            INTtoFIXED((rDest->right >> ulPixelShift) + ulPixelMask));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, PackedDataLimits,    
                            PM_PACKEDDATALIMITS_OFFSET(lOffset)      |
                            PM_PACKEDDATALIMITS_XSTART(rDest->left) |
                            PM_PACKEDDATALIMITS_XEND(rDest->right));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY, INTtoFIXED(rDest->top));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY, INTtoFIXED(1));
    }
    else 
    {
        // Use right to left and bottom to top
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom, 
                        INTtoFIXED((rDest->right >> ulPixelShift) + ulPixelMask));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub, 
                        INTtoFIXED(rDest->left >> ulPixelShift));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, PackedDataLimits,    
                                            PM_PACKEDDATALIMITS_OFFSET(lOffset)       |
                                            PM_PACKEDDATALIMITS_XSTART(rDest->right) |
                                            PM_PACKEDDATALIMITS_XEND(rDest->left));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY, INTtoFIXED(rDest->bottom - 1));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY, (DWORD)INTtoFIXED(-1));
    }

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Count, (rDest->bottom - rDest->top));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Render, __RENDER_TRAPEZOID_PRIMITIVE);

    VMemToVMemPackedCopyBltRegRestore(pThisDisplay);
    START_SOFTWARE_CURSOR(pThisDisplay);

}   //  VidMemtoVidMemPackedCopyBlt 

/*
 * Function:    VidMemToVidMemStretchCopyBlt
 * Description: Does a blt rectangular region from a source surface to 
 *              to a rectangular region in the destination surface. The source
 *              and destination rectangles may not be of the same size.
 *              The source and destination surfaces have the same pixel format,
 *              but may not be of the same size. Mirroring is performed if required.
 */

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
                             BOOL               bYMirror)
                                
{
    LONG                lXScale;
    LONG                lYScale;
    DWORD               dwTexSStart, dwTexTStart;
    P2_SURFACE_FORMAT   P2DestFormat;
    P2_SURFACE_FORMAT   P2SrcFormat;
    ULONG               ulFBReadPixelValue, ulP2SrcPixelSize; 
    DWORD               dwWindowBase, dwSourceOffset;

    DBG_DD(( 5, "DDraw:PermediaStretchCopyBlt"));

    lXScale = ((rSrc->right - rSrc->left) << 20) / (rDest->right - rDest->left);
    lYScale = ((rSrc->bottom - rSrc->top) << 20) / (rDest->bottom - rDest->top);
    
    GetP2SurfaceFormat(pDestPixFormat, &P2DestFormat);

    ulFBReadPixelValue  = P2DestFormat.dwP2PixelSize;
    ASSERTDD(((ulFBReadPixelValue == 0)||(ulFBReadPixelValue == 1)||(ulFBReadPixelValue == 2)), "P2PixelSize Invalid");

    dwWindowBase        = GetP2VideoMemoryOffset(pThisDisplay, fpDestVidMem, ulFBReadPixelValue);

    GetP2SurfaceFormat(pSrcPixFormat, &P2SrcFormat);
    ulP2SrcPixelSize  = P2SrcFormat.dwP2PixelSize;
    ASSERTDD(((ulP2SrcPixelSize == 0)||(ulP2SrcPixelSize == 1)||(ulP2SrcPixelSize == 2)), "P2PixelSize Invalid");

    if (bSrcInNonLocalVidMem)
    {
        dwSourceOffset = GetP2AGPMemoryOffset(pThisDisplay, fpSrcVidMem, ulP2SrcPixelSize);
    }
    else
    {
        dwSourceOffset = GetP2VideoMemoryOffset(pThisDisplay, fpSrcVidMem, ulP2SrcPixelSize);
    }

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    vSyncWithPermedia(pThisDisplay); 
    VidMemToVidMemStretchCopyBltRegInit(pThisDisplay);

    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 27);                

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadPixel, ulFBReadPixelValue);

    if (ulFBReadPixelValue != 0)
    {   // set writeback to dest surface...
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay,  DitherMode,  
                            ((P2DestFormat.dwP2ColorOrder << PM_DITHERMODE_COLORORDER) | 
                             (P2DestFormat.dwP2Format << PM_DITHERMODE_COLORFORMAT) |
                             (P2DestFormat.dwP2FormatExt << PM_DITHERMODE_COLORFORMATEXTENSION) |
                             (__PERMEDIA_ENABLE << PM_DITHERMODE_ENABLE))); 

    } 

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWindowBase, dwWindowBase);

    // set no read of dest.
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadMode, (PARTIAL_PRODUCTS(dwDestWidth)));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LogicalOpMode, __PERMEDIA_DISABLE);

    // set base of source
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureBaseAddress, dwSourceOffset);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureAddressMode,(1 << PM_TEXADDRESSMODE_ENABLE));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureColorMode,((1 << PM_TEXCOLORMODE_ENABLE) |
                                                             (_P2_TEXTURE_COPY << PM_TEXCOLORMODE_APPLICATION)));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureReadMode,
                        (PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE)|
                         PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE)|
                         PM_TEXREADMODE_WIDTH(11) |
                         PM_TEXREADMODE_HEIGHT(11) ));

    // set source bitmap format
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureDataFormat, 
                        (P2SrcFormat.dwP2Format << PM_TEXDATAFORMAT_FORMAT) |
                        (P2SrcFormat.dwP2FormatExt << PM_TEXDATAFORMAT_FORMATEXTENSION) |
                        (P2SrcFormat.dwP2ColorOrder << PM_TEXDATAFORMAT_COLORORDER));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureMapFormat, ((PARTIAL_PRODUCTS(dwSrcWidth)) | 
                                         (ulP2SrcPixelSize << PM_TEXMAPFORMAT_TEXELSIZE)));

    // If we are doing special effects, and we are mirroring, 
    // we need to fix up the rectangles and change the sense of 
    // the render operation - we need to be carefull with overlapping
    // rectangles

    if (bXMirror)        
    {
        dwTexSStart = rSrc->right - 1;
        lXScale = -lXScale;
    }   
    else
    {
        dwTexSStart = rSrc->left;
    }

    if (bYMirror)        
    {
        dwTexTStart = rSrc->bottom - 1;
        lYScale = -lYScale;
    }
    else
    {
        dwTexTStart = rSrc->top;
    }

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, SStart,      (dwTexSStart << 20));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TStart,      (dwTexTStart << 20));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdx,        lXScale);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdyDom,     0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdx,        0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdyDom,     lYScale);
    
    // Render the rectangle
    
    if (dwRenderDirection)
    {
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom, INTtoFIXED(rDest->left));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub, INTtoFIXED(rDest->right));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY,    INTtoFIXED(rDest->top));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,        INTtoFIXED(1));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Count,     (rDest->bottom - rDest->top));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Render,    (__RENDER_TRAPEZOID_PRIMITIVE | 
                                                           __RENDER_TEXTURED_PRIMITIVE));
    } 
    else
    {   // Render right to left, bottom to top
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom, INTtoFIXED(rDest->right));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub, INTtoFIXED(rDest->left));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY,    INTtoFIXED(rDest->bottom - 1));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,        (DWORD)INTtoFIXED(-1));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Count,     (rDest->bottom - rDest->top));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Render,    (__RENDER_TRAPEZOID_PRIMITIVE | 
                                                           __RENDER_TEXTURED_PRIMITIVE));
    }

    if (ulFBReadPixelValue != 0)
    {  
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode, __PERMEDIA_DISABLE);
    }

    VidMemToVidMemStretchCopyBltRegRestore(pThisDisplay);
    START_SOFTWARE_CURSOR(pThisDisplay);
}   // VidMemToVidMemStretchCopyBlt 

/*
 * Function:    VidMemToVidMemChromaBlt
 * Description: Does a blt rectangular region from a source surface to 
 *              to a rectangular region of identical size in the destination surface.
 *              The source and destination surfaces have the same pixel format,
 *              but may not be of the same size. Chroma Keying is done using the
 *              Source Color Key. We use the Texture unit to perform this operation.
 */

VOID VidMemToVidMemChromaBlt(   LPP2THUNKEDDATA             pThisDisplay, 
                                LPDDRAWI_DDRAWSURFACE_LCL   lpSrcLcl, 
                                LPDDRAWI_DDRAWSURFACE_LCL   lpDestLcl, 
                                RECTL                       *rSrc, 
                                RECTL                       *rDest,
                                LPDDHAL_BLTDATA             lpBlt)
{
    DWORD               dwRenderDirection;
    DDPIXELFORMAT       *pPixFormat;
    P2_SURFACE_FORMAT   P2DestFormat;
    P2_SURFACE_FORMAT   P2SrcFormat;
    DWORD               dwWindowBase, dwSourceOffset;
    ULONG               ulP2DestPixelSize, ulP2SrcPixelSize; 
    DWORD               dwLowerBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue;
    DWORD               dwUpperBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue;

    ASSERTDD(lpDestLcl, "Not valid private surface in destination");
    ASSERTDD(lpSrcLcl, "Not valid private surface in source");

    if (lpDestLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        pPixFormat = &lpDestLcl->lpGbl->ddpfSurface;
    }
    else
    {
        pPixFormat = &lpDestLcl->lpGbl->lpDD->vmiData.ddpfDisplay;
    }
    
    GetP2SurfaceFormat(pPixFormat, &P2DestFormat);
    ulP2DestPixelSize  = P2DestFormat.dwP2PixelSize;
    ASSERTDD(((ulP2DestPixelSize == 0)||(ulP2DestPixelSize == 1)||(ulP2DestPixelSize == 2)), "P2PixelSize Invalid");

    dwWindowBase        = GetP2VideoMemoryOffset(pThisDisplay, lpDestLcl->lpGbl->fpVidMem, ulP2DestPixelSize);

    if (lpSrcLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        pPixFormat = &lpSrcLcl->lpGbl->ddpfSurface;
    }
    else
    {
        pPixFormat = &lpSrcLcl->lpGbl->lpDD->vmiData.ddpfDisplay;
    }

    GetP2SurfaceFormat(pPixFormat, &P2SrcFormat);
    ulP2SrcPixelSize  = P2SrcFormat.dwP2PixelSize;
    ASSERTDD(((ulP2SrcPixelSize == 0)||(ulP2SrcPixelSize == 1)||(ulP2SrcPixelSize == 2)), "P2PixelSize Invalid");

    if (lpSrcLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
    {
        dwSourceOffset = GetP2AGPMemoryOffset(pThisDisplay, lpSrcLcl->lpGbl->fpVidMem, ulP2SrcPixelSize);
    }
    else
    {
        dwSourceOffset = GetP2VideoMemoryOffset(pThisDisplay, lpSrcLcl->lpGbl->fpVidMem, ulP2SrcPixelSize);
    }

    GetP2ChromaLevelBounds(&P2SrcFormat, &dwLowerBound, &dwUpperBound);

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    vSyncWithPermedia(pThisDisplay); 
    VidMemToVidMemChromaBltRegInit(pThisDisplay);

    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 32); 
    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadPixel, ulP2DestPixelSize);

    if (ulP2DestPixelSize != 0)
    {
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode,  
                                (P2DestFormat.dwP2ColorOrder << PM_DITHERMODE_COLORORDER) | 
                                (P2DestFormat.dwP2Format << PM_DITHERMODE_COLORFORMAT) |
                                (P2DestFormat.dwP2FormatExt << PM_DITHERMODE_COLORFORMATEXTENSION) |
                                (1 << PM_DITHERMODE_ENABLE)); 
    }

    // Reject range
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, YUVMode, (PM_YUVMODE_CHROMATEST_FAILWITHIN << 1));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWindowBase, dwWindowBase);

    // set no read of source.
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadMode,
                                                (PARTIAL_PRODUCTS(lpDestLcl->lpGbl->wWidth)));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LogicalOpMode, __PERMEDIA_DISABLE);

    // set base of source
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureBaseAddress, dwSourceOffset);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureAddressMode,(1 << PM_TEXADDRESSMODE_ENABLE));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureColorMode,
                                        ((1 << PM_TEXCOLORMODE_ENABLE) |
                                         (_P2_TEXTURE_COPY << PM_TEXCOLORMODE_APPLICATION)));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureReadMode, 
                                        PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE)|
                                        PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE)|
                                        PM_TEXREADMODE_WIDTH(11) |
                                        PM_TEXREADMODE_HEIGHT(11) );

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureDataFormat,
                                        (P2SrcFormat.dwP2Format << PM_TEXDATAFORMAT_FORMAT) |
                                        (P2SrcFormat.dwP2FormatExt << PM_TEXDATAFORMAT_FORMATEXTENSION) |
                                        (P2SrcFormat.dwP2ColorOrder << PM_TEXDATAFORMAT_COLORORDER));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureMapFormat,
                                        (PARTIAL_PRODUCTS(lpSrcLcl->lpGbl->wWidth)) | 
                                         (ulP2SrcPixelSize << PM_TEXMAPFORMAT_TEXELSIZE) );


    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ChromaLowerBound, dwLowerBound);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ChromaUpperBound, dwUpperBound);

    dwRenderDirection = GetRenderDirection(lpSrcLcl->lpGbl->fpVidMem,
                                           lpDestLcl->lpGbl->fpVidMem,
                                           rSrc,
                                           rDest);

    // Render the rectangle 
    if (dwRenderDirection)
    {   // Left -> right, top->bottom
        // set offset of source
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, SStart,      (rSrc->left << 20));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TStart,      (rSrc->top << 20));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdx,        (1 << 20));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdyDom,     0);
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdx,        0);
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdyDom,     (1 << 20));

        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom,   INTtoFIXED(rDest->left));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub,   INTtoFIXED(rDest->right));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY,      INTtoFIXED(rDest->top));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,          INTtoFIXED(1));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Count,       (rDest->bottom - rDest->top));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Render,      (__RENDER_TRAPEZOID_PRIMITIVE | 
                                         __RENDER_TEXTURED_PRIMITIVE));
    }
    else    
    {   // right->left, bottom->top
        // set offset of source
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, SStart,      (rSrc->right << 20));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TStart,      ((rSrc->bottom - 1) << 20));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdx,        (DWORD)(-1 << 20));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdyDom,     0);
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdx,        0);
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdyDom,     (DWORD)(-1 << 20));

        // Render right to left, bottom to top
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom,   INTtoFIXED(rDest->right));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub,   INTtoFIXED(rDest->left));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY,      INTtoFIXED(rDest->bottom - 1));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,          (DWORD)INTtoFIXED(-1));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Count,       (rDest->bottom - rDest->top));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Render,      (__RENDER_TRAPEZOID_PRIMITIVE | 
                                         __RENDER_TEXTURED_PRIMITIVE));
    }

    if (ulP2DestPixelSize != 0)
    {
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode, 0);
    }

    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, YUVMode, 0x0);// Turn off chroma key
    VidMemToVidMemChromaBltRegRestore(pThisDisplay);
    START_SOFTWARE_CURSOR(pThisDisplay);
} //VidMemToVidMemChromaBlt

/*
 * Function:    VidMemToVidMemStretchChromaBlt
 * Description: Does a blt rectangular region from a source surface to 
 *              to a rectangular region in the destination surface. The source
 *              and destination rectangles may not be of the same size.
 *              The source and destination surfaces have the same pixel format,
 *              but may not be of the same size. Mirroring is performed if required.
 *              Chroma Keying is done using the Source Color Key. 
 *              We use the Texture unit to perform this operation. 
 */

VOID
VidMemToVidMemStretchChromaBlt( LPP2THUNKEDDATA             pThisDisplay, 
                                LPDDRAWI_DDRAWSURFACE_LCL   lpSrcLcl, 
                                LPDDRAWI_DDRAWSURFACE_LCL   lpDestLcl, 
                                RECTL                       *rSrc, 
                                RECTL                       *rDest,
                                LPDDHAL_BLTDATA             lpBlt)
{
    LONG                lXScale;
    LONG                lYScale;
    DWORD               dwTexSStart, dwTexTStart;
    DWORD               dwRenderDirection;
    DDPIXELFORMAT       *pPixFormat;
    P2_SURFACE_FORMAT   P2DestFormat;
    P2_SURFACE_FORMAT   P2SrcFormat;
    DWORD               dwWindowBase, dwSourceOffset;
    ULONG               ulP2DestPixelSize, ulP2SrcPixelSize;     
    BOOL                bXMirror = FALSE;
    BOOL                bYMirror = FALSE;
    DWORD               dwLowerBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue;
    DWORD               dwUpperBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue;

    lXScale = ((rSrc->right - rSrc->left) << 20) / (rDest->right - rDest->left);
    lYScale = ((rSrc->bottom - rSrc->top) << 20) / (rDest->bottom - rDest->top);

    DBG_DD(( 5, "DDraw:PermediaStretchCopyChromaBlt"));

    ASSERTDD(lpDestLcl, "Not valid private surface in destination");
    ASSERTDD(lpSrcLcl, "Not valid private surface in source");
    
    if (lpDestLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        pPixFormat = &lpDestLcl->lpGbl->ddpfSurface;
    }
    else
    {
        pPixFormat = &lpDestLcl->lpGbl->lpDD->vmiData.ddpfDisplay;
    }
    
    GetP2SurfaceFormat(pPixFormat, &P2DestFormat);
    ulP2DestPixelSize  = P2DestFormat.dwP2PixelSize;
    ASSERTDD(((ulP2DestPixelSize == 0)||(ulP2DestPixelSize == 1)||(ulP2DestPixelSize == 2)), "P2PixelSize Invalid");

    dwWindowBase        = GetP2VideoMemoryOffset(pThisDisplay, lpDestLcl->lpGbl->fpVidMem, ulP2DestPixelSize);

    if (lpSrcLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        pPixFormat = &lpSrcLcl->lpGbl->ddpfSurface;
    }
    else
    {
        pPixFormat = &lpSrcLcl->lpGbl->lpDD->vmiData.ddpfDisplay;
    }

    GetP2SurfaceFormat(pPixFormat, &P2SrcFormat);
    ulP2SrcPixelSize  = P2SrcFormat.dwP2PixelSize;
    ASSERTDD(((ulP2SrcPixelSize == 0)||(ulP2SrcPixelSize == 1)||(ulP2SrcPixelSize == 2)), "P2PixelSize Invalid");

    if (lpSrcLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
    {
        dwSourceOffset = GetP2AGPMemoryOffset(pThisDisplay, lpSrcLcl->lpGbl->fpVidMem, ulP2SrcPixelSize);
    }
    else
    {
        dwSourceOffset = GetP2VideoMemoryOffset(pThisDisplay, lpSrcLcl->lpGbl->fpVidMem, ulP2SrcPixelSize);
    }

    GetP2ChromaLevelBounds(&P2SrcFormat, &dwLowerBound, &dwUpperBound);

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    vSyncWithPermedia(pThisDisplay); 
    VidMemToVidMemStretchChromaBltRegInit(pThisDisplay);

    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 32);
    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadPixel, ulP2DestPixelSize);

    if (ulP2DestPixelSize != 0)
    {   // set writeback to dest surface...
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode,  
                                (P2DestFormat.dwP2ColorOrder << PM_DITHERMODE_COLORORDER) | 
                                (P2DestFormat.dwP2Format << PM_DITHERMODE_COLORFORMAT) |
                                (P2DestFormat.dwP2FormatExt << PM_DITHERMODE_COLORFORMATEXTENSION) |
                                (1 << PM_DITHERMODE_ENABLE)); 
    } 

    // Reject range
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, YUVMode, PM_YUVMODE_CHROMATEST_FAILWITHIN <<1);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBWindowBase, dwWindowBase);

    // set no read of source.
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, FBReadMode, (PARTIAL_PRODUCTS(lpDestLcl->lpGbl->wWidth)));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, LogicalOpMode, __PERMEDIA_DISABLE);

    // set base of source
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureBaseAddress, dwSourceOffset);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TextureAddressMode,(1 << PM_TEXADDRESSMODE_ENABLE));
    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay,  TextureColorMode,
                        (1 << PM_TEXCOLORMODE_ENABLE) |
                        (_P2_TEXTURE_COPY << PM_TEXCOLORMODE_APPLICATION));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay,  TextureReadMode, 
                        PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                        PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
                        PM_TEXREADMODE_WIDTH(11) |
                        PM_TEXREADMODE_HEIGHT(11));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay,  TextureDataFormat,
                        (P2SrcFormat.dwP2Format << PM_TEXDATAFORMAT_FORMAT) |
                        (P2SrcFormat.dwP2FormatExt << PM_TEXDATAFORMAT_FORMATEXTENSION) |
                        (P2SrcFormat.dwP2ColorOrder << PM_TEXDATAFORMAT_COLORORDER));

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay,  TextureMapFormat, 
                        (PARTIAL_PRODUCTS(lpSrcLcl->lpGbl->wWidth)) | 
                        (ulP2SrcPixelSize << PM_TEXMAPFORMAT_TEXELSIZE) );

    dwRenderDirection = GetRenderDirection(lpSrcLcl->lpGbl->fpVidMem,
                                           lpDestLcl->lpGbl->fpVidMem,
                                           rSrc,
                                           rDest);

    bXMirror          = GetXMirror(lpBlt, dwRenderDirection);
    bYMirror          = GetYMirror(lpBlt, dwRenderDirection);

    if (bXMirror)        
    {
        dwTexSStart = rSrc->right - 1;
        lXScale = -lXScale;
    }   
    else
    {
        dwTexSStart = rSrc->left;
    }

    if (bYMirror)        
    {
        dwTexTStart = rSrc->bottom - 1;
        lYScale = -lYScale;
    }
    else
    {
        dwTexTStart = rSrc->top;
    }

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdyDom, lYScale);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ChromaLowerBound, dwLowerBound);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ChromaUpperBound, dwUpperBound);

    // set texture coordinates
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, SStart,      (dwTexSStart << 20));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, TStart,      (dwTexTStart << 20));
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdx,        lXScale);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dSdyDom,     0);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dTdx,        0);

    //
    // Render the rectangle
    //

    if (dwRenderDirection)
    {
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom, INTtoFIXED(rDest->left));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub, INTtoFIXED(rDest->right));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY,    INTtoFIXED(rDest->top));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,        INTtoFIXED(1));
    }
    else
    {
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXDom, INTtoFIXED(rDest->right));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartXSub, INTtoFIXED(rDest->left));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, StartY,    INTtoFIXED(rDest->bottom - 1));
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, dY,        (DWORD)INTtoFIXED(-1));
    }

    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Count,   rDest->bottom - rDest->top);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, Render,  (__RENDER_TRAPEZOID_PRIMITIVE | 
                                                     __RENDER_TEXTURED_PRIMITIVE));


    // Turn off units
    if (ulP2DestPixelSize != 0)
    {
        LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, DitherMode, 0);
    }

    
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, YUVMode, 0x0);// Turn off chroma key
    VidMemToVidMemStretchChromaBltRegRestore(pThisDisplay);
    START_SOFTWARE_CURSOR(pThisDisplay);
}//VidMemToVidMemStretchChromaBlt
