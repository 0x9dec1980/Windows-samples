/******************************Module*Header*******************************\
* Module Name: driver.h
*
* Contains prototypes for the display driver.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

//////////////////////////////////////////////////////////////////////
// Put all the conditional-compile constants here.  There had better
// not be many!

// Both fast and slow patterns are enabled by default:

#define FASTFILL_PATTERNS       1
#define SLOWFILL_PATTERNS       1

// Multi-board support can be enabled by setting this to 1:

#define MULTI_BOARDS            0

// This is the maximum number of boards we'll support in a single
// virtual driver:

#if MULTI_BOARDS
    #define MAX_BOARDS          16
    #define IBOARD(ppdev)       ((ppdev)->iBoard)

    // We disable slow patterns for multi-boards, because I'm too lazy to
    // change all the realization checks to account for possibly NULL
    // prb->apbe[] values:

    #undef SLOWFILL_PATTERNS
    #define SLOWFILL_PATTERNS   0
#else
    #define MAX_BOARDS          1
    #define IBOARD(ppdev)       0
#endif

// Useful for visualizing the 2-d heap:

#define DEBUG_HEAP              0

//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#define DLL_NAME                L"s3"       // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "S3: "      // All debug output is prefixed
                                            //   by this string

#define CLIP_LIMIT          50  // We'll be taking 800 bytes of stack space

#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

#define TMP_BUFFER_SIZE     8192  // Size in bytes of 'pvTmpBuffer'.  Has to
                                  //   be at least enough to store an entire
                                  //   scan line (i.e., 6400 for 1600x1200x32).

#if defined(_ALPHA_)
    #define XFER_BUFFERS    16  // Defines the maximum number of write buffers
                                //   possible on any Alpha.  Must be a power
#else                           //   of two.
    #define XFER_BUFFERS    1   // On non-alpha systems, we don't have to
                                //   worry about the chip caching our bus
#endif                          //   writes.

#define XFER_MASK           (XFER_BUFFERS - 1)

typedef struct _CLIPENUM {
    LONG    c;
    RECTL   arcl[CLIP_LIMIT];   // Space for enumerating complex clipping

} CLIPENUM;                         /* ce, pce */

typedef struct _PDEV PDEV;      // Handy forward declaration

VOID vSetClipping(PDEV*, RECTL*);
VOID vResetClipping(PDEV*);

//////////////////////////////////////////////////////////////////////
// Text stuff

typedef struct _XLATECOLORS {       // Specifies foreground and background
ULONG   iBackColor;                 //   colours for faking a 1bpp XLATEOBJ
ULONG   iForeColor;
} XLATECOLORS;                          /* xlc, pxlc */

BOOL bEnableText(PDEV*);
VOID vDisableText(PDEV*);
VOID vAssertModeText(PDEV*, BOOL);

VOID vFastText(GLYPHPOS*, ULONG, BYTE*, ULONG, ULONG, RECTL*, RECTL*,
               FLONG, RECTL*, RECTL*);
VOID vClearMemDword(ULONG*, ULONG);

//////////////////////////////////////////////////////////////////////
// Dither stuff

// Describes a single colour tetrahedron vertex for dithering:

typedef struct _VERTEX_DATA {
    ULONG ulCount;              // Number of pixels in this vertex
    ULONG ulVertex;             // Vertex number
} VERTEX_DATA;                      /* vd, pv */

VERTEX_DATA*    vComputeSubspaces(ULONG, VERTEX_DATA*);
VOID            vDitherColor(ULONG*, VERTEX_DATA*, VERTEX_DATA*, ULONG);

//////////////////////////////////////////////////////////////////////
// Brush stuff

// 'Slow' brushes are used when we don't have hardware pattern capability,
// and we have to handle patterns using screen-to-screen blts:

#define SLOW_BRUSH_CACHE_DIM    3   // Controls the number of brushes cached
                                    //   in off-screen memory, when we don't
                                    //   have the S3 hardware pattern support.
                                    //   We allocate 3 x 3 brushes, so we can
                                    //   cache a total of 9 brushes:
#define SLOW_BRUSH_COUNT        (SLOW_BRUSH_CACHE_DIM * SLOW_BRUSH_CACHE_DIM)
#define SLOW_BRUSH_DIMENSION    64  // After alignment is taken care of,
                                    //   every off-screen brush cache entry
                                    //   will be 64 pels in both dimensions
#define SLOW_BRUSH_ALLOCATION   (SLOW_BRUSH_DIMENSION + 8)
                                    // Actually allocate 72x72 pels for each
                                    //   pattern, using the 8 extra for brush
                                    //   alignment

// 'Fast' brushes are used when we have hardware pattern capability:

#define FAST_BRUSH_COUNT        16  // Total number of non-hardware brushes
                                    //   cached off-screen
#define FAST_BRUSH_DIMENSION    8   // Every off-screen brush cache entry
                                    //   is 8 pels in both dimensions
#define FAST_BRUSH_ALLOCATION   8   // We have to align ourselves, so this is
                                    //   the dimension of each brush allocation

// Common to both implementations:

#define RBRUSH_2COLOR           1   // For RBRUSH flags

#define TOTAL_BRUSH_COUNT       max(FAST_BRUSH_COUNT, SLOW_BRUSH_COUNT)
                                    // This is the maximum number of brushes
                                    //   we can possibly have cached off-screen
