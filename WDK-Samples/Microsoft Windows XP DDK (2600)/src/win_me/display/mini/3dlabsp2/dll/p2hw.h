/******************************Module*Header**********************************\
*
*                           ***************************************
*                           * Permedia 2: Direct 3D   SAMPLE CODE *
*                           ***************************************
*
* Module Name: p2hw.h
*
*       Contains Macro definitions and prototypes for Permedia 2
*       hardware specific functions.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef _DLL_P2HW_H_
#define _DLL_P2HW_H_
#include "p2def.h"

#define GLINT_DMA_COMPLETE      0x2     // set when there is no outstanding DMA
#define GLINT_INTR_CONTEXT      0x8     // set if the current context is interrupt enabled
#define GMVF_GCOP               0x00000010 // something is using 4K area (affects mouse)
#define GMVF_VBLANK_OCCURED 0x00010000 // VBlank has occured
#define GMVF_VBLANK_ENABLED     0x00020000 // VBlank interrupt is enabled
#define P2_DMA_COMPLETE         GLINT_DMA_COMPLETE
#define P2_INTR_CONTEXT         GLINT_INTR_CONTEXT

#define FOURCC_YUV2         0x32595559
#define FOURCC_Y411         0x31313459
#define FOURCC_YUV422       0x32595559    

/* 
 * Macros to read/write Permedia2 registers
 */
#define READ_PERMEDIA2_REGISTER(pThisDisplay, RegisterName)         \
                                        pThisDisplay->pPermedia2->RegisterName
#define WRITE_PERMEDIA2_REGISTER(pThisDisplay, RegisterName, Value) \
                                        pThisDisplay->pPermedia2->RegisterName = Value

/* 
 * Macros to check if the monitor is in Vertical Blank Period or in the Display Period
 */
#define IN_VRETRACE(pThisDisplay) \
            (READ_PERMEDIA2_REGISTER(pThisDisplay, LineCount) < READ_PERMEDIA2_REGISTER(pThisDisplay, VbEnd))

#define IN_DISPLAY(pThisDisplay)    !IN_VRETRACE(pThisDisplay)
   
#define CURRENT_VLINE(pThisDisplay) \
            (READ_PERMEDIA2_REGISTER(pThisDisplay, LineCount) - READ_PERMEDIA2_REGISTER(pThisDisplay, VbEnd))

/* 
 * Macros for writing to the Input Fifo
 * Please note that before calling LOAD_INPUT_FIFO or LD_INPUT_FIFO_DATA you need to 
 * ensure there is space in the fifo by calling WAIT_FOR_FREE_ENTRIES
 * One can implement DMA transfers simply by redefining these macros
 */

#define WAIT_FOR_FREE_ENTRIES(pThisDisplay, ulNumberOfEntries)  \
            while (READ_PERMEDIA2_REGISTER(pThisDisplay, InFIFOSpace) < ulNumberOfEntries);

#define LOAD_INPUT_FIFO_DATA(pThisDisplay, ulData) WRITE_PERMEDIA2_REGISTER(pThisDisplay, GPFifo[0], ulData);

