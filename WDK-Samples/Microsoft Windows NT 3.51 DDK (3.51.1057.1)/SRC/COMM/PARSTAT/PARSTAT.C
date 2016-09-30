//////////////////////////////////////////////////////////////////////////////
// Program Name: ParStat.c
// Programmer: Mark A. Overby (MarkOv)
//             Norbert Kusters (NorbertK)
// Program Description: This is a simple parallel class driver, which supports
//    just getting the parallel port status for a specific parallel port.
//    Routine layout and design was taken from parsimp.c (written by NorbertK)
//
// PORTIONS OF THIS CODE WRITTEN BY Norbert Kusters (NorbertK)
//
// Copyright (c) 1994 Microsoft Corporation
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// INCLUDES & DEFINES
//////////////////////////////////////////////////////////////////////////////

#include "ntddk.h"
#include "parallel.h"
#include "ps.h"
#include "parstat.h"

//////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// DriverEntry () - This function is called at driver init time as the entry
//                  point for when the driver is first loaded.
//
// Arguments:       DriverObject - Supplies the Driver Object
//                  RegistryPath - Supplies the Registry Path for this driver
//
// ===========================================================================

NTSTATUS DriverEntry (IN PDRIVER_OBJECT DriverObject,
                      IN PUNICODE_STRING RegistryPath)
{
   ULONG x, Break ;
   UNICODE_STRING szBreakKey, szDebugKey, ParsedString ;

   KdPrint (("ParStat: Entering DriverEntry\n")) ;

//   _asm int 3 ;

   // Now we go into a loop equalling the number of parallel ports that the
   // IO Manager reports being in the system. In this loop, we will initalize
   // a device object for each parallel port that is in the system.

   KdPrint (("ParStat: Entering loop for device objects.\n")) ;
   for (x = 0; x < IoGetConfigurationInformation()->ParallelCount; x++) {
      ParStatInitalizeDeviceObject (DriverObject, x) ;
   }

   // Now we check to see if any device objects were created. If not, then we
   // must not have any parallel ports in the system.

   KdPrint (("ParStat: Checking existance of at least 1 device object.\n")) ;
   if (!DriverObject->DeviceObject) {
      KdPrint (("ParStat: Failed! Returning STATUS_NO_SUCH_DEVICE\n")) ;
      return(STATUS_NO_SUCH_DEVICE) ;
   }

   // Since we have at least one device object. We now initalize entry points
   // for the functions on this driver.

   KdPrint (("ParStat: Setting up entry points for driver functions.\n")) ;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = ParStatCreate ;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = ParStatClose ;
   DriverObject->MajorFunction[IRP_MJ_READ] = ParStatRead ;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = ParStatCleanup ;
   DriverObject->DriverUnload = ParStatUnload ;

   // Now, everything (initalization wise) is done. We can return success.

   KdPrint (("ParStat: DriverEntry Success!\n")) ;

   // Call routine to log success.

   KdPrint (("ParStat: Logging success for driver entry.\n")) ;
   ParStatLogInformation (DriverObject, NULL, PARSTAT_SUCCESSFUL_LOAD,
                          STATUS_SUCCESS) ;

   return(STATUS_SUCCESS) ;

}

// ===========================================================================
// ParStatLogInformation () - This function allocates an error log entry and
//          logs the appropriate code into the error log for a user to look at
//          later for his or her information. Allows retail "diagnosis" of
//          problems with driver.
//
// Arguments: DrvObj - Driver Object.
//            DevObj - Device Object.
//            Code - Event viewer log code (driver defined).
//            FinalStatus - NTSTATUS Code being returned for Irp.
// ===========================================================================

VOID ParStatLogInformation (IN PDRIVER_OBJECT DrvObj,
                            IN PDEVICE_OBJECT DevObj,
                            IN NTSTATUS       Code,
                            IN NTSTATUS       FinalStatus)
{
   PIO_ERROR_LOG_PACKET    LogEntry ;

   KdPrint (("ParStat: Entered ParStatLogInformation.\n")) ;

   // Two different possibilties based on if a DeviceObject or a DriverObject
   // was passed in as a parameter. If DevObj == NULL, then we know to use
   // the driver object, otherwise we will use the device object.

   KdPrint (("ParStat: Checking for existance of second parameter.\n")) ;
   if (DevObj == NULL) {

      // We are going to use the driver object since the device object was
      // NULL.

      KdPrint (("ParStat: 2nd Param == NULL, using DriverObject.\n")) ;
      LogEntry = IoAllocateErrorLogEntry (DrvObj, sizeof (IO_ERROR_LOG_PACKET)) ;

   }
   else {

      // We will use the device object, becuase it was not == NULL.

      KdPrint (("ParStat: 2nd Param != NULL, using DeviceObject.\n")) ;
      LogEntry = IoAllocateErrorLogEntry (DevObj, sizeof (IO_ERROR_LOG_PACKET)) ;

   }

   // We will now check to make sure that it was allocated.

   KdPrint (("ParStat: Checking buffer for error log packet.\n")) ;
   if (!LogEntry) {

      // Failed!

      KdPrint (("ParStat: Could not allocate buffer for packet.\n")) ;
      KdPrint (("ParStat: Leaving ParStatLogInformation.\n")) ;
      return;
   }

   // We have a packet, so now we must fill in the structures accordingly.

   KdPrint (("ParStat: Filling in allocated error log entry.\n")) ;
   LogEntry->RetryCount = 0 ;
   LogEntry->DumpDataSize = 0 ;
   LogEntry->NumberOfStrings = 0 ;
   LogEntry->StringOffset = 0 ;
   LogEntry->ErrorCode = Code ;
   LogEntry->UniqueErrorValue = 0 ;
   LogEntry->FinalStatus = FinalStatus ;
   LogEntry->SequenceNumber = 0 ;

   // Error log entry filled in. Log it.

   KdPrint (("ParStat: Logging entry.\n")) ;
   IoWriteErrorLogEntry (LogEntry) ;

   // Done.

   KdPrint (("ParStat: Leaving ParStatLogInformation.\n")) ;
   return;

}

