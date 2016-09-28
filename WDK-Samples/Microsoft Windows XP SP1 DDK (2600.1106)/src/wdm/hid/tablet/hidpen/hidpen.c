/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        hidpen.c

    Abstract:
        Serial Pen Tablet HID Driver.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Mar-2000

    Revision History:
--*/

#include "pch.h"
#define MODULE_ID                       0

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(INIT, DriverEntry)
  #pragma alloc_text(PAGE, HpenCreateClose)
  #pragma alloc_text(PAGE, HpenSystemControl)
  #pragma alloc_text(PAGE, HpenAddDevice)
  #pragma alloc_text(PAGE, HpenUnload)
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
    DrvObj->MajorFunction[IRP_MJ_CLOSE] = HpenCreateClose;
    DrvObj->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = HpenSystemControl;

    DrvObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HpenInternalIoctl;

    DrvObj->MajorFunction[IRP_MJ_PNP]   = HpenPnp;
    DrvObj->MajorFunction[IRP_MJ_POWER] = HpenPower;
    DrvObj->DriverUnload                = HpenUnload;
    DrvObj->DriverExtension->AddDevice  = HpenAddDevice;

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
                 UNIQUE_ERRID(0x10),
                 NULL,
                 NULL);
        TRACEERROR(("failed to register mini driver (status=%d).\n", status));
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //DriverEntry

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | HpenCreateClose |
            Process the create and close IRPs sent to this device.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O Request Packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS EXTERNAL
HpenCreateClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    TRACEPROC("HpenCreateClose", 1)
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE ();
    UNREFERENCED_PARAMETER(DevObj);
    TRACEENTER(("(DevObj=%p,Irp=%p)\n", DevObj, Irp));

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    TRACEEXIT(("=%x\n", status));
    return status;
}       //HpenCreateClose

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | HpenSystemControl |
            Process the System Control IRPs sent to this device.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O Request Packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS EXTERNAL
