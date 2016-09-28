//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

/**************************************************************************
    File: hwsim.cpp
    Abstract:
        This file contains the hardware simulation.  It fakes "DMA" transfers,
        scatter gather mapping handling, ISR's, etc...  The ISR routine in
        here will be called when an ISR would be generated by the fake hardware
        and it will directly call into the device level ISR for more accurate
        simulation.
**************************************************************************/
#include "BDATuner.h"
#include <intsafe.h>

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CHardwareSimulation::
CHardwareSimulation (
    IN IHardwareSink *HardwareSink
    ) :
    m_HardwareSink (HardwareSink),
    m_ScatterGatherMappingsMax (SCATTER_GATHER_MAPPINGS_MAX)

/*++

Routine Description:

    Construct a hardware simulation

Arguments:

    HardwareSink -
        The hardware sink interface.  This is used to trigger
        fake interrupt service routines from.

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    //
    // Initialize the DPC's, timer's, and locks necessary to simulate
    // this capture hardware.
    //
    KeInitializeDpc (
        &m_IsrFakeDpc, 
        reinterpret_cast <PKDEFERRED_ROUTINE> 
            (CHardwareSimulation::SimulatedInterrupt),
        this
        );

    KeInitializeEvent (
        &m_HardwareEvent,
        SynchronizationEvent,
        FALSE
        );

    KeInitializeTimer (&m_IsrTimer);

    KeInitializeSpinLock (&m_ListLock);
    m_ListLockAddr=&m_ListLock;

}

/*************************************************/


NTSTATUS
CHardwareSimulation::
Start (
    IN CTsSynthesizer *TsSynth,
    IN LONGLONG TimePerFrame,
    IN ULONG PacketSize,
    IN ULONG PacketsPerSample
    )

/*++

Routine Description:

    Start the hardware simulation.  This will kick the interrupts on,
    begin issuing DPC's, filling in capture information, etc...
    We keep track of starvation starting at this point.

Arguments:

    TsSynth -
        The transport stream synthesizer to use to generate TS packets
        on the capture buffer.

    TimePerFrame -
        The time per frame...  we issue interrupts this often.

    PacketSize -
        The size of a single transport stream packet.

    PacketsPerSample -
        The number of packets in a single capture sample

Return Value:

    Success / Failure (typical failure will be out of memory on the 
    scratch buffer, etc...)

--*/

{
    DbgPrint("CHardwareSimulation::Start ");
    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    m_TsSynth = TsSynth;
    m_TimePerFrame = TimePerFrame;
    m_SampleSize = PacketSize * PacketsPerSample;
    m_PacketSize = PacketSize;
    m_PacketsPerSample = PacketsPerSample;
    m_BufferRemaining = 0;

    InitializeListHead (&m_ScatterGatherMappings);
    m_NumMappingsCompleted = 0;
    m_ScatterGatherMappingsQueued = 0;
    m_NumFramesSkipped = 0;
    m_InterruptTime = 0;
    m_wPcrPid = WORD_MAX;
    m_llLastPcr = LONGLONG_MAX;
    m_llPcrOffset = LONGLONG_MAX;
    m_lCurrTSID = -1;

    

    KeQuerySystemTime (&m_StartTime);
    m_LastTimerTime.QuadPart = m_StartTime.QuadPart;

    //
    // Allocate a scratch buffer for the synthesizer.
    //
    m_SynthesisBuffer = reinterpret_cast <PUCHAR> (
        ExAllocatePoolWithTag (
            NonPagedPool,
            m_SampleSize,
			MS_SAMPLE_ANALOG_POOL_TAG
            )
        );

    if (!m_SynthesisBuffer) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If everything is ok, start issuing interrupts.
    //
    if (NT_SUCCESS (Status)) {

        //
        // Initialize the entry lookaside.
        //
        ExInitializeNPagedLookasideList (
            &m_ScatterGatherLookaside,
            NULL,
            NULL,
            0,
            sizeof (SCATTER_GATHER_ENTRY),
            'nEGS',
            0
            );

        //
        // Set up the synthesizer with the width, height, and scratch buffer.
        //
        m_TsSynth -> SetSampleSize (m_PacketSize, m_PacketsPerSample);
        m_TsSynth -> SetBuffer (m_SynthesisBuffer);

        LARGE_INTEGER NextDeltaTime;
        NextDeltaTime.QuadPart = (-1)*m_TimePerFrame;
        m_LastTimerTime.QuadPart -= NextDeltaTime.QuadPart;//compensate the absolute time calculation

        m_HardwareState = HardwareRunning;
        KeSetTimer (&m_IsrTimer, NextDeltaTime, &m_IsrFakeDpc);

    }

    return Status;
        
}

