/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    monotest.c

Abstract:

    A user mode test app for the mono device driver.

Environment:

    User mode only

Revision History:

    03-22-93 : created

--*/

#include "windows.h"
#include "winioctl.h"
#include "stdio.h"
#include "stdlib.h"
#include "monopub.h"



int
__cdecl
main(
    IN int  argc,
    IN char *argv[]
    )
/*++

Routine Description:

    Tries to open the MONO driver & send it a couple of IOCTLs.

Arguments:

    argc - count of command line arguments

    argv - command line arguments

Return Value:


--*/
{

    HANDLE  hDriver;
    UCHAR   outputString[] = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n";
    DWORD   cbReturned;

    if ((hDriver = CreateFile("\\\\.\\MONO",
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              )) != ((HANDLE)-1))

        printf("\nRetrieved valid handle for MONO driver\n");


    else
    {
        printf("Can't get a handle to MONO driver\n");

        return 0;
    }

    if (DeviceIoControl (hDriver,
                         (DWORD) IOCTL_MONO_PRINT,
                         outputString,
                         sizeof(outputString),
                         NULL,
                         0,
                         &cbReturned,
                         0
                         ))
    {
        printf ("DeviceIoControl worked\n\n");

        printf ("Hit <Enter> to clear the mono display: \n");

        getchar ();

        DeviceIoControl (hDriver,
                         (DWORD) IOCTL_MONO_CLEAR_SCREEN,
                         NULL,
                         0,
                         NULL,
                         0,
                         &cbReturned,
                         0);

        printf ("'Bye\n");
    }
    else
    {
        printf ("DeviceIoControl failed\n");
    }

    CloseHandle(hDriver);

    return 1;
}
