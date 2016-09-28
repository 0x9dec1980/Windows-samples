
#include "ntddk.h"
#include "stat.h"

#define NT_DEVICE_NAME	    L"\\Device\\Stat"
#define DOS_DEVICE_NAME     L"\\DosDevices\\LOADTEST"
#define ERROR_MESSAGE	    L"Stat: Error Message Test"

BOOLEAN
statInterruptServiceRoutine(
    IN PKINTERRUPT Interrupt,
    IN OUT PVOID Context
    );

VOID
statDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

BOOLEAN
statReportUsage(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PHYSICAL_ADDRESS PortAddress,
    IN BOOLEAN *GotResources
    );

NTSTATUS
statOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
statClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
statUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
statDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
statStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
statDoIo(
    IN OUT PVOID Context
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{

    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status, ioConnectStatus;
    UNICODE_STRING uniNtNameString;
    UNICODE_STRING uniWin32NameString;
    KIRQL irql = STAT_IRQ;
    KAFFINITY Affinity;
    ULONG MappedVector, AddressSpace = 1;
    PSTAT_DEVICE_EXTENSION extension;
    BOOLEAN GotResources;
    PHYSICAL_ADDRESS InPortAddr, OutPortAddr;

    KdPrint( ("STAT: Entered the Stat driver!\n") );

//    _asm {int 3};

    //
    // This call will map our IRQ to a system vector. It will also fill
    // in the IRQL (the kernel-defined level at which our ISR will run),
    // and affinity mask (which processors our ISR can run on).
    //
    // We need to do this so that when we connect to the interrupt, we
    // can supply the kernel with this information.
    //
    MappedVector = HalGetInterruptVector(
			 Isa,	    // Interface type
			 0,	    // Bus number

                         //
                         // This is the IRQ number. The constannt
                         // STAT_IRQ is defined in stat.h
                         //
			 STAT_IRQ, // Bus interrupt level
			 STAT_IRQ, // Bus interrupt vector

			 // These next two parameters are
			 // filled in by the HAL

			 &irql,     // IRQ level
			 &Affinity  // Affinity mask
			 );


    //
    // Translate the base port address to a system mapped address.
    // This will be saved in the device extension after IoCreateDevice,
    // because we use the translated port address to access the ports.
    //
    InPortAddr.LowPart = DATA_PORT_ADDRESS; //defined in stat.h
    InPortAddr.HighPart = 0;
    if (!HalTranslateBusAddress(
		Isa,
		0,
		InPortAddr,
		&AddressSpace,
		&OutPortAddr
		)) {

        KdPrint( ("STAT: HalTranslateBusAddress failed\n") );
        return STATUS_SOME_NOT_MAPPED;
    }

    //
    // Create counted string version of our device name.
    //

    RtlInitUnicodeString( &uniNtNameString, NT_DEVICE_NAME );

    //
    // Create the device object
    //

    status = IoCreateDevice(
                 DriverObject,
		 sizeof(STAT_DEVICE_EXTENSION),
                 &uniNtNameString,
                 FILE_DEVICE_UNKNOWN,
                 0,
                 FALSE,
                 &deviceObject
                 );

    if ( NT_SUCCESS(status) )
    {

        //
	// Create dispatch points for create/open, close, unload, and ioctl
        //

	DriverObject->MajorFunction[IRP_MJ_CREATE] = statOpen;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = statClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
				      statDeviceControl;
	DriverObject->DriverUnload  = statUnload;
	DriverObject->DriverStartIo = statStartIo;

	//
	// check if resources (ports and interrupt) are available
	//
	if (!statReportUsage(
                    DriverObject,
                    deviceObject,
                    OutPortAddr,
                    &GotResources)) {

	    KdPrint( ("STAT: Couldn't get resources") );
	    IoDeleteDevice(deviceObject);
	    return STATUS_INSUFFICIENT_RESOURCES;
	}

	//
	// register DPC routine
	//
	IoInitializeDpcRequest(deviceObject, statDpcRoutine);

	//
	// fill in the device extension
	//
	extension = (PSTAT_DEVICE_EXTENSION) deviceObject->DeviceExtension;
	extension->DeviceObject = deviceObject;
        extension->PortAddress = (PVOID)OutPortAddr.LowPart;

	//
	// connect the device driver to the IRQ
	//
	ioConnectStatus = IoConnectInterrupt(
				&extension->InterruptObject,
				statInterruptServiceRoutine,
				extension->DeviceObject,
				NULL,
				MappedVector,
				irql,
				irql,
				Latched,
				FALSE,
				Affinity,
				FALSE
				);

	if ( !NT_SUCCESS (ioConnectStatus) )
	{
	    KdPrint( ("STAT: couldn't connect interrupt\n" ) );
	    IoDeleteDevice(deviceObject);
	    return ioConnectStatus;
	}

	KdPrint( ("STAT: just about ready!\n") );

        //
        // Create counted string version of our Win32 device name.
        //
    
        RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );
    
        //
        // Create a link from our device name to a name in the Win32 namespace.
        //
        
        status = IoCreateSymbolicLink( &uniWin32NameString, &uniNtNameString );

        if (!NT_SUCCESS(status))
        {
	    KdPrint( ("STAT: Couldn't create the symbolic link\n") );
	    IoDeleteDevice( DriverObject->DeviceObject );
        }
        else
	{
            //
            // Initialize the device
            //
	    WRITE_PORT_UCHAR (extension->PortAddress+CONTROL_PORT,
			      (UCHAR) RESET);
	    WRITE_PORT_UCHAR (extension->PortAddress+CONTROL_PORT,
			      (UCHAR) LOAD+TF_ALL);
	    WRITE_PORT_UCHAR (extension->PortAddress+CONTROL_PORT,
			      (UCHAR) INIT_PTR);
	    WRITE_PORT_UCHAR (extension->PortAddress+CONTROL_PORT,
			      (UCHAR) PTR_MASTER);
            WRITE_PORT_UCHAR (extension->PortAddress+INTERRUPT_PORT,
                              (UCHAR) 0);

	    KdPrint( ("STAT: All initialized!\n") );
        }
    }
    else
    {
	KdPrint( ("STAT: Couldn't create the device\n") );
    }
    return status;
}