/*************************************************/


NTSTATUS
CHardwareSimulation::
Pause (
    BOOLEAN Pausing
    )

/*++

Routine Description:

    Pause the hardware simulation...  When the hardware simulation is told
    to pause, it stops issuing interrupts, etc...  but it does not reset
    the counters 

Arguments:

    Pausing -
        Indicates whether the hardware is pausing or not. 

        TRUE -
            Pause the hardware

        FALSE -
            Unpause the hardware from a previous pause


Return Value:

    Success / Failure

--*/

{
    DbgPrint("CHardwareSimulation::Pause ");

    PAGED_CODE();

    if (Pausing && m_HardwareState == HardwareRunning) {
        //
        // If we were running, stop completing mappings, etc...
        //
        m_StopHardware = TRUE;
    
        KeWaitForSingleObject (
            &m_HardwareEvent,
            Suspended,
            KernelMode,
            FALSE,
            NULL
            );

        ASSERT (m_StopHardware == FALSE);

        m_HardwareState = HardwarePaused; 

    } else if (!Pausing && m_HardwareState == HardwarePaused) {

        //
        // For unpausing the hardware, we need to compute the relative time
        // and restart interrupts.
        //
        LARGE_INTEGER NextDeltaTime;
        NextDeltaTime.QuadPart = -10000;
        m_LastTimerTime.QuadPart -= NextDeltaTime.QuadPart;//compensate the absolute time calculation

        m_HardwareState = HardwareRunning;
        KeSetTimer (&m_IsrTimer, NextDeltaTime, &m_IsrFakeDpc);//we restart the timer in 1 millisecond

    }

    return STATUS_SUCCESS;

}

/*************************************************/


NTSTATUS
CHardwareSimulation::
Stop (
    )

/*++

Routine Description:

    Stop the hardware simulation....  Wait until the hardware simulation
    has successfully stopped and then return.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{
    DbgPrint("CHardwareSimulation::Stop ");

    PAGED_CODE();

    //
    // If the hardware is told to stop while it's running, we need to
    // halt the interrupts first.  If we're already paused, this has
    // already been done.
    //
    if (m_HardwareState == HardwareRunning) {
    
        m_StopHardware = TRUE;
    
        KeWaitForSingleObject (
            &m_HardwareEvent,
            Suspended,
            KernelMode,
            FALSE,
            NULL
            );
    
        ASSERT (m_StopHardware == FALSE);

    }

    m_HardwareState = HardwareStopped;

    //
    // The image synthesizer may still be around.  Just for safety's
    // sake, NULL out the image synthesis buffer and toast it.
    //
    if ( m_TsSynth )
    {
        m_TsSynth -> SetBuffer (NULL);
        m_TsSynth = NULL;
    }

    if (m_SynthesisBuffer) {
        ExFreePool (m_SynthesisBuffer);
        m_SynthesisBuffer = NULL;
    }

    //
    // Delete the scatter / gather lookaside for this run.
    //
    ExDeleteNPagedLookasideList (&m_ScatterGatherLookaside);

    return STATUS_SUCCESS;

}

/**************************************************************************

    LOCKED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


ULONG
CHardwareSimulation::
ReadNumberOfMappingsCompleted (
    )

/*++

Routine Description:

    Read the number of scatter / gather mappings which have been
    completed (TOTAL NUMBER) since the last reset of the simulated
    hardware

Arguments:

    None

Return Value:

    Total number of completed mappings.

--*/

