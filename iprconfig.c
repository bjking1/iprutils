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

char *tool_name = "iprconfig";

struct devs_to_init_t {
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;
	int new_block_size;
#define IPR_JBOD_BLOCK_SIZE 512
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
#define DEFAULT_EDITOR "vi -R"
#define FAILSAFE_EDITOR "vi -R -"

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
char *__print_device(struct ipr_dev *, char *, char *, int, int, int, int, int);

#define print_dev(i, dev, buf, fmt, type) \
        for (i = 0; i < 2; i++) \
            (buf)[i] = print_device(dev, (buf)[i], fmt, type);

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
			if ((status_record->status != IPR_CMD_STATUS_SUCCESSFUL) &&
			    (status_record->status != IPR_CMD_STATUS_FAILED))
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

static i_container *add_i_con(i_container *i_con, char *f, void *d)
{  
	i_container *new_i_con;
	new_i_con = malloc(sizeof(i_container));

	new_i_con->next_item = NULL;

	/* used to hold data entered into user-entry fields */
	strncpy(new_i_con->field_data, f, MAX_FIELD_SIZE+1); 
	new_i_con->field_data[strlen(f)+1] = '\0';

	/* a pointer to the device information represented by the i_con */
	new_i_con->data = d;

	if (i_con)
		i_con->next_item = new_i_con;
	else
		i_con_head = new_i_con;

	return new_i_con;
}

int exit_confirmed(i_container *i_con)
{
	exit_func();
	exit(0);
	return 0;
}

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

static void free_devs_to_init()
{
	struct devs_to_init_t *dev = dev_init_head;

	while (dev) {
		dev = dev->next;
		free(dev_init_head);
		dev_init_head = dev;
	}
}

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

static void tool_exit_func()
{
	if (use_curses) {
		clearenv();
		clear();
		refresh();
		endwin();
	}
}

static void cmdline_exit_func()
{
}

static char *ipr_list_opts(char *body, char *key, char *list_str)
{
	char *string_buf = _("Select one of the following");
	int   start_len = 0;

	if (!body) {
		body = realloc(body, strlen(string_buf) + 8);
		sprintf(body, "\n%s:\n\n", string_buf);
	}

	start_len = strlen(body);

	body = realloc(body, start_len + strlen(key) + strlen(_(list_str)) + 16);
	sprintf(body + start_len, "    %s. %s\n", key, _(list_str));
	return body;
}

static char *ipr_end_list(char *body)
{
	int start_len = 0;
	char *string_buf = _("Selection");

	start_len = strlen(body);

	body = realloc(body, start_len + strlen(string_buf) + 16);
	sprintf(body + start_len, "\n\n%s: %%1", string_buf);
	return body;
}

static struct screen_output *screen_driver(s_node *screen, int header_lines, i_container *i_con)
{
	WINDOW *w_pad = NULL,*w_page_header = NULL; /* windows to hold text */
	FIELD **fields = NULL;                     /* field list for forms */
	FORM *form = NULL;                       /* form for input fields */
	int rc = 0;                            /* the status of the screen */
	char *status;                            /* displays screen operations */
	char buffer[100];                       /* needed for special status strings */
	bool invalid = false;                   /* invalid status display */

	bool ground_cursor=false;

	int stdscr_max_y,stdscr_max_x;
	int w_pad_max_y=0,w_pad_max_x=0;       /* limits of the windows */
	int pad_l=0,pad_scr_r,pad_t=0,viewable_body_lines;
	int title_lines=2,footer_lines=2; /* position of the pad */
	int center,len=0,ch;
	int i=0,j,k,row=0,col=0,bt_len=0;      /* text positioning */
	int field_width,num_fields=0;          /* form positioning */
	int h,w,t,l,o,b;                       /* form querying */

	int active_field = 0;                  /* return to same active field after a quit */
	bool pages = false,is_bottom = false;   /* control page up/down in multi-page screens */
	bool form_adjust = false;               /* correct cursor position in multi-page screens */
	bool refresh_stdscr = true;
	bool x_offscr,y_offscr;                 /* scrolling windows */
	char *input = NULL;
	struct screen_output *s_out;
	struct screen_opts   *temp;             /* reference to another screen */

	char *title,*body,*body_text;           /* screen text */
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

	/* determine max size needed and find where to put form fields and device input in the text and add them
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

			if ((f_flags & FWD_FLAG) && y_offscr) {
				if (!is_bottom)
					mvaddstr(stdscr_max_y-footer_lines,stdscr_max_x-8,_("More..."));
				else
					mvaddstr(stdscr_max_y-footer_lines,stdscr_max_x-8,"       ");

				for (i = 0; i < num_fields; i++) {
					if (fields[i] != NULL) {
						/* height, width, top, left, offscreen rows, buffer */
						field_info(fields[i],&h,&w,&t,&l,&o,&b); 

						/* turn all offscreen fields off */
						if ((t >= (header_lines + pad_t)) &&
						    (t <= (viewable_body_lines + pad_t))) {

							field_opts_on(fields[i],O_ACTIVE);
							if (form_adjust) {
								if (!(f_flags & TOGGLE_FLAG) || !toggle_field)
									set_current_field(form,fields[i]);
								form_adjust = false;
							}
						} else
							field_opts_off(fields[i],O_ACTIVE);
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
			    ((field_opts(fields[toggle_field]) & O_ACTIVE) != O_ACTIVE)) {

				ch = KEY_NPAGE;
			} else {

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

					if ((temp->key == "\n") ||
					    ((num_fields > 0)?(strcasecmp(input,temp->key) == 0):0)) {

						invalid = false;

						if ((temp->key == "\n") && (num_fields > 0)) {

							/* store field data to existing i_con (which should already
							 contain pointers) */
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
		/*0123456789012345678901234567890123456789012345678901234567890123456789901234567890 */
		"OPT Name   PCI/SCSI Location          Vendor   Product ID       Status",
		"OPT Name   PCI/SCSI Location          Description               Status",
		"Name   PCI/SCSI Location          Vendor   Product ID       Status",
		"Name   PCI/SCSI Location          Description               Status"};
static char *status_sep[] = {
		"--- ------ -------------------------- -------- ---------------- -----------------",
		"--- ------ -------------------------- ------------------------- -----------------",
		"------ -------------------------- -------- ---------------- -----------------",
		"------ -------------------------- ------------------------- -----------------"};
static char *status_header(char *buffer, int *num_lines, int type)
{
	int cur_len = strlen(buffer);
	int header_lines = 0;

	if (type > 1)
		type = 0;
	buffer = realloc(buffer, cur_len + strlen(status_hdr[type]) + strlen(status_sep[type]) + 8);
	cur_len += sprintf(buffer + cur_len, "%s\n", status_hdr[type]);
	cur_len += sprintf(buffer + cur_len, "%s\n", status_sep[type]);
	header_lines += 2;

	*num_lines = header_lines + *num_lines;
	return buffer;
}

/* add_string_to_body adds a string of data to fit in the text window, putting
 carraige returns in when necessary
 NOTE:  This routine expect the language conversion to be done before
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

	getmaxyx(stdscr,max_y,max_x);

	if (body)
		body_len = strlen(body);

	len = body_len;
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

/* add_line_to_body adds a single line to the body text of the screen,
 it is intented primarily to list items with the desciptor on the
 left of the screen and ". . . :" to the data field for the descriptor.
 NOTE:  This routine expect the language conversion to be done before
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

static void body_init_status(char *buffer[2], char **header, int *num_lines)
{
	int z;

	for (z = 0; z < 2; z++)
		buffer[z] = __body_init_status(header, num_lines, z);
}

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

static char *body_init(char **header, int *num_lines)
{
	if (num_lines)
		*num_lines = 0;
	return __body_init(NULL, header, num_lines);
}

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

static int __complete_screen_driver(s_node *n_screen, char *complete, int allow_exit)
{
	fprintf(stdout, "\r%s", complete);
	fflush(stdout);
	return 0;
}

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

static void evaluate_device(struct ipr_dev *ipr_dev, struct ipr_ioa *ioa,
			    int new_block_size)
{
	struct scsi_dev_data *scsi_dev_data = ipr_dev->scsi_dev_data;
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
	} else if (ipr_is_af_dasd_device(ipr_dev)) {
		dev_record = ipr_dev->dev_rcd;
		if (dev_record->no_cfgte_dev) 
			return;

		res_handle = ntohl(dev_record->resource_handle);
		res_addr.bus = dev_record->resource_addr.bus;
		res_addr.target = dev_record->resource_addr.target;
		res_addr.lun = dev_record->resource_addr.lun;
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

	if (ipr_is_af(dev)) {
		if ((rc = ipr_query_command_status(dev, &cmd_status)))
			return;

		if (cmd_status.num_records != 0 &&
		    cmd_status.record->status == IPR_CMD_STATUS_SUCCESSFUL) {
			if (ipr_get_blk_size(dev) == 512)
				evaluate_device(dev, dev->ioa, IPR_JBOD_BLOCK_SIZE);
		}
	} else if (dev->scsi_dev_data->type == TYPE_DISK) {
		rc = ipr_test_unit_ready(dev, &sense_data);

		if (rc != CHECK_CONDITION) {
			if (ipr_get_blk_size(dev) != 512 && dev->ioa->qac_data->num_records != 0) {
				enable_af(dev);
				evaluate_device(dev, dev->ioa, dev->ioa->af_block_size);
			}
		}
	}
}

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

int main_menu(i_container *i_con)
{
	int rc;
	int j;
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	struct scsi_dev_data *scsi_dev_data;

	processing();
	check_current_config(false);

	for_each_ioa(ioa) {
		for (j = 0; j < ioa->num_devices; j++) {
			scsi_dev_data = ioa->dev[j].scsi_dev_data;
			if (!scsi_dev_data || scsi_dev_data->type == IPR_TYPE_ADAPTER)
				continue;
			verify_device(&ioa->dev[j]);
		}
	}

	for (j = 0; j < n_main_menu.num_opts; j++) {
		n_main_menu.body = ipr_list_opts(n_main_menu.body,
						 n_main_menu.options[j].key,
						 n_main_menu.options[j].list_str);
	}

	n_main_menu.body = ipr_end_list(n_main_menu.body);

	s_out = screen_driver(&n_main_menu,0,NULL);
	free(n_main_menu.body);
	n_main_menu.body = NULL;
	rc = s_out->rc;
	i_con = s_out->i_con;
	i_con = free_i_con(i_con);
	free(s_out);
	return rc;
}

static int print_standalone_disks(struct ipr_ioa *ioa,
				  i_container **i_con, char **buffer)
{
	struct scsi_dev_data *scsi_dev_data;
	int j, k;
	int num_lines = 0;

	/* print JBOD and non-member AF devices*/
	for (j = 0; j < ioa->num_devices; j++) {
		scsi_dev_data = ioa->dev[j].scsi_dev_data;
		if (!scsi_dev_data ||
		    (scsi_dev_data->type != TYPE_DISK &&
		     scsi_dev_data->type != IPR_TYPE_AF_DISK) ||
		    ipr_is_hot_spare(&ioa->dev[j]) ||
		    ipr_is_volume_set(&ioa->dev[j]) ||
		    ipr_is_array_member(&ioa->dev[j]))
			continue;

		/* check if evaluate_device is needed for this device */
		verify_device(&ioa->dev[j]);
		print_dev (k, &ioa->dev[j], buffer, "%1", 2+k);

		*i_con = add_i_con(*i_con,"\0",&ioa->dev[j]);  
		num_lines++;
	}

	return num_lines;
}

static int print_hotspare_disks(struct ipr_ioa *ioa,
				i_container **i_con, char **buffer)
{
	struct ipr_dev *dev;
	int k;
	int num_lines = 0;

	for_each_hot_spare(ioa, dev) {
		if (!dev->scsi_dev_data)
			continue;

		print_dev(k, dev, buffer, "%1", 2+k);
		*i_con = add_i_con(*i_con, "\0", dev);  
		num_lines++;
	}

	return num_lines;
}

static int print_vsets(struct ipr_ioa *ioa,
		       i_container **i_con, char **buffer)
{
	int k;
	int num_lines = 0;
	struct ipr_dev *vset, *dev;

	/* print volume set and associated devices*/
	for_each_vset(ioa, vset) {
		print_dev(k, vset, buffer, "%1", 2+k);
		*i_con = add_i_con(*i_con, "\0", vset);
		num_lines++;

		for_each_dev_in_vset(vset, dev) {
			print_dev(k, dev, buffer, "%1", 2+k);
			*i_con = add_i_con(*i_con, "\0", dev);
			num_lines++;
		}
	}

	return num_lines;
}

int disk_status(i_container *i_con)
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
	body_init_status(buffer, n_disk_status.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->ioa.scsi_dev_data)
			continue;

		print_dev(k, &ioa->ioa, buffer, "%1", 2+k);
		i_con = add_i_con(i_con, "\0", &ioa->ioa);

		num_lines++;

		num_lines += print_standalone_disks(ioa, &i_con, buffer);
		num_lines += print_hotspare_disks(ioa, &i_con, buffer);
		num_lines += print_vsets(ioa, &i_con, buffer);
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
		s_out = screen_driver(&n_disk_status,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_disk_status.body = NULL;

	rc = s_out->rc;
	free(s_out);
	return rc;
}

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

static char *ioa_details(char *body, struct ipr_dev *dev)
{
	struct ipr_ioa_vpd ioa_vpd;
	struct ipr_cfc_vpd cfc_vpd;
	struct ipr_dram_vpd dram_vpd;
	struct ipr_inquiry_page0 page0_inq;
	struct scsi_dev_data *scsi_dev_data = dev->scsi_dev_data;
	struct ipr_dual_ioa_entry *ioa_entry;
	int rc, i;
	char buffer[200];
	int cache_size;
	u32 fw_level;

	memset(&ioa_vpd, 0, sizeof(ioa_vpd));
	memset(&cfc_vpd, 0, sizeof(cfc_vpd));
	memset(&dram_vpd, 0, sizeof(dram_vpd));

	ipr_inquiry(dev, IPR_STD_INQUIRY, &ioa_vpd, sizeof(ioa_vpd));

	rc = ipr_inquiry(dev, 0, &page0_inq, sizeof(page0_inq));
	for (i = 0; !rc && i < page0_inq.page_length; i++) {
		if (page0_inq.supported_page_codes[i] == 1) {
			ipr_inquiry(dev, 1, &cfc_vpd, sizeof(cfc_vpd));
			break;
		}
	}

	ipr_inquiry(dev, 2, &dram_vpd, sizeof(dram_vpd));

	fw_level = get_ioa_fw_version(dev->ioa);

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
			ipr_strncpy_0(buffer, (char  *)cfc_vpd.cache_size, IPR_VPD_CACHE_SIZE_LEN);
			cache_size = strtoul(buffer, NULL, 16);
			sprintf(buffer,"%d MB", cache_size);
			body = add_line_to_body(body,_("Cache Size"), buffer);
		}

		memcpy(buffer, dram_vpd.dram_size, IPR_VPD_DRAM_SIZE_LEN);
		sprintf(buffer + IPR_VPD_DRAM_SIZE_LEN, " MB");
		body = add_line_to_body(body,_("DRAM Size"), buffer);
	}
	body = add_line_to_body(body,_("Resource Name"), dev->gen_name);
	body = add_line_to_body(body,"", NULL);
	body = add_line_to_body(body,_("Physical location"), NULL);
	body = add_line_to_body(body,_("PCI Address"), dev->ioa->pci_address);
	sprintf(buffer,"%d", dev->ioa->host_num);
	body = add_line_to_body(body,_("SCSI Host Number"), buffer);
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
			ipr_strncpy_0(buffer, (char *)ioa_entry->remote_vendor_id, IPR_VENDOR_ID_LEN);
			body = add_line_to_body(body,_("Remote Adapter Manufacturer"), buffer);
			ipr_strncpy_0(buffer, (char *)ioa_entry->remote_prod_id, IPR_PROD_ID_LEN);
			body = add_line_to_body(body,_("Remote Adapter Machine Type And Model"),
						buffer);
			ipr_strncpy_0(buffer, (char *)ioa_entry->remote_sn, IPR_SERIAL_NUM_LEN);
			body = add_line_to_body(body,_("Remote Adapter Serial Number"), buffer);
		}
	}

	return body;
}

