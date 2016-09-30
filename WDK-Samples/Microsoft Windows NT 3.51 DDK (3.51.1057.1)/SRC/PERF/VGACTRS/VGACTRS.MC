;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    vgactrs.h
;       (derived from vgactrs.mc by the message compiler  )
;
;Abstract:
;
;   Event message definititions used by routines in VGACTRS.DLL
;
;Created:
;
;    15-Oct-1992  Bob Watson (a-robw)
;
;Revision History:
;
;--*/
;//
;#ifndef _VGACTRS_H_
;#define _VGACTRS_H_
;//
MessageIdTypedef=DWORD
;//
;//     Perfutil messages
;//
MessageId=1900
Severity=Informational
Facility=Application
SymbolicName=UTIL_LOG_OPEN
Language=English
An extensible counter has opened the Event Log for VGACTRS.DLL
.
;//
MessageId=1999
Severity=Informational
Facility=Application
SymbolicName=UTIL_CLOSING_LOG
Language=English
An extensible counter has closed the Event Log for VGACTRS.DLL
.
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=VGAPERF_OPEN_FILE_MAPPING_ERROR
Language=English
Unable to open mapped file containing VGA driver performance data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=VGAPERF_UNABLE_MAP_VIEW_OF_FILE
Language=English
Unable to map to shared memory file containing VGA driver performance data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=VGAPERF_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable open "Performance" key of VGA driver in registy. Status code is returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=VGAPERF_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the "First Counter" value under the Vga\Performance Key. Status codes retuened in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=VGAPERF_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the "First Help" value under the Vga\Performance Key. Status codes retuened in data.
.
;//
;#endif // _VGACTRS_H_