NTSTATUS
statOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    //
    // No need to do anything.
    //

    //
    // Fill these in before calling IoCompleteRequest.
    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    KdPrint( ("STAT: Opened!!\n") );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

NTSTATUS
statClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    //
    // No need to do anything.
    //

    //
    // Fill these in before calling IoCompleteRequest.
    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    KdPrint( ("STAT: Closed!!\n") );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

VOID
statUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    UNICODE_STRING uniWin32NameString;
    PSTAT_DEVICE_EXTENSION extension;
    BOOLEAN GotResources;

    extension = DriverObject->DeviceObject->DeviceExtension;

    //
    // reset the board
    //	1. Master Reset command
    //	2. Load Timer of all timers
    //	3. Initialize Data Pointer
    //	4. Load Data Pointer
    //
    WRITE_PORT_UCHAR (extension->PortAddress+CONTROL_PORT,
		      (UCHAR) RESET);
    WRITE_PORT_UCHAR (extension->PortAddress+CONTROL_PORT,
		      (UCHAR) LOAD+TF_ALL);
    WRITE_PORT_UCHAR (extension->PortAddress+CONTROL_PORT,
		      (UCHAR) INIT_PTR);
    WRITE_PORT_UCHAR (extension->PortAddress+CONTROL_PORT,
		      (UCHAR) PTR_MASTER);

    KdPrint( ("STAT: Unloading!!\n") );

    //
    // Disconnect the interrupt
    //
    IoDisconnectInterrupt(extension->InterruptObject);

    //
    // We don't need our resourced anymore (ports and interrupt)
    //
    IoReportResourceUsage(
	NULL,
        DriverObject,
	NULL,
	0,
        NULL,
        NULL,
        0,
        FALSE,
	&GotResources
        );
    
    //
    // Create counted string version of our Win32 device name.
    //
    RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );

    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //
    IoDeleteSymbolicLink( &uniWin32NameString );

    //
    // Finally delete our device object
    //
    IoDeleteDevice( DriverObject->DeviceObject );
}

NTSTATUS
statDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for IoDeviceControl().
    The only supported IOCTL here returns a single DWORD of information.

Arguments:

    DeviceObject - Pointer to the device object for this driver.
    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.


