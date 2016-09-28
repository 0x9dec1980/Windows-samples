/** FILE: pnpports.c ********** Module Header ********************************
 *
 *  Control panel applet for configuring COM ports.  This file contains
 *  the dialog and utility functions for managing the UI for setting COM
 *  port parameters
 *
 *  Copyright (C) 1990-1996 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "pnpports.h"

//==========================================================================
//                            Local Structures
//==========================================================================
typedef struct _GENERAL_PROP_PARAMS {
    DWORD            ComPortNumber;
    BOOL             FifoEnabled;
    BOOL             FifoChanged;
    HDEVINFO         DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
} GENERAL_PROP_PARAMS, *PGENERAL_PROP_PARAMS;

//==========================================================================
//                            Local Definitions
//==========================================================================
#define PORTS        4
#define MAXPORTS    32
#define KEYBZ       4096

#define DEF_BAUD    3       //  1200
#define DEF_WORD    4       //  8 bits
#define DEF_PARITY  2       //  None
#define DEF_STOP    0       //  1
#define DEF_PORT    0       //  Null Port
#define DEF_SHAKE   2       //  None
#define PAR_EVEN    0
#define PAR_ODD     1
#define PAR_NONE    2
#define PAR_MARK    3
#define PAR_SPACE   4
#define STOP_1      0
#define STOP_15     1
#define STOP_2      2
#define FLOW_XON    0
#define FLOW_HARD   1
#define FLOW_NONE   2

#define CURRENT_SET 1       // TRUE if Registry key CurrentControlSet works

#define MAX_COM_PORT  256   // Maximum number of COM ports NT supports
#define MIN_COM         1   // Minimum new COM port number
#define MIN_SERIAL  10000   // Minimum new registry "Serial" value


//==========================================================================
//                            External Declarations
//==========================================================================


//==========================================================================
//                            Local Data Declarations
//==========================================================================


BOOL bNewPort    = FALSE;
BOOL bResetTitle = FALSE;
BOOL bResetLB    = FALSE;

TCHAR  m_szColon[]      = TEXT( ":" );
TCHAR  m_szComma[]      = TEXT( "," );
TCHAR  m_szCloseParen[] = TEXT( ")" );
TCHAR  m_szPorts[]      = TEXT( "Ports" );
TCHAR  m_szCOM[]        = TEXT( "COM" );
TCHAR  m_szSERIAL[]     = TEXT( "Serial" );

TCHAR  m_szComPort[ 20 ];
TCHAR  m_szSerialKey[ 40 ];
BOOL   m_PortIsPnP;

//
//  NT Registry keys to find COM port to Serial Device mapping
//

TCHAR m_szRegSerialMap[] = TEXT( "Hardware\\DeviceMap\\SerialComm" );

//
//  Registry Serial Port Advanced I/O settings key and valuenames
//

TCHAR m_szRegSerial[]  =
            TEXT( "System\\CurrentControlSet\\Services\\Serial" );

TCHAR m_szRegSerialParam[]  =
            TEXT( "System\\CurrentControlSet\\Services\\Serial\\Parameters" );

TCHAR m_szRegSerialIO[]  =
          TEXT( "System\\CurrentControlSet\\Services\\Serial\\Parameters\\%s" );

TCHAR m_szParametersSerialIO[]  =
          TEXT( "Parameters\\%s" );

TCHAR m_szRegHwDesc[] =
          TEXT( "Hardware\\Description\\System\\" );

TCHAR m_szRegResourceMap[] =
          TEXT( "HARDWARE\\RESOURCEMAP\\LOADED SERIAL DRIVER RESOURCES\\Serial" );

TCHAR m_szSerialController[]     = TEXT( "SerialController" );
TCHAR m_szConfigurationData[]    = TEXT( "Configuration Data" );
TCHAR m_szIdentifier[]           = TEXT( "Identifier" );
TCHAR m_szMultifunctionAdapter[] = TEXT( "MultifunctionAdapter" );
TCHAR m_szEisaAdapter[]          = TEXT( "EisaAdapter" );
TCHAR m_szDeviceSerial[]         = TEXT( "\\Device\\Serial" );
TCHAR m_szDeviceSerialSuffix[]   = TEXT( ".Raw" );

TCHAR m_szRegPortAddress[] = TEXT( "PortAddress" );
TCHAR m_szRegPortIRQ[]     = TEXT( "Interrupt" );
TCHAR m_szFIFO[]           = TEXT( "ForceFifoEnable" );
TCHAR m_szDosDev[]         = TEXT( "DosDevices" );
TCHAR m_szPnPDeviceId[]    = TEXT( "PnPDeviceId" );
TCHAR m_szPortName[]       = TEXT("PortName");
TCHAR m_szSetupApiDll[]    = TEXT( "setupapi.dll" );

int m_nBaudRates[] = { 75, 110, 134, 150, 300, 600, 1200, 1800, 2400,
                       4800, 7200, 9600, 14400, 19200, 38400, 57600,
                       115200, 128000, 0 };

TCHAR m_sz9600[] = TEXT( "9600" );

TCHAR m_szDefParams[] = TEXT( "9600,n,8,1" );

short m_nDataBits[] = { 4, 5, 6, 7, 8, 0};

TCHAR *m_pszParitySuf[] = { TEXT( ",e" ),
                            TEXT( ",o" ),
                            TEXT( ",n" ),
                            TEXT( ",m" ),
                            TEXT( ",s" ) };

TCHAR *m_pszLenSuf[] = { TEXT( ",4" ),
                         TEXT( ",5" ),
                         TEXT( ",6" ),
                         TEXT( ",7" ),
                         TEXT( ",8" ) };

TCHAR *m_pszStopSuf[] = { TEXT( ",1" ),
                          TEXT( ",1.5" ),
                          TEXT( ",2 " ) };

TCHAR *m_pszFlowSuf[] = { TEXT( ",x" ),
                          TEXT( ",p" ),
                          TEXT( " " ) };


//==========================================================================
//                            Local Function Prototypes
//==========================================================================

BOOL
CommDlg(
    IN HWND   hDlg,
    IN UINT   message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

HDEVINFO
OpenPnPSerialDeviceInstance(
    IN  HWND             hwndParent,
    IN  LPCTSTR          SerialKeyName,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    );

INT
DoPnPAdvancedPortDialog(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  HWND             hwndParent,
    IN  BOOL             FirstTime,
    OUT PHKEY            phPortKey       OPTIONAL
    );

DWORD
InstallPnPSerialPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

BOOL
GetSerialPortDevInstConfig(
    IN  DEVINST       DevInst,
    IN  ULONG         LogConfigType,
    OUT PIO_RESOURCE  IoResource,
    OUT PIRQ_RESOURCE IrqResource
    );

VOID
GetComPortForSerialDevice(
    IN     INT  SerialDeviceId,
    IN OUT PINT ComPortNumber
    );

BOOL
CALLBACK
AddPropSheetPageProc(
    IN HPROPSHEETPAGE hpage,
    IN LPARAM lParam
   );

BOOL
GeneralAdvPortsDlg(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    );


//==========================================================================
//                                Functions
//==========================================================================

BOOL ChkCommSettings( HWND hDlg )
{
    TCHAR    szTest[ 133 ];
    TCHAR    *pszVerify;

    SendDlgItemMessage( hDlg, PORT_BAUDRATE, WM_GETTEXT, 80, (LPARAM) szTest );

    for( pszVerify = szTest; *pszVerify != TEXT( '\0' ); pszVerify++ )
    {
        if( ( *pszVerify < TEXT( '0' ) ) || ( *pszVerify > TEXT( '9' ) ) )
        {
            if( !LoadString( g_hInst, ERRORS, szTest, CharSizeOf( szTest ) ) )
                ErrMemDlg( hDlg );
            else
                MessageBox( hDlg, szTest, g_szPortsApplet,
                            MB_OK | MB_ICONINFORMATION );

            //
            //  ERROR EXIT
            //

            return( FALSE );
        }
    }
    return( TRUE );
}


/*
BOOL CheckBaseIOSetting( LPTSTR lpszBaseIO )
{
    LPTSTR lpch;
    BOOL   bRet = TRUE;

    CharUpper( lpszBaseIO );

    for( lpch = lpszBaseIO; *lpch; lpch++ )
        if( ( *lpch < TEXT( '0' ) || *lpch > TEXT( '9' ) )
            && (*lpch < TEXT( 'A' ) || *lpch > TEXT( 'F' ) ) )
        {
            bRet = FALSE;
            break;
        }

    return( bRet );
}
*/

//
//  Set dialog items to defaults from win.ini for given port
//  Port is zero based, 0 = com1, 1 = com2, etc.
//
//  void SetFromWin( HWND hDlg, int Port )
//

