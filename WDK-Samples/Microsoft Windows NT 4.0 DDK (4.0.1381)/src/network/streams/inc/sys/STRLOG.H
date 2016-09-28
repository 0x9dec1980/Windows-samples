/* @(#)strlog.h	1.2 (Spider) 90/11/23 */

#ifndef _SYS_STRLOG_
#define _SYS_STRLOG_

/*
 *  Log message flags
 */

#define SL_FATAL	0x01    /* indicates fatal error */
#define SL_NOTIFY	0x02    /* logger must notify administrator */
#define SL_ERROR        0x04    /* include on the error log */
#define SL_TRACE        0x08    /* include on the trace log */
#define SL_CONSOLE      0x10    /* log message to console */
#define SL_WARN         0x20    /* warning message */
#define SL_NOTE         0x40    /* notice message */

#endif /* _SYS_STRLOG_ */
