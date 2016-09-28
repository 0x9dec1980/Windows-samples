Win32 applications can communicate directly with SCSI devices via
IOCtls.  The Win32 API that is used to implement an IOCtl request is
DeviceIoControl.  In order to make the DeviceIoControl call, a valid
handle must be obtained to the device.  The handle is obtained using
the Win32 API, CreateFile.

There are five IOCtls's that the SCSI port driver supports.  These
are IOCTL_SCSI_GET_INQUIRY_DATA, IOCTL_SCSI_GET_CAPABILITIES,
IOCTL_SCSI_PASS_THROUGH, IOCTL_SCSI_PASS_THROUGH_DIRECT and
IOCTL_SCSI_MINIPORT.  The first four are demonstrated in this sample.

IOCTL_SCSI_GET_INQUIRY_DATA

The first IOCtl, IOCTL_SCSI_GET_INQUIRY_DATA, fills out a
SCSI_ADAPTER_BUS_INFO structure for all devices that are on the SCSI
bus.  The structure member, BusData, is a structure of type
SCSI_BUS_DATA.  It contains an offset to the inquiry data which is
also stored as a structure, SCSI_INQUIRY_DATA.  See NTDDSCSI.H for
more details about these structures.  One interesting
SCSI_INQUIRY_DATA structure member is DeviceClaimed.  This tells the
caller whether or not this device has been claimed by a class driver.

IOCTL_SCSI_GET_CAPABILITIES

IOCTL_SCSI_GET_CAPABILITES fills out an IO_SCSI_CAPABILITES structure
(also found in NTDDSCSI.H).  This structure contains valuable
information.  Two items of note are the MaximumTransferLength, which
is the largest data block that can be handled in a single SRB as
defined by the miniport driver, and the AlignmentMask which defines
the alignment requirements of the current CPU platform.  For an x86
system, the AlignmentMask is 0 as there is no alignment requirement.

IOCTL_SCSI_PASS_THROUGH, IOCTL_SCSI_PASS_THROUGH_DIRECT

The two remaining IOCtls, IOCTL_SCSI_PASS_THROUGH and
IOCTL_SCSI_PASS_THROUGH_DIRECT, allow SCSI CDBs (Command Descriptor
Blocks) to be sent from a Win32 application to a SCSI device. 
Depending on which IOCtl is used, a corresponding pass through
structure is filled out by the Win32 application.  For
IOCTL_SCSI_PASS_THROUGH, the structure is SCSI_PASS_THROUGH.  For 
IOCTL_SCSI_PASS_THROUGH_DIRECT, the structure is
SCSI_PASS_THROUGH_DIRECT.  See NTDDSCSI.H for more details about
these structures.

The two structures SCSI_PASS_THROUGH and SCSI_PASS_THROUGH_DIRECT are
virtually identical.  The only difference is that the data buffer for
the SCSI_PASS_THROUGH structure must be contiguous with the
structure.  This structure member is called DataBufferOffset and is
of type ULONG.  The data buffer for the SCSI_PASS_THROUGH_DIRECT
structure does not have to be contiguous with the structure.  This
structure member is called DataBuffer and is of type PVOID.  This is
the only difference between the two structures.

IOCTL_SCSI_PASS_THROUGH and IOCTL_SCSI_PASS_THROUGH_DIRECT are not
substitues for a SCSI class driver and should only be used as a last
resort.  In situations in which CDBs need to be sent to a SCSI
device, the best solution is to write a SCSI class driver that will
send these CDBs to the respective devices.

Obtaining a handle to a device

