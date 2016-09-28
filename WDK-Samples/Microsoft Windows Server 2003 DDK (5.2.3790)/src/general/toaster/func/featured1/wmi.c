/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    WMI.C

Abstract:

    This module handle all the WMI Irps.

Environment:

    Kernel mode

Revision History:

    Eliyas Yakub Oct 26, 1998

    Updated (9/28/2000) to demonstrate:

    a) How to bring power management tab in the device manager and
       enable the check boxes. I don't show what a driver specifically needs
       to do to support WAIT_WAKE or POWER_DEVICE_ENABLE. Check the
       keyboard class (kdbclass.c) driver source in the DDK to learn
       how to send/cancel WAIT_WAKE Irp.

    b) How to fire a WMI event to notify device arrival.


--*/


#include "toaster.h"
#include <initguid.h>
#include <wdmguid.h>
#include <wmistr.h>
#include <wmilib.h>
#include <stdio.h>

static PCHAR
WMIMinorFunctionString (
    UCHAR MinorFunction
);

static NTSTATUS
GetDeviceFriendlyName(
    IN PDEVICE_OBJECT Pdo,
    IN OUT PUNICODE_STRING DeviceName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ToasterWmiRegistration)
#pragma alloc_text(PAGE,ToasterWmiDeRegistration)
#pragma alloc_text(PAGE,ToasterSystemControl)
#pragma alloc_text(PAGE,ToasterSetWmiDataItem)
#pragma alloc_text(PAGE,ToasterSetWmiDataBlock)
#pragma alloc_text(PAGE,ToasterQueryWmiDataBlock)
#pragma alloc_text(PAGE,ToasterQueryWmiRegInfo)
#endif

#define MOFRESOURCENAME L"ToasterWMI"

#define NUMBER_OF_WMI_GUIDS             4
#define WMI_TOASTER_DRIVER_INFORMATION  0
#define TOASTER_NOTIFY_DEVICE_ARRIVAL   1
#define WMI_POWER_DEVICE_WAKE_ENABLE    2 // wait-wake
#define WMI_POWER_DEVICE_ENABLE         3

WMIGUIDREGINFO ToasterWmiGuidList[] =
{
    {
        &TOASTER_WMI_STD_DATA_GUID, // Toaster device information
        1,
        0
    },
    {
        &TOASTER_NOTIFY_DEVICE_ARRIVAL_EVENT,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },
    //
    // If you handle the following two GUIDs, the device manager will
    // show the power-management tab in the device properites dialog.
    //
    {
        &GUID_POWER_DEVICE_WAKE_ENABLE, // wait-wake
        1,
        0
    },
    {
        &GUID_POWER_DEVICE_ENABLE, // the meaning of this request
        1,                         // is up to the device
        0
    }
};

//
// Global debug error level
//

extern ULONG DebugLevel;

NTSTATUS
ToasterWmiRegistration(
    PFDO_DATA               FdoData
)
/*++
Routine Description

    Registers with WMI as a data provider for this
    instance of the device

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    FdoData->WmiLibInfo.GuidCount = sizeof (ToasterWmiGuidList) /
                                 sizeof (WMIGUIDREGINFO);
    ASSERT (NUMBER_OF_WMI_GUIDS == FdoData->WmiLibInfo.GuidCount);
    FdoData->WmiLibInfo.GuidList = ToasterWmiGuidList;
    FdoData->WmiLibInfo.QueryWmiRegInfo = ToasterQueryWmiRegInfo;
    FdoData->WmiLibInfo.QueryWmiDataBlock = ToasterQueryWmiDataBlock;
    FdoData->WmiLibInfo.SetWmiDataBlock = ToasterSetWmiDataBlock;
    FdoData->WmiLibInfo.SetWmiDataItem = ToasterSetWmiDataItem;
    FdoData->WmiLibInfo.ExecuteWmiMethod = NULL;
    FdoData->WmiLibInfo.WmiFunctionControl = ToasterFunctionControl;

    //
    // Register with WMI
    //

    status = IoWMIRegistrationControl(FdoData->Self,
                             WMIREG_ACTION_REGISTER
                             );

    //
    // Initialize the Std device data structure
    //

    FdoData->StdDeviceData.ConnectorType = TOASTER_WMI_STD_USB;
    FdoData->StdDeviceData.Capacity = 2000;
    FdoData->StdDeviceData.ErrorCount = 0;
    FdoData->StdDeviceData.Controls = 5;
    FdoData->StdDeviceData.DebugPrintLevel = DebugLevel;

    return status;

}

NTSTATUS
ToasterWmiDeRegistration(
    PFDO_DATA               FdoData
)
/*++
Routine Description

     Inform WMI to remove this DeviceObject from its
     list of providers. This function also
     decrements the reference count of the deviceobject.

--*/
{

    PAGED_CODE();

    return IoWMIRegistrationControl(FdoData->Self,
                                 WMIREG_ACTION_DEREGISTER
                                 );

}

