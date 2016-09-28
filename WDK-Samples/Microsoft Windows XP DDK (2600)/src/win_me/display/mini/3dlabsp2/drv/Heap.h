;/******************************Module*Header**********************************\
; *
; *                           **********************************
; *                           * Permedia 2: SAMPLE CODE        *
; *                           **********************************
; *
; * Module Name: heap.h
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
/////////////////////////////////////////////////////////////////////////
// Heap stuff


typedef enum {
    OFL_INUSE       = 1,    // The device bitmap is no longer located in
                            //   off-screen memory; it's been converted to
                            //   a DIB
    OFL_AVAILABLE   = 2,    // Space is in-use
    OFL_PERMANENT   = 4     // Space is available
} OHFLAGS;                  // Space is permanently allocated; never free it

typedef struct _OH OH;

typedef struct _OFFSCREEN_BITMAP
{
    DIBENGINE   DIB;                // DIB Engine data structure
    DWORD       OSB_PixelOffset;
    OH far     *OSB_Heap;
    DWORD       OSB_iUniq;
    DWORD       OSB_cBlt;
    DIBENGINE   HostDIB;            // DIB Engine data structure
} OFFSCREEN_BITMAP;                          /* dsurf, pdsurf */

typedef struct _OH
{
    OHFLAGS     ofl;           // OH_ flags
    int         x;             // x-coordinate of left edge of allocation
    int         y;             // y-coordinate of top edge of allocation
    int         cx;            // Width in pixels of allocation
    int         cy;            // Height in pixels of allocation
    OH far      *pohNext;      // When OFL_AVAILABLE, points to the next free node,
                                //   in ascending cxcy value.  This is kept as a
                                //   circular doubly-linked list with a sentinel
                                //   at the end.
                                // When OFL_INUSE, points to the next most recently
                                //   blitted allocation.  This is kept as a circular
                                //   doubly-linked list so that the list can be
                                //   quickly be updated on every blt.
    OH far      *pohPrev;      // Opposite of 'pohNext'
    ULONG       cxcy;          // Width and height in a dword for searching
    OH far      *pohLeft;      // Adjacent allocation when in-use or available
    OH far      *pohUp;
    OH far      *pohRight;
    OH far      *pohDown;
    OFFSCREEN_BITMAP  far   *osb;       // Points to our DSURF structure
    ULONG       OSB_Base;
    ULONG       OSB_Limit;
};                             /* oh, poh */

// This is the smallest structure used for memory allocations:

typedef struct _OHALLOC OHALLOC;
typedef struct _OHALLOC
{
    OHALLOC far *pohaNext;
    HGLOBAL     hMemory;
    OH          aoh[1];
} OHALLOC;                      /* oha, poha */

typedef struct _HEAP
{
    int      cxMax;         // Largest possible free space by area
    int      cyMax;
    OH       ohAvailable;   // Head of available list (pohNext points to
                            //   smallest available rectangle, pohPrev
                            //   points to largest available rectangle,
                            //   sorted by cxcy)
    OH       ohDfb;         // Head of the list of all DFBs currently in
                            //   offscreen memory that are eligible to be
                            //   tossed out of the heap (pohNext points to
                            //   the most recently blitted; pohPrev points
                            //   to least recently blitted)
    OH far      *pohFreeList;   // List of OH node data structures available
    OHALLOC far *pohaChain;     // Chain of allocations
    LONG        iHeapUniq;
} HEAP;                         /* heap, pheap */

// Off-screen heap allocations have a 4 pixel 'x' alignment:

#define HEAP_X_ALIGNMENT    4

// Number of consider calls necessary before we'll consider putting a DIB DFB back
// into off-screen memory:

#define HEAP_COUNT_DOWN     4

// Flags for 'pohAllocate':

typedef enum {
    FLOH_ONLY_IF_ROOM       = 0x00000001,   // Don't kick stuff out of off-
                                            //   screen memory to make room
} FLOH;
