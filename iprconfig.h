#define EXIT_FLAG 0x8000	/* stops at given screen on exit call */
#define CANCEL_FLAG 0x4000	/* stops at given screen on quit call */
#define REFRESH_FLAG 0x2000	/* refreshes screen on quit or exit */
/*#define CONTINUE_FLAG 0x1000	* returns to function in progress rather than call a new one */
#define NULL_FLAG 0x0000	/* does not stop or refresh screen on q or e */

#define NUM_OPTS(x) (sizeof(x)/sizeof(struct screen_opts))

#define REFRESH_SCREEN -13
#define TOGGLE_SCREEN -14

#define STATUS_SET 888
#define EMPTY_STATUS 1
#define INVALID_OPTION_STATUS 2
#define PGUP_STATUS 3
#define TOP_STATUS 4
#define PGDN_STATUS 5
#define BTM_STATUS 6

#define MAX_FIELD_SIZE 39

typedef enum { list, menu } i_type;

typedef struct info_container i_container;

struct info_container {
	i_container *next_item;	/* reference to next info_container */
	char field_data[MAX_FIELD_SIZE + 1];	/* stores characters entere into a user-entry field */
	void *data;		/* stores a field pointer */
	i_type type;		/* used to determine whether to use data for a list or menu */
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
int disk_unit_details(i_container * i_con);
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
int raid_start_complete(i_container * i_con);
int raid_include(i_container * i_con);
int configure_raid_include(i_container * i_con);
int confirm_raid_include(i_container * i_con);
int confirm_raid_include_device(i_container * i_con);
int disk_unit_recovery(i_container * i_con);
int device_concurrent_maintenance(i_container * i_con);

int concurrent_add_device(i_container * i_con);
int concurrent_remove_device(i_container * i_con);
int issue_concurrent_maintenance(i_container * i_con);

int confirm_device_concurrent_maintenance_remove(i_container * i_con);
int confirm_device_concurrent_maintenance_insert(i_container * i_con);
int concurrent_maintenance_warning_remove(i_container * i_con);
int concurrent_maintenance_warning_insert(i_container * i_con);
int do_dasd_conc_maintenance_remove(i_container * i_con);
int do_dasd_conc_maintenance_insert(i_container * i_con);
int conc_maintenance_complt_remove(i_container * i_con);
int conc_maintenance_complt_insert(i_container * i_con);
int conc_maintenance_failed_remove(i_container * i_con);
int conc_maintenance_failed_insert(i_container * i_con);
int init_device(i_container * i_con);
int confirm_init_device(i_container * i_con);
int send_dev_inits(i_container * i_con);
int reclaim_cache(i_container * i_con);
int confirm_reclaim(i_container * i_con);
int confirm_reclaim_warning(i_container * i_con);
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
int confirm_force_battery_error(i_container * i_con);
int force_battery_error(i_container * i_con);
int show_battery_info(i_container * i_con);
int bus_config(i_container * i_con);
int change_bus_attr(i_container * i_con);
int driver_config(i_container * i_con);
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

/* others */
int menu_test(i_container * i_con);
ITEM **menu_test_menu(int field_index);

typedef struct screen_node s_node;

struct screen_opts {
    int (*screen_function) (i_container *);
    int cat_num;
    char *key;
};

struct screen_node {
    int text_set;
    int rc_flags;
    int num_opts;
    struct screen_opts *options;
    char *title;
    char *body;
    char *footer;
};

struct screen_opts main_menu_opt[] = {
	{disk_status,        2, "1"},
	{raid_screen,        3, "2"},
	{disk_unit_recovery, 4, "3"},
	{bus_config,         5, "4"},
	{driver_config,      6, "5"},
	{log_menu,           7, "6"}
};

s_node n_main_menu = {
    101,
    (EXIT_FLAG | CANCEL_FLAG),
    NUM_OPTS(main_menu_opt),
    &main_menu_opt[0],
    NULL,
    NULL,
    "%e"
};

struct screen_opts disk_status_opt[] = {
	{device_details, 0, "\n"}
};

s_node n_disk_status = {
    102,
    (CANCEL_FLAG),
    NUM_OPTS(disk_status_opt),
    &disk_status_opt[0],
    NULL,
    NULL,
    "%e%q%r%t%f"
};

s_node n_device_details = {
    104,
    (CANCEL_FLAG),
    0,
    NULL,
    NULL,
    NULL,
    "%n%e%q%f"
};

struct screen_opts raid_screen_opt[] = {
	{raid_status,      2, "1"},
	{raid_start,       3, "2"},
	{raid_stop,        4, "3"},
	{raid_include,     5, "4"},
	{af_include,       6, "5"},
	{af_remove,        7, "6"},
	{add_hot_spare,    8, "7"},
	{remove_hot_spare, 9, "8"}
};

s_node n_raid_screen = {
    200,
    (EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
    NUM_OPTS(raid_screen_opt),
    &raid_screen_opt[0],
    NULL,
    NULL,
    "%e%q"
};

s_node n_raid_status = {
    201,
    (CANCEL_FLAG),
    NUM_OPTS(disk_status_opt),
    &disk_status_opt[0],
    NULL,
    NULL,
    "%e%q%r%t%f"
};

struct screen_opts raid_stop_opt[] = {
	{confirm_raid_stop, 0, "\n"}
};

s_node n_raid_stop = {
	203,
	(CANCEL_FLAG),
	NUM_OPTS(raid_stop_opt),
	&raid_stop_opt[0]
};

s_node n_raid_stop_fail = {
	204,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts confirm_raid_stop_opt[] = {
	{do_confirm_raid_stop, 0, "\n"}
};

s_node n_confirm_raid_stop = {
	205,
	(CANCEL_FLAG),
	NUM_OPTS(confirm_raid_stop_opt),
	&confirm_raid_stop_opt[0]
};

s_node n_do_confirm_raid_stop = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_raid_stop_complete = {
	206,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts raid_start_opt[] = {
	{raid_start_loop, 0, "\n"}
};

s_node n_raid_start = {
	202,
	(CANCEL_FLAG | REFRESH_FLAG),
	NUM_OPTS(raid_start_opt),
	&raid_start_opt[0]
};

s_node n_raid_start_fail = {
	207,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_raid_start_loop = {
	999,
	(NULL_FLAG | EXIT_FLAG),
	0,
	NULL
};

struct screen_opts configure_raid_start_opt[] = {
	{NULL, 0, "\n"} /* configure_raid_parameters  */
};

s_node n_configure_raid_start = {
	208,
	(CANCEL_FLAG | REFRESH_FLAG),
	NUM_OPTS(configure_raid_start_opt),
	&configure_raid_start_opt[0]
};

s_node n_configure_raid_parameters = {
	209,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts confirm_raid_start_opt[] = {
  {NULL, 0, "\n"} /* raid_start_complete */
};

s_node n_confirm_raid_start = {
	210,
	(NULL_FLAG),
	NUM_OPTS(confirm_raid_start_opt),
	&confirm_raid_start_opt[0]
};

s_node n_raid_start_complete = {
	211,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts raid_include_opt[] = {
	{configure_raid_include, 0, "\n"}
};

s_node n_raid_include = {
	215,
	(CANCEL_FLAG | REFRESH_FLAG),
	NUM_OPTS(raid_include_opt),
	&raid_include_opt[0]
};

s_node n_raid_include_fail = {
	216,
	NULL_FLAG,
	0,
	NULL
};

struct screen_opts configure_raid_include_opt[] = {
	{NULL, 0, "\n"}	/* confirm_raid_include */
};

s_node n_configure_raid_include = {
	217,
	(CANCEL_FLAG),
	NUM_OPTS(configure_raid_include_opt),
	&configure_raid_include_opt[0]
};

s_node n_configure_raid_include_fail = {
	218,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts confirm_raid_include_opt[] = {
	{confirm_raid_include_device, 0, "\n"}
};

s_node n_confirm_raid_include = {
	219,
	(NULL_FLAG),
	NUM_OPTS(confirm_raid_include_opt),
	&confirm_raid_include_opt[0]
};

struct screen_opts confirm_raid_include_device_opt[] = {
	{NULL, 0, "c"}
};

s_node n_confirm_raid_include_device = {
	220,
	(NULL_FLAG),
	NUM_OPTS(confirm_raid_include_device_opt),
	&confirm_raid_include_device_opt[0]
};

s_node n_dev_include_complete = {
	221,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_af_include = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_af_include_fail = {
        230,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_af_remove = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_af_remove_fail = {
        231,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_add_hot_spare = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_add_hot_spare_fail = {
        232,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_remove_hot_spare = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_remove_hot_spare_fail = {
        233,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts hot_spare_opt[] = {
  {NULL, 0, "\n"}, /* select_hot_spare */
};

s_node n_hot_spare = {
        236,
	(CANCEL_FLAG),
	NUM_OPTS(hot_spare_opt),
	&hot_spare_opt[0]
};

struct screen_opts select_hot_spare_opt[] = {
  {NULL, 0, "\n"}
};

s_node n_select_hot_spare = {
        240,
	(NULL_FLAG),
	NUM_OPTS(select_hot_spare_opt),
	&select_hot_spare_opt[0]
};

struct screen_opts confirm_hot_spare_opt[] = {
  {NULL, 0, "\n"} /* hot_spare_complete */
};

s_node n_confirm_hot_spare = {
        245,
	(NULL_FLAG),
	NUM_OPTS(confirm_hot_spare_opt),
	&confirm_hot_spare_opt[0]
};

struct screen_opts disk_unit_recovery_opt[] = {
	{concurrent_add_device,    2, "1"},
	{concurrent_remove_device, 3, "2"},
	{init_device,              4, "3"},
	{reclaim_cache,            5, "4"},
	{raid_rebuild,             6, "5"},
	{battery_maint,            7, "6"}
};

s_node n_disk_unit_recovery = {
	300,
	(EXIT_FLAG | CANCEL_FLAG),
	NUM_OPTS(disk_unit_recovery_opt),
	&disk_unit_recovery_opt[0]
};

struct screen_opts concurrent_add_device_opt[] = {
	{issue_concurrent_maintenance, 0, "\n"}
};

s_node n_concurrent_add_device = {
	290,
	(CANCEL_FLAG),
	NUM_OPTS(concurrent_add_device_opt),
	&concurrent_add_device_opt[0]
};

struct screen_opts concurrent_remove_device_opt[] = {
	{issue_concurrent_maintenance, 0, "\n"}
};

s_node n_concurrent_remove_device = {
	291,
	(CANCEL_FLAG),
	NUM_OPTS(concurrent_remove_device_opt),
	&concurrent_remove_device_opt[0]
};

s_node n_device_concurrent_maintenance = {
	999,
	(CANCEL_FLAG),
	0,
	NULL
};

struct screen_opts init_device_opt[] = {
	{confirm_init_device, 0, "\n"}
};

s_node n_init_device = {
	312,
	(CANCEL_FLAG),
	NUM_OPTS(init_device_opt),
	&init_device_opt[0]
};

struct screen_opts confirm_inits_opt[] = {
	{send_dev_inits, 0, "c"}
};

s_node n_confirm_init_device = {
	313,
	(NULL_FLAG),
	NUM_OPTS(confirm_inits_opt),
	&confirm_inits_opt[0]
};

s_node n_send_dev_inits = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_dev_init_complete = {
	314,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts reclaim_cache_opts[] = {
	{confirm_reclaim, 0, "1"}
};

s_node n_reclaim_cache = {
	315,
	(CANCEL_FLAG),
	NUM_OPTS(reclaim_cache_opts),
	&reclaim_cache_opts[0]
};

struct screen_opts confirm_reclaim_opt[] = {
	{NULL, 0, "c"}		/* xxx */
};



s_node n_confirm_reclaim = {
	316,
	(NULL_FLAG),
	NUM_OPTS(confirm_reclaim_opt),
	&confirm_reclaim_opt[0]
};

struct screen_opts confirm_reclaim_warning_opt[] = {
	{reclaim_result, 0, "s"}
};

s_node n_confirm_reclaim_warning = {
	317,
	(NULL_FLAG),
	NUM_OPTS(confirm_reclaim_warning_opt),
	&confirm_reclaim_warning_opt[0]
};

s_node n_reclaim_result = {
	319,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts raid_rebuild_opt[] = {
	{confirm_raid_rebuild, 0, "\n"}
};

s_node n_raid_rebuild = {
	321,
	(CANCEL_FLAG),
	NUM_OPTS(raid_rebuild_opt),
	&raid_rebuild_opt[0]
};

s_node n_raid_rebuild_fail = {
	322,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts confirm_raid_rebuild_opt[] = {
	{NULL, 0, "\n"}
};

s_node n_confirm_raid_rebuild = {
	321,
	(CANCEL_FLAG),
	NUM_OPTS(raid_rebuild_opt),
	&confirm_raid_rebuild_opt[0]
};

struct screen_opts configure_af_device_opt[] = {
	{confirm_init_device, 0, "\n"}
};

s_node n_configure_af_device = {
	312,
	(NULL_FLAG),
	NUM_OPTS(configure_af_device_opt),
	&configure_af_device_opt[0]
};

struct screen_opts battery_maint_opt[] = {
	{battery_fork, 0, "\n"}
};

s_node n_battery_maint = {
	330,
	(CANCEL_FLAG),
	NUM_OPTS(battery_maint_opt),
	&battery_maint_opt[0]
};

struct screen_opts battery_fork_opt[] = {
	{confirm_force_battery_error, 2, "2"},
	{show_battery_info, 3, "5"}
};

s_node n_battery_fork = {
	999,
	(NULL_FLAG),
	NUM_OPTS(battery_fork_opt),
	&battery_fork_opt[0]
};

struct screen_opts confirm_force_battery_error_opt[] = {
	{force_battery_error, 0, "c"}
};

s_node n_confirm_force_battery_error = {
	331,
	(NULL_FLAG),
	NUM_OPTS(confirm_force_battery_error_opt),
	&confirm_force_battery_error_opt[0]
};

s_node n_force_battery_error = {
	331,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_show_battery_info = {
	332,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts bus_config_opt[] = {
	{change_bus_attr, 0, "\n"}
};

s_node n_bus_config = {
	401,
	(CANCEL_FLAG | REFRESH_FLAG),
	NUM_OPTS(bus_config_opt),
	&bus_config_opt[0]
};

s_node n_bus_config_fail = {
	402,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_change_bus_attr = { /* this is copied straight MENUS */
	403,
	(CANCEL_FLAG),
	0,
	NULL
};

struct screen_opts confirm_bus_config_opt[] = {
  {NULL, 0, "c"} /* confirm_bus_config_reset */
};


s_node n_confirm_bus_config = {
	404,
	(NULL_FLAG),
	NUM_OPTS(confirm_bus_config_opt),
	&confirm_bus_config_opt[0]
};

s_node n_confirm_bus_config_reset = {
	405,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_processing_bus_config = {
	404,
	(NULL_FLAG),
	NUM_OPTS(confirm_bus_config_opt),
	&confirm_bus_config_opt[0]
};

s_node n_driver_config = {
	459,
	(EXIT_FLAG | CANCEL_FLAG),
	0,
	NULL
};

struct screen_opts log_menu_opt[] = {
	{ibm_storage_log_tail, 2, "1"},
	{ibm_storage_log,      3, "2"},
	{kernel_log,           4, "3"},
	{iprconfig_log,        5, "4"},
	{kernel_root,          6, "5"},
	{set_default_editor,   7, "6"},
	{restore_log_defaults, 8, "7"},
	{ibm_boot_log,         9, "8"}
};

s_node n_log_menu = {
	500,
	(EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG),
	NUM_OPTS(log_menu_opt),
	&log_menu_opt[0]
};

s_node n_ibm_storage_log_tail = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_ibm_storage_log = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_kernel_log = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_iprconfig_log = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

struct screen_opts kernel_root_opt[] = {
	{confirm_kernel_root, 0, "\n"}
};

s_node n_kernel_root = {
	501,
	(NULL_FLAG),
	NUM_OPTS(kernel_root_opt),
	&kernel_root_opt[0]
};

struct screen_opts confirm_kernel_root_opt[] = {
	{NULL, 0, "c"},
};

s_node n_confirm_kernel_root = {
	502,
	(NULL_FLAG),
	NUM_OPTS(confirm_kernel_root_opt),
	&confirm_kernel_root_opt[0]
};

struct screen_opts set_default_editor_opt[] = {
	{confirm_set_default_editor, 0, "\n"}
};

s_node n_set_default_editor = {
	503,
	(NULL_FLAG),
	NUM_OPTS(set_default_editor_opt),
	&set_default_editor_opt[0]
};

struct screen_opts confirm_set_default_editor_opt[] = {
	{NULL, 0, "c"}
};

s_node n_confirm_set_default_editor = {
	504,
	(NULL_FLAG),
	NUM_OPTS(confirm_set_default_editor_opt),
	&confirm_set_default_editor_opt[0]
};

s_node n_restore_log_defaults = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_ibm_boot_log = {
	999,
	(NULL_FLAG),
	0,
	NULL
};

s_node n_print_device = {
        700,
	(NULL_FLAG),
	0,
	NULL
};
