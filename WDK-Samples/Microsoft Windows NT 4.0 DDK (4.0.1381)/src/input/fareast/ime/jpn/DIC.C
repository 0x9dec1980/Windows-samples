/**********************************************************************/
/*                                                                    */
/*      DIC.C - Windows 95 FAKEIME                                    */
/*                                                                    */
/*      Copyright (c) 1994-1995  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/
#include "precomp.h"
#include "vksub.h"


/**********************************************************************/
/*                                                                    */
/* FlushText()                                                        */
/*                                                                    */
/**********************************************************************/
void PASCAL FlushText(HIMC hIMC)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING pComp;
    LPCANDIDATEINFO lpCandInfo;
    GENEMSG GnMsg;

    if (!IsCompStr(hIMC))
        return;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return;

    if (IsCandidate(lpIMC))
    {
        //
        // Flush candidate lists.
        //
        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
        ClearCandidate(lpCandInfo);
        ImmUnlockIMCC(lpIMC->hCandInfo);
        GnMsg.msg = WM_IME_NOTIFY;
        GnMsg.wParam = IMN_CLOSECANDIDATE;
        GnMsg.lParam = 1;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
    }

    if (pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
    {
        //
        // Flush composition strings.
        //
        ClearCompStr(pComp,CLR_RESULT_AND_UNDET);
        ImmUnlockIMCC(lpIMC->hCompStr);

        GnMsg.msg = WM_IME_COMPOSITION;
        GnMsg.wParam = 0;
        GnMsg.lParam = 0;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

        GnMsg.msg = WM_IME_ENDCOMPOSITION;
        GnMsg.wParam = 0;
        GnMsg.lParam = 0;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
    }


    ImmUnlockIMC(hIMC);
}

/**********************************************************************/
/*                                                                    */
/* RevertText()                                                       */
/*                                                                    */
/**********************************************************************/
void PASCAL RevertText(HIMC hIMC)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING pComp;
    LPCANDIDATEINFO lpCandInfo;
    GENEMSG GnMsg;
    LPTSTR lpread,lpstr;

    if (!IsCompStr(hIMC))
        return;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return;

    if (IsCandidate(lpIMC))
    {
        //
        // Flush candidate lists.
        //
        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
        ClearCandidate(lpCandInfo);
        ImmUnlockIMCC(lpIMC->hCandInfo);
        GnMsg.msg = WM_IME_NOTIFY;
        GnMsg.wParam = IMN_CLOSECANDIDATE;
        GnMsg.lParam = 1;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
    }

    if (pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
    {
        lpstr = GETLPCOMPSTR(pComp);
        lpread = GETLPCOMPREADSTR(pComp);
        HanToZen(lpstr, MAXCOMPSIZE, lpread, lpIMC->fdwConversion);

        //
        // make attribute
        //
        pComp->dwCursorPos = lstrlen(lpstr);
        // DeltaStart is 0 at RevertText time.
        pComp->dwDeltaStart = 0;

        lmemset(GETLPCOMPATTR(pComp),0,lstrlen(lpstr));
        lmemset(GETLPCOMPREADATTR(pComp),0,lstrlen(lpread));

        SetClause(GETLPCOMPCLAUSE(pComp),lstrlen(lpstr));
        SetClause(GETLPCOMPREADCLAUSE(pComp),lstrlen(lpread));
        pComp->dwCompClauseLen = 8;
        pComp->dwCompReadClauseLen = 8;

        //
        // make length
        //
        pComp->dwCompStrLen = lstrlen(lpstr);
        pComp->dwCompReadStrLen = lstrlen(lpread);
        pComp->dwCompAttrLen = lstrlen(lpstr);
        pComp->dwCompReadAttrLen = lstrlen(lpread);


        //
        // Generate messages.
        //
        GnMsg.msg = WM_IME_COMPOSITION;
        GnMsg.wParam = 0;
        GnMsg.lParam = GCS_COMPALL | GCS_CURSORPOS | GCS_DELTASTART;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
   
        ImmUnlockIMCC(lpIMC->hCompStr);
    }
    ImmUnlockIMC(hIMC);
}

