/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

   ld.h

Abstract:

   This include file contains the definitions for the drive lock demo
   application, LOCKDEMO.EXE.

Author:

    original rb code by Len Smale

Environment:

    See LOCKDEMO.WRI

Revision History:
    4/19/2000 Modifications to add use of drive locking mechanisms by
	Kevin Burrows and Mark Amos. File names changed to reflect new purpose

--*/

#define MAX_DRIVE_NUMBER 25
#define TESTAPPNAME "LOCKCD.EXE"
#define TESTVXDNAME "LDVXD.VXD"
#define STRVAL(v) STRVALX(v)
#define STRVALX(v) #v

DWORD GetCDRomDrive(VOID);
DWORD GetDeviceStatus(DWORD Volume, PCDROM_DEVSTAT pDevStat);
BOOL  LockDrive(PCDROM_LOCK_DRIVE pCommand);
BOOL  UnlockDrive(PCDROM_LOCK_DRIVE pCommand);
BOOL  IsDriveLocked(PCDROM_LOCK_DRIVE pCommand);
BOOL  RefreshLock(PCDROM_LOCK_DRIVE pCommand);
BOOL  LoadVxD();
void  UnLoadVxD();
DWORD GetVxDVersion();

// Strings
#define LDSTRING(n, s)  const TCHAR n[] = s

LDSTRING(szInProgress, "Test %d in progress\r");
LDSTRING(szEmpty, "                                            ");
LDSTRING(szCarriageReturn, "\r");
LDSTRING(szCtrlC, "^C");
LDSTRING(szProcessTerminated, "Process terminated");
LDSTRING(szTestFailed, "Test Failed: ");
LDSTRING(szTestsAborted, "Tests Aborted: ");
LDSTRING(szDoorLocked, "Door locked");
LDSTRING(szDoorOpen, "Door open");
LDSTRING(szNoDisk, "No disk in drive");
LDSTRING(szPlayingAudio, "Playing audio");
LDSTRING(szNoAudio, "Does not play audio");
LDSTRING(szRawReading, "Raw reading supported");
LDSTRING(szNoRawReading, "Raw reading not supported");
LDSTRING(szNoChannelManipulation, "No audio channel manipulation");
LDSTRING(szRedbookAddressing, "Redbook addressing supported");
LDSTRING(szNoRedbookAddressing, "Redbook addressing not supported");
LDSTRING(szCDXA, "CD-XA supported");
LDSTRING(szNoCDXA, "CD-XA not supported");
LDSTRING(szRWSubchannels, "R-W subchannels supported");
LDSTRING(szNoRWSubchannels, "R-W subchannels not supported");
LDSTRING(szWriteableDrive, "Writeable drive");
LDSTRING(szISO9660, "ISO-9660 interleaving supported");
LDSTRING(szNoISO9660, "ISO-9660 interleaving not supported");
LDSTRING(szSpeedAdjustable, "Speed adjustable");
LDSTRING(szSpeedNotAdjustable, "Speed not adjustable");
LDSTRING(sz1xData, "Set to 1x data rate");
LDSTRING(szPrefetching, "Prefetching supported");
LDSTRING(szNoPrefetching, "No prefetching");
LDSTRING(szfDrive, "Drive: %s");
LDSTRING(szfTrack, "Track: %d");
LDSTRING(szfNoSectors, "Number of sectors: %d");
LDSTRING(szfNoBuffers, "Number of buffers: %d");
LDSTRING(szfInitialReadAhead, "Initial read-ahead before start playing: %d");
LDSTRING(szfCorrespondingWAVFile, "Corresponding WAV file: %s");
LDSTRING(szNotCDROMDrive, "Drive is not a CD-ROM drive");
LDSTRING(sz1xTest, "Testing at 1x speed");
LDSTRING(szFullSpeedTest, "Testing at full speed");
LDSTRING(szBadFirstParameter, "First parameter must be set to determine location running from");
LDSTRING(szNoCDROMDrive, "No CD-ROM drive to test");
LDSTRING(szNoWindowsDirectory, "Unable to obtain name of Windows directory");
LDSTRING(szWindowsNameTooLong, "Windows directory name too long");
// Help text
/*
LDSTRING(szTestSummary,         "rbtest - tests a CD-ROM drive to see if audio tracks can be read digitally.");
LDSTRING(szUsageSummary,        "Usage: rbtest [-?] [-d:CDROM-drive] [-t:track] [-s:number-of-sectors]");
LDSTRING(szUsageSummary1,       "              [-w:WAV-file] [-r:test-repeat-count] [-b:number-of-buffers]");
LDSTRING(szUsageSummary2,       "              [-o:output-file] [-a:read-ahead-buffers] [-i] [-p]");
LDSTRING(szArguments,           "Arguments:");
LDSTRING(szReadAheadParameter,  "-a   (read-ahead) The number of buffers read ahead when reading data.");
LDSTRING(szReadAheadUsage,      "                  This is only used for 1x read tests.");
LDSTRING(szReadAheadValidation, "                  Must be less than (buffers).");
LDSTRING(szReadAheadDefault,    "                  Defaults to "STRVAL(NREADAHEAD)".");
LDSTRING(szBuffersParameter,    "-b   (buffers)    The number of buffers allocated when reading data.");
LDSTRING(szBuffersUsage,        "                  This is only used for 1x read tests.");
LDSTRING(szBuffersDefault,      "                  Defaults to "STRVAL(NPACEBUFFERS)".");
LDSTRING(szDriveParameter,      "-d   (drive)      The CD-ROM drive to be tested.");
LDSTRING(szDriveDefault,        "                  Defaults to the first CD-ROM drive in the system.");
LDSTRING(szOutputParameter,     "-o   (output)     The output file for test results.");
LDSTRING(szRepeatParameter,     "-r   (repeat)     The number of times to repeat the test.");
LDSTRING(szRepeatInfo,          "                  Currently only the accurate read tests are repeated.");

LDSTRING(szRepeatDefault,       "                  Defaults to "STRVAL(NREPEATS)".");
LDSTRING(szSectorsParameter,    "-s   (sectors)    The number of sectors to test from the audio track.");
LDSTRING(szSectorsInfo1,        "                  Each sector has 2352 bytes, and all tests start at");
LDSTRING(szSectorsInfo2,        "                  the first sector of the track.");
LDSTRING(szSectorsDefault,      "                  Defaults to "STRVAL(NSECTORS)".");
LDSTRING(szTrackParameter,      "-t   (track)      The audio track number to test on the CD-ROM disk.");
LDSTRING(szTrackDefault,        "                  Defaults to "STRVAL(NTRACK)".");
LDSTRING(szWAVParameter,        "-w   (WAV)        The WAV file to compare the data read from the track.");
LDSTRING(szOptions,             "Options:");
LDSTRING(szIgnoreOption,        "-i   (ignore)     Continue testing when errors are encountered.");
LDSTRING(szIgnoreInfo,          "                  Normally stops tests when a test fails.");
LDSTRING(szPauseOption,         "-p   (pause)      Pause before exiting.");
LDSTRING(szPauseDefault,        "                  Defaults to no pause.");
LDSTRING(szFastTestOption,      "-f   (fast-test)  Only perform the full speed read test.");
LDSTRING(szFastTestInfo1,       "                  Used to further test CD-ROM drives that fail the 1x test.");
LDSTRING(szFastTestInfo2,       "                  May be combined with the quick-test.");
LDSTRING(szQuickTestOption,     "-q   (quick-test) Only perform the 1x read test.");
LDSTRING(szQuickTestInfo,       "                  Used to find short term CD-ROM startup problems.");
LDSTRING(szQuickTestDefault,    "                  Defaults to one test run (repeat set to 1).");
*/
// Errors
LDSTRING(szNoPerformanceTimer, "The system does not have a performance timer, timings are not accurate.");


