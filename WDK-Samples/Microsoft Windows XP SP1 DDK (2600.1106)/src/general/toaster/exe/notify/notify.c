/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name: notify.c


Abstract:


Author:

     Eliyas Yakub   Nov 23, 1999

Environment:

    User mode only.

Revision History:

    Modified to use linked list for deviceInfo 
    instead of arrays. (5/12/2000)
    
--*/
#define UNICODE 1
#define INITGUID

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <dbt.h>
#include <winioctl.h>
#include "..\inc\public.h"
#include "notify.h"


BOOL 
HandlePowerBroadcast(
    HWND hWnd, 
    WPARAM wParam, 
    LPARAM lParam);
    
//
// Global variables
//
HINSTANCE   hInst;
HWND        hWndList;
TCHAR       szTitle[]=TEXT("Toaster Package Test Application");
LIST_ENTRY  ListHead;
HDEVNOTIFY  hInterfaceNotification;
TCHAR       OutText[500];
UINT        ListBoxIndex = 0;
GUID        InterfaceGuid;// = GUID_DEVINTERFACE_TOASTER;


_inline BOOLEAN IsValid(ULONG No) 
{
    PLIST_ENTRY thisEntry;
    PDEVICE_INFO deviceInfo;

    if(0==(No)) return TRUE; //special case

    for(thisEntry = ListHead.Flink; thisEntry != &ListHead;
                        thisEntry = thisEntry->Flink)
    {
           deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
        if((No) == deviceInfo->SerialNo) {
            return TRUE;
        }
    }
    return FALSE;
}

_inline VOID Display(PWCHAR Format, PWCHAR Str) 
{
    if(Str) {
        wsprintf(OutText, Format, Str);
    } else {
        wsprintf(OutText, Format);
    }
    SendMessage(hWndList, LB_INSERTSTRING, ListBoxIndex++, (LPARAM)OutText);
}


int PASCAL WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpszCmdParam,
                    int       nCmdShow)
{
    static    TCHAR szAppName[]=TEXT("Notify");
    HWND      hWnd;
    MSG       msg;
    WNDCLASS  wndclass;

    InterfaceGuid = GUID_DEVINTERFACE_TOASTER;
    hInst=hInstance;

    if (!hPrevInstance)
       {
         wndclass.style        =  CS_HREDRAW | CS_VREDRAW;
         wndclass.lpfnWndProc  =  WndProc;
         wndclass.cbClsExtra   =  0;
         wndclass.cbWndExtra   =  0;
         wndclass.hInstance    =  hInstance;
         wndclass.hIcon        =  LoadIcon (NULL, IDI_APPLICATION);
         wndclass.hCursor      =  LoadCursor(NULL, IDC_ARROW);
         wndclass.hbrBackground=  GetStockObject(WHITE_BRUSH);
         wndclass.lpszMenuName =  TEXT("GenericMenu");
         wndclass.lpszClassName=  szAppName;

         RegisterClass(&wndclass);
       }

    hWnd = CreateWindow (szAppName,
                         szTitle,
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         NULL,
                         NULL,
                         hInstance,
                         NULL);

    ShowWindow (hWnd,nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage (&msg, NULL, 0,0))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

    return (0);
}


