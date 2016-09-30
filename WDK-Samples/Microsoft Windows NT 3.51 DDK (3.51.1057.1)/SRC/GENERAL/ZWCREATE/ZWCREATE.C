/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    zwcreate.c

Abstract:

    This is a ZwCreateFile sample driver.

Author:

    Paul Sanders Jan 28, 1995.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/


//
// Include files.
//

#include <ntddk.h>          // various NT definitions
// #include <ntdddisk.h>       // disk device driver I/O control codes
#include <ntiologc.h>

#include <string.h>

#include "zwcreate.h"

#if DBG
ULONG   ZwCFDebugLevel = 0;
#endif


NTSTATUS
DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    )
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

    It fills in the dispatch entry points in the driver object.  Then
    ZwCFInitialize is called to create the device object and complete
    the initialization.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if this disk is initialized; an error otherwise.

--*/

{
    NTSTATUS        	ntStatus;
    UNICODE_STRING  	paramPath;
	UNICODE_STRING		fileName;
    static  WCHAR   	SubKeyString[] = L"\\Parameters";

    //
    // The registry path parameter points to our key, we will append
    // the Parameters key and look for any additional configuration items
    // there.  We add room for a trailing NUL for those routines which
    // require it.

    paramPath.MaximumLength = RegistryPath->Length + sizeof(SubKeyString);
    paramPath.Buffer = ExAllocatePool(PagedPool, paramPath.MaximumLength);

    if (paramPath.Buffer != NULL)
    {
        RtlMoveMemory(
            paramPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);

        RtlMoveMemory(
            &paramPath.Buffer[RegistryPath->Length / 2], SubKeyString,
            sizeof(SubKeyString));

        paramPath.Length = paramPath.MaximumLength;
    }
    else
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if DBG
    {
        //
        // We use this to query into the registry as to whether we
        // should break at driver entry.
        //
    
        RTL_QUERY_REGISTRY_TABLE    paramTable[3];
        ULONG                       zero = 0;
    
        ULONG                       debugLevel = 0;
        ULONG                       shouldBreak = 0;
    
        RtlZeroMemory(&paramTable[0], sizeof(paramTable));
    
        paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name = L"BreakOnEntry";
        paramTable[0].EntryContext = &shouldBreak;
        paramTable[0].DefaultType = REG_DWORD;
        paramTable[0].DefaultData = &zero;
        paramTable[0].DefaultLength = sizeof(ULONG);
        
        paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name = L"DebugLevel";
        paramTable[1].EntryContext = &debugLevel;
        paramTable[1].DefaultType = REG_DWORD;
        paramTable[1].DefaultData = &zero;
        paramTable[1].DefaultLength = sizeof(ULONG);
        
        if (!NT_SUCCESS(RtlQueryRegistryValues(
            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
            paramPath.Buffer, &paramTable[0], NULL, NULL)))
        {
            shouldBreak = 0;
            debugLevel = 0;
        }
        
        ZwCFDebugLevel = debugLevel;
        
        if (shouldBreak)
        {
            DbgBreakPoint();
        }
    }
#endif

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ZwCFCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ZwCFCreateClose;
    DriverObject->DriverUnload = ZwCFUnloadDriver;

    ntStatus = ZwCFInitialize(DriverObject, &paramPath);

    //
    // We don't need that path anymore.
    //

    if (paramPath.Buffer)
    {
        ExFreePool( paramPath.Buffer );
    }

    if ( !NT_SUCCESS( ntStatus ) )
    {
        ZwCFDump(
            ZWCFERRORS, ("ZwCF: Problems in ZwCFInitialize routine\n"));
        goto ZwCFDriverEntryExit;
    }

    //
    // Allocate and initialize a Unicode String containing the name of
    // the file to be opened and read.
    //

    RtlInitUnicodeString( &fileName, DEFAULT_FILE_NAME );
	//
	// Open a file using ZwCreateFile and then read the contents of the
	// file into a buffer
	//

	ntStatus = ZwCFOpenAndReadFile(&fileName);

ZwCFDriverEntryExit:

    return ntStatus;
}

