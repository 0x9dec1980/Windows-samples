/*
  (C) Copyright MICROSOFT Corp., 1991
*/

/** posthost.c                                                          **/


/* the includes required to build this file */
    #include <windows.h>
    #include "posthost.h"


/* crude (very) debugging stuff */
    #ifdef DEBUG
        #define D(x)    {x;}
        #define ODS(x)  OutputDebugString(x)
    #else
        #define D(x)
        #define ODS(x)
    #endif


/* defines the device ID of VPOSTD... */
    #define VPOSTD_DEV_ID       0x7fe8


/* internal function prototypes */
    void FAR PASCAL phCallBack( HWND hWnd, WORD wVMID, DWORD lParam );


/* need to guarantee that phCallBack() is in FIXED (page-locked) segment */
    #pragma alloc_text( CALLBACK, phCallBack )


/* global variables */
    HANDLE          ghModule;           /* handle (instance?) of .DLL   */
    WORD            gwError;            /* status info on POSTHOST.DLL  */
    DWORD           VPOSTD_API = NULL;  /* addr of VPOSTD PM API        */



/** void FAR PASCAL phCallBack( HWND hWnd, WORD wVMID, DWORD lParam )
 *
 *  DESCRIPTION:
 *      This is the callback for the VPOSTD.  It should be treated as
 *      an ISR--all rules of ISR's apply here.  You may ONLY call
 *      PostMessage() and PostAppMessage() in an ISR.
 *
 *  ARGUMENTS:
 *      ( HWND hWnd, WORD wVMID, DWORD lParam )
 *
 *  RETURN (void FAR PASCAL):
 *      None.  We were 'far called' (simulated), so we must RETF.  Also
 *      note that we are using the PL/M calling convention, so *we* pop
 *      the stuff off the stack--not the VPOSTD (RETF 8).
 *
 *  NOTES:
 *
 ** cjp */

void FAR PASCAL phCallBack( HWND hWnd, WORD wVMID, DWORD lParam )
{
    ODS( "POSTHOST: phCallBack() called...\r\n" );

    /* quick sanity check--do *NOT* call IsWindow()!!! */
    if ( hWnd )
    {
        /* post the message with the caller's VM ID and CX:DX lParam */
        PostMessage( hWnd, WM_POSTHOSTPOST, wVMID, lParam );
    }
} /* phCallBack() */


/** WORD FAR PASCAL phGetPostHostError( void )
 *
 *  DESCRIPTION: 
 *      This function returns the error status of POSTHOST.DLL.  It should
 *      be used to determine if the .DLL is ready for business or if 
 *      something is hosed.  This is actually quite superfluous; the .DLL
 *      should just fail the LibMain(), but that is harsh for this example.
 *      This works well enough for this example.
 *
 *  ARGUMENTS:
 *      None.
 *
 *  RETURN (WORD FAR PASCAL):
 *      The return value is one of the following:
 *
 *          PHERR_NOERROR  (0)  :   Everything is cool.
 *          PHERR_NO386ENH (1)  :   You gotta run 386 Enh mode!
 *          PHERR_NOVPOSTD (2)  :   VPOSTD.386 is not installed.
 *
 *  NOTES:
 *
 ** cjp */

WORD FAR PASCAL phGetPostHostError( void )
{
    /* simply return gwError--was filled in during LibMain() */
    return ( gwError );
} /* phGetPostHostError() */


/** DWORD FAR PASCAL phGetVPOSTDAPIAddr void )
 *
 *  DESCRIPTION: 
 *      This function is used to get the far pointer to the API that
 *      is exported by VPOSTD.
 *
 *  ARGUMENTS:
 *      None.
 *
 *  RETURN (DWORD FAR PASCAL):
 *      The return value is the address of the *Protect Mode* API that
 *      VPOSTD exports.  It is NULL if VPOSTD is not available.
 *
 *  NOTES:
 *
 ** cjp */

