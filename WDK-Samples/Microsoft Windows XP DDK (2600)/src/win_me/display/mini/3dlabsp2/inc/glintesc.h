/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: glintesc.h
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/


#pragma pack(2)

typedef struct _GlintEscape
{
    WORD        wEscapeCode;
    union
    {
        DWORD       dwParam;
        GLDMAREQ    DmaReq;
        GLREADREGS  ReadRegs;
    } EscParam;
} GLESC, *LPGLESC;

#pragma pack()

#define GLINT_ESCAPE                   0x3d3d
#define GLESC_NEWCONTEXT               0x0001
#define GLESC_DELETECONTEXT            0x0002
#define GLESC_GETDMABUFFERS            0x0003
#define GLESC_FREEDMABUFFERS           0x0004
#define GLESC_STARTDMA                 0x0005
#define GLESC_WAITDMA                  0x0006
#define GLESC_GETDMASEQUENCE           0x0007
#define GLESC_SWITCHCONTEXT            0x0008

