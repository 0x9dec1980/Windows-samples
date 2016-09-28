/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: Direct 3D SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: d3d.c
 *
 * This module contains the Direct 3D Initialization code and D3D callback function
 * entry points, except for DrawPrimitives2 callback.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/
#include "precomp.h"

// D3D callbacks and global data
D3DHAL_GLOBALDRIVERDATA gD3DGlobalDriverData;
D3DHAL_CALLBACKS        gD3DHALCallBacks;

// External Definitions
extern DDSURFACEDESC    gD3DTextureFormats [];
extern ULONG            gD3DNumberOfTextureFormats;
extern LPP2THUNKEDDATA  pDriverData;



/**             
 * Function:    D3DHALCreateDriver
 * Description: Called as part of Direct Draw Initialization.
 *              Returns the Texture Formats supported and 
 *              D3D callbacks supported.
 */

VOID  
D3DHALCreateDriver(LPP2THUNKEDDATA              pThisDisplay, 
                   LPD3DHAL_GLOBALDRIVERDATA    *lpD3DGlobalDriverData,
                   LPD3DHAL_CALLBACKS           *lpD3DHALCallbacks)
{
    D3DDEVICEDESC_V1 *pd3dHWCaps;

    DBG_D3D((6,"Entering D3DHALCreateDriver"));
    // Here we fill in the supplied structures.
    // Can disable D3D HAL in registry if we are in the wrong mode
    if (pThisDisplay->ddpfDisplay.dwRGBBitCount == 8 )
    {   //No D3D HAL support in 8bpp mode
        *lpD3DGlobalDriverData = NULL;
        *lpD3DHALCallbacks = NULL;
        DBG_D3D((0, "D3DHALCreateDriver: Disabled"));
        return;
    }
    // Clear the global data
    memset(&gD3DGlobalDriverData, 0, sizeof(D3DHAL_GLOBALDRIVERDATA));
    gD3DGlobalDriverData.dwSize = sizeof(D3DHAL_GLOBALDRIVERDATA);    
    gD3DGlobalDriverData.dwNumVertices      = 0;    // We don't parse execute buffers
    gD3DGlobalDriverData.dwNumClipVertices  = 0;

    pd3dHWCaps = GetHardwareD3DCapabilities(pThisDisplay);
    memcpy(&gD3DGlobalDriverData.hwCaps, pd3dHWCaps, sizeof(D3DDEVICEDESC_V1));
    gD3DGlobalDriverData.dwNumTextureFormats = gD3DNumberOfTextureFormats;
    gD3DGlobalDriverData.lpTextureFormats = &gD3DTextureFormats[0];

    // Clear the call-backs
    memset(&gD3DHALCallBacks, 0, sizeof(D3DHAL_CALLBACKS));
    gD3DHALCallBacks.dwSize         = sizeof(D3DHAL_CALLBACKS);

    // D3D Context callbacks
    gD3DHALCallBacks.ContextCreate  = D3DContextCreate;
    gD3DHALCallBacks.ContextDestroy = D3DContextDestroy;

    //
    // Return the HAL table.
    //
    *lpD3DGlobalDriverData = &gD3DGlobalDriverData;
    *lpD3DHALCallbacks = &gD3DHALCallBacks;

    DBG_D3D((6,"Exiting D3DHALCreateDriver"));

    return;
}

/**
 *
 * Function:    D3DValidateTextureStageState
 * Description: ValidateTextureStageState evaluates the current state for blending 
 *              operations (including multitexture) and returns the number of passes the 
 *              hardware can do it in. This is a mechanism to query the driver about 
 *              whether it is able to handle the current stage state setup that has been 
 *              set up in hardware.  For example, some hardware cannot do two simultaneous 
 *              modulate operations because they have only one multiplication unit and one 
 *              addition unit.  
 *              The other reason for this function is that some hardware may not map 
 *              directly onto the Direct3D state architecture. This is a mechanism to map 
 *              the hardware's capabilities onto what the Direct3D DDI expects.
 *
 *              Parameters
 *                   lpvtssd
 *                       .dwhContext
 *                            Context handle
 *                       .dwFlags
 *                            Flags, currently set to 0
 *                       .dwReserved
 *                            Reserved
 *                       .dwNumPasses
 *                            Number of passes the hardware can perform the operation in
 *                       .ddrval
 *                            return value
 */

