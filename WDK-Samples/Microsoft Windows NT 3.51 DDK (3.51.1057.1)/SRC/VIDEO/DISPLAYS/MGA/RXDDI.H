/******************************Module*Header*******************************\
* Module Name: rxddi.h
*
*   This header file has the high-level structures and defines
*   needed for this implementation of the 3D DDI extensions. Any
*   hardware-specific members in the structure are clearly identified.
*
* Copyright (c) 1994, 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef _RXDDI_H
#define _RXDDI_H

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

#define PEEK_AHEAD(size) \
    ((RXCMD *)((char *)pEsc->prxCmd + (size)) <= pEsc->pBufEnd)

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

#define CHECK_SHARED_MEM_SIZE(size, prxMemObj)\
{\
    if (size > (prxMemObj->size))\
    {\
        RXDBG_PRINT("Object too large for shared-memory window!");\
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

#define TOFIX(a)    ((RXREAL)((ULONG)(a) << 16))


typedef enum {
    RXHANDLE_RXRC,
    RXHANDLE_SHAREMEM,
    RXHANDLE_TEXTUREHEAP,
    RXHANDLE_TEXTURE,
} RXHANDLETYPE;

typedef struct _RXTEXHEAPOBJ {
    RXHANDLE hrxRC;             // Rendering-context handle
    RXHANDLETYPE type;          // Object type
    ULONG size;                 // size of this structure
    VOID *pHeap;                // pointer texture heap
} RXTEXHEAPOBJ;

typedef struct _RXTEXOBJ {
    RXHANDLE hrxRC;             // Rendering-context handle
    RXHANDLETYPE type;          // Object type
    RXHANDLE hrxTextureHeap;    // Handle of texture heap
    ULONG size;                 // size of this structure
    VOID *pTex;                 // pointer texture heap
} RXTEXOBJ;

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
    ULONG spanColorStride;
    ULONG vertexColorStride;
    ULONG stripVertexStride;
    ULONG listPointerStride;
    ULONG stripVertexSkip;
    ULONG listPointerSkip;
    ULONG lineListPointerStride;
    ULONG triListPointerStride;
    ULONG quadListPointerStride;

// Generic state information

    ULONG flags;
    RXLINEPAT linePattern;
    RXSTIPPLE stipple;
    ULONG rop2;
    ULONG spanType;
    ULONG activeBuffer;
    ULONG planeMask;
    BOOL zWriteEnable;
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

    ULONG hwLineFunc;
    ULONG hwIntLineFunc;
    ULONG hwTrapFunc;
    ULONG hwSpanFunc;
    ULONG hwSolidColor;
    ULONG hwFillColor;
    ULONG hwFillZ;
    ULONG hwPlaneMask;
    ULONG hwPatternLength;
    ULONG hwPatternStart;
    ULONG hwPatternPos;

    ULONG hwBpp;
    ULONG hwBufferOffset;
    ULONG hwBackBufferOffset;
    CHAR hwFifo;
    BOOL hwAllCaps;
    PDEV* ppdev;

// shifts for converting to native bit depth

    ULONG rShift;
    ULONG gShift;
    ULONG bShift;
    ULONG aShift;

// shifts for converting to hardware format

    ULONG hwRShift;
    ULONG hwGShift;
    ULONG hwBShift;
    ULONG hwAShift;

// Not used in this driver, but good information to have around:

    ULONG hwZPitch;
    ULONG hwZBytesPerPixel;
    ULONG hwCPitch;
    ULONG hwCBytesPerPixel;

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
} RXWINDOW;

// external variables and function prototypes

extern RXFUNC rxFuncs[];

ULONG RxEscape(RXESCAPE *);
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
void RxTextureHeap(RXESCAPE *);
void RxAllocTexture(RXESCAPE *pEsc);
void RxLoadTexture(RXESCAPE *pEsc);
VOID RxQueryTextureMemory(RXESCAPE *);

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
RXHANDLE CreateTexHeapObj(RXESCAPE *, VOID *);
BOOL DestroyTexHeapObj(RXHANDLE);
RXHANDLE CreateTexObj(RXESCAPE *, RXHANDLE, VOID *, RXTEXTURE **);
BOOL DestroyTexObj(RXHANDLE);

#if DBG
HLOCAL RxDbgLocalAlloc(UINT, UINT);
HLOCAL RxDbgLocalFree(HLOCAL);

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

#endif /* _RXDDI_H */

