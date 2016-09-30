/******************************Module*Header*******************************\
* Module Name: rxddi.c
*
* This module contains all the base functionality required for the 3D DDI.
*
* This file, as well as 'rxddi.h', should not require any changes to be
* made when porting to other NT drivers -- instead, changes should be
* made to the hardware-specific functionality provided by macros in
* 'rxhw.h' and routines in 'rxhw.c'.
*
* Tip: You really don't want to change this file if you don't have to.
*      Doing so will give you more time to concentrate on your rendering
*      routines, and will make it easier to pick up the inevitable changes
*      and bug fixes we'll be making in the future.
*
* We have made an attempt to separate out Windows NT specific functionality
* to facilitate ports to Windows 95.  Implicit is the assumption that
* entirely 32-bit code may be used.
*
* Goals
* -----
*
* Pervasive throughout this implementation is the influence of the
* following goals:
*
* 1. Robustness
*
*    Windows NT is first and foremost a robust operating system.  There
*    is a simple measure for this: a robust system should never crash.
*    Because the display driver is a trusted component of the operating
*    system, and because the 3D DDI is directly callable from any
*    untrusted application, this has a significant impact on the way we
*    must do things.
*
* 2. Performance
*
*    Performance is the 'raison d'etre' of the 3D DDI; we have tried to
*    have as thin a layer above the rendering code as we could.
*
* 3. Portability
*
*    This implementation is intended to be portable to other
*    graphics accelerators, to different processor types, and to the
*    Windows 95 operating system.
*
* Obviously, Windows 95 implementations may choose to have a different
* order of priority for these goals, and so some of the robustness
* code may be eliminated.  But it is still recommended that it be kept;
* the overhead is reasonably minimal, and people really don't like it
* when their systems crash...
*
* The Rules of Robustness
* -----------------------
*
* 1. Nothing given by the caller can be trusted.
*
*    For example, handles cannot be trusted to be valid.  Handles passed
*    in may actually be for objects not owned by the caller.  Pointers
*    and offsets may not be correctly aligned.  Pointers, offsets, and
*    coordinates may be out of bounds.
*
* 2. Parameters can be asynchronously modified at any time.
*
*    Many commands come from shared memory sections, and any data therein
*    may be asynchronously modified by other threads in the calling
*    application.  As such, parameters may never be validated in-place
*    in the shared section, because the application may corrupt the data
*    after validation but before its use.  Instead, parameters must always
*    be first copied out of the window, and then validated on the safe
*    copy.
*
* 3. We must clean up.
*
*    Applications may die at any time before calling the appropriate
*    clean up functions.  As such, we have to be prepared to clean up
*    any resources ourselves when the application dies.
*
* Important Note
* --------------
*
* The 3D DDI is all about hardware acceleration.  Only operations that
* can be directly accelerated in hardware should be supported by a 3D DDI
* driver; by design, the 3D DDI specifies many optional capabilities
* to facilitate this.  It is intended that any application or drawing
* library sitting atop a 3D DDI driver can query the driver and be
* assured that anything hooked will truly be accelerated.
*
* Please don't go implementing functionality in software for a 3D DDI
* driver!
*
* Copyright (c) 1994-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#include "rxddi.h"
#include "rxhw.h"

// Function table for 3D-DDI routines implemented at the top level of
// the main (rxddi) driver module.  Further functionality should be added
// through RxEnable():

RXFUNC rxFuncs[] =
{
    RxNullFunc,               // undefined
    RxNullFunc,               // RXCMD_ALLOC_TEXTURE
    RxNullFunc,               // RXCMD_CREATE_CONTEXT (called directly)
    RxDeleteResource,         // RXCMD_DELETE_RESOURCE
    RxDrawPrim,               // RXCMD_DRAW_PRIM
    RxEnableBuffers,          // RXCMD_ENABLE_BUFFERS
    RxFillRect,               // RXCMD_FILL_RECT
    RxFlush,                  // RXCMD_FLUSH
    RxGetInfo,                // RXCMD_GET_INFO
    RxNullFunc,               // RXCMD_LOAD_TEXTURE
    RxNullFunc,               // RXCMD_MAP_MEM (called directly)
    RxNullFunc,               // RXCMD_POLY_DRAW_SPAN
    RxNullFunc,               // RXCMD_QUERY_TEXTURE_MEMORY
    RxReadRect,               // RXCMD_READ_RECT
    RxSetState,               // RXCMD_SET_STATE
    RxSwapBuffers,            // RXCMD_SWAP_BUFFERS
    RxNullFunc,               // RXCMD_TEXTURE_HEAP
    RxWriteRect               // RXCMD_WRITE_RECT
};


//****************************************************************************
// BOOL RxEnable(void *pInfo)
//
// This function should be called when the driver is enabled.  Static
// resources (such as internal control structured used by the 3D-DDI)
// should be allocated at this time, and any other structured such as
// the rxFuncs function-table should be modified as needed.
//****************************************************************************

BOOL RxEnable(void *pInfo)
{
    BOOL bStatus;

    HW_RX_ENABLE(pInfo, bStatus);
    return bStatus;
}


//****************************************************************************
// ULONG RxHandler(RXESCAPE *pEsc)
//
// This is the main 3D-DDI function handler.  At this point, there should
// be no platform-specific code since these should have been resolved by
// the escape function.
//****************************************************************************

ULONG RxHandler(RXESCAPE *pEsc)
{
    RXHDR *prxHdr = pEsc->prxHdr;
    RXRC *pRc;

    // Assume that return value is TRUE

    pEsc->retValue = TRUE;

    // If the command buffer is in shared memory, dereference the memory
    // from the handle and check the bounds.

    if (prxHdr->hrxSharedMem)
    {
        RXMEMOBJ *pMemObj = (RXMEMOBJ *)GetPtrFromHandle(prxHdr->hrxSharedMem,
                                                         RXHANDLE_SHAREMEM);

        if (!pMemObj)
        {
            RXDBG_PRINT("RxHandler: Invalid handle for shared memory.");
            return FALSE;
        }

        pEsc->prxCmd =
            (RXCMD *)((char *)prxHdr->pSharedMem + pMemObj->addrOffset);

        pEsc->pBufEnd = (RXCMD *)((char *)pEsc->prxCmd + prxHdr->sharedMemSize);

        CHECK_SHARED_MEM_RANGE_RETVAL(pEsc->prxCmd, pMemObj);
        CHECK_SHARED_MEM_RANGE_RETVAL(pEsc->pBufEnd, pMemObj);
    }
    else
    {
        // The command buffer is not in shared memory. In this case, the
        // only commands that may be processed are RXCMD_MAP_MEM amd
        // RXCMD_CREATE_CONTEXT.  Check the required flags and command,
        // and process the request.

        // Process the command requested (if valid) and return.

        if ((prxHdr->flags & RX_FL_MAP_MEM) &&
            (pEsc->prxCmd->command == RXCMD_MAP_MEM)) {
            RxMapMem(pEsc);
            return pEsc->retValue;
	} else if ((prxHdr->flags & RX_FL_CREATE_CONTEXT) &&
                   (pEsc->prxCmd->command == RXCMD_CREATE_CONTEXT)) {
            RxCreateContext(pEsc);
            return pEsc->retValue;
        } else {
            RXDBG_PRINT("RxHandler: Invalid request in non-shared memory.");
            return FALSE;
        }
    }

    // Get the rendering context if we have both a rendering context handle
    // and valid window information.

    if (pEsc->pWnd && prxHdr->hrxRC)
    {
        RXRCOBJ *pRcObj;

        pRcObj = (RXRCOBJ *)GetPtrFromHandle(prxHdr->hrxRC, RXHANDLE_RXRC);

        if (!pRcObj)
        {
            RXDBG_PRINT("RxHandler: Invalid rendering context handle %x.",
                        prxHdr->hrxRC);
            return FALSE;
        }

        pRc = (RXRC *)pRcObj->pObject;
        pEsc->pRc = (VOID *)pRc;

        // Validate the window in the RC with the window for this escape:

        if (pRc->hwnd != pEsc->pWnd->hwnd)
        {
            RXDBG_PRINT("RxHandler: Invalid RC for this window.");
            return FALSE;
        }

        // Set up clipping pointer.  This may be changed during a command
        // batch by the state of scissoring.  Note that we always regenerate
        // the scissored list since it may have been overwritten by another
        // rendering context:

        GetScissorClip(pEsc);

        // Initialize the hardware rendering context:

        HW_INIT_STATE(pEsc, pRc);
    }
    else
    {
        // Check for commands which do not require a rendering context.
	// By this point, we've already processed RXCMD_MAP_MEM and
	// RXCMD_CREATE_CONTEXT:

        switch (pEsc->prxCmd->command) {
            case RXCMD_GET_INFO:
            case RXCMD_ALLOC_TEXTURE:
            case RXCMD_DELETE_RESOURCE:
            case RXCMD_QUERY_TEXTURE_MEMORY:
            case RXCMD_TEXTURE_HEAP:
                break;
            default:
                RXDBG_PRINT("RxHandler: Null rendering context invalid for this command.");
                return FALSE;
        }

        // !!! In this scenario, call HW_DEFAULT_STATE without first having
        //     called HW_INIT_STATE
    }


    // This is the main 3D DDI command-batch parser.  Continue to
    // process the commands in the buffer until all commands have
    // been processed, or an error has occurred.
    //
    // Note that we subtract the size of RXCMD to ensure that we have
    // enough room in the buffer to safely read 'command':

    while ((BYTE*) pEsc->prxCmd <= (BYTE*) pEsc->pBufEnd - sizeof(RXCMD))
    {
        ULONG cmd = pEsc->prxCmd->command;

        if (cmd > (sizeof(rxFuncs) / sizeof(&RxNullFunc)))
        {
            RXDBG_PRINT("RxHandler: Command value %d invalid.", cmd);
            pEsc->retValue = FALSE;

            break;          // Make sure we execute HW_DEFAULT_STATE
        }

        (*rxFuncs[cmd])(pEsc);
    }

    HW_DEFAULT_STATE(pEsc);

    return pEsc->retValue;
}

//****************************************************************************
// RxCreateContext()
//
// Create a rendering context (RGBA or color-indexed) for the current
// hardware mode.  This call will also initialize window-tracking for
// the context (which is aassociated with the specified window).
//****************************************************************************

void RxCreateContext(RXESCAPE *pEsc)
{
    WNDOBJ *pwo = pEsc->pwo;
    RXWINDOW *pWnd;
    RXRC *pRc;
    RXCREATECONTEXT rxCreateContext;

    NEXT_COMMAND(rxCreateContext);

    // Make sure the hardware supports the type of context requested:

    HW_CONTEXT_CHECK_MODE(pEsc, rxCreateContext);

    // Initialize tracking of this window with a RXWINDOW
    // (via and WNDOBJ on NT) if we are not already tracking the
    // window:

    if (!pEsc->pWnd)
    {
	pWnd = NewRxWindowTrack(pEsc, &rxCreateContext);

        if (!pWnd)
        {
            RXDBG_PRINT("RxCreateContext: Creation of window object failed.");
            ERROR_RETURN;
        }
    }
    else
        pWnd = pEsc->pWnd;

    pRc = NewRxRc(pEsc);

    if (!pRc) {
        RXDBG_PRINT("RxCreateContext: Could not create new context.");
        ERROR_RETURN;
    }

    if (!(pEsc->retValue = (ULONG)CreateRcObj(pEsc, pWnd,
                                              pRc, RXHANDLE_RXRC,
                                              NULL, sizeof(RXRC))))
    {
        RXDBG_PRINT("RxCreateContext: Could not create new handle.");
        ERROR_RETURN;
    }

    // save some information in RC.

    pRc->flags = rxCreateContext.flags;
    pRc->hwnd = rxCreateContext.hwnd;
    pRc->rxHandle = (RXHANDLE)pEsc->retValue;

    NONBATCHED_RETURN;
}


//****************************************************************************
// RxDeleteResource()
//
// Deletes the resource belonging to a rendering context (which currently
// can be texture resources), or the context itself and all associated
// resources.
//****************************************************************************

void RxDeleteResource(RXESCAPE *pEsc)
{
    RXDELETERESOURCE rxDeleteResource;

    NEXT_COMMAND(rxDeleteResource);

    DestroyRCObj((RXWINDOW *)pEsc->pWnd,
                 rxDeleteResource.hrxResource,
                 pEsc->prxHdr->hrxRC);

    NONBATCHED_RETURN;
}


//****************************************************************************
// RxGetInfo()
//
// Returns requested information on global, surface, or primitive-specific
// capabilities.
//****************************************************************************

void RxGetInfo(RXESCAPE *pEsc)
{
    RXGETINFO rxGetInfo;
    RXGLOBALINFO *prxGlobalInfo;
    RXSURFACEINFO *prxSurfaceInfo;
    RXCAPS *prxCaps;

    NEXT_COMMAND(rxGetInfo);

    // Check the mode request to see if it is compatible with what is
    // suported by the hardware...

    HW_INFO_CHECK_MODE(pEsc, rxGetInfo);

    // Now, return relevant information for the request:

    switch (rxGetInfo.infoType)
    {
        case RX_INFO_GLOBAL_CAPS:
            if (pEsc->cjOut < sizeof(RXGLOBALINFO))
            {
                RXDBG_PRINT("RxGetInfo: Output buffer too small.");
                ERROR_RETURN;
            }

            prxGlobalInfo = (RXGLOBALINFO *)(pEsc->pvOut);

            HW_GLOBAL_INFO(pEsc, prxGlobalInfo);

            break;

        case RX_INFO_SURFACE_CAPS:
            if (pEsc->cjOut < sizeof(RXSURFACEINFO))
            {
                RXDBG_PRINT("RxGetInfo: Output buffer too small.");
                ERROR_RETURN;
            }

            prxSurfaceInfo = (RXSURFACEINFO *)(pEsc->pvOut);

            HW_SURFACE_INFO(pEsc, prxSurfaceInfo);

            break;

        case RX_INFO_SPAN_CAPS:
            if (pEsc->cjOut < sizeof(RXCAPS))
            {
                RXDBG_PRINT("RxGetInfo: Output buffer too small.");
                ERROR_RETURN;
            }

            prxCaps = (RXCAPS *)(pEsc->pvOut);

            HW_SPAN_CAPS(pEsc, prxCaps);

            break;

        case RX_INFO_LINE_CAPS:
            if (pEsc->cjOut < sizeof(RXCAPS))
            {
                RXDBG_PRINT("RxGetInfo: Output buffer too small.");
                ERROR_RETURN;
            }

            prxCaps = (RXCAPS *)(pEsc->pvOut);

            HW_LINE_CAPS(pEsc, prxCaps);

            break;

        case RX_INFO_INTLINE_CAPS:
            if (pEsc->cjOut < sizeof(RXCAPS))
            {
                RXDBG_PRINT("RxGetInfo: Output buffer too small.");
                ERROR_RETURN;
            }

            prxCaps = (RXCAPS *)(pEsc->pvOut);

            HW_INTLINE_CAPS(pEsc, prxCaps);

            break;

        case RX_INFO_TRIANGLE_CAPS:
            if (pEsc->cjOut < sizeof(RXCAPS))
            {
                RXDBG_PRINT("RxGetInfo: Output buffer too small.");
                ERROR_RETURN;
            }

            prxCaps = (RXCAPS *)(pEsc->pvOut);

            HW_TRIANGLE_CAPS(pEsc, prxCaps);

            break;

        case RX_INFO_QUAD_CAPS:
            if (pEsc->cjOut < sizeof(RXCAPS))
            {
                RXDBG_PRINT("RxGetInfo: Output buffer too small.");
                ERROR_RETURN;
            }

            prxCaps = (RXCAPS *)(pEsc->pvOut);

            HW_QUAD_CAPS(pEsc, prxCaps);

            break;

        default:
            RXDBG_PRINT("RxGetInfo: Unknown info request.");
            SET_ERROR_CODE;
            break;
    }

    NONBATCHED_RETURN;
}


//****************************************************************************
// RxNullFunc()
//
// This is a stub function for commands that are not implemented in this
// driver.
//****************************************************************************

void RxNullFunc(RXESCAPE *pEsc)
{
    RXDBG_PRINT("Invalid or unimplemented 3D DDI command.");
    ERROR_RETURN;
}


//****************************************************************************
// RxNullPrimitive()
//
// This is a stub function for primitives that are not implemented in this
// driver.
//****************************************************************************

void RxNullPrimitive(RXESCAPE *pEsc, RXDRAWPRIM *prxDrawPrim)
{
    RXDBG_PRINT("Unimplemented 3D DDI primitive %d.", prxDrawPrim->primType);
    ERROR_RETURN;
}


//****************************************************************************
// RxFlush()
//
// Flush all 3D DDI commands in the command stream.
//****************************************************************************

void RxFlush(RXESCAPE *pEsc)
{
    ADVANCE_AND_CHECK(sizeof(RXCMD));
    HW_FLUSH(pEsc);
}


//****************************************************************************
// RxReadRect()
//
// Read the specified rectangle from a color or z buffer.
//****************************************************************************

void RxReadRect(RXESCAPE *pEsc)
{
    RXREADRECT rxReadRect;
    RXRC *pRc = pEsc->pRc;
    char *pDest;
    RXMEMOBJ *pMemObj;
    LONG rectOffsetMin;
    ULONG rectOffsetMax;
    ULONG sharedOffset;
    LONG sharedPitch;
    RECTL readRect;
    RECTL readRectClip;
    LONG maxDstY;

    NEXT_COMMAND(rxReadRect);

    // Make sure we have buffers enabled for the read request:

    if (rxReadRect.sourceBuffer == RX_READ_RECT_BACK_LEFT)
    {
        if (!pRc->backBufEnabled)
        {
            RXDBG_PRINT("RxReadRect: back buffer not enabled.");
            ERROR_RETURN;
        }
    }
    else if (rxReadRect.sourceBuffer == RX_READ_RECT_Z)
    {
        if (!pRc->zBufEnabled)
        {
            RXDBG_PRINT("RxReadRect: z buffer not enabled.");
            ERROR_RETURN;
        }
    }
    else if (rxReadRect.sourceBuffer != RX_READ_RECT_FRONT_LEFT)
    {
        RXDBG_PRINT("RxReadRect: illegal sourceBuffer value");
        ERROR_RETURN;
    }

    // Offset the source rectangle by the window origin:

    rxReadRect.sourceX += pEsc->pWnd->clientRect.left;
    rxReadRect.sourceY += pEsc->pWnd->clientRect.top;

    // Convert rectangle to read from screen coordinates.
    //
    // Note that by clamping the coordinates, we insure that there
    // will be no overflow, the rectangle will be guaranteed to be well
    // ordered, and the resulting coordinates will always be positive:

    readRect.left = CLAMPCOUNT(rxReadRect.sourceX);
    readRect.right = CLAMPCOUNT(rxReadRect.destRect.width) + readRect.left;
    readRect.top = CLAMPCOUNT(rxReadRect.sourceY);
    readRect.bottom = CLAMPCOUNT(rxReadRect.destRect.height) + readRect.top;

    // Get the shared memory for the destination:

    pMemObj = (RXMEMOBJ *)GetPtrFromHandle(rxReadRect.hrxSharedMem,
                                           RXHANDLE_SHAREMEM);
    if (!pMemObj)
    {
        RXDBG_PRINT("RxReadRect: invalid shared-memory handle.");
        ERROR_RETURN;
    }

    pDest = (char *)rxReadRect.pSharedMem + pMemObj->addrOffset;

    CHECK_SHARED_MEM_RANGE(pDest, pMemObj);

    sharedOffset = CLAMPMEM(pDest - pMemObj->pMem); // Must be unsigned
    sharedPitch = CLAMPCOORD(rxReadRect.sharedPitch); // Can be signed
    maxDstY = readRect.bottom + (rxReadRect.destRect.y - rxReadRect.sourceY);

    if (sharedPitch > 0)
    {
        // Note that rectOffsetMax does its calculation using (maxDstY)
        // to account for the width of the bottom scan:

        rectOffsetMin = 0;
        rectOffsetMax = sharedOffset + (maxDstY) * sharedPitch;
    }
    else
    {
        // Note that rectOffsetMax subtracts sharedPitch to account for
        // the width of the top scan:

        rectOffsetMin = sharedOffset + (maxDstY - 1) * sharedPitch;
        rectOffsetMax = sharedOffset - sharedPitch;
    }

    if ((rectOffsetMin < 0) || (rectOffsetMax > pMemObj->size))
    {
        RXDBG_PRINT("RxReadRect: rectangle larger than shared memory.");
        ERROR_RETURN;
    }

    // Initialize hardware state for reading operation:

    HW_START_READ_RECT(pEsc, pRc, rxReadRect);

    // Clip the read rectangle to the client area of the window:

    RxIntersectRect(&readRectClip, &pEsc->pWnd->clientRect, &readRect);

    // Do the read:

    HW_READ_RECT(pEsc, pRc, &readRectClip, rxReadRect, pDest);

    HW_DONE_READ_RECT(pEsc, pRc);
}


//****************************************************************************
// RxWriteRect()
//
// Write the specified rectangle to the enabled color buffer(s) or z buffer.
//****************************************************************************

void RxWriteRect(RXESCAPE *pEsc)
{
    RXWRITERECT rxWriteRect;
    RXRC *pRc = pEsc->pRc;
    char *pSource;
    RXMEMOBJ *pMemObj;
    LONG rectOffsetMin;
    ULONG rectOffsetMax;
    ULONG sharedOffset;
    LONG sharedPitch;
    ULONG cClip;
    RECTL writeRect;
    RECTL writeRectClip;
    RECTL *pClip;
    LONG maxSrcY;

    NEXT_COMMAND(rxWriteRect);

    // Make sure we have a valid area to write to:

    if (!(cClip = pEsc->pWnd->prxClipScissored->c))
        return;

    if (rxWriteRect.destBuffer == RX_WRITE_RECT_Z)
    {
        if (!pRc->zBufEnabled)
        {
            RXDBG_PRINT("RxWriteRect: z buffer not enabled.");
            ERROR_RETURN;
        }
    }
    else if (rxWriteRect.destBuffer != RX_WRITE_RECT_PIX)
    {
        RXDBG_PRINT("RxWriteRect: illegal destBuffer value");
        ERROR_RETURN;
    }

    // Offset the destination rectangle by the window origin:

    rxWriteRect.destRect.x += pEsc->pWnd->clientRect.left;
    rxWriteRect.destRect.y += pEsc->pWnd->clientRect.top;

    // Convert rectangle to write to screen coordinates:
    //
    // Note that by clamping the coordinates, we insure that there
    // will be no overflow, the rectangle will be guaranteed to be well
    // ordered, and the resulting coordinates will always be positive:

    writeRect.left = CLAMPCOUNT(rxWriteRect.destRect.x);
    writeRect.right = CLAMPCOUNT(rxWriteRect.destRect.width) + writeRect.left;
    writeRect.top = CLAMPCOUNT(rxWriteRect.destRect.y);
    writeRect.bottom = CLAMPCOUNT(rxWriteRect.destRect.height) + writeRect.top;

    // Get the shared memory for the destination:

    pMemObj = (RXMEMOBJ *)GetPtrFromHandle(rxWriteRect.hrxSharedMem,
                                           RXHANDLE_SHAREMEM);
    if (!pMemObj)
    {
        RXDBG_PRINT("RxWriteRect: invalid shared-memory handle.");
        ERROR_RETURN;
    }

    // Point to start of DIB:

    pSource = (char *)rxWriteRect.pSharedMem + pMemObj->addrOffset;

    CHECK_SHARED_MEM_RANGE(pSource, pMemObj);

    sharedOffset = CLAMPMEM(pSource - pMemObj->pMem); // Must be unsigned
    sharedPitch = CLAMPCOORD(rxWriteRect.sharedPitch); // Can be signed
    maxSrcY = writeRect.bottom + (rxWriteRect.sourceY - rxWriteRect.destRect.y);

    if (sharedPitch > 0)
    {
        // Note that rectOffsetMax does its calculation using (maxSrcY)
        // to account for the width of the bottom scan:

        rectOffsetMin = 0;
        rectOffsetMax = sharedOffset + (maxSrcY) * sharedPitch;
    }
    else
    {
        // Note that rectOffsetMax subtracts sharedPitch to account for
        // the width of the top scan:

        rectOffsetMin = sharedOffset + (maxSrcY - 1) * sharedPitch;
        rectOffsetMax = sharedOffset - sharedPitch;
    }

    if ((rectOffsetMin < 0) || (rectOffsetMax > pMemObj->size))
    {
        RXDBG_PRINT("RxWriteRect: rectangle larger than shared memory.");
        ERROR_RETURN;
    }

    // Initialize hardware state for reading operation:

    HW_START_WRITE_RECT(pEsc, pRc, rxWriteRect);

    for (pClip = &pEsc->pWnd->prxClipScissored->arcl[0]; cClip; cClip--, pClip++)
    {
        // Test for trivial cases:

        if (writeRect.bottom <= pClip->top)
            break;

        if ((writeRect.left >= pClip->right) ||
            (writeRect.top >= pClip->bottom) ||
            (writeRect.right <= pClip->left))
            continue;

        // Intersect current clip rect with fill rect

        RxIntersectRect(&writeRectClip, pClip, &writeRect);

        // Do the fill:

        HW_WRITE_RECT(pEsc, pRc, &writeRectClip, rxWriteRect, pSource);
    }

    HW_DONE_WRITE_RECT(pEsc, pRc);
}


//****************************************************************************
// RxFillRect()
//
// This function fills a rectangle in the specified buffer(s) with the
// current fill value(s) specified previously with RXCMD_SET_STATE.
// ROPs, z buffer funtions, and plane masks, etc. are ignored.
//****************************************************************************

void RxFillRect(RXESCAPE *pEsc)
{
    RXFILLRECT rxFillRect;
    RXRC *pRc = pEsc->pRc;
    ULONG cClip;
    RECTL fillRect;
    RECTL fillRectClip;
    RECTL *pClip;

    NEXT_COMMAND(rxFillRect);

    // The only possible flags are RX_FILL_RECT_Z and RX_FILL_RECT_COLOR,
    // and at least one flag has to be set:

    if (((rxFillRect.fillType & ~(RX_FILL_RECT_Z | RX_FILL_RECT_COLOR))) ||
        (!rxFillRect.fillType))
    {
        ERROR_RETURN;
    }

    if ((rxFillRect.fillType & RX_FILL_RECT_Z) && (!pRc->zBufEnabled))
    {
        ERROR_RETURN;
    }

    if (!(cClip = pEsc->pWnd->prxClipScissored->c))
        return;

    // convert fill rectangle to screen coordinates:

    fillRect.left = rxFillRect.fillRect.x + pEsc->pWnd->clientRect.left;
    fillRect.right = fillRect.left + rxFillRect.fillRect.width;
    fillRect.top = rxFillRect.fillRect.y + pEsc->pWnd->clientRect.top;
    fillRect.bottom = fillRect.top + rxFillRect.fillRect.height;

    // Initialize hardware state for filling operation:

    HW_START_FILL_RECT(pEsc, pRc, rxFillRect);

    for (pClip = &pEsc->pWnd->prxClipScissored->arcl[0]; cClip; cClip--, pClip++)
    {
        // Test for trivial cases:

        if (fillRect.bottom <= pClip->top)
            break;

        // Determine trivial rejection for just this rect
        if ((fillRect.left >= pClip->right) ||
            (fillRect.top >= pClip->bottom) ||
            (fillRect.right <= pClip->left))
            continue;

        // Intersect current clip rect with fill rect

        RxIntersectRect(&fillRectClip, pClip, &fillRect);

        // Do the fill:

        HW_FILL_RECT(pEsc, pRc, &fillRectClip);
    }

    HW_DONE_FILL_RECT(pEsc, pRc);
}


//****************************************************************************
// RxSetState()
//
// Set the specified state for the rendering context.  Several of the states
// in this function are unused in this driver, but all possible states have
// been included (with the corresponding hardware-specific macro/function).
//****************************************************************************

void RxSetState(RXESCAPE *pEsc)
{
    RXRC *pRc = pEsc->pRc;
    RXSETSTATE rxSetState;
    RXCOLOR *prxColor;
    RXSTIPPLE *prxStipple;
    RXLINEPAT *prxLinePat;
    RXRECT *prxRect;
    VOID *pState;

    NEXT_COMMAND(rxSetState);

    // pState points to the state:

    pState = (char *)pEsc->prxCmd - sizeof(rxSetState.newState[0]);

    switch (rxSetState.stateToChange)
    {
        case RX_LINE_PATTERN:
            prxLinePat = (RXLINEPAT *)pState;

            pEsc->prxCmd = (RXCMD *)(prxLinePat + 1);

            CHECK_BUFFER_SIZE(pEsc->prxCmd);

            HW_LINE_PATTERN(pEsc, pRc, prxLinePat);
            break;

        case RX_STIPPLE_PATTERN:
            prxStipple = (RXSTIPPLE *)pState;

            pEsc->prxCmd = (RXCMD *)(prxStipple + 1);

            CHECK_BUFFER_SIZE(pEsc->prxCmd);

            HW_STIPPLE(pEsc, pRc, prxStipple);
            break;

        case RX_ROP2:
            HW_ROP2(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_SPAN_TYPE:
            HW_SPAN_TYPE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_ACTIVE_BUFFER:
            HW_ACTIVE_BUFFER(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_PLANE_MASK:
            HW_PLANE_MASK(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_Z_WRITE_ENABLE:
            HW_Z_MASK(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_Z_ENABLE:
            HW_Z_ENABLE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_ALPHA_TEST_ENABLE:
            HW_ALPHA_TEST_ENABLE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_LAST_PIXEL:
            HW_LAST_PIXEL(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_TEX_MAG:
            HW_TEX_MAG(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_TEX_MIN:
            HW_TEX_MIN(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_SRC_BLEND:
            HW_SRC_BLEND(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_DST_BLEND:
            HW_DST_BLEND(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_TEX_MAP_BLEND:
            HW_TEX_MAP_BLEND(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_CULL_MODE:
            HW_CULL_MODE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_SPAN_DIRECTION:
            HW_SPAN_DIRECTION(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_Z_FUNC:
            HW_Z_FUNC(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_ALPHA_REF:
            HW_ALPHA_REF(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_ALPHA_FUNC:
            HW_ALPHA_FUNC(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_DITHER_ENABLE:
            HW_DITHER_ENABLE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_BLEND_ENABLE:
            HW_BLEND_ENABLE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_TEXTURE:
            HW_TEXTURE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_FILL_COLOR:
            prxColor = pState;

            pEsc->prxCmd = (RXCMD *)(prxColor + 1);

            CHECK_BUFFER_SIZE(pEsc->prxCmd);

            HW_FILL_COLOR(pEsc, pRc, prxColor);
            break;

        case RX_FILL_Z:
            HW_FILL_Z(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_SOLID_COLOR:
            prxColor = pState;

            pEsc->prxCmd = (RXCMD *)(prxColor + 1);

            CHECK_BUFFER_SIZE(pEsc->prxCmd);

            HW_SOLID_COLOR(pEsc, pRc, prxColor);
            break;

        case RX_SCISSORS_ENABLE:

            // Generate a new scissored clip list:

            pRc->scissorsEnabled = (BOOL)rxSetState.newState[0];
            GetScissorClip(pEsc);

            // Set up hardware for new scissoring state:

            HW_SCISSORS_ENABLE(pEsc, pRc);

            break;

        case RX_SCISSORS_RECT:
            prxRect = pState;

            pEsc->prxCmd = (RXCMD *)(prxRect + 1);

            CHECK_BUFFER_SIZE(pEsc->prxCmd);

            HW_SCISSORS_RECT(pEsc, pRc, prxRect);

            // Generate a new scissored clip list:

            GetScissorClip(pEsc);

            break;

        case RX_MASK_START:
            HW_MASK_START(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_SHADE_MODE:
            HW_SHADE_MODE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_VERTEX_TYPE:
            HW_VERTEX_TYPE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_SPAN_COLOR_TYPE:
            HW_SPAN_COLOR_TYPE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_VERTEX_COLOR_TYPE:
            HW_VERTEX_COLOR_TYPE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_TEXTURE_PERSPECTIVE:
            HW_TEXTURE_PERSPECTIVE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_TEX_TRANSP_ENABLE:
            HW_TEX_TRANSP_ENABLE(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_TEX_TRANSP_COLOR:
            HW_TEX_TRANSP_COLOR(pEsc, pRc, rxSetState.newState[0]);
            break;

        case RX_DITHER_ORIGIN:
            HW_DITHER_ORIGIN(pEsc, pRc, rxSetState.newState[0]);
            break;

        default:
            RXDBG_PRINT("RxSetState: Unknown state %d.",
                        rxSetState.stateToChange);
            SET_ERROR_CODE;
            break;
    }
}


//****************************************************************************
//  RxSwapBuffers()
//
// This function swaps the front buffer with the back buffer in the window
// to which the specfied rendering context is bound.
//****************************************************************************

void RxSwapBuffers(RXESCAPE *pEsc)
{
    RXRC *pRc = pEsc->pRc;
    RXSWAPBUFFERS rxSwapBuffers;
    ULONG cClip;
    RECTL *pClip;

    // Make sure we are double-buffered

    NEXT_COMMAND(rxSwapBuffers);

    if (!pRc->backBufEnabled)
    {
        RXDBG_PRINT("RxSwapBuffers: Back buffer not enabled.");
        SET_ERROR_CODE;
        return;
    }

    HW_START_SWAP_BUFFERS(pEsc, pRc, rxSwapBuffers);

    // Swap all of the clip rectangles in the backbuffer to the front:

    for (cClip = pEsc->pWnd->prxClipUnscissored->c,
           pClip = &pEsc->pWnd->prxClipUnscissored->arcl[0];
         cClip;
         cClip--, pClip++)
    {
        HW_SWAP_RECT(pEsc, pRc, pClip);
    }

    HW_DONE_SWAP_BUFFERS(pEsc, pRc);
}


//****************************************************************************
// RxDrawPrim()
//
// Draws the point, line, triangle, or quadrilateral primitives specified
// (depending on driver capabilities).
//****************************************************************************

void RxDrawPrim(RXESCAPE *pEsc)
{
    RXDRAWPRIM rxDrawPrim;
    LONG primType;

    NEXT_COMMAND(rxDrawPrim);

    // Make sure the primitive type is in range:

    primType = rxDrawPrim.primType;

    if ((primType < RX_PRIM_LINESTRIP) || (primType > RX_PRIM_INTLINESTRIP))
    {
        RXDBG_PRINT("RxDrawPrim: unknown primitive type %d.", primType);
        ERROR_RETURN;
    }

    // Call the rendering function.  Note that since several functions
    // may be called for each primitive type depending on hardware and
    // the rendering state, the calls go through a table which hangs off
    // the RC:

    (*pEsc->pRc->primFunc[primType])(pEsc, &rxDrawPrim);
}


//****************************************************************************
// RxEnableBuffer()
//
// Enables the buffer(s) specified.  If the device does not have enough
// resources for the request, all buffers controlled by this command
// will be disabled.
//****************************************************************************

void RxEnableBuffers(RXESCAPE *pEsc)
{
    RXRC *pRc = pEsc->pRc;
    RXENABLEBUFFERS rxEnableBuffers;

    NEXT_COMMAND(rxEnableBuffers);

    if (rxEnableBuffers.buffers & ~(RX_ENABLE_Z_BUFFER | RX_ENABLE_BACK_LEFT_BUFFER))
    {
        RXDBG_PRINT("RxEnableBuffers: Illegal buffers value");
        SET_ERROR_CODE;
        return;
    }

    HW_ENABLE_BUFFERS(pEsc, pRc, rxEnableBuffers);

    NONBATCHED_RETURN;
}


//****************************************************************************
// RxMapMem()
//
// Creates a driver handle for shared memory.  Note that this must be
// implemented to allow client/driver communications.
//****************************************************************************

void RxMapMem(RXESCAPE *pEsc)
{
    RXMAPMEM rxMapMem;

    NEXT_COMMAND(rxMapMem);

    switch (rxMapMem.action)
    {
        case RX_CREATE_MEM_MAP:
            pEsc->retValue = (ULONG)CreateMapMemObj(pEsc, &rxMapMem);
            break;
        case RX_DELETE_MEM_MAP:
            pEsc->retValue = (ULONG)DestroyMapMemObj(pEsc->pWnd,
                                                     rxMapMem.hrxSharedMem);
            break;
        default:
            RXDBG_PRINT("RxMapMem: Invalid action requested.");
            SET_ERROR_CODE;
            break;
    }

    NONBATCHED_RETURN;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// The following section contains Windows NT-specific code to implement
// the escape entry point, window-tracking, handle-management, etc.  The
// functions below should be rewritten for other Windows platforms
// such as Windows 95.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#if DBG

ULONG rxLocalMemSize = 0;

HLOCAL RxDbgLocalAlloc(UINT flags, UINT size)
{
    HLOCAL hRet;

    if (hRet = LocalAlloc(0, size))
        rxLocalMemSize += size;

    return hRet;
}


HLOCAL RxDbgLocalFree(HLOCAL hMem)
{
    if (hMem == 0)
        return hMem;

    rxLocalMemSize -= LocalSize(hMem);

    return LocalFree(hMem);
}

VOID RxDebugPrint(char *pMessage, ...)
{
    char buffer[256];
    int len;
    va_list ap;

    va_start(ap, pMessage);

    strcpy(buffer, RX_DBGMSG_PREFIX);
    len = vsprintf(buffer + strlen(RX_DBGMSG_PREFIX),
                   pMessage, ap);

    buffer[strlen(RX_DBGMSG_PREFIX) + len]     = '\n';
    buffer[strlen(RX_DBGMSG_PREFIX) + len + 1] = '\0';

    OutputDebugStringA(buffer);

    va_end(ap);
}

void RxDebugPrintAll(ULONG debugLevel, PCHAR pMessage, ...)
{

}
#endif /* DBG */


