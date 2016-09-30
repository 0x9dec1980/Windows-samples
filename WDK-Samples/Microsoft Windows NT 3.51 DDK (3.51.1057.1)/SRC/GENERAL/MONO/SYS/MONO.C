/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    mono.c

Abstract:

    A simple kernel-mode driver sample.

Environment:

    kernel mode only

Notes:

    See readme.txt

Revision History:

    03-22-93 : created
    06-02-93 : #ifdef's removed from IoCreateSymbolicLink, and
               bulk of pMonoReportResourceUsage commented out for
               ease of use with other drivers (see note below)
--*/

#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"
#include "monopub.h"
#include "monopriv.h"



//
// NOTE: Routines prefixed with "p" are private to this module
//

NTSTATUS
MonoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MonoUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
pMonoLocateDevice(
    void
    );

PVOID
pMonoGetMappedAddress(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG            AddressSpace,
    IN ULONG            NumberOfBytes
    );

BOOLEAN
pMonoReportResourceUsage(
    IN PDRIVER_OBJECT   DriverObject,
    IN PMONO_RESOURCE   MonoResources,
    IN ULONG            NumberOfResources
    );

VOID
MonoDbgPrint(
    IN ULONG  DbgPrintLevel,
    IN PUCHAR DbgMessage,
    IN ...
    );

VOID
pMonoPrint(
    IN PMONO_DEVICE_EXTENSION DeviceExtension,
    IN PUCHAR                 TextString,
    IN ULONG                  BytesToMove
    );

