/*++
Copyright (c) 1992  Microsoft Corporation

Module Name:

    Detapp.c

Abstract:

    Window NT Netdetect tester

Author:

    Brian Lieuallen (BrianL)

Environment:

    WIN32 User Mode

Revision History:

--*/



#define UNICODE 1

#include <windows.h>
#include <windowsx.h>

#include "detapp.h"

#include "dialogs.h"

TCHAR   szWindowTitle[] = TEXT("Net Detect DLL Tester");
TCHAR   szAppName[]     = TEXT("DETAPP");


HINSTANCE  hInst;


DETECT  DetectBlock;




LRESULT FAR PASCAL
WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );




int PASCAL
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpszCmdParam,
    int       nCmdShow
    )


{
    HWND      hwnd;
    MSG       msg;
    WNDCLASS  wndclass;

    hInst=hInstance;

    if (!hPrevInstance) {

         wndclass.style        =  CS_HREDRAW | CS_VREDRAW;
         wndclass.lpfnWndProc  =  WndProc;
         wndclass.cbClsExtra   =  0;
         wndclass.cbWndExtra   =  sizeof(PVOID);
         wndclass.hInstance    =  hInstance;
         wndclass.hIcon        =  LoadIcon (hInst, MAKEINTRESOURCE(3000));
         wndclass.hCursor      =  LoadCursor(NULL, IDC_ARROW);
         wndclass.hbrBackground=  GetStockObject(WHITE_BRUSH);
	 wndclass.lpszMenuName =  TEXT("GenericMenu");
	 wndclass.lpszClassName=  szAppName;

         RegisterClass(&wndclass);
    }


    hwnd = CreateWindow (szAppName,
                         szWindowTitle,
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         NULL,
                         NULL,
                         hInstance,
                         NULL);

    ShowWindow (hwnd,nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage (&msg, NULL, 0,0)) {

        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }

    return (msg.wParam);
}


LRESULT FAR PASCAL
WndProc(
    HWND    hWnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )

{
    HDC       hdc;
    PAINTSTRUCT  ps;
    RECT         rect;

    LPDETECT     Detect;


    Detect=(LPDETECT)GetWindowLong(hWnd, GWL_USERDATA);

    switch (message) {

        case WM_COMMAND:

            if (wParam==IDM_ABOUT) {

                CallUpDlgBox(
                    hWnd,
                    AboutBoxProc,
                    MAKEINTRESOURCE(IDD_ABOUT),
                    (LPARAM)Detect
                    );

                return 0;
            }


            if (wParam >= IDM_FIRST_DLL && wParam <= (IDM_FIRST_DLL+Detect->NumberOfDlls)) {

                Detect->CurrentDll=wParam-IDM_FIRST_DLL;

                CallUpDlgBox(hWnd,
                             DetectBoxProc,
                             MAKEINTRESOURCE(IDD_DETECT),
                             (LPARAM)Detect);



                return 0;

            }

            return 0;



        case WM_CREATE:

            return CreationHandler(hWnd);


        case WM_KEYDOWN:

            return 0;

        case WM_PAINT:

            hdc=BeginPaint(hWnd,&ps);
            GetClientRect (hWnd,&rect);

            EndPaint(hWnd,&ps);

            return 0;


        case WM_DESTROY:

            PostQuitMessage(0);

            return 0;

    }

    return DefWindowProc(hWnd,message, wParam, lParam);
}


LRESULT
CreationHandler(
    HWND    hWnd
    )
{

    LPDETECT     Detect;
    UINT         i;

    //
    //  Save our global structure
    //
    SetWindowLong(hWnd,GWL_USERDATA,(ULONG)&DetectBlock);

    Detect=(LPDETECT)GetWindowLong(hWnd, GWL_USERDATA);

    if (GetNetDetectDlls(Detect)) {

        if (StartNetDetectDriver(Detect)) {

            Detect->hMenuBar=GetMenu(hWnd);
            Detect->hMenuPopup1= GetSubMenu(Detect->hMenuBar,0);

            for (i=0; i < Detect->NumberOfDlls ; i++) {

                AppendMenu(
                    Detect->hMenuPopup1,
                    MF_ENABLED | MF_STRING,
                    IDM_FIRST_DLL+i,
                    Detect->DllList[i].pLibName
                    );

            }

            DrawMenuBar(hWnd);

            return 0;
        }
    }

    return -1;

}




BOOLEAN
GetNetDetectDlls(
    LPDETECT   Detect
    )

