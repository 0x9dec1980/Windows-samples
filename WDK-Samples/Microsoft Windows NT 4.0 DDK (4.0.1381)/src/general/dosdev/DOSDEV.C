/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dosdev.c

Abstract:

    A user mode test app that lists all the DOS device names,
     or connects to a device and sends it an IOCTL.

Environment:

    User mode only

Revision History:

    03-03-95 : Glued together from pre-existing pieces.

--*/



#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <winioctl.h>
#include <ntddkbd.h>
#include <ntddmou.h>


//
// function prototypes.
//

void q_DosDev();
int  kbd_attr();
int  mou_attr();
void vPrintUsage();





int __cdecl main(int argc, char *argv[])
{

  //
  //  Skip the name of the executable, and process cmd line arguments.
  //

  argv++;

  if (argv[0]) {


    if ((*argv)[0] == '/' || (*argv)[0] == '-') {

      switch (tolower((*argv)[1])) {

        case 'q':
          q_DosDev();
          return TRUE;
        break;

        case 'k':
          kbd_attr();
          return TRUE;
        break;

        case 'm':
          mou_attr();
          return TRUE;
        break;

        case 'h':
        case '?':
        default:
          vPrintUsage();
          return TRUE;
      }
    }
  }

  vPrintUsage();

  return(0);
}



void vPrintUsage(void)
{
  printf("Usage:   dosdev /q | /k | /m | /? \n");
  printf("  /q  QueryDosDevice()\n");
  printf("  /k  DefineDosDevice(\\Device\\KeyboardClass0) & IOCTL_KEYBOARD_QUERY_ATTRIBUTES\n");
  printf("  /m  DefineDosDevice(\\Device\\PointerClass0) & IOCTL_MOUSE_QUERY_ATTRIBUTES\n");
  printf("  /?  Usage Information. \n");
  return;
}





void q_DosDev(void)
/*++

Routine Description:

    Call the QueryDosDevice() API repeatedly to list all of the
      pre-defined MS-DOS device names.

--*/
{
    char   buf[2048], buf2[2048], *p, *q;

    QueryDosDevice (NULL, buf, 2048);

    p = buf;

    while (*p)
    {
        if (strlen(p) > 7)

          printf (p);

        else

          printf ("%s\t", p);

        QueryDosDevice (p, buf2, 2048);

        q = buf2;

        while (*q)
        {
            printf ("\t%s\n", q);
            q += strlen(q) + 1;
        }

        p += strlen(p) + 1;
    }

    return;
}





int kbd_attr(void)
/*++

Routine Description:

    Creates a symbolic link to \Device\KeyboardPort0, opens the
    device, and sends it an i/o request.

Return Value:

    1 if success
    0 if an error occurs

--*/
{

    int     rc = 1;  // assume success
    HANDLE  hDriver;
    DWORD   cbReturned;
    char    deviceName[] = "KBD";
    char    targetPath[] = "\\Device\\KeyboardClass0";
    char    completeDeviceName[] = "\\\\.\\KBD";

    KEYBOARD_ATTRIBUTES kbdattrs;

    //
    // The kuid=0 here corresponds to the keyboard port device # 0
    // (e.g. \Device\KeyboardPort). We go through the keyboard
    // class device # 0 to get to the kbd port device # 0.
    //

    KEYBOARD_UNIT_ID_PARAMETER kuid = { 0 };


    //
    // First create a symbolic link to the NT keyboard object (number 0-
    // there may be more than 1)
    //

    if (DefineDosDevice (DDD_RAW_TARGET_PATH,
                         deviceName,
                         targetPath
                         ))
    {
        printf ("\nDefineDosDevice (%s, %s) worked\n",
                deviceName,
                targetPath
                );
    }
    else
    {
        printf ("\nDefineDosDevice (%s, %s) failed\n",
                deviceName,
                targetPath
                );

        return 0;
    }



    //
    // Next, try to open the device
    //

    if ((hDriver = CreateFile (completeDeviceName,
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               )) != ((HANDLE)-1))

        printf ("\nRetrieved valid handle for %s\n", deviceName);


    else
    {
        printf ("Can't get a handle to %s\n", deviceName);

        rc = 0;

        goto delete_dos_device;
    }



    //
    // Send it a request
    //

    if (DeviceIoControl (hDriver,
                         (DWORD) IOCTL_KEYBOARD_QUERY_ATTRIBUTES,
                         &kuid,
                         sizeof(KEYBOARD_UNIT_ID_PARAMETER),
                         &kbdattrs,
                         sizeof(KEYBOARD_ATTRIBUTES),
                         &cbReturned,
                         0
                         ))
    {
        printf ("DeviceIoControl worked\n\n");

        printf ("\tkbd_attr.KeyboardIdentifier.Type = %d\n",
                (int) kbdattrs.KeyboardIdentifier.Type
                );

        printf ("\tkbd_attr.KeyboardIdentifier.Subtype = %d\n",
                (int) kbdattrs.KeyboardIdentifier.Subtype
                );

        printf ("\tkbd_attr.KeyboardMode = %d\n",
                (int) kbdattrs.KeyboardMode
                );

        printf ("\tkbd_attr.NumberOfFunctionKeys = %d\n",
                (int) kbdattrs.NumberOfFunctionKeys
                );

        printf ("\tkbd_attr.NumberOfIndicators = %d\n",
                (int) kbdattrs.NumberOfIndicators
                );

        printf ("\tkbd_attr.NumberOfKeysTotal = %d\n",
                (int) kbdattrs.NumberOfKeysTotal
                );

        printf ("\tkbd_attr.InputDataQueueLength = %d\n",
                (int) kbdattrs.InputDataQueueLength
                );

        printf ("\tkbd_attr.KeyRepeatMinimum.UnitId = %d\n",
                (int) kbdattrs.KeyRepeatMinimum.UnitId
                );

        printf ("\tkbd_attr.KeyRepeatMinimum.Rate = %d\n",
                (int) kbdattrs.KeyRepeatMinimum.Rate
                );

        printf ("\tkbd_attr.KeyRepeatMinimum.Delay = %d\n",
                (int) kbdattrs.KeyRepeatMinimum.Delay
                );

        printf ("\tkbd_attr.KeyRepeatMaximum.UnitId = %d\n",
                (int) kbdattrs.KeyRepeatMaximum.UnitId
                );

        printf ("\tkbd_attr.KeyRepeatMaximum.Rate = %d\n",
                (int) kbdattrs.KeyRepeatMaximum.Rate
                );

        printf ("\tkbd_attr.KeyRepeatMaximum.Delay = %d\n",
                (int) kbdattrs.KeyRepeatMaximum.Delay
                );
    }
    else
    {
        DWORD err = GetLastError();

        printf ("DeviceIoControl failed, err = %d\n(%x)\n",
                err,
                err
                );

        rc = 0;
    }



    //
    // Clean up
    //

    CloseHandle(hDriver);

delete_dos_device:

    DefineDosDevice (DDD_REMOVE_DEFINITION,
                     deviceName,
                     NULL
                     );


    return rc;
}



