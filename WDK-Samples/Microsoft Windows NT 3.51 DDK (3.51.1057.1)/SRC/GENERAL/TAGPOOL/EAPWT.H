/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    eapwt.h

Abstract:

    This file includes data declarations for the ExAllocatePoolWithTag sample
    driver for Windows NT 3.5.

Author:

    Paul Sanders Apr 18, 1995.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "eapioctl.h"

#define NT_DEVICE_NAME      L"\\Device\\EAPWT"
#define DOS_DEVICE_NAME     L"\\DosDevices\\TagTest"

#if DBG
#define EAPWTBUGCHECK            ((ULONG)0x80000000)
#define EAPWTDIAG1               ((ULONG)0x00000001)
#define EAPWTDIAG2               ((ULONG)0x00000002)
#define EAPWTERRORS              ((ULONG)0x00000004)

#define EAPWTDump(LEVEL, STRING) \
        do { \
            if (EAPWTDebugLevel & LEVEL) { \
                DbgPrint STRING; \
            } \
            if (LEVEL == EAPWTBUGCHECK) { \
                ASSERT(FALSE); \
            } \
        } while (0)
#else
#define EAPWTDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif

int
sprintf(char *s, const char *format, ...);

//
// Define the allocate pool structure.
//

typedef struct _ALLOCATE_POOL {
    PCHAR Address;
    ULONG Tag;
}ALLOCATE_POOL, *PALLOCATE_POOL;


//
// Device driver routine declarations.
//

NTSTATUS
DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    );

NTSTATUS
EAPWTInitialize(
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      ParamPath
    );

NTSTATUS
EAPWTCreateClose(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
EAPWTDeviceControl(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

VOID
EAPWTUnloadDriver(
    IN PDRIVER_OBJECT       DriverObject
    );
