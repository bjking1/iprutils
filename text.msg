$quote "

$set 0  Generic
1 "Select one of the following"
2 "Selction"

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
3 "%e%q%r%f"

$ Other
101 "Display Disk Array Status"
102 "Parity  Serial          Resource      PCI    PCI   SCSI  SCSI  SCSI  Status\
\n  Set   Number   Model  Name          Bus    Dev    Bus    ID   Lun\
\n#1"
103 "%e%q%r%f"

$set 202 raid_start
$ iSeries
1 "Create a Disk Array"
2 "Select the subsystems to create a disk array.\
\n\
\nType choice, press Enter.\
\n  1=create a disk array\
\n\
\n         Parity  Serial                Resource\
\nOption     Set   Number   Type  Model  Name\
\n#1"
3 "%e%q%f"

$set 203 raid_stop
$ iSeries
1 "Delete a Disk Array"
2 "Select the subsystems to delete a disk array.\
\n\
\nType choice, press Enter.\
\n  1=delete a disk array\
\n\
\n         Parity  Serial                Resource\
\nOption     Set   Number   Type  Model  Name\
\n#1"
3 "%e%q%f"

$set 204 raid_stop_fail
1 "Delete a Disk Array Failed"
2 "There are no disk units eligible for the selected operation\
\ndue to one or more of the following reasons:\
\n\
\no  There are no device parity protected units in the system.\
\no  An IOA is in a condition that makes the disk units attached to\
\n   it read/write protected.  Examine the kernel messages log\
\n   for any errors that are logged for the IO subsystem\
\n   and follow the appropriate procedure for the reference code to\
\n   correct the problem, if necessary.\
\no  Not all disk units attached to an advanced function IOA have\
\n   reported to the system.  Retry the operation.\
\no  The disk units are missing.\
\no  The disk unit/s are currently in use."
3 "%n%e%q"

$set 205 confirm_raid_stop
1 "Confirm Delete a Disk Array"
2 "ATTENTION: Disk units connected to these subsystems will not be\
\nprotected after you confirm your choice.\
\n\
\nPress Enter to continue.\
\nPress q=Cancel to return and change your choice.\
\n\
\n         Parity  Serial                Resource\
\nOption     Set   Number   Type  Model  Name\
\n#1"
3 "%q%f"

$set 206 raid_stop_complete
1 "Stop Device Parity Protection Status"
2 "You selected to stop device parity protection"
3 "%3d%% Complete"

$set 207 raid_start_fail
1 "Create a Disk Array Failed"
2 "There are no disk units eligible for the selected operation\
\ndue to one or more of the following reasons:\
\n\
\no  There are not enough device parity capable units in the system.\
\no  An IOA is in a condition that makes the disk units attached to\
\n   it read/write protected.  Examine the kernel messages log\
\n   for any errors that are logged for the IO subsystem\
\n   and follow the appropriate procedure for the reference code to\
\n   correct the problem, if necessary.\
\no  Not all disk units attached to an advanced function IOA have\
\n   reported to the system.  Retry the operation.\
\no  The disk units are missing.\
\no  The disk unit/s are currently in use."
3 "%n%e%q"

$set 208 configure_raid_start
1 "Select Disk Units for Parity Protection"
2 "Type option, press Enter.\
\n  1=Select\
\n\
\n       Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI\
\nOPT     ID         ID              Number    Bus    Dev    Bus    ID   Lun\
\n#1"
3 "%e%q%f"

$set 209 configure_raid_parameters
1 "Select Protection Level and Stripe Size"
2 "Default array configurations are shown.  To change\
\nsetting hit \"c\" for options menu.  Highlight desired\
\nand option and hit Enter.\
\n\
\nc=Change Setting\
\n\
\n\
\nProtection Level . . . . . . . . . . . . :  RAID 0 -> %6\
\nStripe Size  . . . . . . . . . . . . . . :      64 -> %3 KB"
3 "%n%e%q"

$set 210 confirm_raid_start
1 "Confirm Start Device Parity Protection"
2 "ATTENTION: Data will not be preserved and may be lost on selected disk\
\nunits when parity protection is enabled.\
\n\
\nPress Enter to continue.\
\nPress q=Cancel to return and change your choice.\
\n\
\n         Parity   Vendor    Product           Serial          Resource\
\nOption     Set     ID         ID              Number   Model  Name\
\n#1"
3 "%q%f"

