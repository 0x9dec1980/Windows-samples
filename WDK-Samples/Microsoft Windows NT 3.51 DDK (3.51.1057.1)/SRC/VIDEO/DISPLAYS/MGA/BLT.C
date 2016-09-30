/******************************Module*Header*******************************\
* Module Name: blt.c
*
* Contains the low-level blt functions.
*
* Hopefully, if you're basing your display driver on this code, to
* support all of DrvBitBlt and DrvCopyBits, you'll only have to implement
* the following routines.  You shouldn't have to modify much in
* 'bitblt.c'.  I've tried to make these routines as few, modular, simple,
* and efficient as I could, while still accelerating as many calls as
* possible that would be cost-effective in terms of performance wins
* versus size and effort.
*
* Note: In the following, 'relative' coordinates refers to coordinates
*       that haven't yet had the offscreen bitmap (DFB) offset applied.
*       'Absolute' coordinates have had the offset applied.  For example,
*       we may be told to blt to (1, 1) of the bitmap, but the bitmap may
*       be sitting in offscreen memory starting at coordinate (0, 768) --
*       (1, 1) would be the 'relative' start coordinate, and (1, 769)
*       would be the 'absolute' start coordinate'.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1993-1995 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vFillSolid
*
* Fills a list of rectangles with a solid colour.
*
\**************************************************************************/

VOID vFillSolid(                // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Rop4
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulDwg;
    ULONG   ulHwMix;

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    if (rop4 == 0xf0f0)         // PATCOPY
    {
        ulDwg = opcode_TRAP + atype_RPL + blockm_ON +
                pattern_OFF + transc_BG_OPAQUE +
                bop_SRCCOPY;
    }
    else
    {
        // The ROP3 is a combination of P and D only:
        //
        //      ROP3  Mga   ROP3  Mga   ROP3  Mga   ROP3  Mga
        //
        //      0x00  0     0x50  4     0xa0  8     0xf0  c
        //      0x05  1     0x55  5     0xa5  9     0xf5  d
        //      0x0a  2     0x5a  6     0xaa  a     0xfa  e
        //      0x0f  3     0x5f  7     0xaf  b     0xff  f

        ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

        if (ulHwMix == MGA_WHITENESS)
        {
            rbc.iSolidColor = 0xffffffff;
            ulDwg = opcode_TRAP + atype_RPL + blockm_ON +
                    pattern_OFF + transc_BG_OPAQUE +
                    bop_SRCCOPY;
        }
        else if (ulHwMix == MGA_BLACKNESS)
        {
            rbc.iSolidColor = 0;
            ulDwg = opcode_TRAP + atype_RPL + blockm_ON +
                    pattern_OFF + transc_BG_OPAQUE +
                    bop_SRCCOPY;
        }
        else
        {
            ulDwg = opcode_TRAP + atype_RSTR + blockm_OFF +
                    pattern_OFF + transc_BG_OPAQUE +
                    (ulHwMix << 16);
        }
    }

    if ((ppdev->HopeFlags & (SIGN_CACHE | ARX_CACHE | PATTERN_CACHE)) ==
                            (SIGN_CACHE | ARX_CACHE | PATTERN_CACHE))
    {
        CHECK_FIFO_SPACE(pjBase, 6);
    }
    else
    {
        CHECK_FIFO_SPACE(pjBase, 15);

        if (!(ppdev->HopeFlags & SIGN_CACHE))
        {
            CP_WRITE(pjBase, DWG_SGN, 0);
        }
        if (!(ppdev->HopeFlags & ARX_CACHE))
        {
            CP_WRITE(pjBase, DWG_AR1, 0);
            CP_WRITE(pjBase, DWG_AR2, 0);
            CP_WRITE(pjBase, DWG_AR4, 0);
            CP_WRITE(pjBase, DWG_AR5, 0);
        }
        if (!(ppdev->HopeFlags & PATTERN_CACHE))
        {
            CP_WRITE(pjBase, DWG_SRC0, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC1, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC2, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC3, 0xFFFFFFFF);
        }

        ppdev->HopeFlags = (SIGN_CACHE | ARX_CACHE | PATTERN_CACHE);
    }

    CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, rbc.iSolidColor));
    CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);

    while(TRUE)
    {
        CP_WRITE(pjBase, DWG_FXLEFT,  prcl->left   + xOffset);
        CP_WRITE(pjBase, DWG_FXRIGHT, prcl->right  + xOffset);
        CP_WRITE(pjBase, DWG_LEN,     prcl->bottom - prcl->top);
        CP_START(pjBase, DWG_YDST,    prcl->top    + yOffset);

        if (--c == 0)
            return;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 4);
    }
}

