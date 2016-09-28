/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    Remove.c

Abstract:

    Console app for the removal of Device Drivers in Windows 2000.

Environment:

    user mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

  9/26/99: Created Keith S. Garner.

--*/


#include <stdio.h> 
#include <tchar.h> // Make all functions UNICODE safe.
#include <windows.h>  
#include <setupapi.h> // for SetupDiXxx functions.


int DisplayError(TCHAR * ErrorName)
/*++
Routine Description:

    This Routine will display the LastError in human readable 
    form when possible.

    If the return value is a 32-bit number, and falls in the range:
        ERROR_NO_ASSOCIATED_CLASS   0xE0000200 
    To
        ERROR_CANT_REMOVE_DEVINST   0xE0000232 
    The values defined in setupapi.h can help to determine the error.
    Start by searching for the text string ERROR_NO_ASSOCIATED_CLASS.
  
Arguments:
    
    ErrorName: Human readable description of the last Function called.

Return Value:
    
    Allways returns FALSE.
      
--*/
{
    DWORD Err = GetLastError();
    LPVOID lpMessageBuffer = NULL;
    
    if (FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, 
        Err,  
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMessageBuffer,  
        0,  
        NULL ))
        _tprintf(TEXT("%s FAILURE: %s\n"),ErrorName,(TCHAR *)lpMessageBuffer);
    else 
        _tprintf(TEXT("%s FAILURE: (0x%08x)\n"),ErrorName,Err);
    
    if (lpMessageBuffer) LocalFree( lpMessageBuffer ); // Free system buffer 
    
    SetLastError(Err);    
    return FALSE;
}

int __cdecl _tmain(int argc, _TCHAR **argv, _TCHAR **envp)
/*++
Routine Discription:

    Entry point to Remove.exe.
    Parse the command line, call subroutines.

Arguments:
    
    Standard console 'c' application arguments.

    argv[1] - PnP HardwareID of devices to remove.

Return Value:
    
    Standard Console ERRORLEVEL values:

    0 - Remove Successfull
    2 - Remove Failure.
    
--*/
{
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i,err;

    //
    // Verify the Arguments.
    //
    if (argc < 2)
    {
        _tprintf(TEXT("usage: remove <Hardware_ID>\n"));
        return 1; // Remove Failure
    }

    //
    // Create a Device Information Set with all present devices.
    //
    DeviceInfoSet = SetupDiGetClassDevs(NULL, // All Classes
        0,
        0, 
        DIGCF_ALLCLASSES | DIGCF_PRESENT ); // All devices present on system
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        DisplayError(TEXT("GetClassDevs(All Present Devices)"));        
        return 1;
    }
    
    //
    //  Enumerate through all Devices.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i=0;SetupDiEnumDeviceInfo(DeviceInfoSet,i,&DeviceInfoData);i++)
    {
        DWORD DataT;
        LPTSTR p,buffer = NULL;
        DWORD buffersize = 0;
        
        //
        // We won't know the size of the HardwareID buffer until we call
        // this function. So call it with a null to begin with, and then 
        // use the required buffer size to Alloc the nessicary space.
        // Keep calling we have success or an unknown failure.
        //
        while (!SetupDiGetDeviceRegistryProperty(
            DeviceInfoSet,
            &DeviceInfoData,
            SPDRP_HARDWAREID,
            &DataT,
            (PBYTE)buffer,
            buffersize,
            &buffersize))
        {
            if (GetLastError() == ERROR_INVALID_DATA)
            {
                //
                // May be a Legacy Device with no HardwareID. Continue.
                //
                break;
            }
            else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                //
                // We need to change the buffer size.
                //
                if (buffer) 
                    LocalFree(buffer);
                buffer = LocalAlloc(LPTR,buffersize);
            }
            else
            {
                //
                // Unknown Failure.
                //
                DisplayError(TEXT("GetDeviceRegistryProperty"));
                goto cleanup_DeviceInfo;
            }            
        }

        if (GetLastError() == ERROR_INVALID_DATA) 
            continue;
        
        //
        // Compare each entry in the buffer multi-sz list with our HardwareID.
        //
        for (p=buffer;*p&&(p<&buffer[buffersize]);p+=lstrlen(p)+sizeof(TCHAR))
        {
            _tprintf(TEXT("Compare device ID: [%s]\n"),p);

            if (!_tcscmp(argv[1],p))
            {
                _tprintf(TEXT("Found! [%s]\n"),p);

                //
                // Worker function to remove device.
                //
                if (!SetupDiCallClassInstaller(DIF_REMOVE,
                    DeviceInfoSet,
                    &DeviceInfoData))
                {
                    DisplayError(TEXT("CallClassInstaller(REMOVE)"));
                }
                break;
            }
        }

        if (buffer) LocalFree(buffer);
    }

    if ((GetLastError()!=NO_ERROR)&&(GetLastError()!=ERROR_NO_MORE_ITEMS))
    {
        DisplayError(TEXT("EnumDeviceInfo"));
    }
    
    //
    //  Cleanup.
    //    
cleanup_DeviceInfo:
    err = GetLastError();
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    
    return !(err == NO_ERROR); 
}

