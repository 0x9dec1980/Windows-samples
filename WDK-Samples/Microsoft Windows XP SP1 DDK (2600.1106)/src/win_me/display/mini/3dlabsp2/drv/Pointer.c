/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: GDI  SAMPLE CODE        *
 *                           ***************************************
 *
 * Module Name: pointer.c
 *
 * This module contains the cursor related code. Currently we just use the
 * software cursor.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/


/* Include the windows header to get basic types, but have to ensure it doesnt define
   SetCursor!	*/
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

#define LOAD_GLINT_CTRL_REG(Register, Value) \
        VideoPortWriteRegisterUlong((ULONG FAR *)&(((PREGISTERS)(pGLInfo->pRegs))->Glint.Register), Value)  

#define VS_RAMDAC_MODE      0x1f1
#define VS_VIDEO_MODE       0x1f7

#define SET_P2_VS_MODE(mode)                                                \
    if (pGLInfo->dwRenderChipID == PERMEDIA2_ID || pGLInfo->dwRenderChipID == TIPERMEDIA2_ID) {   \
        LOAD_GLINT_CTRL_REG(VSConfiguration, mode);                     \
    }

typedef void    (FNSHOWPTR)(PREGISTERS, WORD);
typedef void    (FNMOVEPTR)(PREGISTERS, WORD, WORD);
typedef WORD    (FNSETPTR)(PREGISTERS, PPOINTERINFO);

FNSHOWPTR       Cursor_ShowPointerSw;
FNMOVEPTR       Cursor_MovePointerSw;
FNSETPTR        Cursor_SetPointerShapeSw;

FNSHOWPTR       Cursor_PermediaShowPointerTVP4020;
FNMOVEPTR       Cursor_PermediaMovePointerTVP4020;
FNSETPTR        Cursor_PermediaSetPointerShapeTVP4020;

FNSHOWPTR       *ShowPointerFn  = Cursor_ShowPointerSw; 
FNMOVEPTR       *MovePointerFn  = Cursor_MovePointerSw;
FNSETPTR        *SetPointerFn   = Cursor_SetPointerShapeSw;

POINTERINFO     CachedPointer;
signed int      CursorX;
signed int      CursorY;
WORD            PointerEnabled  = 0;
WORD            TrailingCursors = 0;
WORD            UpdatingPointer;
 
void            ClearCursorCache(void);    

//
// the RGB525 keeps its cursor shape as a set of 2 bit pixels. But we are
// given the data as a set of bytes. So we have to expand out each byte
// of the mask and source and interleave them. We will take a nibble at
// a time from each to create a byte. To help this along use a table
// which when indexed by the nibble expands it out to a byte leaving
// space. We choose to expand such that the 4 nibble bits occupy bits
// 1, 3, 5 and 7. In the code we shift this down by 1 if the nibble is
// part of the mask data.
//
BYTE    nibbleToByte[] = 
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

void SetupPointerFunctions()
{
    /* Change the pointer functions appropriately for the new pointer format */
#ifdef USE_HARDWARECURSOR
    if ((CachedPointer.csColor == 0x101) && (!TrailingCursors)) 
    {
        if  (pGLInfo->dwRamDacType == TVP4020_RAMDAC) 
        {
            ShowPointerFn           = Cursor_PermediaShowPointerTVP4020; 
            MovePointerFn           = Cursor_PermediaMovePointerTVP4020;
            SetPointerFn            = Cursor_PermediaSetPointerShapeTVP4020;
            pGLInfo->dwCursorType   = HARDWARECURSOR;
            return;
        }
    }
#endif //#ifdef USE_HARDWARECURSOR

    /* Set up software pointer functions		*/
    ShowPointerFn           = Cursor_ShowPointerSw; 
    MovePointerFn           = Cursor_MovePointerSw;
    SetPointerFn            = Cursor_SetPointerShapeSw;
    pGLInfo->dwCursorType   = SOFTWARECURSOR;
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
	    /* reset the pointer code - done on a mode change.		*/
	    ShowPointerFn = Cursor_ShowPointerSw;   
	    MovePointerFn = Cursor_MovePointerSw;   
	    SetPointerFn = Cursor_SetPointerShapeSw;

    }
    ClearCursorCache(); // To avoid trashed cursors after hibernate
}                       // on secondary cards

