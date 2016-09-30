//--------------------------------------------------------------------------
//
// Module Name:  PSCRPTUI.C
//
// Brief Description:  This module contains the PSCRIPT driver's User
// Interface functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 11-Jul-1991
//
// Copyright (c) 1991-1992 Microsoft Corporation
//
// This module contains routines supporting the setting of Printer and
// Job Property dialogs for the NT Windows PostScript printer driver.
//
// The general outline for much of the code was taken from the NT RASDD
// printer driver's user interface code, which was written by Steve
// Cathcart (stevecat).
//--------------------------------------------------------------------------

#define _HTUI_APIS_

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "pscript.h"
#include "shellapi.h"
#include <winspool.h>
#include "dlgdefs.h"
#include "pscrptui.h"
#include "help.h"
#include "afm.h"

#define UNINITIALIZED_FORM  -1
#define TRANSLATED_TRAYS    0x00000001
#define TRANSLATED_FORMS    0x00000002


// declarations of routines defined within this module.

LONG PrtPropDlgProc( HWND, UINT, DWORD, LONG );
LONG AboutDlgProc( HWND, UINT, DWORD, LONG );
BOOL InitComboBoxes(HWND, PRINTDATA *);
BOOL bAbout(HWND, PNTPD);
VOID GetPrinterForm(PNTPD, FORM_INFO_1 *, PWSTR);

// external routines and data.

extern PSFORM* MatchFormToPPD(PNTPD, LPWSTR);
extern LONG  FontInstDlgProc( HWND, UINT, DWORD, LONG);
extern LONG TTFontDialogProc(HWND, UINT, DWORD, LONG);
//extern TT_FONT_MAPPING TTFontTable[]; // ..\pscript\tables.h.
extern PFORM_INFO_1 GetFormsDataBase(HANDLE, DWORD *, PNTPD);
extern BOOL bIsMetric();
extern VOID GrabDefaultFormName(HANDLE, PWSTR);

VOID
vDoDeviceHTDataUI(
    LPWSTR  pwDeviceName,
    BOOL    ColorDevice,
    BOOL    bUpdate
    );

extern
void
vGetDeviceHTData(
    HANDLE      hPrinter,
    PDEVHTINFO  pDefaultDevHTInfo
    );


extern
BOOL
bSaveDeviceHTData(
    HANDLE  hPrinter,
    BOOL    bForce              // TRUE if always update
    );

extern
PWSTR
ConvertOldTrayTable(
    HANDLE  hHeap,
    PWSTR   pwTrayTableOld,
    DWORD   cbTrayTable
    );

// global Data

DWORD   Type=1;


#if DBG

// 3-Mar-1995 -davidx-
//
// Global debug output level. Patch this in the debugger to
// get more or less debug messages. The default is to show
// all warning messages.
// 

int PsDebugLevel = DBGPS_LEVEL_WARNING;

#endif


#define MAX_TRAYS       9
#define INVALID_TRAY    -1L
#define INVALID_FORM    -1L
#define MAX_SKIP        5
#define SMALL_BUF       32

#define RESOURCE_STRING_LENGTH  128

#if DBG
#define DEF_TRAY_TEST           0
#else
#define DEF_TRAY_TEST           0
#endif


#define DW2FORMIDX(dw)          LOWORD(dw)
#define DW2SEL(dw)              HIWORD(dw)
#define FORMIDXSEL2DW(f,s)      (DWORD)MAKELONG((f),(s))

#define DW2MATCH(dw)            LOWORD(dw)
#define MATCHSEL2DW(m,s)        (DWORD)MAKELONG((m),(s))


typedef struct _TRAYLIST {
    PRINTDATA   *pData;
    HWND        hDlgParent;
    WORD        TrayIdx;
    WORD        FormIdx;
    } TRAYLIST, *PTRAYLIST;



PWSTR
strncpy2WChar(
    PWSTR   pwStr,
    LPSTR   pStr,
    INT     Len
    )
{
    BYTE    Ch;
    LPSTR   pNull = NULL;

    if ((Len) && ((INT)strlen(pStr) >= Len)) {

        pNull  = pStr + Len - 1;
        Ch     = *pNull;
        *pNull = 0;
    }

    strcpy2WChar(pwStr, pStr);

    if (pNull) {

        *pNull = Ch;
    }

    return(pwStr);
}




DWORD
CountFormTrays(
    PDEFFORM    pDefForm,
    int         cTrays,
    WORD        FormIdx
    )