/******************************Public*Routine******************************\
* VOID vFillPat1bpp
*
\**************************************************************************/

VOID vFillPat1bpp(              // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjBase;
    RBRUSH* prb;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulDwg;
    ULONG   ulHwMix;

    ASSERTDD(rbc.prb->fl & RBRUSH_2COLOR, "Must be 2 colour pattern here");

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    if ((rop4 & 0xff) == 0xf0)
    {
        ulDwg = opcode_TRAP + blockm_OFF + atype_RPL + bop_SRCCOPY;
    }
    else
    {
        ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

        ulDwg = opcode_TRAP + blockm_OFF + atype_RSTR + (ulHwMix << 16);
    }

    if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
    {
        // Normal opaque mode:

        ulDwg |= transc_BG_OPAQUE;
    }
    else
    {
        // GDI guarantees us that if the foreground and background
        // ROPs are different, the background rop is LEAVEALONE:

        ulDwg |= transc_BG_TRANSP;
    }

    if ((ppdev->HopeFlags & (SIGN_CACHE | ARX_CACHE)) == (SIGN_CACHE | ARX_CACHE))
    {
        CHECK_FIFO_SPACE(pjBase, 12);
    }
    else
    {
        CHECK_FIFO_SPACE(pjBase, 17);

        CP_WRITE(pjBase, DWG_SGN, 0);
        CP_WRITE(pjBase, DWG_AR1, 0);
        CP_WRITE(pjBase, DWG_AR2, 0);
        CP_WRITE(pjBase, DWG_AR4, 0);
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    ppdev->HopeFlags = (SIGN_CACHE | ARX_CACHE);

    CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);
    CP_WRITE(pjBase, DWG_SHIFT, ((-(pptlBrush->y + yOffset) & 7) << 4) |
                                 (-(pptlBrush->x + xOffset) & 7));

    prb = rbc.prb;
    CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, prb->ulColor[1]));
    CP_WRITE(pjBase, DWG_BCOL, COLOR_REPLICATE(ppdev, prb->ulColor[0]));
    CP_WRITE(pjBase, DWG_SRC0, prb->aulPattern[0]);
    CP_WRITE(pjBase, DWG_SRC1, prb->aulPattern[1]);
    CP_WRITE(pjBase, DWG_SRC2, prb->aulPattern[2]);
    CP_WRITE(pjBase, DWG_SRC3, prb->aulPattern[3]);

    while(TRUE)
    {
        CP_WRITE(pjBase, DWG_FXLEFT,  prcl->left   + xOffset);
        CP_WRITE(pjBase, DWG_FXRIGHT, prcl->right  + xOffset);
        CP_WRITE(pjBase, DWG_LEN,     prcl->bottom - prcl->top);
        CP_START(pjBase, DWG_YDST,    prcl->top    + yOffset);

        if (--c == 0)
            return;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 4);
    }
}

/******************************Public*Routine******************************\
* VOID vXfer1bpp
*
* This routine colour expands a monochrome bitmap.
*
\**************************************************************************/

