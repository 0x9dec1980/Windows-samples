/*++

Copyright (c) 1997-2000  Microsoft Corporation All Rights Reserved

Module Name:

    savedata.h

Abstract:

    Declaration of MSVAD data saving class. This class supplies services
to save data to disk.


--*/

#ifndef _MSVAD_SAVEDATA_H
#define _MSVAD_SAVEDATA_H

//-----------------------------------------------------------------------------
//  Forward declaration
//-----------------------------------------------------------------------------
class CSaveData;
typedef CSaveData *PCSaveData;


//-----------------------------------------------------------------------------
//  Structs 
//-----------------------------------------------------------------------------

// Parameter to workitem.
#include <pshpack1.h>
typedef struct _SAVEWORKER_PARAM {
    PIO_WORKITEM    pWorkItem;
    ULONG           ulFrameNo;
    ULONG           ulDataSize;
    PBYTE           pData;
    PCSaveData      pSaveData;
    KEVENT          EventDone;
} SAVEWORKER_PARAM;
typedef SAVEWORKER_PARAM *PSAVEWORKER_PARAM;
#include <poppack.h>

// wave file header.
#include <pshpack1.h>
typedef struct _OUTPUT_FILE_HEADER
{
    DWORD           dwRiff;
    DWORD           dwFileSize;
    DWORD           dwWave;
    DWORD           dwFormat;
    DWORD           dwFormatLength;
    WAVEFORMATEX    waveFormat;
    DWORD           dwData;
    DWORD           dwDataLength;
} OUTPUT_FILE_HEADER;
typedef OUTPUT_FILE_HEADER *POUTPUT_FILE_HEADER;
#include <poppack.h>

//-----------------------------------------------------------------------------
//  Classes
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// CSaveData
//   Saves the wave data to disk.
//   

class CSaveData
{
protected:
    UNICODE_STRING              m_FileName;         // DataFile name.
    HANDLE                      m_FileHandle;       // DataFile handle.
    PBYTE                       m_pDataBuffer;      // Data buffer.
    ULONG                       m_ulBufferSize;     // Total buffer size.

    ULONG                       m_ulFramePtr;       // Current Frame.
    ULONG                       m_ulFrameCount;     // Frame count.
    ULONG                       m_ulFrameSize;
    ULONG                       m_ulBufferPtr;      // Pointer in buffer.
    PBOOL                       m_fFrameUsed;       // Frame usage table.

    OBJECT_ATTRIBUTES           m_objectAttributes; // Used for opening file.

    PKMUTEX                     m_pFileMutex;       // Sync file access.

    OUTPUT_FILE_HEADER          m_FileHeader;
    PLARGE_INTEGER              m_pFilePtr;

    static PDEVICE_OBJECT       m_pDeviceObject;
    static ULONG                m_ulStreamId;
    static PSAVEWORKER_PARAM    m_pWorkItems;

    BOOL                        m_fWriteDisabled;

public:
    CSaveData();
    ~CSaveData();

    static void                 DestroyWorkItems
    (
        void
    );
    void                        Disable
    (
        BOOL                    fDisable
    );
    static PSAVEWORKER_PARAM    GetNewWorkItem
    (
        void
    );
    NTSTATUS                    Initialize
    (
        void
    );
    static NTSTATUS             InitializeWorkItems
    (
        IN  PDEVICE_OBJECT      DeviceObject
    );
    void                        ReadData
    (
        IN  PBYTE               pBuffer, 
        IN  ULONG               ulByteCount
    );
    void                        SetDataFormat
    (
        IN  PKSDATAFORMAT       pDataFormat
    );
    static void                 WaitAllWorkItems
    (
        void
    );
    void                        WriteData
    (
        IN  PBYTE               pBuffer, 
        IN  ULONG               ulByteCount
    );

private:
    NTSTATUS                    FileClose
    (
        void
    );
    NTSTATUS                    FileOpen
    (
        IN  BOOL                fOverWrite
    );
    NTSTATUS                    FileWrite
    (
        IN  PBYTE               pData, 
        IN  ULONG               ulDataSize
    );
    NTSTATUS                    FileWriteHeader
    (
        void
    );
   
    void                        SaveFrame
    (
        IN  ULONG               ulFrameNo, 
        IN  ULONG               ulDataSize
    );

    friend void                 SaveFrameWorkerCallback
    (
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PVOID               Context
    );
};
typedef CSaveData *PCSaveData;

#endif