static char *vset_details(char *body, struct ipr_dev *dev)
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
	} else {
		scsi_channel = array_rcd->last_resource_addr.bus;
		scsi_id = array_rcd->last_resource_addr.target;
		scsi_lun = array_rcd->last_resource_addr.lun;
	}

	body = add_line_to_body(body,"", NULL);

	ipr_strncpy_0(buffer, (char *)array_rcd->vendor_id, IPR_VENDOR_ID_LEN);
	body = add_line_to_body(body,_("Manufacturer"), buffer);

	sprintf(buffer,"RAID %s", dev->prot_level_str);
	body = add_line_to_body(body,_("RAID Level"), buffer);

	if (ntohs(array_rcd->stripe_size) > 1024)
		sprintf(buffer,"%d M", ntohs(array_rcd->stripe_size)/1024);
	else
		sprintf(buffer,"%d k", ntohs(array_rcd->stripe_size));

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
	body = add_line_to_body(body, _("Resource Name"), dev->dev_name);

	body = add_line_to_body(body, "", NULL);
	body = add_line_to_body(body, _("Physical location"), NULL);
	body = add_line_to_body(body, _("PCI Address"), dev->ioa->pci_address);

	sprintf(buffer, "%d", dev->ioa->host_num);
	body = add_line_to_body(body, _("SCSI Host Number"), buffer);

	sprintf(buffer, "%d", scsi_channel);
	body = add_line_to_body(body, _("SCSI Channel"), buffer);

	sprintf(buffer, "%d", scsi_id);
	body = add_line_to_body(body, _("SCSI Id"), buffer);

	sprintf(buffer, "%d", scsi_lun);
	body = add_line_to_body(body, _("SCSI Lun"), buffer);
	return body;
}

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

static char *disk_details(char *body, struct ipr_dev *dev)
{
	int rc;
	struct ipr_std_inq_data_long std_inq;
	struct ipr_dev_record *device_record;
	struct ipr_dasd_inquiry_page3 page3_inq;
	struct ipr_read_cap read_cap;
	struct ipr_query_res_state res_state;
	long double device_capacity; 
	double lba_divisor;
	char product_id[IPR_PROD_ID_LEN+1];
	char vendor_id[IPR_VENDOR_ID_LEN+1];
	char serial_num[IPR_SERIAL_NUM_LEN+1];
	char buffer[100];
	int len, scsi_channel, scsi_id, scsi_lun;

	device_record = (struct ipr_dev_record *)dev->dev_rcd;

	if (dev->scsi_dev_data) {
		scsi_channel = dev->scsi_dev_data->channel;
		scsi_id = dev->scsi_dev_data->id;
		scsi_lun = dev->scsi_dev_data->lun;
	} else if (device_record) {
		scsi_channel = device_record->last_resource_addr.bus;
		scsi_id = device_record->last_resource_addr.target;
		scsi_lun = device_record->last_resource_addr.lun;
	}

	read_cap.max_user_lba = 0;
	read_cap.block_length = 0;

	rc = ipr_inquiry(dev, IPR_STD_INQUIRY, &std_inq, sizeof(std_inq));

	if (!rc) {
		ipr_strncpy_0(vendor_id, (char *)std_inq.std_inq_data.vpids.vendor_id,
			      IPR_VENDOR_ID_LEN);
		ipr_strncpy_0(product_id, (char *)std_inq.std_inq_data.vpids.product_id,
			      IPR_PROD_ID_LEN);
		ipr_strncpy_0(serial_num, (char *)std_inq.std_inq_data.serial_num,
			      IPR_SERIAL_NUM_LEN);
	} else if (device_record) {
		ipr_strncpy_0(vendor_id, (char *)device_record->vendor_id, IPR_VENDOR_ID_LEN);
		ipr_strncpy_0(product_id, (char *)device_record->product_id, IPR_PROD_ID_LEN);
		ipr_strncpy_0(serial_num, (char *)device_record->serial_num, IPR_SERIAL_NUM_LEN);
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

	memset(&read_cap, 0, sizeof(read_cap));
	rc = ipr_read_capacity(dev, &read_cap);

	if (!rc && ntohl(read_cap.block_length) &&
	    ntohl(read_cap.max_user_lba))  {

		lba_divisor = (1000*1000*1000) /
			ntohl(read_cap.block_length);

		device_capacity = ntohl(read_cap.max_user_lba) + 1;
		sprintf(buffer,"%.2Lf GB", device_capacity / lba_divisor);
		body = add_line_to_body(body,_("Capacity"), buffer);
	}

	if (strlen(dev->dev_name) > 0)
		body = add_line_to_body(body,_("Resource Name"), dev->dev_name);

	rc = ipr_query_resource_state(dev, &res_state);

	if (!rc) {
		if (is_spi(dev->ioa)) {
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

	sprintf(buffer, "%d", dev->ioa->host_num);
	body = add_line_to_body(body, _("SCSI Host Number"), buffer);

	sprintf(buffer, "%d", scsi_channel);
	body = add_line_to_body(body, _("SCSI Channel"), buffer);

	sprintf(buffer, "%d", scsi_id);
	body = add_line_to_body(body, _("SCSI Id"), buffer);

	sprintf(buffer, "%d", scsi_lun);
	body = add_line_to_body(body, _("SCSI Lun"), buffer);

	body = disk_extended_details(body, dev, &std_inq);
	return body;
}

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

	if (dev->scsi_dev_data && dev->scsi_dev_data->type == IPR_TYPE_ADAPTER) {
		n_screen = &n_adapter_details;
		body = ioa_details(body, dev);
	} else if (ipr_is_volume_set(dev)) {
		n_screen = &n_vset_details;
		body = vset_details(body, dev);
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

#define IPR_INCLUDE 0
#define IPR_REMOVE  1
#define IPR_ADD_HOT_SPARE 0
#define IPR_RMV_HOT_SPARE 1
int raid_screen(i_container *i_con)
{
	int rc;
	struct screen_output *s_out;
	int loop;

	free_raid_cmds();

	for (loop = 0; loop < n_raid_screen.num_opts; loop++) {
		n_raid_screen.body =
			ipr_list_opts(n_raid_screen.body,
				      n_raid_screen.options[loop].key,
				      n_raid_screen.options[loop].list_str);
	}

	n_raid_screen.body = ipr_end_list(n_raid_screen.body);

	s_out = screen_driver(&n_raid_screen,0,NULL);
	free(n_raid_screen.body);
	n_raid_screen.body = NULL;
	rc = s_out->rc;
	free(s_out);
	return rc;
}

int raid_status(i_container *i_con)
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
	body_init_status(buffer, n_raid_status.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->ioa.scsi_dev_data)
			continue;

		/* print Hot Spare devices*/
		for_each_hot_spare(ioa, dev) {
			print_dev(k, dev, buffer, "%1", 2+k);
			i_con = add_i_con(i_con, "\0", dev);  
			num_lines++;
		}

		num_lines += print_vsets(ioa, &i_con, buffer);
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
		s_out = screen_driver(&n_raid_status,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_status.body = NULL;

	rc = s_out->rc;
	free(s_out);
	return rc;
}

static int is_array_in_use(struct ipr_ioa *ioa,
			   u8 array_id)
{
	int opens;
	struct ipr_dev *vset;

	for_each_vset(ioa, vset) {
		if (array_id == vset->array_rcd->array_id) {
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

	/* empty the linked list that contains field pointers */
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);
	body_init_status(buffer, n_raid_stop.header, &header_lines);

	for_each_ioa(ioa) {
		for_each_vset(ioa, dev) {
			if (!dev->array_rcd->stop_cand)
				continue;
			if ((rc = is_array_in_use(ioa, dev->array_rcd->array_id)))
				continue;

			add_raid_cmd_tail(ioa, dev, dev->array_rcd->array_id);
			i_con = add_i_con(i_con, "\0", raid_cmd_tail);

			print_dev(k, dev, buffer, "%1", 2+k);
			found++;
		}
	}

	if (!found) {
		/* Stop Device Parity Protection Failed */
		n_raid_stop_fail.body = body_init(n_raid_stop_fail.header, NULL);
		s_out = screen_driver(&n_raid_stop_fail,0,i_con);

		free(n_raid_stop_fail.body);
		n_raid_stop_fail.body = NULL;
	} else {
		toggle_field = 0;

		do {
			n_raid_stop.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_stop,header_lines,i_con);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);
	}

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_stop.body = NULL;

	return rc;
}

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
		s_out = screen_driver(&n_confirm_raid_stop,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_raid_stop.body = NULL;

	return rc;
}

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

int do_confirm_raid_stop(i_container *i_con)
{
	struct ipr_dev *vset;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *ioa;
	int rc;
	int max_y, max_x;

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

		if (vset->scsi_dev_data)
			rc = ipr_start_stop_stop(vset);

		if (rc != 0)
			return (20 | EXIT_FLAG);
		ioa->num_raid_cmds++;
	}

	for_each_ioa(ioa) {
		if (ioa->num_raid_cmds == 0)
			continue;
		ioa->num_raid_cmds = 0;

		rc = ipr_stop_array_protection(ioa);
		if (rc != 0)
			return (20 | EXIT_FLAG);
	}

	rc = raid_stop_complete();
	return rc;
}

int raid_start(i_container *i_con)
{
	int rc, k;
	int found = 0;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;
	int toggle = 0;

	processing();

	/* empty the linked list that contains field pointers */
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);
	body_init_status(buffer, n_raid_start.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->start_array_qac_entry)
			continue;

		add_raid_cmd_tail(ioa, &ioa->ioa, ioa->start_array_qac_entry->array_id);

		i_con = add_i_con(i_con,"\0",raid_cmd_tail);

		print_dev(k, &ioa->ioa, buffer, "%1", k);
		found++;
	}

	if (!found) {
		/* Start Device Parity Protection Failed */
		n_raid_start_fail.body = body_init(n_raid_start_fail.header, NULL);
		s_out = screen_driver(&n_raid_start_fail,0,i_con);

		free(n_raid_start_fail.body);
		n_raid_start_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		do {
			n_raid_start.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_start,header_lines,i_con);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		rc = s_out->rc;
		free(s_out);

		free_raid_cmds();
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_raid_start.body = NULL;

	return rc;
}

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
			s_out = screen_driver(&n_configure_raid_start,header_lines,i_con2);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		rc = s_out->rc;

		if (rc > 0)
			goto leave;

		i_con2 = s_out->i_con;
		free(s_out);

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

int configure_raid_parameters(i_container *i_con)
{
	FIELD *input_fields[5];
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
	char raid_str[16], stripe_str[16], qdepth_str[4];
	int ch, start_row;
	int cur_field_index;
	int selected_count = 0;
	int stripe_sz, stripe_sz_mask, stripe_sz_list[16];
	struct prot_level *prot_level_list;
	int *userptr = NULL;
	int *retptr;
	int max_x,max_y,start_y,start_x;
	int qdepth, new_qdepth;

	getmaxyx(stdscr,max_y,max_x);
	getbegyx(stdscr,start_y,start_x);

	rc = RC_SUCCESS;

	ioa = cur_raid_cmd->ioa;
	supported_arrays = ioa->supported_arrays;
	cap_entry = supported_arrays->entry;

	/* determine number of devices selected for this parity set */
	for_each_af_dasd(ioa, dev) {
		if (dev->dev_rcd->issue_cmd)
			selected_count++;
	}

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

	input_fields[4] = NULL;

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
	mvprintw(10, 0, "Queue Depth (default = %3d). . . . . . . :", qdepth);
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
				 passed pointer to display data */
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
				 passed pointer to display data */
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

				if (new_qdepth > 128)
					qdepth = 128;
				else
					qdepth = new_qdepth;
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
		s_out = screen_driver(&n_confirm_raid_start,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

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
			rc = 19;  
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
	}

	rc = raid_start_complete();

	return rc;
}

int raid_start_complete()
{
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int done_bad;
	int not_done = 0;
	int rc;
	u32 percent_cmplt = 0;
	int device_available = 0;
	int wait_device = 0;
	int exit_now = 0;
	struct ipr_ioa *ioa;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_dev *ipr_dev;
	struct ipr_disk_attr attr;

	while (1) {
		rc = complete_screen_driver(&n_raid_start_complete,percent_cmplt,1);

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

					check_current_config(false);
					ipr_dev = get_dev_from_handle(status_record->resource_handle);
					if ((ipr_dev)  &&  (ipr_dev->scsi_dev_data)) {
						device_available = 1;
						ipr_init_new_dev(ipr_dev);
						if (ipr_get_dev_attr(ipr_dev, &attr)) {
							syslog(LOG_ERR, _("Unable to read queue_depth"));
						} else {
							attr.queue_depth = cur_raid_cmd->qdepth;
							if (ipr_set_dev_attr(ipr_dev, &attr, 1))
								syslog(LOG_ERR, _("Unable to set queue_depth"));
						}
					}
				}

				if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS) {
					if (status_record->percent_complete < percent_cmplt)
						percent_cmplt = status_record->percent_complete;
					not_done = 1;
				} else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL) {
					done_bad = 1;
					syslog(LOG_ERR, _("Start parity protect to %s failed.  "
							  "Check device configuration for proper format.\n"),
					       ioa->ioa.gen_name);
					rc = RC_FAILED;
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

			if (done_bad)
				/* Start Parity Protection failed. */
				return 19; 

			check_current_config(false);

			/* Start Parity Protection completed successfully */
			return 18; 
		}
		not_done = 0;
		sleep(1);
	}
}

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
		s_out = screen_driver(&n_raid_rebuild_fail,0,NULL);

		free(n_raid_rebuild_fail.body);
		n_raid_rebuild_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		do {
			n_raid_rebuild.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_rebuild,header_lines,i_con);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		rc = s_out->rc;
		free(s_out);

		free_raid_cmds();
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_rebuild.body = NULL;

	return rc;
}

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
		s_out = screen_driver(&n_confirm_raid_rebuild,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

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
		rc = complete_screen_driver(&n_raid_resync_complete,percent_cmplt,1);

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

int raid_resync(i_container *i_con)
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
	body_init_status(buffer, n_raid_resync.header, &header_lines);

	for_each_ioa(ioa) {
		ioa->num_raid_cmds = 0;
		for_each_vset(ioa, dev) {
			if (!dev->array_rcd->resync_cand)
				continue;

			add_raid_cmd_tail(ioa, dev, dev->array_rcd->array_id);
			i_con = add_i_con(i_con, "\0", raid_cmd_tail);
			print_dev(k, dev, buffer, "%1", k);
			found++;
			ioa->num_raid_cmds++;
		}
	}
 
	if (!found) {
		/* Start Device Parity Protection Failed */
		n_raid_resync_fail.body = body_init(n_raid_resync_fail.header, NULL);
		s_out = screen_driver(&n_raid_resync_fail,0,NULL);

		free(n_raid_resync_fail.body);
		n_raid_resync_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		do {
			n_raid_resync.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_resync,header_lines,i_con);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		rc = s_out->rc;
		free(s_out);
		free_raid_cmds();
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_resync.body = NULL;

	return rc;
}

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
		s_out = screen_driver(&n_confirm_raid_resync,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

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
						       array_rcd->raid_level);

			if (!cap_entry || !cap_entry->include_allowed || !array_rcd->established)
				continue;

			add_raid_cmd_tail(ioa, dev, array_rcd->array_id);

			i_con = add_i_con(i_con,"\0",raid_cmd_tail);
			print_dev(k, dev, buffer, "%1", k);
			found++;
		}
	}

	if (!found) {
		/* Include Device Parity Protection Failed */
		n_raid_include_fail.body = body_init(n_raid_include_fail.header, NULL);
		s_out = screen_driver(&n_raid_include_fail,0,i_con);

		free(n_raid_include_fail.body);
		n_raid_include_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		toggle_field = 0;

		do {
			n_raid_include.body = buffer[toggle&1];
			s_out = screen_driver(&n_raid_include,header_lines,i_con);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		rc = s_out->rc;
		free(s_out);
		free_raid_cmds();
	}

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_raid_include.body = NULL;

	return rc;
}

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
	rc = ipr_query_array_config(ioa, 0, 1, cur_raid_cmd->array_id, qac_data);

	found = 0;

	if (rc != 0)
		free(qac_data);
	else  {
		cur_raid_cmd->qac = qac_data;
		cap_entry = get_raid_cap_entry(ioa->supported_arrays,
					       vset->array_rcd->raid_level);

		if (cap_entry)
			min_mult_array_devices = cap_entry->min_mult_array_devices;

		for_each_dev_rcd(dev_rcd, qac_data) {
			for_each_dev(ioa, dev) {
				scsi_dev_data = dev->scsi_dev_data;
				if (!scsi_dev_data)
					continue;

				if (scsi_dev_data->handle == dev_rcd->resource_handle &&
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
		s_out = screen_driver(&n_configure_raid_include_fail,0,i_con);

		free(n_configure_raid_include_fail.body);
		n_configure_raid_include_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);

		return rc;
	}

	toggle_field = 0;

	do {
		n_configure_raid_include.body = buffer[toggle&1];
		s_out = screen_driver(&n_configure_raid_include,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_configure_raid_include.body = NULL;

	i_con = s_out->i_con;

	if ((rc = s_out->rc) > 0)
		goto leave;

	/* first, count devices select to be sure min multiple
	 is satisfied */
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

		free(s_out);
	return rc;
}

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
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

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

static int update_include_qac_data(struct ipr_array_query_data *old_qac,
				   struct ipr_array_query_data *new_qac)
{
	struct ipr_dev_record *old_dev_rcd, *new_dev_rcd;
	int found;

	for_each_dev_rcd(old_dev_rcd, old_qac) {
		if (!old_dev_rcd->issue_cmd)
			continue;

		found = 0;
		for_each_dev_rcd(new_dev_rcd, new_qac) {
			if (new_dev_rcd->resource_handle != old_dev_rcd->resource_handle)
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

	rc = __ipr_query_array_config(ioa, fd, 0, 1, array_id, &qac_data);

	if (rc)
		goto out;

	rc = update_include_qac_data(qac, &qac_data);
	if (rc)
		goto out;

	rc = ipr_add_array_device(ioa, fd, &qac_data);

out:
	close(fd);
	return rc;
}

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
		complete_screen_driver(&n_dev_include_complete, percent_cmplt,0);

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
						rc = 26;
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
				return 26 | EXIT_FLAG; 

			/*  "Include Unit completed successfully" */
			complete_screen_driver(&n_dev_include_complete, percent_cmplt,1);
			return 27 | EXIT_FLAG;
		}
		not_done = 0;
		sleep(1);
	}
}

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
}

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
				} else
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

				if (action_code == IPR_REMOVE)
					new_block_size = IPR_JBOD_BLOCK_SIZE;
				else
					new_block_size = ioa->af_block_size;

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
			s_out = screen_driver(&n_af_include_fail,0,i_con);

			free(n_af_include_fail.body);
			n_af_include_fail.body = NULL;
		} else {
			n_af_remove_fail.body = body_init(n_af_remove_fail.header, NULL);
			s_out = screen_driver(&n_af_remove_fail,0,i_con);

			free(n_af_remove_fail.body);
			n_af_remove_fail.body = NULL;
		}
	} else {
		toggle_field = 0;

		do {
			n_screen->body = buffer[toggle&1];
			s_out = screen_driver(n_screen,header_lines,i_con);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		free_devs_to_init();
	}

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_screen->body = NULL;

	return rc;
}

