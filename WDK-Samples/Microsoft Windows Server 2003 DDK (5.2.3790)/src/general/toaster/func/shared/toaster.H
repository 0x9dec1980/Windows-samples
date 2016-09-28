
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.
    
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

#include <ntddk.h> 
#include <wmilib.h> // required for WMILIB_CONTEXT
#include <initguid.h> // required for GUID definitions
#include "driver.h"
#include "public.h"

//
// Let us use newly introduced (Windows Server 2003 DDK) safe string function to avoid 
// security issues related buffer overrun. 
// The advantages of the RtlStrsafe functions include: 
// 1) The size of the destination buffer is always provided to the 
// function to ensure that the function does not write past the end of
// the buffer.
// 2) Buffers are guaranteed to be null-terminated, even if the 
// operation truncates the intended result.
// 

//
// In this driver we are using a safe version vsnprintf, which is 
// RtlStringCbVPrintfA. To use strsafe function on 9x, ME, and Win2K Oses, we 
// have to define NTSTRSAFE_LIB before including this header file and explicitly
// linking to ntstrsafe.lib. If your driver is just targeted for XP and above, there is
// no need to define NTSTRSAFE_LIB and link to the lib.
// 
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>


#define TOASTER_POOL_TAG (ULONG) 'saoT'
#define TOASTER_FDO_INSTANCE_SIGNATURE (ULONG) 'odFT'

#define TOASTER_WAIT_WAKE_ENABLE L"WaitWakeEnabled"
#define TOASTER_POWER_SAVE_ENABLE L"PowerSaveEnabled"

//
// InterlockedOr is not defined in Win2K header files. 
// If not defined, we will define it to use intrinsic function.
// 
#if !defined(InterlockedOr) && (_WIN32_WINNT==0x0500)
#define InterlockedOr _InterlockedOr
#endif

#if !defined(EVENT_TRACING)

//
// Define debug levels
//
#define     ERROR       0 
#define     WARNING  1
#define     TRACE       2
#define     INFO         3

VOID
ToasterDebugPrint    (
    IN ULONG   DebugPrintLevel,
    IN PCCHAR  DebugMessage,
    ...
    );