struct DeviceProps {
    DWORD   Mask;
    PCSTR   SetText;
    PCSTR   ClearText;
} aDeviceProp[] = {
    CDDEVSTAT_DOOR_OPEN,            szDoorOpen,             NULL,
    CDDEVSTAT_NO_DISK_IN_DRIVE,     szNoDisk,               NULL,
    CDDEVSTAT_DOOR_UNLOCKED,        NULL,                   szDoorLocked,
    CDDEVSTAT_PLAYING_AUDIO,        szPlayingAudio,         NULL,
    CDDEVSTAT_PLAY_AUDIO_TOO,       NULL,                   szNoAudio,
    CDDEVSTAT_READ_RAW_TOO,         szRawReading,           szNoRawReading,
    CDDEVSTAT_AUDIO_MANIPULATE,     NULL,                   szNoChannelManipulation,
    CDDEVSTAT_REDBOOK_TOO,          szRedbookAddressing,    szNoRedbookAddressing,
    CDDEVSTAT_CDXA,                 szCDXA,                 szNoCDXA,
    CDDEVSTAT_RW_CHANNELS_OK,       szRWSubchannels,        szNoRWSubchannels,
    CDDEVSTAT_WRITE_TOO,            szWriteableDrive,       NULL,
    CDDEVSTAT_INTERLEAVE,           szISO9660,              szNoISO9660,
    CDDEVSTAT_SPEED_ADJUSTABLE,     szSpeedAdjustable,      szSpeedNotAdjustable,
    CDDEVSTAT_SPEED_NONSTANDARD,    NULL,                   sz1xData,
    CDDEVSTAT_PREFETCHING,          szPrefetching,          szNoPrefetching,
    CDDEVSTAT_RESERVED_13,          NULL,                   NULL,
    CDDEVSTAT_RESERVED_6,           NULL,                   NULL,
    0,                              NULL,                   NULL
};


