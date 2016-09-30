/******************************Module*Header*******************************\
* Module Name: rxhw.c
*
* This module contains all the implementation-dependent capabilities
* required by 'rxddi.c' that haven't been implemented in 'rxhw.h'.
*
* Copyright (c) 1994-1995 Microsoft Corporation
* Copyright (c) 1995 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"
#include "rxddi.h"
#include "rxhw.h"

/******************************Public*Routine******************************\
* BOOL bCommitBuffer
*
\**************************************************************************/

BOOL bCommitBuffer(
RXRC*       pRc,
OH*         poh)            // 2-d heap allocation structure for buffer
{
    PDEV*   ppdev;
    BYTE*   pjBase;
    RECTL   rcl;

    ppdev  = pRc->ppdev;
    pjBase = ppdev->pjBase;

    // Commit the back buffer, if we have one:

    if ((poh == NULL) || (!bOhCommit(ppdev, poh, TRUE)))
        return(FALSE);

    // To be secure, we have to clear the buffer now, otherwise
    // the app could do a READ_RECT and read what was there.
    // Of course, this is mostly an issue only under Windows NT,
    // not Windows 95...

    rcl.left   = poh->x;
    rcl.top    = poh->y;
    rcl.right  = poh->cx + rcl.left;
    rcl.bottom = poh->cy + rcl.top;

    // First, reset the clipping and the plane mask:

    vResetClipping(ppdev);

    CHECK_FIFO_SPACE(pjBase, 16);
    CP_WRITE(pjBase, DWG_PLNWT, -1);

    // Now, do the blt:

    CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP + atype_RPL +
                                 blockm_ON + pattern_OFF +
                                 transc_BG_OPAQUE + bop_SRCCOPY);
    CP_WRITE(pjBase, DWG_SGN, 0);
    CP_WRITE(pjBase, DWG_AR1, 0);
    CP_WRITE(pjBase, DWG_AR2, 0);
    CP_WRITE(pjBase, DWG_AR4, 0);
    CP_WRITE(pjBase, DWG_AR5, 0);
    CP_WRITE(pjBase, DWG_SRC0, -1);
    CP_WRITE(pjBase, DWG_SRC1, -1);
    CP_WRITE(pjBase, DWG_SRC2, -1);
    CP_WRITE(pjBase, DWG_SRC3, -1);
    CP_WRITE(pjBase, DWG_FCOL, 0xffffffff);      // Could be any colour
    CP_WRITE(pjBase, DWG_FXLEFT, poh->x);
    CP_WRITE(pjBase, DWG_FXRIGHT, poh->x + poh->cx);
    CP_WRITE(pjBase, DWG_LEN, poh->cy);
    CP_START(pjBase, DWG_YDST, poh->y);

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID HwEnableBuffers
*
\**************************************************************************/

