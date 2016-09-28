/*
 *  verinfo.h - internal header file to define the build version
 *
 */

//
//  WARNING! Do not put LEADING ZERO's on these numbers or
//  they will end up as OCTAL numbers in the C code!
//


#define  MMVERSION		3
#define  MMREVISION		5
#define  MMRELEASE		00

#ifdef RC_INVOKED
#define VERSIONPRODUCTNAME	"Audio Compression Manager Sample\0"
#define VERSIONCOPYRIGHT	"Copyright \251 Microsoft Corp. 1992-1996\0"
#endif



#define VERSIONSTR		"3.50\0"


#define VERSIONCOMPANYNAME	"Company\0"

#define VER_PRIVATEBUILD	0

#define VER_PRERELEASE		0

#define VER_DEBUG		0

#define VERSIONFLAGS		(VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)
#define VERSIONFILEFLAGSMASK	0x0030003FL

