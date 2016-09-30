//--------------------------------------------------------------------------
//
// Module Name:  QURYPRNT.C
//
// Brief Description:  This module contains the PSCRIPT driver's
// DevQueryPrint routine.
//
// Author:  Kent Settle (kentse)
// Created: 01-Apr-1992
//
//  27-Dec-1994 Tue 15:29:50 updated  -by-  Daniel Chou (danielc)
//      Rewrite the DevQueryPrint, IsMountedForm
//
// Copyright (c) 1992 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "pscript.h"
#include "enable.h"
#include "pscrptui.h"
#include <winspool.h>

extern PNTPD LoadPPD(PWSTR);
extern VOID GrabDefaultFormName(HANDLE, PWSTR);
extern keycpyn(char *s, char *t, int n, BOOL dotranslate);

extern
PWSTR
ConvertOldTrayTable(
    HANDLE  hHeap,
    PWSTR   pwTrayTableOld,
    DWORD   cbTrayTable
    );

PFORM_INFO_1 GetFormsDataBase(HANDLE, DWORD *, PNTPD);

#define SIZE_REGKEY 40

#if DBG
#define DBG_FORM        0
#else
#define DBG_FORM        0
#endif


DWORD
IsMountedForm(
    HANDLE  hPrinter,
    PNTPD   pNTPD,
    DEVMODE *pDevMode
    )

/*++

Routine Description:

    This function check if the form is loaded into the tray and determine
    if we can print from that tray of not


Arguments:

    hPrinter    - handle to the printer

    pNTPD       - Pointer to the NTPD data structrue parse by us

    pDevMode    - Pointer to the DEVMODE structure to be checked

Return Value:

    DWORD Error Resource ID, 0 (no error) if form is loaded.

Author:

    21-Dec-1994 Wed 15:02:43 created  -by-  Daniel Chou (danielc)
        Totally rewrite to make it work

Revision History:


--*/

{
    WCHAR   wckey[SIZE_REGKEY] = L"";
    WCHAR   SlotName[MAX_SLOT_NAME] = L"";
    WCHAR   *pTab;
    DWORD   dwtype;
    DWORD   cTab;
    DWORD   cb;
    DWORD   ErrResID = IDS_INVALID_FORM;
	PWSTR	pwstrFormName = NULL;
	WCHAR	formName[CCHFORMNAME];

	// If DM_PAPERSIZE bit is set, it has precedence over DM_FORMNAME.

	if (pDevMode->dmFields & DM_PAPERSIZE) {

		DWORD           cBytes, cForms;
		FORM_INFO_1 *	pForms;

		EnumForms(hPrinter, 1, NULL, 0, &cBytes, &cForms);
		if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
			(pForms = (PFORM_INFO_1) GlobalAlloc(GPTR, cBytes)))
		{
			if (EnumForms(hPrinter,1,(LPBYTE)pForms,cBytes,&cBytes,&cForms) &&
				DMPAPER_FIRST <= pDevMode->dmPaperSize &&
			    (DWORD) pDevMode->dmPaperSize < DMPAPER_FIRST + cForms)
			{
				wcscpy(formName,
					(pForms + (pDevMode->dmPaperSize - DMPAPER_FIRST))->pName);
				pwstrFormName = formName;
			}

			GlobalFree((HGLOBAL) pForms);
		}

		#if DBG

		if (pwstrFormName == NULL) {
			DbgPrint("Unable to map dmPaperSize to a form name!\n");
		}

		#endif
	}

	// Use dmFormName if DM_PAPERSIZE bit is not set or
	// somehow we couldn't map dmPaperSize to a valid form name

	if (pwstrFormName == NULL) {
		pwstrFormName = pDevMode->dmFormName;
	}

#if DBG_FORM
    DbgPrint("PSUI: IsMountedForm(%ws)\n", pwstrFormName);
