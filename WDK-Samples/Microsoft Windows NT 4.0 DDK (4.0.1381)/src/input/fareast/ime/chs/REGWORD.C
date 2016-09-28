/**********************************************************************/
/*      REGWORD.C - register word into dictionary of IME              */
/*                                                                    */
/*      Copyright (c) 1994 Microsoft Corporation                      */
/**********************************************************************/

#include <windows.h>
#include <imm.h>
#include <imedefs.h>

/**********************************************************************/
/* ImeRegsisterWord                                                   */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeRegisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{
    return (FALSE);
}


/**********************************************************************/
/* ImeUnregsisterWord                                                 */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeUnregisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{
    return (FALSE);
}

/**********************************************************************/
/* ImeGetRegsisterWordStyle                                           */
/* Return Value:                                                      */
/*      number of styles copied/required                              */
/**********************************************************************/
UINT WINAPI ImeGetRegisterWordStyle(
    UINT       nItem,
    LPSTYLEBUF lpStyleBuf)
{
    return (FALSE);
}

#if !defined(ROMANIME)
/**********************************************************************/
/* PatternToReading                                                   */
/**********************************************************************/
void PASCAL PatternToReading(
    DWORD  dwPattern,
    LPTSTR lpszReading)
{
    int i;

    i = lpImeL->nMaxKey;

    *(LPTSTR)((LPBYTE)lpszReading + sizeof(WORD) * i) = TEXT('\0');


    return;
}
#endif

/**********************************************************************/
/* ImeEnumRegisterWord                                                */
/* Return Value:                                                      */
/*      the last value return by the callback function                */
/**********************************************************************/
UINT WINAPI ImeEnumRegisterWord(
    REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
    LPCTSTR              lpszReading,
    DWORD                dwStyle,
    LPCTSTR              lpszString,
    LPVOID               lpData)
{
    return (FALSE);
}
