/*++

Copyright (c) 1990, 1991 Microsoft Corporation

Module Name :

    par.h

Abstract:

    Type definitions and data for the parallel port driver

Author:


Revision History:
--*/


#if DBG
#define PARCONFIG             ((ULONG)0x00000001)
#define PARUNLOAD             ((ULONG)0x00000002)
#define PARINITDEV            ((ULONG)0x00000004)
#define PARTIMEOUT            ((ULONG)0x00000008)
#define PARSTART              ((ULONG)0x00000010)
#define PARISR                ((ULONG)0x00000020)
#define PARDPC                ((ULONG)0x00000040)
#define PARCRIT               ((ULONG)0x00000080)
#define PARBUSYPATH           ((ULONG)0x00000100)
#define PARIRPCOMPLETE        ((ULONG)0x00000200)
#define PARDISPATCH           ((ULONG)0x00000400)
#define PARISRACTION          ((ULONG)0x00000800)
#define PARPOLLREPORT         ((ULONG)0x00001000)
#define PARERRORS             ((ULONG)0x40000000)
#define PARBUGCHECK           ((ULONG)0x80000000)
extern ULONG ParDebugLevel;
#define ParDump(LEVEL,STRING) \
        do { \
            ULONG _level = (LEVEL); \
            if (ParDebugLevel & _level) { \
                DbgPrint STRING; \
            } \
            if (_level == PARBUGCHECK) { \
                ASSERT(FALSE); \
            } \
        } while (0)
#else
#define ParDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif

//
// This define gives the default Object directory
// that we should use to insert the symbolic links
// between the NT device name and namespace used by
// that object directory.
#define DEFAULT_DIRECTORY L"DosDevices"

//
// For the above directory, the serial port will
// use the following name as the suffix of the serial
// ports for that directory.  It will also append
// a number onto the end of the name.  That number
// will start at 1.
#define DEFAULT_PARALLEL_NAME L"LPT"
//
//
// This define gives the default NT name for
// for serial ports detected by the firmware.
// This name will be appended to Device prefix
// with a number following it.  The number is
// incremented each time encounter a serial
// port detected by the firmware.  Note that
// on a system with multiple busses, this means
// that the first port on a bus is not necessarily
// \Device\Parallel0.
//
#define DEFAULT_NT_SUFFIX L"Parallel"


//
// Defines the number of interrupts it takes for us to decide that
// we have an interrupt storm on machine
//
#define PARALLEL_STORM_WATCH 500

#define PARALLEL_DATA_OFFSET 0
#define PARALLEL_STATUS_OFFSET 1
#define PARALLEL_CONTROL_OFFSET 2
#define PARALLEL_REGISTER_SPAN 3

typedef struct _CONFIG_DATA {
    //
    // This list entry is used to link all of the "valid"
    // configuration entries together.
    //
    LIST_ENTRY ConfigList;

    //
    // The nt object directory into which to place the symbolic
    // link to this port.
    //
    UNICODE_STRING ObjectDirectory;

    //
    // The suffix to be used in the nt device name space for this
    // port.
    //
    UNICODE_STRING NtNameForPort;

    //
    // The name to be symbolic linked to the nt name.
    //
    UNICODE_STRING SymbolicLinkName;

    //
    // The base address of the registry set for this device.
    //
    PHYSICAL_ADDRESS Controller;

    //
    // The number of contiguous bytes take up by the register
    // set for the device.
    //
    ULONG SpanOfController;

    //
    // The bus number (with respect to the bus type) of the bus
    // that this device occupies.
    //
    ULONG BusNumber;

    //
    // Denotes whether this devices physical addresses live in io space
    // or memory space.
    //
    ULONG AddressSpace;

    //
    // Denotes whether this device is latched or level sensitive.
    //
    KINTERRUPT_MODE InterruptMode;

    //
    // The kind of bus that this device lives on (e.g. Isa, Eisa, MCA, etc)
    //
    INTERFACE_TYPE InterfaceType;

    //
    // The originalirql is what is optained from the firmware data.  The level
    // is also obtained from the firmware data.  When we get a configuration
    // record based on the services portion of the registry we will always set
    // the vector equal to the irql unless overridden by user input.
    //
    ULONG OriginalVector;
    ULONG OriginalIrql;

    //
    // Denotes whether the device should be disabled after it has been
    // initialized.
    //
    ULONG DisablePort;

    } CONFIG_DATA,*PCONFIG_DATA;