#endif

    if (!pwstrFormName[0]) {

        return(0);
    }

    if ((pDevMode->dmFields & DM_DEFAULTSOURCE) &&
        (pDevMode->dmDefaultSource != DMBIN_FORMSOURCE)) {

        LONG    SlotNum = (LONG)pDevMode->dmDefaultSource;


        if ((SlotNum == DMBIN_MANUAL) ||
            (SlotNum == DMBIN_ENVMANUAL)) {

            LoadString(hModule,
                       SLOT_MANUAL,
                       SlotName,
                       sizeof(SlotName) / sizeof(SlotName[0]));

            ErrResID = IDS_FORM_NOT_IN_MANUAL;

        } else if ((pNTPD->cInputSlots > 1)         &&
                   (SlotNum >= (LONG)DMBIN_USER)    &&
                   (SlotNum <  (LONG)(DMBIN_USER + pNTPD->cInputSlots))) {

            PSINPUTSLOT *pSlot;
            CHAR        PPDSlotName[MAX_SLOT_NAME];


            ErrResID = IDS_FORM_NOT_IN_TRAY;
            pSlot    = (PSINPUTSLOT *)((CHAR *)pNTPD + pNTPD->loPSInputSlots) +
                       (SlotNum - DMBIN_USER);

            keycpyn(PPDSlotName,
                    (CHAR *) pNTPD + pSlot->loSlotName,
                    MAX_SLOT_NAME,
                    TRUE);

            MultiByteToWideChar(CP_ACP,
                                0,
                                PPDSlotName,
                                -1,
                                SlotName,
                                sizeof(SlotName) / sizeof(SlotName[0]));


        }

#if DBG_FORM
        DbgPrint("PSUI: PPDSlotName=%ws\n", SlotName);
#endif
    }

    pTab = NULL;

    if ((LoadString(hModule, IDS_TRAY_FORM_SIZE, wckey, SIZE_REGKEY))   &&
        (GetPrinterData(hPrinter,
                        wckey,
                        &dwtype,
                        (LPBYTE)&cTab,
                        sizeof(cTab),
                        &cb) == ERROR_SUCCESS)                          &&
        (cTab)                                                          &&
        (pTab = (WCHAR *)LocalAlloc(LPTR, cTab))                        &&
        (LoadString(hModule, IDS_TRAY_FORM_TABLE, wckey, SIZE_REGKEY))  &&
        (GetPrinterData(hPrinter,
                        wckey,
                        &dwtype,
                        (LPBYTE)pTab,
                        cTab,
                        &cb) == ERROR_SUCCESS)                          &&
        (cTab == cb)                                                    &&
        (pTab = ConvertOldTrayTable(NULL, pTab, cb))) {

        //
        // We want to skip the first WORD which is the size sanity checking
        //

        PWSTR   pCurTrayTable = (PWSTR)pTab + 1;


        while (*pCurTrayTable) {

            PWSTR   pSlotName;

            //
            // skip over input slot
            //

            pSlotName      = pCurTrayTable;
            pCurTrayTable += wcslen(pSlotName) + 1;

#if DBG_FORM
            DbgPrint("PSUI: SlotName=%ws, Form=%ws\n", pSlotName, pCurTrayTable);
#endif

            if (!wcscmp(pCurTrayTable, pwstrFormName)) {

                //
                // Found the match, try to see if slot ok
                //

                if ((!SlotName[0]) ||
                    (!wcscmp(pSlotName, SlotName))) {

                    ErrResID = 0;
                    break;
                }
            }

            //
            // Skip over formname, PrinterForm and Select
            //

            pCurTrayTable += wcslen(pCurTrayTable) + 1;
            pCurTrayTable += wcslen(pCurTrayTable) + 1;
            pCurTrayTable += 1;
        }

    } else {

        //
        // If we cannot find form and ok to check the default form then
        // do it now
        //

        WCHAR DefaultForm[CCHFORMNAME];

        GrabDefaultFormName(hModule, DefaultForm);

#if DBG_FORM
        DbgPrint("PSUI: Check Default Form=%ws\n", DefaultForm);
#endif
        if (!wcscmp(DefaultForm, pwstrFormName)) {

            ErrResID = 0;
        }
    }

    //
    // Free the memory if we allocate one
    //

    if (pTab) {

        LocalFree((HLOCAL)pTab);
    }