int af_include(i_container *i_con)
{
	int rc;
	rc = configure_af_device(i_con, IPR_INCLUDE);
	return rc;
}

int af_remove(i_container *i_con)
{
	int rc;
	rc = configure_af_device(i_con, IPR_REMOVE);
	return rc;
}

int add_hot_spare(i_container *i_con)
{
	int rc;

	do {
		rc = hot_spare(i_con, IPR_ADD_HOT_SPARE);
	} while (rc & REFRESH_FLAG);
	return rc;
}

int remove_hot_spare(i_container *i_con)
{
	int rc;

	do {
		rc = hot_spare(i_con, IPR_RMV_HOT_SPARE);
	} while (rc & REFRESH_FLAG);
	return rc;
}

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
			s_out = screen_driver(&n_add_hot_spare_fail,0,i_con);

			free(n_add_hot_spare_fail.body);
			n_add_hot_spare_fail.body = NULL;
		} else {
			n_remove_hot_spare_fail.body = body_init(n_remove_hot_spare_fail.header, NULL);
			s_out = screen_driver(&n_remove_hot_spare_fail,0,i_con);

			free(n_remove_hot_spare_fail.body);
			n_remove_hot_spare_fail.body = NULL;
		}

		rc = EXIT_FLAG;
		goto leave;
	}

	do {
		n_screen->body = buffer[toggle&1];
		s_out = screen_driver(n_screen,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	i_con = s_out->i_con;

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
			s_out = screen_driver(n_screen,header_lines,i_con2);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		for (k = 0; k < 2; k++) {
			free(buffer[k]);
			buffer[k] = NULL;
		}

		n_screen->body = NULL;

		rc = s_out->rc;
		i_con2 = s_out->i_con;

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
	free(s_out);
	return rc;
}

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
		s_out = screen_driver(n_screen,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_screen->body = NULL;

	rc = s_out->rc;
	free(s_out);

	if(rc > 0)
		goto leave;

	rc = hot_spare_complete(action);

	leave:

		return rc;
}

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

	s_out = screen_driver(&n_disk_unit_recovery,0,NULL);
	free(n_disk_unit_recovery.body);
	n_disk_unit_recovery.body = NULL;

	rc = s_out->rc;
	free(s_out);
	return rc;
}

static int get_res_addr(struct ipr_dev *dev, struct ipr_res_addr *res_addr)
{
	struct ipr_dev_record *dev_record = dev->dev_rcd;
	struct ipr_array_record *array_record = dev->array_rcd;

	if (dev->scsi_dev_data) {
		res_addr->host = dev->scsi_dev_data->host;
		res_addr->bus = dev->scsi_dev_data->channel;
		res_addr->target = dev->scsi_dev_data->id;
		res_addr->lun = dev->scsi_dev_data->lun;
	} else if (ipr_is_af_dasd_device(dev)) {
		if (dev_record && dev_record->no_cfgte_dev) {
			res_addr->host = dev->ioa->host_num;
			res_addr->bus = dev_record->last_resource_addr.bus;
			res_addr->target = dev_record->last_resource_addr.target;
			res_addr->lun = dev_record->last_resource_addr.lun;
		} else if (dev_record) {
			res_addr->host = dev->ioa->host_num;
			res_addr->bus = dev_record->resource_addr.bus;
			res_addr->target = dev_record->resource_addr.target;
			res_addr->lun = dev_record->resource_addr.lun;
		} else
			return -1;
	} else if (ipr_is_volume_set(dev)) {
		if (array_record && array_record->no_config_entry) {
			res_addr->host = dev->ioa->host_num;
			res_addr->bus = array_record->last_resource_addr.bus;
			res_addr->target = array_record->last_resource_addr.target;
			res_addr->lun = array_record->last_resource_addr.lun;
		} else if (array_record) {
			res_addr->host = dev->ioa->host_num;
			res_addr->bus = array_record->resource_addr.bus;
			res_addr->target = array_record->resource_addr.target;
			res_addr->lun = array_record->resource_addr.lun;
		} else
			return -1;
	} else
		return -1;
	return 0;
}

int process_conc_maint(i_container *i_con, int action)
{
	i_container *temp_i_con;
	int found = 0;
	struct ipr_dev *dev, *ses;
	char *input;
	struct ipr_ioa *ioa;
	int i, k, rc;
	struct ipr_dev_record *dev_rcd;
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status;
	char *buffer[2];
	int header_lines;
	int toggle=1;
	s_node *n_screen;
	struct screen_output *s_out;
	struct ipr_res_addr res_addr;
	int time = 12;
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

	if (get_res_addr(dev, &res_addr))
		return INVALID_OPTION_STATUS; /* FIXME */

	if (ipr_is_af_dasd_device(dev) &&
	    (action == IPR_VERIFY_CONC_REMOVE || action == IPR_WAIT_CONC_REMOVE)) {
		/* issue suspend device bus to verify operation
		 is allowable */
		rc = ipr_suspend_device_bus(dev->ioa, &res_addr,
					    IPR_SDB_CHECK_ONLY);

		if (rc)
			return INVALID_OPTION_STATUS; /* FIXME */

	} else if (num_device_opens(res_addr.host, res_addr.bus,
				    res_addr.target, res_addr.lun))
		return INVALID_OPTION_STATUS;  /* FIXME */

	ioa = dev->ioa;

	/* flash light at location of specified device */
	for_each_ses(ioa, ses) {
		rc = ipr_receive_diagnostics(ses, 2, &ses_data,
					     sizeof(struct ipr_encl_status_ctl_pg));

		if (rc || res_addr.bus != ses->scsi_dev_data->channel)
			continue;

		found = 0;
		i = 0;
		for_each_elem_status(elem_status, &ses_data) {
			if (res_addr.target == elem_status->scsi_id) {
				found++;

				if ((action == IPR_VERIFY_CONC_REMOVE)  ||
				    (action == IPR_VERIFY_CONC_ADD)) {
					elem_status->select = 1;
					elem_status->identify = 1;
				} else if (action == IPR_WAIT_CONC_REMOVE) {
					elem_status->select = 1;
					elem_status->remove = 1;
				} else if (action == IPR_WAIT_CONC_ADD) {
					elem_status->select = 1;
					elem_status->insert = 1;
				}
				break;
			}
			i++;
		}

		if (found)
			break;
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	if (action == IPR_WAIT_CONC_REMOVE || action == IPR_WAIT_CONC_ADD) {
		ses_data.overall_status_select = 1;
		ses_data.overall_status_disable_resets = 1;
		ses_data.overall_status_insert = 0;
		ses_data.overall_status_remove = 0;
		ses_data.overall_status_identify = 0;
	}

	if (action == IPR_VERIFY_CONC_REMOVE)
		n_screen = &n_verify_conc_remove;
	else if (action == IPR_VERIFY_CONC_ADD)
		n_screen = &n_verify_conc_add;
	else if (action == IPR_WAIT_CONC_REMOVE)
		n_screen = &n_wait_conc_remove;
	else
		n_screen = &n_wait_conc_add;

	processing();
	for (k = 0; k < 2; k++) {
		buffer[k] = __body_init_status(n_screen->header, &header_lines, k);
		buffer[k] = print_device(dev, buffer[k], "1", k);
	}

	rc = ipr_send_diagnostics(ses, &ses_data,
				  sizeof(struct ipr_encl_status_ctl_pg));

	if (rc) {
		for (k = 0; k < 2; k++) {
			free(buffer[k]);
			buffer[k] = NULL;
		}
		return 30 | EXIT_FLAG;
	}

	/* call screen driver */
	do {
		n_screen->body = buffer[toggle&1];
		s_out = screen_driver(n_screen,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);
	rc = s_out->rc;

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_screen->body = NULL;

	/* turn light off flashing light */
	ses_data.overall_status_select = 1;
	ses_data.overall_status_disable_resets = 0;
	ses_data.overall_status_insert = 0;
	ses_data.overall_status_remove = 0;
	ses_data.overall_status_identify = 0;
	ses_data.elem_status[i].select = 1;
	ses_data.elem_status[i].insert = 0;
	ses_data.elem_status[i].remove = 0;
	ses_data.elem_status[i].identify = 0;

	ipr_send_diagnostics(ses, &ses_data,
			     sizeof(struct ipr_encl_status_ctl_pg));

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

				ipr_write_dev_attr(dev, "delete", "1");
				evaluate_device(dev, dev->ioa, 0);
				ipr_del_zeroed_dev(dev);
			}
		} else if (action == IPR_VERIFY_CONC_ADD) {
			rc = process_conc_maint(i_con, IPR_WAIT_CONC_ADD);
			if (!rc) {
				getmaxyx(stdscr,max_y,max_x);
				move(max_y-1,0);
				printw(_("Operation in progress - please wait"));
				refresh();
				ipr_scan(ioa, res_addr.bus, res_addr.target, res_addr.lun);

				while (time--) {
					check_current_config(false);
					if ((dev = get_dev_from_addr(&res_addr))) {
						ipr_init_new_dev(dev);
						break;
					}
					sleep(5);
				}
			}
		}
	}

	return rc;
}

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

static void free_empty_slots(struct ipr_dev **dev, int num)
{
	int i;

	for (i = 0; i < num; i++) {
		if (!dev[i] || !dev[i]->scsi_dev_data)
			continue;
		if (dev[i]->scsi_dev_data->type != IPR_TYPE_EMPTY_SLOT)
			continue;
		free(dev[i]->scsi_dev_data);
		dev[i]->scsi_dev_data = NULL;
		free(dev[i]);
		dev[i] = NULL;
	}
}

static struct ipr_dev *alloc_empty_slot(struct ipr_ioa *ioa, int bus, int id)
{
	struct ipr_dev *dev;
	struct scsi_dev_data *scsi_dev_data;

	dev = calloc(1, sizeof(*dev));
	scsi_dev_data = calloc(1, sizeof(*scsi_dev_data));
	scsi_dev_data->type = IPR_TYPE_EMPTY_SLOT;
	scsi_dev_data->host = ioa->host_num;
	scsi_dev_data->channel = bus;
	scsi_dev_data->id = id;
	dev->scsi_dev_data = scsi_dev_data;
	dev->ioa = ioa;
	return dev;
}

