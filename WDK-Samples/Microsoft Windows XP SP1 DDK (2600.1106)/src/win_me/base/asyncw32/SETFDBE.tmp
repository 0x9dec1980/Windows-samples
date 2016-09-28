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

#define WANTVXDWRAPS

#include <basedef.h>
#include <vmm.h>
#include <debug.h>
#include <vxdwraps.h>
#include <vwin32.h>
#include <winerror.h>

//
// define wrappers for VWIN32_DIOCCompletionRoutine and Set_Thread_Time_Out
//
MAKE_HEADER(VOID,    _stdcall, VWIN32_DIOCCompletionRoutine, (DWORD hEvent))
#define VWIN32_DIOCCompletionRoutine PREPEND(VWIN32_DIOCCompletionRoutine)

#define ASYNCW32_VERSION    0x400
#define ASYNCW32_PROC       1

typedef DIOCPARAMETERS *LPDIOC;
typedef OVERLAPPED *LPOVERLAPPED;
typedef char *PSTR;

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

DWORD cbIn, cbOut; // Sizes of input and output buffers
PSTR  lpIn, lpOut; // Pointers to input and output buffers
LPOVERLAPPED lpo;  // Pointer to overlapped structure

//
// Prototypes
//
DWORD _stdcall ASYNCW32_DeviceIOControl(DWORD, DWORD, DWORD, LPDIOC);
DWORD _stdcall ASYNCW32_CleanUp(void);
DWORD _stdcall ASYNCW32_Proc(DWORD, DWORD, LPDIOC);
void  _cdecl   ASYNCW32_Event_Callback(void);
DWORD _stdcall MyPageLock(DWORD, DWORD);
void  _stdcall MyPageUnlock(DWORD, DWORD);

/****************************************************************************
                  ASYNCW32_DeviceIOControl
****************************************************************************/
DWORD _stdcall ASYNCW32_DeviceIOControl(DWORD  dwService,
                                        DWORD  dwDDB,
                                        DWORD  hDevice,
                                        LPDIOC lpDIOCParms)
{
    DWORD dwRetVal = 0;

    // DIOC_OPEN is sent when VxD is loaded w/ CreateFile 
    //  (this happens just after SYS_DYNAMIC_INIT)
    if ( dwService == DIOC_OPEN )
    {
        Out_Debug_String("ASYNCW32: WIN32 DEVIOCTL supported here!\n\r");
        // Must return 0 to tell WIN32 that this VxD supports DEVIOCTL
        dwRetVal = 0;
    }
    // DIOC_CLOSEHANDLE is sent when VxD is unloaded w/ CloseHandle
    //  (this happens just before SYS_DYNAMIC_EXIT)
    else if ( dwService == DIOC_CLOSEHANDLE )
    {
        // Dispatch to cleanup proc
        dwRetVal = ASYNCW32_CleanUp();
    }
    // This sample only has one control code, see cvxd32 sample
    // for handling of multiple control codes.
    else if ( dwService == ASYNCW32_PROC )
    {
        // This check for presence of overlapped structure is not
        // necessary for this sample, but should be done in most
        // cases. If OVERLAPPED structure is not passed in, call
        // cannot be handled asynchronously.
        if ( !lpDIOCParms->lpoOverlapped )
            dwRetVal = ERROR_NOT_SUPPORTED;
        else // CALL requested service
            dwRetVal = ASYNCW32_Proc(dwDDB, hDevice, lpDIOCParms);
    }
    else
    {
        // Returning a positive value will cause the WIN32 DeviceIOControl
        // call to return FALSE, the error code can then be retrieved
        // via the WIN32 API GetLastError.
        dwRetVal = ERROR_NOT_SUPPORTED;
    }
    return(dwRetVal);
}

