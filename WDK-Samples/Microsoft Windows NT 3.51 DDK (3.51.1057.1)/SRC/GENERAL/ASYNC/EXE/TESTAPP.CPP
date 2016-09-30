//////////////////////////////////////////////////////////////////////////////
// Program Name: TestApp.CPP
// Programmer: Mark A. Overby (MarkOv)
// Program Description: This is a multi-threaded app to demonstrate how a
//                      user mode application can be paired with a kernel
//                      mode device driver to have the user-mode app be
//                      notified for events.
//
// NOTE: This application requires that the driver ASYNC be loaded in order
//       to function properly.
//
// Copyright (c) 1994-1995 Microsoft Corporation
//////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// INCLUDES & DEFINES
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "devioctl.h"
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include "control.h"

VOID EventSignal (LPDWORD lpdwParam) ;
VOID ReleaseEvent (LPDWORD lpdwParam) ;

//////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

void __cdecl main (void)
{
	DWORD dwThreadId ;
	DWORD junk ;
	HANDLE hThread ;
	HANDLE hDriver ;

	hDriver = CreateFile ("\\\\.\\IrpTest", GENERIC_READ | GENERIC_WRITE,
								 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED,
								 NULL) ;

	if (hDriver == INVALID_HANDLE_VALUE) {
		printf("Bad handle value. Could not open driver.\n") ;
		exit(0) ;
	}
	printf ("Handle # = %p\n ", hDriver) ;
	hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) EventSignal,
									&hDriver, 0, &dwThreadId) ;
	if (hThread == NULL) {
		printf("Bad thread create. Exiting.\n") ;
		exit(0) ;
	}

	do {
		BOOL bTest ;
      HANDLE hEvt ;
      OVERLAPPED ovl ;

      hEvt = CreateEvent (NULL, TRUE, FALSE, NULL) ;
      if (!hEvt) {
         printf("Error on CreateEvent.\n") ;
         exit (0) ;
      }

      ovl.hEvent = hEvt ;

		bTest = DeviceIoControl (hDriver, IOCTL_DO_NOTHING, NULL, 0, NULL, 0,
										 &junk, &ovl) ;
		if (!bTest && (GetLastError() == ERROR_IO_PENDING)) {
			printf(".") ;
			Sleep (1000) ;
		}
		else {
			printf("Error in DeviceIoControl (IOCTL_DO_NOTHING).\n") ;
			exit(0) ;
		}
	} while (1);
}

void EventSignal (LPDWORD lpdwParam)
{
	HANDLE hDrv = (HANDLE) *lpdwParam ;
	HANDLE hEvt ;
	BOOL bTester ;
	DWORD dwData ;
	OVERLAPPED ovl ;


	hEvt = CreateEvent (NULL, TRUE, FALSE, NULL) ;
	if (!hEvt) {
		printf("Error on CreateEvent.\n") ;
		exit (0) ;
	}

	ovl.hEvent = hEvt ;

	do {
		bTester = DeviceIoControl (hDrv, IOCTL_HOLD_REQUEST, NULL, 0, NULL, 0,
									  &dwData, &ovl) ;
		if (!bTester && (GetLastError() == ERROR_IO_PENDING)) {
			printf("Event waiting.\n") ;
			WaitForSingleObject (hEvt, INFINITE) ;
			printf("Event signalled!.\n") ;
			ResetEvent (hEvt) ;
		}
		else {
			printf("Error! DeviceIoControl IOCTL_HOLD_REQUEST.\n") ;
			printf("Error code: %d\n", GetLastError()) ;
			exit (0) ;
      }
	} while ( 1 );

}

