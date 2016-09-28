/******************************Module*Header**********************************\
*
*       **************************************************
*       * Permedia 2: Direct Draw/Direct 3D  SAMPLE CODE *
*       **************************************************
*
* Module Name: p2hw.c
*
*       Contains synchronization functions for Permedia 2 hardware
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"



/*
 * Function:    vWaitDMAComplete
 * Description: Flush the DMA Buffer and wait until all data is at least sent 
 *              to the chip. Does not wait until the graphics pipeline is idle.
 */

static VOID 
vWaitDMAComplete(LPP2THUNKEDDATA pThisDisplay)
{
    ULONG count;

    if (!(pThisDisplay->pPermediaInfo->GlintBoardStatus & P2_DMA_COMPLETE)) {
        if (pThisDisplay->pPermediaInfo->GlintBoardStatus & P2_INTR_CONTEXT) { 
            /* do any VBLANK wait, wait Q to empty and last DMA to complete */ 
            while (pThisDisplay->pPermediaInfo->frontIndex != pThisDisplay->pPermediaInfo->backIndex); 
        }
        
        while ((count = pThisDisplay->pPermedia2->DMACount) > 0)
        {
        if (count < 32)
            count = 1;      
        else                                
            count <<= 1;                    
        while (--count > 0) NULL;           
        }
        pThisDisplay->pPermediaInfo->GlintBoardStatus |= P2_DMA_COMPLETE;   
    }
}

/*
 * Function:    vSyncWithPermedia
 * Description: Send a sync tag through the Permedia and make sure all pending 
 *              reads and writes are flushed from the graphics pipeline. 
 *              MUST be called before accessing the Frame Buffer directly
 */

VOID 
vSyncWithPermedia(LPP2THUNKEDDATA pThisDisplay)
{ 
    vWaitDMAComplete(pThisDisplay);
    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 3);
    WRITE_PERMEDIA2_REGISTER(pThisDisplay, FilterMode, 0x400);
    WRITE_PERMEDIA2_REGISTER(pThisDisplay, Sync, 0x000);
    WRITE_PERMEDIA2_REGISTER(pThisDisplay, FilterMode, 0x000);

    do {
        while (READ_PERMEDIA2_REGISTER(pThisDisplay, OutFIFOWords) == 0);
    } while (READ_PERMEDIA2_REGISTER(pThisDisplay, GPFifo[0]) != 0x188);
}


/*
 * Function:    bDrawEngineBusy
 * Description: check if P2 is still busy drawing. 
 *              return----  TRUE  P2 is still busy
 *                          FALSE P2 has finished drawing and is not busy anymore
 */

BOOL bDrawEngineBusy(LPP2THUNKEDDATA pThisDisplay)
{
    if (READ_PERMEDIA2_REGISTER(pThisDisplay, DMACount) != 0) 
        return TRUE;
    if (READ_PERMEDIA2_REGISTER(pThisDisplay, FIFODiscon) & 0x80000000L) 
        return TRUE; // Graphics Processor is Busy
    return FALSE;
}

/*
 * Function:    updateFlipStatus
 * Description: return DD_OK when last flip has occured; DDERR_WASSTILLDRAWING
 *              otherwise.
 */

HRESULT 
updateFlipStatus(LPP2THUNKEDDATA pThisDisplay)
{
    DWORD dwVideoControl;

    dwVideoControl = READ_PERMEDIA2_REGISTER(pThisDisplay, VideoControl);
    if (dwVideoControl & 0xc180)
    {
        DBG_DD((7,"  VideoControl still pending (%08lx)",dwVideoControl));
        return DDERR_WASSTILLDRAWING;
    }
    return DD_OK;   
} // updateFlipStatus 

