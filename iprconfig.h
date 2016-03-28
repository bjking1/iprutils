#ifndef iprconfig_h
#define iprconfig_h
/**
 * IBM IPR adapter configuration utility
 *
 * (C) Copyright 2004
 * International Business Machines Corporation and others.
 * All Rights Reserved. This program and the accompanying
 * materials are made available under the terms of the
 * Common Public License v1.0 which accompanies this distribution.
 *
 **/

#define _(string) (string)
#define __(string) (string)
#define EXIT_FLAG		0x8000	/* stops at given screen on exit call */
#define CANCEL_FLAG		0x4000	/* stops at given screen on quit call */
#define REFRESH_FLAG		0x2000	/* refreshes screen on quit or exit */
#define TOGGLE_FLAG		0x1000
#define FWD_FLAG		0x0800
#define CONFIRM_FLAG		0x0400
#define CONFIRM_REC_FLAG	0x0200
#define MENU_FLAG		0x0100
#define ENTER_FLAG		0x0080

#define NUM_OPTS(x) (sizeof(x)/sizeof(struct screen_opts))

#define REFRESH_SCREEN		-13
#define TOGGLE_SCREEN		-14

#define INVALID_OPTION_STATUS	2
#define PGUP_STATUS		3
#define TOP_STATUS		4
#define PGDN_STATUS		5
#define BTM_STATUS		6

#define MAX_FIELD_SIZE		39

typedef struct info_container i_container;

struct info_container {
	i_container *next_item;	/* reference to next info_container */
	char field_data[MAX_FIELD_SIZE + 1]; /* stores characters entered into a user-entry field */
	void *data;		/* stores a field pointer */
	int y;			/* cursor y position of user selection */
	int x;			/* cursor x position of user selection */
};

#define for_each_icon(icon) for (icon = i_con_head; icon; icon = icon->next_item)

struct screen_output {
	int rc;
	i_container *i_con;
};

typedef struct function_output fn_out;

struct function_output {
	fn_out *next;		/* reference to next fn_out if >1 */
	int index;		/* used to plop text into # spots on text.msg */
	int cat_offset;		/* varies text set used in text.msg -> esp dif platforms */
	char *text;		/* holds "special" data to be passed back to the screen */
};

int main_menu(i_container * i_con);
int config_menu(i_container *i_con);

int disk_status(i_container * i_con);
int device_details(i_container * i_con);
int path_details(i_container * i_con);
int raid_screen(i_container * i_con);
int raid_status(i_container * i_con);
int raid_stop(i_container * i_con);
int confirm_raid_stop(i_container * i_con);
int do_confirm_raid_stop(i_container * i_con);
int raid_start(i_container * i_con);
int raid_start_loop(i_container * i_con);
int configure_raid_start(i_container * i_con);
int configure_raid_parameters(i_container * i_con);
int confirm_raid_start(i_container * i_con);
int raid_start_complete();
int raid_include(i_container * i_con);
int configure_raid_include(i_container * i_con);
int confirm_raid_include(i_container * i_con);
int confirm_raid_include_device(i_container * i_con);
int disk_unit_recovery(i_container * i_con);

int concurrent_add_device(i_container *i_con);
int concurrent_remove_device(i_container *i_con);
int verify_conc_maint(i_container * i_con);

int path_status(i_container * i_con);
int init_device(i_container * i_con);
int confirm_init_device(i_container * i_con);
int send_dev_inits(i_container * i_con);
int reclaim_cache(i_container * i_con);
int confirm_reclaim(i_container * i_con);
int reclaim_warning(i_container * i_con);
int reclaim_result(i_container * i_con);
int af_include(i_container * i_con);
int af_remove(i_container * i_con);
int hot_spare_screen(i_container *i_con);
int add_hot_spare(i_container * i_con);
int remove_hot_spare(i_container * i_con);
int hot_spare(i_container * i_con, int action);

int select_hot_spare(i_container * i_con, int action);
int confirm_hot_spare(int action);
int hot_spare_complete(int action);

int raid_migrate(i_container * i_con);
int asym_access(i_container *i_con);
int asym_access_menu(i_container * i_con);

int raid_rebuild(i_container * i_con);
int confirm_raid_rebuild(i_container * i_con);
int raid_resync(i_container * i_con);
int confirm_raid_resync(i_container * i_con);

int battery_maint(i_container * i_con);
int enclosures_maint(i_container * i_con);
int battery_fork(i_container * i_con);
int enclosures_fork(i_container * i_con);
int force_battery_error(i_container *i_con);
int enable_battery(i_container *i_con);
int ipr_suspend_disk_enclosure(i_container *i_con);
int ipr_resume_disk_enclosure(i_container *i_con);
int bus_config(i_container * i_con);
int change_bus_attr(i_container * i_con);
int confirm_change_bus_attr(i_container *i_con);
int driver_config(i_container * i_con);
int change_driver_config(i_container *i_con);
int disk_config(i_container * i_con);
int ioa_config(i_container * i_con);
int change_disk_config(i_container *);
int change_ioa_config(i_container *);
int ucode_screen(i_container *i_con);
int download_ucode(i_container *);
int download_all_ucode(i_container *);
int choose_ucode(i_container *);
int log_menu(i_container *);
int ibm_storage_log_tail(i_container *);
int ibm_storage_log(i_container *);
int kernel_log(i_container *);
int iprconfig_log(i_container *);
int kernel_root(i_container *);
int confirm_kernel_root(i_container *);
int confirm_kernel_root_change(i_container *);
int confirm_kernel_root_change2(i_container *);
int set_default_editor(i_container *);
int confirm_set_default_editor(i_container *);
int confirm_set_default_editor_change(i_container *);
int confirm_set_default_editor_change2(i_container *);
int restore_log_defaults(i_container *);
int ibm_boot_log(i_container *);
int exit_confirmed(i_container *);

int statistics_menu(i_container *);
int device_stats(i_container *);

static int raid_create_check_num_devs(struct ipr_array_cap_entry *, int, int);

/* constant strings */
const char *no_dev_found = __("No devices found");
const char *wait_for_next_screen = __("Please wait for the next screen.");

/* others */
int menu_test(i_container * i_con);
ITEM **menu_test_menu(int field_index);

typedef struct screen_node s_node;

