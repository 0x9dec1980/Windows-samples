//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// General
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: MSG_USAGE
//
// MessageText:
//
//  %1 Usage: %1 [-r] [-m:\\<machine>] <command> [<arg>...]
//  For more information type: %1 help
//
#define MSG_USAGE                        0x0000EA60L

//
// MessageId: MSG_FAILURE
//
// MessageText:
//
//  %1 failed.
//
#define MSG_FAILURE                      0x0000EA61L

//
// MessageId: MSG_COMMAND_USAGE
//
// MessageText:
//
//  %1: Invalid use of %2.
//  For more information type: %1 help %2
//
#define MSG_COMMAND_USAGE                0x0000EA62L

//
// HELP
//
//
// MessageId: MSG_HELP_LONG
//
// MessageText:
//
//  Device Console Help:
//  %1 [-r] [-m:\\<machine>] <command> [<arg>...]
//  -r if specified will reboot machine after command is complete, if needed.
//  <machine> is name of target machine.
//  <command> is command to perform (see below).
//  <arg>... is one or more arguments if required by command.
//  For help on a specific command, type: %1 help <command>
//
#define MSG_HELP_LONG                    0x0000EAC4L

//
// MessageId: MSG_HELP_SHORT
//
// MessageText:
//
//  %1!-20s! Display this information.
//
#define MSG_HELP_SHORT                   0x0000EAC5L

//
// MessageId: MSG_HELP_OTHER
//
// MessageText:
//
//  Unknown command: %1.
//
#define MSG_HELP_OTHER                   0x0000EAC6L

//
// CLASSES
//
//
// MessageId: MSG_CLASSES_LONG
//
// MessageText:
//
//  %1 [-m:\\<machine>] %2
//  List all device setup classes.
//  This command will work for a remote machine.
//  Classes are listed as <name>: <descr>
//  where <name> is the class name and <descr> is the class description.
//
#define MSG_CLASSES_LONG                 0x0000EB28L

//
// MessageId: MSG_CLASSES_SHORT
//
// MessageText:
//
//  %1!-20s! List all device setup classes.
//
#define MSG_CLASSES_SHORT                0x0000EB29L

//
// MessageId: MSG_CLASSES_HEADER
//
// MessageText:
//
//  Listing %1!u! setup class(es) on %2.
//
#define MSG_CLASSES_HEADER               0x0000EB2AL

//
// MessageId: MSG_CLASSES_HEADER_LOCAL
//
// MessageText:
//
//  Listing %1!u! setup class(es).
//
#define MSG_CLASSES_HEADER_LOCAL         0x0000EB2BL

//
// LISTCLASS
//
//
// MessageId: MSG_LISTCLASS_LONG
//
// MessageText:
//
//  %1 [-m:\\<machine>] %2 <class> [<class>...]
//  List all devices for specific setup classes.
//  This command will work for a remote machine.
//  <class> is the class name as obtained from the classes command.
//  Devices are listed as <instance>: <descr>
//  where <instance> is the unique instance of the device and <descr> is the description.
//
#define MSG_LISTCLASS_LONG               0x0000EB8CL

//
// MessageId: MSG_LISTCLASS_SHORT
//
// MessageText:
//
//  %1!-20s! List all devices for a setup class.
//
#define MSG_LISTCLASS_SHORT              0x0000EB8DL

//
// MessageId: MSG_LISTCLASS_HEADER
//
// MessageText:
//
//  Listing %1!u! device(s) for setup class "%2" (%3) on %4.
//
#define MSG_LISTCLASS_HEADER             0x0000EB8EL

//
// MessageId: MSG_LISTCLASS_HEADER_LOCAL
//
// MessageText:
//
//  Listing %1!u! device(s) for setup class "%2" (%3).
//
#define MSG_LISTCLASS_HEADER_LOCAL       0x0000EB8FL

//
// MessageId: MSG_LISTCLASS_NOCLASS
//
// MessageText:
//
//  No setup class "%1" on %2.
//
#define MSG_LISTCLASS_NOCLASS            0x0000EB90L

//
// MessageId: MSG_LISTCLASS_NOCLASS_LOCAL
//
// MessageText:
//
//  No setup class "%1".
//
#define MSG_LISTCLASS_NOCLASS_LOCAL      0x0000EB91L

