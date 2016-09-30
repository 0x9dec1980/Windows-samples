//////////////////////////////////////////////////////////////////////////////
// Program Name: Async.c
// Programmer: Mark A. Overby (MarkOv)
// Program Description: This is a simple driver which does nothing except show
//                      how to use an async private irp to signal to a user
//                      mode application that a specific hardware event that
//                      it has been waiting for has triggered.
//
// Copyright (c) 1994 Microsoft Corporation
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// INCLUDES & DEFINES
//////////////////////////////////////////////////////////////////////////////

#include "ntddk.h"
#include "internal.h"
#include "control.h"

//////////////////////////////////////////////////////////////////////////////
// GLOBALS
//////////////////////////////////////////////////////////////////////////////

ULONG DebugValue = 0 ;

//////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// DriverEntry () - This function is called at driver init time as the entry
//                  point for when the driver is loaded into the system.
//
// Arguments:     DriverObject - Supplies the DriverObject for this driver
//                RegistryPath - Supplies the registry path for this driver
// ===========================================================================

NTSTATUS DriverEntry (IN PDRIVER_OBJECT DriverObject,
                      IN PUNICODE_STRING RegistryPath)
{
   UNICODE_STRING                ParsedRegPath ;
   RTL_QUERY_REGISTRY_TABLE      paramTable[3] ;
   ULONG                         DebugLevel = 0 ;
   ULONG                         BreakNow = 0 ;
   ULONG                         DefaultValue = 0 ;

   UNICODE_STRING                ParamString ;
   NTSTATUS                      Status ;
   UNICODE_STRING                DeviceName ;
   UNICODE_STRING                DosDevName ;
   PDEVICE_OBJECT                pDeviceObject ;
   PDEVICE_EXTENSION             pExtension ;

   KdPrint (("Async: Entering DriverEntry.\n")) ;

   // First thing we will do is prepare a unicode string so that we can
   // pass in the path of our parameters sub-key to read in values to
   // determine if we should break on entry and what our default debug
   // level is.

   RtlInitUnicodeString (&ParamString, L"\\Parameters") ;
   ParsedRegPath.MaximumLength = RegistryPath->Length + ParamString.Length +
                                  sizeof (WCHAR) ;
   ParsedRegPath.Length = 0 ;

   // Now that we have the lengths set correctly, allocate a buffer for
   // the string from pageable pool.

   ParsedRegPath.Buffer = ExAllocatePool (PagedPool,
                                          ParsedRegPath.MaximumLength) ;

   // Check to make sure we could get the memory.

   if (!ParsedRegPath.Buffer) {

      // We failed, return with an appropriate status code.

      KdPrint (("Async: FAILED! Returning INSUFFICENT_RESOURCES.\n")) ;
      return(STATUS_INSUFFICIENT_RESOURCES) ;
   }

   // That worked. Zero out the memory and then parse the strings together.

   RtlZeroMemory (ParsedRegPath.Buffer, ParsedRegPath.MaximumLength) ;
   RtlAppendUnicodeStringToString (&ParsedRegPath, RegistryPath) ;
   RtlAppendUnicodeStringToString (&ParsedRegPath, &ParamString) ;

   // We have our path. Now zero out the memory for the paramTable for
   // RtlQueryRegistryValues.

   RtlZeroMemory (&paramTable[0], sizeof(paramTable)) ;

   // Now set up the paramTable entries to query our registry values.

   paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT ;
   paramTable[0].Name  = L"BreakOnEntry" ;
   paramTable[0].EntryContext = &BreakNow ;
   paramTable[0].DefaultType = REG_DWORD ;
   paramTable[0].DefaultData = &DefaultValue ;
   paramTable[0].DefaultLength = sizeof(ULONG) ;

   paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT ;
   paramTable[1].Name = L"DebugLevel" ;
   paramTable[1].EntryContext = &DebugLevel ;
   paramTable[1].DefaultType = REG_DWORD ;
   paramTable[1].DefaultData = &DefaultValue ;
   paramTable[1].DefaultLength = sizeof(ULONG) ;

   // Now let us try and query the registry for our information.

   Status = RtlQueryRegistryValues (RTL_REGISTRY_ABSOLUTE |
                                    RTL_REGISTRY_OPTIONAL,
                                    ParsedRegPath.Buffer, &paramTable[0],
                                    NULL, NULL) ;

   // Check to see if we were successful.

   if (!NT_SUCCESS(Status)) {

      // Something bad happened. We will return a failure. We musn't
      // forget to free the pool we allocated for the Registry buffer.

      KdPrint (("Async: FAILURE! RtlQueryRegistryValues.\n")) ;
      ExFreePool (ParsedRegPath.Buffer) ;
      return(STATUS_DRIVER_INTERNAL_ERROR) ;
   }

   // We no longer need the buffer for the Parsed registry path. We can free
   // that pool memory.

   ExFreePool (ParsedRegPath.Buffer) ;

   // That worked so now let's act on the information we got. Breakpoint if
   // we are !0 for BreakNow. Now that we are configured. We can use the
   // macro for determining if we should print a message or not.

   DebugValue = DebugLevel ;
   if (BreakNow) {
      DbgBreakPoint () ;
   }

   // Let us continue forward. Next step is to set up the dispatch points for
   // our driver. We (for this sample) are only going to support:
   // IRP_MJ_CREATE - For opening the device.
   // IRP_MJ_CLOSE - For closing the device.
   // IRP_MJ_DEVICE_CONTROL - For our wait, release, and test messages.
   // IRP_MJ_CLEANUP - To clean up requests when a handle is closed.
   // We will have an unload entry point.

   AsyncMessage (INFORM,("Async: Setting up dispatch entry points.\n")) ;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = AsyncCreateClose ;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = AsyncCreateClose ;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AsyncDispatchRoutine ;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = AsyncCleanup ;
   DriverObject->DriverUnload = AsyncUnload ;

   // Let's set up the unicode strings for the symbolic link and the
   // device object name.

   AsyncMessage (INFORM,("Async: Setting up unicode names for objects.\n")) ;
   RtlInitUnicodeString (&DeviceName, L"\\Device\\AsyncTest") ;
   RtlInitUnicodeString (&DosDevName, L"\\DosDevices\\IrpTest") ;

   // Now create the device object.

   AsyncMessage (INFORM,("Async: Creating device object.\n")) ;
   Status = IoCreateDevice (DriverObject, sizeof(DEVICE_EXTENSION),
                            &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE,
                            &pDeviceObject) ;

   // Check for failure.

   AsyncMessage (INFORM,("Async: Checking for failure on device object.\n"));
   if (!NT_SUCCESS(Status)) {

      // We failed. Return the last status we got. Free the buffer that we
      // allocated for the parsed registry path.

      AsyncMessage (ERROR,("Async: Failed! Exiting.\n")) ;
      ExFreePool (ParsedRegPath.Buffer) ;
      return(Status) ;
   }

   // Since we succeeded, set up the device extension with the correct
   // values.

   AsyncMessage (INFORM,("Async: Setting up device extension.\n")) ;
   pExtension = pDeviceObject->DeviceExtension ;
   pExtension->DeviceObject = pDeviceObject ;

   // Now we will create the symbolic link for our device object.

   AsyncMessage (INFORM,("Async: Creating DosDevice.\n")) ;
   Status = IoCreateSymbolicLink (&DosDevName, &DeviceName) ;

   // Check for failure.

   AsyncMessage (INFORM,("Async: Checking for failure of symbolic link.\n")) ;
   if (!NT_SUCCESS(Status)) {

      // We failed. Return the last status we got. Delete the device object,
      // and free the buffer for the parsed registry path.

      AsyncMessage (ERROR,("Async: Failed! Exiting.\n")) ;
      IoDeleteDevice (pDeviceObject) ;
      ExFreePool (ParsedRegPath.Buffer) ;
      return(Status) ;
   }

   // That worked. So we have created all of the objects and links that we
   // need. The next priority is to set up the correct flags in the device
   // object and to initialize the queue in the device extension.

   AsyncMessage (INFORM,("Async: Setting buffered flag in DevObj.\n")) ;
   pDeviceObject->Flags |= DO_BUFFERED_IO ;
   AsyncMessage (INFORM,("Async: Init device queue.\n")) ;
   InitializeListHead (&pExtension->WaitQueue) ;

   // We must also set up the spin lock to protect the queue from other
   // access while we are using it.

   AsyncMessage (INFORM,("Async: Setting up spin lock for wait queue.\n")) ;
   KeInitializeSpinLock (&pExtension->QueueSpin) ;

   // We have completed everything that we need to do. We can return success
   // after freeing the pool memory that we allocated.

   AsyncMessage (INFORM,("Async: Success! Returning.\n")) ;
   return(STATUS_SUCCESS) ;
}