//****************************************************************************
// ULONG RxEscape(SURFOBJ *, ULONG, VOID *, ULONG cjOut, VOID *pvOut)
//
// Driver 3d-ddi escape entry point.
//****************************************************************************

ULONG RxEscape(SURFOBJ *pso,
               ULONG cjIn,  VOID *pvIn,
               ULONG cjOut, VOID *pvOut)
{
    RXESCAPE rxEscape;
    RXHDR *prxHdr;
    RXHDR_NTPRIVATE *prxHdrPriv;

    // This is a 3D-DDI function.  Under Windows NT, we've
    // got an RXHDR_NTPRIVATE structure which we may need
    // to use if the escape does not use shared-memory
    // as is the case with RXCMD_CREATE_CONTEXT and RXCMD_MAP_MEM.

    // Package the things we need into the RXESCAPE structure:

    rxEscape.prxHdr = prxHdr = (RXHDR *)pvIn;

    prxHdrPriv = (RXHDR_NTPRIVATE *)(prxHdr + 1);

    rxEscape.pso = pso;
    if (rxEscape.pwo = prxHdrPriv->pwo)
        rxEscape.pWnd = (RXWINDOW *)prxHdrPriv->pwo->pvConsumer;
    else
        rxEscape.pWnd = (RXWINDOW *)NULL;
    rxEscape.ppdev = (PDEV*)pso->dhpdev;
    rxEscape.pvOut = pvOut;
    rxEscape.cjOut = cjOut;

    if (!prxHdr->hrxSharedMem) {

        if (!prxHdrPriv->pBuffer)
            return (ULONG)FALSE;

        if (prxHdrPriv->bufferSize < sizeof(RXCMD))
            return (ULONG)FALSE;

        rxEscape.prxCmd = (RXCMD *)(prxHdrPriv->pBuffer);
        rxEscape.pBufEnd = (RXCMD *)((char *)rxEscape.prxCmd +
                                     prxHdrPriv->bufferSize);
    }

    return RxHandler(&rxEscape);
}