DWORD CALLBACK 
D3DValidateTextureStageState( LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA lpvtssd )
{
    PERMEDIA_D3DTEXTURE *lpTexture;
    PERMEDIA_D3DCONTEXT *pContext;
    DWORD mag, min, cop, ca1, ca2, aop, aa1, aa2;

    DBG_D3D((6,"Entering D3DValidateTextureStageState"));

    if((pContext = (PERMEDIA_D3DCONTEXT*)lpvtssd->dwhContext) == NULL)
    {
        DBG_D3D((0,"D3DValidateTextureStageState invalid context = 0x%x",lpvtssd->dwhContext));        
        lpvtssd->ddrval = D3DHAL_CONTEXT_BAD;
        return (DDHAL_DRIVER_HANDLED); 
    }

    lpvtssd->dwNumPasses = 0;
    lpvtssd->ddrval =  DD_OK;

    mag = pContext->TssStates[D3DTSS_MAGFILTER];
    min = pContext->TssStates[D3DTSS_MINFILTER];
    cop = pContext->TssStates[D3DTSS_COLOROP];
    ca1 = pContext->TssStates[D3DTSS_COLORARG1];
    ca2 = pContext->TssStates[D3DTSS_COLORARG2];
    aop = pContext->TssStates[D3DTSS_ALPHAOP];
    aa1 = pContext->TssStates[D3DTSS_ALPHAARG1];
    aa2 = pContext->TssStates[D3DTSS_ALPHAARG2];

    if (!pContext->TssStates[D3DTSS_TEXTUREMAP])
    {
        lpvtssd->dwNumPasses = 1;
        // Current is the same as diffuse in stage 0
        if (ca2 == D3DTA_CURRENT)
            ca2 = D3DTA_DIFFUSE;
        if (aa2 == D3DTA_CURRENT)
            aa2 = D3DTA_DIFFUSE;

        // Check TSS even with texture handle = 0 since
        // certain operations with the fragments colors might
        // be  possible. Here we only allow plain "classic" rendering

        if ((   (ca1 == D3DTA_DIFFUSE )    && 
                (cop == D3DTOP_SELECTARG1) &&
                (aa1 == D3DTA_DIFFUSE )    &&
                (aop == D3DTOP_SELECTARG1))     ||

            (   (ca2 == D3DTA_DIFFUSE )    && 
                (cop == D3DTOP_SELECTARG2) &&
                (aa2 == D3DTA_DIFFUSE) &&
                (aop == D3DTOP_SELECTARG2))     ||
       
            (   (ca2 == D3DTA_DIFFUSE)   &&     // Default modulation
                (ca1 == D3DTA_TEXTURE)   && 
                (cop == D3DTOP_MODULATE) &&
                (aa1 == D3DTA_TEXTURE)   && 
                (aop == D3DTOP_SELECTARG1))     ||
            
            (   (cop == D3DTOP_DISABLE))        // Check disable
           )
        {
            goto Exit_ValidateTSS;
        }
        else
        {
            goto Fail_Validate;
        }
    }
    else
    if (!( ((mag == D3DTFG_POINT)||(mag == D3DTFG_LINEAR))&&
           ((min == D3DTFG_POINT)||(min == D3DTFG_LINEAR)) ))
    {
        lpvtssd->ddrval = D3DERR_UNSUPPORTEDTEXTUREFILTER;
        DBG_D3D((2,"D3DERR_UNSUPPORTEDTEXTUREFILTER"));
    }
    else
    {
        lpvtssd->dwNumPasses = 1;

        // Current is the same as diffuse in stage 0
        if (ca2 == D3DTA_CURRENT)
            ca2 = D3DTA_DIFFUSE;
        if (aa2 == D3DTA_CURRENT)
            aa2 = D3DTA_DIFFUSE;

        
        if ((ca1 == D3DTA_TEXTURE )    &&          // Check decal  
            (cop == D3DTOP_SELECTARG1) &&
            (aa1 == D3DTA_TEXTURE)     && 
            (aop == D3DTOP_SELECTARG1))
        {
            goto Exit_ValidateTSS;
        }
        // Check all modulate variations
        else if ((ca2 == D3DTA_DIFFUSE)   && 
                 (ca1 == D3DTA_TEXTURE)   && 
                 (cop == D3DTOP_MODULATE))
        {
            if (
                // legacy (DX5) mode
                ((aa1 == D3DTA_TEXTURE)   && 
                (aop == D3DTOP_LEGACY_ALPHAOVR)) ||
                // modulate color & pass diffuse alpha
                ((aa2 == D3DTA_DIFFUSE)   && 
                     (aop == D3DTOP_SELECTARG2))
               )

            {   // Get Texture for current stage (0) to verify properties
                lpTexture = TextureHandleToPtr(
                                    pContext->TssStates[D3DTSS_TEXTUREMAP],
                                    pContext);

                if (NULL == lpTexture)
                {
                    // we're lacking key information about the texture
                    DBG_D3D((0,"D3DValidateTextureStageState gets "
                               "NULL == lpTexture"));
                    lpvtssd->ddrval = D3DERR_WRONGTEXTUREFORMAT;
                    lpvtssd->dwNumPasses = 0;
                    goto Exit_ValidateTSS;
                }

                // legacy texture modulation must have texture alpha
                if (!lpTexture->bAlpha && (aop == D3DTOP_LEGACY_ALPHAOVR))
                {
                    lpvtssd->ddrval = D3DERR_WRONGTEXTUREFORMAT;
                    lpvtssd->dwNumPasses = 0;
                    DBG_D3D((2,"D3DERR_WRONGTEXTUREFORMAT a format "
                               "with alpha must be used"));
                    goto Exit_ValidateTSS;
                }

                // modulation w diffuse alpha channel must lack texture
                // alpha channel due to Permedia2 limitations on 
                // texture blending operations
                if (lpTexture->bAlpha && (aop == D3DTOP_SELECTARG2))
                {
                    lpvtssd->ddrval = D3DERR_WRONGTEXTUREFORMAT;
                    lpvtssd->dwNumPasses = 0;
                    DBG_D3D((2,"D3DERR_WRONGTEXTUREFORMAT a format "
                               "with alpha must be used"));
                    goto Exit_ValidateTSS;
                }
            }
            
            else if (((aa2 == D3DTA_DIFFUSE)   &&        // modulate alpha
                      (aa1 == D3DTA_TEXTURE)   && 
                      (aop == D3DTOP_MODULATE))     ||
                     ((aa1 == D3DTA_TEXTURE)   &&        // modulate color & pass texture alpha
                      (aop == D3DTOP_SELECTARG1))
                    )
            {
                goto Exit_ValidateTSS;
            }
            else
            {
                goto Fail_Validate;
            }
        }
        else if (   ((ca2 == D3DTA_DIFFUSE)            &&   // Check decal alpha
                     (ca1 == D3DTA_TEXTURE)            && 
                     (cop == D3DTOP_BLENDTEXTUREALPHA) &&
                     (aa2 == D3DTA_DIFFUSE)            && 
                     (aop == D3DTOP_SELECTARG2))            ||
        
                    ((ca2 == D3DTA_DIFFUSE) &&              // Check add
                     (ca1 == D3DTA_TEXTURE) && 
                     (cop == D3DTOP_ADD)    &&
                     (aa2 == D3DTA_DIFFUSE) && 
                     (aop == D3DTOP_SELECTARG2))            ||
                
                    ((cop == D3DTOP_DISABLE) ||             // Check disable
                      (cop == D3DTOP_SELECTARG2 && 
                       ca2 == D3DTA_DIFFUSE     && 
                       aop == D3DTOP_SELECTARG2 && 
                       aa2 == D3DTA_DIFFUSE))
               )
        {
            goto Exit_ValidateTSS;
        }
        // Don't understand
        else 
        {   //Here we make sure that cop, aop, ca1, ca2, aa1 and aa2 are valid
            
Fail_Validate:
            DBG_D3D((4,"Failing with cop=%d ca1=%d ca2=%d aop=%d aa1=%d aa2=%d",
                       cop,ca1,ca2,aop,aa1,aa2));

            if (!((cop == D3DTOP_DISABLE)           ||
                  (cop == D3DTOP_ADD)               ||
                  (cop == D3DTOP_MODULATE)          ||
                  (cop == D3DTOP_BLENDTEXTUREALPHA) ||
                  (cop == D3DTOP_SELECTARG2)        ||
                  (cop == D3DTOP_SELECTARG1)))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDCOLOROPERATION;
            
            else if (!((aop == D3DTOP_SELECTARG1)      ||
                       (aop == D3DTOP_SELECTARG2)      ||
                       (aop == D3DTOP_MODULATE)        ||
                       (aop == D3DTOP_LEGACY_ALPHAOVR)))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDALPHAOPERATION;

            else if (!(ca1 == D3DTA_TEXTURE))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDCOLORARG;

            else if (!(ca2 == D3DTA_DIFFUSE))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDCOLORARG;

            else if (!(aa1 == D3DTA_TEXTURE))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDALPHAARG;

            else if (!(aa2 == D3DTA_DIFFUSE))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDALPHAARG;
            // We don't need to check min and mag since we set the 
            // values of pContext->TssStates[D3DTSS_MAGFILTER] and
            // pContext->TssStates[D3DTSS_MINFILTER] to the correct values.
            // pContext->TssStates[D3DTSS_MAGFILTER] is either D3DTFG_POINT
            // or D3DTFG_LINEAR. pContext->TssStates[D3DTSS_MINFILTER] is
            // either D3DTFN_POINT or D3DTFN_LINEAR

            lpvtssd->dwNumPasses = 0;
            DBG_D3D((2,"D3DERR_UNSUPPORTEDCOLOROPERATION"));
        }
    }
Exit_ValidateTSS:
    DBG_D3D((6,"Exiting D3DValidateTextureStageState with dwNumPasses=%d",
                                                    lpvtssd->dwNumPasses));

    return DDHAL_DRIVER_HANDLED;
} // D3DValidateTextureStageState


