
#include "statlib.h"

//
// port addresses (default 0x250)
//
#define DATA_PORT_ADDRESS	0x250
#define CONTROL_PORT 2
#define INTERRUPT_PORT 3

//
// IRQ (jumpered to 7)
//
#define STAT_IRQ    7

//
// IOCTL
//
// PLEASE NOTE:
// The following driver device number and IOCTL code has been chosen
// at random, and is not intended to be used as is in any real product.
// Refer to the NT DDK Kernel mode Driver Design Guide and the Kernel
// Mode Driver Reference for more information about IOCTL code formats.
// Contact Microsoft Product Support if you have questions about assigning
// your own IOCTL codes.

#define FILE_DEVICE_KRNLDRVR 0x80ff

#define IOCTL_STAT_GET_INTERRUPT_COUNT	 CTL_CODE(FILE_DEVICE_KRNLDRVR, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STAT_SET_TIMER	 CTL_CODE(FILE_DEVICE_KRNLDRVR, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STAT_GET_TIMER	 CTL_CODE(FILE_DEVICE_KRNLDRVR, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STAT_START_TIMER	 CTL_CODE(FILE_DEVICE_KRNLDRVR, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STAT_STOP_TIMER	 CTL_CODE(FILE_DEVICE_KRNLDRVR, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STAT_TEST_TIMER_KERNEL	 CTL_CODE(FILE_DEVICE_KRNLDRVR, 8,  METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// load data pointer commands
//
#define MASTER_MODE	0x17

#define TIMER_MODE_5	0x05
#define TIMER_MODE_4	0x04
#define TIMER_MODE_3	0x03
#define TIMER_MODE_2	0x02
#define TIMER_MODE_1	0x01

#define TIMER_LOAD_5	0x0D
#define TIMER_LOAD_4	0x0C
#define TIMER_LOAD_3	0x0B
#define TIMER_LOAD_2	0x0A
#define TIMER_LOAD_1	0x09

#define TIMER_HOLD_5	0x15
#define TIMER_HOLD_4	0x14
#define TIMER_HOLD_3	0x13
#define TIMER_HOLD_2	0x12
#define TIMER_HOLD_1	0x11

#define TIMER_ALARM_2	0x0F
#define TIMER_ALARM_1	0x07


typedef struct _STAT_DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;
    PKINTERRUPT	InterruptObject;
    ULONG InterruptCount;
    PUCHAR PortAddress;
    USHORT TimerMode;
    USHORT TimerValue;
    USHORT TimerLoad;
    USHORT MtrStatus;
    UCHAR OutStatus;
    UCHAR IntStatus;
    ULONG ioCtlCode;
    PIOCTL_INFORMATION IoctlInfo;

    PUCHAR testbuffer;

} STAT_DEVICE_EXTENSION;
typedef STAT_DEVICE_EXTENSION *PSTAT_DEVICE_EXTENSION;
