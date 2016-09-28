/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 3: GDI  SAMPLE CODE        *
 *                           ***************************************
 *
 * Module Name: pointer.c
 *
 * This module contains the cursor related code. Currently we only support
 * 32x32 monochrome cursor, everything else is software cursor
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/


// Include the windows header to get basic types, but have to ensure it 
// doesnt define SetCursor
   
#define SetCursor SetWindowsCursor
#include "glint.h"
#undef SetCursor

#define INCLUDE_DELAY 1

#include "ramdac.h"

typedef struct 
{
    WORD    csHotX;       
    WORD    csHotY;       
    WORD    csWidth;      
    WORD    csHeight;     
    WORD    csWidthBytes; 
    WORD    csColor;         // 0101h = mono, 0801h = 8bpp, etc.
    BYTE    csANDBits[4*32]; // 32x32 bit mask = 4 bytes/row 
    BYTE    csXORBits[4*32]; // 32x32 color data = 4 bytes/row 
} POINTERINFO, FAR *PPOINTERINFO;

extern LPGLINTINFO PASCAL pGLInfo;

#define LOAD_GLINT_CTRL_REG(Register, Value)                            \
    VideoPortWriteRegisterUlong(                                        \
        (ULONG FAR *)&(((PREGISTERS)(pGLInfo->pRegs))->Glint.Register), \
        Value)  

#define VS_RAMDAC_MODE      0x1f1
#define VS_VIDEO_MODE       0x1f7

#define SET_P2_VS_MODE(mode)                            \
    if ((pGLInfo->dwRenderChipID == PERMEDIA2_ID) ||    \
        (pGLInfo->dwRenderChipID == TIPERMEDIA2_ID))    \
    {                                                   \
        LOAD_GLINT_CTRL_REG(VSConfiguration, mode);     \
    }

typedef void    (FNSHOWPTR)(PREGISTERS, WORD);
typedef void    (FNMOVEPTR)(PREGISTERS, WORD, WORD);
typedef WORD    (FNSETPTR)(PREGISTERS, PPOINTERINFO);

FNSHOWPTR       CursorShowPointerSw;
FNMOVEPTR       CursorMovePointerSw;
FNSETPTR        CursorSetPointerShapeSw;

FNSHOWPTR       CursorShowPointerP3RD;
FNMOVEPTR       CursorMovePointerP3RD;
FNSETPTR        CursorSetPointerShapeP3RD;

FNSHOWPTR       *ShowPointerFn = CursorShowPointerSw; 
FNMOVEPTR       *MovePointerFn = CursorMovePointerSw;
FNSETPTR        *SetPointerFn  = CursorSetPointerShapeSw;

POINTERINFO     CachedPointer;
signed int      CursorX;
signed int      CursorY;
WORD            PointerEnabled  = 0;
WORD            TrailingCursors = 0;
WORD            UpdatingPointer;
 
void            ClearCursorCache(void);    

//
// The RGB525 keeps its cursor shape as a set of 2 bit pixels. But we are
// given the data as a set of bytes. So we have to expand out each byte
// of the mask and source and interleave them. We will take a nibble at
// a time from each to create a byte. To help this along use a table
// which when indexed by the nibble expands it out to a byte leaving
// space. We choose to expand such that the 4 nibble bits occupy bits
// 1, 3, 5 and 7. In the code we shift this down by 1 if the nibble is
// part of the mask data.
//

BYTE nibbleToByteRGB525[] = 
{
    0x00,   // 0000 --> 00000000
    0x02,   // 0001 --> 00000010
    0x08,   // 0010 --> 00001000
    0x0A,   // 0011 --> 00001010
    0x20,   // 0100 --> 00100000
    0x22,   // 0101 --> 00100010
    0x28,   // 0110 --> 00101000
    0x2A,   // 0111 --> 00101010
    0x80,   // 1000 --> 10000000
    0x82,   // 1001 --> 10000010
    0x88,   // 1010 --> 10001000
    0x8A,   // 1011 --> 10001010
    0xA0,   // 1100 --> 10100000
    0xA2,   // 1101 --> 10100010
    0xA8,   // 1110 --> 10101000
    0xAA,   // 1111 --> 10101010
};

