#include        <windef.h>
#include        <wingdi.h>
#include        <stdio.h>

#include        <ntmindrv.h>


NTMD_INIT    ntmdInit;                 /* Function address in RasDD */

/*
 *   Include the module initialisation function so that RasDD will
 * recognise our module.
 */

#define             _GET_FUNC_ADDR    1
#include            "../modinit.c"

int
CBFilterGraphics( lpdv, lpBuf, len )
void  *lpdv;
BYTE  *lpBuf;
int    len;
{
    static   BYTE  FlipTable[256] = {0};
    BYTE  *pFT;
    LPBYTE pb;
    int  i;

    if (!FlipTable[1])      //  need to initialize table
    {
        for( pFT = FlipTable, i = 0; i < sizeof( FlipTable ); i++ )
        {
        *pFT++ = (BYTE)( ((i << 7) & 0x80) |
                         ((i << 5) & 0x40) |
                         ((i << 3) & 0x20) |
                         ((i << 1) & 0x10) |
                         ((i >> 1) & 0x08) |
                         ((i >> 3) & 0x04) |
                         ((i >> 5) & 0x02) |
                         ((i >> 7) & 0x01) );
        }
    }

    for( pb = lpBuf, i = 0; i < len; i++, pb++ )
        *pb = FlipTable[ *pb ];

    ntmdInit.WriteSpoolBuf(lpdv, lpBuf, len);

    return len;
}
