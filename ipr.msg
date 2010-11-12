$quote "

$set 0  Generic
1 "Select one of the following"
2 "Selection"
3 "Complete"
4 "to return to menu, current operations will continue"

$set 101  main_menu
1 "Work with Disk Units"
2 "Display disk hardware status"
3 "Work with Disk Arrays"
4 "Work with disk unit recovery"
5 "Work with SCSI bus configuration"
6 "Work with driver configuration"
7 "Analyze log"

$set 102 disk_status
1 "Display Disk Hardware Status"
2 "Type option, press Enter."
3 "Display hardware resource information details"
4 "No devices found"

$set 104 device_details
1 "IOA Hardware Resource Information Details"
2 "Manufacturer"
3 "Machine Type and Model"
4 "Driver Version"
5 "Firmware Version"
6 "Serial Number"
7 "Part Number"
8 "Plant of Manufacturer"
9 "Cache Size"
10 "DRAM Size"
11 "Resource Name"
12 "Physical location"
13 "PCI Address"
14 "SCSI Host Number"
15 "Resource Path"

101 "Disk Array Information Details"
102 "RAID Level"
103 "Stripe Size"
104 "Capacity"

201 "Disk Unit Hardware Resource Information Details"
202 "Product ID"
203 "SCSI Host Number"
204 "SCSI Channel"
205 "SCSI Id"
206 "SCSI Lun"
207 "Level"
208 "Extended Details"
209 "FRU Number"
210 "EC Level"
211 "Device Specific (Z0)"
212 "Device Specific (Z1)"
213 "Device Specific (Z2)"
214 "Device Specific (Z3)"
215 "Device Specific (Z4)"
216 "Device Specific (Z5)"
217 "Device Specific (Z6)"

$set 200 raid_screen
1 "Work with Disk Arrays"
2 "Display disk array status"
3 "Create a disk array"
4 "Delete a disk array"
5 "Add a device to a disk array"
6 "Format device for advanced function"
7 "Format device for JBOD function"
8 "Configure a hot spare device"
9 "Unconfigure a hot spare device"

$set 201 raid_status
1 "Display Disk Array Status"
2 "Type option, press Enter."
3 "Display hardware resource information details"
4 "No devices found"

$set 202 raid_start
1 "Create a Disk Array"
2 "Select the subsystems to create a disk array."
3 "Type choice, press Enter."
4 "create a disk array"

$set 203 raid_stop
1 "Delete a Disk Array"
2 "Select the subsystems to delete a disk array."
3 "Type choice, press Enter."
4 "delete a disk array"

$set 204 raid_stop_fail
1 "Delete a Disk Array Failed"
2 "There are no disk units eligible for the selected operation\
 due to one or more of the following reasons:"
3 "There are no device parity protected units in the system."
4 "An IOA is in a condition that makes the disk units attached to\
 it read/write protected. Examine the kernel messages log\
 for any errors that are logged for the IO subsystem\
 and follow the appropriate procedure for the reference code to\
 correct the problem, if necessary."
5 "Not all disk units attached to an advanced function IOA have\
 reported to the system. Retry the operation."
6 "The disk units are missing."
7 "The disk unit/s are currently in use."

$set 205 confirm_raid_stop
1 "Confirm Delete a Disk Array"
2 "ATTENTION: Disk units connected to these subsystems will not be\
 protected after you confirm your choice."
3 "Press Enter to continue."
4 "Cancel to return and change your choice."

$set 207 raid_start_fail
1 "Create a Disk Array Failed"
2 "There are no disk units eligible for the selected operation due\
 to one or more of the following reasons:"
3 "There are not enough device parity capable units in the system."
4 "An IOA is in a condition that makes the disk units attached to\
 it read/write protected. Examine the kernel messages log\
 for any errors that are logged for the IO subsystem\
 and follow the appropriate procedure for the reference code to\
 correct the problem, if necessary."
5 "Not all disk units attached to an advanced function IOA have\
 reported to the system. Retry the operation."
6 "The disk units are missing."
7 "The disk unit/s are currently in use."

$set 208 configure_raid_start
1 "Select Disk Units for Parity Protection"
2 "Type option, press Enter."
3 "Select"

