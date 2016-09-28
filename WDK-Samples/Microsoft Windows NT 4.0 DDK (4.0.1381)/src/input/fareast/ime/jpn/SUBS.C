/**********************************************************************/
/*                                                                    */
/*      SUBS.C - Windows 95 FAKEIME                                   */
/*                                                                    */
/*      Copyright (c) 1994-1995  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/

#include "precomp.h"

/**********************************************************************/
/*                                                                    */
/*      InitCompStr()                                                  */
/*                                                                    */
/**********************************************************************/
void PASCAL InitCompStr(LPCOMPOSITIONSTRING pComp,DWORD dwClrFlag)
{
    pComp->dwSize = sizeof(MYCOMPSTR);

    if (dwClrFlag & CLR_UNDET)
    {
        pComp->dwCompReadAttrOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->bCompReadAttr - (LPBYTE)pComp;
        pComp->dwCompReadClauseOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->dwCompReadClause - (LPBYTE)pComp;
        pComp->dwCompReadStrOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->szCompReadStr - (LPBYTE)pComp;
        pComp->dwCompAttrOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->bCompAttr - (LPBYTE)pComp;
        pComp->dwCompClauseOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->dwCompClause - (LPBYTE)pComp;
        pComp->dwCompStrOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->szCompStr - (LPBYTE)pComp;

        pComp->dwCompStrLen = 0;
        pComp->dwCompReadStrLen = 0;
        pComp->dwCompAttrLen = 0;
        pComp->dwCompReadAttrLen = 0;
        pComp->dwCompClauseLen = 0;
        pComp->dwCompReadClauseLen = 0;

        *GETLPCOMPSTR(pComp) = TEXT('\0');
        *GETLPCOMPREADSTR(pComp) = TEXT('\0');

        pComp->dwCursorPos = 0;
    }


    if (dwClrFlag & CLR_RESULT)
    {
        pComp->dwResultStrOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->szResultStr - (LPBYTE)pComp;
        pComp->dwResultClauseOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->dwResultClause - (LPBYTE)pComp;
        pComp->dwResultReadStrOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->szResultReadStr - (LPBYTE)pComp;
        pComp->dwResultReadClauseOffset = 
            (LPBYTE)((LPMYCOMPSTR)pComp)->dwResultReadClause - (LPBYTE)pComp;

        pComp->dwResultStrLen = 0;
        pComp->dwResultClauseLen = 0;
        pComp->dwResultReadStrLen = 0;
        pComp->dwResultReadClauseLen = 0;

        *GETLPRESULTSTR(pComp) = TEXT('\0');
        *GETLPRESULTREADSTR(pComp) = TEXT('\0');
    }

}

/**********************************************************************/
/*                                                                    */
/*      ClearCompStr()                                                */
/*                                                                    */
/**********************************************************************/
void PASCAL ClearCompStr(LPCOMPOSITIONSTRING pComp,DWORD dwClrFlag)
{
    pComp->dwSize = sizeof(MYCOMPSTR);

    if (dwClrFlag & CLR_UNDET)
    {
        pComp->dwCompStrOffset = 0;
        pComp->dwCompClauseOffset = 0;
        pComp->dwCompAttrOffset = 0;
        pComp->dwCompReadStrOffset = 0;
        pComp->dwCompReadClauseOffset = 0;
        pComp->dwCompReadAttrOffset = 0;
        pComp->dwCompStrLen = 0;
        pComp->dwCompClauseLen = 0;
        pComp->dwCompAttrLen = 0;
        pComp->dwCompReadStrLen = 0;
        pComp->dwCompReadClauseLen = 0;
        pComp->dwCompReadAttrLen = 0;
        ((LPMYCOMPSTR)pComp)->szCompStr[0] = TEXT('\0');
        ((LPMYCOMPSTR)pComp)->szCompReadStr[0] = TEXT('\0');
        pComp->dwCursorPos = 0;
    }

    if (dwClrFlag & CLR_RESULT)
    {
        pComp->dwResultStrOffset = 0;
        pComp->dwResultClauseOffset = 0;
        pComp->dwResultReadStrOffset = 0;
        pComp->dwResultReadClauseOffset = 0;
        pComp->dwResultStrLen = 0;
        pComp->dwResultClauseLen = 0;
        pComp->dwResultReadStrLen = 0;
        pComp->dwResultReadClauseLen = 0;
        ((LPMYCOMPSTR)pComp)->szResultStr[0] = TEXT('\0');
        ((LPMYCOMPSTR)pComp)->szResultReadStr[0] = TEXT('\0');
    }

}