/**********************************************************************/
/*                                                                    */
/* ConvKanji()                                                        */
/*                                                                    */
/* Convert the composition string                                     */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL ConvKanji(HIMC hIMC)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING pComp;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    TCHAR szBuf[256];
    int nBufLen;
    LPTSTR lpstr;
    GENEMSG GnMsg;
    LPBYTE lpb;
    LPTSTR lpT;
    int cnt;
    HANDLE hDicFile;
    
    //
    // check if dictionary file exits there
    // 
    hDicFile = CreateFile( szDicFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,      
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL );
    if ( hDicFile == NULL ) {
        MakeGuideLine(hIMC,MYGL_NODICTIONARY);
    } else {
        CloseHandle( hDicFile );
    }

    if (!IsCompStr(hIMC))
        return FALSE;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return FALSE;

    if (!(pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr)))
        goto cvk_exit20;

    if (!(lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo)))
        goto cvk_exit10;

    // 
    // Get the candidate strings from dic file.
    //
    nBufLen = GetPrivateProfileString(GETLPCOMPREADSTR(pComp),
                            NULL,TEXT(""),
                            (LPTSTR)szBuf,256,(LPTSTR)szDicFileName );
    //
    // Check the result of dic. Because my candidate list has only MAXCANDSTRNUM
    // candidate strings.
    //
    lpT = &szBuf[0];
    cnt = 0;
    while(*lpT)
    {
        cnt++;
        lpT += (lstrlen(lpT) + 1);

        if (cnt > MAXCANDSTRNUM)
        {
            //
            // The dic is too big....
            //
            ImmUnlockIMCC(lpIMC->hCompStr);
            ImmUnlockIMCC(lpIMC->hCandInfo);
            ImmUnlockIMC(hIMC);
            return FALSE;
        }
            
    }

    lpb = GETLPCOMPATTR(pComp);

    if (nBufLen < 1)
    {
        if (!*lpb)
        {
            //
            // make attribute
            //
            lmemset(GETLPCOMPATTR(pComp),1,
                  lstrlen(GETLPCOMPSTR(pComp)));
            lmemset(GETLPCOMPREADATTR(pComp),1,
                  lstrlen(GETLPCOMPREADSTR(pComp)));

            GnMsg.msg = WM_IME_COMPOSITION;
            GnMsg.wParam = 0;
            GnMsg.lParam = GCS_COMPSTR | GCS_CURSORPOS |
                           GCS_COMPATTR | GCS_COMPREADATTR;
            GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
        }

        ImmUnlockIMCC(lpIMC->hCompStr);
        ImmUnlockIMCC(lpIMC->hCandInfo);
        ImmUnlockIMC(hIMC);
        return FALSE;
    }

  
    lpstr = (LPTSTR)szBuf;
    if (!*lpb)
    {
        //
        // String is not converted yet.
        //
        while (*lpstr)
        {
            if (lstrcmp(lpstr,GETLPCOMPSTR(pComp)))
            {
set_compstr:
                // 
                // Set the composition string to the structure.
                // 
                lstrcpy(GETLPCOMPSTR(pComp),lpstr);

                lpstr = GETLPCOMPSTR(pComp);

                // 
                // Set the length and cursor position to the structure.
                // 
                pComp->dwCompStrLen = lstrlen(lpstr);
                pComp->dwCursorPos = 0;
                // Because FAKEIME does not support clause, DeltaStart is 0 anytime.
                pComp->dwDeltaStart = 0;

                // 
                // make attribute
                // 
                lmemset(GETLPCOMPATTR(pComp),1, lstrlen(lpstr));
                lmemset(GETLPCOMPREADATTR(pComp),1,
                                      lstrlen(GETLPCOMPREADSTR(pComp)));

                // 
                // make clause info
                // 
                SetClause(GETLPCOMPCLAUSE(pComp),lstrlen(lpstr));
                SetClause(GETLPCOMPREADCLAUSE(pComp),lstrlen(GETLPCOMPREADSTR(pComp)));
                pComp->dwCompClauseLen = 8;
                pComp->dwCompReadClauseLen = 8;

                // 
                // Generate messages.
                // 
                GnMsg.msg = WM_IME_COMPOSITION;
                GnMsg.wParam = 0;
                GnMsg.lParam = GCS_COMPALL | GCS_CURSORPOS | GCS_DELTASTART;
                GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

                ImmUnlockIMCC(lpIMC->hCompStr);
                ImmUnlockIMCC(lpIMC->hCandInfo);
                ImmUnlockIMC(hIMC);
                return TRUE;
            }
            lpstr += (lstrlen(lpstr) + 1);
            
        }
    }
    else
    {
        //
        // String is converted, so that open candidate.
        //
        int i = 0;
        LPDWORD lpdw;

        //
        // generate WM_IME_NOTFIY IMN_OPENCANDIDATE message.
        //
        if (!IsCandidate(lpIMC))
        {
            GnMsg.msg = WM_IME_NOTIFY;
            GnMsg.wParam = IMN_OPENCANDIDATE;
            GnMsg.lParam = 1L;
            GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
        }

        //
        // Make candidate structures.
        //
        lpCandInfo->dwSize = sizeof(MYCAND);
        lpCandInfo->dwCount = 1;
        lpCandInfo->dwOffset[0] = 
              (DWORD)((PBYTE)(&((LPMYCAND)lpCandInfo)->cl) - (PBYTE)lpCandInfo);
        lpCandList = (LPCANDIDATELIST)((PBYTE)lpCandInfo  + lpCandInfo->dwOffset[0]);
        lpdw = (LPDWORD)&(lpCandList->dwOffset);
        while (*lpstr)
        {
            lpCandList->dwOffset[i] = 
                   (DWORD)( (PBYTE)((LPMYCAND)lpCandInfo)->szCand[i] - (PBYTE)lpCandList );
            lstrcpy((LPTSTR)((PBYTE)lpCandList+lpCandList->dwOffset[i]),lpstr);
            lpstr += (lstrlen(lpstr) + 1);
            i++;
        }

        lpCandList->dwSize = sizeof(CANDIDATELIST) + 
                          (MAXCANDSTRNUM * (sizeof(DWORD) + MAXCANDSTRSIZE));
        lpCandList->dwStyle = IME_CAND_READ;
        lpCandList->dwCount = i;
        if (i < MAXCANDPAGESIZE)
            lpCandList->dwPageSize  = i;
        else
            lpCandList->dwPageSize  = MAXCANDPAGESIZE;

        lpCandList->dwSelection++;
        if (lpCandList->dwSelection == (DWORD)i)
        {
            lpCandList->dwPageStart = 0;
            lpCandList->dwSelection = 0;
        }
        else if (lpCandList->dwSelection >= MAXCANDPAGESIZE)
        {
            if (lpCandList->dwPageStart + MAXCANDPAGESIZE < lpCandList->dwCount)
                 lpCandList->dwPageStart++;
        }

        //
        // Generate messages.
        //
        GnMsg.msg = WM_IME_NOTIFY;
        GnMsg.wParam = IMN_CHANGECANDIDATE;
        GnMsg.lParam = 1L;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

        //
        // If the selected candidate string is changed, the composition string
        // should be updated.
        //
        lpstr = (LPTSTR)((LPBYTE)lpCandList +
                   lpCandList->dwOffset[lpCandList->dwSelection]);
        goto set_compstr;
 
    }

    ImmUnlockIMCC(lpIMC->hCandInfo);

