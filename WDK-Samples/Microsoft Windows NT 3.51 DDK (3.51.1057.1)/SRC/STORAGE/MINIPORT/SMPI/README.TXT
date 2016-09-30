This sample program consists of two parts.  First, all the necessary
source code is provided to compile a Win32 sample program that sends
an IOCtl_SCSI_MINIPORT request with a custom ControlCode to the SCSI
miniport driver.  Second, a detailed description is provided that
explains what changes the driver writer has to make to a SCSI
miniport driver to allow it to properly recognize and handle the
corresponding IOCtl request from the Win32 sample program.

Part 1 - Win32 sample

The SCSI Miniport IOCtl sample, SMPI.C, demonstrates how a Win32
application sends a user defined IOCtl Control Code to a SCSI
miniport driver. This is a simple program that is composed of four
steps.

Step 1, a handle is obtained to the SCSI miniport driver using the
Win32 API, CreateFile.  The name of the file to be opened is
\\.\Scsi0:.  As an alternative, a drive letter can be substituted for
\\.\Scsi0: (e.g. - \\.\C:).  This will map to the appropriate SCSI
miniport driver that is responsible for C:.

Step 2, the SRB_IO_CONTROL structure is filled out.  The following
items must be completed :

     HeaderLength - must be the size of an SRB_IO_CONTROL structure
     ControlCode - while strictly optional, this entry should be
              considered mandatory.  The ControlCode value is used to
              further sub-divide IOCTL_SCSI_MINIPORT requests.  The
              contents of ControlCode are defined by the SCSI miniport
              driver writer.
     Length - the size of the data buffer immediately following the
              SRB_IO_CONTROL structure.  If no additional data buffer
              is used, then this must be set to 0.

The following items of the SRB_IO_CONTROL structure are optional :

     Signature - these 8 bytes are available to help prevent IOCtl
              conflicts between various vendors
     Timeout - indicates the minimum time in seconds before the
              request has timed out.  There is no maximum Timeout for
              IOCTL_SCSI_MINIPORT.  Note, for IOCTL_SCSI_PASS_THROUGH,
              the maximum time out value is 108000 seconds (30 minutes).
     ReturnCode - this entry is filled in by the SCSI miniport to
              inform the Win32 application of the results of the
              requested action.  The contents of ReturnCode are defined
              by the SCSI miniport Driver writer.

In the SMPI.C sample, two customer defined ControlCodes are used,
SMP_RETURN_3F and SMP_PRINT_STRING.  The first requires
no additional data buffer.  The second requires that a contiguous
data buffer be appended at the end of the SRB_IO_CONTROL structure. 
The ControlCodes are defined by the SCSI miniport driver (see below).

Step 3, send the SRB_IO_CONTROL structure to the SCSI miniport driver
via the DeviceIoControl Win32 API.  The dwIoControlCode must be
IOCTL_SCSI_MINIPORT.  This particular dwIoControlCode is not
currently defined in any of the Win32 SDK header files and must be
defined in your own personal header file.  It is defined in the
Windows NT DDK header file, NTDDSCSI.H.  Including a Windows NT
DDK header file in a Win32 source file has been avoided strictly to
demonstrate the ability to write a Win32 application that accesses a
device driver without having the Windows NT DDK.

Step 4, close the handle to the SCSI miniport driver.

In SMPI.C, steps 2 and 3 are repeated to demonstrate the two
ControlCodes, SMP_RETURN_3F and SMP_PRINT_STRING.  The
first requires no extra data buffer and so 'length' is set to 0.  The
second does require additional buffer space.  The value of 100 is
used as it makes the buffer large enough to handle up to 100 bytes of
data returned by the SCSI miniport driver.

When strings are manipulated via the _memXXX functions, the
terminating null is not used.  When using strXXX commands, the
terminating null is used.  strlen does not include the terminating
null in it's total.

Part 2 - SCSI Miniport Driver

The SCSI miniport driver writer is free to define the ControlCode to
any value.  Microsoft has provided a template for defining such
values and the driver writer can use this template for determining
their ControlCode values, but are not obligated to do so.  In this
sample, SMP_RETURN_3F was defined using the template and
SMP_PRINT_STRING was defined with a random number.

