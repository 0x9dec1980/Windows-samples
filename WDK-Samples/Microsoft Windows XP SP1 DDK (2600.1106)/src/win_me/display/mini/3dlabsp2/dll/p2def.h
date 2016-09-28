/******************************Module*Header**********************************\
*
*       *************************************************
*       * Permedia 2: Direct Draw/Direct 3D SAMPLE CODE *
*       *************************************************
*
* Module Name: p2def.h
*
* Content:     definitions for Permedia 2 registers.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef _DLL_P2DEF_H_
#define _DLL_P2DEF_H_

// Texture operations
#define _P2_TEXTURE_MODULATE         0
#define _P2_TEXTURE_DECAL            1
#define _P2_TEXTURE_SPECULAR         4
#define _P2_TEXTURE_COPY             3

#define _P2_TEXTURE_CLAMP            0
#define _P2_TEXTURE_MIRROR           2
#define _P2_TEXTURE_REPEAT           1

/*-----------------------------------------------------*/
/* Permedia Register Fields */
/* --------------------- */

/* Common */
#define __PERMEDIA_ENABLE                         1
#define __PERMEDIA_DISABLE                        0

// From FBReadMode
#define __PERMEDIA_PATCH          0
#define __PERMEDIA_SUBPATCH       1
#define __PERMEDIA_SUBPATCHPACK   2

#define __PERMEDIA_8BITPIXEL      0
#define __PERMEDIA_16BITPIXEL     1
#define __PERMEDIA_32BITPIXEL     2
#define __PERMEDIA_4BITPIXEL      3
#define __PERMEDIA_24BITPIXEL     4

/* Host In */
#define __PERMEDIA_TAG_MODE_HOLD      0
#define __PERMEDIA_TAG_MODE_INCREMENT 1
#define __PERMEDIA_TAG_MODE_INDEXED   2
#define __PERMEDIA_TAG_MODE_RESERVED  3

/* Host out (Filter) options. */

#define __PERMEDIA_FILTER_TAG           0x1
#define __PERMEDIA_FILTER_DATA          0x2
#define __PERMEDIA_FILTER_TAG_AND_DATA  0x3

#define __PERMEDIA_STATS_COMPARE_INSIDE_REGION    0
#define __PERMEDIA_STATS_COMPARE_OUTSIDE_REGION   1
#define __PERMEDIA_STATS_EXCLUDE_PASSIVE_STEPS    0
#define __PERMEDIA_STATS_INCLUDE_PASSIVE_STEPS    1
#define __PERMEDIA_STATS_EXCLUDE_ACTIVE_STEPS     0
#define __PERMEDIA_STATS_INCLUDE_ACTIVE_STEPS     1
#define __PERMEDIA_STATS_TYPE_PICKING             0
#define __PERMEDIA_STATS_TYPE_EXTENT              1

/* Rasterization */

#define __PERMEDIA_LINE_PRIMITIVE                  0
#define __PERMEDIA_TRAPEZOID_PRIMITIVE             1
#define __PERMEDIA_POINT_PRIMITIVE                 2
#define __PERMEDIA_RECTANGLE_PRIMITIVE             3

#define __PERMEDIA_FLAT_SHADE_MODE                 0
#define __PERMEDIA_GOURAUD_SHADE_MODE              1

#define __PERMEDIA_FRACTION_ADJUST_NONE         0
#define __PERMEDIA_FRACTION_ADJUST_TRUNC        1
#define __PERMEDIA_FRACTION_ADJUST_HALF         2
#define __PERMEDIA_FRACTION_ADJUST_ALMOST_HALF  3

#define __PERMEDIA_START_BIAS_ZERO              0
#define __PERMEDIA_START_BIAS_HALF              1
#define __PERMEDIA_START_BIAS_ALMOST_HALF       2

/* Scissor */

/* Stipple */

#define __PERMEDIA_RESET_STIPPLE_COUNTERS         0
#define __PERMEDIA_LOAD_STIPPLE_COUNTERS          1
/* specifies pattern size as number of address bits less 1 */
#define __PERMEDIA_AREA_STIPPLE_2_PIXEL_PATTERN   0
#define __PERMEDIA_AREA_STIPPLE_4_PIXEL_PATTERN   1
#define __PERMEDIA_AREA_STIPPLE_8_PIXEL_PATTERN   2
#define __PERMEDIA_AREA_STIPPLE_16_PIXEL_PATTERN  3
#define __PERMEDIA_AREA_STIPPLE_32_PIXEL_PATTERN  4

/* Chroma Test Mode */
#define __PERMEDIA_CHROMA_FBSOURCE      0
#define __PERMEDIA_CHROMA_FBDATA        1
#define __PERMEDIA_CHROMA_INPUT_COLOR   2
#define __PERMEDIA_CHROMA_OUTPUT_COLOR  3
#define __PERMEDIA_CHROMA_INCLUDE       0
#define __PERMEDIA_CHROMA_EXCLUDE       1

/* Texture Filter Mode */
#define __PERMEDIA_TEXTUREFILTER_ALPHAMAPSENSE_INCLUDE    0
#define __PERMEDIA_TEXTUREFILTER_ALPHAMAPSENSE_EXCLUDE    1

/* Local buffer read */

#define __PERMEDIA_PPCODE0  0
#define __PERMEDIA_PPCODE1  1
#define __PERMEDIA_PPCODE2  2
#define __PERMEDIA_PPCODE3  3
#define __PERMEDIA_PPCODE4  4
#define __PERMEDIA_PPCODE5  5
#define __PERMEDIA_PPCODE6  6
#define __PERMEDIA_PPCODE7  7

#define __PERMEDIA_LBDEFAULT      0
#define __PERMEDIA_LBSTENCIL      1
#define __PERMEDIA_LBDEPTH        2

#define __PERMEDIA_FBDEFAULT       0
#define __PERMEDIA_FBCOLOR         1

#define __PERMEDIA_TOP_LEFT_WINDOW_ORIGIN       0
#define __PERMEDIA_BOTTOM_LEFT_WINDOW_ORIGIN    1

#define __PERMEDIA_DEPTH_WIDTH_16      0
#define __PERMEDIA_DEPTH_WIDTH_24      1
#define __PERMEDIA_DEPTH_WIDTH_32      2

#define __PERMEDIA_STENCIL_WIDTH_0     0
#define __PERMEDIA_STENCIL_WIDTH_4     1
#define __PERMEDIA_STENCIL_WIDTH_8     2

#define __PERMEDIA_STENCIL_POSITION_16    0
#define __PERMEDIA_STENCIL_POSITION_20    1 
#define __PERMEDIA_FRAMECOUNT_POSITION_24    2
#define __PERMEDIA_FRAMECOUNT_POSITION_28    3
#define __PERMEDIA_FRAMECOUNT_POSITION_32    4
#define __PERMEDIA_FRAMECOUNT_POSITION_36    5
#define __PERMEDIA_FRAMECOUNT_POSITION_40    6

#define __PERMEDIA_GID_WIDTH_0        0
#define __PERMEDIA_GID_WIDTH_4        1
#define __PERMEDIA_GID_POSITION_16    0
#define __PERMEDIA_GID_POSITION_20    1
#define __PERMEDIA_GID_POSITION_24    2
#define __PERMEDIA_GID_POSITION_28    3
#define __PERMEDIA_GID_POSITION_32    4
#define __PERMEDIA_GID_POSITION_36    5
#define __PERMEDIA_GID_POSITION_40    6
#define __PERMEDIA_GID_POSITION_44    7
#define __PERMEDIA_GID_POSITION_48    8