//
// P3RD has a very different set of nibble to byte pattern
//

BYTE nibbleToByteP3RD[] = {

    0x00,   // 0000 --> 00000000
    0x80,   // 0001 --> 10000000
    0x20,   // 0010 --> 00100000
    0xA0,   // 0011 --> 10100000
    0x08,   // 0100 --> 00001000
    0x88,   // 0101 --> 10001000
    0x28,   // 0110 --> 00101000
    0xA8,   // 0111 --> 10101000
    0x02,   // 1000 --> 00000010
    0x82,   // 1001 --> 10000010
    0x22,   // 1010 --> 00100010
    0xA2,   // 1011 --> 10100010
    0x0A,   // 1100 --> 00001010
    0x8A,   // 1101 --> 10001010
    0x2A,   // 1110 --> 00101010
    0xAA,   // 1111 --> 10101010
};


void SetupPointerFunctions()
{
    // Change the pointer functions appropriately for the new pointer format
#ifdef USE_HARDWARECURSOR
    if ((CachedPointer.csColor == 0x101) && (!TrailingCursors)) 
    {
        if  (pGLInfo->dwRamDacType == P3RD_RAMDAC) 
        {
            ShowPointerFn         = CursorShowPointerP3RD; 
            MovePointerFn         = CursorMovePointerP3RD;
            SetPointerFn          = CursorSetPointerShapeP3RD;
            pGLInfo->dwCursorType = HARDWARECURSOR;
            return;
        }        
    }
#endif // USE_HARDWARECURSOR

    // Set up software pointer functions
    ShowPointerFn         = CursorShowPointerSw; 
    MovePointerFn         = CursorMovePointerSw;
    SetPointerFn          = CursorSetPointerShapeSw;
    pGLInfo->dwCursorType = SOFTWARECURSOR;
}

void far InitialisePointer()
{
    if (PointerEnabled) 
    {
        SetupPointerFunctions();
        if (CachedPointer.csColor == 0x101) 
        {
	        (SetPointerFn)((PREGISTERS)(pGLInfo->pRegs), &CachedPointer);
        }
    }
    else 
    {
        // initialize Cached Pointer
        memset( &CachedPointer, 0, sizeof( POINTERINFO ) );
	    // reset the pointer code - done on a mode change.
	    ShowPointerFn = CursorShowPointerSw;   
	    MovePointerFn = CursorMovePointerSw;   
	    SetPointerFn = CursorSetPointerShapeSw;

    }
    
    //
    // To avoid trashed cursors after hibernate
    // on secondary cards
    //

    ClearCursorCache(); 
}

WORD PASCAL 
SetCursor(
    PPOINTERINFO    pPointer) 
{
    ULONG           OldFormat;
    WORD            wRet;
    LPDIBENGINE     lpDriverPDev = (LPDIBENGINE)(pGLInfo->lpDriverPDevice);

    if (lpDriverPDev->deFlags & BUSY)
        return (0);

    if (pPointer == NULL)
    {
        (ShowPointerFn)((PREGISTERS)(pGLInfo->pRegs), 0);
        PointerEnabled = 0;
        return (0);
    }

    UpdatingPointer = 1;

    OldFormat = CachedPointer.csColor;

    // Update our cached copy
    CachedPointer = *pPointer;

    // If we changed cursor format, make sure our fn pointers are correct
    if ((OldFormat != CachedPointer.csColor) || (!PointerEnabled)) 
    {
        // Disable last pointer type
        (ShowPointerFn)((PREGISTERS)(pGLInfo->pRegs), 0);
        // Set up new function pointers
        SetupPointerFunctions();
        PointerEnabled = 1;
    }

    // Move the cursor to the correct postion on the screen
    (MovePointerFn)((PREGISTERS)(pGLInfo->pRegs), CursorX, CursorY);

    // Then set and enable it
    wRet = (SetPointerFn)((PREGISTERS)(pGLInfo->pRegs), pPointer);

    UpdatingPointer = 0;

    return (wRet);
}

