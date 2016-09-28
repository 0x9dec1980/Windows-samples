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

BPB	STRUC
BPB_BytesPerSector	DW	?
BPB_SectorsPerCluster	DB	?
BPB_ReservedSectors	DW	?
BPB_NumberOfFATs	DB	?
BPB_RootEntries	DW	?
BPB_TotalSectors	DW	?
BPB_MediaDescriptor	DB	?
BPB_SectorsPerFAT	DW	?
BPB_SectorsPerTrack	DW	?
BPB_Heads	DW	?
BPB_HiddenSectors	DW	?
BPB_HiddenSectorsHigh	DW	?
BPB_BigTotalSectors	DW	?
BPB_BigTotalSectorsHigh	DW	?
BPB	ENDS

A_BPB	STRUC
A_BPB_BytesPerSector	DW	?
A_BPB_SectorsPerCluster	DB	?
A_BPB_ReservedSectors	DW	?
A_BPB_NumberOfFATs	DB	?
A_BPB_RootEntries	DW	?
A_BPB_TotalSectors	DW	?
A_BPB_MediaDescriptor	DB	?
A_BPB_SectorsPerFAT	DW	?
A_BPB_SectorsPerTrack	DW	?
A_BPB_Heads	DW	?
A_BPB_HiddenSectors	DW	?
A_BPB_HiddenSectorsHigh	DW	?
A_BPB_BigTotalSectors	DW	?
A_BPB_BigTotalSectorsHigh	DW	?
A_BPB_Reserved	DB	6 DUP (?)
A_BPB	ENDS
