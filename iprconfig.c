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

struct devs_to_init_t {
	struct ipr_dev       *ipr_dev;
	struct ipr_ioa       *ioa;
	int                   change_size;
#define IPR_522_BLOCK 522
#define IPR_512_BLOCK 512
	int                   cmplt;
	int                   do_init;
	int                   dev_type;
#define IPR_AF_DASD_DEVICE      1
#define IPR_JBOD_DASD_DEVICE    2
	struct devs_to_init_t *next;
};

struct prot_level {
	struct ipr_array_cap_entry *array_cap_entry;
	u8                          is_valid_entry:1;
	u8                          reserved:7;
};

struct array_cmd_data {
	u8                           array_id;
	u8                           prot_level;
	u16                          stripe_size;
	u32                          do_cmd;
	struct ipr_ioa              *ipr_ioa;
	struct ipr_dev              *ipr_dev;
	struct ipr_array_query_data *qac_data;
	struct array_cmd_data       *next;
};

/* not needed once menus incorporated into screen_driver */
struct window_l {
	WINDOW          *win;
	struct window_l *next;
};

struct panel_l {
	PANEL *panel;
	struct panel_l *next;
};

struct devs_to_init_t *dev_init_head = NULL;
struct devs_to_init_t *dev_init_tail = NULL;
struct array_cmd_data *raid_cmd_head = NULL;
struct array_cmd_data *raid_cmd_tail = NULL;
i_container *i_con_head = NULL;
char                   log_root_dir[200];
char                   editor[200];
char                   ipr_catalog[200];
FILE                  *errpath;
int                    toggle_field;
nl_catd                catd;

#define DEFAULT_LOG_DIR "/var/log"
#define DEFAULT_EDITOR "vi -R"
#define FAILSAFE_EDITOR "vi -R -"

#define IS_CANCEL_KEY(c) ((c == KEY_F(4)) || (c == 'q') || (c == 'Q'))
#define CANCEL_KEY_LABEL "q=Cancel   "
#define IS_EXIT_KEY(c) ((c == KEY_F(3)) || (c == 'e') || (c == 'E'))
#define EXIT_KEY_LABEL "e=Exit   "
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

i_container *free_i_con(i_container *x_con);
i_container *add_i_con(i_container *x_con, char *f, void *d);

struct special_status {
	int   index;
	int   num;
	char *str;
} s_status;

struct screen_output *screen_driver(s_node *screen,
				    int header_lines,
				    i_container *i_con);
char *print_device(struct ipr_dev *ipr_dev,
		   char *body,
		   char *option,
		   struct ipr_ioa *cur_ioa,
		   int type);
int is_format_allowed(struct ipr_dev *ipr_dev);
int is_rw_protected(struct ipr_dev *ipr_dev);
int select_log_file(const struct dirent *dir_entry);
int compare_log_file(const void *log_file1,
		     const void *log_file2);
void evaluate_device(struct ipr_dev *ipr_dev,
		     struct ipr_ioa *ioa,
		     int change_size);
int is_array_in_use(struct ipr_ioa *cur_ioa,
		    u8 array_id);
int display_menu(ITEM **menu_item,
		 int menu_start_row,
		 int menu_index_max,
		 int **userptr);
void free_screen(struct panel_l *panel,
		 struct window_l *win,
		 FIELD **fields);
void flush_stdscr();
int flush_line();

void *ipr_realloc(void *buffer, int size)
{
	void *new_buffer = realloc(buffer, size);

	/*    syslog(LOG_ERR, "realloc from %lx to %lx of length %d\n",
	 buffer, new_buffer, size);
	 */    return new_buffer;
}

void ipr_free(void *buffer)
{
	free(buffer);
	/*    syslog(LOG_ERR, "free %lx\n", buffer);
	 */}

void *ipr_malloc(int size)
{
	void *new_buffer = malloc(size);

	/*    syslog(LOG_ERR, "malloc %lx of length %d\n", new_buffer, size);
	 */    return new_buffer;
}

char *strip_trailing_whitespace(char *p_str)
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

int main(int argc, char *argv[])
{
	int  next_editor, next_dir, next_catalog, i;
	char parm_editor[200], parm_dir[200], catalog[200];
	char ipr_catalog[200];

	ipr_ioa_head = ipr_ioa_tail = NULL;
	catalog[0] = '\0';

	/* makes program compatible with all terminals -
	 originally did not display text correctly when user was running xterm */
	setenv("TERM", "vt100", 1);

	setlocale(LC_ALL, "");

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
		next_catalog = 0;
		for (i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-e") == 0)
				next_editor = 1;
			else if (strcmp(argv[i], "-k") == 0)
				next_dir = 1;
			else if (strcmp(argv[i], "-c") == 0)
				next_catalog = 1;
			else if (strcmp(argv[i], "--version") == 0) {
				printf("iprconfig: %s\n", IPR_VERSION_STR);
				exit(1);
			} else if (next_editor)	{
				strcpy(parm_editor, argv[i]);
				next_editor = 0;
			} else if (next_dir) {
				strcpy(parm_dir, argv[i]);
				next_dir = 0;
			} else if (next_catalog) {
				strcpy(catalog, argv[i]);
				next_catalog = 0;
			} else {
				printf("Usage: iprconfig [options]\n");
				printf("  Options: -e name    Default editor for viewing error logs\n");
				printf("           -k dir     Kernel messages root directory\n");
				printf("           -c catalog Catalog file to use\n");
				printf("           --version  Print iprconfig version\n");
				printf("  Use quotes around parameters with spaces\n");
				exit(1);
			}
		}
	}

	strcpy(log_root_dir, parm_dir);
	strcpy(editor, parm_editor);
	strcpy(ipr_catalog, catalog);

	tool_init("iprconfig");

	initscr();
	cbreak(); /* take input chars one at a time, no wait for \n */

	keypad(stdscr,TRUE);
	noecho();

	s_status.index = 0;

	/* open the text catalog */
	if (strlen(ipr_catalog) != 0)
		sprintf(ipr_catalog, "%s",ipr_catalog);
	else
		sprintf(ipr_catalog, "%s/ipr.cat",IPR_CATALOG_DIR);

	catd=catopen(ipr_catalog,NL_CAT_LOCALE);

	main_menu(NULL);

	clear();
	refresh();
	endwin();
	exit(0);
	return 0;
}

struct screen_output *screen_driver(s_node *screen, int header_lines, i_container *i_con)
{
	WINDOW *w_pad,*w_page_header;              /* windows to hold text */
	FIELD **fields = NULL;                     /* field list for forms */
	FORM *form = NULL;                       /* form for input fields */
	int rc = 0;                            /* the status of the screen */
	char *status;                            /* displays screen operations */
	char buffer[100];                       /* needed for special status strings */
	bool invalid = false;                   /* invalid status display */

	bool t_on=false,e_on=false,q_on=false,r_on=false,f_on=false,m_on=false,enter_on=false; /* control user input handling */
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
	char *input;
	struct screen_output *s_out;
	struct screen_opts   *temp;                 /* reference to another screen */

	char *title,*body,*footer,*body_text,more_footer[100]; /* screen text */
	i_container *temp_i_con;
	int num_i_cons = 0;
	int x, y;
	s_out = ipr_malloc(sizeof(struct screen_output)); /* passes an i_con and an rc value back to the function */

	/* create text strings */
	title = screen->title;
	body = screen->body;
	footer = screen->footer;

	/* calculate the footer size and turn any special characters on*/
	i=0,j=0;
	while(footer[i] != '\0') {
		if (footer[i] == '%') {
			i++;
			switch (footer[i]) {
			case 'n':
				footer_lines += 3; /* add 3 lines to the footer */
				enter_on = true;
				break;
			case 'e':
				e_on = true; /* exit */
				break;
			case 'q':
				q_on = true; /* cancel */
				break;
			case 'r':
				r_on = true; /* refresh */
				break;
			case 'f':
				f_on = true; /* page up/down */
				break;
			case 'm':
				m_on = true; /* menu on */
				break;
			case 't':
				t_on = true; /* toggle page data */
				active_field = toggle_field;
				break;
			}
		} else {
			if (footer[i] == '\n')
				footer_lines++;
			more_footer[j++] = footer[i];
		}
		i++;
	}

	/* add null char to end of string */
	more_footer[j] = '\0';

	if (f_on && !enter_on)
		footer_lines++;

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
			fields = ipr_realloc(fields,sizeof(FIELD **) * (num_fields + 1));
			i++;

			field_width = 0;
			sscanf(&(body[i]),"%d",&field_width);

			fields[num_fields] = new_field(1,field_width,row,col,0,0);

			if (field_width >= 30)
				field_opts_off(fields[num_fields],O_AUTOSKIP);

			if (field_width > 9)
				/* skip second digit of field index */
				i++; 

			if ((temp_i_con) && (temp_i_con->field_data[0]) && m_on) {
				set_field_buffer(fields[num_fields], 0, temp_i_con->field_data);
				input = field_buffer(fields[num_fields],0);
				temp_i_con = temp_i_con->next_item;
				num_i_cons++;
			}

			set_field_userptr(fields[num_fields],NULL);
			num_fields++;
			col += field_width-1;

			bt_len += field_width;

