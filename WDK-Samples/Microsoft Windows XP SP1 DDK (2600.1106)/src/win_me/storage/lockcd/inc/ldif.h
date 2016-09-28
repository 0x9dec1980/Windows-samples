/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

   ldif.h

Abstract:

   This include file contains the interface definitions for the IOCTLs issued to
   the VxD, RBVXD.VXD, that provides the interface to CDVSD.VXD 
   These interfaces are necessary because the IOCTL interfaces to CDVSD.VXD 
   do not have a user mode equivalent.

Author:

    original Redbook audio CD code by Len Smale

Environment:

Revision History:
    4/19/2000 Modifications to add use of drive locking mechanisms by 
	Kevin Burrows and Mark Amos. File and variable names changed to reflect 
	new purpose

--*/

#define CDROM_IOCTL_READ_DA         0xCD20
#define CDROM_IOCTL_PROXY           0xCD21
#define CDROM_IOCTL_LOCK_DRIVE      0xCD22
#define CDROM_IOCTL_UNLOCK_DRIVE    0xCD23
#define CDROM_IOCTL_IS_DRIVE_LOCKED 0xCD24
#define CDROM_IOCTL_REFRESH_LOCK    0xCD25

#define DA_SIZE 2352

typedef struct  CDROM_READ_DA {
    DWORD       Volume;
    DWORD       Sector;
} CDROM_READ_DA, *PCDROM_READ_DA;

typedef struct  CDROM_LOCK_DRIVE {
    DWORD       Volume;
} CDROM_LOCK_DRIVE, *PCDROM_LOCK_DRIVE;

typedef struct  CDROM_ISSUE_IOCTL {
    DWORD       Volume;
    DWORD       IoctlCode;
} CDROM_ISSUE_IOCTL, *PCDROM_ISSUE_IOCTL;


