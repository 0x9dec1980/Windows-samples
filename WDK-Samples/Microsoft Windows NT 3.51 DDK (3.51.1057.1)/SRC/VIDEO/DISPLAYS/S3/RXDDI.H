/******************************Module*Header*******************************\
* Module Name: rxddi.h
*
*   This header file has the high-level structures and defines
*   needed for this implementation of the 3D DDI extensions. Any
*   hardware-specific members in the structure are clearly identified.
*
* Copyright (c) 1994-1995 Microsoft Corporation
*
\**************************************************************************/

// Clamping for memory sizes, offsets, and counts.  Note that
// CLAMPCOORD is a 'min/max' function because coordinates may
// be negative.

#define CLAMPMEM(value)   ((ULONG)(value) & 0x0fffffff)
#define CLAMPCOUNT(value) ((ULONG)(value) & 0x00007fff)
#define CLAMPCOORD(value) (max(-0x00007fffL, min(0x00007fffL, (LONG) (value))))

// Utility macros for command-buffer management:

#define SET_ERROR_CODE \
    pEsc->retValue = FALSE;

#define ERROR_RETURN \
{\
    pEsc->retValue = FALSE;\
    pEsc->prxCmd = pEsc->pBufEnd;\
    return;\
}

#define NONBATCHED_RETURN \
{\
    if (pEsc->prxCmd != pEsc->pBufEnd)\
    {\
        pEsc->retValue = FALSE;\
        pEsc->prxCmd = pEsc->pBufEnd;\
    }\
    return;\
}

#define NEXT_COMMAND(rxCmd)\
{\
    if (!PEEK_AHEAD(sizeof(rxCmd)))\
    {\
        RXDBG_PRINT("Input too small for command!");\
        ERROR_RETURN;\
    }\
    memcpy(&rxCmd, pEsc->prxCmd, sizeof(rxCmd));\
    pEsc->prxCmd = (RXCMD *)((char *)pEsc->prxCmd + sizeof(rxCmd));\
}

#define PEEK_AHEAD(size) \
    ((RXCMD *)((char *)pEsc->prxCmd + (size)) <= pEsc->pBufEnd)

#define ADVANCE_WITH_NO_CHECK(size) \
    pEsc->prxCmd = (RXCMD *)((char *)pEsc->prxCmd + (size));

#define ADVANCE_AND_CHECK(size)\
{\
    pEsc->prxCmd = (RXCMD *)((char *)pEsc->prxCmd + (size));\
    if (pEsc->prxCmd > pEsc->pBufEnd)\
    {\
        RXDBG_PRINT("Input buffer too small!");\
        pEsc->retValue = FALSE;\
        pEsc->prxCmd = pEsc->pBufEnd;\
        return;\
    }\
}

#define CHECK_BUFFER_SIZE(ptr)\
{\
    if ((RXCMD *)(ptr) > (pEsc->pBufEnd))\
    {\
        RXDBG_PRINT("Input buffer too small!");\
        pEsc->retValue = FALSE;\
        pEsc->prxCmd = pEsc->pBufEnd;\
        return;\
    }\
}


#define CHECK_SHARED_MEM_RANGE(ptr, prxMemObj)\
{\
    if (((char *)(ptr) > (prxMemObj->pMemMax)) ||\
        ((char *)(ptr) < (prxMemObj->pMem)))\
    {\
        RXDBG_PRINT("Pointer out of range in shared-memory window!");\
        pEsc->retValue = FALSE;\
        pEsc->prxCmd = pEsc->pBufEnd;\
        return;\
    }\
}


#define CHECK_SHARED_MEM_RANGE_RETVAL(ptr, prxMemObj)\
{\
    if (((char *)(ptr) > (prxMemObj->pMemMax)) ||\
        ((char *)(ptr) < (prxMemObj->pMem)))\
    {\
        RXDBG_PRINT("Pointer out of range in shared-memory window!");\
        return FALSE;\
    }\
}