			body_text = ipr_realloc(body_text,bt_len);
			for(k=0;k<field_width;k++)
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
		fields = ipr_realloc(fields,sizeof(FIELD **) * (num_fields + 1));
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
		if ((stdscr_max_y - title_lines - footer_lines + 1) > w_pad_max_y) {
			viewable_body_lines = w_pad_max_y;
			pad_t = 0;
			y_offscr = false; /*not scrollable*/
			pages = false;
			for (i = 0; i < num_fields; i++)
				field_opts_on(fields[i],O_ACTIVE);
		} else {
			viewable_body_lines = stdscr_max_y - title_lines - footer_lines - 1;
			y_offscr = true; /*scrollable*/
			if (f_on) {
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

		if (enter_on) {
			if (num_fields == 0) {
				addstr("\nPress Enter to Continue\n\n");
				ground_cursor = true;
			} else
				addstr("\nOr leave blank and press Enter to cancel\n\n");
		}

		if (f_on && !enter_on)
			addstr("\n");
		addstr(more_footer);
		if (e_on)
			addstr(EXIT_KEY_LABEL);
		if (q_on)
			addstr(CANCEL_KEY_LABEL);
		if (r_on)
			addstr(REFRESH_KEY_LABEL);
		if (t_on)
			addstr(TOGGLE_KEY_LABEL);
		if (f_on && y_offscr) {
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
				status = catgets(catd,STATUS_SET,
						 INVALID_OPTION_STATUS,"Status Return Error (invalid)");
				invalid = false;
			} else if (rc != 0) {
				if ((status = strchr(catgets(catd, STATUS_SET,
							     rc,"default"),'%')) == NULL) {
					status = catgets(catd, STATUS_SET,
							 rc,"Status Return Error (rc != 0, w/out %)");
				} else if (status[1] == 'd') {
					sprintf(buffer,catgets(catd,STATUS_SET,
							       rc,"Status Return Error (rc != 0 w/ %%d = %d"),s_status.num);
					status = buffer;
				} else if (status[1] == 's') {
					sprintf(buffer,catgets(catd,STATUS_SET,
							       rc,"Status Return Error rc != 0 w/ %%s = %s"),s_status.str);
					status = buffer;
				}
				rc = 0;
			} else
				status = NULL;

			move(stdscr_max_y-1,0);
			clrtoeol(); /* clear last line */
			mvaddstr(stdscr_max_y-1,0,status);
			status = NULL;

			if (f_on && y_offscr) {
				if (!is_bottom)
					mvaddstr(stdscr_max_y-footer_lines,stdscr_max_x-8,"More...");
				else
					mvaddstr(stdscr_max_y-footer_lines,stdscr_max_x-8,"       ");

				for (i=0;i<num_fields;i++) {
					if (fields[i] != NULL) {
						field_info(fields[i],&h,&w,&t,&l,&o,&b); /* height, width, top, left, offscreen rows, buffer */

						/* turn all offscreen fields off */
						if ((t >= (header_lines + pad_t)) &&
						    (t <= (viewable_body_lines + pad_t))) {

							field_opts_on(fields[i],O_ACTIVE);
							if (form_adjust) {
								if (!t_on || !toggle_field)
									set_current_field(form,fields[i]);
								form_adjust = false;
							}
						} else
							field_opts_off(fields[i],O_ACTIVE);
					}
				}
			}  

			if (m_on) {
				for (i=0;i<num_fields;i++) {
					field_opts_off(fields[i],O_VISIBLE);
					field_opts_on(fields[i],O_VISIBLE);
					set_field_fore(fields[i],A_BOLD);
				}
			}

			if (t_on && toggle_field &&
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
				if (num_fields > 1)
					active_field = field_index(current_field(form));

				if (enter_on && num_fields == 0) {

					rc = (CANCEL_FLAG | rc);
					goto leave;
				} else if (enter_on) {

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
			} else if (IS_EXIT_KEY(ch) && e_on) {
				rc = (EXIT_FLAG | rc);
				goto leave;
			} else if (IS_CANCEL_KEY(ch) && (q_on || screen == &n_main_menu))	{
				rc = (CANCEL_FLAG | rc);
				goto leave;
			} else if ((IS_REFRESH_KEY(ch) && r_on) || (ch == KEY_HOME && r_on)) {
				rc = REFRESH_SCREEN;
				if (num_fields > 1)
					toggle_field = field_index(current_field(form)) + 1;
				goto leave;
			} else if (IS_TOGGLE_KEY(ch) && t_on) {
				rc = TOGGLE_SCREEN;
				if (num_fields > 1)
					toggle_field = field_index(current_field(form)) + 1;
				goto leave;
			} else if (IS_PGUP_KEY(ch) && f_on && y_offscr)	{
				invalid = false;
				if (pad_t > 0) {
					pad_t -= (viewable_body_lines - header_lines + 1);
					rc = PGUP_STATUS;
				} else if (f_on && y_offscr)
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
			} else if (IS_PGDN_KEY(ch) && f_on && y_offscr)	{
				invalid = false;
				if ((stdscr_max_y + pad_t) < (w_pad_max_y + title_lines + footer_lines)) {
					pad_t += (viewable_body_lines - header_lines + 1);
					rc = PGDN_STATUS;

					if (!(stdscr_max_y + pad_t < w_pad_max_y + title_lines + footer_lines))
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
			else if ((m_on) || (num_fields == 0)) {
				invalid = true;

				for (i = 0; i < screen->num_opts; i++) {
					temp = &(screen->options[i]);

					if ((temp->key) && (ch == temp->key[0])) {
						invalid = false;

						if (num_fields > 1) {
							active_field = field_index(current_field(form));
							input = field_buffer(fields[active_field],0);

							if (active_field < num_i_cons) {
								temp_i_con = i_con_head;
								for (j = 0; j < active_field; j++)
									temp_i_con = temp_i_con->next_item;

								temp_i_con->field_data[0] = '\0';
							}
							toggle_field = field_index(current_field(form)) + 1;
						}

						getyx(w_pad,y,x);
						temp_i_con->y = y;
						temp_i_con->x = x;

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
	ipr_free(body_text);
	unpost_form(form);
	free_form(form);

	for (i=0;i < num_fields;i++) {
		if (fields[i] != NULL)
			free_field(fields[i]);
	}
	delwin(w_pad);
	delwin(w_page_header);
	return s_out;
}

int complete_screen_driver(s_node *n_screen, int percent, int allow_exit)
{
	int stdscr_max_y,stdscr_max_x,center,len;
	char *complete;
	char *percent_comp;
	int ch;
	char *exit, *exit_str;

	getmaxyx(stdscr,stdscr_max_y,stdscr_max_x);
	clear();
	center = stdscr_max_x/2;
	len = strlen(n_screen->title);

	/* add the title at the center of stdscr */
	mvaddstr(0,(center - len/2) > 0 ? center-len/2:0, n_screen->title);
	mvaddstr(2,0,n_screen->body);

	complete = catgets(catd,0,3,"Complete");
	percent_comp = malloc(strlen(complete) + 8);
	sprintf(percent_comp,"%3d%% %s",percent, complete);
	len = strlen(percent_comp);
	mvaddstr(stdscr_max_y/2,(center - len/2) > 0 ? center - len/2:0,
		 percent_comp);

	if (allow_exit) {
		exit = catgets(catd,0,4,
			       "to return to menu, current operations will continue");
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

char *ipr_list_opts(char *body, char *key, int catalog_set, int catalog_num)
{
	char *string_buf;
	int   start_len = 0;

	if (body == NULL) {
		string_buf = catgets(catd,0,1,"Select one of the following");
		body = ipr_realloc(body, strlen(string_buf) + 8);
		sprintf(body, "\n%s:\n\n", string_buf);
	}

	start_len = strlen(body);

	string_buf = catgets(catd, catalog_set, catalog_num, "" );

	body = ipr_realloc(body, start_len + strlen(key) + strlen(string_buf) + 16);
	sprintf(body + start_len, "    %s. %s\n", key, string_buf);
	return body;
}

char *ipr_end_list(char *body)
{
	int   start_len = 0;
	char *string_buf;

	start_len = strlen(body);

	string_buf = catgets(catd,0,2,"Selection");
	body = ipr_realloc(body, start_len + strlen(string_buf) + 16);
	sprintf(body + start_len, "\n\n%s: %%1", string_buf);
	return body;
}

int main_menu(i_container *i_con)
{
	int rc;
	struct ipr_cmd_status cmd_status;
	int j;
	struct ipr_ioa *cur_ioa;
	u8 ioctl_buffer[IPR_MODE_SENSE_LENGTH];
	struct sense_data_t sense_data;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	struct ipr_block_desc *block_desc;
	int loop, retries;
	struct screen_output *s_out;
	char *string_buf;
	mvaddstr(0,0,"MAIN MENU FUNCTION CALLED");

	check_current_config(false);

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {

		/* Do a query command status for all devices to check for formats in
		 progress */
		for (j = 0; j < cur_ioa->num_devices; j++) {

			scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;

			if ((scsi_dev_data == NULL) ||
			    (scsi_dev_data->type == IPR_TYPE_ADAPTER))
				continue;

			if (ipr_is_af(&cur_ioa->dev[j])) {

				/* Send Test Unit Ready to start device if its a volume set */
				if (ipr_is_volume_set(&cur_ioa->dev[j])) {
					ipr_test_unit_ready(&cur_ioa->dev[j], &sense_data);
					continue;
				}

				/* Do a query command status */
				rc = ipr_query_command_status(&cur_ioa->dev[j], &cmd_status);

				if (rc != 0)
					continue;

				if ((cmd_status.num_records != 0) &&
				    (cmd_status.record->status == IPR_CMD_STATUS_SUCCESSFUL)) {

					/* Check if evaluate device capabilities is necessary */
					/* Issue mode sense */
					rc = ipr_mode_sense(&cur_ioa->dev[j], 0, ioctl_buffer);

					if (rc == 0) {
						mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

						if (mode_parm_hdr->block_desc_len > 0) {

							block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

							if((block_desc->block_length[1] == 0x02) && 
							   (block_desc->block_length[2] == 0x00))   {

								/* Send evaluate device capabilities */
								evaluate_device(&cur_ioa->dev[j], cur_ioa, IPR_512_BLOCK);
							}
						}
					}
				}
			} else {
				/* JBOD */
				retries = 0;

				redo_tur:

					/* Send Test Unit Ready to find percent complete in sense data. */
					rc = ipr_test_unit_ready(&cur_ioa->dev[j], &sense_data);

				if ((rc == CHECK_CONDITION) &&
				    ((sense_data.error_code & 0x7F) == 0x70) &&
				    ((sense_data.sense_key & 0x0F) == 0x02) &&  /* NOT READY */
				    (sense_data.add_sense_code == 0x04) &&      /* LOGICAL UNIT NOT READY */
				    (sense_data.add_sense_code_qual == 0x02))   /* INIT CMD REQ */
				{
					rc = ipr_start_stop_start(&cur_ioa->dev[j]);

					if (retries == 0) {
						retries++;
						goto redo_tur;
					}
				} else if (rc != CHECK_CONDITION) {
					/* Check if evaluate device capabilities needs to be issued */
					/* Issue mode sense to get the block size */
					rc = ipr_mode_sense(&cur_ioa->dev[j], 0x0a, &ioctl_buffer);

					if (rc == 0) {
						mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

						if (mode_parm_hdr->block_desc_len > 0) {
							block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

							if ((!(block_desc->block_length[1] == 0x02) ||
							     !(block_desc->block_length[2] == 0x00)) &&
							    (cur_ioa->qac_data->num_records != 0)) {

								/* send evaluate device */
								evaluate_device(&cur_ioa->dev[j], cur_ioa, IPR_522_BLOCK);
							}
						}
					}
				}
			}
		}
	}

	string_buf = catgets(catd,n_main_menu.text_set,1,"Work with Disk Units");
	n_main_menu.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_main_menu.title, string_buf);

	for (loop = 0; loop < n_main_menu.num_opts; loop++) {
		n_main_menu.body = ipr_list_opts(n_main_menu.body,
						 n_main_menu.options[loop].key,
						 n_main_menu.text_set,
						 n_main_menu.options[loop].cat_num);
	}
	n_main_menu.body = ipr_end_list(n_main_menu.body);

	s_out = screen_driver(&n_main_menu,0,NULL);
	ipr_free(n_main_menu.body);
	n_main_menu.body = NULL;
	ipr_free(n_main_menu.title);
	n_main_menu.title = NULL;
	rc = s_out->rc;
	i_con = s_out->i_con;
	i_con = free_i_con(i_con);
	ipr_free(s_out);
	return rc;
}

char *status_header(char *buffer, int *num_lines, int type)
{
	int cur_len = strlen(buffer);
	int header_lines = 0;
	char *header[] = {
		/*   .        .                  .            .                           .          */
		/*0123456789012345678901234567890123456789012345678901234567890123456789901234567890 */
		"OPT Name   PCI/SCSI Location          Vendor   Product ID       Status",
		"OPT Name   PCI/SCSI Location          Description               Status"};
	char *sep[]    = {
		"--- ------ -------------------------- -------- ---------------- -----------------",
		"--- ------ -------------------------- ------------------------- -----------------"};

	if (type > 1)
		type = 0;
	buffer = ipr_realloc(buffer, cur_len + strlen(header[type]) + strlen(sep[type]) + 8);
	cur_len += sprintf(buffer + cur_len, "%s\n", header[type]);
	cur_len += sprintf(buffer + cur_len, "%s\n", sep[type]);
	header_lines += 2;

	*num_lines = header_lines + *num_lines;
	return buffer;
}

char *get_prot_level_str(struct ipr_supported_arrays *supported_arrays,
			 int prot_level)
{
	int i;
	struct ipr_array_cap_entry *cur_array_cap_entry;

	cur_array_cap_entry = (struct ipr_array_cap_entry *)supported_arrays->data;

	for (i=0; i<ntohs(supported_arrays->num_entries); i++) {
		if (cur_array_cap_entry->prot_level == prot_level)
			return cur_array_cap_entry->prot_level_str;

		cur_array_cap_entry = (struct ipr_array_cap_entry *)
			((void *)cur_array_cap_entry + ntohs(supported_arrays->entry_length));
	}

	return NULL;
}

/* body_init provides the basic header to the screen and offers
   one selection type formatted line.
   Requirements:  (max_index - min_index) > 0 */
char *body_init(s_node *n_screen, int *num_lines, int min_index,
		int max_index, char *opt_str, int type)
{
	char *string_buf;
	char *buffer = NULL;
	int cur_len;
	int header_lines = 0;
	int i;

	string_buf = catgets(catd,n_screen->text_set, min_index,
			     "Select the desired entry.");
	buffer = ipr_realloc(buffer, strlen(string_buf) + 4);
	cur_len = sprintf(buffer, "%s\n", string_buf);
	header_lines += 1;

	for (i = (min_index + 1); i < max_index; i++) {
		string_buf = catgets(catd,n_screen->text_set, i,
				     "Type choice, press Enter.");
		buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 4);
		cur_len += sprintf(buffer + cur_len, "\n%s\n", string_buf);
		header_lines += 2;
	}

	string_buf = catgets(catd,n_screen->text_set, max_index,"Select");
	buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
	cur_len += sprintf(buffer + cur_len, "  %s%s\n\n", opt_str,string_buf);
	header_lines += 2;

	buffer = status_header(buffer, &header_lines, type);

	*num_lines = header_lines;
	return buffer;
}

/* body_init_double provides 2 options for selection.
   Requirements:  (max_index - min_index) > 1 */
char *body_init_double(s_node *n_screen, int *num_lines, int min_index,
		       int max_index, char *opt_str1, char *opt_str2, int type)
{
	char *string_buf;
	char *buffer = NULL;
	int cur_len;
	int header_lines = 0;
	int i;

	string_buf = catgets(catd,n_screen->text_set, min_index,
			     "Select the desired entry.");
	buffer = ipr_realloc(buffer, strlen(string_buf) + 4);
	cur_len = sprintf(buffer, "%s\n", string_buf);
	header_lines += 1;

	for (i = (min_index + 1); i < (max_index - 1); i++) {
		string_buf = catgets(catd,n_screen->text_set, i,
				     "Type choice, press Enter.");
		buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 4);
		cur_len += sprintf(buffer + cur_len, "\n%s\n", string_buf);
		header_lines += 2;
	}

	string_buf = catgets(catd,n_screen->text_set, (max_index - 1),"Select");
	buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
	cur_len += sprintf(buffer + cur_len, "  %s%s  ", opt_str1,string_buf);
	string_buf = catgets(catd,n_screen->text_set, max_index,"Select");
	buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
	cur_len += sprintf(buffer + cur_len, "  %s%s\n\n", opt_str2,string_buf);
	header_lines += 2;

	buffer = status_header(buffer, &header_lines, type);

	*num_lines = header_lines;
	return buffer;
}

int disk_status(i_container *i_con)
{
	int rc, j, i, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *cur_ioa;
	struct scsi_dev_data *scsi_dev_data;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int array_id;
	char *buffer[2];
	int toggle = 1;
	struct ipr_dev_record *dev_record;
	struct ipr_array_record *array_record;
	char *prot_level_str;
	mvaddstr(0,0,"DISK STATUS FUNCTION CALLED");

	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_disk_status.text_set,1,"Display Disk Hardware Status");
	n_disk_status.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_disk_status.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_disk_status, &header_lines, 2, 3, "5=", k);

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {

		if (cur_ioa->ioa.scsi_dev_data == NULL)
			continue;

		for (k=0; k<2; k++)
			buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"%1", cur_ioa, k);

		i_con = add_i_con(i_con,"\0",&cur_ioa->ioa);

		num_lines++;

		/* print JBOD and non-member AF devices*/
		for (j = 0; j < cur_ioa->num_devices; j++) {
			scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
			if ((scsi_dev_data == NULL) ||
			    (scsi_dev_data->type != TYPE_DISK) ||
			    (ipr_is_hot_spare(&cur_ioa->dev[j])) ||
			    (ipr_is_volume_set(&cur_ioa->dev[j])) ||
			    (ipr_is_array_member(&cur_ioa->dev[j])))

				continue;

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[j],buffer[k], "%1", cur_ioa, k);

			i_con = add_i_con(i_con,"\0",&cur_ioa->dev[j]);  

			num_lines++;
		}

		/* print Hot Spare devices*/
		for (j = 0; j < cur_ioa->num_devices; j++) {
			scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
			if ((scsi_dev_data == NULL ) ||
			    (!ipr_is_hot_spare(&cur_ioa->dev[j])))

				continue;

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[j],buffer[k], "%1", cur_ioa, k);

			i_con = add_i_con(i_con,"\0",&cur_ioa->dev[j]);  

			num_lines++;
		}

		/* print volume set and associated devices*/
		for (j = 0; j < cur_ioa->num_devices; j++) {
			if (!ipr_is_volume_set(&cur_ioa->dev[j]))
				continue;

			array_record = (struct ipr_array_record *)
				cur_ioa->dev[j].qac_entry;

			if (array_record->start_cand)
				continue;

			/* query resource state to acquire protection level string */
			prot_level_str = get_prot_level_str(cur_ioa->supported_arrays,
							    array_record->raid_level);
			strncpy(cur_ioa->dev[j].prot_level_str,prot_level_str, 8);

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[j],buffer[k], "%1", cur_ioa, k);

			i_con = add_i_con(i_con,"\0",&cur_ioa->dev[j]);

			dev_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
			array_id = dev_record->array_id;
			num_lines++;

			for (i = 0; i < cur_ioa->num_devices; i++) {
				dev_record = (struct ipr_dev_record *)cur_ioa->dev[i].qac_entry;

				if (!(ipr_is_array_member(&cur_ioa->dev[i])) ||
				    (ipr_is_volume_set(&cur_ioa->dev[i])) ||
				    ((dev_record != NULL) &&
				     (dev_record->array_id != array_id)))
					continue;

				strncpy(cur_ioa->dev[i].prot_level_str, prot_level_str, 8);

				for (k=0; k<2; k++)
					buffer[k] = print_device(&cur_ioa->dev[i],buffer[k], "%1", cur_ioa, k);

				i_con = add_i_con(i_con,"\0",&cur_ioa->dev[i]);
				num_lines++;
			}
		}
	}


	if (num_lines == 0) {
		string_buf = catgets(catd,n_disk_status.text_set,4,
				     "No devices found");
		len = strlen(buffer[i]);
		buffer[i] = ipr_realloc(buffer[i], len +
					strlen(string_buf) + 4);
		sprintf(buffer[i] + len, "\n%s", string_buf);
	}

	do {
		n_disk_status.body = buffer[toggle&1];
		s_out = screen_driver(&n_disk_status,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_disk_status.body = NULL;

	ipr_free(n_disk_status.title);
	n_disk_status.title = NULL;

	rc = s_out->rc;
	ipr_free(s_out);
	return rc;
}

#define IPR_LINE_FEED 0
#define IPR_RAW_DATA -1
void body_details(s_node *n_screen, int code, char *field_data)
{
	char *string_buf;
	int cur_len = 0, start_len = 0;
	int str_len, field_data_len;
	char *body;
	int i;

	body = n_screen->body;
	if (body != NULL)
		start_len = cur_len = strlen(body);

	if (code == IPR_LINE_FEED) {
		body = ipr_realloc(body, cur_len + 2);
		sprintf(body + cur_len, "\n");
	} else if (code == IPR_RAW_DATA) {
		body = ipr_realloc(body, cur_len + strlen(field_data) + 4);
		sprintf(body + cur_len, "%s\n", field_data);
	} else {
		string_buf = catgets(catd,n_screen->text_set,code,"");
		str_len = strlen(string_buf);

		if (!field_data) {
			body = ipr_realloc(body, cur_len + str_len + 3);
			sprintf(body + cur_len, "%s:\n", string_buf);
		} else {
			field_data_len = strlen(field_data);

			if (str_len < 30) {
				body = ipr_realloc(body, cur_len + field_data_len + 38);
				cur_len += sprintf(body + cur_len, "%s", string_buf);
				for (i = cur_len; i < 30 + start_len; i++) {
					if ((cur_len + start_len)%2)
						cur_len += sprintf(body + cur_len, ".");
					else
						cur_len += sprintf(body + cur_len, " ");
				}
				sprintf(body + cur_len, " : %s\n", field_data);
			} else {
				body = ipr_realloc(body, cur_len + str_len + field_data_len + 8);
				sprintf(body + cur_len, "%s : %s\n",string_buf, field_data);
			}
		}
	}
	n_screen->body = body;
}

int device_details_get_device(i_container *i_con,
			      struct ipr_dev **device)
{
	i_container *temp_i_con;
	int invalid=0;
	int dev_num = 0;
	struct ipr_dev *cur_device;
	char *input;

	if (i_con == NULL)
		return 1;

	for (temp_i_con = i_con_head;
	     temp_i_con != NULL && !invalid;
	     temp_i_con = temp_i_con->next_item) {

		cur_device =(struct ipr_dev *)(temp_i_con->data);

		if (cur_device == NULL)
			continue;

		input = temp_i_con->field_data;

		if (input == NULL)
			continue;

		if (strcmp(input, "5") == 0) {

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

int device_details(i_container *i_con)
{
	char *buffer;
	struct ipr_dev *device;
	int rc;
	struct scsi_dev_data *scsi_dev_data;
	int i;
	u8 product_id[IPR_PROD_ID_LEN+1];
	u8 vendor_id[IPR_VENDOR_ID_LEN+1];
	u8 plant_code[IPR_VPD_PLANT_CODE_LEN+1];
	u8 part_num[IPR_VPD_PART_NUM_LEN+1];
	u8 firmware_version[5];
	u8 serial_num[IPR_SERIAL_NUM_LEN+1];
	int cache_size;
	u8 dram_size[IPR_VPD_DRAM_SIZE_LEN+4];
	struct ipr_read_cap read_cap;
	struct ipr_read_cap16 read_cap16;
	long double device_capacity; 
	unsigned long long max_user_lba_int;
	double lba_divisor;
	int scsi_channel;
	int scsi_id;
	int scsi_lun;
	struct ipr_ioa_vpd ioa_vpd;
	struct ipr_cfc_vpd cfc_vpd;
	struct ipr_dram_vpd dram_vpd;
	struct ipr_inquiry_page0 page0_inq;
	struct ipr_inquiry_page3 page3_inq;
	struct ipr_dasd_inquiry_page3 dasd_page3_inq;
	struct ipr_std_inq_data_long std_inq;
	struct ipr_common_record *common_record;
	struct ipr_dev_record *device_record;
	struct ipr_array_record *array_record;
	char *hex;
	int len;
	struct screen_output *s_out;
	char *string_buf;

	buffer = ipr_malloc(100);
	mvaddstr(0,0,"DEVICE DETAILS FUNCTION CALLED");

	rc = device_details_get_device(i_con, &device);
	if (rc)
		return rc;

	scsi_dev_data = device->scsi_dev_data;
	array_record =
		(struct ipr_array_record *)device->qac_entry;

	rc = 0;

	if (scsi_dev_data) {
		scsi_channel = scsi_dev_data->channel;
		scsi_id = scsi_dev_data->id;
		scsi_lun = scsi_dev_data->lun;
	} else if (device->qac_entry) {
		common_record = device->qac_entry;
		if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) {
			device_record = (struct ipr_dev_record *)common_record;
			scsi_channel = device_record->last_resource_addr.bus;
			scsi_id = device_record->last_resource_addr.target;
			scsi_lun = device_record->last_resource_addr.lun;
		} else if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) {
			scsi_channel = array_record->last_resource_addr.bus;
			scsi_id = array_record->last_resource_addr.target;
			scsi_lun = array_record->last_resource_addr.lun;
		}
	}

	read_cap.max_user_lba = 0;
	read_cap.block_length = 0;

	if ((scsi_dev_data) &&
	    (scsi_dev_data->type == IPR_TYPE_ADAPTER)) {
		string_buf = catgets(catd,n_device_details.text_set,1,"Title");
		n_device_details.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_device_details.title, string_buf);

		memset(&ioa_vpd, 0, sizeof(ioa_vpd));
		memset(&cfc_vpd, 0, sizeof(cfc_vpd));
		memset(&dram_vpd, 0, sizeof(dram_vpd));
		memset(&page3_inq, 0, sizeof(page3_inq));

		ipr_inquiry(device, IPR_STD_INQUIRY, &ioa_vpd, sizeof(ioa_vpd));
		ipr_inquiry(device, 1, &cfc_vpd, sizeof(cfc_vpd));
		ipr_inquiry(device, 2, &dram_vpd, sizeof(dram_vpd));
		ipr_inquiry(device, 3, &page3_inq, sizeof(page3_inq));

		ipr_strncpy_0(vendor_id,
			      ioa_vpd.std_inq_data.vpids.vendor_id,
			      IPR_VENDOR_ID_LEN);
		ipr_strncpy_0(product_id,
			      ioa_vpd.std_inq_data.vpids.product_id,
			      IPR_PROD_ID_LEN);
		ipr_strncpy_0(plant_code,
			      ioa_vpd.ascii_plant_code,
			      IPR_VPD_PLANT_CODE_LEN);
		ipr_strncpy_0(part_num,
			      ioa_vpd.ascii_part_num,
			      IPR_VPD_PART_NUM_LEN);
		ipr_strncpy_0(buffer,
			      cfc_vpd.cache_size,
			      IPR_VPD_CACHE_SIZE_LEN);
		ipr_strncpy_0(serial_num,
			      cfc_vpd.serial_num,
			      IPR_SERIAL_NUM_LEN);
		cache_size = strtoul(buffer, NULL, 16);
		sprintf(buffer,"%d MB", cache_size);
		memcpy(dram_size, dram_vpd.dram_size, IPR_VPD_DRAM_SIZE_LEN);
		sprintf(dram_size + IPR_VPD_DRAM_SIZE_LEN, " MB");
		sprintf(firmware_version,"%02X%02X%02X%02X",
			page3_inq.major_release,
			page3_inq.card_type,
			page3_inq.minor_release[0],
			page3_inq.minor_release[1]);

		body_details(&n_device_details,IPR_LINE_FEED, NULL);
		body_details(&n_device_details,2, vendor_id);
		body_details(&n_device_details,3, product_id);
		body_details(&n_device_details,4, device->ioa->driver_version);
		body_details(&n_device_details,5, firmware_version);
		body_details(&n_device_details,6, serial_num);
		body_details(&n_device_details,7, part_num);
		body_details(&n_device_details,8, plant_code);
		body_details(&n_device_details,9, buffer);
		body_details(&n_device_details,10, dram_size);
		body_details(&n_device_details,11, device->gen_name);
		body_details(&n_device_details,IPR_LINE_FEED, NULL);
		body_details(&n_device_details,12, NULL);
		body_details(&n_device_details,13, device->ioa->pci_address);
		sprintf(buffer,"%d", device->ioa->host_num);
		body_details(&n_device_details,14, buffer);
	} else if ((array_record) &&
		   (array_record->common.record_id ==
		    IPR_RECORD_ID_ARRAY_RECORD)) {

		string_buf = catgets(catd,n_device_details.text_set,101,"Title");

		n_device_details.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_device_details.title, string_buf);

		ipr_strncpy_0(vendor_id,
			      array_record->vendor_id,
			      IPR_VENDOR_ID_LEN);

		body_details(&n_device_details,IPR_LINE_FEED, NULL);
		body_details(&n_device_details,2, vendor_id);

		sprintf(buffer,"RAID %s", device->prot_level_str);
		body_details(&n_device_details,102, buffer);

		if (ntohs(array_record->stripe_size) > 1024)
			sprintf(buffer,"%d M",
				ntohs(array_record->stripe_size)/1024);
		else
			sprintf(buffer,"%d k",
				ntohs(array_record->stripe_size));

		body_details(&n_device_details,103, buffer);

		/* Do a read capacity to determine the capacity */
		rc = ipr_read_capacity_16(device, &read_cap16);

		if ((rc == 0) &&
		    (ntohl(read_cap16.max_user_lba_hi) ||
		     ntohl(read_cap16.max_user_lba_lo)) &&
		    ntohl(read_cap16.block_length)) {

			max_user_lba_int = ntohl(read_cap16.max_user_lba_hi);
			max_user_lba_int <<= 32;
			max_user_lba_int |= ntohl(read_cap16.max_user_lba_lo);

			device_capacity = max_user_lba_int + 1;

			lba_divisor  =
				(1000*1000*1000) / ntohl(read_cap16.block_length);

			sprintf(buffer,"%.2Lf GB",device_capacity/lba_divisor);
			body_details(&n_device_details,104, buffer);
		}
		body_details(&n_device_details,11, device->dev_name);

		body_details(&n_device_details,IPR_LINE_FEED, NULL);
		body_details(&n_device_details,12, NULL);
		body_details(&n_device_details,13, device->ioa->pci_address);

		sprintf(buffer,"%d", device->ioa->host_num);
		body_details(&n_device_details,203, buffer);

		sprintf(buffer,"%d",scsi_channel);
		body_details(&n_device_details,204, buffer);

		sprintf(buffer,"%d",scsi_id);
		body_details(&n_device_details,205, buffer);

		sprintf(buffer,"%d",scsi_lun);
		body_details(&n_device_details,206, buffer);
	} else {
		string_buf = catgets(catd,n_device_details.text_set,201,"Title");

		n_device_details.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_device_details.title, string_buf);

		memset(&read_cap, 0, sizeof(read_cap));
		rc = ipr_read_capacity(device, &read_cap);

		rc = ipr_inquiry(device, 0x03, &dasd_page3_inq, sizeof(dasd_page3_inq));
		rc = ipr_inquiry(device, IPR_STD_INQUIRY, &std_inq, sizeof(std_inq));

		ipr_strncpy_0(vendor_id,
			      std_inq.std_inq_data.vpids.vendor_id,
			      IPR_VENDOR_ID_LEN);
		ipr_strncpy_0(product_id,
			      std_inq.std_inq_data.vpids.product_id,
			      IPR_PROD_ID_LEN);

		body_details(&n_device_details,IPR_LINE_FEED, NULL);
		body_details(&n_device_details,2, vendor_id);
		body_details(&n_device_details,202, product_id);

		len = sprintf(buffer, "%X%X%X%X",
			      dasd_page3_inq.release_level[0],
			      dasd_page3_inq.release_level[1],
			      dasd_page3_inq.release_level[2],
			      dasd_page3_inq.release_level[3]);

		if (isalnum(dasd_page3_inq.release_level[0]) &&
		    isalnum(dasd_page3_inq.release_level[1]) &&
		    isalnum(dasd_page3_inq.release_level[2]) &&
		    isalnum(dasd_page3_inq.release_level[3]))

			sprintf(buffer + len, " (%c%c%c%c)",
				dasd_page3_inq.release_level[0],
				dasd_page3_inq.release_level[1],
				dasd_page3_inq.release_level[2],
				dasd_page3_inq.release_level[3]);

		body_details(&n_device_details,5, buffer);

		if (strcmp(vendor_id,"IBMAS400") == 0) {
			/* The service level on IBMAS400 dasd is located
			 at byte 107 in the STD Inq data */
			hex = (char *)&std_inq;
			sprintf(buffer, "%X",
				hex[IPR_STD_INQ_AS400_SERV_LVL_OFF]);
			body_details(&n_device_details,207, buffer);
		}

		ipr_strncpy_0(serial_num,
			      std_inq.std_inq_data.serial_num,
			      IPR_SERIAL_NUM_LEN);
		body_details(&n_device_details,6, serial_num);

		if (ntohl(read_cap.block_length) &&
		    ntohl(read_cap.max_user_lba))  {

			lba_divisor = (1000*1000*1000) /
				ntohl(read_cap.block_length);

			device_capacity = ntohl(read_cap.max_user_lba) + 1;
			sprintf(buffer,"%.2Lf GB",
				device_capacity/lba_divisor);

			body_details(&n_device_details,104, buffer);
		}

		if (strlen(device->dev_name) > 0)
			body_details(&n_device_details,11, device->dev_name);

		body_details(&n_device_details,IPR_LINE_FEED, NULL);
		body_details(&n_device_details,12, NULL);
		body_details(&n_device_details,13, device->ioa->pci_address);

		sprintf(buffer,"%d", device->ioa->host_num);
		body_details(&n_device_details,203, buffer);

		sprintf(buffer,"%d",scsi_channel);
		body_details(&n_device_details,204, buffer);

		sprintf(buffer,"%d",scsi_id);
		body_details(&n_device_details,205, buffer);

		sprintf(buffer,"%d",scsi_lun);
		body_details(&n_device_details,206, buffer);

		rc =  ipr_inquiry(device, 0, &page0_inq, sizeof(page0_inq));

		for (i = 0; (i < page0_inq.page_length) && !rc; i++) {
			if (page0_inq.supported_page_codes[i] == 0xC7) { /* FIXME 0xC7 is SCSD, do further research to find what needs to be tested for this affirmation.*/

				body_details(&n_device_details,IPR_LINE_FEED, NULL);
				body_details(&n_device_details,208, NULL);

				ipr_strncpy_0(buffer,
					      std_inq.fru_number,
					      IPR_STD_INQ_FRU_NUM_LEN);
				body_details(&n_device_details,209, buffer);

				ipr_strncpy_0(buffer,
					      std_inq.ec_level,
					      IPR_STD_INQ_EC_LEVEL_LEN);
				body_details(&n_device_details,210, buffer);

				ipr_strncpy_0(buffer,
					      std_inq.part_number,
					      IPR_STD_INQ_PART_NUM_LEN);
				body_details(&n_device_details,7, buffer);

				hex = (u8 *)&std_inq.std_inq_data;
				sprintf(buffer, "%02X%02X%02X%02X%02X%02X%02X%02X",
					hex[0], hex[1], hex[2],
					hex[3], hex[4], hex[5],
					hex[6], hex[7]);
				body_details(&n_device_details,211, buffer);

				ipr_strncpy_0(buffer,
					      std_inq.z1_term,
					      IPR_STD_INQ_Z1_TERM_LEN);
				body_details(&n_device_details,212, buffer);

				ipr_strncpy_0(buffer,
					      std_inq.z2_term,
					      IPR_STD_INQ_Z2_TERM_LEN);
				body_details(&n_device_details,213, buffer);

				ipr_strncpy_0(buffer,
					      std_inq.z3_term,
					      IPR_STD_INQ_Z3_TERM_LEN);
				body_details(&n_device_details,214, buffer);

				ipr_strncpy_0(buffer,
					      std_inq.z4_term,
					      IPR_STD_INQ_Z4_TERM_LEN);
				body_details(&n_device_details,215, buffer);

				ipr_strncpy_0(buffer,
					      std_inq.z5_term,
					      IPR_STD_INQ_Z5_TERM_LEN);
				body_details(&n_device_details,216, buffer);

				ipr_strncpy_0(buffer,
					      std_inq.z6_term,
					      IPR_STD_INQ_Z6_TERM_LEN);
				body_details(&n_device_details,217, buffer);
				break;
			}
		}
	}

	s_out = screen_driver(&n_device_details,0,i_con);
	ipr_free(n_device_details.title);
	n_device_details.title = NULL;
	ipr_free(n_device_details.body);
	n_device_details.body = NULL;
	rc = s_out->rc;
	ipr_free(s_out);
	ipr_free(buffer);
	return rc;
}

#define IPR_INCLUDE 0
#define IPR_REMOVE  1
#define IPR_ADD_HOT_SPARE 0
#define IPR_RMV_HOT_SPARE 1
int raid_screen(i_container *i_con)
{
	int rc;
	struct array_cmd_data *cur_raid_cmd;
	struct screen_output *s_out;
	char *string_buf;
	int loop;
	mvaddstr(0,0,"RAID SCREEN FUNCTION CALLED");

	cur_raid_cmd = raid_cmd_head;

	while(cur_raid_cmd)
	{
		cur_raid_cmd = cur_raid_cmd->next;
		ipr_free(raid_cmd_head);
		raid_cmd_head = cur_raid_cmd;
	}

	string_buf = catgets(catd,n_raid_screen.text_set,1,"Title");
	n_raid_screen.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_raid_screen.title, string_buf);

	for (loop = 0; loop < n_raid_screen.num_opts; loop++) {
		n_raid_screen.body =
			ipr_list_opts(n_raid_screen.body,
				      n_raid_screen.options[loop].key,
				      n_raid_screen.text_set,
				      n_raid_screen.options[loop].cat_num);
	}
	n_raid_screen.body = ipr_end_list(n_raid_screen.body);

	s_out = screen_driver(&n_raid_screen,0,NULL);
	ipr_free(n_raid_screen.body);
	n_raid_screen.body = NULL;
	ipr_free(n_raid_screen.title);
	n_raid_screen.title = NULL;
	rc = s_out->rc;
	ipr_free(s_out);
	return rc;
}