#define TOTAL_BRUSH_SIZE        64  // We'll only ever handle 8x8 patterns,
                                    //   and this is the number of pels

typedef struct _BRUSHENTRY BRUSHENTRY;

// NOTE: Changes to the RBRUSH or BRUSHENTRY structures must be reflected
//       in strucs.inc!

typedef struct _RBRUSH {
    FLONG       fl;             // Type flags
    BOOL        bTransparent;   // TRUE if brush was realized for a transparent
                                //   blt (meaning colours are white and black),
                                //   FALSE if not (meaning it's already been
                                //   colour-expanded to the correct colours).
                                //   Value is undefined if the brush isn't
                                //   2 colour.
    ULONG       ulForeColor;    // Foreground colour if 1bpp
    ULONG       ulBackColor;    // Background colour if 1bpp
    POINTL      ptlBrushOrg;    // Brush origin of cached pattern.  Initial
                                //   value should be -1
    BRUSHENTRY* apbe[MAX_BOARDS];// Points to brush-entry that keeps track
                                //   of the cached off-screen brush bits
    ULONG       aulPattern[1];  // Open-ended array for keeping copy of the
      // Don't put anything     //   actual pattern bits in case the brush
      //   after here, or       //   origin changes, or someone else steals
      //   you'll be sorry!     //   our brush entry (declared as a ULONG
                                //   for proper dword alignment)

} RBRUSH;                           /* rb, prb */

typedef struct _BRUSHENTRY {
    RBRUSH*     prbVerify;      // We never dereference this pointer to
                                //   find a brush realization; it is only
                                //   ever used in a compare to verify
                                //   that for a given realized brush, our
                                //   off-screen brush entry is still valid.
    LONG        x;              // x-position of cached pattern
    LONG        y;              // y-position of cached pattern

} BRUSHENTRY;                       /* be, pbe */

typedef union _RBRUSH_COLOR {
    RBRUSH*     prb;
    ULONG       iSolidColor;
} RBRUSH_COLOR;                     /* rbc, prbc */

BOOL bEnableBrushCache(PDEV*);
VOID vDisableBrushCache(PDEV*);
VOID vAssertModeBrushCache(PDEV*, BOOL);

//////////////////////////////////////////////////////////////////////
// Stretch stuff

typedef struct _STR_BLT {
    PDEV*   ppdev;
    PBYTE   pjSrcScan;
    LONG    lDeltaSrc;
    LONG    XSrcStart;
    PBYTE   pjDstScan;
    LONG    lDeltaDst;
    LONG    XDstStart;
    LONG    XDstEnd;
    LONG    YDstStart;
    LONG    YDstCount;
    ULONG   ulXDstToSrcIntCeil;
    ULONG   ulXDstToSrcFracCeil;
    ULONG   ulYDstToSrcIntCeil;
    ULONG   ulYDstToSrcFracCeil;
    ULONG   ulXFracAccumulator;
    ULONG   ulYFracAccumulator;
} STR_BLT;

typedef VOID (*PFN_DIRSTRETCH)(STR_BLT*);

VOID vDirectStretch8Narrow(STR_BLT*);
VOID vDirectStretch8(STR_BLT*);
VOID vDirectStretch16(STR_BLT*);
VOID vDirectStretch32(STR_BLT*);

/////////////////////////////////////////////////////////////////////////
// Heap stuff

typedef enum {
    OFL_INUSE       = 1,    // The device bitmap is no longer located in
                            //   off-screen memory; it's been converted to
                            //   a DIB
    OFL_AVAILABLE   = 2,    // Space is in-use
    OFL_PERMANENT   = 4     // Space is available
} OHFLAGS;                  // Space is permanently allocated; never free it

typedef struct _DSURF DSURF;
typedef struct _OH OH;
typedef struct _OH
{
    OHFLAGS  ofl;           // OH_ flags
    LONG     x;             // x-coordinate of left edge of allocation
    LONG     y;             // y-coordinate of top edge of allocation
    LONG     cx;            // Width in pixels of allocation
    LONG     cy;            // Height in pixels of allocation
    OH*      pohNext;       // When OFL_AVAILABLE, points to the next free node,
                            //   in ascending cxcy value.  This is kept as a
                            //   circular doubly-linked list with a sentinel
                            //   at the end.
                            // When OFL_INUSE, points to the next most recently
                            //   blitted allocation.  This is kept as a circular
                            //   doubly-linked list so that the list can be
                            //   quickly be updated on every blt.
    OH*      pohPrev;       // Opposite of 'pohNext'
    ULONG    cxcy;          // Width and height in a dword for searching
    OH*      pohLeft;       // Adjacent allocation when in-use or available
    OH*      pohUp;
    OH*      pohRight;
    OH*      pohDown;
    DSURF*   pdsurf;        // Points to our DSURF structure
};                              /* oh, poh */

// This is the smallest structure used for memory allocations:

typedef struct _OHALLOC OHALLOC;
typedef struct _OHALLOC
{
    OHALLOC* pohaNext;
    OH       aoh[1];
} OHALLOC;                      /* oha, poha */

typedef struct _HEAP
{
    LONG     cxMax;         // Largest possible free space by area
    LONG     cyMax;
    OH       ohAvailable;   // Head of available list (pohNext points to
                            //   smallest available rectangle, pohPrev
                            //   points to largest available rectangle,
                            //   sorted by cxcy)
    OH       ohDfb;         // Head of the list of all DFBs currently in
                            //   offscreen memory that are eligible to be
                            //   tossed out of the heap (pohNext points to
                            //   the most recently blitted; pohPrev points
                            //   to least recently blitted)
    OH*      pohFreeList;   // List of OH node data structures available
    OHALLOC* pohaChain;     // Chain of allocations
} HEAP;                         /* heap, pheap */