/* FBReadMode */
#define __PERMEDIA_FBREAD_FBDEFAULT             0
#define __PERMEDIA_FBREAD_FBCOLOR               1
#define __PERMEDIA_FBREAD_ORIGINTOPLEFT         0
#define __PERMEDIA_FBREAD_ORIGINBOTTOMLEFT      1
#define __PERMEDIA_FBREAD_FBSOURCE_CONSTANT     0
#define __PERMEDIA_FBREAD_FBSOURCE_INDEX        1
#define __PERMEDIA_FBREAD_FBSOURCE_COORDINATE   2


/* Pixel ownership */
#define __PERMEDIA_GID_COMPARE_ALWAYS       0
#define __PERMEDIA_GID_COMPARE_NEVER        1
#define __PERMEDIA_GID_COMPARE_EQUAL        2
#define __PERMEDIA_GID_COMPARE_NOT_EQUAL    3
#define __PERMEDIA_GID_LBUPDATE_LBDATA      0
#define __PERMEDIA_GID_LBUPDATE_REGISTER    1

/* Stencil Test */

#define __PERMEDIA_STENCIL_BUFFER_WIDTH_4            0
#define __PERMEDIA_STENCIL_BUFFER_WIDTH_8            1
#define __PERMEDIA_STENCIL_BUFFER_WIDTH_1            2

#define __PERMEDIA_STENCIL_SOURCE_TEST_LOGIC        0
#define __PERMEDIA_STENCIL_SOURCE_STENCIL_MSG       1
#define __PERMEDIA_STENCIL_SOURCE_SOURCE_STENCIL    2
#define __PERMEDIA_STENCIL_SOURCE_LBSOURCEDATA_MSG  3

#define __PERMEDIA_STENCIL_COMPARE_MODE_NEVER               0
#define __PERMEDIA_STENCIL_COMPARE_MODE_LESS                1
#define __PERMEDIA_STENCIL_COMPARE_MODE_EQUAL               2
#define __PERMEDIA_STENCIL_COMPARE_MODE_LESS_OR_EQUAL       3
#define __PERMEDIA_STENCIL_COMPARE_MODE_GREATER             4
#define __PERMEDIA_STENCIL_COMPARE_MODE_NOT_EQUAL           5
#define __PERMEDIA_STENCIL_COMPARE_MODE_GREATER_OR_EQUAL    6
#define __PERMEDIA_STENCIL_COMPARE_MODE_ALWAYS              7

#define __PERMEDIA_STENCIL_METHOD_KEEP          0
#define __PERMEDIA_STENCIL_METHOD_ZERO          1
#define __PERMEDIA_STENCIL_METHOD_REPLACE       2
#define __PERMEDIA_STENCIL_METHOD_INCR          3
#define __PERMEDIA_STENCIL_METHOD_DECR          4
#define __PERMEDIA_STENCIL_METHOD_INVERT        5


/* Depth Test */

#define __PERMEDIA_DEPTH_SOURCE_DDA                 0
#define __PERMEDIA_DEPTH_SOURCE_SOURCE_DEPTH        1
#define __PERMEDIA_DEPTH_SOURCE_DEPTH_MSG           2
#define __PERMEDIA_DEPTH_SOURCE_LBSOURCEDATA_MSG    3

#define __PERMEDIA_DEPTH_COMPARE_MODE_NEVER             0
#define __PERMEDIA_DEPTH_COMPARE_MODE_LESS              1
#define __PERMEDIA_DEPTH_COMPARE_MODE_EQUAL             2
#define __PERMEDIA_DEPTH_COMPARE_MODE_LESS_OR_EQUAL     3
#define __PERMEDIA_DEPTH_COMPARE_MODE_GREATER           4
#define __PERMEDIA_DEPTH_COMPARE_MODE_NOT_EQUAL         5
#define __PERMEDIA_DEPTH_COMPARE_MODE_GREATER_OR_EQUAL  6
#define __PERMEDIA_DEPTH_COMPARE_MODE_ALWAYS            7

/* Local buffer write */

#define __PERMEDIA_WRITEMODE_NORMAL         0
#define __PERMEDIA_WRITEMODE_DOWRITE        1
#define __PERMEDIA_WRITEMODE_CANCELWRITE    2

#define __PERMEDIA_UPLOADDATA_NONE          0
#define __PERMEDIA_UPLOADDATA_LBDEPTH       1
#define __PERMEDIA_UPLOADDATA_LBSTENCIL     2

/* Texture / Fog  */

#define __PERMEDIA_TEXTURE_FILTER_NEAREST     0
#define __PERMEDIA_TEXTURE_FILTER_LINEAR2     1
#define __PERMEDIA_TEXTURE_FILTER_TRILINEAR4  2
#define __PERMEDIA_TEXTURE_FILTER_LINEAR4     3
#define __PERMEDIA_TEXTURE_FILTER_TRILINEAR8  4

/* Texture AddressMode */
#define __PERMEDIA_TEXADDRESS_WRAP_CLAMP        0
#define __PERMEDIA_TEXADDRESS_WRAP_REPEAT       1
#define __PERMEDIA_TEXADDRESS_WRAP_MIRROR       2
#define __PERMEDIA_TEXADDRESS_OPERATION_2D      0
#define __PERMEDIA_TEXADDRESS_OPERATION_3D      1
#define __PERMEDIA_TEXADDRESS_TEXMAP_1D         0
#define __PERMEDIA_TEXADDRESS_TEXMAP_2D         1

/* Texture ReadMode */
#define __PERMEDIA_TEXTUREREAD_FILTER_NEAREST           0
#define __PERMEDIA_TEXTUREREAD_FILTER_LINEAR            1
#define __PERMEDIA_TEXTUREREAD_FILTER_NEARMIPNEAREST    2
#define __PERMEDIA_TEXTUREREAD_FILTER_NEARMIPLINEAR     3
#define __PERMEDIA_TEXTUREREAD_FILTER_LINEARMIPNEAREST  4
#define __PERMEDIA_TEXTUREREAD_FILTER_LINEARMIPLINEAR   5
#define __PERMEDIA_TEXTUREREAD_WRAP_CLAMP               0
#define __PERMEDIA_TEXTUREREAD_WRAP_REPEAT              1
#define __PERMEDIA_TEXTUREREAD_WRAP_MIRROR              2
#define __PERMEDIA_TEXTUREREAD_TEXMAP_1D                0
#define __PERMEDIA_TEXTUREREAD_TEXMAP_2D                1
#define __PERMEDIA_TEXTUREREAD_FBSOURCE_NONE            0
#define __PERMEDIA_TEXTUREREAD_FBSOURCE_INDEX           1
#define __PERMEDIA_TEXTUREREAD_FBSOURCE_COORDINATE      2

/* Texture Format */
#define __PERMEDIA_TEXTUREFORMAT_LITTLE_ENDIAN              0
#define __PERMEDIA_TEXTUREFORMAT_BIG_ENDIAN                 1
#define __PERMEDIA_TEXTUREFORMAT_COMPONENTS_RGB             2
#define __PERMEDIA_TEXTUREFORMAT_COMPONENTS_RGBA            3
#define __PERMEDIA_TEXTUREFORMAT_OUTPUT_TEXEL               0
#define __PERMEDIA_TEXTUREFORMAT_OUTPUT_COLOR               1
#define __PERMEDIA_TEXTUREFORMAT_OUTPUT_BITMASK             2
#define __PERMEDIA_TEXTUREFORMAT_OUTPUT_ONECOMP_LUMA        0
#define __PERMEDIA_TEXTUREFORMAT_OUTPUT_ONECOMP_ALPHA       1
#define __PERMEDIA_TEXTUREFORMAT_OUTPUT_ONECOMP_INTESITY    2


