/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 1992  Microsoft Corporation

Module Name:

    soundsys.h

Abstract:

    This include file defines constants and types for
    the Microsoft sound system card.

	This header file is shared between the low level driver and the
	kernel driver.

Author:

    Robin Speed (RobinSp) 20-Oct-92

Revision History:

--*/

#define VALID_IO_PORTS {0x530, 0x604, 0xE80, 0xF40, 0xFFFF}
#define VALID_INTERRUPTS {7, 9, 10, 11, 0xFFFF}
#define VALID_DMA_CHANNELS {0, 1, 3, 0xFFFF}

#define SOUND_DEF_DMACHANNEL 1        // DMA channel no
#define SOUND_DEF_INT        11
#define SOUND_DEF_PORT       0x530

#define SOUND_REG_SINGLEMODEDMA L"Single Mode DMA"

#define SOUNDSYS_BUFFERSIZE 0x4000    // DMA buffer size - 16K

/*
** CAPS strings
*/

#define STR_DRIVERWAVEIN L"Windows Sound System Record"
#define STR_DRIVERWAVEOUT L"Windows Sound System Playback"
#define STR_DRIVERMIDIOUT L"Windows Sound System MIDI"
#define STR_DRIVERMIC L"Windows Sound System Mic"
#define STR_DRIVERLINEIN L"Windows Sound System Line In"
#define STR_DRIVERMASTERVOLUME L"Windows Sound System Master"


#define IDS_WAVEOUT_PNAME                          101
#define IDS_WAVEIN_PNAME                           102
#define IDS_MIDIOUT_PNAME                          103
#define IDS_AUX_PNAME                              104

#define IDS_DESTLINEOUT_SHORT_NAME                 110
#define IDS_DESTLINEOUT_LONG_NAME                  111
#define IDS_DESTWAVEIN_SHORT_NAME                  112
#define IDS_DESTWAVEIN_LONG_NAME                   113
#define IDS_DESTVOICEIN_SHORT_NAME                 114
#define IDS_DESTVOICEIN_LONG_NAME                  115

#define IDS_SRCAUX1_SHORT_NAME                     120
#define IDS_SRCAUX1_LONG_NAME                      121
#define IDS_SRCWAVEOUT_SHORT_NAME                  122
#define IDS_SRCWAVEOUT_LONG_NAME                   123
#define IDS_SRCMIDIOUT_SHORT_NAME                  124
#define IDS_SRCMIDIOUT_LONG_NAME                   125
#define IDS_SRCMIC_SHORT_NAME                      126
#define IDS_SRCMIC_LONG_NAME                       127

#define IDS_CONTROL_VOLLINEOUT_SHORT_NAME          130
#define IDS_CONTROL_VOLLINEOUT_LONG_NAME           131
#define IDS_CONTROL_MUTELINEOUT_SHORT_NAME         132
#define IDS_CONTROL_MUTELINEOUT_LONG_NAME          133
#define IDS_CONTROL_MUXWAVEIN_SHORT_NAME           134
#define IDS_CONTROL_MUXWAVEIN_LONG_NAME            135
#define IDS_CONTROL_MUXVOICEIN_SHORT_NAME          136
#define IDS_CONTROL_MUXVOICEIN_LONG_NAME           137
#define IDS_CONTROL_VOLLINEOUTAUX1_SHORT_NAME      138
#define IDS_CONTROL_VOLLINEOUTAUX1_LONG_NAME       139
#define IDS_CONTROL_MUTELINEOUTAUX1_SHORT_NAME     140
#define IDS_CONTROL_MUTELINEOUTAUX1_LONG_NAME      141
#define IDS_CONTROL_VOLLINEOUTWAVEOUT_SHORT_NAME   142
#define IDS_CONTROL_VOLLINEOUTWAVEOUT_LONG_NAME    143
#define IDS_CONTROL_MUTELINEOUTWAVEOUT_SHORT_NAME  144
#define IDS_CONTROL_MUTELINEOUTWAVEOUT_LONG_NAME   145
#define IDS_CONTROL_PEAKLINEOUTWAVEOUT_SHORT_NAME  146
#define IDS_CONTROL_PEAKLINEOUTWAVEOUT_LONG_NAME   147
#define IDS_CONTROL_VOLLINEOUTMIDIOUT_SHORT_NAME   148
#define IDS_CONTROL_VOLLINEOUTMIDIOUT_LONG_NAME    149
#define IDS_CONTROL_MUTELINEOUTMIDIOUT_SHORT_NAME  150
#define IDS_CONTROL_MUTELINEOUTMIDIOUT_LONG_NAME   151
#define IDS_CONTROL_VOLWAVEINAUX1_SHORT_NAME       152
#define IDS_CONTROL_VOLWAVEINAUX1_LONG_NAME        153
#define IDS_CONTROL_PEAKWAVEINAUX1_SHORT_NAME      154
#define IDS_CONTROL_PEAKWAVEINUAX1_LONG_NAME       155
#define IDS_CONTROL_VOLWAVEINMIC_SHORT_NAME        156
#define IDS_CONTROL_VOLWAVEINMIC_LONG_NAME         157
#define IDS_CONTROL_PEAKWAVEINMIC_SHORT_NAME       158
#define IDS_CONTROL_PEAKWAVEINMIC_LONG_NAME        159
#define IDS_CONTROL_VOLVOICEINAUX1_SHORT_NAME      160
#define IDS_CONTROL_VOLVOICEINAUX1_LONG_NAME       161
#define IDS_CONTROL_PEAKVOICEINAUX1_SHORT_NAME     162
#define IDS_CONTROL_PEAKVOICEINUAX1_LONG_NAME      163
#define IDS_CONTROL_VOLVOICEINMIC_SHORT_NAME       164
#define IDS_CONTROL_VOLVOICEINMIC_LONG_NAME        165
#define IDS_CONTROL_PEAKVOICEINMIC_SHORT_NAME      166
#define IDS_CONTROL_PEAKVOICEINMIC_LONG_NAME       167

#define SR_STR_DRIVER_MIXER                        180