NTSTATUS
ToasterSystemControl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description

    We have just received a System Control IRP.

    Assume that this is a WMI IRP and
    call into the WMI system library and let it handle this IRP for us.

--*/
{
    PFDO_DATA               fdoData;
    SYSCTL_IRP_DISPOSITION  disposition;
    NTSTATUS                status;
    PIO_STACK_LOCATION      stack;

    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation (Irp);

    ToasterDebugPrint(TRACE, "FDO %s\n",
                WMIMinorFunctionString(stack->MinorFunction));

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterIoIncrement (fdoData);

    if (fdoData->DevicePnPState == Deleted) {
        Irp->IoStatus.Status = status = STATUS_NO_SUCH_DEVICE ;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        ToasterIoDecrement (fdoData);
        return status;
    }

    status = WmiSystemControl(&fdoData->WmiLibInfo,
                                 DeviceObject,
                                 Irp,
                                 &disposition);
    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
            break;
        }

        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targeted
            // at a device lower in the stack.
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (fdoData->NextLowerDriver, Irp);
            break;
        }

        default:
        {
            //
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (fdoData->NextLowerDriver, Irp);
            break;
        }
    }

    ToasterIoDecrement(fdoData);

    return(status);
}

//
// WMI System Call back functions
//

NTSTATUS
ToasterSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    PFDO_DATA   fdoData;
    NTSTATUS    status;
    BOOLEAN     waitWakeEnabled;

    PAGED_CODE();

    ToasterDebugPrint(TRACE, "Entered ToasterSetWmiDataItem\n");

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    switch(GuidIndex) {

    case WMI_TOASTER_DRIVER_INFORMATION:
        if(DataItemId == 5)
        {
            if (BufferSize < sizeof(ULONG)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
           DebugLevel = fdoData->StdDeviceData.DebugPrintLevel =
                                    *((PULONG)Buffer);
           status = STATUS_SUCCESS;
        }
        else
            status = STATUS_WMI_READ_ONLY;
        break;

    case WMI_POWER_DEVICE_WAKE_ENABLE:

        //
        // If the wait-wake state is true, we send a IRP_MN_WAIT_WAKE irp to the
        // bus driver. If it's FALSE, we cancel it.
        // Every time the user changes his preference,
        // we write that to the registry to carry over his preference across boot.
        // If the user tries to enable wait-wake, and if the driver stack is not
        // able to do, we will reset the value in the registry to indicate that
        // the device is incapable of wait-waking the system.
        // Note: Featured2 WMI.C implements this.
        if (BufferSize < sizeof(BOOLEAN)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }        
        fdoData->AllowWakeArming = *(PBOOLEAN) Buffer;
        status = STATUS_SUCCESS;
        break;

    case WMI_POWER_DEVICE_ENABLE:
        //
        // Meaning of this request ("Allow the computer to turn off this
        // device to save power") is device dependent. For example NDIS
        // driver interpretation of this checkbox is different from Serial
        // class drivers.
        //
        if (BufferSize < sizeof(BOOLEAN)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }        
        fdoData->AllowIdleDetectionRegistration = *(PBOOLEAN) Buffer;
        status = STATUS_SUCCESS;
        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  0,
                                  IO_NO_INCREMENT);

    return status;
}