/* Texture Color Mode */
#define __PERMEDIA_TEXCOLORMODE_BASEFORMAT_ALPHA       0
#define __PERMEDIA_TEXCOLORMODE_BASEFORMAT_LUMA        1
#define __PERMEDIA_TEXCOLORMODE_BASEFORMAT_LUMA_ALPHA  2
#define __PERMEDIA_TEXCOLORMODE_BASEFORMAT_INTENSITY   3
#define __PERMEDIA_TEXCOLORMODE_BASEFORMAT_RGB         4
#define __PERMEDIA_TEXCOLORMODE_BASEFORMAT_RGBA        5

#define __PERMEDIA_TEXCOLORMODE_TEXTURETYPE_APPLE    1
#define __PERMEDIA_TEXCOLORMODE_TEXTURETYPE_OPENGL   0

#define __PERMEDIA_TEXCOLORMODE_APPLICATION_MODULATE 0
#define __PERMEDIA_TEXCOLORMODE_APPLICATION_DECAL    1
#define __PERMEDIA_TEXCOLORMODE_APPLICATION_BLEND    2
#define __PERMEDIA_TEXCOLORMODE_APPLICATION_COPY     3



#define __PERMEDIA_MODULATE_TEXTURE      0
#define __PERMEDIA_DECAL_TEXTURE         1
#define __PERMEDIA_BLEND_TEXTURE         2
#define __PERMEDIA_OPENGL_COPY_TEXTURE   3

#define __PERMEDIA_OPENGL_APPMODE(n) ((n) << 1)
#define __PERMEDIA_OPENGL_HILITE_APPMODE(n) __PERMEDIA_OPENGL_APPMODE(n | 4)


/* Frame buffer read */

/* shares ppcode, number of reads and window origin defines
   with local buffer read */
#define __PERMEDIA_FBDATA    0
#define __PERMEDIA_FBCOLOR  1


// Config - combined mode register for P2
#define __PERMEDIA_CONFIG_FBREAD_SRC     0x00000001
#define __PERMEDIA_CONFIG_FBREAD_DST     0x00000002
#define __PERMEDIA_CONFIG_PACKED_DATA    0x00000004
#define __PERMEDIA_CONFIG_FBWRITE        0x00000008
#define __PERMEDIA_CONFIG_COLOR_DDA  0x00000010
#define __PERMEDIA_CONFIG_LOGICOP(x)    (0x00000020 | ((x & 0xF) << 6))

/* Antialias */

#define __PERMEDIA_COLOR_MODE_RGBA                  0
#define __PERMEDIA_COLOR_MODE_CI                    1


/* Alpha Blend */

#define __PERMEDIA_BLEND_FUNC_ZERO                      0
#define __PERMEDIA_BLEND_FUNC_ONE                       1
#define __PERMEDIA_BLEND_FUNC_SRC_ALPHA                 4
#define __PERMEDIA_BLEND_FUNC_ONE_MINUS_SRC_ALPHA       5

#define __PERMEDIA_BLENDOP_ONE_AND_ONE                  0x11
#define __PERMEDIA_BLENDOP_SRCALPHA_AND_INVSRCALPHA     0x54
#define __PERMEDIA_BLENDOP_ONE_AND_INVSRCALPHA          0x51
#define __PERMEDIA_BLENDOP_ONE_AND_ZERO                 0x01

/* Dither */

#define __PERMEDIA_COLOR_FORMAT_RGBA_8888            0
#define __PERMEDIA_COLOR_FORMAT_RGBA_5555            1
#define __PERMEDIA_COLOR_FORMAT_RGBA_4444            2
#define __PERMEDIA_COLOR_FORMAT_RGBA_4444_FRONT      3
#define __PERMEDIA_COLOR_FORMAT_RGBA_4444_BACK       4
#define __PERMEDIA_COLOR_FORMAT_RGB_332_FRONT        5
#define __PERMEDIA_COLOR_FORMAT_RGB_332_BACK         6
#define __PERMEDIA_COLOR_FORMAT_RGB_121_FRONT        7
#define __PERMEDIA_COLOR_FORMAT_RGB_121_BACK         8

#define __PERMEDIA_COLOR_FORMAT_CI_8                14
#define __PERMEDIA_COLOR_FORMAT_CI_4                15

#define __PERMEDIA_COLOR_ORDER_BGR                   0
#define __PERMEDIA_COLOR_ORDER_RGB                   1

/* Logical Ops/Write mask */

#define K_LOGICOP_CLEAR                   0
#define K_LOGICOP_AND                     1
#define K_LOGICOP_AND_REVERSE             2
#define K_LOGICOP_COPY                    3
#define K_LOGICOP_AND_INVERTED            4
#define K_LOGICOP_NOOP                    5
#define K_LOGICOP_XOR                     6
#define K_LOGICOP_OR                      7
#define K_LOGICOP_NOR                     8
#define K_LOGICOP_EQUIV                   9
#define K_LOGICOP_INVERT                 10
#define K_LOGICOP_OR_REVERSE             11
#define K_LOGICOP_COPY_INVERT            12
#define K_LOGICOP_OR_INVERT              13
#define K_LOGICOP_NAND                   14
#define K_LOGICOP_SET                    15

#define __PERMEDIA_ALL_WRITEMASKS_SET             0xFFFFFFFF

/* FB Write */

#define __PERMEDIA_BLOCK_WIDTH_8    0
#define __PERMEDIA_BLOCK_WIDTH_16   1
#define __PERMEDIA_BLOCK_WIDTH_32   2

/*-----------------------------------------------------*/
#define __PERMEDIA_3D_CONTEXT  1
#define __PERMEDIA_2D_CONTEXT  2

#define __PERMEDIA_ALPHA_FUNC_SCALE         255.0

#define __PERMEDIA_MIN_SUB_SCAN_LINES       4
#define __PERMEDIA_MAX_SUB_SCAN_LINES       8
/* converts diameter into number of subscanlines in radius */
#define __PERMEDIA_FASTEST_SMOOTH_POINT_RADIUS (__PERMEDIA_MIN_SUB_SCAN_LINES >> 1)
#define __PERMEDIA_NICEST_SMOOTH_POINT_RADIUS  (__PERMEDIA_MAX_SUB_SCAN_LINES >> 1)

#define __PERMEDIA_POINT_TABLE_LONGS  4

#define __PERMEDIA_MAX_PPC_ENTRIES  3

#define __PERMEDIA_PPCODE0_SHIFT  0
#define __PERMEDIA_PPCODE1_SHIFT  5
#define __PERMEDIA_PPCODE2_SHIFT  6
#define __PERMEDIA_PPCODE3_SHIFT  7
#define __PERMEDIA_PPCODE4_SHIFT  8
#define __PERMEDIA_PPCODE5_SHIFT  9
#define __PERMEDIA_PPCODE6_SHIFT  10
#define __PERMEDIA_PPCODE7_SHIFT  11



// Tags
// group 0
#define __Permedia2TagStartXDom                  0
#define __Permedia2TagdXDom                      0x1
#define __Permedia2TagStartXSub                  0x2
#define __Permedia2TagdXSub                      0x3
#define __Permedia2TagStartY                     0x4
#define __Permedia2TagdY                         0x5
#define __Permedia2TagCount                      0x6
#define __Permedia2TagRender                     0x7
#define __Permedia2TagContinueNewLine            0x8
#define __Permedia2TagContinueNewDom             0x9
#define __Permedia2TagContinueNewSub             0xa
#define __Permedia2TagContinue                   0xb
#define __Permedia2TagBitMaskPattern             0xd

// group 1
#define __Permedia2TagRasterizerMode             0x14
#define __Permedia2TagYLimits                    0x15
#define __Permedia2TagWaitForCompletion          0x17
#define __Permedia2TagXLimits                    0x19
#define __Permedia2TagRectangleOrigin            0x1a
#define __Permedia2TagRectangleSize              0x1b

//group 2
#define __Permedia2TagPackedDataLimits           0x2a