//
// MessageId: MSG_LISTCLASS_HEADER_NONE
//
// MessageText:
//
//  No devices for setup class "%1" (%2) on %3.
//
#define MSG_LISTCLASS_HEADER_NONE        0x0000EB92L

//
// MessageId: MSG_LISTCLASS_HEADER_NONE_LOCAL
//
// MessageText:
//
//  No devices for setup class "%1" (%2).
//
#define MSG_LISTCLASS_HEADER_NONE_LOCAL  0x0000EB93L

//
// FIND
//
//
// MessageId: MSG_FIND_LONG
//
// MessageText:
//
//  %1 [-m:\\<machine>] %2 <id> [<id>...]
//  %1 [-m:\\<machine>] %2 =<class> [<id>...]
//  Find devices that match the specific hardware or instance ID.
//  This command will work for a remote machine.
//  Examples of <id> are:
//  *                  - All devices
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//  Devices are listed as <instance>: <descr>
//  where <instance> is the unique instance of the device and <descr> is the description.
//
#define MSG_FIND_LONG                    0x0000EBF0L

//
// MessageId: MSG_FIND_SHORT
//
// MessageText:
//
//  %1!-20s! Find devices that match the specific hardware or instance ID.
//
#define MSG_FIND_SHORT                   0x0000EBF1L

//
// MessageId: MSG_FIND_TAIL_NONE
//
// MessageText:
//
//  No matching devices found on %1.
//
#define MSG_FIND_TAIL_NONE               0x0000EBF2L

//
// MessageId: MSG_FIND_TAIL_NONE_LOCAL
//
// MessageText:
//
//  No matching devices found.
//
#define MSG_FIND_TAIL_NONE_LOCAL         0x0000EBF3L

//
// MessageId: MSG_FIND_TAIL
//
// MessageText:
//
//  %1!u! matching device(s) found on %2.
//
#define MSG_FIND_TAIL                    0x0000EBF4L

//
// MessageId: MSG_FIND_TAIL_LOCAL
//
// MessageText:
//
//  %1!u! matching device(s) found.
//
#define MSG_FIND_TAIL_LOCAL              0x0000EBF5L

//
// MessageId: MSG_FINDALL_LONG
//
// MessageText:
//
//  %1 [-m:\\<machine>] %2 <id> [<id>...]
//  %1 [-m:\\<machine>] %2 =<class> [<id>...]
//  Find devices that match the specific hardware or instance ID, including those that are not present.
//  This command will work for a remote machine.
//  Examples of <id> are:
//  *                  - All devices
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//  Devices are listed as <instance>: <descr>
//  where <instance> is the unique instance of the device and <descr> is the description.
//
#define MSG_FINDALL_LONG                 0x0000EBF6L

//
// MessageId: MSG_FINDALL_SHORT
//
// MessageText:
//
//  %1!-20s! Find devices including those that are not present.
//
#define MSG_FINDALL_SHORT                0x0000EBF7L

//
// MessageId: MSG_STATUS_LONG
//
// MessageText:
//
//  %1 [-m:\\<machine>] %2 <id> [<id>...]
//  %1 [-m:\\<machine>] %2 =<class> [<id>...]
//  Lists running status of devices that match the specific hardware or instance ID.
//  This command will work for a remote machine.
//  Examples of <id> are:
//  *                  - All devices
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//
#define MSG_STATUS_LONG                  0x0000EBF8L

//
// MessageId: MSG_STATUS_SHORT
//
// MessageText:
//
//  %1!-20s! List running status of devices.
//
#define MSG_STATUS_SHORT                 0x0000EBF9L

//
// MessageId: MSG_DRIVERFILES_LONG
//
// MessageText:
//
//  %1 %2 <id> [<id>...]
//  %1 %2 =<class> [<id>...]
//  Lists driver files installed for devices that match the specific hardware or instance ID.
//  This command will only work for local machine.
//  Examples of <id> are:
//  *                  - All devices
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//
#define MSG_DRIVERFILES_LONG             0x0000EBFAL

//
// MessageId: MSG_DRIVERFILES_SHORT
//
// MessageText:
//
//  %1!-20s! List driver files installed for devices.
//
#define MSG_DRIVERFILES_SHORT            0x0000EBFBL

