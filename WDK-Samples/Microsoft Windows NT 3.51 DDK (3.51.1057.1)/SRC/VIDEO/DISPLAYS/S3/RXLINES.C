/******************************Module*Header*******************************\
* Module Name: rxlines.c
*
* This module contains the line-drawing code for the 3D DDI rendering
* extensions.
*
* We support only flat-shaded non-styled integer lines.  It is required
* functionality for lines that we support both line-lists and line-strips,
* and that lines may be either last-pel-inclusive or last-pel-exclusive.
*
* For performance, we have separate routines for clipping when there is
* a single rectangle, and for clipping when there is more than one
* rectangle.  For the single rectangle case, we rely on the hardware's
* scissors capability to do all the clipping.
*
* In all routines, we peek-ahead in the command batch to look for commands
* that typically follow line commands: colour changes and more line
* commands.  By peeking ahead like this, we don't even have to leave our
* rendering routine for many commands.
*
* We have separate routines for regular I/O and memory-mapped I/O
* because not all S3's support memory-mapped I/O.
*
* Copyright (c) 1994-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#include "rxddi.h"
#include "rxhw.h"

// MAX_LINE_COORD is the largest line that is possible without
// overflowing the Bresenham hardware registers.
//
// The S3 line start and length registers are 12 bits...

#define MAX_LINE_COORD 4095

/**************************************************************************
* Line Rasterization
*
* The 3D DDI's line rasterization rules are not as strict as are GDI's.
* In particular, the 3D DDI doesn't care how we handle 'tie-breakers,'
* where the true line falls exactly mid-way between two pixels, as long
* as we're consistent, particularly between unclipped and clipped lines.
*
* For this implementation, I've chosen to match GDI's convention so that
* I can share clipping code with the GDI line drawing routines.
*
* For integer lines, GDI's rule is that when a line falls exactly mid-way
* between two pixels, the upper or left pixel should be left.  For
* standard Bresenham, this means that we should bias the error term down
* by an infinitesimal amount in the following octants:
*
*         \   0 | 1   /
*           \   |   /
*         0   \ | /   0
*         -------------        1 - Indicates the quadrants in which
*         1   / | \   1            the error term should be biased
*           /   |   \
*         /   0 | 1   \
*
* Using 'plus-y' = 4, 'major-y' = 2, and 'plus-x' = 1 as flags for the
* octants, we get the following numbering scheme:
*
*         \   2 | 3   /
*           \   |   /
*         0   \ | /   1
*         -------------
*         4   / | \   5
*           /   |   \
*         /   6 | 7   \
*
**************************************************************************/

// For the S3, we use the following flags to denote the quadrant, and
// we use this as an index into 'gaiLineBias' to determine the Bresenham
// error bias:

#define QUAD_PLUS_X         1
#define QUAD_MAJOR_Y        2
#define QUAD_PLUS_Y         4

LONG gaiLineBias[] = { 0, 0, 1, 0, 0, 1, 1, 1 };

// We special case horizontal and vertical lines, using the flags octant
// index to compute the draw command:

#define RADIAL_CMD (DRAW_LINE | DRAW | DIR_TYPE_RADIAL | MULTIPLE_PIXELS | WRITE)

ULONG gaiInclusiveRadialCmd[] = {
    0,                                                      // 0
    0,                                                      // 1
    0,                                                      // 2
    (RADIAL_CMD | DRAWING_DIRECTION_90),                    // 3
    (RADIAL_CMD | DRAWING_DIRECTION_180),                   // 4
    (RADIAL_CMD | DRAWING_DIRECTION_0),                     // 5
    0,                                                      // 6
    (RADIAL_CMD | DRAWING_DIRECTION_270)                    // 7
};

ULONG gaiExclusiveRadialCmd[] = {
    0,                                                      // 0
    0,                                                      // 1
    0,                                                      // 2
    (RADIAL_CMD | LAST_PIXEL_OFF | DRAWING_DIRECTION_90),   // 3
    (RADIAL_CMD | LAST_PIXEL_OFF | DRAWING_DIRECTION_180),  // 4
    (RADIAL_CMD | LAST_PIXEL_OFF | DRAWING_DIRECTION_0),    // 5
    0,                                                      // 6
    (RADIAL_CMD | LAST_PIXEL_OFF | DRAWING_DIRECTION_270)   // 7
};

// We shift these flags by 'QUADRANT_SHIFT' to send the actual
// command to the S3:

#define QUADRANT_SHIFT      5

#if ((QUAD_PLUS_X << QUADRANT_SHIFT) != PLUS_X)
    #error "PLUS_X define changed"
#endif
#if ((QUAD_PLUS_Y << QUADRANT_SHIFT) != PLUS_Y)
    #error "PLUS_Y define changed"
#endif
#if ((QUAD_MAJOR_Y << QUADRANT_SHIFT) != MAJOR_Y)
    #error "MAJOR_Y define changed"
#endif

/******************************Public*Routine******************************\
* VOID vComplicatedLine
*
* Handles, in a slow sort of way, any icky complicated line:
*
*   1) Any line that is too long for the hardware;
*   2) Any line with complex clipping;
*   3) Any styled lines.
*
\**************************************************************************/

