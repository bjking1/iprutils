#include <libintl.h>
#define _(string) gettext(string)
#define __(string) (string)
#define EXIT_FLAG 0x8000	/* stops at given screen on exit call */
#define CANCEL_FLAG 0x4000	/* stops at given screen on quit call */
#define REFRESH_FLAG 0x2000	/* refreshes screen on quit or exit */
#define TOGGLE_FLAG 0x1000
#define FWD_FLAG 0x0800
#define CONFIRM_FLAG 0x0400
#define CONFIRM_REC_FLAG 0x0200
#define MENU_FLAG 0x0100
#define ENTER_FLAG 0x0080

#define NUM_OPTS(x) (sizeof(x)/sizeof(struct screen_opts))

#define REFRESH_SCREEN -13
#define TOGGLE_SCREEN -14

#define INVALID_OPTION_STATUS 2
#define PGUP_STATUS 3
#define TOP_STATUS 4
#define PGDN_STATUS 5
#define BTM_STATUS 6

#define MAX_FIELD_SIZE 39

typedef struct info_container i_container;

struct info_container {
	i_container *next_item;	/* reference to next info_container */
	char field_data[MAX_FIELD_SIZE + 1]; /* stores characters entered into a user-entry field */
	void *data;		/* stores a field pointer */
	int y;            /* cursor y position of user selection */
	int x;            /* cursor x position of user selection */
};

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

int disk_status(i_container * i_con);
int device_details(i_container * i_con);
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

int init_device(i_container * i_con);
int confirm_init_device(i_container * i_con);
int send_dev_inits(i_container * i_con);
int reclaim_cache(i_container * i_con);
int confirm_reclaim(i_container * i_con);
int reclaim_warning(i_container * i_con);
int reclaim_result(i_container * i_con);
int af_include(i_container * i_con);
int af_remove(i_container * i_con);
int configure_af_device(i_container * i_con, int action_code);	/* called by af_include and af_remove */
int add_hot_spare(i_container * i_con);
int remove_hot_spare(i_container * i_con);
int hot_spare(i_container * i_con, int action);

int select_hot_spare(i_container * i_con, int action);
int confirm_hot_spare(int action);
int hot_spare_complete(int action);

int raid_rebuild(i_container * i_con);
int confirm_raid_rebuild(i_container * i_con);

int battery_maint(i_container * i_con);
int battery_fork(i_container * i_con);
int force_battery_error(i_container *i_con);
int bus_config(i_container * i_con);
int change_bus_attr(i_container * i_con);
int confirm_change_bus_attr(i_container *i_con);
int driver_config(i_container * i_con);
int change_driver_config(i_container *i_con);
int disk_config(i_container * i_con);
int change_disk_config(i_container * i_con);
int download_ucode(i_container * i_con);
int choose_ucode(i_container * i_con);
int log_menu(i_container * i_con);
int ibm_storage_log_tail(i_container * i_con);
int ibm_storage_log(i_container * i_con);
int kernel_log(i_container * i_con);
int iprconfig_log(i_container * i_con);
int kernel_root(i_container * i_con);
int confirm_kernel_root(i_container * i_con);
int confirm_kernel_root_change(i_container * i_con);
int confirm_kernel_root_change2(i_container * i_con);
int set_default_editor(i_container * i_con);
int confirm_set_default_editor(i_container * i_con);
int confirm_set_default_editor_change(i_container * i_con);
int confirm_set_default_editor_change2(i_container * i_con);
int restore_log_defaults(i_container * i_con);
int ibm_boot_log(i_container * i_con);

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
	char *header[];
};

struct screen_opts null_opt[] = {
	{NULL, "\n"}
};

struct screen_opts main_menu_opt[] = {
	{disk_status,        "1", __("Display disk hardware status")},
	{raid_screen,        "2", __("Work with Disk Arrays")},
	{disk_unit_recovery, "3", __("Work with disk unit recovery")},
	{bus_config,         "4", __("Work with SCSI bus configuration")},
	{driver_config,      "5", __("Work with driver configuration")},
	{disk_config,        "6", __("Work with disk configuration")},
	{download_ucode,     "7", __("Download Microcode")},
	{log_menu,           "8", __("Analyze log")}
};

