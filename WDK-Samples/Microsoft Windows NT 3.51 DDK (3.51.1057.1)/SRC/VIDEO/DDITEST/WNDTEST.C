/******************************Module*Header*******************************\
* Module Name: wndtest.c
*
* This file contains the code to support a simple window that has
* a menu with a single item called "Test". When "Test" is selected
* vTest(HWND) is called.
*
* Created: 09-Dec-1992 10:44:31
*
* Copyright (C) 1991-1995  Microsoft Corporation
* All rights reserved.
*
\**************************************************************************/

#include <windows.h>
#include "dditest.h"

HANDLE  ghInstance;
HWND    ghwndMain;
HBRUSH  ghbrWhite;
HDC     ghdc;

/***************************************************************************\
* main(argc, argv[])
*
* Sets up the message loop.
*
\***************************************************************************/

_cdecl
main(
    INT   argc,
    PCHAR argv[])
{
    MSG    msg;
    HANDLE haccel;

    ghInstance = GetModuleHandle(NULL);

    if (argc < 2) {
        MessageBox((HWND)NULL, "No filename specified.", "Input File", MB_OK);
        return(0);
    }

    if (!bInitApp(argv[1]))
    {
        return(0);
    }

    haccel = LoadAccelerators(ghInstance, MAKEINTRESOURCE(1));
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg))
        {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }
    return(1);
}

/***************************************************************************\
* bInitApp(PCHAR)
*
* Initializes app.
*
\***************************************************************************/

BOOL bInitApp(PCHAR pstrFileName)
{
    WNDCLASS wc;

    ghbrWhite = CreateSolidBrush(RGB(0x0,0x0,0x0));

    if (!bInitDrawing(pstrFileName)) {
        MessageBox((HWND)NULL, "Invalid file specified", "Input File", MB_OK);
        return(FALSE);
    }

    wc.style            = 0;
    wc.lpfnWndProc      = lMainWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName    = "TestClass";
    if (!RegisterClass(&wc))
    {
        return(FALSE);
    }
    ghwndMain =
      CreateWindowEx(
        0,
        "TestClass",
        "DDI RX Test",
        MY_WINDOWSTYLE_FLAGS,
        80,
        70,
        400,
        300,
        NULL,
        NULL,
        ghInstance,
        NULL
        );
    if (ghwndMain == NULL)
    {
        return(FALSE);
    }
    SetFocus(ghwndMain);

    return(TRUE);
}

/***************************************************************************\
* lMainWindowProc(hwnd, message, wParam, lParam)
*
* Processes all messages for the main window.
*
\***************************************************************************/

LONG
lMainWindowProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static bTestLoop = FALSE;

    switch (message)
    {
    case WM_COMMAND:

        switch(LOWORD(wParam))
        {
        case MM_TEST:
            vTest(hwnd);
            break;

        default:

            break;
        }
        break;

    case WM_SIZE:
        vResize(hwnd);
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);

            vClippedRedraw(hwnd, &ps.rcPaint);
        }
        break;

    case WM_CREATE:
        ghdc = GetDC(hwnd);

        if (!bInitTest(hwnd)) {
            MessageBox((HWND)NULL, "Could not initialize 3D DDI", "DDI Error", MB_OK);
            return(-1);
        }

        break;

    case WM_DESTROY:

        if (!bCloseTest(hwnd)) {
            MessageBox(hwnd, "Failed to terminate properly.","App Error", MB_OK);
        }

        ReleaseDC(hwnd, ghdc);
        DeleteObject(ghbrWhite);
        PostQuitMessage(0);
        return(DefWindowProc(hwnd, message, wParam, lParam));

    default:

        return(DefWindowProc(hwnd, message, wParam, lParam));
    }
    return(0);
}
