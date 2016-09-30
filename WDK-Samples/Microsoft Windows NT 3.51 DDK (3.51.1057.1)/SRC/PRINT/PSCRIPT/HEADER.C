//--------------------------------------------------------------------------
//
// Module Name:  HEADER.C
//
// Brief Description:  This module contains the PSCRIPT driver's header
// output functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 26-Nov-1990
//
// Copyright (c) 1990-1993 Microsoft Corporation
//
// This routine contains routines to output the PostScript driver's header.
//--------------------------------------------------------------------------

#include "pscript.h"
#include "header.h"
#include "resource.h"
#include "enable.h"


extern HMODULE     ghmodDrv;    // GLOBAL MODULE HANDLE.
extern int keycmp(CHAR *, CHAR *);
extern keycpyn(char *s, char *t, int n, BOOL dotranslate);

BOOL bSendDeviceSetup(PDEVDATA);
VOID DownloadNTProcSet(PDEVDATA, BOOL);
VOID SetFormAndTray(PDEVDATA);
extern BOOL bPageIndependence(PDEVDATA);

#if DBG
#define FT_TEST     0
#else
#define FT_TEST     0
#endif

void PrintBeginFeature(PDEVDATA pdev, char * feature)
{
    PrintString(pdev, "mark {\n%%BeginFeature: *");
    PrintString(pdev, feature);
}


void PrintEndFeature(PDEVDATA pdev)
{
    PrintString(pdev, "\n%%EndFeature\n} stopped cleartomark\n");
}

void SetLandscape(PDEVDATA pdev, BOOL bMinus90, LONG px, LONG py)
{
    /* per Adobe 4.0 ppd specs. px, py are in PS default coordinates space */
    if (bMinus90) {
        PrintString(pdev, "-90 rotate ");
        PrintDecimal(pdev, 1, -py);
        PrintString(pdev, " 0 translate\n");
    } else {
        PrintString(pdev, "90 rotate 0");
        PrintDecimal(pdev, 1, -px);
        PrintString(pdev, " translate\n");
    }
}



VOID
SetResolution(
    PDEVDATA    pdev,
    DWORD       res,
    BOOL        bPJL)

/*++

Routine Description:

    Generate commands to set printer resolution.


Arguments:

    pdev    pointer to device data
    res     desired resolution
    bPJL    TRUE this is called at the beginning of a job
            FALSE if this called during the setup section


Return Value:

    NONE


History:

    04/01/95 -davidx-
        Wrote it.

--*/


{
    PSRESOLUTION   *pRes;
    int             index;
    PNTPD           pntpd = pdev->pntpd;

    // Find out the number of resolutions the printer supports

    index = pntpd->cResolutions;
    if (index-- <= 0)
        return;

    // Check if the desired resolution is supported

    pRes = (PSRESOLUTION*) ((CHAR*) pntpd + pntpd->loResolution);
    while (pRes[index].iValue != res) {

        // Ignore if the desired resolution is not found

        if (--index < 0) {
            DBGPS(DBGPS_LEVEL_ERROR,
                ("The specified resolution is not supported: %d.\n", res));
            return;
        }
    }

    if (bPJL) {

        // Set resolution using PCL commands
        // Check if the printer uses this trash

        if (pntpd->bJCLResolution) {

            if (! (pntpd->flFlags & PJL_PROTOCOL)) {

                DBGPS(DBGPS_LEVEL_ERROR,
                    ("Cannot set resolution using PJL commands.\n"));
            } else {

                PrintString(pdev, (CHAR*) pntpd + pRes[index].loInvocation);
            }
        }

    } else if (! pntpd->bJCLResolution) {

        // Set resolution using normal PS code
        // Do nothing if the printer uses PJL to set resoltion.

        PrintBeginFeature(pdev, "Resolution ");                
        PrintDecimal(pdev, 1, res);
        PrintString(pdev, "\n");
        PrintString(pdev, (CHAR*) pntpd + pRes[index].loInvocation);
        PrintEndFeature(pdev);
    }
}