s_node n_main_menu = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG),
	.num_opts = NUM_OPTS(main_menu_opt),
	.options  = &main_menu_opt[0],
	.title    =  __("Work with Disk Units")
};

struct screen_opts disk_status_opt[] = {
	{device_details, "\n"}
};

s_node n_disk_status = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(disk_status_opt),
	.options  = &disk_status_opt[0],
	.title    = __("Display Disk Hardware Status"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  5=Display hardware resource information details\n\n"),
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

struct screen_opts raid_screen_opt[] = {
	{raid_status,      "1", __("Display disk array status")},
	{raid_start,       "2", __("Create a disk array")},
	{raid_stop,        "3", __("Delete a disk array")},
	{raid_include,     "4", __("Add a device to a disk array")},
	{af_include,       "5", __("Format device for advanced function")},
	{af_remove,        "6", __("Format device for JBOD function")},
	{add_hot_spare,    "7", __("Configure a hot spare device")},
	{remove_hot_spare, "8", __("Unconfigure a hot spare device")}
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
		__("  5=Display hardware resource information details\n\n"),
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
		__("Select the subsystems to delete a disk array.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=delete a disk array\n\n"),
		"" }
};

s_node n_raid_stop_fail = {
	.title    = __("Delete a Disk Array Failed"),
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.header   = {
		__("There are no disk units eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no device parity protected units in the system.\n"),
		__("o An IOA is in a condition that makes the disk units attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disk units attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disk units are missing.\n"),
		__("o The disk unit/s are currently in use.\n"),
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
		__("ATTENTION: Disk units connected to these subsystems"
		   "will not be protected after you confirm your choice.\n\n"),
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
		__("Select the subsystems to create a disk array.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=create a disk array\n\n"),
		"" }
};

s_node n_raid_start_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Create a Disk Array Failed"),
	.header   = {
		__("There are no disk units eligible for the selected operation due "
		   "to one or more of the following reasons:\n\n"),
		__("o There are not enough device parity capable units in the system.\n"),
		__("o An IOA is in a condition that makes the disk units attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disk units attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disk units are missing.\n"),
		__("o The disk unit/s are currently in use.\n"),
		"" }
};

