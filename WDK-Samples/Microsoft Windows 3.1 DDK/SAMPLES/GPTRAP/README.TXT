
GPTRAP.386

This VxD will hook itself so it gets control on GP faults.  It then
executes an INT 1, and passes control to the next handler.

