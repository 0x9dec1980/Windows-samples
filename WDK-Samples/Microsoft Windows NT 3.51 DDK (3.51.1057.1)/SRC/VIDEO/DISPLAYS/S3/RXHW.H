/******************************Module*Header*******************************\
* Module Name: rxhw.h
*
* This header contains macros for much of the hardware-dependent
* functionality required in 'rxddi.c'.
*
* The S3 driver supports the following capabilities for the 3D DDI:
*
*   o Integer lines
*
* Copyright (c) 1994-1995, Microsoft Corporation
*
\**************************************************************************/

VOID vMmDrawLines(RXESCAPE*, RXDRAWPRIM*);
VOID vIoDrawLines(RXESCAPE*, RXDRAWPRIM*);

VOID HwReadRect(RXESCAPE*, RXRC*, RECTL*, RXREADRECT*, VOID*);
VOID HwWriteRect(RXESCAPE*, RXRC*, RECTL*, RXWRITERECT*, VOID*);
ULONG HwMapColor(RXESCAPE*, RXRC*, RXCOLOR*);

#define HW_RX_ENABLE(pInfo, bStatus)\
{\
    bStatus = TRUE;\
}

//////////////////////////////////////////////////////////////////////
// State macros -- for setting Rendering Context state

#define HW_SOLID_COLOR(pEsc, pRc, prxColor)\
{\
    pRc->solidColor = *prxColor;\
    pRc->hwSolidColor = HwMapColor(pEsc, pRc, prxColor);\
}

#define HW_FILL_COLOR(pEsc, pRc, prxColor)\
{\
    pRc->fillColor = *prxColor;\
    pRc->hwFillColor = HwMapColor(pEsc, pRc, prxColor);\
}

#define HW_FILL_Z(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_PLANE_MASK(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_Z_MASK(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_ACTIVE_BUFFER(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_INIT_STATE(pEsc, pRc)\
{\
    PDEV* ppdev = pEsc->ppdev;\
\
    /* Do all drawing to the RC's current buffer */\
\
    ppdev->xOffset = ppdev->ptlDoubleBuffer[pRc->activeBuffer & 1].x;\
    ppdev->yOffset = ppdev->ptlDoubleBuffer[pRc->activeBuffer & 1].y;\
}

#define HW_DEFAULT_STATE(pEsc)

#define HW_ROP2(pEsc, pRc, rxState)\
{\
    if ((rxState > R2_LAST) || (rxState < R2_BLACK))\
    {\
        RXDBG_PRINT("Invalid ROP2 function, Rop2 = %d", rxState);\
        SET_ERROR_CODE;\
    }\
    else\
    {\
        pRc->rop2 = rxState;\
        pRc->hwRop2 = gajHwMixFromMix[rxState];\
    }\
}

#define HW_CULL_MODE(pEsc, pRc, rxState)\
{\
    if (rxState != RX_CULL_NONE)\
        SET_ERROR_CODE;\
}

#define HW_SPAN_RXREAL_MODE(pEsc, pRc, rxState)\
{\
    if (rxState != RX_FIXED)\
        SET_ERROR_CODE;\
}

#define HW_POINT_RXREAL_MODE(pEsc, pRc, rxState)\
{\
    if ((rxState == RX_INTEGER) ||\
        (rxState == RX_FIXED))\
        pRc->pointRealMode = rxState;\
    else\
        SET_ERROR_CODE;\
}

#define HW_LINE_RXREAL_MODE(pEsc, pRc, rxState)\
{\
    if ((rxState == RX_INTEGER) ||\
        (rxState == RX_FIXED))\
        pRc->lineRealMode = rxState;\
    else\
        SET_ERROR_CODE;\
}


#define HW_TRIANGLE_RXREAL_MODE(pEsc, pRc, rxState)\
{\
    if (rxState != RX_FIXED)\
        SET_ERROR_CODE;\
}

#define HW_LAST_PIXEL(pEsc, pRc, rxState)\
{\
    pRc->lastPixel = (BOOL)rxState;\
}

#define HW_Z_FUNC(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_SHADE_MODE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_Z_ENABLE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_VERTEX_TYPE(pEsc, pRc, rxState)\
{\
}

