//*************************************************************
//
//                          SMPI.H
//
//*************************************************************

#define IOCTL_SCSI_MINIPORT 0x0004D008  // see NTDDSCSI.H for definition

#define SMP_RETURN_3F       0x80012140  // defined by our miniport driver
                                        // using the Microsoft template

#define SMP_PRINT_STRING    0x80000001  // random number

//
// Define header for I/O control SRB.  See NTDDSCSI.H for definition
//

typedef struct _SRB_IO_CONTROL {
        ULONG HeaderLength;
        UCHAR Signature[8];
        ULONG Timeout;
        ULONG ControlCode;
        ULONG ReturnCode;
        ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;


typedef struct {
    SRB_IO_CONTROL sic;
    UCHAR          ucDataBuffer[512];
} SRB_BUFFER, *PSRB_BUFFER;