// ===========================================================================
// ParStatInitalizeDeviceObject () - This function (each time it is called)
//          creates a device object for the current parallel port number which
//          is passed in. At this time we also connect this device object to
//          the port object for that port.
//
// Arguments:     DriverObject - The driver object for this driver
//                ParallelPortNum - The number for this instance of the port
// ===========================================================================

VOID ParStatInitalizeDeviceObject (IN PDRIVER_OBJECT DriverObject,
                                   IN ULONG ParallelPortNum)
{
   UNICODE_STRING    szPortName, szClassName, szLinkName ;
   NTSTATUS          Status ;
   PDEVICE_OBJECT    deviceObject ;
   PDEVICE_EXTENSION extension ;
   PFILE_OBJECT      fileObject ;

   KdPrint (("ParStat: Entering ParStatInitalizeDeviceObject.\n")) ;

   // Now we create the strings for the port and class names. We call our
   // routine of ParStatMakeNames to do this.

   KdPrint (("ParStat: Calling ParStatMakeNames.\n")) ;
   if (!ParStatMakeNames (ParallelPortNum, &szPortName, &szClassName, &szLinkName)) {
      KdPrint (("ParStat: Failed parsing names, no device object.\n")) ;
      return ;
   }

   // We got the names parsed successfully. Now we will create our device
   // object.

   KdPrint (("ParStat: Creating device object for #%d\n", ParallelPortNum));
   Status = IoCreateDevice (DriverObject, sizeof (DEVICE_EXTENSION),
                            &szClassName, FILE_DEVICE_PARALLEL_PORT, 0,
                            FALSE, &deviceObject) ;

   // Check for failure on the IoCreateDeviceCall.

   KdPrint (("ParStat: Checking return from IoCreateDevice.\n")) ;
   if (!NT_SUCCESS(Status)) {
      KdPrint (("ParStat: IoCreateDevice failed!\n")) ;

      // In order for us to clean up correctly, we must free the pool for
      // the link buffer that we created in ParStatMakeNames. After that is
      // done, we jump to the location of the general always called clean
      // up routines.

      ExFreePool (szLinkName.Buffer) ;
      goto Cleanup ;
   }

   // Ok, we have our device object now. We allocated a device extension
   // for this device object. We will now initalize the device extension.

   KdPrint (("ParStat: Initalizing device extension.\n")) ;
   extension = deviceObject->DeviceExtension ;
   RtlZeroMemory (extension, sizeof(DEVICE_EXTENSION)) ;

   // Now comes the always interesting setting of the device extension
   // member DeviceObject equal to the DeviceObject which owns this
   // device extension.

   extension->DeviceObject = deviceObject ;

   // Since the parallel port driver does BUFFERED_IO, we set out device
   // object to reflect this.

   KdPrint (("ParStat: Setting flags in deviceObject to do buffered IO.\n")) ;
   deviceObject->Flags |= DO_BUFFERED_IO ;

   // Now we must get the information about the parallel port driver so that
   // we can store it in the device extension for later use for IoCallDriver
   // type calls. We will store the device object of the port driver in our
   // device extension.

   KdPrint (("ParStat: Now getting port driver device object pointer.\n")) ;
   Status = IoGetDeviceObjectPointer (&szPortName, FILE_READ_ATTRIBUTES,
                                      &fileObject,
                                      &extension->PortDeviceObject) ;

   // Check for success.

   KdPrint (("ParStat: Checking return from IoGetDeviceObjectPointer.\n")) ;
   if (!NT_SUCCESS(Status)) {

      // At this stage in the code, for us to clean up correctly, we must
      // free the device object, the LinkName pool that we allocated, and
      // then jump to the appropriate general clean up routines.

      KdPrint (("ParStat: Failure! Freeing device, and szLinkName pool.\n")) ;
      IoDeleteDevice (deviceObject) ;
      ExFreePool (szLinkName.Buffer) ;
      goto Cleanup ;
   }

   // Ok, that worked. Now we can release the file object (not that we really
   // ever needed it.)

   KdPrint (("ParStat: Releasing fileObject.\n")) ;
   ObDereferenceObject (fileObject) ;

   // Now we must insure that when we get an IRP that we have enough stack
   // locations in the IRP to accomodate the fact that we have one level
   // below us that will be receiving most IRP's that we get.

   KdPrint (("ParStat: Setting DeviceObject->StackSize +1.\n")) ;
   extension->DeviceObject->StackSize =
      extension->PortDeviceObject->StackSize + 1 ;

   // With that set, we will set up our work queue for our driver.

   KdPrint (("ParStat: Initalizing work queue.\n")) ;
   InitializeListHead (&extension->WorkQueue) ;
   extension->CurrentIrp = NULL ;

   // We will now get the port information for this instance and store it
   // in the device extension. We will call one of our driver routines to
   // do this.

   KdPrint (("ParStat: Calling ParStatGetPortInfoFromPortDevice.\n")) ;
   Status = ParStatGetPortInfoFromPortDevice (extension) ;

   // Check for success.

   KdPrint (("ParStat: Checking status from previous call.\n")) ;
   if (!NT_SUCCESS(Status)) {

      // To clean up after ourselves, we must delete the device, free the
      // szLinkName pool, and execute the other general clean up routines.

      KdPrint (("ParStat: Failed! Releasing resources.\n")) ;
      IoDeleteDevice (deviceObject) ;
      ExFreePool (szLinkName.Buffer) ;
      goto Cleanup ;
   }

   // Now for our friendly user-mode applications that this parallel class
   // driver was designed for, we must setup our symbolic links to the
   // user-mode subsystem. We will create unprotected symbolic links so that
   // they can be redirected and changed from the user-mode. (All comm class
   // drivers should do this in that manner.)

   KdPrint (("ParStat: Setting up symbolic links.\n")) ;
   Status = IoCreateUnprotectedSymbolicLink (&szLinkName, &szClassName) ;

   // Check for success.

   KdPrint (("ParStat: Checking for success.\n")) ;
   if (!NT_SUCCESS(Status)) {

      // We failed. We won't delete the device or such, we will just free pool
      // that we have allocated and set the device extension flag that
      // indicates that we failed to create the symbolic link.

      KdPrint (("ParStat: Failed! Releasing pool, set dev extension flag.\n"));
      extension->CreatedSymbolicLink = FALSE ;
      ExFreePool (szLinkName.Buffer) ;
      goto Cleanup ;
   }

   // Ok. Mark the fact that we did create a symbolic link (for cleanup
   // reasons) and let's get out of Dodge.

   KdPrint (("ParStat: Marking extension about symbolic link creation.\n")) ;
   KdPrint (("ParStat: Also, putting symbolic link name in dev extension.\n")) ;
   extension->CreatedSymbolicLink = TRUE ;
   extension->SymbolicLinkName = szLinkName ;

   // Time to get out of Dodge. Final cleanup (always called).

   Cleanup:
   KdPrint (("ParStat: Cleanup: area, freeing resources.\n")) ;
   ExFreePool (szPortName.Buffer) ;
   ExFreePool (szClassName.Buffer) ;

   // Done!
   KdPrint (("ParStat: Done! Returning to caller.\n")) ;

}