//
// MessageId: MSG_RESOURCES_LONG
//
// MessageText:
//
//  %1 [-m:\\<machine>] %2 <id> [<id>...]
//  %1 [-m:\\<machine>] %2 =<class> [<id>...]
//  Lists hardware resources of devices that match the specific hardware or instance ID.
//  This command will work for a remote machine.
//  Examples of <id> are:
//  *                  - All devices
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//
#define MSG_RESOURCES_LONG               0x0000EBFCL

//
// MessageId: MSG_RESOURCES_SHORT
//
// MessageText:
//
//  %1!-20s! Lists hardware resources of devices.
//
#define MSG_RESOURCES_SHORT              0x0000EBFDL

//
// MessageId: MSG_HWIDS_LONG
//
// MessageText:
//
//  %1 [-m:\\<machine>] %2 <id> [<id>...]
//  %1 [-m:\\<machine>] %2 =<class> [<id>...]
//  Lists all hardware ID's of devices that match the specific hardware or instance ID.
//  This command will work for a remote machine.
//  Examples of <id> are:
//  *                  - All devices
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//
#define MSG_HWIDS_LONG                   0x0000EBFEL

//
// MessageId: MSG_HWIDS_SHORT
//
// MessageText:
//
//  %1!-20s! Lists hardware ID's of devices.
//
#define MSG_HWIDS_SHORT                  0x0000EBFFL

//
// MessageId: MSG_STACK_LONG
//
// MessageText:
//
//  %1 [-m:\\<machine>] %2 <id> [<id>...]
//  %1 [-m:\\<machine>] %2 =<class> [<id>...]
//  Lists expected driver stack of devices that match the specific hardware or instance ID. %0
//  PnP will call each driver's AddDevice when building the device stack. %0
//  This command will work for a remote machine.
//  Examples of <id> are:
//  *                  - All devices
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//
#define MSG_STACK_LONG                   0x0000EC00L

//
// MessageId: MSG_STACK_SHORT
//
// MessageText:
//
//  %1!-20s! Lists expected driver stack of devices.
//
#define MSG_STACK_SHORT                  0x0000EC01L

//
// ENABLE
//
//
// MessageId: MSG_ENABLE_LONG
//
// MessageText:
//
//  %1 [-r] %2 <id> [<id>...]
//  %1 [-r] %2 =<class> [<id>...]
//  Enable devices that match the specific hardware or instance ID.
//  This command will only work for local machine.
//  Specify -r to reboot automatically if needed.
//  Examples of <id> are:
//  *                  - All devices (not recommended)
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//  Devices are enabled if possible.
//
#define MSG_ENABLE_LONG                  0x0000EC54L

//
// MessageId: MSG_ENABLE_SHORT
//
// MessageText:
//
//  %1!-20s! Enable devices that match the specific hardware or instance ID.
//
#define MSG_ENABLE_SHORT                 0x0000EC55L

//
// MessageId: MSG_ENABLE_TAIL_NONE
//
// MessageText:
//
//  No devices enabled.
//
#define MSG_ENABLE_TAIL_NONE             0x0000EC56L

//
// MessageId: MSG_ENABLE_TAIL_REBOOT
//
// MessageText:
//
//  Not all of %1!u! device(s) enabled, at least one requires reboot to complete the operation.
//
#define MSG_ENABLE_TAIL_REBOOT           0x0000EC57L

//
// MessageId: MSG_ENABLE_TAIL
//
// MessageText:
//
//  %1!u! device(s) enabled.
//
#define MSG_ENABLE_TAIL                  0x0000EC58L

//
// DISABLE
//
//
// MessageId: MSG_DISABLE_LONG
//
// MessageText:
//
//  %1 [-r] %2 <id> [<id>...]
//  %1 [-r] %2 =<class> [<id>...]
//  Disable devices that match the specific hardware or instance ID.
//  This command will only work for local machine.
//  Specify -r to reboot automatically if needed.
//  Examples of <id> are:
//  *                  - All devices (not recommended)
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//  Devices are disabled if possible.
//
#define MSG_DISABLE_LONG                 0x0000ECB8L

//
// MessageId: MSG_DISABLE_SHORT
//
// MessageText:
//
//  %1!-20s! Disable devices that match the specific hardware or instance ID.
//
#define MSG_DISABLE_SHORT                0x0000ECB9L