#else //If software tracing is defined in the sources file..
/*
    WPP_DEFINE_CONTROL_GUID specifies the GUID used for this driver.
    *** REPLACE THE GUID WITH YOUR OWN UNIQUE ID ***
    WPP_DEFINE_BIT allows setting debug bit masks to selectively print.   
    The names defined in the WPP_DEFINE_BIT call define the actual names 
    that are used to control the level of tracing for the control guid specified. 

    Name of the logger is Toaster and the guid is
    {C56386BD-7C67-4264-B8D9-C4A53B93CBEB}
    (0xc56386bd, 0x7c67, 0x4264, 0xb8, 0xd9, 0xc4, 0xa5, 0x3b, 0x93, 0xcb, 0xeb);
*/

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(Toaster,(C56386BD,7C67,4264,B8D9,C4A53B93CBEB), \
        WPP_DEFINE_BIT(ERROR)                /* bit  0 = 0x00000001 */ \
        WPP_DEFINE_BIT(WARNING)              /* bit  1 = 0x00000002 */ \
        WPP_DEFINE_BIT(TRACE)                /* bit  2 = 0x00000004 */ \
        WPP_DEFINE_BIT(INFO)                 /* bit  3 = 0x00000008 */ \
        WPP_DEFINE_BIT(DebugFlag04)          /* bit  4 = 0x00000010 */ \
        WPP_DEFINE_BIT(DebugFlag05)          /* bit  5 = 0x00000020 */ \
        WPP_DEFINE_BIT(DebugFlag06)          /* bit  6 = 0x00000040 */ \
        WPP_DEFINE_BIT(DebugFlag07)          /* bit  7 = 0x00000080 */ \
        WPP_DEFINE_BIT(DebugFlag08)          /* bit  8 = 0x00000100 */ \
        WPP_DEFINE_BIT(DebugFlag09)          /* bit  9 = 0x00000200 */ \
        WPP_DEFINE_BIT(DebugFlag10)          /* bit 10 = 0x00000400 */ \
        WPP_DEFINE_BIT(DebugFlag11)          /* bit 11 = 0x00000800 */ \
        WPP_DEFINE_BIT(DebugFlag12)          /* bit 12 = 0x00001000 */ \
        WPP_DEFINE_BIT(DebugFlag13)          /* bit 13 = 0x00002000 */ \
        WPP_DEFINE_BIT(DebugFlag14)          /* bit 14 = 0x00004000 */ \
        WPP_DEFINE_BIT(DebugFlag15)          /* bit 15 = 0x00008000 */ \
        WPP_DEFINE_BIT(DebugFlag16)          /* bit 16 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag17)          /* bit 17 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag18)          /* bit 18 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag19)          /* bit 19 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag20)          /* bit 20 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag21)          /* bit 21 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag22)          /* bit 22 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag23)          /* bit 23 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag24)          /* bit 24 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag25)          /* bit 25 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag26)          /* bit 26 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag27)          /* bit 27 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag28)          /* bit 28 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag29)          /* bit 29 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag30)           /* bit 30 = 0x00000000 */ \
        WPP_DEFINE_BIT(DebugFlag31)            /* bit 31 = 0x00000000 */ \
        )

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
// Following are the Wake states.
// Even states are cancelled by atomically OR’ing in 1
// WAKESTATE_WAITING -> WAKESTATE_WAITING_CANCELLED
// WAKESTATE_ARMED -> WAKESTATE_ARMING_CANCELLED
// Others unchanged
//

typedef enum {

    WAKESTATE_DISARMED          = 1,     // No outstanding Wait-Wake IRP

    WAKESTATE_WAITING           = 2,      // Wait-Wake IRP requested, not yet seen

    WAKESTATE_WAITING_CANCELLED = 3, // Wait-Wake cancelled before IRP seen again

    WAKESTATE_ARMED             = 4,      // Wait-Wake IRP seen and forwarded. Device is *probably* armed

    WAKESTATE_ARMING_CANCELLED  = 5, // Wait-Wake IRP seen and cancelled. Hasn't reached completion yet

    WAKESTATE_COMPLETING        = 7 // Wait-Wake IRP has passed the completion routine
} WAKESTATE;