If the microsoft template is to be used, then the Windows NT DDK
header file, DEVIOCTL.H, should be consulted before defining a new
IOCtl.  Also, the "Kernel-mode Driver Design Guide" contains
additional information on page B-12.  There are two documentation
errors on this page. First, the bit pattern should be :

      bit(s)     purpose
      ------     -------
      0,1        Transfer type
      2-12       Function Code
      13         Customer bit
      14,15      Required Access
      16-30      Device type
      31         Common bit

The second documentation error states that the Function Code values
can be 0x00 to 0x7F for Microsoft defined IOCtls and 0x80 to 0xFF for
user defined IOCtls.  This should be 0x000 to 0x7FF for Microsoft
defined IOCtls and 0x800 to 0xFFF for user defined IOCtls.  The
Function Code field defines the function and the Customer bit
determines whether the function is defined by Microsoft or a
customer.

The following should be added to the SCSI minport driver's header
file :

//
// IOCtl definitions
//

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0x0000 - 0x7FFF, and 0x8000 - 0xFFFF are
// reserved for use by customers.
//

#define IOCTL_SCSI_MINIPORT_IO_CONTROL  0x8001

//
// Macro definition for defining IOCTL and FSCTL function control codes.
// Note that function codes 0x000 - 0x7FF are reserved for Microsoft
// Corporation, and 0x800 - 0xFFF are reserved for customers.
//

#define RETURNCODE0x0000003F   0x850

#define SMP_RETURN_3F     CTL_CODE(IOCTL_SCSI_MINIPORT_IO_CONTROL, RETURNCODE0x0000003F, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define SMP_PRINT_STRING  0x80000001

PCHAR Signature="MyDrvr";
PCHAR DrvrString="This string was placed in the data area by the SCSI miniport driver\n";

typedef struct {
    SRB_IO_CONTROL sic;
    UCHAR          ucDataBuffer[512];
} SRB_BUFFER, *PSRB_BUFFER;

The following should be added to the SCSI miniport driver's source
code :

#include <miniport.h>
#include <devioctl.h>
#include <ntddscsi.h>
#include "mydriver.h"

and the following should be added to the SCSI miniport driver's
StartIo routine :

case SRB_FUNCTION_IO_CONTROL:

    if (!memcmp(((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature,Signature,strlen(Signature))) {

        DebugPrint((1,"MyDriverStartIo: MiniportIOCtl not supported\n"));

        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

        ScsiPortNotification(RequestComplete,
                             CardPtr,
                             Srb);
        break;
        }


    DebugPrint((1,"MyDriverStartIo: Miniport IOCtl received\n"));

    DebugPrint((3,"MyDriverStartIo: Srb->DataBuffer->ControlCode = %Xh\n",
                    ((PSRB_IO_CONTROL)(Srb->DataBuffer))->ControlCode));

    switch (((PSRB_IO_CONTROL)(Srb->DataBuffer))->ControlCode) {

        case SMP_RETURN_3F :

           Srb->SrbStatus = SRB_STATUS_SUCCESS;

           ((PSRB_IO_CONTROL)(Srb->DataBuffer))->ReturnCode =
              (ULONG) 0x0000003FL;

           ScsiPortNotification(RequestComplete,
                                CardPtr,
                                Srb);
           break;

        case SMP_PRINT_STRING :

           Srb->SrbStatus = SRB_STATUS_SUCCESS;

           DebugPrint((0,"%s",((PSRB_BUFFER)(Srb->DataBuffer))->ucDataBuffer));

           strcpy(((PSRB_BUFFER)(Srb->DataBuffer))->ucDataBuffer,DrvrString);

           ScsiPortNotification(RequestComplete,
                                CardPtr,
                                Srb);
           break;

        default :

           DebugPrint((1,"MyDriverStartIo: MiniportIOCtl not supported\n"));

           Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

           ScsiPortNotification(RequestComplete,
                                CardPtr,
                                Srb);
           break;

    } // end switch

    break;


