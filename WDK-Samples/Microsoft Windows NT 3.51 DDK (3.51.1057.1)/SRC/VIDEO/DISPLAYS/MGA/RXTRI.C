/******************************Module*Header*******************************\
* Module Name: rxtri.c
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#include "rxddi.h"
#include "rxhw.h"


typedef struct _TRIPARAMS {
    RXREAL drdx, drdy;	    // delta values
    RXREAL dgdx, dgdy;
    RXREAL dbdx, dbdy;
    RXREAL dzdx, dzdy;
    RXREAL ixLeftTop;       // pixel-centered x-left coords
    RXREAL ixRightTop;      // pixel-centered x-right coords
    ULONG xOffset, yOffset; // display offset
    PDEV *ppdev;
    CHAR cFifo;
} TRIPARAMS;

#ifdef _X86_

#pragma warning(disable:4035)

// (a * b) >> 16

__inline LONG FIXMUL(LONG a, LONG b)
{
    _asm {
        mov     eax, a
        imul    b
        shrd    eax, edx, 16
    }
}

// (a * b) / divisor

__inline LONG MULDIV(LONG a, LONG b, LONG divisor)
{
    _asm {
        mov     eax, a
        imul    dword ptr b
        idiv    divisor
    }
}


// (a1 * a2) - (b1 * b2)

__inline LONGLONG MULMULSUB(LONG a1, LONG a2, LONG b1, LONG b2)
{
    _asm {
        mov     eax, b1
        imul    dword ptr b2
        mov     ebx, eax
        mov     ecx, edx            // b1 * b2 in ecx:ebx
        mov     eax, a1
        imul    dword ptr a2
        sub     eax, ebx
        sbb     edx, ecx            // getidyac (a1 * a2) - (b1 * b2)
    }
}

// ((a1 * a2) - (b1 * b2)) / divNum

__inline LONG MULMULSUBDIV(LONG a1, LONG a2, LONG b1, LONG b2, LONGLONG divnum,
                           BOOL bigDivNum)
{
    LONGLONG tmp;

    _asm {
        mov     eax, b1
        imul    dword ptr b2
        mov     ebx, eax
        mov     ecx, edx            // b1 * b2 in ecx:ebx
        mov     eax, a1
        imul    dword ptr a2
        sub     eax, ebx
        sbb     edx, ecx            // get (a1 * a2) - (b1 * b2)
    }

    _asm {
        cmp     bigDivNum, 0
        je      smallDiv
        mov     dword ptr tmp, eax
        mov     dword ptr tmp + 4, edx
    }

    return (LONG)(tmp / divnum);

error_ret:
    return 0;

smallDiv:
    _asm {
        mov     ebx, edx
        cmp     edx, 0
        jge     pos_num
        neg     ebx
pos_num:
        mov     ecx, dword ptr divnum
        cmp     ecx, 0
        jge     pos_denom
        neg     ecx
pos_denom:
	shr	ecx, 1
        sub     ebx, ecx
        jge     error_ret
        idiv    dword ptr divnum
    }
}

#pragma warning(default:4035)

#else

#define FIXMUL(a, b)\
    (LONG)(((LONGLONG)(a) * (LONGLONG)(b)) >> 16)

#define MULDIV(a, b, div)\
    (LONG)(((LONGLONG)(a) * (LONGLONG)(b)) / (LONGLONG)(div))

#define MULMULSUB(a1, a2, b1, b2)\
    (((LONGLONG)(a1) * (LONGLONG)(a2)) - ((LONGLONG)(b1) * (LONGLONG)(b2)))

#define MULMULSUBDIV(a1, a2, b1, b2, divnum, bigNum)\
    (LONG)((((LONGLONG)(a1) * (LONGLONG)(a2)) - ((LONGLONG)(b1) * (LONGLONG)(b2))) / divnum)

#endif


#define HIGH_PART(longlong)	(*(((LONG *)&longlong)+1))
#define LOW_PART(longlong)	(*((LONG *)&longlong))


void ADJUSTLEFTEDGE(RXPOINTZTEX *pV, RXCOLOR *pC, RXRC *pRc,
                    TRIPARAMS *pTri, RXREAL dxLeft, RXREAL dyLeft,
                    RXREAL xFrac, RXREAL yFrac, RXREAL xErr)
{
    PDEV *ppdev = pTri->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    ULONG rVal;
    ULONG gVal;
    ULONG bVal;

    // Adjust the color and z values for the first pixel drawn on the left
    // edge to be on the pixel center.  This is especially important to
    // perform accurate z-buffering.

    CHECK_FIFO_FREE(pjBase, pTri->cFifo, 8);

    // We will need to set up the hardware color interpolators:

   if (pRc->shadeMode == RX_SMOOTH) {

        if (pTri->drdx | pTri->drdy) {
            rVal = (ULONG)(TOFIX(pC->r) + FIXMUL(pTri->drdx, xFrac) +
                   FIXMUL(pTri->drdy, yFrac)) >> 1;
        } else {
            rVal = (ULONG)(TOFIX(pC->r) >> 1);
        }

        if (pTri->dgdx | pTri->dgdy) {
            gVal = (ULONG)(TOFIX(pC->g) + FIXMUL(pTri->dgdx, xFrac) +
                   FIXMUL(pTri->dgdy, yFrac)) >> 1;
        } else {
            gVal = (ULONG)(TOFIX(pC->g) >> 1);
        }

        if (pTri->dbdx | pTri->dbdy) {
            bVal =  (ULONG)(TOFIX(pC->b) + FIXMUL(pTri->dbdx, xFrac) +
                    FIXMUL(pTri->dbdy, yFrac)) >> 1;
        } else {
            bVal = (ULONG)(TOFIX(pC->b) >> 1);
        }

    } else {

        // In this case, we're shading the color, but we still need
        // to set up the color registers:

        rVal = (ULONG)(TOFIX(pC->r) >> 1);
        gVal = (ULONG)(TOFIX(pC->g) >> 1);
        bVal = (ULONG)(TOFIX(pC->b) >> 1);
    }

    // Chop off the factional bits used to perform dithering if dithering
    // is disabled.  This will only give the correct result for zero or
    // near-zero delta values, but it is the best we can do on the MGA...

    if (!pRc->ditherEnable) {
        rVal = ((rVal >> pRc->rShift) & (~0x7fff)) << pRc->rShift;
        gVal = ((gVal >> pRc->gShift) & (~0x7fff)) << pRc->gShift;
        bVal = ((bVal >> pRc->bShift) & (~0x7fff)) << pRc->bShift;
    }

    CP_WRITE(pjBase, DWG_DR4, rVal);
    CP_WRITE(pjBase, DWG_DR8, gVal);
    CP_WRITE(pjBase, DWG_DR12, bVal);

    // We always sub-pixel correct the z-buffer:

    if (pRc->zEnable) {
         CP_WRITE(pjBase, DWG_DR0,
                  (ULONG)(pV->z + FIXMUL(pTri->dzdx, xFrac) +
                  FIXMUL(pTri->dzdy, yFrac)) >> 1);
    }

    // We've handled the color/z setup.  Now, take care of the actual
    // DDA.

    if (dxLeft >= 0) {

        ULONG size = (dxLeft | dyLeft) >> 16;

	if (size <= 0xff) {
            dxLeft >>= (8 + 1);
            dyLeft >>= (8 + 1);
        } else if (size <= 0xfff) {
            dxLeft >>= (12 + 1);
            dyLeft >>= (12 + 1);
        } else {
            dxLeft >>= (16 + 1);
            dyLeft >>= (16 + 1);
        }

        CP_WRITE(pjBase, DWG_AR1, -dxLeft + FIXMUL(xErr, dyLeft));
        CP_WRITE(pjBase, DWG_AR2, -dxLeft);
    } else {

        ULONG size = (-dxLeft | dyLeft) >> 16;

	if (size <= 0xff) {
            dxLeft >>= (8 + 1);
            dyLeft >>= (8 + 1);
        } else if (size <= 0xfff) {
            dxLeft >>= (12 + 1);
            dyLeft >>= (12 + 1);
        } else {
            dxLeft >>= (16 + 1);
            dyLeft >>= (16 + 1);
        }

        CP_WRITE(pjBase, DWG_AR1,
                 dxLeft + dyLeft - 1 - FIXMUL(xErr, dyLeft));
        CP_WRITE(pjBase, DWG_AR2, dxLeft);
    }

    if (!dyLeft)
        dyLeft++;

    CP_WRITE(pjBase, DWG_AR0, dyLeft);
    CP_WRITE(pjBase, DWG_FXLEFT, pTri->ixLeftTop + pTri->xOffset);
}


void ADJUSTRIGHTEDGE(RXPOINTZTEX *pV, RXCOLOR *pC, TRIPARAMS *pTri,
                     RXREAL dxRight, RXREAL dyRight, RXREAL xErr)
{
    PDEV *ppdev = pTri->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_FREE(pjBase, pTri->cFifo, 4);

    if (dxRight >= 0) {

        ULONG size = (dxRight | dyRight) >> 16;

	if (size <= 0xff) {
            dxRight >>= (8 + 1);
            dyRight >>= (8 + 1);
        } else if (size <= 0xfff) {
            dxRight >>= (12 + 1);
            dyRight >>= (12 + 1);
        } else {
            dxRight >>= (16 + 1);
            dyRight >>= (16 + 1);
        }

        CP_WRITE(pjBase, DWG_AR4, -dxRight + FIXMUL(xErr, dyRight));
        CP_WRITE(pjBase, DWG_AR5, -dxRight);
    } else {

        ULONG size = (-dxRight | dyRight) >> 16;

	if (size <= 0xff) {
            dxRight >>= (8 + 1);
            dyRight >>= (8 + 1);
        } else if (size <= 0xfff) {
            dxRight >>= (12 + 1);
            dyRight >>= (12 + 1);
        } else {
            dxRight >>= (16 + 1);
            dyRight >>= (16 + 1);
        }

        CP_WRITE(pjBase, DWG_AR4,
                 dxRight + dyRight - 1 - FIXMUL(xErr, dyRight));
        CP_WRITE(pjBase, DWG_AR5, dxRight);
    }

    if (!dyRight)
        dyRight++;

    CP_WRITE(pjBase, DWG_AR6, dyRight);
    CP_WRITE(pjBase, DWG_FXRIGHT, pTri->ixRightTop + pTri->xOffset);
}


__inline void SETUPDELTAS(RXRC *pRc, TRIPARAMS *pTri)
{
    PDEV *ppdev = pTri->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    if (pRc->hwTrapFunc & atype_ZI) {

        if (pRc->shadeMode == RX_SMOOTH) {

            CHECK_FIFO_FREE(pjBase, pTri->cFifo, 8);

            CP_WRITE(pjBase, DWG_DR2, pTri->dzdx >> 1);
            CP_WRITE(pjBase, DWG_DR3, pTri->dzdy >> 1);

            CP_WRITE(pjBase, DWG_DR6, pTri->drdx >> 1);
            CP_WRITE(pjBase, DWG_DR7, pTri->drdy >> 1);

            CP_WRITE(pjBase, DWG_DR10, pTri->dgdx >> 1);
            CP_WRITE(pjBase, DWG_DR11, pTri->dgdy >> 1);

            CP_WRITE(pjBase, DWG_DR14, pTri->dbdx >> 1);
            CP_WRITE(pjBase, DWG_DR15, pTri->dbdy >> 1);

        } else {

            CHECK_FIFO_FREE(pjBase, pTri->cFifo, 2);

            CP_WRITE(pjBase, DWG_DR2, pTri->dzdx >> 1);
            CP_WRITE(pjBase, DWG_DR3, pTri->dzdy >> 1);
        }
    }
}


__inline void DRAWTRAP(TRIPARAMS *pTri, LONG dxLeft, LONG dxRight,
                       LONG y, LONG dy)
{
    PDEV *ppdev = pTri->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    ULONG signs = 0;

    CHECK_FIFO_FREE(pjBase, pTri->cFifo, 3);

    if (dxLeft < 0)
        signs |= sdxl_SUB;

    if (dxRight < 0)
        signs |= sdxr_DEC;

    CP_WRITE(pjBase, DWG_SGN, (scanleft_RIGHT | sdy_ADD | signs));
    CP_WRITE(pjBase, DWG_LEN, dy);
    CP_START(pjBase, DWG_YDST, y);
}


void SETUPTRIANGLEDRAWING(RXESCAPE *pEsc)
{
    RXRC *pRc = pEsc->pRc;
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    if (pRc->shadeMode != RX_SMOOTH) {

        // We will be using the interpolation mode of the hardware, but we'll
        // only be interpolation Z, so set the color deltas to 0.

        CHECK_FIFO_SPACE(pjBase, 13);

        if ((pRc->vertexType > RX_POINT) && (pRc->zEnable))
        {
            CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwTrapFunc | zdrwen_DEPTH);
        }
        else
        {
            CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwTrapFunc);
        }

        CP_WRITE(pjBase, DWG_DR6, 0);
        CP_WRITE(pjBase, DWG_DR7, 0);

        CP_WRITE(pjBase, DWG_DR10, 0);
        CP_WRITE(pjBase, DWG_DR11, 0);

        CP_WRITE(pjBase, DWG_DR14, 0);
        CP_WRITE(pjBase, DWG_DR15, 0);

        if (!(ppdev->HopeFlags & PATTERN_CACHE)) {

            CP_WRITE(pjBase, DWG_SRC0, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC1, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC2, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC3, 0xFFFFFFFF);
        }

        ppdev->HopeFlags = PATTERN_CACHE;

    } else {
        CHECK_FIFO_SPACE(pjBase, 5);

        if ((pRc->vertexType > RX_POINT) && (pRc->zEnable))
        {
            CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwTrapFunc | zdrwen_DEPTH);
        }
        else
        {
            CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwTrapFunc);
        }

        if (!(ppdev->HopeFlags & PATTERN_CACHE)) {

            CP_WRITE(pjBase, DWG_SRC0, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC1, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC2, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC3, 0xFFFFFFFF);
        }

        ppdev->HopeFlags = PATTERN_CACHE;
    }

    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
    pRc->hwFifo = 32;
}

void SETUPCLIPPING(PDEV *ppdev, RECTL *pClip, TRIPARAMS *pTri, RXRC *pRc)
{
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_FREE(pjBase, pTri->cFifo, 4);

    CP_WRITE(pjBase, DWG_CYTOP,
                  ((pClip->top + pRc->hwBufferOffset) * ppdev->cxMemory) +
                  ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_CXLEFT, pClip->left);
    CP_WRITE(pjBase, DWG_CXRIGHT, pClip->right - 1);
    CP_WRITE(pjBase, DWG_CYBOT,
                 ((pClip->bottom + pRc->hwBufferOffset- 1) * ppdev->cxMemory) +
                  ppdev->ulYDstOrg);
}


#define SWAP(a, b, type)\
{\
    type tmp = (type)a;\
    (type)a = (type)b;\
    (type)b = tmp;\
}


void DrawTriangle(RXESCAPE *pEsc, RXPOINTZTEX **pVertex,
                  RXCOLOR **pColor)
{
    RXRC *pRc = pEsc->pRc;
    RXREAL dxAC, dxAB, dxBC;
    RXREAL dyAB, dyAC, dyBC;
    RXREAL aIY, bIY, cIY;
    RXPOINTZTEX *pA, *pB, *pC;
    RXCOLOR *cA, *cB, *cC;
    LONGLONG area;
    BOOL bigArea;
    RXREAL fracX, fracY;        // distance from coord to pixel center
    RXREAL errX;                // x-distance from y-adjusted line to pixel center
    LONG idyAC, idyBC, idyAB;
    RXREAL xLeftTop, xRightTop;
    ULONG clipNum;
    RECTL *pClip;
    TRIPARAMS tri;

    pA = pVertex[0];
    pB = pVertex[1];
    pC = pVertex[2];

    tri.xOffset = pEsc->pwo->rclClient.left;
    tri.yOffset = pEsc->pwo->rclClient.top + pRc->hwBufferOffset;
    tri.ppdev = pEsc->ppdev;
    tri.cFifo = pRc->hwFifo;

    switch (pRc->shadeMode) {
        case RX_SOLID:
        case RX_FLAT:
            cA = cB = pColor[0];
            break;
        case RX_SMOOTH:
            cA = pColor[0];
            cB = pColor[1];
            cC = pColor[2];
            break;
        default:
            ERROR_RETURN;
    }

    if (pA->y > pB->y) {
        SWAP(pA, pB, void *);
        SWAP(cA, cB, void *);
    }

    if (pA->y > pC->y) {
        SWAP(pA, pC, void *);
        SWAP(cA, cC, void *);
    }

    if (pB->y > pC->y) {
        SWAP(pB, pC, void *);
        SWAP(cB, cC, void *);
    }

    // Get integer y coordinates adjusted for pixel-centers:

    aIY = (pA->y + FIX_HALF) >> 16;
    bIY = (pB->y + FIX_HALF) >> 16;
    cIY = (pC->y + FIX_HALF) >> 16;

    idyAB = (bIY - aIY);
    idyBC = (cIY - bIY);
    idyAC = (cIY - aIY);

    if (idyAC <= 0)
        return;

//
//
// The fun begins.  We have y-sorted fixed-point coordinates ready to go at
// this point.  First, get dx/dy values for the triangle:
//
//

    dyAB = pB->y - pA->y;
    dyBC = pC->y - pB->y;
    dyAC = pC->y - pA->y;
    dxAB = pB->x - pA->x;
    dxBC = pC->x - pB->x;
    dxAC = pC->x - pA->x;

// Calculate the half-area.  Since this is really just a cross-product, it
// is used not only as the denominator for calculation of the interpolants,
// but also to do culling and to tell us the orientation of the triangle:

    area = MULMULSUB(dxAC, dyBC, dxBC, dyAC);

    // flag for double-precision fixed-point

    bigArea = ((HIGH_PART(area) < -32768) ||
               (HIGH_PART(area) > 32767));

    area >>= 16;

    // Clamp area at 1/2 pixel:

    if ((area < 0x10000) && (area > -0x10000)) {
        if (area < 0)
            area = -0x10000;
        else
            area = 0x10000;
    }

// Now, calculate the delta values of the required interpolants in
// both x and y:

    if (pRc->zEnable) {
        RXREAL dzAC, dzBC;

	dzAC = pC->z - pA->z;
	dzBC = pC->z - pB->z;

        tri.dzdx = MULMULSUBDIV(dzAC, dyBC,  dzBC, dyAC, area, bigArea);
        tri.dzdy = MULMULSUBDIV(dzBC, dxAC,  dzAC, dxBC, area, bigArea);
    }

    if (pRc->shadeMode == RX_SMOOTH) {

        RXREAL drAC, dgAC, dbAC, daAC;
        RXREAL drBC, dgBC, dbBC, daBC;

        drAC = TOFIX(cC->r) - TOFIX(cA->r);
        drBC = TOFIX(cC->r) - TOFIX(cB->r);

        if (drAC | drBC) {
            tri.drdx = MULMULSUBDIV(drAC, dyBC,  drBC, dyAC, area, bigArea);
            tri.drdy = MULMULSUBDIV(drBC, dxAC,  drAC, dxBC, area, bigArea);
        } else {
            tri.drdx = tri.drdy = 0;
        }

        dgAC = TOFIX(cC->g) - TOFIX(cA->g);
        dgBC = TOFIX(cC->g) - TOFIX(cB->g);

        if (dgAC | dgBC) {
            tri.dgdx = MULMULSUBDIV(dgAC, dyBC,  dgBC, dyAC, area, bigArea);
            tri.dgdy = MULMULSUBDIV(dgBC, dxAC,  dgAC, dxBC, area, bigArea);
        } else {
            tri.dgdx = tri.dgdy = 0;
        }

        dbAC = TOFIX(cC->b) - TOFIX(cA->b);
        dbBC = TOFIX(cC->b) - TOFIX(cB->b);

        if (dbAC | dbBC) {
            tri.dbdx = MULMULSUBDIV(dbAC, dyBC,  dbBC, dyAC, area, bigArea);
            tri.dbdy = MULMULSUBDIV(dbBC, dxAC,  dbAC, dxBC, area, bigArea);
        } else {
            tri.dbdx = tri.dbdy = 0;
        }
    }

// Set up the delta values for the entire triangle:

    SETUPDELTAS(pRc, &tri);

//
//
// We are ready to rasterize.
//
//

    if ((clipNum = pEsc->pWnd->prxClipScissored->c) > 1) {
        pClip = &pEsc->pWnd->prxClipScissored->arcl[0];

        SETUPCLIPPING(tri.ppdev, pClip++, &tri, pRc);
    }

drawAgain:

    if (area > 0) {

        // The bend in the triangle is on the right:

        fracY = ((aIY << 16) + FIX_HALF) - pA->y;
        xLeftTop = pA->x + MULDIV(fracY, dxAC, dyAC);
        tri.ixLeftTop = (xLeftTop + FIX_HALF) >> 16;
        errX = ((tri.ixLeftTop << 16) + FIX_HALF) - xLeftTop;
        fracX = ((tri.ixLeftTop << 16) + FIX_HALF) - pA->x;

        ADJUSTLEFTEDGE(pA, cA, pRc, &tri, dxAC, dyAC, fracX, fracY, errX);

        if (idyAB > 0) {

            xRightTop = pA->x + MULDIV(fracY, dxAB, dyAB);
            tri.ixRightTop = (xRightTop + FIX_HALF) >> 16;
            errX = ((tri.ixRightTop << 16) + FIX_HALF) - xRightTop;

	    ADJUSTRIGHTEDGE(pA, cA, &tri, dxAB, dyAB, errX);

            DRAWTRAP(&tri, dxAC, dxAB, aIY + tri.yOffset, idyAB);
	}

        if (idyBC > 0) {

            fracY = ((bIY << 16) + FIX_HALF) - pB->y;
            xRightTop = pB->x + MULDIV(fracY, dxBC, dyBC);
            tri.ixRightTop = (xRightTop + FIX_HALF) >> 16;
            errX = ((tri.ixRightTop << 16) + FIX_HALF) - xRightTop;

	    ADJUSTRIGHTEDGE(pB, cB, &tri, dxBC, dyBC, errX);

            DRAWTRAP(&tri, dxAC, dxBC, bIY + tri.yOffset, idyBC);
	}
    } else {

        // The bend in the triangle is on the left:

        fracY = ((aIY << 16) + FIX_HALF) - pA->y;
        xRightTop = pA->x + MULDIV(fracY, dxAC, dyAC);
        tri.ixRightTop = (xRightTop + FIX_HALF) >> 16;
        errX = ((tri.ixRightTop << 16) + FIX_HALF) - xRightTop;

	ADJUSTRIGHTEDGE(pA, cA, &tri, dxAC, dyAC, errX);

	if (idyAB > 0) {

            xLeftTop = pA->x + MULDIV(fracY, dxAB, dyAB);
            tri.ixLeftTop = (xLeftTop + FIX_HALF) >> 16;
            errX = ((tri.ixLeftTop << 16) + FIX_HALF) - xLeftTop;
            fracX = ((tri.ixLeftTop << 16) + FIX_HALF) - pA->x;

	    ADJUSTLEFTEDGE(pA, cA, pRc, &tri, dxAB, dyAB, fracX, fracY, errX);

            DRAWTRAP(&tri, dxAB, dxAC, aIY + tri.yOffset, idyAB);
	}

	if (idyBC > 0) {
            fracY = ((bIY << 16) + FIX_HALF) - pB->y;
            xLeftTop = pB->x + MULDIV(fracY, dxBC, dyBC);
            tri.ixLeftTop = (xLeftTop + FIX_HALF) >> 16;
            errX = ((tri.ixLeftTop << 16) + FIX_HALF) - xLeftTop;
            fracX = ((tri.ixLeftTop << 16) + FIX_HALF) - pB->x;

	    ADJUSTLEFTEDGE(pB, cB, pRc, &tri, dxBC, dyBC, fracX, fracY, errX);

            DRAWTRAP(&tri, dxBC, dxAC, bIY + tri.yOffset, idyBC);
	}
    }

    // We could cache all of the values above to avoid redoing any calculations,
    // but for the time being let's brute-force multiple clipping rectangles
    // assuming that they are the exception...

    if (clipNum > 1) {
        SETUPCLIPPING(tri.ppdev, pClip++, &tri, pRc);
        clipNum--;
        goto drawAgain;
    }

    // Save fifo-free cache across triangle calls...

    pRc->hwFifo = tri.cFifo;
}


void DrawTriangleStrip(RXESCAPE *pEsc, RXDRAWPRIM *prxDrawPrim)
{
    RXRC *pRc = pEsc->pRc;
    char *pCurVertex;
    char *maxPtr;
    LONG i, cTriangles;
    char *pVertexData[3];
    RXPOINTZTEX *pVertex[3];
    RXCOLOR *pColor[3];
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

        pCurVertex = pEsc->prxMemVertexData->pMem;
    }

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

    cTriangles = prxDrawPrim->numVertices - 2;

    SETUPTRIANGLEDRAWING(pEsc);

    for (i = 0; i < cTriangles; i++) {

        pVertexData[0] = pCurVertex;
        pVertexData[1] = pCurVertex + pRc->stripVertexStride;
        pVertexData[2] = pVertexData[1] + pRc->stripVertexStride;
        pCurVertex = pVertexData[1];

        switch (shadeMode)
        {
            case RX_SOLID:
                // color already handled...
                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                pVertex[2] = (RXPOINTZTEX *)(pVertexData[2] +
                                             pRc->vertexColorStride);
                break;
            case RX_FLAT:
                pColor[0] = (RXCOLOR *)pVertexData[2];

                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                pVertex[2] = (RXPOINTZTEX *)(pVertexData[2] +
                                             pRc->vertexColorStride);
                break;
            default:
                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                pVertex[2] = (RXPOINTZTEX *)(pVertexData[2] +
                                             pRc->vertexColorStride);
                pColor[0] = (RXCOLOR *)pVertexData[0];
                pColor[1] = (RXCOLOR *)pVertexData[1];
                pColor[2] = (RXCOLOR *)pVertexData[2];
                break;
        }

        DrawTriangle(pEsc, pVertex, pColor);
    }
}


void DrawTriangleList(RXESCAPE *pEsc, RXDRAWPRIM *prxDrawPrim)
{
    RXRC *pRc = pEsc->pRc;
    char *pCurVertex;
    char *maxPtr;
    LONG i, cTriangles;
    char *pVertexData[3];
    RXPOINTZTEX *pVertex[3];
    RXCOLOR *pColor[3];
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

    pCurVertex = (char *)prxDrawPrim->pSharedMem +
                 pEsc->prxMemVertexPtr->addrOffset;

    cTriangles = prxDrawPrim->numVertices / 3;

    // Check that all of the vertex pointers fit into the buffer:

    maxPtr = pCurVertex +
             (cTriangles * pRc->triListPointerStride);

    CHECK_SHARED_MEM_RANGE(pCurVertex, pEsc->prxMemVertexPtr);
    CHECK_SHARED_MEM_RANGE(maxPtr, pEsc->prxMemVertexPtr);


    if (!pEsc->pWnd->prxClipScissored->c)
        return;

    // Now, get the data for the memory to be used with the
    // list of vertex pointer.  First, check to see if the list
    // pointers and vertices reside in the same shared memory, then
    // go for caching:

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

    SETUPTRIANGLEDRAWING(pEsc);

    for (i = 0; i < cTriangles; i++) {
        int j;

        for (j = 0; j < 3; j++, pCurVertex += sizeof(char *))
        {
            char *newVertex;

            // Get pointer to next vertex in list:

            newVertex = (char *)(*((ULONG *)pCurVertex) + pMemObj->addrOffset);

            // Make sure that the entire vertex is in the bounds of the
            // shared-memory buffer:

            if ((newVertex < pMemObj->pMem) ||
                ((newVertex + pRc->vertexStride) > pMemObj->pMemMax))
            {
                ERROR_RETURN;
            }

            pVertexData[j] = newVertex;
        }

        pCurVertex += pRc->listPointerSkip;

        switch (shadeMode)
        {
            case RX_SOLID:
                // color already handled...
                pVertex[0] = (RXPOINTZTEX *)pVertexData[0];
                pVertex[1] = (RXPOINTZTEX *)pVertexData[1];
                pVertex[2] = (RXPOINTZTEX *)pVertexData[2];
                break;
            case RX_FLAT:
                pColor[0] = (RXCOLOR *)pVertexData[2];

                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                pVertex[2] = (RXPOINTZTEX *)(pVertexData[2] +
                                             pRc->vertexColorStride);
                break;
            default:
                pVertex[0] = (RXPOINTZTEX *)(pVertexData[0] +
                                             pRc->vertexColorStride);
                pVertex[1] = (RXPOINTZTEX *)(pVertexData[1] +
                                             pRc->vertexColorStride);
                pVertex[2] = (RXPOINTZTEX *)(pVertexData[2] +
                                             pRc->vertexColorStride);
                pColor[0] = (RXCOLOR *)pVertexData[0];
                pColor[1] = (RXCOLOR *)pVertexData[1];
                pColor[2] = (RXCOLOR *)pVertexData[2];
                break;
        }

        DrawTriangle(pEsc, pVertex, pColor);
    }
}