VOID vXfer1bpp(         // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       rop4,       // Foreground and background hardware mix
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    BYTE*   pjSrcScan0;
    LONG    lSrcDelta;
    ULONG   ulDwg;
    ULONG   ulHwMix;
    ULONG*  pulXlate;
    LONG    cxDst;
    LONG    cyDst;
    LONG    xAlign;
    ULONG   cFullLoops;
    ULONG   cRemLoops;
    BYTE*   pjDma;
    ULONG*  pulSrc;
    ULONG   cdSrc;
    LONG    lSrcSkip;
    ULONG*  pulDst;
    LONG    i;
    BOOL    bHwBug;
    LONG    cFifo;

    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only an opaquing rop");

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    pjSrcScan0 = psoSrc->pvScan0;
    lSrcDelta  = psoSrc->lDelta;

    if (rop4 == 0xcccc)                 // SRCCOPY
    {
        ulDwg = opcode_ILOAD+atype_RPL+blockm_OFF+bltmod_BMONO+
                hbgr_SRC_WINDOWS+pattern_OFF+transc_BG_OPAQUE+bop_SRCCOPY;
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulDwg = opcode_ILOAD+atype_RSTR+blockm_OFF+bltmod_BMONO+
                hbgr_SRC_WINDOWS+pattern_OFF+transc_BG_OPAQUE+ (ulHwMix << 16);
    }

    CHECK_FIFO_SPACE(pjBase, 15);

    CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);

    if (!(ppdev->HopeFlags & SIGN_CACHE))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }

    if (!(ppdev->HopeFlags & ARX_CACHE))
    {
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    // The SRC0 through SRC3 registers are trashed by the blt, and
    // other ARx registers will be modified shortly, so signal it:

    ppdev->HopeFlags = SIGN_CACHE;

    pulXlate = pxlo->pulXlate;
    CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, pulXlate[1]));
    CP_WRITE(pjBase, DWG_BCOL, COLOR_REPLICATE(ppdev, pulXlate[0]));

    while (TRUE)
    {
        cxDst = (prcl->right - prcl->left);
        cyDst = (prcl->bottom - prcl->top);

        CP_WRITE(pjBase, DWG_LEN,     cyDst);
        CP_WRITE(pjBase, DWG_YDST,    prcl->top + yOffset);
        CP_WRITE(pjBase, DWG_FXLEFT,  prcl->left + xOffset);
        CP_WRITE(pjBase, DWG_FXRIGHT, prcl->right + xOffset - 1);

        xAlign = (prcl->left + dx) & 31;

        bHwBug = ((cxDst >= 128) && (xAlign <= 15));

        if (!bHwBug)
        {
            CP_WRITE(pjBase, DWG_SHIFT, 0);
            CP_WRITE(pjBase, DWG_AR3,   xAlign);
            CP_START(pjBase, DWG_AR0,   xAlign + cxDst - 1);
        }
        else
        {
            // We have to work around a hardware bug.  Start 8 pels to
            // the left of the original start.

            CP_WRITE(pjBase, DWG_AR3,   xAlign + 8);
            CP_WRITE(pjBase, DWG_AR0,   xAlign + cxDst + 31);
            CP_START(pjBase, DWG_SHIFT, (24 << 16));
        }

        // We have to ensure that the command has been started before doing
        // the BLT_WRITE_ON:

        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
        BLT_WRITE_ON(ppdev, pjBase);

        // Point to the first dword of the source bitmap that is to be
        // downloaded:

        pulSrc = (ULONG*) (pjSrcScan0 + (((prcl->top + dy) * lSrcDelta
                                      + ((prcl->left + dx) >> 3)) & ~3L));

        // Calculate the number of dwords to be moved per scanline.  Since
        // we align the starting dword on a dword boundary, we know that
        // we cannot overflow the end of the bitmap:

        cdSrc = (xAlign + cxDst + 31) >> 5;

        lSrcSkip = lSrcDelta - (cdSrc << 2);

        pjDma = ppdev->pjBase + DMAWND;

        if (!(bHwBug) && (lSrcSkip == 0))
        {
            // It's rather frequent that there will be no scan-to-scan
            // delta, and no hardware bug, so we can go full speed:

            cdSrc *= cyDst;

            cFullLoops = ((cdSrc - 1) / FIFOSIZE);
            cRemLoops = ((cdSrc - 1) % FIFOSIZE) + 1;

            pulDst = (ULONG*) pjDma;

            if (cFullLoops > 0)
            {
                do {
                    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

                    for (i = FIFOSIZE; i != 0; i--)
                    {
                        CP_WRITE_DMA(ppdev, pulDst, *pulSrc);
                        pulSrc++;
                    }
                } while (--cFullLoops != 0);
            }

            CHECK_FIFO_SPACE(pjBase, (LONG) cRemLoops);

            do {
                CP_WRITE_DMA(ppdev, pulDst, *pulSrc);
                pulSrc++;
            } while (--cRemLoops != 0);
        }
        else
        {
            // Okay, blt it the slow way:

            cFifo = 0;

            do {
                pulDst = (ULONG*) pjDma;

                if (bHwBug)
                {
                    if (--cFifo < 0)
                    {
                        cFifo = FIFOSIZE - 1;
                        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
                    }
                    CP_WRITE_DMA(ppdev, pulDst, 0);  // Account for hardware bug
                }

                for (i = cdSrc; i != 0; i--)
                {
                    if (--cFifo < 0)
                    {
                        cFifo = FIFOSIZE - 1;
                        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
                    }
                    CP_WRITE_DMA(ppdev, pulDst, *pulSrc++);
                }

                pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);

            } while (--cyDst != 0);
        }

        BLT_WRITE_OFF(ppdev, pjBase);

        if (--c == 0)
            break;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 7);
    }
}