static int get_conc_devs(struct ipr_dev ***ret, int action)
{
	struct ipr_ioa *ioa;
	struct ipr_res_addr ra;
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status;
	struct ipr_dev **devs = NULL;
	struct ipr_dev *ses, *dev;
	int num_devs = 0;
	int ses_bus, scsi_id_found;

	for_each_primary_ioa(ioa) {
		for_each_ses(ioa, ses) {
			if (ipr_receive_diagnostics(ses, 2, &ses_data, sizeof(ses_data)))
				continue;

			ses_bus = ses->scsi_dev_data->channel;
			scsi_id_found = 0;

			for_each_elem_status(elem_status, &ses_data) {
				if (scsi_id_found & (1 << elem_status->scsi_id))
					continue;
				scsi_id_found |= (1 << elem_status->scsi_id);

				if (elem_status->status == IPR_DRIVE_ELEM_STATUS_EMPTY) {
					devs = realloc(devs, (sizeof(struct ipr_dev *) * (num_devs + 1)));
					devs[num_devs] = alloc_empty_slot(ioa, ses_bus,
									  elem_status->scsi_id);
					num_devs++;
					continue;
				}

				for_each_dev(ioa, dev) {
					if (get_res_addr(dev, &ra)) {
						scsi_dbg(dev, "Cannot find resource address\n");
						continue;
					}

					if (ra.bus == ses_bus && ra.target == elem_status->scsi_id) {
						if (ipr_is_ses(dev))
							break;

						if (action == IPR_CONC_REMOVE) {
							if (ipr_suspend_device_bus(ioa, &ra, IPR_SDB_CHECK_ONLY))
								break;
							if (format_in_prog(dev))
								break;
						} else if (dev->scsi_dev_data && action != IPR_CONC_IDENTIFY)
							break;
						devs = realloc(devs, (sizeof(struct ipr_dev *) * (num_devs + 1)));
						devs[num_devs] = dev;
						num_devs++;
						break;
					}
				}

				if ((dev - ioa->dev) == ioa->num_devices) {
					devs = realloc(devs, (sizeof(struct ipr_dev *) * (num_devs + 1)));
					devs[num_devs] = alloc_empty_slot(ioa, ses_bus,
									  elem_status->scsi_id);
					num_devs++;
				}
			}
		}
	}

	*ret = devs;
	return num_devs;  
}

int start_conc_maint(i_container *i_con, int action)
{
	int rc, i, k;
	char *buffer[2];
	int num_lines = 0;
	struct screen_output *s_out;
	struct ipr_dev **devs;
	int toggle = 1;
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

	body_init_status(buffer, n_screen->header, &header_lines);

	num_devs = get_conc_devs(&devs, action);

	for (i = 0; i < num_devs; i++) {
		print_dev(k, devs[i], buffer, "%1", k);
		i_con = add_i_con(i_con,"\0", devs[i]);
		num_lines++;
	}

	if (num_lines == 0) {
		for (k = 0; k < 2; k++) 
			buffer[k] = add_string_to_body(buffer[k], _("(No Eligible Devices Found)"),
						     "", NULL);
	}

	do {
		n_screen->body = buffer[toggle&1];
		s_out = screen_driver(n_screen,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_screen->body = NULL;

	if (!s_out->rc) {
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
	free(s_out);
	return rc;  
}

int concurrent_add_device(i_container *i_con)
{
	return start_conc_maint(i_con, IPR_CONC_ADD);
}

int concurrent_remove_device(i_container *i_con)
{
	return start_conc_maint(i_con, IPR_CONC_REMOVE);
}

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

	for_each_ioa(ioa) {
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
		s_out = screen_driver(&n_init_device,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_init_device.body = NULL;
	free_devs_to_init();
	return rc;
}

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
		s_out = screen_driver(&n_confirm_init_device,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_init_device.body = NULL;

	return rc;
}

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
			rc = complete_screen_driver(&n_dev_init_complete, percent_cmplt,1);

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

			if (ipr_read_dev_attr(dev->dev, "type", dev_type)) {
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
					} else if (dev->new_block_size != ioa->af_block_size) {
						ipr_write_dev_attr(dev->dev, "rescan", "1");
						ipr_init_dev(dev->dev);
					}
				}

				if (dev->new_block_size != 0) {
					if (dev->new_block_size == ioa->af_block_size)
						enable_af(dev->dev);

					evaluate_device(dev->dev, ioa, dev->new_block_size);
				}

				if (dev->new_block_size == ioa->af_block_size || ipr_is_af_dasd_device(dev->dev))
					ipr_add_zeroed_dev(dev->dev);
			}

			flush_stdscr();

			if (done_bad) {
				if (!pid)
					exit(0);

				/* Initialize and format failed */
				return 51 | EXIT_FLAG;
			}

			if (!pid)
				exit(0);

			/* Initialize and format completed successfully */
			return 50 | EXIT_FLAG; 
		}

		not_done = 0;
		sleep(1);
	}
}

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

			if (cur_dev_init->new_block_size == IPR_JBOD_BLOCK_SIZE) {
				/* Issue mode select to change block size */
				mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
				memset(ioctl_buffer, 0, 255);

				mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);
				block_desc = (struct ipr_block_desc *)(mode_parm_hdr + 1);

				/* Setup block size */
				block_desc->block_length[0] = 0x00;
				block_desc->block_length[1] = IPR_JBOD_BLOCK_SIZE >> 8;
				block_desc->block_length[2] = IPR_JBOD_BLOCK_SIZE & 0xff;

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
			rc = ipr_format_unit(cur_dev_init->dev);  /* FIXME  Mandatory lock? */
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
				block_desc->block_length[1] = IPR_JBOD_BLOCK_SIZE >> 8;
				block_desc->block_length[2] = IPR_JBOD_BLOCK_SIZE & 0xff;
			}

			length = sizeof(struct ipr_block_desc) +
				sizeof(struct ipr_mode_parm_hdr);

			status = ipr_mode_select(cur_dev_init->dev,
						 &ioctl_buffer, length);

			if (status && cur_dev_init->new_block_size == ioa->af_block_size) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* Issue format */
			status = ipr_format_unit(cur_dev_init->dev);  /* FIXME  Mandatory lock? */   

			if (status) {
				/* Send a device reset to cleanup any old state */
				rc = ipr_reset_device(cur_dev_init->dev);

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
	/* empty the linked list that contains field pointers */
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
		s_out = screen_driver(&n_reclaim_cache,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

leave:

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_reclaim_cache.body = NULL;

	return rc;
}


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
		s_out = screen_driver(&n_confirm_reclaim,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_reclaim.body = NULL;

	return rc;
}

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
		s_out = screen_driver(&n_confirm_reclaim_warning,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_reclaim_warning.body = NULL;

	return rc;
}

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

static void print_reclaim_results(struct ipr_reclaim_query_data *buf)
{
	char *body = get_reclaim_results(buf);

	printf(body);
	free(body);
}

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
	s_out = screen_driver(&n_reclaim_result,0,NULL);
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
		s_out = screen_driver(&n_battery_maint,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

leave:

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_battery_maint.body = NULL;

	return rc;
}

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

	sprintf(buffer,"%d",reclaim_data->raw_power_on_time);
	body = add_line_to_body(body,_("Power on time (days)"), buffer);

	sprintf(buffer,"%d",reclaim_data->adjusted_power_on_time);
	body = add_line_to_body(body,_("Adjusted power on time (days)"), buffer);

	sprintf(buffer,"%d",reclaim_data->estimated_time_to_battery_warning);
	body = add_line_to_body(body,_("Estimated time to warning (days)"), buffer);

	sprintf(buffer,"%d",reclaim_data->estimated_time_to_battery_failure);
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

int show_battery_info(struct ipr_ioa *ioa)
{
	struct screen_output *s_out;
	int rc;

	n_show_battery_info.body = get_battery_info(ioa);
	s_out = screen_driver(&n_show_battery_info,0,NULL);

	free(n_show_battery_info.body);
	n_show_battery_info.body = NULL;
	rc = s_out->rc;
	free(s_out);
	return rc;
}

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
		s_out = screen_driver(&n_confirm_force_battery_error,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}

	n_confirm_force_battery_error.body = NULL;

	return rc;
}

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
		s_out = screen_driver(&n_confirm_start_cache,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_start_cache.body = NULL;

	return rc;
}

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
		if ((!is_spi(ioa) || ioa->is_aux_cache) && !ipr_debug)
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
		s_out = screen_driver(&n_bus_config_fail,0,NULL);

		free(n_bus_config_fail.body);
		n_bus_config_fail.body = NULL;

		rc = s_out->rc;
		free(s_out);
	} else {
		do {
			n_bus_config.body = buffer[toggle&1];
			s_out = screen_driver(&n_bus_config,header_lines,i_con);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		rc = s_out->rc;
		free(s_out);
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

static void free_all_bus_attr_buf(void)
{
	struct bus_attr *bus_attr;

	while (bus_attr_head) {
		bus_attr = bus_attr_head->next;
		free(bus_attr_head);
		bus_attr_head = bus_attr;
	}
}	

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

}

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
		s_out = screen_driver(&n_change_bus_attr,header_lines,i_con);
		rc = s_out->rc;

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
	free(s_out);
	return rc;
}

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

int driver_config(i_container *i_con)
{
	int rc, k;
	char *buffer[2];
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	int header_lines;
	int toggle = 0;

	processing();
	/* empty the linked list that contains field pointers */
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
		s_out = screen_driver(&n_driver_config,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_driver_config.body = NULL;

	return rc;
}

int get_log_level(struct ipr_ioa *ioa)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;
	int value;

	class_device = sysfs_open_class_device("scsi_host",
					       ioa->host_name);
	if (!class_device)
		return 0;

	attr = sysfs_get_classdev_attr(class_device,
				       "log_level");
	if (!attr)
		return 0;

	sscanf(attr->value, "%d", &value);
	sysfs_close_class_device(class_device);

	return value;
}

void set_log_level(struct ipr_ioa *ioa, char *log_level)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;

	class_device = sysfs_open_class_device("scsi_host",
					       ioa->host_name);
	if (!class_device)
		return;

	attr = sysfs_get_classdev_attr(class_device,
				       "log_level");
	if (!attr)
		return;

	sysfs_write_attribute(attr, log_level, 2);
	sysfs_close_class_device(class_device);
}

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

