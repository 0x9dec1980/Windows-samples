/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    Install.c

Abstract:

    Console app for the installation of Device Drivers in Windows 2000.

Environment:

    user mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

  9/22/99: Created Keith S. Garner, with input from Eliyas and others.

--*/


#include <stdio.h> 
#include <tchar.h> // Make all functions UNICODE safe.
#include <windows.h>  
#include <newdev.h> // for the API UpdateDriverForPlugAndPlayDevices().
#include <setupapi.h> // for SetupDiXxx functions.

#define MAX_CLASS_NAME_LEN 32 // Stolen from <cfgmgr32.h>


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

BOOL FindExistingDevice(IN LPTSTR HardwareId)
/*++

Routine Description:

    This routine finds an existing devnode if present.

Arguments:

    HardwareIdList - Supplies a string containing a hardware ID to 
    be associated with the device.

Return Value:

    The function returns TRUE if it is successful. 

    Otherwise it returns FALSE and the logged error can be retrieved 
    with a call to GetLastError. 

    The most common error will be ERROR_NO_MORE_ITEMS, which means the 
    function could not find a devnode with the HardwareID.

--*/
{
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i,err;
    BOOL Found;
    
    //
    // Create a Device Information Set with all present devices.
    //
    DeviceInfoSet = SetupDiGetClassDevs(NULL, // All Classes
        0,
        0, 
        DIGCF_ALLCLASSES | DIGCF_PRESENT ); // All devices present on system
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        return DisplayError(TEXT("GetClassDevs(All Present Devices)"));        
    }
    
    _tprintf(TEXT("Search for Device ID: [%s]\n"),HardwareId);
    
    //
    //  Enumerate through all Devices.
    //
    Found = FALSE;
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
            
            if (!_tcscmp(HardwareId,p))
            {
                _tprintf(TEXT("Found! [%s]\n"),p);
                Found = TRUE;
                break;
            }
        }
        
        if (buffer) LocalFree(buffer);
        if (Found) break;
    }
    
    if (GetLastError() != NO_ERROR)
    {
        DisplayError(TEXT("EnumDeviceInfo"));
    }
    
    //
    //  Cleanup.
    //    
cleanup_DeviceInfo:
    err = GetLastError();
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    SetLastError(err);
    
    return err == NO_ERROR; //???
}

BOOL
InstallRootEnumeratedDriver(IN  LPTSTR HardwareId,
    IN  LPTSTR INFFile,
    OUT PBOOL  RebootRequired  OPTIONAL
    )
/*++

Routine Description:

    This routine creates and installs a new root-enumerated devnode.

Arguments:

    HardwareIdList - Supplies a multi-sz list containing one or more hardware
    IDs to be associated with the device.  These are necessary in order 
    to match up with an INF driver node when we go to do the device 
    installation.

    InfFile - Supplies the full path to the INF File to be used when 
    installing this device.

    RebootRequired - Optionally, supplies the address of a boolean that is 
    set, upon successful return, to indicate whether or not a reboot is 
    required to bring the newly-installed device on-line.

Return Value:

    The function returns TRUE if it is successful. 

    Otherwise it returns FALSE and the logged error can be retrieved 
    with a call to GetLastError. 

--*/
{
    HDEVINFO DeviceInfoSet = 0;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD err;
    
    //
    // Use the INF File to extract the Class GUID. 
    //
    if (!SetupDiGetINFClass(INFFile,&ClassGUID,ClassName,sizeof(ClassName),0))
    {
        return DisplayError(TEXT("GetINFClass"));
    }
    
    //
    // Create the container for the to-be-created Device Information Element.
    //
    DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID,0);
    if(DeviceInfoSet == INVALID_HANDLE_VALUE) 
    {
        return DisplayError(TEXT("CreateDeviceInfoList"));
    }
    
    // 
    // Now create the element. 
    // Use the Class GUID and Name from the INF file.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
        ClassName,
        &ClassGUID,
        NULL,
        0,
        DICD_GENERATE_ID,
        &DeviceInfoData))
    {
        DisplayError(TEXT("CreateDeviceInfo"));
        goto cleanup_DeviceInfo;
    }
    
    //
    // Add the HardwareID to the Device's HardwareID property.
    //
    if(!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
        &DeviceInfoData,
        SPDRP_HARDWAREID,
        (LPBYTE)HardwareId,
        (lstrlen(HardwareId)+1+1)*sizeof(TCHAR))) 
    {
        DisplayError(TEXT("SetDeviceRegistryProperty"));
        goto cleanup_DeviceInfo;
    }
    
    //
    // Transform the registry element into an actual devnode 
    // in the PnP HW tree.
    //
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        DeviceInfoSet,
        &DeviceInfoData))
    {
        DisplayError(TEXT("CallClassInstaller(REGISTERDEVICE)"));
        goto cleanup_DeviceInfo;
    }
    
    //
    // The element is now registered. We must explicitly remove the 
    // device using DIF_REMOVE, if we encounter any failure from now on.
    //
    
    //
    // Install the Driver.
    //
    if (!UpdateDriverForPlugAndPlayDevices(0,
        HardwareId,
        INFFile,
        INSTALLFLAG_FORCE,
        RebootRequired))
    {
        DWORD err = GetLastError();
        DisplayError(TEXT("UpdateDriverForPlugAndPlayDevices"));
        
        if (!SetupDiCallClassInstaller(
            DIF_REMOVE,
            DeviceInfoSet,
            &DeviceInfoData))
        {
            DisplayError(TEXT("CallClassInstaller(REMOVE)"));
        }
        SetLastError(err);
    }
    
    //
    //  Cleanup.
    //    