typedef enum _PAR_COMMAND {
    ParWrite,
    ParSetInformation,
    ParQueryInformation
    } PAR_COMMAND, *PPAR_COMMAND;

typedef struct _PAR_DEVICE_EXTENSION {

    //
    // For reporting resource usage, we keep around the physical
    // address we got from the registry.
    //
    PHYSICAL_ADDRESS OriginalController;

    //
    // We keep a pointer around to our device name for dumps
    // and for creating "external" symbolic links to this
    // device.
    //
    UNICODE_STRING DeviceName;

    //
    // This points to the object directory that we will place
    // a symbolic link to our device name.
    //
    UNICODE_STRING ObjectDirectory;

    //
    // This points to the device name for this device
    // sans device prefix.
    //
    UNICODE_STRING NtNameForPort;

    //
    // This points to the symbolic link name that will be
    // linked to the actual nt device name.
    //
    UNICODE_STRING SymbolicLinkName;

    //
    // Points to the device object that contains
    // this device extension.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // For value -1  when the interval timer maintained by the IO system
    // fires, it implies that the timer routine is to do nothing.
    //
    // For value 0, then the timer routine should timeout the particular
    // operation.
    //
    // For values > 0 then the timer should do nothing other then synchronize
    // with the isr and decrement this value by 1.
    //
    LONG TimerCount;

    //
    // This holds the current value to initialize TimerCount
    // to when an operation starts.
    //
    ULONG TimerStart;

    //
    // Unexpected interrupts are counted here.  It is cleared
    // every second by the timer management routine.  On
    // a device that is being interrupt driven, if the unexpected
    // interrupt count exceeds a threshold, the interrupts will
    // be disabled on the device and the device will go to polled
    // mode.
    //
    ULONG UnexpectedCount;

    //
    // The base address for the set of device registers
    // of the port.
    //
    PUCHAR Controller;

    //
    // Points to the interrupt object used by this device.
    //
    PKINTERRUPT Interrupt;

    //
    // This value holds the span (in units of bytes) of the register
    // set controlling this port.  This is constant over the life
    // of the port.
    //
    ULONG SpanOfController;

    //
    // Set at intialization to indicate that on the current
    // architecture we need to unmap the base register address
    // when we unload the driver.
    //
    BOOLEAN UnMapRegisters;

    //
    // These two variables denote the "staging" of initializing the
    // device.  Some devices won't respond with an interrupt when
    // initializing.  When we start initializing, we set the Initializing
    // variable to TRUE and the Initialized flag to FALES.
    //
    // Whenever we go an access the hardware we check the initializing flag.
    // If it is true, then we check the "status" of the device, and if the
    // everything is ok, set initializing to FALSE and Initialized to TRUE.
    // If the "status" of the device is not "good", then we process it as an
    // error, and return the error indication to the caller.
    //
    BOOLEAN Initialized;
    BOOLEAN Initializing;

    //
    // This denotes the number of characters left in the current write.
    //
    ULONG CountBuffer;

    BOOLEAN AutoFeed;
    BOOLEAN CompletingIoControl;

    //
    // Records whether we actually created the symbolic link name
    // at driver load time.  If we didn't create it, we won't try
    // to distroy it when we unload.
    //
    BOOLEAN CreatedSymbolicLink;

    //
    // This field will denote that the driver is using a timer instead of
    // interrupts to "schedule" work out to the device.
    //
    BOOLEAN UsingATimer;

    PAR_COMMAND Command;

    //
    // Says whether this device can share interrupts with devices
    // other than parallel devices.
    //
    BOOLEAN InterruptShareable;

    //
    // Denotes to the timer routine whether it should log an error
    // because we were blasted by to many interrupts.
    //
    BOOLEAN StormKnocksOutInterrupts;

    //
    // We keep the following values around so that we can connect
    // to the interrupt and report resources after the configuration
    // record is gone.
    //
    //
    // The following two values are obtained from HalGetInterruptVector
    //
    ULONG Vector;
    KIRQL Irql;

    //
    // The following two values are what is obtained (or deduced) from either
    // the firmware created portion of the registry, or the user data.
    //
    ULONG OriginalVector;
    ULONG OriginalIrql;

    //
    // This is either what is deduced from the particular bus this port is
    // on, or overridden by what the user placed in the registry.
    //
    KINTERRUPT_MODE InterruptMode;

    //
    // Give back by HalGetInterruptVector.  This says what processors this
    // device can interrupt to.
    //
    KAFFINITY ProcessorAffinity;

    //
    // The next three are supplied by the firmware or overridden by the user.
    //
    ULONG AddressSpace;
    ULONG BusNumber;
    INTERFACE_TYPE InterfaceType;

    //
    // All irps go through the start io routine.  Whenver
    // we start out a new irp we increment this value.  This
    // is how we can have a unique irp value to give if we
    // need to log an error during the processing of a particular
    // irp.
    //
    ULONG IrpSequence;

    //
    // This spinlock is used to synchronize access to the hardware
    // when the port is being driver off of a timer, rather than
    // when using interrupts.
    //
    KSPIN_LOCK PollingLock;

    //
    // We use a timer so that we can try to do a bunch of
    // operations, then give the rest of the system time
    // to run
    //
    KTIMER PollingTimer;

    //
    // This dpc is queued when the polling timer expires, and it is
    // also queued after we start a write operations.
    //
    KDPC PollingDpc;

    //
    // This is the length of time the timer will spend in the
    // timer queue.
    //
    LARGE_INTEGER PollingDelayAmount;

    //
    // This timer is used for polling the busy bit.
    //
    KTIMER BusyTimer;

    //
    // This dpc is queued from the isr to start the above timer.
    //
    KDPC StartBusyTimerDpc;

    //
    // This is the dpc queued when the above timer fires.
    //
    KDPC BusyTimerDpc;

    //
    // This holds the delta time fed to above timer.
    //
    LARGE_INTEGER BusyDelayAmount;

    //
    // We keep this boolean around to tell use that we've started off
    // the busy "path of execution".  If this variable is true when
    // the isr would start off the busy path, it won't start it.  If
    // it is false, we set it to true and start the path.  It is not
    // set back to false until just before the path calls back the
    // isr.
    //
    BOOLEAN BusyPath;
} PAR_DEVICE_EXTENSION, *PPAR_DEVICE_EXTENSION;

