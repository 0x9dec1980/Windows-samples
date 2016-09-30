/***************************************************************************/
/*  General Definitions                                                    */
/***************************************************************************/

// The following values define valid options for the TimerNumber parameter
// that is used in all of the functions in which only a single timer may be
// affected. TimerNumber provides the number of the target timer.
//
// NOTE : These definitions for Timers 1 - 5 simply reflect a value of 1 - 5
// and, consequently, can be used in loop operations, etc.
// i.e., for (timer = TN_T1;timer <= TN_T5;timer++) { ...

#define TN_T5           0x0005          // TimerNumber = Timer 5
#define TN_T4           0x0004          // TimerNumber = Timer 4
#define TN_T3           0x0003          // TimerNumber = Timer 3
#define TN_T2           0x0002          // TimerNumber = Timer 2
#define TN_T1           0x0001          // TimerNumber = Timer 1

// The following values define valid options for the TimerFlags parameter.
// TimerFlags provide bit masks that represent the target timers.
//
// NOTE : These values may be logically OR'ed together to represent more
// than one target timer, i.e., ArmTimers(TF_T1 | TF_T2);

#define TF_ALL          0x001f          // TimerFlag = All timers
#define TF_T5           0x0010          // TimerFlag = Timer 5
#define TF_T4           0x0008          // TimerFlag = Timer 4
#define TF_T3           0x0004          // TimerFlag = Timer 3
#define TF_T2           0x0002          // TimerFlag = Timer 2
#define TF_T1           0x0001          // TimerFlag = Timer 1

/***************************************************************************/
/* Master Mode Register Definitions                                        */
/***************************************************************************/

// The following values represent field masks and field options for fields
// in the Master Mode Register. These fields may be accessed indirectly
// through library functions or directly in the Master Mode Register.

#define FREQ_SCALE      0x8000          // FreqScale mask
#define FS_16           0x0000          // FreqScale = By 16 (binary)
#define FS_10           0x8000          // FreqScale = By 10 (BCD)

#define PTR_INIT        0x4000          // Pointer initialized mask

#define EN_F6           0x1000          // EnF6 mask
#define F6_ON           0x0000          // F6 enabled
#define F6_OFF          0x1000          // F6 disabled

#define F6_SCALE        0x0f00          // F6Scale mask
#define F6_16           0x0000          // F6Scale = factor by 16
#define F6_15           0x0f00          // F6Scale = factor by 15
#define F6_14           0x0e00          // F6Scale = factor by 14
#define F6_13           0x0d00          // F6Scale = factor by 13
#define F6_12           0x0c00          // F6Scale = factor by 12
#define F6_11           0x0b00          // F6Scale = factor by 11
#define F6_10           0x0a00          // F6Scale = factor by 10
#define F6_9            0x0900          // F6Scale = factor by 9
#define F6_8            0x0800          // F6Scale = factor by 8
#define F6_7            0x0700          // F6Scale = factor by 7
#define F6_6            0x0600          // F6Scale = factor by 6
#define F6_5            0x0500          // F6Scale = factor by 5
#define F6_4            0x0400          // F6Scale = factor by 4
#define F6_3            0x0300          // F6Scale = factor by 3
#define F6_2            0x0200          // F6Scale = factor by 2
#define F6_1            0x0100          // F6Scale = factor by 1

#define F6_SOURCE       0x00f0          // F6Source mask
#define F6_F5           0x00f0          // F6Source = F5
#define F6_F4           0x00e0          // F6Source = F4
#define F6_F3           0x00d0          // F6Source = F3
#define F6_F2           0x00c0          // F6Source = F2
#define F6_F1           0x00b0          // F6Source = F1

#define EN_ALARM2       0x0008          // Enable Timer 2 Alarm mask
#define EN_ALARM1       0x0004          // Enable Timer 1 Alarm mask

/***************************************************************************/
/* Timer Mode Register Definitions                                         */
/***************************************************************************/

// The following values represent field masks and field options for fields
// in the Timer Mode Registers. These fields may be accessed indirectly
// through library functions or directly in the Timer Mode Registers.

