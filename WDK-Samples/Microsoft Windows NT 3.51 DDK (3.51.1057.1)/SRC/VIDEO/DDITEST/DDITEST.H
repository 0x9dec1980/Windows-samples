/****************************** Module Header ******************************\
* Module Name: dditest.h
*
* Copyright (C) 1994-1995 Microsoft Corporation
* All rights reserved.
\***************************************************************************/

#define DONTUSE(x) (x)

#define MM_NEW          0x8001
#define MM_EXIT         0x8002
#define MM_TEST         0x8003

BOOL bInitApp(PCHAR);
VOID vTest(HWND);
VOID vRedraw(HWND);
VOID vClippedRedraw(HWND, RECT*);
VOID vResize(HWND);
BOOL bInitDrawing(PCHAR);
BOOL bCloseTest(HWND);
BOOL bInitTest(HWND);
LONG lMainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#define MY_WINDOWSTYLE_FLAGS       \
            (                      \
                WS_OVERLAPPED   |  \
                WS_CAPTION      |  \
                WS_BORDER       |  \
                WS_THICKFRAME   |  \
                WS_MAXIMIZEBOX  |  \
                WS_MINIMIZEBOX  |  \
                WS_CLIPCHILDREN |  \
                WS_VISIBLE      |  \
                WS_SYSMENU         \
            )