HpenSystemControl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    TRACEPROC("HpenSystemControl", 1)
    NTSTATUS status;
    PDEVICE_EXTENSION devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    PAGED_CODE ();
    TRACEENTER(("(DevObj=%p,Irp=%p)\n", DevObj, Irp));

    status = IoAcquireRemoveLock(&devext->RemoveLock, Irp);
    if (!NT_SUCCESS(status))
    {
        //
        // Someone sent us another plug and play IRP after removed
        //
        LogError(ERRLOG_DEVICE_REMOVED,
                 status,
                 UNIQUE_ERRID(0x20),
                 NULL,
                 NULL);
        TRACEWARN(("received IRP after device was removed (status=%x).\n",
                   status));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        IoSkipCurrentIrpStackLocation(Irp);
        TRACEENTER((".IoCallDriver(DevObj=%p,Irp=%p)\n",
                    devext->LowerDevObj, Irp));
        status = IoCallDriver(devext->LowerDevObj, Irp);
        TRACEEXIT((".IoCallDriver=%x\n", status));
        IoReleaseRemoveLock(&devext->RemoveLock, Irp);
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //HpenSystemControl

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | HpenAddDevice |
            Called by hidclass, allows us to initialize our device extensions.

    @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.
    @parm   IN PDEVICE_OBJECT | DevObj |
            Points to a functional device object created by hidclass.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns NT status code.
--*/

NTSTATUS EXTERNAL
HpenAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT DevObj
    )
{
    TRACEPROC("HpenAddDevice", 1)
    NTSTATUS status;
    PDEVICE_EXTENSION devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    PAGED_CODE ();

    TRACEENTER(("(DrvObj=%p,DevObj=%p)\n", DrvObj, DevObj));

    TRACEASSERT(DevObj != NULL);
    UNREFERENCED_PARAMETER(DrvObj);

    RtlZeroMemory(devext, sizeof(*devext));
    devext->DevObj = DevObj;
    devext->pdo = GET_PDO(DevObj);
    devext->LowerDevObj = GET_NEXT_DEVICE_OBJECT(DevObj);
    IoInitializeRemoveLock(&devext->RemoveLock, HPEN_POOL_TAG, 0, 10);
    KeInitializeSpinLock(&devext->SpinLock);
    devext->PowerState = PowerDeviceD0;
    status = OemAddDevice(devext);
    if (NT_SUCCESS(status))
    {
        DevObj->Flags &= ~DO_DEVICE_INITIALIZING;
        DevObj->Flags |= DO_POWER_PAGABLE;
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //HpenAddDevice

/*++
    @doc    EXTERNAL

    @func   void | HpenUnload | Free all the allocated resources, etc.

    @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.

    @rvalue None.
--*/

VOID EXTERNAL
HpenUnload(
    IN PDRIVER_OBJECT DrvObj
    )
{
    TRACEPROC("HpenUnload", 1)

    PAGED_CODE();

    TRACEENTER(("(DrvObj=%p)\n", DrvObj));

    TRACEASSERT(DrvObj->DeviceObject == NULL);
    UNREFERENCED_PARAMETER(DrvObj);

    TRACEEXIT(("!\n"));
    return;
}       //HpenUnload

/*++
    @doc    INTERNAL

    @func   NTSTATUS | RegQueryValue | Read registry data.

    @parm   IN HANDLE | hkey | Registry handle.
    @parm   IN LPWSTR | pszValueName | Points to the value name string.
    @parm   IN ULONG | dwType | Registry data type.
    @parm   OUT PUCHAR | lpbData | To hold the value data.
    @parm   IN ULONG | dwcbData | Specifies the buffer size.
    @parm   OUT PULONG | pdwcbLen | To hold the require data length (optional).

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns NT status code.
--*/

NTSTATUS INTERNAL
RegQueryValue(
    IN  HANDLE hkey,
    IN  LPWSTR pszValueName,
    IN  ULONG  dwType,
    OUT PUCHAR lpbData,
    IN  ULONG  dwcbData,
    OUT PULONG pdwcbLen OPTIONAL
    )
{
    TRACEPROC("RegQueryValue", 3)
    NTSTATUS status;
    UNICODE_STRING ucstrName;
    PKEY_VALUE_PARTIAL_INFORMATION RegInfo;
    ULONG dwLen;

    PAGED_CODE ();
    TRACEENTER(("(hkey=%x,ValueName=%s,Type=%x,lpbData=%p,Len=%d,pdwLen=%p)\n",
                hkey, pszValueName, dwType, lpbData, dwcbData, pdwcbLen));

    TRACEASSERT(lpbData != NULL);
    RtlInitUnicodeString(&ucstrName, pszValueName);
    dwLen = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + dwcbData;
    RegInfo = ExAllocatePoolWithTag(PagedPool, dwLen, HPEN_POOL_TAG);
    if (RegInfo != NULL)
    {
        status = ZwQueryValueKey(hkey,
                                 &ucstrName,
                                 KeyValuePartialInformation,
                                 RegInfo,
                                 dwLen,
                                 &dwLen);
        if (NT_SUCCESS(status))
        {
            if ((dwType != REG_NONE) && (dwType != RegInfo->Type))
            {
                status = STATUS_OBJECT_TYPE_MISMATCH;
                LogError(ERRLOG_REGTYPE_MISMATCH,
                         status,
                         UNIQUE_ERRID(0x30),
                         NULL,
                         NULL);
                TRACEERROR(("registry data type mismatch (Type=%x,Expected=%x).\n",
                            RegInfo->Type, dwType));
            }
            else
            {
                TRACEASSERT(RegInfo->DataLength <= dwcbData);
                RtlCopyMemory(lpbData, RegInfo->Data, RegInfo->DataLength);
            }
        }

        if ((pdwcbLen != NULL) &&
            ((status == STATUS_SUCCESS) ||
             (status == STATUS_BUFFER_TOO_SMALL)))
        {
            *pdwcbLen = RegInfo->DataLength;
        }
        ExFreePool(RegInfo);
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(ERRLOG_INSUFFICIENT_RESOURCES,
                 status,
                 UNIQUE_ERRID(0x40),
                 NULL,
                 NULL);
        TRACEERROR(("failed to allocate registry info (len=%d).\n", dwcbData));
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //RegQueryValue
