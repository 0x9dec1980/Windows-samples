//--------------------------------------------------------------------------
//
// Module Name:  BRUSH.C
//
// Brief Description:  This module contains the PSCRIPT driver's brush
// realization routines.
//
// Author:  Kent Settle (kentse)
// Created: 13-Dec-1990
//
// Copyright (c) 1990 - 1992 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "pscript.h"
#include "enable.h"

//--------------------------------------------------------------------------
//
// BOOL DrvRealizeBrush(pbo, psoTarget, psoPattern, psoMask, pxlo)
// BRUSHOBJ    *pbo;
// SURFOBJ     *psoTarget;
// SURFOBJ     *psoPattern;
// SURFOBJ     *psoMask;
// XLATEOBJ    *pxlo;
//
// Requests the driver to realize a pattern defined by psoPattern for
// the surface defined by psoTarget.  A realized brush contains the
// information and accelerators the driver needs to fill an area with
// a pattern.  This information is defined by the driver and used only
// by the driver.  The driver's realization of the brush should be
// written into the buffer allocated by calling BRUSHOBJ_pvAllocRBrush.
//
// This function is required for a driver that does any drawing to any
// surface.
//
// Parameters:
//   pbo:
//     Points to the BRUSHOBJ which is being realized.  All the other
//     parameters, except for psoTarget, can actually be queried from
//     this object.  We provide them as an optimization.  pbo is best used
//     only as a parameter for BRUSHOBJ_pvAllocRBrush, which allocates
//     the memory for the realized brush.
//
//   psoTarget:
//     The object for the surface the brush is to be realized for.  This
//     surface will either be the physical surface for the device, or a
//     device format bitmap.
//
//   psoPattern:
//     The surface describing the pattern for the brush.  For a raster
//     device this will always represent a bitmap.  For a vector device
//     this will always be one of the pattern surfaces returned by
//     DrvEnablePDEV.
//
//   psoMask:
//     If this argument is not NULL, it provides a transparency mask for
//     the brush.  This is a one bit per pel bitmap having the same
//     extent as the pattern.  A mask bit of zero means that the pel is
//     considered a background pel for the brush (In transparent
//     background mode, the background pels would be left unaffected in a
//     fill.)
//
//     Plotters can ignore this argument as they are never expected to draw
//     background information.
//
//   pxlo:
//     An XLATEOBJ which tells how to interpret the colors in the pattern.
//     An XLATEOBJ service routine can be called to translate the colors to
//     device color indices.  Vector devices should translate color 0
//     through the XLATEOBJ to get the foreground color for the brush.
//
// Returns:
//     TRUE if the brush was successfully realized.  Otherwise, FALSE
//     and an error code is logged.
//
// History:
//   13-Dec-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//
//  16-Feb-1993 Tue 12:28:06 updated  -by-  Daniel Chou (danielc)
//      Re-write so it takc iHatch rather go through the suface handle to
//      check what type of pattern.
//
//	14-Mar-1995 -davidx-
//		Don't copy color table entries from XLATEOBJ if XO_TABLE bit
//		of flXlate is not set. This prevents us from causing memory
//		access violation.
//
//--------------------------------------------------------------------------

