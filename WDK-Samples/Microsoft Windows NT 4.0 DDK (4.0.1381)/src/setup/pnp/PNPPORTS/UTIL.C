/** FILE: util.c *********** Module Header ********************************
 *
 *  Ports applet utility library routines. This file contains string,
 *  cursor, SendWinIniChange() routines.
 *
 *  Copyright (C) 1990-1996 Microsoft Corporation
 *
 *************************************************************************/
/* Notes -

    Global Functions:

      U T I L I T Y

        BackslashTerm () - add backslash char to path
        DoDialogBoxParam () - invoke dialog
        ErrMemDlg () - display Memory Error message box
        HourGlass () - Turn on/off hour glass cursor
        myatoi () - local implementation of atoi for Unicode strings
        MyItoa () - To convert from ANSI to Unicode string after calling itoa
        MyMessageBox () - display message to user, with parameters
        RestartDlg () - display "Restart System" message, restart system
        SendWinIniChange () - broadcast system change message via USER
        strscan () - Find a string within another string
        StripBlanks () - Strip leading and trailing blanks from a string


    Local Functions:

 */

//==========================================================================
//                                Include files
//==========================================================================

// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Application specific
#include "pnpports.h"


#define INT_SIZE_LENGTH   20
#define LONG_SIZE_LENGTH  40


LPTSTR BackslashTerm( LPTSTR pszPath )
{
    LPTSTR pszEnd;

    pszEnd = pszPath + lstrlen( pszPath );

    //
    //  Get the end of the source directory
    //

    switch( *CharPrev( pszPath, pszEnd ) )
    {
    case TEXT( '\\' ):
    case TEXT( ':' ):
        break;

    default:
        *pszEnd++ = TEXT( '\\' );
        *pszEnd = TEXT( '\0' );
    }
    return( pszEnd );
}


//
//  This does what is necessary to bring up a dialog box
//

int DoDialogBoxParam( int nDlg,
                      HWND hParent,
                      DLGPROC lpProc,
                      DWORD dwParam)
{
    nDlg = DialogBoxParam( g_hInst, (LPTSTR) MAKEINTRESOURCE( nDlg ),
                           hParent, lpProc, dwParam);

    if( nDlg == -1 )
        MyMessageBox( hParent, INITS, INITS+1, IDOK );

    return( nDlg );
}


void ErrMemDlg( HWND hParent )
{
    MessageBox( hParent, g_szErrMem, g_szPortsApplet,
                MB_OK | MB_ICONHAND | MB_SYSTEMMODAL );
}


//
//  Turn hourglass on or off
//

void HourGlass( BOOL bOn )
{
    if( !GetSystemMetrics( SM_MOUSEPRESENT ) )
        ShowCursor( bOn );

    SetCursor( LoadCursor( NULL, bOn ? IDC_WAIT : IDC_ARROW ) );
}


int myatoi( LPTSTR pszInt )
{
    int   retval;
    TCHAR cSave;

    for( retval = 0; *pszInt; ++pszInt )
    {
        if( ( cSave = (TCHAR) ( *pszInt - TEXT( '0' ) ) ) > (TCHAR) 9 )
            break;
        retval = (int) ( retval * 10 + (int) cSave );
    }
    return( retval );
}



///////////////////////////////////////////////////////////////////////////////
//
//   MyItoa
//
//   Desc:  To convert from ANSI to Unicode string after calling
//          CRT itoa function.
//
///////////////////////////////////////////////////////////////////////////////

LPTSTR MyItoa( INT value, LPTSTR  string, INT  radix )
{
   CHAR   szAnsi[ INT_SIZE_LENGTH ];

#ifdef UNICODE

   _itoa( value, szAnsi, radix );
   MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szAnsi, -1,
                        string, INT_SIZE_LENGTH );
#else

   _itoa( value, string, radix );

#endif

   return( string );

} // end of MyItoa()


