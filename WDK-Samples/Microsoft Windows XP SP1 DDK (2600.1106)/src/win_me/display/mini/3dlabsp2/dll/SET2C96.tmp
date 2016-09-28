/******************************Module*Header**********************************\
*
*        ***************************************************
*        * Permedia 2: Direct Draw/Direct 3D   SAMPLE CODE *
*        ***************************************************
*
* Module Name: p2hwmisc.c
*
*       Contains functions to map the Surface Formats into the appropriate
*       Permedia 2 Register Values.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

//
// This array contains the permitted strides in Pixels and their
// corresponding partial products. The stride can only be a multiple of 32 pixels.
// Also the maximum allowable stride is 2048 pixels. Hence we can have an array of
// 2048/32 = 64 entries + 1 (for 0 width which is allowed). Given a surface of width = w,
// we can round it off to the next 32 pixel width by  WRounded = (w + 31) & ~31 
// We can then shift right WRounded by 5 effectively dividing by 32 to index into the
// array and get the Stride or Pitch and the corresponding partial products.
//
PERMEDIA2_STRIDEPP stridePartialProducts[]=
{   //Pitch in Pixels  PartialProducts(PP0 | PP1 << 3 | PP2 << 6) Surface Width in pixels
    {0,                 (0 | 0 << 3 | 0 << 6 )                  },  // 0
    {32,                (1 | 0 << 3 | 0 << 6 )                  },  // 32
    {64,                (1 | 1 << 3 | 0 << 6 )                  },  // 64
    {96,                (1 | 1 << 3 | 1 << 6 )                  },  // 96
    {128,               (2 | 1 << 3 | 1 << 6 )                  },  // 128
    {160,               (2 | 2 << 3 | 1 << 6 )                  },  // 160
    {192,               (2 | 2 << 3 | 2 << 6 )                  },  // 192
    {224,               (3 | 2 << 3 | 1 << 6 )                  },  // 224
    {256,               (3 | 2 << 3 | 2 << 6 )                  },  // 256
    {288,               (3 | 3 << 3 | 1 << 6 )                  },  // 288
    {320,               (3 | 3 << 3 | 2 << 6 )                  },  // 320
    {384,               (3 | 3 << 3 | 3 << 6 )                  },  // 352
    {384,               (3 | 3 << 3 | 3 << 6 )                  },  // 384
    {416,               (4 | 3 << 3 | 1 << 6 )                  },  // 416
    {448,               (4 | 3 << 3 | 2 << 6 )                  },  // 448
    {512,               (4 | 3 << 3 | 3 << 6 )                  },  // 480
    {512,               (4 | 3 << 3 | 3 << 6 )                  },  // 512
    {544,               (4 | 4 << 3 | 1 << 6 )                  },  // 544
    {576,               (4 | 4 << 3 | 2 << 6 )                  },  // 576
    {640,               (4 | 4 << 3 | 3 << 6 )                  },  // 608
    {640,               (4 | 4 << 3 | 3 << 6 )                  },  // 640
    {768,               (4 | 4 << 3 | 4 << 6 )                  },  // 672
    {768,               (4 | 4 << 3 | 4 << 6 )                  },  // 704
    {768,               (4 | 4 << 3 | 4 << 6 )                  },  // 736
    {768,               (4 | 4 << 3 | 4 << 6 )                  },  // 768
    {800,               (5 | 4 << 3 | 1 << 6 )                  },  // 800
    {832,               (5 | 4 << 3 | 2 << 6 )                  },  // 832
    {896,               (5 | 4 << 3 | 3 << 6 )                  },  // 864
    {896,               (5 | 4 << 3 | 3 << 6 )                  },  // 896
    {1024,              (5 | 4 << 3 | 4 << 6 )                  },  // 928
    {1024,              (5 | 4 << 3 | 4 << 6 )                  },  // 960
    {1024,              (5 | 4 << 3 | 4 << 6 )                  },  // 992
    {1024,              (5 | 4 << 3 | 4 << 6 )                  },  // 1024
    {1056,              (5 | 5 << 3 | 1 << 6 )                  },  // 1056
    {1088,              (5 | 5 << 3 | 2 << 6 )                  },  // 1088
    {1152,              (5 | 5 << 3 | 3 << 6 )                  },  // 1120
    {1152,              (5 | 5 << 3 | 3 << 6 )                  },  // 1152
    {1280,              (5 | 5 << 3 | 4 << 6 )                  },  // 1184
    {1280,              (5 | 5 << 3 | 4 << 6 )                  },  // 1216
    {1280,              (5 | 5 << 3 | 4 << 6 )                  },  // 1248
    {1280,              (5 | 5 << 3 | 4 << 6 )                  },  // 1280
    {1536,              (5 | 5 << 3 | 5 << 6 )                  },  // 1312
    {1536,              (5 | 5 << 3 | 5 << 6 )                  },  // 1344
    {1536,              (5 | 5 << 3 | 5 << 6 )                  },  // 1376
    {1536,              (5 | 5 << 3 | 5 << 6 )                  },  // 1408
    {1536,              (5 | 5 << 3 | 5 << 6 )                  },  // 1440
    {1536,              (5 | 5 << 3 | 5 << 6 )                  },  // 1472
    {1536,              (5 | 5 << 3 | 5 << 6 )                  },  // 1504
    {1536,              (5 | 5 << 3 | 5 << 6 )                  },  // 1536
    {1568,              (6 | 5 << 3 | 1 << 6 )                  },  // 1568
    {1600,              (6 | 5 << 3 | 2 << 6 )                  },  // 1600
    {1664,              (6 | 5 << 3 | 3 << 6 )                  },  // 1632
    {1664,              (6 | 5 << 3 | 3 << 6 )                  },  // 1664
    {1792,              (6 | 5 << 3 | 4 << 6 )                  },  // 1696
    {1792,              (6 | 5 << 3 | 4 << 6 )                  },  // 1728
    {1792,              (6 | 5 << 3 | 4 << 6 )                  },  // 1760
    {1792,              (6 | 5 << 3 | 4 << 6 )                  },  // 1792
    {2048,              (6 | 5 << 3 | 5 << 6 )                  },  // 1824
    {2048,              (6 | 5 << 3 | 5 << 6 )                  },  // 1856
    {2048,              (6 | 5 << 3 | 5 << 6 )                  },  // 1888
    {2048,              (6 | 5 << 3 | 5 << 6 )                  },  // 1920
    {2048,              (6 | 5 << 3 | 5 << 6 )                  },  // 1952
    {2048,              (6 | 5 << 3 | 5 << 6 )                  },  // 1984
    {2048,              (6 | 5 << 3 | 5 << 6 )                  },  // 2016
    {2048,              (6 | 5 << 3 | 5 << 6 )                  },  // 2048
};


/*
 * Function:    GetP2SurfaceFormat  
 * Description: Converts the surface format information described in DDPIXELFORMAT
 *              into the equivalent Permedia2 Internal Format
 */