struct screen_opts {
	int (*screen_function) (i_container *);
	char *key;
	char *list_str;
};

struct screen_node {
	int rc_flags;
	int f_flags;
	int num_opts;
	struct screen_opts *options;
	char *title;
	char *body;
	char *header[10];
};

struct screen_opts null_opt[] = {
	{NULL, "\n"}
};

struct screen_opts main_menu_opt[] = {
	{disk_status,        "1", __("Display hardware status")},
	{raid_screen,        "2", __("Work with disk arrays")},
	{disk_unit_recovery, "3", __("Work with disk unit recovery")},
	{config_menu,        "4", __("Work with configuration options")},
	{ucode_screen,	     "5", __("Work with microcode updates")},
	{statistics_menu,    "6", __("Devices Statistics")},
	{log_menu,           "7", __("Analyze log")}
};

s_node n_main_menu = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG),
	.num_opts = NUM_OPTS(main_menu_opt),
	.options  = &main_menu_opt[0],
	.title    =  __("IBM Power RAID Configuration Utility")
};

struct screen_opts exit_confirm_opt[] = {
	{main_menu,        "1", __("Return to main menu")},
	{exit_confirmed,     "2", __("Exit iprconfig")},
};

s_node n_exit_confirm = {
	.num_opts = NUM_OPTS(exit_confirm_opt),
	.options  = &exit_confirm_opt[0],
	.title    =  __("Confirm Exit"),
	.header   = {
		__("One or more disks is currently known to be zeroed. Exiting now "
		   "will cause this state to be lost, resulting in longer array creation times\n\n"),
		"" }
};


struct screen_opts config_menu_opt[] = {
	{bus_config,         "1", __("Work with SCSI bus configuration")},
	{driver_config,      "2", __("Work with driver configuration")},
	{disk_config,        "3", __("Work with disk configuration")},
	{ioa_config,         "4", __("Work with adapter configuration")},
};

s_node n_config_menu = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(config_menu_opt),
	.options  = &config_menu_opt[0],
	.title    =  __("IBM Power RAID Configuration Utility")
};

struct screen_opts disk_status_opt[] = {
	{device_details, "\n"}
};

s_node n_disk_status = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(disk_status_opt),
	.options  = &disk_status_opt[0],
	.title    = __("Display Hardware Status"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Display hardware resource information details\n\n"),
		"" }
};

struct screen_opts device_stats_opt[] = {
	{device_stats, "\n"}
};

s_node n_device_stats = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(device_stats_opt),
	.options  = &device_stats_opt[0],
	.title    = __("Display Device Statistics"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Display device statistics\n\n"),
		"" }
};

s_node n_adapter_details = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG | FWD_FLAG),
	.title    = __("IOA Hardware Resource Information Details")
};

s_node n_device_details = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG | FWD_FLAG),
	.title    = __("Disk Unit Hardware Resource Information Details")
};

s_node n_vset_details = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG | FWD_FLAG),
	.title    = __("Disk Array Information Details")
};

s_node n_ses_details = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG | FWD_FLAG),
	.title    = __("Disk Enclosure Information Details")
};

struct screen_opts raid_screen_opt[] = {
	{raid_status,      "1", __("Display disk array status")},
	{raid_start,       "2", __("Create a disk array")},
	{raid_stop,        "3", __("Delete a disk array")},
	{raid_include,     "4", __("Add a device to a disk array")},
	{af_include,       "5", __("Format device for RAID function")},
	{af_remove,        "6", __("Format device for JBOD function")},
	{hot_spare_screen, "7", __("Work with hot spares")},
	{asym_access,      "8", __("Work with asymmetric access")},
	{raid_resync,      "9", __("Force RAID Consistency Check")},
	{raid_migrate,     "0", __("Migrate disk array protection")},
};

struct screen_opts hot_spare_opt[] = {
	{add_hot_spare,    "1", __("Create a hot spare")},
	{remove_hot_spare, "2", __("Delete a hot spare")},
};


s_node n_asym_access = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Array Asymmetric Access"),
	.header   = {
		__("Select the disk array path.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=change asymmetric access for a disk array\n\n"),
 		"" }
};

s_node n_asym_access_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Setting Array Asymmetric Access Failed"),
	.header   = {
		__("There are no arrays eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o Active/Active mode is not enabled on the IOAs.\n"),
		__("o An IOA needs updated microcode in order to support "
		   "active/active configurations.\n"),
		__("o None of the disk arrays in the system are capable of "
		   "changing asymmetric access attributes.\n"),
		__("o There are no disk arrays in the system.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference "
		   "code to correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		"" }
};

struct screen_opts change_array_asym_access_opt[] = {
	{asym_access_menu, "c"},
	{NULL, "\n"}
};

s_node n_change_array_asym_access = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG | FWD_FLAG | MENU_FLAG),
	.num_opts = NUM_OPTS(change_array_asym_access_opt),
	.options  = &change_array_asym_access_opt[0],
	.title    = __("Change Asymmetric Access Configuration of Array"),
	.header   = {
		__("Current array asymmetric access configuration is shown. To "
		   "change setting hit 'c' for options menu. Highlight "
		   "desired option then hit Enter.\n"),
		__("  c=Change Setting\n\n"),
		"" }
};

s_node n_raid_migrate_complete = {
	.title    = __("Migrate Disk Array Status"),
	.body     = __("You selected to migrate a disk array")
};

s_node n_confirm_raid_migrate = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Migrate a Disk Array"),
	.header   = {
		__("ATTENTION: Disk array will be migrated.\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

s_node n_raid_migrate = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG ),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Migrate Disk Array Protection"),
	.header   = {
		__("Select only one disk array for migration.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=migrate protection for a disk array\n\n"),
 		"" }
};

s_node n_raid_migrate_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Disk Array Migration Failed"),
	.header   = {
		__("There are no arrays eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no disk arrays in the system.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference "
		   "code to correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o There are not enough unused AF disks for the migration.\n"),
		"" }
};

s_node n_raid_migrate_add_disks = {
	.rc_flags = (CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Select Disk Units for Migration"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_hot_spare_screen = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(hot_spare_opt),
	.options  = &hot_spare_opt[0],
	.title    = __("Work with Hot Spares")
};

s_node n_raid_screen = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(raid_screen_opt),
	.options  = &raid_screen_opt[0],
	.title    = __("Work with Disk Arrays")
};

