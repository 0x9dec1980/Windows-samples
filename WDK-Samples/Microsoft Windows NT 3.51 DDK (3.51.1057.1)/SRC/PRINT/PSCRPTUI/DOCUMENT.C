//--------------------------------------------------------------------------
//
// Module Name:  DOCUMENT.C
//
// Brief Description:  This module contains the PSCRIPT driver's User
// Document Property functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 13-Apr-1992
//
// Copyright (c) 1992 Microsoft Corporation
//--------------------------------------------------------------------------

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "pscript.h"
#include "enable.h"
#include <winspool.h>
#include "dlgdefs.h"
#include "pscrptui.h"
#include "help.h"

#define DEFAULT_DOCUMENT_DIALOG 0
#define DOES_DUPLEX             1
#define DOES_COLLATE            2

#define MAX_FORM_NAME   32

// declarations of routines defined within this module.

LONG DocPropDialogProc(HWND, UINT, DWORD, LONG);
LONG AdvDocPropDialogProc(HWND, UINT, DWORD, LONG);
BOOL InitializeDocPropDialog(HWND, PDOCDETAILS);
BOOL InitializeAdvDocPropDialog(HWND, PDOCDETAILS);
VOID SetOrientation(HWND, PDOCDETAILS);
VOID SetDuplex(HWND, PDOCDETAILS);
VOID SetCollation(HWND, PDOCDETAILS);

extern
VOID
vDoColorAdjUI(
    LPWSTR          pwDeviceName,
    COLORADJUSTMENT *pcoloradj,
    BOOL            ColorAble,
    BOOL            bUpdate
    );

BYTE    DP_AnsiDeviceName[128];

extern LONG  AboutDlgProc( HWND, UINT, DWORD, LONG );
extern BOOL SetDefaultPSDEVMODE(PSDEVMODE *, PWSTR, PNTPD, HANDLE, HANDLE);
extern BOOL ValidateSetDEVMODE(PSDEVMODE *, PSDEVMODE *, HANDLE, PNTPD, HANDLE);
extern VOID SetFormSize(HANDLE, PSDEVMODE *, PWSTR);
extern PFORM_INFO_1 GetFormsDataBase(HANDLE, DWORD *, PNTPD);
extern BOOL bAbout(HWND, PNTPD);
extern BOOL bIsMetric();

void fillDMBuf(PSDEVMODE *pDMbuf, PSDEVMODE *pDMin, HANDLE hPrinter,
                PWSTR pDeviceName, PNTPD pntpd)
{
    PPRINTER_INFO_2 pPrinter2 = NULL;
    DWORD cbNeed, cbRet;

    SetDefaultPSDEVMODE(pDMbuf, pDeviceName, pntpd, hPrinter, hModule);

    if ((!GetPrinter(hPrinter, 2, NULL, 0, &cbNeed))                    &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER)                   &&
        (pPrinter2 = (PPRINTER_INFO_2)LocalAlloc(LMEM_FIXED, cbNeed))   &&
        (GetPrinter(hPrinter, 2, (LPBYTE)pPrinter2, cbNeed, &cbRet))    &&
        (cbNeed == cbRet)) {

        ValidateSetDEVMODE(pDMbuf, (PSDEVMODE*)pPrinter2->pDevMode,
                           hPrinter, pntpd, hModule);
    }

    ValidateSetDEVMODE(pDMbuf, pDMin, hPrinter, pntpd, hModule);

    if (pPrinter2) {
        LocalFree((HLOCAL)pPrinter2);
    }
}

//--------------------------------------------------------------------------
// LONG DrvDocumentProperties(hWnd, hPrinter, pDeviceName, pDevModeOutput,
//                            pDevModeInput, fMode)
// HWND        hWnd;
// HANDLE      hPrinter;
// PSTR        pDeviceName;
// PDEVMODE    pDevModeOutput;
// PDEVMODE    pDevModeInput;
// DWORD       fMode;
//
// This routine is called to set the public portions of the DEVMODE
// structure for the given print document.
//
// History:
//   13-Apr-1992    -by-    Kent Settle     (kentse)
//  Re-Wrote it.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

