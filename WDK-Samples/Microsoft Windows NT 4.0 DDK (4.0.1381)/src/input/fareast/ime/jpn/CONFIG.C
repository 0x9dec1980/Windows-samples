/**********************************************************************/
/*                                                                    */
/*      CONFIG.C - Windows 95 FAKEIME                                 */
/*                                                                    */
/*      Copyright (c) 1994-1996  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/
#include "precomp.h"
#include "prsht.h"

#define MAX_PAGES 4

/**********************************************************************/
/*                                                                    */
/*      AddPage()                                                     */
/*                                                                    */
/**********************************************************************/
void AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn)
{
    if (ppsh->nPages < MAX_PAGES) {
        PROPSHEETPAGE psp;

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hInst;
        psp.pszTemplate = MAKEINTRESOURCE(id);
        psp.pfnDlgProc = pfn;
        psp.lParam = 0;

        ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
        if (ppsh->phpage[ppsh->nPages])
             ppsh->nPages++;
    }
}  

/**********************************************************************/
/*                                                                    */
/*      ImeConfigure()                                                */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeConfigure(HKL hKL,HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hWnd;
    psh.hInstance = hInst;
    psh.pszCaption = MAKEINTRESOURCE(IDS_CONFIGNAME);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;

    
    switch (dwMode)
    {
        case IME_CONFIG_GENERAL:
            AddPage(&psh, DLG_GENERAL, GeneralDlgProc);
            AddPage(&psh, DLG_REGISTERWORD, RegWordDlgProc);
            AddPage(&psh, DLG_SELECTDICTIONARY, SelectDictionaryDlgProc);
            AddPage(&psh, DLG_ABOUT, AboutDlgProc);
            PropertySheet(&psh);
            break;

        case IME_CONFIG_REGISTERWORD:
            AddPage(&psh, DLG_REGISTERWORD, RegWordDlgProc);
            AddPage(&psh, DLG_ABOUT, AboutDlgProc);
            PropertySheet(&psh);
            break;

        case IME_CONFIG_SELECTDICTIONARY:
            AddPage(&psh, DLG_SELECTDICTIONARY, SelectDictionaryDlgProc);
            AddPage(&psh, DLG_ABOUT, AboutDlgProc);
            PropertySheet(&psh);
            break;

        default:
            break;
    }

    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/*      RegWordConfigure()                                            */
/*                                                                    */
/**********************************************************************/
BOOL CALLBACK RegWordDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLong(hDlg, DWL_USER));
    UINT nItem;
    UINT i;
    DWORD dwIndex;
    TCHAR szRead[128];
    TCHAR szString[128];
    TCHAR szBuf[128];
    DWORD dwStyle;

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    break;
        
                case PSN_KILLACTIVE:
                    break;
        
                case PSN_APPLY:

                    if (!GetDlgItemText(hDlg, ID_WR_READING, szRead, sizeof(szRead)))
                    {
                        LoadString(hInst,IDS_NOREADING,szBuf,sizeof(szBuf));
                        MessageBox(hDlg, szBuf, NULL, MB_OK);
                        return FALSE;
                    }


                    if (!GetDlgItemText(hDlg, ID_WR_STRING, szString, sizeof(szString)))
                    {
                        LoadString(hInst,IDS_NOSTRING,szBuf,sizeof(szBuf));
                        MessageBox(hDlg, szBuf, NULL, MB_OK);
                        return FALSE;
                    }

                    dwIndex = SendDlgItemMessage(hDlg, ID_WR_STYLE,CB_GETCURSEL,0,0);
                    dwStyle = SendDlgItemMessage(hDlg, ID_WR_STYLE,CB_GETITEMDATA,dwIndex,0);

                    if (!ImeRegisterWord(szRead, dwStyle, szString))
                    {
                        LoadString(hInst,IDS_REGWORDRET,szBuf,sizeof(szBuf));
                        MessageBox(hDlg, szBuf, NULL, MB_OK);
                    }
                    break;

                case PSN_RESET:
                    break;

                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            SetWindowLong(hDlg, DWL_USER, lParam);
            lpPropSheet = (LPPROPSHEETPAGE)lParam;

            if (nItem = ImeGetRegisterWordStyle(0, NULL))
            {
                PBYTE pMem = LocalAlloc(LMEM_FIXED,nItem * sizeof(STYLEBUF));
                LPSTYLEBUF lpStyleBuf = (LPSTYLEBUF)pMem;

                if (!lpStyleBuf)
                {
                    LoadString(hInst,IDS_NOMEMORY,szBuf,sizeof(szBuf));
                    MessageBox(hDlg, szBuf, NULL, MB_OK);
                    return TRUE;
                }

                ImeGetRegisterWordStyle(nItem,lpStyleBuf);

                for (i = 0; i < nItem; i++)
                {
                    dwIndex = SendDlgItemMessage(hDlg,ID_WR_STYLE,CB_ADDSTRING,0,(LPARAM)lpStyleBuf->szDescription);
                    SendDlgItemMessage(hDlg,ID_WR_STYLE,CB_SETITEMDATA,dwIndex,lpStyleBuf->dwStyle);
                    lpStyleBuf++;
                }

                LocalFree(pMem);
            }
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            break;

        case WM_CONTEXTMENU:   // right mouse click
            break;

        case WM_COMMAND:
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 


/**********************************************************************/
/*                                                                    */
/*      SelectDictionaryConfigure()                                   */
/*                                                                    */
/**********************************************************************/
BOOL CALLBACK SelectDictionaryDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLong(hDlg, DWL_USER));

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    break;
        
                case PSN_KILLACTIVE:
                    break;
        
                case PSN_APPLY:
                    break;

                case PSN_RESET:
                    break;

                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            SetWindowLong(hDlg, DWL_USER, lParam);
            lpPropSheet = (LPPROPSHEETPAGE)lParam;
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            break;

        case WM_CONTEXTMENU:   // right mouse click
            break;

        case WM_COMMAND:
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 


/**********************************************************************/
/*                                                                    */
/*      AboutConfigure()                                            */
/*                                                                    */
/**********************************************************************/
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLong(hDlg, DWL_USER));

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    break;
        
                case PSN_KILLACTIVE:
                    break;
        
                case PSN_APPLY:
                    break;

                case PSN_RESET:
                    break;

                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            SetWindowLong(hDlg, DWL_USER, lParam);
            lpPropSheet = (LPPROPSHEETPAGE)lParam;
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            break;

        case WM_CONTEXTMENU:   // right mouse click
            break;

        case WM_COMMAND:
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 


/**********************************************************************/
/*                                                                    */
/*      GeneralConfigure()                                            */
/*                                                                    */
/**********************************************************************/
BOOL CALLBACK GeneralDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLong(hDlg, DWL_USER));

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    break;
        
                case PSN_KILLACTIVE:
                    break;
        
                case PSN_APPLY:
                    break;

                case PSN_RESET:
                    break;

                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            SetWindowLong(hDlg, DWL_USER, lParam);
            lpPropSheet = (LPPROPSHEETPAGE)lParam;
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            break;

        case WM_CONTEXTMENU:   // right mouse click
            break;

        case WM_COMMAND:
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 