s_node n_configure_raid_start = {
	.rc_flags = (CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Select Disk Units for Parity Protection"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_confirm_raid_start = {
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Start Device Parity Protection"),
	.header   = {
		__("ATTENTION: Data will not be preserved and may be "
		   "lost on selected disk units when parity protection is enabled.\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

s_node n_raid_start_complete = {
	.title    = __("Start Device Parity Protection Status"),
	.body     = __("You selected to start device parity protection")
};

struct screen_opts raid_include_opt[] = {
	{configure_raid_include, "\n"}
};

s_node n_raid_include = {
	.rc_flags = (CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(raid_include_opt),
	.options  = &raid_include_opt[0],
	.title    = __("Add a Device to a Disk Array"),
	.header   = {
		__("Select the subsystem that the disk unit will be included.\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Select Parity Array to Include Disk Unit\n\n"),
		"" }
};

s_node n_raid_include_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Include Device Parity Protection Failed"),
	.header   = {
		__("There are no disk units eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no device parity protected units in the system.\n"),
		__("o An IOA is in a condition that makes the disk units attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disk units attached to an advanced function IOA have"
		   "reported to the system. Retry the operation.\n"),
		__("o The disk units are missing.\n"),
		__("o The disk unit/s are currently in use.\n"),
		"" }
};

s_node n_configure_raid_include = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options = &null_opt[0],
	.title   = __("Include Disk Units in Device Parity Protection"),
	.header  = {
		__("Select the units to be included in Device Parity Protection\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Add a Device to a Disk Array\n\n"),
		   "" }
};

s_node n_configure_raid_include_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Include Disk Unit Failed"),
	.header = {
		__("There are no disk units eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are not enough disk units available to be included.\n"),
		__("o The disk unit that needs to be included is not at the right "
		   "location. Examine the 'message log' for the exposed unit "
		   "and make sure that the replacement unit is at the correct "
		   "location\n"),
		__("o Not all disk units attached to an IOA have reported to the "
		   "system. Retry the operation.\n"),
		__("o The disk unit to be included must be the same or greater "
		   "capacity than the smallest device in the volume set and "
		   "be formatted correctly\n"),
		__("o The type/model of the disk units is not supported for the "
		   "requested operation\n"),
		"" }
};

s_node n_confirm_raid_include = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Disk Units to be Included"),
	.header   = {
		__("ATTENTION: All listed device units will be formatted "
		   "and zeroed before listed device is added to array.\n\n"),
		__("Press Enter to confirm your choice to have the system "
		   "include the selected units in device parity protection\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

s_node n_dev_include_complete = {
	.title    = __("Include Disk Units in Device Parity Protection Status"),
	.header   = {
		__("The operation to include units in the device parity protection "
		   "will be done in two phases.  The device must first be formatted "
		   "before device may be included into array set."),
		__("You selected to include a device in a device parity set"),
		""
	}
};

s_node n_af_include_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Format Device for Advanced Function Failed"),
	.header = {
		__("There are no disk units eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no eligible device units in the system.\n"),
		__("o An IOA is in a condition that makes the disk units attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disk units attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disk units are missing.\n"),
		__("o The disk unit/s are currently in use.\n"),
		"" }
};

s_node n_af_remove_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Disable Device for Advanced Functions Failed"),
	.header   = {
		__("There are no disk units eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no eligible device units in the system.\n"),
		__("o An IOA is in a condition that makes the disk units attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disk units attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disk units are missing.\n"),
		__("o The disk unit/s are currently in use.\n"),
		"" }
};

s_node n_add_hot_spare_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Enable Device as Hot Spare Failed"),
	.header = {
		__("There are no disk units eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no eligible device units in the system.\n"),
		__("o An IOA is in a condition that makes the disk units attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disk units attached to an advanced function IOA have "
		   "reported to the system. Retry the operation.\n"),
		__("o The disk units are missing.\n"),
		__("o The disk unit/s are currently in use.\n"),
		"" }
};

s_node n_remove_hot_spare_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Disable Device as Hot Spare Failed"),
	.header = {
		__("There are no disk units eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no eligible device units in the system.\n"),
		__("o An IOA is in a condition that makes the disk units attached to "
		   "it read/write protected. Examine the kernel messages log "
		   "for any errors that are logged for the IO subsystem "
		   "and follow the appropriate procedure for the reference code to "
		   "correct the problem, if necessary.\n"),
		__("o Not all disk units attached to an advanced function IOA have "
		   "reported to the system.  Retry the operation.\n"),
		__("o The disk units are missing.\n"),
		__("o The disk unit/s are currently in use.\n"),
		"" }
};

s_node n_add_hot_spare = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Configure a hot spare device"),
	.header   = {
		__("Select the subsystems which disk units will be "
		   "configured as hot spares\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Select subsystem\n\n"),
		"" },
};

s_node n_remove_hot_spare = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Unconfigure a hot spare device"),
	.header   = {
		__("Select the subsystems which disk units will be "
		   "unconfigured as hot spares\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Select subsystem\n\n"),
		"" },
};

s_node n_select_add_hot_spare = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Select Disk Units to Enable as Hot Spare"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_select_remove_hot_spare = {
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Select Disk Units to Disable as Hot Spare"),
	.header   = {
		__("Type option, press Enter.\n"),
		__("  1=Select\n\n"),
		"" }
};