/*++

Routine Description:

   This function clear all the default entry in all the tray


Arguments:

    pDefForm    - Pointer to the DEFFORM data structure for current trays

    cTrays      - Total tray for this printer

    FormIdx     - The form index to be clear for trays


Return Value:

    DWORD   - LOWORD = cMatch, HIWORD = cSelect

Author:

    19-Dec-1994 Mon 17:11:31 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    WORD    cMatch = 0;
    WORD    Select = 0;

    while (cTrays--) {

        if (pDefForm->Index == FormIdx) {

            ++cMatch;

            if (pDefForm->Select) {

                ++Select;
            }
        }

        ++pDefForm;
    }

    return(MATCHSEL2DW(cMatch, Select));
}




int
ClearDefTray(
    PDEFFORM    pDefForm,
    int         cTrays,
    WORD        FormIdx
    )

/*++

Routine Description:

   This function clear all the default entry in all the tray


Arguments:

    pDefForm    - Pointer to the DEFFORM data structure for current trays

    cTrays      - Total tray for this printer

    FormIdx     - The form index to be clear for trays


Return Value:

    int - Total tray cleared for this form


Author:

    19-Dec-1994 Mon 17:11:31 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    int cMatch = 0;

    while (cTrays--) {

        if (pDefForm->Index == FormIdx) {

            ++cMatch;

            pDefForm->Select = 0;
        }

        ++pDefForm;
    }

    return(cMatch);
}






BOOL
ShowCurTrayInfo(
    HWND        hDlg,
    PRINTDATA   *pData
    )

/*++

Routine Description:

    This function show the current selected tray information, this includes
    gray out/mark the checkbox selection, show if auto selection

Arguments:

    hDlg        - Handle to the dialog

    pDefForm    - Pointer to the DEFFORM data structure for the tray


Return Value:

    BOOL    - TRUE if auto select, FALSE otherwise


Author:

    10-Jan-1995 Tue 19:05:42 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DWORD       dwData;
    int         iTray;
    int         cTrays;
    WCHAR       AutoSelName[64];


    iTray = SendDlgItemMessage(hDlg,
                               IDD_TRAY_FORM_LB,
                               LB_GETCURSEL,
                               (WPARAM)0,
                               (LPARAM)0);

    dwData = (DWORD)SendDlgItemMessage(hDlg,
                                       IDD_TRAY_FORM_LB,
                                       LB_GETITEMDATA,
                                       (WPARAM)iTray,
                                       (LPARAM)0);

    cTrays = (DWORD)SendDlgItemMessage(hDlg,
                                       IDD_TRAY_FORM_LB,
                                       LB_GETCOUNT,
                                       (WPARAM)0,
                                       (LPARAM)0);

    //
    // Check the check box if this is the from to be used
    //

    CheckDlgButton(hDlg, IDD_DRAWPAPER_CHKBOX, DW2SEL(dwData));

    dwData = CountFormTrays(pData->pDefForm, cTrays, DW2FORMIDX(dwData));

    //
    // If only one tray has that form then gray out the check box
    //

    EnableWindow(GetDlgItem(hDlg, IDD_DRAWPAPER_CHKBOX),
                 (pData->bPermission) && (DW2MATCH(dwData) > 1));

    //
    // If non of them selected then it is a AUTO SELECT
    //

    if (!DW2SEL(dwData)) {

        LoadString(hModule,
                   IDS_AUTO_SELECT,
                   AutoSelName,
                   (sizeof(AutoSelName) / sizeof(AutoSelName[0])));


    } else {

        AutoSelName[0] = L'\0';
    }

    SetWindowText(GetDlgItem(hDlg, IDD_FORM_INFO_TEXT), AutoSelName);

    return((BOOL)AutoSelName[0]);
}




VOID
SetTrayFormName(
    HWND        hDlg,
    PRINTDATA   *pData,
    LONG        FirstSel
    )

/*++

Routine Description:

    This function update the tray list box for the current tray/form/select
    information


Arguments:

    hDlg        - Handle to the dialog box

    pDefForm    - Pointer to array of DEFFORM for the trays

    FirstSel    - First time default selection


Return Value:




Author:

    09-Jan-1995 Mon 12:10:32 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDEFFORM    pDefForm;
    int         iTray;
    int         cTrays;
    int         i;


    LockWindowUpdate(GetDlgItem(hDlg, IDD_TRAY_FORM_LB));

    cTrays = SendDlgItemMessage(hDlg, IDD_TRAY_LIST_BOX, CB_GETCOUNT, 0, 0L);

    if ((iTray = FirstSel) < 0) {

        iTray = SendDlgItemMessage(hDlg, IDD_TRAY_FORM_LB, LB_GETCURSEL, 0, 0L);
    }

    for (i = 0, pDefForm = pData->pDefForm; i < cTrays; i++, pDefForm++) {

        DWORD   dwData;


        dwData = MAKELONG(pDefForm->Index, pDefForm->Select);

        if ((FirstSel >= 0) ||
            (dwData != (DWORD)SendDlgItemMessage(hDlg,
                                                 IDD_TRAY_FORM_LB,
                                                 LB_GETITEMDATA,
                                                 (WPARAM)i,
                                                 (LPARAM)0))) {

            WCHAR   wBuf[MAX_SLOT_NAME + CCHFORMNAME + 16];
            WCHAR   wTrayName[MAX_SLOT_NAME];
            WCHAR   wFormName[CCHFORMNAME];


            SendDlgItemMessage(hDlg,
                               IDD_TRAY_LIST_BOX,
                               CB_GETLBTEXT,
                               (WPARAM)i,
                               (LPARAM)wTrayName);
            SendDlgItemMessage(hDlg,
                               IDD_PAPER_LIST_BOX,
                               CB_GETLBTEXT,
                               (WPARAM)pDefForm->Index,
                               (LPARAM)wFormName);

            wTrayName[38] = L'\0';
            wsprintf(wBuf, L"%ws\t%wc%ws",
                        wTrayName, (pDefForm->Select) ? L'*' : L' ', wFormName);

#if DEF_TRAY_TEST
            DbgPrint("%ld: Tray=%ws, Form=%ws (%ld)\n", i,
                            wTrayName, wFormName, pDefForm->Index);
#endif

            if (FirstSel < 0) {

                SendDlgItemMessage(hDlg,
                                   IDD_TRAY_FORM_LB,
                                   LB_DELETESTRING,
                                   (WPARAM)i,
                                   (LPARAM)0);

                SendDlgItemMessage(hDlg,
                                   IDD_TRAY_FORM_LB,
                                   LB_INSERTSTRING,
                                   (WPARAM)i,
                                   (LPARAM)wBuf);

            } else {

                SendDlgItemMessage(hDlg,
                                   IDD_TRAY_FORM_LB,
                                   LB_INSERTSTRING,
                                   (WPARAM)-1,
                                   (LPARAM)wBuf);
            }

            SendDlgItemMessage(hDlg,
                               IDD_TRAY_FORM_LB,
                               LB_SETITEMDATA,
                               (WPARAM)i,
                               (LPARAM)dwData);

            if (iTray == i) {

                SendDlgItemMessage(hDlg,
                                   IDD_TRAY_FORM_LB,
                                   LB_SETCURSEL,
                                   (WPARAM)i,
                                   (LPARAM)0);

            }
        }
    }

    LockWindowUpdate(NULL);

    ShowCurTrayInfo(hDlg, pData);

}




int
ValidateDefTray(
    PDEFFORM    pDefForm,
    int         cTrays,
    WORD        FormIdx
    )

/*++

Routine Description:

    Validate all entry for the default form selection and correct them if not


Arguments:


    pDefForm    - Pointer to the DEFFORM data structure for current trays

    cTrays      - Total tray for this printer

    FormIdx     - The form index to be validate for trays


Return Value:

    int - Total tray matched for this form


Author:

    19-Dec-1994 Mon 17:13:23 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDEFFORM    pDefSel = NULL;
    PDEFFORM    p1stSel = NULL;
    int         cMatch = 0;
    int         i;



    for (i = 0; i < cTrays; i++, pDefForm++) {

#if DEF_TRAY_TEST
        DbgPrint("PSUI: Index=%2ld (%2ld) Select=%ld  ",
                    (DWORD)pDefForm->Index, (DWORD)FormIdx,
                    (DWORD)pDefForm->Select);
#endif

        if (pDefForm->Index == FormIdx) {

            if (++cMatch == 1) {

                p1stSel = pDefForm;
            }

            if (pDefForm->Select) {

#if DEF_TRAY_TEST
                DbgPrint(" ---> MATCH");
#endif

                if (pDefSel) {

                    //
                    // If multiple selections then make it default to none
                    //

                    pDefForm->Select =
                    pDefSel->Select  = 0;
                }

                pDefSel = pDefForm;
            }

        }

#if DEF_TRAY_TEST
        DbgPrint("\n");
#endif
    }

#if DEF_TRAY_TEST
    DbgPrint("PSUI: cMatch=%ld\n", (DWORD)cMatch);
#endif

    if ((cMatch == 1) && (p1stSel)) {


#if DEF_TRAY_TEST
        DbgPrint("PSUI: Only One, Select THIS ONE\n");
#endif
        //
        // Make sure we select this one
        //

        p1stSel->Select = 1;
    }

    return(cMatch);
}




DWORD
SaveTrayFormToReg(
    HWND        hDlg,
    PRINTDATA   *pData
    )

/*++

Routine Description:

    This function write the tray/form table/size to the registry

Arguments:

    hDlg    - Handle to the dialog box

    pData   - Pointer to the PRINTDATA data structure


Return Value:

    cbTrayForms write to the registry


Author:

    11-Jan-1995 Wed 12:36:00 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    WCHAR       *pw;
    WCHAR       *pwTable;
    PDEFFORM    pDefForm;
    FORM_INFO_1 *pForm;
    WCHAR       SlotName[MAX_SLOT_NAME];
    WCHAR       FormName[CCHFORMNAME];
    WCHAR       PrinterForm[CCHFORMNAME];
    DWORD       cbNeeded;
    DWORD       cbTrayForms;
    DWORD       cbEstimate;
    int         i;
    int         cTrays;



    // output the corresponding tray - form pairs to the
    // registry.

    cTrays = SendDlgItemMessage(hDlg,
                                IDD_TRAY_FORM_LB,
                                LB_GETCOUNT,
                                (WPARAM)0,
                                (LPARAM)0);

    //
    // This is the maximum estimated size of the traytable size, the 2 are one
    // for dobule NULL terminator and size sanity checking WORD
    //

    cbEstimate = (((MAX_SLOT_NAME + CCHFORMNAME + CCHFORMNAME) * cTrays) + 2) *
                 sizeof(WCHAR);

    if (!(pwTable = (WCHAR *)LocalAlloc(LPTR, cbEstimate))) {

        RIP("PSCRPTUI!PrtPropDialogProc: LocalAlloc pwTable failed.\n");
        return(0);
    }

    //
    // Now go through each tray and make sure we have everything done
    //

    for (i = 0, cbTrayForms = 2, pDefForm = pData->pDefForm, pw = pwTable + 1;
         i < cTrays;
         i++, pDefForm++) {

        SendDlgItemMessage(hDlg,
                           IDD_TRAY_LIST_BOX,
                           CB_GETLBTEXT,
                           (WPARAM)i,
                           (LPARAM)SlotName);
        SendDlgItemMessage(hDlg,
                           IDD_PAPER_LIST_BOX,
                           CB_GETLBTEXT,
                           (WPARAM)pDefForm->Index,
                           (LPARAM)FormName);

        //
        // now that we have the selected form name match it to a form which
        // the printer supports, and save the printer form name as well.
        //

        GetForm(pData->hPrinter, FormName, 1, NULL, 0, &cbNeeded);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
#if DBG
            DbgPrint("PSCRPTUI!PrinterPropDlgProc: GetForm returns error %x\n",
                        GetLastError());
#endif
            continue;
        }

        if (!(pForm = (FORM_INFO_1 *)LocalAlloc(LPTR, cbNeeded))) {

            continue;
        }

        GetForm(pData->hPrinter,
                FormName,
                1,
                (BYTE *)pForm,
                cbNeeded,
                &cbNeeded);

        GetPrinterForm(pData->pntpd, pForm, PrinterForm);

        LocalFree((HLOCAL)pForm);

        wcscpy(pw, SlotName);
        pw          += (cbNeeded = (wcslen(SlotName) + 1));
        cbTrayForms += cbNeeded;

        wcscpy(pw, FormName);
        pw          += (cbNeeded = (wcslen(FormName) + 1));
        cbTrayForms += cbNeeded;

        wcscpy(pw, PrinterForm);
        pw          += (cbNeeded = (wcslen(PrinterForm) + 1));
        cbTrayForms += cbNeeded;

        *pw++ = (WCHAR)pDefForm->Select;
        cbTrayForms++;

#if DEF_TRAY_TEST
        DbgPrint("*%2ld[%3ld:%ld] %ws-%ws-%ws\n",
                    (DWORD)i, (DWORD)pDefForm->Index, (DWORD)pDefForm->Select,
                    SlotName, FormName, PrinterForm);
#endif
    }

    //
    // Adding DOUBLE NULL terminator and set the sanity checking size WORD
    //

    *pw      = L'\0';
    *pwTable = (WORD)(cbTrayForms *= sizeof(WCHAR));

#if DEF_TRAY_TEST
    DbgPrint("** cbEstimate=%ld, cbTrayForms=%ld\n", cbEstimate, cbTrayForms);

    if (cbTrayForms > cbEstimate) {

        RIP("PSCRPTUI!SetTrayFormReg: FATAL (cbTrayForms > cbEstimate)\n");
    }
#endif

    //
    // now output the tray-form table to the registry.

    LoadString(hModule,
              IDS_TRAY_FORM_TABLE,
              SlotName,
              (sizeof(SlotName) / sizeof(SlotName[0])));

    SetPrinterData(pData->hPrinter,
                   SlotName,
                   REG_BINARY,
                   (LPBYTE)pwTable,
                   cbTrayForms);

    //
    // now output the table size to the registry.
    //

    LoadString(hModule,
               IDS_TRAY_FORM_SIZE,
               SlotName,
               (sizeof(SlotName) / sizeof(SlotName[0])));

    SetPrinterData(pData->hPrinter,
                   SlotName,
                   REG_DWORD,
                   (LPBYTE)&cbTrayForms,
                   sizeof(cbTrayForms));

    //
    // Free up buffer.
    //

    LocalFree((HLOCAL)pwTable);

    return(cbTrayForms);
}





VOID
SetTrayText(
    HWND        hDlg,
    PTRAYLIST   pTrayList
    )

/*++

Routine Description:


    This function set the tray name on form change dialog box


Arguments:

    hDlg        - Handle to the dialog box

    pTrayList   - Pointer to the TRAYLIST data structure


Return Value:

    NONE

Author:

    11-Jan-1995 Wed 13:14:04 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    WCHAR   Buf1[80];
    WCHAR   Buf2[128];
    WCHAR   TrayName[MAX_SLOT_NAME];
    LONG    i;


    i = SendDlgItemMessage(pTrayList->hDlgParent,
                           IDD_TRAY_FORM_LB,
                           LB_GETCURSEL,
                           (WPARAM)0,
                           (LPARAM)0);

    SendDlgItemMessage(pTrayList->hDlgParent,
                       IDD_TRAY_LIST_BOX,
                       CB_GETLBTEXT,
                       (WPARAM)i,
                       (LPARAM)TrayName);

    GetWindowText(GetDlgItem(hDlg, IDD_FORM_TRAY_NAME),
                  Buf1,
                  sizeof(Buf1) / sizeof(Buf1[0]));

    wsprintf(Buf2, Buf1, TrayName);

    SetWindowText(GetDlgItem(hDlg, IDD_FORM_TRAY_NAME), Buf2);
}


LONG
FormToTrayDlgProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    This is the form selection dialog proc


Arguments:

    Per windows


Return Value:

    -1 if not form selected or cancel, else is the form index


Author:

    06-Jan-1995 Fri 12:39:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PTRAYLIST   pTrayList;
    WCHAR       FormName[CCHFORMNAME + 2];
    LONG        Idx;
    LONG        i;


    switch (Msg) {

    case WM_INITDIALOG:

        if (!(pTrayList = (PTRAYLIST)lParam)) {

            RIP("PSCRPTUI!PrtPropDlgProc: null pTrayList.\n");
            return(FALSE);
        }

        //
        // save the PRINTDATA.
        //

        SetWindowLong(hDlg, GWL_USERDATA, (LONG)lParam);


        //
        // put the form in the list box if it is valid for this printer.
        //

        SetTrayText(hDlg, pTrayList);


        i = 0;

        while (SendDlgItemMessage(pTrayList->hDlgParent,
                                  IDD_PAPER_LIST_BOX,
                                  CB_GETLBTEXT,
                                  (WPARAM)i++,
                                  (LPARAM)FormName) != CB_ERR) {

            //
            // add the form to the list box.
            //

            SendDlgItemMessage(hDlg,
                               IDD_PAPER_LIST_BOX,
                               LB_INSERTSTRING,
                               (WPARAM)-1,
                               (LPARAM)FormName);
        }

        SendDlgItemMessage(hDlg,
                           IDD_PAPER_LIST_BOX,
                           LB_SETCURSEL,
                           (WPARAM)pTrayList->FormIdx,
                           0L);

        //
        // intialize the help stuff.
        //

        vHelpInit();

        return(TRUE);

    case WM_COMMAND:

        pTrayList = (PTRAYLIST)GetWindowLong(hDlg, GWL_USERDATA);

        switch (LOWORD(wParam)) {

        case IDD_PAPER_LIST_BOX:

            if (HIWORD(wParam) != LBN_DBLCLK) {

                break;
            }

        case IDOK:

            Idx = SendDlgItemMessage(hDlg,
                                     IDD_PAPER_LIST_BOX,
                                     LB_GETCURSEL,
                                     (WPARAM)0,
                                     (LPARAM)0);

            EndDialog(hDlg, (Idx == pTrayList->FormIdx) ? -1 : Idx);

            return(TRUE);

        case IDCANCEL:

            EndDialog(hDlg, -1);
            return(TRUE);

        default:

            break;
        }

        break;

    case WM_DESTROY:

        //
        // clean up any used help stuff.
        //

        vHelpDone(hDlg);
        return(TRUE);

        break;
    }

    return(FALSE);
}




//--------------------------------------------------------------------------
// BOOL PrinterProperties(HWND  hwnd, LPPRINTER lpPrinter)
//
// This function first retrieves and displays the current set of printer
// properties for the printer.  The user is allowed to change the current
// printer properties from the displayed dialog box.
//
// Returns:
//   This function returns -1L if it fails in any way.  If the dialog is
//   actually displayed, it returns either IDOK or IDCANCEL, depending on
//   what the user chose.   If no dialog box was displayed and the function
//   is successful, IDOK is returned.
//
// History:
//   11-Jul-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL PrinterProperties(HWND  hwnd, HANDLE hPrinter)
{
    int         nResult;
    PRINTDATA   printdata;
    WCHAR       wcbuf[32];
    DWORD       dwTmp;
    DWORD       rc;

    // fill in PRINTDATA structure.

    printdata.hPrinter        = hPrinter;
    printdata.iFreeMemory     = 0;
    printdata.pntpd           = (PNTPD)NULL;
    printdata.pDefForm        = NULL;
    printdata.bHostHalftoning = TRUE;

    // simply try to write out to the registry to see if the caller
    // has permission to change anything.

    LoadString(hModule, IDS_PERMISSION, wcbuf,
               (sizeof(wcbuf) / sizeof(wcbuf[0])));

    dwTmp = 1;

    rc = SetPrinterData(hPrinter, wcbuf, REG_DWORD, (LPBYTE)&dwTmp,
                        sizeof(DWORD));

    if (rc == ERROR_SUCCESS)
        printdata.bPermission = TRUE;
    else
        printdata.bPermission = FALSE;

    // call the printer properties dialog routine.

    nResult = DialogBoxParam (hModule, MAKEINTRESOURCE(PRINTER_PROP), hwnd,
                              (DLGPROC)PrtPropDlgProc, (LPARAM)&printdata);

    return(nResult);
}


//--------------------------------------------------------------------------
// LONG APIENTRY AboutDlgProc (HWND hDlg, UINT message, DWORD wParam,
//                             LONG lParam)
//
// This function processes messages for the "About" dialog box.
//
// Returns:
//   This function returns TRUE if successful, FALSE otherwise.
//
// History:
//   11-Jul-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

LONG AboutDlgProc(
    HWND    hDlg,
    UINT    message,
    DWORD   wParam,
    LONG    lParam
)
{
    UNREFERENCED_PARAMETER (lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            return (TRUE);

        case WM_COMMAND:
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                EndDialog (hDlg, IDOK);
                return (TRUE);
            }
            break;
    }
    return (FALSE);
}


//--------------------------------------------------------------------------
// LONG APIENTRY PrtPropDlgProc(HWND hwnd, UINT usMsg, DWORD wParam,
//                              LONG  lParam)
//
// This function processes messages for the "About" dialog box.
//
// Returns:
//   This function returns TRUE if successful, FALSE otherwise.
//
// History:
//   11-Jul-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

LONG APIENTRY PrtPropDlgProc(HWND hwnd, UINT usMsg, DWORD wParam,
                             LONG  lParam)
{
    int             i;
    int             iSelect;
    WCHAR           wcbuf1[SMALL_BUF];
    WCHAR           wcbuf2[MAX_FONTNAME * 2];
    WCHAR           wcbuf3[MAX_FONTNAME];
    DWORD           cbNeeded;
    int             cTrays;
    int             iForm;
    int             iTray;
    LONG            lReturn;
    BOOL            bTmp;
    DWORD           dwData;
    PRINTDATA      *pdata;
    PDEFFORM        pDefForm;
    TRAYLIST        TrayList;




    switch (usMsg)
    {
        case WM_INITDIALOG:
            pdata = (PRINTDATA *)lParam;

            if (!pdata)
            {
                RIP("PSCRPTUI!PrtPropDlgProc: null pdata.\n");
                return(FALSE);
            }

            // save the PRINTDATA.

            SetWindowLong(hwnd, GWL_USERDATA, lParam);

            if (!(pdata->pntpd = MapPrinter(pdata->hPrinter)))
            {
                RIP("PSCRPTUI!PrtPropDlgProc: MapPrinter failed.\n");
                return(FALSE);
            }


    //
    // First read in the current DEVHTINFO data
    //
    // !!! If you can read the DEVHTINFO from MINI DRIVERS, then pass that
    // !!! pointer to the GetDeviceHTData(), the DEVHTINFO is at end of
    // !!! winddi.h, it includes COLORINFO, DevPelSize, HTPatternSize and
    // !!! so on.  If a NULL is passed then standard halftone default will
    // !!! be used to set in the registry at first time
    //

            vGetDeviceHTData(pdata->hPrinter, NULL);


            // fill in the default printer memory configuration.  first
            // check to see if a value has been saved in the registry.
            // if so, use it, otherwise, get the default value from
            // the PPD file.

            LoadString(hModule, IDS_FREEMEM,
                       wcbuf2, (sizeof(wcbuf2) / sizeof(wcbuf2[0])));

            if (GetPrinterData(pdata->hPrinter,
                               wcbuf2,
                               &Type,
                               (LPBYTE)&pdata->iFreeMemory,
                               sizeof(pdata->iFreeMemory),
                               &cbNeeded) != NO_ERROR) {

                pdata->iFreeMemory = pdata->pntpd->cbFreeVM;
            }

            SetDlgItemInt(hwnd, IDD_MEMORY_EDIT_BOX, pdata->iFreeMemory, FALSE);

            // initialize the printer model name.

            SetDlgItemText(hwnd, IDD_MODEL_TEXT,
                           (PWSTR)((PSTR)pdata->pntpd + pdata->pntpd->lowszPrinterName));

            // initialize the paper tray and paper size list boxes.

            if (!InitComboBoxes(hwnd, pdata))
                RIP("PrtPropDlgProc: InitComboBoxes failed.\n");


            // Get the current setting of the PS_HALFTONING flag from the
            // registry and initialize the check button.

            LoadString(hModule, IDS_HALFTONE,
                       wcbuf3, (sizeof(wcbuf3) / sizeof(wcbuf3[0])));

            if (GetPrinterData(pdata->hPrinter,
                               wcbuf3,
                               &Type,
                               (LPBYTE)&(pdata->bHostHalftoning),
                               sizeof(pdata->bHostHalftoning),
                               &cbNeeded) != NO_ERROR) {

                //
                // printer halftoning is ON by default.  ie, use printer halftoning.
                //

                pdata->bHostHalftoning = FALSE;
            }

            CheckDlgButton(hwnd,
                           IDD_USE_HOST_HALFTONING,
                           !((pdata->bHostHalftoning)));

            // if the user has selected printer halfoning, disable the system halftoning
            // push button.

            if (pdata->bHostHalftoning)
                EnableWindow(GetDlgItem(hwnd, IDD_HALFTONE_PUSH_BUTTON), TRUE);
            else
                EnableWindow(GetDlgItem(hwnd, IDD_HALFTONE_PUSH_BUTTON), FALSE);

            // intialize the help stuff.

            vHelpInit();

            // disable a bunch of stuff if the user does not have
            // permission to change it.

            if (!pdata->bPermission)
            {
                EnableWindow(GetDlgItem(hwnd, IDD_DRAWPAPER_CHKBOX), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDD_FORMSEL_PUSH), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDD_MEMORY_EDIT_BOX), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDD_USE_HOST_HALFTONING), FALSE);
            }
            return TRUE;
            break;

        case WM_COMMAND:
            pdata = (PRINTDATA *)GetWindowLong(hwnd, GWL_USERDATA);

            switch (LOWORD(wParam))
            {
                case IDD_ABOUT_BUTTON:
                    return(bAbout(hwnd, pdata->pntpd));

                case IDD_FONTS_BUTTON:

                    #if 0
                    lReturn = DialogBoxParam (hModule, MAKEINTRESOURCE(FONTINST), hwnd,
                                              (DLGPROC)FontInstDlgProc,
                                              (LPARAM)pdata);
                    if (lReturn == -1)
                    {
                        RIP("PSCRPTUI!PrtPropDlgProc: DialogBoxParam for FONTINST failed.\n");
                        return(FALSE);
                    }
                    #endif

                    return (TRUE);

                case IDD_FONT_SUBST_PUSH_BUTTON:
                    lReturn = DialogBoxParam(hModule, MAKEINTRESOURCE(IDD_TT_DIALOG),
                                             hwnd, (DLGPROC)TTFontDialogProc,
                                             (LPARAM)pdata);

                    if (lReturn == -1)
                    {
                        RIP("PSCRPTUI!PrtPropDlgProc: DialogBoxParam for TTFONTS failed.\n");
                        return(FALSE);
                    }

                    return (TRUE);

                //
                // Button for toggling host versus device halftoning mode.
                // -- DJS
                case IDD_USE_HOST_HALFTONING:

                    if(IsDlgButtonChecked(hwnd, IDD_USE_HOST_HALFTONING))
                    {
                        pdata->bHostHalftoning = FALSE;
                        EnableWindow(GetDlgItem(hwnd, IDD_HALFTONE_PUSH_BUTTON),
                                                FALSE);
                    }
                    else
                    {
                        pdata->bHostHalftoning = TRUE;
                        EnableWindow(GetDlgItem(hwnd, IDD_HALFTONE_PUSH_BUTTON),
                                                TRUE);
                    }

                    break;

                   return(TRUE);

                case IDD_HALFTONE_PUSH_BUTTON:

                    vDoDeviceHTDataUI((LPWSTR)((PSTR)pdata->pntpd +
                                               pdata->pntpd->lowszPrinterName),
                                      (pdata->pntpd->flFlags & COLOR_DEVICE),
                                      pdata->bPermission);

                    return(TRUE);

                case IDD_TRAY_FORM_LB:

                    if (HIWORD(wParam) == LBN_SELCHANGE) {

                        ShowCurTrayInfo(hwnd, pdata);
                    }

                    if ((!pdata->bPermission) ||
                        (HIWORD(wParam) != LBN_DBLCLK)) {


                        break;
                    }

                case IDD_FORMSEL_PUSH:

                    TrayList.pData      = pdata;
                    TrayList.hDlgParent = hwnd;
                    TrayList.TrayIdx    = (WORD)SendDlgItemMessage(
                                                    hwnd,
                                                    IDD_TRAY_FORM_LB,
                                                    LB_GETCURSEL,
                                                    (WPARAM)0,
                                                    (LPARAM)0);
                    dwData = (DWORD)SendDlgItemMessage(hwnd,
                                                       IDD_TRAY_FORM_LB,
                                                       LB_GETITEMDATA,
                                                       (WPARAM)TrayList.TrayIdx,
                                                       (LPARAM)0);
                    TrayList.FormIdx = DW2FORMIDX(dwData);

                    iForm = DialogBoxParam(hModule,
                                           MAKEINTRESOURCE(FORMSEL_DIALOG),
                                           hwnd,
                                           (DLGPROC)FormToTrayDlgProc,
                                           (LPARAM)&TrayList);

                    if (iForm != -1) {

                        cTrays = SendDlgItemMessage(hwnd,
                                                    IDD_TRAY_FORM_LB,
                                                    LB_GETCOUNT,
                                                    (WPARAM)0,
                                                    (LPARAM)0);


                        pDefForm                          = pdata->pDefForm;
                        pDefForm[TrayList.TrayIdx].Index  = (WORD)iForm;
                        pDefForm[TrayList.TrayIdx].Select = 0;

                        //
                        // Validated the old form then the new form
                        //

                        ValidateDefTray(pDefForm, cTrays, TrayList.FormIdx);
                        ValidateDefTray(pDefForm, cTrays, (WORD) iForm);

                        SetTrayFormName(hwnd, pdata, -1);
                    }

                    break;

                case IDD_DRAWPAPER_CHKBOX:

                    if (HIWORD(wParam) == BN_CLICKED) {

                        pDefForm = pdata->pDefForm;

                        cTrays = SendDlgItemMessage(hwnd,
                                                    IDD_TRAY_FORM_LB,
                                                    LB_GETCOUNT,
                                                    (WPARAM)0,
                                                    (LPARAM)0);
                        iTray = SendDlgItemMessage(hwnd,
                                                   IDD_TRAY_FORM_LB,
                                                   LB_GETCURSEL,
                                                   (WPARAM)0,
                                                   (LPARAM)0);

                        iForm = pDefForm[iTray].Index;

                        ClearDefTray(pDefForm, cTrays, (WORD) iForm);

                        if (IsDlgButtonChecked(hwnd, IDD_DRAWPAPER_CHKBOX)) {

                            pDefForm[iTray].Select = 1;
                        }

                        SetTrayFormName(hwnd, pdata, -1);
                    }

                    break;

                case IDD_HELP_BUTTON:

                    vShowHelp(hwnd, HELP_CONTEXT, HLP_PRINTER_SETUP,
                              pdata->hPrinter);
                    return(TRUE);

                case IDOK:

                    if (pdata->bPermission) {

                        // get the new value from the memory edit box.

                        i = GetDlgItemInt(hwnd,
                                          IDD_MEMORY_EDIT_BOX,
                                          &bTmp,
                                          FALSE);

                        //
                        // if the user has changed the memory setting,
                        // pop up a message box.

                        if (i != (int)pdata->iFreeMemory) {

                            // warn the user that changing the memory setting
                            // is dangerous.

                            LoadString(hModule,
                                       IDS_MEMWARN,
                                       (LPWSTR)wcbuf2,
                                       (sizeof(wcbuf2) / sizeof(wcbuf2[0])));

                            LoadString(hModule,
                                       IDS_CAUTION,
                                       (LPWSTR)wcbuf1,
                                       (sizeof(wcbuf1) / sizeof(wcbuf1[0])));

                            iSelect = MessageBox(hwnd, wcbuf2, wcbuf1,
                                                 MB_DEFBUTTON2 |
                                                 MB_ICONEXCLAMATION | MB_YESNO);

                            if (iSelect == IDYES) {

                                pdata->iFreeMemory = i;

                            } else {

                                // reset the memory edit box.

                                SendDlgItemMessage(hwnd, IDD_MEMORY_EDIT_BOX,
                                                   EM_SETSEL, 0, 0x7FFF0000);

                                SetDlgItemInt(hwnd, IDD_MEMORY_EDIT_BOX,
                                              pdata->iFreeMemory, FALSE);

                                break;
                            }
                        }

                        // output the free memory to the .INI file.

                        LoadString(hModule,
                                   IDS_FREEMEM,
                                   wcbuf2,
                                   (sizeof(wcbuf2) / sizeof(wcbuf2[0])));

                        if (SetPrinterData(pdata->hPrinter,
                                           wcbuf2, REG_BINARY,
                                           (LPBYTE)&pdata->iFreeMemory,
                                           sizeof(pdata->iFreeMemory))) {

#if DBG
                            RIP("PSCRPTUI!PrtPropDlgProc: SetPrinterData FreeMem failed.\n");
#endif
                        }

                        // Set Halftone flag in Registry

                        LoadString(hModule,
                                   IDS_HALFTONE,
                                   wcbuf3,
                                   (sizeof(wcbuf3) / sizeof(wcbuf3[0])));

                        SetPrinterData(pdata->hPrinter,
                                       wcbuf3,
                                       REG_BINARY,
                                       (LPBYTE)&(pdata->bHostHalftoning),
                                       sizeof(pdata->bHostHalftoning));


                        SaveTrayFormToReg(hwnd, pdata);

                        //
                        // save that halftone device data back into registry
                        //

                        bSaveDeviceHTData(pdata->hPrinter, FALSE);
                    }

                    if (pdata->pntpd)
                    {
                        GlobalFree((HGLOBAL)pdata->pntpd);
                        pdata->pntpd = NULL;
                    }

                    if (pdata->pDefForm)
                    {
                        GlobalFree((HGLOBAL)pdata->pDefForm);
                        pdata->pDefForm = NULL;
                    }

                    EndDialog (hwnd, IDOK);
                    return TRUE;

                case IDCANCEL:
                    if (pdata->pntpd)
                    {
                        GlobalFree((HGLOBAL)pdata->pntpd);
                        pdata->pntpd = NULL;
                    }

                    if (pdata->pDefForm)
                    {
                        GlobalFree((HGLOBAL)pdata->pDefForm);
                        pdata->pDefForm = NULL;
                    }

                    EndDialog (hwnd, IDCANCEL);
                    return TRUE;

                default:
                    return (FALSE);
            }
            break;

        case WM_DESTROY:
            // clean up any used help stuff.

            vHelpDone(hwnd);
            return (TRUE);

        default:
            return (FALSE);   /* didn't process the message */
    }
}