//////////////////////////////////////////////////////////////////////////////
// CONVERT_POINTER_RANGE
//
// Converts a shared memory section pointer specified by an application
// from its client-side address space to the driver's server-side address
// space, and verifies that the pointer, and a following range of bytes,
// fit within the shared memory section.
//
// Under Windows 95, this can all be a NOP, depending on how much parameter
// validation the driver wants to do.  Under Windows NT, there is no choice
// for the driver -- full validation must be done.
//
// Also, pointers should be dword aligned.  Non-x86 platforms enforce this
// rule and abort if the pointer is not aligned, as they can't run the risk
// of getting an unaligned data exception.  x86 platforms do not enforce this
// as it would not cause a crash, and it's not worth the time to check (but
// it will print a warning message on checked builds).

#define GENERIC_CONVERT_POINTER_RANGE(p, cj, pjClient, cjMax, pjServer, ErrorRoutine) \
{                                                                             \
    (p) = (VOID*) ((ULONG) (p) - (ULONG) (pjClient));                         \
    if ((BYTE*) (p) > (BYTE*) (cjMax))                                        \
        goto ErrorRoutine;                                                    \
    if ((BYTE*) (p) + (ULONG) (cj) > (BYTE*) (cjMax))                         \
        goto ErrorRoutine;                                                    \
    (p) = (VOID*) ((ULONG) (p) + (ULONG) (pjServer));                         \
}

#if defined(i386)

    // On x86 checked builds, we print a message when a given pointer is
    // misaligned.
    //
    // On x86 free builds, we don't take the time to check.

    #define CONVERT_POINTER_RANGE(p, cj, pjClient, cjMax, pjServer, ErrorRoutine) \
    {                                                                             \
        GENERIC_CONVERT_POINTER_RANGE((p), (cj), (pjClient), (cjMax), (pjServer), \
                                      ErrorRoutine);                              \
        if (((ULONG) (p) & 3) != 0)                                               \
        {                                                                         \
            RXDBG_PRINT("Expect dword-aligned pointer.  Continuing...");          \
        }                                                                         \
    }

#else

    // On non-x86 checked builds, we print a message when a given pointer is
    // misaligned, and abort.
    //
    // On non-x86 free builds, we always abortwhen a given pointer is
    // misaligned.

    #define CONVERT_POINTER_RANGE(p, cj, pjClient, cjMax, pjServer, ErrorRoutine) \
    {                                                                             \
        GENERIC_CONVERT_POINTER_RANGE((p), (cj), (pjClient), (cjMax), (pjServer), \
                                      ErrorRoutine);                              \
        if (((ULONG) (p) & 3) != 0)                                               \
        {                                                                         \
            RXDBG_PRINT("Expect dword-aligned pointer.  Aborting...");            \
            goto ErrorRoutine;                                                    \
        }                                                                         \
    }

#endif

typedef enum {
    RXHANDLE_RXRC,
    RXHANDLE_SHAREMEM,
} RXHANDLETYPE;


typedef struct _RXMEMOBJ {
    RXHANDLE hrxRC;             // Rendering-context handle of owner (if any)
    RXHANDLETYPE type;          // Object type
    RXHANDLE hrxSharedMem;      // the handle for the shared memory
    char *pMem;                 // pointer to shared memory
    char *pMemMax;              // pointer to end of shared memory
    char *clientBaseAddress;    // client's start address
    ULONG addrOffset;           // address fixup for converting client pointers
                                // to shared-memory pointers
    ULONG size;                 // size of shared memory
    HANDLE hMapFile;            // handle to mapped file
} RXMEMOBJ;


typedef struct _RXESCAPE
{
    RXHDR *prxHdr;               // RXHDR for command buffer
    RXCMD *prxCmd;               // start of current command
    RXCMD *pBufEnd;              // end of command buffer
    PVOID pvOut;                 // output buffer
    LONG cjOut;                  // output buffer size
    struct _RXRC *pRc;           // current rendering context
    DWORD retValue;              // return value for command batch
    struct _RXWINDOW *pWnd;      // window info
    RXMEMOBJ *prxMemVertexData;  // shared-memory cache for vertex data
    RXMEMOBJ *prxMemVertexPtr;   // shared-memory cache for vertex pointers
    RXMEMOBJ *prxMemCache;       // shared-memory cache for everything else
    WNDOBJ *pwo;                 // WNDOBJ
    SURFOBJ *pso;                // SURFOBJ for escape
    PDEV *ppdev;                 // device-driver info
} RXESCAPE;


