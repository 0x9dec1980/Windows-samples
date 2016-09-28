/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    zwcreate.h

Abstract:

    This file includes data declarations for the ZwCreateFile sample driver.

Author:

    Paul Sanders Jan 28, 1995.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/


#define NT_DEVICE_NAME      L"\\Device\\ZwCF"
#define DOS_DEVICE_NAME     L"\\DosDevices\\ZwTest"
#define DEFAULT_FILE_NAME	L"info.dat"

#if DBG
#define ZWCFBUGCHECK            ((ULONG)0x80000000)
#define ZWCFDIAG1               ((ULONG)0x00000001)
#define ZWCFDIAG2               ((ULONG)0x00000002)
#define ZWCFERRORS              ((ULONG)0x00000004)

#define ZwCFDump(LEVEL, STRING) \
        do { \
            if (ZwCFDebugLevel & LEVEL) { \
                DbgPrint STRING; \
            } \
            if (LEVEL == ZWCFBUGCHECK) { \
                ASSERT(FALSE); \
            } \
        } while (0)
#else
#define ZwCFDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif

int
sprintf(char *s, const char *format, ...);

//
// Device driver routine declarations.
//

NTSTATUS
DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    );

NTSTATUS
ZwCFInitialize(
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      ParamPath
    );

NTSTATUS
ZwCFCreateClose(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
ZwCFDeviceControl(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

VOID
ZwCFUnloadDriver(
    IN PDRIVER_OBJECT       DriverObject
    );

NTSTATUS
ZwCFOpenAndReadFile(
	IN PUNICODE_STRING		fileName
	);
