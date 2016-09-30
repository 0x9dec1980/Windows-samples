/******************************Module*Header*******************************\
* Module Name: misc.c
*
* Miscellaneous common routines.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1993-1995 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* BOOL bIntersect
*
* If 'prcl1' and 'prcl2' intersect, has a return value of TRUE and returns
* the intersection in 'prclResult'.  If they don't intersect, has a return
* value of FALSE, and 'prclResult' is undefined.
*
\**************************************************************************/

BOOL bIntersect(
RECTL*  prcl1,
RECTL*  prcl2,
RECTL*  prclResult)
{
    prclResult->left  = max(prcl1->left,  prcl2->left);
    prclResult->right = min(prcl1->right, prcl2->right);

    if (prclResult->left < prclResult->right)
    {
        prclResult->top    = max(prcl1->top,    prcl2->top);
        prclResult->bottom = min(prcl1->bottom, prcl2->bottom);

        if (prclResult->top < prclResult->bottom)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* LONG cIntersect
*
* This routine takes a list of rectangles from 'prclIn' and clips them
* in-place to the rectangle 'prclClip'.  The input rectangles don't
* have to intersect 'prclClip'; the return value will reflect the
* number of input rectangles that did intersect, and the intersecting
* rectangles will be densely packed.
*
\**************************************************************************/

LONG cIntersect(
RECTL*  prclClip,
RECTL*  prclIn,         // List of rectangles
LONG    c)              // Can be zero
{
    LONG    cIntersections;
    RECTL*  prclOut;

    cIntersections = 0;
    prclOut        = prclIn;

    for (; c != 0; prclIn++, c--)
    {
        prclOut->left  = max(prclIn->left,  prclClip->left);
        prclOut->right = min(prclIn->right, prclClip->right);

        if (prclOut->left < prclOut->right)
        {
            prclOut->top    = max(prclIn->top,    prclClip->top);
            prclOut->bottom = min(prclIn->bottom, prclClip->bottom);

            if (prclOut->top < prclOut->bottom)
            {
                prclOut++;
                cIntersections++;
            }
        }
    }

    return(cIntersections);
}

/******************************Public*Routine******************************\
* VOID vResetClipping
*
\**************************************************************************/

VOID vResetClipping(
PDEV*   ppdev)
{
    BYTE*   pjBase;
    ULONG   ulYDstOrg;

    pjBase    = ppdev->pjBase;
    ulYDstOrg = ppdev->ulYDstOrg;   // MGA's linear offset

    CHECK_FIFO_SPACE(pjBase, 4);

    CP_WRITE(pjBase, DWG_CXLEFT,  0);
    CP_WRITE(pjBase, DWG_CXRIGHT, ppdev->cxMemory - 1);
    CP_WRITE(pjBase, DWG_CYTOP,   ulYDstOrg);
    CP_WRITE(pjBase, DWG_CYBOT,
        (ppdev->cyMemory - 1) * ppdev->cxMemory + ulYDstOrg);
}

/******************************Public*Routine******************************\
* VOID vSetClipping
*
\**************************************************************************/

VOID vSetClipping(
PDEV*   ppdev,
RECTL*  prclClip)           // In relative coordinates
{
    BYTE*   pjBase;
    ULONG   ulYDstOrg;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cxMemory;

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;

    CHECK_FIFO_SPACE(pjBase, 4);

    CP_WRITE(pjBase, DWG_CXLEFT,  xOffset + prclClip->left);
    CP_WRITE(pjBase, DWG_CXRIGHT, xOffset + prclClip->right - 1);

    ulYDstOrg = ppdev->ulYDstOrg;   // MGA's linear offset
    yOffset   = ppdev->yOffset;
    cxMemory  = ppdev->cxMemory;

    CP_WRITE(pjBase, DWG_CYTOP,
        (yOffset + prclClip->top) * cxMemory + ulYDstOrg);
    CP_WRITE(pjBase, DWG_CYBOT,
        (yOffset + prclClip->bottom - 1) * cxMemory + ulYDstOrg);
}

/******************************Public*Routine******************************\
* VOID vGetBits
*
* Copies the bits to the given surface from the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vGetBits(
PDEV*       ppdev,
SURFOBJ*    psoDst,
RECTL*      prclDst,        // Absolute coordinates!
POINTL*     pptlSrc)        // Absolute coordinates!
{
    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        vGetBits8bpp(ppdev, psoDst, prclDst, pptlSrc);
    }
    else if (ppdev->iBitmapFormat == BMF_16BPP)
    {
        vGetBits16bpp(ppdev, psoDst, prclDst, pptlSrc);
    }
    else
    {
        vGetBits24bpp(ppdev, psoDst, prclDst, pptlSrc);
    }
}

/******************************Public*Routine******************************\
* VOID vPutBits
*
* Copies the bits from the given surface to the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vPutBits(
PDEV*       ppdev,
SURFOBJ*    psoSrc,
RECTL*      prclDst,            // Absolute coordinates!
POINTL*     pptlSrc)            // Absolute coordinates!
{
    LONG xOffset;
    LONG yOffset;

    // 'vXferNative' takes relative coordinates, but we have absolute
    // coordinates here.  Temporarily adjust our offset variables:

    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    ppdev->xOffset = 0;
    ppdev->yOffset = 0;

    vXferNative(ppdev, 1, prclDst, 0xCCCC, psoSrc, pptlSrc, prclDst, NULL);

    ppdev->xOffset = xOffset;
    ppdev->yOffset = yOffset;
}
