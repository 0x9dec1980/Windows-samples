#include "precomp.h"
#include "immsec.h"

/**********************************************************************/
/*                                                                    */
/*      FAKEIME.C - Windows 95 FAKEIME                                */
/*                                                                    */
/*      Copyright (c) 1994-1996  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/*    DLLEntry()                                                      */
/**********************************************************************/
BOOL WINAPI DLLEntry (
    HINSTANCE    hInstDLL,
    DWORD        dwFunction,
    LPVOID       lpNot)
{
    PTSTR lpDicFileName;

    switch(dwFunction)
    {
        PSECURITY_ATTRIBUTES psa;

        case DLL_PROCESS_ATTACH:
            //
            // Create/open a system global named mutex.
            // The initial ownership is not needed.
            // CreateSecurityAttributes() will create
            // the proper security attribute for IME.
            //
            psa = CreateSecurityAttributes(); 
            if ( psa == NULL ) { 
                return FALSE;
            }
            hMutex = CreateMutex( psa, FALSE, TEXT("FakeIme_Mutex"));
            FreeSecurityAttributes( psa );
            if ( hMutex == NULL ) {
                return FALSE;
            }

            //
            // open Romaji-Kana conversion defnition file
            //
            if ( ! RK_OpenDefFile() )
                return FALSE;

            hInst= hInstDLL;

#ifdef DEBUG
{
char szDev[80];
wsprintf(szDev,"DLLEntry hInst is %lx\r\n",hInst);
OutputDebugString(szDev);
}
#endif
            IMERegisterClass( hInst );

            // Initialize for FAKEIME.
            lpDicFileName = (LPTSTR)&szDicFileName;
            lpDicFileName += GetWindowsDirectory(lpDicFileName,256);
            if (*(lpDicFileName-1) != TEXT('\\') )
                *lpDicFileName++ = TEXT('\\');
            LoadString( hInst, IDS_DICFILENAME, lpDicFileName, 128);
            break;

        case DLL_PROCESS_DETACH:
            UnregisterClass(szUIClassName,hInst);
            UnregisterClass(szCompStrClassName,hInst);
            UnregisterClass(szCandClassName,hInst);
            UnregisterClass(szStatusClassName,hInst);
            CloseHandle( hMutex );
            break;
    }
    return TRUE;
}
