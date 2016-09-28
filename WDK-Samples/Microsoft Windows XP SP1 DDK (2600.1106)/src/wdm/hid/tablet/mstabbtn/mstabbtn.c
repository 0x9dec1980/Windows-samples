/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        MSTabBtn.c

    Abstract:
        Contains OEM specific functions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Apr-2000

    Revision History:
--*/

#include "pch.h"
#define MODULE_ID                       3

UCHAR gReportDescriptor[] = {
    //
    // Tablet PC Buttons
    //
    0x05, 0x01,                 // USAGE_PAGE (Generic Desktop)             0
    0x09, 0x09,                 // USAGE (TabletPC Buttons)                 2
    0xa1, 0x01,                 // COLLECTION (Application)                 4
    0x85, REPORTID_BTN,         //   REPORT_ID (Btn)                        6
    0x05, 0x09,                 //   USAGE_PAGE (Button)                    8
    0x19, 0x01,                 //   USAGE_MINIMUM (Button 1)               10
    0x29, 0x20,                 //   USAGE_MAXIMUM (Button 32)              12
    0x15, 0x00,                 //   LOGICAL_MINIMUM (0)                    14
    0x25, 0x01,                 //   LOGICAL_MAXIMUM (1)                    16
    0x75, 0x01,                 //   REPORT_SIZE (1)                        18
    0x95, 0x20,                 //   REPORT_COUNT (32)                      20
    0x81, 0x02,                 //   INPUT (Data,Var,Abs)                   22
    0xc0,                       // END_COLLECTION                           28/29
    //
    // SAS Button
    //
    0x05, 0x01,                 // USAGE_PAGE (Generic Desktop)             0
    0x09, 0x06,                 // USAGE (Keyboard)                         2
    0xa1, 0x01,                 // COLLECTION (Application)                 4
    0x85, REPORTID_KBD,         //   REPORT_ID (Kbd)                        6
    0x05, 0x07,                 //   USAGE_PAGE (Key Codes)                 8
    0x09, 0x4c,                 //   USAGE (Del Key)                        10
    0x09, 0xe0,                 //   USAGE (Left Ctrl Key)                  12
    0x09, 0xe2,                 //   USAGE (Left Alt Key)                   14
    0x15, 0x00,                 //   LOGICAL_MINIMUM (0)                    16
    0x25, 0x01,                 //   LOGICAL_MAXIMUM (1)                    18
    0x75, 0x01,                 //   REPORT_SIZE (1)                        20
    0x95, 0x03,                 //   REPORT_COUNT (3)                       22
    0x81, 0x02,                 //   INPUT (Data,Var,Abs)                   24
    0x95, 0x1d,                 //   REPORT_COUNT (29)                      26
    0x81, 0x03,                 //   INPUT (Cnst,Var,Abs)                   28
    0xc0                        // END_COLLECTION                           30/31
};
ULONG gdwcbReportDescriptor = sizeof(gReportDescriptor);

HID_DESCRIPTOR gHidDescriptor =
{
    sizeof(HID_DESCRIPTOR),             //bLength
    HID_HID_DESCRIPTOR_TYPE,            //bDescriptorType
    HID_REVISION,                       //bcdHID
    0,                                  //bCountry - not localized
    1,                                  //bNumDescriptors
    {                                   //DescriptorList[0]
        HID_REPORT_DESCRIPTOR_TYPE,     //bReportType
        sizeof(gReportDescriptor)       //wReportLength
    }
};

PWSTR gpwstrManufacturerID = L"Microsoft";
PWSTR gpwstrProductID = L"TabletPC Buttons";
PWSTR gpwstrSerialNumber = L"0";

//
// Local function prototypes.
//
BOOLEAN EXTERNAL
OemInterruptServiceRoutine(
    IN PKINTERRUPT       Interrupt,
    IN PDEVICE_EXTENSION DevExt
    );

VOID EXTERNAL
OemButtonDebounceDpc(
    IN PKDPC             Dpc,
    IN PDEVICE_EXTENSION DevExt,
    IN PVOID             SysArg1,
    IN PVOID             SysArg2
    );