//group 3
#define __Permedia2TagScissorMode                0x30
#define __Permedia2TagScissorMinXY               0x31
#define __Permedia2TagScissorMaxXY               0x32
#define __Permedia2TagScreenSize                 0x33
#define __Permedia2TagAreaStippleMode            0x34
#define __Permedia2TagWindowOrigin               0x39

// group 4
#define __Permedia2TagAreaStipplePattern0        0x40
#define __Permedia2TagAreaStipplePattern1        0x41
#define __Permedia2TagAreaStipplePattern2        0x42
#define __Permedia2TagAreaStipplePattern3        0x43
#define __Permedia2TagAreaStipplePattern4        0x44
#define __Permedia2TagAreaStipplePattern5        0x45
#define __Permedia2TagAreaStipplePattern6        0x46
#define __Permedia2TagAreaStipplePattern7        0x47

// group 7
#define __Permedia2TagTextureAddressMode         0x70
#define __Permedia2TagSStart                     0x71
#define __Permedia2TagdSdx                       0x72
#define __Permedia2TagdSdyDom                    0x73
#define __Permedia2TagTStart                     0x74
#define __Permedia2TagdTdx                       0x75
#define __Permedia2TagdTdyDom                    0x76
#define __Permedia2TagQStart                     0x77
#define __Permedia2TagdQdx                       0x78
#define __Permedia2TagdQdyDom                    0x79

// group 9
#define __Permedia2TagTexelLUTIndex              0x98
#define __Permedia2TagTexelLUTData               0x99
#define __Permedia2TagTexelLUTAddress            0x9a
#define __Permedia2TagTexelLUTTransfer           0x9b

// group b
#define __Permedia2TagTextureBaseAddress         0xb0
#define __Permedia2TagTextureMapFormat           0xb1
#define __Permedia2TagTextureDataFormat          0xb2

// group c
#define __Permedia2TagTexel0                     0xc0
#define __Permedia2TagTextureReadMode            0xce
#define __Permedia2TagTexelLUTMode               0xcf

// group d
#define __Permedia2TagTextureColorMode           0xd0
#define __Permedia2TagFogMode                    0xd2
#define __Permedia2TagFogColor                   0xd3
#define __Permedia2TagFStart                     0xd4
#define __Permedia2TagdFdx                       0xd5
#define __Permedia2TagdFdyDom                    0xd6
#define __Permedia2TagKsStart                    0xd9
#define __Permedia2TagdKsdx                      0xda
#define __Permedia2TagdKsdyDom                   0xdb
#define __Permedia2TagKdStart                    0xdc
#define __Permedia2TagdKddx                      0xdd
#define __Permedia2TagdKddyDom                   0xde

// group f
#define __Permedia2TagRStart                     0xf0
#define __Permedia2TagdRdx                       0xf1
#define __Permedia2TagdRdyDom                    0xf2
#define __Permedia2TagGStart                     0xf3
#define __Permedia2TagdGdx                       0xf4
#define __Permedia2TagdGdyDom                    0xf5
#define __Permedia2TagBStart                     0xf6
#define __Permedia2TagdBdx                       0xf7
#define __Permedia2TagdBdyDom                    0xf8
#define __Permedia2TagAStart                     0xf9
#define __Permedia2TagColorDDAMode               0xfc
#define __Permedia2TagConstantColor              0xfd
#define __Permedia2TagColor                      0xfe

// group 0x10
#define __Permedia2TagAlphaBlendMode             0x102
#define __Permedia2TagDitherMode                 0x103
#define __Permedia2TagFBSoftwareWriteMask        0x104
#define __Permedia2TagLogicalOpMode              0x105
// undocumented reg:
#define __Permedia2TagFBWriteData                0x106

// group 0x11
#define __Permedia2TagLBReadMode                 0x110
#define __Permedia2TagLBReadFormat               0x111
#define __Permedia2TagLBSourceOffset             0x112
#define __Permedia2TagLBData                     0x113
#define __Permedia2TagLBStencil                  0x115
#define __Permedia2TagLBDepth                    0x116
#define __Permedia2TagLBWindowBase               0x117
#define __Permedia2TagLBWriteMode                0x118
#define __Permedia2TagLBWriteFormat              0x119
#define __Permedia2TagTextureData                0x11d
#define __Permedia2TagTextureDownloadOffset      0x11e

//group 0x13
#define __Permedia2TagWindow                     0x130
#define __Permedia2TagStencilMode                0x131
#define __Permedia2TagStencilData                0x132
#define __Permedia2TagStencil                    0x133
#define __Permedia2TagDepthMode                  0x134
#define __Permedia2TagDepth                      0x135
#define __Permedia2TagZStartU                    0x136
#define __Permedia2TagZStartL                    0x137
#define __Permedia2TagdZdxU                      0x138
#define __Permedia2TagdZdxL                      0x139
#define __Permedia2TagdZdyDomU                   0x13a
#define __Permedia2TagdZdyDomL                   0x13b

// group 0x15
#define __Permedia2TagFBReadMode                 0x150
#define __Permedia2TagFBSourceOffset             0x151
#define __Permedia2TagFBPixelOffset              0x152
#define __Permedia2TagFBColor                    0x153
#define __Permedia2TagFBData                     0x154
#define __Permedia2TagFBSourceData               0x155
#define __Permedia2TagFBWindowBase               0x156
#define __Permedia2TagFBWriteMode                0x157
#define __Permedia2TagFBHardwareWriteMask        0x158
#define __Permedia2TagFBBlockColor               0x159
#define __Permedia2TagFBReadPixel                0x15a
// undocumented register, but used (see packed blt)
#define __Permedia2TagFBWriteConfig              0x15d

// group 0x18
#define __Permedia2TagFilterMode                 0x180
#define __Permedia2TagStatisticMode              0x181
#define __Permedia2TagMinRegion                  0x182
#define __Permedia2TagMaxRegion                  0x183
#define __Permedia2TagResetPickResult            0x184
#define __Permedia2TagMinHitRegion               0x185
#define __Permedia2TagMaxHitRegion               0x186
#define __Permedia2TagPickResult                 0x187
#define __Permedia2TagSync                       0x188
#define __Permedia2TagFBBlockColorU              0x18d
#define __Permedia2TagFBBlockColorL              0x18e
#define __Permedia2TagSuspendUntilFrameBlank     0x18f

// group 0x1b
#define __Permedia2TagFBSourceBase               0x1b0
#define __Permedia2TagFBSourceDelta              0x1b1
#define __Permedia2TagConfig                     0x1b2

// group 0x1d
#define __Permedia2TagTexelLUT0                  0x1d0
#define __Permedia2TagTexelLUT1                  0x1d1
#define __Permedia2TagTexelLUT2                  0x1d2
#define __Permedia2TagTexelLUT3                  0x1d3
#define __Permedia2TagTexelLUT4                  0x1d4
#define __Permedia2TagTexelLUT5                  0x1d5
#define __Permedia2TagTexelLUT6                  0x1d6
#define __Permedia2TagTexelLUT7                  0x1d7
#define __Permedia2TagTexelLUT8                  0x1d8
#define __Permedia2TagTexelLUT9                  0x1d9
#define __Permedia2TagTexelLUT10                 0x1da
#define __Permedia2TagTexelLUT11                 0x1db
#define __Permedia2TagTexelLUT12                 0x1dc
#define __Permedia2TagTexelLUT13                 0x1dd
#define __Permedia2TagTexelLUT14                 0x1de
#define __Permedia2TagTexelLUT15                 0x1df

// group 0x1e
#define __Permedia2TagYUVMode                    0x1e0
#define __Permedia2TagChromaUpperBound           0x1e1
#define __Permedia2TagChromaLowerBound           0x1e2
#define __Permedia2TagAlphaMapUpperBound         0x1e3
#define __Permedia2TagAlphaMapLowerBound         0x1e4
#define __Permedia2TagTextureID                  0x1ee
#define __Permedia2TagTexelLUTID                 0x1ef