{

    //
    // Don't care if this is being updated this moment in the DPC...  I only
    // need a number to return which isn't too great (too small is ok).
    // In real hardware, this wouldn't be done this way anyway.
    //
    return m_NumMappingsCompleted;

}

/*************************************************/


ULONG
CHardwareSimulation::
ProgramScatterGatherMappings (
    IN PUCHAR *Buffer,
    IN PKSMAPPING Mappings,
    IN ULONG MappingsCount,
    IN ULONG MappingStride
    )

/*++

Routine Description:

    Program the scatter gather mapping list.  This shoves a bunch of 
    entries on a list for access during the fake interrupt.  Note that
    we have physical addresses here only for simulation.  We really
    access via the virtual address....  although we chunk it into multiple
    buffers to more realistically simulate S/G

Arguments:

    Buffer -
        The virtual address of the buffer mapped by the mapping list 

    Mappings -
        The KSMAPPINGS array corresponding to the buffer

    MappingsCount -
        The number of mappings in the mappings array

    MappingStride -
        The mapping stride used in initialization of AVStream DMA

Return Value:

    Number of mappings actually inserted.

--*/

{

    KIRQL Irql;

    ULONG MappingsInserted = 0;

    //
    // Protect our S/G list with a spinlock.
    //
    KeAcquireSpinLock (&m_ListLock, &Irql);

    //
    // Loop through the scatter / gather list and break the buffer up into
    // chunks equal to the scatter / gather mappings.  Stuff the virtual
    // addresses of these chunks on a list somewhere.  We update the buffer
    // pointer the caller passes as a more convenient way of doing this.
    //
    // If I could just remap physical in the list to virtual easily here,
    // I wouldn't need to do it.
    //
    
#if defined(_BUILD_SW_TUNER_ON_X64)
    do
    {
        PSCATTER_GATHER_ENTRY Entry =
            reinterpret_cast <PSCATTER_GATHER_ENTRY> (
                ExAllocateFromNPagedLookasideList (
                    &m_ScatterGatherLookaside
                    )
                );


        if (!Entry) {
            break;
        }

        Entry -> Virtual = *Buffer;
        Entry -> ByteCount = MappingsCount;

        //
        // Move forward a specific number of bytes in chunking this into
        // mapping sized va buffers.
        //
        *Buffer += MappingsCount;
        Mappings = reinterpret_cast <PKSMAPPING> (
            (reinterpret_cast <PUCHAR> (Mappings) + MappingStride)
            );

        InsertTailList (&m_ScatterGatherMappings, &(Entry -> ListEntry));
        MappingsInserted = MappingsCount;
        m_ScatterGatherMappingsQueued++;
        m_ScatterGatherBytesQueued += MappingsCount;
    }
#pragma warning(push)
#pragma warning(disable:4127)
    while (FALSE);
#pragma warning (pop)

#else
    for (ULONG MappingNum = 0; 
        MappingNum < MappingsCount &&
            m_ScatterGatherMappingsQueued < m_ScatterGatherMappingsMax; 
        MappingNum++) {

        PSCATTER_GATHER_ENTRY Entry =
            reinterpret_cast <PSCATTER_GATHER_ENTRY> (
                ExAllocateFromNPagedLookasideList (
                    &m_ScatterGatherLookaside
                    )
                );


        if (!Entry) {
            break;
        }

        Entry -> Virtual = *Buffer;
        Entry -> ByteCount = Mappings -> ByteCount;

        //
        // Move forward a specific number of bytes in chunking this into
        // mapping sized va buffers.
        //
        *Buffer += Entry -> ByteCount;
        Mappings = reinterpret_cast <PKSMAPPING> (
            (reinterpret_cast <PUCHAR> (Mappings) + MappingStride)
            );

        InsertTailList (&m_ScatterGatherMappings, &(Entry -> ListEntry));
        MappingsInserted++;
        m_ScatterGatherMappingsQueued++;
        m_ScatterGatherBytesQueued += Entry -> ByteCount;

    }

#endif

    KeReleaseSpinLock (&m_ListLock, Irql);

    return MappingsInserted;

}