void SetFromWin( HWND hDlg )
{
    TCHAR  szParms[ 81 ];               // parms from win.ini for port
    TCHAR *pszCur, *pszNext;
    int    nIndex;
    int    baud;

//  m_szComPort[3] = (TCHAR) ( TEXT( '1' ) + Port );

    GetProfileString( m_szPorts, m_szComPort, g_szNull, szParms, 81 );

    StripBlanks( szParms );

    pszCur = szParms;

    //
    //  baud rate
    //

    pszNext = strscan( pszCur, m_szComma );

    if( *pszNext )
        *pszNext++ = 0;

    //
    // Find current Baud Rate selection
    //

    nIndex = (short) SendDlgItemMessage( hDlg, PORT_BAUDRATE,
                                         CB_FINDSTRING,
                                         (WPARAM) -1,
                                         (LPARAM) pszCur );

    nIndex = (nIndex == CB_ERR) ? 0 : nIndex;

    SendDlgItemMessage( hDlg, PORT_BAUDRATE, CB_SETCURSEL, nIndex, 0L );

    pszCur = pszNext;

    //
    //  parity
    //

    pszNext = strscan( pszCur, m_szComma );

    if( *pszNext )
        *pszNext++ = 0;

    StripBlanks( pszCur );

    switch( *pszCur )
    {
    case TEXT( 'o' ):
        nIndex = PAR_ODD;
        break;

    case TEXT( 'e' ):
        nIndex = PAR_EVEN;
        break;

    case TEXT( 'n' ):
        nIndex = PAR_NONE;
        break;

    case TEXT( 'm' ):
        nIndex = PAR_MARK;
        break;

    case TEXT( 's' ):
        nIndex = PAR_SPACE;
        break;

    default:
        nIndex = DEF_PARITY;
        break;
    }

    SendDlgItemMessage( hDlg, PORT_PARITY, CB_SETCURSEL, nIndex, 0L );
    pszCur = pszNext;

    //
    //  word length
    //

    pszNext = strscan( pszCur, m_szComma );

    if( *pszNext )
        *pszNext++ = 0;

    StripBlanks( pszCur );
    nIndex = ( *pszCur - TEXT( '4' ) );

    if ( (nIndex >= 0) && (nIndex <= 4) )
        SendDlgItemMessage( hDlg, PORT_DATABITS, CB_SETCURSEL, nIndex, 0L );
    else
        SendDlgItemMessage( hDlg, PORT_DATABITS, CB_SETCURSEL, DEF_WORD, 0L );

    pszCur = pszNext;

    //
    //  stop bits
    //

    pszNext = strscan( pszCur, m_szComma );

    if( *pszNext )
        *pszNext++ = 0;

    StripBlanks( pszCur );

    if( !lstrcmp( pszCur, TEXT( "1" ) ) )
        SendDlgItemMessage( hDlg, PORT_STOPBITS, CB_SETCURSEL, STOP_1, 0L );
    else if( !lstrcmp( pszCur, TEXT( "1.5" ) ) )
        SendDlgItemMessage( hDlg, PORT_STOPBITS, CB_SETCURSEL, STOP_15, 0L );
    else if( !lstrcmp( pszCur, TEXT( "2" ) ) )
        SendDlgItemMessage( hDlg, PORT_STOPBITS, CB_SETCURSEL, STOP_2, 0L );
    else
        SendDlgItemMessage( hDlg, PORT_STOPBITS, CB_SETCURSEL, DEF_STOP, 0L );

    pszCur = pszNext;

    //
    //  handshaking: Hardware, xon/xoff, or none
    //

    pszNext = strscan( pszCur, m_szComma );

    if( *pszNext )
        *pszNext++ = 0;

    StripBlanks( pszCur );

    if( *pszCur == TEXT( 'p' ) )
        SendDlgItemMessage( hDlg, PORT_FLOWCTL, CB_SETCURSEL, FLOW_HARD, 0L );
    else if( *pszCur == TEXT( 'x' ) )
        SendDlgItemMessage( hDlg, PORT_FLOWCTL, CB_SETCURSEL, FLOW_XON, 0L );
    else
        SendDlgItemMessage( hDlg, PORT_FLOWCTL, CB_SETCURSEL, FLOW_NONE, 0L );

    return;
}


//
//  This routine reads off the settings from the dialog and writes them
//  out to win.ini
//
//  This routine was completely overhauled to work with the new 3.1 Dialog
//  box.  Modified by C. Stevens, Oct. 90
//

void CommPortsToWin( HWND hDlg )
{
    TCHAR szBuild[ PATHMAX ];
    int   i;

    //
    //  Get the baud rate
    //

    i = SendDlgItemMessage( hDlg, PORT_BAUDRATE, WM_GETTEXT, 18,
                            (LPARAM) szBuild );

    if( i == 0 )
        return;

    //
    //  Get the parity setting
    //

    i = SendDlgItemMessage( hDlg, PORT_PARITY, CB_GETCURSEL, 0, 0L );

    if( ( i == CB_ERR ) || ( i == CB_ERRSPACE ) )
        return;

    lstrcat( szBuild, m_pszParitySuf[ i ] );

    //
    //  Get the word length
    //

    i = SendDlgItemMessage( hDlg, PORT_DATABITS, CB_GETCURSEL, 0, 0L );

    if( ( i == CB_ERR ) || ( i == CB_ERRSPACE ) )
        return;

    lstrcat( szBuild, m_pszLenSuf[ i ] );

    //
    //  Get the stop bits
    //

    i = SendDlgItemMessage( hDlg, PORT_STOPBITS, CB_GETCURSEL, 0, 0L );

    if( ( i == CB_ERR ) || ( i == CB_ERRSPACE ) )
        return;

    lstrcat( szBuild, m_pszStopSuf[ i ] );

    //
    //  Get the flow control
    //

    i = SendDlgItemMessage( hDlg, PORT_FLOWCTL, CB_GETCURSEL, 0, 0L );

    if( ( i == CB_ERR ) || ( i == CB_ERRSPACE ) )
        return;

    lstrcat( szBuild, m_pszFlowSuf[ i ] );

    //
    //  Write settings string to [ports] section in win.ini
    //

    WriteProfileString( m_szPorts, m_szComPort, szBuild );

    SendWinIniChange( (LPTSTR) m_szPorts );
}


