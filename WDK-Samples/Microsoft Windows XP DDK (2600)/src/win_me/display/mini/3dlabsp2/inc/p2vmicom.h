/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: p2vmicom.h
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/
#ifndef __P2VMICOM_H_
#define __P2VMICOM_H_

#define P2_VSBPartialConfig			0xD0
#define P2_VSBControl				0xD1
#define P2_VSBInterruptLine			0xD2
#define P2_VSBCurrentLine			0xD3
#define P2_VSBVideoAddressHost		0xD4
#define P2_VSBVideoAddressIndex		0xD5
#define P2_VSBVideoAddress0			0xD6
#define P2_VSBVideoAddress1			0xD7
#define P2_VSBVideoAddress2			0xD8
#define P2_VSBVideoStride			0xD9
#define P2_VSBVideoStartLine		0xDA
#define P2_VSBVideoEndLine			0xDB
#define P2_VSBVideoStartData		0xDC
#define P2_VSBVideoEndData			0xDD
#define P2_VSBVBIAddressHost		0xDE
#define P2_VSBVBIAddressIndex		0xDF
#define P2_VSBVBIAddress0			0xE0
#define P2_VSBVBIAddress1			0xE1
#define P2_VSBVBIAddress2			0xE2
#define P2_VSBVBIStride				0xE3
#define P2_VSBVBIStartLine			0xE4
#define P2_VSBVBIEndLine			0xE5
#define P2_VSBVBIStartData			0xE6
#define P2_VSBVBIEndData			0xE7
#define P2_VSBFifoControl			0xE8

// STREAM B
// Sections from VSConfiguration
#if BIG_ENDIAN == 1
typedef struct tagPermedia2Reg_VSPartialConfigB
{
	unsigned long Reserved			: 19;
	unsigned long DoubleEdgeB		: 1;
	unsigned long ReverseDataB		: 1;
	unsigned long ColorSpaceB		: 1;
	unsigned long InterlaceB		: 1;
	unsigned long VActiveVBIB		: 1;
	unsigned long FieldEdgeB		: 1;
	unsigned long FieldPolarityB	: 1;
	unsigned long UseFieldB			: 1;
	unsigned long VActivePolarityB	: 1;
	unsigned long VRefPolarityB		: 1;
	unsigned long HRefPolarityB		: 1;
	unsigned long WideOutput		: 1;
	unsigned long EnableStreamB		: 1;
} Permedia2Reg_VSPartialConfigB;
#else
typedef struct tagPermedia2Reg_VSPartialConfigB
{
	unsigned long EnableStreamB		: 1;
	unsigned long WideOutput		: 1;
	unsigned long HRefPolarityB		: 1;
	unsigned long VRefPolarityB		: 1;
	unsigned long VActivePolarityB	: 1;
	unsigned long UseFieldB			: 1;
	unsigned long FieldPolarityB	: 1;
	unsigned long FieldEdgeB		: 1;
	unsigned long VActiveVBIB		: 1;
	unsigned long InterlaceB		: 1;
	unsigned long ColorSpaceB		: 1;
	unsigned long ReverseDataB		: 1;
	unsigned long DoubleEdgeB		: 1;
	unsigned long Reserved			: 19;
} Permedia2Reg_VSPartialConfigB;
#endif

// VSB Control
#if BIG_ENDIAN == 1
typedef struct tagPermedia2Reg_VSBControl
{
	unsigned long Reserved			: 18;
	unsigned long LockToStreamA		: 1;
	unsigned long GammaCorrect		: 1;
	unsigned long RGB				: 1;
	unsigned long PixelSize			: 2;
	unsigned long ColorFormat		: 5;
	unsigned long CombineFields		: 1;
	unsigned long BufferCtl			: 1;
	unsigned long EnableVBI			: 1;
	unsigned long EnableVideo		: 1;
} Permedia2Reg_VSBControl;
#else
typedef struct tagPermedia2Reg_VSBControl
{
	unsigned long EnableVideo		: 1;
	unsigned long EnableVBI			: 1;
	unsigned long BufferCtl			: 1;
	unsigned long CombineFields		: 1;
	unsigned long ColorFormat		: 5;
	unsigned long PixelSize			: 2;
	unsigned long RGB				: 1;
	unsigned long GammaCorrect		: 1;
	unsigned long LockToStreamA		: 1;
	unsigned long Reserved			: 18;
} Permedia2Reg_VSBControl;
#endif

#endif // __P2VMICOM_H_