NTSTATUS
ZwCFInitialize(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  ParamPath
    )

/*++

Routine Description:
    This routine is called at initialization time by DriverEntry().

    It creates and initializes a device object.

    The filename to be opened may be configured using the registry.  

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    ParamPath - a pointer to the Parameters key under our key in the Services
        section of the registry.

Return Value:
    STATUS_SUCCESS if this disk is initialized; an error otherwise.

--*/

{
    UNICODE_STRING      ntUnicodeString;    // NT Device Name "\Device\ZwCF"
    UNICODE_STRING      Win32NameString;    // Win32 Name "\DosDevices\ZwTest"
    
    PDEVICE_OBJECT      deviceObject = NULL;    // ptr to device object
    
    NTSTATUS            ntStatus;

    RtlInitUnicodeString( &ntUnicodeString, NT_DEVICE_NAME );
    
    ntStatus = IoCreateDevice(
        DriverObject,                   // Our Driver Object
        0,                     			// We don't use a device extension
        &ntUnicodeString,               // Device name "\Device\ZwCF"
        FILE_DEVICE_UNKNOWN,       		// Device type
        0,                              // Device characteristics
        FALSE,                          // Not an exclusive device
        &deviceObject );                // Returned ptr to Device Object

    if ( !NT_SUCCESS( ntStatus ) )
    {
        ZwCFDump(
            ZWCFERRORS, ("ZwCF: Couldn't create the device object\n"));
        goto ZwCFInitializeExit;
    }
    
    //
    // Allocate and initialize a Unicode String containing the Win32 name
    // for our device.
    //

    RtlInitUnicodeString( &Win32NameString, DOS_DEVICE_NAME );

    //
    // Create a symbolic link between our device name "\Device\ZwCF" and
    // the Win32 name (ie "\DosDevices\ZwCF").
    //

    ntStatus = IoCreateSymbolicLink(
                        &Win32NameString, &ntUnicodeString );

                        
ZwCFInitializeExit:

    if ( !NT_SUCCESS( ntStatus ) )
    {
        //
        // Delete everything that this routine has allocated.
        //
        
        if ( deviceObject != NULL )
        {
            IoDeleteDevice( deviceObject );
        }
    }

    return ntStatus;
}

NTSTATUS
ZwCFCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when the ZwCF is opened or
    closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_INVALID_PARAMETER if parameters are invalid,
    STATUS_SUCCESS otherwise.

--*/

{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    return STATUS_SUCCESS;
}

VOID
ZwCFUnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine is called by the I/O system to unload the driver.

    Any resources previously allocated must be freed.

Arguments:

    DriverObject - a pointer to the object that represents our driver.

Return Value:

    None
--*/

{
    PDEVICE_OBJECT      deviceObject = DriverObject->DeviceObject;

    UNICODE_STRING uniWin32NameString;

    if ( deviceObject != NULL )
    {
        IoDeleteDevice( deviceObject );
    }

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );


    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //
    
    IoDeleteSymbolicLink( &uniWin32NameString );

}


NTSTATUS
ZwCFOpenAndReadFile(
	PUNICODE_STRING fileName
	)
