#include <conio.h>
#include <windows.h>
#include <stdio.h>
#include "mlioctl.h"

ALLOCATE_POOL Pool[20];
UCHAR PoolOffset = 0;

VOID main( ULONG argc, PCHAR argv[] )
{
    HANDLE hTagTest;

    hTagTest = CreateFile( "\\\\.\\TagTest",
    		GENERIC_READ | GENERIC_WRITE,
    		0,
    		NULL,
    		CREATE_ALWAYS,
    		FILE_ATTRIBUTE_NORMAL,
    		NULL);

	if ( hTagTest != (HANDLE) -1 )
	{

        printf ( "Obtained handle to TagTest...\n" );

    }
    else 
    {
        printf ( "Can't get a handle to TagTest...\n" );

		printf ( "Exiting...\n" );

		ExitProcess ( 1 );
    }

	initializePool ();

	exerciseMemory ( hTagTest );

	CloseHandle ( hTagTest );

	printf ( "Closed handle to TagTest\n");
}

VOID
exerciseMemory ( HANDLE handle )
{
	CHAR keystroke;

	printf ( "\n\nHit an 'a' to allocate memory.\n");

	printf ( "Hit a 'd' to deallocate memory.\n");

	printf ( "Hit a 'q' to quit.\n");

	for (;;)
	{
		keystroke = _getch ( );

		switch ( keystroke )
		{
			case 'a' :
			case 'A' :
				if ( PoolOffset < 20 )
				{

					allocateMemory (handle );

				}
				else
				{

					printf ( "You've allocated your limit.\n" );

				}

				break;

			case 'd' :
			case 'D' :
				if ( PoolOffset > 0 )
				{

					deAllocateMemory (handle );

				}
				else
				{

					printf ( "Nothing to deallocate.\n" );

				}

				break;

			case 'q' :
			case 'Q' :

				printf ( "Bye - bye...\n" );
				ExitProcess ( 0 );

			default :

				break;

		}
	}
}

VOID
allocateMemory ( HANDLE handle )
{
	BOOL bRc;
	ULONG bytesReturned;

	bRc = DeviceIoControl ( handle, 
					( DWORD ) IOCTL_EAPWT_LEAK_ALLOCATE_POOL, 
					&Pool[PoolOffset], 
					sizeof ( ALLOCATE_POOL ), 
					&Pool[PoolOffset], 
					sizeof ( ALLOCATE_POOL ), 
					&bytesReturned,
					NULL 
					);

	if ( bRc )
	{
		printf ( "%02d:Allocate Success - %0p\n", PoolOffset, Pool[PoolOffset++].Address );

	}
	else
	{
		printf ( "%02d: Allocate Failure - %0p\n", PoolOffset, Pool[PoolOffset].Address );

		ExitProcess ( 1 );

	}

}


VOID 
deAllocateMemory ( HANDLE handle )
{
	BOOL bRc;
	ULONG bytesReturned;

	printf ( "%02d: Deallocating %0p - ", PoolOffset, Pool[PoolOffset - 1].Address );

	bRc = DeviceIoControl ( handle, 
					(DWORD) IOCTL_EAPWT_LEAK_DEALLOCATE_POOL, 
					&Pool[--PoolOffset].Address, 
					sizeof ( PCHAR ), 
					NULL, 
					0,
					&bytesReturned,
					NULL 
					);

	if ( bRc )
	{
		printf ( "Success\n" );

	}
	else
	{
		printf ( "Failure\n" );

		ExitProcess ( 1 );
	}

}

VOID
initializePool ()
{
	ULONG i;

	for ( i = 0; i < 20; i++ )
	{
		Pool[i].Address = NULL;

		Pool[i].Tag = 'gaTX';

	}
}