//
// MessageId: MSG_DISABLE_TAIL_NONE
//
// MessageText:
//
//  No devices disabled.
//
#define MSG_DISABLE_TAIL_NONE            0x0000ECBAL

//
// MessageId: MSG_DISABLE_TAIL_REBOOT
//
// MessageText:
//
//  Not all of %1!u! device(s) disabled, at least one requires reboot to complete the operation.
//
#define MSG_DISABLE_TAIL_REBOOT          0x0000ECBBL

//
// MessageId: MSG_DISABLE_TAIL
//
// MessageText:
//
//  %1!u! device(s) disabled.
//
#define MSG_DISABLE_TAIL                 0x0000ECBCL

//
// RESTART
//
//
// MessageId: MSG_RESTART_LONG
//
// MessageText:
//
//  %1 [-r] %2 <id> [<id>...]
//  %1 [-r] %2 =<class> [<id>...]
//  Restarts devices that match the specific hardware or instance ID.
//  This command will only work for local machine.
//  Specify -r to reboot automatically if needed.
//  Examples of <id> are:
//  *                  - All devices (not recommended)
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//  Devices are restarted if possible.
//
#define MSG_RESTART_LONG                 0x0000ED1CL

//
// MessageId: MSG_RESTART_SHORT
//
// MessageText:
//
//  %1!-20s! Restart devices that match the specific hardware or instance ID.
//
#define MSG_RESTART_SHORT                0x0000ED1DL

//
// MessageId: MSG_RESTART_TAIL_NONE
//
// MessageText:
//
//  No devices restarted.
//
#define MSG_RESTART_TAIL_NONE            0x0000ED1EL

//
// MessageId: MSG_RESTART_TAIL_REBOOT
//
// MessageText:
//
//  Not all of %1!u! device(s) restarted, at least one requires reboot to complete the operation.
//
#define MSG_RESTART_TAIL_REBOOT          0x0000ED1FL

//
// MessageId: MSG_RESTART_TAIL
//
// MessageText:
//
//  %1!u! device(s) restarted.
//
#define MSG_RESTART_TAIL                 0x0000ED20L

//
// REBOOT
//
//
// MessageId: MSG_REBOOT_LONG
//
// MessageText:
//
//  %1 %2
//  Reboot local machine indicating planned hardware install.
//
#define MSG_REBOOT_LONG                  0x0000ED80L

//
// MessageId: MSG_REBOOT_SHORT
//
// MessageText:
//
//  %1!-20s! Reboot local machine.
//
#define MSG_REBOOT_SHORT                 0x0000ED81L

//
// MessageId: MSG_REBOOT
//
// MessageText:
//
//  Rebooting local machine.
//
#define MSG_REBOOT                       0x0000ED82L

//
// DUMP
//
//
// MessageId: MSG_DUMP_PROBLEM
//
// MessageText:
//
//  Device has a problem: %1!02u!.
//
#define MSG_DUMP_PROBLEM                 0x0000EDE8L

//
// MessageId: MSG_DUMP_PRIVATE_PROBLEM
//
// MessageText:
//
//  Device has a problem reported by the driver.
//
#define MSG_DUMP_PRIVATE_PROBLEM         0x0000EDE9L

//
// MessageId: MSG_DUMP_STARTED
//
// MessageText:
//
//  Driver is running.
//
#define MSG_DUMP_STARTED                 0x0000EDEAL

//
// MessageId: MSG_DUMP_DISABLED
//
// MessageText:
//
//  Device is disabled.
//
#define MSG_DUMP_DISABLED                0x0000EDEBL

//
// MessageId: MSG_DUMP_NOTSTARTED
//
// MessageText:
//
//  Device is currently stopped.
//
#define MSG_DUMP_NOTSTARTED              0x0000EDECL

//
// MessageId: MSG_DUMP_NO_RESOURCES
//
// MessageText:
//
//  Device is not using any resources.
//
#define MSG_DUMP_NO_RESOURCES            0x0000EDEDL

//
// MessageId: MSG_DUMP_NO_RESERVED_RESOURCES
//
// MessageText:
//
//  Device has no resources reserved.
//
#define MSG_DUMP_NO_RESERVED_RESOURCES   0x0000EDEEL