/******************************Public*Routine******************************\
* VOID vXfer4bpp
*
* Does a 4bpp transfer from a bitmap to the screen.
*
* The reason we implement this is that a lot of resources are kept as 4bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

VOID vXfer4bpp(         // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       rop4,       // Rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPelSize;
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    xSrc;
    LONG    iLoop;
    BYTE    jSrc;
    ULONG*  pulXlate;
    ULONG   ulHwMix;
    ULONG   ulCtl;
    LONG    i;
    ULONG   ul;
    LONG    xBug;
    LONG    xAbsLeft;
    BOOL    bHwBug;
    LONG    cjSrc;
    LONG    cwSrc;
    LONG    lSrcSkip;
    LONG    cxRem;
    ULONG   ul0;
    ULONG   ul1;

    ASSERTDD(psoSrc->iBitmapFormat == BMF_4BPP, "Source must be 4bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");

    pjBase    = ppdev->pjBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    cjPelSize = ppdev->cjPelSize;
    pulXlate  = pxlo->pulXlate;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    if (rop4 == 0xcccc)         // SRCCOPY
    {
        ulCtl = (opcode_ILOAD + atype_RPL + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + bop_SRCCOPY);
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulCtl = (opcode_ILOAD + atype_RSTR + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + (ulHwMix << 16));
    }

    if (cjPelSize >= 3)
    {
        xBug = 0;
        ulCtl |= (hcprs_SRC_24_BPP | bltmod_BUCOL);
    }
    else
    {
        ulCtl |= (bltmod_BFCOL);
        xBug = (8 >> cjPelSize);    // 8bpp and 16bpp have h/w alignment bugs
    }

    CHECK_FIFO_SPACE(pjBase, 11);

    CP_WRITE(pjBase, DWG_DWGCTL, ulCtl);
    CP_WRITE(pjBase, DWG_SHIFT, 0);

    if (!(ppdev->HopeFlags & SIGN_CACHE))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }
    if (!(ppdev->HopeFlags & ARX_CACHE))
    {
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    // The SRC0 - SRC3 registers will be trashed by the blt.  AR0 will
    // be modified shortly:

    ppdev->HopeFlags = SIGN_CACHE;

    while(TRUE)
    {
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        CP_WRITE(pjBase, DWG_FXRIGHT, xOffset + prcl->right - 1);
        CP_WRITE(pjBase, DWG_YDST,    yOffset + prcl->top);
        CP_WRITE(pjBase, DWG_LEN,     cy);
        CP_WRITE(pjBase, DWG_AR3,     0);

        xSrc     =  prcl->left + dx;
        pjSrc    =  pjSrcScan0 + (prcl->top + dy) * lSrcDelta + (xSrc >> 1);

        xAbsLeft = (xOffset + prcl->left);
        CP_WRITE(pjBase, DWG_CXLEFT, xAbsLeft);

        xAbsLeft -= (xSrc & 1);         // Align to start of first source byte
        cx       += (xSrc & 1);

        bHwBug = (xAbsLeft & xBug);
        if (!bHwBug)
        {
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft);
            CP_START(pjBase, DWG_AR0,    cx - 1);
        }
        else
        {
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft - xBug);
            CP_START(pjBase, DWG_AR0,    cx + xBug - 1);
        }

        cjSrc    = (cx + 1) >> 1;   // Number of source bytes touched
        lSrcSkip = lSrcDelta - cjSrc;

        // Make sure the MGA is ready to take the data:

        CHECK_FIFO_SPACE(pjBase, 32);

        if (cjPelSize == 1)
        {
            // This part handles 8bpp output:

            cwSrc = (cjSrc >> 1);    // Number of whole source words

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                for (i = cwSrc; i != 0; i--)
                {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    ul  |= (pulXlate[jSrc & 0xf] << 8);
                    jSrc = *pjSrc++;
                    ul  |= (pulXlate[jSrc >> 4] << 16);
                    ul  |= (pulXlate[jSrc & 0xf] << 24);
                    CP_WRITE_SRC(pjBase, ul);
                }

                // Handle an odd end byte, if there is one:

                if (cjSrc & 1)
                {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    ul  |= (pulXlate[jSrc & 0xf] << 8);
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 2)
        {
            // This part handles 16bpp output:

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                i = cjSrc;
                do {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    ul  |= (pulXlate[jSrc & 0xf] << 16);
                    CP_WRITE_SRC(pjBase, ul);
                } while (--i != 0);

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else
        {
            // This part handles packed 24bpp output:

            ASSERTDD(!bHwBug, "There is no hardware bug when higher than 16bpp");

            cwSrc = (cx >> 2);      // Number of whole source words
            cxRem = (cx & 3);
            if (cxRem == 3)
            {
                // Merge this case into the whole word case:

                cwSrc++;
                cxRem = 0;
            }

            do {
                for (i = cwSrc; i != 0; i--)
                {
                    jSrc = *pjSrc++;
                    ul0  = (pulXlate[jSrc >> 4]);
                    ul1  = (pulXlate[jSrc & 0xf]);
                    ul   = ul0 | (ul1 << 24);
                    CP_WRITE_SRC(pjBase, ul);

                    jSrc = *pjSrc++;
                    ul0  = (pulXlate[jSrc >> 4]);
                    ul   = (ul1 >> 8) | (ul0 << 16);
                    CP_WRITE_SRC(pjBase, ul);

                    ul1  = (pulXlate[jSrc & 0xf]);
                    ul   = (ul1 << 8) | (ul0 >> 16);
                    CP_WRITE_SRC(pjBase, ul);
                }

                if (cxRem > 0)
                {
                    jSrc = *pjSrc++;
                    ul0  = (pulXlate[jSrc >> 4]);
                    ul1  = (pulXlate[jSrc & 0xf]);
                    ul   = ul0 | (ul1 << 24);
                    CP_WRITE_SRC(pjBase, ul);

                    if (cxRem > 1)
                    {
                        ul = (ul1 >> 8);
                        CP_WRITE_SRC(pjBase, ul);
                    }
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }

        if (--c == 0)
        {
            // Restore the clipping:

            CHECK_FIFO_SPACE(pjBase, 1);
            CP_WRITE(pjBase, DWG_CXLEFT, 0);
            return;
        }

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 7);
    }
}

/******************************Public*Routine******************************\
* VOID vXfer8bpp
*
* Does a 8bpp transfer from a bitmap to the screen.
*
* The reason we implement this is that a lot of resources are kept as 8bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

VOID vXfer8bpp(         // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       rop4,       // Rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPelSize;
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    xSrc;
    LONG    iLoop;
    ULONG*  pulXlate;
    ULONG   ulHwMix;
    ULONG   ulCtl;
    LONG    i;
    ULONG   ul;
    LONG    xBug;
    LONG    xAbsLeft;
    BOOL    bHwBug;
    LONG    cwSrc;
    LONG    cdSrc;
    LONG    lSrcSkip;
    LONG    cxRem;
    ULONG   ul0;
    ULONG   ul1;

    ASSERTDD(psoSrc->iBitmapFormat == BMF_8BPP, "Source must be 8bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(pxlo->pulXlate != NULL, "Must be a translate");

    pjBase    = ppdev->pjBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    cjPelSize = ppdev->cjPelSize;
    pulXlate  = pxlo->pulXlate;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    if (rop4 == 0xcccc)         // SRCCOPY
    {
        ulCtl = (opcode_ILOAD + atype_RPL + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + bop_SRCCOPY);
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulCtl = (opcode_ILOAD + atype_RSTR + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + (ulHwMix << 16));
    }

    if (cjPelSize >= 3)
    {
        xBug = 0;
        ulCtl |= (hcprs_SRC_24_BPP | bltmod_BUCOL);
    }
    else
    {
        ulCtl |= (bltmod_BFCOL);
        xBug = (8 >> cjPelSize);    // 8bpp and 16bpp have h/w alignment bugs
    }

    CHECK_FIFO_SPACE(pjBase, 11);

    CP_WRITE(pjBase, DWG_DWGCTL, ulCtl);
    CP_WRITE(pjBase, DWG_SHIFT, 0);

    if (!(ppdev->HopeFlags & SIGN_CACHE))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }
    if (!(ppdev->HopeFlags & ARX_CACHE))
    {
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    // The SRC0 - SRC3 registers will be trashed by the blt.  AR0 will
    // be modified shortly:

    ppdev->HopeFlags = SIGN_CACHE;

    while(TRUE)
    {
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        CP_WRITE(pjBase, DWG_FXRIGHT, xOffset + prcl->right - 1);
        CP_WRITE(pjBase, DWG_YDST,    yOffset + prcl->top);
        CP_WRITE(pjBase, DWG_LEN,     cy);
        CP_WRITE(pjBase, DWG_AR3,     0);

        xSrc     =  prcl->left + dx;
        pjSrc    =  pjSrcScan0 + (prcl->top + dy) * lSrcDelta + xSrc;
        xAbsLeft = (xOffset + prcl->left);

        bHwBug = (xAbsLeft & xBug);
        if (!bHwBug)
        {
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft);
            CP_START(pjBase, DWG_AR0,    cx - 1);
        }
        else
        {
            CP_WRITE(pjBase, DWG_CXLEFT, xAbsLeft);
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft - xBug);
            CP_START(pjBase, DWG_AR0,    cx + xBug - 1);
        }

        lSrcSkip = lSrcDelta - cx;

        // Make sure the MGA is ready to take the data:

        CHECK_FIFO_SPACE(pjBase, 32);

        if (cjPelSize == 1)
        {
            // This part handles 8bpp output:

            cdSrc = (cx >> 2);
            cxRem = (cx & 3);

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                for (i = cdSrc; i != 0; i--)
                {
                    ul  = (pulXlate[*pjSrc++]);
                    ul |= (pulXlate[*pjSrc++] << 8);
                    ul |= (pulXlate[*pjSrc++] << 16);
                    ul |= (pulXlate[*pjSrc++] << 24);
                    CP_WRITE_SRC(pjBase, ul);
                }

                if (cxRem > 0)
                {
                    ul = (pulXlate[*pjSrc++]);
                    if (cxRem > 1)
                    {
                        ul |= (pulXlate[*pjSrc++] << 8);
                        if (cxRem > 2)
                        {
                            ul |= (pulXlate[*pjSrc++] << 16);
                        }
                    }
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 2)
        {
            // This part handles 16bpp output:

            cwSrc = (cx >> 1);
            cxRem = (cx & 1);

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                for (i = cwSrc; i != 0; i--)
                {
                    ul  = (pulXlate[*pjSrc++]);
                    ul |= (pulXlate[*pjSrc++] << 16);
                    CP_WRITE_SRC(pjBase, ul);
                }

                if (cxRem > 0)
                {
                    ul = (pulXlate[*pjSrc++]);
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else
        {
            // This part handles packed 24bpp output:

            ASSERTDD(!bHwBug, "There is no hardware bug when higher than 16bpp");

            cdSrc = (cx >> 2);
            cxRem = (cx & 3);

            do {
                for (i = cdSrc; i != 0; i--)
                {
                    ul0  = (pulXlate[*pjSrc++]);
                    ul1  = (pulXlate[*pjSrc++]);
                    ul   = ul0 | (ul1 << 24);
                    CP_WRITE_SRC(pjBase, ul);

                    ul0  = (pulXlate[*pjSrc++]);
                    ul   = (ul1 >> 8) | (ul0 << 16);
                    CP_WRITE_SRC(pjBase, ul);

                    ul1  = (pulXlate[*pjSrc++]);
                    ul   = (ul1 << 8) | (ul0 >> 16);
                    CP_WRITE_SRC(pjBase, ul);
                }

                if (cxRem > 0)
                {
                    ul0 = (pulXlate[*pjSrc++]);
                    ul  = ul0;
                    if (cxRem > 1)
                    {
                        ul1 = (pulXlate[*pjSrc++]);
                        ul |= (ul1 << 24);
                        CP_WRITE_SRC(pjBase, ul);

                        ul = (ul1 >> 8);
                        if (cxRem > 2)
                        {
                            ul0 = (pulXlate[*pjSrc++]);
                            ul |= (ul0 << 16);
                            CP_WRITE_SRC(pjBase, ul);
                            ul = (ul0 >> 16);
                        }
                    }
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }

        if (bHwBug)
        {
            // Restore the clipping:

            CHECK_FIFO_SPACE(pjBase, 1);
            CP_WRITE(pjBase, DWG_CXLEFT, 0);
        }

        if (--c == 0)
            return;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 7);
    }
}

/******************************Public*Routine******************************\
* VOID vXferNative
*
* Transfers a bitmap that is the same colour depth as the display to
* the screen via the data transfer register, with no translation.
*
\**************************************************************************/