BOOL ShortCommDlg( HWND hDlg, UINT message, DWORD wParam, LONG lParam )
{
    int    i, j, nEntries;
    DWORD  dwSize, dwBufz;
    DWORD  dwType;
    HKEY   hkey, hkeySub;
    TCHAR  szSerial[ 40 ];
    TCHAR  szCom[ 40 ];
    BOOL   PortIsPnP;
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    DEVINST DevInst;

    //
    //  1 based array of all allowable COM ports
    //

    BOOL   bPorts[ MAX_COM_PORT+1 ];

    switch( message )
    {

    case WM_INITDIALOG:
        HourGlass( TRUE );

        //
        // Init globals
        //

        bResetTitle = FALSE;
        bResetLB    = FALSE;

        bNewPort = FALSE;

        //////////////////////////////////////////////////////////////////////
        //  Create a list of PnP COM ports under Services\Serial\Parameters.
        //////////////////////////////////////////////////////////////////////

        //
        //  Always clear BOOL array
        //
        ZeroMemory(bPorts, sizeof(bPorts));

        //
        //  Reset list box before display.
        //
        SendDlgItemMessage( hDlg, PORT_LB, LB_RESETCONTENT, 0, 0L );

        //
        //  Read Serial keys at Services Node
        //

        if( !RegOpenKeyEx( HKEY_LOCAL_MACHINE,        // Root key
                           m_szRegSerialParam,        // Subkey to open
                           0L,                        // Reserved
                           KEY_READ,                  // SAM
                           &hkey ) )                  // return handle
        {
            //
            //  Enumerate all keys under Serial\Parameters
            //

            i = 0;

            while( RegEnumKey (hkey, i++, szSerial, CharSizeOf( szSerial ) )
                                                     != ERROR_NO_MORE_ITEMS )
            {
                hkeySub = NULL;

                if( !RegOpenKeyEx( hkey,                      // Root key
                                   szSerial,                  // Subkey to open
                                   0L,                        // Reserved
                                   KEY_READ,                  // SAM
                                   &hkeySub ) )               // return handle
                {
                    //
                    //  Get DosDevices value for this Serial key
                    //

                    dwSize = sizeof( szCom );

                    if( !RegQueryValueEx( hkeySub, m_szDosDev, NULL, &dwType,
                                          (LPBYTE) szCom, &dwSize ) )
                    {
                        if( dwType != REG_SZ )
                            goto TryNextSubKey;

                        //
                        // Check for the existence of a 'PnPDeviceId" value entry
                        // in this key.  If present, this value indicates what the
                        // associated Plug&Play device instance is for this PnP ISA
                        // enumerated device.
                        //
                        dwSize = sizeof(DeviceInstanceId);
                        PortIsPnP = ((RegQueryValueEx(hkeySub,
                                                      m_szPnPDeviceId,
                                                      NULL,
                                                      &dwType,
                                                      (PBYTE)DeviceInstanceId,
                                                      &dwSize) == ERROR_SUCCESS)
                                      && (dwType == REG_SZ));

                        if(PortIsPnP)
                        {
                            //
                            // Check to see if this device is currently present.
                            //
                            if(CM_Locate_DevInst(&DevInst,
                                                 (DEVINSTID)DeviceInstanceId,
                                                 CM_LOCATE_DEVINST_NORMAL) != CR_SUCCESS) {
                                //
                                // Not present--skip this device.
                                //
                                goto TryNextSubKey;
                            }

                        } else {
                            //
                            // This sample applet only handles PnP COM ports.
                            //
                            goto TryNextSubKey;
                        }

                        //
                        //  Append ":" char to end of szCom string
                        //

                        lstrcat( szCom, m_szColon );

                        //
                        //  Get number of COM port (go past "COM" in string)
                        //

                        j = myatoi( &szCom[ 3 ] );

                        if( ( j <= MAX_COM_PORT ) && !bPorts[ j ] )
                        {
                            bPorts[ j ] = TRUE;
                        }
                        else
                        {
                            goto TryNextSubKey;
                        }

                        //
                        //  Put Port name string in ListBox
                        //

                        if( ( j = (int) SendDlgItemMessage (hDlg,
                                                            PORT_LB,
                                                            LB_INSERTSTRING,
                                                            (WPARAM) -1,
                                                            (LPARAM) szCom ) )
                                                          >= 0 )
                        {
                            SendDlgItemMessage( hDlg,
                                                SERIAL_DBASE,
                                                LB_INSERTSTRING,
                                                j,
                                                (LPARAM) szSerial );
                            //
                            // Set item data associated with this list entry to indicate
                            // whether or not it's a PnP ISA device.  (For this sample, it
                            // will always be a PnP device, as we've filtered non-PnP devices
                            // out in the code above.
                            //
                            SendDlgItemMessage( hDlg, SERIAL_DBASE, LB_SETITEMDATA,
                                                j, (LPARAM) PortIsPnP );

                            ++nEntries;
                        }
                    }
TryNextSubKey:
                    RegCloseKey( hkeySub );
                }
            }
            RegCloseKey( hkey );
        }

        //
        //  By default, select the first item in the Listbox
        //

        SendDlgItemMessage( hDlg, PORT_LB, LB_SETCURSEL, 0, 0L );


        /////////////////////////////////////////////////////////////////
        //  Check for Greying out DELETE button
        /////////////////////////////////////////////////////////////////

        //
        //  Check for registry access to the Services nodes for deleting ports.
        //

        if( (RegOpenKeyEx( HKEY_LOCAL_MACHINE,        // Root key
                           m_szRegSerialParam,        // Subkey to open
                           0L,                        // Reserved
                           KEY_READ | KEY_WRITE,      // SAM
                           &hkey )                    // return handle
                != ERROR_SUCCESS ) )
        {
            EnableWindow( GetDlgItem( hDlg, PORT_DELETE ), FALSE );
            RegCloseKey( hkey );
        }

        HourGlass( FALSE );
        return TRUE;


    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {
        case IDOK:
        case IDCANCEL:
            EndDialog( hDlg, LOWORD( wParam ) );
            break;

        case PORT_LB:
            //
            //  If user Double Clicks on listbox item, react as though they
            //  clicked on "Settings" button
            //

            if( HIWORD( wParam ) != LBN_DBLCLK )
                break;

            //
            //  fall through...
            //

        case PORT_SETTING:
            i = SendDlgItemMessage( hDlg, PORT_LB, LB_GETCURSEL, 0, 0L );

            if( i != LB_ERR )
            {
                //
                // Set the global COM PORT string for later use
                //

                SendDlgItemMessage( hDlg, PORT_LB, LB_GETTEXT, i,
                                    (LPARAM) m_szComPort );

                SendDlgItemMessage( hDlg, SERIAL_DBASE, LB_GETTEXT, i,
                                    (LPARAM) m_szSerialKey );

                //
                // Retrieve the item data associated with the serial key entry, to
                // determine whether this port is a PnP ISA device.
                //
                m_PortIsPnP = SendDlgItemMessage( hDlg, SERIAL_DBASE, LB_GETITEMDATA, i, 0 );
                if( m_PortIsPnP == LB_ERR )
                {
                    m_PortIsPnP = FALSE;
                }

                if( DoDialogBoxParam( DLG_PORTS2, hDlg, (DLGPROC) CommDlg, 0L ) == IDOK )
                {
                    SetDlgItemText( hDlg, IDOK, g_szClose );
                }

                if( bResetLB )
                {
                    //
                    // User changed COM Port Number, delete old, add new
                    //

                    SendDlgItemMessage( hDlg, PORT_LB, LB_DELETESTRING, i, 0L );
                    SendDlgItemMessage( hDlg, SERIAL_DBASE, LB_DELETESTRING,
                                        i, 0L );

                    //
                    // Put New Port and Serial name strings in ListBoxes
                    //

                    if( ( j = (int) SendDlgItemMessage( hDlg,
                                                        PORT_LB,
                                                        LB_INSERTSTRING,
                                                        (WPARAM)(LONG)-1,
                                                        (LPARAM) m_szComPort) )
                                                      >= 0)
                    {
                        SendDlgItemMessage( hDlg, SERIAL_DBASE, LB_INSERTSTRING,
                                            j, (LPARAM) m_szSerialKey );

                        SendDlgItemMessage( hDlg, SERIAL_DBASE, LB_SETITEMDATA,
                                            j, (LPARAM) m_PortIsPnP );
                    }

                    j = (j > 0) ? j : 0;

                    //
                    // Select an item in the Listbox
                    //

                    SendDlgItemMessage( hDlg, PORT_LB, LB_SETCURSEL, j, 0L );

                    //
                    // Reset globals
                    //

                    bResetTitle = FALSE;
                    bResetLB    = FALSE;
                }
            }
            break;

        case PORT_DELETE:
        {
            LONG err1, err2;
            /////////////////////////////////////////////////////////////////
            //  Check for registry access to both the DeviceMap and
            //  Services nodes
            /////////////////////////////////////////////////////////////////

            err1 = RegOpenKeyEx( HKEY_LOCAL_MACHINE,        // Root key
                               m_szRegSerialMap,          // Subkey to open
                               0L,                        // Reserved
                               KEY_READ | KEY_WRITE,      // SAM
                               &hkey );                   // return handle

            if (err1 != ERROR_SUCCESS ) {
                hkey = NULL;
            }

            err2 = RegOpenKeyEx( HKEY_LOCAL_MACHINE,        // Root key
                               m_szRegSerialParam,        // Subkey to open
                               0L,                        // Reserved
                               KEY_READ | KEY_WRITE,      // SAM
                               &hkeySub );                // return handle

            if ( err2 != ERROR_SUCCESS ) {
                hkeySub = NULL;
            }

            // Could not open either key, assume that we don't have permission
            if ((hkey == NULL && hkeySub == NULL) || err1 == ERROR_ACCESS_DENIED || err2 == ERROR_ACCESS_DENIED) {

                MyMessageBox( hDlg, IDS_MYPORT7, INITS+1,
                              MB_OKCANCEL | MB_ICONEXCLAMATION );
                break;
            }


            /////////////////////////////////////////////////////////////////
            //  Confirm deletion
            /////////////////////////////////////////////////////////////////

            i = MyMessageBox( hDlg, IDS_MYPORT6, INITS+1,
                              MB_OKCANCEL | MB_ICONEXCLAMATION );

            /////////////////////////////////////////////////////////////////
            //  Delete everything associated with COM port
            /////////////////////////////////////////////////////////////////

            if( i == IDOK )
            {
                SetDlgItemText( hDlg, IDOK, g_szClose );

                //
                //  Get current selection
                //

                i = SendDlgItemMessage( hDlg, PORT_LB, LB_GETCURSEL, 0, 0L );

                SendDlgItemMessage( hDlg, SERIAL_DBASE, LB_GETTEXT, i,
                                    (LPARAM) szSerial );

                SendDlgItemMessage( hDlg, PORT_LB, LB_GETTEXT, i,
                                    (LPARAM) szCom );

                //
                // Retrieve the item data associated with the serial key entry, to
                // determine whether this port is a PnP ISA device.
                //
                PortIsPnP = SendDlgItemMessage( hDlg, SERIAL_DBASE, LB_GETITEMDATA, i, 0 );
                if( PortIsPnP == LB_ERR )
                {
                    PortIsPnP = FALSE;
                }

                //
                //  Delete Registry entries for this COM port
                //
                //  Delete Value entry in DeviceMap\SERIALCOMM key
                //  Delete entire key under Services\Serial\Parameters node
                //

                if (hkey != NULL) {
                    RegDeleteValue( hkey, szSerial );
                    RegCloseKey( hkey );
                }

                if (hkeySub != NULL) {

                    if (PortIsPnP) {
                        //
                        // This is a PnP ISA port, so we need to retrieve the corresponding
                        // device instance, and remove it.
                        //
                        DeviceInfoSet = OpenPnPSerialDeviceInstance(hDlg, szSerial, &DeviceInfoData);
                        if(DeviceInfoSet != INVALID_HANDLE_VALUE) {
                            SetupDiCallClassInstaller(DIF_REMOVE, DeviceInfoSet, &DeviceInfoData);
                            SetupDiDestroyDeviceInfoList(DeviceInfoSet);
                        }
                    }

                    RegDeleteKey( hkeySub, szSerial );
                    RegCloseKey( hkeySub );
                }

                //
                //  Delete WIN.INI entry - write NULL string to [ports] section
                //

                WriteProfileString( m_szPorts, szCom, (LPTSTR) NULL );

                //
                //  Delete ListBox items
                //

                SendDlgItemMessage( hDlg, PORT_LB, LB_DELETESTRING, i, 0L );
                SendDlgItemMessage( hDlg, SERIAL_DBASE, LB_DELETESTRING, i, 0L );

                //
                //  Selection next item in ListBox
                //

                j = SendDlgItemMessage( hDlg, PORT_LB, LB_GETCOUNT, 0, 0L );

                j = (j > i) ? i : j;

                //
                // Select an item in the Listbox
                //

                SendDlgItemMessage( hDlg, PORT_LB, LB_SETCURSEL, j, 0L );

                //
                // Let the rest of the world know also
                //

                SendWinIniChange( m_szPorts );
            }

            break;
        }

        }
        break;

    default:
        return FALSE;
    }

    return TRUE;

}


