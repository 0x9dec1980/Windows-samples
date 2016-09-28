/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1993-99  Microsoft Corporation.  All Rights Reserved.       *
*                                                                           *
****************************************************************************/

#include "3dver.h"

#define OFFICIAL   1 
#define FINAL      1 

/****************************************************************************
 *                                                                          *
 *      VERSION.H        -- Version information for internal builds         *
 *                                                                          *
 *      This file is only modified by the official builder to update the         *
 *      VERSION, VER_PRODUCTVERSION and VER_PRODUCTVERSION_STR values            *
 *                                                                          *
 ****************************************************************************/

#ifndef VS_FF_DEBUG 
/* ver.h defines constants needed by the VS_VERSION_INFO structure */
#include "ver.h"
#endif 

/*--------------------------------------------------------------*/
/* the following entry should be phased out in favor of         */
/* VER_PRODUCTVERSION_STR, but is used in the shell today.      */
/*--------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* the following values should be modified by the official      */
/* builder for each build                                       */
/*--------------------------------------------------------------*/
#define VERSION                     VER_3DLABS                      
#define VER_PRODUCTVERSION_STR      VER_3DLABS_PRODUCTVERSION_STR   
#define VER_PRODUCTVERSION          VER_3DLABS_PRODUCTVERSION       
#define VER_PRODUCTVERSION_DW       VER_3DLABS_PRODUCTVERSION_DW    

/*--------------------------------------------------------------*/
/* the following section defines values used in the version     */
/* data structure for all files, and which do not change.       */
/*--------------------------------------------------------------*/

/* default is nodebug */
#ifndef DEBUG
#define VER_DEBUG                   0
#else
#define VER_DEBUG                   VS_FF_DEBUG
#endif

/* default is privatebuild */
#ifndef OFFICIAL
#define VER_PRIVATEBUILD            VS_FF_PRIVATEBUILD
#else
#define VER_PRIVATEBUILD            0
#endif

/* default is prerelease */
/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: glintver.h
 *
 *              GLINT register window definition
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/
#ifndef FINAL
#define VER_PRERELEASE              VS_FF_PRERELEASE
#else
#define VER_PRERELEASE              0
#endif

#define VER_FILEFLAGSMASK           VS_FFI_FILEFLAGSMASK
#define VER_FILEOS                  VOS_DOS_WINDOWS16
#define VER_FILEFLAGS               (VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)

#define VER_COMPANYNAME_STR         "3Dlabs Incorporated\0"

#define VER_PRODUCTNAME_STR         "3Dlabs Windows 95 Drivers\0"

#define VER_LEGALCOPYRIGHT_YEARS        "1995"
#define VER_LEGALCOPYRIGHT_STR      "Copyright \251 3Dlabs Inc. " VER_LEGALCOPYRIGHT_YEARS "\0"
#define VER_LEGALTRADEMARKS_STR     "Copyright \251 3Dlabs Inc. 1996\0"

#define VERSION_HEADER_3DLABS                                               \
VS_VERSION_INFO VERSIONINFO                                                 \
 FILEVERSION VER_3DLABS_PRODUCTVERSION                                      \
 PRODUCTVERSION VER_3DLABS_PRODUCTVERSION                                   \
 FILEFLAGSMASK 0x3fL                                                        \
 FILEFLAGS 0xaL                                                             \
 FILEOS 0x10001L                                                            \
 FILETYPE 0x2L                                                              \
 FILESUBTYPE 0x0L                                                           \
BEGIN                                                                       \
    BLOCK "StringFileInfo"                                                  \
    BEGIN                                                                   \
        BLOCK "040904e4"                                                    \
        BEGIN                                                               \
            VALUE "CompanyName", VER_COMPANYNAME_STR                        \
            VALUE "FileDescription", "Direct Draw 32 bit Component DLL\0"   \
            VALUE "OriginalFilename", "GLINTDVM.DLL\0"                      \
            VALUE "InternalName", "GLINTDVM\0"                              \
            VALUE "FileVersion", VER_PRODUCTVERSION_STR                     \
            VALUE "LegalCopyright", VER_LEGALCOPYRIGHT_STR                  \
            VALUE "ProductName", VER_PRODUCTNAME_STR                        \
            VALUE "ProductVersion", VER_PRODUCTVERSION_STR                  \
        END                                                                 \
    END                                                                     \
    BLOCK "VarFileInfo"                                                     \
    BEGIN                                                                   \
        VALUE "Translation", 0x409, 1252                                    \
    END                                                                     \
END                                                                       