$set 211 raid_start_complete
1 "Start Device Parity Protection Status"
2 "You selected to start device parity protection"
3 "%3d%% Complete"

$set 215 raid_include
1 "Add a Device to a Disk Array"
2 "Select the subsystem that the disk unit will be included.\
\n\
\nType choice, press Enter.\
\n  1=Select Parity Array to Include Disk Unit\
\n\
\n         Parity  Serial                Resource\
\nOption     Set   Number   Type  Model  Name\
\n#1"
3 "%e%q%f"

$ Other
101 "Add a Device to a Disk Array"
102 "Select the subsystem that the disk unit will be included.\
\n\
\nType choice, press Enter.\
\n  1=Select Parity Array to Include Disk Unit\
\n\
\n         Parity  Vendor    Product           Serial          Resource\
\nOption     Set    ID         ID              Number   Model  Name\
\n#1"
103 "%e%q%f"

$set 216 raid_include_fail
1 "Include Device Parity Protection Failed"
2 "There are no disk units eligible for the selected operation\
\ndue to one or more of the following reasons:\
\n\
\no  There are no device parity protected units in the system.\
\no  An IOA is in a condition that makes the disk units attached to\
\n   it read/write protected.  Examine the kernel messages log\
\n   for any errors that are logged for the IO subsystem\
\n   and follow the appropriate procedure for the reference code to\
\n   correct the problem, if necessary.\
\no  Not all disk units attached to an advanced function IOA have\
\n   reported to the system.  Retry the operation.\
\no  The disk units are missing.\
\no  The disk unit/s are currently in use."
3 "%n%e%q"

$set 217 configure_raid_include
1 "Include Disk Units in Device Parity Protection"
2 "Select the units to be included in Device Parity Protection\
\n\
\nType choice, press Enter.\
\n  1=Add a Device to a Disk Array\
\n\
\n           Serial                Resource\
\nOption     Number   Type  Model  Name\
\n#1"
3 "%e%q%f"

$set 218 configure_raid_include_fail

1 "Include Disk Unit Failed"
2 "There are no disk units eligible for the selected operation\
\ndue to one or more of the following reasons:\
\n\
\no  There are not enough disk units available to be included.\
\no  The disk unit that needs to be included is not at the right\
\n   location.  Examine the 'message log' for the exposed unit\
\n   and make sure that the replacement unit is at the correct\
\n   location\
\no  Not all disk units attached to an IOA have reported to the\
\n   system.  Retry the operation.\
#1"
3 "%n%e%q"

4 "\no  The disk unit to be included must be the same or greater\
\n   capacity than the smallest device in the volume set and\
\n   be formatted correctly\
\no  The type/model of the disk units is not supported for the\
\n   requested operation"

5 "\no  All the disk units in a parity set must be the same capacity\
\n   with a minumum number of 4 disk units and a maximum of 10 units\
\n   in the resulting parity set\
\no  The type/model of the disk units is not supported for the\
\n   requested operation"

$set 219 confirm_raid_include
1 "Confirm Disk Units to be Included"
2 "ATTENTION: All listed device units will be formatted and zeroed before\
\nlisted device is added to array.\
\n\
\nPress Enter to confirm your choice to have the system include\
\n  the selected units in device parity protection\
\nPress q=Cancel to return and change your choice.\
\n\
\n         Parity  Serial                Resource\
\nOption     Set   Number   Type  Model  Name\
\n#1"
3 "%e%q%f"

$set 220 confirm_raid_include_device
1 "Confirm Initialize and Format Disk Unit"
2 "Press 'c' to confirm your choice for 1=Initialize and format.\
\nPress q to return to change your choice.\
\n\
\n               Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI\
\nOption  Type   Number     ID    Position   Bus    Dev    Bus    ID   Lun\
\n#1"
3 "c=Confirm   %q%f"

$set 221 dev_include_complete
1 "Include Disk Units in Device Parity Protection Status"
2 "The operation to include units in the device parity protection\
\nwill be done in several phases.  The phases are listed here\
\nand the status will be indicated when known.\
\n\
\nPhase                                      Status\
\n\
\nPrepare to include units . . . . . . . . : %3d %%\
\nInclude units  . . . . . . . . . . . . . : %3d %%"
3 ""