VOID SetCBFromRes( HWND hCB, DWORD wRes, DWORD wDef )
{
    TCHAR  szTemp[ 258 ], cSep;
    LPTSTR pThis, pThat;


    if( !LoadString( g_hInst, wRes, szTemp, CharSizeOf( szTemp ) ) )
        return;

    for( pThis = szTemp, cSep = *pThis++; pThis; pThis = pThat )
    {
        if( pThat = _tcschr( pThis, cSep ) )
            *pThat++ = TEXT( '\0' );

        SendMessage( hCB, CB_ADDSTRING, 0, (LPARAM) pThis );
    }

    SendMessage( hCB, CB_SETCURSEL, wDef, 0L );
}


///////////////////////////////////////////////////////////////////////////////
//
// CommDlg
//
//   This is the communications ports setup dialog.  It just reads all the
//   items, munges them all together into a string, and then writes it out
//   to win.ini.  This routine does not interact with the comm driver.  It
//   is the responsibility of each app that uses the comm ports to go first
//   to win.ini, read the appropriate line, and then set up the comm port
//   accordingly
//
///////////////////////////////////////////////////////////////////////////////

BOOL
CommDlg(
    IN HWND   hDlg,
    IN UINT   message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TCHAR    szTitle[ 81 ];
    TCHAR    szTitleFormat[ 81 ];
    short    nIndex;
    HWND     hParent;
    DWORD    dwMask;
    HANDLE   hComm;
    COMMPROP cpComm;

    switch( message )
    {
    case WM_INITDIALOG:
        /* init to defaults */

        // Get info about COM port from Serial driver
        hComm = CreateFile( m_szComPort,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

        if( ( hComm != INVALID_HANDLE_VALUE ) &&
              GetCommProperties( hComm, &cpComm ) )
        {
            //
            //  Check baud rate bitmask and only display settable rates
            //

            for( dwMask = 1, nIndex = 0; m_nBaudRates[ nIndex ]; nIndex++ )
            {
                //
                //  BAUD_128K comes before 115200 in bitmask, special checks
                //

                if( ( cpComm.dwSettableBaud & dwMask ) == BAUD_128K )
                {
                    //
                    //  Check for BAUD_115200 first and put it in
                    //  before this one
                    //

                    if( cpComm.dwSettableBaud & BAUD_115200 )
                    {
                        MyItoa( m_nBaudRates[ nIndex ], szTitle, 10 );

                        SendDlgItemMessage( hDlg, PORT_BAUDRATE, CB_ADDSTRING,
                                                  0, (LPARAM) szTitle );
                    }

                    //
                    //  Now increment to 128K baud rate value
                    //

                    nIndex++;

                    MyItoa( m_nBaudRates[ nIndex ], szTitle, 10 );

                    SendDlgItemMessage( hDlg, PORT_BAUDRATE, CB_ADDSTRING,
                                              0, (LPARAM) szTitle );

                    //
                    //  Move mask over another bit since we have checked
                    //  for the 115200 value
                    //

                    dwMask <<= 1;
                }
                else if( ( cpComm.dwSettableBaud & dwMask ) == BAUD_115200 )
                {
                    MyItoa( m_nBaudRates[ nIndex - 1 ], szTitle, 10 );

                    SendDlgItemMessage( hDlg, PORT_BAUDRATE, CB_ADDSTRING,
                                        0, (LPARAM) szTitle );
                }
                else if( cpComm.dwSettableBaud & dwMask )
                {
                    MyItoa( m_nBaudRates[ nIndex ], szTitle, 10 );

                    SendDlgItemMessage( hDlg, PORT_BAUDRATE, CB_ADDSTRING,
                                        0, (LPARAM) szTitle );
                }

                dwMask <<= 1;
            }
            CloseHandle( hComm );
        }
        else
        {
            //
            //  Open failure, just list all of the baud rates
            //

            for( nIndex = 0; m_nBaudRates[nIndex]; nIndex++ )
            {
                MyItoa( m_nBaudRates[ nIndex ], szTitle, 10 );

                SendDlgItemMessage( hDlg, PORT_BAUDRATE, CB_ADDSTRING, 0,
                                    (LPARAM) szTitle );
            }
        }

        //
        //  Set 9600 as default baud selection
        //

        nIndex = (short) SendDlgItemMessage( hDlg,
                                             PORT_BAUDRATE,
                                             CB_FINDSTRING,
                                             (WPARAM) -1,
                                             (LPARAM) m_sz9600 );

        nIndex = (nIndex == CB_ERR) ? 0 : nIndex;

        SendDlgItemMessage( hDlg, PORT_BAUDRATE, CB_SETCURSEL, nIndex, 0L );

        for( nIndex = 0; m_nDataBits[ nIndex ]; nIndex++ )
        {
            MyItoa( m_nDataBits[ nIndex ], szTitle, 10 );

            SendDlgItemMessage( hDlg, PORT_DATABITS, CB_ADDSTRING, 0,
                                (LPARAM) szTitle );
        }

        SendDlgItemMessage( hDlg, PORT_DATABITS, CB_SETCURSEL, DEF_WORD, 0L );

        SetCBFromRes( GetDlgItem( hDlg, PORT_PARITY), IDS_MYPORT1, DEF_PARITY );
        SetCBFromRes( GetDlgItem( hDlg, PORT_STOPBITS), IDS_MYPORT2, DEF_STOP );
        SetCBFromRes( GetDlgItem( hDlg, PORT_FLOWCTL), IDS_MYPORT3, DEF_SHAKE );

        LoadString( g_hInst, IDS_MYPORT5, szTitleFormat,
                    CharSizeOf( szTitleFormat ) );

        wsprintf( szTitle, szTitleFormat, m_szComPort );

        SetWindowText( hDlg, szTitle );

        SetFromWin( hDlg );

        //
        // center and make visible
        //

        ShowWindow( hDlg, SHOW_OPENWINDOW );

        return( TRUE );
        break;


    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {
        case PORT_ADVANCED:
        {
            INT DialogReturnCode;
            HDEVINFO DeviceInfoSet;
            SP_DEVINFO_DATA DeviceInfoData;

            //
            // For PnP ISA devices (either serial cards or modems), we have to use
            // our Win95-style configuration dialog, in order to be able to tie together
            // the legacy registry information with the Plug&Play information.
            //
            DeviceInfoSet = OpenPnPSerialDeviceInstance(hDlg,
                                                        m_szSerialKey,
                                                        &DeviceInfoData
                                                       );

            if(DeviceInfoSet == INVALID_HANDLE_VALUE) {
                DialogReturnCode = 0;
            } else {
                DialogReturnCode = DoPnPAdvancedPortDialog(DeviceInfoSet,
                                                           &DeviceInfoData,
                                                           hDlg,
                                                           FALSE,
                                                           NULL
                                                          );

                SetupDiDestroyDeviceInfoList(DeviceInfoSet);
            }

            if( DialogReturnCode == 1 )
            {
                SetDlgItemText( hDlg, IDCANCEL, g_szClose );

                hParent = GetParent( hDlg );

                SetDlgItemText( hParent, IDOK, g_szClose );

                //
                //  COM Port Number changed, change our title, tell parent also
                //

                if( bResetTitle )
                {
                    LoadString( g_hInst, IDS_MYPORT5, szTitleFormat,
                                CharSizeOf( szTitleFormat ) );

                    wsprintf( szTitle, szTitleFormat, m_szComPort );

                    SetWindowText( hDlg, szTitle );

                    bResetTitle = FALSE;

                    //
                    //  Tell parent dlg to reset when we get there
                    //

                    bResetLB = TRUE;
                }
            }

            break;
        }

        case IDOK:
            if( ChkCommSettings( hDlg ) )
            {
                //
                //  change cursor to hourglass
                //

                HourGlass( TRUE );

                //
                //  store changes to win.ini; broadcast changes to apps
                //

                CommPortsToWin( hDlg );

                //
                //  change cursor back to arrow
                //

                HourGlass( FALSE );
            }
            else
            {
                SetFocus( GetDlgItem( hDlg, PORT_BAUDRATE ) );
                SendDlgItemMessage( hDlg, PORT_BAUDRATE, EM_SETSEL, 0, 32767 );
                break;
            }
            /* fall through */

        case IDCANCEL:
            EndDialog( hDlg, LOWORD( wParam ) );
            break;

        default:
            break;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}


DWORD
WINAPI
PortsClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )
/*++

Routine Description:

    This routine acts as the class installer for Ports devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/
{
    SP_MOVEDEV_PARAMS MoveDevParams;
    HKEY hDeviceKey;
    TCHAR PortName[20];
    DWORD PortNameSize, Err;

    switch(InstallFunction) {

        case DIF_INSTALLDEVICE :

            return InstallPnPSerialPort(DeviceInfoSet, DeviceInfoData);

        case DIF_MOVEDEVICE :
            //
            // In addition to doing the default action of calling
            // SetupDiMoveDuplicateDevice, we need to retrieve the COM port
            // number of the old device, and set it to be the COM port number
            // of the new device (if the move is successful).  In Win95, setupx
            // had a hack inside of DiMoveDuplicateDevNode to do that, but it's
            // really the class installer's job to do class-specific stuff like
            // this.
            //
            MoveDevParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            if(!SetupDiGetClassInstallParams(DeviceInfoSet,
                                             DeviceInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&MoveDevParams,
                                             sizeof(MoveDevParams),
                                             NULL)) {
                return GetLastError();
            }

            //
            // Open the device key for the source device instance, and retrieve its
            // "PortName" value.
            //
            if((hDeviceKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                                  &MoveDevParams.SourceDeviceInfoData,
                                                  DICS_FLAG_GLOBAL,
                                                  0,
                                                  DIREG_DEV,
                                                  KEY_READ)) == INVALID_HANDLE_VALUE) {
                return GetLastError();
            }

            PortNameSize = sizeof(PortName);
            Err = RegQueryValueEx(hDeviceKey,
                                  m_szPortName,
                                  NULL,
                                  NULL,
                                  (PBYTE)PortName,
                                  &PortNameSize
                                 );

            RegCloseKey(hDeviceKey);

            if(Err != ERROR_SUCCESS) {
                return Err;
            }

            //
            // Now call the default routine for moving devices.
            //
            if(!SetupDiMoveDuplicateDevice(DeviceInfoSet, DeviceInfoData)) {
                return GetLastError();
            }

            //
            // The device was successfully moved.  Now open the destination device's key,
            // and store the COM port name there.  (Ignore failure here.)
            //
            if((hDeviceKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                                  &MoveDevParams.SourceDeviceInfoData,
                                                  DICS_FLAG_GLOBAL,
                                                  0,
                                                  DIREG_DEV,
                                                  KEY_READ)) != INVALID_HANDLE_VALUE) {

                RegSetValueEx(hDeviceKey,
                              m_szPortName,
                              0,
                              REG_SZ,
                              (PBYTE)PortName,
                              ByteCountOf(lstrlen(PortName) + 1)
                             );

                RegCloseKey(hDeviceKey);
            }

            return NO_ERROR;

        default :
            //
            // Just do the default action.
            //
            return ERROR_DI_DO_DEFAULT;
    }
}


HDEVINFO
OpenPnPSerialDeviceInstance(
    IN  HWND             hwndParent,
    IN  LPCTSTR          SerialKeyName,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine creates a device information set, and opens the specified serial
    device information element within that set.

Arguments:

    hwndParent - supplies the handle of the window to be used as the parent for
        any UI to be performed for the device information element.

    SerialKeyName - supplies the name of the subkey under the serial driver's
        Parameters subkey that contains information about this serial device

    DeviceInfoData - supplies the address of a SP_DEVINFO_DATA structure that is
        filled in upon return with the newly-opened device information element.

Return Value:

    If successful, the return value is a handle to the newly-created device
    information set.  If failure, the return value is INVALID_HANDLE_VALUE.

--*/
{
    HKEY hPortKey;
    TCHAR CharBuffer[MAX_PATH];
    DWORD CharBufferSize;
    HDEVINFO DeviceInfoSet;
    DWORD Err;

    //
    // CharBuffer is used to hold both a registry path and a device instance ID.
    // Make sure our assumption about which string is larger remains correct.
    //
#if MAX_DEVICE_ID_LEN > MAX_PATH
#error MAX_DEVICE_ID_LEN is greater than MAX_PATH.  Update CharBuffer.
#endif

    //
    // Retrieve the PnP ISA device instance name associated with this port.
    //
    wsprintf(CharBuffer, m_szRegSerialIO, SerialKeyName);
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    CharBuffer,
                    0,
                    KEY_ALL_ACCESS,
                    &hPortKey) != ERROR_SUCCESS) {

        goto clean0;
    }

    CharBufferSize = sizeof(CharBuffer);
    Err = RegQueryValueEx(hPortKey,
                          m_szPnPDeviceId,
                          NULL,
                          NULL,
                          (PBYTE)CharBuffer,
                          &CharBufferSize
                         );

    RegCloseKey(hPortKey);

    if(Err != ERROR_SUCCESS) {
        goto clean0;
    }

    //
    // We've retrieved the device instance ID, now open this as a device
    // information element.
    //
    if((DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, hwndParent)) == INVALID_HANDLE_VALUE) {
        goto clean0;
    }

    DeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
    if(SetupDiOpenDeviceInfo(DeviceInfoSet,
                             CharBuffer,
                             hwndParent,
                             0,
                             DeviceInfoData)) {

        return DeviceInfoSet;
    }

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