s_node n_confirm_add_hot_spare = {
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Enable Hot Spare"),
	.header   = {
		__("ATTENTION: Existing data on these disk units "
		   "will not be preserved.\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" },
};

s_node n_confirm_remove_hot_spare = {
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Disable Hot Spare"),
	.header   = {
		__("ATTENTION: Existing data on these disk units "
		   "will not be preserved.\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"" }
};

struct screen_opts disk_unit_recovery_opt[] = {
	{concurrent_add_device,    "1", __("Concurrent Add Device")},
	{concurrent_remove_device, "2", __("Concurrent Remove/Replace Device")},
	{init_device,              "3", __("Initialize and format disk unit")},
	{reclaim_cache,            "4", __("Reclaim IOA cache storage")},
	{raid_rebuild,             "5", __("Rebuild disk unit data")},
	{battery_maint,            "6", __("Work with resources containing cache battery packs")}
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
	.title    = __("Verify Device Concurrent Add"),
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
	.title    = __("Select Disk Units to format for Advanced Function"),
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
	.title    = __("Select Disk Units to format for JBOD Function"),
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
	.title    = __("Select Disk Units for Initialize and Format"),
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
	.title    = __("Confirm Initialize and Format Disk Unit"),
	.header   = {
		__("Press 'c' to confirm your choice for 1=Initialize and format.\n"),
		__("  q=Return to change your choice.\n\n"),
		"" }
};

s_node n_dev_init_complete = {
	.title    = __("Initialize and Format Status"),
	.body     = __("You selected to initialize and format a disk unit")
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
		   "will be discarded. Damaged objects may result on the system.\n\n"),
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
		__("The disk units that may be affected by the function are displayed.\n"),
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
	.title    = " ",
	.header   = {
		__("ATTENTION!!!   ATTENTION!!!   ATTENTION!!!   ATTENTION!!!\n"),
		__("ATTENTION: Proceed with this function only if directed to from a "
		   "service procedure. Data in the IOA cache will be discarded. "
		   "By proceeding you will be allowing UNKNOWN data loss. "
		   "This data loss may or may not be detected by the host operating."
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
		__("Select the units to be rebuilt\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=Rebuild\n\n"),
		"" }
};

s_node n_raid_rebuild_fail = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Rebuild Disk Unit Data Failed"),
	.header   = {
		__("There are no disk units eligible for the selected operation "
		   "due to one or more of the following reasons:\n\n"),
		__("o There are no disk units that need to be rebuilt.\n"),
		__("o The disk unit that needs  to be rebuild is not at the right "
		   "location. Examine the 'message log' for the exposed unit "
		   "and make sure that the replacement unit is at the correct "
		   "location.\n"),
		__("o Not all disk units attached to an IOA have reported to the "
		   "system. Retry the operation.\n"),
		__("o All the disk units in a parity set must be the same capacity "
		   "with a minumum number of 4 disk units and a maximum of 10 units"
		   "in the resulting parity set.\n"),
		__("o The type/model of the disk units is not supported for the "
		   "requested operation.\n"),
		"" }
};

s_node n_confirm_raid_rebuild = {
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Rebuild Disk Unit Data"),
	.header   = {
		__("Rebuilding the disk unit data may take several "
		   "minutes for each unit selected.\n\n"),
		__("Press Enter to confirm having the data rebuilt.\n"),
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
		__("  2=Force battery pack into error state\n"),
		__("  5=Display battery information\n\n"),
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
		   "under the direction of the IBM Hardware Service Support\n"),
		__("You have selected to force a cache batter error on an IOA\n"),
		__("You will have to replace the Cache Battery Pack in each selected "
		   "IOA to resume normal operations.\n"),
		__("System performance could be significantly degraded until the cache "
		   "battery packs are replaced on the selected IOAs.\n"),
		__("  c=Continue to force the following battery packs into an error state\n\n"),
		"" },
};

s_node n_show_battery_info = {
	.f_flags  = (ENTER_FLAG | EXIT_FLAG | CANCEL_FLAG),
	.title    = __("Battery Information")
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
		__("Select the subsystem to change scsi bus attribute.\n\n"),
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
		__("Current Bus configurations are shown. To change "
		   "setting hit 'c' for options menu. Hightlight "
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
		__("Select subsystem to change driver configuration\n\n"),
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
		   "setting hit 'c' for options menu. Hightlight "
		   "desired option then hit Enter.\n"),
		__("  c=Change Setting\n\n"),
		"" }
};

struct screen_opts download_ucode_opt[] = {
	{choose_ucode, "\n"}
};

s_node n_download_ucode = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG | FWD_FLAG ),
	.num_opts = NUM_OPTS(download_ucode_opt),
	.options  = &download_ucode_opt[0],
	.title    = __("Download Microcode"),
	.header   = {
		__("Select the subsystem(s) to download microcode\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=device to download microcode\n\n"),
		"" }
};