$set 230 af_include_fail
1 "Format Device for Advanced Function Failed"
2 "There are no disk units eligible for the selected operation\
\ndue to one or more of the following reasons:\
\n\
\no  There are no eligible device units in the system.\
\no  An IOA is in a condition that makes the disk units attached to\
\n   it read/write protected.  Examine the kernel messages log\
\n   for any errors that are logged for the IO subsystem\
\n   and follow the appropriate procedure for the reference code to\
\n   correct the problem, if necessary.\
\no  Not all disk units attached to an advanced function IOA have\
\n   reported to the system.  Retry the operation.\
\no  The disk units are missing.\
\no  The disk unit/s are currently in use."
3 "%n%e%q"

$set 231 af_remove_fail
1 "Disable Device for Advanced Functions Failed"
2 "There are no disk units eligible for the selected operation\
\ndue to one or more of the following reasons:\
\n\
\no  There are no eligible device units in the system.\
\no  An IOA is in a condition that makes the disk units attached to\
\n   it read/write protected.  Examine the kernel messages log\
\n   for any errors that are logged for the IO subsystem\
\n   and follow the appropriate procedure for the reference code to\
\n   correct the problem, if necessary.\
\no  Not all disk units attached to an advanced function IOA have\
\n   reported to the system.  Retry the operation.\
\no  The disk units are missing.\
\no  The disk unit/s are currently in use."
3 "%n%e%q"

$set 232 add_hot_spare_fail
1 "Enable Device as Hot Spare Failed"
2 "There are no disk units eligible for the selected operation\
\ndue to one or more of the following reasons:\
\n\
\no  There are no eligible device units in the system.\
\no  An IOA is in a condition that makes the disk units attached to\
\n   it read/write protected.  Examine the kernel messages log\
\n   for any errors that are logged for the IO subsystem\
\n   and follow the appropriate procedure for the reference code to\
\n   correct the problem, if necessary.\
\no  Not all disk units attached to an advanced function IOA have\
\n   reported to the system.  Retry the operation.\
\no  The disk units are missing.\
\no  The disk unit/s are currently in use."
3 "%n%e%q"

$set 233 remove_hot_spare_fail
1 "Disable Device as Hot Spare Failed"
2 "There are no disk units eligible for the selected operation\
\ndue to one or more of the following reasons:\
\n\
\no  There are no eligible device units in the system.\
\no  An IOA is in a condition that makes the disk units attached to\
\n   it read/write protected.  Examine the kernel messages log\
\n   for any errors that are logged for the IO subsystem\
\n   and follow the appropriate procedure for the reference code to\
\n   correct the problem, if necessary.\
\no  Not all disk units attached to an advanced function IOA have\
\n   reported to the system.  Retry the operation.\
\no  The disk units are missing.\
\no  The disk unit/s are currently in use."
3 "%n%e%q"

$set 236
1 "Configure a hot spare device"
2 "Select the subsystems which disk units will be configured as hot spares\
\n\
\nType choice, press Enter.\
\n  1=Select subsystem\
\n\
\n         Vendor    Product            Serial     PCI    PCI\
\nOption    ID         ID               Number     Bus    Dev\
\n#1"
3 "%e%q%f"
4 "Unconfigure a hot spare device"
5 "Select the subsystems which disk units will be unconfigured as hot spares\
\n\
\nType choice, press Enter.\
\n  1=Select subsystem\
\n\
\n         Vendor    Product            Serial     PCI    PCI\
\nOption    ID         ID               Number     Bus    Dev\
\n#1"
6 "%e%q%f"

$set 240 select_hot_spare
1 "Select Disk Units to Enable as Hot Spare"
2 "Type option, press Enter.\
\n 1=Select\
\n\
\n              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI\
\nOPT    Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun\
\n#1"
3 "%e%q%f"
4 "Select Disk Units to Disable as Hot Spare"
5 "Type option, press Enter.\
\n 1=Select\
\n\
\n              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI\
\nOPT    Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun\
\n#1"
6 "%e%q%f"

$set 245 confirm_hot_spare
1 "Confirm Enable Hot Spare"
2 "ATTENTION: Existing data on these disk units will not be preserved.\
\n\
\nPress Enter to continue.\
\nPress q=Cancel to return and change your choice.\
\n\
\n               Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI\
\nOption  Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun\
\n#1"
3 "%q%f"
4 "Confirm Disable Hot Spare"
5 "ATTENTION: Existing data on these disk units will not be preserved.\
\n\
\n\
\nPress Enter to continue.\
\nPress q=Cancel to return and change your choice.\
\n\
\n               Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI\
\nOption  Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun\
\n#1"
6 "%q%f"