#pragma optimize("gle", off)
void PASCAL 
MoveCursor(
    WORD    x, 
    WORD    y)
{
    if (pGLInfo->dwFlags & GMVF_POWERDOWN)
    {
        pGLInfo->dwFlags &= ~GMVF_POWERDOWN;
        if (pGLInfo->dwRenderFamily == GLINT_ID) 
        {
            LOAD_GLINT_CTRL_REG(InterruptLine, pGLInfo->dwVGADisable);
        }
        else
        {
            LOAD_GLINT_CTRL_REG(VideoControl, pGLInfo->dwVideoControl);
        }
    }

    if (PointerEnabled && (! UpdatingPointer))
    {
        (MovePointerFn)((PREGISTERS)(pGLInfo->pRegs), x, y);
    }

    CursorX = x;
    CursorY = y;
}
#pragma optimize("", on)

void PASCAL 
Cursor_ChangeMouseTrails(
    WORD    nTrailing)
{

    if (nTrailing == 0) 
    {   // Zero cursor trails
        if (TrailingCursors == 0)
        {
            return;
        }
    }
    else
    {
        if (TrailingCursors != 0) 
        {   // Just change the number of trails. Nothing to do
            TrailingCursors = nTrailing;
            return;
        }
    }

    // We have a change in our trailing cursor status
    TrailingCursors = nTrailing;

    if ((! PointerEnabled) || (pGLInfo->dwCursorType == SOFTWARECURSOR))
    {
        return;
    }

    // Disable last pointer type
    (ShowPointerFn)((PREGISTERS)(pGLInfo->pRegs), 0);

    // Set up the new functions
    SetupPointerFunctions();

    // Move the new cursor to the correct postion on the screen
    (MovePointerFn)((PREGISTERS)(pGLInfo->pRegs), CursorX, CursorY);

    // Then set and enable it
    (SetPointerFn)((PREGISTERS)(pGLInfo->pRegs), &CachedPointer);

}


/******************************Public*Routine******************************\
* void CursorShowPointerSw
*
* Show or hide the software pointer.
*
\**************************************************************************/

void 
CursorShowPointerSw(
    PREGISTERS  pRegisters, 
    WORD        bShow)
{
    extern void PASCAL DIB_SetCursorExt(LPVOID, LPDIBENGINE);
    LPDIBENGINE lpDriverPDev = (LPDIBENGINE)(pGLInfo->lpDriverPDevice);

    if (lpDriverPDev->deFlags & BUSY)
    {
        return;
    }

    if (bShow)
    {
        DIB_SetCursorExt(&CachedPointer, lpDriverPDev);
    }
    else
    {
        DIB_SetCursorExt(0, lpDriverPDev);
    }
}

/******************************Public*Routine******************************\
* void CursorMovePointerSw
*
* Move the software pointer.
*
\**************************************************************************/

void
CursorMovePointerSw(
    PREGISTERS  pRegisters, 
    WORD        x, 
    WORD        y)
{
    extern void PASCAL DIB_MoveCursorExt(WORD, WORD, LPDIBENGINE);
    LPDIBENGINE lpDriverPDev = (LPDIBENGINE)(pGLInfo->lpDriverPDevice);

    CursorX = x;
    CursorY = y;

    if (lpDriverPDev->deFlags & BUSY)
    {
        return;
    }

    if (! (pGLInfo->dwFlags & GMVF_GCOP)) 
    {
        DIB_MoveCursorExt(x, y, lpDriverPDev);
    }
}

/******************************Public*Routine******************************\
* BYTE SetPointerShapeSw
*
* Set the software pointer shape.
*
\**************************************************************************/