LRESULT FAR PASCAL 
WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD nEventType = (DWORD)wParam; 
    PDEV_BROADCAST_HDR p = (PDEV_BROADCAST_HDR) lParam;
    DEV_BROADCAST_DEVICEINTERFACE filter;
    
    switch (message)
    {

        case WM_COMMAND:
            HandleCommands(hWnd, message, wParam, lParam);
            return 0;

        case WM_CREATE:

            //
            // Load and set the icon of the program
            //
            SetClassLongPtr(hWnd, GCLP_HICON, 
                (LONG_PTR)LoadIcon((HINSTANCE)lParam,MAKEINTRESOURCE(IDI_CLASS_ICON)));

            hWndList = CreateWindow (TEXT("listbox"),
                         NULL,
                         WS_CHILD|WS_VISIBLE|LBS_NOTIFY |
                         WS_VSCROLL | WS_BORDER,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         hWnd,
                         (HMENU)ID_EDIT,
                         hInst,
                         NULL);
                         
            filter.dbcc_size = sizeof(filter);
            filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
            filter.dbcc_classguid = InterfaceGuid;
            hInterfaceNotification = RegisterDeviceNotification(hWnd, &filter, 0);

            InitializeListHead(&ListHead);
            EnumExistingDevices(hWnd);

            return 0;

      case WM_SIZE:

            MoveWindow(hWndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
            return 0;

      case WM_SETFOCUS:
            SetFocus(hWndList);
            return 0;
            
      case WM_DEVICECHANGE:      

#ifdef DEVNODE
            //
            // The DBT_DEVNODES_CHANGED broadcast message is sent 
            // everytime a device is added or removed. This message
            // is typically handled by Device Manager kind of apps, 
            // which uses it to refresh window whenever something changes.
            // The lParam is always NULL in this case.
            //
            if(DBT_DEVNODES_CHANGED == wParam) {
                Display(TEXT("DBT_DEVNODES_CHANGED"), NULL);
            }
#endif             
            //
            // All the events we're interested in come with lParam pointing to
            // a structure headed by a DEV_BROADCAST_HDR.  This is denoted by
            // bit 15 of wParam being set, and bit 14 being clear.
            //
            if((wParam & 0xC000) == 0x8000) {
            
                if (!p)
                    return 0;
                                  
                if (p->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {

                    HandleDeviceInterfaceChange(hWnd, nEventType, (PDEV_BROADCAST_DEVICEINTERFACE) p);
                } else if (p->dbch_devicetype == DBT_DEVTYP_HANDLE) {

                    HandleDeviceChange(hWnd, nEventType, (PDEV_BROADCAST_HANDLE) p);
                }
            }
            return 0;
#ifdef POWER            
            //
            // If you want to see all the POWER broadcart then
            // turn on the following code. This app doesn't do
            // anything special during suspend/resume. Read 
            // platform SDK docs for further info.
            //
      case WM_POWERBROADCAST:
            HandlePowerBroadcast(hWnd, wParam, lParam);
            return 0;            
#endif
      case WM_CLOSE:
            Cleanup(hWnd);
            UnregisterDeviceNotification(hInterfaceNotification);
            return  DefWindowProc(hWnd,message, wParam, lParam);

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd,message, wParam, lParam);
  }


LRESULT
HandleCommands(
    HWND     hWnd,
    UINT     uMsg,
    WPARAM   wParam,
    LPARAM     lParam
    )

{
     PDIALOG_RESULT result;

    switch (wParam) {

        case IDM_OPEN:
            Cleanup(hWnd); // close all open handles
            EnumExistingDevices(hWnd);
            break;

        case IDM_CLOSE:           
            Cleanup(hWnd);
            break;
            
        case IDM_HIDE:       
            result = (PDIALOG_RESULT)DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd,
                                                            (DLGPROC)DlgProc);           
            if(result && result->SerialNo && IsValid(result->SerialNo)) { 
                UINT bytes;
                PDEVICE_INFO deviceInfo = NULL;
                PLIST_ENTRY thisEntry;

                //
                // Find out the deviceInfo that matches this SerialNo.
                // We need the deviceInfo to get the handle to the device.
                //
                for(thisEntry = ListHead.Flink; thisEntry != &ListHead;
                    thisEntry = thisEntry->Flink)
                {
                       deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
                    if(result->SerialNo == deviceInfo->SerialNo) {
                        break;
                    }
                    deviceInfo = NULL;
                }

                //
                // If found send I/O control
                //
                
                if (deviceInfo && !DeviceIoControl (deviceInfo->hDevice,
                          IOCTL_TOASTER_DONT_DISPLAY_IN_UI_DEVICE,
                          NULL, 0,
                          NULL, 0,
                          &bytes, NULL)) {
                       MessageBox(hWnd, TEXT("Request Failed or Invalid Serial No"),
                                   TEXT("Error"), MB_OK);                                      
                   }
                free(result);
            }
            break;
        case IDM_PLUGIN:  

            result = (PDIALOG_RESULT)DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG), hWnd,
                                                            (DLGPROC)DlgProc);
            if(result) {     
                if(!result->SerialNo || !OpenBusInterface(result->SerialNo, result->DeviceId, PLUGIN))
                    MessageBox(hWnd, TEXT("Request Failed"), TEXT("Error"), MB_OK);                          
                free(result);
            }
            break;
        case IDM_UNPLUG:           
            result = (PDIALOG_RESULT)DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd,
                                                            (DLGPROC)DlgProc);
            
            if(result && IsValid(result->SerialNo)) {                                                            
                if(!OpenBusInterface(result->SerialNo, NULL, UNPLUG))
                       MessageBox(hWnd, TEXT("Request Failed"), TEXT("Error"), MB_OK);                                      
                free(result);
            }
            break;
        case IDM_EJECT:           
            result = (PDIALOG_RESULT)DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd,
                                                            (DLGPROC)DlgProc);
            if(result && IsValid(result->SerialNo)) {   
                if(!OpenBusInterface(result->SerialNo, NULL, EJECT))
                       MessageBox(hWnd, TEXT("Request Failed"), TEXT("Error"), MB_OK);                                      
                free(result);
            }
            break;
        case IDM_EXIT:           
            PostQuitMessage(0);
            break;
            
        default:
            break;
    }
    return TRUE;
}

