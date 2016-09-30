/******************************Module*Header*******************************\
* Module Name: Brush.c
*
* Handles all brush/pattern initialization and realization.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Data*Struct*************************\
* gaaulPat
*
* These are the standard patterns defined for Windows.
*
\**************************************************************************/

ULONG gaaulPat[HS_DDI_MAX][8] = {

// Scans have to be DWORD aligned:

    { 0x00,                // ........     HS_HORIZONTAL 0
      0x00,                // ........
      0x00,                // ........
      0xff,                // ********
      0x00,                // ........
      0x00,                // ........
      0x00,                // ........
      0x00 },              // ........

    { 0x08,                // ....*...     HS_VERTICAL 1
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08 },              // ....*...

    { 0x80,                // *.......     HS_FDIAGONAL 2
      0x40,                // .*......
      0x20,                // ..*.....
      0x10,                // ...*....
      0x08,                // ....*...
      0x04,                // .....*..
      0x02,                // ......*.
      0x01 },              // .......*

    { 0x01,                // .......*     HS_BDIAGONAL 3
      0x02,                // ......*.
      0x04,                // .....*..
      0x08,                // ....*...
      0x10,                // ...*....
      0x20,                // ..*.....
      0x40,                // .*......
      0x80 },              // *.......

    { 0x08,                // ....*...     HS_CROSS 4
      0x08,                // ....*...
      0x08,                // ....*...
      0xff,                // ********
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08 },              // ....*...

    { 0x81,                // *......*     HS_DIAGCROSS 5
      0x42,                // .*....*.
      0x24,                // ..*..*..
      0x18,                // ...**...
      0x18,                // ...**...
      0x24,                // ..*..*..
      0x42,                // .*....*.
      0x81 }               // *......*
};

/******************************Public*Routine******************************\
* BOOL bInitializePatterns
*
* Initialize the default patterns.
*
\**************************************************************************/