// ===========================================================================
// ParStatMakeNames () - This function parses the names of the various
//            components of the system that we will be using for linkage.
//
// Arguments: ParallelPortNum - The port number for this call
//            PortName - Returns the port name
//            ClassName - Returns the class name
//            LinkName - Returning the linkage name
//
// NB: Totally (except comments and KdPrint's Norbert's code ...)
// ===========================================================================

BOOLEAN ParStatMakeNames (IN  ULONG ParallelPortNum,
                          OUT PUNICODE_STRING PortName,
                          OUT PUNICODE_STRING ClassName,
                          OUT PUNICODE_STRING LinkName)
{

   UNICODE_STRING prefix, digits, linkPrefix, linkDigits ;
   WCHAR          digitsBuffer[10], linkDigitsBuffer[10] ;
   UNICODE_STRING portSuffix, classSuffix, linkSuffix ;
   NTSTATUS       Status ;

   KdPrint (("ParStat: Entering ParStatMakeNames.\n")) ;

   // We now will setup all of the local variables to parse together into
   // the names that we will return back from this call.

   KdPrint (("ParStat: Setting up local routine names for parsing.\n")) ;
   RtlInitUnicodeString (&prefix, L"\\Device\\") ;
   RtlInitUnicodeString (&linkPrefix, L"\\DosDevices\\") ;
   RtlInitUnicodeString (&portSuffix, DD_PARALLEL_PORT_BASE_NAME_U) ;
   RtlInitUnicodeString (&classSuffix, L"ParallelStat") ;
   RtlInitUnicodeString (&linkSuffix, L"LPTSTAT") ;
   digits.Length = 0 ;
   digits.MaximumLength = 20 ;
   digits.Buffer = digitsBuffer ;
   linkDigits.Length = 0 ;
   linkDigits.MaximumLength = 20 ;
   linkDigits.Buffer = linkDigitsBuffer ;

   // Ok, now let us convert the ParallelPortNum to a unicode string so that
   // we can append it in the correct location later.

   KdPrint (("ParStat: Converting num to unicode string for parsing.\n")) ;
   Status = RtlIntegerToUnicodeString (ParallelPortNum, 10, &digits) ;

   // Check for success.

   KdPrint (("ParStat: Checking for success.\n")) ;
   if (!NT_SUCCESS(Status)) {

      // We failed. Nothing to free, so we just return a failure.

      KdPrint (("ParStat: Failed! Returning FALSE.\n")) ;
      return(FALSE) ;
   }

   // Ok, now let us convert the ParallelPortNum to a unicode string so that
   // we can append it to the link string (adding 1 first so that we get
   // LPTFOO1 instead of LPTFOO0).

   KdPrint (("ParStat: Converting num to unicode string (+1) for link.\n")) ;
   Status = RtlIntegerToUnicodeString (ParallelPortNum + 1, 10, &linkDigits) ;

   // Check for success.

   KdPrint (("ParStat: Checking for success.\n")) ;
   if (!NT_SUCCESS(Status)) {

      // We failed. Nothing to free, so we just return a failure.

      KdPrint (("ParStat: Failed! Returning FALSE.\n")) ;
      return(FALSE) ;
   }

   // Ok, now we setup the buffers and unicode string members for the port
   // name.

   KdPrint (("ParStat: Setting up port name unicode string buffers.\n")) ;
   PortName->Length = 0 ;
   PortName->MaximumLength = prefix.Length + portSuffix.Length +
                             digits.Length + sizeof (WCHAR) ;

   // Try to allocate paged pool for the buffer.

   KdPrint (("ParStat: Trying to allocate paged pool for buffer.\n")) ;
   PortName->Buffer = ExAllocatePool (PagedPool, PortName->MaximumLength) ;

   // Check for success.

   KdPrint (("ParStat: Checking for success.\n")) ;
   if (!PortName->Buffer) {

      // We failed, since this is the first resource we tried and it failed,
      // ... nothing to free. Just return a failure.

      return(FALSE) ;
   }

   // That worked so zero out the memory and parse in the pieces of the name
   // into the newly allocated buffer.

   KdPrint (("ParStat: Zeroing buffer and parsing strings together.\n")) ;
   RtlZeroMemory (PortName->Buffer, PortName->MaximumLength) ;
   RtlAppendUnicodeStringToString (PortName, &prefix) ;
   RtlAppendUnicodeStringToString (PortName, &portSuffix) ;
   RtlAppendUnicodeStringToString (PortName, &digits) ;

   // Time for the class name. Let's setup the lengths for the unicode
   // string.

   KdPrint (("ParStat: Setting unicode string lengths for class name.\n")) ;
   ClassName->Length = 0 ;
   ClassName->MaximumLength = prefix.Length + classSuffix.Length +
                              digits.Length + sizeof (WCHAR) ;

   // Try and allocated paged pool for the buffer.

   KdPrint (("ParStat: Trying to allocate paged pool for buffer.\n")) ;
   ClassName->Buffer = ExAllocatePool (PagedPool, ClassName->MaximumLength) ;

   // Check for success.

   KdPrint (("ParStat: Checking for success.\n")) ;
   if (!ClassName->Buffer) {

      // We failed. Since one buffer has already been allocated, we must
      // free it before continuing.

      KdPrint (("ParStat: Failed! Releasing buffers and returning FALSE.\n")) ;
      ExFreePool (PortName->Buffer) ;
      return(FALSE) ;
   }

   // That worked, so zero out the memory and parse in the pieces of the name
   // into the newly allocated buffer.

   KdPrint (("ParStat: Zeroing buffer and parsing strings together.\n")) ;
   RtlZeroMemory (ClassName->Buffer, ClassName->MaximumLength) ;
   RtlAppendUnicodeStringToString (ClassName, &prefix) ;
   RtlAppendUnicodeStringToString (ClassName, &classSuffix) ;
   RtlAppendUnicodeStringToString (ClassName, &digits) ;

   // And last, set up the lengths for the symbolic link name.

   KdPrint (("ParStat: Setting unicode string lengths for link name.\n")) ;
   LinkName->Length = 0 ;
   LinkName->MaximumLength = linkPrefix.Length + linkSuffix.Length +
                             linkDigits.Length + sizeof (WCHAR) ;

   // Try and allocated paged pool for the buffer.

   KdPrint (("ParStat: Trying to allocate paged pool for buffer.\n")) ;
   LinkName->Buffer = ExAllocatePool (PagedPool, LinkName->MaximumLength) ;

   // Check for success.

   KdPrint (("ParStat: Checking for success.\n")) ;
   if (!LinkName->Buffer) {

      // We failed. Since one buffer has already been allocated, we must
      // free it before continuing.

      KdPrint (("ParStat: Failed! Releasing buffers and returning FALSE.\n")) ;
      ExFreePool (PortName->Buffer) ;
      ExFreePool (ClassName->Buffer) ;
      return(FALSE) ;
   }

   // That worked, so zero out the memory and parse in the pieces of the name
   // into the newly allocated buffer.

   KdPrint (("ParStat: Zeroing buffer and parsing strings together.\n")) ;
   RtlZeroMemory (LinkName->Buffer, LinkName->MaximumLength) ;
   RtlAppendUnicodeStringToString (LinkName, &linkPrefix) ;
   RtlAppendUnicodeStringToString (LinkName, &linkSuffix) ;
   RtlAppendUnicodeStringToString (LinkName, &linkDigits) ;

   // We are complete. Returning True.

   KdPrint (("ParStat: Success in all! Returning TRUE\n")) ;
   return(TRUE) ;

}