DWORD _stdcall ASYNCW32_Proc(DWORD dwDDB, DWORD hDevice, LPDIOC lpDIOCtl)
{
    PTCB hThrd;

    Out_Debug_String("ASYNCW32: ASYNCW32_Proc\n\r");

    lpIn = (PSTR)lpDIOCtl->lpvInBuffer;
    cbIn = lpDIOCtl->cbInBuffer;
    lpOut = (PSTR)lpDIOCtl->lpvOutBuffer;
    cbOut = lpDIOCtl->cbOutBuffer;
    lpo = (LPOVERLAPPED)lpDIOCtl->lpoOverlapped;

    //
    // All buffers that will be used asynchronously need to be locked
    // and mapped to global space if they might be accessed out of context. 
    // This is certainly true of this sample, as the current thread will 
    // be suspended pending the execution of the VWIN32_DIOCCompletionRoutine 
    // call.
    //
    lpIn = (PSTR)MyPageLock((DWORD)lpIn, cbIn);
    lpOut = (PSTR)MyPageLock((DWORD)lpOut, cbOut);
    lpo = (LPOVERLAPPED)MyPageLock((DWORD)lpo, sizeof(OVERLAPPED));

    //
    // Set up asynchronous processing of buffers - in this case
    // just to occur 1 second in the future. When the time out
    // event occurs it will occur in a different context than
    // CON_AW32, as that thread is suspended via the 
    // GetOverlappedResult call.
    //
    Set_Global_Time_Out(ASYNCW32_Event_Callback, 1000, 0);

    return(-1); // This will make DeviceIOControl return ERROR_IO_PENDING
}

DWORD _stdcall ASYNCW32_Dynamic_Exit(void)
{
    Out_Debug_String("ASYNCW32: Dynamic Exit\n\r");

    return(VXD_SUCCESS);
}

DWORD _stdcall ASYNCW32_CleanUp(void)
{
    Out_Debug_String("ASYNCW32: Cleaning Up\n\r");

    // It is necessary to free the global aliases and there memory
    MyPageUnlock((DWORD)lpIn, cbIn);
    MyPageUnlock((DWORD)lpOut, cbOut);
    MyPageUnlock((DWORD)lpo, sizeof(OVERLAPPED));

    return(VXD_SUCCESS);
}

void _cdecl ASYNCW32_Event_Callback(void)
{
    int i, j;

    // Reverse lpIn into lpOut
    for ( i = 0, j = cbIn-2; i < cbIn-1; i++, j-- )
        lpOut[i] = lpIn[j];

    lpOut[15] = '\0'; // Null terminate string

    // This is the number of bytes we are writing into the out buffer
    lpo->O_InternalHigh = cbOut;

    //
    // The internal member of overlapped structure contains
    // a pointer to the event structure that will be signalled,
    // resuming the execution of the waitng GetOverlappedResult
    // call.
    //
    VWIN32_DIOCCompletionRoutine(lpo->O_Internal);
}

DWORD _stdcall MyPageLock(DWORD lpMem, DWORD cbSize)
{
    DWORD LinPageNum, LinOffset, nPages;

    LinOffset = lpMem & 0xfff; // page offset of memory to map
    LinPageNum = lpMem >> 12;  // generate page number

    // Calculate # of pages to map globally
    nPages = ((lpMem + cbSize) >> 12) - LinPageNum + 1;

    //
    // Return global mapping of passed in pointer, as this new pointer
    // is how the memory must be accessed out of context.
    //
    return (_LinPageLock(LinPageNum, nPages, PAGEMAPGLOBAL) + LinOffset);
}

void _stdcall MyPageUnlock(DWORD lpMem, DWORD cbSize)
{
    DWORD LinPageNum, nPages;

    LinPageNum = lpMem >> 12;
    nPages = ((lpMem + cbSize) >> 12) - LinPageNum + 1;

    // Free globally mapped memory
    _LinPageUnLock(LinPageNum, nPages, PAGEMAPGLOBAL);
}

#pragma VxD_ICODE_SEG
#pragma VxD_IDATA_SEG

DWORD _stdcall ASYNCW32_Dynamic_Init(void)
{
    Out_Debug_String("ASYNCW32: Dynamic Init\n\r");

    return(VXD_SUCCESS);
}