s_node n_raid_status = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(disk_status_opt),
	.options  = &disk_status_opt[0],
	.title    = __("Display Disk Array Status"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Display hardware resource information details\n\n"),
		"" }
};

struct screen_opts raid_stop_opt[] = {
	{confirm_raid_stop, "\n"}
};

s_node n_raid_stop = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG ),
	.num_opts = NUM_OPTS(raid_stop_opt),
	.options  = &raid_stop_opt[0],
	.title    = __("Delete a Disk Array"),
	.header   = {
		__("Select the disk array(s) to delete.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=delete a disk array\n\n"),
		"" }
};

s_node n_raid_stop_fail = {
	.title    = __("Delete a Disk Array Failed"),
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.header   = {
		__("There are no disks eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no disk arrays in the system.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disks are missing.\n"),
		"" }
};

struct screen_opts confirm_raid_stop_opt[] = {
	{do_confirm_raid_stop, "\n"}
};

s_node n_confirm_raid_stop = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(confirm_raid_stop_opt),
	.options  = &confirm_raid_stop_opt[0],
	.title    = __("Confirm Delete a Disk Array"),
	.header   = {
		__("ATTENTION: Disk array will be deleted.\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

struct screen_opts raid_start_opt[] = {
	{raid_start_loop, "\n"}
};

s_node n_raid_start = {
	.rc_flags = (CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(raid_start_opt),
	.options  = &raid_start_opt[0],
	.title    = __("Create a Disk Array"),
	.header   = {
		__("Select the adapter.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=create a disk array\n\n"),
		"" }
};

s_node n_raid_start_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Create a Disk Array Failed"),
	.header   = {
		__("There are no disks eligible for the selected operation due "
		   "to one or more of the following reasons:\n\n"),
		__("o There are not enough advanced function disks in the system.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disks are missing.\n"),
		"" }
};

s_node n_configure_raid_start = {
	.rc_flags = (CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Select Disk Units for Disk Array"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_confirm_raid_start = {
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Create Disk Array"),
	.header   = {
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

s_node n_raid_start_complete = {
	.title    = __("Create Disk Array Status"),
	.body     = __("You selected to create a disk array")
};

struct screen_opts raid_include_opt[] = {
	{configure_raid_include, "\n"}
};

s_node n_raid_include = {
	.rc_flags = (CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(raid_include_opt),
	.options  = &raid_include_opt[0],
	.title    = __("Add Devices to a Disk Array"),
	.header   = {
		__("Select the disk array.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Select disk array\n\n"),
		"" }
};

s_node n_raid_include_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Add Devices to a Disk Array Failed"),
	.header   = {
		__("There are no disks eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no disk arrays in the system.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference "
		   "code to correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have"
		   "reported to the system. Retry the operation.\n"),
		__("o The disks are missing.\n"),
		__("o IO adapter does not support add function.\n"),
		"" }
};

s_node n_configure_raid_include = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options = &null_opt[0],
	.title   = __("Add Devices to a Disk Array"),
	.header  = {
		__("Select the devices to be included in the disk array\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Add a Device to a Disk Array\n\n"),
		   "" }
};

s_node n_configure_raid_include_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Add Devices to a Disk Array Failed"),
	.header = {
		__("There are no disks eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are not enough disks available to be included.\n"),
		__("o Not all disks attached to an IOA have reported to the "
		   "system. Retry the operation.\n"),
		__("o The disk to be included must be the same or greater "
		   "capacity than the smallest device in the disk array and "
		   "be formatted correctly\n"),
		__("o The disk is not supported for the requested operation\n"),
		"" }
};

s_node n_confirm_raid_include = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Devices to Add to a Disk Array"),
	.header   = {
		__("All listed disks will be added to the disk array.\n"),
		__("ATTENTION: Disks that have not been zeroed with be formatted first.\n"),
		__("Press Enter to confirm your choice to have the system "
		   "include the selected disks in the disk array\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

s_node n_dev_include_complete = {
	.title    = __("Add Devices to a Disk Array Status"),
	.header   = {
		__("The operation to add disks to the disk array "
		   "will be done in two phases. The disks will first be "
		   "formatted, then added to the disk array."),
		__("You selected to add disks to a disk array"),
		""
	}
};

s_node n_af_include_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Format Device for RAID Function Failed"),
	.header = {
		__("There are no disks eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no eligible disks in the system.\n"),
		__("o All disks are already formatted for RAID Function.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disks are missing.\n"),
		"" }
};

s_node n_af_remove_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Format Device for JBOD Function (512) Failed"),
	.header   = {
		__("There are no disks eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no eligible disks in the system.\n"),
		__("o All disks are already formatted for JBOD function.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disks are missing.\n"),
		"" }
};

s_node n_add_hot_spare_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Create Hot Spare Failed"),
	.header = {
		__("There are no disks eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no eligible disks in the system.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disks are missing.\n"),
		"" }
};

s_node n_remove_hot_spare_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Delete Hot Spare Failed"),
	.header = {
		__("There are no disks eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no eligible disks in the system.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have "
		   "reported to the system.  Retry the operation.\n"),
		__("o The disks are missing.\n"),
		"" }
};

s_node n_add_hot_spare = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Create a Hot Spare Device"),
	.header   = {
		__("Select the adapter on which disks will be "
		   "configured as hot spares\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Select adapter\n\n"),
		"" },
};

s_node n_remove_hot_spare = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Delete a Hot Spare Device"),
	.header   = {
		__("Select the adapter on which hot spares will be deleted\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Select adapter\n\n"),
		"" },
};/* xxx search for subsystem and disk unit and replace all */