clean0:
    return INVALID_HANDLE_VALUE;
}


INT
DoPnPAdvancedPortDialog(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  HWND             hwndParent,
    IN  BOOL             FirstTime,
    OUT PHKEY            phPortKey       OPTIONAL
    )
/*++

Routine Description:

    This routine displays the advanced configuration property sheet for a
    PnP ISA serial port device.  This contains a property page for resource
    selection, and another page for miscellaneous settings (e.g., port number,
    FIFO enable/disable).

    NOTE:  This routine uses the global variables m_szComPort (read-write) and
    m_szSerialKey (read-only).  It also set the global variable bResetTitle to
    TRUE if the user changed the COM port number.

Arguments:

    DeviceInfoSet - supplies the handle of the device information set containing
        the serial device to display a dialog for.

    DeviceInfoData - supplies the device information element to be configured.

    hwndParent - supplies the handle of the window to be used as the parent for
        the configuration UI.

    FirstTime - If TRUE, then property sheet comes up with resource selection UI
        having focus instead of General.

    phPortKey - Optionally, supplies the address of a variable that will be filled
        in with a handle to the serial key (under services\serial\parameters) upon
        succussful return from this routine.  The caller must close this handle with
        RegCloseKey().  If the routine fails, the variable will be set to
        INVALID_HANDLE_VALUE.

Return Value:

    If success (i.e., if the user changed something), the return value is 1.
    If failure, the return value is 0, and GetLastError() returns the cause.

--*/
{
    TCHAR CharBuffer[MAX_PATH];
    DWORDLONG IoBase, OldIoBase;
    DWORD dwIoBase;
    ULONG Irq, OldIrq;
    DWORD Err, RegDataType, RegDataSize, OldComPortNumber;
    PROPSHEETPAGE     PropPage;
    PROPSHEETHEADER   PropHeader;
    SP_PROPSHEETPAGE_REQUEST PropPageRequest;
    GENERAL_PROP_PARAMS GeneralPropParams;
    HPROPSHEETPAGE hPages[2];
    HINSTANCE hLib;
    INT i, PropSheetReturn;
    FARPROC PropSheetExtProc;
    HKEY hPortKey, hKey;
    DWORD RegDisposition;
    BOOL GlobalFifoEnable;
    IO_RESOURCE IoResource;
    IRQ_RESOURCE IrqResource;
    LOG_CONF ForcedLogConf;
    BOOL DisplayPropSheet = TRUE;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;

    //
    // Retrieve the current settings for this device, so we'll know whether they changed.
    //
    if(GetSerialPortDevInstConfig((DEVINST)(DeviceInfoData->DevInst),
                                  FORCED_LOG_CONF,
                                  &IoResource,
                                  &IrqResource)) {

        OldIoBase = IoResource.IO_Header.IOD_Alloc_Base;
        OldIrq = IrqResource.IRQ_Header.IRQD_Alloc_Num;

    } else {
        //
        // IRQ of -1 means that we had no old settings
        //
        OldIrq = (ULONG)-1;
    }

    //
    // Open the Services\Serial key.
    //
    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           m_szRegSerial,
                           0,
                           KEY_ALL_ACCESS,
                           &hKey)) != ERROR_SUCCESS) {
        goto clean0;
    }

    //
    // Retrieve the state of the global 'ForceFifoEnable' setting.
    //
    RegDataSize = sizeof(GlobalFifoEnable);
    if((RegQueryValueEx(hKey,
                        m_szFIFO,
                        NULL,
                        &RegDataType,
                        (PBYTE)&GlobalFifoEnable,
                        &RegDataSize) != ERROR_SUCCESS) ||
       (RegDataType != REG_DWORD))
    {
        GlobalFifoEnable = TRUE;   // default to TRUE for PnP ISA cards.
    }

    //
    // Now open/create the Parameters\Serialxxx subkey.
    //
    wsprintf(CharBuffer, m_szParametersSerialIO, m_szSerialKey);
    Err = RegCreateKeyEx(hKey,
                         CharBuffer,
                         0,
                         NULL,
                         0,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hPortKey,
                         &RegDisposition
                        );
    //
    // Don't need the handle to the parameter key anymore.
    //
    RegCloseKey(hKey);

    if(Err != ERROR_SUCCESS) {
        goto clean0;
    }

    //
    // Initialize the property pages list, so we'll know which one we need to free
    // later, if an error occurs.
    //
    ZeroMemory(hPages, sizeof(hPages));

    //
    // Create the first page (General)
    //
    GeneralPropParams.ComPortNumber = OldComPortNumber = myatoi(&m_szComPort[3]);

    GeneralPropParams.FifoChanged = FALSE;
    RegDataSize = sizeof(GeneralPropParams.FifoEnabled);
    if((RegQueryValueEx(hPortKey,
                        m_szFIFO,
                        NULL,
                        &RegDataType,
                        (PBYTE)&(GeneralPropParams.FifoEnabled),
                        &RegDataSize) != ERROR_SUCCESS) ||
       (RegDataType != REG_DWORD))
    {
        //
        // Value isn't stored in the serial port's key, so use the global value.
        //
        GeneralPropParams.FifoEnabled = GlobalFifoEnable;
    }

    GeneralPropParams.DeviceInfoSet = DeviceInfoSet;
    GeneralPropParams.DeviceInfoData = DeviceInfoData;

    //
    // We don't want to popup the configuration UI if the 'quiet install' flag is
    // set _and_ we already have a forced config (pre-install support).
    //
    if(CM_Get_First_Log_Conf(&ForcedLogConf, DeviceInfoData->DevInst, FORCED_LOG_CONF) == CR_SUCCESS) {

        CM_Free_Log_Conf_Handle(ForcedLogConf);

        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if(SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
            if(FirstTime && (DeviceInstallParams.Flags & DI_QUIETINSTALL)) {
                DisplayPropSheet = FALSE;
            }
        }
    }

    if(DisplayPropSheet) {

        PropPage.dwSize        = sizeof(PROPSHEETPAGE);
        PropPage.dwFlags       = PSP_DEFAULT;
        PropPage.hInstance     = g_hInst;
        PropPage.pszTemplate   = MAKEINTRESOURCE(DLG_ADVPORTS_GENERAL);
        PropPage.pszIcon       = NULL;
        PropPage.pszTitle      = NULL;
        PropPage.pfnDlgProc    = GeneralAdvPortsDlg;
        PropPage.lParam        = (LPARAM)&GeneralPropParams;
        PropPage.pfnCallback   = NULL;

        if(!(hPages[0] = CreatePropertySheetPage(&PropPage))) {
            //
            // Failure most likely due to out-of-memory.
            //
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean1;
        }

        //
        // Now get the resource selection page from setupapi.dll
        //
        if(!(hLib = GetModuleHandle(m_szSetupApiDll)) ||
           !(PropSheetExtProc = GetProcAddress(hLib, "ExtensionPropSheetPageProc"))) {

            Err = GetLastError();
            goto clean1;
        }

        PropPageRequest.cbSize = sizeof(SP_PROPSHEETPAGE_REQUEST);
        PropPageRequest.PageRequested  = SPPSR_SELECT_DEVICE_RESOURCES;
        PropPageRequest.DeviceInfoSet  = DeviceInfoSet;
        PropPageRequest.DeviceInfoData = DeviceInfoData;

        if(!PropSheetExtProc(&PropPageRequest, AddPropSheetPageProc, &hPages[1])) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean1;
        }

        //
        // Create the property sheet.
        //
        LoadString(g_hInst,
                   IDS_PROPSHEET_TITLE,
                   CharBuffer,
                   CharSizeOf(CharBuffer)
                  );

        PropHeader.dwSize      = sizeof(PROPSHEETHEADER);
        PropHeader.dwFlags     = PSH_NOAPPLYNOW;
        PropHeader.hwndParent  = hwndParent;
        PropHeader.hInstance   = g_hInst;
        PropHeader.pszIcon     = NULL;
        PropHeader.pszCaption  = CharBuffer;
        PropHeader.nPages      = 2;
        PropHeader.phpage      = hPages;
        PropHeader.nStartPage  = FirstTime ? 1 : 0;
        PropHeader.pfnCallback = NULL;

        if((PropSheetReturn = PropertySheet(&PropHeader)) == -1) {
            Err = ERROR_INVALID_DATA;
            goto clean1;
        }

        //
        // Since PropertySheet() was successful, we need to clear the page
        // handles out of our array, because we don't need to destroy them
        // anymore.
        //
        for(i = 0; i < (sizeof(hPages) / sizeof(hPages[0])); i++) {
            hPages[i] = NULL;
        }

        if(!PropSheetReturn) {
            Err = ERROR_CANCELLED;
            goto clean1;
        }
    }

    //
    // The user selected resources for this device--retrieve those resources.
    //
    if(GetSerialPortDevInstConfig((DEVINST)(DeviceInfoData->DevInst),
                                  FORCED_LOG_CONF,
                                  &IoResource,
                                  &IrqResource)) {

        IoBase = IoResource.IO_Header.IOD_Alloc_Base;
        Irq = IrqResource.IRQ_Header.IRQD_Alloc_Num;

    } else {
        Err = ERROR_INVALID_DATA;   // just use a semi-applicable error code.
        goto clean0;
    }

    //
    // Did any of the settings change?
    //
    if(FirstTime ||
       (Irq != OldIrq) ||
       (IoBase != OldIoBase) ||
       (GeneralPropParams.ComPortNumber != OldComPortNumber) ||
       (GeneralPropParams.FifoChanged))
    {
        //
        // We have all the information we need--write out the settings to the serial
        // key.
        //
        wsprintf(m_szComPort, TEXT("%s%d"), m_szCOM, GeneralPropParams.ComPortNumber);

        if((Err = RegSetValueEx(hPortKey,
                                m_szDosDev,
                                0,
                                REG_SZ,
                                (PBYTE)m_szComPort,
                                ByteCountOf(lstrlen(m_szComPort) + 1))) != ERROR_SUCCESS) {
            goto clean1;
        }

        //
        // Also store the port name in a win95-compatible location (so that the modem class
        // installer can use it, for instance).
        //
        if((hKey = SetupDiCreateDevRegKey(DeviceInfoSet,
                                          DeviceInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DEV,
                                          NULL,
                                          NULL)) == INVALID_HANDLE_VALUE) {
            Err = GetLastError();
            goto clean1;
        }

        Err = RegSetValueEx(hKey,
                            m_szPortName,
                            0,
                            REG_SZ,
                            (PBYTE)m_szComPort,
                            ByteCountOf(lstrlen(m_szComPort) + 1)
                           );

        RegCloseKey(hKey);

        if(Err != ERROR_SUCCESS) {
            goto clean1;
        }

        if(GeneralPropParams.FifoChanged) {

            if((Err = RegSetValueEx(hPortKey,
                                    m_szFIFO,
                                    0,
                                    REG_DWORD,
                                    (PBYTE)&GeneralPropParams.FifoEnabled,
                                    sizeof(GeneralPropParams.FifoEnabled))) != ERROR_SUCCESS) {
                goto clean1;
            }
        }

        //
        // Io base is stored in registry a 32-bit, not 64-bit value.
        //
        dwIoBase = (DWORD)IoBase;
        if((Err = RegSetValueEx(hPortKey,
                                m_szRegPortAddress,
                                0,
                                REG_DWORD,
                                (PBYTE)&dwIoBase,
                                sizeof(dwIoBase))) != ERROR_SUCCESS) {
            goto clean1;
        }

        if((Err = RegSetValueEx(hPortKey,
                                m_szRegPortIRQ,
                                0,
                                REG_DWORD,
                                (PBYTE)&Irq,
                                sizeof(Irq))) != ERROR_SUCCESS) {
            goto clean1;
        }

        //
        // Create/Update DeviceMap entry (don't consider problems here a fatal error).
        //
        if(RegCreateKey(HKEY_LOCAL_MACHINE,
                        m_szRegSerialMap,
                        &hKey) == ERROR_SUCCESS) {

            RegSetValueEx(hKey,
                          m_szSerialKey,
                          0,
                          REG_SZ,
                          (PBYTE)m_szComPort,
                          ByteCountOf(lstrlen(m_szComPort) + 1)
                         );

            RegCloseKey(hKey);
        }

        //
        // Now generate a string, to be used for the device's friendly name, of the form:
        //
        //     Communications Port (COM<x>)
        //
        if(LoadString(g_hInst, IDS_FRIENDLYNAME, CharBuffer, CharSizeOf(CharBuffer))) {
            lstrcat(CharBuffer, m_szComPort);
            lstrcat(CharBuffer, m_szCloseParen);
        } else {
            //
            // Simply use COM port name.
            //
            lstrcpy(CharBuffer, m_szComPort);
        }

        SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_FRIENDLYNAME,
                                         (PBYTE)CharBuffer,
                                         ByteCountOf(lstrlen(CharBuffer) + 1)
                                        );

        if(FirstTime) {
            //
            //  Write default Baud rate string to registry "9600,n,8,1"
            //
            WriteProfileString(m_szPorts, m_szComPort, m_szDefParams);

        } else {
            //
            //  Change titlebar in parent dlg if COM Port Number changed
            //
            if(GeneralPropParams.ComPortNumber != OldComPortNumber) {
                bResetTitle = TRUE;
            }

            //
            //  Issue a restart warning
            //
            DialogBoxParam(g_hInst,
                           (LPTSTR)MAKEINTRESOURCE(DLG_RESTART),
                           hwndParent,
                           (DLGPROC)RestartDlg,
                           MAKELONG(IDS_COMCHANGE, 0)
                          );
        }
    }

    //
    // If we get to here, then we've successfully written out all the necessary registry
    // settings.  Return the still-open handle of the serial device's parameter key, in
    // case the caller needs to do some more.
    //
    if(phPortKey) {
        *phPortKey = hPortKey;
    } else {
        RegCloseKey(hPortKey);
    }
    return 1;

