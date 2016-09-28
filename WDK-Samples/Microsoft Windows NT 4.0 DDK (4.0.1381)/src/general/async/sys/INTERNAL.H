//////////////////////////////////////////////////////////////////////////////
// Program Name: Async.h
// Programmer: Mark A. Overby (MarkOv)
// Description: Main header for async driver sources
//////////////////////////////////////////////////////////////////////////////

// Device extension structure definition.

typedef struct _DEVICE_EXTENSION {

    //
    // Points to the device object that contains
    // this device extension.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // Queue for holding the Irp that we will be waiting for our device
    // event to release it.
    //
    LIST_ENTRY WaitQueue;

    // Count of how many times we have passed through our IoTimerRoutine
    // since the last release of an IRP

    LONG nCount ;

    // Count of how many times we should go through the IoTimerRoutine
    // before releasing the IOCTL_HOLD_REQUEST Irp.

    LONG nDelayValue ;

    // Spin lock to protect the WaitQueue

    KSPIN_LOCK QueueSpin ;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Defined values for debugging purposes.

#define INFORM          0x02
#define ERROR           0x01

// Macro for debugging messages.

#define AsyncMessage(LEVEL,MESSAGE) \
      do { \
         if (DebugValue & LEVEL) {  \
            DbgPrint MESSAGE ;        \
         }                            \
      } while ( 0 )

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    ) ;

NTSTATUS
AsyncCreateClose (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    ) ;

NTSTATUS
AsyncDispatchRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    ) ;

NTSTATUS
AsyncCleanup (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    ) ;

VOID
AsyncUnload (
    IN PDRIVER_OBJECT   DriverObject
    ) ;

VOID
AsyncCancelSup (
    IN PIRP             Irp
    ) ;

VOID AsyncCancel (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    ) ;

VOID AsyncTimerRoutine (
    IN PDEVICE_OBJECT   pDeviceObject,
    IN OUT PVOID        Context
    ) ;