s_node n_select_add_hot_spare = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Select Disks to Create Hot Spares"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_select_remove_hot_spare = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Select Hot Spares to Delete"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_confirm_add_hot_spare = {
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Create Hot Spare"),
	.header   = {
		__("ATTENTION: Existing data on these disks "
		   "will not be preserved.\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" },
};

s_node n_confirm_remove_hot_spare = {
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Delete Hot Spare"),
	.header   = {
		__("ATTENTION: Selected disks will no longer be available "
		   "as hot spare devices.\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

struct screen_opts disk_unit_recovery_opt[] = {
	{concurrent_add_device,    "1", __("Concurrent add device")},
	{concurrent_remove_device, "2", __("Concurrent remove device")},
	{init_device,              "3", __("Initialize and format disk")},
	{reclaim_cache,            "4", __("Reclaim IOA cache storage")},
	{raid_rebuild,             "5", __("Rebuild disk unit data")},
	{raid_resync,              "6", __("Force RAID Consistency Check")},
	{battery_maint,            "7", __("Work with resources containing cache battery packs")},
	{path_status,              "8", __("Display SAS path status")},
	{enclosures_maint,              "9", __("Work with disk enclosures")}
};

s_node n_disk_unit_recovery = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(disk_unit_recovery_opt),
	.options  = &disk_unit_recovery_opt[0],
	.title    = __("Work with Disk Unit Recovery")
};

#define IPR_CONC_REMOVE        1
#define IPR_CONC_ADD           2
#define IPR_VERIFY_CONC_REMOVE 3
#define IPR_VERIFY_CONC_ADD    4
#define IPR_WAIT_CONC_REMOVE   5
#define IPR_WAIT_CONC_ADD      6
#define IPR_CONC_IDENTIFY      7
s_node n_concurrent_remove_device = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Concurrent Device Remove"),
	.header   = {
		__("Choose a single location for remove operations\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_concurrent_add_device = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Concurrent Device Add"),
	.header   = {
		__("Choose a single location for add operations\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_verify_conc_remove = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Verify Device Concurrent Remove"),
	.header   =  {
		__("Verify the selected device for concurrent remove operations\n\n"),
		__("Press Enter when verification complete and ready to continue\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

s_node n_verify_conc_add = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Verify Device Concurrent Add"),
	.header   =  {
		__("Verify the selected device for concurrent add operations\n\n"),
		__("Press Enter when verification complete and ready to continue\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

s_node n_wait_conc_remove = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Complete Device Concurrent Remove"),
	.header   =  {
		__("Remove selected device\n\n"),
		__("Press Enter when selected device has been removed\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

s_node n_wait_conc_add = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Complete Device Concurrent Add"),
	.header   =  {
		__("Add selected device\n\n"),
		__("Press Enter when selected device has been installed\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

struct screen_opts init_device_opt[] = {
	{confirm_init_device, "\n"}
};

s_node n_af_init_device = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(init_device_opt),
	.options  = &init_device_opt[0],
	.title    = __("Select Disks to format for RAID Function"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_jbod_init_device = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(init_device_opt),
	.options  = &init_device_opt[0],
	.title    = __("Select Disks to format for JBOD Function (512)"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_init_device = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(init_device_opt),
	.options  = &init_device_opt[0],
	.title    = __("Select Disks for Initialize and Format"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

struct screen_opts confirm_inits_opt[] = {
	{send_dev_inits, "c"}
};

s_node n_confirm_init_device = {
	.f_flags  = (CONFIRM_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(confirm_inits_opt),
	.options  = &confirm_inits_opt[0],
	.title    = __("Confirm Initialize and Format Disks"),
	.header   = {
		__("Press 'c' to confirm your choice for 1=Initialize and format.\n"),
		__("  q=Return to change your choice.\n\n"),
		"" }
};

s_node n_dev_init_complete = {
	.title    = __("Initialize and Format Status"),
	.body     = __("You selected to initialize and format a disk")
};

struct screen_opts reclaim_cache_opts[] = {
	{confirm_reclaim, "\n"}
};

s_node n_reclaim_cache = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG),
	.num_opts = NUM_OPTS(reclaim_cache_opts),
	.options  = &reclaim_cache_opts[0],
	.title    = __("Reclaim IOA Cache Storage"),
	.header   = {
		__("Select the IOA to reclaim IOA cache storage.\n"),
		__("ATTENTION: Proceed with this function only if directed "
		   "to from a service procedure.  Data in the IOA cache "
		   "will be discarded.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Reclaim IOA cache storage.\n\n"),
		"" }
};

struct screen_opts confirm_reclaim_opt[] = {
	{reclaim_warning, "c"}
};

s_node n_confirm_reclaim = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(confirm_reclaim_opt),
	.options  = &confirm_reclaim_opt[0],
	.title    = __("Confirm Reclaim IOA Cache Storage"),
	.header   = {
		__("The disks that may be affected by the function are displayed.\n"),
		__("ATTENTION: Proceed with this function only if directed to from a "
		   "service procedure. Data in the IOA cache will be discarded. "
		   "Filesystem corruption may result on the system.\n\n"),
		__("Press c=Confirm to reclaim cache storage.\n"),
		__("  q=Cancel to return to change your choice.\n\n"),
		"" }
};

struct screen_opts confirm_reclaim_warning_opt[] = {
	{reclaim_result, "s"}
};

s_node n_confirm_reclaim_warning = {
	.f_flags  = (CONFIRM_REC_FLAG | CANCEL_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(confirm_reclaim_warning_opt),
	.options  = &confirm_reclaim_warning_opt[0],
	.title    = __("Confirm Reclaim IOA Cache Storage"),
	.header   = {
		__("ATTENTION!!!   ATTENTION!!!   ATTENTION!!!   ATTENTION!!!\n"),
		__("ATTENTION: Proceed with this function only if directed to from a "
		   "service procedure. Data in the IOA cache will be discarded. "
		   "This data loss may or may not be detected by the host operating "
		   "system. Filesystem corruption may result on the system.\n\n"),
		__("Press 's' to continue.\n\n"),
		"" }
};

s_node n_reclaim_result = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Reclaim IOA Cache Storage Results")
};

struct screen_opts raid_rebuild_opt[] = {
	{confirm_raid_rebuild, "\n"}
};

s_node n_raid_rebuild = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(raid_rebuild_opt),
	.options  = &raid_rebuild_opt[0],
	.title    = __("Rebuild Disk Unit Data"),
	.header   = {
		__("Select the disks to be rebuilt\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Rebuild\n\n"),
		"" }
};

s_node n_raid_rebuild_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Rebuild Disk Unit Data Failed"),
	.header   = {
		__("There are no disks eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no disks that need to be rebuilt.\n"),
		__("o The disk that needs to be rebuilt is not at the right "
		   "location. Examine the 'message log' for the exposed unit "
		   "and make sure that the replacement unit is at the correct "
		   "location.\n"),
		__("o Not all disks attached to an IOA have reported to the "
		   "system. Retry the operation.\n"),
		__("o The disk is not supported for the requested operation.\n"),
		"" }
};

s_node n_confirm_raid_rebuild = {
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Rebuild Disk Unit Data"),
	.header   = {
		__("Rebuilding the disk unit data may take several "
		   "minutes for each disk selected.\n\n"),
		__("Press Enter to confirm having the data rebuilt.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" },
};

s_node n_raid_resync_complete = {
	.title    = __("Force RAID Consistency Check Status"),
	.body     = __("You selected to force a RAID consistency check")
};

struct screen_opts raid_resync_opt[] = {
	{confirm_raid_resync, "\n"}
};

s_node n_raid_resync = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(raid_resync_opt),
	.options  = &raid_resync_opt[0],
	.title    = __("Force RAID Consistency Check"),
	.header   = {
		__("Select the arrays to be checked\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Check\n\n"),
		"" }
};

s_node n_raid_resync_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Force RAID Consistency Check Failed"),
	.header   = {
		__("There are no arrays eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no disk arrays in the system.\n"),
		__("o An IOA is in a condition that makes the disks attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disks attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disks are missing.\n"),
		"" }
};

s_node n_confirm_raid_resync = {
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Force RAID Consistency Check"),
	.header   = {
		__("Forcing a consistency check on a RAID array may take "
		   "a long time, depending on current system activity.\n\n"),
		__("Press Enter to confirm.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" },
};

struct screen_opts configure_af_device_opt[] = {
	{confirm_init_device, "\n"}
};

struct screen_opts battery_maint_opt[] = {
	{battery_fork, "\n"}
};

s_node n_battery_maint = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG),
	.num_opts = NUM_OPTS(battery_maint_opt),
	.options  = &battery_maint_opt[0],
	.title    = __("Work with Resources Containing Cache Battery Packs"),
	.header   = {
		__("Type options, press Enter\n"),
		__("  1=Display battery information\n"),
		__("  2=Force battery pack into error state\n"),
		__("  3=Start IOA cache\n\n"),
		"" }
};

struct screen_opts confirm_force_battery_error_opt[] = {
	{force_battery_error, "c"}
};

s_node n_confirm_force_battery_error = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(confirm_force_battery_error_opt),
	.options  = &confirm_force_battery_error_opt[0],
	.title    = __("Force Battery Packs Into Error State"),
	.header   = {
		__("ATTENTION: This service function should be run only "
		   "under the direction of the IBM Hardware Service Support\n\n"),
		__("You have selected to force a cache batter error on an IOA\n\n"),
		__("You will have to replace the Cache Battery Pack in each selected "
		   "IOA to resume normal operations.\n\n"),
		__("System performance could be significantly degraded until the cache "
		   "battery packs are replaced on the selected IOAs.\n\n"),
		__("  c=Continue to force the following battery packs into an error state\n\n"),
		"" },
};

struct screen_opts confirm_start_cache_opt[] = {
	{enable_battery, "c"}
};

s_node n_confirm_start_cache = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(confirm_start_cache_opt),
	.options  = &confirm_start_cache_opt[0],
	.title    = __("Start IOA cache after concurrently replacing battery pack"),
	.header   = {
		__("ATTENTION: This service function should be run only "
		   "under the direction of the IBM Hardware Service Support\n\n"),
		__("You have selected to start the IOA cache after concurrently replacing the battery pack\n\n"),
		__("  c=Continue to start IOA cache\n\n"),
		"" },
};

s_node n_show_battery_info = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Battery Information")
};

struct screen_opts enclosures_maint_opt[] = {
	{enclosures_fork, "\n"}
};

s_node n_enclosures_maint = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(enclosures_maint_opt),
	.options  = &enclosures_maint_opt[0],
	.title    = __("Work with disk enclosures"),
	.header   = {
		__("Type options, press Enter\n"),
		__("  1=Display disk enclosure details\n"),
		__("  2=Suspend disk enclosure path\n"),
		__("  3=Resume disk enclosure path\n\n"),
		"" }
};

struct screen_opts confirm_suspend_disk_enclosure_opt[] = {
	{ipr_suspend_disk_enclosure, "c"}
};

s_node n_confirm_suspend_disk_enclosure = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(confirm_suspend_disk_enclosure_opt),
	.options  = &confirm_suspend_disk_enclosure_opt[0],
	.title    = __("Suspend Disk Enclosure"),
	.header   = {
		__("ATTENTION: This service function should be run only "
		   "under the direction of the IBM Hardware Service Support\n\n"),
		__("  c=Continue to issue suspend enclosure command\n\n"),
		"" },
};

struct screen_opts confirm_resume_disk_enclosure_opt[] = {
	{ipr_resume_disk_enclosure, "c"}
};

s_node n_confirm_resume_disk_enclosure = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(confirm_resume_disk_enclosure_opt),
	.options  = &confirm_resume_disk_enclosure_opt[0],
	.title    = __("Resume Disk Enclosure"),
	.header   = {
		__("ATTENTION: This service function should be run only "
		   "under the direction of the IBM Hardware Service Support\n\n"),
		__("  c=Continue to issue resume enclosure command\n\n"),
		"" },
};

s_node n_show_enclosure_info = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Hardware detail Information")
};

struct screen_opts path_status_opt[] = {
	{path_details, "\n"}
};

s_node n_path_status = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(path_status_opt),
	.options  = &path_status_opt[0],
	.title    = __("Display SAS Path Status"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Display SAS Path routing details\n\n"),
		"" }
};

s_node n_path_details = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | FWD_FLAG),
	.title    = __("Display SAS Path Details"),
	.header   = {__("\n\n"), ""}
};