cvk_exit10:
    ImmUnlockIMCC(lpIMC->hCompStr);

cvk_exit20:
    ImmUnlockIMC(hIMC);
    return( FALSE );
}

/**********************************************************************/
/*                                                                    */
/* IsEat( code )                                                      */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL IsEat( code )
register TCHAR code;
{
    return( (code >= 0x20 && 0x7f >= code) || (code >= 0xff61 && 0xff9f >= code) ? TRUE : FALSE );
}

/**********************************************************************/
/*                                                                    */
/* DeleteChar()                                                       */
/*                                                                    */
/**********************************************************************/
void PASCAL DeleteChar( HIMC hIMC ,UINT uVKey)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING pComp;
    LPCANDIDATEINFO lpCandInfo;
    LPTSTR lpstr;
    LPTSTR lpread;
    LPTSTR lpptr;
    int nChar;
    BOOL fDone = FALSE;
    DWORD dwCurPos;
    GENEMSG GnMsg;


    if (!IsCompStr(hIMC))
        return;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return;

    pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

    dwCurPos = pComp->dwCursorPos;
    lpstr = GETLPCOMPSTR(pComp);

    if( uVKey ==  VK_BACK )
    {
    //
    // delete the character in front of cursor
    // how nice it is that we don't need to worry about those
    // multi-byte characters!
    //
        if( dwCurPos == 0 )
            goto dc_exit;

        dwCurPos -= 1;
        lpptr = lpstr+dwCurPos;
        lstrcpy( lpptr, lpptr+1);

        fDone = TRUE;
    } 
    else if( uVKey == VK_DELETE )
    {
    //
    // delete the character at the cursor
    // how nice it is that we don't need to worry about those
    // multi-byte characters!
    //
        lpptr = lpstr+dwCurPos;
        lstrcpy( lpptr, lpptr+1);

        fDone = TRUE;
    }

    if (fDone)
    {
        lpstr = GETLPCOMPSTR(pComp);
        lpread = GETLPCOMPREADSTR(pComp);
        ZenToHan(lpread, MAXCOMPSIZE, lpstr);

        lmemset(GETLPCOMPATTR(pComp),0,lstrlen(lpstr));
        lmemset(GETLPCOMPREADATTR(pComp),0,lstrlen(lpread));

        // 
        // make length
        // 
        pComp->dwCompStrLen = lstrlen(lpstr);
        pComp->dwCompReadStrLen = lstrlen(lpread);
        pComp->dwCompAttrLen = lstrlen(lpstr);
        pComp->dwCompReadAttrLen = lstrlen(lpread);

        pComp->dwCursorPos = dwCurPos;
        // DeltaStart is same of Cursor Pos at DeleteChar time.
        pComp->dwDeltaStart = dwCurPos;

        // 
        // make clause info
        // 
        SetClause(GETLPCOMPCLAUSE(pComp),lstrlen(lpstr));
        SetClause(GETLPCOMPREADCLAUSE(pComp),lstrlen(lpread));
        pComp->dwCompClauseLen = 8;
        pComp->dwCompReadClauseLen = 8;

        if (pComp->dwCompStrLen)
        {
            GnMsg.msg = WM_IME_COMPOSITION;
            GnMsg.wParam = 0;
            GnMsg.lParam = GCS_COMPALL | GCS_CURSORPOS | GCS_DELTASTART;
            GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
        }
        else
        {
            if (IsCandidate(lpIMC))
            {
                lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
                ClearCandidate(lpCandInfo);
                GnMsg.msg = WM_IME_NOTIFY;
                GnMsg.wParam = IMN_CLOSECANDIDATE;
                GnMsg.lParam = 1;
                GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
                ImmUnlockIMCC(lpIMC->hCandInfo);
            }

            ClearCompStr(pComp,CLR_RESULT_AND_UNDET);

            GnMsg.msg = WM_IME_COMPOSITION;
            GnMsg.wParam = 0;
            GnMsg.lParam = 0;
            GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

            GnMsg.msg = WM_IME_ENDCOMPOSITION;
            GnMsg.wParam = 0;
            GnMsg.lParam = 0;
            GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
        }
    }