VOID vComplicatedLine(
RXESCAPE*   pEsc,
LONG        x0,     // Coordinates should not have the window offset
LONG        y0,     //   or double-buffer yet added in
LONG        x1,
LONG        y1)
{
    PDEV*       ppdev;
    RXRC*       pRc;
    ENUMRECTS*  pClip;
    POINTFIX    ptfx0;
    POINTFIX    ptfx1;
    PFNSTRIP*   ppfnStrip;
    LINESTATE   ls;
    RECTL       rclBound;
    RECTL       arclClip[4];
    FLONG       fl;
    LONG        crclClip;
    RECTL*      prclClip;

    ppdev = pEsc->ppdev;
    pRc   = pEsc->pRc;
    pClip = pEsc->pWnd->prxClipScissored;

    fl = FL_SIMPLE_CLIP;
    if (pRc->lastPixel)
        fl |= FL_LAST_PEL_INCLUSIVE;

    // Add in the window offset:

    x0 += pEsc->pWnd->clientRect.left;
    x1 += pEsc->pWnd->clientRect.left;
    y0 += pEsc->pWnd->clientRect.top;
    y1 += pEsc->pWnd->clientRect.top;

    // Convert to fixed coordinates for 'bLines':

    ptfx0.x = x0 << 4;
    ptfx1.x = x1 << 4;
    ptfx0.y = y0 << 4;
    ptfx1.y = y1 << 4;

    ppfnStrip = &gapfnStrip[0];

    // Compute the bound box for the line:

    if (x0 < x1)
    {
        rclBound.left   = x0;
        rclBound.right  = x1;
    }
    else
    {
        rclBound.left   = x1;
        rclBound.right  = x0;
    }
    if (y0 < y1)
    {
        rclBound.top    = y0;
        rclBound.bottom = y1;
    }
    else
    {
        rclBound.top    = y1;
        rclBound.bottom = y0;
    }

    // To be consistent with the list of clip rectangles, the bound box
    // must be made lower-right exclusive:

    rclBound.right++;
    rclBound.bottom++;

    for (crclClip = pClip->c, prclClip = pClip->arcl;
         crclClip > 0;
         crclClip--, prclClip++)
    {
        if ((rclBound.left   < prclClip->right)  &&
            (rclBound.right  > prclClip->left)   &&
            (rclBound.top    < prclClip->bottom) &&
            (rclBound.bottom > prclClip->top))
        {
            // The 'bLines' code is pretty silly in that it requires us
            // to give a pre-normalized list of rectangles for the clip
            // rectangle:

            arclClip[0].left   =  prclClip->left;
            arclClip[1].top    =  arclClip[0].left;
            arclClip[2].left   =  arclClip[0].left;
            arclClip[3].top    =  arclClip[0].left;

            arclClip[0].top    =  prclClip->top;
            arclClip[1].left   =  arclClip[0].top;
            arclClip[2].bottom = -arclClip[0].top + 1;
            arclClip[3].right  = -arclClip[0].top + 1;

            arclClip[0].right  =  prclClip->right;
            arclClip[1].bottom =  arclClip[0].right;
            arclClip[2].right  =  arclClip[0].right;
            arclClip[3].bottom =  arclClip[0].right;

            arclClip[0].bottom =  prclClip->bottom;
            arclClip[1].right  =  arclClip[0].bottom;
            arclClip[2].top    = -arclClip[0].bottom + 1;
            arclClip[3].left   = -arclClip[0].bottom + 1;

            // The only way 'bLines' can fail is if the math overflows,
            // and we don't really care for the 3D DDI:

            bLines(ppdev, &ptfx0, &ptfx1, NULL, 1, &ls, arclClip,
                   ppfnStrip, fl);
        }
    }
}

/******************************Public*Routine******************************\
* VOID vMmSimpleLines
*
* Draws solid lines when there is trivial or rectangle clipping, using
* memory-mapped I/O.
*
* We rely on the hardware's clip scissors to do any clipping.  We don't
* even do trivial rejection checks, reasoning that a smart caller will
* have done all the trivial clipping already.
*
* If you don't have hardware clipping, you'll obviously want to also add
* a trivial acceptance check...
*
\**************************************************************************/

