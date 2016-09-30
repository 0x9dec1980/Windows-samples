/******************************Module*Header*******************************\
* Module Name: rxspan.c
*
* Copyright (c) 1995 Microsoft Corporation
\****************************************************************************/

#include "precomp.h"
#include "rxddi.h"
#include "rxhw.h"

#define USE_LINES   1

void RxPolyDrawSpan(RXESCAPE *pEsc)
{
    RXRC *pRc = pEsc->pRc;
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    ENUMRECTS *pClipList;
    RECTL *pWindowRect = &(pEsc->pwo->rclClient);
    char *maxPtr;
    RXPOLYDRAWSPAN rxDrawSpan;
    ULONG numSpans;
    RXSPAN *pSpan;
    RXCOLORREFAF *pColor;
    RXZTEX *pzTex;
    UCHAR *pNextSpan;
    ULONG *pMaskStart;
    ULONG scanStride;
    LONG dr, dg, db, dz;
    BOOL bFlat;
    CHAR cFifo;

    NEXT_COMMAND(rxDrawSpan);

    if (!(numSpans = rxDrawSpan.numSpans))
        return;

    if ((!pEsc->prxMemCache) ||
        (rxDrawSpan.hrxSharedMem != pEsc->prxMemCache->hrxSharedMem)) {
        pEsc->prxMemCache =
            GetPtrFromHandle(rxDrawSpan.hrxSharedMem,
                             RXHANDLE_SHAREMEM);

        if (!pEsc->prxMemCache)
            ERROR_RETURN;
    }

    if (!(pClipList = pEsc->pWnd->prxClipScissored))
        return;

    (char *)pSpan = (char *)rxDrawSpan.pSharedMem +
                    pEsc->prxMemCache->addrOffset;
    pColor = (RXCOLORREFAF *)((char *)pSpan + sizeof(RXSPAN));
    pzTex = (RXZTEX *)((char *)pColor + pRc->spanColorStride);

    CHECK_SHARED_MEM_RANGE(pSpan, pEsc->prxMemCache);

    maxPtr = pEsc->prxMemCache->pMemMax;

    switch (pRc->spanType)
    {
        case RX_SPAN_COLOR:
            scanStride = sizeof(RXSPAN) + pRc->spanColorStride;
            break;
        case RX_SPAN_COLOR_Z:
            scanStride = sizeof(RXSPAN) + pRc->spanColorStride +
                         sizeof(ULONG);
            break;
        case RX_SPAN_COLOR_Z_TEX:
            scanStride = sizeof(RXSPAN) + pRc->spanColorStride +
                         sizeof(RXZTEX);
            break;
        default:
            RXDBG_PRINT("DrawPolySpan: - unknown span type %d",
                        pRc->spanType);
            ERROR_RETURN;
    }

    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
    if ((pRc->spanType > RX_SPAN_COLOR) && (pRc->zEnable))
    {
        CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwSpanFunc | zdrwen_DEPTH);
    }
    else
    {
        CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwSpanFunc);
    }

    CP_WRITE(pjBase, DWG_SHIFT, 0);
    cFifo = 30;

    while (numSpans--)
    {
        pNextSpan = (UCHAR *)pSpan;

        // advance the buffer pointer for current command
        pNextSpan += scanStride;

        if (pNextSpan > maxPtr)
            ERROR_RETURN;

        if (pSpan->flags & RX_SPAN_MASK)
        {
            pMaskStart = (ULONG *)pNextSpan;
            pNextSpan += (((pSpan->count + 31) >> 3) & (~0x3));
        }

        // make sure we don't read past the end of the buffer

        if (pNextSpan > maxPtr)
            ERROR_RETURN;

        // first, check span flags for delta or fill color

        if (pSpan->flags & RX_SPAN_DELTA)
        {
            dr = pColor->r >> 1;
            dg = pColor->g >> 1;
            db = pColor->b >> 1;

            CHECK_FIFO_FREE(pjBase, cFifo, 3);

            CP_WRITE(pjBase, DWG_DR6,  dr);
            CP_WRITE(pjBase, DWG_DR10, dg);
            CP_WRITE(pjBase, DWG_DR14, db);

            bFlat = ((dr | dg | db) == 0);

            if (pRc->spanType >= RX_SPAN_COLOR_Z) {
                dz = pzTex->z >> 1;
                CHECK_FIFO_FREE(pjBase, cFifo, 1);
                CP_WRITE(pjBase, DWG_DR2, dz);
            }
        }
        else
        {
            LONG nClip, maxClip;
            LONG yPos;
            LONG hwYPos;
            RECTL *pClip;

            // If we've gotten here, we're drawing a span

            maxClip = pClipList->c;

            yPos = pSpan->y + pWindowRect->top;
#ifdef USE_LINES
            hwYPos = (yPos + pRc->hwBufferOffset) << 16;
#else
            hwYPos = yPos + pRc->hwBufferOffset;
#endif

            for (nClip = 0, pClip = &pClipList->arcl[0]; nClip < maxClip;
                 nClip++, pClip++)
            {
                LONG count;
                LONG xStart;
                LONG xEnd;
                LONG xAdj = 0;
                RXREAL rVal;
                RXREAL gVal;
                RXREAL bVal;
                ULONG zVal;

                // if the y value for the scan is less than the top
                // of the current clip rectangle, we're done looping since
                // there is no way any other cliprects can intersect the span

                if (yPos < pClip->top)
                    break;

                count = pSpan->count;
                xStart = pWindowRect->left + pSpan->x;
                xEnd = xStart + count;

                // trivially-reject any spans completelely outside the cliprect

                if ((yPos >= pClip->bottom) ||
                    (xStart >= pClip->right) ||
                    (xEnd < pClip->left))
                    continue;

                // Now, clip both ends.  If we clip the start of the span, we
                // will need to advance all of the interpolated values.  We'll
                // do this in software instead of resetting the clipping
                // register each time through, although it's probably a
                // tossup in terms of speed...


                if (xEnd >= pClip->right)
                    count -= (xEnd - pClip->right);

                if (xStart < pClip->left) {
                    xAdj = pClip->left - xStart;
                    xStart = pClip->left;
                    count -= xAdj;
                }

                if (!count)
                    break;

                // Now, draw the span:

                switch(pRc->spanType)
                {
                    case RX_SPAN_COLOR_Z_TEX:
                    case RX_SPAN_COLOR_Z:
                        if (xAdj)
                            zVal = (pzTex->z >> 1) + (xAdj * dz);
                        else
                            zVal = (pzTex->z >> 1);

                    case RX_SPAN_COLOR:
                        if (xAdj) {
                            rVal = (pColor->r >> 1) + (xAdj * dr);
                            gVal = (pColor->g >> 1) + (xAdj * dg);
                            bVal = (pColor->b >> 1) + (xAdj * db);
                        } else {
                            rVal = (pColor->r >> 1);
                            gVal = (pColor->g >> 1);
                            bVal = (pColor->b >> 1);
                        }

                        if (!pRc->ditherEnable) {
                            rVal = ((rVal >> pRc->rShift) & (~0x7fff)) << pRc->rShift;
                            gVal = ((gVal >> pRc->gShift) & (~0x7fff)) << pRc->gShift;
                            bVal = ((bVal >> pRc->bShift) & (~0x7fff)) << pRc->bShift;
                        } 

                        if (pSpan->flags & RX_SPAN_MASK)
                        {
                            LONG pixCount = count;
                            ULONG xPos = xStart;
                            ULONG maskVal;
                            ULONG *pMask;

                            // If we're flat-shading, set the color once:

                            if (bFlat) {
                                CHECK_FIFO_FREE(pjBase, cFifo, 3);
                                CP_WRITE(pjBase, DWG_DR4,  rVal);
                                CP_WRITE(pjBase, DWG_DR8,  gVal);
                                CP_WRITE(pjBase, DWG_DR12, bVal);
                            }

                            pMask = pMaskStart + (xAdj >> 5);
                            xAdj &= 0x1f;
                            maskVal = 0x80000000 >> xAdj;

                            while (pixCount > 0) {
                                ULONG ix = pixCount;
                                ULONG mask = *pMask++;

                                if ((ix + xAdj) > 32)
                                    ix = 32 - xAdj;

                                if (mask == 0xffffffff)
                                {

                                    if (!bFlat) {
                                        CHECK_FIFO_FREE(pjBase, cFifo, 3);
                                        CP_WRITE(pjBase, DWG_DR4,  rVal);
                                        CP_WRITE(pjBase, DWG_DR8,  gVal);
                                        CP_WRITE(pjBase, DWG_DR12, bVal);
                                    }

#if USE_LINES
                                    CHECK_FIFO_FREE(pjBase, cFifo, 3);

                                    CP_WRITE(pjBase, DWG_DR0, zVal);

                                    CP_WRITE(pjBase, DWG_XYSTRT,
                                        (ULONG)(xPos | hwYPos));
                                    CP_START(pjBase, DWG_XYEND,
                                        (ULONG)((xPos + ix - 1) | hwYPos));
#else
                                    CHECK_FIFO_FREE(pjBase, cFifo, 5);

                                    CP_WRITE(pjBase, DWG_DR0, zVal);

                                    CP_WRITE(pjBase, DWG_FXLEFT, xPos);
                                    CP_WRITE(pjBase, DWG_FXRIGHT, xPos + ix);
                                    CP_WRITE(pjBase, DWG_LEN, 1);
                                    CP_START(pjBase, DWG_YDST, hwYPos);
#endif
                                    if (ix == 32) {
                                        rVal += dr << 5;
                                        gVal += dg << 5;
                                        bVal += db << 5;
                                        zVal += dz << 5;
                                    } else {
                                        rVal += (dr * ix);
                                        gVal += (dg * ix);
                                        bVal += (db * ix);
                                        zVal += (dz * ix);
                                    }
                                    xPos += ix;
                                } else if (!mask) {
                                    if (ix == 32) {
                                        rVal += dr << 5;
                                        gVal += dg << 5;
                                        bVal += db << 5;
                                        zVal += dz << 5;
                                    } else {
                                        rVal += (dr * ix);
                                        gVal += (dg * ix);
                                        bVal += (db * ix);
                                        zVal += (dz * ix);
                                    }
                                    xPos += ix;
                                } else {
                                    ULONG maskBit = maskVal;
                                    ULONG i;

                                    for (i = 0; i < ix; i++, maskBit >>= 1) {
                                        if (mask & maskBit)
                                        {
                                            if (!bFlat) {
                                                CHECK_FIFO_FREE(pjBase, cFifo, 3);

                                                CP_WRITE(pjBase, DWG_DR4,  rVal);
                                                CP_WRITE(pjBase, DWG_DR8,  gVal);
                                                CP_WRITE(pjBase, DWG_DR12, bVal);
                                            }

#if USE_LINES
                                            CHECK_FIFO_FREE(pjBase, cFifo, 3);

                                            CP_WRITE(pjBase, DWG_DR0, zVal);

                                            CP_WRITE(pjBase, DWG_XYSTRT,
                                                (ULONG)(xPos | hwYPos));
                                            CP_START(pjBase, DWG_XYEND,
                                                (ULONG)(xPos | hwYPos));
#else
                                            CHECK_FIFO_FREE(pjBase, cFifo, 5);

                                            CP_WRITE(pjBase, DWG_DR0, zVal);

                                            CP_WRITE(pjBase, DWG_FXLEFT, xPos);
                                            CP_WRITE(pjBase, DWG_FXRIGHT, xPos + 1);
                                            CP_WRITE(pjBase, DWG_LEN, 1);
                                            CP_START(pjBase, DWG_YDST, hwYPos);

#endif
                                        }

                                        xPos++;
                                        rVal += dr;
                                        gVal += dg;
                                        bVal += db;
                                        zVal += dz;
                                    }
                                }

                                pixCount -= ix;
                                maskVal = 0x80000000;
                                xAdj = 0;
                            }
                        }
                        else
                        {
                            CHECK_FIFO_FREE(pjBase, cFifo, 3);
                            CP_WRITE(pjBase, DWG_DR4,  rVal);
                            CP_WRITE(pjBase, DWG_DR8,  gVal);
                            CP_WRITE(pjBase, DWG_DR12, bVal);

#ifdef USE_LINES
	                    CHECK_FIFO_FREE(pjBase, cFifo, 3);

                            CP_WRITE(pjBase, DWG_DR0, zVal);

                            CP_WRITE(pjBase, DWG_XYSTRT,
                                (ULONG)(xStart | hwYPos));
                            CP_START(pjBase, DWG_XYEND,
                                (ULONG)((xStart + count - 1) | hwYPos));
#else
	                    CHECK_FIFO_FREE(pjBase, cFifo, 5);

                            CP_WRITE(pjBase, DWG_DR0, zVal);

                            CP_WRITE(pjBase, DWG_FXLEFT, xStart);
                            CP_WRITE(pjBase, DWG_FXRIGHT, xStart + count);
                            CP_WRITE(pjBase, DWG_LEN, 1);
                            CP_START(pjBase, DWG_YDST, hwYPos);
#endif
                        }

                        break;

                    default:
                        RXDBG_PRINT("DrawPolySpan - Unknown span type: %d\n",
                                    pRc->spanType);
                        ERROR_RETURN;
                }
            }
        }

        // Advance to the nex span:

        pSpan = (RXSPAN *)pNextSpan;
        pColor = (RXCOLORREFAF *)((char *)pSpan + sizeof(RXSPAN));
        pzTex = (RXZTEX *)((char *)pColor + pRc->spanColorStride);
    }
}