dc_exit:
    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);
}


/**********************************************************************/
/*                                                                    */
/* AddChar()                                                          */
/*                                                                    */
/* One character add function                                         */
/*                                                                    */
/**********************************************************************/
void PASCAL AddChar( hIMC, code, uVKey )
HIMC hIMC;
TCHAR code;
UINT uVKey;
{
    LPTSTR lpchText;
    LPTSTR lpchInsert;
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING pComp;
    DWORD dwSize;
    GENEMSG GnMsg;
    DWORD dwGCR = 0L;

    lpIMC = ImmLockIMC(hIMC);

    if (ImmGetIMCCSize(lpIMC->hCompStr) < sizeof (MYCOMPSTR))
    {
        // Init time.
        dwSize = sizeof(MYCOMPSTR);
        lpIMC->hCompStr = ImmReSizeIMCC(lpIMC->hCompStr,dwSize);
        pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        pComp->dwSize = dwSize;
    }
    else
    {
        pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    }

    if ( pComp->dwCompStrLen == 0)
    {
        //pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        InitCompStr(pComp,CLR_RESULT_AND_UNDET);

        GnMsg.msg = WM_IME_STARTCOMPOSITION;
        GnMsg.wParam = 0;
        GnMsg.lParam = 0;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

    }
    else if (IsConvertedCompStr(hIMC))
    {
        MakeResultString(hIMC, FALSE);
        InitCompStr(pComp,CLR_UNDET);
        dwGCR = GCS_RESULTALL;
    }

    if( IsEat( code ) )
    {
        PTSTR pTmpBuf;
        PTCH  pTmpCh;
        TCHAR ch;
        DWORD dwConv;
        INT   iCursorPos;

        dwConv = lpIMC->fdwConversion;

        if ( dwConv & IME_CMODE_CHARCODE || ! dwConv & IME_CMODE_NATIVE ) {
            ch = (TCHAR)uVKey;
        } else {
            ch = code;
        }

        iCursorPos = (INT)pComp->dwCursorPos;
        pTmpBuf = (PTSTR)LocalAlloc( LMEM_FIXED, MAXCOMPSIZE * sizeof(TCHAR) );
        if ( pTmpBuf != NULL ) {

            // Get ConvMode from IMC.
            lpchText = GETLPCOMPSTR(pComp);
            lpchInsert = lpchText + iCursorPos;

            if ( dwConv & IME_CMODE_CHARCODE ) {
                if ( ! iswxdigit( ch ) ) {
                    MessageBeep( 0 );
                    goto ac_exit;
                }
            }

            pTmpCh = pTmpBuf;
            *pTmpCh = TEXT('\0');
            if ( iCursorPos > 0 ) {
                // stupid lstrcpyn need +1 for the null terminator
                lstrcpyn( pTmpBuf, lpchText, iCursorPos+1);
                pTmpCh += iCursorPos;
            }
            *pTmpCh++ = ch;
            lstrcpy( pTmpCh, lpchText+pComp->dwCursorPos );
            iCursorPos += 1;

            //
            // Do Romaji-Kana conversion only when IME_CMODE_NATIVE is set
            //
            if ( dwConv & IME_CMODE_ROMAN && dwConv & IME_CMODE_NATIVE ) {
                iCursorPos = RK_Convert( lpchText, 
                                         MAXCOMPSIZE, 
                                         pTmpBuf, 
                                         iCursorPos);
                if ( iCursorPos < 0 )
                    goto ac_exit;

            } else {
                lstrcpy( lpchText, pTmpBuf );
            }
            LocalFree( pTmpBuf );

            iCursorPos = MapCompositionString(lpchText, iCursorPos, dwConv);
            if ( iCursorPos < 0 )
                goto ac_exit;
            pComp->dwCursorPos = (DWORD)iCursorPos;
        }
    }
    UpdateCompositionString( hIMC );

    GnMsg.msg = WM_IME_COMPOSITION;
    GnMsg.wParam = 0;
    GnMsg.lParam = GCS_COMPALL | GCS_CURSORPOS | GCS_DELTASTART | dwGCR;
    GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

ac_exit:
    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);
}

