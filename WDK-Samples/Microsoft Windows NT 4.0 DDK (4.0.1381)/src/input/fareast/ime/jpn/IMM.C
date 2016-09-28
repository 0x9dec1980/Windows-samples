/**********************************************************************/
/*                                                                    */
/*      IMM.C - Windows 95 FAKEIME                                    */
/*                                                                    */
/*      Copyright (c) 1994-1996  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/
#include "precomp.h"


/**********************************************************************/
/*      ImeInquire()                                                  */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeInquire(LPIMEINFO lpIMEInfo,LPTSTR lpszClassName, DWORD dwFlag)
{
    //
    // Init IMEINFO Structure.
    //
    lpIMEInfo->dwPrivateDataSize = sizeof(DWORD);

#ifdef UNICODE
    lpIMEInfo->fdwProperty = IME_PROP_KBD_CHAR_FIRST | IME_PROP_UNICODE | IME_PROP_AT_CARET;
#else
    lpIMEInfo->fdwProperty = IME_PROP_KBD_CHAR_FIRST | IME_PROP_AT_CARET;
#endif
    lpIMEInfo->fdwConversionCaps = IME_CMODE_LANGUAGE |
                                IME_CMODE_FULLSHAPE |
                                IME_CMODE_ROMAN |
                                IME_CMODE_CHARCODE;
    lpIMEInfo->fdwSentenceCaps = 0L;
    lpIMEInfo->fdwUICaps = UI_CAP_2700;
    lpIMEInfo->fdwSCSCaps = 0L;
    lpIMEInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;

    lstrcpy(lpszClassName,(LPTSTR)szUIClassName);

    return TRUE;
}

/**********************************************************************/
/*      ImeConversionList()                                           */
/*                                                                    */
/**********************************************************************/
DWORD WINAPI ImeConversionList(HIMC hIMC,LPCTSTR lpSource,LPCANDIDATELIST lpCandList,DWORD dwBufLen,UINT uFlags)
{


    return 0;
}

/**********************************************************************/
/*      ImeDestroy()                                                  */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeDestroy(UINT uForce)
{

    return TRUE;
}

/**********************************************************************/
/*      ImeEscape()                                                   */
/*                                                                    */
/**********************************************************************/
LRESULT WINAPI ImeEscape(HIMC hIMC,UINT uSubFunc,LPVOID lpData)
{
    LRESULT lRet = FALSE;

    switch (uSubFunc)
    {
        case IME_ESC_QUERY_SUPPORT:
            switch (*(LPUINT)lpData)
            {
                case IME_ESC_QUERY_SUPPORT:
                    // Only this function is supported.
                    lRet = TRUE;
                    break;

                default:
                    lRet = FALSE;
                    break;
            }
            break;

        default:
            lRet = FALSE;
            break;
    }
    return lRet;
}

/**********************************************************************/
/*      ImeSetActiveContext()                                         */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeSetActiveContext(HIMC hIMC,BOOL fFlag)
{
    return TRUE;
}