$set 210 confirm_raid_start
1 "Confirm Start Device Parity Protection"
2 "ATTENTION: Data will not be preserved and may be lost on selected disk\
 units when parity protection is enabled."
3 "Press Enter to continue."
4 "Cancel to return and change your choice."

$set 211 raid_start_complete
1 "Start Device Parity Protection Status"
2 "You selected to start device parity protection"

$set 215 raid_include
1 "Add a Device to a Disk Array"
2 "Select the subsystem that the disk unit will be included."
3 "Type choice, press Enter."
4 "Select Parity Array to Include Disk Unit"

$set 216 raid_include_fail
1 "Include Device Parity Protection Failed"
2 "There are no disk units eligible for the selected operation\
 due to one or more of the following reasons:"
3 "There are no device parity protected units in the system."
4 "An IOA is in a condition that makes the disk units attached to\
 it read/write protected. Examine the kernel messages log\
 for any errors that are logged for the IO subsystem\
 and follow the appropriate procedure for the reference code to\
 correct the problem, if necessary."
5 "Not all disk units attached to an advanced function IOA have\
 reported to the system. Retry the operation."
6 "The disk units are missing."
7 "The disk unit/s are currently in use."

$set 217 configure_raid_include
1 "Include Disk Units in Device Parity Protection"
2 "Select the units to be included in Device Parity Protection"
3 "Type choice, press Enter."
4 "Add a Device to a Disk Array"

$set 218 configure_raid_include_fail
1 "Include Disk Unit Failed"
2 "There are no disk units eligible for the selected operation\
 due to one or more of the following reasons:"
3 "There are not enough disk units available to be included."
4 "The disk unit that needs to be included is not at the right\
 location. Examine the 'message log' for the exposed unit\
 and make sure that the replacement unit is at the correct\
 location"
5 "Not all disk units attached to an IOA have reported to the\
 system. Retry the operation."
6 "The disk unit to be included must be the same or greater\
 capacity than the smallest device in the volume set and\
 be formatted correctly"
7 "The type/model of the disk units is not supported for the\
 requested operation"

$set 219 confirm_raid_include
1 "Confirm Disk Units to be Included"
2 "ATTENTION: All listed device units will be formatted and zeroed before\
 listed device is added to array."
3 "Press Enter to confirm your choice to have the system include\
 the selected units in device parity protection"
4 "Cancel to return and change your choice."

$set 221 dev_include_complete
1 "Include Disk Units in Device Parity Protection Status"
2 "The operation to include units in the device parity protection\
 will be done in two phases.  The device must first be formatted\
 before device may be included into array set."
3 "You selected to include a device in a device parity set"

$set 230 af_include_fail
1 "Format Device for Advanced Function Failed"
2 "There are no disk units eligible for the selected operation\
 due to one or more of the following reasons:"
3 "There are no eligible device units in the system."
4 "An IOA is in a condition that makes the disk units attached to\
 it read/write protected. Examine the kernel messages log\
 for any errors that are logged for the IO subsystem\
 and follow the appropriate procedure for the reference code to\
 correct the problem, if necessary."
5 "Not all disk units attached to an advanced function IOA have\
 reported to the system. Retry the operation."
6 "The disk units are missing."
7 "The disk unit/s are currently in use."

$set 231 af_remove_fail
1 "Disable Device for Advanced Functions Failed"
2 "There are no disk units eligible for the selected operation\
 due to one or more of the following reasons:"
3 "There are no eligible device units in the system."
4 "An IOA is in a condition that makes the disk units attached to\
 it read/write protected. Examine the kernel messages log\
 for any errors that are logged for the IO subsystem\
 and follow the appropriate procedure for the reference code to\
 correct the problem, if necessary."
5 "Not all disk units attached to an advanced function IOA have\
 reported to the system. Retry the operation."
6 "The disk units are missing."
7 "The disk unit/s are currently in use."

$set 232 add_hot_spare_fail
1 "Enable Device as Hot Spare Failed"
2 "There are no disk units eligible for the selected operation\
 due to one or more of the following reasons:"
3 "There are no eligible device units in the system."
4 "An IOA is in a condition that makes the disk units attached to\
 it read/write protected. Examine the kernel messages log\
 for any errors that are logged for the IO subsystem\
 and follow the appropriate procedure for the reference code to\
 correct the problem, if necessary."