VOID
pMonoClearScreen(
    IN PMONO_DEVICE_EXTENSION DeviceExtension
    );



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{

    PDEVICE_OBJECT         deviceObject        = NULL;
    NTSTATUS               ntStatus;
    WCHAR                  deviceNameBuffer[]  = L"\\Device\\Mono";
    UNICODE_STRING         deviceNameUnicodeString;
    PMONO_DEVICE_EXTENSION deviceExtension;
    BOOLEAN                symbolicLinkCreated = FALSE;
    WCHAR                  deviceLinkBuffer[]  = L"\\DosDevices\\MONO";
    UNICODE_STRING         deviceLinkUnicodeString;


    pMonoKdPrint (("MONO.SYS: entering DriverEntry\n"));



    //
    // Check to see if anybody else has claim the resources we want to
    // used. We don't want to be partying on someone else's hardware,
    // even when trying to locate our devicc.
    //

    if (!pMonoReportResourceUsage (DriverObject,
                                   MonoResources,
                                   MONO_NUMBER_RESOURCE_ENTRIES
                                   ))
    {
        //
        // There's a resource conflict
        //

        pMonoKdPrint (("MONO.SYS: pMonoReportResourceUsage failed\n"));

        return STATUS_UNSUCCESSFUL;
    }



    //
    // Try to locate a monochrome display adapter (MDA)
    //

    ntStatus = pMonoLocateDevice ();

    if (!NT_SUCCESS(ntStatus))
    {
        pMonoKdPrint (("MONO.SYS: pMonoLocateDevice failed\n"));

        ntStatus = STATUS_UNSUCCESSFUL;

        goto done_DriverEntry;
    }



    //
    // OK, we've claimed our resources & found our h/w, so create
    // a device and initialize stuff...
    //

    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer
                          );



    //
    // Create an EXCLUSIVE device, i.e. only 1 thread at a time can send
    // i/o requests. If opened as non-exclusive, then we would need to
    // implement a more robust synchronization scheme than the event
    // mechanism we utilize below.
    //

    ntStatus = IoCreateDevice (DriverObject,
                               sizeof (MONO_DEVICE_EXTENSION),
                               &deviceNameUnicodeString,
                               FILE_DEVICE_MONO,
                               0,
                               TRUE,
                               &deviceObject
                               );

    if (NT_SUCCESS(ntStatus))
    {

        GlobalDeviceExtension =
        deviceExtension = (PMONO_DEVICE_EXTENSION) deviceObject->DeviceExtension;


        //
        // Initialize the dispatch event object. This allows us to
        // synchronize access to the h/w registers...
        //

        KeInitializeEvent (&deviceExtension->SyncEvent,
                           SynchronizationEvent,
                           TRUE
                           );



        deviceExtension->InterfaceType = Isa;
        deviceExtension->BusNumber     = 0;


        //
        // Map all the required resources, save the addresses
        //

        deviceExtension->VideoMemory =

            (PUSHORT) pMonoGetMappedAddress (MonoResources[MONO_VIDEO_BUFFER].PhysicalAddress,
                                             MonoResources[MONO_VIDEO_BUFFER].AddressSpace,
                                             MonoResources[MONO_VIDEO_BUFFER].Length
                                             );

        deviceExtension->CRTCRegisters =

            (PUCHAR) pMonoGetMappedAddress (MonoResources[MONO_CRTC_REG].PhysicalAddress,
                                            MonoResources[MONO_CRTC_REG].AddressSpace,
                                            MonoResources[MONO_CRTC_REG].Length
                                            );

        deviceExtension->ModeControlRegister =

            (PUCHAR) pMonoGetMappedAddress (MonoResources[MONO_MODE_CTL_REG].PhysicalAddress,
                                            MonoResources[MONO_MODE_CTL_REG].AddressSpace,
                                            MonoResources[MONO_MODE_CTL_REG].Length
                                            );

        if (!deviceExtension->VideoMemory         ||
            !deviceExtension->CRTCRegisters       ||
            !deviceExtension->ModeControlRegister
            )
        {

            pMonoKdPrint (("MONO.SYS: pMonoGetMappedAddress failed\n"));

            ntStatus = STATUS_UNSUCCESSFUL;

            goto done_DriverEntry;

        }

        deviceExtension->Xpos =
        deviceExtension->Ypos = 0;



        //
        // Create a symbolic link that Win32 apps can specify to gain access
        // to this driver/device
        //

        RtlInitUnicodeString (&deviceLinkUnicodeString,
                              deviceLinkBuffer
                              );

        ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                                         &deviceNameUnicodeString
                                         );


        if (!NT_SUCCESS(ntStatus))
        {
            pMonoKdPrint (("MONO.SYS: IoCreateSymbolicLink failed\n"));
        }
        else
        {
            symbolicLinkCreated = TRUE;
        }



        //
        // Create dispatch points for device control, create, close.
        //

        DriverObject->MajorFunction[IRP_MJ_CREATE]         =
        DriverObject->MajorFunction[IRP_MJ_CLOSE]          =
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MonoDispatch;
        DriverObject->DriverUnload                         = MonoUnload;



        //
        // Set the Start Address registers (indicate which part of
        // video buffer is displayed) to 0
        //

        WRITE_PORT_UCHAR (deviceExtension->CRTCRegisters,     0xc);
        WRITE_PORT_UCHAR (deviceExtension->CRTCRegisters + 1, 0);
        WRITE_PORT_UCHAR (deviceExtension->CRTCRegisters,     0xd);
        WRITE_PORT_UCHAR (deviceExtension->CRTCRegisters + 1, 0);



        //
        // Clear screen & alert world of our humble existence
        //

        pMonoClearScreen (deviceExtension);

        #define INIT_STRING "Mono driver initialized\n"

        pMonoPrint (deviceExtension,
                    INIT_STRING,
                    sizeof (INIT_STRING)
                    );
    }


done_DriverEntry:

    if (!NT_SUCCESS(ntStatus))
    {
        //
        // Something went wrong, so clean up
        //

        pMonoReportResourceUsage (DriverObject,
                                  NULL,
                                  0
                                  );

        if (symbolicLinkCreated)

            IoDeleteSymbolicLink (&deviceLinkUnicodeString);


        if (deviceObject)

            IoDeleteDevice (deviceObject);

        GlobalDeviceExtension = NULL;
    }

    return ntStatus;
}