$ 3's
$ ------------------------

$set 280 disk_unit_recovery
1 "Work with Disk Unit Recovery"
2 "Select one of the following:\
\n\
\n     1. Concurrent Add Device\
\n     2. Concurrent Remove/Replace Device\
\n     3. Initialize and format disk unit\
\n     4. Reclaim IOA cache storage\
\n     5. Rebuild disk unit data\
\n     6. Work with resources containing cache battery packs\
\n\
\n\
\nSelection:  %1"
3 "%e%q"

$set 290 concurrent_add_device
1 "Concurrent Add Device"
2 "Type option, press Enter.\
\n 1=add device\
\n\
\n     Vendor   Product            Resource             PCI/SCSI          Hardware\
\nOPT   ID        ID               Name                   Loc             Status\
\n#1"
3 "%e%q%r%f"

$set 291 device_concurrent_remove
1 "Concurrent Remove/Replace Device"
2 "Type option, press Enter.\
\n 1=remove/replace device\
\n\
\n     Vendor   Product            Resource             PCI/SCSI          Hardware\
\nOPT   ID        ID               Name                   Loc             Status\
\n#1"
3 "%e%q%r%f"


$set 301 device_concurrent_maintenance

1 "Device Concurrent Maintenance"
2 "Type the choices, then press Enter.\
3 "%n"

$set 302 confirm_device_concurrent_maintenance_remove
1 "Device Concurrent Maintenance Action"
2 "Removal of device\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5\
\n\
\nYou have #6 minute(s) to perform the operation\
\n\
\nDuring this time, your system may seem unresponsive"
3 "c=Confirm   %q"

$set 303 confirm_device_concurrent_maintenance_insert
1 "Device Concurrent Maintenance Action"
2 "Installation of device\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5\
\n\
\nYou have #6 minute(s) to perform the operation\
\n\
\nDuring this time, your system may seem unresponsive"
3 "c=Confirm   %q"


$set 304 concurrent_maintenance_warning_remove
1 "Device Concurrent Maintenance Action Warning"
2 "The device currently located in\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5\
\n\
\nis not parity protected by the IOA. Removal of the device may\
\ncause the system to HANG if it contains storage which is vital\
\nor system operation. Proceed with extreme caution.\
\n\
\nPress r=Proceed with Concurrent remove\
\nPress q=Cancel operation"
3 "%qr=Confirm remove"

$set 305 concurrent_maintenance_warning_insert
$ generic
1 "Device Concurrent Maintenance Action Warning"
2 "The device previously located in:\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5\
\n\
\nwas not parity protected by the IOA. Installation of a new device\
\nin this slot will allow the adapter to discard any existing cache\
\ndata for the previously installed device.\
\n\
\n\
\n\
\n\
\nPress i=Proceed with Concurrent install\
\nPress q=Cancel operation"
3 "%qi=Confirm install"

$set 306 do_dasd_conc_maintenance_remove
$ generic
1 "Device Concurrent Maintenance Action In Progress"
2 "\nPlease remove the device located in:\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5"
3 ""

$set 307 do_dasd_conc_maintenance_insert
$ generic
1 "Device Concurrent Maintenance Action In Progress"
2 "\nPlease insert the device located in:\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5"
3 ""


$set 308 conc_maintenance_complt_remove
$ generic
1 "Concurrent Maintenance Results"
2 "\nTHE CONCURRENT MAINTENANCE PROCEDURE HAS COMPLETED SUCCESSFULLY.\
\n\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5   ACTION = REMOVE\
\n\
\n\
\n\
\n\
\n   Press 'c' to continue."
3 ""

$set 309 conc_maintenance_complt_insert

$ generic
1 "Concurrent Maintenance Results"
2 "\nTHE CONCURRENT MAINTENANCE PROCEDURE HAS COMPLETED SUCCESSFULLY.\
\n\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5   ACTION = INSERT\
\n\
\n\
\n\
\n\
\n   Press 'c' to continue."
3 ""

$ insert and format
4 "Concurrent Maintenance Results"
5 "\nTHE CONCURRENT MAINTENANCE PROCEDURE HAS COMPLETED SUCCESSFULLY.\
\n\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5   ACTION = INSERT\
\n\
\n   In order to make this device useable, it must be formatted.\
\n   Press 'f' to proceed to the format screen.\
\n   Or press 'c' to cancel."
6 ""