struct screen_opts bus_config_opt[] = {
	{change_bus_attr, "\n"}
};

s_node n_bus_config = {
	.rc_flags = (CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(bus_config_opt),
	.options  = &bus_config_opt[0],
	.title    = __("Work with SCSI Bus Configuration"),
	.header   = {
		__("Select the adapter to change scsi bus attribute.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=change scsi bus attribute\n\n"),
		"" }
};

s_node n_bus_config_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title  = __("Work with SCSI Bus Configuration Failed"),
	.header = {
		__("There are no SCSI Buses eligible for the selected operation. "
		   "None of the installed adapters support the requested "
		   "operation.\n"),
		"" }
};

struct screen_opts change_bus_attr_opt[] = {
	{NULL, "c"},
	{confirm_change_bus_attr, "\n"}
};

s_node n_change_bus_attr = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG | FWD_FLAG | MENU_FLAG),
	.num_opts = NUM_OPTS(change_bus_attr_opt),
	.options  = &change_bus_attr_opt[0],
	.title    = __("Change SCSI Bus Configuration"),
	.header   = {
		__("Current bus configurations are shown. To change "
		   "setting hit 'c' for options menu. Highlight "
		   "desired option then hit Enter.\n"),
		__("  c=Change Setting\n\n"),
		"" }
};