#define TIMER_SOURCE    0x0f00          // TimerSource mask
#define TS_F6           0x0100          // TimerSource = F6
#define TS_F5           0x0f00          // TimerSource = F5
#define TS_F4           0x0e00          // TimerSource = F4
#define TS_F3           0x0d00          // TimerSource = F3
#define TS_F2           0x0c00          // TimerSource = F2
#define TS_F1           0x0b00          // TimerSource = F1
#define TS_T5           0x0a00          // TimerSource = Timer 5 Output
#define TS_T4           0x0900          // TimerSource = Timer 4 Output
#define TS_T3           0x0800          // TimerSource = Timer 3 Output
#define TS_T2           0x0700          // TimerSource = Timer 2 Output
#define TS_T1           0x0600          // TimerSource = Timer 1 Output
#define TS_TNM1         0x0000          // TimerSource = Timer (n-1)

#define TIMER_CYCLE     0x0020          // TimerCycle mask
#define TC_REPEAT       0x0020          // TimerCycle = Repeat
#define TC_ONCE         0x0000          // TimerCycle = Once

#define TIMER_BASE      0x0010          // TimerBase mask
#define TB_BCD          0x0010          // TimerBase = BCD
#define TB_BIN          0x0000          // TimerBase = Binary

#define TIMER_DIRECTION 0x0008          // TimerDirection mask
#define TD_UP           0x0008          // TimerDirection = Up
#define TD_DOWN         0x0000          // TimerDirection = Down

#define TIMER_OUT_MODE  0x0007          // TimerOutputMode mask
#define TM_TOGGLE       0x0002          // TimerMode = Toggle
#define TM_PULSE        0x0001          // TimerMode = Pulse
#define TM_OFF          0x0000          // TimerMode = Off

/***************************************************************************/
/* Interrupt Register Definitions                                          */
/***************************************************************************/

// The following values represent field masks and field options for fields
// in the Interrupt Register. These fields may be accessed indirectly
// through library functions or directly in the Interrupt Register.

// EnNmi mask and options

#define EN_NMI          0x80            // EnNmi mask
#define EN_ON           0x00            // EnNmi = On
#define EN_OFF          0x80            // EnNmi = Off

// EnInt mask and options

#define EN_INT          0x40            // EnInt mask
#define EI_ON           0x40            // EnInt = On
#define EI_OFF          0x00            // EnInt = Off

// NMI Status Mask

#define NMI_STATUS      0x20            // NMI Status mask (READ-ONLY)

// Interrupt Status Mask

#define INT_STATUS      0x10            // Int Status mask (READ-ONLY)

// Interrupt Source mask and options

#define INT_SOURCE      0x07            // IntSource mask
#define IS_ON           0x07            // IntSource = On
#define IS_SW           0x06            // IntSource = Breakout Switch
#define IS_NMI          0x06            // IntSource = Breakout Switch
#define IS_T5           0x05            // IntSource = Timer 5
#define IS_T4           0x04            // IntSource = Timer 4
#define IS_T3           0x03            // IntSource = Timer 3
#define IS_T2           0x02            // IntSource = Timer 2
#define IS_T1           0x01            // IntSource = Timer 1
#define IS_OFF          0x00            // IntSource = Off

/***************************************************************************/
/* Command Register Definitions (WRITE-ONLY)                               */
/***************************************************************************/

// The following commands affect multiple timers. The target timers are
// specified by logically OR'ing the appropriate TimerFlag values with the
// command byte. These commands may be executed indirectly through equivalent
// library functions or directly by writing the appropriate value to the
// Command Register.

#define LOAD            0x40            // Load timers command
#define ARM             0x20            // Start timers command
#define DISARM          0xc0            // Stop timers command
#define SAVE            0xa0            // Save timers command

// The following commands affect a single timer. The target timer is
// specified by logically OR'ing the appropriate TimerNumber value with the
// command byte. These commands may be executed indirectly through equivalent
// library functions or directly by writing the appropriate value to the
// Command Register.

#define SET_OUT         0xe8            // Set timer output command
#define CLEAR_OUT       0xe0            // Clear timer output command
#define STEP            0xf0            // Step timer commmand

// The following commands provide general control over STAT! resources and
// do not affect any timers directly. These commands may be executed
// indirectly through equivalent library functions or directly by writing
// the appropriate value to the Command Register.