//--------------------------------------------------------------------------
// BOOL InitComboBoxes(hwnd, pdata)
// HWND       hwnd;        // handle to dialog window
// PRINTDATA *pdata;
//
// Parameters
//
// Returns:
//   This routine returns TRUE for success, FALSE otherwise.
//
// History:
//   10-Apr-1991    -by-    Kent Settle     (kentse)
// Made it use PNTPD instead of PPRINTER.
//   21-Jan-1991    -by-    Kent Settle     (kentse)
// Brought in from Windows 3.1, modified for NT, and clean up.
//
//  21-Jul-1994 Thu 17:19:21 updated  -by-  Daniel Chou (danielc)
//      Fixed for bug 1986
//--------------------------------------------------------------------------

BOOL InitComboBoxes(hwnd, pdata)
HWND        hwnd;        // handle to dialog window
PRINTDATA  *pdata;
{
    WCHAR           SlotName[MAX_SLOT_NAME];
    WCHAR           SearchName[MAX_SLOT_NAME + 4];
    WCHAR           FormName[CCHFORMNAME];
    WCHAR           wcbuf[MAX_FONTNAME];
    DWORD           i, j, dwType, cbTrayForm;
    DWORD           cbNeeded, dwTmp, count;
    int             rc;
    int             DefaultForm;
    int             TabStop[4];
    int             DefaultTray;
    int             Len;
    PSFORM         *pPSForm;
    FORM_INFO_1    *pFormI1, *pFormI1Save;
    FORM_INFO_1    *pdbForm, *pdbForms;
    PWSTR           pwstrName, pwstrSave;
    PSINPUTSLOT    *pslots;
    PNTPD           pntpd;
    PDEFFORM        pDefForm;
    PSTR            pstr, pstrSave, pstrDefault;
    BOOL            bFound;
    BOOL            ForceUpdate = FALSE;


    // clear out the tray and form list boxes before we begin.

    SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX, CB_RESETCONTENT, 0, 0L);
    SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_RESETCONTENT, 0, 0L);
    SendDlgItemMessage(hwnd, IDD_TRAY_FORM_LB, LB_RESETCONTENT, 0, 0L);

    TabStop[0] = 92;
    TabStop[1] = 94;
    TabStop[2] = 96;
    TabStop[3] = 98;

    SendDlgItemMessage(hwnd,
                       IDD_TRAY_FORM_LB,
                       LB_SETTABSTOPS,
                       (WPARAM)4,
                       (LPARAM)TabStop);

    // limit the length of the strings allowed.

    SendDlgItemMessage(hwnd,
                       IDD_PAPER_LIST_BOX,
                       CB_LIMITTEXT,
                       (WPARAM)CCHFORMNAME,
                       (LPARAM)0);

    // here is a brief overview of how we will fill in the forms list
    // box:  firstly, call AddForm for each form in the ppd file, if
    // this has not been done yet.  then enumerate the forms database.
    // then for each form in the database, see if there is a form
    // defined in the PPD file which is large enough to print on.  if
    // there is, this form gets added to the list box.

    // start out by setting up a FORM_INFO_1 structure for each form
    // defined in the NTPD structure.

    // point to the start of the array of PSFORM structures within
    // the NTPD structure.

    pntpd = pdata->pntpd;

    pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

    // allocate memory for all the FORM_INFO_1 structures.

    if (!(pFormI1 = (FORM_INFO_1 *)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                               (pntpd->cPSForms *
                                               sizeof(FORM_INFO_1)))))
    {
        RIP("PSCRPTUI!InitComboBoxes: GlobalAlloc for pFormI1 failed.\n");
        return(FALSE);
    }

    // keep a copy of the pointer to the first structure.

    pFormI1Save = pFormI1;

    // allocate memory for the form name buffers.

    if (!(pwstrName = (PWSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                            (pntpd->cPSForms * CCHFORMNAME * sizeof(WCHAR)))))
    {
        RIP("PSCRPTUI:InitComboBoxes: GlobalAlloc for pwstrName failed.\n");
        GlobalFree((HGLOBAL)pFormI1);
        return(FALSE);
    }

    // keep a copy of the pointer to the first form name.

    pwstrSave = pwstrName;

    // add each form defined in the NTPD structure to the forms
    // database.  AddForm should simply do nothing if this form
    // already exists in the database.

    for (i = 0; i < pntpd->cPSForms; i++)
    {
        // get the form name from the PSFORM structure and convert it
        // to UNICODE.  you might think "hey, you should store the
        // name as UNICODE in the PSFORM structure!"  however, the
        // PSFORM structures get calculated on every EnablePDEV, which
        // will probably happen much more often then this dialog gets
        // displayed.

        // check to see if there is a translation string to worry about,
        // for each form.

        pstrSave = (CHAR *)pntpd + pPSForm->loFormName;

        XLATESTRING(pstr);

        // pstr now points to the form name we want.

		MultiByteToWideChar(CP_ACP, 0, pstr, -1, pwstrName, CCHFORMNAME-1);
		pwstrName[CCHFORMNAME-1] = (WCHAR) '\0';

        pFormI1->pName = pwstrName;

        // copy the form size and imageable area.
        // the PSFORM structure stores the form size and the
        // imageable area in postscript USER coordinate (1/72 inch).
        // the forms database stores these values in .001mm.

        pFormI1->Size.cx = USERTO001MM(pPSForm->sizlPaper.cx);
        pFormI1->Size.cy = USERTO001MM(pPSForm->sizlPaper.cy);

        pFormI1->ImageableArea.left = USERTO001MM(pPSForm->imagearea.left);
        pFormI1->ImageableArea.top = USERTO001MM(pPSForm->sizlPaper.cy) -
                                     USERTO001MM(pPSForm->imagearea.top);
        pFormI1->ImageableArea.right = USERTO001MM(pPSForm->imagearea.right);
        pFormI1->ImageableArea.bottom = USERTO001MM(pPSForm->sizlPaper.cy) -
                                        USERTO001MM(pPSForm->imagearea.bottom);

        // point to the next PSFORM structure in the NTPD.  point to the
        // next FORM_INFO_1 structure we are filling in.  point to the next
        // form name buffer.

        pPSForm++;
        pFormI1++;
        pwstrName += CCHFORMNAME;
    }

    LoadString(hModule, IDS_FORMS, SearchName,
               (sizeof(SearchName) / sizeof(SearchName[0])));

    rc = GetPrinterData(pdata->hPrinter, SearchName, &dwType, (PBYTE)&dwTmp,
                        sizeof(dwTmp), &cbNeeded);

    // if we have marked in the registry that we have already added all
    // the forms for this printer to the forms database, then we do not
    // need to do it again.

    if (rc != NO_ERROR)
    {
        // add each form defined in the NTPD structure to the forms
        // database.  AddForm should simply do nothing if this form
        // already exists in the database.

        pFormI1 = pFormI1Save;

        for (i = 0; i < pntpd->cPSForms; i++) {

            bFound = AddForm(pdata->hPrinter, 1, (PBYTE)pFormI1++);
#if 0
            DbgPrint("\nPSUI: AddForm: %ws [%ld]",
                            (pFormI1 - 1)->pName, (DWORD)bFound);
#endif
        }

        // mark that we have added all the forms to the forms database
        // for this printer.

        dwTmp = 1;
        SetPrinterData(pdata->hPrinter, SearchName, REG_DWORD, (PBYTE)&dwTmp,
                       sizeof(dwTmp));
    }

    // we now know that all of the forms in the NTPD structure have been
    // added to the forms database.  now enumerate the database.

    if (!(pdbForms = GetFormsDataBase(pdata->hPrinter, &count, pntpd)))
    {
        RIP("PSCRPTUI!InitComboBoxes: GetFormsDataBase failed.\n");

        GlobalFree((HGLOBAL)pwstrSave);
        GlobalFree((HGLOBAL)pFormI1Save);

        return(FALSE);

    } else {

        //
        // put the form in the list box if it is valid for this printer.
        //

        for (pdbForm = pdbForms, i = 0; i < count; i++, pdbForm++) {

            if (pdbForm->Flags & PSCRIPT_VALID_FORM) {

                //
                // add the form to the list box.
                //

                wcsncpy(FormName, pdbForm->pName, CCHFORMNAME - 1);
                FormName[CCHFORMNAME - 1] = L'\0';

                SendDlgItemMessage(hwnd,
                                   IDD_PAPER_LIST_BOX,
                                   CB_INSERTSTRING,
                                   (WPARAM)-1,
                                   (LPARAM)FormName);
            }
        }
    }

    // free up resources.

    GlobalFree((HGLOBAL)pdbForms);
    GlobalFree((HGLOBAL)pwstrSave);
    GlobalFree((HGLOBAL)pFormI1Save);

    // if the only inputslot defined is the default inputslot, cInputSlots
    // will be zero.  in this case, let's simply let the user know that
    // this is the only paper tray.

    // NOTE!  CB_INSERTSTRING is used rather that CB_ADDSTRING.  This will
    // keep the order of the paper trays the same as in the PPD file, and
    // the same as Win31.  CB_ADDSTRING would sort the list.

    DefaultTray = 0;

    if (pntpd->cInputSlots == 0) {

        LoadString(hModule, SLOT_ONLYONE,
                   SlotName, (sizeof(SlotName) / sizeof(SlotName[0])));

        rc = SendDlgItemMessage(hwnd,
                                IDD_TRAY_LIST_BOX,
                                CB_INSERTSTRING,
                                (WPARAM)-1,
                                (LPARAM)SlotName);

    } else {

        // add each input slot defined in the NTPD to the list box.

        pslots      = (PSINPUTSLOT *)((CHAR *)pntpd + pntpd->loPSInputSlots);
        pstrDefault = (CHAR *)pntpd + pntpd->loDefaultSlot;
        Len         = strlen(pstrDefault);

        for (i = 0; i < (DWORD)pntpd->cInputSlots; i++, pslots++) {

            // check to see if there is a translation string to worry about,
            // for each form.

            pstrSave = (CHAR *)pntpd + pslots->loSlotName;

            XLATESTRING(pstr);

            strncpy2WChar(SlotName, pstr, MAX_SLOT_NAME);

            rc = SendDlgItemMessage(hwnd,
                                    IDD_TRAY_LIST_BOX,
                                    CB_INSERTSTRING,
                                    (WPARAM)-1,
                                    (LPARAM)SlotName);

            if ((pstrSave != pstr)          &&
                ((pstr - pstrSave) > Len)   &&
                (strnicmp(pstrSave, pstrDefault, Len) == 0)) {

                //
                // highlight the default paper tray, and the corresponding
                // form. get the default paper tray from the NTPD structure.
                //

                DefaultTray = rc;
            }
        }
    }

    // check to see if the current printer supports manual feed.  to do
    // this, check to see if a string to turn on manual feed was found.

    if (pntpd->loszManualFeedTRUE != 0)
    {
        // this printer does support manual feed, so insert this as the
        // last choice in the list box.

        LoadString(hModule, SLOT_MANUAL,
                   SlotName, (sizeof(SlotName) / sizeof(SlotName[0])));

        rc = SendDlgItemMessage(hwnd,
                                IDD_TRAY_LIST_BOX,
                                CB_INSERTSTRING,
                                (WPARAM)-1,
                                (LPARAM)SlotName);
    }

    // all of the paper trays have been added to the list box.  now
    // associate each paper tray with a form.  this will be done in the
    // following manner:  for each paper tray, see if an entry exists in
    // the registry (eg, "TrayUpper Letter").  if it does, use the form
    // specified there, and associate the tray with the index into the
    // form list box.  if no entry exists in the registry for the tray,
    // associate it with the default form.

    // first, find the index to the default form.

    GrabDefaultFormName(hModule, FormName);
    DefaultForm = SendDlgItemMessage(hwnd,
                                     IDD_PAPER_LIST_BOX,
                                     CB_FINDSTRING,
                                     (WPARAM)-1,
                                     (LPARAM)FormName);

    // if there was a problem finding the default form, use the first one
    // in the list box.

    if (DefaultForm == CB_ERR)
        DefaultForm = 0;

    // now see if a registry entry exists for each tray, and associate it
    // with the specified form if so.

    count = SendDlgItemMessage(hwnd,
                               IDD_TRAY_LIST_BOX,
                               CB_GETCOUNT,
                               (WPARAM)0,
                               (LPARAM)0);

    // initialize the list of forms selected by the user to be in each
    // paper tray.  fill each form index with -1, then fill in the default
    // tray with the default form index.

    // allocate memory for all trays.

    if (!(pdata->pDefForm = (PDEFFORM)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                                  (count * sizeof(DEFFORM)))))
    {
        RIP("PSCRPTUI!InitComboBoxes: GlobalAlloc for pDefForm failed.\n");
        return(FALSE);
    }

    //
    // Initialize to Index = default form and non of them selected
    //

    for (i = 0, pDefForm = pdata->pDefForm; i < count; i++, pDefForm++) {

        pDefForm->Index  = (WORD)DefaultForm;
        pDefForm->Select = 0;
    }

    // point back to first entry.

    // first find out the size of the tray-form buffer and copy it from
    // the registry.

    // now get the table size from the registry.