struct screen_opts confirm_change_bus_attr_opt[] = {
	{NULL, "c"}
};

s_node n_confirm_change_bus_attr = {
	.f_flags  = (CONFIRM_FLAG | EXIT_FLAG | CANCEL_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(confirm_change_bus_attr_opt),
	.options  = &confirm_change_bus_attr_opt[0],
	.title    = __("Confirm Change SCSI Bus Configuration"),
	.header   = {
		__("Confirming this action will result in the following"
		   "configuration to become active.\n"),
		__("  c=Confirm Change SCSI Bus Configration\n\n"),
		"" }
};

struct screen_opts driver_config_opt[] = {
	{change_driver_config, "\n"}
};

s_node n_driver_config = {
	.rc_flags = (CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(driver_config_opt),
	.options  = &driver_config_opt[0],
	.title    = __("Adapter Driver Configuration"),
	.header   = {
		__("Select adapter to change driver configuration\n\n"),
		__("Type choice, press Enter\n"),
		__("  1=change driver configuration\n\n"),
		"" }
};

s_node n_change_driver_config = {
	.f_flags  = (ENTER_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Change Driver Configuration"),
	.header   = {
		__("Current Driver configurations "
		   "are shown. To change setting, type in new "
		   "value then hit Enter\n\n"),
		"" }
};

struct screen_opts disk_config_opt[] = {
	{change_disk_config, "\n"}
};

s_node n_disk_config = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(disk_config_opt),
	.options  = &disk_config_opt[0],
	.title    = __("Change Disk Configuration"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Change Disk Configuration\n\n"),
		"" }
};

struct screen_opts change_disk_attr_opt[] = {
	{NULL, "c"},
	{NULL, "\n"}
};

s_node n_change_disk_config = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG | FWD_FLAG | MENU_FLAG),
	.num_opts = NUM_OPTS(change_disk_attr_opt),
	.options  = &change_disk_attr_opt[0],
	.title    = __("Change Configuration of Disk"),
	.header   = {
		__("Current Disk configurations are shown. To change "
		   "setting hit 'c' for options menu. Highlight "
		   "desired option then hit Enter.\n"),
		__("  c=Change Setting\n\n"),
		"" }
};

struct screen_opts ioa_config_opt[] = {
	{change_ioa_config, "\n"}
};

s_node n_ioa_config = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(ioa_config_opt),
	.options  = &ioa_config_opt[0],
	.title    = __("Change Adapter Configuration"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Change Adapter Configuration\n\n"),
		"" }
};

struct screen_opts change_ioa_attr_opt[] = {
	{NULL, "c"},
	{NULL, "\n"}
};

s_node n_change_ioa_config = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG | FWD_FLAG | MENU_FLAG),
	.num_opts = NUM_OPTS(change_ioa_attr_opt),
	.options  = &change_ioa_attr_opt[0],
	.title    = __("Change Configuration of Adapter"),
	.header   = {
		__("Current Adapter configurations are shown. To change "
		   "setting hit 'c' for options menu. Highlight "
		   "desired option then hit Enter.\n"),
		__("  c=Change Setting\n\n"),
		"" }
};

struct screen_opts download_ucode_opt[] = {
	{choose_ucode, "\n"}
};

s_node n_download_ucode = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG | FWD_FLAG | TOGGLE_FLAG),
	.num_opts = NUM_OPTS(download_ucode_opt),
	.options  = &download_ucode_opt[0],
	.title    = __("Download Microcode"),
	.header   = {
		__("Select the device(s) to download microcode\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=device to download microcode\n\n"),
		"" }
};

s_node n_choose_ucode = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (CANCEL_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Choose Microcode Image"),
	.header   = {
		__("Select the microcode image to download\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=download microcode\n\n"),
		"OPT  Version         Date     Image File\n",
		"---  --------------- -------- --------------------------\n",
		"" }
};

