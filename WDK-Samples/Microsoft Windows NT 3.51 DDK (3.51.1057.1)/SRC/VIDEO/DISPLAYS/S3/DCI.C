/******************************Module*Header*******************************\
* Module Name: dci.c
*
* This module contains the functions required to support DCI.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* DCIRVAL BeginAccess
*
* Map in the screen memory so that the DCI application can access it.

\**************************************************************************/

DCIRVAL BeginAccess(
DCISURF*    pDCISurf,
LPRECT      rcl)
{
    PDEV*                           ppdev;
    VIDEO_SHARE_MEMORY              shareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION  shareMemoryInformation;
    DWORD                           returnedDataLength;

    DISPDBG((2,"BeginAccess with pDCISurf %08lx", pDCISurf));

    ppdev = pDCISurf->ppdev;

    // Wait for any pending accelerator operations to complete before
    // yielding control:

    IO_GP_WAIT(ppdev);

    if (pDCISurf->SurfaceInfo.dwOffSurface != 0)
    {
        // We already mapped in the frame buffer, for those cards that
        // can support simultaneous accelerator and frame buffer access.
        //
        // Assert that this is the case, which only happens with the S3's
        // 'new memory-mapped I/O' scheme:

        ASSERTDD(ppdev->flCaps & CAPS_NEW_MMIO,
            "GDI should gurantee there won't be recursion of BeginAccess");

        return(DCI_OK);
    }
    else
    {
        shareMemory.ProcessHandle           = EngGetProcessHandle();
        shareMemory.RequestedVirtualAddress = 0;
        shareMemory.ViewOffset              = pDCISurf->Offset;
        shareMemory.ViewSize                = pDCISurf->Size;

        if (!(ppdev->flCaps & CAPS_NEW_MMIO))
        {
            // If we're not mapped linearly, we will have to use banking.
            // Banking shares the CRTC register with the S3's pointer, so
            // we have to grab the CRTC section if this is the case:

            ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

            // Turn on the S3's access to the frame buffer.  Note that
            // this will automatically wait for any pending accelerator
            // operations to complete:

            ppdev->pfnBankSelectMode(ppdev->pvBankData, BANK_ON);
        }

        // Now map the frame buffer into the caller's address space:

        // Be careful when mixing VideoPortMapBankedMemory (i.e., vflatd)
        // access with explicit banking in the driver -- the two may get
        // out of sync with respect to what bank they think the hardware
        // is currently configured for.  The easiest way to avoid any
        // problem is to call VideoPortMapBankedMemory/VideoPortUnmapMemory
        // in the miniport for every BeginAccess/EndAccess pair, and to
        // always explicitly reset the bank after the EndAccess.
        // (VideoPortMapBankedMemory will always reset vflatd's current
        // bank.)

        if (!DeviceIoControl(pDCISurf->ppdev->hDriver,
                             IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                             &shareMemory,
                             sizeof(VIDEO_SHARE_MEMORY),
                             &shareMemoryInformation,
                             sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                             &returnedDataLength,
                             NULL))
        {
            DISPDBG((0, "BeginAccess: failed IOCTL_VIDEO_SHARE_VIDEO_MEMORY"));
            return(DCI_FAIL_GENERIC);
        }

        pDCISurf->SurfaceInfo.wSelSurface  = 0;
        pDCISurf->SurfaceInfo.dwOffSurface =
            (ULONG) shareMemoryInformation.VirtualAddress;

        // We return DCI_STATUS_POINTERCHANGED because in this
        // implementation we remap the DCI pointer every time BeginAccess
        // is called, so their pointer will change every time.

        return(DCI_STATUS_POINTERCHANGED);
    }
}

/******************************Public*Routine******************************\
* VOID vUnmap
*
* Unmap the screen memory so that the DCI application can no longer access
* it.

\**************************************************************************/

VOID vUnmap(
DCISURF* pDCISurf)
{
    PDEV*               ppdev;
    VIDEO_SHARE_MEMORY  shareMemory;
    DWORD               returnedDataLength;

    ppdev = pDCISurf->ppdev;

    shareMemory.ProcessHandle           = EngGetProcessHandle();
    shareMemory.ViewOffset              = 0;
    shareMemory.ViewSize                = 0;
    shareMemory.RequestedVirtualAddress =
        (VOID*) pDCISurf->SurfaceInfo.dwOffSurface;

    if (!DeviceIoControl(pDCISurf->ppdev->hDriver,
                         IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
                         &shareMemory,
                         sizeof(VIDEO_SHARE_MEMORY),
                         NULL,
                         0,
                         &returnedDataLength,
                         NULL))
    {
        RIP("EndAccess failed IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY");
    }
    else
    {
        // Be sure to signal to GDI that the surface is no longer mapped:

        pDCISurf->SurfaceInfo.dwOffSurface = 0;

        // Tell the S3 that it should switch back to accelerator mode, if
        // need be:

        if (!(ppdev->flCaps & CAPS_NEW_MMIO))
        {
            ppdev->pfnBankSelectMode(ppdev->pvBankData, BANK_OFF);

            RELEASE_CRTC_CRITICAL_SECTION(ppdev);
        }
    }
}

/******************************Public*Routine******************************\
* DCIRVAL EndAccess
*
* Unmap the screen memory if necessary.
\**************************************************************************/

