/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        HidBtn.c

    Abstract:
        Tablet PC Buttons HID Driver.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Apr-2000

    Revision History:
--*/

#include "pch.h"
#define MODULE_ID                       0

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(INIT, DriverEntry)
  #pragma alloc_text(PAGE, HbtnCreateClose)
  #pragma alloc_text(PAGE, HbtnAddDevice)
  #pragma alloc_text(PAGE, HbtnUnload)
#endif  //ifdef ALLOC_PRAGMA

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | DriverEntry |
            Installable driver initialization entry point.
            <nl>This entry point is called directly by the I/O system.

    @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.
    @parm   IN PUNICODE_STRINT | RegPath | Points to the registry path.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS EXTERNAL
DriverEntry(
    IN PDRIVER_OBJECT  DrvObj,
    IN PUNICODE_STRING RegPath
    )
{
    TRACEPROC("DriverEntry", 1)
    NTSTATUS status = STATUS_SUCCESS;
    HID_MINIDRIVER_REGISTRATION hidMinidriverRegistration;

    TRACEENTER(("(DrvObj=%p,RegPath=%p)\n", DrvObj, RegPath));

    TRACEINIT(0, 0);
    gDriverObj = DrvObj;
    DrvObj->MajorFunction[IRP_MJ_CREATE] =
    DrvObj->MajorFunction[IRP_MJ_CLOSE] = HbtnCreateClose;

    DrvObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HbtnInternalIoctl;

    DrvObj->MajorFunction[IRP_MJ_PNP]   = HbtnPnp;
    DrvObj->MajorFunction[IRP_MJ_POWER] = HbtnPower;
    DrvObj->DriverUnload                = HbtnUnload;
    DrvObj->DriverExtension->AddDevice  = HbtnAddDevice;

    //
    // Register with HIDCLASS.SYS module
    //
    RtlZeroMemory(&hidMinidriverRegistration,
                  sizeof(hidMinidriverRegistration));

    hidMinidriverRegistration.Revision            = HID_REVISION;
    hidMinidriverRegistration.DriverObject        = DrvObj;
    hidMinidriverRegistration.RegistryPath        = RegPath;
    hidMinidriverRegistration.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
    hidMinidriverRegistration.DevicesArePolled    = FALSE;

    status = HidRegisterMinidriver(&hidMinidriverRegistration);

    if (!NT_SUCCESS(status))
    {
        LogError(ERRLOG_MINIDRV_REG_FAILED,
                 status,
                 UNIQUE_ERRID(0x10), NULL, NULL);
        TRACEERROR(("failed to register mini driver.\n"));
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //DriverEntry

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | HbtnCreateClose |
            Process the create and close IRPs sent to this device.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O Request Packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS EXTERNAL
HbtnCreateClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    TRACEPROC("HbtnCreateClose", 1)
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpsp;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER(DevObj);
    irpsp = IoGetCurrentIrpStackLocation(Irp);

    TRACEENTER(("(DevObj=%p,Irp=%p,IrpStack=%p,Major=%s)\n",
                DevObj, Irp, irpsp,
                LookupName(irpsp->MajorFunction, MajorIrpNames)));

    switch(irpsp->MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            Irp->IoStatus.Information = 0;
            break;

        default:
            TRACEASSERT(FALSE);
            status = STATUS_INVALID_PARAMETER;
            break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    TRACEEXIT(("=%x\n", status));
    return status;
}       //HbtnCreateClose

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | HbtnAddDevice |
            Called by hidclass, allows us to initialize our device extensions.

    @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.
    @parm   IN PDEVICE_OBJECT | DevObj |
            Points to a functional device object created by hidclass.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns NT status code.
--*/

NTSTATUS EXTERNAL
HbtnAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT DevObj
    )
{
    TRACEPROC("HbtnAddDevice", 1)
    NTSTATUS status;
    PDEVICE_EXTENSION devext;

    PAGED_CODE ();
    TRACEENTER(("(DrvObj=%p,DevObj=%p)\n", DrvObj, DevObj));
    TRACEASSERT(DevObj != NULL);
    UNREFERENCED_PARAMETER(DrvObj);

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);
    RtlZeroMemory(devext, sizeof(*devext));
    devext->self = DevObj;
    devext->LowerDevObj = GET_NEXT_DEVICE_OBJECT(DevObj);
    IoInitializeRemoveLock(&devext->RemoveLock, HBTN_POOL_TAG, 0, 10);
    KeInitializeSpinLock(&devext->SpinLock);
    InitializeListHead(&devext->PendingIrpList);
    status = OemAddDevice(devext);
    if (NT_SUCCESS(status))
    {
        DevObj->Flags &= ~DO_DEVICE_INITIALIZING;
        DevObj->Flags |= DO_POWER_PAGABLE;
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //HbtnAddDevice

/*++
    @doc    EXTERNAL

    @func   void | HbtnUnload | Free all the allocated resources, etc.

    @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.

    @rvalue None.
--*/

VOID EXTERNAL
HbtnUnload(
    IN PDRIVER_OBJECT DrvObj
    )
{
    TRACEPROC("HbtnUnload", 1)

    PAGED_CODE();

    TRACEENTER(("(DrvObj=%p)\n", DrvObj));

    TRACEASSERT(DrvObj->DeviceObject == NULL);
    UNREFERENCED_PARAMETER(DrvObj);

    TRACEEXIT(("!\n"));
    return;
}       //HbtnUnload
