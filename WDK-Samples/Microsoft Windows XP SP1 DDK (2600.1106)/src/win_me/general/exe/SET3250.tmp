/***************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

	testapp.cpp

Abstract:

	Application to use with template WDM driver

Environment:

	User mode only

Notes:

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

	Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.


Revision History:

	10-27-96 : created

Author:

	Tom Green

	
****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include "resource.h"
#include "drvshell.h"


// macros

// buffer sizes for loopback and extracting logs and tables
#define MAX_BUFF_SIZE		0x0400
#define MAX_INFO_BUFF_SIZE	0x4000

// timer IDs
#define APP_TIMER_ID		0x0001
#define DLG_TIMER1_ID		0x0002
#define DLG_TIMER2_ID		0x0003


// data structures

// parameters to pass to loopback thread
typedef struct
{
	PSTR		szDeviceName;		// name of device to do loopback on
	HWND		hWnd;				// handle for window that started thread
	BOOL		bKill;				// set this flag to TRUE when you want thread to die
	HANDLE		hThread;			// handle fo thread
	UINT		threadAddr;			// thread ID
} THREAD_PARAMS, *PTHREAD_PARAMS;


// function prototypes

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpszCmdLine, int nCmdShow);

LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

VOID
OnSize(HWND hWnd, UINT state, int cx, int cy);

BOOL
OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);

VOID
OnCommand(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify);

VOID
OnTimer(HWND hWnd, UINT id);

VOID
OnPaint(HWND hWnd);

VOID
OnDestroy(HWND hWnd);

BOOL CALLBACK
About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK
SetSize(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID
GetDriverInfo(HWND hWnd, PSTR szDeviceName, PSTR szLogFile, UINT uIoctl);

VOID
StartLoopback(HWND hWnd, PSTR szDeviceName);

VOID
StopLoopback(VOID);

VOID
ConfigureDriver(HWND hWnd, PSTR szDeviceName, UINT uIoctl);

UINT __stdcall
LoopThread(PVOID params);

UINT __stdcall
DoNothing(PVOID params);


// global variables

// app and window class names
CHAR			szAppName[80];
CHAR			szClassName[80];

// these are used in Dialog boxes to set log and table sizes
PSTR			szSetSizeText;
ULONG			SizeToSet;

// parameters to pass to LoopThread
THREAD_PARAMS	ThreadParams;

// array of icon handles for animated WDM window
HICON			hIcon[4];


// strings to load from string table
CHAR			szOpenFailed[80];
CHAR			szFileOpenFailed[80];
CHAR			szDeviceIOFailed[80];
CHAR			szErrorMsg[30];
CHAR			szBufferCmpFailed[80];
CHAR			szUnderConstruction[80];

/************************************************************************/
/*							WinMain										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Entry point to Win32 application. Registers window class and 		*/
/*  creates window. Includes loop to get messages.						*/
/*																		*/
/************************************************************************/
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpszCmdLine, int nCmdShow)
{
	WNDCLASS	wndClass;
	MSG			msg;
	HANDLE		hAccel;
	HWND		hwnd;

	// load the application name
	LoadString(hInstance, IDS_APP_NAME, szAppName, sizeof(szAppName));

	// load the class name
	LoadString(hInstance, IDS_CLASS_NAME, szClassName, sizeof(szClassName));


	// load error message strings
	LoadString(hInstance, IDS_OPEN_FAILED, szOpenFailed, sizeof(szOpenFailed));
	LoadString(hInstance, IDS_FILE_OPEN_FAILED, szFileOpenFailed, sizeof(szFileOpenFailed));
	LoadString(hInstance, IDS_DEVIO_FAILED, szDeviceIOFailed, sizeof(szDeviceIOFailed));
	LoadString(hInstance, IDS_ERROR_MSG, szErrorMsg, sizeof(szErrorMsg));
	LoadString(hInstance, IDS_BUFFER_CMP_FAILED, szBufferCmpFailed, sizeof(szBufferCmpFailed));
	LoadString(hInstance, IDS_UNDER_CONSTRUCTION, szUnderConstruction, sizeof(szUnderConstruction));
	
	wndClass.style		  	= 0;
	wndClass.lpfnWndProc  	= WndProc;
	wndClass.cbClsExtra	  	= 0;
	wndClass.cbWndExtra	  	= 0;
	wndClass.hInstance	  	= hInstance;
	wndClass.hIcon		  	= LoadIcon(hInstance, "AppIcon");
	wndClass.hCursor	  	= LoadCursor(NULL,IDC_ARROW);
	wndClass.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName 	= "AppMenu";
	wndClass.lpszClassName	= szClassName;

	// register class
	RegisterClass(&wndClass);

	// Create and display the window
	hwnd = CreateWindow(szClassName, szAppName,
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, CW_USEDEFAULT,
						CW_USEDEFAULT, CW_USEDEFAULT,
						NULL, NULL, hInstance, NULL);

	// see if we failed to create window
	if(!hwnd)
		return FALSE;

	// load icons for WDM window
	hIcon[0] = LoadIcon(hInstance, "ICON3");
	hIcon[1] = LoadIcon(hInstance, "ICON2");
	hIcon[2] = LoadIcon(hInstance, "ICON1");
	hIcon[3] = LoadIcon(hInstance, "APPICON");

	// make window visible
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	// load key accelerators
	hAccel = LoadAccelerators(hInstance, "AppAcc");

	// message loop
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(!TranslateAccelerator(hwnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
} // WinMain


/************************************************************************/
/*							WndProc										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handles windows messages. Uses HANDLE_MSG macros for clarity.		*/
/*																		*/
/************************************************************************/
LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
	HANDLE_MSG(hWnd, WM_SIZE, OnSize);
	HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
	HANDLE_MSG(hWnd, WM_TIMER, OnTimer);
	HANDLE_MSG(hWnd, WM_PAINT, OnPaint);
	HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
} // WndProc


/************************************************************************/
/*							OnSize										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handles WM_SIZE messages. Called from WndProc						*/
/*																		*/
/************************************************************************/
VOID
OnSize(HWND hWnd, UINT state, int cx, int cy)
{
	InvalidateRect(hWnd, NULL, TRUE);
} // OnSize

/************************************************************************/
/*							OnCreate									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handles WM_CREATE messages. Called from WndProc.					*/
/*																		*/
/************************************************************************/
BOOL
OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	// set a timer, 1 time a second
	SetTimer(hwnd, APP_TIMER_ID, 1000, NULL);

	return TRUE;
} // OnCreate