// group 0x20..
#define __Permedia2TagV0FixedS                   0x200
#define __Permedia2TagV0FixedT                   0x201
#define __Permedia2TagV0FixedQ                   0x202
#define __Permedia2TagV0FixedKs                  0x203
#define __Permedia2TagV0FixedKd                  0x204
#define __Permedia2TagV0FixedR                   0x205
#define __Permedia2TagV0FixedG                   0x206
#define __Permedia2TagV0FixedB                   0x207
#define __Permedia2TagV0FixedA                   0x208
#define __Permedia2TagV0FixedF                   0x209
#define __Permedia2TagV0FixedX                   0x20a
#define __Permedia2TagV0FixedY                   0x20b
#define __Permedia2TagV0FixedZ                   0x20c
#define __Permedia2TagV0FixedW                   0x20d
#define __Permedia2TagV1FixedS                   0x210
#define __Permedia2TagV1FixedT                   0x211
#define __Permedia2TagV1FixedQ                   0x212
#define __Permedia2TagV1FixedKs                  0x213
#define __Permedia2TagV1FixedKd                  0x214
#define __Permedia2TagV1FixedR                   0x215
#define __Permedia2TagV1FixedG                   0x216
#define __Permedia2TagV1FixedB                   0x217
#define __Permedia2TagV1FixedA                   0x218
#define __Permedia2TagV1FixedF                   0x219
#define __Permedia2TagV1FixedX                   0x21a
#define __Permedia2TagV1FixedY                   0x21b
#define __Permedia2TagV1FixedZ                   0x21c
#define __Permedia2TagV1FixedW                   0x21d
#define __Permedia2TagV2FixedS                   0x220
#define __Permedia2TagV2FixedT                   0x221
#define __Permedia2TagV2FixedQ                   0x222
#define __Permedia2TagV2FixedKs                  0x223
#define __Permedia2TagV2FixedKd                  0x224
#define __Permedia2TagV2FixedR                   0x225
#define __Permedia2TagV2FixedG                   0x226
#define __Permedia2TagV2FixedB                   0x227
#define __Permedia2TagV2FixedA                   0x228
#define __Permedia2TagV2FixedF                   0x229
#define __Permedia2TagV2FixedX                   0x22a
#define __Permedia2TagV2FixedY                   0x22b
#define __Permedia2TagV2FixedZ                   0x22c
#define __Permedia2TagV2FixedW                   0x22d
#define __Permedia2TagV0FloatS                   0x230
#define __Permedia2TagV0FloatT                   0x231
#define __Permedia2TagV0FloatQ                   0x232
#define __Permedia2TagV0FloatKs                  0x233
#define __Permedia2TagV0FloatKd                  0x234
#define __Permedia2TagV0FloatR                   0x235
#define __Permedia2TagV0FloatG                   0x236
#define __Permedia2TagV0FloatB                   0x237
#define __Permedia2TagV0FloatA                   0x238
#define __Permedia2TagV0FloatF                   0x239
#define __Permedia2TagV0FloatX                   0x23a
#define __Permedia2TagV0FloatY                   0x23b
#define __Permedia2TagV0FloatZ                   0x23c
#define __Permedia2TagV0FloatW                   0x23d
#define __Permedia2TagV1FloatS                   0x240
#define __Permedia2TagV1FloatT                   0x241
#define __Permedia2TagV1FloatQ                   0x242
#define __Permedia2TagV1FloatKs                  0x243
#define __Permedia2TagV1FloatKd                  0x244
#define __Permedia2TagV1FloatR                   0x245
#define __Permedia2TagV1FloatG                   0x246
#define __Permedia2TagV1FloatB                   0x247
#define __Permedia2TagV1FloatA                   0x248
#define __Permedia2TagV1FloatF                   0x249
#define __Permedia2TagV1FloatX                   0x24a
#define __Permedia2TagV1FloatY                   0x24b
#define __Permedia2TagV1FloatZ                   0x24c
#define __Permedia2TagV1FloatW                   0x24d
#define __Permedia2TagV2FloatS                   0x250
#define __Permedia2TagV2FloatT                   0x251
#define __Permedia2TagV2FloatQ                   0x252
#define __Permedia2TagV2FloatKs                  0x253
#define __Permedia2TagV2FloatKd                  0x254
#define __Permedia2TagV2FloatR                   0x255
#define __Permedia2TagV2FloatG                   0x256
#define __Permedia2TagV2FloatB                   0x257
#define __Permedia2TagV2FloatA                   0x258
#define __Permedia2TagV2FloatF                   0x259
#define __Permedia2TagV2FloatX                   0x25a
#define __Permedia2TagV2FloatY                   0x25b
#define __Permedia2TagV2FloatZ                   0x25c
#define __Permedia2TagV2FloatW                   0x25d
#define __Permedia2TagDeltaMode                  0x260
#define __Permedia2TagDrawTriangle               0x261
#define __Permedia2TagRepeatTriangle             0x262
#define __Permedia2TagDrawLine01                 0x263
#define __Permedia2TagDrawLine10                 0x264
#define __Permedia2TagRepeatLine                 0x265
// special define???
// #define __Permedia2TagBroadcastMask              0x26f
// #define __MaximumPermediaTagValue  __Permedia2TagBroadcastMask

#define __Permedia2TagAreaStipplePattern(i)     (0x040+(i))

typedef long __Permedia2Tag ;

/*-----------------------------------------------------*/
#define __PERMEDIA_MAX_YSAMPLES 8  

#define __PERMEDIA_SAMPLES 4

#if __PERMEDIA_SAMPLES == 8

#define __PERMEDIA_SUBSAMPLES  64
#define __PERMEDIA_XSUBSAMPLES 8
#define __PERMEDIA_YSUBSAMPLES 8
#define __PERMEDIA_COLOR_SCALE 4

#define __PERMEDIA_SAMPLENORM  0.015625
#define __PERMEDIA_SPANNORM    0.125

#define __PERMEDIA_XSUBPIXSCALE 8
#define __PERMEDIA_XSUBPIXBITS  3
#define __PERMEDIA_XSUBPIXSHIFT 3
#define __PERMEDIA_XSUBPIXMASK  7
#define __PERMEDIA_XFRACSHIFT   0

#define __PERMEDIA_YSUBPIXSCALE __PERMEDIA_XSUBPIXSCALE
#define __PERMEDIA_YSUBPIXBITS  __PERMEDIA_XSUBPIXBITS
#define __PERMEDIA_YSUBPIXSHIFT __PERMEDIA_XSUBPIXSHIFT
#define __PERMEDIA_YSUBPIXMASK  __PERMEDIA_XSUBPIXMASK

#elif __PERMEDIA_SAMPLES == 4

#define __PERMEDIA_SUBSAMPLES  16
#define __PERMEDIA_XSUBSAMPLES 4
#define __PERMEDIA_YSUBSAMPLES 4
#define __PERMEDIA_COLOR_SCALE 16

#define __PERMEDIA_SAMPLENORM   0.0625 
#define __PERMEDIA_SPANNORM     0.25  

#define __PERMEDIA_XSUBPIXSCALE 8
#define __PERMEDIA_XSUBPIXBITS  3 
#define __PERMEDIA_XSUBPIXSHIFT 3 
#define __PERMEDIA_XSUBPIXMASK  7 
#define __PERMEDIA_XFRACSHIFT   1

#define __PERMEDIA_YSUBPIXSCALE 4
#define __PERMEDIA_YSUBPIXBITS  2  
#define __PERMEDIA_YSUBPIXSHIFT 2  
#define __PERMEDIA_YSUBPIXMASK  3 

#elif __PERMEDIA_SAMPLES == 2

"NOT SUPPORTED YET"

#else  /* __PERMEDIA_SAMPLES == 1 Alias Case */