/*************************************************/


NTSTATUS
CHardwareSimulation::
FillScatterGatherBuffers (
    LONGLONG * pllPcr
    )

/*++

Routine Description:

    The hardware has synthesized a buffer in scratch space and we're to
    fill scatter / gather buffers.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{
    //
    // We're using this list lock to protect our scatter / gather lists instead
    // of some hardware mechanism / KeSynchronizeExecution / whatever.
    //
    KeAcquireSpinLockAtDpcLevel (&m_ListLock);

    PUCHAR Buffer = reinterpret_cast<PUCHAR>(m_SynthesisBuffer);
    if (m_BufferRemaining) {
	    Buffer += m_SampleSize - m_BufferRemaining;
    } 
//    DbgPrint( "FillScatterGatherBuffers\n");

    //
    // Obtain location where data is stored.
    //
    Buffer = GetTsLocation (Buffer);

    LONGLONG llPCR = LONGLONG_MAX;

    //
    // For simplification, if there aren't enough scatter / gather buffers
    // queued, we don't partially fill the ones that are available.  We just
    // skip the frame and consider it starvation.
    //
    // This could be enforced by only programming scatter / gather mappings
    // for a buffer if all of them fit in the table also...
    //
    while (m_BufferRemaining &&
           m_ScatterGatherMappingsQueued > 0 &&
           m_ScatterGatherBytesQueued >= m_BufferRemaining) {

        LIST_ENTRY *listEntry = RemoveHeadList (&m_ScatterGatherMappings);
        m_ScatterGatherMappingsQueued--;

        PSCATTER_GATHER_ENTRY SGEntry =  
            reinterpret_cast <PSCATTER_GATHER_ENTRY> (
                CONTAINING_RECORD (
                    listEntry,
                    SCATTER_GATHER_ENTRY,
                    ListEntry
                    )
                );

        //
        // Since we're software, we'll be accessing this by virtual address...
        //
        ULONG BytesToCopy = 
           (m_BufferRemaining < SGEntry -> ByteCount) ?
           m_BufferRemaining :
            SGEntry -> ByteCount;

        //
        // Get last PCR...
        //
        LONGLONG pcr = GetLastPcr (
            Buffer,
            BytesToCopy
            );
        if (pcr != LONGLONG_MAX)
        {
            llPCR = pcr;
        }

        RtlCopyMemory (
            SGEntry -> Virtual,
            Buffer,
            BytesToCopy
            );

        m_BufferRemaining -= BytesToCopy;
        Buffer += BytesToCopy;
        m_NumMappingsCompleted++;
        m_ScatterGatherBytesQueued -= SGEntry -> ByteCount;

        //
        // Release the scatter / gather entry back to our lookaside.
        //
        ExFreeToNPagedLookasideList (
            &m_ScatterGatherLookaside,
            reinterpret_cast <PVOID> (SGEntry)
            );

        //
        // Check again location where data is stored.
        //
        Buffer = GetTsLocation (Buffer);
    }

    *pllPcr = llPCR;

    KeReleaseSpinLockFromDpcLevel (&m_ListLock);

    if (m_BufferRemaining) return STATUS_INSUFFICIENT_RESOURCES;
    else return STATUS_SUCCESS;
    
}



/*************************************************/