VOID HwEnableBuffers(
RXESCAPE*           pEsc,
RXRC*               pRc,
RXENABLEBUFFERS*    prxEnableBuffers)
{
    PDEV*           ppdev;
    ULONG           buffers;
    OH*             poh;
    RBRUSH_COLOR    rbc;

    ppdev = pEsc->ppdev;

    if (!pRc->hwAllCaps)
    {
        SET_ERROR_CODE;
    }
    else
    {
        buffers = prxEnableBuffers->buffers;

        // Athena always has z allocated, when doing shading:

        pRc->zBufEnabled =
            ((prxEnableBuffers->buffers & RX_ENABLE_Z_BUFFER) != 0);

        if (buffers & RX_ENABLE_BACK_LEFT_BUFFER)
        {
            if (!(pRc->backBufEnabled))
            {
                if (ppdev->cDoubleBufferRef == 0)
                {
                    if (!bCommitBuffer(pRc, ppdev->pohBackBuffer))
                    {
                        SET_ERROR_CODE;
                        return;
                    }

                    // Reset the hardware back to the current state, since
                    // 'bCommitBuffer' changed it:

                    HW_INIT_STATE(pEsc, pRc);
                }

                ppdev->cDoubleBufferRef++;
                pRc->backBufEnabled = TRUE;
                pRc->hwBackBufferOffset = ppdev->pohBackBuffer->y;
            }
        }
        else
        {
            if (pRc->backBufEnabled)
            {
                pRc->backBufEnabled = FALSE;

                if (--ppdev->cDoubleBufferRef == 0)
                {
                    // Decommit the back buffer area, so that the off-screen
                    // memory can be used for other stuff:

                    bOhCommit(ppdev, ppdev->pohBackBuffer, FALSE);
                }
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID HwDestroyRc
*
\**************************************************************************/

VOID HwDestroyRc(
RXRC* pRc)
{
    PDEV*   ppdev;

    ppdev = pRc->ppdev;

    // We don't have to worry about freeing a z buffer or back buffer
    // if we only support 3D DDI integer lines on this board:

    if (pRc->hwAllCaps)
    {
        if (pRc->backBufEnabled)
        {
            pRc->backBufEnabled = FALSE;

            if (--ppdev->cDoubleBufferRef == 0)
            {
                // Decommit the back buffer area, so that the off-screen
                // memory can be used for other stuff:

                bOhCommit(ppdev, ppdev->pohBackBuffer, FALSE);
            }
        }

        if (--ppdev->cZBufferRef == 0)
        {
            // The last 3D DDI client has gone away.  Decommit the z buffer
            // area, so that it may be used for other stuff.  Every RC uses
            // the z-buffer because of the Athena bug:

            bOhCommit(ppdev, ppdev->pohZBuffer, FALSE);
        }
    }
}

/******************************Public*Routine******************************\
* VOID HwCreateRc
*
\**************************************************************************/

VOID HwCreateRc(
RXESCAPE*   pEsc,
RXRC*       pRc)
{
    PDEV* ppdev;

    ppdev = pRc->ppdev;

    // Assume that we won't have full 3D capability for this RC (e.g.,
    // we're running in a resolution where we don't have enough room
    // for the back and z-buffer

    pRc->hwAllCaps = FALSE;

    // We don't have to worry about a z buffer when doing only 3D DDI
    // integer lines.
    //
    // Watch out that the hardware doesn't support color indexed mode --
    // we will do only integer lines if asked to do color indexed.

    if ((ppdev->flHwCaps & HWCAPS_ALL) && !(pRc->flags & RX_COLOR_INDEXED))
    {
        pRc->hwAllCaps = TRUE;

        pRc->primFunc[RX_PRIM_INTLINESTRIP] = DrawIntLineStrip;
        pRc->primFunc[RX_PRIM_LINESTRIP] = DrawLineStrip;
        pRc->primFunc[RX_PRIM_TRISTRIP] = DrawTriangleStrip;
        pRc->primFunc[RX_PRIM_LINELIST] = DrawLineList;
        pRc->primFunc[RX_PRIM_TRILIST] = DrawTriangleList;

        if (ppdev->cZBufferRef == 0)
        {
            // Ah ha, we have our first 3D DDI client.  Get our z-buffer
            // memory back and clear it:

            if (!bCommitBuffer(pRc, ppdev->pohZBuffer))
            {
                SET_ERROR_CODE;
                return;
            }
        }

        // Update the reference count to reflect the new client:

        ppdev->cZBufferRef++;
    }
}

/******************************Public*Routine******************************\
* VOID HwAlignedCopy
*
\**************************************************************************/

VOID HwAlignedCopy(
PDEV*   ppdev,
BYTE*   pjBitmap,
LONG    lBitmapDelta,
ULONG   ulScreen,       // Byte offset
LONG    lScreenDelta,   // Byte delta
LONG    cjScan,
LONG    cyScan,
BOOL    bDstIsScreen)
{
    BYTE*   pjBase;
    LONG    cjStartPhase;
    LONG    cjEndPhase;
    LONG    cjMiddle;
    LONG    culMiddle;
    BYTE*   pjScreenStart;
    BYTE*   pjScreen;
    LONG    i;

    cjStartPhase = (-(LONG) ulScreen) & 3;
    cjMiddle     = cjScan - cjStartPhase;

    if (cjMiddle < 0)
    {
        cjStartPhase = 0;
        cjMiddle     = cjScan;
    }

    lBitmapDelta -= cjScan;            // Account for middle

    cjEndPhase = cjMiddle & 3;
    culMiddle  = cjMiddle >> 2;

    pjBase = ppdev->pjBase;
    pjScreenStart = pjBase + SRCWND + (ulScreen & 31);

    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
    WAIT_NOT_BUSY(pjBase);

    if (bDstIsScreen)
    {
        // Align writes to the screen:

        for (; cyScan > 0; cyScan--)
        {
            CP_WRITE_REGISTER(pjBase + HST_DSTPAGE, ulScreen);
            ulScreen += lScreenDelta;
            pjScreen = pjScreenStart;

            for (i = cjStartPhase; i > 0; i--)
            {
                WRITE_REGISTER_UCHAR(pjScreen, *pjBitmap);
                pjBitmap++;
                pjScreen++;
            }

            for (i = culMiddle; i > 0; i--)
            {
                WRITE_REGISTER_ULONG(pjScreen, *((ULONG UNALIGNED *) pjBitmap));
                pjBitmap += sizeof(ULONG);
                pjScreen += sizeof(ULONG);
            }

            for (i = cjEndPhase; i > 0; i--)
            {
                WRITE_REGISTER_UCHAR(pjScreen, *pjBitmap);
                pjBitmap++;
                pjScreen++;
            }

            pjBitmap += lBitmapDelta;
        }
    }
    else
    {
        // Align reads from the screen:

        for (; cyScan > 0; cyScan--)
        {
            CP_WRITE_REGISTER(pjBase + HST_DSTPAGE, ulScreen);
            ulScreen += lScreenDelta;
            pjScreen = pjScreenStart;

            for (i = cjStartPhase; i > 0; i--)
            {
                *pjBitmap = READ_REGISTER_UCHAR(pjScreen);
                pjScreen++;
                pjBitmap++;
            }

            for (i = culMiddle; i > 0; i--)
            {
                *((ULONG UNALIGNED *) pjBitmap) = READ_REGISTER_ULONG(pjScreen);
                pjScreen += sizeof(ULONG);
                pjBitmap += sizeof(ULONG);
            }

            for (i = cjEndPhase; i > 0; i--)
            {
                *pjBitmap = READ_REGISTER_UCHAR(pjScreen);
                pjScreen++;
                pjBitmap++;
            }

            pjBitmap += lBitmapDelta;
        }
    }
}

/******************************Public*Routine******************************\
* BOOL bEnableRx
*
\**************************************************************************/

BOOL bEnableRx(
PDEV*   ppdev)
{
    ULONG   cjVRam;
    LONG    cyFrontBuffer;
    LONG    cyBackBuffer;
    ULONG   ulBackBufferStart;
    ULONG   ulBackBufferEnd;
    LONG    yBackBufferStart;
    LONG    yBackBufferEnd;
    LONG    cyZBuffer;
    ULONG   ulZBufferStart;
    ULONG   ulZBufferEnd;
    LONG    yZBufferStart;
    LONG    yZBufferEnd;
    ULONG   ulBackZBufferOffset;
    POINTL  ptlBackBuffer;
    POINTL  ptlZBuffer;
    OH*     pohBackBuffer;
    OH*     pohZBuffer;

    // We don't enable any 3D DDI functionality when running with
    // a virtual desktop across multiple boards.  It will require
    // a substantial amount of work to get this functionality
    // working.

    ppdev->flHwCaps = 0;

    #if !MULTI_BOARDS
    {
        // We support 3D DDI integer lines on any MGA board, with no
        // back buffer or z:

        ppdev->flHwCaps = HWCAPS_INTLINES;

        // On the Impression Plus, we support shaded lines, triangles,
        // and spans, with z and double buffers.
        //
        // NOTE: With the Impression Plus, we prioritize allocation as
        //       follows: z buffer, then back buffer.  This is because
        //       of the Athena hardware bug that prevents us from ever
        //       turning off writing to z when using gauraud shading.
        //
        //       For the Millenium, assuming the bug is fixed, this
        //       should be prioritized as follows: back buffer, then
        //       z buffer.
        //
        // Also, when at 16bpp, the hardware can only accelerate if the
        // palette is 5-5-5, not 5-6-5:

        if ((ppdev->ulBoardId == MGA_PCI_4M) &&
            (ppdev->ulYDstOrg == 0)          &&
            ((ppdev->iBitmapFormat != BMF_16BPP) ||
             (ppdev->flGreen == 0x3e0)))
        {
            // We're now going to compute where the Z-buffer would appear
            // in off-screen memory.  Obviously, it would be preferable to
            // get this information from the miniport, but right now the
            // miniport doesn't provide an easy mechanism for this.

            cjVRam = 4096*1024;

            // See if there is enough memory for a back buffer.
            //
            // We need an extra scan line for a scratch area, which we'll use
            // when we have to do a dithered fill to only the active buffer
            // (not z) when running on the Athena.  Note that this requires
            // a corresponding scratch area in 'z':

            cyFrontBuffer = ppdev->cyScreen;
            cyBackBuffer  = cyFrontBuffer;

            if (ppdev->iBitmapFormat <= BMF_16BPP)
                cyBackBuffer++;     // Dithering happens only at 8 and 16bpp

            if (ppdev->iBitmapFormat == BMF_8BPP)
            {
                // When running at 8bpp on a 4MB board, we can share the z
                // buffer between the front and back buffers if we start the
                // back buffer at exactly the 1MB mark.
                //
                // I'm feeling a little lazy, and will position the back
                // buffer at the start of the first scan just after the 1MB
                // mark.  This way, we don't have to re-adjust 'YDstOrg'
                // whenever we switch active buffers between the front and
                // back buffers.

                cyZBuffer = cyBackBuffer;

                ulBackBufferStart = (1024 * 1024);

                yBackBufferStart = ulBackBufferStart / ppdev->lDelta;

                if ((ulBackBufferStart % ppdev->lDelta) != 0)
                {
                    // We have to move down a scan to get past the 1MB
                    // mark.  We also have to reserve room for an extra
                    // scan in z to account for this.
                    //
                    // Note that by doing this, we aren't perfectly
                    // aligning the z buffer between the back and front
                    // buffers.  Fortunately, the 3D DDI specifies that
                    // we don't have to preserve z on swap-buffer or
                    // switch-active-buffers commands.

                    yBackBufferStart++;
                    cyZBuffer++;
                    ulBackBufferStart = yBackBufferStart * ppdev->lDelta;
                }

                ptlBackBuffer.x = 0;
                ptlBackBuffer.y = yBackBufferStart;

                // The byte offset from the front z buffer to the back
                // z buffer is as follows, accounting for the fact that
                // the z buffer is always 16 bits:

                ulBackZBufferOffset = 2 * (ulBackBufferStart - 1024 * 1024);
            }
            else
            {
                // At other colour depths with the Athena, it is not possible
                // to share a single z buffer between the front and back
                // buffers.  We pack the front and back buffer together, and
                // allocate a z buffer that is big enough for both:

                yBackBufferStart = ppdev->cyScreen;

                ulBackBufferStart = yBackBufferStart * ppdev->lDelta;

                cyZBuffer = cyFrontBuffer + cyBackBuffer;

                ptlBackBuffer.x = 0;
                ptlBackBuffer.y = yBackBufferStart;

                // Byte offset from the front z buffer to the back z buffer:

                ulBackZBufferOffset = yBackBufferStart * 2 * ppdev->cxMemory;
            }

            pohBackBuffer = pohAllocate(ppdev,
                                        &ptlBackBuffer,
                                        ppdev->cxScreen,
                                        cyBackBuffer,
                                        FLOH_RESERVE);

            // The Z-buffer always starts at 1MB on a 2MB board, and at 2MB
            // on a 4MB board.  The start scan is computed as the floor of
            // the start offset divided by the screen stride:

            ulZBufferStart = (cjVRam / 2);

            yZBufferStart = ulZBufferStart / ppdev->lDelta;

            // Z is always 16bits, irrespective of the current resolution.
            // Unfortunately, with the Impression Plus we need room for a
            // z buffer corresponding to both the front buffer and the back
            // buffer:

            ulZBufferEnd = ulZBufferStart + cyZBuffer * (2 * ppdev->cxMemory);

            // The end 'y' must be computed as a ceiling, and is not inclusive:

            yZBufferEnd = (ulZBufferEnd - 1) / ppdev->lDelta + 1;

            ptlZBuffer.x = 0;
            ptlZBuffer.y = yZBufferStart;

            pohZBuffer = pohAllocate(ppdev,
                                     &ptlZBuffer,
                                     ppdev->cxMemory,
                                     yZBufferEnd - yZBufferStart,
                                     FLOH_RESERVE);

            if ((pohBackBuffer == NULL) || (pohZBuffer == NULL))
            {
                pohFree(ppdev, pohBackBuffer);
                pohFree(ppdev, pohZBuffer);
            }
            else
            {
                DISPDBG((0, ">> Full 3D DDI support enabled!"));

                ppdev->flHwCaps      |= HWCAPS_ALL;
                ppdev->pohBackBuffer  = pohBackBuffer;
                ppdev->ulBackBuffer   = ulBackBufferStart;
                ppdev->pohZBuffer     = pohZBuffer;
                ppdev->ulFrontZBuffer = ulZBufferStart;
                ppdev->ulBackZBuffer  = ulZBufferStart + ulBackZBufferOffset;
            }
        }
    }
    #endif

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisableRx
*
\**************************************************************************/

VOID vDisableRx(
PDEV*   ppdev)
{
    // We don't have to free any 2-d heap allocations, because disabling
    // the 2-d heap will do that automatically.
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