typedef void (*RXFUNC)(RXESCAPE *);
typedef void (*PRIMFUNC)(RXESCAPE *, RXDRAWPRIM *);

typedef struct _RXRC
{
    RXHANDLE rxHandle;
    ULONG hwnd;
    ULONG vertexStride;
    ULONG coordStride;
    ULONG colorStride;

// Generic state information

    ULONG flags;
    RXLINEPAT linePattern;
    RXSTIPPLE stipple;
    ULONG rop2;
    ULONG spanType;
    ULONG activeBuffer;
    ULONG planeMask;
    BOOL zMask;
    BOOL zEnable;
    BOOL alphaTestEnable;
    BOOL lastPixel;
    ULONG texMag;
    ULONG texMin;
    ULONG srcBlend;
    ULONG dstBlend;
    ULONG texMapBlend;
    ULONG cullMode;
    ULONG spanDirection;
    ULONG zFunc;
    ULONG alphaRef;
    ULONG alphaFunc;
    BOOL ditherEnable;
    ULONG blendEnable;
    RXHANDLE texture;
    RXCOLOR fillColor;
    ULONG fillZ;
    RXCOLOR solidColor;
    BOOL scissorsEnabled;
    RXRECT scissorsRect;
    ULONG maskStart;
    ULONG shadeMode;
    ULONG vertexType;
    ULONG vertexColorType;
    ULONG spanColorType;
    ULONG spanRealMode;
    ULONG pointRealMode;
    ULONG lineRealMode;
    ULONG triangleRealMode;
    ULONG quadRealMode;
    BOOL texturePerspective;
    BOOL texTranspEnable;
    ULONG texTranspColor;
    ULONG ditherOrigin;

    BOOL zBufEnabled;
    BOOL backBufEnabled;

// Primitive-rendering function table:

    PRIMFUNC primFunc[RX_PRIM_INTLINESTRIP + 1];

// Hardware-specific state and other information. Modifications will
// not affect high-level (rxddi) code:

    ULONG hwZFunc;
    ULONG hwRop2;
    ULONG hwSolidColor;
    ULONG hwFillColor;
    ULONG hwFillZ;
    ULONG hwPlaneMask;
    ULONG hwBackgroundMode;
    ULONG hwColorExpand;
    ULONG hwLineFuncOp;
    ULONG hwPatternLength;
    ULONG hwPatternStart;
    ULONG hwPatternPos;

// Not used in this driver, but good information to have around:

    ULONG hwZPitch;
    ULONG hwZBytesPerPixel;
    ULONG hwCPitch;
    ULONG hwCBytesPerPixel;

// 2-D off-screen heap stuff:

    LONG xActiveOffset;      // x offset to active buffer
    LONG yActiveOffset;      // y offset to active buffer

} RXRC;

typedef struct _RXRCOBJ {
    RXHANDLE hrxRC;
    RXHANDLETYPE type;
    struct _RXRCOBJ *next;
    VOID *pObject;
    HANDLE handle;
    ULONG size;
} RXRCOBJ;

// Number of list rectangles we can keep in our default buffer:

#define NUM_DEFAULT_CLIP_BUFFER_RECTS   20

// Size in bytes of default buffer size for storing our list of
// current clip rectangles:

#define SIZE_DEFAULT_CLIP_BUFFER        \
    2 * ((NUM_DEFAULT_CLIP_BUFFER_RECTS * sizeof(RECTL)) + sizeof(ULONG))

