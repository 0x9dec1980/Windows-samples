/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    reparse.h

Abstract:

    This file includes data declarations for the Reparse sample
    driver for Windows NT 3.5.

Author:

    Paul Sanders May 4, 1995.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#define NT_DEVICE_NAME      L"\\Device\\Reparse"
#define NT_NEW_DEVICE_NAME  L"\\Device\\Reparse1"
#define DOS_DEVICE_NAME     L"\\DosDevices\\RP"

#if DBG
#define REPARSEBUGCHECK            ((ULONG)0x80000000)
#define REPARSEDIAG1               ((ULONG)0x00000001)
#define REPARSEDIAG2               ((ULONG)0x00000002)
#define REPARSEERRORS              ((ULONG)0x00000004)

#define ReparseDump(LEVEL, STRING) \
        do { \
            if (ReparseDebugLevel & LEVEL) { \
                DbgPrint STRING; \
            } \
            if (LEVEL == REPARSEBUGCHECK) { \
                ASSERT(FALSE); \
            } \
        } while (0)
#else
#define ReparseDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif

//
// Define a device extension
//

typedef struct _LOCAL_DEVICE_EXTENSION {
    PDEVICE_OBJECT  SlaveDeviceObject;     //  The device object for this card
    PVOID           Pool;               // pool containing slave device name
    PDEVICE_OBJECT  MasterDeviceObject; //  \Device\Reparse
}LOCAL_DEVICE_EXTENSION, *PLOCAL_DEVICE_EXTENSION;
        
        
//
// Device driver routine declarations.
//

NTSTATUS
DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    );

NTSTATUS
ReparseInitialize(
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      ParamPath
    );

NTSTATUS
ReparseClose(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
ReparseCreate(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
ReparseDeviceControl(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

VOID
ReparseUnloadDriver(
    IN PDRIVER_OBJECT       DriverObject
    );

