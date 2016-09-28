/******************************Module*Header**********************************\
*
*                           **********************************
*                           * Permedia 2: SAMPLE CODE        *
*                           **********************************
*
* Module Name: heap.c
*
*
*
* This code is part of the NT 3.51 offscreen heap code. It has been modified
* for Windows 95 on a 3Dlabs based board.
*
* This module contains the routines for a 2-d heap.  It is used primarily
* for allocating space for device-format-bitmaps in off-screen memory.
*
* Off-screen bitmaps are a big deal on Windows 95 because:
*
*    1) It reduces the working set.  Any bitmap stored in off-screen
*       memory is a bitmap that isn't taking up space in main memory.
*       The space is still actually allocated, but it should be paged out
*
*    2) There is a speed win by using the accelerator hardware for
*       drawing, in place of the DIB Engine
*
*    3) It leads naturally to nifty tricks that can take advantage of
*       the hardware, such as MaskBlt support and cheap double buffering
*       for OpenGL.
*
* The heap algorithm employed herein attempts to solve an unsolvable
* problem: the problem of keeping arbitrary sized bitmaps as packed as
* possible in a 2-d space, when the bitmaps can come and go at random.
*
* This problem is due entirely to the nature of the hardware for which this
* driver is written: the hardware treats everything as 2-d quantities.  If
* the hardware bitmap pitch could be changed so that the bitmaps could be
* packed linearly in memory, the problem would be infinitely easier (it is
* much easier to track the memory, and the accelerator can be used to re-pack
* the heap to avoid segmentation).
*
* If your hardware can treat bitmaps as one dimensional quantities (as can
* the XGA and ATI), by all means please implement a new off-screen heap.
*
* When the heap gets full, old allocations will automatically be punted
* from off-screen and copied to DIBs, which we'll let the DIB Engine draw on.
*
* Note that this heap manages reverse-L shape off-screen memory
* configurations (where the scan pitch is longer than the visible screen,
* such as happens at 800x600 when the scan length must be a multiple of
* 1024).
*
* NOTE: All heap operations must be done under some sort of synchronization,
*       whether it's controlled by GDI or explicitly by the driver.  All
*       the routines in this module assume that they have exclusive access
*       to the heap data structures; multiple threads partying in here at
*       the same time would be a Bad Thing.  
*
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\**************************************************************************/

#include "glint.h"
#include "heap.h"
#include "debug.h"

#define OH_ALLOC_SIZE   8192        // Do all memory allocations in 8k chunks
#define OH_QUANTUM      8           // The minimum dimension of an allocation
#define CXCY_SENTINEL   0x7fffffffl // The sentinel at the end of the available
                                    //  list has this very large 'cxcy' value

// This macro results in the available list being maintained with a
// cx-major, cy-minor sort:

#define CXCY(cx, cy) ((((LONG)cx) << 16) | ((LONG)cy))

HEAP    OSB_HeapInfo = {0};
WORD    OffscreenEnabled = 0;
LONG	ScreenPixelOffset;

extern OH far *pohValidateAndMoveOffscreenDfbToDib(OH far *);

#define CalculatePixelOffsetForHeap(poh)    (((LONG)wScreenWidth) * ((LONG)poh->y) + (LONG)poh->x + ScreenPixelOffset)

#define CalculateScreenAddrForHeap(poh) ((CalculatePixelOffsetForHeap(poh)) * (wBpp >> 3))

#ifdef DEBUG


extern int DD_DebugLevel;

/*
 * Msg
 *
 * display a message box
 */
#define VALIDATEHEAPINFO(str)   ValidateHeapInfo(str)
#define DISPDBG(abc)            DebugPrint abc
#define DISPPOH(abc)            DebugPoh abc
#define DISPOSB(abc)            DebugOSB abc
#define DISPDIBENGINE(abc)      DebugDIBENGINE abc
#define RIP(x)                  { DebugPrint(-1000, x); DebugCrash();}
#define ASSERTDD(x, y)          if (!(x)) RIP (y)

#pragma warning (disable:4704)
void DebugCrash() {
    _asm    int 1
}
#pragma warning (default:4704)

void ValidateHeapInfo(char *str)
{
	if (!OffscreenEnabled)
		return;
	DISPDBG((0, str));		// tell the debug stream who the caller was...
}

void DebugPoh(int Value, OH far *poh) {
    WORD    Sel, Off, Sel1, Off1;

    if (poh) {
        if (Value <= DD_DebugLevel) {
            Off = *((WORD *)(&poh));
            Sel = *((WORD *)(&poh) + 1);
            DISPDBG((Value, "%4x:%x %d,%d %d x %d, Flags 0x%x, cxcy 0x%x%x", Sel, Off, poh->x, poh->y, poh->cx, poh->cy, poh->ofl, (WORD)(poh->cxcy >> 16), (WORD)poh->cxcy));
            Off = *((WORD *)(&(poh->pohNext)));
            Sel = *((WORD *)(&(poh->pohNext)) + 1);
            if (poh->pohNext && (Sel != -1)) {
                Off1 = *((WORD *)(&(poh->pohNext->osb)));
                Sel1 = *((WORD *)(&(poh->pohNext->osb)) + 1);
                DISPDBG((Value, "Next :%4x:%4x %d,%d %d x %d, Flags 0x%x, cxcy 0x%x%x OSB %4x:%4x", Sel, Off, poh->pohNext->x, poh->pohNext->y, poh->pohNext->cx, poh->pohNext->cy, poh->pohNext->ofl, (WORD)(poh->pohNext->cxcy >> 16), (WORD)poh->pohNext->cxcy, Sel1, Off1));
            } else {
                DISPDBG((Value, "Next :%4x:%4x", Sel, Off));
            }
            Off = *((WORD *)(&(poh->pohPrev)));
            Sel = *((WORD *)(&(poh->pohPrev)) + 1);
            if (poh->pohPrev && (Sel != -1)) {
                Off1 = *((WORD *)(&(poh->pohPrev->osb)));
                Sel1 = *((WORD *)(&(poh->pohPrev->osb)) + 1);
                DISPDBG((Value, "Prev :%4x:%4x %d,%d %d x %d, Flags 0x%x, cxcy 0x%x%x OSB %4x:%4x", Sel, Off, poh->pohPrev->x, poh->pohPrev->y, poh->pohPrev->cx, poh->pohPrev->cy, poh->pohPrev->ofl, (WORD)(poh->pohPrev->cxcy >> 16), (WORD)poh->pohPrev->cxcy, Sel1, Off1));
            } else {
                DISPDBG((Value, "Prev :%4x:%4x", Sel, Off));
            }
            Off = *((WORD *)(&(poh->pohLeft)));
            Sel = *((WORD *)(&(poh->pohLeft)) + 1);
            if (poh->pohLeft && (Sel != -1)) {
                Off1 = *((WORD *)(&(poh->pohLeft->osb)));
                Sel1 = *((WORD *)(&(poh->pohLeft->osb)) + 1);
                DISPDBG((Value, "Left :%4x:%4x %d,%d %d x %d, Flags 0x%x, cxcy 0x%x%x OSB %4x:%4x", Sel, Off, poh->pohLeft->x, poh->pohLeft->y, poh->pohLeft->cx, poh->pohLeft->cy, poh->pohLeft->ofl, (WORD)(poh->pohLeft->cxcy >> 16), (WORD)poh->pohLeft->cxcy, Sel1, Off1));
            } else {
                DISPDBG((Value, "Left :%4x:%4x", Sel, Off));
            }
            Off = *((WORD *)(&(poh->pohRight)));
            Sel = *((WORD *)(&(poh->pohRight)) + 1);
            if (poh->pohRight && (Sel != -1)) {
                Off1 = *((WORD *)(&(poh->pohRight->osb)));
                Sel1 = *((WORD *)(&(poh->pohRight->osb)) + 1);
                DISPDBG((Value, "Right:%4x:%4x %d,%d %d x %d, Flags 0x%x, cxcy 0x%x%x OSB %4x:%4x", Sel, Off, poh->pohRight->x, poh->pohRight->y, poh->pohRight->cx, poh->pohRight->cy, poh->pohRight->ofl, (WORD)(poh->pohRight->cxcy >> 16), (WORD)poh->pohRight->cxcy, Sel1, Off1));
            } else {
                DISPDBG((Value, "Right:%4x:%4x", Sel, Off));
            }
            Off = *((WORD *)(&(poh->pohUp)));
            Sel = *((WORD *)(&(poh->pohUp)) + 1);
            if (poh->pohUp && (Sel != -1)) {
                Off1 = *((WORD *)(&(poh->pohUp->osb)));
                Sel1 = *((WORD *)(&(poh->pohUp->osb)) + 1);
                DISPDBG((Value, "Up   :%4x:%4x %d,%d %d x %d, Flags 0x%x, cxcy 0x%x%x OSB %4x:%4x", Sel, Off, poh->pohUp->x, poh->pohUp->y, poh->pohUp->cx, poh->pohUp->cy, poh->pohUp->ofl, (WORD)(poh->pohUp->cxcy >> 16), (WORD)poh->pohUp->cxcy, Sel1, Off1));
            } else {
                DISPDBG((Value, "Up   :%4x:%4x", Sel, Off));
            }
            Off = *((WORD *)(&(poh->pohDown)));
            Sel = *((WORD *)(&(poh->pohDown)) + 1);
            if (poh->pohDown && (Sel != -1)) {
                Off1 = *((WORD *)(&(poh->pohDown->osb)));
                Sel1 = *((WORD *)(&(poh->pohDown->osb)) + 1);
                DISPDBG((Value, "Down :%4x:%4x %d,%d %d x %d, Flags 0x%x, cxcy 0x%x%x OSB %4x:%4x", Sel, Off, poh->pohDown->x, poh->pohDown->y, poh->pohDown->cx, poh->pohDown->cy, poh->pohDown->ofl, (WORD)(poh->pohDown->cxcy >> 16), (WORD)poh->pohDown->cxcy, Sel1, Off1));
            } else {
                DISPDBG((Value, "Down :%4x:%4x", Sel, Off));
            }
            Off = *((WORD *)(&(poh->osb)));
            Sel = *((WORD *)(&(poh->osb)) + 1);
            DISPDBG((Value, "OSB  : %4x:%4x Base: 0x%x%x Limit: 0x%x%x", Sel, Off, (WORD)(poh->OSB_Base >> 16), (WORD)poh->OSB_Base, (WORD)(poh->OSB_Limit >> 16), (WORD)poh->OSB_Limit));

            if (poh->pohLeft == NULL) {
                DISPDBG((-1, "poh->pohLeft is invalid!"));
                DebugCrash();
            }
            if (poh->pohRight == NULL) {
                DISPDBG((-1, "poh->pohRight is invalid!"));
                DebugCrash();
            }
            if (poh->pohUp == NULL) {
                DISPDBG((-1, "poh->pohUp is invalid!"));
                DebugCrash();
            }
            if (poh->pohDown == NULL) {
                DISPDBG((-1, "poh->pohDown is invalid!"));
                DebugCrash();
            }
        }
    }
}