// ===========================================================================
// ParStatGetPortInfoFromPortDevice () - This function allows us to request
//       information about the parallel ports that the port driver has
//       reserved for us.
//
// Arguments: Extension - Device extension.
//
// ===========================================================================

NTSTATUS ParStatGetPortInfoFromPortDevice (IN OUT PDEVICE_EXTENSION Extension)
{


   KEVENT   kEvent ;    // Used to signal that the IOCTL is done
   PIRP     Irp ;
   PARALLEL_PORT_INFORMATION portInfo ;
   IO_STATUS_BLOCK ioStatus ;
   NTSTATUS Status ;

   KdPrint (("ParStat: Entered ParStatGetPortInfoFromPortDevice.\n")) ;

   // First thing we need to do is initalize the event so that when we send
   // down the IOCTL to the port driver we have an event to wait on.

   KdPrint (("ParStat: Initializing kernel event for IOCTL.\n")) ;
   KeInitializeEvent (&kEvent, NotificationEvent, FALSE) ;

   // Now build the IRP:
   // We want the IOCTL code to be IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO,
   // which is one of the 3 codes that the port driver currently supports.
   // It will return the information we seek into the portInfo buffer that
   // we pass down. No need for any input buffer, only an output buffer.

   KdPrint (("ParStat: Building Irp.\n")) ;
   Irp = IoBuildDeviceIoControlRequest (IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO,
                                        Extension->PortDeviceObject,
                                        NULL, 0, &portInfo,
                                        sizeof(PARALLEL_PORT_INFORMATION),
                                        TRUE, &kEvent, &ioStatus) ;

   // Check to see if the IRP was built.

   KdPrint (("ParStat: Validating IRP was built.\n")) ;
   if (!Irp) {

      // We failed. Must not have enough resources. So we report that back.

      KdPrint (("ParStat: Failed! Returning STATUS_INSUFFICIENT_RESOURCES\n")) ;
      return(STATUS_INSUFFICIENT_RESOURCES) ;
   }

   // Now that we have the IRP, let's send it down to the port driver.

   KdPrint (("ParStat: Sending IRP down to port driver.\n")) ;
   Status = IoCallDriver (Extension->PortDeviceObject, Irp) ;

   // Let's check to make sure something didn't go wrong.

   KdPrint (("ParStat: Checking success of IoCallDriver.\n")) ;
   if (!NT_SUCCESS(Status)) {

      // We failed. We return the reason we failed.

      KdPrint (("ParStat: Failure! Returning cause.\n")) ;
      return(Status) ;
   }

   // Now, we will hang around and wait for the Irp to complete.

   KdPrint (("ParStat: Waiting on event signal for completion of Irp.\n")) ;
   Status = KeWaitForSingleObject (&kEvent, Executive, KernelMode, FALSE, NULL) ;

   // Let's check to make sure we didn't bomb something.

   KdPrint (("ParStat: Checking success of wait.\n")) ;
   if (!NT_SUCCESS(Status)) {

      // We failed. We return the reason we failed.

      KdPrint (("ParStat: Failure! Returning cause.\n")) ;
      return(Status) ;
   }

   // Put the information we just received into the device extension.

   KdPrint (("ParStat: Storing par port info in device extension.\n")) ;

   // Original port address

   Extension->OriginalController = portInfo.OriginalController ;

   // Port address we will use

   Extension->Controller = portInfo.Controller ;

   // Span of ports

   Extension->SpanOfController = portInfo.SpanOfController ;

   // FreePort routine for releasing an allocated port

   Extension->FreePort = portInfo.FreePort ;

   // FreePort context used when calling freeport

   Extension->FreePortContext = portInfo.Context ;

   // Let's check and make sure that the span of ports is large enough
   // for a parallel port.

   KdPrint (("ParStat: Checking validity of port span.\n")) ;
   if (Extension->SpanOfController < PARALLEL_REGISTER_SPAN) {

      // We failed. Must not have enough resources ... (Hmmm)

      KdPrint (("ParStat: Failed! Returning STATUS_INSUFFICIENT_RESOURCES\n")) ;
      return(STATUS_INSUFFICIENT_RESOURCES) ;
   }

   // Done!

   KdPrint (("ParStat: Leaving ParStatGetPortInfoFromPortDevice.\n")) ;
   return(Status) ;
}