/************************************************************************/
/*							OnCommand									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handles WM_COMMAND messages. Called from WndProc.					*/
/*																		*/
/************************************************************************/
VOID
OnCommand(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id)
	{
	case IDM_ABOUT:
		DialogBox((HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE), "AboutBox", hWnd, (DLGPROC) About);
		break;
	case IDM_SAVE_BUFFER_AS:
		MessageBox(hWnd, szUnderConstruction, szUnderConstruction, MB_OK);
		break;
	case IDM_CLEAR_BUFFER:
		MessageBox(hWnd, szUnderConstruction, szUnderConstruction, MB_OK);
		break;
	case IDM_EXIT:
		SendMessage(hWnd, WM_CLOSE, 0, 0L);
		break;
	case IDM_GET_DRIVER_LOG:
		GetDriverInfo(hWnd, "\\\\.\\TESTDRV", "driver.log", GET_DRIVER_LOG);
		break;
	case IDM_GET_IRP_HIST:
		GetDriverInfo(hWnd, "\\\\.\\TESTDRV", "irphist.log", GET_IRP_HIST);
		break;
	case IDM_GET_PATH_HIST:
		GetDriverInfo(hWnd, "\\\\.\\TESTDRV", "pathhist.log", GET_PATH_HIST);
		break;
	case IDM_GET_ERROR_LOG:
		GetDriverInfo(hWnd, "\\\\.\\TESTDRV", "error.log", GET_ERROR_LOG);
		break;
	case IDM_GET_DEVICES:
		GetDriverInfo(hWnd, "\\\\.\\TESTDRV", "devices.log", GET_ATTACHED_DEVICES);
		break;
	case IDM_GET_DRIVER_INFO:
		GetDriverInfo(hWnd, "\\\\.\\TESTDRV", "drvinfo.log", GET_DRIVER_INFO);
		break;
	case IDM_START_LOOPBACK:
		StartLoopback(hWnd, "\\\\.\\TESTDRV");
		break;
	case IDM_STOP_LOOPBACK:
		StopLoopback();
		break;
	case IDM_SET_IRP_HIST_SIZE:
		ConfigureDriver(hWnd, "\\\\.\\TESTDRV", SET_IRP_HIST_SIZE);
		break;
	case IDM_SET_PATH_HIST_SIZE:
		ConfigureDriver(hWnd, "\\\\.\\TESTDRV", SET_PATH_HIST_SIZE);
		break;
	case IDM_SET_ERROR_LOG_SIZE:
		ConfigureDriver(hWnd, "\\\\.\\TESTDRV", SET_ERROR_LOG_SIZE);
		break;
	}
} // OnCommand