INT_PTR CALLBACK 
DlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    BOOL            success;
    PDIALOG_RESULT  dialogResult;
    
    
    switch(message)
    {
        case WM_INITDIALOG:
               SetDlgItemText(hDlg, IDC_DEVICEID, BUS_HARDWARE_IDS);        
            return TRUE;

        case WM_COMMAND:    
            switch( wParam)
            {
            case ID_OK:
                dialogResult = malloc (sizeof(DIALOG_RESULT) + MAX_DEVICENAME);
                if(dialogResult) {
                    dialogResult->DeviceId = (PWCHAR)((PCHAR)dialogResult + sizeof(DIALOG_RESULT));
                    dialogResult->SerialNo = GetDlgItemInt(hDlg,IDC_SERIALNO, &success, FALSE );
                    GetDlgItemText(hDlg, IDC_DEVICEID, dialogResult->DeviceId, MAX_DEVICENAME-1 );                    
                }
                EndDialog(hDlg, (UINT_PTR)dialogResult);
                return TRUE;
            case ID_CANCEL:
                EndDialog(hDlg, 0);
                return TRUE;
                
            }
            break;
                      
    }
    return FALSE;
}

 
BOOL 
HandleDeviceInterfaceChange(
    HWND hWnd, 
    DWORD evtype, 
    PDEV_BROADCAST_DEVICEINTERFACE dip
    )
{
    DEV_BROADCAST_HANDLE    filter;
    PDEVICE_INFO            deviceInfo = NULL;
    
    switch (evtype)
    {
        case DBT_DEVICEARRIVAL:
        //
        // New device arrived. Open handle to the device 
        // and register notification of type DBT_DEVTYP_HANDLE
        //

        deviceInfo = malloc(sizeof(DEVICE_INFO));
        if(!deviceInfo)
            return FALSE;
            
        memset(deviceInfo, 0, sizeof(DEVICE_INFO));
        InitializeListHead(&deviceInfo->ListEntry);
        InsertTailList(&ListHead, &deviceInfo->ListEntry);
        

        if(!GetDeviceDescription(dip->dbcc_name, deviceInfo->DeviceName,
                                 &deviceInfo->SerialNo)) {
            MessageBox(hWnd, TEXT("GetDeviceDescription failed"), TEXT("Error!"), MB_OK);  
        }

        Display(TEXT("New device Arrived (Interface Change Notification): %ws"), 
                    deviceInfo->DeviceName);

        wcscpy(deviceInfo->DevicePath, dip->dbcc_name);
        
        deviceInfo->hDevice = CreateFile(dip->dbcc_name, 
                                        GENERIC_READ, 0, NULL, 
                                        OPEN_EXISTING, 0, NULL);
        if(deviceInfo->hDevice == INVALID_HANDLE_VALUE) {
            free(deviceInfo);
        }
        memset (&filter, 0, sizeof(filter)); //zero the structure
        filter.dbch_size = sizeof(filter);
        filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
        filter.dbch_handle = deviceInfo->hDevice;
 
        deviceInfo->hHandleNotification = 
                            RegisterDeviceNotification(hWnd, &filter, 0);
        break;
 
        case DBT_DEVICEREMOVECOMPLETE:
        Display(TEXT("Remove Complete (Interface Change Notification)"), NULL);
        break;    

        //
        // Device Removed.
        // 

        default:
        Display(TEXT("Unknown (Interface Change Notification)"), NULL);
        break;
    }
    return TRUE;
}
 