//
// Bit Definitions in the status register.
//

#define PAR_STATUS_NOT_ERROR   0x08  //not error on device
#define PAR_STATUS_SLCT        0x10  //device is selected (on-line)
#define PAR_STATUS_PE          0x20  //paper empty
#define PAR_STATUS_NOT_ACK     0x40  //not acknowledge (data transfer was not ok)
#define PAR_STATUS_NOT_BUSY    0x80  //operation in progress

//
//  Bit Definitions in the control register.
//

#define PAR_CONTROL_STROBE      0x01 //to read or write data
#define PAR_CONTROL_AUTOFD      0x02 //to autofeed continuous form paper
#define PAR_CONTROL_NOT_INIT    0x04 //begin an initialization routine
#define PAR_CONTROL_SLIN        0x08 //to select the device
#define PAR_CONTROL_IRQ_ENB     0x10 //to enable interrupts
#define PAR_CONTROL_DIR         0x20 //direction = read
#define PAR_CONTROL_WR_CONTROL  0xc0 //the 2 highest bits of the control
                                     // register must be 1

//VOID StoreData(
//      IN PUCHAR RegisterBase,
//      IN UCHAR DataByte
//      )
//Data must be on line before Strobe = 1;
// Strobe = 1, DIR = 0
//Strobe = 0
//
// We change the port direction to output (and make sure stobe is low).
//
// Note that the data must be available at the port for at least
// .5 microseconds before and after you strobe, and that the strobe
// must be active for at least 500 nano seconds.  We are going
// to end up stalling for twice as much time as we need to, but, there
// isn't much we can do about that.
//
// We put the data into the port and wait for 1 micro.
// We strobe the line for at least 1 micro
// We lower the strobe and again delay for 1 micro
// We then revert to the original port direction.
//
// Thanks to Olivetti for advice.
//

