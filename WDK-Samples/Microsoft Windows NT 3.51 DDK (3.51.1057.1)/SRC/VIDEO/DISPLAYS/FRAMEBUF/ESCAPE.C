/******************************Module*Header*******************************\
* Module Name: escape.c
*
* Handles escapes
*
* Copyright (c) 1994-1995 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

/******************************Public*Routine******************************\
* ULONG APIENTRY DrvEscape(pso,iEsc,cjIn,pvIn,cjOut,pvOut)
*
* Driver escape entry point.  This function should return TRUE for any
* supported escapes in response to QUERYESCSUPPORT, and FALSE for any
* others.  All supported escapes are called from this routine.
*
\**************************************************************************/

ULONG APIENTRY DrvEscape(
SURFOBJ *pso,
ULONG    iEsc,
ULONG    cjIn,
PVOID    pvIn,
ULONG    cjOut,
PVOID    pvOut)

{
    PPDEV ppdev = (PPDEV) pso->dhpdev;
    ULONG retval;

    switch (iEsc)
    {
        case QUERYESCSUPPORT:

            switch (*(DWORD *)pvIn)
            {
                case QUERYESCSUPPORT:
                    return TRUE;

                case DCICOMMAND:
                    return(ppdev->bSupportDCI);

                default:
                    return FALSE;

            }

        case DCICOMMAND:


            if (!ppdev->bSupportDCI)
            {
                //
                // The miniport does not support DCI
                //

                return FALSE;
            }

            if (cjIn < sizeof(DCICMD))
            {
                return  (ULONG) DCI_FAIL_UNSUPPORTED;
            }

            if (((LPDCICREATEINPUT)pvIn)->cmd.dwVersion != 0x100)
            {
                return  (ULONG) DCI_FAIL_UNSUPPORTED;
            }

            switch (((LPDCICREATEINPUT)pvIn)->cmd.dwCommand)
            {
                case DCICREATEPRIMARYSURFACE:

                    retval = DCICreatePrimarySurface(ppdev, cjIn, pvIn, cjOut, pvOut);
                    break;

                default:

                    retval = (ULONG) DCI_FAIL_UNSUPPORTED;
                    break;

            }

            return retval;


        default:

            return FALSE;

    }

    return FALSE;
}