/**
 *
 * Function:    D3DGetDriverState
 *
 * Description: This callback is used by both the DirectDraw and Direct3D 
 *              runtimes to obtain information from the driver about its 
 *              current state.
 *
 * Parameters
 *
 *     lpgdsd   
 *           pointer to GetDriverState data structure
 *
 *           dwFlags
 *                   Flags to indicate the data required
 *           dwhContext
 *                   The ID of the context for which information 
 *                   is being requested
 *           lpdwStates
 *                   Pointer to the state data to be filled in by the driver
 *           dwLength
 *                   Length of the state data buffer to be filled 
 *                   in by the driver
 *           ddRVal
 *                   Return value
 *
 * Return Value
 *
 *      DDHAL_DRIVER_HANDLED
 *      DDHAL_DRIVER_NOTHANDLED
 */

DWORD CALLBACK  
D3DGetDriverState( LPDDHAL_GETDRIVERSTATEDATA lpgdsd )
{
    PERMEDIA_D3DCONTEXT *pContext;
    DBG_D3D((6,"Entering D3DGetDriverState"));
    if (lpgdsd->dwFlags != D3DDEVINFOID_TEXTUREMANAGER)
    {
        DBG_D3D((0,"D3DGetDriverState DEVICEINFOID=%08lx not supported",
            lpgdsd->dwFlags));
        return DDHAL_DRIVER_NOTHANDLED;
    }
    if (lpgdsd->dwLength < sizeof(D3DDEVINFO_TEXTUREMANAGER))
    {
        DBG_D3D((0,"D3DGetDriverState dwLength=%d is not sufficient",
            lpgdsd->dwLength));
        return DDHAL_DRIVER_NOTHANDLED;
    }
    if (pContext = (PERMEDIA_D3DCONTEXT *)lpgdsd->dwhContext)
    {

        TextureCacheManagerGetStats(pContext,
               (LPD3DDEVINFO_TEXTUREMANAGER)lpgdsd->lpdwStates);

        lpgdsd->ddRVal = DD_OK;
    }
    else
    {
         lpgdsd->ddRVal = D3DHAL_CONTEXT_BAD; 
    }
    DBG_D3D((6,"Exiitng D3DGetDriverState"));

    return DDHAL_DRIVER_HANDLED;
} // D3DGetDriverState


