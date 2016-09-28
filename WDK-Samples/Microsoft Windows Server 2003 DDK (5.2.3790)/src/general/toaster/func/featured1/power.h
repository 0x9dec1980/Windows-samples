/*++
Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:

    power.h

Abstract:

    This module contains the declarations used by power.c

Author:

     Eliyas Yakub   Sep 16, 1998

Environment:

    user and kernel
Notes:


Revision History:


--*/

#ifndef __POWER_H
#define __POWER_H

typedef enum {

    IRP_NEEDS_FORWARDING = 1,
    IRP_ALREADY_FORWARDED

} IRP_DIRECTION;

typedef struct _POWER_COMPLETION_CONTEXT {

    PDEVICE_OBJECT  DeviceObject;
    PIRP            SIrp;
} POWER_COMPLETION_CONTEXT, *PPOWER_COMPLETION_CONTEXT;

typedef VOID (*PFN_QUEUE_SYNCHRONIZED_CALLBACK)(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction
    );

typedef struct _WORKER_THREAD_CONTEXT {

    PDEVICE_OBJECT                  DeviceObject;
    PIRP                            Irp;
    IRP_DIRECTION                   IrpDirection;
    PFN_QUEUE_SYNCHRONIZED_CALLBACK Callback;
    PIO_WORKITEM                    WorkItem;

} WORKER_THREAD_CONTEXT, *PWORKER_THREAD_CONTEXT;


NTSTATUS
ToasterDispatchPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterDispatchPowerDefault(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    );

NTSTATUS
ToasterDispatchSetPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterDispatchQueryPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterDispatchSystemPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterCompletionSystemPowerUp(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

VOID
ToasterQueueCorrespondingDeviceIrp(
    IN PIRP SIrp,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
ToasterCompletionOnFinalizedDeviceIrp(
    IN  PDEVICE_OBJECT              DeviceObject,
    IN  UCHAR                       MinorFunction,
    IN  POWER_STATE                 PowerState,
    IN  PVOID   PowerContext,
    IN  PIO_STATUS_BLOCK            IoStatus
    );

NTSTATUS
ToasterDispatchDeviceQueryPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ToasterDispatchDeviceSetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterCompletionDevicePowerUp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID NotUsed
    );

NTSTATUS
ToasterFinalizeDevicePowerIrp(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction,
    IN  NTSTATUS            Result
    );

NTSTATUS
ToasterQueuePassiveLevelPowerCallback(
    IN  PDEVICE_OBJECT                      DeviceObject,
    IN  PIRP                                Irp,
    IN  IRP_DIRECTION                       Direction,
    IN  PFN_QUEUE_SYNCHRONIZED_CALLBACK     Callback
    );

VOID
ToasterCallbackHandleDeviceQueryPower(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction
    );

VOID
ToasterCallbackHandleDeviceSetPower(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction
    );

VOID
ToasterQueuePassiveLevelPowerCallbackWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    );

NTSTATUS
ToasterPowerBeginQueuingIrps(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  ULONG               IrpIoCharges,
    IN  BOOLEAN             Query
    );

NTSTATUS
ToasterGetPowerPoliciesDeviceState(
    IN  PIRP                SIrp,
    IN  PDEVICE_OBJECT      DeviceObject,
    OUT DEVICE_POWER_STATE *DevicePowerState
    );

NTSTATUS
ToasterCanSuspendDevice(
    IN PDEVICE_OBJECT   DeviceObject
    );

PCHAR
DbgPowerMinorFunctionString(
    UCHAR MinorFunction
    );

PCHAR
DbgSystemPowerString(
    IN SYSTEM_POWER_STATE Type
    );

PCHAR
DbgDevicePowerString(
    IN DEVICE_POWER_STATE Type
    );


#endif