5 "Not all disk units attached to an advanced function IOA have\
 reported to the system. Retry the operation."
6 "The disk units are missing."
7 "The disk unit/s are currently in use."

$set 233 remove_hot_spare_fail
1 "Disable Device as Hot Spare Failed"
2 "There are no disk units eligible for the selected operation\
 due to one or more of the following reasons:"
3 "There are no eligible device units in the system."
4 "An IOA is in a condition that makes the disk units attached to\
 it read/write protected. Examine the kernel messages log\
 for any errors that are logged for the IO subsystem\
 and follow the appropriate procedure for the reference code to\
 correct the problem, if necessary."
5 "Not all disk units attached to an advanced function IOA have\
 reported to the system.  Retry the operation."
6 "The disk units are missing."
7 "The disk unit/s are currently in use."

$set 236
1 "Configure a hot spare device"
2 "Select the subsystems which disk units will be configured as hot spares"
3 "Type choice, press Enter."
4 "Select subsystem"
5 "Unconfigure a hot spare device"
6 "Select the subsystems which disk units will be unconfigured as hot spares"
7 "Type choice, press Enter."
8 "Select subsystem"

$set 240 select_hot_spare
1 "Select Disk Units to Enable as Hot Spare"
2 "Type option, press Enter."
3 "Select"
4 "Select Disk Units to Disable as Hot Spare"
5 "Type option, press Enter."
6 "Select"

$set 245 confirm_hot_spare
1 "Confirm Enable Hot Spare"
2 "ATTENTION: Existing data on these disk units will not be preserved."
3 "Press Enter to continue."
4 "Cancel to return and change your choice."
5 "Confirm Disable Hot Spare"
6 "ATTENTION: Existing data on these disk units will not be preserved."
7 "Press Enter to continue."
8 "Cancel to return and change your choice."

$set 300 disk_unit_recovery
1 "Work with Disk Unit Recovery"
2 "Concurrent Add Device"
3 "Concurrent Remove/Replace Device"
4 "Initialize and format disk unit"
5 "Reclaim IOA cache storage"
6 "Rebuild disk unit data"
7 "Work with resources containing cache battery packs"

$set 301 concurrent_add_device
1 "Concurrent Device Add"
2 "Choose a single location for add operations"
3 "Select"

$set 302 concurrent_remove_device
1 "Concurrent Device Remove"
2 "Choose a single location for remove operations"
3 "Select"
4 "Concurrent Device Add"
5 "Choose a single location for add operations"
6 "Select"

$set 303 process_conc_maint
1 "Verify Device Concurrent Remove"
2 "Verify the selected device for concurrent remove operations"
3 "Press Enter when verification complete and ready to continue"
4 "Cancel to return and change your choice."
5 "Verify Device Concurrent Add"
6 "Verify the seleted device for concurrent add operations"
7 "Press Enter when verification complete and ready to continue"
8 "Cancel to return and change your choice."
9 "Complete Device Concurrent Remove"
10 "Remove selected device"
11 "Press Enter when selected device has been removed"
12 "Cancel to return and change your choice."
13 "Complete Device Concurrent Add"
14 "Add device to selected location"
15 "Press Enter when device has been installed"
16 "Cancel to return and change your choice."

$set 312 configure_af_device | init_device
1 "Select Disk Units to format for Advanced Function"
2 "Select Disk Units to format for JBOD Function"
3 "Select Disk Units for Initialize and Format"
4 "Type option, press Enter."
5 "Select"

$set 313
1 "Confirm Initialize and Format Disk Unit"
2 "Press 'c' to confirm your choice for 1=Initialize and format."
3 "Return to change your choice."

$set 314 dev_init_complete
1 "Initialize and Format Status"
2 "You selected to initialize and format a disk unit"
4 "Please wait for the next screen."

$set 315 reclaim_cache
1 "Reclaim IOA Cache Storage"
2 "Select the IOA to reclaim IOA cache storage."
3 "ATTENTION: Proceed with this function only if directed to from a\
 service procedure.  Data in the IOA cache will be discarded.\
 Damaged objects may result on the system."
4 "Type choice, press Enter."
5 "Reclaim IOA cache storage."