$set 310 conc_maintenance_failed_remove
1 "Concurrent Maintenance Results"
2 "\nTHE CONCURRENT MAINTENANCE PROCEDURE HAS FAILED.\
\n\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5   ACTION = REMOVE\
\n\
\n\
\n   Press Enter to continue."
3 ""

$ remove with message 1
4 "Concurrent Maintenance Results"
5 "\nTHE CONCURRENT MAINTENANCE PROCEDURE HAS FAILED.\
\n\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5   ACTION = REMOVE\
\n\
\n Device concurrent maintenance failed, device in use\
\n   Press Enter to continue."
6 ""

$ remove with message 2
7 "Concurrent Maintenance Results"
8 "\nTHE CONCURRENT MAINTENANCE PROCEDURE HAS FAILED.\
\n\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5   ACTION = REMOVE\
\n\
\n Device concurrent maintenance failed, device cannot be removed currently\
\n   Press Enter to continue."
9 ""


$set 311 conc_maintenance_failed_insert
1 "Concurrent Maintenance Results"
2 "\nTHE CONCURRENT MAINTENANCE PROCEDURE HAS FAILED.\
\n\
\n   PCI Bus: #1  PCI Device: #2\
\n   SCSI Channel: #3  SCSI Id: #4  SCSI Lun: #5   ACTION = INSERT\
\n\
\n\
\n   Press Enter to continue."
3 ""

$set 312 configure_af_device | init_device
1 "Select Disk Units for Initialize and Format"
2 "Type option, press Enter.\
\n  1=Select\
\n\
\n       Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI\
\nOPT     ID        ID              Number    Bus    Dev    Bus   ID   Lun\
\n#1"
3 "%e%q%f"

$set 313
1 "Confirm Initialize and Format Disk Unit"
2 "Press 'c' to confirm your choice for 1=Initialize and format.\
\nPress q to return to change your choice.\
\n\
\n        Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI\
\nOption   ID        ID              Number    Bus    Dev    Bus   ID   Lun\
\n#1"
3 "c=Confirm   %q%f"

$set 314 dev_init_complete
1 "Initialize and Format Status"
2 "You selected to initialize and format a disk unit"
3 "%3d%% Complete"

4 "Please wait for the next screen."

$set 315 reclaim_cache
1 "Reclaim IOA Cache Storage"
2 "Select the IOA to reclaim IOA cache storage.\
\n\
\nATTENTION: Proceed with this function only if directed to from a\
\nservice procedure.  Data in the IOA cache will be discarded.\
\nDamaged objects may result on the system.\
\n\
\nType choice, press Enter.\
\n  1=Reclaim IOA cache storage\
\n\
\n                         Serial     Resource\
\nOption    Type   Model   Number     Name"
3 "%e%q"


$set 316 confirm_reclaim
1 "Confirm Reclaim IOA Cache Storage"
2 "The disk units that may be affected by the function are displayed.\
\n\
\nATTENTION: Proceed with this function only if directed to from a\
\nservice procedure.  Data in the IOA cache will be discarded.\
\nFilesystem corruption may result on the system.\
\n\
\nPress c=Confirm to reclaim cache storage.\
\nPress q=Cancel to return to change your choice.\
\n\
\n          Serial                 Resource\
\nOption    Number   Type   Model  Name"
3 "%e%q%f"


$set 317 confirm_reclaim_warning
1 ""
2 "ATTENTION!!!   ATTENTION!!!   ATTENTION!!!   ATTENTION!!!\
\n\
\nATTENTION: Proceed with this function only if directed to from a\
\nservice procedure.  Data in the IOA cache will be discarded.\
\nBy proceeding you will be allowing UNKNOWN data loss.\
\nThis data loss may or may not be detected by the host operating system.\
\nFilesystem corruption may result on the system.\
\n\
\nPress 's' to continue.\
\n          Serial                 Resource\
\nOption    Number   Type   Model  Name"
3 "%qs=Confirm reclaim    %f"

$set 319 reclaim_result
1 "Reclaim IOA Cache Storage Results"
2 "IOA cache storage reclamation has completed.  Use the number\
\nof lost sectors to decide whether to restore data from the\
\nmost recent save media or to continue with possible data loss.\
\n\
\nNumber of lost sectors . . . . . . . . . . : #1"
3 "%n%e%q"

