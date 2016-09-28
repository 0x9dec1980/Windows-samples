/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
*                                                                           *
****************************************************************************/

/** posthost.h                                                          **/

#ifndef _POSTHOST_H_
#define _POSTHOST_H_


/* misc. defines */
    #define WM_POSTHOSTPOST     (WM_USER + 1997)


/* error values as returned by phGetPostHostError() */
    #define PHERR_NOERROR       0
    #define PHERR_NO386ENH      1
    #define PHERR_NOVPOSTD      2


/* public function prototypes */
    WORD FAR PASCAL phGetPostHostError( void );
    DWORD FAR PASCAL phGetVPOSTDAPIAddr( void );
    WORD FAR PASCAL phGetVPOSTDVersion( void );
    BOOL FAR PASCAL phRegisterWindow( HWND hWnd );
    BOOL FAR PASCAL phCallPostHost( DWORD lParam );

#endif

/** EOF: posthost.h **/