int raid_status(i_container *i_con)
{
	int rc, j, i, k;
	int len = 0;
	int num_lines = 0;
	struct ipr_ioa *cur_ioa;
	struct scsi_dev_data *scsi_dev_data;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int array_id;
	char *buffer[2];
	int toggle = 1;
	struct ipr_dev_record *dev_record;
	struct ipr_array_record *array_record;
	char *prot_level_str;
	mvaddstr(0,0,"DISK STATUS FUNCTION CALLED");

	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_raid_status.text_set,1,"Display Disk Array Status");
	n_raid_status.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_raid_status.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_raid_status, &header_lines, 2, 3, "5=", k);

	for(cur_ioa = ipr_ioa_head;
	    cur_ioa != NULL;
	    cur_ioa = cur_ioa->next)
	{
		if (cur_ioa->ioa.scsi_dev_data == NULL)
			continue;

		/* print Hot Spare devices*/
		for (j = 0; j < cur_ioa->num_devices; j++)
		{
			scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
			if (!ipr_is_hot_spare(&cur_ioa->dev[j]))
			{
				continue;
			}

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[j],buffer[k], "%1", cur_ioa, k);

			i_con = add_i_con(i_con,"\0",&cur_ioa->dev[j]);  

			num_lines++;
		}

		/* print volume set and associated devices*/
		for (j = 0; j < cur_ioa->num_devices; j++) {

			scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
			if (!ipr_is_volume_set(&cur_ioa->dev[j]))
				continue;

			array_record = (struct ipr_array_record *)
				cur_ioa->dev[j].qac_entry;

			if (array_record->start_cand)
				continue;

			/* query resource state to acquire protection level string */
			prot_level_str = get_prot_level_str(cur_ioa->supported_arrays,
							    array_record->raid_level);
			strncpy(cur_ioa->dev[j].prot_level_str,prot_level_str, 8);

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[j],buffer[k], "%1", cur_ioa, k);

			i_con = add_i_con(i_con,"\0",&cur_ioa->dev[j]);

			dev_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
			array_id = dev_record->array_id;
			num_lines++;

			for (i = 0; i < cur_ioa->num_devices; i++) {
				scsi_dev_data = cur_ioa->dev[i].scsi_dev_data;
				dev_record = (struct ipr_dev_record *)cur_ioa->dev[i].qac_entry;

				if (!(ipr_is_array_member(&cur_ioa->dev[i])) ||
				    (ipr_is_volume_set(&cur_ioa->dev[i])) ||
				    ((dev_record != NULL) &&
				     (dev_record->array_id != array_id)))
					continue;

				strncpy(cur_ioa->dev[i].prot_level_str, prot_level_str, 8);

				for (k=0; k<2; k++)
					buffer[k] = print_device(&cur_ioa->dev[i],buffer[k], "%1", cur_ioa, k);

				i_con = add_i_con(i_con,"\0",&cur_ioa->dev[i]);
				num_lines++;
			}
		}
	}


	if (num_lines == 0) {
		string_buf = catgets(catd,n_raid_status.text_set,4,
				     "No devices found");
		len = strlen(buffer[i]);
		buffer[i] = ipr_realloc(buffer[i], len +
					strlen(string_buf) + 4);
		sprintf(buffer[i] + len, "\n%s", string_buf);
	}

	toggle_field = 0;

	do {
		n_raid_status.body = buffer[toggle&1];
		s_out = screen_driver(&n_raid_status,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_raid_status.body = NULL;

	ipr_free(n_raid_status.title);
	n_raid_status.title = NULL;

	rc = s_out->rc;
	ipr_free(s_out);
	return rc;
}

char *add_line_to_body(char *body, char *new_text, char *line_fill, int *line_num)
{
	int body_len = 0;
	int new_text_len = strlen(new_text);
	int line_fill_len = strlen(line_fill);
	int rem_length;
	int new_text_offset = 0;
	int max_y, max_x;
	int len;
	int num_lines = 0;

	getmaxyx(stdscr,max_y,max_x);

	if (body)
		body_len = strlen(body);

	len = body_len;
	rem_length = max_x - 1;

	while (len != 0) {
		if (body[len--] == '\n')
			break;
		rem_length--;
	}

	while (1) {
		if (new_text_len < rem_length) {
			/* done, all data fits in current line */
			body = realloc(body, body_len + new_text_len + 4);
			sprintf(body + body_len, "%s\n", &new_text[new_text_offset]);
			num_lines++;
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

		body = realloc(body, body_len + len +
			       line_fill_len + 4);
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

/* This routine expects the first 2 entries in the catalog to be 1: Title
 2: General description: */
void setup_fail_screen(s_node *n_screen, int start_index, int end_index)
{
	char *string_buf;
	char *body;
	int i, len;

	string_buf = catgets(catd, n_screen->text_set, 1, "Requested Action Failed");
	n_screen->title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_screen->title, string_buf);

	/* header */
	string_buf = catgets(catd, n_screen->text_set, 2, "");
	body = malloc(strlen(string_buf) + 8);
	sprintf(body, "\n");

	body = add_line_to_body(body, string_buf, "", NULL);
	len = strlen(body);

	body = realloc(body, len + 4);
	len += sprintf(body + len, "\n");

	for (i = start_index; i <= end_index; i++) {
		body = realloc(body, len + 4);
		sprintf(body + len, "o  ");
		string_buf = catgets(catd, n_screen->text_set, i, "");
		body = add_line_to_body(body, string_buf, "   ", NULL);
		len = strlen(body);
	}
	n_screen->body = body;
}

int raid_stop(i_container *i_con)
{
	int rc, i, k;
	int found = 0;
	struct ipr_common_record *common_record;
	struct ipr_array_record *array_record;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	char *string_buf;
	int header_lines;
	int toggle = 1;
	struct screen_output *s_out;
	char *prot_level_str;
	mvaddstr(0,0,"RAID STOP FUNCTION CALLED");

	/* empty the linked list that contains field pointers */
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_raid_stop.text_set,1,"Delete a Disk Array");
	n_raid_stop.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_raid_stop.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_raid_stop, &header_lines, 2, 4, "1=", k);

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {

		for (i = 0; i < cur_ioa->num_devices; i++) {

			common_record = cur_ioa->dev[i].qac_entry;

			if ((common_record == NULL) ||
			    (common_record->record_id != IPR_RECORD_ID_ARRAY_RECORD))
				continue;

			array_record = (struct ipr_array_record *)common_record;

			if (!array_record->stop_cand)
				continue;

			rc = is_array_in_use(cur_ioa, array_record->array_id);
			if (rc != 0) continue;

			if (raid_cmd_head)
			{
				raid_cmd_tail->next = (struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
				raid_cmd_tail = raid_cmd_tail->next;
			}
			else
			{
				raid_cmd_head = raid_cmd_tail = (struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
			}

			memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

			raid_cmd_tail->array_id = array_record->array_id;
			raid_cmd_tail->ipr_ioa = cur_ioa;
			raid_cmd_tail->ipr_dev = &cur_ioa->dev[i];

			i_con = add_i_con(i_con,"\0",raid_cmd_tail);

			prot_level_str = get_prot_level_str(cur_ioa->supported_arrays,
							    array_record->raid_level);
			strncpy(cur_ioa->dev[i].prot_level_str,prot_level_str, 8);

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[i],buffer[k], "%1", cur_ioa, k);

			found++;
		}
	}

	if (!found) {
		/* Stop Device Parity Protection Failed */
		setup_fail_screen(&n_raid_stop_fail, 3, 7);

		s_out = screen_driver(&n_raid_stop_fail,0,i_con);

		free(n_raid_stop_fail.title);
		n_raid_stop_fail.title = NULL;
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

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_raid_stop.body = NULL;

	ipr_free(n_raid_stop.title);
	n_raid_stop.title = NULL;

	return rc;
}

int confirm_raid_stop(i_container *i_con)
{
	struct ipr_dev *ipr_dev;
	struct ipr_array_record *array_record;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *cur_ioa;
	int rc, j, k;
	int found = 0;
	char *input;    
	char *buffer[2];
	struct ipr_dev_record *dev_record;
	char *string_buf;
	i_container *temp_i_con;
	struct screen_output *s_out;
	int toggle = 1;
	int header_lines;
	mvaddstr(0,0,"CONFIRM RAID STOP FUNCTION CALLED");

	found = 0;

	/* Setup screen title */
	string_buf = catgets(catd,n_confirm_raid_stop.text_set,1,"Confirm Delete a Disk Array");
	n_confirm_raid_stop.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_raid_stop.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_confirm_raid_stop, &header_lines, 2, 4, "q=", k);

	for (temp_i_con = i_con_head; temp_i_con != NULL;
	     temp_i_con = temp_i_con->next_item) {

		cur_raid_cmd = (struct array_cmd_data *)(temp_i_con->data);
		if (cur_raid_cmd == NULL)
			continue;

		input = temp_i_con->field_data;
		if (strcmp(input, "1") == 0) {
			found = 1;
			cur_raid_cmd->do_cmd = 1;

			array_record = (struct ipr_array_record *)cur_raid_cmd->ipr_dev->qac_entry;
			if (!array_record->issue_cmd)
			{
				array_record->issue_cmd = 1;

				/* known_zeroed means do not preserve
				 user data on stop */
				array_record->known_zeroed = 1;
			}
		} else {
			cur_raid_cmd->do_cmd = 0;
			array_record = (struct ipr_array_record *)cur_raid_cmd->ipr_dev->qac_entry;

			if (array_record->issue_cmd)
				array_record->issue_cmd = 0;
		}
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	rc = RC_SUCCESS;

	for (cur_raid_cmd = raid_cmd_head; cur_raid_cmd; cur_raid_cmd = cur_raid_cmd->next) {

		if (!cur_raid_cmd->do_cmd)
			continue;

		cur_ioa = cur_raid_cmd->ipr_ioa;
		ipr_dev = cur_raid_cmd->ipr_dev;

		for (k=0; k<2; k++)
			buffer[k] = print_device(ipr_dev,buffer[k],"1",cur_ioa, k);

		for (j = 0; j < cur_ioa->num_devices; j++) {
			dev_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;

			if ((ipr_is_array_member(&cur_ioa->dev[j])) &&
			    (dev_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
			    (dev_record->array_id == cur_raid_cmd->array_id)) {
				for (k=0; k<2; k++)
					buffer[k] = print_device(&cur_ioa->dev[j],buffer[k],"1",cur_ioa,k);
			}
		}
	}

	toggle_field = 0;

	do {
		n_confirm_raid_stop.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_stop,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_raid_stop.body = NULL;

	ipr_free(n_confirm_raid_stop.title);
	n_confirm_raid_stop.title = NULL;

	return rc;
}

int do_confirm_raid_stop(i_container *i_con)
{
	struct ipr_dev *ipr_dev;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *cur_ioa;
	int rc;
	int raid_stop_complete();
	int max_y, max_x;

	cur_raid_cmd = raid_cmd_head;
	for (cur_raid_cmd = raid_cmd_head; cur_raid_cmd; cur_raid_cmd = cur_raid_cmd->next) {

		if (!cur_raid_cmd->do_cmd)
			continue;

		cur_ioa = cur_raid_cmd->ipr_ioa;
		ipr_dev = cur_raid_cmd->ipr_dev;

		rc = is_array_in_use(cur_ioa, cur_raid_cmd->array_id);
		if (rc != 0)  {

			syslog(LOG_ERR,
			       "Array %s is currently in use and cannot be deleted.\n",
			       ipr_dev->dev_name);
			return (20 | EXIT_FLAG);
		}

		if (ipr_dev->qac_entry->record_id != IPR_RECORD_ID_ARRAY_RECORD)
			continue;

		getmaxyx(stdscr,max_y,max_x);
		move(max_y-1,0);
		printw("Operation in progress - please wait");
		refresh();

		if (ipr_dev->scsi_dev_data)
			rc = ipr_start_stop_stop(ipr_dev);

		if (rc != 0)
			return (20 | EXIT_FLAG);

		rc = ipr_stop_array_protection(cur_ioa);
		if (rc != 0)
			return (20 | EXIT_FLAG);
	}

	rc = raid_stop_complete();
	return rc;
}

int raid_stop_complete()
{
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int done_bad;
	int not_done = 0;
	int rc, j;
	struct ipr_ioa *cur_ioa;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_common_record *common_record;
	struct scsi_dev_data *scsi_dev_data;

	while(1) {
		done_bad = 0;
		not_done = 0;

		for (cur_raid_cmd = raid_cmd_head; cur_raid_cmd != NULL; 
		     cur_raid_cmd = cur_raid_cmd->next) {
			cur_ioa = cur_raid_cmd->ipr_ioa;

			rc = ipr_query_command_status(&cur_ioa->ioa, &cmd_status);

			if (rc) {
				done_bad = 1;
				continue;
			}

			status_record = cmd_status.record;

			for (j=0; j < cmd_status.num_records; j++, status_record++) {

				if ((status_record->command_code == IPR_STOP_ARRAY_PROTECTION) &&
				    (cur_raid_cmd->array_id == status_record->array_id)) {

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

			for (cur_raid_cmd = raid_cmd_head;
			     cur_raid_cmd != NULL; 
			     cur_raid_cmd = cur_raid_cmd->next) {

				common_record = cur_raid_cmd->ipr_dev->qac_entry;
				if ((common_record != NULL) &&
				    (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD)) {

					scsi_dev_data = cur_raid_cmd->ipr_dev->scsi_dev_data;
				}
			}
			check_current_config(false);

			/* "Stop Parity Protection completed successfully" */
			return (21 | EXIT_FLAG);
		}
		not_done = 0;
		sleep(1);
	}
}

int raid_start(i_container *i_con)
{
	int rc, k;
	int found = 0;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_array_record *array_record;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle = 1;
	mvaddstr(0,0,"RAID START FUNCTION CALLED");

	/* empty the linked list that contains field pointers */
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_raid_start.text_set,1,"Create a Disk Array");
	n_raid_start.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_raid_start.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_raid_start, &header_lines, 2, 4, "1=", k);

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {

		array_record = cur_ioa->start_array_qac_entry;

		if (array_record == NULL)
			continue;

		if (raid_cmd_head) {
			raid_cmd_tail->next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
			raid_cmd_tail = raid_cmd_tail->next;
		} else
			raid_cmd_head = raid_cmd_tail = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));

		memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

		raid_cmd_tail->array_id = array_record->array_id;
		raid_cmd_tail->ipr_ioa = cur_ioa;
		raid_cmd_tail->ipr_dev = &cur_ioa->ioa;

		i_con = add_i_con(i_con,"\0",raid_cmd_tail);

		for (k=0; k<2; k++)
			buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"%1",
						 cur_ioa, k);
		found++;
	}

	if (!found) {
		/* Start Device Parity Protection Failed */
		setup_fail_screen(&n_raid_start_fail, 3, 7);

		s_out = screen_driver(&n_raid_start_fail,0,i_con);

		free(n_raid_start_fail.title);
		n_raid_start_fail.title = NULL;
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

		cur_raid_cmd = raid_cmd_head;

		while(cur_raid_cmd) {
			cur_raid_cmd = cur_raid_cmd->next;
			free(raid_cmd_head);
			raid_cmd_head = cur_raid_cmd;
		}
	}

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_raid_start.body = NULL;

	ipr_free(n_raid_start.title);
	n_raid_start.title = NULL;

	return rc;
}

int raid_start_loop(i_container *i_con)
{
	int rc;
	struct array_cmd_data *cur_raid_cmd;
	int found = 0;
	char *input;
	i_container *temp_i_con;
	mvaddstr(0,0,"RAID START LOOP FUNCTION CALLED");

	for (temp_i_con = i_con_head; temp_i_con != NULL;
	     temp_i_con = temp_i_con->next_item) {

		cur_raid_cmd = (struct array_cmd_data *)(temp_i_con->data);
		if (cur_raid_cmd == NULL)
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
	struct ipr_dev *ipr_dev;
	struct array_cmd_data *cur_raid_cmd;
	int rc, j, k;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_dev_record *device_record;
	i_container *i_con2 = NULL;
	i_container *i_con_head_saved;
	i_container *temp_i_con = NULL;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle = 1;
	mvaddstr(0,0,"CONFIGURE RAID START FUNCTION CALLED");

	rc = RC_SUCCESS;

	cur_raid_cmd = (struct array_cmd_data *)(i_con->data);
	cur_ioa = cur_raid_cmd->ipr_ioa;

	/* Setup screen title */
	string_buf = catgets(catd,n_configure_raid_start.text_set,1,"Create a Disk Array");
	n_configure_raid_start.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_configure_raid_start.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_configure_raid_start, &header_lines, 2, 3, "1=", k);

	i_con_head_saved = i_con_head;
	i_con_head = NULL;

	for (j = 0; j < cur_ioa->num_devices; j++) {

		scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
		device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;

		if ((device_record == NULL) ||
		    (scsi_dev_data == NULL))
			continue;

		if ((scsi_dev_data->type != IPR_TYPE_ADAPTER) &&
		    (device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
		    (device_record->start_cand)) {

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[j],buffer[k],"%1", cur_ioa, k);

			i_con2 = add_i_con(i_con2,"\0",&cur_ioa->dev[j]);
		}
	}

	do
	{
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

		for (temp_i_con = i_con_head;
		     temp_i_con != NULL;
		     temp_i_con = temp_i_con->next_item) {

			ipr_dev = (struct ipr_dev *)temp_i_con->data;
			input = temp_i_con->field_data;

			if ((ipr_dev != NULL) &&
			    (strcmp(input, "1") == 0)) {

				found = 1;
				device_record = (struct ipr_dev_record *)ipr_dev->qac_entry;
				device_record->issue_cmd = 1;
			}
		}

		if (found != 0) {

			/* Go to parameters screen */
			rc = configure_raid_parameters(i_con_head_saved);

			if ((rc &  EXIT_FLAG) ||
			    !(rc & CANCEL_FLAG))
				goto leave;

			/* User selected Cancel, clear out current selections
			 for redisplay */
			for (temp_i_con = i_con_head;
			     temp_i_con != NULL;
			     temp_i_con = temp_i_con->next_item) {

				ipr_dev = (struct ipr_dev *)temp_i_con->data;
				input = temp_i_con->field_data;

				if ((ipr_dev != NULL) &&
				    (strcmp(input, "1") == 0)) {

					device_record = (struct ipr_dev_record *)ipr_dev->qac_entry;
					device_record->issue_cmd = 0;
				}
			}
			rc = REFRESH_FLAG;
			continue;
		}
		else {

			s_status.index = INVALID_OPTION_STATUS;
			rc = REFRESH_FLAG;
		}
	} while (rc & REFRESH_FLAG);

	leave:
		i_con2 = free_i_con(i_con2);

	i_con_head = i_con_head_saved;
	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_configure_raid_start.body = NULL;

	ipr_free(n_configure_raid_start.title);
	n_configure_raid_start.title = NULL;

	return rc;
}

int configure_raid_parameters(i_container *i_con)
{
	FIELD *input_fields[4];
	FIELD *cur_field;
	FORM *form;
	ITEM **raid_item = NULL;
	struct array_cmd_data *cur_raid_cmd = (struct array_cmd_data *)(i_con->data);
	int raid_index_default;
	unsigned long raid_index = 0;
	unsigned long index;
	char buffer[24];
	int rc, i;
	struct ipr_ioa *cur_ioa;
	struct ipr_supported_arrays *supported_arrays;
	struct ipr_array_cap_entry *cur_array_cap_entry;
	struct ipr_dev_record *device_record;
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
	int max_x,max_y,start_y,start_x;

	getmaxyx(stdscr,max_y,max_x);
	getbegyx(stdscr,start_y,start_x);

	rc = RC_SUCCESS;

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

	input_fields[3] = NULL;


	cur_ioa = cur_raid_cmd->ipr_ioa;
	supported_arrays = cur_ioa->supported_arrays;
	cur_array_cap_entry = (struct ipr_array_cap_entry *)supported_arrays->data;

	/* determine number of devices selected for this parity set */
	for (i=0; i<cur_ioa->num_devices; i++) {

		device_record = (struct ipr_dev_record *)
			cur_ioa->dev[i].qac_entry;

		if ((device_record != NULL) &&
		    (device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
		    (device_record->issue_cmd)) {

			selected_count++;
		}
	}

	/* set up raid lists */
	raid_index = 0;
	raid_index_default = 0;
	prot_level_list = malloc(sizeof(struct prot_level));
	prot_level_list[raid_index].is_valid_entry = 0;

	for (i=0; i<ntohs(supported_arrays->num_entries); i++)
	{
		if ((selected_count <= cur_array_cap_entry->max_num_array_devices) &&
		    (selected_count >= cur_array_cap_entry->min_num_array_devices) &&
		    ((selected_count % cur_array_cap_entry->min_mult_array_devices) == 0))
		{
			prot_level_list[raid_index].array_cap_entry = cur_array_cap_entry;
			prot_level_list[raid_index].is_valid_entry = 1;
			if (0 == strcmp(cur_array_cap_entry->prot_level_str,IPR_DEFAULT_RAID_LVL))
			{
				raid_index_default = raid_index;
			}
			raid_index++;
			prot_level_list = realloc(prot_level_list, sizeof(struct prot_level) * (raid_index + 1));
			prot_level_list[raid_index].is_valid_entry = 0;
		}
		cur_array_cap_entry = (struct ipr_array_cap_entry *)
			((void *)cur_array_cap_entry + ntohs(supported_arrays->entry_length));
	}

	raid_index = raid_index_default;
	cur_array_cap_entry = prot_level_list[raid_index].array_cap_entry;
	stripe_sz = ntohs(cur_array_cap_entry->recommended_stripe_size);

	form = new_form(input_fields);

	set_current_field(form, input_fields[1]);

	set_field_just(input_fields[0], JUSTIFY_CENTER);

	field_opts_off(input_fields[0], O_ACTIVE);
	field_opts_off(input_fields[1], O_EDIT);
	field_opts_off(input_fields[2], O_EDIT);

	set_field_buffer(input_fields[0],        /* field to alter */
			 0,          /* number of buffer to alter */
			 "Select Protection Level and Stripe Size");  /* string value to set     */

	form_opts_off(form, O_BS_OVERLOAD);

	flush_stdscr();

	clear();

	set_form_win(form,stdscr);
	set_form_sub(form,stdscr);
	post_form(form);

	mvaddstr(2, 1, "Default array configurations are shown.  To change");
	mvaddstr(3, 1, "setting hit \"c\" for options menu.  Highlight desired");
	mvaddstr(4, 1, "option then hit Enter");
	mvaddstr(6, 1, "c=Change Setting");
	mvaddstr(8, 0, "Protection Level . . . . . . . . . . . . :");
	mvaddstr(9, 0, "Stripe Size  . . . . . . . . . . . . . . :");
	mvaddstr(max_y - 4, 0, "Press Enter to Continue");
	mvaddstr(max_y - 2, 0, EXIT_KEY_LABEL CANCEL_KEY_LABEL);

	form_driver(form,               /* form to pass input to */
		    REQ_FIRST_FIELD );     /* form request code */

	while (1)
	{

		sprintf(raid_str,"RAID %s",cur_array_cap_entry->prot_level_str);
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

		refresh();
		ch = getch();

		if (IS_EXIT_KEY(ch))
		{
			rc = EXIT_FLAG;
			goto leave;
		}
		else if (IS_CANCEL_KEY(ch))
		{
			rc = CANCEL_FLAG;
			goto leave;
		}
		else if (ch == 'c')
		{
			cur_field = current_field(form);
			cur_field_index = field_index(cur_field);

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
				if (rc == RC_SUCCESS)
				{
					raid_index = *retptr;

					/* find recommended stripe size */
					cur_array_cap_entry = prot_level_list[raid_index].array_cap_entry;
					stripe_sz = ntohs(cur_array_cap_entry->recommended_stripe_size);
				}
			}
			else if (cur_field_index == 2)
			{
				/* count the number of valid entries */
				index = 0;
				for (i=0; i<16; i++)
				{
					stripe_sz_mask = 1 << i;
					if ((stripe_sz_mask & ntohs(cur_array_cap_entry->supported_stripe_sizes))
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
					if ((stripe_sz_mask & ntohs(cur_array_cap_entry->supported_stripe_sizes))
					    == stripe_sz_mask)
					{
						stripe_sz_list[index] = stripe_sz_mask;
						if (stripe_sz_mask > 1024)
							sprintf(stripe_menu_str[index].line,"%d M",stripe_sz_mask/1024);
						else
							sprintf(stripe_menu_str[index].line,"%d k",stripe_sz_mask);

						if (stripe_sz_mask == ntohs(cur_array_cap_entry->recommended_stripe_size))
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

			if (rc == EXIT_FLAG)
				goto leave;
		}
		else if (IS_ENTER_KEY(ch))
		{
			/* set protection level and stripe size appropriately */
			cur_raid_cmd->prot_level = cur_array_cap_entry->prot_level;
			cur_raid_cmd->stripe_size = stripe_sz;
			goto leave;
		}
		else if (ch == KEY_RIGHT)
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
	struct ipr_ioa *cur_ioa;
	int rc, j, k;
	char *buffer[2];
	struct ipr_dev_record *device_record;
	struct ipr_dev *ipr_dev;
	char *string_buf;
	int header_lines;
	int toggle = 1;
	struct screen_output *s_out;
	mvaddstr(0,0,"CONFIRM RAID START FUNCTION CALLED");

	rc = RC_SUCCESS;

	/* Setup screen title */
	string_buf = catgets(catd,n_confirm_raid_start.text_set,1,"Create a Disk Array");
	n_confirm_raid_start.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_raid_start.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_confirm_raid_start, &header_lines, 2, 4, "q=", k);

	cur_raid_cmd = raid_cmd_head;

	while(cur_raid_cmd) {
		if (cur_raid_cmd->do_cmd) {

			cur_ioa = cur_raid_cmd->ipr_ioa;
			ipr_dev = cur_raid_cmd->ipr_dev;

			for (k=0; k<2; k++)
				buffer[k] = print_device(ipr_dev,buffer[k],"1",cur_ioa, k);

			for (j = 0; j < cur_ioa->num_devices; j++) {

				device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;

				if ((device_record != NULL) &&
				    ((device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
				     (device_record->issue_cmd))) {

					for (k=0; k<2; k++)
						buffer[k] = print_device(&cur_ioa->dev[j],buffer[k],"1",cur_ioa, k);
				}
			}
		}
		cur_raid_cmd = cur_raid_cmd->next;
	}

	toggle_field = 0;

	do {
		n_confirm_raid_start.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_start,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_raid_start.body = NULL;

	ipr_free(n_confirm_raid_start.title);
	n_confirm_raid_start.title = NULL;

	if (rc > 0)
		return rc;

	/* verify the devices are not in use */
	cur_raid_cmd = raid_cmd_head;
	while (cur_raid_cmd)  {
		if (cur_raid_cmd->do_cmd) {
			cur_ioa = cur_raid_cmd->ipr_ioa;
			rc = is_array_in_use(cur_ioa, cur_raid_cmd->array_id);
			if (rc != 0) {

				/* "Start Parity Protection failed." */
				rc = 19;  
				syslog(LOG_ERR, "Devices may have changed state. Command cancelled,"
				       " please issue commands again if RAID still desired %s.\n",
				       cur_ioa->ioa.gen_name);
				return rc;
			}
		}
		cur_raid_cmd = cur_raid_cmd->next;
	}

	/* now issue the start array command */
	for (cur_raid_cmd = raid_cmd_head; cur_raid_cmd != NULL; 
	cur_raid_cmd = cur_raid_cmd->next) {

		if (cur_raid_cmd->do_cmd == 0)
			continue;

		cur_ioa = cur_raid_cmd->ipr_ioa;
		rc = ipr_start_array_protection(cur_ioa,
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
	int rc, j;
	u32 percent_cmplt = 0;
	struct ipr_ioa *cur_ioa;
	struct array_cmd_data *cur_raid_cmd;

	n_raid_start_complete.title = catgets(catd,n_raid_start_complete.text_set,
					      1,"Start Device Parity Protection Status");
	n_raid_start_complete.body = catgets(catd,n_raid_start_complete.text_set,
					     2,"You selected to start device parity protection");

	while (1) {

		rc = complete_screen_driver(&n_raid_start_complete,percent_cmplt,1);

		if (rc & EXIT_FLAG)
			return rc;

		percent_cmplt = 100;
		done_bad = 0;
		not_done = 0;

		for (cur_raid_cmd = raid_cmd_head;
		     cur_raid_cmd != NULL; 
		     cur_raid_cmd = cur_raid_cmd->next) {

			if (cur_raid_cmd->do_cmd == 0)
				continue;

			cur_ioa = cur_raid_cmd->ipr_ioa;
			rc = ipr_query_command_status(&cur_ioa->ioa, &cmd_status);

			if (rc)   {
				done_bad = 1;
				continue;
			}

			status_record = cmd_status.record;

			for (j=0; j < cmd_status.num_records; j++, status_record++) {

				if ((status_record->command_code == IPR_START_ARRAY_PROTECTION) &&
				    (cur_raid_cmd->array_id == status_record->array_id)) {

					if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS) {

						if (status_record->percent_complete < percent_cmplt)
							percent_cmplt = status_record->percent_complete;
						not_done = 1;
					} else if (status_record->status !=
						   IPR_CMD_STATUS_SUCCESSFUL) {

						done_bad = 1;
						syslog(LOG_ERR, "Start parity protect to %s failed.  "
						       "Check device configuration for proper format.\n",
						       cur_ioa->ioa.gen_name);
						rc = RC_FAILED;
					}
					break;
				}
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
	int rc, i, k;
	int found = 0;
	struct ipr_common_record *common_record;
	struct ipr_dev_record *device_record;
	struct array_cmd_data *cur_raid_cmd;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	mvaddstr(0,0,"RAID REBUILD FUNCTION CALLED");

	rc = RC_SUCCESS;

	check_current_config(true);

	/* Setup screen title */
	string_buf = catgets(catd,n_raid_rebuild.text_set,1,"Rebuild Disk Unit Data");
	n_raid_rebuild.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_raid_rebuild.title, string_buf);
    
	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_raid_rebuild, &header_lines, 2, 4, "1=", k);

	for (cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {

		cur_ioa->num_raid_cmds = 0;

		for (i = 0; i < cur_ioa->num_devices; i++) {

			common_record = cur_ioa->dev[i].qac_entry;
			if ((common_record == NULL) ||
			    (common_record->record_id != IPR_RECORD_ID_DEVICE_RECORD))
				continue;

			device_record = (struct ipr_dev_record *)common_record;
			if (!device_record->rebuild_cand)
				continue;

			if (raid_cmd_head) {
				raid_cmd_tail->next = (struct array_cmd_data *)
					malloc(sizeof(struct array_cmd_data));
				raid_cmd_tail = raid_cmd_tail->next;
			} else
				raid_cmd_head = raid_cmd_tail = (struct array_cmd_data *)
					malloc(sizeof(struct array_cmd_data));

			memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

			raid_cmd_tail->ipr_ioa = cur_ioa;
			raid_cmd_tail->ipr_dev = &cur_ioa->dev[i];

			i_con = add_i_con(i_con,"\0",raid_cmd_tail);
			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[i],buffer[k],"%1",cur_ioa, k);

			found++;
			cur_ioa->num_raid_cmds++;
		}
	}
 
	if (!found) {
		/* Start Device Parity Protection Failed */
		setup_fail_screen(&n_raid_rebuild_fail, 3, 7);

		s_out = screen_driver(&n_raid_rebuild_fail,0,NULL);

		free(n_raid_rebuild_fail.title);
		n_raid_rebuild_fail.title = NULL;
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

		cur_raid_cmd = raid_cmd_head;

		while(cur_raid_cmd) {
			cur_raid_cmd = cur_raid_cmd->next;
			free(raid_cmd_head);
			raid_cmd_head = cur_raid_cmd;
		}
	}

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_raid_rebuild.body = NULL;

	ipr_free(n_raid_rebuild.title);
	n_raid_rebuild.title = NULL;

	return rc;
}

int confirm_raid_rebuild(i_container *i_con)
{
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_dev_record *device_record;
	struct ipr_ioa *cur_ioa;
	int rc;
	char *buffer[2];
	int found = 0;
	char *input;
	int k;
	int header_lines;
	int toggle=1;
	i_container *temp_i_con;
	char *string_buf;
	struct screen_output *s_out;
	mvaddstr(0,0,"CONFIRM RAID REBUILD FUNCTION CALLED");

	for (temp_i_con = i_con; temp_i_con != NULL;
	     temp_i_con = temp_i_con->next_item) {

		cur_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
		if (cur_raid_cmd == NULL)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			cur_raid_cmd->do_cmd = 1;

			device_record = (struct ipr_dev_record *)cur_raid_cmd->ipr_dev->qac_entry;
			if (device_record)
				device_record->issue_cmd = 1;

			found++;
		} else {
			cur_raid_cmd->do_cmd = 0;

			device_record = (struct ipr_dev_record *)cur_raid_cmd->ipr_dev->qac_entry;
			if (device_record)
				device_record->issue_cmd = 0;
		}
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	/* Setup screen title */
	string_buf = catgets(catd,n_confirm_raid_rebuild.text_set,1,
			     "Confirm Rebuild Disk Unit Data");
	n_confirm_raid_rebuild.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_raid_rebuild.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_confirm_raid_rebuild, &header_lines, 2, 4, "1=", k);

	for (cur_raid_cmd = raid_cmd_head; cur_raid_cmd; cur_raid_cmd = cur_raid_cmd->next) {
		if (!cur_raid_cmd->do_cmd)
			continue;

		cur_ioa = cur_raid_cmd->ipr_ioa;
		for (k=0; k<2; k++)
			buffer[k] = print_device(cur_raid_cmd->ipr_dev,buffer[k],"1",cur_ioa, k);
	}

	do {
		n_confirm_raid_rebuild.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_rebuild,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_raid_rebuild.body = NULL;

	ipr_free(n_confirm_raid_rebuild.title);
	n_confirm_raid_rebuild.title = NULL;

	if (rc)
		return rc;

	for (cur_ioa = ipr_ioa_head;  cur_ioa != NULL; cur_ioa = cur_ioa->next)	{
		if (cur_ioa->num_raid_cmds == 0)
			continue;

		rc = ipr_rebuild_device_data(cur_ioa);

		if (rc != 0)
			/* Rebuild failed */
			return (29 | EXIT_FLAG);
	}

	/* Rebuild started, view Parity Status Window for rebuild progress */
	return (28 | EXIT_FLAG); 
}

int raid_include(i_container *i_con)
{
	int i, j, k;
	int found = 0;
	struct ipr_common_record *common_record;
	struct ipr_array_record *array_record;
	struct array_cmd_data *cur_raid_cmd;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	int include_allowed;
	struct ipr_supported_arrays *supported_arrays;
	struct ipr_array_cap_entry *cur_array_cap_entry;
	int rc = RC_SUCCESS;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle = 1;
	mvaddstr(0,0,"RAID INCLUDE FUNCTION CALLED");

	i_con = free_i_con(i_con);

	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_raid_include.text_set,1,"Add a Device to a Disk Array");
	n_raid_include.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_raid_include.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_raid_include, &header_lines, 2, 4, "1=", k);

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {

		for (i = 0; i < cur_ioa->num_devices; i++) {

			common_record = cur_ioa->dev[i].qac_entry;

			if ((common_record == NULL) ||
			    (common_record->record_id != IPR_RECORD_ID_ARRAY_RECORD))
				continue;

			array_record = (struct ipr_array_record *)common_record;

			supported_arrays = cur_ioa->supported_arrays;
			cur_array_cap_entry =
				(struct ipr_array_cap_entry *)supported_arrays->data;
			include_allowed = 0;

			for (j = 0; j < ntohs(supported_arrays->num_entries); j++) {

				if (cur_array_cap_entry->prot_level ==
				    array_record->raid_level)  {

					if (cur_array_cap_entry->include_allowed)
						include_allowed = 1;

					strncpy(cur_ioa->dev[i].prot_level_str,
						cur_array_cap_entry->prot_level_str, 8);
					break;
				}

				cur_array_cap_entry = (struct ipr_array_cap_entry *)
					((void *)cur_array_cap_entry +
					 ntohs(supported_arrays->entry_length));
			}

			if (!include_allowed || !array_record->established)
				continue;

			if (raid_cmd_head)
			{
				raid_cmd_tail->next =
					(struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
				raid_cmd_tail = raid_cmd_tail->next;
			}
			else
			{
				raid_cmd_head = raid_cmd_tail =
					(struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
			}

			memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

			raid_cmd_tail->array_id = array_record->array_id;
			raid_cmd_tail->ipr_ioa = cur_ioa;
			raid_cmd_tail->ipr_dev = &cur_ioa->dev[i];
			raid_cmd_tail->qac_data = NULL;

			i_con = add_i_con(i_con,"\0",raid_cmd_tail);

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[i],buffer[k],"%1",cur_ioa, k);

			found++;
		}
	}

	if (!found) {
		/* Include Device Parity Protection Failed */
		setup_fail_screen(&n_raid_include_fail, 3, 7);

		s_out = screen_driver(&n_raid_include_fail,0,i_con);

		free(n_raid_include_fail.title);
		n_raid_include_fail.title = NULL;
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

		cur_raid_cmd = raid_cmd_head;

		while(cur_raid_cmd) {
			if (cur_raid_cmd->qac_data) {
				free(cur_raid_cmd->qac_data);
				cur_raid_cmd->qac_data = NULL;
			}
			cur_raid_cmd = cur_raid_cmd->next;
			free(raid_cmd_head);
			raid_cmd_head = cur_raid_cmd;
		}
	}

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_raid_include.body = NULL;

	ipr_free(n_raid_include.title);
	n_raid_include.title = NULL;

	return rc;
}

int configure_raid_include(i_container *i_con)
{
	int i, j, k;
	int found = 0;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_array_query_data *qac_data = calloc(1,sizeof(struct ipr_array_query_data));
	struct ipr_common_record *common_record;
	struct ipr_dev_record *device_record;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_array_record *array_record;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	char *input;
	struct ipr_dev *ipr_dev;
	struct ipr_supported_arrays *supported_arrays;
	struct ipr_array_cap_entry *cur_array_cap_entry;
	u8 min_mult_array_devices = 1;
	int rc = RC_SUCCESS;
	char *string_buf;
	int header_lines;
	i_container *temp_i_con;
	int toggle = 1;
	struct screen_output *s_out;
	mvaddstr(0,0,"CONFIGURE RAID INCLUDE FUNCTION CALLED");

	for (temp_i_con = i_con_head; temp_i_con != NULL;
	     temp_i_con = temp_i_con->next_item) {

		cur_raid_cmd = (struct array_cmd_data *)i_con->data;
		if (cur_raid_cmd == NULL)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0) {
			found++;
			break;
		}
	}

	if (found != 1)
		return INVALID_OPTION_STATUS;

	i_con = free_i_con(i_con);

	/* Setup screen title */
	string_buf = catgets(catd,n_configure_raid_include.text_set,1,
			     "Include Disk Units in Device Parity Protection");
	n_configure_raid_include.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_configure_raid_include.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_configure_raid_include, &header_lines, 2, 4, "1=", k);

	cur_ioa = cur_raid_cmd->ipr_ioa;
	ipr_dev = cur_raid_cmd->ipr_dev;

	/* save off raid level, this information will be used to find
	 minimum multiple */
	array_record = (struct ipr_array_record *)ipr_dev->qac_entry;

	/* Get Query Array Config Data */
	rc = ipr_query_array_config(cur_ioa, 0, 1, cur_raid_cmd->array_id, qac_data);

	found = 0;

	if (rc != 0)
		free(qac_data);
	else  {
		cur_raid_cmd->qac_data = qac_data;
		common_record = (struct ipr_common_record *)qac_data->data;
		for (k = 0; k < qac_data->num_records; k++) {

			if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) {

				for (j = 0; j < cur_ioa->num_devices; j++) {

					scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
					device_record = (struct ipr_dev_record *)common_record;

					if (scsi_dev_data == NULL)
						continue;

					if ((scsi_dev_data->handle == device_record->resource_handle) &&
					    (scsi_dev_data->opens == 0) &&
					    (device_record->include_cand)) {

						cur_ioa->dev[j].qac_entry = common_record;
						i_con = add_i_con(i_con,"\0",&cur_ioa->dev[j]);

						for (k=0; k<2; k++)
							buffer[k] = print_device(&cur_ioa->dev[j],buffer[k],"%1",cur_ioa, k);

						found++;
						break;
					}
				}
			}

			/* find minimum multiple for the raid level being used */
			if (common_record->record_id == IPR_RECORD_ID_SUPPORTED_ARRAYS) {

				supported_arrays = (struct ipr_supported_arrays *)common_record;
				cur_array_cap_entry = (struct ipr_array_cap_entry *)supported_arrays->data;

				for (i=0; i<ntohs(supported_arrays->num_entries); i++) {

					if (cur_array_cap_entry->prot_level == array_record->raid_level)
						min_mult_array_devices = cur_array_cap_entry->min_mult_array_devices;

					cur_array_cap_entry = (struct ipr_array_cap_entry *)
						((void *)cur_array_cap_entry + supported_arrays->entry_length);
				}
			}
			common_record = (struct ipr_common_record *)
				((unsigned long)common_record + ntohs(common_record->record_len));
		}
	}

	if (found <= min_mult_array_devices) {

		/* Include Device Parity Protection Failed */
		setup_fail_screen(&n_configure_raid_include_fail, 3, 7);

		s_out = screen_driver(&n_configure_raid_include_fail,0,i_con);

		free(n_configure_raid_include_fail.title);
		n_configure_raid_include_fail.title = NULL;
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

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_configure_raid_include.body = NULL;

	ipr_free(n_configure_raid_include.title);
	n_configure_raid_include.title = NULL;

	i_con = s_out->i_con;

	if ((rc = s_out->rc) > 0)
		goto leave;

	/* first, count devices select to be sure min multiple
	 is satisfied */
	found = 0;

	for (temp_i_con = i_con_head; temp_i_con != NULL;
	     temp_i_con = temp_i_con->next_item) {

		input = temp_i_con->field_data;
		if (strcmp(input, "1") == 0)
			found++;
	}

	if (found % min_mult_array_devices != 0)  {
		/* "Error:  number of devices selected must be a multiple of %d" */ 
		s_status.num = min_mult_array_devices;
		return 25 | REFRESH_FLAG; 
	}

	for (temp_i_con = i_con_head; temp_i_con != NULL;
	     temp_i_con = temp_i_con->next_item)  {

		ipr_dev = (struct ipr_dev *)temp_i_con->data;

		if (ipr_dev != NULL) {

			input = temp_i_con->field_data;

			if (strcmp(input, "1") == 0) {

				cur_raid_cmd->do_cmd = 1;

				device_record = (struct ipr_dev_record *)ipr_dev->qac_entry;
				if (!device_record->issue_cmd) {

					device_record->issue_cmd = 1;
					device_record->known_zeroed = 1;
				}
			} else {
				device_record = (struct ipr_dev_record *)ipr_dev->qac_entry;
				if (device_record->issue_cmd) {

					device_record->issue_cmd = 0;
					device_record->known_zeroed = 0;
				}
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
	struct ipr_dev_record *device_record;
	int i, k;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	int rc = RC_SUCCESS;
	struct screen_output *s_out;
	int num_devs;
	int dev_include_complete(u8 num_devs);
	int format_include_cand();
	struct devs_to_init_t *cur_dev_init;
	char *string_buf;
	int header_lines;
	int toggle = 1;
	mvaddstr(0,0,"CONFIRM RAID INCLUDE FUNCTION CALLED");

	/* Setup screen title */
	string_buf = catgets(catd,n_confirm_raid_include.text_set,1,"Create a Disk Array");
	n_confirm_raid_include.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_raid_include.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_confirm_raid_include, &header_lines, 2, 4, "q=", k);

	cur_raid_cmd = raid_cmd_head;

	while(cur_raid_cmd) {

		if (cur_raid_cmd->do_cmd) {

			cur_ioa = cur_raid_cmd->ipr_ioa;

			for (i = 0; i < cur_ioa->num_devices; i++) {

				device_record = (struct ipr_dev_record *)cur_ioa->dev[i].qac_entry;
				if ((device_record != NULL) && (device_record->issue_cmd)) {

					for (k=0; k<2; k++)
						buffer[k] = print_device(&cur_ioa->dev[i],buffer[k],"1",cur_ioa, k);
				}
			}
		}
		cur_raid_cmd = cur_raid_cmd->next;
	}

	do {
		n_confirm_raid_include.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_raid_include,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_raid_include.body = NULL;

	ipr_free(n_confirm_raid_include.title);
	n_confirm_raid_include.title = NULL;

	if (rc)
		return rc;

	/* now format each of the devices */
	num_devs = format_include_cand();

	rc = dev_include_complete(num_devs);

	/* free up memory allocated for format */
	cur_dev_init = dev_init_head;
	while(cur_dev_init)
	{
		cur_dev_init = cur_dev_init->next;
		free(dev_init_head);
		dev_init_head = cur_dev_init;
	}

	return rc;
}

int format_include_cand()
{
	struct scsi_dev_data *scsi_dev_data;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_ioa *cur_ioa;
	int num_devs = 0;
	int rc = 0;
	struct ipr_dev_record *device_record;
	int i, opens;

	cur_raid_cmd = raid_cmd_head;
	while (cur_raid_cmd) {

		if (cur_raid_cmd->do_cmd) {

			cur_ioa = cur_raid_cmd->ipr_ioa;

			for (i = 0; i < cur_ioa->num_devices; i++) {

				device_record = (struct ipr_dev_record *)cur_ioa->dev[i].qac_entry;
				if ((device_record != NULL) && (device_record->issue_cmd)) {

					/* get current "opens" data for this device to determine conditions to continue */
					opens = num_device_opens(scsi_dev_data->host,
								 scsi_dev_data->channel,
								 scsi_dev_data->id,
								 scsi_dev_data->lun);

					if (opens != 0)  {
						syslog(LOG_ERR,"Include Device not allowed, device in use - %s\n",
						       cur_ioa->dev[i].gen_name);
						device_record->issue_cmd = 0;
						continue;
					}

					if (dev_init_head) {
						dev_init_tail->next = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));
						dev_init_tail = dev_init_tail->next;
					} else
						dev_init_head = dev_init_tail = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));

					memset(dev_init_tail, 0, sizeof(struct devs_to_init_t));
					dev_init_tail->ioa = cur_ioa;
					dev_init_tail->ipr_dev = &cur_ioa->dev[i];
					dev_init_tail->do_init = 1;

					/* Issue the format. Failure will be detected by query command status */
					rc = ipr_format_unit(dev_init_tail->ipr_dev);  /* FIXME  Mandatory lock? */

					num_devs++;
				}
			}
		}
		cur_raid_cmd = cur_raid_cmd->next;
	}
	return num_devs;
}

int dev_include_complete(u8 num_devs)
{
	int done_bad;
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int not_done = 0;
	int rc, j;
	struct devs_to_init_t *cur_dev_init;
	u32 percent_cmplt = 0;
	struct ipr_ioa *cur_ioa;
	struct array_cmd_data *cur_raid_cmd;
	struct ipr_common_record *common_record;

	n_dev_include_complete.title = catgets(catd,n_dev_include_complete.text_set,
					       1,"Include Disk Units in Device Parity Protection Status");
	n_dev_include_complete.body = catgets(catd,n_dev_include_complete.text_set,
					      2,"You selected to include device parity protection");

	while(1) {
		complete_screen_driver(&n_dev_include_complete, percent_cmplt,0);

		percent_cmplt = 100;
		done_bad = 0;

		for (cur_dev_init = dev_init_head;
		     cur_dev_init != NULL;
		     cur_dev_init = cur_dev_init->next) {

			if (cur_dev_init->do_init) {
				rc = ipr_query_command_status(cur_dev_init->ipr_dev,
							      &cmd_status);

				if ((rc != 0) || (cmd_status.num_records == 0)) {
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
	for (cur_raid_cmd = raid_cmd_head;
	     cur_raid_cmd != NULL; 
	     cur_raid_cmd = cur_raid_cmd->next){

		if (cur_raid_cmd->do_cmd == 0)
			continue;

		cur_ioa = cur_raid_cmd->ipr_ioa;

		rc = ipr_add_array_device(cur_ioa, cur_raid_cmd->qac_data);

		if (rc != 0)
			rc = 26;
	}

	not_done = 0;
	percent_cmplt = 0;

	n_dev_include_complete.body = catgets(catd,n_dev_include_complete.text_set,
					      3,"You selected to include a device in a device parity set");

	while(1) {

		complete_screen_driver(&n_dev_include_complete, percent_cmplt, 1);

		percent_cmplt = 100;
		done_bad = 0;

		for (cur_raid_cmd = raid_cmd_head; cur_raid_cmd != NULL; 
		cur_raid_cmd = cur_raid_cmd->next)  {

			cur_ioa = cur_raid_cmd->ipr_ioa;

			rc = ipr_query_command_status(&cur_ioa->ioa, &cmd_status);

			if (rc) {
				done_bad = 1;
				continue;
			}

			status_record = cmd_status.record;

			for (j=0; j < cmd_status.num_records; j++, status_record++) {

				if ((status_record->command_code == IPR_ADD_ARRAY_DEVICE) &&
				    (cur_raid_cmd->array_id == status_record->array_id)) {

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
			for (cur_raid_cmd = raid_cmd_head; cur_raid_cmd != NULL; 
			     cur_raid_cmd = cur_raid_cmd->next) {

				if (!cur_raid_cmd->do_cmd)
					continue;

				common_record = cur_raid_cmd->ipr_dev->qac_entry;
				if ((common_record != NULL) &&
				    (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD))

					rc = ipr_re_read_partition(cur_raid_cmd->ipr_dev);
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

int af_include(i_container *i_con)
{
	int rc;
	mvaddstr(0,0,"AF INCLUDE FUNCTION CALLED");
	rc = configure_af_device(i_con,IPR_INCLUDE);
	return rc;
}

int af_remove(i_container *i_con)
{
	int rc;
	mvaddstr(0,0,"AF REMOVE FUNCTION CALLED");
	rc = configure_af_device(i_con,IPR_REMOVE);
	return rc;
}

int configure_af_device(i_container *i_con, int action_code)
{
	int rc, j, k;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_dev_record *device_record;
	int can_init;
	int dev_type;
	int change_size;
	char *buffer[2];
	int num_devs = 0;
	struct ipr_ioa *cur_ioa;
	struct devs_to_init_t *cur_dev_init;
	u8 ioctl_buffer[IOCTL_BUFFER_SIZE];
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	struct ipr_block_desc *block_desc;
	int status;
	u8 length;
	char *string_buf;
	int toggle = 1;
	int header_lines;
	struct screen_output *s_out;
	mvaddstr(0,0,"CONFIGURE AF DEVICE  FUNCTION CALLED");

	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);

	/* Setup screen title */
	if (action_code == IPR_INCLUDE) {
		string_buf = catgets(catd,n_init_device.text_set,1,
				     "Select Disk Units to format for Advanced Function");
		n_init_device.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_init_device.title, string_buf);
	} else {
		string_buf = catgets(catd,n_init_device.text_set,2,
				     "Select Disk Units to format for JBOD Function");
		n_init_device.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_init_device.title, string_buf);
	}

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_init_device, &header_lines, 4, 5, "1=", k);

	for (cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next)  {

		for (j = 0; j < cur_ioa->num_devices; j++) {

			can_init = 1;
			scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;

			/* If not a DASD, disallow format */
			if ((scsi_dev_data == NULL) ||
			    (ipr_is_hot_spare(&cur_ioa->dev[j])) ||
			    (ipr_is_volume_set(&cur_ioa->dev[j])) ||
			    ((scsi_dev_data->type != TYPE_DISK) &&
			     (scsi_dev_data->type != IPR_TYPE_AF_DISK)))
				continue;

			/* If Advanced Function DASD */
			if (ipr_is_af_dasd_device(&cur_ioa->dev[j])) {

				if (action_code == IPR_INCLUDE)
					continue;

				device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
				if ((device_record == NULL) ||
				    (device_record->common.record_id != IPR_RECORD_ID_DEVICE_RECORD))
					continue;

				dev_type = IPR_AF_DASD_DEVICE;
				change_size  = IPR_512_BLOCK;

				/* We allow the user to format the drive if nobody is using it */
				if (cur_ioa->dev[j].scsi_dev_data->opens != 0) {

					syslog(LOG_ERR, "Format not allowed to %s, device in use\n", cur_ioa->dev[j].gen_name); 
					continue;
				}

				if (ipr_is_array_member(&cur_ioa->dev[j])) {

					if (device_record->no_cfgte_vol)
						can_init = 1;
					else
						can_init = 0;
				} else
					can_init = is_format_allowed(&cur_ioa->dev[j]);

			} else if (scsi_dev_data->type == TYPE_DISK) {

				if (action_code == IPR_REMOVE)
					continue;

				dev_type = IPR_JBOD_DASD_DEVICE;
				change_size = IPR_522_BLOCK;

				/* We allow the user to format the drive if nobody is using it, or
				 the device is read write protected. If the drive fails, then is replaced
				 concurrently it will be read write protected, but the system may still
				 have a use count for it. We need to allow the format to get the device
				 into a state where the system can use it */
				if (cur_ioa->dev[j].scsi_dev_data->opens != 0) {
					syslog(LOG_ERR, "Format not allowed to %s, device in use\n", cur_ioa->dev[j].gen_name); 
					continue;
				}

				if (is_af_blocked(&cur_ioa->dev[j], 0)) {

					/* error log is posted in is_af_blocked() routine */
					continue;
				}

				/* check if mode select with 522 block size will work, will be
				 set back to 512 if successfull otherwise will not be candidate. */
				mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
				memset(ioctl_buffer, 0, 255);

				mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);

				block_desc = (struct ipr_block_desc *)(mode_parm_hdr + 1);

				/* Setup block size */
				block_desc->block_length[0] = 0x00;
				block_desc->block_length[1] = 0x02;
				block_desc->block_length[2] = 0x0a;

				length = sizeof(struct ipr_block_desc) +
					sizeof(struct ipr_mode_parm_hdr);

				status = ipr_mode_select(&cur_ioa->dev[j],
							 &ioctl_buffer, length);

				if (status)
					continue;
				else  {
					mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
					memset(ioctl_buffer, 0, 255);

					mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);

					block_desc = (struct ipr_block_desc *)(mode_parm_hdr + 1);

					block_desc->block_length[0] = 0x00;
					block_desc->block_length[1] = 0x02;
					block_desc->block_length[2] = 0x00;

					length = sizeof(struct ipr_block_desc) +
						sizeof(struct ipr_mode_parm_hdr);

					status = ipr_mode_select(&cur_ioa->dev[j],
								 &ioctl_buffer, length);

					if (status)
						continue;
				}

				can_init = is_format_allowed(&cur_ioa->dev[j]);
			}
			else
				continue;

			if (can_init) {
				if (dev_init_head) {
					dev_init_tail->next = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));
					dev_init_tail = dev_init_tail->next;
				}
				else
					dev_init_head = dev_init_tail = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));

				memset(dev_init_tail, 0, sizeof(struct devs_to_init_t));

				dev_init_tail->ioa = cur_ioa;
				dev_init_tail->dev_type = dev_type;
				dev_init_tail->change_size = change_size;
				dev_init_tail->ipr_dev = &cur_ioa->dev[j];

				for (k=0; k<2; k++)
					buffer[k] = print_device(&cur_ioa->dev[j],buffer[k],"%1",cur_ioa, k);

				i_con = add_i_con(i_con,"\0",dev_init_tail);

				num_devs++;
			}
		}
	}

	if (!num_devs) {
		if (action_code == IPR_INCLUDE) {
			setup_fail_screen(&n_af_include_fail, 3, 7);

			s_out = screen_driver(&n_af_include_fail,0,i_con);

			free(n_af_include_fail.title);
			n_af_include_fail.title = NULL;
			free(n_af_include_fail.body);
			n_af_include_fail.body = NULL;
		} else {
			setup_fail_screen(&n_af_remove_fail, 3, 7);

			s_out = screen_driver(&n_af_remove_fail,0,i_con);

			free(n_af_remove_fail.title);
			n_af_remove_fail.title = NULL;
			free(n_af_remove_fail.body);
			n_af_remove_fail.body = NULL;
		}
	} else {
		toggle_field = 0;

		do {
			n_init_device.body = buffer[toggle&1];
			s_out = screen_driver(&n_init_device,header_lines,i_con);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		cur_dev_init = dev_init_head;
		while (cur_dev_init) {
			cur_dev_init = cur_dev_init->next;
			free(dev_init_head);
			dev_init_head = cur_dev_init;
		}
	}

	rc = s_out->rc;
	free(s_out);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_init_device.body = NULL;

	ipr_free(n_init_device.title);
	n_init_device.title = NULL;

	return rc;
}

int add_hot_spare(i_container *i_con)
{
	int rc;
	mvaddstr(0,0,"ADD HOT SPARE FUNCTION CALLED");
	do {
		rc = hot_spare(i_con, IPR_ADD_HOT_SPARE);
	} while (rc & REFRESH_FLAG);
	return rc;
}

int remove_hot_spare(i_container *i_con)
{
	int rc;
	mvaddstr(0,0,"REMOVE HOT SPARE FUNCTION CALLED");
	do {
		rc = hot_spare(i_con, IPR_RMV_HOT_SPARE);
	} while (rc & REFRESH_FLAG);
	return rc;
}

int hot_spare(i_container *i_con, int action)
{
	int rc, i, k;
	struct ipr_common_record *common_record;
	struct ipr_array_record *array_record;
	struct ipr_dev_record *device_record;
	struct array_cmd_data *cur_raid_cmd;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	int found = 0;
	char *input;
	int toggle=1;
	i_container *temp_i_con;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	mvaddstr(0,0,"BUS CONFIG FUNCTION CALLED");

	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);

	if (action == IPR_ADD_HOT_SPARE) {
		/* Setup screen title */
		string_buf = catgets(catd,n_hot_spare.text_set,1,"Configure a hot spare device");
		n_hot_spare.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_hot_spare.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_hot_spare, &header_lines, 2, 4, "1=", k);
	} else {
		/* Setup screen title */
		string_buf = catgets(catd,n_hot_spare.text_set,5,"Unconfigure a hot spare device");
		n_hot_spare.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_hot_spare.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_hot_spare, &header_lines, 6, 8, "1=", k);
	}

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {

		for (i = 0; i < cur_ioa->num_devices; i++) {
			common_record = cur_ioa->dev[i].qac_entry;

			if (common_record == NULL)
				continue;

			array_record = (struct ipr_array_record *)common_record;
			device_record = (struct ipr_dev_record *)common_record;

			if (((common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) &&
			     (!array_record->established) &&
			     (action == IPR_ADD_HOT_SPARE)) ||
			    ((common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
			     (device_record->is_hot_spare) &&
			     (action == IPR_RMV_HOT_SPARE)))  {

				if (raid_cmd_head) {
					raid_cmd_tail->next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
					raid_cmd_tail = raid_cmd_tail->next;
				} else
					raid_cmd_head = raid_cmd_tail = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));

				memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

				raid_cmd_tail->ipr_ioa = cur_ioa;
				raid_cmd_tail->ipr_dev = NULL;

				i_con = add_i_con(i_con,"\0",raid_cmd_tail);

				for (k=0; k<2; k++) 
					buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"%1",cur_ioa, k);

				found++;
				break;
			}
		}
	}

	if (!found)
	{
		if (action == IPR_ADD_HOT_SPARE) {
			setup_fail_screen(&n_add_hot_spare_fail, 3, 7);

			s_out = screen_driver(&n_add_hot_spare_fail,0,i_con);

			free(n_add_hot_spare_fail.title);
			n_add_hot_spare_fail.title = NULL;
			free(n_add_hot_spare_fail.body);
			n_add_hot_spare_fail.body = NULL;
		} else {
			setup_fail_screen(&n_remove_hot_spare_fail, 3, 7);

			s_out = screen_driver(&n_remove_hot_spare_fail,0,i_con);

			free(n_remove_hot_spare_fail.title);
			n_remove_hot_spare_fail.title = NULL;
			free(n_remove_hot_spare_fail.body);
			n_remove_hot_spare_fail.body = NULL;
		}

		rc = EXIT_FLAG;
		goto leave;
	}

	do {
		n_hot_spare.body = buffer[toggle&1];
		s_out = screen_driver(&n_hot_spare,header_lines,i_con);
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

	for (temp_i_con = i_con_head; temp_i_con != NULL;
	temp_i_con = temp_i_con->next_item) {

		cur_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
		if (cur_raid_cmd != NULL)  {
			input = temp_i_con->field_data;

			if (strcmp(input, "1") == 0) {
				do {
					rc = select_hot_spare(temp_i_con, action);
				} while (rc & REFRESH_FLAG);

				if (rc > 0)
					goto leave;

				found = 1;

				if (!cur_raid_cmd->do_cmd)
					cur_raid_cmd->do_cmd = 1;
			} else {
				if (cur_raid_cmd->do_cmd)
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

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_hot_spare.body = NULL;

	ipr_free(n_hot_spare.title);
	n_hot_spare.title = NULL;

	cur_raid_cmd = raid_cmd_head;
	while(cur_raid_cmd) {
		cur_raid_cmd = cur_raid_cmd->next;
		free(raid_cmd_head);
		raid_cmd_head = cur_raid_cmd;
	}

	return rc;
}

int select_hot_spare(i_container *i_con, int action)
{
	struct array_cmd_data *cur_raid_cmd;
	int rc, j, k, found;
	char *buffer[2];
	int num_devs = 0;
	struct ipr_ioa *cur_ioa;
	char *input;
	struct ipr_dev *ipr_dev;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_dev_record *device_record;
	i_container *temp_i_con,*i_con2=NULL;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	i_container *i_con_head_saved;

	mvaddstr(0,0,"SELECT HOT SPARE FUNCTION CALLED");

	if (action == IPR_ADD_HOT_SPARE) {
		/* Setup screen title */
		string_buf = catgets(catd,n_select_hot_spare.text_set,1,
				     "Select Disk Units to Enable as Hot Spare");
		n_select_hot_spare.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_select_hot_spare.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_select_hot_spare, &header_lines, 2, 3, "1=", k);
	} else {
		/* Setup screen title */
		string_buf = catgets(catd,n_select_hot_spare.text_set,4,
				     "Select Disk Units to Disable as Hot Spare");
		n_select_hot_spare.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_select_hot_spare.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_select_hot_spare, &header_lines, 5, 6, "1=", k);
	}

	cur_raid_cmd = (struct array_cmd_data *) i_con->data;
	rc = RC_SUCCESS;
	cur_ioa = cur_raid_cmd->ipr_ioa;
	i_con_head_saved = i_con_head; /* FIXME */
	i_con_head = NULL;

	for (j = 0; j < cur_ioa->num_devices; j++) {

		scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
		device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;

		if ((scsi_dev_data == NULL) ||
		    (device_record == NULL))
			continue;

		if ((scsi_dev_data->type != IPR_TYPE_ADAPTER) &&
		    (device_record != NULL) &&
		    (device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
		    (((device_record->add_hot_spare_cand) && (action == IPR_ADD_HOT_SPARE)) ||
		     ((device_record->rmv_hot_spare_cand) && (action == IPR_RMV_HOT_SPARE))))  {

			i_con2 = add_i_con(i_con2,"\0",&cur_ioa->dev[j]);

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->dev[j],buffer[k],"%1",cur_ioa, k);

			num_devs++;
		}
	}

	if (num_devs) {
		do {
			n_select_hot_spare.body = buffer[toggle&1];
			s_out = screen_driver(&n_select_hot_spare,header_lines,i_con2);
			toggle++;
		} while (s_out->rc == TOGGLE_SCREEN);

		for (k=0; k<2; k++) {
			ipr_free(buffer[k]);
			buffer[k] = NULL;
		}
		n_select_hot_spare.body = NULL;

		ipr_free(n_select_hot_spare.title);
		n_select_hot_spare.title = NULL;

		rc = s_out->rc;
		i_con2 = s_out->i_con;

		if (rc > 0)
			goto leave;

		found = 0;

		for (temp_i_con = i_con_head; temp_i_con != NULL;
		temp_i_con = temp_i_con->next_item) {

			ipr_dev = (struct ipr_dev *)temp_i_con->data;
			if (ipr_dev != NULL) {
				input = temp_i_con->field_data;
				if (strcmp(input, "1") == 0) {
					found = 1;
					device_record = (struct ipr_dev_record *)ipr_dev->qac_entry;
					device_record->issue_cmd = 1;
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
	struct ipr_dev_record *device_record;
	struct ipr_ioa *cur_ioa;
	int rc, j, k;
	char *buffer[2];
	int hot_spare_complete(int action);
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	mvaddstr(0,0,"CONFIRM HOT SPARE FUNCTION CALLED");

	rc = RC_SUCCESS;

	if (action == IPR_ADD_HOT_SPARE) {
		/* Setup screen title */
		string_buf = catgets(catd,n_confirm_hot_spare.text_set,1,
				     "Select Disk Units to Enable as Hot Spare");
		n_confirm_hot_spare.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_confirm_hot_spare.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_confirm_hot_spare, &header_lines, 2, 4, "q=", k);
	} else {
		/* Setup screen title */
		string_buf = catgets(catd,n_confirm_hot_spare.text_set,5,
				     "Select Disk Units to Disable as Hot Spare");
		n_confirm_hot_spare.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_confirm_hot_spare.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_confirm_hot_spare, &header_lines, 6, 8, "q=", k);
	}

	for (cur_raid_cmd = raid_cmd_head; cur_raid_cmd;
	cur_raid_cmd = cur_raid_cmd->next) {

		if (!cur_raid_cmd->do_cmd)
			continue;

		cur_ioa = cur_raid_cmd->ipr_ioa;

		for (j = 0; j < cur_ioa->num_devices; j++) {

			device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;

			if ((device_record != NULL) &&
			    (device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
			    (device_record->issue_cmd)) {

				for (k=0; k<2; k++)
					buffer[k] = print_device(&cur_ioa->dev[j],buffer[k],"1",cur_ioa, k);
			}
		}
	}

	do {
		n_confirm_hot_spare.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_hot_spare,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_hot_spare.body = NULL;

	ipr_free(n_confirm_hot_spare.title);
	n_confirm_hot_spare.title = NULL;

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
	struct ipr_ioa *cur_ioa;
	struct array_cmd_data *cur_raid_cmd;

	/* now issue the start array command with "known to be zero" */
	for (cur_raid_cmd = raid_cmd_head; cur_raid_cmd != NULL; 
	     cur_raid_cmd = cur_raid_cmd->next) {

		if (cur_raid_cmd->do_cmd == 0)
			continue;

		cur_ioa = cur_raid_cmd->ipr_ioa;

		flush_stdscr();

		if (action == IPR_ADD_HOT_SPARE)
			rc = ipr_add_hot_spare(cur_ioa);
		else
			rc = ipr_remove_hot_spare(cur_ioa);

		if (rc != 0)
			return RC_FAILED;
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
	char *string_buf;
	int loop;
	mvaddstr(0,0,"DISK UNIT RECOVERY FUNCTION CALLED");

	string_buf = catgets(catd,n_disk_unit_recovery.text_set,1,"Title");
	n_disk_unit_recovery.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_disk_unit_recovery.title, string_buf);

	for (loop = 0; loop < n_disk_unit_recovery.num_opts; loop++) {
		n_disk_unit_recovery.body = ipr_list_opts(n_disk_unit_recovery.body,
							  n_disk_unit_recovery.options[loop].key,
							  n_disk_unit_recovery.text_set,
							  n_disk_unit_recovery.options[loop].cat_num);
	}
	n_disk_unit_recovery.body = ipr_end_list(n_disk_unit_recovery.body);

	s_out = screen_driver(&n_disk_unit_recovery,0,NULL);
	ipr_free(n_disk_unit_recovery.body);
	n_disk_unit_recovery.body = NULL;
	ipr_free(n_disk_unit_recovery.title);
	n_disk_unit_recovery.title = NULL;

	rc = s_out->rc;
	free(s_out);
	return rc;
}

int process_conc_maint(i_container *i_con, int action)
{
	i_container *temp_i_con;
	int found = 0;
	struct ipr_dev *ipr_dev;
	char *input;
	struct ipr_ioa *cur_ioa;
	int i, j, k, rc;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_encl_status_ctl_pg ses_data;
	char *buffer[2];
	char *string_buf;
	int header_lines;
	int toggle=1;
	struct screen_output *s_out;
	struct ipr_dev_record *dev_record;
	

	for (temp_i_con = i_con_head; temp_i_con != NULL;
	     temp_i_con = temp_i_con->next_item) {

		ipr_dev = (struct ipr_dev *)temp_i_con->data;
		if (ipr_dev == NULL)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "1") == 0)
			found++;
	}

	if (found != 1)
		return INVALID_OPTION_STATUS;

	if ((ipr_is_af_dasd_device(ipr_dev)) &&
	    ((action == IPR_VERIFY_CONC_REMOVE) ||
	     (action == IPR_WAIT_CONC_REMOVE))) {

		/* issue suspend device bus to verify operation
		 is allowable */
		dev_record = (struct ipr_dev_record *)ipr_dev->qac_entry;
		rc = ipr_suspend_device_bus(ipr_dev->ioa,
					    &dev_record->resource_addr,
					    IPR_SDB_CHECK_ONLY);

		if (rc)
			return INVALID_OPTION_STATUS; /* FIXME */

	} else if (num_device_opens(ipr_dev->scsi_dev_data->host,
				    ipr_dev->scsi_dev_data->channel,
				    ipr_dev->scsi_dev_data->id,
				    ipr_dev->scsi_dev_data->lun))
		return INVALID_OPTION_STATUS;  /* FIXME */

	cur_ioa = ipr_dev->ioa;

	/* flash light at location of specified device */
	for (j = 0; j < cur_ioa->num_devices; j++) {
		scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
		if ((scsi_dev_data == NULL) ||
		    (scsi_dev_data->type != IPR_TYPE_SES))
			continue;

		rc = ipr_receive_diagnostics(&cur_ioa->dev[j], 2, &ses_data,
					     sizeof(struct ipr_encl_status_ctl_pg));

		if ((rc) ||
		    (scsi_dev_data->channel != ipr_dev->scsi_dev_data->channel))
			continue;

		for (i=0; i<((ntohs(ses_data.byte_count)-8)/sizeof(struct ipr_drive_elem_status)); i++) {
			if (scsi_dev_data->id == ses_data.elem_status[i].scsi_id) {
				found++;

				if ((action == IPR_VERIFY_CONC_REMOVE)  ||
				    (action == IPR_VERIFY_CONC_ADD)) {
					ses_data.elem_status[i].select = 1;
					ses_data.elem_status[i].identify = 1;
				} else if (action == IPR_WAIT_CONC_REMOVE) {
					ses_data.elem_status[i].select = 1;
					ses_data.elem_status[i].remove = 1;
				} else if (action == IPR_WAIT_CONC_ADD) {
					ses_data.elem_status[i].select = 1;
					ses_data.elem_status[i].insert = 1;
				}
				break;
			}
		}

		break;
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	if ((action == IPR_WAIT_CONC_REMOVE) ||
	    (action == IPR_WAIT_CONC_ADD)) {
		ses_data.overall_status_select = 1;
		ses_data.overall_status_disable_resets = 1;
		ses_data.overall_status_insert = 0;
		ses_data.overall_status_remove = 0;
		ses_data.overall_status_identify = 0;
	}

	rc = ipr_send_diagnostics(&cur_ioa->dev[j], &ses_data,
				  sizeof(struct ipr_encl_status_ctl_pg));

	if (rc)
		return RC_FAILED;

	if (action == IPR_VERIFY_CONC_REMOVE) {
		/* Setup screen title */
		string_buf = catgets(catd,n_process_conc_maint.text_set,1,"Verify Device Concurrent Remove");
		n_process_conc_maint.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_process_conc_maint.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_process_conc_maint, &header_lines, 2, 4, "q=", k);
	} else if (action == IPR_VERIFY_CONC_ADD) {
		/* Setup screen title */
		string_buf = catgets(catd,n_process_conc_maint.text_set,5,"Verify Device Concurrent Add");
		n_process_conc_maint.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_process_conc_maint.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_process_conc_maint, &header_lines, 6, 8, "q=", k);
	} else if (action == IPR_WAIT_CONC_REMOVE) {
		/* Setup screen title */
		string_buf = catgets(catd,n_process_conc_maint.text_set,9,"Complete Device Concurrent Remove");
		n_process_conc_maint.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_process_conc_maint.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_process_conc_maint, &header_lines, 10, 12, "q=", k);
	} else {
		/* Setup screen title */
		string_buf = catgets(catd,n_process_conc_maint.text_set,13,"Complete Device Concurrent Add");
		n_process_conc_maint.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_process_conc_maint.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_process_conc_maint, &header_lines, 14, 16, "q=", k);
	}

	for (k=0; k<2; k++)
		buffer[k] = print_device(ipr_dev,buffer[k],"1",cur_ioa, k);

	/* call screen driver */
	do {
		n_process_conc_maint.body = buffer[toggle&1];
		s_out = screen_driver(&n_process_conc_maint,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);
	rc = s_out->rc;

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_process_conc_maint.body = NULL;

	ipr_free(n_process_conc_maint.title);
	n_process_conc_maint.title = NULL;

	/* turn light off flashing light */
	ses_data.overall_status_select = 1;
	ses_data.overall_status_disable_resets = 1;
	ses_data.overall_status_insert = 0;
	ses_data.overall_status_remove = 0;
	ses_data.overall_status_identify = 0;
	ses_data.elem_status[i].select = 1;
	ses_data.elem_status[i].insert = 0;
	ses_data.elem_status[i].remove = 0;
	ses_data.elem_status[i].identify = 0;

	ipr_send_diagnostics(&cur_ioa->dev[j], &ses_data,
			     sizeof(struct ipr_encl_status_ctl_pg));

	/* call function to complete conc maint */
	if (!rc) {
		if (action == IPR_VERIFY_CONC_REMOVE)
			rc = process_conc_maint(i_con, IPR_WAIT_CONC_REMOVE);
		else if (action == IPR_VERIFY_CONC_ADD)
			rc = process_conc_maint(i_con, IPR_WAIT_CONC_ADD);
	}

	return rc;
}

int start_conc_maint(i_container *i_con, int action)
{
	int rc, i, j, k, l;
	char *buffer[2];
	int num_lines = 0;
	struct ipr_ioa *cur_ioa;
	struct scsi_dev_data *scsi_dev_data;
	struct screen_output *s_out;
	struct ipr_encl_status_ctl_pg ses_data;
	struct ipr_dev **local_dev = NULL;
	int local_dev_count = 0;
	u8 ses_channel;
	char *string_buf;
	int toggle = 1;
	int found;
	int header_lines;
	mvaddstr(0,0,"CONCURRENT REMOVE DEVICE FUNCTION CALLED");

	rc = RC_SUCCESS;
	i_con = free_i_con(i_con);

	check_current_config(false);

	/* Setup screen title */
	if (action == IPR_CONC_REMOVE) {
		string_buf = catgets(catd,n_concurrent_remove_device.text_set,1,"Concurrent Device Remove");
		n_concurrent_remove_device.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_concurrent_remove_device.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_concurrent_remove_device, &header_lines, 2, 3, "1=", k);
	} else {
		string_buf = catgets(catd,n_concurrent_remove_device.text_set,4,"Concurrent Device Add");
		n_concurrent_remove_device.title = ipr_malloc(strlen(string_buf) + 4);
		sprintf(n_concurrent_remove_device.title, string_buf);

		for (k=0; k<2; k++)
			buffer[k] = body_init(&n_concurrent_remove_device, &header_lines, 5, 6, "1=", k);
	}

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {
		for (j = 0; j < cur_ioa->num_devices; j++) {
			scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
			if ((scsi_dev_data == NULL) ||
			    (scsi_dev_data->type != IPR_TYPE_SES))
				continue;

			rc = ipr_receive_diagnostics(&cur_ioa->dev[j], 2, &ses_data,
						     sizeof(struct ipr_encl_status_ctl_pg));

			if (rc)
				continue;

			ses_channel = scsi_dev_data->channel;

			for (i=0; i<((ntohs(ses_data.byte_count)-8)/sizeof(struct ipr_drive_elem_status)); i++) {
				found = 0;

				for (l=0; l<cur_ioa->num_devices; l++) {
					scsi_dev_data = cur_ioa->dev[l].scsi_dev_data;

					if ((scsi_dev_data->channel == ses_channel) &&
					    (scsi_dev_data->id == ses_data.elem_status[i].scsi_id)) {

						found++;
						if (action == IPR_CONC_ADD)
							continue;

						for (k=0; k<2; k++)
							buffer[k] = print_device(&cur_ioa->dev[l],buffer[k],"%1",cur_ioa, k);
						i_con = add_i_con(i_con,"\0", &cur_ioa->dev[l]);

						num_lines++;
					}
				}

				if (!found) {
					local_dev = realloc(local_dev, (sizeof(void *) *
									local_dev_count) + 1);
					local_dev[local_dev_count] = calloc(1,sizeof(struct ipr_dev));
					scsi_dev_data = calloc(1, sizeof(struct scsi_dev_data));
					scsi_dev_data->type = IPR_TYPE_EMPTY_SLOT;
					scsi_dev_data->channel = ses_channel;
					scsi_dev_data->id = scsi_dev_data->id;
					local_dev[local_dev_count]->scsi_dev_data = scsi_dev_data;

					for (k=0; k<2; k++)
						buffer[k] = print_device(local_dev[local_dev_count],buffer[k],"%1",cur_ioa, k);
					i_con = add_i_con(i_con,"\0", local_dev[local_dev_count]);

					num_lines++;
				}
			}
		}
	}

	if (num_lines == 0) {
		for (k=0; k<2; k++) 
			buffer[k] = add_line_to_body(buffer[k], "(No Eligible Devices Found)",
						     "", NULL);
	}

	do {
		n_concurrent_remove_device.body = buffer[toggle&1];
		s_out = screen_driver(&n_concurrent_remove_device,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_concurrent_remove_device.body = NULL;

	ipr_free(n_concurrent_remove_device.title);
	n_concurrent_remove_device.title = NULL;

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

	for (k=0; k<local_dev_count; k++) {
		if (!local_dev[k])
			continue;
		if (local_dev[k]->scsi_dev_data)
			free(local_dev[k]->scsi_dev_data);
		free(local_dev[k]);
	}
	free(local_dev);
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
	int rc, j, k;
	struct ipr_query_res_state res_state;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_dev_record *device_record;
	int can_init;
	int dev_type;
	char *buffer[2];
	int num_devs = 0;
	struct ipr_ioa *cur_ioa;
	struct devs_to_init_t *cur_dev_init;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	mvaddstr(0,0,"INIT DEVICE FUNCTION CALLED");

	rc = RC_SUCCESS;

	i_con = free_i_con(i_con);

	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_init_device.text_set,3,"Configure a hot spare device");
	n_init_device.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_init_device.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_init_device, &header_lines, 4, 5, "1=", k);

	for (cur_ioa = ipr_ioa_head; cur_ioa != NULL;
	cur_ioa = cur_ioa->next) {

		for (j = 0; j < cur_ioa->num_devices; j++)  {
			can_init = 1;
			scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;

			/* If not a DASD, disallow format */
			if ((scsi_dev_data == NULL) ||
			    (ipr_is_hot_spare(&cur_ioa->dev[j])) ||
			    (ipr_is_volume_set(&cur_ioa->dev[j])) ||
			    ((scsi_dev_data->type != TYPE_DISK) &&
			     (scsi_dev_data->type != IPR_TYPE_AF_DISK)))
				continue;

			/* If Advanced Function DASD */
			if (ipr_is_af_dasd_device(&cur_ioa->dev[j])) {

				dev_type = IPR_AF_DASD_DEVICE;

				device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
				if (device_record &&
				    (device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD)) {

					/* We allow the user to format the drive if nobody is using it */
					if (cur_ioa->dev[j].scsi_dev_data->opens != 0) {
						syslog(LOG_ERR,
						       "Format not allowed to %s, device in use\n",
						       cur_ioa->dev[j].gen_name); 
						continue;
					}

					if (ipr_is_array_member(&cur_ioa->dev[j])) {
						if (device_record->no_cfgte_vol)
							can_init = 1;
						else
							can_init = 0;
					} else
						can_init = is_format_allowed(&cur_ioa->dev[j]);
				} else {
					/* Do a query resource state */
					rc = ipr_query_resource_state(&cur_ioa->dev[j], &res_state);

					if (rc != 0)
						continue;

					/* We allow the user to format the drive if 
					 the device is read write protected. If the drive fails, then is replaced
					 concurrently it will be read write protected, but the system may still
					 have a use count for it. We need to allow the format to get the device
					 into a state where the system can use it */
					if ((cur_ioa->dev[j].scsi_dev_data->opens != 0) &&
					    (!res_state.read_write_prot)) {
						syslog(LOG_ERR,
						       "Format not allowed to %s, device in use\n",
						       cur_ioa->dev[j].gen_name); 
						continue;
					}

					if (ipr_is_array_member(&cur_ioa->dev[j])) {
						/* If the device is an array member, only allow a format to it */
						/* if it is read/write protected by the IOA */
						if ((res_state.read_write_prot) && !(res_state.not_ready))
							can_init = 1;
						else
							can_init = 0;
					} else if ((res_state.not_oper) || (res_state.not_ready))
						/* Device is failed - cannot talk to the device */
						can_init = 0;
					else
						can_init = is_format_allowed(&cur_ioa->dev[j]);
				}
			} else if (scsi_dev_data->type == TYPE_DISK) {

				/* We allow the user to format the drive if nobody is using it */
				if (cur_ioa->dev[j].scsi_dev_data->opens != 0) {
					syslog(LOG_ERR,
					       "Format not allowed to %s, device in use\n",
					       cur_ioa->dev[j].gen_name); 
					continue;
				}

				dev_type = IPR_JBOD_DASD_DEVICE;
				can_init = is_format_allowed(&cur_ioa->dev[j]);
			} else
				continue;

			if (can_init) {
				if (dev_init_head) {
					dev_init_tail->next = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));
					dev_init_tail = dev_init_tail->next;
				}
				else
					dev_init_head = dev_init_tail = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));

				memset(dev_init_tail, 0, sizeof(struct devs_to_init_t));

				dev_init_tail->ioa = cur_ioa;
				dev_init_tail->dev_type = dev_type;
				dev_init_tail->ipr_dev = &cur_ioa->dev[j];
				dev_init_tail->change_size = 0;

				for (k=0; k<2; k++)
					buffer[k] = print_device(&cur_ioa->dev[j], buffer[k],"%1",cur_ioa, k);
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

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_init_device.body = NULL;

	ipr_free(n_init_device.title);
	n_init_device.title = NULL;

	cur_dev_init = dev_init_head;
	while(cur_dev_init) {
		cur_dev_init = cur_dev_init->next;
		free(dev_init_head);
		dev_init_head = cur_dev_init;
	}

	return rc;
}

int confirm_init_device(i_container *i_con)
{
	int found;
	char *input;
	char *buffer[2];
	struct devs_to_init_t *cur_dev_init;
	int rc = RC_SUCCESS;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	int k;
	i_container *temp_i_con;
	mvaddstr(0,0,"CONFIRM INIT DEVICE FUNCTION CALLED");
	found = 0;

	/* Setup screen title */
	string_buf = catgets(catd,n_confirm_init_device.text_set,1,"Create a Disk Array");
	n_confirm_init_device.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_init_device.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_confirm_init_device, &header_lines, 2, 3,"q=",k);

	for (temp_i_con = i_con_head;
	     temp_i_con != NULL; temp_i_con = temp_i_con->next_item) {

		if (temp_i_con->data == NULL)
			continue;

		input = temp_i_con->field_data;        
		if (strcmp(input, "1") == 0) {
			found = 1;
			cur_dev_init =(struct devs_to_init_t *)(temp_i_con->data);
			cur_dev_init->do_init = 1;

			for (k=0; k<2; k++)
				buffer[k] = print_device(cur_dev_init->ipr_dev,buffer[k],"1",cur_dev_init->ioa, k);
		}
	}

	if (!found)
		return INVALID_OPTION_STATUS;

	do {
		n_confirm_init_device.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_init_device,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_init_device.body = NULL;

	ipr_free(n_confirm_init_device.title);
	n_confirm_init_device.title = NULL;

	return rc;
}

int send_dev_inits(i_container *i_con)
{
	u8 num_devs = 0;
	struct devs_to_init_t *cur_dev_init;
	int rc;
	struct ipr_query_res_state res_state;
	u8 ioctl_buffer[IOCTL_BUFFER_SIZE];
	u8 ioctl_buffer2[IOCTL_BUFFER_SIZE];
	struct ipr_control_mode_page *control_mode_page;
	struct ipr_control_mode_page *control_mode_page_changeable;
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	struct ipr_block_desc *block_desc;
	struct scsi_dev_data *scsi_dev_data;
	int status;
	int opens;
	u8 failure = 0;
	int is_520_block = 0;
	int max_y, max_x;
	u8 length;

	int dev_init_complete(u8 num_devs);

	getmaxyx(stdscr,max_y,max_x);
	mvaddstr(max_y-1,0,catgets(catd,n_dev_init_complete.text_set,4,"Please wait for the next screen."));
	refresh();

	for (cur_dev_init = dev_init_head;
	     cur_dev_init != NULL;
	     cur_dev_init = cur_dev_init->next)
	{
		if ((cur_dev_init->do_init) &&
		    (cur_dev_init->dev_type == IPR_AF_DASD_DEVICE)) {

			num_devs++;

			rc = ipr_query_resource_state(cur_dev_init->ipr_dev, &res_state);

			if (rc != 0) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			scsi_dev_data = cur_dev_init->ipr_dev->scsi_dev_data;
			if (scsi_dev_data == NULL) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			opens = num_device_opens(scsi_dev_data->host,
						 scsi_dev_data->channel,
						 scsi_dev_data->id,
						 scsi_dev_data->lun);

			if ((opens != 0) && (!res_state.read_write_prot))
			{
				syslog(LOG_ERR,
				       "Cannot obtain exclusive access to %s\n",
				       cur_dev_init->ipr_dev->gen_name);
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* Mode sense */
			rc = ipr_mode_sense(cur_dev_init->ipr_dev, 0x0a, ioctl_buffer);

			if (rc != 0)
			{
				syslog(LOG_ERR, "Mode Sense to %s failed. %m\n",
				       cur_dev_init->ipr_dev->gen_name);
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* Mode sense - changeable parms */
			rc = ipr_mode_sense(cur_dev_init->ipr_dev, 0x4a, ioctl_buffer2);

			if (rc != 0)
			{
				syslog(LOG_ERR, "Mode Sense to %s failed. %m\n",
				       cur_dev_init->ipr_dev->gen_name);
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

			block_desc = (struct ipr_block_desc *)(mode_parm_hdr + 1);
			if ((block_desc->block_length[0] == 0x00) &&
			    (block_desc->block_length[1] == 0x02) &&
			    (block_desc->block_length[2] == 0x08))
				/* block size is 520 */
				is_520_block = 1;

			control_mode_page = (struct ipr_control_mode_page *)
				(((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

			mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer2;

			control_mode_page_changeable = (struct ipr_control_mode_page *)
				(((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

			mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

			/* Turn off QERR since some drives do not like QERR
			 and IMMED bit at the same time. */
			IPR_SET_MODE(control_mode_page_changeable->qerr,
				     control_mode_page->qerr, 0);

			length = mode_parm_hdr->length + 1;
			mode_parm_hdr->length = 0;
			mode_parm_hdr->medium_type = 0;
			mode_parm_hdr->device_spec_parms = 0;
			control_mode_page->hdr.parms_saveable = 0;
			rc = ipr_mode_select(cur_dev_init->ipr_dev, ioctl_buffer, length);

			if (rc != 0) {
				syslog(LOG_ERR, "Mode Select %s to disable qerr failed. %m\n",
				       cur_dev_init->ipr_dev->gen_name);
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* Issue mode select to change block size */
			mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
			memset(ioctl_buffer, 0, 255);

			mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);

			block_desc = (struct ipr_block_desc *)(mode_parm_hdr + 1);

			/* Setup block size */
			block_desc->block_length[0] = 0x00;
			block_desc->block_length[1] = 0x02;

			if (cur_dev_init->change_size == IPR_512_BLOCK)
				block_desc->block_length[2] = 0x00;
			else if (is_520_block)
				block_desc->block_length[2] = 0x08;
			else
				block_desc->block_length[2] = 0x0a;

			rc = ipr_mode_select(cur_dev_init->ipr_dev,ioctl_buffer,
					     sizeof(struct ipr_block_desc) +
					     sizeof(struct ipr_mode_parm_hdr));

			if (rc != 0) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* Issue the format. Failure will be detected by query command status */
			rc = ipr_format_unit(cur_dev_init->ipr_dev);  /* FIXME  Mandatory lock? */
		}
		else if ((cur_dev_init->do_init) &&
			 (cur_dev_init->dev_type == IPR_JBOD_DASD_DEVICE))
		{
			num_devs++;

			scsi_dev_data = cur_dev_init->ipr_dev->scsi_dev_data;
			if (scsi_dev_data == NULL) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			opens = num_device_opens(scsi_dev_data->host,
						 scsi_dev_data->channel,
						 scsi_dev_data->id,
						 scsi_dev_data->lun);

			if (opens != 0)
			{
				syslog(LOG_ERR,
				       "Cannot obtain exclusive access to %s\n",
				       cur_dev_init->ipr_dev->gen_name);
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* Issue mode sense to get the control mode page */
			status = ipr_mode_sense(cur_dev_init->ipr_dev,
						0x0a, &ioctl_buffer);

			if (status)
			{
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* Issue mode sense to get the control mode page */
			status = ipr_mode_sense(cur_dev_init->ipr_dev,
						0x4a, &ioctl_buffer2);

			if (status)
			{
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer2;

			control_mode_page_changeable = (struct ipr_control_mode_page *)
				(((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

			mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

			control_mode_page = (struct ipr_control_mode_page *)
				(((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

			/* Turn off QERR since some drives do not like QERR
			 and IMMED bit at the same time. */
			IPR_SET_MODE(control_mode_page_changeable->qerr,
				     control_mode_page->qerr, 0);

			/* Issue mode select to set page x0A */
			length = mode_parm_hdr->length + 1;

			mode_parm_hdr->length = 0;
			control_mode_page->hdr.parms_saveable = 0;
			mode_parm_hdr->medium_type = 0;
			mode_parm_hdr->device_spec_parms = 0;

			status = ipr_mode_select(cur_dev_init->ipr_dev,
						 &ioctl_buffer, length);

			if (status)
			{
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* Issue mode select to setup block size */
			mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
			memset(ioctl_buffer, 0, 255);

			mode_parm_hdr->block_desc_len = sizeof(struct ipr_block_desc);

			block_desc = (struct ipr_block_desc *)(mode_parm_hdr + 1);

			/* Setup block size */
			block_desc->block_length[0] = 0x00;
			block_desc->block_length[1] = 0x02;

			if (cur_dev_init->change_size == IPR_522_BLOCK)
				block_desc->block_length[2] = 0x0a;
			else
				block_desc->block_length[2] = 0x00;

			length = sizeof(struct ipr_block_desc) +
				sizeof(struct ipr_mode_parm_hdr);

			status = ipr_mode_select(cur_dev_init->ipr_dev,
						 &ioctl_buffer, length);

			if (status) {
				cur_dev_init->do_init = 0;
				num_devs--;
				failure++;
				continue;
			}

			/* Issue format */
			status = ipr_format_unit(cur_dev_init->ipr_dev);  /* FIXME  Mandatory lock? */   

			if (status) {
				/* Send a device reset to cleanup any old state */
				rc = ipr_reset_device(cur_dev_init->ipr_dev);

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

int dev_init_complete(u8 num_devs)
{
	int done_bad;
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int not_done = 0;
	int rc;
	struct devs_to_init_t *cur_dev_init;
	u32 percent_cmplt = 0;
	u8 ioctl_buffer[255];
	struct sense_data_t sense_data;
	struct ipr_ioa *ioa;
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	struct ipr_block_desc *block_desc;

	n_dev_init_complete.title = catgets(catd,n_dev_init_complete.text_set,
					    1,"Initialize and Format Status");
	n_dev_init_complete.body = catgets(catd,n_dev_init_complete.text_set,
					   2,"You selected to initialize and format a disk unit");

	while(1) {
		rc = complete_screen_driver(&n_dev_init_complete, percent_cmplt,1);

		if (rc & EXIT_FLAG)
			return rc;

		percent_cmplt = 100;
		done_bad = 0;

		for (cur_dev_init = dev_init_head;  cur_dev_init != NULL;
		cur_dev_init = cur_dev_init->next) {

			if ((cur_dev_init->do_init) &&
			    (cur_dev_init->dev_type == IPR_AF_DASD_DEVICE)) {

				rc = ipr_query_command_status(cur_dev_init->ipr_dev,
							      &cmd_status);

				if ((rc) || (cmd_status.num_records == 0)) {
					continue;
				}

				status_record = cmd_status.record;
				if (status_record->status == IPR_CMD_STATUS_FAILED) {
					done_bad = 1;
				} else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL) {
					if (status_record->percent_complete < percent_cmplt)
						percent_cmplt = status_record->percent_complete;
					not_done = 1;
				}
			}
			else if ((cur_dev_init->do_init) &&
				 (cur_dev_init->dev_type == IPR_JBOD_DASD_DEVICE))
			{
				/* Send Test Unit Ready to find percent complete in sense data. */
				rc = ipr_test_unit_ready(cur_dev_init->ipr_dev,
							 &sense_data);

				if (rc < 0)
					continue;

				if ((rc == CHECK_CONDITION) &&
				    ((sense_data.error_code & 0x7F) == 0x70) &&
				    ((sense_data.sense_key & 0x0F) == 0x02)) {

					cur_dev_init->cmplt = ((int)sense_data.sense_key_spec[1]*100)/0x100;
					if (cur_dev_init->cmplt < percent_cmplt)
						percent_cmplt = cur_dev_init->cmplt;
					not_done = 1;
				}
			}
		}

		if (!not_done) {

			for (cur_dev_init = dev_init_head; cur_dev_init;
			     cur_dev_init = cur_dev_init->next) {

				if (!cur_dev_init->do_init)
					continue;

				ioa = cur_dev_init->ioa;

				if (cur_dev_init->dev_type == IPR_AF_DASD_DEVICE) {
					/* check if evaluate device is necessary */
					/* issue mode sense */
					rc = ipr_mode_sense(cur_dev_init->ipr_dev, 0, ioctl_buffer);

					if (rc == 0) {
						mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
						if (mode_parm_hdr->block_desc_len > 0) {
							block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

							if((block_desc->block_length[1] == 0x02) && 
							   (block_desc->block_length[2] == 0x00))
								cur_dev_init->change_size = IPR_512_BLOCK;
						}
					}
				} else if (cur_dev_init->dev_type == IPR_JBOD_DASD_DEVICE) {
					/* Check if evaluate device capabilities needs to be issued */
					memset(ioctl_buffer, 0, 255);

					/* Issue mode sense to get the block size */
					rc = ipr_mode_sense(cur_dev_init->ipr_dev,
							    0x00, mode_parm_hdr);

					if (rc == 0) {
						mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

						if (mode_parm_hdr->block_desc_len > 0) {
							block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

							if ((block_desc->block_length[1] != 0x02) ||
							    (block_desc->block_length[2] != 0x00))
								cur_dev_init->change_size = IPR_522_BLOCK;
						}
					}
				}

				if (cur_dev_init->change_size != 0)
					/* send evaluate device down */
					evaluate_device(cur_dev_init->ipr_dev,
							ioa, cur_dev_init->change_size);
			}

			flush_stdscr();

			if (done_bad)
				/* Initialize and format failed */
				return 51 | EXIT_FLAG;

			/* Initialize and format completed successfully */
			return 50 | EXIT_FLAG; 
		}

		not_done = 0;
		sleep(1);
	}
}

int reclaim_cache(i_container* i_con)
{
	int rc;
	struct ipr_ioa *cur_ioa;
	struct ipr_reclaim_query_data *reclaim_buffer;
	struct ipr_reclaim_query_data *cur_reclaim_buffer;
	int found = 0;
	char *buffer[2];
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	int k;
	mvaddstr(0,0,"RECLAIM CACHE FUNCTION CALLED");

	/* empty the linked list that contains field pointers */
	i_con = free_i_con(i_con);

	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_reclaim_cache.text_set,1,"Reclaim IOA Cache Storage");
	n_reclaim_cache.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_reclaim_cache.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_reclaim_cache, &header_lines, 2, 5, "1=", k);

	reclaim_buffer = (struct ipr_reclaim_query_data*)malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

	for (cur_ioa = ipr_ioa_head, cur_reclaim_buffer = reclaim_buffer;
	     cur_ioa != NULL; cur_ioa = cur_ioa->next, cur_reclaim_buffer++) {
		rc = ipr_reclaim_cache_store(cur_ioa,
					     IPR_RECLAIM_QUERY,
					     cur_reclaim_buffer);

		if (rc != 0) {
			cur_ioa->reclaim_data = NULL;
			continue;
		}

		cur_ioa->reclaim_data = cur_reclaim_buffer;

		if (cur_reclaim_buffer->reclaim_known_needed ||
		    cur_reclaim_buffer->reclaim_unknown_needed) {

			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"%1",cur_ioa, k);

			i_con = add_i_con(i_con, "\0",cur_ioa);
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

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_reclaim_cache.body = NULL;

	ipr_free(n_reclaim_cache.title);
	n_reclaim_cache.title = NULL;
		
	return rc;
}


int confirm_reclaim(i_container *i_con)
{
	struct ipr_ioa *cur_ioa, *reclaim_ioa;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_query_res_state res_state;
	int j, k, rc;
	char *buffer[2];
	i_container* temp_i_con;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	int ioa_num;
	mvaddstr(0,0,"CONFIRM RECLAIM CACHE FUNCTION CALLED");

	for (temp_i_con = i_con_head; temp_i_con;
	     temp_i_con = temp_i_con->next_item) {

		cur_ioa = (struct ipr_ioa *) temp_i_con->data;
		if (!cur_ioa)
			continue;

		if (strcmp(temp_i_con->field_data, "1") != 0)
			continue;

		if (cur_ioa->reclaim_data->reclaim_known_needed ||
		    cur_ioa->reclaim_data->reclaim_unknown_needed) {
			reclaim_ioa = cur_ioa;
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
	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_confirm_reclaim.text_set,1,"Create a Disk Array");
	n_confirm_reclaim.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_reclaim.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_confirm_reclaim, &header_lines, 2, 5, "q=", k);

	for (j = 0; j < reclaim_ioa->num_devices; j++) {

		scsi_dev_data = reclaim_ioa->dev[j].scsi_dev_data;
		if ((scsi_dev_data == NULL) ||
		    ((scsi_dev_data->type != TYPE_DISK) &&
		     (scsi_dev_data->type != IPR_TYPE_AF_DISK)))  /* FIXME correct check? */
				continue;

		/* Do a query resource state to see whether or not the
		 device will be affected by the operation */
		rc = ipr_query_resource_state(&reclaim_ioa->dev[j], &res_state);

		if (rc != 0)
			res_state.not_oper = 1;

		/* FIXME Necessary? We have not set this field. Relates to not_oper?*/
		if (!res_state.read_write_prot)
			continue;

		for (k=0; k<2; k++)
			buffer[k] = print_device(&reclaim_ioa->dev[j],buffer[k],"1",reclaim_ioa, k);
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

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_reclaim.body = NULL;

	ipr_free(n_confirm_reclaim.title);
	n_confirm_reclaim.title = NULL;

	return rc;
}

int reclaim_warning(i_container *i_con)
{
	struct ipr_ioa *reclaim_ioa;
	int k, rc;
	char *buffer[2];
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	mvaddstr(0,0,"CONFIRM RECLAIM CACHE FUNCTION CALLED");

	reclaim_ioa = (struct ipr_ioa *)i_con->data;
	if (!reclaim_ioa)
		/* "Invalid option.  No devices selected." */
		return 17; 

	/* Setup screen title */
	string_buf = catgets(catd,n_confirm_reclaim_warning.text_set,1,"Create a Disk Array");
	n_confirm_reclaim_warning.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_reclaim_warning.title, string_buf);

	for (k=0; k<2; k++) {
		buffer[k] = body_init(&n_confirm_reclaim_warning, &header_lines, 2, 4, "", k);
		buffer[k] = print_device(&reclaim_ioa->ioa,buffer[k],"1",reclaim_ioa, k);
	}

	do {
		n_confirm_reclaim_warning.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_reclaim_warning,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_reclaim_warning.body = NULL;

	ipr_free(n_confirm_reclaim_warning.title);
	n_confirm_reclaim_warning.title = NULL;

	return rc;
}

int reclaim_result(i_container *i_con)
{
	int rc;
	struct ipr_ioa *reclaim_ioa;
	int max_y,max_x;
	struct screen_output *s_out;
	int action;
	char out_str[32];
	char *string_buf;
	char *buffer = NULL;
	mvaddstr(0,0,"RECLAIM RESULT FUNCTION CALLED");

	reclaim_ioa = (struct ipr_ioa *) i_con->data;

	action = IPR_RECLAIM_PERFORM;

	if (reclaim_ioa->reclaim_data->reclaim_unknown_needed)
		action |= IPR_RECLAIM_UNKNOWN_PERM;

	rc = ipr_reclaim_cache_store(reclaim_ioa,
				     action,
				     reclaim_ioa->reclaim_data);

	if (rc != 0)
		/* "Reclaim IOA Cache Storage failed" */
		return (EXIT_FLAG | 37); 

	/* Everything going according to plan. Proceed with reclaim. */
	getmaxyx(stdscr,max_y,max_x);
	move(max_y-1,0);
	printw("Please wait - reclaim in progress");
	refresh();

	memset(reclaim_ioa->reclaim_data, 0, sizeof(struct ipr_reclaim_query_data));
	rc = ipr_reclaim_cache_store(reclaim_ioa,
				     IPR_RECLAIM_QUERY,
				     reclaim_ioa->reclaim_data);

	if (rc != 0)
		/* "Reclaim IOA Cache Storage failed" */
		return (EXIT_FLAG | 37); 

	/* Setup screen title */
	string_buf = catgets(catd,n_reclaim_result.text_set,1,"Reclaim IOA Cache Storage Results");
	n_reclaim_result.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_reclaim_result.title, string_buf);

	if (reclaim_ioa->reclaim_data->reclaim_known_performed) {
		string_buf = catgets(catd,n_reclaim_result.text_set,2,
				     "IOA cache storage reclamation has completed. "
				     "Use the number of lost sectors to decide whether "
				     "to restore data from the most recent save media "
				     "or to continue with possible data loss.");
		buffer = add_line_to_body(buffer, string_buf, "", NULL);
		n_reclaim_result.body = buffer;

		if (reclaim_ioa->reclaim_data->num_blocks_needs_multiplier)
			sprintf(out_str, "%12d",
				ntohs(reclaim_ioa->reclaim_data->num_blocks) *
				IPR_RECLAIM_NUM_BLOCKS_MULTIPLIER);
		else
			sprintf(out_str, "%12d",
				ntohs(reclaim_ioa->reclaim_data->num_blocks));

		body_details(&n_reclaim_result,3,out_str);

	} else if (reclaim_ioa->reclaim_data->reclaim_unknown_performed) {

		string_buf = catgets(catd,n_reclaim_result.text_set,4,
				     "IOA cache storage reclamation has completed. "
				     "The number of lost sectors could not be determined.");
		buffer = add_line_to_body(buffer, string_buf, "", NULL);
		n_reclaim_result.body = buffer;
	}

	s_out = screen_driver(&n_reclaim_result,0,NULL);
	free(s_out);

	ipr_free(n_reclaim_result.body);
	n_reclaim_result.body = NULL;
	ipr_free(n_reclaim_result.title);
	n_reclaim_result.title = NULL;

	rc = ipr_reclaim_cache_store(reclaim_ioa,
				     IPR_RECLAIM_RESET,
				     reclaim_ioa->reclaim_data);

	if (rc != 0)
		rc = (EXIT_FLAG | 37);

	return rc;
}

int battery_maint(i_container *i_con)
{
	int rc, k;
	struct ipr_reclaim_query_data *cur_reclaim_buffer;
	struct ipr_ioa *cur_ioa;
	int found = 0;
	char *buffer[2];
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	mvaddstr(0,0,"BATTERY MAINT FUNCTION CALLED");

	i_con = free_i_con(i_con);

	/* Setup screen title */
	string_buf = catgets(catd,n_battery_maint.text_set,1,
			     "Work with Resources Containing Cache Battery Packs");
	n_battery_maint.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_battery_maint.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init_double(&n_battery_maint,
					     &header_lines, 2, 4,"2=","5=", k);

	cur_reclaim_buffer = (struct ipr_reclaim_query_data *)
		malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

	for (cur_ioa = ipr_ioa_head; cur_ioa != NULL;
	     cur_ioa = cur_ioa->next, cur_reclaim_buffer++) {

		rc = ipr_reclaim_cache_store(cur_ioa,
					     IPR_RECLAIM_QUERY | IPR_RECLAIM_EXTENDED_INFO,
					     cur_reclaim_buffer);

		if (rc != 0) {
			cur_ioa->reclaim_data = NULL;
			continue;
		}

		cur_ioa->reclaim_data = cur_reclaim_buffer;

		if (cur_reclaim_buffer->rechargeable_battery_type) {
			i_con = add_i_con(i_con, "\0",cur_ioa); 
			for (k=0; k<2; k++)
				buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"%1",cur_ioa, k);
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

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_battery_maint.body = NULL;

	ipr_free(n_battery_maint.title);
	n_battery_maint.title = NULL;

	return rc;
}

int show_battery_info(struct ipr_ioa *ioa)
{
	int rc;
	struct ipr_reclaim_query_data *reclaim_data = ioa->reclaim_data;
	char buffer[128];
	char *string_buf;
	struct screen_output *s_out;
	struct ipr_ioa_vpd ioa_vpd;
	struct ipr_cfc_vpd cfc_vpd;
	u8 product_id[IPR_PROD_ID_LEN+1];
	u8 serial_num[IPR_SERIAL_NUM_LEN+1];

	string_buf = catgets(catd,n_show_battery_info.text_set,1,"Title");
	n_show_battery_info.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_show_battery_info.title, string_buf);


	memset(&ioa_vpd, 0, sizeof(ioa_vpd));
	memset(&cfc_vpd, 0, sizeof(cfc_vpd));
	ipr_inquiry(&ioa->ioa, IPR_STD_INQUIRY, &ioa_vpd, sizeof(ioa_vpd));
	ipr_inquiry(&ioa->ioa, 1, &cfc_vpd, sizeof(cfc_vpd));
	ipr_strncpy_0(product_id,
		      ioa_vpd.std_inq_data.vpids.product_id,
		      IPR_PROD_ID_LEN);
	ipr_strncpy_0(serial_num,
		      cfc_vpd.serial_num,
		      IPR_SERIAL_NUM_LEN);

	body_details(&n_show_battery_info,IPR_LINE_FEED, NULL);
	body_details(&n_show_battery_info,2, ioa->ioa.gen_name);
	body_details(&n_show_battery_info,3, serial_num);
	body_details(&n_show_battery_info,4, product_id);
	body_details(&n_show_battery_info,5, ioa->pci_address);
	sprintf(buffer,"%d", ioa->host_num);
	body_details(&n_show_battery_info,6, buffer);

	switch (reclaim_data->rechargeable_battery_type) {
	case IPR_BATTERY_TYPE_NICD:
		sprintf(buffer,catgets(catd,n_show_battery_info.text_set,21,"Nickel Cadmium (NiCd)"));
		break;
	case IPR_BATTERY_TYPE_NIMH:
		sprintf(buffer,catgets(catd,n_show_battery_info.text_set,22,"Nickel Metal Hydride (NiMH)"));
		break;
	case IPR_BATTERY_TYPE_LIION:
		sprintf(buffer,catgets(catd,n_show_battery_info.text_set,23,"Lithium Ion (LiIon)"));
		break;
	default:
		sprintf(buffer,catgets(catd,n_show_battery_info.text_set,24,"Unknown"));
		break;
	}

	body_details(&n_show_battery_info,7, buffer);

	switch (reclaim_data->rechargeable_battery_error_state) {
	case IPR_BATTERY_NO_ERROR_STATE:
		sprintf(buffer,catgets(catd,n_show_battery_info.text_set,31,"No battery warning"));
		break;
	case IPR_BATTERY_WARNING_STATE:
		sprintf(buffer,catgets(catd,n_show_battery_info.text_set,32,"Battery warning issued"));
		break;
	case IPR_BATTERY_ERROR_STATE:
		sprintf(buffer,catgets(catd,n_show_battery_info.text_set,33,"Battery error issued"));
		break;
	default:
		sprintf(buffer,catgets(catd,n_show_battery_info.text_set,24,"Unknown"));
		break;
	}

	body_details(&n_show_battery_info,8, buffer);

	sprintf(buffer,"%d",reclaim_data->raw_power_on_time);
	body_details(&n_show_battery_info,9, buffer);

	sprintf(buffer,"%d",reclaim_data->adjusted_power_on_time);
	body_details(&n_show_battery_info,10, buffer);

	sprintf(buffer,"%d",reclaim_data->estimated_time_to_battery_warning);
	body_details(&n_show_battery_info,11, buffer);

	sprintf(buffer,"%d",reclaim_data->estimated_time_to_battery_failure);
	body_details(&n_show_battery_info,12, buffer);

	s_out = screen_driver(&n_show_battery_info,0,NULL);

	ipr_free(n_show_battery_info.title);
	n_show_battery_info.title = NULL;
	ipr_free(n_show_battery_info.body);
	n_show_battery_info.body = NULL;
	rc = s_out->rc;
	ipr_free(s_out);
	return rc;
}

int confirm_force_battery_error(void)
{
	int rc,k;
	struct ipr_ioa *cur_ioa;
	char *buffer[2];
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	mvaddstr(0,0,"CONFIRM FORCE BATTERY ERROR FUNCTION CALLED");

	rc = RC_SUCCESS;

	/* Setup screen title */
	string_buf = catgets(catd,n_confirm_force_battery_error.text_set,1,
			     "Force Battery Packs Into Error State");
	n_confirm_force_battery_error.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_force_battery_error.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_confirm_force_battery_error,
				      &header_lines, 2, 6, "c=", k);

	for (cur_ioa = ipr_ioa_head; cur_ioa; cur_ioa = cur_ioa->next) {
		if ((!cur_ioa->reclaim_data) || (!cur_ioa->ioa.is_reclaim_cand))
			continue;
		
		for (k=0; k<2; k++)
			buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"2",cur_ioa, k);
	}

	do {
		n_confirm_force_battery_error.body = buffer[toggle&1];
		s_out = screen_driver(&n_confirm_force_battery_error,header_lines,NULL);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_confirm_force_battery_error.body = NULL;

	ipr_free(n_confirm_force_battery_error.title);
	n_confirm_force_battery_error.title = NULL;

	return rc;
}

int battery_fork(i_container *i_con)
{
	int rc = 0;
	int force_error = 0;
	struct ipr_ioa *cur_ioa;
	char *input;

	i_container *temp_i_con;
	mvaddstr(0,0,"CONFIRM FORCE BATTERY ERROR FUNCTION CALLED");

	for (temp_i_con = i_con_head; temp_i_con != NULL; temp_i_con = temp_i_con->next_item) {
		cur_ioa = (struct ipr_ioa *)temp_i_con->data;
		if (cur_ioa == NULL)
			continue;

		input = temp_i_con->field_data;

		if (strcmp(input, "2") == 0) {
			force_error++;
			cur_ioa->ioa.is_reclaim_cand = 1;
		} else if (strcmp(input, "5") == 0)	{
			rc = show_battery_info(cur_ioa);
			return rc;
		} else
			cur_ioa->ioa.is_reclaim_cand = 0;
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
	struct ipr_ioa *cur_ioa;
	struct ipr_reclaim_query_data reclaim_buffer;
	mvaddstr(0,0,"FORCE BATTERY ERROR FUNCTION CALLEd");

	reclaim_rc = rc = RC_SUCCESS;

	for (cur_ioa = ipr_ioa_head; cur_ioa; cur_ioa = cur_ioa->next) {
		if ((!cur_ioa->reclaim_data) ||
		    (!cur_ioa->ioa.is_reclaim_cand))
			continue;

		rc = ipr_reclaim_cache_store(cur_ioa,
					     IPR_RECLAIM_FORCE_BATTERY_ERROR,
					     &reclaim_buffer);

		if (rc != 0)
			/* "Attempting to force the selected battery packs into an
			 error state failed." */
			reclaim_rc = 43;

		if (reclaim_buffer.action_status != IPR_ACTION_SUCCESSFUL) {
			
			/* "Attempting to force the selected battery packs into an
			 error state failed." */
			syslog(LOG_ERR,"Battery Force Error to %s failed. %m\n",
			       cur_ioa->ioa.gen_name);
			reclaim_rc = 43; 
		}
	}

	if (reclaim_rc != RC_SUCCESS)
		return reclaim_rc;

	return rc;
}

int bus_config(i_container *i_con)
{
	int rc, k;
	int found = 0;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	u8 ioctl_buffer[255];
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	struct ipr_mode_page_28_header *modepage_28_hdr;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle=1;
	mvaddstr(0,0,"BUS CONFIG FUNCTION CALLED");
	rc = RC_SUCCESS;

	i_con = free_i_con(i_con);

	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_bus_config.text_set,1,"Create a Disk Array");
	n_bus_config.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_bus_config.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_bus_config, &header_lines, 2, 4, "1=", k);

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {

		/* mode sense page28 to focal point */
		mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
		rc = ipr_mode_sense(&cur_ioa->ioa, 0x28, mode_parm_hdr);

		if (rc != 0)
			continue;

		modepage_28_hdr = (struct ipr_mode_page_28_header *) (mode_parm_hdr + 1);

		if (modepage_28_hdr->num_dev_entries == 0)
			continue;

		i_con = add_i_con(i_con,"\0",(char *)cur_ioa);
		for (k=0; k<2; k++)
			buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"%1",cur_ioa, k);
		found++;
	}

	if (!found)	{
		setup_fail_screen(&n_bus_config_fail, 0, -1);

		s_out = screen_driver(&n_bus_config_fail,0,NULL);

		free(n_bus_config_fail.title);
		n_bus_config_fail.title = NULL;
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

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_bus_config.body = NULL;

	ipr_free(n_bus_config.title);
	n_bus_config.title = NULL;

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

int change_bus_attr(i_container *i_con)
{
	struct ipr_ioa *cur_ioa;
	int rc, j, i;
	struct ipr_scsi_buses page_28_cur;
	struct ipr_scsi_buses page_28_chg;
	struct scsi_dev_data *scsi_dev_data;
	char scsi_id_str[5][16];
	char max_xfer_rate_str[5][16];
	i_container *temp_i_con;
	int found = 0;
	char *input;
	struct bus_attr *bus_attr = NULL;
	char *string_buf;
	int header_lines = 0;
	int cur_len;
	char *bus_str;
	struct screen_output *s_out;
	u8 bus_num;
	int bus_attr_menu(struct ipr_ioa *cur_ioa,
			  struct bus_attr *bus_attr,
			  int row,
			  int header_lines);

	mvaddstr(0,0,"CHANGE BUS ATTR FUNCTION CALLED");

	rc = RC_SUCCESS;

	for (temp_i_con = i_con_head; temp_i_con != NULL;
	     temp_i_con = temp_i_con->next_item) {

		cur_ioa = (struct ipr_ioa *)temp_i_con->data;
		if (cur_ioa == NULL)
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
	rc = ipr_get_bus_attr(cur_ioa, &page_28_cur);

	if (rc != 0)
		/* "Change SCSI Bus configurations failed" */
		return 46 | EXIT_FLAG; 

	/* determine changable and default values */
	for (j = 0; j < page_28_cur.num_buses; j++) {

		page_28_chg.bus[j].qas_capability = 0;
		page_28_chg.bus[j].scsi_id = 0; /* FIXME!!! need to allow dart (by vend/prod & subsystem id) */
		page_28_chg.bus[j].bus_width = 1;
		page_28_chg.bus[j].max_xfer_rate = 1;
	}

	/* Title */
	string_buf = catgets(catd,n_change_bus_attr.text_set,1,
			     "Change SCSI Bus Configuration");
	n_change_bus_attr.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_change_bus_attr.title, string_buf);

	string_buf = catgets(catd,n_change_bus_attr.text_set,2,
			     "Current Bus configurations are shown. To change "
			     "setting hit 'c' for options menu. Hightlight "
			     "desired option then hit Enter");
	n_change_bus_attr.body =
		add_line_to_body(n_change_bus_attr.body, string_buf, "", &header_lines);

	string_buf = catgets(catd,n_change_bus_attr.text_set, 3,"Change Setting");
	cur_len = strlen(n_change_bus_attr.body);
	n_change_bus_attr.body = ipr_realloc(n_change_bus_attr.body,
					     cur_len + strlen(string_buf) + 8);
	cur_len += sprintf(n_change_bus_attr.body + cur_len,"  c=%s\n\n", string_buf);
	header_lines += 2;

	body_details(&n_change_bus_attr, IPR_RAW_DATA, cur_ioa->ioa.gen_name);
	header_lines++;

	string_buf = catgets(catd,n_change_bus_attr.text_set,4,"BUS");
	bus_str = malloc(strlen(string_buf) + 8);

	for (j = 0; j < page_28_cur.num_buses; j++) {
		sprintf(bus_str,"%s%d",string_buf, j);
		body_details(&n_change_bus_attr, IPR_RAW_DATA, bus_str);

		if (page_28_chg.bus[j].qas_capability) {
			body_details(&n_change_bus_attr,5, "%9");

			bus_attr = get_bus_attr_buf(bus_attr);
			bus_attr->type = qas_capability_type;
			bus_attr->bus = j;
			bus_attr->page_28 = &page_28_cur;
			bus_attr->ioa = cur_ioa;

			if (page_28_cur.bus[j].qas_capability ==
			    IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL)

				i_con = add_i_con(i_con,"Disabled",bus_attr);
			else
				i_con = add_i_con(i_con,"Enabled",bus_attr);
		}
		if (page_28_chg.bus[j].scsi_id) {
			body_details(&n_change_bus_attr,6, "%3");

			bus_attr = get_bus_attr_buf(bus_attr);
			bus_attr->type = scsi_id_type;
			bus_attr->bus = j;
			bus_attr->page_28 = &page_28_cur;
			bus_attr->ioa = cur_ioa;

			sprintf(scsi_id_str[j],"%d",page_28_cur.bus[j].scsi_id);
			i_con = add_i_con(i_con,scsi_id_str[j],bus_attr);
		}
		if (page_28_chg.bus[j].bus_width)	{
			/* check if 8 bit bus is allowable with current configuration before
			 enabling option */
			for (i=0; i < cur_ioa->num_devices; i++) {
				scsi_dev_data = cur_ioa->dev[i].scsi_dev_data;

				if ((scsi_dev_data) && (scsi_dev_data->id & 0xF8) &&
				    (j == scsi_dev_data->channel)) {

					page_28_chg.bus[j].bus_width = 0;
					break;
				}
			}
		}
		if (page_28_chg.bus[j].bus_width)	{
			body_details(&n_change_bus_attr,7, "%4");

			bus_attr = get_bus_attr_buf(bus_attr);
			bus_attr->type = bus_width_type;
			bus_attr->bus = j;
			bus_attr->page_28 = &page_28_cur;
			bus_attr->ioa = cur_ioa;

			if (page_28_cur.bus[j].bus_width == 16)
				i_con = add_i_con(i_con,"Yes",bus_attr);
			else
				i_con = add_i_con(i_con,"No",bus_attr);
		}
		if (page_28_chg.bus[j].max_xfer_rate) {
			body_details(&n_change_bus_attr,8, "%9");

			bus_attr = get_bus_attr_buf(bus_attr);
			bus_attr->type = max_xfer_rate_type;
			bus_attr->bus = j;
			bus_attr->page_28 = &page_28_cur;
			bus_attr->ioa = cur_ioa;

			sprintf(max_xfer_rate_str[j],"%d MB/s",
				(page_28_cur.bus[j].max_xfer_rate *
				 page_28_cur.bus[j].bus_width)/(10 * 8));
			i_con = add_i_con(i_con,max_xfer_rate_str[j],bus_attr);
		}
	}

	while (1) {
		s_out = screen_driver(&n_change_bus_attr,header_lines,i_con);
		rc = s_out->rc;

		for (temp_i_con = i_con_head; temp_i_con; temp_i_con = temp_i_con->next_item) {
			if (!temp_i_con->field_data[0]) {
				bus_attr = (struct bus_attr *)temp_i_con->data;
				rc = bus_attr_menu(cur_ioa, bus_attr, temp_i_con->y, header_lines);
				if (rc == CANCEL_FLAG)
					rc = RC_SUCCESS;

				bus_num = bus_attr->bus;

				if (bus_attr->type == qas_capability_type) {
					if (page_28_cur.bus[bus_num].qas_capability ==
					    IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL)

						sprintf(temp_i_con->field_data,"Disable");
					else
						sprintf(temp_i_con->field_data,"Enable");
				} else if (bus_attr->type == scsi_id_type) {
					sprintf(temp_i_con->field_data,"%d",page_28_cur.bus[bus_num].scsi_id);
				} else if (bus_attr->type == bus_width_type) {
					if (page_28_cur.bus[bus_num].bus_width == 16)
						sprintf(temp_i_con->field_data,"Yes");
					else
						sprintf(temp_i_con->field_data,"No");
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
	free(bus_str);
	ipr_free(n_change_bus_attr.body);
	n_change_bus_attr.body = NULL;
	ipr_free(n_change_bus_attr.title);
	n_change_bus_attr.title = NULL;
	free_i_con(i_con);
	ipr_free(s_out);
	return rc;
}

int confirm_change_bus_attr(i_container *i_con)
{
	struct ipr_ioa *cur_ioa;
	int rc, j, i;
	struct ipr_scsi_buses *page_28_cur;
	struct ipr_scsi_buses page_28_chg;
	struct scsi_dev_data *scsi_dev_data;
	char scsi_id_str[5][16];
	char max_xfer_rate_str[5][16];
	char *string_buf;
	int header_lines;
	int cur_len;
	char *bus_str;
	struct screen_output *s_out;

	mvaddstr(0,0,"CHANGE BUS ATTR FUNCTION CALLED");

	rc = RC_SUCCESS;

	page_28_cur = bus_attr_head->page_28;
	cur_ioa = bus_attr_head->ioa;

	/* determine changable and default values */
	for (j = 0; j < page_28_cur->num_buses; j++) {

		page_28_chg.bus[j].qas_capability = 0;
		page_28_chg.bus[j].scsi_id = 0; /* FIXME!!! need to allow dart (by vend/prod & subsystem id) */
		page_28_chg.bus[j].bus_width = 1;
		page_28_chg.bus[j].max_xfer_rate = 1;
	}

	/* Title */
	string_buf = catgets(catd,n_confirm_change_bus_attr.text_set,21,
			     "Confirm Change SCSI Bus Configuration");
	n_confirm_change_bus_attr.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_change_bus_attr.title, string_buf);

	string_buf = catgets(catd,n_confirm_change_bus_attr.text_set,22,
			     "Confirming this action will result in the following"
			     "configuration to become active.");
	n_confirm_change_bus_attr.body =
		add_line_to_body(n_confirm_change_bus_attr.body, string_buf, "", &header_lines);

	string_buf = catgets(catd,n_confirm_change_bus_attr.text_set, 23,
			     "Confirm Change SCSI Bus Configration");
	cur_len = strlen(n_confirm_change_bus_attr.body);
	n_confirm_change_bus_attr.body = ipr_realloc(n_confirm_change_bus_attr.body,
					     cur_len + strlen(string_buf) + 8);
	cur_len += sprintf(n_confirm_change_bus_attr.body + cur_len,"  c=%s\n\n", string_buf);
	header_lines += 2;

	body_details(&n_confirm_change_bus_attr, IPR_RAW_DATA, cur_ioa->ioa.gen_name);
	header_lines++;

	string_buf = catgets(catd,n_confirm_change_bus_attr.text_set,4,"BUS");
	bus_str = malloc(strlen(string_buf) + 8);

	for (j = 0; j < page_28_cur->num_buses; j++) {
		sprintf(bus_str,"%s%d",string_buf, j);
		body_details(&n_confirm_change_bus_attr, IPR_RAW_DATA, bus_str);

		if (page_28_chg.bus[j].qas_capability) {
			if (page_28_cur->bus[j].qas_capability ==
			    IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL)

				body_details(&n_confirm_change_bus_attr,5, "Disable");
			else
				body_details(&n_confirm_change_bus_attr,5, "Enable");
		}
		if (page_28_chg.bus[j].scsi_id) {
			sprintf(scsi_id_str[j],"%d",page_28_cur->bus[j].scsi_id);
			body_details(&n_confirm_change_bus_attr,6, scsi_id_str[j]);
		}
		if (page_28_chg.bus[j].bus_width)	{
			/* check if 8 bit bus is allowable with current configuration before
			 enabling option */
			for (i=0; i < cur_ioa->num_devices; i++) {
				scsi_dev_data = cur_ioa->dev[i].scsi_dev_data;

				if ((scsi_dev_data) && (scsi_dev_data->id & 0xF8) &&
				    (j == scsi_dev_data->channel)) {

					page_28_chg.bus[j].bus_width = 0;
					break;
				}
			}
		}
		if (page_28_chg.bus[j].bus_width)	{
			if (page_28_cur->bus[j].bus_width == 16)
				body_details(&n_confirm_change_bus_attr,7, "Yes");
			else
				body_details(&n_confirm_change_bus_attr,7, "No");
		}
		if (page_28_chg.bus[j].max_xfer_rate) {
			sprintf(max_xfer_rate_str[j],"%d MB/s",
				(page_28_cur->bus[j].max_xfer_rate *
				 page_28_cur->bus[j].bus_width)/(10 * 8));
			body_details(&n_confirm_change_bus_attr,8, max_xfer_rate_str[j]);
		}
	}

	s_out = screen_driver(&n_confirm_change_bus_attr,header_lines,NULL);

	rc = s_out->rc;
	free(s_out);
	free(bus_str);

	ipr_free(n_confirm_change_bus_attr.body);
	n_confirm_change_bus_attr.body = NULL;
	ipr_free(n_confirm_change_bus_attr.title);
	n_confirm_change_bus_attr.title = NULL;

	if (rc)
		return rc;

	rc = ipr_set_bus_attr(cur_ioa, page_28_cur, 1);
	return rc;
}


int bus_attr_menu(struct ipr_ioa *cur_ioa, struct bus_attr *bus_attr, int start_row, int header_lines)
{
	int i, scsi_id, found;
	int num_menu_items;
	int menu_index;
	ITEM **menu_item = NULL;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_scsi_buses *page_28_cur;
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
			for (i=0; i < cur_ioa->num_devices; i++) {

				scsi_dev_data = cur_ioa->dev[i].scsi_dev_data;
				if (scsi_dev_data == NULL)
					continue;

				if ((scsi_id == scsi_dev_data->id) &&
				    (bus_num == scsi_dev_data->channel)) {
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
		max_xfer_rate = get_max_bus_speed(cur_ioa, bus_num);

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
	if ((menu_start_row + menu_rows + 1 + 1) > max_y)
	{
		menu_start_row = max_y - (menu_rows + 1 + 1);
		if (menu_start_row < (8 + 1))
		{
			/* size is adjusted by 2 to allow for top
			 and bottom border, add +1 for buffer */
			menu_rows_disp = max_y - (8 + 2 + 1);
			menu_start_row = (8 + 1);
		}
	}

	if (menu_rows > menu_rows_disp)
	{
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
	if (use_box2)
	{
		box2_win = newwin(menu_rows + 2, 3, menu_start_row - 1, 54 + menu_cols + 1);
		box(box2_win,ACS_VLINE,ACS_HLINE);

		/* initialize scroll bar */
		for (i = 0;
		     i < menu_rows_disp/2;
		     i++)
		{
			mvwaddstr(box2_win, i + 1, 1, "#");
		}

		/* put down arrows in */
		for (i = menu_rows_disp;
		     i > (menu_rows_disp - menu_rows_disp/2);
		     i--)
		{
			mvwaddstr(box2_win, i, 1, "v");
		}
	}
	wnoutrefresh(box1_win);

	menu_index = 0;
	while (1)
	{
		if (use_box2)
			wnoutrefresh(box2_win);
		wnoutrefresh(field_win);
		doupdate();
		ch = getch();
		if (IS_ENTER_KEY(ch))
		{
			cur_item = current_item(menu);
			cur_item_index = item_index(cur_item);
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
						mvwaddstr(box2_win, i + 1, 1, "^");
					}
				}
				if (menu_index == (menu_index_max - 1))
				{
					/* remove down arrows */
					for (i = menu_rows_disp;
					     i > (menu_rows_disp - menu_rows_disp/2);
					     i--)
					{
						mvwaddstr(box2_win, i, 1, "#");
					}
				}
			}

			menu_driver(menu,REQ_DOWN_ITEM);
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
						mvwaddstr(box2_win, i, 1, "v");
					}
				}
				if (menu_index == 0)
				{
					/* remove up arrows */
					for (i = 0;
					     i < menu_rows_disp/2;
					     i++)
					{
						mvwaddstr(box2_win, i + 1, 1, "#");
					}
				}
			}
			menu_driver(menu,REQ_UP_ITEM);
		}
		else if (IS_EXIT_KEY(ch))
		{
			rc = EXIT_FLAG;
			break;
		}
		else if (IS_CANCEL_KEY(ch))
		{
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

#define MAX_CMD_LENGTH 1000
int log_menu(i_container *i_con)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc, loop;
	struct stat file_stat;
	struct screen_output *s_out;
	int offset = 0;
	char *string_buf;
	mvaddstr(0,0,"LOG MENU FUNCTION CALLED");

	sprintf(cmnd,"%s/boot.msg",log_root_dir);
	rc = stat(cmnd, &file_stat);

	if (rc)
		/* file does not exist - do not display option */
		offset--;

	string_buf = catgets(catd,n_log_menu.text_set,1,"Work with Disk Units");
	n_log_menu.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_log_menu.title, string_buf);

	for (loop = 0; loop < (n_log_menu.num_opts + offset); loop++) {
		n_log_menu.body = ipr_list_opts(n_log_menu.body,
						 n_log_menu.options[loop].key,
						 n_log_menu.text_set,
						 n_log_menu.options[loop].cat_num);
	}
	n_log_menu.body = ipr_end_list(n_log_menu.body);

	s_out = screen_driver(&n_log_menu,0,NULL);
	ipr_free(n_log_menu.body);
	n_log_menu.body = NULL;
	ipr_free(n_log_menu.title);
	n_log_menu.title = NULL;
	rc = s_out->rc;
	i_con = s_out->i_con;
	i_con = free_i_con(i_con);
	ipr_free(s_out);
	return rc;
}


int ibm_storage_log_tail(i_container *i_con)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc, len;
	mvaddstr(0,0,"IBM STORAGE LOG TAIL FUNCTION CALLED");

	def_prog_mode();
	rc = system("rm -rf /tmp/.ipr.err; mkdir /tmp/.ipr.err");

	if (rc != 0) {
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
		syslog(LOG_ERR, "Error encountered concatenating log files...\n");
		syslog(LOG_ERR, "Using failsafe editor...\n");
		closelog();
		openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
		sleep(3);
	} else {
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

	if ((rc != 0) && (rc != 127)) {
		/* "Editor returned %d. Try setting the default editor" */
		s_status.num = rc;
		return 65; 
	}
	else
		/* return with no status */
		return 1; 
}

int ibm_storage_log(i_container *i_con)
{
	char cmnd[MAX_CMD_LENGTH];
	int rc, i;
	struct dirent **log_files;
	struct dirent **dirent;
	int num_dir_entries;
	int len;

	mvaddstr(0,0,"IBM STORAGE LOG FUNCTION CALLED");

	def_prog_mode();

	num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
	rc = system("rm -rf /tmp/.ipr.err; mkdir /tmp/.ipr.err");
	if (num_dir_entries)
		dirent = log_files;

	if (rc != 0)
	{
		len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

		/* Probably have a read-only root file system */
		for (i = 0; i < num_dir_entries; i++)
		{
			len += sprintf(cmnd + len, "%s ", (*dirent)->d_name);
			dirent++;
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
		syslog(LOG_ERR, "Error encountered concatenating log files...\n");
		syslog(LOG_ERR, "Using failsafe editor...\n");
		closelog();
		openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);
		sleep(3);
	}
	else
	{
		for (i = 0; i < num_dir_entries; i++)
		{
			len = sprintf(cmnd,"cd %s; zcat -f %s | grep ipr-err |", log_root_dir, (*dirent)->d_name);
			len += sprintf(cmnd + len, "sed \"s/ `hostname -s` kernel: ipr-err//g\"");
			len += sprintf(cmnd + len, " | sed \"s/ localhost kernel: ipr-err//g\"");
			len += sprintf(cmnd + len, " | sed \"s/ kernel: ipr-err//g\"");
			len += sprintf(cmnd + len, "| sed \"s/ eldrad//g\""); /* xx */
			len += sprintf(cmnd + len, " | sed \"s/\\^M//g\" ");
			len += sprintf(cmnd + len, ">> /tmp/.ipr.err/errors");
			system(cmnd);
			dirent++;
		}

		sprintf(cmnd, "%s /tmp/.ipr.err/errors", editor);
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

	mvaddstr(0,0,"KERNEL LOG FUNCTION CALLED");

	def_prog_mode();

	num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
	if (num_dir_entries)
		dirent = log_files;

	rc = system("rm -rf /tmp/.ipr.err; mkdir /tmp/.ipr.err");

	if (rc != 0)
	{
		/* Probably have a read-only root file system */
		len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

		/* Probably have a read-only root file system */
		for (i = 0; i < num_dir_entries; i++)
		{
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
				"cd %s; zcat -f %s | sed \"s/\\^M//g\" >> /tmp/.ipr.err/errors",
				log_root_dir, (*dirent)->d_name);
			system(cmnd);
			dirent++;
		}

		sprintf(cmnd, "%s /tmp/.ipr.err/errors", editor);
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

	mvaddstr(0,0,"IPRCONFIG LOG FUNCTION CALLED");

	def_prog_mode();

	num_dir_entries = scandir(log_root_dir, &log_files, select_log_file, compare_log_file);
	rc = system("rm -rf /tmp/.ipr.err; mkdir /tmp/.ipr.err");
	if (num_dir_entries)
		dirent = log_files;

	if (rc != 0)
	{
		len = sprintf(cmnd, "cd %s; zcat -f ", log_root_dir);

		/* Probably have a read-only root file system */
		for (i = 0; i < num_dir_entries; i++)
		{
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
	}
	else
	{
		for (i = 0; i < num_dir_entries; i++)
		{
			len = sprintf(cmnd,"cd %s; zcat -f %s | grep iprconfig ", log_root_dir, (*dirent)->d_name);
			len += sprintf(cmnd + len, ">> /tmp/.ipr.err/errors");
			system(cmnd);
			dirent++;
		}

		sprintf(cmnd, "%s /tmp/.ipr.err/errors", editor);
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

int kernel_root(i_container *i_con)
{
	int rc;
	struct screen_output *s_out;
	char *string_buf;
	mvaddstr(0,0,"KERNEL ROOT FUNCTION CALLED");

	i_con = free_i_con(i_con);

	/* i_con to return field data */
	i_con = add_i_con(i_con,"",NULL); 

	string_buf = catgets(catd,n_kernel_root.text_set,1,
			     "Kernel Messages Log Root Directory");
	n_kernel_root.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_kernel_root.title, string_buf);

	string_buf = catgets(catd,n_kernel_root.text_set,2,
			     "Press 'c' to confirm your new kernel root directory choice.");
	n_kernel_root.body =
		add_line_to_body(n_kernel_root.body, string_buf, "", NULL);

	body_details(&n_kernel_root,IPR_LINE_FEED, NULL);
	body_details(&n_kernel_root,IPR_LINE_FEED, NULL);
	body_details(&n_kernel_root,IPR_LINE_FEED, NULL);
	body_details(&n_kernel_root,3, log_root_dir);
	body_details(&n_kernel_root,4, "%39");

	s_out = screen_driver(&n_kernel_root,0,i_con);

	ipr_free(n_kernel_root.title);
	n_kernel_root.title = NULL;
	ipr_free(n_kernel_root.body);
	n_kernel_root.body = NULL;
	rc = s_out->rc;
	ipr_free(s_out);

	return rc;
}

int confirm_kernel_root(i_container *i_con)
{
	int rc;
	DIR *dir;
	char *input;
	struct screen_output *s_out;
	char *string_buf;
	mvaddstr(0,0,"CONFIRM KERNEL ROOT FUNCTION CALLED");

	input = strip_trailing_whitespace(i_con->field_data);
	dir = opendir(input);

	if (dir == NULL)
		/* "Invalid directory" */
		return 59; 
	else {
		closedir(dir);
		if (strcmp(log_root_dir, input) == 0)
			/*"Root directory unchanged"*/
			return 61;
	}

	string_buf = catgets(catd,n_confirm_kernel_root.text_set,1,
			     "Confirm Root Directory Change");
	n_confirm_kernel_root.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_kernel_root.title, string_buf);

	body_details(&n_confirm_kernel_root,IPR_LINE_FEED, NULL);

	string_buf = catgets(catd,n_confirm_kernel_root.text_set,2,
			     "Press 'c' to confirm your new kernel root directory choice.");
	n_confirm_kernel_root.body =
		add_line_to_body(n_confirm_kernel_root.body, string_buf, "", NULL);

	string_buf = catgets(catd,n_confirm_kernel_root.text_set,3,
			     "To return to the log menu without changing the root directory, choose 'q'.");
	n_confirm_kernel_root.body =
		add_line_to_body(n_confirm_kernel_root.body, string_buf, "", NULL);

	body_details(&n_confirm_kernel_root,IPR_LINE_FEED, NULL);
	body_details(&n_confirm_kernel_root,IPR_LINE_FEED, NULL);
	body_details(&n_confirm_kernel_root,4, input);

	s_out = screen_driver(&n_confirm_kernel_root,0,i_con);

	ipr_free(n_confirm_kernel_root.title);
	n_confirm_kernel_root.title = NULL;
	ipr_free(n_confirm_kernel_root.body);
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
	char *string_buf;
	mvaddstr(0,0,"SET DEFAULT EDITOR FUNCTION CALLED");

	i_con = free_i_con(i_con);
	i_con = add_i_con(i_con,"",NULL);

	string_buf = catgets(catd,n_set_default_editor.text_set,1,
			     "Kernel Messages Editor");
	n_set_default_editor.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_set_default_editor.title, string_buf);

	string_buf = catgets(catd,n_set_default_editor.text_set,2,
			     "Press 'c' to confirm your new kernel root directory choice.");
	n_set_default_editor.body =
		add_line_to_body(n_set_default_editor.body, string_buf, "", NULL);

	body_details(&n_set_default_editor,IPR_LINE_FEED, NULL);
	body_details(&n_set_default_editor,IPR_LINE_FEED, NULL);
	body_details(&n_set_default_editor,IPR_LINE_FEED, NULL);
	body_details(&n_set_default_editor,3, editor);
	body_details(&n_set_default_editor,4, "%39");

	s_out = screen_driver(&n_set_default_editor,0,i_con);

	ipr_free(n_set_default_editor.title);
	n_set_default_editor.title = NULL;
	ipr_free(n_set_default_editor.body);
	n_set_default_editor.body = NULL;
	rc = s_out->rc;
	ipr_free(s_out);

	return rc;
}

int confirm_set_default_editor(i_container *i_con)
{
	int rc;
	char *input;
	struct screen_output *s_out;
	char *string_buf;
	mvaddstr(0,0,"CONFIRM SET DEFAULT EDITOR FUNCTION CALLED");

	input = strip_trailing_whitespace(i_con->field_data);
	if (strcmp(editor, input) == 0)
		/*"Editor unchanged"*/
		return 63; 

	string_buf = catgets(catd,n_confirm_set_default_editor.text_set,1,
			     "Confirm Root Directory Change");
	n_confirm_set_default_editor.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_confirm_set_default_editor.title, string_buf);

	body_details(&n_confirm_set_default_editor,IPR_LINE_FEED, NULL);

	string_buf = catgets(catd,n_confirm_set_default_editor.text_set,2,
			     "Press 'c' to confirm your editor choice");
	n_confirm_set_default_editor.body =
		add_line_to_body(n_confirm_set_default_editor.body, string_buf, "", NULL);

	string_buf = catgets(catd,n_confirm_set_default_editor.text_set,3,
			     "To return to the log menu without changing the editor, choose 'q'.");
	n_confirm_set_default_editor.body =
		add_line_to_body(n_confirm_set_default_editor.body, string_buf, "", NULL);

	body_details(&n_confirm_set_default_editor,IPR_LINE_FEED, NULL);
	body_details(&n_confirm_set_default_editor,IPR_LINE_FEED, NULL);
	body_details(&n_confirm_set_default_editor,4, input);

	s_out = screen_driver(&n_confirm_set_default_editor,0,i_con);

	ipr_free(n_confirm_set_default_editor.title);
	n_confirm_set_default_editor.title = NULL;
	ipr_free(n_confirm_set_default_editor.body);
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
	mvaddstr(0,0,"RESTORE LOG DEFAULTS FUNCTION CALLED");
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

	mvaddstr(0,0,"IBM BOOT LOG FUNCTION CALLED");

	sprintf(cmnd,"%s/boot.msg",log_root_dir);
	rc = stat(cmnd, &file_stat);
	if (rc)
		return 2; /* "Invalid option specified" */

	def_prog_mode();
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
		syslog(LOG_ERR, "Error encountered concatenating log files...\n");
		syslog(LOG_ERR, "Using failsafe editor...\n");
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

	if ((rc != 0) && (rc != 127))
	{
		s_status.num = rc;
		return 65; /* "Editor returned %d. Try setting the default editor" */
	}
	else
		return 1; /* return with no status */
}


int select_log_file(const struct dirent *dir_entry)
{
	if (dir_entry)
	{
		if (strstr(dir_entry->d_name, "messages") == dir_entry->d_name)
			return 1;
	}
	return 0;
}


int compare_log_file(const void *log_file1, const void *log_file2)
{
	char *first_start_num, *first_end_num;
	char *second_start_num, *second_end_num;
	struct dirent **first_dir, **second_dir;
	char name1[100], name2[100];
	int first, second;

	first_dir = (struct dirent **)log_file1;
	second_dir = (struct dirent **)log_file2;

	if (strcmp((*first_dir)->d_name, "messages") == 0)
		return 1;
	if (strcmp((*second_dir)->d_name, "messages") == 0)
		return -1;

	strcpy(name1, (*first_dir)->d_name);
	strcpy(name2, (*second_dir)->d_name);
	first_start_num = strchr(name1, '.');
	first_end_num = strrchr(name1, '.');
	second_start_num = strchr(name2, '.');
	second_end_num = strrchr(name2, '.');

	if (first_start_num == first_end_num)
	{
		/* Not compressed */
		first_end_num = name1 + strlen(name1);
		second_end_num = name2 + strlen(name2);
	}
	else
	{
		*first_end_num = '\0';
		*second_end_num = '\0';
	}
	first = strtoul(first_start_num, NULL, 10);
	second = strtoul(second_start_num, NULL, 10);

	if (first > second)
		return -1;
	else
		return 1;
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

int driver_config(i_container *i_con)
{
	int rc, k;
	char *buffer[2];
	struct ipr_ioa *cur_ioa;
	struct screen_output *s_out;
	char *string_buf;
	int header_lines;
	int toggle = 1;
	mvaddstr(0,0,"RAID START FUNCTION CALLED");

	/* empty the linked list that contains field pointers */
	i_con = free_i_con(i_con);

	rc = RC_SUCCESS;

	check_current_config(false);

	/* Setup screen title */
	string_buf = catgets(catd,n_driver_config.text_set,1,"Adapter Driver Configuration");
	n_driver_config.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_driver_config.title, string_buf);

	for (k=0; k<2; k++)
		buffer[k] = body_init(&n_driver_config, &header_lines, 2, 4, "1=", k);

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {
		i_con = add_i_con(i_con,"\0",cur_ioa);
		for (k=0; k<2; k++)
			buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"%1", cur_ioa, k);
	}

	do {
		n_driver_config.body = buffer[toggle&1];
		s_out = screen_driver(&n_driver_config,header_lines,i_con);
		toggle++;
	} while (s_out->rc == TOGGLE_SCREEN);

	rc = s_out->rc;
	free(s_out);

	for (k=0; k<2; k++) {
		ipr_free(buffer[k]);
		buffer[k] = NULL;
	}
	n_driver_config.body = NULL;

	ipr_free(n_driver_config.title);
	n_driver_config.title = NULL;

	return rc;
}

int change_driver_config(i_container *i_con)
{
	int rc;
	int found = 0;
	i_container *temp_i_con;
	struct ipr_ioa *ioa;
	struct screen_output *s_out;
	char buffer[128];
	char *string_buf;
	char *input;
	mvaddstr(0,0,"KERNEL ROOT FUNCTION CALLED");

	for (temp_i_con = i_con_head; temp_i_con != NULL;
	     temp_i_con = temp_i_con->next_item) {

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

	string_buf = catgets(catd,n_change_driver_config.text_set,1,
			     "Change Driver Configuration");
	n_change_driver_config.title = ipr_malloc(strlen(string_buf) + 4);
	sprintf(n_change_driver_config.title, string_buf);

	string_buf = catgets(catd,n_change_driver_config.text_set,2,
			     "Current Driver configurations are shown. To change "
			     "setting, type in new value then hit Enter");
	n_change_driver_config.body =
		add_line_to_body(n_change_driver_config.body, string_buf, "", NULL);

	body_details(&n_change_driver_config,IPR_LINE_FEED, NULL);
	body_details(&n_change_driver_config,IPR_LINE_FEED, NULL);
	body_details(&n_change_driver_config,IPR_LINE_FEED, NULL);
	sprintf(buffer,"%d", get_log_level(ioa));
	body_details(&n_change_driver_config,3, buffer);
	body_details(&n_change_driver_config,4, "%2");

	s_out = screen_driver(&n_change_driver_config,0,i_con);

	ipr_free(n_change_driver_config.title);
	n_change_driver_config.title = NULL;
	ipr_free(n_change_driver_config.body);
	n_change_driver_config.body = NULL;
	rc = s_out->rc;

	if (!rc) {
		if (strlen(i_con->field_data) != 0) {
			set_log_level(ioa,i_con->field_data);
			rc = 57;
		}
	}

	ipr_free(s_out);

	return rc;
}


char *print_device(struct ipr_dev *ipr_dev, char *body, char *option,
		   struct ipr_ioa *cur_ioa, int type)
{
	u16 len = 0;
	struct scsi_dev_data *scsi_dev_data = ipr_dev->scsi_dev_data;
	int i, rc;
	struct ipr_query_res_state res_state;
	u8 ioctl_buffer[255];
	char raid_str[48];
	int status;
	int format_req = 0;
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	struct ipr_block_desc *block_desc;
	struct sense_data_t sense_data;
	char *dev_name = ipr_dev->dev_name;
	char *gen_name = ipr_dev->gen_name;
	int tab_stop = 0;
	char vendor_id[IPR_VENDOR_ID_LEN];
	char product_id[IPR_PROD_ID_LEN];
	struct ipr_common_record *common_record;
	struct ipr_dev_record *device_record;
	struct ipr_array_record *array_record;
	struct ipr_cmd_status cmd_status;
	struct ipr_cmd_status_record *status_record;
	int percent_cmplt = 0;
	int format_in_progress = 0;
	int is_start_cand = 0;

	if (body)
		len = strlen(body);
	body = ipr_realloc(body, len + 100);

	if (type == 0)
		len += sprintf(body + len, " %s  %-6s %s.%d/",
			       option,
			       &dev_name[5],
			       cur_ioa->pci_address,
			       cur_ioa->host_num);
	else
		len += sprintf(body + len, " %s  %-6s %s.%d/",
			       option,
			       &gen_name[5],
			       cur_ioa->pci_address,
			       cur_ioa->host_num);

	if ((scsi_dev_data) &&
	    (scsi_dev_data->type == IPR_TYPE_ADAPTER)) {
		if (type == 0)
			len += sprintf(body + len,"            %-8s %-16s ",
				       scsi_dev_data->vendor_id,
				       scsi_dev_data->product_id);
		else
			len += sprintf(body + len,"            %-25s ","SCSI Adapter");

		if (cur_ioa->ioa_dead)
			len += sprintf(body + len, "Not Operational\n");
		else if (cur_ioa->nr_ioa_microcode)
			len += sprintf(body + len, "Not Ready\n");
		else
			len += sprintf(body + len, "Operational\n");
	} else if ((scsi_dev_data) &&
		   (scsi_dev_data->type == IPR_TYPE_EMPTY_SLOT)) {

		tab_stop  = sprintf(body + len,"%d:%d: ",
				    scsi_dev_data->channel,
				    scsi_dev_data->id);

		len += tab_stop;

		for (i=0;i<12-tab_stop;i++)
			body[len+i] = ' ';

		len += 12-tab_stop;
		len += sprintf(body + len, "%-8s %-16s "," ", " ");
		len += sprintf(body + len, "Empty\n");
	} else {
		if (scsi_dev_data) {
			tab_stop  = sprintf(body + len,"%d:%d:%d ",
					    scsi_dev_data->channel,
					    scsi_dev_data->id,
					    scsi_dev_data->lun);

			ipr_strncpy_0(vendor_id, scsi_dev_data->vendor_id, IPR_VENDOR_ID_LEN);
			ipr_strncpy_0(product_id, scsi_dev_data->product_id, IPR_PROD_ID_LEN);
		}
		else if (ipr_dev->qac_entry) {
			common_record = ipr_dev->qac_entry;
			if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) {
				device_record = (struct ipr_dev_record *)common_record;
				tab_stop  = sprintf(body + len,"%d:%d:%d ",
						    device_record->last_resource_addr.bus,
						    device_record->last_resource_addr.target,
						    device_record->last_resource_addr.lun);
				ipr_strncpy_0(vendor_id, device_record->vendor_id, IPR_VENDOR_ID_LEN);
				ipr_strncpy_0(product_id , device_record->product_id, IPR_PROD_ID_LEN);
			} else if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) {
				array_record = (struct ipr_array_record *)common_record;

				if (array_record->start_cand) {
					tab_stop  = sprintf(body + len,"            ");
					ipr_strncpy_0(vendor_id, array_record->vendor_id, IPR_VENDOR_ID_LEN);
					product_id[0] = '\0';
					is_start_cand = 1;
				} else {
					tab_stop  = sprintf(body + len,"%d:%d:%d ",
							    array_record->last_resource_addr.bus,
							    array_record->last_resource_addr.target,
							    array_record->last_resource_addr.lun);
					ipr_strncpy_0(vendor_id, array_record->vendor_id, IPR_VENDOR_ID_LEN);
					ipr_strncpy_0(product_id , array_record->product_id,
						      IPR_PROD_ID_LEN);
				}
			}
		}

		len += tab_stop;

		for (i=0;i<12-tab_stop;i++)
			body[len+i] = ' ';

		len += 12-tab_stop;

		if (type == 0) {
			len += sprintf(body + len, "%-8s %-16s ",
				       vendor_id,
				       product_id);
		}
		else {
			if (ipr_is_hot_spare(ipr_dev))
				len += sprintf(body + len, "%-25s ", "Hot Spare");
			else if (ipr_is_volume_set(ipr_dev)) {
				if (is_start_cand)
					len += sprintf(body + len, "%-25s ",
						       "SCSI RAID Adapter");
				else 
					len += sprintf(body + len, "RAID %-20s ",
						       ipr_dev->prot_level_str);

				rc = ipr_query_command_status(&cur_ioa->ioa, &cmd_status);

				if (!rc) {

					array_record = (struct ipr_array_record *)ipr_dev->qac_entry;
					status_record = cmd_status.record;

					for (i=0; i < cmd_status.num_records; i++, status_record++) {

						if ((status_record->command_code == IPR_START_ARRAY_PROTECTION) &&
						    (array_record->array_id == status_record->array_id)) {

							if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS)
								percent_cmplt = status_record->percent_complete;
						}
					}
				}
			}
			else if (ipr_is_array_member(ipr_dev)) {
				sprintf(raid_str,"  RAID %s Array Member",
					ipr_dev->prot_level_str);
				len += sprintf(body + len, "%-25s ", raid_str);

			}
			else if (ipr_is_af_dasd_device(ipr_dev)) {

				rc = ipr_query_command_status(ipr_dev, &cmd_status);

				if ((rc == 0) && (cmd_status.num_records != 0)) {

					status_record = cmd_status.record;
					if ((status_record->status != IPR_CMD_STATUS_SUCCESSFUL) &&
					    (status_record->status != IPR_CMD_STATUS_FAILED)) {

						percent_cmplt = status_record->percent_complete;
						format_in_progress++;
					}
				}

				len += sprintf(body + len, "%-25s ", "Advanced Function Disk");
			} else {
				/* Send Test Unit Ready to find percent complete in sense data. */
				rc = ipr_test_unit_ready(ipr_dev, &sense_data);

				if ((rc == CHECK_CONDITION) &&
				    ((sense_data.error_code & 0x7F) == 0x70) &&
				    ((sense_data.sense_key & 0x0F) == 0x02)) {

					percent_cmplt = ((int)sense_data.sense_key_spec[1]*100)/0x100;
					format_in_progress++;
				}

				len += sprintf(body + len, "%-25s ", "Physical Disk");
			}
		}

		if (ipr_is_af(ipr_dev))
		{
			memset(&res_state, 0, sizeof(res_state));

			/* Do a query resource state */
			rc = ipr_query_resource_state(ipr_dev, &res_state);

			if (rc != 0)
				res_state.not_oper = 1;
		}
		else /* JBOD */
		{
			memset(&res_state, 0, sizeof(res_state));

			format_req = 0;

			/* Issue mode sense/mode select to determine if device needs to be formatted */
			status = ipr_mode_sense(ipr_dev, 0x0a, &ioctl_buffer);

			if (status)
				res_state.not_oper = 1;
			else {
				mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

				if (mode_parm_hdr->block_desc_len > 0) {
					block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

					if ((block_desc->block_length[1] != 0x02) ||
					    (block_desc->block_length[2] != 0x00))

						format_req = 1;
					else {
						/* check device with test unit ready */
						rc = ipr_test_unit_ready(ipr_dev, &sense_data);

						if ((rc < 0) || (rc == CHECK_CONDITION))
							format_req = 1;
					}
				}
			}
		}

		if (format_in_progress)
			sprintf(body + len, "%d%% Formatted\n", percent_cmplt);
		else if (is_start_cand)
			sprintf(body + len, "Available");
		else if (!scsi_dev_data)
			sprintf(body + len, "Missing\n");
		else if (!scsi_dev_data->online)
			sprintf(body + len, "Offline\n");
		else if (res_state.not_oper)
			sprintf(body + len, "Not Operational\n");
		else if (res_state.not_ready ||
			 (res_state.read_write_prot &&
			  (ntohl(res_state.dasd.failing_dev_ioasc) ==
			   0x02040200u)))
			sprintf(body + len, "Not ready\n");
		else if ((res_state.read_write_prot) || (is_rw_protected(ipr_dev)))
			sprintf(body + len, "R/W Protected\n");
		else if (res_state.prot_dev_failed)
			sprintf(body + len, "DPY/Failed\n");
		else if (res_state.prot_suspended)
			sprintf(body + len, "DPY/Unprotected\n");
		else if (res_state.prot_resuming) {
			if ((type == 0) || (percent_cmplt == 0))
				sprintf(body + len, "DPY/Rebuilding\n");
			else
				sprintf(body + len, "%d%% Rebuilt\n", percent_cmplt);
		}
		else if (res_state.degraded_oper)
			sprintf(body + len, "Perf Degraded\n");
		else if (res_state.service_req)
			sprintf(body + len, "Redundant Hw Fail\n");
		else if (format_req)
			sprintf(body + len, "Format Required\n");
		else
		{
			if (ipr_is_array_member(ipr_dev))
				sprintf(body + len, "DPY/Active\n");
			else if (ipr_is_hot_spare(ipr_dev))
				sprintf(body + len, "HS/Active\n");
			else
				sprintf(body + len, "Operational\n");
		}
	}
	return body;
}

int is_format_allowed(struct ipr_dev *ipr_dev)
{
	return 1;
}

int is_rw_protected(struct ipr_dev *ipr_dev)
{
	return 0;  /* FIXME */
}

void evaluate_device(struct ipr_dev *ipr_dev, struct ipr_ioa *ioa, int change_size)
{
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_res_addr res_addr;
	int device_converted;
	int timeout = 60;

	/* send evaluate device down */
	scsi_dev_data = ipr_dev->scsi_dev_data;
	if (scsi_dev_data == NULL)
		return;

	ipr_evaluate_device(&ioa->ioa, scsi_dev_data->handle);

	res_addr.bus = scsi_dev_data->channel;
	res_addr.target = scsi_dev_data->id;
	res_addr.lun = scsi_dev_data->lun;

	device_converted = 0;

	while (!device_converted && timeout) {
		/* FIXME how to determine evaluate device completed? */

		device_converted = 1;
		sleep(1);
		timeout--;
	}
}

/* not needed after screen_driver can do menus */
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

/* not needed after screen_driver can do menus */
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

/*not needed after screen_driver can do menus */
void free_screen(struct panel_l *panel, struct window_l *win, FIELD **fields)
{
	struct panel_l *cur_panel;
	struct window_l *cur_win;
	int i;

	cur_panel = panel;
	while(cur_panel)
	{
		panel = panel->next;
		del_panel(cur_panel->panel);
		free(cur_panel);
		cur_panel = panel;
	}
	cur_win = win;
	while(cur_win)
	{
		win = win->next;
		delwin(cur_win->win);
		free(cur_win);
		cur_win = win;
	}

	if (fields)
	{
		for (i = 0; fields[i] != NULL; i++)
		{
			free_field(fields[i]);
		}
	}
}

int is_array_in_use(struct ipr_ioa *cur_ioa,
		    u8 array_id)
{
	int j, opens;
	struct ipr_dev_record *device_record;
	struct ipr_array_record *array_record;
	struct scsi_dev_data *scsi_dev_data;

	for (j = 0; j < cur_ioa->num_devices; j++)
	{
		device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
		array_record = (struct ipr_array_record *)cur_ioa->dev[j].qac_entry;

		if (device_record == NULL)
			continue;

		if (array_record->common.record_id == IPR_RECORD_ID_ARRAY_RECORD)
		{
			if (array_id == array_record->array_id)
			{
				scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
				if (scsi_dev_data == NULL)
					continue;

				opens = num_device_opens(scsi_dev_data->host,
							 scsi_dev_data->channel,
							 scsi_dev_data->id,
							 scsi_dev_data->lun);

				if (opens != 0)
					return 1;
			}
		}
	}
	return 0;
}

i_container *free_i_con(i_container *i_con)
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

i_container *add_i_con(i_container *i_con, char *f, void *d)
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
