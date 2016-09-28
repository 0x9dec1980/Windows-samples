;****************************************************************************
;                                                                           *
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
; PURPOSE.                                                                  *
;                                                                           *
; Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
;                                                                           *
;****************************************************************************

ifdef WIN31COMPAT
MEM_POOL_PAGES	EQU	3
else
MEM_POOL_PAGES	EQU	2
endif
BIG_MEM_POOL_PAGES	EQU	16
IOS_DOS_PAGER_MEM_SUP	EQU	<PAGE_SIZE*14>
MAX_POOL_PAGES	EQU	<10h>
PAGE_SIZE	EQU	1000H
PAGE_SHIFT	EQU	12
MAX_SG_ELEMENT_SIZE	EQU	0FFFFH
MAX_LOGICAL_DRIVES	EQU	26
ifdef	NEC_98
MAX_PHYSICAL_DRIVES	EQU	<48h>
else
MAX_PHYSICAL_DRIVES	EQU	24
endif
MAX_REG_UNITS_PER_DEVICE	EQU	16
MAX_PHYS_FLOPPY_DRIVES	EQU	8
MAX_IRQ	EQU	32
MAX_IRQ_LEVEL	EQU	31
IOS_LOG_NAME	EQU	<"IOSLOG$">
LOG_BUF_CNT	EQU	10
BIG_MEM_THRESHOLD	EQU	0FFFFFFH
DEFAULT_BIG_MEM_PAGES	EQU	16
BIG_IO_SIZE	EQU	<0FFFFh>
