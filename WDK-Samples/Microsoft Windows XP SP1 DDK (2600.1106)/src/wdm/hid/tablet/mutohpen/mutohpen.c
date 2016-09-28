/*++
    Copyright (c) 2000,2002 Microsoft Corporation

    Module Name:
        mutohpen.c

    Abstract:
        Contains mutoh specific functions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 13-Mar-2000

    Revision History:
--*/

#include "pch.h"
#define MODULE_ID                       5

UCHAR gReportDescriptor[] = {
    0x05, 0x0d,                         // USAGE_PAGE (Digitizers)          0
    0x09, 0x02,                         // USAGE (Pen)                      2
    0xa1, 0x01,                         // COLLECTION (Application)         4
    0x85, REPORTID_PEN,                 //   REPORT_ID (Pen)                6
    0x09, 0x20,                         //   USAGE (Stylus)                 8
    0xa1, 0x00,                         //   COLLECTION (Physical)          10
    0x09, 0x42,                         //     USAGE (Tip Switch)           12
    0x09, 0x44,                         //     USAGE (Barrel Switch)        14
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)          16
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)          18
    0x75, 0x01,                         //     REPORT_SIZE (1)              20
    0x95, 0x02,                         //     REPORT_COUNT (2)             22
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         24
    0x95, 0x03,                         //     REPORT_COUNT (3)             26
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)         28
    0x09, 0x32,                         //     USAGE (In Range)             30
    0x95, 0x01,                         //     REPORT_COUNT (1)             32
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         34
    0x95, 0x02,                         //     REPORT_COUNT (2)             36
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)         38
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop) 40
    0x09, 0x30,                         //     USAGE (X)                    42
    0x75, 0x10,                         //     REPORT_SIZE (16)             44
    0x95, 0x01,                         //     REPORT_COUNT (1)             46
    0xa4,                               //     PUSH                         48
    0x55, 0x0d,                         //     UNIT_EXPONENT (-3)           49
    0x65, 0x33,                         //     UNIT (Inch,EngLinear)        51
    0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)         53
    0x46, 0x3a, 0x20,                   //     PHYSICAL_MAXIMUM (8250)      55
    0x26, 0x97, 0x21,                   //     LOGICAL_MAXIMUM (8599)       58
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         61
    0x09, 0x31,                         //     USAGE (Y)                    63
    0x46, 0x2c, 0x18,                   //     PHYSICAL_MAXIMUM (6188)      65
    0x26, 0x4f, 0x19,                   //     LOGICAL_MAXIMUM (6479)       68
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         71
    0xb4,                               //     POP                          73/74
#ifdef _TIMESTAMP_
    0x06, 0x00, 0xff,                   //     USAGE_PAGE (Vendor Defined)  0
    0x09, 0x80,                         //     USAGE (Vendor Usage 0x80)    3
    0x27, 0xff, 0xff, 0xff, 0xff,       //     LOGICAL_MAXIMUM (0xffffffff) 5
    0x75, 0x20,                         //     REPORT_SIZE (32)             10
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         12
    0x09, 0x81,                         //     USAGE (Vendor Usage 0x81)    14
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         16/18
#endif
    0xc0,                               //   END_COLLECTION                 0
    0xc0,                               // END_COLLECTION                   1/2

    //
    // Dummy mouse collection starts here
    //
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)     0
    0x09, 0x02,                         // USAGE (Mouse)                    2
    0xa1, 0x01,                         // COLLECTION (Application)         4
    0x85, REPORTID_MOUSE,               //   REPORT_ID (Mouse)              6
    0x09, 0x01,                         //   USAGE (Pointer)                8
    0xa1, 0x00,                         //   COLLECTION (Physical)          10
    0x05, 0x09,                         //     USAGE_PAGE (Button)          12
    0x19, 0x01,                         //     USAGE_MINIMUM (Button 1)     14
    0x29, 0x02,                         //     USAGE_MAXIMUM (Button 2)     16
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)          18
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)          20
    0x75, 0x01,                         //     REPORT_SIZE (1)              22
    0x95, 0x02,                         //     REPORT_COUNT (2)             24
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         26
    0x95, 0x06,                         //     REPORT_COUNT (6)             28
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)         30
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop) 32
    0x09, 0x30,                         //     USAGE (X)                    34
    0x09, 0x31,                         //     USAGE (Y)                    36
    0x15, 0x81,                         //     LOGICAL_MINIMUM (-127)       38
    0x25, 0x7f,                         //     LOGICAL_MAXIMUM (127)        40
    0x75, 0x08,                         //     REPORT_SIZE (8)              42
    0x95, 0x02,                         //     REPORT_COUNT (2)             44
    0x81, 0x06,                         //     INPUT (Data,Var,Rel)         46
    0xc0,                               //   END_COLLECTION                 48
    0xc0                                // END_COLLECTION                   49/50
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