DWORD FAR PASCAL phGetVPOSTDAPIAddr( void )
{
    /* return the far pointer to PM API entry */
    return ( VPOSTD_API );
} /* phGetVPOSTDAPIAddr() */


/** WORD FAR PASCAL phGetVPOSTDVersion( void )
 *
 *  DESCRIPTION: 
 *      This function returns the version of VPOSTD.  The HIBYTE() of the
 *      return value is the major version number; the LOBYTE() is the 
 *      minor version number.
 *
 *  ARGUMENTS:
 *      None.
 *
 *  RETURN (WORD FAR PASCAL):
 *      The return value is the version of the VPOSTD.  The HIBYTE() of the
 *      return value is the major version number; the LOBYTE() is the 
 *      minor version number.  The return value is zero if VPOSTD is not
 *      available.
 *
 *  NOTES:
 *
 ** cjp */

WORD FAR PASCAL phGetVPOSTDVersion( void )
{
    WORD    wReturn = 0x0000;           /* assume the worst         */

    /* sanity check */
    if ( VPOSTD_API )
    {
        _asm
        {
            mov     ax, 0                   ; func 0 is 'Get Version'
            call    dword ptr [VPOSTD_API]  ; call VPOSTD
            mov     wReturn, ax             ; stuff the return value
        }
    }

    /* return the result */
    return ( wReturn );
} /* phGetVPOSTDVersion() */


/** BOOL FAR PASCAL phRegisterWindow( HWND hWnd )
 *
 *  DESCRIPTION: 
 *      This function is used to register a window handle with VPOSTD
 *      to receive posts.  In this example, only one window may receive
 *      messages at a time.
 *
 *  ARGUMENTS:
 *      HWND hWnd   :   Window to receive posts.  If NULL, the callback
 *                      will be de-registered.
 *
 *  RETURN (BOOL FAR PASCAL):
 *      The return value is TRUE if this function succeeds in setting
 *      the window handle in VPOSTD to hWnd.  It is FALSE if it fails.
 *
 *  NOTES:
 *
 ** cjp */

BOOL FAR PASCAL phRegisterWindow( HWND hWnd )
{
    BOOL    fReturn     = FALSE;        /* assume bad things        */
    DWORD   dwCallBack;
    
    
    dwCallBack = hWnd ? (DWORD)phCallBack : NULL;

    /* make sure we have the API entry point to VPOSTD */
    if ( VPOSTD_API )
    {
        _asm
        {
            mov     ax, 1                   ; func 1 is 'Register Callback'
            mov     bx, hWnd                ; hWnd in bx
            les     di, dwCallBack          ; callback addr in es:di

            call    dword ptr [VPOSTD_API]  ; hello VPOSTD...

            mov     fReturn, ax             ; return result
        }
    }

    /* return outcome of the function */
    return ( fReturn );
} /* phRegisterWindow() */


/** BOOL FAR PASCAL phCallPostHost( DWORD lParam )
 *
 *  DESCRIPTION: 
 *      This function will call VPOSTD to post a message to the current
 *      hWnd that has been registered with phRegisterWindow.  You could
 *      say that this is doing it the hard way, but it demonstrates what
 *      it is intended to demonstrate...
 *
 *  ARGUMENTS:
 *      DWORD lParam    :   Arbitrary DWORD to be posted as the lParam
 *                          to the current registered window.
 *
 *  RETURN (BOOL FAR PASCAL):
 *      The return value is TRUE if this function succeeds.  It is FALSE
 *      if VPOSTD is not available.
 *
 *  NOTES:
 *
 ** cjp */

BOOL FAR PASCAL phCallPostHost( DWORD lParam )
{
    BOOL    fReturn = FALSE;            /* assume bad things        */

    /* make sure we have the API entry point to VPOSTD */
    if ( VPOSTD_API )
    {
        _asm
        {
            mov     ax, 2                       ; func 2 is 'Call PostHost'
            mov     dx, word ptr [lParam + 0]   ; low word in DX
            mov     cx, word ptr [lParam + 2]   ; high word in CX

            call    dword ptr [VPOSTD_API]      ; call VPOSTD

            mov     fReturn, ax                 ; return result
        }
    }

    /* return the outcome of the function */
    return ( fReturn );
} /* phCallPostHost() */