BOOL 
HandleDeviceChange(
    HWND hWnd, 
    DWORD evtype, 
    PDEV_BROADCAST_HANDLE dhp
    )
{
    UINT i;
    DEV_BROADCAST_HANDLE    filter;
    PDEVICE_INFO            deviceInfo = NULL;
    PLIST_ENTRY             thisEntry;

    //
    // Walk the list to get the deviceInfo for this device
    // by matching the handle given in the notification.
    //
    for(thisEntry = ListHead.Flink; thisEntry != &ListHead;
                        thisEntry = thisEntry->Flink)
    {
           deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
        if(dhp->dbch_handle == deviceInfo->hDevice) {
            break;
        }
        deviceInfo = NULL;
    }

    if(!deviceInfo) {
        MessageBox(hWnd, TEXT("Unknown Device"), TEXT("Error"), MB_OK);  
        return FALSE;
    }

    switch (evtype)
    {
    
    case DBT_DEVICEQUERYREMOVE:

        Display(TEXT("Query Remove (Handle Notification)"),
                        deviceInfo->DeviceName);
        //
        // Close the handle so that target device can
        // get removed. Do not unregister the notification
        // at this point, because you want to know whether
        // the device is successfully removed or not.
        //
 
        if (deviceInfo->hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(deviceInfo->hDevice);
            Display(TEXT("Closed handle to device %ws"), 
                            deviceInfo->DeviceName );
            // since we use the handle to locate the deviceinfo, we
            // will defer the following to the remove_pending.
            //deviceInfo->hDevice = INVALID_HANDLE_VALUE;
            
         }
        break;
        
    case DBT_DEVICEREMOVECOMPLETE:
 
        Display(TEXT("Remove Complete (Handle Notification):%ws"),
                    deviceInfo->DeviceName);
        //
        // Device is removed so close the handle if it's there
        // and unregister the notification
        //
 
        if (deviceInfo->hHandleNotification) {
            UnregisterDeviceNotification(deviceInfo->hHandleNotification);
            deviceInfo->hHandleNotification = NULL;
         }
        if (deviceInfo->hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(deviceInfo->hDevice);
            Display(TEXT("Closed handle to device %ws"), 
                                deviceInfo->DeviceName );
            deviceInfo->hDevice = INVALID_HANDLE_VALUE;
         }
        //
        // Unlink this deviceInfo from the list and free the memory
        //
         RemoveEntryList(&deviceInfo->ListEntry);
         free(deviceInfo);
         
        break;
        
    case DBT_DEVICEREMOVEPENDING:
 
        Display(TEXT("Remove Pending (Handle Notification):%ws"),
                                        deviceInfo->DeviceName);
        //
        // Device is removed so close the handle if it's there
        // and unregister the notification
        //
        if (deviceInfo->hHandleNotification) {
            UnregisterDeviceNotification(deviceInfo->hHandleNotification);
            deviceInfo->hHandleNotification = NULL;
            deviceInfo->hDevice = INVALID_HANDLE_VALUE;
         }
        //
        // Unlink this deviceInfo from the list and free the memory
        //
         RemoveEntryList(&deviceInfo->ListEntry);
         free(deviceInfo);

        break;

    case DBT_DEVICEQUERYREMOVEFAILED :
        Display(TEXT("Remove failed (Handle Notification):%ws"),
                                    deviceInfo->DeviceName);
        //
        // Remove failed. So reopen the device and register for
        // notification on the new handle. But first we should unregister
        // the previous notification.
        //
        if (deviceInfo->hHandleNotification) {
            UnregisterDeviceNotification(deviceInfo->hHandleNotification);
            deviceInfo->hHandleNotification = NULL;
         }
        deviceInfo->hDevice = CreateFile(deviceInfo->DevicePath, GENERIC_READ, 
                                0, NULL, OPEN_EXISTING, 0, NULL);
        if(deviceInfo->hDevice == INVALID_HANDLE_VALUE) {
            free(deviceInfo);
            break;
        }

        //
        // Register handle based notification to receive pnp 
        // device change notification on the handle.
        //
        memset (&filter, 0, sizeof(filter)); //zero the structure
        filter.dbch_size = sizeof(filter);
        filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
        filter.dbch_handle = deviceInfo->hDevice;
 
        deviceInfo->hHandleNotification = 
                            RegisterDeviceNotification(hWnd, &filter, 0);
        Display(TEXT("Reopened device %ws"), deviceInfo->DeviceName);        
        break;
        
    default:
        Display(TEXT("Unknown (Handle Notification)"),
                                    deviceInfo->DeviceName);
        break;
 
    }
    return TRUE;
}


