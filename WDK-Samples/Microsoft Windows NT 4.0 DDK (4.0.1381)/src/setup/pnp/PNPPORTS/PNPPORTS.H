/** FILE: pnpports.h ********* Module Header ********************************
 *
 *  Control Panel System applet common definitions, resource ids, typedefs,
 *  external declarations and library routine function prototypes.
 *
 *
 *  Copyright (C) 1990-1996 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                            Include Files
//==========================================================================
#include <windows.h>
#include <tchar.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <regstr.h>

//==========================================================================
//                        Definitions
//==========================================================================

//
//  General definitions
//

#define PATHMAX             MAX_PATH

//
//  String Resource IDs
//


#define INITS                0
#define ERRORS               8

#define IDS_DEFAULT         10

#define IDS_MYPORT1         11
#define IDS_MYPORT2         12
#define IDS_MYPORT3         13
#define IDS_MYPORT4         14
#define IDS_MYPORT5         15
#define IDS_MYPORT6         16
#define IDS_MYPORT7         17

#define IDS_FRIENDLYNAME    18
#define IDS_PROPSHEET_TITLE 19

#define IDS_PORTS           INITS+3
#define IDS_PORTSINFO       INITS+4

#define IDS_SYSSETCHANGE    50
#define IDS_COMCHANGE       51
#define IDS_RESTART         52

//
//  Icon ids
//

#define PORTS_ICON          2

//
//  Dialog ids
//

#define DLG_PORTS             4
#define DLG_PORTS2           19
#define DLG_PORTS3           33
#define DLG_RESTART          13
#define DLG_ADVPORTS_GENERAL 14

//
//  For useless control ids
//

#define FOO             -1


//
//  NT Ports Applet Dialogs
//


#define PORT_BAUDRATE   800
#define PORT_DATABITS   801
#define PORT_PARITY     802
#define PORT_STOPBITS   803
#define PORT_FLOWCTL    804
#define PORT_ADVANCED   805
#define PORT_BASEIO     806
#define PORT_IRQ        807
#define PORT_SPINNER    808

#define PORT_LB         810
#define PORT_FIFO       812
#define PORT_NUMBER     813
#define SERIAL_DBASE    815
#define PORT_DELETE     816

#define PORT_SETTING    828
#define PORT_TITLE      829

#define IDC_PORTS       838
#define IDC_DEVDESC     839

//
//  Restart Dialog ids
//

#define RESTART_TEXT    100


//==========================================================================
//                              Macros
//==========================================================================

#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))
#define ByteCountOf(x)  ((x) * sizeof(TCHAR))


//==========================================================================
//                         External Declarations
//==========================================================================
//
//  DATA

//
//  exported from cpl.c
//

extern HANDLE g_hInst;
extern TCHAR  g_szSysDir[ ];        //  GetSystemDirectory
extern TCHAR  g_szWinDir[ ];        //  GetWindowsDirectory
extern TCHAR  g_szClose[ ];         //  "Close" string
extern TCHAR  g_szSharedDir[ ];     //  Shared dir found by Version apis
extern TCHAR  g_szErrMem[ ];        //  Low memory message
extern TCHAR  g_szPortsApplet[ ];   //  "Ports Control Panel Applet" title
extern TCHAR  g_szNull[];           //  Null string


//==========================================================================
//                            Function Prototypes
//==========================================================================

//
//  ports.c
//
extern BOOL APIENTRY ShortCommDlg (HWND, UINT, DWORD, LONG);


//
//  util.c
//
extern LPTSTR BackslashTerm( LPTSTR pszPath );
extern int    MyMessageBox( HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ... );
extern void   StripBlanks( LPTSTR pszString );
extern LPTSTR strscan( LPTSTR pszString, LPTSTR pszTarget );
extern int    myatoi( LPTSTR pszInt );
extern int    DoDialogBoxParam( int nDlg, HWND hParent, DLGPROC lpProc, DWORD dwParam );
extern LPTSTR MyItoa( INT value, LPTSTR  string, INT  radix );
extern void   SendWinIniChange( LPTSTR szSection );
extern void   ErrMemDlg( HWND hParent );
extern void   HourGlass( BOOL bOn);
extern BOOL   RestartDlg( HWND hDlg, UINT message, DWORD wParam, LONG lParam );

