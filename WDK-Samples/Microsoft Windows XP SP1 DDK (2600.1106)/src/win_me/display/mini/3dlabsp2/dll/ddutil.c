/******************************Module*Header**********************************\
*
*       ****************************************
*       * Permedia 2: Direct Draw  SAMPLE CODE *
*       ****************************************
*
* Module Name: ddutil.c
*
*       Direct Draw Utility functions
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

extern DDSURFACEDESC gD3DTextureFormats [];
extern ULONG gD3DNumberOfTextureFormats;

/*
 * Function:    bComparePixelFormat
 * Description: Function used to compare 2 pixels formats for equality. This is a 
 *              helper function to bCheckTextureFormat. A return value of TRUE 
 *              indicates equality
 */

BOOL 
bComparePixelFormat(LPDDPIXELFORMAT lpddpf1, 
                    LPDDPIXELFORMAT lpddpf2)
{
    if (lpddpf1->dwFlags != lpddpf2->dwFlags)
    {
        return FALSE;
    }

    // same bitcount for non-YUV surfaces?
    if (!(lpddpf1->dwFlags & (DDPF_YUV | DDPF_FOURCC)))
    {
        if (lpddpf1->dwRGBBitCount != lpddpf2->dwRGBBitCount )
        {
            return FALSE;
        }
    }

    // same RGB properties?
    if (lpddpf1->dwFlags & DDPF_RGB)
    {
        if ((lpddpf1->dwRBitMask != lpddpf2->dwRBitMask) ||
            (lpddpf1->dwGBitMask != lpddpf2->dwGBitMask) ||
            (lpddpf1->dwBBitMask != lpddpf2->dwBBitMask) ||
            (lpddpf1->dwRGBAlphaBitMask != lpddpf2->dwRGBAlphaBitMask))
        { 
             return FALSE;
        }
    }
    
    // same YUV properties?
    if (lpddpf1->dwFlags & DDPF_YUV)    
    {
        if ((lpddpf1->dwFourCC != lpddpf2->dwFourCC) ||
            (lpddpf1->dwYUVBitCount != lpddpf2->dwYUVBitCount) ||
            (lpddpf1->dwYBitMask != lpddpf2->dwYBitMask) ||
            (lpddpf1->dwUBitMask != lpddpf2->dwUBitMask) ||
            (lpddpf1->dwVBitMask != lpddpf2->dwVBitMask) ||
            (lpddpf1->dwYUVAlphaBitMask != lpddpf2->dwYUVAlphaBitMask))
        {
             return FALSE;
        }
    }
    else if (lpddpf1->dwFlags & DDPF_FOURCC)
    {
        if (lpddpf1->dwFourCC != lpddpf2->dwFourCC)
        {
            return FALSE;
        }
    }

    // If Interleaved Z then check Z bit masks are the same
    if (lpddpf1->dwFlags & DDPF_ZPIXELS)
    {
        if (lpddpf1->dwRGBZBitMask != lpddpf2->dwRGBZBitMask)
        {
            return FALSE;
        }
    }

    return TRUE;
} // bComparePixelFormat

/**
 * Function:    bCheckTextureFormat
 * Description: Function used to determine if a texture format is supported. 
 *              It traverses the gD3DTextureFormats array. Return value of 
 *              TRUE indicates that we do support the requested texture format.
 */

BOOL 
bCheckTextureFormat(LPDDPIXELFORMAT lpddpf)
{
    DWORD i;

    // Run the list for a matching format
    for (i=0; i < gD3DNumberOfTextureFormats; i++)
    {
        if (bComparePixelFormat(lpddpf, 
                                &gD3DTextureFormats[i].ddpfPixelFormat))
        {
            return TRUE;
        }   
    }

    return FALSE;
} // bCheckTextureFormat


