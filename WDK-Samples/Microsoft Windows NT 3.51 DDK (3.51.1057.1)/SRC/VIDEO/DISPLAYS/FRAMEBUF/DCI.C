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


BOOLEAN gDoFastMap = FALSE;

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
    shareMemory.ViewSize   = ROUND_TO_64K(ppdev->cyScreen * ppdev->lDeltaScreen);

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

    DISPDBG((2,"DCICreatePrimarySurface\n"));

    if (lpInput)
    {
        if (cjIn >= sizeof(DCICREATEINPUT))
        {
            if (lpInput->cmd.dwVersion == DCI_VERSION)
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
                    lpSurfaceInfo->dwBitCount = ppdev->ulBitCount;
            
                    lpSurfaceInfo->dwWidth  = ppdev->cxScreen;
                    lpSurfaceInfo->dwHeight = ppdev->cyScreen;
                    lpSurfaceInfo->lStride  = ppdev->lDeltaScreen;
            
                    if (ppdev->ulBitCount == 8)
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
                    pDCISurf->prevOffscreen = NULL;
                    pDCISurf->nextOffscreen = NULL;
                    pDCISurf->Offset = 0;
                    pDCISurf->Size = ROUND_TO_64K(ppdev->cyScreen *
                                                  ppdev->lDeltaScreen);
                    pDCISurf->mapped = FALSE;

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
                retval = DCI_FAIL_UNSUPPORTEDVERSION;
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

   if (pDCISurf->mapped)
   {
        if (gDoFastMap)
        {
            DISPDBG((2,"BeginAccess with pDCISurf %08lx - premapped !\n", pDCISurf));
        }
        else
        {
            RIP("BeginAccess : recursing memory mapping !");
        }

        return DCI_OK;
    }
    else
    {
        shareMemory.ProcessHandle = EngGetProcessHandle();
        shareMemory.RequestedVirtualAddress = 0;

        shareMemory.ViewOffset = pDCISurf->Offset;
        shareMemory.ViewSize   = pDCISurf->Size;


        if (pDCISurf->SurfaceInfo.dwHeight == 0)
            RIP("pDCISurf->SurfaceInfo.dwHeight == 0");

        if (pDCISurf->Size == 0)
            RIP("pDCISurf->Size == 0");

        if (pDCISurf->SurfaceInfo.lStride == 0)
            RIP("pDCISurf->SurfaceInfo.lStride == 0");


        DISPDBG((2, "ProcessId %08lx, ViewOffset %08lx, ViewSize %08lx\n",
                 shareMemory.ProcessHandle, shareMemory.ViewOffset,
                 shareMemory.ViewSize));

        if (!DeviceIoControl(pDCISurf->ppdev->hDriver,
                             IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                             &shareMemory,
                             sizeof(VIDEO_SHARE_MEMORY),
                             &shareMemoryInformation,
                             sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                             &returnedDataLength,
                             NULL))
        {
            RIP("DISP failed IOCTL_VIDEO_SHARE_VIDEO_MEMORY");
            return DCI_FAIL_GENERIC;
        }

        DISPDBG((2, "VirtualAddress %08lx, ViewOffset %08lx, ViewSize %08lx\n",
                 shareMemoryInformation.VirtualAddress,
                 shareMemoryInformation.SharedViewOffset,
                 shareMemoryInformation.SharedViewSize));


        if (shareMemoryInformation.SharedViewSize != pDCISurf->Size)
            RIP("shareMemoryInformation.SharedViewSize != pDCISurf->Size");


        pDCISurf->SurfaceInfo.wSelSurface = 0;
        pDCISurf->SurfaceInfo.dwOffSurface =
            (ULONG) shareMemoryInformation.VirtualAddress;

        pDCISurf->mapped = TRUE;

        //
        // we only support boards with a H/W cursor in the sample driver, so do
        // nothing.
        //

        return DCI_STATUS_POINTERCHANGED;
    }
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

    //
    // Lets optimize this by never unmapping the surface. !!!
    //

   if (((PDCISURF)pDCISurf)->mapped)
   {
        if (gDoFastMap)
        {
            DISPDBG((2,"EndAccess with pDCISurf %08lx - fast unmap!\n", pDCISurf));
            return DCI_OK;
        }
        else
        {
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
                    RIP("DISP failed IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY");
                }
                else
                {
                    ((PDCISURF)pDCISurf)->SurfaceInfo.dwOffSurface = 0;
                    ((PDCISURF)pDCISurf)->mapped = FALSE;
                }
            }
            else
            {
                DISPDBG((2, "EndAccess: No Memory to unmap\n"));
            }

            DISPDBG((2,"EndAccess with pDCISurf %08lx complete\n", pDCISurf));

            return DCI_STATUS_POINTERCHANGED;
        }
    }
    else
    {
        RIP("EndAccess: unmapping memory that is not mapped !");
        return DCI_OK;
    }
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