/**
 *
 * Function:    D3DDestroyDDLocal
 *
 * Description: D3dDestroyDDLocal destroys all the Direct3D surfaces previously 
 *              created by D3DCreateSurfaceEx that belong to the same given 
 *              local DirectDraw object.
 *
 *              All Direct3D drivers must support D3dDestroyDDLocal.
 *              Direct3D calls D3dDestroyDDLocal when the application indicates 
 *              that the Direct3D context is no longer required and it will be 
 *              destroyed along with all surfaces associated to it. 
 *              The association comes through the pointer to the local 
 *              DirectDraw object. The driver must free any memory that the
 *              driver's D3dCreateSurfaceEx callback allocated for
 *              each surface if necessary. The driver should not destroy 
 *              the DirectDraw surfaces associated with these Direct3D surfaces; 
 *              this is the application's responsibility.
 *
 * Parameters
 *
 *      lpdddd
 *            Pointer to the DestoryLocalDD structure that contains the
 *            information required for the driver to destroy the surfaces.
 *
 *            dwFlags
 *                  Currently unused
 *            pDDLcl
 *                  Pointer to the local Direct Draw object which serves as a
 *                  reference for all the D3D surfaces that have to be destroyed.
 *            ddRVal
 *                  Specifies the location in which the driver writes the return
 *                  value of D3dDestroyDDLocal. A return code of DD_OK indicates
 *                   success.
 *
 * Return Value
 *
 *      DDHAL_DRIVER_HANDLED
 *      DDHAL_DRIVER_NOTHANDLED
 */

DWORD CALLBACK  
D3DDestroyDDLocal( LPDDHAL_DESTROYDDLOCALDATA lpdddd )
{
    DBG_D3D((6,"Entering D3DDestroyDDLocal"));

    RemoveDirectDrawLocal((LPVOID)(lpdddd->pDDLcl));
    lpdddd->ddRVal = DD_OK;

    DBG_D3D((6,"Exiting D3DDestroyDDLocal"));

    return DDHAL_DRIVER_HANDLED;
} // D3DDestroyDDLocal


/**
 *
 * Function:    DdSetColorkey
 *
 * Description: DirectDraw SetColorkey callback
 *
 *  Parameters
 *       lpSetColorKey
 *             Pointer to the LPDDHAL_SETCOLORKEYDATA parameters structure 
 *
 *             lpDDSurface
 *                  Surface struct
 *             dwFlags
 *                  Flags
 *             ckNew
 *                  New chroma key color values
 *             ddRVal
 *                  Return value
 *             SetColorKey
 *                  Unused: Win95 compatibility
 *
 */

DWORD CALLBACK 
DdSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKey)
{
    LPDWLIST    lpHandleList;
    DWORD       dwSurfaceHandle =
                        lpSetColorKey->lpDDSurface->lpSurfMore->dwSurfaceHandle;

    DBG_D3D((6,"Entering DdSetColorKey dwSurfaceHandle=%d ",dwSurfaceHandle));

    lpSetColorKey->ddRVal = DD_OK;

    // We don't have to do anything for normal blt source colour keys:
    if (!(DDSCAPS_TEXTURE & lpSetColorKey->lpDDSurface->ddsCaps.dwCaps) ||
        !(DDSCAPS_VIDEOMEMORY & lpSetColorKey->lpDDSurface->ddsCaps.dwCaps) 
       )
    {
        return(DDHAL_DRIVER_HANDLED);
    }

    if (0 != dwSurfaceHandle)
    {
        lpHandleList = GetSurfaceHandleList(lpSetColorKey->lpDDSurface->lpSurfMore->lpDD_lcl);

        if (lpHandleList != NULL)
        {
            PERMEDIA_D3DTEXTURE *pTexture = lpHandleList->dwSurfaceList[dwSurfaceHandle];

            ASSERTDD((PtrToUlong(lpHandleList->dwSurfaceList[0]) > dwSurfaceHandle),
                        "SetColorKey: incorrect dwSurfaceHandle");

            if (NULL != pTexture)
            {
                DBG_D3D((4, "DdSetColorKey surface=%08lx KeyLow=%08lx", 
                                                        dwSurfaceHandle,
                                                        pTexture->dwKeyLow));
                pTexture->dwFlags   |= DDRAWISURF_HASCKEYSRCBLT;
                pTexture->dwKeyLow  = lpSetColorKey->ckNew.dwColorSpaceLowValue;
                pTexture->dwKeyHigh = lpSetColorKey->ckNew.dwColorSpaceHighValue;
                return DDHAL_DRIVER_HANDLED;
            }
        }
    }

    lpSetColorKey->ddRVal = DDERR_INVALIDPARAMS;

    DBG_D3D((6,"Exiting DdSetColorKey"));

    return DDHAL_DRIVER_HANDLED;
}   // DdSetColorKey