//****************************************************************************
// GetPtrFromHandle()
//
// Converts a driver handle to a pointer.  Note that we lock and unlock
// the object, and do not hold the lock during use of the pointer.  This
// simplifies much of the other logic in the driver, especially in
// early- or error-return cases, and is safe since we as single-threaded
// inside the driver.
//****************************************************************************

VOID *GetPtrFromHandle(HANDLE handle, RXHANDLETYPE type)
{
    RXRCOBJ *pRcObject;
    DRIVEROBJ *pDrvObj;

    pDrvObj = (DRIVEROBJ *)EngLockDriverObj((HDRVOBJ)handle);

    if (!pDrvObj)
    {
        RXDBG_PRINT("GetPtrFromHandle: Couldn't unlock driver object.");
        return (PVOID)NULL;
    }
    else
    {
        pRcObject = (RXRCOBJ *)pDrvObj->pvObj;
        EngUnlockDriverObj((HDRVOBJ)handle);

        if (pRcObject->type != type)
        {
            RXDBG_PRINT("GetPtrFromHandle: Wrong type.");
            return (PVOID)NULL;
        }
        else
            return pRcObject;
    }
}


//****************************************************************************
// FreeMapMemObj()
//
// Engine callback for freeing memory used by shared-memory handles.
//****************************************************************************