// ===========================================================================
// ParStatCreate () - This is the entry point routine to handle
//                    IRP_MJ_CREATE requests. (e.g. CreateFile)
//
// Arguments: DeviceObject - Device object we are operating with
//            Irp - The Irp
// ===========================================================================

NTSTATUS ParStatCreate (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   KdPrint (("ParStat: Entered ParStatCreate.\n")) ;

   // Nothing really for us to do here except just complete this with
   // success.

   KdPrint (("ParStat: Setting Irp status and information fields.\n")) ;
   Irp->IoStatus.Status = STATUS_SUCCESS ;
   Irp->IoStatus.Information = 0 ;

   // Complete the request.

   KdPrint (("ParStat: Complete request.\n")) ;
   IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

   // Return the status and we are done.

   KdPrint (("ParStat: Completed with STATUS_SUCCESS. Leaving function.\n")) ;
   return(STATUS_SUCCESS) ;

}

// ===========================================================================
// ParStatClose () - This is the entry point routine to handle IRP_MJ_CLOSE
//                   requests. (e.g. CloseHandle)
//
// Arguments: DeviceObject - Device object we are operating with
//            Irp - The Irp
// ===========================================================================

NTSTATUS ParStatClose (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   KdPrint (("ParStat: Entered ParStatClose.\n")) ;

   // Nothing to do for us except just complete this with success.

   KdPrint (("ParStat: Setting Irp status and information fields.\n")) ;
   Irp->IoStatus.Status = STATUS_SUCCESS ;
   Irp->IoStatus.Information = 0 ;

   // Complete the request.

   KdPrint (("ParStat: Complete request.\n")) ;
   IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

   // Return the status and we are done.

   KdPrint (("ParStat: Completed w/ STATUS_SUCCESS. Leaving ParStatClose.\n")) ;
   return(STATUS_SUCCESS) ;
}

// ===========================================================================
// ParStatRead () - This is the entry point routine to handle IRP_MJ_READ
//                  requests. (e.g. ReadFile)
//
// Arguments: DeviceObject - Device object we are operating with
//            Irp - The Irp
// ===========================================================================