//
// MessageId: MSG_DUMP_RESOURCES
//
// MessageText:
//
//  Device is currently using the following resources:
//
#define MSG_DUMP_RESOURCES               0x0000EDEFL

//
// MessageId: MSG_DUMP_RESERVED_RESOURCES
//
// MessageText:
//
//  Device has the following resources reserved:
//
#define MSG_DUMP_RESERVED_RESOURCES      0x0000EDF0L

//
// MessageId: MSG_DUMP_DRIVER_FILES
//
// MessageText:
//
//  Driver installed from %2 [%3]. %1!u! file(s) used by driver:
//
#define MSG_DUMP_DRIVER_FILES            0x0000EDF1L

//
// MessageId: MSG_DUMP_NO_DRIVER_FILES
//
// MessageText:
//
//  Driver installed from %2 [%3]. No files used by driver.
//
#define MSG_DUMP_NO_DRIVER_FILES         0x0000EDF2L

//
// MessageId: MSG_DUMP_NO_DRIVER
//
// MessageText:
//
//  No driver information available for device.
//
#define MSG_DUMP_NO_DRIVER               0x0000EDF3L

//
// MessageId: MSG_DUMP_HWIDS
//
// MessageText:
//
//  Hardware ID's:
//
#define MSG_DUMP_HWIDS                   0x0000EDF4L

//
// MessageId: MSG_DUMP_COMPATIDS
//
// MessageText:
//
//  Compatible ID's:
//
#define MSG_DUMP_COMPATIDS               0x0000EDF5L

//
// MessageId: MSG_DUMP_NO_HWIDS
//
// MessageText:
//
//  No hardware/compatible ID's available for this device.
//
#define MSG_DUMP_NO_HWIDS                0x0000EDF6L

//
// MessageId: MSG_DUMP_NO_DRIVERNODES
//
// MessageText:
//
//  No DriverNodes found for device.
//
#define MSG_DUMP_NO_DRIVERNODES          0x0000EDF7L

//
// MessageId: MSG_DUMP_DRIVERNODE_HEADER
//
// MessageText:
//
//  DriverNode #%1!u!:
//
#define MSG_DUMP_DRIVERNODE_HEADER       0x0000EDF8L

//
// MessageId: MSG_DUMP_DRIVERNODE_INF
//
// MessageText:
//
//  Inf file is %1
//
#define MSG_DUMP_DRIVERNODE_INF          0x0000EDF9L

//
// MessageId: MSG_DUMP_DRIVERNODE_SECTION
//
// MessageText:
//
//  Inf section is %1
//
#define MSG_DUMP_DRIVERNODE_SECTION      0x0000EDFAL

//
// MessageId: MSG_DUMP_DRIVERNODE_DESCRIPTION
//
// MessageText:
//
//  Driver description is %1
//
#define MSG_DUMP_DRIVERNODE_DESCRIPTION  0x0000EDFBL

//
// MessageId: MSG_DUMP_DRIVERNODE_MFGNAME
//
// MessageText:
//
//  Manufacturer name is %1
//
#define MSG_DUMP_DRIVERNODE_MFGNAME      0x0000EDFCL

//
// MessageId: MSG_DUMP_DRIVERNODE_PROVIDERNAME
//
// MessageText:
//
//  Provider name is %1
//
#define MSG_DUMP_DRIVERNODE_PROVIDERNAME 0x0000EDFDL

//
// MessageId: MSG_DUMP_DRIVERNODE_DRIVERDATE
//
// MessageText:
//
//  Driver date is %1
//
#define MSG_DUMP_DRIVERNODE_DRIVERDATE   0x0000EDFEL

//
// MessageId: MSG_DUMP_DRIVERNODE_DRIVERVERSION
//
// MessageText:
//
//  Driver version is %1!u!.%2!u!.%3!u!.%4!u!
//
#define MSG_DUMP_DRIVERNODE_DRIVERVERSION 0x0000EDFFL

//
// MessageId: MSG_DUMP_DRIVERNODE_RANK
//
// MessageText:
//
//  Driver node rank is %1!u!
//
#define MSG_DUMP_DRIVERNODE_RANK         0x0000EE00L