BOOL CALLBACK FreeMapMemObj(DRIVEROBJ *pDrvObj)
{
    RXMEMOBJ *pMemObj = (RXMEMOBJ *)pDrvObj->pvObj;

    UnmapViewOfFile(pMemObj->pMem);
    CloseHandle(pMemObj->hMapFile);
    RxLocalFree((HLOCAL)pMemObj);

    return TRUE;
}


//****************************************************************************
// CreateMapMemObj()
//
// Creates a handle the the specified shared memory.
//****************************************************************************

RXHANDLE CreateMapMemObj(RXESCAPE *pEsc, RXMAPMEM *prxMapMem)
{
    HANDLE hCallingProcess;
    HANDLE hMapFile;
    RXHANDLE hrx;
    RXSHAREMEM *pRxShareMem = &prxMapMem->shareMem;
    char *pMem;
    RXMEMOBJ *pMemObj;

    // Obtain a handle to the process which called us:

    hCallingProcess = OpenProcess(
        PROCESS_DUP_HANDLE,                 // Access type
        FALSE,                              // Inheritable
        pRxShareMem->sourceProcessID);      // PID

    if (hCallingProcess)
    {
        // Duplicate the handle to memory for the mapping operation:

        if (DuplicateHandle(hCallingProcess,        // Caller's process handle
                            pRxShareMem->hSource,   // Handle to duplicate
                            GetCurrentProcess(),    // Our process handle
                            &hMapFile,              // Duplicated handle
                            0,                      // Access will be duplicated
                            FALSE,                  // Inheritable
                            DUPLICATE_SAME_ACCESS)) // Use same access we have
        {
            // Map a view of the requested memory:

            pMem = (BYTE*) MapViewOfFile(hMapFile,                  // Mapping file
                                         FILE_MAP_READ | FILE_MAP_WRITE,
                                         0,                         // High Order offset
                                         pRxShareMem->offset,       // Low Order offset
                                         pRxShareMem->size);        // Amount to map

            if (pMem)
            {
                pMemObj = (RXMEMOBJ *) RxLocalAlloc(0,sizeof(RXMEMOBJ));
                if (pMemObj)
                {
                    hrx = EngCreateDriverObj(pMemObj,
                                             FreeMapMemObj,
                                             pEsc->ppdev->hdevEng);
                    if (hrx)
                    {
                        // Set up the memory object:

                        pMemObj->hrxRC             = NULL;
                        pMemObj->type              = RXHANDLE_SHAREMEM;
                        pMemObj->hrxSharedMem      = hrx;
                        pMemObj->pMem              = pMem;
                        pMemObj->pMemMax           = pMem + pRxShareMem->size;
                        pMemObj->clientBaseAddress = (char *)pRxShareMem->clientBaseAddress;
                        pMemObj->size              = pRxShareMem->size;
                        pMemObj->hMapFile          = hMapFile;
                        pMemObj->addrOffset        =
                            (char *)pMem - (char *)pRxShareMem->clientBaseAddress;

                        return(hrx);
                    }

                    RxLocalFree((HLOCAL) pMemObj);
                }

                UnmapViewOfFile(pMem);
            }

            CloseHandle(hMapFile);
        }
    }

    return(0);
}