/*
 *
 * Function:    D3DContextCreate
 *
 * Description: The ContextCreate callback is invoked when a new Direct3D device 
 *              is being created by a Direct3D application. The driver is 
 *              required to generate a unique context id for this new context. 
 *              Direct3D will then use this context id in every subsequent 
 *              callback invocation for this Direct3D device. 
 *
 *              Context can be thought of as a virtual 3D Hardware Device. 
 *              Using contexts different Direct3D applications can be provided
 *              this illusion of owning their own 3D hardware. This is 
 *              analogous to different processes owning the CPU in a
 *              multi-tasking O.S. 
 *              This implies that we need to provide mechanisms for saving and 
 *              restoring the state of the 3D hardware device when there is
 *              a change in the Direct3D context. 
 *              Direct3D applications identify themselves via this context in 
 *              the Driver space. Please note that it possible for more than 
 *              one context to exit per application.
 *              In the current driver, the context is encapsulated with
 *              the values of the minimal set of registers (SR) so that we can 
 *              faithfully restore the state of the 3D hardware in event the 
 *              context is re-activated later. We make one more optimization. 
 *              Since a context really does not get activated till an actual 
 *              rendering operation occurs (for example draw a tri-fan), we simply
 *              maintain a software copy of the register set (SR). When we
 *              receive a rendering operation we then load the register set (SR)
 *              with the software copy. This is done only if the Direct3D
 *              context currently running on the 3D hardware is different from
 *              the one for which the rendering operation is to be performed.
 *
 *              Please note that a couple of parameters to this function
 *              are RenderTargetHandle and ZBufferHandle (if Z buffering is done)
 *              These handles (and ofcourse their associated surfaces) are
 *              created prior to CreateContext callback. This means that
 *              you need to do some bookkeeping so that you can validate
 *              the handles.
 * 
 *              In addition to keeping the state of 3D hardware, the Context
 *              is a repository for keeping track of all resources associated
 *              associated with it. This can include texture handles, palette
 *              handles, mapping from texture handles to the actual surfaces etc.
 *
 *              Please note that Texture Handles are created per Direct Draw Local.
 *              Hence a texture handle (and hence a texture) can be theortically
 *              shared across multiple contexts. This means that when a Direct3D
 *              Context is destroyed, the Texture Handles (and hence the textures)
 *              associated with it should not be destroyed.
 *
 * Parameters
 *      pccd
 *           Pointer to a structure containing things including the current
 *           rendering surface, the current Z surface, and the DirectX object
 *           handle, etc.
 *
 *          .lpDDGbl    
 *                Points to the DirectDraw structure representing the 
 *                DirectDraw object. 
 *          .lpDDLcl(replaces lpDDGbl in DX7)    
 *                Points to the DirectDraw structure representing the 
 *                DirectDraw object. 
 *          .lpDDS      
 *                This is the surface that is to be used as the rendering 
 *                target, i.e., the 3D accelerator sprays its bits at this 
 *                surface. 
 *          .lpDDSZ     
 *                The surface that is to be used as the Z buffer. If this 
 *                is NULL, no Z buffering is to be performed. 
 *          .dwPid      
 *                The process id of the Direct3D application that initiated 
 *                the creation of the Direct3D device. 
 *          .dwhContext 
 *                The driver should place the context ID that it wants Direct3D 
 *                to use when communicating with the driver. This should be 
 *                unique. 
 *          .ddrval     
 *                Return code. DD_OK indicates success. 
 *
 * Return Value
 *      Returns one of the following values: 
 *                DDHAL_DRIVER_HANDLED  
 *                DDHAL_DRIVER_NOTHANDLED   
 *
 */

TextureCacheManager P2TextureManager;

