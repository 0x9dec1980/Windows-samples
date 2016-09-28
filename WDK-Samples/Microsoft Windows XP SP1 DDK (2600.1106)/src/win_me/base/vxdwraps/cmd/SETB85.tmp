#include "pch.h"

#pragma VxD_LOCKED_DATA_SEG

#pragma VxD_LOCKED_CODE_SEG

/****************************************************************************
 *
 *	CMDReadNumber - Returns an hex number read from the debug terminal
 *
 *	ENTRY:	pszQuestion is the prompt (can be NULL).
 *
 *		bNumDigits is the number of hex digits of maximum input (1-8).
 *
 *		fAppendCrLf is TRUE if a carriage return is wanted after the
 *		input.
 *
 *	EXIT:	A DWORD being the inputted value.
 *
 ***************************************************************************/
DWORD CMD_LOCAL
CMDReadNumber(PCHAR pszQuestion, BYTE bNumDigits, BOOL fAppendCrLf)
{
	BYTE	bPos=bNumDigits;
	DWORD	dwResult=0;
	CHAR	cDigit;

	//
	// Display our prompt.
	//
	if (pszQuestion)
		CMDD("%s ? ", pszQuestion);

	//
	// As long as we have characters to input.
	//
	while (bPos) {

		cDigit=CMDInChar();

		switch (cDigit) {
			case 0x28:
			case '\b':
				if (bPos!=bNumDigits) {
					bPos++;
					dwResult=dwResult>>4;
					CMDD("\b \b");
				}
				break;

			case ' ':
				while (bPos!=0) {
					dwResult=dwResult<<4;
					bPos--;
					CMDD("0");
				}
				break;

			case 0x2D:
			case 0x0D:
				bPos=0;
				break;

			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				cDigit-=0x20;
				// No break

			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				cDigit-=('A'-'9'-1);
				// No break

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				cDigit-='0';
				dwResult=(dwResult<<4)+cDigit;
				bPos--;
				CMDD("%01X", cDigit);
				break;
		}
	}

	if (fAppendCrLf)
		CMDD("\n");

	return(dwResult);
}