//****************************************************************************
// DetroyMapMemObj()
//
// Deletes the handle and associated data for the specified shared
// memory handle.
//****************************************************************************

BOOL DestroyMapMemObj(RXWINDOW *pWnd, RXHANDLE handle)
{
    RXMEMOBJ *pMemObject;
    DRIVEROBJ *pDrvObj;

    pDrvObj = (DRIVEROBJ *)EngLockDriverObj((HDRVOBJ)handle);

    if (!pDrvObj)
        return FALSE;

    pMemObject = (RXMEMOBJ *)pDrvObj->pvObj;
    EngUnlockDriverObj((HDRVOBJ)handle);

    if (pMemObject->type != RXHANDLE_SHAREMEM)
        return FALSE;

    return (EngDeleteDriverObj((HDRVOBJ)handle, TRUE, FALSE) != 0);
}


//****************************************************************************
// NewRxRc()
//
// Creates and initializes a new rendering context.
//****************************************************************************

RXRC *NewRxRc(RXESCAPE *pEsc)
{
    RXRC *pRc = (RXRC *)RxLocalAlloc(0,sizeof(RXRC));
    RXCOLOR black = {0, 0, 0, 0};
    RXLINEPAT solid = {1, 0xffff};
    RXRECT nullRect = {0, 0, 0, 0};

    if (!pRc)
        return NULL;

//!!! double-check default values!!!

    pRc->vertexStride = pRc->coordStride = sizeof(RXPOINT);
    pRc->colorStride = sizeof(RXCOLORREF);

    pRc->linePattern = solid;
    pRc->rop2 = R2_COPYPEN;
    pRc->spanType = RX_POINT;
    pRc->activeBuffer = RX_FRONT_LEFT;
    pRc->planeMask = ~((ULONG)0);
    pRc->zMask = TRUE;
    pRc->zEnable = FALSE;
    pRc->alphaTestEnable = FALSE;
    pRc->lastPixel = FALSE;
    pRc->texMag = RX_TEX_NEAREST;
    pRc->texMin = RX_TEX_NEAREST;
    pRc->srcBlend = RX_BLEND_ONE;
    pRc->dstBlend = RX_BLEND_ZERO;
    pRc->texMapBlend = RX_BLEND_ZERO;
    pRc->cullMode = RX_CULL_NONE;
    pRc->spanDirection = RX_SPAN_HORIZONTAL;
    pRc->zFunc = RX_CMP_NEVER;
    pRc->alphaRef = 0;
    pRc->alphaFunc = RX_CMP_NEVER;
    pRc->ditherEnable = FALSE;
    pRc->blendEnable = FALSE;
    pRc->texture = NULL;
    pRc->fillColor = black;
    pRc->fillZ = 0;
    pRc->solidColor = black;
    pRc->scissorsEnabled = FALSE;
    pRc->scissorsRect = nullRect;
    pRc->maskStart = RX_MASK_MSB;
    pRc->shadeMode = RX_SOLID;
    pRc->vertexType = RX_POINT;
    pRc->spanColorType = RX_SPAN_COLOR_RGB;
    pRc->vertexColorType = RX_VERTEX_COLOR_PACKED;
    pRc->texturePerspective = FALSE;
    pRc->texTranspEnable = FALSE;
    pRc->texTranspColor = 0;
    pRc->ditherOrigin = 0;

    pRc->zBufEnabled = FALSE;
    pRc->backBufEnabled = FALSE;

    pRc->primFunc[RX_PRIM_LINESTRIP] = RxNullPrimitive;
    pRc->primFunc[RX_PRIM_TRISTRIP] = RxNullPrimitive;
    pRc->primFunc[RX_PRIM_QUADSTRIP] = RxNullPrimitive;
    pRc->primFunc[RX_PRIM_LINELIST] = RxNullPrimitive;
    pRc->primFunc[RX_PRIM_TRILIST] = RxNullPrimitive;
    pRc->primFunc[RX_PRIM_QUADLIST] = RxNullPrimitive;
    pRc->primFunc[RX_PRIM_INTLINESTRIP] = RxNullPrimitive;

    // Initialize hardware-specific values:

    HW_INIT_DEFAULT_RC(pEsc, pRc);

    return pRc;
}


