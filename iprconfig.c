/******************************************************************/
/* Linux IBM IPR configuration utility                            */
/* Description: IBM Storage IOA Interface Specification (IPR)     */
/*              Linux configuration utility                       */
/*                                                                */
/* (C) Copyright 2000, 2001                                       */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprconfig.c,v 1.3 2003/10/31 15:16:43 manderso Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <term.h>
#include <ncurses.h>
#include <form.h>
#include <panel.h>
#include <menu.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <sys/stat.h>
#include <unistd.h>

struct device_detail_struct{
    struct ipr_ioa *p_ioa;
    struct ipr_device *p_ipr_dev;
    int field_index;
};

typedef struct {
    WINDOW *init_win;
    WINDOW *win;
    FORM *p_form;
    struct termios term;
    int max_x;
    int max_y;
    int start_x;
    int start_y;
} curses_info_t;

struct devs_to_init_t{
    struct ipr_device *p_ipr_device;
    struct ipr_ioa *p_ioa;
    int change_size;
#define IPR_522_BLOCK 522
#define IPR_512_BLOCK 512
    int cmplt;
    int fd;
    int do_init;
    int dev_type;
#define IPR_AF_DASD_DEVICE      1
#define IPR_JBOD_DASD_DEVICE    2
    struct devs_to_init_t *p_next;
};

struct prot_level
{
    struct ipr_array_cap_entry *p_array_cap_entry;
    u8     is_valid_entry:1;
    u8     reserved:7;
};

struct array_cmd_data{
    u8 array_id;
    u8 prot_level;
    u16 stripe_size;
    u32 array_num;
    u32 do_cmd;
    struct ipr_array_query_data   *p_qac_data;
    struct ipr_ioa        *p_ipr_ioa;
    struct ipr_device     *p_ipr_device;
    struct array_cmd_data *p_next;
};

struct window_l{
    WINDOW *p_win;
    struct window_l *p_next;
};

struct panel_l{
    PANEL *p_panel;
    struct panel_l *p_next;
};

static int max_x, max_y;
static int start_x, start_y;
static curses_info_t curses_info;
struct devs_to_init_t *p_head_dev_to_init;
struct devs_to_init_t *p_tail_dev_to_init;
struct array_cmd_data *p_head_raid_cmd;
struct array_cmd_data *p_tail_raid_cmd;
static char dev_field[] = "dev";
static int term_is_5250 = 0;
char log_root_dir[200];
char editor[200];
FILE *errpath;

#define DEFAULT_LOG_DIR "/var/log"
#define DEFAULT_EDITOR "vi -R"
#define FAILSAFE_EDITOR "vi -R -"
#define IS_CANCEL_KEY(c) ((c == KEY_F(4)) || (c == 'q') || (c == 'Q'))
#define CANCEL_KEY_LABEL "q"
#define IS_EXIT_KEY(c) ((c == KEY_F(3)) || (c == 'e') || (c == 'E'))
#define EXIT_KEY_LABEL "e"
#define ENTER_KEY 13
#define IS_ENTER_KEY(c) ((c == KEY_ENTER) || (c == ENTER_KEY))
#define IS_REFRESH_KEY(c) ((c == 'r') || (c == 'R'))
#define REFRESH_KEY_LABEL "r"

#define IS_PGDN_KEY(c) ((c == KEY_NPAGE) || (c == 'f') || (c == 'F'))
#define IS_PGUP_KEY(c) ((c == KEY_PPAGE) || (c == 'b') || (c == 'B'))

#define IS_5250_CHAR(c) (c == 0xa)

char *strip_trailing_whitespace(char *p_str);
u16 print_dev(struct ipr_resource_entry *p_resource_entry,
              char *p_buf,
              char *dev_name);
u16 print_dev2(struct ipr_resource_entry *p_resource_entry,
               char *p_buf,
               struct ipr_resource_flags *p_resource_flags,
               char *dev_name);
u16 print_dev_sel(struct ipr_resource_entry *p_resource_entry, char *p_buf);
u16 print_dev_pty(struct ipr_resource_entry *p_resource_entry, char *p_buf, char *dev_name);
u16 print_dev_pty2(struct ipr_resource_entry *p_resource_entry, char *p_buf, char *dev_name);
int device_concurrent_maintenance();
int issue_concurrent_maintenance(char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                                 char *p_location, char *p_pci_bus, char *p_pci_device,
                                 char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun,
                                 char *p_action, char *p_delay);
int is_valid_device(char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                    char *p_location, char *p_pci_bus, char *p_pci_device,
                    char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun);
u16 get_model(struct ipr_resource_flags *p_resource_flags);
int not_implemented();
int init_device();
int confirm_init_device();
void handle_sig_int(int dummy);
void handle_sig_term(int signal);
void handle_sig_winch(int dummy);
int open_dev(struct ipr_ioa *p_ioa, struct ipr_device *p_device);
void free_screen(struct panel_l *p_panel, struct window_l *p_win, FIELD **p_fields);
int confirm_reclaim(struct ipr_ioa *p_reclaim_ioa);
int reclaim_result(struct ipr_ioa *p_reclaim_ioa);
int device_details(struct device_detail_struct *p_detail_device);
int disk_unit_details();
int device_extended_vpd(struct device_detail_struct *p_detail_device);
int select_log_file(const struct dirent *p_dir_entry);
int compare_log_file(const void *pp_first, const void *pp_second);
void revalidate_devs(struct ipr_ioa *p_ioa);
void revalidate_dev(struct ipr_device *p_ipr_device, struct ipr_ioa *p_ioa, int force);
void evaluate_device(struct ipr_device *p_ipr_device, struct ipr_ioa *p_ioa, int change_size);
void flush_stdscr();
int flush_line();
void disk_unit_recovery();
int concurrent_maintenance_warning(struct ipr_ioa *p_ioa,
                                   struct ipr_device *p_ipr_device,
                                   char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                                   char *p_location, char *p_pci_bus, char *p_pci_device,
                                   char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun,
                                   char *p_action, u32 delay);
int do_dasd_conc_maintenance(struct ipr_ioa *p_ioa,
                             struct ipr_device *p_ipr_device,
                             char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                             char *p_location, char *p_pci_bus, char *p_pci_device,
                             char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun,
                             char *p_action, u32 delay, int replace_dev);
int conc_maintenance_complt(struct ipr_ioa *p_ioa,
                            struct ipr_device *p_ipr_device,
                            char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                            char *p_location, char *p_pci_bus, char *p_pci_device,
                            char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun,
                            char *p_action, u32 delay, int replace_dev);
int conc_maintenance_failed(char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                            char *p_location, char *p_pci_bus, char *p_pci_device,
                            char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun, char *p_action, int rc);
int is_array_in_use(struct ipr_ioa *p_cur_ioa, u8 array_id);
int display_menu(ITEM **menu_item,
                 int menu_start_row,
                 int menu_index_max,
                 int **userptr);
int print_parity_status(char *buffer,
                        u8 array_id,
                        struct ipr_query_res_state *p_res_state,
                        struct ipr_cmd_status *p_cmd_status);

int main(int argc, char *argv[])
{
    int main_menu();
    int next_editor, next_dir, next_platform, i;
    char parm_editor[200], parm_dir[200], platform_str[200];
    bool platform_override = false;
 
    p_head_ipr_ioa = p_last_ipr_ioa = NULL;

    setenv("TERM", "vt100", 1);

    strcpy(parm_dir, DEFAULT_LOG_DIR);
    strcpy(parm_editor, DEFAULT_EDITOR);

    openlog("iprconfig",
            LOG_PID |     /* Include the PID with each error */
            LOG_CONS,     /* Write to system console if there is an error
                           sending to system logger */
            LOG_USER);

    if (argc > 1)
    {
        next_editor = 0;
        next_dir = 0;
        next_platform = 0;
        for (i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "-e") == 0)
                next_editor = 1;
            else if (strcmp(argv[i], "-k") == 0)
                next_dir = 1;
            else if (strcmp(argv[i], "-p") == 0)
                next_platform = 1;
            else if (strcmp(argv[i], "--version") == 0)
            {
                printf("iprconfig: %s"IPR_EOL, IPR_VERSION_STR);
                exit(1);
            }
            else if (next_editor)
            {
                strcpy(parm_editor, argv[i]);
                next_editor = 0;
            }
            else if (next_dir)
            {
                strcpy(parm_dir, argv[i]);
                next_dir = 0;
            }
            else if (next_platform)
            {
                strcpy(platform_str, argv[i]);
                next_platform = 0;
                platform_override = true;
            }
            else
            {
                printf("Usage: iprconfig [options]"IPR_EOL);
                printf("  Options: -e name    Default editor for viewing error logs"IPR_EOL);
                printf("           -k dir     Kernel messages root directory"IPR_EOL);
                printf("           --version  Print iprconfig version"IPR_EOL);
                printf("  Use quotes around parameters with spaces"IPR_EOL);
                exit(1);
            }
        }
    }

    strcpy(log_root_dir, parm_dir);
    strcpy(editor, parm_editor);

    tool_init("iprconfig");

    if (platform_override)
    {
        if (strcasecmp(platform_str, "iseries") == 0)
            platform = IPR_ISERIES;
        else if (strcasecmp(platform_str, "pseries") == 0)
            platform = IPR_PSERIES;
        else
            platform = IPR_GENERIC;
    }

    /* Do ncurses initialization stuff */
    curses_info.init_win = initscr();
    getmaxyx(curses_info.init_win, max_y, max_x);
    getbegyx(curses_info.init_win, start_y, start_x);
    if (max_y < 16)
    {
	endwin();
	printf("ERROR: iprconfig requires a window size containing at least 16 rows"IPR_EOL);
	printf("       current number of rows is %d"IPR_EOL, max_y);
	syslog(LOG_ERR, "Fatal Error! iprconfig requires a window size containing at least 16 rows"IPR_EOL);
	exit(1);
    }
    curses_info.max_y = max_y;
    curses_info.max_x = max_x;
    curses_info.start_y = start_y;
    curses_info.start_x = start_x;
    cbreak();  /* take input chars one at a time, no wait for \n */
    noecho();
    nonl();
    keypad(stdscr, TRUE);

    signal(SIGINT, handle_sig_int);
    signal(SIGQUIT, handle_sig_term);
    signal(SIGTERM, handle_sig_term);
    signal(SIGWINCH, handle_sig_winch);

    while(RC_REFRESH == main_menu())
    {
    }

    return 0;
}

int main_menu()
{
    /*
                                      Work with Disk Units

        Select one of the following:

             1. Display disk hardware status
             2. Work with device parity protection
             3. Work with disk unit recovery
             4. Work with configuration
             5. Analyze Log



        Selection:

        e=Exit

     */
    int rc;
    FIELD *p_fields[3];
    FORM *p_form;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_cmd_status cmd_status;
    char init_message[60];
    int fd, fdev, k, j, ch2;
    struct ipr_cmd_status_record *p_status_record;
    int num_devs = 0;
    int num_stops = 0;
    int num_starts = 0;
    struct ipr_ioa *p_cur_ioa;
    struct devs_to_init_t *p_cur_dev_to_init;
    struct array_cmd_data *p_cur_raid_cmd;
    u8 ioctl_buffer[255], *p_byte;
    struct sense_data_t sense_data;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    struct ipr_block_desc *p_block_desc;
    int loop, len, retries;

    void raid_screen();
    int disk_status();
    int reclaim_cache();
    int dev_init_complete(u8 num_devs);
    void log_menu();
    int raid_status();
    int raid_stop_complete();
    int raid_start_complete();
    void configure_options();

    init_message[0] = '\0';

    check_current_config(false);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        p_cur_ioa->num_raid_cmds = 0;

        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            continue;
        }

        /* Do a query command status to see if any long running array commands
         are currently running */
        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.buffer = &cmd_status;
        ioa_cmd.buffer_len = sizeof(cmd_status);
        ioa_cmd.read_not_write = 1;

        rc = ipr_ioctl(fd, IPR_QUERY_COMMAND_STATUS, &ioa_cmd);

        if (rc != 0)
        {
            if (errno != EINVAL)
                syslog(LOG_ERR, "Query command status to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            close(fd);
            continue;
        }

        p_status_record = cmd_status.record;

        for (k = 0; k < cmd_status.num_records; k++, p_status_record++)
        {
            if ((p_status_record->status & IPR_CMD_STATUS_IN_PROGRESS))
            {
                if (p_status_record->command_code == IPR_START_ARRAY_PROTECTION)
                {
                    p_cur_ioa->num_raid_cmds++;
                    num_starts++;

                    if (p_head_raid_cmd)
                    {
                        p_tail_raid_cmd->p_next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                        p_tail_raid_cmd = p_tail_raid_cmd->p_next;
                    }
                    else
                    {
                        p_head_raid_cmd = p_tail_raid_cmd = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                    }

                    p_tail_raid_cmd->p_next = NULL;

                    memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

                    p_tail_raid_cmd->array_id = p_status_record->array_id;
                    p_tail_raid_cmd->p_ipr_ioa = p_cur_ioa;
                    p_tail_raid_cmd->do_cmd = 1;
                }
                else if (p_status_record->command_code == IPR_STOP_ARRAY_PROTECTION)
                {
                    p_cur_ioa->num_raid_cmds++;
                    num_stops++;

                    if (p_head_raid_cmd)
                    {
                        p_tail_raid_cmd->p_next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                        p_tail_raid_cmd = p_tail_raid_cmd->p_next;
                    }
                    else
                    {
                        p_head_raid_cmd = p_tail_raid_cmd =
                            (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                    }

                    p_tail_raid_cmd->p_next = NULL;

                    memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

                    p_tail_raid_cmd->array_id = p_status_record->array_id;
                    p_tail_raid_cmd->p_ipr_ioa = p_cur_ioa;
                    p_tail_raid_cmd->do_cmd = 1;
                }
                else if (p_status_record->command_code == IPR_ADD_ARRAY_DEVICE)
                {
                    while(RC_REFRESH == raid_status())
                    {
                    }
                }
                else if (p_status_record->command_code == IPR_REMOVE_ARRAY_DEVICE)
                {
                    while(RC_REFRESH == raid_status())
                    {
                    }
                }
            }
        }

        /* Do a query command status for all devices to check for formats in
         progress */
        for (j = 0; j < p_cur_ioa->num_devices; j++)
        {
            p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

            if (p_resource_entry->is_ioa_resource)
                continue;

            if (p_resource_entry->is_af)
            {
                /* ignore if its a volume set */
                if (p_resource_entry->subtype == IPR_SUBTYPE_VOLUME_SET)
                    continue;

                /* Do a query command status */
                ioa_cmd.buffer = &cmd_status;
                ioa_cmd.buffer_len = sizeof(cmd_status);
                ioa_cmd.read_not_write = 1;
                ioa_cmd.device_cmd = 1;
                ioa_cmd.resource_address = p_resource_entry->resource_address;

                rc = ipr_ioctl(fd, IPR_QUERY_COMMAND_STATUS, &ioa_cmd);

                if (rc != 0)
                {
                    syslog(LOG_ERR, "Query Command status to %s failed. %m"IPR_EOL, p_cur_ioa->dev[j].dev_name);
                    close(fd);
                    continue;
                }

                if (cmd_status.num_records == 0)
                {
                    /* Do nothing */
                }
                else
                {
                    if (cmd_status.record->status == IPR_CMD_STATUS_IN_PROGRESS)
                    {
                        if (strlen(p_cur_ioa->dev[j].dev_name) != 0)
                        {
                            fdev = open(p_cur_ioa->dev[j].dev_name, O_RDWR | O_NONBLOCK);

                            if (fdev)
                            {
                                fcntl(fdev, F_SETFD, 0);

                                /* Try to get an exclusive lock to the device */
                                rc = flock(fdev, LOCK_EX | LOCK_NB);
                            }

                            /* If one already exists, continue anyway */
                            if (rc && (errno != EWOULDBLOCK))
                            {
                                syslog(LOG_ERR, "Couldn't set lock on %s. %m rc=0x%x"IPR_EOL, p_cur_ioa->dev[j].dev_name, rc);
                            }
                        }
                        else
                        {
                            fdev = 0;
                        }

                        if (p_head_dev_to_init)
                        {
                            p_tail_dev_to_init->p_next = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));
                            p_tail_dev_to_init = p_tail_dev_to_init->p_next;
                        }
                        else
                            p_head_dev_to_init = p_tail_dev_to_init = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));

                        memset(p_tail_dev_to_init, 0, sizeof(struct devs_to_init_t));

                        p_tail_dev_to_init->p_ioa = p_cur_ioa;
                        p_tail_dev_to_init->dev_type = IPR_AF_DASD_DEVICE;
                        p_tail_dev_to_init->p_ipr_device = &p_cur_ioa->dev[j];
                        p_tail_dev_to_init->fd = fdev;
                        p_tail_dev_to_init->do_init = 1;

                        num_devs++;
                    }
                    else if (cmd_status.record->status == IPR_CMD_STATUS_SUCCESSFUL)
                    {
                        /* Check if evaluate device capabilities is necessary */
                        /* Issue mode sense */
                        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));
                        p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

                        ioa_cmd.buffer = p_mode_parm_hdr;
                        ioa_cmd.buffer_len = 255;
                        ioa_cmd.read_not_write = 1;
                        ioa_cmd.device_cmd = 1;
                        ioa_cmd.resource_address = p_resource_entry->resource_address;

                        rc = ipr_ioctl(fd, MODE_SENSE, &ioa_cmd);

                        if (rc != 0)
                        {
                            syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
                                   p_cur_ioa->dev[j].dev_name);
                        }
                        else
                        {
                            if (p_mode_parm_hdr->block_desc_len > 0)
                            {
                                p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr+1);

                                if((p_block_desc->block_length[1] == 0x02) && 
                                   (p_block_desc->block_length[2] == 0x00))
                                {
                                    /* Send evaluate device capabilities */
                                    remove_device(p_cur_ioa->dev[j].p_resource_entry->resource_address,
                                                  p_cur_ioa);
                                    evaluate_device(&p_cur_ioa->dev[j], p_cur_ioa, IPR_512_BLOCK);
                                }

                            }
                        }
                    }
                }
            }
            else /* JBOD */
            {
                retries = 0;

       redo_tur:
                /* Send Test Unit Ready to find percent complete in sense data. */
                memset(cdb, 0, IPR_CCB_CDB_LEN);

                if ((fdev = open(p_cur_ioa->dev[j].gen_name, O_RDWR)) < 0)
                {
                    if (strlen(p_cur_ioa->dev[j].gen_name) != 0)
                        syslog(LOG_ERR, "Unable to open device %s. %m"IPR_EOL,
                               p_cur_ioa->dev[j].gen_name);
                    continue;
                }

                cdb[0] = TEST_UNIT_READY;

                rc = sg_ioctl(fdev, cdb,
                              NULL, 0, SG_DXFER_NONE,
                              &sense_data, 30);

                if (rc < 0)
                {
                    syslog(LOG_ERR, "Could not send test unit ready to %s. %m"IPR_EOL,p_cur_ioa->dev[j].dev_name);
                }
                else if ((rc == CHECK_CONDITION) &&
                         ((sense_data.error_code & 0x7F) == 0x70) &&
                         ((sense_data.sense_key & 0x0F) == 0x02) &&  /* NOT READY */
                         (sense_data.add_sense_code == 0x04) &&      /* LOGICAL UNIT NOT READY */
                         (sense_data.add_sense_code_qual == 0x02))   /* INIT CMD REQ */
                {
                    cdb[0] = START_STOP;
                    cdb[4] = IPR_START_STOP_START;

                    rc = sg_ioctl(fdev, cdb,
                                  NULL, 0, SG_DXFER_NONE,
                                  &sense_data, 120);

                    if (retries == 0)
                    {
                        retries++;
                        goto redo_tur;
                    }
                    else
                    {
                        syslog(LOG_ERR, "Test Unit Ready failed to %s. %m"IPR_EOL,
                               p_cur_ioa->dev[j].gen_name);
                    }
                }
                else if ((rc == CHECK_CONDITION) &&
                         ((sense_data.error_code & 0x7F) == 0x70) &&
                         ((sense_data.sense_key & 0x0F) == 0x02) &&  /* NOT READY */
                         (sense_data.add_sense_code == 0x04) &&      /* LOGICAL UNIT NOT READY */
                         (sense_data.add_sense_code_qual == 0x04))   /* FORMAT IN PROGRESS */
                {
                    if (p_head_dev_to_init)
                    {
                        p_tail_dev_to_init->p_next = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));
                        p_tail_dev_to_init = p_tail_dev_to_init->p_next;
                    }
                    else
                        p_head_dev_to_init = p_tail_dev_to_init = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));

                    memset(p_tail_dev_to_init, 0, sizeof(struct devs_to_init_t));

                    p_tail_dev_to_init->p_ioa = p_cur_ioa;
                    p_tail_dev_to_init->dev_type = IPR_JBOD_DASD_DEVICE;
                    p_tail_dev_to_init->p_ipr_device = &p_cur_ioa->dev[j];

                    p_tail_dev_to_init->fd = 0;
                    p_tail_dev_to_init->do_init = 1;

                    num_devs++;
                }
                else if (rc == CHECK_CONDITION)
                {
                    len = sprintf(ioctl_buffer, "Test Unit Ready failed to %s. %m  ",
                                  p_cur_ioa->dev[j].gen_name);

                    p_byte = (u8 *)&sense_data;

                    for (loop = 0; loop < 18; loop++)
                    {
                        len += sprintf(ioctl_buffer + len,"%.2x ", p_byte[loop]);
                    }

                    syslog(LOG_ERR, "sense data = %s"IPR_EOL, ioctl_buffer);
                }
                else
                {
                    /* Check if evaluate device capabilities needs to be issued */
                    memset(cdb, 0, IPR_CCB_CDB_LEN);

                    /* Issue mode sense to get the block size */
                    cdb[0] = MODE_SENSE;
                    cdb[2] = 0x0a; /* PC = current values, page code = 0ah */
                    cdb[4] = 255;

                    rc = sg_ioctl(fdev, cdb,
                                  &ioctl_buffer, 255, SG_DXFER_FROM_DEV,
                                  &sense_data, 30);

                    if (rc < 0)
                    {
                        syslog(LOG_ERR, "Could not send mode sense to %s. %m"IPR_EOL,
                               p_cur_ioa->dev[j].gen_name);
                    }
                    else if (rc == CHECK_CONDITION)
                    {
                        len = sprintf(ioctl_buffer,"Mode Sense failed to %s. %m  ",
                                      p_cur_ioa->dev[j].gen_name);

                        p_byte = (u8 *)&sense_data;

                        for (loop = 0; loop < 18; loop++)
                        {
                            len += sprintf(ioctl_buffer + len,"%.2x ",p_byte[loop]);
                        }

                        syslog(LOG_ERR, "sense data = %s"IPR_EOL, ioctl_buffer);
                    }
                    else
                    {
                        p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

                        if (p_mode_parm_hdr->block_desc_len > 0)
                        {
                            p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr+1);

                            if ((!(p_block_desc->block_length[1] == 0x02) ||
                                 !(p_block_desc->block_length[2] == 0x00)) &&
                                (p_cur_ioa->p_qac_data->num_records != 0))
                            {
                                /* send evaluate device */
                                evaluate_device(&p_cur_ioa->dev[j], p_cur_ioa, IPR_522_BLOCK);
                            }
                            else
                            {
                                if (strlen(p_cur_ioa->dev[j].dev_name) == 0)
                                {
                                    revalidate_dev(&p_cur_ioa->dev[j],
                                                   p_cur_ioa, 1);
                                }
                            }
                        }
                    }
                }
                close(fdev);
            }
        }
        close(fd);
    }

    if (num_devs)
    {
        rc = dev_init_complete(num_devs);

        if (rc == RC_SUCCESS)
            sprintf(init_message, " Initialize and format completed successfully");
        else if (rc == RC_FAILED)
            sprintf(init_message, " Initialize and format failed");
    }
    else if (num_stops)
    {
        rc = raid_stop_complete();

        if (rc == RC_SUCCESS)
            sprintf(init_message, "Stop Parity Protection completed successfully");
        else if (rc == RC_FAILED)
            sprintf(init_message, "Stop Parity Protection failed");
    }
    else if (num_starts)
    {
        rc = raid_start_complete();

        if (rc == RC_SUCCESS)
            sprintf(init_message, "Start Parity Protection completed successfully");
        else if (rc == RC_FAILED)
            sprintf(init_message, "Start Parity Protection failed");
    }
    else
        revalidate_devs(NULL);

    p_cur_dev_to_init = p_head_dev_to_init;
    while(p_cur_dev_to_init)
    {
        p_cur_dev_to_init = p_cur_dev_to_init->p_next;
        free(p_head_dev_to_init);
        p_head_dev_to_init = p_cur_dev_to_init;
    }

    p_cur_raid_cmd = p_head_raid_cmd;
    while(p_cur_raid_cmd)
    {
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }

    /* Title */
    p_fields[0] = new_field(1, max_x - start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            12, 12,
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Work with Disk Units");          /* string value to set */

    clear();
    refresh();
    post_form(p_form);

    mvaddstr(curses_info.max_y - 1, 0, init_message); 

    while(1)
    {
        mvaddstr(2, 0, "Select one of the following:");
        mvaddstr(4, 0, "     1. Display disk hardware status");
        mvaddstr(5, 0, "     2. Work with device parity protection");
        mvaddstr(6, 0, "     3. Work with disk unit recovery");
        mvaddstr(7, 0, "     4. Work with configuration");
        mvaddstr(8, 0, "     5. Analyze Log");
        mvaddstr(12, 0, "Selection: ");
        mvaddstr(curses_info.max_y - 2, 0, EXIT_KEY_LABEL"=Exit");

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        refresh();
        for (;;)
        {
            int ch = getch();

            if (IS_ENTER_KEY(ch))
            {
                char *p_char;
                unpost_form(p_form);

                nodelay(stdscr, TRUE);
                ch2 = getch();
                nodelay(stdscr, FALSE);

                if (IS_5250_CHAR(ch2))
                    term_is_5250 = 1;

                p_char = field_buffer(p_fields[1], 0);
                switch(p_char[0])
                {
                    case '1':
                        while (RC_REFRESH == (rc = disk_status()))
                            clear();
                        clear();
                        post_form(p_form);
                        break;
                    case '2':
                        raid_screen();
                        clear();
                        post_form(p_form);
                        break;
                    case '3':
                        disk_unit_recovery();
                        clear();
                        post_form(p_form);
                        break;
                    case '4':
                        configure_options();
                        clear();
                        post_form(p_form);
                        break;
                    case '5':
                        log_menu();
                        clear();
                        post_form(p_form);
                        form_driver(p_form,               /* form to pass input to */
                                    REQ_LAST_FIELD );     /* form request code */
                        break;
                    case ' ':
                        clear();
                        post_form(p_form);
                        break;
                    default:
                        clear();
                        post_form(p_form);
                        mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
                        break;
                }
                form_driver(p_form, ' ');
                break;
            }
            else if (IS_EXIT_KEY(ch) || IS_CANCEL_KEY(ch))
            {
                unpost_form(p_form);
                free_form(p_form);

                for (j=0; j < 2; j++)
                    free_field(p_fields[j]);
                clear();
                refresh();
                endwin();
                exit(EXIT_SUCCESS);
                return 0;
            }
            else if (( ch == '1') || (ch == '2') || (ch == '3') || (ch == '4') || (ch == '5'))
            {
                form_driver(p_form, ch);
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                refresh();
            }
        }
    }
}

int disk_status()
{
#define DISK_STATUS_HDR_SIZE		4
#define DISK_STATUS_MENU_SIZE		5

    /* iSeries
                                  Display Disk Hardware Status

         Serial                Resource                                Hardware
         Number   Type  Model  Name                                    Status
        02137002  2757   001   /dev/ipr6                            Operational
        03060044  5703   001   /dev/ipr5                            Not Operational
        02127005  2782   001   /dev/ipr4                            Operational
        002443C6  6713   072   /dev/sdr                                DPY/Active
        0024B45C  6713   072   /dev/sds                                DPY/Active
        00000DC7  6717   072   /dev/sdt                                DPY/Active
        000BBBC1  6717   072   /dev/sdu                                DPY/Active
        0002F5AA  6717   070   /dev/sdv                                DPY/Active
        02123028  5702   001   /dev/ipr3                            Operational
        09112004  2763   001   /dev/ipr2                            Operational
        0007438C  6607   050   /dev/sdm                                Perf Degraded
        0006B81C  6607   050   /dev/sdn                                Perf Degraded
        00224BF2  6713   050   /dev/sdo                                Perf Degraded
        00224ABE  6713   050   /dev/sdp                                Perf Degraded
                                                                                More...
        Press Enter to continue.

        e=Exit           q=Cancel         r=Refresh   f=PageDn   b=PageUp
        d=Display disk unit details       p=Display device parity status
     */

/* non-iSeries
                          Display Disk Hardware Status

 Serial   Vendor   Product                  Resource           Hardware
 Number    ID        ID              Model  Name               Status
03079003  IBM      2780001            001   /dev/ipr2       Operational
0024A0B9  IBMAS400 DCHS09W            070                      DPY/Active
00D37A11  IBMAS400 DFHSS4W            070                      DPY/Active
00D38D0F  IBMAS400 DFHSS4W            070                      DPY/Active
00034563  IBMAS400 DGVS09U            070                      DPY/Active
00D32240  IBMAS400 DFHSS4W            050                      Operational
443EA8CA  IBM      01BB2952           200   /dev/sdb           Operational
A014696F  IBM      211CA8B1           200   /dev/sdc           Operational
02056069  IBM      5702001            001   /dev/ipr1       Operational
02123012  IBM      5702001            001   /dev/ipr0       Operational






Press Enter to continue.

e=Exit           q=Cancel         r=Refresh   f=PageDn   b=PageUp
d=Display disk unit details       p=Display device parity status

*/

    int fd, rc, array_num, j;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    int len = 0;
    char buffer[100];
    int max_size = curses_info.max_y - (DISK_STATUS_HDR_SIZE + DISK_STATUS_MENU_SIZE + 1);
    int display_index = 0;
    struct ipr_query_res_state res_state;
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    PANEL *p_panel;
    u8 ioctl_buffer[255];
    int status;
    int format_req = 0;
    struct ipr_resource_entry *p_resource_entry;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    struct ipr_block_desc *p_block_desc;
    struct sense_data_t sense_data;
    u8 product_id[IPR_PROD_ID_LEN+1];
    u8 vendor_id[IPR_VENDOR_ID_LEN+1];

    int raid_status();

    rc = RC_SUCCESS;

    p_title_win = newwin(DISK_STATUS_HDR_SIZE,
                         curses_info.max_x,
                         curses_info.start_y,
                         curses_info.start_x);

    p_menu_win = newwin(DISK_STATUS_MENU_SIZE,
                        curses_info.max_x,
                        curses_info.start_y + max_size + DISK_STATUS_HDR_SIZE + 1,
                        curses_info.start_x);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */


    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Display Disk Hardware Status");          /* string value to set */

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + DISK_STATUS_HDR_SIZE,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    clear();
    post_form(p_form);

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 2, 0, " Serial                Resource                                Hardware");
        mvwaddstr(p_title_win, 3, 0, " Number   Type  Model  Name                                    Status");
    }
    else
    {
        mvwaddstr(p_title_win, 2, 0, " Serial   Vendor   Product                  Resource           Hardware");
        mvwaddstr(p_title_win, 3, 0, " Number    ID        ID              Model  Name               Status");
    }

    mvwaddstr(p_menu_win, 0, 0, "Press Enter to continue.");
    mvwaddstr(p_menu_win,
              2, 0, EXIT_KEY_LABEL"=Exit           "CANCEL_KEY_LABEL"=Cancel         r=Refresh   f=PageDn   b=PageUp");
    mvwaddstr(p_menu_win, 3, 0, "d=Display disk unit details       p=Display device parity status");

    check_current_config(false);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        array_num = 1;
        if (p_cur_ioa->ioa.p_resource_entry == NULL)
            continue;

        if (platform == IPR_ISERIES)
        {
            len = sprintf(buffer, "%8s  %X   %03d   %-39s ",
                          p_cur_ioa->serial_num, p_cur_ioa->ccin, 1, p_cur_ioa->ioa.dev_name);
        }
        else
        {
            memcpy(vendor_id, p_cur_ioa->ioa.p_resource_entry->std_inq_data.vpids.vendor_id,
                   IPR_VENDOR_ID_LEN);
            vendor_id[IPR_VENDOR_ID_LEN] = '\0';
            memcpy(product_id, p_cur_ioa->ioa.p_resource_entry->std_inq_data.vpids.product_id,
                   IPR_PROD_ID_LEN);
            product_id[IPR_PROD_ID_LEN] = '\0';

            len = sprintf(buffer, "%8s  %-8s %-18s %03d   %-18s ",
                          p_cur_ioa->serial_num,
                          vendor_id,
                          product_id,
                          p_cur_ioa->ioa.p_resource_entry->model, p_cur_ioa->ioa.dev_name);
        }

        if (p_cur_ioa->ioa.p_resource_entry->ioa_dead)
            len += sprintf(buffer+len, "Not Operational");
        else if (p_cur_ioa->ioa.p_resource_entry->nr_ioa_microcode)
            len += sprintf(buffer+len, "Not Ready");
        else
            len += sprintf(buffer+len, "Operational");

        if ((num_lines > 0) && ((num_lines % max_size) == 0))
        {
            mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

            p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
            p_tail_win = p_tail_win->p_next;

            p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
            p_tail_panel = p_tail_panel->p_next;

            p_tail_win->p_win = newwin(max_size + 1,
                                       curses_info.max_x,
                                       curses_info.start_y + DISK_STATUS_HDR_SIZE,
                                       curses_info.start_x);
            p_tail_win->p_next = NULL;

            p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
            p_tail_panel->p_next = NULL;
        }

        mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);

        memset(buffer, 0, 100);

        len = 0;
        num_lines++;

        for (j = 0; j < p_cur_ioa->num_devices; j++)
        {
            p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
            if (p_resource_entry->is_ioa_resource)
            {
                continue;
            }

            if (p_resource_entry->is_af)
            {
                fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

                /* Do a query resource state */
                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                ioa_cmd.buffer = &res_state;
                ioa_cmd.buffer_len = sizeof(res_state);
                ioa_cmd.read_not_write = 1;
                ioa_cmd.device_cmd = 1;
                ioa_cmd.resource_address = p_resource_entry->resource_address;

                memset(&res_state, 0, sizeof(res_state));

                rc = ipr_ioctl(fd, IPR_QUERY_RESOURCE_STATE, &ioa_cmd);

                if (rc != 0)
                {
                    syslog(LOG_ERR,"Query Resource State failed. %m"IPR_EOL);
                    iprlog_location(p_resource_entry);
                    res_state.not_oper = 1;
                }
                close(fd);
            }
            else /* JBOD */
            {
                memset(cdb, 0, IPR_CCB_CDB_LEN);
                memset(&res_state, 0, sizeof(res_state));

                format_req = 0;

                /* Issue mode sense/mode select to determine if device needs to be formatted */
                cdb[0] = MODE_SENSE;
                cdb[2] = 0x0a; /* PC = current values, page code = 0ah */
                cdb[4] = 255;

                fd = open(p_cur_ioa->dev[j].gen_name, O_RDONLY);

                if (fd <= 1)
                {
                    if (strlen(p_cur_ioa->dev[j].gen_name) != 0)
                        syslog(LOG_ERR, "Could not open file %s. %m"IPR_EOL, p_cur_ioa->dev[j].gen_name);
                    res_state.not_oper = 1;
                }
                else
                {
                    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
                    memset(&sense_data, 0, sizeof(sense_data));

                    status = sg_ioctl(fd, cdb,
                                      &ioctl_buffer, 255, SG_DXFER_FROM_DEV,
                                      &sense_data, 30);

                    if (status)
                    {
                        syslog(LOG_ERR, "Could not send mode sense to %s. %m"IPR_EOL, p_cur_ioa->dev[j].gen_name);
                        res_state.not_oper = 1;
                    }
                    else
                    {
                        if (p_mode_parm_hdr->block_desc_len > 0)
                        {
                            p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr+1);

                            if ((p_block_desc->block_length[1] != 0x02) ||
                                (p_block_desc->block_length[2] != 0x00))
                            {
                                format_req = 1;
                            }
                            else
                            {
                                /* check device with test unit ready */
                                memset(cdb, 0, IPR_CCB_CDB_LEN);
                                cdb[0] = TEST_UNIT_READY;

                                rc = sg_ioctl(fd, cdb,
                                              NULL, 0, SG_DXFER_NONE,
                                              &sense_data, 30);

                                if ((rc < 0) || (rc == CHECK_CONDITION))
                                {
                                    format_req = 1;
                                }
                            }
                        }
                    }

                    close(fd);
                }
            }

            len = print_dev(p_resource_entry,
                            buffer,
                            p_cur_ioa->dev[j].dev_name);

            if (res_state.not_oper)
                len += sprintf(buffer + len, "Not Operational");
            else if (res_state.not_ready ||
                     (res_state.read_write_prot && (iprtoh32(res_state.failing_dev_ioasc) == 0x02040200u)))
                len += sprintf(buffer + len, "Not ready");
            else if ((res_state.read_write_prot) || (p_resource_entry->rw_protected))
                len += sprintf(buffer + len, "R/W Protected");
            else if (res_state.prot_dev_failed)
                len += sprintf(buffer + len, "DPY/Failed");
            else if (res_state.prot_suspended)
                len += sprintf(buffer + len, "DPY/Unprotected");
            else if (res_state.prot_resuming)
                len += sprintf(buffer + len, "DPY/Rebuilding");
            else if (res_state.degraded_oper)
                len += sprintf(buffer + len, "Perf Degraded");
            else if (res_state.service_req)
                len += sprintf(buffer + len, "Redundant Hw Fail");
            else if (format_req)
                len += sprintf(buffer + len, "Format Required");
            else
            {
                if (p_resource_entry->is_array_member)
                    len += sprintf(buffer + len, "DPY/Active");
                else if (p_resource_entry->is_hot_spare)
                    len += sprintf(buffer + len, "HS/Active");
                else
                    len += sprintf(buffer + len, "Operational");
            }

            if ((num_lines > 0) && ((num_lines % max_size) == 0))
            {
                mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                p_tail_win = p_tail_win->p_next;

                p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                p_tail_panel = p_tail_panel->p_next;

                p_tail_win->p_win = newwin(max_size + 1,
                                           curses_info.max_x,
                                           curses_info.start_y + DISK_STATUS_HDR_SIZE,
                                           curses_info.start_x);
                p_tail_win->p_next = NULL;

                p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                p_tail_panel->p_next = NULL;
            }

            mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);

            memset(buffer, 0, 100);

            len = 0;
            num_lines++;
        }
    }

    if (num_lines == 0)
        mvwaddstr(p_tail_win->p_win, 1, 2, "(No devices found)");

    display_index = 0;

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        update_panels();
        wrefresh(p_menu_win);
        wrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if (IS_REFRESH_KEY(ch))
            {
                rc = RC_REFRESH;
                goto leave;
            }
            else if ((ch == 'd') || (ch == 'D'))
            {
                unpost_form(p_form);
                free_form(p_form);
                curses_info.p_form = p_old_form;

                free_screen(p_head_panel, p_head_win, p_fields);
                delwin(p_title_win);
                delwin(p_menu_win);
                werase(stdscr);

                if (disk_unit_details() != RC_EXIT)
                    return RC_REFRESH;
                else
                    return RC_EXIT;
            }
            else if ((ch == 'p') || (ch == 'P'))
            {
                unpost_form(p_form);
                free_form(p_form);
                curses_info.p_form = p_old_form;
                free_screen(p_head_panel, p_head_win, p_fields);
                delwin(p_title_win);
                delwin(p_menu_win);
                while (RC_REFRESH == (rc = raid_status()))
                {
                    clear();
                    refresh();
                }
                if (RC_CANCEL == rc)
                    return RC_REFRESH;
                return RC_EXIT;
            }
            else if (IS_PGDN_KEY(ch))
            {
                flush_stdscr();
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                flush_stdscr();
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_5250_CHAR(ch))
            {
                /* Ignore this - we will get this when running under 5250 telnet */
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                update_panels();
                wrefresh(p_menu_win);
                wrefresh(p_title_win);
                doupdate();
            }
            else
            {
                mvwaddstr(p_menu_win, 4, 0, "Invalid option specified.");
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                update_panels();
                wrefresh(p_menu_win);
                wrefresh(p_title_win);
                doupdate();
            }
        }
    }

    leave:
        flush_stdscr();
    unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);

    return rc;
}

/* Completely flush any characters in the input buffer */
void flush_stdscr()
{
    nodelay(stdscr, TRUE);

    while(getch() != ERR)
    {
    }
    nodelay(stdscr, FALSE);
}

/* Flush the input buffer until we hit either an eol or run
 out of characters in the input buffer */
int flush_line()
{
    int ch;

    nodelay(stdscr, TRUE);

    do
    {
        ch = getch();
    } while (( ch != ERR) && !IS_5250_CHAR(ch));

    nodelay(stdscr, FALSE);

    return ch;
}

int disk_unit_details()
{
#define DISK_UD_HDR_SIZE		7
#define DISK_UD_MENU_SIZE		5
    /* iSeries
                           Display Disk Unit Details  

     Type option, press Enter.
     5=Display hardware resource information details

                Serial                        Sys    Sys    I/O   I/O
     OPT        Number   DSA/UA               Bus    Card Adapter Bus   Ctl  Dev
              00234019  00181E00/0FFFFFFF     24     30      0
              00058B10  00181E00/010700FF     24     30      0     1     7    0 
              00055B7A  00181E00/010600FF     24     30      0     1     6    0 
              0005935E  00181E00/010500FF     24     30      0     1     5    0  
              00055BF8  00181E00/010400FF     24     30      0     1     4    0 
              000585BA  00181E00/010300FF     24     30      0     1     3    0 
              09228053  00181B00/0FFFFFFF     24     27      0
              000564FD  00181B00/020700FF     24     27      0     2     7    0 
              000585F1  00181B00/020600FF     24     27      0     2     6    0 
              000591FB  00181B00/020500FF     24     27      0     2     5    0
              00056880  00181B00/020400FF     24     27      0     2     4    0
              000595AD  00181B00/020300FF     24     27      0     2     3    0
     More...



     e=Exit          q=Cancel   f=PageDn   b=PageUp

     */
    /* pSeries
                            Display Disk Unit Details  

     Type option, press Enter.
     5=Display hardware resource information details

             Serial                        PCI    PCI   SCSI  SCSI  SCSI  SCSI
     OPT     Number  Location              Bus    Dev   Host   Bus   ID   Lun
           00234019  P1-I1                 02     01      0
           00058B10  P1-I1/Z1-A0           02     01      0     1     0    0 
           00055B7A  P1-I1/Z1-A1           02     01      0     1     1    0 
           0005935E  P1-I1/Z1-A2           02     01      0     1     2    0  
           00055BF8  P1-I1/Z1-A3           02     01      0     1     3    0 
           000585BA  P1-I1/Z1-A4           02     01      0     1     4    0 
           09228053  P1-I2                 02     02      1
           000564FD  P1-I2/Z2-A0           02     02      1     2     0    0 
           000585F1  P1-I2/Z2-A1           02     02      1     2     1    0 
           000591FB  P1-I2/Z2-A2           02     02      1     2     2    0
           00056880  P1-I2/Z2-A3           02     02      1     2     3    0
           000595AD  P1-I2/Z2-A4           02     02      1     2     4    0
     More...



     e=Exit          q=Cancel   f=PageDn   b=PageUp

     */
    /* Generic
                            Display Disk Unit Details  

     Type option, press Enter.
     5=Display hardware resource information details

             Serial    PCI    PCI   SCSI  SCSI  SCSI  SCSI
     OPT     Number    Bus    Dev   Host   Bus   ID   Lun
           00234019    02     01      0
           00058B10    02     01      0     1     0    0 
           00055B7A    02     01      0     1     1    0 
           0005935E    02     01      0     1     2    0  
           00055BF8    02     01      0     1     3    0 
           000585BA    02     01      0     1     4    0 
           09228053    02     02      1
           000564FD    02     02      1     2     0    0 
           000585F1    02     02      1     2     1    0 
           000591FB    02     02      1     2     2    0
           00056880    02     02      1     2     3    0
           000595AD    02     02      1     2     4    0
     More...



     e=Exit          q=Cancel   f=PageDn   b=PageUp

     */
    int rc, array_num, j, i, last_char;
    struct ipr_resource_entry *p_resource;
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form, *p_list_form;
    int len = 0;
    int max_size = curses_info.max_y - (DISK_UD_HDR_SIZE + DISK_UD_MENU_SIZE);
    char buffer[100], dev_sn[15], *p_char;
    struct ipr_ioa *p_cur_ioa;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    int num_devs = 0;
    int found = 0;
    int dev_num, invalid;
    struct device_detail_struct detail_device;
    int init_index, init_page, new_page;
    u32 dsa, ua;
    struct ipr_res_addr res_addr;

    rc = RC_SUCCESS;

    p_title_win = newwin(DISK_UD_HDR_SIZE, curses_info.max_x,
                         curses_info.start_y, curses_info.start_x);

    p_menu_win = newwin(DISK_UD_MENU_SIZE,
                        curses_info.max_x,
                        curses_info.start_y + max_size + DISK_UD_HDR_SIZE,
                        curses_info.start_x);

    p_field_win = newwin(max_size, curses_info.max_x, DISK_UD_HDR_SIZE, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */


    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Display Disk Unit Details");          /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    clear();
    post_form(p_form);

    check_current_config(false);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        if (p_cur_ioa->ioa.p_resource_entry == NULL)
            continue;

        start_y = num_devs % (max_size - 1);
        if ((num_devs > 0) && (start_y == 0))
        {
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, curses_info.max_x,
                                              max_size - 1,
                                              0,
                                              0, 0);

            set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
            set_field_buffer(pp_field[field_index], 0, "More... ");
            set_field_userptr(pp_field[field_index], NULL);
            field_opts_off(pp_field[field_index++],
                           O_ACTIVE);
        }

        p_resource = p_cur_ioa->ioa.p_resource_entry;

        if (platform == IPR_ISERIES)
        {
            len = sprintf(buffer, "%8s  %08X/%08X",
                          p_cur_ioa->serial_num,
                          p_resource->dsa,
                          p_resource->unit_address);

            /* Sys Bus number */
            len += sprintf(buffer + len,
                           "%7d ", IPR_GET_SYS_BUS(p_resource->dsa));

            /* Sys Card */
            len += sprintf(buffer + len,
                           "%6d ", IPR_GET_SYS_CARD(p_resource->dsa));

            /* I/O Adapter */
            len += sprintf(buffer + len,
                           "%6d ", IPR_GET_IO_ADAPTER(p_resource->dsa));
        }
        else if (platform == IPR_PSERIES)
        {
            len = sprintf(buffer, "%8s  %-22s",
                          p_cur_ioa->serial_num,
                          p_resource->pseries_location);

            /* PCI Bus number */
            len += sprintf(buffer + len,
                           "%02d ", p_resource->pci_bus_number);

            /* PCI Device number */
            len += sprintf(buffer + len,
                           "    %02d ", p_resource->pci_slot);

            /* SCSI Host */
            len += sprintf(buffer + len,
                           "%6d ", p_resource->host_no);
        }
        else
        {
            len = sprintf(buffer, "%8s    ",
                          p_cur_ioa->serial_num);

            /* PCI Bus number */
            len += sprintf(buffer + len,
                           "%02d ", p_resource->pci_bus_number);

            /* PCI Device number */
            len += sprintf(buffer + len,
                           "    %02d ", p_resource->pci_slot);

            /* SCSI Host */
            len += sprintf(buffer + len,
                           "%6d ", p_resource->host_no);
        }

        /* User entry field */
        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = new_field(1, 1, start_y, 1, 0, 0);

        set_field_userptr(pp_field[field_index++], (char *)p_resource);

        /* Description field */
        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = new_field(1,
                                          curses_info.max_x - DISK_UD_HDR_SIZE, 
                                          start_y, DISK_UD_HDR_SIZE, 0, 0);

        set_field_buffer(pp_field[field_index], 0, buffer);

        field_opts_off(pp_field[field_index], /* field to alter */
                       O_ACTIVE);             /* attributes to turn off */ 

        set_field_userptr(pp_field[field_index++], NULL);

        memset(buffer, 0, 100);

        len = 0;
        num_devs++;

        array_num = 1;

        for (j = 0; j < p_cur_ioa->num_devices; j++)
        {
            if (p_cur_ioa->dev[j].p_resource_entry->is_ioa_resource)
            {
                continue;
            }

            start_y = num_devs % (max_size - 1);

            if ((num_devs > 0) && (start_y == 0))
            {
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, curses_info.max_x,
                                                  max_size - 1,
                                                  0,
                                                  0, 0);

                set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                set_field_buffer(pp_field[field_index], 0, "More... ");
                set_field_userptr(pp_field[field_index], NULL);
                field_opts_off(pp_field[field_index++], O_ACTIVE);
            }

            strncpy(dev_sn, (char *)&p_cur_ioa->dev[j].p_resource_entry->std_inq_data.serial_num, 8);
            dev_sn[8] = '\0';

            len = sprintf(buffer, "%8s  ", dev_sn);

            res_addr = p_cur_ioa->dev[j].p_resource_entry->resource_address;

            if (platform == IPR_ISERIES)
            {
                len += sprintf(buffer + len,
                               "%08X/%08X",
                               p_cur_ioa->dev[j].p_resource_entry->dsa,
                               p_cur_ioa->dev[j].p_resource_entry->unit_address);

                dsa = p_cur_ioa->dev[j].p_resource_entry->dsa;
                ua = p_cur_ioa->dev[j].p_resource_entry->unit_address;

                /* Sys Bus number */
                len += sprintf(buffer + len,
                               "%7d ", IPR_GET_SYS_BUS(dsa));

                /* Sys Card */
                len += sprintf(buffer + len,
                               "%6d ", IPR_GET_SYS_CARD(dsa));

                /* I/O Adapter */
                len += sprintf(buffer + len,
                               "%6d ", IPR_GET_IO_ADAPTER(dsa));

                /* I/O Bus */
                len += sprintf(buffer + len,
                               "%5d ", IPR_GET_IO_BUS(ua));

                /* Ctl */
                len += sprintf(buffer + len,
                               "%5d ", IPR_GET_CTL(ua));

                /* Dev */
                len += sprintf(buffer + len,
                               "%4d ", IPR_GET_DEV(ua));
            }
            else
            {
                if (platform == IPR_PSERIES)
                {
                    len += sprintf(buffer + len,
                                   "%-22s",
                                   p_cur_ioa->dev[j].p_resource_entry->pseries_location);
                }
                else
                {
                    len += sprintf(buffer + len, "  ");
                }

                /* PCI Bus number */
                len += sprintf(buffer + len,
                               "%02d ", p_cur_ioa->dev[j].p_resource_entry->pci_bus_number);

                /* PCI Device number */
                len += sprintf(buffer + len,
                               "    %02d ", p_cur_ioa->dev[j].p_resource_entry->pci_slot);

                /* SCSI Host */
                len += sprintf(buffer + len,
                               "%6d ", p_cur_ioa->dev[j].p_resource_entry->host_no);

                /* SCSI Bus */
                len += sprintf(buffer + len,
                               "%5d ", res_addr.bus);

                /* SCSI Target */
                len += sprintf(buffer + len,
                               "%5d ", res_addr.target);

                /* SCSI Lun */
                len += sprintf(buffer + len,
                               "%4d ", res_addr.lun);
            }

            /* User entry field */
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, 1, start_y, 1, 0, 0);

            set_field_userptr(pp_field[field_index++], (char *)p_cur_ioa->dev[j].p_resource_entry);

            /* Description field */
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1,
                                              curses_info.max_x - DISK_UD_HDR_SIZE, 
                                              start_y,
                                              DISK_UD_HDR_SIZE, 0, 0);

            set_field_buffer(pp_field[field_index], 0, buffer);

            field_opts_off(pp_field[field_index], /* field to alter */
                           O_ACTIVE);             /* attributes to turn off */ 

            set_field_userptr(pp_field[field_index++], NULL);

            memset(buffer, 0, 100);

            len = 0;
            num_devs++;

        }
    }

    flush_stdscr();

    if (num_devs == 0)
        mvaddstr(1, 2, "(No devices found)");
    else
    {
        field_opts_off(p_fields[1], O_ACTIVE);

        init_index = ((max_size - 1) * 2) + 1;
        for (i = init_index; i < field_index; i += init_index)
            set_new_page(pp_field[i], TRUE);

        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = NULL;

        p_list_form = new_form(pp_field);

        set_form_win(p_list_form, p_field_win);

        post_form(p_list_form);

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        form_driver(p_list_form,
                    REQ_FIRST_FIELD);
        wnoutrefresh(p_field_win);
        doupdate();

        while(1)
        {
            invalid = 0;
            mvwaddstr(p_title_win, 2, 0, "Type option, press Enter.");
            mvwaddstr(p_title_win, 3, 0, "  5=Display hardware resource information details");
            if (platform == IPR_ISERIES)
            {
                mvwaddstr(p_title_win, 5, 0, "        Serial                        Sys    Sys    I/O   I/O");
                mvwaddstr(p_title_win, 6, 0, "OPT     Number   DSA/UA               Bus    Card Adapter Bus   Ctl  Dev");
            }
            else if (platform == IPR_PSERIES)
            {
                mvwaddstr(p_title_win, 5, 0, "        Serial                        PCI    PCI   SCSI  SCSI  SCSI  SCSI");
                mvwaddstr(p_title_win, 6, 0, "OPT     Number   Location             Bus    Dev   Host   Bus   ID   Lun");
            }
            else
            {
                mvwaddstr(p_title_win, 5, 0, "        Serial    PCI    PCI   SCSI  SCSI  SCSI  SCSI");
                mvwaddstr(p_title_win, 6, 0, "OPT     Number    Bus    Dev   Host   Bus   ID   Lun");
            }

            mvwaddstr(p_menu_win, 3, 0, EXIT_KEY_LABEL"=Exit          "CANCEL_KEY_LABEL"=Cancel   f=PageDn   b=PageUp");

            wnoutrefresh(stdscr);
            wnoutrefresh(p_menu_win);
            wnoutrefresh(p_title_win);
            wnoutrefresh(p_field_win);
            doupdate();

            for (;;)
            {
                int ch = getch();

                if (IS_EXIT_KEY(ch))
                {
                    rc = RC_EXIT;
                    goto leave;
                }
                else if (IS_CANCEL_KEY(ch))
                {
                    rc = RC_CANCEL;
                    goto leave;
                }
                else if (IS_ENTER_KEY(ch))
                {
                    dev_num = 0;

                    for (i = 0; (i < field_index) && !invalid; i++)
                    {
                        p_resource = (struct ipr_resource_entry *)field_userptr(pp_field[i]);
                        if (p_resource != NULL)
                        {
                            p_char = field_buffer(pp_field[i], 0);

                            if (strcmp(p_char, "5") == 0)
                            {
                                if (dev_num)
                                {
                                    dev_num++;
                                    break;
                                }
                                else
                                {
                                    found = 0;

                                    for (p_cur_ioa = p_head_ipr_ioa;
                                         p_cur_ioa != NULL;
                                         p_cur_ioa = p_cur_ioa->p_next)
                                    {
                                        if (p_cur_ioa->ioa.p_resource_entry == p_resource)
                                        {
                                            detail_device.p_ipr_dev = &p_cur_ioa->ioa;
                                            found = 1;
                                            break;
                                        }
                                        for (j = 0; j < p_cur_ioa->num_devices; j++)
                                        {
                                            if (p_resource == p_cur_ioa->dev[j].p_resource_entry)
                                            {
                                                detail_device.p_ipr_dev = &p_cur_ioa->dev[j];
                                                found = 1;
                                                break;
                                            }
                                        }
                                        if (found)
                                            break;
                                    }

                                    detail_device.p_ioa = p_cur_ioa;
                                    detail_device.field_index = i;
                                    dev_num++;
                                }
                            }
                            else if (strcmp(p_char, " ") != 0)
                            {
                                mvwaddstr(p_menu_win, 4, 1, "The selection is not valid");
                                invalid = 1;
                            }
                        }
                    }

                    if (invalid)
                        break;
                    if (dev_num == 0)
                        break;
                    else if (dev_num == 1)
                    {
                        unpost_form(p_list_form);
                        unpost_form(p_form);

                        rc = device_details(&detail_device);

                        if ((rc == RC_NOOP) ||
                            (rc == RC_CANCEL))
                        {
                            werase(p_title_win);
                            post_form(p_form);
                            post_form(p_list_form);
                            set_field_buffer(pp_field[detail_device.field_index], 0, " ");
                            set_current_field(p_list_form, pp_field[detail_device.field_index]);
                            wnoutrefresh(stdscr);
                            wnoutrefresh(p_menu_win);
                            wnoutrefresh(p_title_win);
                            wnoutrefresh(p_field_win);
                            doupdate();
                            break;
                        }
                        else
                        {
                            goto leave;
                        }
                    }
                    else
                    {
                        mvwaddstr(p_menu_win, 4, 0, " Error.  More than one unit was selected.");
                        break;
                    }
                }
                else if (ch == KEY_DOWN)
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else if (ch == KEY_UP)
                {
                    form_driver(p_list_form,
                                REQ_PREV_FIELD);
                    break;
                }
                else if (IS_PGDN_KEY(ch))
                {
                    mvwaddstr(p_menu_win, 4, 0, "                                               ");
                    init_page = form_page(p_list_form);
                    form_driver(p_list_form,
                                REQ_NEXT_PAGE);

                    new_page = form_page(p_list_form);

                    if (new_page > init_page)
                    {
                        form_driver(p_list_form,
                                    REQ_FIRST_FIELD);
                    }
                    else
                    {
                        form_driver(p_list_form,
                                    REQ_PREV_PAGE);
                        form_driver(p_list_form,
                                    REQ_LAST_FIELD);
                        mvwaddstr(p_menu_win, 4, 0, " Already at bottom.");
                    }
                    break;
                }
                else if (IS_PGUP_KEY(ch))
                {
                    mvwaddstr(p_menu_win, 4, 0, "                                               ");
                    init_page = form_page(p_list_form);
                    form_driver(p_list_form,
                                REQ_PREV_PAGE);

                    new_page = form_page(p_list_form);

                    if (new_page < init_page)
                    {
                        form_driver(p_list_form,
                                    REQ_FIRST_FIELD);
                    }
                    else
                    {
                        form_driver(p_list_form,
                                    REQ_NEXT_PAGE);
                        form_driver(p_list_form,
                                    REQ_FIRST_FIELD);
                        mvwaddstr(p_menu_win, 4, 0, " Already at top.");
                    }
                    break;
                }
                else if (ch == '\t')
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else
                {
                    /* Terminal is running under 5250 telnet */
                    if (term_is_5250)
                    {
                        /* Send it to the form to be processed and flush this line */
                        form_driver(p_list_form, ch);
                        last_char = flush_line();
                        if (IS_5250_CHAR(last_char))
                            ungetch(ENTER_KEY);
                        else if (last_char != ERR)
                            ungetch(last_char);
                    }
                    else
                    {
                        form_driver(p_list_form, ch);
                    }
                    break;
                }
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;

    free_screen(NULL, NULL, p_fields);
    free_screen(NULL, NULL, pp_field);
    free(pp_field);
    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    flush_stdscr();
    return rc;
}

int device_details(struct device_detail_struct *p_detail_device)
{
    /* iSeries
                   IOA Hardware Resource Information Details


     Type. . . . . . . . . . . . : 2778
     Driver Version. . . . . . . : Ver. 1 Rev. 5 (April 18, 2001)
     Firmware Version. . . . . . : Ver. 1 Rev. 2
     Serial Number . . . . . . . : 00234019
     Part Number . . . . . . . . : 0000021P3735
     Resource Name . . . . . . . : /dev/scsi/host1/controller

     Physical location:
     Frame ID . . . . . . . . . : 3
     Card position. . . . . . . : C15
     Device position. . . . . . :







     Press Enter to continue.

     e=Exit         q=Cancel

     */
    /* pSeries
                   IOA Hardware Resource Information Details


     Type. . . . . . . . . . . . : 2778
     Driver Version. . . . . . . : Ver. 1 Rev. 5 (April 18, 2001)
     Firmware Version. . . . . . : Ver. 1 Rev. 2
     Serial Number . . . . . . . : 00234019
     Part Number . . . . . . . . : 0000021P3735
     Resource Name . . . . . . . : /dev/scsi/host1/controller

     Physical location:
     Location . . . . . . . . . : P1-I1
     PCI Bus. . . . . . . . . . : 1
     PCI Device . . . . . . . . : 4
     SCSI Host. . . . . . . . . : 0






     Press Enter to continue.

     e=Exit         q=Cancel

     */
    /* Generic
                   IOA Hardware Resource Information Details


     Type. . . . . . . . . . . . : 2778
     Driver Version. . . . . . . : Ver. 1 Rev. 5 (April 18, 2001)
     Firmware Version. . . . . . : Ver. 1 Rev. 2
     Serial Number . . . . . . . : 00234019
     Part Number . . . . . . . . : 0000021P3735
     Resource Name . . . . . . . : /dev/scsi/host1/controller

     Physical location:
     PCI Bus. . . . . . . . . . : 1
     PCI Device . . . . . . . . : 4
     SCSI Host. . . . . . . . . : 0






     Press Enter to continue.

     e=Exit         q=Cancel

     */

    /* iSeries
                Disk Unit Hardware Resource Information Details


     Type. . . . . . . . . . . . : 6717
     Firmware Version. . . . . . : 66209
     Model . . . . . . . . . . . : 074
     Level . . . . . . . . . . . : 3
     Serial Number . . . . . . . : 00058B10
     Resource Name . . . . . . . : /dev/scsi/host1/bus1/target0/lun0/disc

     Physical location:
     Frame ID . . . . . . . . . : 3
     Card position. . . . . . . : D10
     Device position. . . . . . :







     Press Enter to continue.

     e=Exit         q=Cancel

     */
    /* pSeries
                Disk Unit Hardware Resource Information Details


     Vendor ID . . . . . . . . . : IBM
     Product ID. . . . . . . . . : ST336607LC
     Firmware Version. . . . . . : 66209
     Model . . . . . . . . . . . : 074
     Level . . . . . . . . . . . : 0
     Serial Number . . . . . . . : 00058B10
     Resource Name . . . . . . . : /dev/scsi/host1/bus1/target0/lun0/disc

     Physical location:
     Location . . . . . . . . . : P1-I1/Z1-A4
     PCI Bus. . . . . . . . . . : 1
     PCI Device . . . . . . . . : 4
     SCSI Host. . . . . . . . . : 0
     SCSI Bus . . . . . . . . . : 1
     SCSI Target. . . . . . . . : 4
     SCSI Lun . . . . . . . . . : 0


     Press Enter to continue.

     e=Exit         q=Cancel

     */
    /* Generic
                Disk Unit Hardware Resource Information Details


     Vendor ID . . . . . . . . . : IBM
     Product ID. . . . . . . . . : ST336607LC
     Firmware Version. . . . . . : 66209
     Model . . . . . . . . . . . : 074
     Level . . . . . . . . . . . : 0
     Serial Number . . . . . . . : 00058B10
     Resource Name . . . . . . . : /dev/scsi/host1/bus1/target0/lun0/disc

     Physical location:
     PCI Bus. . . . . . . . . . : 1
     PCI Device . . . . . . . . : 4
     SCSI Host. . . . . . . . . : 0
     SCSI Bus . . . . . . . . . : 1
     SCSI Target. . . . . . . . : 4
     SCSI Lun . . . . . . . . . : 0



     Press Enter to continue.

     e=Exit         q=Cancel

     */

    FIELD *p_fields[3];
    FORM *p_form;
    FORM *p_old_form;
    int rc;
    char dev_name[50];
    struct ipr_resource_entry *p_resource =
        p_detail_device->p_ipr_dev->p_resource_entry;
    struct ipr_array2_record *ipr_array2_record =
        (struct ipr_array2_record *)p_detail_device->p_ipr_dev->p_qac_entry;
    struct ipr_ioa *p_ioa = p_detail_device->p_ioa;
    struct ipr_supported_arrays *p_supported_arrays;
    struct ipr_array_cap_entry *p_cur_array_cap_entry;
    int fd, i, field_index;
    unsigned int ucode_rev = 0;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_dasd_inquiry_page3 dasd_page3_inq;
    u8 product_id[IPR_PROD_ID_LEN+1];
    u8 vendor_id[IPR_VENDOR_ID_LEN+1];
    u8 cdb[IPR_CCB_CDB_LEN];
    struct ipr_read_cap read_cap;
    struct ipr_read_cap16 read_cap16;
    struct sense_data_t sense_data;
    long double device_capacity;
    unsigned long long max_user_lba_int;
    char *p_dev_name;
    double lba_divisor;
    char pci_bus_number[10], pci_device[10], scsi_channel[3], scsi_id[3], scsi_lun[4];
    char dsa_str[20], ua_str[20];
    bool removeable = false;
    char buf[100];

    dsa_str[0] = ua_str[0] = '\0';

    if (p_resource->dsa)
        sprintf(dsa_str, "%X", p_resource->dsa);
    if (p_resource->unit_address)
        sprintf(ua_str, "%X", p_resource->unit_address);
    sprintf(pci_bus_number, "%d", p_resource->pci_bus_number);
    sprintf(pci_device, "%d", p_resource->pci_slot);
    sprintf(scsi_channel, "%d", p_resource->resource_address.bus);
    sprintf(scsi_id, "%d", p_resource->resource_address.target);
    sprintf(scsi_lun, "%d", p_resource->resource_address.lun);

    read_cap.max_user_lba = 0;
    read_cap.block_length = 0;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

refresh:
    if (p_resource->is_ioa_resource)
    {
        set_field_buffer(p_fields[0], 0, "IOA Hardware Resource Information Details");

        post_form(p_form);

        field_index = 3;

        if (platform == IPR_ISERIES)
        {
            mvprintw(field_index++, 1, "Type. . . . . . . . . . . . : %X", p_ioa->ccin);
        }
        else
        {
            memcpy(vendor_id, p_resource->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
            vendor_id[IPR_VENDOR_ID_LEN] = '\0';
            memcpy(product_id, p_resource->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);
            product_id[IPR_PROD_ID_LEN] = '\0';

            mvprintw(field_index++, 1, "Manufacturer. . . . . . . . : %s", vendor_id);
            mvprintw(field_index++, 1, "Type. . . . . . . . . . . . : %X", p_ioa->ccin);
        }

        mvprintw(field_index++, 1, "Driver Version. . . . . . . : %s", p_ioa->driver_version);
        mvprintw(field_index++, 1, "Firmware Version. . . . . . : %s", p_ioa->firmware_version);
        mvprintw(field_index++, 1, "Serial Number . . . . . . . : %-8s", p_resource->serial_num);
        mvprintw(field_index++, 1, "Part Number . . . . . . . . : %s", p_ioa->part_num);
        mvprintw(field_index++, 1, "Resource Name . . . . . . . : %s", p_ioa->ioa.dev_name);
        field_index++;
        mvprintw(field_index++, 1, "Physical location:");

        if (platform == IPR_ISERIES)
        {
            mvprintw(field_index++, 2, "Frame ID . . . . . . . . . : %s", p_resource->frame_id);
            mvprintw(field_index++, 2, "Card position. . . . . . . : %s", p_resource->slot_label);
            mvprintw(field_index++, 2, "Device position. . . . . . : ");
        }
        else
        {
            if (platform == IPR_PSERIES)
                mvprintw(field_index++, 2,
                         "Location . . . . . . . . . : %s", p_resource->pseries_location);

            mvprintw(field_index++, 2, "PCI Bus. . . . . . . . . . : %d", p_resource->pci_bus_number);
            mvprintw(field_index++, 2, "PCI Device . . . . . . . . : %d", p_resource->pci_slot);
            mvprintw(field_index++, 2, "SCSI Host Number . . . . . : %d", p_resource->host_no);
        }

        mvprintw(curses_info.max_y - 4, 1, "Press Enter to continue.");
        mvprintw(curses_info.max_y - 2, 1, EXIT_KEY_LABEL"=Exit         "CANCEL_KEY_LABEL"=Cancel");
    }
    else if ((ipr_array2_record) &&
             (ipr_array2_record->common.record_id == IPR_RECORD_ID_ARRAY2_RECORD))
    {
        set_field_buffer(p_fields[0], 0, "Disk Array Information Details");

        post_form(p_form);

        field_index = 3;

        memcpy(vendor_id, p_resource->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
        vendor_id[IPR_VENDOR_ID_LEN] = '\0';
        mvprintw(field_index++, 1, "Manufacturer. . . . . . . . : %s", vendor_id);
        memcpy(product_id, p_resource->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);
        product_id[IPR_PROD_ID_LEN] = '\0';
        mvprintw(field_index++, 1, "Machine Type and Model. . . : %s", product_id);
        mvprintw(field_index++, 1, "Serial Number . . . . . . . : %-8s",
                 p_resource->serial_num);

        p_supported_arrays = p_ioa->p_supported_arrays;
        p_cur_array_cap_entry = (struct ipr_array_cap_entry *)p_supported_arrays->data;
        for (i=0; i<iprtoh16(p_supported_arrays->num_entries); i++)
        {
            if (p_cur_array_cap_entry->prot_level == ipr_array2_record->raid_level)
            {
                mvprintw(field_index++, 1, "RAID Level  . . . . . . . . : RAID %s", p_cur_array_cap_entry->prot_level_str);
                break;
            }

            p_cur_array_cap_entry = (struct ipr_array_cap_entry *)
                ((void *)p_cur_array_cap_entry + iprtoh16(p_supported_arrays->entry_length));
        }

        if (iprtoh16(ipr_array2_record->stripe_size) > 1024)
        {
            mvprintw(field_index++, 1, "Stripe Size . . . . . . . . : %d M",
                     iprtoh16(ipr_array2_record->stripe_size)/1024);
        }
        else
        {
            mvprintw(field_index++, 1, "Stripe Size . . . . . . . . : %d k",
                     iprtoh16(ipr_array2_record->stripe_size));
        }

        fd = open(p_ioa->ioa.dev_name, O_RDWR);

        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_ioa->ioa.dev_name);
        }
        else
        {
            /* Do a read capacity to determine the capacity */
            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

            ioa_cmd.buffer = &read_cap16;
            ioa_cmd.buffer_len = sizeof(read_cap16);
            ioa_cmd.read_not_write = 1;
            ioa_cmd.device_cmd = 1;
            ioa_cmd.cdb[1] = IPR_READ_CAPACITY_16;
            ioa_cmd.resource_address = p_resource->resource_address;

            memset(&read_cap16, 0, sizeof(read_cap16));

            rc = ipr_ioctl(fd, IPR_SERVICE_ACTION_IN, &ioa_cmd);
        }

        if (rc != 0)
            syslog(LOG_ERR, "Read capacity to %s failed. %m"IPR_EOL, dev_name);
        else if ((iprtoh32(read_cap16.max_user_lba_hi) ||
                  iprtoh32(read_cap16.max_user_lba_lo)) &&
                 iprtoh32(read_cap16.block_length))
        {
            max_user_lba_int = iprtoh32(read_cap16.max_user_lba_hi);
            max_user_lba_int <<= 32;
            max_user_lba_int |= iprtoh32(read_cap16.max_user_lba_lo);

            device_capacity = max_user_lba_int + 1;

            lba_divisor  = (1000*1000*1000)/iprtoh32(read_cap16.block_length);

            mvprintw(field_index++, 1, "Capacity. . . . . . . . . . : %.2Lf GB",
                     device_capacity/lba_divisor);
        }

        mvprintw(field_index++, 1, "Resource Name . . . . . . . : %s",
                 p_detail_device->p_ipr_dev->dev_name);

        mvprintw(curses_info.max_y - 4, 1, "Press Enter to continue.");
        mvprintw(curses_info.max_y - 2, 1, EXIT_KEY_LABEL"=Exit         "CANCEL_KEY_LABEL"=Cancel");
    }
    else
    {
        set_field_buffer(p_fields[0], 0, "Disk Unit Hardware Resource Information Details");

        post_form(p_form);

        if (p_resource->subtype == IPR_SUBTYPE_GENERIC_SCSI)
            p_dev_name = p_detail_device->p_ipr_dev->gen_name;
        else
            p_dev_name = p_ioa->ioa.dev_name;

        fd = open(p_dev_name, O_RDWR);

        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_dev_name);
        }
        else
        {
            if ((platform == IPR_ISERIES) || (platform == IPR_PSERIES))
            {
                if (p_detail_device->p_ipr_dev->p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI)
                {
                    memset(&dasd_page3_inq, 0, sizeof(dasd_page3_inq));
                    memset(cdb, 0, IPR_CCB_CDB_LEN);

                    /* Issue page 3 inquiry */
                    cdb[0] = INQUIRY;
                    cdb[1] = 0x01; /* EVPD */
                    cdb[2] = 0x03; /* Page 3 */
                    cdb[4] = sizeof(dasd_page3_inq);

                    rc = sg_ioctl(fd, cdb, &dasd_page3_inq,
                                  sizeof(dasd_page3_inq),
                                  SG_DXFER_FROM_DEV, &sense_data, 30);
                }
                else
                {
                    /* Do a page 3 inquiry to determine the firmware level */
                    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                    ioa_cmd.buffer = &dasd_page3_inq;
                    ioa_cmd.buffer_len = sizeof(dasd_page3_inq);
                    ioa_cmd.cdb[1] = 0x01;
                    ioa_cmd.cdb[2] = 0x03;
                    ioa_cmd.read_not_write = 1;
                    ioa_cmd.device_cmd = 1;
                    ioa_cmd.resource_address = p_resource->resource_address;

                    memset(&dasd_page3_inq, 0, sizeof(dasd_page3_inq));

                    rc = ipr_ioctl(fd, INQUIRY, &ioa_cmd);
                }

                if (rc != 0)
                    syslog(LOG_ERR, "Inquiry to %s failed. %m"IPR_EOL, p_dev_name);
                else
                    ucode_rev = (dasd_page3_inq.release_level[0] << 24) |
                        (dasd_page3_inq.release_level[1] << 16) |
                        (dasd_page3_inq.release_level[2] << 8) |
                        dasd_page3_inq.release_level[3];
            }
            else
            {
                char buf[5];
                memcpy(buf, p_detail_device->p_ipr_dev->p_resource_entry->std_inq_data.ros_rsvd_ram_rsvd, 4);
                buf[4] = '\0';
                
                ucode_rev = strtoul(buf, NULL, 16);
            }

            if (p_detail_device->p_ipr_dev->p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI)
            {
                memset(&read_cap, 0, sizeof(read_cap));
                memset(cdb, 0, IPR_CCB_CDB_LEN);

                cdb[0] = READ_CAPACITY;

                rc = sg_ioctl(fd, cdb, &read_cap,
                              sizeof(read_cap), SG_DXFER_FROM_DEV,
                              &sense_data, 30);
            }
            else
            {
                /* Do a read capacity to determine the capacity */
                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));
                memset(&read_cap, 0, sizeof(read_cap));

                ioa_cmd.buffer = &read_cap;
                ioa_cmd.buffer_len = sizeof(read_cap);
                ioa_cmd.read_not_write = 1;
                ioa_cmd.device_cmd = 1;
                ioa_cmd.resource_address = p_resource->resource_address;

                rc = ipr_ioctl(fd, READ_CAPACITY, &ioa_cmd);
            }

            if (rc != 0)
                syslog(LOG_ERR, "Read capacity to %s failed. %m"IPR_EOL, dev_name);

            close(fd);

            if (!is_valid_device(p_resource->frame_id, p_resource->slot_label, dsa_str, ua_str,
                                 p_resource->pseries_location,
                                 pci_bus_number, pci_device, scsi_channel, scsi_id, scsi_lun))
            {
                removeable = true;
            }
        }

        field_index = 3;

        if ((platform == IPR_ISERIES) || (p_resource->type != 0x6600))
        {
            mvprintw(field_index++, 1, "Type. . . . . . . . . . . . : %X",
                     p_resource->type);
        }

        if (platform != IPR_ISERIES)
        {
            memcpy(vendor_id, p_resource->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
            vendor_id[IPR_VENDOR_ID_LEN] = '\0';
            memcpy(product_id, p_resource->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);
            product_id[IPR_PROD_ID_LEN] = '\0';

            mvprintw(field_index++, 1, "Manufacturer. . . . . . . . : %s", vendor_id);
            mvprintw(field_index++, 1, "Machine Type and Model. . . : %s", product_id);
        }

        mvprintw(field_index++, 1, "Firmware Version. . . . . . : %X",
                 ucode_rev);

        mvprintw(field_index++, 1, "Model . . . . . . . . . . . : %03d",
                 p_resource->model);

        if (p_resource->type != 0x6600)
        {
            mvprintw(field_index++, 1, "Level . . . . . . . . . . . : %X",
                     p_resource->level);
        }

        mvprintw(field_index++, 1, "Serial Number . . . . . . . : %-8s",
                 p_resource->serial_num);

        if (iprtoh32(read_cap.block_length) && iprtoh32(read_cap.max_user_lba))
        {
            lba_divisor = (1000*1000*1000)/iprtoh32(read_cap.block_length);

            device_capacity = iprtoh32(read_cap.max_user_lba) + 1;

            mvprintw(field_index++, 1, "Capacity. . . . . . . . . . : %.2Lf GB",
                     device_capacity/lba_divisor);
        }

        if (strlen(p_detail_device->p_ipr_dev->dev_name) > 0)
        {
            mvprintw(field_index++, 1, "Resource Name . . . . . . . : %s",
                     p_detail_device->p_ipr_dev->dev_name);
        }

        field_index++;

        mvprintw(field_index++, 1, "Physical location:");

        if (platform == IPR_ISERIES)
        {
            mvprintw(field_index++, 2, "Frame ID . . . . . . . . . : %s",
                     p_resource->frame_id);
            mvprintw(field_index++, 2, "Card position. . . . . . . : %s",
                     p_resource->slot_label);
            mvprintw(field_index++, 2, "Device position. . . . . . : ");
        }
        else
        {
            if (platform == IPR_PSERIES)
            {
                mvprintw(field_index++, 2, "Location . . . . . . . : %s",
                         p_resource->pseries_location);
            }

            mvprintw(field_index++, 2, "PCI Bus. . . . . . . . . . : %d",
                     p_resource->pci_bus_number);
            mvprintw(field_index++, 2, "PCI Device . . . . . . . . : %d",
                     p_resource->pci_slot);
            mvprintw(field_index++, 2, "SCSI Host Number . . . . . : %d",
                     p_resource->host_no);
            mvprintw(field_index++, 2, "SCSI Channel. .  . . . . . : %d",
                     p_resource->resource_address.bus);
            mvprintw(field_index++, 2, "SCSI Id. . . . . . . . . . : %d",
                     p_resource->resource_address.target);
            mvprintw(field_index++, 2, "SCSI Lun. . . .  . . . . . : %d",
                     p_resource->resource_address.lun);
        }

        mvprintw(curses_info.max_y - 4, 1, "Press Enter to continue.");

        strcpy(buf, EXIT_KEY_LABEL"=Exit         "CANCEL_KEY_LABEL"=Cancel");

        if (removeable)
            strcat(buf, "         r=Remove");

        if (platform == IPR_PSERIES)
            strcat(buf, "         v=Display Extended VPD");

        mvprintw(curses_info.max_y - 2, 1, buf);
    }

    while(1)
    {
        form_driver(p_form,
                    REQ_LAST_FIELD);
        refresh();

        while(1)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if (((ch == 'r') || (ch == 'R')) && removeable)
            {

                rc = device_concurrent_maintenance(p_resource->frame_id, p_resource->slot_label,
                                                   dsa_str, ua_str,
                                                   p_resource->pseries_location,
                                                   pci_bus_number, pci_device, scsi_channel,
                                                   scsi_id, scsi_lun, "1");

                goto leave;
            }
            else if ((platform == IPR_PSERIES) && ((ch == 'v') || (ch == 'V')))
            {
                rc = device_extended_vpd(p_detail_device);
                if (rc == RC_EXIT)
                    goto leave;
                else
                    goto refresh;
            }
        }
    }
    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(NULL, NULL, p_fields);

    flush_stdscr();
    return rc;
}

int device_extended_vpd(struct device_detail_struct *p_detail_device)
{

    FIELD *p_fields[3];
    FORM *p_form;
    FORM *p_old_form;
    int rc;
    struct ipr_resource_entry *p_resource =
        p_detail_device->p_ipr_dev->p_resource_entry;
    int fd, field_index;
    struct ipr_std_inq_data_long std_inq_data;
    struct sense_data_t sense_data;
    u8 product_id[IPR_PROD_ID_LEN+1];
    u8 vendor_id[IPR_VENDOR_ID_LEN+1];
    u8 cdb[IPR_CCB_CDB_LEN];
    u8 *p_char;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0], 0, "Disk Unit Hardware Resource Extended Details");

    post_form(p_form);

    if (p_detail_device->p_ipr_dev->p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI)
    {
        fd = open(p_detail_device->p_ipr_dev->gen_name, O_RDWR);

        if (fd != -1)
        {
            memset(&std_inq_data, 0, sizeof(std_inq_data));
            memset(cdb, 0, IPR_CCB_CDB_LEN);

            /* Issue mode sense to get the block size */
            cdb[0] = INQUIRY;
            cdb[4] = sizeof(std_inq_data);

            rc = sg_ioctl(fd, cdb,
                          &std_inq_data, sizeof(std_inq_data),
                          SG_DXFER_FROM_DEV,
                          &sense_data, 30);

            close(fd);

            if (rc == 0)
            {
                /* Fill in additional VPD information */
                ipr_memory_copy(p_resource->part_number, std_inq_data.part_number,
                                   IPR_STD_INQ_PART_NUM_LEN);
                p_resource->part_number[IPR_STD_INQ_PART_NUM_LEN] = '\0';

                ipr_memory_copy(p_resource->ec_level, std_inq_data.ec_level,
                                   IPR_STD_INQ_EC_LEVEL_LEN);
                p_resource->ec_level[IPR_STD_INQ_EC_LEVEL_LEN] = '\0';

                ipr_memory_copy(p_resource->fru_number, std_inq_data.fru_number,
                                   IPR_STD_INQ_FRU_NUM_LEN);
                p_resource->fru_number[IPR_STD_INQ_FRU_NUM_LEN] = '\0';

                ipr_memory_copy(p_resource->z1_term, std_inq_data.z1_term,
                                   IPR_STD_INQ_Z1_TERM_LEN);
                p_resource->z1_term[IPR_STD_INQ_Z1_TERM_LEN] = '\0';

                ipr_memory_copy(p_resource->z2_term, std_inq_data.z2_term,
                                   IPR_STD_INQ_Z2_TERM_LEN);
                p_resource->z2_term[IPR_STD_INQ_Z2_TERM_LEN] = '\0';

                ipr_memory_copy(p_resource->z3_term, std_inq_data.z3_term,
                                   IPR_STD_INQ_Z3_TERM_LEN);
                p_resource->z3_term[IPR_STD_INQ_Z3_TERM_LEN] = '\0';

                ipr_memory_copy(p_resource->z4_term, std_inq_data.z4_term,
                                   IPR_STD_INQ_Z4_TERM_LEN);
                p_resource->z4_term[IPR_STD_INQ_Z4_TERM_LEN] = '\0';

                ipr_memory_copy(p_resource->z5_term, std_inq_data.z5_term,
                                   IPR_STD_INQ_Z5_TERM_LEN);
                p_resource->z5_term[IPR_STD_INQ_Z5_TERM_LEN] = '\0';

                ipr_memory_copy(p_resource->z6_term, std_inq_data.z6_term,
                                   IPR_STD_INQ_Z6_TERM_LEN);
                p_resource->z6_term[IPR_STD_INQ_Z6_TERM_LEN] = '\0';
            }
            else
            {
                syslog(LOG_ERR, "Inquiry to %s failed. %m"IPR_EOL, p_detail_device->p_ipr_dev->gen_name);
            }
        }
        else
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_detail_device->p_ipr_dev->gen_name);
        }
    }

    field_index = 3;

    memcpy(vendor_id, p_resource->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
    vendor_id[IPR_VENDOR_ID_LEN] = '\0';
    memcpy(product_id, p_resource->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);
    product_id[IPR_PROD_ID_LEN] = '\0';

    if (strlen(p_detail_device->p_ipr_dev->dev_name) > 0)
    {
        mvprintw(field_index++, 1, "Resource Name . . . . . . . : %s",
                 p_detail_device->p_ipr_dev->dev_name);
    }
    mvprintw(field_index++, 1, "Manufacturer. . . . . . . . : %s", vendor_id);
    mvprintw(field_index++, 1, "Machine Type and Model. . . : %s", product_id);
    mvprintw(field_index++, 1, "FRU Number. . . . . . . . . : %s",
             p_resource->fru_number);
    mvprintw(field_index++, 1, "Serial Number . . . . . . . : %-8s",
             p_resource->serial_num);
    mvprintw(field_index++, 1, "EC Level . .  . . . . . . . : %s",
             p_resource->ec_level);

    mvprintw(field_index++, 1, "Part Number.  . . . . . . . : %s",
             p_resource->part_number);

    p_char = (u8 *)&p_resource->std_inq_data;

    mvprintw(field_index++, 1, "Device Specific (Z0). . . . : %02X%02X%02X%02X%02X%02X%02X%02X",
             p_char[0], p_char[1], p_char[2],
             p_char[3], p_char[4], p_char[5],
             p_char[6], p_char[7]);

    mvprintw(field_index++, 1, "Device Specific (Z1). . . . : %s",
             p_resource->z1_term);

    mvprintw(field_index++, 1, "Device Specific (Z2). . . . : %s",
             p_resource->z2_term);

    mvprintw(field_index++, 1, "Device Specific (Z3). . . . : %s",
             p_resource->z3_term);

    mvprintw(field_index++, 1, "Device Specific (Z4). . . . : %s",
             p_resource->z4_term);

    mvprintw(field_index++, 1, "Device Specific (Z5). . . . : %s",
             p_resource->z5_term);

    mvprintw(field_index++, 1, "Device Specific (Z6). . . . : %s",
             p_resource->z6_term);

    mvprintw(curses_info.max_y - 4, 1, "Press Enter to continue.");

    mvprintw(curses_info.max_y - 2, 1,
             EXIT_KEY_LABEL"=Exit         "CANCEL_KEY_LABEL"=Cancel");

    while(1)
    {
        form_driver(p_form,
                    REQ_LAST_FIELD);
        refresh();

        while(1)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
        }
    }
    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(NULL, NULL, p_fields);

    flush_stdscr();
    return rc;
}

void raid_screen()
{
    /*
                               Work with Device Parity Protection

        Select one of the following:

             1. Display device parity status
             2. Start device parity protection
             3. Stop device parity protection
             4. Include unit in device parity protection
             5. Work with disk unit configuration



        Selection:

        e=Exit  q=Cancel
     */
    FIELD *p_fields[12];
    FORM *p_form, *p_old_form;
    int invalid = 0;
    int rc = RC_SUCCESS;
    int j;

    int raid_status();
    int raid_stop();
    int raid_start();
    int raid_include();
    int configure_device();

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 2, 
                            12, 12,
                            0,
                            0);

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Work with Device Parity Protection");          /* string value to set */

    flush_stdscr();

    clear();

    post_form(p_form);

    while(1)
    {
        mvaddstr(2, 0, "Select one of the following:");
        mvaddstr(4, 0, "     1. Display device parity status");
        mvaddstr(5, 0, "     2. Start device parity protection");
        mvaddstr(6, 0, "     3. Stop device parity protection");
        mvaddstr(7, 0, "     4. Include unit in device parity protection");
        mvaddstr(8, 0, "     5. Work with disk unit configuration");
        mvaddstr(12, 0, "Selection: ");
        mvaddstr(curses_info.max_y - 2, 0, EXIT_KEY_LABEL"=Exit  "CANCEL_KEY_LABEL"=Cancel");

        if (invalid)
        {
            invalid = 0;
            mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
        }

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        refresh();
        for (;;)
        {
            int ch = getch();

            if (IS_ENTER_KEY(ch))
            {
                char *p_char;
                refresh();
                unpost_form(p_form);
                p_char = field_buffer(p_fields[1], 0);

                switch(p_char[0])
                {
                    case '1':  /* Display device parity status */
                        while (RC_REFRESH == (rc = raid_status()))
                            clear();
                        clear();
                        if (rc == RC_EXIT)
                            goto leave;
                        post_form(p_form);
                        break;
                    case '2':  /* Start device parity protection */
                        while (RC_REFRESH == (rc = raid_start()))
                            clear();
                        if (RC_EXIT == rc)
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0,
                                     "Start Parity Protection completed successfully");
                        }
                        else if (rc == RC_FAILED)
                        {
                            mvprintw(curses_info.max_y - 1, 0, "Start Parity Protection failed.");
                        }
                        else if (rc == RC_EXIT)
                            goto leave;
                        break;
                    case '3':  /* Stop device parity protection */
                        while (RC_REFRESH == (rc = raid_stop()))
                            clear();
                        clear();
                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0,
                                     "Stop Parity Protection completed successfully");
                        }
                        else if (rc == RC_FAILED)
                        {
                            mvprintw(curses_info.max_y - 1, 0, "Stop Parity Protection failed.");
                        }
                        else if (rc == RC_EXIT)
                            goto leave;
                        break;
                    case '4':  /* Include unit in device parity protection */
                        while (RC_REFRESH == (rc = raid_include()));
                        if (rc == RC_EXIT)
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0, "Include Unit completed successfully");
                        }
                        else if (rc == RC_FAILED)
                            mvaddstr(curses_info.max_y - 1, 0, "Include failed");
                        break;
                    case '5':  /* Work with disk unit configuration */
                        while (RC_REFRESH == (rc = configure_device()));
                        if (rc == RC_EXIT)
                            goto leave;
                        clear();
                        post_form(p_form);
                        break;
                    case ' ':
                        clear();
                        post_form(p_form);
                        break;
                    default:
                        clear();
                        post_form(p_form);
                        mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
                        break;
                }
                form_driver(p_form, ' ');
                break;
            }
            else if (IS_EXIT_KEY(ch))
                goto leave;
            else if (IS_CANCEL_KEY(ch))
                goto leave;
            else
            {
                form_driver(p_form, ch);
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                refresh();
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    for (j=0; j < 2; j++)
        free_field(p_fields[j]);
    flush_stdscr();
    return;
}

int configure_device()
{
    /*
                               Work with Disk Unit Configuration

        Select one of the following:

             1. Format device for advanced function
             2. Format device for JBOD function
             3. Configure a hot spare device
             4. Unconfigure a hot spare device




        Selection:

        e=Exit  q=Cancel
     */
    FIELD *p_fields[12];
    FORM *p_form, *p_old_form;
    int invalid = 0;
    int rc = RC_SUCCESS;
    int j;
    char title_str[50];
    int configure_af_device(int action_code);
    int hot_spare(int action);
#define IPR_INCLUDE 0
#define IPR_REMOVE  1
#define IPR_ADD_HOT_SPARE 0
#define IPR_RMV_HOT_SPARE 1

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 2,   /* new field size */ 
                            12, 12,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Work with Disk Unit Configuration");          /* string value to set */

    flush_stdscr();

    clear();

    post_form(p_form);

    while(1)
    {
        mvaddstr(2, 0, "Select one of the following:");
        mvaddstr(4, 0, "     1. Format device for advanced function");
        mvaddstr(5, 0, "     2. Format device for JBOD function");
        mvaddstr(6, 0, "     3. Configure a hot spare device");
        mvaddstr(7, 0, "     4. Unconfigure a hot spare device");
        mvaddstr(12, 0, "Selection: ");
        mvaddstr(curses_info.max_y - 2, 0, EXIT_KEY_LABEL"=Exit  "CANCEL_KEY_LABEL"=Cancel");

        if (invalid)
        {
            invalid = 0;
            mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
        }

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        refresh();
        for (;;)
        {
            int ch = getch();

            if (IS_ENTER_KEY(ch))
            {
                char *p_char;
                refresh();
                unpost_form(p_form);
                p_char = field_buffer(p_fields[1], 0);

                switch(p_char[0])
                {
                    case '1':  /* Display candidates for AF inclusion */
                        rc = configure_af_device(IPR_INCLUDE);
                        if (RC_EXIT == rc)
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0, "Completed successfully");
                        }
                        else if (rc == RC_FAILED)
                        {
                            mvprintw(curses_info.max_y - 1, 0, "Failed.");
                        }
                        else if (rc == RC_NOOP)
                        {
                            sprintf(title_str,"Format device for advanced function failed");
                        }
                        break;
                    case '2':  /* Display candidates for AF removal */
                        rc = configure_af_device(IPR_REMOVE);
                        if (RC_EXIT == rc)
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0, "Completed successfully");
                        }
                        else if (rc == RC_FAILED)
                        {
                            mvprintw(curses_info.max_y - 1, 0, "Failed.");
                        }
                        else if (rc == RC_NOOP)
                        {
                            sprintf(title_str,"Disable Device Advanced Functions Failed");
                        }
                        break;
                    case '3':  /* Display candidates for AF removal */
                        rc = hot_spare(IPR_ADD_HOT_SPARE);
                        if (RC_EXIT == rc)
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0, "Completed successfully");
                        }
                        else if (rc == RC_FAILED)
                        {
                            mvprintw(curses_info.max_y - 1, 0, "Failed.");
                        }
                        else if (rc == RC_NOOP)
                        {
                            sprintf(title_str,"Enable Device as Hot Spare Failed");
                        }
                        break;
                    case '4':  /* Display candidates for AF removal */
                        rc = hot_spare(IPR_RMV_HOT_SPARE);
                        if (RC_EXIT == rc)
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0, "Completed successfully");
                        }
                        else if (rc == RC_FAILED)
                        {
                            mvprintw(curses_info.max_y - 1, 0, "Failed.");
                        }
                        else if (rc == RC_NOOP)
                        {
                            sprintf(title_str,"Disable Device as Hot Spare Failed");
                        }
                        break;
                    case ' ':
                        clear();
                        post_form(p_form);
                        break;
                    default:
                        clear();
                        post_form(p_form);
                        mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
                        break;
                }

                if (rc == RC_NOOP)
                {
                    clear();
                    set_field_buffer(p_fields[0],                       /* field to alter */
                                     0,                                 /* number of buffer to alter */
                                     title_str);                        /* string value to set */

                    mvaddstr( 2, 0, "There are no disk units eligible for the selected operation");
                    mvaddstr( 3, 0, "due to one or more of the following reasons:");
                    mvaddstr( 5, 0, "o  There are no eligible device units in the system.");
                    mvaddstr( 6, 0, "o  An IOA is in a condition that makes the disk units attached to");
                    mvaddstr( 7, 0, "   it read/write protected.  Examine the kernel messages log");
                    mvaddstr( 8, 0, "   for any errors that are logged for the IO subsystem");
                    mvaddstr( 9, 0, "   and follow the appropriate procedure for the reference code to");
                    mvaddstr(10, 0, "   correct the problem, if necessary.");
                    mvaddstr(11, 0, "o  Not all disk units attached to an advanced function IOA have");
                    mvaddstr(12, 0, "   reported to the system.  Retry the operation.");
                    mvaddstr(13, 0, "o  The disk units are missing.");
                    mvaddstr(14, 0, "o  The disk unit/s are currently in use.");
                    mvaddstr( curses_info.max_y - 2, 0, "Press Enter to continue.");
                    mvaddstr( curses_info.max_y - 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

                    form_driver(p_form,               /* form to pass input to */
                                REQ_LAST_FIELD);     /* form request code */

                    wnoutrefresh(stdscr);
                    doupdate();

                    for (;;)
                    {
                        int ch = getch();

                        if (IS_EXIT_KEY(ch))
                        {
                            rc = RC_EXIT;
                            goto leave;
                        }
                        else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
                        {
                            break;
                        }
                    }
                    clear();
                }

                form_driver(p_form, ' ');
                break;
            }
            else if (IS_EXIT_KEY(ch))
                goto leave;
            else if (IS_CANCEL_KEY(ch))
                goto leave;
            else
            {
                form_driver(p_form, ch);
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                refresh();
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    for (j=0; j < 2; j++)
        free_field(p_fields[j]);
    flush_stdscr();
    return 0;
}

int raid_status()
{
#define RAID_STATUS_HDR_SIZE			4
#define RAID_STATUS_MENU_SIZE			5
/* iSeries
                  Display Device Parity Status

Parity  Serial                Resource
  Set   Number   Type  Model  Name                                    Status
   1   02127005  2782   001   /dev/ipr4
       002443C6  6713   072   /dev/sdr                                Active
       0024B45C  6713   072   /dev/sds                                Active
       00000DC7  6717   072   /dev/sdt                                Active
       000BBBC1  6717   072   /dev/sdu                                Active
       0002F5AA  6717   070   /dev/sdv                                Active
   2   00103039  2748   001   /dev/ipr0
       0004C93D  6607   072   /dev/sda                                Active
       0006E12B  6607   072   /dev/sdb                                Active
       000716B1  6607   070   /dev/sdc                                Active
       0006CBF1  6607   072   /dev/sdd                                Active
       0005722D  6607   072   /dev/sde                                Active
       0006E103  6607   070   /dev/sdf                                Active

e=Exit   q=Cancel   r=Refresh    f=PageDn   b=PageUp
*/

/* Generic
                  Display Device Parity Status

Parity  Serial          Resource       PCI    PCI   SCSI  SCSI  SCSI  Status
  Set   Number   Model  Name           Bus    Dev    Bus    ID   Lun
   1   02127005   200   /dev/sda        05     03    255     0    0   Active
       002443C6   072                   05     03      0     1    0   Active
       0024B45C   072                   05     03      0     5    0   Active
       00000DC7   072                   05     03      0     9    0   Active
       000BBBC1   072                   05     03      0     6    0   Active
       0002F5AA   070                   05     03      0     4    0   Active
   2   00103039   200   /dev/sdb        05     03    255     0    1   Active
       0004C93D   072                   05     03      1     5    0   Active
       0006E12B   072                   05     03      1     6    0   Active
       000716B1   070                   05     03      1     9    0   Active
       0006CBF1   072                   05     03      1     2    0   Active
       0005722D   072                   05     03      1     3    0   Active
       0006E103   070                   05     03      1     4    0   Active

e=Exit   q=Cancel   r=Refresh    f=PageDn   b=PageUp
*/

    int fd, rc, array_num, i, j;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_cmd_status cmd_status;
    struct ipr_array_query_data *array_query_data;
    struct ipr_record_common *p_common_record;
    struct ipr_array_record *p_array_record;
    struct ipr_array2_record *p_array2_record;
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    int len = 0;
    u8 array_id;
    int max_size = curses_info.max_y - (RAID_STATUS_HDR_SIZE + RAID_STATUS_MENU_SIZE + 1);
    char buffer[100];
    struct ipr_query_res_state res_state;
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    PANEL *p_panel;
    struct ipr_resource_entry *p_resource_entry;
    int is_first;

    rc = RC_SUCCESS;

    p_title_win = newwin(RAID_STATUS_HDR_SIZE, curses_info.max_x,
                         curses_info.start_y, curses_info.start_x);

    p_menu_win = newwin(RAID_STATUS_MENU_SIZE,
                        curses_info.max_x,
                        curses_info.start_y + max_size + RAID_STATUS_HDR_SIZE + 1,
                        curses_info.start_x);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */


    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Display Device Parity Status");          /* string value to set */

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + RAID_STATUS_HDR_SIZE,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    flush_stdscr();

    clear();

    post_form(p_form);

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 2, 0,
                  "Parity  Serial                Resource");
        mvwaddstr(p_title_win, 3, 0,
                  "  Set   Number   Type  Model  Name                                  Status");
    }
    else
    {
        mvwaddstr(p_title_win, 2, 0,
                  "Parity  Serial          Resource      PCI    PCI   SCSI  SCSI  SCSI  Status");
        mvwaddstr(p_title_win, 3, 0,
                  "  Set   Number   Model  Name          Bus    Dev    Bus    ID   Lun");
    }

    mvwaddstr(p_menu_win, 3, 0,
              EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel   r=Refresh    f=PageDn   b=PageUp");

    array_num = 1;
    check_current_config(false);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            continue;
        }

        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.buffer = &cmd_status;
        ioa_cmd.buffer_len = sizeof(cmd_status);
        ioa_cmd.read_not_write = 1;

        rc = ipr_ioctl(fd, IPR_QUERY_COMMAND_STATUS, &ioa_cmd);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Query Command Status to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            close(fd);
            continue;
        }

        array_query_data = p_cur_ioa->p_qac_data;
        p_common_record =
            (struct ipr_record_common *)array_query_data->data;

        for (i = 0;
             i < array_query_data->num_records;
             i++, p_common_record =
             (struct ipr_record_common *)((unsigned long)p_common_record + iprtoh16(p_common_record->record_len)))
        {
            if (p_common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD)
            {
                p_array_record = (struct ipr_array_record *)p_common_record;
                array_id =  p_array_record->array_id;

                /* Ignore array candidates */
                if (!p_array_record->established)
                    continue;

                /* Print out "array entry" */
                len += sprintf(buffer, "%4d   %8s  %x   001   %s",
                               array_num++, p_cur_ioa->serial_num, p_cur_ioa->ccin,
                               p_cur_ioa->ioa.dev_name);
            }
            else if (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD)
            {
                p_array2_record = (struct ipr_array2_record *)p_common_record;
                array_id =  p_array2_record->array_id;

                /* Ignore array candidates */
                if (!p_array2_record->established)
                    continue;

                if (p_array2_record->no_config_entry)
                {
                    char serial_num[IPR_SERIAL_NUM_LEN+1];

                    /* Print out "array entry" */
                    len += sprintf(buffer, "%4d   ",array_num++);

                    memcpy(serial_num, p_array2_record->serial_number, IPR_SERIAL_NUM_LEN);
                    serial_num[IPR_SERIAL_NUM_LEN] = '\0';

                    len += sprintf(buffer + len, "%8s  ", serial_num);

                    if (platform == IPR_ISERIES)
                        len += sprintf(buffer + len, "6600   ");

                    len += sprintf(buffer + len, "%03d   ", IPR_VSET_MODEL_NUMBER);

                    if (platform == IPR_ISERIES)
                    {
                        len += sprintf(buffer + len, "%-38s", " ");
                    }
                    else
                    {
                        len += sprintf(buffer + len, "%-16s", " ");
                        len += sprintf(buffer + len, "%02d     %02d",
                                       p_cur_ioa->ioa.p_resource_entry->pci_bus_number,
                                       p_cur_ioa->ioa.p_resource_entry->pci_slot);

                        len += sprintf(buffer + len, "    %3d    %2d    %2d  ",
                                       p_array2_record->last_resource_address.bus,
                                       p_array2_record->last_resource_address.target,
                                       p_array2_record->last_resource_address.lun);
                    }
                }
                else
                {
                    for (j = 0;
                         j < p_cur_ioa->num_devices;
                         j++)
                    {
                        if (p_cur_ioa->dev[j].p_resource_entry->resource_handle ==
                            p_array2_record->resource_handle)
                        {
                            /* Print out "array entry" */
                            len += sprintf(buffer, "%4d   ",array_num++);

                            len += print_dev_pty2(p_cur_ioa->dev[j].p_resource_entry,
                                                  buffer + len,
                                                  p_cur_ioa->dev[j].dev_name);

                            /* Do a query resource state */
                            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                            ioa_cmd.buffer = &res_state;
                            ioa_cmd.buffer_len = sizeof(res_state);
                            ioa_cmd.read_not_write = 1;
                            ioa_cmd.device_cmd = 1;
                            ioa_cmd.resource_address =
                                p_cur_ioa->dev[j].p_resource_entry->resource_address;

                            memset(&res_state, 0, sizeof(res_state));

                            rc = ipr_ioctl(fd, IPR_QUERY_RESOURCE_STATE, &ioa_cmd);

                            if (rc != 0)
                            {
                                syslog(LOG_ERR, "Query Resource State to a device (0x%08X) failed. %m"IPR_EOL,
                                       IPR_GET_PHYSICAL_LOCATOR(p_resource_entry->resource_address));
                                res_state.not_oper = 1;
                            }

                            len += print_parity_status(buffer + len, array_id, &res_state, &cmd_status);

                            break;
                        }
                    }
                }
            }
            else
            {
                continue;
            }

            if ((num_lines > 0) && ((num_lines % max_size) == 0))
            {
                mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                p_tail_win = p_tail_win->p_next;

                p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                p_tail_panel = p_tail_panel->p_next;

                p_tail_win->p_win = newwin(max_size + 1,
                                           curses_info.max_x,
                                           curses_info.start_y + RAID_STATUS_HDR_SIZE,
                                           curses_info.start_x);
                p_tail_win->p_next = NULL;

                p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                p_tail_panel->p_next = NULL;
            }

            mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);

            memset(buffer, 0, 100);

            len = 0;
            num_lines++;

            for (j = 0;
                 j < p_cur_ioa->num_devices;
                 j++)
            {
                p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

                if ((p_resource_entry->is_array_member) &&
                    (array_id == p_resource_entry->array_id))
                {
                    len += sprintf(buffer + len, "       ");
                    len += print_dev_pty2(p_resource_entry,
                                          buffer + len,
                                          p_cur_ioa->dev[j].dev_name);

                    /* Do a query resource state */
                    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                    ioa_cmd.buffer = &res_state;
                    ioa_cmd.buffer_len = sizeof(res_state);
                    ioa_cmd.read_not_write = 1;
                    ioa_cmd.device_cmd = 1;
                    ioa_cmd.resource_address = p_resource_entry->resource_address;

                    memset(&res_state, 0, sizeof(res_state));

                    rc = ipr_ioctl(fd, IPR_QUERY_RESOURCE_STATE, &ioa_cmd);

                    if (rc != 0)
                    {
                        syslog(LOG_ERR, "Query Resource State to a device (0x%08X) failed. %m"IPR_EOL,
                               IPR_GET_PHYSICAL_LOCATOR(p_resource_entry->resource_address));
                        res_state.not_oper = 1;
                    }

                    len += print_parity_status(buffer + len, array_id, &res_state, &cmd_status);

                    if ((num_lines > 0) && ((num_lines % max_size) == 0))
                    {
                        mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                        p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                        p_tail_win = p_tail_win->p_next;

                        p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                        p_tail_panel = p_tail_panel->p_next;

                        p_tail_win->p_win = newwin(max_size + 1,
                                                   curses_info.max_x,
                                                   curses_info.start_y + RAID_STATUS_HDR_SIZE,
                                                   curses_info.start_x);
                        p_tail_win->p_next = NULL;

                        p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                        p_tail_panel->p_next = NULL;
                    }

                    mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);

                    memset(buffer, 0, 100);

                    len = 0;
                    num_lines++;
                }
            }
        }

        /* now display Hot Spares if any */
        is_first = 1;
        for (j = 0;
             j < p_cur_ioa->num_devices;
             j++)
        {
            p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

            if (p_resource_entry->is_hot_spare)
            {
                /* Print out "adapter entry" */
                if (is_first)
                {
                    is_first = 0;
                    len = sprintf(buffer, "   -   ");
                    len += print_dev_pty(p_cur_ioa->ioa.p_resource_entry,
                                         buffer + len,
                                         p_cur_ioa->ioa.dev_name);

                    if ((num_lines > 0) && ((num_lines % max_size) == 0))
                    {
                        mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                        p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                        p_tail_win = p_tail_win->p_next;

                        p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                        p_tail_panel = p_tail_panel->p_next;

                        p_tail_win->p_win = newwin(max_size + 1,
                                                   curses_info.max_x,
                                                   curses_info.start_y + RAID_STATUS_HDR_SIZE,
                                                   curses_info.start_x);
                        p_tail_win->p_next = NULL;

                        p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                        p_tail_panel->p_next = NULL;
                    }

                    mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);

                    memset(buffer, 0, 100);
                    len = 0;
                    num_lines++;
                }

                len = sprintf(buffer + len, "       ");

                len += print_dev_pty(p_resource_entry,
                                     buffer + len,
                                     p_cur_ioa->dev[j].dev_name);

                /* Do a query resource state */
                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                ioa_cmd.buffer = &res_state;
                ioa_cmd.buffer_len = sizeof(res_state);
                ioa_cmd.read_not_write = 1;
                ioa_cmd.device_cmd = 1;
                ioa_cmd.resource_address = p_resource_entry->resource_address;

                memset(&res_state, 0, sizeof(res_state));

                rc = ipr_ioctl(fd, IPR_QUERY_RESOURCE_STATE, &ioa_cmd);

                if (rc != 0)
                {
                    syslog(LOG_ERR, "Query Resource State to a device (0x%08X) failed. %m"IPR_EOL,
                           IPR_GET_PHYSICAL_LOCATOR(p_resource_entry->resource_address));
                    res_state.not_oper = 1;
                }

                len += print_parity_status(buffer + len, IPR_INVALID_ARRAY_ID,
                                           &res_state, &cmd_status);

                if ((num_lines > 0) && ((num_lines % max_size) == 0))
                {
                    mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                    p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                    p_tail_win = p_tail_win->p_next;

                    p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                    p_tail_panel = p_tail_panel->p_next;

                    p_tail_win->p_win = newwin(max_size + 1,
                                               curses_info.max_x,
                                               curses_info.start_y + RAID_STATUS_HDR_SIZE,
                                               curses_info.start_x);
                    p_tail_win->p_next = NULL;

                    p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                    p_tail_panel->p_next = NULL;
                }

                mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);

                memset(buffer, 0, 100);

                len = 0;
                num_lines++;
            }
        }
        close(fd);
    }


    if (num_lines == 0)
        mvwaddstr(p_tail_win->p_win, 1, 2, "(Device parity protection has not been started)");

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */
        update_panels();
        wrefresh(p_menu_win);
        wrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if (IS_REFRESH_KEY(ch))
            {
                rc = RC_REFRESH;
                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(panel_below(NULL));
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(panel_above(NULL));
                    update_panels();
                    doupdate();
                }
                break;
            }
            else
            {
                mvwaddstr(p_menu_win, 4, 0, "Invalid option specified.");
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                update_panels();
                wrefresh(p_menu_win);
                wrefresh(p_title_win);
                doupdate();
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}

int print_parity_status(char *buffer,
                        u8 array_id,
                        struct ipr_query_res_state *p_res_state,
                        struct ipr_cmd_status *p_cmd_status)
{
    int len = 0;
    struct ipr_cmd_status_record *p_status_record;
    int k;

    if (p_res_state->not_oper)
        len += sprintf(buffer + len, "Failed");
    else if (p_res_state->not_ready)
        len += sprintf(buffer + len, "Not Ready");
    else if (p_res_state->read_write_prot)
        len += sprintf(buffer + len, "R/W Prot");
    else if (p_res_state->prot_dev_failed)
        len += sprintf(buffer + len, "Failed");
    else
    {

        p_status_record = p_cmd_status->record;
        for (k = 0;
             k < p_cmd_status->num_records;
             k++, p_status_record =
             (struct ipr_cmd_status_record *)((unsigned long)p_status_record + iprtoh16(p_status_record->length)))
        {
            if ((p_status_record->array_id == array_id) &&
                (p_status_record->status & IPR_CMD_STATUS_IN_PROGRESS))
            {
                len += sprintf(buffer + len,"%d%% ",p_status_record->percent_complete);
                switch(p_status_record->command_code)
                {
                    case IPR_RESYNC_ARRAY_PROTECTION:
                        len += sprintf(buffer + len, "Synched");
                        break;
                    case IPR_REBUILD_DEVICE_DATA:
                        len += sprintf(buffer + len, "Rebuilt");
                        break;
                }
                break;
            }
        }

        if (k == p_cmd_status->num_records)
        {
            if ((p_res_state->prot_suspended ||
                 p_res_state->prot_resuming))
            {
                len += sprintf(buffer + len, "Unprot");
            }
            else
            {
                len += sprintf(buffer + len, "Active");
            }
        }
    }

    return len;
}

int raid_stop()
{
/* iSeries
                         Stop Device Parity Protection

Select the subsystems to stop device parity protection.

Type choice, press Enter.
  1=Stop device parity protection

         Parity  Serial                Resource
Option     Set   Number   Type  Model  Name
            1   02127005  2782   001   /dev/ipr4
            2   00103039  2748   001   /dev/ipr0

e=Exit   q=Cancel    f=PageDn   b=PageUp
*/

/* non-iSeries
                         Stop Device Parity Protection

Select the subsystems to stop device parity protection.

Type choice, press Enter.
  1=Stop device parity protection

         Parity  Vendor    Product           Serial          Resource
Option     Set    ID         ID              Number   Model  Name
            1    IBM      DAB61C0E          3AFB348C   200   /dev/sdb
            2    IBM      5C88AFDC          A775915E   200   /dev/sdc


e=Exit   q=Cancel    f=PageDn   b=PageUp
*/
    int rc, i, array_num, last_char;
    struct ipr_record_common *p_common_record;
    struct ipr_array_record *p_array_record;
    struct array_cmd_data *p_cur_raid_cmd;
    u8 array_id;
    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    char buffer[100];
    int array_index;
    int max_size = curses_info.max_y - 12;
    int confirm_raid_stop();
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    int found = 0;
    char *p_char;
    rc = RC_SUCCESS;


    p_title_win = newwin(9, curses_info.max_x, 0, 0);
    p_menu_win = newwin(3, curses_info.max_x, max_size + 9, 0);
    p_field_win = newwin(max_size, curses_info.max_x, 9, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],                       /* field to alter */
                     0,                                 /* number of buffer to alter */
                     "Stop Device Parity Protection");  /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    array_num = 1;

    check_current_config(false);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        p_cur_ioa->num_raid_cmds = 0;

        for (i = 0;
             i < p_cur_ioa->num_devices;
             i++)
        {
            p_common_record = p_cur_ioa->dev[i].p_qac_entry;

            if ((p_common_record != NULL) &&
                ((p_common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) ||
                (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD)))
            {
                p_array_record = (struct ipr_array_record *)p_common_record;

                if (p_array_record->stop_cand)
                {
                    rc = is_array_in_use(p_cur_ioa, p_array_record->array_id);
                    if (rc != 0) continue;

                    if (p_head_raid_cmd)
                    {
                        p_tail_raid_cmd->p_next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                        p_tail_raid_cmd = p_tail_raid_cmd->p_next;
                    }
                    else
                    {
                        p_head_raid_cmd = p_tail_raid_cmd = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                    }

                    p_tail_raid_cmd->p_next = NULL;

                    memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

                    array_id =  p_array_record->array_id;

                    p_tail_raid_cmd->array_id = array_id;
                    p_tail_raid_cmd->array_num = array_num;
                    p_tail_raid_cmd->p_ipr_ioa = p_cur_ioa;
                    p_tail_raid_cmd->p_ipr_device = &p_cur_ioa->dev[i];

                    start_y = (array_num-1) % (max_size - 1);

                    if ((num_lines > 0) && (start_y == 0))
                    {
                        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                        pp_field[field_index] = new_field(1, curses_info.max_x,
                                                          max_size - 1,
                                                          0,
                                                          0, 0);

                        set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                        set_field_buffer(pp_field[field_index], 0, "More... ");
                        set_field_userptr(pp_field[field_index], NULL);
                        field_opts_off(pp_field[field_index++],
                                       O_ACTIVE);
                    }

                    /* User entry field */
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, 1, start_y, 3, 0, 0);

                    set_field_userptr(pp_field[field_index++], (char *)p_tail_raid_cmd);

                    /* Description field */
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, curses_info.max_x - 9, start_y, 9, 0, 0);

                    if (platform == IPR_ISERIES)
                    {
                        len = sprintf(buffer, "%4d   %8s  %x   %03d   %s",
                                      array_num,
                                      p_cur_ioa->dev[i].p_resource_entry->serial_num,
                                      p_cur_ioa->dev[i].p_resource_entry->type,
                                      p_cur_ioa->dev[i].p_resource_entry->model,
                                      p_cur_ioa->dev[i].dev_name);
                    }
                    else
                    {
                        char buf[100];

                        len = sprintf(buffer, "%4d    ", array_num);

                        memcpy(buf, p_cur_ioa->dev[i].p_resource_entry->std_inq_data.vpids.vendor_id,
                               IPR_VENDOR_ID_LEN);
                        buf[IPR_VENDOR_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "%-8s ", buf);

                        memcpy(buf, p_cur_ioa->dev[i].p_resource_entry->std_inq_data.vpids.product_id,
                               IPR_PROD_ID_LEN);
                        buf[IPR_PROD_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "%-12s  ", buf);

                        len += sprintf(buffer + len, "%8s   %03d   %s",
                                       p_cur_ioa->dev[i].p_resource_entry->serial_num,
                                       p_cur_ioa->dev[i].p_resource_entry->model,
                                       p_cur_ioa->dev[i].dev_name);
                    }
                    set_field_buffer(pp_field[field_index], 0, buffer);
                    field_opts_off(pp_field[field_index], O_ACTIVE);
                    set_field_userptr(pp_field[field_index++], NULL);

                    memset(buffer, 0, 100);

                    len = 0;
                    num_lines++;
                    array_num++;
                }
            }
        }
    }

    if (array_num == 1)
    {
        set_field_buffer(p_fields[0],                       /* field to alter */
                         0,                                 /* number of buffer to alter */
                         "Stop Device Parity Protection Failed");  /* string value to set */

        mvwaddstr(p_title_win, 2, 0, "There are no disk units eligible for the selected operation");
        mvwaddstr(p_title_win, 3, 0, "due to one or more of the following reasons:");
        mvwaddstr(p_title_win, 5, 0, "o  There are no device parity protected units in the system.");
        mvwaddstr(p_title_win, 6, 0, "o  An IOA is in a condition that makes the disk units attached to");
        mvwaddstr(p_title_win, 7, 0, "   it read/write protected.  Examine the kernel messages log");
        mvwaddstr(p_title_win, 8, 0, "   for any errors that are logged for the IO subsystem");
        mvwaddstr(p_field_win, 0, 0, "   and follow the appropriate procedure for the reference code to");
        mvwaddstr(p_field_win, 1, 0, "   correct the problem, if necessary.");
        mvwaddstr(p_field_win, 2, 0, "o  Not all disk units attached to an advanced function IOA have");
        mvwaddstr(p_field_win, 3, 0, "   reported to the system.  Retry the operation.");
        mvwaddstr(p_field_win, 4, 0, "o  The disk units are missing.");
        mvwaddstr(p_field_win, 5, 0, "o  The disk unit/s are currently in use.");
        mvwaddstr(p_field_win, max_size -1, 0, "Press Enter to continue.");
        mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD);     /* form request code */

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        wnoutrefresh(p_field_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
        }
    }

    mvwaddstr(p_title_win, 2, 0, "Select the subsystems to stop device parity protection.");
    mvwaddstr(p_title_win, 4, 0, "Type choice, press Enter.");
    mvwaddstr(p_title_win, 5, 0, "  1=Stop device parity protection");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 7, 0, "         Parity  Serial                Resource");
        mvwaddstr(p_title_win, 8, 0, "Option     Set   Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 7, 0, "         Parity  Vendor    Product           Serial          Resource");
        mvwaddstr(p_title_win, 8, 0, "Option     Set    ID         ID              Number   Model  Name");
    }
    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");


    field_opts_off(p_fields[1], O_ACTIVE);

    array_index = ((max_size - 1) * 2) + 1;
    for (i = array_index; i < field_index; i += array_index)
        set_new_page(pp_field[i], TRUE);

    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
    pp_field[field_index] = NULL;

    flush_stdscr();

    p_list_form = new_form(pp_field);

    set_form_win(p_list_form, p_field_win);

    post_form(p_list_form);

    wnoutrefresh(stdscr);
    wnoutrefresh(p_menu_win);
    wnoutrefresh(p_title_win);
    form_driver(p_list_form,
                REQ_FIRST_FIELD);
    wnoutrefresh(p_field_win);
    doupdate();

    while(1)
    {
        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_NEXT_PAGE);
                form_driver(p_list_form,
                            REQ_FIRST_FIELD);

                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_PREV_PAGE);
                form_driver(p_list_form,
                            REQ_FIRST_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if ((ch == KEY_DOWN) ||
                     (ch == '\t'))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_NEXT_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (ch == KEY_UP)
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_PREV_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (IS_ENTER_KEY(ch))
            {
                found = 0;
                unpost_form(p_list_form);

                for (i = 0; i < field_index; i++)
                {
                    p_cur_raid_cmd = (struct array_cmd_data *)field_userptr(pp_field[i]);
                    if (p_cur_raid_cmd != NULL)
                    {
                        p_char = field_buffer(pp_field[i], 0);
                        if (strcmp(p_char, "1") == 0)
                        {
                            found = 1;
                            p_cur_raid_cmd->do_cmd = 1;

                            p_array_record = (struct ipr_array_record *)p_cur_raid_cmd->p_ipr_device->p_qac_entry;
                            if (!p_array_record->issue_cmd)
                            {
                                p_array_record->issue_cmd = 1;

                                /* known_zeroed means do not preserve
                                   user data on stop */
                                p_array_record->known_zeroed = 1;
                                p_cur_raid_cmd->p_ipr_ioa->num_raid_cmds++;
                            }
                        }
                        else
                        {
                            p_cur_raid_cmd->do_cmd = 0;
                            p_array_record = (struct ipr_array_record *)p_cur_raid_cmd->p_ipr_device->p_qac_entry;

                            if (p_array_record->issue_cmd)
                            {
                                p_array_record->issue_cmd = 0;
                                p_cur_raid_cmd->p_ipr_ioa->num_raid_cmds--;
                            }
                        }
                    }
                }

                if (found != 0)
                {
                    /* Go to verification screen */
                    unpost_form(p_form);
                    rc = confirm_raid_stop();
                    goto leave;
                }
                else
                {
                    post_form(p_list_form);
                    rc = 1;
                }     
            }
            else
            {
                /* Terminal is running under 5250 telnet */
                if (term_is_5250)
                {
                    /* Send it to the form to be processed and flush this line */
                    form_driver(p_list_form, ch);
                    last_char = flush_line();
                    if (IS_5250_CHAR(last_char))
                        ungetch(ENTER_KEY);
                    else if (last_char != ERR)
                        ungetch(last_char);
                }
                else
                {
                    form_driver(p_list_form, ch);
                }
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(pp_field);

    p_cur_raid_cmd = p_head_raid_cmd;
    while(p_cur_raid_cmd)
    {
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }

    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}


int confirm_raid_stop()
{
#define CONFIRM_STOP_HDR_SIZE		10
#define CONFIRM_STOP_MENU_SIZE	3
/* iSeries
                     Confirm Stop Device Parity Protection

ATTENTION: Disk units connected to these subsystems will not be
protected after you confirm your choice.

Press Enter to continue. 
Press q=Cancel to return and change your choice.

         Parity  Serial                Resource
Option     Set   Number   Type  Model  Name
    1       1   02127005  2782   001   /dev/ipr4
    1       1   002443C6  6713   050   /dev/sdr    
    1       1   0024B45C  6713   050   /dev/sds
    1       1   00000DC7  6717   050   /dev/sdt
    1       1   000BBBC1  6717   050   /dev/sdu
    1       1   0002F5AA  6717   050   /dev/sdv

q=Cancel    f=PageDn   b=PageUp
*/
/* non-iSeries

                     Confirm Stop Device Parity Protection

ATTENTION: Disk units connected to these subsystems will not be
protected after you confirm your choice.

Press Enter to continue.         
Press q=Cancel to return and change your choice.

         Parity   Vendor    Product           Serial          Resource
Option     Set     ID         ID              Number   Model  Name   
    1       1     IBM       DAB61C0E         3AFB348C   200   /dev/sdb
    1       1     IBMAS400  DCHS09W          0024A0B9   070
    1       1     IBMAS400  DFHSS4W          00D37A11   070

q=Cancel    f=PageDn   b=PageUp         

*/
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    struct ipr_device *p_ipr_device;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_device_record *p_device_record;
    struct ipr_resource_flags *p_resource_flags;
    struct ipr_ioa *p_cur_ioa;
    int fd, rc, j;
    u16 len;
    char buffer[100];
    u32 num_lines = 0;
    int max_size = curses_info.max_y - (CONFIRM_STOP_HDR_SIZE + CONFIRM_STOP_MENU_SIZE + 1);
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    PANEL *p_panel;

    int raid_stop_complete();

    p_title_win = newwin(CONFIRM_STOP_HDR_SIZE,
                         curses_info.max_x,
                         curses_info.start_y,
                         curses_info.start_x);

    p_menu_win = newwin(CONFIRM_STOP_MENU_SIZE,
                        curses_info.max_x,
                        curses_info.start_y + max_size + CONFIRM_STOP_HDR_SIZE + 1,
                        curses_info.start_x);

    rc = RC_SUCCESS;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Confirm Stop Device Parity Protection");

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + CONFIRM_STOP_HDR_SIZE,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    wattron(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 0, "ATTENTION:");
    wattroff(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 10, " Disk units connected to these subsystems will not be");
    mvwaddstr(p_title_win, 3, 0, "protected after you confirm your choice.");
    mvwaddstr(p_title_win, 5, 0, "Press Enter to continue.");
    mvwaddstr(p_title_win, 6, 0, "Press "CANCEL_KEY_LABEL"=Cancel to return and change your choice.");

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 8, 0, "         Parity  Serial                Resource");
        mvwaddstr(p_title_win, 9, 0, "Option     Set   Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 8, 0, "         Parity   Vendor    Product           Serial          Resource");
        mvwaddstr(p_title_win, 9, 0, "Option     Set     ID         ID              Number   Model  Name");
    }

    mvwaddstr(p_menu_win, 1, 0, CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_raid_cmd = p_head_raid_cmd;

    while(p_cur_raid_cmd)
    {
        if (p_cur_raid_cmd->do_cmd)
        {
            p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
            p_ipr_device = p_cur_raid_cmd->p_ipr_device;

            if (platform == IPR_ISERIES)
            {
                /* Print out "array entry" */
                len = sprintf(buffer, "    1    %4d   %8s  %x   %03d   %s",
                              p_cur_raid_cmd->array_num,
                              p_ipr_device->p_resource_entry->serial_num,
                              p_ipr_device->p_resource_entry->type,
                              p_ipr_device->p_resource_entry->model,
                              p_ipr_device->dev_name);
            }
            else
            {
                char buf[100];

                /* Print out "array entry" */
                len = sprintf(buffer, "    1    %4d     ", p_cur_raid_cmd->array_num);

                memcpy(buf, p_ipr_device->p_resource_entry->std_inq_data.vpids.vendor_id,
                       IPR_VENDOR_ID_LEN);
                buf[IPR_VENDOR_ID_LEN] = '\0';

                len += sprintf(buffer + len, "%-8s  ", buf);

                memcpy(buf, p_ipr_device->p_resource_entry->std_inq_data.vpids.product_id,
                       IPR_PROD_ID_LEN);
                buf[IPR_PROD_ID_LEN] = '\0';

                len += sprintf(buffer + len, "%-12s ", buf);

                len += sprintf(buffer + len, "%8s   %03d   %s",
                               p_ipr_device->p_resource_entry->serial_num,
                               p_ipr_device->p_resource_entry->model,
                               p_ipr_device->dev_name);
            }

            if ((num_lines > 0) && ((num_lines % max_size) == 0))
            {
                mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                p_tail_win = p_tail_win->p_next;

                p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                p_tail_panel = p_tail_panel->p_next;

                p_tail_win->p_win = newwin(max_size + 1,
                                           curses_info.max_x,
                                           curses_info.start_y + CONFIRM_STOP_HDR_SIZE,
                                           curses_info.start_x);
                p_tail_win->p_next = NULL;

                p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                p_tail_panel->p_next = NULL;
            }

            mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
            memset(buffer, 0, 100);
            num_lines++;

            for (j = 0; j < p_cur_ioa->num_devices; j++)
            {
                p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

                if (p_resource_entry->is_array_member &&
                    (p_cur_raid_cmd->array_id == p_resource_entry->array_id))
                {
                    len = sprintf(buffer, "    1    %4d   ", p_cur_raid_cmd->array_num);

                    p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;
                    p_resource_flags = &p_device_record->resource_flags_to_become;

                    if (platform == IPR_ISERIES)
                    {
                        len += print_dev2(p_resource_entry,
                                          buffer + len,
                                          p_resource_flags,
                                          p_cur_ioa->dev[j].dev_name);
                    }
                    else
                    {
                        char buf[100];

                        memcpy(buf, p_cur_ioa->dev[j].p_resource_entry->std_inq_data.vpids.vendor_id,
                               IPR_VENDOR_ID_LEN);
                        buf[IPR_VENDOR_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "  %-8s  ", buf);

                        memcpy(buf, p_cur_ioa->dev[j].p_resource_entry->std_inq_data.vpids.product_id,
                               IPR_PROD_ID_LEN);
                        buf[IPR_PROD_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "%-12s ", buf);

                        len += sprintf(buffer + len, "%8s   %03d   %s",
                                       p_resource_entry->serial_num,
                                       p_resource_entry->model,
                                       p_cur_ioa->dev[j].dev_name);
                    }

                    if ((num_lines > 0) && ((num_lines % max_size) == 0))
                    {
                        mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                        p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                        p_tail_win = p_tail_win->p_next;

                        p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                        p_tail_panel = p_tail_panel->p_next;

                        p_tail_win->p_win = newwin(max_size + 1,
                                                   curses_info.max_x,
                                                   curses_info.start_y + CONFIRM_STOP_HDR_SIZE,
                                                   curses_info.start_x);
                        p_tail_win->p_next = NULL;

                        p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                        p_tail_panel->p_next = NULL;
                    }

                    mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
                    memset(buffer, 0, 100);
                    num_lines++;
                }
            }
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        wnoutrefresh(stdscr);
        update_panels();
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_REFRESH;
                goto leave;
            }
            else if (IS_ENTER_KEY(ch))
            {
                p_cur_raid_cmd = p_head_raid_cmd;
                while(p_cur_raid_cmd)
                {
                    if (p_cur_raid_cmd->do_cmd)
                    {
                        p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
                        p_ipr_device = p_cur_raid_cmd->p_ipr_device;

                        rc = is_array_in_use(p_cur_ioa, p_cur_raid_cmd->array_id);
                        if (rc != 0)
                        {
                            syslog(LOG_ERR,
                                   "Array %s is currently in use and cannot be deleted."
                                   IPR_EOL, p_ipr_device->dev_name);
                            rc = RC_FAILED;
                            goto leave;
                        }

                        if (p_ipr_device->p_qac_entry->record_id == IPR_RECORD_ID_ARRAY2_RECORD)
                        {
                            fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
                            if (fd <= 1)
                            {
                                rc = RC_FAILED;
                                syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                                goto leave;
                            }

                            mvaddstr(curses_info.max_y - 1, 0,
                                     "Operation in progress - please wait");

                            form_driver(p_form,               /* form to pass input to */
                                        REQ_LAST_FIELD );     /* form request code */

                            wnoutrefresh(stdscr);
                            update_panels();
                            wnoutrefresh(p_menu_win);
                            wnoutrefresh(p_title_win);
                            doupdate();

                            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                            ioa_cmd.buffer = NULL;
                            ioa_cmd.buffer_len = 0;
                            ioa_cmd.device_cmd = 1;
                            ioa_cmd.cdb[4] = IPR_START_STOP_STOP;
                            ioa_cmd.resource_address = p_ipr_device->p_resource_entry->resource_address;

                            rc = ipr_ioctl(fd, START_STOP, &ioa_cmd);
                            close(fd);

                            if (rc != 0)
                            {
                                syslog(LOG_ERR, "Stop Start/Stop Unit to %s failed. %m"IPR_EOL, p_ipr_device->dev_name);
                                rc = RC_FAILED;
                                goto leave;
                            }
                        }
                    }
                    p_cur_raid_cmd = p_cur_raid_cmd->p_next;
                }

                for (p_cur_ioa = p_head_ipr_ioa;
                     p_cur_ioa != NULL;
                     p_cur_ioa = p_cur_ioa->p_next)
                {
                    if (p_cur_ioa->num_raid_cmds > 0)
                    {
                        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
                        if (fd <= 1)
                        {
                            rc = RC_FAILED;
                            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                            goto leave;
                        }

                        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                        ioa_cmd.buffer = p_cur_ioa->p_qac_data;
                        ioa_cmd.buffer_len = iprtoh16(p_cur_ioa->p_qac_data->resp_len);
                        ioa_cmd.read_not_write = 0;

                        rc = ipr_ioctl(fd, IPR_STOP_ARRAY_PROTECTION, &ioa_cmd);
                        close(fd);

                        if (rc != 0)
                        {
                            syslog(LOG_ERR, "Stop Array Protection to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                            rc = RC_FAILED;
                            goto leave;
                        }
                    }
                }

                rc = raid_stop_complete();
                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else
            {
                break;
            }
        }

    }

    leave:
        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}


int raid_stop_complete()
{
    /*
                 Stop Device Parity Protection Status

     You selected to stop device parity protection









                                16% Complete
     */

    FIELD *p_fields[3];
    FORM *p_form;
    FORM *p_old_form;
    struct ipr_cmd_status cmd_status;
    struct ipr_cmd_status_record *p_record;
    int done_bad;
    int not_done = 0;
    int rc, j, fd;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    u32 percent_cmplt = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 num_stops = 0;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_record_common *p_common_record;
    struct ipr_resource_entry *p_resource_entry;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            curses_info.max_y - 1, curses_info.max_x - 1, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Stop Device Parity Protection Status");

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    while(1)
    {
        clear();
        post_form(p_form);

        mvaddstr(2, 0, "You selected to stop device parity protection");
        mvprintw(curses_info.max_y / 2, (curses_info.max_x / 2) - 7, "%d%% Complete", percent_cmplt);

        form_driver(p_form,
                    REQ_FIRST_FIELD);
        refresh();

        percent_cmplt = 100;
        done_bad = 0;
        num_stops = 0;
        not_done = 0;

        for (p_cur_ioa = p_head_ipr_ioa;
             p_cur_ioa != NULL;
             p_cur_ioa = p_cur_ioa->p_next)
        {
            if (p_cur_ioa->num_raid_cmds > 0)
            {
                fd = open(p_cur_ioa->ioa.dev_name, O_RDONLY);

                if (fd < 0)
                {
                    syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    p_cur_ioa->num_raid_cmds = 0;
                    done_bad = 1;
                    continue;
                }
                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                ioa_cmd.buffer = &cmd_status;
                ioa_cmd.buffer_len = sizeof(cmd_status);
                ioa_cmd.read_not_write = 1;

                rc = ipr_ioctl(fd, IPR_QUERY_COMMAND_STATUS, &ioa_cmd);

                if (rc)
                {
                    syslog(LOG_ERR, "Query Command Status to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    close(fd);
                    p_cur_ioa->num_raid_cmds = 0;
                    done_bad = 1;
                    continue;
                }

                close(fd);
                p_record = cmd_status.record;

                for (j=0; j < cmd_status.num_records; j++, p_record++)
                {
                    if (p_record->command_code == IPR_STOP_ARRAY_PROTECTION)
                    {
                        for (p_cur_raid_cmd = p_head_raid_cmd;
                             p_cur_raid_cmd != NULL; 
                             p_cur_raid_cmd = p_cur_raid_cmd->p_next)
                        {
                            if ((p_cur_raid_cmd->p_ipr_ioa = p_cur_ioa) &&
                                (p_cur_raid_cmd->array_id == p_record->array_id))
                            {
                                num_stops++;
                                if (p_record->status == IPR_CMD_STATUS_IN_PROGRESS)
                                {
                                    if (p_record->percent_complete < percent_cmplt)
                                        percent_cmplt = p_record->percent_complete;
                                    not_done = 1;
                                }
                                else if (p_record->status != IPR_CMD_STATUS_SUCCESSFUL)
                                {
                                    rc = RC_FAILED;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        unpost_form(p_form);

        if (!not_done)
        {
            free_form(p_form);
            curses_info.p_form = p_old_form;
            for (j=0; j < 2; j++)
                free_field(p_fields[j]);

            if (done_bad)
            {
                flush_stdscr();
                return RC_FAILED;
            }

            for (p_cur_raid_cmd = p_head_raid_cmd;
                 p_cur_raid_cmd != NULL; 
                 p_cur_raid_cmd = p_cur_raid_cmd->p_next)
            {
                p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
                p_common_record = p_cur_raid_cmd->p_ipr_device->p_qac_entry;
                if ((p_common_record != NULL) &&
                    (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD))
                {
                    p_resource_entry = p_cur_raid_cmd->p_ipr_device->p_resource_entry;
                    remove_device(p_resource_entry->resource_address, p_cur_ioa);
                    scan_device(p_resource_entry->resource_address, p_cur_ioa->host_num);
                }
            }
            check_current_config(false);
            revalidate_devs(NULL);
            flush_stdscr();
            return RC_SUCCESS;
        }
        not_done = 0;
        sleep(2);
    }
}

int raid_start()
{
/* iSeries
                         Start Device Parity Protection

Select the subsystems to start device parity protection.

Type choice, press Enter.
  1=Start device parity protection

         Parity  Serial                Resource
Option     Set   Number   Type  Model  Name
   1        1   02127005  5703   001   /dev/ipr1


e=Exit   q=Cancel    f=PageDn   b=PageUp

*/
/* non-iSeries
                         Start Device Parity Protection

Select the subsystems to start device parity protection.

Type choice, press Enter.
  1=Start device parity protection

         Parity  Vendor    Product           Serial          Resource
Option     Set    ID         ID              Number   Model  Name
            1    IBM      DAB61C0E          3AFB348C   200   /dev/sdb
            2    IBM      5C88AFDC          A775915E   200   /dev/sdc


e=Exit   q=Cancel    f=PageDn   b=PageUp

*/

    int rc, i, array_num, last_char;
    struct ipr_record_common *p_common_record;
    struct ipr_array_record *p_array_record;
    struct array_cmd_data *p_cur_raid_cmd;
    u8 array_id;
    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    char buffer[100];
    int array_index, num_vsets;
    int max_size = curses_info.max_y - 12;
    int confirm_raid_start();
    int configure_raid_start(struct array_cmd_data *p_cur_raid_cmd,
			     int num_vsets);
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    int found = 0;
    char *p_char;
    int ch;

    rc = RC_SUCCESS;

    p_title_win = newwin(9, curses_info.max_x, 0, 0);
    p_menu_win = newwin(3, curses_info.max_x, max_size + 9, 0);
    p_field_win = newwin(max_size, curses_info.max_x, 9, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],                        /* field to alter */
                     0,                                  /* number of buffer to alter */
                     "Start Device Parity Protection");  /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    array_num = 1;

    check_current_config(false);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        p_cur_ioa->num_raid_cmds = 0;

        for (i = 0;
             i < p_cur_ioa->num_devices;
             i++)
        {
            p_common_record = p_cur_ioa->dev[i].p_qac_entry;

            if ((p_common_record != NULL) &&
                ((p_common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) ||
                (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD)))
            {
                p_array_record = (struct ipr_array_record *)p_common_record;

                if (p_array_record->start_cand)
                {
                    rc = is_array_in_use(p_cur_ioa, p_array_record->array_id);
                    if (rc != 0) continue;

                    if (p_head_raid_cmd)
                    {
                        p_tail_raid_cmd->p_next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                        p_tail_raid_cmd = p_tail_raid_cmd->p_next;
                    }
                    else
                    {
                        p_head_raid_cmd = p_tail_raid_cmd = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                    }

                    p_tail_raid_cmd->p_next = NULL;

                    memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

                    array_id =  p_array_record->array_id;

                    p_tail_raid_cmd->array_id = array_id;
                    p_tail_raid_cmd->array_num = array_num;
                    p_tail_raid_cmd->p_ipr_ioa = p_cur_ioa;
                    p_tail_raid_cmd->p_ipr_device = &p_cur_ioa->dev[i];
                    p_tail_raid_cmd->p_qac_data = NULL;

                    start_y = (array_num-1) % (max_size - 1);

                    if ((num_lines > 0) && (start_y == 0))
                    {
                        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                        pp_field[field_index] = new_field(1, curses_info.max_x,
                                                          max_size - 1,
                                                          0,
                                                          0, 0);

                        set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                        set_field_buffer(pp_field[field_index], 0, "More... ");
                        set_field_userptr(pp_field[field_index], NULL);
                        field_opts_off(pp_field[field_index++],
                                       O_ACTIVE);
                    }

                    /* User entry field */
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, 1, start_y, 3, 0, 0);

                    set_field_userptr(pp_field[field_index++], (char *)p_tail_raid_cmd);

                    /* Description field */
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, curses_info.max_x - 9, start_y, 9, 0, 0);

                    if (platform == IPR_ISERIES)
                    {
                        len = sprintf(buffer, "%4d   %8s  %x   %03d   %s",
                                      array_num,
                                      p_cur_ioa->dev[i].p_resource_entry->serial_num,
                                      p_cur_ioa->dev[i].p_resource_entry->type,
                                      p_cur_ioa->dev[i].p_resource_entry->model,
                                      p_cur_ioa->dev[i].dev_name);
                    }
                    else
                    {
                        char buf[100];

                        len = sprintf(buffer, "%4d    ", array_num);

                        memcpy(buf, p_cur_ioa->dev[i].p_resource_entry->std_inq_data.vpids.vendor_id,
                               IPR_VENDOR_ID_LEN);
                        buf[IPR_VENDOR_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "%-8s ", buf);

                        memcpy(buf, p_cur_ioa->dev[i].p_resource_entry->std_inq_data.vpids.product_id,
                               IPR_PROD_ID_LEN);
                        buf[IPR_PROD_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "%-12s  ", buf);

                        len += sprintf(buffer + len, "%8s   %03d   %s",
                                       p_cur_ioa->dev[i].p_resource_entry->serial_num,
                                       p_cur_ioa->dev[i].p_resource_entry->model,
                                       p_cur_ioa->dev[i].dev_name);
                    }

                    set_field_buffer(pp_field[field_index], 0, buffer);
                    field_opts_off(pp_field[field_index], O_ACTIVE);
                    set_field_userptr(pp_field[field_index++], NULL);

                    memset(buffer, 0, 100);

                    len = 0;
                    num_lines++;
                    array_num++;
                }
            }
        }
    }

    if (array_num == 1)
    {
        set_field_buffer(p_fields[0],                       /* field to alter */
                         0,                                 /* number of buffer to alter */
                         "Start Device Parity Protection Failed");  /* string value to set */

        mvwaddstr(p_title_win, 2, 0, "There are no disk units eligible for the selected operation");
        mvwaddstr(p_title_win, 3, 0, "due to one or more of the following reasons:");
        mvwaddstr(p_title_win, 5, 0, "o  There are not enough device parity capable units in the system.");
        mvwaddstr(p_title_win, 6, 0, "o  An IOA is in a condition that makes the disk units attached to");
        mvwaddstr(p_title_win, 7, 0, "   it read/write protected.  Examine the kernel messages log");
        mvwaddstr(p_title_win, 8, 0, "   for any errors that are logged for the IO subsystem");
        mvwaddstr(p_field_win, 0, 0, "   and follow the appropriate procedure for the reference code to");
        mvwaddstr(p_field_win, 1, 0, "   correct the problem, if necessary.");
        mvwaddstr(p_field_win, 2, 0, "o  Not all disk units attached to an advanced function IOA have");
        mvwaddstr(p_field_win, 3, 0, "   reported to the system.  Retry the operation.");
        mvwaddstr(p_field_win, 4, 0, "o  The disk units are missing.");
        mvwaddstr(p_field_win, 5, 0, "o  The disk unit/s are currently in use.");
        mvwaddstr(p_field_win, max_size -1, 0, "Press Enter to continue.");
        mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD);     /* form request code */

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        wnoutrefresh(p_field_win);
        doupdate();

        for (;;)
        {
            ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
        }
    }

    mvwaddstr(p_title_win, 2, 0, "Select the subsystems to start device parity protection.");
    mvwaddstr(p_title_win, 4, 0, "Type choice, press Enter.");
    mvwaddstr(p_title_win, 5, 0, "  1=Start device parity protection");

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 7, 0, "         Parity  Serial                Resource");
        mvwaddstr(p_title_win, 8, 0, "Option     Set   Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 7, 0, "         Parity  Vendor    Product           Serial          Resource");
        mvwaddstr(p_title_win, 8, 0, "Option     Set    ID         ID              Number   Model  Name");
    }

    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");


    field_opts_off(p_fields[1], O_ACTIVE);

    array_index = ((max_size - 1) * 2) + 1;
    for (i = array_index; i < field_index; i += array_index)
        set_new_page(pp_field[i], TRUE);

    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
    pp_field[field_index] = NULL;

    flush_stdscr();

    p_list_form = new_form(pp_field);

    set_form_win(p_list_form, p_field_win);

    post_form(p_list_form);

    form_driver(p_list_form,
                REQ_FIRST_FIELD);

    while(1)
    {
        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        wnoutrefresh(p_field_win);
        doupdate();

        ch = getch();

        if (IS_EXIT_KEY(ch))
        {
            rc = RC_EXIT;
            goto leave;
        }
        else if (IS_CANCEL_KEY(ch))
        {
            rc = RC_CANCEL;
            goto leave;
        }
        else if (IS_PGDN_KEY(ch))
        {
            form_driver(p_list_form,
                        REQ_NEXT_PAGE);
            form_driver(p_list_form,
                        REQ_FIRST_FIELD);
        }
        else if (IS_PGUP_KEY(ch))
        {
            form_driver(p_list_form,
                        REQ_PREV_PAGE);
            form_driver(p_list_form,
                        REQ_FIRST_FIELD);
        }
        else if ((ch == KEY_DOWN) ||
                 (ch == '\t'))
        {
            form_driver(p_list_form,
                        REQ_NEXT_FIELD);
        }
        else if (ch == KEY_UP)
        {
            form_driver(p_list_form,
                        REQ_PREV_FIELD);
        }
        else if (IS_ENTER_KEY(ch))
        {
            found = 0;

            unpost_form(p_list_form);
            p_cur_raid_cmd = p_head_raid_cmd;

            for (i = 0; i < field_index; i++)
            {
                p_cur_raid_cmd = (struct array_cmd_data *)field_userptr(pp_field[i]);
                if (p_cur_raid_cmd != NULL)
                {
                    p_char = field_buffer(pp_field[i], 0);
                    p_array_record = (struct ipr_array_record *)p_cur_raid_cmd->p_ipr_device->p_qac_entry;

                    if (strcmp(p_char, "1") == 0)
                    {
                        if (p_array_record->common.record_id == IPR_RECORD_ID_ARRAY2_RECORD)
                        {
                            rc = RC_SUCCESS;
                            num_vsets = 0;
                            while((RC_REFRESH == rc) ||
                                  (RC_SUCCESS == rc))
                            {
                                clear();
                                rc = configure_raid_start(p_cur_raid_cmd, num_vsets);

                                if (RC_SUCCESS == rc)
                                {
                                    num_vsets++;
                                    found = 1;
                                    p_cur_raid_cmd->do_cmd = 1;
                                    p_array_record->issue_cmd = 1;
                                    p_cur_raid_cmd->p_ipr_ioa->num_raid_cmds++;

                                    p_tail_raid_cmd->p_next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                                    p_tail_raid_cmd = p_tail_raid_cmd->p_next;
                                    p_tail_raid_cmd->p_next = NULL;

                                    memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

                                    array_id =  p_array_record->array_id;

                                    p_tail_raid_cmd->array_id = array_id;
                                    p_tail_raid_cmd->array_num = array_num;
                                    p_tail_raid_cmd->p_ipr_ioa = p_cur_raid_cmd->p_ipr_ioa;
                                    p_tail_raid_cmd->p_ipr_device = p_cur_raid_cmd->p_ipr_device;

                                    p_cur_raid_cmd = p_tail_raid_cmd;
                                    array_num++;
                                }
                            }
                            if (rc == RC_EXIT)
                                goto leave;
                            if (rc == RC_CANCEL)
                            {
                                rc = RC_REFRESH;
                                goto leave;
                            }
                        }
                        else
                        {
                            found = 1;
                            p_cur_raid_cmd->do_cmd = 1;
                            p_array_record->issue_cmd = 1;
                            p_cur_raid_cmd->p_ipr_ioa->num_raid_cmds++;
                        }
                    }
                    else
                    {
                        p_cur_raid_cmd->do_cmd = 0;
                    }
                }
            }

            if (found != 0)
            {
                /* Go to verification screen */
                unpost_form(p_form);
                rc = confirm_raid_start();
                goto leave;
            }
            else
	    {
		rc = RC_REFRESH;
		goto leave;
            }     
        }
        else
        {
            /* Terminal is running under 5250 telnet */
            if (term_is_5250)
            {
                /* Send it to the form to be processed and flush this line */
                form_driver(p_list_form, ch);
                last_char = flush_line();
                if (IS_5250_CHAR(last_char))
                    ungetch(ENTER_KEY);
                else if (last_char != ERR)
                    ungetch(last_char);
            }
            else
            {
                form_driver(p_list_form, ch);
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(pp_field);

    p_cur_raid_cmd = p_head_raid_cmd;
    while(p_cur_raid_cmd)
    {
	if (p_cur_raid_cmd->p_qac_data)
	{
	    free(p_cur_raid_cmd->p_qac_data);
	    p_cur_raid_cmd->p_qac_data = NULL;
	}
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }

    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int configure_raid_start(struct array_cmd_data *p_cur_raid_cmd,
                         int num_vsets)
{
/*


                    Select Disk Units for Parity Protection

Type option, press Enter.
  1=Select

       Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
OPT     ID        ID              Number    Bus    Dev    Bus   ID   Lun
       IBM      DFHSS4W          0024A0B9    05     03      0     3    0
       IBM      DFHSS4W          00D38D0F    05     03      0     5    0
       IBM      DFHSS4W          00D32240    05     03      0     9    0
       IBM      DFHSS4W          00034563    05     03      0     6    0
       IBM      DFHSS4W          00D37A11    05     03      0     4    0

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

                    Select Disk Units for Parity Protection

Type option, press Enter.
  1=Select

              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI
OPT    Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun
       6607  0024A0B9    03      D01       05     03      0     3    0
       6607  00D38D0F    03      D02       05     03      0     5    0
       6607  00D32240    03      D03       05     03      0     9    0
       6607  00034563    03      D04       05     03      0     6    0
       6717  00D37A11    03      D05       05     03      0     4    0

*/

    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    int rc, i, j, init_index, found, last_char;
    char buffer[100];
    int max_size = curses_info.max_y - 10;
    int num_devs = 0;
    struct ipr_ioa *p_cur_ioa;
    char *p_char;
    u32 len;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int field_index = 0;
    int start_y;
    struct ipr_device *p_ipr_device;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_device_record *p_device_record;
    struct ipr_array_query_data *p_qac_data_tmp;
    int configure_raid_parameters(struct array_cmd_data *p_cur_raid_cmd);
    int continue_raid_start(struct ipr_ioa *p_cur_ioa);

    rc = RC_SUCCESS;

    p_title_win = newwin(7, curses_info.max_x,
                         0, 0);

    p_menu_win = newwin(3, curses_info.max_x,
                        max_size + 7, 0);

    p_field_win = newwin(max_size, curses_info.max_x, 7, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,  /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],    /* field to alter */
                     0,          /* number of buffer to alter */
                     "Select Disk Units for Parity Protection");

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

    for (j = 0; j < p_cur_ioa->num_devices; j++)
    {
        p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
        p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;

        if (p_device_record == NULL)
            continue;

        if (p_cur_ioa->dev[j].is_start_cand)
            continue;

        if (!p_resource_entry->is_ioa_resource &&
            (p_device_record->common.record_id == IPR_RECORD_ID_DEVICE2_RECORD) &&
            (p_device_record->start_cand))
        {
            len = print_dev_sel(p_resource_entry, buffer);
            start_y = num_devs % (max_size - 1);

            /* User entry field */
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, 1, start_y, 1, 0, 0);

            set_field_userptr(pp_field[field_index++], (char *)&p_cur_ioa->dev[j]);

            /* Description field */
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, curses_info.max_x - 7,   /* new field size */ 
                                              start_y, 7, 0, 0);

            set_field_buffer(pp_field[field_index], 0, buffer);

            field_opts_off(pp_field[field_index], /* field to alter */
                           O_ACTIVE);             /* attributes to turn off */ 

            set_field_userptr(pp_field[field_index++], NULL);

            if ((num_devs > 0) && ((num_devs % (max_size - 2)) == 0))
            {
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, curses_info.max_x,
                                                  start_y + 1,
                                                  0,
                                                  0, 0);

                set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                set_field_buffer(pp_field[field_index], 0, "More... ");
                set_field_userptr(pp_field[field_index], NULL);
                field_opts_off(pp_field[field_index++],
                               O_ACTIVE);
            }

            memset(buffer, 0, 100);

            len = 0;
            num_devs++;
        }
    }

    if (num_devs)
    {
        if (num_vsets > 0)
        {
            rc = continue_raid_start(p_cur_ioa);
            if (rc != RC_SUCCESS)
            {
                free_field(p_fields[0]);
                return rc;
            }
        }

        mvwaddstr(p_title_win, 2, 0, "Type option, press Enter.");
        mvwaddstr(p_title_win, 3, 0, "  1=Select");

        if (platform == IPR_ISERIES)
        {
            mvwaddstr(p_title_win, 5, 0,
                      "              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI");
            mvwaddstr(p_title_win, 6, 0,
                      "OPT    Type   Number     ID    Position   Bus    Dev    Bus    ID   Lun");
        }
        else
        {
            mvwaddstr(p_title_win, 5, 0,
                      "       Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
            mvwaddstr(p_title_win, 6, 0,
                      "OPT     ID         ID              Number    Bus    Dev    Bus    ID   Lun");
        }

        mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

        p_qac_data_tmp = malloc(sizeof(struct ipr_array_query_data));
        memcpy(p_qac_data_tmp,
               p_cur_ioa->p_qac_data,
               sizeof(struct ipr_array_query_data));

        field_opts_off(p_fields[1], O_ACTIVE);

        init_index = ((max_size - 1) * 2) + 1;
        for (i = init_index; i < field_index; i += init_index)
            set_new_page(pp_field[i], TRUE);

        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = NULL;

        p_list_form = new_form(pp_field);

        set_form_win(p_list_form, p_field_win);

        post_form(p_list_form);

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);

        form_driver(p_list_form,
                    REQ_FIRST_FIELD);
        wnoutrefresh(p_field_win);
        doupdate();

        while (1)
        {
            wnoutrefresh(stdscr);
            wnoutrefresh(p_menu_win);
            wnoutrefresh(p_title_win);
            wnoutrefresh(p_field_win);
            doupdate();

            for (;;)
            {
                int ch = getch();

                if (IS_EXIT_KEY(ch))
                {
                    rc = RC_EXIT;
                    goto leave;
                }
                else if (IS_CANCEL_KEY(ch))
                {
                    rc = RC_NOOP;
                    goto leave;
                }
                else if (ch == KEY_DOWN)
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else if (ch == KEY_UP)
                {
                    form_driver(p_list_form,
                                REQ_PREV_FIELD);
                    break;
                }
                else if (IS_ENTER_KEY(ch))
                {
                    found = 0;

                    mvwaddstr(p_menu_win, 2, 1,
                              "                                           ");
                    wrefresh(p_menu_win);

                    for (i = 0;
                         i < field_index; i++)
                    {
                        p_ipr_device =
                            (struct ipr_device *)field_userptr(pp_field[i]);

                        if (p_ipr_device != NULL)
                        {
                            p_char = field_buffer(pp_field[i], 0);
                            if (strcmp(p_char, "1") == 0)
                            {
                                found = 1;
                                p_device_record = (struct ipr_device_record *)p_ipr_device->p_qac_entry;
                                p_device_record->issue_cmd = 1;
                                p_ipr_device->is_start_cand = 1;
                            }
                        }
                    }

                    if (found != 0)
                    {
                        /* Go to parameters screen */
                        unpost_form(p_list_form);
                        unpost_form(p_form);
                        rc = configure_raid_parameters(p_cur_raid_cmd);
                        if (rc == RC_EXIT)
                            goto leave;
                        if (rc == RC_CANCEL)
                        {
                            /* clear out current selections */
                            for (i = 0; i < field_index; i++)
                            {
                                p_ipr_device = (struct ipr_device *)field_userptr(pp_field[i]);
                                if (p_ipr_device != NULL)
                                {
                                    p_char = field_buffer(pp_field[i], 0);
                                    if (strcmp(p_char, "1") == 0)
                                    {
                                        p_device_record = (struct ipr_device_record *)p_ipr_device->p_qac_entry;
                                        p_device_record->issue_cmd = 0;
                                        p_ipr_device->is_start_cand = 0;
                                    }
                                }
                            }
                            rc = RC_REFRESH;
                            goto leave;
                        }
                        p_cur_raid_cmd->p_qac_data =
                            malloc(sizeof(struct ipr_array_query_data));

                        /* save off qac data image with issue_cmd set
                         for selected devices */
                        memcpy(p_cur_raid_cmd->p_qac_data,
                               p_cur_ioa->p_qac_data,
                               sizeof(struct ipr_array_query_data));
                        goto leave;
                    }
                    else
                    {
                        break;
                    }
                }
                else if (IS_PGDN_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_NEXT_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (IS_PGUP_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_PREV_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (ch == '\t')
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else
                {
                    /* Terminal is running under 5250 telnet */
                    if (term_is_5250)
                    {
                        /* Send it to the form to be processed and flush this line */
                        form_driver(p_list_form, ch);
                        last_char = flush_line();
                        if (IS_5250_CHAR(last_char))
                            ungetch(ENTER_KEY);
                        else if (last_char != ERR)
                            ungetch(last_char);
                    }
                    else
                    {
                        form_driver(p_list_form, ch);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        /* No devices to initialize */
        free_field(p_fields[0]);
        return RC_NOOP;
    }
    leave:

        /* restore unmodified image of qac data */
        memcpy(p_cur_ioa->p_qac_data,
               p_qac_data_tmp,
               sizeof(struct ipr_array_query_data));
    free(p_qac_data_tmp);
    unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;

    free_screen(NULL, NULL, p_fields);
    free_screen(NULL, NULL, pp_field);
    free(pp_field);
    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    flush_stdscr();
    return rc;
}

int configure_raid_parameters(struct array_cmd_data *p_cur_raid_cmd)
{
    /*
                    Select Protection Level and Stripe Size

       Default array configurations are shown.  To change
       setting hit "c" for options menu.  Highlight desired
       and option and hit Enter.

       c=Change Setting


     Protection Level . . . . . . . . . . . . :  RAID 0
     Stripe Size  . . . . . . . . . . . . . . :  64  KB






     
     Press Enter to Continue.
 
     e=Exit   q=Cancel
     */
    FIELD *p_input_fields[4];
    FIELD *p_cur_field;
    FORM *p_form, *p_old_form;
    ITEM **raid_item = NULL;
    int raid_index_default;
    unsigned long raid_index = 0;
    unsigned long index;
    char buffer[24];
    int rc, i;
    struct ipr_ioa *p_cur_ioa;
    struct ipr_supported_arrays *p_supported_arrays;
    struct ipr_array_cap_entry *p_cur_array_cap_entry;
    struct ipr_device_record *p_device_record;
    struct text_str
    {
        char line[16];
    } *raid_menu_str = NULL, *stripe_menu_str = NULL;
    char raid_str[16], stripe_str[16];
    int ch, start_row;
    int cur_field_index;
    int selected_count = 0;
    int stripe_sz, stripe_sz_mask, stripe_sz_list[16];
    struct prot_level *prot_level_list;
    int *userptr = NULL;
    int *retptr;

    rc = RC_SUCCESS;

    /* Title */
    p_input_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                                  0, 0,       /* upper left corner */
                                  0,           /* number of offscreen rows */
                                  0);               /* number of working buffers */

    /* Protection Level */
    p_input_fields[1] = new_field(1, 9,   /* new field size */ 
                                  8, 44,       /*  */
                                  0,           /* number of offscreen rows */
                                  0);          /* number of working buffers */

    /* Stripe Size */
    p_input_fields[2] = new_field(1, 9,   /* new field size */ 
                                  9, 44,       /*  */
                                  0,           /* number of offscreen rows */
                                  0);          /* number of working buffers */

    p_input_fields[3] = NULL;


    p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
    p_supported_arrays = p_cur_ioa->p_supported_arrays;
    p_cur_array_cap_entry = (struct ipr_array_cap_entry *)p_supported_arrays->data;

    /* determine number of devices selected for this parity set */
    for (i=0; i<p_cur_ioa->num_devices; i++)
    {
        p_device_record = (struct ipr_device_record *)
            p_cur_ioa->dev[i].p_qac_entry;

        if ((p_device_record != NULL) &&
            (p_device_record->common.record_id == IPR_RECORD_ID_DEVICE2_RECORD) &&
            (p_device_record->issue_cmd))
        {
            selected_count++;
        }
    }

    /* set up raid lists */
    raid_index = 0;
    raid_index_default = 0;
    prot_level_list = malloc(sizeof(struct prot_level));
    prot_level_list[raid_index].is_valid_entry = 0;

    for (i=0; i<iprtoh16(p_supported_arrays->num_entries); i++)
    {
        if ((selected_count <= p_cur_array_cap_entry->max_num_array_devices) &&
            (selected_count >= p_cur_array_cap_entry->min_num_array_devices) &&
            ((selected_count % p_cur_array_cap_entry->min_mult_array_devices) == 0))
        {
            prot_level_list[raid_index].p_array_cap_entry = p_cur_array_cap_entry;
            prot_level_list[raid_index].is_valid_entry = 1;
            if (0 == strcmp(p_cur_array_cap_entry->prot_level_str,IPR_DEFAULT_RAID_LVL))
            {
                raid_index_default = raid_index;
            }
            raid_index++;
            prot_level_list = realloc(prot_level_list, sizeof(struct prot_level) * (raid_index + 1));
            prot_level_list[raid_index].is_valid_entry = 0;
        }
        p_cur_array_cap_entry = (struct ipr_array_cap_entry *)
            ((void *)p_cur_array_cap_entry + iprtoh16(p_supported_arrays->entry_length));
    }

    raid_index = raid_index_default;
    p_cur_array_cap_entry = prot_level_list[raid_index].p_array_cap_entry;
    stripe_sz = iprtoh16(p_cur_array_cap_entry->recommended_stripe_size);

    p_old_form = curses_info.p_form;

    curses_info.p_form = p_form = new_form(p_input_fields);

    set_current_field(p_form, p_input_fields[1]);

    set_field_just(p_input_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_input_fields[0], O_ACTIVE);
    field_opts_off(p_input_fields[1], O_EDIT);
    field_opts_off(p_input_fields[2], O_EDIT);

    set_field_buffer(p_input_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Select Protection Level and Stripe Size");  /* string value to set     */

    form_opts_off(p_form, O_BS_OVERLOAD);

    flush_stdscr();

    clear();

    post_form(p_form);

    mvaddstr(2, 1, "Default array configurations are shown.  To change");
    mvaddstr(3, 1, "setting hit \"c\" for options menu.  Highlight desired");
    mvaddstr(4, 1, "option then hit Enter");
    mvaddstr(6, 1, "c=Change Setting");
    mvaddstr(8, 0, "Protection Level . . . . . . . . . . . . :");
    mvaddstr(9, 0, "Stripe Size  . . . . . . . . . . . . . . :");
    mvaddstr(curses_info.max_y - 3, 1, "Press Enter to Continue");
    mvaddstr(curses_info.max_y - 1, 1, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

    form_driver(p_form,               /* form to pass input to */
                REQ_FIRST_FIELD );     /* form request code */

    while (1)
    {

        sprintf(raid_str,"RAID %s",p_cur_array_cap_entry->prot_level_str);
        set_field_buffer(p_input_fields[1],        /* field to alter */
                         0,                       /* number of buffer to alter */
                         raid_str);              /* string value to set */

        if (stripe_sz > 1024)
            sprintf(stripe_str,"%d M",stripe_sz/1024);
        else
            sprintf(stripe_str,"%d k",stripe_sz);

        set_field_buffer(p_input_fields[2],     /* field to alter */
                         0,                    /* number of buffer to alter */
                         stripe_str);          /* string value to set */

        refresh();
        ch = getch();

        if (IS_EXIT_KEY(ch))
        {
            rc = RC_EXIT;
            goto leave;
        }
        else if (IS_CANCEL_KEY(ch))
        {
            rc = RC_CANCEL;
            goto leave;
        }
        else if (ch == 'c')
        {
            p_cur_field = current_field(p_form);
            cur_field_index = field_index(p_cur_field);

            if (cur_field_index == 1)
            {
                /* count the number of valid entries */
                index = 0;
                while (prot_level_list[index].is_valid_entry)
                {
                    index++;
                }

                /* get appropriate memory, the text portion needs to be
                   done up front as the new_item() function uses the
                   passed pointer to display data */
                raid_item = realloc(raid_item, sizeof(ITEM **) * (index + 1));
                raid_menu_str = realloc(raid_menu_str, sizeof(struct text_str) * (index));
                userptr = realloc(userptr, sizeof(int) * (index + 1));

                /* set up protection level menu */
                index = 0;
                while (prot_level_list[index].is_valid_entry)
                {
                    raid_item[index] = (ITEM *)NULL;
                    sprintf(raid_menu_str[index].line,"RAID %s",prot_level_list[index].p_array_cap_entry->prot_level_str);
                    raid_item[index] = new_item(raid_menu_str[index].line,"");
                    userptr[index] = index;
                    set_item_userptr(raid_item[index],
                                     (char *)&userptr[index]);
                    index++;
                }
                raid_item[index] = (ITEM *)NULL;
                start_row = 8;
                rc = display_menu(raid_item, start_row, index, &retptr);
                if (rc == RC_SUCCESS)
                {
                    raid_index = *retptr;

                    /* find recommended stripe size */
                    p_cur_array_cap_entry = prot_level_list[raid_index].p_array_cap_entry;
                    stripe_sz = iprtoh16(p_cur_array_cap_entry->recommended_stripe_size);
                }
            }
            else if (cur_field_index == 2)
            {
                /* count the number of valid entries */
                index = 0;
                for (i=0; i<16; i++)
                {
                    stripe_sz_mask = 1 << i;
                    if ((stripe_sz_mask & iprtoh16(p_cur_array_cap_entry->supported_stripe_sizes))
                        == stripe_sz_mask)
                    {
                        index++;
                    }
                }

                /* get appropriate memory, the text portion needs to be
                   done up front as the new_item() function uses the
                   passed pointer to display data */
                raid_item = realloc(raid_item, sizeof(ITEM **) * (index + 1));
                stripe_menu_str = realloc(stripe_menu_str, sizeof(struct text_str) * (index));
                userptr = realloc(userptr, sizeof(int) * (index + 1));

                /* set up stripe size menu */
                index = 0;
                for (i=0; i<16; i++)
                {
                    stripe_sz_mask = 1 << i;
                    raid_item[index] = (ITEM *)NULL;
                    if ((stripe_sz_mask & iprtoh16(p_cur_array_cap_entry->supported_stripe_sizes))
                        == stripe_sz_mask)
                    {
                        stripe_sz_list[index] = stripe_sz_mask;
                        if (stripe_sz_mask > 1024)
                            sprintf(stripe_menu_str[index].line,"%d M",stripe_sz_mask/1024);
                        else
                            sprintf(stripe_menu_str[index].line,"%d k",stripe_sz_mask);

                        if (stripe_sz_mask == iprtoh16(p_cur_array_cap_entry->recommended_stripe_size))
                        {
                            sprintf(buffer,"%s - recommend",stripe_menu_str[index].line);
                            raid_item[index] = new_item(buffer, "");
                        }
                        else
                        {
                            raid_item[index] = new_item(stripe_menu_str[index].line, "");
                        }
                        userptr[index] = index;
                        set_item_userptr(raid_item[index],
                                         (char *)&userptr[index]);
                        index++;
                    }
                }
                raid_item[index] = (ITEM *)NULL;
                start_row = 9;
                rc = display_menu(raid_item, start_row, index, &retptr);
                if (rc == RC_SUCCESS)
                {
                    stripe_sz = stripe_sz_list[*retptr];
                }
            }
            else
                continue;

            i=0;
            while (raid_item[i] != NULL)
                free_item(raid_item[i++]);
            realloc(raid_item, 0);
            raid_item = NULL;

            if (rc == RC_EXIT)
                goto leave;
        }
        else if (IS_ENTER_KEY(ch))
        {
            /* set protection level and stripe size appropriately */
            p_cur_raid_cmd->prot_level = p_cur_array_cap_entry->prot_level;
            p_cur_raid_cmd->stripe_size = stripe_sz;
            goto leave;
        }
        else if (ch == KEY_RIGHT)
            form_driver(p_form, REQ_NEXT_CHAR);
        else if (ch == KEY_LEFT)
            form_driver(p_form, REQ_PREV_CHAR);
        else if ((ch == KEY_BACKSPACE) || (ch == 127))
            form_driver(p_form, REQ_DEL_PREV);
        else if (ch == KEY_DC)
            form_driver(p_form, REQ_DEL_CHAR);
        else if (ch == KEY_IC)
            form_driver(p_form, REQ_INS_MODE);
        else if (ch == KEY_EIC)
            form_driver(p_form, REQ_OVL_MODE);
        else if (ch == KEY_HOME)
            form_driver(p_form, REQ_BEG_FIELD);
        else if (ch == KEY_END)
            form_driver(p_form, REQ_END_FIELD);
        else if (ch == '\t')
            form_driver(p_form, REQ_NEXT_FIELD);
        else if (ch == KEY_UP)
            form_driver(p_form, REQ_PREV_FIELD);
        else if (ch == KEY_DOWN)
            form_driver(p_form, REQ_NEXT_FIELD);
        else if (isascii(ch))
            form_driver(p_form, ch);
    }
    leave:

        free_form(p_form);
    free_screen(NULL, NULL, p_input_fields);
    curses_info.p_form = p_old_form;

    flush_stdscr();
    return rc;
}

int continue_raid_start(struct ipr_ioa *p_cur_ioa)
{
#define CONT_START_HDR_SIZE	10
#define CONT_START_MENU_SIZE	3
/* iSeries
                Additional Devices For Parity Protection

Choose to Add more parity sets to this resource or to
Continue with the current configurations

Press a=Add more parity sets to this resource
Press c=Continue

 Parity  Serial                Resource
   Set   Number   Type  Model  Name
    1   02127005  5703   001   /dev/ipr1
    1   000BBBC1  6717   050



a=Add     c=Continue
q=Cancel    f=PageDn   b=PageUp

*/
/* non-iSeries
                Additional Devices For Parity Protection

Choose to Add more parity sets to this resource or to
Continue with the current configurations

Press a=Add more parity sets to this resource
Press c=Continue

 Parity   Vendor    Product           Serial          Resource
   Set     ID         ID              Number   Model  Name   
    1     IBM       DAB61C0E         3AFB348C   200   /dev/sdb
    1     IBMAS400  DCHS09W          0024A0B9   070
    1     IBMAS400  DFHSS4W          00D37A11   070


a=Add     c=Continue
q=Cancel    f=PageDn   b=PageUp

*/

    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    struct ipr_resource_entry *p_resource_entry;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_resource_flags *p_resource_flags;
    int rc, j;
    u16 len = 0;
    char buffer[100];
    u32 num_lines = 0;
    int max_size = curses_info.max_y - (CONT_START_HDR_SIZE + CONT_START_MENU_SIZE + 1);
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    PANEL *p_panel;
    struct ipr_array_record *p_array_record;
    struct ipr_device_record *p_device_record;
    struct ipr_array_query_data *p_qac_data_tmp;
    struct ipr_device *p_ipr_device;

    p_title_win = newwin(CONT_START_HDR_SIZE,
                         curses_info.max_x,
                         curses_info.start_y,
                         curses_info.start_x);

    p_menu_win = newwin(CONT_START_MENU_SIZE,
                        curses_info.max_x,
                        curses_info.start_y + max_size + CONT_START_HDR_SIZE + 1,
                        curses_info.start_x);

    rc = RC_SUCCESS;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Additional Devices For Parity Protection");

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + CONT_START_HDR_SIZE,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    mvwaddstr(p_title_win, 2, 0, "Choose to Add more parity sets to this resource or to");
    mvwaddstr(p_title_win, 3, 0, "Continue with the current configurations");
    mvwaddstr(p_title_win, 5, 0, "Press a=Add more parity sets to this resource");
    mvwaddstr(p_title_win, 6, 0, "Press c=Continue");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 8, 0, " Parity  Serial                Resource");
        mvwaddstr(p_title_win, 9, 0, "   Set   Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 8, 0, " Parity   Vendor    Product           Serial          Resource");
        mvwaddstr(p_title_win, 9, 0, "   Set     ID         ID              Number   Model  Name");
    }
    mvwaddstr(p_menu_win, 0, 0, "a=Add       c=Continue");
    mvwaddstr(p_menu_win, 1, 0, CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_raid_cmd = p_head_raid_cmd;

    p_qac_data_tmp = malloc(sizeof(struct ipr_array_query_data));
    memcpy(p_qac_data_tmp,
	   p_cur_ioa->p_qac_data,
	   sizeof(struct ipr_array_query_data));

    while(p_cur_raid_cmd)
    {
        if ((p_cur_raid_cmd->do_cmd) &&
            (p_cur_ioa == p_cur_raid_cmd->p_ipr_ioa))
        {
            p_ipr_device = p_cur_raid_cmd->p_ipr_device;

            p_array_record = (struct ipr_array_record *)p_ipr_device->p_qac_entry;

            /* swap in this raid cmd's image of qac data */
            memcpy(p_cur_ioa->p_qac_data,
                   p_cur_raid_cmd->p_qac_data,
                   sizeof(struct ipr_array_query_data));

            if (platform == IPR_ISERIES)
            {
                /* Print out "array entry" */
                len  = sprintf(buffer, " %4d   %8s  %x   001   %s",
                               p_cur_raid_cmd->array_num, p_cur_ioa->serial_num,
                               p_cur_ioa->ccin, p_cur_ioa->ioa.dev_name);
            }
            else
            {
                char buf[100];

                /* Print out "array entry" */
                len = sprintf(buffer, " %4d     ", p_cur_raid_cmd->array_num);

                memcpy(buf, p_ipr_device->p_resource_entry->std_inq_data.vpids.vendor_id,
                       IPR_VENDOR_ID_LEN);
                buf[IPR_VENDOR_ID_LEN] = '\0';

                len += sprintf(buffer + len, "%-8s  ", buf);

                memcpy(buf, p_ipr_device->p_resource_entry->std_inq_data.vpids.product_id,
                       IPR_PROD_ID_LEN);
                buf[IPR_PROD_ID_LEN] = '\0';

                len += sprintf(buffer + len, "%-12s ", buf);

                len += sprintf(buffer + len, "%8s   %03d   %s",
                               p_ipr_device->p_resource_entry->serial_num,
                               p_ipr_device->p_resource_entry->model,
                               p_ipr_device->dev_name);
            }

            if ((num_lines > 0) && ((num_lines % max_size) == 0))
            {
                mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                p_tail_win = p_tail_win->p_next;

                p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                p_tail_panel = p_tail_panel->p_next;

                p_tail_win->p_win = newwin(max_size + 1,
                                           curses_info.max_x,
                                           curses_info.start_y + CONT_START_HDR_SIZE,
                                           curses_info.start_x);
                p_tail_win->p_next = NULL;

                p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                p_tail_panel->p_next = NULL;
            }

            mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
            memset(buffer, 0, 100);
            num_lines++;

            for (j = 0; j < p_cur_ioa->num_devices; j++)
            {
                p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
                p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;

                if ((p_device_record != NULL) &&
                    (p_device_record->common.record_id == IPR_RECORD_ID_DEVICE2_RECORD) &&
                    (p_device_record->issue_cmd))
                {
                    p_resource_flags = &p_device_record->resource_flags_to_become;
                    len = sprintf(buffer, " %4d   ", p_cur_raid_cmd->array_num);

                    if (platform == IPR_ISERIES)
                    {
                        print_dev2(p_resource_entry, buffer + len, p_resource_flags, p_cur_ioa->dev[j].dev_name);
                    }
                    else
                    {
                        char buf[100];

                        memcpy(buf, p_cur_ioa->dev[j].p_resource_entry->std_inq_data.vpids.vendor_id,
                               IPR_VENDOR_ID_LEN);
                        buf[IPR_VENDOR_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "  %-8s  ", buf);

                        memcpy(buf, p_cur_ioa->dev[j].p_resource_entry->std_inq_data.vpids.product_id,
                               IPR_PROD_ID_LEN);
                        buf[IPR_PROD_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "%-12s ", buf);

                        len += sprintf(buffer + len, "%8s   %03d   %s",
                                       p_resource_entry->serial_num,
                                       p_resource_entry->model,
                                       p_cur_ioa->dev[j].dev_name);
                    }

                    if ((num_lines > 0) && ((num_lines % max_size) == 0))
                    {
                        mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                        p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                        p_tail_win = p_tail_win->p_next;

                        p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                        p_tail_panel = p_tail_panel->p_next;

                        p_tail_win->p_win = newwin(max_size + 1,
                                                   curses_info.max_x,
                                                   curses_info.start_y + CONT_START_HDR_SIZE,
                                                   curses_info.start_x);
                        p_tail_win->p_next = NULL;

                        p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                        p_tail_panel->p_next = NULL;
                    }

                    mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
                    memset(buffer, 0, 100);
                    num_lines++;
                }
            }
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        wnoutrefresh(stdscr);
        update_panels();
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if (IS_ENTER_KEY(ch))
	    {
		break;
	    }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
	    else if ((ch == 'a') ||
		     (ch == 'A'))
	    {
		rc = RC_SUCCESS;
		goto leave;
	    }
	    else if ((ch == 'c') ||
		     (ch == 'C'))
	    {
		rc = RC_NOOP;
		goto leave;
	    }
            else
            {
                break;
            }
        }
    }

    leave:

    /* restore unmodified image of qac data */
    memcpy(p_cur_ioa->p_qac_data,
	   p_qac_data_tmp,
	   sizeof(struct ipr_array_query_data));
    free(p_qac_data_tmp);

    unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}


int confirm_raid_start()
{
#define CONFIRM_START_HDR_SIZE		10
#define CONFIRM_START_MENU_SIZE	3
/* iSeries
                     Confirm Start Device Parity Protection

ATTENTION: Data will not be preserved and may be lost on selected disk
units when parity protection is enabled.              

Press Enter to continue.
Press q=Cancel to return and change your choice.

         Parity  Serial                Resource                 
Option     Set   Number   Type  Model  Name                     
    1       1   02127005  5703   001   /dev/ipr1
    1       1   0024B45C  6713   050
    1       1   002443C6  6713   050
    1       1   00C02D9E  6719   050
    1       1   0004C93D  6607   050

q=Cancel    f=PageDn   b=PageUp
*/
/* non-iSeries
                     Confirm Start Device Parity Protection

ATTENTION: Data will not be preserved and may be lost on selected disk
units when parity protection is enabled.              

Press Enter to continue.
Press q=Cancel to return and change your choice.

         Parity   Vendor    Product           Serial          Resource
Option     Set     ID         ID              Number   Model  Name   
    1       1     IBM       DAB61C0E         3AFB348C   200   /dev/sdb
    1       1     IBMAS400  DCHS09W          0024A0B9   070
    1       1     IBMAS400  DFHSS4W          00D37A11   070

q=Cancel    f=PageDn   b=PageUp
*/
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    struct ipr_resource_entry *p_resource_entry;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_resource_flags *p_resource_flags;
    struct ipr_ioa *p_cur_ioa;
    int rc, i, j, fd;
    u16 len = 0;
    char buffer[100];
    u32 num_lines = 0;
    int max_size = curses_info.max_y - (CONFIRM_START_HDR_SIZE + CONFIRM_START_MENU_SIZE + 1);
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    PANEL *p_panel;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_array_record *p_array_record;
    struct ipr_record_common *p_common_record_saved;
    struct ipr_record_common *p_common_record;
    struct ipr_device_record *p_device_record_saved;
    struct ipr_device_record *p_device_record;
    struct ipr_device *p_ipr_device;
    int found;
    int raid_start_complete();

    p_title_win = newwin(CONFIRM_START_HDR_SIZE,
                         curses_info.max_x,
                         curses_info.start_y,
                         curses_info.start_x);

    p_menu_win = newwin(CONFIRM_START_MENU_SIZE,
                        curses_info.max_x,
                        curses_info.start_y + max_size + CONFIRM_START_HDR_SIZE + 1,
                        curses_info.start_x);

    rc = RC_SUCCESS;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Confirm Start Device Parity Protection");

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + CONFIRM_START_HDR_SIZE,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    wattron(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 0, "ATTENTION:");
    wattroff(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 10, " All listed device units will be configured for parity");
    mvwaddstr(p_title_win, 3, 0, "protection, existing data will not be preserved.");
    mvwaddstr(p_title_win, 5, 0, "Press Enter to continue.");
    mvwaddstr(p_title_win, 6, 0, "Press "CANCEL_KEY_LABEL"=Cancel to return and change your choice.");

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 8, 0, "         Parity  Serial                Resource");
        mvwaddstr(p_title_win, 9, 0, "Option     Set   Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 8, 0, "         Parity   Vendor    Product           Serial          Resource");
        mvwaddstr(p_title_win, 9, 0, "Option     Set     ID         ID              Number   Model  Name");
    }

    mvwaddstr(p_menu_win, 1, 0, CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_raid_cmd = p_head_raid_cmd;

    while(p_cur_raid_cmd)
    {
        if (p_cur_raid_cmd->do_cmd)
        {
            p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

            p_ipr_device = p_cur_raid_cmd->p_ipr_device;

            p_array_record = (struct ipr_array_record *)p_ipr_device->p_qac_entry;

            if (p_array_record->common.record_id == IPR_RECORD_ID_ARRAY2_RECORD)
            {
                /* swap in this raid cmd's image of qac data */
                memcpy(p_cur_ioa->p_qac_data,
                       p_cur_raid_cmd->p_qac_data,
                       sizeof(struct ipr_array_query_data));
            }

            if (platform == IPR_ISERIES)
            {
                /* Print out "array entry" */
                len  = sprintf(buffer, "    1    %4d   %8s  %x   001   %s",
                               p_cur_raid_cmd->array_num, p_cur_ioa->serial_num,
                               p_cur_ioa->ccin, p_cur_ioa->ioa.dev_name);
            }
            else
            {
                char buf[100];

                /* Print out "array entry" */
                len = sprintf(buffer, "    1    %4d     ", p_cur_raid_cmd->array_num);

                memcpy(buf, p_ipr_device->p_resource_entry->std_inq_data.vpids.vendor_id,
                       IPR_VENDOR_ID_LEN);
                buf[IPR_VENDOR_ID_LEN] = '\0';

                len += sprintf(buffer + len, "%-8s  ", buf);

                memcpy(buf, p_ipr_device->p_resource_entry->std_inq_data.vpids.product_id,
                       IPR_PROD_ID_LEN);
                buf[IPR_PROD_ID_LEN] = '\0';

                len += sprintf(buffer + len, "%-12s ", buf);

                len += sprintf(buffer + len, "%8s   %03d   %s",
                               p_ipr_device->p_resource_entry->serial_num,
                               p_ipr_device->p_resource_entry->model,
                               p_ipr_device->dev_name);
            }

            if ((num_lines > 0) && ((num_lines % max_size) == 0))
            {
                mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                p_tail_win = p_tail_win->p_next;

                p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                p_tail_panel = p_tail_panel->p_next;

                p_tail_win->p_win = newwin(max_size + 1,
                                           curses_info.max_x,
                                           curses_info.start_y + CONFIRM_START_HDR_SIZE,
                                           curses_info.start_x);
                p_tail_win->p_next = NULL;

                p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                p_tail_panel->p_next = NULL;
            }

            mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
            memset(buffer, 0, 100);
            num_lines++;

            for (j = 0; j < p_cur_ioa->num_devices; j++)
            {
                p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
                p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;

                if ((p_device_record != NULL) &&
                    (((p_device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
                      (p_device_record->array_id == p_cur_raid_cmd->array_id) &&
                      (p_device_record->start_cand)) ||
                     ((p_device_record->common.record_id == IPR_RECORD_ID_DEVICE2_RECORD) &&
                      (p_device_record->issue_cmd))))
                {
                    p_resource_flags = &p_device_record->resource_flags_to_become;
                    len = sprintf(buffer, "    1    %4d   ", p_cur_raid_cmd->array_num);

                    if (platform == IPR_ISERIES)
                    {
                        print_dev2(p_resource_entry, buffer + len,
                                   p_resource_flags, p_cur_ioa->dev[j].dev_name);
                    }
                    else
                    {
                        char buf[100];

                        memcpy(buf, p_resource_entry->std_inq_data.vpids.vendor_id,
                               IPR_VENDOR_ID_LEN);
                        buf[IPR_VENDOR_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "  %-8s  ", buf);

                        memcpy(buf, p_resource_entry->std_inq_data.vpids.product_id,
                               IPR_PROD_ID_LEN);
                        buf[IPR_PROD_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "%-12s ", buf);

                        len += sprintf(buffer + len, "%8s   %03d   %s",
                                       p_resource_entry->serial_num,
                                       p_resource_entry->model,
                                       p_cur_ioa->dev[j].dev_name);
                    }

                    if ((num_lines > 0) && ((num_lines % max_size) == 0))
                    {
                        mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                        p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                        p_tail_win = p_tail_win->p_next;

                        p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                        p_tail_panel = p_tail_panel->p_next;

                        p_tail_win->p_win = newwin(max_size + 1,
                                                   curses_info.max_x,
                                                   curses_info.start_y + CONFIRM_START_HDR_SIZE,
                                                   curses_info.start_x);
                        p_tail_win->p_next = NULL;

                        p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                        p_tail_panel->p_next = NULL;
                    }

                    mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
                    memset(buffer, 0, 100);
                    num_lines++;
                }
            }
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        wnoutrefresh(stdscr);
        update_panels();
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_REFRESH;
                goto leave;
            }
            else if (IS_ENTER_KEY(ch))
            {
                /* verify the devices are not in use */
                p_cur_raid_cmd = p_head_raid_cmd;
                while (p_cur_raid_cmd)
                {
                    if (p_cur_raid_cmd->do_cmd)
                    {
                        p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
                        rc = is_array_in_use(p_cur_ioa, p_cur_raid_cmd->array_id);
                        if (rc != 0)
                        {
                            rc = RC_FAILED;
                            syslog(LOG_ERR, "Devices may have changed state. Command cancelled,"
                                   " please issue commands again if RAID still desired %s."
                                   IPR_EOL, p_cur_ioa->ioa.dev_name);
                            goto leave;
                        }
                    }
                    p_cur_raid_cmd = p_cur_raid_cmd->p_next;
                }

                unpost_form(p_form);
                free_form(p_form);
                curses_info.p_form = p_old_form;
                free_screen(p_head_panel, p_head_win, p_fields);
                delwin(p_title_win);
                delwin(p_menu_win);
                werase(stdscr);

                /* now issue the start array command */
                for (p_cur_raid_cmd = p_head_raid_cmd;
                     p_cur_raid_cmd != NULL; 
                     p_cur_raid_cmd = p_cur_raid_cmd->p_next)
                {
                    if (p_cur_raid_cmd->do_cmd == 0)
                        continue;

                    p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

                    if (p_cur_ioa->num_raid_cmds > 0)
                    {
                        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
                        if (fd <= 1)
                        {
                            syslog(LOG_ERR,
                                   "Could not open %s. %m"IPR_EOL,
                                   p_cur_ioa->ioa.dev_name);
                            continue;
                        }

                        p_array_record = (struct ipr_array_record *)
                            p_cur_raid_cmd->p_ipr_device->p_qac_entry;

                        if (p_array_record->common.record_id ==
                            IPR_RECORD_ID_ARRAY2_RECORD)
                        {
                            /* refresh qac data for ARRAY2 */
                            memset(&ioa_cmd, 0,
                                   sizeof(struct ipr_ioctl_cmd_internal));

                            ioa_cmd.buffer = p_cur_ioa->p_qac_data;
                            ioa_cmd.buffer_len = sizeof(struct ipr_array_query_data);
                            ioa_cmd.cdb[1] = 0x80; /* Prohibit Rebuild Candidate Refresh */
                            ioa_cmd.read_not_write = 1;

                            rc = ipr_ioctl(fd, IPR_QUERY_ARRAY_CONFIG, &ioa_cmd);

                            if (rc != 0)
                            {
                                if (errno != EINVAL)
                                    syslog(LOG_ERR,"Query Array Config to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                                close(fd);
                                continue;
                            }

                            /* scan through this raid command to determine which
                             devices should be part of this parity set */
                            p_common_record_saved = (struct ipr_record_common *)
                                p_cur_raid_cmd->p_qac_data->data;
                            for (i = 0; i < p_cur_raid_cmd->p_qac_data->num_records; i++)
                            {
                                p_device_record_saved = (struct ipr_device_record *)p_common_record_saved;
                                if (p_device_record_saved->issue_cmd)
                                {
                                    p_common_record = (struct ipr_record_common *)
                                        p_cur_ioa->p_qac_data->data;
                                    found = 0;
                                    for (j = 0; j < p_cur_ioa->p_qac_data->num_records; j++)
                                    {
                                        p_device_record = (struct ipr_device_record *)p_common_record;
                                        if (p_device_record->resource_handle ==
                                            p_device_record_saved->resource_handle)
                                        {
                                            p_device_record->issue_cmd = 1;
                                            found = 1;
                                            break;
                                        }
                                        p_common_record = (struct ipr_record_common *)
                                            ((unsigned long)p_common_record +
                                             iprtoh16(p_common_record->record_len));
                                    }
                                    if (!found)
                                    {
                                        syslog(LOG_ERR,
                                               "Resource not found to start parity protection to %s"
                                               "failed"IPR_EOL, p_cur_ioa->ioa.dev_name);
                                        break;
                                    }
                                }
                                p_common_record_saved = (struct ipr_record_common *)
                                    ((unsigned long)p_common_record_saved +
                                     iprtoh16(p_common_record_saved->record_len));
                            }

                            if (!found)
                            {
                                close(fd);
                                continue;
                            }

                            p_array_record->issue_cmd = 1;
                            p_cur_raid_cmd->array_id = p_array_record->array_id;
                        }
                        else
                        {
                            if (p_cur_ioa->num_raid_cmds > 1)
                            {
                                /* need to only issue one start for this
                                 adapter, only send start for the last
                                 listed array, which contains data for
                                 all arrays.  A positive value of num_raid_cmds
                                 must be maintained for status screen */
                                p_cur_ioa->num_raid_cmds--;
                                continue;
                            }
                        }

                        memset(&ioa_cmd, 0,
                               sizeof(struct ipr_ioctl_cmd_internal));

                        ioa_cmd.buffer = p_cur_ioa->p_qac_data;
                        ioa_cmd.buffer_len = iprtoh16(p_cur_ioa->p_qac_data->resp_len);
                        ioa_cmd.cdb[4] = (u8)(p_cur_raid_cmd->stripe_size >> 8);
                        ioa_cmd.cdb[5] = (u8)(p_cur_raid_cmd->stripe_size);
                        ioa_cmd.cdb[6] = p_cur_raid_cmd->prot_level;
                        rc = ipr_ioctl(fd, IPR_START_ARRAY_PROTECTION, &ioa_cmd);

                        close(fd);
                        if (rc != 0)
                        {
                            syslog(LOG_ERR, "Start Array Protection to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                        }
                    }
                }

                rc = raid_start_complete();
                return rc;
            }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else
            {
                break;
            }
        }

    }

    leave:
        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}

int raid_start_complete()
{
    /*
     Start Device Parity Protection Status

     You selected to start device parity protection









     16% Complete











     */

    FIELD *p_fields[3];
    FORM *p_form;
    FORM *p_old_form;
    struct ipr_cmd_status cmd_status;
    struct ipr_cmd_status_record *p_record;
    int done_bad;
    int not_done = 0;
    int rc, j;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    u32 percent_cmplt = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 num_starts = 0;
    struct array_cmd_data *p_cur_raid_cmd;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            curses_info.max_y - 1, curses_info.max_x - 1, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Start Device Parity Protection Status");

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    while(1)
    {
        clear();
        post_form(p_form);

        mvaddstr(2, 0, "You selected to start device parity protection");
        mvprintw(curses_info.max_y / 2, (curses_info.max_x / 2) - 7, "%d%% Complete", percent_cmplt);

        form_driver(p_form,
                    REQ_FIRST_FIELD);
        refresh();

        percent_cmplt = 100;
        done_bad = 0;
        num_starts = 0;
        not_done = 0;

        for (p_cur_ioa = p_head_ipr_ioa;
             p_cur_ioa != NULL;
             p_cur_ioa = p_cur_ioa->p_next)
        {
            if (p_cur_ioa->num_raid_cmds > 0)
            {
                int fd = open(p_cur_ioa->ioa.dev_name, O_RDONLY);

                if (fd < 0)
                {
                    syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    p_cur_ioa->num_raid_cmds = 0;
                    done_bad = 1;
                    continue;
                }
                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                ioa_cmd.buffer = &cmd_status;
                ioa_cmd.buffer_len = sizeof(cmd_status);
                ioa_cmd.read_not_write = 1;

                rc = ipr_ioctl(fd, IPR_QUERY_COMMAND_STATUS, &ioa_cmd);

                if (rc)
                {
                    syslog(LOG_ERR, "Query Command Status to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    close(fd);
                    p_cur_ioa->num_raid_cmds = 0;
                    done_bad = 1;
                    continue;
                }

                close(fd);
                p_record = cmd_status.record;

                for (j=0; j < cmd_status.num_records; j++, p_record++)
                {
                    if (p_record->command_code == IPR_START_ARRAY_PROTECTION)
                    {
                        for (p_cur_raid_cmd = p_head_raid_cmd;
                             p_cur_raid_cmd != NULL; 
                             p_cur_raid_cmd = p_cur_raid_cmd->p_next)
                        {
                            if ((p_cur_raid_cmd->p_ipr_ioa == p_cur_ioa) &&
                                (p_cur_raid_cmd->array_id == p_record->array_id))
                            {
                                num_starts++;
                                if (p_record->status == IPR_CMD_STATUS_IN_PROGRESS)
                                {
                                    if (p_record->percent_complete < percent_cmplt)
                                        percent_cmplt = p_record->percent_complete;
                                    not_done = 1;
                                }
                                else if (p_record->status != IPR_CMD_STATUS_SUCCESSFUL)
                                {
                                    done_bad = 1;
                                    syslog(LOG_ERR, "Start parity protect to %s failed.  "
                                           "Check device configuration for proper format."IPR_EOL,
                                           p_cur_ioa->ioa.dev_name);
                                    rc = RC_FAILED;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        unpost_form(p_form);

        if (!not_done)
        {
            free_form(p_form);
            curses_info.p_form = p_old_form;
            for (j=0; j < 2; j++)
                free_field(p_fields[j]);

            if (done_bad)
            {
                flush_stdscr();
                return RC_FAILED;
            }

            check_current_config(false);

            revalidate_devs(NULL);
            flush_stdscr();
            return RC_SUCCESS;
        }
        not_done = 0;
        sleep(2);
    }
}

int raid_rebuild()
{
    /* iSeries
                     Rebuild Disk Unit Data    

     Select the units to be rebuilt

     Type choice, press Enter.
     1=Rebuild

               Parity  Serial                Resource 
     Option     Set   Number   Type  Model  Name
                 1   00058B10  6717   050   /dev/sda
                 2   000585F1  6717   050   /dev/sdj


     e=Exit   q=Cancel

     */

/* non-iSeries
                               Rebuild Disk Unit Data

Type option, press Enter.
  1=Select

       Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
OPT     ID        ID              Number    Bus    Dev    Bus   ID   Lun
       IBM      DFHSS4W          0024A0B9    05     03      0     3    0


e=Exit   q=Cancel

*/
    int rc, i, array_num, last_char;
    struct ipr_record_common *p_common_record;
    struct ipr_device_record *p_device_record;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_resource_entry *p_resource_entry;
    u8 array_id;
    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    char buffer[100];
    int array_index;
    int max_size = curses_info.max_y - 12;
    int confirm_raid_rebuild();
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len = 0;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    int found = 0;
    char *p_char;

    rc = RC_SUCCESS;

    p_title_win = newwin(9, curses_info.max_x, 0, 0);
    p_menu_win = newwin(3, curses_info.max_x, max_size + 9, 0);
    p_field_win = newwin(max_size, curses_info.max_x, 9, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],               /* field to alter */
                     0,                         /* number of buffer to alter */
                     "Rebuild Disk Unit Data"); /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    array_num = 1;

    check_current_config(true);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        p_cur_ioa->num_raid_cmds = 0;

        for (i = 0;
             i < p_cur_ioa->num_devices;
             i++)
        {
            p_common_record = p_cur_ioa->dev[i].p_qac_entry;

            if ((p_common_record != NULL) &&
                ((p_common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) ||
                (p_common_record->record_id == IPR_RECORD_ID_DEVICE2_RECORD)))
            {
                p_device_record = (struct ipr_device_record *)p_common_record;

                if (p_device_record->rebuild_cand)
                {
                    /* now find matching resource to display device data */
                    p_resource_entry = p_cur_ioa->dev[i].p_resource_entry;

                    if (p_head_raid_cmd)
                    {
                        p_tail_raid_cmd->p_next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                        p_tail_raid_cmd = p_tail_raid_cmd->p_next;
                    }
                    else
                    {
                        p_head_raid_cmd = p_tail_raid_cmd = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                    }

                    p_tail_raid_cmd->p_next = NULL;

                    memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

                    array_id =  p_device_record->array_id;

                    p_tail_raid_cmd->array_id = array_id;
                    p_tail_raid_cmd->array_num = array_num;
                    p_tail_raid_cmd->p_ipr_ioa = p_cur_ioa;
                    p_tail_raid_cmd->p_ipr_device = &p_cur_ioa->dev[i];

                    start_y = (array_num-1) % (max_size - 1);

                    if ((num_lines > 0) && (start_y == 0))
                    {
                        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                        pp_field[field_index] = new_field(1, curses_info.max_x,
                                                          max_size - 1,
                                                          0,
                                                          0, 0);

                        set_field_just(pp_field[field_index], JUSTIFY_RIGHT);

                        set_field_buffer(pp_field[field_index], 0, "More... ");

                        set_field_userptr(pp_field[field_index], NULL);

                        field_opts_off(pp_field[field_index++],
                                       O_ACTIVE);
                    }

                    /* User entry field */
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, 1, start_y, 3, 0, 0);

                    set_field_userptr(pp_field[field_index++], (char *)p_tail_raid_cmd);

                    /* Description field */
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, curses_info.max_x - 9, start_y, 9, 0, 0);

                    if (platform == IPR_ISERIES)
                    {
                        len = sprintf(buffer, "%4d   %8s  %x   %03d   %-38s",
                                      array_num,
                                      p_resource_entry->serial_num,
                                      p_resource_entry->type,
                                      p_resource_entry->model,
                                      p_cur_ioa->dev[i].dev_name);
                    }
                    else
                    {
                        len = print_dev_sel(p_resource_entry, buffer);
                    }

                    set_field_buffer(pp_field[field_index], 0, buffer);

                    field_opts_off(pp_field[field_index], O_ACTIVE);

                    set_field_userptr(pp_field[field_index++], NULL);

                    memset(buffer, 0, 100);

                    len = 0;
                    num_lines++;
                    array_num++;
                }
            }
        }
    }

    if (array_num == 1)
    {
        set_field_buffer(p_fields[0],                       /* field to alter */
                         0,                                 /* number of buffer to alter */
                         "Rebuild Disk Unit Data Failed");  /* string value to set */

        mvwaddstr(p_title_win, 2, 0, "There are no disk units eligible for the selected operation");
        mvwaddstr(p_title_win, 3, 0, "due to one or more of the following reasons:");
        mvwaddstr(p_title_win, 5, 0, "o  There are no disk units that need to be rebuilt.");
        mvwaddstr(p_title_win, 6, 0, "o  The disk unit that needs to be rebuilt is not at the right");
        mvwaddstr(p_title_win, 7, 0, "   location.  Examine the 'message log' for the exposed unit");
        mvwaddstr(p_title_win, 8, 0, "   and make sure that the replacement unit is at the correct");
        mvwaddstr(p_field_win, 0, 0, "   location");
        mvwaddstr(p_field_win, 1, 0, "o  Not all disk units attached to an IOA have reported to the");
        mvwaddstr(p_field_win, 2, 0, "   system.  Retry the operation.");
        mvwaddstr(p_field_win, 3, 0, "o  All the disk units in a parity set must be the same capacity");
        mvwaddstr(p_field_win, 4, 0, "   with a minumum number of 4 disk units and a maximum of 10 units");
        mvwaddstr(p_field_win, 5, 0, "   in the resulting parity set");
        mvwaddstr(p_field_win, 6, 0, "o  The type/model of the disk units is not supported for the");
        mvwaddstr(p_field_win, 7, 0, "   requested operation");
        mvwaddstr(p_field_win, max_size -1, 0, "Press Enter to continue.");
        mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD);     /* form request code */

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        wnoutrefresh(p_field_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
        }
    }

    mvwaddstr(p_title_win, 2, 0, "Select the units to be rebuilt");
    mvwaddstr(p_title_win, 4, 0, "Type choice, press Enter.");
    mvwaddstr(p_title_win, 5, 0, "  1=Rebuild");

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 7, 0, "         Parity  Serial                Resource");
        mvwaddstr(p_title_win, 8, 0, "Option     Set   Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 7, 0, "         Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 8, 0, "Option    ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }

    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");


    field_opts_off(p_fields[1], O_ACTIVE);

    array_index = ((max_size - 1) * 2) + 1;
    for (i = array_index; i < field_index; i += array_index)
        set_new_page(pp_field[i], TRUE);

    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
    pp_field[field_index] = NULL;

    flush_stdscr();

    p_list_form = new_form(pp_field);

    set_form_win(p_list_form, p_field_win);

    post_form(p_list_form);

    wnoutrefresh(stdscr);
    wnoutrefresh(p_menu_win);
    wnoutrefresh(p_title_win);
    form_driver(p_list_form,
                REQ_FIRST_FIELD);
    wnoutrefresh(p_field_win);
    doupdate();

    while(1)
    {
        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_NEXT_PAGE);
                form_driver(p_list_form,
                            REQ_FIRST_FIELD);

                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_PREV_PAGE);
                form_driver(p_list_form,
                            REQ_FIRST_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if ((ch == KEY_DOWN) ||
                     (ch == '\t'))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_NEXT_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (ch == KEY_UP)
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_PREV_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (IS_ENTER_KEY(ch))
            {
                found = 0;

                unpost_form(p_list_form);

                for (i = 0; i < field_index; i++)
                {
                    p_cur_raid_cmd = (struct array_cmd_data *)field_userptr(pp_field[i]);
                    if (p_cur_raid_cmd != NULL)
                    {
                        p_char = field_buffer(pp_field[i], 0);

                        if (strcmp(p_char, "1") == 0)
                        {
                            found = 1;
                            p_cur_raid_cmd->do_cmd = 1;

                            p_device_record = (struct ipr_device_record *)p_cur_raid_cmd->p_ipr_device->p_qac_entry;
                            if (!p_device_record->issue_cmd)
                            {
                                p_device_record->issue_cmd = 1;
                                p_cur_raid_cmd->p_ipr_ioa->num_raid_cmds++;
                            }
                        }
                        else
                        {
                            p_cur_raid_cmd->do_cmd = 0;

                            p_device_record = (struct ipr_device_record *)p_cur_raid_cmd->p_ipr_device->p_qac_entry;
                            if (p_device_record->issue_cmd)
                            {
                                p_device_record->issue_cmd = 0;
                                p_cur_raid_cmd->p_ipr_ioa->num_raid_cmds--;
                            }
                        }
                    }
                }

                if (found != 0)
                {
                    /* Go to verification screen */
                    unpost_form(p_form);
                    rc = confirm_raid_rebuild();
                    goto leave;
                }
                else
                {
                    post_form(p_list_form);
                    rc = 1;
                }     
            }
            else
            {
                /* Terminal is running under 5250 telnet */
                if (term_is_5250)
                {
                    /* Send it to the form to be processed and flush this line */
                    form_driver(p_list_form, ch);
                    last_char = flush_line();
                    if (IS_5250_CHAR(last_char))
                        ungetch(ENTER_KEY);
                    else if (last_char != ERR)
                        ungetch(last_char);
                }
                else
                {
                    form_driver(p_list_form, ch);
                }
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(pp_field);

    p_cur_raid_cmd = p_head_raid_cmd;
    while(p_cur_raid_cmd)
    {
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }

    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int confirm_raid_rebuild()
{
#define CONFIRM_REBUILD_HDR_SIZE	10
#define CONFIRM_REBUILD_MENU_SIZE	3
    /* iSeries
                        Confirm Rebuild Disk Unit Data

     Rebuilding the disk unit data may take several minutes for
     each unit selected.

     Press Enter to confirm having the data rebuilt. 
     Press q=Cancel to return and change your choice.

              Parity  Serial                Resource 
     Option     Set   Number   Type  Model  Name       
                 1   00058B10  6717   070   /dev/sda
                 2   000585F1  6717   070   /dev/sdb

     q=Cancel

     */
/* non-iSeries
                          Confirm Rebuild Disk Unit Data        

Rebuilding the disk unit data may take several minutes for
each unit selected.

Press Enter to confirm having the data rebuilt. 
Press q=Cancel to return and change your choice.

        Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
Option   ID        ID              Number    Bus    Dev    Bus   ID   Lun
  1     IBM      DFHSS4W          0024A0B9    05     03      0     3    0

 q=Cancel

*/
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_device_record *p_device_record;
    struct ipr_resource_flags *p_resource_flags;
    struct ipr_ioa *p_cur_ioa;
    int fd, rc;
    u16 len;
    char buffer[100];
    u32 num_lines = 0;
    int max_size = curses_info.max_y - (CONFIRM_REBUILD_HDR_SIZE + CONFIRM_REBUILD_MENU_SIZE + 1);
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    PANEL *p_panel;

    p_title_win = newwin(CONFIRM_REBUILD_HDR_SIZE,
                         curses_info.max_x,
                         curses_info.start_y,
                         curses_info.start_x);

    p_menu_win = newwin(CONFIRM_REBUILD_MENU_SIZE,
                        curses_info.max_x,
                        curses_info.start_y + max_size + CONFIRM_REBUILD_HDR_SIZE + 1,
                        curses_info.start_x);

    rc = RC_SUCCESS;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Confirm Rebuild Disk Unit Data");

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + CONFIRM_REBUILD_HDR_SIZE,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    mvwaddstr(p_title_win, 2, 0, "Rebuilding the disk unit data may take several minutes for");
    mvwaddstr(p_title_win, 3, 0, "each unit selected.");
    mvwaddstr(p_title_win, 5, 0, "Press Enter to confirm having the data rebuilt.");
    mvwaddstr(p_title_win, 6, 0, "Press "CANCEL_KEY_LABEL"=Cancel to return and change your choice.");

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 8, 0, "         Parity  Serial                Resource");
        mvwaddstr(p_title_win, 9, 0, "Option     Set   Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 8, 0,
                  "        Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 9, 0,
                  "Option   ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }

    mvwaddstr(p_menu_win, 1, 0, CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_raid_cmd = p_head_raid_cmd;

    while(p_cur_raid_cmd)
    {
        if (p_cur_raid_cmd->do_cmd)
        {
            p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
            p_device_record = (struct ipr_device_record *)p_cur_raid_cmd->p_ipr_device->p_qac_entry;

            if (platform == IPR_ISERIES)
            {
                len = sprintf(buffer, "    1    %4d   ", p_cur_raid_cmd->array_num);
                p_resource_flags = &p_device_record->resource_flags_to_become;
                len += print_dev2(p_cur_raid_cmd->p_ipr_device->p_resource_entry,
                                  buffer + len,
                                  p_resource_flags,
                                  p_cur_raid_cmd->p_ipr_device->dev_name);
            }
            else
            {
                len = sprintf(buffer, "    1   ");
                print_dev_sel(p_cur_raid_cmd->p_ipr_device->p_resource_entry, buffer + len);
            }

            if ((num_lines > 0) && ((num_lines % max_size) == 0))
            {
                mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");
                
                p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                p_tail_win = p_tail_win->p_next;

                p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                p_tail_panel = p_tail_panel->p_next;

                p_tail_win->p_win = newwin(max_size + 1,
                                           curses_info.max_x,
                                           curses_info.start_y + CONFIRM_REBUILD_HDR_SIZE,
                                           curses_info.start_x);
                p_tail_win->p_next = NULL;
                
                p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                p_tail_panel->p_next = NULL;
            }

            mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
            memset(buffer, 0, 100);
            num_lines++;
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        wnoutrefresh(stdscr);
        update_panels();
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_REFRESH;
                goto leave;
            }
            else if (IS_ENTER_KEY(ch))
            {
                for (p_cur_ioa = p_head_ipr_ioa;
                     p_cur_ioa != NULL;
                     p_cur_ioa = p_cur_ioa->p_next)
                {
                    if (p_cur_ioa->num_raid_cmds > 0)
                    {
                        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
                        if (fd <= 1)
                        {
                            rc = RC_FAILED;
                            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                            goto leave;
                        }

                        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                        ioa_cmd.buffer = p_cur_ioa->p_qac_data;
                        ioa_cmd.buffer_len = iprtoh16(p_cur_ioa->p_qac_data->resp_len);
                        ioa_cmd.read_not_write = 0;

                        rc = ipr_ioctl(fd, IPR_REBUILD_DEVICE_DATA, &ioa_cmd);

                        if (rc != 0)
                        {
                            syslog(LOG_ERR, "Rebuild Disk Unit Data to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                            rc = RC_FAILED;
                            close(fd);
                            goto leave;
                        }
                        close(fd);
                    }
                }

                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else
            {
                break;
            }
        }

    }

    leave:
        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}

int raid_include()
{
/*
                    Include Disk Units in Parity Protection

Select the subsystem that the disk unit will be included.

Type choice, press Enter.
  1=Select Parity Array to Include Disk Unit

         Parity  Serial                Resource
Option     Set   Number   Type  Model  Name
            1   02127005  2782   001   /dev/ipr4
            2   00103039  2748   001   /dev/ipr0

e=Exit   q=Cancel    f=PageDn   b=PageUp

*/
/* non-iSeries
                    Include Disk Units in Parity Protection

Select the subsystem that the disk unit will be included.

Type choice, press Enter.
  1=Select Parity Array to Include Disk Unit

         Parity  Vendor    Product           Serial          Resource
Option     Set    ID         ID              Number   Model  Name
            1    IBM      DAB61C0E          3AFB348C   200   /dev/sdb
            2    IBM      5C88AFDC          A775915E   200   /dev/sdc


e=Exit   q=Cancel    f=PageDn   b=PageUp
*/

    int rc, i, j, array_num, last_char;
    struct ipr_record_common *p_common_record;
    struct ipr_array_record *p_array_record;
    struct array_cmd_data *p_cur_raid_cmd;
    u8 array_id;
    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    char buffer[100];
    int array_index;
    int max_size = curses_info.max_y - 12;
    int confirm_raid_stop();
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    int found = 0;
    int include_allowed;
    char *p_char;
    struct ipr_array2_record *p_array2_record;
    struct ipr_supported_arrays *p_supported_arrays;
    struct ipr_array_cap_entry *p_cur_array_cap_entry;
    int configure_raid_include(struct array_cmd_data *p_cur_raid_cmd);
    int confirm_raid_include();

    rc = RC_SUCCESS;
    p_title_win = newwin(9, curses_info.max_x, 0, 0);
    p_menu_win = newwin(3, curses_info.max_x, max_size + 9, 0);
    p_field_win = newwin(max_size, curses_info.max_x, 9, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],                       /* field to alter */
                     0,                                 /* number of buffer to alter */
                     "Include Disk Units in Parity Protection");  /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    array_num = 1;

    check_current_config(false);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        p_cur_ioa->num_raid_cmds = 0;

        for (i = 0;
             i < p_cur_ioa->num_devices;
             i++)
        {
            p_common_record = p_cur_ioa->dev[i].p_qac_entry;

            if ((p_common_record != NULL) &&
                ((p_common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) ||
                 (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD)))
            {
                if (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD)
                {
                    p_array2_record = (struct ipr_array2_record *)
                        p_common_record;

                    p_supported_arrays = p_cur_ioa->p_supported_arrays;
                    p_cur_array_cap_entry =
                        (struct ipr_array_cap_entry *)p_supported_arrays->data;
                    include_allowed = 0;
                    for (j = 0;
                         j < iprtoh16(p_supported_arrays->num_entries);
                         j++)
                    {
                        if (p_cur_array_cap_entry->prot_level ==
                            p_array2_record->raid_level)
                        {
                            if (p_cur_array_cap_entry->include_allowed)
                                include_allowed = 1;
                            break;
                        }

                        p_cur_array_cap_entry = (struct ipr_array_cap_entry *)
                            ((void *)p_cur_array_cap_entry +
                             iprtoh16(p_supported_arrays->entry_length));
                    }

                    if (!include_allowed)
                        continue;
                }

                p_array_record = (struct ipr_array_record *)p_common_record;

                if (p_array_record->established)
                {
                    if (p_head_raid_cmd)
                    {
                        p_tail_raid_cmd->p_next =
                            (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                        p_tail_raid_cmd = p_tail_raid_cmd->p_next;
                    }
                    else
                    {
                        p_head_raid_cmd = p_tail_raid_cmd =
                            (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                    }

                    p_tail_raid_cmd->p_next = NULL;

                    memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

                    array_id =  p_array_record->array_id;

                    p_tail_raid_cmd->array_id = array_id;
                    p_tail_raid_cmd->array_num = array_num;
                    p_tail_raid_cmd->p_ipr_ioa = p_cur_ioa;
                    p_tail_raid_cmd->p_ipr_device = &p_cur_ioa->dev[i];
                    p_tail_raid_cmd->p_qac_data = NULL;

                    start_y = (array_num-1) % (max_size - 1);

                    if ((num_lines > 0) && (start_y == 0))
                    {
                        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                        pp_field[field_index] = new_field(1, curses_info.max_x,
                                                          max_size - 1,
                                                          0,
                                                          0, 0);

                        set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                        set_field_buffer(pp_field[field_index], 0, "More... ");
                        set_field_userptr(pp_field[field_index], NULL);
                        field_opts_off(pp_field[field_index++],
                                       O_ACTIVE);
                    }

                    /* User entry field */
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, 1, start_y, 3, 0, 0);

                    set_field_userptr(pp_field[field_index++], (char *)p_tail_raid_cmd);

                    /* Description field */
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, curses_info.max_x - 9, start_y, 9, 0, 0);

                    if (platform == IPR_ISERIES)
                    {
                        len = sprintf(buffer, "%4d   %8s  %x   %3d   %s",
                                      array_num,
                                      p_cur_ioa->dev[i].p_resource_entry->serial_num,
                                      p_cur_ioa->dev[i].p_resource_entry->type,
                                      p_cur_ioa->ioa.p_resource_entry->model,
                                      p_cur_ioa->dev[i].dev_name);
                    }
                    else
                    {
                        char buf[100];

                        len = sprintf(buffer, "%4d    ", array_num);

                        memcpy(buf, p_cur_ioa->dev[i].p_resource_entry->std_inq_data.vpids.vendor_id,
                               IPR_VENDOR_ID_LEN);
                        buf[IPR_VENDOR_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "%-8s ", buf);

                        memcpy(buf, p_cur_ioa->dev[i].p_resource_entry->std_inq_data.vpids.product_id,
                               IPR_PROD_ID_LEN);
                        buf[IPR_PROD_ID_LEN] = '\0';

                        len += sprintf(buffer + len, "%-12s  ", buf);

                        len += sprintf(buffer + len, "%8s   %03d   %s",
                                       p_cur_ioa->dev[i].p_resource_entry->serial_num,
                                       p_cur_ioa->dev[i].p_resource_entry->model,
                                       p_cur_ioa->dev[i].dev_name);
                    }

                    set_field_buffer(pp_field[field_index], 0, buffer);
                    field_opts_off(pp_field[field_index], O_ACTIVE);
                    set_field_userptr(pp_field[field_index++], NULL);

                    memset(buffer, 0, 100);

                    len = 0;
                    num_lines++;
                    array_num++;
                }
            }
        }
    }

    if (array_num == 1)
    {
        set_field_buffer(p_fields[0],                       /* field to alter */
                         0,                                 /* number of buffer to alter */
                         "Include Device Parity Protection Failed");  /* string value to set */

        mvwaddstr(p_title_win, 2, 0, "There are no disk units eligible for the selected operation");
        mvwaddstr(p_title_win, 3, 0, "due to one or more of the following reasons:");
        mvwaddstr(p_title_win, 5, 0, "o  There are no device parity protected units in the system.");
        mvwaddstr(p_title_win, 6, 0, "o  An IOA is in a condition that makes the disk units attached to");
        mvwaddstr(p_title_win, 7, 0, "   it read/write protected.  Examine the kernel messages log");
        mvwaddstr(p_title_win, 8, 0, "   for any errors that are logged for the IO subsystem");
        mvwaddstr(p_field_win, 0, 0, "   and follow the appropriate procedure for the reference code to");
        mvwaddstr(p_field_win, 1, 0, "   correct the problem, if necessary.");
        mvwaddstr(p_field_win, 2, 0, "o  Not all disk units attached to an advanced function IOA have");
        mvwaddstr(p_field_win, 3, 0, "   reported to the system.  Retry the operation.");
        mvwaddstr(p_field_win, 4, 0, "o  The disk units are missing.");
        mvwaddstr(p_field_win, 5, 0, "o  The disk unit/s are currently in use.");
        mvwaddstr(p_field_win, max_size -1, 0, "Press Enter to continue.");
        mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD);     /* form request code */

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        wnoutrefresh(p_field_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
        }
    }

    mvwaddstr(p_title_win, 2, 0, "Select the subsystem that the disk unit will be included.");
    mvwaddstr(p_title_win, 4, 0, "Type choice, press Enter.");
    mvwaddstr(p_title_win, 5, 0, "  1=Select Parity Array to Include Disk Unit");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 7, 0, "         Parity  Serial                Resource");
        mvwaddstr(p_title_win, 8, 0, "Option     Set   Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 7, 0, "         Parity  Vendor    Product           Serial          Resource");
        mvwaddstr(p_title_win, 8, 0, "Option     Set    ID         ID              Number   Model  Name");
    }

    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");


    field_opts_off(p_fields[1], O_ACTIVE);

    array_index = ((max_size - 1) * 2) + 1;
    for (i = array_index; i < field_index; i += array_index)
        set_new_page(pp_field[i], TRUE);

    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
    pp_field[field_index] = NULL;

    flush_stdscr();

    p_list_form = new_form(pp_field);

    set_form_win(p_list_form, p_field_win);

    post_form(p_list_form);

    wnoutrefresh(stdscr);
    wnoutrefresh(p_menu_win);
    wnoutrefresh(p_title_win);
    form_driver(p_list_form,
                REQ_FIRST_FIELD);
    wnoutrefresh(p_field_win);
    doupdate();

    while(1)
    {
        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_NEXT_PAGE);
                form_driver(p_list_form,
                            REQ_FIRST_FIELD);

                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_PREV_PAGE);
                form_driver(p_list_form,
                            REQ_FIRST_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if ((ch == KEY_DOWN) ||
                     (ch == '\t'))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_NEXT_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (ch == KEY_UP)
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_PREV_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (IS_ENTER_KEY(ch))
            {
                found = 0;
                unpost_form(p_list_form);

                for (i = 0; i < field_index; i++)
                {
                    p_cur_raid_cmd = (struct array_cmd_data *)field_userptr(pp_field[i]);
                    if (p_cur_raid_cmd != NULL)
                    {
                        p_char = field_buffer(pp_field[i], 0);
                        if (strcmp(p_char, "1") == 0)
                        {
                            rc = configure_raid_include(p_cur_raid_cmd);
                            if (rc == RC_CANCEL)
                            {
                                rc = RC_REFRESH;
                                goto leave;
                            }
                            if (rc == RC_EXIT)
                                goto leave;
                            found = 1;
			    break;
                        }
                    }
                }

                if (found != 0)
                {
                    /* Go to verification screen */
                    unpost_form(p_form);
                    rc = confirm_raid_include();
                    goto leave;
                }
                else
                {
                    post_form(p_list_form);
                    rc = 1;
                }     
            }
            else
            {
                /* Terminal is running under 5250 telnet */
                if (term_is_5250)
                {
                    /* Send it to the form to be processed and flush this line */
                    form_driver(p_list_form, ch);
                    last_char = flush_line();
                    if (IS_5250_CHAR(last_char))
                        ungetch(ENTER_KEY);
                    else if (last_char != ERR)
                        ungetch(last_char);
                }
                else
                {
                    form_driver(p_list_form, ch);
                }
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(pp_field);

    p_cur_raid_cmd = p_head_raid_cmd;
    while(p_cur_raid_cmd)
    {
	if (p_cur_raid_cmd->p_qac_data)
	{
	    free(p_cur_raid_cmd->p_qac_data);
	    p_cur_raid_cmd->p_qac_data = NULL;
	}
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }

    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int configure_raid_include(struct array_cmd_data *p_cur_raid_cmd)
{
/* iSeries
                 Include Disk Units in Device Parity Protection

Select the units to be included in Device Parity Protection

Type choice, press Enter.
  1=Include unit in device parity protection

         Parity  Serial                Resource
Option     Set   Number   Type  Model  Name
     1   000BBBC1  6717   050           
     2   0002F5AA  6717   050


e=Exit   q=Cancel    f=PageDn   b=PageUp
*/

/* non-iSeries
                 Include Disk Units in Device Parity Protection

Select the units to be included in Device Parity Protection

Type choice, press Enter.
  1=Include unit in device parity protection

       Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
Option  ID         ID              Number    Bus    Dev    Bus    ID   Lun
       IBMAS400  DGVS09U          00034563    05     03      0     6    0
       IBMAS400  DFHSS4W          00D32240    05     03      0     9    0


e=Exit   q=Cancel    f=PageDn   b=PageUp
*/
    int fd, rc, i, j, k, array_num, last_char;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_array_query_data *p_array_query_data;
    struct ipr_record_common *p_common_record;
    struct ipr_device_record *p_device_record;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_array2_record *p_array2_record;
    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    char buffer[100];
    int array_index;
    int max_size = curses_info.max_y - 12;
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len = 0;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    int found = 0;
    char *p_char;
    struct ipr_device *p_ipr_device;
    struct ipr_supported_arrays *p_supported_arrays;
    struct ipr_array_cap_entry *p_cur_array_cap_entry;
    u8 min_mult_array_devices = 1;
    u8 raid_level;
    char error_str[128];

    rc = RC_SUCCESS;

    p_title_win = newwin(9, curses_info.max_x, 0, 0);
    p_menu_win = newwin(3, curses_info.max_x, max_size + 9, 0);
    p_field_win = newwin(max_size, curses_info.max_x, 9, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],               /* field to alter */
                     0,                         /* number of buffer to alter */
                     "Include Disk Units in Device Parity Protection"); /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    array_num = 1;

    p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
    p_ipr_device = p_cur_raid_cmd->p_ipr_device;
    p_common_record = p_ipr_device->p_qac_entry;

    if (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD)
    {
        /* save off raid level, this information will be used to find
           minimum multiple */
        p_array2_record = (struct ipr_array2_record *)p_common_record;
        raid_level = p_array2_record->raid_level;

        /* Get Query Array Config Data */
        p_array_query_data = (struct ipr_array_query_data *)
            malloc(sizeof(struct ipr_array_query_data));
        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.buffer = p_array_query_data;
        ioa_cmd.buffer_len = sizeof(struct ipr_array_query_data);
        ioa_cmd.cdb[1] = 0x01; /* Volume Set Include */
        ioa_cmd.cdb[2] = p_cur_raid_cmd->array_id;
        ioa_cmd.read_not_write = 1;

        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
        rc = ipr_ioctl(fd, IPR_QUERY_ARRAY_CONFIG, &ioa_cmd);
        close(fd);

        if (rc != 0)
        {
            if (errno != EINVAL)
            {
                syslog(LOG_ERR, "Query Array Config to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            }
	    free(p_array_query_data);
        }
        else
        {
            /* re-map the cross reference table with the new qac data */
            p_cur_raid_cmd->p_qac_data = p_array_query_data;
            p_common_record = (struct ipr_record_common *)p_array_query_data->data;
            for (k = 0; k < p_array_query_data->num_records; k++)
            {
                if (p_common_record->record_id == IPR_RECORD_ID_DEVICE2_RECORD)
                {
                    for (j = 0; j < p_cur_ioa->num_devices; j++)
                    {
                        p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
                        p_device_record = (struct ipr_device_record *)p_common_record;
                        if (p_resource_entry->resource_handle ==
                            p_device_record->resource_handle)
                        {
                            p_cur_ioa->dev[j].p_qac_entry = p_common_record;
                        }
                    }
                }

                /* find minimum multiple for the raid level being used */
                if (p_common_record->record_id == IPR_RECORD_ID_SUPPORTED_ARRAYS)
                {
                    p_supported_arrays = (struct ipr_supported_arrays *)p_common_record;
                    p_cur_array_cap_entry = (struct ipr_array_cap_entry *)p_supported_arrays->data;

                    for (i=0; i<iprtoh16(p_supported_arrays->num_entries); i++)
                    {
                        if (p_cur_array_cap_entry->prot_level == raid_level)
                        {
                            min_mult_array_devices = p_cur_array_cap_entry->min_mult_array_devices;
                        }
                        p_cur_array_cap_entry = (struct ipr_array_cap_entry *)
                            ((void *)p_cur_array_cap_entry + p_supported_arrays->entry_length);
                    }
                }
                p_common_record = (struct ipr_record_common *)
                    ((unsigned long)p_common_record + iprtoh16(p_common_record->record_len));
            }
        }
    }

    for (j = 0; j < p_cur_ioa->num_devices; j++)
    {
        p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;

        if (p_device_record == NULL)
            continue;

        if (p_cur_ioa->dev[j].is_start_cand)
            continue;

        if (p_device_record->include_cand)
        {
            if (p_cur_ioa->dev[j].opens != 0)
            {
                syslog(LOG_ERR,"Include Device not allowed, device in use - %s"IPR_EOL,
                       p_cur_ioa->dev[j].dev_name); 
                continue;
            }

            start_y = (array_num-1) % (max_size - 1);

            if ((num_lines > 0) && (start_y == 0))
            {
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, curses_info.max_x,
                                                  max_size - 1,
                                                  0,
                                                  0, 0);

                set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                set_field_buffer(pp_field[field_index], 0, "More... ");
                set_field_userptr(pp_field[field_index], NULL);
                field_opts_off(pp_field[field_index++],
                               O_ACTIVE);
            }

            /* User entry field */
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, 1, start_y, 3, 0, 0);
            set_field_userptr(pp_field[field_index++], (char *)&p_cur_ioa->dev[j]);

            /* Description field */
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, curses_info.max_x - 9, start_y, 9, 0, 0);

            p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

            if (platform == IPR_ISERIES)
            {
                len = sprintf(buffer, " %8s  %x   %03d   %-38s",
                              p_resource_entry->serial_num,
                              p_resource_entry->type,
                              p_resource_entry->model,
                              p_cur_ioa->dev[j].dev_name);
            }
            else
            {
                len = print_dev_sel(p_resource_entry, buffer);
            }

            set_field_buffer(pp_field[field_index], 0, buffer);
            field_opts_off(pp_field[field_index], O_ACTIVE);
            set_field_userptr(pp_field[field_index++], NULL);

            memset(buffer, 0, 100);

            len = 0;
            num_lines++;
            array_num++;
        }
    }

    if  (array_num <= min_mult_array_devices)
    {
        p_ipr_device = p_cur_raid_cmd->p_ipr_device;
        p_common_record = p_ipr_device->p_qac_entry;

        set_field_buffer(p_fields[0],                  /* field to alter */
                         0,                            /* number of buffer to alter */
                         "Include Disk Unit Failed");  /* string value to set */

        mvwaddstr(p_title_win, 2, 0, "There are no disk units eligible for the selected operation");
        mvwaddstr(p_title_win, 3, 0, "due to one or more of the following reasons:");
        mvwaddstr(p_title_win, 5, 0, "o  There are not enough disk units available to be included.");
        mvwaddstr(p_title_win, 6, 0, "o  The disk unit that needs to be included is not at the right");
        mvwaddstr(p_title_win, 7, 0, "   location.  Examine the 'message log' for the exposed unit");
        mvwaddstr(p_title_win, 8, 0, "   and make sure that the replacement unit is at the correct");
        mvwaddstr(p_field_win, 0, 0, "   location");
        mvwaddstr(p_field_win, 1, 0, "o  Not all disk units attached to an IOA have reported to the");
        mvwaddstr(p_field_win, 2, 0, "   system.  Retry the operation.");
        if (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD)
        {
            mvwaddstr(p_field_win, 3, 0, "o  The disk unit to be included must be the same or greater");
            mvwaddstr(p_field_win, 4, 0, "   capacity than the smallest device in the volume set and");
            mvwaddstr(p_field_win, 5, 0, "   be formatted correctly");
        }
        else
        {
            mvwaddstr(p_field_win, 3, 0, "o  All the disk units in a parity set must be the same capacity");
            mvwaddstr(p_field_win, 4, 0, "   with a minumum number of 4 disk units and a maximum of 10 units");
            mvwaddstr(p_field_win, 5, 0, "   in the resulting parity set");
        }
        mvwaddstr(p_field_win, 6, 0, "o  The type/model of the disk units is not supported for the");
        mvwaddstr(p_field_win, 7, 0, "   requested operation");
        mvwaddstr(p_field_win, max_size -1, 0, "Press Enter to continue.");
        mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD);     /* form request code */

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        wnoutrefresh(p_field_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
        }
    }

    mvwaddstr(p_title_win, 2, 0, "Select the units to be included in Device Parity Protection");
    mvwaddstr(p_title_win, 4, 0, "Type choice, press Enter.");
    mvwaddstr(p_title_win, 5, 0, "  1=Include unit in device parity protection");

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 7, 0, "           Serial                Resource");
        mvwaddstr(p_title_win, 8, 0, "Option     Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 7, 0,
                  "         Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 8, 0,
                  "Option    ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }

    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");


    field_opts_off(p_fields[1], O_ACTIVE);

    array_index = ((max_size - 1) * 2) + 1;
    for (i = array_index; i < field_index; i += array_index)
        set_new_page(pp_field[i], TRUE);

    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
    pp_field[field_index] = NULL;

    flush_stdscr();

    p_list_form = new_form(pp_field);

    set_form_win(p_list_form, p_field_win);

    post_form(p_list_form);

    wnoutrefresh(stdscr);
    wnoutrefresh(p_menu_win);
    wnoutrefresh(p_title_win);
    form_driver(p_list_form,
                REQ_FIRST_FIELD);
    wnoutrefresh(p_field_win);
    doupdate();

    while(1)
    {
        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_NEXT_PAGE);
                form_driver(p_list_form,
                            REQ_FIRST_FIELD);

                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_PREV_PAGE);
                form_driver(p_list_form,
                            REQ_FIRST_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if ((ch == KEY_DOWN) ||
                     (ch == '\t'))
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_NEXT_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (ch == KEY_UP)
            {
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);

                form_driver(p_list_form,
                            REQ_PREV_FIELD);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
            else if (IS_ENTER_KEY(ch))
            {
                /* first, count devices select to be sure min multiple
                   is satisfied */
                found = 0;
                for (i = 0; i < field_index; i++)
                {
                    p_char = field_buffer(pp_field[i], 0);

                    if (strcmp(p_char, "1") == 0)
                    {
                        found++;
                    }
                }

                if (found % min_mult_array_devices != 0)
                {
                    sprintf(error_str,"Error:  number of devices selected must be a multiple of %d",min_mult_array_devices);
                    mvwaddstr(p_menu_win, 2, 0, error_str);

                    wnoutrefresh(stdscr);
                    wnoutrefresh(p_menu_win);
                    wnoutrefresh(p_title_win);

                    form_driver(p_list_form,
                                REQ_NEXT_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);

                    wnoutrefresh(p_field_win);
                    doupdate();
                    break;
                }

                found = 0;

                unpost_form(p_list_form);

                for (i = 0; i < field_index; i++)
                {
                    p_ipr_device = (struct ipr_device *)field_userptr(pp_field[i]);
                    if (p_ipr_device != NULL)
                    {
                        p_char = field_buffer(pp_field[i], 0);

                        if (strcmp(p_char, "1") == 0)
                        {
                            found = 1;
                            p_cur_raid_cmd->do_cmd = 1;

                            p_device_record = (struct ipr_device_record *)p_ipr_device->p_qac_entry;
                            if (!p_device_record->issue_cmd)
                            {
                                p_device_record->issue_cmd = 1;
                                p_device_record->known_zeroed = 1;
                                p_ipr_device->is_start_cand = 1;
                                p_cur_raid_cmd->p_ipr_ioa->num_raid_cmds++;
                            }
                        }
                        else
                        {
                            p_device_record = (struct ipr_device_record *)p_ipr_device->p_qac_entry;
                            if (p_device_record->issue_cmd)
                            {
                                p_device_record->issue_cmd = 0;
                                p_device_record->known_zeroed = 0;
                                p_ipr_device->is_start_cand = 0;
                            }
                        }
                    }
                }

                if (found != 0)
                {
                    /* Go to verification screen */
                    unpost_form(p_form);
                    goto leave;
                }
                else
                {
                    post_form(p_list_form);
                    rc = 1;
                }     
            }
            else
            {
                /* Terminal is running under 5250 telnet */
                if (term_is_5250)
                {
                    /* Send it to the form to be processed and flush this line */
                    form_driver(p_list_form, ch);
                    last_char = flush_line();
                    if (IS_5250_CHAR(last_char))
                        ungetch(ENTER_KEY);
                    else if (last_char != ERR)
                        ungetch(last_char);
                }
                else
                {
                    form_driver(p_list_form, ch);
                }
                wnoutrefresh(stdscr);
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);
                wnoutrefresh(p_field_win);
                doupdate();
                break;
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(pp_field);

    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int confirm_raid_include()
{
#define CONFIRM_INCLUDE_HDR_SIZE	11
#define CONFIRM_INCLUDE_MENU_SIZE	3
/* iSeries
                         Confirm Disk Units to be Included

ATTENTION: All listed device units will be formatted and zeroed before
listed device is added to array.

Press Enter to confirm your choice to have the system include
the selected units in device parity protection. 
Press q=Cancel to return and change your choice.

         Parity  Serial                Resource 
Option     Set   Number   Type  Model  Name       
   1        1   00058B10  6717   070   /dev/sda
   1        2   000585F1  6717   070   /dev/sdb


q=Cancel

*/
/* non-iSeries
                         Confirm Disk Units to be Included

ATTENTION: All listed device units will be formatted and zeroed before
listed device is added to array.

Press Enter to confirm your choice to have the system include
the selected units in device parity protection. 
Press q=Cancel to return and change your choice.

Parity   Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
   Set    ID         ID              Number    Bus    Dev    Bus    ID   Lun
    1    IBMAS400  DGVS09U          00034563    05     03      0     6    0
    1    IBMAS400  DFHSS4W          00D32240    05     03      0     9    0


q=Cancel

*/
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    struct ipr_resource_entry *p_resource_entry;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_device_record *p_device_record;
    int rc, i, j, found;
    u16 len;
    char buffer[100];
    u32 num_lines = 0;
    int max_size = curses_info.max_y - (CONFIRM_INCLUDE_HDR_SIZE + CONFIRM_INCLUDE_MENU_SIZE + 1);
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    PANEL *p_panel;
    int confirm_raid_include_device();
    struct ipr_array_record *p_array_record;
    struct ipr_array_query_data   *p_qac_data;
    struct ipr_record_common *p_common_record;
    struct ipr_ioa *p_cur_ioa;

    p_title_win = newwin(CONFIRM_INCLUDE_HDR_SIZE,
                         curses_info.max_x,
                         curses_info.start_y,
                         curses_info.start_x);

    p_menu_win = newwin(CONFIRM_INCLUDE_MENU_SIZE,
                        curses_info.max_x,
                        curses_info.start_y + max_size + CONFIRM_INCLUDE_HDR_SIZE + 1,
                        curses_info.start_x);

    rc = RC_SUCCESS;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Confirm Disk Units to be Included");

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + CONFIRM_INCLUDE_HDR_SIZE,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    wattron(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 0, "ATTENTION:");
    wattroff(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 10, " All listed device units will be formatted and zeroed before");
    mvwaddstr(p_title_win, 3, 0, "listed device is added to array.");
    mvwaddstr(p_title_win, 5, 0, "Press Enter to confirm your choice to have the system include");
    mvwaddstr(p_title_win, 6, 0, "  the selected units in device parity protection");
    mvwaddstr(p_title_win, 7, 0, "Press "CANCEL_KEY_LABEL"=Cancel to return and change your choice.");

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 9, 0,
                  "         Parity  Serial                Resource");
        mvwaddstr(p_title_win,10, 0,
                  "Option     Set   Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 9, 0,
                  "Parity   Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win,10, 0,
                  "   Set    ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }

    mvwaddstr(p_menu_win, 1, 0, CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_raid_cmd = p_head_raid_cmd;

    while(p_cur_raid_cmd)
    {
        if (p_cur_raid_cmd->do_cmd)
        {
            p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

            p_array_record = (struct ipr_array_record *)
                p_cur_raid_cmd->p_ipr_device->p_qac_entry;

            if (p_array_record->common.record_id ==
                IPR_RECORD_ID_ARRAY2_RECORD)
            {
                p_qac_data = p_cur_raid_cmd->p_qac_data;
            }
            else
            {
                p_qac_data = p_cur_ioa->p_qac_data;
            }

            p_common_record = (struct ipr_record_common *)
                p_qac_data->data;

            for (i = 0; i < p_qac_data->num_records; i++)
            {
                p_device_record = (struct ipr_device_record *)p_common_record;
                if (p_device_record->issue_cmd)
                {
                    found = 0;
                    for (j = 0; j < p_cur_ioa->num_devices; j++)
                    {
                        p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

                        if (p_resource_entry->resource_handle ==
                            p_device_record->resource_handle)
                        {
                            found = 1;
                            break;
                        }
                    }

                    if (found)
                    {
                        if (platform == IPR_ISERIES)
                        {
                            len = sprintf(buffer, "    1    %4d   ", p_cur_raid_cmd->array_num);

                            len += print_dev2(p_resource_entry, buffer + len,
                                              &p_device_record->resource_flags_to_become,
                                              p_cur_ioa->dev[j].dev_name);
                        }
                        else
                        {
                            char buf[100];

                            len = sprintf(buffer, " %4d    ", p_cur_raid_cmd->array_num);

                            memcpy(buf, p_resource_entry->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
                            buf[IPR_VENDOR_ID_LEN] = '\0';

                            len += sprintf(buffer + len, "%-8s  ", buf);

                            memcpy(buf, p_resource_entry->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);
                            buf[IPR_PROD_ID_LEN] = '\0';

                            len += sprintf(buffer + len, "%-12s ", buf);

                            len += sprintf(buffer + len, "%8s    ", p_resource_entry->serial_num);

                            len += sprintf(buffer + len, "%02d     %02d     %2d    %2d   %2d",
                                           p_resource_entry->pci_bus_number,
                                           p_resource_entry->pci_slot,
                                           p_resource_entry->resource_address.bus,
                                           p_resource_entry->resource_address.target,
                                           p_resource_entry->resource_address.lun);
                        }

                        if ((num_lines > 0) && ((num_lines % max_size) == 0))
                        {
                            mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                            p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                            p_tail_win = p_tail_win->p_next;

                            p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                            p_tail_panel = p_tail_panel->p_next;

                            p_tail_win->p_win = newwin(max_size + 1,
                                                       curses_info.max_x,
                                                       curses_info.start_y + CONFIRM_INCLUDE_HDR_SIZE,
                                                       curses_info.start_x);
                            p_tail_win->p_next = NULL;

                            p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                            p_tail_panel->p_next = NULL;
                        }

                        mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
                        memset(buffer, 0, 100);
                        num_lines++;
                    }
                }

                p_common_record = (struct ipr_record_common *)
                    ((unsigned long)p_common_record +
                     iprtoh16(p_common_record->record_len));
            }
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        wnoutrefresh(stdscr);
        update_panels();
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_REFRESH;
                goto leave;
            }
            else if (IS_ENTER_KEY(ch))
            {
                unpost_form(p_form);
                free_form(p_form);
                curses_info.p_form = p_old_form;
                free_screen(p_head_panel, p_head_win, p_fields);
                delwin(p_title_win);
                delwin(p_menu_win);
                werase(stdscr);
                rc = confirm_raid_include_device();

                return rc;
            }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else
            {
                break;
            }
        }

    }

    leave:
        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}

int confirm_raid_include_device()
{
/*
                            Confirm Initialize and Format Disk Unit        

Press 'c' to confirm your choice for 1=Initialize and format.
Press q to return to change your choice.

        Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
Option   ID        ID              Number    Bus    Dev    Bus   ID   Lun
        IBM      DFHSS4W          0024A0B9    05     03      0     3    0
        IBM      DFHSS4W          00D38D0F    05     03      0     5    0
        IBM      DFHSS4W          00D32240    05     03      0     9    0
        IBM      DFHSS4W          00034563    05     03      0     6    0
        IBM      DFHSS4W          00D37A11    05     03      0     4    0

c=Confirm   q=Cancel    f=PageDn   b=PageUp

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx


                            Confirm Initialize and Format Disk Unit        

Press 'c' to confirm your choice for 1=Initialize and format.
Press q to return to change your choice.

               Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI
Option  Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun
        6607  0024A0B9    03      D01       05     03      0     3    0
        6607  00D38D0F    03      D02       05     03      0     5    0
        6607  00D32240    03      D03       05     03      0     9    0
        6607  00034563    03      D04       05     03      0     6    0
        6717  00D37A11    03      D05       05     03      0     4    0

c=Confirm   q=Cancel    f=PageDn   b=PageUp



*/
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    char buffer[100];
    int max_size = curses_info.max_y - 10;
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel;
    WINDOW *p_title_win, *p_menu_win;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_device_record *p_device_record;
    struct devs_to_init_t *p_cur_dev_to_init;
    u32 len, i, j, found;
    u32 num_lines = 0;
    int rc = RC_SUCCESS;
    PANEL *p_panel;
    u8 num_devs=0;
    struct ipr_ioa *p_cur_ioa;
    struct ipr_array_record *p_array_record;
    struct ipr_array_query_data *p_qac_data;
    struct ipr_record_common *p_common_record;

    int format_include_cand();
    int dev_include_complete(u8 num_devs);

    p_title_win = newwin(7, curses_info.max_x,
                         curses_info.start_y, curses_info.start_x);

    p_menu_win = newwin(3, curses_info.max_x,
                        curses_info.start_y + max_size + 7, curses_info.start_x);


    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Confirm Initialize and Format Disk Unit");

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));


    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + 7,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    mvwaddstr(p_title_win, 2, 0, "Press 'c' to confirm your choice for 1=Initialize and format.");
    mvwaddstr(p_title_win, 3, 0, "Press "CANCEL_KEY_LABEL" to return to change your choice.");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 5, 0,
                  "               Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "Option  Type   Number     ID    Position   Bus    Dev    Bus    ID   Lun");
    }
    else
    {
        mvwaddstr(p_title_win, 5, 0,
                  "        Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "Option   ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }

    mvwaddstr(p_menu_win, 1, 0, "c=Confirm   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_raid_cmd = p_head_raid_cmd;

    while(p_cur_raid_cmd)
    {
        if (p_cur_raid_cmd->do_cmd)
        {
            p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

            p_array_record = (struct ipr_array_record *)
                p_cur_raid_cmd->p_ipr_device->p_qac_entry;

            if (p_array_record->common.record_id ==
                IPR_RECORD_ID_ARRAY2_RECORD)
            {
                p_qac_data = p_cur_raid_cmd->p_qac_data;
            }
            else
            {
                p_qac_data = p_cur_ioa->p_qac_data;
            }

            p_common_record = (struct ipr_record_common *)
                p_qac_data->data;

            for (i = 0; i < p_qac_data->num_records; i++)
            {
                p_device_record = (struct ipr_device_record *)p_common_record;
                if (p_device_record->issue_cmd)
                {
                    found = 0;
                    for (j = 0; j < p_cur_ioa->num_devices; j++)
                    {
                        p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

                        if (p_resource_entry->resource_handle ==
                            p_device_record->resource_handle)
                        {
                            found = 1;
                            break;
                        }
                    }

                    if (found)
                    {
                        len = sprintf(buffer, "    1   ");
                        len += print_dev_sel(p_resource_entry, buffer + len);

                        if ((num_lines > 0) && ((num_lines % max_size) == 0))
                        {
                            mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                            p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                            p_tail_win = p_tail_win->p_next;

                            p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                            p_tail_panel = p_tail_panel->p_next;

                            p_tail_win->p_win = newwin(max_size + 1,
                                                       curses_info.max_x,
                                                       curses_info.start_y + CONFIRM_INCLUDE_HDR_SIZE,
                                                       curses_info.start_x);
                            p_tail_win->p_next = NULL;

                            p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                            p_tail_panel->p_next = NULL;
                        }

                        mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
                        memset(buffer, 0, 100);
                        num_lines++;
                    }
                }

                p_common_record = (struct ipr_record_common *)
                    ((unsigned long)p_common_record +
                     iprtoh16(p_common_record->record_len));
            }
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

    clear();
    flush_stdscr();
    form_driver(p_form,               /* form to pass input to */
                REQ_FIRST_FIELD );     /* form request code */

    wnoutrefresh(stdscr);
    update_panels();
    wnoutrefresh(p_menu_win);
    wnoutrefresh(p_title_win);
    doupdate();

    while (1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_FIRST_FIELD );     /* form request code */

        wnoutrefresh(stdscr);
        update_panels();
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_REFRESH;
                goto leave;
            }
            else if ((ch == 'c') || (ch == 'C'))
            {
                /* now format each of the devices */
                num_devs = format_include_cand();

                unpost_form(p_form);
                rc = dev_include_complete(num_devs);

                /* free up memory allocated for format */
                p_cur_dev_to_init = p_head_dev_to_init;
                while(p_cur_dev_to_init)
                {
                    p_cur_dev_to_init = p_cur_dev_to_init->p_next;
                    free(p_head_dev_to_init);
                    p_head_dev_to_init = p_cur_dev_to_init;
                }
                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else
            {
                break;
            }
        }
    }
    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}

int format_include_cand()
{
    int fd, fdev, pid;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_resource_entry *p_resource_entry;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_ioa *p_cur_ioa;
    int num_devs = 0;
    int rc = 0;
    struct ipr_array_record *p_array_record;
    struct ipr_array_query_data *p_qac_data;
    struct ipr_record_common *p_common_record;
    struct ipr_device_record *p_device_record;
    int i, j, found, opens;

    p_cur_raid_cmd = p_head_raid_cmd;
    while (p_cur_raid_cmd)
    {
        if (p_cur_raid_cmd->do_cmd)
        {
            p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

            p_array_record = (struct ipr_array_record *)
                p_cur_raid_cmd->p_ipr_device->p_qac_entry;

            if (p_array_record->common.record_id ==
                IPR_RECORD_ID_ARRAY2_RECORD)
            {
                p_qac_data = p_cur_raid_cmd->p_qac_data;
            }
            else
            {
                p_qac_data = p_cur_ioa->p_qac_data;
            }

            p_common_record = (struct ipr_record_common *)
                p_qac_data->data;

            for (i = 0; i < p_qac_data->num_records; i++)
            {
                p_device_record = (struct ipr_device_record *)p_common_record;
                if (p_device_record->issue_cmd)
                {
                    found = 0;
                    for (j = 0; j < p_cur_ioa->num_devices; j++)
                    {
                        p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

                        if (p_resource_entry->resource_handle ==
                            p_device_record->resource_handle)
                        {
                            found = 1;
                            break;
                        }
                    }

                    if (found)
                    {
                        /* get current "opens" data for this device to determine conditions to continue */
                        opens = num_device_opens(p_cur_ioa->host_num,
                                                 p_resource_entry->resource_address.bus,
                                                 p_resource_entry->resource_address.target,
                                                 p_resource_entry->resource_address.lun);
                        if (opens != 0)
                        {
                            syslog(LOG_ERR,"Include Device not allowed, device in use - %s"IPR_EOL,
                                   p_cur_ioa->dev[j].dev_name);
                            p_device_record->issue_cmd = 0;
                            continue;
                        }

                        if (p_head_dev_to_init)
                        {
                            p_tail_dev_to_init->p_next = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));
                            p_tail_dev_to_init = p_tail_dev_to_init->p_next;
                        }
                        else
                            p_head_dev_to_init = p_tail_dev_to_init = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));

                        memset(p_tail_dev_to_init, 0, sizeof(struct devs_to_init_t));
                        p_tail_dev_to_init->p_ioa = p_cur_ioa;
                        p_tail_dev_to_init->p_ipr_device = &p_cur_ioa->dev[j];
                        p_tail_dev_to_init->do_init = 1;

                        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                        ioa_cmd.buffer = NULL;
                        ioa_cmd.buffer_len = 0;
                        ioa_cmd.device_cmd = 1;
                        ioa_cmd.resource_address = p_resource_entry->resource_address;

                        if (!p_tail_dev_to_init->p_ipr_device->p_resource_entry->is_hidden)
                        {
                            fdev = open(p_tail_dev_to_init->p_ipr_device->dev_name, O_RDWR | O_NONBLOCK);

                            if (fdev)
                            {
                                fcntl(fdev, F_SETFD, 0);
                                if (flock(fdev, LOCK_EX))
                                    syslog(LOG_ERR,
                                           "Could not set lock for %s. %m"IPR_EOL,
                                           p_tail_dev_to_init->p_ipr_device->dev_name);
                            }

                            p_tail_dev_to_init->fd = fdev;
                        }

                        pid = fork();
                        if (pid)
                        {
                            /* Do nothing */
                        }
                        else
                        {
                            /* Issue the format. Failure will be detected by query command status */
                            fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
                            rc = ipr_ioctl(fd, FORMAT_UNIT, &ioa_cmd);
                            close(fd);
                            exit(0);
                        }
                        num_devs++;

                        if (rc != 0)
                        {
                            syslog(LOG_ERR, "Format failed for device %s.  %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                        }
                    }
                }
                p_common_record = (struct ipr_record_common *)
                    ((unsigned long)p_common_record +
                     iprtoh16(p_common_record->record_len));
            }
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }
    return num_devs;
}

int dev_include_complete(u8 num_devs)
{
#define FORMAT_COMPLETE_HDR_SIZE        7
    FIELD *p_fields[3];
    FORM *p_form;
    FORM *p_old_form;
    int done_bad;
    int max_size = curses_info.max_y - (FORMAT_COMPLETE_HDR_SIZE + 3 + 1);
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel;
    struct ipr_cmd_status cmd_status;
    struct ipr_cmd_status_record *p_record;
    int not_done = 0;
    int rc, i, j;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct devs_to_init_t *p_cur_dev_to_init;
    u32 percent_cmplt = 0;
    WINDOW *p_title_win, *p_menu_win;
    int num_includes = 0;
    int fd, found;
    struct ipr_ioa *p_cur_ioa;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_array_record *p_array_record;
    struct ipr_record_common *p_common_record_saved;
    struct ipr_device_record *p_device_record_saved;
    struct ipr_record_common *p_common_record;
    struct ipr_device_record *p_device_record;

    p_title_win = newwin(FORMAT_COMPLETE_HDR_SIZE,
                         curses_info.max_x,
                         curses_info.start_y,
                         curses_info.start_x);

    p_menu_win = newwin(3, curses_info.max_x,
                        curses_info.start_y + max_size + 7, curses_info.start_x);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Include Disk Units in Device Parity Protection Status");

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + FORMAT_COMPLETE_HDR_SIZE,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    mvwaddstr(p_title_win, 2, 0, "The operation to include units in the device parity protection");
    mvwaddstr(p_title_win, 3, 0, "will be done in several phases.  The phases are listed here");
    mvwaddstr(p_title_win, 4, 0, "and the status will be indicated when known.");
    mvwaddstr(p_title_win, 6, 0, "Phase                                       Status");
    mvwaddstr(p_menu_win, 2, 0, "Please wait for next screen");
    mvwaddstr(p_tail_win->p_win,0, 0, "Prepare to include units . . . . . . . . :");
    mvwaddstr(p_tail_win->p_win,1, 0, "Include units  . . . . . . . . . . . . . :");

    form_driver(p_form,               /* form to pass input to */
                REQ_FIRST_FIELD );     /* form request code */

    clear ();
    wnoutrefresh(stdscr);
    update_panels();
    wnoutrefresh(p_title_win);
    wnoutrefresh(p_menu_win);
    doupdate();

    mvprintw(7, 46, "%3d %%", percent_cmplt);
    refresh();

    /* Here we need to sleep for a bit to make sure our children have started
     their formats. There isn't really a good way to know whether or not the formats
     have started, so we just wait for 60 seconds */
    sleep(60);

    while(1)
    {
        mvprintw(7, 46, "%3d %%", percent_cmplt);
        form_driver(p_form,
                    REQ_FIRST_FIELD);
        refresh();

        percent_cmplt = 100;
        done_bad = 0;

        for (p_cur_dev_to_init = p_head_dev_to_init;
             p_cur_dev_to_init != NULL;
             p_cur_dev_to_init = p_cur_dev_to_init->p_next)
        {
            if (p_cur_dev_to_init->do_init)
            {
                fd = open(p_cur_dev_to_init->p_ioa->ioa.dev_name, O_RDWR);

                if (fd < 0)
                {
                    syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_dev_to_init->p_ioa->ioa.dev_name);
                    done_bad = 1;
                    continue;
                }

                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                ioa_cmd.buffer = &cmd_status;
                ioa_cmd.buffer_len = sizeof(cmd_status);
                ioa_cmd.read_not_write = 1;
                ioa_cmd.device_cmd = 1;
                ioa_cmd.resource_address = p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address;

                rc = ipr_ioctl(fd, IPR_QUERY_COMMAND_STATUS, &ioa_cmd);

                close(fd);
                if (rc)
                {
                    syslog(LOG_ERR, "Query Command Status to %s failed. %m"IPR_EOL,
                           p_cur_dev_to_init->p_ipr_device->dev_name);
                    p_cur_dev_to_init->cmplt = 100;
                    continue;
                }

                if (cmd_status.num_records == 0)
                    p_cur_dev_to_init->cmplt = 100;
                else
                {
                    p_record = cmd_status.record;
                    if (p_record->status == IPR_CMD_STATUS_SUCCESSFUL)
                    {
                        p_cur_dev_to_init->cmplt = 100;
                    }
                    else if (p_record->status == IPR_CMD_STATUS_FAILED)
                    {
                        p_cur_dev_to_init->cmplt = 100;
                        done_bad = 1;
                    }
                    else
                    {
                        p_cur_dev_to_init->cmplt = p_record->percent_complete;
                        if (p_cur_dev_to_init->cmplt < percent_cmplt)
                            percent_cmplt = p_cur_dev_to_init->cmplt;
                        not_done = 1;
                    }
                }
            }
        }

        unpost_form(p_form);

        if (!not_done)
        {
            p_cur_dev_to_init = p_head_dev_to_init;
            while(p_cur_dev_to_init)
            {
                if (p_cur_dev_to_init->do_init)
                {
                    if (p_cur_dev_to_init->fd)
                        close(p_cur_dev_to_init->fd);
                }
                p_cur_dev_to_init = p_cur_dev_to_init->p_next;
            }

            if (done_bad)
            {
                rc = RC_FAILED;
                goto leave;
            }

            break;
        }
        not_done = 0;
        sleep(10);
    }

    mvprintw(7, 44, "Complete");
    form_driver(p_form,
                REQ_FIRST_FIELD);
    refresh();
    sleep(5);

    /* now issue the start array command with "known to be zero" */
    for (p_cur_raid_cmd = p_head_raid_cmd;
	 p_cur_raid_cmd != NULL; 
	 p_cur_raid_cmd = p_cur_raid_cmd->p_next)
    {
        if (p_cur_raid_cmd->do_cmd == 0)
            continue;

        p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

        if (p_cur_ioa->num_raid_cmds > 0)
        {
            fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
            if (fd <= 1)
            {
                rc = RC_FAILED;
                syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                continue;
            }

            p_array_record = (struct ipr_array_record *)
                p_cur_raid_cmd->p_ipr_device->p_qac_entry;

            if (p_array_record->common.record_id ==
                IPR_RECORD_ID_ARRAY2_RECORD)
            {
                /* refresh qac data for ARRAY2 */
                memset(&ioa_cmd, 0,
                       sizeof(struct ipr_ioctl_cmd_internal));

                ioa_cmd.buffer = p_cur_ioa->p_qac_data;
                ioa_cmd.buffer_len = sizeof(struct ipr_array_query_data);
                ioa_cmd.cdb[1] = 0x01; /* Volume Set Include */
                ioa_cmd.cdb[2] = p_cur_raid_cmd->array_id;
                ioa_cmd.read_not_write = 1;

                rc = ipr_ioctl(fd, IPR_QUERY_ARRAY_CONFIG, &ioa_cmd);

                if (rc != 0)
                {
                    if (errno != EINVAL)
                        syslog(LOG_ERR,"Query Array Config to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    close(fd);
                    continue;
                }

                /* scan through this raid command to determine which
                 devices should be part of this parity set */
                p_common_record_saved = (struct ipr_record_common *)
                    p_cur_raid_cmd->p_qac_data->data;
                for (i = 0; i < p_cur_raid_cmd->p_qac_data->num_records; i++)
                {
                    p_device_record_saved = (struct ipr_device_record *)p_common_record_saved;
                    if (p_device_record_saved->issue_cmd)
                    {
                        p_common_record = (struct ipr_record_common *)
                            p_cur_ioa->p_qac_data->data;
                        found = 0;
                        for (j = 0; j < p_cur_ioa->p_qac_data->num_records; j++)
                        {
                            p_device_record = (struct ipr_device_record *)p_common_record;
                            if (p_device_record->resource_handle ==
                                p_device_record_saved->resource_handle)
                            {
                                p_device_record->issue_cmd = 1;
                                p_device_record->known_zeroed = 1;
                                found = 1;
                                break;
                            }
                            p_common_record = (struct ipr_record_common *)
                                ((unsigned long)p_common_record +
                                 iprtoh16(p_common_record->record_len));
                        }
                        if (!found)
                        {
                            syslog(LOG_ERR,"Resource not found to include device to %s failed"IPR_EOL, p_cur_ioa->ioa.dev_name);
                            break;
                        }
                    }
                    p_common_record_saved = (struct ipr_record_common *)
                        ((unsigned long)p_common_record_saved +
                         iprtoh16(p_common_record_saved->record_len));
                }

                if (!found)
                {
                    close(fd);
                    continue;
                }
            }

            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

            ioa_cmd.buffer = p_cur_ioa->p_qac_data;
            ioa_cmd.buffer_len = iprtoh16(p_cur_ioa->p_qac_data->resp_len);
            rc = ipr_ioctl(fd, IPR_ADD_ARRAY_DEVICE, &ioa_cmd);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Add Array Device to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                rc = RC_FAILED;
                close(fd);
                goto leave;
            }
            close(fd);
        }
    }

    not_done = 0;
    percent_cmplt = 0;

    while(1)
    {
        mvprintw(8, 46, "%3d %%", percent_cmplt);
        form_driver(p_form,
                    REQ_FIRST_FIELD);
        refresh();

        percent_cmplt = 100;
        done_bad = 0;
        num_includes = 0;

        for (p_cur_ioa = p_head_ipr_ioa;
             p_cur_ioa != NULL;
             p_cur_ioa = p_cur_ioa->p_next)
        {
            if (p_cur_ioa->num_raid_cmds > 0)
            {
                int fd = open(p_cur_ioa->ioa.dev_name, O_RDONLY);

                if (fd < 0)
                {
                    syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    p_cur_ioa->num_raid_cmds = 0;
                    done_bad = 1;
                    continue;
                }
                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                ioa_cmd.buffer = &cmd_status;
                ioa_cmd.buffer_len = sizeof(cmd_status);
                ioa_cmd.read_not_write = 1;

                rc = ipr_ioctl(fd, IPR_QUERY_COMMAND_STATUS, &ioa_cmd);

                if (rc)
                {
                    syslog(LOG_ERR, "Query Command Status to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    close(fd);
                    p_cur_ioa->num_raid_cmds = 0;
                    done_bad = 1;
                    continue;
                }

                close(fd);
                p_record = cmd_status.record;

                for (j=0; j < cmd_status.num_records; j++, p_record++)
                {
                    if (p_record->command_code == IPR_ADD_ARRAY_DEVICE)
                    {
                        for (p_cur_raid_cmd = p_head_raid_cmd;
                             p_cur_raid_cmd != NULL; 
                             p_cur_raid_cmd = p_cur_raid_cmd->p_next)
                        {
                            if ((p_cur_raid_cmd->p_ipr_ioa = p_cur_ioa) &&
                                (p_cur_raid_cmd->array_id == p_record->array_id))
                            {
                                num_includes++;
                                if (p_record->status == IPR_CMD_STATUS_IN_PROGRESS)
                                {
                                    if (p_record->percent_complete < percent_cmplt)
                                        percent_cmplt = p_record->percent_complete;
                                    not_done = 1;
                                }
                                else if (p_record->status != IPR_CMD_STATUS_SUCCESSFUL)
                                {
                                    rc = RC_FAILED;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        unpost_form(p_form);

        if (!not_done)
        {
            if (done_bad)
            {
                rc =  RC_FAILED;
                goto leave;
            }

            check_current_config(false);
            revalidate_devs(NULL);
            rc =  RC_SUCCESS;
            mvprintw(8, 44, "Complete");
            form_driver(p_form,
                        REQ_FIRST_FIELD);
            refresh();
            sleep(2);
            goto leave;
        }
        not_done = 0;
        sleep(2);
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}

int hot_spare(int action)
{
/* iSeries
                          Configure a hot spare device

Select the subsystems which disk units will be configured as hot spares

Type choice, press Enter.
  1=Select subsystem

          Serial                Resource
Option    Number   Type  Model  Name
         02137002  2757   001   /dev/ipr5
         03060044  5703   001   /dev/ipr4
         02127005  5703   001   /dev/ipr3

e=Exit   q=Cancel    f=PageDn   b=PageUp
*/
/* non-iSeries
                          Configure a hot spare device

Select the subsystems which disk units will be configured as hot spares

Type choice, press Enter.
  1=Select subsystem

         Vendor    Product            Serial     PCI    PCI
Option    ID         ID               Number     Bus    Dev
         IBM       2780001           3AFB348C     05     03
         IBM       5703001           A775915E     04     01


e=Exit   q=Cancel    f=PageDn   b=PageUp
*/
    int rc, array_num, i, last_char;
    struct ipr_record_common *p_common_record;
    struct ipr_array_record *p_array_record;
    struct ipr_device_record *p_device_record;
    struct array_cmd_data *p_cur_raid_cmd;
    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    char buffer[100];
    int array_index;
    int max_size = curses_info.max_y - 12;
    int confirm_hot_spare(int action);
    int select_hot_spare(struct array_cmd_data *p_cur_raid_cmd, int action);
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    int found = 0;
    char *p_char;
    int ch;
    char title_str[64];
    char descript_str[128];
    char instruct_str[64];
    struct ipr_resource_entry *p_resource_entry;

    rc = RC_SUCCESS;

    array_num = 1;

    p_title_win = newwin(9, curses_info.max_x, 0, 0);
    p_menu_win = newwin(3, curses_info.max_x, max_size + 9, 0);
    p_field_win = newwin(max_size, curses_info.max_x, 9, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    if (action == IPR_ADD_HOT_SPARE)
    {
        sprintf(title_str,"Configure a hot spare device");
        sprintf(descript_str,"Select the subsystems which disk units will be configured as hot spares");
        sprintf(instruct_str,"Select subsystem");
    }
    else
    {
        sprintf(title_str,"Unconfigure a Hot Spare Device");
        sprintf(descript_str,"Select the subsystems which hot spares will be unconfigured");
        sprintf(instruct_str,"Select subsystem");
    }

    set_field_buffer(p_fields[0],     /* field to alter */
                     0,               /* number of buffer to alter */
                     title_str);      /* string value to set
                                                          */
    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    check_current_config(false);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        p_cur_ioa->num_raid_cmds = 0;

        for (i = 0;
             i < p_cur_ioa->num_devices;
             i++)
        {
            p_common_record = p_cur_ioa->dev[i].p_qac_entry;

            if (p_common_record == NULL)
                continue;

            p_array_record = (struct ipr_array_record *)p_common_record;
            p_device_record = (struct ipr_device_record *)p_common_record;

            if (((p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD) &&
                 (!p_array_record->established) &&
                 (action == IPR_ADD_HOT_SPARE)) ||
                ((p_common_record->record_id == IPR_RECORD_ID_DEVICE2_RECORD) &&
                 (p_device_record->is_hot_spare) &&
                 (action == IPR_RMV_HOT_SPARE)))
            {
                if (p_head_raid_cmd)
                {
                    p_tail_raid_cmd->p_next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                    p_tail_raid_cmd = p_tail_raid_cmd->p_next;
                }
                else
                {
                    p_head_raid_cmd = p_tail_raid_cmd = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                }

                p_tail_raid_cmd->p_next = NULL;

                memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

                p_tail_raid_cmd->array_id = 0;
                p_tail_raid_cmd->array_num = array_num;
                p_tail_raid_cmd->p_ipr_ioa = p_cur_ioa;
                p_tail_raid_cmd->p_ipr_device = NULL;

                start_y = (array_num-1) % (max_size - 1);

                if ((num_lines > 0) && (start_y == 0))
                {
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, curses_info.max_x,
                                                      max_size - 1,
                                                      0,
                                                      0, 0);

                    set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                    set_field_buffer(pp_field[field_index], 0, "More... ");
                    set_field_userptr(pp_field[field_index], NULL);
                    field_opts_off(pp_field[field_index++],
                                   O_ACTIVE);
                }

                /* User entry field */
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, 1, start_y, 3, 0, 0);

                set_field_userptr(pp_field[field_index++], (char *)p_tail_raid_cmd);

                /* Description field */
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, curses_info.max_x - 9, start_y, 9, 0, 0);

                p_resource_entry = p_cur_ioa->ioa.p_resource_entry;

                if (platform == IPR_ISERIES)
                {
                    len = sprintf(buffer, "%8s  %x   001   %s",
                                  p_resource_entry->serial_num,
                                  p_resource_entry->type,
                                  p_cur_ioa->ioa.dev_name);
                }
                else
                {
                    char buf[100];

                    memcpy(buf, p_resource_entry->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
                    buf[IPR_VENDOR_ID_LEN] = '\0';

                    len = sprintf(buffer, "%-8s  ", buf);

                    memcpy(buf, p_resource_entry->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);
                    buf[IPR_PROD_ID_LEN] = '\0';

                    len += sprintf(buffer + len, "%-12s  ", buf);

                    len += sprintf(buffer + len, "%8s     ", p_resource_entry->serial_num);

                    len += sprintf(buffer + len, "%02d     %02d",
                                   p_resource_entry->pci_bus_number,
                                   p_resource_entry->pci_slot);
                }

                set_field_buffer(pp_field[field_index], 0, buffer);
                field_opts_off(pp_field[field_index], O_ACTIVE);
                set_field_userptr(pp_field[field_index++], NULL);

                memset(buffer, 0, 100);

                len = 0;
                num_lines++;
                array_num++;
                break;
            }
        }
    }

    if (array_num == 1)
    {
        rc = RC_NOOP;
        goto leave;
    }

    mvwaddstr(p_title_win, 2, 0, descript_str);
    mvwaddstr(p_title_win, 4, 0, "Type choice, press Enter.");
    mvwaddstr(p_title_win, 5, 0, "  1=");
    mvwaddstr(p_title_win, 5, 4, instruct_str);

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 7, 0, "          Serial                Resource");
        mvwaddstr(p_title_win, 8, 0, "Option    Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 7, 0, "         Vendor    Product            Serial     PCI    PCI");
        mvwaddstr(p_title_win, 8, 0, "Option    ID         ID               Number     Bus    Dev");
    }

    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");


    field_opts_off(p_fields[1], O_ACTIVE);

    array_index = ((max_size - 1) * 2) + 1;
    for (i = array_index; i < field_index; i += array_index)
        set_new_page(pp_field[i], TRUE);

    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
    pp_field[field_index] = NULL;

    flush_stdscr();

    p_list_form = new_form(pp_field);

    set_form_win(p_list_form, p_field_win);

    post_form(p_list_form);

    form_driver(p_list_form,
                REQ_FIRST_FIELD);

    while(1)
    {
        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        wnoutrefresh(p_field_win);
        doupdate();

        ch = getch();

        if (IS_EXIT_KEY(ch))
        {
            rc = RC_EXIT;
            goto leave;
        }
        else if (IS_CANCEL_KEY(ch))
        {
            rc = RC_CANCEL;
            goto leave;
        }
        else if (IS_PGDN_KEY(ch))
        {
            form_driver(p_list_form,
                        REQ_NEXT_PAGE);
            form_driver(p_list_form,
                        REQ_FIRST_FIELD);
        }
        else if (IS_PGUP_KEY(ch))
        {
            form_driver(p_list_form,
                        REQ_PREV_PAGE);
            form_driver(p_list_form,
                        REQ_FIRST_FIELD);
        }
        else if ((ch == KEY_DOWN) ||
                 (ch == '\t'))
        {
            form_driver(p_list_form,
                        REQ_NEXT_FIELD);
        }
        else if (ch == KEY_UP)
        {
            form_driver(p_list_form,
                        REQ_PREV_FIELD);
        }
        else if (IS_ENTER_KEY(ch))
        {
            found = 0;

            unpost_form(p_list_form);
            p_cur_raid_cmd = p_head_raid_cmd;

            for (i = 0; i < field_index; i++)
            {
                p_cur_raid_cmd = (struct array_cmd_data *)field_userptr(pp_field[i]);
                if (p_cur_raid_cmd != NULL)
                {
                    p_char = field_buffer(pp_field[i], 0);
                    if (strcmp(p_char, "1") == 0)
                    {
                        while(RC_REFRESH == (rc = select_hot_spare(p_cur_raid_cmd, action)))
                            clear();
                        if (rc == RC_EXIT)
                            goto leave;
                        if (rc == RC_CANCEL)
                        {
                            rc = RC_REFRESH;
                            goto leave;
                        }

                        found = 1;
                        if (!p_cur_raid_cmd->do_cmd)
                        {
                            p_cur_raid_cmd->do_cmd = 1;
                            p_cur_raid_cmd->p_ipr_ioa->num_raid_cmds++;
                        }
                    }
                    else
                    {
                        if (p_cur_raid_cmd->do_cmd)
                        {
                            p_cur_raid_cmd->do_cmd = 0;
                            p_cur_raid_cmd->p_ipr_ioa->num_raid_cmds--;
                        }
                    }
                }
            }

            if (found != 0)
            {
                /* Go to verification screen */
                unpost_form(p_form);
                rc = confirm_hot_spare(action);
                goto leave;
            }
            else
            {
                post_form(p_list_form);
                rc = 1;
            }     
        }
        else
        {
            /* Terminal is running under 5250 telnet */
            if (term_is_5250)
            {
                /* Send it to the form to be processed and flush this line */
                form_driver(p_list_form, ch);
                last_char = flush_line();
                if (IS_5250_CHAR(last_char))
                    ungetch(ENTER_KEY);
                else if (last_char != ERR)
                    ungetch(last_char);
            }
            else
            {
                form_driver(p_list_form, ch);
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(pp_field);

    p_cur_raid_cmd = p_head_raid_cmd;
    while(p_cur_raid_cmd)
    {
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }

    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int select_hot_spare(struct array_cmd_data *p_cur_raid_cmd,
                            int action)
{
/*
                 Select Disk Units to Enable as Hot Spare

Type option, press Enter.
1=Select

       Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
OPT     ID        ID              Number    Bus    Dev    Bus   ID   Lun
       IBM      DFHSS4W          0024A0B9    05     03      0     3    0
       IBM      DFHSS4W          00D38D0F    05     03      0     5    0
       IBM      DFHSS4W          00D32240    05     03      0     9    0
       IBM      DFHSS4W          00034563    05     03      0     6    0
       IBM      DFHSS4W          00D37A11    05     03      0     4    0

e=Exit   q=Cancel    f=PageDn   b=PageUp

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

                 Select Disk Units to Enable as Hot Spare

Type option, press Enter.
1=Select

              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI
OPT    Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun
       6607  0024A0B9    03      D01       05     03      0     3    0
       6607  00D38D0F    03      D02       05     03      0     5    0
       6607  00D32240    03      D03       05     03      0     9    0
       6607  00034563    03      D04       05     03      0     6    0
       6717  00D37A11    03      D05       05     03      0     4    0

e=Exit   q=Cancel    f=PageDn   b=PageUp


*/

    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    int rc, i, j, init_index, found, last_char;
    char buffer[100];
    int max_size = curses_info.max_y - 10;
    int num_devs = 0;
    struct ipr_ioa *p_cur_ioa;
    char *p_char;
    u32 len;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int field_index = 0;
    int start_y;
    struct ipr_device *p_ipr_device;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_device_record *p_device_record;
    char title_str[64];

    rc = RC_SUCCESS;

    p_title_win = newwin(7, curses_info.max_x,
                         0, 0);

    p_menu_win = newwin(3, curses_info.max_x,
                        max_size + 7, 0);

    p_field_win = newwin(max_size, curses_info.max_x, 7, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,  /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    if (action == IPR_ADD_HOT_SPARE)
    {
        sprintf(title_str, "Select Disk Units to Enable as Hot Spare");
    }
    else
    {
        sprintf(title_str, "Select Disk Units to Disable Hot Spare");
    }

    set_field_buffer(p_fields[0],    /* field to alter */
                     0,          /* number of buffer to alter */
                     title_str);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    mvwaddstr(p_title_win, 2, 0, "Type option, press Enter.");
    mvwaddstr(p_title_win, 3, 0, "  1=Select");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 5, 0,
                  "              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "OPT    Type   Number     ID    Position   Bus    Dev    Bus    ID   Lun");
    }
    else
    {
        mvwaddstr(p_title_win, 5, 0,
                  "       Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "OPT     ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }
    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

    for (j = 0; j < p_cur_ioa->num_devices; j++)
    {
        p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
        p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;

        if (!p_resource_entry->is_ioa_resource &&
            (p_device_record != NULL) &&
            (p_device_record->common.record_id == IPR_RECORD_ID_DEVICE2_RECORD) &&
            (((p_device_record->add_hot_spare_cand) && (action == IPR_ADD_HOT_SPARE)) ||
             ((p_device_record->rmv_hot_spare_cand) && (action == IPR_RMV_HOT_SPARE))))
        {
            len = print_dev_sel(p_resource_entry, buffer);
            start_y = num_devs % (max_size - 1);

            /* User entry field */
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, 1, start_y, 1, 0, 0);
            
            set_field_userptr(pp_field[field_index++], (char *)&p_cur_ioa->dev[j]);

            /* Description field */
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, curses_info.max_x - 7,   /* new field size */ 
                                              start_y, 7, 0, 0);

            set_field_buffer(pp_field[field_index], 0, buffer);

            field_opts_off(pp_field[field_index], /* field to alter */
                           O_ACTIVE);             /* attributes to turn off */ 

            set_field_userptr(pp_field[field_index++], NULL);

            if ((num_devs > 0) && ((num_devs % (max_size - 2)) == 0))
            {
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, curses_info.max_x,
                                                  start_y + 1,
                                                  0,
                                                  0, 0);

                set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                set_field_buffer(pp_field[field_index], 0, "More... ");
                set_field_userptr(pp_field[field_index], NULL);
                field_opts_off(pp_field[field_index++],
                               O_ACTIVE);
            }

            memset(buffer, 0, 100);

            len = 0;
            num_devs++;
        }
    }

    if (num_devs)
    {
        field_opts_off(p_fields[1], O_ACTIVE);

        init_index = ((max_size - 1) * 2) + 1;
        for (i = init_index; i < field_index; i += init_index)
            set_new_page(pp_field[i], TRUE);

        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = NULL;

        p_list_form = new_form(pp_field);

        set_form_win(p_list_form, p_field_win);

        post_form(p_list_form);

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);

        form_driver(p_list_form,
                    REQ_FIRST_FIELD);
        wnoutrefresh(p_field_win);
        doupdate();

        while (1)
        {
            wnoutrefresh(stdscr);
            wnoutrefresh(p_menu_win);
            wnoutrefresh(p_title_win);
            wnoutrefresh(p_field_win);
            doupdate();

            for (;;)
            {
                int ch = getch();

                if (IS_EXIT_KEY(ch))
                {
                    rc = RC_EXIT;
                    goto leave;
                }
                else if (IS_CANCEL_KEY(ch))
                {
                    rc = RC_CANCEL;
                    goto leave;
                }
                else if (ch == KEY_DOWN)
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else if (ch == KEY_UP)
                {
                    form_driver(p_list_form,
                                REQ_PREV_FIELD);
                    break;
                }
                else if (IS_ENTER_KEY(ch))
                {
                    found = 0;

                    unpost_form(p_list_form);
                    mvwaddstr(p_menu_win, 2, 1, "                                           ");
                    wrefresh(p_menu_win);

                    for (i = 0; i < field_index; i++)
                    {
                        p_ipr_device = (struct ipr_device *)field_userptr(pp_field[i]);
                        if (p_ipr_device != NULL)
                        {
                            p_char = field_buffer(pp_field[i], 0);
                            if (strcmp(p_char, "1") == 0)
                            {
                                found = 1;
                                p_device_record = (struct ipr_device_record *)p_ipr_device->p_qac_entry;
                                p_device_record->issue_cmd = 1;
                            }
                        }
                    }

                    if (found != 0)
                    {
                        /* Go to parameters screen */
                        unpost_form(p_form);
                    }
                    else
                    {
                        post_form(p_list_form);
                        break;
                    }

                    goto leave;
                }
                else if (IS_PGDN_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_NEXT_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (IS_PGUP_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_PREV_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (ch == '\t')
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else
                {
                    /* Terminal is running under 5250 telnet */
                    if (term_is_5250)
                    {
                        /* Send it to the form to be processed and flush this line */
                        form_driver(p_list_form, ch);
                        last_char = flush_line();
                        if (IS_5250_CHAR(last_char))
                            ungetch(ENTER_KEY);
                        else if (last_char != ERR)
                            ungetch(last_char);
                    }
                    else
                    {
                        form_driver(p_list_form, ch);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        /* No devices to initialize */
        free_field(p_fields[0]);
        return RC_NOOP;
    }
    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;

    free_screen(NULL, NULL, p_fields);
    free_screen(NULL, NULL, pp_field);
    free(pp_field);
    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    flush_stdscr();
    return rc;
}

int confirm_hot_spare(int action)
{
#define CONFIRM_START_HDR_SIZE      10
#define CONFIRM_START_MENU_SIZE     3
/*
                            Confirm Enable Hot Spare        

ATTENTION: Existing data on these disk units will not be preserved.


Press Enter to continue.              
Press q=Cancel to return and change your choice.

        Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
Option   ID        ID              Number    Bus    Dev    Bus   ID   Lun
        IBM      DFHSS4W          0024A0B9    05     03      0     3    0
        IBM      DFHSS4W          00D38D0F    05     03      0     5    0
        IBM      DFHSS4W          00D32240    05     03      0     9    0
        IBM      DFHSS4W          00034563    05     03      0     6    0
        IBM      DFHSS4W          00D37A11    05     03      0     4    0

 q=Cancel    f=PageDn   b=PageUp

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx


                            Confirm Enable Hot Spare        

ATTENTION: Existing data on these disk units will not be preserved.


Press Enter to continue.              
Press q=Cancel to return and change your choice.

               Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI
Option  Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun
        6607  0024A0B9    03      D01       05     03      0     3    0
        6607  00D38D0F    03      D02       05     03      0     5    0
        6607  00D32240    03      D03       05     03      0     9    0
        6607  00034563    03      D04       05     03      0     6    0
        6717  00D37A11    03      D05       05     03      0     4    0

 q=Cancel    f=PageDn   b=PageUp



*/
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    struct ipr_resource_entry *p_resource_entry;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_device_record *p_device_record;
    struct ipr_ioa *p_cur_ioa;
    int rc, j;
    u16 len = 0;
    char buffer[100];
    u32 num_lines = 0;
    int max_size = curses_info.max_y - (CONFIRM_START_HDR_SIZE + CONFIRM_START_MENU_SIZE + 1);
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    PANEL *p_panel;
    int hot_spare_complete();
    char title_str[64];

    p_title_win = newwin(CONFIRM_START_HDR_SIZE,
                         curses_info.max_x,
                         curses_info.start_y,
                         curses_info.start_x);

    p_menu_win = newwin(CONFIRM_START_MENU_SIZE,
                        curses_info.max_x,
                        curses_info.start_y + max_size + CONFIRM_START_HDR_SIZE + 1,
                        curses_info.start_x);

    rc = RC_SUCCESS;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    if (action == IPR_ADD_HOT_SPARE)
    {
        sprintf(title_str, "Confirm Enable Hot Spare");
    }
    else
    {
        sprintf(title_str, "Confirm Disable Hot Spare");
    }

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     title_str);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1,
                               curses_info.max_x,
                               curses_info.start_y + CONFIRM_START_HDR_SIZE,
                               curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    wattron(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 0, "ATTENTION:");
    wattroff(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 10, " Existing data on these disk units will not be preserved.");
    mvwaddstr(p_title_win, 5, 0, "Press Enter to continue.");
    mvwaddstr(p_title_win, 6, 0, "Press "CANCEL_KEY_LABEL"=Cancel to return and change your choice.");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 8, 0,
                  "               Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 9, 0,
                  "Option  Type   Number     ID    Position   Bus    Dev    Bus    ID   Lun");
    }
    else
    {
        mvwaddstr(p_title_win, 8, 0,
                  "        Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 9, 0,
                  "Option   ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }

    mvwaddstr(p_menu_win, 1, 0, CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_raid_cmd = p_head_raid_cmd;

    while(p_cur_raid_cmd)
    {
        if (p_cur_raid_cmd->do_cmd)
        {
            p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

            for (j = 0; j < p_cur_ioa->num_devices; j++)
            {
                p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
                p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;

                if ((p_device_record != NULL) &&
                    (p_device_record->common.record_id == IPR_RECORD_ID_DEVICE2_RECORD) &&
                    (p_device_record->issue_cmd))
                {
                    len = sprintf(buffer, "    1   ");
                    print_dev_sel(p_resource_entry, buffer + len);

                    if ((num_lines > 0) && ((num_lines % max_size) == 0))
                    {
                        mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                        p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                        p_tail_win = p_tail_win->p_next;

                        p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                        p_tail_panel = p_tail_panel->p_next;

                        p_tail_win->p_win = newwin(max_size + 1,
                                                   curses_info.max_x,
                                                   curses_info.start_y + CONFIRM_START_HDR_SIZE,
                                                   curses_info.start_x);
                        p_tail_win->p_next = NULL;

                        p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                        p_tail_panel->p_next = NULL;
                    }

                    mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
                    memset(buffer, 0, 100);
                    num_lines++;
                }
            }
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        wnoutrefresh(stdscr);
        update_panels();
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_REFRESH;
                goto leave;
            }
            else if (IS_ENTER_KEY(ch))
            {
                /* verify the devices are not in use */
                p_cur_raid_cmd = p_head_raid_cmd;
                while (p_cur_raid_cmd)
                {
                    if (p_cur_raid_cmd->do_cmd)
                    {
                        p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
                        if (rc != 0)
                        {
                            rc = RC_FAILED;
                            syslog(LOG_ERR, "Devices may have changed state. Command cancelled, "
                                   "please issue commands again if RAID still desired %s."
                                   IPR_EOL, p_cur_ioa->ioa.dev_name);
                            goto leave;
                        }
                    }
                    p_cur_raid_cmd = p_cur_raid_cmd->p_next;
                }

                unpost_form(p_form);
                free_form(p_form);
                curses_info.p_form = p_old_form;
                free_screen(p_head_panel, p_head_win, p_fields);
                delwin(p_title_win);
                delwin(p_menu_win);
                werase(stdscr);
                rc = hot_spare_complete(action);
                return rc;
            }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else
            {
                break;
            }
        }

    }

    leave:
        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}

int hot_spare_complete(int action)
{
    /*
         no screen is displayed for this action 
     */
#define FORMAT_COMPLETE_HDR_SIZE        7
    int rc;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    int fd;
    struct ipr_ioa *p_cur_ioa;
    struct array_cmd_data *p_cur_raid_cmd;

    /* now issue the start array command with "known to be zero" */
    for (p_cur_raid_cmd = p_head_raid_cmd;
         p_cur_raid_cmd != NULL; 
         p_cur_raid_cmd = p_cur_raid_cmd->p_next)
    {
        if (p_cur_raid_cmd->do_cmd == 0)
            continue;

        p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;

        if (p_cur_ioa->num_raid_cmds > 0)
        {
            fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
            if (fd <= 1)
            {
                rc = RC_FAILED;
                syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                return rc;
            }

            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

            ioa_cmd.buffer = p_cur_ioa->p_qac_data;
            ioa_cmd.buffer_len = iprtoh16(p_cur_ioa->p_qac_data->resp_len);
            ioa_cmd.cdb[1] = 0x01; /* configure Hot Spare */
            if (action == IPR_ADD_HOT_SPARE)
                rc = ipr_ioctl(fd, IPR_START_ARRAY_PROTECTION, &ioa_cmd);
            else
                rc = ipr_ioctl(fd, IPR_STOP_ARRAY_PROTECTION, &ioa_cmd);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Start Array Protection to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                rc = RC_FAILED;
                close(fd);
                return rc;
            }
            close(fd);
        }
    }

    return rc;
}

int reclaim_cache()
{
#define RECLAIM_HDR_SIZE		13
#define RECLAIM_MENU_SIZE		4
    /*
                       Reclaim IOA Cache Storage

     Select the IOA to reclaim IOA cache storage.

     ATTENTION: Proceed with this function only if directed to from a
     service procedure.  Data in the IOA cache will be discarded.
     Damaged objects may result on the system.

     Type choice, press Enter.
     1=Reclaim IOA cache storage

                              Serial     Resource
     Option    Type   Model   Number     Name
               2748    001   09228053    /dev/ipr0


     e=Exit   q=Cancel
     */
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form, *p_list_form;
    int fd, rc, i, len, last_char;
    struct ipr_reclaim_query_data *p_reclaim_buffer, *p_cur_reclaim_buffer;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    int num_lines = 0;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    struct ipr_ioa *p_cur_ioa, *p_reclaim_ioa;
    int max_size = curses_info.max_y - (RECLAIM_HDR_SIZE + RECLAIM_MENU_SIZE);
    int num_needed = 0;
    int ioa_num = 0;
    char buffer[100], *p_char;

    rc = RC_SUCCESS;

    p_reclaim_buffer = (struct ipr_reclaim_query_data*)malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

    p_title_win = newwin(RECLAIM_HDR_SIZE, curses_info.max_x, 0, 0);
    p_menu_win = newwin(RECLAIM_MENU_SIZE, curses_info.max_x, max_size + RECLAIM_HDR_SIZE, 0);
    p_field_win = newwin(max_size, curses_info.max_x, RECLAIM_HDR_SIZE, 0);

    clear();

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],                       /* field to alter */
                     0,                                 /* number of buffer to alter */
                     "Reclaim IOA Cache Storage");  /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    mvwaddstr(p_title_win, 2, 0, "Select the IOA to reclaim IOA cache storage.");
    wattron(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 4, 0, "ATTENTION:");
    wattroff(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 4, 10, " Proceed with this function only if directed to from a");
    mvwaddstr(p_title_win, 5, 0, "service procedure.  Data in the IOA cache will be discarded.");
    mvwaddstr(p_title_win, 6, 0, "Damaged objects may result on the system.");
    mvwaddstr(p_title_win, 8, 0, "Type choice, press Enter.");
    mvwaddstr(p_title_win, 9, 0, "  1=Reclaim IOA cache storage");
    mvwaddstr(p_title_win, 11, 0, "                         Serial     Resource");
    mvwaddstr(p_title_win, 12, 0, "Option    Type   Model   Number     Name");
    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

    for (p_cur_ioa = p_head_ipr_ioa, p_cur_reclaim_buffer = p_reclaim_buffer;
         p_cur_ioa != NULL;
         p_cur_ioa = p_cur_ioa->p_next, p_cur_reclaim_buffer++)
    {
        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            continue;
        }

        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.buffer = p_cur_reclaim_buffer;
        ioa_cmd.buffer_len = sizeof(struct ipr_reclaim_query_data);
        ioa_cmd.cdb[1] = IPR_RECLAIM_QUERY;
        ioa_cmd.read_not_write = 1;

        rc = ipr_ioctl(fd, IPR_RECLAIM_CACHE_STORE, &ioa_cmd);

        if (rc != 0)
        {
            if (errno != EINVAL)
            {
                syslog(LOG_ERR, "Reclaim Query to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            }
            close(fd);
            p_cur_ioa->p_reclaim_data = NULL;
            continue;
        }

        close(fd);

        p_cur_ioa->p_reclaim_data = p_cur_reclaim_buffer;

        if (p_cur_reclaim_buffer->reclaim_known_needed ||
            p_cur_reclaim_buffer->reclaim_unknown_needed)
        {
            num_needed++;
        }
    }

    if (num_needed)
    {
        p_cur_ioa = p_head_ipr_ioa;

        while(p_cur_ioa)
        {
            if ((p_cur_ioa->p_reclaim_data) &&
                (p_cur_ioa->p_reclaim_data->reclaim_known_needed ||
                 p_cur_ioa->p_reclaim_data->reclaim_unknown_needed))
            {
                start_y = ioa_num % (max_size - 1);

                if ((num_lines > 0) && (start_y == 0))
                {
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));

                    pp_field[field_index] = new_field(1, curses_info.max_x,
                                                      max_size - 1, 0, 0, 0);

                    set_field_just(pp_field[field_index], JUSTIFY_RIGHT);

                    set_field_buffer(pp_field[field_index], 0, "More... ");

                    set_field_userptr(pp_field[field_index], NULL);

                    field_opts_off(pp_field[field_index++],
                                   O_ACTIVE);
                }

                /* User entry field */
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 2));

                pp_field[field_index] = new_field(1, 1, start_y, 3, 0, 0);

                set_field_userptr(pp_field[field_index++], (char *)p_cur_ioa);

                /* Description field */
                pp_field[field_index] = new_field(1, curses_info.max_x - RECLAIM_HDR_SIZE,
                                                  start_y, 10, 0, 0);

                len = sprintf(buffer, "%x    001   %8s    %s",
                              p_cur_ioa->ccin, p_cur_ioa->serial_num,
                              p_cur_ioa->ioa.dev_name);

                set_field_buffer(pp_field[field_index], 0, buffer);

                field_opts_off(pp_field[field_index], O_ACTIVE);

                set_field_userptr(pp_field[field_index++], NULL);

                memset(buffer, 0, 100);
                len = 0;
                num_lines++;
                ioa_num++;
            }
            p_cur_reclaim_buffer++;
            p_cur_ioa = p_cur_ioa->p_next;

        }

        field_opts_off(p_fields[1], O_ACTIVE);

        ioa_num = ((max_size - 1) * 2) + 1;
        for (i = ioa_num; i < field_index; i += ioa_num)
            set_new_page(pp_field[i], TRUE);

        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = NULL;

        p_list_form = new_form(pp_field);

        set_form_win(p_list_form, p_field_win);

        post_form(p_list_form);

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        form_driver(p_list_form,
                    REQ_FIRST_FIELD);
        wnoutrefresh(p_field_win);
        doupdate();


        while(1)
        {
            mvwaddstr(p_title_win, 2, 0, "Select the IOA to reclaim IOA cache storage.");
            wattron(p_title_win, A_BOLD);
            mvwaddstr(p_title_win, 4, 0, "ATTENTION:");
            wattroff(p_title_win, A_BOLD);
            mvwaddstr(p_title_win, 4, 10, " Proceed with this function only if directed to from a");
            mvwaddstr(p_title_win, 5, 0, "service procedure.  Data in the IOA cache will be discarded.");
            mvwaddstr(p_title_win, 6, 0, "Damaged objects may result on the system.");
            mvwaddstr(p_title_win, 8, 0, "Type choice, press Enter.");
            mvwaddstr(p_title_win, 9, 0, "  1=Reclaim IOA cache storage");
            mvwaddstr(p_title_win, 11, 0, "                         Serial     Resource");
            mvwaddstr(p_title_win, 12, 0, "Option    Type   Model   Number     Name");
            mvwaddstr(p_menu_win, 1, 0,
                      EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

            wnoutrefresh(stdscr);
            wnoutrefresh(p_menu_win);
            wnoutrefresh(p_title_win);
            wnoutrefresh(p_field_win);
            doupdate();

            for (;;)
            {
                int ch = getch();

                if (IS_EXIT_KEY(ch))
                {
                    rc = RC_EXIT;
                    goto leave;
                }
                else if (IS_CANCEL_KEY(ch))
                {
                    rc = RC_CANCEL;
                    goto leave;
                }
                else if (IS_PGDN_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_NEXT_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (IS_PGUP_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_PREV_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if ((ch == KEY_DOWN) ||
                         (ch == '\t'))
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else if (ch == KEY_UP)
                {
                    form_driver(p_list_form,
                                REQ_PREV_FIELD);
                    break;
                }
                else if (IS_ENTER_KEY(ch))
                {
                    ioa_num = 0;

                    for (i = 0; i < field_index; i++)
                    {
                        p_cur_ioa = (struct ipr_ioa *)field_userptr(pp_field[i]);
                        if (p_cur_ioa != NULL)
                        {
                            p_char = field_buffer(pp_field[i], 0);

                            if (strcmp(p_char, "1") == 0)
                            {
                                if (ioa_num)
                                {
                                    ioa_num++;
                                    break;
                                }
                                else if (p_cur_ioa->p_reclaim_data->reclaim_known_needed ||
                                         p_cur_ioa->p_reclaim_data->reclaim_unknown_needed)
                                {
                                    p_reclaim_ioa = p_cur_ioa;
                                    ioa_num++;
                                }
                            }
                        }
                    }

                    if (ioa_num == 0)
                    {
                        wclear(p_menu_win);
                        break;
                    }
                    else if (ioa_num == 1)
                    {
                        unpost_form(p_list_form);
                        unpost_form(p_form);

                        rc = confirm_reclaim(p_reclaim_ioa);
                        if ((rc == RC_NOOP) ||
                            (rc == RC_CANCEL))
                        {
                            werase(p_title_win);
                            werase(p_menu_win);
                            post_form(p_form);
                            post_form(p_list_form);
                            form_driver(p_list_form,
                                        REQ_FIRST_FIELD);
                            break;
                        }
                        else
                        {
                            goto leave;
                        }
                    }
                    else
                    {
                        mvwaddstr(p_menu_win, 2, 0, " Error. You have selected more than one IOA.");
                        break;
                    }
                }
                else
                {
                    /* Terminal is running under 5250 telnet */
                    if (term_is_5250)
                    {
                        /* Send it to the form to be processed and flush this line */
                        form_driver(p_list_form, ch);
                        last_char = flush_line();
                        if (IS_5250_CHAR(last_char))
                            ungetch(ENTER_KEY);
                        else if (last_char != ERR)
                            ungetch(last_char);
                    }
                    else
                    {
                        form_driver(p_list_form, ch);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        rc = RC_NOOP;
        goto leave;
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(p_reclaim_buffer);
    free(pp_field);
    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int confirm_reclaim(struct ipr_ioa *p_reclaim_ioa)
{
    /*
                         Confirm Reclaim IOA Cache Storage

     The disk units that may be affected by the function are displayed.

     ATTENTION: Proceed with this function only if directed to from a
     service procedure.  Data in the IOA cache will be discarded.
     Filesystem corruption may result on the system.

     Press c=Confirm to reclaim cache storage.
     Press q=Cancel to return to change your choice.

               Serial                 Resource   
     Option    Number   Type   Model  Name   
        1    000564FD  6717   050   /dev/sda
        1    000585F1  6717   050   /dev/sdb
        1    000591FB  6717   050   /dev/sdc
        1    00056880  6717   050   /dev/sdd
        1    000595AD  6717   050   /dev/sde

     q=Cancel    c=Confirm reclaim
     */
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_query_res_state res_state;
    int fd, j, rc;
    u16 len;
    u32 num_lines = 0;
    char buffer[100];
    int max_size = curses_info.max_y - 16;
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    PANEL *p_panel;
    int confirm = 0;

    p_title_win = newwin(13, curses_info.max_x,
                         curses_info.start_y, curses_info.start_x);

    p_menu_win = newwin(3, curses_info.max_x,
                        curses_info.start_y + max_size + 13, curses_info.start_x);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Confirm Reclaim IOA Cache Storage");

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1, curses_info.max_x,
                               curses_info.start_y + 13, curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    mvwaddstr(p_title_win, 2, 0, "The disk units that may be affected by the function are displayed.");
    wattron(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 4, 0, "ATTENTION:");
    wattroff(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 4, 10, " Proceed with this function only if directed to from a");
    mvwaddstr(p_title_win, 5, 0, "service procedure.  Data in the IOA cache will be discarded.");
    mvwaddstr(p_title_win, 6, 0, "Filesystem corruption may result on the system.");
    mvwaddstr(p_title_win, 8, 0, "Press c=Confirm to reclaim cache storage.");
    mvwaddstr(p_title_win, 9, 0, "Press "CANCEL_KEY_LABEL"=Cancel to return to change your choice.");
    mvwaddstr(p_title_win, 11, 0, "          Serial                 Resource");
    mvwaddstr(p_title_win, 12, 0, "Option    Number   Type   Model  Name");

    mvwaddstr(p_menu_win, 1, 0, CANCEL_KEY_LABEL"=Cancel    c=Confirm reclaim    f=PageDn   b=PageUp");

    check_current_config(false);

    fd = open(p_reclaim_ioa->ioa.dev_name, O_RDWR);
    if (fd <= 1)
    {
        rc = RC_FAILED;
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
        goto leave;
    }

    for (j = 0; j < p_reclaim_ioa->num_devices; j++)
    {
        p_resource_entry = p_reclaim_ioa->dev[j].p_resource_entry;
        if (!IPR_IS_DASD_DEVICE(p_resource_entry->std_inq_data))
            continue;

        /* Do a query resource state to see whether or not the device will be
         affected by the operation */
        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.buffer = &res_state;
        ioa_cmd.buffer_len = sizeof(res_state);
        ioa_cmd.read_not_write = 1;
        ioa_cmd.device_cmd = 1;
        ioa_cmd.resource_address = p_resource_entry->resource_address;

        memset(&res_state, 0, sizeof(res_state));

        rc = ipr_ioctl(fd, IPR_QUERY_RESOURCE_STATE, &ioa_cmd);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Query Resource State to a device (0x%08X) failed. %m"IPR_EOL,
                   IPR_GET_PHYSICAL_LOCATOR(p_resource_entry->resource_address));
            res_state.not_oper = 1;
        }

        if (!res_state.read_write_prot)
            continue;

        len = sprintf(buffer, "    1    ");
        len += print_dev(p_resource_entry, buffer + len, p_reclaim_ioa->dev[j].dev_name);

        if ((num_lines > 0) && ((num_lines % max_size) == 0))
        {
            mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

            p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
            p_tail_win = p_tail_win->p_next;

            p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
            p_tail_panel = p_tail_panel->p_next;

            p_tail_win->p_win = newwin(max_size + 1, curses_info.max_x,
                                       curses_info.start_y + 13, curses_info.start_x);
            p_tail_win->p_next = NULL;

            p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
            p_tail_panel->p_next = NULL;
        }

        mvwaddstr(p_tail_win->p_win, num_lines % max_size, 0, buffer);
        memset(buffer, 0, 100);
        num_lines++;
    }

    close(fd);

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        wnoutrefresh(stdscr);
        update_panels();
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if ((ch == 'c') || (ch == 'C') || 
                     (((ch == 's') || (ch == 'S')) && (confirm == 1)))
            {
                if (p_reclaim_ioa->p_reclaim_data->reclaim_unknown_needed &&
                    ((confirm == 0) || (ch == 'c') || (ch == 'C')))
                {
                    confirm = 1;
                    unpost_form(p_form);
                    werase(p_title_win);
                    post_form(p_form);

                    wattron(p_title_win, A_STANDOUT);
                    mvwaddstr(p_title_win, 2, 0, "ATTENTION!!!   ATTENTION!!!   ATTENTION!!!   ATTENTION!!!");
                    wattroff(p_title_win, A_STANDOUT);
                    mvwaddstr(p_title_win, 4, 0, "Proceed with this function only if directed to from a");
                    mvwaddstr(p_title_win, 5, 0, "service procedure.  Data in the IOA cache will be discarded.");
                    mvwaddstr(p_title_win, 6, 0, "By proceeding you will be allowing ");
                    wattron(p_title_win, A_BOLD);
                    mvwaddstr(p_title_win, 6, 35, "UNKNOWN");
                    wattroff(p_title_win, A_BOLD);
                    mvwaddstr(p_title_win, 6, 42, " data loss.");
                    mvwaddstr(p_title_win, 7, 0, "This data loss may or may not be detected by the host operating system.");
                    wattron(p_title_win, A_UNDERLINE);
                    mvwaddstr(p_title_win, 8, 0, "Filesystem corruption may result on the system.");
                    wattroff(p_title_win, A_UNDERLINE);
                    mvwaddstr(p_title_win, 10, 0, "Press 's' to continue.");
                    mvwaddstr(p_title_win, 11, 0, "          Serial                 Resource");
                    mvwaddstr(p_title_win, 12, 0, "Option    Number   Type   Model  Name");
                    mvwaddstr(p_menu_win, 1, 0, CANCEL_KEY_LABEL"=Cancel    s=Confirm reclaim    f=PageDn   b=PageUp");

                    break;
                }
                else
                {
                    fd = open(p_reclaim_ioa->ioa.dev_name, O_RDWR);
                    if (fd <= 1)
                    {
                        rc = RC_FAILED;
                        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
                        goto leave;
                    }

                    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                    ioa_cmd.buffer = p_reclaim_ioa->p_reclaim_data;
                    ioa_cmd.buffer_len = sizeof(struct ipr_reclaim_query_data);
                    ioa_cmd.cdb[1] = IPR_RECLAIM_PERFORM;

                    if (p_reclaim_ioa->p_reclaim_data->reclaim_unknown_needed)
                        ioa_cmd.cdb[1] |= IPR_RECLAIM_UNKNOWN_PERM;

                    mvwaddstr(p_menu_win, 2, 0, "Please wait - reclaim in progress");

                    form_driver(p_form,               /* form to pass input to */
                                REQ_LAST_FIELD );     /* form request code */

                    wnoutrefresh(stdscr);
                    update_panels();
                    wnoutrefresh(p_menu_win);
                    wnoutrefresh(p_title_win);
                    doupdate();

                    rc = ipr_ioctl(fd, IPR_RECLAIM_CACHE_STORE, &ioa_cmd);

                    if (rc != 0)
                    {
                        syslog(LOG_ERR, "Reclaim to %s failed. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
                        close(fd);
                        rc = RC_FAILED;
                        goto leave;
                    }

                    close(fd);

                    rc = reclaim_result(p_reclaim_ioa);
                    goto leave;
                }
            }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else
            {
                break;
            }
        }
    }

    leave:
        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}

int reclaim_result(struct ipr_ioa *p_reclaim_ioa)
{
    /*
                   Reclaim IOA Cache Storage Results

     IOA cache storage reclaimation has completed.  Use the number
     of lost sectors to decide whether to restore data from the
     most recent save media or to continue with possible data loss.     

     Number of lost sectors . . . . . . . . . . :       40168

     Press Enter to continue.














     e=Exit   q=Cancel

     */
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    int rc, fd;
    struct ipr_ioctl_cmd_internal ioa_cmd;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Reclaim IOA Cache Storage Results");

    clear();
    refresh();

    flush_stdscr();

    post_form(p_form);

    fd = open(p_reclaim_ioa->ioa.dev_name, O_RDWR);

    if (fd <= 1)
    {
        rc = RC_FAILED;
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
        goto leave;
    }

    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

    memset(p_reclaim_ioa->p_reclaim_data, 0, sizeof(struct ipr_reclaim_query_data));
    ioa_cmd.buffer = p_reclaim_ioa->p_reclaim_data;
    ioa_cmd.buffer_len = sizeof(struct ipr_reclaim_query_data);
    ioa_cmd.cdb[1] = IPR_RECLAIM_QUERY;
    ioa_cmd.read_not_write = 1;

    rc = ipr_ioctl(fd, IPR_RECLAIM_CACHE_STORE, &ioa_cmd);

    close(fd);

    if (rc)
        goto leave;

    if (p_reclaim_ioa->p_reclaim_data->reclaim_known_performed)
    {
        mvaddstr(2, 0, "IOA cache storage reclaimation has completed.  Use the number");
        mvaddstr(3, 0, "of lost sectors to decide whether to restore data from the");
        mvaddstr(4, 0, "most recent save media or to continue with possible data loss.");
        if (p_reclaim_ioa->p_reclaim_data->num_blocks_needs_multiplier)
        {
            mvprintw(6, 0, "Number of lost sectors . . . . . . . . . . :%12d",
                     iprtoh16(p_reclaim_ioa->p_reclaim_data->num_blocks) *
                     IPR_RECLAIM_NUM_BLOCKS_MULTIPLIER);
        }
        else
        {
            mvprintw(6, 0, "Number of lost sectors . . . . . . . . . . :%12d",
                     iprtoh16(p_reclaim_ioa->p_reclaim_data->num_blocks));
        }
        mvaddstr(8, 0, "Press Enter to continue.");
    }
    else if (p_reclaim_ioa->p_reclaim_data->reclaim_unknown_performed)
    {
        mvaddstr(2, 0, "IOA cache storage reclaimation has completed.  The number of");
        mvaddstr(3, 0, "lost sectors could not be determined.");
        mvaddstr(5, 0, "Press Enter to continue.");
    }
    else
        goto leave;

    mvaddstr(curses_info.max_y - 2, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

    form_driver(p_form,               /* form to pass input to */
                REQ_LAST_FIELD );     /* form request code */

    for (;;)
    {
        int ch = getch();

        if (IS_EXIT_KEY(ch) || IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
        {
            rc = RC_SUCCESS;
            goto leave;
        }
        else
            mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
    }

    leave:
        fd = open(p_reclaim_ioa->ioa.dev_name, O_RDWR);

    if (fd <= 1)
    {
        rc = RC_FAILED;
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
    }
    else
    {
        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.buffer = p_reclaim_ioa->p_reclaim_data;
        ioa_cmd.buffer_len = sizeof(struct ipr_reclaim_query_data);
        ioa_cmd.cdb[1] = IPR_RECLAIM_RESET;

        rc = ipr_ioctl(fd, IPR_RECLAIM_CACHE_STORE, &ioa_cmd);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Reclaim reset to %s failed. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
            rc = RC_FAILED;
        }
        close(fd);
    }
    check_current_config(false);
    revalidate_devs(p_reclaim_ioa);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int configure_af_device(int action_code)
{
/*
                    Select Disk Units for Initialize and Format

Type option, press Enter.
  1=Select

       Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
OPT     ID        ID              Number    Bus    Dev    Bus   ID   Lun
       IBM      DFHSS4W          0024A0B9    05     03      0     3    0
       IBM      DFHSS4W          00D38D0F    05     03      0     5    0
       IBM      DFHSS4W          00D32240    05     03      0     9    0
       IBM      DFHSS4W          00034563    05     03      0     6    0
       IBM      DFHSS4W          00D37A11    05     03      0     4    0

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

                    Select Disk Units for Initialize and Format

Type option, press Enter.
  1=Select

              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI
OPT    Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun
       6607  0024A0B9    03      D01       05     03      0     3    0
       6607  00D38D0F    03      D02       05     03      0     5    0
       6607  00D32240    03      D03       05     03      0     9    0
       6607  00034563    03      D04       05     03      0     6    0
       6717  00D37A11    03      D05       05     03      0     4    0

*/

    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    int fd, rc, i, j, init_index, found, last_char;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_device_record *p_device_record;
    int can_init;
    int dev_type;
    int change_size;
    char buffer[100];
    int max_size = curses_info.max_y - 10;
    int num_devs = 0;
    struct ipr_ioa *p_cur_ioa;
    char  *p_char;
    u32 len;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int field_index = 0;
    int start_y;
    struct devs_to_init_t *p_cur_dev_to_init;
    u8 cdb[IPR_CCB_CDB_LEN];
    u8 ioctl_buffer[IOCTL_BUFFER_SIZE];
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    struct ipr_block_desc *p_block_desc;
    int status;
    struct sense_data_t sense_data;

    rc = RC_SUCCESS;

    p_title_win = newwin(7, curses_info.max_x,
                         0, 0);

    p_menu_win = newwin(3, curses_info.max_x,
                        max_size + 7, 0);

    p_field_win = newwin(max_size, curses_info.max_x, 7, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,  /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],    /* field to alter */
                     0,          /* number of buffer to alter */
                     "Select Disk Units for Initialize and Format");

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    mvwaddstr(p_title_win, 2, 0, "Type option, press Enter.");
    mvwaddstr(p_title_win, 3, 0, "  1=Select");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 5, 0,
                  "              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "OPT    Type   Number     ID    Position   Bus    Dev    Bus    ID   Lun");
    }
    else
    {
        mvwaddstr(p_title_win, 5, 0,
                  "       Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "OPT     ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }
    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    check_current_config(false);

    for (p_cur_ioa = p_head_ipr_ioa;
         p_cur_ioa != NULL;
         p_cur_ioa = p_cur_ioa->p_next)
    {
        if ((p_cur_ioa->p_qac_data) &&
            (p_cur_ioa->p_qac_data->num_records == 0))
            continue;

        for (j = 0; j < p_cur_ioa->num_devices; j++)
        {
            can_init = 1;
            p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

            /* If not a DASD, disallow format */
            if ((!IPR_IS_DASD_DEVICE(p_resource_entry->std_inq_data)) ||
                (p_resource_entry->is_hot_spare))
                continue;

            /* If Advanced Function DASD */
            if (ipr_is_af_dasd_device(p_resource_entry))
            {
                if (action_code == IPR_INCLUDE)
                    continue;

                p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;
                if (!p_device_record ||
                    (p_device_record->common.record_id != IPR_RECORD_ID_DEVICE2_RECORD))
                {
                    continue;
                }

                dev_type = IPR_AF_DASD_DEVICE;
                change_size  = IPR_512_BLOCK;

                /* We allow the user to format the drive if nobody is using it */
                if (p_cur_ioa->dev[j].opens != 0)
                {
                    syslog(LOG_ERR, "Format not allowed to %s, device in use"IPR_EOL, p_cur_ioa->dev[j].dev_name); 
                    continue;
                }

                if (p_resource_entry->is_array_member)
                {
                    if (p_device_record->no_cfgte_vol)
                        can_init = 1;
                    else
                        can_init = 0;
                }
                else
                {
                    can_init = p_resource_entry->format_allowed;
                }
            }
            else if (p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI) /* Is JBOD */
            {
                if (action_code == IPR_REMOVE)
                    continue;

                dev_type = IPR_JBOD_DASD_DEVICE;
                change_size = IPR_522_BLOCK;

                /* We allow the user to format the drive if nobody is using it, or
                 the device is read write protected. If the drive fails, then is replaced
                 concurrently it will be read write protected, but the system may still
                 have a use count for it. We need to allow the format to get the device
                 into a state where the system can use it */
                if (p_cur_ioa->dev[j].opens != 0)
                {
                    syslog(LOG_ERR, "Format not allowed to %s, device in use"IPR_EOL, p_cur_ioa->dev[j].dev_name); 
                    continue;
                }

                if (is_af_blocked(&p_cur_ioa->dev[j], 0))
                {
                    /* error log is posted in is_af_blocked() routine */
                    continue;
                }

                /* check if mode select with 522 block size will work, will be
                 set back to 512 if successfull otherwise will not be candidate. */
                memset(cdb, 0, IPR_CCB_CDB_LEN);

                cdb[0] = MODE_SELECT;
                cdb[1] = 0x10; /* PF = 1, SP = 0 */

                p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
                memset(ioctl_buffer, 0, 255);

                p_mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);

                p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr + 1);

                /* Setup block size */
                p_block_desc->block_length[0] = 0x00;
                p_block_desc->block_length[1] = 0x02;
                p_block_desc->block_length[2] = 0x0a;

                cdb[4] = sizeof(struct ipr_block_desc) +
                    sizeof(struct ipr_mode_parm_hdr);

                fd = open(p_cur_ioa->dev[j].gen_name, O_RDWR);

                if (fd <= 1)
                {
                    syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->dev[j].gen_name);
                    continue;
                }

                status = sg_ioctl(fd, cdb,
                                  &ioctl_buffer, cdb[4], SG_DXFER_TO_DEV,
                                  &sense_data, 30);            

                if (status)
                {
                    syslog(LOG_ERR, "Unable to change block size on device %s."IPR_EOL,
                           p_cur_ioa->dev[j].dev_name);
                    close(fd);
                    continue;
                }
                else
                {
                    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
                    memset(ioctl_buffer, 0, 255);

                    p_mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);

                    p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr + 1);

                    p_block_desc->block_length[0] = 0x00;
                    p_block_desc->block_length[1] = 0x02;
                    p_block_desc->block_length[2] = 0x00;

                    cdb[4] = sizeof(struct ipr_block_desc) +
                        sizeof(struct ipr_mode_parm_hdr);

                    status = sg_ioctl(fd, cdb,
                                      &ioctl_buffer, cdb[4], SG_DXFER_TO_DEV,
                                      &sense_data, 30);            

                    close(fd);
                    if (status)
                    {
                        syslog(LOG_ERR, "Unable to preserve block size on device %s."IPR_EOL,
                               p_cur_ioa->dev[j].dev_name);
                        continue;
                    }
                }

                can_init = p_resource_entry->format_allowed;
            }
            else
                continue;

            if (can_init)
            {
                if (p_head_dev_to_init)
                {
                    p_tail_dev_to_init->p_next = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));
                    p_tail_dev_to_init = p_tail_dev_to_init->p_next;
                }
                else
                    p_head_dev_to_init = p_tail_dev_to_init = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));

                memset(p_tail_dev_to_init, 0, sizeof(struct devs_to_init_t));

                p_tail_dev_to_init->p_ioa = p_cur_ioa;
                p_tail_dev_to_init->dev_type = dev_type;
                p_tail_dev_to_init->change_size = change_size;
                p_tail_dev_to_init->p_ipr_device = &p_cur_ioa->dev[j];

                len = print_dev_sel(p_resource_entry, buffer);

                start_y = num_devs % (max_size - 1);

                /* User entry field */
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, 1, start_y, 1, 0, 0);

                set_field_userptr(pp_field[field_index++], dev_field);

                /* Description field */
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, curses_info.max_x - 7,   /* new field size */ 
                                                  start_y, 7, 0, 0);

                set_field_buffer(pp_field[field_index], 0, buffer);

                field_opts_off(pp_field[field_index], /* field to alter */
                               O_ACTIVE);             /* attributes to turn off */ 

                set_field_userptr(pp_field[field_index++], NULL);

                if ((num_devs > 0) && ((num_devs % (max_size - 2)) == 0))
                {
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, curses_info.max_x,
                                                      start_y + 1,
                                                      0,
                                                      0, 0);

                    set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                    set_field_buffer(pp_field[field_index], 0, "More... ");
                    set_field_userptr(pp_field[field_index], NULL);
                    field_opts_off(pp_field[field_index++],
                                   O_ACTIVE);
                }

                memset(buffer, 0, 100);

                len = 0;
                num_devs++;
            }
        }
    }

    if (num_devs)
    {
        field_opts_off(p_fields[1], O_ACTIVE);

        init_index = ((max_size - 1) * 2) + 1;
        for (i = init_index; i < field_index; i += init_index)
            set_new_page(pp_field[i], TRUE);

        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = NULL;

        p_list_form = new_form(pp_field);

        set_form_win(p_list_form, p_field_win);

        post_form(p_list_form);

        flush_stdscr();
        clear();
        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);

        form_driver(p_list_form,
                    REQ_FIRST_FIELD);
        wnoutrefresh(p_field_win);
        doupdate();

        while (1)
        {
            wnoutrefresh(stdscr);
            wnoutrefresh(p_menu_win);
            wnoutrefresh(p_title_win);
            wnoutrefresh(p_field_win);
            doupdate();

            for (;;)
            {
                int ch = getch();

                if (IS_EXIT_KEY(ch))
                {
                    rc = RC_EXIT;
                    goto leave;
                }
                else if (IS_CANCEL_KEY(ch))
                {
                    rc = RC_CANCEL;
                    goto leave;
                }
                else if (ch == KEY_DOWN)
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else if (ch == KEY_UP)
                {
                    form_driver(p_list_form,
                                REQ_PREV_FIELD);
                    break;
                }
                else if (IS_ENTER_KEY(ch))
                {
                    found = 0;

                    unpost_form(p_list_form);
                    mvwaddstr(p_menu_win, 2, 1, "                                           ");
                    wrefresh(p_menu_win);
                    p_cur_dev_to_init = p_head_dev_to_init;

                    for (i = 0; i < field_index; i++)
                    {

                        if (field_userptr(pp_field[i]) != NULL)
                        {
                            p_char = field_buffer(pp_field[i], 0);
                            if (strcmp(p_char, "1") == 0)
                            {
                                found = 1;
                                p_cur_dev_to_init->do_init = 1;
                            }

                            p_cur_dev_to_init = p_cur_dev_to_init->p_next;
                        }
                    }

                    if (found != 0)
                    {
                        /* Go to verification screen */
                        unpost_form(p_form);
                        rc = confirm_init_device();
                    }
                    else
                    {
                        post_form(p_list_form);
                        break;
                    }

                    goto leave;
                }
                else if (IS_PGDN_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_NEXT_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (IS_PGUP_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_PREV_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (ch == '\t')
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else
                {
                    /* Terminal is running under 5250 telnet */
                    if (term_is_5250)
                    {
                        /* Send it to the form to be processed and flush this line */
                        form_driver(p_list_form, ch);
                        last_char = flush_line();
                        if (IS_5250_CHAR(last_char))
                            ungetch(ENTER_KEY);
                        else if (last_char != ERR)
                            ungetch(last_char);
                    }
                    else
                    {
                        form_driver(p_list_form, ch);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        /* No devices to initialize */
        free_field(p_fields[0]);
        return RC_NOOP;
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;

    p_cur_dev_to_init = p_head_dev_to_init;
    while(p_cur_dev_to_init)
    {
        p_cur_dev_to_init = p_cur_dev_to_init->p_next;
        free(p_head_dev_to_init);
        p_head_dev_to_init = p_cur_dev_to_init;
    }

    free_screen(NULL, NULL, p_fields);
    free_screen(NULL, NULL, pp_field);
    free(pp_field);
    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    flush_stdscr();
    return rc;
}

int init_device()
{
/*
                    Select Disk Units for Initialize and Format

Type option, press Enter.
  1=Select

       Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
OPT     ID        ID              Number    Bus    Dev    Bus   ID   Lun
       IBM      DFHSS4W          0024A0B9    05     03      0     3    0
       IBM      DFHSS4W          00D38D0F    05     03      0     5    0
       IBM      DFHSS4W          00D32240    05     03      0     9    0
       IBM      DFHSS4W          00034563    05     03      0     6    0
       IBM      DFHSS4W          00D37A11    05     03      0     4    0

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

                    Select Disk Units for Initialize and Format

Type option, press Enter.
  1=Select

              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI
OPT    Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun
       6607  0024A0B9    03      D01       05     03      0     3    0
       6607  00D38D0F    03      D02       05     03      0     5    0
       6607  00D32240    03      D03       05     03      0     9    0
       6607  00034563    03      D04       05     03      0     6    0
       6717  00D37A11    03      D05       05     03      0     4    0

*/

    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    int fd, rc, i, j, init_index, found, last_char;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_query_res_state res_state;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_device_record *p_device_record;
    int can_init;
    int dev_type;
    char buffer[100];
    int max_size = curses_info.max_y - 10;
    int num_devs = 0;
    struct ipr_ioa *p_cur_ioa;
    char  *p_char;
    u32 len;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int field_index = 0;
    int start_y;
    struct devs_to_init_t *p_cur_dev_to_init;

    rc = RC_SUCCESS;

    p_title_win = newwin(7, curses_info.max_x,
                         0, 0);

    p_menu_win = newwin(3, curses_info.max_x,
                        max_size + 7, 0);

    p_field_win = newwin(max_size, curses_info.max_x, 7, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,  /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],    /* field to alter */
                     0,          /* number of buffer to alter */
                     "Select Disk Units for Initialize and Format");

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    mvwaddstr(p_title_win, 2, 0, "Type option, press Enter.");
    mvwaddstr(p_title_win, 3, 0, "  1=Select");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 5, 0,
                  "              Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "OPT    Type   Number     ID    Position   Bus    Dev    Bus    ID   Lun");
    }
    else
    {
        mvwaddstr(p_title_win, 5, 0,
                  "       Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "OPT     ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }
    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    check_current_config(false);

    for (p_cur_ioa = p_head_ipr_ioa;
         p_cur_ioa != NULL;
         p_cur_ioa = p_cur_ioa->p_next)
    {
        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            continue;
        }

        for (j = 0; j < p_cur_ioa->num_devices; j++)
        {
            can_init = 1;
            p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

            /* If not a DASD, disallow format */
            if ((!IPR_IS_DASD_DEVICE(p_resource_entry->std_inq_data)) ||
                (p_resource_entry->is_hot_spare))
                continue;

            /* If Advanced Function DASD */
            if (ipr_is_af_dasd_device(p_resource_entry))
            {
                dev_type = IPR_AF_DASD_DEVICE;

                p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;
                if (p_device_record &&
                    (p_device_record->common.record_id == IPR_RECORD_ID_DEVICE2_RECORD))
                {
                    /* We allow the user to format the drive if nobody is using it */
                    if (p_cur_ioa->dev[j].opens != 0)
                    {
                        syslog(LOG_ERR,
                               "Format not allowed to %s, device in use"IPR_EOL,
                               p_cur_ioa->dev[j].dev_name); 
                        continue;
                    }

                    if (p_resource_entry->is_array_member)
                    {
                        if (p_device_record->no_cfgte_vol)
                            can_init = 1;
                        else
                            can_init = 0;
                    }
                    else
                    {
                        can_init = p_resource_entry->format_allowed;
                    }
                }
                else
                {
                    /* Do a query resource state */
                    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                    ioa_cmd.buffer = &res_state;
                    ioa_cmd.buffer_len = sizeof(res_state);
                    ioa_cmd.read_not_write = 1;
                    ioa_cmd.device_cmd  = 1;
                    ioa_cmd.resource_address = p_resource_entry->resource_address;

                    memset(&res_state, 0, sizeof(res_state));

                    rc = ipr_ioctl(fd, IPR_QUERY_RESOURCE_STATE, &ioa_cmd);

                    if (rc != 0)
                    {
                        syslog(LOG_ERR, "Query Resource State to %s failed. %m"IPR_EOL, p_cur_ioa->dev[j].dev_name);
                        continue;
                    }

                    /* We allow the user to format the drive if 
                     the device is read write protected. If the drive fails, then is replaced
                     concurrently it will be read write protected, but the system may still
                     have a use count for it. We need to allow the format to get the device
                     into a state where the system can use it */
                    if ((p_cur_ioa->dev[j].opens != 0) && (!res_state.read_write_prot))
                    {
                        syslog(LOG_ERR, "Format not allowed to %s, device in use"IPR_EOL, p_cur_ioa->dev[j].dev_name); 
                        continue;
                    }

                    if (p_resource_entry->is_array_member)
                    {
                        /* If the device is an array member, only allow a format to it */
                        /* if it is read/write protected by the IOA */
                        if ((res_state.read_write_prot) && !(res_state.not_ready))
                            can_init = 1;
                        else
                            can_init = 0;
                    }
                    else if ((res_state.not_oper) || (res_state.not_ready))
                    {
                        /* Device is failed - cannot talk to the device */
                        can_init = 0;
                    }
                    else
                    {
                        can_init = p_resource_entry->format_allowed;
                    }
                }
            }
            else if (p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI) /* Is JBOD */
            {
                /* We allow the user to format the drive if nobody is using it */
                if (p_cur_ioa->dev[j].opens != 0)
                {
                    syslog(LOG_ERR,
                           "Format not allowed to %s, device in use"IPR_EOL,
                           p_cur_ioa->dev[j].dev_name); 
                    continue;
                }

                dev_type = IPR_JBOD_DASD_DEVICE;
                can_init = p_resource_entry->format_allowed;
            }
            else
                continue;

            if (can_init)
            {
                if (p_head_dev_to_init)
                {
                    p_tail_dev_to_init->p_next = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));
                    p_tail_dev_to_init = p_tail_dev_to_init->p_next;
                }
                else
                    p_head_dev_to_init = p_tail_dev_to_init = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));

                memset(p_tail_dev_to_init, 0, sizeof(struct devs_to_init_t));

                p_tail_dev_to_init->p_ioa = p_cur_ioa;
                p_tail_dev_to_init->dev_type = dev_type;
                p_tail_dev_to_init->p_ipr_device = &p_cur_ioa->dev[j];
                p_tail_dev_to_init->change_size = 0;

                len = print_dev_sel(p_resource_entry, buffer);

                start_y = num_devs % (max_size - 1);

                /* User entry field */
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, 1, start_y, 1, 0, 0);

                set_field_userptr(pp_field[field_index++], (char *)&p_cur_ioa->dev[j]);

                /* Description field */
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                pp_field[field_index] = new_field(1, curses_info.max_x - 7,   /* new field size */ 
                                                  start_y, 7, 0, 0);

                set_field_buffer(pp_field[field_index], 0, buffer);

                field_opts_off(pp_field[field_index], /* field to alter */
                               O_ACTIVE);             /* attributes to turn off */ 

                set_field_userptr(pp_field[field_index++], NULL);

                if ((num_devs > 0) && ((num_devs % (max_size - 2)) == 0))
                {
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
                    pp_field[field_index] = new_field(1, curses_info.max_x,
                                                      start_y + 1,
                                                      0,
                                                      0, 0);

                    set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                    set_field_buffer(pp_field[field_index], 0, "More... ");
                    set_field_userptr(pp_field[field_index], NULL);
                    field_opts_off(pp_field[field_index++],
                                   O_ACTIVE);
                }

                memset(buffer, 0, 100);

                len = 0;
                num_devs++;
            }
        }
        close(fd);
    }

    if (num_devs)
    {
        field_opts_off(p_fields[1], O_ACTIVE);

        init_index = ((max_size - 1) * 2) + 1;
        for (i = init_index; i < field_index; i += init_index)
            set_new_page(pp_field[i], TRUE);

        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = NULL;

        p_list_form = new_form(pp_field);

        set_form_win(p_list_form, p_field_win);

        post_form(p_list_form);

        flush_stdscr();
        clear();
        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);

        form_driver(p_list_form,
                    REQ_FIRST_FIELD);
        wnoutrefresh(p_field_win);
        doupdate();

        while (1)
        {
            wnoutrefresh(stdscr);
            wnoutrefresh(p_menu_win);
            wnoutrefresh(p_title_win);
            wnoutrefresh(p_field_win);
            doupdate();

            for (;;)
            {
                int ch = getch();

                if (IS_EXIT_KEY(ch))
                {
                    rc = RC_EXIT;
                    goto leave;
                }
                else if (IS_CANCEL_KEY(ch))
                {
                    rc = RC_CANCEL;
                    goto leave;
                }
                else if (ch == KEY_DOWN)
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else if (ch == KEY_UP)
                {
                    form_driver(p_list_form,
                                REQ_PREV_FIELD);
                    break;
                }
                else if (IS_ENTER_KEY(ch))
                {
                    found = 0;

                    unpost_form(p_list_form);
                    mvwaddstr(p_menu_win, 2, 1, "                                           ");
                    wrefresh(p_menu_win);
                    p_cur_dev_to_init = p_head_dev_to_init;

                    for (i = 0; i < field_index; i++)
                    {

                        if (field_userptr(pp_field[i]) != NULL)
                        {
                            p_char = field_buffer(pp_field[i], 0);
                            if (strcmp(p_char, "1") == 0)
                            {
                                found = 1;
                                p_cur_dev_to_init->do_init = 1;
                            }

                            p_cur_dev_to_init = p_cur_dev_to_init->p_next;
                        }
                    }

                    if (found != 0)
                    {
                        /* Go to verification screen */
                        unpost_form(p_form);
                        rc = confirm_init_device();
                    }
                    else
                    {
                        post_form(p_list_form);
                        break;
                    }

                    goto leave;
                }
                else if (IS_PGDN_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_NEXT_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (IS_PGUP_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_PREV_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (ch == '\t')
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else
                {
                    /* Terminal is running under 5250 telnet */
                    if (term_is_5250)
                    {
                        /* Send it to the form to be processed and flush this line */
                        form_driver(p_list_form, ch);
                        last_char = flush_line();
                        if (IS_5250_CHAR(last_char))
                            ungetch(ENTER_KEY);
                        else if (last_char != ERR)
                            ungetch(last_char);
                    }
                    else
                    {
                        form_driver(p_list_form, ch);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        /* No devices to initialize */
        free_field(p_fields[0]);
        return RC_NOOP;
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;

    p_cur_dev_to_init = p_head_dev_to_init;
    while(p_cur_dev_to_init)
    {
        p_cur_dev_to_init = p_cur_dev_to_init->p_next;
        free(p_head_dev_to_init);
        p_head_dev_to_init = p_cur_dev_to_init;
    }

    free_screen(NULL, NULL, p_fields);
    free_screen(NULL, NULL, pp_field);
    free(pp_field);
    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    flush_stdscr();
    return rc;
}

#define MAX_CMD_LENGTH 1000

void log_menu()
{
    /*
                            Kernel Messages Log

     Select one of the following:

     1. Use vi to view most recent IBM Storage error messages
     2. Use vi to view IBM Storage error messages
     3. Use vi to view all kernel error messages
     4. Use vi to view iprconfig error messages
     5. Set root kernel message log directory
     6. Set default editor
     7. Restore defaults
     8. Use vi to view IBM boot time messages





     Selection:

     e=Exit  q=Cancel

     */
    FIELD *p_fields[3], *p_input_fields[4];
    FORM *p_form, *p_input_form;
    FORM *p_old_form;
    int invalid = 0;
    char cmnd[MAX_CMD_LENGTH];
    int rc, i, j;
    DIR *p_dir;
    int dir_changed, editor_changed;
    char short_editor[200], status[200];
    char *p_ch;
    struct dirent **log_files;
    struct dirent **pp_dirent;
    int num_dir_entries;
    int len, ch, show_op7;
    struct stat file_stat;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 2,   /* new field size */ 
                            12, 12,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    status[0] = '\0';

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Kernel Messages Log");          /* string value to set */
    post_form(p_form);

    sprintf(cmnd,"%s/boot.msg",log_root_dir);
    rc = stat(cmnd, &file_stat);
    if (rc)
    {	/* file does not exist */
        show_op7 = 0;
    }
    else
    {
        show_op7 = 1;
    }


    while(1)
    {
        strcpy(short_editor, editor);
        p_ch = strchr(short_editor, ' ');
        if (p_ch)
            *p_ch = '\0';

        mvaddstr(2, 0, "Select one of the following:");
        mvprintw(4, 0, "     1. Use %s to view most recent IBM Storage error messages", short_editor);
        mvprintw(5, 0, "     2. Use %s to view IBM Storage error messages", short_editor);
        mvprintw(6, 0, "     3. Use %s to view all kernel error messages", short_editor);
        mvprintw(7, 0, "     4. Use %s to view iprconfig error messages", short_editor);
        mvaddstr(8, 0, "     5. Set root kernel message log directory");
        mvaddstr(9, 0, "     6. Set default editor");
        mvaddstr(10, 0, "     7. Restore defaults");
        if (show_op7)
        {
            mvprintw(11,0, "     8. Use %s to view IBM Storage boot time messages", short_editor);
        }
        mvaddstr(12, 0, "Selection: ");
        mvaddstr(curses_info.max_y - 2, 0, EXIT_KEY_LABEL"=Exit  "CANCEL_KEY_LABEL"=Cancel");

        if (status[0])
        {
            mvprintw(curses_info.max_y - 1, 0, status);
            status[0] = '\0';
        }

        if (invalid)
        {
            invalid = 0;
            mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
        }

        flush_stdscr();

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        refresh();
        for (;;)
        {
            ch = getch();

            if (IS_ENTER_KEY(ch))
            {
                char *p_char;
                refresh();
                unpost_form(p_form);
                p_char = field_buffer(p_fields[1], 0);

                switch(p_char[0])
                {
                    case '1':
                        refresh();
                        def_prog_mode();
                        endwin();
                        rc = system("rm -rf /tmp/.ipr.err; mkdir /tmp/.ipr.err");

                        if (rc != 0)
                        {
                            len = sprintf(cmnd, "cd %s; zcat -f messages", log_root_dir);

                            len += sprintf(cmnd + len," | grep ipr-err | ");
                            len += sprintf(cmnd + len, "sed \"s/ `hostname -s` kernel: ipr-err//g\" | ");
                            len += sprintf(cmnd + len, "sed \"s/ localhost kernel: ipr-err//g\" | ");
                            len += sprintf(cmnd + len, "sed \"s/ kernel: ipr-err//g\" | ");
                            len += sprintf(cmnd + len, "sed \"s/ eldrad//g\" | "); /* xx */
                            len += sprintf(cmnd + len, "sed \"s/\\^M//g\" | ");
                            len += sprintf(cmnd + len, FAILSAFE_EDITOR);
                            closelog();
                            openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
                            syslog(LOG_ERR, "Error encountered concatenating log files..."IPR_EOL);
                            syslog(LOG_ERR, "Using failsafe editor..."IPR_EOL);
                            closelog();
                            openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
                            sleep(3);
                        }
                        else
                        {
                            len = sprintf(cmnd,"cd %s; zcat -f messages | grep ipr-err |", log_root_dir);
                            len += sprintf(cmnd + len, "sed \"s/ `hostname -s` kernel: ipr-err//g\"");
                            len += sprintf(cmnd + len, " | sed \"s/ localhost kernel: ipr-err//g\"");
                            len += sprintf(cmnd + len, " | sed \"s/ kernel: ipr-err//g\"");
                            len += sprintf(cmnd + len, "| sed \"s/ eldrad//g\""); /* xx */
                            len += sprintf(cmnd + len, " | sed \"s/\\^M//g\" ");
                            len += sprintf(cmnd + len, ">> /tmp/.ipr.err/errors");
                            system(cmnd);

                            sprintf(cmnd, "%s /tmp/.ipr.err/errors", editor);
                        }
                        rc = system(cmnd);

                        mvaddstr(curses_info.max_y - 1, 0, "                        ");
                        post_form(p_form);
                        if ((rc != 0) && (rc != 127))
                            sprintf(status, " Editor returned %d. Try setting the default editor", rc);

                        break;
                    case '2':
                        refresh();
                        def_prog_mode();
                        endwin();
                        num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
                        rc = system("rm -rf /tmp/.ipr.err; mkdir /tmp/.ipr.err");
                        if (num_dir_entries)
                            pp_dirent = log_files;

                        if (rc != 0)
                        {
                            len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

                            /* Probably have a read-only root file system */
                            for (i = 0; i < num_dir_entries; i++)
                            {
                                len += sprintf(cmnd + len, "%s ", (*pp_dirent)->d_name);
                                pp_dirent++;
                            }

                            len += sprintf(cmnd + len," | grep ipr-err | ");
                            len += sprintf(cmnd + len, "sed \"s/ `hostname -s` kernel: ipr-err//g\" | ");
                            len += sprintf(cmnd + len, "sed \"s/ localhost kernel: ipr-err//g\" | ");
                            len += sprintf(cmnd + len, "sed \"s/ kernel: ipr-err//g\" | ");
                            len += sprintf(cmnd + len, "sed \"s/ eldrad//g\" | "); /* xx */
                            len += sprintf(cmnd + len, "sed \"s/\\^M//g\" | ");
                            len += sprintf(cmnd + len, FAILSAFE_EDITOR);
                            closelog();
                            openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
                            syslog(LOG_ERR, "Error encountered concatenating log files..."IPR_EOL);
                            syslog(LOG_ERR, "Using failsafe editor..."IPR_EOL);
                            closelog();
                            openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
                            sleep(3);
                        }
                        else
                        {
                            for (i = 0; i < num_dir_entries; i++)
                            {
                                len = sprintf(cmnd,"cd %s; zcat -f %s | grep ipr-err |", log_root_dir, (*pp_dirent)->d_name);
                                len += sprintf(cmnd + len, "sed \"s/ `hostname -s` kernel: ipr-err//g\"");
                                len += sprintf(cmnd + len, " | sed \"s/ localhost kernel: ipr-err//g\"");
                                len += sprintf(cmnd + len, " | sed \"s/ kernel: ipr-err//g\"");
                                len += sprintf(cmnd + len, "| sed \"s/ eldrad//g\""); /* xx */
                                len += sprintf(cmnd + len, " | sed \"s/\\^M//g\" ");
                                len += sprintf(cmnd + len, ">> /tmp/.ipr.err/errors");
                                system(cmnd);
                                pp_dirent++;
                            }

                            sprintf(cmnd, "%s /tmp/.ipr.err/errors", editor);
                        }
                        rc = system(cmnd);

                        free(*log_files);
                        mvaddstr(curses_info.max_y - 1, 0, "                        ");
                        post_form(p_form);
                        if ((rc != 0) && (rc != 127))
                            sprintf(status, " Editor returned %d. Try setting the default editor", rc);

                        break;
                    case '3':
                        refresh();
                        def_prog_mode();
                        endwin();
                        num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
                        if (num_dir_entries)
                            pp_dirent = log_files;

                        rc = system("rm -rf /tmp/.ipr.err; mkdir /tmp/.ipr.err");

                        if (rc != 0)
                        {
                            /* Probably have a read-only root file system */
                            len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

                            /* Probably have a read-only root file system */
                            for (i = 0; i < num_dir_entries; i++)
                            {
                                len += sprintf(cmnd + len, "%s ", (*pp_dirent)->d_name);
                                pp_dirent++;
                            }

                            len += sprintf(cmnd + len, " | sed \"s/\\^M//g\" ");
                            len += sprintf(cmnd + len, "| %s", FAILSAFE_EDITOR);
                            closelog();
                            openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
                            syslog(LOG_ERR, "Error encountered concatenating log files..."IPR_EOL);
                            syslog(LOG_ERR, "Using failsafe editor..."IPR_EOL);
                            openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
                            sleep(3);
                        }
                        else
                        {
                            for (i = 0; i < num_dir_entries; i++)
                            {
                                sprintf(cmnd,
                                        "cd %s; zcat -f %s | sed \"s/\\^M//g\" >> /tmp/.ipr.err/errors",
                                        log_root_dir, (*pp_dirent)->d_name);
                                system(cmnd);
                                pp_dirent++;
                            }

                            sprintf(cmnd, "%s /tmp/.ipr.err/errors", editor);
                        }

                        rc = system(cmnd);
                        free(*log_files);
                        mvaddstr(curses_info.max_y - 1, 0, "                        ");
                        post_form(p_form);
                        if ((rc != 0) && (rc != 127))
                            sprintf(status, " Editor returned %d. Try setting the default editor", rc);
                        break;
                    case '4':
                        refresh();
                        def_prog_mode();
                        endwin();
                        num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
                        rc = system("rm -rf /tmp/.ipr.err; mkdir /tmp/.ipr.err");
                        if (num_dir_entries)
                            pp_dirent = log_files;

                        if (rc != 0)
                        {
                            len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

                            /* Probably have a read-only root file system */
                            for (i = 0; i < num_dir_entries; i++)
                            {
                                len += sprintf(cmnd + len, "%s ", (*pp_dirent)->d_name);
                                pp_dirent++;
                            }

                            len += sprintf(cmnd + len," | grep iprconfig | ");
                            len += sprintf(cmnd + len, FAILSAFE_EDITOR);
                            closelog();
                            openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
                            syslog(LOG_ERR, "Error encountered concatenating log files..."IPR_EOL);
                            syslog(LOG_ERR, "Using failsafe editor..."IPR_EOL);
                            openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
                            sleep(3);
                        }
                        else
                        {
                            for (i = 0; i < num_dir_entries; i++)
                            {
                                len = sprintf(cmnd,"cd %s; zcat -f %s | grep iprconfig ", log_root_dir, (*pp_dirent)->d_name);
                                len += sprintf(cmnd + len, ">> /tmp/.ipr.err/errors");
                                system(cmnd);
                                pp_dirent++;
                            }

                            sprintf(cmnd, "%s /tmp/.ipr.err/errors", editor);
                        }
                        rc = system(cmnd);
                        free(*log_files);

                        mvaddstr(curses_info.max_y - 1, 0, "                        ");
                        post_form(p_form);
                        if ((rc != 0) && (rc != 127))
                            sprintf(status, " Editor returned %d. Try setting the default editor", rc);

                        break;
                    case '5':
                        dir_changed = 0;

                        /* Title */
                        p_input_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                                                      0, 0,       /* upper left corner */
                                                      0,           /* number of offscreen rows */
                                                      0);               /* number of working buffers */

                        /* User input field */
                        p_input_fields[1] = new_field(1, 40,   /* new field size */ 
                                                      11, 24,       /* upper left corner */
                                                      0,           /* number of offscreen rows */
                                                      0);          /* number of working buffers */

                        /* User input field */
                        p_input_fields[2] = new_field(1, 2,   /* new field size */ 
                                                      12, 0,       /* upper left corner */
                                                      0,           /* number of offscreen rows */
                                                      0);          /* number of working buffers */

                        p_input_fields[3] = NULL;

                        p_input_form = new_form(p_input_fields);

                        set_field_just(p_input_fields[0], JUSTIFY_CENTER);

                        field_opts_off(p_input_fields[0],          /* field to alter */
                                       O_ACTIVE);             /* attributes to turn off */ 

                        set_field_buffer(p_input_fields[0],        /* field to alter */
                                         0,          /* number of buffer to alter */
                                         "Kernel Messages Log Root Directory");          /* string value to set */

                        form_opts_off(p_input_form, O_BS_OVERLOAD);

                        flush_stdscr();

                        post_form(p_input_form);

                        mvprintw(10, 0, "Current root directory: %s", log_root_dir);
                        mvaddstr(11, 0, "New root directory: ");
                        mvaddstr(13, 0, "Enter new directory and press Enter");
                        mvaddstr(14, 0, "Or leave blank and press Enter to cancel");

                        set_current_field(p_input_form, p_input_fields[1]);
                        refresh();

                        while(1)
                        {
                            char *p_char, *p_tmp;
                            int len;
                            int ch = getch();

                            if (IS_ENTER_KEY(ch))
                            {
                                flush_stdscr();
                                form_driver(p_input_form, REQ_FIRST_FIELD );
                                unpost_form(p_input_form);
                                p_char = field_buffer(p_input_fields[1], 0);
                                len = strlen(p_char);
                                p_tmp = p_char + len - 1;
                                while(*p_tmp == ' ')
                                    p_tmp--;
                                p_tmp++;
                                *p_tmp = '\0';
                                p_dir = opendir(p_char);
                                if (p_char[0] == '\0')
                                {
                                    break;
                                }
                                else if (p_dir == NULL)
                                {
                                    post_form(p_input_form);
                                    mvprintw(10, 0, "Current root directory: %s", log_root_dir);
                                    mvaddstr(11, 0, "New root directory: ");
                                    mvaddstr(13, 0, "Enter new directory and press Enter");
                                    mvaddstr(14, 0, "Or leave blank and press Enter to cancel");
                                    mvprintw(curses_info.max_y - 1, 0, " Invalid directory %s", p_char);
                                    set_current_field(p_input_form, p_input_fields[1]);

                                    refresh();
                                }
                                else
                                {
                                    closedir(p_dir);
                                    if (strcmp(log_root_dir, p_char) != 0)
                                    {
                                        werase(stdscr);
                                        set_field_buffer(p_input_fields[0],        /* field to alter */
                                                         0,          /* number of buffer to alter */
                                                         "Confirm Root Directory Change");          /* string value to set */

                                        post_form(p_input_form);
                                        mvprintw(11, 0, "New root directory: %s                                                ", p_char);
                                        mvprintw(curses_info.max_y - 2, 0,
                                                 "c=Confirm    "CANCEL_KEY_LABEL"=Cancel");
                                        set_current_field(p_input_form, p_input_fields[2]);

                                        refresh();

                                        while(1)
                                        {
                                            int ch = getch();
                                            if ((ch == 'c') || (ch == 'C'))
                                            {
                                                strcpy(log_root_dir, p_char);
                                                dir_changed = 1;
                                                break;
                                            }
                                            else if (IS_CANCEL_KEY(ch))
                                            {
                                                dir_changed = 0;
                                                break;
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                            else if (ch == KEY_RIGHT)
                                form_driver(p_input_form, REQ_NEXT_CHAR);
                            else if (ch == KEY_LEFT)
                                form_driver(p_input_form, REQ_PREV_CHAR);
                            else if ((ch == KEY_BACKSPACE) || (ch == 127))
                                form_driver(p_input_form, REQ_DEL_PREV);
                            else if (ch == KEY_DC)
                                form_driver(p_input_form, REQ_DEL_CHAR);
                            else if (ch == KEY_IC)
                                form_driver(p_input_form, REQ_INS_MODE);
                            else if (ch == KEY_EIC)
                                form_driver(p_input_form, REQ_OVL_MODE);
                            else if (ch == KEY_HOME)
                                form_driver(p_input_form, REQ_BEG_FIELD);
                            else if (ch == KEY_END)
                                form_driver(p_input_form, REQ_END_FIELD);
                            else if (isascii(ch))
                                form_driver(p_input_form, ch);
                            refresh();
                        }
                        unpost_form(p_input_form);
                        free_form(p_input_form);
                        free_screen(NULL, NULL, p_input_fields);
                        if (rc == RC_EXIT)
                            goto leave;
                        post_form(p_form);
                        if (dir_changed)
                            sprintf(status, " Root directory changed to %s", log_root_dir);
                        else
                            sprintf(status, " Root directory unchanged");
                        break;
                    case '6':
                        editor_changed = 0;

                        /* Title */
                        p_input_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                                                      0, 0,       /* upper left corner */
                                                      0,           /* number of offscreen rows */
                                                      0);               /* number of working buffers */

                        /* User input field */
                        p_input_fields[1] = new_field(1, 40,   /* new field size */ 
                                                      11, 16,       /* upper left corner */
                                                      0,           /* number of offscreen rows */
                                                      0);          /* number of working buffers */

                        /* User input field */
                        p_input_fields[2] = new_field(1, 2,   /* new field size */ 
                                                      12, 0,       /* upper left corner */
                                                      0,           /* number of offscreen rows */
                                                      0);          /* number of working buffers */

                        p_input_fields[3] = NULL;

                        p_input_form = new_form(p_input_fields);

                        set_field_just(p_input_fields[0], JUSTIFY_CENTER);

                        field_opts_off(p_input_fields[0],          /* field to alter */
                                       O_ACTIVE);             /* attributes to turn off */ 

                        set_field_buffer(p_input_fields[0],        /* field to alter */
                                         0,          /* number of buffer to alter */
                                         "Kernel Messages Editor");          /* string value to set */

                        form_opts_off(p_input_form, O_BS_OVERLOAD);

                        post_form(p_input_form);

                        mvprintw(10, 0, "Current editor: %s", editor);
                        mvaddstr(11, 0, "New editor: ");
                        mvaddstr(14, 0, "Enter new editor command and press Enter");
                        mvaddstr(15, 0, "Or leave blank and press Enter to cancel");
                        set_current_field(p_input_form, p_input_fields[1]);
                        refresh();

                        while(1)
                        {
                            char *p_char, *p_tmp;
                            int len;
                            int ch = getch();

                            if (IS_ENTER_KEY(ch))
                            {
                                form_driver(p_input_form, REQ_FIRST_FIELD );
                                unpost_form(p_input_form);
                                p_char = field_buffer(p_input_fields[1], 0);
                                len = strlen(p_char);
                                p_tmp = p_char + len - 1;
                                while(*p_tmp == ' ')
                                    p_tmp--;
                                p_tmp++;
                                *p_tmp = '\0';
                                if ((strcmp(editor, p_char) != 0) && (strlen(p_char) > 0))
                                {
                                    werase(stdscr);
                                    set_field_buffer(p_input_fields[0],        /* field to alter */
                                                     0,          /* number of buffer to alter */
                                                     "Confirm Editor Change");          /* string value to set */

                                    post_form(p_input_form);
                                    mvprintw(11, 0, "New editor: %s                                                ", p_char);
                                    mvprintw(curses_info.max_y - 2, 0,
                                             "c=Confirm    "CANCEL_KEY_LABEL"=Cancel");
                                    set_current_field(p_input_form, p_input_fields[2]); 
                                    refresh();

                                    while(1)
                                    {
                                        int ch = getch();
                                        if ((ch == 'c') || (ch == 'C'))
                                        {
                                            strcpy(editor, p_char);
                                            editor_changed = 1;
                                            break;
                                        }
                                        else if (IS_CANCEL_KEY(ch))
                                        {
                                            editor_changed = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                            else if (ch == KEY_RIGHT)
                                form_driver(p_input_form, REQ_NEXT_CHAR);
                            else if (ch == KEY_LEFT)
                                form_driver(p_input_form, REQ_PREV_CHAR);
                            else if ((ch == KEY_BACKSPACE) || (ch == 127))
                                form_driver(p_input_form, REQ_DEL_PREV);
                            else if (ch == KEY_DC)
                                form_driver(p_input_form, REQ_DEL_CHAR);
                            else if (ch == KEY_IC)
                                form_driver(p_input_form, REQ_INS_MODE);
                            else if (ch == KEY_EIC)
                                form_driver(p_input_form, REQ_OVL_MODE);
                            else if (ch == KEY_HOME)
                                form_driver(p_input_form, REQ_BEG_FIELD);
                            else if (ch == KEY_END)
                                form_driver(p_input_form, REQ_END_FIELD);
                            else if (isascii(ch))
                                form_driver(p_input_form, ch);
                            refresh();
                        }
                        unpost_form(p_input_form);
                        free_form(p_input_form);
                        free_screen(NULL, NULL, p_input_fields);
                        if (rc == RC_EXIT)
                            goto leave;
                        post_form(p_form);
                        if (editor_changed)
                            sprintf(status, " Editor changed to %s", editor);
                        else
                            sprintf(status, " Editor unchanged");
                        break;
                    case '7':
                        strcpy(log_root_dir, DEFAULT_LOG_DIR);
                        strcpy(editor, DEFAULT_EDITOR);
                        break;
                    case '8':
                        if (show_op7 == 0)
                        {
                            post_form(p_form);
                            if (isdigit(p_char[0]))
                                sprintf(status, "Invalid option specified.");
                            break;
                        }

                        refresh();
                        def_prog_mode();
                        endwin();
                        rc = system("rm -rf /tmp/.ipr.err; mkdir /tmp/.ipr.err");

                        if (rc != 0)
                        {
                            len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

                            /* Probably have a read-only root file system */
                            len += sprintf(cmnd + len, "boot.msg");
                            len += sprintf(cmnd + len," | grep ipr |");
                            len += sprintf(cmnd + len, FAILSAFE_EDITOR);
                            closelog();
                            openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
                            syslog(LOG_ERR, "Error encountered concatenating log files..."IPR_EOL);
                            syslog(LOG_ERR, "Using failsafe editor..."IPR_EOL);
                            closelog();
                            openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
                            sleep(3);
                        }
                        else
                        {
                            len = sprintf(cmnd,"cd %s; zcat -f boot.msg | grep ipr  | "
                                          "sed 's/<[0-9]>ipr-err: //g' | sed 's/<[0-9]>ipr: //g'",
                                          log_root_dir);
                            len += sprintf(cmnd + len, ">> /tmp/.ipr.err/errors");
                            system(cmnd);
                            sprintf(cmnd, "%s /tmp/.ipr.err/errors", editor);
                        }
                        rc = system(cmnd);

                        mvaddstr(curses_info.max_y - 1, 0, "                        ");
                        post_form(p_form);
                        if ((rc != 0) && (rc != 127))
                            sprintf(status, " Editor returned %d. Try setting the default editor", rc);

                        break;
                    default:
                        post_form(p_form);
                        if (isdigit(p_char[0]))
                            sprintf(status, "Invalid option specified.");
                        break;
                }
                flush_stdscr();
                unpost_form(p_form);
                clear();
                post_form(p_form);
                form_driver(p_form, ' ');
                break;
            }
            else if (IS_EXIT_KEY(ch))
            {
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                goto leave;
            }
            else
            {
                form_driver(p_form, ch);
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                refresh();
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    for (j=0; j < 2; j++)
        free_field(p_fields[j]);
    flush_stdscr();
    return;
}

int select_log_file(const struct dirent *p_dir_entry)
{
    if (p_dir_entry)
    {
        if (strstr(p_dir_entry->d_name, "messages") == p_dir_entry->d_name)
            return 1;
    }
    return 0;
}

int compare_log_file(const void *pp_f, const void *pp_s)
{
    char *p_first_start_num, *p_first_end_num;
    char *p_second_start_num, *p_second_end_num;
    struct dirent **pp_first, **pp_second;
    char name1[100], name2[100];
    int first, second;

    pp_first = (struct dirent **)pp_f;
    pp_second = (struct dirent **)pp_s;

    if (strcmp((*pp_first)->d_name, "messages") == 0)
        return 1;
    if (strcmp((*pp_second)->d_name, "messages") == 0)
        return -1;

    strcpy(name1, (*pp_first)->d_name);
    strcpy(name2, (*pp_second)->d_name);
    p_first_start_num = strchr(name1, '.');
    p_first_end_num = strrchr(name1, '.');
    p_second_start_num = strchr(name2, '.');
    p_second_end_num = strrchr(name2, '.');

    if (p_first_start_num == p_first_end_num)
    {
        /* Not compressed */
        p_first_end_num = name1 + strlen(name1);
        p_second_end_num = name2 + strlen(name2);
    }
    else
    {
        *p_first_end_num = '\0';
        *p_second_end_num = '\0';
    }
    first = strtoul(p_first_start_num, NULL, 10);
    second = strtoul(p_second_start_num, NULL, 10);

    if (first > second)
        return -1;
    else
        return 1;
}

int confirm_init_device()
{
/*
                            Confirm Initialize and Format Disk Unit        

Press 'c' to confirm your choice for 1=Initialize and format.
Press q to return to change your choice.

        Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
Option   ID        ID              Number    Bus    Dev    Bus   ID   Lun
        IBM      DFHSS4W          0024A0B9    05     03      0     3    0
        IBM      DFHSS4W          00D38D0F    05     03      0     5    0
        IBM      DFHSS4W          00D32240    05     03      0     9    0
        IBM      DFHSS4W          00034563    05     03      0     6    0
        IBM      DFHSS4W          00D37A11    05     03      0     4    0

c=Confirm   q=Cancel    f=PageDn   b=PageUp

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx


                            Confirm Initialize and Format Disk Unit        

Press 'c' to confirm your choice for 1=Initialize and format.
Press q to return to change your choice.

               Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI
Option  Type   Number     ID    Position   Bus    Dev    Bus   ID   Lun
        6607  0024A0B9    03      D01       05     03      0     3    0
        6607  00D38D0F    03      D02       05     03      0     5    0
        6607  00D32240    03      D03       05     03      0     9    0
        6607  00034563    03      D04       05     03      0     6    0
        6717  00D37A11    03      D05       05     03      0     4    0

c=Confirm   q=Cancel    f=PageDn   b=PageUp



*/

    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    char buffer[100];
    int max_size = curses_info.max_y - 7 - 3 - 1;
    struct window_l *p_head_win, *p_tail_win;
    struct panel_l *p_head_panel, *p_tail_panel, *p_cur_panel;
    WINDOW *p_title_win, *p_menu_win;
    struct devs_to_init_t *p_cur_dev_to_init;
    u32 len;
    u32 num_devs = 0;
    int rc = RC_SUCCESS;
    PANEL *p_panel;
    int send_dev_inits(u8 num_devs);

    p_title_win = newwin(7, curses_info.max_x,
                         curses_info.start_y, curses_info.start_x);

    p_menu_win = newwin(3, curses_info.max_x,
                        curses_info.start_y + max_size + 7 + 1, curses_info.start_x);


    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Confirm Initialize and Format Disk Unit");

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    p_head_win = p_tail_win = (struct window_l *)malloc(sizeof(struct window_l));
    p_head_panel = p_tail_panel = (struct panel_l *)malloc(sizeof(struct panel_l));

    p_head_win->p_win = newwin(max_size + 1, curses_info.max_x,
                               curses_info.start_y + 7, curses_info.start_x);
    p_head_win->p_next = NULL;

    p_head_panel->p_panel = new_panel(p_head_win->p_win);
    p_head_panel->p_next = NULL;

    mvwaddstr(p_title_win, 2, 0, "Press 'c' to confirm your choice for 1=Initialize and format.");
    mvwaddstr(p_title_win, 3, 0, "Press "CANCEL_KEY_LABEL" to return to change your choice.");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 5, 0,
                  "               Serial    Frame    Card     PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "Option  Type   Number     ID    Position   Bus    Dev    Bus    ID   Lun");
    }
    else
    {
        mvwaddstr(p_title_win, 5, 0,
                  "        Vendor    Product           Serial    PCI    PCI   SCSI  SCSI  SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "Option   ID         ID              Number    Bus    Dev    Bus    ID   Lun");
    }
    mvwaddstr(p_menu_win, 1, 0, "c=Confirm   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_dev_to_init = p_head_dev_to_init;

    while(p_cur_dev_to_init)
    {
        if (p_cur_dev_to_init->do_init)
        {
            len = sprintf(buffer, "    1   ");
            len += print_dev_sel(p_cur_dev_to_init->p_ipr_device->p_resource_entry,
                                 buffer + len);

            if ((num_devs > 0) && ((num_devs % max_size) == 0))
            {
                mvwaddstr(p_tail_win->p_win, max_size, curses_info.max_x - 8, "More...");

                p_tail_win->p_next = (struct window_l *)malloc(sizeof(struct window_l));
                p_tail_win = p_tail_win->p_next;

                p_tail_panel->p_next = (struct panel_l *)malloc(sizeof(struct panel_l));
                p_tail_panel = p_tail_panel->p_next;

                p_tail_win->p_win = newwin(max_size + 1, curses_info.max_x,
                                           curses_info.start_y + 7, curses_info.start_x);
                p_tail_win->p_next = NULL;

                p_tail_panel->p_panel = new_panel(p_tail_win->p_win);
                p_tail_panel->p_next = NULL;
            }

            mvwaddstr(p_tail_win->p_win, num_devs % max_size, 0, buffer);
            memset(buffer, 0, 100);
            num_devs++;
        }
        p_cur_dev_to_init = p_cur_dev_to_init->p_next;
    }

    p_cur_panel = p_head_panel;

    while(p_cur_panel)
    {
        bottom_panel(p_cur_panel->p_panel);
        p_cur_panel = p_cur_panel->p_next;
    }

    form_driver(p_form,               /* form to pass input to */
                REQ_FIRST_FIELD );     /* form request code */

    wnoutrefresh(stdscr);
    update_panels();
    wnoutrefresh(p_menu_win);
    wnoutrefresh(p_title_win);
    doupdate();

    while (1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_FIRST_FIELD );     /* form request code */

        wnoutrefresh(stdscr);
        update_panels();
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_REFRESH;
                goto leave;
            }
            else if ((ch == 'c') || (ch == 'C'))
            {
                form_driver(p_form,               /* form to pass input to */
                            REQ_FIRST_FIELD );     /* form request code */

                wnoutrefresh(stdscr);
                update_panels();
                mvwaddstr(p_menu_win, 2, 0, " Please wait for the next screen.");
                wnoutrefresh(p_menu_win);
                wnoutrefresh(p_title_win);
                doupdate();
                rc = send_dev_inits(num_devs);
                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                p_panel = panel_below(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    bottom_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                p_panel = panel_above(NULL);
                if (p_panel != p_tail_panel->p_panel)
                {
                    top_panel(p_panel);
                    update_panels();
                    doupdate();
                }
                break;
            }
            else
            {
                break;
            }
        }
    }
    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(p_head_panel, p_head_win, p_fields);
    delwin(p_title_win);
    delwin(p_menu_win);
    flush_stdscr();
    return rc;
}

int send_dev_inits(u8 num_devs)
{
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct devs_to_init_t *p_cur_dev_to_init;
    int rc, fd, fdev, pid;
    struct ipr_query_res_state res_state;
    u8 ioctl_buffer[IOCTL_BUFFER_SIZE];
    u8 ioctl_buffer2[IOCTL_BUFFER_SIZE];
    struct ipr_control_mode_page *p_control_mode_page;
    struct ipr_control_mode_page *p_control_mode_page_changeable;
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    struct ipr_block_desc *p_block_desc;
    struct sense_data_t sense_data;
    struct ipr_resource_entry *p_resource_entry;
    int status, arg;
    int loop, len, opens;
    u8 remaining_devs = num_devs;
    u8 defect_list_hdr[4];
    u8 cdb[IPR_CCB_CDB_LEN];
    u8 *p_char;
    int is_520_block = 0;

    int dev_init_complete(u8 num_devs);

    for (p_cur_dev_to_init = p_head_dev_to_init;
         p_cur_dev_to_init != NULL;
         p_cur_dev_to_init = p_cur_dev_to_init->p_next)
    {
        if ((p_cur_dev_to_init->do_init) &&
            (p_cur_dev_to_init->dev_type == IPR_AF_DASD_DEVICE))
        {
            fd = open(p_cur_dev_to_init->p_ioa->ioa.dev_name, O_RDWR);

            if (fd < 0)
            {
                syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ioa->ioa.dev_name);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

            ioa_cmd.buffer = &res_state;
            ioa_cmd.buffer_len = sizeof(res_state);
            ioa_cmd.read_not_write = 1;
            ioa_cmd.device_cmd = 1;
            ioa_cmd.resource_address = p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address;

            memset(&res_state, 0, sizeof(res_state));

            rc = ipr_ioctl(fd, IPR_QUERY_RESOURCE_STATE, &ioa_cmd);

            if (rc != 0)
            {
                syslog(LOG_ERR,
                       "Query Resource State to %s failed. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            p_resource_entry = p_cur_dev_to_init->p_ipr_device->p_resource_entry;
            opens = num_device_opens(p_cur_dev_to_init->p_ioa->host_num,
                                     p_resource_entry->resource_address.bus,
                                     p_resource_entry->resource_address.target,
                                     p_resource_entry->resource_address.lun);

            if ((opens != 0) && (!res_state.read_write_prot))
            {
                syslog(LOG_ERR,
                       "Cannot obtain exclusive access to %s"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            if (!p_cur_dev_to_init->p_ipr_device->p_resource_entry->is_hidden)
            {
                fdev = open(p_cur_dev_to_init->p_ipr_device->dev_name, O_RDWR | O_NONBLOCK);

                if (fdev)
                {
                    fcntl(fdev, F_SETFD, 0);
                    if (flock(fdev, LOCK_EX))
                        syslog(LOG_ERR,
                               "Could not set lock for %s. %m"IPR_EOL,
                               p_cur_dev_to_init->p_ipr_device->dev_name);
                }

                p_cur_dev_to_init->fd = fdev;
            }

            /* Mode sense */
            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

            ioa_cmd.buffer = ioctl_buffer;
            ioa_cmd.buffer_len = 255;
            ioa_cmd.cdb[2] = 0x0a;
            ioa_cmd.read_not_write = 1;
            ioa_cmd.device_cmd = 1;
            ioa_cmd.resource_address = p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address;

            rc = ipr_ioctl(fd, MODE_SENSE, &ioa_cmd);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                iprlog_location(p_cur_dev_to_init->p_ipr_device->p_resource_entry);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            /* Mode sense - changeable parms */
            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

            ioa_cmd.buffer = ioctl_buffer2;
            ioa_cmd.buffer_len = 255;
            ioa_cmd.cdb[2] = 0x4a;
            ioa_cmd.read_not_write = 1;
            ioa_cmd.device_cmd = 1;
            ioa_cmd.resource_address = p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address;

            rc = ipr_ioctl(fd, MODE_SENSE, &ioa_cmd);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                iprlog_location(p_cur_dev_to_init->p_ipr_device->p_resource_entry);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

            p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr + 1);
            if ((p_block_desc->block_length[0] == 0x00) &&
                (p_block_desc->block_length[1] == 0x02) &&
                (p_block_desc->block_length[2] == 0x08))
                /* block size is 520 */
                is_520_block = 1;

            p_control_mode_page = (struct ipr_control_mode_page *)
                (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

            p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer2;

            p_control_mode_page_changeable = (struct ipr_control_mode_page *)
                (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

            p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

            /* Turn off QERR since some drives do not like QERR
             and IMMED bit at the same time. */
            IPR_SET_MODE(p_control_mode_page_changeable->qerr,
                            p_control_mode_page->qerr, 0);

            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

            ioa_cmd.buffer = ioctl_buffer;
            ioa_cmd.buffer_len = p_mode_parm_hdr->length + 1;
            p_mode_parm_hdr->length = 0;
            p_mode_parm_hdr->medium_type = 0;
            p_mode_parm_hdr->device_spec_parms = 0;
            p_control_mode_page->header.parms_saveable = 0;
            ioa_cmd.read_not_write = 0;
            ioa_cmd.device_cmd = 1;
            ioa_cmd.resource_address = p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address;
            rc = ipr_ioctl(fd, MODE_SELECT, &ioa_cmd);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Select %s to disable qerr failed. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                iprlog_location(p_cur_dev_to_init->p_ipr_device->p_resource_entry);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            /* Issue mode select to change block size */
            p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
            memset(ioctl_buffer, 0, 255);

            p_mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);

            p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr + 1);

            /* Setup block size */
            p_block_desc->block_length[0] = 0x00;
            p_block_desc->block_length[1] = 0x02;

            if (p_cur_dev_to_init->change_size == IPR_512_BLOCK)
                p_block_desc->block_length[2] = 0x00;
            else if (is_520_block)
                p_block_desc->block_length[2] = 0x08;
            else
                p_block_desc->block_length[2] = 0x0a;

            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

            ioa_cmd.buffer = ioctl_buffer;
            ioa_cmd.buffer_len = sizeof(struct ipr_block_desc) +
                sizeof(struct ipr_mode_parm_hdr);
            ioa_cmd.read_not_write = 0;
            ioa_cmd.device_cmd = 1;
            ioa_cmd.resource_address = p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address;
            rc = ipr_ioctl(fd, MODE_SELECT, &ioa_cmd);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Select to %s failed. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                iprlog_location(p_cur_dev_to_init->p_ipr_device->p_resource_entry);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            pid = fork();
            if (pid)
            {
                /* Do nothing */
            }
            else
            {
                /* Issue the format. Failure will be detected by query command status */
                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));
                ioa_cmd.buffer = NULL;
                ioa_cmd.buffer_len = 0;
                ioa_cmd.device_cmd = 1;
                ioa_cmd.resource_address = p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address;
                ipr_ioctl(fd, FORMAT_UNIT, &ioa_cmd);
                close(fd);
                exit(0);
            }
        }
        else if ((p_cur_dev_to_init->do_init) &&
                 (p_cur_dev_to_init->dev_type == IPR_JBOD_DASD_DEVICE))
        {
            p_resource_entry = p_cur_dev_to_init->p_ipr_device->p_resource_entry;
            opens = num_device_opens(p_cur_dev_to_init->p_ioa->host_num,
                                     p_resource_entry->resource_address.bus,
                                     p_resource_entry->resource_address.target,
                                     p_resource_entry->resource_address.lun);

            if (opens != 0)
            {
                syslog(LOG_ERR,
                       "Cannot obtain exclusive access to %s"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            memset(cdb, 0, IPR_CCB_CDB_LEN);

            /* Issue mode sense to get the control mode page */
            cdb[0] = MODE_SENSE;
            cdb[2] = 0x0a; /* PC = current values, page code = 0ah */
            cdb[4] = 255;

            fd = open(p_cur_dev_to_init->p_ipr_device->gen_name, O_RDWR);

            status = sg_ioctl(fd, cdb,
                              &ioctl_buffer, 255, SG_DXFER_FROM_DEV,
                              &sense_data, 30);

            if (status)
            {
                syslog(LOG_ERR,
                       "Could not send mode sense to %s. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->gen_name);
                iprlog_location(p_cur_dev_to_init->p_ipr_device->p_resource_entry);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            memset(cdb, 0, IPR_CCB_CDB_LEN);

            /* Issue mode sense to get the control mode page */
            cdb[0] = MODE_SENSE;
            cdb[2] = 0x4a; /* PC = changeable values, page code = 0ah */
            cdb[4] = 255;

            status = sg_ioctl(fd, cdb,
                              &ioctl_buffer2, 255, SG_DXFER_FROM_DEV,
                              &sense_data, 30);

            if (status)
            {
                syslog(LOG_ERR,
                       "Could not send mode sense to %s. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->gen_name);
                iprlog_location(p_cur_dev_to_init->p_ipr_device->p_resource_entry);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer2;

            p_control_mode_page_changeable = (struct ipr_control_mode_page *)
                (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

            p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

            p_control_mode_page = (struct ipr_control_mode_page *)
                (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

            /* Turn off QERR since some drives do not like QERR
             and IMMED bit at the same time. */
            IPR_SET_MODE(p_control_mode_page_changeable->qerr,
                            p_control_mode_page->qerr, 0);

            memset(cdb, 0, IPR_CCB_CDB_LEN);

            /* Issue mode select to set page x0A */
            cdb[0] = MODE_SELECT;
            cdb[1] = 0x10; /* PF = 1, SP = 0 */
            cdb[4] = p_mode_parm_hdr->length + 1;

            p_mode_parm_hdr->length = 0;
            p_control_mode_page->header.parms_saveable = 0;
            p_mode_parm_hdr->medium_type = 0;
            p_mode_parm_hdr->device_spec_parms = 0;

            status = sg_ioctl(fd, cdb,
                              &ioctl_buffer, cdb[4], SG_DXFER_TO_DEV,
                              &sense_data, 30);            

            if (status)
            {
                syslog(LOG_ERR,
                       "Could not send mode select to %s."IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->gen_name);
                iprlog_location(p_cur_dev_to_init->p_ipr_device->p_resource_entry);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            /* Issue mode select to setup block size */
            p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
            memset(ioctl_buffer, 0, 255);

            p_mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);

            p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr + 1);

            /* Setup block size */
            p_block_desc->block_length[0] = 0x00;
            p_block_desc->block_length[1] = 0x02;

            if (p_cur_dev_to_init->change_size == IPR_522_BLOCK)
                p_block_desc->block_length[2] = 0x0a;
            else
                p_block_desc->block_length[2] = 0x00;

            cdb[4] = sizeof(struct ipr_block_desc) +
                sizeof(struct ipr_mode_parm_hdr);

            status = sg_ioctl(fd, cdb,
                              &ioctl_buffer, cdb[4], SG_DXFER_TO_DEV,
                              &sense_data, 30);            

            if (status)
            {
                syslog(LOG_ERR, "Could not send mode select to %s to change blocksize."IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->gen_name);
                iprlog_location(p_cur_dev_to_init->p_ipr_device->p_resource_entry);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            /* Issue format */
            memset(defect_list_hdr, 0, sizeof(defect_list_hdr));
            memset(cdb, 0, IPR_CCB_CDB_LEN);

            defect_list_hdr[1] = 0x02; /* FOV = 0, DPRY = 0, DCRT = 0, STPF = 0, IP = 0, DSP = 0, Immed = 1, VS = 0 */

            /* Issue mode sense to get the control mode page */
            cdb[0] = FORMAT_UNIT;
            cdb[1] = 0x10; /* lun = 0, fmtdata = 1, cmplst = 0, defect list format = 0 */

            status = sg_ioctl(fd, cdb,
                              &defect_list_hdr, 4, SG_DXFER_TO_DEV,
                              &sense_data, 30);            

            if (status)
            {
                syslog(LOG_ERR, "Could not send format unit to %s."IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->gen_name);
                iprlog_location(p_cur_dev_to_init->p_ipr_device->p_resource_entry);
                len = 0;

                p_char = (u8 *)&sense_data;

                for (loop = 0; loop < 18; loop++)
                    len += sprintf(ioctl_buffer + len,"%.2x ", p_char[loop + 8]);

                syslog(LOG_ERR, "sense data = %s"IPR_EOL, ioctl_buffer);

                /* Send a device reset to cleanup any old state */
                arg = SG_SCSI_RESET_DEVICE;
                rc = ioctl(fd, SG_SCSI_RESET, &arg);

                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }
            close(fd);
            p_cur_dev_to_init->fd = 0;
        }
    }

    /* Here we need to sleep for a bit to make sure our children have started
     their formats. There isn't really a good way to know whether or not the formats
     have started, so we just wait for 5 seconds */
    sleep(5);

    if (remaining_devs)
        rc = dev_init_complete(remaining_devs);

    if (remaining_devs == num_devs)
        return rc;
    else
        return RC_FAILED;
}

int dev_init_complete(u8 num_devs)
{
    FIELD *p_fields[3];
    FORM *p_form;
    FORM *p_old_form;
    int done_bad;
    struct ipr_cmd_status cmd_status;
    struct ipr_cmd_status_record *p_record;
    int not_done = 0;
    int rc, j;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct devs_to_init_t *p_cur_dev_to_init;
    u32 percent_cmplt = 0;
    u8 ioctl_buffer[255];
    struct sense_data_t sense_data;
    u8 cdb[IPR_CCB_CDB_LEN], *p_char;
    int fd, fdev;
    struct ipr_ioa *p_ioa;
    int len, loop;
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    struct ipr_block_desc *p_block_desc;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            curses_info.max_y - 1, curses_info.max_x - 1, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Function Status");

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    while(1)
    {
        clear();
        post_form(p_form);

        mvaddstr(2, 0, "You selected to initialize and format a disk unit");
        mvprintw(curses_info.max_y / 2, (curses_info.max_x / 2) - 7, "%d%% Complete", percent_cmplt);

        form_driver(p_form,
                    REQ_FIRST_FIELD);

        refresh();

        percent_cmplt = 100;
        done_bad = 0;

        for (p_cur_dev_to_init = p_head_dev_to_init;
             p_cur_dev_to_init != NULL;
             p_cur_dev_to_init = p_cur_dev_to_init->p_next)
        {
            if ((p_cur_dev_to_init->do_init) &&
                (p_cur_dev_to_init->dev_type == IPR_AF_DASD_DEVICE))
            {
                fdev = p_cur_dev_to_init->fd; /* maintained for locking purposes */
                fd = open(p_cur_dev_to_init->p_ioa->ioa.dev_name, O_RDWR);

                if (fd < 0)
                {
                    syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_dev_to_init->p_ioa->ioa.dev_name);
                    done_bad = 1;
                    continue;
                }

                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                ioa_cmd.buffer = &cmd_status;
                ioa_cmd.buffer_len = sizeof(cmd_status);
                ioa_cmd.read_not_write = 1;
                ioa_cmd.device_cmd = 1;
                ioa_cmd.resource_address = p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address;

                rc = ipr_ioctl(fd, IPR_QUERY_COMMAND_STATUS, &ioa_cmd);

                close(fd);

                if (rc)
                {
                    syslog(LOG_ERR, "Query Command Status to %s failed. %m"IPR_EOL,
                           p_cur_dev_to_init->p_ipr_device->dev_name);
                    p_cur_dev_to_init->cmplt = 100;
                    continue;
                }

                if (cmd_status.num_records == 0)
                    p_cur_dev_to_init->cmplt = 100;
                else
                {
                    p_record = cmd_status.record;
                    if (p_record->status == IPR_CMD_STATUS_SUCCESSFUL)
                    {
                        p_cur_dev_to_init->cmplt = 100;
                    }
                    else if (p_record->status == IPR_CMD_STATUS_FAILED)
                    {
                        p_cur_dev_to_init->cmplt = 100;
                        done_bad = 1;
                    }
                    else
                    {
                        p_cur_dev_to_init->cmplt = p_record->percent_complete;
                        if (p_record->percent_complete < percent_cmplt)
                            percent_cmplt = p_record->percent_complete;
                        not_done = 1;
                    }
                }
            }
            else if ((p_cur_dev_to_init->do_init) &&
                     (p_cur_dev_to_init->dev_type == IPR_JBOD_DASD_DEVICE))
            {
                /* Send Test Unit Ready to find percent complete in sense data. */
                memset(cdb, 0, IPR_CCB_CDB_LEN);

                fdev = p_cur_dev_to_init->fd;

                if (fdev == 0)
                {
                    fdev = open(p_cur_dev_to_init->p_ipr_device->gen_name, O_RDWR);
                    p_cur_dev_to_init->fd = fdev;
                }

                if (fdev <= 1)
                {
                    syslog(LOG_ERR,
                           "Could not open %s. %m"IPR_EOL,
                           p_cur_dev_to_init->p_ipr_device->gen_name);
                    done_bad = 1;
                    continue;
                }

                cdb[0] = TEST_UNIT_READY;

                rc = sg_ioctl(fdev, cdb,
                              NULL, 0, SG_DXFER_NONE,
                              &sense_data, 30);


                if (rc < 0)
                {
                    syslog(LOG_ERR,
                           "Could not send test unit ready to %s. %m"IPR_EOL,
                           p_cur_dev_to_init->p_ipr_device->gen_name);
                    continue;
                }

                if ((rc == CHECK_CONDITION) &&
                    ((sense_data.error_code & 0x7F) == 0x70) &&
                    ((sense_data.sense_key & 0x0F) == 0x02))
                {
                    p_cur_dev_to_init->cmplt = ((int)sense_data.sense_key_spec[1]*100)/0x100;
                    if (p_cur_dev_to_init->cmplt < percent_cmplt)
                        percent_cmplt = p_cur_dev_to_init->cmplt;
                    not_done = 1;
                }
                else
                {
                    p_cur_dev_to_init->cmplt = 100;
                }
            }
        }

        unpost_form(p_form);

        if (!not_done)
        {
            p_cur_dev_to_init = p_head_dev_to_init;
            while(p_cur_dev_to_init)
            {
                if (p_cur_dev_to_init->do_init)
                {
                    p_ioa = p_cur_dev_to_init->p_ioa;

                    if (p_cur_dev_to_init->dev_type == IPR_AF_DASD_DEVICE)
                    {
                        /* check if evaluate device is necessary */
                        /* issue mode sense */
                        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));
                        p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

                        ioa_cmd.buffer = p_mode_parm_hdr;
                        ioa_cmd.buffer_len = 255;
                        ioa_cmd.read_not_write = 1;
                        ioa_cmd.device_cmd = 1;
                        ioa_cmd.resource_address = p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address;

                        fd = open(p_ioa->ioa.dev_name, O_RDWR);

                        if (fd < 0)
                        {
                            syslog(LOG_ERR, "Cannot open %s. %m\n", p_ioa->ioa.dev_name);
                        }
                        else
                        {
                            rc = ipr_ioctl(fd, MODE_SENSE, &ioa_cmd);

                            close(fd);

                            if (rc != 0)
                            {
                                syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
                                       p_cur_dev_to_init->p_ipr_device->dev_name);
                            }
                            else
                            {
                                if (p_mode_parm_hdr->block_desc_len > 0)
                                {
                                    p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr+1);

                                    if((p_block_desc->block_length[1] == 0x02) && 
                                       (p_block_desc->block_length[2] == 0x00))
                                    {
                                        p_cur_dev_to_init->change_size = IPR_512_BLOCK;
                                    }
                                }
                            }
                        }
                    }
                    else if (p_cur_dev_to_init->dev_type == IPR_JBOD_DASD_DEVICE)
                    {
                        fdev = p_cur_dev_to_init->fd;

                        if (fdev == 0)
                        {
                            fdev = open(p_cur_dev_to_init->p_ipr_device->gen_name, O_RDWR);
                            p_cur_dev_to_init->fd = fdev;
                        }

                        if (fdev <= 1)
                        {
                            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL,
                                   p_cur_dev_to_init->p_ipr_device->gen_name);
                        }
                        else
                        {
                            /* Check if evaluate device capabilities needs to be issued */
                            memset(ioctl_buffer, 0, 255);
                            memset(cdb, 0, IPR_CCB_CDB_LEN);
                            p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

                            /* Issue mode sense to get the block size */
                            cdb[0] = MODE_SENSE;
                            cdb[4] = 255;

                            rc = sg_ioctl(fdev, cdb,
                                          p_mode_parm_hdr, 255, SG_DXFER_FROM_DEV,
                                          &sense_data, 30);

                            close(p_cur_dev_to_init->fd);
                            p_cur_dev_to_init->fd = 0;

                            if (rc < 0)
                            {
                                syslog(LOG_ERR, "Could not send mode sense to %s. %m"IPR_EOL,
                                       p_cur_dev_to_init->p_ipr_device->gen_name);
                            }
                            else if (rc == CHECK_CONDITION)
                            {
                                len = sprintf("Mode Sense failed to %s. %m  ",
                                              p_cur_dev_to_init->p_ipr_device->gen_name);

                                p_char = (u8 *)&sense_data;

                                for (loop = 0; loop < 18; loop++)
                                    len += sprintf(ioctl_buffer + len,"%.2x ",p_char[loop]);

                                syslog(LOG_ERR, "sense data = %s"IPR_EOL, ioctl_buffer);
                            }
                            else
                            {
                                p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

                                if (p_mode_parm_hdr->block_desc_len > 0)
                                {
                                    p_block_desc = (struct ipr_block_desc *)(p_mode_parm_hdr+1);

                                    if ((p_block_desc->block_length[1] != 0x02) ||
                                        (p_block_desc->block_length[2] != 0x00))
                                    {
                                        p_cur_dev_to_init->change_size = IPR_522_BLOCK;
                                        remove_device(p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_address,
                                                      p_ioa);
                                    }
                                    else
                                    {
                                        if (strlen(p_cur_dev_to_init->p_ipr_device->dev_name) == 0)
                                        {
                                            revalidate_dev(p_cur_dev_to_init->p_ipr_device,
                                                           p_ioa, 1);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (p_cur_dev_to_init->fd)
                        close(p_cur_dev_to_init->fd);

                    if (p_cur_dev_to_init->change_size != 0)
                    {
                        /* send evaluate device down */
                        evaluate_device(p_cur_dev_to_init->p_ipr_device,
                                        p_ioa, p_cur_dev_to_init->change_size);
                    }
                }
                p_cur_dev_to_init = p_cur_dev_to_init->p_next;
            }

            check_current_config(false);
            revalidate_devs(NULL);

            free_form(p_form);
            curses_info.p_form = p_old_form;
            for (j=0; j < 2; j++)
                free_field(p_fields[j]);

            flush_stdscr();

            if (done_bad)
                return RC_FAILED;
            return RC_SUCCESS;
        }
        not_done = 0;
        sleep(10);
    }
}

void disk_unit_recovery()
{
    /*
                                 Work with Disk Unit Recovery

       Select one of the following:

            1. Device Concurrent Maintenance
            2. Initialize and format disk unit
            3. Reclaim IOA cache storage
            4. Rebuild disk unit data
            5. Work with resources containing cache battery packs



       Selection:

       e=Exit  q=Cancel
     */
    FIELD *p_fields[12];
    FORM *p_form, *p_old_form;
    int invalid = 0;
    int rc = RC_SUCCESS;
    int j;
    char null_char = '\0';
    int battery_maint();

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 2,   /* new field size */ 
                            12, 12,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Work with Disk Unit Recovery");          /* string value to set */

    flush_stdscr();

    clear();

    post_form(p_form);

    while(1)
    {
        mvaddstr(2, 0, "Select one of the following:");
        mvaddstr(4, 0, "     1. Device Concurrent Maintenance");
        mvaddstr(5, 0, "     2. Initialize and format disk unit");
        mvaddstr(6, 0, "     3. Reclaim IOA cache storage");
        mvaddstr(7, 0, "     4. Rebuild disk unit data");
        mvaddstr(8, 0, "     5. Work with resources containing cache battery packs");
        mvaddstr(12, 0, "Selection: ");
        mvaddstr(curses_info.max_y - 2, 0, EXIT_KEY_LABEL"=Exit  "CANCEL_KEY_LABEL"=Cancel");

        if (invalid)
        {
            invalid = 0;
            mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
        }

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        refresh();
        for (;;)
        {
            int ch = getch();

            if (IS_ENTER_KEY(ch))
            {
                char *p_char;
                refresh();
                unpost_form(p_form);
                p_char = field_buffer(p_fields[1], 0);

                switch(p_char[0])
                {
                    case '1':  /* Device Concurrent Maintenance */
                        if (RC_EXIT == (rc = device_concurrent_maintenance(&null_char, &null_char, &null_char,
                                                                           &null_char, &null_char, &null_char,
                                                                           &null_char, &null_char, &null_char,
                                                                           &null_char, &null_char)))
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_FAILED)
                            mvaddstr(curses_info.max_y - 1, 0," Device concurrent maintenance failed");
                        else if (rc == RC_IN_USE)
                            mvaddstr(curses_info.max_y - 1, 0," Device concurrent maintenance failed, device in use");
                        else if (rc == RC_SUCCESS)
                            mvaddstr(curses_info.max_y - 1, 0," Device concurrent maintenance completed successfully");
                        break;
                    case '2':  /* Initialize and format disk unit */
                        while (RC_REFRESH == (rc = init_device()))
                            clear();
                        clear();

                        post_form(p_form);
                        if (rc == RC_NOOP)
                        {
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " No units available for initialize and format");
                        }
                        else if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0, " Initialize and format completed successfully");
                        }
                        else if (rc == RC_FAILED)
                            mvaddstr(curses_info.max_y - 1, 0, " Initialize and format failed");
                        break;
                    case '3':  /* Reclaim IOA cache storage */
                        rc = reclaim_cache();
                        clear();

                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " Reclaim IOA Cache Storage completed successfully");
                        }
                        else if (rc == RC_FAILED)
                        {
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " Reclaim IOA Cache Storage failed");
                        }
                        else if (rc == RC_NOOP)
                        {
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " No Reclaim IOA Cache Storage is necessary");
                        }
                        else if (rc == RC_NOT_PERFORMED)
                        {
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " No Reclaim IOA Cache Storage performed");
                        }
                        break;
                    case '4':  /* Rebuild disk unit data */
                        if (RC_EXIT == (rc = raid_rebuild()))
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                        {
                            mvaddstr(curses_info.max_y - 1, 0, " Rebuild started, view Parity Status Window for rebuild progress");
                        }
                        else if (rc == RC_FAILED)
                            mvaddstr(curses_info.max_y - 1, 0, " Rebuild failed");
                        break;
                    case '5':
			while (RC_REFRESH == (rc = battery_maint()));

                        clear();
                        post_form(p_form);
                        if (rc == RC_SUCCESS)
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " The selected batter packs have successfully been placed into an error state.");
                        else if (rc == RC_FAILED)
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " Attempting to force the selected battery packs into an error state failed.");
                        else if (rc == RC_NOOP)
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " No configured resources contain a cache battery pack.");
                        break;
                    case ' ':
                        clear();
                        post_form(p_form);
                        break;
                    default:
                        clear();
                        post_form(p_form);
                        mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
                        break;
                }
                form_driver(p_form, ' ');
                refresh();
                break;
            }
            else if (IS_EXIT_KEY(ch))
                goto leave;
            else if (IS_CANCEL_KEY(ch))
                goto leave;
            else
            {
                form_driver(p_form, ch);
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                refresh();
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    for (j=0; j < 2; j++)
        free_field(p_fields[j]);
    flush_stdscr();
    return;
}

int device_concurrent_maintenance(char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                                  char *p_location, char *p_pci_bus, char *p_pci_device,
                                  char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun, char *p_action)
{
/* iSeries

                         Device Concurrent Maintenance

 Type the choices, then press Enter.

  Specify either Physical Location or DSA/UA.
      Physical Location   . . .  Frame ID:        Position:
           OR
      DSA/UA in hex . . . . . .  DSA:              UA:


  Specify action as   1=Remove device   2=Install device
      Action to be performed  . . . . . . . . . :


  Enter a time value between 01 and 19.
      Time needed in minutes  . . . . . . . . . :


  Or leave blank and press Enter to cancel


*/
/* pSeries

                         Device Concurrent Maintenance

 Type the choices, then press Enter.

  Specify either Physical Location or PCI/SCSI location.
      Physical Location   . . . : 
           OR
      PCI/SCSI . . . PCI Bus:         PCI Device: 
                SCSI Channel:    SCSI Id:    SCSI Lun:   


  Specify action as   1=Remove device   2=Install device
      Action to be performed  . . . . . . . . . :


  Enter a time value between 01 and 19.
      Time needed in minutes  . . . . . . . . . :


  Or leave blank and press Enter to cancel


*/
/* xSeries

                         Device Concurrent Maintenance

 Type the choices, then press Enter.

  Specify PCI/SCSI location.
      PCI/SCSI . . . PCI Bus:   PCI Device:
                 SCSI Channel:   SCSI Id:   SCSI Lun:  




  Specify action as   1=Remove device   2=Install device
      Action to be performed  . . . . . . . . . :


  Enter a time value between 01 and 19.
      Time needed in minutes  . . . . . . . . . :


  Or leave blank and press Enter to cancel


*/

    FIELD *p_input_fields[10], *p_confirm_fields[3];
    FORM *p_input_form;
    FORM *p_confirm_form;
    FORM *p_old_form;
    int rc;
    int line_num;
    int field_index = 0;

    rc = RC_SUCCESS;

    /* Title */
    p_input_fields[field_index++] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                                              0, 0,       /* upper left corner */
                                              0,           /* number of offscreen rows */
                                              0);               /* number of working buffers */

    if (platform == IPR_ISERIES)
    {
        /* Frame ID field */
        p_input_fields[field_index++] = new_field(1, 2,   /* new field size */ 
                                                  5, 43,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* Position field */
        p_input_fields[field_index++] = new_field(1, 3,   /* new field size */ 
                                                  5, 60,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* DSA field */
        p_input_fields[field_index++] = new_field(1, 8,   /* new field size */ 
                                                  7, 38,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* UA field */
        p_input_fields[field_index++] = new_field(1, 8,   /* new field size */ 
                                                  7, 55,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* Action field */
        p_input_fields[field_index++] = new_field(1, 1,   /* new field size */ 
                                                  11, 50,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* Delay field */
        p_input_fields[field_index++] = new_field(1, 2,   /* new field size */ 
                                                  15, 50,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */
    }
    else if (platform == IPR_PSERIES)
    {
        /* Physical Location field */
        p_input_fields[field_index++] = new_field(1, 16,   /* new field size */ 
                                                  5, 34,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* PCI Bus field */
        p_input_fields[field_index++] = new_field(1, 4,   /* new field size */ 
                                                  7, 30,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* PCI Device field */
        p_input_fields[field_index++] = new_field(1, 4,   /* new field size */ 
                                                  7, 50,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* SCSI Channel field */
        p_input_fields[field_index++] = new_field(1, 1,   /* new field size */ 
                                                  8, 30,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* SCSI Id field */
        p_input_fields[field_index++] = new_field(1, 2,   /* new field size */ 
                                                  8, 42,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* SCSI Lun field */
        p_input_fields[field_index++] = new_field(1, 2,   /* new field size */ 
                                                  8, 55,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* Action field */
        p_input_fields[field_index++] = new_field(1, 1,   /* new field size */ 
                                                  12, 50,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* Delay field */
        p_input_fields[field_index++] = new_field(1, 2,   /* new field size */ 
                                                  16, 50,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */
    }
    else
    {
        /* PCI Bus field */
        p_input_fields[field_index++] = new_field(1, 4,   /* new field size */ 
                                                  5, 30,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* PCI Device field */
        p_input_fields[field_index++] = new_field(1, 4,   /* new field size */ 
                                                  5, 50,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* SCSI Channel field */
        p_input_fields[field_index++] = new_field(1, 1,   /* new field size */ 
                                                  6, 30,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* SCSI Id field */
        p_input_fields[field_index++] = new_field(1, 2,   /* new field size */ 
                                                  6, 42,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* SCSI Lun field */
        p_input_fields[field_index++] = new_field(1, 2,   /* new field size */ 
                                                  6, 55,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* Action field */
        p_input_fields[field_index++] = new_field(1, 1,   /* new field size */ 
                                                  10, 50,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */

        /* Delay field */
        p_input_fields[field_index++] = new_field(1, 2,   /* new field size */ 
                                                  14, 50,       /* upper left corner */
                                                  0,           /* number of offscreen rows */
                                                  0);          /* number of working buffers */
    }


    p_input_fields[field_index++] = NULL;

    p_old_form = curses_info.p_form;

    curses_info.p_form = p_input_form = new_form(p_input_fields);

    set_current_field(p_input_form, p_input_fields[1]);

    set_field_just(p_input_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_input_fields[0], O_ACTIVE);

    set_field_buffer(p_input_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Device Concurrent Maintenance");          /* string value to set */

    field_index = 1;

    if (platform == IPR_ISERIES)
    {
        set_field_buffer(p_input_fields[field_index++], 0, p_frame);

        set_field_buffer(p_input_fields[field_index++], 0, p_slot);

        set_field_buffer(p_input_fields[field_index++], 0, p_dsa);

        set_field_buffer(p_input_fields[field_index++], 0, p_ua);

        set_field_buffer(p_input_fields[field_index++], 0, p_action);

    }
    else if (platform == IPR_PSERIES)
    {
        set_field_buffer(p_input_fields[field_index++], 0, p_location);

        set_field_buffer(p_input_fields[field_index++], 0, p_pci_bus);

        set_field_buffer(p_input_fields[field_index++], 0, p_pci_device);

        set_field_buffer(p_input_fields[field_index++], 0, p_scsi_channel);

        set_field_buffer(p_input_fields[field_index++], 0, p_scsi_id);

        set_field_buffer(p_input_fields[field_index++], 0, p_scsi_lun);

        set_field_buffer(p_input_fields[field_index++], 0, p_action);

    }
    else
    {
        set_field_buffer(p_input_fields[field_index++], 0, p_pci_bus);

        set_field_buffer(p_input_fields[field_index++], 0, p_pci_device);

        set_field_buffer(p_input_fields[field_index++], 0, p_scsi_channel);

        set_field_buffer(p_input_fields[field_index++], 0, p_scsi_id);

        set_field_buffer(p_input_fields[field_index++], 0, p_scsi_lun);

        set_field_buffer(p_input_fields[field_index++], 0, p_action);
    }

    form_opts_off(p_input_form, O_BS_OVERLOAD);

    flush_stdscr();

    clear();

    post_form(p_input_form);

    line_num = 2;

    mvaddstr(line_num++, 1, "Type the choices, then press Enter.");
    line_num++;

    if (platform == IPR_ISERIES)
    {
        mvaddstr(line_num++, 2, "Specify either Physical Location or DSA/UA.");
        mvaddstr(line_num++, 6, "Physical Location   . . .  Frame ID:        Position:");
        mvaddstr(line_num++, 11, "OR");
        mvaddstr(line_num, 6, "DSA/UA in hex . . . . . .  DSA:");
        mvaddstr(line_num++, 51, "UA:");
        line_num += 2;
    }
    else
    {
        if (platform == IPR_PSERIES)
        {
            mvaddstr(line_num++, 2, "Specify either Physical Location or PCI/SCSI location.");
            mvaddstr(line_num++, 6, "Physical Location   . . . :");
            mvaddstr(line_num++, 11, "OR");
        }
        else
            mvaddstr(line_num++, 2, "Specify PCI/SCSI location.");

        mvaddstr(line_num, 6, "PCI/SCSI . . . PCI Bus:");
        mvaddstr(line_num++, 38, "PCI Device:");
        mvaddstr(line_num, 16, "SCSI Channel:");
        mvaddstr(line_num, 33, "SCSI Id:");
        mvaddstr(line_num++, 45, "SCSI Lun:");
        line_num += 2;
    }

    mvaddstr(line_num++, 2, "Specify action as   1=Remove device   2=Install device");
    mvaddstr(line_num++, 6, "Action to be performed  . . . . . . . . . :");
    line_num += 2;
    mvaddstr(line_num++, 2, "Enter a time value between 01 and 19.");
    mvaddstr(line_num++, 6, "Time needed in minutes  . . . . . . . . . :");
    line_num += 2;
    mvaddstr(18, 2, "Or leave blank and press Enter to cancel");

    if ((strlen(p_frame) > 0) || (strlen(p_dsa) > 0) || (strlen(p_location) > 0) ||
        (strlen(p_pci_bus) > 0))
    {
        set_current_field(p_input_form, p_input_fields[field_index]);
    }
    else
    {
        form_driver(p_input_form,               /* form to pass input to */
                    REQ_FIRST_FIELD );     /* form request code */
    }

    refresh();

    while(1)
    {
        char *p_action, *p_delay;
        int ch = getch();
        char null_ch = '\0';

        if (IS_ENTER_KEY(ch))
        {
            form_driver(p_input_form, REQ_NEXT_FIELD);
            unpost_form(p_input_form);

            p_action = p_delay = &null_ch;

            if (platform == IPR_ISERIES)
            {
                p_frame = strip_trailing_whitespace(field_buffer(p_input_fields[1], 0));
                p_slot = strip_trailing_whitespace(field_buffer(p_input_fields[2], 0));
                p_slot[0] = toupper(p_slot[0]);
                p_dsa = strip_trailing_whitespace(field_buffer(p_input_fields[3], 0));
                p_ua = strip_trailing_whitespace(field_buffer(p_input_fields[4], 0));
                p_action = strip_trailing_whitespace(field_buffer(p_input_fields[5], 0));
                p_delay = strip_trailing_whitespace(field_buffer(p_input_fields[6], 0));
            }
            else if (platform == IPR_PSERIES)
            {
                p_location = strip_trailing_whitespace(field_buffer(p_input_fields[1], 0));
                p_pci_bus = strip_trailing_whitespace(field_buffer(p_input_fields[2], 0));
                p_pci_device = strip_trailing_whitespace(field_buffer(p_input_fields[3], 0));
                p_scsi_channel = strip_trailing_whitespace(field_buffer(p_input_fields[4], 0));
                p_scsi_id = strip_trailing_whitespace(field_buffer(p_input_fields[5], 0));
                p_scsi_lun = strip_trailing_whitespace(field_buffer(p_input_fields[6], 0));
                p_action = strip_trailing_whitespace(field_buffer(p_input_fields[7], 0));
                p_delay = strip_trailing_whitespace(field_buffer(p_input_fields[8], 0));
            }
            else
            {
                p_pci_bus = strip_trailing_whitespace(field_buffer(p_input_fields[1], 0));
                p_pci_device = strip_trailing_whitespace(field_buffer(p_input_fields[2], 0));
                p_scsi_channel = strip_trailing_whitespace(field_buffer(p_input_fields[3], 0));
                p_scsi_id = strip_trailing_whitespace(field_buffer(p_input_fields[4], 0));
                p_scsi_lun = strip_trailing_whitespace(field_buffer(p_input_fields[5], 0));
                p_action = strip_trailing_whitespace(field_buffer(p_input_fields[6], 0));
                p_delay = strip_trailing_whitespace(field_buffer(p_input_fields[7], 0));
            }

            if ((((strlen(p_frame) > 0) && (strlen(p_slot) > 0)) ||
                 ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0)) ||
                 (strlen(p_location) > 0) ||
                 ((strlen(p_pci_bus) > 0) && (strlen(p_pci_device) > 0) &&
                  (strlen(p_scsi_channel) && (strlen(p_scsi_id) > 0) &&
                   (strlen(p_scsi_lun) > 0))))
                && ((strlen(p_action) > 0) && (strlen(p_delay) > 0)))
            {
                rc = is_valid_device(p_frame, p_slot, p_dsa, p_ua,
                                     p_location, p_pci_bus, p_pci_device,
                                     p_scsi_channel, p_scsi_id, p_scsi_lun);

                if (rc)
                {
                    rc = conc_maintenance_failed(p_frame, p_slot, p_dsa, p_ua,
                                                 p_location, p_pci_bus, p_pci_device,
                                                 p_scsi_channel, p_scsi_id, p_scsi_lun, p_action, rc);
                    break;
                }

                clear();

                /* Title */
                p_confirm_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                                                0, 0,       /* upper left corner */
                                                0,           /* number of offscreen rows */
                                                0);               /* number of working buffers */

                /* Confirm field */
                p_confirm_fields[1] = new_field(1, 1,   /* new field size */ 
                                                16, 0,       /* upper left corner */
                                                0,           /* number of offscreen rows */
                                                0);          /* number of working buffers */

                p_confirm_fields[2] = NULL;

                p_confirm_form = new_form(p_confirm_fields);

                set_current_field(p_confirm_form, p_confirm_fields[1]);

                set_field_just(p_confirm_fields[0], JUSTIFY_CENTER);

                field_opts_off(p_confirm_fields[0],          /* field to alter */
                               O_ACTIVE);             /* attributes to turn off */ 

                if (rc)
                {
                    set_field_buffer(p_confirm_fields[0],        /* field to alter */
                                     0,          /* number of buffer to alter */
                                     "Confirm Device Concurrent Maintenance Action");/* string value to set */
                }
                else
                {
                    set_field_buffer(p_confirm_fields[0],        /* field to alter */
                                     0,          /* number of buffer to alter */
                                     "Device Concurrent Maintenance Action");/* string value to set */
                }

                flush_stdscr();

                clear();

                post_form(p_confirm_form);

                if (strcmp(p_action, "1") == 0)
                {
                    mvaddstr(2, 1, "Removal of device:");
                }
                else if (strcmp(p_action, "2") == 0)
                {
                    mvaddstr(2, 1, "Installation of device:");
                }
                else
                {
                    rc = RC_FAILED;
                    unpost_form(p_confirm_form);
                    free_form(p_confirm_form);
                    free_screen(NULL, NULL, p_confirm_fields);
                    syslog(LOG_ERR,
                           "Invalid Device Concurrent Maintenance action selected: %s"IPR_EOL,
                           p_action);
                    goto leave;
                }

                if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
                {
                    p_frame[0] = toupper(p_frame[0]);
                    mvprintw(3, 4, "Frame ID: %s  Position: %s", p_frame, p_slot);
                }
                else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
                {
                    mvprintw(3, 4, "DSA: %s  UA: %s", p_dsa, p_ua);
                }
                else if (strlen(p_location) > 0)
                {
                    mvprintw(3, 4, "Location: %s", p_location);
                }
                else
                {
                    mvprintw(3, 4, "PCI Bus: %s  PCI Device: %s", p_pci_bus, p_pci_device);
                    mvprintw(4, 4, "SCSI Channel: %s  SCSI Id: %s  SCSI Lun: %s", p_scsi_channel,
                             p_scsi_id, p_scsi_lun);
                }

                if (strtoul(p_delay, NULL, 10) > 1)
                    mvprintw(6, 2, "You have %s minutes to perform the operation", p_delay);
                else
                    mvprintw(6, 2, "You have %s minute to perform the operation", p_delay);
                mvaddstr(8, 2, "During this time, your system may seem unresponsive");
                mvprintw(curses_info.max_y - 2, 0,
                         "c=Confirm    "CANCEL_KEY_LABEL"=Cancel");
                set_current_field(p_confirm_form, p_input_fields[1]); 
                refresh();

                while(1)
                {
                    int ch = getch();
                    if ((ch == 'c') || (ch == 'C'))
                    {
                        /* Issue Concurrent maintenance action */
                        rc = issue_concurrent_maintenance(p_frame, p_slot, p_dsa, p_ua,
                                                          p_location, p_pci_bus, p_pci_device,
                                                          p_scsi_channel, p_scsi_id, p_scsi_lun,
                                                          p_action, p_delay);
                        unpost_form(p_confirm_form);
                        free_form(p_confirm_form);
                        free_screen(NULL, NULL, p_confirm_fields);
                        goto leave;
                    }
                    else if (IS_CANCEL_KEY(ch))
                    {
                        rc = RC_CANCEL;
                        unpost_form(p_confirm_form);
                        free_form(p_confirm_form);
                        free_screen(NULL, NULL, p_confirm_fields);
                        goto leave;
                    }
                }
            }
            rc = RC_CANCEL;
            break;
        }
        else if (ch == KEY_RIGHT)
            form_driver(p_input_form, REQ_NEXT_CHAR);
        else if (ch == KEY_LEFT)
            form_driver(p_input_form, REQ_PREV_CHAR);
        else if ((ch == KEY_BACKSPACE) || (ch == 127))
            form_driver(p_input_form, REQ_DEL_PREV);
        else if (ch == KEY_DC)
            form_driver(p_input_form, REQ_DEL_CHAR);
        else if (ch == KEY_IC)
            form_driver(p_input_form, REQ_INS_MODE);
        else if (ch == KEY_EIC)
            form_driver(p_input_form, REQ_OVL_MODE);
        else if (ch == KEY_HOME)
            form_driver(p_input_form, REQ_BEG_FIELD);
        else if (ch == KEY_END)
            form_driver(p_input_form, REQ_END_FIELD);
        else if (ch == '\t')
            form_driver(p_input_form, REQ_NEXT_FIELD);
        else if (ch == KEY_UP)
            form_driver(p_input_form, REQ_PREV_FIELD);
        else if (ch == KEY_DOWN)
            form_driver(p_input_form, REQ_NEXT_FIELD);
        else if (isascii(ch))
            form_driver(p_input_form, ch);
        refresh();
    }


    leave:
        free_form(p_input_form);
    free_screen(NULL, NULL, p_input_fields);
    curses_info.p_form = p_old_form;
    flush_stdscr();
    return rc;
}

int issue_concurrent_maintenance(char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                                 char *p_location, char *p_pci_bus, char *p_pci_device,
                                 char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun,
                                 char *p_action, char *p_delay)
{
    struct ipr_ioa *p_cur_ioa = p_head_ipr_ioa;
    int frame = (strlen(p_frame) > 0) && (strlen(p_slot) > 0);
    int frame_id;
    int fd, rc, i;
    u32 dsa, ua, delay;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    int res_found = 0;
    u8 scsi_bus, scsi_id, scsi_lun;
    u16 pci_bus, pci_device;
    struct ipr_resource_entry *p_resource_entry;

    delay = strtoul(p_delay, NULL, 10);

    rc = RC_SUCCESS;

    if (frame)
    {
        frame_id = strtoul(p_frame, NULL, 10);
    }
    else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
    {
        dsa = strtoul(p_dsa, NULL, 16);
        ua = strtoul(p_ua, NULL, 16);
    }
    else if (strlen(p_location) == 0)
    {
        pci_bus = strtoul(p_pci_bus, NULL, 10);
        pci_device = strtoul(p_pci_device, NULL, 10);
        scsi_bus = strtoul(p_scsi_channel, NULL, 10);
        scsi_id = strtoul(p_scsi_id, NULL, 10);
        scsi_lun = strtoul(p_scsi_lun, NULL, 10);
    }

    check_current_config(false);

    /* Loop through the adapters on the system to find the adapter that
     this physical resource belongs to */
    for (p_cur_ioa = p_head_ipr_ioa; p_cur_ioa; p_cur_ioa = p_cur_ioa->p_next)
    {
        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

        if (fd < 1)
        {
            syslog(LOG_ERR, "Cannot open %s. %m\n", p_cur_ioa->ioa.dev_name);
            continue;
        }

        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.cdb[0] = IPR_CONC_MAINT;
        ioa_cmd.cdb[1] = IPR_CONC_MAINT_CHECK_ONLY;
        ioa_cmd.buffer = NULL;
        ioa_cmd.buffer_len = 0;
        ioa_cmd.driver_cmd = 1;

        if (frame)
        {
            ioa_cmd.cdb[2] = IPR_CONC_MAINT_FRAME_ID_FMT;
            ioa_cmd.cdb[4] = (frame_id << 8) & 0xff;
            ioa_cmd.cdb[5] = frame_id & 0xff;

            ioa_cmd.cdb[6] = p_slot[0];
            ioa_cmd.cdb[7] = p_slot[1];
            ioa_cmd.cdb[8] = p_slot[2];
            ioa_cmd.cdb[9] = p_slot[3];
            ioa_cmd.cdb[12] = delay;
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            ioa_cmd.cdb[2] = IPR_CONC_MAINT_DSA_FMT;
            ioa_cmd.cdb[4] = (dsa >> 24) & 0xff;
            ioa_cmd.cdb[5] = (dsa >> 16) & 0xff;
            ioa_cmd.cdb[6] = (dsa >> 8) & 0xff;
            ioa_cmd.cdb[7] = dsa & 0xff;

            ioa_cmd.cdb[8] = (ua >> 24) & 0xff;
            ioa_cmd.cdb[9] = (ua >> 16) & 0xff;
            ioa_cmd.cdb[10] = (ua >> 8) & 0xff;
            ioa_cmd.cdb[11] = ua & 0xff;
            ioa_cmd.cdb[12] = delay;
        }
        else if (strlen(p_location) > 0)
        {
            ioa_cmd.cdb[2] = IPR_CONC_MAINT_PSERIES_FMT;
            ioa_cmd.cdb[12] = delay;
            ioa_cmd.buffer = p_location;
            ioa_cmd.buffer_len = strlen(p_location);
        }
        else
        {
            ioa_cmd.cdb[2] = IPR_CONC_MAINT_XSERIES_FMT;
            ioa_cmd.cdb[4] = (pci_bus >> 8) & 0xff;
            ioa_cmd.cdb[5] = pci_bus & 0xff;
            ioa_cmd.cdb[6] = (pci_device >> 8) & 0xff;
            ioa_cmd.cdb[7] = pci_device & 0xff;

            ioa_cmd.cdb[8] = scsi_bus & 0xff;
            ioa_cmd.cdb[9] = scsi_id & 0xff;
            ioa_cmd.cdb[10] = scsi_lun & 0xff;
            ioa_cmd.cdb[12] = delay;
        }

        /* Issue a concurrent maintenance check command, which will tell us if this
         device slot exists under this adapter */
        rc = ipr_ioctl(fd, IPR_CONC_MAINT, &ioa_cmd);

        if (rc != 0)
        {
            if (errno == ENXIO)
            {
                /* No such device or address - response which occurs when
                 the passed slot does not exist on this IOA - try another */
                close(fd);
                continue;
            }
            else
            {
                /* An unexpected error occurred during the ioctl */
                syslog(LOG_ERR, "Concurrent maintenance to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                close(fd);
                return rc;
            }
        }
        else
        {
            /* Address exists on this adapter and we are allowed to do concurrent maintenance
             to the slot (if this device is running hardware RAID, removal of the device
             would not cause the array to go non-functional) */

            for (i = 0; i < p_cur_ioa->num_devices; i++)
            {
                p_resource_entry = p_cur_ioa->dev[i].p_resource_entry;

                if (frame)
                {
                    if ((strcasecmp(p_resource_entry->frame_id, p_frame) == 0) &&
                        (strcasecmp(p_resource_entry->slot_label, p_slot) == 0))
                    {
                        res_found = 1;
                    }
                }
                else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
                {
                    if ((p_resource_entry->dsa == dsa) &&
                        (p_resource_entry->unit_address == ua))
                    {
                        res_found = 1;
                    }
                }
                else if (strlen(p_location) > 0)
                {
                    if (strcasecmp(p_location, p_resource_entry->pseries_location) == 0)
                    {
                        res_found = 1;
                    }
                }
                else
                {
                    if ((p_resource_entry->pci_bus_number == pci_bus) &&
                        (p_resource_entry->pci_slot == pci_device) &&
                        (p_resource_entry->resource_address.bus == scsi_bus) &&
                        (p_resource_entry->resource_address.target == scsi_id) &&
                        (p_resource_entry->resource_address.lun == scsi_lun))
                    {
                        res_found = 1;
                    }
                }

                if (res_found &&
                    (p_resource_entry->subtype == IPR_SUBTYPE_AF_DASD) &&
                    (!p_resource_entry->is_array_member) &&
                    p_cur_ioa->dev[i].p_qac_entry &&
                    (p_cur_ioa->dev[i].p_qac_entry->record_id == IPR_RECORD_ID_DEVICE_RECORD))
                {
                    /* A device already exists at this location and it is not protected
                     by hardware RAID. Need to post a warning */
                    rc = concurrent_maintenance_warning(p_cur_ioa, &p_cur_ioa->dev[i], 
                                                        p_frame, p_slot, p_dsa, p_ua,
                                                        p_location, p_pci_bus, p_pci_device,
                                                        p_scsi_channel, p_scsi_id, p_scsi_lun,
                                                        p_action, delay);
                    break;
                }
                else if (res_found)
                {
                    if (p_action[0] == '1') /* Remove device */
                    {
                        rc = do_dasd_conc_maintenance(p_cur_ioa, &p_cur_ioa->dev[i],
                                                      p_frame, p_slot, p_dsa, p_ua,
                                                      p_location, p_pci_bus, p_pci_device,
                                                      p_scsi_channel, p_scsi_id, p_scsi_lun,
                                                      p_action, delay, 0);
                    }
                    else /* insert device */
                    {
                        rc = do_dasd_conc_maintenance(p_cur_ioa, &p_cur_ioa->dev[i],
                                                      p_frame, p_slot, p_dsa, p_ua,
                                                      p_location, p_pci_bus, p_pci_device,
                                                      p_scsi_channel, p_scsi_id, p_scsi_lun,
                                                      p_action, delay, 1);
                    }
                    break;
                }
            }

            if (res_found == 0)
            {
                rc = do_dasd_conc_maintenance(p_cur_ioa, NULL,
                                              p_frame, p_slot, p_dsa, p_ua,
                                              p_location, p_pci_bus, p_pci_device,
                                              p_scsi_channel, p_scsi_id, p_scsi_lun,
                                              p_action, delay, 0);
            }
            return rc;
        }
    }
    return RC_FAILED;
}

int is_valid_device(char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                    char *p_location, char *p_pci_bus, char *p_pci_device,
                    char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun)
{
    struct ipr_ioa *p_cur_ioa;
    int frame = ((strlen(p_frame) > 0) && (strlen(p_slot) > 0));
    int pseries_loc = (strlen(p_location) > 0);
    int frame_id;
    u8 scsi_bus, scsi_id, scsi_lun;
    u16 pci_bus, pci_device;
    int fd, rc, i, opens;
    u32 dsa, ua, delay = 1;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    int res_found = 0;
    struct ipr_resource_entry *p_resource_entry;

    rc = RC_SUCCESS;

    if (frame)
    {
        frame_id = strtoul(p_frame, NULL, 10);
    }
    else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
    {
        dsa = strtoul(p_dsa, NULL, 16);
        ua = strtoul(p_ua, NULL, 16);
    }
    else if (!pseries_loc)
    {
        pci_bus = strtoul(p_pci_bus, NULL, 10);
        pci_device = strtoul(p_pci_device, NULL, 10);
        scsi_bus = strtoul(p_scsi_channel, NULL, 10);
        scsi_id = strtoul(p_scsi_id, NULL, 10);
        scsi_lun = strtoul(p_scsi_lun, NULL, 10);
    }

    /* Loop through the adapters on the system to find the adapter that
     this physical resource belongs to */
    for (p_cur_ioa = p_head_ipr_ioa; p_cur_ioa; p_cur_ioa = p_cur_ioa->p_next)
    {
        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

        if (fd < 1)
        {
            syslog(LOG_ERR, "Cannot open %s. %m\n", p_cur_ioa->ioa.dev_name);
            continue;
        }

        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.cdb[0] = IPR_CONC_MAINT;
        ioa_cmd.cdb[1] = IPR_CONC_MAINT_CHECK_ONLY;
        ioa_cmd.buffer = NULL;
        ioa_cmd.buffer_len = 0;
        ioa_cmd.driver_cmd = 1;

        if (frame)
        {
            ioa_cmd.cdb[2] = IPR_CONC_MAINT_FRAME_ID_FMT;
            ioa_cmd.cdb[4] = (frame_id << 8) & 0xff;
            ioa_cmd.cdb[5] = frame_id & 0xff;

            ioa_cmd.cdb[6] = p_slot[0];
            ioa_cmd.cdb[7] = p_slot[1];
            ioa_cmd.cdb[8] = p_slot[2];
            ioa_cmd.cdb[9] = p_slot[3];
            ioa_cmd.cdb[12] = delay;
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            ioa_cmd.cdb[2] = IPR_CONC_MAINT_DSA_FMT;
            ioa_cmd.cdb[4] = (dsa >> 24) & 0xff;
            ioa_cmd.cdb[5] = (dsa >> 16) & 0xff;
            ioa_cmd.cdb[6] = (dsa >> 8) & 0xff;
            ioa_cmd.cdb[7] = dsa & 0xff;

            ioa_cmd.cdb[8] = (ua >> 24) & 0xff;
            ioa_cmd.cdb[9] = (ua >> 16) & 0xff;
            ioa_cmd.cdb[10] = (ua >> 8) & 0xff;
            ioa_cmd.cdb[11] = ua & 0xff;
            ioa_cmd.cdb[12] = delay;
        }
        else if (pseries_loc)
        {
            ioa_cmd.cdb[2] = IPR_CONC_MAINT_PSERIES_FMT;
            ioa_cmd.cdb[12] = delay;
            ioa_cmd.buffer = p_location;
            ioa_cmd.buffer_len = strlen(p_location);
        }
        else
        {
            ioa_cmd.cdb[2] = IPR_CONC_MAINT_XSERIES_FMT;
            ioa_cmd.cdb[4] = (pci_bus >> 8) & 0xff;
            ioa_cmd.cdb[5] = pci_bus & 0xff;
            ioa_cmd.cdb[6] = (pci_device >> 8) & 0xff;
            ioa_cmd.cdb[7] = pci_device & 0xff;

            ioa_cmd.cdb[8] = scsi_bus & 0xff;
            ioa_cmd.cdb[9] = scsi_id & 0xff;
            ioa_cmd.cdb[10] = scsi_lun & 0xff;
            ioa_cmd.cdb[12] = delay;
        }

        /* Issue a concurrent maintenance check command, which will tell us if this
         device slot exists under this adapter */
        rc = ipr_ioctl(fd, IPR_CONC_MAINT, &ioa_cmd);

        if (rc != 0)
        {
            if (errno == ENXIO)
            {
                /* No such device or address - response which occurs when
                 the passed slot does not exist on this IOA - try another */
                close(fd);
                continue;
            }
            else
            {
                if (errno != EACCES)
                {
                    /* An unexpected error occurred during the ioctl */
                    syslog(LOG_ERR, "Concurrent maintenance to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                }
                close(fd);
                return rc;
            }
        }
        else
        {
            /* Address exists on this adapter and we are allowed to do concurrent maintenance
             to the slot (if this device is running hardware RAID, removal of the device
             would not cause the array to go non-functional) */

            /* Check to see if a device exists at this resource and if so, make sure it
             is not currently in use */
            for (i = 0; i < p_cur_ioa->num_devices; i++)
            {
                p_resource_entry = p_cur_ioa->dev[i].p_resource_entry;

                if (frame)
                {
                    if ((strcasecmp(p_resource_entry->frame_id, p_frame) == 0) &&
                        (strcasecmp(p_resource_entry->slot_label, p_slot) == 0))
                    {
                        res_found = 1;
                    }
                }
                else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
                {
                    if ((p_resource_entry->dsa == dsa) &&
                        (p_resource_entry->unit_address == ua))
                    {
                        res_found = 1;
                    }
                }
                else if (pseries_loc)
                {
                    if (strcasecmp(p_location, p_resource_entry->pseries_location) == 0)
                    {
                        res_found = 1;
                    }
                }
                else
                {
                    if ((p_resource_entry->pci_bus_number == pci_bus) &&
                        (p_resource_entry->pci_slot == pci_device) &&
                        (p_resource_entry->resource_address.bus == scsi_bus) &&
                        (p_resource_entry->resource_address.target == scsi_id) &&
                        (p_resource_entry->resource_address.lun == scsi_lun))
                    {
                        res_found = 1;
                    }
                }

                if (res_found)
                {
                    opens = num_device_opens(p_cur_ioa->host_num,
                                             p_resource_entry->resource_address.bus,
                                             p_resource_entry->resource_address.target,
                                             p_resource_entry->resource_address.lun);

                    if ((opens != 0) && (!p_resource_entry->is_array_member))
                        rc = RC_IN_USE;
                    break;
                }
            }

            close(fd);
            return rc;
        }
    }
    return RC_FAILED;
}

int concurrent_maintenance_warning(struct ipr_ioa *p_ioa,
                                   struct ipr_device *p_ipr_device,
                                   char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                                   char *p_location, char *p_pci_bus, char *p_pci_device,
                                   char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun,
                                   char *p_action, u32 delay)
{
    /*
                     Device Concurrent Maintenance Action Warning

     The device currently located in:
       Frame ID: 3  Position: D16

     is not parity protected by the IOA. Removal of the device may
     cause the system to HANG if it contains storage which is vital
     for system operation. Proceed with extreme caution. 

     Press r=Proceed with Concurrent remove
     Press q=Cancel operation




     q=Cancel    r=Confirm remove


     OR


                Device Concurrent Maintenance Action Warning

     The device previously located in:
        Frame ID: 3  Position: D16

     was not parity protected by the IOA. Installation of a new device
     in this slot will allow the adapter to discard any existing cache
     data for the previously installed device. 




     Press i=Proceed with Concurrent install
     Press q=Cancel operation




     q=Cancel    i=Confirm install


     */
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    int rc;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Device Concurrent Maintenance Action Warning");

    post_form(p_form);


    if (p_action[0] == '1') /* Remove device */
    {
        mvaddstr(2, 0, "The device currently located in:");
        if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
        {
            mvprintw(3, 4, "Frame ID: %s  Position: %s", p_frame, p_slot);
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            mvprintw(3, 4, "DSA: %s  UA: %s", p_dsa, p_ua);
        }
        else if (strlen(p_location) > 0)
        {
            mvprintw(3, 4, "Location: %s", p_location);
        }
        else
        {
            mvprintw(3, 4, "PCI Bus: %s  PCI Device: %s", p_pci_bus, p_pci_device);
            mvprintw(4, 4, "SCSI Channel: %s  SCSI Id: %s  SCSI Lun: %s", p_scsi_channel,
                     p_scsi_id, p_scsi_lun);
        }

        mvaddstr(6, 0, "is not parity protected by the IOA. Removal of the device may");
        mvaddstr(7, 0, "cause the system to HANG if it contains storage which is vital");
        mvaddstr(8, 0, "for system operation. Proceed with extreme caution.");
        mvaddstr(11, 0, "Press r=Proceed with Concurrent remove");
        mvaddstr(12, 0, "Press "CANCEL_KEY_LABEL"=Cancel operation");
        mvaddstr(curses_info.max_y - 2, 0, CANCEL_KEY_LABEL"=Cancel    r=Confirm remove");
    }
    else /* Insert device */
    {
        mvaddstr(2, 0, "The device previously located in:");
        if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
        {
            mvprintw(3, 4, "Frame ID: %s  Position: %s", p_frame, p_slot);
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            mvprintw(3, 4, "DSA: %s  UA: %s", p_dsa, p_ua);
        }
        else if (strlen(p_location) > 0)
        {
            mvprintw(3, 4, "Location: %s", p_location);
        }
        else
        {
            mvprintw(3, 4, "PCI Bus: %s  PCI Device: %s", p_pci_bus, p_pci_device);
            mvprintw(4, 4, "SCSI Channel: %s  SCSI Id: %s  SCSI Lun: %s", p_scsi_channel,
                     p_scsi_id, p_scsi_lun);
        }

        mvaddstr(6, 0, "was not parity protected by the IOA. Installation of a new device");
        mvaddstr(7, 0, "in this slot will allow the adapter to discard any existing cache");
        mvaddstr(8, 0, "data for the previously installed device.");
        mvaddstr(11, 0, "Press i=Proceed with Concurrent install");
        mvaddstr(12, 0, "Press q=Cancel operation");
        mvaddstr(curses_info.max_y - 2, 0, CANCEL_KEY_LABEL"=Cancel    i=Confirm install");
    }

    while(1)
    {
        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */
        refresh();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if ((p_action[0] == '1') && ((ch == 'r') || (ch == 'R')))
            {
                rc = do_dasd_conc_maintenance(p_ioa, p_ipr_device,
                                              p_frame, p_slot, p_dsa, p_ua,
                                              p_location, p_pci_bus, p_pci_device,
                                              p_scsi_channel, p_scsi_id, p_scsi_lun,
                                              p_action, delay, 0);
                goto leave;
            }
            else if ((p_action[0] == '2') &&
                     ((ch == 'i') || (ch == 'I')))
            {
                rc = do_dasd_conc_maintenance(p_ioa, p_ipr_device,
                                              p_frame, p_slot, p_dsa, p_ua,
                                              p_location, p_pci_bus, p_pci_device,
                                              p_scsi_channel, p_scsi_id, p_scsi_lun,
                                              p_action, delay, 1);
                goto leave;
            }
            else
            {
                break;
            }
        }
    }

    leave:
        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int do_dasd_conc_maintenance(struct ipr_ioa *p_ioa,
                             struct ipr_device *p_ipr_device,
                             char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                             char *p_location, char *p_pci_bus, char *p_pci_device,
                             char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun,
                             char *p_action, u32 delay, int replace_dev)
{
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    int fd, j, rc, k, devfd;
    struct ipr_discard_cache_data discard_data;
    struct ipr_resource_entry *p_ref_resource_entry;
    u32 dsa, ua;
    u16 pci_bus, pci_device;
    int frame_id, cmp;
    int res_found = 0;
    u8 scsi_bus, scsi_id, scsi_lun;
    struct ipr_resource_entry resource_entry;

    if (p_ipr_device)
    {
        ipr_memory_copy(&resource_entry,
                           p_ipr_device->p_resource_entry,
                           sizeof(resource_entry));
    }
    else
    {
        memset(&resource_entry, 0, sizeof(resource_entry));
    }

    if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
    {
        frame_id = strtoul(p_frame, NULL, 10);
    }
    else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
    {
        dsa = strtoul(p_dsa, NULL, 16);
        ua = strtoul(p_ua, NULL, 16);
    }

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,        /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,        /* new field size */ 
                            1, 0,        /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);            /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,                  /* number of buffer to alter */
                     "Device Concurrent Maintenance Action In Progress");

    post_form(p_form);

    if (p_action[0] == '1') /* Remove device */
    {
        mvaddstr(3, 0, "Please remove the device located in:");

        if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
        {
            mvprintw(4, 4, "Frame ID: %s  Position: %s", p_frame, p_slot);
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            mvprintw(4, 4, "DSA: %s  UA: %s", p_dsa, p_ua);
        }
        else if (strlen(p_location) > 0)
        {
            mvprintw(4, 4, "Location: %s", p_location);
        }
        else 
        {
            mvprintw(4, 4, "PCI Bus: %s  PCI Device: %s", p_pci_bus, p_pci_device);
            mvprintw(5, 4, "SCSI Channel: %s  SCSI Id: %s  SCSI Lun: %s", p_scsi_channel,
                     p_scsi_id, p_scsi_lun);
        }
    }
    else /* Insert device */
    {
        mvaddstr(3, 0, "Please insert the device located in:");
        if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
        {
            mvprintw(4, 4, "Frame ID: %s  Position: %s", p_frame, p_slot);
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            mvprintw(4, 4, "DSA: %s  UA: %s", p_dsa, p_ua);
        }
        else if (strlen(p_location) > 0)
        {
            mvprintw(4, 4, "Location: %s", p_location);
        }
        else 
        {
            mvprintw(4, 4, "PCI Bus: %s  PCI Device: %s", p_pci_bus, p_pci_device);
            mvprintw(5, 4, "SCSI Channel: %s  SCSI Id: %s  SCSI Lun: %s", p_scsi_channel,
                     p_scsi_id, p_scsi_lun);
        }

    }

    form_driver(p_form,               /* form to pass input to */
                REQ_LAST_FIELD );     /* form request code */
    refresh();

    fd = open(p_ioa->ioa.dev_name, O_RDWR);

    if (fd <= 1)
    {
        rc = RC_FAILED;
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_ioa->ioa.dev_name);
        goto leave;
    }

    /* Build the Concurrent maintenance command */
    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

    ioa_cmd.cdb[0] = IPR_CONC_MAINT;
    ioa_cmd.cdb[1] = IPR_CONC_MAINT_CHECK_AND_QUIESCE;
    ioa_cmd.buffer = NULL;
    ioa_cmd.buffer_len = 0;
    ioa_cmd.driver_cmd = 1;

    if (p_action[0] == '1') /* Remove device */
    {
        ioa_cmd.cdb[2] = IPR_CONC_MAINT_REMOVE << IPR_CONC_MAINT_TYPE_SHIFT;
    }
    else if (p_action[0] == '2') /* Insert device */
    {
        ioa_cmd.cdb[2] = IPR_CONC_MAINT_INSERT << IPR_CONC_MAINT_TYPE_SHIFT;
    }
    else
    {
        syslog(LOG_ERR, "Invalid action specified : %c."IPR_EOL, p_action[0]);
        rc = RC_FAILED;
        goto leave;
    }

    if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
    {
        ioa_cmd.cdb[2] |= IPR_CONC_MAINT_FRAME_ID_FMT;
        ioa_cmd.cdb[4] = (frame_id << 8) & 0xff;
        ioa_cmd.cdb[5] = frame_id & 0xff;

        ioa_cmd.cdb[6] = p_slot[0];
        ioa_cmd.cdb[7] = p_slot[1];
        ioa_cmd.cdb[8] = p_slot[2];
        ioa_cmd.cdb[9] = p_slot[3];
        ioa_cmd.cdb[12] = delay;
    }
    else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
    {
        ioa_cmd.cdb[2] |= IPR_CONC_MAINT_DSA_FMT;
        ioa_cmd.cdb[4] = (dsa >> 24) & 0xff;
        ioa_cmd.cdb[5] = (dsa >> 16) & 0xff;
        ioa_cmd.cdb[6] = (dsa >> 8) & 0xff;
        ioa_cmd.cdb[7] = dsa & 0xff;

        ioa_cmd.cdb[8] = (ua >> 24) & 0xff;
        ioa_cmd.cdb[9] = (ua >> 16) & 0xff;
        ioa_cmd.cdb[10] = (ua >> 8) & 0xff;
        ioa_cmd.cdb[11] = ua & 0xff;
        ioa_cmd.cdb[12] = delay;
    }
    else if (strlen(p_location) > 0)
    {
        ioa_cmd.cdb[2] |= IPR_CONC_MAINT_PSERIES_FMT;
        ioa_cmd.cdb[12] = delay;
        ioa_cmd.buffer = p_location;
        ioa_cmd.buffer_len = strlen(p_location);
    }
    else
    {
        pci_bus = strtoul(p_pci_bus, NULL, 10);
        pci_device = strtoul(p_pci_device, NULL, 10);
        scsi_bus = strtoul(p_scsi_channel, NULL, 10);
        scsi_id = strtoul(p_scsi_id, NULL, 10);
        scsi_lun = strtoul(p_scsi_lun, NULL, 10);

        ioa_cmd.cdb[2] |= IPR_CONC_MAINT_XSERIES_FMT;
        ioa_cmd.cdb[4] = (pci_bus >> 8) & 0xff;
        ioa_cmd.cdb[5] = pci_bus & 0xff;
        ioa_cmd.cdb[6] = (pci_device >> 8) & 0xff;
        ioa_cmd.cdb[7] = pci_device & 0xff;

        ioa_cmd.cdb[8] = scsi_bus & 0xff;
        ioa_cmd.cdb[9] = scsi_id & 0xff;
        ioa_cmd.cdb[10] = scsi_lun & 0xff;
        ioa_cmd.cdb[12] = delay;
    }

    rc = ipr_ioctl(fd, IPR_CONC_MAINT, &ioa_cmd);

    if (rc != 0)
    {
        syslog(LOG_ERR, "Concurrent maintenance to %s failed. %m"IPR_EOL, p_ioa->ioa.dev_name);
        goto leave;
    }
    else
    {
        /* Concurrent maintenance was successful */
        /* Lets see if this was an insert of a new device */

        if ((p_action[0] == '2') &&
            (!resource_entry.is_array_member))
        {
            /* This was an insert. We may need to tell the system about it */
            res_found = 0;

            /* Wait for the new device to show up to the system for 1 minute,
             looking every 5 seconds */
            for (k = 0; (k < 12) && !res_found; k++)
            {
                sleep(5);
                check_current_config(false);

                for (j = 0; j < p_ioa->num_devices; j++)
                {
                    p_ref_resource_entry = p_ioa->dev[j].p_resource_entry;

                    if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
                    {
                        if ((strcasecmp(p_ref_resource_entry->frame_id, p_frame) == 0) &&
                            (strcasecmp(p_ref_resource_entry->slot_label, p_slot) == 0))
                        {
                            res_found = 1;
                        }
                    }
                    else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
                    {
                        if ((p_ref_resource_entry->dsa == dsa) &&
                            (p_ref_resource_entry->unit_address == ua))
                        {
                            res_found = 1;
                        }
                    }
                    else if (strlen(p_location) > 0)
                    {
                        if (strcasecmp(p_location, p_ref_resource_entry->pseries_location) == 0)
                        {
                            res_found = 1;
                        }
                    }
                    else
                    {
                        if ((p_ref_resource_entry->pci_bus_number == pci_bus) &&
                            (p_ref_resource_entry->pci_slot == pci_device) &&
                            (p_ref_resource_entry->resource_address.bus == scsi_bus) &&
                            (p_ref_resource_entry->resource_address.target == scsi_id) &&
                            (p_ref_resource_entry->resource_address.lun == scsi_lun))
                        {
                            res_found = 1;
                        }
                    }

                    if (res_found && replace_dev)
                    {
                        cmp = memcmp(&resource_entry.std_inq_data.vpids,
                                     &p_ref_resource_entry->std_inq_data.vpids,
                                     sizeof(struct ipr_std_inq_vpids)) ||
                            memcmp(&resource_entry.std_inq_data.serial_num,
                                   &p_ref_resource_entry->std_inq_data.serial_num,
                                   IPR_SERIAL_NUM_LEN);
                    }

                    if (res_found &&
                        (!replace_dev || cmp || p_ref_resource_entry->dev_changed ||
                         (resource_entry.subtype != IPR_SUBTYPE_AF_DASD)))
                    { 
                        if (replace_dev && cmp)
                        {
                            /* Replacement device is different than original */
                            /* We want to discard any cache data for the old device */
                            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                            discard_data.length = htoipr32(0x20);
                            discard_data.data.vpids_sn.vpids = resource_entry.std_inq_data.vpids;
                            memcpy(&discard_data.data.vpids_sn.serial_num,
                                   &resource_entry.std_inq_data.serial_num,
                                   IPR_SERIAL_NUM_LEN);

                            ioa_cmd.buffer = &discard_data;
                            ioa_cmd.buffer_len = sizeof(struct ipr_discard_cache_data);
                            ioa_cmd.cdb[1] = IPR_PROHIBIT_CORR_INFO_UPDATE;
                            rc = ipr_ioctl(fd, IPR_DISCARD_CACHE_DATA, &ioa_cmd);

                            if (rc != 0)
                            {
                                syslog(LOG_ERR, "Discard cache data to %s failed. %m"IPR_EOL, p_ioa->ioa.dev_name);
                            }
                        }

                        /* A device now exists at this location */
                        /* See if we can open it. */
                        if (IPR_IS_DASD_DEVICE(p_ref_resource_entry->std_inq_data))
                        {
                            devfd = open_dev(p_ioa, &p_ioa->dev[j]);

                            if (devfd > 1)
                                close(devfd);
                        }

                        /* We may need to format the device to make it useable */
                        /* Lets take the user to that screen */
                        conc_maintenance_complt(p_ioa,
                                                &p_ioa->dev[j],
                                                p_frame, p_slot, p_dsa, p_ua,
                                                p_location, p_pci_bus, p_pci_device,
                                                p_scsi_channel, p_scsi_id, p_scsi_lun,
                                                p_action, delay, replace_dev);
                        goto leave;
                    }
                    else
                        res_found = 0;
                }
            }
            syslog(LOG_ERR, "Timed out waiting for new device to show up\n");
            conc_maintenance_complt(p_ioa,
                                    NULL,
                                    p_frame, p_slot, p_dsa, p_ua,
                                    p_location, p_pci_bus, p_pci_device,
                                    p_scsi_channel, p_scsi_id, p_scsi_lun,
                                    p_action, delay, 0);
            goto leave;
        }
        else
        {
            conc_maintenance_complt(p_ioa,
                                    p_ipr_device,
                                    p_frame, p_slot, p_dsa, p_ua,
                                    p_location, p_pci_bus, p_pci_device,
                                    p_scsi_channel, p_scsi_id, p_scsi_lun,
                                    p_action, delay, 0);
        }
    }

    leave:
        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    close(fd);
    return rc;
}

int conc_maintenance_complt(struct ipr_ioa *p_ioa,
                            struct ipr_device *p_ipr_device,
                            char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                            char *p_location, char *p_pci_bus, char *p_pci_device,
                            char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun,
                            char *p_action, u32 delay, int replace_dev)
{
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    int rc, fd;
    int frame = (strlen(p_frame) > 0) && (strlen(p_slot) > 0);
    u32 dsa, ua, resource_handle;
    int frame_id;
    struct devs_to_init_t cur_dasd_init;
    struct ipr_resource_entry *p_resource_entry = NULL;
    struct ipr_ioctl_cmd_internal ioa_cmd;

    if (p_ipr_device)
        p_resource_entry = p_ipr_device->p_resource_entry;

    if (!frame)
    {
        dsa = strtoul(p_dsa, NULL, 16);
        ua = strtoul(p_ua, NULL, 16);
    }
    else
    {
        frame_id = strtoul(p_frame, NULL, 10);
    }

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Concurrent Maintenance Results");

    refresh:

        post_form(p_form);

    mvaddstr(3, 0, "THE CONCURRENT MAINTENANCE PROCEDURE HAS COMPLETED SUCCESSFULLY.");

    if (p_action[0] == '1') /* Remove device */
    {
        if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
        {
            mvprintw(5, 6, "FRAME = %s      POSITION: %s      ACTION = REMOVE", p_frame, p_slot);
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            mvprintw(5, 6, "DSA =  %s      UA = %s      ACTION = REMOVE", p_dsa, p_ua);
        }
        else if (strlen(p_location) > 0)
        {
            mvprintw(5, 6, "Location =  %s      ACTION = REMOVE", p_location);
        }
        else
        {
            mvprintw(5, 4, "PCI Bus: %s  PCI Device: %s", p_pci_bus, p_pci_device);
            mvprintw(6, 4, "SCSI Channel: %s  SCSI Id: %s  SCSI Lun: %s   ACTION = REMOVE",
                     p_scsi_channel, p_scsi_id, p_scsi_lun);
        }

        if (p_resource_entry)
        {
            /* Remove the device from the midlayer */
            remove_device(p_resource_entry->resource_address, p_ioa);

            /* send evaluate device capabilities down */
            memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

            ioa_cmd.buffer = NULL;
            ioa_cmd.buffer_len = 0;
            resource_handle = iprtoh32(p_resource_entry->resource_handle);
            ioa_cmd.cdb[2] = (u8)(resource_handle >> 24);
            ioa_cmd.cdb[3] = (u8)(resource_handle >> 16);
            ioa_cmd.cdb[4] = (u8)(resource_handle >> 8);
            ioa_cmd.cdb[5] = (u8)(resource_handle);

            fd = open(p_ioa->ioa.dev_name, O_RDWR);
            if (fd != -1)
                ipr_ioctl(fd, IPR_EVALUATE_DEVICE, &ioa_cmd);
        }
    }
    else /* Insert device */
    {
        if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
        {
            mvprintw(5, 6, "FRAME = %s      POSITION: %s      ACTION = INSERT", p_frame, p_slot);
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            mvprintw(5, 6, "DSA =  %s      UA = %s      ACTION = INSERT", p_dsa, p_ua);
        }
        else if (strlen(p_location) > 0)
        {
            mvprintw(5, 6, "Location =  %s      ACTION = INSERT", p_location);
        }
        else
        {
            mvprintw(5, 4, "PCI Bus: %s  PCI Device: %s", p_pci_bus, p_pci_device);
            mvprintw(6, 4, "SCSI Channel: %s  SCSI Id: %s  SCSI Lun: %s   ACTION = INSERT",
                     p_scsi_channel, p_scsi_id, p_scsi_lun);
        }
    }

    if (replace_dev && p_resource_entry && !p_resource_entry->is_array_member)
    {
        mvprintw(8, 4, "In order to make this device useable, it must be formatted.");
        mvprintw(9, 4, "Press 'f' to proceed to the format screen.");
        mvprintw(10, 4, "Or press 'c' to cancel.");
        memset (&cur_dasd_init, 0, sizeof(struct devs_to_init_t));
        p_head_dev_to_init = p_tail_dev_to_init = &cur_dasd_init;
        cur_dasd_init.p_ipr_device = p_ipr_device; 
        cur_dasd_init.p_ioa = p_ioa;
        cur_dasd_init.do_init = 1;
        cur_dasd_init.p_next = NULL;
    }
    else
    {
        mvprintw(11, 4, "Press 'c' to continue.");
    }

    form_driver(p_form,               /* form to pass input to */
                REQ_LAST_FIELD );     /* form request code */
    refresh();

    for (;;)
    {
        int ch = getch();

        if (IS_EXIT_KEY(ch))
        {
            rc = RC_EXIT;
            unpost_form(p_form);
            break;
        }
        else if (IS_CANCEL_KEY(ch) || (ch == 'c'))
        {
            rc = RC_CANCEL;
            unpost_form(p_form);
            break;
        }
        else if (((ch == 'f') || (ch == 'F')) && (replace_dev))
        {
            unpost_form(p_form);
            confirm_init_device();
            break;
        }
        else
        {
            unpost_form(p_form);
            clear();
            goto refresh;
        }
    }

    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    p_head_dev_to_init = p_tail_dev_to_init = NULL;
    return rc;
}

int conc_maintenance_failed(char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                            char *p_location, char *p_pci_bus, char *p_pci_device,
                            char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun, char *p_action, int rc)
{
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form;
    int frame = (strlen(p_frame) > 0) && (strlen(p_slot) > 0);
    u32 dsa, ua;
    int frame_id;

    if (!frame)
    {
        dsa = strtoul(p_dsa, NULL, 16);
        ua = strtoul(p_ua, NULL, 16);
    }
    else
    {
        frame_id = strtoul(p_frame, NULL, 10);
    }

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0, /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Concurrent Maintenance Results");

    post_form(p_form);

    mvaddstr(3, 0, "THE CONCURRENT MAINTENANCE PROCEDURE HAS FAILED.");

    if (p_action[0] == '1') /* Remove device */
    {
        if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
        {
            mvprintw(5, 6, "FRAME = %s      POSITION: %s      ACTION = REMOVE", p_frame, p_slot);
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            mvprintw(5, 6, "DSA =  %s      UA = %s      ACTION = REMOVE", p_dsa, p_ua);
        }
        else if (strlen(p_location) > 0)
        {
            mvprintw(5, 6, "Location =  %s      ACTION = REMOVE", p_location);
        }
        else
        {
            mvprintw(5, 4, "PCI Bus: %s  PCI Device: %s", p_pci_bus, p_pci_device);
            mvprintw(6, 4, "SCSI Channel: %s  SCSI Id: %s  SCSI Lun: %s   ACTION = REMOVE",
                     p_scsi_channel, p_scsi_id, p_scsi_lun);
        }
    }
    else /* Insert device */
    {
        if ((strlen(p_frame) > 0) && (strlen(p_slot) > 0))
        {
            mvprintw(5, 6, "FRAME = %s      POSITION: %s      ACTION = INSERT", p_frame, p_slot);
        }
        else if ((strlen(p_dsa) > 0) && (strlen(p_ua) > 0))
        {
            mvprintw(5, 6, "DSA =  %s      UA = %s      ACTION = INSERT", p_dsa, p_ua);
        }
        else if (strlen(p_location) > 0)
        {
            mvprintw(5, 6, "Location =  %s      ACTION = INSERT", p_location);
        }
        else
        {
            mvprintw(5, 4, "PCI Bus: %s  PCI Device: %s", p_pci_bus, p_pci_device);
            mvprintw(6, 4, "SCSI Channel: %s  SCSI Id: %s  SCSI Lun: %s   ACTION = INSERT",
                     p_scsi_channel, p_scsi_id, p_scsi_lun);
        }
    }


    if (rc == RC_IN_USE)
        mvaddstr(8, 0," Device concurrent maintenance failed, device in use");
    else if (rc == EACCES)
        mvaddstr(8, 0," Device concurrent maintenance failed, device cannot be removed currently");

    mvprintw(9, 4, "Press Enter to continue.");

    form_driver(p_form,               /* form to pass input to */
                REQ_LAST_FIELD );     /* form request code */
    refresh();

    for (;;)
    {
        int ch = getch();

        if (IS_EXIT_KEY(ch))
        {
            rc = RC_EXIT;
            unpost_form(p_form);
            break;
        }
        else if (IS_CANCEL_KEY(ch) || (ch == 'c'))
        {
            rc = RC_CANCEL;
            unpost_form(p_form);
            break;
        }
        else
        {
            rc = RC_CANCEL;
            unpost_form(p_form);
            break;
        }
    }

    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int battery_maint()
{
#define BATTERY_HDR_SIZE           7
#define BATTERY_MENU_SIZE          4
    /*
                       Work with Resources Containing Cache Battery Packs

        Type options, press Enter
          2=Force battery pack into error state    5=Display battery information

            Resource                               Frame Card
        Opt Name          Serial Number Type-Model  ID   Position
            /dev/ipr5    02137002     2757-001    2    C02
            /dev/ipr4    03060044     5703-001    2    003
            /dev/ipr3    02127005     5703-001    1    C10
            /dev/ipr1    01205058     2778-001    2    C03
            /dev/ipr0    00103039     2748-001    2    002

        e=Exit   q=Cancel

     */
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form, *p_list_form;
    int fd, rc, i, len, last_char;
    struct ipr_reclaim_query_data *p_reclaim_buffer, *p_cur_reclaim_buffer;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    int num_lines = 0;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    struct ipr_ioa *p_cur_ioa;
    int max_size = curses_info.max_y - (BATTERY_HDR_SIZE + BATTERY_MENU_SIZE);
    int num_needed = 0;
    int ioa_num = 0, force_error;
    char buffer[100], *p_char;
    struct ipr_resource_entry *p_resource;
    int show_battery_info(struct ipr_ioa *p_ioa);
    int confirm_force_battery_error();

    rc = RC_SUCCESS;

    p_reclaim_buffer = (struct ipr_reclaim_query_data*)malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

    p_title_win = newwin(BATTERY_HDR_SIZE, curses_info.max_x, 0, 0);
    p_menu_win = newwin(BATTERY_MENU_SIZE, curses_info.max_x, max_size + BATTERY_HDR_SIZE, 0);
    p_field_win = newwin(max_size, curses_info.max_x, BATTERY_HDR_SIZE, 0);

    clear();

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],                       /* field to alter */
                     0,                                 /* number of buffer to alter */
                     "Work with Resources Containing Cache Battery Packs");  /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    mvwaddstr(p_title_win, 2, 0,
              "Type options, press Enter");
    mvwaddstr(p_title_win, 3, 0,
              "  2=Force battery pack into error state    5=Display battery information");
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 5, 0,
                  "    Resource                               Frame Card");
        mvwaddstr(p_title_win, 6, 0,
                  "Opt Name          Serial Number Type-Model  ID   Position");
    }
    else if (platform == IPR_PSERIES)
    {
        mvwaddstr(p_title_win, 5, 0,
                  "    Resource                                           PCI  PCI   SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "Opt Name          Serial Number Type-Model  Location   Bus Device Host");
    }
    else
    {
        mvwaddstr(p_title_win, 5, 0,
                  "    Resource                               PCI  PCI   SCSI");
        mvwaddstr(p_title_win, 6, 0,
                  "Opt Name          Serial Number Type-Model Bus Device Host");
    }
    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

    for (p_cur_ioa = p_head_ipr_ioa, p_cur_reclaim_buffer = p_reclaim_buffer;
         p_cur_ioa != NULL;
         p_cur_ioa = p_cur_ioa->p_next, p_cur_reclaim_buffer++)
    {
        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            continue;
        }

        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.buffer = p_cur_reclaim_buffer;
        ioa_cmd.buffer_len = sizeof(struct ipr_reclaim_query_data);
        ioa_cmd.cdb[1] = IPR_RECLAIM_QUERY | IPR_RECLAIM_EXTENDED_INFO;
        ioa_cmd.read_not_write = 1;

        rc = ipr_ioctl(fd, IPR_RECLAIM_CACHE_STORE, &ioa_cmd);

        if (rc != 0)
        {
            if (errno != EINVAL)
            {
                syslog(LOG_ERR, "Reclaim Query to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            }
            close(fd);
            p_cur_ioa->p_reclaim_data = NULL;
            continue;
        }

        close(fd);

        p_cur_ioa->p_reclaim_data = p_cur_reclaim_buffer;

        if (p_cur_reclaim_buffer->rechargeable_battery_type)
        {
            num_needed++;
        }
    }

    if (num_needed)
    {
        p_cur_ioa = p_head_ipr_ioa;

        while(p_cur_ioa)
        {
            if ((p_cur_ioa->p_reclaim_data) &&
                (p_cur_ioa->p_reclaim_data->rechargeable_battery_type))
            {
                start_y = ioa_num % (max_size - 1);

                if ((num_lines > 0) && (start_y == 0))
                {
                    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));

                    pp_field[field_index] = new_field(1, curses_info.max_x,
                                                      max_size - 1, 0, 0, 0);

                    set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                    set_field_buffer(pp_field[field_index], 0, "More... ");
                    set_field_userptr(pp_field[field_index], NULL);
                    field_opts_off(pp_field[field_index++],
                                   O_ACTIVE);
                }

                /* User entry field */
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 2));
                pp_field[field_index] = new_field(1, 1, start_y, 1, 0, 0);
                set_field_userptr(pp_field[field_index++], (char *)p_cur_ioa);

                /* Description field */
                pp_field[field_index] = new_field(1, curses_info.max_x - 4,
                                                  start_y, 4, 0, 0);

                p_resource = p_cur_ioa->ioa.p_resource_entry;

                if (platform == IPR_ISERIES)
                {
                    len = sprintf(buffer, "%s    %8s     %x-001    %s    %s",
                                  p_cur_ioa->ioa.dev_name,
                                  p_cur_ioa->serial_num,
                                  p_cur_ioa->ccin,
                                  p_resource->frame_id,
                                  p_resource->slot_label);
                }
                else if (platform == IPR_PSERIES)
                {
                    len = sprintf(buffer, "%s    %8s     %x-001    %s      %d   %d    %d",
                                  p_cur_ioa->ioa.dev_name,
                                  p_cur_ioa->serial_num,
                                  p_cur_ioa->ccin,
                                  p_resource->pseries_location,
                                  p_resource->pci_bus_number,
                                  p_resource->pci_slot,
                                  p_resource->host_no);
                }
                else
                {
                    len = sprintf(buffer, "%s    %8s     %x-001   %d    %d     %d",
                                  p_cur_ioa->ioa.dev_name,
                                  p_cur_ioa->serial_num,
                                  p_cur_ioa->ccin,
                                  p_resource->pci_bus_number,
                                  p_resource->pci_slot,
                                  p_resource->host_no);
                }

                set_field_buffer(pp_field[field_index], 0, buffer);
                field_opts_off(pp_field[field_index], O_ACTIVE);
                set_field_userptr(pp_field[field_index++], NULL);

                memset(buffer, 0, 100);
                len = 0;
                num_lines++;
                ioa_num++;
            }
            p_cur_reclaim_buffer++;
            p_cur_ioa = p_cur_ioa->p_next;

        }

        field_opts_off(p_fields[1], O_ACTIVE);

        ioa_num = ((max_size - 1) * 2) + 1;
        for (i = ioa_num; i < field_index; i += ioa_num)
            set_new_page(pp_field[i], TRUE);

        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = NULL;

        p_list_form = new_form(pp_field);

        set_form_win(p_list_form, p_field_win);

        post_form(p_list_form);

        form_driver(p_list_form,
                    REQ_FIRST_FIELD);

        while(1)
        {
            wnoutrefresh(stdscr);
            wnoutrefresh(p_menu_win);
            wnoutrefresh(p_title_win);
            wnoutrefresh(p_field_win);
            doupdate();

            for (;;)
            {
                int ch = getch();

                if (IS_EXIT_KEY(ch))
                {
                    rc = RC_EXIT;
                    goto leave;
                }
                else if (IS_CANCEL_KEY(ch))
                {
                    rc = RC_CANCEL;
                    goto leave;
                }
                else if (IS_PGDN_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_NEXT_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if (IS_PGUP_KEY(ch))
                {
                    form_driver(p_list_form,
                                REQ_PREV_PAGE);
                    form_driver(p_list_form,
                                REQ_FIRST_FIELD);
                    break;
                }
                else if ((ch == KEY_DOWN) ||
                         (ch == '\t'))
                {
                    form_driver(p_list_form,
                                REQ_NEXT_FIELD);
                    break;
                }
                else if (ch == KEY_UP)
                {
                    form_driver(p_list_form,
                                REQ_PREV_FIELD);
                    break;
                }
                else if (IS_ENTER_KEY(ch))
                {
                    force_error = 0;

                    for (i = 0; i < field_index; i++)
                    {
                        p_cur_ioa = (struct ipr_ioa *)field_userptr(pp_field[i]);
                        if (p_cur_ioa != NULL)
                        {
                            p_char = field_buffer(pp_field[i], 0);

                            if (strcmp(p_char, "2") == 0)
                            {
                                force_error++;
                                p_cur_ioa->ioa.is_reclaim_cand = 1;
                            }
                            else if (strcmp(p_char, "5") == 0)
                            {
                                rc = show_battery_info(p_cur_ioa);
				if (rc == RC_CANCEL)
				    rc = RC_REFRESH;
				goto leave;
                            }
                            else
                            {
                                p_cur_ioa->ioa.is_reclaim_cand = 0;
                            }
                        }
                    }

                    if (force_error)
                    {
                        rc = confirm_force_battery_error();
			if (rc == RC_CANCEL)
			    rc = RC_REFRESH;
                        goto leave;
                    }
                }
                else
                {
                    /* Terminal is running under 5250 telnet */
                    if (term_is_5250)
                    {
                        /* Send it to the form to be processed and flush this line */
                        form_driver(p_list_form, ch);
                        last_char = flush_line();
                        if (IS_5250_CHAR(last_char))
                            ungetch(ENTER_KEY);
                        else if (last_char != ERR)
                            ungetch(last_char);
                    }
                    else
                    {
                        form_driver(p_list_form, ch);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        rc = RC_NOOP;
        goto leave;
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(p_reclaim_buffer);
    free(pp_field);
    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int confirm_force_battery_error()
{
#define BATTERY_CNF_HDR_SIZE          17
#define BATTERY_CNF_MENU_SIZE          4
    /*
                              Force Battery Packs Into Error State

        ATTENTION: This service function should be run only under the direction
        of the IBM Hardware Service Support

        You have selected to force a cache batter error on an IOA

        You will have to replace the Cache Battery Pack in each selected IOA to
        resume normal operations.

        System performance could be significantly degraded until the cache battery
        packs are replaced on the selected IOAs.

        Press c=Continue to force the following battery packs into an error state

            Resource                               Frame Card
        Opt Name          Serial Number Type-Model  ID   Position
         2  /dev/ipr5    02137002     2757-001    2    C02
         2  /dev/ipr4    03060044     5703-001    2    003
         2  /dev/ipr3    02127005     5703-001    1    C10
                                                                                 More...

        e=Exit   q=Cancel    f=PageDn   b=PageUp

     */
    FIELD *p_fields[3];
    FORM *p_form, *p_old_form, *p_list_form;
    int fd, rc, i, len, last_char, reclaim_rc;
    struct ipr_reclaim_query_data reclaim_buffer;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_resource_entry *p_resource;
    int num_lines = 0;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    struct ipr_ioa *p_cur_ioa;
    int max_size = curses_info.max_y - (BATTERY_CNF_HDR_SIZE + BATTERY_CNF_MENU_SIZE);
    int ioa_num = 0;
    char buffer[100];
    int init_page, new_page;

    reclaim_rc = rc = RC_SUCCESS;

    p_title_win = newwin(BATTERY_CNF_HDR_SIZE, curses_info.max_x, 0, 0);
    p_menu_win = newwin(BATTERY_CNF_MENU_SIZE, curses_info.max_x, max_size + BATTERY_CNF_HDR_SIZE, 0);
    p_field_win = newwin(max_size, curses_info.max_x, BATTERY_CNF_HDR_SIZE, 0);

    clear();

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],                       /* field to alter */
                     0,                                 /* number of buffer to alter */
                     "Force Battery Packs Into Error State");  /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    wattron(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 0, "ATTENTION:");
    wattroff(p_title_win, A_BOLD);
    mvwaddstr(p_title_win, 2, 10,
              " This service function should be run only under the direction");
    mvwaddstr(p_title_win, 3, 0,
              "of the IBM Hardware Service Support");
    mvwaddstr(p_title_win, 5, 0,
              "You have selected to force a cache batter error on an IOA");
    mvwaddstr(p_title_win, 7, 0,
              "You will have to replace the Cache Battery Pack in each selected IOA to");
    mvwaddstr(p_title_win, 8, 0,
              "resume normal operations.");
    mvwaddstr(p_title_win, 10, 0,
              "System performance could be significantly degraded until the cache battery");
    mvwaddstr(p_title_win, 11, 0,
              "packs are replaced on the selected IOAs.");
    mvwaddstr(p_title_win, 13, 0,
              "Press c=Continue to force the following battery packs into an error state");
    
    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 15, 0,
                  "    Resource                               Frame Card");
        mvwaddstr(p_title_win, 16, 0,
                  "Opt Name          Serial Number Type-Model  ID   Position");
    }
    else if (platform == IPR_PSERIES)
    {
        mvwaddstr(p_title_win, 15, 0,
                  "    Resource                                        PCI  PCI   SCSI");
        mvwaddstr(p_title_win, 16, 0,
                  "Opt Name          Serial Number Type-Model Location Bus Device Host");
    }
    else
    {
        mvwaddstr(p_title_win, 15, 0,
                  "    Resource                               PCI  PCI   SCSI");
        mvwaddstr(p_title_win, 16, 0,
                  "Opt Name          Serial Number Type-Model Bus Device Host");
    }
    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");

    p_cur_ioa = p_head_ipr_ioa;

    while(p_cur_ioa)
    {
        if ((p_cur_ioa->p_reclaim_data) &&
            (p_cur_ioa->ioa.is_reclaim_cand))
        {
            start_y = ioa_num % (max_size - 1);

            if ((num_lines > 0) && (start_y == 0))
            {
                pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));

                pp_field[field_index] = new_field(1, curses_info.max_x,
                                                  max_size - 1, 0, 0, 0);

                set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
                set_field_buffer(pp_field[field_index], 0, "More... ");
                set_field_userptr(pp_field[field_index], NULL);
                field_opts_off(pp_field[field_index++],
                               O_ACTIVE);
            }

            /* Description field */
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, curses_info.max_x,
                                              start_y, 0, 0, 0);

            p_resource = p_cur_ioa->ioa.p_resource_entry;

            if (platform == IPR_ISERIES)
            {
                len = sprintf(buffer, " 2  %s    %8s     %x-001    %s    %s",
                              p_cur_ioa->ioa.dev_name,
                              p_cur_ioa->serial_num,
                              p_cur_ioa->ccin,
                              p_resource->frame_id,
                              p_resource->slot_label);
            }
            else if (platform == IPR_PSERIES)
            {
                len = sprintf(buffer, " 2  %s    %8s     %x-001    %s   %d   %d    %d",
                              p_cur_ioa->ioa.dev_name,
                              p_cur_ioa->serial_num,
                              p_cur_ioa->ccin,
                              p_resource->pseries_location,
                              p_resource->pci_bus_number,
                              p_resource->pci_slot,
                              p_resource->host_no);
            }
            else
            {
                len = sprintf(buffer, " 2  %s    %8s     %x-001   %d    %d     %d",
                              p_cur_ioa->ioa.dev_name,
                              p_cur_ioa->serial_num,
                              p_cur_ioa->ccin,
                              p_resource->pci_bus_number,
                              p_resource->pci_slot,
                              p_resource->host_no);
            }

            set_field_buffer(pp_field[field_index], 0, buffer);
            field_opts_off(pp_field[field_index], O_EDIT);
            set_field_userptr(pp_field[field_index++], NULL);

            memset(buffer, 0, 100);
            len = 0;
            num_lines++;
            ioa_num++;
        }
        p_cur_ioa = p_cur_ioa->p_next;
    }

    field_opts_off(p_fields[1], O_ACTIVE);

    ioa_num = max_size;
    for (i = ioa_num; i < field_index; i += ioa_num)
        set_new_page(pp_field[i], TRUE);

    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
    pp_field[field_index] = NULL;

    p_list_form = new_form(pp_field);

    set_form_win(p_list_form, p_field_win);

    post_form(p_list_form);

    wnoutrefresh(stdscr);
    wnoutrefresh(p_menu_win);
    wnoutrefresh(p_title_win);

    while(1)
    {
	wnoutrefresh(p_field_win);
        doupdate();

        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else if (IS_PGDN_KEY(ch))
            {
                init_page = form_page(p_list_form);
                form_driver(p_list_form,
                            REQ_NEXT_PAGE);

                new_page = form_page(p_list_form);

                if (new_page > init_page)
                {
                }
                else
                {
                    form_driver(p_list_form,
                                REQ_PREV_PAGE);
                }
                break;
            }
            else if (IS_PGUP_KEY(ch))
            {
                init_page = form_page(p_list_form);
                form_driver(p_list_form,
                            REQ_PREV_PAGE);

                new_page = form_page(p_list_form);

                if (new_page < init_page)
                {
                }
                else
                {
                    form_driver(p_list_form,
                                REQ_NEXT_PAGE);
                }
                break;
            }
            else if ((ch == 'c') ||
                     (ch == 'C'))
            {
                p_cur_ioa = p_head_ipr_ioa;

                while(p_cur_ioa)
                {
                    if ((p_cur_ioa->p_reclaim_data) &&
                        (p_cur_ioa->ioa.is_reclaim_cand))
                    {
                        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

                        if (fd <= 1)
                        {
                            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL,
                                   p_cur_ioa->ioa.dev_name);
                            reclaim_rc = RC_FAILED;
                            p_cur_ioa = p_cur_ioa->p_next;
                            continue;
                        }

                        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

                        ioa_cmd.buffer = &reclaim_buffer;
                        ioa_cmd.buffer_len = sizeof(struct ipr_reclaim_query_data);
                        ioa_cmd.cdb[1] = IPR_RECLAIM_FORCE_BATTERY_ERROR;
                        ioa_cmd.read_not_write = 1;

                        rc = ipr_ioctl(fd, IPR_RECLAIM_CACHE_STORE, &ioa_cmd);

                        close(fd);
                        if (rc != 0)
                        {
                            if (errno != EINVAL)
                            {
                                syslog(LOG_ERR,
                                       "Reclaim to %s for Battery Maintenance failed. %m"IPR_EOL,
                                       p_cur_ioa->ioa.dev_name);
                                reclaim_rc = RC_FAILED;
                            }
                        }

                        if (reclaim_buffer.action_status !=
                            IPR_ACTION_SUCCESSFUL)
                        {
                            syslog(LOG_ERR,
                                   "Battery Force Error to %s failed. %m"IPR_EOL,
                                   p_cur_ioa->ioa.dev_name);
                            reclaim_rc = RC_FAILED;
                        }
                    }
                    p_cur_ioa = p_cur_ioa->p_next;
                }
                return reclaim_rc;
            }
            else
            {
                /* Terminal is running under 5250 telnet */
                if (term_is_5250)
                {
                    /* Send it to the form to be processed and flush this line */
                    form_driver(p_list_form, ch);
                    last_char = flush_line();
                    if (IS_5250_CHAR(last_char))
                        ungetch(ENTER_KEY);
                    else if (last_char != ERR)
                        ungetch(last_char);
                }
                else
                {
                    form_driver(p_list_form, ch);
                }
                break;
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(pp_field);
    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int show_battery_info(struct ipr_ioa *p_ioa)
{
    /*
                                      Battery Information                


         Resource Name . . . . . . . . . . . : /dev/ipr5
         Serial Number . . . . . . . . . . . : 02137002
         Type-model  . . . . . . . . . . . . : 2757-001      
         Frame ID  . . . . . . . . . . . . . : 2                 
         Card position . . . . . . . . . . . : C02           
         Battery type  . . . . . . . . . . . : Lithium Ion (LiIon)
         Battery state . . . . . . . . . . . : No battery warning
         Power on time (days)  . . . . . . . : 3             
         Adjusted power on time (days) . . . : 3             
         Estimated time to warning (days)  . : 939
         Estimated time to error (days)  . . : 1030

         Press Enter to continue.

         e=Exit         q=Cancel
     */

    FIELD *p_fields[3];
    FORM *p_form;
    FORM *p_old_form;
    int rc;
    struct ipr_resource_entry *p_resource =
        p_ioa->ioa.p_resource_entry;
    int field_index;
    struct ipr_reclaim_query_data *p_reclaim_data =
        p_ioa->p_reclaim_data;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 1,   /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0], 0, "Battery Information");

    post_form(p_form);

    field_index = 3;

    mvprintw(field_index++, 1, "Resource Name . . . . . . . . . . . : %s", p_ioa->ioa.dev_name);
    mvprintw(field_index++, 1, "Serial Number . . . . . . . . . . . : %-8s", p_resource->serial_num);
    mvprintw(field_index++, 1, "Type-model  . . . . . . . . . . . . : %X-001", p_ioa->ccin);

    if (platform == IPR_ISERIES)
    {
        mvprintw(field_index++, 1, "Frame ID  . . . . . . . . . . . . . : %s", p_resource->frame_id);
        mvprintw(field_index++, 1, "Card position . . . . . . . . . . . : %s", p_resource->slot_label);
    }
    else
    {
        if (platform == IPR_PSERIES)
            mvprintw(field_index++, 1,
                     "Location  . . . . . . . . . . . . . : %s", p_resource->pseries_location);

        mvprintw(field_index++, 1, "PCI Bus . . . . . . . . . . . . . . : %d", p_resource->pci_bus_number);
        mvprintw(field_index++, 1, "PCI Device  . . . . . . . . . . . . : %d", p_resource->pci_slot);
        mvprintw(field_index++, 1, "SCSI Host Number  . . . . . . . . . : %d", p_resource->host_no);
    }

    switch (p_reclaim_data->rechargeable_battery_type)
    {
        case IPR_BATTERY_TYPE_NICD:
            mvprintw(field_index++, 1, "Battery type  . . . . . . . . . . . : Nickel Cadmium (NiCd)");
            break;
        case IPR_BATTERY_TYPE_NIMH:
            mvprintw(field_index++, 1, "Battery type  . . . . . . . . . . . : Nickel Metal Hydride (NiMH)");
            break;
        case IPR_BATTERY_TYPE_LIION:
            mvprintw(field_index++, 1, "Battery type  . . . . . . . . . . . : Lithium Ion (LiIon)");
            break;
        default:
            break;
    }

    switch (p_reclaim_data->rechargeable_battery_error_state)
    {
        case IPR_BATTERY_NO_ERROR_STATE:
            mvprintw(field_index++, 1, "Battery state . . . . . . . . . . . : No battery warning");
            break;
        case IPR_BATTERY_WARNING_STATE:
            mvprintw(field_index++, 1, "Battery state . . . . . . . . . . . : Battery warning issued");
            break;
        case IPR_BATTERY_ERROR_STATE:
            mvprintw(field_index++, 1, "Battery state . . . . . . . . . . . : Battery error issued");
            break;
        default:
            break;
    }

    mvprintw(field_index++, 1, "Power on time (days)  . . . . . . . : %d",
             p_reclaim_data->raw_power_on_time);
    mvprintw(field_index++, 1, "Adjusted power on time (days) . . . : %d",
             p_reclaim_data->adjusted_power_on_time);
    mvprintw(field_index++, 1, "Estimated time to warning (days)  . : %d",
             p_reclaim_data->estimated_time_to_battery_warning);   
    mvprintw(field_index++, 1, "Estimated time to error (days)  . . : %d",
             p_reclaim_data->estimated_time_to_battery_failure);

    mvprintw(curses_info.max_y - 4, 1, "Press Enter to continue.");
    mvprintw(curses_info.max_y - 2, 1, EXIT_KEY_LABEL"=Exit         "CANCEL_KEY_LABEL"=Cancel");

    while(1)
    {
        form_driver(p_form,
                    REQ_LAST_FIELD);
        refresh();

        while(1)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
        }
    }
    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free_screen(NULL, NULL, p_fields);

    flush_stdscr();
    return rc;
}

void configure_options()
{
    /*
                                   Work with Configuration

       Select one of the following:

            1. Work with SCSI bus configuration
            2. Work with driver configuration
            3. Work with disk unit configuration





       Selection:

       e=Exit  q=Cancel
     */
    FIELD *p_fields[12];
    FORM *p_form, *p_old_form;
    int invalid = 0;
    int rc = RC_SUCCESS;
    int j;
    int bus_config();
    int driver_config();

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 2,   /* new field size */ 
                            12, 12,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Work with Configuration");          /* string value to set */

    flush_stdscr();

    clear();

    post_form(p_form);

    while(1)
    {
        mvaddstr(2, 0, "Select one of the following:");
        mvaddstr(4, 0, "     1. Work with SCSI bus configuration");
        mvaddstr(5, 0, "     2. Work with driver configuration");
        mvaddstr(6, 0, "     3. Work with disk unit configuration");
        mvaddstr(12, 0, "Selection: ");
        mvaddstr(curses_info.max_y - 2, 0, EXIT_KEY_LABEL"=Exit  "CANCEL_KEY_LABEL"=Cancel");

        if (invalid)
        {
            invalid = 0;
            mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
        }

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        refresh();
        for (;;)
        {
            int ch = getch();

            if (IS_ENTER_KEY(ch))
            {
                char *p_char;
                refresh();
                unpost_form(p_form);
                p_char = field_buffer(p_fields[1], 0);

                switch(p_char[0])
                {
                    case '1':  /* Work with scsi bus attributes */
                        while (RC_REFRESH == (rc = bus_config()))
                        if (RC_EXIT == rc)
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_FAILED)
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " Change SCSI Bus configurations failed");
                        else if (rc == RC_SUCCESS)
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " Change SCSI Bus configurations completed successfully");
                        break;
                    case '2':  /* Work with device driver configuration */
                        if (RC_EXIT == (rc = driver_config()))
                            goto leave;
                        clear();
                        post_form(p_form);
                        if (rc == RC_FAILED)
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " Change Device Driver Configurations failed");
                        else if (rc == RC_SUCCESS)
                            mvaddstr(curses_info.max_y - 1, 0,
                                     " Change Device Driver Configurations completed successfully");
                        break;
                    case '3': /* Work with device configuration */
                        while (RC_REFRESH == (rc = configure_device()));
                        if (rc == RC_EXIT)
                            goto leave;
                        clear();
                        post_form(p_form);
                        break;
                    case ' ':
                        clear();
                        post_form(p_form);
                        break;
                    default:
                        clear();
                        post_form(p_form);
                        mvaddstr(curses_info.max_y - 1, 0, "Invalid option specified.");
                        break;
                }
                form_driver(p_form, ' ');
                refresh();
                break;
            }
            else if (IS_EXIT_KEY(ch))
                goto leave;
            else if (IS_CANCEL_KEY(ch))
                goto leave;
            else
            {
                form_driver(p_form, ch);
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                refresh();
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    for (j=0; j < 2; j++)
        free_field(p_fields[j]);
    flush_stdscr();
    return;
}


int bus_config()
{
/* iSeries
                        Work with SCSI Bus Configuration

Select the subsystem to change scsi bus attribute.

Type choice, press Enter.
  1=change scsi bus attribute

          Serial                Resource
Option    Number   Type  Model  Name
         02137002  2757   001   /dev/ipr6
         03060044  5703   001   /dev/ipr5
         02127005  2782   001   /dev/ipr4
         02123028  5702   001   /dev/ipr3

e=Exit   q=Cancel    f=PageDn   b=PageUp
*/
/* non-iSeries
                        Work with SCSI Bus Configuration

Select the subsystem to change scsi bus attribute.

Type choice, press Enter.
  1=change scsi bus attribute

         Vendor    Product            Serial     PCI    PCI
Option    ID         ID               Number     Bus    Dev
         IBM       2780001           3AFB348C     05     03
         IBM       5703001           A775915E     04     01

e=Exit   q=Cancel    f=PageDn   b=PageUp
*/
    int rc, i, config_num, last_char;
    struct array_cmd_data *p_cur_raid_cmd;
    FIELD *p_fields[3];
    FORM *p_form, *p_list_form, *p_old_form;
    char buffer[100];
    int array_index;
    int max_size = curses_info.max_y - 12;
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len;
    WINDOW *p_title_win, *p_menu_win, *p_field_win;
    FIELD **pp_field = NULL;
    int start_y;
    int field_index = 0;
    int found = 0;
    char *p_char;
    int ch;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    u8 ioctl_buffer[255];
    struct ipr_mode_parm_hdr *p_mode_parm_header;
    struct ipr_mode_page_28_header *p_modepage_28_header;
    int fd;

    int change_bus_attr(struct ipr_ioa *p_cur_ioa);

    rc = RC_SUCCESS;

    p_title_win = newwin(9, curses_info.max_x, 0, 0);
    p_menu_win = newwin(3, curses_info.max_x, max_size + 9, 0);
    p_field_win = newwin(max_size, curses_info.max_x, 9, 0);

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    p_fields[1] = new_field(1, 1,  /* new field size */ 
                            1, 0,       /* upper left corner */
                            0,          /* number of offscreen rows */
                            0);         /* number of working buffers */

    p_fields[2] = NULL;

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_fields[0],                        /* field to alter */
                     0,                                  /* number of buffer to alter */
                     "Work with SCSI Bus Configuration");  /* string value to set */

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_form_win(p_form, p_title_win);

    post_form(p_form);

    config_num = 0;

    check_current_config(false);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        /* mode sense page28 to focal point */
        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));
        p_mode_parm_header = (struct ipr_mode_parm_hdr *)ioctl_buffer;

        ioa_cmd.buffer = p_mode_parm_header;
        ioa_cmd.buffer_len = 255;
        ioa_cmd.cdb[2] = 0x28;
        ioa_cmd.read_not_write = 1;
        ioa_cmd.driver_cmd = 1;

        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            continue;
        }

        rc = ipr_ioctl(fd, IPR_MODE_SENSE_PAGE_28, &ioa_cmd);
        close(fd);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
                   p_cur_ioa->ioa.dev_name);
            continue;
        }

        p_modepage_28_header = (struct ipr_mode_page_28_header *) (p_mode_parm_header + 1);

        if (p_modepage_28_header->num_dev_entries == 0)
            continue;

        start_y = (config_num) % (max_size - 1);

        if ((num_lines > 0) && (start_y == 0))
        {
            pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
            pp_field[field_index] = new_field(1, curses_info.max_x,
                                              max_size - 1,
                                              0,
                                              0, 0);

            set_field_just(pp_field[field_index], JUSTIFY_RIGHT);
            set_field_buffer(pp_field[field_index], 0, "More... ");
            set_field_userptr(pp_field[field_index], NULL);
            field_opts_off(pp_field[field_index++],
                           O_ACTIVE);
        }

        /* User entry field */
        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = new_field(1, 1, start_y, 3, 0, 0);

        set_field_userptr(pp_field[field_index++], (char *)p_cur_ioa);

        /* Description field */
        pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
        pp_field[field_index] = new_field(1, curses_info.max_x - 9, start_y, 9, 0, 0);

        if (platform == IPR_ISERIES)
        {
            len = sprintf(buffer, "%8s  %x   001   %s",
                          p_cur_ioa->serial_num,
                          p_cur_ioa->ccin,
                          p_cur_ioa->ioa.dev_name);
        }
        else
        {
            char buf[100];

            memcpy(buf, p_cur_ioa->ioa.p_resource_entry->std_inq_data.vpids.vendor_id,
                   IPR_VENDOR_ID_LEN);
            buf[IPR_VENDOR_ID_LEN] = '\0';

            len = sprintf(buffer, "%-8s  ", buf);

            memcpy(buf, p_cur_ioa->ioa.p_resource_entry->std_inq_data.vpids.product_id,
                   IPR_PROD_ID_LEN);
            buf[IPR_PROD_ID_LEN] = '\0';

            len += sprintf(buffer + len, "%-12s  ", buf);

            len += sprintf(buffer + len, "%8s     ",
                           p_cur_ioa->ioa.p_resource_entry->serial_num);

            len += sprintf(buffer + len, "%02d     %02d",
                           p_cur_ioa->ioa.p_resource_entry->pci_bus_number,
                           p_cur_ioa->ioa.p_resource_entry->pci_slot);
        }

        set_field_buffer(pp_field[field_index], 0, buffer);
        field_opts_off(pp_field[field_index], O_ACTIVE);
        set_field_userptr(pp_field[field_index++], NULL);

        memset(buffer, 0, 100);

        len = 0;
        num_lines++;
        config_num++;
    }

    if (config_num == 0)
    {
        set_field_buffer(p_fields[0],                       /* field to alter */
                         0,                                 /* number of buffer to alter */
                         "Work with SCSI Bus Configuration Failed");  /* string value to set */

        mvwaddstr(p_title_win, 2, 0, "There are no SCSI Buses eligible for the selected operation.");
        mvwaddstr(p_title_win, 3, 0, "None of the installed adapters support the requested");
        mvwaddstr(p_title_win, 4, 0, "operation.");
        mvwaddstr(p_field_win, max_size -1, 0, "Press Enter to continue.");
        mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD);     /* form request code */

        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        wnoutrefresh(p_field_win);
        doupdate();

        for (;;)
        {
            ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
        }
    }

    mvwaddstr(p_title_win, 2, 0, "Select the subsystem to change scsi bus attribute.");
    mvwaddstr(p_title_win, 4, 0, "Type choice, press Enter.");
    mvwaddstr(p_title_win, 5, 0, "  1=change scsi bus attribute");

    if (platform == IPR_ISERIES)
    {
        mvwaddstr(p_title_win, 7, 0, "          Serial                Resource");
        mvwaddstr(p_title_win, 8, 0, "Option    Number   Type  Model  Name");
    }
    else
    {
        mvwaddstr(p_title_win, 7, 0, "         Vendor    Product            Serial     PCI    PCI");
        mvwaddstr(p_title_win, 8, 0, "Option    ID         ID               Number     Bus    Dev");
    }

    mvwaddstr(p_menu_win, 1, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel    f=PageDn   b=PageUp");


    field_opts_off(p_fields[1], O_ACTIVE);

    array_index = ((max_size - 1) * 2) + 1;
    for (i = array_index; i < field_index; i += array_index)
        set_new_page(pp_field[i], TRUE);

    pp_field = realloc(pp_field, sizeof(FIELD **) * (field_index + 1));
    pp_field[field_index] = NULL;

    flush_stdscr();

    p_list_form = new_form(pp_field);

    set_form_win(p_list_form, p_field_win);

    post_form(p_list_form);

    form_driver(p_list_form,
                REQ_FIRST_FIELD);

    while(1)
    {
        wnoutrefresh(stdscr);
        wnoutrefresh(p_menu_win);
        wnoutrefresh(p_title_win);
        wnoutrefresh(p_field_win);
        doupdate();

        ch = getch();

        if (IS_EXIT_KEY(ch))
        {
            rc = RC_EXIT;
            goto leave;
        }
        else if (IS_CANCEL_KEY(ch))
        {
            rc = RC_CANCEL;
            goto leave;
        }
        else if (IS_PGDN_KEY(ch))
        {
            form_driver(p_list_form,
                        REQ_NEXT_PAGE);
            form_driver(p_list_form,
                        REQ_FIRST_FIELD);
        }
        else if (IS_PGUP_KEY(ch))
        {
            form_driver(p_list_form,
                        REQ_PREV_PAGE);
            form_driver(p_list_form,
                        REQ_FIRST_FIELD);
        }
        else if ((ch == KEY_DOWN) ||
                 (ch == '\t'))
        {
            form_driver(p_list_form,
                        REQ_NEXT_FIELD);
        }
        else if (ch == KEY_UP)
        {
            form_driver(p_list_form,
                        REQ_PREV_FIELD);
        }
        else if (IS_ENTER_KEY(ch))
        {
            found = 0;
            unpost_form(p_list_form);

            for (i = 0; i < field_index; i++)
            {
                p_cur_ioa = (struct ipr_ioa *)field_userptr(pp_field[i]);
                if (p_cur_ioa != NULL)
                {
                    p_char = field_buffer(pp_field[i], 0);
                    if (strcmp(p_char, "1") == 0)
                    {
                        rc = change_bus_attr(p_cur_ioa);

                        if (rc == RC_EXIT)
                            goto leave;
                    }
                }
            }

            if (rc == RC_CANCEL)
                rc = RC_REFRESH;
            goto leave;
        }
        else
        {
            /* Terminal is running under 5250 telnet */
            if (term_is_5250)
            {
                /* Send it to the form to be processed and flush this line */
                form_driver(p_list_form, ch);
                last_char = flush_line();
                if (IS_5250_CHAR(last_char))
                    ungetch(ENTER_KEY);
                else if (last_char != ERR)
                    ungetch(last_char);
            }
            else
            {
                form_driver(p_list_form, ch);
            }
        }
    }

    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    free(pp_field);

    p_cur_raid_cmd = p_head_raid_cmd;
    while(p_cur_raid_cmd)
    {
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }

    delwin(p_title_win);
    delwin(p_menu_win);
    delwin(p_field_win);
    free_screen(NULL, NULL, p_fields);
    flush_stdscr();
    return rc;
}

int change_bus_attr(struct ipr_ioa *p_cur_ioa)
{
    /*
                            Change SCSI Bus Configuration

     Current Bus configurations are shown.  To change
     setting hit "c" for options menu.  Highlight desired
     option then hit Enter

     c=Change Setting

     /dev/ipr1 
      BUS 0
        Wide Enabled . . . . . . . . . . . . :  Yes
        Maximum Bus Throughput . . . . . . . :  40 MB/s
      BUS 1
        Wide Enabled . . . . . . . . . . . . :  Yes
        Maximum Bus Throughput . . . . . . . :  40 MB/s

     Press Enter to Continue

     e=Exit   q=Cancel
     */
#define DSP_FIELD_START_ROW 9
    int rc, j, i;
    FORM *p_form, *p_old_form;
    FORM *p_fields_form;
    FIELD **p_input_fields, *p_cur_field;
    FIELD *p_title_fields[3];
    WINDOW *p_field_pad;
    int cur_field_index;
    char buffer[100];
    int input_field_index = 0;
    int num_lines = 0;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    u8 ioctl_buffer0[255];
    u8 ioctl_buffer1[255];
    u8 ioctl_buffer2[255];
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    int fd;
    struct ipr_pagewh_28 *p_page_28_sense;
    struct ipr_page_28 *p_page_28_cur, *p_page_28_chg, *p_page_28_dflt;
    struct ipr_page_28 page_28_ipr;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_resource_table *p_res_table;
    char scsi_id_str[5][16];
    char max_xfer_rate_str[5][16];
    int  ch;
    int form_rows, form_cols;
    int field_start_row, field_end_row, field_num_rows;
    int pad_start_row;
    int form_adjust = 0;
    int confirm = 0;
    int is_reset_req;
    int bus_attr_menu(struct ipr_ioa *p_cur_ioa,
                      struct ipr_page_28 *p_page_28_cur,
                      struct ipr_page_28 *p_page_28_chg,
                      struct ipr_page_28 *p_page_28_dflt,
                      struct ipr_page_28 *p_page_28_ipr,
                      int cur_field_index,
                      int offset);

    rc = RC_SUCCESS;

    /* zero out user page 28 page, this data is used to indicate
     which fields the user changed */
    memset(&page_28_ipr, 0, sizeof(struct ipr_page_28));

    /* mode sense page28 to get current parms */
    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));
    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer0;

    ioa_cmd.buffer = p_mode_parm_hdr;
    ioa_cmd.buffer_len = 255;
    ioa_cmd.cdb[2] = 0x28;
    ioa_cmd.read_not_write = 1;
    ioa_cmd.driver_cmd = 1;

    fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
        return RC_FAILED;
    }

    rc = ipr_ioctl(fd, IPR_MODE_SENSE_PAGE_28, &ioa_cmd);

    if (rc != 0)
    {
        close(fd);
        syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
               p_cur_ioa->ioa.dev_name);
        return RC_FAILED;
    }

    p_page_28_sense = (struct ipr_pagewh_28 *)p_mode_parm_hdr;
    p_page_28_cur = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* now issue mode sense to get changeable parms */
    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer1;

    ioa_cmd.buffer = p_mode_parm_hdr;
    ioa_cmd.buffer_len = 255;
    ioa_cmd.cdb[2] = 0x68;

    rc = ipr_ioctl(fd, IPR_MODE_SENSE_PAGE_28, &ioa_cmd);

    if (rc != 0)
    {
        close(fd);
        syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
               p_cur_ioa->ioa.dev_name);
        return RC_FAILED;
    }

    p_page_28_chg = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* now Issue mode sense to get default parms */
    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer2;

    ioa_cmd.buffer = p_mode_parm_hdr;
    ioa_cmd.buffer_len = 255;
    ioa_cmd.cdb[2] = 0xA8;

    rc = ipr_ioctl(fd, IPR_MODE_SENSE_PAGE_28, &ioa_cmd);
    close(fd);

    if (rc != 0)
    {
        syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
               p_cur_ioa->ioa.dev_name);
        return RC_FAILED;
    }

    p_page_28_dflt = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* Title */
    p_title_fields[0] =
        new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                  0, 0,       /* upper left corner */
                  0,           /* number of offscreen rows */
                  0);               /* number of working buffers */

    p_title_fields[1] =
        new_field(1, 1,   /* new field size */ 
                  1, 0,       /* upper left corner */
                  0,           /* number of offscreen rows */
                  0);               /* number of working buffers */

    p_title_fields[2] = NULL;

    set_field_just(p_title_fields[0], JUSTIFY_CENTER);

    set_field_buffer(p_title_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Change SCSI Bus Configuration");  /* string value to set     */

    field_opts_off(p_title_fields[0], O_ACTIVE);

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_title_fields);

    p_input_fields = malloc(sizeof(FIELD *));

    /* Loop for each device bus entry */
    for (j = 0,
         num_lines = 0;
         j < p_page_28_cur->page_hdr.num_dev_entries;
         j++)
    {
        p_input_fields = realloc(p_input_fields,
                                 sizeof(FIELD *) * (input_field_index + 1));
        p_input_fields[input_field_index] =
            new_field(1, 6,
                      num_lines, 2,
                      0, 0);

        field_opts_off(p_input_fields[input_field_index], O_ACTIVE);
        memset(buffer, 0, 100);
        sprintf(buffer,"BUS %d",
                p_page_28_cur->attr[j].res_addr.bus);

        set_field_buffer(p_input_fields[input_field_index++],
                         0,
                         buffer);

        num_lines++;
        if (p_page_28_chg->attr[j].qas_capability)
        {
            /* options are ENABLED(10)/DISABLED(01) */
            p_input_fields = realloc(p_input_fields,
                                     sizeof(FIELD *) * (input_field_index + 2));
            p_input_fields[input_field_index] =
                new_field(1, 38,
                          num_lines, 4,
                          0, 0);

            field_opts_off(p_input_fields[input_field_index], O_ACTIVE);
            set_field_buffer(p_input_fields[input_field_index++],
                             0,
                             "QAS Capability . . . . . . . . . . . :");

            p_input_fields[input_field_index++] =
                new_field(1, 9,   /* new field size */ 
                          num_lines, 44,       /*  */
                          0,           /* number of offscreen rows */
                          0);          /* number of working buffers */
            num_lines++;
        }
        if (p_page_28_chg->attr[j].scsi_id)
        {
            /* options are based on current scsi ids currently in use
             value must be 15 or less */
            p_input_fields = realloc(p_input_fields,
                                     sizeof(FIELD *) * (input_field_index + 2));
            p_input_fields[input_field_index] =
                new_field(1, 38,
                          num_lines, 4,
                          0, 0);

            field_opts_off(p_input_fields[input_field_index], O_ACTIVE);
            set_field_buffer(p_input_fields[input_field_index++],
                             0,
                             "Host SCSI ID . . . . . . . . . . . . :");

            p_input_fields[input_field_index++] =
                new_field(1, 3,   /* new field size */ 
                          num_lines, 44,       /*  */
                          0,           /* number of offscreen rows */
                          0);          /* number of working buffers */
            num_lines++;
        }
        if (p_page_28_chg->attr[j].bus_width)
        {
            /* check if 8 bit bus is allowable with current configuration before
               enabling option */
            p_res_table = p_cur_ioa->p_resource_table;

            for (i=0;
                 i < p_res_table->hdr.num_entries;
                 i++)
            {
                p_resource_entry = &p_res_table->dev[i];
                if ((!p_resource_entry->is_ioa_resource)
                    && (p_resource_entry->resource_address.target & 0xF8)
                    && (p_page_28_cur->attr[j].res_addr.bus ==
                        p_resource_entry->resource_address.bus))
                {
                    p_page_28_chg->attr[j].bus_width = 0;
                    break;
                }
            }

        }
        if (p_page_28_chg->attr[j].bus_width)
        {
            /* options for "Wide Enabled" (bus width), No(8) or Yes(16) bits wide */
            p_input_fields = realloc(p_input_fields,
                                     sizeof(FIELD *) * (input_field_index + 2));
            p_input_fields[input_field_index] =
                new_field(1, 38,
                          num_lines, 4,
                          0, 0);

            field_opts_off(p_input_fields[input_field_index], O_ACTIVE);
            set_field_buffer(p_input_fields[input_field_index++],
                             0,
                             "Wide Enabled . . . . . . . . . . . . :");

            p_input_fields[input_field_index++] =
                new_field(1, 4,   /* new field size */ 
                          num_lines, 44,       /*  */
                          0,           /* number of offscreen rows */
                          0);          /* number of working buffers */
            num_lines++;
        }
        if (p_page_28_chg->attr[j].max_xfer_rate)
        {
            /* options are 5, 10, 20, 40, 80, 160, 320 */
            p_input_fields = realloc(p_input_fields,
                                     sizeof(FIELD *) * (input_field_index + 2));
            p_input_fields[input_field_index] =
                new_field(1, 38,
                          num_lines, 4,
                          0, 0);

            field_opts_off(p_input_fields[input_field_index], O_ACTIVE);
            set_field_buffer(p_input_fields[input_field_index++],
                             0,
                             "Maximum Bus Throughput . . . . . . . :");

            p_input_fields[input_field_index++] =
                new_field(1, 9,   /* new field size */ 
                          num_lines, 44,       /*  */
                          0,           /* number of offscreen rows */
                          0);          /* number of working buffers */
            num_lines++;
        }
    }

    p_input_fields[input_field_index] = NULL;

    p_fields_form = new_form(p_input_fields);
    scale_form(p_fields_form, &form_rows, &form_cols);
    p_field_pad = newpad(form_rows, form_cols);
    set_form_win(p_fields_form, p_field_pad);
    set_current_field(p_fields_form, p_input_fields[0]);
    for (i = 0; i < input_field_index; i++)
    {
        field_opts_off(p_input_fields[i], O_EDIT);
    }

    form_opts_off(p_fields_form, O_BS_OVERLOAD);

    flush_stdscr();
    clear();
    post_form(p_form);
    post_form(p_fields_form);

    field_start_row = DSP_FIELD_START_ROW;
    field_end_row = curses_info.max_y - 4;
    field_num_rows = field_end_row - field_start_row + 1;

    mvaddstr(2, 1, "Current Bus configurations are shown.  To change");
    mvaddstr(3, 1, "setting hit \"c\" for options menu.  Highlight desired");
    mvaddstr(4, 1, "option then hit Enter");
    mvaddstr(6, 1, "c=Change Setting");
    if (field_num_rows < form_rows)
    {
        mvaddstr(field_end_row, 43, "More...");
        mvaddstr(curses_info.max_y - 2, 25, "f=PageDn");
        field_end_row--;
        field_num_rows--;
    }
    mvaddstr(curses_info.max_y - 3, 1, "Press Enter to Continue");
    mvaddstr(curses_info.max_y - 2, 1, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

    memset(buffer, 0, 100);
    sprintf(buffer,"%s",
            p_cur_ioa->ioa.dev_name);
    mvaddstr(8, 1, buffer);

    refresh();
    pad_start_row = 0;
    pnoutrefresh(p_field_pad,
                 pad_start_row, 0,
                 field_start_row, 0,
                 field_end_row,
                 curses_info.max_x);
    doupdate();

    form_driver(p_fields_form,
                REQ_FIRST_FIELD);

    while(1)
    {
        /* Loop for each device bus entry */
        input_field_index = 0;
        for (j = 0,
             num_lines = 0;
             j < p_page_28_cur->page_hdr.num_dev_entries;
             j++)
        {
            num_lines++;
            input_field_index++;
            if (p_page_28_chg->attr[j].qas_capability)
            {
                input_field_index++;
                if ((num_lines >= pad_start_row) &&
                    (num_lines < (pad_start_row + field_num_rows)))
                {
                    field_opts_on(p_input_fields[input_field_index],
                                  O_ACTIVE);
                }
                else
                {
                    field_opts_off(p_input_fields[input_field_index],
                                   O_ACTIVE);
                }
                if (p_page_28_cur->attr[j].qas_capability ==
                    IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL)
                {
                    if (page_28_ipr.attr[j].qas_capability)
                        set_field_buffer(p_input_fields[input_field_index++],
                                         0,
                                         "Disable");
                    else
                        set_field_buffer(p_input_fields[input_field_index++],
                                         0,
                                         "Disabled");

                }
                else
                {
                    if (page_28_ipr.attr[j].qas_capability)
                        set_field_buffer(p_input_fields[input_field_index++],
                                         0,
                                         "Enable");
                    else
                        set_field_buffer(p_input_fields[input_field_index++],
                                         0,
                                         "Enabled");
                }
                num_lines++;
            }
            if (p_page_28_chg->attr[j].scsi_id)
            {
                input_field_index++;
                if ((num_lines >= pad_start_row) &&
                    (num_lines < (pad_start_row + field_num_rows)))
                {
                    field_opts_on(p_input_fields[input_field_index],
                                  O_ACTIVE);
                }
                else
                {
                    field_opts_off(p_input_fields[input_field_index],
                                   O_ACTIVE);
                }
                sprintf(scsi_id_str[j],"%d",p_page_28_cur->attr[j].scsi_id);
                set_field_buffer(p_input_fields[input_field_index++],
                                 0,
                                 scsi_id_str[j]);
                num_lines++;
            }
            if (p_page_28_chg->attr[j].bus_width)
            {
                input_field_index++;
                if ((num_lines >= pad_start_row) &&
                    (num_lines < (pad_start_row + field_num_rows)))
                {
                    field_opts_on(p_input_fields[input_field_index],
                                  O_ACTIVE);
                }
                else
                {
                    field_opts_off(p_input_fields[input_field_index],
                                   O_ACTIVE);
                }
                if (p_page_28_cur->attr[j].bus_width == 16)
                {
                    set_field_buffer(p_input_fields[input_field_index++],
                                     0,
                                     "Yes");
                }
                else
                {
                    set_field_buffer(p_input_fields[input_field_index++],
                                     0,
                                     "No");
                }
                num_lines++;
            }
            if (p_page_28_chg->attr[j].max_xfer_rate)
            {
                input_field_index++;
                if ((num_lines >= pad_start_row) &&
                    (num_lines < (pad_start_row + field_num_rows)))
                {
                    field_opts_on(p_input_fields[input_field_index],
                                  O_ACTIVE);
                }
                else
                {
                    field_opts_off(p_input_fields[input_field_index],
                                   O_ACTIVE);
                }
                sprintf(max_xfer_rate_str[j],"%d MB/s",
                        (iprtoh32(p_page_28_cur->attr[j].max_xfer_rate) *
			 p_page_28_cur->attr[j].bus_width)/(10 * 8));
                set_field_buffer(p_input_fields[input_field_index++],
                                 0,
                                 max_xfer_rate_str[j]);
                num_lines++;
            }
        }

        if (form_adjust)
        {
            form_driver(p_fields_form,
                        REQ_NEXT_FIELD);
            form_adjust = 0;
            refresh();
        }

        prefresh(p_field_pad,
                 pad_start_row, 0,
                 field_start_row, 0,
                 field_end_row,
                 curses_info.max_x);
        doupdate();
        ch = getch();

        if (IS_EXIT_KEY(ch))
        {
            rc = RC_EXIT;
            goto leave;
        }
        else if (IS_CANCEL_KEY(ch))
        {
            rc = RC_CANCEL;
            goto leave;
        }
        else if (ch == 'c')
        {
            if (!confirm)
            {
                p_cur_field = current_field(p_fields_form);
                cur_field_index = field_index(p_cur_field);

                rc = bus_attr_menu(p_cur_ioa,
                                   p_page_28_cur,
                                   p_page_28_chg,
                                   p_page_28_dflt,
                                   &page_28_ipr,
                                   cur_field_index,
                                   pad_start_row);

                if (rc == RC_EXIT)
                    goto leave;
            }
            else
            {
                clear();
                set_field_buffer(p_title_fields[0],
                                 0,
                                 "Processing Change SCSI Bus Configuration");
                mvaddstr(curses_info.max_y - 2, 1, "Please wait for next screen");
                doupdate();
                refresh();

                /* issue mode select and reset if necessary */
                memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));
                ioa_cmd.buffer = p_page_28_sense;
                ioa_cmd.buffer_len = sizeof(struct ipr_pagewh_28);
                ioa_cmd.read_not_write = 0;
                ioa_cmd.driver_cmd = 1;

                fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
                if (fd <= 1)
                {
                    syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    return RC_FAILED;
                }

                rc = ipr_ioctl(fd, IPR_MODE_SELECT_PAGE_28, &ioa_cmd);

                if (rc != 0)
                {
                    close(fd);
                    syslog(LOG_ERR, "Mode Select to %s failed. %m"IPR_EOL,
                           p_cur_ioa->ioa.dev_name);
                    return RC_FAILED;
                }

                ipr_save_page_28(p_cur_ioa,
                                 p_page_28_cur,
                                 p_page_28_chg,
                                 &page_28_ipr);

                if (is_reset_req)
                {
                    rc = ipr_ioctl(fd, IPR_RESET_HOST_ADAPTER, &ioa_cmd);
                }

                close(fd);
                goto leave;
            }

        }
        else if (IS_ENTER_KEY(ch))
        {
            if (!confirm)
            {
                confirm = 1;
                clear();
                for (j = 0,
                     is_reset_req = 0;
                     j < p_page_28_cur->page_hdr.num_dev_entries;
                     j++)
                {
                    if (page_28_ipr.attr[j].scsi_id)
                        is_reset_req = 1;
                }

                set_field_buffer(p_title_fields[0],
                                 0,
                                 "Confirm Change SCSI Bus Configuration");
                mvaddstr(1, 1, "Confirming this action will result in the following configuration to");
                mvaddstr(2, 1, "become active.");
                if (is_reset_req)
                {
                    mvaddstr(3, 1, "ATTENTION:  This action requires a reset of the Host Adapter.  Confirming");
                    mvaddstr(4, 1, "this action may affect system performance while processing changes.");
                }
                mvaddstr(6, 1, "Press c=Confirm Change SCSI Bus Configuration ");
                if (field_num_rows < form_rows)
                {
                    mvaddstr(field_end_row + 1, 43, "More...");
                    mvaddstr(curses_info.max_y - 2, 25, "f=PageDn");
                }
                mvaddstr(curses_info.max_y - 3, 1, "c=Confirm");
                mvaddstr(curses_info.max_y - 2, 1, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

                memset(buffer, 0, 100);
                sprintf(buffer,"%s",
                        p_cur_ioa->ioa.dev_name);
                mvaddstr(8, 1, buffer);

                doupdate();
                pad_start_row = 0;
                form_adjust = 1;
            }
        }
        else if ((ch == 'f') ||
                 (ch == 'F'))
        {
            if ((form_rows - field_num_rows) > pad_start_row)
            {
                pad_start_row += field_num_rows;
                if ((form_rows - field_num_rows) < pad_start_row)
                {
                    pad_start_row = form_rows - field_num_rows;
                    mvaddstr(field_end_row + 1, 43, "       ");
                    mvaddstr(curses_info.max_y - 2, 25, "        ");
                }
                mvaddstr(curses_info.max_y - 2, 36, "b=PageUp");
                form_adjust = 1;
            }
        }
        else if ((ch == 'b') ||
                 (ch == 'B'))
        {
            if (pad_start_row != 0)
            {
                if ((form_rows - field_num_rows) <= pad_start_row)
                {
                    mvaddstr(field_end_row + 1, 43, "More...");
                }
                pad_start_row -= field_num_rows;
                if (pad_start_row < 0)
                {
                    pad_start_row = 0;
                    mvaddstr(curses_info.max_y - 2, 36, "        ");
                }
                form_adjust = 1;
                mvaddstr(curses_info.max_y - 2, 25, "f=PageDn");
            }
        }
        else if (ch == '\t')
            form_driver(p_fields_form, REQ_NEXT_FIELD);
        else if (ch == KEY_UP)
            form_driver(p_fields_form, REQ_PREV_FIELD);
        else if (ch == KEY_DOWN)
            form_driver(p_fields_form, REQ_NEXT_FIELD);
    }

    leave:

        free_form(p_form);
    free_screen(NULL, NULL, p_input_fields);
    curses_info.p_form = p_old_form;

    flush_stdscr();
    return rc;
}

int bus_attr_menu(struct ipr_ioa *p_cur_ioa,
                  struct ipr_page_28 *p_page_28_cur,  
                  struct ipr_page_28 *p_page_28_chg,
                  struct ipr_page_28 *p_page_28_dflt,
                  struct ipr_page_28 *p_page_28_ipr,
                  int cur_field_index,
                  int offset)
{
    int input_field_index;
    int start_row;
    int i, j, scsi_id, found;
    int num_menu_items;
    int menu_index;
    ITEM **menu_item = NULL;
    struct ipr_resource_entry *p_resource_entry;
    int *userptr;
    int *retptr;
    int rc = RC_SUCCESS;

    /* Loop for each device bus entry */
    input_field_index = 1;
    start_row = DSP_FIELD_START_ROW - offset;
    for (j = 0;
         j < p_page_28_cur->page_hdr.num_dev_entries;
         j++)
    {
        /* start_row represents the row in which the top most
         menu item will be placed.  Ideally this will be on the
         same line as the field in which the menu is opened for*/
        start_row++;
        input_field_index += 1;
        if ((p_page_28_chg->attr[j].qas_capability) &&
            (input_field_index == cur_field_index))
        {
            num_menu_items = 2;
            menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
            userptr = malloc(sizeof(int) * num_menu_items);

            menu_item[0] = new_item("Enable","");
            userptr[0] = 2;
            set_item_userptr(menu_item[0],
                             (char *)&userptr[0]);

            menu_item[1] = new_item("Disable","");
            userptr[1] = 1;
            set_item_userptr(menu_item[1],
                             (char *)&userptr[1]);

            menu_item[2] = (ITEM *)NULL;

            rc = display_menu(menu_item, start_row, menu_index, &retptr);
            if ((rc == RC_SUCCESS) &&
                (p_page_28_cur->attr[j].qas_capability != *retptr))
            {
                p_page_28_cur->attr[j].qas_capability = *retptr;
                p_page_28_ipr->attr[j].qas_capability =
                    p_page_28_chg->attr[j].qas_capability;
            }
            i=0;
            while (menu_item[i] != NULL)
                free_item(menu_item[i++]);
            free(menu_item);
            free(userptr);
            menu_item = NULL;
            break;
        }
        else if (p_page_28_chg->attr[j].qas_capability)
        {
            input_field_index += 2;
            start_row++;
        }

        if ((p_page_28_chg->attr[j].scsi_id) &&
            (input_field_index == cur_field_index))
        {
            num_menu_items = 16;
            menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
            userptr = malloc(sizeof(int) * num_menu_items);

            menu_index = 0;
            for (scsi_id = 0,
                 found = 0;
                 scsi_id < 16;
                 scsi_id++, found = 0)
            {
                for (i=0;
                     i < p_cur_ioa->p_resource_table->hdr.num_entries;
                     i++)
                {
                    p_resource_entry = &p_cur_ioa->p_resource_table->dev[i];
                    if ((scsi_id == p_resource_entry->resource_address.target) &&
                        (p_page_28_cur->attr[j].res_addr.bus ==
                         p_resource_entry->resource_address.bus))
                    {
                        found = 1;
                        break;
                    }
                }

                if (!found)
                {
                    switch (scsi_id)
                    {
                        case 0:
                            menu_item[menu_index] = new_item("0","");
                            userptr[menu_index] = 0;
                            set_item_userptr(menu_item[menu_index],
                                             (char *)&userptr[menu_index]);
                            menu_index++;
                            break;
                        case 1:
                            menu_item[menu_index] = new_item("1","");
                            userptr[menu_index] = 1;
                            set_item_userptr(menu_item[menu_index],
                                             (char *)&userptr[menu_index]);
                            menu_index++;
                            break;
                        case 2:
                            menu_item[menu_index] = new_item("2","");
                            userptr[menu_index] = 2;
                            set_item_userptr(menu_item[menu_index],
                                             (char *)&userptr[menu_index]);
                            menu_index++;
                            break;
                        case 3:
                            menu_item[menu_index] = new_item("3","");
                            userptr[menu_index] = 3;
                            set_item_userptr(menu_item[menu_index],
                                             (char *)&userptr[menu_index]);
                            menu_index++;
                            break;
                        case 4:
                            menu_item[menu_index] = new_item("4","");
                            userptr[menu_index] = 4;
                            set_item_userptr(menu_item[menu_index],
                                             (char *)&userptr[menu_index]);
                            menu_index++;
                            break;
                        case 5:
                            menu_item[menu_index] = new_item("5","");
                            userptr[menu_index] = 5;
                            set_item_userptr(menu_item[menu_index],
                                             (char *)&userptr[menu_index]);
                            menu_index++;
                            break;
                        case 6:
                            menu_item[menu_index] = new_item("6","");
                            userptr[menu_index] = 6;
                            set_item_userptr(menu_item[menu_index],
                                             (char *)&userptr[menu_index]);
                            menu_index++;
                            break;
                        case 7:
                            menu_item[menu_index] = new_item("7","");
                            userptr[menu_index] = 7;
                            set_item_userptr(menu_item[menu_index],
                                             (char *)&userptr[menu_index]);
                            menu_index++;
                            break;
                        default:
                            break;
                    }
                }
            }
            menu_item[menu_index] = (ITEM *)NULL;
            rc = display_menu(menu_item, start_row, menu_index, &retptr);
            if ((rc == RC_SUCCESS) &&
                (p_page_28_cur->attr[j].scsi_id != *retptr))
            {
                p_page_28_cur->attr[j].scsi_id = *retptr;
                p_page_28_ipr->attr[j].scsi_id =
                    p_page_28_chg->attr[j].scsi_id;
            }
            i=0;
            while (menu_item[i] != NULL)
                free_item(menu_item[i++]);
            free(menu_item);
            free(userptr);
            menu_item = NULL;
            break;
        }
        else if (p_page_28_chg->attr[j].scsi_id)
        {
            input_field_index += 2;
            start_row++;
        }

        if ((p_page_28_chg->attr[j].bus_width) &&
            (input_field_index == cur_field_index))
        {
            num_menu_items = 2;
            menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
            userptr = malloc(sizeof(int) * num_menu_items);

            menu_index = 0;
            menu_item[menu_index] = new_item("No","");
            userptr[menu_index] = 8;
            set_item_userptr(menu_item[menu_index],
                             (char *)&userptr[menu_index]);

            menu_index++;
            if (p_page_28_dflt->attr[j].bus_width == 16)
            {
                menu_item[menu_index] = new_item("Yes","");
                userptr[menu_index] = 16;
                set_item_userptr(menu_item[menu_index],
                                 (char *)&userptr[menu_index]);
                menu_index++;
            }
            menu_item[menu_index] = (ITEM *)NULL;
            rc = display_menu(menu_item, start_row, menu_index, &retptr);
            if ((rc == RC_SUCCESS) &&
                (p_page_28_cur->attr[j].bus_width != *retptr))
            {
                p_page_28_cur->attr[j].bus_width = *retptr;
                p_page_28_ipr->attr[j].bus_width =
                    p_page_28_chg->attr[j].bus_width;
            }
            i=0;
            while (menu_item[i] != NULL)
                free_item(menu_item[i++]);
            free(menu_item);
            free(userptr);
            menu_item = NULL;
            break;
        }
        else if (p_page_28_chg->attr[j].bus_width)
        {
            input_field_index += 2;
            start_row++;
        }

        if ((p_page_28_chg->attr[j].max_xfer_rate) &&
            (input_field_index == cur_field_index))
        {
            num_menu_items = 7;
            menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
            userptr = malloc(sizeof(int) * num_menu_items);

            menu_index = 0;
            switch (((iprtoh32(p_page_28_dflt->attr[j].max_xfer_rate)) *
                     p_page_28_cur->attr[j].bus_width)/(10 * 8))
            {
                case 320:
                    menu_item[menu_index] = new_item("320 MB/s","");
                    userptr[menu_index] = 3200;
                    set_item_userptr(menu_item[menu_index],
                                     (char *)&userptr[menu_index]);
                    menu_index++;
                case 160:
                    menu_item[menu_index] = new_item("160 MB/s","");
                    userptr[menu_index] = 1600;
                    set_item_userptr(menu_item[menu_index],
                                     (char *)&userptr[menu_index]);
                    menu_index++;
                case 80:
                    menu_item[menu_index] = new_item(" 80 MB/s","");
                    userptr[menu_index] = 800;
                    set_item_userptr(menu_item[menu_index],
                                     (char *)&userptr[menu_index]);
                    menu_index++;
                case 40:
                    menu_item[menu_index] = new_item(" 40 MB/s","");
                    userptr[menu_index] = 400;
                    set_item_userptr(menu_item[menu_index],
                                     (char *)&userptr[menu_index]);
                    menu_index++;
                case 20:
                    menu_item[menu_index] = new_item(" 20 MB/s","");
                    userptr[menu_index] = 200;
                    set_item_userptr(menu_item[menu_index],
                                     (char *)&userptr[menu_index]);
                    menu_index++;
                case 10:
                    menu_item[menu_index] = new_item(" 10 MB/s","");
                    userptr[menu_index] = 100;
                    set_item_userptr(menu_item[menu_index],
                                     (char *)&userptr[menu_index]);
                    menu_index++;
                case 5:
                    menu_item[menu_index] = new_item("  5 MB/s","");
                    userptr[menu_index] = 50;
                    set_item_userptr(menu_item[menu_index],
                                     (char *)&userptr[menu_index]);
                    menu_index++;
                    break;
                default:
                    menu_item[menu_index] = new_item("        ","");
                    userptr[menu_index] = 0;
                    set_item_userptr(menu_item[menu_index],
                                     (char *)&userptr[menu_index]);
                    menu_index++;
                    break;
            }
            menu_item[menu_index] = (ITEM *)NULL;
            rc = display_menu(menu_item, start_row, menu_index, &retptr);
            if ((rc == RC_SUCCESS) &&
                (iprtoh32(p_page_28_cur->attr[j].max_xfer_rate) !=
                 ((*retptr) * 8)/p_page_28_cur->attr[j].bus_width))
            {
                p_page_28_cur->attr[j].max_xfer_rate =
                    htoipr32(((*retptr) * 8)/p_page_28_cur->attr[j].bus_width);
                p_page_28_ipr->attr[j].max_xfer_rate =
                    p_page_28_chg->attr[j].max_xfer_rate;
            }
            i=0;
            while (menu_item[i] != NULL)
                free_item(menu_item[i++]);
            free(menu_item);
            free(userptr);
            menu_item = NULL;
            break;
        }
        else if (p_page_28_chg->attr[j].max_xfer_rate)
        {
            input_field_index += 2;
            start_row++;
        }
    }
    return rc;
}

int display_menu(ITEM **menu_item,
                 int menu_start_row,
                 int menu_index_max,
                 int **userptrptr)
{
    ITEM *p_cur_item;
    MENU *p_menu;
    WINDOW *p_box1_win, *p_box2_win, *p_field_win;
    int menu_rows, menu_cols, menu_rows_disp;
    int menu_index;
    int use_box2 = 0;
    int cur_item_index;
    int ch, i;
    int rc = RC_SUCCESS;

    p_menu = new_menu(menu_item);
    scale_menu(p_menu, &menu_rows, &menu_cols);
    menu_rows_disp = menu_rows;

    /* check if fits in current screen, + 1 is added for
     bottom border, +1 is added for buffer */
    if ((menu_start_row + menu_rows + 1 + 1) > curses_info.max_y)
    {
        menu_start_row = curses_info.max_y - (menu_rows + 1 + 1);
        if (menu_start_row < (8 + 1))
        {
            /* size is adjusted by 2 to allow for top
             and bottom border, add +1 for buffer */
            menu_rows_disp = curses_info.max_y - (8 + 2 + 1);
            menu_start_row = (8 + 1);
        }
    }

    if (menu_rows > menu_rows_disp)
    {
        set_menu_format(p_menu, menu_rows_disp, 1);
        use_box2 = 1;
    }

    set_menu_mark(p_menu, "*");
    scale_menu(p_menu, &menu_rows, &menu_cols);
    p_box1_win = newwin(menu_rows + 2, menu_cols + 2, menu_start_row - 1, 54);
    p_field_win = newwin(menu_rows, menu_cols, menu_start_row, 55);
    set_menu_win(p_menu, p_field_win);
    post_menu(p_menu);
    box(p_box1_win,ACS_VLINE,ACS_HLINE);
    if (use_box2)
    {
        p_box2_win = newwin(menu_rows + 2, 3, menu_start_row - 1, 54 + menu_cols + 1);
        box(p_box2_win,ACS_VLINE,ACS_HLINE);

        /* initialize scroll bar */
        for (i = 0;
             i < menu_rows_disp/2;
             i++)
        {
            mvwaddstr(p_box2_win, i + 1, 1, "#");
        }

        /* put down arrows in */
        for (i = menu_rows_disp;
             i > (menu_rows_disp - menu_rows_disp/2);
             i--)
        {
            mvwaddstr(p_box2_win, i, 1, "v");
        }
    }
    wnoutrefresh(p_box1_win);

    menu_index = 0;
    while (1)
    {
        if (use_box2)
            wnoutrefresh(p_box2_win);
        wnoutrefresh(p_field_win);
        doupdate();
        ch = getch();
        if (IS_ENTER_KEY(ch))
        {
            p_cur_item = current_item(p_menu);
            cur_item_index = item_index(p_cur_item);
            *userptrptr = item_userptr(menu_item[cur_item_index]);
            break;
        }
        else if ((ch == KEY_DOWN) || (ch == '\t'))
        {
            if (use_box2 &&
                (menu_index < (menu_index_max - 1)))
            {
                menu_index++;
                if (menu_index == menu_rows_disp)
                {
                    /* put up arrows in */
                    for (i = 0;
                         i < menu_rows_disp/2;
                         i++)
                    {
                        mvwaddstr(p_box2_win, i + 1, 1, "^");
                    }
                }
                if (menu_index == (menu_index_max - 1))
                {
                    /* remove down arrows */
                    for (i = menu_rows_disp;
                         i > (menu_rows_disp - menu_rows_disp/2);
                         i--)
                    {
                        mvwaddstr(p_box2_win, i, 1, "#");
                    }
                }
            }

            menu_driver(p_menu,REQ_DOWN_ITEM);
        }
        else if (ch == KEY_UP)
        {
            if (use_box2 &&
                (menu_index != 0))
            {
                menu_index--;
                if (menu_index == (menu_index_max - menu_rows_disp - 1))
                {
                    /* put down arrows in */
                    for (i = menu_rows_disp;
                         i > (menu_rows_disp - menu_rows_disp/2);
                         i--)
                    {
                        mvwaddstr(p_box2_win, i, 1, "v");
                    }
                }
                if (menu_index == 0)
                {
                    /* remove up arrows */
                    for (i = 0;
                         i < menu_rows_disp/2;
                         i++)
                    {
                        mvwaddstr(p_box2_win, i + 1, 1, "#");
                    }
                }
            }
            menu_driver(p_menu,REQ_UP_ITEM);
        }
        else if (IS_EXIT_KEY(ch))
        {
            rc = RC_EXIT;
            break;
        }
        else if (IS_CANCEL_KEY(ch))
        {
            rc = RC_CANCEL;
            break;
        }
    }

    unpost_menu(p_menu);
    free_menu(p_menu);
    wclear(p_field_win);
    wrefresh(p_field_win);
    delwin(p_field_win);
    wclear(p_box1_win);
    wrefresh(p_box1_win);
    delwin(p_box1_win);
    if (use_box2)
    {
        wclear(p_box2_win);
        wrefresh(p_box2_win);
        delwin(p_box2_win);
    }

    return rc;
}

int driver_config()
{
    /*
                                  Change Driver Configuration

         Current Driver configurations are shown.  To change
         setting hit "c" for options menu.  Highlight desired
         option then hit Enter

         c=Change Setting


            Verbosity  . . . . . . . . . . . . . :  1
            Trace Level  . . . . . . . . . . . . :  1

         Press Enter to Enable Changes

         e=Exit   q=Cancel
     */
#define DEBUG_INDEX 2
#define TRACE_INDEX 4
    int rc;
    FORM *p_old_form;
    FORM *p_fields_form;
    FIELD *p_input_fields[6], *p_cur_field;
    ITEM **menu_item;
    int cur_field_index;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    int fd;
    struct ipr_driver_cfg driver_cfg;
    int  ch;
    int  i;
    int form_adjust = 0;
    char debug_level_str[4];
    char trace_level_str[4];
    int *userptr;
    int *retptr;
    struct text_str
    {
        char line[4];
    } *menu_str;

    rc = RC_SUCCESS;

    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

    ioa_cmd.buffer = &driver_cfg;
    ioa_cmd.buffer_len = sizeof(struct ipr_driver_cfg);
    ioa_cmd.driver_cmd = 1;
    ioa_cmd.read_not_write = 1;

    fd = open(p_head_ipr_ioa->ioa.dev_name, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_head_ipr_ioa->ioa.dev_name);
        return RC_FAILED;
    }

    rc = ipr_ioctl(fd, IPR_READ_DRIVER_CFG, &ioa_cmd);
    close(fd);

    if (rc != 0)
    {
        syslog(LOG_ERR, "Read driver configuration failed. %m"IPR_EOL);
        return RC_FAILED;
    }

    /* Title */
    p_input_fields[0] =
        new_field(1, curses_info.max_x - curses_info.start_x, 
                  0, 0, 0, 0);
    set_field_just(p_input_fields[0], JUSTIFY_CENTER);
    set_field_buffer(p_input_fields[0], 0,
                     "Change Driver Configuration");
    field_opts_off(p_input_fields[0], O_ACTIVE);

    p_input_fields[1] = new_field(1, 38, DSP_FIELD_START_ROW, 4,
				  0, 0);
    field_opts_off(p_input_fields[1], O_ACTIVE | O_EDIT);
    set_field_buffer(p_input_fields[1], 0,
		     "Verbosity  . . . . . . . . . . . . . :");

    p_input_fields[DEBUG_INDEX] = new_field(1, 3, DSP_FIELD_START_ROW, 44,
				  0, 0);
    field_opts_off(p_input_fields[DEBUG_INDEX], O_EDIT);

    p_input_fields[3] = new_field(1, 38, DSP_FIELD_START_ROW + 1, 4,
				  0, 0);
    field_opts_off(p_input_fields[3], O_ACTIVE | O_EDIT);
    set_field_buffer(p_input_fields[3], 0,
		     "Trace Level  . . . . . . . . . . . . :");

    p_input_fields[TRACE_INDEX] = new_field(1, 3, DSP_FIELD_START_ROW + 1, 44,
				 0, 0);
    field_opts_off(p_input_fields[TRACE_INDEX], O_EDIT);

    p_input_fields[5] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_fields_form = new_form(p_input_fields);

    set_current_field(p_fields_form, p_input_fields[2]);

    form_opts_off(p_fields_form, O_BS_OVERLOAD);

    flush_stdscr();
    clear();
    post_form(p_fields_form);

    mvaddstr(2, 1, "Current Driver configurations are shown.  To change");
    mvaddstr(3, 1, "setting hit \"c\" for options menu.  Highlight desired");
    mvaddstr(4, 1, "option then hit Enter");
    mvaddstr(6, 1, "c=Change Setting");
    mvaddstr(curses_info.max_y - 3, 1, "Press Enter to Enable Changes");
    mvaddstr(curses_info.max_y - 1, 1, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

    refresh();
    doupdate();

    form_driver(p_fields_form,
                REQ_FIRST_FIELD);

    while(1)
    {
	if (driver_cfg.debug_level)
	    sprintf(debug_level_str,"%d",driver_cfg.debug_level);
	else
	    sprintf(debug_level_str,"OFF");

	set_field_buffer(p_input_fields[DEBUG_INDEX],
			 0,
			 debug_level_str);

	if (driver_cfg.trace_level)
	    sprintf(trace_level_str,"%d",driver_cfg.trace_level);
	else
	    sprintf(trace_level_str,"OFF");

	set_field_buffer(p_input_fields[TRACE_INDEX],
			 0,
			 trace_level_str);

        if (form_adjust)
        {
            form_driver(p_fields_form,
                        REQ_NEXT_FIELD);
            form_adjust = 0;
            refresh();
        }

        doupdate();
        ch = getch();

        if (IS_EXIT_KEY(ch))
        {
            rc = RC_EXIT;
            goto leave;
        }
        else if (IS_CANCEL_KEY(ch))
        {
            rc = RC_CANCEL;
            goto leave;
        }
        else if (ch == 'c')
        {
            p_cur_field = current_field(p_fields_form);
            cur_field_index = field_index(p_cur_field);
            if (cur_field_index == DEBUG_INDEX)
            {
                menu_item = malloc(sizeof(ITEM **) *
                                   (driver_cfg.debug_level_max + 2));
                menu_str = malloc(sizeof(struct text_str) *
                                  (driver_cfg.debug_level_max + 2));
                userptr = malloc(sizeof(int) *
                                 (driver_cfg.debug_level_max + 1));

                menu_item[0] = new_item("OFF","");
                userptr[0] = 0;
                set_item_userptr(menu_item[0],
                                 (char *)&userptr[0]);

                for (i = 1; i <= driver_cfg.debug_level_max; i++)
                {
                    sprintf(menu_str[i].line,"%d",i);
                    menu_item[i] = new_item(menu_str[i].line,"");
                    userptr[i] = i;
                    set_item_userptr(menu_item[i],
                                     (char *)&userptr[i]);
                }

                menu_item[driver_cfg.debug_level_max + 1] = (ITEM *)NULL;
                rc = display_menu(menu_item, DSP_FIELD_START_ROW,
                                  driver_cfg.debug_level_max + 1, &retptr);
                if (rc == RC_SUCCESS)
                    driver_cfg.debug_level = *retptr;

                i=0;
                while (menu_item[i] != NULL)
                    free_item(menu_item[i++]);
                free(menu_item);
                free(menu_str);
                free(userptr);
                menu_item = NULL;
            }
            else if (cur_field_index == TRACE_INDEX)
            {
                menu_item = malloc(sizeof(ITEM **) *
                                   (driver_cfg.trace_level_max + 2));
                menu_str = malloc(sizeof(struct text_str) *
                                  (driver_cfg.trace_level_max + 2));
                userptr = malloc(sizeof(int) *
                                 (driver_cfg.trace_level_max + 1));

                menu_item[0] = new_item("OFF","");
                userptr[0] = 0;
                set_item_userptr(menu_item[0],
                                 (char *)&userptr[0]);

                for (i = 1; i <= driver_cfg.trace_level_max; i++)
                {
                    sprintf(menu_str[i].line,"%d",i);
                    menu_item[i] = new_item(menu_str[i].line,"");
                    userptr[i] = i;
                    set_item_userptr(menu_item[i],
                                     (char *)&userptr[i]);
                }

                menu_item[driver_cfg.trace_level_max + 1] = (ITEM *)NULL;
                rc = display_menu(menu_item, DSP_FIELD_START_ROW + 1,
                                  driver_cfg.trace_level_max + 1, &retptr);
                if (rc == RC_SUCCESS)
                    driver_cfg.trace_level = *retptr;

                i=0;
                while (menu_item[i] != NULL)
                    free_item(menu_item[i++]);
                free(menu_item);
                free(menu_str);
                free(userptr);
                menu_item = NULL;
            }

            if (rc == RC_EXIT)
                goto leave;
        }
        else if (IS_ENTER_KEY(ch))
        {
	    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

	    ioa_cmd.buffer = &driver_cfg;
	    ioa_cmd.buffer_len = sizeof(struct ipr_driver_cfg);
	    ioa_cmd.driver_cmd = 1;

	    fd = open(p_head_ipr_ioa->ioa.dev_name, O_RDWR);
	    if (fd <= 1)
	    {
		syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_head_ipr_ioa->ioa.dev_name);
		return RC_FAILED;
	    }

	    rc = ipr_ioctl(fd, IPR_WRITE_DRIVER_CFG, &ioa_cmd);
	    close(fd);

	    if (rc != 0)
	    {
		syslog(LOG_ERR, "Write driver configuration failed. %m"IPR_EOL);
		return RC_FAILED;
	    }
	    goto leave;
        }
        else if (ch == '\t')
            form_driver(p_fields_form, REQ_NEXT_FIELD);
        else if (ch == KEY_UP)
            form_driver(p_fields_form, REQ_PREV_FIELD);
        else if (ch == KEY_DOWN)
            form_driver(p_fields_form, REQ_NEXT_FIELD);
    }

    leave:

        free_form(p_fields_form);
    free_screen(NULL, NULL, p_input_fields);
    curses_info.p_form = p_old_form;

    flush_stdscr();
    return rc;
}

char *strip_trailing_whitespace(char *p_str)
{
    int len;
    char *p_tmp;

    len = strlen(p_str);

    if (len == 0)
        return p_str;

    p_tmp = p_str + len - 1;
    while(*p_tmp == ' ')
        p_tmp--;
    p_tmp++;
    *p_tmp = '\0';
    return p_str;
}

/*
 Prints following to buffer:

 serial_num  ccin  model  /dev/scsi/...
 */

u16 print_dev(struct ipr_resource_entry *p_resource_entry,
              char *p_buf,
              char *dev_name)
{
    u16 len = 0;
    u8 product_id[IPR_PROD_ID_LEN+1];
    u8 vendor_id[IPR_VENDOR_ID_LEN+1];

    if (platform == IPR_ISERIES)
    {
        len += sprintf(p_buf, "%8s  %X   ", p_resource_entry->serial_num, p_resource_entry->type);
        len += sprintf(p_buf + len, "%03d   ", p_resource_entry->model);
        len += sprintf(p_buf + len, "%-38s  ", dev_name);
    }
    else
    {
        memcpy(vendor_id, p_resource_entry->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
        vendor_id[IPR_VENDOR_ID_LEN] = '\0';
        memcpy(product_id, p_resource_entry->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);
        product_id[IPR_PROD_ID_LEN] = '\0';

        len = sprintf(p_buf, "%8s  %-8s %-18s %03d   %-18s ",
                      p_resource_entry->serial_num,
                      vendor_id, product_id,
                      p_resource_entry->model, dev_name);
    }

    return len;
}

/*
 Prints following to buffer:
 serial_num  ccin  model  /dev/scsi/...
 */
u16 print_dev2(struct ipr_resource_entry *p_resource_entry,
               char *p_buf,
               struct ipr_resource_flags *p_resource_flags,
               char *dev_name)
{
    u16 len = 0;

    len += sprintf(p_buf, "%8s  %X   ", p_resource_entry->serial_num, p_resource_entry->type);

    len += sprintf(p_buf + len, "%03d   ", get_model(p_resource_flags));

    len += sprintf(p_buf + len, "%-38s", dev_name);

    return len;
}

/*
 Print device into buffer for a selection list 
 */
u16 print_dev_sel(struct ipr_resource_entry *p_resource_entry, char *p_buf)
{
    u16 len = 0;
    char buf[100];

    if (platform == IPR_ISERIES)
    {
        len += sprintf(p_buf + len, "%X  ", p_resource_entry->type);
    }
    else
    {
        memcpy(buf, p_resource_entry->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
        buf[IPR_VENDOR_ID_LEN] = '\0';

        len += sprintf(p_buf + len, "%-8s  ", buf);

        memcpy(buf, p_resource_entry->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);
        buf[IPR_PROD_ID_LEN] = '\0';

        len += sprintf(p_buf + len, "%-12s ", buf);
    }
    
    len += sprintf(p_buf + len, "%8s    ", p_resource_entry->serial_num);

    if (platform == IPR_ISERIES)
    {
        len += sprintf(p_buf + len, "%2s      %3s       ",
                       p_resource_entry->frame_id,
                       p_resource_entry->slot_label);
    }

    len += sprintf(p_buf + len, "%02d     %02d     %2d    %2d   %2d",
                   p_resource_entry->pci_bus_number,
                   p_resource_entry->pci_slot,
                   p_resource_entry->resource_address.bus,
                   p_resource_entry->resource_address.target,
                   p_resource_entry->resource_address.lun);

    return len;
}

/*
 Print device into buffer for a selection list 
 */
u16 print_dev_pty(struct ipr_resource_entry *p_resource_entry, char *p_buf, char *dev_name)
{
    u16 len = 0;

    len += sprintf(p_buf + len, "%8s  ", p_resource_entry->serial_num);

    if (platform == IPR_ISERIES)
        len += sprintf(p_buf + len, "%X   ", p_resource_entry->type);

    len += sprintf(p_buf + len, "%03d   ", p_resource_entry->model);

    if (platform == IPR_ISERIES)
    {
        len += sprintf(p_buf + len, "%-38s", dev_name);
    }
    else
    {
        len += sprintf(p_buf + len, "%-17s", dev_name);
        len += sprintf(p_buf + len, "%02d     %02d",
                       p_resource_entry->pci_bus_number,
                       p_resource_entry->pci_slot);

        if (p_resource_entry->is_af)
        {
            len += sprintf(p_buf + len, "    %3d    %2d    %2d  ",
                           p_resource_entry->resource_address.bus,
                           p_resource_entry->resource_address.target,
                           p_resource_entry->resource_address.lun);
        }
        else
        {
            len += sprintf(p_buf + len, "                     ");
        }
    }

    return len;
}

/*
 Print device into buffer for a selection list 
 */
u16 print_dev_pty2(struct ipr_resource_entry *p_resource_entry, char *p_buf, char *dev_name)
{
    u16 len = 0;

    len += sprintf(p_buf + len, "%8s  ", p_resource_entry->serial_num);

    if (platform == IPR_ISERIES)
        len += sprintf(p_buf + len, "%X   ", p_resource_entry->type);

    len += sprintf(p_buf + len, "%03d   ", p_resource_entry->model);

    if (platform == IPR_ISERIES)
    {
        len += sprintf(p_buf + len, "%-38s", dev_name);
    }
    else
    {
        len += sprintf(p_buf + len, "%-16s", dev_name);
        len += sprintf(p_buf + len, "%02d     %02d",
                       p_resource_entry->pci_bus_number,
                       p_resource_entry->pci_slot);

        if (p_resource_entry->is_af)
        {
            len += sprintf(p_buf + len, "    %3d    %2d    %2d  ",
                           p_resource_entry->resource_address.bus,
                           p_resource_entry->resource_address.target,
                           p_resource_entry->resource_address.lun);
        }
        else
        {
            len += sprintf(p_buf + len, "                     ");
        }
    }

    return len;
}

/* Can only be called for DASD devices */
u16 get_model(struct ipr_resource_flags *p_resource_flags)
{
    u16 model;

    if (p_resource_flags->is_array_member)
        model = 70;
    else
        model = 50;

    if (p_resource_flags->is_compressed)
        model += 10;

    if (model > 60)
    {
        switch (IPR_GET_CAP_REDUCTION(*p_resource_flags))
        {
            case IPR_HALF_REDUCTION:
                model += 8;
                break;
            case IPR_QUARTER_REDUCTION:
                model += 4;
                break;
            case IPR_EIGHTH_REDUCTION:
                model += 2;
                break;
            case IPR_SIXTEENTH_REDUCTION:
                model += 1;
                break;
            case IPR_UNKNOWN_REDUCTION:
                model += 9;
                break;
        }
    }
    return model;
}

int not_implemented()
{
    /*
     Not Implemented          











     This function is not yet implemented









     e=Exit   q=Cancel

     */
    FIELD *p_fields[3];
    FORM *p_form;
    FORM *p_old_form;
    int rc = RC_SUCCESS;
    int j;

    /* Title */
    p_fields[0] = new_field(1, curses_info.max_x - curses_info.start_x,   /* new field size */ 
                            0, 0,       /* upper left corner */
                            0,           /* number of offscreen rows */
                            0);               /* number of working buffers */

    /* User input field */
    p_fields[1] = new_field(1, 2,   /* new field size */ 
                            curses_info.max_y - 2, curses_info.max_x - 2,       /* lower right corner */
                            0,           /* number of offscreen rows */
                            0);          /* number of working buffers */

    p_fields[2] = NULL;

    p_old_form = curses_info.p_form;
    curses_info.p_form = p_form = new_form(p_fields);

    set_field_just(p_fields[0], JUSTIFY_CENTER);

    field_opts_off(p_fields[0],          /* field to alter */
                   O_ACTIVE);             /* attributes to turn off */ 

    set_field_buffer(p_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Not Implemented");          /* string value to set */

    while(1)
    {
        post_form(p_form);

        mvaddstr(curses_info.max_y/2, (curses_info.max_x - 36)/2 , "This function is not yet implemented");
        mvaddstr(curses_info.max_y - 2, 0, EXIT_KEY_LABEL"=Exit   "CANCEL_KEY_LABEL"=Cancel");

        form_driver(p_form,               /* form to pass input to */
                    REQ_LAST_FIELD );     /* form request code */

        refresh();
        for (;;)
        {
            int ch = getch();

            if (IS_EXIT_KEY(ch))
            {
                rc = RC_EXIT;
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) || IS_ENTER_KEY(ch))
            {
                rc = RC_CANCEL;
                goto leave;
            }
            else
            {
                form_driver(p_form,               /* form to pass input to */
                            REQ_LAST_FIELD );     /* form request code */
                refresh();
            }
        }
    }
    leave:

        unpost_form(p_form);
    free_form(p_form);
    curses_info.p_form = p_old_form;
    for (j=0; j < 2; j++)
        free_field(p_fields[j]);
    flush_stdscr();
    return rc;
}

void handle_sig_int(int dummy)
{
    clear();
    refresh();
    endwin();
    printf(IPR_EOL"Exit on signal...%d"IPR_EOL, dummy);
    exit(0);
}

void handle_sig_term(int signal)
{
    clear();
    refresh();
    endwin();
    printf(IPR_EOL"Exit on signal...%d"IPR_EOL, signal);
    exit(0);
}

void handle_sig_winch(int dummy)
{
    /* xx Can I do a better job here? */
    signal(SIGWINCH, handle_sig_winch);

    endwin();
    refresh();

    getmaxyx(curses_info.init_win, max_y, max_x);
    getbegyx(curses_info.init_win, start_y, start_x);
    curses_info.max_y = max_y;
    curses_info.max_x = max_x;
    curses_info.start_y = start_y;
    curses_info.start_x = start_x;
}

int open_dev(struct ipr_ioa *p_ioa, struct ipr_device *p_device)
{
    int fd;

    fd = open(p_device->dev_name, O_RDWR);

    if ((fd <= 1) && ((errno == ENOENT) || (errno == ENXIO)))
    {
        syslog(LOG_ERR, "Cannot open device %s. %m"IPR_EOL, p_device->dev_name);
        syslog(LOG_ERR, "Rescanning SCSI bus for device"IPR_EOL);
        scan_device(p_device->p_resource_entry->resource_address, p_ioa->host_num);

        fd = open(p_device->dev_name, O_RDWR);
    }

    return fd;
}

void free_screen(struct panel_l *p_panel, struct window_l *p_win, FIELD **p_fields)
{
    struct panel_l *p_cur_panel;
    struct window_l *p_cur_win;
    int i;

    p_cur_panel = p_panel;
    while(p_cur_panel)
    {
        p_panel = p_panel->p_next;
        del_panel(p_cur_panel->p_panel);
        free(p_cur_panel);
        p_cur_panel = p_panel;
    }
    p_cur_win = p_win;
    while(p_cur_win)
    {
        p_win = p_win->p_next;
        delwin(p_cur_win->p_win);
        free(p_cur_win);
        p_cur_win = p_win;
    }

    if (p_fields)
    {
        for (i = 0; p_fields[i] != NULL; i++)
        {
            free_field(p_fields[i]);
        }
    }
}

void revalidate_devs(struct ipr_ioa *p_ioa)
{
    int rc, j;
    struct ipr_ioa *p_cur_ioa;

    rc = RC_SUCCESS;

    for (p_cur_ioa = p_head_ipr_ioa;
         p_cur_ioa != NULL;
         p_cur_ioa = p_cur_ioa->p_next)
    {
        if (p_ioa && (p_ioa != p_cur_ioa))
            continue;

        for (j = 0; j < p_cur_ioa->num_devices; j++)
        {
            /* If not a DASD, ignore entry */
            if (!IPR_IS_DASD_DEVICE(p_cur_ioa->dev[j].p_resource_entry->std_inq_data))
                continue;

            revalidate_dev(&p_cur_ioa->dev[j], p_cur_ioa,
                           ((p_ioa != NULL) && p_cur_ioa->dev[j].p_resource_entry->is_af));
        }
    }

    return;
}

void evaluate_device(struct ipr_device *p_ipr_device, struct ipr_ioa *p_ioa, int change_size)
{
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_res_addr resource_address;
    struct ipr_resource_table resource_table;
    u32 resource_handle;
    int j, rc, fd, device_converted;
    int timeout = 60;

    /* send evaluate device down */
    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));
    p_resource_entry = p_ipr_device->p_resource_entry;

    ioa_cmd.buffer = NULL;
    ioa_cmd.buffer_len = 0;
    resource_handle = iprtoh32(p_resource_entry->resource_handle);
    ioa_cmd.cdb[2] = (u8)(resource_handle >> 24);
    ioa_cmd.cdb[3] = (u8)(resource_handle >> 16);
    ioa_cmd.cdb[4] = (u8)(resource_handle >> 8);
    ioa_cmd.cdb[5] = (u8)(resource_handle);

    fd = open(p_ioa->ioa.dev_name, O_RDWR);
    if (fd < 0)
    {
        syslog(LOG_ERR, "Cannot open %s. %m\n", p_ioa->ioa.dev_name);
        return;
    }

    ipr_ioctl(fd, IPR_EVALUATE_DEVICE, &ioa_cmd);

    resource_address = p_resource_entry->resource_address;
    device_converted = 0;

    while (!device_converted && timeout)
    {
        /* issue query ioa config here */
        memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

        ioa_cmd.buffer = &resource_table;
        ioa_cmd.buffer_len = sizeof(struct ipr_resource_table);
        ioa_cmd.read_not_write = 1;

        rc = ipr_ioctl(fd, IPR_QUERY_IOA_CONFIG, &ioa_cmd);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Query IOA Config to %s failed. %m"IPR_EOL, p_ioa->ioa.dev_name);
            close(fd);
            return;
        }

        for (j = 0; j <resource_table.hdr.num_entries; j++)
        {
            p_resource_entry = &resource_table.dev[j];
            if ((memcmp(&resource_address,
                        &p_resource_entry->resource_address,
                        sizeof(resource_address)) == 0) &&
                (((p_resource_entry->is_hidden) &&
                  (change_size == IPR_522_BLOCK)) ||
                 ((!p_resource_entry->is_hidden) &&
                  (change_size == IPR_512_BLOCK))))
            {
                device_converted = 1;
                break;
            }
        }
        sleep(1);
        timeout--;
    }
    close(fd);

    revalidate_dev(p_ipr_device, p_ioa, 1);
}

void revalidate_dev(struct ipr_device *p_ipr_device, struct ipr_ioa *p_ioa, int force)
{
    char dev_name[50];
    int fd, rc;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_resource_entry *p_resource_entry;

    p_resource_entry = p_ipr_device->p_resource_entry;

    if (!p_resource_entry->dev_changed && !force)
        return;

    if (p_ipr_device->opens != 0)
        return;

    remove_device(p_resource_entry->resource_address, p_ioa);

    if (!p_resource_entry->is_hidden)
    {
        sleep(1);
        scan_device(p_resource_entry->resource_address, p_ioa->host_num);
    }

    /* Reset device changed bit */
    fd = open(p_ioa->ioa.dev_name, O_RDWR);

    if (fd < 0)
    {
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_ioa->ioa.dev_name);
        return;
    }

    memset(&ioa_cmd, 0, sizeof(struct ipr_ioctl_cmd_internal));

    ioa_cmd.buffer = NULL;
    ioa_cmd.buffer_len = 0;
    ioa_cmd.device_cmd = 1;
    ioa_cmd.driver_cmd = 1;
    ioa_cmd.resource_address = p_resource_entry->resource_address;

    rc = ipr_ioctl(fd, IPR_RESET_DEV_CHANGED, &ioa_cmd);

    if ((rc != 0) && !p_ioa)
        syslog(LOG_ERR, "Reset dev changed bit to %s failed. %m"IPR_EOL, dev_name);
    close (fd);
}

int is_array_in_use(struct ipr_ioa *p_cur_ioa,
                    u8 array_id)
{
    int j, opens;
    struct ipr_device_record *p_device_record;
    struct ipr_array2_record *p_array2_record;
    struct ipr_resource_entry *p_resource_entry;

    for (j = 0; j < p_cur_ioa->num_devices; j++)
    {
        p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;
        p_array2_record = (struct ipr_array2_record *)p_cur_ioa->dev[j].p_qac_entry;

        if (p_device_record == NULL)
            continue;

        if (p_device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD)
        {
            if ((array_id == p_device_record->array_id) &&
                ((p_device_record->start_cand) ||
                 (p_device_record->stop_cand)))
            {
                p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
                opens = num_device_opens(p_cur_ioa->host_num,
                                         p_resource_entry->resource_address.bus,
                                         p_resource_entry->resource_address.target,
                                         p_resource_entry->resource_address.lun);

                if (opens != 0)
                    return 1;
            }
        }
        /* there is no need to check the devices for an array2 start candidate
         as the devices are not accessible to linux.  for stop candidate the
         status of the volume set must be checked as this is the linux
         accessible device which may be in use */
        else if (p_array2_record->common.record_id == IPR_RECORD_ID_ARRAY2_RECORD)
        {
            if (array_id == p_array2_record->array_id)
            {
                p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
                opens = num_device_opens(p_cur_ioa->host_num,
                                         p_resource_entry->resource_address.bus,
                                         p_resource_entry->resource_address.target,
                                         p_resource_entry->resource_address.lun);

                if (opens != 0)
                    return 1;
            }
        }
    }
    return 0;
}

