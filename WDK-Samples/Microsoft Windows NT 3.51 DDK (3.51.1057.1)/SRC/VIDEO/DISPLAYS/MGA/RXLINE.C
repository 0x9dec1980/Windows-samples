/******************************Module*Header*******************************\
* Module Name: rxline.c
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#include "rxddi.h"
#include "rxhw.h"


#ifdef _X86_

#pragma warning(disable:4035)

// (a * b) / divisor

__inline LONG MULDIV(LONG a, LONG b, LONG divisor)
{
    if ((divisor < 0xffff) && (divisor > -0xffff))
        return 0;
    _asm {
        mov     eax, a
        imul    dword ptr b
        idiv    divisor
    }
}

// (a * b) >> 16

__inline LONG FIXMUL(LONG a, LONG b)
{
    _asm {
        mov     eax, a
        imul    b
        shrd    eax, edx, 16
    }
}

// (a << 16) / b

__inline LONG FIXDIV(LONG a, LONG b)
{
    _asm {
        mov     eax, a
        cdq
        shld    edx, eax, 16
        shl     eax, 16
        idiv    b
    }
}

#pragma warning(default:4035)

#else

#define MULDIV(a, b, div)\
    (LONG)(((LONGLONG)(a) * (LONGLONG)(b)) / (LONGLONG)(div))

#define FIXMUL(a, b)\
    (LONG)(((LONGLONG)(a) * (LONGLONG)(b)) >> 16)

#define FIXDIV(a, b)\
    (LONG)(((LONGLONG)(a) << 16) / (LONGLONG)(b))

#endif

__inline void SetupIntLine(RXESCAPE *pEsc, RXRC *pRc)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_SPACE(pjBase, 7);
    CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwIntLineFunc);
    CP_WRITE(pjBase, DWG_FCOL, pRc->hwSolidColor);
    CP_WRITE(pjBase, DWG_SRC0, 0xffffffff);
    CP_WRITE(pjBase, DWG_SRC1, 0xffffffff);
    CP_WRITE(pjBase, DWG_SRC2, 0xffffffff);
    CP_WRITE(pjBase, DWG_SRC3, 0xffffffff);
    CP_WRITE(pjBase, DWG_SHIFT, 0x007f007f);
}


__inline void SetupLine(RXESCAPE *pEsc, RXRC *pRc)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
    pRc->hwFifo = 32;

    CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 7);
    if ((pRc->vertexType > RX_POINT) && (pRc->zEnable))
    {
        CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwLineFunc | zdrwen_DEPTH);
    }
    else
    {
        CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwLineFunc);
    }
    CP_WRITE(pjBase, DWG_FCOL, pRc->hwSolidColor);
    CP_WRITE(pjBase, DWG_SRC0, 0xffffffff);
    CP_WRITE(pjBase, DWG_SRC1, 0xffffffff);
    CP_WRITE(pjBase, DWG_SRC2, 0xffffffff);
    CP_WRITE(pjBase, DWG_SRC3, 0xffffffff);
    CP_WRITE(pjBase, DWG_SHIFT, 0);
}


__inline void DrawFastLine(RXESCAPE *pEsc, RXRC *pRc,
                           LONG x1, LONG y1, LONG x2, LONG y2)
{
    ULONG i;
    ULONG maxClip = pEsc->pWnd->prxClipScissored->c;
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    RECTL *pClip;

    if ((y1 < 0) && (y2 < 0))
        return;

    if (y1 < 0) {
        LONG temp;

        temp = y2;
        y2 = y1;
        y1 = temp;
        temp = x2;
        x2 = x1;
        x1 = temp;
    }

    CHECK_FIFO_SPACE(pjBase, 2);
    CP_WRITE(pjBase, DWG_XYSTRT, (ULONG)((x1 & 0xffff) | (y1 << 16)));
    CP_START(pjBase, DWG_XYEND, (ULONG)((x2 & 0xffff) | (y2 << 16)));
}


__inline void SetLineClippingAndPattern(RXESCAPE *pEsc, RECTL *pClip)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    RXRC *pRc = pEsc->pRc;

    CHECK_FIFO_SPACE(pjBase, 4);
    CP_WRITE(pjBase, DWG_CYTOP,
                  ((pClip->top + pRc->hwBufferOffset) * ppdev->cxMemory) +
                  ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_CXLEFT, pClip->left);
    CP_WRITE(pjBase, DWG_CXRIGHT, pClip->right - 1);
    CP_WRITE(pjBase, DWG_CYBOT,
                 ((pClip->bottom + pRc->hwBufferOffset - 1) * ppdev->cxMemory) +
                  ppdev->ulYDstOrg);

    if (pRc->linePattern.linePattern != 0xffff) {
        CHECK_FIFO_SPACE(pjBase, 2);
        CP_WRITE(pjBase, DWG_SRC0, pRc->linePattern.linePattern |
                                   (pRc->linePattern.linePattern << 16));
        CP_WRITE(pjBase, DWG_SHIFT, 0x000f000f);
    }
}


void DrawLine(RXESCAPE *pEsc, RXPOINTZTEX **pVertex,
              RXCOLOR **pColor)
{
    RXRC *pRc = pEsc->pRc;
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    RXREAL drdx, drdy;	    // delta values
    RXREAL dgdx, dgdy;
    RXREAL dbdx, dbdy;
    RXREAL dzdx, dzdy;
    RXPOINTZTEX *p1, *p2;
    RXCOLOR *c1, *c2;
    RXREAL dx, dy;
    RXREAL iy1, iy2, ix1, ix2;
    RXREAL idy;
    RXREAL absDx, absDy;
    ULONG clipNum;
    RECTL *pClip;
    RXREAL rStart, gStart, bStart, zStart;
    RXREAL startCoord;
    RXREAL endCoord;

    p1 = pVertex[0];
    p2 = pVertex[1];
    c1 = pColor[0];
    c2 = pColor[1];

    if ((dx = p2->x - p1->x) < 0) {
        absDx = -dx;
    } else {
        absDx = dx;
    }

    if ((dy = p2->y - p1->y) < 0) {
        absDy = -dy;
    } else {
        absDy = dy;
    }

    if (absDx > absDy) {

        iy1 = (p1->y >> 16);
        iy2 = (p2->y >> 16);

        // Prevent division overflows:

        if (absDx < 0x10000)
            absDx = 0x10000;

        if (dx > 0) {

        // +X major

            ix1 = (p1->x >> 16);
            ix2 = (p2->x >> 16);

            if ((ix2 - ix1) <= 0)
                return;

        } else {

        // -X major

            ix1 = (p1->x >> 16);
            ix2 = (p2->x >> 16);

            if ((ix1 - ix2) <= 0)
                return;
        }

        if (pRc->shadeMode == RX_SMOOTH) {
            drdx = FIXDIV(TOFIX(c2->r - c1->r), absDx);
            dgdx = FIXDIV(TOFIX(c2->g - c1->g), absDx);
            dbdx = FIXDIV(TOFIX(c2->b - c1->b), absDx);

            rStart = (ULONG)(TOFIX(c1->r)) >> 1;
            gStart = (ULONG)(TOFIX(c1->g)) >> 1;
            bStart = (ULONG)(TOFIX(c1->b)) >> 1;

            CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 9);

            CP_WRITE(pjBase, DWG_DR6,  drdx >> 1);
            CP_WRITE(pjBase, DWG_DR7,  drdx >> 1);
            CP_WRITE(pjBase, DWG_DR10, dgdx >> 1);
            CP_WRITE(pjBase, DWG_DR11, dgdx >> 1);
            CP_WRITE(pjBase, DWG_DR14, dbdx >> 1);
            CP_WRITE(pjBase, DWG_DR15, dbdx >> 1);

            CP_WRITE(pjBase, DWG_DR4,  rStart);
            CP_WRITE(pjBase, DWG_DR8,  gStart);
            CP_WRITE(pjBase, DWG_DR12, bStart);

        } else if (pRc->shadeMode == RX_FLAT) {

            CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 3);

            CP_WRITE(pjBase, DWG_DR4,  (ULONG)(TOFIX(c1->r)) >> 1);
            CP_WRITE(pjBase, DWG_DR8,  (ULONG)(TOFIX(c1->g)) >> 1);
            CP_WRITE(pjBase, DWG_DR12, (ULONG)(TOFIX(c1->b)) >> 1);
        }

        if (pRc->zEnable) {
            dzdx = FIXDIV(p2->z - p1->z, absDx);

            zStart = (ULONG)(p1->z) >> 1;

            CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 3);

            CP_WRITE(pjBase, DWG_DR2, dzdx >> 1);
            CP_WRITE(pjBase, DWG_DR3, dzdx >> 1);
            CP_WRITE(pjBase, DWG_DR0, zStart);
        }

    } else {

        ix1 = (p1->x >> 16);
        ix2 = (p2->x >> 16);

        // Prevent division overflows:

        if (absDy < 0x10000)
            absDy = 0x10000;

        if (dy > 0) {

        // +Y major

            iy1 = (p1->y >> 16);
            iy2 = (p2->y >> 16);

            if ((iy2 - iy1) <= 0)
                return;

        } else {

        // -Y major

            iy1 = (p1->y >> 16);
            iy2 = (p2->y >> 16);

            if ((iy1 - iy2) <= 0)
                return;
        }

        if (pRc->shadeMode == RX_SMOOTH) {
            drdy = FIXDIV(TOFIX(c2->r - c1->r), absDy);
            dgdy = FIXDIV(TOFIX(c2->g - c1->g), absDy);
            dbdy = FIXDIV(TOFIX(c2->b - c1->b), absDy);

            rStart = (ULONG)(TOFIX(c1->r)) >> 1;
            gStart = (ULONG)(TOFIX(c1->g)) >> 1;
            bStart = (ULONG)(TOFIX(c1->b)) >> 1;

            CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 9);

            CP_WRITE(pjBase, DWG_DR6,  drdy >> 1);
            CP_WRITE(pjBase, DWG_DR7,  drdy >> 1);
            CP_WRITE(pjBase, DWG_DR10, dgdy >> 1);
            CP_WRITE(pjBase, DWG_DR11, dgdy >> 1);
            CP_WRITE(pjBase, DWG_DR14, dbdy >> 1);
            CP_WRITE(pjBase, DWG_DR15, dbdy >> 1);

            CP_WRITE(pjBase, DWG_DR4,  (ULONG)(TOFIX(c1->r)) >> 1);
            CP_WRITE(pjBase, DWG_DR8,  (ULONG)(TOFIX(c1->g)) >> 1);
            CP_WRITE(pjBase, DWG_DR12, (ULONG)(TOFIX(c1->b)) >> 1);

        } else if (pRc->shadeMode == RX_FLAT) {

            CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 3);

            CP_WRITE(pjBase, DWG_DR4,  (ULONG)(TOFIX(c1->r)) >> 1);
            CP_WRITE(pjBase, DWG_DR8,  (ULONG)(TOFIX(c1->g)) >> 1);
            CP_WRITE(pjBase, DWG_DR12, (ULONG)(TOFIX(c1->b)) >> 1);
        }

        if (pRc->zEnable) {
            dzdy = FIXDIV(p2->z - p1->z, absDy);

            zStart = (ULONG)(p1->z) >> 1;

            CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 3);

            CP_WRITE(pjBase, DWG_DR2, dzdy >> 1);
            CP_WRITE(pjBase, DWG_DR3, dzdy >> 1);
            CP_WRITE(pjBase, DWG_DR0, zStart);

        }
    }

    if ((clipNum = pEsc->pWnd->prxClipScissored->c) > 1) {
        pClip = &pEsc->pWnd->prxClipScissored->arcl[0];

        CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 4);

        CP_WRITE(pjBase, DWG_CYTOP,
                      ((pClip->top + pRc->hwBufferOffset) * ppdev->cxMemory) +
                      ppdev->ulYDstOrg);
        CP_WRITE(pjBase, DWG_CXLEFT, pClip->left);
        CP_WRITE(pjBase, DWG_CXRIGHT, pClip->right - 1);
        CP_WRITE(pjBase, DWG_CYBOT,
                     ((pClip->bottom + pRc->hwBufferOffset - 1) * ppdev->cxMemory) +
                      ppdev->ulYDstOrg);

        pClip++;
    }

    // Okay, finally draw the line...

    startCoord = ((ix1 + pEsc->pwo->rclClient.left) & 0xffff) |
                 ((iy1 + pEsc->pwo->rclClient.top + pRc->hwBufferOffset) << 16);
    endCoord = ((ix2 + pEsc->pwo->rclClient.left) & 0xffff) |
               ((iy2 + pEsc->pwo->rclClient.top + pRc->hwBufferOffset) << 16);

    CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 2);
    CP_WRITE(pjBase, DWG_XYSTRT, startCoord);
    CP_START(pjBase, DWG_XYEND, endCoord);

    while (--clipNum > 0) {

        if (pRc->shadeMode == RX_SMOOTH) {
            CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 3);


            CP_WRITE(pjBase, DWG_DR4,  (ULONG)(c1->r) >> 1);
            CP_WRITE(pjBase, DWG_DR8,  (ULONG)(c1->g) >> 1);
            CP_WRITE(pjBase, DWG_DR12, (ULONG)(c1->b) >> 1);
        }

        if (pRc->zEnable) {
            CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 1);

            CP_WRITE(pjBase, DWG_DR0, zStart);
        }

        CHECK_FIFO_FREE(pjBase, pRc->hwFifo, 8);

        CP_WRITE(pjBase, DWG_CYTOP,
                      ((pClip->top + pRc->hwBufferOffset) * ppdev->cxMemory) +
                      ppdev->ulYDstOrg);
        CP_WRITE(pjBase, DWG_CXLEFT, pClip->left);
        CP_WRITE(pjBase, DWG_CXRIGHT, pClip->right - 1);
        CP_WRITE(pjBase, DWG_CYBOT,
                     ((pClip->bottom + pRc->hwBufferOffset - 1) * ppdev->cxMemory) +
                      ppdev->ulYDstOrg);

        CP_WRITE(pjBase, DWG_XYSTRT, startCoord);
        CP_START(pjBase, DWG_XYEND, endCoord);

        pClip++;
    }
}


void DrawLineStrip(RXESCAPE *pEsc, RXDRAWPRIM *prxDrawPrim)
{
    RXRC *pRc = pEsc->pRc;
    char *pCurVertex;
    char *maxPtr;
    LONG cLines;
    char *pVertexData[2];
    RXPOINTZTEX *pVertex[2];
    RXCOLOR *pColor[2];
    ULONG shadeMode = pRc->shadeMode;

    // Point to start of vertices (or vertex pointers):

    if ((!pEsc->prxMemVertexData) ||
        (prxDrawPrim->hrxSharedMemVertexData !=
         pEsc->prxMemVertexData->hrxSharedMem)) {
        pEsc->prxMemVertexData =
            GetPtrFromHandle(prxDrawPrim->hrxSharedMemVertexData,
                             RXHANDLE_SHAREMEM);

        if (!pEsc->prxMemVertexData)
            ERROR_RETURN;
    }

    SetupLine(pEsc, pRc);

    pCurVertex = (char *)prxDrawPrim->pSharedMem +
                 pEsc->prxMemVertexData->addrOffset;

    // Make sure entire command (including vertices) fits into the
    // command buffer.

    maxPtr = (char *)pCurVertex +
             (prxDrawPrim->numVertices * pRc->stripVertexStride);

    CHECK_SHARED_MEM_RANGE(pCurVertex, pEsc->prxMemVertexData);
    CHECK_SHARED_MEM_RANGE(maxPtr, pEsc->prxMemVertexData);


    if (!pEsc->pWnd->prxClipScissored->c)
        return;

    if (shadeMode == RX_SOLID)
        pColor[0] = (RXCOLOR *)(&pRc->solidColor);

    for (cLines = prxDrawPrim->numVertices - 1; cLines > 0; cLines--) {

        pVertexData[0] = pCurVertex;
        pVertexData[1] = pCurVertex + pRc->stripVertexStride;
        pCurVertex = pVertexData[1];

        switch (shadeMode)
        {
            case RX_SOLID:
                // color already handled...
                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                break;
            case RX_FLAT:
                pColor[0] = (RXCOLOR *)pVertexData[1];

                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                break;
            default:
                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                pColor[0] = (RXCOLOR *)pVertexData[0];
                pColor[1] = (RXCOLOR *)pVertexData[1];
                break;
        }

        DrawLine(pEsc, pVertex, pColor);
    }
}

void DrawLineList(RXESCAPE *pEsc, RXDRAWPRIM *prxDrawPrim)
{
    RXRC *pRc = pEsc->pRc;
    char *pCurVertex;
    char *maxPtr;
    LONG i, cLines;
    char *pVertexData[2];
    RXPOINTZTEX *pVertex[2];
    RXCOLOR *pColor[2];
    ULONG shadeMode = pRc->shadeMode;
    RXMEMOBJ *pMemObj;

    if ((!pEsc->prxMemVertexPtr) ||
        (prxDrawPrim->hrxSharedMemVertexPtr !=
         pEsc->prxMemVertexPtr->hrxSharedMem)) {
        pEsc->prxMemVertexPtr =
            GetPtrFromHandle(prxDrawPrim->hrxSharedMemVertexPtr,
                             RXHANDLE_SHAREMEM);

        if (!pEsc->prxMemVertexPtr)
            ERROR_RETURN;
    }

    SetupLine(pEsc, pRc);

    pCurVertex = (char *)prxDrawPrim->pSharedMem +
                 pEsc->prxMemVertexPtr->addrOffset;

    // Check that all of the vertex pointers fit into the buffer:

    cLines = prxDrawPrim->numVertices >> 1;

    maxPtr = pCurVertex +
             (cLines * pRc->lineListPointerStride);

    CHECK_SHARED_MEM_RANGE(pCurVertex, pEsc->prxMemVertexPtr);
    CHECK_SHARED_MEM_RANGE(maxPtr, pEsc->prxMemVertexPtr);


    if (!pEsc->pWnd->prxClipScissored->c)
        return;


    // Now, get the data for the memory to be used with the
    // list of vertex pointer:

    if (prxDrawPrim->hrxSharedMemVertexPtr ==
        prxDrawPrim->hrxSharedMemVertexData) {
         pMemObj = pEsc->prxMemVertexPtr;
    } else if ((pEsc->prxMemVertexData) &&
        (prxDrawPrim->hrxSharedMemVertexData ==
         pEsc->prxMemVertexData->hrxSharedMem)) {
         pMemObj = pEsc->prxMemVertexData;
    } else {
        pEsc->prxMemVertexData =
            GetPtrFromHandle(prxDrawPrim->hrxSharedMemVertexData,
                             RXHANDLE_SHAREMEM);

        if (!pEsc->prxMemVertexData)
            ERROR_RETURN;

        pMemObj = pEsc->prxMemVertexData;
    }


    if (shadeMode == RX_SOLID)
        pColor[0] = (RXCOLOR *)(&pRc->solidColor);

    for (i = 0; i < cLines; i++) {
        int j;

        // Get pointer to next vertex in list

        pVertexData[0] = (char *)(*((ULONG *)pCurVertex) +
                                  pMemObj->addrOffset);

        // Make sure that the entire vertex is in the bounds of the
        // shared-memory buffer:

        if ((pVertexData[0] < pMemObj->pMem) ||
            ((pVertexData[0] + pRc->vertexStride) > pMemObj->pMemMax))
        {
            ERROR_RETURN;
        }

	// Now, do the same for the second vertex:

        pVertexData[1] = (char *)(*((ULONG *)(pCurVertex + sizeof(char *))) +
                                  pMemObj->addrOffset);

        if ((pVertexData[1] < pMemObj->pMem) ||
            ((pVertexData[1] + pRc->vertexStride) > pMemObj->pMemMax))
        {
            ERROR_RETURN;
        }

        pCurVertex += pRc->lineListPointerStride;

        switch (shadeMode)
        {
            case RX_SOLID:
                // color already handled...
                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                break;
            case RX_FLAT:
                pColor[0] = (RXCOLOR *)pVertexData[1];

                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                break;
            default:
                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                pColor[0] = (RXCOLOR *)pVertexData[0];
                pColor[1] = (RXCOLOR *)pVertexData[1];
                break;
        }

        DrawLine(pEsc, pVertex, pColor);
    }
}


void DrawIntLineStrip(RXESCAPE *pEsc, RXDRAWPRIM *prxDrawPrim)
{
    RXRC *pRc = pEsc->pRc;
    LONG xOffset = pEsc->pwo->rclClient.left;
    LONG yOffset = pEsc->pwo->rclClient.top + pRc->hwBufferOffset;
    BOOL bFastPath = (pEsc->pWnd->prxClipScissored->c <= 1) &&
                     (pRc->linePattern.linePattern == 0xffff);
    PDEV *ppdev = (PDEV *)pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    LONG x1, y1, x2, y2;
    RXDRAWPRIM rxDrawPrim;
    char *pCurVertex;
    CHAR *maxPtr;
    LONG cLines;
    RXHANDLE rxSharedMem;
    ULONG sharedMemOffset;

    if (!pEsc->pWnd->prxClipScissored->c)
        return;

    SetupIntLine(pEsc, pRc);

    // Point to start of vertices (or vertex pointers):

    rxSharedMem = prxDrawPrim->hrxSharedMemVertexData;

    if ((!pEsc->prxMemVertexData) ||
        (rxSharedMem != pEsc->prxMemVertexData->hrxSharedMem)) {
        pEsc->prxMemVertexData =
            GetPtrFromHandle(rxSharedMem, RXHANDLE_SHAREMEM);

        if (!pEsc->prxMemVertexData)
            ERROR_RETURN;
    }

    sharedMemOffset = pEsc->prxMemVertexData->addrOffset;

    pCurVertex = (char *)prxDrawPrim->pSharedMem + sharedMemOffset;

    // Make sure entire command (including vertices) fits into the
    // command buffer.


    maxPtr = (char *)pCurVertex +
             (prxDrawPrim->numVertices * sizeof(RXPOINTINT));

    CHECK_SHARED_MEM_RANGE(pCurVertex, pEsc->prxMemVertexData);
    CHECK_SHARED_MEM_RANGE(maxPtr, pEsc->prxMemVertexData);

    // Keep looping until we run out of integer lines:

    for (;;) {
        if (bFastPath) {
            CHAR cFifo = 0;
            LONG startCoord;
            LONG lastCoord;

            CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

            startCoord = ((((RXPOINTINT *)pCurVertex)->x + xOffset) & 0xffff) |
                          ((((RXPOINTINT *)pCurVertex)->y + yOffset) << 16);

            if (startCoord >= 0)
                CP_WRITE(pjBase, DWG_XYSTRT, startCoord);

            for (maxPtr = maxPtr - sizeof(RXPOINTINT); pCurVertex < maxPtr; ) {

                pCurVertex += sizeof(RXPOINTINT);

                lastCoord = ((((RXPOINTINT *)pCurVertex)->x + xOffset) & 0xffff) |
                            ((((RXPOINTINT *)pCurVertex)->y + yOffset) << 16);

                if ((startCoord & lastCoord) < 0) {
                    startCoord = lastCoord;
                    continue;
                }

                if (startCoord < 0) {
                    CHECK_FIFO_FREE(pjBase, cFifo, 3);

                    CP_WRITE(pjBase, DWG_XYSTRT, lastCoord);
                    CP_START(pjBase, DWG_XYEND, startCoord);
                    CP_WRITE(pjBase, DWG_XYSTRT, lastCoord);
                } else {
                    CHECK_FIFO_FREE(pjBase, cFifo, 1);

                    CP_START(pjBase, DWG_XYEND, lastCoord);
                }

                startCoord = lastCoord;
            }
        } else {

            ULONG maxClip = pEsc->pWnd->prxClipScissored->c;
            RECTL *pClip;
            ULONG i;

            for (i = 0, pClip = &pEsc->pWnd->prxClipScissored->arcl[0];
                 i < maxClip; i++, pClip++) {

                SetLineClippingAndPattern(pEsc, pClip);

                for (maxPtr = maxPtr - sizeof(RXPOINTINT); 
                     pCurVertex < maxPtr; ) {

                    x1 = ((RXPOINTINT *)pCurVertex)->x + xOffset;
                    y1 = ((RXPOINTINT *)pCurVertex)->y + yOffset;
                    pCurVertex += sizeof(RXPOINTINT);
                    x2 = ((RXPOINTINT *)pCurVertex)->x + xOffset;
                    y2 = ((RXPOINTINT *)pCurVertex)->y + yOffset;

                    DrawFastLine(pEsc, pRc, x1, y1, x2, y2);
                }
            }
        }

        if ((BYTE*)pEsc->prxCmd > (BYTE*)pEsc->pBufEnd - sizeof(RXDRAWPRIM))
            return;

        // See if the next command is another integer line in the
        // same memory window:

        prxDrawPrim = (RXDRAWPRIM *)(pEsc->prxCmd);

        if ((prxDrawPrim->command != RXCMD_DRAW_PRIM) ||
            (prxDrawPrim->primType != RX_PRIM_INTLINESTRIP) ||
            (prxDrawPrim->hrxSharedMemVertexData != rxSharedMem))
            return;

        memcpy(&rxDrawPrim, pEsc->prxCmd, sizeof(RXDRAWPRIM));
        prxDrawPrim = (RXDRAWPRIM *)&rxDrawPrim;

        pCurVertex = (char *)prxDrawPrim->pSharedMem + sharedMemOffset;

        maxPtr = (char *)pCurVertex +
                 (prxDrawPrim->numVertices * sizeof(RXPOINTINT));

        CHECK_SHARED_MEM_RANGE(pCurVertex, pEsc->prxMemVertexData);
        CHECK_SHARED_MEM_RANGE(maxPtr, pEsc->prxMemVertexData);

        pEsc->prxCmd = (RXCMD *)((char *)pEsc->prxCmd + sizeof(RXDRAWPRIM));
    }
}

