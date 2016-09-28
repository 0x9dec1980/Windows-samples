/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

   ld.c

Abstract:

   This is a drive lock API demo application, LOCKDEMO.EXE.
   This module contains all of the main code for the console application.
   I/O to the console window goes through an abstract interface to make it
   possible to quickly change this to a Windows application in the future.

Author:

    original Redbook audio CD demo code by Len Smale

Environment:

   Kernel mode only


Revision History:
    4/19/2000 Modifications to add use of drive locking mechanisms by
	Kevin Burrows and documented by Mark Amos.
	Redbook-specific code stripped, tool now independent of disk format.
	File and function names changed to reflect new purpose. Thanks, Len!

--*/

#include <windows.h>
#include <stdio.h>
#include <cdioctl.h>
#include "ldif.h"
#include "ld.h"

//
// Variables global to this file
//
HANDLE ghDevice = INVALID_HANDLE_VALUE;

void main(int argc, char *argv[] )
{
  char             chDrive = '\0';
  PCSTR            szText = NULL;
  INT              i=0;
  CDROM_LOCK_DRIVE LockCommand = {0};
  WIN32_FIND_DATA  FindFileData = {0};
  PSTR             szFile = "X:\\*.*";
  HANDLE           hFind = NULL;
  DWORD            dwDrive = 0;
  CDROM_DEVSTAT    DevStat = {0};

  //
  // Load the VxD
  //
  if (!LoadVxD())
  {
    printf( "Unable to load "TESTVXDNAME"\n" );
    goto EXIT_PROGRAM;
  }

  //
  // Find a CDRom drive
  //
  dwDrive = GetCDRomDrive();
  if( dwDrive > MAX_DRIVE_NUMBER )
  {
    printf("Unable to find a CDRom drive on this system\n");
    goto EXIT_PROGRAM;
  }

  chDrive = 'A'+(BYTE)dwDrive;
  printf( "Drive %c will be used to demonstrate CDROM drive locking.\n",
			chDrive );

  //
  // Use FindFirstFile as an attempt to force the driver to update its status
  //
  szFile[0] = chDrive;
  hFind = FindFirstFile(szFile, &FindFileData);
  if( hFind != INVALID_HANDLE_VALUE )
    FindClose( hFind );

  //
  // Lock the drive
  //
  LockCommand.Volume = dwDrive;
  if( LockDrive(&LockCommand) )
  {
    printf( "Drive %c is now locked\n", chDrive );
  } else {
    printf( "Unable to lock drive\n" );
    goto EXIT_PROGRAM;
  }

  //
  // Query the status of the lock
  //
  printf( "Verify that drive %c is locked: ", chDrive );
  if( IsDriveLocked(&LockCommand) )
  {
    printf( "Verified!\n" );
  } else {
    printf( "Verification failed.\n" );
    goto EXIT_PROGRAM;
  }

  //
  // The lock expires after one minute of inactivity.
  //
  printf( "Reset the lock's expiration timer\n" );
  if( RefreshLock(&LockCommand) )
  {
    printf( "Lock timer is reset\n" );
  } else {
    printf( "Lock timer is not reset\n" );
    goto EXIT_PROGRAM;
  }

  //
  // Exercise the lock by sending a request to the driver
  //
  printf( "Get the status of drive %c\n", chDrive );
  GetDeviceStatus(dwDrive, &DevStat);
  do {
    szText = (DevStat.DeviceStatus & aDeviceProp[i].Mask) ?
              aDeviceProp[i].SetText : aDeviceProp[i].ClearText;
    if (szText) {
       printf("        ");
       printf(szText);
       printf("\n");
    }
  } while (aDeviceProp[++i].Mask != 0);

  //
  // Unlock the drive
  //
  LockCommand.Volume = dwDrive;
  if( UnlockDrive(&LockCommand) )
  {
    printf( "Drive %c is now unlocked\n", chDrive );
  } else {
    printf( "Unable to unlock drive\n" );
  }

  //
  // Query the status of the lock
  //
  printf( "Verify that drive %c is unlocked: ", chDrive );
  if( !IsDriveLocked(&LockCommand) )
  {
    printf( "Verified!\n" );
  } else {
    printf( "Verification failed.\n" );
    goto EXIT_PROGRAM;
  }

EXIT_PROGRAM:
  UnLoadVxD();
  return;
}

// Function: LoadVxD - attempt to load the desired VxD
BOOL LoadVxD()
{
  if( ghDevice == INVALID_HANDLE_VALUE )
  {
    // The VxD cannot be unloaded when the test application is done with
    // it because it registers with IOS and has no way to deregister.
    // Consequently it is loaded permamently here.
    //
    ghDevice = CreateFile("\\\\.\\" TESTVXDNAME,
                          0,
                          0,
                          NULL,
                          0,
                          FILE_FLAG_DELETE_ON_CLOSE,
                          NULL);

    if( GetVxDVersion() == 0 )
      ghDevice = INVALID_HANDLE_VALUE;
  }
  return ghDevice != INVALID_HANDLE_VALUE;
}

// Function: UnLoadVxD - Attempt to unload the previously loaded VxD
void UnLoadVxD()
{
 if( ghDevice != INVALID_HANDLE_VALUE )
 {
   CloseHandle( ghDevice );
   ghDevice = INVALID_HANDLE_VALUE;
 }
}

