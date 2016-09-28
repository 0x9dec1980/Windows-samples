/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: p2string.h
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#ifndef _INC_P2STRING_H_
#define _INC_P2STRING_H_

BOOL __inline IsASpace(int CharIn)
{
    if ((CharIn >= 0x09 && CharIn <= 0x0D) || CharIn == 0x20) 
        return TRUE;
    else 
        return FALSE;
}

BOOL __inline IsADigit(int CharIn)
{
    if ((CharIn >= '0') && (CharIn <= '9')) 
        return TRUE;
    else 
        return FALSE;
}

long __inline StringToLong(const char *nptr)
{
    int     c;      /* current char */
    long    total;  /* current total */
    int     sign;   /* if '-', then negative, otherwise positive */

    /* skip whitespace */
    while ( IsASpace((int)(unsigned char)*nptr) )
        ++nptr;

    c = (int)(unsigned char)*nptr++;
    sign = c;                           /* save sign indication */
    if (c == '-' || c == '+')
        c = (int)(unsigned char)*nptr++;/* skip sign */

    total = 0;

    while (IsADigit(c)) 
    {
        total = 10 * total + (c - '0'); /* accumulate digit */
        c = (int)(unsigned char)*nptr++;/* get next char */
    }

    if (sign == '-')
        return -total;
    else
        return total;   /* return result, negated if necessary */
}

void __inline LongToString(unsigned long val, 
                           char *buf, 
                           unsigned radix, 
                           int is_neg)
{
    char *p;                /* pointer to traverse string */
    char *firstdig;         /* pointer to first digit */
    char temp;              /* temp char */
    unsigned digval;        /* value of digit */

    p = buf;

    if (is_neg) {/* negative, so output '-' and negate */
        *p++ = '-';
        val = (unsigned long)(-(long)val);
    }

    firstdig = p;   /* save pointer to first digit */

    do {
        digval = (unsigned) (val % radix);
        val /= radix;       /* get next digit */

        /* convert to ascii and store */
        if (digval > 9)
            *p++ = (char) (digval - 10 + 'a');  /* a letter */
        else
            *p++ = (char) (digval + '0');       /* a digit */
    } while (val > 0);

    /* 
     * We now have the digit of the number in the buffer, 
     * but in reverse order.  Thus we reverse them now. 
     */

    *p-- = '\0';            /* terminate string; p points to last digit */

    do {
        temp = *p;
        *p = *firstdig;
        *firstdig = temp;   /* swap *p and *firstdig */
        --p;
        ++firstdig;         /* advance to next two digits */
    } while (firstdig < p); /* repeat until halfway */
}

#endif /* _INC_P2STRING_H_ */