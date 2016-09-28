/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
*                                                                           *
****************************************************************************/

/***	ERROR.H - DOS Error Codes
 *
 *    The DOS 2.0 and above "XENIX-style" calls return error codes
 *    through AX.  If an error occurred then the carry bit will be set
 *    and the error code is in AX.  If no error occurred then the
 *    carry bit is reset and AX contains returned info.
 *
 *    Since the set of error codes is being extended as we extend the operating
 *    system, we have provided a means for applications to ask the system for a
 *    recommended course of action when they receive an error.
 *
 *    The GetExtendedError system call returns a universal error, an error
 *    location and a recommended course of action.  The universal error code is
 *    a symptom of the error REGARDLESS of the context in which GetExtendedError
 *    is issued.
 */


/*
 *	2.0 error codes
 */

#define	NO_ERROR			0	/* Success */
#define	ERROR_INVALID_FUNCTION		1
#define	ERROR_FILE_NOT_FOUND		2
#define	ERROR_PATH_NOT_FOUND		3
#define	ERROR_TOO_MANY_OPEN_FILES	4
#define	ERROR_ACCESS_DENIED		5
#define	ERROR_INVALID_HANDLE		6
#define	ERROR_ARENA_TRASHED		7
#define	ERROR_NOT_ENOUGH_MEMORY		8
#define	ERROR_INVALID_BLOCK		9
#define	ERROR_BAD_ENVIRONMENT		10
#define	ERROR_BAD_FORMAT		11
#define	ERROR_INVALID_ACCESS		12
#define	ERROR_INVALID_DATA		13
/***** reserved			EQU	14	; *****/
#define	ERROR_INVALID_DRIVE		15
#define	ERROR_CURRENT_DIRECTORY		16
#define	ERROR_NOT_SAME_DEVICE		17
#define	ERROR_NO_MORE_FILES		18

/*
 *	These are the universal int 24 mappings for
 *	the old INT 24 set of errors.
 */

#define	ERROR_WRITE_PROTECT		19
#define	ERROR_BAD_UNIT			20
#define	ERROR_NOT_READY			21
#define	ERROR_BAD_COMMAND		22
#define	ERROR_CRC			23
#define	ERROR_BAD_LENGTH		24
#define	ERROR_SEEK			25
#define	ERROR_NOT_DOS_DISK		26
#define	ERROR_SECTOR_NOT_FOUND		27
#define	ERROR_OUT_OF_PAPER		28
#define	ERROR_WRITE_FAULT		29
#define	ERROR_READ_FAULT		30
#define	ERROR_GEN_FAILURE		31

/*
 *	The 3.0 and 4.0 error codes reported through INT 24.
 */

#define	ERROR_SHARING_VIOLATION		32
#define	ERROR_LOCK_VIOLATION		33
#define	ERROR_WRONG_DISK		34
#define	ERROR_FCB_UNAVAILABLE		35
#define	ERROR_SHARING_BUFFER_EXCEEDED	36
#define	ERROR_CODE_PAGE_MISMATCHED	37    /* DOS 4.00 */
#define	ERROR_HANDLE_EOF		38    /* DOS 4.00 */
#define	ERROR_HANDLE_DISK_FULL		39    /* DOS 4.00 */

/*
 *	OEM network-related errors are 50-79.
 */

#define	ERROR_NOT_SUPPORTED		50

#define	ERROR_NET_ACCESS_DENIED		65
#define ERROR_BAD_NET_NAME		67 /* Network name not found */

/*
 *	End of INT 24 reportable errors
 */


#define	ERROR_FILE_EXISTS		80
#define	ERROR_DUP_FCB			81
#define	ERROR_CANNOT_MAKE		82
#define	ERROR_FAIL_I24			83

/*
 *	3.0 and 4.0 network related error codes.
 */

#define	ERROR_OUT_OF_STRUCTURES		84
#define	ERROR_ALREADY_ASSIGNED		85
#define	ERROR_INVALID_PASSWORD		86
#define	ERROR_INVALID_PARAMETER		87
#define	ERROR_NET_WRITE_FAULT		88
#define	ERROR_SYS_COMP_NOT_LOADED	90	/* DOS 4.00 */


/*
 *	OS/2-compatible error codes.
 *	NOTE: values in the range 100 to 599 may be defined
 *	by OS/2.  Do NOT add NEW error codes in this range.
 */

#define ERROR_BUFFER_OVERFLOW		111	/* Buffer too small for data */
#define ERROR_CALL_NOT_IMPLEMENTED	120	/* Returned from stub apis. */
#define ERROR_BAD_PIPE			230	/* No such pipe or bad oper. */
#define ERROR_PIPE_BUSY			231	/* Pipe is busy */
#define ERROR_PIPE_NOT_CONNECTED	233	/* Server disconnected pipe */
#define ERROR_MORE_DATA 		234	/* More data is available */


