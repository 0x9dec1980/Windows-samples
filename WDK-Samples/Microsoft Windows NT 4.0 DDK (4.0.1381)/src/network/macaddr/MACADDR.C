/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    macaddr.c

Abstract:

    This is a Win32 program used to access a MAC driver to query stats.

Author:

    Robert Nelson (RobertN) 22-Sep-1993.

Environment:

    Win32 user mode.

Notes:

Revision History:

--*/


//
// Include files
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winioctl.h>
#include "ntddndis.h"        // This defines the IOCTL constants.

#define DEVICE_PREFIX   "\\\\.\\"

int
__cdecl
main(
    int argc,
    char ** argv
    )
{
    
    UCHAR       LinkName[512];
    UCHAR       DeviceName[80];
    UCHAR       szMACFileName[80];
    UCHAR       OidData[4096];
    NDIS_OID    OidCode;
    BOOLEAN     bCreatedDevice;
    DWORD       ErrorNumber;
    DWORD       ReturnedCount;
    HANDLE      hMAC;

    //
    // Check to make sure we got the right number of arguments before we
    // create any devices.
    //

    if (argc != 2)
    {
        printf("usage: macdmp <device>\n");
        return(1);
    }

    //
    // Check to see if the DOS name for the MAC driver already exists.
    // Its not created automatically in version 3.1 but may be later.
    //

    if (QueryDosDevice(argv[1], LinkName, sizeof(LinkName)) == 0)
    {
        if ((ErrorNumber = GetLastError()) == ERROR_FILE_NOT_FOUND)
        {
            strcpy(DeviceName, "\\Device\\");
            strcat(DeviceName, argv[1]);

            //
            // It doesn't exist so create it.
            //
            if (!DefineDosDevice( DDD_RAW_TARGET_PATH, argv[1], DeviceName))
            {
                printf(
                    "DefineDosDevice returned an error creating the device = %d\n",
                    GetLastError()
                    );
                return(1);
            }
            bCreatedDevice = TRUE;
        }
        else
        {
            printf("QueryDosDevice returned an error = %d\n", GetLastError());
            return(1);
        }
    }
    else
    {
        bCreatedDevice = FALSE;
    }

    //
    // Construct a device name to pass to CreateFile
    //
    strcpy(szMACFileName, DEVICE_PREFIX);
    strcat(szMACFileName, argv[1]);

    hMAC = CreateFile(
                szMACFileName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                INVALID_HANDLE_VALUE
                );

    if (hMAC != INVALID_HANDLE_VALUE)
    {
        //
        // We successfully opened the driver, format the IOCTL to pass the
        // driver.
        //

        OidCode = OID_802_3_CURRENT_ADDRESS;

        if (DeviceIoControl(
                hMAC,
                IOCTL_NDIS_QUERY_GLOBAL_STATS,
                &OidCode,
                sizeof(OidCode),
                OidData,
                sizeof(OidData),
                &ReturnedCount,
                NULL
                ))
        {
            if (ReturnedCount == 6)
            {
                printf(
                    "Mac address = %02.2X-%02.2X-%02.2X-%02.2X-%02.2X-%02.2X\n",
                    OidData[0], OidData[1], OidData[2], OidData[3],
                    OidData[4], OidData[5], OidData[6], OidData[7]
                    );
            }
            else
            {
                printf(
                    "DeviceIoControl returned an invalid count = %d\n",
                    ReturnedCount
                    );
            }
        }
        else
        {
            printf("DeviceIoControl returned an error = %d\n", GetLastError());
        }
    }
    else
    {
        printf("CreateFile returned an error = %d\n", GetLastError());
    }

    if (bCreatedDevice)
    {
        //
        // The MAC driver wasn't visible in the Win32 name space so we created
        // a link.  Now we have to delete it.
        //
        if (!DefineDosDevice(
                DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION |
                    DDD_EXACT_MATCH_ON_REMOVE,
                argv[1],
                DeviceName)
                )
        {
            printf(
                "DefineDosDevice returned an error removing the device = %d\n",
                GetLastError()
                );
            return(1);
        }
    }
    
    return(0);
}
