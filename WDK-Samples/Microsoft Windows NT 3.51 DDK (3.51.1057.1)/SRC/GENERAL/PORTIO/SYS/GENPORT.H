// genport.h    Include file for Generic Port I/O Example Driver
//
// Robert R. Howell         January 6, 1993
// Robert B. Nelson (MS)    January 12, 1993
//      Modified comments prior to posting to Compuserve
//      Changed Wkd* to Gpd*
// Robert B. Nelson (MS)    March 1, 1993
//      Cleanup and bug fixes for Beta 2
//
#include <ntddk.h>
#include <string.h>
#include <devioctl.h>
#include "gpioctl.h"        // Get IOCTL interface definitions

/* Default base port, and # of ports */
#define BASE_PORT       0x300
#define NUMBER_PORTS        4

// NT device name
#define GPD_DEVICE_NAME L"\\Device\\Gpd0"

// File system device name.   When you execute a CreateFile call to open the
// device, use "\\.\GpdDev", or, given C's conversion of \\ to \, use
// "\\\\.\\GpdDev"

#define DOS_DEVICE_NAME L"\\DosDevices\\GpdDev"


// driver local data structure specific to each device object
typedef struct _LOCAL_DEVICE_INFO {
    ULONG               DeviceType;     // Our private Device Type
    BOOLEAN             PortWasMapped;  // If TRUE, we have to unmap on unload
    PVOID               PortBase;       // base port address
    ULONG               PortCount;      // Count of I/O addresses used.
    ULONG               PortMemoryType; // HalTranslateBusAddress MemoryType
    PDEVICE_OBJECT      DeviceObject;   // The Gpd device object.
} LOCAL_DEVICE_INFO, *PLOCAL_DEVICE_INFO;

/********************* function prototypes ***********************************/
//

NTSTATUS    DriverEntry(       IN  PDRIVER_OBJECT DriverObject,
                               IN  PUNICODE_STRING RegistryPath );

NTSTATUS    GpdCreateDevice(   IN  PWSTR szPrototypeName,
                               IN  DEVICE_TYPE DeviceType,
                               IN  PDRIVER_OBJECT DriverObject,
                               OUT PDEVICE_OBJECT *ppDevObj     );

NTSTATUS    GpdDispatch(       IN  PDEVICE_OBJECT pDO,
                               IN  PIRP pIrp                    );

NTSTATUS    GpdIoctlReadPort(  IN  PLOCAL_DEVICE_INFO pLDI,
                               IN  PIRP pIrp,
                               IN  PIO_STACK_LOCATION IrpStack,
                               IN  ULONG IoctlCode              );

NTSTATUS    GpdIoctlWritePort( IN  PLOCAL_DEVICE_INFO pLDI,
                               IN  PIRP pIrp,
                               IN  PIO_STACK_LOCATION IrpStack,
                               IN  ULONG IoctlCode              );

VOID        GpdUnload(         IN  PDRIVER_OBJECT DriverObject );

