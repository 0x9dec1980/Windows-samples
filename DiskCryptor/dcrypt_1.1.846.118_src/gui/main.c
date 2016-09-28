/*  *
    * DiskCryptor - open source partition encryption tool
	* Copyright (c) 2007-2010 
	* ntldr <ntldr@diskcryptor.net> PGP key ID - 0xC48251EB4F8E4E6E
    *

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <windows.h>
#include <shlwapi.h>

#include "main.h"
#include "autorun.h"

#include "rand.h"
#include "prc_keyfiles.h"

#pragma warning(disable : 4995)

int _tmr_elapse[ ] = 
{ 
	1000,    // MAIN_TIMER
	100,     // PROC_TIMER
	3000,    // RAND_TIMER
	500,     // SHRN_TIMER
	500      // POST_TIMER
};

int __status;

dc_conf_data __config;

list_entry __volumes;
list_entry __action;
list_entry __drives;

CRITICAL_SECTION crit_sect;

ATOM dlg_class;

void _set_timer(
		int  index,
		BOOL set,
		BOOL refresh
	)
{
	if ( refresh ) 
	{
		_refresh(TRUE);
	}
	if ( set ) 
	{
		SetTimer(
			__dlg, IDC_TIMER + index, _tmr_elapse[index], (TIMERPROC)_timer_handle
			);
	} else
	{
		KillTimer( __dlg, IDC_TIMER + index );
	}
}


void _refresh(
		char main
	)
{
	_timer_handle(
		__dlg, WM_TIMER, IDC_TIMER + (main ? MAIN_TIMER : PROC_TIMER), IDC_TIMER
		);
}


static DWORD _dc_upd_bootloader( )
{
	ldr_config conf;
	DWORD      status;
	
	if ( dc_get_mbr_config( -1, NULL, &conf ) != ST_OK ) return NO_ERROR;
	if ( (status =  dc_update_boot(-1)) != ST_OK ) return 100000 + status;
	return NO_ERROR;
}


DWORD _drv_action(int action, int version)
{
	static wchar_t restart_confirm[ ] = 
						L"You must restart your computer before the new settings will take effect.\n\n"
						L"Do you want to restart your computer now?";
	DWORD status = ERROR_INVALID_FUNCTION;

	switch (action) 
	{
		case DA_INSTAL: 
		{
			if ( dc_is_driver_installed() ) 
			{
				if ( __msg_q( HWND_DESKTOP, restart_confirm ) )
				{
					_reboot( );
				}
				status = NO_ERROR;
			} else
			{
				if ( __msg_q( HWND_DESKTOP, L"Install DiskCryptor driver?" ) )
				{
					if ( (status = dc_install_driver()) == NO_ERROR )
					{
						if ( __msg_q( HWND_DESKTOP, restart_confirm ) )
						{
							_reboot( );					
						}						
					}
				} else {
					status = NO_ERROR;
				}
			}
		}
		break;
		case DA_REMOVE: 
		{
			if ( dc_is_driver_installed() ) 
			{
				if ( (status = dc_remove_driver()) == NO_ERROR )
				{
					if ( __msg_q( HWND_DESKTOP, restart_confirm ) )
					{
						_reboot( );
					}
				}
			}
		}
		break;
		case DA_UPDATE: 
		{
			wchar_t up_atom[MAX_PATH];

			_snwprintf(up_atom, countof(up_atom), L"DC_UPD_%d", version);

			if (GlobalFindAtom(up_atom) != 0)
			{
				if ( __msg_q( HWND_DESKTOP, restart_confirm ) ) _reboot( );
				status = NO_ERROR;
				break;
			}

			if ( dc_is_driver_installed() == FALSE )
			{
				status = ERROR_PRODUCT_UNINSTALLED;
				break;
			}

			if ( __msg_q( HWND_DESKTOP, L"Update DiskCryptor?" ) )
			{
				if ( (status = dc_update_driver()) == NO_ERROR && (status = _dc_upd_bootloader()) == NO_ERROR )
				{
					if ( __msg_q( HWND_DESKTOP, restart_confirm ) ) _reboot();
				}
			}
		}
		break;
	}
	return status;

}


static
LRESULT CALLBACK
_class_dlg_proc(
		HWND hwnd, 
		UINT message,
		WPARAM wparam, 
		LPARAM lparam
	)
{
	return 
		DefDlgProc(
			hwnd, message, wparam, lparam
		);
}


int WINAPI wWinMain(
		HINSTANCE hinst,
		HINSTANCE hprev,
		LPWSTR    cmd_line,
		int       cmd_show
	)
{
	int rlt;
	int ver;
	int app_start = on_app_start( cmd_line );

#ifdef LOG_FILE
	_log( L"%0.8X app start", app_start );
#endif

	if ( app_start == ST_NEED_EXIT )
	{
		return 0;
	}
	if ( _ui_init(hinst) == 0 )
	{
		__error_s( HWND_DESKTOP, L"Error GUI initialization", ST_OK );
		return 0;
	}
	if ( is_admin( ) != ST_OK )
	{
		__error_s( HWND_DESKTOP, L"Admin Privileges Required", ST_OK );
		return 0;
	}
#ifdef _M_IX86 
	if ( is_wow64( ) != 0 )
	{
		__error_s( HWND_DESKTOP, L"Please use x64 version of DiskCryptor", ST_OK );
		return 0;
	}
#endif
	if ( dc_is_old_runned( ) != 0 )
	{
		__error_s(
			HWND_DESKTOP, 
			L"DiskCryptor 0.1-0.4 installed, please completely uninstall it before use this version.", ST_OK
			);

		return 0;
	}
#ifdef LOG_FILE
	_log( L"%0.8X driver status", dc_driver_status( ) );
#endif
	if ( dc_is_driver_works( ) == FALSE )
	{
		if ( ( rlt = _drv_action(DA_INSTAL, 0) ) != NO_ERROR )
		{
			__error_s( HWND_DESKTOP, NULL, rlt );
		}
		return 0;
	}
	if ( ( rlt = dc_open_device( ) ) != ST_OK )
	{
		__error_s( HWND_DESKTOP, L"Can not open DC device", rlt );
		return 0; 
	}
	
	ver = dc_get_version( );

#ifdef LOG_FILE
	_log( L"%0.8X dc version", ver );
#endif

	if ( ver < DC_DRIVER_VER )
	{
		if ( ( rlt = _drv_action(DA_UPDATE, ver) ) != NO_ERROR )
		{
			__error_s( HWND_DESKTOP, NULL, rlt );
		}
		return 0;
	}

	if ( ver > DC_DRIVER_VER )
	{
		__msg_i(
			HWND_DESKTOP,
			L"DiskCryptor driver v%d detected\n"
			L"Please use last program version", ver
			);

		return 0;
	}
	{
		HWND h_find;
		WNDCLASS wc = { 0 };

		wc.lpszClassName = DC_CLASS;
		wc.lpfnWndProc   = &_class_dlg_proc;
		wc.cbWndExtra    = DLGWINDOWEXTRA;
		wc.hIcon         = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON_TRAY));

		dlg_class = RegisterClass(&wc);

#ifdef LOG_FILE
	_log( L"%0.8X register class", dlg_class );
#endif

		h_find = FindWindow(DC_CLASS, NULL);

#ifdef LOG_FILE
	_log( L"%0.8X find window", h_find );
#endif

		if ( h_find != NULL )
		{
			ShowWindow( h_find, SW_SHOW );
			SetForegroundWindow( h_find );

#ifdef LOG_FILE
	_log( L"show window [ %0.8X ] return", h_find );
#endif
			return 0;
		}
	}
	if ( ( rlt = rnd_init( ) ) != ST_OK )
	{
		__error_s( HWND_DESKTOP, L"Can not initialize RNG", rlt );
		return 0;
	}

	if ( (rlt = dc_load_config(&__config) == NO_ERROR ? ST_OK : ST_ERROR) != ST_OK )
	{
		__error_s( HWND_DESKTOP, L"Error get config", rlt );
		return 0;		
	}
	InitializeCriticalSection( &crit_sect );

#ifdef LOG_FILE
	_log( L"initialize critical section" );
#endif

	_init_list_head( &__drives );
	_init_list_head( &__action );

	_init_keyfiles_list( );

#ifdef LOG_FILE
	_log( L"init keyfiles list" );
#endif
	{
		HWND   h_dialog;
		MSG    msg;

		__hacc = LoadAccelerators( hinst, MAKEINTRESOURCE(IDR_MAIN_ACCEL) );

#ifdef LOG_FILE
		_log( L"before create dialog" );
#endif

		h_dialog = CreateDialog( GetModuleHandleA(NULL), MAKEINTRESOURCE(IDD_MAIN_DLG), HWND_DESKTOP, _main_dialog_proc );

#ifdef LOG_FILE
		_log( L"%0.8X create dialog", h_dialog );
#endif

		rlt = ShowWindow( h_dialog, app_start == ST_AUTORUNNED ? SW_HIDE : SW_SHOW );

#ifdef LOG_FILE
		_log( L"%0.8X show window", rlt );
#endif

		while ( GetMessage( &msg, NULL, 0, 0 ) )
		{
			if ( !TranslateAccelerator( h_dialog, __hacc, &msg ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		DestroyAcceleratorTable( __hacc );
	}

	return TRUE;
}

