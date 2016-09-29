;   (C) Copyright MICROSOFT Corp., 1991
;***********************************************************************
;
;   itimer.inc
;
;
;   Summary:
;
;   The Virtual Interval Timer is provided in enhanced mode Windows by
;   the VITD VxD. The timer is strictly virtual in the sense that there
;   is no real hardware directly associated with ports used in accessing
;   the timer. In other words, this timer is simulated in software. The
;   following comments will describe how to use the timer, and what
;   its characteristics are.
;
;
;   Basically, the VIT enables programs to measure elapsed CPU time in
;   a virtual machine. This timer DOES NOT count down in real time. For
;   every millesecond that a virtual machine is allowed to run, this 
;   timer is decremented by one. When the value of the timer is reaches
;   zero, an interrupt will be posted to the virtual machine.
;
;   VIT Registers:
;
;   I/O address                 Description
;   -----------                 -----------
;       30h                 --- VIT Count Register. This is a 32 bit
;                               register that contains the number of
;                               milleseconds remaining before a VIT
;                               interrupt occurs. The count register
;                               will be decremented only when the timer
;                               is running, as defined by the state
;                               register. Reading or writing this 
;                               register is done bytewise, from the
;                               least significant byte to the most
;                               significant.                     
;
;       31h                 --- VIT State Register. The following bits
;                               are defined in this register:
;               defined bits
;               ------------
;               00000001b       ITIMER_RUNNING: This bit indicates that
;                               the timer is currently running. When
;                               writing to the state register, this bit
;                               is used to start or stop the timer.
;                               If this bit is set, the count register
;                               and all other bits in the state register
;                               are read/only.
;
;               00000010b       ITIMER_ENHANCED: This bit can be used
;                               in conjunction with the ITIMER_RUNNING
;                               bit to start an interval in which the
;                               system timer is running at a much higher
;                               accuracy.
;
;               00000100b       ITIMER_NO_RUPT: Setting this bit in the
;                               state register specifies that the timer
;                               should not reflect a timer interrupt when
;                               the count reaches zero.
;
;                               !!WARNING!! - Fast timer interrupt periods
;                               can be very, very expensive in terms of
;                               total system performance. For example,
;                               on some machines a timer interrupt of
;                               1 millesecond will degrade total machine
;                               throughput by 10 percent and disk I/O by
;                               up to 50 percent.
;
;               10000000b       ITIMER_ERROR: An error was encountered
;                               in setting the interval timer. Unexpected
;                               results may occur.
;
;
;   VIT Interrupt:
;
;   When the timer is running, and ITIMER_NO_RUPT has not been specified,
;   an interrupt will be reflected into the virtual machine after the
;   count register decrements to zero. This interrupt is made on vector
;   66h. 
;
;
;   Virtual Interval Timer Operation:
;
;   When the interval timer count has been programmed, and the timer
;   has been set to the "running" state, the interval count will be
;   decremented by 1 for each millesecond that the virtual machine is
;   scheduled and run. So, this count will indicate VM execution time,
;   not real time. When the count reaches zero and an interrupt has
;   been requested, then an interrupt will be posted to the virtual
;   machine on vector 66h. 
;
;   Note that the interval timer may in fact decrement beyond zero. This
;   depends on the accuracy of the hardware system timer, and also on
;   the state of the system at the time of the "timeout". Also, the
;   interrupt may not be reflected immediately to the virtual machine,
;   because this event is scheduled to occur when the VM is enabled for
;   interrupts and not in a critical section. 
;
;************************************************************************

ITIMER_COUNT    equ     30h


ITIMER_STATE    equ     31h
;               Bits defined in the state register 
ITIMER_STOPPED  EQU     0
ITIMER_RUNNING  EQU     01h
ITIMER_ENHANCED EQU     02h
ITIMER_NO_RUPT  EQU     04h
ITIMER_ERROR    EQU     80h


ITIMER_VECTOR   equ     66h
ITIMER_SIG      equ     1630h   