VOID vXferNative(       // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       rop4,       // Rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPel;
    LONG    dx;
    LONG    dy;
    BYTE*   pjSrcScan0;
    LONG    lSrcDelta;
    ULONG   ulCtl;
    ULONG   ulHwMix;
    LONG    yTop;
    LONG    xLeft;
    LONG    xAbsLeft;
    LONG    xBug;
    LONG    xRight;
    LONG    cy;
    LONG    xOriginalLeft;
    BYTE*   pjSrc;
    LONG    cdSrc;

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;
    cjPel   = ppdev->cjPelSize;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    pjSrcScan0 = psoSrc->pvScan0;
    lSrcDelta  = psoSrc->lDelta;

    if (rop4 == 0xcccc)         // SRCCOPY
    {
        ulCtl = (opcode_ILOAD + atype_RPL + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + bop_SRCCOPY);
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulCtl = (opcode_ILOAD + atype_RSTR + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + (ulHwMix << 16));
    }

    if (ppdev->iBitmapFormat == BMF_24BPP)
    {
        ulCtl |= (hcprs_SRC_24_BPP | bltmod_BUCOL);
    }
    else
    {
        ulCtl |= (bltmod_BFCOL);
    }

    CHECK_FIFO_SPACE(pjBase, 11);

    CP_WRITE(pjBase, DWG_DWGCTL, ulCtl);
    CP_WRITE(pjBase, DWG_SHIFT, 0);

    if (!(ppdev->HopeFlags & SIGN_CACHE))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }
    if (!(ppdev->HopeFlags & ARX_CACHE))
    {
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    // The SRC0 - SRC3 registers will be trashed by the blt.  AR0 will
    // be modified shortly:

    ppdev->HopeFlags = SIGN_CACHE;

    while (TRUE)
    {
        yTop          = prcl->top;
        cy            = prcl->bottom - yTop;
        xRight        = prcl->right;
        xLeft         = prcl->left;
        xOriginalLeft = xLeft;

        // Adjust the destination so that the source is dword aligned.
        // Note that this works at 24bpp (but is less restrictive than
        // it could be at 16bpp):

        xLeft -= (xLeft + dx) & 3;

        // Since we're using hardware clipping, the start is always
        // dword aligned:

        pjSrc = pjSrcScan0 + (yTop + dy) * lSrcDelta + ((xLeft + dx) * cjPel);
        cdSrc = ((xRight - xLeft) * cjPel + 3) >> 2;

        CP_WRITE(pjBase, DWG_FXRIGHT, xOffset + xRight - 1);
        CP_WRITE(pjBase, DWG_YDST,    yOffset + yTop);
        CP_WRITE(pjBase, DWG_LEN,     cy);
        CP_WRITE(pjBase, DWG_AR3,     0);

        xAbsLeft = (xOffset + xLeft);
        xBug     = (8 >> cjPel);                // 4 for 8bpp, 2 for 16bpp

        if (!(xAbsLeft & xBug) || (cjPel > 2))  // 24bpp doesn't have h/w bug
        {
            // Don't have to work-around the hardware bug:

            if (xLeft != xOriginalLeft)
            {
                // Since we always dword align the source by adjusting
                // the destination rectangle, we may have to set the clip
                // register to compensate:

                CP_WRITE(pjBase, DWG_CXLEFT, xOffset + xOriginalLeft);
            }

            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft);
            CP_START(pjBase, DWG_AR0,    xRight - xLeft - 1);

            // Make sure the MGA is ready to take the data:

            CHECK_FIFO_SPACE(pjBase, 32);

            do {
                DATA_TRANSFER(pjBase, pjSrc, cdSrc);
                pjSrc += lSrcDelta;
            } while (--cy != 0);

            if (xLeft != xOriginalLeft)
            {
                CHECK_FIFO_SPACE(pjBase, 1);
                CP_WRITE(pjBase, DWG_CXLEFT, 0);
            }
        }
        else
        {
            // Work-around the hardware bug:

            CP_WRITE(pjBase, DWG_CXLEFT, xOffset + xOriginalLeft);
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft - xBug);
            CP_START(pjBase, DWG_AR0,    xRight - xLeft + xBug - 1);

            // Make sure the MGA is ready to take the data:

            CHECK_FIFO_SPACE(pjBase, 32);

            do {
                DATA_TRANSFER(pjBase, pjSrc, 1);    // Account for h/w bug
                DATA_TRANSFER(pjBase, pjSrc, cdSrc);
                pjSrc += lSrcDelta;
            } while (--cy != 0);

            CHECK_FIFO_SPACE(pjBase, 1);
            CP_WRITE(pjBase, DWG_CXLEFT, 0);
        }

        if (--c == 0)
            break;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 7);
    }
}