--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PULONG OutputBuffer;
    PSTAT_DEVICE_EXTENSION extension;
    ULONG ioControlCode;

    extension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Save the control code in the extension so we can use it later
    //
    ioControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    switch (ioControlCode) {

	case IOCTL_STAT_GET_INTERRUPT_COUNT:

            //
            // This IOCTL will return the information stored in the
            // device extension, so we could just move the date to the
            // output buffer and complete the IRP here.
            //

	    KdPrint( ("STAT: Entering IOCTL GetIntCount\n") );

/*
extension->testbuffer = NULL;
extension->testbuffer = (PUCHAR)ExAllocatePool(PagedPool, 20);
if (extension->testbuffer) {
    KdPrint( ("STAT: Pool Allocate Success\n") );
    ExFreePool(extension->testbuffer);
}
else {
    KdPrint( ("STAT: Pool Allocate Failure\n") );
}
*/

            Irp->IoStatus.Status = STATUS_SUCCESS;

            OutputBuffer = (PULONG) Irp->AssociatedIrp.SystemBuffer;

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(ULONG)) {

                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                goto Done;

            }

	    *OutputBuffer = extension->InterruptCount;
	    extension->InterruptCount = 0;

	    Irp->IoStatus.Information = sizeof( ULONG );

Done:
            status = Irp->IoStatus.Status;
            IoCompleteRequest( Irp, 0 );

            return status;

	case IOCTL_STAT_SET_TIMER:
	case IOCTL_STAT_GET_TIMER:
	case IOCTL_STAT_START_TIMER:
	case IOCTL_STAT_STOP_TIMER:
	case IOCTL_STAT_TEST_TIMER_KERNEL:

            //
            // These IOCTLs will interface with the device so we are going
            // to set up the device extension with some pertinent information
            // from the IRP, mark the IRP pending, and call our StartIO
            // procedure (via IoStartPacket).
            //

	    //
	    // indicate which function in the IOCTL information structure
	    //
	    extension->ioCtlCode = ioControlCode;

            if (extension->ioCtlCode == IOCTL_STAT_START_TIMER ||
                extension->ioCtlCode == IOCTL_STAT_SET_TIMER ||
                extension->ioCtlCode == IOCTL_STAT_GET_TIMER ) {
       	        if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(PIOCTL_INFORMATION)) {

                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
             	    break;
                }
            } else {
       	        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(PIOCTL_INFORMATION)) {

                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
             	    break;
                }
            }

            extension->IoctlInfo = (PIOCTL_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

	    IoMarkIrpPending(Irp);

	    IoStartPacket(DeviceObject, Irp, NULL, NULL);

	    return STATUS_PENDING;
    }
}

