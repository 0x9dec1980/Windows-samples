/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: p2vidreg.h
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/
#ifndef __P2VIDREG_H_
#define __P2VIDREG_H_

// VS Configuration
#if BIG_ENDIAN == 1
typedef struct 
{
	unsigned long Reserved			: 3;
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
	unsigned long ReverseDataA		: 1;
	unsigned long InterlaceA		: 1;
	unsigned long VActiveVBIA		: 1;
	unsigned long FieldEdgeA		: 1;
	unsigned long FieldPolarityA	: 1;
	unsigned long UseFieldA			: 1;
	unsigned long VActivePolarityA	: 1;
	unsigned long VRefPolarityA		: 1;
	unsigned long HRefPolarityA		: 1;
	unsigned long ROMPulse			: 5;
	unsigned long GPModeA			: 1;
	unsigned long UnitMode			: 3;
} __PermediaVSConfiguration;
#else
typedef struct 
{
	unsigned long UnitMode			: 3;
	unsigned long GPModeA			: 1;
	unsigned long ROMPulse			: 5;
	unsigned long HRefPolarityA		: 1;
	unsigned long VRefPolarityA		: 1;
	unsigned long VActivePolarityA	: 1;
	unsigned long UseFieldA			: 1;
	unsigned long FieldPolarityA	: 1;
	unsigned long FieldEdgeA		: 1;
	unsigned long VActiveVBIA		: 1;
	unsigned long InterlaceA		: 1;
	unsigned long ReverseDataA		: 1;
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
	unsigned long Reserved			: 3;
} __PermediaVSConfiguration;
#endif 

// VSB Control
#if BIG_ENDIAN == 1
typedef struct 
{
	unsigned long Reserved			: 18;
	unsigned long LockToStreamA		: 1;
	unsigned long GammaCorrect		: 1;
	unsigned long RGB				: 1;
	unsigned long PixelSize			: 2;
	unsigned long ColorFormat		: 5;
	unsigned long CombineFields		: 1;
	unsigned long BufferCtrl		: 1;
	unsigned long VBI				: 1;
	unsigned long Video				: 1;
} __PermediaVSBControl;
#else
typedef struct 
{
	unsigned long Video				: 1;
	unsigned long VBI				: 1;
	unsigned long BufferCtrl		: 1;
	unsigned long CombineFields		: 1;
	unsigned long ColorFormat		: 5;
	unsigned long PixelSize			: 2;
	unsigned long RGB				: 1;
	unsigned long GammaCorrect		: 1;
	unsigned long LockToStreamA		: 1;
	unsigned long Reserved			: 18;
} __PermediaVSBControl;
#endif 

// VSB Fifo Control
#if BIG_ENDIAN == 1
typedef struct 
{
	unsigned long Reserved					: 16;
	unsigned long HighPriorityThreshold		: 8;
	unsigned long LowPriorityThreshold		: 8;
} __PermediaVSBFifoControl;
#else
typedef struct 
{
	unsigned long LowPriorityThreshold		: 8;
	unsigned long HighPriorityThreshold		: 8;
	unsigned long Reserved					: 16;
} __PermediaVSBFifoControl;
#endif 

#endif //__P2VIDREG_H_