NTSTATUS
MonoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{

    PIO_STACK_LOCATION      irpStack;
    PMONO_DEVICE_EXTENSION  deviceExtension;
    PVOID                   ioBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    ULONG                   ioControlCode;
    NTSTATUS                ntStatus;



    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;


    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation (Irp);



    //
    // Get a pointer to the device extension
    //

    deviceExtension = DeviceObject->DeviceExtension;



    //
    // Get the pointer to the input/output buffer and it's length
    //

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;



    //
    // Synchronize execution of the dispatch routine by acquiring the device
    // event object. This ensures all request are serialized.
    //

    KeWaitForSingleObject (&deviceExtension->SyncEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           (PTIME) NULL
                           );



    switch (irpStack->MajorFunction)
    {
    case IRP_MJ_CREATE:

        pMonoKdPrint (("MONO.SYS: IRP_MJ_CREATE\n"));

        break;

    case IRP_MJ_CLOSE:

        pMonoKdPrint (("MONO.SYS: IRP_MJ_CLOSE\n"));

        break;

    case IRP_MJ_DEVICE_CONTROL:

        pMonoKdPrint (("MONO.SYS: IRP_MJ_DEVICE_CONTROL\n"));

        ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

        switch (ioControlCode)
        {

        case IOCTL_MONO_PRINT:
        {
            pMonoPrint (deviceExtension,
                        ioBuffer,
                        inputBufferLength
                        );

            break;
        }

        case IOCTL_MONO_CLEAR_SCREEN:
        {
            pMonoClearScreen (deviceExtension);

            break;
        }

        default:

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

            pMonoKdPrint (("MONO.SYS: unknown IRP_MJ_DEVICE_CONTROL\n"));

            break;

        }

        break;
    }


    KeSetEvent (&deviceExtension->SyncEvent,
                0,
                FALSE
                );

    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );


    //
    // We never have pending operation so always return the status code.
    //

    return ntStatus;
}



VOID
MonoUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:


--*/
{
    WCHAR                  deviceLinkBuffer[]  = L"\\DosDevices\\MONO";
    UNICODE_STRING         deviceLinkUnicodeString;


    //
    // Unreport the resources
    //

    pMonoReportResourceUsage (DriverObject,
                              NULL,
                              0
                              );



    //
    // Delete the symbolic link
    //

    RtlInitUnicodeString (&deviceLinkUnicodeString,
                          deviceLinkBuffer
                          );

    IoDeleteSymbolicLink (&deviceLinkUnicodeString);




    //
    // Delete the device object
    //

    IoDeleteDevice (DriverObject->DeviceObject);

    pMonoKdPrint (("MONO.SYS: unloading\n"));
}



NTSTATUS
pMonoLocateDevice(
    )
/*++

Routine Description:

    Attempts to locate an MDA

Arguments:


Return Value:

    STATUS_SUCCESS if adapter found,
    STATUS_UNSUCCESSFUL otherwise

--*/
{

    NTSTATUS         ntStatus = STATUS_SUCCESS;
    PHYSICAL_ADDRESS physicalAddress;
    PUCHAR           crtcRegisters;

    //
    // Our FindAdapter logic:
    //
    //    - retrieve an address for the 6845's index register (0x3b4)
    //    - write a 0x0f to the index register (0x0f = index of Cursor
    //      Location Low register)
    //    - write a 0x55 to 0x3b5
    //    - read 0x3b5, and it should == 0x55
    //

    physicalAddress.LowPart  = 0x3b4;
    physicalAddress.HighPart = 0;


    if ((crtcRegisters = pMonoGetMappedAddress (physicalAddress,
                                                1,
                                                2
                                                )))
    {
        WRITE_PORT_UCHAR (crtcRegisters,     0x0f);
        WRITE_PORT_UCHAR (crtcRegisters + 1, 0x55);

        if (READ_PORT_UCHAR (crtcRegisters + 1) != 0x55)
        {
            pMonoKdPrint (("MONO.SYS: could not find adapter\n"));

            ntStatus = STATUS_UNSUCCESSFUL;

            goto done_LocateDevice;
        }

        //
        //  Set current cursor location back to (0,0)
        //

        WRITE_PORT_UCHAR (crtcRegisters + 1, 0x00);
    }

    else
    {
        pMonoKdPrint (("MONO.SYS: HalTranslatedAddress(0x3b4) failed\n"));

        ntStatus = STATUS_UNSUCCESSFUL;
    }

done_LocateDevice:

    return ntStatus;
}



PVOID
pMonoGetMappedAddress(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG            AddressSpace,
    IN ULONG            NumberOfBytes
    )