int mou_attr(void)
/*++

Routine Description:

    Creates a symbolic link to \Device\MousePort0, opens the
    device, and sends it an i/o request.

Return Value:

    1 if success
    0 if an error occurs

--*/
{

    int     rc = 1;  // assume success
    HANDLE  hDriver;
    DWORD   cbReturned;
    char    deviceName[] = "MOU";
    char    targetPath[] = "\\Device\\PointerClass0";
    char    completeDeviceName[] = "\\\\.\\MOU";

    MOUSE_ATTRIBUTES mouattrs;


    MOUSE_UNIT_ID_PARAMETER muid = { 0 };


    //
    // First create a symbolic link to the NT keyboard object (number 0-
    // there may be more than 1)
    //

    if (DefineDosDevice (DDD_RAW_TARGET_PATH,
                         deviceName,
                         targetPath
                         ))
    {
        printf ("\nDefineDosDevice (%s, %s) worked\n",
                deviceName,
                targetPath
                );
    }
    else
    {
        printf ("\nDefineDosDevice (%s, %s) failed\n",
                deviceName,
                targetPath
                );

        return 0;
    }



    //
    // Next, try to open the device
    //

    if ((hDriver = CreateFile (completeDeviceName,
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               )) != ((HANDLE)-1))

        printf ("\nRetrieved valid handle for %s\n", deviceName);

    else
    {
        printf ("Can't get a handle to %s\n", deviceName);

        rc = 0;

        goto delete_dos_device;
    }



    //
    // Send it a request
    //

    if (DeviceIoControl (hDriver,
                         (DWORD) IOCTL_MOUSE_QUERY_ATTRIBUTES,
                         &muid,
                         sizeof(MOUSE_UNIT_ID_PARAMETER),
                         &mouattrs,
                         sizeof(MOUSE_ATTRIBUTES),
                         &cbReturned,
                         0
                         ))
    {
        printf ("DeviceIoControl worked\n\n");

        printf ("\tmou_attr.MouseIdentifier = %d\n",
                (int) mouattrs.MouseIdentifier
                );

        printf ("\tmou_attr.NumberOfButtons = %d\n",
                (int) mouattrs.NumberOfButtons
                );

        printf ("\tmou_attr.SampleRate = %d\n",
                (int) mouattrs.SampleRate
                );

        printf ("\tmou_attr.InputDataQueueLength = %d\n",
                (int) mouattrs.InputDataQueueLength
                );


    }
    else
    {
        DWORD err = GetLastError();

        printf ("DeviceIoControl failed, err = %d\n(%x)\n",
                err,
                err
                );

        rc = 0;
    }



    //
    // Clean up
    //

    CloseHandle(hDriver);

delete_dos_device:

    DefineDosDevice (DDD_REMOVE_DEFINITION,
                     deviceName,
                     NULL
                     );


    return rc;
}