//
// MessageId: MSG_DUMP_DRIVERNODE_FLAGS
//
// MessageText:
//
//  Driver node flags are %1!08X!
//
#define MSG_DUMP_DRIVERNODE_FLAGS        0x0000EE01L

//
// MessageId: MSG_DUMP_DRIVERNODE_FLAGS_OLD_INET_DRIVER
//
// MessageText:
//
//  Inf came from the Internet
//
#define MSG_DUMP_DRIVERNODE_FLAGS_OLD_INET_DRIVER 0x0000EE02L

//
// MessageId: MSG_DUMP_DRIVERNODE_FLAGS_BAD_DRIVER
//
// MessageText:
//
//  Driver node is marked as BAD
//
#define MSG_DUMP_DRIVERNODE_FLAGS_BAD_DRIVER 0x0000EE03L

//
// MessageId: MSG_DUMP_DRIVERNODE_FLAGS_INF_IS_SIGNED
//
// MessageText:
//
//  Inf is digitally signed
//
#define MSG_DUMP_DRIVERNODE_FLAGS_INF_IS_SIGNED 0x0000EE04L

//
// MessageId: MSG_DUMP_DRIVERNODE_FLAGS_OEM_F6_INF
//
// MessageText:
//
//  Inf was installed using F6 during textmode setup
//
#define MSG_DUMP_DRIVERNODE_FLAGS_OEM_F6_INF 0x0000EE05L

//
// MessageId: MSG_DUMP_DRIVERNODE_FLAGS_BASIC_DRIVER
//
// MessageText:
//
//  Driver provides basic functionality if no other signed driver exists.
//
#define MSG_DUMP_DRIVERNODE_FLAGS_BASIC_DRIVER 0x0000EE06L

//
// MessageId: MSG_DUMP_DEVICESTACK_UPPERCLASSFILTERS
//
// MessageText:
//
//  Class upper filters:
//
#define MSG_DUMP_DEVICESTACK_UPPERCLASSFILTERS 0x0000EE07L

//
// MessageId: MSG_DUMP_DEVICESTACK_UPPERFILTERS
//
// MessageText:
//
//  Upper filters:
//
#define MSG_DUMP_DEVICESTACK_UPPERFILTERS 0x0000EE08L

//
// MessageId: MSG_DUMP_DEVICESTACK_SERVICE
//
// MessageText:
//
//  Controlling service:
//
#define MSG_DUMP_DEVICESTACK_SERVICE     0x0000EE09L

//
// MessageId: MSG_DUMP_DEVICESTACK_NOSERVICE
//
// MessageText:
//
//  (none)
//
#define MSG_DUMP_DEVICESTACK_NOSERVICE   0x0000EE0AL

//
// MessageId: MSG_DUMP_DEVICESTACK_LOWERCLASSFILTERS
//
// MessageText:
//
//  Class lower filters:
//
#define MSG_DUMP_DEVICESTACK_LOWERCLASSFILTERS 0x0000EE0BL

//
// MessageId: MSG_DUMP_DEVICESTACK_LOWERFILTERS
//
// MessageText:
//
//  Lower filters:
//
#define MSG_DUMP_DEVICESTACK_LOWERFILTERS 0x0000EE0CL

//
// MessageId: MSG_DUMP_SETUPCLASS
//
// MessageText:
//
//  Setup Class: %1 %2
//
#define MSG_DUMP_SETUPCLASS              0x0000EE0DL

//
// MessageId: MSG_DUMP_NOSETUPCLASS
//
// MessageText:
//
//  Device not setup.
//
#define MSG_DUMP_NOSETUPCLASS            0x0000EE0EL

//
// MessageId: MSG_DUMP_DESCRIPTION
//
// MessageText:
//
//  Name: %1
//
#define MSG_DUMP_DESCRIPTION             0x0000EE0FL

//
// INSTALL
//
//
// MessageId: MSG_INSTALL_LONG
//
// MessageText:
//
//  %1 [-r] %2 <inf> <hwid>
//  Manually installs a device.
//  This command will only work for local machine.
//  Specify -r to reboot automatically if needed.
//  <inf> is an INF to use to install the device.
//  <hwid> is a hardware ID to apply to the device.
//
#define MSG_INSTALL_LONG                 0x0000EE48L