int MyMessageBox( HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ... )
{
    TCHAR   szText[ 4 * PATHMAX ], szCaption[ 2 * PATHMAX ];
    int     ival;
    va_list parg;

    va_start( parg, wType );

    if( wText == INITS )
        goto NoMem;

    if( !LoadString( g_hInst, wText, szCaption, CharSizeOf( szCaption ) ) )
        goto NoMem;

    wvsprintf( szText, szCaption, parg );

    if( !LoadString( g_hInst, wCaption, szCaption, CharSizeOf( szCaption ) ) )
        goto NoMem;

    if( (ival = MessageBox( hWnd, szText, szCaption, wType ) ) == 0 )
        goto NoMem;

    va_end( parg );

    return( ival );

NoMem:
    va_end( parg );

    ErrMemDlg( hWnd );
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// EnablePrivilege
//
// The following function enables the specified privilege.
//
// Parameters:
//
// PrivilegeName - Specifies the name of the privilege to be enabled.
//
// Return Value: TRUE if the call is successful, FALSE otherwise.
//
///////////////////////////////////////////////////////////////////////////////

BOOL
EnablePrivilege( LPCTSTR PrivilegeName )
{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}


///////////////////////////////////////////////////////////////////////////////
//
// RestartDlg
//
// The following function is the dialog procedure for bringing up a system
// restart message.  This dialog is called whenever the user is advised to
// reboot the system.  The dialog contains an IDOK and IDCANCEL button, which
// instructs the function whether to restart windows immediately.
//
// Parameters:
//
// lParam - The LOWORD portion contains an index to the resource string
//          used to compose the restart message.  This string is inserted
//          before the string IDS_RESTART.
//
// Return Value: The usual dialog return value.
//
///////////////////////////////////////////////////////////////////////////////

BOOL RestartDlg( HWND hDlg, UINT message, DWORD wParam, LONG lParam )
{
    TCHAR   szMessage[ 200 ];
    TCHAR   szTemp[ 100 ];

    switch (message)
    {

        case WM_INITDIALOG:

            //
            //  Set up the restart message
            //

            LoadString( g_hInst, LOWORD(lParam), szMessage,
                        CharSizeOf( szMessage ) );

            if( LoadString( g_hInst, IDS_RESTART, szTemp, CharSizeOf( szTemp ) ) )
                lstrcat( szMessage, szTemp );

            SetDlgItemText( hDlg, RESTART_TEXT, szMessage );
            break;

        case WM_COMMAND:

            switch( LOWORD( wParam ) )
            {
                case IDOK:
                    EnablePrivilege(SE_SHUTDOWN_NAME);
                    ExitWindowsEx( EWX_REBOOT | EWX_SHUTDOWN, (DWORD) (-1) );
                    break;

                case IDCANCEL:
                    EndDialog( hDlg, 0L );
                    break;

                default:
                    return( FALSE );
            }
            return( TRUE );

        default:
            return( FALSE );
    }

    return( FALSE );
}


void SendWinIniChange( LPTSTR lpSection )
{
    SendMessageTimeout( (HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection,
                        SMTO_ABORTIFHUNG, 1000, NULL );
}


LPTSTR strscan( LPTSTR pszString, LPTSTR pszTarget )
{
    LPTSTR psz;

    if( psz = _tcsstr( pszString, pszTarget ) )
        return( psz );
    else
        return( pszString + lstrlen( pszString ) );
}


///////////////////////////////////////////////////////////////////////////////
//
//  StripBlanks()
//
//   Strips leading and trailing blanks from a string.
//   Alters the memory where the string sits.
//
///////////////////////////////////////////////////////////////////////////////

void StripBlanks( LPTSTR pszString )
{
    LPTSTR  pszPosn;

    //
    //  strip leading blanks
    //

    pszPosn = pszString;

    while( *pszPosn == TEXT( ' ' ) )
        pszPosn++;

    if( pszPosn != pszString );
        lstrcpy( pszString, pszPosn );

    //
    //  strip trailing blanks
    //

    if( ( pszPosn = pszString + lstrlen( pszString ) ) != pszString )
    {
       pszPosn = CharPrev( pszString, pszPosn );

       while( *pszPosn == TEXT( ' ' ) )
           pszPosn = CharPrev( pszString, pszPosn );

       pszPosn = CharNext( pszPosn );

       *pszPosn = TEXT( '\0' );
    }
}


