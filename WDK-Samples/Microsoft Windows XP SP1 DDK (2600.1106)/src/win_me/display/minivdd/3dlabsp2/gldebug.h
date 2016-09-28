/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: gldebug.h
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#if DEBUG

extern LONG VDD_DebugLevel;

#define DISPASSERT(expr) if( !(expr) ) { __asm int 1 }

#define DISPDBG(arg) { _Debug_Printf_Service arg ; _Debug_Printf_Service("\n"); }

#define DISPCHIP(arg) { long TempValue = 1;                     \
    switch (arg->ChipID) {                                      \
	    case GLINT500TX_ID:                                     \
		    _Debug_Printf_Service("  Glint 500TX Chip   ");     \
		    break;                                              \
	    case GLINTMX_ID:                                        \
		    _Debug_Printf_Service("  Glint MX Chip      ");     \
		    break;                                              \
	    case DELTA_ID:                                          \
		    _Debug_Printf_Service("  DELTA Chip         ");     \
		    break;                                              \
	    case PERMEDIA_ID:                                       \
		    _Debug_Printf_Service("  IBM PERMEDIA Chip  ");     \
		    break;                                              \
	    case TIPERMEDIA_ID:                                     \
		    _Debug_Printf_Service("  TI PERMEDIA Chip   ");     \
		    break;                                              \
	    case PERMEDIA2_ID:                                      \
		    _Debug_Printf_Service("  IBM PERMEDIA2 Chip ");     \
		    break;                                              \
	    case TIPERMEDIA2_ID:                                    \
		    _Debug_Printf_Service("  TI PERMEDIA2 Chip  ");     \
		    break;                                              \
        default:                                                \
            TempValue = 0;                                      \
    }                                                           \
    if (TempValue) DISPDBG(("Devnode 0x%x, pDev 0x%x", arg->DevNode, arg));    \
}

#if DEBUG_RAMDAC
#define VideoDebugPrint(arg) _Debug_Printf_Service arg
#else
#define VideoDebugPrint(arg)
#endif

// If we are not in a debug environment, we want all of the debug
// information to be stripped out.

#else

#define DISPASSERT(expr)
#define DISPDBG(arg)
#define DISPCHIP(arg)
#define VideoDebugPrint(arg)

#endif
