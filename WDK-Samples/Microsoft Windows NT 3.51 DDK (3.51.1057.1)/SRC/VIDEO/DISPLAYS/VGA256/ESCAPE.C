/******************************Module*Header*******************************\
* Module Name: escape.c
*
* Handles escapes
*
* Copyright (c) 1994-1995 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

//****************************************************************************
// ULONG DrvEscape(SURFOBJ *, ULONG, ULONG, VOID *, ULONG cjOut, VOID *pvOut)
//
// Driver escape entry point.  This function should return TRUE for any
// supported escapes in response to QUERYESCSUPPORT, and FALSE for any
// others.  All supported escapes are called from this routine.
//****************************************************************************

ULONG DrvEscape(
SURFOBJ*    pso,
ULONG       iEsc,
ULONG       cjIn,
VOID*       pvIn,
ULONG       cjOut,
VOID*       pvOut)
{
    PDEV*   ppdev;
    ULONG   iQuery;
    LONG    lRet;
    DCICMD* pDciCmd;

    lRet = 0;  // Assume failure, or not suported

    ppdev = (PDEV*) pso->dhpdev;

    switch (iEsc)
    {
    case QUERYESCSUPPORT:
        // Note:  we don't need to check cjIn for this case since
        // NT's GDI validates this for use.

        iQuery = *(ULONG *)pvIn;

        switch (iQuery)
        {
        case DCICOMMAND:
            lRet = ppdev->bSupportDCI;
            break;
        }
        break;

    case DCICOMMAND:
        pDciCmd = (DCICMD*) pvIn;

        if ((cjIn < sizeof(DCICMD)) ||
            (pDciCmd->dwVersion != DCI_VERSION) ||
            (!ppdev->bSupportDCI))
        {
            lRet = DCI_FAIL_UNSUPPORTED;
        }
        else
        {
            switch(pDciCmd->dwCommand)
            {
            case DCICREATEPRIMARYSURFACE:
                lRet = DCICreatePrimarySurface(ppdev, cjIn, pvIn, cjOut, pvOut);
                break;

            default:
                lRet = DCI_FAIL_UNSUPPORTED;
                break;
            }
        }
        break;
    }

    return((ULONG) lRet);
}