DWORD CALLBACK 
D3DContextCreate(LPD3DHAL_CONTEXTCREATEDATA pccd)
{
    PERMEDIA_D3DCONTEXT*    pContext;
    LPDDRAWI_DIRECTDRAW_GBL lpDDGbl=pccd->lpDDLcl->lpGbl;
    LPP2THUNKEDDATA         ppdev;

    if (pccd->lpDDLcl->lpGbl->dwReserved3) 
        ppdev = (LPP2THUNKEDDATA)pccd->lpDDLcl->lpGbl->dwReserved3;
    else 
        ppdev = pDriverData; 

    DBG_D3D((6,"Entering D3DContextCreate"));
    
    pContext = (PERMEDIA_D3DCONTEXT *)D3DSHAREDALLOC(sizeof(PERMEDIA_D3DCONTEXT));

    if (pContext == NULL)
    {
        DBG_D3D((0,"ERROR: Couldn't allocate Context mem"));
        pccd->ddrval = DDERR_OUTOFMEMORY;
        return (DDHAL_DRIVER_HANDLED);
    }

    memset(pContext, 0, sizeof(PERMEDIA_D3DCONTEXT));
    // Remember the card we are running on
    pContext->pThisDisplay  = ppdev;

    // Set context handle in driver's D3D context
    pccd->dwhContext        = (ULONG_PTR)pContext; //out:Context handle

    DBG_D3D((4,"Allocated Direct3D context: 0x%x",pccd->dwhContext));


    // No texture at present
    pContext->CurrentTextureHandle = 0;

    // Initialize texture management for this context
    if (D3D_OK != TextureCacheManagerInitialize(&P2TextureManager))
    {
        DBG_D3D((0,"ERROR: Couldn't initialize TextureCacheManager"));
        D3DSHAREDFREE(pContext);
        pccd->ddrval = DDERR_OUTOFMEMORY;
        return (DDHAL_DRIVER_HANDLED);
    }
    pContext->pTextureManager = &P2TextureManager;

    // Remember the local DD object and get the 
    // correct array of surfaces for this context
    pContext->pDDLcl = pccd->lpDDLcl;
    pContext->pHandleList = GetSurfaceHandleList(pccd->lpDDLcl);
    if (pContext->pHandleList == NULL)
    {
        DBG_D3D((0,"ERROR: Couldn't get a surface handle for lpDDLcl"));
        TextureCacheManagerRelease(&P2TextureManager);
        D3DSHAREDFREE(pContext);
        pccd->ddrval = DDERR_OUTOFMEMORY;
        return (DDHAL_DRIVER_HANDLED);
    }

    DBG_D3D((4,"Getting pHandleList=0x%x for pDDLcl 0x%x",
                                 pContext->pHandleList,pccd->lpDDLcl));

    AddContextToHandleList(pContext);

    pContext->RenderSurfaceHandle = pccd->lpDDSLcl->lpSurfMore->dwSurfaceHandle;
    if (NULL != pccd->lpDDSZ) 
        pContext->ZBufferHandle = pccd->lpDDSZLcl->lpSurfMore->dwSurfaceHandle;
    else
        pContext->ZBufferHandle = 0;
    // Now write the default setup to the chip.
    HWInitContext(pContext);

    if ((pccd->ddrval = ValidateRenderTargetAndZBuffer(pContext)) != DD_OK)
    {
        TextureCacheManagerRelease(&P2TextureManager);
        D3DSHAREDFREE(pContext);
        return (DDHAL_DRIVER_HANDLED);
    }
    // ---------------- Setup default states in driver ------------------------
#if D3D_STATEBLOCKS
    // Default state block recording mode = no recording
    pContext->bStateRecMode     = FALSE;
    pContext->pCurrSS           = NULL;
    pContext->pIndexTableSS     = NULL;
    pContext->dwMaxSSIndex      = 0;
#endif //D3D_STATEBLOCKS

    pContext->Flags             = CTXT_HAS_GOURAUD_ENABLED ;
    pContext->CullMode          = D3DCULL_CCW;

    // Set the last alpha value to 16 to force a new
    // send of the flat stipple patterns.
    pContext->LastAlpha         = 16;

    pContext->bKeptStipple      = FALSE;  // By default, stippling is off
    pContext->bCanChromaKey     = FALSE;  // Turn Chroma keying off by default
    pContext->LowerChromaColor  = 0x0; // These are the default chromakey values
    pContext->UpperChromaColor  = 0x0;

    pContext->FakeBlendNum      = 0;       // No need to emulate any blend mode


    // Initialise the RenderCommand.  States will add to this
    RENDER_SUB_PIXEL_CORRECTION_ENABLE(pContext->RenderCommand);

    // Setup TSS defaults for stage 0
    pContext->TssStates[D3DTSS_TEXTUREMAP]      = 0;
    pContext->TssStates[D3DTSS_COLOROP]         = D3DTOP_MODULATE;
    pContext->TssStates[D3DTSS_ALPHAOP]         = D3DTOP_SELECTARG1;
    pContext->TssStates[D3DTSS_COLORARG1]       = D3DTA_TEXTURE;
    pContext->TssStates[D3DTSS_COLORARG2]       = D3DTA_CURRENT;
    pContext->TssStates[D3DTSS_ALPHAARG1]       = D3DTA_TEXTURE;
    pContext->TssStates[D3DTSS_ALPHAARG2]       = D3DTA_CURRENT;
    pContext->TssStates[D3DTSS_TEXCOORDINDEX]   = 0;
    pContext->TssStates[D3DTSS_ADDRESS]         = D3DTADDRESS_WRAP;
    pContext->TssStates[D3DTSS_ADDRESSU]        = D3DTADDRESS_WRAP;
    pContext->TssStates[D3DTSS_ADDRESSV]        = D3DTADDRESS_WRAP;
    pContext->TssStates[D3DTSS_MAGFILTER]       = D3DTFG_POINT;
    pContext->TssStates[D3DTSS_MINFILTER]       = D3DTFN_POINT;
    pContext->TssStates[D3DTSS_MIPFILTER]       = D3DTFP_NONE;

    pContext->TssStates[D3DTSS_BUMPENVMAT00]    = 0;           // info we don't use
    pContext->TssStates[D3DTSS_BUMPENVMAT01]    = 0;           // in this sample 
    pContext->TssStates[D3DTSS_BUMPENVMAT10]    = 0;
    pContext->TssStates[D3DTSS_BUMPENVMAT11]    = 0;
    pContext->TssStates[D3DTSS_BUMPENVLSCALE]   = 0;
    pContext->TssStates[D3DTSS_BUMPENVLOFFSET]  = 0;
    pContext->TssStates[D3DTSS_BORDERCOLOR]     = 0x00000000;
    pContext->TssStates[D3DTSS_MAXMIPLEVEL]     = 0;
    pContext->TssStates[D3DTSS_MAXANISOTROPY]   = 1;

    // Initially set values to force change
    pContext->bStateValid = TRUE;

    // Force a change in texture before any 
    // rendering takes place for this context
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;

    DBG_D3D((6,"Exiting D3DContextCreate pccd->dwhContext=0x%x", pccd->dwhContext));

    pccd->ddrval = DD_OK;
    return (DDHAL_DRIVER_HANDLED);
} // D3DContextCreate

/**
 *
 * Function:    D3DContextDestroy
 *
 * Description: This callback is invoked when a Direct3D Device is being 
 *              destroyed. As each device is represented by a context ID, 
 *              the driver is passed a context to destroy. 
 *              Please note that although we call TextureCacheManagerRelease
 *              the Textures are released only when all the Direct3D contexts
 *              are destroyed.
 *
 * Parameters
 *     pcdd
 *          Pointer to Context destroy information.
 *
 *          .dwhContext 
 *               The ID of the context to be destroyed. 
 *          .ddrval
 *               Return code. DD_OK indicates success. 
 *
 * Return Value
 *      Returns one of the following values: 
 *                DDHAL_DRIVER_HANDLED   
 *                DDHAL_DRIVER_NOTHANDLED    
 */

DWORD CALLBACK 
D3DContextDestroy(LPD3DHAL_CONTEXTDESTROYDATA pcdd)
{
    PERMEDIA_D3DCONTEXT *pContext = (PERMEDIA_D3DCONTEXT *)pcdd->dwhContext ;

    // Deleting context
    DBG_D3D((6,"Entering D3DContextDestroy, context = %08lx",pContext));

    TextureCacheManagerRelease(&P2TextureManager);

#if D3D_STATEBLOCKS
    // Free up any remaining state sets
    __DeleteAllStateSets(pContext);
#endif //D3D_STATEBLOCKS

    DeleteContextFromHandleList(pContext);
    // Finally, free up the rendering context structure itself
    D3DSHAREDFREE(pContext);

    pcdd->ddrval = DD_OK;

    DBG_D3D((6,"Exiting D3DContextDestroy"));

    return (DDHAL_DRIVER_HANDLED);
} // D3DContextDestroy


