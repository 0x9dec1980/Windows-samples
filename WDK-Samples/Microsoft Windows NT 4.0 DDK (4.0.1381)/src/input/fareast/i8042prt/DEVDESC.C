#include <ntddk.h>

#include "devdesc.h"

#ifdef PNP_IDENTIFY

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,LinkDeviceToDescription)
#endif

NTSTATUS
LinkDeviceToDescription(
    IN PUNICODE_STRING     RegistryPath,
    IN PUNICODE_STRING     DeviceName,
    IN INTERFACE_TYPE      BusType,
    IN ULONG               BusNumber,
    IN CONFIGURATION_TYPE  ControllerType,
    IN ULONG               ControllerNumber,
    IN CONFIGURATION_TYPE  PeripheralType,
    IN ULONG               PeripheralNumber
    )
{
    //
    // This routine will create a volatile "Description" key under the
    // drivers service key. It will store values of the following form
    // in that key:
    //
    // \\Device\\PointerPortX:REG_BINARY:...
    // \\Device\\KeyboardPortX:REG_BINARY:...
    //
    // Where the binary data is six ULONG values (passed as parameters
    // to this routine) that describe the physical location of the device.
    //

    NTSTATUS            Status = STATUS_SUCCESS;
    HANDLE              ServiceKey = NULL, DescriptionKey = NULL;
    UNICODE_STRING      RegString;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    ULONG               disposition;
    HWDESC_INFO         HwDescInfo;

    HwDescInfo.InterfaceType    = BusType;
    HwDescInfo.InterfaceNumber  = BusNumber;
    HwDescInfo.ControllerType   = ControllerType;
    HwDescInfo.ControllerNumber = ControllerNumber;
    HwDescInfo.PeripheralType   = PeripheralType;
    HwDescInfo.PeripheralNumber = PeripheralNumber;


    //
    // Open the service subkey
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&ServiceKey,
                       KEY_WRITE,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    //
    // Create a volatile Description subkey under the service subkey
    //
    RtlInitUnicodeString(&RegString, L"Description");

    InitializeObjectAttributes(&ObjectAttributes,
                               &RegString,
                               OBJ_CASE_INSENSITIVE,
                               ServiceKey,
                               NULL);

    Status = ZwCreateKey(&DescriptionKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         (PUNICODE_STRING)NULL,
                         REG_OPTION_VOLATILE,
                         &disposition);

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    //
    // The description data is stored under a REG_BINARY value (name
    // is the DeviceName passed in as a parameter)
    //
    Status = ZwSetValueKey(DescriptionKey,
                           DeviceName,
                           0,
                           REG_BINARY,
                           &HwDescInfo,
                           sizeof(HwDescInfo));


    Clean0:

    if (DescriptionKey) {
        ZwClose(DescriptionKey);
    }

    if (ServiceKey) {
        ZwClose(ServiceKey);
    }

    return Status;
}

#ifdef WINDOWS_FE
#include "stdio.h"
#include "stdlib.h"

#define ISA_BUS_NODE \
        "\\Registry\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter\\%d"

ULONG
FindIsaBusNode(
    IN OUT VOID
    )
{
    ULONG   NodeNumber = 0;
    BOOLEAN FoundBus = FALSE;

    NTSTATUS Status;

    RTL_QUERY_REGISTRY_TABLE parameters[2];

    UNICODE_STRING invalidBusName;
    UNICODE_STRING targetBusName;
    UNICODE_STRING isaBusName;
#if !defined(_X86_)
    UNICODE_STRING jazzBusName;
#endif // !defined(_X86_)

    //
    // Initialize invalid bus name.
    //
    RtlInitUnicodeString(&invalidBusName,L"BADBUS");

    //
    // Initialize "ISA" bus name.
    //
    RtlInitUnicodeString(&isaBusName,L"ISA");
#if !defined(_X86_)
    RtlInitUnicodeString(&jazzBusName,L"Jazz-Internal Bus");
#endif // !defined(_X86_)

    parameters[0].QueryRoutine = NULL;
    parameters[0].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                          RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name = L"Identifier";
    parameters[0].EntryContext = &targetBusName;
    parameters[0].DefaultType = REG_SZ;
    parameters[0].DefaultData = &invalidBusName;
    parameters[0].DefaultLength = 0;

    parameters[1].QueryRoutine = NULL;
    parameters[1].Flags = 0;
    parameters[1].Name = NULL;
    parameters[1].EntryContext = NULL;

    do {
        CHAR AnsiBuffer[512];

        ANSI_STRING AnsiString;
        UNICODE_STRING registryPath;

        //
        // Build path buffer...
        //
        sprintf(AnsiBuffer,ISA_BUS_NODE,NodeNumber);
        RtlInitAnsiString(&AnsiString,AnsiBuffer);
        RtlAnsiStringToUnicodeString(&registryPath,&AnsiString,TRUE);

        //
        // Initialize recieve buffer.
        //
        targetBusName.Buffer = NULL;

        //
        // Query it.
        //
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        registryPath.Buffer,
                                        parameters,
                                        NULL,
                                        NULL);

        RtlFreeUnicodeString(&registryPath);

        if (!NT_SUCCESS(Status) || (targetBusName.Buffer == NULL)) {
            break;
        }

        //
        // Is this "ISA" node ?
        //
        if (RtlCompareUnicodeString(&targetBusName,&isaBusName,TRUE) == 0) {
            //
            // Found.
            //
            FoundBus = TRUE;
            break;
        }

#if !defined(_X86_)
        //
        // Is this "Jazz-Internal Bus" node ?
        //
        if (RtlCompareUnicodeString(&targetBusName,&jazzBusName,TRUE) == 0) {
            //
            // Found.
            //
            FoundBus = TRUE;
            break;
        }
#endif // !defined(_X86_)

        //
        // Can we find any node for this ??
        //
        if (RtlCompareUnicodeString(&targetBusName,&invalidBusName,TRUE) == 0) {
            //
            // Not found.
            //
            break;
        }

        RtlFreeUnicodeString(&targetBusName);

        //
        // Next node number..
        //
        NodeNumber++;

    } while (TRUE);
        
    if (targetBusName.Buffer) {
        RtlFreeUnicodeString(&targetBusName);
    }

    if (!FoundBus) {
        NodeNumber = (ULONG)-1;
    }

    return (NodeNumber);
}