typedef struct _RXWINDOW
{
    RXRCOBJ *objectList;            // List of objects associated with this
                                    //   window
    WNDOBJ *pwo;                    // WndObj with which this is associated
    ULONG hwnd;                     // Window with which this is associated
    RECTL clientRect;               // Rectangle describing current window
                                    //   client area
    ENUMRECTS *prxClipUnscissored;  // List of rectangles describing the
                                    //   entire current clip region
    ENUMRECTS *prxClipScissored;    // List of rectangles describing the
                                    //   current clip region intersected
                                    //   with the current scissors rectangle
                                    // Note: This list is particular to an RC,
                                    //   so has to be reset whenever a new
                                    //   RC is used
    char defaultClipBuffer[SIZE_DEFAULT_CLIP_BUFFER];
                                    // Used for storing above rectangle lists
                                    //   when they can fit
    char *pAllocatedClipBuffer;     // Points to allocated storage for storing
                                    //   rectangle lists when they don't fit
                                    //   in 'defaultClipBuffer'.  NULL if
                                    //   not allocated.
    ULONG sizeClipBuffer;           // Size of clip storage pointed to by
                                    //   'prxClipScissored' taking both
                                    //   lists into account.

//  Hardware-specific values:

    PDEV *ppdev;
    ULONG TGHWWriteFrontBuffer;
    ULONG TGHWWriteBackBuffer;
    ULONG utilityBitValue;
} RXWINDOW;

// external variables and function prototypes

extern RXFUNC rxFuncs[];

ULONG RxHandler(RXESCAPE *);
BOOL RxEnable(void *);
void RxCreateContext(RXESCAPE *);
void RxDeleteResource(RXESCAPE *);
void RxGetInfo(RXESCAPE *);
void RxMapMem(RXESCAPE *);
void RxEnableBuffers(RXESCAPE *);
void RxDrawPrim(RXESCAPE *);
void RxSwapBuffers(RXESCAPE *);
void RxDrawPolySpan(RXESCAPE *);
void RxPolyWriteSpan(RXESCAPE *);
void RxFillRect(RXESCAPE *);
void RxSetState(RXESCAPE *);
void RxReadRect(RXESCAPE *);
void RxWriteRect(RXESCAPE *);
void RxFlush(RXESCAPE *);
void RxNullFunc(RXESCAPE *);
void RxFlush(RXESCAPE *);

HANDLE CreateMapMemObj(RXESCAPE *, RXMAPMEM *);
BOOL DestroyMapMemObj(RXWINDOW *, RXHANDLE);
BOOL DestroyRCObj(RXWINDOW *, HANDLE, RXHANDLE);
BOOL DestroyRC(RXWINDOW *, RXHANDLE);
VOID *GetPtrFromHandle(HANDLE, RXHANDLETYPE type);
RXWINDOW *NewRxWindowTrack(RXESCAPE *, RXCREATECONTEXT *);
RXRC *NewRxRc(RXESCAPE *);
void GetScissorClip(RXESCAPE *);
HANDLE CreateRcObj(RXESCAPE *, RXWINDOW *, PVOID, RXHANDLETYPE,
                   RXHANDLE, DWORD);

#if DBG
#define RxLocalAlloc   RxDbgLocalAlloc
#define RxLocalFree    RxDbgLocalFree
#else
#define RxLocalAlloc   LocalAlloc
#define RxLocalFree    LocalFree
#endif

#define RX_DBGMSG_PREFIX        "[RX] "

#if DBG

VOID RxDebugPrint(char *, ...);

#define RXDBG_PRINT             RxDebugPrint
#else
#define RXDBG_PRINT
#endif

// Inline function to find the intersection of two rectangles:

_inline void RxIntersectRect(RECTL *pRectInter, RECTL *pRectA, RECTL *pRectB)
{
    // Get intersection of left, right, top, and bottom edges of the
    // two source rectangles:

    pRectInter->left   = max(pRectA->left, pRectB->left);
    pRectInter->right  = min(pRectA->right, pRectB->right);
    pRectInter->top    = max(pRectA->top, pRectB->top);
    pRectInter->bottom = min(pRectA->bottom, pRectB->bottom);
}