BOOL DrvRealizeBrush(pbo, psoTarget, psoPattern, psoMask, pxlo, iHatch)
BRUSHOBJ    *pbo;
SURFOBJ     *psoTarget;
SURFOBJ     *psoPattern;
SURFOBJ     *psoMask;
XLATEOBJ    *pxlo;
ULONG       iHatch;
{
    ULONG           cbTotal, cbBits, cColorTable;
    PBYTE           pbSrc, pbTarg;
    DEVBRUSH       *pBrush;
    PDEVDATA        pdev;

	// 
	// Declare function name for debugging purposes
	//

	DBGPS_FUNCNAME("pscript!DrvRealizeBrush")

    UNREFERENCED_PARAMETER(psoMask);

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA) psoTarget->dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
    {
		DBGPS(DBGPS_LEVEL_ERROR, ("%s: invalid pdev.\n", _funcname));
		SetLastError(ERROR_INVALID_PARAMETER);
		return(FALSE);
    }

    if (iHatch >= HS_DDI_MAX)
    {
        // we have a bitmap for the pattern.
        // determine the size of the bitmap, remembering that the
        // bitmaps are DWORD (32 bit) bounded.

        // how many pels per scanline?

        cbBits = psoPattern->sizlBitmap.cx;

        // times how many bits per pel.

		DBGPS(DBGPS_LEVEL_VERBOSE,
			("%s: pattern bitmap type = %d, format = %d\n",
			 _funcname,
			 psoPattern->iType,
			 psoPattern->iBitmapFormat));

        switch (psoPattern->iBitmapFormat)
        {
            case BMF_1BPP:
                break;

            case BMF_4BPP:
                cbBits *= 4;
                break;

            case BMF_8BPP:
                cbBits *= 8;
                break;

            case BMF_16BPP:
                cbBits *= 16;
                break;

            case BMF_24BPP:
                cbBits *= 24;
                break;

            case BMF_32BPP:
                cbBits *= 32;
                break;
        }

        // cbBits now equals the number of bits per scanline.
        // convert it to the number of bytes per scanline, taking into
        // account that scanlines are padded out to 32 bit boundaries.

        cbBits = (((cbBits + 31) / 32) * 4);

        // now that we have the number of bytes per scanline, get the
        // total number of bytes.

        cbBits *= psoPattern->sizlBitmap.cy;
    }
    else
    {
        // we have one of the predefined patterns.

        cbBits = 0;
    }

    // leave room for the color table.
	// check to make sure there is indeed a color table.

	cColorTable = (pxlo->flXlate & XO_TABLE) ? pxlo->cEntries : 0;

	DBGPS(DBGPS_LEVEL_VERBOSE,
		("%s: number of color table entries = %d\n",
		 _funcname,
		 cColorTable));

    // allocate new brush.

    cbTotal = sizeof(DEVBRUSH) + cbBits + cColorTable*sizeof(ULONG);
    pBrush = BRUSHOBJ_pvAllocRbrush(pbo, cbTotal);

    if (!pBrush) {

		DBGPS(DBGPS_LEVEL_ERROR,
			("%s: brush allocation failed.\n", _funcname));
        return(FALSE);
    }

    // fill in the DEVBRUSH information.

    pBrush->iPatIndex   = iHatch;
    pBrush->cXlate      = cColorTable;
    pBrush->offsetXlate = sizeof(DEVBRUSH) + cbBits;

	// copy color table entries and don't copy anything
	// when there is no color table

	if (cColorTable > 0) {

		pbTarg = (PBYTE)((DEVBRUSH *)pBrush) + pBrush->offsetXlate;
		CopyMemory(pbTarg, (PBYTE)pxlo->pulXlate, cColorTable*sizeof(ULONG));
	}

    if (iHatch >= HS_DDI_MAX)
    {
        // we have a bitmap for the pattern.  fill in the appropriate
        // information in the DEVBRUSH, including the bitmap itself.

        pBrush->sizlBitmap = psoPattern->sizlBitmap;
        pBrush->iFormat = (ULONG)psoPattern->iBitmapFormat;
        pBrush->flBitmap = (FLONG)psoPattern->fjBitmap;

        // get a pointer to the pattern bits.

        pbSrc = (PBYTE) psoPattern->pvBits;

        // get a pointer to the destination in the brush.

        pbTarg = (PBYTE)((DEVBRUSH *)pBrush)->ajBits;

        // copy the bits from the pattern surface to the brush.

        memcpy(pbTarg, pbSrc, cbBits);
    }

    // set the pointer to our brush in the BRUSHOBJ.  REMEMBER,
    // the engine will take care of discarding this memory, so
    // we don't have to.

    pbo->pvRbrush = (PVOID)pBrush;

    return(TRUE);
}