// ===========================================================================
// AsyncCreateClose () - This function handles both the IRP_MJ_CREATE and
//          the IRP_MJ_CLOSE requests that the executive will issue to us to
//          create and close file objects to our device. There is nothing for
//          us to do in either circumstance, so we will just return success
//          for both operations.
//
// Arguments: DeviceObject - Device object for this operation.
//            Irp - The Irp for this operation.
// ===========================================================================

NTSTATUS AsyncCreateClose (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   AsyncMessage (INFORM,("Async: Entering AsyncCreateClose.\n")) ;

   // Nothing to do except set fields in the Irp and return SUCCESS.

   AsyncMessage (INFORM,("Async: Setting Irp status and inform fields.\n")) ;
   Irp->IoStatus.Status = STATUS_SUCCESS ;
   Irp->IoStatus.Information = 0 ;

   // Complete the request.

   AsyncMessage (INFORM,("Async: Completing request.\n")) ;
   IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

   // Return.

   AsyncMessage (INFORM,("Async: Returning SUCCESS.\n")) ;
   return(STATUS_SUCCESS) ;
}

// ===========================================================================
// AsyncDispatchRoutine () - This function handles the DeviceIoControl IRP's
//             that we will be receiving from the executive to control our
//             device. We will be handing 3 different custom private
//             IOCTL's.
//             IOCTL_HOLD_REQUEST - To put a request in the queue for later
//                                  release.
//             IOCTL_RELEASE_REQUEST - To release a request in the queue.
//             IOCTL_DO_NOTHING - Just to show that we are async.
//
// Arguments: DeviceObject - Device object for this operation.
//            Irp - The Irp for this operation.
// ===========================================================================