BOOLEAN INTERNAL
SendButtonReport(
    IN PDEVICE_EXTENSION DevExt,
    IN UCHAR             bReportID,
    IN ULONG             dwCurButtons
    );

NTSTATUS INTERNAL
RegQueryValue(
    IN  HANDLE hkey,
    IN  LPWSTR pszValueName,
    IN  ULONG  dwType,
    OUT PUCHAR lpbData,
    IN  ULONG  dwcbData,
    OUT PULONG pdwcbLen OPTIONAL
    );

PCM_PARTIAL_RESOURCE_DESCRIPTOR INTERNAL
RtlUnpackPartialDesc(
    IN UCHAR             ResType,
    IN PCM_RESOURCE_LIST ResList,
    IN OUT PULONG        Count
    );

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, OemAddDevice)
  #pragma alloc_text(PAGE, OemStartDevice)
  #pragma alloc_text(PAGE, OemRemoveDevice)
  #pragma alloc_text(PAGE, RegQueryValue)
  #pragma alloc_text(PAGE, RtlUnpackPartialDesc)
#endif  //ifdef ALLOC_PRAGMA

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemAddDevice | OEM specific AddDevice code.

    @parm   IN PDEVICE_EXTENSION | devext | Points to the device extension.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns NT status code.
--*/

