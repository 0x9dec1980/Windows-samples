/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    perfvga.c

Abstract:

    This file implements the Extensible Objects for  the Vga object type

Created:    

    Russ Blake  24 Feb 93

Revision History


--*/

//
//  Include Files
//

#include <windows.h>
#include <string.h>
#include <winperf.h>
#include "vgactrs.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "datavga.h"

//
//  References to constants which initialize the Object type definitions
//

extern VGA_DATA_DEFINITION VgaDataDefinition;
    
DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    bInitOK = FALSE;        // true = DLL initialized OK

//
// Vga data structures
//

HANDLE hVgaSharedMemory;		 // Handle of Vga Shared Memory
PPERF_COUNTER_BLOCK pCounterBlock;

//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

DWORD APIENTRY   OpenVgaPerformanceData(LPWSTR);
DWORD APIENTRY   CollectVgaPerformanceData(LPWSTR, LPVOID *, LPDWORD, LPDWORD);
DWORD APIENTRY   CloseVgaPerformanceData(void);


DWORD APIENTRY
OpenVgaPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open and map the memory used by the VGA driver to
    pass performance data in. This routine also initializes the data
    structures used to pass data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (VGA)


Return Value:

    None.

--*/

{
    LONG status;
    TCHAR szMappedObject[] = TEXT("VGA_COUNTER_BLOCK");
    HKEY hKeyDriverPerf;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread 
    //  at a time so synchronization (i.e. reentrancy) should not be 
    //  a problem
    //

    if (!dwOpenCount) {
        // open Eventlog interface

        hEventLog = MonOpenEventLog();

        // open shared memory used by device driver to pass performance values

        hVgaSharedMemory = OpenFileMapping(FILE_MAP_READ,
				        FALSE,
				        szMappedObject);
        pCounterBlock = NULL;   // initialize pointer to memory

        // log error if unsuccessful

        if (hVgaSharedMemory == NULL) {
            REPORT_ERROR (VGAPERF_OPEN_FILE_MAPPING_ERROR, LOG_USER);
            // this is fatal, if we can't get data then there's no
            // point in continuing.
            status = GetLastError(); // return error
            goto OpenExitPoint;
        } else {
            // if opened ok, then map pointer to memory
	        pCounterBlock = (PPERF_COUNTER_BLOCK) 
			        MapViewOfFile(hVgaSharedMemory,
				            FILE_MAP_READ,
				            0,
				            0,
				            0);
	        if (pCounterBlock == NULL) {
                REPORT_ERROR (VGAPERF_UNABLE_MAP_VIEW_OF_FILE, LOG_USER);
                // this is fatal, if we can't get data then there's no
                // point in continuing.
                status = GetLastError(); // return error
	        }
        }

        // get counter and help index base values from registry
        //      Open key to registry entry
        //      read First Counter and First Help values
        //      update static data strucutures by adding base to 
        //          offset value in structure.

        status = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
    	    "SYSTEM\\CurrentControlSet\\Services\\Vga\\Performance",
            0L,
	        KEY_ALL_ACCESS,
            &hKeyDriverPerf);

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (VGAPERF_UNABLE_OPEN_DRIVER_KEY, LOG_USER,
                &status, sizeof(status));
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf, 
		            "First Counter",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstCounter,
                    &size);

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (VGAPERF_UNABLE_READ_FIRST_COUNTER, LOG_USER,
                &status, sizeof(status));
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf, 
        		    "First Help",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstHelp,
		    &size);

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (VGAPERF_UNABLE_READ_FIRST_HELP, LOG_USER,
                &status, sizeof(status));
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }
 
        //
        //  NOTE: the initialization program could also retrieve
        //      LastCounter and LastHelp if they wanted to do 
        //      bounds checking on the new number. e.g.
        //
        //      counter->CounterNameTitleIndex += dwFirstCounter;
        //      if (counter->CounterNameTitleIndex > dwLastCounter) {
        //          LogErrorToEventLog (INDEX_OUT_OF_BOUNDS);
        //      }

        VgaDataDefinition.VgaObjectType.ObjectNameTitleIndex += dwFirstCounter;
        VgaDataDefinition.VgaObjectType.ObjectHelpTitleIndex += dwFirstHelp;

        VgaDataDefinition.NumBitBlts.CounterNameTitleIndex += dwFirstCounter;
        VgaDataDefinition.NumBitBlts.CounterHelpTitleIndex += dwFirstHelp;

        VgaDataDefinition.NumTextOuts.CounterNameTitleIndex += dwFirstCounter;
        VgaDataDefinition.NumTextOuts.CounterHelpTitleIndex += dwFirstHelp;

        RegCloseKey (hKeyDriverPerf); // close key to registry

        bInitOK = TRUE; // ok to use this function
    }

    dwOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return status;
}


DWORD APIENTRY
CollectVgaPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the VGA counters.

Arguments:

   IN       LPWSTR   lpValueName
         pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed 
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the 
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the 
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added 
            by this routine 
         OUT: the number of objects added by this routine is writted to the 
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    //  Variables for reformating the data

    ULONG SpaceNeeded;
    PDWORD pdwCounter;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    VGA_DATA_DEFINITION *pVgaDataDefinition;
    DWORD   dwQueryType;

    //
    // before doing anything else, see if Open went OK
    //
    if (!bInitOK) {
        // unable to continue because open failed.
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }
    
    // see if this is a foreign (i.e. non-NT) computer data request 
    //
    dwQueryType = GetQueryType (lpValueName);
    
    if (dwQueryType == QUERY_FOREIGN) {
        // this routine does not service requests for data from
        // Non-NT computers
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS){
	if ( !(IsNumberInUnicodeList (VgaDataDefinition.VgaObjectType.ObjectNameTitleIndex, lpValueName))) {
             
            // request received for data object not provided by this routine
            *lpcbTotalBytes = (DWORD) 0;
    	    *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    pVgaDataDefinition = (VGA_DATA_DEFINITION *) *lppData;

    SpaceNeeded = sizeof(VGA_DATA_DEFINITION) +
		  SIZE_OF_VGA_PERFORMANCE_DATA;

    if ( *lpcbTotalBytes < SpaceNeeded ) {
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove(pVgaDataDefinition,
	   &VgaDataDefinition,
	   sizeof(VGA_DATA_DEFINITION));

    //
    //	Format and collect VGA data from shared memory 
    //

    pPerfCounterBlock = (PERF_COUNTER_BLOCK *) &pVgaDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_VGA_PERFORMANCE_DATA;

    pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

    *pdwCounter = *((PDWORD) pCounterBlock);
    *++pdwCounter = ((PDWORD) pCounterBlock)[1];

    *lppData = (PVOID) ++pdwCounter;

    // update arguments fore return
    
    *lpNumObjectTypes = 1;

    *lpcbTotalBytes = (PBYTE) pdwCounter - (PBYTE) pVgaDataDefinition;

    return ERROR_SUCCESS;
}


DWORD APIENTRY
CloseVgaPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to VGA device performance counters

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    if (!(--dwOpenCount)) { // when this is the last thread...

        CloseHandle(hVgaSharedMemory);

        pCounterBlock = NULL;

        MonCloseEventLog();
    }

    return ERROR_SUCCESS;

}