WORD 
CursorSetPointerShapeSw(
    PREGISTERS      pRegisters, 
    PPOINTERINFO    pPointer)
{
    extern void PASCAL DIB_SetCursorExt(LPVOID, LPDIBENGINE);
    LPDIBENGINE     lpDriverPDev = (LPDIBENGINE)(pGLInfo->lpDriverPDevice);

    if (lpDriverPDev->deFlags & BUSY)
    {
        return (0);
    }

    if (! (pGLInfo->dwFlags & GMVF_GCOP)) 
    {
        DIB_SetCursorExt(pPointer, lpDriverPDev);
        return (1);
    }

    return(0);
}

#pragma optimize("gle", off)

// 4 cursors, AND and XOR, 32 lines per cursor
static DWORD        RGBcursorImages[4][2*32];  
static unsigned int rgbDACCache[4];
static unsigned int clock = 0;
static int          whichRGBCursor = 0;


void ClearCursorCache(void)
{
    memset(RGBcursorImages, 0, sizeof(RGBcursorImages));
    memset(rgbDACCache, 0, sizeof(rgbDACCache));
}

static int 
Matches(
    PPOINTERINFO    pPointer, 
    DWORD          *image)
{
    DWORD	       *xor, *and;
    int             i;

    xor = (DWORD *)pPointer->csXORBits;
    and = (DWORD *)pPointer->csANDBits;

    for (i = 0; i < 32; i++) 
    {
        if (*image++ != *and++)
        {
            return 0;
        }
        if (*image++ != *xor++)
        {
            return 0;
        }
    }

    return (1);
} // Matches()

//
// This function will load cursor data into the P3RD RAMDAC
// P3RD is 16 bytes X 64 rows is divided into 4 quanrants for 32x32 mono cursor
//

int
LoadCursorDataP3RD(
    PPOINTERINFO    pPointer, 
    pP3RDRAMDAC     pP3RDRegs,
    WORD            wCursor)            // Which cursor to load
{
    WORD            wOffset;
    WORD            i, j;
    BYTE            ucCursorColor;
    BYTE           *pANDBits;
    BYTE           *pXORBits;
    BYTE            ucANDBits;
    BYTE            ucXORBits;

    // Sanity check
    if ((! pPointer) || (! pP3RDRegs) || (wCursor > 3)) 
    {
        return (0);
    }

    //
    // Calculate the initial offset, P3RD actually has linear cursor heap
    //

    wOffset = wCursor*8*32;

    // Initialize the AND and XOR bit pointers
    pANDBits = &pPointer->csANDBits[0];
    pXORBits = &pPointer->csXORBits[0];

    P3RD_CURSOR_ARRAY_START(wOffset);

    // Load the into RAMDAC row by row
    for (i = 0; i < 32; i++) 
    {
        // 4 bytes (32 pixel) per row
        for (j = 0; j < 4; j++, pANDBits++, pXORBits++) 
        {
           ucANDBits = *pANDBits;
           ucXORBits = *pXORBits;

           ucCursorColor = nibbleToByteP3RD[ucANDBits >> 4] |
                           (nibbleToByteP3RD[ucXORBits >> 4] >> 1);
           P3RD_LOAD_CURSOR_ARRAY(ucCursorColor);
           
           ucCursorColor = nibbleToByteP3RD[ucANDBits & 0xF] |
                           (nibbleToByteP3RD[ucXORBits & 0xF] >> 1);
           P3RD_LOAD_CURSOR_ARRAY(ucCursorColor);
        }
    }

    // Load the black / white into the cursor palette
    P3RD_CURSOR_PALETTE_CURSOR_RGB(0, 0x00, 0x00, 0x00);
    P3RD_CURSOR_PALETTE_CURSOR_RGB(1, 0xFF, 0xFF, 0xFF);
        
    return (1);
}


//
// This function determines which of the cached cursors should be 
// used and returns that cursor index.
// It overwrites the oldest cursor if there isn't a copy that matches
//

#define NUM_CACHE_SLOT  4

