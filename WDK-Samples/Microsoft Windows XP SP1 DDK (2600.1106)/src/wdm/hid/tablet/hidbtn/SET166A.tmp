/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        HidBtn.h

    Abstract:
        Contains definitions of all constants and data types for the
        serial pen hid driver.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Apr-2000

    Revision History:
--*/

#ifndef _HIDBTN_H
#define _HIDBTN_H

//
// Constants
//
#define HBTN_POOL_TAG                   'ntbH'

// dwfHBtn flag values
#define HBTNF_DEVICE_STARTED            0x00000001

//
// Macros
//
#define GET_MINIDRIVER_DEVICE_EXTENSION(DO) \
    ((PDEVICE_EXTENSION)(((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))
#define GET_NEXT_DEVICE_OBJECT(DO)          \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)
#define GET_PDO(DO)                         \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->PhysicalDeviceObject)
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//
// Type Definitions
//
typedef struct _DEVICE_EXTENSION
{
    ULONG          dwfHBtn;             //flags
    PDEVICE_OBJECT self;                //my device object
    PDEVICE_OBJECT LowerDevObj;         //lower device object
    IO_REMOVE_LOCK RemoveLock;          //to protect IRP_MN_REMOVE_DEVICE
    KSPIN_LOCK     SpinLock;
    LIST_ENTRY     PendingIrpList;
    OEM_EXTENSION  OemExtension;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Function prototypes
//

// hidbtn.c
NTSTATUS EXTERNAL
DriverEntry(
    IN PDRIVER_OBJECT  DrvObj,
    IN PUNICODE_STRING RegPath
    );

NTSTATUS EXTERNAL
HbtnCreateClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS EXTERNAL
HbtnAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT DevObj
    );

VOID EXTERNAL
HbtnUnload(
    IN PDRIVER_OBJECT DrvObj
    );

// pnp.c
NTSTATUS EXTERNAL
HbtnPnp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS EXTERNAL
HbtnPower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
SendSyncIrp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN BOOLEAN        fCopyToNext
    );

NTSTATUS INTERNAL
IrpCompletion(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PKEVENT        Event
    );

// ioctl.c
NTSTATUS EXTERNAL
HbtnInternalIoctl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
GetDeviceDescriptor(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS INTERNAL
GetReportDescriptor(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS INTERNAL
ReadReport(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

VOID EXTERNAL
ReadReportCanceled(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );

NTSTATUS INTERNAL
GetString(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS INTERNAL
GetAttributes(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

// oembtn.c
NTSTATUS INTERNAL
OemAddDevice(
    IN PDEVICE_EXTENSION devext
    );

NTSTATUS INTERNAL
OemStartDevice(
    IN PDEVICE_EXTENSION devext,
    IN PIRP              Irp
    );

NTSTATUS INTERNAL
OemRemoveDevice(
    IN PDEVICE_EXTENSION devext,
    IN PIRP              Irp
    );

NTSTATUS INTERNAL
OemWriteReport(
    IN PDEVICE_EXTENSION devext,
    IN PIRP Irp
    );

#endif  //ifndef _HIDBTN_H