NTSTATUS
ToasterSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/
{
    PFDO_DATA   fdoData;
    NTSTATUS    status;
    BOOLEAN     waitWakeEnabled;

    PAGED_CODE();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    ToasterDebugPrint(TRACE, "Entered ToasterSetWmiDataBlock\n");

    switch(GuidIndex) {
    case WMI_TOASTER_DRIVER_INFORMATION:

        //
        // We will update only writable elements.
        //
        if (BufferSize < sizeof(TOASTER_WMI_STD_DATA)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }        
        DebugLevel = fdoData->StdDeviceData.DebugPrintLevel =
                    ((PTOASTER_WMI_STD_DATA)Buffer)->DebugPrintLevel;

        status = STATUS_SUCCESS;

        break;


    case WMI_POWER_DEVICE_WAKE_ENABLE:

        //
        // If the wait-wake state is true (box is checked), we send a IRP_MN_WAIT_WAKE
        //  irp to the bus driver. If it's FALSE, we cancel it.
        // Every time the user changes his preference,
        // we write that to the registry to carry over his preference across boot.
        // If the user tries to enable wait-wake, and if the driver stack is not
        // able to do, it will reset the value in the registry to indicate that
        // the device is incapable of wait-waking the system. Actuall implement
        // of this is shown in the next version of the toaster (featured2).
        //
        if (BufferSize < sizeof(BOOLEAN)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }        
        fdoData->AllowWakeArming = *(PBOOLEAN) Buffer;
        status = STATUS_SUCCESS;
        break;

    case WMI_POWER_DEVICE_ENABLE:
        if (BufferSize < sizeof(BOOLEAN)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }        
        fdoData->AllowIdleDetectionRegistration = *(PBOOLEAN) Buffer;
        status = STATUS_SUCCESS;
        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  0,
                                  IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
ToasterQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    InstanceCount is the number of instances expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fulfill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    PFDO_DATA               fdoData;
    NTSTATUS    status;
    ULONG       size = 0;
    WCHAR       modelName[]=L"Aishwarya\0\0";
    USHORT      modelNameLen = (wcslen(modelName) + 1) * sizeof(WCHAR);

    PAGED_CODE();

    ToasterDebugPrint(TRACE, "Entered ToasterQueryWmiDataBlock\n");

    //
    // Only ever registers 1 instance per guid
    ASSERT((InstanceIndex == 0) &&
           (InstanceCount == 1));

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    switch (GuidIndex) {
    case WMI_TOASTER_DRIVER_INFORMATION:

        size = sizeof (TOASTER_WMI_STD_DATA) + modelNameLen + sizeof(USHORT);
        if (OutBufferSize < size ) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        //
        // Copy the structure information
        //
        * (PTOASTER_WMI_STD_DATA) Buffer = fdoData->StdDeviceData;
        Buffer += sizeof (TOASTER_WMI_STD_DATA);

        //
        // Copy the string. Put length of string ahead of string.
        //
        *((PUSHORT)Buffer) = modelNameLen ;
        Buffer = (PUCHAR)Buffer + sizeof(USHORT);// advance past string length
        RtlCopyBytes((PVOID)Buffer, (PVOID)modelName, modelNameLen);

        *InstanceLengthArray = size ;
        status = STATUS_SUCCESS;
        break;


    case WMI_POWER_DEVICE_WAKE_ENABLE:
        //
        // Here we return the current preference of the user for wait-waking
        // the system. We read(IoOpenDeviceRegistryKey/ZwQueryValueKey)
        // the default value written by the INF file in the HW registery.
        // If the user changes his preference, then we must record
        // the changes in the registry to have that in affect across
        // boots.
        // Note: Featured2 WMI.C implements this.


        size = sizeof(BOOLEAN);

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        *(PBOOLEAN) Buffer = fdoData->AllowWakeArming;
        *InstanceLengthArray = size;
        status = STATUS_SUCCESS;
        break;

    case WMI_POWER_DEVICE_ENABLE:
        //
        // The same rule discussed above applies here
        // for responding to the query.
        //
        size = sizeof(BOOLEAN);

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        *(PBOOLEAN) Buffer = fdoData->AllowIdleDetectionRegistration;
        *InstanceLengthArray = size;
        status = STATUS_SUCCESS;
        break;
    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  size,
                                  IO_NO_INCREMENT);

    return status;
}

NTSTATUS
ToasterFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine is a callback into the driver to enabled or disable event
    generation or data block collection. A device should only expect a
    single enable when the first event or data consumer enables events or
    data collection and a single disable when the last event or data
    consumer disables events or data collection. Data blocks will only
    receive collection enable/disable if they were registered as requiring
    it. If the driver can complete enabling/disabling within the callback it
    should call WmiCompleteRequest to complete the irp before returning to
    the caller. Or the driver can return STATUS_PENDING if the irp cannot be
    completed immediately and must then call WmiCompleteRequest once the
    data is changed.

Arguments:

    DeviceObject is the device object

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function specifies which functionality is being enabled or disabled

    Enable is TRUE then the function is being enabled else disabled

Return Value:

    status

--*/
{
    NTSTATUS status;

    //
    // Normally we should save the Enable value in our
    // device extension, and check that before firing an
    // event. Since we are firing an event to indicate
    // device arrival, we can't wait for the WMI to send us
    // enable IRP, so we don't bother to check that.
    //

    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     STATUS_SUCCESS,
                                     0,
                                     IO_NO_INCREMENT);


    return(status);
}