/************************************************************************/
/*							OnTimer										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handles WM_TIMER messages. Called from WndProc.						*/
/*																		*/
/************************************************************************/
VOID
OnTimer(HWND hWnd, UINT id)
{
} // OnTimer
			
/************************************************************************/
/*							OnPaint										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handles WM_PAINT messages. Called from WndProc.						*/
/*																		*/
/************************************************************************/
VOID
OnPaint(HWND hWnd)
{
	PAINTSTRUCT		ps;

	BeginPaint(hWnd, &ps);
	EndPaint(hWnd, &ps);
} // OnPaint

/************************************************************************/
/*							OnDestroy									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Handles WM_DESTROY messages. Called from WndProc. Release resouces	*/
/*  here.																*/
/*																		*/
/************************************************************************/
VOID
OnDestroy(HWND hWnd)
{
	// release resources
	KillTimer(hWnd, APP_TIMER_ID);
	PostQuitMessage(0);
} // OnDestroy

/************************************************************************/
/*							About										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Callback routine for About Box.	Includes the ever popular animated	*/
/*  WDM window.															*/
/*																		*/
/************************************************************************/
BOOL CALLBACK
About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static INT			currIcon;

	switch(message)
	{
	case WM_INITDIALOG:
		// kick off a timer, 2 times a second to animate WDM icon
		SetTimer(hDlg, DLG_TIMER1_ID, 500, NULL);

		// init for the current icon
		currIcon = 0;

		// display initial icon
		SendDlgItemMessage(hDlg, IDC_ABOUTBOX_ICON, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon[currIcon]); 
		return TRUE;
	case WM_DESTROY:
		// free up resources
		KillTimer(hDlg, DLG_TIMER1_ID);
		return TRUE;
	case WM_TIMER:
		// cycle through 4 icons
		currIcon = ++currIcon % 4;
		SendDlgItemMessage(hDlg, IDC_ABOUTBOX_ICON, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon[currIcon]); 
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hDlg, TRUE);
		default:
			return TRUE;
		}
		break;
	default:
		break;
	}

	return FALSE;
} // About


/************************************************************************/
/*							SetSize										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Callback routine for Dialog box to set the size of a log or table	*/
/*  in the driver. And, if the animated icon wasn't annoying enough		*/
/*  in the AboutBox, it makes a reappearance here.						*/
/*																		*/
/************************************************************************/
BOOL CALLBACK
SetSize(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL				bTranslated;
	static INT			currIcon;

	switch(message)
	{
	case WM_INITDIALOG:
		// set up static text to describe what resource we are setting and
		// init size to 0
		SetDlgItemText(hDlg, IDC_SIZE_NAME, szSetSizeText);
		SetDlgItemInt(hDlg, IDC_LOG_SIZE, 0, FALSE);

		// kick off a timer, 2 times a second to animate WDM icon
		SetTimer(hDlg, DLG_TIMER2_ID, 500, NULL);

		// init for the current icon
		currIcon = 0;

		// display initial icon
		SendDlgItemMessage(hDlg, IDC_SETLOG_ICON, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon[currIcon]); 
		return TRUE;
	case WM_DESTROY:
		// free up resources
		KillTimer(hDlg, DLG_TIMER2_ID);
		return TRUE;
	case WM_TIMER:
		// cycle through 4 icons
		currIcon = ++currIcon % 4;
		SendDlgItemMessage(hDlg, IDC_SETLOG_ICON, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon[currIcon]); 
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			// get the new size from the edit control if they select OK
			SizeToSet = GetDlgItemInt(hDlg, IDC_LOG_SIZE, &bTranslated, FALSE);
			EndDialog(hDlg, TRUE);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;
		default:
			return TRUE;
		}
		break;
	default:
		break;
	}

	return FALSE;
} // SetSize