typedef enum {
    DT_SCREEN,              // Surface is kept in screen memory
    DT_DIB                  // Surface is kept as a DIB
} DSURFTYPE;                    /* dt, pdt */

typedef struct _DSURF
{
    DSURFTYPE dt;           // DSURF status (whether off-screen or in a DIB)
    SIZEL     sizl;         // Size of the original bitmap (could be smaller
                            //   than poh->sizl)
    PDEV*     ppdev;        // Need this for deleting the bitmap
    union {
        OH*         poh;    // If DT_SCREEN, points to off-screen heap node
        SURFOBJ*    pso;    // If DT_DIB, points to locked GDI surface
    };

    // The following are used for DT_DIB only...

    ULONG     cBlt;         // Counts down the number of blts necessary at
                            //   the current uniqueness before we'll consider
                            //   putting the DIB back into off-screen memory
    ULONG     iUniq;        // Tells us whether there have been any heap
                            //   'free's since the last time we looked at
                            //   this DIB

} DSURF;                          /* dsurf, pdsurf */

// GDI expects dword alignment for any bitmaps on which it is expected
// to draw.  Since we occasionally ask GDI to draw directly on our off-
// screen bitmaps, this means that any off-screen bitmaps must be dword
// aligned in the frame buffer.  We enforce this merely by ensuring that
// all off-screen bitmaps are four-pel aligned (we may waste a couple of
// pixels at the higher colour depths):

#define HEAP_X_ALIGNMENT    4

// Number of blts necessary before we'll consider putting a DIB DFB back
// into off-screen memory:

#define HEAP_COUNT_DOWN     6

// Flags for 'pohAllocate':

typedef enum {
    FLOH_ONLY_IF_ROOM       = 0x00000001,   // Don't kick stuff out of off-
                                            //   screen memory to make room
} FLOH;

BOOL bEnableOffscreenHeap(PDEV*);
VOID vDisableOffscreenHeap(PDEV*);
VOID vAssertModeOffscreenHeap(PDEV*, BOOL);

OH*  pohMoveOffscreenDfbToDib(PDEV*, OH*);
BOOL bMoveDibToOffscreenDfbIfRoom(PDEV*, DSURF*);
OH*  pohAllocatePermanent(PDEV*, LONG, LONG);
BOOL bMoveAllDfbsFromOffscreenToDibs(PDEV* ppdev);

/////////////////////////////////////////////////////////////////////////
// Bank manager stuff

#define BANK_DATA_SIZE  80      // Number of bytes to allocate for the
                                //   miniport down-loaded bank code working
                                //   space

typedef struct _BANK
{
    // Private data:

    RECTL    rclDraw;           // Rectangle describing the remaining undrawn
                                //   portion of the drawing operation
    RECTL    rclSaveBounds;     // Saved from original CLIPOBJ for restoration
    BYTE     iSaveDComplexity;  // Saved from original CLIPOBJ for restoration
    BYTE     fjSaveOptions;     // Saved from original CLIPOBJ for restoration
    LONG     iBank;             // Current bank
    PDEV*    ppdev;             // Saved copy

    // Public data:

    SURFOBJ* pso;               // Surface wrapped around the bank.  Has to be
                                //   passed as the surface in any banked call-
                                //   back.
    CLIPOBJ* pco;               // Clip object that is the intersection of the
                                //   original clip object with the bounds of the
                                //   current bank.  Has to be passed as the clip
                                //   object in any banked call-back.

} BANK;                         /* bnk, pbnk */

// Note: BANK_MODE is duplicated in i386\strucs.inc!

typedef enum {
    BANK_OFF = 0,       // We've finished using the memory aperture
    BANK_ON,            // We're about to use the memory aperture
    BANK_ON_NO_WAIT,    // We're about to use the memory aperture, and are
                        //   doing our own hardware synchronization
    BANK_DISABLE,       // We're about to enter full-screen; shut down banking
    BANK_ENABLE,        // We've exited full-screen; re-enable banking

} BANK_MODE;                    /* bankm, pbankm */

typedef struct _BANKDATA {

// Common to both old and new bank schemes:

ULONG   ulGp_stat_cmd;                   // Port number of status register
ULONG   ulRegisterLock_35;               // Default for index 35

// Only applies to new bank schemes:

ULONG   ulSystemConfiguration_40;        // Default for index 40
ULONG   ulExtendedSystemControl2_51;     // Default for index 51
ULONG   ulExtendedMemoryControl_53;      // Default for index 53
ULONG   ulLinearAddressWindowControl_58; // Default for index 58
ULONG   ulExtendedSystemControl4_6a;     // Default for index 6a

ULONG   ulEnableMemoryMappedIo;          // Bit mask to enable MM IO

} BANKDATA;                      /* bd, pbd */

typedef VOID (FNBANKMAP)(BANKDATA*, LONG);
typedef VOID (FNBANKSELECTMODE)(BANKDATA*, BANK_MODE);
typedef VOID (FNBANKINITIALIZE)(BANKDATA*, BOOL);
typedef BOOL (FNBANKCOMPUTE)(PDEV*, RECTL*, RECTL*, LONG*, LONG*);