{
    LONG       Status;
    ULONG      BufferSize=512;
    PTSTR      pStr;
    PTSTR      pLibName;
    HINSTANCE  hLib;

    ULONG      i;


    pStr=GlobalAllocPtr(
             GMEM_MOVEABLE | GMEM_ZEROINIT,
             BufferSize
             );

    if (pStr==NULL) {

       return FALSE;
    }


    Status=ReadRegistry(
               pStr,
               &BufferSize
               );

    if (Status == ERROR_SUCCESS) {

        Detect->NumberOfDlls=0;

        pLibName=pStr;

        for (i=0; i<BufferSize/sizeof(TCHAR); i++) {

            if (pStr[i] == (TCHAR)0) {

                hLib=LoadLibrary(pLibName);

                if (hLib != NULL) {


                    Detect->DllList[Detect->NumberOfDlls].IdentifyProc=(NC_DETECT_IDENTIFY)
                           GetProcAddress(
                               hLib,
                               "NcDetectIdentify"
                               );

                    Detect->DllList[Detect->NumberOfDlls].QueryMaskProc=(NC_DETECT_QUERY_MASK)
                           GetProcAddress(
                               hLib,
                               "NcDetectQueryMask"
                               );

                    Detect->DllList[Detect->NumberOfDlls].FirstNextProc=(NC_DETECT_FIRST_NEXT)
                           GetProcAddress(
                               hLib,
                               "NcDetectFirstNext"
                               );

                    Detect->DllList[Detect->NumberOfDlls].OpenProc=(NC_DETECT_OPEN_HANDLE)
                           GetProcAddress(
                               hLib,
                               "NcDetectOpenHandle"
                               );

                    Detect->DllList[Detect->NumberOfDlls].QueryProc=(NC_DETECT_QUERY_CFG)
                           GetProcAddress(
                               hLib,
                               "NcDetectQueryCfg"
                               );

                    Detect->DllList[Detect->NumberOfDlls].CloseProc=(NC_DETECT_CLOSE_HANDLE)
                           GetProcAddress(
                               hLib,
                               "NcDetectCloseHandle"
                               );

                    Detect->DllList[Detect->NumberOfDlls].CreateProc=(NC_DETECT_CREATE_HANDLE)
                           GetProcAddress(
                               hLib,
                               "NcDetectCreateHandle"
                               );

                    Detect->DllList[Detect->NumberOfDlls].RangeProc=(NC_DETECT_PARAM_RANGE)
                           GetProcAddress(
                               hLib,
                               "NcDetectParamRange"
                               );

                    Detect->DllList[Detect->NumberOfDlls].VerifyProc=(NC_DETECT_VERIFY_CFG)
                           GetProcAddress(
                               hLib,
                               "NcDetectVerifyCfg"
                               );


                    if ((Detect->DllList[Detect->NumberOfDlls].IdentifyProc == NULL)  ||
                        (Detect->DllList[Detect->NumberOfDlls].QueryMaskProc == NULL) ||
                        (Detect->DllList[Detect->NumberOfDlls].FirstNextProc == NULL) ||
                        (Detect->DllList[Detect->NumberOfDlls].OpenProc == NULL)      ||
                        (Detect->DllList[Detect->NumberOfDlls].QueryProc == NULL)     ||
                        (Detect->DllList[Detect->NumberOfDlls].CloseProc == NULL)     ||
                        (Detect->DllList[Detect->NumberOfDlls].CreateProc == NULL)    ||
                        (Detect->DllList[Detect->NumberOfDlls].RangeProc==NULL)) {

                        MessageBox(
                            NULL,
                            TEXT("Failed to get procedure address from DLL"),
                            szWindowTitle,
                            MB_OK | MB_ICONEXCLAMATION
                            );


                        return FALSE;
                    }



                    Detect->DllList[Detect->NumberOfDlls].hLib=hLib;

                    Detect->DllList[Detect->NumberOfDlls].pLibName=pLibName;

                    Detect->NumberOfDlls++;

                }

                if (pStr[i+1] == (TCHAR)0) {

                    break;

                } else {

                    pLibName=&pStr[i+1];
                }

            }
        }
    }


    return TRUE;

}



ULONG
ReadRegistry(
    PTSTR   pStr,
    PULONG  BufferSize
    )

{

    HKEY       SystemKey;
    HKEY       SetupKey;
    LONG       Status;

    DWORD      RegType;

    Status=RegOpenKeyEx(
               HKEY_LOCAL_MACHINE,
               TEXT("SYSTEM"),
               0,
               KEY_READ,
               &SystemKey
               );

    if (Status == ERROR_SUCCESS) {

        Status=RegOpenKeyEx(
                   SystemKey,
                   TEXT("Setup"),
                   0,
                   KEY_READ,
                   &SetupKey
                   );


        if (Status == ERROR_SUCCESS) {

            Status=RegQueryValueEx(
                       SetupKey,
                       TEXT("NetcardDlls"),
                       NULL,
                       &RegType,
                       (LPBYTE)pStr,
                       BufferSize
                       );

            RegCloseKey(SetupKey);

        }

        RegCloseKey(SystemKey);
    }

    return Status;

}



BOOLEAN
StartNetDetectDriver(
    LPDETECT  Detect
    )

{

    BOOLEAN  Status;

    /* Open a handle to the SC Manager database. */

    Detect->schSCManager = OpenSCManager(
                                 NULL,                   /* local machine           */
                                 NULL,                   /* ServicesActive database */
                                 SC_MANAGER_ALL_ACCESS); /* full access rights      */

    if (Detect->schSCManager == NULL) {

        MessageBox(NULL,TEXT("Could not open SC"),szWindowTitle,MB_OK);

    } else {

        Detect->ServiceHandle=OpenService(Detect->schSCManager,
                                          TEXT("NetDetect"),
                                          SERVICE_START);

        if (Detect->ServiceHandle == NULL) {

            MessageBox(NULL,TEXT("Could not open service"),szWindowTitle,MB_OK);

        }

        Status=StartService(
                   Detect->ServiceHandle,
                   0,
                   NULL
                   );

        if (!Status) {

            if (GetLastError()==ERROR_SERVICE_ALREADY_RUNNING) {

                return TRUE;
            }
        }

        return Status;



    }

    return FALSE;

}
