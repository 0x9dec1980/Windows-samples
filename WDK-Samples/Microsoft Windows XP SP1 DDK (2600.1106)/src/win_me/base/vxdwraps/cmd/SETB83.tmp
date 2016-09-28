#include "pch.h"

#pragma VxD_LOCKED_DATA_SEG

#pragma VxD_LOCKED_CODE_SEG

/****************************************************************************
 *
 *	CMDMenu - Display standard menu
 *
 *	ENTRY:	pszVxDName is the name of the VxD which wants this debugger.
 *
 *		pdcDebugCommands are the various debug commands.
 *
 *	EXIT:	None.
 *
 ***************************************************************************/
VOID CMD_LOCAL
CMDMenu(PCHAR pszVxDName, PCMDDC pdcDebugCommands)
{
	CHAR	cCommand;
	BOOL	fDoneOneCommand;
	PCMDDC	pdcCurrentCommand;

	for (;;) {

		//
		// Show our prompt.
		//
		CMDD("%s (space for help)? ", pszVxDName);

		//
		// Get the command character.
		//
		cCommand=CMDInChar();

		//
		// Lowercase it
		//
		cCommand|=0x20;

		//
		// Show it.
		//
		CMDD("%c\n", cCommand);

		//
		// If we need to quit, quit.
		//
		if (cCommand=='q')
			break;

		//
		// Check if we will do something.
		//
		fDoneOneCommand=FALSE;

		pdcCurrentCommand=pdcDebugCommands;
		while (pdcCurrentCommand->cLetter) {
			if (pdcCurrentCommand->cLetter==cCommand){
				fDoneOneCommand=TRUE;
				CMDD("%s\n", pdcCurrentCommand->pszShortName);
				pdcCurrentCommand->pFunction();
				break;
			}
			pdcCurrentCommand++;
		}

		if (!fDoneOneCommand) {
			CMDD("Welcome to %s's debugger\n", pszVxDName);

			pdcCurrentCommand=pdcDebugCommands;
			while (pdcCurrentCommand->cLetter) {
				CMDD(	"%c %s %s\n",
					pdcCurrentCommand->cLetter,
					pdcCurrentCommand->pszShortName,
					pdcCurrentCommand->pszExplanation);
				pdcCurrentCommand++;
			}
		}
	}
}
