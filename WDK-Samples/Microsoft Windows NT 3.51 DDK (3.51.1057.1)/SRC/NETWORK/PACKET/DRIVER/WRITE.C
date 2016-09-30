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
PacketWrite(
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
    PIO_STACK_LOCATION IrpSp;
    PNDIS_PACKET       pPacket;

    NDIS_STATUS     Status;

    IF_LOUD(DbgPrint("Packet: SendAdapter\n");)

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Open=IrpSp->FileObject->FsContext;
    //
    //  Try to get a packet from our list of free ones
    //

    NdisAllocatePacket(
        &Status,
        &pPacket,
        Open->PacketPool
        );

    if (Status != NDIS_STATUS_SUCCESS) {

        //
        //  No free packets
        //
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    RESERVED(pPacket)->Irp=Irp;

    //
    //  Attach the writes buffer to the packet
    //
    NdisChainBufferAtFront(pPacket,Irp->MdlAddress);

    IoMarkIrpPending(Irp);
    Irp->IoStatus.Status = STATUS_PENDING;

    //
    //  Call the MAC
    //
    NdisSend(
        &Status,
        Open->AdapterHandle,
        pPacket);


    if (Status != NDIS_STATUS_PENDING) {
        //
        //  The send didn't pend so call the completion handler now
        //
        PacketSendComplete(
            Open,
            pPacket,
            Status
            );


    }



    return(STATUS_PENDING);

}



VOID
PacketSendComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status
    )

{
    PIRP              Irp;

    IF_LOUD(DbgPrint("Packet: SendComplete\n");)

    Irp=RESERVED(pPacket)->Irp;

    //
    //  recyle the packet
    //
    NdisReinitializePacket(pPacket);

    //
    //  Put the packet back on the free list
    //
    NdisFreePacket(pPacket);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return;

}