clean1:
    for(i = 0; i < (sizeof(hPages) / sizeof(hPages[0])); i++) {
        if(hPages[i]) {
            DestroyPropertySheetPage(hPages[i]);
        }
    }
    RegCloseKey(hPortKey);

clean0:
    if(phPortKey) {
        *phPortKey = INVALID_HANDLE_VALUE;
    }
    SetLastError(Err);
    return 0;
}


DWORD
InstallPnPSerialPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine performs the installation of a PnP ISA serial port device (may
    actually be a modem card).  This involves the following steps:

        1.  Select a COM port number and serial device name for this port.
        2.  Create a subkey under the serial driver's Parameters key, and
            set it up just as if it was a manually-installed port.
        3.  Display the resource selection dialog, and allow the user to
            configure the settings for the port.
        4.  Write out the settings to the serial port's key in legacy format
            (i.e., the way serial.sys expects to see it).
        5.  Write out PnPDeviceId value to the serial port's key, which gives
            the device instance name with which this port is associated.
        6.  Write out PortName value to the devnode key, so that modem class
            installer can continue with installation (if this is really a
            PnP ISA modem).

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device being installed.

    DeviceInfoData - Supplies the address of the device information element
        being installed.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is a Win32 error code.

--*/
{
    HKEY hKey, hSubKey;
    int PortList[MAX_COM_PORT+1];    // 1 based array of all allowable COM ports
    TCHAR SerialName[40];
    TCHAR ComPort[40];
    DWORD SerialNameSize, ComPortSize, RegDataType;
    int i, ComPortNumber, NewCom, DupSerial, NewSerial, HighestFreeCom;
    TCHAR CharBuffer[MAX_PATH];
    DWORD RegDisposition, Err;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    HWND hwndParent;

    //
    // CharBuffer is used to hold both a registry path and a device instance ID.
    // Make sure our assumption about which string is larger remains correct.
    //
#if MAX_DEVICE_ID_LEN > MAX_PATH
#error MAX_DEVICE_ID_LEN is greater than MAX_PATH.  Update CharBuffer.
#endif

    //
    // Now go and find an unused COM port number and serial port name.  We have to
    // merge two lists of serial port devices in order to make this determination--the
    // device map list and the serial driver's Parameters list.
    //
    FillMemory(PortList, sizeof(PortList), 0xFF);   // use -1 as unused marker, since Serial0 is valid name

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    m_szRegSerialMap,
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS) {

        SerialNameSize = sizeof(SerialName);
        ComPortSize = sizeof(ComPort);
        i = 0;

        while(RegEnumValue(hKey,
                           i++,
                           SerialName,
                           &SerialNameSize,
                           NULL,
                           &RegDataType,
                           (LPBYTE)ComPort,
                           &ComPortSize) == ERROR_SUCCESS) {

            if(RegDataType != REG_SZ) {
                goto TryNextSerialValue;
            }

            //
            //  Get number of COM port (go past "COM" in string)
            //
            ComPortNumber = myatoi(&ComPort[3]);

            if(ComPortNumber <= MAX_COM_PORT) {
                //
                // Store the serial port's number in this location in the ports array.
                // (go past "Serial" in string).
                //
                PortList[ComPortNumber] = myatoi(&SerialName[6]);
            }

TryNextSerialValue:
            SerialNameSize = sizeof(SerialName);
            ComPortSize = sizeof(ComPort);
        }

        RegCloseKey(hKey);
    }

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    m_szRegSerialParam,
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS) {
        //
        //  Enumerate all keys under Serial\Parameters
        //
        i = 0;

        while(RegEnumKey(hKey,
                         i++,
                         SerialName,
                         CharSizeOf(SerialName)) != ERROR_NO_MORE_ITEMS) {

            if(RegOpenKeyEx(hKey,
                            SerialName,
                            0,
                            KEY_READ,
                            &hSubKey) == ERROR_SUCCESS) {
                //
                //  Get DosDevices value for this Serial key
                //
                ComPortSize = sizeof(ComPort);
                if(RegQueryValueEx(hSubKey,
                                   m_szDosDev,
                                   NULL,
                                   &RegDataType,
                                   (LPBYTE)ComPort,
                                   &ComPortSize) == ERROR_SUCCESS) {

                    if(RegDataType != REG_SZ) {
                        goto TryNextSubKey;
                    }

                    //
                    //  Get number of COM port (go past "COM" in string)
                    //
                    ComPortNumber = myatoi(&ComPort[3]);

                    if(ComPortNumber > MAX_COM_PORT) {
                        goto TryNextSubKey;
                    }

                    //
                    // Store the serial port's number in this location in the ports array.
                    // (go past "Serial" in string).
                    //
                    PortList[ComPortNumber] = myatoi(&SerialName[6]);
                }
TryNextSubKey:
                RegCloseKey(hSubKey);
            }
        }
        RegCloseKey(hKey);
    }

    //
    // OK, now we have an array whose non-zero elements have indexes corresponding to COM port
    // numbers that are taken.  These elements specify the number of the corresponding serial
    // name.  Search through this list, and pick a number that is one greater than the existing
    // highest number (above a certain threshold).
    //
    NewCom = MIN_COM;
    NewSerial = MIN_SERIAL;
    HighestFreeCom = 0;

    for(i = 1; i <= MAX_COM_PORT; i++) {
        if(PortList[i] != -1) {
            NewCom = max(i+1, NewCom);
            NewSerial = max(PortList[i]+1, NewSerial);
        } else {
            //
            // Remember the highest com port number that isn't taken, because it could be that
            // 256 was taken, but 128 was free, for example.
            //
            HighestFreeCom = i;
        }
    }

    if(NewCom > MAX_COM_PORT) {
        //
        // Did we find a hole along the way?
        //
        if(HighestFreeCom) {
            NewCom = HighestFreeCom;
        } else {
            //
            // All ports are taken!  Fail the installation.
            //
            return ERROR_OUT_OF_STRUCTURES;
        }
    }

    //
    // Generate the serial and COM port names based on the numbers we picked.
    //
    wsprintf(m_szSerialKey, TEXT("%s%d"), m_szSERIAL, NewSerial);
    wsprintf(m_szComPort, TEXT("%s%d"), m_szCOM, NewCom);

    //
    // Now set the device instance's 'Service' registry property to "Serial".
    //
    // NOTE:  Normally, this would done via the selected driver node's ".Services"
    // install section.  But in the case of PnP ISA modems, we manually transform
    // "Modem"-class devices into "Ports"-class devices without having an INF.
    // Setting this property here ensures that if the INF does not specify a
    // controlling driver (or if there is no INF), then the serial driver will be
    // used.
    //
    if(!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_SERVICE,
                                         (PBYTE)m_szSERIAL,
                                         sizeof(m_szSERIAL))) {
        return GetLastError();
    }

    //
    // Now generate a string, to be used for the device's friendly name, of the form:
    //
    //     Communications Port (COM<x>)
    //
    if(LoadString(g_hInst, IDS_FRIENDLYNAME, CharBuffer, CharSizeOf(CharBuffer))) {
        lstrcat(CharBuffer, m_szComPort);
        lstrcat(CharBuffer, m_szCloseParen);
    } else {
        //
        // Simply use COM port name.
        //
        lstrcpy(CharBuffer, m_szComPort);
    }

    SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_FRIENDLYNAME,
                                     (PBYTE)CharBuffer,
                                     ByteCountOf(lstrlen(CharBuffer) + 1)
                                    );

    //
    // Retrieve the device install parameters, and set the DI_NEEDREBOOT flag.  If this
    // fails, it's no big deal, as the device installer should be able to figure it out
    // on its own.  (Also, retrieve the parent window handle for resource selection UI.)
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
        hwndParent = DeviceInstallParams.hwndParent;
        DeviceInstallParams.Flags |= DI_NEEDREBOOT;
        SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams);
    } else {
        hwndParent = NULL;
    }

    //
    // Now do the installation for this device.
    //
    if(!SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData)) {
        return GetLastError();
    }

    //
    // Finally, popup the advanced configuration dialog for the user to pick the resources
    // for this new port.
    //
    if(!DoPnPAdvancedPortDialog(DeviceInfoSet, DeviceInfoData, hwndParent, TRUE, &hKey)) {
        Err = GetLastError();
        goto RegistryError;
    }

    //
    // Set "PnPDeviceId" entry
    //
    SetupDiGetDeviceInstanceId(DeviceInfoSet,
                               DeviceInfoData,
                               CharBuffer,
                               sizeof(CharBuffer) / sizeof(TCHAR),
                               NULL
                              );

    Err = RegSetValueEx(hKey,
                        m_szPnPDeviceId,
                        0,
                        REG_SZ,
                        (PBYTE)CharBuffer,
                        ByteCountOf(lstrlen(CharBuffer) + 1)
                       );

    //
    // Don't need the handle to the serial device's parameter key anymore.
    //
    RegCloseKey(hKey);

    if(Err == ERROR_SUCCESS) {
        return NO_ERROR;
    }

    //
    // Drop down into error handling.
    //