int disk_config(i_container * i_con)
{
	int rc, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *ioa;
	struct scsi_dev_data *scsi_dev_data;
	struct screen_output *s_out;
	int header_lines;
	char *buffer[2];
	int toggle = 1;
	struct ipr_dev *dev, *vset;

	processing();
	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status(buffer, n_disk_config.header, &header_lines);

	for_each_ioa(ioa) {
		if (ioa->is_secondary || ioa->ioa_dead || !ioa->ioa.scsi_dev_data)
			continue;

		/* print JBOD and non-member AF devices*/
		for_each_dev(ioa, dev) {
			scsi_dev_data = dev->scsi_dev_data;
			if (!scsi_dev_data ||
			    ((scsi_dev_data->type != TYPE_DISK) &&
			     (scsi_dev_data->type != IPR_TYPE_AF_DISK)) ||
			    ipr_is_hot_spare(dev) || ipr_is_volume_set(dev) ||
			    ipr_is_array_member(dev))
				continue;

			print_dev(k, dev, buffer, "%1", k);
			i_con = add_i_con(i_con, "\0", dev);  

			num_lines++;
		}

		/* print Hot Spare devices*/
		for_each_hot_spare(ioa, dev) {
			if (!dev->scsi_dev_data)
				continue;

			print_dev(k, dev, buffer, "%1", k);
			i_con = add_i_con(i_con, "\0", dev);  

			num_lines++;
		}

		/* print volume set and associated devices*/
		for_each_vset(ioa, vset) {
			print_dev(k, vset, buffer, "%1", k);
			i_con = add_i_con(i_con, "\0", vset);
			num_lines++;

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
		n_disk_config.body = buffer[toggle&1];
		s_out = screen_driver(&n_disk_config,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_disk_config.body = NULL;

	rc = s_out->rc;
	free(s_out);
	return rc;
}

struct disk_config_attr {
	int option;
	int queue_depth;
	int format_timeout;
	int tcq_enabled;
};

static int disk_config_menu(struct ipr_dev *ipr_dev, struct disk_config_attr *disk_config_attr,
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
		if (disk_config_attr->queue_depth > 128)
			disk_config_attr->queue_depth = 128;
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
	}

	return rc;
}

int change_disk_config(i_container * i_con)
{
	int rc;
	i_container *i_con_head_saved;
	struct ipr_dev *dev;
	char qdepth_str[4];
	char format_timeout_str[5];
	char buffer[128];
	i_container *temp_i_con;
	int found = 0;
	char *input;
	struct disk_config_attr disk_config_attr[3];
	struct disk_config_attr *config_attr = NULL;
	struct ipr_disk_attr disk_attr;
	int header_lines = 0;
	char *body = NULL;
	struct screen_output *s_out;
	struct ipr_array_record *array_record;
	int tcq_warning = 0;
	int tcq_blocked = 0;
	int tcq_enabled = 0;

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
		body = add_line_to_body(body,_("Queue Depth"), "%3");
		disk_config_attr[0].option = 1;
		disk_config_attr[0].queue_depth = disk_attr.queue_depth;
		sprintf(qdepth_str,"%d",disk_config_attr[0].queue_depth);
		i_con = add_i_con(i_con, qdepth_str, &disk_config_attr[0]);
	}

	array_record = dev->array_rcd;

	if (array_record &&
	    array_record->common.record_id == IPR_RECORD_ID_ARRAY_RECORD) {
		/* VSET, no further fields */
	} else if (ipr_is_af_dasd_device(dev)) {
		disk_config_attr[1].option = 2;
		disk_config_attr[1].format_timeout = disk_attr.format_timeout / (60 * 60);
		body = add_line_to_body(body,_("Format Timeout"), "%4");
		sprintf(format_timeout_str,"%d hr",disk_config_attr[1].format_timeout);
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

	n_change_disk_config.body = body;
	while (1) {
		s_out = screen_driver(&n_change_disk_config, header_lines, i_con);
		rc = s_out->rc;

		found = 0;

		for_each_icon(temp_i_con) {
			if (!temp_i_con->field_data[0]) {
				config_attr = (struct disk_config_attr *)temp_i_con->data;
				rc = disk_config_menu(dev, config_attr, temp_i_con->y, header_lines);

				if (rc == CANCEL_FLAG)
					rc = RC_SUCCESS;

				if (config_attr->option == 1) {
					sprintf(temp_i_con->field_data,"%d", config_attr->queue_depth);
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

	leave:

	free(n_change_disk_config.body);
	n_change_disk_config.body = NULL;
	free_i_con(i_con);
	i_con_head = i_con_head_saved;
	free(s_out);
	return rc;
}

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
		if (!ioa->dual_raid_support)
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
		s_out = screen_driver(&n_ioa_config,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_ioa_config.body = NULL;

	rc = s_out->rc;
	free(s_out);
	return rc;
}

struct ioa_config_attr {
	int option;
	int preferred_primary;
};

int ioa_config_menu(struct ipr_dev *ipr_dev, struct ioa_config_attr *ioa_config_attr,
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

	if (ioa_config_attr->option == 1) {
		num_menu_items = 2;
		menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
		userptr = malloc(sizeof(int) * num_menu_items);

		menu_index = 0;
		menu_item[menu_index] = new_item("None","");
		userptr[menu_index] = 0;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);

		menu_index++;

		menu_item[menu_index] = new_item("Primary","");
		userptr[menu_index] = 1;
		set_item_userptr(menu_item[menu_index],
				 (char *)&userptr[menu_index]);
		menu_index++;

		menu_item[menu_index] = (ITEM *)NULL;
		rc = display_menu(menu_item, start_row, menu_index, &retptr);
		if (rc == RC_SUCCESS)
			ioa_config_attr->preferred_primary = *retptr;

		i = 0;
		while (menu_item[i] != NULL)
			free_item(menu_item[i++]);
		free(menu_item);
		free(userptr);
		menu_item = NULL;
	}

	return rc;
}

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
	struct ioa_config_attr ioa_config_attr[3];
	struct ioa_config_attr *config_attr = NULL;
	struct ipr_ioa_attr ioa_attr;
	int header_lines = 0;
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

	body = body_init(n_change_disk_config.header, &header_lines);
	sprintf(buffer, "Adapter: %s/%d   %s %s\n", dev->ioa->pci_address,
		dev->ioa->host_num, dev->scsi_dev_data->vendor_id,
		dev->scsi_dev_data->product_id);
	body = add_line_to_body(body, buffer, NULL);
	header_lines += 2;

	body = add_line_to_body(body,_("Preferred Dual Adapter State"), "%13");
	ioa_config_attr[0].option = 1;
	ioa_config_attr[0].preferred_primary = ioa_attr.preferred_primary;
	if (ioa_attr.preferred_primary)
		sprintf(pref_str, "Primary");
	else
		sprintf(pref_str, "None");
	i_con = add_i_con(i_con, pref_str, &ioa_config_attr[0]);

	n_change_ioa_config.body = body;
	while (1) {
		s_out = screen_driver(&n_change_ioa_config, header_lines, i_con);
		rc = s_out->rc;

		found = 0;
		for_each_icon(temp_i_con) {
			if (!temp_i_con->field_data[0]) {
				config_attr = (struct ioa_config_attr *)temp_i_con->data;
				rc = ioa_config_menu(dev, config_attr, temp_i_con->y, header_lines);

				if (rc == CANCEL_FLAG)
					rc = RC_SUCCESS;

				if (config_attr->option == 1) {
					if (config_attr->preferred_primary)
						sprintf(temp_i_con->field_data, "Primary");
					else
						sprintf(temp_i_con->field_data, "None");
					ioa_attr.preferred_primary = config_attr->preferred_primary;
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

	ipr_set_ioa_attr(dev->ioa, &ioa_attr, 1);

	leave:

	free(n_change_ioa_config.body);
	n_change_ioa_config.body = NULL;
	free_i_con(i_con);
	i_con_head = i_con_head_saved;
	free(s_out);
	return rc;
}

int download_ucode(i_container * i_con)
{
	int rc, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev, *vset;
	struct scsi_dev_data *scsi_dev_data;
	struct screen_output *s_out;
	int header_lines;
	char *buffer[2];
	int toggle = 0;

	processing();

	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);
	body_init_status(buffer, n_download_ucode.header, &header_lines);

	for_each_ioa(ioa) {
		if (!ioa->ioa.scsi_dev_data || ioa->ioa_dead || ioa->is_secondary)
			continue;

		print_dev(k, &ioa->ioa, buffer, "%1", k);
		i_con = add_i_con(i_con, "\0", &ioa->ioa);

		num_lines++;

		/* print JBOD and non-member AF devices*/
		for_each_dev(ioa, dev) {
			scsi_dev_data = dev->scsi_dev_data;
			if ((scsi_dev_data == NULL) ||
			    ((scsi_dev_data->type != TYPE_DISK) &&
			     (scsi_dev_data->type != IPR_TYPE_AF_DISK) /* &&
									(scsi_dev_data->type != TYPE_ENCLOSURE) &&
									(scsi_dev_data->type != TYPE_PROCESSOR) */) ||
			    ipr_is_hot_spare(dev) || ipr_is_volume_set(dev) ||
			    ipr_is_array_member(dev))
				continue;

			print_dev(k, dev, buffer, "%1", k);
			i_con = add_i_con(i_con, "\0", dev);  

			num_lines++;
		}

		/* print Hot Spare devices*/
		for_each_hot_spare(ioa, dev) {
			print_dev(k, dev, buffer, "%1", k);
			i_con = add_i_con(i_con, "\0", dev);  
			num_lines++;
		}

		/* print volume set associated devices*/
		for_each_vset(ioa, vset) {
			num_lines++;

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
		n_download_ucode.body = buffer[toggle&1];
		s_out = screen_driver(&n_download_ucode,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k = 0; k < 2; k++) {
		free(buffer[k]);
		buffer[k] = NULL;
	}
	n_download_ucode.body = NULL;

	rc = s_out->rc;
	free(s_out);
	return rc;
}

static int update_ucode(struct ipr_dev *dev, struct ipr_fw_images *fw_image)
{
	char *body;
	int header_lines, status, time = 0;
	pid_t pid, done;
	struct ipr_dasd_inquiry_page3 page3_inq;
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
		if (dev->scsi_dev_data && dev->scsi_dev_data->type == IPR_TYPE_ADAPTER)
			ipr_update_ioa_fw(dev->ioa, fw_image, 1);
		else
			ipr_update_disk_fw(dev, fw_image, 1);
		exit(0);
	}

	memset(&page3_inq, 0, sizeof(page3_inq));
	ipr_inquiry(dev, 3, &page3_inq, sizeof(page3_inq));

	fw_version = page3_inq.release_level[0] << 24 |
		page3_inq.release_level[1] << 16 |
		page3_inq.release_level[2] << 8 |
		page3_inq.release_level[3];

	if (fw_version != fw_image->version)
		rc = 67 | EXIT_FLAG;

	free(n_download_ucode_in_progress.body);
	n_download_ucode_in_progress.body = NULL;
	return rc;
}

int process_choose_ucode(struct ipr_dev *ipr_dev)
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
	struct ipr_dasd_inquiry_page3 page3_inq;
	struct ipr_res_addr res_addr;

	if (!ipr_dev->scsi_dev_data)
		return 67 | EXIT_FLAG;
	if (ipr_dev->scsi_dev_data->type == IPR_TYPE_ADAPTER)
		rc = get_ioa_firmware_image_list(ipr_dev->ioa, &list);
/*
	else if (ipr_dev->scsi_dev_data->type == TYPE_ENCLOSURE || ipr_dev->scsi_dev_data->type == TYPE_PROCESSOR)
		rc = get_ses_firmware_image_list(ipr_dev, &list);
*/
	else
		rc = get_dasd_firmware_image_list(ipr_dev, &list);

	if (rc < 0)
		return 67 | EXIT_FLAG;
	else
		list_count = rc;

	memset(&page3_inq, 0, sizeof(page3_inq));
	ipr_inquiry(ipr_dev, 3, &page3_inq, sizeof(page3_inq));

	if (ipr_dev->scsi_dev_data &&
	    ipr_dev->scsi_dev_data->type == IPR_TYPE_ADAPTER) {
		sprintf(buffer, _("Adapter to download: %-8s %-16s\n"),
			ipr_dev->scsi_dev_data->vendor_id,
			ipr_dev->scsi_dev_data->product_id);
		body = add_string_to_body(NULL, buffer, "", &header_lines);
		sprintf(buffer, _("Adapter Location: %s.%d/\n"),
			ipr_dev->ioa->pci_address, ipr_dev->ioa->host_num);

	} else {
		get_res_addr(ipr_dev, &res_addr);
		sprintf(buffer, _("Device to download: %-8s %-16s\n"),
			ipr_dev->scsi_dev_data->vendor_id,
			ipr_dev->scsi_dev_data->product_id);
		body = add_string_to_body(NULL, buffer, "", &header_lines);
		sprintf(buffer, _("Device Location: %d:%d:%d\n"),
			res_addr.bus, res_addr.target, res_addr.lun);
	}

	body = add_string_to_body(body, buffer, "", &header_lines);

	if (isprint(page3_inq.release_level[0]) &&
	    isprint(page3_inq.release_level[1]) &&
	    isprint(page3_inq.release_level[2]) &&
	    isprint(page3_inq.release_level[3]))
		sprintf(buffer, "%s%02X%02X%02X%02X (%c%c%c%c)\n\n",
			_("The current microcode for this device is: "),
			page3_inq.release_level[0],
			page3_inq.release_level[1],
			page3_inq.release_level[2],
			page3_inq.release_level[3],
			page3_inq.release_level[0],
			page3_inq.release_level[1],
			page3_inq.release_level[2],
			page3_inq.release_level[3]);
	else
		sprintf(buffer, "%s%02X%02X%02X%02X\n\n",
			_("The current microcode for this device is: "),
			page3_inq.release_level[0],
			page3_inq.release_level[1],
			page3_inq.release_level[2],
			page3_inq.release_level[3]);

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
				sprintf(buffer," %%1   %.8X (%c%c%c%c) %s",list[i].version,
					version[0], version[1], version[2],	version[3],
					list[i].file);
			else
				sprintf(buffer," %%1   %.8X        %s",list[i].version,list[i].file);

			body = add_line_to_body(body, buffer, NULL);
			i_con = add_i_con(i_con, "\0", &list[i]);
		}
	} else {
		body = add_line_to_body(body,"(No available images)",NULL);
	}

	n_choose_ucode.body = body;
	s_out = screen_driver(&n_choose_ucode, header_lines, i_con);

	free(n_choose_ucode.body);
	n_choose_ucode.body = NULL;

	for_each_icon(temp_i_con) {
		fw_image =(struct ipr_fw_images *)(temp_i_con->data);

		if (ipr_dev == NULL)
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
		sprintf(buffer," 1   %.8X (%c%c%c%c) %s\n",fw_image->version,
			version[0], version[1], version[2],	version[3],
			fw_image->file);
	else
		sprintf(buffer," 1   %.8X        %s\n",fw_image->version,fw_image->file);

	body = add_line_to_body(body, buffer, NULL);

	n_confirm_download_ucode.body = body;
	s_out = screen_driver(&n_confirm_download_ucode,header_lines,i_con);

	free(n_confirm_download_ucode.body);
	n_confirm_download_ucode.body = NULL;

	if (!s_out->rc)
		rc = update_ucode(ipr_dev, fw_image);

	leave:
		free(list);

	i_con = free_i_con(i_con);
	i_con_head = i_con_head_saved;
	free(s_out);
	return rc;
}

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
		dev =(struct ipr_dev *)(temp_i_con->data);

		if (!dev)
			continue;

		input = temp_i_con->field_data;

		if (input == NULL)
			continue;

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

#define MAX_CMD_LENGTH 1000
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

	s_out = screen_driver(&n_log_menu,0,NULL);
	free(n_log_menu.body);
	n_log_menu.body = NULL;
	rc = s_out->rc;
	i_con = s_out->i_con;
	i_con = free_i_con(i_con);
	free(s_out);
	return rc;
}


int ibm_storage_log_tail(i_container *i_con)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc, len;

	def_prog_mode();
	rc = system("rm -rf "_PATH_TMP".ipr.err; mkdir "_PATH_TMP".ipr.err");

	if (rc != 0) {
		len = sprintf(cmnd, "cd %s; zcat -f messages", log_root_dir);

		len += sprintf(cmnd + len," | grep ipr | ");
		len += sprintf(cmnd + len, "sed \"s/ `hostname -s` kernel: ipr//g\" | ");
		len += sprintf(cmnd + len, "sed \"s/ localhost kernel: ipr//g\" | ");
		len += sprintf(cmnd + len, "sed \"s/ kernel: ipr//g\" | ");
		len += sprintf(cmnd + len, "sed \"s/\\^M//g\" | ");
		len += sprintf(cmnd + len, FAILSAFE_EDITOR);
		closelog();
		openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
		syslog(LOG_ERR, "Error encountered concatenating log files...\n");
		syslog(LOG_ERR, "Using failsafe editor...\n");
		closelog();
		openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
		sleep(3);
	} else {
		len = sprintf(cmnd,"cd %s; zcat -f messages | grep ipr |", log_root_dir);
		len += sprintf(cmnd + len, "sed \"s/ `hostname -s` kernel: ipr//g\"");
		len += sprintf(cmnd + len, " | sed \"s/ localhost kernel: ipr//g\"");
		len += sprintf(cmnd + len, " | sed \"s/ kernel: ipr//g\"");
		len += sprintf(cmnd + len, " | sed \"s/\\^M//g\" ");
		len += sprintf(cmnd + len, ">> "_PATH_TMP".ipr.err/errors");
		system(cmnd);

		sprintf(cmnd, "%s "_PATH_TMP".ipr.err/errors", editor);
	}

	rc = system(cmnd);

	if ((rc != 0) && (rc != 127)) {
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return 65; 
	} else
		/* return with no status */
		return 1; 
}

static int select_log_file(const struct dirent *dir_entry)
{
	if (dir_entry) {
		if (strstr(dir_entry->d_name, "messages") == dir_entry->d_name)
			return 1;
	}
	return 0;
}

static int compare_log_file(const void *log_file1, const void *log_file2)
{
	struct dirent **first_dir, **second_dir;
	char name1[MAX_CMD_LENGTH], name2[MAX_CMD_LENGTH];
	struct stat stat1, stat2;

	first_dir = (struct dirent **)log_file1;
	second_dir = (struct dirent **)log_file2;

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

int ibm_storage_log(i_container *i_con)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc, i;
	struct dirent **log_files;
	struct dirent **dirent;
	int num_dir_entries;
	int len;

	def_prog_mode();

	num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
	rc = system("rm -rf "_PATH_TMP".ipr.err; mkdir "_PATH_TMP".ipr.err");
	if (num_dir_entries)
		dirent = log_files;

	if (rc != 0) {
		len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

		/* Probably have a read-only root file system */
		for (i = 0; i < num_dir_entries; i++) {
			len += sprintf(cmnd + len, "%s ", (*dirent)->d_name);
			dirent++;
		}

		len += sprintf(cmnd + len," | grep ipr-err | ");
		len += sprintf(cmnd + len, "sed \"s/ `hostname -s` kernel: ipr//g\" | ");
		len += sprintf(cmnd + len, "sed \"s/ localhost kernel: ipr//g\" | ");
		len += sprintf(cmnd + len, "sed \"s/ kernel: ipr//g\" | ");
		len += sprintf(cmnd + len, "sed \"s/\\^M//g\" | ");
		len += sprintf(cmnd + len, FAILSAFE_EDITOR);
		closelog();
		openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
		syslog(LOG_ERR, "Error encountered concatenating log files...\n");
		syslog(LOG_ERR, "Using failsafe editor...\n");
		closelog();
		openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
		sleep(3);
	} else {
		for (i = 0; i < num_dir_entries; i++) {
			len = sprintf(cmnd,"cd %s; zcat -f %s | grep ipr |", log_root_dir, (*dirent)->d_name);
			len += sprintf(cmnd + len, "sed \"s/ `hostname -s` kernel: ipr//g\"");
			len += sprintf(cmnd + len, " | sed \"s/ localhost kernel: ipr//g\"");
			len += sprintf(cmnd + len, " | sed \"s/ kernel: ipr//g\"");
			len += sprintf(cmnd + len, " | sed \"s/\\^M//g\" ");
			len += sprintf(cmnd + len, ">> "_PATH_TMP".ipr.err/errors");
			system(cmnd);
			dirent++;
		}

		sprintf(cmnd, "%s "_PATH_TMP".ipr.err/errors", editor);
	}

	rc = system(cmnd);

	if (num_dir_entries)
		free(*log_files);

	if ((rc != 0) && (rc != 127)) {
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return 65; 
	} else
		/* return with no status */
		return 1; 
}

int kernel_log(i_container *i_con)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc, i;
	struct dirent **log_files;
	struct dirent **dirent;
	int num_dir_entries;
	int len;

	def_prog_mode();

	num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
	if (num_dir_entries)
		dirent = log_files;

	rc = system("rm -rf "_PATH_TMP".ipr.err; mkdir "_PATH_TMP".ipr.err");

	if (rc != 0) {
		/* Probably have a read-only root file system */
		len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

		/* Probably have a read-only root file system */
		for (i = 0; i < num_dir_entries; i++) {
			len += sprintf(cmnd + len, "%s ", (*dirent)->d_name);
			dirent++;
		}

		len += sprintf(cmnd + len, " | sed \"s/\\^M//g\" ");
		len += sprintf(cmnd + len, "| %s", FAILSAFE_EDITOR);
		closelog();
		openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
		syslog(LOG_ERR, "Error encountered concatenating log files...\n");
		syslog(LOG_ERR, "Using failsafe editor...\n");
		openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
		sleep(3);
	} else {
		for (i = 0; i < num_dir_entries; i++) {
			sprintf(cmnd,
				"cd %s; zcat -f %s | sed \"s/\\^M//g\" >> "_PATH_TMP".ipr.err/errors",
				log_root_dir, (*dirent)->d_name);
			system(cmnd);
			dirent++;
		}

		sprintf(cmnd, "%s "_PATH_TMP".ipr.err/errors", editor);
	}

	rc = system(cmnd);
	if (num_dir_entries)
		free(*log_files);

	if ((rc != 0) && (rc != 127))	{
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return 65; 
	}
	else
		/* return with no status */
		return 1; 
}

int iprconfig_log(i_container *i_con)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc, i;
	struct dirent **log_files;
	struct dirent **dirent;
	int num_dir_entries;
	int len;

	def_prog_mode();

	num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
	rc = system("rm -rf "_PATH_TMP".ipr.err; mkdir "_PATH_TMP".ipr.err");
	if (num_dir_entries)
		dirent = log_files;

	if (rc != 0) {
		len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

		/* Probably have a read-only root file system */
		for (i = 0; i < num_dir_entries; i++) {
			len += sprintf(cmnd + len, "%s ", (*dirent)->d_name);
			dirent++;
		}

		len += sprintf(cmnd + len," | grep iprconfig | ");
		len += sprintf(cmnd + len, FAILSAFE_EDITOR);
		closelog();
		openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
		syslog(LOG_ERR, "Error encountered concatenating log files...\n");
		syslog(LOG_ERR, "Using failsafe editor...\n");
		openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
		sleep(3);
	} else {
		for (i = 0; i < num_dir_entries; i++) {
			len = sprintf(cmnd,"cd %s; zcat -f %s | grep iprconfig ", log_root_dir, (*dirent)->d_name);
			len += sprintf(cmnd + len, ">> "_PATH_TMP".ipr.err/errors");
			system(cmnd);
			dirent++;
		}

		sprintf(cmnd, "%s "_PATH_TMP".ipr.err/errors", editor);
	}

	rc = system(cmnd);
	if (num_dir_entries)
		free(*log_files);

	if ((rc != 0) && (rc != 127))	{
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return 65; 
	} else
		/* return with no status */
		return 1; 
}

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
	s_out = screen_driver(&n_kernel_root,0,i_con);

	free(n_kernel_root.body);
	n_kernel_root.body = NULL;
	rc = s_out->rc;
	free(s_out);

	return rc;
}

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
	s_out = screen_driver(&n_confirm_kernel_root,0,i_con);

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
	s_out = screen_driver(&n_set_default_editor,0,i_con);

	free(n_set_default_editor.body);
	n_set_default_editor.body = NULL;
	rc = s_out->rc;
	free(s_out);

	return rc;
}

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
	s_out = screen_driver(&n_confirm_set_default_editor,0,i_con);

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

