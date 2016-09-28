/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Instdrv.c

Abstract:

    A simple Win32 app that installs a device driver

Environment:

    user mode only

Notes:

    See readme.txt

Revision History:

    06-25-93 : created
--*/



#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



BOOL
InstallDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName,
    IN LPCTSTR    ServiceExe
    );

BOOL
RemoveDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    );

BOOL
StartDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    );

BOOL
StopDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    );

BOOL
OpenDevice(
    IN LPCTSTR    DriverName
    );



VOID
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    SC_HANDLE   schSCManager;

    if (argc != 3)
    {
        char currentDirectory[128];

        printf ("usage: instdrv <driver name> <.sys location>\n");
        printf ("           to install a kernel-mode device driver, or:\n");
        printf ("       instdrv <driver name> remove\n");
        printf ("           to remove a kernel-mode device driver\n\n");

        GetCurrentDirectory (128,
                             currentDirectory
                             );

        printf ("       Example: instdrv simpldrv %s\\obj\\i386\\simpldrv.sys\n",
                currentDirectory
                );

        exit (1);
    }

    schSCManager = OpenSCManager (NULL,                 // machine (NULL == local)
                                  NULL,                 // database (NULL == default)
                                  SC_MANAGER_ALL_ACCESS // access required
                                  );

    if (!_stricmp (argv[2],
                  "remove"
                  ))
    {
        StopDriver (schSCManager,
                    argv[1]
                    );

        RemoveDriver (schSCManager,
                      argv[1]
                      );
    }
    else
    {
        InstallDriver (schSCManager,
                       argv[1],
                       argv[2]
                       );

        StartDriver (schSCManager,
                     argv[1]
                     );

        OpenDevice (argv[1]);

    }

    CloseServiceHandle (schSCManager);
}



BOOL
InstallDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName,
    IN LPCTSTR    ServiceExe
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    SC_HANDLE  schService;
    DWORD      err;



    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //

    schService = CreateService (SchSCManager,          // SCManager database
                                DriverName,           // name of service
                                DriverName,           // name to display
                                SERVICE_ALL_ACCESS,    // desired access
                                SERVICE_KERNEL_DRIVER, // service type
                                SERVICE_DEMAND_START,  // start type
                                SERVICE_ERROR_NORMAL,  // error control type
                                ServiceExe,            // service's binary
                                NULL,                  // no load ordering group
                                NULL,                  // no tag identifier
                                NULL,                  // no dependencies
                                NULL,                  // LocalSystem account
                                NULL                   // no password
                                );

    if (schService == NULL)
    {
        err = GetLastError();

        if (err == ERROR_SERVICE_EXISTS)
        {
            //
            // A common cause of failure (easier to read than an error code)
            //

            printf ("failure: CreateService, ERROR_SERVICE_EXISTS\n");
        }
        else
        {
            printf ("failure: CreateService (0x%02x)\n",
                    err
                    );
        }

        return FALSE;
    }
    else
    {
        printf ("CreateService SUCCESS\n");
    }

    CloseServiceHandle (schService);

    return TRUE;
}



BOOL
RemoveDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    SC_HANDLE  schService;
    BOOL       ret;

    schService = OpenService (SchSCManager,
                              DriverName,
                              SERVICE_ALL_ACCESS
                              );

    if (schService == NULL)
    {
        printf ("failure: OpenService (0x%02x)\n", GetLastError());
        return FALSE;
    }

    ret = DeleteService (schService);

    if (ret)
    {
        printf ("DeleteService SUCCESS\n");
    }
    else
    {
        printf ("failure: DeleteService (0x%02x)\n",
                GetLastError()
                );
    }

    CloseServiceHandle (schService);

    return ret;
}



BOOL
StartDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    )
{
    SC_HANDLE  schService;
    BOOL       ret;
    DWORD      err;

    schService = OpenService (SchSCManager,
                              DriverName,
                              SERVICE_ALL_ACCESS
                              );

    if (schService == NULL)
    {
        printf ("failure: OpenService (0x%02x)\n", GetLastError());
        return FALSE;
    }

    ret = StartService (schService,    // service identifier
                        0,             // number of arguments
                        NULL           // pointer to arguments
                        );
    if (ret)
    {
        printf ("StartService SUCCESS\n");
    }
    else
    {
        err = GetLastError();

        if (err == ERROR_SERVICE_ALREADY_RUNNING)
        {
            //
            // A common cause of failure (easier to read than an error code)
            //

            printf ("failure: StartService, ERROR_SERVICE_ALREADY_RUNNING\n");
        }
        else
        {
            printf ("failure: StartService (0x%02x)\n",
                    err
                    );
        }
    }

    CloseServiceHandle (schService);

    return ret;
}



BOOL
StopDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    )
{
    SC_HANDLE       schService;
    BOOL            ret;
    SERVICE_STATUS  serviceStatus;

    schService = OpenService (SchSCManager,
                              DriverName,
                              SERVICE_ALL_ACCESS
                              );

    if (schService == NULL)
    {
        printf ("failure: OpenService (0x%02x)\n", GetLastError());
        return FALSE;
    }

    ret = ControlService (schService,
                          SERVICE_CONTROL_STOP,
                          &serviceStatus
                          );
    if (ret)
    {
        printf ("ControlService SUCCESS\n");
    }
    else
    {
        printf ("failure: ControlService (0x%02x)\n",
                GetLastError()
                );
    }

    CloseServiceHandle (schService);

    return ret;
}



BOOL
OpenDevice(
    IN LPCTSTR    DriverName
    )
{
    char     completeDeviceName[64] = "";
    LPCTSTR  dosDeviceName = DriverName;
    HANDLE   hDevice;
    BOOL     ret;



    //
    // Create a \\.\XXX device name that CreateFile can use
    //
    // NOTE: We're making an assumption here that the driver
    //       has created a symbolic link using it's own name
    //       (i.e. if the driver has the name "XXX" we assume
    //       that it used IoCreateSymbolicLink to create a
    //       symbolic link "\DosDevices\XXX". Usually, there
    //       is this understanding between related apps/drivers.
    //
    //       An application might also peruse the DEVICEMAP
    //       section of the registry, or use the QueryDosDevice
    //       API to enumerate the existing symbolic links in the
    //       system.
    //

    strcat (completeDeviceName,
            "\\\\.\\"
            );

    strcat (completeDeviceName,
            dosDeviceName
            );

    hDevice = CreateFile (completeDeviceName,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                          );

    if (hDevice == ((HANDLE)-1))
    {
        printf ("Can't get a handle to %s\n",
                completeDeviceName
                );

        ret = FALSE;
    }
    else
    {
        printf ("CreateFile SUCCESS\n");

        CloseHandle (hDevice);

        ret = TRUE;
    }

    return ret;
}
