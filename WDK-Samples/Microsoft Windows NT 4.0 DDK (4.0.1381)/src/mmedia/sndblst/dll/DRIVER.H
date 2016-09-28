/****************************************************************************
 *
 *   driver.h
 *
 *   Copyright (c) 1994 Microsoft Corporation.  All rights reserved.
 *
 ***************************************************************************/


#define STR_DRIVERNAME TEXT("sndblst")

//
// General config stuff
//

#include <soundcfg.h>
#include <sndblst.h>                // Shared with kernel driver

//
// Patch stuff (support synthlib)
//

#define DATA_FMPATCHES          1234

#ifndef RC_INVOKED
#define RT_BINARY               MAKEINTRESOURCE( 256 )
#else
#define RT_BINARY               256
#endif


//
// Config ID's
//

#include    "config.h"

//
// Driver Version
//
#define DRIVER_VERSION          0x0100      // 1.00

//
// New return code
//

#define DRVCNF_CONTINUE  (-1)

/****************************************************************************

       globals

 ***************************************************************************/

HANDLE     ghModule;            /* our module handle                       */
REG_ACCESS RegAccess;           /* Handles to registry and services        */
BOOL       bInstall;            /* Tell config we're on an install         */
HINSTANCE  hInstSetupAPI;
HINSTANCE  hInstCfgMgr;

/***************************************************************************

    prototypes

***************************************************************************/

/* config.c */
extern int      Config(HWND hWnd, HANDLE hInstance);
extern LRESULT  ConfigRemove(HWND hDlg);
extern int      ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern void cdecl  ConfigErrorMsgBox(HWND hDlg, UINT StringId, ...);
extern DWORD    InstallPnPDevice(IN HDEVINFO DeviceInfoSet,
                                 IN PSP_DEVINFO_DATA DeviceInfoData);

/* drvproc.c */
extern LRESULT  DriverProc(DWORD dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2);



/****************************************************************************

       global function pointers to Dynalink'ed functions

 ***************************************************************************/

FARPROC DLSetupDiRemoveDevice;
FARPROC DLSetupDiDestroyDeviceInfoList;
FARPROC DLSetupDiGetDeviceInstallParams;
FARPROC DLSetupDiSetDeviceInstallParams;
FARPROC DLSetupDiGetDeviceInstanceId;
FARPROC DLSetupDiGetDeviceRegistryProperty;
FARPROC DLSetupDiSetDeviceRegistryProperty;
FARPROC DLSetupDiGetSelectedDriver;
FARPROC DLSetupDiDeleteDevRegKey;
FARPROC DLSetupDiCreateDeviceInfoList;
FARPROC DLSetupDiOpenDeviceInfo;
FARPROC DLCM_Get_First_Log_Conf;
FARPROC DLCM_Free_Log_Conf_Handle;
FARPROC DLCM_Get_Next_Res_Des;
FARPROC DLCM_Get_Res_Des_Data;
FARPROC DLCM_Free_Res_Des_Handle;