PUCHAR
CHardwareSimulation::
GetTsLocation (
    PUCHAR Buffer
    )

/*++

Routine Description:

    Get location where TS data is stored.
     - m_BufferRemaining should be updated.
     - If new file starts, some initialization happens here.

Arguments:

    Buffer -
        Current buffer location which this class is using.

Return Value:

    Buffer location (updated).

--*/

{
    BOOL fNewFile = FALSE;
    if (!m_BufferRemaining) {
	    Buffer = m_TsSynth -> GetTsLocation(&m_BufferRemaining, &fNewFile);
	    if (Buffer == NULL) 
	        m_BufferRemaining = 0;
    }

    // New file?
    if (fNewFile)
    {
        m_wPcrPid = WORD_MAX;
        m_llLastPcr = LONGLONG_MAX;
        m_llPcrOffset = LONGLONG_MAX;
        m_lCurrTSID = -1;
        KeQuerySystemTime (&m_StartTime);
        m_LastTimerTime.QuadPart = m_StartTime.QuadPart;
    }

    return Buffer;
}



/*************************************************/


void
CHardwareSimulation::
FakeHardware (
    )

/*++

Routine Description:

    Simulate an interrupt and what the hardware would have done in the
    time since the previous interrupt.

Arguments:

    None

Return Value:

    None

--*/