VOID 
GetP2SurfaceFormat(DDPIXELFORMAT        *pPixFormat,
                   PP2_SURFACE_FORMAT   pP2SurfaceFormat)
{
    switch(pPixFormat->dwRGBBitCount)
    {
        case 8:
            pP2SurfaceFormat->dwP2PixelSize = 0;
            pP2SurfaceFormat->dwP2Format    = PERMEDIA_8BIT_PALETTEINDEX;
            pP2SurfaceFormat->dwP2FormatExt = PERMEDIA_8BIT_PALETTEINDEX_EXTENSION;
            pP2SurfaceFormat->dwP2ColorOrder = 0; // not really relevant
        break;

        case 16:
            pP2SurfaceFormat->dwP2PixelSize = 1;
            if ((pPixFormat->dwRBitMask == 0x7c00)&&
                (pPixFormat->dwGBitMask == 0x03e0)&&
                (pPixFormat->dwBBitMask == 0x001f)&&
                (pPixFormat->dwRGBAlphaBitMask == 0x8000))
            {//1:5:5:5
                pP2SurfaceFormat->dwP2Format    = PERMEDIA_5551_RGB;
                pP2SurfaceFormat->dwP2FormatExt = PERMEDIA_5551_RGB_EXTENSION;
                pP2SurfaceFormat->dwP2ColorOrder = COLOR_MODE; 
            }
            else if ((pPixFormat->dwRBitMask == 0xf800)&&
                    (pPixFormat->dwGBitMask == 0x07e0)&&
                    (pPixFormat->dwBBitMask == 0x001f)&&
                    (pPixFormat->dwRGBAlphaBitMask == 0x0000))
            {//5:6:5
                pP2SurfaceFormat->dwP2Format    = PERMEDIA_565_RGB;
                pP2SurfaceFormat->dwP2FormatExt = PERMEDIA_565_RGB_EXTENSION;
                pP2SurfaceFormat->dwP2ColorOrder = COLOR_MODE; 
            }
            else 
            {//4:4:4:4
                pP2SurfaceFormat->dwP2Format    = PERMEDIA_444_RGB;
                pP2SurfaceFormat->dwP2FormatExt = PERMEDIA_444_RGB_EXTENSION;
                pP2SurfaceFormat->dwP2ColorOrder = COLOR_MODE; 
            }
        break;

        case 32:
            pP2SurfaceFormat->dwP2PixelSize = 2;
            if ((pPixFormat->dwRBitMask == 0x00ff0000)&&
                (pPixFormat->dwGBitMask == 0x0000ff00)&&
                (pPixFormat->dwBBitMask == 0x000000ff)&&
                (pPixFormat->dwRGBAlphaBitMask == 0xff000000))
            {//8:8:8:8 
                pP2SurfaceFormat->dwP2Format    = PERMEDIA_8888_RGB;
                pP2SurfaceFormat->dwP2FormatExt = PERMEDIA_8888_RGB_EXTENSION;
                pP2SurfaceFormat->dwP2ColorOrder = COLOR_MODE; 
            }
            else
            {//0:8:8:8
                pP2SurfaceFormat->dwP2Format    = PERMEDIA_888_RGB;
                pP2SurfaceFormat->dwP2FormatExt = PERMEDIA_888_RGB_EXTENSION;
                pP2SurfaceFormat->dwP2ColorOrder = COLOR_MODE; 
            }
        break;
    }
}//GetP2SurfaceFormat