BOOLEAN
EnumExistingDevices(
    HWND   hWnd
)
{
    HDEVINFO                            hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
    ULONG                               predictedLength = 0;
    ULONG                               requiredLength = 0, bytes=0;
    DWORD                               dwRegType, error;
    DEV_BROADCAST_HANDLE                filter;
    PDEVICE_INFO                        deviceInfo =NULL;
    UINT                                i=0;

    hardwareDeviceInfo = SetupDiGetClassDevs (
                       (LPGUID)&InterfaceGuid,
                       NULL, // Define no enumerator (global)
                       NULL, // Define no
                       (DIGCF_PRESENT | // Only Devices present
                       DIGCF_DEVICEINTERFACE)); // Function class devices.
    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        goto Error;
    }
  
    //
    // Enumerate devices of toaster class
    //
    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

    for(i=0; SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                 0, // No care about specific PDOs
                                 (LPGUID)&InterfaceGuid,
                                 i, //
                                 &deviceInterfaceData); i++ ) {
                                 
        //
        // Allocate a function class device data structure to 
        // receive the information about this particular device.
        //

        //
        // First find out required length of the buffer
        //
        if(deviceInterfaceDetailData)
                free (deviceInterfaceDetailData);
                
        if(!SetupDiGetDeviceInterfaceDetail (
                hardwareDeviceInfo,
                &deviceInterfaceData,
                NULL, // probing so no output buffer yet
                0, // probing so output buffer length of zero
                &requiredLength,
                NULL) && (error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
        {
            goto Error;
        }
        predictedLength = requiredLength;

        deviceInterfaceDetailData = malloc (predictedLength);
        deviceInterfaceDetailData->cbSize = 
                        sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);


        if (! SetupDiGetDeviceInterfaceDetail (
                   hardwareDeviceInfo,
                   &deviceInterfaceData,
                   deviceInterfaceDetailData,
                   predictedLength,
                   &requiredLength,
                   NULL)) {              
            goto Error;
        }

        deviceInfo = malloc(sizeof(DEVICE_INFO));
        if(!deviceInfo)
            goto Error;
            
        memset(deviceInfo, 0, sizeof(DEVICE_INFO));
        InitializeListHead(&deviceInfo->ListEntry);
        InsertTailList(&ListHead, &deviceInfo->ListEntry);
        
        //
        // Get the device details such as friendly name and SerialNo
        //
        if(!GetDeviceDescription(deviceInterfaceDetailData->DevicePath, 
                                 deviceInfo->DeviceName,
                                 &deviceInfo->SerialNo)){
            goto Error;
        }

        Display(TEXT("Found device %ws"), deviceInfo->DeviceName );

        wcscpy(deviceInfo->DevicePath, deviceInterfaceDetailData->DevicePath);
        //
        // Open an handle to the device.
        //
        deviceInfo->hDevice = CreateFile ( 
                deviceInterfaceDetailData->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL, // no SECURITY_ATTRIBUTES structure
                OPEN_EXISTING, // No special create flags
                0, // No special attributes
                NULL);

        if (INVALID_HANDLE_VALUE == deviceInfo->hDevice) {
            goto Error;
        }
        
        //
        // Register handle based notification to receive pnp 
        // device change notification on the handle.
        //

        memset (&filter, 0, sizeof(filter)); //zero the structure
        filter.dbch_size = sizeof(filter);
        filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
        filter.dbch_handle = deviceInfo->hDevice;

        deviceInfo->hHandleNotification = RegisterDeviceNotification(hWnd, &filter, 0);        

    } 

    if(deviceInterfaceDetailData)
        free (deviceInterfaceDetailData);

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    return 0;

Error:

    error = GetLastError();
    MessageBox(hWnd, TEXT("EnumExisting Devices failed"), TEXT("Error!"), MB_OK);  
    if(deviceInterfaceDetailData)
        free (deviceInterfaceDetailData);

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    Cleanup(hWnd);
    return 0;
}