#define HW_SPAN_COLOR_TYPE(pEsc, pRc, rxState)\
{\
}

#define HW_VERTEX_COLOR_TYPE(pEsc, pRc, rxState)\
{\
}

#define HW_MASK_START(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_BLEND_ENABLE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_ALPHA_TEST_ENABLE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_SCISSORS_ENABLE(pEsc, pRc)\
{\
}

#define HW_DITHER_ENABLE(pEsc, pRc, rxState)

#define HW_SCISSORS_RECT(pEsc, pRc, prxRect)\
{\
    pRc->scissorsRect = *prxRect;\
}

#define HW_SPAN_TYPE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_SPAN_DIRECTION(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
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

#define HW_TEXTURE(pEsc, pRc, rxState)\
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

#define HW_LINE_PATTERN(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

#define HW_STIPPLE(pEsc, pRc, rxState)\
{\
    SET_ERROR_CODE;\
}

//////////////////////////////////////////////////////////////////////
// Rendering macros

#define HW_START_FILL_RECT(pEsc, pRc, rxFillRect)

#define HW_FILL_RECT(pEsc, pRc, pRecl)\
{\
    PDEV* ppdev;\
    RBRUSH_COLOR rbc;\
\
    rbc.iSolidColor = pRc->hwFillColor;\
    ppdev = pEsc->ppdev;\
    ppdev->pfnFillSolid(ppdev, 1, pRecl, OVERPAINT, OVERPAINT, rbc, NULL);\
}

#define HW_DONE_FILL_RECT(pEsc, pRc)



#define HW_START_READ_RECT(pEsc, pRc, rxReadRect)\

#define HW_READ_RECT(pEsc, pRc, prclReadRectClip, rxReadRect, pDest)\
{\
    HwReadRect(pEsc, pRc, prclReadRectClip, &rxReadRect, pDest);\
}

#define HW_DONE_READ_RECT(pEsc, pRc)



#define HW_START_WRITE_RECT(pEsc, pRc, rxWriteRect)

#define HW_WRITE_RECT(pEsc, pRc, prclWriteRectClip, rxWriteRect, pSource)\
{\
    HwWriteRect(pEsc, pRc, prclWriteRectClip, &rxWriteRect, pSource);\
}

#define HW_DONE_WRITE_RECT

#define HW_START_SWAP_BUFFERS(pEsc, pRc, rxSwapBuffers)

#define HW_SWAP_RECT(pEsc, pRc, pRecl)

#define HW_DONE_SWAP_BUFFERS(pEsc, pRc)


#define HW_ENABLE_BUFFERS(pEsc, pRc, rxEnableBuffers)\
{\
    SET_ERROR_CODE;\
}

#define HW_FREE_BUFFERS(pWnd)

#define HW_FLUSH(pEsc)\
{\
    IO_GP_WAIT(pEsc->ppdev);\
}

#define HW_INIT_DEFAULT_RC(pEsc, pRc)\
{\
    PDEV* ppdev;\
\
    /* Defaults should correspond to RC defaults */\
\
    ppdev = pEsc->ppdev;\
    pRc->hwRop2 = OVERPAINT;\
    pRc->hwSolidColor = 0;\
    pRc->hwFillColor = 0;\
    pRc->backBufEnabled = FALSE;\
    if ((ppdev->flCaps & CAPS_MM_IO) && (ppdev->flCaps & CAPS_16_ENTRY_FIFO))\
    {\
        pRc->primFunc[RX_PRIM_INTLINESTRIP] = vMmDrawLines;\
    }\
    else\
    {\
        pRc->primFunc[RX_PRIM_INTLINESTRIP] = vIoDrawLines;\
    }\
}

#define HW_INIT_DEFAULT_WNDOBJ(pPriv)\
{\
}

#define HW_TRACK_WNDOBJ(pwo, deltaType)