// Function: GetVxDVersion - attempt to discover the version of the VxD
DWORD GetVxDVersion()
{
  DWORD dwVersion = 0;
  DWORD nRet;

  if( ghDevice != INVALID_HANDLE_VALUE )
  {
    DeviceIoControl(ghDevice,
                    1,
                    NULL,
                    0,
                    &dwVersion,
                    sizeof(dwVersion),
                    &nRet,
                    NULL);
  }
  return dwVersion;
}

// Function: LockDrive - Perform a lock on the drive number passed using
//		standard device API call
BOOL LockDrive(PCDROM_LOCK_DRIVE pCommand)
{
  DWORD cbStatus = 0;
  DWORD dwStatus = 0;
  DWORD dwRet = 0;

  if( ghDevice != INVALID_HANDLE_VALUE )
  {
    dwRet = DeviceIoControl(ghDevice,
                            CDROM_IOCTL_LOCK_DRIVE,
                            pCommand,
                            sizeof(CDROM_LOCK_DRIVE),
                            &dwStatus,
                            sizeof(DWORD),
                            &cbStatus,
                            NULL);
  } else {
    SetLastError(ERROR_INVALID_DRIVE);
    dwRet = 0;
  }
  return dwRet ? TRUE : FALSE;
}

// Function: UnlockDrive - Perform an unlock on the drive number passed using
//		standard device API call
BOOL UnlockDrive(PCDROM_LOCK_DRIVE pCommand)
{
  DWORD cbStatus = 0;
  DWORD dwStatus = 0;
  DWORD dwRet = 0;

  if (ghDevice != INVALID_HANDLE_VALUE)
  {
    dwRet = DeviceIoControl(ghDevice,
                            CDROM_IOCTL_UNLOCK_DRIVE,
                            pCommand,
                            sizeof(CDROM_LOCK_DRIVE),
                            &dwStatus,
                            sizeof(DWORD),
                            &cbStatus,
                            NULL);
  } else {
    SetLastError(ERROR_INVALID_DRIVE);
    dwRet = 0;
  }
  return dwRet ? TRUE : FALSE;
}

// Function: IsDriveLocked - Perform a query on the lock status of the drive
//		number passed using standard device API call
BOOL IsDriveLocked(PCDROM_LOCK_DRIVE pCommand)
{
  DWORD cbStatus = 0;
  DWORD dwStatus = 0;
  DWORD dwRet = 0;

  if (ghDevice != INVALID_HANDLE_VALUE)
  {
    dwRet = DeviceIoControl(ghDevice,
                            CDROM_IOCTL_IS_DRIVE_LOCKED,
                            pCommand,
                            sizeof(CDROM_LOCK_DRIVE),
                            &dwStatus,
                            sizeof(DWORD),
                            &cbStatus,
                            NULL);
  } else {
    SetLastError(ERROR_INVALID_DRIVE);
    dwRet = 0;
  }
  return dwRet ? dwStatus : FALSE;
}

// Function: RefreshLock - Attempt to refresh the lock of the drive
//		number passed using standard device API call. Lock will time out if
//		lock is not either used or refreshed in a specified time period.
BOOL RefreshLock(PCDROM_LOCK_DRIVE pCommand)
{
  DWORD cbStatus = 0;
  DWORD dwStatus = 0;
  DWORD dwRet = 0;

  if (ghDevice != INVALID_HANDLE_VALUE)
  {
    dwRet = DeviceIoControl(ghDevice,
                            CDROM_IOCTL_REFRESH_LOCK,
                            pCommand,
                            sizeof(CDROM_LOCK_DRIVE),
                            &dwStatus,
                            sizeof(DWORD),
                            &cbStatus,
                            NULL);
  } else {
    SetLastError(ERROR_INVALID_DRIVE);
    dwRet = 0;
  }
  return dwRet ? TRUE : FALSE;
}

// Function: GetCDRomDrive - Attempt to locate a CDROM drive connected to system.
//		Stops at first drive located.
DWORD GetCDRomDrive( VOID )
{
  PSTR  szPath = "X:\\";
  DWORD dwDrive = 0;

  while( dwDrive <= MAX_DRIVE_NUMBER )
  {
    szPath[0] = 'A' + (BYTE)dwDrive;
    if( GetDriveType(szPath) == DRIVE_CDROM )
      break;//out of loop
    dwDrive++;
  }
  return dwDrive;
}

// Function: GetDeviceStatus - Optain the details of the current status of the
//		CDROM drive specified.
DWORD GetDeviceStatus(DWORD Volume, PCDROM_DEVSTAT pDevStat)
{
    DWORD               Status = NO_ERROR;
    CDROM_ISSUE_IOCTL   Ioctl;
    BOOL                bResult;
    DWORD               nret;

    Ioctl.Volume = Volume;
    Ioctl.IoctlCode = CDROM_IOCTL_GET_DEVICE_STATUS;
    pDevStat->Reserved = 0;
    if (ghDevice == INVALID_HANDLE_VALUE) {
	    Status = ERROR_INVALID_DRIVE;
	    SetLastError(Status);
    }
    else {
            bResult = DeviceIoControl(ghDevice,
				      CDROM_IOCTL_PROXY,
				      &Ioctl,
				      sizeof(Ioctl),
				      pDevStat,
				      sizeof(CDROM_DEVSTAT),
				      &nret,
				      NULL);
	    if (!bResult) {
	        Status = GetLastError();
	    }
    }
    return Status;
}
// EOF