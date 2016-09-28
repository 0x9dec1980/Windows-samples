/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: context.h
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#pragma pack(2)

typedef struct _GlintReadRegs
{
        WORD    wNumRegs;
        WORD    wFirstReg;
} GLREADREGS, *LPGLREADREGS;

typedef struct _GlintContext
{
        WORD   hContext;
        GLREADREGS ReadRegs;
} GLCONTEXT, *LPGLCONTEXT;

#pragma pack()