$set 316 confirm_reclaim
1 "Confirm Reclaim IOA Cache Storage"
2 "The disk units that may be affected by the function are displayed."
3 "ATTENTION: Proceed with this function only if directed to from a\
 service procedure. Data in the IOA cache will be discarded.\
 Filesystem corruption may result on the system."
4 "Press c=Confirm to reclaim cache storage."
5 "Cancel to return to change your choice."

$set 317 confirm_reclaim_warning
2 "ATTENTION!!!   ATTENTION!!!   ATTENTION!!!   ATTENTION!!!"
3 "ATTENTION: Proceed with this function only if directed to from a\
 service procedure. Data in the IOA cache will be discarded.\
 By proceeding you will be allowing UNKNOWN data loss.\
 This data loss may or may not be detected by the host operating system.\
 Filesystem corruption may result on the system."
4 "Press 's' to continue."

$set 319 reclaim_result
1 "Reclaim IOA Cache Storage Results"
2 "IOA cache storage reclamation has completed. Use the number\
 of lost sectors to decide whether to restore data from the\
 most recent save media or to continue with possible data loss."
3 "Number of lost sectors"
4 "IOA cache storage reclamation has completed.  The number of\
 lost sectors could not be determined."

$set 321 raid_rebuild
1 "Rebuild Disk Unit Data"
2 "Select the units to be rebuilt"
3 "Type choice, press Enter."
4 "Rebuild"

$set 322 raid_rebuild_fail
1 "Rebuild Disk Unit Data Failed"
2 "There are no disk units eligible for the selected operation\
 due to one or more of the following reasons:"
3 "There are no disk units that need to be rebuilt."
4 "The disk unit that needs  to be rebuild is not at the right\
 location. Examine the 'message log' for the exposed unit\
 and make sure that the replacement unit is at the correct\
 location."
5 "Not all disk units attached to an IOA have reported to the\
 system. Retry the operation."
6 "All the disk units in a parity set must be the same capacity\
 with a minumum number of 4 disk units and a maximum of 10 units\
 in the resulting parity set."
7 "The type/model of the disk units is not supported for the\
 requested operation."

$set 323 confirm_raid_rebuild
1 "Confirm Rebuild Disk Unit Data"
2 "Rebuilding the disk unit data may take several minutes for\
 each unit selected."
3 "Press Enter to confirm having the data rebuilt."
4 "Cancel to return and change your choice."

$set 330 battery_maint
1 "Work with Resources Containing Cache Battery Packs"
2 "Type options, press Enter"
3 "Force battery pack into error state"
4 "Display battery information"

$set 341 confirm_force_battery_error
1 "Force Battery Packs Into Error State"
2 "ATTENTION: This service function should be run only under the direction\
 of the IBM Hardware Service Support"
3 "You have selected to force a cache batter error on an IOA"
4 "You will have to replace the Cache Battery Pack in each selected IOA to\
 resume normal operations."
5 "System performance could be significantly degraded until the cache battery\
 packs are replaced on the selected IOAs."
6 "Continue to force the following battery packs into an error state"

$set 332 show_battery_info
1 "Battery Information"
2 "Resource Name"
3 "Serial Number"
4 "Type"
5 "PCI Address"
6 "SCSI Host Number"
7 "Battery type"
8 "Battery state"
9 "Power on time (days)"
10 "Adjusted power on time (days)"
11 "Estimated time to warning (days)"
12 "Estimated time to error (days)"
13 "Resource Path"

21 "Nickel Cadmium (NiCd)"
22 "Nickel Metal Hydride (NiMH)"
23 "Lithium Ion (LiIon)"
24 "Unknown"

31 "No battery warning"
32 "Battery warning issued"
33 "Battery error issued"

$set 401 bus_config
1 "Work with SCSI Bus Configuration"
2 "Select the subsystem to change scsi bus attribute."
3 "Type choice, press Enter."
4 "change scsi bus attribute"

$set 402 bus_config_fail
1 "Work with SCSI Bus Configuration Failed"
2 "There are no SCSI Buses eligible for the selected operation.\
 None of the installed adapters support the requested\
 operation."

$set 403 change_bus_attr
1 "Change SCSI Bus Configuration"
2 "Current Bus configurations are shown.  To change\
 setting hit 'c' for options menu. Hightlight desired\
 option then hit Enter"
