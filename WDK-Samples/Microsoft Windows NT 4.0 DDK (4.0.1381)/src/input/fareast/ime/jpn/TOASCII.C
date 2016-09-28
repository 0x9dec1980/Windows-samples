/**********************************************************************/
/*                                                                    */
/*      TOASCII.C - Windows 95 FAKEIME                                */
/*                                                                    */
/*      Copyright (c) 1994-1995  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/
#include "precomp.h"

/**********************************************************************
*      ImeToAsciiEx                                                  
*                                                                    
*
* if we're Unicode IME
*
*      LOBYTE(LOWORD(uVKey)) : virtual key code
*      HIBYTE(LOWORD(uVKey)) : not used
*      HIWORD(uVKey)         : unicode character code (16bit)
*
* if we're ANSI IME
*
*      LOBYTE(LOWORD(uVKey)) : virtual key code
*      HIBYTE(LOWORD(uVKey)) : character code (8bit)
*      HIWORD(uVKey)         : not used
*
**********************************************************************/
UINT WINAPI ImeToAsciiEx (UINT uVKey,UINT uScanCode,CONST LPBYTE lpbKeyState,LPDWORD lpdwTransKey,UINT fuState,HIMC hIMC)
{
    LPARAM lParam;
    LPINPUTCONTEXT lpIMC;
    BOOL fOpen;

    lpdwCurTransKey = lpdwTransKey;
    lParam = ((DWORD)uScanCode << 16) + 1L;
    
    // Init uNumTransKey here.
    uNumTransKey = 0;

    // if hIMC is NULL, this means DISABLE IME.
    if (!hIMC)
        return 0;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return 0;

    fOpen = lpIMC->fOpen;

    ImmUnlockIMC(hIMC);

    // The current status of IME is "closed".
    if (!fOpen)
        goto itae_exit;

    if (uScanCode & 0x8000)
        IMEKeyupHandler( hIMC, uVKey, lParam, lpbKeyState);
    else
        IMEKeydownHandler( hIMC, uVKey, lParam, lpbKeyState);


    // Clear static value, no more generated message!
    lpdwCurTransKey = NULL;

itae_exit:

    // If trans key buffer that is allocated by USER.EXE full up,
    // the return value is the negative number.
    if (fOverTransKey)
    {
#ifdef DEBUG
OutputDebugString((LPSTR)"***************************************\r\n");
OutputDebugString((LPSTR)"*   TransKey OVER FLOW Messages!!!    *\r\n");
OutputDebugString((LPSTR)"*                by FAKEIME.DLL       *\r\n");
OutputDebugString((LPSTR)"***************************************\r\n");
#endif
        return (int)uNumTransKey;
    }

    return (int)uNumTransKey;
}


/**********************************************************************/
/*      GenerateMessageToTransKey()                                   */
/*                                                                    */
/*      Update the transrate key buffer.                              */
/**********************************************************************/
BOOL PASCAL GenerateMessageToTransKey(LPDWORD lpdwTransKey,LPGENEMSG lpGeneMsg)
{
    LPGENEMSG lpgmT0;

    uNumTransKey++;
    if (uNumTransKey >= (UINT)*lpdwTransKey)
    {
        fOverTransKey = TRUE;
        return FALSE;
    }

    lpgmT0= (LPGENEMSG)(lpdwTransKey+1) + (uNumTransKey - 1);
    *lpgmT0= *lpGeneMsg;

    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/* IMEKeydownHandler()                                                */
/*                                                                    */
/* A function which handles WM_IMEKEYDOWN                             */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL IMEKeydownHandler( hIMC, wParam, lParam, lpbKeyState)
HIMC hIMC;
WPARAM wParam;
LPARAM lParam;
LPBYTE lpbKeyState;
{
    UINT uVKey;
    TCHAR ch;

    uVKey = LOBYTE(LOWORD(wParam));
    switch( uVKey ){
        case VK_SHIFT:
        case VK_CONTROL:
            //goto not_proccessed;
            break;

        default:
#ifdef UNICODE
            ch = (TCHAR)HIWORD(wParam);
#else
            ch = (TCHAR)HIBYTE(LOWORD(wParam));
#endif
            DicKeydownHandler( hIMC, uVKey, ch, lpbKeyState );
            break;
    }
    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/* IMEKeyupHandler()                                                  */
/*                                                                    */
/* A function which handles WM_IMEKEYUP                               */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL IMEKeyupHandler( hIMC, wParam, lParam ,lpbKeyState)
HIMC hIMC;
WPARAM wParam;
LPARAM lParam;
LPBYTE lpbKeyState;
{
    return FALSE;
}


