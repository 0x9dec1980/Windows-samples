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

AUDIO_PAUSED		equ	0001H		; audio output is paused


PVDAudio		struc

  PVD_BlkSize	DD	?	;  Total Length of data block (including hdr)
  PVD_Instance	DW	?	;  Block Instance ID
  PVD_Group	DW	?	;  Block Group ID
  PVD_HdrFlags	DD	?	;  Flags
  PVD_Semaphore	DD	?	;  Semaphore or Semaphore handle
  PVD_TimerAdd	DD	?	;  Ptr to timer Addition function
  PVD_TimerSub	DD	?	;  Ptr to timer Subtract function

  RequestCnt		dd	0
  StartedCnt		dd	0
  CompletionCnt		dd	0
PVDAudio		ends

PVDataCounterSize	equ	size PVDAudio - RequestCnt


VSDDATA	struc
			db	size DDB dup(?)

  VSD_flags		dd	?

  PVData		db	size PVDAudio dup (?)
				 
VSDDATA	ends