int restore_log_defaults(i_container *i_con)
{
	strcpy(log_root_dir, DEFAULT_LOG_DIR);
	strcpy(editor, DEFAULT_EDITOR);
	return 64; /*"Default log values restored"*/
}

int ibm_boot_log(i_container *i_con)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc;
	int len;
	struct stat file_stat;

	sprintf(cmnd,"%s/boot.msg",log_root_dir);
	rc = stat(cmnd, &file_stat);
	if (rc)
		return 2; /* "Invalid option specified" */

	def_prog_mode();
	rc = system("rm -rf "_PATH_TMP".ipr.err; mkdir "_PATH_TMP".ipr.err");

	if (rc != 0) {
		len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

		/* Probably have a read-only root file system */
		len += sprintf(cmnd + len, "boot.msg");
		len += sprintf(cmnd + len," | grep ipr |");
		len += sprintf(cmnd + len, FAILSAFE_EDITOR);
		closelog();
		openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
		syslog(LOG_ERR, "Error encountered concatenating log files...\n");
		syslog(LOG_ERR, "Using failsafe editor...\n");
		closelog();
		openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
		sleep(3);
	} else {
		len = sprintf(cmnd,"cd %s; zcat -f boot.msg | grep ipr  | "
			      "sed 's/<[0-9]>ipr-err: //g' | sed 's/<[0-9]>ipr: //g'",
			      log_root_dir);
		len += sprintf(cmnd + len, ">> "_PATH_TMP".ipr.err/errors");
		system(cmnd);
		sprintf(cmnd, "%s "_PATH_TMP".ipr.err/errors", editor);
	}
	rc = system(cmnd);

	if ((rc != 0) && (rc != 127)) {
		s_status.num = rc;
		return 65; /* "Editor returned %d. Try setting the default editor" */
	} else
		return 1; /* return with no status */
}

static void get_status(struct ipr_dev *ipr_dev, char *buf, int percent)
{
	struct scsi_dev_data *scsi_dev_data = ipr_dev->scsi_dev_data;
	struct ipr_ioa *ioa = ipr_dev->ioa;
	int rc;
	struct ipr_query_res_state res_state;
	u8 ioctl_buffer[255];
	int status;
	int format_req = 0;
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	struct ipr_block_desc *block_desc;
	struct sense_data_t sense_data;
	struct ipr_array_record *array_record;
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int percent_cmplt = 0;
	int format_in_progress = 0;
	int resync_in_progress = 0;

	if (scsi_dev_data && scsi_dev_data->type == IPR_TYPE_ADAPTER) {
		if (ioa->ioa_dead)
			sprintf(buf, "Not Operational");
		else if (ioa->nr_ioa_microcode)
			sprintf(buf, "Not Ready");
		else
			sprintf(buf, "Operational");
	} else if (scsi_dev_data && scsi_dev_data->type == IPR_TYPE_EMPTY_SLOT) {
		sprintf(buf, "Empty");
	} else {
		if (ipr_is_volume_set(ipr_dev)) {
			rc = ipr_query_command_status(&ioa->ioa, &cmd_status);

			if (!rc) {
				array_record = ipr_dev->array_rcd;

				for_each_cmd_status(status_record, &cmd_status) {
					if (array_record->array_id == status_record->array_id) {
						if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS) {
							if (status_record->command_code == IPR_RESYNC_ARRAY_PROTECTION)
								resync_in_progress = 1;
							percent_cmplt = status_record->percent_complete;
						}
					}
				}
			}
		} else if (ipr_is_af_dasd_device(ipr_dev)) {
			rc = ipr_query_command_status(ipr_dev, &cmd_status);

			if ((rc == 0) && (cmd_status.num_records != 0)) {
				status_record = cmd_status.record;
				if ((status_record->status != IPR_CMD_STATUS_SUCCESSFUL) &&
				    (status_record->status != IPR_CMD_STATUS_FAILED)) {
					percent_cmplt = status_record->percent_complete;
					format_in_progress++;
				}
			}
		} else if (ipr_is_gscsi(ipr_dev)) {
			/* Send Test Unit Ready to find percent complete in sense data. */
			rc = ipr_test_unit_ready(ipr_dev, &sense_data);

			if ((rc == CHECK_CONDITION) &&
			    ((sense_data.error_code & 0x7F) == 0x70) &&
			    ((sense_data.sense_key & 0x0F) == 0x02) && /* NOT READY */
			    (sense_data.add_sense_code == 0x04) &&     /* LOGICAL UNIT NOT READY */
			    (sense_data.add_sense_code_qual == 0x04)) {/* FORMAT IN PROGRESS */

				percent_cmplt = ((int)sense_data.sense_key_spec[1]*100)/0x100;
				format_in_progress++;
			}
		}

		if (ipr_is_af(ipr_dev)) {
			memset(&res_state, 0, sizeof(res_state));

			/* Do a query resource state */
			rc = ipr_query_resource_state(ipr_dev, &res_state);

			if (rc != 0)
				res_state.not_oper = 1;
		} else { /* JBOD */
			memset(&res_state, 0, sizeof(res_state));

			format_req = 0;

			/* Issue mode sense/mode select to determine if device needs to be formatted */
			status = ipr_mode_sense(ipr_dev, 0x0a, &ioctl_buffer);

			if (status == CHECK_CONDITION &&
			    sense_data.add_sense_code == 0x31 &&
			    sense_data.add_sense_code_qual == 0x00) {
				format_req = 1;
			} else {
				if (!status) {
					mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

					if (mode_parm_hdr->block_desc_len > 0) {
						block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

						if ((block_desc->block_length[1] != 0x02) ||
						    (block_desc->block_length[2] != 0x00))
							format_req = 1;
					}
				}

				/* check device with test unit ready */
				rc = ipr_test_unit_ready(ipr_dev, &sense_data);

				if (rc == CHECK_CONDITION &&
				    sense_data.add_sense_code == 0x31 &&
				    sense_data.add_sense_code_qual == 0x00)
					format_req = 1;
				else if (rc)
					res_state.not_oper = 1;
			}
		}

		if (format_in_progress)
			sprintf(buf, "%d%% Formatted", percent_cmplt);
		else if (!scsi_dev_data && ipr_dev->ioa->is_secondary)
			sprintf(buf, "Remote");
		else if (!scsi_dev_data)
			sprintf(buf, "Missing");
		else if (!scsi_dev_data->online)
			sprintf(buf, "Offline");
		else if (res_state.not_oper || res_state.not_func)
			sprintf(buf, "Failed");
		else if (res_state.read_write_prot)
			sprintf(buf, "R/W Protected");
		else if (res_state.prot_dev_failed)
			sprintf(buf, "Failed");
		else if (ipr_is_volume_set(ipr_dev)) {
			if (res_state.prot_suspended && ipr_is_volume_set(ipr_dev))
				sprintf(buf, "Degraded");
			else if (res_state.prot_resuming && ipr_is_volume_set(ipr_dev)) {
				if (!percent || (percent_cmplt == 0))
					sprintf(buf, "Rebuilding");
				else
					sprintf(buf, "%d%% Rebuilt", percent_cmplt);
			} else if (resync_in_progress) {
				if (!percent || (percent_cmplt == 0))
					sprintf(buf, "Checking");
				else
					sprintf(buf, "%d%% Checked", percent_cmplt);
			} else if (res_state.degraded_oper || res_state.service_req)
				sprintf(buf, "Degraded");
			else
				sprintf(buf, "Active");
		} else if (format_req)
			sprintf(buf, "Format Required");
		else if (ipr_device_is_zeroed(ipr_dev))
			sprintf(buf, "Zeroed");
		else
			sprintf(buf, "Active");
	}
}

char *__print_device(struct ipr_dev *dev, char *body, char *option,
		     int sd, int sg, int vpd, int percent, int indent)
{
	u16 len = 0;
	struct scsi_dev_data *scsi_dev_data = dev->scsi_dev_data;
	int i;
	struct ipr_res_addr res_addr;
	char ioctl_buffer[255];
	char raid_str[48];
	char *dev_name = dev->dev_name;
	char *gen_name = dev->gen_name;
	char node_name[7], buf[100];
	int tab_stop = 0;
	char vendor_id[IPR_VENDOR_ID_LEN + 1];
	char product_id[IPR_PROD_ID_LEN + 1];
	struct ipr_common_record *common_record;
	struct ipr_dev_record *device_record;
	struct ipr_array_record *array_record;
	struct ipr_ioa *ioa = dev->ioa;

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

	len += sprintf(body + len, "%-6s %s/%d:",
		       node_name,
		       ioa->pci_address,
		       ioa->host_num);

	if (scsi_dev_data && scsi_dev_data->type == IPR_TYPE_ADAPTER) {
		if (!vpd) {
			len += sprintf(body + len,"            %-25s ", get_ioa_desc(dev->ioa));
		} else
			len += sprintf(body + len,"            %-8s %-16s ",
				       scsi_dev_data->vendor_id,
				       scsi_dev_data->product_id);
	} else if ((scsi_dev_data) &&
		   (scsi_dev_data->type == IPR_TYPE_EMPTY_SLOT)) {

		tab_stop  = sprintf(body + len,"%d:%d: ",
				    scsi_dev_data->channel,
				    scsi_dev_data->id);

		len += tab_stop;

		for (i = 0; i < 12-tab_stop; i++)
			body[len+i] = ' ';

		len += 12-tab_stop;
		len += sprintf(body + len, "%-8s %-16s "," ", " ");
	} else {
		get_res_addr(dev, &res_addr);

		tab_stop  = sprintf(body + len,"%d:%d:%d ", res_addr.bus,
				    res_addr.target, res_addr.lun);

		if (scsi_dev_data) {
			ipr_strncpy_0(vendor_id, scsi_dev_data->vendor_id, IPR_VENDOR_ID_LEN);
			ipr_strncpy_0(product_id, scsi_dev_data->product_id, IPR_PROD_ID_LEN);
		}
		else if (dev->qac_entry) {
			common_record = dev->qac_entry;
			if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) {
				device_record = (struct ipr_dev_record *)common_record;
				ipr_strncpy_0(vendor_id, (char *)device_record->vendor_id, IPR_VENDOR_ID_LEN);
				ipr_strncpy_0(product_id , (char *)device_record->product_id, IPR_PROD_ID_LEN);
			} else if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) {
				array_record = (struct ipr_array_record *)common_record;
				ipr_strncpy_0(vendor_id, (char *)array_record->vendor_id, IPR_VENDOR_ID_LEN);
				ipr_strncpy_0(product_id , (char *)array_record->product_id,
					      IPR_PROD_ID_LEN);
			}
		}

		len += tab_stop;

		for (i = 0; i < 12-tab_stop; i++)
			body[len+i] = ' ';

		len += 12-tab_stop;

		if (vpd) {
			len += sprintf(body + len, "%-8s %-16s ",
				       vendor_id, product_id);
		} else {
			if (ipr_is_hot_spare(dev))
				len += sprintf(body + len, "%-25s ", "Hot Spare");
			else if (ipr_is_volume_set(dev)) {
				sprintf(ioctl_buffer, "RAID %s Disk Array",
					dev->prot_level_str);
				len += sprintf(body + len, "%-25s ", ioctl_buffer);
			} else if (ipr_is_array_member(dev)) {
				if (indent)
					sprintf(raid_str,"  RAID %s Array Member",
						dev->prot_level_str);
				else
					sprintf(raid_str,"RAID %s Array Member",
						dev->prot_level_str);

				len += sprintf(body + len, "%-25s ", raid_str);
			} else if (ipr_is_af_dasd_device(dev))
				len += sprintf(body + len, "%-25s ", "Advanced Function Disk");
			else if (scsi_dev_data && scsi_dev_data->type == TYPE_ENCLOSURE)
				len += sprintf(body + len, "%-25s ", "Enclosure");
			else if (scsi_dev_data && scsi_dev_data->type == TYPE_PROCESSOR)
				len += sprintf(body + len, "%-25s ", "Processor");
			else
				len += sprintf(body + len, "%-25s ", "Physical Disk");
		}
	}

	get_status(dev, buf, percent);
	sprintf(body + len, "%s\n", buf);
	return body;

}

/*
 * Print the given device to a buffer. The type parameter is defined as
 * a bitfield which has the following behavior:
 *
 * 0: print sd, device VPD
 * 1: print sd, device description, print % complete
 * 2: print sg, device VPD
 * 3: print sd, device description, indent array members, print % complete
 *
 */
char *print_device(struct ipr_dev *dev, char *body, char *option, int type)
{
	int sd, sg, vpd, percent, indent;

	sd = sg = vpd = percent = indent = 0;

	if (type == 2)
		sg = 1;
	else
		sd = 1;

	if (type & 1)
		percent = 1;
	else
		vpd = 1;

	if (type == 3)
		indent = 1;

	return __print_device(dev, body, option, sd, sg, vpd, percent, indent);
}

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

static struct sysfs_dev *head_sdev;
static struct sysfs_dev *tail_sdev;

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