/*++

Routine Description:

    Given a physical address, retrieves a corresponding system address
    that can be used by a kernel mode driver.

Arguments:

    PhysicalAddress - physical address to map

    AddressSpace    - 0 if in memory space, 1 if in I/O space

    NumberOfBytes   - length of section to map

Return Value:

    A valid system address is successful,
    NULL otherwise.

--*/
{
    //
    // Assume we're on Isa bus # 0
    //

    IN INTERFACE_TYPE  interfaceType = Isa;
    IN ULONG           busNumber = 0;
    PHYSICAL_ADDRESS   translatedAddress;
    PVOID              deviceBase = NULL;

    if (HalTranslateBusAddress (interfaceType,
                                busNumber,
                                PhysicalAddress,
                                &AddressSpace,
                                &translatedAddress
                                ))
    {
        if (!AddressSpace)
        {
            if (!(deviceBase = MmMapIoSpace (translatedAddress,
                                             NumberOfBytes,
                                             FALSE          // noncached memory
                                             )))
            {
                pMonoKdPrint (("MONO.SYS: MmMapIoSpaceFailed\n"));
            }
        }
        else
        {
            deviceBase = (PVOID) translatedAddress.LowPart;
        }
    }

    else
    {
        pMonoKdPrint (("MONO.SYS: HalTranslateBusAddress failed\n"));
    }

    return deviceBase;
}



BOOLEAN
pMonoReportResourceUsage(
    IN PDRIVER_OBJECT DriverObject,
    IN PMONO_RESOURCE MonoResources,
    IN ULONG          NumberOfResources
    )
/*++

Routine Description:

    Reports the resources used by a device.

Arguments:

    DriverObject      - pointer to a driver object

    MonoResources     - pointer to an array of resource information, or
                        NULL is unreporting resources for this driver

    NumberOfResources - number of entries in the resource array, or
                        0 if unreporting resources for this driver

Return Value:

    TRUE if resources successfully report (and no conflicts),
    FALSE otherwise.

--*/
{



//
// NOTE: The following is commented out because some of the video miniport
//       drivers provided with the system (and in the NT DDK) regsister
//       their resources as exclusive;  if these resources overlap with
//       the resources used by the mono driver, then this driver will fail
//       to load.  This can be solved in one of two ways: by commenting
//       out the code below, and using the resources without reporting them
//       (not safe to do, but the easiest for this simple example); or, by
//       editing the source file(s) of the appropriate miniport driver
//       such that the VIDEO_ACCESS_RANGE.RangeSharable element is set to
//       TRUE, and rebuilding the driver.
//
//       A real driver should *always* report it's resources.
//


/*
    ULONG                           sizeOfResourceList = 0;
    PCM_RESOURCE_LIST               resourceList       = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    ULONG                           i;
    UNICODE_STRING                  className;
    BOOLEAN                         conflictDetected;


    if (NumberOfResources > 0)
    {
        //
        // Alloc enough memory to build a resource list & zero it out
        //

        sizeOfResourceList = sizeof(CM_RESOURCE_LIST) +
                             (sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)*
                             (NumberOfResources - 1)
                             );

        resourceList = ExAllocatePool (PagedPool,
                                       sizeOfResourceList
                                       );

        if (!resourceList)
        {
            pMonoKdPrint (("MONO.SYS: ExAllocPool failed\n"));

            return FALSE;
        }

        RtlZeroMemory (resourceList,
                       sizeOfResourceList
                       );


        //
        // Fill in the reosurce list
        //
        // NOTE: Assume Isa, bus # 0
        //

        resourceList->Count = 1;

        resourceList->List[0].InterfaceType = Isa;
        resourceList->List[0].BusNumber     = 0;

        resourceList->List[0].PartialResourceList.Count = NumberOfResources;

        partial = &resourceList->List[0].PartialResourceList.PartialDescriptors[0];

        //
        // Account for the space used by the controller.
        //

        for (i = 0; i < NumberOfResources; i++)
        {
            if (MonoResources[i].AddressSpace)
            {
                partial->Type          = CmResourceTypePort;
                partial->Flags         = CM_RESOURCE_PORT_IO;
                partial->u.Port.Start  = MonoResources[i].PhysicalAddress;
                partial->u.Port.Length = MonoResources[i].Length;
            }
            else
            {
                partial->Type            = CmResourceTypeMemory;
                partial->Flags           = CM_RESOURCE_MEMORY_READ_WRITE;
                partial->u.Memory.Start  = MonoResources[i].PhysicalAddress;
                partial->u.Memory.Length = MonoResources[i].Length;
            }

            if (MonoResources[i].RangeSharable)
            {
                partial->ShareDisposition = CmResourceShareShared;
            }

            else
            {
                partial->ShareDisposition = CmResourceShareDeviceExclusive;
            }

            partial++;
        }
    }

    RtlInitUnicodeString (&className,
                          L"LOADED MONO DRIVER RESOURCES"
                          );

    IoReportResourceUsage (&className,
                           DriverObject,
                           resourceList,
                           sizeOfResourceList,
                           NULL,
                           NULL,
                           0,
                           FALSE,
                           &conflictDetected
                           );

    if (resourceList)
    {
        ExFreePool (resourceList);


        if (conflictDetected)

            return FALSE;

        else

            return TRUE;
    }
*/
    return TRUE;
}