LONG DrvDocumentProperties(HWND hWnd,
                            HANDLE hPrinter,
                            PWSTR pDeviceName,
                            PDEVMODE pDMOut,
                            PDEVMODE pDMIn,
                            DWORD fMode)
{
    LONG            ReturnValue = IDOK;
    DOCDETAILS      DocDetails;
    PSDEVMODE      *pDMtemp = NULL;
    int             iDialog;

    // if the user passed in fMode == 0, return the size of the
    // total DEVMODE structure.

    if (!fMode)
        return(sizeof(PSDEVMODE));

#if 0
//!!! in order not to break WOW just yet, we won't check this, but
//!!! we will at some point.  -kentse.

    // it would be wrong to call this function with fMode != 0 and not
    // have the DM_COPY bit set.

    if (!(fMode & DM_COPY))
    {
        RIP("PSCRPTUI!DocumentProperties: fMode not NULL and no DM_COPY.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(-1L);
    }
#endif

    // if the copy bit is set, then there had better be a pointer
    // to an output buffer.

    if ((!pDMOut) && (fMode & DM_COPY))
    {
        RIP("PSCRPTUI!DocumentProperties: NULL pDevModeOutput and DM_COPY.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(-1L);
    }

    // initialize the document details structure.

    memset (&DocDetails, 0, sizeof(DOCDETAILS));

    // fill in document details.

    DocDetails.hPrinter = hPrinter;
    DocDetails.pDeviceName = pDeviceName;
    DocDetails.flDialogFlags = DEFAULT_DOCUMENT_DIALOG;

    // get a pointer to our NTPD structure for printer information.

    if (!(DocDetails.pntpd = MapPrinter(hPrinter)))
    {
        RIP("PSCRPTUI!DocumentProperties: MapPrinter failed.\n");
        return(-1L);
    }

    // use pDMIn regardless of DM_IN_BUFFER for compatibility with old apps

    fillDMBuf(&DocDetails.DMBuf, (PSDEVMODE *)pDMIn,
              hPrinter, pDeviceName, DocDetails.pntpd);

    if (fMode & DM_COPY)
        memcpy(pDMOut, &DocDetails.DMBuf, sizeof (PSDEVMODE));

    // prompt the user with a dialog box, if that flag is set.

    if (fMode & DM_PROMPT) {

        // set some option flags, depending on which functions the
        // current printer supports.  these options will be used to
        // determine which dialog box to present to the user.

        if ((DocDetails.pntpd->loszDuplexNone) ||
            (DocDetails.pntpd->loszDuplexNoTumble) ||
            (DocDetails.pntpd->loszDuplexTumble))
            DocDetails.flDialogFlags |= DOES_DUPLEX;

        if (DocDetails.pntpd->loszCollateOn)
            DocDetails.flDialogFlags |= DOES_COLLATE;

        switch(DocDetails.flDialogFlags)
        {
            case DEFAULT_DOCUMENT_DIALOG:
                iDialog = DOCPROPDIALOG;

                break;

            case DOES_DUPLEX:
                iDialog = DOCDUPDIALOG;

                break;

            case DOES_COLLATE:
                iDialog = DOCCOLLDIALOG;

                break;

            case (DOES_DUPLEX | DOES_COLLATE):
                iDialog = DOCBOTHDIALOG;

                break;
        }

        // present the user with the document properties dialog
        // box, which may or may not support duplex or collate.

        ReturnValue = DialogBoxParam(hModule,
                                     MAKEINTRESOURCE(iDialog),
                                     hWnd, (DLGPROC)DocPropDialogProc,
                                     (LPARAM)&DocDetails);

        // copy our newly developed PSDEVMODE structure
        // into the provided buffer.

        if ((ReturnValue == IDOK) && (fMode & DM_COPY))
            memcpy(pDMOut, &DocDetails.DMBuf, sizeof (PSDEVMODE));
    }

    // free up the NTPD memory.
    GlobalFree((HGLOBAL)DocDetails.pntpd);

    return ReturnValue;
}


//--------------------------------------------------------------------------
// BOOL InitializeDocPropDialog(hWnd, pDocDetails)
// HWND                hWnd;
// PDOCDETAILS         pDocDetails;
//
// This routine does what it's name suggests.
//
// History:
//   14-Apr-1992    -by-    Kent Settle     (kentse)
//  Re-Wrote form enumeration stuff.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL InitializeDocPropDialog(hWnd, pDocDetails)
HWND                hWnd;
PDOCDETAILS         pDocDetails;
{
    FORM_INFO_1    *pdbForm, *pdbForms;
    DWORD           count;
    DWORD           i, iForm;
    PNTPD           pntpd;
    PWSTR           pwstrFormName;
    BOOL            bTmp;

    // default to proper orientation, duplex and collation.

    bTmp = (pDocDetails->DMBuf.dm.dmOrientation == DMORIENT_LANDSCAPE);

    CheckRadioButton(hWnd, IDD_DEVICEMODEPORTRAIT, IDD_DEVICEMODELANDSCAPE,
                     bTmp ? IDD_DEVICEMODELANDSCAPE : IDD_DEVICEMODEPORTRAIT);

    SetOrientation(hWnd, pDocDetails);

    if (pDocDetails->flDialogFlags & DOES_DUPLEX)
    {
        switch (pDocDetails->DMBuf.dm.dmDuplex)
        {
            case DMDUP_VERTICAL:
                i = IDD_DUPLEX_LONG;
                break;

            case DMDUP_HORIZONTAL:
                i = IDD_DUPLEX_SHORT;
                break;

            default:
                i = IDD_DUPLEX_NONE;
                break;
        }

        // now set the appropriate radio button.

        CheckRadioButton(hWnd, IDD_DUPLEX_NONE, IDD_DUPLEX_SHORT, i);

        SetDuplex(hWnd, pDocDetails);
    }

    if (pDocDetails->flDialogFlags & DOES_COLLATE) {

        CheckRadioButton(hWnd,
                         IDD_COLLATE_OFF,
                         IDD_COLLATE_ON,
                         (pDocDetails->DMBuf.dm.dmCollate == DMCOLLATE_TRUE) ?
                                IDD_COLLATE_ON : IDD_COLLATE_OFF);

        SetCollation(hWnd, pDocDetails);
    }

    // fill in the given number of copies.

    SetDlgItemInt(hWnd, IDD_DEVICEMODENOCOPIES,
                  (int)pDocDetails->DMBuf.dm.dmCopies, FALSE);

    // get a local pointer to NTPD structure.

    pntpd = pDocDetails->pntpd;

    // fill in the forms list box.

    if (!(pdbForms = GetFormsDataBase(pDocDetails->hPrinter, &count, pntpd)))
    {
        RIP("PSCRPTUI!InitializeDocPropDialog: GetFormsDataBase failed.\n");
        return(FALSE);
    }
    else
    {
        // enumerate each form name.  check to see if it is
        // valid for the current printer.  if it is, insert
        // the form name into the forms combo box.

        pdbForm = pdbForms;

        for (i = 0; i < count; i++)
        {
            if (pdbForm->Flags & PSCRIPT_VALID_FORM)
            {
                // add the form to the list box.

                SendDlgItemMessage(hWnd, IDD_FORMCOMBO, CB_ADDSTRING,
                                   (WPARAM)0, (LPARAM)pdbForm->pName);
            }

            pdbForm++;
        }

        SendDlgItemMessage(hWnd, IDD_FORMCOMBO, CB_SETCURSEL, 0, 0L);
    }

    GlobalFree((HGLOBAL)pdbForms);

    // select the form specified in the DEVMODE in the combo box.

    pwstrFormName = pDocDetails->DMBuf.dm.dmFormName;

    iForm = SendDlgItemMessage(hWnd, IDD_FORMCOMBO, CB_FINDSTRING,
                               (WPARAM)-1, (LPARAM)pwstrFormName);

    // if the specified form could not be found in the list box, simply
    // select the first one.

    if (iForm == CB_ERR)
        iForm = 0;

    SendDlgItemMessage(hWnd, IDD_FORMCOMBO, CB_SETCURSEL, (WPARAM)iForm,
                       (LPARAM)0);

    return(TRUE);
}


//--------------------------------------------------------------------------
// LONG DocPropDialogProc(hWnd, usMst, wParam, lParam)
// HWND    hWnd;
// UINT    usMsg;
// DWORD   wParam;
// LONG    lParam;
//
// This routine services the Document Properties dialog box.
//
// History:
//   15-Apr-1992    -by-    Kent Settle     (kentse)
//  Added NTPD stuff, IDD_OPTIONS, IDD_FORMCOMBO.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

LONG DocPropDialogProc(hWnd, usMsg, wParam, lParam)
HWND    hWnd;
UINT    usMsg;
DWORD   wParam;
LONG    lParam;
{
    PDOCDETAILS     pDocDetails;
    BOOL            bTmp;
    DWORD           dwDialog;

    switch(usMsg)
    {
        case WM_INITDIALOG:
            pDocDetails = (PDOCDETAILS)lParam;

            if (!pDocDetails)
            {
                RIP("PSCRPTUI!DocPropDialogProc: null pDocDetails.\n");
                return(FALSE);
            }

            // save the pointer to the DOCDETAILS structure.

            SetWindowLong(hWnd, GWL_USERDATA, lParam);

            InitializeDocPropDialog(hWnd, pDocDetails);

            // intialize the help stuff.

            vHelpInit();

            return TRUE;

        case WM_COMMAND:
            pDocDetails = (PDOCDETAILS)GetWindowLong(hWnd, GWL_USERDATA);

            switch(LOWORD(wParam))
            {
                case IDD_HELP_BUTTON:
                    dwDialog = HLP_DOC_PROP_STANDARD;

                    switch(pDocDetails->flDialogFlags)
                    {
                        case DEFAULT_DOCUMENT_DIALOG:
                            break;

                        case DOES_DUPLEX:
                            dwDialog = HLP_DOC_PROP_DUPLEX;
                            break;

                        case DOES_COLLATE:
                            dwDialog = HLP_DOC_PROP_COLLATE;
                            break;

                        case (DOES_DUPLEX | DOES_COLLATE):
                            dwDialog = HLP_DOC_PROP_BOTH;
                            break;
                    }

                    vShowHelp(hWnd, HELP_CONTEXT, dwDialog,
                              pDocDetails->hPrinter);
                    return(TRUE);

                case IDOK:
                    // copy the number of copies into PSDEVMODE.

                    pDocDetails->DMBuf.dm.dmCopies =
                            (short)GetDlgItemInt(hWnd, IDD_DEVICEMODENOCOPIES,
                                                 &bTmp, FALSE);

                    GetDlgItemText(hWnd, IDD_FORMCOMBO,
                                   pDocDetails->DMBuf.dm.dmFormName,
                                   sizeof(pDocDetails->DMBuf.dm.dmFormName));

                    // update paper size information in DEVMODE based on the
                    // form name.
                    pDocDetails->DMBuf.dm.dmFields &= ~(DM_PAPERSIZE);
                    SetFormSize(pDocDetails->hPrinter, &pDocDetails->DMBuf,
                                pDocDetails->DMBuf.dm.dmFormName);

                    EndDialog (hWnd, IDOK);
                    return IDOK;

                case IDCANCEL:
                    EndDialog (hWnd, IDCANCEL);
                    return IDCANCEL;

                case IDD_DEVICEMODEPORTRAIT:
                    pDocDetails->DMBuf.dm.dmOrientation = DMORIENT_PORTRAIT;
                    SetOrientation(hWnd, pDocDetails);
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_DEVICEMODELANDSCAPE:
                    pDocDetails->DMBuf.dm.dmOrientation = DMORIENT_LANDSCAPE;
                    SetOrientation(hWnd, pDocDetails);
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_DUPLEX_NONE:
                    pDocDetails->DMBuf.dm.dmDuplex = DMDUP_SIMPLEX;
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_DUPLEX_LONG:
                    pDocDetails->DMBuf.dm.dmDuplex = DMDUP_VERTICAL;
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_DUPLEX_SHORT:
                    pDocDetails->DMBuf.dm.dmDuplex = DMDUP_HORIZONTAL;
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_COLLATE_ON:
                case IDD_COLLATE_OFF:

                    pDocDetails->DMBuf.dm.dmCollate =
                                (LOWORD(wParam) == IDD_COLLATE_ON) ?
                                            DMCOLLATE_TRUE : DMCOLLATE_FALSE;

                    SetCollation(hWnd, pDocDetails);
                    break;

                case IDD_OPTIONS: // Advanced dialog stuff.
                    DialogBoxParam(hModule, MAKEINTRESOURCE(ADVDOCPROPDIALOG),
                                   hWnd, (DLGPROC)AdvDocPropDialogProc,
                                   (LPARAM)pDocDetails);

                    SetFocus(GetDlgItem(hWnd, IDOK));
                    return TRUE;

                case IDD_HALFTONE_PUSH_BUTTON:

                    vDoColorAdjUI((LPWSTR)((PSTR)pDocDetails->pntpd +
                                        pDocDetails->pntpd->lowszPrinterName),
                                  & pDocDetails->DMBuf.coloradj,
                                  (BOOL)((pDocDetails->pntpd->flFlags &
                                                            COLOR_DEVICE) &&
                                         (pDocDetails->DMBuf.dm.dmColor ==
                                                            DMCOLOR_COLOR)),
                                  TRUE);

                    return(TRUE);

                case IDD_ABOUT:
                    return(bAbout(hWnd, pDocDetails->pntpd));

                default:
                    return (FALSE);
            }

            break;

        case WM_DESTROY:
            // clean up any used help stuff.

            vHelpDone(hWnd);
            return(TRUE);

        default:
            return (FALSE);
    }

    return  FALSE;
}


//--------------------------------------------------------------------------
// LONG DrvAdvancedDocumentProperties(hWnd, hPrinter, pDeviceName, pDevModeOutput,
//                                 pDevModeInput)
// HWND        hWnd;
// HANDLE      hPrinter;
// LPSTR       pDeviceName;
// PDEVMODE    pDevModeOutput;
// PDEVMODE    pDevModeInput;
//
// This routine is called to set the public portions of the DEVMODE
// structure for the given print document.
//
// History:
//   15-Apr-1992    -by-    Kent Settle     (kentse)
//  Added DocDetails.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

LONG DrvAdvancedDocumentProperties(hWnd, hPrinter, pDeviceName, pDevModeOutput,
                                   pDevModeInput)
HWND        hWnd;
HANDLE      hPrinter;
PWSTR       pDeviceName;
PDEVMODE    pDevModeOutput;
PDEVMODE    pDevModeInput;
{
    LONG        ReturnValue;
    DOCDETAILS  DocDetails;

    if (!pDevModeOutput) return sizeof (PSDEVMODE);

    // Fill in the DOCDETAILS structure so that lower level functions
    // can access this important data.

    DocDetails.hPrinter = hPrinter;
    DocDetails.pDeviceName = pDeviceName;

    // get a pointer to our NTPD structure for printer information.

    if (!(DocDetails.pntpd = MapPrinter(DocDetails.hPrinter)))
    {
        RIP("PSCRPTUI!DocumentProperties: MapPrinter failed.\n");
        return FALSE;
    }

    fillDMBuf(&DocDetails.DMBuf, (PSDEVMODE *) pDevModeInput, hPrinter, pDeviceName,
                DocDetails.pntpd);

    memcpy(pDevModeOutput, &DocDetails.DMBuf, sizeof (PSDEVMODE));

    ReturnValue = DialogBoxParam(hModule,
                             MAKEINTRESOURCE(ADVDOCPROPDIALOG),
                             hWnd,
                             (DLGPROC)AdvDocPropDialogProc,
                             (LPARAM)&DocDetails);

    if (ReturnValue == IDOK)
        memcpy(pDevModeOutput, &DocDetails.DMBuf, sizeof (PSDEVMODE));

    // free up the NTPD memory.
    GlobalFree((HGLOBAL)DocDetails.pntpd);

    return ReturnValue == IDOK ? TRUE : FALSE;
}


//--------------------------------------------------------------------------
// LONG AdvDocPropDialogProc(hWnd, usMsg, wParam, lParam)
// HWND    hWnd;
// UINT    usMsg;
// DWORD   wParam;
// LONG    lParam;
//
// This routine services the Advanced Document Properties dialog box.
//
// History:
//   15-Apr-1992    -by-    Kent Settle     (kentse)
//  Re-Wrote it.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

LONG AdvDocPropDialogProc(hWnd, usMsg, wParam, lParam)
HWND    hWnd;
UINT    usMsg;
DWORD   wParam;
LONG    lParam;
{
    PDOCDETAILS pDocDetails;
    LONG        i;
    BOOL        bTmp;


    switch(usMsg) {

    case WM_INITDIALOG:

        //
        // get a local pointer to our DOCDETAILS structure.
        //

        pDocDetails = (PDOCDETAILS)lParam;
        SetWindowLong(hWnd, GWL_USERDATA, (LONG)pDocDetails);

        if (!InitializeAdvDocPropDialog(hWnd, pDocDetails)) {

            RIP("PSCRPTUI!AdvDocPropDialogProc: InitializeAdvDocPropDialog failed.\n");
            return(FALSE);
        }

        //
        // intialize the help stuff.
        //

        vHelpInit();

        return (TRUE);

    case WM_COMMAND:

        //
        // get the DOCDETAILS structure we save at initialization.
        //

        pDocDetails = (PDOCDETAILS)GetWindowLong(hWnd, GWL_USERDATA);

        switch(LOWORD(wParam)) {

        case IDOK:

            //
            // copy the selected resolution into PSDEVMODE. first
            // get the selected value from the combo box.
            //

            if (((i = SendDlgItemMessage(hWnd,
                                        IDD_RESOLUTION_COMBO_BOX,
                                        CB_GETCURSEL,
                                        0,
                                        0L)) != CB_ERR) &&
                ((i = SendDlgItemMessage(hWnd,
                                        IDD_RESOLUTION_COMBO_BOX,
                                        CB_GETITEMDATA,
                                        i,
                                        0L)) != CB_ERR)) {

                pDocDetails->DMBuf.dm.dmPrintQuality = (short)i;

            } else {

                RIP("PSCRPTUI!AdvDocPropDialogProc: CB_GETCURSEL failed.\n");
            }

            //
            // copy the scaling percent into PSDEVMODE.
            //

            pDocDetails->DMBuf.dm.dmScale =
                                    (short)GetDlgItemInt(hWnd,
                                                         IDD_SCALING_EDIT_BOX,
                                                         &bTmp,
                                                         FALSE);

            //
            // Get Color Setting
            //

            pDocDetails->DMBuf.dm.dmColor =
                        (IsDlgButtonChecked(hWnd, IDD_COLOR_CHECK_BOX)) ?
                                            DMCOLOR_COLOR : DMCOLOR_MONOCHROME;

            //
            // Now Get rest of the flags/check box/radio buttons
            //

            pDocDetails->DMBuf.dwFlags &= ~(PSDEVMODE_BLACK             |
                                            PSDEVMODE_NEG               |
                                            PSDEVMODE_EHANDLER          |
                                            PSDEVMODE_MIRROR            |
                                            PSDEVMODE_FONTSUBST         |
                                            PSDEVMODE_INDEPENDENT       |
                                            PSDEVMODE_COMPRESSBMP       |
                                            PSDEVMODE_ENUMPRINTERFONTS);

            if (IsDlgButtonChecked(hWnd, IDD_COLORS_TO_BLACK_CHECK_BOX)) {

                pDocDetails->DMBuf.dwFlags |= PSDEVMODE_BLACK;
            }

            if (IsDlgButtonChecked(hWnd, IDD_NEG_IMAGE_CHECK_BOX)) {

                pDocDetails->DMBuf.dwFlags |= PSDEVMODE_NEG;
            }

            if (IsDlgButtonChecked(hWnd, IDD_MIRROR_CHECK_BOX)) {

                pDocDetails->DMBuf.dwFlags |= PSDEVMODE_MIRROR;
            }

            if (IsDlgButtonChecked(hWnd, IDD_SUBST_RADIO_BUTTON)) {

                pDocDetails->DMBuf.dwFlags |= PSDEVMODE_FONTSUBST;
            }

            if (IsDlgButtonChecked(hWnd, IDD_PAGE_INDEPENDENCE)) {

                pDocDetails->DMBuf.dwFlags |= PSDEVMODE_INDEPENDENT;
            }

            if (IsDlgButtonChecked(hWnd, IDD_COMPRESS_BITMAPS)) {

                pDocDetails->DMBuf.dwFlags |= PSDEVMODE_COMPRESSBMP;
            }

            if (IsDlgButtonChecked(hWnd, IDD_ENUM_PRINTERFONTS)) {

                pDocDetails->DMBuf.dwFlags |= PSDEVMODE_ENUMPRINTERFONTS;
            }

            EndDialog (hWnd, IDOK);
            return (TRUE);

        case IDCANCEL:

            EndDialog (hWnd, IDCANCEL);
            return (TRUE);

        case IDD_HELP_BUTTON:

            vShowHelp(hWnd,
                      HELP_CONTEXT,
                      HLP_ADV_DOC_PROP,
                      pDocDetails->hPrinter);

            return (TRUE);

        default:

            break;
        }

        break;

    case WM_DESTROY:
        // clean up any used help stuff.

        vHelpDone(hWnd);
        return (TRUE);

    default:

        break;
    }

    return (FALSE);
}


//--------------------------------------------------------------------------
// BOOL InitializeAdvDocPropDialog(hWnd, pDocDetails)
// HWND                hWnd;
// PDOCDETAILS         pDocDetails;
//
// This routine does what it's name suggests.
//
// History:
//   15-Apr-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//
//  18-Jan-1995 Wed 12:18:01 updated  -by-  Daniel Chou (danielc)
//      Updated so that not used checkbox will be dynamically removed or
//      change position then re-size the dialog box window
//--------------------------------------------------------------------------

BOOL InitializeAdvDocPropDialog(hWnd, pDocDetails)
HWND                hWnd;
PDOCDETAILS         pDocDetails;
{
    PSDEVMODE       *pdevmode;
    WCHAR           wcbuf[8];
    LONG            i;
    LONG            j;
    LONG            iResolution;
    PSRESOLUTION    *pRes;
    PNTPD           pntpd;
    RECT            rc;
    POINT           pt;
    LONG            SubY;
    LONG            OneChkBoxY;
    DWORD           dwFlags;
    BOOL            bFontSubstitution;

    pntpd = pDocDetails->pntpd;
    pdevmode = &pDocDetails->DMBuf;

    //
    // reset the content of the resolution combo box and fill in the
    // resolution combo box.
    //

    SendDlgItemMessage (hWnd, IDD_RESOLUTION_COMBO_BOX, CB_RESETCONTENT, 0, 0);

    if (!(i = (int)pntpd->cResolutions)) {

        i    = 1;
        pRes = NULL;

    } else {

        pRes = (PSRESOLUTION *)((CHAR *)pntpd + pntpd->loResolution);
    }

    while (i--) {

        iResolution = (LONG)((pRes) ? pRes->iValue : pntpd->iDefResolution);

        wsprintf(wcbuf, L"%ld", iResolution);

        j = SendDlgItemMessage(hWnd,
                               IDD_RESOLUTION_COMBO_BOX,
                               CB_INSERTSTRING,
                               (WPARAM)-1,
                               (LPARAM)wcbuf);

        SendDlgItemMessage(hWnd,
                           IDD_RESOLUTION_COMBO_BOX,
                           CB_SETITEMDATA,
                           (WPARAM)j,
                           (LPARAM)iResolution);

        if ((!j)    ||
            (!pRes) ||
            (pdevmode->dm.dmPrintQuality == (short)iResolution)) {

            SendDlgItemMessage(hWnd,
                               IDD_RESOLUTION_COMBO_BOX,
                               CB_SETCURSEL,
                               j,
                               0L);
        }

        pRes++;
    }

    // fill in the scaling percentage.

    SetDlgItemInt(hWnd, IDD_SCALING_EDIT_BOX, (int)pdevmode->dm.dmScale, FALSE);

    // set the state of the color check box.  if the printer is b/w, grey
    // out the check box.  if the printer is color, and DMCOLOR_COLOR is
    // set in the PSDEVMODE, then check the check box.  otherwise, clear
    // the check box.

    if (!(pntpd->flFlags & COLOR_DEVICE)) {

        pdevmode->dm.dmColor == DMCOLOR_MONOCHROME;
        EnableWindow(GetDlgItem(hWnd, IDD_COLOR_CHECK_BOX), FALSE);
    }

    CheckDlgButton(hWnd,
                   IDD_COLOR_CHECK_BOX,
                   pdevmode->dm.dmColor == DMCOLOR_COLOR);


    //
    // Now check if IDD_COMPRESS_BITMAPS, and IDD_ENUM_PRINTERFONTS need to
    // removed or move to different position and re-size the window
    //

    GetWindowRect(GetDlgItem(hWnd, IDD_COMPRESS_BITMAPS), &rc);
    OneChkBoxY = rc.bottom;
    GetWindowRect(GetDlgItem(hWnd, IDD_PAGE_INDEPENDENCE), &rc);
    OneChkBoxY -= rc.bottom;
    SubY = 0;

    if (pntpd->LangLevel == 1) {

        //
        // Add support for the compress bitmap checkbox. This is only available
        // on level two printers.
        //

        GetWindowRect(GetDlgItem(hWnd, IDD_COMPRESS_BITMAPS), &rc);
        SubY += OneChkBoxY;

        pdevmode->dwFlags &= ~PSDEVMODE_COMPRESSBMP;
        ShowWindow(GetDlgItem(hWnd, IDD_COMPRESS_BITMAPS), SW_HIDE);

    } else {

        GetWindowRect(GetDlgItem(hWnd, IDD_ENUM_PRINTERFONTS), &rc);
    }

    if (GetACP() == 1252) {

        //
        // built in postscript printer fonts contain code page 1252
        // (StandardEncoding reencoded) so that by default in non 1252
        // environments we do not want to have printer fonts enumerated
        //

        GetWindowRect(GetDlgItem(hWnd, IDD_ENUM_PRINTERFONTS), &rc);
        SubY += OneChkBoxY;
        ShowWindow(GetDlgItem(hWnd, IDD_ENUM_PRINTERFONTS), SW_HIDE);

        pdevmode->dwFlags |= PSDEVMODE_ENUMPRINTERFONTS;

    } else if (SubY) {

        //
        // Move the Enum Resident Printer Fonts up to the Compress Bitmap
        //

        pt.x = rc.left;
        pt.y = rc.top;

        ScreenToClient(hWnd, &pt);

        SetWindowPos(GetDlgItem(hWnd, IDD_ENUM_PRINTERFONTS),
                     NULL,
                     pt.x,
                     pt.y,
                     0,
                     0,
                     SWP_NOSIZE | SWP_NOZORDER);
    }

    //
    // If we ever change the positon of check-box or removed them then change
    // window size to match that too
    //

    if (SubY) {

        GetWindowRect(hWnd, &rc);
        rc.bottom -= SubY;

        SetWindowPos(hWnd,
                     NULL,
                     0,
                     0,
                     rc.right - rc.left,
                     rc.bottom - rc.top,
                     SWP_NOMOVE | SWP_NOZORDER);

    }

    dwFlags = pdevmode->dwFlags;

    CheckDlgButton(hWnd,
                   IDD_COLORS_TO_BLACK_CHECK_BOX,
                   dwFlags & PSDEVMODE_BLACK);

    CheckDlgButton(hWnd,
                   IDD_NEG_IMAGE_CHECK_BOX,
                   dwFlags & PSDEVMODE_NEG);

    CheckDlgButton(hWnd,
                   IDD_MIRROR_CHECK_BOX,
                   dwFlags & PSDEVMODE_MIRROR);

    CheckRadioButton(hWnd,
                     IDD_SUBST_RADIO_BUTTON,
                     IDD_SOFTFONT_RADIO_BUTTON,
                     (dwFlags & PSDEVMODE_FONTSUBST) ?
                                        IDD_SUBST_RADIO_BUTTON :
                                        IDD_SOFTFONT_RADIO_BUTTON);
    CheckDlgButton(hWnd,
                   IDD_PAGE_INDEPENDENCE,
                   dwFlags & PSDEVMODE_INDEPENDENT);

    CheckDlgButton(hWnd,
                   IDD_COMPRESS_BITMAPS,
                   dwFlags & PSDEVMODE_COMPRESSBMP);

    CheckDlgButton(hWnd,
                   IDD_ENUM_PRINTERFONTS,
                   dwFlags & PSDEVMODE_ENUMPRINTERFONTS);

    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID SetOrientation(hWnd, pDocDetails)
// HWND        hWnd;
// DOCDETAILS *pDocDetails;
//
// This routine selects the proper portrait or landscape icon, depending
// on bPortrait.
//
// History:
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

VOID SetOrientation(hWnd, pDocDetails)
HWND        hWnd;
DOCDETAILS *pDocDetails;
{
    BOOL    bLandscape;

    bLandscape = (pDocDetails->DMBuf.dm.dmOrientation == DMORIENT_LANDSCAPE);

    // load the icons needed within this dialog box.

    pDocDetails->hIconPortrait = LoadIcon(hModule,
                                          MAKEINTRESOURCE(ICOPORTRAIT));
    pDocDetails->hIconLandscape = LoadIcon(hModule,
                                          MAKEINTRESOURCE(ICOLANDSCAPE));

    SendDlgItemMessage(hWnd, IDD_ORIENTATION_ICON, STM_SETICON,
                       bLandscape ? (WPARAM)pDocDetails->hIconLandscape :
                                   (WPARAM)pDocDetails->hIconPortrait, 0L);
}


//--------------------------------------------------------------------------
// VOID SetDuplex(hWnd, pDocDetails)
// HWND        hWnd;
// PDOCDETAILS pDocDetails;
//
// This routine will operate on pDocDetails->pDMInput PSDEVMODE structure,
// making sure that is a structure we know about and can handle.
//
// History:
//   20-Apr-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID SetDuplex(hWnd, pDocDetails)
HWND        hWnd;
PDOCDETAILS pDocDetails;
{
    BOOL    bPortrait;
    HANDLE  hDuplexIcon;

    // set up for proper default duplex mode if it exists on the printer.

    if (pDocDetails->flDialogFlags & DOES_DUPLEX)
    {
        // load the duplex icons.

        pDocDetails->hIconPDuplexNone = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_P_NONE));
        pDocDetails->hIconLDuplexNone = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_L_NONE));
        pDocDetails->hIconPDuplexTumble = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_P_HORIZ));
        pDocDetails->hIconLDuplexTumble = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_L_VERT));
        pDocDetails->hIconPDuplexNoTumble = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_P_VERT));
        pDocDetails->hIconLDuplexNoTumble = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_L_HORIZ));