NTSTATUS ToasterFireArrivalEvent(
    IN PDEVICE_OBJECT DeviceObject
    )
{

    PWNODE_SINGLE_INSTANCE  wnode;
    ULONG                   wnodeSize;
    ULONG                   wnodeDataBlockSize;
    ULONG                   wnodeInstanceNameSize;
    PUCHAR                  ptmp;
    ULONG                   size;
    UNICODE_STRING          deviceName;
    UNICODE_STRING          modelName;
    NTSTATUS                status;
    PFDO_DATA               fdoData = DeviceObject->DeviceExtension;

    RtlInitUnicodeString(&modelName, L"Sonali\0\0");
    //
    // Get the device name.
    //
    status = GetDeviceFriendlyName (fdoData->UnderlyingPDO,
                                        &deviceName);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Determine the amount of wnode information we need.
    //
    wnodeSize = sizeof(WNODE_SINGLE_INSTANCE);
    wnodeInstanceNameSize = deviceName.Length + sizeof(USHORT);
    wnodeDataBlockSize = modelName.Length + sizeof(USHORT);

    size = wnodeSize + wnodeInstanceNameSize + wnodeDataBlockSize;

    //
    // Allocate memory for the WNODE from NonPagedPool
    //
    wnode = ExAllocatePoolWithTag (NonPagedPool, size, TOASTER_POOL_TAG);

    if (NULL != wnode) {
        RtlZeroMemory(wnode, size);

        wnode->WnodeHeader.BufferSize = size;
        wnode->WnodeHeader.ProviderId =
                            IoWMIDeviceObjectToProviderId(DeviceObject);
        wnode->WnodeHeader.Version = 1;
        KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);

        RtlCopyMemory(&wnode->WnodeHeader.Guid,
                        &TOASTER_NOTIFY_DEVICE_ARRIVAL_EVENT,
                        sizeof(GUID));

        //
        // Set flags to indicate that you are creating dynamic instance names.
        // The reason we chose to do dynamic instance is becuase we can fire
        // the events anytime. If we do static instance names, we can only
        // fire events after WMI queries for IRP_MN_REGINFO, which happens
        // after IRP_MN_START_DEVICE. Another point to note is that if you
        // are firing an event after the device is started, it would be a
        // good idea to check whether the event is enabled. Becuase that
        // shows there is somebody interested in receiving the event.
        // Why fire an event when nobody is interested and waste
        // system resources.
        //

        wnode->WnodeHeader.Flags = WNODE_FLAG_EVENT_ITEM |
                                    WNODE_FLAG_SINGLE_INSTANCE;

        wnode->OffsetInstanceName = wnodeSize;
        wnode->DataBlockOffset = wnodeSize + wnodeInstanceNameSize;
        wnode->SizeDataBlock = wnodeDataBlockSize;

        //
        // Get a pointer to the start of the instance name block.
        //
        ptmp = (PUCHAR)wnode + wnode->OffsetInstanceName;

        //
        // Save the number of elements in the first ULONG.
        //
        *((PUSHORT)ptmp) = deviceName.Length;

        //
        // Copy the data after the number of elements.
        //
        RtlCopyMemory(ptmp + sizeof(USHORT),
                            deviceName.Buffer,
                            deviceName.Length);

        //
        // Get a pointer to the start of the data block.
        //
        ptmp = (PUCHAR)wnode + wnode->DataBlockOffset;

        //
        // Save the number of elements in the first ULONG.
        //
        *((PUSHORT)ptmp) = modelName.Length;

        //
        // Copy the data after the number of elements.
        //
        RtlCopyMemory(ptmp + sizeof(USHORT),
                        modelName.Buffer,
                        modelName.Length);

        //
        // Indicate the event to WMI. WMI will take care of freeing
        // the WMI struct back to pool.
        //
        status = IoWMIWriteEvent(wnode);
        if (!NT_SUCCESS(status)) {
            ToasterDebugPrint(ERROR, "IoWMIWriteEvent failed %x\n", status);
            ExFreePool(wnode);
        }
    }else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Free the memory allocated by GetDeviceFriendlyName function.
    //
    ExFreePool(deviceName.Buffer);
    return status;
}

