/******************************Module*Header*******************************\
* Module Name: dci.c
*
* This module contains the functions required to support DCI.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

DCIRVAL BeginAccess(LPVOID pDCISurf, LPRECT r);
DCIRVAL EndAccess(LPVOID pDCISurf);
void DestroySurface(LPVOID pDCISurf);

/******************************Public*Routine******************************\
* VOID bEnableDCI(ppdev)
*
* Verify that the miniport supports DCI and initialize any data.
*
\**************************************************************************/

BOOL bEnableDCI(PPDEV ppdev)
{
    VIDEO_SHARE_MEMORY shareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION shareMemoryInformation;
    DWORD returnedDataLength;
    BOOL retval;

    //
    // Call the miniport and request a DCI view.  If the call
    // succeeds then the miniport supports DCI.  Don't forget
    // to unshare the view.
    //

    shareMemory.ProcessHandle = 0;
    shareMemory.RequestedVirtualAddress = 0;
    shareMemory.ViewOffset = 0;
    shareMemory.ViewSize   = ROUND_TO_64K(ppdev->cyScreen * ppdev->lNextScan);

    ppdev->bSupportDCI = DeviceIoControl(ppdev->hDriver,
                                         IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                                         &shareMemory,
                                         sizeof(VIDEO_SHARE_MEMORY),
                                         &shareMemoryInformation,
                                         sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                                         &returnedDataLength,
                                         NULL);

    if (ppdev->bSupportDCI)
    {
        DISPDBG((0,"Miniport supports DCI\n"));

        DeviceIoControl(ppdev->hDriver,
                        IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
                        &shareMemory,
                        sizeof(VIDEO_SHARE_MEMORY),
                        NULL,
                        0,
                        &returnedDataLength,
                        NULL);
    }
    else
    {
        DISPDBG((0,"Miniport doesn't support DCI\n"));
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* ULONG DCICreatePrimarySurface(ppdev,cjIn,pvIn,cjOut,pvOut)
*
* Create a DCI surface to provide access to the visible screen.

\**************************************************************************/

ULONG DCICreatePrimarySurface(
    PPDEV ppdev,
    ULONG cjIn,
    PVOID pvIn,
    ULONG cjOut,
    PVOID pvOut)
{
    PDCISURF         pDCISurf;
    LPDCICREATEINPUT lpInput = (LPDCICREATEINPUT)pvIn;
    LPDCISURFACEINFO lpSurfaceInfo;
    LONG retval;
    LONG cBpp = 2;   // this driver is always 2 Bytes per pixel

    DISPDBG((2,"DCICreatePrimarySurface\n"));

    if (lpInput)
    {
        if (cjIn >= sizeof(DCICREATEINPUT))
        {
            pDCISurf = (PDCISURF) LocalAlloc(LMEM_ZEROINIT, sizeof(DCISURF));

            if (pDCISurf)
            {
                LPDCISURFACEINFO lpSurfaceInfo = (LPDCISURFACEINFO) &(pDCISurf->SurfaceInfo);

                //
                // This is to support PRIMARY surface creation
                // We just want to return all the information
                //

                lpSurfaceInfo->dwMask[0]  = ppdev->flRed;
                lpSurfaceInfo->dwMask[1]  = ppdev->flGreen;
                lpSurfaceInfo->dwMask[2]  = ppdev->flBlue;
                lpSurfaceInfo->dwBitCount = (cBpp * 8);

                lpSurfaceInfo->dwWidth  = ppdev->cxScreen;
                lpSurfaceInfo->dwHeight = ppdev->cyScreen;
                lpSurfaceInfo->lStride  = ppdev->lNextScan;

                if (cBpp == 1)
                {
                    lpSurfaceInfo->dwCompression = BI_RGB;
                }
                else
                {
                    lpSurfaceInfo->dwCompression = BI_BITFIELDS;
                }

                retval = DCI_OK;


                DISPDBG((2,"Create Primary Surface pDCISurf %08lx\n", pDCISurf));

                //
                // Save the pointer.
                //

                *((PDCISURF *) pvOut) = pDCISurf;
                lpSurfaceInfo = &(pDCISurf->SurfaceInfo);

                //
                // Store the ppdev in that structure
                //

                pDCISurf->ppdev = ppdev;
                pDCISurf->Offset = 0;
                pDCISurf->Size = ROUND_TO_64K(ppdev->cyScreen *
                                              ppdev->lNextScan);

                //
                // Fill DCISURFACEINFO for return to caller
                //

                lpSurfaceInfo->dwSize         = sizeof(DCISURFACEINFO);
                lpSurfaceInfo->dwDCICaps      = DCI_PRIMARY | DCI_VISIBLE;

                lpSurfaceInfo->BeginAccess    = BeginAccess;
                lpSurfaceInfo->EndAccess      = EndAccess;
                lpSurfaceInfo->DestroySurface = DestroySurface;

                //
                // Memory not yet mapped.
                //

                lpSurfaceInfo->wSelSurface    = 0;
                lpSurfaceInfo->dwOffSurface   = 0;

                retval = DCI_OK;
            }
            else
            {
                retval = DCI_ERR_OUTOFMEMORY;
            }
        }
        else
        {
            retval = DCI_FAIL_GENERIC;
        }
    }
    else
    {
        retval = DCI_FAIL_INVALIDSURFACE;
    }

    return (ULONG) retval;

}

//
//
// Callback routines.
//
//

/******************************Public*Routine******************************\
* DCIRVAL BeginAccess(pDCISurf,rcl)
*
* Map in the screen memory so that the DCI application can access it.

\**************************************************************************/

DCIRVAL
BeginAccess(
    PDCISURF pDCISurf,
    LPRECT rcl
    )
{

    VIDEO_SHARE_MEMORY shareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION shareMemoryInformation;
    DWORD returnedDataLength;

    DISPDBG((2,"BeginAccess with pDCISurf %08lx\n", pDCISurf));

    shareMemory.ProcessHandle = EngGetProcessHandle();
    shareMemory.RequestedVirtualAddress = 0;

    shareMemory.ViewOffset = pDCISurf->Offset;
    shareMemory.ViewSize   = pDCISurf->Size;

    DISPDBG((2, "ProcessId %08lx, ViewOffset %08lx, ViewSize %08lx\n",
             shareMemory.ProcessHandle, shareMemory.ViewOffset,
             shareMemory.ViewSize));

    //
    // Make sure that the banking is non planar.
    //

    pDCISurf->ppdev->pfnBankControl(pDCISurf->ppdev, 0, JustifyTop);

    if (!DeviceIoControl(pDCISurf->ppdev->hDriver,
                         IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                         &shareMemory,
                         sizeof(VIDEO_SHARE_MEMORY),
                         &shareMemoryInformation,
                         sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                         &returnedDataLength,
                         NULL))
    {
        DISPDBG((0, "BeginAccess: failed IOCTL_VIDEO_SHARE_VIDEO_MEMORY\n"));
        return DCI_FAIL_GENERIC;
    }

    DISPDBG((2, "VirtualAddress %08lx, ViewOffset %08lx, ViewSize %08lx\n",
             shareMemoryInformation.VirtualAddress,
             shareMemoryInformation.SharedViewOffset,
             shareMemoryInformation.SharedViewSize));

    pDCISurf->SurfaceInfo.wSelSurface = 0;
    pDCISurf->SurfaceInfo.dwOffSurface =
        (ULONG) shareMemoryInformation.VirtualAddress;

    // We return DCI_STATUS_POINTERCHANGED because in this
    // implementation we remap the DCI pointer every time BeginAccess
    // is called, so their pointer will change every time.

    return DCI_STATUS_POINTERCHANGED;
}


/******************************Public*Routine******************************\
* DCIRVAL EndAccess(pDCISurf)
*
* Unmap the screen memory so that the DCI application can no longer access it.

\**************************************************************************/

DCIRVAL
EndAccess(
    LPVOID pDCISurf
    )
{
    VIDEO_SHARE_MEMORY shareMemory;
    DWORD returnedDataLength;

    DISPDBG((2,"EndAccess with pDCISurf %08lx\n", pDCISurf));

    if (((PDCISURF)pDCISurf)->SurfaceInfo.dwOffSurface != 0)
    {
        //
        // The memory was previously mapped.
        // Lets call the driver to unmap it.
        //

        shareMemory.ProcessHandle = EngGetProcessHandle();
        shareMemory.ViewOffset = 0;
        shareMemory.ViewSize = 0;
        shareMemory.RequestedVirtualAddress =
            (PVOID) ((PDCISURF)pDCISurf)->SurfaceInfo.dwOffSurface;

        if (!DeviceIoControl(((PDCISURF)pDCISurf)->ppdev->hDriver,
                             IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
                             &shareMemory,
                             sizeof(VIDEO_SHARE_MEMORY),
                             NULL,
                             0,
                             &returnedDataLength,
                             NULL))
        {
            RIP("EndAccess failed IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY\n");
        }
        else
        {
            ((PDCISURF)pDCISurf)->SurfaceInfo.dwOffSurface = 0;
        }

        //
        // We don't know what bank is actually mapped in right now,
        // so map in Bank 0 to put us in a known state.
        //

        ((PDCISURF)pDCISurf)->ppdev->pfnBankControl(
            ((PDCISURF)pDCISurf)->ppdev, 0, JustifyTop);
    }
    else
    {
        DISPDBG((2, "EndAccess: No Memory to unmap\n"));
    }

    DISPDBG((2,"EndAccess with pDCISurf %08lx complete\n", pDCISurf));

    return DCI_OK;
}

/******************************Public*Routine******************************\
* VOID DestroySurface(pDCISurf)
*
* Destroy the DCI surface and free up any allocations.

\**************************************************************************/

VOID
DestroySurface(
    PVOID pDCISurf
    )
{
    DISPDBG((2,"DestroySurface with pDCISurf %08lx\n", pDCISurf));

    LocalFree(pDCISurf);

    return;
}