/*++

Routine Description:

    This routine opens a file for future mapping and reads its contents
    into allocated memory.

Arguments:

    fileName - The name of the file

Return Value:

    STATUS_INSUFFICIENT_RESOURCES if not enough memory for filename
         or fileimage buffer, or
    STATUS_NO_SUCH_FILE if the file cannot be opened,
    STATUS_UNSUCCESSFUL if the length of the read file is 1, or if the
         file cannot be read.
                        
    STATUS_SUCCESS otherwise.

--*/
{
   NTSTATUS ntStatus;
   IO_STATUS_BLOCK IoStatus;
   HANDLE NtFileHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   ULONG LengthOfFile;
   WCHAR PathPrefix[] = L"\\SystemRoot\\system32\\drivers\\";
   UNICODE_STRING FullFileName;
   ULONG FullFileNameLength;
   PVOID FileImage;

   FILE_STANDARD_INFORMATION StandardInfo;

   //
   // Insert the correct path prefix.
   //

   FullFileNameLength = sizeof(PathPrefix) + fileName->MaximumLength;

   FullFileName.Buffer = ExAllocatePool(NonPagedPool,
                                       FullFileNameLength);

   if (FullFileName.Buffer == NULL) {
       ntStatus = STATUS_INSUFFICIENT_RESOURCES;
       return ntStatus;
   }

   FullFileName.Length = sizeof(PathPrefix) - sizeof(WCHAR);
   FullFileName.MaximumLength = (USHORT)FullFileNameLength;
   RtlMoveMemory (FullFileName.Buffer, PathPrefix, sizeof(PathPrefix));

   RtlAppendUnicodeStringToString (&FullFileName, fileName);

   ZwCFDump(
       ZWCFDIAG1, ("ZwCF: Attempting to open %wZ\n", &FullFileName));

   InitializeObjectAttributes ( &ObjectAttributes,
                                &FullFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

   ntStatus = ZwCreateFile( &NtFileHandle,
                            SYNCHRONIZE | FILE_READ_DATA,
                            &ObjectAttributes,
                            &IoStatus,
                            NULL,                          // alloc size = none
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ,
                            FILE_OPEN,
                            FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,  // eabuffer
                            0 );   // ealength

    if ( !NT_SUCCESS( ntStatus ) )
    {
        ZwCFDump(
            ZWCFERRORS, ("ZwCF: Error opening file %x\n", ntStatus));

        ExFreePool(FullFileName.Buffer);
        ntStatus = STATUS_NO_SUCH_FILE;
        return ntStatus;
    }

   ExFreePool(FullFileName.Buffer);

   //
   // Query the object to determine its length.
   //

   ntStatus = ZwQueryInformationFile( NtFileHandle,
                                      &IoStatus,
                                      &StandardInfo,
                                      sizeof(FILE_STANDARD_INFORMATION),
                                      FileStandardInformation );

   if (!NT_SUCCESS(ntStatus)) {
        ZwCFDump(
            ZWCFERRORS, ("Error querying info on file %x\n", ntStatus));
        ZwClose( NtFileHandle );
        return ntStatus;
   }

   LengthOfFile = StandardInfo.EndOfFile.LowPart;

   ZwCFDump(
       ZWCFDIAG1, ("File length is %d\n", LengthOfFile));

   //
   // Might be corrupted.
   //

   if( LengthOfFile < 1 )
   {
        ZwCFDump(
            ZWCFDIAG2, ("Bad file length %d\n", LengthOfFile));

        ZwClose( NtFileHandle );
        ntStatus = STATUS_UNSUCCESSFUL;
        return ntStatus;
   }

   //
   // Allocate buffer for this file
   //

   FileImage = ExAllocatePool( NonPagedPool,
                             LengthOfFile );

   if( FileImage == NULL )
   {
        ZwCFDump(
            ZWCFERRORS, ("Could not allocate buffer\n"));
        ZwClose( NtFileHandle );
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        return ntStatus;
   }

   //
   // Read the file into our buffer.
   //

   ntStatus = ZwReadFile( NtFileHandle,
                          NULL,
                          NULL,
                          NULL,
                          &IoStatus,
                          FileImage,
                          LengthOfFile,
                          NULL,
                          NULL );

   if( (!NT_SUCCESS(ntStatus)) || (IoStatus.Information != LengthOfFile) )
   {
        ZwCFDump(
            ZWCFERRORS, ("error reading file %x\n", ntStatus));
        ntStatus = STATUS_UNSUCCESSFUL;
        ExFreePool( FileImage );
        return ntStatus;
   }

   ZwClose( NtFileHandle );

   ExFreePool( FileImage );

   return ntStatus;

}