4 "Reclaim IOA Cache Storage Results"
5 "IOA cache storage reclamation has completed.  The number of\
\nlost sectors could not be determined."
6 "%n%e%q"

$set 321 raid_rebuild
1 "Rebuild Disk Unit Data"
2 "Select the units to be rebuilt\
\n\
\nType choice, press Enter.\
\n  1=Rebuild\
\n\
\n         Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI\
\nOption    ID         ID              Number    Bus    Dev    Bus    ID   Lun\
\n#1"
3 "%e%q%f"

$set 322 raid_rebuild_fail
1 "Rebuild Disk Unit Data Failed"
2 "There are no disk units eligible for the selected operation\
\ndue to one or more of the following reasons:\
\no  There are no disk units that need to be rebuilt.\
\no  The disk unit that needs  to be rebuild is not at the right\
\n   location.  Examine the 'message log' for the exposed unit\
\n   and make sure that the replacement unit is at the correct\
\n   location.\
\no  Not all disk units attached to an IOA have reported to the\
\n   system.  Retry the operation.\
\no  All the disk units in a parity set must be the same capacity\
\n   with a minumum number of 4 disk units and a maximum of 10 units\
\n   in the resulting parity set.\
\no  The type/model of the disk units is not supported for the\
\n   requested operation."
3 "%n%e%q"

$set 323 confirm_raid_rebuild
1 "Confirm Rebuild Disk Unit Data"
2 "Rebuilding the disk unit data may take several minutes for\
\neach unit selected.\
\n\
\nPress Enter to confirm having the data rebuilt.\
\nPress q=Cancel to return and change your choice.\
\n\
\n        Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI\
\nOption   ID         ID              Number    Bus    Dev    Bus    ID   Lun\
\n#1"
3 "%q%f"

$set 330 battery_maint
1 "Work with Resources Containing Cache Battery Packs"
2 "Type options, press Enter\
\n  2=Force battery pack into error state    5=Display battery information\
\n\
\n    Resource                               PCI  PCI   SCSI\
\nOpt Name          Serial Number Type-Model Bus Device Host\
\n#1"
3 "%e%q"

$set 331 confirm_force_battery_error
1 "Force Battery Packs Into Error State"
2 "ATTENTION: This service function should be run only under the direction\
\nof the IBM Hardware Service Support\
\n\
\nYou have selected to force a cache batter error on an IOA\
\n\
\nYou will have to replace the Cache Battery Pack in each selected IOA to\
\nresume normal operations.\
\n\
\nSystem performance could be significantly degraded until the cache battery\
\npacks are replaced on the selected IOAs.\
\n\
\nPress c=Continue to force the following battery packs into an error state\
\n\
\n    Resource                               PCI  PCI   SCSI\
\nOpt Name          Serial Number Type-Model Bus Device Host\
\n#1"
3 "%e%q%f"

$set 332 show_battery_info
1 "Battery Information"
2 "Resource Name . . . . . . . . . . . : #1\
\nSerial Number . . . . . . . . . . . : #2\
\nType  . . . . . . . . . . . . . . . : #3\
\nPCI Address . . . . . . . . . . . . : #4\
\nSCSI Host Number  . . . . . . . . . : #6\
\nBattery type  . . . . . . . . . . . : #7\
\nBattery state . . . . . . . . . . . : #8\
\nPower on time (days)  . . . . . . . : #9\
\nAdjusted power on time (days) . . . : #10\
\nEstimated time to warning (days)  . : #11\
\nEstimated time to error (days)  . . : #12"
3 "%n%e%q"

4 "Nickel Cadmium (NiCd)"
5 "Nickel Metal Hydride (NiMH)"
6 "Lithium Ion (LiIon)"
7 "Unknown"
8 "No battery warning"
9 "Battery warning issued"
10 "Battery error issued"


$ 4's
$ ------------------------

$set 401 bus_config
1 "Work with SCSI Bus Configuration"
2 "Select the subsystem to change scsi bus attribute.\
\n\
\nType choice, press Enter.\
\n  1=change scsi bus attribute\
\n\
\n         Vendor    Product            Serial     PCI    PCI\
\nOption    ID         ID               Number     Bus    Dev\
\n#1"
3 "%e%q%f"