static int raid_create_add_dev(char *name)
{
	struct ipr_dev *dev = find_and_add_dev(name);

	if (!dev)
		return -EINVAL;

	if (ipr_is_gscsi(dev)) {
		if (is_af_blocked(dev, 0))
			return -EINVAL;
		if (!is_format_allowed(dev))
			return -EIO;
		add_format_device(dev, dev->ioa->af_block_size);
		dev_init_tail->do_init = 1;
	}

	return 0;
}

static int raid_create_check_num_devs(struct ipr_array_cap_entry *cap,
				      int num_devs)
{
	if (num_devs < cap->min_num_array_devices ||
	    num_devs > cap->max_num_array_devices ||
	    (num_devs % cap->min_mult_array_devices) != 0) {
		syslog(LOG_ERR, _("Invalid number of devices selected for RAID %s array. %d. "
				  "Must select minimum of %d devices, maximum of %d devices, "
				  "and must be a multiple of %d devices\n"),
		       cap->prot_level_str, num_devs, cap->min_num_array_devices,
		       cap->max_num_array_devices, cap->min_mult_array_devices);
		return -EINVAL;
	}
	return 0;
}

static void curses_init()
{
	/* makes program compatible with all terminals -
	 originally did not display text correctly when user was running xterm */
	setenv("TERM", "vt100", 1);
	setlocale(LC_ALL, "");
	initscr();
}

static int format_devices(char **args, int num_args, int blksz)
{
	int i;
	struct ipr_dev *dev;

	for (i = 0; i < num_args; i++) {
		dev = find_dev(args[i]);
		if (!dev) {
			fprintf(stderr, "Invalid device: %s\n", args[i]);
			continue;
		}

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

	send_dev_inits(NULL);
	free_devs_to_init();
	return 0;
}

static int recovery_format(char **args, int num_args)
{
	return format_devices(args, num_args, 0);
}

static int format_for_jbod(char **args, int num_args)
{
	return format_devices(args, num_args, IPR_JBOD_BLOCK_SIZE);
}

static int format_for_raid(char **args, int num_args)
{
	int i;
	struct ipr_dev *dev;

	for (i = 0; i < num_args; i++) {
		dev = find_and_add_dev(args[i]);
		if (!dev || !can_format_for_raid(dev)) {
			fprintf(stderr, "Invalid device: %s\n", args[i]);
			return -EINVAL;
		}

		add_format_device(dev, dev->ioa->af_block_size);
		dev_init_tail->do_init = 1;
	}

	send_dev_inits(NULL);
	free_devs_to_init();
	return 0;
}

static int raid_create(char **args, int num_args)
{
	int i, num_devs = 0, rc;
	int next_raid_level, next_stripe_size, next_qdepth;
	char *raid_level = IPR_DEFAULT_RAID_LVL;
	int stripe_size, qdepth;
	struct ipr_dev *dev;
	struct sysfs_dev *sdev;
	struct ipr_ioa *ioa = NULL;
	struct ipr_array_cap_entry *cap;

	next_raid_level = 0;
	next_stripe_size = 0;
	next_qdepth = 0;
	stripe_size = 0;

	for (i = 0; i < num_args; i++) {
		if (strcmp(args[i], "-r") == 0)
			next_raid_level = 1;
		else if (strcmp(args[i], "-s") == 0)
			next_stripe_size = 1;
		else if (strcmp(args[i], "-q") == 0)
			next_qdepth = 1;
		else if (next_raid_level) {
			next_raid_level = 0;
			raid_level = args[i];
		} else if (next_stripe_size) {
			next_stripe_size = 0;
			stripe_size = strtoul(args[i], NULL, 10);
		} else if (next_qdepth) {
			next_qdepth = 0;
			qdepth = strtoul(args[i], NULL, 10);
		} else if (find_dev(args[i])) {
			num_devs++;
			if (raid_create_add_dev(args[i]))
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
			syslog(LOG_ERR, _("All devices must be attached to the same adapter\n"));
			return -EINVAL;
		}
	}

	cap = get_cap_entry(ioa->supported_arrays, raid_level);

	if (!cap) {
		syslog(LOG_ERR, _("Invalid RAID level for selected adapter. %s\n"), raid_level);
		return -EINVAL;
	}

	if (stripe_size && ((cap->supported_stripe_sizes & stripe_size) != stripe_size)) {
		syslog(LOG_ERR, _("Unsupported stripe size for selected adapter. %d\n"),
		       stripe_size);
		return -EINVAL;
	}

	if ((rc = raid_create_check_num_devs(cap, num_devs)))
		return rc;

	if (!stripe_size)
		stripe_size = cap->recommended_stripe_size;
	if (!qdepth)
		qdepth = num_devs * 4;

	if (dev_init_head) {
		send_dev_inits(NULL);
		free_devs_to_init();
	}

	check_current_config(false);

	for (sdev = head_sdev; sdev; sdev = sdev->next) {
		dev = ipr_sysfs_dev_to_dev(sdev);
		if (!dev) {
			syslog(LOG_ERR, _("Cannot find device: %s\n"),
			       sdev->sysfs_device_name);
			return -EIO;
		}

		if (!ipr_is_af_dasd_device(dev)) {
			scsi_err(dev, "Invalid device type\n");
			return -EIO;
		}

		dev->dev_rcd->issue_cmd = 1;
	}

	add_raid_cmd_tail(ioa, &ioa->ioa, ioa->start_array_qac_entry->array_id);
	raid_cmd_tail->qdepth = qdepth;
	raid_cmd_tail->do_cmd = 1;

	rc = ipr_start_array_protection(ioa, stripe_size, cap->prot_level);

	if (rc)
		return rc;

	return raid_start_complete();
}

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

static int raid_include_cmd(char **args, int num_args)
{
	int i, zeroed = 0;
	struct ipr_dev *dev, *vset = NULL;
	struct ipr_ioa *ioa;

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
				fprintf(stderr, "Invalid parameters. Only one disk array can "
					"specified at once\n");
				return -EINVAL;
			}

			vset = dev;
			ioa = vset->ioa;
			continue;
		}

		if (!ipr_is_af_dasd_device(dev)) {
			fprintf(stderr, "Invalid device specified. Device must be formatted "
				"to Advanced Function Format\n");
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
		fprintf(stderr, "Invalid parameters. A disk array must be specified\n");
		return -EINVAL;
	}

	if (!zeroed) {
		send_dev_inits(NULL);
		free_devs_to_init();
	}

	return do_array_include(vset->ioa, vset->array_rcd->array_id,
				vset->ioa->qac_data);
}

static int raid_delete(char **args, int num_args)
{
	int rc;
	struct ipr_dev *dev;

	dev = find_and_add_dev(args[0]);

	if (!dev) {
		syslog(LOG_ERR, _("Invalid device specified: %s\n"), args[0]);
		return -EINVAL;
	}

	dev->array_rcd->issue_cmd = 1;
	if (dev->scsi_dev_data)
		rc = ipr_start_stop_stop(dev);
	if ((rc = ipr_stop_array_protection(dev->ioa)))
		return rc;

	return rc;
}

static void printf_device(struct ipr_dev *dev, int type)
{
	char *buf = print_device(dev, NULL, NULL, type);
	printf("%s", buf);
	free(buf);
}

static void __printf_device(struct ipr_dev *dev, int sd, int sg, int vpd,
			    int percent, int indent)
{
	char *buf = __print_device(dev, NULL, NULL, sd, sg, vpd, percent, indent);
	printf("%s", buf);
	free(buf);
}

static void printf_vsets(struct ipr_ioa *ioa, int type)
{
	struct ipr_dev *vset, *dev;

	for_each_vset(ioa, vset) {
		printf_device(vset, type);
		for_each_dev_in_vset(vset, dev)
			printf_device(dev, type);
	}
}

static void printf_hot_spare_disks(struct ipr_ioa *ioa, int type)
{
	struct ipr_dev *dev;

	for_each_hot_spare(ioa, dev)
		printf_device(dev, type);
}

static void printf_standlone_disks(struct ipr_ioa *ioa, int type)
{
	struct ipr_dev *dev;

	for_each_standalone_disk(ioa, dev)
		printf_device(dev, type);
}

static void printf_ioa(struct ipr_ioa *ioa, int type)
{
	if (!ioa->ioa.scsi_dev_data)
		return;

	printf_device(&ioa->ioa, type);
	printf_standlone_disks(ioa, type);
	printf_hot_spare_disks(ioa, type);
	printf_vsets(ioa, type);
}

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

	ioa = dev->ioa;

	if (!ioa->start_array_qac_entry)
		return 0;

	for_each_disk(ioa, dev) {
		if (!device_supported(dev))
			continue;
		if (ipr_is_af_dasd_device(dev) && !dev->dev_rcd->start_cand)
			continue;

		if (!num)
			printf("%s\n%s\n", status_hdr[2], status_sep[2]);
		__printf_device(dev, 1, 1, 1, 0, 0);
		num++;
	}

	if (num)
		printf("\b\n");

	return 0;
}

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

	ioa = dev->ioa;

	for_each_vset(ioa, dev) {
		if (!dev->array_rcd->stop_cand)
			continue;
		if (is_array_in_use(ioa, dev->array_rcd->array_id))
			continue;
		if (!num)
			printf("%s\n%s\n", status_hdr[3], status_sep[3]);
		printf_device(dev, 3);
		num++;
	}

	return 0;
}

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