/*
 * 
 * Function:    D3DCreateSurfaceEx
 * Description: D3dCreateSurfaceEx creates a Direct3D surface from a DirectDraw  
 *              surface and associates a requested handle value to it.
 * 
 *              All Direct3D drivers must support D3dCreateSurfaceEx.
 * 
 *              D3dCreateSurfaceEx creates an association between a  
 *              DirectDraw surface and a small integer surface handle. 
 *              By creating these associations between a
 *              handle and a DirectDraw surface, D3dCreateSurfaceEx allows a 
 *              surface handle to be imbedded in the Direct3D command stream.
 *              For example when the D3DDP2OP_TEXBLT command token is sent
 *              to D3dDrawPrimitives2 to load a texture map, it uses a source 
 *              handle and destination handle which were associated
 *              with a DirectDraw surface through D3dCreateSurfaceEx.
 *
 *              For every DirectDraw surface created under the DirectDrawLocal 
 *              object, the runtime generates a valid handle that uniquely
 *              identifies the surface and places it in
 *              pcsxd->lpDDSLcl->lpSurfMore->dwSurfaceHandle. This handle value
 *              is also used with the D3DRENDERSTATE_TEXTUREHANDLE render state 
 *              to enable texturing, and with the D3DDP2OP_SETRENDERTARGET and 
 *              D3DDP2OP_CLEAR commands to set and/or clear new rendering and 
 *              depth buffers. The driver should fail the call and return
 *              DDHAL_DRIVER_HANDLED if it cannot create the Direct3D
 *              surface. If the DDHAL_CREATESURFACEEX_SWAPHANDLES flag is set, 
 *              the handles should be swapped over two sequential calls to 
 *              D3dCreateSurfaceEx. As appropriate, the driver should also 
 *              store any surface-related information that it will subsequently 
 *              need when using the surface. The driver must create
 *              a new surface table for each new lpDDLcl and implicitly grow 
 *              the table when necessary to accommodate more surfaces. 
 *              Typically this is done with an exponential growth algorithm 
 *              so that you don't have to grow the table too often. Direct3D 
 *              calls D3dCreateSurfaceEx after the surface is created by
 *              DirectDraw by request of the Direct3D runtime or the application.
 *
 * Parameters
 *
 *      lpcsxd
 *           pointer to CreateSurfaceEx structure that contains the information
 *           required for the driver to create the surface (described below). 
 *
 *           dwFlags
 *           lpDDLcl
 *                   Handle to the DirectDraw object created by the application.
 *                   This is the scope within which the lpDDSLcl handles exist.
 *                   A DD_DIRECTDRAW_LOCAL structure describes the driver.
 *           lpDDSLcl
 *                   Handle to the DirectDraw surface we are being asked to
 *                   create for Direct3D. These handles are unique within each
 *                   different DD_DIRECTDRAW_LOCAL. A DD_SURFACE_LOCAL structure
 *                   represents the created surface object.
 *           ddRVal
 *                   Specifies the location in which the driver writes the return
 *                   value of the D3dCreateSurfaceEx callback. A return code of
 *                   DD_OK indicates success.
 *
 * Return Value
 *
 *      DDHAL_DRIVER_HANDLE
 *      DDHAL_DRIVER_NOTHANDLE
 *
 */

DWORD CALLBACK 
D3DCreateSurfaceEx( LPDDHAL_CREATESURFACEEXDATA lpcsxd )
{
    LPVOID                      pDDLcl= (LPVOID)lpcsxd->lpDDLcl;
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSLcl=lpcsxd->lpDDSLcl;
    LPP2THUNKEDDATA             ppdev;
    LPDDRAWI_DDRAWSURFACE_LCL   lpStartDDSLcl = lpDDSLcl;

    if (lpcsxd->lpDDLcl->lpGbl->dwReserved3) 
        ppdev = (LPP2THUNKEDDATA)lpcsxd->lpDDLcl->lpGbl->dwReserved3;
    else 
        ppdev = pDriverData; 

    DBG_D3D((6,"Entering D3DCreateSurfaceEx"));

    lpcsxd->ddRVal = DD_OK;

    if (NULL == lpDDSLcl || NULL == pDDLcl)
    {
        DBG_D3D((0,"D3DCreateSurfaceEx received 0 lpDDLcl or lpDDSLcl pointer"));
        return DDHAL_DRIVER_HANDLED;
    }


    // We check that what we are handling is a texture, zbuffer or a rendering
    // target buffer. We don't check if it is however stored in local video
    // memory since it might also be a system memory texture that we will later
    // blt with __TextureBlt.
    // also if your driver supports DDSCAPS_EXECUTEBUFFER create itself, it must 
    // process DDSCAPS_EXECUTEBUFFER here as well.
    if (!(lpDDSLcl->ddsCaps.dwCaps & 
             (DDSCAPS_TEXTURE       | 
              DDSCAPS_3DDEVICE      | 
              DDSCAPS_ZBUFFER))
       )
    {
        DBG_D3D((2,"D3DCreateSurfaceEx w/o "
             "DDSCAPS_TEXTURE/3DDEVICE/ZBUFFER Ignored"
             "dwCaps=%08lx dwSurfaceHandle=%08lx",
             lpDDSLcl->ddsCaps.dwCaps,
             lpDDSLcl->lpSurfMore->dwSurfaceHandle));
        return DDHAL_DRIVER_HANDLED;
    }

    lpcsxd->ddRVal = CreateSurfaceHandleLoop( ppdev, 
                                              pDDLcl, 
                                              lpDDSLcl, 
                                              lpDDSLcl);

    DBG_D3D((4,"Exiting D3DCreateSurfaceEx"));

    return DDHAL_DRIVER_HANDLED;
} // D3DCreateSurfaceEx