VOID vMmSimpleLines(
RXESCAPE*       pEsc,
RXDRAWPRIM*     prxDrawPrim)
{
    PDEV*           ppdev;
    RXRC*           pRc;
    RXPOINTINT*     ppti;
    ULONG           cpti;
    LONG            cFifo;
    BYTE*           pjMmBase;
    ULONG           ulDefaultCmd;
    ULONG*          piRadialCmd;
    LONG            xOffset;
    LONG            yOffset;
    FLONG           flQuadrant;
    LONG            x;
    LONG            y;
    LONG            dx;
    LONG            dy;
    RXMEMOBJ*       prxMemObj;
    RXHANDLE        hrxSharedMem;
    BYTE*           pjClientBase;
    BYTE*           pjServerBase;
    ULONG           cjMax;

    ppdev    = pEsc->ppdev;
    pRc      = pEsc->pRc;

    cFifo    = 0;                           // Count of free FIFO entries
    pjMmBase = ppdev->pjMmBase;

    piRadialCmd  = gaiInclusiveRadialCmd;
    ulDefaultCmd = (DRAW_LINE | DRAW | DIR_TYPE_XY | MULTIPLE_PIXELS | WRITE);

    if (!pRc->lastPixel)
    {
        piRadialCmd   = gaiExclusiveRadialCmd;
        ulDefaultCmd |= LAST_PIXEL_OFF;
    }

    xOffset = ppdev->xOffset + pEsc->pWnd->clientRect.left;
    yOffset = ppdev->yOffset + pEsc->pWnd->clientRect.top;

NewHandle:

    hrxSharedMem = prxDrawPrim->hrxSharedMemVertexData;
    prxMemObj = (RXMEMOBJ*) GetPtrFromHandle(hrxSharedMem, RXHANDLE_SHAREMEM);
    if (prxMemObj == NULL)
    {
        RXDBG_PRINT("vMmSimpleLines: illegal 'hrxSharedMemVertexData' handle");
        goto ReturnFailure;
    }

    // Calculate the transformation from the client-side address to
    // the server-side address, as well as the bounds:

    pjClientBase = prxMemObj->clientBaseAddress;
    pjServerBase = prxMemObj->pMem;
    cjMax        = (ULONG) (prxMemObj->pMemMax - prxMemObj->pMem);

Restart:

    ppti = (RXPOINTINT*) prxDrawPrim->pSharedMem;
    cpti = CLAMPCOUNT(prxDrawPrim->numVertices);

    CONVERT_POINTER_RANGE(ppti, cpti * sizeof(RXPOINTINT),
                          pjClientBase, cjMax, pjServerBase, ReturnFailure);

NewLine:

    if (--cpti > 0)
    {
        cFifo -= 2;
        if (cFifo < 0)
        {
            IO_ALL_EMPTY(ppdev);
            cFifo = MM_ALL_EMPTY_FIFO_COUNT - 2;
        }

        MM_ABS_CUR_X(ppdev, pjMmBase, xOffset + ppti->x);
        MM_ABS_CUR_Y(ppdev, pjMmBase, yOffset + ppti->y);

        while (TRUE)
        {
            // We have to load the vertices out of the command buffer because
            // they're sitting in a shared memory section and may be modified
            // asynchronously by the app.

            flQuadrant = (QUAD_PLUS_X | QUAD_PLUS_Y);

            x  = xOffset + (ppti    )->x;
            dx = xOffset + (ppti + 1)->x;
            y  = yOffset + (ppti    )->y;
            dy = yOffset + (ppti + 1)->y;

            ppti++;

            // This unsigned comparison means that we will be taking
            // the slow boat for any lines with negative coordinates.
            // But negative coordinates should be rare, so I'm not
            // going to slow down the common case:

            if ((ULONG) (x | y | dx | dy) <= MAX_LINE_COORD)
            {
                dx -= x;
                if (dx < 0)
                {
                    dx = -dx;
                    flQuadrant &= ~QUAD_PLUS_X;
                }

                dy -= y;
                if (dy < 0)
                {
                    dy = -dy;
                    flQuadrant &= ~QUAD_PLUS_Y;
                }

                if (dy > dx)
                {
                    register LONG l;

                    l  = dy;
                    dy = dx;
                    dx = l;                     // Swap 'dx' and 'dy'
                    flQuadrant |= QUAD_MAJOR_Y;
                }

                if (dy == 0)
                {
                    // We have a horizontal or vertical line.

                    cFifo -= 2;
                    if (cFifo < 0)
                    {
                        IO_ALL_EMPTY(ppdev);
                        cFifo = MM_ALL_EMPTY_FIFO_COUNT - 2;
                    }

                    MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, dx);
                    MM_CMD(ppdev, pjMmBase, piRadialCmd[flQuadrant]);

                    if (--cpti != 0)
                        continue;
                    else
                        break;
                }
                else
                {
                    // We have a regular line.

                    cFifo -= 5;
                    if (cFifo < 0)
                    {
                        IO_ALL_EMPTY(ppdev);
                        cFifo = MM_ALL_EMPTY_FIFO_COUNT - 5;
                    }

                    MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, dx);
                    MM_AXSTP(ppdev, pjMmBase, dy);
                    MM_DIASTP(ppdev, pjMmBase, dy - dx);
                    MM_ERR_TERM(ppdev, pjMmBase,
                        (dy + dy - dx - gaiLineBias[flQuadrant]) >> 1);
                    MM_CMD(ppdev, pjMmBase, ulDefaultCmd | (flQuadrant << QUADRANT_SHIFT));

                    if (--cpti != 0)
                        continue;
                    else
                        break;
                }
            }

            // The line is too long for the hardware to handle, so pass it off
            // to our slow routine:

            vComplicatedLine(pEsc, x  - xOffset, y  - yOffset,
                                   dx - xOffset, dy - yOffset);

            cFifo = 0;
            goto NewLine;
        }
    }

    // Peek ahead in the command stream to see if we have another
    // linestrip command or a colour change, because we already have
    // the hardware warmed up to do lines.

    if (PEEK_AHEAD(sizeof(RXSETSTATE)))
    {
        RXSETSTATE*  prxSetState;
        RXCOLOR* prxColor;

        prxSetState = (RXSETSTATE*) pEsc->prxCmd;
        if ((prxSetState->command == RXCMD_SET_STATE) &&
            (prxSetState->stateToChange == RX_SOLID_COLOR))
        {
            ADVANCE_AND_CHECK(sizeof(RXSETSTATE)
                            + sizeof(RXCOLOR)
                            - sizeof(prxSetState->newState[0]));

            prxColor = (RXCOLOR*) &prxSetState->newState[0];
            HW_SOLID_COLOR(pEsc, pRc, prxColor);

            cFifo -= 2;
            if (cFifo < 0)
            {
                IO_ALL_EMPTY(ppdev);
                cFifo = MM_ALL_EMPTY_FIFO_COUNT - 2;
            }

            if (DEPTH32(ppdev))
            {
                MM_FRGD_COLOR32(ppdev, pjMmBase, pRc->hwSolidColor);
            }
            else
            {
                MM_FRGD_COLOR(ppdev, pjMmBase, pRc->hwSolidColor);
            }
        }
    }

    if (PEEK_AHEAD(sizeof(RXDRAWPRIM)))
    {
        prxDrawPrim = (RXDRAWPRIM*) pEsc->prxCmd;
        if ((prxDrawPrim->command == RXCMD_DRAW_PRIM) &&
            (prxDrawPrim->primType == RX_PRIM_INTLINESTRIP))
        {
            ADVANCE_WITH_NO_CHECK(sizeof(RXDRAWPRIM));
            if (hrxSharedMem == prxDrawPrim->hrxSharedMemVertexData)
                goto Restart;
            else
                goto NewHandle;
        }
    }

    // Success!

    return;

ReturnFailure:

    ERROR_RETURN;
}

/******************************Public*Routine******************************\
* VOID vMmComplexLines()
*
* Draws solid lines when there is complex clipping, using memory-mapped
* I/O.
*
\**************************************************************************/