NTSTATUS ParStatRead (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{


   PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension ;
   KIRQL kCancelSpin ;
   BOOLEAN bAllocPort ;
   PIO_STACK_LOCATION pIrpStack ;

   KdPrint (("ParStat: Entered ParStatRead.\n")) ;

   // Get the current IrpStack information to check the buffers.

   KdPrint (("ParStat: Getting current Irp stack location.\n")) ;
   pIrpStack = IoGetCurrentIrpStackLocation (Irp) ;

   // Check the buffer to make sure that it is big enough to
   // hold a UCHAR. If not, fail it with STATUS_INVALID_PARAMETER. Not likely
   // that this will ever happen, but we just need to make sure.

   KdPrint (("ParStat: Checking size of return buffer.\n")) ;
   if (pIrpStack->Parameters.Read.Length < sizeof (UCHAR)) {

      // Nope, buffer too small.

      KdPrint (("ParStat: Failed! Buffer too small.\n")) ;
      return(STATUS_INVALID_PARAMETER) ;
   }

   // Next thing we need to do (since we are going to allow cancelable IRP's)
   // is to acquire the cancel spin lock. We do this so that no one else can
   // be changing the cancel state of the Irp while we are trying to do that.

   KdPrint (("ParStat: Acquiring CancelSpinLock.\n")) ;
   IoAcquireCancelSpinLock (&kCancelSpin) ;

   // Now that we know that we are safe ... check to see if we are already
   // working on an Irp with this device extension.

   KdPrint (("ParStat: Checking to see if we already have a current Irp.\n")) ;
   if (extension->CurrentIrp) {

      // We have a current Irp. So we will queue this Irp up and let it sit
      // in the device queue until ready for processing. At this time, we
      // will also set the cancel routine for this Irp in case it must be
      // cancelled.

      KdPrint (("ParStat: Current Irp found. Queueing this Irp.\n")) ;
      KdPrint (("ParStat: Setting cancel routine.\n")) ;
      IoSetCancelRoutine (Irp, ParStatCancel) ;
      InsertTailList (&extension->WorkQueue, &Irp->Tail.Overlay.ListEntry) ;
      bAllocPort = FALSE ;
   }
   else {

      // We do not currently have an Irp that we are working on. So we
      // indicate in the device extension that this is the current irp
      // that we are working on. We also indicate through the local variable
      // bAllocPort that we must allocate the parallel driver for our use
      // through a device io control to the port driver.

      KdPrint (("ParStat: No current Irp found. Setting Irp current.\n")) ;
      extension->CurrentIrp = Irp ;
      bAllocPort = TRUE ;
   }

   // Release the cancel spin lock so that someone else can use it.

   KdPrint (("ParStat: Releasing cancel spin lock.\n")) ;
   IoReleaseCancelSpinLock (kCancelSpin) ;

   // Now we must mark the Irp as pending. We do this so that the system
   // knows that there is further processing that we are going to do with
   // it and we are not done. (Because we did not complete in the dispatch
   // routine.

   KdPrint (("ParStat: Marking Irp as pending.\n")) ;
   IoMarkIrpPending (Irp) ;

   // Check to see if we must allocate the port, or if we already hold it
   // open.

   KdPrint (("ParStat: Checking to see if port needs to be allocated.\n")) ;
   if (bAllocPort) {

      // Yep, we must allocate the port.

      KdPrint (("ParStat: Calling ParStatAllocPort.\n")) ;
      ParStatAllocPort (extension) ;
   }

   // Since we have processing to do no matter what and we have not completed
   // the request, we return STATUS_PENDING

   KdPrint (("ParStat: Irp has not been completed. Returning STATUS_PENDING\n")) ;
   return(STATUS_PENDING) ;
}

// ===========================================================================
// ParStatAllocPort () - This function takes the device extension's current
//                       Irp and creates a Irp to allocate the port for our
//                       exclusive use.
//
// Parameters: Extension - The device extension.
// ===========================================================================

VOID ParStatAllocPort (IN PDEVICE_EXTENSION Extension)
{


   PIO_STACK_LOCATION   nextStack ;

   KdPrint (("ParStat: Entering ParStatAllocPort.\n")) ;

   // What we must do is get the next stack location and prepare that for
   // transmittal to the port driver. We'll start by getting the stack
   // location.

   KdPrint (("ParStat: Getting next stack location.\n")) ;
   nextStack = IoGetNextIrpStackLocation (Extension->CurrentIrp) ;

   // Now that we have the next stack location, we set the appropriate
   // control code and function code.

   KdPrint (("ParStat: Setting the control and function code in Irp.\n")) ;
   nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL ;
   nextStack->Parameters.DeviceIoControl.IoControlCode =
         IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE ;

   // Set the completion routine. This is mandatory since we have our own
   // device queue and have marked the Irp as pending. (This routine
   // is most likely to be called from the dispatch point)

   KdPrint (("ParStat: Setting the completion routine for this Irp.\n")) ;
   IoSetCompletionRoutine (Extension->CurrentIrp,
                           ParStatReadCompletionRoutine,
                           Extension, TRUE, TRUE, TRUE) ;

   // We are now ready to send the Irp down to the port driver.

   KdPrint (("ParStat: Sending Irp to port driver for processing.\n")) ;
   IoCallDriver (Extension->PortDeviceObject, Extension->CurrentIrp) ;

   // Done.

   KdPrint (("ParStat: Leaving ParStatAllocPort.\n")) ;
}

// ===========================================================================
// ParStatReadCompletionRoutine () - This function is the completion
//       routine that we set for all Irp's which are being sent down to the
//       port driver for processing.
//
// Arguments: DeviceObject - The current device object.
//            Irp - The current irp being worked on.
//            Extension - The current device object extension.
// ===========================================================================

NTSTATUS ParStatReadCompletionRoutine ( IN PDEVICE_OBJECT DeviceObject,
                                        IN PIRP Irp,
                                        IN PVOID Extension)
{


   PDEVICE_EXTENSION extension = Extension ;
   NTSTATUS Status ;
   PUCHAR ParPortStatus = (PUCHAR) Irp->AssociatedIrp.SystemBuffer ;
   PIO_STACK_LOCATION pIrpStack ;

   KdPrint (("ParStat: Entering ParStatAllocateCompletionRoutine.\n")) ;

   // First, let's check to see if we got the allocation OK.

   KdPrint (("ParStat: Checking for success on allocation.\n")) ;
   if (!NT_SUCCESS(Irp->IoStatus.Status)) {

      // Didn't work. So let's just fail out and continue with the next irp
      // in our device queue.

      KdPrint (("ParStat: Failed! Failing Irp and going to next in queue.\n")) ;
      Status = Irp->IoStatus.Status ;
      IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
      ParStatAllocPortWithNext (extension) ;
      return(Status) ;
   }

   // Ok, it worked. Now we go out and actually read the port. The data that
   // we retrieve is going into ParPortStatus which aliases into the Irp
   // system buffer that we will return.

   KdPrint (("ParStat: Reading port using READ_PORT_UCHAR.\n")) ;
   KdPrint (("ParStat: Data going into system buffer in Irp for return\n")) ;
   *ParPortStatus = READ_PORT_UCHAR (extension->Controller +
                                     PARALLEL_STATUS_OFFSET) ;

   // Set the information field of the Irp to indicate how many bytes we read.

   KdPrint (("ParStat: Setting buffer for number of bytes read.\n")) ;
   Irp->IoStatus.Information = sizeof (UCHAR) ;

   // Set the status to success and complete the request.

   KdPrint (("ParStat: Setting irp STATUS_SUCCESS and completing irp.\n")) ;
   Irp->IoStatus.Status = STATUS_SUCCESS ;
   IoCompleteRequest (Irp, IO_PARALLEL_INCREMENT) ;

   // Now free up the port that we allocated.

   KdPrint (("ParStat: Freeing allocated channel for port.\n")) ;
   extension->FreePort(extension->FreePortContext) ;

   // Go on to the next irp in the device queue. If there is nothing to be
   // processed, then we just kick out and return success and wait for the
   // entry point to be called again, otherwise we're going to recurse
   // back through this whole process again.

   KdPrint (("ParStat: Calling ParStatAllocPortWithNext.\n")) ;
   ParStatAllocPortWithNext (extension) ;

   // We're done with this round, returning STATUS_SUCCESS.

   KdPrint (("ParStat: Returning STATUS_MORE_PROCESSING_REQUIRED.\n")) ;
   return(STATUS_MORE_PROCESSING_REQUIRED) ;
}

// ===========================================================================
// ParStatAllocPortWithNext () - This function allows us to take the next
//       irp out of the queue (if there is one) and begin processing on it. If
//       there is no irps left in the queue to work on, we just return from
//       the routine.
//
// Arguments: Extension - The current device extension.
// ===========================================================================

VOID ParStatAllocPortWithNext (IN OUT PDEVICE_EXTENSION Extension)
{


   KIRQL kCancelSpin ;
   PLIST_ENTRY head ;

   KdPrint (("ParStat: Entered ParStatAllocPortWithNext.\n")) ;

   // First thing is (once again) since we will possible be modifying the
   // cancel areas of the irp we must acquire the cancel spin lock.

   KdPrint (("ParStat: Acquiring spin lock for cancel modification.\n")) ;
   IoAcquireCancelSpinLock (&kCancelSpin) ;

   // Now that we have that, reset the device extension so that we have no
   // current irp.

   KdPrint (("ParStat: Resetting device extension to show no current irp.\n")) ;
   Extension->CurrentIrp = NULL ;

   // We now check to see if there is anything in the list.

   KdPrint (("ParStat: Checking to see if device irp queue is empty.")) ;
   if (IsListEmpty (&Extension->WorkQueue)) {

      // Nothing to do. Release the spin lock.

      KdPrint (("ParStat: Nothing in queue. Releasing cancel spin lock\n")) ;
      IoReleaseCancelSpinLock (kCancelSpin) ;
   }
   else {

      // Things to do in the queue. First lets get the first member out of the
      // queue to work with.

      KdPrint (("ParStat: Irps in queue. Getting first Irp.\n")) ;
      head = RemoveHeadList (&Extension->WorkQueue) ;

      // Now set the current irp equal to the wholly formed irp. We do this
      // by calling the macro CONTAINING_RECORD which takes a sub-structure
      // pointer (in this case Tail.Overlay.ListEntry) and reforms it into
      // the Irp.

      KdPrint (("ParStat: Converting tail pointer to full Irp pointer.\n")) ;
      Extension->CurrentIrp = CONTAINING_RECORD(head, IRP, Tail.Overlay.ListEntry) ;

      // We have the IRP. Reset the cancel routine to NULL.

      KdPrint (("ParStat: Resetting cancel routine for current irp.\n")) ;
      IoSetCancelRoutine (Extension->CurrentIrp, NULL) ;

      // No need for further use of the cancel spin lock. Time to release
      // the spin lock.

      KdPrint (("ParStat: Releasing cancel spin lock.\n")) ;
      IoReleaseCancelSpinLock (kCancelSpin) ;

      // More recursion. Now that we have our IRP, go allocate the port.

      KdPrint (("ParStat: Calling ParStatAllocPort\n")) ;
      ParStatAllocPort (Extension) ;
   }

   // Done.
   KdPrint (("ParStat: Leaving ParStatAllocPortWithNext.\n")) ;
}

// ===========================================================================
// ParStatCleanup () - This function will clean up any Irps hanging around in
//                     the work queue.
//
// Parameters: DeviceObject - Current device object.
//             Irp - Current IRP.
// ===========================================================================

NTSTATUS ParStatCleanup (IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP           Irp)
{


   PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension ;
   KIRQL kCancelSpin ;
   PLIST_ENTRY head ;
   PIRP irp ;

   KdPrint (("ParStat: Entering ParStatCleanup.\n")) ;

   // First, since we are going to be modifying the cancel routines for the
   // irps in our device queue, we need to the get the spin lock.

   KdPrint (("ParStat: Getting cancel spin lock.\n")) ;
   IoAcquireCancelSpinLock (&kCancelSpin) ;

   // Now loop until the queue is empty

   KdPrint (("ParStat: Entering loop to empty out queue.\n")) ;
   while (!IsListEmpty(&extension->WorkQueue)) {

      // Get the first irp in line.

      KdPrint (("ParStat: Getting head irp.\n")) ;
      head = RemoveHeadList (&extension->WorkQueue) ;

      // Parse the structure back into the parent IRP.

      KdPrint (("ParStat: Reconstructing IRP.\n")) ;
      irp = CONTAINING_RECORD (head, IRP, Tail.Overlay.ListEntry) ;

      // Set the flag in the IRP to indicate to anyone else that looks at it
      // that we are cancellation is in progress.

      KdPrint (("ParStat: Setting IRP->Cancel field.\n")) ;
      irp->Cancel = TRUE ;

      // Set the cancel IRQ level and NULL out the cancel routine for the
      // IRP.

      KdPrint (("ParStat: Setting irql level and NULL cancel routine.\n")) ;
      irp->CancelIrql = kCancelSpin ;
      irp->CancelRoutine = NULL ;

      // Call the ParStatCancel routine to actually cancel the IRP.

      KdPrint (("ParStat: Calling ParStatCancel to cancel Irp.\n")) ;
      ParStatCancel (DeviceObject, irp) ;

      // Re-acquire the cancel spin lock as ParStatCancel will release it.

      KdPrint (("ParStat: Reacquiring cancel spin lock.\n")) ;
      IoAcquireCancelSpinLock (&kCancelSpin) ;

   }

   // Since we are done and the queue is empty, we can release the cancel
   // spin lock.

   KdPrint (("ParStat: Done with queue cancellation. Release spin lock.\n")) ;
   IoReleaseCancelSpinLock (kCancelSpin) ;

   // Set the irp information which caused us to enter this entry point.

   KdPrint (("ParStat: Setting information for IRP_MJ_CLEANUP irp.\n")) ;
   Irp->IoStatus.Status = STATUS_SUCCESS ;
   Irp->IoStatus.Information = 0 ;

   // Complete the IRP.

   KdPrint (("ParStat: Completing IRP_MJ_CLEANUP request.\n")) ;
   IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

   // Done, return STATUS_SUCCESS.

   KdPrint (("ParStat: Returning STATUS_SUCCESS!\n")) ;
   return(STATUS_SUCCESS) ;
}

// ===========================================================================
// ParStatUnload () - This routine loops through the device list and cleans
//                    up after each of the devices we created.
//
// Parameters: DriverObject - Current driver object.
// ===========================================================================

VOID ParStatUnload (IN PDRIVER_OBJECT DriverObject)
{


   PDEVICE_OBJECT currentDevice ;
   PDEVICE_EXTENSION extension ;

   KdPrint (("ParStat: Entered ParStatUnload.\n")) ;

   // Loop while there is still a valid device object for the driver object

   KdPrint (("ParStat: Entering loop to begin cleanup of each device.\n")) ;
   while (currentDevice = DriverObject->DeviceObject) {

      // Set the extension equal to the current device object extension

      KdPrint (("ParStat: Getting current device extension for this device.\n")) ;
      extension = currentDevice->DeviceExtension ;

      // Check to see if this device object create a symbolic link.

      KdPrint (("ParStat: Checking for a created symbolic link.\n")) ;
      if (extension->CreatedSymbolicLink) {

         // Yes, so delete the symbolic link first.

         KdPrint (("ParStat: Found a symbolic link. Deleting it.\n")) ;
         IoDeleteSymbolicLink (&extension->SymbolicLinkName) ;

         // Now free the pool for the symbolic link name buffer.

         KdPrint (("ParStat: Freeing pool for symbolic link buffer.\n")) ;
         ExFreePool (extension->SymbolicLinkName.Buffer) ;
      }


      // We are cleared to delete the device now.

      KdPrint (("ParStat: Deleting current device.\n")) ;
      IoDeleteDevice (currentDevice) ;
   }

   // Done and out.

   KdPrint (("ParStat: Leaving ParStatUnload.\n")) ;
}

// ===========================================================================
// ParStatCancel () - Cancel routine for irp's.
//
// Parameters: DeviceObject - Current device object.
//             Irp - Current Irp.
// ===========================================================================

VOID ParStatCancel (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   KdPrint (("ParStat: Entering ParStatCancel.\n")) ;

   // Since we come into the function with the cancel spin lock already held,
   // we can go ahead and remove this irp from the device queue.

   KdPrint (("ParStat: Removing irp totally from queue.\n")) ;
   RemoveEntryList (&Irp->Tail.Overlay.ListEntry) ;

   // Now we can release the cancel spin lock.

   KdPrint (("ParStat: Releasing spin lock on cancel.\n")) ;
   IoReleaseCancelSpinLock (Irp->CancelIrql) ;

   // Set the information in the irp that we have cancelled it.

   KdPrint (("ParStat: Setting information in irp for completion.\n")) ;
   Irp->IoStatus.Information = 0;
   Irp->IoStatus.Status = STATUS_CANCELLED ;

   // We can now complete the request.

   KdPrint (("ParStat: Complete the request.\n")) ;
   IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

   // Done.

   KdPrint (("ParStat: Leaving ParStatCancel routine.\n")) ;

}