Before any IOCtls can be sent to a SCSI device, a handle for the
device must be obtained.  The Win32 API, CreateFile, is used to
obtain this handle and to define the sharing mode and the access
mode.  The key is to supply the proper filename for the device that
is to be opened.  It is possible to obtain a handle to the device via
either the SCSI port driver or the appropriate SCSI class driver.  If
the handle is obtained via the class driver, then the filename to be
opened is defined by the class driver.  In the case of fixed disks
and optical storage devices, the filename is the corresponding drive
letter.  "\\.\I:" can be used to obtain a handle to a CD-ROM drive
that has been mapped to I:.  In the case of SCSI printers, the
filename is "LPTn", where n = 1, 2, etc.  For all remaining SCSI
devices, the appropriate name is defined by the SCSI class driver. 
Typically the name is related to the device (e.g. - "\\.\Scanner0",
"\\.\Tape1").  Except for COMn and LPTn, all device filenames must be
prepended with a \\.\ to inform the I/O Manager that this is a
device.

If the device is unclaimed by a SCSI class driver, then a handle to
the SCSI port driver is required.  The filename in this case is
"\\.\ScsiN:", where N = 0, 1, 2, etc.  The number N corresponds to
the SCSI host adapter card number that controls the desired SCSI
device.  If the device is claimed by a class driver and one of the
pass through IOCtls is to be used, then the filename opened must be
the one that is defined by the class driver.  While a handle to the
SCSI port driver can be obtained, subsequent calls to DeviceIoControl
with a control code of IOCTL_SCSI_PASS_THROUGH (or
IOCTL_SCSI_PASS_THROUGH_DIRECT) that reference a claimed device will
always fail with ERROR_INVALID_PARAMETER.

Initializing the structures

Once a valid handle is obtained to a SCSI device, then appropriate
input/output buffers for the requested IOCtl must be allocated and
defined in the DeviceIoControl parameters.  In the case of
IOCTL_SCSI_GET_INQUIRY_DATA and IOCTL_SCSI_GET_CAPABILITIES, no data
is sent to the device.  Data is only read from the device. Therefore,
the pointer to the input buffer is NULL and it's length is 0.  As for
the IOCTL_SCSI_GET_CAPABILITES output buffer, the buffer size must be
at least as large as the IO_SCSI_CAPABILITES structure.  The output
buffer for IOCTL_SCSI_GET_INQUIRY_DATA should be quite large as each
SCSI device on the bus will provide data that will fill three
structures for each device, SCSI_ADAPTER_BUS_INFO, SCSI_BUS_DATA and
SCSI_INQUIRY_DATA.  For the two pass through IOCtls,
IOCTL_SCSI_PASS_THROUGH and IOCTL_SCSI_PASS_THROUGH_DIRECT, both the
input and output buffers can vary in size depending on the sense info
buffer size and the data buffer size.  In all cases, the input and
output buffer sizes must be at least the size of the
SCSI_PASS_THROUGH (or SCSI_PASS_THROUGH_DIRECT) structure.  If the
SCSI port driver detects that one of the two buffers is too small,
then the DeviceIoControl API will fail with ERROR_INVALID_PARAMETER.

Once the appropriate input/output buffers have been allocated, then,
in the case of IOCTL_SCSI_PASS_THROUGH and
IOCTL_SCSI_PASS_THROUGH_DIRECT, the appropriate structure must be
initialized.  The SCSI_PASS_THROUGH structure is defined in
NTDDSCSI.H as follows :

typedef struct _SCSI_PASS_THROUGH {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG DataBufferOffset;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
}SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;

