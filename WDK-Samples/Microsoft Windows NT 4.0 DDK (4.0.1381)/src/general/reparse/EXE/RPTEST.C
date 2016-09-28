#include <windows.h>
#include <stdio.h>

VOID main( ULONG argc, PCHAR argv[] )
{
    HANDLE hRP;

    hRP = CreateFile( "\\\\.\\RP",
    		GENERIC_READ | GENERIC_WRITE,
    		0,
    		NULL,
    		CREATE_ALWAYS,
    		FILE_ATTRIBUTE_NORMAL,
    		NULL);

	if ( hRP != (HANDLE) -1 )
	{

        printf ( "Obtained handle to RP...\n" );

    }
    else 
    {
        printf ( "Can't get a handle to RP...\n" );

		printf ( "Error:%d\n", GetLastError ( ) );

		printf ( "Exiting...\n" );

		//exit ( 1 );
                                ExitProcess( 1 );
    }

	CloseHandle ( hRP );

	printf ( "Closed handle to RP.\n");
}