VOID vBankStart(PDEV*, RECTL*, CLIPOBJ*, BANK*);
BOOL bBankEnum(BANK*);

FNBANKCOMPUTE bBankComputeNonPower2;
FNBANKCOMPUTE bBankComputePower2;

BOOL bEnableBanking(PDEV*);
VOID vDisableBanking(PDEV*);
VOID vAssertModeBanking(PDEV*, BOOL);

/////////////////////////////////////////////////////////////////////////
// Pointer stuff

#define POINTER_DATA_SIZE       40      // Number of bytes to allocate for the
                                        //   miniport down-loaded pointer code
                                        //   working space
#define HW_INVISIBLE_OFFSET     2       // Offset from 'ppdev->yPointerBuffer'
                                        //   to the invisible pointer
#define HW_POINTER_DIMENSION    64      // Maximum dimension of default
                                        //   (built-in) hardware pointer
#define HW_POINTER_HIDE         63      // Hardware pointer start pixel
                                        //   position used to hide the pointer
#define HW_POINTER_TOTAL_SIZE   1024    // Total size in bytes required
                                        //   to define the hardware pointer
                                        //   (must be a power of 2 for
                                        //   allocating space for the shape)

typedef VOID (FNSHOWPOINTER)(VOID*, BOOL);
typedef VOID (FNMOVEPOINTER)(VOID*, LONG, LONG);
typedef BOOL (FNSETPOINTERSHAPE)(VOID*, LONG, LONG, LONG, LONG, LONG, LONG,
                                 BYTE*);
typedef VOID (FNENABLEPOINTER)(VOID*, BOOL);

BOOL bEnablePointer(PDEV*);
VOID vDisablePointer(PDEV*);
VOID vAssertModePointer(PDEV*, BOOL);

/////////////////////////////////////////////////////////////////////////
// Palette stuff

BOOL bEnablePalette(PDEV*);
VOID vDisablePalette(PDEV*);
VOID vAssertModePalette(PDEV*, BOOL);

BOOL bInitializePalette(PDEV*, DEVINFO*);
VOID vUninitializePalette(PDEV*);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

/////////////////////////////////////////////////////////////////////////
// 3D DDI Rendering Extension stuff

BOOL bEnableRx(PDEV*);
VOID vDisableRx(PDEV*);
VOID vAssertModeRx(PDEV*, BOOL);
ULONG RxEscape(SURFOBJ*, ULONG, VOID*, ULONG, VOID*);

/////////////////////////////////////////////////////////////////////////
// DCI stuff



#define ROUND_UP_TO_64K(x)  (((ULONG)(x) + 0x10000 - 1) & ~(0x10000 - 1))

typedef struct _DCISURF
{
    // This union must appear at the beginning of the structure.  It
    // defines the public fields returned to GDI.

    union {
        DCISURFACEINFO SurfaceInfo;
        DCIOFFSCREEN   OffscreenInfo;
        DCIOVERLAY     OverlayInfo;
    };

    // The following are private fields we use to maintain the the
    // DCI surface.

    PDEV* ppdev;                    // To find our PDEV
    ULONG Offset;                   // Location of surface in memory.
    ULONG Size;                     // Size of surface in memory.
                                    //   This information could be changed
                                    //   to rectangles.
} DCISURF, *PDCISURF;

LONG DCICreatePrimarySurface(PDEV*, ULONG, VOID*, ULONG, VOID*);

//////////////////////////////////////////////////////////////////////
// Low-level blt function prototypes

