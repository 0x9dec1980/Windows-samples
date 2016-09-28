/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    main.c

Abstract:

    Main function and callback for dialog box.

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

//
//  Global Data
//
static SP_CLASSIMAGELIST_DATA ClassImageListData;
static BOOL ShowHidden = 0;
static DWORD SelectedItem = -1;
static HDEVINFO hDevInfo = 0;

/*++
Routine Description:

    Given a Class GUID, this function will return the index of the 
    image within the imagelist.
  
Arguments:
    
    ClassGUID - Pointer to the Class GUID.

    Index - Pointer to an int that will receive the image index.

Return Value:
    
    Returns the status of the function.
      
--*/
BOOL GetClassImageIndex(LPGUID ClassGuid, 
                        PINT Index)
{
    return SetupDiGetClassImageIndex(&ClassImageListData,ClassGuid,Index);
}

/*++
Routine Description:

    Primary callback routine for the dialog box.
  
Arguments:
    
    hwndDlg - Window handle to the dialog box.
    uMsg - Message value.
    wParam - first message parameter
    lParam - Second message parameter.

Return Value:
    
    returns True if this function processed the message.
      
--*/
BOOL CALLBACK EnableDisableDialogProc(HWND hwndDlg, 
                               UINT uMsg,    
                               WPARAM wParam,
                               LPARAM lParam )
{
    HWND TreeHWnd = GetDlgItem(hwndDlg, IDC_TREE1);

    switch (uMsg) 
    {
    case WM_INITDIALOG: // Initialize Dialog...

        //
        // Set the Dialog Icon to IDI_ICON1, passed in through lParam
        //
        SetClassLongPtr(hwndDlg,GCLP_HICON, 
            (LONG_PTR)LoadIcon((HINSTANCE)lParam,MAKEINTRESOURCE(IDI_ICON1)));

        //
        // Get a handle to all devices in all classes present on system
        //
        if (INVALID_HANDLE_VALUE == (hDevInfo = 
                SetupDiGetClassDevs(NULL,NULL,hwndDlg,
                DIGCF_PRESENT|DIGCF_ALLCLASSES)))
        {
            DisplayError(TEXT("GetClassDevs"));
            return FALSE;
        }

        //
        // Get the Images for all classes, and bind to the TreeView
        //
        ClassImageListData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
        if (!SetupDiGetClassImageList( &ClassImageListData ))
        {
            DisplayError(TEXT("GetClassImageList"));
            return FALSE;
        }

        TreeView_SetImageList(TreeHWnd,
            ClassImageListData.ImageList,LVSIL_NORMAL);

        //
        // Add the devices to the TreeView window.
        //
        return EnumAddDevices(ShowHidden,TreeHWnd,hDevInfo);

    case WM_DESTROY: // Program Closing, cleanup...
        SetupDiDestroyDeviceInfoList(hDevInfo);
        SetupDiDestroyClassImageList( &ClassImageListData );
        return TRUE;
        
    case WM_CLOSE: // User has selected CLOSE from System Menu...
        return EndDialog(hwndDlg,TRUE);

    case WM_SIZE: // Resize the child windows...
        return MoveWindow(TreeHWnd,0,0,LOWORD(lParam),HIWORD(lParam),TRUE);

    case WM_NOTIFY: // User has selected something in the TreeView...
        switch(((NMHDR *)lParam)->code)
        {
        case TVN_SELCHANGED: // User has selected a new item...

            //
            // Clear the Menu Items.
            //
            EnableMenuItem(GetMenu(hwndDlg),ID_CHANGE_ENABLEDEVICE, 
                MF_BYCOMMAND|MF_GRAYED);
            EnableMenuItem(GetMenu(hwndDlg),ID_CHANGE_DISABLEDEVICE, 
                MF_BYCOMMAND|MF_GRAYED);

            //
            // Update the Menu to the current status
            // ( running/stoped ) of the selected device.
            //
            ((LPNMTREEVIEW)lParam)->itemNew.mask = TVIF_PARAM;
            if (!TreeView_GetItem(TreeHWnd,&((LPNMTREEVIEW)lParam)->itemNew))
                break;

            //
            // Get the Index for selected Device within the Device Info Set 
            //
            SelectedItem = (DWORD) ((LPNMTREEVIEW)lParam)->itemNew.lParam;

            if (IsDisabled(SelectedItem,hDevInfo))
                EnableMenuItem(GetMenu(hwndDlg),ID_CHANGE_ENABLEDEVICE, 
                    MF_BYCOMMAND|MF_ENABLED);
            else if (IsDisableable(SelectedItem,hDevInfo))
                EnableMenuItem(GetMenu(hwndDlg),ID_CHANGE_DISABLEDEVICE, 
                    MF_BYCOMMAND|MF_ENABLED);

            return TRUE;

        case NM_DBLCLK: // User has double-clicked on an item...

            //
            // TreeView doesn't like nesting. So Post message instead.
            //

            if (IsDisabled(SelectedItem,hDevInfo))
               return PostMessage(hwndDlg,WM_COMMAND,ID_CHANGE_ENABLEDEVICE,0);
            else if (IsDisableable(SelectedItem,hDevInfo))
               return PostMessage(hwndDlg,WM_COMMAND,ID_CHANGE_DISABLEDEVICE,0);

        default:
            break;
        }
        break;
    case WM_COMMAND: // User Has selected a Menu Command...
        switch (LOWORD(wParam))
        {
        case ID_CHANGE_DISABLEDEVICE:
            if (MessageBox(hwndDlg,
                TEXT("Disable this Device?"),
                TEXT("Change Device Status"),
                MB_OKCANCEL) == IDOK)
            {
                if (StateChange(DICS_DISABLE,SelectedItem,hDevInfo))
                    return EnumAddDevices(ShowHidden,TreeHWnd,hDevInfo);
            }
            break;

        case ID_CHANGE_ENABLEDEVICE:
            if (MessageBox(hwndDlg,
                TEXT("Enable this Device?"),
                TEXT("Change Device Status"),
                MB_OKCANCEL) == IDOK)
            {
                if (StateChange(DICS_ENABLE,SelectedItem,hDevInfo))
                    return EnumAddDevices(ShowHidden,TreeHWnd,hDevInfo);
            }
            break;

        case ID_OPTIONS_SHOWHIDDENDEVICES:
            ShowHidden = !ShowHidden;
            CheckMenuItem(GetMenu(hwndDlg),
                LOWORD(wParam),
                MF_BYCOMMAND | (ShowHidden?MF_CHECKED:MF_UNCHECKED));
            return EnumAddDevices(ShowHidden,TreeHWnd,hDevInfo);

        case ID_FILE_REFRESH:
            return EnumAddDevices(ShowHidden,TreeHWnd,hDevInfo);

        case ID_FILE_EXIT:
            return EndDialog(hwndDlg,TRUE);

        default:
            break;
        }
    default:
        break;
    }
    return FALSE;
}

/*++
Routine Description:

    Standard entry point for a Win32 Application ( Non-Console )
  
Arguments:
    
    Standard WinMain() Arguments

Return Value:
    
    Allways returns 0 ( TRUE ).
      
--*/
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    InitCommonControls(); // Ensure Common Ctrls are loaded ( List View )

    DialogBoxParam(hInstance,
        MAKEINTRESOURCE(IDD_DIALOG1),
        0,
        (DLGPROC)EnableDisableDialogProc,
        (LPARAM)hInstance);
    
    return 0;
}