$set 402 bus_config_fail
1 "Work with SCSI Bus Configuration Failed"
2 "There are no SCSI Buses eligible for the selected operation.\
\nNone of the installed adapters support the requested\
\noperation."
3 "%n%e%q"

$set 403 change_bus_attr
1 "Change SCSI Bus Configuration"
2 "Current Bus configurations are shown.  To change\
\nsetting hit \"c\" for options menu.  Hightlight desired\
\noption then hit Enter\
\n\
\nc=Change Setting\
\n\
\n#1"
3 "%n%e%q%f"

$set 404 confirm_bus_config
1 "Confirm Change SCSI Bus Configuration"
2 "Confirming this action will result in the following configuration to\
\nbecome active.\
\n\
\n\
\n\
\nPress c=Confirm Change SCSI Bus Configuration\
\n\
\n#1"
3 "c=Confirm   %e%q%f"


$set 405
1 "Confirm Change SCSI Bus Configuration"
2 "Confirming this action will result in the following configuration to\
\nbecome active.\
\nATTENTION:  This action requires a reset of the Host Adapter.  Confirming\
\nthis action may affect system performance while processing changes.\
\n\
\nPress c=Confirm Change SCSI Bus Configuration\
\n\
\n#1"
3 "c=Confirm   %e%q%f"


$set 406 processing_bus_config
1 "Processing Change SCSI Bus Configuration"
2 "Please wait for next screen"
3 ""

$set 459 driver_config
1 "Change Driver Configuration"

2 " Current Driver configurations are shown.  To change\
\n setting hit "c" for options menu.  Highlight desired\
\n option then hit Enter\
\n\
\n c=Change Setting\
\n\
\n\
\n    Verbosity  . . . . . . . . . . . . . :  %m"
3 "%n%e%q"


$ 5's
$ ------------------------

$set 500 log_menu
1 "Kernel Messages Log"
2 "Select one of the following:\
\n\
\n     1. Use #1 to view most recent IBM Storage error messages\
\n     2. Use #1 to view IBM Storage error messages\
\n     3. Use #1 to view all kernel error messages\
\n     4. Use #1 to view sisconfig error messages\
\n     5. Set root kernel message log directory\
\n     6. Set default editor\
\n     7. Restore defaults\
\n     8. Use #1 to view IBM boot time messages\
\n\
\nSelection:  %1"
3 "%e%q"

$ used when IBM boot message log does not exist
4 "Kernel Messages Log"
5 "Select one of the following:\
\n\
\n     1. Use #1 to view most recent IBM Storage error messages\
\n     2. Use #1 to view IBM Storage error messages\
\n     3. Use #1 to view all kernel error messages\
\n     4. Use #1 to view sisconfig error messages\
\n     5. Set root kernel message log directory\
\n     6. Set default editor\
\n     7. Restore defaults\
\n\
\n\
\nSelection:  %1"
6 "%e%q"

$set 501 kernel_root
1 "Kernel Messages Log Root Directory"
2 "\n\
\nEnter new directory and press Enter\
\n\
\n\
\nCurrent root directory: #1\
\nNew root directory:     %39"
3 "%n"

$set 502 confirm_kernel_root
1 "Confirm Root Directory Change"
2 "\nPress \"c\" to confirm your new kernel root directory choice.\
\nTo return to the log menu without changing the root directory, choose \"q\".\
\n\
\n\
\nNew root directory:     #1"
3 "c=Confirm   %q"

$set 503 set_default_editor
1 "Kernel Messages Editor"
2 "\n\
\nEnter new editor command and press Enter\
\n\
\n\
\nCurrent editor: #1\
\nNew editor:     %39"
3 "%n"

$set 504 confirm_set_default_editor
1 "Confirm Editor Change"
2 "\nPress \"c\" to confirm your editor choice,\
\nTo return to the log menu without changing the editor, choose \"q\".\
\n\
\n\
\nNew editor:     #1"
3 "c=Confirm   %q"

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

$set 900
1 "Menu Test Screen"
2 "This is test of the emergency menu system\
\nUnder normal conditions, this screen would be\
\nfollowed by bla bla bla...\
\n\
\nHit 'c' to choose one:\
\n\
\nOption1 .  .  .  .  .  .  .  .  .  .  .  : >%m<menu\
\n\
\nOption2 .  .  .  .  .  .  .  .  .  .  .  : %m\
\n field length 2 -->%2<--"
3 "%n%e%q%f"

$set 999 empty set used for screens without displays
