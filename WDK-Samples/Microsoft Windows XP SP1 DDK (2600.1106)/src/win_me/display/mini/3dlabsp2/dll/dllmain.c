/******************************Module*Header**********************************\
*
*       **************************************************
*       * Permedia 2: Direct Draw/Direct 3D  SAMPLE CODE *
*       **************************************************
*
* Module Name: dllmain.c
*
*       Main entry point to the 32bit DLL
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

/************************************************************************/
/* Prototypes for Public Functions                                      */
/************************************************************************/
BOOL WINAPI     DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved);
DWORD __stdcall DriverInit( DWORD dwDriverData );

/************************************************************************/
/* Prototypes for Local Functions                                       */
/************************************************************************/
static BOOL buildDDHalInfo32(LPP2THUNKEDDATA pThisDisplay);
static void SetupGlobalData();

/************************************************************************/
/* Globals                                                              */
/************************************************************************/

static              HINSTANCE           hInstance;
LPP2THUNKEDDATA     pDriverData         = NULL;
BOOL                D3DInitialised      = FALSE;
static LONG         lProcessCount       = 0;
HANDLE              hSharedHeap;
#define             HEAP_SHARED         0x04000000
#define             SHARED_HEAP_SIZE    1024*1024*4    //pool of memory from which the 
                                                       //all the DDRAW and D3D allocations
                                                       //are made. 
/************************************************************************/
/* Public Functions                                                     */
/************************************************************************/

/*
 * Function:    DllMain
 * Description: Main Entry point for the 32bit DLL
 */

BOOL WINAPI 
DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
    LONG        tmp;

    hInstance = hModule;    // The 16 bit side requires an HINSTANCE

    switch( dwReason ) {
        case DLL_PROCESS_ATTACH:  // The DLL is being mapped into the process's address space
            DisableThreadLibraryCalls( hModule );   // This tells the system we don't want 
                                                    // DLL_THREAD_ATTACH and DLL_THREAD_DETACH
                                                    // modifications sent to the specified
                                                    // DLL's DllMain function
            tmp = InterlockedExchange(&lProcessCount, -1);
            if (tmp == 0)
            {
                hSharedHeap = HeapCreate( HEAP_SHARED, SHARED_HEAP_SIZE, 0);
            }
            tmp++;
            InterlockedExchange(&lProcessCount, tmp);
        break;

        case DLL_PROCESS_DETACH:  // The DLL is being unmapped from the 

            tmp = InterlockedExchange(&lProcessCount, -1);
            tmp--;
            if( tmp == 0)         // Last process?
            {
                HeapDestroy( hSharedHeap);
                hSharedHeap = NULL;
            }
            InterlockedExchange(&lProcessCount, tmp);
        break;
        
        case DLL_THREAD_ATTACH:  // A thread is being created
        break;
        
        case DLL_THREAD_DETACH:  // A thread is exiting cleanly
        break;
        
        default:
        break;
    }    
    return TRUE;
} /* DllMain */

/*
 * Function:    DriverInit
 * Description: Initializes the 32bit portion of Direct Draw
 */

DWORD __stdcall 
DriverInit( DWORD dwDriverData )
{
    LPP2THUNKEDDATA pData;
    extern LPVOID _stdcall MapSL( DWORD );   // 16:16 -> 0:32

    pData = (LPP2THUNKEDDATA)MapSL(dwDriverData);

    if (pData->pPermediaInfo->dwDeviceHandle == 1)
    {
        pDriverData = (LPP2THUNKEDDATA)pData;
        DBG_DD((0, "Device is the Primary, Setting sData: 0x%x", pDriverData));
    }
    else
    {
        DBG_DD((0, "Device NOT Primary Display, Setting dwReturn: 0x%x", pData));
    }
    // Pass the current pointer to the init function
    if (!buildDDHalInfo32(pData) )
    {
        DBG_DD((0,"ERROR: DriverInit Failed!"));
        return 0;
    }
    else
    {
        DBG_DD((0,"Returned pDriverData"));
        return ((DWORD)pData);
    }

} /* DriverInit */

/************************************************************************/
/* Local Functions                                                      */
/************************************************************************/

/*
 * Function:    InitDDHAL32Bit
 * Descrition:  Initializes the Direct Draw HAL on the 32 bit side.
 */
static BOOL 
buildDDHalInfo32(LPP2THUNKEDDATA pThisDisplay)
{
    pThisDisplay->bDdStereoMode         = 0;
    pThisDisplay->dwGARTLin             = 0;
    pThisDisplay->dwGARTDev             = 0;
    pThisDisplay->pPermedia2            = (FPPERMEDIA2REG)pThisDisplay->control;
    pThisDisplay->dwNewDDSurfaceOffset  = 0xffffffff;
    pThisDisplay->hInstance = (DWORD)hInstance;

    InitDDHAL(pThisDisplay);

    pThisDisplay->bSetupThisDisplay = TRUE;

    return TRUE;
} /* Init32BitHal */


