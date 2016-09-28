/******************************Module*Header**********************************\
*
*       **************************************************
*       * Permedia 2: Direct Draw/Direct 3D  SAMPLE CODE *
*       **************************************************
*
* Module Name: debug.c
*
*       Debug functions
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifdef DEBUG

#include "ddrawi.h"
#include "debug.h"

static DWORD P2DLL_DebugLevel = 0;

DWORD  DebugFilter = (DEBUG_FILTER_D3D | DEBUG_FILTER_DD );

void __cdecl DebugPrint(DWORD DebugLevelPrint,  LPSTR szFormat, ... )
{
    if (DebugLevelPrint <= P2DLL_DebugLevel)
    {
        char    str[256];

        lstrcpy(str, "PERMEDIA2DLL:");
        wvsprintf( str+lstrlen( str ), szFormat, (LPVOID)(&szFormat+1) );
        lstrcat( (LPSTR) str, "" );
        OutputDebugString( str );
        OutputDebugString( "\r\n" );
    }

} /* Msg */


#endif