typedef VOID (FNFILL)(PDEV*, LONG, RECTL*, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
typedef VOID (FNXFER)(PDEV*, LONG, RECTL*, ULONG, ULONG, SURFOBJ*, POINTL*,
                      RECTL*, XLATEOBJ*);
typedef VOID (FNCOPY)(PDEV*, LONG, RECTL*, ULONG, POINTL*, RECTL*);
typedef VOID (FNFASTLINE)(PDEV*, PATHOBJ*, RECTL*, PFNSTRIP*, LONG, ULONG,
                          ULONG);
typedef BOOL (FNFASTFILL)(PDEV*, LONG, POINTFIX*, ULONG, ULONG, ULONG, RBRUSH*);
typedef VOID (FNFASTPATREALIZE)(PDEV*, RBRUSH*, POINTL*, BOOL);
typedef VOID (FNIMAGETRANSFER)(PDEV*, BYTE*, LONG, LONG, LONG, ULONG);
typedef VOID (FNTEXTOUT)(PDEV*, STROBJ*, CLIPOBJ*, RECTL*, BRUSHOBJ*,
                         BRUSHOBJ*);

FNFILL              vIoFillPatFast;
FNFILL              vIoFillPatSlow;
FNFILL              vIoFillSolid;
FNXFER              vIoXfer1bpp;
FNXFER              vIoXfer4bpp;
FNXFER              vIoXferNative;
FNXFER              vXferNativeSrccopy;
FNCOPY              vIoCopyBlt;
FNFASTLINE          vIoFastLine;
FNFASTFILL          bIoFastFill;
FNFASTPATREALIZE    vIoFastPatRealize;
FNIMAGETRANSFER     vIoImageTransferIo16;
FNIMAGETRANSFER     vIoImageTransferMm16;
FNTEXTOUT           vIoTextOut;

FNFILL              vMmFillPatFast;
FNFILL              vMmFillPatSlow;
FNFILL              vMmFillSolid;
FNXFER              vMmXfer1bpp;
FNXFER              vMmXfer4bpp;
FNXFER              vMmXferNative;
FNCOPY              vMmCopyBlt;
FNFASTLINE          vMmFastLine;
FNFASTFILL          bMmFastFill;
FNFASTPATREALIZE    vMmFastPatRealize;
FNIMAGETRANSFER     vMmImageTransferMm16;
FNIMAGETRANSFER     vMmImageTransferMm32;
FNTEXTOUT           vMmTextOut;

FNTEXTOUT           vNmTextOut;

VOID vPutBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vGetBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vIoSlowPatRealize(PDEV*, RBRUSH*, BOOL);

////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to us from the S3 miniport.  They
// come from the 'DriverSpecificAttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to us via an 'VIDEO_QUERY_AVAIL_MODES' or 'VIDEO_QUERY_CURRENT_MODE'
// IOCTL.
//
// NOTE: These definitions must match those in the S3 miniport's 's3.h'!

typedef enum {
    CAPS_NEW_MMIO           = 0x00000100,   // Can use 'new memory-mapped
                                            //   I/O' scheme introduced with
                                            //   868/968
    CAPS_POLYGON            = 0x00000200,   // Can do polygons in hardware
    CAPS_24BPP              = 0x00000400,   // Has 24bpp capability
    CAPS_BAD_24BPP          = 0x00000800,   // Has 868/968 early rev chip bugs
                                            //   when at 24bpp
    CAPS_BAD_PACKED_EXPANDS = 0x00001000,   // Has 868/968 early rev chip bug,
                                            //   where packed 'new 32-bit
                                            //   transfers' don't work at 8bpp
    CAPS_PIXEL_FORMATTER    = 0x00002000,   // Can do colour space conversions,
                                            //   and one-dimensional hardware
                                            //   stretches
    CAPS_BAD_DWORD_READS    = 0x00004000,   // Dword or word reads from the
                                            //   frame buffer will occasionally
                                            //   return an incorrect result
    CAPS_NO_DIRECT_ACCESS   = 0x00008000,   // Frame buffer can't be directly
                                            //   accessed by GDI or DCI --
                                            //   because dword or word reads
                                            //   would crash system, or Alpha
                                            //   is running in sparse space

    CAPS_HW_PATTERNS        = 0x00010000,   // 8x8 hardware pattern support
    CAPS_MM_TRANSFER        = 0x00020000,   // Memory-mapped image transfers
    CAPS_MM_IO              = 0x00040000,   // Memory-mapped I/O
    CAPS_MM_32BIT_TRANSFER  = 0x00080000,   // Can do 32bit bus size transfers
    CAPS_16_ENTRY_FIFO      = 0x00100000,   // At least 16 entries in FIFO
    CAPS_SW_POINTER         = 0x00200000,   // No hardware pointer; use software
                                            //   simulation
    CAPS_BT485_POINTER      = 0x00400000,   // Use Brooktree 485 pointer
    CAPS_TI025_POINTER      = 0x00800000,   // Use TI TVP3020/3025 pointer
    CAPS_SCALE_POINTER      = 0x01000000,   // Set if the S3 hardware pointer
                                            //   x position has to be scaled by
                                            //   two
    CAPS_SPARSE_SPACE       = 0x02000000,   // Frame buffer is mapped in sparse
                                            //   space on the Alpha
    CAPS_NEW_BANK_CONTROL   = 0x04000000,   // Set if 801/805/928 style banking
    CAPS_NEWER_BANK_CONTROL = 0x08000000,   // Set if 864/964 style banking
    CAPS_RE_REALIZE_PATTERN = 0x10000000,   // Set if we have to work around the
                                            //   864/964 hardware pattern bug
    CAPS_SLOW_MONO_EXPANDS  = 0x20000000,   // Set if we have to slow down
                                            //   monochrome expansions
    CAPS_MM_GLYPH_EXPAND    = 0x40000000,   // Use memory-mapped I/O glyph-
                                            //   expand method of drawing text
                                            //   for 928/864/964
} CAPS;

#define CAPS_DAC_POINTER    (CAPS_BT485_POINTER | CAPS_TI025_POINTER)

// DIRECT_ACCESS(ppdev) returns TRUE if GDI and DCI can directly access
// the frame buffer.  It returns FALSE if there are hardware bugs
// when reading words or dwords from the frame buffer that cause non-x86
// systems to crash.  It will also return FALSE is the Alpha frame buffer
// is mapped in using 'sparse space'.

#if defined(i386)
    #define DIRECT_ACCESS(ppdev)    1
#else
    #define DIRECT_ACCESS(ppdev)    \
        (!(ppdev->flCaps & (CAPS_NO_DIRECT_ACCESS | CAPS_SPARSE_SPACE)))
#endif

// DENSE(ppdev) returns TRUE if the normal 'dense space' mapping of the
// frame buffer is being used.  It returns FALSE only on the Alpha when
// the frame buffer is mapped in using 'sparse space,' meaning that all
// reads and writes to and from the frame buffer must be done through the
// funky 'ioaccess.h' macros.

#if defined(_ALPHA_)
    #define DENSE(ppdev)        (!(ppdev->flCaps & CAPS_SPARSE_SPACE))
#else
    #define DENSE(ppdev)        1
#endif

////////////////////////////////////////////////////////////////////////
// Status flags

typedef enum {
    STAT_GLYPH_CACHE        = 0x0001,   // Glyph cache successfully allocated
    STAT_BRUSH_CACHE        = 0x0002,   // Brush cache successfully allocated
} STATUS;

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    LONG        xOffset;                // Pixel offset from (0, 0) to current
    LONG        yOffset;                //   DFB located in off-screen memory
    BYTE*       pjMmBase;               // Start of memory mapped I/O
    BYTE*       pjScreen;               // Points to base screen address
    LONG        lDelta;                 // Distance from one scan to the next.
    LONG        cPelSize;               // 0 if 8bpp, 1 if 16bpp, 2 if 32bpp
    ULONG       iBitmapFormat;          // BMF_8BPP or BMF_16BPP or BMF_32BPP
                                        //   (our current colour depth)
    LONG        iBoard;                 // Logical multi-board identifier
                                        //   (zero by default)

    // Rendering extensions colour information.

    UCHAR       rDepth;                 // Number of red bits
    UCHAR       gDepth;                 // Number of green bits
    UCHAR       bDepth;                 // Number of blue bits
    UCHAR       aDepth;                 // Number of alpha bits
    UCHAR       rBitShift;              // Left bit-shift for red component
    UCHAR       gBitShift;              // Left bit-shift for green component
    UCHAR       bBitShift;              // Left bit-shift for blue component
    UCHAR       aBitShift;              // Left bit-shift for alpha component

    // Enhanced mode register addresses.

    ULONG       ioCur_y;
    ULONG       ioCur_x;
    ULONG       ioDesty_axstp;
    ULONG       ioDestx_diastp;
    ULONG       ioErr_term;
    ULONG       ioMaj_axis_pcnt;
    ULONG       ioGp_stat_cmd;
    ULONG       ioShort_stroke;
    ULONG       ioBkgd_color;
    ULONG       ioFrgd_color;
    ULONG       ioWrt_mask;
    ULONG       ioRd_mask;
    ULONG       ioColor_cmp;
    ULONG       ioBkgd_mix;
    ULONG       ioFrgd_mix;
    ULONG       ioMulti_function;
    ULONG       ioPix_trans;

    // Important data for accessing the frame buffer.

    VOID*               pvBankData;         // Points to aulBankData[0]
    FNBANKSELECTMODE*   pfnBankSelectMode;  // Routine to enable or disable
                                            //   direct frame buffer access
    BANK_MODE           bankmOnOverlapped;  // BANK_ON or BANK_ON_NO_WAIT,
                                            //   depending on whether card
                                            //   can handle simulataneous
                                            //   frame buffer and accelerator
                                            //   access
    BOOL                bMmIo;              // Can do CAPS_MM_IO

    // -------------------------------------------------------------------
    // NOTE: Changes up to here in the PDEV structure must be reflected in
    // i386\strucs.inc (assuming you're on an x86, of course)!

    CAPS        flCaps;                 // Capabilities flags
    STATUS      flStatus;               // Status flags
    BOOL        bEnabled;               // In graphics mode (not full-screen)

    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen surface
    DSURF*      pdsurfScreen;           // Our private DSURF for the screen

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    LONG        cxMemory;               // Width of Video RAM
    LONG        cyMemory;               // Height of Video RAM
    LONG        cBitsPerPel;            // Bits per pel (8, 15, 16, 24 or 32)
    ULONG       ulMode;                 // Mode the mini-port driver is in.

    FLONG       flHooks;                // What we're hooking from GDI
    ULONG       ulWhite;                // 0xff if 8bpp, 0xffff if 16bpp,
                                        //   0xffffffff if 32bpp
    UCHAR*      pucCsrBase;             // Mapped IO port base for this PDEV
    VOID*       pvTmpBuffer;            // General purpose temporary buffer,
                                        //   TMP_BUFFER_SIZE bytes in size
                                        //   (Remember to synchronize if you
                                        //   use this for device bitmaps or
                                        //   async pointers)
    UCHAR*      apjMmXfer[XFER_BUFFERS];// Pre-computed array of unique
    USHORT*     apwMmXfer[XFER_BUFFERS];//   addresses for doing memory-mapped
    ULONG*      apdMmXfer[XFER_BUFFERS];//   transfers without memory barriers
    CRITICAL_SECTION csCrtc;            // Used for synchronizing access to
                                        //   the CRTC register

    ////////// Low-level blt function pointers:

    FNFILL*     pfnFillSolid;
    FNFILL*     pfnFillPat;
    FNXFER*     pfnXfer1bpp;
    FNXFER*     pfnXfer4bpp;
    FNXFER*     pfnXferNative;
    FNCOPY*     pfnCopyBlt;
    FNFASTLINE* pfnFastLine;
    FNFASTFILL* pfnFastFill;
    FNFASTPATREALIZE* pfnFastPatRealize;
    FNIMAGETRANSFER*  pfnImageTransfer;
    FNTEXTOUT*  pfnTextOut;

    ////////// Palette stuff:

    PALETTEENTRY* pPal;                 // The palette if palette managed
    HPALETTE    hpalDefault;            // GDI handle to the default palette.
    FLONG       flRed;                  // Red mask for 16/32bpp bitfields
    FLONG       flGreen;                // Green mask for 16/32bpp bitfields
    FLONG       flBlue;                 // Blue mask for 16/32bpp bitfields
    ULONG       cPaletteShift;          // number of bits the 8-8-8 palette must
                                        // be shifted by to fit in the hardware
                                        // palette.
    ////////// Heap stuff:

    HEAP        heap;                   // All our off-screen heap data
    ULONG       iHeapUniq;              // Incremented every time room is freed
                                        //   in the off-screen heap

    ////////// Banking stuff:

    LONG        cjBank;                 // Size of a bank, in bytes
    LONG        cPower2ScansPerBank;    // Used by 'bBankComputePower2'
    LONG        cPower2BankSizeInBytes; // Used by 'bBankComputePower2'
    CLIPOBJ*    pcoBank;                // Clip object for banked call backs
    SURFOBJ*    psoBank;                // Surface object for banked call backs
    ULONG       aulBankData[BANK_DATA_SIZE / 4];
                                        // Private work area for downloaded
                                        //   miniport banking code

    FNBANKMAP*          pfnBankMap;
    FNBANKCOMPUTE*      pfnBankCompute;

    ////////// Pointer stuff:

    BOOL        bHwPointerActive;       // Currently using the h/w pointer?
    LONG        xPointerHot;            // xHot of current hardware pointer
    LONG        yPointerHot;            // yHot of current hardware pointer

    LONG        cjPointerOffset;        // Byte offset from start of frame
                                        //   buffer to off-screen memory where
                                        //   we stored the pointer shape
    LONG        xPointerShape;          // x-coordinate
    LONG        yPointerShape;          // y-coordinate
    LONG        iPointerBank;           // Bank containing pointer shape
    VOID*       pvPointerShape;         // Points to pointer shape when bank
                                        //   is mapped in
    LONG        xPointer;               // Start x-position for the current
                                        //   S3 pointer
    LONG        yPointer;               // Start y-position for the current
                                        //   S3 pointer
    LONG        dxPointer;              // Start x-pixel position for the
                                        //   current S3 pointer
    LONG        dyPointer;              // Start y-pixel position for the
                                        //   current S3 pointer
    LONG        cPointerShift;          // Horizontal scaling factor for
                                        //   hardware pointer position

    ULONG       ulHwGraphicsCursorModeRegister_45;
                                        // Default value for index 45
    VOID*       pvPointerData;          // Points to ajPointerData[0]
    BYTE        ajPointerData[POINTER_DATA_SIZE];
                                        // Private work area for downloaded
                                        //   miniport pointer code

    FNSHOWPOINTER*      pfnShowPointer;
    FNMOVEPOINTER*      pfnMovePointer;
    FNSETPOINTERSHAPE*  pfnSetPointerShape;
    FNENABLEPOINTER*    pfnEnablePointer;

    ////////// Brush stuff:

    LONG        cPatterns;              // Count of bitmap patterns created
    LONG        iBrushCache;            // Index for next brush to be allocated
    LONG        cBrushCache;            // Total number of brushes cached
    BRUSHENTRY  abe[TOTAL_BRUSH_COUNT]; // Keeps track of brush cache
    HBITMAP     ahbmPat[HS_DDI_MAX];    // Engine handles to standard patterns
    POINTL      ptlReRealize;           // Work area for 864/964 pattern
                                        //   hardware bug work-around

    ////////// 3D DDI Rendering Extension stuff:

    VOID*       pvOut;                  // Points to current output buffer
    VOID*       pvOutMax;               // Points to end of current output buffer
    VOID*       pvInMax;                // Points to end of current input buffer
    OH*         pohFrontBuffer;         // Allocation structure for the screen
    OH*         pohBackBuffer;          // Allocation structure for optional
                                        //   off-screen back buffer
    LONG        cDoubleBufferRef;       // Reference count for current number
                                        //   of RC's that have active double-
                                        //   buffers
    POINTL      ptlDoubleBuffer[2];     // (x, y) positions of front and back
                                        //   buffers in video memory
                                        //   0 -- RX_BACK_LEFT
                                        //   1 -- RX_FRONT_LEFT
                                        // Note: Make sure that the index
                                        //   is in the range [0, 1]!

} PDEV;