#if 0
    LoadString(hModule,
               IDS_TRAY_FORM_TABLE,
               wcbuf,
               (sizeof(wcbuf) / sizeof(wcbuf[0])));

    cbNeeded = 1;

    rc = GetPrinterData(pdata->hPrinter,
                        wcbuf,
                        &dwType,
                        (LPBYTE)cbNeeded,
                        cbNeeded,
                        &cbNeeded);

    DbgPrint("\nPSUI: GetPrinterData(NULL), rc=%ld, cbNeeded=%ld, dwType=%ld, LastErr=%ld\n",
                rc, cbNeeded, dwType, GetLastError());
#endif

    LoadString(hModule, IDS_TRAY_FORM_SIZE,
              wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

    cbTrayForm = 0;

    if (GetPrinterData(pdata->hPrinter,
                       wcbuf,
                       &dwType,
                       (LPBYTE)&cbTrayForm,
                       sizeof(cbTrayForm),
                       &cbNeeded) != ERROR_SUCCESS) {

        cbTrayForm = 0;
    }

#if 0
    DbgPrint("\nPSUI: cbTrayForm=%ld\n", cbTrayForm);
#endif

    if (cbTrayForm) {

        // allocate a buffer.

        if (!(pwstrName = (WCHAR *)LocalAlloc(LPTR, cbTrayForm))) {

            RIP("PSCRPTUI!InitComboBoxes: GlobalAlloc pwstrName failed.\n");
            return(FALSE);
        }

        pwstrSave = pwstrName;

        // now grab the table itself from the registry.

        LoadString(hModule, IDS_TRAY_FORM_TABLE,
                  wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

        rc = GetPrinterData(pdata->hPrinter, wcbuf, &dwType, (LPBYTE)pwstrName,
                            cbTrayForm, &cbNeeded);

        if (rc != ERROR_SUCCESS) {

            RIP("PSCRPTUI!InitComboBoxes: GetPrinterData pwstrName failed.\n");
            LocalFree((HLOCAL)pwstrSave);
            return(FALSE);
        }

        if (!(pwstrSave = ConvertOldTrayTable(NULL, pwstrSave, cbTrayForm))) {

            RIP("PSCRPTUI!InitComboBoxes: ConvertOldTrayTable()=NULL\n");
            return(FALSE);
        }

        if ((pwstrSave != pwstrName) && (pdata->bPermission)) {
#if DBG
            DbgPrint("PSUI: OldTable so force update the TrayTable Registry\n");
#endif
            ForceUpdate = TRUE;
        }

        for (i = 0, pDefForm = pdata->pDefForm;
             i < count;
             i++, pDefForm++) {

            PWSTR   pwSlotName;
            PWSTR   pwFormName;

            // get the tray name.

            SendDlgItemMessage(hwnd,
                               IDD_TRAY_LIST_BOX,
                               CB_GETLBTEXT,
                               (WPARAM)i,
                               (LPARAM)SlotName);

            pwstrName = pwstrSave + 1;

            // search the tray-form table for a matching tray.

            bFound = FALSE;

            while (*pwstrName) {

                pwstrName  += (wcslen(pwSlotName = pwstrName) + 1);
                pwFormName  = pwstrName;

                //
                // Skip FormName, PrinterName
                //

                pwstrName += (wcslen(pwFormName) + 1);
                pwstrName += (wcslen(pwstrName) + 1);

                if (!(wcscmp(pwSlotName, SlotName))) {

                    //
                    // we found the inplut slot, now get the matching form.
                    //

                    wcsncpy(FormName, pwFormName, CCHFORMNAME - 1);
                    FormName[CCHFORMNAME - 1] = L'\0';

                    bFound = TRUE;
                    break;
                }

                //
                // Skip the selection
                //

                pwstrName++;
            }

#if DEF_TRAY_TEST
            DbgPrint("%2ld: Slot=%ws, [%ld] Form=%ws\n",
                    (DWORD)i,
                    pwSlotName,
                    (DWORD)((bFound) ? 1 : 0),
                    pwFormName);
#endif
            if ((bFound) &&
                ((rc = SendDlgItemMessage(hwnd,
                                          IDD_PAPER_LIST_BOX,
                                          CB_FINDSTRING,
                                          (WPARAM)-1,
                                          (LPARAM)FormName)) != CB_ERR)) {

                pDefForm->Index  = (WORD)rc;
                pDefForm->Select = (WORD)*pwstrName;
            }
        }

        //
        // free up the tray-form table buffer.
        //

        LocalFree((HLOCAL)pwstrSave);
    }

    //
    // Validate All Trays
    //

    DefaultForm = -1;

    for (i = 0, pDefForm = pdata->pDefForm; i < count; i++, pDefForm++) {

        if (DefaultForm != (int)pDefForm->Index) {

            ValidateDefTray(pdata->pDefForm, count, pDefForm->Index);
            DefaultForm = (int)pDefForm->Index;
        }
    }

    SetTrayFormName(hwnd, pdata, DefaultTray);

    //
    // If we have old data then write the new one out
    //

    if (ForceUpdate) {

        SaveTrayFormToReg(hwnd, pdata);
    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL bAbout(hwnd, pntpd)
// HWND    hwnd;
// PNTPD   pntpd;
//
// This routine invokes the common About dialog.
//
// Returns:
//   This routine returns TRUE for success, FALSE otherwise.
//
// History:
//   22-Apr-1993    -by-    Kent Settle     (kentse)
// Borrowed from Rasdd, and modified.
//--------------------------------------------------------------------------

BOOL bAbout(hwnd, pntpd)
HWND    hwnd;
PNTPD   pntpd;
{
    HANDLE          hCursor, hIcon;
    WCHAR           wszTitle[RESOURCE_STRING_LENGTH] = L"";
    WCHAR           wszModel[RESOURCE_STRING_LENGTH] = L"";
    WCHAR          *pwstr;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    hIcon = LoadIcon(hModule, MAKEINTRESOURCE(ICO_PRINTER));

    pwstr = (PWSTR)((PSTR)pntpd + pntpd->lowszPrinterName);

    // fill in the device name string.

    LoadString(hModule, IDS_MODEL_STRING, wszModel,
               sizeof(wszModel) / sizeof(wszModel[0]));

    // fill in the model name itself.

    pwstr = wszModel + wcslen(wszModel);

    wcsncpy(pwstr, (LPWSTR)((PSTR)pntpd + pntpd->lowszPrinterName),
            RESOURCE_STRING_LENGTH - wcslen(wszModel));

    // get our name.

    LoadString(hModule, IDS_PSCRIPT_VERSION,
               wszTitle, sizeof(wszTitle) / sizeof(wszTitle[0]));

    // call off to the common about dialog.

    ShellAbout(hwnd, wszTitle, wszModel, hIcon);

    SetCursor(hCursor);

    return(TRUE);
}

VOID GetPrinterForm(pntpd, pForm, pPrinterForm)
PNTPD           pntpd;
FORM_INFO_1    *pForm;
PWSTR           pPrinterForm;
{
    PSFORM         *pPSForm;
    DWORD           i;
    BOOL            bFound;
    SIZEL           sizldelta, sizltmp;

	/* Ensure null termination of string on return */
	pPrinterForm[CCHFORMNAME-1] = (WCHAR) '\0';

	/* Simply return form name if it matches by name a PPD pagesize */
	if (MatchFormToPPD(pntpd, (LPWSTR) pForm->pName) != (PSFORM *) NULL) {
		wcsncpy(pPrinterForm, pForm->pName, CCHFORMNAME-1);
		return;
	}

    // if we did not find a name match, try to locate a form by size.

    // get pointer to first form in NTPD.

    pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

    bFound = FALSE;
    for (i = 0; i < pntpd->cPSForms; i++) {

        sizltmp.cx = USERTO001MM(pPSForm->sizlPaper.cx) - pForm->Size.cx;
        sizltmp.cy = USERTO001MM(pPSForm->sizlPaper.cy) - pForm->Size.cy;

        // see if we have an exact match on size (within 1mm).

        if (((DWORD)abs(sizltmp.cx) <= 1000) &&
             ((DWORD)abs(sizltmp.cy) <= 1000)) {
            // we have an exact match on size, so overwrite the form
            // name with the name the printer knows about.

			MultiByteToWideChar(CP_ACP, 0, (CHAR *)pntpd + pPSForm->loFormName, -1, pPrinterForm, CCHFORMNAME-1);
            return;
        }

        // not an exact match, but see if we could fit on this form.

        if ((sizltmp.cx >= 0) && (sizltmp.cy >= 0))	{
            // we can fit on this form.  let's see if it is the smallest.

            if (!bFound || (sizltmp.cx <= sizldelta.cx && sizltmp.cy <= sizldelta.cy)) {
                // this form is the smallest yet.

                sizldelta = sizltmp;
				MultiByteToWideChar(CP_ACP, 0, (CHAR *)pntpd + pPSForm->loFormName, -1, pPrinterForm, CCHFORMNAME-1);
				bFound = TRUE;
			}
		}

        // point to the next PSFORM.

        pPSForm++;
    }

    // set to default if no form was found.

    if (!bFound) GrabDefaultFormName(hModule, pPrinterForm);
}