VOID
MonoDbgPrint(
    IN ULONG  DbgPrintLevel,
    IN PUCHAR DbgMessage,
    IN ...
    )
/*++

Routine Description:

    Format the incoming debug message & calls pMonoPrint to write the
    message out to the attached MDA.

Arguments:

    DbgPrintLevel - level of message verboseness

    DbgMessage    - printf-style format string, followed by appropriate
                    list of arguments

Return Value:


--*/
{
    va_list ap;

    va_start(ap, DbgMessage);

    if (DbgPrintLevel <= MonoDbgLevel)
    {
        char buf[256];

        vsprintf (buf,
                  DbgMessage,
                  ap
                  );

        if (GlobalDeviceExtension)

            pMonoPrint (GlobalDeviceExtension,
                        buf,
                        strlen (buf)
                        );
    }

    va_end(ap);
}



VOID
pMonoPrint(
    IN PMONO_DEVICE_EXTENSION DeviceExtension,
    IN PUCHAR                 TextString,
    IN ULONG                  BytesToMove
    )
/*++

Routine Description:

    Writes a specified string to MDA memory

Arguments:

    DeviceExtension - pointer to a device extension

    TextString      - pointer to an ascii string

    BytesToMove     - number of characters to write

Return Value:


--*/
{

    PUSHORT VideoMemory;
    ULONG   x,y,i,j;
    UCHAR   Letter;

    VideoMemory = DeviceExtension->VideoMemory;
    x           = DeviceExtension->Xpos;
    y           = DeviceExtension->Ypos;


    for (i = 0; i < BytesToMove; i++)
    {
        Letter = (UCHAR) *(TextString + i);

        if (Letter == '\n')
        {
            //
            // Blank out the rest of the line
            //

            for (j = x; j < 80; j++)
            {
                *(VideoMemory + j + (y * 80)) = 0;
            }
            x = 0;
            y = y + 1;
        }

        else
        {
            //
            // Write character & attribute out to video memory
            //
            // 0x0700 = No blink, black background, normal intensity,
            //          white chracter
            //

            *(VideoMemory + x + (y * 80)) = Letter | 0x0700;

            x++;
        }

        if (x > 79)
        {
            //
            // Wrap around to next line
            //

            x = 0;
            y++;
        }

        if (y > 24)
        {
            //
            // We've reached the bottom of the screen, scroll screen
            // up by 1 row
            //

            RtlMoveMemory (VideoMemory,
                           VideoMemory+80,
                           24 * 80 * 2
                           );
            y = 24;


            //
            // Zero out the bottom row
            //

            for (j = 0; j < 80; j++)
            {
                *(VideoMemory + j + (24 * 80)) = 0;
            }
        }
    }

    DeviceExtension->Xpos = x;
    DeviceExtension->Ypos = y;
}



VOID
pMonoClearScreen(
    IN PMONO_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Clears the MDA

Arguments:


Return Value:


--*/
{

    ULONG  i;

    //
    // Zero out the MDA memory
    //

    for (i = 0; i < 25 * 80; i++)

        *(DeviceExtension->VideoMemory + i) = 0;

    DeviceExtension->Xpos =
    DeviceExtension->Ypos = 0;
}
