// mlioctl.h    Include file for ExAllocatePoolwithTag Example Driver
//
// Define the IOCTL codes we will use.  The IOCTL code contains a command
// identifier, plus other information about the device, the type of access
// with which the file must have been opened, and the type of buffering.
//
// Paul Sanders (Microsoft)     April 19, 1995

// Device type           -- in the "User Defined" range."
#define EAPWT_TYPE 40000

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//


#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe


// The IOCTL function codes from 0x800 to 0xFFF are for customer use.

#define IOCTL_EAPWT_LEAK_ALLOCATE_POOL \
    CTL_CODE( EAPWT_TYPE, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_EAPWT_LEAK_DEALLOCATE_POOL \
    CTL_CODE( EAPWT_TYPE, 0x901, METHOD_BUFFERED, FILE_READ_ACCESS )

//
// Define the allocate pool structure.
//

typedef struct _ALLOCATE_POOL {
    PCHAR Address;
    ULONG Tag;
}ALLOCATE_POOL, *PALLOCATE_POOL;


VOID
allocateMemory ( HANDLE );

VOID 
deAllocateMemory ( HANDLE );

VOID
initializePool ( );

VOID
exerciseMemory ( HANDLE );