//****************************************************************************
// FreeRCObj()
//
// Engine callback for freeing the memory used for rendering-context
// handles.
//****************************************************************************

BOOL CALLBACK FreeRCObj(DRIVEROBJ *pDrvObj)
{
    RXRCOBJ *pRcObj = (RXRCOBJ *)pDrvObj->pvObj;
    RXRC *pRC = (RXRC *)pRcObj->pObject;

    RxLocalFree((HLOCAL)pRC);
    RxLocalFree((HLOCAL)pRcObj);

    return TRUE;
}


//****************************************************************************
// CreateRCObj()
//
// Create a handle for the specified object.  Although this function
// currently handles only rendering contexts, it can be expanded to
// accomodate arbitrary objects.
//****************************************************************************

RXHANDLE CreateRcObj(RXESCAPE *pEsc, RXWINDOW *pWnd,
                     PVOID pObj, RXHANDLETYPE newType,
                     RXHANDLE hrxRC, DWORD size)
{
    FREEOBJPROC proc;
    RXRCOBJ *newRcObject;
    HANDLE handle;

    switch (newType)
    {
        case RXHANDLE_RXRC:
            proc = FreeRCObj;
            break;
        default:
            return NULL;
    }

    newRcObject = (RXRCOBJ *)RxLocalAlloc(0,sizeof(RXRCOBJ));
    if (!newRcObject)
        return NULL;

    handle = EngCreateDriverObj(newRcObject, proc, pEsc->ppdev->hdevEng);

    if (handle) {
        newRcObject->handle = handle;
        newRcObject->hrxRC = (newType == RXHANDLE_RXRC ? handle : hrxRC);
        newRcObject->type = newType;
        newRcObject->pObject = pObj;
        newRcObject->size = size;

        // Add the object to the list in the RXWINDOW
        newRcObject->next = pWnd->objectList;
        pWnd->objectList = newRcObject;

    } else {
        RxLocalFree(newRcObject);
        RXDBG_PRINT("CreateRcObj: Could not create handle.");
    }

    return (handle);
}