The Length is the size of the the SCSI_PASS_THROUGH structure.  The
ScsiStatus should be initialized to 0.  The status of the requested
SCSI operation is returned in this structure member.  The possible
SCSI statuses are defined in SCSI.H and are of the form SCSISTAT_xxx.
The PathId is the bus number for the SCSI host adapter that controls
the SCSI device in question.  Typically, this value will be 0, but
there are SCSI host adapters available that contain more than one
SCSI bus.  The TargetId and Lun are the SCSI ID number and logical
unit number for the device.  If the handle was obtained for a claimed
device, then the PathId, TargetId and Lun as defined in this
structure will be ignored and the appropriate class driver will
provide these three items.  If the handle was obtained via the SCSI
port driver, then the PathId, TargetId and Lun must be correct for
the device intended.  The CdbLength is the length of the CDB. Typical
values are 6, 10, 12 up to the maximum of 16.  The SenseInfoLength is
the length of the SenseInfo buffer.  DataIn has three possible values
which are defined in NTDDSCSI.H; SCSI_IOCTL_DATA_OUT,
SCSI_IOCTL_DATA_IN and SCSI_IOCTL_DATA_UNSPECIFIED. 
SCSI_IOCTL_DATA_UNSPECIFIED should be used only if the appropriate
SCSI miniport driver supports its usage.  The DataTransferLength is
the size of the data buffer.  The TimeOutValue is the length of time,
in milliseconds, until a time out error should occur.  This can range
from 0 to a maximum of 30 minutes (108000 seconds).  The
DataBufferOffset is the offset of the data buffer from the beginning
of the pass through structure.  For the SCSI_PASS_THROUGH_DIRECT
structure, this value is no longer an offset, but rather is a pointer
to a data buffer.  The SenseInfoOffset is similarly an offset to the
SenseInfo buffer from the beginning of the pass through structure. 
Finally, the sixteen remaining bytes are for the CDB data.  The
format of this data must conform to the SCSI-2 standard as defined by
ANSI.

Buffer Alignment

The issue of buffer alignment is handled using two possible methods
for assuring that data buffers are aligned on the appropriate
boundaries.  The first method uses the compiler to align the buffer
on the correct boundary.  The structure
SCSI_PASS_THROUGH_WITH_BUFFERS contains a member, Filler, that is of
type ULONG.  The compiler aligns Filler on a ULONG (double word)
boundary.  The structure member that follows Filler, ucSenseBuf, is
now also on a double word boundary.  The ucSenseBuf array is of a
size that is a multiple of a double word, and so this makes the last
structure member, ucDataBuf, also begin on a double word boundary.

The other method to ensure buffer alignment is demonstrated in the
AllocateAlignedBuffer procedure.  This works on the fact that a
buffer that is aligned on a certain boundary will have 0's in it's
least significant bits depending on the aligment requirements.  A
buffer allocation request is made using the C runtime call, malloc,
that is the size of the requested buffer plus the alignment mask
value as returned by IOCTL_SCSI_GET_CAPABILITIES.  A pointer is
manipulated so that it is pointing to the first possible address in
the buffer that meets the alignment requirements.

METHOD_BUFFERED

The various IOCtls that are demonstrated in this sample are defined
by the SCSI port driver (see NTDDSCSI.H for details).  The memory
buffering is METHOD_BUFFERED.  What this means if the I/O manager
examines the length of the input buffer and the length of the output
buffer as defined by the DeviceIoControl parameters and allocates a
contiguous buffer that is the size of the larger of the two buffers. 
On entry, the contents of the Win32 application's input buffer are
copied to the buffer that was allocated by the I/O manager.  The
amount of data copied is controlled by the input buffer lenght.  The
I/O manager then makes calls the appropriate SCSI class driver's (or
port driver's) DeviceIoControl routine passing it the new buffer.  On
exit, the reverse process occurs and the data in the new buffer is
copied back into the Win32 application's output buffer as defined by
the output buffer length.  This is not user configurable.

Running the SPTI.EXE sample

Two command line parameters can be used with SPTI.EXE.  The first
parameter is mandatory.  It is the name of the device to be opened. 
Typical values for this are drive letters such as C:, or device
names as defined by a class driver such as Scanner0, or the SCSI port
driver name, ScsiN:, where N = 0, 1, 2, etc.

The second parameter is optional and is used to set the access flags
and the sector size.  The default is READ/WRITE mode and a sector
size of 512.  A value of 'r' makes the mode read-only.  A value of
'w' makes the mode write-only.  A value of 'c' makes the mode
read-only and makes the sector size 2048.  Only optical storage
devices would use the 'c' parameter.
