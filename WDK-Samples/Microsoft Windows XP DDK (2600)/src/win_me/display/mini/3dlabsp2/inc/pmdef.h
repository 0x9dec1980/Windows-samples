/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: pmdef.h
 * This file containes defines for values that are filled into fields on Permedia
 * The file glintdef.h is the equivalent for glint chips.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/


// These defines are typically used in conjunction with the macros in bitmacro.h, 
// which shift the values to their correct locations.

#ifdef __PMDEF
#pragma message ("FILE : "__FILE__" : Multiple Inclusion");
#endif

#define __PMDEF

// Texture unit bit fields
// Texture color mode
#define PM_TEXCOLORMODE_ENABLE 0
#define PM_TEXCOLORMODE_APPLICATION 1
#define PM_TEXCOLORMODE_TEXTURETYPE 4

// Texture address mode
#define PM_TEXADDRESSMODE_ENABLE 0
#define PM_TEXADDRESSMODE_PERSPECTIVE 1
#define PM_TEXADDRESSMODE_FAST 2

// Texture map format
#define PM_TEXMAPFORMAT_PP0 0
#define PM_TEXMAPFORMAT_PP1 3
#define PM_TEXMAPFORMAT_PP2 6
#define PM_TEXMAPFORMAT_TEXELSIZE 19

// Texture data format
#define PM_TEXDATAFORMAT_ALPHAMAP_EXCLUDE 2
#define PM_TEXDATAFORMAT_ALPHAMAP_INCLUDE 1
#define PM_TEXDATAFORMAT_ALPHAMAP_DISABLE 0

#define PM_TEXDATAFORMAT_FORMAT 0
#define PM_TEXDATAFORMAT_FORMATEXTENSION 6
#define PM_TEXDATAFORMAT_COLORORDER 5

// Dither unit bit fields
#define PM_DITHERMODE_ENABLE 0
#define PM_DITHERMODE_DITHERENABLE 1
#define PM_DITHERMODE_COLORFORMAT 2
#define PM_DITHERMODE_XOFFSET 6
#define PM_DITHERMODE_YOFFSET 8
#define PM_DITHERMODE_COLORORDER 10
#define PM_DITHERMODE_DITHERMETHOD 11
#define PM_DITHERMODE_FORCEALPHA 12
#define PM_DITHERMODE_COLORFORMATEXTENSION 16

// Window register
#define PM_WINDOW_LBUPDATESOURCE_LBSOURCEDATA 0
#define PM_WINDOW_LBUPDATESOURCE_REGISTERS 1

// Texture unit YUV mode
#define PM_YUVMODE_CHROMATEST_DISABLE		0
#define PM_YUVMODE_CHROMATEST_PASSWITHIN	1
#define PM_YUVMODE_CHROMATEST_FAILWITHIN	2
#define PM_YUVMODE_TESTDATA_INPUT	0
#define PM_YUVMODE_TESTDATA_OUTPUT	1

