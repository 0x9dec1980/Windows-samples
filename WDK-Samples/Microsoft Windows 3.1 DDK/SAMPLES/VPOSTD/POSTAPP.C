/*

     (C) Copyright MICROSOFT Corp., 1991

*/

/** postapp.c                                                           **/


/* the includes we need */
    #include <windows.h>
    #include "posthost.h"
    #include "postapp.h"


/* prototypes for good measure */
    long FAR PASCAL WndProc( HWND, unsigned, WORD, LONG );


/* globals, no less */
    char    szAppName[] = "POSTAPP";



/** long FAR PASCAL WndProc( HWND, unsigned, WORD, LONG )
 *
 *  DESCRIPTION: 
 *      This is just your normal everyday WndProc().
 *
 *  NOTES:
 *
 ** cjp */

long FAR PASCAL WndProc( HWND hWnd, unsigned iMsg, WORD wParam, LONG lParam )
{
    switch ( iMsg ) 
    {
        case WM_DESTROY:
            if ( !phRegisterWindow( NULL ) )
                MessageBox( NULL, "Could not de-register window!",
                                        "phRegisterWindow", MB_OK );
                
            PostQuitMessage( 0 );
        break;

        case WM_COMMAND:
        {
            char    szBuf[ 256 ];

            switch ( wParam )
            {
                case IDM_GETVERSION:
                {
                    WORD wVersion = phGetVPOSTDVersion();

                    wsprintf( szBuf, "VPOSTD Version: %.4Xh", wVersion );
                    MessageBox( hWnd, szBuf, "phGetVPOSTDVersion", MB_OK );
                }
                break;
                
                case IDM_GETAPIADDR:
                {
                    DWORD dwAPIAddr = phGetVPOSTDAPIAddr();

                    wsprintf( szBuf, "VPOSTD API Addr: %.8lXh", dwAPIAddr );
                    MessageBox( hWnd, szBuf, "phGetVPOSTDAPIAddr", MB_OK );
                }
                break;

                case IDM_CALLPOSTHOST:
                    if ( !phCallPostHost( 0x12345678 ) )
                        MessageBox( hWnd, "UH-OH!", "phCallPostHost", MB_OK );
                break;
            }
        }
        break;

        case WM_POSTHOSTPOST:
        {
            char    szBuf[ 256 ];

            wsprintf( szBuf, "VM ID: %d   lParam: %.8lXh", wParam, lParam );
            MessageBox( hWnd, szBuf, "WM_POSTHOSTPOST",
                                        MB_OK | MB_SYSTEMMODAL );
        }
        break;

        default:
            return ( DefWindowProc( hWnd, iMsg, wParam, lParam ) );
    }

    return ( 0L );
} /* WndProc() */


/** int PASCAL WinMain( HANDLE, HANDLE, LPSTR, int )
 *
 *  DESCRIPTION: 
 *      This is just your normal everyday WinMain().
 *
 *  NOTES:
 *
 ** cjp */

int PASCAL WinMain( HANDLE  hInstance,
                    HANDLE  hPrevInstance,
                    LPSTR   lpszCmdLine,
                    int     nCmdShow )
{
    WNDCLASS    wndclass;
    HWND        hWnd;
    MSG         msg;
    WORD        wError;

    /* get the status of POSTHOST.DLL--hope for PHERR_NOERROR (0) */
    if ( (wError = phGetPostHostError()) != PHERR_NOERROR )
    {
        char    *szError;

        /* error checking and reporting */
        if ( wError == PHERR_NO386ENH )
            szError = "RUN ENHANCED WINDOWS!";

        else if ( wError == PHERR_NOVPOSTD )
            szError = "INSTALL VPOSTD.386 FIRST!";

        MessageBox( NULL, szError, "POSTHOST.DLL", MB_OK );

        return ( FALSE );
    }

    if ( !hPrevInstance ) 
    {
        wndclass.style         = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc   = WndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = hInstance;
        wndclass.hIcon         = LoadIcon( hInstance, "ICON_POSTAPP" );
        wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW );
        wndclass.hbrBackground = GetStockObject( WHITE_BRUSH );
        wndclass.lpszMenuName  = "MENU_POSTAPP";
        wndclass.lpszClassName = szAppName;

        if ( !RegisterClass( &wndclass ) )
            return ( FALSE );
    }

    hWnd = CreateWindow( szAppName, "PostApp",             
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, 400, 60,
                         NULL, NULL, hInstance, NULL );             

    ShowWindow( hWnd, SW_SHOW );
    UpdateWindow( hWnd );


    if ( !phRegisterWindow( hWnd ) )
        MessageBox( hWnd, "UH-OH!  Could not register window!",
                                        "phRegisterWindow", MB_OK );

    while ( GetMessage( &msg, NULL, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    return ( msg.wParam );
} /* WinMain() */


/** EOF: postapp.c **/
