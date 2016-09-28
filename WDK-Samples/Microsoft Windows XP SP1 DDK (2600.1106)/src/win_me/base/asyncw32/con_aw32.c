/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright 1993-95  Microsoft Corporation.  All Rights Reserved.           *
*                                                                           *
****************************************************************************/

/****************************************************************************
*
* PROGRAM: CON_SAMP.C
*
* PURPOSE: Simple console application for calling ASYNCW32 (C VxD Sample) VxD
*
* FUNCTIONS:
*  main() - Console application calls VMM and VKD through ASYNCW32 which
*           supports DeviceIoControl. ASYNCW32 will return values to this
*           application through this same DeviceIoControl interface.
*
* SPECIAL INSTRUCTIONS:
*
****************************************************************************/

#include <stdio.h>
#include <windows.h>
#include <vmm.h>

#define ASYNCW32_PROC 1

#define ERROR_OUT(str) {printf(str);CloseHandle(hVxD);return(0);}

int main()
{
    HANDLE      hVxD = 0, hEvent = 0;
    OVERLAPPED  ovlp = {0,0,0,0,0};
    BYTE        InBuff[16], OutBuff[16];
    DWORD       cbIn, cbOut, nTicks;
    DWORD       cbRet;
    DWORD       dwErrorCode;

    //
    // Dynamically load and prepare to call ASYNCW32
    // The FILE_FLAG_DELETE_ON_CLOSE flag is used so that CloseHandle can
    // be used to dynamically unload the VxD.
    // The FILE_FLAG_OVERLAPPED flag is used to inform the Win32 subsystem
    // that the VxD will be processing some DeviceIOControl calls
    // asynchronously.
    //
    hVxD = CreateFile("\\\\.\\ASYNCW32.VXD", 0,0,0,0,
                       FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OVERLAPPED, 0);

    if ( hVxD == INVALID_HANDLE_VALUE )
    {
        dwErrorCode = GetLastError();
        if ( dwErrorCode == ERROR_NOT_SUPPORTED )
        {
            printf("Unable to open VxD,\ndevice does not support DeviceIOCTL\n");
        }
        else
        {
            printf("Unable to open VxD, Error code: %lx\n", dwErrorCode);
        }
        return(0);
    }

    // Create event which will be used by VxD to indicate operation is
    // complete
    if ( !(hEvent = CreateEvent(0, TRUE, 0, NULL)) )
        ERROR_OUT("CreateEvent failed!\n")

    // Prepare data for call to VxD.
    cbIn  = sizeof(InBuff);
    cbOut = sizeof(OutBuff);
    lstrcpy(InBuff,"gnirtSBackwards");
    printf("DevIoctl input: %s\n", InBuff);
    ovlp.hEvent = hEvent;
    
    // Make Asynchronous VxD call here
    if ( !DeviceIoControl(hVxD, ASYNCW32_PROC, InBuff, cbIn,
                            OutBuff, cbOut, &cbRet, &ovlp) )
    {
        if ( GetLastError() == ERROR_IO_PENDING )
            printf("VxD correctly returned operation incomplete.\n");
        else
            ERROR_OUT("VxD does not support the requested API!!!\n");
    }
    else
        ERROR_OUT("VxD processed the call synchronously!!!\n");

    //
    // DeviceIoControl call will have returned without completing
    // requested function. GetOverlappedResult at this point
    // should return ERROR_IO_INCOMPLETE if called w/ fWait=FALSE.
    //
    if ( !GetOverlappedResult(hVxD, &ovlp, &cbRet, FALSE) )
    {
        if ( GetLastError() == ERROR_IO_INCOMPLETE )
            printf("GetOverlappedResult returned expected value.\n");
        else
            ERROR_OUT("GetOverlappedResult returned unexpected error.\n");
    }
    else
        ERROR_OUT("GetOverlappedResult unexpectedly returned success.\n");

    nTicks = GetTickCount();

    //
    // This call to GetOverlappedResult will suspend this thread 
    // until the operation is completed by the VxD. I.e. until the 
    // VxD calls DIOC_VWIN32CompletionRoutine.
    //
    GetOverlappedResult(hVxD, &ovlp, &cbRet, TRUE);

    nTicks = GetTickCount() - nTicks; // This will wrap after 47 days

    printf("DevIoctl Call elapsed time: %d ms\n", nTicks);
    printf("DevIoctl returned: %d bytes\n", cbRet);
    printf("DevIoctl output: %s\n", OutBuff);

    // Dynamically UNLOAD the Virtual Device sample.
    CloseHandle(hVxD);

    CloseHandle(hEvent);    // This would be freed in any case on exit.

    return(0);
}