#if DBG_FORM
    if (ErrResID) {

        WCHAR   ErrIDName[80];

        LoadString(hModule,
                   ErrResID,
                   ErrIDName,
                   sizeof(ErrIDName) / sizeof(ErrIDName[0]));

        DbgPrint("PSUI: IsMountedForm Error=%ws\n", ErrIDName);

    } else {

        DbgPrint("PSUI: IsMountedForm = OK TO PRINT THE FORM\n");
    }
#endif

    return(ErrResID);
}



BOOL
DevQueryPrint(
    HANDLE  hPrinter,
    DEVMODE *pDevMode,
    DWORD   *pResID
    )

/*++

Routine Description:

    This routine determines whether or not the driver can print the job
    described by pDevMode on the printer described by hPrinter.  If if can,
    it puts zero into pResID.  If it cannot, it puts the resource id of the
    string describing why it could not.

Arguments:

    hPrinter    - Handle to the printer interested

    pDevMode    - Pointer to the DEVMODE structure for the print job.

    pResID      - Pointer to the resource ID to describe the failure.


Return Value:

   This routine returns TRUE for success, FALSE for failure.


Author:

    22-Dec-1994 Thu 09:39:05 created  -by-  Daniel Chou (danielc)
        Clean up, optimized and make it correct for checking form

Revision History:


--*/

{
    DWORD   ResID;
    PNTPD   pntpd;


    //
    // if we have a NULL pDevMode, then we have nothing to do.
    // the printer will just use defaults.
    //

    if (pDevMode == NULL) {

        return(TRUE);
    }

    if (!(pntpd = MapPrinter(hPrinter))) {

        RIP("PSCRPTUI!DevQueryPrinter: MapPrinter failed.\n");
        return(FALSE);
    }

    //
    // assume everything will work.
    //

    ResID = 0;

    // verify a bunch of stuff in the DEVMODE structure.

//!!! we should do some kind of version checking, once it has
//!!! been defined what we should check for.

#if 0
    if ((pDevMode->dmSpecVersion != DM_SPECVERSION) ||
        (pDevMode->dmDriverVersion != DRIVER_VERSION)) {

        ResID = IDS_INVALID_VERSION;

    } else
#endif

    if (pDevMode->dmSize != sizeof(DEVMODE)) {

        ResID = IDS_INVALID_DEVMODE_SIZE;

    } else if ((pDevMode->dmOrientation != DMORIENT_PORTRAIT) &&
               (pDevMode->dmOrientation != DMORIENT_LANDSCAPE)) {

        ResID = IDS_INVALID_ORIENTATION;

    } else if (ResID = IsMountedForm(hPrinter, pntpd, pDevMode)) {

        // when a form is specified in the DEVMODE structure, first search
        // to see if it can be found in the forms database.  if it is not
        // found, return that fact to the caller.  if it is found, then
        // check to see if the current printer can print on the form.

        NULL;

    } else if ((pDevMode->dmFields & DM_PAPERLENGTH) &&
               (pDevMode->dmFields & DM_PAPERWIDTH)  &&
               ((!pDevMode->dmPaperLength) ||
                (!pDevMode->dmPaperWidth))) {

        //
        // What happened here???  DanielC
        //
        // override the paper size if both the paper length and width
        // fields are set, and the corresponding values are valid.
        //

        ResID = IDS_INVALID_PAPER_SIZE;

    } else if ((pDevMode->dmScale < MIN_SCALE) ||
               (pDevMode->dmScale > MAX_SCALE)) {

        ResID = IDS_INVALID_SCALE;

    } else if ((pDevMode->dmCopies < MIN_COPIES) ||
               (pDevMode->dmCopies > MAX_COPIES)) {

        ResID = IDS_INVALID_NUMBER_OF_COPIES;

    } else if ((pDevMode->dmColor != DMCOLOR_COLOR) &&
                  (pDevMode->dmColor != DMCOLOR_MONOCHROME)) {

        ResID = IDS_INVALID_COLOR;

    } else if ((pDevMode->dmColor == DMCOLOR_COLOR) &&
               (!(pntpd->flFlags & COLOR_DEVICE))) {

        ResID = IDS_COLOR_ON_BW;

    } else if ((pDevMode->dmDuplex != DMDUP_SIMPLEX)     &&
               (pDevMode->dmDuplex != DMDUP_HORIZONTAL)  &&
               (pDevMode->dmDuplex != DMDUP_VERTICAL)) {

        ResID = IDS_INVALID_DUPLEX;

    } else if ((pDevMode->dmDriverExtra != 0) &&
               (pDevMode->dmDriverExtra !=
                                (sizeof(PSDEVMODE) - pDevMode->dmSize))) {

        ResID = IDS_INVALID_DRIVER_EXTRA_SIZE;

    } else if (pDevMode->dmPrintQuality > 0) {

        //
        // Check out if print quality is ok at last
        //

        if (pntpd->cResolutions == 0) {

            if (pDevMode->dmPrintQuality != (SHORT)pntpd->iDefResolution) {

                ResID = IDS_INVALID_RESOLUTION;
            }

        } else {

            PSRESOLUTION    *pRes;
            DWORD           i;


            //
            // the current device supports multiple resolutions, so make
            // sure that the user has selected one of them.
            //

            pRes  = (PSRESOLUTION *)((CHAR *)pntpd + pntpd->loResolution);
            ResID = IDS_INVALID_RESOLUTION;

            for (i = 0; i < pntpd->cResolutions; i++) {

                if ((pDevMode->dmPrintQuality == (SHORT)pRes++->iValue)) {

                    ResID = 0;
                    break;
                }
            }
        }
    }

    //
    // Return this back to the caller
    //

    *pResID = ResID;

    //
    // free up the NTPD resource.
    //

    GlobalFree((HGLOBAL)pntpd);

    return(TRUE);
}