//****************************************************************************
// DestroyRCObj()
//
// Deletes the handle belonging to the specified rendering context, or
// the rendering context itself.
//****************************************************************************

BOOL DestroyRCObj(RXWINDOW *pWnd, HANDLE handle, RXHANDLE hrxRC)
{
    RXRCOBJ *pRcObject;
    DRIVEROBJ *pDrvObj;

    pDrvObj = (DRIVEROBJ *)EngLockDriverObj((HDRVOBJ)handle);

    if (!pDrvObj)
        return FALSE;

    pRcObject = (RXRCOBJ *)pDrvObj->pvObj;
    EngUnlockDriverObj((HDRVOBJ)handle);

    // Check that the resource belongs to the specified rendering context:

    if (pRcObject->hrxRC != NULL) {
        if (pRcObject->hrxRC != hrxRC)
            return FALSE;
    }

    // Check for deletion of entire rendering context.  If this happens,
    // we must free all associated handles and resources.

    if (pRcObject->type == RXHANDLE_RXRC) {
        return DestroyRC(pWnd, hrxRC);
    } else
        return (EngDeleteDriverObj((HDRVOBJ)handle, TRUE, FALSE) != 0);
}


//****************************************************************************
// DestroyRxWindow()
//
// Destroy the specified RXWINDOW and any associated handles (such rendering
// contexts).
//****************************************************************************

void DestroyRxWindow(RXWINDOW *pWnd)
{
    RXRCOBJ *nextObject;

    // Delete all of the handles associated with the window:

    while (pWnd->objectList)
    {
        nextObject = pWnd->objectList->next;
        EngDeleteDriverObj((HDRVOBJ)pWnd->objectList->handle,
                           TRUE, FALSE);
        pWnd->objectList = nextObject;
    }

    // Free any memory associated with back or z buffers:

    HW_FREE_BUFFERS(pWnd);

    RxLocalFree(pWnd->pAllocatedClipBuffer);

    // Free the RXWINDOW memory area

    RxLocalFree((HLOCAL)pWnd);
}


//****************************************************************************
// DestroyRC()
//
// Destroys the specified rendering context and associated handles.
//****************************************************************************

BOOL DestroyRC(RXWINDOW *pWnd, RXHANDLE hrxRC)
{
    RXRCOBJ *nextObject;
    RXRCOBJ *lastObject;
    RXRCOBJ *curObject;
    BOOL bRet = TRUE;

    // Delete all of the handles associated with the rendering context

    for (lastObject = curObject = pWnd->objectList;
         curObject; curObject = nextObject)
    {
        nextObject = curObject->next;

        if (curObject->hrxRC == hrxRC) {
            if (pWnd->objectList == curObject)
                pWnd->objectList = nextObject;
            else
                lastObject->next = nextObject;

            if (!EngDeleteDriverObj((HDRVOBJ)curObject->handle, TRUE, FALSE))
                bRet = FALSE;
        } else
            lastObject = curObject;
    }

    return bRet;
}


//****************************************************************************
// GetScissorClip()
//
// Generate a new clip list based on the current list of clip rectanges
// for the window, and the specified scissor rectangle.
//****************************************************************************

void GetScissorClip(RXESCAPE *pEsc)
{
    RXRC* pRc;
    RXWINDOW *pWnd;
    ENUMRECTS *prxClipUnscissored;
    ENUMRECTS *prxClipScissored;
    RECTL *pRectUnscissored;
    RECTL *pRectScissored;
    RECTL rectScissor;
    ULONG numUnscissoredRects;
    ULONG numScissoredRects;

    pRc = pEsc->pRc;
    pWnd = pEsc->pWnd;

    if (!pRc->scissorsEnabled)
    {
        // Scissors aren't enabled, so the unscissored and scissored
        // clip lists are identical:

        pWnd->prxClipScissored = pWnd->prxClipUnscissored;
    }
    else
    {
        // The scissored list will go in the second half of our clip
        // buffer:

        prxClipUnscissored
            = pWnd->prxClipUnscissored;

        prxClipScissored
            = (ENUMRECTS*) ((BYTE*) prxClipUnscissored + pWnd->sizeClipBuffer / 2);

        pWnd->prxClipScissored = prxClipScissored;

        // Convert RXRECT scissor to a RECTL in screen coordinates:

        rectScissor.left   = pRc->scissorsRect.x      + pWnd->clientRect.left;
        rectScissor.right  = pRc->scissorsRect.width  + rectScissor.left;
        rectScissor.top    = pRc->scissorsRect.y      + pWnd->clientRect.top;
        rectScissor.bottom = pRc->scissorsRect.height + rectScissor.top;

        pRectUnscissored = &prxClipUnscissored->arcl[0];
        pRectScissored = &prxClipScissored->arcl[0];
        numScissoredRects = 0;

        for (numUnscissoredRects = prxClipUnscissored->c;
             numUnscissoredRects != 0;
             numUnscissoredRects--, pRectUnscissored++)
        {
            // Since our clipping rectangles are ordered from top to
            // bottom, we can early-out if the tops of the remaining
            // rectangles are not in the scissor rectangle

            if (rectScissor.bottom <= pRectUnscissored->top)
                break;

            // Continue without updating new clip list is there is
            // no overlap.

            if ((rectScissor.left  >= pRectUnscissored->right)  ||
                (rectScissor.top   >= pRectUnscissored->bottom) ||
                (rectScissor.right <= pRectUnscissored->left))
               continue;

            // If we reach this point, we must intersect the given rectangle
            // with the scissor.

            RxIntersectRect(pRectScissored, pRectUnscissored, &rectScissor);

            numScissoredRects++;
            pRectScissored++;
        }

        prxClipScissored->c = numScissoredRects;
    }
}


//****************************************************************************
// GetClipLists()
//
// Updates the clip list for the specified window.  Space is also allocated
// the scissored clip list.
//
//****************************************************************************