DCIRVAL EndAccess(
DCISURF* pDCISurf)
{
    PDEV*   ppdev;

    DISPDBG((2,"EndAccess with pDCISurf %08lx", pDCISurf));

    ASSERTDD(pDCISurf->SurfaceInfo.dwOffSurface != 0,
        "GDI should assure us that EndAccess can't be recursive");

    ppdev = pDCISurf->ppdev;

    // On those chips that support simultaneous accelerator and
    // frame buffer access, we optimize performance a bit by
    // unmapping the frame buffer memory only at DestroySurface
    // time.
    //
    // If your video card has no problem supporting simultaneous
    // accelerator and frame buffer access, you're encouraged to
    // do this too, for better performance.

    if (!(ppdev->flCaps & CAPS_NEW_MMIO))
    {
        // Unfortunately, old S3's require that we switch out of
        // accelerator mode before allowing access to the frame buffer.
        // Consequently, we cannot allow the calling application to
        // write to the frame buffer after an EndAccess, so we must
        // 'un-map' the frame buffer memory on every EndAccess call.

        vUnmap(pDCISurf);
    }

    return(DCI_OK);
}

/******************************Public*Routine******************************\
* VOID DestroySurface
*
* Destroy the DCI surface and free up any allocations.

\**************************************************************************/

VOID DestroySurface(
DCISURF* pDCISurf)
{
    DISPDBG((2,"DestroySurface with pDCISurf %08lx", pDCISurf));

    if (pDCISurf->SurfaceInfo.dwOffSurface != 0)
    {
        // On chips that handle simultaneous accelerator and frame
        // buffer access, we optimized a bit by not unmapping the
        // frame buffer on every EndAccess call.  But we finally
        // have to do the unmap now:

        vUnmap(pDCISurf);
    }

    LocalFree(pDCISurf);
}

/******************************Public*Routine******************************\
* ULONG DCICreatePrimarySurface
*
* Create a DCI surface to provide access to the visible screen.

\**************************************************************************/

LONG DCICreatePrimarySurface(
PDEV* ppdev,
ULONG cjIn,
VOID* pvIn,
ULONG cjOut,
VOID* pvOut)
{
    DCISURF*         pDCISurf;
    LPDCICREATEINPUT pInput;
    LPDCISURFACEINFO pSurfaceInfo;
    LONG             lRet;

    DISPDBG((2,"DCICreatePrimarySurface"));

    #if defined(MIPS) || defined(_PPC_)
    {
        // !!! vflatd seems to currently have a bug on Mips and PowerPC:

        return(DCI_FAIL_UNSUPPORTED);
    }
    #endif

    if (!DIRECT_ACCESS(ppdev))
    {
        // We don't support DCI on the Alpha when running in sparse
        // space, because we can't.  We also don't support DCI if doing
        // a dword read would cause the system to crash.

        lRet = DCI_FAIL_UNSUPPORTED;
    }
    else
    {
        pInput = (DCICREATEINPUT*) pvIn;

        if (cjIn >= sizeof(DCICREATEINPUT))
        {
            pDCISurf = (DCISURF*) LocalAlloc(LMEM_ZEROINIT, sizeof(DCISURF));

            if (pDCISurf)
            {
                // Initializate all public information about the primary
                // surface:

                pDCISurf->SurfaceInfo.dwSize         = sizeof(DCISURFACEINFO);
                pDCISurf->SurfaceInfo.dwDCICaps      = DCI_PRIMARY | DCI_VISIBLE;
                pDCISurf->SurfaceInfo.BeginAccess    = BeginAccess;
                pDCISurf->SurfaceInfo.EndAccess      = EndAccess;
                pDCISurf->SurfaceInfo.DestroySurface = DestroySurface;
                pDCISurf->SurfaceInfo.dwMask[0]      = ppdev->flRed;
                pDCISurf->SurfaceInfo.dwMask[1]      = ppdev->flGreen;
                pDCISurf->SurfaceInfo.dwMask[2]      = ppdev->flBlue;
                pDCISurf->SurfaceInfo.dwBitCount     = ppdev->cBitsPerPel;
                pDCISurf->SurfaceInfo.dwWidth        = ppdev->cxScreen;
                pDCISurf->SurfaceInfo.dwHeight       = ppdev->cyScreen;
                pDCISurf->SurfaceInfo.lStride        = ppdev->lDelta;
                pDCISurf->SurfaceInfo.wSelSurface    = 0;
                pDCISurf->SurfaceInfo.dwOffSurface   = 0;

                if (pDCISurf->SurfaceInfo.dwBitCount <= 8)
                {
                    pDCISurf->SurfaceInfo.dwCompression = BI_RGB;
                }
                else
                {
                    pDCISurf->SurfaceInfo.dwCompression = BI_BITFIELDS;
                }

                // Now initialize our private fields that we want associated
                // with the DCI surface:

                pDCISurf->ppdev  = ppdev;
                pDCISurf->Offset = 0;

                // Under NT, all mapping is done with a 64K granularity:

                pDCISurf->Size = ROUND_UP_TO_64K(ppdev->cyScreen * ppdev->lDelta);

                // Return a pointer to the DCISURF to GDI by placing
                // it in the 'pvOut' buffer:

                *((DCISURF**) pvOut) = pDCISurf;

                lRet = DCI_OK;
            }
            else
            {
                lRet = DCI_ERR_OUTOFMEMORY;
            }
        }
        else
        {
            lRet = DCI_FAIL_GENERIC;
        }
    }

    return(lRet);
}
