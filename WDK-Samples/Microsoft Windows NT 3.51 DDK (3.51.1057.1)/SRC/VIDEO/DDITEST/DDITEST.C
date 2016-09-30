/******************************Module*Header*******************************\
* Module Name: dditest.c
*
* Copyright (c) 1993-1995 Microsoft Corporation
*
* Contains integer line tests for the 3D DDI.
*
\**************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <windows.h>
#include <winddi.h>
#include <rx.h>

typedef struct _GMEM {
    RXHDR rxHdr;
    HANDLE hSharedMem;
    VOID* pvBuf;
} GMEM;

#define DIFF(end, start) ((BYTE*) (end) - (BYTE*) (start))

DWORD totalLines = 0;
RXHANDLE ghrxMemMap = 0;
RXHANDLE ghrxRC = 0;
FLONG gflColorMode;
GMEM gmem;

extern HDC ghdc;

typedef struct _DRAWBUF {
    HANDLE hSharedMem;
    RXHANDLE hrxSharedMem;
    VOID *pvSharedMem;
    char *pDataCurrent;
    struct _DRAWBUF *next;
    char *pEndBuffer;
} DRAWBUF;

typedef struct _CMDBUF {
    HANDLE hSharedMem;
    RXHANDLE hrxSharedMem;
    VOID *pvSharedMem;
    VOID *pCmdBuffer;
    ULONG sharedMemSize;
    struct _CMDBUF *next;
    RXDRAWPRIM *pStripCurrent;
    char *pEndBuffer;
} CMDBUF;

static DRAWBUF *gpDrawBuf;      // Current vertex buffer structure
static CMDBUF *gpCmdBuf;        // Current command buffer structure
static DRAWBUF *gpDrawFirst;     // First vertex buffer structure
static CMDBUF *gpCmdFirst;      // First command buffer structure
static PCHAR pFileName;

#define BUFFERSIZE 65536L

PALETTEENTRY gapal[] = {

    { 0,   0,   0,   0 },       // 0
    { 0x80,0,   0,   0 },       // 1
    { 0,   0x80,0,   0 },       // 2
    { 0x80,0x80,0,   0 },       // 3
    { 0,   0,   0x80,0 },       // 4
    { 0x80,0,   0x80,0 },       // 5
    { 0,   0x80,0x80,0 },       // 6
    { 0x80,0x80,0x80,0 },       // 7

    { 0xC0,0xC0,0xC0,0 },       // 8
    { 0xFF,0,   0,   0 },       // 9
    { 0,   0xFF,0,   0 },       // 10
    { 0xFF,0xFF,0,   0 },       // 11
    { 0,   0,   0xFF,0 },       // 12
    { 0xFF,0,   0xFF,0 },       // 13
    { 0,   0xFF,0xFF,0 },       // 14
    { 0xFF,0xFF,0xFF,0 }        // 15
};

void initBuffer()
{
    gpDrawFirst = NULL;
    gpCmdFirst = NULL;
    gpDrawBuf = NULL;
    gpCmdBuf = NULL;
}

VOID* createSection(HDC hdc, int bufferSize, RXHANDLE* phrxSharedMem, HANDLE* phSharedMem)
{
    HANDLE hSharedMem;
    char *pmem;
    char ajMapMem[sizeof(RXHDR) + sizeof(RXMAPMEM)];
    RXHDR *prxHdr;
    RXMAPMEM *prxMapMem;
    RXHANDLE hrxSharedMem;

    pmem = NULL;
    hrxSharedMem = 0;

    hSharedMem = CreateFileMapping((HANDLE) -1, NULL, PAGE_READWRITE | SEC_COMMIT, 0,
                             bufferSize, NULL);
    if (hSharedMem)
    {
        pmem = MapViewOfFile(hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (pmem)
        {
            // We have to register this memory section with the display driver:

            prxHdr = (RXHDR*) ajMapMem;
            prxHdr->flags                           = RX_FL_MAP_MEM;
            prxHdr->hrxRC                           = 0;
            prxHdr->hrxSharedMem                    = 0;

            prxMapMem = (RXMAPMEM*) (prxHdr + 1);
            prxMapMem->command                      = RXCMD_MAP_MEM;
            prxMapMem->action                       = RX_CREATE_MEM_MAP;
            prxMapMem->shareMem.sourceProcessID     = GetCurrentProcessId();
            prxMapMem->shareMem.hSource             = hSharedMem;
            prxMapMem->shareMem.offset              = 0;
            prxMapMem->shareMem.size                = bufferSize;
            prxMapMem->shareMem.clientBaseAddress   = (ULONG) pmem;

            hrxSharedMem = (RXHANDLE) ExtEscape(hdc, RXFUNCS,
                            DIFF(prxMapMem + 1, ajMapMem), (LPCSTR) ajMapMem,
                            0, NULL);

            if (!hrxSharedMem)
            {
                printf("Failed RXCMD_MAP_MEM RX_CREATE_MEM_MAP\n");
                return(NULL);
            }
        }
    }

    *phrxSharedMem = hrxSharedMem;
    *phSharedMem = hSharedMem;
    return(pmem);
}

VOID newCmdBuffer(int bufferSize)
{
    char *pmem;
    HANDLE hSharedMem;
    RXHANDLE hrxSharedMem;

    pmem = createSection(ghdc, bufferSize, &hrxSharedMem, &hSharedMem);
    if (pmem == NULL)
    {
        printf("We really should handle this more gracefully here\n");
        printf("We failed the createSection, so we're about to AV...\n");
    }

    // Now take care of our own lists:

    if (gpCmdBuf)
        gpCmdBuf->next = (CMDBUF *)pmem;
    else
        gpCmdFirst = (CMDBUF *)pmem;

    gpCmdBuf = (CMDBUF *)pmem;
    gpCmdBuf->next            = NULL;
    gpCmdBuf->pEndBuffer      = (char *)gpCmdBuf +
                                 (bufferSize - sizeof(CMDBUF) - 1);
    gpCmdBuf->hSharedMem      = hSharedMem;
    gpCmdBuf->pvSharedMem     = gpCmdBuf;
    gpCmdBuf->hrxSharedMem    = hrxSharedMem;
    gpCmdBuf->pCmdBuffer      = (VOID *)(pmem + sizeof(CMDBUF));
    gpCmdBuf->sharedMemSize   = bufferSize;
    gpCmdBuf->pStripCurrent   = (RXDRAWPRIM *)(char *)(gpCmdBuf + 1);
}


VOID newDrawBuffer(int bufferSize)
{
    char *pmem;
    HANDLE hSharedMem;
    RXHANDLE hrxSharedMem;

    pmem = createSection(ghdc, bufferSize, &hrxSharedMem, &hSharedMem);
    if (pmem == NULL)
    {
        printf("We really should handle this more gracefully here\n");
        printf("We failed the createSection, so we're about to AV...\n");
    }

    // Now take care of our own lists:

    if (gpDrawBuf)
        gpDrawBuf->next = (DRAWBUF *)pmem;
    else
        gpDrawFirst = (DRAWBUF *)pmem;

    gpDrawBuf = (DRAWBUF *)pmem;
    gpDrawBuf->next            = NULL;
    gpDrawBuf->pEndBuffer      = (char *)gpDrawBuf +
                                 (bufferSize - sizeof(DRAWBUF) - 1);
    gpDrawBuf->hSharedMem      = hSharedMem;
    gpDrawBuf->pvSharedMem     = gpDrawBuf;
    gpDrawBuf->hrxSharedMem    = hrxSharedMem;
    gpDrawBuf->pDataCurrent    = (char *)(gpDrawBuf + 1);
}

void unmapMem(RXHANDLE hrxSharedMem, HANDLE hSharedMem, VOID *pvSharedMem)
{
    char ajMapMem[sizeof(RXHDR) + sizeof(RXMAPMEM)];
    RXHDR *prxHdr;
    RXMAPMEM *prxMapMem;

    prxHdr = (RXHDR*) ajMapMem;
    prxHdr->flags           = RX_FL_MAP_MEM;
    prxHdr->hrxRC           = 0;
    prxHdr->hrxSharedMem    = 0;

    prxMapMem = (RXMAPMEM*) (prxHdr + 1);
    prxMapMem->command      = RXCMD_MAP_MEM;
    prxMapMem->action       = RX_DELETE_MEM_MAP;
    prxMapMem->hrxSharedMem = hrxSharedMem;

    if (!ExtEscape(ghdc, RXFUNCS,
                   DIFF(prxMapMem + 1, ajMapMem), (LPCSTR) ajMapMem,
                   0, NULL))
    {
        printf("Couldn't delete shared memory resource\n");
    }

    if (!UnmapViewOfFile(pvSharedMem))
    {
        printf("Couldn't UnmapViewOfFile\n");
    }

    if (!CloseHandle(hSharedMem))
    {
        printf("Couldn't CloseHandle\n");
    }
}

void freeBuffers()
{
    DRAWBUF *pdBuf;
    DRAWBUF *nextDBuf;
    CMDBUF *pcBuf;
    CMDBUF *nextCBuf;
    RXHDR *prxHdr;
    HANDLE hSharedMem;
    RXMAPMEM *prxMapMem;

    for (pdBuf = gpDrawFirst; pdBuf; ) {

        nextDBuf = pdBuf->next;

	unmapMem(pdBuf->hrxSharedMem, pdBuf->hSharedMem, pdBuf->pvSharedMem);

        pdBuf = nextDBuf;
    }

    for (pcBuf = gpCmdFirst; pcBuf; ) {

        nextCBuf = pcBuf->next;

	unmapMem(pcBuf->hrxSharedMem, pcBuf->hSharedMem, pcBuf->pvSharedMem);

        pcBuf = nextCBuf;
    }

    gpDrawFirst = NULL;
    gpCmdFirst = NULL;
}

void endCmdBuffer()
{
    gpCmdBuf->sharedMemSize = (char *)gpCmdBuf->pStripCurrent -
                              (char *)gpCmdBuf->pCmdBuffer;
}

void endDrawBuffer()
{
}

ULONG roomInCmdBuffer()
{
    return (gpCmdBuf->pEndBuffer - (char *)gpCmdBuf->pStripCurrent);
}

ULONG roomInDrawBuffer()
{
    return (gpDrawBuf->pEndBuffer - gpDrawBuf->pDataCurrent);
}

void newClear(int xsize, int ysize)
{
    RXSETSTATE *prxSetState;
    RXCOLOR *prxColor;
    RXFILLRECT *prxFillRect;

    prxSetState = (RXSETSTATE *)gpCmdBuf->pStripCurrent;
    prxSetState->command = RXCMD_SET_STATE;
    prxSetState->stateToChange = RX_FILL_COLOR;

    prxColor = (RXCOLOR *)&prxSetState->newState;
    prxColor->r = 0;
    prxColor->g = 0;
    prxColor->b = 0;
    prxColor->a = 0;

    prxFillRect = (RXFILLRECT *) (prxColor + 1);
    prxFillRect->command = RXCMD_FILL_RECT;
    prxFillRect->fillType = RX_FILL_RECT_COLOR;
    prxFillRect->fillRect.x = 0;
    prxFillRect->fillRect.y = 0;
    prxFillRect->fillRect.width = xsize;
    prxFillRect->fillRect.height = ysize;

    (char *)gpCmdBuf->pStripCurrent = (char *)(prxFillRect + 1);
}

void newColor(DWORD color)
{
    RXSETSTATE *ddiStateCmd;
    RXCOLOR *prxColor;

    ddiStateCmd = (RXSETSTATE *)gpCmdBuf->pStripCurrent;

    ddiStateCmd->command = RXCMD_SET_STATE;
    ddiStateCmd->stateToChange = RX_SOLID_COLOR;
    prxColor = (RXCOLOR *)&ddiStateCmd->newState;

    if (gflColorMode & RX_COLOR_INDEXED)
    {
        // The COLOR_INDEXED colour is specified as the index into the
        // currently realized hardware palette:

        if (color >= 8)
            color = color + (247 - 8);

        prxColor->r = (UCHAR) color;
    }
    else
    {
        // The RGBA colour is specified to the 3D DDI using an 8-8-8-8
        // structure:

        prxColor->r = gapal[color].peRed;
        prxColor->g = gapal[color].peGreen;
        prxColor->b = gapal[color].peBlue;
    }

    (char *)gpCmdBuf->pStripCurrent = (char *)(prxColor + 1);
}

void newPoint(ULONG x, ULONG y, ULONG primType)
{
    RXPOINTINT *pPt;
    RXDRAWPRIM *ddiStripCmd;

    ddiStripCmd = gpCmdBuf->pStripCurrent;
    gpCmdBuf->pStripCurrent += 1;
    ddiStripCmd->command = RXCMD_DRAW_PRIM;
    ddiStripCmd->primType = primType;
    ddiStripCmd->numVertices = 1;
    ddiStripCmd->hrxSharedMemVertexData = gpDrawBuf->hrxSharedMem;
    ddiStripCmd->pSharedMem = (void *)gpDrawBuf->pDataCurrent;

    pPt = (RXPOINTINT *)gpDrawBuf->pDataCurrent;
    gpDrawBuf->pDataCurrent += sizeof(RXPOINTINT);
    pPt->x = (LONGFIX)x;
    pPt->y = (LONGFIX)y;
}

void addPoint(long x, long y)
{
    RXPOINTINT *pPt;

    pPt = (RXPOINTINT *)gpDrawBuf->pDataCurrent;
    gpDrawBuf->pDataCurrent += sizeof(RXPOINTINT);
    pPt->x = (LONGFIX)x;
    pPt->y = (LONGFIX)y;

    (gpCmdBuf->pStripCurrent - 1)->numVertices++;
}


VOID vReadSlide(char *strfname, int xsize, int ysize)
{
    HFILE hfile;
    DWORD numread;
    DWORD last_x;
    DWORD last_y;
    DWORD last_color;
    DWORD org_xsize, org_ysize;
    unsigned char buffer[4096];
    unsigned char *pbuf;
    int last_op = 0;
    BOOL bStartPoly = FALSE;
    DWORD polyVCount = 0;
    DWORD firstPolyX;
    DWORD firstPolyY;
    float aspect_org, aspect;

#define SCALEX(x)   (((x) * xsize) / org_xsize)
#define SCALEY(y)   (((y) * ysize) / org_ysize)

    totalLines = 0;
    hfile = _lopen(strfname, OF_READ);

    if (hfile == HFILE_ERROR)
        return;

    initBuffer();

    newCmdBuffer(BUFFERSIZE);
    newDrawBuffer(BUFFERSIZE);
    newClear(xsize, ysize);

    _lread(hfile, buffer, 17);
    _lread(hfile, buffer, 14);
    org_xsize = *(short UNALIGNED *)(buffer + 2) + 1;
    org_ysize = *(short UNALIGNED *)(buffer + 4) + 1;

    aspect_org = (float)org_xsize / (float)org_ysize;
    aspect = (float)xsize / (float)ysize;

    if (aspect_org > aspect)
        ysize = (DWORD)((float)ysize * (aspect / aspect_org));
    else
        xsize = (DWORD)((float)xsize * (aspect_org / aspect));

    numread = _lread(hfile, buffer, sizeof(buffer));
    pbuf = buffer;

    while (numread) {
        unsigned char rec;
        long x1, y1, x2, y2;

        if (numread < 8) {
            memcpy(buffer, pbuf, numread);
            numread += _lread(hfile, buffer + numread,
                              sizeof(buffer) - numread);
            pbuf = buffer;
        }

	rec = *(pbuf + 1);

        if (rec <= 0x7f) {

            if (roomInDrawBuffer() < (2 * sizeof(RXPOINTINT))) {
                endDrawBuffer();
                newDrawBuffer(BUFFERSIZE);
            }

            if (roomInCmdBuffer() < sizeof(RXDRAWPRIM)) {
                endCmdBuffer();
                newCmdBuffer(BUFFERSIZE);
            }

            last_x = x2 = *(short UNALIGNED *)pbuf;
            last_y = y2 = *(short UNALIGNED *)(pbuf + 2);
            x1 = *(short UNALIGNED *)(pbuf + 4);
            y1 = *(short UNALIGNED *)(pbuf + 6);

            newPoint(SCALEX(x1), ysize - SCALEY(y1), RX_PRIM_INTLINESTRIP);
            addPoint(SCALEX(x2), ysize - SCALEY(y2));
            totalLines++;
            pbuf += 8;
            numread -= 8;
        } else {
            switch (rec) {
            case 0xfb:

                if (roomInDrawBuffer() < (2 * sizeof(RXPOINTINT))) {
                    endDrawBuffer();
                    newDrawBuffer(BUFFERSIZE);
                }

                if (roomInCmdBuffer() < sizeof(RXDRAWPRIM)) {
                    endCmdBuffer();
                    newCmdBuffer(BUFFERSIZE);
                }

                x2 = last_x + *((char *)(pbuf));
                y2 = last_y + *((char *)(pbuf+2));
                x1 = last_x + *((char *)(pbuf+3));
                y1 = last_y + *((char *)(pbuf+4));

                last_x = x2;
                last_y = y2;

                newPoint(SCALEX(x1), ysize - SCALEY(y1), RX_PRIM_INTLINESTRIP);
                addPoint(SCALEX(x2), ysize - SCALEY(y2));
                totalLines++;
                pbuf += 5;
                numread -= 5;
                break;
            case 0xfc:          // EOF
                pbuf += 2;
                numread -= 2;
                _lclose(hfile);
                endCmdBuffer();
                endDrawBuffer();
                return;
                break;
            case 0xfd:          // Solid fill
                x1 = *(short UNALIGNED *)(pbuf + 2);
                y1 = *(short UNALIGNED *)(pbuf + 4);

		if (y1 < 0) {
                    if (x1 > 0) {
                        bStartPoly = TRUE;
                        polyVCount = x1;
                        totalLines += x1;
                        if (roomInDrawBuffer() <
                            ((x1 + 1) * sizeof(RXPOINTINT))) {
                            endDrawBuffer();
                            newDrawBuffer(BUFFERSIZE);
                        }

                        if (roomInCmdBuffer() < sizeof(RXDRAWPRIM)) {
                            endCmdBuffer();
                            newCmdBuffer(BUFFERSIZE);
                        }
                    }
                } else {
                    polyVCount--;

                    if (bStartPoly) {
                        firstPolyX = SCALEX(x1);
                        firstPolyY = ysize - SCALEY(y1);
                        newPoint(firstPolyX, firstPolyY, RX_PRIM_INTLINESTRIP);
                    } else {
                        addPoint(SCALEX(x1), ysize - SCALEY(y1));
                        if (!polyVCount)
                            addPoint(firstPolyX, firstPolyY);
                    }

                    bStartPoly = FALSE;
                }

                pbuf += 6;
                numread -= 6;
                break;
            case 0xfe:          // Common endpoint vector

                if (roomInDrawBuffer() < (2 * sizeof(RXPOINTINT))) {
                    endDrawBuffer();
                    newDrawBuffer(BUFFERSIZE);
                    last_op = 0xff;
                }

                if (roomInCmdBuffer() < sizeof(RXDRAWPRIM)) {
                    endCmdBuffer();
                    newCmdBuffer(BUFFERSIZE);
                    last_op = 0xff;
                }

                if ((last_op == 0xfd) || (last_op == 0xff))
                    newPoint(SCALEX(last_x), ysize - SCALEY(last_y), RX_PRIM_INTLINESTRIP);

                x1 = last_x + *((char *)(pbuf));
                y1 = last_y + *((char *)(pbuf+2));
                last_x = x1;
                last_y = y1;
                addPoint(SCALEX(x1), ysize - SCALEY(y1));
                totalLines++;
                pbuf += 3;
                numread -= 3;
                break;
            case 0xff:          // New color

                if (roomInCmdBuffer() < (sizeof(RXSETSTATE) +
                                         sizeof(RXCOLOR))) {
                    endCmdBuffer();
                    newCmdBuffer(BUFFERSIZE);
                }

                last_color = (DWORD)*pbuf & 0xff;
		newColor(last_color);
                pbuf += 2;
                numread -= 2;
                break;
            default:
                break;
            }
        }
        last_op = rec;
    }
    endDrawBuffer();
    endCmdBuffer();
    _lclose(hfile);
}

void doRedraw()
{
    RXHDR   rxHdr;
    CMDBUF *pcBuf = gpCmdFirst;

    rxHdr.flags = 0;
    rxHdr.hrxRC = ghrxRC;

    while (pcBuf) {
        rxHdr.hrxSharedMem    = pcBuf->hrxSharedMem;
        rxHdr.pSharedMem      = pcBuf->pCmdBuffer;
        rxHdr.sharedMemSize   = pcBuf->sharedMemSize;

        if (!ExtEscape(ghdc, RXFUNCS, sizeof(RXHDR), (LPCSTR) &rxHdr, 0, NULL))
            printf("Drawing failed.\n");

        pcBuf = pcBuf->next;
    }
}

BOOL close3dDDI()
{
    BYTE ajMapMem[sizeof(RXHDR) + sizeof(RXMAPMEM)];
    RXDELETERESOURCE *prxDeleteResource;
    RXMAPMEM *prxMapMem;
    RXHDR *prxHdr;
    BOOL bRet;

    // Check for the case where we failed initialization:

    if (gmem.pvBuf == NULL)
        return(TRUE);

    prxDeleteResource = gmem.pvBuf;
    prxDeleteResource->command     = RXCMD_DELETE_RESOURCE;
    prxDeleteResource->hrxResource = ghrxRC;

    gmem.rxHdr.sharedMemSize = sizeof(RXDELETERESOURCE);
    gmem.rxHdr.hrxRC         = ghrxRC;

    bRet = (BOOL)ExtEscape(ghdc, RXFUNCS, sizeof(RXHDR), (LPCSTR) &gmem.rxHdr,
                           0, NULL);

    if (!bRet)
    {
        printf("RXCMD_DELETE_RESOURCE failed\n");
    }

    prxHdr = (RXHDR*) ajMapMem;
    prxHdr->flags           = RX_FL_MAP_MEM;
    prxHdr->hrxRC           = 0;
    prxHdr->hrxSharedMem    = 0;

    prxMapMem = (RXMAPMEM*) (prxHdr + 1);
    prxMapMem->command      = RXCMD_MAP_MEM;
    prxMapMem->action       = RX_DELETE_MEM_MAP;
    prxMapMem->hrxSharedMem = gmem.rxHdr.hrxSharedMem;

    if (!ExtEscape(ghdc, RXFUNCS,
                   DIFF(prxMapMem + 1, ajMapMem), (LPCSTR) ajMapMem,
                   0, NULL))
    {
        printf("RXCMD_MAP_MEM RX_DELETE_MEM_MAP failed\n");
    }

    if (!UnmapViewOfFile(gmem.pvBuf))
    {
        printf("Couldn't UnmapViewOfFile\n");
    }

    if (!CloseHandle(gmem.hSharedMem))
    {
        printf("Couldn't CloseHandle\n");
    }

    return bRet;
}

BOOL setup3dDDI()
{
    HWND hwnd;
    ULONG iRXFuncs;
    RXGETINFO *prxGetInfo;
    BYTE ajCreateContext[sizeof(RXHDR) + sizeof(RXCREATECONTEXT)];
    RXHDR *prxHdr;
    RXCREATECONTEXT *prxCreateContext;
    RXSETSTATE *prxSetState;
    RXCAPS rxIntLineCaps;
    RXCAPS rxCaps;

    // Check if the driver supports any 3D DDI rendering extensions.

    iRXFuncs = RXFUNCS;
    if (!ExtEscape(ghdc, QUERYESCSUPPORT, sizeof(iRXFuncs), (LPCSTR) &iRXFuncs,
                                         0, NULL))
    {
        printf("QUERYESCSUPPORT failed -- driver doesn't support 3D DDI\n");
        goto EarlyOut;
    }

    // Create shared-memory section and header template for doing all our
    // miscellaneous commands:

    gmem.pvBuf = createSection(ghdc, 256, &gmem.rxHdr.hrxSharedMem, &gmem.hSharedMem);
    if (!gmem.pvBuf)
    {
        printf("Couldn't create shared section.  Failing...");
        return(FALSE);
    }

    // Now that we know it supports the 3D DDI rendering extensions, query
    // whether it does integer lines in either colour-index or RGBA modes:

    prxGetInfo = gmem.pvBuf;
    prxGetInfo->command        = RXCMD_GET_INFO;
    prxGetInfo->infoType       = RX_INFO_INTLINE_CAPS;
    prxGetInfo->flags          = RX_QUERY_CURRENT_MODE | RX_GET_INFO_COLOR_INDEX;

    // If we succeed with RX_GET_INFO_COLOR_INDEX, we'll use colour-index
    // mode:

    gflColorMode = RX_COLOR_INDEXED;

    gmem.rxHdr.hrxRC           = 0;
    gmem.rxHdr.sharedMemSize   = sizeof(RXGETINFO);
    gmem.rxHdr.pSharedMem      = gmem.pvBuf;
    gmem.rxHdr.flags           = 0;

    if (!ExtEscape(ghdc, RXFUNCS, sizeof(RXHDR),         (LPCSTR) &gmem.rxHdr,
                   sizeof(rxIntLineCaps), (LPSTR) &rxIntLineCaps))
    {
        // Since it doesn't do color-index, see if it will do RGBA in this
        // mode:

        prxGetInfo->flags = RX_QUERY_CURRENT_MODE;
        gflColorMode      = 0;

        if (!ExtEscape(ghdc, RXFUNCS, sizeof(RXHDR), (LPCSTR) &gmem.rxHdr,
                       sizeof(rxIntLineCaps), (LPSTR) &rxIntLineCaps))
        {
            // What, the display driver doesn't support either RGBA or
            // color-index in this mode?

            printf("RXCMD_GET_INFO RX_INFO_INTLINE_CAPS failed.\n");
            printf("(Driver doesn't support integer lines.)\n");
            goto EarlyOut;
        }
    }

    // Now create a rendering context for our window.  I prefer colour-index
    // to RGBA mode because I don't want to set up the palette :

    hwnd = WindowFromDC(ghdc);
    if (hwnd == 0)
    {
        printf("Null hwnd\n");
        goto EarlyOut;
    }

    prxHdr = (RXHDR*) ajCreateContext;
    prxHdr->flags        = RX_FL_CREATE_CONTEXT;
    prxHdr->hrxRC        = 0;
    prxHdr->hrxSharedMem = 0;

    prxCreateContext = (RXCREATECONTEXT*) (prxHdr + 1);
    prxCreateContext->command   = RXCMD_CREATE_CONTEXT;
    prxCreateContext->hwnd      = (ULONG) hwnd;
    prxCreateContext->flags     = gflColorMode;

    ghrxRC = (RXHANDLE) ExtEscape(ghdc, RXFUNCS,
                                  sizeof(RXHDR) + sizeof(RXCREATECONTEXT),
                                  (LPCSTR) prxHdr, 0, NULL);
    if (ghrxRC == 0)
    {
        printf("RXCMD_CREATE_CONTEXT failed\n");
        goto EarlyOut;
    }

    //////////////////////////////////////////////////////////////////////
    // Set some state for the Rendering Context

    // I want to use last-pel inclusion on lines:

    prxSetState = gmem.pvBuf;
    prxSetState->command       = RXCMD_SET_STATE;
    prxSetState->stateToChange = RX_LAST_PIXEL;
    prxSetState->newState[0]   = TRUE;

    gmem.rxHdr.hrxRC         = ghrxRC;
    gmem.rxHdr.sharedMemSize = sizeof(RXSETSTATE);

    if (!ExtEscape(ghdc, RXFUNCS, sizeof(RXHDR), (LPCSTR) &gmem.rxHdr, 0, NULL))
    {
        printf("RX_SET_STATE RX_LAST_PIXEL failed\n");
        goto CleanUp;
    }

    return(TRUE);

CleanUp:

    close3dDDI();
    ghrxRC = NULL;

EarlyOut:

    return(FALSE);
}

BOOL bInitTest(HWND hwnd)
{
    BOOL bRet;

    bRet = setup3dDDI();

    return bRet;
}

BOOL bCloseTest(HWND hwnd)
{
    BOOL bRet;

    bRet = close3dDDI();

    freeBuffers();

    return bRet;
}

BOOL bInitDrawing(PCHAR pfname)
{
    HFILE hfile;

    pFileName = pfname;

    hfile = _lopen(pfname, OF_READ);

    if (hfile == HFILE_ERROR)
        return FALSE;

    _lclose(hfile);
    return TRUE;
}

void vRedraw(HWND hwnd)
{
    doRedraw();
}

void vClippedRedraw(HWND hwnd, RECT* prc)
{
    RXSETSTATE* prxSetState;
    RXRECT*     prxRect;

    prxSetState = gmem.pvBuf;
    prxSetState->command        = RXCMD_SET_STATE;
    prxSetState->stateToChange  = RX_SCISSORS_ENABLE;
    prxSetState->newState[0]    = TRUE;

    prxSetState++;
    prxSetState->command        = RXCMD_SET_STATE;
    prxSetState->stateToChange  = RX_SCISSORS_RECT;

    prxRect = (RXRECT*) &prxSetState->newState[0];
    prxRect->x                  = prc->left;
    prxRect->y                  = prc->top;
    prxRect->width              = prc->right - prc->left;
    prxRect->height             = prc->bottom - prc->top;

    gmem.rxHdr.hrxRC         = ghrxRC;
    gmem.rxHdr.sharedMemSize = DIFF(prxRect + 1, gmem.pvBuf);

    if (!ExtEscape(ghdc, RXFUNCS, sizeof(RXHDR), (LPCSTR) &gmem.rxHdr,
                                  0, NULL))
    {
        printf("RX_SET_STATE clipping failed\n");
    }

    vRedraw(hwnd);

    prxSetState = gmem.pvBuf;
    prxSetState->command        = RXCMD_SET_STATE;
    prxSetState->stateToChange  = RX_SCISSORS_ENABLE;
    prxSetState->newState[0]    = FALSE;

    gmem.rxHdr.hrxRC = ghrxRC;
    gmem.rxHdr.sharedMemSize = sizeof(RXSETSTATE);

    if (!ExtEscape(ghdc, RXFUNCS, sizeof(RXHDR), (LPCSTR) &gmem.rxHdr,
                                  0, NULL))
    {
        printf("RX_SET_STATE reset clipping failed\n");
    }
}

void vResize(HWND hwnd)
{
    RECT rect;

    GetClientRect(hwnd, &rect);

    freeBuffers();

    vReadSlide(pFileName, rect.right, rect.bottom);
    if (gpCmdFirst && ghrxRC) {
        vRedraw(hwnd);
        ValidateRect(hwnd, &rect);
    }
}

void vTest(HWND hwnd)
{
    DWORD tick_count;
    RECT rect;
    int reps;

    GetClientRect(hwnd, &rect);

    tick_count = GetTickCount();

    for (reps = 0; reps < 10; reps++) {
        doRedraw();
    }

    tick_count = GetTickCount() - tick_count;

    printf("%d lines/sec\n", (int)(((float)totalLines * 10.0) /
           ((float)tick_count / 1000.0)));
    printf("%d lines/redraw\n", totalLines);

}