//
// This routine is called when we want to interface with the physical device.
// We synchronize access to the device with our ISR, to interface with the
// hardware synchronously.
//
VOID
statStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSTAT_DEVICE_EXTENSION extension;

    extension = DeviceObject->DeviceExtension;

    KeSynchronizeExecution(extension->InterruptObject,
			   statDoIo,
			   extension);

    //
    // only the IOCTL_STAT_SET_TIMER will generate an interrupt so for all
    // other cases, we can complete the IRP here.
    //
    if (extension->ioCtlCode != IOCTL_STAT_SET_TIMER) {
  	//
	// need to fill in this field to get the I/O manager to copy the data
	// back to user address space
	//
        if (extension->ioCtlCode == IOCTL_STAT_START_TIMER)
            Irp->IoStatus.Information = 0;
        else
	    Irp->IoStatus.Information = sizeof(IOCTL_INFORMATION);

	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoStartNextPacket(DeviceObject, FALSE);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

//
// This is where we actually 'talk' to the device. We know at this point that
// nothing else will be talking to the device at the same time because when
// we call KeSynchronizeExecution, the IRQL is raised to our interrupt IRQL
// before this routine is called, and a spinlock is acquired.
//
// Everything in here is device specific. This is where we initiate the I/O
// which could possibly generate an interrupt.
//
BOOLEAN
statDoIo(
    IN OUT PVOID Context
    )
{
    PSTAT_DEVICE_EXTENSION extension;
    PIOCTL_INFORMATION IoctlInfo;
    USHORT Input;

    extension = (PSTAT_DEVICE_EXTENSION) Context;
    IoctlInfo = extension->IoctlInfo;

    switch (extension->ioCtlCode) {

	case IOCTL_STAT_GET_TIMER:

            WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
                             (UCHAR) (DISARM+IoctlInfo->wTimerNumber));
	    WRITE_PORT_UCHAR (extension->PortAddress+INTERRUPT_PORT,
			      (UCHAR) 0);

	    break;

	case IOCTL_STAT_SET_TIMER:
	    //
	    // load timer 3 to generate interrupts
	    //
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T3MODE);

	    WRITE_PORT_USHORT( (PUSHORT)extension->PortAddress,
			       (USHORT) (IoctlInfo->wTimerSource +
                                        IoctlInfo->wTimerCycle +
                                        IoctlInfo->wTimerDirection +
                                        IoctlInfo->wTimerMode ));

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T3LOAD);
	    WRITE_PORT_USHORT((PUSHORT)extension->PortAddress,
			     (USHORT) 2000);

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) (LOAD+IoctlInfo->wTimerNumber));
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) CLEAR_OUT+TN_T3);

	    WRITE_PORT_UCHAR(extension->PortAddress+INTERRUPT_PORT,
			     (UCHAR) EN_OFF+EI_ON+IS_T3);

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) (ARM+IoctlInfo->wTimerNumber));
	    break;

	case IOCTL_STAT_START_TIMER:
	    //
	    // load timer 1
	    //
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T1MODE);
	    WRITE_PORT_USHORT((PUSHORT)extension->PortAddress,
			     (USHORT) TS_F1+TC_REPEAT+TD_UP);
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T1LOAD);
	    WRITE_PORT_USHORT((PUSHORT)extension->PortAddress,
			     (USHORT) 0);

	    //
	    // load timer 2 (cascaded with 1)
	    //
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T2MODE);
	    WRITE_PORT_USHORT((PUSHORT)extension->PortAddress,
			     (USHORT) TC_REPEAT+TD_UP);
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T2LOAD);
	    WRITE_PORT_USHORT((PUSHORT)extension->PortAddress,
			     (USHORT) 0);

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) LOAD+TF_T1+TF_T2);
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) ARM+TF_T1+TF_T2);

	    break;

	case IOCTL_STAT_STOP_TIMER:
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) DISARM+TF_T1+TF_T2);

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) SAVE+TF_T2+TF_T1);
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T2HOLD);
	    Input = READ_PORT_USHORT((PUSHORT)extension->PortAddress);

	    IoctlInfo->ElapsedTime = ((ULONG)Input << (sizeof(USHORT)*8));

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T1HOLD);
	    Input = READ_PORT_USHORT((PUSHORT)extension->PortAddress);

	    //
	    // Save value in structure
	    //
	    IoctlInfo->ElapsedTime += Input;

	    break;

        case IOCTL_STAT_TEST_TIMER_KERNEL:
	    //
	    // load timer 1
	    //
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T1MODE);
	    WRITE_PORT_USHORT((PUSHORT)extension->PortAddress,
			     (USHORT) TS_F1+TC_REPEAT+TD_UP);
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T1LOAD);
	    WRITE_PORT_USHORT((PUSHORT)extension->PortAddress,
			     (USHORT) 0);

	    //
	    // load timer 2 (cascaded with 1)
	    //
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T2MODE);
	    WRITE_PORT_USHORT((PUSHORT)extension->PortAddress,
			     (USHORT) TC_REPEAT+TD_UP);
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T2LOAD);
	    WRITE_PORT_USHORT((PUSHORT)extension->PortAddress,
			     (USHORT) 0);

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) LOAD+TF_T1+TF_T2);

            //
            // just arm the timer and immediately disarm to see minimum latency
            //
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) ARM+TF_T1+TF_T2);

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) DISARM+TF_T1+TF_T2);

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR) SAVE+TF_T2+TF_T1);
	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T2HOLD);
	    Input = READ_PORT_USHORT((PUSHORT)extension->PortAddress);

	    IoctlInfo->ElapsedTime = ((ULONG)Input << (sizeof(USHORT)*8));

	    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
			     (UCHAR)PTR_T1HOLD);
	    Input = READ_PORT_USHORT((PUSHORT)extension->PortAddress);

	    //
	    // Save value in structure
	    //
	    IoctlInfo->ElapsedTime += Input;

	    break;

    }
    return TRUE;
}

//
// This is our ISR
//
BOOLEAN
statInterruptServiceRoutine(
    IN PKINTERRUPT Interrupt,
    IN OUT PVOID Context
    )
{
    PDEVICE_OBJECT DeviceObject;
    PSTAT_DEVICE_EXTENSION extension;

    DeviceObject = Context;
    extension = DeviceObject->DeviceExtension;

    //
    // device specific stuff to dismiss the interrupt
    //
    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
		     (UCHAR) CLEAR_OUT+TN_T3);
    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
                     (UCHAR) DISARM+TF_T3);
    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
		     (UCHAR) LOAD+TF_T3);
    WRITE_PORT_UCHAR(extension->PortAddress+CONTROL_PORT,
		     (UCHAR) ARM+TF_T3);

    //
    // Note: the CurrentIrp field of the device object gets filled in
    // by IoStartPacket or IoStartNextPacket (which eventually leads
    // calling the drivers StartIo routine, which should lead to the
    // interrupt).
    //
    // If this interrupt was not the result of driver initiated I/O
    // then this field would not be accurate.
    //
    IoRequestDpc(DeviceObject,
		 DeviceObject->CurrentIrp,
		 NULL
		 );

    return TRUE;
}