NTSTATUS
OverrideDeviceIdentifier(
    IN LPWSTR              DeviceIdentifier,
    IN INTERFACE_TYPE      BusType,
    IN ULONG               BusNumber,
    IN CONFIGURATION_TYPE  ControllerType,
    IN ULONG               ControllerNumber,
    IN CONFIGURATION_TYPE  PeripheralType,
    IN ULONG               PeripheralNumber
    )
{
    UNICODE_STRING RegistryPath;
    WCHAR RegistryPathBuffer[512] = L"\0";

    UNICODE_STRING Number;
    WCHAR NumberBuffer[10];

    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE KeyHandle;
    NTSTATUS Status;

    //
    // If there is no string to write, just return here..
    //
    if ((DeviceIdentifier == NULL) || (*DeviceIdentifier == UNICODE_NULL)) {
        return STATUS_SUCCESS;
    }

    //
    // Initialize Unicode string.
    //
    RegistryPath.Buffer = RegistryPathBuffer;
    RegistryPath.Length = 0;
    RegistryPath.MaximumLength = sizeof(RegistryPathBuffer);

    Number.Buffer = NumberBuffer;
    Number.Length = 0;
    Number.MaximumLength = sizeof(NumberBuffer);

    //
    // Build registry path (root)
    //
    RtlAppendUnicodeToString(&RegistryPath,
                             L"\\Registry\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\");

    //
    // Add Bus adaptor name.
    //
    if (BusType == Eisa) {
        //
        // Add "EisaAdapter"..
        //
        RtlAppendUnicodeToString(&RegistryPath,L"EisaAdapter\\");
    } else {
        //
        // Add "MultifunctionAdapter".
        //
        RtlAppendUnicodeToString(&RegistryPath,L"MultifunctionAdapter\\");

        //
        // Find "ISA" bus node..
        //
        BusNumber = FindIsaBusNode();

        if (BusNumber == (ULONG)-1 ) {
            //
            // Could not find "ISA" bus, how we can handle this ???.
            //
            BusNumber = 0;
        }
    }

    //
    // Add Bus number.
    //
    RtlIntegerToUnicodeString((ULONG)BusNumber,10,&Number);
    RtlAppendUnicodeStringToString(&RegistryPath,&Number);
    RtlAppendUnicodeToString(&RegistryPath,L"\\");

    //
    // Add "KeyboardController"
    //
    RtlAppendUnicodeToString(&RegistryPath,L"KeyboardController\\");

    //
    // Add Controller number.
    //
    RtlIntegerToUnicodeString((ULONG)ControllerNumber,10,&Number);
    RtlAppendUnicodeStringToString(&RegistryPath,&Number);
    RtlAppendUnicodeToString(&RegistryPath,L"\\");

    //
    // Add "KeyboardPeripheral"
    //
    RtlAppendUnicodeToString(&RegistryPath,L"KeyboardPeripheral\\");

    //
    // Add Peripheral number.
    //
    RtlIntegerToUnicodeString((ULONG)PeripheralNumber,10,&Number);
    RtlAppendUnicodeStringToString(&RegistryPath,&Number);

    //
    // Open the key
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);

    if (NT_SUCCESS(Status)) {

        UNICODE_STRING ValueName;

        RtlInitUnicodeString(&ValueName,L"Identifier");
    
        //
        // Save the value.
        //
        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_SZ,
                               DeviceIdentifier,
                               (wcslen(DeviceIdentifier) + 1) * sizeof(WCHAR));

        ZwClose(KeyHandle);
    }

    return (Status);
}
#endif // WINDOWS_FE
#endif