/**********************************************************************/
/*                                                                    */
/*      ClearCandidate()                                              */
/*                                                                    */
/**********************************************************************/
void PASCAL ClearCandidate(LPCANDIDATEINFO lpCandInfo)
{
    lpCandInfo->dwSize = 0L;
    lpCandInfo->dwCount = 0L;
    lpCandInfo->dwOffset[0] = 0L;
    
    ((LPMYCAND)lpCandInfo)->cl.dwSize =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwStyle =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwCount =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwSelection =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwPageStart =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwPageSize =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwOffset[0] =0L;

}
/**********************************************************************/
/*                                                                    */
/*      ChangeMode()                                                  */
/*                                                                    */
/*    return value: fdwConversion                                        */
/*                                                                    */
/**********************************************************************/
void PASCAL ChangeMode(HIMC hIMC, DWORD dwToMode)
{
    LPINPUTCONTEXT lpIMC;
    DWORD fdwConversion;
    GENEMSG GnMsg;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return;

    fdwConversion = lpIMC->fdwConversion;

    switch (dwToMode)
    {
        case TO_CMODE_ALPHANUMERIC:
            fdwConversion = (fdwConversion & ~IME_CMODE_LANGUAGE);
            break;

        case TO_CMODE_KATAKANA:
            fdwConversion |= (IME_CMODE_NATIVE | IME_CMODE_KATAKANA);
            break;

        case TO_CMODE_HIRAGANA:
            fdwConversion = 
                ((fdwConversion & ~IME_CMODE_LANGUAGE) | IME_CMODE_NATIVE);
            fdwConversion |= IME_CMODE_FULLSHAPE;
            break;

        case TO_CMODE_FULLSHAPE:
            if (fdwConversion & IME_CMODE_FULLSHAPE)
            {
                // To SBCS mode.
                fdwConversion &= ~IME_CMODE_FULLSHAPE;
 
                // If hiragana mode, make it katakana mode.
                if ((fdwConversion & IME_CMODE_LANGUAGE) == IME_CMODE_NATIVE)
                    fdwConversion |= (IME_CMODE_NATIVE | IME_CMODE_KATAKANA);
                
            }
            else
            {
                // To DBCS mode.
                fdwConversion |= IME_CMODE_FULLSHAPE;

            }
            break;

        case TO_CMODE_ROMAN:
            if (fdwConversion & IME_CMODE_ROMAN)
                fdwConversion &= ~IME_CMODE_ROMAN;
            else
                fdwConversion |= IME_CMODE_ROMAN;
            break;

        case TO_CMODE_CHARCODE:
            break;
        case TO_CMODE_TOOLBAR:
            break;
        default:
            break;
    }

    if (lpIMC->fdwConversion != fdwConversion)
    {
        lpIMC->fdwConversion = fdwConversion;
        GnMsg.msg = WM_IME_NOTIFY;
        GnMsg.wParam = IMN_SETCONVERSIONMODE;
        GnMsg.lParam = 0L;
        GenerateMessage(hIMC, lpIMC, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
    }

    ImmUnlockIMC(hIMC);
    return;    
}

/**********************************************************************/
/*                                                                    */
/*      ChangeCompStr()                                               */
/*                                                                    */
/**********************************************************************/
void PASCAL ChangeCompStr(HIMC hIMC, DWORD dwToMode)
{
    PINPUTCONTEXT pInputContext;
    PCOMPOSITIONSTRING pComp;
    PTSTR pTmpBuf = NULL;
    PTSTR pCompStr = NULL;
    INT  iCursorPos;
    BOOL fChange = FALSE;

    if ( (pInputContext = ImmLockIMC(hIMC)) == NULL ) {
        return;
    }

    if ( (pComp = (PCOMPOSITIONSTRING)ImmLockIMCC(pInputContext->hCompStr)) == NULL ) {
        ImmUnlockIMC(hIMC);
        return;
    }
    pCompStr = ((LPMYCOMPSTR)pComp)->szCompStr;
    iCursorPos = (INT)pComp->dwCursorPos;
    
    switch (dwToMode)
    {
        case TO_CMODE_ALPHANUMERIC:
            break;

        case TO_CMODE_KATAKANA:
            if ( ( pTmpBuf = LocalAlloc( LMEM_FIXED, MAXCOMPSIZE )) != NULL ) {
                lstrcpy( pTmpBuf, pCompStr );
                iCursorPos = MapString( pCompStr, 
                                        MAXCOMPSIZE, 
                                        pTmpBuf, 
                                        LCMAP_KATAKANA, 
                                        iCursorPos );
                fChange = TRUE;
            }
            break;

        case TO_CMODE_HIRAGANA:
            if ( ( pTmpBuf = LocalAlloc( LMEM_FIXED, MAXCOMPSIZE )) != NULL ) {
                lstrcpy( pTmpBuf, pCompStr );
                iCursorPos = MapString( pCompStr, 
                                        MAXCOMPSIZE, 
                                        pTmpBuf, 
                                        LCMAP_FULLWIDTH|LCMAP_HIRAGANA, 
                                        iCursorPos );
                fChange = TRUE;
            }
            break;

        case TO_CMODE_FULLSHAPE:
            break;

        case TO_CMODE_ROMAN:
            break;
    }


    if (fChange)
    {
        GENEMSG GnMsg;

        LocalFree( pTmpBuf );
        (INT)pComp->dwCursorPos = iCursorPos;

        GnMsg.msg = WM_IME_COMPOSITION;
        GnMsg.wParam = 0;
        GnMsg.lParam = GCS_COMPSTR;
        GenerateMessage(hIMC, pInputContext, lpdwCurTransKey,(LPGENEMSG)&GnMsg);
    }

    ImmUnlockIMCC(pInputContext->hCompStr);
    ImmUnlockIMC(hIMC);
    return;
}


/*****************************************************************************
*                                                                            *
* IsCompStr( hIMC )                                                          *
*                                                                            *
*****************************************************************************/
BOOL PASCAL IsCompStr(HIMC hIMC)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING pComp;
    BOOL fRet = FALSE;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return FALSE;

    if (ImmGetIMCCSize(lpIMC->hCompStr) < sizeof (COMPOSITIONSTRING))
    {
        ImmUnlockIMC(hIMC);
        return FALSE;
    }

    pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

    fRet = (pComp->dwCompStrLen > 0);

    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);

    return fRet;
}
/*****************************************************************************
*                                                                            *
* IsConvertedCompStr( hIMC )                                                 *
*                                                                            *
*****************************************************************************/
BOOL PASCAL IsConvertedCompStr(HIMC hIMC)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING pComp;
    BOOL fRet = FALSE;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return FALSE;

    if (ImmGetIMCCSize(lpIMC->hCompStr) < sizeof (MYCOMPSTR))
    {
        ImmUnlockIMC(hIMC);
        return FALSE;
    }

    pComp = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

    if (pComp->dwCompStrLen > 0)
        fRet = (((LPMYCOMPSTR)pComp)->bCompAttr[0] > 0);

    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);

    return fRet;
}
/*****************************************************************************
*                                                                            *
* IsCandidate( lpIMC )                                                       *
*                                                                            *
*****************************************************************************/
BOOL PASCAL IsCandidate(LPINPUTCONTEXT lpIMC)
{
    LPCANDIDATEINFO lpCandInfo;
    BOOL fRet = FALSE;

    if (ImmGetIMCCSize(lpIMC->hCandInfo) < sizeof (CANDIDATEINFO))
        return FALSE;

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);

    fRet = (lpCandInfo->dwCount > 0);

    ImmUnlockIMCC(lpIMC->hCandInfo);
    return fRet;
}
void PASCAL lmemset(PBYTE p, BYTE b, UINT cnt)
{
    register UINT i;
    register BYTE bt = b;
    for (i=0;i<cnt;i++)
        *p++ = bt;
}