static int query_raid_consistency_check(char **args, int num_args)
{
	struct ipr_dev *vset;
	struct ipr_ioa *ioa;
	int hdr = 0;

	for_each_ioa(ioa) {
		for_each_vset(ioa, vset) {
			if (!vset->array_rcd->resync_cand)
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

static int raid_consistency_check(char **args, int num_args)
{
	struct ipr_dev *vset = find_dev(args[0]);

	if (!vset) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	vset->array_rcd->issue_cmd = 1;
	return ipr_resync_array(vset->ioa);
}

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

	if (!is_spi(dev->ioa) || dev->ioa->is_aux_cache)
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

static int set_bus_speed(char **args, int num_args)
{
	int rc, max_xfer_rate, new_xfer_rate;
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

	if (!is_spi(dev->ioa) || dev->ioa->is_aux_cache)
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

	max_xfer_rate = get_max_bus_speed(ioa, bus);
	new_xfer_rate = IPR_BUS_THRUPUT_TO_XFER_RATE(speed,
						     page_28_cur.bus[bus].bus_width);

	if (new_xfer_rate > max_xfer_rate) {
		fprintf(stderr, "Max speed allowed: %d MB/sec\n",
			IPR_BUS_XFER_RATE_TO_THRUPUT(max_xfer_rate,
						     page_28_cur.bus[bus].bus_width));
		return -EINVAL;
	}

	page_28_cur.bus[bus].max_xfer_rate = new_xfer_rate;
	return ipr_set_bus_attr(ioa, &page_28_cur, 1);
}

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

	if (!is_spi(dev->ioa) || dev->ioa->is_aux_cache)
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

static struct ipr_dev *find_slot(struct ipr_dev **devs, int num_devs, char *slot)
{
	int i;
	char buf[100];
	struct ipr_res_addr res_addr;

	for (i = 0; i < num_devs; i++) {
		get_res_addr(devs[i], &res_addr);
		sprintf(buf, "%s/%d:%d:%d:", devs[i]->ioa->pci_address, devs[i]->ioa->host_num,
			res_addr.bus, res_addr.target);

		if (!strncmp(buf, slot, strlen(buf)))
			return devs[i];
	}

	return NULL;
}

static int __add_device(struct ipr_dev *dev, int on)
{
	struct ipr_dev *ses;
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status;
	struct ipr_res_addr res_addr;
	int rc, found;
	int time = 12;

	if (get_res_addr(dev, &res_addr)) {
		fprintf(stderr, "Invalid device\n");
		return -EINVAL;
	}

	for_each_ses(dev->ioa, ses) {
		rc = ipr_receive_diagnostics(ses, 2, &ses_data,
					     sizeof(struct ipr_encl_status_ctl_pg));

		if (rc || res_addr.bus != ses->scsi_dev_data->channel)
			continue;

		found = 0;
		for_each_elem_status(elem_status, &ses_data) {
			if (res_addr.target == elem_status->scsi_id) {
				found++;
				elem_status->select = 1;
				elem_status->remove = 0;
				elem_status->insert = on;
				elem_status->identify = 0;
				break;
			}
		}

		if (found)
			break;
	}

	if (!found) {
		scsi_err(dev, "Cannot find SES device for specified device\n");
		return -EINVAL;
	}

	ses_data.overall_status_select = 1;
	ses_data.overall_status_disable_resets = on;

	rc = ipr_send_diagnostics(ses, &ses_data,
				  sizeof(struct ipr_encl_status_ctl_pg));

	if (!on) {
		ipr_scan(dev->ioa, res_addr.bus, res_addr.target, res_addr.lun);
		while (time--) {
			check_current_config(false);
			if ((dev = get_dev_from_addr(&res_addr))) {
				ipr_init_new_dev(dev);
				break;
			}
			sleep(5);
		}
	}

	return rc;
}

static int __remove_device(struct ipr_dev *dev, int on)
{
	struct ipr_dev *ses;
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status;
	struct ipr_res_addr res_addr;
	int rc, found;

	if (get_res_addr(dev, &res_addr)) {
		fprintf(stderr, "Invalid device\n");
		return -EINVAL;
	}

	if (!on && ipr_is_af_dasd_device(dev)) {
		rc = ipr_suspend_device_bus(dev->ioa, &res_addr,
					    IPR_SDB_CHECK_ONLY);

		if (rc) {
			scsi_err(dev, "Cannot remove device\n");
			return -EINVAL;
		}
	}

	for_each_ses(dev->ioa, ses) {
		rc = ipr_receive_diagnostics(ses, 2, &ses_data,
					     sizeof(struct ipr_encl_status_ctl_pg));

		if (rc || res_addr.bus != ses->scsi_dev_data->channel)
			continue;

		found = 0;
		for_each_elem_status(elem_status, &ses_data) {
			if (res_addr.target == elem_status->scsi_id) {
				found++;
				elem_status->select = 1;
				elem_status->remove = on;
				elem_status->insert = 0;
				elem_status->identify = 0;
				break;
			}
		}

		if (found)
			break;
	}

	if (!found) {
		scsi_err(dev, "Cannot find SES device for specified device\n");
		return -EINVAL;
	}

	ses_data.overall_status_select = 1;
	ses_data.overall_status_disable_resets = on;

	rc = ipr_send_diagnostics(ses, &ses_data,
				  sizeof(struct ipr_encl_status_ctl_pg));

	if (!on) {
		ipr_write_dev_attr(dev, "delete", "1");
		evaluate_device(dev, dev->ioa, 0);
		ipr_del_zeroed_dev(dev);
	}

	return rc;
}

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

static int remove_disk(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	int on = strtoul(args[1], NULL, 10);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	return __remove_device(dev, on);
}

static int __identify_device(struct ipr_dev *dev, int on)
{
	struct ipr_dev *ses;
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_drive_elem_status *elem_status;
	struct ipr_res_addr res_addr;
	int rc, found;

	if (get_res_addr(dev, &res_addr)) {
		fprintf(stderr, "Invalid device\n");
		return -EINVAL;
	}

	for_each_ses(dev->ioa, ses) {
		rc = ipr_receive_diagnostics(ses, 2, &ses_data,
					     sizeof(struct ipr_encl_status_ctl_pg));

		if (rc || res_addr.bus != ses->scsi_dev_data->channel)
			continue;

		found = 0;
		for_each_elem_status(elem_status, &ses_data) {
			if (res_addr.target == elem_status->scsi_id) {
				found++;
				elem_status->select = 1;
				elem_status->identify = on;
				elem_status->insert = 0;
				elem_status->remove = 0;
				break;
			}
		}

		if (found)
			break;
	}

	if (!found) {
		scsi_err(dev, "Cannot find SES device for specified device\n");
		return -EINVAL;
	}

	rc = ipr_send_diagnostics(ses, &ses_data,
				  sizeof(struct ipr_encl_status_ctl_pg));

	return rc;
}

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

static int identify_disk(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	return __identify_device(dev, strtoul(args[1], NULL, 10));
}

static int set_log_level_cmd(char **args, int num_args)
{
	char buf[4];
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (&dev->ioa->ioa != dev) {
		fprintf(stderr, "Device is not an IOA\n");
		return -EINVAL;
	}

	snprintf(buf, sizeof(buf), "%s\n", args[1]);
	set_log_level(dev->ioa, buf);
	return 0;
}

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

static int set_qdepth(char **args, int num_args)
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

	attr.queue_depth = strtoul(args[1], NULL, 10);

	return ipr_set_dev_attr(dev, &attr, 1);
}

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

static int update_ioa_ucode(struct ipr_ioa *ioa, char *file)
{
	struct ipr_fw_images image;

	strcpy(image.file, file);
	image.version = get_ioa_ucode_version(file);
	image.has_header = 0;

	ipr_update_ioa_fw(ioa, &image, 1);

	if (image.version != get_ioa_fw_version(ioa))
		return -EIO;
	return 0;
}

static int update_dev_ucode(struct ipr_dev *dev, char *file)
{
	struct ipr_fw_images image;

	strcpy(image.file, file);
	image.version = get_dasd_ucode_version(file, 1);
	image.has_header = 1;

	ipr_update_disk_fw(dev, &image, 1);

	if (image.version != get_dev_fw_version(dev))
		return -EIO;
	return 0;
}

static int update_ucode_cmd(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (&dev->ioa->ioa == dev)
		return update_ioa_ucode(dev->ioa, args[1]);
	else
		return update_dev_ucode(dev, args[1]);
}

static int raid_rebuild_cmd(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	enable_af(dev);
	dev->dev_rcd->issue_cmd = 1;
	return ipr_rebuild_device_data(dev->ioa);
}

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

static int reclaim(char **args, int num_args)
{
	return __reclaim(args, num_args, IPR_RECLAIM_PERFORM);
}

static int reclaim_unknown(char **args, int num_args)
{
	return __reclaim(args, num_args,
			 IPR_RECLAIM_PERFORM | IPR_RECLAIM_UNKNOWN_PERM);
}

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

static int query_bus_width(char **args, int num_args)
{
	int rc, bus;
	struct ipr_scsi_buses page_28;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (!is_spi(dev->ioa) || dev->ioa->is_aux_cache)
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

static int query_bus_speed(char **args, int num_args)
{
	int rc, bus;
	struct ipr_scsi_buses page_28;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (!is_spi(dev->ioa) || dev->ioa->is_aux_cache)
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

static int query_initiator_id(char **args, int num_args)
{
	int rc, bus;
	struct ipr_scsi_buses page_28;
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Invalid device %s\n", args[0]);
		return -EINVAL;
	}

	if (!is_spi(dev->ioa) || dev->ioa->is_aux_cache)
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

static int query_add_remove(int action)
{
	int i, num_devs;
	struct ipr_dev **devs;

	num_devs = get_conc_devs(&devs, action);

	for (i = 0; i < num_devs; i++) {
		if (i == 0)
			printf("%s\n%s\n", status_hdr[3], status_sep[3]);

		printf_device(devs[i], 1);
	}

	free_empty_slots(devs, num_devs);
	free(devs);

	return 0;

}

static int query_remove_device(char **args, int num_args)
{
	return query_add_remove(IPR_CONC_REMOVE);
}

static int query_add_device(char **args, int num_args)
{
	return query_add_remove(IPR_CONC_ADD);
}

static int query_log_level(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (&dev->ioa->ioa != dev) {
		fprintf(stderr, "Device is not an IOA\n");
		return -EINVAL;
	}

	printf("%d\n", get_log_level(dev->ioa));
	return 0;
}

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

static int query_ucode_level(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);
	u32 level, level_sw;
	char *asc;

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", args[0]);
		return -EINVAL;
	}

	if (&dev->ioa->ioa == dev)
		printf("%08X\n", get_ioa_fw_version(dev->ioa));
	else {
		level = get_dev_fw_version(dev);
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

static int query_recovery_format(char **args, int num_args)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	int hdr = 0;

	for_each_ioa(ioa) {
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

	rc = ipr_query_array_config(ioa, 0, 1, vset->array_rcd->array_id, &qac_data);

	if (rc)
		return rc;

	for_each_dev_rcd(dev_rcd, &qac_data) {
		for_each_disk(ioa, dev) {
			if (dev->scsi_dev_data->handle == dev_rcd->resource_handle &&
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
						       array_rcd->raid_level);

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

static void show_dev_details(struct ipr_dev *dev)
{
	char *body = NULL;

	if (dev->scsi_dev_data && dev->scsi_dev_data->type == IPR_TYPE_ADAPTER)
		body = ioa_details(body, dev);
	else if (ipr_is_volume_set(dev))
		body = vset_details(body, dev);
	else
		body = disk_details(body, dev);

	printf("%s\n", body);
	free(body);
}

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

static int show_details(char **args, int num_args)
{
	struct ipr_dev *dev = find_dev(args[0]);

	dev = find_dev(args[0]);
	if (!dev) {
		fprintf(stderr, "Missing device: %s\n", args[0]);
		return -EINVAL;
	}

	show_dev_details(dev);
	return 0;
}

static int __print_status(char **args, int num_args, int percent)
{
	struct ipr_dev *dev = find_dev(args[0]);
	char buf[100];

	if (!dev) {
		printf("Missing\n");
		return -EINVAL;
	}

	get_status(dev, buf, percent);
	printf("%s\n", buf);
	return 0;
}

static int print_alt_status(char **args, int num_args)
{
	return __print_status(args, num_args, 1);
}

static int print_status(char **args, int num_args)
{
	return __print_status(args, num_args, 0);
}

static int __show_config(int type)
{
	struct ipr_ioa *ioa;

	printf("%s\n%s\n", status_hdr[(type&1)+2], status_sep[(type&1)+2]);
	for_each_ioa(ioa)
		printf_ioa(ioa, type);
	return 0;
}

static int show_config(char **args, int num_args)
{
	return __show_config(3);
}

static int show_alt_config(char **args, int num_args)
{
	return __show_config(2);
}

static int show_ioas(char **args, int num_args)
{
	struct ipr_ioa *ioa;

	if (num_ioas)
		printf("%s\n%s\n", status_hdr[2], status_sep[2]);
	for_each_ioa(ioa)
		printf_device(&ioa->ioa, 2);

	return 0;
}

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

static int set_primary(char **args, int num_args)
{
	return set_preferred_primary(args[0], 1);
}

static int set_secondary(char **args, int num_args)
{
	return set_preferred_primary(args[0], 0);
}

static const struct {
	char *cmd;
	int min_args;
	int unlimited_max;
	int max_args;
	int (*func)(char **, int);
	char *usage;
} command [] = {
	{ "show-config",				0, 0, 0, show_config, "" },
	{ "show-alt-config",			0, 0, 0, show_alt_config, "" },
	{ "show-ioas",				0, 0, 0, show_ioas, "" },
	{ "show-arrays",				0, 0, 0, show_arrays, "" },
	{ "show-battery-info",			1, 0, 1, battery_info, "sg5" },
	{ "show-details",				1, 0, 1, show_details, "sda" },
	{ "show-hot-spares",			0, 0, 0, show_hot_spares, "" },
	{ "show-af-disks",			0, 0, 0, show_af_disks, "" },
	{ "show-all-af-disks",			0, 0, 0, show_all_af_disks, "" },
	{ "show-jbod-disks",			0, 0, 0, show_jbod_disks, "" },
	{ "status",					1, 0, 1, print_status, "sda" },
	{ "alt-status",				1, 0, 1, print_alt_status, "sda" },
	{ "query-raid-create",			1, 0, 1, query_raid_create, "sg5" },
	{ "query-raid-delete",			1, 0, 1, query_raid_delete, "sg5" },
	{ "query-hot-spare-create",		1, 0, 1, query_hot_spare_create, "sg5" },
	{ "query-hot-spare-delete",		1, 0, 1, query_hot_spare_delete, "sg5" },
	{ "query-raid-consistency-check",	0, 0, 0, query_raid_consistency_check, "" },
	{ "query-format-for-jbod",		0, 0, 0, query_format_for_jbod, "" },
	{ "query-reclaim",			0, 0, 0, query_reclaim, "" },
	{ "query-arrays-raid-include",	0, 0, 0, query_arrays_include, "" },
	{ "query-devices-raid-include",	1, 0, 1, query_devices_include, "sdb" },
	{ "query-recovery-format",		0, 0, 0, query_recovery_format, "" },
	{ "query-raid-rebuild",			0, 0, 0, query_raid_rebuild, "" },
	{ "query-format-for-raid",		0, 0, 0, query_format_for_raid, "" },
	{ "query-ucode-level",			1, 0, 1, query_ucode_level, "sda" },
	{ "query-format-timeout",		1, 0, 1, query_format_timeout, "sg6" },
	{ "query-qdepth",				1, 0, 1, query_qdepth, "sda" },
	{ "query-tcq-enable",			1, 0, 1, query_tcq_enable, "sda" },
	{ "query-log-level",			1, 0, 1, query_log_level, "sg5" },
	{ "query-add-device",			0, 0, 0, query_add_device, "" },
	{ "query-remove-device",		0, 0, 0, query_remove_device, "" },
	{ "query-initiator-id",			2, 0, 2, query_initiator_id, "sg5 0" },
	{ "query-bus-speed",			2, 0, 2, query_bus_speed, "sg5 0" },
	{ "query-bus-width",			2, 0, 2, query_bus_width, "sg5 0" },
	{ "query-supported-raid-levels",	1, 0, 1, query_raid_levels, "sg5" },
	{ "query-include-allowed",		2, 0, 2, query_include_allowed, "sg5 5" },
	{ "query-max-devices-in-array",	2, 0, 2, query_max_array_devices, "sg5 5" },
	{ "query-min-devices-in-array",	2, 0, 2, query_min_array_devices, "sg5 5" },
	{ "query-min-mult-in-array",		2, 0, 2, query_min_mult_in_array, "sg5 5" },
	{ "query-supp-stripe-sizes",		2, 0, 2, query_supp_stripe_sizes, "sg5 5" },
	{ "query-recommended-stripe-size",	2, 0, 2, query_recommended_stripe_size, "sg5 5" },
	{ "primary",				1, 0, 1, set_primary, "sg5" },
	{ "secondary",				1, 0, 1, set_secondary, "sg5" },
	{ "raid-create",				1, 1, 0, raid_create, "-r 5 -s 64 sda sdb sg6 sg7" },
	{ "raid-delete",				1, 0, 1, raid_delete, "sdb" },
	{ "raid-include",				2, 0, 17, raid_include_cmd, "sda sg6 sg7" },
	{ "format-for-raid",			1, 1, 0, format_for_raid, "sda sdb sdc" },
	{ "format-for-jbod",			1, 1, 0, format_for_jbod, "sg6 sg7 sg8" },
	{ "recovery-format",			1, 1, 0, recovery_format, "sda sg7" },
	{ "hot-spare-create",			1, 0, 1, hot_spare_create, "sg6" },
	{ "hot-spare-delete",			1, 0, 1, hot_spare_delete, "sg6" },
	{ "reclaim-cache",			1, 0, 1, reclaim, "sg5" },
	{ "reclaim-unknown-cache",		1, 0, 1, reclaim_unknown, "sg5" },
	{ "force-cache-battery-error",	1, 0, 1, force_cache_battery_error, "sg5" },
	{ "start-ioa-cache",			1, 0, 1, start_ioa_cache, "sg5" },
	{ "raid-consistency-check",		1, 0, 1, raid_consistency_check, "sg5" },
	{ "raid-rebuild",				1, 0, 1, raid_rebuild_cmd, "sg6" },
	{ "update-ucode",				2, 0, 2, update_ucode_cmd, "sg5 /root/ucode.bin" },
	{ "set-format-timeout",			2, 0, 2, set_format_timeout, "sg6 4" },
	{ "set-qdepth",				2, 0, 2, set_qdepth, "sda 16" },
	{ "set-tcq-enable",			2, 0, 2, set_tcq_enable, "sda 0" },
	{ "set-log-level",			2, 0, 2, set_log_level_cmd, "sg5 2" },
	{ "identify-disk",			2, 0, 2, identify_disk, "sda 1" },
	{ "identify-slot",			2, 0, 2, identify_slot, "0000:d8:01.0/0:1:1: 1" },
	{ "remove-disk",				2, 0, 2, remove_disk, "sda 1" },
	{ "remove-slot",				2, 0, 2, remove_slot, "0000:d8:01.0/0:1:1: 1" },
	{ "add-slot",				2, 0, 2, add_slot, "0000:d8:01.0/0:1:1: 1" },
	{ "set-initiator-id",			3, 0, 3, set_initiator_id, "sg5 0 7" },
	{ "set-bus-speed",			3, 0, 3, set_bus_speed, "sg5 0 320" },
	{ "set-bus-width",			3, 0, 3, set_bus_width, "sg5 0 16" },
};

static int non_interactive_cmd(char *cmd, char **args, int num_args)
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
			return -EINVAL;
		}

		if (!command[i].unlimited_max && num_args > command[i].max_args) {
			fprintf(stderr, "Too many arguments specified.\n");
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

int main(int argc, char *argv[])
{
	int  next_editor, next_dir, next_cmd, next_args, i, rc = 0;
	char parm_editor[200], parm_dir[200], cmd[200];
	int non_interactive = 0;

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
			else if (strcmp(argv[i], "-e") == 0)
				next_editor = 1;
			else if (strcmp(argv[i], "-k") == 0)
				next_dir = 1;
			else if (strcmp(argv[i], "-c") == 0)
				next_cmd = 1;
			else if (next_editor)	{
				strcpy(parm_editor, argv[i]);
				next_editor = 0;
			} else if (next_dir) {
				strcpy(parm_dir, argv[i]);
				next_dir = 0;
			} else if (next_cmd) {
				strcpy(cmd, argv[i]);
				non_interactive = 1;
				next_cmd = 0;
				next_args = 1;
			} else if (next_args) {
				add_args = realloc(add_args, sizeof(*add_args) * (num_add_args + 1));
				add_args[num_add_args] = malloc(strlen(argv[i]) + 1);
				strcpy(add_args[num_add_args], argv[i]);
				num_add_args++;
			} else {
				usage();
				exit(1);
			}
		}
	}

	strcpy(log_root_dir, parm_dir);
	strcpy(editor, parm_editor);

	system("modprobe sg");
	exit_func = tool_exit_func;
	tool_init(0);

	if (non_interactive)
		return non_interactive_cmd(cmd, add_args, num_add_args);

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

 		n_exit_confirm.body = body_init(n_exit_confirm.header, NULL);

		for (i = 0; i < n_exit_confirm.num_opts; i++) {
			n_exit_confirm.body = ipr_list_opts(n_exit_confirm.body,
							    n_exit_confirm.options[i].key,
							    n_exit_confirm.options[i].list_str);
		}

		n_exit_confirm.body = ipr_end_list(n_exit_confirm.body);

		s_out = screen_driver(&n_exit_confirm,0,NULL);
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