NTSTATUS AsyncDispatchRoutine (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   PIO_STACK_LOCATION               pIrpStack ;
   KIRQL                            kCancelSpin ;
   PDEVICE_EXTENSION                extension = DeviceObject->DeviceExtension;
   PIRP                             NewIrp ;
   PLIST_ENTRY                      head ;
   PUCHAR                           pData ;

   AsyncMessage (INFORM,("Async: Entering AsyncDispatchRoutine.\n")) ;

   // First thing on the agenda is to get the current stack location for
   // this IRP.

   AsyncMessage (INFORM,("Async: Getting current Irp stack location.\n")) ;
   pIrpStack = IoGetCurrentIrpStackLocation (Irp) ;

   // Now that we have the current Irp location, we will enter our big
   // switch statement to switch on the IOCTL command.

   AsyncMessage (INFORM,("Async: Switching on control code.\n")) ;
   switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {

   case IOCTL_HOLD_REQUEST:

      // This is a request to put a message into our queue for release at
      // a later time.

      AsyncMessage (INFORM,("Async: *IOCTL_HOLD_REQUEST found.\n")) ;

      // We are going to set a cancel routine for this Irp so that if
      // the user app aborts we can remove this Irp from the queue.

      AsyncMessage (INFORM,("Async: *Getting cancel spin lock.\n")) ;
      IoAcquireCancelSpinLock (&kCancelSpin) ;

      // Now that it is safe, set the cancel routine in the Irp.

      AsyncMessage (INFORM,("Async: *Setting cancel routine in Irp.\n")) ;
      IoSetCancelRoutine (Irp, AsyncCancel) ;

      // We no longer need the cancel spin lock, so we will release it.

      AsyncMessage (INFORM,("Async: *Releasing cancel spin lock.\n")) ;
      IoReleaseCancelSpinLock (kCancelSpin) ;

      // Now insert in the Irp into our queue.

      AsyncMessage (INFORM,("Async: *Inserting Irp into queue.\n"))  ;
      ExInterlockedInsertTailList (&extension->WaitQueue,
                                   &Irp->Tail.Overlay.ListEntry,
                                   &extension->QueueSpin) ;

      // Mark the Irp as pending.

      AsyncMessage (INFORM,("Async: *Marking Irp as pending.\n")) ;
      IoMarkIrpPending (Irp) ;

      // Return STATUS_PENDING.

      AsyncMessage (INFORM,("Async: *Returning STATUS_PENDING.\n")) ;
      return(STATUS_PENDING) ;
      break;

   case IOCTL_RELEASE_REQUEST:

      // This is a request to complete an Irp that was put into our
      // wait queue for later processing by an event. This is our faked
      // event. In reality, this would probably come from and Isr's DPC
      // or some other similar event.

      AsyncMessage (INFORM,("Async: =IOCTL_RELEASE_REQUEST found.\n")) ;

      // We are going to check first and see if the list is empty or not.

      AsyncMessage (INFORM,("Async: =Checking to see if queue is empty.\n"));
      if (IsListEmpty (&extension->WaitQueue)) {

         // Nothing for us to do ... Although VERY odd that we tried to
         // release a message that did not exist. We will set the
         // information in the Irp and complete it.

         AsyncMessage (ERROR,("Async: =Nothing in queue to release.\n")) ;
         AsyncMessage (INFORM,("Async: =Setting information in Irp.\n")) ;
         Irp->IoStatus.Status = STATUS_SUCCESS ;
         Irp->IoStatus.Information = 0 ;
         AsyncMessage (INFORM,("Async: =Completing Irp.\n")) ;
         IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

         // Return STATUS_SUCCESS.

         AsyncMessage (INFORM,("Async: =Returning SUCCESS.\n")) ;
         return(STATUS_SUCCESS) ;
      }

      // Ok, we have an Irp in the queue that we can complete. First we will
      // acquire the spin lock so that we can reset the cancel routine.

      AsyncMessage (INFORM,("Async: =Acquiring cancel spin lock.\n")) ;
      IoAcquireCancelSpinLock (&kCancelSpin) ;

      // Now it is safe to get the irp from the queue and rebuild it.

      AsyncMessage (INFORM,("Async: =Getting irp from queue.\n")) ;
      head = ExInterlockedRemoveHeadList (&extension->WaitQueue,
                                          &extension->QueueSpin) ;
      NewIrp = CONTAINING_RECORD (head, IRP, Tail.Overlay.ListEntry) ;

      // Now that we have the IRP, reset the cancel routine.

      AsyncMessage (INFORM,("Async: =Resetting cancel routine.\n")) ;
      IoSetCancelRoutine (NewIrp, NULL) ;

      // We are done with cancel operations, so release the cancel spinl.

      AsyncMessage (INFORM,("Async: =Releasing cancel spin lock.\n")) ;
      IoReleaseCancelSpinLock (kCancelSpin) ;

      // Set the local data pointer for the system buffer of held IOCTL.
      // Then put data in that buffer.

      AsyncMessage (INFORM,("Async: =Getting local ptr for sys buffer.\n")) ;
      pData = (PUCHAR) NewIrp->AssociatedIrp.SystemBuffer ;
      AsyncMessage (INFORM,("Async: =Setting data to be returned.\n")) ;
      *pData = 101 ;

      // Ok, now complete the request that was waiting. Set the information
      // in the IRP and then send it on its way.

      AsyncMessage (INFORM,("Async: =Completing Irp that was in queue.\n")) ;
      NewIrp->IoStatus.Information = 1 ;
      NewIrp->IoStatus.Status = STATUS_SUCCESS ;
      IoCompleteRequest (NewIrp, IO_NO_INCREMENT) ;

      // We have completed our business so complete the irp that released
      // the irp from the queue.

      AsyncMessage (INFORM,("Async: =Completing release request.\n")) ;
      Irp->IoStatus.Information = 0 ;
      Irp->IoStatus.Status = STATUS_SUCCESS ;
      IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

      // Return STATUS_SUCCESS.

      AsyncMessage (INFORM,("Async: =Returning STATUS_SUCCESS.\n")) ;
      return(STATUS_SUCCESS) ;
      break;

   case IOCTL_DO_NOTHING:

      // This is our do nothing IOCTL. All it does is prove that other requests
      // can be active at the same time as the waiting request.

      AsyncMessage (INFORM,("Async: /IOCTL_DO_NOTHING found.\n")) ;

      // Just set up the fields in the Irp and return it right away.

      AsyncMessage (INFORM,("Async: /Setting fields in Irp & complete.\n")) ;
      Irp->IoStatus.Information = 0 ;
      Irp->IoStatus.Status = STATUS_SUCCESS ;
      IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

      // Return STATUS_SUCCESS.

      AsyncMessage (INFORM,("Async: /Returning success.\n")) ;
      return(STATUS_SUCCESS) ;
      break;

   default:

      // We got an IOCTL we don't handle.

      AsyncMessage (ERROR,("Async: !!Unknown IOCTL received.\n")) ;

      // Set up the fields in the Irp and return.

      AsyncMessage (INFORM,("Async: !!Setting fields in bogus Irp.\n")) ;
      Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED ;
      Irp->IoStatus.Information = 0 ;

      // Return the bad request.

      AsyncMessage (INFORM,("Async: !!Returning bad request and exiting.\n")) ;
      IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
      return(STATUS_NOT_IMPLEMENTED) ;
   }
}