s_node n_choose_ucode = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Choose Microcode Image"),
	.header   = {
		__("Select the microcode image to download\n\n"),
		__("Type choice, press Enter.\n"),
		__("  1=download microcode\n\n"),
		"OPT  Version  Image File\n",
		"---  -------- --------------------------\n",
		"" }
};

s_node n_confirm_download_ucode = {
	.rc_flags = (CANCEL_FLAG),
	.f_flags  = (CANCEL_FLAG | TOGGLE_FLAG | FWD_FLAG),
	.num_opts = NUM_OPTS(null_opt),
	.options  = &null_opt[0],
	.title    = __("Confirm Microcode Download"),
	.header   = {
		__("ATTENTION:  System performance may be affected during "
		   "the microcode download process\n\n"),
		__("Press Enter to continue.\n"),
		__("  q=Cancel to return and change your choice.\n\n"),
		"OPT  Version  Image File\n",
		"---  -------- --------------------------\n",
		"" }
};

struct screen_opts log_menu_opt[] = {
	{ibm_storage_log_tail, "1", __("View most recent IBM Storage error messages")},
	{ibm_storage_log,      "2", __("View IBM Storage error messages")},
	{kernel_log,           "3", __("View all kernel error messages")},
	{iprconfig_log,        "4", __("View sisconfig error messages")},
	{kernel_root,          "5", __("Set root kernel message log directory")},
	{set_default_editor,   "6", __("Set default editor")},
	{restore_log_defaults, "7", __("Restore defaults")},
	{ibm_boot_log,         "8", __("View IBM boot time messages")}
};

s_node n_log_menu = {
	.rc_flags = (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	.f_flags  = (EXIT_FLAG | CANCEL_FLAG),
	.num_opts = NUM_OPTS(log_menu_opt),
	.options  = &log_menu_opt[0],
	.title    = __("Kernel Messages Log")
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
	/* 15 */ __("The selection is not valid"),
	/* 16 */ __("Error.  More than one unit was selected."),
	/* 17 */ __("Invalid option.  No devices selected."),
	/* 18 */ __("Start Parity Protection completed successfully"),
	/* 19 */ __("Start Parity Protection failed."),
	/* 20 */ __("Stop Parity Protection failed"),
	/* 21 */ __("Stop Parity Protection completed successfully."),
	/* 22 */ "",
	/* 23 */ "",
	/* 24 */ "",
	/* 25 */ __("Error:  number of devices selected must be a multiple of %d"),
	/* 26 */ __("Include failed"),
	/* 27 */ __("Include Unit completed successfully"),
	/* 28 */ __("Rebuild started, view Parity Status Window for rebuild progress"),
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
	/* 40 */ __("Rebuild started, view Parity Status Window for rebuild progress"),
	/* 41 */ __("Rebuild failed"),
	/* 42 */ __("The selected battery packs have successfully been placed into an error state."),
	/* 43 */ __("Attempting to force the selected battery packs into an error state failed."),
	/* 44 */ __("No configured resources contain a cache battery pack."),
	/* 45 */ __("Change SCSI Bus configurations completed successfully"),
	/* 46 */ __("Change SCSI Bus configurations failed"),
	/* 47 */ __("Change Device Driver Configurations completed successfully"),
	/* 48 */ __("Change Device Driver Configurations failed"),
	/* 49 */ __("No units available for initialize and format"),
	/* 50 */ __("Initialize and format completed successfully"),
	/* 51 */ __("Initialize and format failed"),
	/* 52 */ __("No devices available for the selected hot spare operation"),
	/* 53 */ __("Enable Device as Hot Spare Completed successfully"),
	/* 54 */ __("Disable Device as Hot Spare Completed successfully"),
	/* 55 */ __("Configure a hot spare device failed"),
	/* 56 */ __("Unconfigure a hot spare device failed"),
	/* 57 */ __("Change Device Driver Configurations completed successfully"),
	/* 58 */ __("Change Device Driver Configurations failed"),
	/* 59 */ __("Invalid directory"),
	/* 60 */ __("Root directory changed to %s"),
	/* 61 */ __("Root directory unchanged"),
	/* 62 */ __("Editor changed to %s"),
	/* 63 */ __("Editor unchanged"),
	/* 64 */ __("Default log values restored"),
	/* 65 */ __("Editor returned %d. Try setting the default editor.")
};