#define __PERMEDIA_SUBSAMPLES  1
#define __PERMEDIA_XSUBSAMPLES 1
#define __PERMEDIA_YSUBSAMPLES 1
#define __PERMEDIA_COLOR_SCALE 255

#define __PERMEDIA_SAMPLENORM   1
#define __PERMEDIA_SPANNORM     1

#define __PERMEDIA_XSUBPIXSCALE 8
#define __PERMEDIA_XSUBPIXBITS  3
#define __PERMEDIA_XSUBPIXSHIFT 3
#define __PERMEDIA_XSUBPIXMASK  7
 
#define __PERMEDIA_XFRACSHIFT   3

#define __PERMEDIA_YSUBPIXSCALE 1
#define __PERMEDIA_YSUBPIXBITS  0
#define __PERMEDIA_YSUBPIXSHIFT 0
#define __PERMEDIA_YSUBPIXMASK  ~0
#endif

/*-----------------------------------------------------*/
#define __PERMEDIA_POS_Z_FIXED_PT_SCALE  2147483647.0    /* 2**31 - 1*/
#define __PERMEDIA_NEG_Z_FIXED_PT_SCALE  1073741824.0    /* 2**30 */

#define __PERMEDIA_POS_Z_FIXED_PT_SCALE_RECIP  (1.0/2147483647.0)   
#define __PERMEDIA_NEG_Z_FIXED_PT_SCALE_RECIP  (1.0/1073741824.0)

#define RGB_MODE         1
#define COLOR_MODE       1 /* 0=BGR, 1=RGB */
#define DITHER_XOFFSET   0
#define DITHER_YOFFSET   0
#define DITHER_ENABLE    __PERMEDIA_ENABLE
#define MS_BIT_MASK             0x80000000l

// Shifts to enable units
// DepthMode
#define __PERMEDIA_DEPTHMODE_ENABLE 0
#define __PERMEDIA_DEPTHMODE_WRITEMASK 1
#define __PERMEDIA_DEPTHMODE_SOURCEDEPTH 2
#define __PERMEDIA_DEPTHMODE_COMPARISON 4

/* sync_mode definitions for the SuspendUntilFramBlank tag */

#define EXT_VIDEO_WAITFOR_FB     0
#define EXT_VIDEO_IMMEDIATE      1
#define VTG_FRAMEROW_WAITFOR_FB  2
#define VTG_FRAMEROW_IMMEDIATE   3

typedef struct PartialProductCode
{
    ULONG   cxDelta;
    ULONG   ulPartialProducts;
}
PPCODE;

extern const PPCODE aPartialProducts[];

#define GET_PPCODE(cxTextureWidth, cxTextureDelta, pp) \
{ \
    int i = (cxTextureWidth + 31) / 32; \
    cxTextureDelta  = aPartialProducts[i].cxDelta; \
    pp              = aPartialProducts[i].ulPartialProducts; \
}

// Color component order
#define INV_COLOR_MODE   0 /* 0=RGB, 1=BGR */

//
// these constants are used in the PERMEDIA_SURFACE structure,
// Format and FormatExtension
#define PERMEDIA_4BIT_PALETTEINDEX 15
#define PERMEDIA_4BIT_PALETTEINDEX_EXTENSION 0
#define PERMEDIA_8BIT_PALETTEINDEX 14
#define PERMEDIA_8BIT_PALETTEINDEX_EXTENSION 0
#define PERMEDIA_332_RGB 5
#define PERMEDIA_332_RGB_EXTENSION 0
#define PERMEDIA_2321_RGB 9
#define PERMEDIA_2321_RGB_EXTENSION 0
#define PERMEDIA_5551_RGB 1
#define PERMEDIA_5551_RGB_EXTENSION 0
#define PERMEDIA_565_RGB 0
#define PERMEDIA_565_RGB_EXTENSION 1
#define PERMEDIA_8888_RGB 0
#define PERMEDIA_8888_RGB_EXTENSION 0
#define PERMEDIA_888_RGB 4
#define PERMEDIA_888_RGB_EXTENSION 1
#define PERMEDIA_444_RGB 2
#define PERMEDIA_444_RGB_EXTENSION 0
#define PERMEDIA_YUV422 3
#define PERMEDIA_YUV422_EXTENSION 1
#define PERMEDIA_YUV411 2
#define PERMEDIA_YUV411_EXTENSION 1


// 
// Color formating helper defines
// they convert an RGB value in a certain format to a RGB 32 bit value
#define FORMAT_565_32BIT(val) \
( (((val & 0xF800) >> 8) << 16) |\
 (((val & 0x7E0) >> 3) << 8) |\
 ((val & 0x1F) << 3) )

#define FORMAT_565_32BIT_BGR(val)   \
    ( ((val & 0xF800) >> 8) |           \
      (((val & 0x7E0) >> 3) << 8) |     \
      ((val & 0x1F) << 19) )

#define FORMAT_5551_32BIT(val)      \
( (((val & 0x8000) >> 8) << 24) |\
 (((val & 0x7C00) >> 7) << 16) |\
 (((val & 0x3E0) >> 2) << 8) | ((val & 0x1F) << 3) )

#define FORMAT_5551_32BIT_BGR(val)  \
( (((val & 0x8000) >> 8) << 24) |       \
  ((val & 0x7C00) >> 7) |               \
  (((val & 0x3E0) >> 2) << 8) |         \
  ((val & 0x1F) << 19) )

#define FORMAT_4444_32BIT(val)          \
( ((val & 0xF000) << 16) | \
 (((val & 0xF00) >> 4) << 16) | \
 ((val & 0xF0) << 8) | ((val & 0xF) << 4) )

#define FORMAT_4444_32BIT_BGR(val)  \
( ((val & 0xF000) << 16) |              \
  ((val & 0xF00) >> 4) |                \
  ((val & 0xF0) << 8) |                 \
  ((val & 0xF) << 20) )

#define FORMAT_332_32BIT(val)           \
( ((val & 0xE0) << 16) |\
 (((val & 0x1C) << 3) << 8) |\
 ((val & 0x3) << 6) ) 

#define FORMAT_332_32BIT_BGR(val)   \
( (val & 0xE0) |                        \
  (((val & 0x1C) << 3) << 8) |          \
  ((val & 0x3) << 22) )

#define FORMAT_2321_32BIT(val)          \
( ((val & 0x80) << 24) | ((val & 0x60) << 17) |\
 (((val & 0x1C) << 3) << 8) | ((val & 0x3) << 6) ) 

#define FORMAT_2321_32BIT_BGR(val)      \
( ((val & 0x80) << 24) |                \
  ((val & 0x60) << 1) |                 \
  (((val & 0x1C) << 3) << 8) |          \
  ((val & 0x3) << 22) )

#define FORMAT_8888_32BIT_BGR(val)  \
( (val & 0xFF00FF00) | ( ((val & 0xFF0000) >> 16) | ((val & 0xFF) << 16) ) )

#define FORMAT_888_32BIT_BGR(val)   \
( (val & 0xFF00FF00) | ( ((val & 0xFF0000) >> 16) | ((val & 0xFF) << 16) ) )

#define CHROMA_UPPER_ALPHA(val) \
    (val | 0xFF000000)

#define CHROMA_LOWER_ALPHA(val) \
    (val & 0x00FFFFFF)

#define CHROMA_332_UPPER(val) \
    (val | 0x001F1F3F)

#define FORMAT_PALETTE_32BIT(val) \
    ( (val & 0xFF) | ((val & 0xFF) << 8) | ((val & 0xFF) << 16))

// DD Blit helper defines.
#define PIXELS_INTO_RECT_PACKED(rect, PixelPitch, lPixelMask) \
((rect->top * PixelPitch) + \
(rect->left & ~lPixelMask))