//--------------------------------------------------------------------------
// PNTPD MapPrinter(hPrinter)
// HANDLE  hPrinter;
//
// This routine takes a handle to a printer and returns a pointer
// to the memory mapped for the corresponding NTPD structure.
//
// This routine returns NULL for failure.
//
// History:
//   15-Apr-1992     -by-     Kent Settle     (kentse)
//  Wrote it.
//
//  8-Mar-1995 updated -davidx-
//      Rewrote it for better error handling.
//--------------------------------------------------------------------------

PNTPD MapPrinter(hPrinter)
HANDLE  hPrinter;
{
    LPDRIVER_INFO_2 pDriverInfo;
    DWORD           cbNeeded;
    PNTPD           pntpd;

    // Find out how much space required for driver info.

    if (! GetPrinterDriver(hPrinter, NULL, 2, NULL, 0, &cbNeeded)) {
		
		// GetPrinterDriver() failed. Find out what the error was.

        DWORD errorCode = GetLastError();

        if (errorCode != ERROR_INSUFFICIENT_BUFFER) {

            // The error was real. Report it and return NULL.

            DBGPS(DBGPS_LEVEL_ERROR,
                  ("pscrptui!MapPrinter: GetPrinterDriver failed (%d).\n",
                   errorCode));
            return NULL;
        }
	}

    // Allocate memory to hold driver info.

    pDriverInfo = (LPDRIVER_INFO_2)
        GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, cbNeeded);

    if (pDriverInfo == NULL) {

        // Couldn't allocate memory. Return error.

		DBGPS(DBGPS_LEVEL_ERROR,
              ("pscrptui!MapPrinter: GlobalAlloc failed (%d).\n",
                GetLastError()));
		return NULL;
    }

    if (! GetPrinterDriver(hPrinter, NULL, 2, (LPBYTE) pDriverInfo,
                           cbNeeded, &cbNeeded))
    {
		// GetPrinterDriver() failed.
		// Report error and return NULL.

		DBGPS(DBGPS_LEVEL_ERROR,
              ("pscrptui!MapPrinter: GetPrinterDriver failed (%d).\n",
                GetLastError()));
        GlobalFree ((HGLOBAL) pDriverInfo);
		return NULL;
    }

    // Parse the PPD file.

    if (! (pntpd = LoadPPD(pDriverInfo->pDataFile))) {

		DBGPS(DBGPS_LEVEL_ERROR,
              ("pscrptui!MapPrinter: Couldn't parse PPD file.\n"));
        GlobalFree((HGLOBAL) pDriverInfo);
        return NULL;
    }

    // free up memory allocated above.

    GlobalFree((HGLOBAL) pDriverInfo);
    return (pntpd);
}