cleanup_DeviceInfo:
    err = GetLastError();
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    SetLastError(err);
    
    return err == NO_ERROR;
}


int __cdecl _tmain(int argc, _TCHAR **argv, _TCHAR **envp)
/*++
Routine Discription:

    Entry point to install.exe.
    Parse the command line, call subroutines.

Arguments:
    
    Standard console 'c' application arguments.

    argv[1] - Full path of INF file.
    argv[2] - PnP HardwareID of device.

Return Value:
    
    Standard Console ERRORLEVEL values:

    0 - Install Successfull, no reboot required.
    1 - Install Successfull, reboot required.
    2 - Install Failure.
    
--*/
{
    WIN32_FIND_DATA FindFileData;
    BOOL RebootRequired = 0; // Must be cleared.
    
    //
    // Verify the Arguments.
    //
    if (argc != 3)
    {
        _tprintf(TEXT("usage: install <INF_File> <Hardware_ID>\n"));
        return 2; // Install Failure
    }
    
    if (FindFirstFile(argv[1],&FindFileData)==INVALID_HANDLE_VALUE)
    {
        _tprintf(TEXT("  File not found.\n"));
        _tprintf(TEXT("usage: install <INF_File> <Hardware_ID>\n"));
        return 2; // Install Failure
    }
    
    //
    // Look to see if this device allready exists.
    //
    if (FindExistingDevice(argv[2]))
    {
        //
        // No Need to Create a Device Node, just call our API.
        //
        if (!UpdateDriverForPlugAndPlayDevices(0, // No Window Handle
            argv[2], // Hardware ID
            argv[1], // FileName
            INSTALLFLAG_FORCE,
            &RebootRequired))
        {
            DisplayError(TEXT("UpdateDriverForPlugAndPlayDevices"));
            return 2; // Install Failure
        }
    }
    else
    {
        if (GetLastError()!= ERROR_NO_MORE_ITEMS)
        {
            //
            // An unknown failure from FindExistingDevice()
            //
            return 2; // Install Failure
        }
        
        // 
        // Driver Does not exist, Create and call the API.
        // HardwareID must be a multi-sz string, which argv[2] is.
        //
        if (!InstallRootEnumeratedDriver(argv[2], // HardwareID
            argv[1], // FileName
            &RebootRequired))
        {
            return 2; // Install Failure
        }
    }
    
    _tprintf(TEXT("Driver Installed successfully.\n"));
    
    if (RebootRequired)
    {
        _tprintf(TEXT("(Reboot Required)\n"));
        return 1; // Install Success, reboot required.
    }
    
    return 0; // Install Success, no reboot required.
}

