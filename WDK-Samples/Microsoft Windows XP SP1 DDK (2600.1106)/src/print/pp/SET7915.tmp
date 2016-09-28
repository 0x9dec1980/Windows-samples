/*++ 

Copyright (c) 1990-1998 Microsoft Corporation
All Rights Reserved

Windows NT Printing DDK Samples

--*/



#include "pp.h"

PPORT pFirstPort = NULL;


BOOL
IsLocalMachine
(
    LPWSTR pszName
)
{
    if (!pszName || !*pszName)
    {
        return TRUE;
    }

    if (*pszName == L'\\' && *(pszName+1) == L'\\')
    {
        if (!lstrcmpi(pszName, szMachineName))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
PortExists
(
    LPWSTR  pPortName,
    LPDWORD pError
)

{
    DWORD cbNeeded;
    DWORD cReturned;
    DWORD cbPorts;
    LPPORT_INFO_1W pPorts;
    DWORD i;
    BOOL  Found = FALSE;

    *pError = NO_ERROR;

    if (!hSpoolssDll)
    {
        if (hSpoolssDll = LoadLibrary(L"SPOOLSS.DLL"))
        {
            pfnSpoolssEnumPorts = GetProcAddress(hSpoolssDll, "EnumPortsW");
            if (!pfnSpoolssEnumPorts)
            {
                *pError = GetLastError();
                FreeLibrary(hSpoolssDll);
                hSpoolssDll = NULL;
            }
        }
        else
        {
            *pError = GetLastError();
        }
    }

    if (!pfnSpoolssEnumPorts)
    {
        return FALSE;
    }

    if (!(*pfnSpoolssEnumPorts)(NULL, 1, NULL, 0, &cbNeeded, &cReturned))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            cbPorts = cbNeeded;

            EnterCriticalSection(&SplSem);

            pPorts = LocalAlloc(LMEM_ZEROINIT, cbPorts);
            if (pPorts)
            {
                if ((*pfnSpoolssEnumPorts)(NULL, 1, (LPBYTE)pPorts, cbPorts,
                                             &cbNeeded, &cReturned))
                {
                    for (i = 0; i < cReturned; i++)
                    {
                        if (!lstrcmpi( pPorts[i].pName, pPortName))
                        {
                            Found = TRUE;
                        }
                    }
                }
                else
                {
                    *pError = GetLastError();
                }
                LocalFree(pPorts);
            }
            else
            {
                *pError = ERROR_NOT_ENOUGH_MEMORY;
            }
            LeaveCriticalSection(&SplSem);
        }
        else
        {
            *pError = GetLastError();
        }
    }
    return Found;
}



BOOL
PortKnown
(
    LPWSTR   pPortName
)
{
    PPORT pPort;

    EnterCriticalSection(&SplSem);

    pPort = pFirstPort;

    while (pPort)
    {
        if (!lstrcmpi( pPort->pName, pPortName))
        {
            LeaveCriticalSection(&SplSem);
            return TRUE;
        }

        pPort = pPort->pNext;
    }
    LeaveCriticalSection(&SplSem);
    return FALSE;
}



PPORT
CreatePortEntry
(
    LPWSTR   pPortName
)
{
    PPORT pPort, pPortTemp;

    DWORD cb = sizeof(PORT) + (wcslen(pPortName) + 1) * sizeof(WCHAR);

    if (pPort = LocalAlloc(LMEM_ZEROINIT, cb))
    {
        pPort->pName = wcscpy((LPWSTR)(pPort+1), pPortName);
        pPort->cb = cb;
        pPort->pNext = NULL;

        EnterCriticalSection(&SplSem);

        if (pPortTemp = pFirstPort)
        {
            while (pPortTemp->pNext)
            {
                pPortTemp = pPortTemp->pNext;
            }
            pPortTemp->pNext = pPort;
        }
        else
        {
            pFirstPort = pPort;
        }

        LeaveCriticalSection(&SplSem);
    }

    return pPort;
}



BOOL
DeletePortEntry
(
    LPWSTR   pPortName
)

{
    BOOL fRetVal;
    PPORT pPort, pPrevPort;

    EnterCriticalSection(&SplSem);

    pPort = pFirstPort;
    while (pPort && lstrcmpi(pPort->pName, pPortName))
    {
        pPrevPort = pPort;
        pPort = pPort->pNext;
    }

    if (pPort)
    {
        if (pPort == pFirstPort)
        {
            pFirstPort = pPort->pNext;
        }
        else
        {
            pPrevPort->pNext = pPort->pNext;
        }
        LocalFree(pPort);
        fRetVal = TRUE;
    }
    else
    {
        fRetVal = FALSE;
    }
    LeaveCriticalSection(&SplSem);
    return fRetVal;
}



VOID
DeleteAllPortEntries
(
    VOID
)
{
    PPORT pPort, pNextPort;

    for (pPort = pFirstPort; pPort; pPort = pNextPort) 
    {
        pNextPort = pPort->pNext;
        LocalFree(pPort);
    }
}



DWORD
CreateRegistryEntry
(
    LPWSTR pPortName
)
{
    DWORD  err;
    HKEY   hkeyPath;
    HKEY   hkeyPortNames;

    err = RegCreateKeyEx( HKEY_LOCAL_MACHINE, pszRegistryPath, 0,
                          NULL, 0, KEY_WRITE, NULL, &hkeyPath, NULL );

    if (!err)
    {
        err = RegCreateKeyEx( hkeyPath, pszRegistryPortNames, 0,
                              NULL, 0, KEY_WRITE, NULL, &hkeyPortNames, NULL );

        if (!err)
        {
            err = RegSetValueEx( hkeyPortNames,
                                 pPortName,
                                 0,
                                 REG_SZ,
                                 (LPBYTE) L"",
                                 0 );

            RegCloseKey(hkeyPortNames);
        }
        RegCloseKey(hkeyPath);
    }
    return err;
}



DWORD
DeleteRegistryEntry
(
    LPWSTR pPortName
)
{
    DWORD  err;
    HANDLE hToken;
    HKEY   hkeyPath;
    HKEY   hkeyPortNames;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszRegistryPath, 0,
                        KEY_WRITE, &hkeyPath
                      );

    if (!err)
    {

        err = RegOpenKeyEx( hkeyPath, pszRegistryPortNames, 0,
                            KEY_WRITE, &hkeyPortNames );

        if (!err)
        {
            err = RegDeleteValue(hkeyPortNames, pPortName);
            RegCloseKey(hkeyPortNames);
        }
        RegCloseKey(hkeyPath);

    }

    return err;
    UNREFERENCED_PARAMETER(hToken);
}




LPWSTR
AllocSplStr
(
    LPWSTR pStr
)

{
    LPWSTR pMem;

    if (!pStr)
    {
        return NULL;
    }

    if (pMem = LocalAlloc(0, (wcslen(pStr) + 1) * sizeof(WCHAR)))
    {
        wcscpy(pMem, pStr);
    }

    return pMem;
}



VOID
FreeSplStr
(
   LPWSTR pStr
)

{
    if (pStr) 
    {
        LocalFree(pStr);
    }
}


BOOL
ValidateUNCName
(
   LPWSTR pName
)    

{
    if (   pName 
       && (*pName++ == L'\\') 
       && (*pName++ == L'\\') 
       && (wcschr(pName, L'\\'))
       )
    {
        return TRUE;
    }

    return FALSE;
}


