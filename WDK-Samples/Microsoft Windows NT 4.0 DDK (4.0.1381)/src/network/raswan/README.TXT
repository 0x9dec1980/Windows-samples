HTDSU41.SYS -- Release 1.0 -- 9/7/94

Release notes for the HT Communications DSU41 WAN/TAPI Miniport Sample 
Driver.

This driver provides a sample of how to develop a WAN/TAPI Miniport 
driver, and shows how to install the driver to work with Windows 
NT 3.5 Remote Access Service (RAS).

This version of the driver has been tested using the WinSockets 
Stress test (WASP) and the data integrity test (CCTEST).  The REDIAL 
tests were not successful because of protocol re-synchronization 
problems in the DSU41 firmware (Rev 3.4).  This problem has been 
reported to HT Communications and should be fixed in version 3.5 
of the firmware.

This problem can make it difficult to reconnect to a host once the 
initial call has hung up.  Usually, it will allow a reconnect if 
you shut down the RAS Server side and then start it up again.  
Otherwise, it may take several attempts to get connected again.  
Once connected, communications works fine with all PPP protocols.  
The symptom of this problem is that one side will send, but the 
other side will not receive.