#define LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, ulTag, ulData)   \
            {                                                   \
                LOAD_INPUT_FIFO_DATA(pThisDisplay, __Permedia2Tag##ulTag)   \
                LOAD_INPUT_FIFO_DATA(pThisDisplay, ulData)                  \
            }

#define LOAD_INPUT_FIFO_TAG_OFFSET_DATA(pThisDisplay, ulTag, ulData, lOffset)   \
            {                                                   \
                LOAD_INPUT_FIFO_DATA(pThisDisplay, (__Permedia2Tag##ulTag + lOffset))   \
                LOAD_INPUT_FIFO_DATA(pThisDisplay, ulData)                              \
            }

/*
 * Load "count" number of entries into Input Fifo of Permedia 2 from starting at pSrcData.
 */
_inline void LoadInputFifoNData(LPP2THUNKEDDATA pThisDisplay, BYTE *pSrcData, ULONG count)
{   
    ULONG     i; 
    DWORD   *pData = (DWORD *)pSrcData;

    while (count > 256) 
    {
        WAIT_FOR_FREE_ENTRIES(pThisDisplay, 256);
        for (i = 0; i < 256; i++)
        {
            LOAD_INPUT_FIFO_DATA(pThisDisplay, *pData);
            pData++;
        }
        count = count - 256;
    }

    if (count)
    {
        WAIT_FOR_FREE_ENTRIES(pThisDisplay, count);
        for (i = 0; i < count; i++)
        {
            LOAD_INPUT_FIFO_DATA(pThisDisplay, *pData);
            pData++;
        }
    }
}

/* 
 * Inline functions definitions for supporting stereo mode
 */
#define PREG_VC_STEREOENABLE 0x800

_inline void HWEnableStereoMode(LPP2THUNKEDDATA pThisDisplay)
{ 
    ULONG ulVControl;

    ulVControl = READ_PERMEDIA2_REGISTER(pThisDisplay, VideoControl);

    if ((ulVControl&PREG_VC_STEREOENABLE)==0 ||
        !pThisDisplay->bDdStereoMode)
    {
        pThisDisplay->bDdStereoMode=TRUE;
        WRITE_PERMEDIA2_REGISTER(pThisDisplay, VideoControl, (ulVControl|PREG_VC_STEREOENABLE));
    }
}

_inline void HWDisableStereoMode(LPP2THUNKEDDATA pThisDisplay)
{ 
    ULONG ulVControl;

    if (pThisDisplay->bDdStereoMode)
    {
        pThisDisplay->bDdStereoMode=FALSE;
        ulVControl = READ_PERMEDIA2_REGISTER(pThisDisplay, VideoControl);
        WRITE_PERMEDIA2_REGISTER(pThisDisplay, VideoControl, (ulVControl& ~PREG_VC_STEREOENABLE));
    }
}
// Load the new base address after >> 3 which is as per spec.
_inline void HWLoadNewFrameBaseAddress(LPP2THUNKEDDATA pThisDisplay, DWORD dwDDSurfaceOffset)
{
    WAIT_FOR_FREE_ENTRIES(pThisDisplay, 1);
    LOAD_INPUT_FIFO_TAG_DATA(pThisDisplay, SuspendUntilFrameBlank, (dwDDSurfaceOffset>>3));
}

_inline void HWLoadLeftSurfaceBaseAddress(LPP2THUNKEDDATA pThisDisplay, DWORD dwDDSurfaceOffset)
{
    WRITE_PERMEDIA2_REGISTER(pThisDisplay, ScreenBaseRight, (dwDDSurfaceOffset>>3));
} 

/*
 * Inline function for loading Base Address of AGP Textures.
 */
_inline void HWLoadAGPTextureBaseAddress(LPP2THUNKEDDATA pThisDisplay)
{
    WRITE_PERMEDIA2_REGISTER(pThisDisplay, AGPTexBaseAddress, pThisDisplay->dwGARTDev);
}


#ifdef DDFLIP_INTERVALN
/*
 * Inlines for supporting DDFLIP_INTERVALN
 */
_inline ULONG ulHWGetVerticalBlankIntrCount(LPP2THUNKEDDATA pThisDisplay)
{
    ULONG   ulP2IntrEnable = READ_PERMEDIA2_REGISTER(pThisDisplay, IntEnable);

    if (ulP2IntrEnable & 0x10)//mask for VBlank Interrupt Enable
    {
        return (pThisDisplay->pPermediaInfo->InterruptCount);
    }
    else
    {   // Vertical Blank Interrupts are not enabled so enable them
        pThisDisplay->pPermediaInfo->InterruptCount = 0;
        WRITE_PERMEDIA2_REGISTER(pThisDisplay, IntEnable, (ulP2IntrEnable| 0x10));
        return 0;
    }
}

_inline VOID vHWDisableVerticalBlankInterrupts(LPP2THUNKEDDATA pThisDisplay)
{
    ULONG   ulP2IntrEnable = READ_PERMEDIA2_REGISTER(pThisDisplay, IntEnable);

    pThisDisplay->pPermediaInfo->InterruptCount = 0;

    if (ulP2IntrEnable & 0x10)
        WRITE_PERMEDIA2_REGISTER(pThisDisplay, IntEnable, (ulP2IntrEnable & ~0x10));
}

#endif //#ifdef DDFLIP_INTERVALN

/* 
 * Macros for Starting/Stopping Software Cursor
 */
#define STOP_SOFTWARE_CURSOR(pThisDisplay) pThisDisplay->pPermediaInfo->dwFlags |= GMVF_GCOP;
#define START_SOFTWARE_CURSOR(pThisDisplay) pThisDisplay->pPermediaInfo->dwFlags &= ~GMVF_GCOP;

#define SyncHardware(pThisDisplay) vSyncWithPermedia(pThisDisplay)

/*
 * definitions in p2hw.c
 */
VOID    vSyncWithPermedia(LPP2THUNKEDDATA pThisDisplay);
BOOL    bDrawEngineBusy(LPP2THUNKEDDATA pThisDisplay);
HRESULT updateFlipStatus(LPP2THUNKEDDATA pThisDisplay);
/**************************************************************/

/*
 * Structure to capture the mapping between surface stride in pixels to 
 * Permedia 2 internal register format.
 */
typedef struct _P2_STRIDE_PP
{
    ULONG       ulStrideInPixels;
    ULONG       ulPartialProducts;
} PERMEDIA2_STRIDEPP, *PPERMEDIA2_STRIDEPP;

extern PERMEDIA2_STRIDEPP stridePartialProducts[];
#define PITCH_IN_PIXELS(width)  stridePartialProducts[(((width + 31)& ~31) >> 5)].ulStrideInPixels
#define PARTIAL_PRODUCTS(width) stridePartialProducts[(((width + 31)& ~31) >> 5)].ulPartialProducts

/*
 * Structure to capture the mapping between surface pixel format to 
 * Permedia 2 internal register format.
 */
typedef struct _P2_SURFACE_FORMAT
{
    DWORD   dwP2PixelSize;
    DWORD   dwP2Format;
    DWORD   dwP2FormatExt;
    DWORD   dwP2ColorOrder;
} P2_SURFACE_FORMAT, *PP2_SURFACE_FORMAT;

/*
 * Inlines for getting the starting offset of Direct Draw Surfaces 
 */
_inline DWORD  GetP2AGPMemoryOffset(LPP2THUNKEDDATA           pThisDisplay,
                                    FLATPTR                   fpVidMem,
                                    DWORD                     dwP2PixelSize)
{
    DWORD   dwAGPMemoryOffset;

    dwAGPMemoryOffset = ((DWORD)fpVidMem - pThisDisplay->dwGARTLin) >> dwP2PixelSize;
    dwAGPMemoryOffset |= ( 0x1 << 30); // From spec. to denote that it is AGP Memory
   
    return(dwAGPMemoryOffset);
}

_inline DWORD  GetP2VideoMemoryOffset(LPP2THUNKEDDATA           pThisDisplay,
                                      FLATPTR                   fpVidMem,
                                      DWORD                     dwP2PixelSize)
{
    return(((DWORD)fpVidMem - pThisDisplay->dwScreenFlatAddr) >> dwP2PixelSize);
}

/*
 * Waits for bunch of commands written into the Input Fifo to complete.
 */
_inline VOID vWaitForCmdCompletion(LPP2THUNKEDDATA  pThisDisplay)
{
    vSyncWithPermedia(pThisDisplay);
    while (bDrawEngineBusy(pThisDisplay));
}

/*
 * definitions in p2cntxt.c
 */
VOID    P2DDorD3DContextInit(LPP2THUNKEDDATA  pThisDisplay, ULONG ContextToSave);

#define VMemToVMemPackedCopyBltRegInit(pThisDisplay)        P2DDorD3DContextInit(pThisDisplay, D3D_OR_DDRAW_CONTEXT0) 
#define VidMemFastFillRegInit(pThisDisplay)                 P2DDorD3DContextInit(pThisDisplay, D3D_OR_DDRAW_CONTEXT0)
#define SymMemToVidMemPkdDwnldRegInit(pThisDisplay)         P2DDorD3DContextInit(pThisDisplay, D3D_OR_DDRAW_CONTEXT0)

#define VidMemToVidMemStretchCopyBltRegInit(pThisDisplay)   P2DDorD3DContextInit(pThisDisplay, D3D_OR_DDRAW_CONTEXT1)
#define VidMemToVidMemChromaBltRegInit(pThisDisplay)        P2DDorD3DContextInit(pThisDisplay, D3D_OR_DDRAW_CONTEXT1)
#define VidMemToVidMemStretchChromaBltRegInit(pThisDisplay) P2DDorD3DContextInit(pThisDisplay, D3D_OR_DDRAW_CONTEXT1)
#define VidMemYUVtoRGBRegInit(pThisDisplay)                 P2DDorD3DContextInit(pThisDisplay, D3D_OR_DDRAW_CONTEXT1)

#define VMemToVMemPackedCopyBltRegRestore(pThisDisplay)         vWaitForCmdCompletion(pThisDisplay)        
#define VidMemFastFillRegRestore(pThisDisplay)                  vWaitForCmdCompletion(pThisDisplay)    
#define SymMemToVidMemPkdDwnldRegRestore(pThisDisplay)          vWaitForCmdCompletion(pThisDisplay)

#define VidMemToVidMemStretchCopyBltRegRestore(pThisDisplay)    vWaitForCmdCompletion(pThisDisplay)
#define VidMemToVidMemChromaBltRegRestore(pThisDisplay)         vWaitForCmdCompletion(pThisDisplay)
#define VidMemToVidMemStretchChromaBltRegRestore(pThisDisplay)  vWaitForCmdCompletion(pThisDisplay)
#define VidMemYUVtoRGBRegRestore(pThisDisplay)                  vWaitForCmdCompletion(pThisDisplay)

#define DrawPrim2ContextRegSave(pContext)       P2DDorD3DContextInit(pContext->pThisDisplay, D3D_OR_DDRAW_CONTEXT2)
#define DrawPrim2ContextRegRestore(pContext)    vWaitForCmdCompletion(pContext->pThisDisplay)



VOID    DrawPrim2ContextRegInit(PERMEDIA_D3DCONTEXT  *pContext);

/*
 * definitions in p2hwmisc.c
 */
VOID
GetP2ChromaLevelBounds(PP2_SURFACE_FORMAT   pP2SurfaceFormat,
                       DWORD                *dwLowerBound,
                       DWORD                *dwUpperBound);
VOID 
GetP2SurfaceFormat(DDPIXELFORMAT        *pPixFormat,
                   PP2_SURFACE_FORMAT   pP2SurfaceFormat);

#endif /* #ifndef _DLL_P2HW_H_ */
 