//
// This is the deferred procedure call that gets queued by the ISR to
// finish any interrupt relate processing
//
VOID
statDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PDEVICE_OBJECT DeviceObject;
    PSTAT_DEVICE_EXTENSION extension;
    PIRP Irp;

    DeviceObject = DeferredContext;
    Irp = DeviceObject->CurrentIrp;
    extension = DeviceObject->DeviceExtension;
    extension->InterruptCount++;

    if (Irp) {
        //
        // need to fill in this field to get the I/O manager to copy the data
        // back to user address space
        //
        Irp->IoStatus.Information = sizeof(IOCTL_INFORMATION);
        Irp->IoStatus.Status = STATUS_SUCCESS;

        IoStartNextPacket(DeviceObject, FALSE);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return;
}

//
// In this routine, we are going to log the resources we are using, such as
// interrupts and ports, in the registry by calling IoReportResourceUsage.
//
// If there is a conflict with another driver, IoReportResourceUsage will
// return an appropriate value, the result in this driver will be that we
// do not load.
//
BOOLEAN
statReportUsage(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PHYSICAL_ADDRESS PortAddress,
    IN BOOLEAN *GotResources
    )
{

    ULONG sizeOfResourceList;
    PCM_RESOURCE_LIST resourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR nextFrd;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;


    //
    // The size of the resource list is going to be one full descriptor
    // which already has one partial descriptor included, plus another
    // partial descriptor. One partial descriptor will be for the
    // interrupt, and the other for the port addresses.
    //

    sizeOfResourceList = sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

    //
    // The full resource descriptor already contains one
    // partial.	Make room for one more.
    //
    // It will hold the irq "prd", and the port "prd".
    //	  ("prd" = partial resource descriptor)
    //

    sizeOfResourceList += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    //
    // Now we increment the length of the resource list by field offset
    // of the first frd.   This will give us the length of what preceeds
    // the first frd in the resource list.
    //	 (frd = full resource descriptor)
    //

    sizeOfResourceList += FIELD_OFFSET(
                              CM_RESOURCE_LIST,
                              List[0]
                              );

    resourceList = ExAllocatePool(
                       PagedPool,
                       sizeOfResourceList
                       );

    if (!resourceList) {

        return FALSE;

    }

    //
    // Zero out the list
    //

    RtlZeroMemory(
        resourceList,
        sizeOfResourceList
        );

    resourceList->Count = 1;
    nextFrd = &resourceList->List[0];

    nextFrd->InterfaceType = Isa;
    nextFrd->BusNumber = 0;

    //
    // We are going to report port addresses and interrupt
    //

    nextFrd->PartialResourceList.Count = 2;

    //
    // Now fill in the port data.  We don't wish to share
    // this port range with anyone.
    //
    // Note: the port address we pass in is the one we got
    // back from HalTranslateBusAddress.
    //

    partial = &nextFrd->PartialResourceList.PartialDescriptors[0];

    partial->Type = CmResourceTypePort;
    partial->ShareDisposition = CmResourceShareDriverExclusive;
    partial->Flags = CM_RESOURCE_PORT_IO;
    partial->u.Port.Start = PortAddress;
    partial->u.Port.Length = 4;

    partial++;

    //
    // Now fill in the irq stuff.
    //
    // Note: for IoReportResourceUsage, the Interrupt.Level and
    // Interrupt.Vector are bus-specific level and vector, just
    // as we passed in to HalGetInterruptVector, not the mapped
    // system vector we got back from HalGetInterruptVector.
    //

    partial->Type = CmResourceTypeInterrupt;
    partial->u.Interrupt.Level = STAT_IRQ;
    partial->u.Interrupt.Vector = STAT_IRQ;
    partial->ShareDisposition = CmResourceShareDriverExclusive;
    partial->Flags = CM_RESOURCE_INTERRUPT_LATCHED;

    IoReportResourceUsage(
        NULL,
        DriverObject,
        resourceList,
        sizeOfResourceList,
        NULL,
        NULL,
        0,
        FALSE,
	GotResources
        );

    //
    // The above routine sets the boolean the parameter
    // to TRUE if a conflict was detected.
    //

    *GotResources = !*GotResources;

    ExFreePool(resourceList);

    return *GotResources;

}
