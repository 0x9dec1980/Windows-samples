/******************************Module*Header**********************************\
*
*       **************************************************
*       * Permedia 2: Direct Draw/Direct 3D  SAMPLE CODE *
*       **************************************************
*
* Module Name: dllmain.c
*
*       Main entry point to the 32bit DLL
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

// #include "precomp.h"
#include "glint.h"
#include "direct3d.h"
#include "..\..\gdi\dbgtags.c"

typedef LPVOID PPDEV;

//
// 2 helper functions (NT version in registry.c)
//

/******************************Public*Data*Struct**************************\
* VOID vGlintInitializeRegistryKey
*
\**************************************************************************/

VOID
vGlintInitializeRegistryKey(
    P3_THUNKEDDATA     *pThisDisplay)
{
    // Nothing to do for now
}


/******************************Public*Data*Struct**************************\
* BOOL bGlintQueryRegistryValueUlong
*
* Take a string and look up its value in the registry. We assume that the
* value fits into 4 bytes. Fill in the supplied DWORD pointer with the value.
*
* Returns:
*   TRUE if we found the string, FALSE if not. Note, if we failed to init
*   the registry the query funtion will simply fail and we act as though
*   the string was not defined.
*
\**************************************************************************/

BOOL
bGlintQueryRegistryValueUlong(
    LPTSTR              szValueStr, 
    PULONG              pulData)
{
    ULONG               ulRet;
    HKEY                hDevKey;
    DWORD               dwType, dwCBData, dwValue;

    //
    // Clear out the return value
    //

    *pulData = 0;

    //
    // There is service in MVD to retrieve the registry path, but ppdev need
    // to be some kind of unique id for this purpose
    //

    ulRet = RegOpenKey(
               HKEY_LOCAL_MACHINE,
               "Software\\3Dlabs",
               &hDevKey);
    if (ulRet != ERROR_SUCCESS) {
        return (FALSE);
    }

    //
    // Retrieve the value under the device key
    //

    dwCBData = sizeof(DWORD);

    ulRet = RegQueryValueEx(
                hDevKey,
                szValueStr,
                0,
                &dwType,
                (LPBYTE)pulData,
                &dwCBData);
                
    //
    // Close the registry key handle
    //

    RegCloseKey(hDevKey);

    if (ulRet != ERROR_SUCCESS) 
    {
        return (FALSE);
    }
    else
    {
        return (TRUE);
    }
}


//
// Stub only
//

BOOL 
bGlintSetRegistryValueString(
    LPTSTR              szValueStr, 
    ULONG               ulData)
{
    return (FALSE);
}


//
// Win9x specific functions
//

void 
Wait_2D_DMA_Complete(
    P3_THUNKEDDATA     *pThisDisplay)
{
}


void _cdecl 
ChangeContext(
    P3_THUNKEDDATA     *pThisDisplay, 
    LPGLINTINFO         pGLInfo,
    unsigned long       ulContext)
{

    pGLInfo->dwCurrentContext = ulContext;
}


void _cdecl 
SetEndIndex(
    LPGLINTINFO         pGLInfo, 
    unsigned long       ulParam, 
    unsigned short      ulShort)
{
}


void 
StartDMAProper(
    P3_THUNKEDDATA     *pThisDisplay,
    LPGLINTINFO         pGLInfo, 
    unsigned long       dwPhys, 
    unsigned long       dwVirt, 
    unsigned long       dwSize)
{
}

//-----------------------------------------------------------------------------
//
// SharedHeapAlloc
//
// Allocates memory from some sort of shared heap.
//
//-----------------------------------------------------------------------------
BOOL SharedHeapAlloc(DWORD* p16, ULONG_PTR* p32, DWORD dwSize)
{
    ALLOCREQUEST memreq;
    ALLOCREQUEST memret;
    HANDLE  hDevice = NULL;
    DWORD dwSizeRet;
    BOOL bResult = FALSE;

    memset(&memreq, 0, sizeof(ALLOCREQUEST));
    memreq.dwSize = sizeof(ALLOCREQUEST);
    memreq.dwFlags = GLINT_MEMORY_ALLOCATE;
    memreq.dwBytes = dwSize;
 
    hDevice = CreateFile("\\\\.\\perm3mvd", 
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        (LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL); 
    if (hDevice == (HANDLE) INVALID_HANDLE_VALUE) 
    { 
        DISPDBG((0, "ERROR: Invalid Handle"));
        return FALSE; 
    }
    else 
    { 
        DISPDBG((3, "Got handle"));
        bResult = DeviceIoControl(hDevice, 
                                  GLINT_MEMORY_REQUEST, 
                                  &memreq, 
                                  sizeof(ALLOCREQUEST), 
                                  &memret, 
                                  sizeof(ALLOCREQUEST), 
                                  &dwSizeRet, 
                                  0); 
 
        if (!bResult || (memret.ptr32 == 0)) 
        {
            DISPDBG((0,"ERROR: Memory allocation failed"));
            CloseHandle(hDevice); 
            return FALSE;
        }
        else
        {
            DISPDBG((0,"Allocated memory ptr32:0x%x ptr16:0x%x", 
                       memret.ptr32, memret.ptr16));
            *p16 = memret.ptr16;
            *p32 = memret.ptr32;
        }
    } 
    
    CloseHandle(hDevice); 

    return TRUE;
} // SharedHeapAlloc

//-----------------------------------------------------------------------------
//
// SharedHeapFree
//
// Allocates memory from some sort of shared heap.
//
//-----------------------------------------------------------------------------
void SharedHeapFree(DWORD ptr16, ULONG_PTR ptr32)
{
    ALLOCREQUEST memreq;
    ALLOCREQUEST memret;
    HANDLE  hDevice = NULL;
    DWORD dwSizeRet;
    BOOL bResult = FALSE;
    memreq.dwSize = sizeof(ALLOCREQUEST);
    memreq.dwFlags = GLINT_MEMORY_FREE;
    memreq.dwBytes = 0;
    memreq.ptr16 = ptr16;
    memreq.ptr32 = ptr32;
 
    hDevice = CreateFile("\\\\.\\perm3mvd", 
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        (LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL); 
 
    if (hDevice == (HANDLE) INVALID_HANDLE_VALUE) 
    { 
        DISPDBG((0, "ERROR: Invalid Handle"));
        return;
    }
    else 
    { 
        DISPDBG((3, "Got handle"));
        bResult = DeviceIoControl(hDevice, 
                                  GLINT_MEMORY_REQUEST, 
                                  &memreq, 
                                  sizeof(ALLOCREQUEST), 
                                  &memret, 
                                  sizeof(ALLOCREQUEST), 
                                  &dwSizeRet, 
                                  0); 
 
        if (!bResult) 
        {
            DISPDBG((0,"ERROR: Memory free failed"));
            CloseHandle(hDevice); 
            return;
        }
    } 
    
    CloseHandle(hDevice); 
    return;
    
} // SharedHeapFree