//
// called when the the contents of composition
// string is changed.
//
//
VOID UpdateCompositionString( HIMC hIMC )
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING pComp;
    LPTSTR lpread;
    LPTSTR lpstr;

    lpIMC = ImmLockIMC(hIMC);
    if ( lpIMC == NULL ) {
        return;
    }

    pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if ( pComp == NULL ) {
        ImmUnlockIMC(hIMC);
        return;
    }

    lpstr = GETLPCOMPSTR(pComp);
    if ( *lpstr == TEXT('\0') ) {
        ImmUnlockIMCC(lpIMC->hCompStr);
        ImmUnlockIMC(hIMC);
        return;
    }

    // make reading string.
    lpread = GETLPCOMPREADSTR(pComp);
    ZenToHan (lpread, MAXCOMPSIZE, lpstr);
    
    if ( pComp->dwCursorPos > 0 )
        pComp->dwDeltaStart = pComp->dwCursorPos - 1;
    else
        pComp->dwDeltaStart = 0;

    //MakeAttrClause(pComp);
    lmemset(GETLPCOMPATTR(pComp),0, lstrlen(lpstr));
    lmemset(GETLPCOMPREADATTR(pComp),0, lstrlen(lpread));

    // make length
    pComp->dwCompStrLen = lstrlen(lpstr);
    pComp->dwCompReadStrLen = lstrlen(lpread);
    pComp->dwCompAttrLen = lstrlen(lpstr);
    pComp->dwCompReadAttrLen = lstrlen(lpread);

    // 
    // make clause info
    // 
    SetClause(GETLPCOMPCLAUSE(pComp),lstrlen(lpstr));
    SetClause(GETLPCOMPREADCLAUSE(pComp),lstrlen(lpread));
    pComp->dwCompClauseLen = 8;
    pComp->dwCompReadClauseLen = 8;


    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);
}


