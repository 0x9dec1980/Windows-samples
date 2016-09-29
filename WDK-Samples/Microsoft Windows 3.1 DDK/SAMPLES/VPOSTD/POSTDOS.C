/*
  (C) Copyright MICROSOFT Corp., 1991
*/

/** postdos.c                                                           **/


/* the includes required to build this file */
    #include <stdio.h>


/* the DWORD value to post */
    #define PDC_POSTVALUE   (0x1BADBABE)


/* quick Windows style typedefs */
    typedef unsigned short  BOOL;
    typedef unsigned short  WORD;
    typedef unsigned long   DWORD;


/* these also */
    #ifndef TRUE
        #define TRUE    (1)
        #define FALSE   (0)
    #endif


/* crude (very) debugging stuff */
    #ifdef DEBUG
        #define D(x)    {x;}
        #define ODS(x)  puts(x)
    #else
        #define D(x)
        #define ODS(x)
    #endif


/* defines for getting the device ID of VPOSTD... */
    #define VPOSTD_DEV_ID       0x7fe8


/* global variables */
    WORD        gwError;                    /* error status             */
    DWORD       VPOSTD_API = (DWORD)NULL;   /* addr of VPOSTD RM API    */


/** WORD pdGetVPOSTDVersion( void )
 *
 *  DESCRIPTION: 
 *      This function returns the version of VPOSTD.  The HIBYTE() of the
 *      return value is the major version number; the LOBYTE() is the 
 *      minor version number.
 *
 *  ARGUMENTS:
 *      None.
 *
 *  RETURN (WORD):
 *      The return value is the version of the VPOSTD.  The HIBYTE() of the
 *      return value is the major version number; the LOBYTE() is the 
 *      minor version number.  The return value is zero if VPOSTD is not
 *      available.
 *
 *  NOTES:
 *
 ** cjp */

WORD pdGetVPOSTDVersion( void )
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
} /* pdGetVPOSTDVersion() */


/** BOOL pdCallPostHost( DWORD lParam )
 *
 *  DESCRIPTION: 
 *      This function will call VPOSTD to post a message to the current
 *      hWnd that has been registered with phRegisterWindow (in Windows).
 *      Imagine this!  Doing a PostMessage() to a Windows app from a
 *      DOS app!  
 *
 *  ARGUMENTS:
 *      DWORD lParam    :   Arbitrary DWORD to be posted as the lParam
 *                          to the current registered window in Windows.
 *
 *  RETURN (BOOL):
 *      The return value is TRUE if this function succeeds.  It is FALSE
 *      if VPOSTD is not available.
 *
 *  NOTES:
 *
 ** cjp */

BOOL pdCallPostHost( DWORD lParam )
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
} /* pdCallPostHost() */



/** short main( void )
 *
 *  DESCRIPTION: 
 *      Your standard, everyday, run-of-the-mill, main()--with a few
 *      extras...
 *
 *  ARGUMENTS:
 *      None.
 *
 *  RETURN (short):
 *      Always TRUE.
 *
 *  NOTES:
 *
 ** cjp */

short main( void )
{
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

    /* can we call VPOSTD? */
    if ( gwError )
    {
        if ( gwError == 1 )
            puts( "RUN ENHANCED MODE WINDOWS FIRST!" );
        else
            puts( "YOU MUST INSTALL VPOSTD.386 FIRST!" );
    }

    /* good!  now run it through a small test... */
    else
    {
        BOOL    fResult;
        WORD    wVersion = pdGetVPOSTDVersion();

        printf( "Real Mode API Addr: %.8lXh\n", VPOSTD_API );
        printf( "VPOSTD.386 Version: %.4Xh\n", wVersion );

        printf( "Posting [%.8lXh]...", PDC_POSTVALUE );

        if ( ! (fResult = pdCallPostHost( PDC_POSTVALUE )) )
            puts( "FAILED!  You need to run POSTAPP.EXE in Windows!" );
        else
            puts( "SUCCESS!" );
    }

    /* return success--even though we may have failed */
    return ( TRUE );
} /* main() */


/** EOF: postdos.c **/