/**********************************************************************/
/*      ImeProcessKey()                                               */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeProcessKey(HIMC hIMC,UINT vKey,LPARAM lKeyData, CONST LPBYTE lpbKeyState)
{
    BOOL fRet = FALSE;
    BOOL fOpen;
    BOOL fCompStr = FALSE;
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING pComp;

    if (lKeyData & 0x80000000)
        return FALSE;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return FALSE;

    fOpen = lpIMC->fOpen;

    if (fOpen)
    {
        if (pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
        {
            if ((pComp->dwSize > sizeof(COMPOSITIONSTRING)) && 
                (pComp->dwCompStrLen))
                fCompStr = TRUE;
        }

        if (lpbKeyState[VK_MENU] & 0x80)
        {
            fRet = FALSE;
        }
        else if (lpbKeyState[VK_CONTROL] & 0x80)
        {
            if (fCompStr)
                fRet = (BOOL)bCompCtl[vKey];
            else
                fRet = (BOOL)bNoCompCtl[vKey];
        }
        else if (lpbKeyState[VK_SHIFT] & 0x80)
        {
            if (fCompStr)
                fRet = (BOOL)bCompSht[vKey];
            else
                fRet = (BOOL)bNoCompSht[vKey];
        }
        else 
        {
            if (fCompStr)
                fRet = (BOOL)bComp[vKey];
            else
                fRet = (BOOL)bNoComp[vKey];
        }

        if (pComp)
            ImmUnlockIMCC(lpIMC->hCompStr);
    }

    ImmUnlockIMC(hIMC);
    return fRet;
}

/**********************************************************************/
/*      NotifyIME()                                                   */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI NotifyIME(HIMC hIMC,DWORD dwAction,DWORD dwIndex,DWORD dwValue)
{
    LPINPUTCONTEXT      lpIMC;
    BOOL                bRet = FALSE;
    LPCOMPOSITIONSTRING pComp;
    LPCANDIDATEINFO     lpCandInfo;
    LPCANDIDATELIST     lpCandList;
    TCHAR                szBuf[256];
    int                 nBufLen;
    LPTSTR               lpstr;
    GENEMSG             GnMsg;
    int                 i = 0;
    LPDWORD             lpdw;

    switch(dwAction)
    {

        case NI_CONTEXTUPDATED:
             switch (dwValue)
             {
                 case IMC_SETOPENSTATUS:
                     lpIMC = ImmLockIMC(hIMC);
                     if (!lpIMC->fOpen && IsCompStr(hIMC))
                         FlushText(hIMC);
                     ImmUnlockIMC(hIMC);
                     bRet = TRUE;
                     break;

                 case IMC_SETCONVERSIONMODE:
                     lpIMC = ImmLockIMC(hIMC);
                     if ( lpIMC != NULL ) {
                        pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                        if ( pComp != NULL ) {
                            INT iCursor = (INT)pComp->dwCursorPos;

                            lpstr = GETLPCOMPSTR(pComp);
                            iCursor = MapCompositionString(lpstr, iCursor, lpIMC->fdwConversion);
                            pComp->dwCursorPos = (DWORD)iCursor;
                            UpdateCompositionString( hIMC );
                            GnMsg.msg = WM_IME_COMPOSITION;
                            GnMsg.wParam = 0;
                            GnMsg.lParam = GCS_COMPALL | GCS_CURSORPOS | GCS_DELTASTART;
                            GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
                            ImmUnlockIMCC(lpIMC->hCompStr);
                        }
                        ImmUnlockIMC(hIMC);
                     }
                     break;

                 case IMC_SETCOMPOSITIONWINDOW:
                     break;

                 default:
                     break;
             }
             break;

        case NI_COMPOSITIONSTR:
             switch (dwIndex)
             {
                 case CPS_COMPLETE:
                     MakeResultString(hIMC,TRUE);
                     bRet = TRUE;
                     break;

                 case CPS_CONVERT:
                     ConvKanji(hIMC);
                     bRet = TRUE;
                     break;

                 case CPS_REVERT:
                     RevertText(hIMC);
                     bRet = TRUE;
                     break;

                 case CPS_CANCEL:
                     FlushText(hIMC);
                     bRet = TRUE;
                     break;

                 default:
                     break;
             }
             break;

        case  NI_OPENCANDIDATE:
             if (IsConvertedCompStr(hIMC))
             {

                 if (!(lpIMC = ImmLockIMC(hIMC)))
                     return FALSE;

                 if (!(pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr)))
                 {
                     ImmUnlockIMC(hIMC);
                     return FALSE;
                 }

                 if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
                 {

                     // 
                     // Get the candidate strings from dic file.
                     //
                     nBufLen = GetPrivateProfileString(GETLPCOMPREADSTR(pComp),
                            NULL,TEXT(""),
                            (LPTSTR)szBuf,256,(LPTSTR)szDicFileName );

                     //
                     // generate WM_IME_NOTFIY IMN_OPENCANDIDATE message.
                     //
                     GnMsg.msg = WM_IME_NOTIFY;
                     GnMsg.wParam = IMN_OPENCANDIDATE;
                     GnMsg.lParam = 1L;
                     GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

                     //
                     // Make candidate structures.
                     //
                     lpCandInfo->dwSize = sizeof(MYCAND);
                     lpCandInfo->dwCount = 1;
                     lpCandInfo->dwOffset[0] = 
                           (DWORD)((LPTSTR)&((LPMYCAND)lpCandInfo)->cl - (LPTSTR)lpCandInfo);
                     lpCandList = (LPCANDIDATELIST)((LPTSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
                     lpdw = (LPDWORD)&(lpCandList->dwOffset);

                     lpstr = &szBuf[0];
                     while (*lpstr && (i < MAXCANDSTRNUM))
                     {
                         lpCandList->dwOffset[i] = 
                                (DWORD)((LPTSTR)((LPMYCAND)lpCandInfo)->szCand[i] - (LPTSTR)lpCandList);
                         lstrcpy((LPTSTR)((LPTSTR)lpCandList+lpCandList->dwOffset[i]),lpstr);
                         lpstr += (lstrlen(lpstr) + 1);
                         i++;
                     }

                     lpCandList->dwSize = sizeof(CANDIDATELIST) + 
                          (MAXCANDSTRNUM * (sizeof(DWORD) + MAXCANDSTRSIZE));
                     lpCandList->dwStyle = IME_CAND_READ;
                     lpCandList->dwCount = i;
                     lpCandList->dwPageStart = 0;
                     if (i < MAXCANDPAGESIZE)
                         lpCandList->dwPageSize  = i;
                     else
                         lpCandList->dwPageSize  = MAXCANDPAGESIZE;

                     lpCandList->dwSelection++;
                     if (lpCandList->dwSelection == (DWORD)i)
                         lpCandList->dwSelection = 0;

                     //
                     // Generate messages.
                     //
                     GnMsg.msg = WM_IME_NOTIFY;
                     GnMsg.wParam = IMN_CHANGECANDIDATE;
                     GnMsg.lParam = 1L;
                     GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

                     ImmUnlockIMCC(lpIMC->hCandInfo);
                     ImmUnlockIMC(hIMC);

                     bRet = TRUE;
                 }
             }
             break;

        case  NI_CLOSECANDIDATE:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                 return FALSE;
             if (IsCandidate(lpIMC))
             {
                 GnMsg.msg = WM_IME_NOTIFY;
                 GnMsg.wParam = IMN_CLOSECANDIDATE;
                 GnMsg.lParam = 1L;
                 GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
                 bRet = TRUE;
             }
             ImmUnlockIMC(hIMC);
             break;

        case  NI_SELECTCANDIDATESTR:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                 return FALSE;

             if (dwIndex == 1 && IsCandidate(lpIMC))
             {

                 if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
                 {

                     lpCandList = (LPCANDIDATELIST)((LPTSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
                     if (lpCandList->dwCount > dwValue)
                     {
                         lpCandList->dwSelection = dwValue;
                         bRet = TRUE;

                         //
                         // Generate messages.
                         //
                         GnMsg.msg = WM_IME_NOTIFY;
                         GnMsg.wParam = IMN_CHANGECANDIDATE;
                         GnMsg.lParam = 1L;
                         GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
                     }
                     ImmUnlockIMCC(lpIMC->hCandInfo);

                 }
             }
             ImmUnlockIMC(hIMC);
             break;

        case  NI_CHANGECANDIDATELIST:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                return FALSE;

             if (dwIndex == 1 && IsCandidate(lpIMC))
                 bRet = TRUE;

             ImmUnlockIMC(hIMC);
             break;

        case NI_SETCANDIDATE_PAGESIZE:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                return FALSE;

             if (dwIndex == 1 && IsCandidate(lpIMC))
             {
                 if (dwValue > MAXCANDPAGESIZE)
                     return FALSE;


                 if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
                 {
                     lpCandList = (LPCANDIDATELIST)((LPTSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
                     if (lpCandList->dwCount > dwValue)
                     {
                         lpCandList->dwPageSize = dwValue;
                         bRet = TRUE;

                         //
                         // Generate messages.
                         //
                         GnMsg.msg = WM_IME_NOTIFY;
                         GnMsg.wParam = IMN_CHANGECANDIDATE;
                         GnMsg.lParam = 1L;
                         GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
                     }
                     ImmUnlockIMCC(lpIMC->hCandInfo);
                 }
             }
             ImmUnlockIMC(hIMC);
             break;

        case NI_SETCANDIDATE_PAGESTART:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                 return FALSE;

             if (dwIndex == 1 && IsCandidate(lpIMC))
             {
                 if (dwValue > MAXCANDPAGESIZE)
                     return FALSE;


                 if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
                 {
                     lpCandList = (LPCANDIDATELIST)((LPTSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
                     if (lpCandList->dwCount > dwValue)
                     {
                         lpCandList->dwPageStart = dwValue;
                         bRet = TRUE;

                         //
                         // Generate messages.
                         //
                         GnMsg.msg = WM_IME_NOTIFY;
                         GnMsg.wParam = IMN_CHANGECANDIDATE;
                         GnMsg.lParam = 1L;
                         GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
                     }
                     ImmUnlockIMCC(lpIMC->hCandInfo);

                 }
             }
             ImmUnlockIMC(hIMC);
             break;

        default:
             break;
    }
    return bRet;
}

/**********************************************************************/
/*      ImeSelect()                                                   */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
    LPINPUTCONTEXT lpIMC;

    // it's NULL context.
    if (!hIMC)
        return TRUE;

    if (lpIMC = ImmLockIMC(hIMC))
    {
        if (fSelect)
        {
            LPCOMPOSITIONSTRING pComp;
            LPCANDIDATEINFO lpCandInfo;

            // Init the general member of IMC.
            if (!(lpIMC->fdwInit & INIT_LOGFONT))
            {
#ifdef UNICODE
                lpIMC->lfFont.W.lfCharSet = SHIFTJIS_CHARSET;
#else
                lpIMC->lfFont.A.lfCharSet = SHIFTJIS_CHARSET;
#endif
                lpIMC->fdwInit |= INIT_LOGFONT;
            }

        
            if (!(lpIMC->fdwInit & INIT_CONVERSION))
            {
                lpIMC->fdwConversion = IME_CMODE_ROMAN | IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE;
                lpIMC->fdwInit |= INIT_CONVERSION;
            }

            lpIMC->hCompStr = ImmReSizeIMCC(lpIMC->hCompStr,sizeof(MYCOMPSTR));
            if (pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr)) 
            {
                pComp->dwSize = sizeof(MYCOMPSTR);
                ImmUnlockIMCC(lpIMC->hCompStr);
            }
            lpIMC->hCandInfo = ImmReSizeIMCC(lpIMC->hCandInfo,sizeof(MYCAND));
            if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo)) 
            {
                lpCandInfo->dwSize = sizeof(MYCAND);
                ImmUnlockIMCC(lpIMC->hCandInfo);
            }
        }
    }

    ImmUnlockIMC(hIMC);
    return TRUE;
}


/**********************************************************************/
/*      ImeSetCompositionString()                                     */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwComp, LPVOID lpRead, DWORD dwRead)
{
    return FALSE;
}