//--------------------------------------------------------------------------
// PFORM_INFO_1 GetFormsDataBase(hPrinter, pcount, pntpd)
// HANDLE      hPrinter;
// DWORD      *pcount;
// PNTPD       pntpd;
//
// This routine takes a handle to a printer, enumerates the forms
// database, determines which forms are valid for the specified printer,
// and returns a pointer to an array of PFORM_INFO_1 structures.
// It also fills in the count of the forms enumerated.
//
// This routine returns NULL for failure.
//
// History:
//   21-Apr-1993     -by-     Kent Settle     (kentse)
//  Made a seperate routine.
//--------------------------------------------------------------------------

PFORM_INFO_1 GetFormsDataBase(hPrinter, pcount, pntpd)
HANDLE      hPrinter;
DWORD      *pcount;
PNTPD       pntpd;
{
    DWORD           cbNeeded, count;
    DWORD           i, j;
    PFORM_INFO_1    pdbForms, pdbForm;
    PSFORM         *pPSForm;
    SIZEL           sizlForm, sizlPSForm;

    // enumerate all the forms in the forms database.  first, pass in a
    // NULL buffer pointer, to get the size of buffer needed.

    if (!EnumForms(hPrinter, 1, NULL, 0, &cbNeeded, &count))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            RIP("PSCRPTUI!GetFormsDataBase: 1st EnumForms failed.\n");
            return((PFORM_INFO_1)NULL);
        }
    }

    // now allocate the buffer needed to enumerate all the forms.

    if (!(pdbForms = (PFORM_INFO_1)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                             cbNeeded)))
    {
        RIP("PSCRPTUI!GetFormsDataBase: GlobalAlloc failed.\n");
        return((PFORM_INFO_1)NULL);
    }

    // now get all the forms.

    if (!EnumForms(hPrinter, 1, (LPBYTE)pdbForms, cbNeeded,
                   &cbNeeded, &count))
    {
        // something went wrong.  let the caller know the enumeration failed.

        *pcount = 0;
        GlobalFree((HGLOBAL)pdbForms);
        return((PFORM_INFO_1)NULL);
    }

    // we now have a list of all the forms in the database.  now determine
    // which are valid for the current printer.

    // enumerate each form name.  check to see if it is
    // valid for the current printer.  mark the high bit of the
    // Flags element of the FORM_INFO_1 structure.

    pdbForm = pdbForms;

    for (i = 0; i < count; i++)
    {
        sizlForm = pdbForm->Size;

        pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

        // clear the valid form bit.

        pdbForm->Flags &= ~PSCRIPT_VALID_FORM;

        for (j = 0; j < pntpd->cPSForms; j++)
        {
            // convert the PSFORM sizlPaper from USER to
            // .001mm coordinates.

            sizlPSForm.cx = USERTO001MM(pPSForm->sizlPaper.cx);
            sizlPSForm.cy = USERTO001MM(pPSForm->sizlPaper.cy);

            // look for each form which matches in size.
            // (within one mm).

            if ((sizlForm.cx <= sizlPSForm.cx + 1000) &&
                (sizlForm.cx >= sizlPSForm.cx - 1000) &&
                (sizlForm.cy <= sizlPSForm.cy + 1000) &&
                (sizlForm.cy >= sizlPSForm.cy - 1000))
            {
                // mark the form as valid for this printer, and update the
                // valid form counter.

                pdbForm->Flags |= PSCRIPT_VALID_FORM;
                break;
            }

            // point to the next PSFORM.

            pPSForm++;
        }

        pdbForm++;
    }

    // everything must have worked.

    *pcount = count;
    return(pdbForms);
}

