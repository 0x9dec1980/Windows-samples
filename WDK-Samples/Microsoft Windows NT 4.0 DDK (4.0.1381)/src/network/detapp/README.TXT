
Microsoft Net Card Detection DLL Tester

Use.

Upon starting the app you will be presented with a main window with a
menu bar with a DLL menu.  The popup menu for the DLL menu will
contain a list of detect dlls which may be tested.  The list of DLL's
is build from the \HKEY_LM\SYSTEM\SETUP\NetcardDlls registry key.
Selecting one of the DLL choices will present the main dialog which is
to do the testing.

The dialog is composed of five listbox, four push buttons and  four
radio buttons.

The List box labled "Adpapters" displays all of the adapters that a
given DLL supports.  This list is build by calling the DLL's
NcDetectIdentify() with an Index starting at 1000 until it does not
return SUCCESS.  To the right of each choice is its search order as
returned from NcDetectIdentify() by specifing (Index+3).

The list box labeled "Parameters" displays the parameters returned by
a call to NcDetectQueryMask() for the netcard selected in the
"Adapters" list box.

The list box labeled "Range" displays the possible values of the
current selection in the "Parameters" list box.  These values are
returned by a call to NcDetectParamRange().

The "Find" button will cause the NcDetectFirstNext() function for the
adapter selected in the "Adapters" list box to be called.  If an
adapter is found then NcDetectOpenHandle() will be called for that
adapter.  If successful NcDetectQueryCfg() will be called for the
adapter.  If successful the buffer returned will be used to call
NcDetectVerifyCfg().  Finally NcDetectCloseHandle() will be called.
The Adapter name will now be displayed in the "Adapters Found" list
box.  NcDetctFirstNext() will then be called with (First==FALSE) and
the process will repeat until no more adapters are found.

The configuration of Adapters listed in the "Adapters Found" list box
may be displayed in the "Found Adapter Configuration" list box by
pressing the "Query" push button.

The "Create" button will cause NcDetectCreateHandle() to be called for
the selected adapter.  If successful NcDetectCloseHandle() will then
be called.  A message box will be displayed indicating the result.

The BusType parameter for the call to NcDetectFirstNext() and
NcDetectCreateHandle()is determined by the radio button group labeled
"Bus Type".