//
// MessageId: MSG_INSTALL_SHORT
//
// MessageText:
//
//  %1!-20s! Manually install a device.
//
#define MSG_INSTALL_SHORT                0x0000EE49L

//
// UPDATE
//
//
// MessageId: MSG_UPDATE_LONG
//
// MessageText:
//
//  %1 [-r] %2 <inf> <hwid>
//  Update drivers for devices.
//  This command will only work for local machine.
//  Specify -r to reboot automatically if needed.
//  <inf> is an INF to use to install the device.
//  All devices that match <hwid> are updated.
//
#define MSG_UPDATE_LONG                  0x0000EEACL

//
// MessageId: MSG_UPDATE_SHORT
//
// MessageText:
//
//  %1!-20s! Manually update a device.
//
#define MSG_UPDATE_SHORT                 0x0000EEADL

//
// MessageId: MSG_UPDATE_INF
//
// MessageText:
//
//  Updating drivers for %1 from %2.
//
#define MSG_UPDATE_INF                   0x0000EEAEL

//
// MessageId: MSG_UPDATE
//
// MessageText:
//
//  Updating drivers for %1.
//
#define MSG_UPDATE                       0x0000EEAFL

//
// REMOVE
//
//
// MessageId: MSG_REMOVE_LONG
//
// MessageText:
//
//  %1 [-r] %2 <id> [<id>...]
//  %1 [-r] %2 =<class> [<id>...]
//  Remove devices that match the specific hardware or instance ID.
//  This command will only work for local machine.
//  Specify -r to reboot automatically if needed.
//  Examples of <id> are:
//  *                  - All devices (not recommended)
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//
#define MSG_REMOVE_LONG                  0x0000EF10L

//
// MessageId: MSG_REMOVE_SHORT
//
// MessageText:
//
//  %1!-20s! Remove devices that match the specific hardware or instance ID.
//
#define MSG_REMOVE_SHORT                 0x0000EF11L

//
// MessageId: MSG_REMOVE_TAIL_NONE
//
// MessageText:
//
//  No devices removed.
//
#define MSG_REMOVE_TAIL_NONE             0x0000EF12L

//
// MessageId: MSG_REMOVE_TAIL_REBOOT
//
// MessageText:
//
//  Not all of %1!u! device(s) removed, at least one requires reboot to complete the operation.
//
#define MSG_REMOVE_TAIL_REBOOT           0x0000EF13L

//
// MessageId: MSG_REMOVE_TAIL
//
// MessageText:
//
//  %1!u! device(s) removed.
//
#define MSG_REMOVE_TAIL                  0x0000EF14L

//
// RESCAN
//
//
// MessageId: MSG_RESCAN_LONG
//
// MessageText:
//
//  %1 [-m:\\<machine>]
//  Tell plug&play to scan for new hardware.
//
#define MSG_RESCAN_LONG                  0x0000EF74L

//
// MessageId: MSG_RESCAN_SHORT
//
// MessageText:
//
//  %1!-20s! Scan for new hardware.
//
#define MSG_RESCAN_SHORT                 0x0000EF75L

//
// MessageId: MSG_RESCAN_LOCAL
//
// MessageText:
//
//  Scanning for new hardware.
//
#define MSG_RESCAN_LOCAL                 0x0000EF76L

//
// MessageId: MSG_RESCAN
//
// MessageText:
//
//  Scanning for new hardware on %1.
//
#define MSG_RESCAN                       0x0000EF77L

//
// DRIVERNODES
//
//
// MessageId: MSG_DRIVERNODES_LONG
//
// MessageText:
//
//  %1 %2 <id> [<id>...]
//  %1 %2 =<class> [<id>...]
//  Lists driver nodes for devices that match the specific hardware or instance ID.
//  This command will only work for local machine.
//  Examples of <id> are:
//  *                  - All devices
//  ISAPNP\PNP0501     - Hardware ID
//  *PNP*              - Hardware ID with wildcards (* matches anything)
//  @ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
//  <class> is a setup class name as obtained from the classes command.
//
#define MSG_DRIVERNODES_LONG             0x0000EFD8L

//
// MessageId: MSG_DRIVERNODES_SHORT
//
// MessageText:
//
//  %1!-20s! Lists all the driver nodes of devices.
//
#define MSG_DRIVERNODES_SHORT            0x0000EFD9L


