/******************************Module*Header**********************************\
*
*                           ***************************************
*                           * Permedia 2: Direct 3D   SAMPLE CODE *
*                           ***************************************
*
* Module Name: p2d3d.h
*
*       Contains prototypes for Direct 3D related Permedia 2 
*       hardware specific functions.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef _DLL_P2D3D_H_
#define _DLL_P2D3D_H_

#ifdef SUPPORTING_MONOFLAG
#define RENDER_MONO (Flags & CTXT_HAS_MONO_ENABLED)
#else
#define RENDER_MONO 0
#endif
//
// Hardware primitive setup functions
//

typedef void (D3DFVFDRAWTRIFUNC)(PERMEDIA_D3DCONTEXT *, 
                                 LPD3DTLVERTEX, 
                                 LPD3DTLVERTEX,
                                 LPD3DTLVERTEX, 
                                 LPP2FVFOFFSETS);
typedef void (D3DFVFDRAWPNTFUNC)(PERMEDIA_D3DCONTEXT *, 
                                 LPD3DTLVERTEX, 
                                 LPP2FVFOFFSETS);

typedef D3DFVFDRAWTRIFUNC *D3DFVFDRAWTRIFUNCPTR;


typedef D3DFVFDRAWPNTFUNC *D3DFVFDRAWPNTFUNCPTR;

/*
 * definition in p2d3dpt.c
 */
D3DFVFDRAWPNTFUNCPTR    
HWSetPointFunc( PERMEDIA_D3DCONTEXT *pContext,
                LPP2FVFOFFSETS      lpP2FVFOff);

/*
 * definition in p2d3dtri.c
 */
D3DFVFDRAWTRIFUNCPTR    
HWSetTriangleFunc(  PERMEDIA_D3DCONTEXT *pContext);

/*
 * definition in p2d3dln.c
 */
void 
P2DrawFVFLine(  PERMEDIA_D3DCONTEXT *pContext, 
                LPD3DTLVERTEX       lpV0, 
                LPD3DTLVERTEX       lpV1,
                LPD3DTLVERTEX       lpVFlat, 
                LPP2FVFOFFSETS      lpFVFOff);


/*
 * definitions in p2d3dhw.c
 */

void
SetupP2RenderStates(PERMEDIA_D3DCONTEXT   *pContext,
                    DWORD                 dwRSType,
                    DWORD                 dwRSVal);


VOID 
HWInitContext(PERMEDIA_D3DCONTEXT   *pContext);

VOID 
SetupPermediaRenderTarget(PERMEDIA_D3DCONTEXT   *pContext);

void 
__HandleDirtyPermediaState(PERMEDIA_D3DCONTEXT  *pContext,
                           LPP2FVFOFFSETS       lpP2FVFOff);
VOID 
StorePermediaLODLevel(LPP2THUNKEDDATA           ppdev, 
                      PERMEDIA_D3DTEXTURE       *pTexture, 
                      LPDDRAWI_DDRAWSURFACE_LCL pSurf, 
                      int                       LOD);
void LoadManagedTextureToVidMem(LPP2THUNKEDDATA      pThisDisplay,
                                PERMEDIA_D3DTEXTURE  *pTexture);

#endif /* #ifndef _DLL_P2D3D_H_ */
