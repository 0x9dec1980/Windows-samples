//////////////////////////////////////////////////////////////////////////////
// Program Name: ParStat.h
// Programmer: Norbert Kusters (NorbertK)
//             Mark Overby (MarkOv)
// Description: Main header for parstat sources
//////////////////////////////////////////////////////////////////////////////

#define PARALLEL_DATA_OFFSET 0
#define PARALLEL_STATUS_OFFSET 1
#define PARALLEL_CONTROL_OFFSET 2
#define PARALLEL_REGISTER_SPAN 3

ULONG debugLevel ;

typedef struct _DEVICE_EXTENSION {

    //
    // Points to the device object that contains
    // this device extension.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // Points to the port device object that this class device is
    // connected to.
    //
    PDEVICE_OBJECT PortDeviceObject;

    //
    // Work queue.  Manipulate with cancel spin lock.
    //
    LIST_ENTRY WorkQueue;
    PIRP CurrentIrp;

    //
    // This holds the result of the get parallel port info
    // request to the port driver.
    //
    PHYSICAL_ADDRESS OriginalController;
    PUCHAR Controller;
    ULONG SpanOfController;
    PPARALLEL_FREE_ROUTINE FreePort;
    PVOID FreePortContext;

    //
    // Records whether we actually created the symbolic link name
    // at driver load time and the symbolic link itself.  If we didn't
    // create it, we won't try to destroy it when we unload.
    //
    BOOLEAN CreatedSymbolicLink;
    UNICODE_STRING SymbolicLinkName;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    ) ;

VOID
ParStatInitalizeDeviceObject (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  ULONG           ParallelPortNum
    ) ;

BOOLEAN
ParStatMakeNames (
    IN  ULONG           ParallelPortNum,
    OUT PUNICODE_STRING PortName,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    ) ;

NTSTATUS
ParStatGetPortInfoFromPortDevice (
    IN OUT PDEVICE_EXTENSION Extension
    ) ;

NTSTATUS
ParStatCreate(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParStatClose (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    ) ;

NTSTATUS
ParStatRead(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
ParStatAllocPort (
    IN  PDEVICE_EXTENSION  Extension
    ) ;

NTSTATUS
ParStatReadCompletionRoutine (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    ) ;

VOID
ParStatAllocPortWithNext (
    IN OUT PDEVICE_EXTENSION  Extension
    ) ;

NTSTATUS
ParStatCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
ParStatUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );

VOID
ParStatCancel (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    ) ;

VOID
ParStatLogInformation (
    IN  PDRIVER_OBJECT  DrvObj,
    IN  PDEVICE_OBJECT  DevObj,
    IN  NTSTATUS        Code,
    IN  NTSTATUS        FinalStatus) ;
