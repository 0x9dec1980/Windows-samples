#include "windows.h"
#include "stdio.h"

void main (void)
{
	HANDLE event ;
	int x ;
	
	event = OpenEvent (SYNCHRONIZE, FALSE, "Async2SignalEvt") ;

	if (event == NULL)
	{
		printf ("Error opening event: %d\n", GetLastError()) ;
		return ;
	}

	for (x = 0 ; x < 10 ; x++)
	{
	
	printf ("Tick in: %d .....", GetTickCount()) ;
	WaitForSingleObject (event, INFINITE) ;

	printf ("Event signalled! .... Tick out: %d\n", GetTickCount()) ;
	printf ("Waiting.....") ;
	}

	CloseHandle (event) ;
}