int	doDisplay = 0;
int	numPermanentAllocs = 0;

#define DISPLAYHEAP() displayOH()
void
displayOH()
{

	OH *h = OSB_HeapInfo.ohAvailable.pohNext;

	if (!doDisplay)
		return;

	while (h->cxcy != CXCY_SENTINEL)
		h = h->pohNext;
	h = h->pohNext;

	DISPDBG((2, "\r\nBegin Free Heap Dump: (sorted by increasing width)"));
	while (h->cxcy != CXCY_SENTINEL) {
		DISPDBG((2, "\tAt (%4d, %4d) size = [%4d, %4d]", h->x, h->y, h->cx, h->cy));
		h = h->pohNext;
	}
	DISPDBG((2, "There are %d permanent blocks too...", numPermanentAllocs));
	DISPDBG((2, "End Heap dump:\n\r"));
} // displayOH()


void DebugDIBENGINE(int Value, DIBENGINE far *dib) {
    WORD    Sel, Off;

    if ((Value <= DD_DebugLevel) && (dib != NULL)) {
        Off = *((WORD *)(&dib));
        Sel = *((WORD *)(&dib) + 1);
        DISPDBG((Value, "DIBENGINE at %x:%x", Sel, Off));
    }
}

void DebugOSB(int Value, OFFSCREEN_BITMAP far *osb) {
    WORD    Sel, Off;

    if ((Value <= DD_DebugLevel) && (osb != NULL)) {
        Off = *((WORD *)(&osb));
        Sel = *((WORD *)(&osb) + 1);
        DISPDBG((Value, "OFFSCREEN_BITMAP at %x:%x, PixelOffset 0x%x%x", Sel, Off, (WORD)(osb->OSB_PixelOffset >> 16), (WORD)osb->OSB_PixelOffset));

        DISPDBG((Value+1,"Offscreen Heap:"));
        DISPPOH((Value+1, osb->OSB_Heap));
        DISPDBG((Value+1,"Offscreen Bitmap:"));
        DISPDIBENGINE((Value+1, &(osb->DIB)));
        DISPDBG((Value+1,"Host Bitmap:"));
        DISPDIBENGINE((Value+1, &(osb->HostDIB)));
    }
}

void _cdecl DebugDIB(long lValue, OFFSCREEN_BITMAP far *osb) {
    WORD    Sel, Off;
    int     Value = (int)lValue+2;
    int     osbstr;

    if ((osb != NULL) && (osb->DIB.deType == TYPE_DIBENG)) {

        if ((osb->DIB.deFlags & VRAM) && (osb->DIB.deDriverReserved & ACCELERATED)) {
            DISPDBG((Value, "Dumping OSB DIB Structure:"));
            osbstr = 1;

            // Dont print out the screen bitmap.
            if ((osb->OSB_Heap == NULL) && (osb->OSB_PixelOffset == 0)) {
                return;
            }

        }
        else {
            osbstr = 0;
            DISPDBG((Value, "Dumping Basic DIB Structure:"));
        }

        DISPDIBENGINE((Value, &(osb->DIB)));

        if (osbstr) {
            OH far  *pohThis;          // Points to found available rectangle we'll use

            Off = *((WORD *)(&(osb->OSB_PixelOffset)));
            Sel = *((WORD *)(&(osb->OSB_PixelOffset)) + 1);
            DISPDBG((Value, "OSB_PixelOffset %x:%x", Sel, Off));
            Off = *((WORD *)(&(osb->OSB_iUniq)));
            Sel = *((WORD *)(&(osb->OSB_iUniq)) + 1);
            DISPDBG((Value, "OSB_iUniq       %x:%x", Sel, Off));
            Off = *((WORD *)(&(osb->OSB_cBlt)));
            Sel = *((WORD *)(&(osb->OSB_cBlt)) + 1);
            DISPDBG((Value, "OSB_cBlt        %x:%x", Sel, Off));
            Off = *((WORD *)(&(osb->OSB_Heap)));
            Sel = *((WORD *)(&(osb->OSB_Heap)) + 1);
            DISPDBG((Value, "OSB_Heap        %x:%x", Sel, Off));
            DISPPOH((Value, osb->OSB_Heap));

            DISPDBG((Value, "Secondary DIB Structure:"));
            DISPDIBENGINE((Value, &(osb->HostDIB)));

            // Dont do this if we are screen bitmap.
            if ((osb->OSB_Heap != NULL) || (osb->OSB_PixelOffset != 0)) {

                pohThis = OSB_HeapInfo.ohDfb.pohNext;
                while (pohThis != &OSB_HeapInfo.ohDfb)
                {
                    if (pohThis == osb->OSB_Heap) {
                        break;
                    }
                    pohThis = pohThis->pohNext;
                }
                if (pohThis != osb->OSB_Heap) {
                    DebugCrash();
                }
            }

            if ((osb->OSB_Heap == NULL) || (osb->OSB_PixelOffset == 0)) {
                DebugCrash();
            }

            {
                WORD    Off1, Sel1;
                Off = *((WORD *)(&(osb->OSB_Heap->osb)));
                Sel = *((WORD *)(&(osb->OSB_Heap->osb)) + 1);
                Off1 = *((WORD *)(&(osb)));
                Sel1 = *((WORD *)(&(osb)) + 1);
                if ((osb->OSB_Heap->ofl & OFL_AVAILABLE) || (Off != Off1) || ((Sel | 7) != (Sel1 | 7))) {
                    DISPDBG((-1, "Either osb points at 'Available' bitmap or reverse pointer invalid"));
                    DebugCrash();
                }
            }

            VALIDATEHEAPINFO("DEBUGDIB");
        }
    }
}