/******************************Public*Routine******************************\
* VOID vCopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
* Note: We may call this function with weird ROPs to do colour-expansion
*       from off-screen monochrome bitmaps.
*
\**************************************************************************/

VOID vCopyBlt(      // Type FNCOPY
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ULONG   rop4,       // Rop4
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    FLONG   flDirCode;
    LONG    lSignedPitch;
    ULONG   ulHwMix;
    ULONG   ulDwg;
    LONG    yDst;
    LONG    ySrc;
    LONG    cy;
    LONG    xSrc;
    LONG    lSignedWidth;
    LONG    lSrcStart;

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;         // Add to destination to get source

    flDirCode    = DRAWING_DIR_TBLR;
    lSignedPitch = ppdev->cxMemory;

    // If the destination and source rectangles overlap, we will have to
    // tell the accelerator in which direction the copy should be done:

    if (OVERLAP(prclDst, pptlSrc))
    {
        if (prclDst->left > pptlSrc->x)
        {
            flDirCode |= scanleft_RIGHT_TO_LEFT;
        }
        if (prclDst->top > pptlSrc->y)
        {
            flDirCode |= sdy_BOTTOM_TO_TOP;
            lSignedPitch = -lSignedPitch;
        }
    }

    if (rop4 == 0xcccc)
    {
        ulDwg = opcode_BITBLT + atype_RPL + blockm_OFF + bltmod_BFCOL +
                pattern_OFF + transc_BG_OPAQUE + bop_SRCCOPY;
    }
    else if (rop4 == 0xaacc)
    {
        // Special set-up for circle caching:

        ulDwg = opcode_BITBLT + atype_RPL + blockm_OFF + bltmod_BPLAN +
                pattern_OFF + transc_BG_TRANSP + bop_SRCCOPY;
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulDwg = opcode_BITBLT + atype_RSTR + blockm_OFF + bltmod_BFCOL +
                pattern_OFF + transc_BG_OPAQUE + (ulHwMix << 16);
    }

    // The SRC0 to SRC3 registers are probably trashed by the blt, and we
    // may be using a different SGN:

    ppdev->HopeFlags = 0;

    CHECK_FIFO_SPACE(pjBase, 10);

    CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);
    CP_WRITE(pjBase, DWG_SHIFT,  0);
    CP_WRITE(pjBase, DWG_SGN,    flDirCode);
    CP_WRITE(pjBase, DWG_AR5,    lSignedPitch);

    while (TRUE)
    {
        CP_WRITE(pjBase, DWG_LEN,     prcl->bottom - prcl->top);
        CP_WRITE(pjBase, DWG_FXLEFT,  prcl->left  + xOffset);
        CP_WRITE(pjBase, DWG_FXRIGHT, prcl->right + xOffset - 1);

        yDst = yOffset + prcl->top;
        ySrc = yOffset + prcl->top + dy;
        if (flDirCode & sdy_BOTTOM_TO_TOP)
        {
            cy = prcl->bottom - prcl->top - 1;
            yDst += cy;
            ySrc += cy;
        }

        CP_WRITE(pjBase, DWG_YDST, yDst);

        xSrc         = xOffset + prcl->left + dx;
        lSignedWidth = prcl->right - prcl->left - 1;
        if (flDirCode & scanleft_RIGHT_TO_LEFT)
        {
            xSrc += lSignedWidth;
            lSignedWidth = -lSignedWidth;
        }

        lSrcStart = ppdev->ulYDstOrg + (ySrc * ppdev->cxMemory) + xSrc;
        CP_WRITE(pjBase, DWG_AR3, lSrcStart);
        CP_START(pjBase, DWG_AR0, lSrcStart + lSignedWidth);

        if (--c == 0)
            break;

        CHECK_FIFO_SPACE(pjBase, 6);
        prcl++;
    }
}