s_node n_confirm_download_ucode = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (CANCEL_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Microcode Download"),
	.header   = {
		__("ATTENTION:  System performance may be affected during "
		   "the microcode download process\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"OPT  Version         Date     Image File\n",
		"---  --------------- -------- --------------------------\n",
		"" }
};

s_node n_confirm_download_all_ucode = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (CANCEL_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Microcode Download"),
	.header   = {
		__("The following devices are going to be updated.\n\n"
		   "ATTENTION:  System performance may be affected during "
		   "the microcode download process. "
		   "Download may take some time.\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"Name   Version    Date       Image File\n",
		"------ ---------  ---------  -----------------------------\n",
		"" }
};

s_node n_download_ucode_in_progress = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (CANCEL_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Microcode Download In Progress"),
	.header   = {
		__("Please wait for the microcode update to complete\n"),
		__("This process may take several minutes\n"),
		"" }
};

struct screen_opts log_menu_opt[] = {
	{ibm_storage_log_tail, "1", __("View most recent ipr error messages")},
	{ibm_storage_log,      "2", __("View ipr error messages")},
	{kernel_log,           "3", __("View all kernel error messages")},
	{iprconfig_log,        "4", __("View iprconfig error messages")},
	{kernel_root,          "5", __("Set root kernel message log directory")},
	{set_default_editor,   "6", __("Set default editor")},
	{restore_log_defaults, "7", __("Restore defaults")},
	{ibm_boot_log,         "8", __("View ipr boot time messages")}
};

s_node n_log_menu = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(log_menu_opt),
	.options  = &log_menu_opt[0],
	.title    = __("Kernel Messages Log"),
	.header   = {
		__("ATTENTION: Devices listed below are currently known to be zeroed. "
		   "By exiting now, this information will be lost, resulting in potentially "
		   "longer array creation times.\n\n"),
		__("  q=Cancel to return to iprconfig.\n"),
		__("  e=Exit iprconfig\n")
	}
};

s_node n_exit_menu = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(log_menu_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Exit")
};

struct screen_opts kernel_root_opt[] = {
	{confirm_kernel_root, "\n"}
};

s_node n_kernel_root = {
	.f_flags  = (ENTER_FLAG),
	.num_opts = NUM_OPTS(kernel_root_opt),
	.options  = &kernel_root_opt[0],
	.title    = __("Kernel Messages Log Root Directory"),
	.header   = {
		__("Enter new directory and press Enter\n\n"),
		"" }
};

struct screen_opts confirm_kernel_root_opt[] = {
	{NULL, "c"},
};

s_node n_confirm_kernel_root = {
	.f_flags  = (CONFIRM_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(confirm_kernel_root_opt),
	.options  = &confirm_kernel_root_opt[0],
	.title    = __("Confirm Root Directory Change"),
	.header   = {
		__("Press 'c' to confirm your new "
		   "kernel root directory choice.\n\n"),
		__("To return to the log menu without "
		   "changing the root directory, choose 'q'.\n\n"),
		"" }
};

struct screen_opts set_default_editor_opt[] = {
	{confirm_set_default_editor, "\n"}
};

s_node n_set_default_editor = {
	.f_flags  = (ENTER_FLAG),
	.num_opts = NUM_OPTS(set_default_editor_opt),
	.options  = &set_default_editor_opt[0],
	.title    = __("Kernel Messages Editor"),
	.header   = {
		__("Enter new editor command and press Enter\n\n"),
		"" }
};

struct screen_opts confirm_set_default_editor_opt[] = {
	{NULL, "c"}
};

s_node n_confirm_set_default_editor = {
	.f_flags  = (CONFIRM_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(confirm_set_default_editor_opt),
	.options  = &confirm_set_default_editor_opt[0],
	.title    = __("Confirm Editor Change"),
	.header   = {
		__("Press 'c' to confirm your editor choice\n\n"),
		__("To return to the log menu without "
		   "changing the editor, choose 'q'.\n\n"),
		"" }
};


struct screen_opts ucode_screen_opt[] = {
	{download_ucode,      "1", __("Download microcode")},
	{download_all_ucode,  "2", __("Download latest microcode to all devices")},
};

s_node n_ucode_screen = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(ucode_screen_opt),
	.options  = &ucode_screen_opt[0],
	.title    = __("Work with microcode updates")
};

const char *screen_status[] = {
	/*  0 */ "",
	/*  1 */ "",
	/*  2 */ __("Invalid option specified"),
	/*  3 */ __("Screen paged up"),
	/*  4 */ __("Already at top"),
	/*  5 */ __("Screen paged down"),
	/*  6 */ __("Already at bottom"),
	/*  7 */ "",
	/*  8 */ "",
	/*  9 */ "",
	/* 10 */ __("No devices to show details for"),
	/* 11 */ "",
	/* 12 */ "",
	/* 13 */ "",
	/* 14 */ "",
	/* 15 */ __("The selection is not valid."),
	/* 16 */ __("Error.  More than one device was selected."),
	/* 17 */ __("Invalid option.  No devices selected."),
	/* 18 */ __("Disk array successfully created."),
	/* 19 */ __("Create disk array failed."),
	/* 20 */ __("Delete disk array failed."),
	/* 21 */ __("Disk array successfully deleted."),
	/* 22 */ __("Create disk array failed - can not mix SSDs and HDDs."),
	/* 23 */ "",
	/* 24 */ "",
	/* 25 */ __("Error:  number of devices selected must be a multiple of %d"),
	/* 26 */ __("Add device failed"),
	/* 27 */ __("Add device completed successfully"),
	/* 28 */ __("Rebuild started, view Disk Array Status Window for rebuild progress"),
	/* 29 */ __("Rebuild failed"),
	/* 30 */ __("Device concurrent maintenance failed"),
	/* 31 */ __("Device concurrent maintenance failed, device in use"),
	/* 32 */ __("Device concurrent maintenance completed successfully"),
	/* 33 */ __("No units available for initialize and format"),
	/* 34 */ __("Initialize and format completed successfully"),
	/* 35 */ __("Initialize and format failed"),
	/* 36 */ __("Reclaim IOA Cache Storage completed successfully"),
	/* 37 */ __("Reclaim IOA Cache Storage failed"),
	/* 38 */ __("No Reclaim IOA Cache Storage is necessary"),
	/* 39 */ __("No Reclaim IOA Cache Storage performed"),
	/* 40 */ __("Rebuild started, view Disk Array Status Window for rebuild progress"),
	/* 41 */ __("Rebuild failed"),
	/* 42 */ __("The selected battery packs have successfully been placed into an error state."),
	/* 43 */ __("Failed to force the selected battery packs into an error state."),
	/* 44 */ __("No configured resources contain a cache battery pack."),
	/* 45 */ __("Change SCSI Bus configurations completed successfully"),
	/* 46 */ __("Change SCSI Bus configurations failed"),
	/* 47 */ __("Change device driver configurations completed successfully"),
	/* 48 */ __("Change device driver configurations failed"),
	/* 49 */ __("No units available for initialize and format"),
	/* 50 */ __("Initialize and format completed successfully"),
	/* 51 */ __("Initialize and format failed"),
	/* 52 */ __("No devices available for the selected hot spare operation"),
	/* 53 */ __("Hot spare successfully created"),
	/* 54 */ __("Hot spare successfully deleted"),
	/* 55 */ __("Failed to create hot spare"),
	/* 56 */ __("Failed to delete hot spare"),
	/* 57 */ __("Successfully changed device driver configuration"),
	/* 58 */ __("Failed to change device driver configuration"),
	/* 59 */ __("Invalid directory"),
	/* 60 */ __("Root directory changed to %s"),
	/* 61 */ __("Root directory unchanged"),
	/* 62 */ __("Editor changed to %s"),
	/* 63 */ __("Editor unchanged"),
	/* 64 */ __("Default log values restored"),
	/* 65 */ __("Editor returned %d. Try setting the default editor."),
	/* 66 */ __("Failed to change disk configuration."),
	/* 67 */ __("Microcode Download failed."),
	/* 68 */ __("Failed to enable IOA cache."),
	/* 69 */ __("Invalid number of devices selected."),
	/* 70 */ __("Failed to start IOA cache."),
	/* 71 */ __("IOA cache started successfully."),
	/* 72 */ __("Selected battery packs successfully forced into an error state."),
	/* 73 */ __("Force RAID Consistency check failed"),
	/* 74 */ __("RAID Consistency check successful"),
	/* 75 */ __("Failed to read error log. Try setting root kernel message log directory."),
	/* 76 */ __("No SAS disks available"),
	/* 77 */ __("Too many disks were selected.  The maximum is %d."),
	/* 78 */ __("Too few disks were selected.  The minimum is %d."),
	/* 79 */ __("Migrate Array Protection completed successfully."),
	/* 80 */ __("Migrate Array Protection failed."),
	/* 81 */ __("Selected disk enclosure was suspended successfully."),
	/* 82 */ __("Failed to suspend disk enclosure"),
	/* 83 */ __("Disk enclosure is already in suspend state"),
	/* 84 */ __("Disk enclosure is already in active state"),
	/* 85 */ __("Selected disk enclosure was resumed successfully."),
	/* 86 */ __("Failed to resume disk enclosure"),
	/* 87 */ __("Can not suspend an expander from the RAID controller with the same serial number"),
	/* 88 */ __("Incorrect device type specified. Please specify a valid disk enclosure to suspend"),
	/* 89 */ __("Incorrect device type specified. Please specify a valid disk enclosure to resume"),
	/* 90 */ __("Selected disk enclosure is in Unknown state. Please check your hardware support"),
	/* 91 */ __("Create disk array failed - can not mix 5XX and 4K disks."),
	/* 92 */ __("Create disk array failed - can not build with read intensive disks only."),
	/* 93 */ __("All devices up to date"),
	/* 94 */ __("Temporary log file creation failed: %s"),

      /* NOTE:  127 maximum limit */
};

/* TODO - replace constants in iprconfig.c with the following enums/defines */
enum {
	RC_0_Success = 0,
	RC_1_Blank,
	RC_2_Invalid_Option,
	RC_3_Screen_Up,
	RC_4_At_Top,
	RC_5_Screen_Down,
	RC_6_At_Bottom,
	RC_7_Blank,
	RC_8_Blank,
	RC_9_Blank,
	RC_10_No_Devices,
	RC_11_Blank,
	RC_12_Blank,
	RC_13_Blank,
	RC_14_Blank,
	RC_15_Invalid_Selection,
	RC_16_More_Than_One_Dev,
	RC_17_Invalid_Option,
	RC_18_Array_Created,
	RC_19_Create_Fail,
	RC_20_Delete_Fail,
	RC_21_Delete_Success,
	RC_22_Mixed_Block_Dev_Classes,
	RC_23_Blank,
	RC_24_Blank,
	RC_25_Devices_Multiple,
	RC_26_Include_Fail,
	RC_27_Include_Success,
	RC_28_Rebuild_Started,
	RC_29_Rebuild_Fail,
	RC_30_Maint_Fail,
	RC_31_Maint_Fail_In_Use,
	RC_32_Maint_Success,
	RC_33_No_Units,
	RC_34_Init_Format_Success,
	RC_35_Init_Format_Fail,
	RC_36_Reclaim_Cache_Success,
	RC_37_Reclaim_Cache_Fail,
	RC_38_No_Reclaim_Needed,
	RC_39_No_Reclaim_Performed,
	RC_40_Rebuild_Started,
	RC_41_Rebuild_Failed,
	RC_42_Battery_Err_State_Success,
	RC_43_Battery_Err_State_Fail,
	RC_44_No_Battery_Pack,
	RC_45_Change_Bus_Conf_Success,
	RC_46_Change_Bus_Conf_Fail,
	RC_47_Change_Driver_Conf_Success,
	RC_48_Change_Driver_Conf_Fail,
	RC_49_No_Units_Available,
	RC_50_Init_Format_Success,
	RC_51_Init_Format_Fail,
	RC_52_No_Devices_Available,
	RC_53_Hot_Spare_Created,
	RC_54_Hot_Spare_Deleted,
	RC_55_Failed_Create,
	RC_56_Failed_Delete,
	RC_57_Change_Driver_Conf_Success,
	RC_58_Change_Driver_Conf_Fail,
	RC_59_Invalid_Dir,
	RC_60_Root_Changed,
	RC_61_Root_Unchanged,
	RC_62_Editor_Changed,
	RC_63_Editor_Unchanged,
	RC_64_Log_Restored,
	RC_65_Set_Default_Editor,
	RC_66_Disk_Conf_Fail,
	RC_67_uCode_Download_Fail,
	RC_68_Cache_Enable_Fail,
	RC_69_Invalid_Number_Devs,
	RC_70_Cache_Start_Fail,
	RC_71_Cache_Start_Success,
	RC_72_Battery_Err_State,
	RC_73_Force_Check_Fail,
	RC_74_Check_Success,
	RC_75_Failed_Read_Err_Log,
	RC_76_No_SAS_Disks,
	RC_77_Too_Many_Disks,
	RC_78_Too_Few_Disks,
	RC_79_Migrate_Prot_Success,
	RC_80_Migrate_Prot_Fail,
	RC_81_Suspended_Success,
	RC_82_Suspended_Fail,
	RC_83_Enclosure_Is_Suspend,
	RC_84_Enclosure_Is_Active,
	RC_85_Enclosure_Resume_Success,
	RC_86_Enclosure_Resume_Fail,
	RC_87_No_Suspend_Same_Seri_Num,
	RC_88_Invalid_Dev_For_Suspend,
	RC_89_Invalid_Dev_For_Resume,
	RC_90_Enclosure_Is_Unknown,
	RC_91_Mixed_Logical_Blk_Size,
	RC_92_UNSUPT_REQ_BLK_DEV_CLASS,
	RC_93_All_Up_To_Date,
	RC_94_Tmp_Log_Fail,

	/* NOTE:  127 maximum limit */
};

#endif /* iprconfig_h */