NTSTATUS
ToasterQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    WmiCompleteRequest.

Arguments:

    DeviceObject is the device whose data block is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver

    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is returned in
        *RegFlags.

Return Value:

    status

--*/
{
    PFDO_DATA fdoData;

    PAGED_CODE();

    ToasterDebugPrint(TRACE, "Entered ToasterQueryWmiRegInfo\n");

    fdoData = DeviceObject->DeviceExtension;

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &Globals.RegistryPath;
    *Pdo = fdoData->UnderlyingPDO;
    RtlInitUnicodeString(MofResourceName, MOFRESOURCENAME);

    return STATUS_SUCCESS;
}

NTSTATUS
GetDeviceFriendlyName(
    IN PDEVICE_OBJECT Pdo,
    IN OUT PUNICODE_STRING DeviceName
    )
/*++

Routine Description:

    Return the friendly name associated with the given device object.

Arguments:

Return Value:

    NT status

--*/
{
    NTSTATUS                    status;
    ULONG                       resultLength = 0;
    DEVICE_REGISTRY_PROPERTY    property;
    PWCHAR                      valueInfo = NULL;
    ULONG                       i;

    valueInfo = NULL;

    //
    // First get the length of the string. If the FriendlyName
    // is not there then get the lenght of device description.
    //
    property = DevicePropertyFriendlyName;

    for (i = 0; i < 2; i++) {
        status = IoGetDeviceProperty(Pdo,
                                       property,
                                       0,
                                       NULL,
                                       &resultLength);

        if (status != STATUS_BUFFER_TOO_SMALL) {
            property = DevicePropertyDeviceDescription;
        }
    }

    valueInfo = ExAllocatePoolWithTag (NonPagedPool, resultLength,
                                                TOASTER_POOL_TAG);
    if (NULL == valueInfo) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Now we have the right size buffer, we can get the name
    //
    status = IoGetDeviceProperty(Pdo,
                                   property,
                                   resultLength,
                                   valueInfo,
                                   &resultLength);
    if (!NT_SUCCESS(status)) {
        ToasterDebugPrint(ERROR, "IoGetDeviceProperty returned %x\n", status);
        goto Error;
    }

    RtlInitUnicodeString(DeviceName, valueInfo);

    return STATUS_SUCCESS;

Error:
    if (valueInfo) {
        ExFreePool(valueInfo);
    }
    return (status);
}


PCHAR
WMIMinorFunctionString (
    UCHAR MinorFunction
)
{
    switch (MinorFunction)
    {
        case IRP_MN_CHANGE_SINGLE_INSTANCE:
            return "IRP_MN_CHANGE_SINGLE_INSTANCE";
        case IRP_MN_CHANGE_SINGLE_ITEM:
            return "IRP_MN_CHANGE_SINGLE_ITEM";
        case IRP_MN_DISABLE_COLLECTION:
            return "IRP_MN_DISABLE_COLLECTION";
        case IRP_MN_DISABLE_EVENTS:
            return "IRP_MN_DISABLE_EVENTS";
        case IRP_MN_ENABLE_COLLECTION:
            return "IRP_MN_ENABLE_COLLECTION";
        case IRP_MN_ENABLE_EVENTS:
            return "IRP_MN_ENABLE_EVENTS";
        case IRP_MN_EXECUTE_METHOD:
            return "IRP_MN_EXECUTE_METHOD";
        case IRP_MN_QUERY_ALL_DATA:
            return "IRP_MN_QUERY_ALL_DATA";
        case IRP_MN_QUERY_SINGLE_INSTANCE:
            return "IRP_MN_QUERY_SINGLE_INSTANCE";
        case IRP_MN_REGINFO:
            return "IRP_MN_REGINFO";
        default:
            return "unknown_syscontrol_irp";
    }
}