VOID vMmComplexLines(
RXESCAPE*       pEsc,
RXDRAWPRIM*     prxDrawPrim)
{
    PDEV*           ppdev;
    RXRC*           pRc;
    RXPOINTINT*     ppti;
    ULONG           cpti;
    ENUMRECTS*      pClip;
    LONG            cFifo;
    BYTE*           pjMmBase;
    ULONG           ulDefaultCmd;
    ULONG*          piRadialCmd;
    LONG            xWindow;
    LONG            yWindow;
    LONG            xOffset;
    LONG            yOffset;
    LONG            crcl;
    RECTL*          prcl;
    FLONG           flQuadrant;
    LONG            x;
    LONG            y;
    LONG            dx;
    LONG            dy;
    LONG            xLeft;
    LONG            xRight;
    LONG            yTop;
    LONG            yBottom;
    RXMEMOBJ*       prxMemObj;
    RXHANDLE        hrxSharedMem;
    BYTE*           pjClientBase;
    BYTE*           pjServerBase;
    ULONG           cjMax;

    ppdev    = pEsc->ppdev;
    pRc      = pEsc->pRc;
    pClip    = pEsc->pWnd->prxClipScissored;

    cFifo    = 0;                           // Count of free FIFO entries
    pjMmBase = ppdev->pjMmBase;

    piRadialCmd  = gaiInclusiveRadialCmd;
    ulDefaultCmd = (DRAW_LINE | DRAW | DIR_TYPE_XY | MULTIPLE_PIXELS | WRITE);

    if (!pRc->lastPixel)
    {
        piRadialCmd   = gaiExclusiveRadialCmd;
        ulDefaultCmd |= LAST_PIXEL_OFF;
    }

    // Our double-buffer 'xOffset' and 'yOffset' must be added in after
    // the line is compared to the clip rectangle:

    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;
    xWindow = pEsc->pWnd->clientRect.left;
    yWindow = pEsc->pWnd->clientRect.top;

NewHandle:

    hrxSharedMem = prxDrawPrim->hrxSharedMemVertexData;
    prxMemObj = (RXMEMOBJ*) GetPtrFromHandle(hrxSharedMem, RXHANDLE_SHAREMEM);
    if (prxMemObj == NULL)
    {
        RXDBG_PRINT("vNmSimpleLines: illegal 'hrxSharedMemVertexData' handle");
        goto ReturnFailure;
    }

    // Calculate the transformation from the client-side address to
    // the server-side address, as well as the bounds:

    pjClientBase = prxMemObj->clientBaseAddress;
    pjServerBase = prxMemObj->pMem;
    cjMax        = (ULONG) (prxMemObj->pMemMax - prxMemObj->pMem);

Restart:

    ppti = (RXPOINTINT*) prxDrawPrim->pSharedMem;
    cpti = CLAMPCOUNT(prxDrawPrim->numVertices);

    CONVERT_POINTER_RANGE(ppti, cpti * sizeof(RXPOINTINT),
                          pjClientBase, cjMax, pjServerBase, ReturnFailure);

    for (cpti--; cpti > 0; cpti--)
    {
        // We have to load the vertices out of the command buffer because
        // they're sitting in a shared memory section and may be modified
        // asynchronously by the app.

        x  = xWindow + (ppti    )->x;
        dx = xWindow + (ppti + 1)->x;
        y  = yWindow + (ppti    )->y;
        dy = yWindow + (ppti + 1)->y;

        ppti++;

        // Compute a well-ordered bound-box for the line:

        if (x < dx)
        {
            xLeft   = x;
            xRight  = dx;
        }
        else
        {
            xLeft   = dx;
            xRight  = x;
        }
        if (y < dy)
        {
            yTop    = y;
            yBottom = dy;
        }
        else
        {
            yTop    = dy;
            yBottom = y;
        }

        // Make it lower-right exclusive to be consistent with the given
        // clip rectangles:

        xRight++;
        yBottom++;

        // Now search through every clip rectangle to find the first
        // intersection:

        for (crcl = pClip->c, prcl = pClip->arcl;
             crcl != 0;
             crcl--, prcl++)
        {
            // The clip rectangles are ordered left-to-right, top-to-bottom.
            // We take advantage of that ordering to simplify the hit test
            // to two 'if's:

            if ((xLeft < prcl->right) &&
                (yTop  < prcl->bottom))
            {
                // We found a clip rectangle that intersects our line's
                // bound box.  Usually, the line will be completely
                // contained in the clip rectangle, so we check for that:

                if ((xLeft   >= prcl->left)   &&
                    (yTop    >= prcl->top)    &&
                    (xRight  <= prcl->right)  &&
                    (yBottom <= prcl->bottom))
                {
                    // The line is entirely contained within one rectangle.
                    // Since the line is essentially unclipped, it doesn't
                    // extend past any edge of the screen, and so we are
                    // guaranteed that we won't overflow the hardware's line
                    // registers.

                    flQuadrant = (QUAD_PLUS_X | QUAD_PLUS_Y);

                    dx -= x;
                    if (dx < 0)
                    {
                        dx = -dx;
                        flQuadrant &= ~QUAD_PLUS_X;
                    }

                    dy -= y;
                    if (dy < 0)
                    {
                        dy = -dy;
                        flQuadrant &= ~QUAD_PLUS_Y;
                    }

                    if (dy > dx)
                    {
                        register LONG l;

                        l  = dy;
                        dy = dx;
                        dx = l;                     // Swap 'dx' and 'dy'
                        flQuadrant |= QUAD_MAJOR_Y;
                    }

                    if (dy == 0)
                    {
                        // We have a horizontal or vertical line.

                        cFifo -= 4;
                        if (cFifo < 0)
                        {
                            IO_ALL_EMPTY(ppdev);
                            cFifo = MM_ALL_EMPTY_FIFO_COUNT - 4;
                        }

                        MM_ABS_CUR_X(ppdev, pjMmBase, x + xOffset);
                        MM_ABS_CUR_Y(ppdev, pjMmBase, y + yOffset);
                        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, dx);
                        MM_CMD(ppdev, pjMmBase, piRadialCmd[flQuadrant]);

                        break;  // Break out of clip rectangle loop
                    }
                    else
                    {
                        // We have a regular line.

                        cFifo -= 7;
                        if (cFifo < 0)
                        {
                            IO_ALL_EMPTY(ppdev);
                            cFifo = MM_ALL_EMPTY_FIFO_COUNT - 7;
                        }

                        MM_ABS_CUR_X(ppdev, pjMmBase, x + xOffset);
                        MM_ABS_CUR_Y(ppdev, pjMmBase, y + yOffset);
                        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, dx);
                        MM_AXSTP(ppdev, pjMmBase, dy);
                        MM_DIASTP(ppdev, pjMmBase, dy - dx);
                        MM_ERR_TERM(ppdev, pjMmBase, (dy + dy - dx - gaiLineBias[flQuadrant]) >> 1);
                        MM_CMD(ppdev, pjMmBase, ulDefaultCmd | (flQuadrant << QUADRANT_SHIFT));

                        break;  // Break out of clip rectangle loop
                    }
                }
                else
                {
                    // The line may intersect more than one clip rectangle.
                    // Ugh, we have some hard work to do.

                    vComplicatedLine(pEsc, x  - xWindow, y  - yWindow,
                                           dx - xWindow, dy - yWindow);
                    cFifo = 0;

                    break;      // Break out of clip rectangle loop
                }
            }
        }
    }

    // Peek ahead in the command stream to see if we have another
    // linestrip command or a colour change, because we already have
    // the hardware warmed up to do lines.

    if (PEEK_AHEAD(sizeof(RXSETSTATE)))
    {
        RXSETSTATE*     prxSetState;
        RXCOLOR* prxColor;

        prxSetState = (RXSETSTATE*) pEsc->prxCmd;

        if ((prxSetState->command == RXCMD_SET_STATE) &&
            (prxSetState->stateToChange == RX_SOLID_COLOR))
        {
            ADVANCE_AND_CHECK(sizeof(RXSETSTATE)
                            + sizeof(RXCOLOR)
                            - sizeof(prxSetState->newState[0]));

            prxColor = (RXCOLOR*) &prxSetState->newState[0];
            HW_SOLID_COLOR(pEsc, pRc, prxColor);

            cFifo -= 2;
            if (cFifo < 0)
            {
                IO_ALL_EMPTY(ppdev);
                cFifo = MM_ALL_EMPTY_FIFO_COUNT - 2;
            }

            if (DEPTH32(ppdev))
            {
                MM_FRGD_COLOR32(ppdev, pjMmBase, pRc->hwSolidColor);
            }
            else
            {
                MM_FRGD_COLOR(ppdev, pjMmBase, pRc->hwSolidColor);
            }
        }
    }

    if (PEEK_AHEAD(sizeof(RXDRAWPRIM)))
    {
        prxDrawPrim = (RXDRAWPRIM*) pEsc->prxCmd;
        if ((prxDrawPrim->command == RXCMD_DRAW_PRIM) &&
            (prxDrawPrim->primType == RX_PRIM_INTLINESTRIP))
        {
            ADVANCE_WITH_NO_CHECK(sizeof(RXDRAWPRIM));
            if (hrxSharedMem == prxDrawPrim->hrxSharedMemVertexData)
                goto Restart;
            else
                goto NewHandle;
        }
    }

    // Success!

    return;

