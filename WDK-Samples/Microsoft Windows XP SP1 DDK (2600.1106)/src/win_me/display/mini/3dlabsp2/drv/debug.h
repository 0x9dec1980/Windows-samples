;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: DEBUG.H
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

#ifndef _DRV_DEBUG_H_
#define _DRV_DEBUG_H_

#ifdef DEBUG

extern void FAR __cdecl DPF(LPSTR szFormat, ...);
extern void DebugPrint(int Value, LPSTR szFormat, ... );
#else  /* DEBUG */
#define DPF 	1 ? (void)0 : (void)
#endif /* DEBUG */

#endif /*_DRV_DEBUG_H_ */