//--------------------------------------------------------------------------
// BOOL bOutputHeader(pdev)
// PDEVDATA    pdev;
//
// This routine sends the driver's header to the output channel.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
// Returns:
//   This function returns TRUE if the header was successfully sent,
//   FALSE otherwise.
//
// History:
//  04/01/95 -davidx-
//      Set resolution using either PJL commands before the job
//      or using regular PS commands in the setup section.
//
//   26-Nov-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------
BOOL  bOutputHeader(PDEVDATA pdev)
{
    CHAR            buf[128];
    CHAR           *pstr;
    WCHAR          *pwstr;
    DWORD           cTmp, i;
    SYSTEMTIME      systime;
    PNTPD           pntpd;

    // get a local pointer.

    pntpd = pdev->pntpd;

    if (!bPageIndependence(pdev)) {

        if (pntpd->flFlags & PJL_PROTOCOL) {

            // Universal exit language

            PrintString(pdev, "\033%-12345X");

            // If the printer uses PJL commands to set resolution,
            // then do it before the job.

            SetResolution(pdev, (DWORD) pdev->psdm.dm.dmPrintQuality, TRUE);

            // if the printer supports job switching, put the printer into
            // postscript mode now.

            PrintString(pdev, "@PJL ENTER LANGUAGE=POSTSCRIPT\n");
        }

        if (pntpd->flFlags & SIC_PROTOCOL) {
            // call directly to bPSWrite to output the necessary escape commands.
            // PrintString will NOT output '\000'.

            bPSWrite(pdev, "\033\133\113\030\000\006\061\010\000\000\000\000\000", 13);
            bPSWrite(pdev, "\000\000\000\000\000\000\000\000\004\033\133\113\003", 13);
            bPSWrite(pdev, "\000\006\061\010\004", 5);
        }
    }

    //!!! output something different if Encapsulated PS.
    //!!! PrintString(pdev, "%!PS-Adobe-2.0 EPSF-2.0\n");

    PrintString(pdev, "%!PS-Adobe-3.0\n");

    // output the title of the document.

    if (pdev->pwstrDocName)
    {
//!!! need to output UNICODE document name???  -kentse.
//!!! for now just lop off top word.
        pstr = buf;
        pwstr = pdev->pwstrDocName;

        cTmp = min((sizeof(buf) - 1), wcslen(pwstr));

        while (cTmp--)
            *pstr++ = (CHAR)*pwstr++;

        // NULL terminate the document name.

        *pstr = '\0';

        PrintString(pdev, "%%Title: ");
        PrintString(pdev, buf);
        PrintString(pdev, "\n");
    } else
        PrintString(pdev, "%%Title: Untitled Document\n");

    // let the world know who we are.

    PrintString(pdev, "%%Creator: Windows NT 3.5\n");

    // print the date and time of creation.

    GetLocalTime(&systime);

    PrintString(pdev, "%%CreationDate: ");
    PrintDecimal(pdev, 1, systime.wHour);
    PrintString(pdev, ":");
    PrintDecimal(pdev, 1, systime.wMinute);
    PrintString(pdev, " ");
    PrintDecimal(pdev, 1, systime.wMonth);
    PrintString(pdev, "/");
    PrintDecimal(pdev, 1, systime.wDay);
    PrintString(pdev, "/");
    PrintDecimal(pdev, 1, systime.wYear);
    PrintString(pdev, "\n");

    if (!(pdev->dwFlags & PDEV_EPSPRINTING_ESCAPE))
        PrintString(pdev, "%%Pages: (atend)\n");

    // mark the bounding box of the document.

    PrintString(pdev, "%%BoundingBox: ");
    PrintDecimal(pdev, 4, pdev->CurForm.imagearea.left,
                 pdev->CurForm.imagearea.bottom,
                 pdev->CurForm.imagearea.right,
         pdev->CurForm.imagearea.top);
    PrintString(pdev, "\n");


    if (pdev->cCopies > 1)
    {
        PrintString(pdev, "%%Requirements: numcopies(");
        PrintDecimal(pdev, 1, pdev->cCopies);
        PrintString(pdev, ") collate\n");
    }

    // we are done with the comments portion of the document.

    PrintString(pdev, "%%EndComments\n");

    // define our procedure set.

    if (!(pdev->dwFlags & PDEV_EPSPRINTING_ESCAPE)) {
        DownloadNTProcSet(pdev, TRUE);
        PrintString(pdev, "%%EndProlog\n");
    }

    // do the device setup.

    PrintString(pdev, "%%BeginSetup\n");

    // If the printer uses regular PS commands to set
    // resolution, then do it here during setup section.

    SetResolution(pdev, (DWORD) pdev->psdm.dm.dmPrintQuality, FALSE);

    // send form and tray selection commands.

    SetFormAndTray(pdev);

    // handle duplex if necessary.

    if (pntpd->loszDuplexNone || pntpd->loszDuplexNoTumble || pntpd->loszDuplexTumble) {

        if (pdev->psdm.dm.dmDuplex == DMDUP_HORIZONTAL) {
            /* Horizontal == ShortEdge == Tumble */
            PrintBeginFeature(pdev, "Duplex DuplexTumble\n");
            if (pntpd->loszDuplexTumble) pstr = (char *)pntpd + pntpd->loszDuplexTumble;
        } else if (pdev->psdm.dm.dmDuplex == DMDUP_VERTICAL) {
            /* Vertical == LongEdge == NoTumble */
            PrintBeginFeature(pdev, "Duplex DuplexNoTumble\n");
            if (pntpd->loszDuplexNoTumble) pstr = (char *)pntpd + pntpd->loszDuplexNoTumble;
        } else {
            // turn duplex off.
            PrintBeginFeature(pdev, "Duplex None\n");
            if (pntpd->loszDuplexNone) pstr = (char *)pntpd + pntpd->loszDuplexNone;
        }
        PrintString(pdev, pstr);
        PrintEndFeature(pdev);
    }

    // handle collation if the device supports it.

    if (pntpd->loszCollateOn && pntpd->loszCollateOff)
    {
        if (pdev->psdm.dm.dmCollate == DMCOLLATE_TRUE)
        {
            PrintString(pdev, "%%BeginFeature: *Collate True\n");
            pstr = (char *)pntpd + pntpd->loszCollateOn;
        }
        else
        {
            PrintString(pdev, "%%BeginFeature: *Collate False\n");
            pstr = (char *)pntpd + pntpd->loszCollateOff;
        }

        PrintString(pdev, pstr);
        PrintString(pdev, "\n%%EndFeature\n");
    }

    /* Set number of copies */
    PrintString(pdev, "/#copies ");
    PrintDecimal(pdev, 1, pdev->cCopies);
    PrintString(pdev, " def\n");

    /* The implemention of EPSPRINTING escape here just follows Win31 */
    if (pdev->dwFlags & PDEV_EPSPRINTING_ESCAPE
        && pdev->psdm.dm.dmOrientation == DMORIENT_LANDSCAPE)
        SetLandscape(pdev, TRUE, pdev->CurForm.sizlPaper.cy,
                                 pdev->CurForm.sizlPaper.cx);

    PrintString(pdev, "%%EndSetup\n");

    // the form / tray information has already been sent for the first page.

    pdev->dwFlags &= ~PDEV_RESETPDEV;

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL bSendDeviceSetup(pdev)
// PDEVDATA    pdev;
//
// This routine sends the driver's device setup section of the header
// to the output channel.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
// Returns:
//   This function returns TRUE if the header was successfully sent,
//   FALSE otherwise.
//
// History:
//   26-Nov-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL bSendDeviceSetup(pdev)
PDEVDATA    pdev;
{
    PSTR    pstr;
    PNTPD   pntpd;

    pntpd = pdev->pntpd;

    // send the proper normalized transfer function, if one exists.
    // send the inverted transfer function if the PSDEVMODE_NEG
    // flag is set.

    if (pdev->psdm.dwFlags & PSDEVMODE_NEG)
    {
        // if an inverse normalized transfer function is defined for
        // this device, send it to the printer, else send the default
        // inverse transfer function.

        if (pntpd->loszInvTransferNorm)
        {
            pstr = (char *)pntpd + pntpd->loszInvTransferNorm;
            PrintString(pdev, pstr);
        }
        else // default inverse transfer function.
            PrintString(pdev, "{1 exch sub}");

        PrintString(pdev, " settransfer\n");

        /* Erase page in preparation for negative print */
        PrintString(pdev, "gsave clippath 1 setgray fill grestore\n");
    }
    else
    {
        // send the normalized transfer function to the printer if
        // one exists for this printer.

        if (pntpd->loszTransferNorm)
        {
            pstr = (char *)pntpd + pntpd->loszTransferNorm;
            PrintString(pdev, pstr);
            PrintString(pdev, " settransfer\n");
        }
    }

    /* Hardwired to Minus 90 for now  pingw 22APR94 */
    if (pdev->psdm.dm.dmOrientation == DMORIENT_LANDSCAPE)
        SetLandscape(pdev, TRUE, pdev->CurForm.sizlPaper.cy,
                                 pdev->CurForm.sizlPaper.cx);

    if (pdev->psdm.dwFlags & PSDEVMODE_MIRROR) {
        PrintDecimal(pdev, 2, pdev->CurForm.imagearea.right - pdev->CurForm.imagearea.left, 0);
        PrintString(pdev, " translate -1 1 scale\n");

    }

    /* Translate origin to upper left corner a la GDI */
    PrintDecimal(pdev, 2, pdev->CurForm.imagearea.left, pdev->CurForm.imagearea.top);
    PrintString(pdev, " translate ");
    
    /* Flip y-axis to point downwards and scale from points to dpi */
    PrintDecimal(pdev, 2, 72, pdev->psdm.dm.dmPrintQuality);
    PrintString(pdev, " div dup neg scale\n");

    /* Snap to pixel */
    PrintString(pdev, "0 0 transform .25 add round .25 sub exch .25 add round .25 sub exch itransform translate\n");

    return TRUE;
}

//--------------------------------------------------------------------
// BOOL bSendPSProcSet(pdev, ulPSid)
// PDEVDATA        pdev;
// ULONG           ulPSid;
//
// Routine Description:
//
// This routine will output the PS Procset Resource referenced by ulPSid
// to the PS Interpreter. See PSPROC.H for valid ids.
//
// Return Value:
//
//  FALSE if an error occurred.
//
// Author:
//
//  15-Feb-1993 created  -by-  Rob Kiesler
//
//
// Revision History:
//--------------------------------------------------------------------

BOOL bSendPSProcSet(pdev, ulPSid)
PDEVDATA    pdev;
ULONG       ulPSid;
{
    HANDLE  hRes;
    USHORT  usSize;
    HANDLE  hProcRes;
    PSZ     pntps;

    if (pdev->dwFlags & PDEV_CANCELDOC)
        return(TRUE);

    if (!(hRes = FindResource(ghmodDrv, MAKEINTRESOURCE(ulPSid),
                  MAKEINTRESOURCE(PSPROC))))
    {
        RIP("PSCRIPT!bSendPSProcSet: Couldn't find proc set resource\n");
        return(FALSE);
    }

    usSize = (USHORT)SizeofResource(ghmodDrv, hRes);

    //
    // Get the handle to the resource.
    //
    if (!(hProcRes = LoadResource(ghmodDrv, hRes)))
    {
        RIP("PSCRIPT!bSendPSProcSet: LoadResource failed.\n");
        return(FALSE);
    }

    //
    // Get a pointer to the resource data.
    //
    if (!(pntps = (PSZ) LockResource(hProcRes)))
    {
        RIP("PSCRIPT!bSendPSProcSet: LockResource failed.\n");
        FreeResource(hProcRes);
        return(FALSE);
    }
    if (!bPSWrite(pdev, pntps, usSize))
    {
        RIP("PSCRIPT!bSendPSProcSet: Output of Header failed.\n");
        FreeResource(hProcRes);
        return(FALSE);
    }

    FreeResource(hProcRes);
    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID DownloadNTProcSet(pdev, bEhandler)
// PDEVDATA    pdev;
// BOOL        bEhandler;
//
// This routine sends the driver's ProcSet to the output channel.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   bEhandler:
//     TRUE if we should even consider sending the error handler,
//     otherwise FALSE.
//
// Returns:
//   This function returns no value.
//
// History:
//   11-May-1993     -by-     Kent Settle     (kentse)
//  Broke into a separate routine.
//--------------------------------------------------------------------------

VOID DownloadNTProcSet(pdev, bEhandler)
PDEVDATA    pdev;
BOOL        bEhandler;
{
    PSZ            *ppsz;

    // download our error handler if we are told to.
    if (bEhandler)
    {
#if 0
        if (pdev->psdm.dwFlags & PSDEVMODE_EHANDLER)
        {
            ppsz = apszEHandler;
            while (*ppsz)
            {
                PrintString(pdev, (PSZ)*ppsz++);
                PrintString(pdev, "\n");
            }
        }
#endif
    }

    PrintString(pdev, "%%BeginProcSet: " PROCSETNAME "\n");
    PrintString(pdev, "% Copyright (c) 1991 - 1994 Microsoft Corporation\n");
    PrintString(pdev, "/" PROCSETNAME " 100 dict dup begin\n");

    // download our procedure definitions code.

    ppsz = apszHeader;
    while (*ppsz) {
        PrintString(pdev, (PSZ)*ppsz++);
        PrintString(pdev, "\n");
    }

    PrintString(pdev, " end def\n");
    PrintString(pdev, "%%EndProcSet\n");

}





VOID
SetFormAndTray(
    PDEVDATA    pPDev
    )

/*++

Routine Description:

    This function pick the tray for a particular form selected, depends on
    the user request in the UI


Arguments:

    PDev    - Pointer to our PDEVDATA


Return Value:

    VOID


Author:

    20-Dec-1994 Tue 09:38:07 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PNTPD       pNTPD;
    PSINPUTSLOT *pSlot;
    PSFORM      *pPSForm;
    WCHAR       FormName[CCHFORMNAME];
    LONG        SlotNum;
    DWORD       i;
    BOOL        IsManual;
    BOOL        bRegion;


    pNTPD = pPDev->pntpd;

    //
    // Assume the form is not in manual feed first
    //

    if (pPDev->dwFlags & PDEV_MANUALFEED) {

        PrintBeginFeature(pPDev, "ManualFeed False\n");
        PrintString(pPDev, (CHAR *)pNTPD + pNTPD->loszManualFeedFALSE);
        PrintEndFeature(pPDev);
        pPDev->dwFlags &= ~PDEV_MANUALFEED;
    }

#if FT_TEST
    DbgPrint("PS: dmDefaultSource = %ld\n", pPDev->psdm.dm.dmDefaultSource);
#endif

    //
    // We need to check:
    //
    //  1. If user specified Input Slot then use it even default is set diff
    //  2. else check FORM selected in the UI, if default specified then use it
    //  3. else only use page size to have device determine which one ot use
    //

    IsManual = FALSE;
    SlotNum  = -1;

    if ((pPDev->psdm.dm.dmFields & DM_DEFAULTSOURCE)    &&
        (pPDev->psdm.dm.dmDefaultSource != DMBIN_FORMSOURCE)) {

        i = (DWORD)pPDev->psdm.dm.dmDefaultSource;

        //
        // The input slot is specified, check see if valid
        //

        if ((i == DMBIN_MANUAL) ||
            (i == DMBIN_ENVMANUAL)) {

            //
            // User wnat to pull it from manual feed slot
            //
#if FT_TEST
            DbgPrint("PS: User Select Manual Feeder\n");
#endif
            IsManual = TRUE;

        } else if ((pNTPD->cInputSlots > 1) &&
                   (i >= (DWORD)DMBIN_USER)  &&
                   (i <  (DWORD)(DMBIN_USER + pNTPD->cInputSlots))) {

            //
            // We have user specified the slot number, use it
            //

            SlotNum = (LONG)(i - DMBIN_USER);
#if FT_TEST
            DbgPrint("PS: User Select SlotNumber=%ld\n", SlotNum);
#endif
        }

    } else if (pPDev->pTrayFormTable) {

        //
        // User did not specified the slot number or it specified to use
        // PrintMan's FORM slection, so check it out if we have TrayFormTable
        //

        WCHAR   *pwStr = pPDev->pTrayFormTable;
        WCHAR   *pFormName;
        WCHAR   *pSlotName;
        WCHAR   *pPrinterForm;
        WCHAR   SlotName[MAX_SLOT_NAME];
        WORD    IsDefTray;


        //
        // Get unicode version of the form name. and get the manual tray name
        //

        MultiByteToWideChar(CP_ACP,
                            0,
                            pPDev->CurForm.FormName,
                            -1,
                            FormName,
                            sizeof(FormName) / sizeof(FormName[0]));

        //
        // Get the manual feed name
        //

        LoadString(ghmodDrv,
               SLOT_MANUAL,
               SlotName,
               (sizeof(SlotName) / sizeof(SlotName[0])));

#if FT_TEST
        DbgPrint("PS: CurFormName=%ws\n", FormName);
        DbgPrint("PS: ManualFeedName=%ws\n", SlotName);
#endif

        while (*pwStr) {

            pSlotName    = pwStr;
            pFormName    = pSlotName + (wcslen(pSlotName) + 1);
            pPrinterForm = pFormName + (wcslen(pFormName) + 1);
            pwStr        = pPrinterForm + (wcslen(pPrinterForm) + 1);
            IsDefTray    = (WORD)*pwStr++;

            //
            // If we have more than one tray have same form name then the
            // IsDefTray may not be set, but if we ONLY have one tray for the
            // form name then IsDefTray will always SET
            //

#if FT_TEST
            DbgPrint("PS: pSlotName=%ws\n", pSlotName);
            DbgPrint("PS: pFormName=%ws\n", pFormName);
            DbgPrint("PS: pPrinterForm=%ws\n", pPrinterForm);
            DbgPrint("PS: IsDefTray=%ld", (DWORD)IsDefTray);
#endif
            if (!wcsncmp(pFormName, FormName, CCHFORMNAME)) {
#if FT_TEST
                DbgPrint(" --> MATCH=");
#endif
                if (IsDefTray) {
#if FT_TEST
                    DbgPrint(" (DefTray)");
#endif
                    if (IsManual = !(BOOL)wcscmp(pSlotName, SlotName)) {
#if FT_TEST
                        DbgPrint(" (MANUAL FEED)\n\n");
#endif
                    } else if (pNTPD->cInputSlots > 1) {

                        //
                        // Now figure out this tray slot number
                        //

                        CHAR    PPDSlotName[MAX_SLOT_NAME];


                        WideCharToMultiByte(CP_ACP,
                                            0,
                                            pSlotName,
                                            -1,
                                            (CHAR *)SlotName,
                                            MAX_SLOT_NAME,
                                            NULL,
                                            NULL);
                        //
                        // Try matching up form's tray with a PPD entry
                        //

                        pSlot = (PSINPUTSLOT *)((CHAR *)pNTPD +
                                pNTPD->loPSInputSlots);

                        for (i = 0; i < pNTPD->cInputSlots; i++, pSlot++) {

                            //
                            // Copy the PPD translation string for comparison
                            //

                            keycpyn(PPDSlotName,
                                    (CHAR *)pNTPD + pSlot->loSlotName,
                                    MAX_SLOT_NAME,
                                    TRUE);

                            if (!strncmp((CHAR *)SlotName,
                                         PPDSlotName,
                                         MAX_SLOT_NAME)) {

                                SlotNum = (LONG)i;
#if FT_TEST
                                DbgPrint(" ** SlotNum=%ld **\n\n", SlotNum);
#endif
                                break;
                            }
                        }
#if FT_TEST
                        if (SlotNum < 0) {

                            DbgPrint(" CANNOT FOUND SlotName=%ws\n\n",SlotName);
                        }
#endif
                    }

                    break;
                }
            }
#if FT_TEST
            DbgPrint("\n\n");
#endif
        }
    }

#if FT_TEST
    DbgPrint("PS: SlotNum=%ld, IsManual=%ld\n", SlotNum, IsManual);
#endif

    //
    // If we know the slot number then output postscript proc to slect it
    //

    if (SlotNum >= 0) {

        pSlot = (PSINPUTSLOT *)((CHAR *)pNTPD + pNTPD->loPSInputSlots) +
                SlotNum;

        PrintBeginFeature(pPDev, "InputSlot ");
        PrintString(pPDev, (CHAR *)pNTPD + pSlot->loSlotName);
        PrintString(pPDev, "\n");
        PrintString(pPDev, (CHAR *)pNTPD + pSlot->loSlotInvo);
        PrintEndFeature(pPDev);

#if FT_TEST
        DbgPrint("PS: InputSlot: %s\n", (CHAR *)pNTPD + pSlot->loSlotName);
        DbgPrint("PS: SlotInvo: %s\n", (CHAR *)pNTPD + pSlot->loSlotInvo);
#endif
    }

    //
    // If this is the manual feed then output postscript proc
    //

    if ((IsManual) && (pNTPD->loszManualFeedTRUE != 0)) {

        //
        // the requested form is in the manual feed slot,
        //

        PrintBeginFeature(pPDev, "ManualFeed True\n");
        PrintString(pPDev, (CHAR *)pNTPD + pNTPD->loszManualFeedTRUE);
        PrintEndFeature(pPDev);
        pPDev->dwFlags |= PDEV_MANUALFEED;

#if FT_TEST
        DbgPrint("PS: ManualFeed TRUE: %s\n",
                                (CHAR *)pNTPD + pNTPD->loszManualFeedTRUE);
#endif
    }

    //
    // select the page region if we are also selecting from multiple
    // paper trays or manual feed, otherwise select page size.
    //

    bRegion = (BOOL)((pNTPD->cPageRegions > 0) &&
                     ((pPDev->dwFlags & PDEV_MANUALFEED) ||
                      (SlotNum >= 0)));

    PrintBeginFeature(pPDev, (bRegion) ? "PageRegion " : "PageSize ");

    //
    // Print PageSize option scans translation, omit null terminator
    //

    keycpyn((CHAR *)FormName,
            pPDev->CurForm.PrinterForm,
            CCHFORMNAME + 1,
            FALSE);

    PrintString(pPDev, (CHAR *)FormName);
    PrintString(pPDev, "\n");

#if FT_TEST
    DbgPrint("PS: (%s) PrinterForm: %s\n",
                        (bRegion) ? "PageRegion" : "PageSize",
                        (CHAR *)FormName);
#endif

    //
    // find the PSFORM structure in the NTPD for the current form.
    //

    for (i = 0, pPSForm = (PSFORM *)((CHAR *)pNTPD + pNTPD->loPSFORMArray);
         i < pNTPD->cPSForms;
         i++, pPSForm++) {

        if (!(keycmp((CHAR *)pPDev->CurForm.PrinterForm,
                     (CHAR *)pNTPD + pPSForm->loFormName))) {

            PrintString(pPDev,
                        (CHAR *)pNTPD + ((bRegion) ? pPSForm->loRegionInvo :
                                                     pPSForm->loSizeInvo));

#if FT_TEST
            DbgPrint("PS: %sInvo=%s\n",
                        (bRegion) ? "Region" : "Size",
                        (CHAR *)pNTPD + ((bRegion) ? pPSForm->loRegionInvo :
                                                     pPSForm->loSizeInvo));
#endif
            break;
        }
    }

    PrintEndFeature(pPDev);
}
