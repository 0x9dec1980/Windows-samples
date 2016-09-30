/******************************Module*Header*******************************\
* Module Name: rxhw.c
*
* This module contains all the implementation-dependent capabilities
* required by 'rxddi.c' that haven't been implemented in 'rxhw.h'.
*
* Copyright (c) 1994-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#include "rxddi.h"
#include "rxhw.h"

/******************************Public*Routine******************************\
* ULONG HwMapColor()
*
* Truncates an RXCOLOR colour to a hardware-native RGB colour.
*
\**************************************************************************/

ULONG HwMapColor(
RXESCAPE*   pEsc,
RXRC*       pRc,
RXCOLOR*    prxColor)
{
    PDEV*   ppdev;
    ULONG   ul;

    if (pRc->flags & RX_COLOR_INDEXED)
    {
        // The colour index is specified in the 'red' component:

        ul = (ULONG) prxColor->r;
    }
    else
    {
        // It's an RGB colour:

        ppdev = pEsc->ppdev;

        ul = (((ULONG) prxColor->r >> (8 - ppdev->rDepth)) << ppdev->rBitShift) |
             (((ULONG) prxColor->g >> (8 - ppdev->gDepth)) << ppdev->gBitShift) |
             (((ULONG) prxColor->b >> (8 - ppdev->bDepth)) << ppdev->bBitShift);
    }

    return(ul);
}

/******************************Public*Routine******************************\
* VOID HwReadRect()
*
\**************************************************************************/

VOID HwReadRect(
RXESCAPE*   pEsc,
RXRC*       pRc,
RECTL*      prclReadRectClip,
RXREADRECT* prxReadRect,
VOID*       pDest)
{
    PDEV*   ppdev;
    SURFOBJ soDst;
    LONG    dx;
    LONG    dy;
    RECTL   rclDst;
    POINTL  ptlSrc;
    ULONG   ulSourceBuffer;

    ppdev = pEsc->ppdev;

    soDst.pvScan0 = pDest;
    soDst.lDelta = prxReadRect->sharedPitch;    // Pitch specified in bytes

    dx = prxReadRect->destRect.x - prxReadRect->sourceX;
    dy = prxReadRect->destRect.y - prxReadRect->sourceY;

    rclDst.left   = prclReadRectClip->left   + dx;
    rclDst.right  = prclReadRectClip->right  + dx;
    rclDst.top    = prclReadRectClip->top    + dy;
    rclDst.bottom = prclReadRectClip->bottom + dy;

    ulSourceBuffer = prxReadRect->sourceBuffer;

    ptlSrc.x = prclReadRectClip->left + ppdev->ptlDoubleBuffer[ulSourceBuffer & 1].x;
    ptlSrc.y = prclReadRectClip->top  + ppdev->ptlDoubleBuffer[ulSourceBuffer & 1].y;

    vGetBits(ppdev, &soDst, &rclDst, &ptlSrc);
}

/******************************Public*Routine******************************\
* VOID HwWriteRect()
*
\**************************************************************************/

VOID HwWriteRect(
RXESCAPE*       pEsc,
RXRC*           pRc,
RECTL*          prclWriteRectClip,
RXWRITERECT*    prxWriteRect,
VOID*           pSource)
{
    PDEV*   ppdev;
    SURFOBJ soSrc;
    LONG    dx;
    LONG    dy;
    RECTL   rclDst;
    POINTL  ptlSrc;

    ppdev = pEsc->ppdev;

    soSrc.pvScan0 = pSource;
    soSrc.lDelta = prxWriteRect->sharedPitch;   // Pitch specified in bytes

    dx = prxWriteRect->sourceX - prxWriteRect->destRect.x;
    dy = prxWriteRect->sourceY - prxWriteRect->destRect.y;

    ptlSrc.x = prclWriteRectClip->left + dx;
    ptlSrc.y = prclWriteRectClip->top  + dy;

    rclDst.left   = prclWriteRectClip->left   + ppdev->xOffset;
    rclDst.right  = prclWriteRectClip->right  + ppdev->xOffset;
    rclDst.top    = prclWriteRectClip->top    + ppdev->yOffset;
    rclDst.bottom = prclWriteRectClip->bottom + ppdev->yOffset;

    vPutBits(ppdev, &soSrc, &rclDst, &ptlSrc);
}

/******************************Public*Routine******************************\
* VOID vDecodeMask
*
* Computes the number of bits and left bit-shift value for an RGB colour
* component.
*
\**************************************************************************/

VOID vDecodeMask(
ULONG   ulMask,
UCHAR*  pcDepth,
UCHAR*  pcShift)
{
    UCHAR   cDepth;
    UCHAR   cShift;

    cShift = 0;
    while (!(ulMask & 1))
    {
        ulMask >>= 1;
        cShift++;
    }
    *pcShift = cShift;

    cDepth = 0;
    while (ulMask & 1)
    {
        ulMask >>= 1;
        cDepth++;
    }
    *pcDepth = cDepth;
}

/******************************Public*Routine******************************\
* BOOL bEnableRx
*
\**************************************************************************/

BOOL bEnableRx(
PDEV*   ppdev)
{
    FLONG   flRed;
    FLONG   flGreen;
    FLONG   flBlue;

    flRed   = ppdev->flRed;
    flGreen = ppdev->flGreen;
    flBlue  = ppdev->flBlue;

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        // We support 3:3:2 at 8bpp by assuming that the application has
        // correctly set up the palette:

        flRed   = 0xE0;
        flGreen = 0x1C;
        flBlue  = 0x03;
    }

    vDecodeMask(flRed,   &ppdev->rDepth, &ppdev->rBitShift);
    vDecodeMask(flGreen, &ppdev->gDepth, &ppdev->gBitShift);
    vDecodeMask(flBlue,  &ppdev->bDepth, &ppdev->bBitShift);

    ppdev->aDepth    = 0;
    ppdev->aBitShift = 0;

    // Initialize some variables for double-buffering:

    ppdev->ptlDoubleBuffer[RX_FRONT_LEFT & 1].x = 0;
    ppdev->ptlDoubleBuffer[RX_FRONT_LEFT & 1].y = 0;

    // Finally, run the common RX initializer:

    return(RxEnable(ppdev));
}

/******************************Public*Routine******************************\
* VOID vDisableRx
*
\**************************************************************************/

VOID vDisableRx(
PDEV*   ppdev)
{

}

/******************************Public*Routine******************************\
* VOID vAssertModeRx
*
\**************************************************************************/

VOID vAssertModeRx(
PDEV*   ppdev,
BOOL    bEnable)
{

}