// ===========================================================================
// AsyncCleanup () - This routine will match the file object of the cleanup
//                   IRP with any IRP's that are outstanding in our wait
//                   queue. If it finds some, then it will cancel those
//                   IRP's.
//
// Arguments: DeviceObject - DeviceObject for this operation.
//            Irp - Irp for this operation.
// ===========================================================================

NTSTATUS AsyncCleanup (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension ;
   KIRQL kCancelSpin, kOld ;
   PLIST_ENTRY head ;
   PIRP rcirp ;
   LIST_ENTRY StandbyList ;
   PIO_STACK_LOCATION ClnUpStack ;
   PIO_STACK_LOCATION IrpStack ;

   AsyncMessage (INFORM,("Async: Entered AsyncCleanup.\n")) ;

   // Get the stack location of the cleanup IRP.

   AsyncMessage (INFORM,("Async: Getting cleanup IRP stack.\n")) ;
   ClnUpStack = IoGetCurrentIrpStackLocation (Irp) ;

   // Initialize the stand by queue.

   AsyncMessage (INFORM,("Async: Setting up standby queue.\n")) ;
   InitializeListHead (&StandbyList) ;

   // First order of business is to acquire the cancel spin lock.

   AsyncMessage (INFORM,("Async: Getting cancel spin lock.\n")) ;
   IoAcquireCancelSpinLock (&kCancelSpin) ;

   // Enter a loop to empty out the current queue. We will check the file
   // objects to see if they match. If they do, then we will cancel the
   // irp. If not, then we will put the irp in the standby queue and when
   // done with the device extension queue, we will requeue the standby
   // queue back into the main queue.

   AsyncMessage (INFORM,("Async: Entering WaitQueue loop.\n")) ;
   while (!IsListEmpty (&extension->WaitQueue)) {

      // Get the head of list irp.

      AsyncMessage (INFORM,("Async: Getting head irp.\n")) ;
      head = ExInterlockedRemoveHeadList (&extension->WaitQueue,
                                          &extension->QueueSpin) ;

      // Reconstruct the Irp.

      AsyncMessage (INFORM,("Async: Reconstructing IRP.\n")) ;
      rcirp = CONTAINING_RECORD (head, IRP, Tail.Overlay.ListEntry) ;

      // Check to see if this is an IRP we need to cancel.

      AsyncMessage (INFORM,("Async: Checking for match on file object.\n"));
      IrpStack = IoGetCurrentIrpStackLocation (rcirp) ;
      if (IrpStack->FileObject == ClnUpStack->FileObject) {

         // This is one we need to clean up.

         AsyncMessage (INFORM,("Async: Found an Irp. Setting cancel.\n")) ;
         rcirp->Cancel = TRUE ;
         rcirp->CancelIrql = kCancelSpin ;
         rcirp->CancelRoutine = NULL ;

         // Cancel the IRP.

         AsyncMessage (INFORM,("Async: Cancelling IRP.\n")) ;
         AsyncCancelSup (rcirp) ;
      }
      else {

         // Not one of ours. Put it in the standby queue.

         AsyncMessage (INFORM,("Async: No match. Standby queue.\n")) ;
         InsertTailList (&StandbyList,
                         &rcirp->Tail.Overlay.ListEntry ) ;

         // Continue on our merry way.
      }
   }

   // Ok, the WaitQueue is empty. Now re-queue anything we did not cancel.

   AsyncMessage (INFORM,("Async: Requeuing irps not cancelled.\n")) ;
   while (!IsListEmpty (&StandbyList)) {

      // Take the head irp, reconstruct it, and put it back in the wait
      // queue.

      AsyncMessage (INFORM,("Async: Irp transiting to WaitQueue.\n")) ;
      head = RemoveHeadList (&StandbyList) ;
      rcirp = CONTAINING_RECORD (head, IRP, Tail.Overlay.ListEntry) ;
      ExInterlockedInsertTailList (&extension->WaitQueue,
                                   &rcirp->Tail.Overlay.ListEntry,
                                   &extension->QueueSpin) ;

   }

   // We are done with any cancel operations, release that spin lock.

   AsyncMessage (INFORM,("Async: Releasing cancel spin lock.\n")) ;
   IoReleaseCancelSpinLock (kCancelSpin) ;

   // Set the information for completion of the IRP_MJ_CLEANUP.

   AsyncMessage (INFORM,("Async: Setting information in cleanup.\n")) ;
   Irp->IoStatus.Status = STATUS_SUCCESS ;
   Irp->IoStatus.Information = 0 ;

   // Complete the request.

   AsyncMessage (INFORM,("Async: Completing request.\n")) ;
   IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

   // Return success.

   AsyncMessage (INFORM,("Async: Returning STATUS_SUCCESS.\n")) ;
   return(STATUS_SUCCESS) ;
}