ReturnFailure:

    ERROR_RETURN;
}

/******************************Public*Routine******************************\
* VOID vMmDrawLines
*
* Draws integer line-strips and line-lists using memory-mapped I/O.
*
\**************************************************************************/

VOID vMmDrawLines(
RXESCAPE*   pEsc,
RXDRAWPRIM* prxDrawPrim)
{
    PDEV*           ppdev;
    RXRC*           pRc;
    ENUMRECTS*      pClip;
    LONG            cpti;
    RXPOINTINT*     pptiStart;
    ULONG*          pdjStart;
    VOID*           pvEnd;
    BYTE*           pjMmBase;

    ppdev    = pEsc->ppdev;
    pjMmBase = ppdev->pjMmBase;
    pRc      = pEsc->pRc;
    pClip    = pEsc->pWnd->prxClipScissored;

    // Check for VGA full-screen after parsing the command stream but
    // before touching the hardware...

    if (pClip->c != 0)
    {
        if (DEPTH32(ppdev))
        {
            IO_FIFO_WAIT(ppdev, 8);
            MM_FRGD_COLOR32(ppdev, pjMmBase, pRc->hwSolidColor);
        }
        else
        {
            IO_FIFO_WAIT(ppdev, 7);
            MM_FRGD_COLOR(ppdev, pjMmBase, pRc->hwSolidColor);
        }

        MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | pRc->hwRop2);
        MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);

        if (pClip->c == 1)
        {
            // Don't forget that the S3 scissor registers are lower-right
            // inclusive:

            MM_SCISSORS_L(ppdev, pjMmBase, pClip->arcl[0].left);
            MM_SCISSORS_T(ppdev, pjMmBase, pClip->arcl[0].top);
            MM_SCISSORS_R(ppdev, pjMmBase, pClip->arcl[0].right - 1);
            MM_SCISSORS_B(ppdev, pjMmBase, pClip->arcl[0].bottom - 1);

            vMmSimpleLines(pEsc, prxDrawPrim);

            IO_FIFO_WAIT(ppdev, 4);
            MM_ABS_SCISSORS_L(ppdev, pjMmBase, 0);
            MM_ABS_SCISSORS_T(ppdev, pjMmBase, 0);
            MM_ABS_SCISSORS_R(ppdev, pjMmBase, ppdev->cxMemory - 1);
            MM_ABS_SCISSORS_B(ppdev, pjMmBase, ppdev->cyMemory - 1);
        }
        else
        {
            vMmComplexLines(pEsc, prxDrawPrim);
        }
    }
}


/******************************Public*Routine******************************\
* VOID vIoSimpleLines
*
* Draws solid lines when there is trivial or rectangle clipping, using
* memory-mapped I/O.
*
* We rely on the hardware's clip scissors to do any clipping.  We don't
* even do trivial rejection checks, reasoning that a smart caller will
* have done all the trivial clipping already.
*
* If you don't have hardware clipping, you'll obviously want to also add
* a trivial acceptance check...
*
\**************************************************************************/

