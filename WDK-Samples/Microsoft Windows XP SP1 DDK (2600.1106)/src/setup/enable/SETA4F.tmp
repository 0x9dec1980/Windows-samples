/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    config.c

Abstract:

    Support routines to call Config Manager / SetupDi API functions.

Environment:

    user mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

  10/17/99: Created Keith S. Garner

--*/

#include "common.h"

/*++
Routine Description:

    Should we display the Device Nodes for the given Class?
  
Arguments:
    
    ClassGuid - GUID of the Class Type we are intrested in.

Return Value:
    
    If TRUE, then we should not display the Class
      
--*/
BOOL IsClassHidden( GUID * ClassGuid )
{
    BOOL bHidden = FALSE;
    HKEY hKeyClass;

    //
    // If the devices class has the NoDisplayClass value then
    // don't display this device.
    //
    if (hKeyClass = SetupDiOpenClassRegKey(ClassGuid,KEY_READ))
    {
        bHidden = (RegQueryValueEx(hKeyClass, 
            REGSTR_VAL_NODISPLAYCLASS, 
            NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
        RegCloseKey(hKeyClass);
    }                                 

    return bHidden;
}

/*++
Routine Description:

    Wrapper arround the SetupDiGetDeviceRegistryProperty() API call.

    This function will automatically allocate the memory nessicary to
    store the returned value. The calling function must free the buffer
    with a call to LocalFree().
  
Arguments:
    
    DevInst - Handle to Config Manager Device Instance.
    Property - ID of property.
    Buffer - Pointer to pchar to receive .
    Length - size of buffer allocated.

Return Value:
    
    returns the status of the operation True/False
      
--*/
BOOL GetRegistryProperty(HDEVINFO  DeviceInfoSet,
                         PSP_DEVINFO_DATA  DeviceInfoData,
                         ULONG Property,
                         PVOID Buffer,
                         PULONG Length)
{
    while (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
        DeviceInfoData,
        Property,
        NULL,
        (PVOID)*(TCHAR **)Buffer,
        *Length,
        Length
        ))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            //
            // We need to change the buffer size.
            //
            if (*(LPTSTR *)Buffer) 
                LocalFree(*(LPTSTR *)Buffer);
            *(LPTSTR *)Buffer = LocalAlloc(LPTR,*Length);
        }
        else
        {
            //
            // Unknown Failure.
            //
            if (GetLastError() != ERROR_INVALID_DATA)
               DisplayError(TEXT("GetDeviceRegistryProperty"));
            return FALSE;
        }            
    }

    return (*(LPTSTR *)Buffer)[0];
}

/*++
Routine Description:

    Given a DeviceNode, will try to construct a Human Readable Name
  
Arguments:
    
    DeviceInfoSet - Handle to a set of Device Information Nodes. 
    DeviceInfoData - Pointer to a struct of a DevNode.
    Buffer - Pointer to pchar to receive .
    Length - size of buffer allocated.

Return Value:
    
    Returns the status of the function. True/False
      
--*/
#define UnknownDevice TEXT("<Unknown Device>")
BOOL ConstructDeviceName( HDEVINFO DeviceInfoSet,
                          PSP_DEVINFO_DATA DeviceInfoData,
                          PVOID Buffer,
                          PULONG Length)
{
    if (!GetRegistryProperty(DeviceInfoSet,
        DeviceInfoData,
        SPDRP_FRIENDLYNAME ,
        Buffer,
        Length))
    {
        if (!GetRegistryProperty(DeviceInfoSet,
            DeviceInfoData,
            SPDRP_DEVICEDESC ,
            Buffer,
            Length))
        {
            if (!GetRegistryProperty(DeviceInfoSet,
                DeviceInfoData,
                SPDRP_CLASS ,
                Buffer,
                Length))
            {
                if (!GetRegistryProperty(DeviceInfoSet,
                    DeviceInfoData,
                    SPDRP_CLASSGUID ,
                    Buffer,
                    Length))
                {
                     *Length = (_tcslen(UnknownDevice)+1)*sizeof(TCHAR);
                     *(LPTSTR *)Buffer = LocalAlloc(LPTR,*Length);
                     _tcscpy(*(LPTSTR *)Buffer,UnknownDevice);
                }
            }
        }
    }
    return TRUE;
}

/*++
Routine Description:

    Can this DevNode be disabled?

Arguments:
    
    SelectedItem - Index of the Device Handle in the Device Information Set
    DevInst - Handle to Config Manager Device Instance.

Return Value:
    
    Returns TRUE if this DevNode can be disabled.
      
--*/
BOOL IsDisableable ( DWORD SelectedItem, HDEVINFO hDevInfo )
{
    SP_DEVINFO_DATA DeviceInfoData = {sizeof(SP_DEVINFO_DATA)};
    DWORD Status, Problem;

    //
    // Get a handle to the Selected Item.
    //
    if (!SetupDiEnumDeviceInfo(hDevInfo,SelectedItem,&DeviceInfoData))
    {
        DisplayError(TEXT("EnumDeviceInfo"));
        return FALSE;
    }

    if (CR_SUCCESS != CM_Get_DevNode_Status(&Status, &Problem,
                DeviceInfoData.DevInst,0))
    {
        DisplayError(TEXT("Get_DevNode_Status"));
        return FALSE;
    }

    return ((Status & DN_DISABLEABLE) && 
        (CM_PROB_HARDWARE_DISABLED != Problem));
}

/*++
Routine Description:

    Is this DevNode Disabled?

Arguments:
    
    SelectedItem - Index of the Device Handle in the Device Information Set
    DevInst - Handle to Config Manager Device Instance.

Return Value:
    
    Returns TRUE if the DevNode is disabled.
      
--*/
BOOL IsDisabled ( DWORD SelectedItem, HDEVINFO hDevInfo )
{
    SP_DEVINFO_DATA DeviceInfoData = {sizeof(SP_DEVINFO_DATA)};
    DWORD Status, Problem;

    //
    // Get a handle to the Selected Item.
    //
    if (!SetupDiEnumDeviceInfo(hDevInfo,SelectedItem,&DeviceInfoData))
    {
        DisplayError(TEXT("EnumDeviceInfo"));
        return FALSE;
    }

    if (CR_SUCCESS != CM_Get_DevNode_Status(&Status, &Problem,
                DeviceInfoData.DevInst,0))
    {
        DisplayError(TEXT("Get_DevNode_Status"));
        return FALSE;
    }

    return ((Status & DN_HAS_PROBLEM) && (CM_PROB_DISABLED == Problem)) ;
}

