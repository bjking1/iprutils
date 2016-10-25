/**
 * IBM IPR adapter configuration utility
 *
 * (C) Copyright 2000, 2004
 * International Business Machines Corporation and others.
 * All Rights Reserved. This program and the accompanying
 * materials are made available under the terms of the
 * Common Public License v1.0 which accompanies this distribution.
 **/

#ifndef iprlib_h
#include "iprlib.h"
#endif
#include <nl_types.h>
#include <locale.h>
#include <term.h>
#include <ncurses.h>
#include <form.h>
#include <panel.h>
#include <menu.h>
#include <string.h>
#include "iprconfig.h"

#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libgen.h>

#include <math.h>

char *tool_name = "iprconfig";

#define MAX_CMD_LENGTH 1000

struct devs_to_init_t {
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;
	u64 device_id;
	int new_block_size;
	int cmplt;
	int do_init;
	int done;
	int dev_type;
#define IPR_AF_DASD_DEVICE      1
#define IPR_JBOD_DASD_DEVICE    2
	struct devs_to_init_t *next;
};

#define for_each_dev_to_init(dev) for (dev = dev_init_head; dev; dev = dev->next)

struct prot_level {
	struct ipr_array_cap_entry *array_cap_entry;
	u8 is_valid:1;
	u8 reserved:7;
};

struct array_cmd_data {
	u8 array_id;
	u8 prot_level;
	u16 stripe_size;
	int qdepth;
	u32 do_cmd;
	int vset_cache;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct ipr_array_query_data *qac;
	struct array_cmd_data *next;
};

/* not needed once menus incorporated into screen_driver */
struct window_l {
	WINDOW *win;
	struct window_l *next;
};

struct panel_l {
	PANEL *panel;
	struct panel_l *next;
};

static struct devs_to_init_t *dev_init_head = NULL;
static struct devs_to_init_t *dev_init_tail = NULL;
static struct array_cmd_data *raid_cmd_head = NULL;
static struct array_cmd_data *raid_cmd_tail = NULL;
static i_container *i_con_head = NULL;  /* FIXME requires multiple heads */
static char log_root_dir[200];
static char editor[200];
FILE *errpath;
static int toggle_field;
nl_catd catd;
static char **add_args;
static int num_add_args;
static int use_curses;

#define for_each_raid_cmd(cmd) for (cmd = raid_cmd_head; cmd; cmd = cmd->next)

#define DEFAULT_LOG_DIR "/var/log"
#define DEFAULT_EDITOR "less"

#define IS_CANCEL_KEY(c) ((c == KEY_F(12)) || (c == 'q') || (c == 'Q'))
#define CANCEL_KEY_LABEL "q=Cancel   "
#define IS_EXIT_KEY(c) ((c == KEY_F(3)) || (c == 'e') || (c == 'E'))
#define EXIT_KEY_LABEL "e=Exit   "
#define CONFIRM_KEY_LABEL "c=Confirm   "
#define CONFIRM_REC_KEY_LABEL "s=Confirm Reclaim   "
#define ENTER_KEY '\n'
#define IS_ENTER_KEY(c) ((c == KEY_ENTER) || (c == ENTER_KEY))
#define IS_REFRESH_KEY(c) ((c == 'r') || (c == 'R'))
#define REFRESH_KEY_LABEL "r=Refresh   "
#define IS_TOGGLE_KEY(c) ((c == 't') || (c == 'T'))
#define TOGGLE_KEY_LABEL "t=Toggle   "

#define IS_PGDN_KEY(c) ((c == KEY_NPAGE) || (c == 'f') || (c == 'F'))
#define PAGE_DOWN_KEY_LABEL "f=PageDn   "
#define IS_PGUP_KEY(c) ((c == KEY_PPAGE) || (c == 'b') || (c == 'B'))
#define PAGE_UP_KEY_LABEL "b=PageUp   "

#define IS_5250_CHAR(c) (c == 0xa)
#define is_digit(ch) (((ch)>=(unsigned)'0'&&(ch)<=(unsigned)'9')?1:0)

struct special_status {
	int   index;
	int   num;
	char *str;
} s_status;

char *print_device(struct ipr_dev *, char *, char *, int);
int display_menu(ITEM **, int, int, int **);
char *__print_device(struct ipr_dev *, char *, char *, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
static char *print_path_details(struct ipr_dev *, char *);
static int get_drive_phy_loc(struct ipr_ioa *ioa);
static char *print_ssd_report(struct ipr_dev *dev, char *body);
static char *af_dasd_perf (char *body, struct ipr_dev *dev);

#define print_dev(i, dev, buf, fmt, type) \
        for (i = 0; i < 2; i++) \
            (buf)[i] = print_device(dev, (buf)[i], fmt, type);

#define print_dev_conc(i, dev, buf, fmt, type) \
        for (i = 0; i < 3; i++) \
            (buf)[i] = print_device(dev, (buf)[i], fmt, (type + 5));

#define print_dev_enclosure(i, dev, buf, fmt, type) \
        for (i = 0; i < 2; i++) { \
		if ( i == 0 ) \
		    (buf)[i] = print_device(dev, (buf)[i], fmt, (type + 8)); \
		if (i == 1) \
		    (buf)[i] = print_device(dev, (buf)[i], fmt, (type + 8)); \
	}

static struct sysfs_dev *head_sdev;
static struct sysfs_dev *tail_sdev;

/**
 * is_format_allowed - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   1 if format is allowed / 0 otherwise
 **/
static int is_format_allowed(struct ipr_dev *dev)
{
	int rc;
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	struct sense_data_t sense_data;

	if (ipr_is_af_dasd_device(dev)) {
		rc = ipr_query_command_status(dev, &cmd_status);
		if (rc == 0 && cmd_status.num_records != 0) {
			status_record = cmd_status.record;
			if ((status_record->status != IPR_CMD_STATUS_SUCCESSFUL)
			    && (status_record->status != IPR_CMD_STATUS_FAILED))
				return 0;
		}
	} else {
		rc = ipr_test_unit_ready(dev, &sense_data);

		if (rc == CHECK_CONDITION && (sense_data.error_code & 0x7F) == 0x70) {
			if (sense_data.add_sense_code == 0x31 &&
			    sense_data.add_sense_code_qual == 0x00)
				return 1;
			else if ((sense_data.sense_key & 0x0F) == 0x02)
				return 0;

		}
	}

	return 1;
}

/**
 * can_format_for_raid - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   1 if format for raid is allowed / 0 otherwise
 **/
static int can_format_for_raid(struct ipr_dev *dev)
{
	if (dev->ioa->is_secondary)
		return 0;
	if (!ipr_is_gscsi(dev) && !ipr_is_af_dasd_device(dev))
		return 0;
	if (ipr_is_hot_spare(dev) || !device_supported(dev))
		return 0;
	if (ipr_is_array_member(dev) && !dev->dev_rcd->no_cfgte_vol)
		return 0;
	if (ipr_is_af_dasd_device(dev) && ipr_device_is_zeroed(dev))
		return 0;
	if (!ipr_is_af_dasd_device(dev)) {
		/* If on a JBOD adapter */
		if (!dev->ioa->qac_data->num_records)
			return 0;
		if (is_af_blocked(dev, 0))
			return 0;
	}

	if (!is_format_allowed(dev))
		return 0;
	return 1;
}

static int wait_for_formatted_af_dasd(int timeout_in_secs)
{
	struct devs_to_init_t *dev = dev_init_head;
	struct scsi_dev_data *scsi_devs;
	struct scsi_dev_data *scsi_dev_data;
	int num_devs, j, af_found, jbod2af_formats, num_secs;
	u64 device_id;

	for (num_secs = 0; num_secs < timeout_in_secs; timeout_in_secs++) {
		af_found = 0;
		jbod2af_formats = 0;
		scsi_devs = NULL;

		num_devs = get_scsi_dev_data(&scsi_devs);

		for_each_dev_to_init(dev) {
			if (!dev->dev || !dev->ioa)
				continue;
			if (!dev->dev->scsi_dev_data)
				continue;
			if (dev->dev_type != IPR_JBOD_DASD_DEVICE)
				continue;
			if (!ipr_is_af_blk_size(dev->ioa, dev->new_block_size))
				continue;
			if (!dev->do_init)
				continue;

			jbod2af_formats++;
			device_id = dev->dev->scsi_dev_data->device_id;

			for (j = 0, scsi_dev_data = scsi_devs;
			     j < num_devs; j++, scsi_dev_data++) {
				if (scsi_dev_data->host != dev->ioa->host_num)
					continue;
				if (get_sg_name(scsi_dev_data))
					continue;
				if (scsi_dev_data->type != IPR_TYPE_AF_DISK)
					continue;
				if (dev->device_id != scsi_dev_data->device_id)
					continue;

				scsi_dbg(dev->dev, "Format complete. AF DASD found. New Device ID=%lx, Old Device ID=%lx\n",
					  scsi_dev_data->device_id, dev->device_id);
				af_found++;
				break;
			}
		}

		free(scsi_devs);
		if (af_found == jbod2af_formats)
			break;
		sleep(1);
	}

	return ((af_found == jbod2af_formats) ? 0 : -ETIMEDOUT);
}

/**
 * flush_stdscr - 
 *
 * Returns:
 *   nothing
 **/
/* not needed after screen_driver can do menus */
static void flush_stdscr()
{
	if (!use_curses) {
		fprintf(stdout, "\r");
		fflush(stdout);
		return;
	}

	nodelay(stdscr, TRUE);

	while(getch() != ERR) {}
	nodelay(stdscr, FALSE);
}

/**
 * free_i_con - free memory from an i_container list
 * @i_con:		i_container struct
 *
 * Returns:
 *   NULL
 **/
static i_container *free_i_con(i_container *i_con)
{
	i_container *temp_i_con;

	i_con = i_con_head;
	if (i_con == NULL)
		return NULL;

	do {
		temp_i_con = i_con->next_item;
		free(i_con);
		i_con = temp_i_con;
	} while (i_con);

	i_con_head = NULL;
	return NULL;
}

/**
 * add_i_con - create a new i_con list or add an i_container to a list
 * @i_con:		i_container struct
 * @f:			field data
 * @d:			data buffer
 *
 * Returns:
 *   i_container pointen
 **/
static i_container *add_i_con(i_container *i_con, char *f, void *d)
{  
	i_container *new_i_con;
	new_i_con = malloc(sizeof(i_container));

	new_i_con->next_item = NULL;

	/* used to hold data entered into user-entry fields */
	strncpy(new_i_con->field_data, f, MAX_FIELD_SIZE+1); 
	new_i_con->field_data[strlen(f)+1] = '\0';

	/* a pointen to the device information represented by the i_con */
	new_i_con->data = d;

	if (i_con)
		i_con->next_item = new_i_con;
	else
		i_con_head = new_i_con;

	return new_i_con;
}

/**
 * exit_confirmed - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0
 **/
int exit_confirmed(i_container *i_con)
{
	exit_func();
	exit(0);
	return 0;
}

/**
 * free_screen - 
 * @panel:		
 * @win:		
 * @fields:		
 *
 * Returns:
 *   nothing
 **/
/*not needed after screen_driver can do menus */
static void free_screen(struct panel_l *panel, struct window_l *win,
			FIELD **fields)
{
	struct panel_l *cur_panel;
	struct window_l *cur_win;
	int i;

	cur_panel = panel;

	while(cur_panel) {
		panel = panel->next;
		del_panel(cur_panel->panel);
		free(cur_panel);
		cur_panel = panel;
	}

	cur_win = win;
	while(cur_win) {
		win = win->next;
		delwin(cur_win->win);
		free(cur_win);
		cur_win = win;
	}

	if (fields) {
		for (i = 0; fields[i] != NULL; i++)
			free_field(fields[i]);
	}
}

/**
 * add_raid_cmd_tail - 
 * @ioa:		ipr ioa struct
 * @dev:		ipr dev struct
 * @array_id:		array ID
 *
 * Returns:
 *   nothing
 **/
static void add_raid_cmd_tail(struct ipr_ioa *ioa, struct ipr_dev *dev, u8 array_id)
{
	if (raid_cmd_head) {
		raid_cmd_tail->next = malloc(sizeof(struct array_cmd_data));
		raid_cmd_tail = raid_cmd_tail->next;
	} else {
		raid_cmd_head = raid_cmd_tail = malloc(sizeof(struct array_cmd_data));
	}

	memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

	raid_cmd_tail->array_id = array_id;
	raid_cmd_tail->ioa = ioa;
	raid_cmd_tail->dev = dev;
}

/**
 * free_raid_cmds - free the raid_cmd_head list
 *
 * Returns:
 *   nothing
 **/
static void free_raid_cmds()
{
	struct array_cmd_data *cur = raid_cmd_head;

	while(cur) {
		free(cur->qac);
		cur = cur->next;
		free(raid_cmd_head);
		raid_cmd_head = cur;
	}
}

/**
 * free_devs_to_init - free the dev_init_head list
 *
 * Returns:
 *   nothing
 **/
static void free_devs_to_init()
{
	struct devs_to_init_t *dev = dev_init_head;

	while (dev) {
		dev = dev->next;
		free(dev_init_head);
		dev_init_head = dev;
	}
}

/**
 * strip_trailing_whitespace - remove trailing white space from a string
 * @p_str:		string
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static char *strip_trailing_whitespace(char *p_str)
{
	int len;
	char *p_tmp;

	len = strlen(p_str);

	if (len == 0)
		return p_str;

	p_tmp = p_str + len - 1;
	while(*p_tmp == ' ' || *p_tmp == '\n')
		p_tmp--;
	p_tmp++;
	*p_tmp = '\0';
	return p_str;
}

/**
 * tool_exit_func - clean up curses stuff on exit if needed
 *
 * Returns:
 *   nothing
 **/
static void tool_exit_func()
{
	if (use_curses) {
		clearenv();
		clear();
		refresh();
		endwin();
	}
}

/**
 * cmdline_exit_func - 
 *
 * Returns:
 *   nothing
 **/
static void cmdline_exit_func()
{
}

/**
 * ipr_list_opts - 
 * @body:		
 * @key:		
 * @list_str:		
 *
 * Returns:
 *   pointen to body string
 **/
static char *ipr_list_opts(char *body, char *key, char *list_str)
{
	int   start_len = 0;
	char *string_buf = _("Select one of the following");

	if (!body) {
		body = realloc(body, strlen(string_buf) + 8);
		sprintf(body, "\n%s:\n\n", string_buf);
	}

	start_len = strlen(body);

	body = realloc(body, start_len + strlen(key) + strlen(_(list_str)) + 16);
	sprintf(body + start_len, "    %s. %s\n", key, _(list_str));
	return body;
}

/**
 * ipr_end_list -
 * @body:		
 *
 * Returns:
 *   pointen to body string
 **/
static char *ipr_end_list(char *body)
{
	int start_len = 0;
	char *string_buf = _("Selection");

	start_len = strlen(body);

	body = realloc(body, start_len + strlen(string_buf) + 16);
	sprintf(body + start_len, "\n\n%s: %%1", string_buf);
	return body;
}

/**
 * screen_driver - Main driver for curses based display
 * @screen:		s_node
 * @header_lines:	number of header lines
 * @i_con:		i_container struct
 *
 * Returns:
 *   screen_output struct
 **/
static struct screen_output *screen_driver(s_node *screen, int header_lines, i_container *i_con)
{
	WINDOW *w_pad = NULL,*w_page_header = NULL; /* windows to hold text */
	FIELD **fields = NULL;                 /* field list for forms */
	FORM *form = NULL;                     /* form for input fields */
	int rc = 0;                            /* the status of the screen */
	char *status;                          /* displays screen operations */
	char buffer[100];                      /* needed for special status strings */
	bool invalid = false;                  /* invalid status display */

	bool ground_cursor=false;

	int stdscr_max_y,stdscr_max_x;
	int w_pad_max_y=0,w_pad_max_x=0;       /* limits of the windows */
	int pad_l=0,pad_scr_r,pad_t=0,viewable_body_lines;
	int title_lines=2,footer_lines=2;      /* position of the pad */
	int center,len=0,ch;
	int i=0,j,k,row=0,col=0,bt_len=0;      /* text positioning */
	int field_width,num_fields=0;          /* form positioning */
	int h,w,t,l,o,b;                       /* form querying */

	int active_field = 0;                  /* return to same active field after a quit */
	bool pages = false,is_bottom = false;  /* control page up/down in multi-page screens */
	bool form_adjust = false;              /* correct cursor position in multi-page screens */
	bool refresh_stdscr = true;
	bool x_offscr,y_offscr;                /* scrolling windows */
	bool pagedn, fakepagedn;
	char *input = NULL;
	struct screen_output *s_out;
	struct screen_opts   *temp;            /* reference to another screen */

	char *title,*body,*body_text;          /* screen text */
	i_container *temp_i_con;
	int num_i_cons = 0;
	int x, y;
	int f_flags;

	s_out = malloc(sizeof(struct screen_output)); /* passes an i_con and an rc value back to the function */

	/* create text strings */
	title = _(screen->title);
	body = screen->body;
	f_flags = screen->f_flags;

	if (f_flags & ENTER_FLAG)
		footer_lines += 3;
	else if (f_flags & FWD_FLAG)
		footer_lines++;

	if (f_flags & TOGGLE_FLAG)
		active_field = toggle_field;

	bt_len = strlen(body);
	body_text = calloc(bt_len + 1, sizeof(char));

	/* determine max size needed and find where to put form fields and device
	 * input in the text and add them
	 * '%' marks a field.  The number after '%' represents the field's width
	 */
	i=0;
	temp_i_con = i_con_head;
	while(body[i] != '\0') {
		if (body[i] == '\n') {
			row++;
			col = -1;

			body_text[len] = body[i];
			len++;
		} else if ((body[i] == '%') && is_digit(body[i+1])) {
			fields = realloc(fields,sizeof(FIELD **) * (num_fields + 1));
			i++;

			field_width = 0;
			sscanf(&(body[i]),"%d",&field_width);

			fields[num_fields] = new_field(1,field_width,row,col,0,0);

			if (field_width >= 30)
				field_opts_off(fields[num_fields],O_AUTOSKIP);

			if (field_width > 9)
				/* skip second digit of field index */
				i++; 

			if ((temp_i_con) && (temp_i_con->field_data[0]) && (f_flags & MENU_FLAG)) {
				set_field_buffer(fields[num_fields], 0, temp_i_con->field_data);
				input = field_buffer(fields[num_fields],0);
				temp_i_con = temp_i_con->next_item;
				num_i_cons++;
			}

			set_field_userptr(fields[num_fields],NULL);
			num_fields++;
			col += field_width-1;

			bt_len += field_width;

			body_text = realloc(body_text,bt_len);
			for(k = 0; k < field_width; k++)
				body_text[len+k] = ' ';
			len += field_width;
		} else {
			body_text[len] = body[i];
			len++;
		}

		col++;
		i++;

		if (col > w_pad_max_x)
			w_pad_max_x = col;
	}

	body_text[len] = '\0';

	w_pad_max_y = row;

	w_pad_max_y++; /* some forms may not display correctly if this is not included */
	w_pad_max_x++; /* adds a column for return char \n : else, a blank line will appear in the pad */

	w_pad = newpad(w_pad_max_y,w_pad_max_x);
	keypad(w_pad,TRUE);

	if (num_fields) {
		fields = realloc(fields,sizeof(FIELD **) * (num_fields + 1));
		fields[num_fields] = NULL;
		form = new_form(fields);
	}

	/* make the form appear in the pad; not stdscr */
	set_form_win(form,w_pad);
	set_form_sub(form,w_pad);

	while(1) {
		getmaxyx(stdscr,stdscr_max_y,stdscr_max_x);

		/* clear all windows before writing new text to them */
		clear();
		wclear(w_pad);
		if(pages)
			wclear(w_page_header);

		center = stdscr_max_x/2;
		len = strlen(title);

		/* add the title at the center of stdscr */
		mvaddstr(0,(center-len/2)>0?center-len/2:0,title);

		/* set the boundaries of the pad based on the size of the screen
		 and determine whether or not the user is able to scroll */
		if ((stdscr_max_y - title_lines - footer_lines + 1) >= w_pad_max_y) {
			viewable_body_lines = w_pad_max_y;
			pad_t = 0;
			y_offscr = false; /*not scrollable*/
			pages = false;
			for (i = 0; i < num_fields; i++)
				field_opts_on(fields[i],O_ACTIVE);
		} else {
			viewable_body_lines = stdscr_max_y - title_lines - footer_lines - 1;
			y_offscr = true; /*scrollable*/
			if (f_flags & FWD_FLAG) {
				pages = true;
				w_page_header = subpad(w_pad,header_lines,w_pad_max_x,0,0);
			}
		}

		if (stdscr_max_x > w_pad_max_x) {
			pad_scr_r = w_pad_max_x;
			pad_l = 0;
			x_offscr = false;
		} else {
			pad_scr_r = stdscr_max_x;
			x_offscr = true;
		}

		move(stdscr_max_y - footer_lines,0);

		if (f_flags & ENTER_FLAG) {
			if (num_fields == 0) {
				addstr(_("\nPress Enter to Continue\n\n"));
				ground_cursor = true;
			} else
				addstr(_("\nOr leave blank and press Enter to cancel\n\n"));
		}

		if ((f_flags & FWD_FLAG) && !(f_flags & ENTER_FLAG))
			addstr("\n");
		if (f_flags & CONFIRM_FLAG)
			addstr(CONFIRM_KEY_LABEL);
		if (f_flags & EXIT_FLAG)
			addstr(EXIT_KEY_LABEL);
		if (f_flags & CANCEL_FLAG)
			addstr(CANCEL_KEY_LABEL);
		if (f_flags & CONFIRM_REC_FLAG)
			addstr(CONFIRM_REC_KEY_LABEL);
		if (f_flags & REFRESH_FLAG)
			addstr(REFRESH_KEY_LABEL);
		if (f_flags & TOGGLE_FLAG)
			addstr(TOGGLE_KEY_LABEL);
		if ((f_flags & FWD_FLAG) && y_offscr) {
			addstr(PAGE_DOWN_KEY_LABEL);
			addstr(PAGE_UP_KEY_LABEL);
		}

		post_form(form);

		/* add the body of the text to the screen */
		if (pages) {
			wmove(w_page_header,0,0);
			waddstr(w_page_header,body_text);
		}
		wmove(w_pad,0,0);
		waddstr(w_pad,body_text);

		if (num_fields) {
			set_current_field(form,fields[active_field]);
			if (active_field == 0)
				form_driver(form,REQ_NEXT_FIELD);
			form_driver(form,REQ_PREV_FIELD);
		}

		refresh_stdscr = true;

		for(;;) {
			/* add global status if defined */
			if (s_status.index) {
				rc = s_status.index;
				s_status.index = 0;
			}

			if (invalid) {
				status = (char *)screen_status[INVALID_OPTION_STATUS];
				invalid = false;
			} else if (rc != 0) {
				if ((status = strchr(screen_status[rc],'%')) == NULL) {
					status = (char *)screen_status[rc];
				} else if (status[1] == 'd') {
					sprintf(buffer,screen_status[rc],s_status.num);
					status = buffer;
				} else if (status[1] == 's') {
					sprintf(buffer,screen_status[rc],s_status.str);
					status = buffer;
				}
				rc = 0;
			} else
				status = NULL;

			move(stdscr_max_y-1,0);
			clrtoeol(); /* clear last line */
			mvaddstr(stdscr_max_y-1,0,status);
			status = NULL;

			pagedn = false;
			if ((f_flags & FWD_FLAG) && y_offscr) {
				if (!is_bottom)
					mvaddstr(stdscr_max_y-footer_lines,stdscr_max_x-8,_("More..."));
				else
					mvaddstr(stdscr_max_y-footer_lines,stdscr_max_x-8,"       ");

				for (i = 0; i < num_fields; i++) {
					if (!fields[i])
						continue;
					/* height, width, top, left, offscreen rows, buffer */
					field_info(fields[i],&h,&w,&t,&l,&o,&b); 

					/* turn all offscreen fields off */
					if ((t >= (header_lines + pad_t)) &&
					    (t <= (viewable_body_lines + pad_t))) {

						field_opts_on(fields[i],O_ACTIVE);
						if (!form_adjust)
							continue;
						if (!(f_flags & TOGGLE_FLAG) || !toggle_field)
							set_current_field(form,fields[i]);
						form_adjust = false;
					} else if (field_opts_off(fields[i],O_ACTIVE) == E_CURRENT &&
						   toggle_field && i == (toggle_field - 1)) {
						pagedn = true;
					}
				}
			}  

			if (f_flags & MENU_FLAG) {
				for (i = 0; i < num_fields; i++) {
					field_opts_off(fields[i],O_VISIBLE);
					field_opts_on(fields[i],O_VISIBLE);
					set_field_fore(fields[i],A_BOLD);
				}
			}

			if ((f_flags & TOGGLE_FLAG) && toggle_field &&
			    (((field_opts(fields[toggle_field-1]) & O_ACTIVE) != O_ACTIVE) ||
			     pagedn)) {
				pagedn = false;
				fakepagedn = true;
				ch = KEY_NPAGE;
			} else {
				fakepagedn = false;
				toggle_field = 0;

				if (refresh_stdscr) {
					touchwin(stdscr);
					refresh_stdscr = FALSE;
				} else
					touchline(stdscr,stdscr_max_y - 1,1);

				refresh();

				if (pages) {
					touchwin(w_page_header);
					prefresh(w_page_header, 0, pad_l,
						 title_lines, 0,
						 title_lines + header_lines - 1, pad_scr_r - 1);
					touchwin(w_pad);
					prefresh(w_pad, pad_t + header_lines, pad_l,
						 title_lines + header_lines, 0,
						 title_lines + viewable_body_lines, pad_scr_r - 1);
				} else {
					touchwin(w_pad);
					prefresh(w_pad, pad_t, pad_l,
						 title_lines, 0,
						 title_lines + viewable_body_lines, pad_scr_r - 1);
				}

				if (ground_cursor) {
					move(stdscr_max_y - 1, stdscr_max_x - 1);
					refresh();
				}

				ch = wgetch(w_pad); /* pause for user input */
			}

			if (ch == KEY_RESIZE)
				break;

			if (IS_ENTER_KEY(ch)) {
				if (num_fields > 0)
					active_field = field_index(current_field(form));

				if ((f_flags & ENTER_FLAG) && num_fields == 0) {

					rc = (CANCEL_FLAG | rc);
					goto leave;
				} else if (f_flags & ENTER_FLAG) {

					/* cancel if all fields are empty */
					form_driver(form,REQ_VALIDATION);
					for (i = 0; i < num_fields; i++) {
						if (strlen(field_buffer(fields[i],0)) != 0)
							/* fields are not empty -> continue */
							break;

						if (i == (num_fields - 1)) {
							/* all fields are empty */
							rc = (CANCEL_FLAG | rc);
							goto leave;
						}
					}
				}

				if (num_fields > 0)
					input = field_buffer(fields[active_field],0);

				invalid = true;

				for (i = 0; i < screen->num_opts; i++) {
					temp = &(screen->options[i]);

					if ((temp->key[0] == '\n') ||
					    ((num_fields > 0)?(strcasecmp(input,temp->key) == 0):0)) {

						invalid = false;

						if ((temp->key[0] == '\n') && (num_fields > 0)) {

							/* store field data to existing i_con (which should already
							 contain pointens) */
							i_container *temp_i_con = i_con_head;
							form_driver(form,REQ_VALIDATION);

							for (i = 0; i < num_fields; i++) {
								strncpy(temp_i_con->field_data,field_buffer(fields[i],0),MAX_FIELD_SIZE);
								temp_i_con = temp_i_con->next_item;
							}
						}

						if (temp->screen_function == NULL)
							/* continue with function */
							goto leave; 

						toggle_field = 0;

						do
							rc = temp->screen_function(i_con_head);
						while (rc == REFRESH_SCREEN || rc & REFRESH_FLAG);

						/* if screen flags exist on rc and they don't match
						 the screen's flags, return */
						if ((rc & 0xF000) && !(screen->rc_flags & (rc & 0xF000)))
							goto leave; 

						/* strip screen flags from rc */
						rc &= ~(EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG); 

						if (screen->rc_flags & REFRESH_FLAG) {

							s_status.index = rc;
							rc = (REFRESH_FLAG | rc);
							goto leave;
						}
						break;
					}
				}

				if (!invalid) {
					/*clear fields*/
					form_driver(form,REQ_LAST_FIELD);
					for (i = 0; i < num_fields; i++) {
						form_driver(form,REQ_CLR_FIELD);
						form_driver(form,REQ_NEXT_FIELD);
					}
					break;
				}
			} else if (IS_EXIT_KEY(ch) && (f_flags & EXIT_FLAG)) {
				rc = (EXIT_FLAG | rc);
				goto leave;
			} else if (IS_CANCEL_KEY(ch) &&
				   ((f_flags & CANCEL_FLAG) || screen == &n_main_menu))	{
				rc = (CANCEL_FLAG | rc);
				goto leave;
			} else if ((IS_REFRESH_KEY(ch) && (f_flags & REFRESH_FLAG)) ||
				   (ch == KEY_HOME && (f_flags & REFRESH_FLAG))) {
				rc = REFRESH_SCREEN;
				if (num_fields > 1)
					toggle_field = field_index(current_field(form)) + 1;
				goto leave;
			} else if (IS_TOGGLE_KEY(ch) && (f_flags & TOGGLE_FLAG)) {
				rc = TOGGLE_SCREEN;
				if (num_fields > 1)
					toggle_field = field_index(current_field(form)) + 1;
				goto leave;
			} else if (IS_PGUP_KEY(ch) && (f_flags & FWD_FLAG) && y_offscr)	{
				invalid = false;
				if (pad_t > 0) {
					pad_t -= (viewable_body_lines - header_lines + 1);
					rc = PGUP_STATUS;
				} else if ((f_flags & FWD_FLAG) && y_offscr)
					rc = TOP_STATUS;
				else
					invalid = true;

				if (!invalid) {
					if (pad_t < 0)
						pad_t = 0;

					is_bottom = false;
					form_adjust = true;
					refresh_stdscr = true;
				}
			} else if (IS_PGDN_KEY(ch) && (f_flags & FWD_FLAG) && y_offscr)	{
				invalid = false;
				if ((stdscr_max_y - title_lines - footer_lines + 1) < (w_pad_max_y - pad_t)) {
					pad_t += (viewable_body_lines - header_lines + 1);
					rc = PGDN_STATUS;

					if ((stdscr_max_y - title_lines - footer_lines + 1) >= (w_pad_max_y - pad_t))
						is_bottom = true;
				} else if (is_bottom)
					rc = BTM_STATUS;
				else
					invalid = true;

				if (fakepagedn)
					rc = 0;
				if (!invalid) {
					form_adjust = true;
					refresh_stdscr = true;
				}
			} else if (ch == KEY_RIGHT) {
				if (x_offscr && stdscr_max_x+pad_l<w_pad_max_x)
					pad_l++;
				else 
					form_driver(form, REQ_NEXT_CHAR);
			} else if (ch == KEY_LEFT) {
				if (pad_l>0)
					pad_l--;
				else
					form_driver(form, REQ_PREV_CHAR);
			} else if (ch == KEY_UP) {
				if ((y_offscr || (num_fields == 0)) && !pages) {
					if (pad_t > 0) {
						pad_t--;
						form_adjust = true;
						is_bottom = false;
					}
				} else
					form_driver(form, REQ_PREV_FIELD);
			} else if (ch == KEY_DOWN) {
				if ((y_offscr || (num_fields == 0)) && !pages) {
					if (y_offscr && ((stdscr_max_y + pad_t ) <
					     (w_pad_max_y + title_lines + footer_lines))) {
						pad_t++;
						form_adjust = true;
					}
					if (!((stdscr_max_y + pad_t) <
					      (w_pad_max_y + title_lines + footer_lines)))

						is_bottom = true;
				}
				else
					form_driver(form, REQ_NEXT_FIELD);
			} else if ((ch == KEY_BACKSPACE) || (ch == 127))
				form_driver(form, REQ_DEL_PREV);
			else if (ch == KEY_DC)
				form_driver(form, REQ_DEL_CHAR);
			else if (ch == KEY_IC)
				form_driver(form, REQ_INS_MODE);
			else if (ch == KEY_EIC)
				form_driver(form, REQ_OVL_MODE);
			else if (ch == KEY_HOME)
				form_driver(form, REQ_BEG_FIELD);
			else if (ch == KEY_END)
				form_driver(form, REQ_END_FIELD);
			else if (ch == '\t')
				form_driver(form, REQ_NEXT_FIELD);
			else if ((f_flags & MENU_FLAG) || (num_fields == 0)) {
				invalid = true;

				for (i = 0; i < screen->num_opts; i++) {
					temp = &(screen->options[i]);

					if ((temp->key) && (ch == temp->key[0])) {
						invalid = false;

						if (num_fields > 0) {
							active_field = field_index(current_field(form));
							input = field_buffer(fields[active_field],0);

							if (active_field < num_i_cons) {
								temp_i_con = i_con_head;
								for (j = 0; j < active_field; j++)
									temp_i_con = temp_i_con->next_item;

								temp_i_con->field_data[0] = '\0';

								getyx(w_pad,y,x);
								temp_i_con->y = y;
								temp_i_con->x = x;
							}
							toggle_field = field_index(current_field(form)) + 1;
						}

						if (temp->screen_function == NULL)
							goto leave;

						do 
							rc = temp->screen_function(i_con_head);
						while (rc == REFRESH_SCREEN || rc & REFRESH_FLAG);

						if ((rc & 0xF000) && !(screen->rc_flags & (rc & 0xF000)))
							goto leave;

						rc &= ~(EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG);

						if (screen->rc_flags & REFRESH_FLAG) {
							rc = (REFRESH_FLAG | rc);
							goto leave;
						}		      
						break;
					}
				}
			} else if (isascii(ch))
				form_driver(form, ch);
		}
	}

leave:
	s_out->i_con = i_con_head;
	s_out->rc = rc;
	free(body_text);
	unpost_form(form);
	free_form(form);

	for (i = 0; i < num_fields; i++) {
		if (fields[i] != NULL)
			free_field(fields[i]);
	}
	free(fields);
	delwin(w_pad);
	delwin(w_page_header);
	return s_out;
}

static char *status_hdr[] = {
		/*   .        .                  .            .                           .          */
		/*012345678901234567890123456789012345678901234567890123456789012345678901234567890 */
		"OPT Name   Resource Path/Address      Vendor   Product ID          Status",
		"OPT Name   PCI/SCSI Location          Description                  Status",
		"Name   Resource Path/Address      Vendor   Product ID       Status",
		"Name   PCI/SCSI Location          Description               Status",
		"OPT SAS Port/SAS Address   Description        Active Status            Info",
		"OPT SAS Port/SAS Address   Description        Active Status            Info",
		"SAS Port/SAS Address   Description        Active Status            Info",
		"SAS Port/SAS Address   Description        Active Status            Info",
		"OPT Name   Platform Location          Description                  Status",
		"OPT Name   SCSI Host/Resource Path      Vendor   Product ID          Status",
		"OPT Name   SCSI Host/Resource Path      Vendor   Product ID          Status",
		"Name   Platform Location          Description                  Status",
		"OPT Name   PCI/Host/Resource Path                   Serial Number Status",
		"OPT Name   Physical Location                        Production ID    Status",
		"Name   Physical Location                        Serial Number Status",
		"Name   PCI/SCSI Location          Vendor   Product ID          Current    Available",
		"OPT Name   PCI/SCSI Location          Vendor   Product ID          Current    Available",
		"OPT Name   Resource Path/Address      Vendor   Product ID          Current    Available",
};

static char *status_sep[] = {
		"--- ------ -------------------------- -------- ------------------- -----------------",
		"--- ------ -------------------------  ---------------------------- -----------------",
		"------ -------------------------- -------- ---------------- -----------------",
		"------ -------------------------  ------------------------- -----------------",
		"--- --------------------- ------------------ ------ ----------------- ----------",
		"--- ---------------------- ------------------ ------ ----------------- ----------",
		"---------------------- ------------------ ------ ----------------- ----------",
		"---------------------- ------------------ ------ ----------------- ----------",
		"--- ------ -------------------------- ---------------------------- -----------------",
		"--- ------ ---------------------------- -------- ------------------- --------------",
		"--- ------ ---------------------------- -------- ------------------- --------------",
		"------ -------------------------- ---------------------------- ------------",
		"--- ------ ---------------------------------------- ------------- ------------",
		"--- ------ ---------------------------------------- ---------------- ------------",
		"------ ---------------------------------------- ------------- ------------",
		"-----  ----------------------     -------- ----------------    ---------  ----------",
		"--- -----  ----------------------     -------- ----------------    ---------  ----------",
		"--- -----  ----------------------     -------- ----------------    ---------  ----------",
};

/**
 * status_header - 
 * @buffer:		data buffer
 * @num_lines:		number of lines
 * @type:		type index
 *
 * Returns:
 *   buffer
 **/
static char *status_header(char *buffer, int *num_lines, int type)
{
	int cur_len = strlen(buffer);
	int header_lines = 0;

	buffer = realloc(buffer, cur_len + strlen(status_hdr[type]) + strlen(status_sep[type]) + 8);
	cur_len += sprintf(buffer + cur_len, "%s\n", status_hdr[type]);
	cur_len += sprintf(buffer + cur_len, "%s\n", status_sep[type]);
	header_lines += 2;

	*num_lines = header_lines + *num_lines;
	return buffer;
}

/**
 * add_string_to_body - adds a string of data to fit in the text window,
 * 			putting carraige returns in when necessary
 * @body:		
 * @new_text:		
 * @line_fill:		
 * @line_num:		
 *
 * Returns:
 *   body on success / 0 if no space available
 **/
/* NOTE:  This routine expects the language conversion to be done before
 entering this routine */
static char *add_string_to_body(char *body, char *new_text, char *line_fill, int *line_num)
{
	int body_len = 0;
	int new_text_len = strlen(new_text);
	int line_fill_len = strlen(line_fill);
	int rem_length;
	int new_text_offset = 0;
	int max_y, max_x;
	int len, i;
	int num_lines = 0;
	struct winsize w;

	if (body)
		body_len = strlen(body);

	len = body_len;

	if (use_curses)
		getmaxyx(stdscr,max_y,max_x);
	else {
		memset(&w, 0, sizeof(struct winsize));
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		if (w.ws_col > 0)
			max_x = w.ws_col;
		else
			max_x = 80;
	}
	rem_length = max_x - 1;

	for (i = 0; i < strlen(new_text); i++) {
		if (new_text[i] == '\n')
			num_lines++;
	}

	while (len != 0) {
		if (body[len--] == '\n')
			break;
		rem_length--;
	}

	while (1) {
		if (new_text_len < rem_length) {
			/* done, all data fits in current line */
			body = realloc(body, body_len + new_text_len + 8);
			sprintf(body + body_len, "%s", &new_text[new_text_offset]);
			break;
		}

		len = rem_length;
		while (len != 0) {
			if (new_text[new_text_offset + len] == ' ') {
				/* found a place to \n */
				break;
			}
			len--;
		}

		if (len == 0)
			/* no space to fit any word */
			return 0;

		/* adjust len to compensate 0 based array */
		len += 1;

		body = realloc(body, body_len + len + line_fill_len + 8);
		strncpy(body + body_len, new_text + new_text_offset, len);
		body_len += len;
		new_text_offset += len;
		new_text_len -= len;

		body_len += sprintf(body + body_len, "\n%s", line_fill);
		rem_length = max_x - 1 - line_fill_len;
		num_lines++;
	}

	if (line_num)
		*line_num = *line_num + num_lines;
	return body;
}

/**
 * add_line_to_body - adds a single line to the body text of the screen, it is
 *		      intented primarily to list items with the desciptor on the
 *		      left of the screen and ". . . :" to the data field for the
 *		      descriptor.
 * @body:		
 * @new_text:		
 * @field_data:		
 *
 * Returns:
 *   body
 **/
/* NOTE:  This routine expects the language conversion to be done before
 entering this routine */
#define DETAILS_DESCR_LEN 40
static char *add_line_to_body(char *body, char *new_text, char *field_data)
{
	int cur_len = 0, start_len = 0;
	int str_len, field_data_len;
	int i;

	if (body != NULL)
		start_len = cur_len = strlen(body);

	str_len = strlen(new_text);

	if (!field_data) {
		body = realloc(body, cur_len + str_len + 8);
		sprintf(body + cur_len, "%s\n", new_text);
	} else {
		field_data_len = strlen(field_data);

		if (str_len < DETAILS_DESCR_LEN) {
			body = realloc(body, cur_len + field_data_len + DETAILS_DESCR_LEN + 8);
			cur_len += sprintf(body + cur_len, "%s", new_text);
			for (i = cur_len; i < DETAILS_DESCR_LEN + start_len; i++) {
				if ((cur_len + start_len) % 2)
					cur_len += sprintf(body + cur_len, ".");
				else
					cur_len += sprintf(body + cur_len, " ");
			}
			sprintf(body + cur_len, " : %s\n", field_data);
		} else {
			body = realloc(body, cur_len + str_len + field_data_len + 8);
			sprintf(body + cur_len, "%s : %s\n",new_text, field_data);
		}
	}

	return body;
}

/**
 * __body_init_status - 
 * @header:		
 * @num_lines:		number of lines
 * @type:		type index
 *
 * Returns:
 *   data buffer
 **/
static char *__body_init_status(char **header, int *num_lines, int type)
{
	char *buffer = NULL;
	int header_lines = 0;
	int j;
	char *x_header;

	for (j = 0, x_header = _(header[j]); strlen(x_header) != 0; j++, x_header = _(header[j]))
		buffer = add_string_to_body(buffer, x_header, "", &header_lines);

	buffer = status_header(buffer, &header_lines, type);

	if (num_lines != NULL)
		*num_lines = header_lines;
	return buffer;
}

/**
 * body_init_status - 
 * @buffer:		char array
 * @header:		
 * @num_lines:		number of lines
 *
 * Returns:
 *   nothing
 **/
static void body_init_status(char *buffer[2], char **header, int *num_lines)
{
	int z;

	for (z = 0; z < 2; z++)
		buffer[z] = __body_init_status(header, num_lines, z);
}

static void body_init_status2(char *buffer[2], char **header, int *num_lines,
			      int type)
{
	int z;

	for (z = 0; z < 2; z++)
		buffer[z] = __body_init_status(header, num_lines, type+z);
}

/**
 * body_init_status_conc -
 * @buffer:		char array
 * @header:
 * @num_lines:		number of lines
 *
 * Returns:
 *   nothing
 **/
static void body_init_status_conc(char *buffer[3], char **header, int *num_lines)
{
	int z;

	for (z = 0; z < 3; z++)
		buffer[z] = __body_init_status(header, num_lines, (z + 8));
}

/**
 * body_init_status_enclosure -
 * @buffer:		char array
 * @header:
 * @num_lines:		number of lines
 *
 * Returns:
 *   nothing
 **/
static void body_init_status_enclosure(char *buffer[2], char **header, int *num_lines)
{
	buffer[0] = __body_init_status(header, num_lines, 12);
	buffer[1] = __body_init_status(header, num_lines, 13);
}

/**
 * __body_init - 
 * @buffer:		char array
 * @header:		
 * @num_lines:		number of lines
 *
 * Returns:
 *   data buffer
 **/
static char *__body_init(char *buffer, char **header, int *num_lines)
{
	int j;
	int header_lines = 0;
	char *x_header;

	if (num_lines)
		header_lines = *num_lines;

	for (j = 0, x_header = _(header[j]); strlen(x_header) != 0; j++, x_header = _(header[j])) {
		if (j == 0)
			buffer = add_string_to_body(buffer, x_header, "", &header_lines);
		else
			buffer = add_string_to_body(buffer, x_header, "   ", &header_lines);
	}

	if (num_lines)
		*num_lines = header_lines;
	return buffer;
}

/**
 * body_init - 
 * @header:		
 * @num_lines:		number of lines
 *
 * Returns:
 *   data buffer
 **/
static char *body_init(char **header, int *num_lines)
{
	if (num_lines)
		*num_lines = 0;
	return __body_init(NULL, header, num_lines);
}

/**
 * __complete_screen_driver_curses - 
 * @n_screen:		
 * @complete:		% complete string
 * @allow_exit:		allow exit flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __complete_screen_driver_curses(s_node *n_screen, char *complete, int allow_exit)
{
	int stdscr_max_y,stdscr_max_x,center,len;
	int ch;
	char *exit = _("Return to menu, current operations will continue.");
	char *exit_str;

	getmaxyx(stdscr,stdscr_max_y,stdscr_max_x);
	clear();
	center = stdscr_max_x/2;
	len = strlen(n_screen->title);

	/* add the title at the center of stdscr */
	mvaddstr(0,(center - len/2) > 0 ? center-len/2:0, n_screen->title);
	mvaddstr(2,0,n_screen->body);

	len = strlen(complete);
	mvaddstr(stdscr_max_y/2,(center - len/2) > 0 ? center - len/2:0,
		 complete);

	if (allow_exit) {
		exit_str = malloc(strlen(exit) + strlen(EXIT_KEY_LABEL) + 8);
		sprintf(exit_str, EXIT_KEY_LABEL"%s",exit);
		mvaddstr(stdscr_max_y - 3, 2, exit_str);

		nodelay(stdscr, TRUE);

		do {
			ch = getch();
			if (IS_EXIT_KEY(ch)) {
				nodelay(stdscr, FALSE);
				return EXIT_FLAG;
			}
		} while (ch != ERR);

		nodelay(stdscr, FALSE);
	}

	move(stdscr_max_y - 1, stdscr_max_x - 1);
	refresh();
	return 0;
}

/**
 * __complete_screen_driver - 
 * @n_screen:		
 * @complete:		% complete string
 * @allow_exit:		allow_exit flag
 *
 * Returns:
 *   0
 **/
static int __complete_screen_driver(s_node *n_screen, char *complete, int allow_exit)
{
	fprintf(stdout, "\r%s", complete);
	fflush(stdout);
	return 0;
}

/**
 * complete_screen_driver - 
 * @n_screen:		
 * @percent:		% complete value
 * @allow_exit:		allow exit flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int complete_screen_driver(s_node *n_screen, int percent, int allow_exit)
{
	char *complete = _("Complete");
	char *percent_comp;
	int rc;

	percent_comp = malloc(strlen(complete) + 8);
	sprintf(percent_comp,"%3d%% %s", percent, complete);

	if (use_curses)
		rc = __complete_screen_driver_curses(n_screen, percent_comp, allow_exit);
	else
		rc = __complete_screen_driver(n_screen, percent_comp, allow_exit);
	free(percent_comp);

	return rc;
}

/**
 * time_screen_driver - 
 * @n_screen:		
 * @seconds:		number of seconds
 * @allow_exit:		allow exit flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int time_screen_driver(s_node *n_screen, unsigned long seconds, int allow_exit)
{
	char *complete = _("Elapsed Time: ");
	char *comp_str;
	int rc, hours, minutes;

	hours = seconds/3600;
	seconds -= (hours * 3600);
	minutes = seconds/60;
	seconds -= (minutes *60);

	comp_str = malloc(strlen(complete) + 40);
	sprintf(comp_str,"%s%d:%02d:%02ld", complete, hours, minutes, seconds);

	if (use_curses)
		rc = __complete_screen_driver_curses(n_screen, comp_str, allow_exit);
	else
		rc = __complete_screen_driver(n_screen, comp_str, allow_exit);
	free(comp_str);

	return rc;
}

/**
 * evaluate_device - 
 * @dev:		ipr dev struct
 * @ioa:		ipr ioa struct
 * @new_block_size:	not used?
 *
 * Returns:
 *   nothing
 **/
static void evaluate_device(struct ipr_dev *dev, struct ipr_ioa *ioa,
			    int new_block_size)
{
	struct scsi_dev_data *scsi_dev_data = dev->scsi_dev_data;
	struct ipr_res_addr res_addr;
	struct ipr_dev_record *dev_record;
	int device_converted;
	int timeout = 60;
	u32 res_handle;

	if (scsi_dev_data) {
		res_handle = scsi_dev_data->handle;
		res_addr.bus = scsi_dev_data->channel;
		res_addr.target = scsi_dev_data->id;
		res_addr.lun = scsi_dev_data->lun;
	} else if (ipr_is_af_dasd_device(dev)) {
		dev_record = dev->dev_rcd;
		if (dev_record->no_cfgte_dev) 
			return;

		res_handle = ntohl(dev->resource_handle);
		res_addr.bus = dev_record->type2.resource_addr.bus;
		res_addr.target = dev_record->type2.resource_addr.target;
		res_addr.lun = dev_record->type2.resource_addr.lun;
	} else
		return;

	/* send evaluate device down */
	ipr_evaluate_device(&ioa->ioa, res_handle);

	device_converted = 0;

	while (!device_converted && timeout) {
		/* xxx FIXME how to determine evaluate device completed? */
		device_converted = 1;
		sleep(1);
		timeout--;
	}
}

/**
 * verify_device - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
static void verify_device(struct ipr_dev *dev)
{
	int rc;
	struct ipr_cmd_status cmd_status;
	struct sense_data_t sense_data;

	if (ipr_fast)
		return;

	/* Send Test Unit Ready to start device if its a volume set */
	if (ipr_is_volume_set(dev)) {
		ipr_test_unit_ready(dev, &sense_data);
		return;
	}

	if (ipr_device_lock(dev))
		return;

	if (ipr_is_af(dev)) {
		if ((rc = ipr_query_command_status(dev, &cmd_status)))
			return;

		if (cmd_status.num_records != 0 &&
		    cmd_status.record->status == IPR_CMD_STATUS_SUCCESSFUL) {
			if (ipr_get_blk_size(dev) == 512)
				evaluate_device(dev, dev->ioa, IPR_JBOD_BLOCK_SIZE);
			else if (dev->ioa->support_4k && ipr_get_blk_size(dev) == IPR_JBOD_4K_BLOCK_SIZE)
				evaluate_device(dev, dev->ioa, IPR_JBOD_4K_BLOCK_SIZE);
		}
	} else if (dev->scsi_dev_data->type == TYPE_DISK) {
		rc = ipr_test_unit_ready(dev, &sense_data);
		if (!rc) {
			if (ipr_get_blk_size(dev) == dev->ioa->af_block_size &&
			    dev->ioa->qac_data->num_records != 0) {
				enable_af(dev);
				evaluate_device(dev, dev->ioa, dev->ioa->af_block_size);
			}
			else if (dev->ioa->support_4k && ipr_get_blk_size(dev) == IPR_AF_4K_BLOCK_SIZE &&
			    dev->ioa->qac_data->num_records != 0) {
				enable_af(dev);
				evaluate_device(dev, dev->ioa, IPR_AF_4K_BLOCK_SIZE);
			}
		}
	}

	ipr_device_unlock(dev);
}

/**
 * processing - displays "Processing" at the bottom of the screen
 *
 * Returns:
 *   nothing
 **/
static void processing()
{
	int max_y, max_x;

	if (!use_curses)
		return;

	getmaxyx(stdscr,max_y,max_x);
	move(max_y-1,0);
	printw(_("Processing"));
	refresh();
}

int display_features_menu(i_container *i_con, s_node *menu)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc, loop;
	struct stat file_stat;
	struct screen_output *s_out;
	int offset = 0;

	for (loop = 0; loop < (menu->num_opts); loop++) {
		menu->body = ipr_list_opts(menu->body,
					   menu->options[loop].key,
					   menu->options[loop].list_str);
	}
	menu->body = ipr_end_list(menu->body);

	s_out = screen_driver(menu, 0, NULL);
	free(menu->body);
	menu->body = NULL;
	rc = s_out->rc;
	i_con = s_out->i_con;
	i_con = free_i_con(i_con);
	free(s_out);
	return rc;
}

/**
 * main_menu - Main menu
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int main_menu(i_container *i_con)
{
	int j;
	struct ipr_ioa *ioa;
	struct scsi_dev_data *scsi_dev_data;

	processing();
	check_current_config(false);

	for_each_ioa(ioa) {
		for (j = 0; j < ioa->num_devices; j++) {
			scsi_dev_data = ioa->dev[j].scsi_dev_data;
			if (ipr_is_ioa(&ioa->dev[j]))
				continue;
			verify_device(&ioa->dev[j]);
		}
	}

	return display_features_menu(i_con, &n_main_menu);
}

/**
 * conf_menu - Config menu
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int config_menu(i_container *i_con)
{
	return display_features_menu(i_con, &n_config_menu);
}


/**
 * print_standalone_disks - 
 * @ioa:		ipr ioa struct
 * @i_con:		i_container struct
 * @buffer:		
 * @type:		
 *
 * Returns:
 *   number of lines printed
 **/
static int print_standalone_disks(struct ipr_ioa *ioa,
				  i_container **i_con, char **buffer, int type)
{
	struct ipr_dev *dev;
	int k;
	int num_lines = 0;

	__for_each_standalone_disk(ioa, dev) {
		print_dev (k, dev, buffer, "%1", type+k);
		*i_con = add_i_con(*i_con, "\0", dev);  
		num_lines++;
	}

	return num_lines;
}

/**
 * print_hotspare_disks - 
 * @ioa:		ipr ioa struct
 * @i_con:		i_container struct
 * @buffer:		
 * @type:		
 *
 * Returns:
 *   number of lines printed
 **/
static int print_hotspare_disks(struct ipr_ioa *ioa,
				i_container **i_con, char **buffer, int type)
{
	struct ipr_dev *dev;
	int k;
	int num_lines = 0;

	for_each_hot_spare(ioa, dev) {
		print_dev(k, dev, buffer, "%1", type+k);
		*i_con = add_i_con(*i_con, "\0", dev);  
		num_lines++;
	}

	return num_lines;
}

/**
 * print_ses_devices - 
 * @ioa:		ipr ioa struct
 * @i_con:		i_container struct
 * @buffer:		
 * @type:		
 *
 * Returns:
 *   number of lines printed
 **/
static int print_ses_devices(struct ipr_ioa *ioa,
			     i_container **i_con, char **buffer, int type)
{
	struct ipr_dev *dev;
	int k;
	int num_lines = 0;

	for_each_ses(ioa, dev) {
		print_dev(k, dev, buffer, "%1", type+k);
		*i_con = add_i_con(*i_con, "\0", dev);  
		num_lines++;
	}

	return num_lines;
}

static int print_dvd_tape_devices(struct ipr_ioa *ioa,
			     i_container **i_con, char **buffer, int type)
{
	struct ipr_dev *dev;
	int k;
	int num_lines = 0;

	for_each_dvd_tape(ioa, dev) {
		print_dev(k, dev, buffer, "%1", type+k);
		*i_con = add_i_con(*i_con, "\0", dev);  
		num_lines++;
	}

	return num_lines;
}

/**
 * print_sas_ses_devices - 
 * @ioa:		ipr ioa struct
 * @i_con:		i_container struct
 * @buffer:		
 * @type:		
 *
 * Returns:
 *   number of lines printed
 **/
static int print_sas_ses_devices(struct ipr_ioa *ioa,
				 i_container **i_con, char **buffer, int type)
{
	struct ipr_dev *dev;
	int k;
	int num_lines = 0;

	if (__ioa_is_spi(ioa))
		return 0;

	for_each_ses(ioa, dev) {
		print_dev(k, dev, buffer, "%1", type+k);
		*i_con = add_i_con(*i_con, "\0", dev);  
		num_lines++;
	}

	return num_lines;
}

/**
 * print_vsets - 
 * @ioa:		ipr ioa struct
 * @i_con:		i_container struct
 * @buffer:		
 * @type:		
 *
 * Returns:
 *   number of lines printed
 **/
static int print_vsets(struct ipr_ioa *ioa,
		       i_container **i_con, char **buffer, int type)
{
	int k;
	int num_lines = 0;
	struct ipr_dev *vset, *dev;

	/* print volume set and associated devices*/
	for_each_vset(ioa, vset) {
		print_dev(k, vset, buffer, "%1", type+k);
		*i_con = add_i_con(*i_con, "\0", vset);
		num_lines++;

		for_each_dev_in_vset(vset, dev) {
			print_dev(k, dev, buffer, "%1", type+k);
			*i_con = add_i_con(*i_con, "\0", dev);
			num_lines++;
		}
	}

	return num_lines;
}

/**
 * disk_status - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int disk_status(i_container *i_con)
{
	int rc, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct screen_output *s_out;
	int header_lines;
	char *buffer[2];
	int toggle = 1;

	processing();

	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status(buffer, n_disk_status.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->ioa.scsi_dev_data)
			continue;

		print_dev(k, &ioa->ioa, buffer, "%1", 2+k);
		i_con = add_i_con(i_con, "\0", &ioa->ioa);

		num_lines++;

		/* xxxx remove? */
		__for_each_standalone_disk(ioa, dev)
			verify_device(dev);
		num_lines += print_standalone_disks(ioa, &i_con, buffer, 2);
		num_lines += print_hotspare_disks(ioa, &i_con, buffer, 2);
		num_lines += print_vsets(ioa, &i_con, buffer, 2);
		num_lines += print_dvd_tape_devices(ioa, &i_con, buffer, 2);
		num_lines += print_ses_devices(ioa, &i_con, buffer, 2);
	}

	if (num_lines == 0) {
		for (k = 0; k < 2; k++) {
			len = strlen(buffer[k]);
			buffer[k] = realloc(buffer[k], len +
						strlen(_(no_dev_found)) + 8);
			sprintf(buffer[k] + len, "\n%s", _(no_dev_found));
		}
	}

	do {
		n_disk_status.body = buffer[toggle&1];
		s_out = screen_driver(&n_disk_status, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_disk_status.body = NULL;

	return rc;
}

/**
 * device_details_get_device - 
 * @i_con:		i_container struct
 * @device:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int device_details_get_device(i_container *i_con,
				     struct ipr_dev **device)
{
	i_container *temp_i_con;
	int invalid=0;
	int dev_num = 0;
	struct ipr_dev *cur_device;
	char *input;

	if (!i_con)
		return 1;

	for (temp_i_con = i_con_head; temp_i_con && !invalid;
	     temp_i_con = temp_i_con->next_item) {

		cur_device = (struct ipr_dev *)(temp_i_con->data);

		if (!cur_device)
			continue;

		input = temp_i_con->field_data;

		if (!input)
			continue;

		if (strcmp(input, "1") == 0) {
			if (dev_num) {
				dev_num++;
				break;
			} else {
				*device = cur_device;
				dev_num++;
			}
		} else if ((strcmp(input, " ") != 0) &&
			   (input != NULL)           &&
			   (strcmp(input, "") != 0)) {

			invalid = 1;
		}
	}

	if (invalid)
		/* The selection is not valid */
		return 15; 

	if (dev_num == 0)
		/* Invalid option.  No devices selected. */
		return 17; 
	else if (dev_num != 1) 
		/* Error.  More than one unit was selected. */
		return 16; 

	return 0;
}

/**
 * ioa_details - get IOA details
 * @body:		data buffer
 * @dev:		ipr dev struct
 *
 * Returns:
 *   body
 **/
static char *ioa_details(char *body, struct ipr_dev *dev)
{
	struct ipr_ioa_vpd ioa_vpd;
	struct ipr_cfc_vpd cfc_vpd;
	struct ipr_cache_cap_vpd cc_vpd;
	struct ipr_dram_vpd dram_vpd;
	struct ipr_inquiry_page0 page0_inq;
	struct scsi_dev_data *scsi_dev_data = dev->scsi_dev_data;
	struct ipr_dual_ioa_entry *ioa_entry;
	struct ipr_reclaim_query_data reclaim_data;
	int rc, i;
	char buffer[200];
	char *dynbuf;
	int cache_size, dram_size;
	u32 fw_level;

	memset(&ioa_vpd, 0, sizeof(ioa_vpd));
	memset(&cfc_vpd, 0, sizeof(cfc_vpd));
	memset(&cc_vpd, 0, sizeof(cc_vpd));
	memset(&dram_vpd, 0, sizeof(dram_vpd));
	memset(&reclaim_data, 0, sizeof(reclaim_data));

	ipr_inquiry(dev, IPR_STD_INQUIRY, &ioa_vpd, sizeof(ioa_vpd));

	rc = ipr_inquiry(dev, 0, &page0_inq, sizeof(page0_inq));
	for (i = 0; !rc && i < page0_inq.page_length; i++) {
		if (page0_inq.supported_page_codes[i] == 1) {
			ipr_inquiry(dev, 1, &cfc_vpd, sizeof(cfc_vpd));
			break;
		}
		if (page0_inq.supported_page_codes[i] == 0xC4) {
			ipr_inquiry(dev, 0xC4, &cc_vpd, sizeof(cc_vpd));
			break;
		}
	}

	ipr_inquiry(dev, 2, &dram_vpd, sizeof(dram_vpd));

	fw_level = get_fw_version(&dev->ioa->ioa);

	body = add_line_to_body(body,"", NULL);
	ipr_strncpy_0(buffer, scsi_dev_data->vendor_id, IPR_VENDOR_ID_LEN);
	body = add_line_to_body(body,_("Manufacturer"), buffer);
	ipr_strncpy_0(buffer, scsi_dev_data->product_id, IPR_PROD_ID_LEN);
	body = add_line_to_body(body,_("Machine Type and Model"), buffer);
	sprintf(buffer,"%08X", fw_level);
	body = add_line_to_body(body,_("Firmware Version"), buffer);
	if (!dev->ioa->ioa_dead) {
		ipr_strncpy_0(buffer, (char *)ioa_vpd.std_inq_data.serial_num, IPR_SERIAL_NUM_LEN);
		body = add_line_to_body(body,_("Serial Number"), buffer);
		ipr_strncpy_0(buffer, (char *)ioa_vpd.ascii_part_num, IPR_VPD_PART_NUM_LEN);
		body = add_line_to_body(body,_("Part Number"), buffer);
		ipr_strncpy_0(buffer, (char *)ioa_vpd.ascii_plant_code, IPR_VPD_PLANT_CODE_LEN);
		body = add_line_to_body(body,_("Plant of Manufacturer"), buffer);
		if (cfc_vpd.cache_size[0]) {
			ipr_strncpy_0(buffer, (char *)cfc_vpd.cache_size, IPR_VPD_CACHE_SIZE_LEN);
			cache_size = strtoul(buffer, NULL, 16);
			sprintf(buffer,"%d MB", cache_size);
			body = add_line_to_body(body,_("Cache Size"), buffer);
		}
		if (ntohl(cc_vpd.write_cache_size)) {
			sprintf(buffer,"%d MB", ntohl(cc_vpd.write_cache_size));
			body = add_line_to_body(body,_("Write Cache Size"), buffer);
		}
		if (ntohl(cc_vpd.data_store_size)) {
			sprintf(buffer,"%d MB", ntohl(cc_vpd.data_store_size));
			body = add_line_to_body(body,_("DRAM Size"), buffer);
		} else {


			ipr_strncpy_0(buffer, (char *)dram_vpd.dram_size,
				      IPR_VPD_DRAM_SIZE_LEN);
			dram_size = strtoul(buffer, NULL, 16);
			sprintf(buffer, "%d MB", dram_size);
			body = add_line_to_body(body,_("DRAM Size"), buffer);
		}
	}
	body = add_line_to_body(body,_("Resource Name"), dev->gen_name);
	body = add_line_to_body(body,"", NULL);
	body = add_line_to_body(body,_("Physical location"), NULL);
	body = add_line_to_body(body,_("PCI Address"), dev->ioa->pci_address);
	if (dev->ioa->sis64)
		body = add_line_to_body(body,_("Resource Path"), scsi_dev_data->res_path);
	sprintf(buffer,"%d", dev->ioa->host_num);
	body = add_line_to_body(body,_("SCSI Host Number"), buffer);

	if (strlen(dev->ioa->physical_location))
		body = add_line_to_body(body,_("Platform Location"),
					dev->ioa->physical_location);

	if (dev->ioa->dual_raid_support) {
		ioa_entry = (struct ipr_dual_ioa_entry *)
			(((unsigned long)&dev->ioa->ioa_status.cap) +
			 ntohl(dev->ioa->ioa_status.cap.length));
		body = add_line_to_body(body,"", NULL);
		body = add_line_to_body(body,_("Current Dual Adapter State"),
					dev->ioa->dual_state);
		body = add_line_to_body(body,_("Preferred Dual Adapter State"),
					dev->ioa->preferred_dual_state);
		if (ntohl(dev->ioa->ioa_status.num_entries)) {
			ipr_strncpy_0(buffer, (char *)ioa_entry->fmt0.remote_vendor_id, IPR_VENDOR_ID_LEN);
			body = add_line_to_body(body,_("Remote Adapter Manufacturer"), buffer);
			ipr_strncpy_0(buffer, (char *)ioa_entry->fmt0.remote_prod_id, IPR_PROD_ID_LEN);
			body = add_line_to_body(body,_("Remote Adapter Machine Type And Model"),
						buffer);
			ipr_strncpy_0(buffer, (char *)ioa_entry->fmt0.remote_sn, IPR_SERIAL_NUM_LEN);
			body = add_line_to_body(body,_("Remote Adapter Serial Number"), buffer);
		}
	}

	if (dev->ioa->configure_rebuild_verify) {
		if (dev->ioa->disable_rebuild_verify)
			body = add_line_to_body(body,_("Rebuild Verification"), "Disabled");
		else
			body = add_line_to_body(body,_("Rebuild Verification"), "Enabled");
	}

	if (dev->ioa->asymmetric_access) {
		body = add_line_to_body(body,"", NULL);
		if (dev->ioa->asymmetric_access_enabled)
			body = add_line_to_body(body,_("Current Asymmetric Access State"), _("Enabled"));
		else
			body = add_line_to_body(body,_("Current Asymmetric Access State"), _("Disabled"));
	}

	if (dev->ioa->has_cache && !dev->ioa->has_vset_write_cache) {
		if (get_ioa_caching(dev->ioa) == IPR_IOA_REQUESTED_CACHING_DISABLED)
			body = add_line_to_body(body,_("Current Requested Caching Mode"), _("Disabled"));
		else
			body = add_line_to_body(body,_("Current Requested Caching Mode"), _("Default"));
	}

	if (dev->ioa->has_cache) {
		if (dev->ioa->vset_write_cache) {
			dynbuf = "Synchronize Cache";
		} else if (!dev->ioa->has_vset_write_cache) {
			if (ipr_reclaim_cache_store(dev->ioa,
						    IPR_RECLAIM_QUERY,
						    &reclaim_data))
				goto out;

			dynbuf = (reclaim_data.rechargeable_battery_type ?
				  "Battery Backed" : "Supercap Protected");
		} else {
			/* This is the case where adapters even support
			   Sync cache, but it is disabled. This should
			   never happen... unless it is an old Bare
			   Metal system. Let's handle this here so
			   people don't get confused.  */
			   goto out;

		}
		body = add_line_to_body(body, _("Cache Protection"), dynbuf);
	}

	out:
	return body;
}

/**
 * vset_array_details - get vset/array details
 * @body:		data buffer
 * @dev:		ipr dev struct
 *
 * Returns:
 *   body
 **/
static char *vset_array_details(char *body, struct ipr_dev *dev)
{
	struct ipr_read_cap16 read_cap16;
	unsigned long long max_user_lba_int;
	long double device_capacity; 
	double lba_divisor;
	struct ipr_array_record *array_rcd = dev->array_rcd;
	char buffer[100];
	int rc;
	int scsi_channel, scsi_id, scsi_lun;

	if (dev->scsi_dev_data) {
		scsi_channel = dev->scsi_dev_data->channel;
		scsi_id = dev->scsi_dev_data->id;
		scsi_lun = dev->scsi_dev_data->lun;
	} else if (!dev->ioa->sis64) {
		scsi_channel = array_rcd->type2.last_resource_addr.bus;
		scsi_id = array_rcd->type2.last_resource_addr.target;
		scsi_lun = array_rcd->type2.last_resource_addr.lun;
	}

	body = add_line_to_body(body,"", NULL);

	ipr_strncpy_0(buffer, (char *)dev->vendor_id, IPR_VENDOR_ID_LEN);
	body = add_line_to_body(body,_("Manufacturer"), buffer);

	sprintf(buffer,"RAID %s", dev->prot_level_str);
	body = add_line_to_body(body,_("RAID Level"), buffer);

	if (ntohs(dev->stripe_size) > 1024)
		sprintf(buffer,"%d M", ntohs(dev->stripe_size)/1024);
	else
		sprintf(buffer,"%d k", ntohs(dev->stripe_size));

	body = add_line_to_body(body,_("Stripe Size"), buffer);

	/* Do a read capacity to determine the capacity */
	rc = ipr_read_capacity_16(dev, &read_cap16);

	if (!rc && (ntohl(read_cap16.max_user_lba_hi) ||
		    ntohl(read_cap16.max_user_lba_lo)) &&
	    ntohl(read_cap16.block_length)) {

		max_user_lba_int = ntohl(read_cap16.max_user_lba_hi);
		max_user_lba_int <<= 32;
		max_user_lba_int |= ntohl(read_cap16.max_user_lba_lo);

		device_capacity = max_user_lba_int + 1;

		lba_divisor  =
			(1000*1000*1000) / ntohl(read_cap16.block_length);

		sprintf(buffer, "%.2Lf GB", device_capacity / lba_divisor);
		body = add_line_to_body(body, _("Capacity"), buffer);
	}

	if (ipr_is_volume_set(dev)) {
		body = add_line_to_body(body, _("Resource Name"), dev->dev_name);

		if (dev->ioa->sis64 && dev->array_rcd->type3.desc[0] != ' ') {
			ipr_strncpy_0(buffer, (char *)dev->array_rcd->type3.desc,
				      sizeof (dev->array_rcd->type3.desc));
			body = add_line_to_body(body,_("Label"), buffer);
		}
	}

	ipr_strncpy_0(buffer, (char *)dev->serial_number, IPR_SERIAL_NUM_LEN);
	body = add_line_to_body(body,_("Serial Number"), buffer);


	body = add_line_to_body(body, "", NULL);
	body = add_line_to_body(body, _("Physical location"), NULL);
	body = add_line_to_body(body, _("PCI Address"), dev->ioa->pci_address);

	if (dev->ioa->sis64) {
		if (dev->scsi_dev_data)
			body = add_line_to_body(body,_("Resource Path"), dev->scsi_dev_data->res_path);
		else {
			ipr_format_res_path(dev->dev_rcd->type3.res_path, dev->res_path_name, IPR_MAX_RES_PATH_LEN);
			body = add_line_to_body(body,_("Resource Path"), dev->res_path_name);
		}
	}

	if (dev->scsi_dev_data || !dev->ioa->sis64) {
		sprintf(buffer, "%d", dev->ioa->host_num);
		body = add_line_to_body(body, _("SCSI Host Number"), buffer);

		sprintf(buffer, "%d", scsi_channel);
		body = add_line_to_body(body, _("SCSI Channel"), buffer);

		sprintf(buffer, "%d", scsi_id);
		body = add_line_to_body(body, _("SCSI Id"), buffer);

		sprintf(buffer, "%d", scsi_lun);
		body = add_line_to_body(body, _("SCSI Lun"), buffer);
	}

	return body;
}

/**
 * disk_extended_details - get extended disk details
 * @body:		data buffer
 * @dev:		ipr dev struct
 * @std_inq:		ipr_std_inq_data_long struct
 *
 * Returns:
 *   body
 **/
static char *disk_extended_details(char *body, struct ipr_dev *dev,
				   struct ipr_std_inq_data_long *std_inq)
{
	int rc, i;
	struct ipr_inquiry_page0 page0_inq;
	char buffer[100];
	char *hex;

	rc =  ipr_inquiry(dev, 0, &page0_inq, sizeof(page0_inq));

	for (i = 0; !rc && i < page0_inq.page_length; i++) {
		if (page0_inq.supported_page_codes[i] == 0xC7) {
			/* FIXME 0xC7 is SCSD, do further research to find
			 what needs to be tested for this affirmation.*/

			/* xxx add_str_to_body that does the strncpy_0? */

			body = add_line_to_body(body,"", NULL);
			body = add_line_to_body(body,_("Extended Details"), NULL);

			ipr_strncpy_0(buffer, (char *)std_inq->fru_number, IPR_STD_INQ_FRU_NUM_LEN);
			body = add_line_to_body(body,_("FRU Number"), buffer);

			ipr_strncpy_0(buffer, (char *)std_inq->ec_level, IPR_STD_INQ_EC_LEVEL_LEN);
			body = add_line_to_body(body,_("EC Level"), buffer);

			ipr_strncpy_0(buffer, (char *)std_inq->part_number, IPR_STD_INQ_PART_NUM_LEN);
			body = add_line_to_body(body,_("Part Number"), buffer);

			hex = (char *)&std_inq->std_inq_data;
			sprintf(buffer, "%02X%02X%02X%02X%02X%02X%02X%02X",
				hex[0], hex[1], hex[2], hex[3], hex[4], hex[5],	hex[6], hex[7]);
			body = add_line_to_body(body,_("Device Specific (Z0)"), buffer);

			ipr_strncpy_0(buffer, (char *)std_inq->z1_term, IPR_STD_INQ_Z1_TERM_LEN);
			body = add_line_to_body(body,_("Device Specific (Z1)"), buffer);

			ipr_strncpy_0(buffer, (char *)std_inq->z2_term, IPR_STD_INQ_Z2_TERM_LEN);
			body = add_line_to_body(body,_("Device Specific (Z2)"), buffer);

			ipr_strncpy_0(buffer, (char *)std_inq->z3_term, IPR_STD_INQ_Z3_TERM_LEN);
			body = add_line_to_body(body,_("Device Specific (Z3)"), buffer);

			ipr_strncpy_0(buffer, (char *)std_inq->z4_term, IPR_STD_INQ_Z4_TERM_LEN);
			body = add_line_to_body(body,_("Device Specific (Z4)"), buffer);

			ipr_strncpy_0(buffer, (char *)std_inq->z5_term, IPR_STD_INQ_Z5_TERM_LEN);
			body = add_line_to_body(body,_("Device Specific (Z5)"), buffer);

			ipr_strncpy_0(buffer, (char *)std_inq->z6_term, IPR_STD_INQ_Z6_TERM_LEN);
			body = add_line_to_body(body,_("Device Specific (Z6)"), buffer);
			break;
		}
	}

	return body;
}

/**
 * disk_details - get disk details
 * @body:		data buffer
 * @dev:		ipr dev struct
 *
 * Returns:
 *   body
 **/
static char *disk_details(char *body, struct ipr_dev *dev)
{
	int rc;
	struct ipr_std_inq_data_long std_inq;
	struct ipr_dev_record *device_record;
	struct ipr_dasd_inquiry_page3 page3_inq;
	struct ipr_read_cap16 read_cap16;
	unsigned long long max_user_lba_int;
	struct ipr_query_res_state res_state;
	long double device_capacity; 
	double lba_divisor;
	char product_id[IPR_PROD_ID_LEN+1];
	char vendor_id[IPR_VENDOR_ID_LEN+1];
	char serial_num[IPR_SERIAL_NUM_LEN+1];
	char buffer[100];
	int len, scsi_channel, scsi_id, scsi_lun;

	device_record = (struct ipr_dev_record *)dev->dev_rcd;

	if (strlen(dev->physical_location) == 0)
		get_drive_phy_loc(dev->ioa);

	if (dev->scsi_dev_data) {
		scsi_channel = dev->scsi_dev_data->channel;
		scsi_id = dev->scsi_dev_data->id;
		scsi_lun = dev->scsi_dev_data->lun;
	} else if (device_record) {
		scsi_channel = device_record->type2.last_resource_addr.bus;
		scsi_id = device_record->type2.last_resource_addr.target;
		scsi_lun = device_record->type2.last_resource_addr.lun;
	}

	rc = ipr_inquiry(dev, IPR_STD_INQUIRY, &std_inq, sizeof(std_inq));

	if (!rc) {
		ipr_strncpy_0(vendor_id, (char *)std_inq.std_inq_data.vpids.vendor_id,
			      IPR_VENDOR_ID_LEN);
		ipr_strncpy_0(product_id, (char *)std_inq.std_inq_data.vpids.product_id,
			      IPR_PROD_ID_LEN);
		ipr_strncpy_0(serial_num, (char *)std_inq.std_inq_data.serial_num,
			      IPR_SERIAL_NUM_LEN);
	} else if (device_record) {
		ipr_strncpy_0(vendor_id, (char *)dev->vendor_id, IPR_VENDOR_ID_LEN);
		ipr_strncpy_0(product_id, (char *)dev->product_id, IPR_PROD_ID_LEN);
		ipr_strncpy_0(serial_num, (char *)dev->serial_number, IPR_SERIAL_NUM_LEN);
	} else {
		ipr_strncpy_0(vendor_id, dev->scsi_dev_data->vendor_id, IPR_VENDOR_ID_LEN);
		ipr_strncpy_0(product_id, dev->scsi_dev_data->product_id, IPR_PROD_ID_LEN);
		serial_num[0] = '\0';
	}

	body = add_line_to_body(body,"", NULL);
	body = add_line_to_body(body,_("Manufacturer"), vendor_id);
	body = add_line_to_body(body,_("Product ID"), product_id);

	rc = ipr_inquiry(dev, 0x03, &page3_inq, sizeof(page3_inq));

	if (!rc) {
		len = sprintf(buffer, "%X%X%X%X", page3_inq.release_level[0],
			      page3_inq.release_level[1], page3_inq.release_level[2],
			      page3_inq.release_level[3]);

		if (isalnum(page3_inq.release_level[0]) && isalnum(page3_inq.release_level[1]) &&
		    isalnum(page3_inq.release_level[2]) && isalnum(page3_inq.release_level[3])) {
			sprintf(buffer + len, " (%c%c%c%c)", page3_inq.release_level[0],
				page3_inq.release_level[1], page3_inq.release_level[2],
				page3_inq.release_level[3]);
		}

		body = add_line_to_body(body, _("Firmware Version"), buffer);
	}

	if (strcmp(vendor_id,"IBMAS400") == 0) {
		sprintf(buffer, "%X", std_inq.as400_service_level);
		body = add_line_to_body(body,_("Level"), buffer);
	}

	if (serial_num[0] != '\0')
		body = add_line_to_body(body, _("Serial Number"), serial_num);

	/* Do a read capacity to determine the capacity */
	memset(&read_cap16, 0, sizeof(read_cap16));
	rc = ipr_read_capacity_16(dev, &read_cap16);

	if (!rc && (ntohl(read_cap16.max_user_lba_hi) ||
		    ntohl(read_cap16.max_user_lba_lo)) &&
	    ntohl(read_cap16.block_length)) {

		max_user_lba_int = ntohl(read_cap16.max_user_lba_hi);
		max_user_lba_int <<= 32;
		max_user_lba_int |= ntohl(read_cap16.max_user_lba_lo);

		device_capacity = max_user_lba_int + 1;

		lba_divisor  =
			(1000*1000*1000) / ntohl(read_cap16.block_length);

		sprintf(buffer, "%.2Lf GB", device_capacity / lba_divisor);
		body = add_line_to_body(body, _("Capacity"), buffer);
	}

	if (strlen(dev->dev_name) > 0)
		body = add_line_to_body(body,_("Resource Name"), dev->dev_name);

	rc = ipr_query_resource_state(dev, &res_state);

	if (!rc && !res_state.not_oper && !res_state.not_ready) {
		if (ioa_is_spi(dev->ioa) && !res_state.prot_dev_failed) {
			if (ntohl(res_state.gscsi.data_path_width) == 16)
				body = add_line_to_body(body, _("Wide Enabled"), _("Yes"));
			else
				body = add_line_to_body(body, _("Wide Enabled"), _("No"));

			sprintf(buffer, "%d MB/s",
				(ntohl(res_state.gscsi.data_xfer_rate) *
				 ntohl(res_state.gscsi.data_path_width))/(10 * 8));
			if (res_state.gscsi.data_xfer_rate != 0xffffffff)
				body = add_line_to_body(body, _("Current Bus Throughput"), buffer);
		}
	}

	body = add_line_to_body(body, "", NULL);
	body = add_line_to_body(body, _("Physical location"), NULL);
	body = add_line_to_body(body ,_("PCI Address"), dev->ioa->pci_address);

	if (dev->ioa->sis64) {
		if (dev->scsi_dev_data)
			body = add_line_to_body(body,_("Resource Path"), dev->scsi_dev_data->res_path);
		else {
			ipr_format_res_path(dev->dev_rcd->type3.res_path, dev->res_path_name, IPR_MAX_RES_PATH_LEN);
			body = add_line_to_body(body,_("Resource Path"), dev->res_path_name);
		}
	}

	sprintf(buffer, "%d", dev->ioa->host_num);
	body = add_line_to_body(body, _("SCSI Host Number"), buffer);

	sprintf(buffer, "%d", scsi_channel);
	body = add_line_to_body(body, _("SCSI Channel"), buffer);

	sprintf(buffer, "%d", scsi_id);
	body = add_line_to_body(body, _("SCSI Id"), buffer);

	sprintf(buffer, "%d", scsi_lun);
	body = add_line_to_body(body, _("SCSI Lun"), buffer);

	if (strlen(dev->physical_location))
		body = add_line_to_body(body,_("Platform Location"), dev->physical_location);

	body = disk_extended_details(body, dev, &std_inq);
	return body;
}

/**
 * get_ses_phy_loc - get ses physical location
 * @dev:		ipr dev struct
 *
 * Returns:
 *   body
 **/
int get_ses_phy_loc(struct ipr_dev *dev)
{
	int rc, i, ret = 1;
	struct ses_inquiry_page0  ses_page0_inq;
	struct ses_serial_num_vpd ses_vpd_inq;
	struct esm_serial_num_vpd esm_vpd_inq;
	char buffer[100];

	memset(&ses_vpd_inq, 0, sizeof(ses_vpd_inq));

	rc = ipr_inquiry(dev, 0x00, &ses_page0_inq, sizeof(ses_page0_inq));

	for (i = 0; !rc && i < ses_page0_inq.page_length; i++) {
		if (ses_page0_inq.supported_vpd_page[i] == 0x04) {
			ret = ipr_inquiry(dev, 0x04, &ses_vpd_inq, sizeof(ses_vpd_inq));
			break;
		}
	}

	if (ret == 0 ) {
		dev->physical_location[0] = '\0';
		strncat(dev->physical_location, "U", strlen("U"));
		ipr_strncpy_0(buffer, (char *)ses_vpd_inq.feature_code,
				sizeof(ses_vpd_inq.feature_code));
		strncat(dev->physical_location, buffer, strlen(buffer));
		ipr_strncpy_0(buffer, (char *)ses_vpd_inq.count,
				sizeof(ses_vpd_inq.count));
		strncat(dev->physical_location, ".", strlen("."));
		strncat(dev->physical_location, buffer, strlen(buffer));
		ipr_strncpy_0(buffer, (char *)ses_vpd_inq.ses_serial_num,
				sizeof(ses_vpd_inq.ses_serial_num));
		strncat(dev->physical_location, ".", strlen("."));
		strncat(dev->physical_location, buffer, strlen(buffer));

	}

	if (strlen(dev->physical_location)) {
		for (i = 0; !rc && i < ses_page0_inq.page_length; i++) {
			if (ses_page0_inq.supported_vpd_page[i] == 0x01) {
				ret = ipr_inquiry(dev, 0x01, &esm_vpd_inq, sizeof(esm_vpd_inq));
				break;
			}
		}

		if (ret == 0 ) {
			ipr_strncpy_0((char *)&dev->serial_number, (char *)&esm_vpd_inq.esm_serial_num[0], sizeof(esm_vpd_inq.esm_serial_num));
			ipr_strncpy_0(buffer, (char *)esm_vpd_inq.frb_label,
					sizeof(esm_vpd_inq.frb_label));
			strncat(dev->physical_location, "-", strlen("-"));
			strncat(dev->physical_location, buffer, strlen(buffer));
			return 0;
		}
	}

	return 1;
}

int get_drive_phy_loc_with_ses_phy_loc(struct ipr_dev *ses, struct drive_elem_desc_pg *drive_data, int slot_id, char *buf, int conc_maint)
{
	char buffer[DISK_PHY_LOC_LENGTH + 1], *first_hyphen;
	char unit_phy_loc[PHYSICAL_LOCATION_LENGTH+1];

	ipr_strncpy_0(buffer, (char *)drive_data->dev_elem[slot_id].disk_physical_loc, DISK_PHY_LOC_LENGTH);
	if (strlen(ses->physical_location)) {
		if (strlen(buffer)) {
			ipr_strncpy_0(unit_phy_loc, ses->physical_location, PHYSICAL_LOCATION_LENGTH);
			first_hyphen = strchr(unit_phy_loc, '-');
			if (first_hyphen != NULL)
				*first_hyphen = '\0';
			sprintf(buf, "%s-%s", unit_phy_loc, buffer);
		}
		else
			sprintf(buf, "%s", "\0");
	}
	else if (strlen(buffer)) {
		if (strlen(ses->ioa->physical_location)) {
			ipr_strncpy_0(unit_phy_loc, ses->ioa->physical_location, PHYSICAL_LOCATION_LENGTH);
			first_hyphen = strchr(unit_phy_loc, '-');
			if (first_hyphen != NULL)
				*first_hyphen = '\0';
			sprintf(buf, "%s-%s", unit_phy_loc, buffer);
		} else
			sprintf(buf, "%s", "\0");
	}
	else
		sprintf(buf, "%s", "\0");

	return 0;
}

int index_in_page2(struct ipr_encl_status_ctl_pg *ses_data, int slot_id)
{
	int i;

	for (i = 1; i < IPR_NUM_DRIVE_ELEM_STATUS_ENTRIES; i++) {
		if (ses_data->elem_status[i].slot_id == slot_id)
			return (i - 1);
	}

	return -1;
}
/**
 * ses_details - get ses details
 * @body:		data buffer
 * @dev:		ipr dev struct
 *
 * Returns:
 *   body
 **/
static char *ses_details(char *body, struct ipr_dev *dev)
{
	int rc;
	u8 release_level[4];
	char product_id[IPR_PROD_ID_LEN+1];
	char vendor_id[IPR_VENDOR_ID_LEN+1];
	char buffer[100];
	int len;

	if (strlen(dev->physical_location) == 0)
		get_ses_phy_loc(dev);

	ipr_strncpy_0(vendor_id, dev->scsi_dev_data->vendor_id, IPR_VENDOR_ID_LEN);
	ipr_strncpy_0(product_id, dev->scsi_dev_data->product_id, IPR_PROD_ID_LEN);
	ipr_strncpy_0(buffer, (char *)&dev->serial_number, ESM_SERIAL_NUM_LEN);

	body = add_line_to_body(body,"", NULL);
	body = add_line_to_body(body,_("Manufacturer"), vendor_id);
	body = add_line_to_body(body,_("Product ID"), product_id);
	body = add_line_to_body(body,_("Serial Number"), buffer);

	rc = ipr_get_fw_version(dev, release_level);

	if (!rc) {
		len = sprintf(buffer, "%X%X%X%X", release_level[0], release_level[1],
			      release_level[2], release_level[3]);

		if (isalnum(release_level[0]) && isalnum(release_level[1]) &&
		    isalnum(release_level[2]) && isalnum(release_level[3])) {
			sprintf(buffer + len, " (%c%c%c%c)", release_level[0],
				release_level[1], release_level[2], release_level[3]);
		}

		body = add_line_to_body(body, _("Firmware Version"), buffer);
	}

	if (strlen(dev->gen_name) > 0)
		body = add_line_to_body(body,_("Resource Name"), dev->gen_name);

	body = add_line_to_body(body, "", NULL);
	body = add_line_to_body(body, _("Physical location"), NULL);
	body = add_line_to_body(body ,_("PCI Address"), dev->ioa->pci_address);
	if (dev->ioa->sis64)
		body = add_line_to_body(body,_("Resource Path"), dev->scsi_dev_data->res_path);

	sprintf(buffer, "%d", dev->ioa->host_num);
	body = add_line_to_body(body, _("SCSI Host Number"), buffer);

	sprintf(buffer, "%d", dev->scsi_dev_data->channel);
	body = add_line_to_body(body, _("SCSI Channel"), buffer);

	sprintf(buffer, "%d", dev->scsi_dev_data->id);
	body = add_line_to_body(body, _("SCSI Id"), buffer);

	sprintf(buffer, "%d", dev->scsi_dev_data->lun);
	body = add_line_to_body(body, _("SCSI Lun"), buffer);

	if (strlen(dev->physical_location))
		body = add_line_to_body(body,_("Platform Location"), dev->physical_location);

	return body;
}

/**
 * device_details - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int device_details(i_container *i_con)
{
	char *body = NULL;
	struct ipr_dev *dev;
	struct screen_output *s_out;
	s_node *n_screen;
	int rc;

	processing();

	if ((rc = device_details_get_device(i_con, &dev)))
		return rc;

	if (ipr_is_ioa(dev)) {
		n_screen = &n_adapter_details;
		body = ioa_details(body, dev);
	} else if (ipr_is_volume_set(dev)) {
		n_screen = &n_vset_details;
		body = vset_array_details(body, dev);
	} else if (ipr_is_ses(dev)) {
		n_screen = &n_ses_details;
		body = ses_details(body, dev);
	} else {
		n_screen = &n_device_details;
		body = disk_details(body, dev);
	}

	n_screen->body = body;
	s_out = screen_driver(n_screen, 0, i_con);
	free(n_screen->body);
	n_screen->body = NULL;
	rc = s_out->rc;
	free(s_out);
	return rc;
}

int device_stats(i_container *i_con)
{
	int rc;
	struct ipr_dev *dev;
	char *body = NULL;
	char *tmp;
	s_node *n_screen = &n_device_stats;
	struct screen_output *s_out;

	processing();
	if ((rc = device_details_get_device(i_con, &dev)))
		return rc;

	body = print_ssd_report(dev, body);

	if (ipr_is_af_dasd_device(dev)) {
		body = af_dasd_perf(body, dev);
	}

	if (!body)
		return rc;

	n_screen->body = body;
	s_out = screen_driver(n_screen, 0, i_con);
	free(n_screen->body);
	n_screen->body = NULL;
	rc = s_out->rc;
	free(s_out);
	return rc;
}

int statistics_menu(i_container *i_con)
{
	int rc, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct screen_output *s_out;
	int header_lines;
	char *buffer[2];
	int toggle = 1;
	struct ipr_dev *vset;
	struct ipr_sas_std_inq_data std_inq;

	processing();

	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status(buffer, n_device_stats.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->ioa.scsi_dev_data)
			continue;

		__for_each_standalone_disk(ioa, dev) {
			if (ipr_is_gscsi(dev)) {
				rc = ipr_inquiry(dev, IPR_STD_INQUIRY, &std_inq, sizeof(std_inq));
				if (rc || !std_inq.is_ssd)
					continue;
			}

			print_dev(k, dev, buffer, "%1", 2+k);
			i_con = add_i_con(i_con, "\0", dev);
			num_lines++;
		}

		num_lines += print_hotspare_disks(ioa, &i_con, buffer, 2);

		for_each_vset(ioa, vset) {
			for_each_dev_in_vset(vset, dev) {
				print_dev(k, dev, buffer, "%1", k);
				i_con = add_i_con(i_con, "\0", dev);
				num_lines++;
			}
		}
	}

	if (num_lines == 0) {
		for (k = 0; k < 2; k++) {
			len = strlen(buffer[k]);
			buffer[k] = realloc(buffer[k], len +
						strlen(_(no_dev_found)) + 8);
			sprintf(buffer[k] + len, "\n%s", _(no_dev_found));
		}
	}

	do {
		n_device_stats.body = buffer[toggle&1];
		s_out = screen_driver(&n_device_stats, header_lines,
				      i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_device_stats.body = NULL;

	return rc;
}



#define IPR_INCLUDE 0
#define IPR_REMOVE  1
#define IPR_ADD_HOT_SPARE 0
#define IPR_RMV_HOT_SPARE 1
#define IPR_FMT_JBOD 0
#define IPR_FMT_RECOVERY 1

/**
* hot_spare_screen - Display choices for working with hot spares.
* @i_con:            i_container struct
*
* Returns:
*   0 if success / non-zero on failure
**/
int hot_spare_screen(i_container *i_con)
{
	return display_features_menu(i_con, &n_hot_spare_screen);
}

/**
* raid_screen -
* @i_con:            i_container struct
*
* Returns:
*   0 if success / non-zero on failure
**/
int raid_screen(i_container *i_con)
{
	return display_features_menu(i_con, &n_raid_screen);
}

/**
 * raid_status -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_status(i_container *i_con)
{
	int rc, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;
	char *buffer[2];
	int toggle = 1;

	processing();

	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status(buffer, n_raid_status.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->ioa.scsi_dev_data)
			continue;

		num_lines += print_hotspare_disks(ioa, &i_con, buffer, 2);
		num_lines += print_vsets(ioa, &i_con, buffer, 2);
	}

	if (num_lines == 0) {
		for (k = 0; k < 2; k++) {
			len = strlen(buffer[k]);
			buffer[k] = realloc(buffer[k], len +
						strlen(_(no_dev_found)) + 8);
			sprintf(buffer[k] + len, "%s\n", _(no_dev_found));
		}
	}

	toggle_field = 0;

	do {
		n_raid_status.body = buffer[toggle&1];
		s_out = screen_driver(&n_raid_status, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_status.body = NULL;

	return rc;
}

/**
 * is_array_in_use -
 * @ioa:		ipr ioa struct
 * @array_id:		array ID
 *
 * Returns:
 *   1 if the array is in use / 0 otherwise
 *
 * TODO: This routine no longer works.  It was originally designed to work
 *       with the 2.4 kernels.  For now, the routine will just return 0.
 *       The rest of the code is being left in place in case this kind of
 *       test becomes possible in the future.
 *       JWB - August 11, 2009
 **/
static int is_array_in_use(struct ipr_ioa *ioa,
			   u8 array_id)
{
	int opens;
	struct ipr_dev *vset;

	return 0;

	for_each_vset(ioa, vset) {
		if (array_id == vset->array_id) {
			if (!vset->scsi_dev_data)
				continue;

			opens = num_device_opens(vset->scsi_dev_data->host,
						 vset->scsi_dev_data->channel,
						 vset->scsi_dev_data->id,
						 vset->scsi_dev_data->lun);

			if (opens != 0)
				return 1;
		}
	}

	return 0;
}

/**
 * raid_stop - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_stop(i_container *i_con)
{
	int rc, k;
	int found = 0;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int header_lines;
	int toggle = 1;
	struct screen_output *s_out;

	processing();

	/* empty the linked list that contains field pointens */
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);
	body_init_status(buffer, n_raid_stop.header, &header_lines);

	for_each_ioa(ioa) {
		for_each_vset(ioa, dev) {
			if (!dev->array_rcd->stop_cand)
				continue;
			if ((rc = is_array_in_use(ioa, dev->array_id)))
				continue;

			add_raid_cmd_tail(ioa, dev, dev->array_id);
			i_con = add_i_con(i_con, "\0", raid_cmd_tail);

			print_dev(k, dev, buffer, "%1", 2+k);
			found++;
		}
	}

	if (!found) {
		/* Stop Device Parity Protection Failed */
		n_raid_stop_fail.body = body_init(n_raid_stop_fail.header, NULL);
		s_out = screen_driver(&n_raid_stop_fail, 0, i_con);

		free(n_raid_stop_fail.body);
		n_raid_stop_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		toggle_field = 0;

		do {
			n_raid_stop.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_stop, header_lines, i_con);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_stop.body = NULL;

	return rc;
}

/**
 * confirm_raid_stop - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int confirm_raid_stop(i_container *i_con)
{
	struct ipr_dev *dev;
	struct ipr_array_record *array_record;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *ioa;
	int rc, k;
	int found = 0;
	char *input;    
	char *buffer[2];
	i_container *temp_i_con;
	struct screen_output *s_out;
	int toggle = 1;
	int header_lines;

	found = 0;

	body_init_status(buffer, n_confirm_raid_stop.header, &header_lines);

	for_each_icon(temp_i_con) {
		cur_raid_cmd = (struct array_cmd_data *)(temp_i_con->data);
		if (!cur_raid_cmd)
			continue;

		input = temp_i_con->field_data;
		array_record = cur_raid_cmd->dev->array_rcd;

		if (strcmp(input, "1") == 0) {
			found = 1;
			cur_raid_cmd->do_cmd = 1;
			if (!array_record->issue_cmd)
				array_record->issue_cmd = 1;
		} else {
			cur_raid_cmd->do_cmd = 0;
			if (array_record->issue_cmd)
				array_record->issue_cmd = 0;
		}
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	rc = RC_SUCCESS;

	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;

		ioa = cur_raid_cmd->ioa;
		print_dev(k, cur_raid_cmd->dev, buffer, "1", 2+k);

		for_each_dev_in_vset(cur_raid_cmd->dev, dev)
			print_dev(k, dev, buffer, "1", 2+k);
	}

	toggle_field = 0;

	do {
		n_confirm_raid_stop.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_stop, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_raid_stop.body = NULL;

	return rc;
}

/**
 * raid_stop_complete - 
 *
 * Returns:
 *   21 | EXIT_FLAG if success / 20 | EXIT_FLAG on failure
 **/
static int raid_stop_complete()
{
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int done_bad;
	int not_done = 0;
	int rc;
	struct ipr_ioa *ioa;
	struct array_cmd_data *cur_raid_cmd;

	while(1) {
		done_bad = 0;
		not_done = 0;

		for_each_raid_cmd(cur_raid_cmd) {
			ioa = cur_raid_cmd->ioa;

			rc = ipr_query_command_status(&ioa->ioa, &cmd_status);

			if (rc) {
				done_bad = 1;
				continue;
			}

			for_each_cmd_status(status_record, &cmd_status) {
				if (status_record->command_code == IPR_STOP_ARRAY_PROTECTION &&
				    cur_raid_cmd->array_id == status_record->array_id) {

					if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS)
						not_done = 1;
					else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL)
						/* "Stop Parity Protection failed" */
						done_bad = 1;
					break;
				}
			}
		}

		if (!not_done) {
			if (done_bad)
				/* "Stop Parity Protection failed" */
				return (20 | EXIT_FLAG); 

			check_current_config(false);

			/* "Stop Parity Protection completed successfully" */
			return (21 | EXIT_FLAG);
		}
		not_done = 0;
		sleep(1);
	}
}

/**
 * do_confirm_raid_stop - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   21 | EXIT_FLAG if success / 20 | EXIT_FLAG on failure
 **/
int do_confirm_raid_stop(i_container *i_con)
{
	struct ipr_dev *vset;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *ioa;
	int rc;
	int max_y, max_x;
	struct ipr_res_addr *ra;

	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;

		ioa = cur_raid_cmd->ioa;
		vset = cur_raid_cmd->dev;

		rc = is_array_in_use(ioa, cur_raid_cmd->array_id);
		if (rc != 0)  {
			syslog(LOG_ERR,
			       _("Array %s is currently in use and cannot be deleted.\n"),
			       vset->dev_name);
			return (20 | EXIT_FLAG);
		}

		getmaxyx(stdscr, max_y, max_x);
		move(max_y-1, 0);
		printw(_("Operation in progress - please wait"));
		refresh();

		if (vset->scsi_dev_data) {
			ipr_allow_restart(vset, 0);
			ipr_set_manage_start_stop(vset);
			ipr_start_stop_stop(vset);
			ipr_write_dev_attr(vset, "delete", "1");
		}
		if (vset->alt_path && vset->alt_path->scsi_dev_data) {
			ipr_allow_restart(vset->alt_path, 0);
			ipr_set_manage_start_stop(vset->alt_path);
			ipr_start_stop_stop(vset->alt_path);
			ipr_write_dev_attr(vset->alt_path, "delete", "1");
		}
		ioa->num_raid_cmds++;
	}

	sleep(2);

	for_each_ioa(ioa) {
		if (ioa->num_raid_cmds == 0)
			continue;
		ioa->num_raid_cmds = 0;

		rc = ipr_stop_array_protection(ioa);
		if (rc != 0) {
			if (vset->scsi_dev_data) {
				ra = &vset->res_addr[0];
				ipr_scan(vset->ioa, ra->bus, ra->target, ra->lun);
				ipr_allow_restart(vset, 1);
			}
			if (vset->alt_path && vset->alt_path->scsi_dev_data) {
				ra = &vset->alt_path->res_addr[0];
				ipr_scan(vset->alt_path->ioa, ra->bus, ra->target, ra->lun);
				ipr_allow_restart(vset->alt_path, 1);
			}
			return (20 | EXIT_FLAG);
		}
	}

	rc = raid_stop_complete();
	return rc;
}

/**
 * raid_start - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_start(i_container *i_con)
{
	int rc, k;
	int found = 0;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;
	int toggle = 0;
	u8 array_id;

	processing();

	/* empty the linked list that contains field pointens */
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);
	body_init_status(buffer, n_raid_start.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->start_array_qac_entry)
			continue;

		if (ioa->sis64)
		       array_id = ioa->start_array_qac_entry->type3.array_id;
		else
		       array_id = ioa->start_array_qac_entry->type2.array_id;

		add_raid_cmd_tail(ioa, &ioa->ioa, array_id);

		i_con = add_i_con(i_con,"\0",raid_cmd_tail);

		print_dev(k, &ioa->ioa, buffer, "%1", k);
		found++;
	}

	if (!found) {
		/* Start Device Parity Protection Failed */
		n_raid_start_fail.body = body_init(n_raid_start_fail.header, NULL);
		s_out = screen_driver(&n_raid_start_fail, 0, i_con);

		free(n_raid_start_fail.body);
		n_raid_start_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		do {
			n_raid_start.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_start, header_lines, i_con);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		free_raid_cmds();
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_raid_start.body = NULL;

	return rc;
}

/**
 * raid_start_loop -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_start_loop(i_container *i_con)
{
	int rc;
	struct array_cmd_data *cur_raid_cmd;
	int found = 0;
	char *input;
	i_container *temp_i_con;

	for_each_icon(temp_i_con) {
		cur_raid_cmd = (struct array_cmd_data *)(temp_i_con->data);
		if (!cur_raid_cmd)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			rc = configure_raid_start(temp_i_con);

			if (RC_SUCCESS == rc) {
				found = 1;
				cur_raid_cmd->do_cmd = 1;
			} else
				return rc;
		} else
			cur_raid_cmd->do_cmd = 0;
	}

	if (found != 0) {
		/* Go to verification screen */
		rc = confirm_raid_start(NULL);
		rc |= EXIT_FLAG;
	} else
		rc =  INVALID_OPTION_STATUS; /* "Invalid options specified" */

	return rc;
}

/**
 * configure_raid_start -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int configure_raid_start(i_container *i_con)
{
	char *input;
	int found;
	struct ipr_dev *dev;
	struct array_cmd_data *cur_raid_cmd;
	int rc, k;
	char *buffer[2];
	struct ipr_ioa *ioa;
	i_container *i_con2 = NULL;
	i_container *i_con_head_saved;
	i_container *temp_i_con = NULL;
	i_container *temp_i_con2;
	struct screen_output *s_out;
	int header_lines;
	int toggle = 0;

	rc = RC_SUCCESS;

	cur_raid_cmd = (struct array_cmd_data *)(i_con->data);
	ioa = cur_raid_cmd->ioa;

	body_init_status(buffer, n_configure_raid_start.header, &header_lines);

	i_con_head_saved = i_con_head;
	i_con_head = NULL;

	for_each_af_dasd(ioa, dev) {
		if (!dev->scsi_dev_data)
			continue;

		if (dev->dev_rcd->start_cand && device_supported(dev)) {
			print_dev(k, dev, buffer, "%1", k);
			i_con2 = add_i_con(i_con2, "\0", dev);
		}
	}

	do {
		toggle_field = 0;

		do {
			n_configure_raid_start.body = buffer[toggle&1];
			s_out = screen_driver(&n_configure_raid_start, header_lines, i_con2);
			temp_i_con2 = s_out->i_con;
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		if (rc > 0)
			goto leave;

		i_con2 = temp_i_con2;

		found = 0;

		for_each_icon(temp_i_con) {
			dev = (struct ipr_dev *)temp_i_con->data;
			input = temp_i_con->field_data;

			if (dev && strcmp(input, "1") == 0) {
				found = 1;
				dev->dev_rcd->issue_cmd = 1;
			}
		}

		if (found != 0) {
			/* Go to parameters screen */
			rc = configure_raid_parameters(i_con);

			if ((rc &  EXIT_FLAG) ||
			    !(rc & CANCEL_FLAG))
				goto leave;

			/* User selected Cancel, clear out current selections
			 for redisplay */
			for_each_icon(temp_i_con) {
				dev = (struct ipr_dev *)temp_i_con->data;
				input = temp_i_con->field_data;

				if (dev && (strcmp(input, "1") == 0))
					dev->dev_rcd->issue_cmd = 0;
			}
			rc = REFRESH_FLAG;
			continue;
		} else {

			s_status.index = INVALID_OPTION_STATUS;
			rc = REFRESH_FLAG;
		}
	} while (rc & REFRESH_FLAG);

leave:
	i_con2 = free_i_con(i_con2);

	i_con_head = i_con_head_saved;
	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_configure_raid_start.body = NULL;
	return rc;
}

/**
 * display_input - 
 * @field_start_row:	
 * @userprt:		
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int display_input(int field_start_row, int *userptr)
{
	FIELD *input_fields[3];
	FORM *form;
	WINDOW *box1_win, *field_win;
	int field_rows = 2;
	int field_cols = 16;
	char *input;
	int ch;
	int rc = RC_SUCCESS;

	input_fields[0] = new_field(1, 16, 0, 0, 0, 0);
	input_fields[1] = new_field(1, 3, 1, 0, 0, 0);
	input_fields[2] = NULL;

	set_field_buffer(input_fields[0],0, "Enter new value");
	field_opts_off(input_fields[0], O_ACTIVE);
	field_opts_off(input_fields[0], O_EDIT);
	box1_win = newwin(field_rows + 2, field_cols + 2, field_start_row - 1, 54);
	field_win = newwin(field_rows, field_cols, field_start_row, 55);
	box(box1_win,ACS_VLINE,ACS_HLINE);

	form = new_form(input_fields);
	set_current_field(form, input_fields[1]);

	set_form_win(form,field_win);
	set_form_sub(form,field_win);
	post_form(form);

	form_driver(form, REQ_FIRST_FIELD);
	wnoutrefresh(box1_win);

	while (1) {
		wnoutrefresh(field_win);
		doupdate();
		ch = getch();
		if (IS_ENTER_KEY(ch)) {
			form_driver(form,REQ_VALIDATION);
			input = field_buffer(input_fields[1],0);
			sscanf(input, "%d",userptr);
			break;
		} else if (IS_EXIT_KEY(ch)) {
			rc = EXIT_FLAG;
			break;
		} else if (IS_CANCEL_KEY(ch)) {
			rc = CANCEL_FLAG;
			break;
		} else if (isascii(ch))
			form_driver(form, ch);
	}

	wclear(field_win);
	wrefresh(field_win);
	delwin(field_win);
	wclear(box1_win);
	wrefresh(box1_win);
	delwin(box1_win);

	return rc;
}

/**
 * configure_raid_parameters -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int configure_raid_parameters(i_container *i_con)
{
	FIELD *input_fields[6];
	FIELD *cur_field;
	FORM *form;
	ITEM **raid_item = NULL;
	struct array_cmd_data *cur_raid_cmd = (struct array_cmd_data *)(i_con->data);
	int raid_index_default;
	unsigned long raid_index = 0;
	unsigned long index;
	char buffer[24];
	int rc, i;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct ipr_supported_arrays *supported_arrays;
	struct ipr_array_cap_entry *cap_entry;
	struct text_str
	{
		char line[16];
	} *raid_menu_str = NULL, *stripe_menu_str = NULL;
	char *cache_pols[] = {"Write Through", "Write Back"};
	ITEM *cache_prot_item[ARRAY_SIZE(cache_pols) + 1];
	char raid_str[16], stripe_str[16], qdepth_str[4], *cache_prot_str;
	int ch, start_row;
	int cur_field_index;
	int selected_count = 0, ssd_num = 0, hdd_num = 0;
	int stripe_sz, stripe_sz_mask, stripe_sz_list[16];
	struct prot_level *prot_level_list;
	int *userptr = NULL;
	int *retptr;
	int max_x,max_y,start_y,start_x;
	int qdepth, new_qdepth, max_qdepth;
	int cache_prot;

	getmaxyx(stdscr,max_y,max_x);
	getbegyx(stdscr,start_y,start_x);

	rc = RC_SUCCESS;

	ioa = cur_raid_cmd->ioa;
	supported_arrays = ioa->supported_arrays;
	cap_entry = supported_arrays->entry;

	/* determine number of devices selected for this parity set */
	for_each_af_dasd(ioa, dev) {
		if (dev->dev_rcd->issue_cmd) {
			selected_count++;
			if (dev->block_dev_class & IPR_SSD)
				ssd_num++;
			else
				hdd_num++;
		}
	}

	max_qdepth = ipr_max_queue_depth(ioa, selected_count, ssd_num);
	if (ioa->sis64)
		qdepth = selected_count * 16;
	else
		qdepth = selected_count * 4;
	cur_raid_cmd->qdepth = qdepth;

	/* set up raid lists */
	raid_index = 0;
	raid_index_default = -1;
	prot_level_list = malloc(sizeof(struct prot_level));
	prot_level_list[raid_index].is_valid = 0;

	for_each_cap_entry(cap_entry, supported_arrays) {
		if (selected_count <= cap_entry->max_num_array_devices &&
		    selected_count >= cap_entry->min_num_array_devices &&
		    ((selected_count % cap_entry->min_mult_array_devices) == 0)) {
			if (cap_entry->min_num_per_tier &&
                            (ssd_num < cap_entry->min_num_per_tier ||
                             hdd_num < cap_entry->min_num_per_tier))
                                continue;

			prot_level_list[raid_index].array_cap_entry = cap_entry;
			prot_level_list[raid_index].is_valid = 1;
			if (raid_index_default == -1 &&
			    !strcmp((char *)cap_entry->prot_level_str, "10"))
				raid_index_default = raid_index;
			else if (!strcmp((char *)cap_entry->prot_level_str, IPR_DEFAULT_RAID_LVL))
				raid_index_default = raid_index;

			raid_index++;
			prot_level_list = realloc(prot_level_list, sizeof(struct prot_level) * (raid_index + 1));
			prot_level_list[raid_index].is_valid = 0;
		}
	}

	if (!raid_index)
		return 69 | EXIT_FLAG;

	if (raid_index_default == -1)
		raid_index_default = 0;

	/* Title */
	input_fields[0] = new_field(1, max_x - start_x,   /* new field size */ 
				    0, 0,       /* upper left corner */
				    0,           /* number of offscreen rows */
				    0);               /* number of working buffers */

	/* Protection Level */
	input_fields[1] = new_field(1, 9,   /* new field size */ 
				    8, 44,       /*  */
				    0,           /* number of offscreen rows */
				    0);          /* number of working buffers */

	/* Stripe Size */
	input_fields[2] = new_field(1, 9,   /* new field size */ 
				    9, 44,       /*  */
				    0,           /* number of offscreen rows */
				    0);          /* number of working buffers */

	/* Queue depth */
	input_fields[3] = new_field(1, 3,   /* new field size */ 
				    10, 44,       /*  */
				    0,           /* number of offscreen rows */
				    0);          /* number of working buffers */


	if (ioa->vset_write_cache) {
		/* Sync Cache protection */
		input_fields[4] = new_field(1, 13,  /* new field size */
					    11, 44,     /*  */
					    0,          /* number of offscreen rows */
					    0);         /* number of working buffers */
		cache_prot = IPR_DEV_CACHE_WRITE_BACK;

		set_field_just(input_fields[4], JUSTIFY_LEFT);
		field_opts_off(input_fields[4], O_EDIT);
	} else {
		input_fields [4] = NULL;
	}

	input_fields[5] = NULL;

	raid_index = raid_index_default;
	cap_entry = prot_level_list[raid_index].array_cap_entry;
	stripe_sz = ntohs(cap_entry->recommended_stripe_size);

	form = new_form(input_fields);

	set_current_field(form, input_fields[1]);

	set_field_just(input_fields[0], JUSTIFY_CENTER);
	set_field_just(input_fields[3], JUSTIFY_LEFT);

	field_opts_off(input_fields[0], O_ACTIVE);
	field_opts_off(input_fields[1], O_EDIT);
	field_opts_off(input_fields[2], O_EDIT);
	field_opts_off(input_fields[3], O_EDIT);

	set_field_buffer(input_fields[0],        /* field to alter */
			 0,          /* number of buffer to alter */
			 _("Select Protection Level and Stripe Size"));  /* string value to set     */

	form_opts_off(form, O_BS_OVERLOAD);

	flush_stdscr();

	clear();

	set_form_win(form,stdscr);
	set_form_sub(form,stdscr);
	post_form(form);

	mvaddstr(2, 1, _("Default array configurations are shown.  To change"));
	mvaddstr(3, 1, _("setting hit \"c\" for options menu.  Highlight desired"));
	mvaddstr(4, 1, _("option then hit Enter"));
	mvaddstr(6, 1, _("c=Change Setting"));
	mvaddstr(8, 0,  "Protection Level . . . . . . . . . . . . :");
	mvaddstr(9, 0,  "Stripe Size  . . . . . . . . . . . . . . :");
	mvprintw(10, 0, "Queue Depth (default = %3d)(max = %3d) . :", qdepth, max_qdepth);
	if (ioa->vset_write_cache)
		mvprintw(11, 0, "Array Write Cache Policy . . . . . . . . :");
	mvaddstr(max_y - 4, 0, _("Press Enter to Continue"));
	mvaddstr(max_y - 2, 0, EXIT_KEY_LABEL CANCEL_KEY_LABEL);

	form_driver(form,               /* form to pass input to */
		    REQ_FIRST_FIELD );     /* form request code */

	while (1) {
		sprintf(raid_str,"RAID %s",cap_entry->prot_level_str);
		set_field_buffer(input_fields[1],        /* field to alter */
				 0,                       /* number of buffer to alter */
				 raid_str);              /* string value to set */

		if (stripe_sz > 1024)
			sprintf(stripe_str,"%d M",stripe_sz/1024);
		else
			sprintf(stripe_str,"%d k",stripe_sz);

		set_field_buffer(input_fields[2],     /* field to alter */
				 0,                    /* number of buffer to alter */
				 stripe_str);          /* string value to set */

		sprintf(qdepth_str,"%3d",qdepth);

		set_field_buffer(input_fields[3],     /* field to alter */
				 0,                    /* number of buffer to alter */
				 qdepth_str);          /* string value to set */

		if (ioa->vset_write_cache) {
			cache_prot_str = cache_pols[cache_prot];
			set_field_buffer(input_fields[4],   /* field to alter */
					 0,                 /* number of buffer to alter */
					 cache_prot_str);   /* string value to set */
		}

		refresh();
		ch = getch();

		if (IS_EXIT_KEY(ch)) {
			rc = EXIT_FLAG;
			goto leave;
		} else if (IS_CANCEL_KEY(ch)) {
			rc = CANCEL_FLAG;
			goto leave;
		} else if (ch == 'c') {
			cur_field = current_field(form);
			cur_field_index = field_index(cur_field);

			if (cur_field_index == 1) {
				/* count the number of valid entries */
				index = 0;
				while (prot_level_list[index].is_valid)
					index++;

				/* get appropriate memory, the text portion needs to be
				 done up front as the new_item() function uses the
				 passed pointen to display data */
				raid_item = realloc(raid_item, sizeof(ITEM **) * (index + 1));
				raid_menu_str = realloc(raid_menu_str, sizeof(struct text_str) * (index));
				userptr = realloc(userptr, sizeof(int) * (index + 1));

				/* set up protection level menu */
				index = 0;
				while (prot_level_list[index].is_valid) {
					raid_item[index] = (ITEM *)NULL;
					sprintf(raid_menu_str[index].line,"RAID %s",prot_level_list[index].array_cap_entry->prot_level_str);
					raid_item[index] = new_item(raid_menu_str[index].line,"");
					userptr[index] = index;
					set_item_userptr(raid_item[index],
							 (char *)&userptr[index]);
					index++;
				}

				raid_item[index] = (ITEM *)NULL;
				start_row = 8;
				rc = display_menu(raid_item, start_row, index, &retptr);
				if (rc == RC_SUCCESS) {
					raid_index = *retptr;

					/* find recommended stripe size */
					cap_entry = prot_level_list[raid_index].array_cap_entry;
					stripe_sz = ntohs(cap_entry->recommended_stripe_size);
				}
			} else if (cur_field_index == 2) {
				/* count the number of valid entries */
				index = 0;
				for (i = 0; i < 16; i++) {
					stripe_sz_mask = 1 << i;
					if ((stripe_sz_mask & ntohs(cap_entry->supported_stripe_sizes))
					    == stripe_sz_mask) {
						index++;
					}
				}

				/* get appropriate memory, the text portion needs to be
				 done up front as the new_item() function uses the
				 passed pointen to display data */
				raid_item = realloc(raid_item, sizeof(ITEM **) * (index + 1));
				stripe_menu_str = realloc(stripe_menu_str, sizeof(struct text_str) * (index));
				userptr = realloc(userptr, sizeof(int) * (index + 1));

				/* set up stripe size menu */
				index = 0;
				for (i = 0; i < 16; i++) {
					stripe_sz_mask = 1 << i;
					raid_item[index] = (ITEM *)NULL;
					if ((stripe_sz_mask & ntohs(cap_entry->supported_stripe_sizes))
					    == stripe_sz_mask) {
						stripe_sz_list[index] = stripe_sz_mask;
						if (stripe_sz_mask > 1024)
							sprintf(stripe_menu_str[index].line,"%d M",stripe_sz_mask/1024);
						else
							sprintf(stripe_menu_str[index].line,"%d k",stripe_sz_mask);

						if (stripe_sz_mask == ntohs(cap_entry->recommended_stripe_size)) {
							sprintf(buffer,_("%s - recommend"),stripe_menu_str[index].line);
							raid_item[index] = new_item(buffer, "");
						} else {
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
					stripe_sz = stripe_sz_list[*retptr];
			} else if (cur_field_index == 3) {
				start_row = 10;
				rc = display_input(start_row, &new_qdepth);
				if (rc == EXIT_FLAG)
					goto leave;
				if (rc == CANCEL_FLAG)
					continue;

				qdepth = (new_qdepth < max_qdepth)? new_qdepth:max_qdepth;

				continue;
			} else if (cur_field_index == 4 && ioa->vset_write_cache) {
				userptr = realloc(userptr,
						  (sizeof(int) *
						   (ARRAY_SIZE(cache_pols) + 1)));

				for (index = 0; index < ARRAY_SIZE(cache_pols);
				     index++) {
					cache_prot_item[index] =
						new_item(cache_pols[index], "");
					userptr[index] = index;
					set_item_userptr(cache_prot_item[index],
							 (char *)&userptr[index]);
				}
				cache_prot_item[index] = (ITEM *) NULL;

				start_row = 8;
				rc = display_menu(cache_prot_item, start_row,
						  index, &retptr);
				if (rc == RC_SUCCESS) {
					cache_prot = *retptr;
				}
				for (index = 0; index < ARRAY_SIZE(cache_pols);
				     index++)
					free_item(cache_prot_item[index]);
				if (rc == EXIT_FLAG)
					goto leave;
				continue;
			} else
				continue;

			i = 0;
			while (raid_item[i] != NULL)
				free_item(raid_item[i++]);
			free(raid_item);
			raid_item = NULL;

			if (rc == EXIT_FLAG)
				goto leave;
		} else if (IS_ENTER_KEY(ch)) {
			/* set protection level and stripe size appropriately */
			cur_raid_cmd->prot_level = cap_entry->prot_level;
			cur_raid_cmd->stripe_size = stripe_sz;
			cur_raid_cmd->qdepth = qdepth;
			if (ioa->vset_write_cache)
				cur_raid_cmd->vset_cache = cache_prot;
			goto leave;
		} else if (ch == KEY_RIGHT)
			form_driver(form, REQ_NEXT_CHAR);
		else if (ch == KEY_LEFT)
			form_driver(form, REQ_PREV_CHAR);
		else if ((ch == KEY_BACKSPACE) || (ch == 127))
			form_driver(form, REQ_DEL_PREV);
		else if (ch == KEY_DC)
			form_driver(form, REQ_DEL_CHAR);
		else if (ch == KEY_IC)
			form_driver(form, REQ_INS_MODE);
		else if (ch == KEY_EIC)
			form_driver(form, REQ_OVL_MODE);
		else if (ch == KEY_HOME)
			form_driver(form, REQ_BEG_FIELD);
		else if (ch == KEY_END)
			form_driver(form, REQ_END_FIELD);
		else if (ch == '\t')
			form_driver(form, REQ_NEXT_FIELD);
		else if (ch == KEY_UP)
			form_driver(form, REQ_PREV_FIELD);
		else if (ch == KEY_DOWN)
			form_driver(form, REQ_NEXT_FIELD);
		else if (isascii(ch))
			form_driver(form, ch);
	}
leave:
	unpost_form(form);
	free_form(form);
	free_screen(NULL, NULL, input_fields);

	flush_stdscr();
	return rc;
}

/**
 * confirm_raid_start - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int confirm_raid_start(i_container *i_con)
{
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *ioa;
	int rc, k;
	char *buffer[2];
	struct ipr_dev *dev;
	int header_lines;
	int toggle = 0;
	struct screen_output *s_out;

	rc = RC_SUCCESS;

	body_init_status(buffer, n_confirm_raid_start.header, &header_lines);

	for_each_raid_cmd(cur_raid_cmd) {
		if (cur_raid_cmd->do_cmd) {
			ioa = cur_raid_cmd->ioa;
			print_dev(k, cur_raid_cmd->dev, buffer, "1", k);

			for_each_af_dasd(ioa, dev) {
				if (dev->dev_rcd->issue_cmd)
					print_dev(k, dev, buffer, "1", k);
			}
		}
	}

	toggle_field = 0;

	do {
		n_confirm_raid_start.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_start, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_raid_start.body = NULL;

	if (rc > 0)
		return rc;

	/* verify the devices are not in use */
	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;
		ioa = cur_raid_cmd->ioa;
		rc = is_array_in_use(ioa, cur_raid_cmd->array_id);
		if (rc != 0) {
			/* "Start Parity Protection failed." */
			rc = RC_19_Create_Fail;
			syslog(LOG_ERR, _("Devices may have changed state. Command cancelled,"
					  " please issue commands again if RAID still desired %s.\n"),
			       ioa->ioa.gen_name);
			return rc;
		}
	}

	/* now issue the start array command */
	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;

		ioa = cur_raid_cmd->ioa;
		rc = ipr_start_array_protection(ioa,
						cur_raid_cmd->stripe_size,
						cur_raid_cmd->prot_level);
		if (rc) 
			return RC_19_Create_Fail;
	}

	rc = raid_start_complete();

	return rc;
}

/**
 * raid_start_complete - 
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_start_complete()
{
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int done_bad;
	int not_done = 0;
	int retry = 1;
	int timeout;
	int rc;
	u32 percent_cmplt = 0;
	int device_available = 0;
	int wait_device = 0;
	int exit_now = 0;
	struct ipr_ioa *ioa;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_dev *dev;
	struct ipr_disk_attr attr;

	while (1) {
		rc = complete_screen_driver(&n_raid_start_complete, percent_cmplt, 1);

		if ((rc & EXIT_FLAG) || exit_now) {
			exit_now = 1;

			if (device_available)
				return EXIT_FLAG;
		}

		percent_cmplt = 100;
		done_bad = 0;
		not_done = 0;

		for_each_raid_cmd(cur_raid_cmd) {
			if (cur_raid_cmd->do_cmd == 0)
				continue;

			ioa = cur_raid_cmd->ioa;
			rc = ipr_query_command_status(&ioa->ioa, &cmd_status);

			if (rc)   {
				done_bad = 1;
				continue;
			}

			for_each_cmd_status(status_record, &cmd_status) {
				if ((status_record->command_code != IPR_START_ARRAY_PROTECTION) ||
				    (cur_raid_cmd->array_id != status_record->array_id))
					continue;

				if (!device_available &&
				    (status_record->resource_handle != IPR_IOA_RESOURCE_HANDLE)) {

					timeout = 30;
					while (retry && timeout) {
						check_current_config(false);
						dev = get_dev_from_handle(ioa, status_record->resource_handle);
						if (dev && dev->scsi_dev_data) {
							device_available = 1;
							if (ipr_init_new_dev(dev)) {
								sleep(1);
								timeout--;
								continue;
							} else
								retry = 0;
							if (ipr_get_dev_attr(dev, &attr)) {
								syslog(LOG_ERR,
								       _("Unable to read attributes when creating arrays"));
							} else {
								attr.queue_depth = cur_raid_cmd->qdepth;
								attr.write_cache_policy = cur_raid_cmd->vset_cache;
								if (ipr_set_dev_attr(dev, &attr, 1))
									syslog(LOG_ERR,
									       _("Unable to set attributes when creating arrays"));
							}
						} else
							break;
					}
				}

				if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS) {
					if (status_record->percent_complete < percent_cmplt)
						percent_cmplt = status_record->percent_complete;
					not_done = 1;
				} else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL) {
					done_bad = 1;
					if (status_record->status == IPR_CMD_STATUS_MIXED_BLK_DEV_CLASESS) {
						syslog(LOG_ERR, _("Start parity protect to %s failed.  "
								  "SSDs and HDDs can not be mixed in an array.\n"),
						       ioa->ioa.gen_name);
						rc = RC_22_Mixed_Block_Dev_Classes;
					} else  if (status_record->status == IPR_CMD_STATUS_MIXED_LOG_BLK_SIZE) {
						 syslog(LOG_ERR, _("Start parity protect to %s failed.  "
								  "non 4K disks and 4K disks can not be mixed in an array.\n"),
						       ioa->ioa.gen_name);
						rc = RC_91_Mixed_Logical_Blk_Size;
					} else  if (status_record->status == IPR_CMD_STATUS_UNSUPT_REQ_BLK_DEV_CLASS) {
						 syslog(LOG_ERR, _("Start parity protect to %s failed.  "
								  "These device contained a conmination of block device class filed that was not supported in an array.\n"),
						       ioa->ioa.gen_name);
						rc = RC_92_UNSUPT_REQ_BLK_DEV_CLASS;
					} else {

						syslog(LOG_ERR, _("Start parity protect to %s failed.  "
								  "Check device configuration for proper format.\n"),
						       ioa->ioa.gen_name);
						rc = RC_FAILED;
					}
				} else if (!(device_available)) {
					wait_device++;

					/* if device did not show up after
					  parity set created, allow exit */
					if (wait_device == 20)
						syslog(LOG_ERR, _("Unable to set queue_depth"));
					else
						not_done = 1;
				}
				break;
			}
		}

		if (!not_done) {
			flush_stdscr();

			if (done_bad) {
				if (status_record->status == IPR_CMD_STATUS_MIXED_BLK_DEV_CLASESS)
					return RC_22_Mixed_Block_Dev_Classes;
				if (status_record->status == IPR_CMD_STATUS_MIXED_LOG_BLK_SIZE)
					return  RC_91_Mixed_Logical_Blk_Size;
				if (status_record->status == IPR_CMD_STATUS_UNSUPT_REQ_BLK_DEV_CLASS)
					return RC_92_UNSUPT_REQ_BLK_DEV_CLASS;

				/* Start Parity Protection failed. */
				return RC_19_Create_Fail;
			}

			format_done = 1;
			check_current_config(false);

			/* Start Parity Protection completed successfully */
			return RC_18_Array_Created;
		}
		not_done = 0;
		sleep(1);
	}
}

/**
 * raid_rebuild - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_rebuild(i_container *i_con)
{
	int rc, k;
	int found = 0;
	struct ipr_dev *dev;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;
	int toggle=1;

	processing();

	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(true);
	body_init_status(buffer, n_raid_rebuild.header, &header_lines);

	for_each_ioa(ioa) {
		ioa->num_raid_cmds = 0;

		for_each_af_dasd(ioa, dev) {
			if (!dev->dev_rcd->rebuild_cand)
				continue;

			add_raid_cmd_tail(ioa, dev, 0);
			i_con = add_i_con(i_con,"\0",raid_cmd_tail);
			print_dev(k, dev, buffer, "%1", k);

			enable_af(dev);
			found++;
			ioa->num_raid_cmds++;
		}
	}
 
	if (!found) {
		/* Start Device Parity Protection Failed */
		n_raid_rebuild_fail.body = body_init(n_raid_rebuild_fail.header, NULL);
		s_out = screen_driver(&n_raid_rebuild_fail, 0, NULL);

		free(n_raid_rebuild_fail.body);
		n_raid_rebuild_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		do {
			n_raid_rebuild.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_rebuild, header_lines, i_con);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		free_raid_cmds();
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_rebuild.body = NULL;

	return rc;
}

/**
 * confirm_raid_rebuild -
 * @i_con:		i_container struct
 *
 * Returns:
 *   
 **/
int confirm_raid_rebuild(i_container *i_con)
{
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *ioa;
	int rc;
	char *buffer[2];
	int found = 0;
	char *input;
	int k;
	int header_lines;
	int toggle=1;
	i_container *temp_i_con;
	struct screen_output *s_out;

	for_each_icon(temp_i_con) {
		cur_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
		if (!cur_raid_cmd)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			cur_raid_cmd->do_cmd = 1;
			if (cur_raid_cmd->dev->dev_rcd)
				cur_raid_cmd->dev->dev_rcd->issue_cmd = 1;
			found++;
		} else {
			cur_raid_cmd->do_cmd = 0;
			if (cur_raid_cmd->dev->dev_rcd)
				cur_raid_cmd->dev->dev_rcd->issue_cmd = 0;
		}
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	body_init_status(buffer, n_confirm_raid_rebuild.header, &header_lines);

	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;

		ioa = cur_raid_cmd->ioa;
		print_dev(k, cur_raid_cmd->dev, buffer, "1", k);
	}

	do {
		n_confirm_raid_rebuild.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_rebuild, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_raid_rebuild.body = NULL;

	if (rc)
		return rc;

	for_each_ioa(ioa) {
		if (ioa->num_raid_cmds == 0)
			continue;

		if ((rc = ipr_rebuild_device_data(ioa)))
			/* Rebuild failed */
			return (29 | EXIT_FLAG);
	}

	/* Rebuild started, view Parity Status Window for rebuild progress */
	return (28 | EXIT_FLAG); 
}

/**
 * raid_resync_complete -
 *
 * Returns:
 *   EXIT_FLAG / 74 | EXIT_FLAG on success / 73 | EXIT_FLAG otherwise
 **/
static int raid_resync_complete()
{
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int done_bad;
	int not_done = 0;
	int rc;
	u32 percent_cmplt = 0;
	struct ipr_ioa *ioa;
	struct array_cmd_data *cur_raid_cmd;

	while (1) {
		rc = complete_screen_driver(&n_raid_resync_complete, percent_cmplt, 1);

		if (rc & EXIT_FLAG)
			return EXIT_FLAG;

		percent_cmplt = 100;
		done_bad = 0;
		not_done = 0;

		for_each_raid_cmd(cur_raid_cmd) {
			if (cur_raid_cmd->do_cmd == 0)
				continue;

			ioa = cur_raid_cmd->ioa;
			rc = ipr_query_command_status(&ioa->ioa, &cmd_status);

			if (rc) {
				done_bad = 1;
				continue;
			}

			for_each_cmd_status(status_record, &cmd_status) {
				if ((status_record->command_code != IPR_RESYNC_ARRAY_PROTECTION) ||
				    (cur_raid_cmd->array_id != status_record->array_id))
					continue;

				if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS) {
					if (status_record->percent_complete < percent_cmplt)
						percent_cmplt = status_record->percent_complete;
					not_done = 1;
				} else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL) {
					done_bad = 1;
					syslog(LOG_ERR, _("Resync array to %s failed.\n"),
					       ioa->ioa.gen_name);
					rc = RC_FAILED;
				}
				break;
			}
		}

		if (!not_done) {
			flush_stdscr();

			if (done_bad)
				/* Resync array failed. */
				return (73 | EXIT_FLAG); 

			/* Resync completed successfully */
			return (74 | EXIT_FLAG); 
		}
		not_done = 0;
		sleep(1);
	}
}

/**
 * raid_resync - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_resync(i_container *i_con)
{
	int rc, k;
	int found = 0;
	struct ipr_dev *dev, *array;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;
	int toggle=1;

	processing();

	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(true);
	body_init_status(buffer, n_raid_resync.header, &header_lines);

	for_each_ioa(ioa) {
		ioa->num_raid_cmds = 0;
		for_each_array(ioa, array) {
			if (!array->array_rcd->resync_cand)
				continue;

			dev = array;

			add_raid_cmd_tail(ioa, dev, dev->array_id);
			i_con = add_i_con(i_con, "\0", raid_cmd_tail);
			print_dev(k, dev, buffer, "%1", 2+k);
			found++;
			ioa->num_raid_cmds++;
		}
	}
 
	if (!found) {
		/* Start Device Parity Protection Failed */
		n_raid_resync_fail.body = body_init(n_raid_resync_fail.header, NULL);
		s_out = screen_driver(&n_raid_resync_fail, 0, NULL);

		free(n_raid_resync_fail.body);
		n_raid_resync_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		do {
			n_raid_resync.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_resync, header_lines, i_con);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		free_raid_cmds();
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_resync.body = NULL;

	return rc;
}

/**
 * confirm_raid_resync - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   EXIT_FLAG / 74 | EXIT_FLAG on success / 73 | EXIT_FLAG otherwise
 *   INVALID_OPTION_STATUS if nothing to process
 **/
int confirm_raid_resync(i_container *i_con)
{
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *ioa;
	int rc;
	char *buffer[2];
	int found = 0;
	char *input;
	int k;
	int header_lines;
	int toggle=1;
	i_container *temp_i_con;
	struct screen_output *s_out;

	for_each_icon(temp_i_con) {
		cur_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
		if (!cur_raid_cmd)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			cur_raid_cmd->do_cmd = 1;
			if (cur_raid_cmd->dev->dev_rcd)
				cur_raid_cmd->dev->dev_rcd->issue_cmd = 1;
			found++;
		} else {
			cur_raid_cmd->do_cmd = 0;
			if (cur_raid_cmd->dev->dev_rcd)
				cur_raid_cmd->dev->dev_rcd->issue_cmd = 0;
		}
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	body_init_status(buffer, n_confirm_raid_resync.header, &header_lines);

	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;

		ioa = cur_raid_cmd->ioa;
		print_dev(k, cur_raid_cmd->dev, buffer, "1", k);
	}

	do {
		n_confirm_raid_resync.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_resync, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_raid_resync.body = NULL;

	if (rc)
		return rc;

	for_each_ioa(ioa) {
		if (ioa->num_raid_cmds == 0)
			continue;

		if ((rc = ipr_resync_array(ioa)))
			return (73 | EXIT_FLAG);
	}

	return raid_resync_complete();
}

/**
 * do_raid_migrate - Process the command line Migrate Array command.  Check the
 * 		     arguments against the qac data to make sure that the
 * 		     migration is possible.  Then issue the migrate command.
 * @ioa:		struct ipr_ioa
 * @qac_data:		query array config data
 * @vset:		struct ipr_dev
 * @raid_level:		new raid level
 * @stripe_size:	new stripe size
 * @num_devs:		number of devices to add
 * @fd:			file descriptor
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int do_raid_migrate(struct ipr_ioa *ioa, struct ipr_array_query_data *qac_data,
		    struct ipr_dev *vset, char *raid_level, int stripe_size,
		    int num_devs, int fd)
{
	int found = 0, rc = 0;
	int enc_raid_level;
	struct ipr_dev *dev = NULL;
	struct sysfs_dev *sdev;
	struct ipr_array_cap_entry *cap;
	struct ipr_array_record *array_rcd;
	struct ipr_dev_record *dev_rcd;
	struct ipr_supported_arrays *supported_arrays = NULL;

	/* check that the given raid level is valid */
	for_each_supported_arrays_rcd(supported_arrays, qac_data) {
		cap = get_cap_entry(supported_arrays, raid_level);
		if (cap)
			break;
	}

	if (!cap) {
		fprintf(stderr, "RAID level %s is unsupported.\n", raid_level);
		return -EINVAL;
	}
	enc_raid_level = cap->prot_level;

	if (ioa->sis64)
		vset = get_array_from_vset(ioa, vset);

	/* loop through returned records - looking at array records */
	for_each_array_rcd(array_rcd, qac_data) {
		if (vset->resource_handle == ipr_get_arr_res_handle(ioa, array_rcd)) {
			if (array_rcd->migrate_cand) {
				array_rcd->issue_cmd = 1;
				found = 1;
			}
			break;
		}
	}

	/* device is valid, but array migration is not supported */
	if (!found) {
		fprintf(stderr, "Migrate array protection is not currently ");
		fprintf(stderr, "possible on the %s device.\n", vset->gen_name);
		return -EINVAL;
	}

	/* process stripe size */
	if (stripe_size) {
		/* is stripe size supported? */
		if ((stripe_size & ntohs(cap->supported_stripe_sizes)) != stripe_size) {
			fprintf(stderr, "Unsupported stripe size for ");
			fprintf(stderr, "selected adapter. %d\n", stripe_size);
			return -EINVAL;
		}
	} else
		/* get current stripe size */
		if (ioa->sis64)
			stripe_size = ntohs(array_rcd->type3.stripe_size);
		else
			stripe_size = ntohs(array_rcd->type2.stripe_size);

	/* if adding devices, check that there are a valid number of them */
	if (cap->format_overlay_type == IPR_FORMAT_ADD_DEVICES) {
 		rc = raid_create_check_num_devs(cap, num_devs, 0);

		if (rc != 0) {
			fprintf(stderr,"Incorrect number of devices given.\n");
			fprintf(stderr,"Minimum number of devices = %d\n",
				       	cap->min_num_array_devices);
			fprintf(stderr,"Maximum number of devices = %d\n",
				       	cap->max_num_array_devices);
			return rc;
		}
	}

	/* if adding devs, verify them and set issue_cmd = 1 */
	for_each_dev_rcd(dev_rcd, qac_data) {
		if (num_devs == 0)
			break;

		/* check the given devices for migrate candidate status */
		for (sdev = head_sdev; sdev; sdev = sdev->next) {
			dev = ipr_sysfs_dev_to_dev(sdev);
			if (ipr_get_dev_res_handle(ioa, dev_rcd) == dev->resource_handle) {
				if (dev_rcd->migrate_array_prot_cand)
					dev_rcd->issue_cmd = 1;
				else {
					fprintf(stderr, "%s is not available "
					        "for migrate array "
						"protection\n", dev->gen_name);
					return -EINVAL;
				}
			}
		}
	}

	/* send command to adapter */
	rc = ipr_migrate_array_protection(ioa, qac_data, fd, stripe_size, enc_raid_level);

	return rc;
}

/**
 * raid_migrate_cmd - Command line handler for Migrate Array command.  Process
 * 		      the command arguments and prepare for do_raid_migrate().
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int raid_migrate_cmd(char **args, int num_args)
{
	int i, fd, rc = 0;
	int next_raid_level = 0, raid_level_set = 0;
	int next_stripe_size = 0;
	int stripe_size = 0;
	int num_devs = 0;
	char *raid_level = IPR_DEFAULT_RAID_LVL;
	struct ipr_dev *dev = NULL, *vset = NULL;
	struct ipr_ioa *ioa = NULL;
	struct ipr_array_query_data qac_data;

	memset(&qac_data, 0, sizeof(qac_data));

	for (i = 0; i < num_args; i++) {
		if (strcmp(args[i], "-r") == 0)
			next_raid_level = 1;
		else if (strcmp(args[i], "-s") == 0)
			next_stripe_size = 1;
		else if (next_raid_level) {
			next_raid_level = 0;
			raid_level_set = 1;
			raid_level = args[i];
		} else if (next_stripe_size) {
			next_stripe_size = 0;
			stripe_size = strtoul(args[i], NULL, 10);
		} else {
			dev = find_dev(args[i]);
			if (!dev) {
				fprintf(stderr, "Cannot find %s\n", args[i]);
				return -EINVAL;
			}

			if (ipr_is_volume_set(dev)) {
				if (vset) {
					fprintf(stderr, "Invalid parameters. "
						"Only one disk "
						"array can be specified.\n");
					return -EINVAL;
				}

				vset = dev;
				ioa = vset->ioa;
				continue;
			}

			if (!ipr_is_af_dasd_device(dev)) {
				fprintf(stderr, "Invalid device specified. "
				        "Device must be formatted "
					"to Advanced Function Format.\n");
				return -EINVAL;
			} else {
				/* keep track of these */
				num_devs++;
				ipr_add_sysfs_dev(dev, &head_sdev, &tail_sdev);
			}
		}
	}

	if (vset == NULL) {
		fprintf(stderr, "No RAID device specified\n");
		return -EINVAL;
	}
	if (!raid_level_set) {
		fprintf(stderr, "No RAID level specified\n");
		return -EINVAL;
	}

	/* lock the ioa device */
	fd = open_and_lock(ioa->ioa.gen_name);
	if (fd <= 1)
		return -EIO;

	/* query array configuration for volume set migrate status */
	rc = __ipr_query_array_config(ioa, fd, 0, 0, 1, vset->array_id, &qac_data);

	if (rc != 0) {
		fprintf(stderr, "Array query failed.  rc = %d\n", rc);
		close(fd);
		return -EINVAL;
	}

	/* Check arguments against the qac data and issue the migrate command */
	rc = do_raid_migrate(ioa, &qac_data, vset, raid_level, stripe_size, num_devs, fd);
	close(fd);

	return rc;
}

/**
 * raid_migrate_complete - Display a status screen showing the command
 *                         completion percentage.
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_migrate_complete()
{
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int done_bad;
	int not_done = 0;
	int rc;
	u32 percent_cmplt = 0;
	int device_available = 0;
	int exit_now = 0;
	struct ipr_ioa *ioa;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_dev *dev;

	ENTER;
	while (1) {
		rc = complete_screen_driver(&n_raid_migrate_complete, percent_cmplt, 1);

		if (rc & EXIT_FLAG || exit_now) {
			exit_now = 1;

			if (device_available)
				return EXIT_FLAG;
		}

		percent_cmplt = 100;
		done_bad = 0;
		not_done = 0;

		for_each_raid_cmd(cur_raid_cmd) {
			if (cur_raid_cmd->do_cmd == 0)
				continue;

			ioa = cur_raid_cmd->ioa;
			rc = ipr_query_command_status(&ioa->ioa, &cmd_status);

			if (rc)   {
				done_bad = 1;
				continue;
			}

			for_each_cmd_status(status_record, &cmd_status) {
				if ((status_record->command_code != IPR_MIGRATE_ARRAY_PROTECTION) ||
				    (cur_raid_cmd->array_id != status_record->array_id))
					continue;

				if (!device_available &&
				    (status_record->resource_handle != IPR_IOA_RESOURCE_HANDLE)) {

					check_current_config(false);
					dev = get_dev_from_handle(ioa, status_record->resource_handle);
					if (dev && dev->scsi_dev_data)
						device_available = 1;
				}

				if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS) {
					if (status_record->percent_complete < percent_cmplt)
						percent_cmplt = status_record->percent_complete;
					not_done = 1;
				} else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL) {
					done_bad = 1;
					ioa_err(ioa, "Migrate array protection failed.\n");
					rc = RC_FAILED;
				} else if (!device_available)
					not_done = 1;

				break;
			}
		}

		if (!not_done) {
			flush_stdscr();

			if (done_bad)
				/* Migrate Array Protection failed. */
				return RC_80_Migrate_Prot_Fail;

			check_current_config(false);

			/* Migrate Array Protection completed successfully */
			LEAVE;
			return RC_79_Migrate_Prot_Success;
		}
		not_done = 0;
		sleep(1);
	}
}

/**
 * confirm_raid_migrate - Confirm that the migration choices are correct.
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int confirm_raid_migrate()
{
	struct array_cmd_data *cur_raid_cmd;
	int rc, k;
	char *buffer[2];
	struct screen_output *s_out;
	int toggle = 1;
	int header_lines;

	ENTER;
	body_init_status(buffer, n_confirm_raid_migrate.header, &header_lines);

	/* Display the array being migrated.  Also display any disks if
	   they are required for the migration.  */
	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;

		print_dev(k, cur_raid_cmd->dev, buffer, "1", 2+k);
	}

	rc = RC_SUCCESS;
	toggle_field = 0;

	do {
		n_confirm_raid_migrate.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_migrate, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_raid_migrate.body = NULL;

	LEAVE;
	return rc;
}

/**
 * choose_migrate_disks - Choose the additional disks required for migration.
 * @ioa:
 * @qac:		i_query_array_data struct
 * @cap_entry:
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int choose_migrate_disks(struct ipr_ioa *ioa, struct ipr_array_query_data *qac,
			 struct ipr_array_cap_entry *cap_entry)
{
	struct array_cmd_data *tmp_raid_cmd;
	struct ipr_dev_record *dev_rcd;
	struct ipr_dev *dev;
	i_container *dev_i_con, *temp_i_con;
	int rc, min, max, mult;
	int k = 0;
	char *buffer[2];
	char info[256];
	int header_lines = 0;
	struct screen_output *s_out;
	int toggle = 0;
	int found = 0;
	char *input;

	ENTER;
	min = cap_entry->min_num_array_devices;
	max = cap_entry->max_num_array_devices;
	mult = cap_entry->min_mult_array_devices;

	sprintf(info, _("A minimum of %d disks must be selected.\n"), min);
	buffer[0] = add_string_to_body(NULL, info, "", &header_lines);
	buffer[1] = add_string_to_body(NULL, info, "", &header_lines);

	sprintf(info, _("A maximum of %d disks must be selected.\n"), max);
	buffer[0] = add_string_to_body(buffer[0], info, "", &header_lines);
	buffer[1] = add_string_to_body(buffer[1], info, "", &header_lines);

	sprintf(info, _("The number of disks selected must be a multiple of %d.\n\n"), mult);
	buffer[0] = add_string_to_body(buffer[0], info, "", &header_lines);
	buffer[1] = add_string_to_body(buffer[1], info, "", &header_lines);

	buffer[0] = status_header(buffer[0], &header_lines, 0);
	buffer[1] = status_header(buffer[1], &header_lines, 1);

	/* Ensure i_con_head gets set to the head of the
	   disk candidate list in add_i_con() */
	dev_i_con = NULL;

	for_each_dev_rcd(dev_rcd, qac) {
		if (!dev_rcd->migrate_array_prot_cand)
			continue;

		dev = get_dev_from_handle(ioa, ipr_get_dev_res_handle(ioa, dev_rcd));
		if (dev) {
			print_dev(k, dev, buffer, "%1", 2);

			add_raid_cmd_tail(NULL, dev, 0);

			/* Add disk candidates to the i_con list. */
			dev_i_con = add_i_con(dev_i_con, "\0", raid_cmd_tail);
		}
	}

	do {
		toggle_field = 0;

		do {
			n_raid_migrate_add_disks.body = buffer[toggle&1];
			/* select disks to use */
			s_out = screen_driver(&n_raid_migrate_add_disks, header_lines, NULL);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		if (rc)
			break;

		found = 0;

		/* Loop through, count the selected devices and
		 * set the do_cmd flag. */
		for_each_icon(temp_i_con) {
			tmp_raid_cmd = temp_i_con->data;
			input = temp_i_con->field_data;

			if (strcmp(input, "1") == 0) {
				found++;
				tmp_raid_cmd->do_cmd = 1;
			}
		}

		if (found) {
			rc = raid_create_check_num_devs(cap_entry, found, 1);
			if (rc == 0)
				break;
			if (rc == RC_77_Too_Many_Disks) {
				s_status.num = cap_entry->max_num_array_devices;
				s_status.index = RC_77_Too_Many_Disks;
			}
			if (rc == RC_78_Too_Few_Disks) {
				s_status.num = cap_entry->min_num_array_devices;
				s_status.index = RC_78_Too_Few_Disks;
			}
			if (rc == RC_25_Devices_Multiple) {
				s_status.num = cap_entry->min_mult_array_devices;
				s_status.index = RC_25_Devices_Multiple;
			}
			/* clear the do_cmd flag */
			for_each_icon(temp_i_con) {
				tmp_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
				input = temp_i_con->field_data;

				if (tmp_raid_cmd && strcmp(input, "1") == 0)
					tmp_raid_cmd->do_cmd = 0;
			}
		}

		rc = REFRESH_FLAG;

	} while (rc & REFRESH_FLAG);

	LEAVE;
	return rc;
}

/**
 * do_ipr_migrate_array_protection - Migrate array protection for an array
 * @array_i_con:	i_container for selected array
 * @ioa:                ipr ioa struct
 * @qac_data:           struct ipr_array_query_data
 * @stripe_size:        new or existing stripe size for array
 * @prot_level:         new protection (RAID) level for array
 * @array_id:           array id
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int do_ipr_migrate_array_protection(i_container *array_i_con, struct ipr_ioa *ioa,
				    struct ipr_array_query_data *qac,
				    int stripe_size, int prot_level, u8 array_id)
{
	int fd, rc;
	i_container *temp_i_con;
	struct array_cmd_data *acd;
	struct ipr_array_record *array_rcd;
	struct ipr_dev_record *dev_rcd;
	struct ipr_array_query_data new_qac;
	char *input;

	ENTER;
	/* Now that we have the selected array, lock the ioa
	 * device, then issue another query to make sure that
	 * the adapter's version of the QAC data matches what
	 * we are processing. */
	fd = open_and_lock(ioa->ioa.gen_name);
	if (fd <= 1)
		return fd;

	rc = __ipr_query_array_config(ioa, fd, 0, 0, 1, array_id, &new_qac);
	if (rc) {
		close(fd);
		return rc;
	}

	/* Set the "do it" bit for the select disks if needed. */
	for_each_icon(temp_i_con) {
		acd = temp_i_con->data;
		input = temp_i_con->field_data;

		if (acd && strcmp(input, "1") == 0)
			for_each_dev_rcd(dev_rcd, &new_qac)
				if (acd->dev->resource_handle == ipr_get_dev_res_handle(ioa, dev_rcd))
					dev_rcd->issue_cmd = 1;
	}

	/* get the array command data and associated data from the data field
	   of the passed in array info container. */
	acd = array_i_con->data;

	/* Set the "do it" bit for the selected array. */
	for_each_array_rcd(array_rcd, &new_qac)
		if (acd->dev->resource_handle == ipr_get_arr_res_handle(ioa, array_rcd))
			array_rcd->issue_cmd = 1;

	rc = ipr_migrate_array_protection(ioa, &new_qac, fd, stripe_size, prot_level);

	close(fd);
	LEAVE;
	return rc;
}

/**
 * configure_raid_migrate - The array to migrate has been selected, now get the
 * 			    new RAID level.
 * @array_i_con:	i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int configure_raid_migrate(i_container *array_i_con)
{
	FIELD *input_fields[3];
	FIELD *cur_field;
	FORM *form;
	ITEM **raid_item = NULL;
	struct array_cmd_data *cur_raid_cmd;
	int raid_index_default = 0;
	unsigned long raid_index = 0;
	unsigned long index;
	int rc, i;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct ipr_array_query_data qac;
	struct ipr_supported_arrays *supported_arrays;
	struct ipr_array_cap_entry *cap_entry;
	struct text_str
	{
		char line[16];
	} *raid_menu_str = NULL;
	char raid_str[16];
	char dev_str[60];
	int ch, start_row;
	int cur_field_index;
	struct prot_level *prot_level_list;
	int *userptr = NULL;
	int *retptr;
	int max_x,max_y,start_y,start_x;
	i_container *saved_i_con_head;

	ENTER;
	getmaxyx(stdscr,max_y,max_x);
	getbegyx(stdscr,start_y,start_x);

	/* get the array command data and associated data from the data field
	   of the passed in array info container. */
	cur_raid_cmd = array_i_con->data;
	ioa = cur_raid_cmd->ioa;
	dev = cur_raid_cmd->dev;

	/* Get the QAC data for the chosen array. */
	rc = ipr_query_array_config(ioa, 0, 0, 1, cur_raid_cmd->array_id, &qac);
	if (rc)
		return RC_80_Migrate_Prot_Fail | EXIT_FLAG;

	/* allows cancelling out if the user doesn't change the existing
	   protection level. */
	rc = CANCEL_FLAG;

	/* Save the current i_con_head, as it points to the list of array
	   candidates.  We need to restore this before leaving this routine. */
	saved_i_con_head = i_con_head;
	i_con_head = NULL;

	/* set up raid lists */
	prot_level_list = malloc(sizeof(struct prot_level));
	prot_level_list[raid_index].is_valid = 0;

	for_each_supported_arrays_rcd(supported_arrays, &qac) {
		for_each_cap_entry(cap_entry, supported_arrays) {

			prot_level_list[raid_index].array_cap_entry = cap_entry;
			prot_level_list[raid_index].is_valid = 1;

			if (!strcmp((char *)cap_entry->prot_level_str, dev->prot_level_str))
				raid_index_default = raid_index;

			raid_index++;
			prot_level_list = realloc(prot_level_list, sizeof(struct prot_level) * (raid_index + 1));
			prot_level_list[raid_index].is_valid = 0;
		}
	}

	if (!raid_index) {
		/* We should never get here. */
		free(prot_level_list);
		scsi_err(dev, "The query data for the array indicate "
			 "that there are no supported RAID levels.\n");
		return RC_80_Migrate_Prot_Fail | EXIT_FLAG;
	}

	/* Title */
	input_fields[0] = new_field(1, max_x - start_x,   /* new field size */
				    0, 0,                 /* upper left corner */
				    0,                    /* number of offscreen rows */
				    0);                   /* number of working buffers */

	/* Protection Level */
	input_fields[1] = new_field(1, 9,        /* new field size */
				    8, 44,       /*  */
				    0,           /* number of offscreen rows */
				    0);          /* number of working buffers */

	input_fields[2] = NULL;

	raid_index = raid_index_default;
	cap_entry = prot_level_list[raid_index].array_cap_entry;

	form = new_form(input_fields);

	set_current_field(form, input_fields[1]);

	set_field_just(input_fields[0], JUSTIFY_CENTER);

	field_opts_off(input_fields[0], O_ACTIVE);
	field_opts_off(input_fields[1], O_EDIT);

	set_field_buffer(input_fields[0],	/* field to alter */
			 0,			/* number of buffer to alter */
			 _("Select Protection Level"));  /* string value to set */

	form_opts_off(form, O_BS_OVERLOAD);

	flush_stdscr();

	clear();

	set_form_win(form,stdscr);
	set_form_sub(form,stdscr);
	post_form(form);

	mvaddstr(2, 1, _("Current RAID protection level is shown.  To change"));
	mvaddstr(3, 1, _("setting hit \"c\" for options menu.  Highlight desired"));
	mvaddstr(4, 1, _("option then hit Enter"));
	mvaddstr(6, 1, _("c=Change Setting"));
	sprintf(dev_str, "%s - Protection Level . . . . . . :", dev->dev_name);
	mvaddstr(8, 0,  dev_str);
	mvaddstr(max_y - 4, 0, _("Press Enter to Continue"));
	mvaddstr(max_y - 2, 0, EXIT_KEY_LABEL CANCEL_KEY_LABEL);

	form_driver(form,			/* form to pass input to */
		    REQ_FIRST_FIELD );		/* form request code */

	sprintf(raid_str, "RAID %s", dev->prot_level_str);

	while (1) {
		set_field_buffer(input_fields[1],      /* field to alter */
				 0,                    /* number of buffer to alter */
				 raid_str);            /* string value to set */

		refresh();
		ch = getch();

		if (IS_EXIT_KEY(ch)) {
			rc = EXIT_FLAG;
			break;
		} else if (IS_CANCEL_KEY(ch)) {
			rc = CANCEL_FLAG;
			break;
		} else if (ch == 'c') {
			cur_field = current_field(form);
			cur_field_index = field_index(cur_field);

			if (cur_field_index == 1) {
				/* count the number of valid entries */
				index = 0;
				while (prot_level_list[index].is_valid)
					index++;

				/* get appropriate memory, the text portion
				   needs to be done up front as the new_item()
				   function uses the passed pointen to display data */
				raid_item = realloc(raid_item, sizeof(ITEM **) * (index + 1));
				raid_menu_str = realloc(raid_menu_str, sizeof(struct text_str) * (index));
				userptr = realloc(userptr, sizeof(int) * (index + 1));

				/* set up protection level menu */
				index = 0;
				while (prot_level_list[index].is_valid) {
					raid_item[index] = NULL;
					cap_entry = prot_level_list[index].array_cap_entry;
					sprintf(raid_menu_str[index].line,
						"RAID %s", cap_entry->prot_level_str);
					raid_item[index] = new_item(raid_menu_str[index].line,"");
					userptr[index] = index;
					set_item_userptr(raid_item[index],
							 (char *)&userptr[index]);
					index++;
				}

				raid_item[index] = NULL;
				start_row = 8;
				/* select the desired new protection level */
				rc = display_menu(raid_item, start_row, index, &retptr);
				if (rc == RC_SUCCESS) {
					raid_index = *retptr;
					cap_entry = prot_level_list[raid_index].array_cap_entry;
					sprintf(raid_str, "RAID %s", cap_entry->prot_level_str);

				}

				/* clean up */
				for (i=0; raid_item[i] != NULL; i++)
					free_item(raid_item[i]);

				free(raid_item);
				raid_item = NULL;
			} else
				continue;
		} else if (IS_ENTER_KEY(ch)) {
			/* don't change anything if Cancel or Exit */
			if (rc != RC_SUCCESS)
				break;

			/* set protection level */
			cur_raid_cmd->prot_level = cap_entry->prot_level;

			/* are additional devices needed? */
			if (cap_entry->format_overlay_type == IPR_FORMAT_ADD_DEVICES) {
				rc = choose_migrate_disks(ioa, &qac, cap_entry);
				if (rc)
					break;

			} else if (cap_entry->format_overlay_type != IPR_FORMAT_REMOVE_DEVICES) {
				/* We should never get here. */
				scsi_err(dev, "The capability entry for the array "
					 "does not have a valid overlay type.\n");
				return RC_80_Migrate_Prot_Fail | EXIT_FLAG;
			}

			rc = confirm_raid_migrate();

			if (rc)
				break;

			/* set up qac and send command to adapter */
			rc = do_ipr_migrate_array_protection(array_i_con, ioa, &qac,
				       			     ntohs(dev->stripe_size),
							     cur_raid_cmd->prot_level,
							     cur_raid_cmd->array_id);

			break;

		} else if (ch == KEY_RIGHT)
			form_driver(form, REQ_NEXT_CHAR);
		else if (ch == KEY_LEFT)
			form_driver(form, REQ_PREV_CHAR);
		else if ((ch == KEY_BACKSPACE) || (ch == 127))
			form_driver(form, REQ_DEL_PREV);
		else if (ch == KEY_DC)
			form_driver(form, REQ_DEL_CHAR);
		else if (ch == KEY_IC)
			form_driver(form, REQ_INS_MODE);
		else if (ch == KEY_EIC)
			form_driver(form, REQ_OVL_MODE);
		else if (ch == KEY_HOME)
			form_driver(form, REQ_BEG_FIELD);
		else if (ch == KEY_END)
			form_driver(form, REQ_END_FIELD);
		else if (ch == '\t')
			form_driver(form, REQ_NEXT_FIELD);
		else if (ch == KEY_UP)
			form_driver(form, REQ_PREV_FIELD);
		else if (ch == KEY_DOWN)
			form_driver(form, REQ_NEXT_FIELD);
		else if (isascii(ch))
			form_driver(form, ch);
	}

	/* restore i_con_head */
	i_con_head = saved_i_con_head;

	free(prot_level_list);
	free(raid_menu_str);
	free(userptr);

	unpost_form(form);
	free_form(form);
	free_screen(NULL, NULL, input_fields);

	flush_stdscr();
	LEAVE;
	return rc;
}

/**
 * process_raid_migrate - Process the list of arrays that can be migrated.
 * @buffer:
 * @header_lines:
 * @last_array_cmd:
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int process_raid_migrate(char *buffer[], int header_lines)
{
	struct array_cmd_data *acd, *saved_cmd_head;
	struct array_cmd_data *last_array_cmd;
	struct screen_output *s_out;
	int found = 0;
	int toggle = 1;
	i_container *temp_i_con;
	char *input;
	int rc = REFRESH_FLAG;

	ENTER;
	while (rc & REFRESH_FLAG) {
		toggle_field = 0;

		do {
			n_raid_migrate.body = buffer[toggle&1];
			/* display disk array selection screen */
			s_out = screen_driver(&n_raid_migrate, header_lines, NULL);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		if (rc & EXIT_FLAG || rc & CANCEL_FLAG)
			break;

		found = 0;
		/* Only one migration is allowed at a time.  Set the do_cmd
		 * flag for the first one that is found. */
		for_each_icon(temp_i_con) {
			acd = temp_i_con->data;
			input = temp_i_con->field_data;

			if (acd && strcmp(input, "1") == 0) {
				found++;
				acd->do_cmd = 1;
				break;
			}
		}

		if (found) {
			last_array_cmd = raid_cmd_tail;

			/* Go to protection level selection screen */
			rc = configure_raid_migrate(temp_i_con);

			/* Keep the array information in the list, but
			 * strip off any disk inforamtion. */
			saved_cmd_head = raid_cmd_head;
			raid_cmd_head = last_array_cmd->next;
			free_raid_cmds();
			raid_cmd_head = saved_cmd_head;
			raid_cmd_tail = last_array_cmd;
			last_array_cmd->next = NULL;

			/* Done on success or exit, otherwise refresh */
			if (rc == RC_SUCCESS || rc & EXIT_FLAG)
				break;

			/* clear the do_cmd flag */
			acd->do_cmd = 0;

			rc = REFRESH_FLAG;
		} else {
			s_status.index = INVALID_OPTION_STATUS;
			rc = REFRESH_FLAG;
		}
		toggle++;
	}
	LEAVE;
	return rc;
}

/**
 * raid_migrate - GUI routine for migrate raid array protection.
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_migrate(i_container *i_con)
{
	int rc, k;
	int found = 0;
	struct ipr_dev *array, *dev;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;

	ENTER;
	processing();

	/* make sure the i_con list is empty */
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);

	body_init_status(buffer, n_raid_migrate.header, &header_lines);

	for_each_primary_ioa(ioa) {
		for_each_array(ioa, array) {
			dev = array;
			if (!dev->array_rcd->migrate_cand)
				continue;

			if (ioa->sis64)
				dev = get_vset_from_array(ioa, dev);

			add_raid_cmd_tail(ioa, dev, dev->array_id);
			i_con = add_i_con(i_con, "\0", raid_cmd_tail);

			print_dev(k, dev, buffer, "%1", 2+k);
			found++;
		}
	}

	if (!found) {
		n_raid_migrate_fail.body = body_init(n_raid_migrate_fail.header, NULL);
		s_out = screen_driver(&n_raid_migrate_fail, 0, i_con);

		free(n_raid_migrate_fail.body);
		n_raid_migrate_fail.body = NULL;

		rc = s_out->rc | CANCEL_FLAG;
		free(s_out);
	} else
		rc = process_raid_migrate(buffer, header_lines);

	/* Display a progress screen. */
	if (rc == 0)
		return raid_migrate_complete();

	/* Free the remaining raid commands. */
	free_raid_cmds();

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_migrate.body = NULL;

	LEAVE;
	return rc;
}

/**
 * asym_access_menu - Display the Optimized and Non-Optimized choices.
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int asym_access_menu(i_container *i_con)
{
	struct array_cmd_data *acd;
	int start_row;
	int num_menu_items;
	int menu_index = 0;
	ITEM ** menu_item = NULL;
	int *userptr = NULL;
	int *new_state;
	int rc;

	acd = (struct array_cmd_data *)i_con->data;

	start_row = i_con->y + 2;

	num_menu_items = 3;

	menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
	userptr = malloc(sizeof(int) * num_menu_items);

	menu_item[menu_index] = new_item("Optimized","");
	userptr[menu_index] = 0;
	set_item_userptr(menu_item[menu_index],
			 (char *)&userptr[menu_index]);

	menu_index++;
	menu_item[menu_index] = new_item("Non-Optimized","");
	userptr[menu_index] = 1;
	set_item_userptr(menu_item[menu_index],
			 (char *)&userptr[menu_index]);

	menu_index++;
	menu_item[menu_index] = new_item("Not Set","");
	userptr[menu_index] = 2;
	set_item_userptr(menu_item[menu_index],
			 (char *)&userptr[menu_index]);

	menu_index++;
	menu_item[menu_index] = NULL;
	rc = display_menu(menu_item, start_row, menu_index, &new_state);

	if (rc == RC_SUCCESS) {
		/* set field_data based on menu choice */
		if (*new_state == IPR_ACTIVE_OPTIMIZED)
			sprintf(i_con->field_data, "Optimized");
		else if (*new_state == IPR_ACTIVE_NON_OPTIMIZED)
			sprintf(i_con->field_data, "Non-Optimized");
		else if (*new_state == IPR_ACTIVE_STANDBY)
			sprintf(i_con->field_data, "Not Set");
		/* only issue the command if the state selection changed */
		if (acd->dev->array_rcd->saved_asym_access_state != *new_state)
			acd->do_cmd = 1;
		else
			acd->do_cmd = 0;
	} else {
		/* set field_data based on existing value */
		if (acd->dev->array_rcd->saved_asym_access_state == IPR_ACTIVE_OPTIMIZED)
			sprintf(i_con->field_data, "Optimized");
		else if (acd->dev->array_rcd->saved_asym_access_state == IPR_ACTIVE_NON_OPTIMIZED)
			sprintf(i_con->field_data, "Non-Optimized");
		else
			sprintf(i_con->field_data, "Not Set");
	}

	menu_index = 0;
	while (menu_item[menu_index] != NULL)
		free_item(menu_item[menu_index++]);
	free(menu_item);
	free(userptr);
	menu_item = NULL;

	return rc;
}

/**
 * configure_asym_access - Configure the array setting for asymmetric access.
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int configure_asym_access(struct array_cmd_data *acd)
{
	i_container *new_i_con, *i_con_head_saved;
	int header_lines = 0;
	char pref_str[20];
	char buffer[128];
	char *body = NULL;
	struct screen_output *s_out;
	int state = 0, rc = REFRESH_FLAG;

	i_con_head_saved = i_con_head;
	i_con_head = new_i_con = NULL;

	body = body_init(n_change_array_asym_access.header, &header_lines);
	sprintf(buffer, "Array: %s", acd->dev->dev_name);
	body = add_line_to_body(body, buffer, NULL);
	if (acd->dev->array_rcd->current_asym_access_state == IPR_ACTIVE_OPTIMIZED)
		sprintf(buffer, "Current asymmetric access state: Optimized");
	else
		sprintf(buffer, "Current asymmetric access state: Non-Optimized");
	body = add_line_to_body(body, buffer, NULL);
	if (acd->dev->array_rcd->saved_asym_access_state == IPR_ACTIVE_OPTIMIZED) {
		sprintf(buffer, "Saved asymmetric access state: Optimized\n");
		sprintf(pref_str, "Optimized");
	} else if (acd->dev->array_rcd->saved_asym_access_state == IPR_ACTIVE_NON_OPTIMIZED) {
		sprintf(buffer, "Saved asymmetric access state: Non-Optimized\n");
		sprintf(pref_str, "Non-Optimized");
	} else {
		sprintf(buffer, "Saved asymmetric access state: Not Set\n");
		sprintf(pref_str, "Not Set");
	}
	body = add_line_to_body(body, buffer, NULL);

	header_lines += 2;

	body = add_line_to_body(body,_("Preferred Asymmetric Access State"), "%13");

	new_i_con = add_i_con(new_i_con, pref_str, acd);

	n_change_array_asym_access.body = body;

	while (rc & REFRESH_FLAG) {
		s_out = screen_driver(&n_change_array_asym_access, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);

		if (rc == RC_SUCCESS) {
			if (!acd->do_cmd) {
				rc = REFRESH_FLAG;
				break;
			}

			if (!strncmp(new_i_con->field_data, "Optimized", 8))
				state = IPR_ACTIVE_OPTIMIZED;
			else if (!strncmp(new_i_con->field_data, "Non-Optimized", 8))
				state = IPR_ACTIVE_NON_OPTIMIZED;
			else
				state = IPR_ACTIVE_STANDBY;

			acd->dev->array_rcd->issue_cmd = 1;
			acd->dev->array_rcd->saved_asym_access_state = state;
			rc = ipr_set_array_asym_access(acd->ioa);
		}
	}
	processing();

	free(n_change_array_asym_access.body);
	n_change_array_asym_access.body = NULL;
	free_i_con(new_i_con);
	i_con_head = i_con_head_saved;
	return rc;
}

/**
 * process_asym_access - Process the arrays that support asymmetric access.
 * @buffer:
 * @header_lines:
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int process_asym_access(char *buffer[], int header_lines)
{
	struct screen_output *s_out;
	int found = 0;
	int toggle = 1;
	i_container *i_con;
	char *input;
	int rc;
	struct array_cmd_data *acd;

	while (1) {
		toggle_field = 0;

		do {
			n_asym_access.body = buffer[toggle&1];
			/* display array selection screen */
			s_out = screen_driver(&n_asym_access, header_lines, NULL);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		if (rc & EXIT_FLAG || rc & CANCEL_FLAG)
			break;

		found = 0;
		 /* do one at a time */
		for_each_icon(i_con) {
			acd = i_con->data;
			input = i_con->field_data;

			if (acd && strcmp(input, "1") == 0) {
				found++;
				break;
			}
		}

		if (found) {
			/* Go to asymmetric access selection screen */
			rc = configure_asym_access(acd);

			/* Done on success or exit */
			if (rc == RC_SUCCESS || rc & EXIT_FLAG)
				break;

			/* clear the do_cmd flag */
			acd->do_cmd = 0;
		} else
			s_status.index = INVALID_OPTION_STATUS;
	}

	return rc;
}

/**
 * asym_access - GUI routine for setting array asymmetric access.
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int asym_access(i_container *i_con)
{
	int rc, k;
	int found = 0;
	char *buffer[2];
	struct screen_output *s_out;
	int header_lines;
	struct ipr_ioa *ioa;
	struct ipr_dev *array;

	processing();

	/* make sure the i_con list is empty */
	i_con = free_i_con(i_con);

	check_current_config(false);

	rc = RC_SUCCESS;

	body_init_status(buffer, n_asym_access.header, &header_lines);

	for_each_primary_ioa(ioa) {
		if (!ioa->asymmetric_access || !ioa->asymmetric_access_enabled)
			continue;
		for_each_array(ioa, array) {
			if (!array->array_rcd->asym_access_cand) {
				syslog_dbg("candidate not set\n");
				continue;
			}
			add_raid_cmd_tail(ioa, array, array->array_id);
			i_con = add_i_con(i_con, "\0", raid_cmd_tail);

			print_dev(k, array, buffer, "%1", k+2);
			found++;
		}
	}

	if (!found) {
		n_asym_access_fail.body = body_init(n_asym_access_fail.header, NULL);
		s_out = screen_driver(&n_asym_access_fail, 0, i_con);

		free(n_asym_access_fail.body);
		n_asym_access_fail.body = NULL;

		rc = s_out->rc | CANCEL_FLAG;
		free(s_out);
	} else
		rc = process_asym_access(buffer, header_lines);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	return rc;
}

/**
 * raid_include - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int raid_include(i_container *i_con)
{
	int k;
	int found = 0;
	struct ipr_array_record *array_rcd;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct ipr_array_cap_entry *cap_entry;
	int rc = RC_SUCCESS;
	struct screen_output *s_out;
	int header_lines;
	int toggle = 1;

	processing();

	i_con = free_i_con(i_con);

	check_current_config(false);

	body_init_status(buffer, n_raid_include.header, &header_lines);

	for_each_ioa(ioa) {
		if (ioa->is_secondary)
			continue;

		for_each_vset(ioa, dev) {
			array_rcd = dev->array_rcd;
			cap_entry = get_raid_cap_entry(ioa->supported_arrays,
						       dev->raid_level);

			if (!cap_entry || !cap_entry->include_allowed || !array_rcd->established)
				continue;

			add_raid_cmd_tail(ioa, dev, dev->array_id);

			i_con = add_i_con(i_con,"\0",raid_cmd_tail);
			print_dev(k, dev, buffer, "%1", k);
			found++;
		}
	}

	if (!found) {
		/* Include Device Parity Protection Failed */
		n_raid_include_fail.body = body_init(n_raid_include_fail.header, NULL);
		s_out = screen_driver(&n_raid_include_fail, 0, i_con);

		free(n_raid_include_fail.body);
		n_raid_include_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		toggle_field = 0;

		do {
			n_raid_include.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_include, header_lines, i_con);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		free_raid_cmds();
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_include.body = NULL;

	return rc;
}

/**
 * configure_raid_include - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 *   INVALID_OPTION_STATUS if nothing to process
 **/
int configure_raid_include(i_container *i_con)
{
	int k;
	int found = 0;
	struct array_cmd_data *cur_raid_cmd = NULL;
	struct ipr_array_query_data *qac_data = calloc(1,sizeof(struct ipr_array_query_data));
	struct ipr_dev_record *dev_rcd;
	struct scsi_dev_data *scsi_dev_data;
	char *buffer[2];
	struct ipr_ioa *ioa;
	char *input;
	struct ipr_dev *vset, *dev;
	struct ipr_array_cap_entry *cap_entry;
	u8 min_mult_array_devices = 1;
	int rc = RC_SUCCESS;
	int header_lines;
	i_container *temp_i_con;
	int toggle = 0;
	struct screen_output *s_out;

	for_each_icon(temp_i_con) {
		cur_raid_cmd = temp_i_con->data;
		if (!cur_raid_cmd)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			found++;
			cur_raid_cmd->do_cmd = 1;
			break;
		}
	}

	if (found != 1)
		return INVALID_OPTION_STATUS;

	i_con = free_i_con(i_con);

	body_init_status(buffer, n_configure_raid_include.header, &header_lines);

	ioa = cur_raid_cmd->ioa;
	vset = cur_raid_cmd->dev;

	/* Get Query Array Config Data */
	rc = ipr_query_array_config(ioa, 0, 1, 0, cur_raid_cmd->array_id, qac_data);

	found = 0;

	if (rc != 0)
		free(qac_data);
	else  {
		cur_raid_cmd->qac = qac_data;
		cap_entry = get_raid_cap_entry(ioa->supported_arrays,
					       vset->raid_level);
		if (cap_entry)
			min_mult_array_devices = cap_entry->min_mult_array_devices;

		for_each_dev_rcd(dev_rcd, qac_data) {
			for_each_dev(ioa, dev) {
				scsi_dev_data = dev->scsi_dev_data;
				if (!scsi_dev_data)
					continue;

				if (scsi_dev_data->handle == ipr_get_dev_res_handle(ioa, dev_rcd) &&
				    scsi_dev_data->opens == 0 &&
				    dev_rcd->include_cand && device_supported(dev)) {
					dev->dev_rcd = dev_rcd;
					i_con = add_i_con(i_con, "\0", dev);
					print_dev(k, dev, buffer, "%1", k);
					found++;
					break;
				}
			}
		}
	}

	if (found < min_mult_array_devices) {
		/* Include Device Parity Protection Failed */
		n_configure_raid_include_fail.body = body_init(n_configure_raid_include_fail.header, NULL);
		s_out = screen_driver(&n_configure_raid_include_fail, 0, i_con);

		free(n_configure_raid_include_fail.body);
		n_configure_raid_include_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);

		return rc;
	}

	toggle_field = 0;

	do {
		n_configure_raid_include.body = buffer[toggle&1];
		s_out = screen_driver(&n_configure_raid_include, header_lines, i_con);
		temp_i_con = s_out->i_con;
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_configure_raid_include.body = NULL;

	i_con = temp_i_con;

	if (rc > 0)
		goto leave;

	/* first, count devices selected to be sure min multiple is satisfied */
	found = 0;

	for_each_icon(temp_i_con) {
		input = temp_i_con->field_data;
		if (strcmp(input, "1") == 0)
			found++;
	}

	if (found % min_mult_array_devices != 0)  {
		/* "Error:  number of devices selected must be a multiple of %d" */
		s_status.num = min_mult_array_devices;
		return 25 | REFRESH_FLAG;
	}

	for_each_icon(temp_i_con) {
		dev = (struct ipr_dev *)temp_i_con->data;
		if (!dev)
			continue;
		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			if (!dev->dev_rcd->issue_cmd) {
				dev->dev_rcd->issue_cmd = 1;
				dev->dev_rcd->known_zeroed = 1;
			}
		} else {
			if (dev->dev_rcd->issue_cmd) {
				dev->dev_rcd->issue_cmd = 0;
				dev->dev_rcd->known_zeroed = 0;
			}
		}
	}
	rc = confirm_raid_include(i_con);

leave:
	return rc;
}

/**
 * confirm_raid_include - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int confirm_raid_include(i_container *i_con)
{
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int k;
	char *buffer[2];
	int rc = RC_SUCCESS;
	struct screen_output *s_out;
	int num_devs = 0;
	int dev_include_complete(u8 num_devs);
	int format_include_cand();
	int header_lines;
	int toggle = 0;
	int need_formats = 0;

	body_init_status(buffer, n_confirm_raid_include.header, &header_lines);

	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->qac)
			continue;

		ioa = cur_raid_cmd->ioa;
		for_each_af_dasd(ioa, dev) {
			if (!dev->dev_rcd->issue_cmd)
				continue;
			print_dev(k, dev, buffer, "1", k);
			if (!ipr_device_is_zeroed(dev))
				need_formats = 1;
			num_devs++;
		}
	}

	do {
		n_confirm_raid_include.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_include, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_raid_include.body = NULL;

	if (rc)
		return rc;

	/* now format each of the devices */
	if (need_formats)
		num_devs = format_include_cand();

	rc = dev_include_complete(num_devs);
	free_devs_to_init();

	return rc;
}

/**
 * format_include_cand -
 *
 * Returns:
 *   number of devices processed
 **/
int format_include_cand()
{
	struct scsi_dev_data *scsi_dev_data;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int num_devs = 0;
	int rc = 0;
	struct ipr_dev_record *device_record;
	int opens = 0;

	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;
		ioa = cur_raid_cmd->ioa;

		for_each_dev(ioa, dev) {
			device_record = dev->dev_rcd;
			if (!device_record || !device_record->issue_cmd)
				continue;

			/* get current "opens" data for this device to determine conditions to continue */
			scsi_dev_data = dev->scsi_dev_data;
			if (scsi_dev_data)
				opens = num_device_opens(scsi_dev_data->host,
							 scsi_dev_data->channel,
							 scsi_dev_data->id,
							 scsi_dev_data->lun);

			if (opens != 0)  {
				syslog(LOG_ERR,_("Include Device not allowed, device in use - %s\n"),
				       dev->gen_name);
				device_record->issue_cmd = 0;
				continue;
			}

			if (dev_init_head) {
				dev_init_tail->next = malloc(sizeof(struct devs_to_init_t));
				dev_init_tail = dev_init_tail->next;
			} else
				dev_init_head = dev_init_tail =
					malloc(sizeof(struct devs_to_init_t));

			memset(dev_init_tail, 0, sizeof(struct devs_to_init_t));
			dev_init_tail->ioa = ioa;
			dev_init_tail->dev = dev;
			dev_init_tail->do_init = 1;

			/* Issue the format. Failure will be detected by query command status */
			rc = ipr_format_unit(dev);

			num_devs++;
		}
	}
	return num_devs;
}

/**
 * update_include_qac_data - 
 * @ioa:		ipr ioa struct
 * @old_qac:		ipr_array_query_data struct
 * @new_qac:		ipr_array_query_data struct
 *
 * Returns:
 *   0 if success / 26 on failure
 **/
static int update_include_qac_data(struct ipr_ioa *ioa,
				   struct ipr_array_query_data *old_qac,
				   struct ipr_array_query_data *new_qac)
{
	struct ipr_dev_record *old_dev_rcd, *new_dev_rcd;
	int found;

	for_each_dev_rcd(old_dev_rcd, old_qac) {
		if (!old_dev_rcd->issue_cmd)
			continue;

		found = 0;
		for_each_dev_rcd(new_dev_rcd, new_qac) {
			if (ipr_get_dev_res_handle(ioa, new_dev_rcd) !=
			    ipr_get_dev_res_handle(ioa, old_dev_rcd))
				continue;

			new_dev_rcd->issue_cmd = 1;
			new_dev_rcd->known_zeroed = 1;
			found = 1;
			break;
		}

		if (!found)
			return 26;
	}

	return 0;
}

/**
 * do_array_include - 
 * @ioa:		ipr ioa struct
 * @array_id:		array ID
 * @qac:		ipr_array_query_data struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int do_array_include(struct ipr_ioa *ioa, int array_id,
			    struct ipr_array_query_data *qac)
{
	int fd, rc;
	struct ipr_array_query_data qac_data;

	memset(&qac_data, 0, sizeof(qac_data));

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1)
		return -EIO;

	rc = flock(fd, LOCK_EX);
	if (rc)
		goto out;

	rc = __ipr_query_array_config(ioa, fd, 0, 1, 0, array_id, &qac_data);

	if (rc)
		goto out;

	rc = update_include_qac_data(ioa, qac, &qac_data);
	if (rc)
		goto out;

	rc = ipr_add_array_device(ioa, fd, &qac_data);

out:
	close(fd);
	return rc;
}

/**
 * dev_include_complete - 
 * @num_devs:		not used ?
 *
 * Returns:
 *   27 | EXIT_FLAG on success / 26 | EXIT_FLAG otherwise
 **/
int dev_include_complete(u8 num_devs)
{
	int done_bad;
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int not_done = 0;
	int rc;
	struct devs_to_init_t *cur_dev_init;
	u32 percent_cmplt = 0;
	struct ipr_ioa *ioa;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_common_record *common_record;

	n_dev_include_complete.body = n_dev_include_complete.header[0];

	while(1) {
		complete_screen_driver(&n_dev_include_complete, percent_cmplt, 0);

		percent_cmplt = 100;
		done_bad = 0;

		for_each_dev_to_init(cur_dev_init) {
			if (cur_dev_init->do_init) {
				rc = ipr_query_command_status(cur_dev_init->dev, &cmd_status);

				if (rc || cmd_status.num_records == 0) {
					cur_dev_init->cmplt = 100;
					continue;
				}

				status_record = cmd_status.record;

				if (status_record->status == IPR_CMD_STATUS_FAILED) {
					cur_dev_init->cmplt = 100;
					done_bad = 1;
				} else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL) {
					cur_dev_init->cmplt = status_record->percent_complete;
					if (cur_dev_init->cmplt < percent_cmplt)
						percent_cmplt = cur_dev_init->cmplt;
					not_done = 1;
				}
			}
		}

		if (!not_done) {
			flush_stdscr();

			if (done_bad) {
				/* "Include failed" */
				rc = 26;
				return rc | EXIT_FLAG; 
			}

			break;
		}
		not_done = 0;
		sleep(1);
	}

	complete_screen_driver(&n_dev_include_complete, percent_cmplt, 0);

	/* now issue the start array command with "known to be zero" */
	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;

		if (do_array_include(cur_raid_cmd->ioa,
				     cur_raid_cmd->array_id, cur_raid_cmd->qac)) {
			rc = 26;
			continue;
		}
	}

	not_done = 0;
	percent_cmplt = 0;

	n_dev_include_complete.body = n_dev_include_complete.header[1];

	while(1) {
		complete_screen_driver(&n_dev_include_complete, percent_cmplt, 1);

		percent_cmplt = 100;
		done_bad = 0;

		for_each_raid_cmd(cur_raid_cmd) {
			ioa = cur_raid_cmd->ioa;

			rc = ipr_query_command_status(&ioa->ioa, &cmd_status);

			if (rc) {
				done_bad = 1;
				continue;
			}

			for_each_cmd_status(status_record, &cmd_status) {
				if (status_record->command_code == IPR_ADD_ARRAY_DEVICE &&
				    cur_raid_cmd->array_id == status_record->array_id) {

					if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS) {
						if (status_record->percent_complete < percent_cmplt)
							percent_cmplt = status_record->percent_complete;

						not_done = 1;
					} else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL)
						/* "Include failed" */
						rc = RC_26_Include_Fail;
					break;
				}
			}
		}

		if (!not_done) {
			/* Call ioctl() to re-read partition table to handle change in size */
			for_each_raid_cmd(cur_raid_cmd) {
				if (!cur_raid_cmd->do_cmd)
					continue;

				common_record = cur_raid_cmd->dev->qac_entry;
				if (common_record &&
				    common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD)

					rc = ipr_write_dev_attr(cur_raid_cmd->dev, "rescan", "1");
			}

			flush_stdscr();

			if (done_bad)
				/* "Include failed" */
				return RC_26_Include_Fail | EXIT_FLAG;

			/*  "Include Unit completed successfully" */
			complete_screen_driver(&n_dev_include_complete, percent_cmplt, 1);
			return RC_27_Include_Success | EXIT_FLAG;
		}
		not_done = 0;
		sleep(1);
	}
}

/**
 * add_format_device - 
 * @dev:		ipr dev struct
 * @blk_size:		block size
 *
 * Returns:
 *   nothing
 **/
static void add_format_device(struct ipr_dev *dev, int blk_size)
{
	if (dev_init_head) {
		dev_init_tail->next = malloc(sizeof(struct devs_to_init_t));
		dev_init_tail = dev_init_tail->next;
	}
	else
		dev_init_head = dev_init_tail = malloc(sizeof(struct devs_to_init_t));

	memset(dev_init_tail, 0, sizeof(struct devs_to_init_t));

	dev_init_tail->ioa = dev->ioa;
	if (ipr_is_af_dasd_device(dev))
		dev_init_tail->dev_type = IPR_AF_DASD_DEVICE;
	else
		dev_init_tail->dev_type = IPR_JBOD_DASD_DEVICE;
	dev_init_tail->new_block_size = blk_size;
	dev_init_tail->dev = dev;
	dev_init_tail->device_id = dev->scsi_dev_data->device_id;

	scsi_dbg(dev, "Formatting device to %d bytes/block. Device ID=%lx\n",
		 blk_size, dev_init_tail->device_id);
}

/**
 * configure_af_device -
 * @i_con:		i_container struct
 * @action_code:	include or remove
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int configure_af_device(i_container *i_con, int action_code)
{
	int rc, j, k;
	struct scsi_dev_data *scsi_dev_data;
	int can_init;
	int dev_type;
	int new_block_size;
	char *buffer[2];
	int num_devs = 0;
	struct ipr_ioa *ioa;
	int toggle = 0;
	int header_lines;
	s_node *n_screen;
	struct screen_output *s_out;

	processing();

	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);

	/* Setup screen title */
	if (action_code == IPR_INCLUDE)
		n_screen = &n_af_init_device;
	else
		n_screen = &n_jbod_init_device;

	body_init_status(buffer, n_screen->header, &header_lines);

	for_each_ioa(ioa) {
		if (ioa->is_secondary)
			continue;

		for (j = 0; j < ioa->num_devices; j++) {
			can_init = 1;
			scsi_dev_data = ioa->dev[j].scsi_dev_data;

			/* If not a DASD, disallow format */
			if (!scsi_dev_data ||
			    ipr_is_hot_spare(&ioa->dev[j]) ||
			    ipr_is_volume_set(&ioa->dev[j]) ||
			    !device_supported(&ioa->dev[j]) ||
			    (scsi_dev_data->type != TYPE_DISK &&
			     scsi_dev_data->type != IPR_TYPE_AF_DISK))
				continue;

			/* If Advanced Function DASD */
			if (ipr_is_af_dasd_device(&ioa->dev[j])) {
				if (action_code == IPR_INCLUDE) {
					if (ipr_device_is_zeroed(&ioa->dev[j]))
						continue;
					new_block_size = 0;
				} else if (ioa->dev[j].block_dev_class & IPR_BLK_DEV_CLASS_4K)
						new_block_size = IPR_JBOD_4K_BLOCK_SIZE;
					else
						new_block_size = IPR_JBOD_BLOCK_SIZE;

				dev_type = IPR_AF_DASD_DEVICE;

				/* We allow the user to format the drive if nobody is using it */
				if (ioa->dev[j].scsi_dev_data->opens != 0) {
					syslog(LOG_ERR, _("Format not allowed to %s, device in use\n"), ioa->dev[j].gen_name); 
					continue;
				}

				if (ipr_is_array_member(&ioa->dev[j])) {
					if (ioa->dev[j].dev_rcd->no_cfgte_vol)
						can_init = 1;
					else
						can_init = 0;
				} else
					can_init = is_format_allowed(&ioa->dev[j]);
			} else if (scsi_dev_data->type == TYPE_DISK){
				/* If on a JBOD adapter */
				if (!ioa->qac_data->num_records) {
					if (action_code != IPR_REMOVE)
						continue;
				} else if (is_af_blocked(&ioa->dev[j], 0))
					continue;
				if (action_code == IPR_REMOVE && !format_req(&ioa->dev[j]))
					continue;

				if (action_code == IPR_REMOVE) {
					if (ioa->support_4k && ioa->dev[j].block_dev_class & IPR_BLK_DEV_CLASS_4K)
						new_block_size = IPR_JBOD_4K_BLOCK_SIZE;
					else
						new_block_size = IPR_JBOD_BLOCK_SIZE;
				} else {
					if (ioa->support_4k && ioa->dev[j].block_dev_class & IPR_BLK_DEV_CLASS_4K)
						new_block_size = IPR_AF_4K_BLOCK_SIZE;
					else
						new_block_size = ioa->af_block_size;
				}

				dev_type = IPR_JBOD_DASD_DEVICE;

				/* We allow the user to format the drive if nobody is using it, or
				 the device is read write protected. If the drive fails, then is replaced
				 concurrently it will be read write protected, but the system may still
				 have a use count for it. We need to allow the format to get the device
				 into a state where the system can use it */
				if (ioa->dev[j].scsi_dev_data->opens != 0) {
					syslog(LOG_ERR, _("Format not allowed to %s, device in use\n"), ioa->dev[j].gen_name); 
					continue;
				}

				can_init = is_format_allowed(&ioa->dev[j]);
			} else
				continue;

			if (can_init) {
				add_format_device(&ioa->dev[j], new_block_size);
				print_dev(k, &ioa->dev[j], buffer, "%1", k);
				i_con = add_i_con(i_con,"\0",dev_init_tail);
				num_devs++;
			}
		}
	}

	if (!num_devs) {
		if (action_code == IPR_INCLUDE) {
			n_af_include_fail.body = body_init(n_af_include_fail.header, NULL);
			s_out = screen_driver(&n_af_include_fail, 0, i_con);

			free(n_af_include_fail.body);
			n_af_include_fail.body = NULL;
		} else {
			n_af_remove_fail.body = body_init(n_af_remove_fail.header, NULL);
			s_out = screen_driver(&n_af_remove_fail, 0, i_con);

			free(n_af_remove_fail.body);
			n_af_remove_fail.body = NULL;
		}

		rc = s_out->rc;
		free(s_out);
	} else {
		toggle_field = 0;

		do {
			n_screen->body = buffer[toggle&1];
			s_out = screen_driver(n_screen, header_lines, i_con);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		free_devs_to_init();
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_screen->body = NULL;

	return rc;
}

/**
 * af_include - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int af_include(i_container *i_con)
{
	int rc;
	rc = configure_af_device(i_con, IPR_INCLUDE);
	return rc;
}

/**
 * af_remove - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int af_remove(i_container *i_con)
{
	int rc;
	rc = configure_af_device(i_con, IPR_REMOVE);
	return rc;
}

/**
 * add_hot_spare -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int add_hot_spare(i_container *i_con)
{
	int rc;

	do {
		rc = hot_spare(i_con, IPR_ADD_HOT_SPARE);
	} while (rc & REFRESH_FLAG);
	return rc;
}

/**
 * remove_hot_spare -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int remove_hot_spare(i_container *i_con)
{
	int rc;

	do {
		rc = hot_spare(i_con, IPR_RMV_HOT_SPARE);
	} while (rc & REFRESH_FLAG);
	return rc;
}

/**
 * hot_spare - 
 * @i_con:		i_container struct
 * @action:		add or remove
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int hot_spare(i_container *i_con, int action)
{
	int rc, k;
	struct array_cmd_data *cur_raid_cmd;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int found = 0;
	char *input;
	int toggle = 0;
	i_container *temp_i_con;
	struct screen_output *s_out;
	s_node *n_screen;
	int header_lines;

	processing();
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);

	if (action == IPR_ADD_HOT_SPARE)
		n_screen = &n_add_hot_spare;
	else
		n_screen = &n_remove_hot_spare;

	body_init_status(buffer, n_screen->header, &header_lines);

	for_each_ioa(ioa) {
		for_each_af_dasd(ioa, dev) {
			if ((action == IPR_ADD_HOT_SPARE &&
			     dev->dev_rcd->add_hot_spare_cand) ||
			    (action == IPR_RMV_HOT_SPARE &&
			     dev->dev_rcd->rmv_hot_spare_cand)) {

				add_raid_cmd_tail(ioa, NULL, 0);

				i_con = add_i_con(i_con,"\0",raid_cmd_tail);

				print_dev(k, &ioa->ioa, buffer, "%1", k);
				found++;
				break;
			}
		}
	}

	if (!found) {
		if (action == IPR_ADD_HOT_SPARE) {
			n_add_hot_spare_fail.body = body_init(n_add_hot_spare_fail.header, NULL);
			s_out = screen_driver(&n_add_hot_spare_fail, 0, i_con);

			free(n_add_hot_spare_fail.body);
			n_add_hot_spare_fail.body = NULL;
		} else {
			n_remove_hot_spare_fail.body = body_init(n_remove_hot_spare_fail.header, NULL);
			s_out = screen_driver(&n_remove_hot_spare_fail, 0, i_con);

			free(n_remove_hot_spare_fail.body);
			n_remove_hot_spare_fail.body = NULL;
		}

		free(s_out);
		rc = EXIT_FLAG;
		goto leave;
	}

	do {
		n_screen->body = buffer[toggle&1];
		s_out = screen_driver(n_screen, header_lines, i_con);
		temp_i_con = s_out->i_con;
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	i_con = temp_i_con;

	if (rc > 0) {
		rc = EXIT_FLAG;
		goto leave;
	}

	found = 0;

	for_each_icon(temp_i_con) {
		cur_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
		if (cur_raid_cmd)  {
			input = temp_i_con->field_data;

			if (strcmp(input, "1") == 0) {
				do {
					rc = select_hot_spare(temp_i_con, action);
				} while (rc & REFRESH_FLAG);

				if (rc > 0)
					goto leave;

				found = 1;

				cur_raid_cmd->do_cmd = 1;
			} else {
				cur_raid_cmd->do_cmd = 0;
			}
		}
	}

	if (found) {
		/* Go to verification screen */
		rc = confirm_hot_spare(action);
	} else
		rc = REFRESH_FLAG | INVALID_OPTION_STATUS;


leave:
	if (rc & CANCEL_FLAG) {
		s_status.index = (rc & ~CANCEL_FLAG);
		rc = (rc | REFRESH_FLAG) & ~CANCEL_FLAG;
	}

	i_con = free_i_con(i_con);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_screen->body = NULL;
	free_raid_cmds();

	return rc;
}

/**
 * select_hot_spare - 
 * @i_con:		i_container struct
 * @action:		add or remove
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int select_hot_spare(i_container *i_con, int action)
{
	struct array_cmd_data *cur_raid_cmd;
	int rc, k, found;
	char *buffer[2];
	int num_devs = 0;
	struct ipr_ioa *ioa;
	char *input;
	struct ipr_dev *dev;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_dev_record *dev_rcd;
	i_container *temp_i_con,*i_con2 = NULL;
	i_container *temp_i_con2;
	struct screen_output *s_out = NULL;
	int header_lines;
	s_node *n_screen;
	int toggle = 0;
	i_container *i_con_head_saved;

	if (action == IPR_ADD_HOT_SPARE)
		n_screen = &n_select_add_hot_spare;
	else
		n_screen = &n_select_remove_hot_spare;

	body_init_status(buffer, n_screen->header, &header_lines);

	cur_raid_cmd = (struct array_cmd_data *) i_con->data;
	rc = RC_SUCCESS;
	ioa = cur_raid_cmd->ioa;
	i_con_head_saved = i_con_head; /* FIXME */
	i_con_head = NULL;

	for_each_af_dasd(ioa, dev) {
		scsi_dev_data = dev->scsi_dev_data;
		dev_rcd = dev->dev_rcd;

		if (!scsi_dev_data)
			continue;

		if (device_supported(dev) &&
		    ((dev_rcd->add_hot_spare_cand && action == IPR_ADD_HOT_SPARE) ||
		     (dev_rcd->rmv_hot_spare_cand && action == IPR_RMV_HOT_SPARE))) {
			i_con2 = add_i_con(i_con2, "\0", dev);

			print_dev(k, dev, buffer, "%1", k);
			num_devs++;
		}
	}

	if (num_devs) {
		do {
			n_screen->body = buffer[toggle&1];
			s_out = screen_driver(n_screen, header_lines, i_con2);
			temp_i_con2 = s_out->i_con;
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);

		for (k = 0; k < 2; k++) {
			free(buffer[k]);
			buffer[k] = NULL;
		}

		n_screen->body = NULL;

		i_con2 = temp_i_con2;

		if (rc > 0)
			goto leave;

		found = 0;

		for_each_icon(temp_i_con) {
			dev = (struct ipr_dev *)temp_i_con->data;
			if (dev != NULL) {
				input = temp_i_con->field_data;
				if (strcmp(input, "1") == 0) {
					found = 1;
					dev->dev_rcd->issue_cmd = 1;
				}
			}
		}

		if (found)
			rc = RC_SUCCESS;
		else  {
			s_status.index = INVALID_OPTION_STATUS;
			rc = REFRESH_FLAG;
		}
	} else
		/* "No devices available for the selected hot spare operation"  */
		rc = 52 | CANCEL_FLAG; 

leave:
	i_con2 = free_i_con(i_con2);
	i_con_head = i_con_head_saved;
	return rc;
}

/**
 * confirm_hot_spare - 
 * @action:		add or remove
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int confirm_hot_spare(int action)
{
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int rc, k;
	char *buffer[2];
	int hot_spare_complete(int action);
	struct screen_output *s_out;
	s_node *n_screen;
	int header_lines;
	int toggle = 0;

	rc = RC_SUCCESS;

	if (action == IPR_ADD_HOT_SPARE)
		n_screen = &n_confirm_add_hot_spare;
	else
		n_screen = &n_confirm_remove_hot_spare;

	body_init_status(buffer, n_screen->header, &header_lines);

	for_each_raid_cmd(cur_raid_cmd) {
		if (!cur_raid_cmd->do_cmd)
			continue;

		ioa = cur_raid_cmd->ioa;

		for_each_af_dasd(ioa, dev) {
			if (dev->dev_rcd->issue_cmd)
				print_dev(k, dev, buffer, "1", k);
		}
	}

	do {
		n_screen->body = buffer[toggle&1];
		s_out = screen_driver(n_screen, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_screen->body = NULL;

	if(rc > 0)
		goto leave;

	rc = hot_spare_complete(action);

leave:
	return rc;
}

/**
 * hot_spare_complete -
 * @action:		add or remove
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int hot_spare_complete(int action)
{
	int rc;
	struct ipr_ioa *ioa;
	struct array_cmd_data *cmd;

	for_each_ioa(ioa) {
		for_each_raid_cmd(cmd) {
			if (cmd->do_cmd == 0 || ioa != cmd->ioa)
				continue;

			flush_stdscr();

			if (action == IPR_ADD_HOT_SPARE)
				rc = ipr_add_hot_spare(ioa);
			else
				rc = ipr_remove_hot_spare(ioa);

			if (rc != 0) {
				if (action == IPR_ADD_HOT_SPARE)
					/* Enable Device as Hot Spare failed */
					return 55 | EXIT_FLAG; 
				else
					/* Disable Device as Hot Spare failed */
					return 56 | EXIT_FLAG;
			} 
		}
	}

	flush_stdscr();

	if (action == IPR_ADD_HOT_SPARE)
		/* "Enable Device as Hot Spare Completed successfully"  */
		rc = 53 | EXIT_FLAG; 
	else
		/* "Disable Device as Hot Spare Completed successfully"  */
		rc = 54 | EXIT_FLAG; 

	return rc;
}

/**
 * disk_unit_recovery - 
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int disk_unit_recovery(i_container *i_con)
{
	int rc;
	struct screen_output *s_out;
	int loop;

	for (loop = 0; loop < n_disk_unit_recovery.num_opts; loop++) {
		n_disk_unit_recovery.body = ipr_list_opts(n_disk_unit_recovery.body,
							  n_disk_unit_recovery.options[loop].key,
							  n_disk_unit_recovery.options[loop].list_str);
	}

	n_disk_unit_recovery.body = ipr_end_list(n_disk_unit_recovery.body);

	s_out = screen_driver(&n_disk_unit_recovery, 0, NULL);
	free(n_disk_unit_recovery.body);
	n_disk_unit_recovery.body = NULL;

	rc = s_out->rc;
	free(s_out);
	return rc;
}

/**
 * get_elem_status - 
 * @dev:		ipr dev struct
 * @ses:		ipr dev struct
 * @ses_data:		ipr_encl_status_ctl_pg struct
 * @ses_config:		ipr_ses_config_pg struct
 *
 * Returns:
 *   ipr_drive_elem_status struct if success / NULL otherwise
 **/
static struct ipr_drive_elem_status *
get_elem_status(struct ipr_dev *dev, struct ipr_dev *ses,
		struct ipr_encl_status_ctl_pg *ses_data, struct ipr_ses_config_pg *ses_cfg)
{
	struct ipr_ioa *ioa = dev->ioa;
	struct ipr_res_addr *ra;
	struct ipr_drive_elem_status *elem_status, *overall;
	int bus = ses->scsi_dev_data->channel;
	int box, slot, is_vses;

	if (ioa->hop_count == IPR_2BIT_HOP)
		box = ses->scsi_dev_data->phy_2bit_hop.box;
	else
		box = ses->scsi_dev_data->phy.box;

	if (ipr_receive_diagnostics(ses, 2, ses_data, sizeof(*ses_data)))
		return NULL;
	if (ipr_receive_diagnostics(ses, 1, ses_cfg, sizeof(*ses_cfg)))
		return NULL;

	overall = ipr_get_overall_elem(ses_data, ses_cfg);
	if (!ioa_is_spi(dev->ioa) && (overall->device_environment == 0))
		is_vses = 1;
	else
		is_vses = 0;

	for_each_elem_status(elem_status, ses_data, ses_cfg) {
		slot = elem_status->slot_id;

		for_each_ra(ra, dev) {
			if (is_vses && (ra->bus != slot || ra->target != 0))
				continue;
			if (ioa->hop_count == IPR_2BIT_HOP) {
				if (!is_vses && (ra->bus != bus ||
						 ra->phy_2bit_hop.box != box ||
						 ra->phy_2bit_hop.phy != slot))
					continue;
			} else {
				if (!is_vses && (ra->bus != bus ||
						 ra->phy.box != box ||
						 ra->phy.phy != slot))
					continue;
			}
			return elem_status;
		}
	}

	return NULL;
}

/**
 * get_elem_status_64bit -
 * @dev:		ipr dev struct
 * @ses:		ipr dev struct
 * @ses_data:		ipr_encl_status_ctl_pg struct
 * @ses_config:		ipr_ses_config_pg struct
 *
 * Returns:
 *   ipr_drive_elem_status struct if success / NULL otherwise
 **/
static struct ipr_drive_elem_status *
get_elem_status_64bit(struct ipr_dev *dev, struct ipr_dev *ses,
		struct ipr_encl_status_ctl_pg *ses_data, struct ipr_ses_config_pg *ses_cfg)
{
	struct ipr_res_path *rp;
	struct ipr_drive_elem_status *elem_status;
	int slot, res_path_len, dev_slot, ses_path_len;

	if (ipr_receive_diagnostics(ses, 2, ses_data, sizeof(*ses_data)))
		return NULL;

	if (ipr_receive_diagnostics(ses, 1, ses_cfg, sizeof(*ses_cfg)))
		return NULL;

	ses_path_len  = strlen(ses->res_path_name);
	res_path_len = strlen(dev->res_path_name);
	dev_slot = strtoul(dev->res_path_name + (res_path_len-2), NULL, 16);

	if (ses_path_len != res_path_len)
		return NULL;

	for_each_elem_status(elem_status, ses_data, ses_cfg) {
		slot = elem_status->slot_id;
		for_each_rp(rp, dev) {
			if (!memcmp(&rp->res_path_bytes, &ses->res_path[0], ses_path_len/3) && slot == dev_slot)
				return elem_status;
		}
	}
	return NULL;
}

/**
 * wait_for_new_dev -
 * @ioa:		ipr ioa struct
 * @res_addr:		ipr_res_addr struct
 *
 * Returns:
 *   nothing
 **/
static void wait_for_new_dev(struct ipr_ioa *ioa, struct ipr_res_addr *res_addr)
{
	int time = 12;
	struct ipr_dev *dev;
	struct ipr_res_addr_aliases aliases;
	struct ipr_res_addr *ra;

	ipr_query_res_addr_aliases(ioa, res_addr, &aliases);

	for_each_ra_alias(ra, &aliases)
		ipr_scan(ioa, ra->bus, ra->target, ra->lun);

	while (time--) {
		check_current_config(false);
		for_each_ra_alias(ra, &aliases) {
			if ((dev = get_dev_from_addr(ra))) {
				ipr_init_new_dev(dev);
				return;
			}
		}
		sleep(5);
	}
}

/**
 * get_dev_from_res_path -
 * @res_addr:		ipr_res_addr struct
 *
 * Returns:
 *   ipr_dev struct if success / NULL otherwise
 **/
struct ipr_dev *get_dev_from_res_path(struct ipr_res_path *res_path)
{
	struct ipr_ioa *ioa;
	int j;
	struct scsi_dev_data *scsi_dev_data;
	char res_path_name[IPR_MAX_RES_PATH_LEN];

	ipr_format_res_path(res_path->res_path_bytes, res_path_name, IPR_MAX_RES_PATH_LEN);
	for_each_primary_ioa(ioa) {
		for (j = 0; j < ioa->num_devices; j++) {
			scsi_dev_data = ioa->dev[j].scsi_dev_data;

			if (!scsi_dev_data)
				continue;
			if (!strncmp(scsi_dev_data->res_path, res_path_name, IPR_MAX_RES_PATH_LEN))
				return &ioa->dev[j];
		}
	}
	return NULL;
}

static void wait_for_new_dev_64bit(struct ipr_ioa *ioa, struct ipr_res_path *res_path)
{
	int time = 12;
	struct ipr_dev *dev;
	struct ipr_res_path_aliases aliases;
	struct ipr_res_path *rp;

	ipr_query_res_path_aliases(ioa, res_path, &aliases);
	if (aliases.length < (4 + 2 * sizeof(struct ipr_res_path))) {
		aliases.length = 4 + 2 * sizeof(struct ipr_res_path);
		memcpy(&aliases.res_path[0], res_path, 2 * sizeof(struct ipr_res_path));
	}

	ipr_scan(ioa, 0, -1, -1);

	while (time--) {
		check_current_config(false);
		for_each_rp_alias(rp, &aliases) {
			if ((dev = get_dev_from_res_path(rp))) {
				ipr_init_new_dev(dev);
				return;
			}
		}
		sleep(5);
	}
}

static struct ipr_dev *delete_secondary_sysfs_name(struct ipr_dev *delete_dev)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev, *ses;

	for_each_secondary_ioa(ioa) {
		for_each_ses(ioa, ses) {
			for_each_dev(ses->ioa, dev) {
				if (ipr_is_ses(dev))
					continue;
				if (!strncmp(dev->res_path_name, delete_dev->res_path_name, strlen(delete_dev->res_path_name)))
					return dev;
			}
		}
	}
	return NULL;
}

int remove_or_add_back_device_64bit(struct ipr_dev *dev)
{
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status;
	struct ipr_ses_config_pg ses_cfg;
	int res_path_len, dev_slot;
	struct ipr_dev *sec_dev, *tmp_dev;
	char new_sysfs_res_path[IPR_MAX_RES_PATH_LEN];
	int rc;

	res_path_len  = strlen(dev->res_path_name);
	dev_slot = strtoul(dev->res_path_name + (res_path_len - 2), NULL, 16);

	if (!ipr_read_dev_attr(dev, "resource_path",
			       new_sysfs_res_path, IPR_MAX_RES_PATH_LEN))
		if (strncmp(dev->res_path_name, new_sysfs_res_path, sizeof(dev->res_path_name)))
			for_each_dev(dev->ioa, tmp_dev)
				if (!strncmp(tmp_dev->res_path_name, new_sysfs_res_path, sizeof(tmp_dev->res_path_name))) {
					dev = tmp_dev;
					break;
				}
	evaluate_device(dev, dev->ioa, 0);

	if (ipr_receive_diagnostics(dev->ses[0], 2, &ses_data, sizeof(ses_data)))
		return INVALID_OPTION_STATUS;
	if (ipr_receive_diagnostics(dev->ses[0], 1, &ses_cfg, sizeof(ses_cfg)))
		return INVALID_OPTION_STATUS;

	for_each_elem_status(elem_status, &ses_data, &ses_cfg) {
		if (elem_status->slot_id == dev_slot &&
		elem_status->status == IPR_DRIVE_ELEM_STATUS_POPULATED ) {
			wait_for_new_dev_64bit(dev->ioa, &(dev->res_path[0]));
			rc = 0;
			break;
		}
		else
			if (elem_status->slot_id == dev_slot &&
			elem_status->status == IPR_DRIVE_ELEM_STATUS_EMPTY ) {
				ipr_write_dev_attr(dev, "delete", "1");
				ipr_del_zeroed_dev(dev);
				sec_dev = delete_secondary_sysfs_name(dev);
				if (sec_dev) {
					evaluate_device(sec_dev, dev->ioa, 0);
					ipr_write_dev_attr(sec_dev, "delete", "1");
					ipr_del_zeroed_dev(sec_dev);
				}
				rc = 1;
				break;
			}
	}/*for_each_elem_status*/

	return rc;

}

/**
 * process_conc_maint -
 * @i_con:		i_container struct
 * @action:		add or remove
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int process_conc_maint(i_container *i_con, int action)
{
	i_container *temp_i_con;
	int found = 0;
	struct ipr_dev *dev, *ses;
	char *input;
	struct ipr_ioa *ioa;
	int k, rc;
	struct ipr_dev_record *dev_rcd;
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_ses_config_pg ses_cfg;
	struct ipr_drive_elem_status *elem_status, *overall;
	char *buffer[3];
	int header_lines;
	int toggle=0;
	s_node *n_screen;
	struct screen_output *s_out;
	struct ipr_res_addr res_addr;
	struct ipr_res_path res_path[2];
	int max_y, max_x;

	for_each_icon(temp_i_con) {
		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			dev = (struct ipr_dev *)temp_i_con->data;
			if (dev != NULL)
				found++;
		}
	}

	if (found != 1)
		return INVALID_OPTION_STATUS;

	if (dev->ioa->sis64)
		memcpy(&res_path, &(dev->res_path), sizeof(struct ipr_res_path)*2);
	else
		memcpy(&res_addr, &(dev->res_addr), sizeof(res_addr));

	if (ipr_is_af_dasd_device(dev) &&
	    (action == IPR_VERIFY_CONC_REMOVE || action == IPR_WAIT_CONC_REMOVE)) {
		/* issue suspend device bus to verify operation
		 is allowable */
		if (!ipr_can_remove_device(dev))
			return INVALID_OPTION_STATUS; /* FIXME */
	} else if (num_device_opens(res_addr.host, res_addr.bus,
				    res_addr.target, res_addr.lun))
		return INVALID_OPTION_STATUS;  /* FIXME */

	ioa = dev->ioa;
	ses = dev->ses[0];

	if (ioa->sis64)
		elem_status = get_elem_status_64bit(dev, ses, &ses_data, &ses_cfg);
	else
		elem_status = get_elem_status(dev, ses, &ses_data, &ses_cfg);
	if (!elem_status)
		return INVALID_OPTION_STATUS;

	if ((action == IPR_VERIFY_CONC_REMOVE)  ||
	    (action == IPR_VERIFY_CONC_ADD)) {
		elem_status->select = 1;
		elem_status->identify = 1;
	} else if (action == IPR_WAIT_CONC_REMOVE) {
		elem_status->select = 1;
		elem_status->remove = 1;
		elem_status->identify = 1;
	} else if (action == IPR_WAIT_CONC_ADD) {
		elem_status->select = 1;
		elem_status->insert = 1;
		elem_status->identify = 1;
		elem_status->enable_byp = 0;
	}

	overall = ipr_get_overall_elem(&ses_data, &ses_cfg);
	overall->select = 1;
	overall->insert = 0;
	overall->remove = 0;
	overall->identify = 0;

	if (action == IPR_WAIT_CONC_REMOVE || action == IPR_WAIT_CONC_ADD)
		overall->disable_resets = 1;

	if (action == IPR_VERIFY_CONC_REMOVE)
		n_screen = &n_verify_conc_remove;
	else if (action == IPR_VERIFY_CONC_ADD)
		n_screen = &n_verify_conc_add;
	else if (action == IPR_WAIT_CONC_REMOVE)
		n_screen = &n_wait_conc_remove;
	else
		n_screen = &n_wait_conc_add;

	processing();
	for (k = 0; k < 3; k++) {
		buffer[k] = __body_init_status(n_screen->header, &header_lines, (k + 8));
		buffer[k] = print_device(dev, buffer[k], "1", (k + 5));
	}

	rc = ipr_send_diagnostics(ses, &ses_data, ntohs(ses_data.byte_count) + 4);

	if (rc) {
		for (k = 0; k < 3; k++) {
			free(buffer[k]);
			buffer[k] = NULL;
		}
		return 30 | EXIT_FLAG;
	}

	/* call screen driver */
	do {
		n_screen->body = buffer[toggle%3];
		s_out = screen_driver(n_screen, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 3; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_screen->body = NULL;

	if (ioa->sis64)
		elem_status = get_elem_status_64bit(dev, ses, &ses_data, &ses_cfg);
	else
		elem_status = get_elem_status(dev, ses, &ses_data, &ses_cfg);
	if (!elem_status)
		return 30 | EXIT_FLAG;;

	/* turn light off flashing light */
	overall = ipr_get_overall_elem(&ses_data, &ses_cfg);
	overall->select = 1;
	overall->disable_resets = 0;
	overall->insert = 0;
	overall->remove = 0;
	overall->identify = 0;
	elem_status->select = 1;
	elem_status->insert = 0;
	elem_status->remove = 0;
	elem_status->identify = 0;

	ipr_send_diagnostics(ses, &ses_data, ntohs(ses_data.byte_count) + 4);

	/* call function to complete conc maint */
	if (!rc) {
		if (action == IPR_VERIFY_CONC_REMOVE) {
			rc = process_conc_maint(i_con, IPR_WAIT_CONC_REMOVE);
			if (!rc) {
				dev_rcd = dev->dev_rcd;
				getmaxyx(stdscr,max_y,max_x);
				move(max_y-1,0);
				printw(_("Operation in progress - please wait"));
				refresh();

				if (dev->ioa->sis64) {
					if (!remove_or_add_back_device_64bit(dev)) {
						clear();
						move(max_y/2,0);
						printw(_("    Disk was not removed!") );
						refresh();
						sleep(5);
					}
				}
				else {
					ipr_write_dev_attr(dev, "delete", "1");
					evaluate_device(dev, dev->ioa, 0);
					ipr_del_zeroed_dev(dev);
				}
			}
		} else if (action == IPR_VERIFY_CONC_ADD) {
			rc = process_conc_maint(i_con, IPR_WAIT_CONC_ADD);
			if (!rc) {
				getmaxyx(stdscr,max_y,max_x);
				move(max_y-1,0);
				printw(_("Operation in progress - please wait"));
				refresh();
				if (ioa->sis64)
					wait_for_new_dev_64bit(ioa, res_path);
				else
					wait_for_new_dev(ioa, &res_addr);
			}
		}
	}

	return rc;
}

/**
 * format_in_prog - check to determine if format is in progress
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if format is not in progress / 1 if format is in progress
 **/
static int format_in_prog(struct ipr_dev *dev)
{
	struct ipr_cmd_status cmd_status;
	struct sense_data_t sense_data;
	int rc;

	if (ipr_is_af_dasd_device(dev)) {
		rc = ipr_query_command_status(dev, &cmd_status);

		if (!rc && cmd_status.num_records != 0 &&
		    cmd_status.record->status == IPR_CMD_STATUS_IN_PROGRESS)
			return 1;
	} else if (ipr_is_gscsi(dev)) {
		rc = ipr_test_unit_ready(dev, &sense_data);

		if (rc == CHECK_CONDITION &&
		    (sense_data.error_code & 0x7F) == 0x70 &&
		    (sense_data.sense_key & 0x0F) == 0x02 &&  /* NOT READY */
		    sense_data.add_sense_code == 0x04 &&      /* LOGICAL UNIT NOT READY */
		    sense_data.add_sense_code_qual == 0x04)   /* FORMAT IN PROGRESS */
			return 1;
	}

	return 0;
}

/**
 * free_empty_slot - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
static void free_empty_slot(struct ipr_dev *dev)
{
	if (!dev->scsi_dev_data)
		return;
	if (dev->scsi_dev_data->type != IPR_TYPE_EMPTY_SLOT)
		return;
	free(dev->scsi_dev_data);
	dev->scsi_dev_data = NULL;
	free(dev);
}

/**
 * free_empty_slots - 
 * @dev:		ipr dev struct
 * @num:		number of slots to free
 *
 * Returns:
 *   nothing
 **/
static void free_empty_slots(struct ipr_dev **dev, int num)
{
	int i;

	for (i = 0; i < num; i++) {
		if (!dev[i] || !dev[i]->scsi_dev_data)
			continue;
		if (dev[i]->scsi_dev_data->type != IPR_TYPE_EMPTY_SLOT)
			continue;
		free_empty_slot(dev[i]);
		dev[i] = NULL;
	}
}

/**
 * get_res_addrs -
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0
 **/
static int get_res_addrs(struct ipr_dev *dev)
{
	struct ipr_res_addr *ra;
	struct ipr_res_addr_aliases aliases;
	int i = 0;

	ipr_query_res_addr_aliases(dev->ioa, &(dev->res_addr[0]), &aliases);

	for_each_ra_alias(ra, &aliases) {
		memcpy(&(dev->res_addr[i]), ra, sizeof(*ra));
		if (++i >= IPR_DEV_MAX_PATHS)
			break;
	}

	scsi_dbg(dev, "Resource address aliases: %08X %08X\n",
		 IPR_GET_PHYSICAL_LOCATOR(&(dev->res_addr[0])),
		 IPR_GET_PHYSICAL_LOCATOR(&(dev->res_addr[1])));

	return 0;
}

/**
 * get_res_path -
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0
 **/
static int get_res_path(struct ipr_dev *dev)
{
	struct ipr_res_path *rp;
	struct ipr_res_path_aliases aliases;
	int i = 0;

	ipr_query_res_path_aliases(dev->ioa, &(dev->res_path[0]), &aliases);

	for_each_rp_alias(rp, &aliases) {
		memcpy(&(dev->res_path[i]), rp, sizeof(*rp));
		if (++i >= IPR_DEV_MAX_PATHS)
			break;
	}

	while (i < IPR_DEV_MAX_PATHS)
		memset(&dev->res_path[i++], 0xff, sizeof(struct ipr_res_path));

	return 0;
}

/**
 * alloc_empty_slot -
 * @ses:		ipr dev struct
 * @slot:		slot number
 * @is_vses:		vses flag
 *
 * Returns:
 *   ipr_dev struct
 **/
static struct ipr_dev *alloc_empty_slot(struct ipr_dev *ses, int slot, int is_vses, char *phy_loc)
{
	struct ipr_ioa *ioa = ses->ioa;
	struct ipr_dev *dev;
	struct scsi_dev_data *scsi_dev_data;
	int bus = ses->scsi_dev_data->channel;
	int box;

	if (ioa->hop_count == IPR_2BIT_HOP)
		box = ses->scsi_dev_data->phy_2bit_hop.box;
	else
		box = ses->scsi_dev_data->phy.box;

	dev = calloc(1, sizeof(*dev));
	scsi_dev_data = calloc(1, sizeof(*scsi_dev_data));
	scsi_dev_data->type = IPR_TYPE_EMPTY_SLOT;
	scsi_dev_data->host = ioa->host_num;
	scsi_dev_data->lun = 0;

	if (is_vses) {
		scsi_dev_data->channel = slot;
		scsi_dev_data->id = 0;
	} else {
		scsi_dev_data->channel = bus;
		if (ioa->hop_count == IPR_2BIT_HOP)
			scsi_dev_data->id = slot | (box << 6);
		else
			scsi_dev_data->id = slot | (box << 5);
	}

	dev->res_addr[0].bus = scsi_dev_data->channel;
	dev->res_addr[0].target = scsi_dev_data->id;
	dev->res_addr[0].lun = 0;

	dev->scsi_dev_data = scsi_dev_data;
	dev->ses[0] = ses;
	dev->ioa = ioa;
	dev->physical_location[0] = '\0';
	strncat(dev->physical_location, phy_loc, strlen(phy_loc));
	get_res_addrs(dev);
	return dev;
}

/**
* alloc_empty_slot_64bit -
* @ses:		ipr dev struct
* @slot:		slot number
* @is_vses:		vses flag
*
* Returns:
*   ipr_dev struct
**/
static struct ipr_dev *alloc_empty_slot_64bit(struct ipr_dev *ses, int slot, int is_vses, char *phy_loc)
{
	struct ipr_ioa *ioa = ses->ioa;
	struct ipr_dev *dev;
	struct scsi_dev_data *scsi_dev_data;
	char slot_buf[50];
	int n, len;

	dev = calloc(1, sizeof(*dev));
	scsi_dev_data = calloc(1, sizeof(*scsi_dev_data));
	scsi_dev_data->type = IPR_TYPE_EMPTY_SLOT;
	scsi_dev_data->host = ioa->host_num;

	n = sprintf(slot_buf, "%02X",slot);

	strncpy(scsi_dev_data->res_path, ses->scsi_dev_data->res_path, strlen(ses->scsi_dev_data->res_path));

	scsi_dev_data->res_path[strlen(ses->scsi_dev_data->res_path) -2] = slot_buf[0];
	scsi_dev_data->res_path[strlen(ses->scsi_dev_data->res_path) -1] = slot_buf[1];

	strncpy(dev->res_path_name, scsi_dev_data->res_path, strlen(scsi_dev_data->res_path));
	len = strlen(ses->scsi_dev_data->res_path);
	memcpy(&dev->res_path[0], &ses->res_path[0], sizeof(struct ipr_res_path));
	dev->res_path[0].res_path_bytes[len/3] = slot;

	dev->scsi_dev_data = scsi_dev_data;
	dev->ses[0] = ses;
	dev->ioa = ioa;
	dev->physical_location[0] = '\0';
	strncat(dev->physical_location, phy_loc, strlen(phy_loc));
	get_res_path(dev);
	return dev;
}

/**
 * can_perform_conc_action -
 * @dev:		ipr dev struct
 * @action:		action type
 *
 * Returns:
 *   1 if able to perform the action / 0 otherwise
 **/
static int can_perform_conc_action(struct ipr_dev *dev, int action)
{
	if (action == IPR_CONC_REMOVE) {
		if (format_in_prog(dev))
			return 0;
		if (!ipr_can_remove_device(dev))
			return 0;
	} else if (dev->scsi_dev_data && action != IPR_CONC_IDENTIFY)
		return 0;
	return 1;
}

/**
 * get_dev_for_slot -
 * @ses:		ipr dev struct
 * @slot:		slot number
 * @is_vses:		action type
 *
 * Returns:
 *   ipr_dev struct if success / NULL otherwise
 **/
static struct ipr_dev *get_dev_for_slot(struct ipr_dev *ses, int slot, int is_vses, char *phy_loc)
{
	struct ipr_ioa *ioa = ses->ioa;
	struct ipr_dev *dev;
	struct ipr_res_addr *ra;
	int bus = ses->scsi_dev_data->channel;
	int box, i;

	if (ioa->hop_count == IPR_2BIT_HOP)
		box = ses->scsi_dev_data->phy_2bit_hop.box;
	else
		box = ses->scsi_dev_data->phy.box;

	for_each_dev(ses->ioa, dev) {
		if (ipr_is_ses(dev))
			continue;

		for_each_ra(ra, dev) {
			if (is_vses && (ra->bus != slot || ra->target != 0))
				continue;
			if (ioa->hop_count == IPR_2BIT_HOP) {
				if (!is_vses && (ra->bus != bus ||
						 ra->phy_2bit_hop.box != box ||
						 ra->phy_2bit_hop.phy != slot))
					continue;
			} else {
				if (!is_vses && (ra->bus != bus ||
						 ra->phy.box != box ||
						 ra->phy.phy != slot))
					continue;
			}
			for (i = 0; i < IPR_DEV_MAX_PATHS; i++) {
				if (dev->ses[i])
					continue;
				dev->ses[i] = ses;
			}
			dev->physical_location[0] = '\0';
			if (strlen(phy_loc))
				strncat(dev->physical_location, phy_loc, strlen(phy_loc));
			return dev;
		}
	}

	return NULL;
}

/**
 * get_dev_for_slot_64bit -
 * @ses:		ipr dev struct
 * @slot:		slot number
 * @is_vses:		action type
 *
 * Returns:
 *   ipr_dev struct if success / NULL otherwise
 **/
static struct ipr_dev *get_dev_for_slot_64bit(struct ipr_dev *ses, int slot, char *phy_loc)
{
	struct ipr_dev *dev;
	struct ipr_res_path *rp;
	int i, dev_slot, res_path_len,ses_path_len;

	for_each_dev(ses->ioa, dev) {
		if (ipr_is_ses(dev))
			continue;

		ses_path_len  = strlen(ses->res_path_name);
		res_path_len  = strlen(dev->res_path_name);
		dev_slot = strtoul(dev->res_path_name + (res_path_len - 2), NULL, 16);

		if (ses_path_len != res_path_len)
			continue;

		if (ipr_is_volume_set(dev) || ipr_is_array(dev))
			continue;

		for_each_rp(rp, dev) {
			if ( !memcmp(&rp->res_path_bytes, &ses->res_path[0], ses_path_len/3) && slot == dev_slot ) {

				for (i = 0; i < IPR_DEV_MAX_PATHS; i++) {
					if (dev->ses[i])
						continue;
					dev->ses[i] = ses;
					break;
				}
			dev->physical_location[0] = '\0';
			if (strlen(phy_loc))
				strncat(dev->physical_location, phy_loc, strlen(phy_loc));
			return dev;

			}
		}
	}
	return NULL;
}

/**
 * search_empty_dev64 -
 * @dev:		ipr dev struct
 * @devs:		ipr dev struct array
 * @num_devs:		number of devices
 *
 * Returns:
 *   1 if duplicate device detected / 0 otherwise
 **/
static int search_empty_dev64(struct ipr_dev *dev, struct ipr_dev **devs, int num_devs)
{
	int i;
	u8 empty_res_path[8];

	memset(&empty_res_path, 0xff, sizeof(struct ipr_res_path));
	if (memcmp(&dev->res_path[1], &empty_res_path, sizeof(struct ipr_res_path)))
		return 0;

	for (i = 0; i < num_devs; i++) {
		if (devs[i]->scsi_dev_data && devs[i]->scsi_dev_data->type == IPR_TYPE_EMPTY_SLOT &&
			!strncmp(dev->physical_location, devs[i]->physical_location, PHYSICAL_LOCATION_LENGTH)) {

			memcpy(&devs[i]->res_path[1], &dev->res_path[0], sizeof(struct ipr_res_path));
			devs[i]->ses[1] = dev->ses[0];
			return 1;
		}
	}
	return 0;
}

/**
 * conc_dev_is_dup -
 * @dev:		ipr dev struct
 * @devs:		ipr dev struct array
 * @num_devs:		number of devices
 *
 * Returns:
 *   1 if duplicate device detected / 0 otherwise
 **/
static int conc_dev_is_dup(struct ipr_dev *dev, struct ipr_dev **devs, int num_devs)
{
	int i;

	for (i = 0; i < num_devs; i++)
		if (dev == devs[i])
			return 1;
	return 0;
}

/**
 * dev_is_dup - indicate whether or not the devices are the same
 * @first:		ipr dev struct
 * @second:		ipr dev struct
 *
 * Returns:
 *   1 if devices are the same / 0 otherwise
 **/
static int dev_is_dup(struct ipr_dev *first, struct ipr_dev *second)
{
	struct ipr_res_addr *first_ra;
	struct ipr_res_addr *second_ra;

	if (first->ioa != second->ioa)
		return 0;

	for_each_ra(first_ra, first) {
		for_each_ra(second_ra, second) {
			if (!memcmp(first_ra, second_ra, sizeof(*first_ra)))
				return 1;
		}
	}

	return 0;
}

/**
 * dev_is_dup64 - indicate whether or not the devices are the same
 * @first:		ipr dev struct
 * @second:		ipr dev struct
 *
 * Returns:
 *   1 if devices are the same / 0 otherwise
 **/
static int dev_is_dup64(struct ipr_dev *first, struct ipr_dev *second)
{
	struct ipr_res_path *first_rp;
	struct ipr_res_path *second_rp;
	struct ipr_res_path no_exist_rp;

	if (first->ioa != second->ioa)
		return 0;

	memset(&no_exist_rp, 0xff, sizeof(struct ipr_res_path));

	for_each_rp(first_rp, first) {
		for_each_rp(second_rp, second) {
			if (!memcmp(first_rp, &no_exist_rp, sizeof(*first_rp))
			|| !memcmp(second_rp, &no_exist_rp, sizeof(*second_rp)))
				continue;

			if (!memcmp(first_rp, second_rp, sizeof(*first_rp)))
				return 1;
		}
	}

	return 0;
}

/**
 * find_dup -
 * @dev:		ipr dev struct
 * @devs:		ipr dev struct array
 * @num_devs:		number of devices
 *
 * Returns:
 *   ipr_dev struct if duplicate found / NULL otherwise
 **/
static struct ipr_dev *find_dup(struct ipr_dev *dev, struct ipr_dev **devs, int num_devs)
{
	int i;

	for (i = 0; i < num_devs; i++) {
		if (dev->ioa->sis64) {
			if (dev_is_dup64(dev, devs[i]))
				return devs[i];
		}
		else {
			if (dev_is_dup(dev, devs[i]))
				return devs[i];
		}
	}

	return NULL;
}

/**
 * remove_dup_dev -
 * @dev:		ipr dev struct
 * @devs:		ipr dev struct array
 * @num_devs:		number of devices
 *
 * Returns:
 *   num_devs
 **/
static int remove_dup_dev(struct ipr_dev *dev, struct ipr_dev **devs, int num_devs)
{
	int i, j;

	for (i = 0; i < num_devs; i++) {
		if (dev == devs[i]) {
			free_empty_slot(dev);
			devs[i] = NULL;
			for (j = (i + 1); j < num_devs; j++) {
				devs[j - 1] = devs[j];
				devs[j] = NULL;
			}
			return (num_devs - 1);
		}
	}

	return num_devs;
}

/**
 * remove_conc_dups -
 * @devs:		ipr dev struct array
 * @num_devs:		number of devices
 *
 * Returns:
 *   num_devs
 **/
static int remove_conc_dups(struct ipr_dev **devs, int num_devs)
{
	int i;
	struct ipr_dev *dev;

	for (i = 0; i < num_devs; i++) {
		dev = find_dup(devs[i], &devs[i+1], (num_devs - i - 1));
		if (!dev)
			continue;
		num_devs = remove_dup_dev(dev, devs, num_devs);
	}

	return num_devs;
}

/**
 * get_conc_devs -
 * @ret:		ipr dev struct 
 * @action:		action typoe
 *
 * Returns:
 *   num)devs
 **/
static int get_conc_devs(struct ipr_dev ***ret, int action)
{
	struct ipr_ioa *ioa;
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status, *overall;
	struct ipr_dev **devs = NULL;
	struct ipr_dev *ses, *dev;
	int num_devs = 0;
	int ses_bus, scsi_id_found, is_spi, is_vses;
	struct ipr_ses_config_pg ses_cfg;
	struct drive_elem_desc_pg drive_data;
	char phy_loc[PHYSICAL_LOCATION_LENGTH + 1];
	int times, index;

	for_each_primary_ioa(ioa) {
		is_spi = ioa_is_spi(ioa);

		for_each_hotplug_dev(ioa, dev) {
			if (ioa->sis64)
				get_res_path(dev);
			else
				get_res_addrs(dev);
		}

		for_each_ses(ioa, ses) {
			times = 5;
			if (strlen(ses->physical_location) == 0)
				get_ses_phy_loc(ses);

			while (times--) {
				if (!ipr_receive_diagnostics(ses, 2, &ses_data, sizeof(ses_data)))
					break;
			}
			if (times < 0 ) continue;

			if (ipr_receive_diagnostics(ses, 1, &ses_cfg, sizeof(ses_cfg)))
				continue;
			if (ipr_receive_diagnostics(ses, 7, &drive_data, sizeof(drive_data)))
				continue;

			overall = ipr_get_overall_elem(&ses_data, &ses_cfg);
			ses_bus = ses->scsi_dev_data->channel;
			scsi_id_found = 0;

			if (!is_spi && (overall->device_environment == 0))
				is_vses = 1;
			else
				is_vses = 0;

			scsi_dbg(ses, "%s\n", is_vses ? "Found VSES" : "Found real SES");

			for_each_elem_status(elem_status, &ses_data, &ses_cfg) {
				index = index_in_page2(&ses_data, elem_status->slot_id);
				if (index != -1)
					get_drive_phy_loc_with_ses_phy_loc(ses, &drive_data, index, phy_loc, 0);
				
				if (elem_status->status == IPR_DRIVE_ELEM_STATUS_UNSUPP)
					continue;
				if (elem_status->status == IPR_DRIVE_ELEM_STATUS_NO_ACCESS)
					continue;
				if (is_spi && (scsi_id_found & (1 << elem_status->slot_id)))
					continue;
				scsi_id_found |= (1 << elem_status->slot_id);

				if (elem_status->status == IPR_DRIVE_ELEM_STATUS_EMPTY) {
					if (ioa->sis64) {
						dev = alloc_empty_slot_64bit(ses, elem_status->slot_id, is_vses, phy_loc);
						if (!search_empty_dev64(dev, devs, num_devs)){
							devs = realloc(devs, (sizeof(struct ipr_dev *) * (num_devs + 1)));
							devs[num_devs++] = dev;
						}
						else
							free_empty_slot(dev);
					}
					else {
						devs = realloc(devs, (sizeof(struct ipr_dev *) * (num_devs + 1)));
						devs[num_devs++] = alloc_empty_slot(ses, elem_status->slot_id, is_vses, phy_loc);
					}
					continue;
				}

				if (ioa->sis64)
					dev = get_dev_for_slot_64bit(ses, elem_status->slot_id, phy_loc);
				else
					dev = get_dev_for_slot(ses, elem_status->slot_id, is_vses, phy_loc);

				if (dev && !can_perform_conc_action(dev, action))
					continue;
				if (dev && conc_dev_is_dup(dev, devs, num_devs))
					continue;
				else if (!dev) {
					if (ioa->sis64)
						dev = alloc_empty_slot_64bit(ses, elem_status->slot_id, is_vses, phy_loc);
					else
						dev = alloc_empty_slot(ses, elem_status->slot_id, is_vses, phy_loc);
				}

				devs = realloc(devs, (sizeof(struct ipr_dev *) * (num_devs + 1)));
				devs[num_devs++] = dev;
			}
		}
	}

	num_devs = remove_conc_dups(devs, num_devs);
	*ret = devs;
	return num_devs;  
}

/**
 * start_conc_maint -
 * @i_con:		i_container struct
 * @action:		action type
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int start_conc_maint(i_container *i_con, int action)
{
	int rc, s_rc, i, k;
	char *buffer[3];
	int num_lines = 0;
	struct screen_output *s_out;
	struct ipr_dev **devs;
	int toggle = 0;
	s_node *n_screen;
	int header_lines;
	int num_devs;

	processing();

	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);

	/* Setup screen title */
	if (action == IPR_CONC_REMOVE)
		n_screen = &n_concurrent_remove_device;
	else
		n_screen = &n_concurrent_add_device;

	body_init_status_conc(buffer, n_screen->header, &header_lines);

	num_devs = get_conc_devs(&devs, action);

	for (i = 0; i < num_devs; i++) {
		print_dev_conc(k, devs[i], buffer, "%1", k);
		i_con = add_i_con(i_con,"\0", devs[i]);
		num_lines++;
	}

	if (num_lines == 0) {
		for (k = 0; k < 3; k++)
			buffer[k] = add_string_to_body(buffer[k], _("(No Eligible Devices Found)"),
						     "", NULL);
	}

	do {
		n_screen->body = buffer[toggle%3];
		s_out = screen_driver(n_screen, header_lines, i_con);
		s_rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (s_rc == TOGGLE_SCREEN);

	for (k = 0; k < 3; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_screen->body = NULL;

	if (!s_rc) {
		if (action == IPR_CONC_REMOVE)
			rc = process_conc_maint(i_con, IPR_VERIFY_CONC_REMOVE);
		else
			rc = process_conc_maint(i_con, IPR_VERIFY_CONC_ADD);
	}

	if (rc & CANCEL_FLAG) {
		s_status.index = (rc & ~CANCEL_FLAG);
		rc = (rc | REFRESH_FLAG) & ~CANCEL_FLAG;
	}

	free_empty_slots(devs, num_devs);
	free(devs);
	return rc;  
}

/**
 * concurrent_add_device -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int concurrent_add_device(i_container *i_con)
{
	return start_conc_maint(i_con, IPR_CONC_ADD);
}

/**
 * concurrent_remove_device -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int concurrent_remove_device(i_container *i_con)
{
	return start_conc_maint(i_con, IPR_CONC_REMOVE);
}

/**
 * path_details -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int path_details(i_container *i_con)
{
	int rc, header_lines = 2;
	char *body = NULL;
	struct ipr_dev *dev;
	struct screen_output *s_out;
	char location[512];
	char *name;

	rc = RC_SUCCESS;

	processing();

	if ((rc = device_details_get_device(i_con, &dev)))
		return rc;

	if (strlen(dev->dev_name))
		name = dev->dev_name;
	else
		name = dev->gen_name;

	if (dev->ioa->sis64)
		sprintf(location, "%d/%s", dev->ioa->host_num,dev->scsi_dev_data->res_path);
	else
		sprintf(location, "%s/%d:%d:%d:%d", dev->ioa->pci_address, dev->ioa->host_num,
			dev->res_addr[0].bus, dev->res_addr[0].target, dev->res_addr[0].lun);
	body = add_line_to_body(body, _("Device"), name);
	if (strlen(dev->physical_location))
		body = add_line_to_body(body, _("Location"), dev->physical_location);
	else
		body = add_line_to_body(body, _("Location"), location);
	body = __body_init(body, n_path_details.header, &header_lines);
	body = status_header(body, &header_lines, 6);

	body = print_path_details(dev, body);

	n_path_details.body = body;
	s_out = screen_driver(&n_path_details, header_lines, i_con);
	free(n_path_details.body);
	n_path_details.body = NULL;
	rc = s_out->rc;
	free(s_out);
	return rc;
}

/**
 * path_status -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int path_status(i_container * i_con)
{
	int rc, k, header_lines;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct screen_output *s_out;
	int toggle = 1, num_devs = 0;

	rc = RC_SUCCESS;

	processing();

	i_con = free_i_con(i_con);

	check_current_config(false);

	for (k = 0; k < 2; k++)
		buffer[k] = __body_init_status(n_path_status.header, &header_lines, k);

	for_each_sas_ioa(ioa) {
		if (ioa->ioa_dead)
			continue;

		for_each_disk(ioa, dev) {
			buffer[0] = __print_device(dev, buffer[0], "%1", 1, 1, 1, 0, 0, 1, 0, 1, 0 ,0, 0, 0, 0, 0, 0);
			buffer[1] = __print_device(dev, buffer[1], "%1", 1, 1, 0, 0, 0, 0, 0, 1, 0 ,0, 0, 0, 0, 0, 0);
			i_con = add_i_con(i_con, "\0", dev);
			num_devs++;
		}
	}

	if (!num_devs)
		/* "No SAS disks available" */
		return 76; 

	do {
		n_path_status.body = buffer[toggle&1];
		s_out = screen_driver(&n_path_status, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_path_status.body = NULL;
	return rc;
}

/**
 * init_device -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int init_device(i_container *i_con)
{
	int rc, k;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_dev_record *device_record;
	int can_init;
	int dev_type;
	char *buffer[2];
	int num_devs = 0;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct screen_output *s_out;
	int header_lines;
	int toggle = 0;

	rc = RC_SUCCESS;

	processing();

	i_con = free_i_con(i_con);

	check_current_config(false);

	body_init_status(buffer, n_init_device.header, &header_lines);

	for_each_primary_ioa(ioa) {
		if (ioa->ioa_dead)
			continue;

		for_each_dev(ioa, dev) {
			can_init = 1;
			scsi_dev_data = dev->scsi_dev_data;

			/* If not a DASD, disallow format */
			if (!scsi_dev_data || ipr_is_hot_spare(dev) ||
			    ipr_is_volume_set(dev) || !device_supported(dev) ||
			    (scsi_dev_data->type != TYPE_DISK &&
			     scsi_dev_data->type != IPR_TYPE_AF_DISK))
				continue;

			/* If Advanced Function DASD */
			if (ipr_is_af_dasd_device(dev)) {
				dev_type = IPR_AF_DASD_DEVICE;
				device_record = dev->dev_rcd;

				/* We allow the user to format the drive if nobody is using it */
				if (scsi_dev_data->opens != 0) {
					syslog(LOG_ERR,
					       _("Format not allowed to %s, device in use\n"),
					       dev->gen_name); 
					continue;
				}

				if (ipr_is_array_member(dev)) {
					if (device_record->no_cfgte_vol)
						can_init = 1;
					else
						can_init = 0;
				} else
					can_init = is_format_allowed(dev);
			} else if (scsi_dev_data->type == TYPE_DISK) {
				/* We allow the user to format the drive if nobody is using it */
				if (scsi_dev_data->opens != 0) {
					syslog(LOG_ERR,
					       _("Format not allowed to %s, device in use\n"),
					       dev->gen_name); 
					continue;
				}

				dev_type = IPR_JBOD_DASD_DEVICE;
				can_init = is_format_allowed(dev);
			} else
				continue;

			if (can_init) {
				add_format_device(dev, 0);
				print_dev(k, dev, buffer, "%1", k);
				i_con = add_i_con(i_con,"\0",dev_init_tail);
				num_devs++;
			}
		}
	}


	if (!num_devs)
		/* "No units available for initialize and format" */
		return 49; 

	do {
		n_init_device.body = buffer[toggle&1];
		s_out = screen_driver(&n_init_device, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_init_device.body = NULL;
	free_devs_to_init();
	return rc;
}

/**
 * confirm_init_device -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int confirm_init_device(i_container *i_con)
{
	char *input;
	char *buffer[2];
	struct devs_to_init_t *cur_dev_init;
	int rc = RC_SUCCESS;
	struct screen_output *s_out;
	int header_lines = 0;
	int toggle = 0;
	int k;
	i_container *temp_i_con;
	int found = 0;
	int post_attention = 0;
	struct ipr_dev *dev;

	for_each_icon(temp_i_con) {
		if (temp_i_con->data == NULL)
			continue;

		input = temp_i_con->field_data;        
		if (strcmp(input, "1") == 0) {
			found++;
			cur_dev_init =(struct devs_to_init_t *)(temp_i_con->data);
			cur_dev_init->do_init = 1;
			dev = cur_dev_init->dev;

			if (ipr_is_gscsi(dev))
				post_attention++;
		} else {
			cur_dev_init = (struct devs_to_init_t *)(temp_i_con->data);
			cur_dev_init->do_init = 0;
		}
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	if (post_attention) {
		for (k = 0; k < 2; k++) {
			header_lines = 0;
			buffer[k] = add_string_to_body(NULL, _("ATTENTION!  System crash may occur "
							       "if selected device is in use. Data loss will "
							       "occur on selected device.  Proceed with "
							       "caution.\n\n"), "", &header_lines);
			buffer[k] = __body_init(buffer[k],n_confirm_init_device.header, &header_lines);
			buffer[k] = status_header(buffer[k], &header_lines, k);
		}
	} else {
		body_init_status(buffer, n_confirm_init_device.header, &header_lines);
	}

	for_each_dev_to_init(cur_dev_init) {
		if (!cur_dev_init->do_init)
			continue;

		print_dev(k, cur_dev_init->dev, buffer, "1", k);
	}

	do {
		n_confirm_init_device.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_init_device, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_init_device.body = NULL;

	return rc;
}

/**
 * dev_init_complete -
 * @num_devs:		number of devices
 *
 * Returns:
 *   50 | EXIT_FLAG if success /  51 | EXIT_FLAG on failure
 **/
static int dev_init_complete(u8 num_devs)
{
	int done_bad = 0;
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int not_done = 0;
	int rc = 0;
	struct devs_to_init_t *dev;
	u32 percent_cmplt = 0;
	struct sense_data_t sense_data;
	struct ipr_ioa *ioa;
	int pid = 1;
	char dev_type[100];
	int type;

	while(1) {
		if (pid)
			rc = complete_screen_driver(&n_dev_init_complete, percent_cmplt, 0);

		if (rc & EXIT_FLAG) {
			pid = fork();
			if (pid)
				return rc;
			rc = 0;
		}

		percent_cmplt = 100;

		for_each_dev_to_init(dev) {
			if (!dev->do_init || dev->done)
				continue;

			if (ipr_read_dev_attr(dev->dev, "type",
					      dev_type, 100)) {
				dev->done = 1;
				continue;
			}

			if (sscanf(dev_type, "%d", &type) != 1) {
				dev->done = 1;
				continue;
			}

			if (dev->dev_type == IPR_AF_DASD_DEVICE) {
				if (type != IPR_TYPE_AF_DISK) {
					dev->done = 1;
					continue;
				}

				rc = ipr_query_command_status(dev->dev, &cmd_status);

				if (rc || cmd_status.num_records == 0) {
					dev->done = 1;
					continue;
				}

				status_record = cmd_status.record;
				if (status_record->status == IPR_CMD_STATUS_FAILED) {
					done_bad = 1;
					dev->done = 1;
				} else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL) {
					if (status_record->percent_complete < percent_cmplt)
						percent_cmplt = status_record->percent_complete;
					not_done = 1;
				} else
					dev->done = 1;
			} else if (dev->dev_type == IPR_JBOD_DASD_DEVICE) {
				if (type != TYPE_DISK) {
					dev->done = 1;
					continue;
				}

				/* Send Test Unit Ready to find percent complete in sense data. */
				rc = ipr_test_unit_ready(dev->dev,
							 &sense_data);

				if (rc < 0) {
					dev->done = 1;
					done_bad = 1;
					continue;
				}

				if (rc == CHECK_CONDITION &&
				    (sense_data.error_code & 0x7F) == 0x70 &&
				    (sense_data.sense_key & 0x0F) == 0x02 &&
				    sense_data.add_sense_code == 0x04 &&
				    sense_data.add_sense_code_qual == 0x04) {
					dev->cmplt = ((int)sense_data.sense_key_spec[1]*100)/0x100;
					if (dev->cmplt < percent_cmplt)
						percent_cmplt = dev->cmplt;
					not_done = 1;
				} else
					dev->done = 1;
			}
		}

		if (!not_done) {
			for_each_dev_to_init(dev) {
				if (!dev->do_init)
					continue;

				ioa = dev->ioa;
				if (ipr_is_gscsi(dev->dev)) {
					rc = ipr_test_unit_ready(dev->dev, &sense_data);
					if (rc) {
						done_bad = 1;
					} else if (!ipr_is_af_blk_size(ioa, dev->new_block_size)) {
						ipr_write_dev_attr(dev->dev, "rescan", "1");
						ipr_init_dev(dev->dev);
					}
				}

				if (dev->new_block_size != 0) {
					if (ipr_is_af_blk_size(ioa, dev->new_block_size))
						enable_af(dev->dev);

					evaluate_device(dev->dev, ioa, dev->new_block_size);
				}

				ipr_device_unlock(dev->dev);

				if (ipr_is_af_blk_size(ioa, dev->new_block_size) || ipr_is_af_dasd_device(dev->dev))
					ipr_add_zeroed_dev(dev->dev);
			}

			flush_stdscr();

			if (done_bad) {
				if (!pid)
					exit(0);

				/* Initialize and format failed */
				return 51 | EXIT_FLAG;
			}

			format_done = 1;
			wait_for_formatted_af_dasd(30);
			check_current_config(false);

			if (!pid)
				exit(0);

			/* Initialize and format completed successfully */
			return 50 | EXIT_FLAG; 
		}

		not_done = 0;
		sleep(1);
	}
}

/**
 * send_dev_inits -
 * @i_con:		i_container struct
 *
 * Returns:
 *   50 | EXIT_FLAG if success /  51 | EXIT_FLAG on failure
 **/
int send_dev_inits(i_container *i_con)
{
	u8 num_devs = 0;
	struct devs_to_init_t *cur_dev_init;
	int rc = 0;
	struct ipr_query_res_state res_state;
	u8 ioctl_buffer[IOCTL_BUFFER_SIZE];
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	struct ipr_block_desc *block_desc;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_ioa *ioa;
	int status;
	int opens;
	u8 failure = 0;
	int max_y, max_x;
	u8 length;

	if (use_curses) {
		getmaxyx(stdscr,max_y,max_x);
		mvaddstr(max_y-1,0,wait_for_next_screen);
		refresh();
	}

	for_each_dev_to_init(cur_dev_init) {
		if (cur_dev_init->do_init &&
		    cur_dev_init->dev_type == IPR_AF_DASD_DEVICE) {
			num_devs++;
			ioa = cur_dev_init->ioa;

			rc = ipr_query_resource_state(cur_dev_init->dev, &res_state);

			if (rc != 0) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			scsi_dev_data = cur_dev_init->dev->scsi_dev_data;
			if (!scsi_dev_data) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			opens = num_device_opens(scsi_dev_data->host,
						 scsi_dev_data->channel,
						 scsi_dev_data->id,
						 scsi_dev_data->lun);

			if ((opens != 0) && (!res_state.read_write_prot)) {
				syslog(LOG_ERR,
				       _("Cannot obtain exclusive access to %s\n"),
				       cur_dev_init->dev->gen_name);
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			ipr_disable_qerr(cur_dev_init->dev);

			if (cur_dev_init->new_block_size == IPR_JBOD_BLOCK_SIZE ||
				cur_dev_init->new_block_size == IPR_JBOD_4K_BLOCK_SIZE) {
				/* Issue mode select to change block size */
				mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
				memset(ioctl_buffer, 0, 255);

				mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);
				block_desc = (struct ipr_block_desc *)(mode_parm_hdr + 1);

				/* Setup block size */
				block_desc->block_length[0] = 0x00;
				block_desc->block_length[1] = cur_dev_init->new_block_size >> 8;
				block_desc->block_length[2] = cur_dev_init->new_block_size & 0xff;

				rc = ipr_mode_select(cur_dev_init->dev,ioctl_buffer,
						     sizeof(struct ipr_block_desc) +
						     sizeof(struct ipr_mode_parm_hdr));

				if (rc != 0) {
					cur_dev_init->do_init = 0;
					num_devs--;
					failure++;
					continue;
				}
			}

			/* Issue the format. Failure will be detected by query command status */
			rc = ipr_format_unit(cur_dev_init->dev);
		} else if (cur_dev_init->do_init &&
			   cur_dev_init->dev_type == IPR_JBOD_DASD_DEVICE) {
			num_devs++;
			ioa = cur_dev_init->ioa;
			scsi_dev_data = cur_dev_init->dev->scsi_dev_data;
			if (!scsi_dev_data) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			opens = num_device_opens(scsi_dev_data->host,
						 scsi_dev_data->channel,
						 scsi_dev_data->id,
						 scsi_dev_data->lun);

			if (opens) {
				syslog(LOG_ERR,
				       _("Cannot obtain exclusive access to %s\n"),
				       cur_dev_init->dev->gen_name);
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			ipr_disable_qerr(cur_dev_init->dev);

			/* Issue mode select to setup block size */
			mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
			memset(ioctl_buffer, 0, 255);

			mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);

			block_desc = (struct ipr_block_desc *)(mode_parm_hdr + 1);
			/* xxx Setup block size */
			if (cur_dev_init->new_block_size == ioa->af_block_size) {
				block_desc->block_length[0] = 0x00;
				block_desc->block_length[1] = ioa->af_block_size >> 8;
				block_desc->block_length[2] = ioa->af_block_size & 0xff;
			} else {
				block_desc->block_length[0] = 0x00;
				block_desc->block_length[1] = cur_dev_init->new_block_size >> 8;
				block_desc->block_length[2] = cur_dev_init->new_block_size & 0xff;
			}

			length = sizeof(struct ipr_block_desc) +
				sizeof(struct ipr_mode_parm_hdr);

			status = ipr_mode_select(cur_dev_init->dev,
						 &ioctl_buffer, length);

			if (status && ipr_is_af_blk_size(ioa, cur_dev_init->new_block_size)) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* unbind device */
			if (ipr_jbod_sysfs_bind(cur_dev_init->dev,
						IPR_JBOD_SYSFS_UNBIND))
				syslog(LOG_ERR, "Could not unbind %s: %m\n",
				       cur_dev_init->dev->dev_name);

			status = 0;

			if (ipr_is_af_blk_size(ioa, cur_dev_init->new_block_size))
				status = ipr_device_lock(cur_dev_init->dev);

			if (!status)
				status = ipr_format_unit(cur_dev_init->dev);

			if (status) {
				/* Send a device reset to cleanup any old state */
				rc = ipr_reset_device(cur_dev_init->dev);
				if (ipr_jbod_sysfs_bind(cur_dev_init->dev,
							IPR_JBOD_SYSFS_BIND))
					syslog(LOG_ERR,
					       "Could not bind %s: %m\n",
					       cur_dev_init->dev->dev_name);

				ipr_device_unlock(cur_dev_init->dev);
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}
		}
	}

	if (num_devs)
		rc = dev_init_complete(num_devs);

	if (failure == 0)
		return rc;
	else
		/* "Initialize and format failed" */
		return 51 | EXIT_FLAG; 
}

/**
 * reclaim_cache -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int reclaim_cache(i_container* i_con)
{
	int rc;
	struct ipr_ioa *ioa;
	struct ipr_reclaim_query_data *reclaim_buffer;
	struct ipr_reclaim_query_data *cur_reclaim_buffer;
	int found = 0;
	char *buffer[2];
	struct screen_output *s_out;
	int header_lines;
	int toggle=1;
	int k;

	processing();
	/* empty the linked list that contains field pointens */
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status(buffer, n_reclaim_cache.header, &header_lines);

	reclaim_buffer = malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

	for (ioa = ipr_ioa_head, cur_reclaim_buffer = reclaim_buffer;
	     ioa != NULL; ioa = ioa->next, cur_reclaim_buffer++) {
		rc = ipr_reclaim_cache_store(ioa,
					     IPR_RECLAIM_QUERY,
					     cur_reclaim_buffer);

		if (rc != 0) {
			ioa->reclaim_data = NULL;
			continue;
		}

		ioa->reclaim_data = cur_reclaim_buffer;

		if (cur_reclaim_buffer->reclaim_known_needed ||
		    cur_reclaim_buffer->reclaim_unknown_needed) {

			print_dev(k, &ioa->ioa, buffer, "%1", k); 
			i_con = add_i_con(i_con, "\0",ioa);
			found++;
		}
	}

	if (!found) {
		/* "No Reclaim IOA Cache Storage is necessary" */
		rc = 38;
		goto leave;
	} 

	do {
		n_reclaim_cache.body = buffer[toggle&1];
		s_out = screen_driver(&n_reclaim_cache, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

leave:

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_reclaim_cache.body = NULL;

	return rc;
}

/**
 * confirm_reclaim -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int confirm_reclaim(i_container *i_con)
{
	struct ipr_ioa *ioa, *reclaim_ioa;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_query_res_state res_state;
	struct ipr_dev *dev;
	int k, rc;
	char *buffer[2];
	i_container* temp_i_con;
	struct screen_output *s_out;
	int header_lines;
	int toggle=1;
	int ioa_num = 0;

	for_each_icon(temp_i_con) {
		ioa = (struct ipr_ioa *) temp_i_con->data;
		if (!ioa)
			continue;

		if (strcmp(temp_i_con->field_data, "1") != 0)
			continue;

		if (ioa->reclaim_data->reclaim_known_needed ||
		    ioa->reclaim_data->reclaim_unknown_needed) {
			reclaim_ioa = ioa;
			ioa_num++;
		}
	}

	if (ioa_num == 0)
		/* "Invalid option.  No devices selected." */
		return 17; 
	else if (ioa_num > 1)
		/* "Error.  More than one unit was selected." */
		return 16; 

	/* One IOA selected, ready to proceed */
	body_init_status(buffer, n_confirm_reclaim.header, &header_lines);

	for_each_dev(reclaim_ioa, dev) {
		scsi_dev_data = dev->scsi_dev_data;
		if (!scsi_dev_data || !ipr_is_af(dev))
			continue;

		/* Do a query resource state to see whether or not the
		 device will be affected by the operation */
		rc = ipr_query_resource_state(dev, &res_state);

		if (rc != 0)
			res_state.not_oper = 1;

		if (!res_state.read_write_prot && !res_state.not_oper)
			continue;

		print_dev(k, dev, buffer, "1", k);
	}

	i_con = free_i_con(i_con);

	/* Save the chosen IOA for later use */
	add_i_con(i_con, "\0", reclaim_ioa);

	do {
		n_confirm_reclaim.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_reclaim, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_reclaim.body = NULL;

	return rc;
}

/**
 * reclaim_warning -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int reclaim_warning(i_container *i_con)
{
	struct ipr_ioa *reclaim_ioa;
	int k, rc;
	char *buffer[2];
	struct screen_output *s_out;
	int header_lines;
	int toggle=1;

	reclaim_ioa = (struct ipr_ioa *)i_con->data;
	if (!reclaim_ioa)
		/* "Invalid option.  No devices selected." */
		return 17; 

	for (k = 0; k < 2; k++) {
		buffer[k] = __body_init_status(n_confirm_reclaim_warning.header, &header_lines,  k);
		buffer[k] = print_device(&reclaim_ioa->ioa, buffer[k], "1", k);
	}

	do {
		n_confirm_reclaim_warning.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_reclaim_warning, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_reclaim_warning.body = NULL;

	return rc;
}

/**
 * get_reclaim_results -
 * @buf:		ipr_reclaim_query_data struct
 *
 * Returns:
 *   ipr_reclaim_results struct
 **/
static char *get_reclaim_results(struct ipr_reclaim_query_data *buf)
{
	char *body = NULL;
	char buffer[32];

	if (buf->reclaim_known_performed) {
		body = add_string_to_body(body,
					  _("IOA cache storage reclamation has completed. "
					    "Use the number of lost sectors to decide whether "
					    "to restore data from the most recent save media "
					    "or to continue with possible data loss.\n"),
					  "", NULL);

		if (buf->num_blocks_needs_multiplier)
			sprintf(buffer, "%12d", ntohs(buf->num_blocks) *
				IPR_RECLAIM_NUM_BLOCKS_MULTIPLIER);
		else
			sprintf(buffer, "%12d", ntohs(buf->num_blocks));

		body = add_line_to_body(body, _("Number of lost sectors"), buffer);
	} else if (buf->reclaim_unknown_performed) {
		body = add_string_to_body(body,
					  _("IOA cache storage reclamation has completed. "
					    "The number of lost sectors could not be determined.\n"),
					  "", NULL);
	} else {
		body = add_string_to_body(body,
					  _("IOA cache storage reclamation has failed.\n"),
					  "", NULL);
	}

	return body;
}

/**
 * print_reclaim_results - display reclaim results
 * @buf:		ipr_reclaim_query_data struct
 *
 * Returns:
 *   nothing
 **/
static void print_reclaim_results(struct ipr_reclaim_query_data *buf)
{
	char *body = get_reclaim_results(buf);

	printf("%s", body);
	free(body);
}

/**
 * reclaim_result -
 * @i_con:		i_container struct
 *
 * Returns:
 *   EXIT_FLAG if success / 37 | EXIT_FLAG on failure
 **/
int reclaim_result(i_container *i_con)
{
	int rc;
	struct ipr_ioa *reclaim_ioa;
	int max_y,max_x;
	struct screen_output *s_out;
	int action;

	reclaim_ioa = (struct ipr_ioa *) i_con->data;

	action = IPR_RECLAIM_PERFORM;

	/* Everything going according to plan. Proceed with reclaim. */
	getmaxyx(stdscr,max_y,max_x);
	move(max_y-1,0);
	printw("Please wait - reclaim in progress");
	refresh();

	if (reclaim_ioa->reclaim_data->reclaim_unknown_needed)
		action |= IPR_RECLAIM_UNKNOWN_PERM;

	rc = ipr_reclaim_cache_store(reclaim_ioa,
				     action,
				     reclaim_ioa->reclaim_data);

	if (rc != 0)
		/* "Reclaim IOA Cache Storage failed" */
		return (EXIT_FLAG | 37); 

	memset(reclaim_ioa->reclaim_data, 0, sizeof(struct ipr_reclaim_query_data));
	rc = ipr_reclaim_cache_store(reclaim_ioa,
				     IPR_RECLAIM_QUERY,
				     reclaim_ioa->reclaim_data);

	if (rc != 0)
		/* "Reclaim IOA Cache Storage failed" */
		return (EXIT_FLAG | 37); 

	n_reclaim_result.body = get_reclaim_results(reclaim_ioa->reclaim_data);
	s_out = screen_driver(&n_reclaim_result, 0, NULL);
	free(s_out);

	free(n_reclaim_result.body);
	n_reclaim_result.body = NULL;

	rc = ipr_reclaim_cache_store(reclaim_ioa,
				     IPR_RECLAIM_RESET,
				     reclaim_ioa->reclaim_data);

	if (rc != 0)
		rc = (EXIT_FLAG | 37);
	else
		rc = EXIT_FLAG;

	return rc;
}

/**
 * battery_maint -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int battery_maint(i_container *i_con)
{
	int rc, k;
	struct ipr_reclaim_query_data *cur_reclaim_buffer;
	struct ipr_ioa *ioa;
	int found = 0;
	char *buffer[2];
	struct screen_output *s_out;
	int header_lines;
	int toggle=1;

	i_con = free_i_con(i_con);
	body_init_status(buffer, n_battery_maint.header, &header_lines);

	cur_reclaim_buffer = malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

	for (ioa = ipr_ioa_head; ioa; ioa = ioa->next, cur_reclaim_buffer++) {
		rc = ipr_reclaim_cache_store(ioa,
					     IPR_RECLAIM_QUERY | IPR_RECLAIM_EXTENDED_INFO,
					     cur_reclaim_buffer);

		if (rc != 0) {
			ioa->reclaim_data = NULL;
			continue;
		}

		ioa->reclaim_data = cur_reclaim_buffer;

		if (cur_reclaim_buffer->rechargeable_battery_type) {
			i_con = add_i_con(i_con, "\0",ioa);
			print_dev(k, &ioa->ioa, buffer, "%1", k);
			found++;
		}
	}

	if (!found) {
		/* No configured resources contain a cache battery pack */
		rc = 44;
		goto leave;
	}

	do {
		n_battery_maint.body = buffer[toggle&1];
		s_out = screen_driver(&n_battery_maint, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);


leave:
	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_battery_maint.body = NULL;

	return rc;
}

/**
 * get_battery_info -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   char *
 **/
static char *get_battery_info(struct ipr_ioa *ioa)
{
	struct ipr_reclaim_query_data *reclaim_data = ioa->reclaim_data;
	char buffer[128];
	struct ipr_ioa_vpd ioa_vpd;
	struct ipr_cfc_vpd cfc_vpd;
	char product_id[IPR_PROD_ID_LEN+1];
	char serial_num[IPR_SERIAL_NUM_LEN+1];
	char *body = NULL;

	memset(&ioa_vpd, 0, sizeof(ioa_vpd));
	memset(&cfc_vpd, 0, sizeof(cfc_vpd));
	ipr_inquiry(&ioa->ioa, IPR_STD_INQUIRY, &ioa_vpd, sizeof(ioa_vpd));
	ipr_inquiry(&ioa->ioa, 1, &cfc_vpd, sizeof(cfc_vpd));
	ipr_strncpy_0(product_id,
		      (char *)ioa_vpd.std_inq_data.vpids.product_id,
		      IPR_PROD_ID_LEN);
	ipr_strncpy_0(serial_num, (char *)cfc_vpd.serial_num,
		      IPR_SERIAL_NUM_LEN);

	body = add_line_to_body(body,"", NULL);
	body = add_line_to_body(body,_("Resource Name"), ioa->ioa.gen_name);
	body = add_line_to_body(body,_("Serial Number"), serial_num);
	body = add_line_to_body(body,_("Type"), product_id);
	body = add_line_to_body(body,_("PCI Address"), ioa->pci_address);
	if (ioa->sis64)
		body = add_line_to_body(body,_("Resource Path"), ioa->ioa.scsi_dev_data->res_path);
	sprintf(buffer,"%d", ioa->host_num);
	body = add_line_to_body(body,_("SCSI Host Number"), buffer);

	switch (reclaim_data->rechargeable_battery_type) {
	case IPR_BATTERY_TYPE_NICD:
		sprintf(buffer,_("Nickel Cadmium (NiCd)"));
		break;
	case IPR_BATTERY_TYPE_NIMH:
		sprintf(buffer,_("Nickel Metal Hydride (NiMH)"));
		break;
	case IPR_BATTERY_TYPE_LIION:
		sprintf(buffer,_("Lithium Ion (LiIon)"));
		break;
	default:
		sprintf(buffer,_("Unknown"));
		break;
	}

	body = add_line_to_body(body,_("Battery type"), buffer);

	switch (reclaim_data->rechargeable_battery_error_state) {
	case IPR_BATTERY_NO_ERROR_STATE:
		sprintf(buffer,_("No battery warning"));
		break;
	case IPR_BATTERY_WARNING_STATE:
		sprintf(buffer,_("Battery warning issued"));
		break;
	case IPR_BATTERY_ERROR_STATE:
		sprintf(buffer,_("Battery error issued"));
		break;
	default:
		sprintf(buffer,_("Unknown"));
		break;
	}

	body = add_line_to_body(body,_("Battery state"), buffer);

	sprintf(buffer,"%d",htons(reclaim_data->raw_power_on_time));
	body = add_line_to_body(body,_("Power on time (days)"), buffer);

	sprintf(buffer,"%d",htons(reclaim_data->adjusted_power_on_time));
	body = add_line_to_body(body,_("Adjusted power on time (days)"), buffer);

	sprintf(buffer,"%d",htons(reclaim_data->estimated_time_to_battery_warning));
	body = add_line_to_body(body,_("Estimated time to warning (days)"), buffer);

	sprintf(buffer,"%d",htons(reclaim_data->estimated_time_to_battery_failure));
	body = add_line_to_body(body,_("Estimated time to error (days)"), buffer);

	if (reclaim_data->conc_maint_battery)
		sprintf(buffer, "%s", _("Yes"));
	else
		sprintf(buffer, "%s", _("No"));
	body = add_line_to_body(body,_("Concurrently maintainable battery pack"), buffer);

	if (reclaim_data->battery_replace_allowed)
		sprintf(buffer, "%s", _("Yes"));
	else
		sprintf(buffer, "%s", _("No"));
	body = add_line_to_body(body,_("Battery pack can be safely replaced"), buffer);

	return body;
}

/**
 * show_battery_info -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int show_battery_info(struct ipr_ioa *ioa)
{
	struct screen_output *s_out;
	int rc;

	n_show_battery_info.body = get_battery_info(ioa);
	s_out = screen_driver(&n_show_battery_info, 0, NULL);

	free(n_show_battery_info.body);
	n_show_battery_info.body = NULL;
	rc = s_out->rc;
	free(s_out);
	return rc;
}

/**
 * confirm_force_battery_error -
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int confirm_force_battery_error(void)
{
	int rc,k;
	struct ipr_ioa *ioa;
	char *buffer[2];
	struct screen_output *s_out;
	int header_lines;
	int toggle=1;

	rc = RC_SUCCESS;
	body_init_status(buffer, n_confirm_force_battery_error.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->reclaim_data || !ioa->ioa.is_reclaim_cand)
			continue;

		print_dev(k, &ioa->ioa, buffer, "2", k);
	}

	do {
		n_confirm_force_battery_error.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_force_battery_error, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_force_battery_error.body = NULL;

	return rc;
}

/**
 * enable_battery -
 * @i_con:		i_container struct
 *
 * Returns:
 *   71 | EXIT_FLAG if success / 70 | EXIT_FLAG on failure
 **/
int enable_battery(i_container *i_con)
{
	int rc, reclaim_rc;
	struct ipr_ioa *ioa;

	reclaim_rc = rc = RC_SUCCESS;

	for_each_ioa(ioa) {
		if (!ioa->reclaim_data || !ioa->ioa.is_reclaim_cand)
			continue;

		rc = ipr_reclaim_cache_store(ioa,
					     IPR_RECLAIM_RESET_BATTERY_ERROR | IPR_RECLAIM_EXTENDED_INFO,
					     ioa->reclaim_data);

		if (rc != 0)
			reclaim_rc = 70;

		if (ioa->reclaim_data->action_status != IPR_ACTION_SUCCESSFUL) {
			ioa_err(ioa, "Start IOA cache failed.\n");
			reclaim_rc = 70; 
		}
	}

	if (reclaim_rc != RC_SUCCESS)
		return reclaim_rc | EXIT_FLAG;

	return 71 | EXIT_FLAG;
}

/**
 * confirm_enable_battery -
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int confirm_enable_battery(void)
{
	int rc,k;
	struct ipr_ioa *ioa;
	char *buffer[2];
	struct screen_output *s_out;
	int header_lines;
	int toggle=1;

	rc = RC_SUCCESS;
	body_init_status(buffer, n_confirm_start_cache.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->reclaim_data || !ioa->ioa.is_reclaim_cand)
			continue;

		print_dev(k, &ioa->ioa, buffer, "3", k);
	}

	do {
		n_confirm_start_cache.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_start_cache, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_start_cache.body = NULL;

	return rc;
}

/**
 * battery_fork -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int battery_fork(i_container *i_con)
{
	int rc = 0;
	int force_error = 0;
	struct ipr_ioa *ioa;
	char *input;

	i_container *temp_i_con;

	for_each_icon(temp_i_con) {
		ioa = (struct ipr_ioa *)temp_i_con->data;
		if (ioa == NULL)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "3") == 0) {
			ioa->ioa.is_reclaim_cand = 1;
			return confirm_enable_battery();
		} else if (strcmp(input, "2") == 0) {
			force_error++;
			ioa->ioa.is_reclaim_cand = 1;
		} else if (strcmp(input, "1") == 0)	{
			rc = show_battery_info(ioa);
			return rc;
		} else
			ioa->ioa.is_reclaim_cand = 0;
	}

	if (force_error) {
		rc = confirm_force_battery_error();
		return rc;
	}

	return INVALID_OPTION_STATUS;
}

/**
 * force_battery_error -
 * @i_con:		i_container struct
 *
 * Returns:
 *   72 | EXIT_FLAG if success / 43 | EXIT_FLAG on failure
 **/
int force_battery_error(i_container *i_con)
{
	int rc, reclaim_rc;
	struct ipr_ioa *ioa;
	struct ipr_reclaim_query_data reclaim_buffer;

	reclaim_rc = rc = RC_SUCCESS;

	for_each_ioa(ioa) {
		if (!ioa->reclaim_data || !ioa->ioa.is_reclaim_cand)
			continue;

		rc = ipr_reclaim_cache_store(ioa,
					     IPR_RECLAIM_FORCE_BATTERY_ERROR,
					     &reclaim_buffer);

		if (rc != 0)
			reclaim_rc = 43;

		if (reclaim_buffer.action_status != IPR_ACTION_SUCCESSFUL) {
			ioa_err(ioa, "Battery Force Error failed.\n");
			reclaim_rc = 43; 
		}
	}

	if (reclaim_rc != RC_SUCCESS)
		return reclaim_rc | EXIT_FLAG;

	return 72 | EXIT_FLAG;
}

/**
 * bus_config -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int bus_config(i_container *i_con)
{
	int rc, k;
	int found = 0;
	char *buffer[2];
	struct ipr_ioa *ioa;
	u8 ioctl_buffer[255];
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	struct ipr_mode_page_28_header *modepage_28_hdr;
	struct screen_output *s_out;
	int header_lines;
	int toggle = 0;
	rc = RC_SUCCESS;

	processing();
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status(buffer, n_bus_config.header, &header_lines);

	for_each_ioa(ioa) {
		if ((!ioa_is_spi(ioa) || ioa->is_aux_cache) && !ipr_debug)
			continue;

		/* mode sense page28 to focal point */
		mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
		rc = ipr_mode_sense(&ioa->ioa, 0x28, mode_parm_hdr);

		if (rc != 0)
			continue;

		modepage_28_hdr = (struct ipr_mode_page_28_header *) (mode_parm_hdr + 1);

		if (modepage_28_hdr->num_dev_entries == 0)
			continue;

		i_con = add_i_con(i_con,"\0",(char *)ioa);
		print_dev(k, &ioa->ioa, buffer, "%1", k);
		found++;
	}

	if (!found)	{
		n_bus_config_fail.body = body_init(n_bus_config_fail.header, NULL);
		s_out = screen_driver(&n_bus_config_fail, 0, NULL);

		free(n_bus_config_fail.body);
		n_bus_config_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		do {
			n_bus_config.body = buffer[toggle&1];
			s_out = screen_driver(&n_bus_config, header_lines, i_con);
			rc = s_out->rc;
			free(s_out);
			toggle++;
		} while (rc == TOGGLE_SCREEN);
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_bus_config.body = NULL;

	return rc;
}

enum attr_type {qas_capability_type, scsi_id_type, bus_width_type, max_xfer_rate_type};
struct bus_attr {
	enum attr_type type;
	u8 bus;
	struct ipr_scsi_buses *page_28;
	struct ipr_ioa *ioa;
	struct bus_attr *next;
};
struct bus_attr *bus_attr_head = NULL;

/**
 * get_bus_attr_buf - Start array protection for an array
 * @bus_attr:		bus_attr struct
 *
 * Returns:
 *   bus_attr struct
 **/
static struct bus_attr *get_bus_attr_buf(struct bus_attr *bus_attr)
{
	if (!bus_attr) {
		bus_attr_head = malloc(sizeof(struct bus_attr));
		bus_attr_head->next = NULL;
		return bus_attr_head;
	}

	bus_attr->next = malloc(sizeof(struct bus_attr));
	bus_attr->next->next = NULL;
	return bus_attr->next;
}

/**
 * free_all_bus_attr_buf -
 *
 * Returns:
 *   nothing
 **/
static void free_all_bus_attr_buf(void)
{
	struct bus_attr *bus_attr;

	while (bus_attr_head) {
		bus_attr = bus_attr_head->next;
		free(bus_attr_head);
		bus_attr_head = bus_attr;
	}
}	

/**
 * get_changeable_bus_attr -
 * @ioa:		ipr ioa struct
 * @page28:		ipr_scsi_buses struct
 * @num_buses:		number of busses
 *
 * Returns:
 *   nothing
 **/
static void get_changeable_bus_attr(struct ipr_ioa *ioa,
				    struct ipr_scsi_buses *page28, int num_buses)
{
	int j;
	struct ipr_dev *dev;

	for (j = 0; j < num_buses; j++) {
		page28->bus[j].qas_capability = 0;

		if (ioa->scsi_id_changeable)
			page28->bus[j].scsi_id = 1;
		else
			page28->bus[j].scsi_id = 0;

		page28->bus[j].bus_width = 1;

		for_each_dev(ioa, dev) {
			if (!dev->scsi_dev_data)
				continue;
			if (j != dev->scsi_dev_data->channel)
				continue;
			if (dev->scsi_dev_data->id <= 7)
				continue;

			page28->bus[j].bus_width = 0;
			break;
		}

		page28->bus[j].max_xfer_rate = 1;
	}

	if (ioa->protect_last_bus && num_buses) {
		page28->bus[num_buses-1].qas_capability = 0;
		page28->bus[num_buses-1].scsi_id = 0;
		page28->bus[num_buses-1].bus_width = 0;
		page28->bus[num_buses-1].max_xfer_rate = 0;
	}
}

/**
 * change_bus_attr -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int change_bus_attr(i_container *i_con)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int rc, j;
	struct ipr_scsi_buses page_28_cur;
	struct ipr_scsi_buses page_28_chg;
	struct scsi_dev_data *scsi_dev_data;
	char scsi_id_str[5][16];
	char max_xfer_rate_str[5][16];
	i_container *temp_i_con;
	int found = 0;
	char *input;
	struct bus_attr *bus_attr = NULL;
	int header_lines = 0;
	char *body = NULL;
	char buffer[128];
	struct screen_output *s_out;
	u8 bus_num;
	int bus_attr_menu(struct ipr_ioa *ioa,
			  struct bus_attr *bus_attr,
			  int row,
			  int header_lines);

	rc = RC_SUCCESS;

	for_each_icon(temp_i_con) {
		ioa = (struct ipr_ioa *)temp_i_con->data;
		if (!ioa)
			continue;

		input = temp_i_con->field_data;
		if (strcmp(input, "1") == 0) {
			found++;
			break;
		}	
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	i_con = free_i_con(i_con);

	/* zero out user page 28 page, this data is used to indicate
	 which fields the user changed */
	memset(&page_28_cur, 0, sizeof(struct ipr_scsi_buses));
	memset(&page_28_chg, 0, sizeof(struct ipr_scsi_buses));

	/* mode sense page28 to get current parms */
	rc = ipr_get_bus_attr(ioa, &page_28_cur);

	if (rc != 0)
		/* "Change SCSI Bus configurations failed" */
		return 46 | EXIT_FLAG; 

	get_changeable_bus_attr(ioa, &page_28_chg, page_28_cur.num_buses);

	body = body_init(n_change_bus_attr.header, &header_lines);
	sprintf(buffer, "Adapter Location: %s\n", ioa->pci_address);
	body = add_line_to_body(body, buffer, NULL);
	header_lines++;

	for (j = 0; j < page_28_cur.num_buses; j++) {
		if (ioa->protect_last_bus && j == (page_28_cur.num_buses - 1))
			continue;
		sprintf(buffer,_("  BUS%d"), j);
		body = add_line_to_body(body, buffer, NULL);

		if (page_28_chg.bus[j].qas_capability) {
			body = add_line_to_body(body,_("QAS Capability"), "%9");

			bus_attr = get_bus_attr_buf(bus_attr);
			bus_attr->type = qas_capability_type;
			bus_attr->bus = j;
			bus_attr->page_28 = &page_28_cur;
			bus_attr->ioa = ioa;

			if (page_28_cur.bus[j].qas_capability ==
			    IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL)

				i_con = add_i_con(i_con,_("Disabled"),bus_attr);
			else
				i_con = add_i_con(i_con,_("Enabled"),bus_attr);
		}
		if (page_28_chg.bus[j].scsi_id) {
			body = add_line_to_body(body,_("Host SCSI ID"), "%3");

			bus_attr = get_bus_attr_buf(bus_attr);
			bus_attr->type = scsi_id_type;
			bus_attr->bus = j;
			bus_attr->page_28 = &page_28_cur;
			bus_attr->ioa = ioa;

			sprintf(scsi_id_str[j],"%d",page_28_cur.bus[j].scsi_id);
			i_con = add_i_con(i_con,scsi_id_str[j],bus_attr);
		}
		if (page_28_chg.bus[j].bus_width)	{
			/* check if 8 bit bus is allowable with current configuration before
			 enabling option */
			for_each_dev(ioa, dev) {
				scsi_dev_data = dev->scsi_dev_data;

				if (scsi_dev_data && (scsi_dev_data->id & 0xF8) &&
				    (j == scsi_dev_data->channel)) {
					page_28_chg.bus[j].bus_width = 0;
					break;
				}
			}
		}
		if (page_28_chg.bus[j].bus_width)	{
			body = add_line_to_body(body,_("Wide Enabled"), "%4");

			bus_attr = get_bus_attr_buf(bus_attr);
			bus_attr->type = bus_width_type;
			bus_attr->bus = j;
			bus_attr->page_28 = &page_28_cur;
			bus_attr->ioa = ioa;

			if (page_28_cur.bus[j].bus_width == 16)
				i_con = add_i_con(i_con,_("Yes"),bus_attr);
			else
				i_con = add_i_con(i_con,_("No"),bus_attr);
		}
		if (page_28_chg.bus[j].max_xfer_rate) {
			body = add_line_to_body(body,_("Maximum Bus Throughput"), "%9");

			bus_attr = get_bus_attr_buf(bus_attr);
			bus_attr->type = max_xfer_rate_type;
			bus_attr->bus = j;
			bus_attr->page_28 = &page_28_cur;
			bus_attr->ioa = ioa;

			sprintf(max_xfer_rate_str[j],"%d MB/s",
				(page_28_cur.bus[j].max_xfer_rate *
				 page_28_cur.bus[j].bus_width)/(10 * 8));
			i_con = add_i_con(i_con,max_xfer_rate_str[j],bus_attr);
		}
	}

	n_change_bus_attr.body = body;
	while (1) {
		s_out = screen_driver(&n_change_bus_attr, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);

		for_each_icon(temp_i_con) {
			if (!temp_i_con->field_data[0]) {
				bus_attr = (struct bus_attr *)temp_i_con->data;
				rc = bus_attr_menu(ioa, bus_attr, temp_i_con->y, header_lines);
				if (rc == CANCEL_FLAG)
					rc = RC_SUCCESS;

				bus_num = bus_attr->bus;

				if (bus_attr->type == qas_capability_type) {
					if (page_28_cur.bus[bus_num].qas_capability ==
					    IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL)

						sprintf(temp_i_con->field_data,_("Disable"));
					else
						sprintf(temp_i_con->field_data,_("Enable"));
				} else if (bus_attr->type == scsi_id_type) {
					sprintf(temp_i_con->field_data,"%d",page_28_cur.bus[bus_num].scsi_id);
				} else if (bus_attr->type == bus_width_type) {
					if (page_28_cur.bus[bus_num].bus_width == 16)
						sprintf(temp_i_con->field_data,_("Yes"));
					else
						sprintf(temp_i_con->field_data,_("No"));
				} else if (bus_attr->type == max_xfer_rate_type) {
					sprintf(temp_i_con->field_data,"%d MB/s",
						(page_28_cur.bus[bus_num].max_xfer_rate *
						 page_28_cur.bus[bus_num].bus_width)/(10 * 8));
				}
				break;
			}
		}

		if (rc)
			goto leave;
	}

leave:
	free_all_bus_attr_buf();
	free(n_change_bus_attr.body);
	n_change_bus_attr.body = NULL;
	free_i_con(i_con);
	return rc;
}

/**
 * confirm_change_bus_attr -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / 45 | EXIT_FLAG on failure
 **/
int confirm_change_bus_attr(i_container *i_con)
{
	struct ipr_ioa *ioa;
	int rc, j;
	struct ipr_scsi_buses *page_28_cur;
	struct ipr_scsi_buses page_28_chg;
	char scsi_id_str[5][16];
	char max_xfer_rate_str[5][16];
	int header_lines = 0;
	char *body = NULL;
	char buffer[128];
	struct screen_output *s_out;

	rc = RC_SUCCESS;

	page_28_cur = bus_attr_head->page_28;
	ioa = bus_attr_head->ioa;

	get_changeable_bus_attr(ioa, &page_28_chg, page_28_cur->num_buses);

	body = body_init(n_confirm_change_bus_attr.header, &header_lines);
	body = add_line_to_body(body, ioa->ioa.gen_name, NULL);
	header_lines++;

	for (j = 0; j < page_28_cur->num_buses; j++) {
		if (ioa->protect_last_bus && j == (page_28_cur->num_buses - 1))
			continue;
		sprintf(buffer,_("  BUS%d"), j);
		body = add_line_to_body(body, buffer, NULL);

		if (page_28_chg.bus[j].qas_capability) {
			if (page_28_cur->bus[j].qas_capability ==
			    IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL)

				body = add_line_to_body(body, _("QAS Capability"), _("Disable"));
			else
				body = add_line_to_body(body, _("QAS Capability"), _("Enable"));
		}
		if (page_28_chg.bus[j].scsi_id) {
			sprintf(scsi_id_str[j],"%d",page_28_cur->bus[j].scsi_id);
			body = add_line_to_body(body,_("Host SCSI ID"), scsi_id_str[j]);
		}
		if (page_28_chg.bus[j].bus_width)	{
			if (page_28_cur->bus[j].bus_width == 16)
				body = add_line_to_body(body,_("Wide Enabled"), _("Yes"));
			else
				body = add_line_to_body(body,_("Wide Enabled"), _("No"));
		}
		if (page_28_chg.bus[j].max_xfer_rate) {
			sprintf(max_xfer_rate_str[j],"%d MB/s",
				(page_28_cur->bus[j].max_xfer_rate *
				 page_28_cur->bus[j].bus_width)/(10 * 8));
			body = add_line_to_body(body,_("Maximum Bus Throughput"), max_xfer_rate_str[j]);
		}
	}

	n_confirm_change_bus_attr.body = body;
	s_out = screen_driver(&n_confirm_change_bus_attr, header_lines, NULL);

	rc = s_out->rc;
	free(s_out);

	free(n_confirm_change_bus_attr.body);
	n_confirm_change_bus_attr.body = NULL;

	if (rc)
		return rc;

	processing();

	rc = ipr_set_bus_attr(ioa, page_28_cur, 1);
	if (!rc)
		rc = 45 | EXIT_FLAG;
	return rc;
}

/**
 * bus_attr_menu -
 * @ioa:		ipr ioa struct
 * @bus_attr:		bus_attr struct
 * @start_row:		starting row number
 * @header_lines:	number of header lines
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int bus_attr_menu(struct ipr_ioa *ioa, struct bus_attr *bus_attr, int start_row, int header_lines)
{
	int i, scsi_id, found;
	int num_menu_items;
	int menu_index = 0;
	ITEM **menu_item = NULL;
	struct ipr_scsi_buses *page_28_cur;
	struct ipr_dev *dev;
	int *userptr;
	int *retptr;
	u8 bus_num;
	int max_bus_width, max_xfer_rate;
	int rc = RC_SUCCESS;

	/* start_row represents the row in which the top most
	 menu item will be placed.  Ideally this will be on the
	 same line as the field in which the menu is opened for*/
	start_row += 2; /* for title */  /* FIXME */
	page_28_cur = bus_attr->page_28;
	bus_num = bus_attr->bus;

	if (bus_attr->type == qas_capability_type) {
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
		    (page_28_cur->bus[bus_num].qas_capability != *retptr)) {

			page_28_cur->bus[bus_num].qas_capability = *retptr;
		}
		i=0;
		while (menu_item[i] != NULL)
			free_item(menu_item[i++]);
		free(menu_item);
		free(userptr);
		menu_item = NULL;
	} else if (bus_attr->type == scsi_id_type) {
		num_menu_items = 16;
		menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
		userptr = malloc(sizeof(int) * num_menu_items);

		menu_index = 0;
		for (scsi_id = 0, found = 0; scsi_id < 16; scsi_id++, found = 0) {
			for_each_dev(ioa, dev) {
				if (!dev->scsi_dev_data)
					continue;

				if (scsi_id == dev->scsi_dev_data->id &&
				    bus_num == dev->scsi_dev_data->channel) {
					found = 1;
					break;
				}
			}

			if (!found) {
				switch (scsi_id) {
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
		    (page_28_cur->bus[bus_num].scsi_id != *retptr)) {

			page_28_cur->bus[bus_num].scsi_id = *retptr;
		}
		i=0;
		while (menu_item[i] != NULL)
			free_item(menu_item[i++]);
		free(menu_item);
		free(userptr);
		menu_item = NULL;
	} else if (bus_attr->type == bus_width_type) {
		num_menu_items = 2;
		menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
		userptr = malloc(sizeof(int) * num_menu_items);

		menu_index = 0;
		menu_item[menu_index] = new_item("No","");
		userptr[menu_index] = 8;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);

		menu_index++;
		max_bus_width = 16;
		if (max_bus_width == 16) {
			menu_item[menu_index] = new_item("Yes","");
			userptr[menu_index] = 16;
			set_item_userptr(menu_item[menu_index],
					 (char *)&userptr[menu_index]);
			menu_index++;
		}
		menu_item[menu_index] = (ITEM *)NULL;
		rc = display_menu(menu_item, start_row, menu_index, &retptr);
		if ((rc == RC_SUCCESS) &&
		    (page_28_cur->bus[bus_num].bus_width != *retptr)) {

			page_28_cur->bus[bus_num].bus_width = *retptr;
		}
		i=0;
		while (menu_item[i] != NULL)
			free_item(menu_item[i++]);
		free(menu_item);
		free(userptr);
		menu_item = NULL;
	} else if (bus_attr->type == max_xfer_rate_type) {
		num_menu_items = 7;
		menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
		userptr = malloc(sizeof(int) * num_menu_items);

		menu_index = 0;
		max_xfer_rate = get_max_bus_speed(ioa, bus_num);

		switch (max_xfer_rate) {
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
		    (page_28_cur->bus[bus_num].max_xfer_rate !=
		     ((*retptr) * 8)/page_28_cur->bus[bus_num].bus_width)) {

			page_28_cur->bus[bus_num].max_xfer_rate =
				((*retptr) * 8)/page_28_cur->bus[bus_num].bus_width;
		}
		i=0;
		while (menu_item[i] != NULL)
			free_item(menu_item[i++]);
		free(menu_item);
		free(userptr);
		menu_item = NULL;
	}

	return rc;
}

/**
 * display_menu -
 * @menu_item:		menu items
 * @menu_start_row:	start row
 * @menu_index_max:	max menu index
 * @userptrptr:		
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int display_menu(ITEM **menu_item,
		 int menu_start_row,
		 int menu_index_max,
		 int **userptrptr)
{
	ITEM *cur_item;
	MENU *menu;
	WINDOW *box1_win, *box2_win, *field_win;
	int menu_rows, menu_cols, menu_rows_disp;
	int menu_index;
	int use_box2 = 0;
	int cur_item_index;
	int ch, i;
	int rc = RC_SUCCESS;
	int max_y,max_x;

	getmaxyx(stdscr,max_y,max_x);

	menu = new_menu(menu_item);
	scale_menu(menu, &menu_rows, &menu_cols);
	menu_rows_disp = menu_rows;

	/* check if fits in current screen, + 1 is added for
	 bottom border, +1 is added for buffer */
	if ((menu_start_row + menu_rows + 1 + 1) > max_y) {
		menu_start_row = max_y - (menu_rows + 1 + 1);
		if (menu_start_row < (8 + 1)) {
			/* size is adjusted by 2 to allow for top
			 and bottom border, add +1 for buffer */
			menu_rows_disp = max_y - (8 + 2 + 1);
			menu_start_row = (8 + 1);
		}
	}

	if (menu_rows > menu_rows_disp) {
		set_menu_format(menu, menu_rows_disp, 1);
		use_box2 = 1;
	}

	set_menu_mark(menu, "*");
	scale_menu(menu, &menu_rows, &menu_cols);
	box1_win = newwin(menu_rows + 2, menu_cols + 2, menu_start_row - 1, 54);
	field_win = newwin(menu_rows, menu_cols, menu_start_row, 55);
	set_menu_win(menu, field_win);
	set_menu_sub(menu, derwin(field_win, menu_rows, menu_cols, 0, 0));
	post_menu(menu);
	box(box1_win,ACS_VLINE,ACS_HLINE);
	if (use_box2) {
		box2_win = newwin(menu_rows + 2, 3, menu_start_row - 1, 54 + menu_cols + 1);
		box(box2_win,ACS_VLINE,ACS_HLINE);

		/* initialize scroll bar */
		for (i = 0; i < menu_rows_disp/2; i++)
			mvwaddstr(box2_win, i + 1, 1, "#");

		/* put down arrows in */
		for (i = menu_rows_disp; i > (menu_rows_disp - menu_rows_disp/2); i--)
			mvwaddstr(box2_win, i, 1, "v");
	}
	wnoutrefresh(box1_win);

	menu_index = 0;
	while (1) {
		if (use_box2)
			wnoutrefresh(box2_win);
		wnoutrefresh(field_win);
		doupdate();
		ch = getch();
		if (IS_ENTER_KEY(ch)) {
			cur_item = current_item(menu);
			cur_item_index = item_index(cur_item);
			*userptrptr = item_userptr(menu_item[cur_item_index]);
			break;
		} else if ((ch == KEY_DOWN) || (ch == '\t')) {
			if (use_box2 && (menu_index < (menu_index_max - 1))) {
				menu_index++;
				if (menu_index == menu_rows_disp) {
					/* put up arrows in */
					for (i = 0; i < menu_rows_disp/2; i++)
						mvwaddstr(box2_win, i + 1, 1, "^");
				}
				if (menu_index == (menu_index_max - 1)) {
					/* remove down arrows */
					for (i = menu_rows_disp; i > (menu_rows_disp - menu_rows_disp/2); i--)
						mvwaddstr(box2_win, i, 1, "#");
				}
			}

			menu_driver(menu,REQ_DOWN_ITEM);
		} else if (ch == KEY_UP) {
			if (use_box2 && menu_index != 0) {
				menu_index--;
				if (menu_index == (menu_index_max - menu_rows_disp - 1)) {
					/* put down arrows in */
					for (i = menu_rows_disp; i > (menu_rows_disp - menu_rows_disp/2); i--)
						mvwaddstr(box2_win, i, 1, "v");
				}
				if (menu_index == 0) {
					/* remove up arrows */
					for (i = 0; i < menu_rows_disp/2; i++)
						mvwaddstr(box2_win, i + 1, 1, "#");
				}
			}
			menu_driver(menu,REQ_UP_ITEM);
		} else if (IS_EXIT_KEY(ch)) {
			rc = EXIT_FLAG;
			break;
		} else if (IS_CANCEL_KEY(ch)) {
			rc = CANCEL_FLAG;
			break;
		}
	}

	unpost_menu(menu);
	free_menu(menu);
	wclear(field_win);
	wrefresh(field_win);
	delwin(field_win);
	wclear(box1_win);
	wrefresh(box1_win);
	delwin(box1_win);
	if (use_box2) {
		wclear(box2_win);
		wrefresh(box2_win);
		delwin(box2_win);
	}

	return rc;
}

/**
 * driver_config -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int driver_config(i_container *i_con)
{
	int rc, k;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;
	int toggle = 0;

	processing();
	/* empty the linked list that contains field pointens */
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);
	body_init_status(buffer, n_driver_config.header, &header_lines);

	for_each_ioa(ioa) {
		i_con = add_i_con(i_con,"\0",ioa);
		print_dev(k, &ioa->ioa, buffer, "%1", k);
	}

	do {
		n_driver_config.body = buffer[toggle&1];
		s_out = screen_driver(&n_driver_config, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_driver_config.body = NULL;

	return rc;
}

/**
 * get_log_level - return the log level
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   log level value if success / 0 on failure
 **/
int get_log_level(struct ipr_ioa *ioa)
{
	char value_str[100];
	int value;

	if (ipr_read_host_attr(ioa, "log_level", value_str, 100) < 0)
		return 0;

	sscanf(value_str, "%d", &value);
	return value;
}

/**
 * set_log_level - set the log level
 * @ioa:		ipr ioa struct
 * @log_level:		log level string
 *
 * Returns:
 *   nothing
 **/
void set_log_level(struct ipr_ioa *ioa, char *log_level)
{
	ipr_write_host_attr(ioa, "log_level", log_level,
			    strlen(log_level));
}

/**
 * change_driver_config -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int change_driver_config(i_container *i_con)
{
	int rc, i, digits;
	int found = 0;
	i_container *temp_i_con;
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	char buffer[128];
	char *body = NULL;
	char *input;

	for_each_icon(temp_i_con) {
		ioa = (struct ipr_ioa *)(temp_i_con->data);
		if (ioa == NULL)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			found++;
			break;
		}
	}

	if (!found)
		return 2;

	i_con = free_i_con(i_con);

	/* i_con to return field data */
	i_con = add_i_con(i_con,"",NULL); 

	body = body_init(n_change_driver_config.header, NULL);
	sprintf(buffer,"Adapter Location: %s\n", ioa->pci_address);
	body = add_line_to_body(body, buffer, NULL);
	sprintf(buffer,"%d", get_log_level(ioa));
	body = add_line_to_body(body,_("Current Log Level"), buffer);
	body = add_line_to_body(body,_("New Log Level"), "%2");

	n_change_driver_config.body = body;
	s_out = screen_driver(&n_change_driver_config, 0, i_con);

	free(n_change_driver_config.body);
	n_change_driver_config.body = NULL;
	rc = s_out->rc;

	if (!rc) {
		for (i = 0, digits = 0; i < strlen(i_con->field_data); i++) {
			if (isdigit(i_con->field_data[i])) {
				digits++;
			} else if (!isspace(i_con->field_data[i])) {
				rc = 48;
				break;
			}
		}
		if (!rc && digits) {
			set_log_level(ioa, i_con->field_data);
			rc = 57;
		}
	}

	free(s_out);

	return rc;
}

/**
 * disk_config -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int disk_config(i_container * i_con)
{
	int rc, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;
	char *buffer[2];
	int toggle = 1;

	processing();
	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status(buffer, n_disk_config.header, &header_lines);

	for_each_ioa(ioa) {
		if (ioa->is_secondary || ioa->ioa_dead || !ioa->ioa.scsi_dev_data)
			continue;

		num_lines += print_standalone_disks(ioa, &i_con, buffer, 0);
		num_lines += print_hotspare_disks(ioa, &i_con, buffer, 0);
		num_lines += print_vsets(ioa, &i_con, buffer, 0);
	}


	if (num_lines == 0) {
		for (k = 0; k < 2; k++) {
			len = strlen(buffer[k]);
			buffer[k] = realloc(buffer[k], len +
						strlen(_(no_dev_found)) + 8);
			sprintf(buffer[k] + len, "\n%s", _(no_dev_found));
		}
	}

	do {
		n_disk_config.body = buffer[toggle&1];
		s_out = screen_driver(&n_disk_config, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_disk_config.body = NULL;

	return rc;
}

struct disk_config_attr {
	int option;
	int queue_depth;
	int format_timeout;
	int tcq_enabled;
	int write_cache_policy;
};

/**
 * disk_config_menu -
 * @disk_config_attr:	disk_config_attr struct
 * @start_row:		start row number
 * @header_lines:	number of header lines
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int disk_config_menu(struct disk_config_attr *disk_config_attr,
			    int start_row, int header_lines)
{
	int i;
	int num_menu_items;
	int menu_index;
	ITEM **menu_item = NULL;
	int *retptr;
	int *userptr = NULL;
	int rc = RC_SUCCESS;

	/* start_row represents the row in which the top most
	 menu item will be placed.  Ideally this will be on the
	 same line as the field in which the menu is opened for*/
	start_row += 2; /* for title */  /* FIXME */

	if (disk_config_attr->option == 1) { /* queue depth*/
		rc = display_input(start_row, &disk_config_attr->queue_depth);
	} else if (disk_config_attr->option == 3) {  /* tag command queue.*/
		num_menu_items = 2;
		menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
		userptr = malloc(sizeof(int) * num_menu_items);

		menu_index = 0;
		menu_item[menu_index] = new_item("No","");
		userptr[menu_index] = 0;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);

		menu_index++;

		menu_item[menu_index] = new_item("Yes","");
		userptr[menu_index] = 1;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);
		menu_index++;

		menu_item[menu_index] = (ITEM *)NULL;
		rc = display_menu(menu_item, start_row, menu_index, &retptr);
		if (rc == RC_SUCCESS)
			disk_config_attr->tcq_enabled = *retptr;

		i=0;
		while (menu_item[i] != NULL)
			free_item(menu_item[i++]);
		free(menu_item);
		free(userptr);
		menu_item = NULL;
	} else if (disk_config_attr->option == 2) { /* format t.o.*/
		num_menu_items = 5;
		menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
		userptr = malloc(sizeof(int) * num_menu_items);

		menu_index = 0;

		menu_item[menu_index] = new_item("8 hr","");
		userptr[menu_index] = 8;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);
		menu_index++;

		menu_item[menu_index] = new_item("4 hr","");
		userptr[menu_index] = 4;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);
		menu_index++;

		menu_item[menu_index] = new_item("3 hr","");
		userptr[menu_index] = 3;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);
		menu_index++;

		menu_item[menu_index] = new_item("2 hr","");
		userptr[menu_index] = 2;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);
		menu_index++;

		menu_item[menu_index] = new_item("1 hr","");
		userptr[menu_index] = 1;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);
		menu_index++;

		menu_item[menu_index] = (ITEM *)NULL;
		rc = display_menu(menu_item, start_row, menu_index, &retptr);
		if (rc == RC_SUCCESS)
			disk_config_attr->format_timeout = *retptr;

		i=0;
		while (menu_item[i] != NULL)
			free_item(menu_item[i++]);
		free(menu_item);
		free(userptr);
		menu_item = NULL;
	} else if (disk_config_attr->option == 4) {  /* write cache policy.*/
		num_menu_items = 2;
		menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
		userptr = malloc(sizeof(int) * num_menu_items);

		menu_index = 0;
		menu_item[menu_index] = new_item("Write Through","");
		userptr[menu_index] = IPR_DEV_CACHE_WRITE_THROUGH;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);

		menu_index++;

		menu_item[menu_index] = new_item("Write Back","");
		userptr[menu_index] = IPR_DEV_CACHE_WRITE_BACK;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);
		menu_index++;

		menu_item[menu_index] = (ITEM *)NULL;
		rc = display_menu(menu_item, start_row, menu_index, &retptr);
		if (rc == RC_SUCCESS) {
			disk_config_attr->write_cache_policy = *retptr;
		}

		i=0;
		while (menu_item[i] != NULL)
			free_item(menu_item[i++]);
		free(menu_item);
		free(userptr);
		menu_item = NULL;
	}

	return rc;
}

/**
 * change_disk_config -
 * @i_con:		i_container struct
 *
 * Returns:
 *    non-zero
 **/
int change_disk_config(i_container * i_con)
{
	int rc;
	i_container *i_con_head_saved;
	struct ipr_dev *dev;
	char qdepth_str[4];
	char format_timeout_str[8];
	char buffer[200];
	i_container *temp_i_con;
	int found = 0;
	char *input;
	struct disk_config_attr disk_config_attr[4];
	struct disk_config_attr *config_attr = NULL;
	struct ipr_disk_attr disk_attr;
	int header_lines = 0;
	char *body = NULL;
	struct screen_output *s_out;
	struct ipr_array_record *array_record;
	int tcq_warning = 0;
	int tcq_blocked = 0;
	int tcq_enabled = 0;
	int num_devs, ssd_num_devs, max_qdepth;

	rc = RC_SUCCESS;

	for_each_icon(temp_i_con) {
		dev = (struct ipr_dev *)temp_i_con->data;
		if (!dev)
			continue;

		input = temp_i_con->field_data;
		if (strcmp(input, "1") == 0) {
			found++;
			break;
		}	
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	rc = ipr_get_dev_attr(dev, &disk_attr);
	if (rc)
		return 66 | EXIT_FLAG;

	i_con_head_saved = i_con_head; /* FIXME */
	i_con_head = i_con = NULL;

	body = body_init(n_change_disk_config.header, &header_lines);
	sprintf(buffer, "Device: %s   %s\n", dev->scsi_dev_data->sysfs_device_name,
		dev->dev_name);
	header_lines += 2;

	if (ipr_is_volume_set(dev) || page0x0a_setup(dev))
		header_lines += 1;
/* xxx
	else if (ipr_is_af_dasd_device(dev) && disk_attr.queue_depth > 1)
		tcq_warning = 1;
	else if (ipr_is_gscsi(dev) && disk_attr.tcq_enabled)
		tcq_warning = 1;
	else
		tcq_blocked = 1;
*/

	if (tcq_warning) {
		body = add_line_to_body(body, buffer, NULL);
		body = add_string_to_body(body,
					  _("Note: Tagged queuing to this device is ENABLED "
					    "and it does not support the required "
					    "control mode page settings to run safely in this mode!\n\n"),
					  "", &header_lines);
	} else if (tcq_blocked) {
		body = add_line_to_body(body, buffer, NULL);
		body = add_string_to_body(body,
					  _("Note: Tagged queuing to this device is disabled by default "
					    "since it does not support the required control mode page "
					    "settings to run safely in this mode.\n\n"),
					  "", &header_lines);
	}

	if (ipr_force)
		tcq_warning = tcq_blocked = 0;

	if (!tcq_blocked || ipr_is_gscsi(dev)) {
		ipr_count_devices_in_vset(dev, &num_devs, &ssd_num_devs);
		max_qdepth = ipr_max_queue_depth(dev->ioa, num_devs, ssd_num_devs);
		sprintf(buffer, "Queue Depth (max = %03d)", max_qdepth);
		body = add_line_to_body(body, buffer, "%3");

		disk_config_attr[0].option = 1;

		if (disk_attr.queue_depth > max_qdepth)
			disk_attr.queue_depth = max_qdepth;

		disk_config_attr[0].queue_depth = disk_attr.queue_depth;
		sprintf(qdepth_str, "%d", disk_config_attr[0].queue_depth);
		i_con = add_i_con(i_con, qdepth_str, &disk_config_attr[0]);
	}

	array_record = dev->array_rcd;

	if (array_record &&
	    ipr_is_vset_record(array_record->common.record_id)) {
		/* VSET, no further fields */
	} else if (ipr_is_af_dasd_device(dev)) {
		disk_config_attr[1].option = 2;
		disk_config_attr[1].format_timeout = disk_attr.format_timeout / (60 * 60);
		body = add_line_to_body(body,_("Format Timeout"), "%4");

		snprintf(format_timeout_str, sizeof(format_timeout_str), "%d hr",
			 disk_config_attr[1].format_timeout);

		i_con = add_i_con(i_con, format_timeout_str, &disk_config_attr[1]);
	} else if (!tcq_blocked) {
		disk_config_attr[2].option = 3;
		disk_config_attr[2].tcq_enabled = disk_attr.tcq_enabled;
		body = add_line_to_body(body,_("Tag Command Queueing"), "%4");
		tcq_enabled = disk_config_attr[2].tcq_enabled;
		if (disk_config_attr[2].tcq_enabled)
			i_con = add_i_con(i_con,_("Yes"),&disk_config_attr[2]);
		else
			i_con = add_i_con(i_con,_("No"),&disk_config_attr[2]);
	}

	if (ipr_is_gscsi(dev) ||
	    (dev->ioa->vset_write_cache && ipr_is_volume_set(dev))) {
		disk_config_attr[3].option = 4;
		disk_config_attr[3].write_cache_policy =
						disk_attr.write_cache_policy;
		body = add_line_to_body(body,_("Device Write Cache Policy"), "%14");

		if (disk_config_attr[3].write_cache_policy ==
						IPR_DEV_CACHE_WRITE_THROUGH)
			i_con = add_i_con(i_con,_("Write Through"),
							&disk_config_attr[3]);

		else
			i_con = add_i_con(i_con,_("Write Back"),
							&disk_config_attr[3]);
	}

	n_change_disk_config.body = body;
	while (1) {
		s_out = screen_driver(&n_change_disk_config, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);

		found = 0;

		for_each_icon(temp_i_con) {
			if (!temp_i_con->field_data[0]) {
				config_attr = (struct disk_config_attr *)temp_i_con->data;
				rc = disk_config_menu(config_attr, temp_i_con->y, header_lines);

				if (rc == CANCEL_FLAG)
					rc = RC_SUCCESS;

				if (config_attr->option == 1) {
					sprintf(temp_i_con->field_data,"%d", config_attr->queue_depth);
					if (config_attr->queue_depth > max_qdepth)
						disk_attr.queue_depth = max_qdepth;
					else
						disk_attr.queue_depth = config_attr->queue_depth;
				}
				else if (config_attr->option == 2) {
					sprintf(temp_i_con->field_data,"%d hr", config_attr->format_timeout);
					disk_attr.format_timeout = config_attr->format_timeout * 60 * 60;
				}
				else if (config_attr->option == 3) {
					if (config_attr->tcq_enabled)
						sprintf(temp_i_con->field_data,_("Yes"));
					else
						sprintf(temp_i_con->field_data,_("No"));
					disk_attr.tcq_enabled = config_attr->tcq_enabled;
				}
				else if (config_attr->option == 4) {
					if (config_attr->write_cache_policy == IPR_DEV_CACHE_WRITE_THROUGH)
						sprintf(temp_i_con->field_data,
									_("Write Through"));
					else
						sprintf(temp_i_con->field_data,
									_("Write Back"));

					disk_attr.write_cache_policy =
						config_attr->write_cache_policy;
				}
				found++;
				break;
			}
		}

		if (rc)
			goto leave;

		if (!found)
			break;
	}

	ipr_set_dev_attr(dev, &disk_attr, 1);
	check_current_config(false);

leave:
	free(n_change_disk_config.body);
	n_change_disk_config.body = NULL;
	free_i_con(i_con);
	i_con_head = i_con_head_saved;
	return rc;
}

struct ioa_config_attr {
	int option;
	int preferred_primary;
	int gscsi_only_ha;
	int active_active;
	int caching;
	int verify_array_rebuild;
	int rebuild_rate;
};

/**
 * ioa_config_menu -
 * @ioa_config_attr:	ipr_config_attr struct
 * @start_row:		start row
 * @header_lines:	number of header lines
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ioa_config_menu(struct ioa_config_attr *ioa_config_attr,
		    int start_row, int header_lines)
{
	int i;
	int num_menu_items;
	int menu_index;
	ITEM **menu_item = NULL;
	int *retptr;
	int *userptr = NULL;
	int rc = RC_SUCCESS;

	/* start_row represents the row in which the top most
	 menu item will be placed.  Ideally this will be on the
	 same line as the field in which the menu is opened for*/
	start_row += 2; /* for title */  /* FIXME */

	if (ioa_config_attr->option == 1 || ioa_config_attr->option == 2
	    || ioa_config_attr->option == 3 || ioa_config_attr->option == 4
	    || ioa_config_attr->option == 5) {
		num_menu_items = 4;
		menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
		userptr = malloc(sizeof(int) * num_menu_items);

		menu_index = 0;
		if (ioa_config_attr->option == 1)
			menu_item[menu_index] = new_item("None","");
		else if (ioa_config_attr->option == 2)
			menu_item[menu_index] = new_item("RAID","");
		else if (ioa_config_attr->option == 3)
			menu_item[menu_index] = new_item("Disabled","");
		else if (ioa_config_attr->option == 5)
			menu_item[menu_index] = new_item("Enabled","");
		else
			menu_item[menu_index] = new_item("Default","");

		userptr[menu_index] = 0;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);

		menu_index++;

		if (ioa_config_attr->option == 1)
			menu_item[menu_index] = new_item("Primary","");
		else if (ioa_config_attr->option == 2)
			menu_item[menu_index] = new_item("JBOD","");
		else if (ioa_config_attr->option == 3)
			menu_item[menu_index] = new_item("Enabled","");
		else if (ioa_config_attr->option == 5)
			menu_item[menu_index] = new_item("Disabled","");
		else
			menu_item[menu_index] = new_item("Disabled","");

		userptr[menu_index] = 1;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);
		menu_index++;

		menu_item[menu_index] = (ITEM *)NULL;
		rc = display_menu(menu_item, start_row, menu_index, &retptr);
		if (rc == RC_SUCCESS) {
			if (ioa_config_attr->option == 1)
				ioa_config_attr->preferred_primary = *retptr;
			else if (ioa_config_attr->option == 2)
				ioa_config_attr->gscsi_only_ha = *retptr;
			else if (ioa_config_attr->option == 3)
				ioa_config_attr->active_active = *retptr;
			else if (ioa_config_attr->option == 5)
				ioa_config_attr->verify_array_rebuild = *retptr;
			else
				ioa_config_attr->caching = *retptr;
		}

		i = 0;
		while (menu_item[i] != NULL)
			free_item(menu_item[i++]);
		free(menu_item);
		free(userptr);
		menu_item = NULL;
	} else if (ioa_config_attr->option == 6) {
		rc = display_input(start_row + 1,
						&ioa_config_attr->rebuild_rate);
		/* (start_row + 1) to correct window location */

		if (ioa_config_attr->rebuild_rate > 100)
			ioa_config_attr->rebuild_rate = 100;

		if (ioa_config_attr->rebuild_rate < 0)
			ioa_config_attr->rebuild_rate = 0;

			ioa_config_attr->rebuild_rate =
				(ioa_config_attr->rebuild_rate * 15) / 100;
		/*
		 * this constant multiplication *(15/100) it's only used to
		 * give the user values 0-100 even though IOA only knows 0-15
		*/
	}

	return rc;
}

/**
 * change_ioa_config -
 * @i_con:		i_container struct
 *
 * Returns:
 *   non-zero on exit
 **/
int change_ioa_config(i_container * i_con)
{
	int rc;
	i_container *i_con_head_saved;
	struct ipr_dev *dev;
	char pref_str[20];
	char buffer[128];
	i_container *temp_i_con;
	int found = 0;
	char *input;
	struct ioa_config_attr ioa_config_attr[6];
	/*
	 * For now, the above 6 is a magic number - it means the maximum
	 * number of options showed at screen. It should be more generic...
	*/
	struct ioa_config_attr *config_attr = NULL;
	struct ipr_ioa_attr ioa_attr;
	int header_lines = 0, index = 0;
	char *body = NULL;
	struct screen_output *s_out;

	rc = RC_SUCCESS;

	for_each_icon(temp_i_con) {
		dev = (struct ipr_dev *)temp_i_con->data;
		if (!dev)
			continue;

		input = temp_i_con->field_data;
		if (strcmp(input, "1") == 0) {
			found++;
			break;
		}	
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	rc = ipr_get_ioa_attr(dev->ioa, &ioa_attr);
	if (rc)
		return 66 | EXIT_FLAG;

	i_con_head_saved = i_con_head; /* FIXME */
	i_con_head = i_con = NULL;

	body = body_init(n_change_ioa_config.header, &header_lines);
	sprintf(buffer, "Adapter: %s/%d   %s %s\n", dev->ioa->pci_address,
		dev->ioa->host_num, dev->scsi_dev_data->vendor_id,
		dev->scsi_dev_data->product_id);
	body = add_line_to_body(body, buffer, NULL);
	header_lines += 2;

	if (dev->ioa->dual_raid_support) {
		body = add_line_to_body(body,_("Preferred Dual Adapter State"), "%13");
		ioa_config_attr[index].option = 1;
		ioa_config_attr[index].preferred_primary = ioa_attr.preferred_primary;
		if (ioa_attr.preferred_primary)
			sprintf(pref_str, "Primary");
		else
			sprintf(pref_str, "None");
		i_con = add_i_con(i_con, pref_str, &ioa_config_attr[index++]);
	}

	if (dev->ioa->gscsi_only_ha) {
		body = add_line_to_body(body,_("High-Availability Mode"), "%13");
		ioa_config_attr[index].option = 2;
		ioa_config_attr[index].gscsi_only_ha = ioa_attr.gscsi_only_ha;
		if (ioa_attr.gscsi_only_ha)
			sprintf(pref_str, "JBOD");
		else
			sprintf(pref_str, "Normal");
		i_con = add_i_con(i_con, pref_str, &ioa_config_attr[index++]);
	}
	if (dev->ioa->asymmetric_access) {
		body = add_line_to_body(body,_("Active/Active Mode"), "%13");
		ioa_config_attr[index].option = 3;
		ioa_config_attr[index].active_active = ioa_attr.active_active;
		if (ioa_attr.active_active)
			sprintf(pref_str, "Enabled");
		else
			sprintf(pref_str, "Disabled");
		i_con = add_i_con(i_con, pref_str, &ioa_config_attr[index++]);
	}
	if (dev->ioa->has_cache && !dev->ioa->has_vset_write_cache) {
		body = add_line_to_body(body,_("IOA Caching Mode"), "%13");
		ioa_config_attr[index].option = 4;
		ioa_config_attr[index].caching = ioa_attr.caching;
		if (ioa_attr.caching == IPR_IOA_REQUESTED_CACHING_DEFAULT)
			sprintf(pref_str, "Default");
		else
			sprintf(pref_str, "Disabled");
		i_con = add_i_con(i_con, pref_str, &ioa_config_attr[index++]);
	}
	if (dev->ioa->configure_rebuild_verify) {
		body = add_line_to_body(body,
					_("Verify data during array rebuild"),
					"%13");
		ioa_config_attr[index].option = 5;
		ioa_config_attr[index].verify_array_rebuild
			= ioa_attr.disable_rebuild_verify;

		if (ioa_attr.disable_rebuild_verify == 1)
			sprintf(pref_str, "Disabled");
		else
			sprintf(pref_str, "Enabled");

		i_con = add_i_con(i_con, pref_str,
				  &ioa_config_attr[index++]);
	}

	if (dev->ioa->sis64) {
		body = add_line_to_body(body, _("Array rebuild rate"), "%3");
		ioa_config_attr[index].option = 6;
		ioa_config_attr[index].rebuild_rate = ioa_attr.rebuild_rate;

		sprintf(pref_str, "%d",
			(ioa_config_attr[index].rebuild_rate* 100) / 15);

		i_con = add_i_con(i_con, pref_str, &ioa_config_attr[index++]);
	}
	n_change_ioa_config.body = body;
	while (1) {
		s_out = screen_driver(&n_change_ioa_config, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);

		found = 0;
		for_each_icon(temp_i_con) {
			if (!temp_i_con->field_data[0]) {
				config_attr = (struct ioa_config_attr *)temp_i_con->data;
				rc = ioa_config_menu(config_attr, temp_i_con->y, header_lines);

				if (rc == CANCEL_FLAG)
					rc = RC_SUCCESS;

				if (config_attr->option == 1) {
					if (config_attr->preferred_primary)
						sprintf(temp_i_con->field_data, "Primary");
					else
						sprintf(temp_i_con->field_data, "None");
					ioa_attr.preferred_primary = config_attr->preferred_primary;
				} else if (config_attr->option == 2) {
					if (config_attr->gscsi_only_ha)
						sprintf(temp_i_con->field_data, "JBOD");
					else
						sprintf(temp_i_con->field_data, "Normal");
					ioa_attr.gscsi_only_ha = config_attr->gscsi_only_ha;
				} else if (config_attr->option == 3) {
					if (config_attr->active_active)
						sprintf(temp_i_con->field_data, "Enabled");
					else
						sprintf(temp_i_con->field_data, "Disabled");
					ioa_attr.active_active = config_attr->active_active;
				} else if (config_attr->option == 4) {
					if (config_attr->caching == IPR_IOA_REQUESTED_CACHING_DEFAULT)
						sprintf(temp_i_con->field_data, "Default");
					else
						sprintf(temp_i_con->field_data, "Disabled");
					ioa_attr.caching = config_attr->caching;
				} else if (config_attr->option == 5) {
					if (config_attr->verify_array_rebuild == 0) {
						sprintf(temp_i_con->field_data, "Enabled");
						ioa_attr.disable_rebuild_verify = 0;
					}
					else {
						sprintf(temp_i_con->field_data, "Disabled");
						ioa_attr.disable_rebuild_verify = 1;
					}
				} else if (config_attr->option == 6) {
						sprintf( temp_i_con->field_data, "%d",
							(config_attr->rebuild_rate * 100) / 15 );
						ioa_attr.rebuild_rate =
							config_attr->rebuild_rate;
				}
				found++;
				break;
			}
		}

		if (rc)
			goto leave;

		if (!found)
			break;
	}

	processing();
	rc = ipr_set_ioa_attr(dev->ioa, &ioa_attr, 1);
	check_current_config(false);

leave:
	free(n_change_ioa_config.body);
	n_change_ioa_config.body = NULL;
	free_i_con(i_con);
	i_con_head = i_con_head_saved;
	return rc;
}

/**
 * ioa_config -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int ioa_config(i_container * i_con)
{
	int rc, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;
	char *buffer[2];
	int toggle = 1;

	processing();
	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status(buffer, n_ioa_config.header, &header_lines);

	for_each_ioa(ioa) {
		if (ioa->ioa.scsi_dev_data == NULL)
			continue;
		if (!ioa->dual_raid_support && !ioa->gscsi_only_ha && !ioa->sis64)
			continue;

		print_dev(k, &ioa->ioa, buffer, "%1", k);
		i_con = add_i_con(i_con, "\0", &ioa->ioa);
		num_lines++;
	}

	if (num_lines == 0) {
		for (k = 0; k < 2; k++) {
			len = strlen(buffer[k]);
			buffer[k] = realloc(buffer[k], len +
						strlen(_(no_dev_found)) + 8);
			sprintf(buffer[k] + len, "\n%s", _(no_dev_found));
		}
	}

	do {
		n_ioa_config.body = buffer[toggle&1];
		s_out = screen_driver(&n_ioa_config, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_ioa_config.body = NULL;

	return rc;
}

/**
 * download_ucode -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int download_ucode(i_container * i_con)
{
	int rc, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev, *vset;
	struct screen_output *s_out;
	int header_lines = 0;
	char *buffer[2];
	int toggle = 0;

	processing();

	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status2(buffer, n_download_ucode.header, &header_lines, 16);

	for_each_ioa(ioa) {
		if (!ioa->ioa.scsi_dev_data || ioa->ioa_dead)
			continue;

		print_dev(k, &ioa->ioa, buffer, "%1", 10);
		i_con = add_i_con(i_con, "\0", &ioa->ioa);

		num_lines++;

		if (ioa->is_secondary)
			continue;

		num_lines += print_standalone_disks(ioa, &i_con, buffer, 10);
		num_lines += print_sas_ses_devices(ioa, &i_con, buffer, 10);
		num_lines += print_hotspare_disks(ioa, &i_con, buffer, 10);

		/* print volume set associated devices*/
		for_each_vset(ioa, vset) {
			num_lines++;

			for_each_dev_in_vset(vset, dev) {
				print_dev(k, dev, buffer, "%1", 10);
				i_con = add_i_con(i_con, "\0", dev);
				num_lines++;
			}
		}
	}

	if (num_lines == 0) {
		for (k = 0; k < 2; k++) {
			len = strlen(buffer[k]);
			buffer[k] = realloc(buffer[k], len +
						strlen(_(no_dev_found)) + 8);
			sprintf(buffer[k] + len, "\n%s", _(no_dev_found));
		}
	}

	do {
		n_download_ucode.body = buffer[toggle&1];
		s_out = screen_driver(&n_download_ucode, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_download_ucode.body = NULL;

	return rc;
}

/**
 * update_ucode -
 * @dev:		ipr dev struct
 * @fw_image:		ipr_fw_images struct
 *
 * Returns:
 *   0 if success / 67 | EXIT_FLAG on failure
 **/
static int update_ucode(struct ipr_dev *dev, struct ipr_fw_images *fw_image)
{
	char *body;
	int header_lines, status, time = 0;
	pid_t pid, done;
	u8 release_level[5];
	u32 fw_version;
	int rc = 0;

	body = body_init(n_download_ucode_in_progress.header, &header_lines);
	n_download_ucode_in_progress.body = body;
	time_screen_driver(&n_download_ucode_in_progress, time, 0);

	pid = fork();

	if (pid) {
		do {
			done = waitpid(pid, &status, WNOHANG);
			sleep(1);
			time++;
			time_screen_driver(&n_download_ucode_in_progress, time, 0);
		} while (done == 0);			
	} else {
		if (ipr_is_ioa(dev))
			ipr_update_ioa_fw(dev->ioa, fw_image, 1);
		else
			ipr_update_disk_fw(dev, fw_image, 1);
		exit(0);
	}

	ipr_get_fw_version(dev, release_level);

	fw_version = release_level[0] << 24 | release_level[1] << 16 |
		release_level[2] << 8 | release_level[3];

	if (fw_version != fw_image->version)
		rc = 67 | EXIT_FLAG;

	free(n_download_ucode_in_progress.body);
	n_download_ucode_in_progress.body = NULL;
	return rc;
}

struct download_ucode_elem {
	struct ipr_dev* dev;
	struct ipr_fw_images *lfw;
	struct download_ucode_elem *next;
};

/**
 * download_all_ucode -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int download_all_ucode(i_container *i_con)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct ipr_fw_images *lfw;
	struct screen_output *s_out = NULL;
	struct download_ucode_elem *elem, *update_list = NULL;
	int header_lines;
	char line[BUFSIZ];
	char *body;
	int rc = RC_93_All_Up_To_Date;

	processing();
	i_con = free_i_con(i_con);
	check_current_config(false);
	body = body_init(n_confirm_download_all_ucode.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->ioa.scsi_dev_data || ioa->ioa_dead)
			continue;

		for_each_dev (ioa, dev) {
			if (ipr_is_volume_set(dev))
				continue;

			lfw = get_latest_fw_image(dev);
			if (!lfw || lfw->version <= get_fw_version(dev)) {
				free(lfw);
				continue;
			}

			if (ioa->is_secondary) {
				free(lfw);
				continue;
			}

			sprintf(line, "%-6s %-10X %-10s %s\n",
				basename(dev->gen_name), lfw->version,
				lfw->date, lfw->file);

			body = add_string_to_body(body, line, "", &header_lines);

			/*  Add device to update list */
			elem = malloc(sizeof(struct download_ucode_elem));
			elem->dev = dev;
			elem->lfw = lfw;
			elem->next = update_list;
			update_list = elem;
		}
	}

	if (update_list) {
		n_confirm_download_all_ucode.body = body;
		s_out = screen_driver(&n_confirm_download_all_ucode, header_lines, i_con);
	}

	if (!s_out || s_out->rc != CANCEL_FLAG) {
		while (update_list) {
			elem = update_list;
			update_list = elem->next;
			update_ucode(elem->dev, elem->lfw);
			free(elem->lfw);
			free(elem);
		}
	}

	if (s_out) {
		rc = s_out->rc;
		free(s_out);
	}

	return rc;
}

/**
 * process_choose_ucode -
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / 67 | EXIT_FLAG on failure
 **/
int process_choose_ucode(struct ipr_dev *dev)
{
	struct ipr_fw_images *list;
	struct ipr_fw_images *fw_image;
	char *body = NULL;
	int header_lines = 0;
	int list_count, rc, i;
	char buffer[256];
	int found = 0;
	i_container *i_con_head_saved;
	i_container *i_con = NULL;
	i_container *temp_i_con;
	char *input;
	int version_swp;
	char *version;
	struct screen_output *s_out;
	u8 release_level[4];

	if (!dev->scsi_dev_data)
		return 67 | EXIT_FLAG;
	if (ipr_is_ioa(dev))
		rc = get_ioa_firmware_image_list(dev->ioa, &list);
	else if (dev->scsi_dev_data->type == TYPE_ENCLOSURE || dev->scsi_dev_data->type == TYPE_PROCESSOR)
		rc = get_ses_firmware_image_list(dev, &list);
	else
		rc = get_dasd_firmware_image_list(dev, &list);

	if (rc < 0)
		return 67 | EXIT_FLAG;
	else
		list_count = rc;

	ipr_get_fw_version(dev, release_level);

	if (ipr_is_ioa(dev)) {
		sprintf(buffer, _("Adapter to download: %-8s %-16s\n"),
			dev->scsi_dev_data->vendor_id,
			dev->scsi_dev_data->product_id);
		body = add_string_to_body(NULL, buffer, "", &header_lines);
		sprintf(buffer, _("Adapter Location: %s.%d/\n"),
			dev->ioa->pci_address, dev->ioa->host_num);

	} else {
		sprintf(buffer, _("Device to download: %-8s %-16s\n"),
			dev->scsi_dev_data->vendor_id,
			dev->scsi_dev_data->product_id);
		body = add_string_to_body(NULL, buffer, "", &header_lines);
		sprintf(buffer, _("Device Location: %d:%d:%d\n"),
			dev->res_addr[0].bus, dev->res_addr[0].target, dev->res_addr[0].lun);
	}

	body = add_string_to_body(body, buffer, "", &header_lines);

	if (isprint(release_level[0]) &&
	    isprint(release_level[1]) &&
	    isprint(release_level[2]) &&
	    isprint(release_level[3]))
		sprintf(buffer, "%s%02X%02X%02X%02X (%c%c%c%c)\n\n",
			_("The current microcode for this device is: "),
			release_level[0], release_level[1],
			release_level[2], release_level[3],
			release_level[0], release_level[1],
			release_level[2], release_level[3]);
	else
		sprintf(buffer, "%s%02X%02X%02X%02X\n\n",
			_("The current microcode for this device is: "),
			release_level[0], release_level[1],
			release_level[2], release_level[3]);

	body = add_string_to_body(body, buffer, "", &header_lines);
	body = __body_init(body, n_choose_ucode.header, &header_lines);

	i_con_head_saved = i_con_head;
	i_con_head = i_con = NULL;

	if (list_count) {
		for (i = 0; i < list_count; i++) {
			version_swp = htonl(list[i].version);
			version = (char *)&version_swp;
			if (isprint(version[0]) && isprint(version[1]) &&
			    isprint(version[2]) && isprint(version[3]))
				sprintf(buffer," %%1   %.8X (%c%c%c%c) %s %s",list[i].version,
					version[0], version[1], version[2],	version[3],
					list[i].date, list[i].file);
			else
				sprintf(buffer," %%1   %.8X        %s %s",list[i].version, list[i].date, list[i].file);

			body = add_line_to_body(body, buffer, NULL);
			i_con = add_i_con(i_con, "\0", &list[i]);
		}
	} else {
		body = add_line_to_body(body,"(No available images)",NULL);
	}

	n_choose_ucode.body = body;
	s_out = screen_driver(&n_choose_ucode, header_lines, i_con);
	free(s_out);

	free(n_choose_ucode.body);
	n_choose_ucode.body = NULL;

	for_each_icon(temp_i_con) {
		fw_image =(struct ipr_fw_images *)(temp_i_con->data);

		if (dev == NULL)
			continue;

		input = temp_i_con->field_data;

		if (input == NULL)
			continue;

		if (strcmp(input, "1") == 0) {
			found++;
			break;
		}
	}

	if (!found)
		goto leave;

	body = body_init(n_confirm_download_ucode.header, &header_lines);
	version_swp = htonl(fw_image->version);
	version = (char *)&version_swp;
	if (isprint(version[0]) && isprint(version[1]) &&
	    isprint(version[2]) && isprint(version[3]))
		sprintf(buffer," 1   %.8X (%c%c%c%c) %s %s\n",fw_image->version,
			version[0], version[1], version[2],	version[3],
			fw_image->date, fw_image->file);
	else
		sprintf(buffer," 1   %.8X        %s %s\n",fw_image->version,fw_image->date, fw_image->file);

	body = add_line_to_body(body, buffer, NULL);

	n_confirm_download_ucode.body = body;
	s_out = screen_driver(&n_confirm_download_ucode, header_lines, i_con);

	free(n_confirm_download_ucode.body);
	n_confirm_download_ucode.body = NULL;

	if (!s_out->rc)
		rc = update_ucode(dev, fw_image);

	free(s_out);

leave:
	free(list);
	i_con = free_i_con(i_con);
	i_con_head = i_con_head_saved;
	return rc;
}

/**
 * choose_ucode -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / 67 | EXIT_FLAG on failure
 **/
int choose_ucode(i_container * i_con)
{
	i_container *temp_i_con;
	struct ipr_dev *dev;
	char *input;
	int rc;
	int dev_num = 0;

	if (!i_con)
		return 1;

	for_each_icon(temp_i_con) {
		if (!temp_i_con->data)
			continue;
		if (!temp_i_con->field_data)
			continue;

		dev =(struct ipr_dev *)(temp_i_con->data);
		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			rc = process_choose_ucode(dev);
			if (rc & EXIT_FLAG)
				return rc;
			dev_num++;
		}
	}

	if (dev_num == 0)
		/* Invalid option.  No devices selected. */
		return 17; 

	return 0;
}

/**
* ucode_screen - Configure microcode screen
* @i_con:            i_container struct
*
* Returns:
*   0 if success / non-zero on failure
**/
int ucode_screen(i_container *i_con)
{
	return display_features_menu(i_con, &n_ucode_screen);
}

/**
* log_menu -
* @i_con:            i_container struct
*
* Returns:
*   0 if success / non-zero on failure FIXME
**/
int log_menu(i_container *i_con)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc, loop;
	struct stat file_stat;
	struct screen_output *s_out;
	int offset = 0;

	sprintf(cmnd,"%s/boot.msg",log_root_dir);
	rc = stat(cmnd, &file_stat);

	if (rc)
		/* file does not exist - do not display option */
		offset--;

	for (loop = 0; loop < (n_log_menu.num_opts + offset); loop++) {
		n_log_menu.body = ipr_list_opts(n_log_menu.body,
						 n_log_menu.options[loop].key,
						 n_log_menu.options[loop].list_str);
	}
	n_log_menu.body = ipr_end_list(n_log_menu.body);

	s_out = screen_driver(&n_log_menu, 0, NULL);
	free(n_log_menu.body);
	n_log_menu.body = NULL;
	rc = s_out->rc;
	i_con = s_out->i_con;
	i_con = free_i_con(i_con);
	free(s_out);
	return rc;
}

static int invoke_pager(char *filename)
{
	int pid, status;
	char *argv[] = {
		DEFAULT_EDITOR, "-c",
		filename, NULL
	};

	pid = fork();
	if (pid) {
		waitpid(pid, &status, 0);
	} else {
		/* disable insecure pager features */
		putenv("LESSSECURE=1");
		execvp(argv[0], argv);
		_exit(errno);
	}
	return WEXITSTATUS(status);
}

#define EDITOR_MAX_ARGS 32
static int invoke_editor(char* filename)
{
	int i;
	int rc, pid, status;

	/* if editor not set, use secure pager */
	if (strcmp(editor, DEFAULT_EDITOR) == 0) {
		rc = invoke_pager(filename);
		return rc;
	}

	pid = fork();
	if (pid == 0) {
		char *tok;
		char *argv[EDITOR_MAX_ARGS];
		char cmnd[MAX_CMD_LENGTH];

		/* copy editor name to argv[0] */
		strncpy(cmnd, editor, sizeof(cmnd) - 1);
		tok = strtok(cmnd, " ");
		argv[0] = malloc(strlen(tok));
		strcpy(argv[0], tok);

		/* handle editor arguments, if any */
		for (i = 1; i < EDITOR_MAX_ARGS - 2; ++i) {
			tok = strtok(NULL, " ");
			if (tok == NULL)
				break;
			argv[i] = malloc(strlen(tok));
			strcpy(argv[i], tok);
		}
		argv[i] = filename;
		argv[i+1] = NULL;

		execvp(argv[0], argv);
		_exit(errno);
	} else {
		waitpid(pid, &status, 0);
	}

	return WEXITSTATUS(status);
}

/**
 * ibm_storage_log_tail -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 on success / non-zero on failure
 **/
int ibm_storage_log_tail(i_container *i_con)
{
	int rc, len;
	int log_fd;
	char line[MAX_CMD_LENGTH];
	char logfile[MAX_CMD_LENGTH];
	char *tmp_log;
	FILE *logsource_fp;

	/* aux variables for erasing unnecessary log info */
	const char *local_s = "localhost kernel: ipr";
	const char *kernel_s = "kernel: ipr";
	char prefix[256];
	char host[256];
	char *dot;

	tmp_log = strdup(_PATH_TMP"iprerror-XXXXXX");
	log_fd = mkstemp(tmp_log);
	if (log_fd < 0) {
		s_status.str = strerror(errno);
		syslog(LOG_ERR, "Could not create tmp log file: %m\n");
		free(tmp_log);
		return RC_94_Tmp_Log_Fail;
	}

	def_prog_mode();
	endwin();

	snprintf(logfile, sizeof(logfile), "%s/messages", log_root_dir);
	logsource_fp = fopen(logfile, "r");
	if (!logsource_fp) {
		syslog(LOG_ERR, "Could not open %s: %m\n", logfile);
		free(tmp_log);
		close(log_fd);
		return RC_75_Failed_Read_Err_Log;
	}

	while (fgets(line, sizeof(line), logsource_fp)) {
		/* ignore lines that dont contain 'ipr' */
		if (strstr(line, "ipr") == NULL)
			continue;

		/* build prefix using current hostname */
		gethostname(host, sizeof(host));
		dot = strchr(host, '.');
		if (dot)
			*dot = '\0';
		snprintf(prefix, sizeof(prefix), "%s kernel: ipr", host);

		/* erase prefix, local_s and kernel_s from line:
		 * dot+strlen points to beginning of prefix
		 * strlen(dot) - strlen(prefix) + 1 == size of the rest of line
		 *
		 * code below moves 'rest of line' on top of prefix et al. */
		if (dot = strstr(line, prefix)) {
			memmove(dot, dot + strlen(prefix),
				strlen(dot) - strlen(prefix) + 1);
		} else if (dot = strstr(line, local_s)) {
			memmove(dot, dot+strlen(local_s),
				strlen(dot) - strlen(local_s) + 1);
		} else if (dot = strstr(line, kernel_s)) {
			memmove(dot, dot+strlen(kernel_s),
				strlen(dot) - strlen(kernel_s) + 1);
		}

		write(log_fd, line, strlen(line));
	}

	fclose(logsource_fp);
	close(log_fd);
	rc = invoke_editor(tmp_log);
	free(tmp_log);

	if ((rc != 0) && (rc != 127)) {
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return RC_65_Set_Default_Editor;
	} else {
		/* return with success */
		return RC_0_Success;
	}
}

/**
 * select_log_file -
 * @dir_entry:		dirent struct
 *
 * Returns:
 *   1 if "messages" if found in the dir_entry->d_name / 0 otherwise
 **/
static int select_log_file(const struct dirent *dir_entry)
{
	if (dir_entry) {
		if (strstr(dir_entry->d_name, "messages") == dir_entry->d_name)
			return 1;
	}
	return 0;
}

/**
 * compare_log_file -
 * @log_file1:		
 * @log_file2:		
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int compare_log_file(const struct dirent **first_dir, const struct dirent **second_dir)
{
	char name1[MAX_CMD_LENGTH], name2[MAX_CMD_LENGTH];
	struct stat stat1, stat2;

	if (strcmp((*first_dir)->d_name, "messages") == 0)
		return 1;
	if (strcmp((*second_dir)->d_name, "messages") == 0)
		return -1;

	sprintf(name1, "%s/%s", log_root_dir, (*first_dir)->d_name);
	sprintf(name2, "%s/%s", log_root_dir, (*second_dir)->d_name);

	if (stat(name1, &stat1))
		return 1;
	if (stat(name2, &stat2))
		return -1;

	if (stat1.st_mtime < stat2.st_mtime)
		return -1;
	if (stat1.st_mtime > stat2.st_mtime)
		return 1;
	return 0;
}

/**
 * ibm_storage_log -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 on success / non-zero on failure FIXME
 **/
int ibm_storage_log(i_container *i_con)
{
	char line[MAX_CMD_LENGTH];
	char logfile[MAX_CMD_LENGTH];
	int i;
	struct dirent **log_files;
	struct dirent **dirent;
	int num_dir_entries;
	int rc, len;
	int log_fd;
	char *tmp_log;
	gzFile logsource_fp;

	/* aux variables for erasing unnecessary log info */
	const char *local_s = "localhost kernel: ipr";
	const char *kernel_s = "kernel: ipr";
	char prefix[256];
	char host[256];
	char *dot;

	tmp_log = strdup(_PATH_TMP"iprerror-XXXXXX");
	log_fd = mkstemp(tmp_log);
	if (log_fd < 0) {
		s_status.str = strerror(errno);
		syslog(LOG_ERR, "Could not create tmp log file: %m\n");
		free(tmp_log);
		return RC_94_Tmp_Log_Fail;
	}

	def_prog_mode();
	endwin();

	num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
	if (num_dir_entries < 0) {
		s_status.num = 75;
		return RC_75_Failed_Read_Err_Log;
	}

	if (num_dir_entries)
		dirent = log_files;

	for (i = 0; i < num_dir_entries; ++i) {
		snprintf(logfile, sizeof(logfile), "%s/%s", log_root_dir,
			 (*dirent)->d_name);
		logsource_fp = gzopen(logfile, "r");
		if (logsource_fp == NULL) {
			syslog(LOG_ERR, "Could not open %s: %m\n", line);
			close(log_fd);
			continue; /* proceed to next log file */
		}

		while (gzgets(logsource_fp, line, sizeof(line))) {
			/* ignore lines that dont contain 'ipr' */
			if (strstr(line, "ipr") == NULL)
				continue;

			gethostname(host, sizeof(host));
			dot = strchr(host, '.');
			if (dot)
				*dot = '\0';
			snprintf(prefix, sizeof(prefix), "%s kernel: ipr",
				 host);

			/* erase prefix, local_s and kernel_s from line:
			 * dot+strlen points to beginning of prefix
			 * strlen(dot) - strlen(prefix) + 1 ==
			 *             size of the rest of line
			 *
			 * code below moves 'rest of line' on top of prefix */
			if (dot = strstr(line, prefix)) {
				memmove(dot, dot + strlen(prefix),
					strlen(dot) - strlen(prefix) + 1);
			} else if (dot = strstr(line, local_s)) {
				memmove(dot, dot + strlen(local_s),
					strlen(dot) - strlen(local_s) + 1);
			} else if (dot = strstr(line, kernel_s)) {
				memmove(dot, dot + strlen(kernel_s),
					strlen(dot) - strlen(kernel_s) + 1);
			}

			write(log_fd, line, strlen(line));
		}
		gzclose(logsource_fp);
		dirent++;
	}

	close(log_fd);
	rc = invoke_editor(tmp_log);
	free(tmp_log);

	if (num_dir_entries) {
		while (num_dir_entries--)
			free(log_files[num_dir_entries]);
		free(log_files);
	}

	if ((rc != 0) && (rc != 127)) {
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return RC_65_Set_Default_Editor;
	} else {
		/* return with success */
		return RC_0_Success;
	}
}

/**
 * kernel_log -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int kernel_log(i_container *i_con)
{
	char line[MAX_CMD_LENGTH];
	char logfile[MAX_CMD_LENGTH];
	int rc, i;
	struct dirent **log_files;
	struct dirent **dirent;
	int num_dir_entries;
	int len;
	int log_fd;
	char *tmp_log;
	gzFile logsource_fp;

	tmp_log = strdup(_PATH_TMP"iprerror-XXXXXX");
	log_fd = mkstemp(tmp_log);
	if (log_fd < 0) {
		s_status.str = strerror(errno);
		syslog(LOG_ERR, "Could not create tmp log file: %m\n");
		free(tmp_log);
		return RC_94_Tmp_Log_Fail;
	}

	def_prog_mode();
	endwin();

	num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
	if (num_dir_entries < 0) {
		s_status.num = 75;
		return RC_75_Failed_Read_Err_Log;
	}

	if (num_dir_entries)
		dirent = log_files;

	for (i = 0; i < num_dir_entries; ++i) {
		snprintf(logfile, sizeof(logfile), "%s/%s", log_root_dir,
			 (*dirent)->d_name);
		logsource_fp = gzopen(logfile, "r");
		if (logsource_fp == NULL) {
			syslog(LOG_ERR, "Could not open %s: %m\n", line);
			close(log_fd);
			continue; /* proceed to next log file */
		}

		while (gzgets(logsource_fp, line, sizeof(line)))
			write(log_fd, line, strlen(line));
		gzclose(logsource_fp);
		dirent++;
	}

	close(log_fd);
	rc = invoke_editor(tmp_log);
	free(tmp_log);

	if (num_dir_entries > 0) {
		while (num_dir_entries--)
			free(log_files[num_dir_entries]);
		free(log_files);
	}

	if ((rc != 0) && (rc != 127))	{
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return RC_65_Set_Default_Editor;
	}
	else {
		/* return with success */
		return RC_0_Success;
	}
}

/**
 * iprconfig_log -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int iprconfig_log(i_container *i_con)
{
	char line[MAX_CMD_LENGTH];
	char logfile[MAX_CMD_LENGTH];
	int rc, i;
	struct dirent **log_files;
	struct dirent **dirent;
	int num_dir_entries;
	int len;
	int log_fd;
	char *tmp_log;
	gzFile logsource_fp;

	tmp_log = strdup(_PATH_TMP"iprerror-XXXXXX");
	log_fd = mkstemp(tmp_log);
	if (log_fd < 0) {
		s_status.str = strerror(errno);
		syslog(LOG_ERR, "Could not create tmp log file: %m\n");
		free(tmp_log);
		return RC_94_Tmp_Log_Fail;
	}

	def_prog_mode();
	endwin();

	num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
	if (num_dir_entries < 0) {
		s_status.num = 75;
		return RC_75_Failed_Read_Err_Log;
	}

	if (num_dir_entries)
		dirent = log_files;

	for (i = 0; i < num_dir_entries; ++i) {
		snprintf(logfile, sizeof(logfile), "%s/%s", log_root_dir,
			 (*dirent)->d_name);
		logsource_fp = gzopen(logfile, "r");
		if (logsource_fp == NULL) {
			syslog(LOG_ERR, "Could not open %s: %m\n", line);
			close(log_fd);
			continue; /* proceed to next log file */
		}

		while (gzgets(logsource_fp, line, sizeof(line))) {
			/* ignore lines that dont contain 'iprconfig' */
			if (strstr(line, "iprconfig") == NULL)
				continue;
			write(log_fd, line, strlen(line));
		}
		gzclose(logsource_fp);
		dirent++;
	}

	close(log_fd);
	rc = invoke_editor(tmp_log);
	free(tmp_log);

	if (num_dir_entries) {
		while (num_dir_entries--)
			free(log_files[num_dir_entries]);
		free(log_files);
	}

	if ((rc != 0) && (rc != 127))	{
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return RC_65_Set_Default_Editor;
	} else {
		/* return with success */
		return RC_0_Success;
	}
}

/**
 * kernel_root -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int kernel_root(i_container *i_con)
{
	int rc;
	struct screen_output *s_out;
	char *body = NULL;

	i_con = free_i_con(i_con);

	/* i_con to return field data */
	i_con = add_i_con(i_con,"",NULL); 

	body = body_init(n_kernel_root.header, NULL);
	body = add_line_to_body(body,_("Current root directory"), log_root_dir);
	body = add_line_to_body(body,_("New root directory"), "%39");

	n_kernel_root.body = body;
	s_out = screen_driver(&n_kernel_root, 0, i_con);

	free(n_kernel_root.body);
	n_kernel_root.body = NULL;
	rc = s_out->rc;
	free(s_out);

	return rc;
}

/**
 * confirm_kernel_root -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int confirm_kernel_root(i_container *i_con)
{
	int rc;
	DIR *dir;
	char *input;
	struct screen_output *s_out;
	char *body = NULL;

	input = strip_trailing_whitespace(i_con->field_data);
	if (strlen(input) == 0)
		return (EXIT_FLAG | 61);

	dir = opendir(input);

	if (dir == NULL)
		/* "Invalid directory" */
		return 59; 
	else {
		closedir(dir);
		if (strcmp(log_root_dir, input) == 0)
			/*"Root directory unchanged"*/
			return (EXIT_FLAG | 61);
	}

	body = body_init(n_confirm_kernel_root.header, NULL);
	body = add_line_to_body(body,_("New root directory"), input);

	n_confirm_kernel_root.body = body;
	s_out = screen_driver(&n_confirm_kernel_root, 0, i_con);

	free(n_confirm_kernel_root.body);
	n_confirm_kernel_root.body = NULL;

	rc = s_out->rc;
	if (rc == 0) {
		/*"Root directory changed to %s"*/
		strcpy(log_root_dir, i_con->field_data);
		s_status.str = log_root_dir;
		rc = (EXIT_FLAG | 60); 
	}

	else
		/*"Root directory unchanged"*/
		rc = (EXIT_FLAG | 61); 

	free(s_out);
	return rc;
}

/**
 * set_default_editor -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int set_default_editor(i_container *i_con)
{
	int rc = 0;
	struct screen_output *s_out;
	char *body = NULL;

	i_con = free_i_con(i_con);
	i_con = add_i_con(i_con,"",NULL);

	body = body_init(n_set_default_editor.header, NULL);
	body = add_line_to_body(body, _("Current editor"), editor);
	body = add_line_to_body(body, _("New editor"), "%39");

	n_set_default_editor.body = body;
	s_out = screen_driver(&n_set_default_editor, 0, i_con);

	free(n_set_default_editor.body);
	n_set_default_editor.body = NULL;
	rc = s_out->rc;
	free(s_out);

	return rc;
}

/**
 * confirm_set_default_editor -
 * @i_con:		i_container struct
 *
 * Returns:
 *   EXIT_FLAG | 62 if editor changed / EXIT_FLAG | 63 if editor unchanged
 **/
int confirm_set_default_editor(i_container *i_con)
{
	int rc;
	char *input;
	struct screen_output *s_out;
	char *body = NULL;

	input = strip_trailing_whitespace(i_con->field_data);
	if (strlen(input) == 0)
		/*"Editor unchanged"*/
		return (EXIT_FLAG | 63); 

	if (strcmp(editor, input) == 0)
		/*"Editor unchanged"*/
		return (EXIT_FLAG | 63); 

	body = body_init(n_confirm_set_default_editor.header, NULL);
	body = add_line_to_body(body,_("New editor"), input);

	n_confirm_set_default_editor.body = body;
	s_out = screen_driver(&n_confirm_set_default_editor, 0, i_con);

	free(n_confirm_set_default_editor.body);
	n_confirm_set_default_editor.body = NULL;

	rc = s_out->rc;
	if (rc == 0) {
		/*"Editor changed to %s"*/
		strcpy(editor, i_con->field_data);
		s_status.str = editor;
		rc = (EXIT_FLAG | 62); 
	} else
		/*"Editor unchanged"*/
		rc = (EXIT_FLAG | 63); 

	free(s_out);
	return rc;
}

/**
 * restore_log_defaults -
 * @i_con:		i_container struct
 *
 * Returns:
 *   64
 **/
int restore_log_defaults(i_container *i_con)
{
	strcpy(log_root_dir, DEFAULT_LOG_DIR);
	strcpy(editor, DEFAULT_EDITOR);
	return 64; /*"Default log values restored"*/
}

/**
 * ibm_boot_log -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int ibm_boot_log(i_container *i_con)
{
	char line[MAX_CMD_LENGTH];
	char logfile[MAX_CMD_LENGTH];
	int rc;
	int len;
	struct stat file_stat;
	int log_fd;
	char *tmp_log;
	FILE *logsource_fp;

	sprintf(line,"%s/boot.msg",log_root_dir);
	rc = stat(line, &file_stat);
	if (rc)
		return 2; /* "Invalid option specified" */

	tmp_log = strdup(_PATH_TMP"iprerror-XXXXXX");
	log_fd = mkstemp(tmp_log);
	if (log_fd < 0) {
		s_status.str = strerror(errno);
		syslog(LOG_ERR, "Could not create temp log file: %m\n");
		free(tmp_log);
		return RC_94_Tmp_Log_Fail;
	}

	def_prog_mode();
	endwin();

	snprintf(logfile, sizeof(logfile), "%s/boot.msg", log_root_dir);
	logsource_fp = fopen(logfile, "r");
	if (!logsource_fp) {
		syslog(LOG_ERR, "Could not open %s: %m\n", line);
		free(tmp_log);
		close(log_fd);
		return RC_75_Failed_Read_Err_Log;
	}

	while (fgets(line, sizeof(line), logsource_fp)) {
		/* ignore lines that dont contain 'ipr' */
		if (strstr(line, "ipr") == NULL)
			continue;

		write(log_fd, line, strlen(line));
	}

	fclose(logsource_fp);
	close(log_fd);
	rc = invoke_editor(tmp_log);
	free(tmp_log);

	if ((rc != 0) && (rc != 127)) {
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return RC_65_Set_Default_Editor;
	} else {
		/* return with no status */
		return RC_0_Success;
	}
}

int get_ses_ioport_status(struct ipr_dev *ses)
{
	struct ipr_query_io_port io_status;
	int rc = ipr_query_io_dev_port(ses, &io_status);

	if (!rc)
		ses->active_suspend = io_status.port_state;
	else
		ses->active_suspend = IOA_DEV_PORT_UNKNOWN;
	
	return rc;
}

/**
 * get_status -
 * @dev:		ipr dev struct
 * @buf:		data buffer
 * @percent:		
 * @path_status:	
 *
 * Returns:
 *   nothing
 **/
static void get_status(struct ipr_dev *dev, char *buf, int percent, int path_status)
{
	struct scsi_dev_data *scsi_dev_data = dev->scsi_dev_data;
	struct ipr_ioa *ioa = dev->ioa;
	struct ipr_query_res_state res_state;
	int status, rc;
	int format_req = 0;
	int blk_size = 0;
	struct ipr_mode_pages mode_pages;
	struct ipr_block_desc *block_desc;
	struct sense_data_t sense_data;
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int percent_cmplt = 0;
	int format_in_progress = 0;
	int resync_in_progress = 0;
	int initialization_in_progress = 0;
	struct ipr_res_redundancy_info info;

	if (ipr_is_ioa(dev)) {
		if (!scsi_dev_data->online)
			sprintf(buf, "Offline");
		else if (ioa->ioa_dead)
			sprintf(buf, "Not Operational");
		else if (ioa->nr_ioa_microcode)
			sprintf(buf, "Not Ready");
		else
			sprintf(buf, "Operational");
	} else if (scsi_dev_data && scsi_dev_data->type == IPR_TYPE_EMPTY_SLOT) {
		sprintf(buf, "Empty");
	} else {
		if (ipr_is_volume_set(dev) || ipr_is_array(dev)) {
			rc = ipr_query_command_status(&ioa->ioa, &cmd_status);

			if (!rc) {
				for_each_cmd_status(status_record, &cmd_status) {
					if (dev->array_id != status_record->array_id)
						continue;
					if (status_record->status != IPR_CMD_STATUS_IN_PROGRESS)
						continue;
					if (status_record->command_code == IPR_RESYNC_ARRAY_PROTECTION)
						resync_in_progress = 1;
					if (status_record->command_code == IPR_START_ARRAY_PROTECTION)
						initialization_in_progress = 1;
					percent_cmplt = status_record->percent_complete;
				}
			}
		} else if (ipr_is_af_dasd_device(dev)) {
			rc = ipr_query_command_status(dev, &cmd_status);

			if ((rc == 0) && (cmd_status.num_records != 0)) {
				status_record = cmd_status.record;
				if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL &&
				    status_record->status != IPR_CMD_STATUS_FAILED) {
					percent_cmplt = status_record->percent_complete;
					format_in_progress++;
				}
			}
		} else if (ipr_is_gscsi(dev)) {
			/* Send Test Unit Ready to find percent complete in sense data. */
			rc = ipr_test_unit_ready(dev, &sense_data);

			if ((rc == CHECK_CONDITION) &&
			    ((sense_data.error_code & 0x7F) == 0x70) &&
			    ((sense_data.sense_key & 0x0F) == 0x02) && /* NOT READY */
			    (sense_data.add_sense_code == 0x04) &&     /* LOGICAL UNIT NOT READY */
			    (sense_data.add_sense_code_qual == 0x04)) {/* FORMAT IN PROGRESS */

				percent_cmplt = ((int)sense_data.sense_key_spec[1]*100)/0x100;
				format_in_progress++;
			}
		}

		memset(&res_state, 0, sizeof(res_state));

		if (ipr_is_af(dev)) {
			rc = ipr_query_resource_state(dev, &res_state);

			if (rc != 0)
				res_state.not_oper = 1;
		} else if (ipr_is_gscsi(dev)) {
			format_req = 0;

			/* Issue mode sense/mode select to determine if device needs to be formatted */
			status = ipr_mode_sense(dev, 0x0a, &mode_pages);

			if (status == CHECK_CONDITION &&
			    sense_data.add_sense_code == 0x31 &&
			    sense_data.add_sense_code_qual == 0x00) {
				format_req = 1;
			} else {
				if (!status) {
					if (mode_pages.hdr.block_desc_len > 0) {
						block_desc = (struct ipr_block_desc *)(mode_pages.data);
						blk_size = block_desc->block_length[2] +
							   (block_desc->block_length[1] << 8) +
							   (block_desc->block_length[0] << 16);

						if ((blk_size != IPR_JBOD_BLOCK_SIZE) &&
						    (blk_size != IPR_JBOD_4K_BLOCK_SIZE))
							format_req = 1;
					}
				}

				/* check device with test unit ready */
				rc = ipr_test_unit_ready(dev, &sense_data);

				if (rc == CHECK_CONDITION &&
				    sense_data.add_sense_code == 0x31 &&
				    sense_data.add_sense_code_qual == 0x00)
					format_req = 1;
				else if (rc)
					res_state.not_oper = 1;
			}
		}

		if (path_status)
			rc = ipr_query_res_redundancy_info(dev, &info);

		if (ioa->ioa_dead)
			sprintf(buf, "Unknown");
		else if (path_status && !rc) {
			if (info.healthy_paths > 1)
				sprintf(buf, "Redundant");
			else if (info.healthy_paths)
				sprintf(buf, "Single");
			else
				sprintf(buf, "No Paths");
		} else if (format_in_progress)
			sprintf(buf, "%d%% Formatted", percent_cmplt);
		else if (ioa->sis64 && ipr_is_remote_af_dasd_device(dev)) {
			if (!scsi_dev_data)
				sprintf(buf, "Missing");
			else
				sprintf(buf, "Remote");
			}
		else if (!ioa->sis64 && ioa->is_secondary && !scsi_dev_data)
			sprintf(buf, "Remote");
		else if (!scsi_dev_data)
			sprintf(buf, "Missing");
		else if (!scsi_dev_data->online) {
				if (dev->scsi_dev_data->type == TYPE_ENCLOSURE) {
					get_ses_ioport_status(dev);
					if (dev->active_suspend == IOA_DEV_PORT_SUSPEND)
						sprintf(buf, "Suspend");
					else
						sprintf(buf, "Offline");
				}
			}
		else if (res_state.not_oper || res_state.not_func)
			sprintf(buf, "Failed");
		else if (res_state.read_write_prot)
			sprintf(buf, "R/W Protected");
		else if (res_state.prot_dev_failed)
			sprintf(buf, "Failed");
		else if (ipr_is_volume_set(dev) || ipr_is_array(dev)) {
			if (res_state.prot_resuming) {
				if (!percent || (percent_cmplt == 0))
					sprintf(buf, "Rebuilding");
				else
					sprintf(buf, "%d%% Rebuilt", percent_cmplt);
			} else if (resync_in_progress) {
				if (!percent || (percent_cmplt == 0))
					sprintf(buf, "Checking");
				else
					sprintf(buf, "%d%% Checked", percent_cmplt);
			} else if (initialization_in_progress) {
				if ((!percent || (percent_cmplt == 0)))
					sprintf(buf, "Initializing");
				else
					sprintf(buf, "%d%% Initializing", percent_cmplt);
			} else if (res_state.prot_suspended)
				sprintf(buf, "Degraded");
			else if (res_state.degraded_oper || res_state.service_req)
				sprintf(buf, "Degraded");
			else if ((dev->ioa->asymmetric_access &&
				 dev->array_rcd->current_asym_access_state == IPR_ACTIVE_OPTIMIZED)
				 || (!dev->ioa->is_secondary && !dev->ioa->asymmetric_access))
				sprintf(buf, "Optimized");
			else
				sprintf(buf, "Non-Optimized");
		} else if (format_req)
			sprintf(buf, "Format Required");
		else if (ipr_device_is_zeroed(dev))
			sprintf(buf, "Zeroed");
		else
			sprintf(buf, "Active");
	}
}

static const struct {
	u8 status;
	char *desc;
} path_status_desc[] = {
	{ IPR_PATH_CFG_NO_PROB, "Functional" },
	{ IPR_PATH_CFG_DEGRADED, "Degraded" },
	{ IPR_PATH_CFG_FAILED, "Failed" },
	{ IPR_PATH_CFG_SUSPECT, "Suspect" },
	{ IPR_PATH_NOT_DETECTED, "Missing" },
	{ IPR_PATH_INCORRECT_CONN, "Incorrect connect" }
};

/**
 * get_phy_state -
 * @state:		state value
 *
 * Returns:
 *   state description
 **/
static const char *get_phy_state(u8 state)
{
	int i;
	u8 mstate = state & IPR_PATH_CFG_STATUS_MASK;

	for (i = 0; i < ARRAY_SIZE(path_status_desc); i++) {
		if (path_status_desc[i].status == mstate)
			return path_status_desc[i].desc;
	}

	return "Unknown";
}

static const struct {
	u8 type;
	char *desc;
} path_type_desc[] = {
	{ IPR_PATH_CFG_IOA_PORT, "IOA port" },
	{ IPR_PATH_CFG_EXP_PORT, "Expander port" },
	{ IPR_PATH_CFG_DEVICE_PORT, "Device port" },
	{ IPR_PATH_CFG_DEVICE_LUN, "Device LUN" }
};

/**
 * get_phy_type -
 * @type:		type value
 *
 * Returns:
 *   type description
 **/
static const char *get_phy_type(u8 type)
{
	int i;
	u8 mtype = type & IPR_PATH_CFG_TYPE_MASK;

	for (i = 0; i < ARRAY_SIZE(path_type_desc); i++) {
		if (path_type_desc[i].type == mtype)
			return path_type_desc[i].desc;
	}

	return "Unknown";
}

static const char *link_rate[] = {
	"Enabled",
	"Disabled",
	"Reset Prob",
	"SpinupHold",
	"Port Selec",
	"Resetting",
	"Unknown",
	"Unknown",
	"1.5Gbps",
	"3.0Gbps",
	"6.0Gbps",
	"12.0Gbps",
	"22.5Gbps",
	"Enabled",
	"Enabled",
	"Enabled"
};

static const struct {
	u8 state;
	char *desc;
} path_state_desc[] = {
	{ IPR_PATH_STATE_NO_INFO, "Unknown" },
	{ IPR_PATH_HEALTHY, "Healthy" },
	{ IPR_PATH_DEGRADED, "Degraded" },
	{ IPR_PATH_FAILED, "Failed" }
};

/**
 * get_path_state -
 * @path_state:		path_state value
 *
 * Returns:
 *   path state description
 **/
static const char *get_path_state(u8 path_state)
{
	int i;
	u8 state = path_state & IPR_PATH_STATE_MASK;

	for (i = 0; i < ARRAY_SIZE(path_state_desc); i++) {
		if (path_state_desc[i].state == state)
			return path_state_desc[i].desc;
	}

	return "Unknown";
}

/**
 * print_phy -
 * @cfg:		ipr_fabric_config_element struct
 * @body:		message buffer
 *
 * Returns:
 *   body
 **/
static char *print_phy(struct ipr_fabric_config_element *cfg, char *body)
{
	int len = strlen(body);

	body = realloc(body, len + 256);
	body[len] = '\0';

	len += sprintf(body + len, "%2X/%08X%08X    ", cfg->phy,
		       ntohl(cfg->wwid[0]), ntohl(cfg->wwid[1]));

	len += sprintf(body + len, " %-17s       ", get_phy_type(cfg->type_status));
	len += sprintf(body + len, " %-17s", get_phy_state(cfg->type_status));
	len += sprintf(body + len, " %s \n",
		       link_rate[cfg->link_rate & IPR_PHY_LINK_RATE_MASK]);
	return body;
}

/**
 * print_phy64 -
 * @cfg:		ipr_fabric_config_element struct
 * @body:		message buffer
 *
 * Returns:
 *   body
 **/
static char *print_phy64(struct ipr_fabric_config_element64 *cfg, char *body, int res_path_len)
{
	int i, ff_len;
	int len = strlen(body);
	char buffer[IPR_MAX_RES_PATH_LEN];

	body = realloc(body, len + 256);
	body[len] = '\0';

	ipr_format_res_path_wo_hyphen(cfg->res_path, buffer, IPR_MAX_RES_PATH_LEN);
	ff_len = res_path_len - strlen(buffer);
	for ( i = 0; i < ff_len;  i++)
		strncat(buffer, "F", strlen("F"));

	len += sprintf(body + len, "%s", buffer);

	len += sprintf(body + len, "/%08X%08X",
		       ntohl(cfg->wwid[0]), ntohl(cfg->wwid[1]));
	len += sprintf(body + len, " %-17s       ", get_phy_type(cfg->type_status));
	len += sprintf(body + len, " %-17s", get_phy_state(cfg->type_status));
	len += sprintf(body + len, " %s \n",
		       link_rate[cfg->link_rate & IPR_PHY_LINK_RATE_MASK]);
	return body;
}

/**
 * print_path -
 * @ioa:		ipr_fabric_descriptor struct
 * @body:		message buffer
 *
 * Returns:
 *   body
 **/
static char *print_path(struct ipr_fabric_descriptor *fabric, char *body)
{
	int len = strlen(body);
	struct ipr_fabric_config_element *cfg;

	body = realloc(body, len + 256);
	body[len] = '\0';

	len += sprintf(body + len, "%2X/                    "
		       "Physical Path      ", fabric->ioa_port);

	if (fabric->path_state & IPR_PATH_ACTIVE)
		len += sprintf(body + len, " Yes   ");
	else if (fabric->path_state & IPR_PATH_NOT_ACTIVE)
		len += sprintf(body + len, " No    ");
	else 
		len += sprintf(body + len, " ???   ");

	len += sprintf(body + len, "%-17s \n", get_path_state(fabric->path_state));

	for_each_fabric_cfg(fabric, cfg)
		body = print_phy(cfg, body);

	return body;
}

/**
 * print_path64 -
 * @ioa:		ipr_fabric_descriptor64 struct
 * @body:		message buffer
 *
 * Returns:
 *   body
 **/
static char *print_path64(struct ipr_fabric_descriptor64 *fabric, char *body)
{
	int len = strlen(body);
	struct ipr_fabric_config_element64 *cfg;
	char buffer[IPR_MAX_RES_PATH_LEN];
	int max_res_path_len = 0;

	body = realloc(body, len + 256);
	body[len] = '\0';

	ipr_format_res_path_wo_hyphen(fabric->res_path, buffer, IPR_MAX_RES_PATH_LEN);
	len += sprintf(body + len, "%s                "
		       "Resource Path    ", buffer);

	if (fabric->path_state & IPR_PATH_ACTIVE)
		len += sprintf(body + len, " Yes    ");
	else if (fabric->path_state & IPR_PATH_NOT_ACTIVE)
		len += sprintf(body + len, " No     ");
	else
		len += sprintf(body + len, " ???    ");

	len += sprintf(body + len, "%-17s \n", get_path_state(fabric->path_state));

	for_each_fabric_cfg(fabric, cfg) {
		ipr_format_res_path_wo_hyphen(cfg->res_path, buffer, IPR_MAX_RES_PATH_LEN);
		if (strlen(buffer) > max_res_path_len)
			max_res_path_len = strlen(buffer);
	}

	for_each_fabric_cfg(fabric, cfg) {
		body = print_phy64(cfg, body, max_res_path_len);
	}

	return body;
}

/**
 * print_path_details -
 * @dev:		ipr dev struct
 * @body:		message buffer
 *
 * Returns:
 *   body
 **/
static char *print_path_details(struct ipr_dev *dev, char *body)
{
	int rc, len = 0;
	struct ipr_res_redundancy_info info;
	struct ipr_fabric_descriptor *fabric;
	struct ipr_fabric_descriptor64 *fabric64;

	rc = ipr_query_res_redundancy_info(dev, &info);

	if (rc)
		return body;

	if (body)
		len = strlen(body);
	body = realloc(body, len + 256);
	body[len] = '\0';

	if (dev->ioa->sis64)
		for_each_fabric_desc64(fabric64, &info)
			body = print_path64(fabric64, body);
	else
		for_each_fabric_desc(fabric, &info)
			body = print_path(fabric, body);

	return body;
}

/**
 * __print_device -
 * @dev:		ipr dev struct
 * @body:		message buffer
 * @option:		
 * @sd:			
 * @sg:			
 * @vpd:		
 * @percent:		
 * @indent:		
 * @res_path:		
 * @ra:			
 * @path_status:	
 *
 * Returns:
 *   char * buffer
 **/
char *__print_device(struct ipr_dev *dev, char *body, char *option,
		     int sd, int sg, int vpd, int percent, int indent,
		     int res_path, int ra, int path_status,
		     int hw_loc, int conc_main, int ioa_flag, int serial_num,
		     int ucode, int skip_status, int skip_vset)
{
	u16 len = 0;
	struct scsi_dev_data *scsi_dev_data = dev->scsi_dev_data;
	int i;
	char *dev_name = dev->dev_name;
	char *gen_name = dev->gen_name;
	char node_name[7], buf[100], raid_str[48];
	char res_path_name[IPR_MAX_RES_PATH_LEN];
	int tab_stop = 0;
	int loc_len = 0;
	char vendor_id[IPR_VENDOR_ID_LEN + 1];
	char product_id[IPR_PROD_ID_LEN + 1];
	struct ipr_ioa *ioa = dev->ioa, *ioa_phy_loc;
	bool is4k = false, isri = false;

	/* In cases where we're having problems with the device */
	if (!ioa)
		return body;

	if (skip_vset && ipr_is_volume_set (dev))
		return body;

	if (ioa_flag)
		ioa_phy_loc = dev->ioa;

	if (body)
		len = strlen(body);
	body = realloc(body, len + 256);

	if (sd && strlen(dev_name) > 5)
		ipr_strncpy_0(node_name, &dev_name[5], 6);
	else if (sg && (strlen(gen_name) > 5))
		ipr_strncpy_0(node_name, &gen_name[5], 6);
	else
		node_name[0] = '\0';

	if (option)
		len += sprintf(body + len, " %s  ", option);

	if (scsi_dev_data && scsi_dev_data->type == TYPE_ENCLOSURE && (serial_num == 1 || hw_loc == 1)) 
		len += sprintf(body + len, "%-8s ", node_name);
	else
		len += sprintf(body + len, "%-6s ", node_name);

	if (hw_loc) {
		if (ioa_flag) {
			if (strlen(ioa_phy_loc->physical_location)) {
				loc_len = sprintf(body + len, "%-40s ", ioa_phy_loc->physical_location);
				len += loc_len;
			}
			else {
				for (i = 0; i < 40-loc_len; i++)
					body[len+i] = ' ';

				len += 40-loc_len;
			}
		} else {
			if (strlen(dev->physical_location)) {
				if (conc_main) {
					loc_len = sprintf(body + len, "%-26s ", dev->physical_location);
					len += loc_len;
				} else {
					loc_len = sprintf(body + len, "%-38s ", dev->physical_location);
					len += loc_len;
				}
			} else {
				if (conc_main) {
					loc_len = sprintf(body + len, "%d/%d:", ioa->host_num,ioa->host_num);
					len += loc_len;
				} else {
					for (i = 0; i < 39-loc_len; i++)
						body[len+i] = ' ';

					len += 39-loc_len;
				}
			}
		}
	}
	else {
		if (res_path) {
			if (ioa->sis64)
				if (serial_num == 1) {
					if (ipr_is_ioa(dev))
						loc_len = sprintf(body + len, "%s/%-28d", ioa->pci_address, ioa->host_num);
					else {
						ipr_format_res_path(dev->res_path[ra].res_path_bytes, res_path_name, IPR_MAX_RES_PATH_LEN);
						loc_len = sprintf(body + len, "%s/%d/%-24s", ioa->pci_address, ioa->host_num, res_path_name);
					}
				} else if (conc_main) {
			
					ipr_format_res_path(dev->res_path[ra].res_path_bytes, res_path_name, IPR_MAX_RES_PATH_LEN);
					loc_len = sprintf(body + len, "%d/%-26s ", ioa->host_num, res_path_name);
				 } else
					loc_len = sprintf(body + len, "%-26s ",scsi_dev_data ?
					scsi_dev_data->res_path : "<unknown>");
			else /*32bit*/
				if (serial_num == 1) {
					if (ipr_is_ioa(dev))
						loc_len = sprintf(body + len, "%s/%-29d:", ioa->pci_address, ioa->host_num);
					else
						loc_len = sprintf(body + len, "%s/%d", ioa->pci_address, ioa->host_num);
				} else if (conc_main)
					loc_len = sprintf(body + len, "%d/%d:", ioa->host_num,ioa->host_num);
				else
					loc_len = sprintf(body + len, "%d:", ioa->host_num);

			len += loc_len;
		}
		else {
			loc_len = sprintf(body + len, "%s/%d:",
				 ioa->pci_address,
				 ioa->host_num);
			len += loc_len;
		}
	}

	if (ipr_is_ioa(dev)) {
		if (serial_num == 1 ) {
			if (!res_path || !ioa->sis64) {
				for (i = 0; i < 28-loc_len; i++)
					body[len+i] = ' ';

				len += 28-loc_len;
			}

			len += sprintf(body + len,"%-13s ",
				       ioa->yl_serial_num);

		} else if (hw_loc == 1 ) {
				if (!res_path || !ioa->sis64) {
					for (i = 0; i < 40-loc_len; i++)
						body[len+i] = ' ';

					len += 40-loc_len;
				}
				len += sprintf(body + len,"%-19s ",
				       scsi_dev_data->product_id);
			}
		else { 
			if (!res_path || !ioa->sis64) {
				for (i = 0; i < 27-loc_len; i++)
					body[len+i] = ' ';

				len += 27-loc_len;
			}

			if (!vpd) {
				tab_stop = sprintf(body + len,"%s %s", get_bus_desc(ioa),
						   get_ioa_desc(dev->ioa));

				len += tab_stop;

				for (i = 0; i < 29-tab_stop; i++)
					body[len+i] = ' ';

				len += 29-tab_stop;
			} else 
				len += sprintf(body + len,"%-8s %-19s ",
					       scsi_dev_data->vendor_id,
					       scsi_dev_data->product_id);
		}
	} else if (scsi_dev_data && scsi_dev_data->type == IPR_TYPE_EMPTY_SLOT) {
		if (!res_path || !ioa->sis64) {
			if (hw_loc) {
				for (i = 0; i < 27-loc_len; i++)
					body[len+i] = ' ';

				len += 27-loc_len;
			}
			else {
				tab_stop = sprintf(body + len,"%d:%d: ",
						   dev->res_addr[ra].bus,
						   dev->res_addr[ra].target);

				loc_len += tab_stop;
				len += tab_stop;
				
				for (i = 0; i < 29-loc_len; i++)
					body[len+i] = ' ';

				len += 29-loc_len;
			}
		}
		len += sprintf(body + len, "%-8s %-19s ", " ", " ");
	} else {
		if (serial_num) {
			if (!res_path || !ioa->sis64) {
				for (i = 0; i < 26-loc_len; i++)
					body[len+i] = ' ';

				len += 26-loc_len;
			}
		}
		else  if (hw_loc) {
			if (!res_path || !ioa->sis64) {
				for (i = 0; i < 27-loc_len; i++)
					body[len+i] = ' ';

				len += 27-loc_len;
			}
		}
		else {
			if (!res_path || !ioa->sis64) {
				tab_stop = sprintf(body + len,"%d:%d:%d ", dev->res_addr[ra].bus,
						   dev->res_addr[ra].target, dev->res_addr[ra].lun);

				loc_len += tab_stop;
				len += tab_stop;

				if (conc_main && (hw_loc != 1)) {
					for (i = 0; i < 29-loc_len; i++)
						body[len+i] = ' ';

					len += 29-loc_len;
				} else {
					for (i = 0; i < 27-loc_len; i++)
						body[len+i] = ' ';

					len += 27-loc_len;
				}
			}
		}

		if (vpd) {
			if (scsi_dev_data) {
				if (dev->qac_entry && ipr_is_device_record(dev->qac_entry->record_id)) {
					ipr_strncpy_0(vendor_id, (char *)dev->vendor_id, IPR_VENDOR_ID_LEN);
					ipr_strncpy_0(product_id, (char *)dev->product_id, IPR_PROD_ID_LEN);
			} else {
				ipr_strncpy_0(vendor_id, scsi_dev_data->vendor_id, IPR_VENDOR_ID_LEN);
				ipr_strncpy_0(product_id, scsi_dev_data->product_id, IPR_PROD_ID_LEN);
				}
			} else if (dev->qac_entry) {
				if (ipr_is_device_record(dev->qac_entry->record_id)) {
						ipr_strncpy_0(vendor_id, (char *)dev->vendor_id, IPR_VENDOR_ID_LEN);
						ipr_strncpy_0(product_id, (char *)dev->product_id, IPR_PROD_ID_LEN);
				} else if (ipr_is_vset_record(dev->qac_entry->record_id)) {
					ipr_strncpy_0(vendor_id, (char *)dev->vendor_id, IPR_VENDOR_ID_LEN);
					ipr_strncpy_0(product_id, (char *)dev->product_id, IPR_PROD_ID_LEN);
				}
			}
			if (hw_loc) {
				len += sprintf(body + len, "%-19s ",
						product_id);
			}
			else {
				len += sprintf(body + len, "%-8s %-19s ",
					       vendor_id, product_id);
			}

		} else {
			if (ioa->support_4k && dev->block_dev_class & IPR_BLK_DEV_CLASS_4K)
				is4k = true;
			else
				is4k = false;

			if (ipr_is_hot_spare(dev)) {
				if (dev->block_dev_class & IPR_SSD) {
					if (dev->read_intensive & IPR_RI)
						isri = true;
					else
						isri = false;
					sprintf(buf, "%s%sSSD Hot Spare", is4k ? "4K " : "", isri ? "RI " : "");
				} else
					sprintf(buf, "%s Hot Spare", is4k ? "4K " : "");
				len += sprintf(body + len, "%-28s ", buf);
			} else if (ipr_is_volume_set(dev) || ipr_is_array(dev)) {
				if (dev->block_dev_class & IPR_SSD) {
					if (dev->read_intensive & IPR_RI)
						isri = true;
					else
						isri = false;
					sprintf(buf, "RAID %s%s%s SSD Array",
						get_prot_level_str(ioa->supported_arrays, dev->raid_level),
						is4k ? " 4K" : "", isri ? " RI" : "");
				} else
					sprintf(buf, "RAID %s%s Array",
						get_prot_level_str(ioa->supported_arrays, dev->raid_level),
						is4k ? " 4K" : "");
				len += sprintf(body + len, "%-28s ", buf);
			} else if (ipr_is_array_member(dev)) {
				if (indent)
					if (dev->block_dev_class & IPR_SSD) {
						if (dev->read_intensive & IPR_RI)
							isri = true;
						else
							isri = false;
						sprintf(raid_str,"  RAID %s%s%s SSD Member",
							dev->prot_level_str, is4k ? " 4K" : "", isri ? " RI" : "");
					} else
						sprintf(raid_str,"  RAID %s%s Array Member",
							dev->prot_level_str, is4k ? " 4K" : "");
				else
					if (dev->block_dev_class & IPR_SSD) {
						if (dev->read_intensive & IPR_RI)
							isri = true;
						else
							isri = false;
						sprintf(raid_str,"RAID %s%s SSD %s Member",
							dev->prot_level_str, is4k ? " 4K" : "", isri ? " RI" : "");
					} else
						sprintf(raid_str,"RAID %s%s Array Member",
							dev->prot_level_str, is4k ? " 4K" : "");

				len += sprintf(body + len, "%-28s ", raid_str);
			} else if (ipr_is_af_dasd_device(dev))
				if (dev->block_dev_class & IPR_SSD) {
					if (dev->read_intensive & IPR_RI)
						len += sprintf(body + len, "%-28s ", is4k ? "Advanced Function 4K RI SSD" :
								"Advanced Function RI SSD");
					else
						len += sprintf(body + len, "%-28s ", is4k ? "Advanced Function 4K SSD" :
								"Advanced Function SSD");
				} else
					len += sprintf(body + len, "%-28s ", is4k ? "Advanced Function 4K Disk" :
							"Advanced Function Disk");
			else if (scsi_dev_data && scsi_dev_data->type == TYPE_ENCLOSURE) {
				if (serial_num == 1)
					len += sprintf(body + len, "%-13s ", (char *)&dev->serial_number);
				else
					len += sprintf(body + len, "%-28s ", "Enclosure");

			} else if (scsi_dev_data && scsi_dev_data->type == TYPE_PROCESSOR)
				len += sprintf(body + len, "%-28s ", "Processor");
			else if (scsi_dev_data && scsi_dev_data->type == TYPE_ROM)
				len += sprintf(body + len, "%-28s ", "CD/DVD");

			else if (scsi_dev_data && scsi_dev_data->type == TYPE_TAPE)
				len += sprintf(body + len, "%-28s ", "Tape");

			else if (ioa->ioa_dead)
				len += sprintf(body + len, "%-28s ", "Unavailable Device");
			else {
				len += sprintf(body + len, "%-28s ",
						is4k ? "Physical 4K Disk" : "Physical Disk");
			}
		}
	}

	if (ucode) {
		int cur_ulevel = get_fw_version(dev);
		int newest_ulevel = get_latest_fw_image_version(dev);
		char updatable;
		newest_ulevel = (newest_ulevel < 0) ?
			cur_ulevel:newest_ulevel;
		updatable = (cur_ulevel < newest_ulevel) ? '*':' ';
		len +=  sprintf(body + len, "%-10X %X%c", cur_ulevel,
				newest_ulevel, updatable);
	}

	if (!skip_status) {
		get_status(dev, buf, percent, path_status);
		len += sprintf(body + len, "%s", buf);
	}
	sprintf(body + len, "\n");

	return body;
}

/**
 * print_device -
 * @dev:		ipr dev struct
 * @body:		message buffer
 * @option:		
 * @type:		bitfield - type if information to print
 *
 * Returns:
 *   char * buffer
 **/
/*
 * Print the given device to a buffer. The type parameter is defined as
 * a bitfield which has the following behavior:
 *
 * 0: print sd, resource address (sis32)/resource path (sis64), device VPD
 * 1: print sd, pci/scsi location, device description, print % complete
 * 2: print sg, resource address (sis32)/resource path (sis64), device VPD
 * 3: print sd, pci/scsi location, device description, indent array members, print % complete
 * 4: print sg, pci/scsi location, device description, print % complete
 * 5: print sd, hardware location, status
 * 6: print sd, the first resource adress(sis32)/resource path(sis64), dev VPD
 * 7: print sd, the second resource adress(sis32)/resource path(sis64), dev VPD
 * 8: print sg, pci/host/resource path, serial number, status
 * 9: print sg, physical location, production id, status
 * 10: print sg, pci/host/resource vendor, product id, ucode version, available ucode
 * 11: print sd, resource address (sis32)/resource path (sis64), vendor, product id, ucode version, available ucode
 */
char *print_device(struct ipr_dev *dev, char *body, char *option, int type)
{
	int sd, sg, vpd, percent, indent, res_path, hw_loc, conc_main, ioa_flag, serial_num;
	int ucode = 0;
	int skip_status = 0;
	int skip_vset = 0;

	sd = sg = vpd = percent = indent = res_path = hw_loc = conc_main = ioa_flag = serial_num = 0;

	if ((type & 0x1000) == 0x1000) ioa_flag = 1;

	type &= 0x0fff;

	if (type == 2 || type == 4)
		sg = 1;
	else
		sd = 1;

	if (type == 0 || type == 2 || type == 6 || type == 7) {
		vpd = 1;
		res_path = 1;
	} else
		percent = 1;

	if (type == 3)
		indent = 1;

	if (type == 5) {
		hw_loc = 1;
		conc_main = 1;
		sg = 1;
	}

	if (type == 6 || type == 7) {
		conc_main = 1;
		sg = 1;
	}
	if (type == 8) {
		sg = 1;
		if (dev->ioa->sis64)
			res_path = 1;
		else
			res_path = 0;
		serial_num = 1;
	}
	if (type == 9) {
		sg = 1;
		hw_loc = 1;
		if (dev->ioa->sis64)
			res_path = 1;
		else
			res_path = 0;
		vpd = 1;
	}
	if (type == 10) {
		ucode = 1;
		sd = 1;
		sg = 1;
		vpd = 1;
		skip_status = 1;
		skip_vset = 1;
	}
	if (type == 11) {
		ucode = 1;
		sg = 1;
		vpd = 1;
		skip_status = 1;
		skip_vset = 1;
		res_path = 1;
		percent =1;
	}

	return __print_device(dev, body, option, sd, sg, vpd, percent,
			      indent, res_path, type&1, 0, hw_loc,
			      conc_main, ioa_flag, serial_num, ucode,
			      skip_status, skip_vset);
}

/**
 * usage - display usage information
 *
 * Returns:
 *   nothing
 **/
static void usage()
{
	printf(_("Usage: iprconfig [options]\n"));
	printf(_("  Options: -e name    Default editor for viewing error logs\n"));
	printf(_("           -k dir     Kernel messages root directory\n"));
	printf(_("           -c command Command line configuration\n"));
	printf(_("                      See man page for complete details\n"));
	printf(_("           --version  Print iprconfig version\n"));
	printf(_("           --debug    Enable additional error logging\n"));
	printf(_("           --force    Disable safety checks. See man page for details.\n"));
	printf(_("  Use quotes around parameters with spaces\n"));
}

/**
 * find_and_add_dev -
 * @name:		device name
 *
 * Returns:
 *   ipr_dev struct if success / NULL on failure
 **/
static struct ipr_dev *find_and_add_dev(char *name)
{
	struct ipr_dev *dev = find_dev(name);

	if (!dev) {
		syslog(LOG_ERR, _("Invalid device specified: %s\n"), name);
		return NULL;
	}

	ipr_add_sysfs_dev(dev, &head_sdev, &tail_sdev);
	return dev;
}

/**
 * raid_create_add_dev -
 * @name:		device name
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int raid_create_add_dev(char *name, const char *raid_level, int skip_format)
{
	struct ipr_dev *dev = find_and_add_dev(name);

	if (!dev)
		return -EINVAL;

	if (ipr_is_gscsi(dev)) {
		if (is_af_blocked(dev, 0))
			return -EINVAL;
		if (!is_format_allowed(dev))
			return -EIO;
	} else if (ipr_is_af_dasd_device(dev)) {
		/* We always skip formatting for SIS32 if we are building
		 * a RAID0 */
		if (!dev->ioa->sis64 && strcmp (raid_level, "0") == 0)
			skip_format = 1;
	}

	if (!skip_format) {
		/* Format device */
		if (dev->ioa->support_4k && dev->block_dev_class & IPR_BLK_DEV_CLASS_4K)
			add_format_device(dev, IPR_AF_4K_BLOCK_SIZE);
		else
			add_format_device(dev, dev->ioa->af_block_size);
		dev_init_tail->do_init = 1;
	}

	return 0;
}

/**
 * raid_create_check_num_devs -
 * @ioa:		ipr_array_cap_entry struct
 * @num_devs:		number of devices
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int raid_create_check_num_devs(struct ipr_array_cap_entry *cap,
				      int num_devs, int err_type)
{
	if (num_devs < cap->min_num_array_devices ||
	    num_devs > cap->max_num_array_devices ||
	    (num_devs % cap->min_mult_array_devices) != 0) {
		syslog(LOG_ERR, _("Invalid number of devices (%d) selected for "
				  "RAID %s array.\n"
				  "Must select a minimum of %d devices, a "
				  "maximum of %d devices, "
				  "and must be a multiple of %d devices.\n"),
		       num_devs, cap->prot_level_str, cap->min_num_array_devices,
		       cap->max_num_array_devices, cap->min_mult_array_devices);
		if (err_type == 0)
			return -EINVAL;
		else if (num_devs > cap->min_num_array_devices)
			return RC_77_Too_Many_Disks;
		else if (num_devs < cap->min_num_array_devices)
			return RC_78_Too_Few_Disks;
		else if ((num_devs % cap->min_num_array_devices) != 0)
			return RC_25_Devices_Multiple;
	}
	return 0;
}

/**
 * curses_init - initialize curses
 *
 * Returns:
 *   nothing
 **/
static void curses_init()
{
	/* makes program compatible with all terminals -
	 originally didn't display text correctly when user was running xterm */
	setenv("TERM", "vt100", 1);
	setlocale(LC_ALL, "");
	initscr();
}

/**
 * format_devices -
 * @args:		argument vector
 * @num_args:		number of arguments
 * @blksz:		block size
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int format_devices(char **args, int num_args, int fmt_flag)
{
	int i, rc, blksz;
	struct ipr_dev *dev;

	for (i = 0; i < num_args; i++) {
		dev = find_dev(args[i]);
		if (!dev) {
			fprintf(stderr, "Invalid device: %s\n", args[i]);
			continue;
		}
		if (!dev->ioa->support_4k && dev->block_dev_class & IPR_BLK_DEV_CLASS_4K) {
			fprintf(stderr, "Invalid device specified: %s. 4K disks not supported on this adapter", args[i]);
			continue;
		}

		if (fmt_flag == IPR_FMT_JBOD ) {
			if (dev->ioa->support_4k && dev->block_dev_class & IPR_BLK_DEV_CLASS_4K)
				blksz = IPR_JBOD_4K_BLOCK_SIZE;
			else
				blksz = IPR_JBOD_BLOCK_SIZE;
		} else if (fmt_flag == IPR_FMT_RECOVERY)
			blksz = 0;

		if (!ipr_is_af_dasd_device(dev) && !ipr_is_gscsi(dev))
			continue;

		if (ipr_is_af_dasd_device(dev)) {
			if (ipr_is_array_member(dev) && !dev->dev_rcd->no_cfgte_vol)
				continue;
			if (!is_format_allowed(dev))
				continue;
		} else {
			if (!is_format_allowed(dev))
				continue;
		}

		add_format_device(dev, blksz);
		dev_init_tail->do_init = 1;
	}

	rc = send_dev_inits(NULL);
	free_devs_to_init();
	return IPR_XLATE_DEV_FMT_RC(rc);
}

/**
 * recovery_format -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int recovery_format(char **args, int num_args)
{
	return format_devices(args, num_args, IPR_FMT_RECOVERY);
}

/**
 * format_for_jbod -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int format_for_jbod(char **args, int num_args)
{
	return format_devices(args, num_args, IPR_FMT_JBOD);
}

/**
 * format_for_raid -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int format_for_raid(char **args, int num_args)
{
	int i, rc, blk_size;
	struct ipr_dev *dev;

	for (i = 0; i < num_args; i++) {
		dev = find_and_add_dev(args[i]);
		if (!dev || !can_format_for_raid(dev)) {
			fprintf(stderr, "Invalid device: %s\n", args[i]);
			return -EINVAL;
		}

		if (!dev->ioa->support_4k && dev->block_dev_class & IPR_BLK_DEV_CLASS_4K) {
			fprintf(stderr, "Invalid device specified: %s. 4K disks not supported on this adapter\n", args[i]);
			return -EINVAL;
		}

		if (dev->ioa->support_4k && dev->block_dev_class & IPR_BLK_DEV_CLASS_4K)
			blk_size = IPR_AF_4K_BLOCK_SIZE;
		else
			blk_size = dev->ioa->af_block_size;

		add_format_device(dev, blk_size);
		dev_init_tail->do_init = 1;
	}

	rc = send_dev_inits(NULL);
	free_devs_to_init();
	return IPR_XLATE_DEV_FMT_RC(rc);
}

/**
 * raid_create 
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int raid_create(char **args, int num_args)
{
	int i, num_devs = 0, ssd_num_devs = 0, rc, prot_level;
	int non_4k_count = 0, is_4k_count = 0;
	int next_raid_level, next_stripe_size, next_qdepth, next_label;
	char *raid_level = IPR_DEFAULT_RAID_LVL;
	char label[8];
	int stripe_size, qdepth, zeroed_devs, skip_format, max_qdepth;
	int next_vcache, vcache = -1;
	struct ipr_dev *dev;
	struct sysfs_dev *sdev;
	struct ipr_ioa *ioa = NULL;
	struct ipr_array_cap_entry *cap;
	u8 array_id;

	label[0] = '\0';
	next_raid_level = 0;
	next_stripe_size = 0;
	next_qdepth = 0;
	next_label = 0;
	next_vcache = 0;
	stripe_size = 0;
	qdepth = 0;
	zeroed_devs = 0;
	skip_format = 0;

	for (i = 0; i < num_args; i++) {
		if (strcmp(args[i], "-z") == 0)
			zeroed_devs = 1;
		else if (strcmp(args[i], "-r") == 0)
			next_raid_level = 1;
		else if (strcmp(args[i], "-s") == 0)
			next_stripe_size = 1;
		else if (strcmp(args[i], "-q") == 0)
			next_qdepth = 1;
		else if (strcmp(args[i], "-l") == 0)
			next_label = 1;
		else if (strcmp(args[i], "-c") == 0)
			 next_vcache = 1;
		else if (strcmp(args[i], "--skip-format") == 0)
			skip_format = 1;
		else if (next_raid_level) {
			next_raid_level = 0;
			raid_level = args[i];
		} else if (next_stripe_size) {
			next_stripe_size = 0;
			stripe_size = strtoul(args[i], NULL, 10);
		} else if (next_qdepth) {
			next_qdepth = 0;
			qdepth = strtoul(args[i], NULL, 10);
		} else if (next_vcache) {
			next_vcache = 0;
			if (strcmp(args[i], "writethrough") == 0)
			    vcache = IPR_DEV_CACHE_WRITE_THROUGH;
			else if (strcmp(args[i], "writeback") == 0)
				vcache = IPR_DEV_CACHE_WRITE_BACK;
			else {
				syslog(LOG_ERR,
				       _("Invalid value passed to -c."));
				return -EINVAL;
			}
		} else if (next_label) {
			next_label = 0;
			if (strlen(args[i]) > (sizeof(label) - 1)) {
				syslog(LOG_ERR, _("RAID array label too long. Maximum length is %d characters\n"),
				       (int)(sizeof(label) - 1));
				return -EINVAL;
			}

			strcpy(label, args[i]);
		} else if ((dev = find_dev(args[i]))) {
			if (dev->block_dev_class & IPR_SSD)
				ssd_num_devs++;
			num_devs++;
			if (zeroed_devs)
				ipr_add_zeroed_dev(dev);
			if (raid_create_add_dev(args[i], raid_level, skip_format))
				return -EIO;
		}
	}

	for (sdev = head_sdev; sdev; sdev = sdev->next) {
		dev = ipr_sysfs_dev_to_dev(sdev);
		if (!dev) {
			syslog(LOG_ERR, _("Cannot find device\n"));
			return -EINVAL;
		}

		if (!ioa)
			ioa = dev->ioa;
		else if (ioa != dev->ioa) {
			syslog(LOG_ERR, _("All devices must be attached to the same adapter.\n"));
			return -EINVAL;
		}

		if (dev->block_dev_class & IPR_BLK_DEV_CLASS_4K)
			is_4k_count++;
		else
			non_4k_count++;
	}

	if (is_4k_count > 0 && non_4k_count > 0) {
		syslog(LOG_ERR, _("4K disks and 5XX disks can not be mixed in an array.\n"));
		return -EINVAL;
	}

	if (!ioa) {
		syslog(LOG_ERR, _("No valid devices specified.\n"));
		return -EINVAL;
	}

	cap = get_cap_entry(ioa->supported_arrays, raid_level);

	if (!cap) {
		syslog(LOG_ERR, _("Invalid RAID level for selected adapter. %s\n"), raid_level);
		return -EINVAL;
	}

	prot_level = cap->prot_level >> 4;
	if (dev->block_dev_class == IPR_HDD
	    && (prot_level == 0x5 || prot_level == 0x6)
	    && !ioa->has_cache && !ipr_force) {
		fprintf(stderr,"\
Warning: Performance may be suboptimal for this RAID level without an \
IOA write cache.  Use --force to force creation.\n");
		return -EINVAL;
	}

	if (stripe_size && ((cap->supported_stripe_sizes & stripe_size) != stripe_size)) {
		syslog(LOG_ERR, _("Unsupported stripe size for selected adapter. %d\n"),
		       stripe_size);
		return -EINVAL;
	}

	if ((rc = raid_create_check_num_devs(cap, num_devs, 0)))
		return rc;

	if (!stripe_size)
		stripe_size = ntohs(cap->recommended_stripe_size);

	max_qdepth = ipr_max_queue_depth(dev->ioa, num_devs, ssd_num_devs);
	if (qdepth > max_qdepth)
		qdepth = max_qdepth;
	if (!qdepth) {
		if (ioa->sis64)
			qdepth = num_devs * 16;
		else
			qdepth = num_devs * 4;
	}

	if (vcache == IPR_DEV_CACHE_WRITE_BACK && !ioa->vset_write_cache)
		syslog(LOG_ERR,
		       _("Cannot set requested caching mode, using default.\n"));
	else if(ioa->vset_write_cache && vcache < 0) {
		/* For performance reasons Writeback is the default
		  Vset Cache policy. Users can override this setting
		  by passing option '-c writethrough' to raid create */
		vcache = IPR_DEV_CACHE_WRITE_BACK;
	}

	if (dev_init_head) {
		rc = send_dev_inits(NULL);
		free_devs_to_init();
		if (IPR_XLATE_DEV_FMT_RC(rc))
			return rc;
	}

	check_current_config(false);

	for (sdev = head_sdev; sdev; sdev = sdev->next) {
		dev = ipr_sysfs_dev_to_dev(sdev);
		if (!dev) {
			syslog(LOG_ERR, _("Cannot find device: %lx\n"),
			       sdev->device_id);
			return -EIO;
		}

		if (!ipr_is_af_dasd_device(dev)) {
			scsi_err(dev, "Invalid device type\n");
			return -EIO;
		}

		dev->dev_rcd->issue_cmd = 1;
	}

	if (!ioa->start_array_qac_entry) {
		scsi_err(dev, "Invalid device type\n");
		return -EIO;
	}

	if (ioa->sis64) {
		array_id = ioa->start_array_qac_entry->type3.array_id;
		if (strlen(label))
			strcpy((char *)ioa->start_array_qac_entry->type3.desc, label);
	} else
		array_id = ioa->start_array_qac_entry->type2.array_id;

	add_raid_cmd_tail(ioa, &ioa->ioa, array_id);
	raid_cmd_tail->qdepth = qdepth;
	raid_cmd_tail->do_cmd = 1;
	raid_cmd_tail->vset_cache = vcache;

	rc = ipr_start_array_protection(ioa, stripe_size, cap->prot_level);

	if (rc)
		return rc;

	rc = raid_start_complete();
	format_done = 1;
	if (rc == RC_18_Array_Created)
		return 0;
	else
		return -EIO;
}

/**
 * hot_spare_delete -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int hot_spare_delete(char **args, int num_args)
{
	struct ipr_dev *dev;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	dev->dev_rcd->issue_cmd = 1;
	return ipr_remove_hot_spare(dev->ioa);
}

/**
 * hot_spare_create -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int hot_spare_create(char **args, int num_args)
{
	struct ipr_dev *dev;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	dev->dev_rcd->issue_cmd = 1;
	return ipr_add_hot_spare(dev->ioa);
}

/**
 * raid_include_cmd -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int raid_include_cmd(char **args, int num_args)
{
	int i, rc, zeroed = 0;
	struct ipr_dev *dev, *vset = NULL;
	struct ipr_ioa *ioa;
	struct devs_to_init_t *dev_init;

	for (i = 0; i < num_args; i++) {
		if (!strcmp(args[i], "-z"))
			zeroed = 1;
	}

	for (i = 0; i < num_args; i++) {
		if (!strcmp(args[i], "-z"))
			continue;

		dev = find_and_add_dev(args[i]);
		if (!dev)
			return -EINVAL;

		if (ipr_is_volume_set(dev)) {
			if (vset) {
				fprintf(stderr, "Invalid parameters. Only one "
					"disk array can be specified at a time.\n");
				return -EINVAL;
			}

			vset = dev;
			ioa = vset->ioa;
			continue;
		}

		if (!ipr_is_af_dasd_device(dev)) {
			fprintf(stderr, "Invalid device specified. Device must "
				"be formatted to Advanced Function Format.\n");
			return -EINVAL;
		}

		dev->dev_rcd->issue_cmd = 1;
		if (!zeroed) {
			if (!is_format_allowed(dev)) {
				fprintf(stderr, "Format not allowed to %s\n", args[i]);
				return -EIO;
			}

			add_format_device(dev, 0);
			dev_init_tail->do_init = 1;
		}
	}

	if (!vset) {
		fprintf(stderr, "Invalid parameters. A disk array must be specified.\n");
		return -EINVAL;
	}

	for_each_dev_to_init(dev_init)
		if (vset->block_dev_class != dev_init->dev->block_dev_class) {
			fprintf(stderr, "Invalid parameter. Can not mix SSDs and HDDs.\n");
			return -EINVAL;
		}

	if (!zeroed) {
		rc = send_dev_inits(NULL);
		free_devs_to_init();
		if (IPR_XLATE_DEV_FMT_RC(rc))
			return rc;
	}

	return do_array_include(vset->ioa, vset->array_id,
				vset->ioa->qac_data);
}

/**
 * raid_delete -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
static int raid_delete(char **args, int num_args)
{
	int rc;
	struct ipr_dev *dev;
	struct ipr_res_addr *ra;

	dev = find_and_add_dev(args[0]);

	if (!dev || !ipr_is_volume_set(dev) || dev->ioa->is_secondary) {
		syslog(LOG_ERR, _("Invalid device specified: %s\n"), args[0]);
		return -EINVAL;
	}

	dev->array_rcd->issue_cmd = 1;
	if (dev->scsi_dev_data) {
		ipr_allow_restart(dev, 0);
		ipr_set_manage_start_stop(dev);
		ipr_start_stop_stop(dev);
		ipr_write_dev_attr(dev, "delete", "1");
	}
	if (dev->alt_path && dev->alt_path->scsi_dev_data) {
		ipr_allow_restart(dev->alt_path, 0);
		ipr_set_manage_start_stop(dev->alt_path);
		ipr_start_stop_stop(dev->alt_path);
		ipr_write_dev_attr(dev->alt_path, "delete", "1");
	}

	sleep(2);

	rc = ipr_stop_array_protection(dev->ioa);

	if (rc != 0) {
		if (dev->scsi_dev_data) {
			ra = &dev->res_addr[0];
			ipr_scan(dev->ioa, ra->bus, ra->target, ra->lun);
			ipr_allow_restart(dev, 1);
		}
		if (dev->alt_path && dev->alt_path->scsi_dev_data) {
			ra = &dev->alt_path->res_addr[0];
			ipr_scan(dev->alt_path->ioa, ra->bus, ra->target, ra->lun);
			ipr_allow_restart(dev->alt_path, 1);
		}
	}

	return rc;
}

/**
 * printf_device - Display the given device 
 * @dev:		ipr dev struct
 * @type:		type of information to print
 *
 * Returns:
 *   nothing
 **/
static void printf_device(struct ipr_dev *dev, int type)
{
	char *buf = print_device(dev, NULL, NULL, type);
	if (buf) {
		printf("%s", buf);
		free(buf);
	}
}

/**
 * __printf_device -
 * @dev:		ipr dev struct
 * @sd:			
 * @sg:			
 * @vpd:		
 * @percent:		
 * @indent:		
 * @res_path:		
 *
 * Returns:
 *   nothing
 **/
static void __printf_device(struct ipr_dev *dev, int sd, int sg, int vpd,
			    int percent, int indent, int res_path)
{
	char *buf = __print_device(dev, NULL, NULL, sd, sg, vpd, percent, indent, res_path, 0, 0, 0, 0, 0, 0, 0, 0 ,0);
	if (buf) {
		printf("%s", buf);
		free(buf);
	}
}

/**
 * printf_vsets -
 * @ioa:		ipr ioa struct
 * @type:		type of information to print
 *
 * Returns:
 *   nothing
 **/
static void printf_vsets(struct ipr_ioa *ioa, int type)
{
	struct ipr_dev *vset, *dev;

	for_each_vset(ioa, vset) {
		printf_device(vset, type);
		for_each_dev_in_vset(vset, dev)
			printf_device(dev, type);
	}
}

/**
 * printf_hot_spare_disks -
 * @ioa:		ipr ioa struct
 * @type:		type of information to print
 *
 * Returns:
 *   nothing
 **/
static void printf_hot_spare_disks(struct ipr_ioa *ioa, int type)
{
	struct ipr_dev *dev;

	for_each_hot_spare(ioa, dev)
		printf_device(dev, type);
}

/**
 * printf_standlone_disks -
 * @ioa:		ipr ioa struct
 * @type:		type of information to print
 *
 * Returns:
 *   nothing
 **/
static void printf_standlone_disks(struct ipr_ioa *ioa, int type)
{
	struct ipr_dev *dev;

	for_each_standalone_disk(ioa, dev)
		printf_device(dev, type);
}

/**
 * printf_ses_devices -
 * @ioa:		ipr ioa struct
 * @type:		type of information to print
 *
 * Returns:
 *   nothing
 **/
static void printf_ses_devices(struct ipr_ioa *ioa, int type)
{
	struct ipr_dev *dev;

	for_each_ses(ioa, dev)
		printf_device(dev, type);
}

/**
 * printf_ioa -
 * @ioa:		ipr ioa struct
 * @type:		type of information to print
 *
 * Returns:
 *   nothing
 **/
static void printf_ioa(struct ipr_ioa *ioa, int type)
{
	if (!ioa->ioa.scsi_dev_data)
		return;

	printf_device(&ioa->ioa, type);
	printf_standlone_disks(ioa, type);
	printf_hot_spare_disks(ioa, type);
	printf_vsets(ioa, type);
	printf_ses_devices(ioa, type);
}

/**
 * query_raid_create -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_raid_create(char **args, int num_args)
{
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;
	int num = 0;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "Device is not an IOA\n");
		return -EINVAL;
	}

	ioa = dev->ioa;

	if (!ioa->qac_data->num_records)
		return 0;

	for_each_disk(ioa, dev) {
		if (!device_supported(dev))
			continue;
		if (ipr_is_af_dasd_device(dev) && !dev->dev_rcd->start_cand)
			continue;

		if (!num)
			printf("%s\n%s\n", status_hdr[2], status_sep[2]);
		__printf_device(dev, 1, 1, 1, 0, 0, 0);
		num++;
	}

	if (num)
		printf("\b\n");

	return 0;
}

/**
 * query_raid_delete -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_raid_delete(char **args, int num_args)
{
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;
	int num = 0;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "Device is not an IOA\n");
		return -EINVAL;
	}

	ioa = dev->ioa;

	for_each_vset(ioa, dev) {
		if (!dev->array_rcd->stop_cand)
			continue;
		if (is_array_in_use(ioa, dev->array_id))
			continue;
		if (!num)
			printf("%s\n%s\n", status_hdr[3], status_sep[3]);
		printf_device(dev, 3);
		num++;
	}

	return 0;
}

/**
 * query_hot_spare_create -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_hot_spare_create(char **args, int num_args)
{
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;
	int hdr = 0;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	ioa = dev->ioa;

	for_each_af_dasd(ioa, dev) {
		if (!dev->dev_rcd->add_hot_spare_cand)
			continue;

		if (!hdr) {
			hdr = 1;
			printf("%s\n%s\n", status_hdr[2], status_sep[2]);
		}

		printf_device(dev, 2);
	}

	return 0;
}

/**
 * query_hot_spare_delete -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_hot_spare_delete(char **args, int num_args)
{
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;
	int hdr = 0;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	ioa = dev->ioa;

	for_each_hot_spare(ioa, dev) {
		if (!dev->dev_rcd->rmv_hot_spare_cand)
			continue;

		if (!hdr) {
			hdr = 1;
			printf("%s\n%s\n", status_hdr[2], status_sep[2]);
		}

		printf_device(dev, 2);
	}

	return 0;
}

/**
 * query_raid_consistency_check -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_raid_consistency_check(char **args, int num_args)
{
	struct ipr_dev *dev, *array;
	struct ipr_ioa *ioa;
	int hdr = 0;

	for_each_ioa(ioa) {
		for_each_array(ioa, array) {
			if (!array->array_rcd->resync_cand)
				continue;

			dev = array;
			if (ioa->sis64)
				dev = get_vset_from_array(ioa, dev);

			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[3], status_sep[3]);
			}

			printf_device(dev, 1);
		}
	}

	return 0;
}

/**
 * raid_consistency_check -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int raid_consistency_check(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->sis64)
		dev = get_array_from_vset(dev->ioa, dev);

	if (!dev->array_rcd->resync_cand) {
		fprintf(stderr, "%s is not a candidate for checking.\n", args[0]);
		return -EINVAL;
	}

	dev->array_rcd->issue_cmd = 1;
	return ipr_resync_array(dev->ioa);
}

/**
 * start_ioa_cache -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int start_ioa_cache(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_reclaim_query_data reclaim_buffer;
	int rc;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	rc =  ipr_reclaim_cache_store(dev->ioa,
				      IPR_RECLAIM_RESET_BATTERY_ERROR | IPR_RECLAIM_EXTENDED_INFO,
				      &reclaim_buffer);

	if (rc)
		return rc;

	if (reclaim_buffer.action_status != IPR_ACTION_SUCCESSFUL)
		rc = -EIO;

	return rc;
}

/**
 * force_cache_battery_error -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int force_cache_battery_error(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_reclaim_query_data reclaim_buffer;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	return ipr_reclaim_cache_store(dev->ioa, IPR_RECLAIM_FORCE_BATTERY_ERROR,
				       &reclaim_buffer);
}

/**
 * set_bus_width -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_bus_width(char **args, int num_args)
{
	int rc;
	struct ipr_scsi_buses page_28_cur;
	struct ipr_scsi_buses page_28_chg;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev = find_dev(args[0]);
	int bus = strtoul(args[1], NULL, 10);
	int width = strtoul(args[2], NULL, 10);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (!ioa_is_spi(dev->ioa) || dev->ioa->is_aux_cache)
		return -EINVAL;

	ioa = dev->ioa;
	memset(&page_28_cur, 0, sizeof(page_28_cur));
	memset(&page_28_chg, 0, sizeof(page_28_chg));

	rc = ipr_get_bus_attr(ioa, &page_28_cur);

	if (rc)
		return rc;

	if (bus >= page_28_cur.num_buses) {
		fprintf(stderr, "Invalid bus specified: %d\n", bus);
		return -EINVAL;
	}

	get_changeable_bus_attr(ioa, &page_28_chg, page_28_cur.num_buses);

	if (!page_28_chg.bus[bus].bus_width) {
		fprintf(stderr, "Bus width not changeable for this device\n");
		return -EINVAL;
	}

	page_28_cur.bus[bus].bus_width = width;
	return ipr_set_bus_attr(ioa, &page_28_cur, 1);
}

/**
 * set_bus_speed -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_bus_speed(char **args, int num_args)
{
	int rc, max_speed, new_xfer_rate;
	struct ipr_scsi_buses page_28_cur;
	struct ipr_scsi_buses page_28_chg;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev = find_dev(args[0]);
	int bus = strtoul(args[1], NULL, 10);
	int speed = strtoul(args[2], NULL, 10);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (!ioa_is_spi(dev->ioa) || dev->ioa->is_aux_cache)
		return -EINVAL;

	ioa = dev->ioa;
	memset(&page_28_cur, 0, sizeof(page_28_cur));
	memset(&page_28_chg, 0, sizeof(page_28_chg));

	rc = ipr_get_bus_attr(ioa, &page_28_cur);

	if (rc)
		return rc;

	if (bus >= page_28_cur.num_buses) {
		fprintf(stderr, "Invalid bus specified: %d\n", bus);
		return -EINVAL;
	}

	get_changeable_bus_attr(ioa, &page_28_chg, page_28_cur.num_buses);

	max_speed = get_max_bus_speed(ioa, bus);
	new_xfer_rate = IPR_BUS_THRUPUT_TO_XFER_RATE(speed,
						     page_28_cur.bus[bus].bus_width);

	if (speed > max_speed) {
		fprintf(stderr, "Max speed allowed: %d MB/sec\n", max_speed);
		return -EINVAL;
	}

	page_28_cur.bus[bus].max_xfer_rate = new_xfer_rate;
	return ipr_set_bus_attr(ioa, &page_28_cur, 1);
}

/**
 * set_initiator_id -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_initiator_id(char **args, int num_args)
{
	int rc;
	struct ipr_scsi_buses page_28_cur;
	struct ipr_scsi_buses page_28_chg;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev = find_dev(args[0]);
	int bus = strtoul(args[1], NULL, 10);
	int scsi_id = strtoul(args[2], NULL, 10);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (!ioa_is_spi(dev->ioa) || dev->ioa->is_aux_cache)
		return -EINVAL;

	if (scsi_id > 7) {
		fprintf(stderr, "Host scsi id must be < 7\n");
		return -EINVAL;
	}

	ioa = dev->ioa;
	memset(&page_28_cur, 0, sizeof(page_28_cur));
	memset(&page_28_chg, 0, sizeof(page_28_chg));

	rc = ipr_get_bus_attr(ioa, &page_28_cur);

	if (rc)
		return rc;

	if (bus >= page_28_cur.num_buses) {
		fprintf(stderr, "Invalid bus specified: %d\n", bus);
		return -EINVAL;
	}

	get_changeable_bus_attr(ioa, &page_28_chg, page_28_cur.num_buses);

	if (!page_28_chg.bus[bus].scsi_id) {
		fprintf(stderr, "SCSI ID is not changeable for this adapter.\n");
		return -EINVAL;
	}

	for_each_dev(ioa, dev) {
		if (!dev->scsi_dev_data)
			continue;

		if (scsi_id == dev->scsi_dev_data->id &&
		    bus == dev->scsi_dev_data->channel) {
			fprintf(stderr, "SCSI ID %d conflicts with a device\n", scsi_id);
			return -EINVAL;
		}
	}

	page_28_cur.bus[bus].scsi_id = scsi_id;
	return ipr_set_bus_attr(ioa, &page_28_cur, 1);
}

/**
 * find_slot -
 * @devs:		ipr_dev struct
 * @num_devs:		number of devices
 * @slot:		slot description
 *
 * Returns:
 *   ipr_dev struct on success / NULL otherwise
 **/
static struct ipr_dev *find_slot(struct ipr_dev **devs, int num_devs, char *slot)
{
	int i;

	for (i = 0; i < num_devs; i++) {
		syslog_dbg("Looking for slot %s at pos %d\n", slot, i);
		if (!strncmp(devs[i]->physical_location, slot, strlen(slot)))
			return devs[i];
	}

	return NULL;
}

/**
 * __add_device -
 * @dev:		ipr dev struct
 * @on:			
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __add_device(struct ipr_dev *dev, int on)
{
	struct ipr_dev *ses = dev->ses[0];
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status, *overall;
	struct ipr_ses_config_pg ses_cfg;
	int rc;

	if (!ses) {
		fprintf(stderr, "Invalid device\n");
		return -EINVAL;
	}

	if (dev->ioa->sis64)
		elem_status = get_elem_status_64bit(dev, ses, &ses_data, &ses_cfg);
	else
		elem_status = get_elem_status(dev, ses, &ses_data, &ses_cfg);
	if (!elem_status) {
		scsi_err(dev, "Cannot find SES device for specified device\n");
		return -EINVAL;
	}

	elem_status->select = 1;
	elem_status->remove = 0;
	elem_status->insert = on;
	elem_status->identify = on;
	elem_status->enable_byp = 0;

	overall = ipr_get_overall_elem(&ses_data, &ses_cfg); 
	overall->select = 1;
	overall->disable_resets = on;

	rc = ipr_send_diagnostics(ses, &ses_data, ntohs(ses_data.byte_count) + 4);

	if (!on) {
		if (dev->ioa->sis64)
			wait_for_new_dev_64bit(dev->ioa, &(dev->res_path[0]));
		else
			wait_for_new_dev(dev->ioa, &(dev->res_addr[0]));
	}
	return rc;
}

/**
 * __remove_device -
 * @dev:		ipr dev struct
 * @on:			
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __remove_device(struct ipr_dev *dev, int on)
{
	struct ipr_dev *ses = dev->ses[0];
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status, *overall;
	struct ipr_ses_config_pg ses_cfg;
	int rc;

	if (!dev->ses) {
		fprintf(stderr, "Invalid device specified\n");
		return -EINVAL;
	}

	if (!on && ipr_is_af_dasd_device(dev)) {
		if (!ipr_can_remove_device(dev)) {
			scsi_err(dev, "Cannot remove device\n");
			return -EINVAL;
		}
	}

	if (dev->ioa->sis64)
		elem_status = get_elem_status_64bit(dev, ses, &ses_data, &ses_cfg);
	else
		elem_status = get_elem_status(dev, ses, &ses_data, &ses_cfg);
	if (!elem_status) {
		scsi_err(dev, "Invalid device specified\n");
		return -EINVAL;
	}

	elem_status->select = 1;
	elem_status->remove = on;
	elem_status->insert = 0;
	elem_status->identify = on;

	overall = ipr_get_overall_elem(&ses_data, &ses_cfg);
	overall->select = 1;
	overall->disable_resets = on;

	rc = ipr_send_diagnostics(ses, &ses_data, ntohs(ses_data.byte_count) + 4);

	if (!on) {
		if (dev->ioa->sis64) {
			if (!remove_or_add_back_device_64bit(dev)) {
				printf(_("Disk is not removed physically, Add it back automically\n"));
				sleep(5);
			}
		}
		else {
			ipr_write_dev_attr(dev, "delete", "1");
			evaluate_device(dev, dev->ioa, 0);
			ipr_del_zeroed_dev(dev);
		}
	}

	return rc;
}

/**
 * add_slot -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int add_slot(char **args, int num_args)
{
	int num_devs, rc;
	struct ipr_dev *dev, **devs;
	int on = strtoul(args[1], NULL, 10);

	num_devs = get_conc_devs(&devs, IPR_CONC_ADD);
	dev = find_slot(devs, num_devs, args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid location %s\n", args[0]);
		return -EINVAL;
	}

	rc = __add_device(dev, on);
	free_empty_slots(devs, num_devs);
	free(devs);
	return rc;
}

/**
 * remove_slot -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int remove_slot(char **args, int num_args)
{
	int num_devs, rc;
	struct ipr_dev *dev, **devs;
	int on = strtoul(args[1], NULL, 10);

	num_devs = get_conc_devs(&devs, IPR_CONC_REMOVE);
	dev = find_slot(devs, num_devs, args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid location %s\n", args[0]);
		return -EINVAL;
	}

	rc = __remove_device(dev, on);
	free_empty_slots(devs, num_devs);
	free(devs);
	return rc;
}

/**
 * remove_disk -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int remove_disk(char **args, int num_args)
{
	int num_devs, i;
	struct ipr_dev **devs;
	struct ipr_dev *dev = find_dev(args[0]);
	int on = strtoul(args[1], NULL, 10);
	int rc = -EINVAL;

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	num_devs = get_conc_devs(&devs, IPR_CONC_REMOVE);
	for (i = 0; i < num_devs; i++) {
		if (devs[i] == dev) {
			rc = __remove_device(dev, on);
			break;
		}
	}

	free_empty_slots(devs, num_devs);
	free(devs);
	return rc;
}

/**
 * __identify_device -
 * @dev:		ipr dev struct
 * @on:			
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __identify_device(struct ipr_dev *dev, int on)
{
	struct ipr_dev *ses = dev->ses[0];
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status;
	struct ipr_ses_config_pg ses_cfg;

	if (!dev->ses) {
		fprintf(stderr, "Invalid device specified\n");
		return -EINVAL;
	}

	if (dev->ioa->sis64)
		elem_status = get_elem_status_64bit(dev, ses, &ses_data, &ses_cfg);
	else
		elem_status = get_elem_status(dev, ses, &ses_data, &ses_cfg);
	if (!elem_status) {
		scsi_err(dev, "Invalid device specified\n");
		return -EINVAL;
	}

	elem_status->select = 1;
	elem_status->identify = on;
	elem_status->insert = 0;
	elem_status->remove = 0;

	return ipr_send_diagnostics(ses, &ses_data, ntohs(ses_data.byte_count) + 4);
}

/**
 * identify_slot -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int identify_slot(char **args, int num_args)
{
	int num_devs, rc;
	struct ipr_dev *dev, **devs;
	int on = strtoul(args[1], NULL, 10);

	num_devs = get_conc_devs(&devs, IPR_CONC_IDENTIFY);
	dev = find_slot(devs, num_devs, args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid location %s\n", args[0]);
		return -EINVAL;
	}

	rc = __identify_device(dev, on);
	free_empty_slots(devs, num_devs);
	free(devs);
	return rc;
}

/**
 * identify_disk -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int identify_disk(char **args, int num_args)
{
	int num_devs, i;
	struct ipr_dev **devs;
	struct ipr_dev *dev = find_dev(args[0]);
	int rc = -EINVAL;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	num_devs = get_conc_devs(&devs, IPR_CONC_IDENTIFY);
	for (i = 0; i < num_devs; i++) {
		if (devs[i] == dev) {
			rc = __identify_device(dev, strtoul(args[1], NULL, 10));
			break;
		}
	}

	free_empty_slots(devs, num_devs);
	free(devs);
	return rc;
}

/**
 * set_log_level_cmd -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_log_level_cmd(char **args, int num_args)
{
	char buf[4];
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "Device is not an IOA\n");
		return -EINVAL;
	}

	snprintf(buf, sizeof(buf), "%s\n", args[1]);
	set_log_level(dev->ioa, buf);
	return 0;
}

/**
 * set_tcq_enable -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_tcq_enable(char **args, int num_args)
{
	int rc;
	struct ipr_disk_attr attr;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	rc = ipr_get_dev_attr(dev, &attr);

	if (rc)
		return rc;

	rc = ipr_modify_dev_attr(dev, &attr);

	if (rc)
		return rc;

	attr.tcq_enabled = strtoul(args[1], NULL, 10);

	if (attr.tcq_enabled != 0 && attr.tcq_enabled != 1) {
		fprintf(stderr, "Invalid parameter\n");
		return -EINVAL;
	}

	return ipr_set_dev_attr(dev, &attr, 1);
}

/**
 * set_qdepth -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_qdepth(char **args, int num_args)
{
	int rc;
	struct ipr_disk_attr attr;
	struct ipr_dev *dev = find_dev(args[0]);
	int num_devs, ssd_num_devs, max_qdepth;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	rc = ipr_get_dev_attr(dev, &attr);

	if (rc)
		return rc;

	rc = ipr_modify_dev_attr(dev, &attr);

	if (rc)
		return rc;

	ipr_count_devices_in_vset(dev, &num_devs, &ssd_num_devs);
	max_qdepth = ipr_max_queue_depth(dev->ioa, num_devs, ssd_num_devs);

	attr.queue_depth = strtoul(args[1], NULL, 10);

	if (attr.queue_depth > max_qdepth) {
		fprintf(stderr, "Invalid queue depth, max queue depth is %d\n",
			max_qdepth);
		return -EINVAL;
	}

	return ipr_set_dev_attr(dev, &attr, 1);
}

/**
 * set_format_timeout -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_format_timeout(char **args, int num_args)
{
	int rc;
	struct ipr_disk_attr attr;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	rc = ipr_get_dev_attr(dev, &attr);

	if (rc)
		return rc;

	rc = ipr_modify_dev_attr(dev, &attr);

	if (rc)
		return rc;

	attr.format_timeout = strtoul(args[1], NULL, 10) * 3600;

	return ipr_set_dev_attr(dev, &attr, 1);
}

/**
 * set_device_write_cache_policy
 *
 * Sets the write cache policy for a device.  The aruments are the
 * device (sd*, sg*) and the new policy (writeback,writethrough).
 *
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_device_write_cache_policy(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_disk_attr attr;
	char *policy = args[1];
	int ipolicy;
	int rc;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_gscsi(dev) &&
	    (!ipr_is_volume_set(dev) || !dev->ioa->vset_write_cache)) {
		scsi_err(dev, "Unable to set cache policy.\n");
		return -EINVAL;
	}

	if (strncmp(policy, "writethrough", sizeof ("writethrough") - 1) == 0)
		ipolicy = IPR_DEV_CACHE_WRITE_THROUGH;
	else if (strncmp(policy, "writeback", sizeof ("writeback") - 1) == 0)
		ipolicy = IPR_DEV_CACHE_WRITE_BACK;
	else {
		fprintf(stderr, "Invalid cache policy.\n");
		return -EINVAL;
	}

	rc = ipr_get_dev_attr(dev, &attr);
	if (rc)
		return rc;

	attr.write_cache_policy = ipolicy;
	rc = ipr_set_dev_attr(dev, &attr, 1);

	if (rc) {
		scsi_err(dev, "Unable to set cache policy.\n");
		return rc;
	}
	return 0;
}

/**
 * query_device_write_cache_policy
 *
 * Query the write cache policy for a device.  User passes the
 * device (sd*, sg*) to be queried.
 *
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_device_write_cache_policy(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_disk_attr attr;
	char *policy;
	int rc;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}
	if (!ipr_is_gscsi(dev) && !ipr_is_volume_set(dev)) {
		scsi_err(dev, "Cannot query write back policy.\n");
		return -EINVAL;
	}

	rc = ipr_get_dev_attr(dev, &attr);
	if (rc)
		return rc;

	switch (attr.write_cache_policy) {
	case IPR_DEV_CACHE_WRITE_THROUGH:
		policy = "write through";
		break;
	case IPR_DEV_CACHE_WRITE_BACK:
		policy = "write back";
		break;
	default:
		return -EINVAL;
	}

	printf("%s\n", policy);
	return 0;
}

/**
 * update_ioa_ucode -
 * @ioa:		ipr ioa struct
 * @file:		file name
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int update_ioa_ucode(struct ipr_ioa *ioa, char *file)
{
	struct ipr_fw_images image;
	int rc;

	strcpy(image.file, file);
	image.version = get_ioa_ucode_version(file);
	image.has_header = 0;

	rc = ipr_update_ioa_fw(ioa, &image, 1);

	if (image.version != get_fw_version(&ioa->ioa))
		return -EIO;
	return rc;
}

/**
 * update_dev_ucode -
 * @dev:		ipr dev struct
 * @file:		file name
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int update_dev_ucode(struct ipr_dev *dev, char *file)
{
	struct ipr_fw_images image;
	int rc;

	strcpy(image.file, file);

	if (dev->scsi_dev_data->type == TYPE_ENCLOSURE ||
	    dev->scsi_dev_data->type == TYPE_PROCESSOR) {
		image.version = get_ses_ucode_version(file);
	} else {
		image.version = get_dasd_ucode_version(file, 1);
		image.has_header = 1;
	}

	rc = ipr_update_disk_fw(dev, &image, 1);

	if (image.version != get_fw_version(dev))
		return -EIO;
	return rc;
}

/**
 * update_ucode_cmd -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int update_ucode_cmd(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (ipr_is_ioa(dev))
		return update_ioa_ucode(dev->ioa, args[1]);
	else
		return update_dev_ucode(dev, args[1]);
}

/**
 * update_all_ucodes - update-all-ucodes Command line interface.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int update_all_ucodes(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int failed_count = 0;
	struct ipr_fw_images *lfw;
	int rc;

	for_each_ioa(ioa) {
		if (!ioa->ioa.scsi_dev_data)
			continue;
		for_each_dev(ioa, dev) {
			if (ipr_is_volume_set(dev))
				continue;

			lfw = get_latest_fw_image(dev);
			if (!lfw || lfw->version <= get_fw_version(dev))
				continue;

			if (ipr_is_ioa(dev))
				rc = update_ioa_ucode(dev->ioa, lfw->file);
			else
				rc = update_dev_ucode(dev, lfw->file);

			free(lfw);

			if (rc)
				failed_count++;
		}
	}

	if (failed_count)
		fprintf(stderr, "\
Failed to update %d devices. Run dmesg for more information.\n",
			failed_count);

	return 0;
}

/**
 * show_ucode_levels - List microcode level of every device and adapter
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_ucode_levels(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	printf("%s\n%s\n", status_hdr[15], status_sep[15]);
	for_each_ioa(ioa) {
		if (ioa->is_secondary){
			printf_device(&ioa->ioa, 10);
			continue;
		}

		printf_ioa(ioa, 10);
	}
	return 0;
}

/**
 * disrupt_device_cmd -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int disrupt_device_cmd(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_af_dasd_device(dev)) {
		fprintf(stderr, "%s is not an Advanced Function disk\n", args[0]);
		return -EINVAL;
	}

	return ipr_disrupt_device(dev);
}

/**
 * raid_rebuild_cmd -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int raid_rebuild_cmd(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_af_dasd_device(dev)) {
		fprintf(stderr, "%s is not an Advanced Function disk\n", args[0]);
		return -EINVAL;
	}

	enable_af(dev);
	dev->dev_rcd->issue_cmd = 1;
	return ipr_rebuild_device_data(dev->ioa);
}

/**
 * __reclaim - Reclaim the specified IOA's write cache.
 * @args:		argument vector
 * @num_args:		number of arguments
 * @action:		
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __reclaim(char **args, int num_args, int action)
{
	struct ipr_reclaim_query_data buf;
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;
	int rc;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	ioa = dev->ioa;

	rc = ipr_reclaim_cache_store(ioa, action, &buf);

	if (rc != 0)
		return rc;

	memset(&buf, 0, sizeof(buf));
	rc = ipr_reclaim_cache_store(ioa, IPR_RECLAIM_QUERY, &buf);

	if (rc != 0)
		return rc; 

	print_reclaim_results(&buf);
	return 0;
}

/**
 * reclaim - Reclaim the specified IOA's write cache.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int reclaim(char **args, int num_args)
{
	return __reclaim(args, num_args, IPR_RECLAIM_PERFORM);
}

/**
 * reclaim_unknown - Reclaim the specified IOA's write cache and allow
 * 		     unknown data loss.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int reclaim_unknown(char **args, int num_args)
{
	return __reclaim(args, num_args,
			 IPR_RECLAIM_PERFORM | IPR_RECLAIM_UNKNOWN_PERM);
}

/**
 * query_path_details -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/

static int query_path_details(char **args, int num_args)
{
	char *buf;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	buf = print_path_details(dev, NULL);
	if (buf) {
		printf("%s\n%s\n", status_hdr[6], status_sep[6]);
		printf("%s", buf);
		free(buf);
	}

	return 0;
}

/**
 * printf_path_status -
 * @dev:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
static void printf_path_status(struct ipr_dev *dev)
{
	char *buf = __print_device(dev, NULL, NULL, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	if (buf) {
		printf("%s", buf);
		free(buf);
	}
}

/**
 * printf_ioa_path_status -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0
 **/
static int printf_ioa_path_status(struct ipr_ioa *ioa)
{
	struct ipr_dev *dev;

	if (__ioa_is_spi(ioa))
		return 0;

	for_each_disk(ioa, dev)
		printf_path_status(dev);

	return 0;
}

/**
 * query_path_status -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_path_status(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev = NULL;
	int hdr = 1;

	if (!num_args) {
		for_each_sas_ioa(ioa) {
			if (hdr) {
				printf("%s\n%s\n", status_hdr[2], status_sep[2]);
				hdr = 0;
			}

			printf_ioa_path_status(ioa);
		}
		return 0;
	}

	dev = find_dev(args[0]);
	if (!dev)
		return -ENXIO;

	printf("%s\n%s\n", status_hdr[2], status_sep[2]);

	if (ipr_is_ioa(dev))
		printf_ioa_path_status(dev->ioa);
	else
		printf_path_status(dev);

	return 0;
}

/**
 * query_recommended_stripe_size -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_recommended_stripe_size(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_array_cap_entry *cap;

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->is_aux_cache || !dev->ioa->supported_arrays)
		return -EINVAL;

	cap = get_cap_entry(dev->ioa->supported_arrays, args[1]);

	if (cap)
		printf("%d\n", ntohs(cap->recommended_stripe_size));

	return 0;
}

/**
 * query_supp_stripe_sizes -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_supp_stripe_sizes(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_array_cap_entry *cap;
	int i;

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->is_aux_cache || !dev->ioa->supported_arrays)
		return -EINVAL;

	cap = get_cap_entry(dev->ioa->supported_arrays, args[1]);

	if (!cap)
		return 0;

	for (i = 0; i < (sizeof(cap->supported_stripe_sizes) * 8); i++)
		if (ntohs(cap->supported_stripe_sizes) & (1 << i))
			printf("%d ", (1 << i));

	printf("\n");

	return 0;
}

/**
 * query_min_mult_in_array - Start array protection for an array
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_min_mult_in_array(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_array_cap_entry *cap;

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->is_aux_cache || !dev->ioa->supported_arrays)
		return -EINVAL;

	cap = get_cap_entry(dev->ioa->supported_arrays, args[1]);

	if (cap)
		printf("%d\n", cap->min_mult_array_devices);

	return 0;
}

/**
 * query_min_array_devices -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_min_array_devices(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_array_cap_entry *cap;

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->is_aux_cache || !dev->ioa->supported_arrays)
		return -EINVAL;

	cap = get_cap_entry(dev->ioa->supported_arrays, args[1]);

	if (cap)
		printf("%d\n", cap->min_num_array_devices);

	return 0;
}

/**
 * query_max_array_devices -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_max_array_devices(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_array_cap_entry *cap;

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->is_aux_cache || !dev->ioa->supported_arrays)
		return -EINVAL;

	cap = get_cap_entry(dev->ioa->supported_arrays, args[1]);

	if (cap)
		printf("%d\n", cap->max_num_array_devices);

	return 0;
}

/**
 * query_include_allowed -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_include_allowed(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_array_cap_entry *cap;

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->is_aux_cache || !dev->ioa->supported_arrays) {
		printf("no\n");
		return 0;
	}

	cap = get_cap_entry(dev->ioa->supported_arrays, args[1]);

	if (!cap) {
		printf("no\n");
		return 0;
	}

	if (cap->include_allowed)
		printf("yes\n");
	else
		printf("no\n");
	return 0;
}

/**
 * query_raid_levels -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_raid_levels(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	struct ipr_array_cap_entry *cap;

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->is_aux_cache || !dev->ioa->supported_arrays)
		return 0;

	for_each_cap_entry(cap, dev->ioa->supported_arrays)
		printf("%s ", cap->prot_level_str);

	printf("\n");
	return 0;
}

/**
 * query_bus_width -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_bus_width(char **args, int num_args)
{
	int rc, bus;
	struct ipr_scsi_buses page_28;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (!ioa_is_spi(dev->ioa) || dev->ioa->is_aux_cache)
		return -EINVAL;

	memset(&page_28, 0, sizeof(struct ipr_scsi_buses));

	rc = ipr_get_bus_attr(dev->ioa, &page_28);

	if (rc)
		return rc;

	bus = strtoul(args[1], NULL, 10);

	if (bus >= page_28.num_buses) {
		fprintf(stderr, "Invalid bus specified: %d\n", bus);
		return -EINVAL;
	}

	fprintf(stdout, "%d\n", page_28.bus[bus].bus_width);
	return 0;
}

/**
 * query_bus_speed -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_bus_speed(char **args, int num_args)
{
	int rc, bus;
	struct ipr_scsi_buses page_28;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (!ioa_is_spi(dev->ioa) || dev->ioa->is_aux_cache)
		return -EINVAL;

	memset(&page_28, 0, sizeof(struct ipr_scsi_buses));

	rc = ipr_get_bus_attr(dev->ioa, &page_28);

	if (rc)
		return rc;

	bus = strtoul(args[1], NULL, 10);

	if (bus >= page_28.num_buses) {
		fprintf(stderr, "Invalid bus specified: %d\n", bus);
		return -EINVAL;
	}

	fprintf(stdout, "%d\n",
		IPR_BUS_XFER_RATE_TO_THRUPUT(page_28.bus[bus].max_xfer_rate,
					     page_28.bus[bus].bus_width));
	return 0;
}

/**
 * query_initiator_id -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_initiator_id(char **args, int num_args)
{
	int rc, bus;
	struct ipr_scsi_buses page_28;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (!ioa_is_spi(dev->ioa) || dev->ioa->is_aux_cache)
		return -EINVAL;

	memset(&page_28, 0, sizeof(struct ipr_scsi_buses));

	rc = ipr_get_bus_attr(dev->ioa, &page_28);

	if (rc)
		return rc;

	bus = strtoul(args[1], NULL, 10);

	if (bus >= page_28.num_buses) {
		fprintf(stderr, "Invalid bus specified: %d\n", bus);
		return -EINVAL;
	}

	fprintf(stdout, "%d\n", page_28.bus[bus].scsi_id);
	return 0;
}

/**
 * query_add_remove -
 * @action:		action type
 *
 * Returns:
 *   0
 **/
static int query_add_remove(int action)
{
	int i, num_devs;
	struct ipr_dev **devs;

	num_devs = get_conc_devs(&devs, action);

	for (i = 0; i < num_devs; i++) {
		if (i == 0)
			printf("%s\n%s\n", status_hdr[11], status_sep[11]);

		printf_device(devs[i], 5);
	}

	free_empty_slots(devs, num_devs);
	free(devs);

	return 0;

}

/**
 * query_remove_device -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_remove_device(char **args, int num_args)
{
	return query_add_remove(IPR_CONC_REMOVE);
}

/**
 * query_add_device -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_add_device(char **args, int num_args)
{
	return query_add_remove(IPR_CONC_ADD);
}

/**
 * show_slots -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_slots(char **args, int num_args)
{
	return query_add_remove(IPR_CONC_IDENTIFY);
}

/**
 * query_log_level -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_log_level(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "Device is not an IOA\n");
		return -EINVAL;
	}

	printf("%d\n", get_log_level(dev->ioa));
	return 0;
}

/**
 * query_tcq_enable -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_tcq_enable(char **args, int num_args)
{
	int rc;
	struct ipr_disk_attr attr;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	rc = ipr_get_dev_attr(dev, &attr);

	if (rc)
		return rc;

	printf("%d\n", attr.tcq_enabled);
	return 0;
}

/**
 * query_qdepth -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_qdepth(char **args, int num_args)
{
	int rc;
	struct ipr_disk_attr attr;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	rc = ipr_get_dev_attr(dev, &attr);

	if (rc)
		return rc;

	printf("%d\n", attr.queue_depth);
	return 0;
}

/**
 * query_format_timeout -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_format_timeout(char **args, int num_args)
{
	int rc;
	struct ipr_disk_attr attr;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_af_dasd_device(dev)) {
		dprintf("%s is not an Advanced Function disk\n", args[0]);
		return -EINVAL;
	}

	rc = ipr_get_dev_attr(dev, &attr);

	if (rc)
		return rc;

	printf("%d\n", attr.format_timeout / 3600);
	return 0;
}

/**
 * query_ucode_level -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_ucode_level(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	u32 level, level_sw;
	char *asc;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (ipr_is_ioa(dev))
		printf("%08X\n", get_fw_version(dev));
	else {
		level = get_fw_version(dev);
		level_sw = htonl(level);
		asc = (char *)&level_sw;
		if (isprint(asc[0]) && isprint(asc[1]) &&
		    isprint(asc[2]) && isprint(asc[3]))
			printf("%.8X (%c%c%c%c)\n", level, asc[0],
			       asc[1], asc[2], asc[3]);
		else
			printf("%.8X\n", level);
	}

	return 0;
}

/**
 * query_format_for_raid -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_format_for_raid(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int hdr = 0;

	for_each_primary_ioa(ioa) {
		for_each_disk(ioa, dev) {
			if (!can_format_for_raid(dev))
				continue;

			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[2], status_sep[2]);
			}

			if (strlen(dev->dev_name))
				printf_device(dev, 0);
			else
				printf_device(dev, 2);
		}
	}

	return 0;
}

/**
 * query_raid_rebuild -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_raid_rebuild(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int hdr = 0;

	for_each_ioa(ioa) {
		for_each_af_dasd(ioa, dev) {
			if (!dev->dev_rcd->rebuild_cand)
				continue;

			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[2], status_sep[2]);
			}

			printf_device(dev, 2);
		}
	}

	return 0;
}

/**
 * query_recovery_format -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_recovery_format(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int hdr = 0;

	for_each_primary_ioa(ioa) {
		for_each_disk(ioa, dev) {
			if (ipr_is_hot_spare(dev) || !device_supported(dev))
				continue;
			if (ipr_is_array_member(dev) && !dev->dev_rcd->no_cfgte_vol)
				continue;;
			if (!is_format_allowed(dev))
				continue;

			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[2], status_sep[2]);
			}

			if (strlen(dev->dev_name))
				printf_device(dev, 0);
			else
				printf_device(dev, 2);
		}
	}

	return 0;
}

/**
 * query_devices_include -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_devices_include(char **args, int num_args)
{
	int rc, hdr = 0;;
	struct ipr_array_query_data qac_data;
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;
	struct ipr_dev *vset = find_dev(args[0]);
	struct ipr_dev_record *dev_rcd;

	if (!vset) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	ioa = vset->ioa;
	memset(&qac_data, 0, sizeof(qac_data));

	rc = ipr_query_array_config(ioa, 0, 1, 0, vset->array_id, &qac_data);

	if (rc)
		return rc;

	for_each_dev_rcd(dev_rcd, &qac_data) {
		for_each_disk(ioa, dev) {
			if (dev->scsi_dev_data->handle == ipr_get_dev_res_handle(ioa, dev_rcd) &&
			    dev_rcd->include_cand && device_supported(dev)) {
				if (!hdr) {
					hdr = 1;
					printf("%s\n%s\n", status_hdr[2], status_sep[2]);
				}

				printf_device(dev, 2);
				break;
			}
		}
	}

	return 0;
}

/**
 * query_arrays_include -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_arrays_include(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *vset;
	struct ipr_array_cap_entry *cap_entry;
	struct ipr_array_record *array_rcd;
	int hdr = 0;

	for_each_primary_ioa(ioa) {
		for_each_vset(ioa, vset) {
			array_rcd = vset->array_rcd;
			cap_entry = get_raid_cap_entry(ioa->supported_arrays,
						       vset->raid_level);

			if (!cap_entry || !cap_entry->include_allowed || !array_rcd->established)
				continue;

			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[3], status_sep[3]);
			}

			printf_device(vset, 1);
		}
	}

	return 0;
}

/**
 * query_reclaim -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_reclaim(char **args, int num_args)
{
	struct ipr_reclaim_query_data buf;
	struct ipr_ioa *ioa;
	int hdr = 0, rc;

	for_each_ioa(ioa) {
		memset(&buf, 0, sizeof(buf));
		rc = ipr_reclaim_cache_store(ioa, IPR_RECLAIM_QUERY, &buf);

		if (rc != 0)
			continue;

		if (buf.reclaim_known_needed || buf.reclaim_unknown_needed) {
			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[2], status_sep[2]);
			}

			printf_device(&ioa->ioa, 2);
		}
	}

	return 0;
}

/**
 * query_arrays_raid_migrate - Show the arrays that can be migrated to a
 * 			       different protection level.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_arrays_raid_migrate(char **args, int num_args)
{
	int hdr = 0;
	struct ipr_dev *array, *vset;
	struct ipr_ioa *ioa;

	for_each_primary_ioa(ioa) {
		for_each_array(ioa, array) {
			if (!array->array_rcd->migrate_cand)
				continue;
			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[3], status_sep[3]);
			}

			if (ioa->sis64) {
				vset = get_vset_from_array(ioa, array);
				printf_device(vset, 1);
			} else
				printf_device(array, 1);
		}
	}
	return 0;
}

/**
 * query_devices_raid_migrate - Given an array, show the AF disks that are
 * 				candidates to be used in a migration.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_devices_raid_migrate(char **args, int num_args)
{
	struct ipr_dev *dev;
	struct ipr_dev_record *dev_rcd;
	int hdr = 0;
	int rc = 0;
	struct ipr_array_query_data qac_data;

	memset(&qac_data, 0, sizeof(qac_data));

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->sis64 && ipr_is_volume_set(dev))
		dev = get_array_from_vset(dev->ioa, dev);

	if (!ipr_is_array(dev)) {
		fprintf(stderr, "%s is not an array.\n", args[0]);
		return -EINVAL;
	}

	/* query array config for volume set migrate status */
	rc = ipr_query_array_config(dev->ioa, 0, 0, 1, dev->array_id, &qac_data);
	if (rc != 0)
		return rc;

	for_each_dev_rcd(dev_rcd, &qac_data) {
		if (!dev_rcd->migrate_array_prot_cand)
			continue;
		if (!hdr) {
			hdr = 1;
			printf("%s\n%s\n", status_hdr[2], status_sep[2]);
		}

		dev = get_dev_from_handle(dev->ioa, ipr_get_dev_res_handle(dev->ioa, dev_rcd));
		if (dev)
			printf_device(dev, 2);
	}
	return 0;
}

/**
 * query_raid_levels_raid_migrate - Given an array, display the protection
 * 				    levels to which the array can be migrated.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_raid_levels_raid_migrate(char **args, int num_args)
{
	struct ipr_dev *dev;
	int rc = 0;
	struct ipr_array_query_data qac_data;
	struct ipr_supported_arrays *sa_rcd;
	struct ipr_array_cap_entry *cap_entry;

	memset(&qac_data, 0, sizeof(qac_data));

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->sis64 && ipr_is_volume_set(dev))
		dev = get_array_from_vset(dev->ioa, dev);

	if (!ipr_is_array(dev)) {
		fprintf(stderr, "%s is not an array.\n", args[0]);
		return -EINVAL;
	}

	if (!dev->array_rcd->migrate_cand) {
		scsi_err(dev, "%s is not a candidate for array migration.\n",
			 args[0]);
		return -EINVAL;
	}

	/* query array config for volume set migrate status */
	rc = ipr_query_array_config(dev->ioa, 0, 0, 1, dev->array_id, &qac_data);
	if (rc)
		return rc;

	for_each_supported_arrays_rcd(sa_rcd, &qac_data)
		for_each_cap_entry(cap_entry, sa_rcd)
			printf("%s ", cap_entry->prot_level_str);

	printf("\n");
	return 0;
}

/**
 * query_stripe_sizes_raid_migrate - Given an array and a protection level,
 * 				     show the valid stripe sizes to which the
 * 				     array can be migrated.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_stripe_sizes_raid_migrate(char **args, int num_args)
{
	struct ipr_dev *dev;
	int i;
	int rc = 0;
	struct ipr_array_query_data qac_data;
	struct ipr_supported_arrays *sa_rcd;
	struct ipr_array_cap_entry *cap_entry;

	memset(&qac_data, 0, sizeof(qac_data));

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->sis64 && ipr_is_volume_set(dev))
		dev = get_array_from_vset(dev->ioa, dev);

	if (!ipr_is_array(dev)) {
		fprintf(stderr, "%s is not an array.\n", args[0]);
		return -EINVAL;
	}

	/* query array config for volume set migrate status */
	rc = ipr_query_array_config(dev->ioa, 0, 0, 1, dev->array_id, &qac_data);
	if (rc)
		return rc;

	for_each_supported_arrays_rcd(sa_rcd, &qac_data) {
		for_each_cap_entry(cap_entry, sa_rcd) {
			if (strcmp(args[1], (char *)cap_entry->prot_level_str))
				continue;

			for (i = 0; i < (sizeof(cap_entry->supported_stripe_sizes) * 8); i++)
				if (ntohs(cap_entry->supported_stripe_sizes) & (1 << i))
					printf("%d ", (1 << i));

			printf("\n");
			break;
		}
	}

	return 0;
}

/**
 * query_devices_min_max_raid_migrate - Given an array and a protection level,
 * 					show the minimum and maximum number of
 * 					devices needed to do the migration.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int query_devices_min_max_raid_migrate(char **args, int num_args)
{
	struct ipr_dev *dev;
	int rc = 0;
	struct ipr_array_query_data qac_data;
	struct ipr_supported_arrays *supported_arrays;
	struct ipr_array_cap_entry *cap;

	memset(&qac_data, 0, sizeof(qac_data));

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->sis64 && ipr_is_volume_set(dev))
		dev = get_array_from_vset(dev->ioa, dev);

	if (!ipr_is_array(dev)) {
		fprintf(stderr, "%s is not an array.\n", args[0]);
		return -EINVAL;
	}

	/* query array config for volume set migrate status */
	rc = ipr_query_array_config(dev->ioa, 0, 0, 1, dev->array_id, &qac_data);
	if (rc)
		return rc;

	/* check that the given raid level is valid */
	for_each_supported_arrays_rcd(supported_arrays, &qac_data) {
		cap = get_cap_entry(supported_arrays, args[1]);
		if (cap)
			break;
	}

	if (!cap) {
		fprintf(stderr, "RAID level %s is unsupported.\n", args[1]);
		return -EINVAL;
	}

	if (cap->format_overlay_type == IPR_FORMAT_REMOVE_DEVICES)
		fprintf(stderr, "%d %s will be removed from the array\n",
			cap->min_num_array_devices,
			cap->min_num_array_devices == 1 ? "device" : "devices");
	else if (cap->format_overlay_type == IPR_FORMAT_ADD_DEVICES) {
		fprintf(stderr, "Minimum number of devices required = %d\n",
				cap->min_num_array_devices);
		fprintf(stderr, "Maximum number of devices allowed = %d\n",
				cap->max_num_array_devices);
		fprintf(stderr, "Number of devices must be a multiple of %d\n",
				cap->min_mult_array_devices);
	} else {
		fprintf(stderr, "Unknown overlay type in qac data\n");
		return -EINVAL;
	}

	return 0;
}

/**
 * query_ioas_asym_access - Show the ioas that support asymmetric access.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_ioas_asym_access(char **args, int num_args)
{
	int hdr = 0;
	struct ipr_ioa *ioa;

	for_each_ioa(ioa) {
		if (!ioa->asymmetric_access)
			continue;
		if (!hdr) {
			hdr = 1;
			printf("%s\n%s\n", status_hdr[3], status_sep[3]);
		}
		printf_device(&ioa->ioa, 2);
	}
	return 0;
}

/**
 * query_arrays_asym_access - Show the arrays that support asymmetric access.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_arrays_asym_access(char **args, int num_args)
{
	int hdr = 0;
	struct ipr_ioa *ioa;
	struct ipr_dev *array;

	for_each_ioa(ioa) {
		if (!ioa->asymmetric_access || !ioa->asymmetric_access_enabled)
			continue;
		for_each_array(ioa, array) {
			if (!array->array_rcd->asym_access_cand)
				continue;
			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[3], status_sep[3]);
			}
			if (ioa->sis64)
				printf_device(array, 4);
			else
				printf_device(array, 1);
		}
	}
	return 0;
}

/**
 * query_ioa_asym_access_mode - Show the current asymmetric access state for
 * 				the given IOA.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_ioa_asym_access_mode(char **args, int num_args)
{
	struct ipr_dev *dev;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}
	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "%s is not an IOA.\n", args[0]);
		return -EINVAL;
	}

	if (dev->ioa->asymmetric_access_enabled)
		printf("Enabled\n");
	else if (dev->ioa->asymmetric_access)
		printf("Disabled\n");
	else
		printf("Unsupported\n");

	return 0;
}

/**
 * query_array_asym_access_mode - Show the current asymmetric access state for
 * 				  the given array.
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_array_asym_access_mode(char **args, int num_args)
{
	struct ipr_dev *array;

	array = find_dev(args[0]);
	if (!array) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_array(array)) {
		if (array->ioa->sis64 && ipr_is_volume_set(array))
			array = get_array_from_vset(array->ioa, array);
		else {
			fprintf(stderr, "%s is not an array.\n", array->gen_name);
			return -EINVAL;
		}
	}

	if (!array->array_rcd->asym_access_cand)
		printf("Unsupported\n");
	else if (array->array_rcd->current_asym_access_state == IPR_ACTIVE_OPTIMIZED)
		printf("Optimized\n");
	else if (array->array_rcd->current_asym_access_state == IPR_ACTIVE_NON_OPTIMIZED)
		printf("Not Optimized\n");
	else
		scsi_dbg(array, "Unrecognized state - %d.", array->array_rcd->current_asym_access_state);

	return 0;
}

/**
 * query_format_for_jbod -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_format_for_jbod(char **args, int num_args)
{
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;
	int hdr = 0;

	for_each_primary_ioa(ioa) {
		for_each_af_dasd(ioa, dev) {
			if (ipr_device_is_zeroed(dev))
				continue;
			if ((ipr_is_array_member(dev) && dev->dev_rcd->no_cfgte_vol) ||
			    (!ipr_is_array_member(dev) && is_format_allowed(dev))) {
				if (!hdr) {
					hdr = 1;
					printf("%s\n%s\n", status_hdr[2], status_sep[2]);
				}

				printf_device(dev, 2);
			}
		}
	}

	return 0;
}

/**
 * show_dev_details -
 * @dev:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
static void show_dev_details(struct ipr_dev *dev)
{
	char *body = NULL;

	if (ipr_is_ioa(dev))
		body = ioa_details(body, dev);
	else if (ipr_is_volume_set(dev) || ipr_is_array(dev))
		body = vset_array_details(body, dev);
	else if (ipr_is_ses(dev))
		body = ses_details(body, dev);
	else
		body = disk_details(body, dev);

	printf("%s\n", body);
	free(body);
}

/**
 * show_jbod_disks -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_jbod_disks(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int hdr = 0;

	for_each_ioa(ioa) {
		for_each_jbod_disk(ioa, dev) {
			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[2], status_sep[2]);
			}

			printf_device(dev, 0);
		}
	}

	return 0;
}

/**
 * show_all_af_disks -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_all_af_disks(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int hdr = 0;

	for_each_ioa(ioa) {
		for_each_af_dasd(ioa, dev) {
			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[2], status_sep[2]);
			}

			printf_device(dev, 2);
		}
	}

	return 0;
}

/**
 * show_af_disks -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_af_disks(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int hdr = 0;

	for_each_ioa(ioa) {
		for_each_standalone_af_disk(ioa, dev) {
			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[2], status_sep[2]);
			}

			printf_device(dev, 2);
		}
	}

	return 0;
}

/**
 * show_hot_spares -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_hot_spares(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int hdr = 0;

	for_each_ioa(ioa) {
		for_each_hot_spare(ioa, dev) {
			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[2], status_sep[2]);
			}

			printf_device(dev, 2);
		}
	}

	return 0;
}

/**
 * show_details -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_details(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Missing device: %s\n", args[0]);
		return -EINVAL;
	}

	show_dev_details(dev);
	return 0;
}

/**
 * __print_status -
 * @args:		argument vector
 * @num_args:		number of arguments
 * @percent:		percent value
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __print_status(char **args, int num_args, int percent)
{
	struct ipr_dev *dev = find_dev(args[0]);
	char buf[100];

	if (!dev) {
		printf("Missing\n");
		return -EINVAL;
	}

	get_status(dev, buf, percent, 0);
	printf("%s\n", buf);
	return 0;
}

/**
 * print_alt_status -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int print_alt_status(char **args, int num_args)
{
	return __print_status(args, num_args, 1);
}

/**
 * print_status -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int print_status(char **args, int num_args)
{
	return __print_status(args, num_args, 0);
}

/**
 * __show_config -
 * @type:		type value
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __show_config(int type)
{
	struct ipr_ioa *ioa;

	printf("%s\n%s\n", status_hdr[(type&1)+2], status_sep[(type&1)+2]);
	for_each_ioa(ioa)
		printf_ioa(ioa, type);
	return 0;
}

/**
 * show_config -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_config(char **args, int num_args)
{
	return __show_config(3);
}

/**
 * show_alt_config - 
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_alt_config(char **args, int num_args)
{
	return __show_config(2);
}

/**
 * show_ioas -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_ioas(char **args, int num_args)
{
	struct ipr_ioa *ioa;

	if (num_ioas)
		printf("%s\n%s\n", status_hdr[2], status_sep[2]);
	else
		printf("No IOAs found\n");
	for_each_ioa(ioa)
		printf_device(&ioa->ioa, 2);

	return 0;
}

/**
 * show_arrays -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int show_arrays(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *vset;
	int hdr = 0;

	for_each_ioa(ioa) {
		for_each_vset(ioa, vset) {
			if (!hdr) {
				hdr = 1;
				printf("%s\n%s\n", status_hdr[3], status_sep[3]);
			}

			printf_device(vset, 3);
		}
	}

	return 0;
}

/**
 * battery_info -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int battery_info(char **args, int num_args)
{
	struct ipr_reclaim_query_data reclaim;
	struct ipr_dev *dev;
	char *buf;
	int rc;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	rc = ipr_reclaim_cache_store(dev->ioa,
				     IPR_RECLAIM_QUERY | IPR_RECLAIM_EXTENDED_INFO,
				     &reclaim);

	if (rc)
		return rc;

	dev->ioa->reclaim_data = &reclaim;
	buf = get_battery_info(dev->ioa);
	printf("%s\n", buf);
	dev->ioa->reclaim_data = NULL;
	free(buf);
	return 0;
}

/**
 * get_ha_mode -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int get_ha_mode(char **args, int num_args)
{
	struct ipr_dev *dev = find_gen_dev(args[0]);

	if (!dev)
		return -ENXIO;

	if (dev->ioa->in_gscsi_only_ha)
		printf("JBOD\n");
	else
		printf("Normal\n");
	return 0;
}

/**
 * __set_ha_mode -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __set_ha_mode(char **args, int num_args)
{
	struct ipr_dev *dev = find_gen_dev(args[0]);

	if (!dev)
		return -ENXIO;

	if (!strncasecmp(args[1], "Normal", 4))
		return set_ha_mode(dev->ioa, 0);
	else if (!strncasecmp(args[1], "JBOD", 4))
		return set_ha_mode(dev->ioa, 1);

	return -EINVAL; 
}

/**
 * __set_preferred_primary -
 * @sg_name:		sg device name
 * @preferred_primary:	preferred primary value
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __set_preferred_primary(char *sg_name, int preferred_primary)
{
	struct ipr_dev *dev = find_gen_dev(sg_name);

	if (dev)
		return set_preferred_primary(dev->ioa, preferred_primary);
	return -ENXIO;
}

/**
 * set_primary -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_primary(char **args, int num_args)
{
	return __set_preferred_primary(args[0], 1);
}

/**
 * set_secondary -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_secondary(char **args, int num_args)
{
	return __set_preferred_primary(args[0], 0);
}

/**
 * set_all_primary -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_all_primary(char **args, int num_args)
{
	struct ipr_ioa *ioa;

	for_each_ioa(ioa)
		set_preferred_primary(ioa, 1);

	return 0;
}

/**
 * set_all_secondary -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_all_secondary(char **args, int num_args)
{
	struct ipr_ioa *ioa;

	for_each_ioa(ioa)
		set_preferred_primary(ioa, 0);

	return 0;
}

/**
 * check_and_set_active_active - Do some sanity checks and then set the
 * 				 active/active state to the requested mode.
 * @dev_name:		specified IOA
 * @mode:		mode to set - enable or disable
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int check_and_set_active_active(char *dev_name, int mode)
{
	struct ipr_dev *dev;

	dev = find_dev(dev_name);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", dev_name);
		return -EINVAL;
	}
	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "%s is not an IOA.\n", dev_name);
		return -EINVAL;
	}

	/* Check that asymmetric access is supported by the adapter. */
	if (!dev->ioa->asymmetric_access) {
		ioa_err(dev->ioa, "IOA does not support asymmetric access.\n");
		return -EINVAL;
	}

	/* Check the state of asymmetric access. */
	if (dev->ioa->asymmetric_access_enabled == mode) {
		ioa_dbg(dev->ioa, "Asymmetric access is already %s.\n",
			mode == 0 ? "disabled" : "enabled");
		return 0;
	}

	return set_active_active_mode(dev->ioa, mode);
}

/**
 * get_drive_phy_loc - get drive physical location
 * @ioa:	ipr ioa struct
 *
 * Returns:
 *   body
 **/
static int get_drive_phy_loc(struct ipr_ioa *ioa)
{
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status, *overall;
	struct ipr_dev *ses, *dev;
	struct ipr_ses_config_pg ses_cfg;
	int ses_bus, scsi_id_found, is_spi, is_vses;
	struct drive_elem_desc_pg drive_data;
	char phy_loc[PHYSICAL_LOCATION_LENGTH + 1];
	int times, index;

	for_each_ioa(ioa)  {
		is_spi = ioa_is_spi(ioa);

		for_each_hotplug_dev(ioa, dev) {
			if (ioa->sis64)
				get_res_path(dev);
			else
				get_res_addrs(dev);
		}

		for_each_ses(ioa, ses) {
			times = 5;
			if (strlen(ses->physical_location) == 0)
				get_ses_phy_loc(ses);
			while (times--) {
				if (!ipr_receive_diagnostics(ses, 2, &ses_data, sizeof(ses_data)))
					break;
			}
			if (times < 0 ) continue;

			if (ipr_receive_diagnostics(ses, 1, &ses_cfg, sizeof(ses_cfg)))
				continue;

			if (ipr_receive_diagnostics(ses, 7, &drive_data, sizeof(drive_data)))
				continue;

			overall = ipr_get_overall_elem(&ses_data, &ses_cfg);
			ses_bus = ses->scsi_dev_data->channel;
			scsi_id_found = 0;

			if (!is_spi && (overall->device_environment == 0))
				is_vses = 1;
			else
				is_vses = 0;

			scsi_dbg(ses, "%s\n", is_vses ? "Found VSES" : "Found real SES");

			for_each_elem_status(elem_status, &ses_data, &ses_cfg) {
				index = index_in_page2(&ses_data, elem_status->slot_id);
				if (index != -1)
					get_drive_phy_loc_with_ses_phy_loc(ses, &drive_data, index, phy_loc, 0);

				if (elem_status->status == IPR_DRIVE_ELEM_STATUS_UNSUPP)
					continue;
				if (elem_status->status == IPR_DRIVE_ELEM_STATUS_NO_ACCESS)
					continue;
				if (is_spi && (scsi_id_found & (1 << elem_status->slot_id)))
					continue;
				scsi_id_found |= (1 << elem_status->slot_id);

				if (ioa->sis64)
					dev = get_dev_for_slot_64bit(ses, elem_status->slot_id, phy_loc);
				else
					dev = get_dev_for_slot(ses, elem_status->slot_id, is_vses, phy_loc);
				if (!dev)
					continue;

			}

		}
	}
	return 0;
}

/**
 * set_ioa_asymmetric_access - Change the asymmetric access mode for an IOA.
 *			       Enabling occurs by sending a change multi adapter
 *			       assignment command from ipr_set_ioa_attr().
 *			       Disabling occurs by resetting the adapter and
 *			       then sending a mode select
 *			       page 24 in ipr_set_active_active_mode().
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_ioa_asymmetric_access(char **args, int num_args)
{
	if (!strncasecmp(args[1], "Enable", 4))
		return check_and_set_active_active(args[0], 1);
	else if (!strncasecmp(args[1], "Disable", 4))
		return check_and_set_active_active(args[0], 0);
	return -EINVAL;
}

/**
 * set_array_asymmetric_access - Set the desired asymmetric access state of the
 * 				 array to "Optimized" or "Non-Optimized".
 *
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_array_asymmetric_access(char **args, int num_args)
{
	int state;
	struct ipr_dev *array = NULL;

	array = find_dev(args[0]);
	if (!array) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_array(array)) {
		if (array->ioa->sis64 && ipr_is_volume_set(array))
			array = get_array_from_vset(array->ioa, array);
		else {
			scsi_err(array,  "Given device is not an array.");
			return -EINVAL;
		}
	}

	/* Check that the adapter is a primary adapter. */
	if (array->ioa->is_secondary) {
		ioa_err(array->ioa, "Asymmetric access commands must be issued "
				   "for arrays on the primary IOA");
		return -EINVAL;
	}

	/* Check that asymmetric access is supported by the adapter. */
	if (!array->ioa->asymmetric_access) {
		ioa_err(array->ioa, "Asymmetric access is not supported.");
		return -EINVAL;
	}

	/* Check that asymmetric access is enabled on the adapter. */
	if (!array->ioa->asymmetric_access_enabled) {
		ioa_err(array->ioa, "Asymmetric access is not enabled.");
		return -EINVAL;
	}

	/* Optimized or Non-optimized? */
	if (!strncasecmp(args[1], "Optimized", 3))
		state = IPR_ACTIVE_OPTIMIZED;
	else if (!strncasecmp(args[1], "Non-Optimized", 3))
		state = IPR_ACTIVE_NON_OPTIMIZED;
	else {
		scsi_err(array, "Unrecognized state given for asymmetric access.");
		return -EINVAL;
	}

	if (array->array_rcd->saved_asym_access_state == state) {
		scsi_dbg(array, "Array is already set to requested state.");
		return 0;
	}

	if (array->array_rcd->asym_access_cand)
		array->array_rcd->issue_cmd = 1;
	else {
		scsi_err(array, "%s is not a candidate for asymmetric access.",
			 array->dev_name);
		return -EINVAL;
	}

	array->array_rcd->saved_asym_access_state = state;

	return ipr_set_array_asym_access(array->ioa);
}

static int __query_array_label(struct ipr_ioa *ioa, char *name)
{
	struct ipr_dev *vset;

	if (ioa->sis64) {
		for_each_vset(ioa, vset) {
			if (!strcmp((char *)vset->array_rcd->type3.desc, name)) {
				fprintf(stdout, "%s\n", vset->dev_name);
				return 0;
			}
		}
	}

	return -ENODEV;
}

/**
 * query_array_label - Display the device name for the specified label
 * @args:	       argument vector
 * @num_args:	       number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_array_label(char **args, int num_args)
{
	struct ipr_ioa *ioa;

	for_each_primary_ioa(ioa) {
		if (!__query_array_label(ioa, args[0]))
			return 0;
	}

	for_each_secondary_ioa(ioa) {
		if (!__query_array_label(ioa, args[0]))
			return 0;
	}

	return -ENODEV;
}

static int __query_array(struct ipr_ioa *ioa, char *name)
{
	struct ipr_dev *vset;
	struct ipr_dev *dev;

	get_drive_phy_loc(ioa);

	for_each_vset(ioa, vset) {
		for_each_dev_in_vset(vset, dev) {
			strip_trailing_whitespace(dev->physical_location);
			if (!strcmp(dev->physical_location, name)) {
				fprintf(stdout, "%s\n", vset->dev_name);
				return 0;
			}
		}
	}

	return -ENODEV;
}

/**
 * query_array - Display the array device name for the specified device location
 * @args:	       argument vector
 * @num_args:	       number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_array(char **args, int num_args)
{
	struct ipr_ioa *ioa;

	for_each_primary_ioa(ioa) {
		if (!__query_array(ioa, args[0]))
			return 0;
	}

	for_each_secondary_ioa(ioa) {
		if (!__query_array(ioa, args[0]))
			return 0;
	}

	return -ENODEV;
}

static int __query_device(struct ipr_ioa *ioa, char *name)
{
	struct ipr_dev *dev;

	get_drive_phy_loc(ioa);

	for_each_disk(ioa, dev) {
		strip_trailing_whitespace(dev->physical_location);
		if (!strcmp(dev->physical_location, name)) {
			if (strlen(dev->dev_name))
				fprintf(stdout, "%s\n", dev->dev_name);
			else
				fprintf(stdout, "%s\n", dev->gen_name);
			return 0;
		}
	}

	return -ENODEV;
}

/**
 * query_device - Display the device name for the specified device location
 * @args:	       argument vector
 * @num_args:	       number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_device(char **args, int num_args)
{
	struct ipr_ioa *ioa;

	for_each_primary_ioa(ioa) {
		if (!__query_device(ioa, args[0]))
			return 0;
	}

	for_each_secondary_ioa(ioa) {
		if (!__query_device(ioa, args[0]))
			return 0;
	}

	return -ENODEV;
}


/**
 * query_location - Display the physical location for the specified device name
 * @args:	       argument vector
 * @num_args:	       number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_location(char **args, int num_args)
{
	struct ipr_dev *dev, *tmp, *cdev;
	int num_devs = 0;

	dev = find_dev(args[0]);

	if (!dev)
		return -ENODEV;

	get_drive_phy_loc(dev->ioa);

	if (!ipr_is_volume_set(dev)){
		if (ipr_is_ioa(dev))
			fprintf(stdout, "%s\n", dev->ioa->physical_location);
		else
			fprintf(stdout, "%s\n", dev->physical_location);
		return 0;
	}

	for_each_dev_in_vset(dev, tmp) {
		cdev = tmp;
		num_devs++;
	}

	if (num_devs != 1)
		return -EINVAL;

	fprintf(stdout, "%s\n", cdev->physical_location);
	return 0;
}

/**
 * query_ioa_caching - Show whether or not user requested caching mode is set
 * 		       to default or disabled.
 * @args:	       argument vector
 * @num_args:	       number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_ioa_caching(char **args, int num_args)
{
	int mode;
	struct ipr_dev *dev;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}
	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "%s is not an IOA.\n", args[0]);
		return -EINVAL;
	}
	if (!dev->ioa->has_cache) {
		printf("none\n");
		return 0;
	}

	mode = get_ioa_caching(dev->ioa);

	switch (mode) {
	case IPR_IOA_REQUESTED_CACHING_DISABLED:
		printf("disabled\n");
		break;
	case IPR_IOA_CACHING_DISABLED_DUAL_ENABLED:
		printf("disabled, 0x3\n");
		break;
	case IPR_IOA_CACHING_DEFAULT_DUAL_ENABLED:
		printf("default, 0x3\n");
		break;
	default:
		printf("default\n");
		break;
	}

	return 0;
}

/**
 * set_ioa_caching - Set the requested IOA caching mode to default or disabled.
 * @args:	     argument vector
 * @num_args:	     number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_ioa_caching(char **args, int num_args)
{
	int mode, rc;
	struct ipr_dev *dev;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}
	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "%s is not an IOA.\n", args[0]);
		return -EINVAL;
	}
	if (dev->ioa->is_aux_cache) {
		return -EINVAL;
	}
	if (!dev->ioa->has_cache) {
		fprintf(stderr, "%s does not have cache.\n", args[0]);
		return -EINVAL;
	}

	if (!strncasecmp(args[1], "Default", 3))
		mode = IPR_IOA_SET_CACHING_DEFAULT;
	else if (!strncasecmp(args[1], "Disabled", 3))
		mode = IPR_IOA_SET_CACHING_DISABLED;
	else if (!strncasecmp(args[1], "Dual-Disabled", 8))
		mode = IPR_IOA_SET_CACHING_DUAL_DISABLED;
	else if (!strncasecmp(args[1], "Dual-Enabled", 8))
		mode = IPR_IOA_SET_CACHING_DUAL_ENABLED;
	else {
		scsi_err(dev, "Unrecognized mode given for IOA caching.");
		return -EINVAL;
	}

	rc = ipr_change_cache_parameters(dev->ioa , mode);

	return rc;
}

/**
 * set_array_rebuild_verify - Enable/Disable Verification during an array rebuild.
 * @args:	     IOA
 * @num_args:
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_array_rebuild_verify(char **args, int num_args)
{
	int rc;
	struct ipr_ioa_attr attr;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int disable_rebuild_verify;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}
	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "%s is not an IOA.\n", args[0]);
		return -EINVAL;
	}

	if (strncasecmp(args[1], "enable", 6) == 0)
		disable_rebuild_verify = 0;
	else if (strncasecmp(args[1], "disable", 7) == 0
		 || strncasecmp(args[1], "default", 7) == 0)
		disable_rebuild_verify = 1;
	else {
		scsi_err(dev, "'%s' is not a valid value.\n"
			 "Accepted values are [enable|disable].\n",
			 args[1]);
		return -EINVAL;
	}

	ioa = dev->ioa;

	if (ioa->configure_rebuild_verify == 0
	    && disable_rebuild_verify == 1) {
		scsi_err(dev, "Adapter doesn't support disabling array "
			 "rebuild data verification.\n");
		return -EINVAL;
	}

	if (ipr_get_ioa_attr(ioa, &attr))
		return -EIO;

	attr.disable_rebuild_verify = disable_rebuild_verify;

	rc = ipr_set_ioa_attr(ioa, &attr, 1);
	if (rc) {
		scsi_err(ioa->dev, "Unable to %s array rebuild verification.",
			 args[1]);
		return rc;
	}
	return 0;
}

/**
 * query_array_rebuild_verify - Query whether verification is enabled
 * during an array rebuild.
 * @args:	     IOA
 * @num_args:
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_array_rebuild_verify(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}
	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "%s is not an IOA.\n", args[0]);
		return -EINVAL;
	}

	ioa = dev->ioa;

	if (ioa->disable_rebuild_verify == 1)
		printf("Disabled\n");
	else
		printf("Enabled\n");
	return 0;
}

/**
 * set_array_rebuild_rate - Set the Array Rebuild Rate for every array of an IOA.
 * @args:	     IOA
 * @num_args:	     Array Rebuild Rate in range [0-100]
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_array_rebuild_rate(char**args, int num_args)
{
	int err_rebuild_rate = 0;
	int rebuild_rate = 0;
	int rc;
	struct ipr_ioa_attr attr;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;

	dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}
	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "%s is not an IOA.\n", args[0]);
		return -EINVAL;
	}

	if (strncmp(args[1], "default", 7) == 0) {
		rebuild_rate = 0;
	} else {
		char *endptr = NULL;
		rebuild_rate = strtol(args[1], &endptr, 10);
		if (endptr == args[1]
		    || rebuild_rate < 10 || rebuild_rate > 100) {
			scsi_err(dev,
				 "'%s' is not a valid rebuild rate value.\n"
				 "Supported values are in the range 10-100.\n"
				 "Use 'default' to reset the configuration.\n"
				 "Higher values may affect performance but "
				 "decrease the total rebuild time.\n", args[1]);
			return -EINVAL;
		}
		err_rebuild_rate = rebuild_rate;
		rebuild_rate = (rebuild_rate * 15) / 100;
	}

	ioa = dev->ioa;
	if (!ioa->sis64) {
		scsi_err(dev,
			 "Adapter doesn't support modifying Array Rebuild Rate\n.");
		return -EINVAL;
	}

	if (ipr_get_ioa_attr(ioa, &attr))
		return -EIO;

	attr.rebuild_rate = rebuild_rate;

	rc = ipr_set_ioa_attr(ioa, &attr, 1);
	if (rc) {
		scsi_err(ioa->dev, "Unable to set rebuild rate value %d",
			 err_rebuild_rate);
		return rc;
	}
	return 0;
}

/**
 * query_array_rebuild_rate - Fetch the Array Rebuild Rate of an IOA.
 * @args:	     IOA
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_array_rebuild_rate(char**args, int num_args)
{
	struct ipr_ioa_attr attr;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int rebuild_rate = 0;

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}
	if (!ipr_is_ioa(dev)) {
		fprintf(stderr, "%s is not an IOA.\n", args[0]);
		return -EINVAL;
	}

	ioa = dev->ioa;
	if (!ioa->sis64) {
		scsi_err(dev,
			 "Adapter doesn't support modifying Array Rebuild Rate\n.");
		return -EINVAL;
	}

	if (ipr_get_ioa_attr(ioa, &attr))
		return -EIO;

	rebuild_rate = (attr.rebuild_rate * 100) / 15;

	if (rebuild_rate == 0)
		printf("default\n");
	else
		printf("%d\n", rebuild_rate);

	return 0;
}

/**
 * get_live_dump - 
 * @args:	     argument vector
 * @num_args:	     number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int get_live_dump (char **args, int num_args)
{
	struct ipr_dev *dev = find_gen_dev(args[0]);

	if (dev)
		return ipr_get_live_dump(dev->ioa);
	return -ENXIO;
}

/**
 * query_ses_mode - 
 * @args:	     argument vector
 * @num_args:	     number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_ses_mode (char **args, int num_args)
{
	struct ipr_dev *dev = find_gen_dev(args[0]);
	struct ipr_ses_inquiry_pageC3 pageC3_inq;
	int rc;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_ses(dev)) {
		fprintf(stderr, "Invalid device type\n");
		return -EINVAL;
	}

	memset(&pageC3_inq, 0, sizeof(pageC3_inq));
	rc = ipr_inquiry(dev, 0xc3, &pageC3_inq, sizeof(pageC3_inq));

	if (rc != 0) {
		scsi_dbg(dev, "Inquiry failed\n");
		return -EIO;
	}

	printf("%c%c%c%c\n", pageC3_inq.mode[0], pageC3_inq.mode[1], 
	       pageC3_inq.mode[2], pageC3_inq.mode[3]);

	return 0;
}

/**
 * set_ses_mode - 
 * @args:	     argument vector
 * @num_args:	     number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_ses_mode (char **args, int num_args)
{
	struct ipr_dev *dev = find_gen_dev(args[0]);
	int mode = atoi(args[1]);
	int rc;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (!ipr_is_ses(dev)) {
		fprintf(stderr, "Invalid device type\n");
		return -EINVAL;
	}

	rc = ipr_set_ses_mode(dev, mode);
	if (rc != 0) {
		scsi_dbg(dev, "Change SES mode command failed");
		return -EIO;
	}
	return 0;
}

/**
 * show_enclosure_info -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int show_enclosure_info(struct ipr_dev *ses)
{
	struct screen_output *s_out;
	int rc;

	if (ses->scsi_dev_data->type == TYPE_ENCLOSURE)
		n_show_enclosure_info.body = ses_details(n_show_enclosure_info.body, ses);
	else
		n_show_enclosure_info.body = ioa_details(n_show_enclosure_info.body, ses);
	s_out = screen_driver(&n_show_enclosure_info, 0, NULL);

	free(n_show_enclosure_info.body);
	n_show_enclosure_info.body = NULL;
	rc = s_out->rc;
	free(s_out);
	return rc;
}

int ipr_suspend_disk_enclosure(i_container *i_con)
{
	int rc, suspend_rc;
	struct ipr_ioa *ioa;
	struct ipr_dev *ses;

	suspend_rc = rc = RC_SUCCESS;

	for_each_ioa(ioa) {
		for_each_ses(ioa, ses) {
			if (ses->is_suspend_cand) {
				rc = ipr_suspend_device_bus(ses, &ses->res_addr[0],
						    IPR_SDB_CHECK_AND_QUIESCE_ENC);
				if (rc !=0 )
					suspend_rc = RC_82_Suspended_Fail;
				else
					rc = ipr_write_dev_attr(ses, "state", "offline\n");
			}
		}
	}
	if (suspend_rc != RC_SUCCESS)
		return suspend_rc | EXIT_FLAG;

	return  RC_81_Suspended_Success | EXIT_FLAG;
}

int ipr_resume_disk_enclosure(i_container *i_con)
{
	int rc, resume_rc;
	struct ipr_ioa *ioa;
	struct ipr_dev *ses;

	resume_rc = rc = RC_SUCCESS;

	for_each_ioa(ioa) {
		for_each_ses(ioa, ses) {
			if (ses->is_resume_cand) {
				rc = ipr_resume_device_bus(ses, &ses->res_addr[0]);
				if (rc !=0 )
					resume_rc = RC_86_Enclosure_Resume_Fail;
				else
					ipr_write_dev_attr(ses, "state", "running\n");
			}
		}
	}
	if (resume_rc != RC_SUCCESS)
		return resume_rc | EXIT_FLAG;

	return  RC_85_Enclosure_Resume_Success | EXIT_FLAG;
}

/**
 * confirm_suspend_disk_enclosure -
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int confirm_suspend_disk_enclosure(void)
{
	int rc,k;
	struct ipr_ioa *ioa;
	struct ipr_dev *ses;
	char *buffer[2];
	struct screen_output *s_out;
	int header_lines;
	int toggle= 0;

	rc = RC_SUCCESS;

	body_init_status_enclosure(buffer, n_confirm_suspend_disk_enclosure.header, &header_lines);

	for_each_ioa(ioa) {
		for_each_ses(ioa, ses) {
			if (!ses->is_suspend_cand)
				continue;
			print_dev_enclosure(k, ses, buffer, "2", k);
		}
	}

	do {
		n_confirm_suspend_disk_enclosure.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_suspend_disk_enclosure, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);
	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_suspend_disk_enclosure.body = NULL;

	return rc;
}

/**
 * confirm_resume_disk_enclosure -
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int confirm_resume_disk_enclosure(void)
{
	int rc,k;
	struct ipr_ioa *ioa;
	struct ipr_dev *ses;
	char *buffer[2];
	struct screen_output *s_out;
	int header_lines;
	int toggle= 0;

	rc = RC_SUCCESS;

	body_init_status_enclosure(buffer, n_confirm_resume_disk_enclosure.header, &header_lines);

	for_each_ioa(ioa) {
		for_each_ses(ioa, ses) {
			if (!ses->is_resume_cand)
				continue;
			print_dev_enclosure(k, ses, buffer, "3", k);
		}
	}

	do {
		n_confirm_resume_disk_enclosure.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_resume_disk_enclosure, header_lines, NULL);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_resume_disk_enclosure.body = NULL;

	return rc;
}


/**
 * enclosures_fork -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int enclosures_fork(i_container *i_con)
{
	int rc = 0, suspend_flag =0, resume_flag = 0;
	struct ipr_dev *ses;
	struct ipr_ioa *ioa;
	char *input;
	i_container *temp_i_con;

	for_each_icon(temp_i_con) {
		ses = (struct ipr_dev *)temp_i_con->data;
		if (ses == NULL)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "3") == 0) {
			if (ipr_is_ioa(ses))
				return RC_89_Invalid_Dev_For_Resume;

			if (ses->active_suspend == IOA_DEV_PORT_UNKNOWN)	
				return RC_90_Enclosure_Is_Unknown;

			if (ses->active_suspend == IOA_DEV_PORT_ACTIVE)	
				return RC_84_Enclosure_Is_Active;

			ses->is_resume_cand = 1;
			resume_flag = 1;
		} else if (strcmp(input, "2") == 0) {
				if (ipr_is_ioa(ses))
					return RC_88_Invalid_Dev_For_Suspend;

				if (ses->active_suspend == IOA_DEV_PORT_UNKNOWN)	
					return RC_90_Enclosure_Is_Unknown;

				if (!strncmp((char *)&ses->serial_number, (char *)&ses->ioa->yl_serial_num[0], ESM_SERIAL_NUM_LEN))
					return RC_87_No_Suspend_Same_Seri_Num;

				if (ses->active_suspend == IOA_DEV_PORT_SUSPEND)	
					return RC_83_Enclosure_Is_Suspend;

				ses->is_suspend_cand = 1;
				suspend_flag = 1;
		} else if (strcmp(input, "1") == 0)	{
			rc = show_enclosure_info(ses);
			return rc;
		} 
	}

	if (suspend_flag) {
		rc =confirm_suspend_disk_enclosure();
		for_each_ioa(ioa)
			for_each_ses(ioa, ses) 
				ses->is_suspend_cand = 0;
		return rc;
	}

	if (resume_flag) {
		rc = confirm_resume_disk_enclosure();
		for_each_ioa(ioa)
			for_each_ses(ioa, ses) 
				ses->is_resume_cand = 0;
		return rc;
	}

	return INVALID_OPTION_STATUS;
}

/**
 * enclosures_maint -
 * @i_con:		i_container struct
 *
 * Returns:
 *   0 if success / non-zero on failure FIXME
 **/
int enclosures_maint(i_container *i_con)
{
	int rc, k, first_ses;
	struct ipr_ioa *ioa;
	struct ipr_dev *ses;
	char *buffer[2];
	struct screen_output *s_out;
	int header_lines;
	int toggle=0;
	char product_id[IPR_PROD_ID_LEN + 1];

	processing();
	i_con = free_i_con(i_con);
	body_init_status_enclosure(buffer, n_enclosures_maint.header, &header_lines);

	check_current_config(false);

	for_each_ioa(ioa) {
		if (!ioa->sis64) 
			continue;
		first_ses = 1;
		for_each_ses(ioa, ses) {
			ipr_strncpy_0(product_id, ses->scsi_dev_data->product_id, IPR_PROD_ID_LEN);
			if (!strncmp(product_id,"5888", strlen("5888")) || !strncmp(product_id, "EDR1", strlen("EDR1"))) {
				if (strlen(ses->physical_location) == 0)
					get_ses_phy_loc(ses);
				if (first_ses) {
					i_con = add_i_con(i_con, "\0",ioa);
					print_dev_enclosure(k, &ioa->ioa, buffer, "%1", (k | 0x1000));
					first_ses = 0;
				}
				i_con = add_i_con(i_con, "\0",ses);
				print_dev_enclosure(k, ses, buffer, "%1", k);
			}
		}
	}

	do {
		n_enclosures_maint.body = buffer[toggle&1];
		s_out = screen_driver(&n_enclosures_maint, header_lines, i_con);
		rc = s_out->rc;
		free(s_out);
		toggle++;
	} while (rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_enclosures_maint.body = NULL;

	return rc;
}

/**
 * suspend_disk_enclosure -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int suspend_disk_enclosure(char **args, int num_args)
{
	struct ipr_dev *ses;
	int rc = 0;

	ses = find_dev(args[0]);
	get_ses_ioport_status(ses);

	if (ipr_is_ioa(ses))  {
		fprintf(stderr,"Incorrect device type specified. Please specify a valid SAS device to suspend.\n");
		return 1;
	}

	if (ses->scsi_dev_data && ses->scsi_dev_data->type != TYPE_ENCLOSURE) {
		fprintf(stderr, "Selected device is not SAS expander.\n");
		return 1;
	}

	if (strlen(ses->physical_location) == 0)
		get_ses_phy_loc(ses);

	if (!strncmp((char *)&ses->serial_number, (char *)&ses->ioa->yl_serial_num[0], ESM_SERIAL_NUM_LEN)) {
		fprintf(stderr, "Could not suspend an expander from the RAID controller with the same serial number.\n");
		return 1;
	}

	if (ses->active_suspend == IOA_DEV_PORT_SUSPEND) {
		printf("The expander is already suspended\n");
		return 1;
	}

	if (ses->active_suspend == IOA_DEV_PORT_UNKNOWN)	
		return 90;

	rc = ipr_suspend_device_bus(ses, &ses->res_addr[0],
			    IPR_SDB_CHECK_AND_QUIESCE_ENC);
	if (!rc) {
		rc = ipr_write_dev_attr(ses, "state", "offline\n");
		printf("Selected disk enclosure was suspended successfully.\n");
		return  rc;
	}
	fprintf(stderr, "Failed to suspend this SAS expander.\n");
	return 1;
}

/**
 * resume_disk_enclosure -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int resume_disk_enclosure(char **args, int num_args)
{
	struct ipr_dev *ses;
	int rc = 0;

	ses = find_dev(args[0]);
	get_ses_ioport_status(ses);

	if (ipr_is_ioa(ses)) {
		fprintf(stderr,"Incorrect device type specified. Please specify a valid SAS device to resume.\n");
		return 1;
	}

	if (ses->scsi_dev_data && ses->scsi_dev_data->type != TYPE_ENCLOSURE) {
		fprintf(stderr, "Selected device is not SAS expander.\n");
		return 1;
	}

	if (ses->active_suspend == IOA_DEV_PORT_ACTIVE)	 {
		printf("The expander is already in active state.\n");
		return 1;
	}

	if (ses->active_suspend == IOA_DEV_PORT_UNKNOWN)	 {
		printf("The expander is an unknown state.\n");
		return 1;
	}

	rc = ipr_resume_device_bus(ses, &ses->res_addr[0]);
	if (!rc) {
		ipr_write_dev_attr(ses, "state", "running\n");
		printf("Selected disk enclosure was resumed successfully.\n");
		return  rc;
	}

	printf("Failed to resume this SAS expander.\n");
	return 1;
}

/**
 * query_disk_enclosure_status -
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int query_disk_enclosure_status(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *ses;
	int rc = 0;

	printf("%s\n%s\n", status_hdr[14], status_sep[14]);

	for_each_ioa(ioa) {
		printf_device((struct ipr_dev *)ioa, 0x8 | 0x1000);
		for_each_ses(ioa, ses) {
			get_ses_ioport_status(ses);
			if (strlen(ses->physical_location) == 0)
				get_ses_phy_loc(ses);
			printf_device(ses, 0x8);
		}
	}
	return rc;
}

static char *af_dasd_perf (char *body, struct ipr_dev *dev)
{
	struct ipr_dasd_perf_counters_log_page30 log;
	char buffer[200];
	unsigned long long idle_loop_count;
	u32 idle_count;

	memset(&log, 0, sizeof(log));

	body = add_line_to_body(body,"", NULL);
	if (!ipr_is_log_page_supported(dev, 0x30)) {
		body = add_line_to_body(body,_("Log page 30 not supported"), NULL);
		return body;
	}

	ipr_log_sense(dev, 0x30, &log, sizeof(log));

	sprintf(buffer,"%d", ntohs(log.dev_no_seeks));
	body = add_line_to_body(body,_("Seek = 0 disk"), buffer);
	sprintf(buffer,"%d", ntohs(log.dev_seeks_2_3));
	body = add_line_to_body(body,_("Seek >= 2/3 disk"), buffer);
	sprintf(buffer,"%d", ntohs(log.dev_seeks_1_3));
	body = add_line_to_body(body,_("Seek >= 1/3 and < 2/3 disk"), buffer);
	sprintf(buffer,"%d", ntohs(log.dev_seeks_1_6));
	body = add_line_to_body(body,_("Seek >= 1/6 and < 1/3 disk"), buffer);
	sprintf(buffer,"%d", ntohs(log.dev_seeks_1_12));
	body = add_line_to_body(body,_("Seek >= 1/12 and < 1/6 disk"), buffer);
	sprintf(buffer,"%d", ntohs(log.dev_seeks_0));
	body = add_line_to_body(body,_("Seek > 0 and < 1/12 disk"), buffer);

	body = add_line_to_body(body,"", NULL);

	sprintf(buffer,"%d", ntohs(log.dev_read_buf_overruns));
	body = add_line_to_body(body,_("Device Read Buffer Overruns"), buffer);
	sprintf(buffer,"%d", ntohs(log.dev_write_buf_underruns));
	body = add_line_to_body(body,_("Device Write Buffer Underruns"), buffer);
	sprintf(buffer,"%d", ntohl(log.dev_cache_read_hits));
	body = add_line_to_body(body,_("Device Cache Read Hits"), buffer);
	sprintf(buffer,"%d", ntohl(log.dev_cache_partial_read_hits));
	body = add_line_to_body(body,_("Device Partial Read Hits"), buffer);
	sprintf(buffer,"%d", ntohl(log.dev_cache_write_hits));
	body = add_line_to_body(body,_("Device Cache Write Hits"), buffer);
	sprintf(buffer,"%d", ntohl(log.dev_cache_fast_write_hits));
	body = add_line_to_body(body,_("Device Cache Fast Writes"), buffer);

	body = add_line_to_body(body,"", NULL);

	sprintf(buffer,"%d", ntohl(log.ioa_dev_read_ops));
	body = add_line_to_body(body,_("IOA Issued Device Reads"), buffer);
	sprintf(buffer,"%d", ntohl(log.ioa_dev_write_ops));
	body = add_line_to_body(body,_("IOA Issued Device Writes"), buffer);
	sprintf(buffer,"%d", ntohl(log.ioa_cache_read_hits));
	body = add_line_to_body(body,_("IOA Cache Read Hits"), buffer);
	sprintf(buffer,"%d", ntohl(log.ioa_cache_partial_read_hits));
	body = add_line_to_body(body,_("IOA Cache Partial Read Hits"), buffer);
	sprintf(buffer,"%d", ntohl(log.ioa_cache_write_hits));
	body = add_line_to_body(body,_("IOA Cache Write Hits"), buffer);
	sprintf(buffer,"%d", ntohl(log.ioa_cache_fast_write_hits));
	body = add_line_to_body(body,_("IOA Cache Fast Writes"), buffer);
	sprintf(buffer,"%d", ntohl(log.ioa_cache_emu_read_hits));
	body = add_line_to_body(body,_("IOA Emulated Read Hits"), buffer);

	body = add_line_to_body(body,"", NULL);
	sprintf(buffer,"%08X%08X", ntohl(log.ioa_idle_loop_count[0]),
			ntohl(log.ioa_idle_loop_count[1]));
	idle_loop_count = strtoull(buffer, NULL, 16);
	sprintf(buffer,"%llu", idle_loop_count);
	body = add_line_to_body(body,_("IOA Idle Loop Count"), buffer);

	idle_count = ntohl(log.ioa_idle_count_value);
	sprintf(buffer,"%d", idle_count);
	body = add_line_to_body(body,_("IOA Idle Count"), buffer);

	sprintf(buffer,"%d", log.ioa_idle_units);
	body = add_line_to_body(body,_("IOA Idle Units"), buffer);

	idle_count = idle_loop_count * ( idle_count * pow(10, -log.ioa_idle_units));

	sprintf(buffer,"%d seconds.", idle_count);
	body = add_line_to_body(body,_("IOA Idle Time"), buffer);

	return body;
}


static int show_perf (char **args, int num_args)
{
	char *body = NULL;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (ipr_is_af_dasd_device(dev)) {
		body = af_dasd_perf(body, dev);
	} else {
		fprintf(stderr, "%s is not a valid DASD\n", args[0]);
		return -EINVAL;
	}
	printf("%s\n", body);
	free(body);
	return 0;
}

static int dump (char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct ipr_disk_attr attr;

	if (!ipr_ioa_head) {
		printf("There are no IPR adapters in the system. No dump collected...\n");
		return 0;
	}

	printf("\n === Running 'show-config' ===\n");
	show_config(NULL,0);

	printf("\n === Running 'show-alt-config' ===\n");
	show_alt_config(NULL,0);

	printf("\n === Running 'show-ioas' ===\n");
	show_ioas(NULL, 0);

	printf("\n === Running 'show-arrays' ===\n");
	show_arrays(NULL,0);

	printf("\n === Running 'show-hot-spares' ===\n");
	show_hot_spares(NULL, 0);

	printf("\n === Running 'show-af-disks' ===\n");
	show_af_disks(NULL, 0);

	printf("\n === Running 'show-all-af-disks' ===\n");
	show_all_af_disks(NULL, 0);

	printf("\n === Running 'show-jbod-disks' ===\n");
	show_jbod_disks(NULL, 0);

	printf("\n === Running 'show-slots' ===\n");
	show_slots(NULL, 0);

	printf("\n === Running show-details ===\n");
	for_each_ioa(ioa){
		printf("IOA %s:\n", ioa->ioa.gen_name);
		show_dev_details(&ioa->ioa);

		for_each_dev(ioa, dev){
			printf("Device %s:\n", dev->gen_name);
			show_dev_details(dev);

			if (ipr_is_volume_set(dev) || ipr_is_gscsi(dev)) {
				if (ipr_get_dev_attr(dev, &attr))
					continue;
				printf("Write cache mode: %s\n\n",
				       attr.write_cache_policy ?
				       "write back" : "write through");
			}
		}
	}

	printf("\n === Running show-battery-info ===\n");
	for_each_ioa(ioa){
		char *array[] = { ioa->ioa.gen_name };
		printf("Device %s:\n", ioa->ioa.gen_name);
		battery_info(array, 1);
	}

	printf("\n === Running 'query-path-status' ===\n");
	for_each_primary_ioa(ioa){
		for_each_dev(ioa, dev){
			char *array[] = { dev->gen_name };
			printf("\nDevice %s:\n", dev->gen_name);
			query_path_status(array, 1);
		}
	}

	printf("\n\n === Running 'query-path-details' ===\n");
	for_each_primary_ioa(ioa){
		for_each_dev(ioa, dev){
			char *array[] = { dev->gen_name };
			printf("\nDevice: %s\n", dev->gen_name);
			query_path_details(array, 1);
		}
	}

	printf("\n === Running 'show-perf' ===\n");
	for_each_ioa(ioa){
		for_each_dev(ioa, dev){
			if (ipr_is_af_dasd_device(dev)) {
				char *array[] = { dev->gen_name };
				printf("\nDevice: %s\n", dev->gen_name);
				show_perf(array, 1);
			}
		}
	}

	return 0;
}

static char *print_ssd_report(struct ipr_dev *dev, char *body)
{
	struct ipr_inquiry_page0 page0_inq;
	struct ipr_dasd_inquiry_page3 page3_inq;
	struct ipr_sas_std_inq_data std_inq;
	struct ipr_sas_inquiry_pageC4 pageC4_inq;
	struct ipr_sas_inquiry_pageC7 pageC7_inq;
	struct ipr_sas_log_page page34_log;
	struct ipr_sas_log_page page2F_log;
	struct ipr_sas_log_page page02_log;

	struct ipr_sas_log_smart_attr *smart_attr;
	struct ipr_sas_log_inf_except_attr *inf_except_attr;
	struct ipr_sas_log_write_err_cnt_attr *bytes_counter;

	char buffer[BUFSIZ];
	int rc, len, nentries = 0;
	u64 aux;
	u64 total_gb_writ = 0;
	u32 uptime = 0;
	u32 life_remain = 0;
	int pfa_trip = 0;

	memset(&std_inq, 0, sizeof(std_inq));
	memset(&pageC4_inq, 0, sizeof(pageC4_inq));
	memset(&pageC7_inq, 0, sizeof(pageC7_inq));
	memset(&page0_inq, 0, sizeof(page0_inq));
	memset(&page3_inq, 0, sizeof(page3_inq));
	memset(&page34_log, 0, sizeof(page34_log));
	memset(&page2F_log, 0, sizeof(page2F_log));
	memset(&page02_log, 0, sizeof(page02_log));

	rc = ipr_inquiry(dev, IPR_STD_INQUIRY, &std_inq, sizeof(std_inq));
	if (rc)
		return NULL;

	if (!std_inq.is_ssd) {
		scsi_err(dev, "%s is not a SSD.\n", dev->gen_name);
		return NULL;
	}

	rc = ipr_inquiry(dev, 0xC4, &pageC4_inq, sizeof(pageC4_inq));
	if (rc)
		return NULL;

	if (pageC4_inq.endurance != IPR_SAS_ENDURANCE_LOW_EZ &&
	    pageC4_inq.endurance != IPR_SAS_ENDURANCE_LOW3 &&
	    pageC4_inq.endurance != IPR_SAS_ENDURANCE_LOW1) {
		scsi_err(dev, "%s is not a Read Intensive SSD.\n",
			 dev->gen_name);
		return NULL;
	}

	rc = ipr_inquiry(dev, 0x3, &page3_inq, sizeof(page3_inq));
	if (rc)
		return NULL;

	rc = ipr_inquiry(dev, 0xC7, &pageC7_inq, sizeof(pageC7_inq));
	if (rc)
		return NULL;

	rc = ipr_log_sense(dev, 0x34, &page34_log, sizeof(page34_log));
	if (rc)
		return NULL;

	smart_attr = ipr_sas_log_get_param(&page34_log, 0xE7, NULL);
	if (smart_attr && smart_attr->norm_worst_val > 0)
		life_remain = smart_attr->norm_worst_val;

	smart_attr = ipr_sas_log_get_param(&page34_log, 0x09, NULL);
	if (smart_attr)
		uptime = ntohl(*((u32 *) smart_attr->raw_data));

	rc = ipr_log_sense(dev, 0x02, &page02_log, sizeof(page02_log));
	if (rc)
		return NULL;

	bytes_counter = ipr_sas_log_get_param(&page02_log, 0x05, NULL);
	if (bytes_counter)
		total_gb_writ = be64toh(*(u64 *) &bytes_counter->counter) >> 14;

	rc = ipr_log_sense(dev, 0x2F, &page2F_log, sizeof(page2F_log));
	if (rc)
		return NULL;

	inf_except_attr = ipr_sas_log_get_param(&page2F_log, 0x0, &nentries);
	if (inf_except_attr)
		pfa_trip = inf_except_attr->inf_except_add_sense_code;
	if (nentries > 1)
		pfa_trip = 1;

	body = add_line_to_body(body, "", NULL);
	/* FRU Number */
	ipr_strncpy_0(buffer, std_inq.asm_part_num,
		      IPR_SAS_STD_INQ_ASM_PART_NUM);
	body = add_line_to_body(body, _("FRU Number"), buffer);

	/* Serial number */
	ipr_strncpy_0(buffer, (char *)std_inq.std_inq_data.serial_num,
		      IPR_SERIAL_NUM_LEN);
	body = add_line_to_body(body, _("Serial Number"), buffer);

	/* FW level */
	len = sprintf(buffer, "%X%X%X%X", page3_inq.release_level[0],
		      page3_inq.release_level[1], page3_inq.release_level[2],
		      page3_inq.release_level[3]);

	if (isalnum(page3_inq.release_level[0]) &&
	    isalnum(page3_inq.release_level[1]) &&
	    isalnum(page3_inq.release_level[2]) &&
	    isalnum(page3_inq.release_level[3]))
		sprintf(buffer + len, " (%c%c%c%c)", page3_inq.release_level[0],
			page3_inq.release_level[1], page3_inq.release_level[2],
			page3_inq.release_level[3]);

	body = add_line_to_body(body, _("Firmware Version"), buffer);

	/* Bytes written */
	snprintf(buffer, BUFSIZ, "%ld GB", total_gb_writ);
	body = add_line_to_body(body, _("Total Bytes Written"), buffer);

	/* Max bytes. */
	aux = ntohl(*((u32 *) pageC7_inq.total_bytes_warranty)) >> 8;
	snprintf(buffer, BUFSIZ, "%ld GB", aux * 1024L);
	body = add_line_to_body(body, _("Number of written bytes supported"),
				buffer);

	/* Life remaining */
	snprintf(buffer, BUFSIZ, "%hhd %%", life_remain);
	body = add_line_to_body(body, _("Life Remaining Gauge"), buffer);

	/* PFA Trip */
	body = add_line_to_body(body, _("PFA Trip"), pfa_trip? "yes": "no");

	/* Power-on days */
	snprintf (buffer, 4, "%d", uptime / 24);
	body = add_line_to_body(body, _("Power-on Days"), buffer);

	return body;
}

static int ssd_report(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	char *body;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	body = print_ssd_report(dev, NULL);
	if (!body) {
		scsi_err(dev, "Device inquiry failed.\n");
		return -EIO;
	}

	printf("%s", body);

	return 0;
}

static const struct {
	char *cmd;
	int min_args;
	int unlimited_max;
	int max_args;
	int (*func)(char **, int);
	char *usage;
} command [] = {
	{ "show-config",			0, 0, 0, show_config, "" },
	{ "show-alt-config",			0, 0, 0, show_alt_config, "" },
	{ "show-ioas",				0, 0, 0, show_ioas, "" },
	{ "show-arrays",			0, 0, 0, show_arrays, "" },
	{ "show-battery-info",			1, 0, 1, battery_info, "sg5" },
	{ "show-details",			1, 0, 1, show_details, "sda" },
	{ "show-hot-spares",			0, 0, 0, show_hot_spares, "" },
	{ "show-af-disks",			0, 0, 0, show_af_disks, "" },
	{ "show-all-af-disks",			0, 0, 0, show_all_af_disks, "" },
	{ "show-jbod-disks",			0, 0, 0, show_jbod_disks, "" },
	{ "show-slots",				0, 0, 0, show_slots, "" },
	{ "status",				1, 0, 1, print_status, "sda" },
	{ "alt-status",				1, 0, 1, print_alt_status, "sda" },
	{ "query-raid-create",			1, 0, 1, query_raid_create, "sg5" },
	{ "query-raid-delete",			1, 0, 1, query_raid_delete, "sg5" },
	{ "query-hot-spare-create",		1, 0, 1, query_hot_spare_create, "sg5" },
	{ "query-hot-spare-delete",		1, 0, 1, query_hot_spare_delete, "sg5" },
	{ "query-raid-consistency-check",	0, 0, 0, query_raid_consistency_check, "" },
	{ "query-format-for-jbod",		0, 0, 0, query_format_for_jbod, "" },
	{ "query-reclaim",			0, 0, 0, query_reclaim, "" },
	{ "query-arrays-raid-include",		0, 0, 0, query_arrays_include, "" },
	{ "query-devices-raid-include",		1, 0, 1, query_devices_include, "sdb" },
	{ "query-arrays-raid-migrate",		0, 0, 0, query_arrays_raid_migrate, "" },
	{ "query-devices-raid-migrate",		1, 0, 1, query_devices_raid_migrate, "sda" },
	{ "query-raid-levels-raid-migrate",	1, 0, 1, query_raid_levels_raid_migrate, "sda" },
	{ "query-stripe-sizes-raid-migrate",	2, 0, 2, query_stripe_sizes_raid_migrate, "sg5 10" },
	{ "query-devices-min-max-raid-migrate",	2, 0, 2, query_devices_min_max_raid_migrate, "sg5 10" },
	{ "query-ioas-asymmetric-access",	0, 0, 0, query_ioas_asym_access, "" },
	{ "query-arrays-asymmetric-access",	0, 0, 0, query_arrays_asym_access, "" },
	{ "query-ioa-asymmetric-access-mode",	1, 0, 1, query_ioa_asym_access_mode, "sg5" },
	{ "query-array-asymmetric-access-mode",	1, 0, 1, query_array_asym_access_mode, "sda" },
	{ "query-recovery-format",		0, 0, 0, query_recovery_format, "" },
	{ "query-raid-rebuild",			0, 0, 0, query_raid_rebuild, "" },
	{ "query-format-for-raid",		0, 0, 0, query_format_for_raid, "" },
	{ "query-ucode-level",			1, 0, 1, query_ucode_level, "sda" },
	{ "query-format-timeout",		1, 0, 1, query_format_timeout, "sg6" },
	{ "query-qdepth",			1, 0, 1, query_qdepth, "sda" },
	{ "query-tcq-enable",			1, 0, 1, query_tcq_enable, "sda" },
	{ "query-log-level",			1, 0, 1, query_log_level, "sg5" },
	{ "query-add-device",			0, 0, 0, query_add_device, "" },
	{ "query-remove-device",		0, 0, 0, query_remove_device, "" },
	{ "query-initiator-id",			2, 0, 2, query_initiator_id, "sg5 0" },
	{ "query-bus-speed",			2, 0, 2, query_bus_speed, "sg5 0" },
	{ "query-bus-width",			2, 0, 2, query_bus_width, "sg5 0" },
	{ "query-supported-raid-levels",	1, 0, 1, query_raid_levels, "sg5" },
	{ "query-include-allowed",		2, 0, 2, query_include_allowed, "sg5 5" },
	{ "query-max-devices-in-array",		2, 0, 2, query_max_array_devices, "sg5 5" },
	{ "query-min-devices-in-array",		2, 0, 2, query_min_array_devices, "sg5 5" },
	{ "query-min-mult-in-array",		2, 0, 2, query_min_mult_in_array, "sg5 5" },
	{ "query-supp-stripe-sizes",		2, 0, 2, query_supp_stripe_sizes, "sg5 5" },
	{ "query-recommended-stripe-size",	2, 0, 2, query_recommended_stripe_size, "sg5 5" },
	{ "query-path-status",			0, 0, 1, query_path_status, "sg5" },
	{ "query-path-details",			1, 0, 1, query_path_details, "sda" },
	{ "query-ioa-caching",			1, 0, 1, query_ioa_caching, "sg5" },
	{ "query-array-label",			1, 0, 1, query_array_label, "mylabel" },
	{ "query-array",				1, 0, 1, query_array, "U5886.001.P915059-P1-D1" },
	{ "query-device",				1, 0, 1, query_device, "U5886.001.P915059-P1-D1" },
	{ "query-location",			1, 0, 1, query_location, "sg5" },
	{ "query-array-rebuild-rate",		1, 0, 1, query_array_rebuild_rate, "sg5"},
	{ "query-array-rebuild-verify",		1, 0, 1, query_array_rebuild_verify, "sg5" },
	{ "set-ioa-caching",			2, 0, 2, set_ioa_caching, "sg5 [Default | Disabled]" },
	{ "set-array-rebuild-rate",		2, 0, 2, set_array_rebuild_rate, "sg5 10"},
	{ "set-array-rebuild-verify",           2, 0, 2, set_array_rebuild_verify, "sg5 [default | enable | disable]" },
	{ "primary",				1, 0, 1, set_primary, "sg5" },
	{ "secondary",				1, 0, 1, set_secondary, "sg5" },
	{ "set-all-primary",			0, 0, 0, set_all_primary, "" },
	{ "set-all-secondary",			0, 0, 0, set_all_secondary, "" },
	{ "query-ha-mode",			1, 0, 1, get_ha_mode, "sg5" },
	{ "set-ha-mode",			2, 0, 2, __set_ha_mode, "sg5 [Normal | JBOD]" },
	{ "set-ioa-asymmetric-access-mode",	2, 0, 2, set_ioa_asymmetric_access, "sg5 [Enabled | Disabled]" },
	{ "set-array-asymmetric-access-mode",	2, 0, 2, set_array_asymmetric_access, "sda [Optimized | Non-Optimized]" },
	{ "raid-create",			1, 1, 0, raid_create, "-r 5 -s 64 sda sdb sg6 sg7" },
	{ "raid-delete",			1, 0, 1, raid_delete, "sdb" },
	{ "raid-include",			2, 0, 17, raid_include_cmd, "sda sg6 sg7" },
	{ "format-for-raid",			1, 1, 0, format_for_raid, "sda sdb sdc" },
	{ "format-for-jbod",			1, 1, 0, format_for_jbod, "sg6 sg7 sg8" },
	{ "recovery-format",			1, 1, 0, recovery_format, "sda sg7" },
	{ "hot-spare-create",			1, 0, 1, hot_spare_create, "sg6" },
	{ "hot-spare-delete",			1, 0, 1, hot_spare_delete, "sg6" },
	{ "reclaim-cache",			1, 0, 1, reclaim, "sg5" },
	{ "reclaim-unknown-cache",		1, 0, 1, reclaim_unknown, "sg5" },
	{ "force-cache-battery-error",		1, 0, 1, force_cache_battery_error, "sg5" },
	{ "start-ioa-cache",			1, 0, 1, start_ioa_cache, "sg5" },
	{ "raid-consistency-check",		1, 0, 1, raid_consistency_check, "sg5" },
	{ "raid-rebuild",			1, 0, 1, raid_rebuild_cmd, "sg6" },
	{ "disrupt-device",			1, 0, 1, disrupt_device_cmd, "sg6" },
	{ "update-ucode",			2, 0, 2, update_ucode_cmd, "sg5 /root/ucode.bin" },
	{ "show-ucode-levels",			0, 0, 0, show_ucode_levels, "" },
	{ "update-all-ucodes",			0, 0, 0, update_all_ucodes, "" },
	{ "set-format-timeout",			2, 0, 2, set_format_timeout, "sg6 4" },
	{ "set-qdepth",				2, 0, 2, set_qdepth, "sda 16" },
	{ "set-tcq-enable",			2, 0, 2, set_tcq_enable, "sda 0" },
	{ "set-log-level",			2, 0, 2, set_log_level_cmd, "sg5 2" },
	{ "set-write-cache-policy",		2, 0, 2, set_device_write_cache_policy, "sg5 [writeback|writethrough]" },
	{ "query-write-cache-policy",		1, 0, 1, query_device_write_cache_policy, "sg5" },
	{ "ssd-report",				1, 0, 1, ssd_report, "sg5" },
	{ "identify-disk",			2, 0, 2, identify_disk, "sda 1" },
	{ "identify-slot",			2, 0, 2, identify_slot, "U5886.001.P915059-P1-D1 1" },
	{ "remove-disk",			2, 0, 2, remove_disk, "sda 1" },
	{ "remove-slot",			2, 0, 2, remove_slot, "U5886.001.P915059-P1-D1 1" },
	{ "add-slot",				2, 0, 2, add_slot, "U58886.001.P915059-P1-D1 1" },
	{ "set-initiator-id",			3, 0, 3, set_initiator_id, "sg5 0 7" },
	{ "set-bus-speed",			3, 0, 3, set_bus_speed, "sg5 0 320" },
	{ "set-bus-width",			3, 0, 3, set_bus_width, "sg5 0 16" },
	{ "raid-migrate",			3, 1, 0, raid_migrate_cmd, "-r 10 [-s 256] sda sg6 sg7" },
	{ "get-live-dump",			1, 0, 1, get_live_dump, "sg5" },
	{ "query-ses-mode",                     1, 0, 1, query_ses_mode, "sg8"},
	{ "set-ses-mode",                       2, 0, 2, set_ses_mode, "sg8 2"},
	{ "query-disk-enclosure-status",        0, 0, 0, query_disk_enclosure_status},
	{ "suspend-disk-enclosure",             1, 0, 1, suspend_disk_enclosure, "sg8"},
	{ "resume-disk-enclosure",              1, 0, 1, resume_disk_enclosure, "sg8 "},
	{ "show-perf",                          1, 0, 1, show_perf, "sg8"},
	{ "dump",				0, 0, 0, dump, "" },
};

/**
 * non_intenactive_cmd - process a command line command
 * @cmd:		command string
 * @args:		argument vector
 * @num_args:		number of arguments
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int non_intenactive_cmd(char *cmd, char **args, int num_args)
{
	int rc, i;

	exit_func = cmdline_exit_func;
	closelog();
	openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
	check_current_config(false);

	for (i = 0; i < ARRAY_SIZE(command); i++) {
		if (strcmp(cmd, command[i].cmd) != 0)
			continue;

		if (num_args < command[i].min_args) {
			fprintf(stderr, "Not enough arguments specified.\n");
			fprintf(stderr, "Usage: iprconfig -c %s %s\n",
				cmd, command[i].usage);
			return -EINVAL;
		}

		if (!command[i].unlimited_max && num_args > command[i].max_args) {
			fprintf(stderr, "Too many arguments specified.\n");
			fprintf(stderr, "Usage: iprconfig -c %s %s\n",
				cmd, command[i].usage);
			return -EINVAL;
		}

		rc = command[i].func(args, num_args);
		exit_func();
		return rc;
	}

	exit_func();
	usage();
	return -EINVAL;
}

void list_options()
{
	int i;
	for (i = 0; i < ARRAY_SIZE(command); i++)
		printf ("%s\n", command[i].cmd);
}

/**
 * main - program entry point
 * @argc:		number of arguments
 * @argv:		argument vector
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int main(int argc, char *argv[])
{
	int  next_editor, next_dir, next_cmd, next_args, i, rc = 0;
	char parm_editor[200], parm_dir[200], cmd[200];
	int non_intenactive = 0;

	tool_init_retry = 0;
	strcpy(parm_dir, DEFAULT_LOG_DIR);
	strcpy(parm_editor, DEFAULT_EDITOR);

	openlog("iprconfig",
		LOG_PID |     /* Include the PID with each error */
		LOG_CONS,     /* Write to system console if there is an error
			       sending to system logger */
		LOG_USER);

	if (argc > 1) {
		next_editor = 0;
		next_dir = 0;
		next_cmd = 0;
		next_args = 0;
		for (i = 1; i < argc; i++) {
			if (parse_option(argv[i]))
				continue;
			else if (next_editor)	{
				strcpy(parm_editor, argv[i]);
				next_editor = 0;
			} else if (next_dir) {
				strcpy(parm_dir, argv[i]);
				next_dir = 0;
			} else if (next_cmd) {
				strcpy(cmd, argv[i]);
				non_intenactive = 1;
				next_cmd = 0;
				next_args = 1;
			} else if (next_args) {
				add_args = realloc(add_args, sizeof(*add_args) * (num_add_args + 1));
				add_args[num_add_args] = malloc(strlen(argv[i]) + 1);
				strcpy(add_args[num_add_args], argv[i]);
				num_add_args++;
			} else if (strcmp(argv[i], "-e") == 0) {
				next_editor = 1;
			} else if (strcmp(argv[i], "-k") == 0) {
				next_dir = 1;
			} else if (strcmp(argv[i], "-c") == 0) {
				next_cmd = 1;
			} else if (strcmp(argv[i], "-l") == 0) {
				list_options();
				exit(1);
			}
			else {
				usage();
				exit(1);
			}
		}
	}

	strcpy(log_root_dir, parm_dir);
	strcpy(editor, parm_editor);

	if (check_sg_module() == -1)
		system("modprobe sg");
	exit_func = tool_exit_func;
	tool_init(0);

	if (non_intenactive)
		return non_intenactive_cmd(cmd, add_args, num_add_args);

	use_curses = 1;
	curses_init();
	cbreak(); /* take input chars one at a time, no wait for \n */

	keypad(stdscr,TRUE);
	noecho();

	s_status.index = 0;

	main_menu(NULL);
	if (head_zdev){
		check_current_config(false);
		ipr_cleanup_zeroed_devs();
	}

	while (head_zdev) {
		struct screen_output *s_out;
		i_container *i_con;
		struct ipr_ioa *ioa;
		struct ipr_dev *dev;
		int num_zeroed = 0;

		for_each_ioa(ioa) {
			for_each_af_dasd(ioa, dev) {
				if (ipr_device_is_zeroed(dev) &&
				    !ipr_known_zeroed_is_saved(dev)) {
					num_zeroed++;
				}
			}
		}

		if (!num_zeroed)
			break;

 		n_exit_confirm.body = body_init(n_exit_confirm.header, NULL);

		for (i = 0; i < n_exit_confirm.num_opts; i++) {
			n_exit_confirm.body = ipr_list_opts(n_exit_confirm.body,
							    n_exit_confirm.options[i].key,
							    n_exit_confirm.options[i].list_str);
		}

		n_exit_confirm.body = ipr_end_list(n_exit_confirm.body);

		s_out = screen_driver(&n_exit_confirm, 0, NULL);
		free(n_exit_confirm.body);
		n_exit_confirm.body = NULL;
		rc = s_out->rc;
		i_con = s_out->i_con;
		i_con = free_i_con(i_con);
		free(s_out);
		check_current_config(false);
		ipr_cleanup_zeroed_devs();
	}

	clear();
	refresh();
	endwin();
	exit(0);
	return 0;
}