/************************************************************************/
/*							GetInformation								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Does one of the information type IOCTLs and writes to a file.		*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  szDeviceName - name of device to open.								*/
/*																		*/
/*  szLogFile    - name of log file to write info to.					*/
/*																		*/
/*  uIoctl       - type of IOCTL to send to driver.						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*  VOID																*/
/*																		*/
/************************************************************************/
VOID
GetDriverInfo(HWND hWnd, PSTR szDeviceName, PSTR szLogFile, UINT uIoctl)
{
	HANDLE			hDev, hFile;
	DWORD			dwRet, dwBytesRet;
	PUCHAR			ucBuff;
	GLOBALHANDLE	hBuff;
	CHAR			tmpBuff[80];

	hDev = CreateFile(szDeviceName,
					  GENERIC_WRITE | GENERIC_READ,
					  FILE_SHARE_WRITE | FILE_SHARE_READ,
					  NULL,
					  OPEN_EXISTING,
					  0,
					  NULL);

	if(hDev != INVALID_HANDLE_VALUE)
	{
		// get a buffer
		hBuff = GlobalAlloc(GHND, MAX_INFO_BUFF_SIZE);

		// lock it down
		ucBuff = (PUCHAR) GlobalLock(hBuff);

		// extract driver log
		dwRet = DeviceIoControl(hDev, 
								uIoctl,
								NULL,				//inbuff
								0, 
								ucBuff,				//outbuff
								MAX_INFO_BUFF_SIZE, 
								&dwBytesRet,
								NULL);
		if(!dwRet)
		{
			wsprintf(tmpBuff, szDeviceIOFailed, szDeviceName);
			MessageBox(hWnd, tmpBuff, szErrorMsg, MB_OK);
		}
		// open log file and write log to it
		else
		{
			hFile = CreateFile(szLogFile,
							   GENERIC_WRITE | GENERIC_READ,
							   FILE_SHARE_WRITE | FILE_SHARE_READ,
							   NULL,
							   CREATE_ALWAYS,
							   0,
							   NULL);
			if(hFile == INVALID_HANDLE_VALUE)
			{
				wsprintf(tmpBuff, szFileOpenFailed, szLogFile);
				MessageBox(hWnd, tmpBuff, szErrorMsg, MB_OK);
			}
			else
			{
				// write the log to the file
				WriteFile(hFile, ucBuff, dwBytesRet, &dwBytesRet, NULL);
				CloseHandle(hFile);
			}
		}

		// free up resources		
		CloseHandle(hDev);
		GlobalUnlock(hBuff);
		GlobalFree(hBuff);
	}
	else
	{
		wsprintf(tmpBuff, szOpenFailed, szDeviceName);
		MessageBox(hWnd, tmpBuff, szErrorMsg, MB_OK);
	}

} // GetDriverInfo

/************************************************************************/
/*							StartLoopback								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Start in-memory loopback. Starts up LoopThread. Fills in parameter	*/
/*  structure to pass in.												*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  szDeviceName - name of device for LoopThread to open.				*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*  VOID																*/
/*																		*/
/************************************************************************/
VOID
StartLoopback(HWND hWnd, PSTR szDeviceName)
{
	// start up LoopThread, fill in parameters structure
	ThreadParams.szDeviceName = szDeviceName;
	ThreadParams.hWnd = hWnd;
	ThreadParams.bKill = FALSE;

	ThreadParams.hThread = (HANDLE) _beginthreadex(NULL, 0, LoopThread,
												   (PVOID) &ThreadParams,
												   0, &ThreadParams.threadAddr);
} // StartLoopback


/************************************************************************/
/*							StopLoopback								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Stop in-memory loopback. Tells LoopThread to die by setting flag.	*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  VOID																*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*  VOID																*/
/*																		*/
/************************************************************************/
VOID
StopLoopback(VOID)
{
	// kill the thread
	ThreadParams.bKill = TRUE;

	WaitForSingleObject(ThreadParams.hThread, INFINITE);

	CloseHandle(ThreadParams.hThread);
} // StopLoopback