/*
 * Function:    GetP2ChromaLevelBounds  
 * Description: Converts the bounds of the Chroma values provided in locations referenced 
 *              by the pointers pdwLowerBound and pdwUppperBound into the equivalent 
 *              Permedia2 Internal Format
 */

VOID
GetP2ChromaLevelBounds(PP2_SURFACE_FORMAT   pP2SurfaceFormat,
                       DWORD                *pdwLowerBound,
                       DWORD                *pdwUpperBound)
{
    switch (pP2SurfaceFormat->dwP2Format)
    {
        case PERMEDIA_8BIT_PALETTEINDEX:
            *pdwLowerBound = CHROMA_LOWER_ALPHA(FORMAT_PALETTE_32BIT(*pdwLowerBound));
            *pdwUpperBound = CHROMA_UPPER_ALPHA(FORMAT_PALETTE_32BIT(*pdwUpperBound));
        break;
        case PERMEDIA_444_RGB:
            *pdwLowerBound = CHROMA_LOWER_ALPHA(FORMAT_4444_32BIT_BGR(*pdwLowerBound));
            *pdwUpperBound = CHROMA_UPPER_ALPHA(FORMAT_4444_32BIT_BGR(*pdwUpperBound));
            // Acount for the internal 8888 -> 4444 translation
            // which causes bilinear chroma keying to fail in
            // some cases
            *pdwLowerBound = *pdwLowerBound & 0xF0F0F0F0;
            *pdwUpperBound = *pdwUpperBound | 0x0F0F0F0F;
        break;
        case PERMEDIA_5551_RGB:
            *pdwLowerBound = CHROMA_LOWER_ALPHA(FORMAT_5551_32BIT_BGR(*pdwLowerBound));
            *pdwUpperBound = CHROMA_UPPER_ALPHA(FORMAT_5551_32BIT_BGR(*pdwUpperBound));
            // Acount for the internal 8888 -> 5551 translation
            // which causes bilinear chroma keying to fail in
            // some cases
            *pdwLowerBound = *pdwLowerBound & 0x80F8F8F8;
            *pdwUpperBound = *pdwUpperBound | 0x7F070707;
        break;
        case PERMEDIA_8888_RGB:
            // dwp2Format is identical for PERMEDIA_8888_RGB and PERMEDIA_565_RGB
            // hence we need to check PERMEDIA_565_RGB_EXTENSION
            if (pP2SurfaceFormat->dwP2FormatExt == PERMEDIA_565_RGB_EXTENSION)
            {
                *pdwLowerBound = CHROMA_LOWER_ALPHA(FORMAT_565_32BIT_BGR(*pdwLowerBound));
                *pdwUpperBound = CHROMA_UPPER_ALPHA(FORMAT_565_32BIT_BGR(*pdwUpperBound));
                *pdwLowerBound = *pdwLowerBound & 0xF8F8FCF8;
                *pdwUpperBound = *pdwUpperBound | 0x07070307;
            }
            else
            {
                *pdwLowerBound = CHROMA_LOWER_ALPHA(FORMAT_8888_32BIT_BGR(*pdwLowerBound));
                *pdwUpperBound = CHROMA_UPPER_ALPHA(FORMAT_8888_32BIT_BGR(*pdwUpperBound));
            }
        break;
        case PERMEDIA_888_RGB:
            *pdwLowerBound = CHROMA_LOWER_ALPHA(FORMAT_8888_32BIT_BGR(*pdwLowerBound));
            *pdwUpperBound = CHROMA_UPPER_ALPHA(FORMAT_8888_32BIT_BGR(*pdwUpperBound));
        break;
    }
}//GetP2ChromaLevelBounds