int 
SelectP3RDCachedCursorUse(
    PPOINTERINFO    pPointer, 
    PREGISTERS      pRegisters)
{
    BYTE	       *pjAnd, *pjXor;
    int		        lru, i;
    unsigned int    time;
    DWORD	       *dest;
    pP3RDRAMDAC     pP3RDRegs = (pP3RDRAMDAC)&(pRegisters->Glint.ExtVCReg);

    // The clock ticks every time we instantiate a new cursor
    clock++;		
    if (! clock) 
    {
        //
        // When the clock gets back to zero we have to zero all values 
        // to prevent the LRU approach from hammering on just one cursor
        //

        for (i = 0; i < NUM_CACHE_SLOT; i++)
        {
            rgbDACCache[i] = 0;
        }
    }

    //
    // 1) Look for a matching cursor in the cache
    //

    for (i = 0; i < NUM_CACHE_SLOT; i++) 
    {
        if (Matches(pPointer, RGBcursorImages[i])) 
        {
            rgbDACCache[i] = clock;	// This is when it was last used
            return (i);
        }
    }

    //
    // 2) Given that we don't have the requested cursor in cache 
    //    determine which to discard...

    lru = 0;
    time = rgbDACCache[0];
    for (i = 1; i < NUM_CACHE_SLOT; i++)
    {
        if (time > rgbDACCache[i]) 
        {
            lru = i;
            time = rgbDACCache[i];
        }
    }

    //
    // 3) Now that we have established that 'which' is the
    //    LRU cursor, we'll replace it with the new one.
    //
    
    rgbDACCache[lru] = clock;		// Set it's last used stamp...

    // Download cursor data into the P3RD
    LoadCursorDataP3RD(
        pPointer,               // The cursor data
        pP3RDRegs,              // The RAMDAC register
        lru);                   // Which cursor to load

    // And update the cached version...
    pjAnd = pPointer->csANDBits;
    pjXor = pPointer->csXORBits;
    dest = RGBcursorImages[lru];
    for (i = 0; i < 32; i++) 
    {
        *dest++ = *(DWORD *)pjAnd;  // copy 1 DWORD per line from each image
        *dest++ = *(DWORD *)pjXor;
        pjAnd += 4;
        pjXor += 4;
    }

    return (lru);

} // SelectP3RDCachedCursorUse


/******************************Public*Routine******************************\
* void CursorShowPointerP3RD
*
* Show or hide the P3RD hardware pointer.
*
\**************************************************************************/
VOID
CursorShowPointerP3RD(
    PREGISTERS  pRegisters,
    WORD        bShow)
{
    pP3RDRAMDAC pP3RDRegs = (pP3RDRAMDAC)&(pRegisters->Glint.ExtVCReg);
    WORD        CursorMode;

    // P3RD_SYNC_WITH_GLINT; 
    if (bShow)
    {
        CursorMode = P3RD_CURSOR_MODE_ENABLED | 
                     P3RD_CURSOR_SEL(P3RD_CURSOR_SIZE_32_MONO, whichRGBCursor) |
                     P3RD_CURSOR_MODE_WINDOWS;
    }
    else
    {
        CursorMode = P3RD_CURSOR_MODE_OFF;
    }

    if (pGLInfo->dwFlags & GMVF_VBLANK_ENABLED) 
    {   // Rely on interrupt handler to move cursor
        pGLInfo->wCursorMode = CursorMode | 0x8000;
    }
    else 
    {   // No interrupt handler - use direct access to cursors
        if (! CursorMode) 
        {
            //
            // Hotspot of P3RD must maintained correctly
            //

            P3RD_CURSOR_HOTSPOT(CachedPointer.csHotX, CachedPointer.csHotY);
        }
        P3RD_SET_CURSOR_MODE(CursorMode);
    }
}

/******************************Public*Routine******************************\
* void CursorMovePointerP3RD
*
* Move the P3RD hardware pointer.
*
\**************************************************************************/