/////////////////////////////////////////////////////////////////////////
// Miscellaneous prototypes:

BOOL bIntersect(RECTL*, RECTL*, RECTL*);
LONG cIntersect(RECTL*, RECTL*, LONG);
BOOL bFastFill(PDEV*, LONG, POINTFIX*, ULONG, ULONG, ULONG, RBRUSH*);
DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION*, DWORD*);

BOOL bInitializeModeFields(PDEV*, GDIINFO*, DEVINFO*, DEVMODEW*);

BOOL bEnableHardware(PDEV*);
VOID vDisableHardware(PDEV*);
BOOL bAssertModeHardware(PDEV*, BOOL);

BOOL bInitializePatterns(PDEV*, INT);
VOID vUninitializePatterns(PDEV*);

extern BYTE gajHwMixFromMix[];

/////////////////////////////////////////////////////////////////////////
// The x86 C compiler insists on making a divide and modulus operation
// into two DIVs, when it can in fact be done in one.  So we use this
// macro.
//
// Note: QUOTIENT_REMAINDER implicitly takes unsigned arguments.

#if defined(i386)

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
{                                                               \
    __asm mov eax, ulNumerator                                  \
    __asm sub edx, edx                                          \
    __asm div ulDenominator                                     \
    __asm mov ulQuotient, eax                                   \
    __asm mov ulRemainder, edx                                  \
}