VOID vIoSimpleLines(
RXESCAPE*       pEsc,
RXDRAWPRIM*     prxDrawPrim)
{
    PDEV*           ppdev;
    RXRC*           pRc;
    RXPOINTINT*     ppti;
    ULONG           cpti;
    LONG            cFifo;
    ULONG           ulDefaultCmd;
    ULONG*          piRadialCmd;
    LONG            xOffset;
    LONG            yOffset;
    FLONG           flQuadrant;
    LONG            x;
    LONG            y;
    LONG            dx;
    LONG            dy;
    RXMEMOBJ*       prxMemObj;
    RXHANDLE        hrxSharedMem;
    BYTE*           pjClientBase;
    BYTE*           pjServerBase;
    ULONG           cjMax;

    ppdev    = pEsc->ppdev;
    pRc      = pEsc->pRc;

    cFifo    = 0;                           // Count of free FIFO entries

    piRadialCmd  = gaiInclusiveRadialCmd;
    ulDefaultCmd = (DRAW_LINE | DRAW | DIR_TYPE_XY | MULTIPLE_PIXELS | WRITE);

    if (!pRc->lastPixel)
    {
        piRadialCmd   = gaiExclusiveRadialCmd;
        ulDefaultCmd |= LAST_PIXEL_OFF;
    }

    xOffset = ppdev->xOffset + pEsc->pWnd->clientRect.left;
    yOffset = ppdev->yOffset + pEsc->pWnd->clientRect.top;

NewHandle:

    hrxSharedMem = prxDrawPrim->hrxSharedMemVertexData;
    prxMemObj = (RXMEMOBJ*) GetPtrFromHandle(hrxSharedMem, RXHANDLE_SHAREMEM);
    if (prxMemObj == NULL)
    {
        RXDBG_PRINT("vIoSimpleLines: illegal 'hrxSharedMemVertexData' handle");
        goto ReturnFailure;
    }

    // Calculate the transformation from the client-side address to
    // the server-side address, as well as the bounds:

    pjClientBase = prxMemObj->clientBaseAddress;
    pjServerBase = prxMemObj->pMem;
    cjMax        = (ULONG) (prxMemObj->pMemMax - prxMemObj->pMem);

Restart:

    ppti = (RXPOINTINT*) prxDrawPrim->pSharedMem;
    cpti = CLAMPCOUNT(prxDrawPrim->numVertices);

    CONVERT_POINTER_RANGE(ppti, cpti * sizeof(RXPOINTINT),
                          pjClientBase, cjMax, pjServerBase, ReturnFailure);

NewLine:

    if (--cpti > 0)
    {
        cFifo -= 2;
        if (cFifo < 0)
        {
            IO_FIFO_WAIT(ppdev, IO_ALL_EMPTY_FIFO_COUNT);
            cFifo = IO_ALL_EMPTY_FIFO_COUNT - 2;
        }

        IO_ABS_CUR_X(ppdev, xOffset + ppti->x);
        IO_ABS_CUR_Y(ppdev, yOffset + ppti->y);

        while (TRUE)
        {
            // We have to load the vertices out of the command buffer because
            // they're sitting in a shared memory section and may be modified
            // asynchronously by the app.

            flQuadrant = (QUAD_PLUS_X | QUAD_PLUS_Y);

            x  = xOffset + (ppti    )->x;
            dx = xOffset + (ppti + 1)->x;
            y  = yOffset + (ppti    )->y;
            dy = yOffset + (ppti + 1)->y;

            ppti++;

            // This unsigned comparison means that we will be taking
            // the slow boat for any lines with negative coordinates.
            // But negative coordinates should be rare, so I'm not
            // going to slow down the common case:

            if ((ULONG) (x | y | dx | dy) <= MAX_LINE_COORD)
            {
                dx -= x;
                if (dx < 0)
                {
                    dx = -dx;
                    flQuadrant &= ~QUAD_PLUS_X;
                }

                dy -= y;
                if (dy < 0)
                {
                    dy = -dy;
                    flQuadrant &= ~QUAD_PLUS_Y;
                }

                if (dy > dx)
                {
                    register LONG l;

                    l  = dy;
                    dy = dx;
                    dx = l;                     // Swap 'dx' and 'dy'
                    flQuadrant |= QUAD_MAJOR_Y;
                }

                if (dy == 0)
                {
                    // We have a horizontal or vertical line.

                    cFifo -= 2;
                    if (cFifo < 0)
                    {
                        IO_FIFO_WAIT(ppdev, IO_ALL_EMPTY_FIFO_COUNT);
                        cFifo = IO_ALL_EMPTY_FIFO_COUNT - 2;
                    }

                    IO_MAJ_AXIS_PCNT(ppdev, dx);
                    IO_CMD(ppdev, piRadialCmd[flQuadrant]);

                    if (--cpti != 0)
                        continue;
                    else
                        break;
                }
                else
                {
                    // We have a regular line.

                    cFifo -= 5;
                    if (cFifo < 0)
                    {
                        IO_FIFO_WAIT(ppdev, IO_ALL_EMPTY_FIFO_COUNT);
                        cFifo = IO_ALL_EMPTY_FIFO_COUNT - 5;
                    }

                    IO_MAJ_AXIS_PCNT(ppdev, dx);
                    IO_AXSTP(ppdev, dy);
                    IO_DIASTP(ppdev, dy - dx);
                    IO_ERR_TERM(ppdev, (dy + dy - dx - gaiLineBias[flQuadrant]) >> 1);
                    IO_CMD(ppdev, ulDefaultCmd | (flQuadrant << QUADRANT_SHIFT));

                    if (--cpti != 0)
                        continue;
                    else
                        break;
                }
            }

            // The line is too long for the hardware to handle, so pass it off
            // to our slow routine:

            vComplicatedLine(pEsc, x  - xOffset, y  - yOffset,
                                   dx - xOffset, dy - yOffset);

            cFifo = 0;
            goto NewLine;
        }
    }

    // Peek ahead in the command stream to see if we have another
    // linestrip command or a colour change, because we already have
    // the hardware warmed up to do lines.

    if (PEEK_AHEAD(sizeof(RXSETSTATE)))
    {
        RXSETSTATE*  prxSetState;
        RXCOLOR* prxColor;

        prxSetState = (RXSETSTATE*) pEsc->prxCmd;
        if ((prxSetState->command == RXCMD_SET_STATE) &&
            (prxSetState->stateToChange == RX_SOLID_COLOR))
        {
            ADVANCE_AND_CHECK(sizeof(RXSETSTATE)
                            + sizeof(RXCOLOR)
                            - sizeof(prxSetState->newState[0]));

            prxColor = (RXCOLOR*) &prxSetState->newState[0];
            HW_SOLID_COLOR(pEsc, pRc, prxColor);

            cFifo -= 2;
            if (cFifo < 0)
            {
                IO_FIFO_WAIT(ppdev, IO_ALL_EMPTY_FIFO_COUNT);
                cFifo = IO_ALL_EMPTY_FIFO_COUNT - 2;
            }

            if (DEPTH32(ppdev))
            {
                IO_FRGD_COLOR32(ppdev, pRc->hwSolidColor);
            }
            else
            {
                IO_FRGD_COLOR(ppdev, pRc->hwSolidColor);
            }
        }
    }

    if (PEEK_AHEAD(sizeof(RXDRAWPRIM)))
    {
        prxDrawPrim = (RXDRAWPRIM*) pEsc->prxCmd;
        if ((prxDrawPrim->command == RXCMD_DRAW_PRIM) &&
            (prxDrawPrim->primType == RX_PRIM_INTLINESTRIP))
        {
            ADVANCE_WITH_NO_CHECK(sizeof(RXDRAWPRIM));
            if (hrxSharedMem == prxDrawPrim->hrxSharedMemVertexData)
                goto Restart;
            else
                goto NewHandle;
        }
    }

    // Success!

    return;

ReturnFailure:

    ERROR_RETURN;
}

