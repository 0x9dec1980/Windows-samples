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
#include "myvkd.h"
#include "vkdwraps.h"

#define CVXD_VERSION 0x400

#define CVXD_V86_FUNCTION1 1
#define CVXD_V86_FUNCTION2 2
#define CVXD_PM_FUNCTION1  1
#define CVXD_PM_FUNCTION2  2

typedef DIOCPARAMETERS *LPDIOC;

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

HVM hSysVM;

DWORD _stdcall CVXD_W32_DeviceIOControl(DWORD, DWORD, DWORD, LPDIOC);
DWORD _stdcall CVXD_CleanUp(void);
DWORD _stdcall CVXD_W32_Proc1(DWORD, DWORD, LPDIOC);
DWORD _stdcall CVXD_W32_Proc2(DWORD, DWORD, LPDIOC);

DWORD ( _stdcall *CVxD_W32_Proc[] )(DWORD, DWORD, LPDIOC) = {
        CVXD_W32_Proc1,
        CVXD_W32_Proc2};

#define MAX_CVXD_W32_API (sizeof(CVxD_W32_Proc)/sizeof(DWORD))

/****************************************************************************
                  CVXD_W32_DeviceIOControl
****************************************************************************/
DWORD _stdcall CVXD_W32_DeviceIOControl(DWORD  dwService,
                                        DWORD  dwDDB,
                                        DWORD  hDevice,
                                        LPDIOC lpDIOCParms)
{
    DWORD dwRetVal = 0;

    // DIOC_OPEN is sent when VxD is loaded w/ CreateFile 
    //  (this happens just after SYS_DYNAMIC_INIT)
    if ( dwService == DIOC_OPEN )
    {
        Out_Debug_String("CVXDSAMP: WIN32 DEVIOCTL supported here!\n\r");
        // Must return 0 to tell WIN32 that this VxD supports DEVIOCTL
        dwRetVal = 0;
    }
    // DIOC_CLOSEHANDLE is sent when VxD is unloaded w/ CloseHandle
    //  (this happens just before SYS_DYNAMIC_EXIT)
    else if ( dwService == DIOC_CLOSEHANDLE )
    {
        // Dispatch to cleanup proc
        dwRetVal = CVXD_CleanUp();
    }
    else if ( dwService > MAX_CVXD_W32_API )
    {
        // Returning a positive value will cause the WIN32 DeviceIOControl
        // call to return FALSE, the error code can then be retrieved
        // via the WIN32 GetLastError
        dwRetVal = ERROR_NOT_SUPPORTED;
    }
    else
    {
        // CALL requested service
        dwRetVal = (CVxD_W32_Proc[dwService-1])(dwDDB, hDevice, lpDIOCParms);
    }
    return(dwRetVal);
}

DWORD _stdcall CVXD_W32_Proc1(DWORD dwDDB, DWORD hDevice, LPDIOC lpDIOCParms)
{
    PDWORD pdw;

    Out_Debug_String("CVXDSAMP: CVXD_W32_Proc1\n\r");

    pdw = (PDWORD)lpDIOCParms->lpvOutBuffer;
    hSysVM = Get_Sys_VM_Handle();
    pdw[0] = hSysVM;
    pdw[1] = Get_Execution_Focus();

    return(NO_ERROR);
}

DWORD _stdcall CVXD_W32_Proc2(DWORD dwDDB, DWORD hDevice, LPDIOC lpDIOCParms)
{
    PDWORD pdw;

    Out_Debug_String("CVXDSAMP: CVXD_W32_Proc2\n\r");

    pdw = (PDWORD)lpDIOCParms->lpvOutBuffer;
    *pdw = hSysVM;
    pdw[1] = VKD_Get_Kbd_Owner();

    return(NO_ERROR);
}

DWORD _stdcall CVXD_Dynamic_Exit(void)
{
    Out_Debug_String("CVXDSAMP: Dynamic Exit\n\r");

    return(VXD_SUCCESS);
}

DWORD _stdcall CVXD_CleanUp(void)
{
    Out_Debug_String("CVXDSAMP: Cleaning Up\n\r");
    return(VXD_SUCCESS);
}

/****************************************************************************
 *                CVXD_VMAPI
 *
 *    ENTRY: function - the function number (passed in eax)
 *        parm1 - parameter 1 (passed in ebx)
 *        parm2 - parameter 2 (passed in ecx)
 *
 *    EXIT:    NONE
 ***************************************************************************/
int _stdcall CVXD_VMAPI(unsigned int function,
                        unsigned int parm1,
                        unsigned int parm2)
{
    int retcode;

    Out_Debug_String("******* CVXD: V86 API Call\n\r");

    switch (function)
        {
        case CVXD_V86_FUNCTION1:
            retcode = V86Func1(parm1);
        break;

        case CVXD_V86_FUNCTION2:
            retcode = V86Func2(parm1, parm2);
        break;

        default:
            retcode = FALSE;
        break;
        }

    return (retcode);
}

/****************************************************************************
 *                CVXD_PMAPI
 *
 *    ENTRY: function - the function number (passed in eax)
 *        parm1 - parameter 1 (passed in ebx)
 *        parm2 - parameter 2 (passed in ecx)
 *
 *    EXIT:    NONE
 ***************************************************************************/
int _stdcall CVXD_PMAPI(unsigned int function,
                        unsigned int parm1,
                        unsigned int parm2)
{
    int retcode;

    switch (function)
        {
        case CVXD_PM_FUNCTION1:
            retcode = PMFunc1(parm1);
        break;

        case CVXD_PM_FUNCTION2:
            retcode = PMFunc2(parm1, parm2);
        break;

        default:
            retcode = 0;
        break;
        }

    return (retcode);
}


/****************************************************************************
 *                V86Func1
 *
 *    ENTRY: parm1 - sample parameter
 *
 *    EXIT: 1
 ***************************************************************************/
int V86Func1(unsigned int parm1)
{
    Out_Debug_String("******* CVXD: V86 API 2 Call\n\r");
    return (1);
}

/****************************************************************************
 *                V86Func2
 *
 *    ENTRY: parm1 - sample parameter1
 *           parm2 - sample parameter2
 *
 *    EXIT: 2
 ***************************************************************************/
int V86Func2(unsigned int parm1,
         unsigned int parm2)
{
    Out_Debug_String("******* CVXD: V86 API 2 Call\n\r");
    return (2);
}

/****************************************************************************
 *                PMFunc1
 *
 *    ENTRY: parm1 - sample parameter
 *
 *    EXIT: 1
 ***************************************************************************/
int PMFunc1(unsigned int parm1)
{
    Out_Debug_String("******* CVXD: PM API 1 Call\n\r");
    return (1);
}

/****************************************************************************
 *                PMFunc2
 *
 *    ENTRY: parm1 - sample parameter1
 *           parm2 - sample parameter2
 *
 *    EXIT: 2
 ***************************************************************************/
int PMFunc2(unsigned int parm1,
        unsigned int parm2)
{
    Out_Debug_String("******* CVXD: PM API 2 Call\n\r");
    return (2);
}

#pragma VxD_ICODE_SEG
#pragma VxD_IDATA_SEG

DWORD _stdcall CVXD_Dynamic_Init(void)
{
    Out_Debug_String("CVXDSAMP: Dynamic Init\n\r");

    return(VXD_SUCCESS);
}