RegistryError:
    //
    // Clean up the device instance.  This behavior is taken from the clean-up code in
    // SetupDiInstallDevice.
    //
    // Disable the device if the error wasn't a user cancel.
    //
    if(Err != ERROR_CANCELLED) {

        DWORD ConfigFlags;
        SP_DRVINFO_DATA DriverInfoData;

        //
        // The device is in an unknown state.  Disable it by setting the
        // CONFIGFLAG_DISABLED config flag.
        //
        if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_CONFIGFLAGS,
                                             NULL,
                                             (PBYTE)&ConfigFlags,
                                             sizeof(ConfigFlags),
                                             NULL))
        {
            //
            // Couldn't retrieve ConfigFlags--default to zero.
            //
            ConfigFlags = 0;
        }

        ConfigFlags |= (CONFIGFLAG_DISABLED | CONFIGFLAG_REINSTALL);

        SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_CONFIGFLAGS,
                                         (PBYTE)&ConfigFlags,
                                         sizeof(ConfigFlags)
                                        );

        //
        // Delete the Driver= entry from the Dev Reg Key and delete the
        // DrvRegKey
        //
        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if(SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData)) {

            SetupDiDeleteDevRegKey(DeviceInfoSet,
                                   DeviceInfoData,
                                   DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGGENERAL,
                                   0,
                                   DIREG_DRV
                                  );

            SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_DRIVER,
                                             NULL,
                                             0
                                            );
        }
    }

    //
    // Clean up parameters key.
    //
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    m_szRegSerialParam,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey) == ERROR_SUCCESS) {

        RegDeleteKey(hKey, m_szSerialKey);
        RegCloseKey(hKey);
    }

    return Err;
}


