/**********************************************************************/
/*                                                                    */
/*      DATA.C - Windows 95 FAKEIME                                   */
/*                                                                    */
/*      Copyright (c) 1994-1996  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/

#include "precomp.h"

HANDLE      hMutex = NULL;
HINSTANCE   hInst;

/* for Translat */
LPDWORD lpdwCurTransKey= NULL;
UINT    uNumTransKey;
BOOL    fOverTransKey = FALSE;

/* for UI */
TCHAR    szUIClassName[]      = TEXT("FAKEIMEUI");
TCHAR    szCompStrClassName[] = TEXT("FAKEIMECompStr");
TCHAR    szCandClassName[]    = TEXT("FAKEIMECand");
TCHAR    szStatusClassName[]  = TEXT("FAKEIMEStatus");
TCHAR    szGuideClassName[]   = TEXT("FAKEIMEGuide");


MYGUIDELINE glTable[] = {
        {GL_LEVEL_ERROR,   GL_ID_NODICTIONARY, IDS_GL_NODICTIONARY},
        {GL_LEVEL_WARNING, GL_ID_TYPINGERROR,  IDS_GL_TYPINGERROR}
        };

/* for DIC */
TCHAR    szDicFileName[256];         /* Dictionary file name stored buffer */
