/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    packet.c

Abstract:


Author:


Environment:

    Kernel mode only.

Notes:


Future:



Revision History:

--*/

#include "stdarg.h"
#include "ntddk.h"
#include "ntiologc.h"
#include "ndis.h"

#include "debug.h"
#include "packet.h"



NTSTATUS
PacketRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for create/open and close requests.
    These requests complete successfully.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    POPEN_INSTANCE      Open;
    PIO_STACK_LOCATION  IrpSp;
    PNDIS_PACKET        pPacket;
    PMDL                pMdl;
    NDIS_STATUS         Status;

    IF_LOUD(DbgPrint("Packet: Read\n");)

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Open=IrpSp->FileObject->FsContext;

    //
    //  See if the buffer is atleast big enough to hold the
    //  ethernet header
    //
    if (IrpSp->Parameters.Read.Length < ETHERNET_HEADER_LENGTH) {

        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    //
    //  Allocate an MDL to map the portion of the buffer following the
    //  header
    //
    pMdl=IoAllocateMdl(
              MmGetMdlVirtualAddress(Irp->MdlAddress),
              MmGetMdlByteCount(Irp->MdlAddress),
              FALSE,
              FALSE,
              NULL
              );


    if (pMdl == NULL) {
        IF_LOUD(DbgPrint("Packet: Read-Failed to allocate Mdl\n");)
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    //
    //  Build the mdl to point to the the portion of the buffer followin
    //  the header
    //
    IoBuildPartialMdl(
        Irp->MdlAddress,
        pMdl,
        ((PUCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress))+ETHERNET_HEADER_LENGTH,
        0
        );

    //
    //  Clear the next link in the new MDL
    //
    pMdl->Next=NULL;

    //
    //  Try to get a packet from our list of free ones
    //

    NdisAllocatePacket(
        &Status,
        &pPacket,
        Open->PacketPool
        );

    if (Status != NDIS_STATUS_SUCCESS) {
        IF_LOUD(DbgPrint("Packet: Read- No free packets\n");)

        IoFreeMdl(pMdl);
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    //
    //  Get a pointer to the packet itself
    //

    RESERVED(pPacket)->Irp=Irp;
    RESERVED(pPacket)->pMdl=pMdl;

    IoMarkIrpPending(Irp);
    Irp->IoStatus.Status = STATUS_PENDING;


    //
    //  Attach our new MDL to the packet
    //
    NdisChainBufferAtFront(pPacket,pMdl);

    //
    //  Put this packet in a list of pending reads.
    //  The receive indication handler will attemp to remove packets
    //  from this list for use in transfer data calls
    //
    ExInterlockedInsertTailList(
        &Open->RcvList,
        &RESERVED(pPacket)->ListElement,
        &Open->RcvQSpinLock);




    return(STATUS_PENDING);

}




NDIS_STATUS
PacketReceiveIndicate (
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID       HeaderBuffer,
    IN UINT        HeaderBufferSize,
    IN PVOID       LookAheadBuffer,
    IN UINT        LookaheadBufferSize,
    IN UINT        PacketSize
    )

{
    POPEN_INSTANCE      Open;
    PIO_STACK_LOCATION  IrpSp;
    PIRP                Irp;
    PLIST_ENTRY         PacketListEntry;
    PNDIS_PACKET        pPacket;
    ULONG               SizeToTransfer;
    NDIS_STATUS         Status;
    UINT                BytesTransfered;
    ULONG               BufferLength;

    PPACKET_RESERVED    Reserved;

    IF_LOUD(DbgPrint("Packet: ReceiveIndicate\n");)

    Open= (POPEN_INSTANCE)ProtocolBindingContext;

    if (HeaderBufferSize > ETHERNET_HEADER_LENGTH) {

        return NDIS_STATUS_SUCCESS;
    }

    //
    //  See if there are any pending read that we can satisfy
    //
    PacketListEntry=ExInterlockedRemoveHeadList(
                        &Open->RcvList,
                        &Open->RcvQSpinLock
                        );

    if (PacketListEntry == NULL) {

        return NDIS_STATUS_SUCCESS;
    }


    Reserved=CONTAINING_RECORD(PacketListEntry,PACKET_RESERVED,ListElement);
    pPacket=CONTAINING_RECORD(Reserved,NDIS_PACKET,ProtocolReserved);

    Irp=RESERVED(pPacket)->Irp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    //  This is the length of our partial MDL
    //
    BufferLength=IrpSp->Parameters.Read.Length-ETHERNET_HEADER_LENGTH;

    //
    //  Find out how much to transfer
    //
    SizeToTransfer = (PacketSize < BufferLength) ?
                       PacketSize : BufferLength;

    //
    //  copy the ethernet header into the actual readbuffer
    //
    NdisMoveMappedMemory(
        MmGetSystemAddressForMdl(Irp->MdlAddress),
        HeaderBuffer,
        HeaderBufferSize
        );

    //
    //  Call the Mac to transfer the packet
    //

    NdisTransferData(
        &Status,
        Open->AdapterHandle,
        MacReceiveContext,
        0,
        SizeToTransfer,
        pPacket,
        &BytesTransfered);

    if (Status != NDIS_STATUS_PENDING) {

        //
        //  If it didn't pend, call the completeion routine now
        //
        PacketTransferDataComplete(
            Open,
            pPacket,
            Status,
            BytesTransfered
            );


    }



    return NDIS_STATUS_SUCCESS;

}


VOID
PacketTransferDataComplete (
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status,
    IN UINT          BytesTransfered
    )

{
    PIO_STACK_LOCATION   IrpSp;
    POPEN_INSTANCE       Open;
    PIRP                 Irp;
    PMDL                 pMdl;

    IF_LOUD(DbgPrint("Packet: TransferDataComplete\n");)

    Open= (POPEN_INSTANCE)ProtocolBindingContext;

    Irp=RESERVED(pPacket)->Irp;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    pMdl=RESERVED(pPacket)->pMdl;

    //
    //  Free the MDL that we allocated
    //
    IoFreeMdl(pMdl);

    //
    //  recylcle the packet
    //
    NdisReinitializePacket(pPacket);

    //
    //  Put the packet on the free queue
    //
    NdisFreePacket(pPacket);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesTransfered+ETHERNET_HEADER_LENGTH;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);


    return;


}









VOID
PacketReceiveComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    )

{

    return;

}
