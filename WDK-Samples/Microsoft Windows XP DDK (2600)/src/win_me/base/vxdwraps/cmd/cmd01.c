#include "pch.h"

#pragma VxD_LOCKED_DATA_SEG

#pragma VxD_LOCKED_CODE_SEG

#pragma warning (disable:4035)	// turn off no return code warning

/****************************************************************************
 *
 *	CMDInChar - Get a character from the debug terminal
 *
 *	ENTRY:	None.
 *
 *	EXIT:	ASCII character.
 *
 ***************************************************************************/
CHAR CMD_LOCAL
CMDInChar(VOID)
{
	VMMCall(In_Debug_Chr);
}

#pragma warning (default:4035)	// turn on must return value warning