VOID 
CursorMovePointerP3RD(
    PREGISTERS  pRegisters, 
    WORD        x, 
    WORD        y)
{
    pP3RDRAMDAC pP3RDRegs = (pP3RDRAMDAC)&(pRegisters->Glint.ExtVCReg);

    // P3RD_SYNC_WITH_GLINT;
    if (pGLInfo->dwFlags & GMVF_VBLANK_ENABLED) 
    {
        WORD tmpCursorMode;
        // Clear interrupt enable bit
        tmpCursorMode = pGLInfo->wCursorMode;
        pGLInfo->wCursorMode &= ~0x8000;
        P3RD_MOVE_CURSOR(x, y);
        pGLInfo->wCursorMode = tmpCursorMode;
    }
    else 
    {   // No interrupt handler - use direct access to cursors
        P3RD_MOVE_CURSOR(x, y);
    }
}

/******************************Public*Routine******************************\
* BYTE CursorSetPointerShapeP3RD
*
* Set the P3RD hardware pointer shape.
*
\**************************************************************************/

WORD 
CursorSetPointerShapeP3RD(
    PREGISTERS      pRegisters, 
    PPOINTERINFO    pPointer)
{
    int             i;
    static BOOL     firstTime = TRUE;
    pP3RDRAMDAC     pP3RDRegs = (pP3RDRAMDAC)&(pRegisters->Glint.ExtVCReg);
    
    // 
    // We currently don't handle colored cursors. Later, we could handle
    // cursors up to 64x64 with <= 3 colors. Checking this and setting it up
    // may be more trouble than it's worth. 
    // Note also that the code doesn't claim to handle cursors larger than 32x32 
    // in order to only download the minimum 
    //
    if (pPointer->csColor != 0x101)
    {
    	return (0);
    }

    if ((pPointer->csWidth != 32) || (pPointer->csHeight != 32)) 
    {
        return (0);
    }

    if (pPointer == NULL)
    {
        return (1);
    }

    // Clear interrupt enable bit 
    pGLInfo->wCursorMode &= ~0x8000;

    if (firstTime) 
    {
        // Clear out all of the P3RD cursor RAM
        P3RD_CURSOR_ARRAY_START(0);
        
        // 16 bytes per row, 64 rows 
        for (i = 0; i < 16*64; i++)
        {
            P3RD_LOAD_CURSOR_ARRAY(0);
        }
        
        firstTime = FALSE;
    }

    //
    // By current design, the P3RD cursor memory can hold up to 
    // 4 32x32 monochrome cursors
    //

    whichRGBCursor = SelectP3RDCachedCursorUse(pPointer, pRegisters);

    // Set the position and hot spot of the cursor:
    if (CursorX >= 0)
    {
        WORD   X = CursorX, Y = CursorY, CursorMode;

        // P3RD_SYNC_WITH_GLINT;
        if  (wScreenWidth < 512) 
        {
            // When using Line doubling and Pixel Zooming, we need to double
            // our coordinates. This gives us the weird effect that our cursor
            // pixels are half the size of our screen pixels!
            //
            X <<= 1;
            Y <<= 1;
        }

        // Enable the cursor:
        CursorMode = P3RD_CURSOR_MODE_ENABLED | 
                     P3RD_CURSOR_SEL(P3RD_CURSOR_SIZE_32_MONO, whichRGBCursor) |
                     P3RD_CURSOR_MODE_WINDOWS;

        // Set the hotspot of the cursor, if even pattern may not have been reloaded
        P3RD_CURSOR_HOTSPOT(pPointer->csHotX, pPointer->csHotY);
    
        P3RD_MOVE_CURSOR(X, Y);
        if (pGLInfo->dwFlags & GMVF_VBLANK_ENABLED) 
        {
            // Rely on interrupt handler to move cursor
            pGLInfo->wCursorMode = CursorMode | 0x8000;
        }
        else 
        {
            // No interrupt handler - use direct access to cursors
            P3RD_SET_CURSOR_MODE(CursorMode);
        }
    }

    return (1);
}

#pragma optimize("", on)