#else
#define VALIDATEHEAPINFO(str)
#define DISPDBG(abc)
#define DISPPOH(abc)
#define DISPOSB(abc)
#define DISPDIBENGINE(abc)
#define ASSERTDD(x, y)
#define DISPLAYHEAP()
#define RIP(x)
#endif

BOOL bValidateOSB(OH far *poh)
{
    int Selector;

    Selector = *(((WORD *)&(poh->osb)) + 1);
    DISPDBG((1, "bValidateOSB Selector 0x%x", Selector));
    DISPPOH((10, poh));

    if (Selector == 0) {
        DISPDBG((-1, "Selector Invalid"));
#if DEBUG
        DebugCrash();
#endif
        return (FALSE);
    }

    // There are some cases where a bitmap can be deallocated without us
    // being called, eg if an app crashes or doesnt free up its bitmaps
    // before it exits. Return true if it appears to be valid.
    if (GlobalHandle(Selector) == 0) {
        DISPDBG((-1, "Global Handle Invalid"));
#if DEBUG
        DebugCrash();
#endif
        return(FALSE);
    }

    if (GetSelectorLimit(Selector) < poh->OSB_Limit) {
        DISPDBG((-1, "Selector Limit not the same as it used to be"));
#if DEBUG
        DebugCrash();
#endif
        return(FALSE);
    }

    if ((poh->osb->DIB.deType != TYPE_DIBENG) || (poh->osb->HostDIB.deType != TYPE_DIBENG) 
            || (poh->osb->OSB_Heap != poh)){
        DISPDBG((-1, "Selector doesnt point to a valid bitmap!"));
#if DEBUG
        DebugCrash();
#endif
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* OH* pohNewNode
*
* Allocates a basic memory unit in which we'll pack our data structures.
*
* Since we'll have a lot of OH nodes, most of which we will be
* occasionally traversing, we do our own memory allocation scheme to
* keep them densely packed in memory.
*
* It would be the worst possible thing for the working set to simply
* call GlobalAlloc(sizeof(OH)) every time we needed a new node.  There
* would be no locality; OH nodes would get scattered throughout memory,
* and as we traversed the available list for one of our allocations,
* it would be far more likely that we would hit a hard page fault.
\**************************************************************************/

OH* pohNewNode()
{
    int         i;
    int         cOhs;
    OHALLOC far *poha;
    OH far      *poh;
    HGLOBAL     hMemory;

    if (OSB_HeapInfo.pohFreeList == NULL)
    {
        // We zero-init to initialize all the OH flags, and to help in
        // debugging (we can afford to do this since we'll be doing this
        // very infrequently):

        hMemory = MiniVDD_AllocateGlobalMemory(OH_ALLOC_SIZE);
        *((WORD *)&poha) = 0;
        *(((WORD *)&poha) + 1) = hMemory;

        if (poha == NULL)
            return(NULL);

        // Insert this OHALLOC at the begining of the OHALLOC chain:

        poha->pohaNext  = OSB_HeapInfo.pohaChain;
        poha->hMemory = hMemory;
        OSB_HeapInfo.pohaChain = poha;

        // This has a '+ 1' because OHALLOC includes an extra OH in its
        // structure declaration:

        cOhs = (OH_ALLOC_SIZE - sizeof(OHALLOC)) / sizeof(OH) + 1;

        // The big OHALLOC allocation is simply a container for a bunch of
        // OH data structures in an array.  The new OH data structures are
        // linked together and added to the OH free list:

        poh = &poha->aoh[0];
        for (i = cOhs - 1; i != 0; i--)
        {
            poh->pohNext = poh + 1;
            poh          = poh + 1;
        }

        poh->pohNext      = NULL;
        OSB_HeapInfo.pohFreeList = &poha->aoh[0];
    }

    poh = OSB_HeapInfo.pohFreeList;
    OSB_HeapInfo.pohFreeList = poh->pohNext;

    return(poh);
}

/******************************Public*Routine******************************\
* VOID vOhFreeNode
*
* Frees our basic data structure allocation unit by adding it to a free
* list.
*
\**************************************************************************/

VOID vOhFreeNode(OH far *poh)
{
    DISPDBG((1, "ohFreeNode"));

    if (poh == NULL)
        return;

#ifdef DEBUG
	memset(poh, -1, sizeof(OH));
#endif

    poh->pohNext                = OSB_HeapInfo.pohFreeList;
    OSB_HeapInfo.pohFreeList   = poh;
    poh->ofl                    = 0;

    DISPPOH((1, poh));
}

/******************************Public*Routine******************************\
* OH* pohFree
*
* Frees an off-screen heap allocation.  The free space will be combined
* with any adjacent free spaces to avoid segmentation of the 2-d heap.
*
* Note: A key idea here is that the data structure for the upper-left-
*       most node must be kept at the same physical CPU memory so that
*       adjacency links are kept correctly (when two free spaces are
*       merged, the lower or right node can be freed).
*
\**************************************************************************/

OH far *pohFree(OH far *poh)
{  
    ULONG   cxcy;
    OH far *pohBeside;
    OH far *pohNext;
    OH far *pohPrev;

    if (poh == NULL)
        return(NULL);

    DISPDBG((1, "In pohFree"));

    // Update the uniqueness to show that space has been freed, so that
    // we may decide to see if some DIBs can be moved back into off-screen
    // memory:

    OSB_HeapInfo.iHeapUniq++;

MergeLoop:

    DISPDBG((1, "Freeing %d x %d at (%d, %d)", 
            poh->cx, poh->cy, poh->x, poh->y));
    DISPPOH((1, poh));

    // Try merging with the right sibling:

    pohBeside = poh->pohRight;
    if ((pohBeside->ofl & OFL_AVAILABLE)        &&
        (pohBeside->cy      == poh->cy)         &&
        (pohBeside->pohUp   == poh->pohUp)      &&
        (pohBeside->pohDown == poh->pohDown)    &&
        (pohBeside->pohRight->pohLeft != pohBeside))
    {
        // Add the right rectangle to ours:

        poh->cx      += pohBeside->cx;
        poh->pohRight = pohBeside->pohRight;

        // Remove 'pohBeside' from the ??? list and free it:

        pohBeside->pohNext->pohPrev = pohBeside->pohPrev;
        pohBeside->pohPrev->pohNext = pohBeside->pohNext;

        vOhFreeNode(pohBeside);
        goto MergeLoop;
    }

    // Try merging with the lower sibling:

    pohBeside = poh->pohDown;
    if ((pohBeside->ofl & OFL_AVAILABLE)        &&
        (pohBeside->cx       == poh->cx)        &&
        (pohBeside->pohLeft  == poh->pohLeft)   &&
        (pohBeside->pohRight == poh->pohRight)  &&
        (pohBeside->pohDown->pohUp != pohBeside))
    {
        poh->cy     += pohBeside->cy;
        poh->pohDown = pohBeside->pohDown;

        pohBeside->pohNext->pohPrev = pohBeside->pohPrev;
        pohBeside->pohPrev->pohNext = pohBeside->pohNext;

        vOhFreeNode(pohBeside);
        goto MergeLoop;
    }

    // Try merging with the left sibling:

    pohBeside = poh->pohLeft;
    if ((pohBeside->ofl & OFL_AVAILABLE)        &&
        (pohBeside->cy       == poh->cy)        &&
        (pohBeside->pohUp    == poh->pohUp)     &&
        (pohBeside->pohDown  == poh->pohDown)   &&
        (pohBeside->pohRight == poh)            &&
        (poh->pohRight->pohLeft != poh))
    {
        // We add our rectangle to the one to the left:

        pohBeside->cx      += poh->cx;
        pohBeside->pohRight = poh->pohRight;

        // Remove 'poh' from the ??? list and free it:

        poh->pohNext->pohPrev = poh->pohPrev;
        poh->pohPrev->pohNext = poh->pohNext;

        vOhFreeNode(poh);

        poh = pohBeside;
        goto MergeLoop;
    }

    // Try merging with the upper sibling:

    pohBeside = poh->pohUp;
    if ((pohBeside->ofl & OFL_AVAILABLE)        &&
        (pohBeside->cx       == poh->cx)        &&
        (pohBeside->pohLeft  == poh->pohLeft)   &&
        (pohBeside->pohRight == poh->pohRight)  &&
        (pohBeside->pohDown  == poh)            &&
        (poh->pohDown->pohUp != poh))
    {
        pohBeside->cy      += poh->cy;
        pohBeside->pohDown  = poh->pohDown;

        poh->pohNext->pohPrev = poh->pohPrev;
        poh->pohPrev->pohNext = poh->pohNext;

        vOhFreeNode(poh);

        poh = pohBeside;
        goto MergeLoop;
    }

    DISPDBG((1, "Actually Freeing %d x %d at (%d, %d)", 
            poh->cx, poh->cy, poh->x, poh->y));

    // Remove the node from the ???list if it was in use (we wouldn't
    // want to do this for a OFL_PERMANENT node that had been freed):

    poh->pohNext->pohPrev = poh->pohPrev;
    poh->pohPrev->pohNext = poh->pohNext;

    cxcy = CXCY(poh->cx, poh->cy);

    // Insert the node into the available list:

    pohNext = OSB_HeapInfo.ohAvailable.pohNext;
    while (pohNext->cxcy < cxcy)
    {
        pohNext = pohNext->pohNext;
    }
    pohPrev = pohNext->pohPrev;

    pohPrev->pohNext    = poh;
    pohNext->pohPrev    = poh;
    poh->pohPrev        = pohPrev;
    poh->pohNext        = pohNext;

    poh->ofl            = OFL_AVAILABLE;
    poh->cxcy           = cxcy;
    poh->osb            = NULL;

    // Return the node pointer for the new and improved available rectangle:

	DISPLAYHEAP();

    return(poh);
}

/******************************Public*Routine******************************\
* OH* pohAllocate
*
* Allocates space for an off-screen rectangle.  It will attempt to find
* the smallest available free rectangle, and will allocate the block out
* of its upper-left corner.  The remaining two rectangles will be placed
* on the available free space list.
*
* If the rectangle would have been large enough to fit into off-screen
* memory, but there is not enough available free space, we will boot
* bitmaps out of off-screen and into DIBs until there is enough room.
*
\**************************************************************************/

OH far *pohAllocate(
int     cxThis,             // Width of rectangle to be allocated
int     cyThis,             // Height of rectangle to be allocated
FLOH    floh)               // Allocation flags
{
    ULONG   cxcyThis;         // Width and height search key
    OH far  *pohThis;          // Points to found available rectangle we'll use
    ULONG   cxcy;             // Temporary versions
    OH far  *pohNext;
    OH far  *pohPrev;

    int     cxRem;
    int     cyRem;

    OH far  *pohBelow;
    int     cxBelow;
    int     cyBelow;

    OH far  *pohBeside;
    int     cxBeside;
    int     cyBeside;

    DISPDBG((3, "Allocating %d x %d...", cxThis, cyThis));

    // Increase the width to get the proper alignment (thus ensuring that all
    // allocations will be properly aligned):

    cxThis = (cxThis + (HEAP_X_ALIGNMENT - 1)) & ~(HEAP_X_ALIGNMENT - 1);

    // We can't succeed if the requested rectangle is larger than the
    // largest possible available rectangle:

    if ((cxThis > OSB_HeapInfo.cxMax) || (cyThis > OSB_HeapInfo.cyMax)) {
        DISPDBG((1, "requested rect bigger than largest available (%d x %d)",
                            OSB_HeapInfo.cxMax, OSB_HeapInfo.cyMax));
        return(NULL);
    }

    // Find the first available rectangle the same size or larger than
    // the requested one:

    cxcyThis = CXCY(cxThis, cyThis);
    pohThis  = OSB_HeapInfo.ohAvailable.pohNext;
    while (pohThis->cxcy < cxcyThis)
    {
        pohThis = pohThis->pohNext;
    }

    while (pohThis->cy < cyThis)
    {
        pohThis = pohThis->pohNext;
    }

    if (pohThis->cxcy == CXCY_SENTINEL)
    {
        // There was no space large enough...

        if (floh & FLOH_ONLY_IF_ROOM) {
			DISPDBG((1, "Allocation allowed to fail"));
            return(NULL);
		}

		DISPDBG((1, "->Allocation requires some freeing on the way"));

        // We couldn't find an available rectangle that was big enough
        // to fit our request.  So throw things out of the heap until we
        // have room:

        do {
            pohThis = OSB_HeapInfo.ohDfb.pohPrev;  // Least-recently blitted

            ASSERTDD(pohThis != &OSB_HeapInfo.ohDfb, "Ran out of in-use entries");

			if ((pohThis == NULL) || (pohThis == &OSB_HeapInfo.ohDfb)) {
				DISPDBG((-1, "Very bad news on the pohThis front (0x%08X)", pohThis));
				DISPLAYHEAP();
			}
            // We can safely exit here if we have to:

            pohThis = pohValidateAndMoveOffscreenDfbToDib(pohThis);
            if (pohThis == NULL)
                return(NULL);

        } while ((pohThis->cx < cxThis) || (pohThis->cy < cyThis));

		DISPDBG((1, "Heap free list after freeing..."));
		DISPLAYHEAP();
    }

    // We've now found an available rectangle that is the same size or
    // bigger than our requested rectangle.  We're going to use the
    // upper-left corner of our found rectangle, and divide the unused
    // remainder into two rectangles which will go on the available
    // list.

    // Compute the width of the unused rectangle to the right, and the
    // height of the unused rectangle below:

    cyRem = pohThis->cy - cyThis;
    cxRem = pohThis->cx - cxThis;

    // Given finite area, we wish to find the two rectangles that are
    // most square -- i.e., the arrangement that gives two rectangles
    // with the least perimiter:

    cyBelow  = cyRem;
    cxBeside = cxRem;

	if ((cyRem <= OH_QUANTUM) || (cxRem <= OH_QUANTUM) || (pohThis->cx != (int)wScreenWidth)) {
		if (cxRem <= cyRem)
		{
			cxBelow  = cxThis + cxRem;
			cyBeside = cyThis;
		}
		else
		{
			cxBelow  = cxThis;
			cyBeside = cyThis + cyRem;
		}
	} else {
		cxBelow  = cxThis + cxRem;
		cyBeside = cyThis;
	}


    // We only make new available rectangles of the unused right and bottom
    // portions if they're greater in dimension than OH_QUANTUM (it hardly
    // makes sense to do the book-work to keep around a 2-pixel wide
    // available space, for example):

    pohBeside = NULL;
    if (cxBeside >= OH_QUANTUM)
    {
        pohBeside = pohNewNode();
        if (pohBeside == NULL)
            return(NULL);
    }

    pohBelow = NULL;
    if (cyBelow >= OH_QUANTUM)
    {
        pohBelow = pohNewNode();
        if (pohBelow == NULL)
        {
            vOhFreeNode(pohBeside);
            return(NULL);
        }

        // Insert this rectangle into the available list (which is
        // sorted on ascending cxcy):

        cxcy    = CXCY(cxBelow, cyBelow);
        pohNext = OSB_HeapInfo.ohAvailable.pohNext;
        while (pohNext->cxcy < cxcy)
        {
            pohNext = pohNext->pohNext;
        }
        pohPrev = pohNext->pohPrev;

        pohPrev->pohNext   = pohBelow;
        pohNext->pohPrev   = pohBelow;
        pohBelow->pohPrev  = pohPrev;
        pohBelow->pohNext  = pohNext;

        // Now update the adjacency information:

        pohBelow->pohLeft  = pohThis->pohLeft;
        pohBelow->pohUp    = pohThis;
        pohBelow->pohRight = pohThis->pohRight;
        pohBelow->pohDown  = pohThis->pohDown;

        // Update the rest of the new node information:

        pohBelow->cxcy     = cxcy;
        pohBelow->ofl      = OFL_AVAILABLE;
        pohBelow->x        = pohThis->x;
        pohBelow->y        = pohThis->y + cyThis;
        pohBelow->cx       = cxBelow;
        pohBelow->cy       = cyBelow;

        // Modify the current node to reflect the changes we've made:

        pohThis->cy        = cyThis;
    }

    if (cxBeside >= OH_QUANTUM)
    {
        // Insert this rectangle into the available list (which is
        // sorted on ascending cxcy):

        cxcy    = CXCY(cxBeside, cyBeside);
        pohNext = OSB_HeapInfo.ohAvailable.pohNext;
        while (pohNext->cxcy < cxcy)
        {
            pohNext = pohNext->pohNext;
        }
        pohPrev = pohNext->pohPrev;

        pohPrev->pohNext    = pohBeside;
        pohNext->pohPrev    = pohBeside;
        pohBeside->pohPrev  = pohPrev;
        pohBeside->pohNext  = pohNext;

        // Now update the adjacency information:

        pohBeside->pohUp    = pohThis->pohUp;
        pohBeside->pohLeft  = pohThis;
        pohBeside->pohDown  = pohThis->pohDown;
        pohBeside->pohRight = pohThis->pohRight;

        // Update the rest of the new node information:

        pohBeside->cxcy     = cxcy;
        pohBeside->ofl      = OFL_AVAILABLE;
        pohBeside->x        = pohThis->x + cxThis;
        pohBeside->y        = pohThis->y;
        pohBeside->cx       = cxBeside;
        pohBeside->cy       = cyBeside;

        // Modify the current node to reflect the changes we've made:

        pohThis->cx         = cxThis;
    }

    if (pohBelow != NULL)
    {
        pohThis->pohDown = pohBelow;
        if ((pohBeside != NULL) && (cyBeside == pohThis->cy))
            pohBeside->pohDown = pohBelow;
    }
    if (pohBeside != NULL)
    {
        pohThis->pohRight = pohBeside;
        if ((pohBelow != NULL) && (cxBelow == pohThis->cx))
            pohBelow->pohRight  = pohBeside;
    }

    pohThis->ofl                 = OFL_INUSE;
    pohThis->cxcy                = CXCY(pohThis->cx, pohThis->cy);
    pohThis->osb                 = NULL;    // Caller is responsible for
                                            //   setting this field

    // Remove this from the available list:

    pohThis->pohPrev->pohNext    = pohThis->pohNext;
    pohThis->pohNext->pohPrev    = pohThis->pohPrev;

    // Now insert this at the head of the DFB list:

    pohThis->pohNext                   = OSB_HeapInfo.ohDfb.pohNext;
    pohThis->pohPrev                   = &OSB_HeapInfo.ohDfb;
    OSB_HeapInfo.ohDfb.pohNext->pohPrev = pohThis;
    OSB_HeapInfo.ohDfb.pohNext          = pohThis;

    DISPDBG((1, "Allocated at (%d, %d)", pohThis->x, pohThis->y));
    DISPPOH((1, pohThis));

    return(pohThis);
}

/******************************Public*Routine******************************\
* VOID vCalculateMaxmimum
*
* Traverses the list of in-use and available rectangles to find the one
* with the maximal area.
*
\**************************************************************************/

VOID vCalculateMaximum()
{
    OH*     poh;
    OH*     pohSentinel;
    LONG    lArea;
    LONG    lMaxArea;
    int     cxMax;
    int     cyMax;
    int     i;

    lMaxArea = 0;
    cxMax    = 0;
    cyMax    = 0;

    // First time through, loop through the list of available rectangles:

    pohSentinel = &OSB_HeapInfo.ohAvailable;

    for (i = 2; i != 0; i--)
    {
        for (poh = pohSentinel->pohNext; poh != pohSentinel; poh = poh->pohNext)
        {
            // We don't have worry about this multiply overflowing
            // because we are dealing in physical screen coordinates,
            // which will probably never be more than 15 bits:

            lArea = ((LONG)poh->cx) * ((LONG)poh->cy);
            if (lArea > lMaxArea)
            {
                cxMax    = poh->cx;
                cyMax    = poh->cy;
                lMaxArea = lArea;
            }
        }

        // Second time through, loop through the list of in-use rectangles:
        pohSentinel = &OSB_HeapInfo.ohDfb;
    }

    // All that we are interested in is the dimensions of the rectangle
    // that has the largest possible available area (and remember that
    // there might not be any possible available area):

    OSB_HeapInfo.cxMax = cxMax;
    OSB_HeapInfo.cyMax = cyMax;
}

/******************************Public*Routine******************************\
* OH* pohAllocatePermanent
*
* Allocates an off-screen rectangle that can never be booted of the heap.
* It's the caller's responsibility to manage the rectangle, which includes
* what to do with the memory in DrvAssertMode when the display is changed
* to full-screen mode.
*
\**************************************************************************/

OH far *pohAllocatePermanent(
int    cx,
int    cy)
{
    OH far *poh;

    poh = pohAllocate(cx, cy, 0);
    if (poh != NULL)
    {
        // Mark the rectangle as permanent:

        poh->ofl = OFL_PERMANENT;

        // Remove the node from the most-recently blitted list:

        poh->pohPrev->pohNext = poh->pohNext;
        poh->pohNext->pohPrev = poh->pohPrev;
        poh->pohPrev = NULL;
        poh->pohNext = NULL;

        DISPPOH((20, poh));

        // Now calculate the new maximum size rectangle available in the
        // heap:

        vCalculateMaximum();
#ifdef DEBUG
		numPermanentAllocs++;
#endif
    }

    return(poh);
}

/******************************Public*Routine******************************\
* BOOL bMoveDibToOffscreenDfbIfRoom
*
* Converts the DIB DFB to an off-screen DFB, if there's room for it in
* off-screen memory.
*
* Returns: FALSE if there wasn't room, TRUE if successfully moved.
*
\**************************************************************************/

BOOL bMoveDibToOffscreenDfbIfRoom(OFFSCREEN_BITMAP far *osb)
{
    OH      far *poh;
    int Selector;

    DISPDBG((1, "bMoveDibToOffscreenDfbIfRoom called"));
    ASSERTDD(!(osb->DIB.deFlags & OFFSCREEN),
             "Can't move a bitmap off-screen when it's already off-screen");
    DISPOSB((10, osb));

    // If offscreen is disabled for any reason, we cant move anything back into
    // offscreen memory.
    if (!OffscreenEnabled ||
		(osb->DIB.deHeight <= 8 && osb->DIB.deWidth <= 8) ||
		osb->DIB.deHeight > wScreenHeight || osb->DIB.deWidth > wScreenWidth)
        return(FALSE);

    poh = pohAllocate(osb->DIB.deWidth, osb->DIB.deHeight,
                      FLOH_ONLY_IF_ROOM);
    if (poh == NULL)
    {
        // There wasn't any free room.

        DISPDBG((8, "bMoveDibToOffscreenDfbIfRoom failed. no room in heap"));
        return(FALSE);
    }

    // Update the data structures to reflect the new off-screen node:
    poh->osb   = osb;

    Selector = *(((WORD *)(&osb)) + 1);
    poh->OSB_Base = GetSelectorBase(Selector);
    poh->OSB_Limit = GetSelectorLimit(Selector);

    DISPPOH((1, poh));


    // do these here since we can't fail after this
    osb->DIB.deFlags |= VRAM | OFFSCREEN;
    osb->DIB.deDriverReserved = ACCELERATED;
    osb->OSB_Heap = poh;
    osb->OSB_PixelOffset = CalculatePixelOffsetForHeap(poh);
    osb->DIB.deDeltaScan = wScreenWidthBytes;
    osb->DIB.deBitsSelector = (WORD)pGLInfo->dwFBSel;
    osb->DIB.deBitsOffset = CalculateScreenAddrForHeap(poh);
    osb->DIB.deBeginAccess = (void far *)BeginAccess;
    osb->DIB.deEndAccess = (void far *)EndAccess;
    DISPOSB((10, osb));

    // Download the bitmap data
    BitBlt16(&(osb->DIB), 0, 0, &(osb->HostDIB), 
        0, 0, osb->DIB.deWidth, osb->DIB.deHeight, SRCCOPY, NULL, NULL);

    DISPDBG((1, "bMoveDibToOffscreenDfbIfRoom Finished"));

    return(TRUE);

}

/******************************Public*Routine******************************\
* OH* pohMoveOffscreenDfbToDib
*
* Converts the DFB from being off-screen to being a DIB.
*
* Note: The caller does NOT have to call 'pohFree' on 'poh' after making
*       this call.
*
* Returns: NULL if the function failed (due to a memory allocation).
*          Otherwise, it returns a pointer to the coalesced off-screen heap
*          node that has been made available for subsequent allocations
*          (useful when trying to free enough memory to make a new
*          allocation).
\**************************************************************************/

OH far *pohMoveOffscreenDfbToDib(OH far *poh)
{
    OFFSCREEN_BITMAP far *osb;

    DISPDBG((1, "pohMoveOffscreenDfbToDib called. Throwing out:"));
    DISPPOH((1, poh));

    osb = poh->osb;

    DISPOSB((10, osb));

    ASSERTDD((poh->x != 0) || (poh->y != 0),
            "Can't make the visible screen into a DIB");
    ASSERTDD(osb->DIB.deFlags | OFFSCREEN,
            "Can't make a DIB into even more of a DIB");

    // Upload the bitmap data
    BitBlt16(&(osb->HostDIB), 0, 0, &(osb->DIB), 0, 0,
        osb->DIB.deWidth, osb->DIB.deHeight, SRCCOPY, NULL, NULL);

#ifdef DEBUG
        // Clear the old bitmap down to black
        BitBlt16(&(osb->DIB), 0, 0, &(osb->HostDIB), 0, 0,
            osb->DIB.deWidth, osb->DIB.deHeight, BLACKNESS, NULL, NULL);
#endif

    osb->OSB_Heap = NULL;
    osb->OSB_PixelOffset = 0;
    osb->DIB.deFlags = osb->HostDIB.deFlags;
    osb->DIB.deDriverReserved = 0;
    osb->DIB.deBitsOffset = osb->HostDIB.deBitsOffset;
    osb->DIB.deBitsSelector = osb->HostDIB.deBitsSelector;
    osb->DIB.deDeltaScan = osb->HostDIB.deDeltaScan;
    osb->DIB.deBeginAccess = osb->HostDIB.deBeginAccess;
    osb->DIB.deEndAccess = osb->HostDIB.deEndAccess;

    pGLInfo->dwCurrentOffscreen = 0;

    // Don't even bother checking to see if this DIB should
    // be put back into off-screen memory until the next
    // heap 'free' occurs:

    osb->OSB_iUniq = OSB_HeapInfo.iHeapUniq;
    osb->OSB_cBlt  = 0;

    poh->osb            = NULL;

    // Remove this node from the off-screen DFB list, and free
    // it.  'pohFree' will never return NULL:

    return(pohFree(poh));
}

OH far *pohValidateAndMoveOffscreenDfbToDib(OH far *poh)
{
    DISPDBG((1, "in pohValidateAndMoveOffscreenDfbToDib"));
    if (bValidateOSB(poh)) {
        return(pohMoveOffscreenDfbToDib(poh));
    }
    else {
        DISPDBG((-1, "Application exited without freeing a bitmap!"));
        DISPPOH((1, poh));
        return(pohFree(poh));
    }
}

/******************************Public*Routine******************************\
* BOOL bMoveEverythingFromOffscreenToDibs
*
* This function is used when we're about to enter full-screen mode, which
* would wipe all our off-screen bitmaps.  GDI can ask us to draw on
* device bitmaps even when we're in full-screen mode, and we do NOT have
* the option of stalling the call until we switch out of full-screen.
* We have no choice but to move all the off-screen DFBs to DIBs.
*
* Returns TRUE if all DSURFs have been successfully moved.
*
\**************************************************************************/

BOOL bMoveAllDfbsFromOffscreenToDibs()
{
    OH far *poh;
    OH far *pohNext;
    BOOL bRet;

    DISPDBG((1, "in bMoveAllDfbsFromOffscreenToDibs"));

    bRet = TRUE;
    poh  = OSB_HeapInfo.ohDfb.pohNext;
    while (poh != &OSB_HeapInfo.ohDfb)
    {
        pohNext = poh->pohNext;

        DISPDBG((1, "moving another in bMoveAllDfbsFromOffscreenToDibs"));
        if (bValidateOSB(poh)) {
            // If something's already a DIB, we shouldn't try to make it even
            // more of a DIB:

            if (poh->osb->DIB.deFlags & OFFSCREEN)
            {
                if (!pohMoveOffscreenDfbToDib(poh))
                    bRet = FALSE;
            }
        }
        else {
            DISPDBG((-1, "Application exited without freeing a bitmap!"));
            pohFree(poh);
        }
        poh = pohNext;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID CreateOffscreenBitmap
*
*   Function called to turn a newly allocated bitmap structure into Offscreen.
* It is called from RealizeObject, with an osb in which the HostDIB section has
* been intialised.
*
\**************************************************************************/

WORD _cdecl CreateOffscreenBitmap(OFFSCREEN_BITMAP far *osb)
{
    OH far *poh;


    DISPDBG((1, "CreateOffscreenBitmap %d x %d", osb->HostDIB.deHeight, osb->HostDIB.deWidth));

    osb->HostDIB.deDriverReserved = 0;
    osb->DIB = osb->HostDIB;
    osb->OSB_Heap = NULL;

    DISPOSB((10, osb));

	// Don't accept the bitmap if OSB's are disabled, this one is ridiculously 
	// small (i.e. brush sized) or if it is higher or wider than the screen
	// NOTE: this if needs to be maintained in bMoveDibToOffscreenDfbIfRoom too
    if (!OffscreenEnabled ||
		(osb->DIB.deHeight <= 8 && osb->DIB.deWidth <= 8) ||
		osb->DIB.deHeight > wScreenHeight || osb->DIB.deWidth > wScreenWidth)
        return(FALSE);

    VALIDATEHEAPINFO("COB++");

    poh = pohAllocate(osb->DIB.deWidth, osb->DIB.deHeight, 0);
    if (poh != NULL)
    {
        int Selector;

        DISPPOH((10, poh));

        osb->DIB.deFlags |= VRAM | OFFSCREEN;
        osb->DIB.deDriverReserved = ACCELERATED;
        osb->OSB_Heap = poh;
        osb->OSB_PixelOffset = CalculatePixelOffsetForHeap(poh);
        osb->DIB.deDeltaScan = wScreenWidthBytes;
        osb->DIB.deBitsOffset = CalculateScreenAddrForHeap(poh);
        osb->DIB.deBitsSelector = (WORD)pGLInfo->dwFBSel;
        osb->DIB.deBeginAccess = (void far *)BeginAccess;
        osb->DIB.deEndAccess = (void far *)EndAccess;
        osb->OSB_iUniq = OSB_HeapInfo.iHeapUniq;
        osb->OSB_cBlt  = 0;
        poh->osb = osb;

        Selector = *(((WORD *)(&osb)) + 1);
        poh->OSB_Base = GetSelectorBase(Selector);
        poh->OSB_Limit = GetSelectorLimit(Selector);

        DISPPOH((1, poh));

#ifdef DEBUG
        // Clear the bitmap down to black
        BitBlt16(&(osb->DIB), 0, 0, &(osb->HostDIB), 0, 0,
            osb->DIB.deWidth, osb->DIB.deHeight, BLACKNESS, NULL, NULL);
#endif
    }
    else {
        DISPDBG((1, "DrvCreateDeviceBitmap failed for size %d x %d", osb->DIB.deWidth, osb->DIB.deHeight));
        VALIDATEHEAPINFO("COB--");
        return(0);
    }
    VALIDATEHEAPINFO("COB--");
    return(1);

}

/******************************Public*Routine******************************\
* VOID DeleteOffscreenBitmap
*
* Deletes a DFB.
*
\**************************************************************************/

VOID _cdecl DeleteOffscreenBitmap(OFFSCREEN_BITMAP far *osb)
{

    DISPDBG((1, "DeleteOffscreenBitmap"));
    DISPOSB((10, osb));

    if ((osb) && (osb->DIB.deFlags & OFFSCREEN))
    {
        if (!(osb->OSB_Heap->ofl & OFL_PERMANENT)) {
            OH far *poh = osb->OSB_Heap;

            VALIDATEHEAPINFO("DOB++");
            DISPPOH((20, osb->OSB_Heap));

#ifdef DEBUG
            // Clear the old bitmap down to black
            BitBlt16(&(osb->DIB), 0, 0, &(osb->HostDIB), 0, 0,
                osb->DIB.deWidth, osb->DIB.deHeight, BLACKNESS, NULL, NULL);
#endif
            osb->OSB_Heap->osb = NULL;
            osb->OSB_Heap = NULL;
            osb->OSB_PixelOffset = 0;
            osb->DIB.deFlags = osb->HostDIB.deFlags;
            osb->DIB.deDriverReserved = 0;
            pohFree(poh);
            VALIDATEHEAPINFO("DOB--");
		}
		DISPLAYHEAP();
    }
}


VOID _cdecl ConsiderMovingToOffscreen(OFFSCREEN_BITMAP far *osb)
{

    VALIDATEHEAPINFO("CMTO++");
    if (osb->OSB_iUniq == (DWORD )OSB_HeapInfo.iHeapUniq)
    {
        if (--osb->OSB_cBlt == 0)
        {
            bMoveDibToOffscreenDfbIfRoom(osb);                            
        }
    }
    else
    {
        // Some space was freed up in off-screen memory,
        // so reset the counter for this DFB:

        osb->OSB_iUniq = OSB_HeapInfo.iHeapUniq;
        osb->OSB_cBlt  = HEAP_COUNT_DOWN;
    }
    VALIDATEHEAPINFO("CMTO--");
}


/******************************Public*Routine******************************\
* VOID DisableOffscreenHeap
*
* Frees any resources allocated by the off-screen heap.
*
\**************************************************************************/

VOID _cdecl DisableOffscreenHeap()
{
    OHALLOC* poha;
    OHALLOC* pohaNext;
    HGLOBAL  hMemory;
   
	DISPDBG((1, "Disable..."));

    if (pGLInfo->DisabledByGLDD || ((pGLInfo->dwFlags & GMVF_OFFSCRNBM) == 0)) {
		DISPDBG((-1, "  disableFlags = %x, flag = %x", pGLInfo->DisabledByGLDD, (pGLInfo->dwFlags & GMVF_OFFSCRNBM)));
        return;
	}


    if ((pGLInfo->dwRenderFamily == PERMEDIA_ID) && (wScreenWidth > 1280)) {
        // 1600x1200 on a Permedia is not hardware accelerated. Therefore,
        // Offscreen bitmaps will run slower!
		DISPDBG((-1, "  Screen res too high"));
        return;
    }

	// Reinitialise the font cache - ensuring fonts are cached in memory.
	InitialiseFontCacheFromC(TEXT_CACHED_IN_MEMORY);


    // Make sure all our offscreen bitmaps get converted back to Host memory bitmaps
	DISPDBG((-1, "  actually disabling"));
    bMoveAllDfbsFromOffscreenToDibs();

    // Free up all the data
    poha = OSB_HeapInfo.pohaChain;
    while (poha != NULL)
    {
        pohaNext = poha->pohaNext;  // Grab the next pointer before it's freed
        hMemory = poha->hMemory;

        MiniVDD_FreeGlobalMemory(hMemory);

        poha = pohaNext;
    }

    OffscreenEnabled = 0;
}

/******************************Public*Routine******************************\
* BOOL EnableOffscreenHeap
*
* Initializes the off-screen heap using all available video memory,
* accounting for the portion taken by the visible screen.
*
* Input: ppdev->cxScreen
*        ppdev->cyScreen
*        ppdev->cxMemory
*        ppdev->cyMemory
*
\**************************************************************************/

BOOL _cdecl EnableOffscreenHeap(WORD cxScreen, WORD cyScreen)
{
    long cxMemory;
    long cyMemory;
	long cyFonts, cyOffscreen;
    OH far *poh, *tempPoh;

	DISPDBG((-1, "Enable:-"));

	if ((pGLInfo->dwFlags & GMVF_OFFSCRNBM) == 0) {
		// Offscreen bitmaps should not be enabled.
		DISPDBG((-1, "  Permenantly off..."));
		InitialiseFontCacheFromC(TEXT_CACHED_IN_MEMORY);
		return FALSE;
	}

    if (pGLInfo->DisabledByGLDD)
	{	// DirectDraw is active so it will use the memory that could otherwise
		// have been used for storing offscreen bitmaps.
		DISPDBG((-1, "  Disable Count = %d", pGLInfo->DisabledByGLDD));
        return(FALSE);
	}

    if ((pGLInfo->dwRenderFamily == PERMEDIA_ID) && (wScreenWidth > 1280)) {
        // 1600x1200 on a Permedia is not hardware accelerated. Therefore,
        // Offscreen bitmaps will run slower!
		DISPDBG((-1, "  Screen res too high"));
		InitialiseFontCacheFromC(TEXT_CACHED_IN_MEMORY);
        return(FALSE);
    }

    if (OffscreenEnabled) {
		DISPDBG((-1, "  Mysteriously all ready enabled??"));
        DisableOffscreenHeap();
	}

	DISPDBG((-1, "  actually enabling"));

    cxMemory = wScreenWidth;
    cyMemory = (WORD)((pGLInfo->ddFBSize - pGLInfo->dwScreenBase) / wScreenWidthBytes);

	ScreenPixelOffset = pGLInfo->dwScreenBase / (pGLInfo->dwBpp >> 3);

    DISPDBG((0, "Screen: %d x %d  Memory: %d x %d",
        cxScreen, cyScreen, cxMemory, cyMemory));

    OSB_HeapInfo.pohaChain   = NULL;
    OSB_HeapInfo.pohFreeList = NULL;

    // Initialize the available list, which will be a circular
    // doubly-linked list kept in ascending 'cxcy' order, with a
    // 'sentinel' at the end of the list:

    poh = pohNewNode();
    if (poh == NULL)
        goto ReturnFalse;

    // The first node describes the entire video memory size:

    poh->pohNext  = &OSB_HeapInfo.ohAvailable;
    poh->pohPrev  = &OSB_HeapInfo.ohAvailable;
    poh->ofl      = OFL_AVAILABLE;
    poh->x        = 0;
    poh->y        = 0;
    poh->cx       = (WORD)cxMemory;
    poh->cy       = (WORD)cyMemory;
    poh->cxcy     = CXCY(cxMemory, cyMemory);
    poh->pohLeft  = &OSB_HeapInfo.ohAvailable;
    poh->pohUp    = &OSB_HeapInfo.ohAvailable;
    poh->pohRight = &OSB_HeapInfo.ohAvailable;
    poh->pohDown  = &OSB_HeapInfo.ohAvailable;

    // The second node is our available list sentinel:

    OSB_HeapInfo.ohAvailable.pohNext = poh;
    OSB_HeapInfo.ohAvailable.pohPrev = poh;
    OSB_HeapInfo.ohAvailable.cxcy    = CXCY_SENTINEL;
    OSB_HeapInfo.ohAvailable.cx      = 0x7fff;
    OSB_HeapInfo.ohAvailable.cy      = 0x7fff;
    OSB_HeapInfo.ohAvailable.ofl     = OFL_PERMANENT;
    OSB_HeapInfo.ohDfb.pohLeft       = NULL;
    OSB_HeapInfo.ohDfb.pohUp         = NULL;
    OSB_HeapInfo.ohDfb.pohRight      = NULL;
    OSB_HeapInfo.ohDfb.pohDown       = NULL;

    // Initialize the most-recently-blitted DFB list, which will be
    // a circular doubly-linked list kept in order, with a sentinel at
    // the end.  This node is also used for the screen-surface, for its
    // offset:

    OSB_HeapInfo.ohDfb.pohNext  = &OSB_HeapInfo.ohDfb;
    OSB_HeapInfo.ohDfb.pohPrev  = &OSB_HeapInfo.ohDfb;
    OSB_HeapInfo.ohDfb.ofl      = OFL_PERMANENT;
    DISPDBG((10, "bEnableOSB_HeapInfo: screen OH is at 0x%x", &OSB_HeapInfo.ohDfb));

    // For the moment, make the max really big so that the first
    // allocation we're about to do will succeed:

    OSB_HeapInfo.cxMax = 0x7fff;
    OSB_HeapInfo.cyMax = 0x7fff;

    // Finally, reserve the upper-left corner for the screen.  We can
    // actually throw away 'poh' because we'll never need it again
    // (not even for disabling the off-screen heap since everything is
    // freed using OHALLOCs):

    poh = pohAllocatePermanent(cxScreen, cyScreen);

    ASSERTDD((poh != NULL) && (poh->x == 0) && (poh->y == 0),
             "We assumed allocator would use the upper-left corner");

	// We dont support OST on a Permedia 1 or any Glint family chip.
	// We also cannot support OST at 24bpp as it requires Block Fills,
	// which we can only do in Greys.
    if (poh && pGLInfo->dwRenderFamily == PERMEDIA_ID
		&& pGLInfo->dwRenderChipID != PERMEDIA_ID
		&& pGLInfo->dwRenderChipID != TIPERMEDIA_ID
		&& pGLInfo->dwBpp != 24) {						// At 24 bpp we dont support OST														
        HKEY    hKey;
		char	RegistryKey[SIZE_CONFIGURATIONBASE+18];

        // Reserve the Font Cache. We can decide exactly how much to allocate here.
		// We want to allocate the cache at the end of the offscreen to enhance
		// performance, so preallocate the earlier scanlines and free them
		cyFonts = (WORD) ((pGLInfo->dwFontCacheSize << 12) / pGLInfo->dwScreenWidthBytes);

		lstrcpy(RegistryKey, REGKEYROOT);
		lstrcat(RegistryKey, pGLInfo->RegistryConfigBase);
		lstrcat(RegistryKey, REGKEYDISPLAYSUFFIX);
        if (RegOpenKey(HKEY_LOCAL_MACHINE, RegistryKey, &hKey) == ERROR_SUCCESS) {
            DWORD Size = 4, CacheSize;
            if (RegQueryValueEx(hKey, "OffscreenTextCache", NULL, NULL, (char far *)&CacheSize, &Size) == ERROR_SUCCESS) {
                // Registry is in number of K. Convert to number of bytes.
		        CacheSize = CacheSize << 10;
                cyFonts = (WORD) (CacheSize / pGLInfo->dwScreenWidthBytes);
                // Double check for garbage
                if (cyFonts < 0 || cyFonts > cyMemory)
                    cyFonts = 0;
                DISPDBG((-1, "Registry setting OS FontCache to %d lines", cyFonts));
            }
            RegCloseKey(hKey);
        }

		cyOffscreen = cyMemory - cyScreen - cyFonts;
		if (cyOffscreen < cyFonts * 2) {
			cyFonts = (cyFonts + cyOffscreen) >> 2;
			cyOffscreen = cyMemory - cyScreen - cyFonts;
		}

		// Minimum of 16K offscreen Font Cache
		if ((LONG)cyFonts * pGLInfo->dwScreenWidthBytes < 0x4000) {
			cyFonts = 0;
			cyOffscreen = cyMemory - cyScreen;
		}
		else {
			tempPoh = pohAllocate((WORD)cxMemory, (WORD)cyOffscreen, 0);
			if (tempPoh) {

				poh = pohAllocatePermanent((WORD)cxMemory, (WORD)cyFonts);

				// Font cache runs from here to end of memory
				if (poh) {
#if DEBUG
                    OH far *tempPoh2;

                    if (tempPoh2 = pohAllocate((WORD)cxMemory, 1, FLOH_ONLY_IF_ROOM)) {
						RIP(("Did not use up entire offscreen!"));
					}
                    else
#endif
					    pGLInfo->dwOSFontCacheStart = CalculateScreenAddrForHeap(poh);
                }
				else {
					RIP(("Failed to allocate Font Cache"));
				}
				pohFree(tempPoh);
			}
            else {
			    RIP(("Failed to allocate Temporary poh for Font Cache"));
            }
        }

        DISPDBG((-1, "Allocated %d lines to OS FontCache (%d KBytes)", cyFonts, (cyFonts * pGLInfo->dwScreenWidthBytes) / 1024));

		InitialiseFontCacheFromC(TEXT_CACHED_IN_OFFSCREEN);
	}
	else {
		InitialiseFontCacheFromC(TEXT_CACHED_IN_MEMORY);
	}


    DISPDBG((5, "Validating heap information:"));

    VALIDATEHEAPINFO("EOH--");

    DISPDBG((5, "Passed bEnableOSB_HeapInfo"));

    OffscreenEnabled = 1;

	DISPLAYHEAP();

    if (poh != NULL)
        return(TRUE);

    DisableOffscreenHeap();

ReturnFalse:

    DISPDBG((10, "Failed bEnableOSB_HeapInfo"));

    return(FALSE);
}