/******************************Public*Routine******************************\
* VOID vIoComplexLines()
*
* Draws solid lines when there is complex clipping, using memory-mapped
* I/O.
*
\**************************************************************************/

VOID vIoComplexLines(
RXESCAPE*       pEsc,
RXDRAWPRIM*     prxDrawPrim)
{
    PDEV*           ppdev;
    RXRC*           pRc;
    RXPOINTINT*     ppti;
    ULONG           cpti;
    ENUMRECTS*      pClip;
    LONG            cFifo;
    ULONG           ulDefaultCmd;
    ULONG*          piRadialCmd;
    LONG            xWindow;
    LONG            yWindow;
    LONG            xOffset;
    LONG            yOffset;
    LONG            crcl;
    RECTL*          prcl;
    FLONG           flQuadrant;
    LONG            x;
    LONG            y;
    LONG            dx;
    LONG            dy;
    LONG            xLeft;
    LONG            xRight;
    LONG            yTop;
    LONG            yBottom;
    RXMEMOBJ*       prxMemObj;
    RXHANDLE        hrxSharedMem;
    BYTE*           pjClientBase;
    BYTE*           pjServerBase;
    ULONG           cjMax;

    ppdev    = pEsc->ppdev;
    pRc      = pEsc->pRc;
    pClip    = pEsc->pWnd->prxClipScissored;

    cFifo    = 0;                           // Count of free FIFO entries

    piRadialCmd  = gaiInclusiveRadialCmd;
    ulDefaultCmd = (DRAW_LINE | DRAW | DIR_TYPE_XY | MULTIPLE_PIXELS | WRITE);

    if (!pRc->lastPixel)
    {
        piRadialCmd   = gaiExclusiveRadialCmd;
        ulDefaultCmd |= LAST_PIXEL_OFF;
    }

    // Our double-buffer 'xOffset' and 'yOffset' must be added in after
    // the line is compared to the clip rectangle:

    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;
    xWindow = pEsc->pWnd->clientRect.left;
    yWindow = pEsc->pWnd->clientRect.top;

NewHandle:

    hrxSharedMem = prxDrawPrim->hrxSharedMemVertexData;
    prxMemObj = (RXMEMOBJ*) GetPtrFromHandle(hrxSharedMem, RXHANDLE_SHAREMEM);
    if (prxMemObj == NULL)
    {
        RXDBG_PRINT("vIoSimpleLines: illegal 'hrxSharedMemVertexData' handle");
        goto ReturnFailure;
    }

    // Calculate the transformation from the client-side address to
    // the server-side address, as well as the bounds:

    pjClientBase = prxMemObj->clientBaseAddress;
    pjServerBase = prxMemObj->pMem;
    cjMax        = (ULONG) (prxMemObj->pMemMax - prxMemObj->pMem);

Restart:

    ppti = (RXPOINTINT*) prxDrawPrim->pSharedMem;
    cpti = CLAMPCOUNT(prxDrawPrim->numVertices);

    CONVERT_POINTER_RANGE(ppti, cpti * sizeof(RXPOINTINT),
                          pjClientBase, cjMax, pjServerBase, ReturnFailure);

    for (cpti--; cpti > 0; cpti--)
    {
        // We have to load the vertices out of the command buffer because
        // they're sitting in a shared memory section and may be modified
        // asynchronously by the app.

        x  = xWindow + (ppti    )->x;
        dx = xWindow + (ppti + 1)->x;
        y  = yWindow + (ppti    )->y;
        dy = yWindow + (ppti + 1)->y;

        ppti++;

        // Compute a well-ordered bound-box for the line:

        if (x < dx)
        {
            xLeft   = x;
            xRight  = dx;
        }
        else
        {
            xLeft   = dx;
            xRight  = x;
        }
        if (y < dy)
        {
            yTop    = y;
            yBottom = dy;
        }
        else
        {
            yTop    = dy;
            yBottom = y;
        }

        // Make it lower-right exclusive to be consistent with the given
        // clip rectangles:

        xRight++;
        yBottom++;

        // Now search through every clip rectangle to find the first
        // intersection:

        for (crcl = pClip->c, prcl = pClip->arcl;
             crcl != 0;
             crcl--, prcl++)
        {
            // The clip rectangles are ordered left-to-right, top-to-bottom.
            // We take advantage of that ordering to simplify the hit test
            // to two 'if's:

            if ((xLeft < prcl->right) &&
                (yTop  < prcl->bottom))
            {
                // We found a clip rectangle that intersects our line's
                // bound box.  Usually, the line will be completely
                // contained in the clip rectangle, so we check for that:

                if ((xLeft   >= prcl->left)   &&
                    (yTop    >= prcl->top)    &&
                    (xRight  <= prcl->right)  &&
                    (yBottom <= prcl->bottom))
                {
                    // The line is entirely contained within one rectangle.
                    // Since the line is essentially unclipped, it doesn't
                    // extend past any edge of the screen, and so we are
                    // guaranteed that we won't overflow the hardware's line
                    // registers.

                    flQuadrant = (QUAD_PLUS_X | QUAD_PLUS_Y);

                    dx -= x;
                    if (dx < 0)
                    {
                        dx = -dx;
                        flQuadrant &= ~QUAD_PLUS_X;
                    }

                    dy -= y;
                    if (dy < 0)
                    {
                        dy = -dy;
                        flQuadrant &= ~QUAD_PLUS_Y;
                    }

                    if (dy > dx)
                    {
                        register LONG l;

                        l  = dy;
                        dy = dx;
                        dx = l;                     // Swap 'dx' and 'dy'
                        flQuadrant |= QUAD_MAJOR_Y;
                    }

                    if (dy == 0)
                    {
                        // We have a horizontal or vertical line.

                        cFifo -= 4;
                        if (cFifo < 0)
                        {
                            IO_FIFO_WAIT(ppdev, IO_ALL_EMPTY_FIFO_COUNT);
                            cFifo = IO_ALL_EMPTY_FIFO_COUNT - 4;
                        }

                        IO_ABS_CUR_X(ppdev, x + xOffset);
                        IO_ABS_CUR_Y(ppdev, y + yOffset);
                        IO_MAJ_AXIS_PCNT(ppdev, dx);
                        IO_CMD(ppdev, piRadialCmd[flQuadrant]);

                        break;  // Break out of clip rectangle loop
                    }
                    else
                    {
                        // We have a regular line.

                        cFifo -= 7;
                        if (cFifo < 0)
                        {
                            IO_FIFO_WAIT(ppdev, IO_ALL_EMPTY_FIFO_COUNT);
                            cFifo = IO_ALL_EMPTY_FIFO_COUNT - 7;
                        }

                        IO_ABS_CUR_X(ppdev, x + xOffset);
                        IO_ABS_CUR_Y(ppdev, y + yOffset);
                        IO_MAJ_AXIS_PCNT(ppdev, dx);
                        IO_AXSTP(ppdev, dy);
                        IO_DIASTP(ppdev, dy - dx);
                        IO_ERR_TERM(ppdev, (dy + dy - dx - gaiLineBias[flQuadrant]) >> 1);
                        IO_CMD(ppdev, ulDefaultCmd | (flQuadrant << QUADRANT_SHIFT));

                        break;  // Break out of clip rectangle loop
                    }
                }
                else
                {
                    // The line may intersect more than one clip rectangle.
                    // Ugh, we have some hard work to do.

                    vComplicatedLine(pEsc, x  - xWindow, y  - yWindow,
                                           dx - xWindow, dy - yWindow);
                    cFifo = 0;

                    break;      // Break out of clip rectangle loop
                }
            }
        }
    }

    // Peek ahead in the command stream to see if we have another
    // linestrip command or a colour change, because we already have
    // the hardware warmed up to do lines.

    if (PEEK_AHEAD(sizeof(RXSETSTATE)))
    {
        RXSETSTATE*     prxSetState;
        RXCOLOR* prxColor;

        prxSetState = (RXSETSTATE*) pEsc->prxCmd;

        if ((prxSetState->command == RXCMD_SET_STATE) &&
            (prxSetState->stateToChange == RX_SOLID_COLOR))
        {
            ADVANCE_AND_CHECK(sizeof(RXSETSTATE)
                            + sizeof(RXCOLOR)
                            - sizeof(prxSetState->newState[0]));

            prxColor = (RXCOLOR*) &prxSetState->newState[0];
            HW_SOLID_COLOR(pEsc, pRc, prxColor);

            cFifo -= 2;
            if (cFifo < 0)
            {
                IO_FIFO_WAIT(ppdev, IO_ALL_EMPTY_FIFO_COUNT);
                cFifo = IO_ALL_EMPTY_FIFO_COUNT - 2;
            }

            if (DEPTH32(ppdev))
            {
                IO_FRGD_COLOR32(ppdev, pRc->hwSolidColor);
            }
            else
            {
                IO_FRGD_COLOR(ppdev, pRc->hwSolidColor);
            }
        }
    }

    if (PEEK_AHEAD(sizeof(RXDRAWPRIM)))
    {
        prxDrawPrim = (RXDRAWPRIM*) pEsc->prxCmd;
        if ((prxDrawPrim->command == RXCMD_DRAW_PRIM) &&
            (prxDrawPrim->primType == RX_PRIM_INTLINESTRIP))
        {
            ADVANCE_WITH_NO_CHECK(sizeof(RXDRAWPRIM));
            if (hrxSharedMem == prxDrawPrim->hrxSharedMemVertexData)
                goto Restart;
            else
                goto NewHandle;
        }
    }

    // Success!

    return;

ReturnFailure:

    ERROR_RETURN;
}