BOOL
GetSerialPortDevInstConfig(
    IN  DEVINST       DevInst,
    IN  ULONG         LogConfigType,
    OUT PIO_RESOURCE  IoResource,
    OUT PIRQ_RESOURCE IrqResource
    )
/*++

Routine Description:

    This routine retrieves the base IO port and IRQ for the specified device instance
    in a particular logconfig.

Arguments:

    DevInst - Supplies the handle of a device instance to retrieve configuration for.

    LogConfigType - Specifies the type of logconfig to retrieve.  Must be either
        ALLOC_LOG_CONF, BOOT_LOG_CONF, or FORCED_LOG_CONF.

    IoResource - Supplies the address of an Io resource structure that receives the
        Io resource retreived.

    IrqResource - Supplies the address of an IRQ resource variable that receives the
        IRQ resource retrieved.

Return Value:

    If success, the return value is TRUE, otherwise it is FALSE.

--*/
{
    LOG_CONF LogConfig;
    RES_DES ResDes;
    CONFIGRET cr;
    BOOL Success;

    if(CM_Get_First_Log_Conf(&LogConfig, DevInst, LogConfigType) != CR_SUCCESS) {
        return FALSE;
    }

    Success = FALSE;    // assume failure.

    //
    // First, get the Io base port
    //
    if(CM_Get_Next_Res_Des(&ResDes, LogConfig, ResType_IO, NULL, 0) != CR_SUCCESS) {
        goto clean0;
    }

    cr = CM_Get_Res_Des_Data(ResDes, IoResource, sizeof(IO_RESOURCE), 0);

    CM_Free_Res_Des_Handle(ResDes);

    if(cr != CR_SUCCESS) {
        goto clean0;
    }

    //
    // Now, get the IRQ
    //
    if(CM_Get_Next_Res_Des(&ResDes, LogConfig, ResType_IRQ, NULL, 0) != CR_SUCCESS) {
        goto clean0;
    }

    if(CM_Get_Res_Des_Data(ResDes, IrqResource, sizeof(IRQ_RESOURCE), 0) == CR_SUCCESS) {
        Success = TRUE;
    }

    CM_Free_Res_Des_Handle(ResDes);

clean0:
    CM_Free_Log_Conf_Handle(LogConfig);

    return Success;
}


VOID
GetComPortForSerialDevice(
    IN     INT  SerialDeviceId,
    IN OUT PINT ComPortNumber
    )
/*++

Routine Description:

    This routine retrieves the COM port number associated with the specified
    serial device ID.

Arguments:

    SerialDeviceId - Supplies the ID of the serial port device whose COM port
        number is to be retrieved.

    ComPortNumber - Supplies the address of a variable that receives the COM
        port number for the specified serial port device.  If no port number
        is found, this value is not modified.

Return Value:

    None.

--*/
{
    TCHAR SerialDeviceName[40];
    TCHAR SerialParamPath[MAX_PATH];
    TCHAR ComPortName[40];
    HKEY hKey;
    DWORD ComPortNameSize;
    BOOL Success = FALSE;

    wsprintf(SerialDeviceName, TEXT("%s%d"), m_szSERIAL, SerialDeviceId);

    //
    // First, look in HKLM\HARDWARE\DEVICEMAP\SERIALCOMM for a mapping.
    //
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    m_szRegSerialMap,
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS) {

        ComPortNameSize = sizeof(ComPortName);
        if(RegQueryValueEx(hKey,
                           SerialDeviceName,
                           NULL,
                           NULL,
                           (PBYTE)ComPortName,
                           &ComPortNameSize) == ERROR_SUCCESS) {

            *ComPortNumber = myatoi(&ComPortName[3]);
            Success = TRUE;
        }
        RegCloseKey(hKey);
        if(Success) {
            return;
        }
    }

    //
    // Well, that didn't work, try looking in the serial driver's parameter subkey for
    // this device.
    //
    wsprintf(SerialParamPath, m_szRegSerialIO, SerialDeviceName);

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    SerialParamPath,
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS) {

        ComPortNameSize = sizeof(ComPortName);
        if(RegQueryValueEx(hKey,
                           m_szDosDev,
                           NULL,
                           NULL,
                           (PBYTE)ComPortName,
                           &ComPortNameSize) == ERROR_SUCCESS) {

            *ComPortNumber = myatoi(&ComPortName[3]);
        }
        RegCloseKey(hKey);
    }
}


BOOL
CALLBACK
AddPropSheetPageProc(
    IN HPROPSHEETPAGE hpage,
    IN LPARAM lParam
   )
{
    *((HPROPSHEETPAGE *)lParam) = hpage;
    return TRUE;
}


BOOL
APIENTRY
GeneralAdvPortsDlg(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    PGENERAL_PROP_PARAMS GeneralPropParams = (PGENERAL_PROP_PARAMS)GetWindowLong(hDlg, DWL_USER);
    UINT i;
    TCHAR CharBuffer[LINE_LEN];

    switch(uMessage) {

        case WM_INITDIALOG :
            //
            // on WM_INITDIALOG call, lParam points to the property sheet page.
            // The lParam field in the property sheet page struct is set by,
            // caller. When I created the property sheet, I passed in a pointer
            // to a HWPROF_INFO struct. Save this in the user window long so I
            // can access it on later messages.
            //
            GeneralPropParams = (PGENERAL_PROP_PARAMS)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLong(hDlg, DWL_USER, (LONG)GeneralPropParams);

            //
            // Fill in COM port number drop-down listbox, and pre-select current number.
            //
            for(i = MIN_COM; i <= MAX_COM_PORT; i++ )
            {
                wsprintf(CharBuffer, TEXT("%d"), i);
                SendDlgItemMessage(hDlg,
                                   PORT_NUMBER,
                                   CB_ADDSTRING,
                                   0,
                                   (LPARAM)CharBuffer
                                  );
            }

            SendDlgItemMessage(hDlg,
                               PORT_NUMBER,
                               CB_SETCURSEL,
                               GeneralPropParams->ComPortNumber - MIN_COM,
                               0
                              );

            //
            // Initialize the "FIFO Enabled" checkbox.
            //
            CheckDlgButton(hDlg, PORT_FIFO, GeneralPropParams->FifoEnabled);

            //
            // Retrieve the description of the device (try to use friendly name first,
            // and fall back to stock
            //
            if(!SetupDiGetDeviceRegistryProperty(GeneralPropParams->DeviceInfoSet,
                                                 GeneralPropParams->DeviceInfoData,
                                                 SPDRP_FRIENDLYNAME,
                                                 NULL,
                                                 (PBYTE)CharBuffer,
                                                 sizeof(CharBuffer),
                                                 NULL))
            {
                if(!SetupDiGetDeviceRegistryProperty(GeneralPropParams->DeviceInfoSet,
                                                     GeneralPropParams->DeviceInfoData,
                                                     SPDRP_DEVICEDESC,
                                                     NULL,
                                                     (PBYTE)CharBuffer,
                                                     sizeof(CharBuffer),
                                                     NULL))
                {
                    //
                    // This should never happen!  Fall back to "COM<x>".
                    //
                    wsprintf(CharBuffer, TEXT( "%s%d" ), m_szCOM, GeneralPropParams->ComPortNumber);

                }
            }

            SetDlgItemText(hDlg, IDC_DEVDESC, CharBuffer);

            //
            // No need for us to set the focus.
            //
            return TRUE;

        case WM_COMMAND :

            switch(LOWORD(wParam)) {

                case PORT_FIFO :
                    GeneralPropParams->FifoChanged = TRUE;
                    return TRUE;

                default :
                    return FALSE;
            }

        case WM_NOTIFY :

            switch (((NMHDR *)lParam)->code) {

                case PSN_APPLY :
                    //
                    // Retrieve the new values of COM port number and FIFO enabled flag.
                    //
                    GeneralPropParams->FifoEnabled = IsDlgButtonChecked(hDlg, PORT_FIFO);
                    if(GetDlgItemText(hDlg, PORT_NUMBER, CharBuffer, CharSizeOf(CharBuffer))) {
                        GeneralPropParams->ComPortNumber = myatoi(CharBuffer);
                    }

                    SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;

                default :
                    return FALSE;
            }

        default :
            return FALSE;
    }
}

