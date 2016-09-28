/******************************Module*Header**********************************\
*
*       **************************************************
*       * Permedia 2: Direct 3D  SAMPLE CODE *
*       **************************************************
*
* Module Name: d3dcb.h
*
*       Contains D3D function callbacks.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef _DLL_D3DCB_H_
#define _DLL_D3DCB_H_

//
// Driver callbacks
//
/*
 * defined in d3d.c
 */

VOID 
D3DHALCreateDriver(LPP2THUNKEDDATA              pThisDisplay, 
                   LPD3DHAL_GLOBALDRIVERDATA    *lpD3DGlobalDriverData,
                   LPD3DHAL_CALLBACKS           *lpD3DHALCallbacks);

DWORD CALLBACK 
D3DValidateTextureStageState( LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA lpvtssd );

DWORD CALLBACK 
D3DGetDriverState( LPDDHAL_GETDRIVERSTATEDATA);

DWORD CALLBACK 
D3DCreateSurfaceEx( LPDDHAL_CREATESURFACEEXDATA);

DWORD CALLBACK 
D3DDestroyDDLocal( LPDDHAL_DESTROYDDLOCALDATA);

DWORD CALLBACK 
DdSetColorKey(LPDDHAL_SETCOLORKEYDATA psckd);

DWORD CALLBACK 
D3DContextCreate(LPD3DHAL_CONTEXTCREATEDATA pccd);

DWORD CALLBACK 
D3DContextDestroy(LPD3DHAL_CONTEXTDESTROYDATA pcdd);

/*
 * defined in d3ddp2.c
 */
DWORD CALLBACK 
D3DDrawPrimitives2( LPD3DHAL_DRAWPRIMITIVES2DATA pd );


#endif //#ifndef _DLL_D3DCB_H_
