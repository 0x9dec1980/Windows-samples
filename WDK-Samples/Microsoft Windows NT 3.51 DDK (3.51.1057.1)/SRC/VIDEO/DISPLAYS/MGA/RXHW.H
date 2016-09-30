/******************************Module*Header*******************************\
* Module Name: rxhw.h
*
* Include file which indirects all of the hardware-dependent functionality
* in rxddi.c.  Other hardware-dependent functions called through the HW_xxx
* macros are also contained in rxhw.c.
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#ifndef _RXHW_H
#define _RXHW_H

#define FIX_HALF 0x00008000

#define HWCAPS_INTLINES     1   // We do integer lines only on some MGA cards
#define HWCAPS_ALL          2   // We can do all our 3D DDI support on others

VOID HwEnableBuffers(RXESCAPE*, RXRC*, RXENABLEBUFFERS*);
VOID HwCreateRc(RXESCAPE*, RXRC*);
VOID HwDestroyRc(RXRC*);
VOID HwAlignedCopy(PDEV*, BYTE*, LONG, ULONG, LONG, LONG, LONG, BOOL);

void DrawIntLineStrip(RXESCAPE *, RXDRAWPRIM *);
void DrawLineStrip(RXESCAPE *, RXDRAWPRIM *);
void DrawTriangleStrip(RXESCAPE *, RXDRAWPRIM *);
void DrawPointList(RXESCAPE *, RXDRAWPRIM *);
void DrawLineList(RXESCAPE *, RXDRAWPRIM *);
void DrawTriangleList(RXESCAPE *, RXDRAWPRIM *);

void RxPolyDrawSpan(RXESCAPE *);

__inline BOOL HW_INIT_RXESCAPE(RXESCAPE *prxEscape)
{
    PDEV *ppdev = (PDEV *)prxEscape->pso->dhpdev;

    prxEscape->ppdev = ppdev;

    // We don't support the 3D DDI in some configurations:

    return(ppdev->flHwCaps != 0);
}

__inline void HW_LINE_PATTERN(RXESCAPE *pEsc, RXRC *pRc, RXLINEPAT *prxLinePat)
{
    PDEV *pdev = (PDEV *)pEsc->ppdev;
    ULONG patStart;
    ULONG repFactor = prxLinePat->repFactor;
    ULONG repCount = repFactor;
    ULONG length = repFactor * 16;
    ULONG bitPos = 0x8000;
    ULONG dwLength;

    if (prxLinePat->linePattern == 0xffff) {
        return;
    }

    if (!repFactor || (length > 16)) {
        SET_ERROR_CODE;
        return;
    }

    pRc->linePattern = *prxLinePat;
}

__inline void HW_RX_ENABLE(void *pInfo, BOOL *pbStatus)
{
    rxFuncs[RXCMD_POLY_DRAW_SPAN] = RxPolyDrawSpan;
    *pbStatus = TRUE;
}

__inline void HW_SOLID_COLOR(RXESCAPE *pEsc, RXRC *pRc, RXCOLOR *prxColor)
{
    PDEV *ppdev = pEsc->ppdev;
    ULONG color;

    pRc->solidColor = *prxColor;

    if (pRc->flags & RX_COLOR_INDEXED)
    {
        // The colour index is specified in the 'red' component:

        color = (ULONG) prxColor->r;
    }
    else
    {
        color =
            (((ULONG)((UCHAR)(prxColor->r >> pRc->rShift)) << pRc->hwRShift) |
             ((ULONG)((UCHAR)(prxColor->g >> pRc->gShift)) << pRc->hwGShift) |
             ((ULONG)((UCHAR)(prxColor->b >> pRc->bShift)) << pRc->hwBShift));
    }

    switch (pRc->hwBpp) {
        case 8:
            pRc->hwSolidColor = (color | (color << 8) |
                                 (color << 16) | (color << 24));
            break;
        case 15:
        case 16:
            pRc->hwSolidColor = color | (color << 16);
            break;
        default:
            pRc->hwSolidColor = color;
            break;
    }
}


__inline void HW_FILL_COLOR(RXESCAPE *pEsc, RXRC *pRc, RXCOLOR *prxColor)
{
    PDEV *ppdev = pEsc->ppdev;
    ULONG color;

    pRc->fillColor = *prxColor;

    color =
        (((ULONG)((UCHAR)(prxColor->r >> pRc->rShift)) << pRc->hwRShift) |
         ((ULONG)((UCHAR)(prxColor->g >> pRc->gShift)) << pRc->hwGShift) |
         ((ULONG)((UCHAR)(prxColor->b >> pRc->bShift)) << pRc->hwBShift));

    switch (pRc->hwBpp) {
        case 8:
            pRc->hwFillColor = (color | (color << 8) |
                               (color << 16) | (color << 24));
            break;
        case 15:
        case 16:
            pRc->hwFillColor = color | (color << 16);
            break;
        default:
            pRc->hwFillColor = color;
            break;
    }
}


__inline void HW_FILL_Z(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    PDEV *ppdev = pEsc->ppdev;

    pRc->fillZ = rxState;
    pRc->hwFillZ = ((ULONG)rxState >> 1);
}


__inline void HW_PLANE_MASK(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    ULONG mask = pRc->planeMask = rxState;
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    switch (pRc->hwBpp) {
        case 8:
            mask = rxState & 0xff;
            pRc->hwPlaneMask = (mask | (mask << 8) |
                                (mask << 16) | (mask << 24));
            break;
        case 15:
            mask = rxState & 0x7fff;
            pRc->hwPlaneMask = mask | (mask << 16);
            break;
        case 16:
            mask = rxState & 0xffff;
            pRc->hwPlaneMask = mask | (mask << 16);
            break;
        default:
            pRc->hwPlaneMask = mask;
    }

    CHECK_FIFO_SPACE(pjBase, 1);
    CP_WRITE(pjBase, DWG_PLNWT, pRc->hwPlaneMask);
}

__inline void HW_Z_WRITE_ENABLE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    PDEV *ppdev = pEsc->ppdev;

    pRc->zWriteEnable = (BOOL)rxState;
}

__inline void HW_ACTIVE_BUFFER(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    PDEV *ppdev = pEsc->ppdev;

    if (!rxState || (rxState & ~(RX_FRONT_LEFT | RX_BACK_LEFT)))
    {
        SET_ERROR_CODE;
        return;
    }

    if ((rxState & RX_BACK_LEFT) &&
        (!pRc->backBufEnabled))
    {
        SET_ERROR_CODE;
        return;
    }
    else
    {
        if (rxState & RX_BACK_LEFT)
            pRc->hwBufferOffset = pRc->hwBackBufferOffset;
        else
            pRc->hwBufferOffset = 0;

        pRc->activeBuffer = rxState;
    }
}

__inline void HW_SET_ROP_FUNC(RXESCAPE *pEsc, RXRC *pRc)
{
    pRc->hwIntLineFunc = (pRc->hwIntLineFunc & ~(bop_WHITENESS)) |
                         ((pRc->rop2 - 1) << 16);
    pRc->hwLineFunc = (pRc->hwLineFunc & ~(bop_WHITENESS)) |
                      ((pRc->rop2 - 1) << 16);
    pRc->hwTrapFunc = (pRc->hwTrapFunc & ~(bop_WHITENESS)) |
                      ((pRc->rop2 - 1) << 16);
    pRc->hwSpanFunc = (pRc->hwSpanFunc & ~(bop_WHITENESS)) |
                      ((pRc->rop2 - 1) << 16);

    if (pRc->rop2 == R2_COPYPEN) {
        pRc->hwIntLineFunc = (pRc->hwIntLineFunc & ~(atype_ZI)) | atype_RPL;
        pRc->hwLineFunc = (pRc->hwLineFunc & ~(atype_ZI)) | atype_RPL;
        pRc->hwTrapFunc = (pRc->hwTrapFunc & ~(atype_ZI)) | atype_RPL;
    } else {
        pRc->hwIntLineFunc = (pRc->hwIntLineFunc & ~(atype_ZI)) | atype_RSTR;
        pRc->hwLineFunc = (pRc->hwLineFunc & ~(atype_ZI)) | atype_RSTR;
        pRc->hwTrapFunc = (pRc->hwTrapFunc & ~(atype_ZI)) | atype_RSTR;
    }

    if (pRc->lastPixel) {
        pRc->hwIntLineFunc |= 0x2;
        pRc->hwLineFunc |= 0x2;
    } else {
        pRc->hwIntLineFunc &= ~0x2;
        pRc->hwLineFunc &= ~0x2;
    }

    //XXX For now, simple assume we're always using ZI mode for lines
    //XXX and triangles:

    pRc->hwLineFunc |= atype_ZI;
    pRc->hwTrapFunc |= atype_ZI;
}


__inline void HW_INIT_STATE(RXESCAPE *pEsc, RXRC *pRc)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    pEsc->prxMemVertexData = NULL;
    pEsc->prxMemVertexPtr = NULL;
    pEsc->prxMemCache = NULL;

//!!! Ack, this function is being called after every primitive

    CHECK_FIFO_SPACE(pjBase, 6);

    if (pEsc->pWnd->prxClipScissored->c == 1)
    {
        RECTL *pClip = &pEsc->pWnd->prxClipScissored->arcl[0];

        CP_WRITE(pjBase, DWG_CYTOP,
                      ((pClip->top + pRc->hwBufferOffset) * ppdev->cxMemory) +
                      ppdev->ulYDstOrg);
        CP_WRITE(pjBase, DWG_CXLEFT, pClip->left);
        CP_WRITE(pjBase, DWG_CXRIGHT, pClip->right - 1);
        CP_WRITE(pjBase, DWG_CYBOT,
                     ((pClip->bottom + pRc->hwBufferOffset - 1) * ppdev->cxMemory) +
                      ppdev->ulYDstOrg);
    }
    else
    {
        CP_WRITE(pjBase, DWG_CYTOP,
                      ((pEsc->pwo->coClient.rclBounds.top + pRc->hwBufferOffset) * ppdev->cxMemory) +
                      ppdev->ulYDstOrg);
        CP_WRITE(pjBase, DWG_CXLEFT, pEsc->pwo->coClient.rclBounds.left);
        CP_WRITE(pjBase, DWG_CXRIGHT, pEsc->pwo->coClient.rclBounds.right - 1);
        CP_WRITE(pjBase, DWG_CYBOT,
                     ((pEsc->pwo->coClient.rclBounds.bottom + pRc->hwBufferOffset - 1) * ppdev->cxMemory) +
                      ppdev->ulYDstOrg);
    }

    CP_WRITE(pjBase, DWG_PLNWT, pRc->hwPlaneMask);
    CP_WRITE(pjBase, DWG_ZMSK, 0);
}

__inline void HW_DEFAULT_STATE(RXESCAPE *pEsc)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_SPACE(pjBase, 6);

    // restore default clipping, sign register, plane mask, pitch

//    CP_WRITE(pjBase, DWG_SGN, 0);
    CP_WRITE(pjBase, DWG_PITCH, ppdev->cxMemory | ylin_LINEARIZE);
    CP_WRITE(pjBase, DWG_PLNWT, ppdev->ulPlnWt);

    vResetClipping(ppdev);

    ppdev->HopeFlags = 0;                                // brute-force this
}

__inline void HW_ROP2(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    if ((rxState > R2_LAST) || (rxState < R2_BLACK))
    {
        RXDBG_PRINT("Invalid ROP2 function, Rop2 = %d", rxState);
        SET_ERROR_CODE;
        return;
    }

    pRc->rop2 = rxState;

    HW_SET_ROP_FUNC(pEsc, pRc);
}

__inline void HW_CULL_MODE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    if (rxState != RX_CULL_NONE)
        SET_ERROR_CODE;
}

__inline void HW_LAST_PIXEL(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    pRc->lastPixel = (BOOL)rxState;

    HW_SET_ROP_FUNC(pEsc, pRc);
}


__inline void HW_Z_FUNC(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    PDEV *ppdev = pEsc->ppdev;

    pRc->zFunc = rxState;

    switch (rxState)
    {
        case RX_CMP_LESS:
            pRc->hwLineFunc &= ~(1 << 26);
            pRc->hwTrapFunc &= ~(1 << 26);
            pRc->hwSpanFunc &= ~(1 << 26);
            break;
        case RX_CMP_LEQUAL:
            pRc->hwLineFunc |= (1 << 26);
            pRc->hwTrapFunc |= (1 << 26);
            pRc->hwSpanFunc |= (1 << 26);
            break;
        default:
            SET_ERROR_CODE;
            return;
    }
}

__inline void HW_SHADE_MODE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    if (!((rxState == RX_FLAT) ||
          (rxState == RX_SMOOTH) ||
          (rxState == RX_SOLID)))
    {
        SET_ERROR_CODE;
        return;
    }
    pRc->shadeMode = rxState;

    HW_SET_ROP_FUNC(pEsc, pRc);
}

__inline void HW_Z_ENABLE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    pRc->zEnable = (BOOL)rxState && pRc->zBufEnabled;
    HW_SET_ROP_FUNC(pEsc, pRc);
}

__inline void HW_VERTEX_TYPE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
}

__inline void HW_COLOR_TYPE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
#ifdef HANDLE_RXREALS

    pEsc->retValue = TRUE;

    switch (rxState)
    {
        default:
            SET_ERROR_CODE;
            return;
        case RX_VERTEX_COLOR_RGB:
            pRc->vertexColorStride = sizeof(RXCOLORREF);
            break;
        case RX_VERTEX_COLOR_RGBA:
            pRc->vertexColorStride = sizeof(RXCOLORREFA);
            break;
        case RX_VERTEX_COLOR_RGBAF:
            pRc->vertexColorStride = sizeof(RXCOLORREFAF);
            break;
        }
    pRc->vertexColorType = rxState;
#endif
}

__inline void HW_MASK_START(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    if (rxState != RX_MASK_MSB)
    {
	SET_ERROR_CODE;
    }
    else
	pRc->maskStart = rxState;
}

__inline void HW_BLEND_ENABLE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    if (rxState)
	SET_ERROR_CODE;
}

__inline void HW_ALPHA_TEST_ENABLE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    if (rxState)
	SET_ERROR_CODE;
}

__inline void HW_SCISSORS_ENABLE(RXESCAPE *pEsc, RXRC *pRc)
{
    if (pEsc->pWnd->prxClipScissored->c == 1)
    {
	PDEV *ppdev = pEsc->ppdev;
        RECTL *pClip = &pEsc->pWnd->prxClipScissored->arcl[0];
    }
}

__inline void HW_DITHER_ENABLE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    pRc->ditherEnable = (BOOL)rxState;
}

__inline void HW_SCISSORS_RECT(RXESCAPE *pEsc, RXRC *pRc, RXRECT *prxRect)
{
    pRc->scissorsRect = *prxRect;
}

__inline void HW_SPAN_TYPE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    if ((rxState == RX_SPAN_COLOR) ||
        (rxState == RX_SPAN_COLOR_Z) ||
        (rxState == RX_SPAN_COLOR_Z_TEX))
        pRc->spanType = rxState;
    else
        SET_ERROR_CODE;
}

__inline void HW_SPAN_DIRECTION(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
    if (rxState != RX_SPAN_HORIZONTAL)
    {
        SET_ERROR_CODE;
    }
}

#define HW_PRIMLIST_SKIP

#define HW_PRIMSTRIP_SKIP


__inline void HW_VERTEX_COLOR_TYPE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{

#ifdef HANDLE_RXREALS

    pEsc->retValue = TRUE;

    switch (rxState)
    {
        default:
            SET_ERROR_CODE;
            return;
        case RX_VERTEX_COLOR_RGB:
            pRc->vertexColorStride = sizeof(RXCOLORREF);
            break;
        case RX_VERTEX_COLOR_RGBA:
            pRc->vertexColorStride = sizeof(RXCOLORREFA);
            break;
        case RX_VERTEX_COLOR_RGBAF:
            pRc->vertexColorStride = sizeof(RXCOLORREFAF);
            break;
        }
    pRc->vertexColorType = rxState;
#endif
}


__inline void HW_SPAN_COLOR_TYPE(RXESCAPE *pEsc, RXRC *pRc, ULONG rxState)
{
}


#define HW_TEX_MAG(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_TEX_MIN(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_SRC_BLEND(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_DST_BLEND(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_ALPHA_REF(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_ALPHA_FUNC(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_QUAD_RXREAL_MODE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_TEX_MAP_BLEND(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_TEXTURE_PERSPECTIVE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_TEX_TRANSP_ENABLE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_TEX_TRANSP_COLOR(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_DITHER_ORIGIN(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_STIPPLE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

__inline void HW_START_FILL_RECT(RXESCAPE *pEsc, RXRC *pRc,
                                 RXFILLRECT *prxFillRect)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    ULONG andColor;
    ULONG orColor;
    ULONG hwOp;
    BOOL bFastColor;
    BOOL bFillC = (prxFillRect->fillType & RX_FILL_RECT_COLOR);
    BOOL bFillZ = (prxFillRect->fillType & RX_FILL_RECT_Z) &&
                  (pRc->zBufEnabled);

    andColor = (pRc->fillColor.r & pRc->fillColor.g & pRc->fillColor.b) >> 16;
    orColor = (pRc->fillColor.r | pRc->fillColor.g | pRc->fillColor.b) >> 16;

    bFastColor = (andColor == 0xff) || (orColor == 0) || (!pRc->ditherEnable);

    if (bFastColor && !bFillZ) {
        CHECK_FIFO_SPACE(pjBase, 2);

        CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP | atype_RPL | bop_SRCCOPY |
                      zdrwen_NO_DEPTH | blockm_ON);
        CP_WRITE(pjBase, DWG_FCOL, pRc->hwFillColor);
    } else {
        CHECK_FIFO_SPACE(pjBase, 15);

        // NOTE: For Storm, assuming it's fixed so that the z-buffer isn't
        //       always written, regardless of the 'zdrwen' bit, we will
        //       have to clear the z-buffer using a 2-d blt (don't forget
        //       to reset clipping!).  But with the Athena, we get this
        //       functionality by default.

        if (bFillZ) {
            CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP | atype_ZI | bop_SRCCOPY);
            CP_WRITE(pjBase, DWG_DR0, pRc->hwFillZ);
        } else {
            CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP | bop_SRCCOPY);
        }

        // If we're filling the z-buffer only, zero the plane mask:

        if (bFillZ && !bFillC)
            CP_WRITE(pjBase, DWG_PLNWT, 0);

        CP_WRITE(pjBase, DWG_DR4, (pRc->fillColor.r >> 1));
        CP_WRITE(pjBase, DWG_DR8, (pRc->fillColor.g >> 1));
        CP_WRITE(pjBase, DWG_DR12, (pRc->fillColor.b >> 1));

        // Set all deltas to 0

        CP_WRITE(pjBase, DWG_DR2, 0);
        CP_WRITE(pjBase, DWG_DR3, 0);

        CP_WRITE(pjBase, DWG_DR6, 0);
        CP_WRITE(pjBase, DWG_DR7, 0);

        CP_WRITE(pjBase, DWG_DR10, 0);
        CP_WRITE(pjBase, DWG_DR11, 0);

        CP_WRITE(pjBase, DWG_DR14, 0);
        CP_WRITE(pjBase, DWG_DR15, 0);
    }

    CHECK_FIFO_SPACE(pjBase, 13);

    CP_WRITE(pjBase, DWG_SRC0, 0xffffffff);
    CP_WRITE(pjBase, DWG_SRC1, 0xffffffff);
    CP_WRITE(pjBase, DWG_SRC2, 0xffffffff);
    CP_WRITE(pjBase, DWG_SRC3, 0xffffffff);

    // Set up two vertical edges:

    CP_WRITE(pjBase, DWG_AR1, 0);
    CP_WRITE(pjBase, DWG_AR2, 0);
    CP_WRITE(pjBase, DWG_AR4, 0);
    CP_WRITE(pjBase, DWG_AR5, 0);
    CP_WRITE(pjBase, DWG_SGN, scanleft_RIGHT | sdy_ADD);

    CP_WRITE(pjBase, DWG_CYTOP,
                  ((pEsc->pwo->coClient.rclBounds.top + pRc->hwBufferOffset) * ppdev->cxMemory) +
                  ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_CXLEFT, pEsc->pwo->coClient.rclBounds.left);
    CP_WRITE(pjBase, DWG_CXRIGHT, pEsc->pwo->coClient.rclBounds.right - 1);
    CP_WRITE(pjBase, DWG_CYBOT,
                 ((pEsc->pwo->coClient.rclBounds.bottom + pRc->hwBufferOffset - 1) * ppdev->cxMemory) +
                  ppdev->ulYDstOrg);
}

__inline void HW_FILL_RECT(RXESCAPE *pEsc, RXRC *pRc, RECTL *pRecl)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_SPACE(pjBase, 4);
    CP_WRITE(pjBase, DWG_YDST, pRecl->top + pRc->hwBufferOffset);
    CP_WRITE(pjBase, DWG_FXLEFT, pRecl->left);
    CP_WRITE(pjBase, DWG_LEN, pRecl->bottom - pRecl->top);
    CP_START(pjBase, DWG_FXRIGHT, pRecl->right);
}


__inline void HW_DONE_FILL_RECT(RXESCAPE *pEsc, RXRC *pRc)
{
    if (pEsc->prxCmd != pEsc->pBufEnd)
        HW_INIT_STATE(pEsc, pRc);
}

__inline void HW_START_READ_RECT(RXESCAPE *pEsc, RXRC *pRc,
                                 RXREADRECT *prxReadRect)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_SPACE(pjBase, 4);
    CP_WRITE(pjBase, DWG_CYTOP, ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_CXLEFT, 0);
    CP_WRITE(pjBase, DWG_CXRIGHT, ppdev->cxMemory - 1);
    CP_WRITE(pjBase, DWG_CYBOT,
                ((ppdev->cyMemory - 1) * ppdev->cxMemory) + ppdev->ulYDstOrg);
}


__inline void HW_READ_RECT(RXESCAPE *pEsc, RXRC *pRc, RECTL *prcl,
                           RXREADRECT *prxReadRect, char *pDestRect)
{
    PDEV *ppdev = pEsc->ppdev;
    ULONG ulScreenOffset;
    ULONG cjHwPel;
    LONG lBitmapDelta;
    LONG dx;
    LONG dy;
    BYTE *pjBitmap;
    ULONG ulScreen;
    LONG cj;
    LONG cy;

    if (prxReadRect->sourceBuffer == RX_READ_RECT_FRONT_LEFT) {
        cjHwPel = ppdev->cjHwPel;
        ulScreenOffset = 0;

    } else if (prxReadRect->sourceBuffer == RX_READ_RECT_BACK_LEFT) {
        cjHwPel = ppdev->cjHwPel;
        ulScreenOffset = ppdev->ulBackBuffer;       // Offset in bytes

    } else {
        cjHwPel = 2;                                // Z is always 16bpp
        if (pRc->activeBuffer & RX_BACK_LEFT)
            ulScreenOffset = ppdev->ulBackZBuffer;  // Offset in bytes
        else
            ulScreenOffset = ppdev->ulFrontZBuffer;
    }

    lBitmapDelta = prxReadRect->sharedPitch;    // Pitch specified in bytes

    dx = prxReadRect->destRect.x - prxReadRect->sourceX;
    dy = prxReadRect->destRect.y - prxReadRect->sourceY;

    pjBitmap = pDestRect + (prcl->top + dy) * lBitmapDelta
                         + (prcl->left + dx) * cjHwPel;

    ulScreen = ulScreenOffset
             + cjHwPel * (prcl->top * ppdev->cxMemory + prcl->left);

    cj = (prcl->right - prcl->left) * cjHwPel;
    cy = (prcl->bottom - prcl->top);

    HwAlignedCopy(ppdev, pjBitmap, lBitmapDelta, ulScreen, ppdev->lDelta,
                  cj, cy, FALSE);
}

__inline void HW_DONE_READ_RECT(RXESCAPE *pEsc, RXRC *pRc)
{
    if (pEsc->prxCmd != pEsc->pBufEnd)
        HW_INIT_STATE(pEsc, pRc);
}


__inline void HW_START_WRITE_RECT(RXESCAPE *pEsc, RXRC *pRc,
                                  RXWRITERECT *prxWriteRect)\
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_SPACE(pjBase, 5);
    CP_WRITE(pjBase, DWG_CYTOP, ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_CXLEFT, 0);
    CP_WRITE(pjBase, DWG_CXRIGHT, ppdev->cxMemory - 1);
    CP_WRITE(pjBase, DWG_CYBOT,
                ((ppdev->cyMemory - 1) * ppdev->cxMemory) + ppdev->ulYDstOrg);

    if (prxWriteRect->destBuffer != RX_WRITE_RECT_PIX) {

        // Remember that the plane mask isn't supposed affect writes
        // to 'z':

        CP_WRITE(pjBase, DWG_PLNWT, -1);
    }
}

__inline void HW_WRITE_RECT(RXESCAPE *pEsc, RXRC *pRc, RECTL *prcl,
                            RXWRITERECT *prxWriteRect, char *pSourceRect)
{
    PDEV *ppdev = pEsc->ppdev;
    ULONG ulScreenOffset;
    ULONG cjHwPel;
    LONG lBitmapDelta;
    LONG dx;
    LONG dy;
    BYTE *pjBitmap;
    ULONG ulScreen;
    LONG cj;
    LONG cy;

    if (prxWriteRect->destBuffer == RX_WRITE_RECT_PIX) {
        cjHwPel = ppdev->cjHwPel;

        if (pRc->activeBuffer & RX_BACK_LEFT)
            ulScreenOffset = ppdev->ulBackBuffer;   // Offset in bytes
        else
            ulScreenOffset = 0;
    } else {
        cjHwPel = 2;                                // Z is always 16bpp

        if (pRc->activeBuffer & RX_BACK_LEFT)
            ulScreenOffset = ppdev->ulBackZBuffer;
        else
            ulScreenOffset = ppdev->ulFrontZBuffer;
    }

    lBitmapDelta = prxWriteRect->sharedPitch;   // Pitch specified in bytes

    dx = prxWriteRect->sourceX - prxWriteRect->destRect.x;
    dy = prxWriteRect->sourceY - prxWriteRect->destRect.y;

    pjBitmap = pSourceRect + (prcl->top + dy) * lBitmapDelta
                           + (prcl->left + dx) * cjHwPel;

    ulScreen = ulScreenOffset
             + cjHwPel * (prcl->top * ppdev->cxMemory + prcl->left);

    cj = (prcl->right - prcl->left) * cjHwPel;
    cy = (prcl->bottom - prcl->top);

    HwAlignedCopy(ppdev, pjBitmap, lBitmapDelta, ulScreen, ppdev->lDelta,
                  cj, cy, TRUE);
}

__inline void HW_DONE_WRITE_RECT(RXESCAPE *pEsc, RXRC *pRc)
{
    if (pEsc->prxCmd != pEsc->pBufEnd)
        HW_INIT_STATE(pEsc, pRc);
}


__inline void HW_START_SWAP_BUFFERS(RXESCAPE *pEsc, RXRC *pRc,
                                    RXSWAPBUFFERS *prxSwapBuffers)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    RXWINDOW *pWnd = pEsc->pWnd;

    CHECK_FIFO_SPACE(pjBase, 4);
    CP_WRITE(pjBase, DWG_DWGCTL, opcode_BITBLT + atype_RPL + blockm_OFF +
                                 bltmod_BFCOL + pattern_OFF + transc_BG_OPAQUE +
                                 bop_SRCCOPY);
    CP_WRITE(pjBase, DWG_SHIFT, 0);
    CP_WRITE(pjBase, DWG_SGN, 0);
    CP_WRITE(pjBase, DWG_AR5, ppdev->cxMemory);
}


__inline void HW_SWAP_RECT(RXESCAPE *pEsc, RXRC *pRc, RECTL *pRecl)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    ULONG sourceOffset;

    CHECK_FIFO_SPACE(pjBase, 10);

    // We always have to reset the clipping, whether it's being applied
    // to the front or the back buffer:

    CP_WRITE(pjBase, DWG_CYTOP, ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_CXLEFT, 0);
    CP_WRITE(pjBase, DWG_CXRIGHT, ppdev->cxMemory - 1);
    CP_WRITE(pjBase, DWG_CYBOT, ((ppdev->cyMemory - 1) * ppdev->cxMemory +
                                ppdev->ulYDstOrg));

    CP_WRITE(pjBase, DWG_LEN, pRecl->bottom - pRecl->top);
    CP_WRITE(pjBase, DWG_FXLEFT, pRecl->left);
    CP_WRITE(pjBase, DWG_FXRIGHT, pRecl->right - 1);
    CP_WRITE(pjBase, DWG_YDST, pRecl->top);

    // Note that the window offset has already been applied to 'pRecl';
    // we only have to add in the back-buffer sourceOffset:

    sourceOffset = (pRecl->top + pRc->hwBackBufferOffset) * ppdev->cxMemory
           + pRecl->left + ppdev->ulYDstOrg;

    CP_WRITE(pjBase, DWG_AR3, sourceOffset);
    CP_START(pjBase, DWG_AR0, sourceOffset + pRecl->right - pRecl->left - 1);
}


__inline void HW_DONE_SWAP_BUFFERS(RXESCAPE *pEsc, RXRC *pRc)
{
    PDEV *ppdev = pEsc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    if (pEsc->prxCmd != pEsc->pBufEnd)
        HW_INIT_STATE(pEsc, pRc);           // !!! This seems bogus
}

__inline void HW_ENABLE_BUFFERS(RXESCAPE *pEsc, RXRC *pRc,
                                RXENABLEBUFFERS *prxEnableBuffers)
{
    HwEnableBuffers(pEsc, pRc, prxEnableBuffers);
}

#define HW_FREE_BUFFERS(pWnd)

#define HW_FLUSH(pEsc)

__inline void HW_CREATE_RC(RXESCAPE *pEsc, RXRC *pRc)
{
    HwCreateRc(pEsc, pRc);
}

__inline void HW_DESTROY_RC(RXRC *pRc)
{
    HwDestroyRc(pRc);
}

__inline void HW_INIT_DEFAULT_RC(RXESCAPE *pEsc, RXRC *pRc)
{
    PDEV *ppdev = pEsc->ppdev;

    pRc->hwZBytesPerPixel = 2;
    pRc->hwZPitch = ppdev->cxScreen * 2;

    switch (ppdev->iBitmapFormat) {
        case BMF_8BPP:
            pRc->hwCBytesPerPixel = 1;
            pRc->rShift = 5;
            pRc->gShift = 5;
            pRc->bShift = 6;
            pRc->hwRShift = 5;
            pRc->hwGShift = 2;
            pRc->hwBShift = 0;
            pRc->hwBpp = 8;
            break;
        case BMF_16BPP:
            pRc->hwCBytesPerPixel = 2;
            if (ppdev->flGreen == 0x3e0) {  // 5-5-5
                pRc->rShift = 3;
                pRc->gShift = 3;
                pRc->bShift = 3;
                pRc->hwRShift = 10;
                pRc->hwGShift = 5;
                pRc->hwBShift = 0;
                pRc->hwBpp = 15;
            } else {                        // 5-6-5
                pRc->rShift = 3;
                pRc->gShift = 2;
                pRc->bShift = 3;
                pRc->hwRShift = 11;
                pRc->hwGShift = 5;
                pRc->hwBShift = 0;
                pRc->hwBpp = 16;
            }
            break;
        default:
            pRc->hwCBytesPerPixel = 4;
            pRc->rShift = 0;
            pRc->gShift = 0;
            pRc->bShift = 0;
            pRc->hwRShift = 16;
            pRc->hwGShift = 8;
            pRc->hwBShift = 0;
            pRc->hwBpp = 24;
            break;
    }

    pRc->aShift += 32;      // no alpha for now...
    pRc->hwAShift = 0;

    pRc->hwCPitch = ppdev->cxMemory;
    pRc->hwSolidColor = 0;
    pRc->hwFillColor = 0;
    pRc->backBufEnabled = FALSE;
    pRc->hwPlaneMask = pRc->planeMask;
    pRc->hwFillZ = (ULONG)pRc->fillZ >> 16;
    pRc->hwIntLineFunc = opcode_AUTOLINE_OPEN | atype_RPL | bop_SRCCOPY |
                        zdrwen_NO_DEPTH | bltmod_BFCOL | transc_BG_TRANSP;
    pRc->hwLineFunc = opcode_AUTOLINE_OPEN | atype_RPL | bop_SRCCOPY |
                      zdrwen_NO_DEPTH;
    pRc->hwTrapFunc = opcode_TRAP | atype_RPL | bop_SRCCOPY |
                      zdrwen_NO_DEPTH;
    pRc->hwSpanFunc = opcode_AUTOLINE_CLOSE | atype_ZI | bop_SRCCOPY |
                      zdrwen_NO_DEPTH;

    pRc->hwBufferOffset = 0;
    if (ppdev->flHwCaps) {

        // Default to only integer lines for now.  HwCreateRc will update
        // this list, if necessary:

        pRc->primFunc[RX_PRIM_INTLINESTRIP] = DrawIntLineStrip;
    }

    HW_SET_ROP_FUNC(pEsc, pRc);
    HW_PLANE_MASK(pEsc, pRc, pRc->planeMask);
}

__inline void HW_INIT_DEFAULT_WNDOBJ(RXWINDOW *pWnd)
{
}

#define HW_TRACK_WNDOBJ(pwo, deltaType) // !!! Move Z buffer and back buffer

__inline void HW_INFO_CHECK_MODE(RXESCAPE *pEsc, RXGETINFO *prxGetInfo)
{
    PDEV *ppdev = pEsc->ppdev;

    if (prxGetInfo->infoType != RX_INFO_GLOBAL_CAPS) {

        // We allow color-index when running at 8bpp for integer lines:

        if ((prxGetInfo->flags & RX_GET_INFO_COLOR_INDEX) &&
            (ppdev->iBitmapFormat != BMF_8BPP)) {
            ERROR_RETURN;
        }

        // We don't allow RGB at 8bpp if we don't have full 3D acceleration
        // (otherwise OpenGL will always realize a 3:3:2 palette for us
        // when we don't need it and don't particularly want it):

        if (!(prxGetInfo->flags & RX_GET_INFO_COLOR_INDEX) &&
            !(ppdev->flHwCaps & HWCAPS_ALL) &&
            (ppdev->iBitmapFormat == BMF_8BPP)) {
            ERROR_RETURN;
        }

        // !!! We only support RX_QUERY_CURRENT_MODE for now

        if (!(prxGetInfo->flags & RX_QUERY_CURRENT_MODE)) {
            ERROR_RETURN;
        }
    }
}

__inline void HW_CONTEXT_CHECK_MODE(RXESCAPE *pEsc,
                                    RXCREATECONTEXT *prxCreateContext)
{
    RXRC *pRc = pEsc->pRc;
    PDEV *ppdev = pEsc->ppdev;

    // We allow color-index when running at 8bpp for integer lines:

    if ((prxCreateContext->flags & RX_COLOR_INDEXED) &&
        (ppdev->iBitmapFormat != BMF_8BPP)) {

        RXDBG_PRINT("RxCreateContext: RX_COLOR_INDEX invalid.");
        ERROR_RETURN;
    }

    // We don't allow RGB at 8bpp if we don't have full 3D acceleration
    // (otherwise OpenGL will always realize a 3:3:2 palette for us
    // when we don't need it and don't particularly want it):

    if (!(prxCreateContext->flags & RX_COLOR_INDEXED) &&
        !(ppdev->flHwCaps & HWCAPS_ALL) &&
        (ppdev->iBitmapFormat == BMF_8BPP)) {
        ERROR_RETURN;
    }
}


__inline void HW_GLOBAL_INFO(RXESCAPE *pEsc, RXGETINFO *prxGetInfo, RXGLOBALINFO *prxGlobalInfo)
{
    prxGlobalInfo->verMajor = RX_VERSION_MAJOR;
    prxGlobalInfo->verMinor = RX_VERSION_MINOR;
    prxGlobalInfo->verDriver = 1;
}

__inline void HW_SURFACE_INFO(RXESCAPE *pEsc, RXGETINFO *prxGetInfo, RXSURFACEINFO *prxSurfaceInfo)
{
    PDEV *ppdev = pEsc->ppdev;

    switch (ppdev->iBitmapFormat) {
        case BMF_8BPP:
            prxSurfaceInfo->colorBytesPerPixel = 1;
            prxSurfaceInfo->rDepth    = 3;
            prxSurfaceInfo->gDepth    = 3;
            prxSurfaceInfo->bDepth    = 2;
            prxSurfaceInfo->aDepth    = 0;
            prxSurfaceInfo->rBitShift = 5;
            prxSurfaceInfo->gBitShift = 2;
            prxSurfaceInfo->bBitShift = 0;
            prxSurfaceInfo->aBitShift = 0;
            break;
        case BMF_16BPP:
            prxSurfaceInfo->colorBytesPerPixel = 2;
            if (ppdev->flGreen == 0x3e0) {      // 5-5-5
                prxSurfaceInfo->rDepth    = 5;
                prxSurfaceInfo->gDepth    = 5;
                prxSurfaceInfo->bDepth    = 5;
                prxSurfaceInfo->aDepth    = 0;
                prxSurfaceInfo->rBitShift = 10;
                prxSurfaceInfo->gBitShift = 5;
                prxSurfaceInfo->bBitShift = 0;
                prxSurfaceInfo->aBitShift = 0;
            } else {                            // 5-6-5
                prxSurfaceInfo->rDepth    = 5;
                prxSurfaceInfo->gDepth    = 6;
                prxSurfaceInfo->bDepth    = 5;
                prxSurfaceInfo->aDepth    = 0;
                prxSurfaceInfo->rBitShift = 11;
                prxSurfaceInfo->gBitShift = 5;
                prxSurfaceInfo->bBitShift = 0;
                prxSurfaceInfo->aBitShift = 0;
            }
            break;
        default:
            prxSurfaceInfo->colorBytesPerPixel = 4;
            prxSurfaceInfo->rDepth    = 8;
            prxSurfaceInfo->gDepth    = 8;
            prxSurfaceInfo->bDepth    = 8;
            prxSurfaceInfo->aDepth    = 0;
            prxSurfaceInfo->rBitShift = 16;
            prxSurfaceInfo->gBitShift = 8;
            prxSurfaceInfo->bBitShift = 0;
            prxSurfaceInfo->aBitShift = 0;
            break;
    }

    if (ppdev->flHwCaps & HWCAPS_ALL) {
        prxSurfaceInfo->flags = RX_BACK_BUFFER;
        prxSurfaceInfo->zBytesPerPixel = 2;
        prxSurfaceInfo->zDepth    = 16;
        prxSurfaceInfo->zBitShift = 16;
    } else {
        prxSurfaceInfo->flags = 0;
        prxSurfaceInfo->zBytesPerPixel = 0;
        prxSurfaceInfo->zDepth    = 0;
        prxSurfaceInfo->zBitShift = 0;
    }
}

__inline void HW_SPAN_CAPS(RXESCAPE *pEsc, RXGETINFO *prxGetInfo, RXCAPS *prxCaps)
{
    PDEV *ppdev = pEsc->ppdev;

    if ((prxGetInfo->flags & RX_GET_INFO_COLOR_INDEX)
     || !(ppdev->flHwCaps & HWCAPS_ALL)) {
        SET_ERROR_CODE;
    } else {
        prxCaps->miscCaps     = RX_MASK_PLANES | RX_MASK_Z | RX_MASK_MSB |
                                RX_HORIZONTAL_SPANS;
        prxCaps->rasterCaps   = RX_RASTER_ROP2 | RX_RASTER_DITHER;
        prxCaps->zCmpCaps     = RX_CMP_LESS | RX_CMP_LEQUAL;
        prxCaps->srcBlendCaps = 0;
        prxCaps->dstBlendCaps = 0;
        prxCaps->alphaCmpCaps = 0;
        prxCaps->shadeCaps    = RX_SHADE_SMOOTH;
        prxCaps->texCaps      = 0;
        prxCaps->texFilterCaps= 0;
        prxCaps->texBlendCaps = 0;
        prxCaps->texMaxWidth  = 0;
        prxCaps->texMaxHeight = 0;
        prxCaps->rasterCalcType = RX_RASTER_FIXED;
        prxCaps->fractionalRasterBits = 15;
    }
}


__inline void HW_LINE_CAPS(RXESCAPE *pEsc, RXGETINFO *prxGetInfo, RXCAPS *prxCaps)
{
    PDEV *ppdev = pEsc->ppdev;

    if ((prxGetInfo->flags & RX_GET_INFO_COLOR_INDEX)
     || !(ppdev->flHwCaps & HWCAPS_ALL)) {
        SET_ERROR_CODE;
    } else {
        prxCaps->miscCaps     = RX_MASK_PLANES | RX_MASK_Z;
        prxCaps->rasterCaps   = RX_RASTER_ROP2;
        prxCaps->zCmpCaps     = RX_CMP_LESS | RX_CMP_LEQUAL;
        prxCaps->srcBlendCaps = 0;
        prxCaps->dstBlendCaps = 0;
        prxCaps->alphaCmpCaps = 0;
        prxCaps->shadeCaps    = RX_SHADE_SMOOTH;
        prxCaps->texCaps      = 0;
        prxCaps->texFilterCaps= 0;
        prxCaps->texBlendCaps = 0;
        prxCaps->texMaxWidth  = 0;
        prxCaps->texMaxHeight = 0;
        prxCaps->rasterCalcType = RX_RASTER_ERROR_TERM;
        prxCaps->fractionalRasterBits = 0;
    }
}


__inline void HW_INTLINE_CAPS(RXESCAPE *pEsc, RXGETINFO *prxGetInfo, RXCAPS *prxCaps)
{
    PDEV *ppdev = pEsc->ppdev;

    prxCaps->miscCaps     = RX_MASK_PLANES;
    prxCaps->rasterCaps   = RX_RASTER_ROP2 | RX_RASTER_PAT;
    prxCaps->zCmpCaps     = 0;
    prxCaps->srcBlendCaps = 0;
    prxCaps->dstBlendCaps = 0;
    prxCaps->alphaCmpCaps = 0;

    // If running on a board that doesn't support all 3d capabilities,
    // or if running on a board that supports all 3d capabilities but
    // only in RGB mode and we're asked about color index, we return the
    // fact that the hardware won't do shaded integer lines:

    if ((prxGetInfo->flags & RX_GET_INFO_COLOR_INDEX)
     || !(ppdev->flHwCaps & HWCAPS_ALL)) {
        prxCaps->shadeCaps = 0;
    } else {
        prxCaps->shadeCaps = RX_SHADE_SMOOTH;
    }

    prxCaps->texCaps      = 0;
    prxCaps->texFilterCaps= 0;
    prxCaps->texBlendCaps = 0;
    prxCaps->texMaxWidth  = 0;
    prxCaps->texMaxHeight = 0;
    prxCaps->rasterCalcType = RX_RASTER_ERROR_TERM;
    prxCaps->fractionalRasterBits = 0;
}

//#define DEBUG_SPANS	1


__inline void HW_TRIANGLE_CAPS(RXESCAPE *pEsc, RXGETINFO *prxGetInfo, RXCAPS *prxCaps)
{
#if DEBUG_SPANS
    SET_ERROR_CODE;
#else
    PDEV *ppdev = pEsc->ppdev;

    if ((prxGetInfo->flags & RX_GET_INFO_COLOR_INDEX)
     || !(ppdev->flHwCaps & HWCAPS_ALL)) {
        SET_ERROR_CODE;
    } else {
        prxCaps->miscCaps     = RX_MASK_PLANES | RX_MASK_Z;
        prxCaps->rasterCaps   = RX_RASTER_ROP2 | RX_RASTER_DITHER;
        prxCaps->zCmpCaps     = RX_CMP_LESS | RX_CMP_LEQUAL;
        prxCaps->srcBlendCaps = 0;
        prxCaps->dstBlendCaps = 0;
        prxCaps->alphaCmpCaps = 0;
        prxCaps->shadeCaps    = RX_SHADE_SMOOTH;
        prxCaps->texCaps      = 0;
        prxCaps->texFilterCaps= 0;
        prxCaps->texBlendCaps = 0;
        prxCaps->texMaxWidth  = 0;
        prxCaps->texMaxHeight = 0;
        prxCaps->rasterCalcType = RX_RASTER_ERROR_TERM;
        prxCaps->fractionalRasterBits = 0;
    }
#endif
}


#define HW_QUAD_CAPS(pEsc, prxGetInfo, prxCaps)\
    SET_ERROR_CODE;


//-------------------------------------------------------------------------
// Utility macros/functions
//-------------------------------------------------------------------------

__inline ULONG GETSOLIDCOLOR(RXRC *pRc, RXCOLOR *prxColor)
{
    ULONG color;

    color =
        (((ULONG)((UCHAR)(prxColor->r >> pRc->rShift)) << pRc->hwRShift) |
         ((ULONG)((UCHAR)(prxColor->g >> pRc->gShift)) << pRc->hwGShift) |
         ((ULONG)((UCHAR)(prxColor->b >> pRc->bShift)) << pRc->hwBShift));

    switch (pRc->hwBpp) {
        case 8:
            color = (color | (color << 8) |
                    (color << 16) | (color << 24));
            break;
        case 15:
        case 16:
            color = color | (color << 16);
            break;
        default:
            break;
    }

    return color;
}

__inline VOID *HW_CREATE_TEXTURE_HEAP(RXESCAPE *pEsc,
                                      RXTEXTUREHEAP *prxTextureHeap)
{
    return (VOID *)NULL;
}

__inline VOID *HW_ALLOC_TEXTURE(RXESCAPE *pEsc, RXHANDLE handle, VOID *pHeap,
                                RXTEXTURE **pprxTexture)
{
    return (VOID *)NULL;
}

__inline void HW_CLEAR_TEXTURE_HEAP(RXESCAPE *pEsc, VOID *pHeap)
{
    return;
}

__inline void HW_TEXTURE(RXESCAPE *pEsc, void *pTex, BOOL state)
{
    return;
}


__inline void HW_FREE_TEXTURE_HEAP(VOID *pHeap)
{
    return;
}

__inline BOOL HW_FREE_TEXTURE(VOID *pTexture, BOOL bForce)
{
    return FALSE;
}

__inline BOOL HW_LOAD_TEXTURE(RXESCAPE *pEsc, VOID *pTex,
                              RXTEXTURE **pprxTexture)
{
    return FALSE;
}


__inline void HW_ACTIVATE_TEXTURE(RXESCAPE *pEsc,
                                  ULONG x, ULONG y, ULONG w, ULONG h)
{
    return;
}

__inline void HW_TEX_MEM_STATS(RXESCAPE *pEsc, RXTEXMEMSTATS *prxTexMemStats,
                               void *pHeap)
{
    return;
}


#endif /* _RXHW_H */