PWSTR gpwstrManufacturerID = L"Mutoh";
PWSTR gpwstrProductID = L"Serial Pen Tablet (3310)";
PWSTR gpwstrSerialNumber = L"0";

//
// Local function prototypes.
//
NTSTATUS INTERNAL
QueryDeviceInfo(
    IN PDEVICE_EXTENSION DevExt
    );

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, OemInitDevice)
  #pragma alloc_text(PAGE, OemWakeupDevice)
  #pragma alloc_text(PAGE, OemStandbyDevice)
  #pragma alloc_text(PAGE, OemInitSerialPort)
  #pragma alloc_text(PAGE, QueryDeviceInfo)
#endif  //ifdef ALLOC_PRAGMA

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemInitDevice | Initialize pen tablet device.

    @parm   IN PDEVICE_EXTENSION | devext | Points to the device extension.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
OemInitDevice(
    IN PDEVICE_EXTENSION devext
    )
{
    TRACEPROC("OemInitDevice", 2)
    NTSTATUS status;

    PAGED_CODE();
    TRACEENTER(("(devext=%p)\n", devext));

    status = SerialSyncReadWritePort(FALSE,
                                     devext,
                                     "@",
                                     1,
                                     NULL,
                                     NULL);
    if (!NT_SUCCESS(status))
    {
        LogError(ERRLOG_INIT_DIGITIZER_FAILED,
                 status,
                 UNIQUE_ERRID(0x10),
                 NULL,
                 NULL);
        TRACEERROR(("failed to send reset command to digitizer (status=%x).\n",
                    status));
    }
    else
    {
        LONGLONG WaitTime = Int32x32To64(100, -10000);

        //
        // We need to delay 20msec after a software reset is sent.
        //
        KeDelayExecutionThread(KernelMode,
                               FALSE,
                               (LARGE_INTEGER *)&WaitTime);

        //
        // Set sampling rate to 133 samples/sec and turn digital
        // filter on.
        //
        status = SerialSyncReadWritePort(FALSE,
                                         devext,
                                         "V1",
                                         2,
                                         NULL,
                                         NULL);
        if (!NT_SUCCESS(status))
        {
            LogError(ERRLOG_INIT_DIGITIZER_FAILED,
                     status,
                     UNIQUE_ERRID(0x20),
                     NULL,
                     NULL);
            TRACEERROR(("failed to initialize digitizer hardware (status=%x).\n",
                        status));
        }
        else if (!NT_SUCCESS(status = SerialSyncReadWritePort(
                                        FALSE,
                                        devext,
                                        "LO",
                                        2,
                                        NULL,
                                        NULL)))
        {
            LogError(ERRLOG_INIT_DIGITIZER_FAILED,
                     status,
                     UNIQUE_ERRID(0x30),
                     NULL,
                     NULL);
            TRACEERROR(("failed to set digitizer configuration (status=%x).\n",
                        status));
        }
        else
        {
            status = QueryDeviceInfo(devext);
            if (!NT_SUCCESS(status))
            {
                //
                // It's not a big deal if we don't get the device
                // info. It's more important to keep the driver
                // running.
                //
                LogError(ERRLOG_INIT_DIGITIZER_FAILED,
                         status,
                         UNIQUE_ERRID(0x40),
                         NULL,
                         NULL);
                TRACEWARN(("failed to query digitizer info (status=%x).\n",
                           status));
                status = STATUS_SUCCESS;
            }
        }
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemInitDevice

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemWakeupDevice | OEM specific wake up code.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
OemWakeupDevice(
    IN PDEVICE_EXTENSION DevExt
    )
{
    TRACEPROC("OemWakeupDevice", 2)
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();
    TRACEENTER(("(DevExt=%p)\n", DevExt));

    if (DevExt->dwfHPen & HPENF_DEVICE_STARTED)
    {
        status = SerialAsyncReadWritePort(FALSE, DevExt, "A", 1);
        if (!NT_SUCCESS(status))
        {
            LogError(ERRLOG_WAKE_DIGITIZER_FAILED,
                     status,
                     UNIQUE_ERRID(0x50),
                     NULL,
                     NULL);
            TRACEWARN(("failed to wake the digitizer (status=%x).\n", status));
        }
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemWakeupDevice

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemStandbyDevice | OEM specific wake up code.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
OemStandbyDevice(
    IN PDEVICE_EXTENSION DevExt
    )
{
    TRACEPROC("OemStandbyDevice", 2)
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();
    TRACEENTER(("(DevExt=%p)\n", DevExt));

    if (DevExt->dwfHPen & HPENF_DEVICE_STARTED)
    {
        status = SerialAsyncReadWritePort(FALSE, DevExt, "W", 1);
        if (!NT_SUCCESS(status))
        {
            LogError(ERRLOG_SLEEP_DIGITIZER_FAILED,
                     status,
                     UNIQUE_ERRID(0x60),
                     NULL,
                     NULL);
            TRACEWARN(("failed to send StandBy command (status=%x).\n",
                       status));
        }
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemStandbyDevice

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemInitSerialPort | Initialize com port for
            communication.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
OemInitSerialPort(
    IN PDEVICE_EXTENSION DevExt
    )
{
    TRACEPROC("OemInitSerialPort", 2)
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    PAGED_CODE();
    TRACEENTER(("(DevExt=%p)\n", DevExt));

    //
    // Set the com port to basic operating mode: reads/writes one byte at
    // time, no handshake flow control or timeouts.
    //
    status = SerialSyncSendIoctl(IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS,
                                 DevExt->LowerDevObj,
                                 NULL,
                                 0,
                                 &DevExt->OemData.PrevSerialSettings,
                                 sizeof(DevExt->OemData.PrevSerialSettings),
                                 TRUE,
                                 &iosb);

    if (!NT_SUCCESS(status))
    {
        LogError(ERRLOG_INIT_SERIAL_FAILED,
                 status,
                 UNIQUE_ERRID(0x70),
                 NULL,
                 NULL);
        TRACEERROR(("failed to set basic settings of COM (status=%x).\n",
                    status));
    }
    else
    {
        SERIAL_BAUD_RATE sbr;

        DevExt->dwfHPen |= HPENF_SERIAL_INITED;
        sbr.BaudRate = OEM_SERIAL_BAUDRATE;
        status = SerialSyncSendIoctl(IOCTL_SERIAL_SET_BAUD_RATE,
                                     DevExt->LowerDevObj,
                                     &sbr,
                                     sizeof(sbr),
                                     NULL,
                                     0,
                                     FALSE,
                                     &iosb);
        if (!NT_SUCCESS(status))
        {
            LogError(ERRLOG_INIT_SERIAL_FAILED,
                     status,
                     UNIQUE_ERRID(0x80),
                     NULL,
                     NULL);
            TRACEERROR(("failed to set COM baudrate (status=%x).\n", status));
        }
        else
        {
            SERIAL_LINE_CONTROL slc;

            slc.WordLength = OEM_SERIAL_WORDLEN;
            slc.Parity = OEM_SERIAL_PARITY;
            slc.StopBits = OEM_SERIAL_STOPBITS;
            status = SerialSyncSendIoctl(IOCTL_SERIAL_SET_LINE_CONTROL,
                                         DevExt->LowerDevObj,
                                         &slc,
                                         sizeof(slc),
                                         NULL,
                                         0,
                                         FALSE,
                                         &iosb);

            if (!NT_SUCCESS(status))
            {
                LogError(ERRLOG_INIT_SERIAL_FAILED,
                         status,
                         UNIQUE_ERRID(0x90),
                         NULL,
                         NULL);
                TRACEERROR(("failed to set COM LineControl (status=%x).\n",
                            status));
            }
            else
            {
                //
                // Enable FIFO receive trigger at 4 bytes
                //
                ULONG Data = SERIAL_IOC_FCR_FIFO_ENABLE |
                             SERIAL_IOC_FCR_RCVR_RESET |
                             SERIAL_IOC_FCR_XMIT_RESET |
                             SERIAL_IOC_FCR_RCVR_TRIGGER_04_BYTES;

                status = SerialSyncSendIoctl(IOCTL_SERIAL_SET_FIFO_CONTROL,
                                             DevExt->LowerDevObj,
                                             &Data,
                                             sizeof(Data),
                                             NULL,
                                             0,
                                             FALSE,
                                             &iosb);

                if (!NT_SUCCESS(status))
                {
                    LogError(ERRLOG_INIT_SERIAL_FAILED,
                             status,
                             UNIQUE_ERRID(0xa0),
                             NULL,
                             NULL);
                    TRACEERROR(("failed to set COM FIFO control (status=%x).\n",
                                status));
                }
                else
                {
                    Data = SERIAL_PURGE_RXCLEAR;
                    status = SerialSyncSendIoctl(IOCTL_SERIAL_PURGE,
                                                 DevExt->LowerDevObj,
                                                 &Data,
                                                 sizeof(Data),
                                                 NULL,
                                                 0,
                                                 FALSE,
                                                 &iosb);

                    if (!NT_SUCCESS(status))
                    {
                        LogError(ERRLOG_INIT_SERIAL_FAILED,
                                 status,
                                 UNIQUE_ERRID(0xb0),
                                 NULL,
                                 NULL);
                        TRACEERROR(("failed to flush COM receive buffer (status=%x).\n",
                                    status));
                    }
                }
            }
        }
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //OemInitSerialPort

/*++
    @doc    INTERNAL

    @func   NTSTATUS | QueryDeviceInfo | Query pen tablet device information.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
QueryDeviceInfo(
    IN PDEVICE_EXTENSION DevExt
    )
{
    TRACEPROC("QueryDeviceInfo", 2)
    NTSTATUS status, status2;

    PAGED_CODE();
    TRACEENTER(("(DevExt=%p)\n", DevExt));

    status = SerialSyncReadWritePort(FALSE, DevExt, "K", 1, NULL, NULL);
    if (NT_SUCCESS(status))
    {
        LARGE_INTEGER Timeout;
        OEM_INPUT_REPORT InData[3];
        ULONG BytesRead;

        // Set timeout to 100 msec.
        Timeout.QuadPart = Int32x32To64(100, -10000);
        while ((status = SerialSyncReadWritePort(TRUE,
                                                 DevExt,
                                                 (PUCHAR)InData,
                                                 1,
                                                 &Timeout,
                                                 &BytesRead)) ==
                STATUS_SUCCESS)
        {
            if (InData[0].InputReport.bStatus == 0x88)
            {
                break;
            }
        }

        if (NT_SUCCESS(status))
        {
            status = SerialSyncReadWritePort(TRUE,
                                             DevExt,
                                             ((PUCHAR)InData) + 1,
                                             sizeof(InData) - 1,
                                             &Timeout,
                                             &BytesRead);
            if (NT_SUCCESS(status))
            {
                if ((BytesRead == sizeof(InData) - 1) &&
                    (InData[0].InputReport.bStatus == 0x88) &&
                    (InData[1].InputReport.bStatus == 0x88) &&
                    (InData[2].InputReport.bStatus == 0x8f))
                {
                    DevExt->OemData.wFirmwareDate =
                                NORMALIZE_DATA(InData[0].InputReport.wXData);
                    DevExt->OemData.wFirmwareYear =
                                NORMALIZE_DATA(InData[0].InputReport.wYData);
                    DevExt->OemData.wProductID =
                                NORMALIZE_DATA(InData[1].InputReport.wXData);
                    DevExt->OemData.wFirmwareVer =
                                NORMALIZE_DATA(InData[1].InputReport.wYData);
                    DevExt->OemData.wCorrectionRev =
                                NORMALIZE_DATA(InData[2].InputReport.wXData);
                    TRACEINFO(1, ("FirmwareDate=%d,FirmwareYear=%d,ProductID=%d,FirmwareVer=%d,CorrectionRev=%d\n",
                                  DevExt->OemData.wFirmwareDate,
                                  DevExt->OemData.wFirmwareYear,
                                  DevExt->OemData.wProductID,
                                  DevExt->OemData.wFirmwareVer,
                                  DevExt->OemData.wCorrectionRev));
                }
                else
                {
                    status = STATUS_DEVICE_DATA_ERROR;
                    LogError(ERRLOG_QUERY_DEVINFO_FAILED,
                             status,
                             UNIQUE_ERRID(0xc0),
                             NULL,
                             NULL);
                    TRACEWARN(("invalid status response (size=%d,InData=%p).\n",
                               BytesRead, InData));
                }
            }
            else
            {
                LogError(ERRLOG_QUERY_DEVINFO_FAILED,
                         status,
                         UNIQUE_ERRID(0xd0),
                         NULL,
                         NULL);
                TRACEWARN(("failed to read response packet (status=%x).\n",
                           status));
            }
        }
        else
        {
            LogError(ERRLOG_QUERY_DEVINFO_FAILED,
                     status,
                     UNIQUE_ERRID(0xe0),
                     NULL,
                     NULL);
            TRACEWARN(("failed to read 1st response byte (status=%x).\n",
                       status));
        }
    }
    else
    {
        LogError(ERRLOG_QUERY_DEVINFO_FAILED,
                 status,
                 UNIQUE_ERRID(0xf0),
                 NULL,
                 NULL);
        TRACEWARN(("failed to send status command (status=%x).\n", status));
    }

    status2 = SerialSyncReadWritePort(FALSE,
                                      DevExt,
                                      "A",
                                      1,
                                      NULL,
                                      NULL);
    if (!NT_SUCCESS(status2))
    {
        LogError(ERRLOG_QUERY_DEVINFO_FAILED,
                 status,
                 UNIQUE_ERRID(0x110),
                 NULL,
                 NULL);
        TRACEWARN(("failed to acknowledge QueryStatus (status=%x).\n",
                   status2));
    }

    TRACEEXIT(("=%x\n", status));
    return status;
}       //QueryDeviceInfo

/*++
    @doc    INTERNAL

    @func   BOOLEAN | OemIsResyncDataValid | Check if the data in the resync
            buffer is valid.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOLEAN INTERNAL
OemIsResyncDataValid(
    IN PDEVICE_EXTENSION DevExt
    )
{
    TRACEPROC("OemIsResyncDataValid", 2)
    BOOLEAN rc;

    TRACEENTER(("(DevExt=%p)\n", DevExt));

    rc = IsValidPacket(&DevExt->OemData.ResyncData[0]);
    if ((rc == FALSE) ||
        (DevExt->OemData.BytesInBuff > sizeof(OEM_INPUT_REPORT)))
    {
        PUCHAR pb = (PUCHAR)&DevExt->OemData.ResyncData[0] +
                            DevExt->OemData.BytesInBuff - 1;
        PUCHAR pbEnd = rc? (PUCHAR)&DevExt->OemData.ResyncData[1]:
                           (PUCHAR)&DevExt->OemData.ResyncData[0];

        //
        // Even if we seem to have a valid packet in the resync buffer, we
        // still need to scan the next packet if any.  If the next packet
        // has a sync bit out of place, the first packet could still be
        // invalid and we better throw it away.
        //
        while (pb > pbEnd)
        {
            if (*pb & INSTATUS_SYNC)
            {
                TRACEINFO(3,
                          ("invalid buffer (len=%d,status0=%x,xData0=%x,yData0=%x,status1=%x,xData1=%x,yData1=%x)\n",
                           DevExt->OemData.BytesInBuff,
                           DevExt->OemData.ResyncData[0].InputReport.bStatus,
                           DevExt->OemData.ResyncData[0].InputReport.wXData,
                           DevExt->OemData.ResyncData[0].InputReport.wYData,
                           DevExt->OemData.ResyncData[1].InputReport.bStatus,
                           DevExt->OemData.ResyncData[1].InputReport.wXData,
                           DevExt->OemData.ResyncData[1].InputReport.wYData));
                DevExt->OemData.BytesInBuff = (ULONG)
                        ((PUCHAR)&DevExt->OemData.ResyncData[0] +
                                  DevExt->OemData.BytesInBuff - pb);
                RtlMoveMemory(&DevExt->OemData.ResyncData[0],
                              pb,
                              DevExt->OemData.BytesInBuff);
                TRACEINFO(3,
                          ("Resync'd buffer (len=%d,status=%x,xData=%x,yData=%x)\n",
                           DevExt->OemData.BytesInBuff,
                           DevExt->OemData.ResyncData[0].InputReport.bStatus,
                           DevExt->OemData.ResyncData[0].InputReport.wXData,
                           DevExt->OemData.ResyncData[0].InputReport.wYData));
              #ifdef DEBUG
                {
                    ULONG dwcDeletedBytes = (ULONG)
                                (pb - (PUCHAR)&DevExt->OemData.ResyncData[0]);

                    DevExt->OemData.dwcLostBytes +=
                        (dwcDeletedBytes > sizeof(OEM_INPUT_REPORT))?
                                sizeof(OEM_INPUT_REPORT)*2 - dwcDeletedBytes:
                                sizeof(OEM_INPUT_REPORT) - dwcDeletedBytes;
                }
              #endif
                rc = FALSE;
                break;
            }
            --pb;
        }

        if ((rc == FALSE) && (pb <= pbEnd))
        {
            //
            // We didn't have a valid packet and we couldn't find the sync
            // bit of the next packet, so the whole resync buffer is invalid.
            //
            DevExt->OemData.BytesInBuff = 0;
        }
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //OemIsResyncDataValid

/*++
    @doc    INTERNAL

    @func   NTSTATUS | OemNormalizeInputData | Normalize the input data.

    @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
    @parm   IN OUT POEM_INPUT_REPORT | InData | Points to the input data packet.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns STATUS_DATA_ERROR.
--*/

NTSTATUS INTERNAL
OemNormalizeInputData(
    IN     PDEVICE_EXTENSION DevExt,
    IN OUT POEM_INPUT_REPORT InData
    )
{
    TRACEPROC("OemNormalizeInputData", 2)
    NTSTATUS status = STATUS_SUCCESS;
#ifdef DEBUG
    ULONGLONG CurrentTime;
#endif

    TRACEENTER(("(DevExt=%p,InData=%p,Status=%x,XData=%x,YData=%x)\n",
                DevExt, InData, InData->InputReport.bStatus,
                InData->InputReport.wXData, InData->InputReport.wYData));

    InData->InputReport.wXData = NORMALIZE_DATA(InData->InputReport.wXData);
    InData->InputReport.wYData = NORMALIZE_DATA(InData->InputReport.wYData);
    if (InData->InputReport.wXData >= OEM_PEN_MAX_X)
    {
      #ifdef DEBUG
        if (InData->InputReport.wXData > DevExt->OemData.wMaxX)
        {
            DevExt->OemData.wMaxX = InData->InputReport.wXData;
        }
      #endif
        InData->InputReport.wXData = OEM_PEN_MAX_X - 1;
    }

    if (InData->InputReport.wYData >= OEM_PEN_MAX_Y)
    {
      #ifdef DEBUG
        if (InData->InputReport.wYData > DevExt->OemData.wMaxY)
        {
            DevExt->OemData.wMaxY = InData->InputReport.wYData;
        }
      #endif
        InData->InputReport.wYData = 0;
    }
    else
    {
        InData->InputReport.wYData = OEM_PEN_MAX_Y - 1 -
                                     InData->InputReport.wYData;
    }

#ifdef _TIMESTAMP_
    DevExt->OemData.LastCounter = KeQueryPerformanceCounter(NULL);
#endif
#ifdef DEBUG
    CurrentTime = KeQueryInterruptTime();
    if ((DevExt->OemData.LastReport.InputReport.bStatus ^
         InData->InputReport.bStatus) & INSTATUS_PEN_TIP_DOWN)
    {
        //
        // The tip switch changes state
        //
        if (InData->InputReport.bStatus & INSTATUS_PEN_TIP_DOWN)
        {
            DevExt->OemData.dwcSamples = 0;
            DevExt->OemData.dwcLostBytes = 0;
            DevExt->OemData.StartTime = CurrentTime;
        }
        else
        {
            CurrentTime -= DevExt->OemData.StartTime;
            CurrentTime /= 10000;
            TRACEINFO(1, ("Samples=%d,Elapsed=%d,Rate=%d,BytesLost=%d\n",
                          DevExt->OemData.dwcSamples,
                          (ULONG)CurrentTime,
                          CurrentTime? DevExt->OemData.dwcSamples*1000/
                                       CurrentTime:
                                       0,
                          DevExt->OemData.dwcLostBytes));
        }
    }

    DevExt->OemData.dwcSamples++;

    DevExt->OemData.LastReportTime = CurrentTime;
    RtlCopyMemory(&DevExt->OemData.LastReport,
                  InData,
                  sizeof(DevExt->OemData.LastReport));
#endif
    PoSetSystemState(ES_USER_PRESENT);

    TRACEINFO(3, ("status=%x,xData=%x(%d),yData=%x(%d)\n",
                  InData->InputReport.bStatus,
                  InData->InputReport.wXData,
                  InData->InputReport.wXData,
                  InData->InputReport.wYData,
                  InData->InputReport.wYData));

    TRACEEXIT(("=%x (Status=%x,XData=%x(%d),YData=%x(%d)\n",
               status,
               InData->InputReport.bStatus,
               InData->InputReport.wXData,
               InData->InputReport.wXData,
               InData->InputReport.wYData,
               InData->InputReport.wYData));
    return status;
}       //OemNormalizeInputData
