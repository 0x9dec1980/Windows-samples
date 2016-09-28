;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: DEBUG.C
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

#pragma warning(disable:4103)
#include <windows.h>


#include "debug.h"

#pragma optimize("", on)
#ifdef DEBUG

int DD_DebugLevel = -1;

void FAR __cdecl DPF(LPSTR szFormat, ...)
{
   static int (WINAPI *fpwvsprintf)(LPSTR lpszOut, LPCSTR lpszFmt, const void FAR* lpParams);
   char str[256];

   if (fpwvsprintf == NULL)
   {
      fpwvsprintf = (LPVOID) GetProcAddress(GetModuleHandle("USER"),"wvsprintf");
      if (fpwvsprintf == NULL)
         return;
   }


   lstrcpy(str, "PERMEDIA2: ");
   fpwvsprintf(str+lstrlen(str), szFormat, (LPVOID)(&szFormat+1));

   lstrcat(str, "\r\n");
   OutputDebugString(str);
}

void DebugPrint(int Value, LPSTR szFormat, ... )
{
   static int (WINAPI *fpwvsprintf)(LPSTR lpszOut, LPCSTR lpszFmt, const void FAR* lpParams);
   char str[256];

    if (Value <= DD_DebugLevel) 
    {
        if (fpwvsprintf == NULL)
        {
            fpwvsprintf = (LPVOID) GetProcAddress(GetModuleHandle("USER"),"wvsprintf");
            if (fpwvsprintf == NULL)
                return;
        }
        lstrcpy(str, "PERMEDIA2: ");
        fpwvsprintf(str+lstrlen(str), szFormat, (LPVOID)(&szFormat+1));
        lstrcat(str, "\r\n");
        OutputDebugString(str);
    }
}
#endif
#pragma optimize("", off)