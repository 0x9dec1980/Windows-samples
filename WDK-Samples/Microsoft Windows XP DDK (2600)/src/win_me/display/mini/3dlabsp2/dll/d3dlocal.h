/******************************Module*Header**********************************\
*
*       **************************************
*       * Permedia 2: Direct 3D  SAMPLE CODE *
*       **************************************
*
* Module Name: d3dlocal.h
*
*       Contains prototypes for all Direct 3D related functions which are
*       hardware independant and not callbacks.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef _DLL_D3DFN_H_
#define _DLL_D3DFN_H_


/*
 * defined in d3dutil.c
 */

D3DDEVICEDESC_V1 *
GetHardwareD3DCapabilities(LPP2THUNKEDDATA pThisDisplay);

/*
 *defined in d3dstate.c
 */

DWORD 
ProcessRenderStates(PERMEDIA_D3DCONTEXT *pContext, 
                    DWORD               dwCount,
                    LPD3DSTATE          lpState,
                    LPDWORD             lpStateMirror);

VOID 
MapRenderStateToTextureStage0State(PERMEDIA_D3DCONTEXT   *pContext,
                                   DWORD                 dwRSType,
                                   DWORD                 dwRSVal);
#if D3D_STATEBLOCKS

void 
DumpStateSet(P2StateSetRec      *pSSRec);

VOID 
__DeleteAllStateSets(PERMEDIA_D3DCONTEXT    *pContext);

VOID 
__BeginStateSet(PERMEDIA_D3DCONTEXT     *pContext, 
                DWORD                   dwParam);

VOID 
__EndStateSet(PERMEDIA_D3DCONTEXT       *pContext);

VOID 
__DeleteStateSet(PERMEDIA_D3DCONTEXT    *pContext, 
                 DWORD                  dwParam);

VOID 
__ExecuteStateSet(PERMEDIA_D3DCONTEXT   *pContext,
                  DWORD                 dwParam);

VOID 
__CaptureStateSet(PERMEDIA_D3DCONTEXT   *pContext, 
                  DWORD                 dwParam);

VOID 
__SelectFVFTexCoord(LPP2FVFOFFSETS      lpP2FVFOff, 
                    DWORD               dwTexCoord);

#endif //#if D3D_STATEBLOCKS

VOID 
PreProcessTextureStageState(PERMEDIA_D3DCONTEXT   *pContext, 
                            DWORD                 dwStage, 
                            DWORD                 dwState, 
                            DWORD                 dwValue);
/*
 * defined in d3dresrc.c
 */
LPDWLIST     
GetSurfaceHandleList(LPVOID);


VOID
RemoveDirectDrawLocal(LPVOID);

PPERMEDIA_D3DPALETTE 
PaletteHandleToPtr(UINT_PTR            phandle,
                   PERMEDIA_D3DCONTEXT *pContext);

PPERMEDIA_D3DTEXTURE 
TextureHandleToPtr(UINT_PTR             thandle,
                   PERMEDIA_D3DCONTEXT  *pContext);

VOID                
AddContextToHandleList(PERMEDIA_D3DCONTEXT *pContext);

VOID                
DeleteContextFromHandleList(PERMEDIA_D3DCONTEXT *pContext);

DWORD CreateSurfaceHandle(LPP2THUNKEDDATA            ppdev,
                          LPVOID                     pDDLcl,
                          LPDDRAWI_DDRAWSURFACE_LCL  lpDDSLcl);
VOID 
RemoveSurfaceHandle(LPVOID  lpDDLcl,
                    DWORD   dwSurfaceHandle);
VOID
RemoveManagedSurface(LPVOID   lpDDLcl, 
                     DWORD    dwSurfaceHandle);
BOOL
LockManagedSurface(LPVOID   lpDDLcl,
                   DWORD    dwSurfaceHandle);

VOID
MarkManagedSurfaceNeedsUpdate(LPVOID   lpDDLcl, 
                              DWORD    dwSurfaceHandle);

VOID
DirtyContextWithThisRenderTarget(LPVOID   lpDDLcl, 
                                 DWORD    dwSurfaceHandle);

VOID                
ClearManagedSurface(LPDDRAWI_DDRAWSURFACE_LCL pDestLcl,
                    RECTL                     *rDest,
                    DWORD                     dwColor);
HRESULT 
__PaletteSet(PERMEDIA_D3DCONTEXT    *pContext,
             DWORD                  dwSurfaceHandle,
             DWORD                  dwPaletteHandle,
             DWORD                  dwPaletteFlags);

HRESULT 
__PaletteUpdate(PERMEDIA_D3DCONTEXT *pContext,
                DWORD               dwPaletteHandle, 
                WORD                wStartIndex, 
                WORD                wNumEntries,
                BYTE                *pPaletteData);

/*
 * defined in d3d.c
 */

DWORD CreateSurfaceHandleLoop( LPP2THUNKEDDATA              ppdev,
                                LPVOID                      pDDLcl,
                                LPDDRAWI_DDRAWSURFACE_LCL   lpDDSLclroot,
                                LPDDRAWI_DDRAWSURFACE_LCL   lpDDSLcl);

HRESULT
ValidateRenderTargetAndZBuffer(PERMEDIA_D3DCONTEXT    *pContext);

#endif /* #ifdef _DLL_D3DFN_H_ */