#else

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
{                                                               \
    ulQuotient  = (ULONG) ulNumerator / (ULONG) ulDenominator;  \
    ulRemainder = (ULONG) ulNumerator % (ULONG) ulDenominator;  \
}

#endif

/////////////////////////////////////////////////////////////////////////
// OVERLAP - Returns TRUE if the same-size lower-right exclusive
//           rectangles defined by 'pptl' and 'prcl' overlap:

#define OVERLAP(prcl, pptl)                                             \
    (((prcl)->right  > (pptl)->x)                                   &&  \
     ((prcl)->bottom > (pptl)->y)                                   &&  \
     ((prcl)->left   < ((pptl)->x + (prcl)->right - (prcl)->left))  &&  \
     ((prcl)->top    < ((pptl)->y + (prcl)->bottom - (prcl)->top)))

/////////////////////////////////////////////////////////////////////////
// SWAP - Swaps the value of two variables, using a temporary variable

#define SWAP(a, b, tmp) { (tmp) = (a); (a) = (b); (b) = (tmp); }

//////////////////////////////////////////////////////////////////////
// These Mul prototypes are thunks for multi-board support:

ULONG   MulGetModes(HANDLE, ULONG, DEVMODEW*);
DHPDEV  MulEnablePDEV(DEVMODEW*, PWSTR, ULONG, HSURF*, ULONG, ULONG*,
                      ULONG, DEVINFO*, PWSTR, PWSTR, HANDLE);