NTSTATUS INTERNAL
OemAddDevice(
    IN PDEVICE_EXTENSION devext
    )
{
    TRACEPROC("OemAddDevice", 2)
    NTSTATUS status;
    HANDLE hkey;

    PAGED_CODE ();
    TRACEENTER(("(devext=%p)\n", devext));

    devext->OemExtension.DebounceTime.QuadPart =
        Int32x32To64(OEM_BUTTON_DEBOUNCE_TIME,-10000);
    KeInitializeTimer(&devext->OemExtension.DebounceTimer);
    KeInitializeDpc(&devext->OemExtension.TimerDpc,
                    OemButtonDebounceDpc,
                    devext);

    status = IoOpenDeviceRegistryKey(GET_PDO(devext->self),
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_READ,
                                     &hkey);
    if (NT_SUCCESS(status))
    {
        status = RegQueryValue(hkey,
                               L"SASButtonID",
                               REG_DWORD,
                               (PUCHAR)&devext->OemExtension.dwSASButtonID,
                               sizeof(devext->OemExtension.dwSASButtonID),
                               NULL);
        if (NT_SUCCESS(status))
        {
            TRACEINFO(3, ("SASButtonID=%x\n",
                          devext->OemExtension.dwSASButtonID));
            if ((devext->OemExtension.dwSASButtonID &
                 (devext->OemExtension.dwSASButtonID^
                  (-(LONG)devext->OemExtension.dwSASButtonID))) != 0)
            {
                //
                // We have more than one bit set in SASButtonID, that's a
                // no-no.
                //
                status = STATUS_INVALID_PARAMETER;
                LogError(ERRLOG_INVALID_CONFIGURATION,
                         status,
                         UNIQUE_ERRID(0x10), NULL, NULL);
                TRACEWARN(("Invalid data (SASButtonID=%x).\n",
                           devext->OemExtension.dwSASButtonID));
                devext->OemExtension.dwSASButtonID = 0;
            }
        }
        else
        {
            LogError(ERRLOG_READREG_FAILED,
                     status,
                     UNIQUE_ERRID(0x20), NULL, NULL);
            TRACEWARN(("failed to read registry value SASButtonID.\n"));
        }
        ZwClose(hkey);
    }
    else
    {
        LogError(ERRLOG_OPENREG_FAILED,
                 status,
                 UNIQUE_ERRID(0x30), NULL, NULL);
        TRACEWARN(("failed to open device registry key (status=%x).\n",
                   status));
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemAddDevice

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemStartDevice |
            Get the device information and attempt to initialize a
            configuration for a device.  If we cannot identify this as a
            valid HID device or configure the device, our start device
            function is failed.

    @parm   IN PDEVICE_EXTENSION | devext | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS INTERNAL
OemStartDevice(
    IN PDEVICE_EXTENSION devext,
    IN PIRP              Irp
    )
{
    TRACEPROC("OemStartDevice", 2)
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpsp;

    PAGED_CODE();
    TRACEENTER(("(devext=%p,Irp=%p)\n", devext, Irp));

    irpsp = IoGetCurrentIrpStackLocation(Irp);
    if (irpsp->Parameters.StartDevice.AllocatedResourcesTranslated == NULL)
    {
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        LogError(ERRLOG_INVALID_CONFIGURATION,
                 status,
                 UNIQUE_ERRID(0x40),
                 NULL,
                 NULL);
        TRACEERROR(("no allocated resources.\n"));
    }
    else
    {
        ULONG Count;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR pDesc;

        Count = 0;
        pDesc = RtlUnpackPartialDesc(
                    CmResourceTypePort,
                    irpsp->Parameters.StartDevice.AllocatedResourcesTranslated,
                    &Count);
        if (pDesc == NULL)
        {
            status = STATUS_DEVICE_CONFIGURATION_ERROR;
            LogError(ERRLOG_INVALID_CONFIGURATION,
                     status,
                     UNIQUE_ERRID(0x50),
                     NULL,
                     NULL);
            TRACEERROR(("no allocated port resources.\n"));
        }
        else
        {
            devext->OemExtension.IORes = *pDesc;
            Count = 0;
            pDesc = RtlUnpackPartialDesc(
                        CmResourceTypeInterrupt,
                        irpsp->Parameters.StartDevice.AllocatedResourcesTranslated,
                        &Count);
            if (pDesc == NULL)
            {
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
                LogError(ERRLOG_INVALID_CONFIGURATION,
                         status,
                         UNIQUE_ERRID(0x60),
                         NULL,
                         NULL);
                TRACEERROR(("no allocated IRQ resources.\n"));
            }
            else
            {
                WCHAR wstrMsg[64];

                TRACEASSERT(!(devext->dwfHBtn & HBTNF_INTERRUPT_CONNECTED));
                devext->OemExtension.IRQRes = *pDesc;

                status = IoConnectInterrupt(
                            &devext->OemExtension.InterruptObject,
                            OemInterruptServiceRoutine,
                            devext,
                            NULL,
                            devext->OemExtension.IRQRes.u.Interrupt.Vector,
                            (KIRQL)devext->OemExtension.IRQRes.u.Interrupt.Level,
                            (KIRQL)devext->OemExtension.IRQRes.u.Interrupt.Level,
                            (devext->OemExtension.IRQRes.Flags &
                             CM_RESOURCE_INTERRUPT_LATCHED)?
                                Latched: LevelSensitive,
                            (devext->OemExtension.IRQRes.ShareDisposition ==
                             CmResourceShareShared),
                            devext->OemExtension.IRQRes.u.Interrupt.Affinity,
                            FALSE);
                if (NT_SUCCESS(status))
                {
                    devext->dwfHBtn |= HBTNF_INTERRUPT_CONNECTED;
                    TRACEINFO(3, ("IO(Start=0x%x%x,Len=0x%x), IRQ(Level=%d,Vector=0x%x,Affinity=%d)\n",
                                  devext->OemExtension.IORes.u.Port.Start.HighPart,
                                  devext->OemExtension.IORes.u.Port.Start.LowPart,
                                  devext->OemExtension.IORes.u.Port.Length,
                                  devext->OemExtension.IRQRes.u.Interrupt.Level,
                                  devext->OemExtension.IRQRes.u.Interrupt.Vector,
                                  devext->OemExtension.IRQRes.u.Interrupt.Affinity));
                }
                else
                {
                    swprintf(wstrMsg, L"IRQ=%d",
                             devext->OemExtension.IRQRes.u.Interrupt.Level);
                    LogError(ERRLOG_CONNECTIRQ_FAILED,
                             status,
                             UNIQUE_ERRID(0x70),
                             NULL,
                             NULL);
                    TRACEERROR(("failed to connect IRQ %d\n",
                                devext->OemExtension.IRQRes.u.Interrupt.Level));
                }
            }
        }
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemStartDevice

/*++
    @doc    INTERNAL

    @func   VOID | OemRemoveDevice | FDO Remove routine

    @parm   IN PDEVICE_EXTENSION | devext | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS INTERNAL
OemRemoveDevice(
    IN PDEVICE_EXTENSION devext,
    IN PIRP              Irp
    )
{
    TRACEPROC("OemRemoveDevice", 2)
    NTSTATUS status;

    PAGED_CODE();
    TRACEENTER(("(devext=%p)\n", devext));

    if (devext->dwfHBtn & HBTNF_INTERRUPT_CONNECTED)
    {
        IoDisconnectInterrupt(devext->OemExtension.InterruptObject);
        devext->dwfHBtn &= ~HBTNF_INTERRUPT_CONNECTED;
    }

    if (devext->dwfHBtn & HBTNF_DEBOUNCE_TIMER_SET)
    {
        KeCancelTimer(&devext->OemExtension.DebounceTimer);
    }
    status = STATUS_SUCCESS;

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemRemoveDevice

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemWriteReport | Write output report.

    @parm   IN PDEVICE_EXTENSION | devext | Points to the device extension.
    @parm   IN PIRP | Irp | Points to an I/O Request Packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS.
    @rvalue FAILURE | returns NT status code.
--*/

NTSTATUS INTERNAL
OemWriteReport(
    IN PDEVICE_EXTENSION devext,
    IN PIRP              Irp
    )
{
    TRACEPROC("OemWriteReport", 2)
    NTSTATUS status;

    TRACEENTER(("(devext=%p,Irp=%p)\n", devext, Irp));

    status = STATUS_NOT_SUPPORTED;
    TRACEWARN(("unsupported WriteReport.\n"));

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemWriteReport

/*++
    @doc    EXTERNAL

    @func   BOOLEAN | OemInterruptServiceRoutine |
            Interrupt service routine for the button device.

    @parm   IN PKINTERRUPT | Interrupt | Points to the interrupt object.
    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.

    @rvalue SUCCESS | returns TRUE - it's our interrupt.
    @rvalue FAILURE | returns FALSE - it's not our interrupt.
--*/

BOOLEAN EXTERNAL
OemInterruptServiceRoutine(
    IN PKINTERRUPT       Interrupt,
    IN PDEVICE_EXTENSION DevExt
    )
{
    TRACEPROC("OemInterruptServiceRoutine", 5)
    BOOLEAN rc = FALSE;
    ULONG ButtonState;

    TRACEENTER(("(Interrupt=%p,DevExt=%p)\n", Interrupt, DevExt));
    UNREFERENCED_PARAMETER(Interrupt);

    //
    // Note: the action of reading the button state will also clear
    // the interrupt.
    //
    ButtonState = READBUTTONSTATE(DevExt);
    if ((ButtonState & BUTTON_INTERRUPT_MASK) &&
        ((ButtonState & BUTTON_STATUS_MASK)!=
          DevExt->OemExtension.dwLastButtonState))
    {
        TRACEINFO(1, ("Button interrupt (Buttons=%x)\n", ButtonState));
        DevExt->OemExtension.dwLastButtonState = ButtonState &
                                                 BUTTON_STATUS_MASK;
        DevExt->dwfHBtn |= HBTNF_DEBOUNCE_TIMER_SET;
        KeSetTimer(&DevExt->OemExtension.DebounceTimer,
                   DevExt->OemExtension.DebounceTime,
                   &DevExt->OemExtension.TimerDpc);
        TRACEINFO(3, ("Button Interrupt (ButtonState=%x)\n", ButtonState));
        rc = TRUE;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //OemInterruptServiceRoutine

/*++
    @doc    EXTERNAL

    @func   VOID | OemButtonDebounceDpc |
            Timer DPC routine to handle button debounce.

    @parm   IN PKDPC | Dpc | Points to the DPC object.
    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN PVOID | SysArg1 | System argument 1.
    @parm   IN PVOID | SysArg2 | System arugment 2.

    @rvalue None.
--*/

VOID EXTERNAL
OemButtonDebounceDpc(
    IN PKDPC             Dpc,
    IN PDEVICE_EXTENSION DevExt,
    IN PVOID             SysArg1,
    IN PVOID             SysArg2
    )
{
    TRACEPROC("OemButtonDebounceDpc", 2)
    ULONG dwCurButtons;

    TRACEENTER(("(Dpc=%p,DevExt=%p,SysArg1=%p,SysArg2=%p)\n",
                Dpc, DevExt, SysArg1, SysArg2));

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SysArg1);
    UNREFERENCED_PARAMETER(SysArg2);

    dwCurButtons = READBUTTONSTATE(DevExt) & BUTTON_STATUS_MASK;
    if (dwCurButtons == DevExt->OemExtension.dwLastButtonState)
    {
        ULONG dwChangedButtons;

        DevExt->dwfHBtn &= ~HBTNF_DEBOUNCE_TIMER_SET;
        dwChangedButtons = dwCurButtons^DevExt->OemExtension.dwLastButtons;
        TRACEINFO(3, ("Last=%x,Curr=%x,Changed=%x\n",
                      DevExt->OemExtension.dwLastButtons, dwCurButtons,
                      dwChangedButtons));
        if (dwChangedButtons & ~DevExt->OemExtension.dwSASButtonID)
        {
            //
            // Not a SAS button event, some other buttons.
            //
            SendButtonReport(DevExt, REPORTID_BTN, dwCurButtons);
        }

        if (dwChangedButtons & DevExt->OemExtension.dwSASButtonID)
        {
            //
            // SAS button event.
            //
            SendButtonReport(DevExt, REPORTID_KBD, dwCurButtons);
        }

        DevExt->OemExtension.dwLastButtons = dwCurButtons;
    }
    else
    {
        TRACEINFO(3, ("button state is unstable, try again (PrevState=%x,NewState=%x)\n",
                      DevExt->OemExtension.dwLastButtonState, dwCurButtons));
        DevExt->OemExtension.dwLastButtonState = dwCurButtons;
        KeSetTimer(&DevExt->OemExtension.DebounceTimer,
                   DevExt->OemExtension.DebounceTime,
                   &DevExt->OemExtension.TimerDpc);
    }

    TRACEEXIT(("!\n"));
    return;
}       //OemButtonDebounceDpc

/*++
    @doc    INTERNAL

    @func   BOOLEAN | SendButtonReport | Send a button report.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN UCHAR | bReportID | Specifies report ID.
    @parm   IN ULONG | dwCurButtons | Current button states.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.

    @note   This routine is always called at DISPATCH_LEVEL.
--*/

BOOLEAN INTERNAL
SendButtonReport(
    IN PDEVICE_EXTENSION DevExt,
    IN UCHAR             bReportID,
    IN ULONG             dwCurButtons
    )
{
    TRACEPROC("SendButtonReport", 2)
    BOOLEAN rc = FALSE;
    KIRQL OldIrql;
    PIRP irp = NULL;

    TRACEENTER(("(DevExt=%p,ReportID=%x,CurButtons=%x)\n",
                DevExt, bReportID, dwCurButtons));

    KeAcquireSpinLock(&DevExt->SpinLock, &OldIrql);
    if (!IsListEmpty(&DevExt->PendingIrpList))
    {
        PLIST_ENTRY plist;
        PDRIVER_CANCEL OldCancelRoutine;

        plist = RemoveHeadList(&DevExt->PendingIrpList);
        irp = CONTAINING_RECORD(plist, IRP, Tail.Overlay.ListEntry);
        OldCancelRoutine = IoSetCancelRoutine(irp, NULL);
        TRACEASSERT(OldCancelRoutine == ReadReportCanceled);
    }
    KeReleaseSpinLock(&DevExt->SpinLock, OldIrql);

    if (irp != NULL)
    {
        PHID_INPUT_REPORT report = (PHID_INPUT_REPORT)irp->UserBuffer;

        report->ReportID = bReportID;
        if (bReportID == REPORTID_BTN)
        {
            report->BtnReport.dwButtonStates =
                dwCurButtons & ~DevExt->OemExtension.dwSASButtonID;
            TRACEINFO(3, ("Button Event (Buttons=%x)\n",
                          report->BtnReport.dwButtonStates));
        }
        else
        {
            report->KbdReport.bKeys =
                (dwCurButtons & DevExt->OemExtension.dwSASButtonID)? SASF_CAD:
                                                                     0;
            TRACEINFO(3, ("SAS Event (Buttons=%x)\n",
                          dwCurButtons & DevExt->OemExtension.dwSASButtonID));
        }
        irp->IoStatus.Information = sizeof(HID_INPUT_REPORT);
        irp->IoStatus.Status = STATUS_SUCCESS;
        IoReleaseRemoveLock(&DevExt->RemoveLock, irp);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        rc = TRUE;
    }
    else
    {
        TRACEWARN(("no pending ReadReport irp, must have been canceled\n"));
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //SendButtonReport

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
    TRACEENTER(("(hkey=%x,ValueName=%S,Type=%x,lpbData=%p,Len=%d,pdwLen=%p)\n",
                hkey, pszValueName, dwType, lpbData, dwcbData, pdwcbLen));

    TRACEASSERT(lpbData != NULL);
    RtlInitUnicodeString(&ucstrName, pszValueName);
    dwLen = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + dwcbData;
    RegInfo = ExAllocatePool(PagedPool, dwLen);
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
                         UNIQUE_ERRID(0x80), NULL, NULL);
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
                 UNIQUE_ERRID(0x90), NULL, NULL);
        TRACEERROR(("failed to allocate registry info (len=%d).\n", dwcbData));
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //RegQueryValue

/*++
    @doc    EXTERNAL

    @func   PCM_PARTIAL_RESOURCE_DESCRIPTOR | RtlUnpackPartialDesc |
            Pulls out a pointer to the partial descriptor you are interested
            in.

    @parm   IN UCHAR | ResType | Specifies the resource type we are interested
            in.
    @parm   IN PCM_RESOURCE_LIST | ResList | Points to the resource list to
            search.
    @parm   IN OUT PULONG | Count | Points to the index of the partial
            descriptor you are looking for, gets incremented if found.
            i.e. starts with *Count = 0, then subsequent calls will find the
            next partial descriptor.

    @rvalue SUCCESS | returns pointer to the partial descriptor found.
    @rvalue FAILURE | returns NULL.
--*/

PCM_PARTIAL_RESOURCE_DESCRIPTOR INTERNAL
RtlUnpackPartialDesc(
    IN UCHAR             ResType,
    IN PCM_RESOURCE_LIST ResList,
    IN OUT PULONG        Count
    )
{
    TRACEPROC("RtlUnpackPartialDesc", 2)
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pDesc = NULL;
    ULONG i, j, hit = 0;

    PAGED_CODE();
    TRACEENTER(("(ResType=%x,ResList=%p,Count=%d)\n",
                ResType, ResList, *Count));

    for (i = 0; (i < ResList->Count) && (pDesc == NULL); ++i)
    {
        for (j = 0; j < ResList->List[i].PartialResourceList.Count; ++j)
        {
            if (ResList->List[i].PartialResourceList.PartialDescriptors[j].Type
                == ResType)
            {
                if (hit == *Count)
                {
                    (*Count)++;
                    pDesc = &ResList->List[i].PartialResourceList.PartialDescriptors[j];
                    break;
                }
                else
                {
                    hit++;
                }
            }
        }
    }

    TRACEEXIT(("=%p (Count=%d)\n", pDesc, *Count));
    return pDesc;
}       //RtlUnpackPartialDesc