BOOL bInitializePatterns(
PDEV* ppdev,
INT   cPatterns)
{
    SIZEL   sizl;
    LONG    i;
    HBITMAP hbm;

    sizl.cx = 8;
    sizl.cy = 8;

    for (i = 0; i < cPatterns; i++)
    {
        hbm = EngCreateBitmap(sizl,
                              (LONG) sizeof(ULONG),
                              BMF_1BPP,
                              BMF_TOPDOWN,
                              &gaaulPat[i][0]);

        ppdev->ahbmPat[i] = hbm;
        if (hbm == 0)
        {
            // Set the count created so vUninitializePatterns will clean up:

            ppdev->cPatterns = i;
            DISPDBG((0, "Failed bInitializePatterns"));
            return(FALSE);
        }
    }

    DISPDBG((5, "Passed bInitializePatterns"));

    ppdev->cPatterns = i;
    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vUninitializePatterns
*
* Delete the standard patterns allocated.
*
* Note: In an error case, this may be called before bInitializePatterns.
*
\**************************************************************************/

VOID vUninitializePatterns(
PDEV* ppdev)
{
    LONG i;

    // Erase all patterns:

    for (i = 0; i < ppdev->cPatterns; i++)
    {
        EngDeleteSurface((HSURF) ppdev->ahbmPat[i]);
    }
}

/******************************Public*Routine******************************\
* VOID vRealizeDitherPattern
*
* Generates an 8x8 dither pattern, in our internal realization format, for
* the colour ulRGBToDither.  Note that the high byte of ulRGBToDither does
* not need to be set to zero, because vComputeSubspaces ignores it.
\**************************************************************************/

VOID vRealizeDitherPattern(
PDEV*   ppdev,
RBRUSH* prb,
ULONG   ulRGBToDither)
{
    ULONG           ulNumVertices;
    VERTEX_DATA     vVertexData[4];
    VERTEX_DATA*    pvVertexData;
    LONG            i;

    // Calculate what colour subspaces are involved in the dither:

    pvVertexData = vComputeSubspaces(ulRGBToDither, vVertexData);

    // Now that we have found the bounding vertices and the number of
    // pixels to dither for each vertex, we can create the dither pattern

    ulNumVertices = pvVertexData - vVertexData;
                      // # of vertices with more than zero pixels in the dither

    // Do the actual dithering:

    vDitherColor(&prb->aulPattern[0], vVertexData, pvVertexData, ulNumVertices);

    // Initialize the fields we need:

    prb->fl         = 0;
    prb->pfnFillPat = ppdev->pfnFillPatNative;

    for (i = 0; i < MAX_BOARDS; i++)
    {
        prb->apbe[i] = &ppdev->beUnrealizedBrush;
    }
}

/******************************Public*Routine******************************\
* BOOL DrvRealizeBrush
*
* This function allows us to convert GDI brushes into an internal form
* we can use.  It may be called directly by GDI at SelectObject time, or
* it may be called by GDI as a result of us calling BRUSHOBJ_pvGetRbrush
* to create a realized brush in a function like DrvBitBlt.
*
* Note that we have no way of determining what the current Rop or brush
* alignment are at this point.
*
\**************************************************************************/

BOOL DrvRealizeBrush(
BRUSHOBJ*   pbo,
SURFOBJ*    psoDst,
SURFOBJ*    psoPattern,
SURFOBJ*    psoMask,
XLATEOBJ*   pxlo,
ULONG       iHatch)
{
    PDEV*       ppdev;
    ULONG       iPatternFormat;
    BYTE        jSrc;
    BYTE*       pjSrc;
    BYTE*       pjDst;
    LONG        lSrcDelta;
    LONG        cj;
    LONG        i;
    LONG        j;
    RBRUSH*     prb;
    ULONG*      pulXlate;
    SURFOBJ*    psoPunt;
    RECTL       rclDst;

    ppdev = (PDEV*) psoDst->dhpdev;

    // We have a fast path for dithers when we set GCAPS_DITHERONREALIZE:

    if (iHatch & RB_DITHERCOLOR)
    {
        if (!(ppdev->flStatus & STAT_BRUSH_CACHE))
            goto ReturnFalse;

        // Implementing DITHERONREALIZE increased our score on a certain
        // unmentionable benchmark by 0.4 million 'megapixels'.  Too bad
        // this didn't work in the first version of NT.

        prb = BRUSHOBJ_pvAllocRbrush(pbo,
              sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE * ppdev->cjPelSize));
        if (prb == NULL)
            goto ReturnFalse;

        vRealizeDitherPattern(ppdev, prb, iHatch);
        return(TRUE);
    }

    // We only handle colour brushes if we have an off-screen brush cache
    // available.  If there isn't one, we can simply fail the realization,
    // and eventually GDI will do the drawing for us (although a lot
    // slower than we could have done it).
    //
    // We also only accelerate 8x8 patterns.  Since Win3.1 and Chicago don't
    // support patterns of any other size, it's a safe bet that 99.9%
    // of the patterns we'll ever get will be 8x8:

    if ((psoPattern->sizlBitmap.cx != 8) ||
        (psoPattern->sizlBitmap.cy != 8) ||
        ((psoPattern->iBitmapFormat != BMF_1BPP) &&
         !(ppdev->flStatus & STAT_BRUSH_CACHE)))
        goto ReturnFalse;

    prb = BRUSHOBJ_pvAllocRbrush(pbo,
          sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE * ppdev->cjPelSize));
    if (prb == NULL)
        goto ReturnFalse;

    // Initialize the fields we need:

    prb->fl         = 0;
    prb->pfnFillPat = ppdev->pfnFillPatNative;

    for (i = 0; i < MAX_BOARDS; i++)
    {
        prb->apbe[i] = &ppdev->beUnrealizedBrush;
    }

    lSrcDelta = psoPattern->lDelta;
    pjSrc     = (BYTE*) psoPattern->pvScan0;
    pjDst     = (BYTE*) &prb->aulPattern[0];

    iPatternFormat = psoPattern->iBitmapFormat;
    if ((ppdev->iBitmapFormat == iPatternFormat) &&
        ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
    {
        DISPDBG((5, "Realizing un-translated brush"));

        // The pattern is the same colour depth as the screen, and
        // there's no translation to be done:

        cj = (8 * ppdev->cjPelSize);    // Every pattern is 8 pels wide

        for (i = 8; i != 0; i--)
        {
            RtlCopyMemory(pjDst, pjSrc, cj);

            pjSrc += lSrcDelta;
            pjDst += cj;
        }
    }
    else if (iPatternFormat == BMF_1BPP)
    {
        DISPDBG((5, "Realizing 1bpp brush"));

        // Since we allocated at least 64 bytes when we did our
        // BRUSHOBJ_pvAllocBrush call, we've got plenty of space
        // to store our monochrome brush.
        //
        // Since the Windows convention for monochrome bitmaps is that
        // the MSB of a given byte represents the leftmost pixel, which
        // is opposite that of the MGA, we must reverse the order of
        // each byte before using it in SRC0 through SRC3.  Moreover,
        // each byte must be replicated so as to yield a 16x8 pattern.

        for (i = 8; i != 0; i--)
        {
            jSrc         = gajFlip[*pjSrc];
            *(pjDst)     = jSrc;
            *(pjDst + 1) = jSrc;
            pjDst       += 2;
            pjSrc       += lSrcDelta;
        }

        pulXlate         = pxlo->pulXlate;
        prb->fl         |= RBRUSH_2COLOR;
        prb->ulColor[1]  = pulXlate[1];
        prb->ulColor[0]  = pulXlate[0];
        prb->pfnFillPat  = vFillPat1bpp;
    }
    else if ((iPatternFormat == BMF_4BPP) && (ppdev->iBitmapFormat == BMF_8BPP))
    {
        DISPDBG((5, "Realizing 4bpp brush"));

        // The screen is 8bpp and the pattern is 4bpp:

        ASSERTDD((ppdev->iBitmapFormat == BMF_8BPP) &&
                 (iPatternFormat == BMF_4BPP),
                 "Messed up brush logic");

        pulXlate = pxlo->pulXlate;

        for (i = 8; i != 0; i--)
        {
            // Inner loop is repeated only 4 times because each loop
            // handles 2 pixels:

            for (j = 4; j != 0; j--)
            {
                *pjDst++ = (BYTE) pulXlate[*pjSrc >> 4];
                *pjDst++ = (BYTE) pulXlate[*pjSrc & 15];
                pjSrc++;
            }

            pjSrc += lSrcDelta - 4;
        }
    }
    else
    {
        // We've got a brush whose format we haven't special cased.  No
        // problem, we can have GDI convert it to our device's format.
        // We simply use a temporary surface object that was created with
        // the same format as the display, and point it to our brush
        // realization:

        DISPDBG((5, "Realizing funky brush"));

        psoPunt          = ppdev->psoPunt;
        psoPunt->pvScan0 = pjDst;
        psoPunt->lDelta  = 8 * ppdev->cjPelSize;

        rclDst.left      = 0;
        rclDst.top       = 0;
        rclDst.right     = 8;
        rclDst.bottom    = 8;

        if (!EngCopyBits(psoPunt, psoPattern, NULL, pxlo,
                         &rclDst, (POINTL*) &rclDst))
        {
            goto ReturnFalse;
        }
    }

    return(TRUE);

ReturnFalse:

    if (psoPattern != NULL)
    {
        DISPDBG((5, "Failed realization -- Type: %li Format: %li cx: %li cy: %li",
                    psoPattern->iType, psoPattern->iBitmapFormat,
                    psoPattern->sizlBitmap.cx, psoPattern->sizlBitmap.cy));
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bEnableBrushCache
*
* Allocates off-screen memory for storing the brush cache.
\**************************************************************************/

BOOL bEnableBrushCache(
PDEV*   ppdev)
{
    OH*             poh;            // Points to off-screen chunk of memory
    BRUSHENTRY*     pbe;            // Pointer to the brush-cache entry
    CIRCLEENTRY*    pce;
    ULONG           ulLinearStart;
    ULONG           ulLinearEnd;
    LONG            cBrushCache;
    ULONG           ulTmp;
    LONG            x;
    LONG            y;
    LONG            i;

    pbe = ppdev->pbe;       // Points to where we'll put the first brush
                            //   cache entry

    poh = pohAllocate(ppdev,
                      NULL,
                      ppdev->cxMemory,
                      BRUSH_CACHE_HEIGHT,
                      FLOH_MAKE_PERMANENT);
    if (poh == NULL)
        goto ReturnTrue;    // See note about why we can return TRUE...

    ulLinearStart = (poh->y  * ppdev->cxMemory) + ppdev->ulYDstOrg;
    ulLinearEnd   = (poh->cy * ppdev->cxMemory) + ulLinearStart;

    // An MGA brush is always cached with a 256-pel alignment.  The brush
    // can be 16x16, or two interleaved 16x8 brushes.  We use the second
    // option, so that every second brush starts on a 256+16 alignment.
    //
    // So the brushes are stored in pairs, with a 256-pel alignment:

    ulLinearStart = (ulLinearStart + 0xff) & ~0xff;

    cBrushCache = (ulLinearEnd - ulLinearStart) >> 8;
    cBrushCache *= 2;       // Don't forget they're pairs

    pbe = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                     cBrushCache * sizeof(BRUSHENTRY));
    if (pbe == NULL)
        goto ReturnTrue;    // See note about why we can return TRUE...

    ppdev->cBrushCache = cBrushCache;
    ppdev->pbe         = pbe;

    do {
        // If we hadn't allocated 'pbe' with LMEM_ZEROINIT, we would have
        // to initialize pbe->prbVerify, too...

        // Set up linear coordinate for reading the pattern from offscreen
        // memory:

        pbe->ulLinear = ulLinearStart;

        // Set up coordinates for writing the pattern into offscreen
        // memory, assuming a '32' stride:

        ulTmp         = ulLinearStart - ppdev->ulYDstOrg;
        x             = ulTmp % ppdev->cxMemory;
        y             = ulTmp / ppdev->cxMemory;
        pbe->ulLeft   = x & 31;
        pbe->ulYDst   = (y * ppdev->cxMemory + x) >> 5;

        // Account for the interleave, where every second cached brush
        // starts on a 256+16 boundary:

        if ((cBrushCache & 1) == 0)
        {
            ulLinearStart += 16;
        }
        else
        {
            ulLinearStart += (256 - 16);
        }

    } while (pbe++, --cBrushCache != 0);

    // When we create a new brush, we always point it to our
    // 'beUnrealizedBrush' entry, which will always have 'prbVerify'
    // set to NULL.  In this way, we can remove an 'if' from our
    // check to see if we have to realize the brush in 'vFillPat' --
    // we only have to compare to 'prbVerify':

    ppdev->beUnrealizedBrush.prbVerify = NULL;

    // Note that we don't have to remember 'poh' for when we have
    // to disable brushes -- the off-screen heap frees any
    // off-screen heap allocations automatically.

    // We successfully allocated the brush cache, so let's turn
    // on the switch showing that we can use it:

    ppdev->flStatus |= STAT_BRUSH_CACHE;

    ////////////////////////////////////////////////////////////////////////
    // Now allocate our circle cache.
    //
    // Note that we don't have to initially mark the entries as invalid,
    // as the ppdev was zero-filled, and so we are assured that every
    // 'rcfxBound' will be {0, 0, 0, 0}, which will never match any
    // circle when looking for a matching entry.

    poh = pohAllocate(ppdev,
                      NULL,
                      CIRCLE_ALLOCATION_CX * TOTAL_CIRCLE_COUNT,
                      CIRCLE_ALLOCATION_CY,
                      FLOH_MAKE_PERMANENT);
    if (poh == NULL)
        goto ReturnTrue;

    pce = &ppdev->ace[0];       // Points to where we'll put the first circle
                                //   cache entry
    for (i = 0; i < TOTAL_CIRCLE_COUNT; i++)
    {
        pce->x = poh->x + (i * CIRCLE_ALLOCATION_CX);
        pce->y = poh->y;
        pce++;
    }

    ppdev->flStatus |= STAT_CIRCLE_CACHE;

ReturnTrue:

    // If we couldn't allocate a brush cache, it's not a catastrophic
    // failure; patterns will still work, although they'll be a bit
    // slower since they'll go through GDI.  As a result we don't
    // actually have to fail this call:

    DISPDBG((5, "Passed bEnableBrushCache"));

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisableBrushCache
*
* Cleans up anything done in bEnableBrushCache.
\**************************************************************************/

VOID vDisableBrushCache(PDEV* ppdev)
{
    LocalFree(ppdev->pbe);
}

/******************************Public*Routine******************************\
* VOID vAssertModeBrushCache
*
* Resets the brush cache when we exit out of full-screen.
\**************************************************************************/

VOID vAssertModeBrushCache(
PDEV*   ppdev,
BOOL    bEnable)
{
    BRUSHENTRY*     pbe;
    CIRCLEENTRY*    pce;
    LONG            i;

    if (bEnable)
    {
        // Invalidate the brush cache:

        pbe = ppdev->pbe;

        for (i = ppdev->cBrushCache; i != 0; i--)
        {
            pbe->prbVerify = NULL;
            pbe++;
        }

        // Invalidate the circle cache:

        pce = &ppdev->ace[0];

        for (i = TOTAL_CIRCLE_COUNT; i != 0; i--)
        {
            pce->rcfxCircle.xLeft  = 0;
            pce->rcfxCircle.xRight = 0;
            pce++;
        }
    }
}