VOID    MulCompletePDEV(DHPDEV, HDEV);
HSURF   MulEnableSurface(DHPDEV);
BOOL    MulStrokePath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*, BRUSHOBJ*,
                      POINTL*, LINEATTRS*, MIX);
BOOL    MulFillPath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*,
                    MIX, FLONG);
BOOL    MulBitBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                  RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);
VOID    MulDisablePDEV(DHPDEV);
VOID    MulDisableSurface(DHPDEV);
VOID    MulAssertMode(DHPDEV, BOOL);
VOID    MulMovePointer(SURFOBJ*, LONG, LONG, RECTL*);
ULONG   MulSetPointerShape(SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*, LONG,
                           LONG, LONG, LONG, RECTL*, FLONG);
ULONG   MulDitherColor(DHPDEV, ULONG, ULONG, ULONG*);
BOOL    MulSetPalette(DHPDEV, PALOBJ*, FLONG, ULONG, ULONG);
BOOL    MulCopyBits(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, POINTL*);
BOOL    MulTextOut(SURFOBJ*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*, RECTL*,
                   BRUSHOBJ*, BRUSHOBJ*, POINTL*, MIX);
VOID    MulDestroyFont(FONTOBJ*);
BOOL    MulPaint(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*, MIX);
BOOL    MulRealizeBrush(BRUSHOBJ*, SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*,
                        ULONG);
HBITMAP MulCreateDeviceBitmap(DHPDEV, SIZEL, ULONG);
VOID    MulDeleteDeviceBitmap(DHSURF);
BOOL    MulStretchBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                      COLORADJUSTMENT*, POINTL*, RECTL*, RECTL*, POINTL*,
                      ULONG);

// These Dbg prototypes are thunks for debugging:

ULONG   DbgGetModes(HANDLE, ULONG, DEVMODEW*);
DHPDEV  DbgEnablePDEV(DEVMODEW*, PWSTR, ULONG, HSURF*, ULONG, ULONG*,
                      ULONG, DEVINFO*, PWSTR, PWSTR, HANDLE);
VOID    DbgCompletePDEV(DHPDEV, HDEV);
HSURF   DbgEnableSurface(DHPDEV);
BOOL    DbgStrokePath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*, BRUSHOBJ*,
                      POINTL*, LINEATTRS*, MIX);
BOOL    DbgFillPath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*,
                    MIX, FLONG);
BOOL    DbgBitBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                  RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);
VOID    DbgDisablePDEV(DHPDEV);
VOID    DbgDisableSurface(DHPDEV);
VOID    DbgAssertMode(DHPDEV, BOOL);
VOID    DbgMovePointer(SURFOBJ*, LONG, LONG, RECTL*);
ULONG   DbgSetPointerShape(SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*, LONG,
                           LONG, LONG, LONG, RECTL*, FLONG);
ULONG   DbgDitherColor(DHPDEV, ULONG, ULONG, ULONG*);
BOOL    DbgSetPalette(DHPDEV, PALOBJ*, FLONG, ULONG, ULONG);
BOOL    DbgCopyBits(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, POINTL*);
BOOL    DbgTextOut(SURFOBJ*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*, RECTL*,
                   BRUSHOBJ*, BRUSHOBJ*, POINTL*, MIX);
VOID    DbgDestroyFont(FONTOBJ*);
BOOL    DbgPaint(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*, MIX);
BOOL    DbgRealizeBrush(BRUSHOBJ*, SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*,
                        ULONG);
HBITMAP DbgCreateDeviceBitmap(DHPDEV, SIZEL, ULONG);
VOID    DbgDeleteDeviceBitmap(DHSURF);
BOOL    DbgStretchBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                      COLORADJUSTMENT*, POINTL*, RECTL*, RECTL*, POINTL*,
                      ULONG);
ULONG   DbgEscape(SURFOBJ*, ULONG, ULONG, VOID*, ULONG, VOID*);