#define RECTS_PIXEL_OFFSET(rS,rD,SourcePitch,DestPitch,Mask) \
(PIXELS_INTO_RECT_PACKED(rS, SourcePitch, Mask) - \
 PIXELS_INTO_RECT_PACKED(rD, DestPitch, Mask) )

#define LINEAR_FUDGE(SourcePitch, DestPitch, rectDest) \
((DestPitch - SourcePitch) * (rectDest->top))

//
// Useful macros not defined in standard Permedia 2 header files. Generally, for
// speed we don't want to use the bitfield structures so we define the bit
// shifts to get at the various fields.
//
#define INTtoFIXED(i)               ((i) << 16)    // int to 16.16 fixed format
#define FIXEDtoINT(i)               ((i) >> 16)    // 16.16 fixed format to int
#define INTofFIXED(i)   ((i) & 0xffff0000)  // int part of 16.16
#define FRACTofFIXED(i) ((i) & 0xffff)      // fractional part of 16.16

#define FIXtoFIXED(i)   ((i) << 12)         // 12.4 to 16.16
#define FIXtoINT(i)     ((i) >> 4)          // 28.4 to 28

#define __PERMEDIA_CONSTANT_FB_WRITE   (1 << (4+1))
#define __COLOR_DDA_FLAT_SHADE      (__PERMEDIA_ENABLE | \
                                        (__PERMEDIA_FLAT_SHADE_MODE << 1))
#define __COLOR_DDA_GOURAUD_SHADE   (__PERMEDIA_ENABLE | \
                                        (__PERMEDIA_GOURAUD_SHADE_MODE << 1))

#define INVERT_BITMASK_BITS         (1 << 1)
#define BYTESWAP_BITMASK            (3 << 7)
#define FORCE_BACKGROUND_COLOR      (1 << 6)    // Permedia only

//
// Bits in the Render command
//
#define __RENDER_INCREASE_Y             (1 << 22)
#define __RENDER_INCREASE_X             (1 << 21)
#define __RENDER_VARIABLE_SPANS         (1 << 18)
#define __RENDER_REUSE_BIT_MASK         (1 << 17)
#define __RENDER_TEXTURE_ENABLE         (1 << 13)
#define __RENDER_SYNC_ON_HOST_DATA      (1 << 12)
#define __RENDER_SYNC_ON_BIT_MASK       (1 << 11)
#define __RENDER_RECTANGLE_PRIMITIVE    (__PERMEDIA_RECTANGLE_PRIMITIVE << 6)
#define __RENDER_TRAPEZOID_PRIMITIVE    (__PERMEDIA_TRAPEZOID_PRIMITIVE << 6)
#define __RENDER_LINE_PRIMITIVE         (__PERMEDIA_LINE_PRIMITIVE << 6)
#define __RENDER_POINT_PRIMITIVE        (__PERMEDIA_POINT_PRIMITIVE << 6)
#define __RENDER_FAST_FILL_INC(n)       (((n) >> 4) << 4) // n = 8, 16 or 32
#define __RENDER_FAST_FILL_ENABLE       (1 << 3)
#define __RENDER_RESET_LINE_STIPPLE     (1 << 2)
#define __RENDER_LINE_STIPPLE_ENABLE    (1 << 1)
#define __RENDER_AREA_STIPPLE_ENABLE    (1 << 0)
#define __RENDER_TEXTURED_PRIMITIVE     (1 << 13)

//
// Bits in the ScissorMode register
//
#define USER_SCISSOR_ENABLE             (1 << 0)
#define SCREEN_SCISSOR_ENABLE           (1 << 1)
#define SCISSOR_XOFFSET                 0
#define SCISSOR_YOFFSET                 16

//
// Bits in the FBReadMode register
//
#define __FB_READ_SOURCE                (1 << 9)
#define __FB_READ_DESTINATION           (1 << 10)
#define __FB_COLOR                      (1 << 15)
#define __FB_WINDOW_ORIGIN              (1 << 16)
#define __FB_PACKED_DATA                (1 << 19)
#define __FB_USE_PACKED                 (1 << 19)

//
// Extra bits in PERMEDIA FBReadMode
//
#define __FB_RELATIVE_OFFSET            20

//
// P2 also provides a version of Relative Offset in the PackedDataLimits
// register
//
#define __PDL_RELATIVE_OFFSET           29

//
// Bits in the LBReadMode register
//
#define __LB_READ_SOURCE                (1 << 9)
#define __LB_READ_DESTINATION           (1 << 10)
#define __LB_STENCIL                    (1 << 16)
#define __LB_DEPTH                      (1 << 17)
#define __LB_WINDOW_ORIGIN              (1 << 18)
#define __LB_READMODE_PATCH             (1 << 19)
#define __LB_SCAN_INTERVAL_2            (1 << 20)

//
// Bits in the DepthMode register
//
#define __DEPTH_ENABLE                  1
#define __DEPTH_WRITE_ENABLE            (1<<1)
#define __DEPTH_REGISTER_SOURCE         (2<<2)
#define __DEPTH_MSG_SOURCE              (3<<2)
#define __DEPTH_ALWAYS                  (7<<4)

//
// Bits in the LBReadFormat/LBWriteFormat registers
//
#define __LB_FORMAT_DEPTH32             2

//
// Macros to load indexed tags more efficiently than using __HwDMATag struct
//
#define P2_TAG_MAJOR(x)              ((x) & 0xff0)
#define P2_TAG_MINOR(x)              ((x) & 0x00f)

#define P2_TAG_MAJOR_INDEXED(x)                                          \
    ((__PERMEDIA_TAG_MODE_INDEXED << (5+4+1+4)) | P2_TAG_MAJOR(x))
#define P2_TAG_MINOR_INDEX(x)                                            \
    (1 << (P2_TAG_MINOR(x) + 16))

//
// Macro to take a permedia2 logical op and return the enabled LogcialOpMode bits
//
#define P2_ENABLED_LOGICALOP(op)     (((op) << 1) | __PERMEDIA_ENABLE)

#define RECTORIGIN_YX(y,x)              (((y) << 16) | ((x) & 0xFFFF))

//
// Area stipple shifts and bit defines
//
#define AREA_STIPPLE_XSEL(x)            ((x) << 1)
#define AREA_STIPPLE_YSEL(y)            ((y) << 4)
#define AREA_STIPPLE_XOFF(x)            ((x) << 7)
#define AREA_STIPPLE_YOFF(y)            ((y) << 12)
#define AREA_STIPPLE_INVERT_PAT         (1 << 17)
#define AREA_STIPPLE_MIRROR_X           (1 << 18)
#define AREA_STIPPLE_MIRROR_Y           (1 << 19)

//
// We always use 8x8 monochrome brushes.
//
#define AREA_STIPPLE_8x8_ENABLE                                             \
    (__PERMEDIA_ENABLE |                                                    \
    AREA_STIPPLE_XSEL(__PERMEDIA_AREA_STIPPLE_8_PIXEL_PATTERN) |            \
    AREA_STIPPLE_YSEL(__PERMEDIA_AREA_STIPPLE_8_PIXEL_PATTERN))

//
// RasteriserMode values
//
#define BIAS_NONE                  (__PERMEDIA_START_BIAS_ZERO << 4)
#define BIAS_HALF                  (__PERMEDIA_START_BIAS_HALF << 4)
#define BIAS_NEARLY_HALF           (__PERMEDIA_START_BIAS_ALMOST_HALF << 4)

#define FRADJ_NONE                 (__PERMEDIA_FRACTION_ADJUST_NONE << 2)
#define FRADJ_ZERO                 (__PERMEDIA_FRACTION_ADJUST_TRUNC << 2)
#define FRADJ_HALF                 (__PERMEDIA_FRACTION_ADJUST_HALF << 2)
#define FRADJ_NEARLY_HALF          (__PERMEDIA_FRACTION_ADJUST_ALMOST_HALF << 2)