#define StoreData(RegisterBase,DataByte)                            \
{                                                                   \
    PUCHAR _Address = RegisterBase;                                 \
    UCHAR _Control;                                                 \
    _Control = GetControl(_Address);                                \
    ASSERT(!(_Control & PAR_CONTROL_STROBE));                       \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)(_Control & ~(PAR_CONTROL_STROBE | PAR_CONTROL_DIR)) \
        );                                                          \
    WRITE_PORT_UCHAR(                                               \
        _Address+PARALLEL_DATA_OFFSET,                              \
        (UCHAR)DataByte                                             \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)((_Control | PAR_CONTROL_STROBE) & ~PAR_CONTROL_DIR) \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)(_Control & ~(PAR_CONTROL_STROBE | PAR_CONTROL_DIR)) \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)_Control                                             \
        );                                                          \
}

//UCHAR
//GetControl(
//  IN PUCHAR RegisterBase
//  )
#define GetControl(RegisterBase) \
    (READ_PORT_UCHAR((RegisterBase)+PARALLEL_CONTROL_OFFSET))


//VOID
//StoreControl(
//  IN PUCHAR RegisterBase,
//  IN UCHAR ControlByte
//  )
#define StoreControl(RegisterBase,ControlByte)  \
{                                               \
    WRITE_PORT_UCHAR(                           \
        (RegisterBase)+PARALLEL_CONTROL_OFFSET, \
        (UCHAR)ControlByte                      \
        );                                      \
}


//UCHAR
//GetStatus(
//  IN PUCHAR RegisterBase
//  )

#define GetStatus(RegisterBase) \
    (READ_PORT_UCHAR((RegisterBase)+PARALLEL_STATUS_OFFSET))

typedef enum _PAR_ERROR_TYPE {
    PoweredOff,
    NotConnected,
    Offline,
    PaperEmpty,
    PowerFailure,
    DataError,
    Busy
} PAR_ERROR_TYPE, *PPAR_ERROR_TYPE;

BOOLEAN
ParInitializeDevice(
    IN PVOID Context
    );

NTSTATUS
ParDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ParSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ParQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ParDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
ParTimerRoutine(
    PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

BOOLEAN
ParManageTimeOut(
    IN PVOID Context
    );

BOOLEAN
ParInterruptServiceRoutine(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
ParInitiateIo(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
ParStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ParUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
ParCancelRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
ParSynchronizeExecution(
    IN PPAR_DEVICE_EXTENSION Extension,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

VOID
ParPollingDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
ParPolling(
    IN PVOID Context
    );

VOID
ParLogError(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PHYSICAL_ADDRESS P1,
    IN PHYSICAL_ADDRESS P2,
    IN ULONG SequenceNumber,
    IN UCHAR MajorFunctionCode,
    IN UCHAR RetryCount,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN NTSTATUS SpecificIOStatus
    );

VOID
ParStartBusyTimer(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
ParBusyTimer(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
ParBusyCallIsr(
    IN PVOID Context
    );