#define RESET           0xff            // Master reset command
#define INIT_PTR        0xe8            // Initialize pointer command
#define OFF_F6          0xee            // Disable F6 command
#define ON_F6           0xe6            // Enable F6 command

// The Load Pointer Command provides a pointer mechanism to access registers
// through the Data Port. The target register is specified by logically
// OR'ing one Group value and one Element value with the Load Pointer command
// byte. The library functions do not offer an equivalent command as those
// functions manage the data pointer transparently.

#define LD_PTR          0x00            // Load Pointer command

#define CTL_GROUP       0x07            // Control Group

#define MASTER_ELEM     0x10            // Control Elements
#define ALARM1_ELEM     0x00
#define ALARM2_ELEM     0x08

#define T5_GROUP        0x05            // Timer Groups
#define T4_GROUP        0x04
#define T3_GROUP        0x03
#define T2_GROUP        0x02
#define T1_GROUP        0x01

#define MODE_ELEM       0x00            // Timer Elements
#define LOAD_ELEM       0x08
#define HOLD_ELEM       0x10

// By deriving all possible Load Pointer command Group and Element values,
// the following equivalent Load Pointer command possibilities are defined
// directly.

#define PTR_MASTER      0x17            // Point to Master Mode command

#define PTR_T5MODE      0x05            // Point to Timer 5 Mode command
#define PTR_T5LOAD      0x0d            // Point to Timer 5 Load command
#define PTR_T5HOLD      0x15            // Point to Timer 5 Hold command

#define PTR_T4MODE      0x04            // Point to Timer 4 Mode command
#define PTR_T4LOAD      0x0c            // Point to Timer 4 Load command
#define PTR_T4HOLD      0x14            // Point to Timer 4 Hold command

#define PTR_T3MODE      0x03            // Point to Timer 3 Mode command
#define PTR_T3LOAD      0x0b            // Point to Timer 3 Load command
#define PTR_T3HOLD      0x13            // Point to Timer 3 Hold command

#define PTR_T2MODE      0x02            // Point to Timer 2 Mode command
#define PTR_T2LOAD      0x0a            // Point to Timer 2 Load command
#define PTR_T2HOLD      0x12            // Point to Timer 2 Hold command
#define PTR_T2ALARM     0x1f            // Point to Timer 2 Alarm command

#define PTR_T1MODE      0x01            // Point to Timer 1 Mode command
#define PTR_T1LOAD      0x09            // Point to Timer 1 Load command
#define PTR_T1HOLD      0x11            // Point to Timer 1 Hold command
#define PTR_T1ALARM     0x0f            // Point to Timer 1 Alarm command

/***************************************************************************/
/* Status Register Definitions (READ-ONLY)                                 */
/***************************************************************************/

// The following values represent fields in the Status Register. These
// fields may be accessed indirectly through library functions or directly
// by reading the Status Register.

#define T5_STATUS       0x20            // Timer 5 Output Status
#define T4_STATUS       0x10            // Timer 4 Output Status
#define T3_STATUS       0x08            // Timer 3 Output Status
#define T2_STATUS       0x04            // Timer 2 Output Status
#define T1_STATUS       0x02            // Timer 1 Output Status

/***************************************************************************/
/*  Function-Specific Definitions                                          */
/***************************************************************************/

// The following values define the valid options for the TimerOut
// parameter of the SetTimerOut function.

#define TO_ACTIVE       0x08            // TimerOut = Active
#define TO_INACTIVE     0x00            // TimerOut = Inactive

// The following values define the valid options for the TimerAlarm
// parameter of the SetEnableAlarm function.

#define TA_ON           0x02            // TimerAlarm = On
#define TA_OFF          0x00            // TimerAlarm = Off


#define TIMER_CASCADE	0x80

//
// Structure used to pass information to/from the stat driver and apps
//
typedef struct _IOCTL_INFORMATION {
    ULONG   ElapsedTime;
    USHORT  wTimerSource;	// frequency source
    USHORT  wTimerCycle;	// once or repeat
    USHORT  wTimerBase;		// binary or BCD
    USHORT  wTimerDirection;	// up or down
    USHORT  wTimerMode;		// off, pulse, or toggle
    UCHAR   wTimerNumber;	// timer number
} IOCTL_INFORMATION;
typedef IOCTL_INFORMATION *PIOCTL_INFORMATION;