/************************************************************************/
/*							ConfigureDriver								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Configure log and table sizes.										*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*  VOID																*/
/*																		*/
/************************************************************************/
VOID
ConfigureDriver(HWND hWnd, PSTR szDeviceName, UINT uIoctl)
{
	HANDLE			hDev;
	DWORD			dwRet, dwBytesRet;
	CHAR			tmpBuff[80];

	switch(uIoctl)
	{
	case SET_IRP_HIST_SIZE:
		szSetSizeText = "Set IRP History Size";
		if(!DialogBox((HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE), "SetLogSize", hWnd, (DLGPROC) SetSize))
			return;
		break;
	case SET_PATH_HIST_SIZE:
		szSetSizeText = "Set Path History Size";
		if(!DialogBox((HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE), "SetLogSize", hWnd, (DLGPROC) SetSize))
			return;
		break;
	case SET_ERROR_LOG_SIZE:
		szSetSizeText = "Set Error Log Size";
		if(!DialogBox((HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE), "SetLogSize", hWnd, (DLGPROC) SetSize))
			return;
		break;
	}

	hDev = CreateFile(szDeviceName,
					  GENERIC_WRITE | GENERIC_READ,
					  FILE_SHARE_WRITE | FILE_SHARE_READ,
					  NULL,
					  OPEN_EXISTING,
					  0,
					  NULL);

	if(hDev != INVALID_HANDLE_VALUE)
	{
		// configure driver
		dwRet = DeviceIoControl(hDev, 
								uIoctl,
								&SizeToSet,			//inbuff
								sizeof(SizeToSet), 
								NULL,				//outbuff
								0, 
								&dwBytesRet,
								NULL);
		if(!dwRet)
		{
			wsprintf(tmpBuff, szDeviceIOFailed, szDeviceName);
			MessageBox(hWnd, tmpBuff, szErrorMsg, MB_OK);
		}

		CloseHandle(hDev);
	}
	else
	{
		wsprintf(tmpBuff, szOpenFailed, szDeviceName);
		MessageBox(hWnd, tmpBuff, szErrorMsg, MB_OK);
	}
} // ConfigureDriver


/************************************************************************/
/*							LoopThread									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Thread to do in-memory loopback. This must be declared __stdcall	*/
/*  to override compiler set calling conventions.						*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  params - pointer to parameter structure passed into thread			*/
/*           (THREAD_PARAMS type)										*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*  UINT																*/
/*																		*/
/************************************************************************/
UINT __stdcall
LoopThread(PVOID params)
{
	PTHREAD_PARAMS	pThreadParams;
	HANDLE			hDev;
	DWORD			dwBytesRet;
	UINT			iter;
	PUCHAR			ucWriteBuff;
	PUCHAR			ucReadBuff;
	INT				memcmpResult;
	HMENU			hMenu;
	CHAR			tmpBuff[80];

	// get pointer to our thread parameters
	pThreadParams = (PTHREAD_PARAMS) params;

	// try to open the device
	hDev = CreateFile(pThreadParams->szDeviceName,
					  GENERIC_WRITE | GENERIC_READ,
					  FILE_SHARE_WRITE | FILE_SHARE_READ,
					  NULL,
					  OPEN_EXISTING,
					  0,
					  NULL);

	// flop menus
	hMenu = GetMenu(pThreadParams->hWnd);
	EnableMenuItem(hMenu, IDM_START_LOOPBACK, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_STOP_LOOPBACK, MF_ENABLED);

	// device failed to open
	if(hDev == INVALID_HANDLE_VALUE)
	{
		wsprintf(tmpBuff, szOpenFailed, pThreadParams->szDeviceName);
		MessageBox(pThreadParams->hWnd, tmpBuff, szErrorMsg, MB_OK);
	}
	// device opened, so let's do loopback
	else
	{

		// allocate read and write buffers
		ucWriteBuff = (PUCHAR) GlobalAlloc(GPTR, MAX_BUFF_SIZE);
		ucReadBuff  = (PUCHAR) GlobalAlloc(GPTR, MAX_BUFF_SIZE);

		// loop until the thread is killed
		while(!pThreadParams->bKill)
		{
			// fill a buffer of 1 to MAX_BUFF_SIZE,
			// write buffer, read buffer and compare
			for(iter = 1; iter <= MAX_BUFF_SIZE; iter++)
			{
				// fill buffer with low byte of iter
				memset(ucWriteBuff, (CHAR)iter, iter);

				// write to device
				WriteFile(hDev, ucWriteBuff, iter, &dwBytesRet, NULL);

				// read from device
				ReadFile(hDev, ucReadBuff, iter, &dwBytesRet, NULL);

				// compare buffers
				memcmpResult = memcmp(ucReadBuff, ucWriteBuff, iter);

				// check results of memcmp
				if(memcmpResult != 0)
				{
					MessageBox(pThreadParams->hWnd, szBufferCmpFailed, szErrorMsg, MB_OK);
					goto LoopThread_End;
				}

				// see if we have been shut down
				if(pThreadParams->bKill)
					break;
			}
		}
LoopThread_End:
		// free up all resources
		CloseHandle(hDev);
		GlobalFree(ucWriteBuff);
		GlobalFree(ucReadBuff);
	}

	// flop menus again
	EnableMenuItem(hMenu, IDM_START_LOOPBACK, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_STOP_LOOPBACK, MF_GRAYED);

	// that's it, shut this thread down
	_endthread();
	return(0);
} // LoopThread