/*
 *	DOS 6.0 error codes.
 */

#define	ERROR_IO_PENDING		600	/* BUGBUG: NT value? */
#define	ERROR_TIMEOUT			601	/* BUGBUG: NT value? */
#define	ERROR_PIPE_CLOSED		602	/* BUGBUG: NT value? */
#define	ERROR_PIPE_LISTENING		603	/* BUGBUG: NT value? */
#define	ERROR_TOO_MANY_INSTANCES	604	/* BUGBUG: NT value? */
#define	ERROR_INVALID_PROVIDER		605
#define	ERROR_VOLUME_EXISTS		606	/* MOUNT Volume already exists */
#define	ERROR_VOLUME_HARD_ERROR		607	/* MOUNT Hard error reading volume */
#define	ERROR_VOLUME_UNRECOGNIZED	608	/* MOUNT Fsd doesn't recognize media file system */


/*
 *	Int24 Error Codes
 */

#define	ERROR_I24_WRITE_PROTECT		0
#define	ERROR_I24_BAD_UNIT		1
#define	ERROR_I24_NOT_READY		2
#define	ERROR_I24_BAD_COMMAND		3
#define	ERROR_I24_CRC			4
#define	ERROR_I24_BAD_LENGTH		5
#define	ERROR_I24_SEEK			6
#define	ERROR_I24_NOT_DOS_DISK		7
#define	ERROR_I24_SECTOR_NOT_FOUND	8
#define	ERROR_I24_OUT_OF_PAPER		9
#define	ERROR_I24_WRITE_FAULT		0x0A
#define	ERROR_I24_READ_FAULT		0x0B
#define	ERROR_I24_GEN_FAILURE		0x0C
/* NOTE: Code 0DH is used by MT-DOS. */
#define	ERROR_I24_WRONG_DISK		0x0F


/*
 *	THE FOLLOWING ARE MASKS FOR THE AH REGISTER ON Int 24
 *
 *	NOTE: ABORT is ALWAYS allowed
 */

#define	ALLOWED_FAIL			0x08
#define	ALLOWED_RETRY			0x10
#define	ALLOWED_IGNORE			0x20

#define	I24_OPERATION			0x01	/* Z if READ,NZ if Write */
#define	I24_AREA			0x06	/* 0x00 if DOS
						 * 0x02 if FAT
						 * 0x04 if root DIR
						 * 0x06 if DATA
						 */
#define	I24_CLASS			0x80	/* Z = DISK, NZ = FAT or char */


/***	GetExtendedError CLASSes ACTIONs LOCUSs
 *
 *	The GetExtendedError call takes an error code and returns CLASS,
 *	ACTION and LOCUS codes to help programs determine the proper action
 *	to take for error codes that they don't explicitly understand.
 */


/*
 *	Values for error CLASS.
 */

#define	ERRCLASS_OUTRES			1	/* Out of Resource */
#define	ERRCLASS_TEMPSIT		2	/* Temporary Situation */
#define	ERRCLASS_AUTH			3	/* Permission problem */
#define	ERRCLASS_INTRN			4	/* Internal System Error */
#define	ERRCLASS_HRDFAIL		5	/* Hardware Failure */
#define	ERRCLASS_SYSFAIL		6	/* System Failure */
#define	ERRCLASS_APPERR			7	/* Application Error */
#define	ERRCLASS_NOTFND			8	/* Not Found */
#define	ERRCLASS_BADFMT			9	/* Bad Format */
#define	ERRCLASS_LOCKED			10	/* Locked */
#define	ERRCLASS_MEDIA			11	/* Media Failure */
#define	ERRCLASS_ALREADY		12	/* Collision with Existing Item */
#define	ERRCLASS_UNK			13	/* Unknown/other */

/*
 *	Values for error ACTION.
 */

#define	ERRACT_RETRY			1	/* Retry */
#define	ERRACT_DLYRET			2	/* Retry after delay */
#define	ERRACT_USER			3	/* Ask user to regive info */
#define	ERRACT_ABORT			4	/* abort with clean up */
#define	ERRACT_PANIC			5	/* abort immediately */
#define	ERRACT_IGNORE			6	/* ignore */
#define	ERRACT_INTRET			7	/* Retry after User Intervention */

/*
 *	Values for error LOCUS.
 */

#define	ERRLOC_UNK			1	/* No appropriate value */
#define	ERRLOC_DISK			2	/* Random Access Mass Storage */
#define	ERRLOC_NET			3	/* Network */
#define	ERRLOC_SERDEV			4	/* Serial Device */
#define	ERRLOC_MEM			5	/* Memory */