/** BOOL FAR PASCAL WEP( int nArgument )
 *
 *  DESCRIPTION:
 *      Performs cleanup tasks when the .DLL is unloaded.  The WEP() is
 *      called automatically by Windows when the .DLL is unloaded (no
 *      remaining tasks still have the .DLL loaded).  It is strongly
 *      recommended that a .DLL have a WEP() function, even if it does
 *      nothing but return, as in this example.
 *
 *      Make sure that the WEP() is @1 RESIDENTNAME in the EXPORTS
 *      section of the .DEF file.  This ensures that the WEP() can
 *      be called as quickly as possible.  Incidently, this is why
 *      the WEP() is called the WEP() instead of WindowsExitProcedure().
 *      It takes up the minimum amount of space and is quickly located.
 *
 *
 ** cjp */

BOOL FAR PASCAL WEP( int nArgument )
{
    switch ( nArgument )
    {
        case WEP_SYSTEM_EXIT:
            ODS( "POSTHOST: WEP_SYSTEM_EXIT\r\n" );
        break;

        case WEP_FREE_DLL:
            ODS( "POSTHOST: WEP_FREE_DLL\r\n" );
        break;

        default:
            ODS( "POSTHOST: WEP_UNDEFINED!!!\r\n" );
        break;
    }

    /* always return TRUE (1) */
    return ( TRUE );
} /* WEP() */


/** BOOL FAR PASCAL LibMain( HANDLE, WORD, WORD, LPSTR )
 *
 *  DESCRIPTION:
 *      Is called by LibInit().  LibInit() is called by Windows when
 *      the .DLL is loaded.  LibInit() initializes the DLL's heap, if
 *      a HEAPSIZE value is specified in the DLL's .DEF file.  Then
 *      LibInit() calls LibMain().  The LibMain() function below
 *      satisfies that call.
 *
 *      The LibMain() function should perform additional initialization
 *      tasks required by the .DLL.  In this example, the hModule is
 *      made global and the VPOSTD_API is justified.  If the latter
 *      fails, the .DLL will be unloaded...
 *
 ** cjp */

BOOL FAR PASCAL LibMain( HANDLE hModule,
                         WORD   wDataSeg,
                         WORD   cbHeapSize,
                         LPSTR  lpszCmdLine )
{
    /* make instance handle global */
    ghModule = hModule;

    /* get API entry to VPOSTD--if it is installed */
    _asm
    {
        mov     ax, 1600h               ; enhanced mode?
        int     2Fh                     ; api call
        test    al, 7Fh                 ; enhance mode running?
        jz      not_running_enhanced    ; no

        mov     ax, 1684h               ; get device API call
        mov     bx, VPOSTD_DEV_ID       ; for the VPOSTD VxD
        int     2Fh                     ; get api entry point
        mov     word ptr VPOSTD_API, di ; save the callback address
        mov     word ptr VPOSTD_API + 2, es

        mov     ax, es                  ; is VPOSTD installed?
        or      ax, di
        jz      vxd_not_installed       ; if not, split

        mov     gwError, 0              ; show success (PHERR_NOERROR)
        jmp     get_out

not_running_enhanced:
        mov     gwError, 1              ; no enh windows! (PHERR_NO386ENH)
        jmp     get_out                 ; return our error code

vxd_not_installed:
        mov     gwError, 2              ; VPOSTD?  (PHERR_NOVPOSTD)

get_out:
    }

    /* jolly good show... */
    ODS( "POSTHOST: LibMain() succeeded...\r\n" );

    /* return success--even though we may have failed */
    return ( TRUE );
} /* LibMain() */


/** EOF: posthost.c **/