// ===========================================================================
// AsyncUnload () - This function completes any required functionality for us
//                  to unload our driver. For us, we just need to delete the
//                  symbolic link and the device and we can exit.
//
// Arguments: DriverObject - Driver object for this operation.
// ===========================================================================

VOID AsyncUnload (IN PDRIVER_OBJECT DriverObject)
{
   PDEVICE_OBJECT currentDevice = DriverObject->DeviceObject ;
   UNICODE_STRING symbolicLink ;

   AsyncMessage (INFORM,("Async: Entering unload.\n")) ;

   // Delete the symbolic link first.

   AsyncMessage (INFORM,("Async: Deleting symbolic link.\n")) ;
   RtlInitUnicodeString (&symbolicLink, L"\\DosDevices\\IrpTest") ;
   IoDeleteSymbolicLink (&symbolicLink) ;

   // Now delete the device.

   AsyncMessage (INFORM,("Async: Deleting device object.\n")) ;
   IoDeleteDevice (currentDevice) ;

   // Done!

   KdPrint (("Async: Driver unloaded.\n")) ;
}

// ===========================================================================
// AsyncCancelSup () - This function will cancel a specific IRP. This is the
//                     support routine for all cancel operations which
//                     actually performs the cancel.
//
// Parameters: Irp - Irp to be cancelled.
// ===========================================================================