/*
 *
 * Function:    CreateSurfaceHandleLoop
 *
 * Description: Allocate a list of new surface handles by traversing AttachList
 *              recursively and calling __CreateSurfaceHandle()
 *              only exceptions are for MIPMAP and CUBMAP, which we only
 *              use one handle to the root to represent the whole surface
 *  return value
 *
 *      DD_OK   -- no error
 *      DDERR_OUTOFMEMORY -- allocation of texture handle failed
 *
 */

DWORD CreateSurfaceHandleLoop( LPP2THUNKEDDATA              ppdev,
                                LPVOID                      pDDLcl,
                                LPDDRAWI_DDRAWSURFACE_LCL   lpDDSLclroot,
                                LPDDRAWI_DDRAWSURFACE_LCL   lpDDSLcl)
{
    LPATTACHLIST    curr;
    DWORD ddRVal=DD_OK;
    // Now allocate the texture data space
    if (0 == lpDDSLcl->lpSurfMore->dwSurfaceHandle)
    {
        DBG_D3D((0,"CreateSurfaceHandleLoop got 0 handle dwCaps=%08lx",
            lpDDSLcl->ddsCaps.dwCaps));
        return DD_OK;
    }

    ddRVal = CreateSurfaceHandle( ppdev, pDDLcl, lpDDSLcl);
    if (DD_OK != ddRVal)
    {
        return ddRVal;
    }

    // for some surfaces other than MIPMAP or CUBEMAP, such as
    // flipping chains, we make a slot for every surface, as
    // they are not as interleaved
    if ((lpDDSLcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP) ||
        (lpDDSLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP)
       )
    {
        return DD_OK;
    }
    curr = lpDDSLcl->lpAttachList;
    if (NULL == curr) 
        return DD_OK;

    // check if there is another surface attached!
    if (curr->lpLink)
    {
        lpDDSLcl=curr->lpLink->lpAttached; 
        if (NULL != lpDDSLcl && lpDDSLcl != lpDDSLclroot)
        {
            ddRVal = CreateSurfaceHandleLoop( ppdev, 
                                              pDDLcl, 
                                              lpDDSLclroot, 
                                              lpDDSLcl);
            if (DD_OK != ddRVal)
            {
                return ddRVal;
            }
        }
    }

    lpDDSLcl=curr->lpAttached;
    if (NULL != lpDDSLcl && lpDDSLcl != lpDDSLclroot)
        ddRVal = CreateSurfaceHandleLoop(ppdev, 
                                         pDDLcl, 
                                         lpDDSLclroot, 
                                         lpDDSLcl);

    return ddRVal;
} //CreateSurfaceHandleLoop

/*
 *
 * Function:    ValidateRenderTargetAndZBuffer
 *
 * Description: Validates the RenderTarget and Zbuffer (if present) Handles.
 */

HRESULT
ValidateRenderTargetAndZBuffer(PERMEDIA_D3DCONTEXT* pContext)
{
    PP2REG_SOFTWARE_COPY    pSoftPermedia   = &pContext->SoftCopyP2Regs;
    LPP2THUNKEDDATA         ppdev           = pContext->pThisDisplay;
    PPERMEDIA_D3DTEXTURE    pSurfRender,pSurfZBuffer;

    DBG_D3D((10,"Entering ValidateRenderTargetAndZBuffer"));

    pSurfRender = 
        TextureHandleToPtr(pContext->RenderSurfaceHandle, pContext);
    if (pSurfRender == NULL) 
    {
        DBG_D3D((0,"ERROR: SetupPermediaRenderTarget"
            " Invalid pSurfRender handle=%08lx",
            pContext->RenderSurfaceHandle));
        return DDERR_INVALIDPARAMS;
    }

    if (DDSCAPS_SYSTEMMEMORY & pSurfRender->dwCaps)
    {
        DBG_D3D((0, "ERROR: SetupPermediaRenderTarget"
            " Render Surface in SYSTEMMEMORY handle=%08lx",
            pContext->RenderSurfaceHandle));
        return DDERR_INVALIDPARAMS;    
    }
    if (0 != pContext->ZBufferHandle)
    {
        pSurfZBuffer = 
            TextureHandleToPtr(pContext->ZBufferHandle, pContext);
        if (pSurfZBuffer == NULL)
        {
            DBG_D3D((0,"ERROR: SetupPermediaRenderTarget"
                " invalid pSurfZBuffer handle=%08lx",
                pContext->ZBufferHandle));
            pContext->ZBufferHandle = 0;
        }
        else
        if (DDSCAPS_SYSTEMMEMORY & pSurfZBuffer->dwCaps)
        {
            DBG_D3D((0, "ERROR: SetupPermediaRenderTarget"
                " pSurfZBuffer in SYSTEMMEMORY  handle=%08lx",
                pContext->ZBufferHandle));
            pContext->ZBufferHandle = 0;
        }
    }
    SetupPermediaRenderTarget(pContext);

    DBG_D3D((10,"Exiting ValidateRenderTargetAndZBuffer"));
    return DD_OK;   
} //ValidateRenderTargetAndZBuffer