//
// The device extension for the device object
//
typedef struct _FDO_DATA
{

    ULONG   Signature; // must be TOASTER_FDO_INSTANCE_SIGNATURE

    PDEVICE_OBJECT      Self; // a back pointer to the DeviceObject.

    PDEVICE_OBJECT      UnderlyingPDO;// The underlying PDO

    PDEVICE_OBJECT      NextLowerDriver; // The top of the device stack just
                                         // beneath this device object.
    DEVICE_PNP_STATE    DevicePnPState;   // Track the state of the device

    DEVICE_PNP_STATE    PreviousPnPState; // Remembers the previous pnp state
    
    UNICODE_STRING      InterfaceName; // The name returned from
                                       // IoRegisterDeviceInterface,
                                       
    // All the fields above this are used in INCOMPLETE1 version
                                       
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

    KEVENT              RemoveEvent; // an event to sync outstandingIO to zero.

    KEVENT              StopEvent;  // an event to sync outstandingIO to 1.

    ULONG               OutstandingIO; // 1-biased count of reasons why
                                       // this object should stick around.
                                       
    // All the fields above this are used in INCOMPLETE2 version*/
    

    BOOLEAN             DontDisplayInUI; // Current state of the device
                                          // as reported by the user.
    SYSTEM_POWER_STATE  SystemPowerState;   // The general system power state

    DEVICE_POWER_STATE  DevicePowerState; // The state of the device(D0 or D3)

    WMILIB_CONTEXT      WmiLibInfo;     // WMI Information

    TOASTER_WMI_STD_DATA   StdDeviceData;

    DEVICE_CAPABILITIES DeviceCaps;   // Copy of the device capability
                                       // Used to find S to D mappings
    PIRP                PendingSIrp; // S-IRP is saved here before generating
                                      // correspoding D-IRP
    BOOLEAN             AllowIdleDetectionRegistration;  // This variable tracks the
                                          // current state of checkbox "..save power"in the
                                          // power management tab. TRUE means
                                          // the box is checked. Initial value of
                                          // this variable should be read from
                                          // the device registry.
    BOOLEAN             AllowWakeArming;  // This variable is TRUE if WMI
                                          // requests to arm for wake should be
                                          // acknowledged
                                          
    // All the fields above this are used in FEATURED1 version*/
                                          
    WAKESTATE           WakeState; //Current wakestate

    PIRP                WakeIrp; //current wait-wake IRP

    KEVENT              WakeCompletedEvent; //Event to flush outstanding wait-wake IRPs

    KEVENT              WakeDisableEnableLock; //Event to sync simultaneous arming & disarming requests

    // All the fields above this are used in FEATURED2 version
                                          
    PULONG          IdleDetection; // Handle returned by 
                                     // PoRegisterDeviceForIdleDetection
    KEVENT          PowerSaveDisableEnableLock;
  
    // All the fields above this are used in FEATURED3 version*/
    
}  FDO_DATA, *PFDO_DATA;

#define CLRMASK(x, mask)     ((x) &= ~(mask));
#define SETMASK(x, mask)     ((x) |=  (mask));

//
// Function prototypes for all versions of toaster func driver
// is given below.
//
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
ToasterDispatchIO(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
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

NTSTATUS
ToasterDispatchWaitWake(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ToasterSetWaitWakeEnableState(
    IN PFDO_DATA FdoData,
    IN BOOLEAN WakeState
    );

BOOLEAN
ToasterGetWaitWakeEnableState(
    IN PFDO_DATA   FdoData
    );

VOID
ToasterAdjustCapabilities(
    IN OUT PDEVICE_CAPABILITIES DeviceCapabilities
    );

BOOLEAN
ToasterArmForWake(
    IN  PFDO_DATA   FdoData,
    IN  BOOLEAN     DeviceStateChange
    );

VOID
ToasterDisarmWake(
    IN  PFDO_DATA   FdoData,
    IN  BOOLEAN     DeviceStateChange
    );

NTSTATUS
ToasterWaitWakeIoCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

VOID
ToasterWaitWakePoCompletionRoutine(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         State,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    );

VOID
ToasterPassiveLevelReArmCallbackWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
ToasterPassiveLevelClearWaitWakeEnableState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
ToasterQueuePassiveLevelCallback(
    IN PFDO_DATA    FdoData,
    IN PIO_WORKITEM_ROUTINE CallbackFunction
    );

NTSTATUS
ToasterGetStandardInterface(
    IN PDEVICE_OBJECT Pdo,
    OUT PTOASTER_INTERFACE_STANDARD BusInterface
    );

VOID
ToasterRegisterForIdleDetection( 
    IN PFDO_DATA   FdoData,
    IN BOOLEAN      DeviceStateChange
    );

VOID
ToasterDeregisterIdleDetection( 
    IN PFDO_DATA   FdoData,
    IN BOOLEAN      DeviceStateChange
    );

NTSTATUS
ToasterSetPowerSaveEnableState(
    IN PFDO_DATA FdoData,
    IN BOOLEAN State
    );

BOOLEAN
ToasterGetPowerSaveEnableState(
    IN PFDO_DATA   FdoData
    );

VOID
ToasterPowerUpDevice(
    IN PFDO_DATA FdoData
    );

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
);


#endif  // _TOASTER_H_