/**********************************************************************/
/*                                                                    */
/* DicKeydownHandler()                                                */
/*                                                                    */
/* WM_KEYDOWN handler for dictionary routine                          */
/*                                                                    */
/* wParam                                                             */
/* virtual key                                                        */
/*                                                                    */
/* lParam                                                             */
/* differ depending on wParam                                         */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL DicKeydownHandler( hIMC, uVKey, ch, lpbKeyState)
HIMC hIMC;
UINT uVKey;
TCHAR ch;
LPBYTE lpbKeyState;
{
    LPINPUTCONTEXT lpIMC;

    switch( uVKey ) 
    {
        case VK_ESCAPE:
            FlushText(hIMC);
            break;

        case VK_DELETE:
        case VK_BACK:
            DeleteChar(hIMC,uVKey);
            break;

        case VK_SPACE:
            ConvKanji(hIMC);
            break;

        case VK_F3:
            if (IsCTLPushed(lpbKeyState))
                ChangeMode(hIMC,TO_CMODE_ROMAN);
            break;

        case VK_F6:
            if (IsCTLPushed(lpbKeyState))
                ChangeMode(hIMC,TO_CMODE_HIRAGANA);
            else
                ChangeCompStr(hIMC,TO_CMODE_HIRAGANA);
            break;

        case VK_F7:
            if (IsCTLPushed(lpbKeyState))
                ChangeMode(hIMC,TO_CMODE_KATAKANA);
            else
                ChangeCompStr(hIMC,TO_CMODE_KATAKANA);
            break;

        case VK_F8:
            if (IsCTLPushed(lpbKeyState))
                ChangeMode(hIMC,TO_CMODE_FULLSHAPE);
            else
                ChangeCompStr(hIMC,TO_CMODE_FULLSHAPE);
            break;

        case VK_F9:
            if (IsCTLPushed(lpbKeyState))
                ChangeMode(hIMC,TO_CMODE_ALPHANUMERIC);
            else
                ChangeCompStr(hIMC,TO_CMODE_ALPHANUMERIC);
            break;

        case VK_RETURN:
            lpIMC = ImmLockIMC(hIMC);

            if( !( lpIMC->fdwConversion & IME_CMODE_CHARCODE ) )
                MakeResultString(hIMC,TRUE);
            else
                FlushText(hIMC);

            ImmUnlockIMC(hIMC);
            break;

        default:
            break;
    }

    if (( VK_0 <= uVKey && VK_9 >= uVKey ) ||
        ( VK_A <= uVKey && VK_Z >= uVKey ) ||
        ( VK_NUMPAD0 <= uVKey && VK_NUMPAD9 >= uVKey ) ||
        ( VK_OEM_1 <= uVKey && VK_OEM_9 >= uVKey ) ||
        ( VK_MULTIPLY <= uVKey && VK_DIVIDE >= uVKey ))
    {
        AddChar( hIMC,  ch, uVKey );
    }
    else
        return( TRUE );
}

