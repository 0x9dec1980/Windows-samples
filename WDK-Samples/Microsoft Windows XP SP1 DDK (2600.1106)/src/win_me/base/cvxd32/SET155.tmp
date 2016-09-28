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
* PURPOSE: Simple console application for calling CVXDSAMP (C VxD Sample) VxD
*
* FUNCTIONS:
*  main() - Console application calls VMM and VKD through CVXDSAMP which
*           supports DeviceIoControl. CVXDSAMP will return values to this
*           application through this same DeviceIoControl interface.
*
* SPECIAL INSTRUCTIONS:
*
****************************************************************************/

#include <stdio.h>
#include <windows.h>
#include <vmm.h>
#include <vxdldr.h>

#define CVXD_APIFUNC_1 1
#define CVXD_APIFUNC_2 2

int main()
{
    HANDLE      hCVxD = 0;
    DWORD       cbBytesReturned;
    DWORD       dwErrorCode;
    DWORD       RetInfo[2];

    // Dynamically load and prepare to call CVXDSAMP
    // The CREATE_NEW flag is not necessary
    hCVxD = CreateFile("\\\\.\\CVXDSAMP.VXD", 0,0,0,
                        CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, 0);

    if ( hCVxD == INVALID_HANDLE_VALUE )
    {
        dwErrorCode = GetLastError();
        if ( dwErrorCode == ERROR_NOT_SUPPORTED )
        {
            printf("Unable to open VxD, \n device does not support DeviceIOCTL\n");
        }
        else
        {
            printf("Unable to open VxD, Error code: %lx\n", dwErrorCode);
        }
    }
    else
    {
        // Make 1st VxD call here
        if ( DeviceIoControl(hCVxD, CVXD_APIFUNC_1,
                (LPVOID)NULL, 0,
                (LPVOID)RetInfo, sizeof(RetInfo),
                &cbBytesReturned, NULL) )
        {
            printf("System VM Handle, VM w/ Execution Focus: %lx,%lx\n", RetInfo[0], RetInfo[1]);
        }
        else
        {
            printf("Device does not support the requested API\n");
        }

        // Make 2nd VxD call here
        if ( DeviceIoControl(hCVxD, CVXD_APIFUNC_2,
                (LPVOID)NULL, 0,
                (LPVOID)RetInfo, sizeof(RetInfo),
                &cbBytesReturned, NULL) )
        {
            printf("System VM Handle: %lx\n", RetInfo[0]);
            printf("VKD_Get_Kbd_Owner: %lx\n", RetInfo[1]);
        }
        else
        {
            printf("Device does not support the requested API\n");
        }

        // Dynamically UNLOAD the C Virtual Device sample.
        CloseHandle(hCVxD);
    }
    return(0);
}