VOID GetClipLists(WNDOBJ *pwo, RXWINDOW *pWnd)
{
    ENUMRECTS *prxDefault;
    ULONG numClipRects;
    char *pClipBuffer;
    ULONG sizeClipBuffer;

    prxDefault = (ENUMRECTS*) &pWnd->defaultClipBuffer[0];

    if (pwo->coClient.iDComplexity == DC_RECT)
    {
        RxLocalFree(pWnd->pAllocatedClipBuffer);
        pWnd->pAllocatedClipBuffer = NULL;
        pWnd->prxClipUnscissored = prxDefault;
        pWnd->prxClipScissored = prxDefault;
        pWnd->sizeClipBuffer = SIZE_DEFAULT_CLIP_BUFFER;

        if ((pwo->coClient.rclBounds.left >= pwo->coClient.rclBounds.right) ||
            (pwo->coClient.rclBounds.top  >= pwo->coClient.rclBounds.bottom))
        {
            // Full-screen VGA mode is represented by a DC_RECT clip object
            // with an empty bounding rectangle.  We'll denote it by
            // setting the rectangle count to zero:

            prxDefault->c = 0;
        }
        else
        {
            prxDefault->c = 1;
            prxDefault->arcl[0] = pwo->coClient.rclBounds;
        }
    }
    else
    {
        WNDOBJ_cEnumStart(pwo, CT_RECTANGLES, CD_RIGHTDOWN, 0);

        // Note that this is divide-by-2 for the buffer size because we
        // need room for two copies of the rectangle list:

        if (WNDOBJ_bEnum(pwo, SIZE_DEFAULT_CLIP_BUFFER / 2, (ULONG*) prxDefault))
        {
            // Okay, the list of rectangles won't fit in our default buffer.
            // Unfortunately, there is no way to obtain the total count of clip
            // rectangles other than by enumerating them all, as cEnumStart
            // will occasionally give numbers that are far too large (because
            // GDI itself doesn't feel like counting them all).
            //
            // Note that we can use the full default buffer here for this
            // enumeration loop:

            numClipRects = prxDefault->c;
            while (WNDOBJ_bEnum(pwo, SIZE_DEFAULT_CLIP_BUFFER, (ULONG*) prxDefault))
                numClipRects += prxDefault->c;

            // Don't forget that we are given a valid output buffer even
            // when 'bEnum' returns FALSE:

            numClipRects += prxDefault->c;

            pClipBuffer = pWnd->pAllocatedClipBuffer;
            sizeClipBuffer = 2 * (numClipRects * sizeof(RECTL) + sizeof(ULONG));

            if ((pClipBuffer == NULL) || (sizeClipBuffer > pWnd->sizeClipBuffer))
            {
                // Our allocated buffer is too small; we have to free it and
                // allocate a new one.  Take the opportunity to add some
                // growing room to our allocation:

                sizeClipBuffer += 8 * sizeof(RECTL);    // Arbitrary growing room

                RxLocalFree(pClipBuffer);

                pClipBuffer = (char *) RxLocalAlloc(LMEM_FIXED, sizeClipBuffer);

                if (pClipBuffer == NULL)
                {
                    // Oh no: we couldn't allocate enough room for the clip list.
                    // So pretend we have no visible area at all:

                    pWnd->pAllocatedClipBuffer = NULL;
                    pWnd->prxClipUnscissored = prxDefault;
                    pWnd->prxClipScissored = prxDefault;
                    pWnd->sizeClipBuffer = SIZE_DEFAULT_CLIP_BUFFER;
                    prxDefault->c = 0;
                    return;
                }

                pWnd->pAllocatedClipBuffer = pClipBuffer;
                pWnd->prxClipUnscissored = (ENUMRECTS*) pClipBuffer;
                pWnd->prxClipScissored = (ENUMRECTS*) pClipBuffer;
                pWnd->sizeClipBuffer = sizeClipBuffer;
            }

            // Now actually get all the clip rectangles:

            WNDOBJ_cEnumStart(pwo, CT_RECTANGLES, CD_RIGHTDOWN, 0);
            WNDOBJ_bEnum(pwo, sizeClipBuffer, (ULONG*) pClipBuffer);
        }
        else
        {
            // How nice, there are no more clip rectangles, which meant that
            // the entire list fits in our default clip buffer, with room
            // for the scissored version of the list:

            RxLocalFree(pWnd->pAllocatedClipBuffer);
            pWnd->pAllocatedClipBuffer = NULL;
            pWnd->prxClipUnscissored = prxDefault;
            pWnd->prxClipScissored = prxDefault;
            pWnd->sizeClipBuffer = SIZE_DEFAULT_CLIP_BUFFER;
        }
    }

//    ASSERTDD(pWnd->prxClipUnscissored->c * sizeof(RECTL) + sizeof(ULONG) <=
//             pWnd->sizeClipBuffer / 2, "sizeClipBuffer too small");
}


//****************************************************************************
// WndObjChangeProc()
//
// This is the callback function for window-change notification.  We update
// our clip list, and also allow the hardware to respond to the client
// and surface deltas, as well as the client message itself.
//****************************************************************************

VOID CALLBACK WndObjChangeProc(WNDOBJ *pwo, FLONG fl)
{
    if (pwo)
    {
        RXWINDOW *pWnd = (RXWINDOW *)pwo->pvConsumer;

        switch (fl)
        {
            case WOC_RGN_CLIENT:        // Capture the clip list

                GetClipLists(pwo, pWnd);

                pWnd->clientRect = pwo->rclClient;

                HW_TRACK_WNDOBJ(pwo, fl);
                break;

            case WOC_RGN_CLIENT_DELTA:
            case WOC_RGN_SURFACE_DELTA:
                HW_TRACK_WNDOBJ(pwo, fl);
                break;

            case WOC_DELETE:

            // Window is being deleted, so destroy our private window data,
            // and set the consumer field of the WNDOBJ to NULL:

                if (pWnd)
                {
                    DestroyRxWindow(pWnd);
                    WNDOBJ_vSetConsumer(pwo, (PVOID) NULL);
                }
                break;

            default:
                break;
         }
    }
}


//****************************************************************************
// NewRxWindowTrack()
//
// Creates and initializes a new RXWINDOW and initializes tracking of the
// associated window through callback notification.
//****************************************************************************

RXWINDOW *NewRxWindowTrack(RXESCAPE *pEsc, RXCREATECONTEXT *prxCreateContext)
{
    WNDOBJ *pwo;
    RXWINDOW *pWnd;
    ENUMRECTS *prxDefault;

    pWnd= (RXWINDOW *)RxLocalAlloc(0,sizeof(RXWINDOW));
    if (!pWnd)
        return(NULL);

    // Set the consumer field for the WNDOBJ.

    pwo = EngCreateWnd(pEsc->pso,
                       (HWND)(prxCreateContext->hwnd),
                       WndObjChangeProc,
                       (WO_RGN_CLIENT | WO_RGN_CLIENT_DELTA |
                        WO_RGN_SURFACE_DELTA),
                       0);

    if (!pwo || ((LONG)pwo == -1))
    {
        RXDBG_PRINT("Could not create wndobj.");
        RxLocalFree((HLOCAL)pWnd);
        return NULL;
    }

    // Initialize the structure members:

    pWnd->pwo = pwo;
    pWnd->objectList = NULL;
    pWnd->hwnd = 0;
    pWnd->ppdev = pEsc->ppdev;
    pWnd->hwnd = prxCreateContext->hwnd;

    // Initialize the clipping:

    prxDefault = (ENUMRECTS*) &pWnd->defaultClipBuffer[0];
    prxDefault->c = 0;
    pWnd->pAllocatedClipBuffer = NULL;
    pWnd->prxClipUnscissored = prxDefault;
    pWnd->prxClipScissored = prxDefault;
    pWnd->sizeClipBuffer = SIZE_DEFAULT_CLIP_BUFFER;

    // Initialize hardware-specific information in the RXWINDOW:

    HW_INIT_DEFAULT_WNDOBJ(pWnd);

    // Set the consumer field in the WNDOBJ:

    WNDOBJ_vSetConsumer(pwo, (PVOID)pWnd);

    return(pWnd);
}