BOOLEAN Cleanup(HWND hWnd)
{
    PDEVICE_INFO    deviceInfo =NULL;
    PLIST_ENTRY     thisEntry;

    while (!IsListEmpty(&ListHead)) {
        thisEntry = RemoveHeadList(&ListHead);
        deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
        if (deviceInfo->hHandleNotification) {
            UnregisterDeviceNotification(deviceInfo->hHandleNotification);
            deviceInfo->hHandleNotification = NULL;
        }
        if (deviceInfo->hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(deviceInfo->hDevice);
            deviceInfo->hDevice = INVALID_HANDLE_VALUE;
            Display(TEXT("Closed handle to device %ws"), deviceInfo->DeviceName );
        }
        free(deviceInfo);
    }
    return TRUE;
}


BOOL 
GetDeviceDescription(
    LPTSTR DevPath, 
    LPTSTR OutBuffer,
    PULONG SerialNo
)
{
    HDEVINFO                            hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
    SP_DEVINFO_DATA                     deviceInfoData;
    DWORD                               dwRegType, error;

    hardwareDeviceInfo = SetupDiCreateDeviceInfoList(NULL, NULL);
    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        goto Error;
    }
    
    //
    // Enumerate devices of toaster class
    //
    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

    SetupDiOpenDeviceInterface (hardwareDeviceInfo, DevPath,
                                 0, //
                                 &deviceInterfaceData);
                                 
    deviceInfoData.cbSize = sizeof(deviceInfoData);
    if(!SetupDiGetDeviceInterfaceDetail (
            hardwareDeviceInfo,
            &deviceInterfaceData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            NULL,
            &deviceInfoData) && (error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
    {
        goto Error;
    }
    //
    // Get the friendly name for this instance, if that fails
    // try to get the device description.
    //

    if(!SetupDiGetDeviceRegistryProperty(hardwareDeviceInfo, &deviceInfoData,
                                     SPDRP_FRIENDLYNAME,
                                     &dwRegType,
                                     (BYTE*) OutBuffer,
                                     MAX_DEVICENAME,
                                     NULL))
    {
        if(!SetupDiGetDeviceRegistryProperty(hardwareDeviceInfo, &deviceInfoData,
                                     SPDRP_DEVICEDESC,
                                     &dwRegType,
                                     (BYTE*) OutBuffer,
                                     MAX_DEVICENAME,
                                     NULL)){
            goto Error;
                                     
        }
        

    }

    //
    // Get the serial number of the device. The bus driver reports
    // the device serial number as UINumber in the devcaps.
    //
    if(!SetupDiGetDeviceRegistryProperty(hardwareDeviceInfo, 
                 &deviceInfoData,
                 SPDRP_UI_NUMBER,
                 &dwRegType,
                 (BYTE*) SerialNo,
                 sizeof(PULONG),
                 NULL)) {
        goto Error;
    }


    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    return TRUE;

Error:

    error = GetLastError();
    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    return FALSE;
}



BOOLEAN
OpenBusInterface (
    IN ULONG SerialNo,
    IN PWCHAR DeviceId,
    IN USER_ACTION_TYPE Action                
    )
{
    HANDLE                              hDevice=INVALID_HANDLE_VALUE;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
    ULONG                               predictedLength = 0;
    ULONG                               requiredLength = 0;
    ULONG                               bytes;
    BUSENUM_UNPLUG_HARDWARE             unplug;
    BUSENUM_EJECT_HARDWARE              eject;
    PBUSENUM_PLUGIN_HARDWARE            hardware;
    HDEVINFO                            hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA               deviceInterfaceData;
    BOOLEAN                                status = FALSE;
    //
    // Open a handle to the device interface information set of all 
    // present toaster bus enumerator interfaces.
    //

    hardwareDeviceInfo = SetupDiGetClassDevs (
                       (LPGUID)&GUID_DEVINTERFACE_BUSENUM_TOASTER,
                       NULL, // Define no enumerator (global)
                       NULL, // Define no
                       (DIGCF_PRESENT | // Only Devices present
                       DIGCF_DEVICEINTERFACE)); // Function class devices.

    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        return FALSE;
    }

    deviceInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

    if (!SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                 0, // No care about specific PDOs
                                 (LPGUID)&GUID_DEVINTERFACE_BUSENUM_TOASTER,
                                 0, //
                                 &deviceInterfaceData)) {

        if (ERROR_NO_MORE_ITEMS == GetLastError()) {   
        }
    }

    //
    // Allocate a function class device data structure to receive the
    // information about this particular device.
    //
    
    SetupDiGetDeviceInterfaceDetail (
            hardwareDeviceInfo,
            &deviceInterfaceData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            &requiredLength,
            NULL); // not interested in the specific dev-node


    predictedLength = requiredLength;

    deviceInterfaceDetailData = malloc (predictedLength);

    if(deviceInterfaceDetailData) {
        deviceInterfaceDetailData->cbSize = 
                        sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
    } else {
        goto Clean0;
    }

    
    if (! SetupDiGetDeviceInterfaceDetail (
               hardwareDeviceInfo,
               &deviceInterfaceData,
               deviceInterfaceDetailData,
               predictedLength,
               &requiredLength,
               NULL)) {
        goto Clean1;
    }


    hDevice = CreateFile ( deviceInterfaceDetailData->DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        0, // FILE_SHARE_READ | FILE_SHARE_WRITE
                        NULL, // no SECURITY_ATTRIBUTES structure
                        OPEN_EXISTING, // No special create flags
                        0, // No special attributes
                        NULL); // No template file

    if (INVALID_HANDLE_VALUE == hDevice) {
        goto Clean1;
    }
    
    //
    // Enumerate Devices
    //
    
    if(Action == PLUGIN) {


        hardware = malloc (bytes = (sizeof (BUSENUM_PLUGIN_HARDWARE) +
                                      (wcslen(DeviceId)+2)*sizeof(WCHAR)));

        if(hardware) {
            memset(hardware, 0, bytes);
            hardware->Size = sizeof (BUSENUM_PLUGIN_HARDWARE);
            hardware->SerialNo = SerialNo;
        } else {
            goto Clean2;
        }

        //
        // copy the Device ID
        //
        wcscpy(hardware->HardwareIDs, DeviceId);
        
        if (!DeviceIoControl (hDevice,
                              IOCTL_BUSENUM_PLUGIN_HARDWARE ,
                              hardware, bytes,
                              hardware, bytes,
                              &bytes, NULL)) {
              free (hardware);
              goto Clean2;
        }

        free (hardware);
    }

    //
    // Removes a device if given the specific Id of the device. Otherwise this
    // ioctls removes all the devices that are enumerated so far.
    //
    
    if(Action == UNPLUG) {

        unplug.Size = bytes = sizeof (unplug);
        unplug.SerialNo = SerialNo;
        if (!DeviceIoControl (hDevice,
                              IOCTL_BUSENUM_UNPLUG_HARDWARE,
                              &unplug, bytes,
                              &unplug, bytes,
                              &bytes, NULL)) {
            goto Clean2;
        }
    }

    //
    // Ejects a device if given the specific Id of the device. Otherwise this
    // ioctls ejects all the devices that are enumerated so far.
    //

    if(Action == EJECT)
    {

        eject.Size = bytes = sizeof (eject);
        eject.SerialNo = SerialNo;
        if (!DeviceIoControl (hDevice,
                              IOCTL_BUSENUM_EJECT_HARDWARE,
                              &eject, bytes,
                              &eject, bytes,
                              &bytes, NULL)) {
            goto Clean2;
        }
    }

    status = TRUE;
    