WORD PASCAL 
SetCursor(PPOINTERINFO pPointer) 
{
    ULONG OldFormat;
    WORD RetVal;
    LPDIBENGINE lpDriverPDev = (LPDIBENGINE)(pGLInfo->lpDriverPDevice);

    if (lpDriverPDev->deFlags & BUSY)
        return (0);

    if (pPointer == NULL)
    {
        (ShowPointerFn)((PREGISTERS)(pGLInfo->pRegs), 0);
        return (0);
    }

    UpdatingPointer = 1;

    OldFormat = CachedPointer.csColor;

    /* Update our cached copy		*/
    CachedPointer = *pPointer;

    /* If we changed cursor format, make sure our fn pointers are correct	*/
    if ((OldFormat != CachedPointer.csColor) || (!PointerEnabled)) 
    {
        /* Disable last pointer type		*/
        (ShowPointerFn)((PREGISTERS)(pGLInfo->pRegs), 0);
        /* Set up new functions			*/
        SetupPointerFunctions();
        PointerEnabled = 1;
    }

    /* Move the cursor to the correct postion on the screen		*/
    (MovePointerFn)((PREGISTERS)(pGLInfo->pRegs), CursorX, CursorY);

    /* Then set and enable it			*/
    RetVal = (SetPointerFn)((PREGISTERS)(pGLInfo->pRegs), pPointer);

    UpdatingPointer = 0;

    return (RetVal);
}

#pragma optimize("gle", off)
void PASCAL 
MoveCursor( WORD x, 
            WORD y)
{

    if (pGLInfo->dwFlags & GMVF_POWERDOWN)
    {
        pGLInfo->dwFlags &= ~GMVF_POWERDOWN;
        if (pGLInfo->dwRenderFamily == GLINT_ID) 
        {
            LOAD_GLINT_CTRL_REG(InterruptLine, pGLInfo->dwVGADisable);
        }
        else {
            LOAD_GLINT_CTRL_REG(VideoControl, pGLInfo->dwVideoControl);
        }
    }

    if (PointerEnabled && !UpdatingPointer)
        (MovePointerFn)((PREGISTERS)(pGLInfo->pRegs), x, y);

    CursorX = x;
    CursorY = y;
}
#pragma optimize("", on)

void PASCAL 
Cursor_ChangeMouseTrails(WORD nTrailing)
{

    if (nTrailing == 0) 
    {   /* Zero cursor trails	*/
        if (TrailingCursors == 0)
            return;
    }
    else {
        if (TrailingCursors != 0) 
        {   /* Just change the number of trails. Nothing to do	*/
            TrailingCursors = nTrailing;
            return;
        }
    }

    /* We have a change in our trailing cursor status		*/
    TrailingCursors = nTrailing;

    if (!PointerEnabled || pGLInfo->dwCursorType == SOFTWARECURSOR)
        return;

    /* Disable last pointer type			*/
    (ShowPointerFn)((PREGISTERS)(pGLInfo->pRegs), 0);

    /* Set up the new functions				*/
    SetupPointerFunctions();

    /* Move the new cursor to the correct postion on the screen	*/
    (MovePointerFn)((PREGISTERS)(pGLInfo->pRegs), CursorX, CursorY);

    /* Then set and enable it				*/
    (SetPointerFn)((PREGISTERS)(pGLInfo->pRegs), &CachedPointer);

}


/******************************Public*Routine******************************\
* void vShowPointerSw
*
* Show or hide the software pointer.
*
\**************************************************************************/