3 "Change Setting"
4 "BUS"
5 "QAS Capability"
6 "Host SCSI ID"
7 "Wide Enabled"
8 "Maximum Bus Throughput"
9 "Enable"
10 "Enabled"
11 "Disable"
12 "Disabled"
13 "Yes"
14 "No"

21 "Confirm Change SCSI Bus Configuration"
22 "Confirming this action will result in the following\
 configuration to become active."
23 "Confirm Change SCSI Bus Configration"

$set 458 driver_config
1 "Adapter Driver Configuration"
2 "Select subsystem to change driver configuration"
3 "Type choice, press Enter"
4 "change driver configuration"

$set 459 change_driver_config
1 "Change Driver Configuration"
2 "Current Driver configurations are shown. To change\
 setting, type in new value then hit Enter"
3 "Current Verbosity"
4 "New Verbosity"

$set 500 log_menu
1 "Kernel Messages Log"
2 "View most recent IBM Storage error messages"
3 "View IBM Storage error messages"
4 "View all kernel error messages"
5 "View sisconfig error messages"
6 "Set root kernel message log directory"
7 "Set default editor"
8 "Restore defaults"
9 "View IBM boot time messages"

$set 501 kernel_root
1 "Kernel Messages Log Root Directory"
2 "Enter new directory and press Enter"
3 "Current root directory:"
4 "New root directory:"

$set 502 confirm_kernel_root
1 "Confirm Root Directory Change"
2 "Press 'c' to confirm your new kernel root directory choice."
3 "To return to the log menu without changing the root directory, choose 'q'."
4 "New root directory:"

$set 503 set_default_editor
1 "Kernel Messages Editor"
2 "Enter new editor command and press Enter"
3 "Current editor:"
4 "New editor:"

$set 504 confirm_set_default_editor
1 "Confirm Editor Change"
2 "Press 'c' to confirm your editor choice"
3 "To return to the log menu without changing the editor, choose 'q'."
4 "New editor:"

$set 888 screen status strings
1 ""
2 "Invalid option specified"
3 "Screen paged up"
4 "Already at top"
5 "Screen paged down"
6 "Already at bottom"
10 "No devices to show details for"
15 "The selection is not valid"
16 "Error.  More than one unit was selected."
17 "Invalid option.  No devices selected."
18 "Start Parity Protection completed successfully"
19 "Start Parity Protection failed."
20 "Stop Parity Protection failed"
21 "Stop Parity Protection completed successfully."
25 "Error:  number of devices selected must be a multiple of %d"
26 "Include failed"
27 "Include Unit completed successfully"
28 "Rebuild started, view Parity Status Window for rebuild progress"
29 "Rebuild failed"
30 "Device concurrent maintenance failed"
31 "Device concurrent maintenance failed, device in use"
32 "Device concurrent maintenance completed successfully"
33 "No units available for initialize and format"
34 "Initialize and format completed successfully"
35 "Initialize and format failed"
36 "Reclaim IOA Cache Storage completed successfully"
37 "Reclaim IOA Cache Storage failed"
38 "No Reclaim IOA Cache Storage is necessary"
39 "No Reclaim IOA Cache Storage performed"
40 "Rebuild started, view Parity Status Window for rebuild progress"
41 "Rebuild failed"
42 "The selected battery packs have successfully been placed into an error state."
43 "Attempting to force the selected battery packs into an error state failed."
44 "No configured resources contain a cache battery pack."

45 "Change SCSI Bus configurations completed successfully"
46 "Change SCSI Bus configurations failed"
47 "Change Device Driver Configurations completed successfully"
48 "Change Device Driver Configurations failed"

49 "No units available for initialize and format"
50 "Initialize and format completed successfully"
51 "Initialize and format failed"

52 "No devices available for the selected hot spare operation"
53 "Enable Device as Hot Spare Completed successfully"
54 "Disable Device as Hot Spare Completed successfully"
55 "Configure a hot spare device failed"
56 "Unconfigure a hot spare device failed"

57 "Change Device Driver Configurations completed successfully"
58 "Change Device Driver Configurations failed"

59 "Invalid directory"
60 "Root directory changed to %s"
61 "Root directory unchanged"
62 "Editor changed to %s"
63 "Editor unchanged"
64 "Default log values restored"
65 "Editor returned %d. Try setting the default editor."

$set 999 empty set used for screens without displays