//!!! Should we have to worry about TRUE and FALSE Duplex printers? - kentse.

        bPortrait = (pDocDetails->DMBuf.dm.dmOrientation ==
                    DMORIENT_PORTRAIT);

        switch (pDocDetails->DMBuf.dm.dmDuplex)
        {
            case DMDUP_VERTICAL:
                hDuplexIcon = bPortrait ? pDocDetails->hIconPDuplexNoTumble :
                                          pDocDetails->hIconLDuplexTumble;
                break;

            case DMDUP_HORIZONTAL:
                hDuplexIcon = bPortrait ? pDocDetails->hIconPDuplexTumble :
                                          pDocDetails->hIconLDuplexNoTumble;
                break;

            default:
                hDuplexIcon = bPortrait ? pDocDetails->hIconPDuplexNone :
                                          pDocDetails->hIconLDuplexNone;
                break;
        }

        // now set the appropriate icon.

        SendDlgItemMessage(hWnd, IDD_DUPLEX_ICON, STM_SETICON,
                           (LONG)hDuplexIcon, 0L);
    }
}


//--------------------------------------------------------------------------
// VOID SetCollation(hWnd, pDocDetails)
// HWND        hWnd;
// DOCDETAILS *pDocDetails;
//
// This routine selects the proper collation icon and radio
// buttons, depending on pDocDetails.
//
// History:
//   01-Dec-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID SetCollation(hWnd, pDocDetails)
HWND        hWnd;
DOCDETAILS *pDocDetails;
{
    //
    // set up for proper default collate mode if it exists on the printer.
    //

    if (pDocDetails->flDialogFlags & DOES_COLLATE) {

        HICON   hIcon;

        hIcon = LoadIcon(hModule,
                         (pDocDetails->DMBuf.dm.dmCollate == DMCOLLATE_TRUE) ?
                                MAKEINTRESOURCE(ICO_COLLATE) :
                                MAKEINTRESOURCE(ICO_NO_COLLATE));

        SendDlgItemMessage(hWnd,
                           IDD_COLLATE_ICON,
                           STM_SETICON,
                           (WPARAM)hIcon,
                           (LPARAM)0);
    }
}