void 
Cursor_ShowPointerSw(PREGISTERS pRegisters, 
                     WORD       bShow)
{
    extern void PASCAL DIB_SetCursorExt(LPVOID, LPDIBENGINE);
    LPDIBENGINE lpDriverPDev = (LPDIBENGINE)(pGLInfo->lpDriverPDevice);

    if (lpDriverPDev->deFlags & BUSY)
        return;

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
* void vMovePointerSw
*
* Move the software pointer.
*
\**************************************************************************/

void
Cursor_MovePointerSw(PREGISTERS pRegisters, 
                     WORD       x, 
                     WORD       y)
{
    extern void PASCAL DIB_MoveCursorExt(WORD, WORD, LPDIBENGINE);
    LPDIBENGINE lpDriverPDev = (LPDIBENGINE)(pGLInfo->lpDriverPDevice);

    CursorX = x;
    CursorY = y;

    if (lpDriverPDev->deFlags & BUSY)
        return;

    if (!(pGLInfo->dwFlags & GMVF_GCOP)) 
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
Cursor_SetPointerShapeSw(PREGISTERS     pRegisters, 
                         PPOINTERINFO   pPointer)
{
    extern void PASCAL DIB_SetCursorExt(LPVOID, LPDIBENGINE);
    LPDIBENGINE lpDriverPDev = (LPDIBENGINE)(pGLInfo->lpDriverPDevice);

    if (lpDriverPDev->deFlags & BUSY)
        return (0);

    if (!(pGLInfo->dwFlags & GMVF_GCOP)) 
    {
        DIB_SetCursorExt(pPointer, lpDriverPDev);
        return(1);
    }
    return(0);
}

#pragma optimize("gle", off)

extern void near WriteCursorMask(BYTE *what, volatile ULONG *where, ULONG trail);

static DWORD RGBcursorImages[4][2*32];		/* 4 cursors, AND and XOR, 32 lines per cursor */
static unsigned int rgbDACCache[4];
static unsigned int clock = 0;
static int whichRGBCursor = 0;


void ClearCursorCache(void)
{
    memset( RGBcursorImages, 0, sizeof(RGBcursorImages) );
    memset( rgbDACCache,     0, sizeof(rgbDACCache) );
}

static int 
matches(PPOINTERINFO    pPointer, 
        DWORD           *image)
{
    DWORD	*xor, *and;
    int i;

    xor = (DWORD *)pPointer->csXORBits;
    and = (DWORD *)pPointer->csANDBits;

    for (i = 0; i < 32; i++) 
    {
	if (*image++ != *and++)
	    return 0;
	if (*image++ != *xor++)
	    return 0;
    }
    return 1;
} /* matches() */


void near 
WriteCursorMask4020(BYTE            *what, 
                    volatile ULONG  *where, 
                    int             whichCur, 
                    int             startOffset, 
                    PREGISTERS      pRegisters)
{
    volatile BYTE   *toWhat = (volatile BYTE *)where;
    int             y, thisY;
    pTVP4020RAMDAC  pTVP4020Regs = (pTVP4020RAMDAC)&(pRegisters->Glint.ExtVCReg);

    thisY = startOffset + ((whichCur & 2) * 128) + ((whichCur & 1) * 4);
    
    for (y = 0; y < 32; y++) 
    {
	TVP4020_CURSOR_ARRAY_START(thisY);
	thisY += 8;
	*toWhat = *what++;
	*toWhat = *what++;
	*toWhat = *what++;
	*toWhat = *what++;
    }
} // WriteCursorMask4020()


extern int whichRGBCursor;
/* This function determines which of the cached cursors should be used and returns that cursor index.
   It overwrites the oldest cursor if there isn't a copy that matches */
int 
selectTVP4020CachedCursorUse(PPOINTERINFO   pPointer, 
                             PREGISTERS     pRegisters)
{
    BYTE	    *pjAnd, *pjXor;
    int		    lru, i;
    unsigned int    time;
    DWORD	    *dest;
    int		    inset;
    pTVP4020RAMDAC   pTVP4020Regs = (pTVP4020RAMDAC)&(pRegisters->Glint.ExtVCReg);

    clock++;		/* The clock ticks every time we instantiate a new cursor */
    if (!clock) 
    {
	/* When the clock gets back to zero we have to zero all values to prevent the LRU approach
	   from hammering on just one cursor */
	for (i = 0; i < 4; i++)
	    rgbDACCache[i] = 0;
    }

    /* 1) Look for a matching cursor in the cache */
    for (i = 0; i < 4; i++) 
    {
	if (matches(pPointer, RGBcursorImages[i])) 
        {
	    rgbDACCache[i] = clock;			/* This is when it was last used after all... */
	    return i;
	}
    }

    /* 2) Given that we don't have the requested cursor in cache determine which to discard... */
    lru = 0;
    time = rgbDACCache[0];
    for (i = 1; i < 4; i++)
    {
	if (time > rgbDACCache[i]) 
        {
	    lru = i;
	    time = rgbDACCache[i];
	}
    }

    /* 3) Now that we have established that 'which' is the LRU cursor, we'll trash it */
    rgbDACCache[lru] = clock;		/* Set it's last used stamp... */

    inset = ((lru & 1) * 4) + ((lru & 2) * (64 * 4));

    /* Send cursor plane 0 - in our case XOR   */
    WriteCursorMask4020(pPointer->csXORBits, &pTVP4020Regs->curRAMData.reg, lru, 0, pRegisters);
    /* Send cursor plane 1 - in our case AND */
    WriteCursorMask4020(pPointer->csANDBits, &pTVP4020Regs->curRAMData.reg, lru, 0x200, pRegisters);

    /* and copy the image over the cached version... */
    pjAnd = pPointer->csANDBits;
    pjXor = pPointer->csXORBits;
    dest = RGBcursorImages[lru];
    for (i = 0; i < 32; i++) 
    {
	*dest++ = *(DWORD *)pjAnd;		/* copy 1 DWORD per line from each image */
	*dest++ = *(DWORD *)pjXor;
	pjAnd += 4;
	pjXor += 4;
    }

    return lru;
} /* selectTVP4020CachedCursorUse */

/******************************Public*Routine******************************\
* void vShowPointerTVP4020
*
* Show or hide the TI TVP4020 hardware pointer.
*
\**************************************************************************/
VOID
Cursor_PermediaShowPointerTVP4020(PREGISTERS    pRegisters,
                                  WORD          bShow)
{
    pTVP4020RAMDAC  pTVP4020Regs = (pTVP4020RAMDAC)&(pRegisters->Glint.ExtVCReg);
    WORD            CursorCtrl;

    /* TVP4020_SYNC_WITH_GLINT; */
    if (bShow)
    {
        CursorCtrl =    TVP4020_CURSOR_XGA | 
                        TVP4020_CURSOR_SIZE_32 | 
                        TVP4020_CURSOR_32_SEL(whichRGBCursor);
    }
    else
    {
        CursorCtrl = TVP4020_CURSOR_OFF;
    }

    if (pGLInfo->dwFlags & GMVF_NOIRQ) 
    {   // No interrupt handler - use direct access to cursors
        TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, CursorCtrl);
    }
    else 
    {   // Rely on interrupt handler to move cursor
        pGLInfo->wCursorControl = CursorCtrl | 0x8000;
    }
}

/******************************Public*Routine******************************\
* void vMovePointerTVP4020
*
* Move the TI TVP4020 hardware pointer.
*
\**************************************************************************/

VOID 
Cursor_PermediaMovePointerTVP4020(PREGISTERS   pRegisters, 
                                       WORD         x, 
                                       WORD         y)
{
    WORD            X, Y;
    pTVP4020RAMDAC  pTVP4020Regs = (pTVP4020RAMDAC)&(pRegisters->Glint.ExtVCReg);

    /*TVP4020_SYNC_WITH_GLINT; */
    X = x + 32 - CachedPointer.csHotX;
    Y = y + 32 - CachedPointer.csHotY;

    if (pGLInfo->dwFlags & GMVF_NOIRQ) 
    {   // No interrupt handler - use direct access to cursors
        TVP4020_MOVE_CURSOR(X, Y);
    }
    else 
    {
        WORD tempCursorControl;
        // Clear interrupt enable bit
        tempCursorControl = pGLInfo->wCursorControl;
        pGLInfo->wCursorControl &= ~0x8000;
        TVP4020_MOVE_CURSOR(X, Y);
        pGLInfo->wCursorControl = tempCursorControl;
    }
}

/******************************Public*Routine******************************\
* BYTE SetPointerShapeTVP4020
*
* Set the TI TVP4020 hardware pointer shape.
*
\**************************************************************************/

WORD 
Cursor_PermediaSetPointerShapeTVP4020(PREGISTERS    pRegisters, 
                                      PPOINTERINFO  pPointer)
{
    int             i;
    static BOOL     firstTime       = TRUE;
    pTVP4020RAMDAC  pTVP4020Regs    = (pTVP4020RAMDAC)&(pRegisters->Glint.ExtVCReg);
    
    /* 
     * We currently don't handle colored cursors. Later, we could handle
     * cursors up to 64x64 with <= 3 colors. Checking this and setting it up
     * may be more trouble than it's worth. 
     * Note also that the code doesn't claim to handle cursors larger than 32x32 
     * in order to only download the minimum 
     */
    if (pPointer->csColor != 0x101)
    {
    	return 0;
    }

    if (pPointer == NULL)
        return 1;

    // Clear interrupt enable bit 
    pGLInfo->wCursorControl &= ~0x8000;

    if (firstTime) 
    {
        /* Clear out all of the TVP cursor memory */
        TVP4020_CURSOR_ARRAY_START(0);
        for (i = 0; i < 64 * 8; i++)
            TVP4020_LOAD_CURSOR_ARRAY(0);
        for (i = 0; i < 64 * 8; i++)
            TVP4020_LOAD_CURSOR_ARRAY(0xff);
        firstTime = FALSE;
    }


    whichRGBCursor = selectTVP4020CachedCursorUse(pPointer, pRegisters);

    /* Set the position and hot spot of the cursor:*/
    if (CursorX >= 0)
    {
        WORD   X = CursorX, Y = CursorY, CursorCtrl;

        /* TVP4020_SYNC_WITH_GLINT; */
        if  (wScreenWidth < 512) 
        {
            /* When using Line doubling and Pixel Zooming, we need to double
            our coordinates. This gives us the weird effect that our cursor
            pixels are half the size of our screen pixels!
            */
            X <<= 1;
            Y <<= 1;
        }
        /* Enable the cursor:	*/
        CursorCtrl = TVP4020_CURSOR_XGA | TVP4020_CURSOR_SIZE_32 | TVP4020_CURSOR_32_SEL(whichRGBCursor);

        X += 32 - CachedPointer.csHotX;
        Y += 32 - CachedPointer.csHotY;

        TVP4020_MOVE_CURSOR(X, Y);
        if (pGLInfo->dwFlags & GMVF_NOIRQ) 
        {
            // No interrupt handler - use direct access to cursors
            TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, CursorCtrl);
        }
        else 
        {
            // Rely on interrupt handler to move cursor
            pGLInfo->wCursorControl = CursorCtrl | 0x8000;
        }
    }

    return(1);
}

#pragma optimize("", on)