#define HW_INFO_CHECK_MODE(pEsc, rxGetInfo)\
{\
    if (rxGetInfo.infoType != RX_INFO_GLOBAL_CAPS)\
    {\
        /* We support only color-index at 8bpp, and RGB at 16bpp and */\
        /* higher.  We don't support RGB at 8bpp, because we do no shading, */\
        /* and because OpenGL would always realize a 3:3:2 palette, which */\
        /* clobbers the palette for all other applications running */\
\
        if (pEsc->ppdev->iBitmapFormat == BMF_8BPP) {\
            if (!(rxGetInfo.flags & RX_GET_INFO_COLOR_INDEX)) {\
                ERROR_RETURN;\
            }\
        } else {\
            if (rxGetInfo.flags & RX_GET_INFO_COLOR_INDEX) {\
                ERROR_RETURN;\
            }\
        }\
\
        /* !!! We only support RX_QUERY_CURRENT_MODE for now */\
\
        if (!(rxGetInfo.flags & RX_QUERY_CURRENT_MODE)) {\
            ERROR_RETURN;\
        }\
    }\
}

#define HW_CONTEXT_CHECK_MODE(pEsc, rxCreateContext)\
{\
    if (((rxCreateContext.flags & ~RX_COLOR_INDEXED) != 0) ||\
        ((rxCreateContext.flags & RX_COLOR_INDEXED) &&\
         (pEsc->ppdev->iBitmapFormat != BMF_8BPP)) ||\
        (!(rxCreateContext.flags & RX_COLOR_INDEXED) &&\
         (pEsc->ppdev->iBitmapFormat == BMF_8BPP)))\
    {\
        RXDBG_PRINT("RxCreateContext: Invalid flags.");\
        ERROR_RETURN;\
    }\
}

#define HW_GLOBAL_INFO(pEsc, prxGlobalInfo)\
{\
    prxGlobalInfo->verMajor = RX_VERSION_MAJOR;\
    prxGlobalInfo->verMinor = RX_VERSION_MINOR;\
    prxGlobalInfo->verDriver = 1;\
}

#define HW_SURFACE_INFO(pEsc, prxSurfaceInfo)\
{\
    PDEV* ppdev = pEsc->ppdev;\
\
    prxSurfaceInfo->flags     = 0;\
    prxSurfaceInfo->colorBytesPerPixel = 1 << ppdev->cPelSize;\
    prxSurfaceInfo->rDepth    = ppdev->rDepth;\
    prxSurfaceInfo->gDepth    = ppdev->gDepth;\
    prxSurfaceInfo->bDepth    = ppdev->bDepth;\
    prxSurfaceInfo->aDepth    = 0;\
    prxSurfaceInfo->rBitShift = ppdev->rBitShift;\
    prxSurfaceInfo->gBitShift = ppdev->gBitShift;\
    prxSurfaceInfo->bBitShift = ppdev->bBitShift;\
    prxSurfaceInfo->aBitShift = 0;\
    prxSurfaceInfo->zBytesPerPixel = 0;\
    prxSurfaceInfo->zDepth    = 0;\
    prxSurfaceInfo->zBitShift = 0;\
}

#define HW_SPAN_CAPS(pEsc, prxCaps)\
{\
    SET_ERROR_CODE;\
}


#define HW_POINT_CAPS(pEsc, prxCaps)\
{\
    SET_ERROR_CODE;\
}


#define HW_LINE_CAPS(pEsc, prxCaps)\
{\
    SET_ERROR_CODE;\
}


#define HW_INTLINE_CAPS(pEsc, prxCaps)\
{\
    prxCaps->miscCaps     = 0;\
    prxCaps->rasterCaps   = RX_RASTER_ROP2;\
    prxCaps->zCmpCaps     = 0;\
    prxCaps->srcBlendCaps = 0;\
    prxCaps->dstBlendCaps = 0;\
    prxCaps->alphaCmpCaps = 0;\
    prxCaps->shadeCaps    = 0;\
    prxCaps->texCaps      = 0;\
    prxCaps->texFilterCaps= 0;\
    prxCaps->texBlendCaps = 0;\
    prxCaps->texMaxWidth  = 0;\
    prxCaps->texMaxHeight = 0;\
    prxCaps->fractionalRasterBits = 0;\
}


#define HW_TRIANGLE_CAPS(pEsc, prxCaps)\
{\
    SET_ERROR_CODE;\
}


#define HW_QUAD_CAPS(pEsc, prxCaps)\
{\
    SET_ERROR_CODE;\
}