/**********************************************************************/
/*                                                                    */
/*  Entry    : MakeResultString( HIMC)                                */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI MakeResultString( HIMC hIMC, BOOL fFlag)
{
    GENEMSG GnMsg;
    LPCOMPOSITIONSTRING pComp;
    LPCANDIDATEINFO lpCandInfo;
    LPINPUTCONTEXT lpIMC;

    if (!IsCompStr(hIMC))
        return FALSE;

    lpIMC = ImmLockIMC(hIMC);

    pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

    if (IsCandidate(lpIMC))
    {
        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
        ClearCandidate(lpCandInfo);
        ImmUnlockIMCC(lpIMC->hCandInfo);
        GnMsg.msg = WM_IME_NOTIFY;
        GnMsg.wParam = IMN_CLOSECANDIDATE;
        GnMsg.lParam = 1L;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
    }

    lstrcpy(GETLPRESULTSTR(pComp),GETLPCOMPSTR(pComp));
    lstrcpy(GETLPRESULTREADSTR(pComp),GETLPCOMPREADSTR(pComp));


    pComp->dwResultStrLen = pComp->dwCompStrLen;
    pComp->dwResultReadStrLen = pComp->dwCompReadStrLen;
    pComp->dwCompStrLen = 0;
    pComp->dwCompReadStrLen = 0;

    // 
    // make clause info
    // 
    SetClause(GETLPRESULTCLAUSE(pComp),lstrlen(GETLPRESULTSTR(pComp)));
    SetClause(GETLPRESULTREADCLAUSE(pComp),lstrlen(GETLPRESULTREADSTR(pComp)));
    pComp->dwResultClauseLen = 8;
    pComp->dwResultReadClauseLen = 8;

    ImmUnlockIMCC(lpIMC->hCompStr);

    if (fFlag)
    {
        GnMsg.msg = WM_IME_COMPOSITION;
        GnMsg.wParam = 0;
        GnMsg.lParam = GCS_RESULTALL;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

        GnMsg.msg = WM_IME_ENDCOMPOSITION;
        GnMsg.wParam = 0;
        GnMsg.lParam = 0;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
    }

    ImmUnlockIMC(hIMC);

    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/*      MakeGuideLine()                                               */
/*                                                                    */
/*      Update the transrate key buffer.                              */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL MakeGuideLine(HIMC hIMC, DWORD dwID)
{

    LPINPUTCONTEXT lpIMC;
    LPGUIDELINE    lpGuideLine;
    GENEMSG GnMsg;
    DWORD dwSize = sizeof(GUIDELINE) + (MAXGLCHAR * sizeof(TCHAR)) + 2;
    LPTSTR lpStr;

    lpIMC = ImmLockIMC(hIMC);
    lpIMC->hGuideLine = ImmReSizeIMCC(lpIMC->hGuideLine,dwSize);
    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

    lpGuideLine->dwSize = dwSize;
    lpGuideLine->dwLevel = glTable[dwID].dwLevel;
    lpGuideLine->dwIndex = glTable[dwID].dwIndex;
    lpGuideLine->dwStrOffset = sizeof(GUIDELINE) + 1;
    lpStr = (LPTSTR)(((PBYTE)lpGuideLine) + lpGuideLine->dwStrOffset);
    LoadString(hInst,glTable[dwID].dwStrID,lpStr, MAXGLCHAR);
    lpGuideLine->dwStrLen = lstrlen(lpStr);

    GnMsg.msg = WM_IME_NOTIFY;
    GnMsg.wParam = IMN_GUIDELINE;
    GnMsg.lParam = 0;
    GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);

    ImmUnlockIMCC(lpIMC->hGuideLine);
    ImmUnlockIMC(hIMC);

    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/*      GenerateMessage()                                             */
/*                                                                    */
/*      Update the transrate key buffer.                              */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL GenerateMessage(HIMC hIMC, LPINPUTCONTEXT lpIMC, LPDWORD lpdwTransKey,LPGENEMSG lpGeneMsg)
{
    if (lpdwTransKey)
        return GenerateMessageToTransKey(lpdwTransKey,lpGeneMsg);
    
    if (IsWindow(lpIMC->hWnd))
    {
        LPDWORD lpdw;
        if (!(lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf,
                                 sizeof(DWORD) * (lpIMC->dwNumMsgBuf +1) * 3)))
            return FALSE;

        if (!(lpdw = (LPDWORD)ImmLockIMCC(lpIMC->hMsgBuf)))
            return FALSE;

        lpdw += (lpIMC->dwNumMsgBuf) * 3;
        *((LPGENEMSG)lpdw) = *lpGeneMsg;
        ImmUnlockIMCC(lpIMC->hMsgBuf);
        lpIMC->dwNumMsgBuf++;

        ImmGenerateMessage(hIMC);
    }
    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/*  Entry    : CheckAttr( LPCOMPOSITIONSTRING)                           */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL CheckAttr( LPCOMPOSITIONSTRING pComp)
{
    int i,len;
    LPBYTE lpb = GETLPCOMPATTR(pComp);

    len = pComp->dwCompAttrLen;
    for (i = 0; i < len; i++)
        if (*lpb++ & 0x01)
            return TRUE;

    return FALSE;
}

/**********************************************************************/
/*                                                                    */
/*  Entry    : MakeAttrClause( LPCOMPOSITIONSTRING)                         */
/*                                                                    */
/**********************************************************************/
void PASCAL MakeAttrClause( LPCOMPOSITIONSTRING pComp)
{
    int len = pComp->dwCompAttrLen;
    int readlen = pComp->dwCompReadAttrLen;
    PBYTE pb;
    PDWORD pdw;
    DWORD dwCursorPos = pComp->dwCursorPos;
    int i;

    if (len != readlen)
        return;

    pb = GETLPCOMPATTR(pComp);
    for (i = 0;i < len; i++)
    {
        if ((DWORD)i < dwCursorPos)
            *pb++ = 0x10;
        else
            *pb++ = 0x00;
    }

    pb = GETLPCOMPREADATTR(pComp);
    for (i = 0;i < readlen; i++)
    {
        if ((DWORD)i < dwCursorPos)
            *pb++ = 0x10;
        else
            *pb++ = 0x00;
    }

    pdw = GETLPCOMPCLAUSE(pComp);
    *pdw++ = 0;
    *pdw = len;

    pdw = GETLPCOMPREADCLAUSE(pComp);
    *pdw++ = 0;
    *pdw = len;
}


BOOL ZenToHan(LPTSTR lpDst, INT cDst, LPTSTR lpSrc)
{
    int i;
    DWORD lcmap_flag = LCMAP_HALFWIDTH |
                       LCMAP_KATAKANA  |
                       LCMAP_LOWERCASE;

    i =  LCMapString( JAPANESE_LOCALE,  // LCID Locale, locale identifier 
                      lcmap_flag,       // dwMapFlags, mapping transformation type 
                      lpSrc,            // LPCTSTR lpSrcStr, address of source string 
                      -1,               // number of characters in source string 
                      lpDst,            // address of destination buffer 
                      cDst );           // size of destination buffer 
    if ( i > 0 ) 
        return TRUE;
    else
        return FALSE;
}

BOOL HanToZen(LPTSTR lpDst, INT cDst, LPTSTR lpSrc,DWORD fdwConversion)
{
    int i;
    DWORD lcmap_flag;

    if ( fdwConversion &  IME_CMODE_KATAKANA )
        lcmap_flag = LCMAP_FULLWIDTH | LCMAP_KATAKANA;
    else
        lcmap_flag = LCMAP_FULLWIDTH | LCMAP_HIRAGANA;

    i =  LCMapString( JAPANESE_LOCALE,  // LCID Locale, locale identifier 
                      lcmap_flag,       // dwMapFlags, mapping transformation type 
                      lpSrc,            // LPCTSTR lpSrcStr, address of source string 
                      -1,               // number of characters in source string 
                      lpDst,            // address of destination buffer 
                      cDst );           // size of destination buffer 
    if ( i > 0 ) 
        return TRUE;
    else
        return FALSE;
}

//
// calls LCMapString. 
// returns -1 if error ( mostly buffer over flow )
// returns the new cursor position
//
INT MapString( 
  PTSTR pDst,   // destination buffer
  INT cDst,     // the size of destination buffer(including the null terminaator)
  PTSTR pSrc,   // source string (must be null terminated)
  DWORD dwFlag, // map flag
  INT iCursorPos // cursor position (starting at 0)
)
{
    INT cSrc;
    INT iNewCursorPos = -1;
    INT i;

    //
    // if the cursor is not at the middle of the source string
    // we don't need to split the source string.
    //
    cSrc = lstrlen( pSrc );
    if ( iCursorPos == 0 ) {
       iNewCursorPos = 0; 
    } else {
         
        i =  LCMapString( JAPANESE_LOCALE,  // LCID Locale, locale identifier
                          dwFlag,           // dwMapFlags, mapping transformation type
                          pSrc,             // LPCTSTR lpSrcStr, address of source string
                          iCursorPos,       // number of characters in source string
                          pDst,             // address of destination buffer
                          cDst );           // size of destination buffer
        if ( i <= 0 || i == cDst ) {
            return -1;
        }
        iNewCursorPos = i;
        *(pDst + i) = TEXT('\0');
    }

    if ( iCursorPos != cSrc ) {
        i =  LCMapString( JAPANESE_LOCALE,  
                          dwFlag,           
                          pSrc + iCursorPos,             
                          cSrc - iCursorPos,
                          pDst + iNewCursorPos,             
                          cDst - iNewCursorPos);           
        if ( i < 0 || i == (cDst - iNewCursorPos) ) {
            return -1;
        }
        *(pDst + iNewCursorPos + i) = TEXT('\0');
    }
    return iNewCursorPos;
}

INT MapCompositionString(
    PTSTR  pCompStr, 
    INT    iCursorPos, 
    DWORD  dwConv)
{
    PTSTR pTmp;

    pTmp = (PTSTR)LocalAlloc( LMEM_FIXED, MAXCOMPSIZE * sizeof(TCHAR) );
    if ( pTmp == NULL ) {
        return -1;
    }
    lstrcpy( pTmp, pCompStr );

    if ( dwConv & IME_CMODE_FULLSHAPE ) {

        if ( dwConv & IME_CMODE_KATAKANA ) {
        //
        // zenkaku katakana
        //
            iCursorPos = MapString( pCompStr,
                                    MAXCOMPSIZE,
                                    pTmp,
                                    LCMAP_FULLWIDTH | LCMAP_KATAKANA, 
                                    iCursorPos );

        } else if ( dwConv & IME_CMODE_NATIVE ) {
        //
        // zenkaku hiragana
        //
            iCursorPos = MapString( pCompStr,
                                    MAXCOMPSIZE,
                                    pTmp, 
                                    LCMAP_FULLWIDTH | LCMAP_HIRAGANA, 
                                    iCursorPos );
        } else {
        //
        // zenkaku alphanumeric
        //
            iCursorPos = MapString( pCompStr,
                                    MAXCOMPSIZE,
                                    pTmp, 
                                    LCMAP_FULLWIDTH,
                                    iCursorPos );
        }

    } else {

        if ( dwConv & IME_CMODE_NATIVE ) {
    //
    // hankaku kana
    //
            iCursorPos = MapString( pCompStr,
                                    MAXCOMPSIZE,
                                    pTmp, 
                                    LCMAP_HALFWIDTH | LCMAP_KATAKANA, 
                                    iCursorPos );
        } else {
    //
    // hankaku alphanumeric
    //
                iCursorPos = MapString( pCompStr,
                                        MAXCOMPSIZE,
                                        pTmp, 
                                        LCMAP_HALFWIDTH, 
                                        iCursorPos );
        }
    } 
    LocalFree( pTmp );
    return iCursorPos;
}
