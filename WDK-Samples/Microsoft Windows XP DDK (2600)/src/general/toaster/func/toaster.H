
/*++

Copyright (c) 1990-2000 Microsoft Corporation All Rights Reserved

Module Name:

    Toaster.h

Abstract:

    Header file for the toaster driver modules.

Environment:

    Kernel mode

Revision History:

    Eliyas Yakub Oct 6, 1998

--*/


#if !defined(_TOASTER_H_)
#define _TOASTER_H_

#include "stdarg.h"
#include "stdio.h"
#include "stddef.h"

#include <ntddk.h> //wdm.h>
#include "wmilib.h"

#define TOASTER_POOL_TAG (ULONG) 'saoT'


#if DBG
#define ToasterDebugPrint(_x_) \
               DriverDbgPrint _x_;
VOID
DriverDbgPrint    (
    ULONG   DebugPrintLevel,
    PCCHAR  DebugMessage,
    ...
    );

#define TRAP() DbgBreakPoint()

#else
#define ToasterDebugPrint(_x_)
#define TRAP()

#endif


typedef struct _GLOBALS {

    //
    // Path to the driver's Services Key in the registry
    //

    UNICODE_STRING RegistryPath;

} GLOBALS;

extern GLOBALS Globals;

//
// Connector Types
//

#define TOASTER_WMI_STD_I8042 0
#define TOASTER_WMI_STD_SERIAL 1
#define TOASTER_WMI_STD_PARALEL 2
#define TOASTER_WMI_STD_USB 3

typedef struct _TOASTER_WMI_STD_DATA {
    //
    // Hardware Types
    //

    UINT32   ConnectorType;

    //
    // Current Temperature
    //

    UINT32   Capacity;

    //
    // The error Count
    //
    UINT32   ErrorCount;

    //
    // Number of controls on the toaster device
    //
    UINT32   Controls;

    //
    // Debug Print Level
    //

    UINT32  DebugPrintLevel;

} TOASTER_WMI_STD_DATA, * PTOASTER_WMI_STD_DATA;

//
// These are the states FDO transition to upon
// receiving a specific PnP Irp. Refer to the PnP Device States
// diagram in DDK documentation for better understanding.
//

typedef enum _DEVICE_PNP_STATE {

    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovePending,          // Device has received the QUERY_REMOVE IRP
    SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
    Deleted                 // Device has received the REMOVE_DEVICE IRP

} DEVICE_PNP_STATE;

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_)->DevicePnPState =  NotStarted;\
        (_Data_)->PreviousPnPState = NotStarted;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_)->PreviousPnPState =  (_Data_)->DevicePnPState;\
        (_Data_)->DevicePnPState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_)->DevicePnPState =   (_Data_)->PreviousPnPState;\

typedef enum _QUEUE_STATE {

    HoldRequests = 0,        // Device is not started yet, temporarily
                            // stopped for resource rebalancing, or
                            // entering a sleep state.
    AllowRequests,         // Device is ready to process pending requests
                            // and take in new requests.
    FailRequests             // Fail both existing and queued up requests.

} QUEUE_STATE;

//
// The device extension for the device object
//
typedef struct _FDO_DATA
{
    PDEVICE_OBJECT      Self; // a back pointer to the DeviceObject.

    PDEVICE_OBJECT      UnderlyingPDO;// The underlying PDO

    PDEVICE_OBJECT      NextLowerDriver; // The top of the device stack just
                                         // beneath this device object.
    DEVICE_PNP_STATE    DevicePnPState;   // Track the state of the device

    DEVICE_PNP_STATE    PreviousPnPState; // Remembers the previous pnp state

    QUEUE_STATE         QueueState;      // This flag is set whenever the
                                         // device needs to queue incoming
                                         // requests (when it receives a
                                         // QUERY_STOP or QUERY_REMOVE).

    LIST_ENTRY          NewRequestsQueue; // The queue where the incoming
                                          // requests are held when
                                          // QueueState is set to HoldRequest
                                          // or the device is busy.
    KSPIN_LOCK          QueueLock;        // The spin lock that protects
                                          // the queue

    BOOLEAN             DontDisplayInUI; // Current state of the device
                                          // as reported by the user.

    BOOLEAN             WaitWake;
    BOOLEAN             PowerDownEnable;  // These two variables track the
                                          // current state of checkbox in the
                                          // power management tab. TRUE means
                                          // the box is checked. Initial value of
                                          // these variable should be read from
                                          // the device registry.
    CHAR                Filler[1];


    KEVENT              RemoveEvent; // an event to sync outstandingIO to zero.

    KEVENT              StopEvent;  // an event to sync outstandingIO to 1.

    ULONG               OutstandingIO; // 1-biased count of reasons why
                                       // this object should stick around.

    SYSTEM_POWER_STATE  SystemPowerState;   // The general system power state

    DEVICE_POWER_STATE  DevicePowerState; // The state of the device(D0 or D3)

    UNICODE_STRING      InterfaceName; // The name returned from
                                       // IoRegisterDeviceInterface,
    DEVICE_CAPABILITIES DeviceCaps;   // Copy of the device capability
                                       // Used to find S to D mappings
    WMILIB_CONTEXT      WmiLibInfo;     // WMI Information

    TOASTER_WMI_STD_DATA   StdDeviceData;

}  FDO_DATA, *PFDO_DATA;

#define CLRMASK(x, mask)     ((x) &= ~(mask));
#define SETMASK(x, mask)     ((x) |=  (mask));

#if DBG

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
);

#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


NTSTATUS
ToasterAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


NTSTATUS
ToasterDispatchPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterDispatchPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
ToasterDispatchIoctl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ToasterCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterReadWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterCleanup (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
ToasterStartDevice (
    IN PFDO_DATA     FdoData,
    IN PIRP             Irp
    );



NTSTATUS
ToasterDispatchPnpComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


VOID
ToasterUnload(
    IN PDRIVER_OBJECT DriverObject
    );



VOID
ToasterCancelQueued (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );



LONG
ToasterIoIncrement    (
    IN  OUT PFDO_DATA   FdoData
    );


LONG
ToasterIoDecrement    (
    IN  OUT PFDO_DATA   FdoData
    );

NTSTATUS
ToasterSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
ToasterSendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
ToasterSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
ToasterQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
ToasterQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
ToasterWmiRegistration(
    IN PFDO_DATA               FdoData
);

NTSTATUS
ToasterWmiDeRegistration(
    IN PFDO_DATA               FdoData
);

NTSTATUS
ToasterReturnResources (
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ToasterQueueRequest    (
    IN OUT PFDO_DATA FdoData,
    IN PIRP Irp
    );


VOID
ToasterProcessQueuedRequests    (
    IN OUT PFDO_DATA FdoData
    );


NTSTATUS
ToasterCanStopDevice    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
ToasterCanRemoveDevice    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
ToasterFireArrivalEvent(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ToasterFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );
#endif  // _TOASTER_H_