{

    m_InterruptTime++;

    LONGLONG llPcr = LONGLONG_MAX;

    //
    // The hardware can be in a pause state in which case, it issues interrupts
    // but does not complete mappings.  In this case, don't bother synthesizing
    // a frame and doing the work of looking through the mappings table.
    //
    if (m_HardwareState == HardwareRunning) {
    
        //
        // Fill scatter gather buffers
        //
        if (!NT_SUCCESS (FillScatterGatherBuffers (&llPcr))) {
            InterlockedIncrement (PLONG (&m_NumFramesSkipped));
        }

        //
        // Treat PCR and offset
        //
        if (m_llPcrOffset == LONGLONG_MAX && llPcr != LONGLONG_MAX)
        {
            m_llPcrOffset = llPcr;
            DbgPrint( "PCR Offset adjustment= %I64d\n", m_llPcrOffset); 
        }
        if (llPcr != LONGLONG_MAX)
        {
            if (m_llLastPcr != LONGLONG_MAX)
            {
                LONGLONG llElapsedTime = llPcr - m_llLastPcr;
                if (llElapsedTime < 0 || (27000000/10*7)/*0.7sec*/ < llElapsedTime)
                {
                    // Such gap cannot be allowed.
                    m_llPcrOffset += llElapsedTime;
                    DbgPrint( "growing PCR Offset to m_llPcrOffset= %I64d\n", m_llPcrOffset);
                }
            }
            m_llLastPcr = llPcr;
        }
    }
        
    //
    // Issue an interrupt to our hardware sink.  This is a "fake" interrupt.
    // It will occur at DISPATCH_LEVEL.
    //
    m_HardwareSink -> Interrupt ();

    //
    // Reschedule the timer if the hardware isn't being stopped.
    //
    if (!m_StopHardware) {

        //
        // Reschedule the timer for the next interrupt time.
        //
        if (llPcr == LONGLONG_MAX)
        {
            LARGE_INTEGER NextDeltaTime;
            NextDeltaTime.QuadPart = (-10000); // 1ms
            m_LastTimerTime.QuadPart -= NextDeltaTime.QuadPart; //compensate the calculation
            KeSetTimer (&m_IsrTimer, NextDeltaTime, &m_IsrFakeDpc);
            return;
        }

        //
        // Convert 27MHz(MPEG-TS) to 100ns-unit(system)
        //
        LARGE_INTEGER NextTime;
        LARGE_INTEGER NextDeltaTime;
        NextTime.QuadPart = m_StartTime.QuadPart + (llPcr - m_llPcrOffset) * 10 / 27;
        NextDeltaTime.QuadPart = m_LastTimerTime.QuadPart - NextTime.QuadPart;

        //every about one second we check the drift between relative and absolute, which is m_LastTimerTime vs current time
        //then we compensate the drift or if the clock was changed inbetween we correct the start of the time calculation
        
        if ((m_InterruptTime % 11 == 10) && (NextDeltaTime.QuadPart < 0))
        {
            LARGE_INTEGER CurrentTime;
            KeQuerySystemTime (&CurrentTime);
            LARGE_INTEGER DeltaTimeToReal; 
            DeltaTimeToReal.QuadPart = m_LastTimerTime.QuadPart - CurrentTime.QuadPart; //relative time vs absolute 
            
            if ( DeltaTimeToReal.QuadPart < (-10000000) )
            {
                //the clock was changed by the user into the future
                DbgPrint( "Timer was changed into the future\n");
                m_StartTime.QuadPart -= DeltaTimeToReal.QuadPart; 
            }
            else if ( DeltaTimeToReal.QuadPart > 10000000 )
            {
                //the clock was changed by the user into the past
                DbgPrint( "Timer was changed into the past\n");
                m_StartTime.QuadPart -= DeltaTimeToReal.QuadPart;
            }
            else
            {
                NextDeltaTime.QuadPart += DeltaTimeToReal.QuadPart; //compensate drift of the relative/absolute timing 
                if (NextDeltaTime.QuadPart >= 0)
                {
                    DbgPrint( "Timer compensation close to now, throttling\n");
                    NextDeltaTime.QuadPart = (-100000);
                }
            }
            m_LastTimerTime.QuadPart = CurrentTime.QuadPart - NextDeltaTime.QuadPart;//add the delta to the current time for the next comparison time
            //DbgPrint( "Compare Timers Tracking = %I64d  Real = %I64d\n", m_LastTimerTime.QuadPart - m_StartTime.QuadPart, CurrentTime.QuadPart - m_StartTime.QuadPart);
        }
        else
        {

            if (NextDeltaTime.QuadPart >= 0) //the time delta always should be negative except no timestamps are detected  
            {
                //DbgPrint( "Timer Delta for streaming wrong DeltaTime = %I64d\n", NextDeltaTime.QuadPart);
                NextDeltaTime.QuadPart = (-100000); // we wait 10ms if no timestamps are detected 
            }
            m_LastTimerTime.QuadPart -= NextDeltaTime.QuadPart; //compensate the calculation
        }
            
        KeSetTimer (&m_IsrTimer, NextDeltaTime, &m_IsrFakeDpc);
        
    } else {
        //
        // If someone is waiting on the hardware to stop, raise the stop
        // event and clear the flag.
        //
        m_StopHardware = FALSE;
        KeSetEvent (&m_HardwareEvent, IO_NO_INCREMENT, FALSE);
    }

}

/*************************************************/


LONGLONG
CHardwareSimulation::
GetLastPcr (
    PUCHAR  pBuffer,
    ULONG   ulBytes
    )

/*++

Routine Description:

    Returns the last PCR value in buffer.

Arguments:

    pBuffer -
        Buffer includes TS stream.

    ulBytes -
        Buffer size includes TS stream.

Return Value:

    ULONGLONG_MAX: in case the buffer does not include any PCR value

TO DO:

    We need to support that sometimes PCR_PID may be changed.

--*/