//
// Some constants
//
#define ONE                         0x00010000
#define MINUS_ONE                   0xFFFF0000
#define PLUS_ONE                    ONE
#define NEARLY_ONE                  0x0000FFFF
#define HALF                        0x00008000
#define NEARLY_HALF                 0x00007FFF

//
// Max length of GIQ conformant lines that Permedia2 can draw
// Permedia has only 15 bits of fraction so reduce the lengths.
//
#define MAX_LENGTH_CONFORMANT_NONINTEGER_LINES  (16/2)
#define MAX_LENGTH_CONFORMANT_INTEGER_LINES     (194/2)

//
// We need to byte swap monochrome bitmaps. On 486 we can do this with
// fast assembler.
//
#if defined(_X86_)
//
// This only works on a 486 so the driver won't run on a 386.
//
#define LSWAP_BYTES(dst, pSrc)                                              \
{                                                                           \
    __asm mov eax, pSrc                                                     \
    __asm mov eax, [eax]                                                    \
    __asm bswap eax                                                         \
    __asm mov dst, eax                                                      \
}
#else
#define LSWAP_BYTES(dst, pSrc)                                              \
{                                                                           \
    ULONG   _src = *(ULONG *)pSrc;                                           \
    dst = ((_src) >> 24) |                                                   \
          ((_src) << 24) |                                                   \
          (((_src) >> 8) & 0x0000ff00) |                                     \
          (((_src) << 8) & 0x00ff0000);                                      \
}

#endif

// macro to swap the Red and Blue component of a 32 bit dword
//

#define SWAP_BR(a) ((a & 0xff00ff00l) | \
                   ((a&0xff0000l)>> 16) | \
                   ((a & 0xff) << 16))

//
// min. and max. values for Permedia PP register
#define MAX_PARTIAL_PRODUCT_P2          10
#define MIN_PARTIAL_PRODUCT_P2          5

//
// The following structures and macros define the memory map for the Permedia2
// control registers. We don't use this memory map to access Permedia2 registers
// since on Alpha machines we want to precompute the addresses. So we do
// a TRANSLATE_ADDR_ULONG on all the addresses here and save them into a
// P2RegAddrRec. We use that to obtain the addresses for the different
// registers.
//
typedef struct
{
    ULONG   reg;
    ULONG   pad;
} RAMDAC_REG;

//
// Macros to add padding words to the structures. For the core registers we use
// the tag ids when specifying the pad. So we must multiply by 8 to get a byte
// pad. We need to add an id to make each pad field in the struct unique. The
// id is irrelevant as long as it's different from every other id used in the
// same struct. It's a pity pad##__LINE__ doesn't work.
//
//#define PAD(id, n)              UCHAR   pad##id[n]


//
// Interrupt status bits
//
typedef enum
{
    DMA_INTERRUPT_AVAILABLE     = 0x01, // can use DMA interrupts
    VBLANK_INTERRUPT_AVAILABLE  = 0x02, // can use VBLANK interrupts
} INTERRUPT_CONTROL;

extern DWORD    LogicopReadDest[];  // Indicates which logic ops need dest read
                                    // turned on
#define DEFAULTWRITEMASK 0xffffffffl


// *******************************************************************
// Permedia Bit Field Macros

// FBReadMode 
#define PM_FBREADMODE_PARTIAL(a)           (a << 0)
#define PM_FBREADMODE_READSOURCE(a)        (a << 9)
#define PM_FBREADMODE_READDEST(a)          (a << 10)
#define PM_FBREADMODE_PATCHENABLE(a)       (a << 18)
#define PM_FBREADMODE_PACKEDDATA(a)        (a << 19)
#define PM_FBREADMODE_RELATIVEOFFSET(a)    (a << 20)
#define PM_FBREADMODE_PATCHMODE(a)         (a << 25)

// Texture read mode
#define PM_TEXREADMODE_ENABLE(a)         (a << 0)
#define PM_TEXREADMODE_WIDTH(a)          (a << 9)
#define PM_TEXREADMODE_HEIGHT(a)         (a << 13)
#define PM_TEXREADMODE_FILTER(a)         (a << 17)

// PackedDataLimits
#define PM_PACKEDDATALIMITS_OFFSET(a)    (a << 29)
#define PM_PACKEDDATALIMITS_XSTART(a)    (a << 16)
#define PM_PACKEDDATALIMITS_XEND(a)      (a << 0)

// Window Register
#define PM_WINDOW_LBUPDATESOURCE(a)      (a << 4)
#define PM_WINDOW_DISABLELBUPDATE(a)     (a << 18)

// Colors
#define PM_BYTE_COLOR(a) (a << 15)

// Config register
#define PM_CHIPCONFIG_AGPCAPABLE (1 << 9)


// Texture unit bit fields
// Texture color mode
#define PM_TEXCOLORMODE_ENABLE 0
#define PM_TEXCOLORMODE_APPLICATION 1
#define PM_TEXCOLORMODE_TEXTURETYPE 4

// Texture address mode
#define PM_TEXADDRESSMODE_ENABLE 0
#define PM_TEXADDRESSMODE_PERSPECTIVE 1
#define PM_TEXADDRESSMODE_FAST 2

// Texture map format
#define PM_TEXMAPFORMAT_PP0 0
#define PM_TEXMAPFORMAT_PP1 3
#define PM_TEXMAPFORMAT_PP2 6
#define PM_TEXMAPFORMAT_TEXELSIZE 19

// Texture data format
#define PM_TEXDATAFORMAT_ALPHAMAP_EXCLUDE 2
#define PM_TEXDATAFORMAT_ALPHAMAP_INCLUDE 1
#define PM_TEXDATAFORMAT_ALPHAMAP_DISABLE 0

#define PM_TEXDATAFORMAT_FORMAT 0
#define PM_TEXDATAFORMAT_NOALPHAPIXELS 4
#define PM_TEXDATAFORMAT_FORMATEXTENSION 6
#define PM_TEXDATAFORMAT_COLORORDER 5

// Dither unit bit fields
#define PM_DITHERMODE_ENABLE 0
#define PM_DITHERMODE_DITHERENABLE 1
#define PM_DITHERMODE_COLORFORMAT 2
#define PM_DITHERMODE_XOFFSET 6
#define PM_DITHERMODE_YOFFSET 8
#define PM_DITHERMODE_COLORORDER 10
#define PM_DITHERMODE_DITHERMETHOD 11
#define PM_DITHERMODE_FORCEALPHA 12
#define PM_DITHERMODE_COLORFORMATEXTENSION 16

// Alpha Blend unit bit fields
#define PM_ALPHABLENDMODE_ENABLE 0
#define PM_ALPHABLENDMODE_OPERATION 1
#define PM_ALPHABLENDMODE_COLORFORMAT 8
#define PM_ALPHABLENDMODE_COLORORDER 13
#define PM_ALPHABLENDMODE_BLENDTYPE 14
#define PM_ALPHABLENDMODE_COLORFORMATEXTENSION 16

// Window register
#define PM_WINDOW_LBUPDATESOURCE_LBSOURCEDATA 0
#define PM_WINDOW_LBUPDATESOURCE_REGISTERS 1

// Texture unit YUV mode
#define PM_YUVMODE_CHROMATEST_DISABLE       0
#define PM_YUVMODE_CHROMATEST_PASSWITHIN    1
#define PM_YUVMODE_CHROMATEST_FAILWITHIN    2
#define PM_YUVMODE_TESTDATA_INPUT   0
#define PM_YUVMODE_TESTDATA_OUTPUT  1


static __inline int log2(int s)
{
    int d = 1, iter = -1;
    do {
         d *= 2;
         iter++;
    } while (d <= s);
    return iter;
}

#endif /* #ifndef _DLL_P2DEF_H_ */