VOID AsyncCancelSup (IN PIRP Irp)
{
   AsyncMessage (INFORM,("Async: Entered AsyncCancelSup.\n")) ;

   // Set the information in the IRP.

   AsyncMessage (INFORM,("Async: Setting information in IRP.\n")) ;
   Irp->IoStatus.Status = STATUS_CANCELLED ;
   Irp->IoStatus.Information = 0 ;

   // Complete the request.

   AsyncMessage (INFORM,("Async: Completing IRP.\n")) ;
   IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

   // Done.
}

// ===========================================================================
// AsyncCancel () - This function removes the irp from our queue and then
//                  calls to the support routines to cancel the irp.
//
// Parameters: DeviceObject - Device object for this operation.
//             Irp - Irp to be cancelled.
// ===========================================================================

VOID AsyncCancel (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   KIRQL kOld ;
   PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension ;

   AsyncMessage (INFORM,("Async: Entered AsyncCancel.\n")) ;

   // Acquire the spin lock for the wait queue.

   AsyncMessage (INFORM,("Async: Getting spin lock for wait queue.\n")) ;
   KeAcquireSpinLock (&extension->QueueSpin, &kOld) ;

   // Now remove the irp from the queue completely.

   AsyncMessage (INFORM,("Async: Removing Irp from queue.\n")) ;
   RemoveEntryList (&Irp->Tail.Overlay.ListEntry) ;

   // Release the spin lock.

   AsyncMessage (INFORM,("Async: Releasing spin lock.\n")) ;
   KeReleaseSpinLock (&extension->QueueSpin, kOld) ;

   // Release the cancel spin lock.

   AsyncMessage (INFORM,("Async: Releasing cancel spin lock.\n")) ;
   IoReleaseCancelSpinLock (Irp->CancelIrql) ;

   // Call out to our support routine.

   AsyncMessage (INFORM,("Async: Calling support routine.\n")) ;
   AsyncCancelSup (Irp) ;

   // Done.

   AsyncMessage (INFORM,("Async: Done.\n")) ;
}