/******************************Public*Routine******************************\
* VOID vIoDrawLines
*
* Draws integer line-strips and line-lists using memory-mapped I/O.
*
\**************************************************************************/

VOID vIoDrawLines(
RXESCAPE*   pEsc,
RXDRAWPRIM* prxDrawPrim)
{
    PDEV*           ppdev;
    RXRC*           pRc;
    ENUMRECTS*      pClip;
    LONG            cpti;
    RXPOINTINT*     pptiStart;
    ULONG*          pdjStart;
    VOID*           pvEnd;

    ppdev    = pEsc->ppdev;
    pRc      = pEsc->pRc;
    pClip    = pEsc->pWnd->prxClipScissored;

    // Check for VGA full-screen after parsing the command stream but
    // before touching the hardware...

    if (pClip->c != 0)
    {
        if (DEPTH32(ppdev))
        {
            IO_FIFO_WAIT(ppdev, 8);
            IO_FRGD_COLOR32(ppdev, pRc->hwSolidColor);
        }
        else
        {
            IO_FIFO_WAIT(ppdev, 7);
            IO_FRGD_COLOR(ppdev, pRc->hwSolidColor);
        }

        IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | pRc->hwRop2);
        IO_PIX_CNTL(ppdev, ALL_ONES);

        if (pClip->c == 1)
        {
            // Don't forget that the S3 scissor registers are lower-right
            // inclusive:

            IO_SCISSORS_L(ppdev, pClip->arcl[0].left);
            IO_SCISSORS_T(ppdev, pClip->arcl[0].top);
            IO_SCISSORS_R(ppdev, pClip->arcl[0].right - 1);
            IO_SCISSORS_B(ppdev, pClip->arcl[0].bottom - 1);

            vIoSimpleLines(pEsc, prxDrawPrim);

            IO_FIFO_WAIT(ppdev, 4);
            IO_ABS_SCISSORS_L(ppdev, 0);
            IO_ABS_SCISSORS_T(ppdev, 0);
            IO_ABS_SCISSORS_R(ppdev, ppdev->cxMemory - 1);
            IO_ABS_SCISSORS_B(ppdev, ppdev->cyMemory - 1);
        }
        else
        {
            vIoComplexLines(pEsc, prxDrawPrim);
        }
    }
}
