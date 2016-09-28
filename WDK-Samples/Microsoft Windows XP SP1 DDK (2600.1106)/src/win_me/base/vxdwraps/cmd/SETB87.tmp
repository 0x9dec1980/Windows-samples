#include "pch.h"

#pragma VxD_LOCKED_DATA_SEG

#pragma VxD_LOCKED_CODE_SEG

/***LP  CMDGetString - Read a string from the debug terminal
 *
 *  ENTRY
 *      pszPrompt -> prompt string
 *      pszBuff -> buffer to hold the string
 *      dwcbLen - buffer length
 *      fUpper - TRUE if convert to upper case
 *
 *  EXIT
 *      returns the number of characters read including the terminating newline.
 */

DWORD CMD_LOCAL CMDGetString(PCHAR pszPrompt, PCHAR pszBuff, DWORD dwcbLen,
                             BOOL fUpper)
{
    DWORD i;
    CHAR  ch;

    if (pszPrompt != NULL)
	CMDD(pszPrompt);

    for (i = 0; i < dwcbLen - 1; ++i)
    {
        ch = CMDInChar();

        if ((ch == '\r') || (ch == '\n'))
	{
            pszBuff[i] = '\n';
	    CMDD("\n");
            break;
	}
        else if (ch == '\b')
        {
            if (i > 0)
                i -= 2;
	    else
	    {
		ch = '\a';
		i--;
	    }
        }
        else if (fUpper && (ch >= 'a') && (ch <= 'z'))
            pszBuff[i] = (BYTE)(ch - 'a' + 'A');
        else if ((ch < ' ') || (ch > '~'))
        {
            ch = '\a';          //change it to a BELL character
            i--;                //don't store it
        }
        else
            pszBuff[i] = ch;

        CMDD("%c", ch);
    }
    pszBuff[i] = '\0';

    return _lstrlen(pszBuff);
}       //GetString