#define AF_PCR_FLAG                                 0x10
#define NTOH_S(s)                                   ( (((s) & 0xFF00) >> 8) | (((s) & 0x00FF) << 8) )
#define BYTE_VALUE(pb,i)                            (((BYTE *) (pb))[i])
#define BYTE_OFFSET_TS(pb,i)                        (& BYTE_VALUE((pb),i))
#define WORD_VALUE(pb,i)                            (* (UNALIGNED WORD *) BYTE_OFFSET_TS((pb),i))
#define PAYLOAD_UNIT_START_INDICATOR(pb)            ((BYTE_VALUE((pb),1) & 0x00000040) >> 6)
#define TRANSPORT_SCRAMBLING_CONTROL_VALUE(pb)      ((BYTE_VALUE((pb),3) & 0x000000c0) >> 6)
#define ADAPTATION_FIELD_CONTROL_VALUE(pb)          ((BYTE_VALUE((pb),3) & 0x00000030) >> 4)
#define ADAPTATION_FIELD_LENGTH_VALUE(pb)           (BYTE_VALUE((pb),0))
#define PID_VALUE(pb)                               (NTOH_S(WORD_VALUE(pb,1)) & 0x00001fff)
#define SWAP_WORD(pb,i)                             (NTOH_S(WORD_VALUE(pb,i)))
#define SWAP_DWORD(pb,i)                            ((SWAP_WORD(pb,i)<<16)|SWAP_WORD(pb,i+2))

{
    LONGLONG qwPCR = LONGLONG_MAX;

    while (ulBytes >= PACKET_SIZE/*188*/)
    {
        // Check TS sync byte
        if (ulBytes > PACKET_SIZE && (pBuffer[0] != 0x47 || pBuffer[PACKET_SIZE] != 0x47))
        {
            pBuffer ++;
            ulBytes --;
            continue;
        }
        else if (pBuffer[0] != 0x47)
        {
            pBuffer ++;
            ulBytes --;
            continue;
        }

#ifdef ISDBS_RECEIVER
        // PAT
        if (0 == PID_VALUE(pBuffer))
        {
            if (PAYLOAD_UNIT_START_INDICATOR(pBuffer))
            {
                BYTE bHeaderSize = 4 + (ADAPTATION_FIELD_CONTROL_VALUE(pBuffer) >= 2 ? ADAPTATION_FIELD_LENGTH_VALUE(pBuffer + 4) : 0);
                BYTE bPointer = pBuffer[bHeaderSize];
                if ((bHeaderSize + 1 + bPointer + 3) < (PACKET_SIZE - 1))
                {
                    WORD TSID = SWAP_WORD(pBuffer, bHeaderSize + 1 + bPointer + 3);
                    m_lCurrTSID = TSID;
                }
            }
            pBuffer += PACKET_SIZE;
            ulBytes -= PACKET_SIZE;
            continue;
        }
        // Is this packet PCR_PID ?
        else
#endif
        if (m_wPcrPid != WORD_MAX && PID_VALUE(pBuffer) != m_wPcrPid)
        {
            pBuffer += PACKET_SIZE;
            ulBytes -= PACKET_SIZE;
            continue;
        }

        // Is a PCR present in this packet?
        BOOL fPCR = ADAPTATION_FIELD_CONTROL_VALUE(pBuffer) >= 2 &&   // af?
        ADAPTATION_FIELD_LENGTH_VALUE(pBuffer + 4) &&                 // af_len > 0?
        (pBuffer[5] & AF_PCR_FLAG);                                   // PCR present
        if (fPCR == FALSE)
        {
            pBuffer += PACKET_SIZE;
            ulBytes -= PACKET_SIZE;
            continue;
        }

        // Get PCR
        qwPCR =  (SWAP_DWORD(pBuffer,  6) & 0x00000000FFFFFFFF) << 1;
        qwPCR += (BYTE_VALUE(pBuffer, 10) & 0x0000000000000080) >> 7;
        qwPCR *= 300;
        qwPCR += (BYTE_VALUE(pBuffer, 10) & 0x0000000000000001) << 8;
        qwPCR += (BYTE_VALUE(pBuffer, 11) & 0x00000000000000FF);

        // Store this PID
        m_wPcrPid = PID_VALUE(pBuffer);

        // Next packet
        pBuffer += PACKET_SIZE;
        ulBytes -= PACKET_SIZE;
    }

    return qwPCR;
}
