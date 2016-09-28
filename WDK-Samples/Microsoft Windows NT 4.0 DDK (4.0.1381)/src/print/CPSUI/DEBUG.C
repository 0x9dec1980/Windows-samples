/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    debug.c


Abstract:

    This module contains all debugging routines


Author:

    30-Aug-1995 Wed 19:01:07 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop

#if DBG


BOOL    DoCPSUIWarn = TRUE;



VOID
cdecl
CPSUIDbgPrint(
    LPSTR   pszFormat,
    ...
    )

/*++

Routine Description:

    This fucntion output the debug informat to the debugger


Arguments:

    pszFormat   - format string

    ...         - variable data


Return Value:


    VOID

Author:

    30-Aug-1995 Wed 19:10:34 updated  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    va_list         vaList;
    static TCHAR    OutBuf[768];
#ifdef UNICODE
    static WCHAR    FormatBuf[256];
#endif
    //
    // We assume that UNICODE flag is turn on for the compilation, bug the
    // format string passed to here is ASCII version, so we need to convert
    // it to LPWSTR before the wvsprintf()
    //

    va_start(vaList, pszFormat);

#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, 0, pszFormat, -1, FormatBuf, 256);
    wvsprintf(OutBuf, FormatBuf, vaList);
#else
    wvsprintf(OutBuf, pszFormat, vaList);
#endif
    va_end(vaList);

    OutputDebugString((LPTSTR)OutBuf);
    OutputDebugString(TEXT("\n"));
}




VOID
CPSUIDbgType(
    INT    Type
    )

/*++

Routine Description:

    this function output the ERROR/WARNING message


Arguments:

    Type

Return Value:


Author:

    30-Aug-1995 Wed 19:10:42 updated  -by-  Daniel Chou (danielc)

Revision History:


--*/

{
    static TCHAR    DebugDLLName[] = TEXT("SurPtrUI");

    if (Type) {

        OutputDebugString((Type < 0) ? TEXT("ERROR) ") :
                                       TEXT("WARNING: "));
    }

    OutputDebugString(DebugDLLName);
    OutputDebugString(TEXT("!"));
}


#endif  // DBG