Clean2:
    CloseHandle(hDevice);
Clean1:
    free (deviceInterfaceDetailData);
Clean0:
    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    return status;
}

BOOL 
HandlePowerBroadcast(
    HWND hWnd, 
    WPARAM wParam, 
    LPARAM lParam)
{
    BOOL fRet = TRUE;
    
    switch (wParam)
    {
        case PBT_APMQUERYSTANDBY:
            Display(TEXT("PBT_APMQUERYSTANDBY"), NULL);
            break;
        case PBT_APMQUERYSUSPEND:
            Display(TEXT("PBT_APMQUERYSUSPEND"), NULL);
            break;
        case PBT_APMSTANDBY :
            Display(TEXT("PBT_APMSTANDBY"), NULL);
            break;
        case PBT_APMSUSPEND :
            Display(TEXT("PBT_APMSUSPEND"), NULL);
            break;
        case PBT_APMQUERYSTANDBYFAILED:
            Display(TEXT("PBT_APMQUERYSTANDBYFAILED"), NULL);
            break;
        case PBT_APMRESUMESTANDBY:
            Display(TEXT("PBT_APMRESUMESTANDBY"), NULL);
            break;
        case PBT_APMQUERYSUSPENDFAILED:
            Display(TEXT("PBT_APMQUERYSUSPENDFAILED"), NULL);
            break;
        case PBT_APMRESUMESUSPEND:
            Display(TEXT("PBT_APMRESUMESUSPEND"), NULL);
            break;
        case PBT_APMBATTERYLOW:
            Display(TEXT("PBT_APMBATTERYLOW"), NULL);
            break;
        case PBT_APMOEMEVENT:
            Display(TEXT("PBT_APMOEMEVENT"), NULL);
            break;
        case PBT_APMRESUMEAUTOMATIC:
            Display(TEXT("PBT_APMRESUMEAUTOMATIC"), NULL);
            break;
        case PBT_APMRESUMECRITICAL:
            Display(TEXT("PBT_APMRESUMECRITICAL"), NULL);
            break;
        case PBT_APMPOWERSTATUSCHANGE:
            Display(TEXT("PBT_APMPOWERSTATUSCHANGE"), NULL);
            break;
        default: 
            Display(TEXT("Default"), NULL);
            break;
    }
    return fRet;
}
