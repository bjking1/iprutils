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

struct device_detail_struct{
  struct ipr_ioa *p_ioa;
  struct ipr_device *p_ipr_dev;
  int field_index;
};

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

/* not needed once menus incorporated into screen_driver */
struct window_l{
    WINDOW *p_win;
    struct window_l *p_next;
};

struct panel_l{
    PANEL *p_panel;
    struct panel_l *p_next;
};

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
#define CANCEL_KEY_LABEL "q=Cancel   "
#define IS_EXIT_KEY(c) ((c == KEY_F(3)) || (c == 'e') || (c == 'E'))
#define EXIT_KEY_LABEL "e=Exit   "
#define ENTER_KEY '\n'
#define IS_ENTER_KEY(c) ((c == KEY_ENTER) || (c == ENTER_KEY))
#define IS_REFRESH_KEY(c) ((c == 'r') || (c == 'R'))
#define REFRESH_KEY_LABEL "r=Refresh   "

#define IS_PGDN_KEY(c) ((c == KEY_NPAGE) || (c == 'f') || (c == 'F'))
#define PAGE_DOWN_KEY_LABEL "f=PageDn   "
#define IS_PGUP_KEY(c) ((c == KEY_PPAGE) || (c == 'b') || (c == 'B'))
#define PAGE_UP_KEY_LABEL "b=PageUp   "

#define IS_5250_CHAR(c) (c == 0xa)

i_container *free_i_con(i_container *x_con);
fn_out *free_fn_out(fn_out *out);
i_container *add_i_con(i_container *x_con,char *f,void *d,i_type t);
fn_out *init_output();
fn_out *add_fn_out(fn_out *output,int i,char *text);
i_container *copy_i_con(i_container *x_con);

struct special_status
{
  int index;
  int num;
  char *str;
} s_status;

struct screen_output *screen_driver(s_node *screen, fn_out *output, i_container *i_con);
char *strip_trailing_whitespace(char *p_str);


char *print_device(struct ipr_device *p_ipr_dev, char *body, char *option, struct ipr_ioa *p_cur_ioa);
int is_format_allowed(struct ipr_device *p_ipr_dev);
int is_rw_protected(struct ipr_device *p_ipr_dev);

u16 print_dev_pty(struct ipr_resource_entry *p_resource_entry, char *p_buf, char *dev_name);
u16 print_dev_pty2(struct ipr_resource_entry *p_resource_entry, char *p_buf, char *dev_name);

/*int is_valid_device(char *p_frame, char *p_slot, char *p_dsa, char *p_ua,
                    char *p_location, char *p_pci_bus, char *p_pci_device,
                    char *p_scsi_channel, char *p_scsi_id, char *p_scsi_lun);*/
u16 get_model(struct ipr_resource_flags *p_resource_flags);

int select_log_file(const struct dirent *p_dir_entry);
int compare_log_file(const void *pp_first, const void *pp_second);

void evaluate_device(struct ipr_device *p_ipr_device, struct ipr_ioa *p_ioa, int change_size);

int is_array_in_use(struct ipr_ioa *p_cur_ioa, u8 array_id);

int display_menu(ITEM **menu_item,
                 int menu_start_row,
                 int menu_index_max,
                 int **userptr);

char *print_parity_status(char *buffer,
                          u8 array_id,
                          struct ipr_query_res_state *p_res_state,
                          struct ipr_cmd_status *p_cmd_status);

void free_screen(struct panel_l *p_panel, struct window_l *p_win, FIELD **p_fields);
void flush_stdscr();
int flush_line();

nl_catd catd;

void *ipr_realloc(void *buffer, int size)
{
    void *new_buffer = realloc(buffer, size);

/*    syslog(LOG_ERR, "realloc from %lx to %lx of length %d\n", buffer, new_buffer, size);
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
    
int main(int argc, char *argv[])
{
  int next_editor, next_dir, next_platform, i;
  char parm_editor[200], parm_dir[200];
  
  p_head_ipr_ioa = p_last_ipr_ioa = NULL;
  
  /* makes program compatible with all terminals - originally did not display text correctly when user was running xterm */
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
      next_platform = 0;
      for (i = 1; i < argc; i++)
        {
	  if (strcmp(argv[i], "-e") == 0)
	    next_editor = 1;
	  else if (strcmp(argv[i], "-k") == 0)
	    next_dir = 1;
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

  initscr();
  cbreak(); /* take input chars one at a time, no wait for \n */
       
  keypad(stdscr,TRUE);
  noecho();

  s_status.index = 0;

  /* open the text catalog */
  catd=catopen("./text.cat",NL_CAT_LOCALE);

  main_menu(NULL);

  clear();
  refresh();
  endwin();
  exit(0);
  return 0;
}

struct screen_output *screen_driver_new(s_node *screen, fn_out *output, int header_lines, i_container *i_con)
{
    /*     Screen Name   <- title

     0  Header    bla      bla   <- pad_top = 0
     1  heading  heading  heading   <- header_lines=2
     2  output   out      out
     3  output   out      out  <- body (represented by text, % signs for fields, and # signs for output)
     4  output   out      out
     5  output   out      out
     6  output   out      out  <- pad_b = std_scr_max_y-pad_scr_t-pad_scr_b-1 = number dislayed lines - 1 = 6

     e=exit   q=quit  f=pagedn ...  <- footer
     */

    WINDOW *w_pad,*w_page_header; /* windows to hold text */
    FIELD **p_fields = NULL; /* field list for forms */
    FORM *p_form = NULL; /* p_form for input fields */

    fn_out *output_temp; /* help store function input */
    int rc = 0; /* the status of the screen */
    char *status; /* displays screen operations */
    char buffer[100]; /* needed for special status strings */
    bool invalid = false; /* invalid status display */

    bool e_on=false,q_on=false,r_on=false,f_on=false,enter_on=false,menus_on=false; /* control user input handling */
    bool ground_cursor=false;

    int stdscr_max_y,stdscr_max_x,w_pad_max_y=0,w_pad_max_x=0; /* limits of the windows */
    int pad_l=0,pad_r,pad_t=0,pad_b,pad_scr_t=2,pad_scr_b=2,pad_scr_l=0; /* position of the pad */
    int center,len=0,ch,i=0,j,k,row=0,col=0,bt_len=0; /* text positioning */
    int field_width,num_fields=0; /*form positioning */
    int h,w,t,l,o,b; /* form querying */

    int active_field=0; /* return to same active field after a quit */

    bool pages = false,is_bottom = false; /* control page up/down in multi-page screens */
    bool form_adjust = false; /* correct cursor position in multi-page screens */
    int str_index,max_str_index=0; /* text positioning in multi-page screens */
    bool refresh_stdscr = true;
    bool x_offscr,y_offscr; /* scrolling windows */

    struct screen_output *s_out;

    struct screen_opts *temp; /* reference to another screen */

    char *title,*body,*footer,*body_text,more_footer[100]; /* screen text */

    s_out = ipr_malloc(sizeof(struct screen_output)); /* passes an i_con and an rc value back to the function */

    /* create text strings */
    if (screen->title)
        title = screen->title;
    else
        title = catgets(catd,screen->text_set,1+output->cat_offset,"BOOM");
    if (screen->body)
        body = screen->body;
    else
        body = catgets(catd,screen->text_set,2+output->cat_offset,"BANG");
    if (screen->footer)
        footer = screen->footer;
    else
        footer = catgets(catd,screen->text_set,3+output->cat_offset,"KAPLOWEE   %e");

    bt_len = strlen(body);
    body_text = calloc(bt_len+1,sizeof(char));

    /* determine max size needed and find where to put form fields and device input in the text and add them
     * '%' marks a field.  The number after '%' represents the field's width
     * '#' marks device input.  The number after '#' represents the index of the input to display
     */
    while(body[i] != '\0')
    {
        if (body[i] == '\n')
        {
            row++;
            if (output)
                header_lines++;
            col = -1;

            body_text[len] = body[i];
            len++;
        }
        else if (body[i] == '%')
        {
            p_fields = ipr_realloc(p_fields,sizeof(FIELD **) * (num_fields + 1));
            i++;

            if (body[i] == 'm')
            {
                menus_on = true;
                field_width = 15;
                p_fields[num_fields] = new_field(1,field_width,row,col,0,0);
                field_opts_off(p_fields[num_fields], O_EDIT);
            }
            else
            {
                field_width = 0;
                sscanf(&(body[i]),"%d",&field_width);

                p_fields[num_fields] = new_field(1,field_width,row,col,0,0);

                if (field_width >= 30)
                    field_opts_off(p_fields[num_fields],O_AUTOSKIP);

                if (field_width > 9)
                    i++; /* skip second digit of field index */
            }
            set_field_userptr(p_fields[num_fields],NULL);
            num_fields++;
            col += field_width-1;

            bt_len += field_width;

            body_text = ipr_realloc(body_text,bt_len);
            for(k=0;k<field_width;k++)
                body_text[len+k] = ' ';
            len += field_width;
        }

        else if ((body[i] == '#') && output)
        {
            i++;
            str_index = 0;
            sscanf(&(body[i]),"%d",&str_index);

            max_str_index++;

            if (str_index > 9)
                i++; /* skip second digit of string index */

            output_temp = output;

            while (output_temp->next != NULL)
            {
                output_temp = output_temp->next;

                if (output_temp->index == str_index)
                    break;
            }

            /* check if loop completed because no match found */
            if (output_temp->index == str_index)
            {
                if (output_temp->text != NULL)
                {
                    bt_len += strlen(output_temp->text);
                    body_text = ipr_realloc(body_text,bt_len);
                    j = 0;
                    while (output_temp->text[j] != '\0')
                    {
                        if (output_temp->text[j] == '\n')
                        {
                            body_text[len] = output_temp->text[j];
                            len++;
                            row++;
                            col = -1;
                        }
                        else if(output_temp->text[j] == '\t')
                        {
                            int mt = 8;
                            int tab_width = (mt-col%mt);/*?mt-col%mt:mt);*/
                            bt_len+=tab_width-1;
                            body_text = ipr_realloc(body_text,bt_len);

                            for(k=0;k<tab_width;k++)
                                body_text[len+k] = ' ';

                            len+=tab_width;
                            col+=tab_width-1;
                        }
                        else if (output_temp->text[j] == '%')
                        {
                            p_fields = ipr_realloc(p_fields,sizeof(FIELD **) * (num_fields + 1));
                            j++; 

                            if (output_temp->text[j] == 'm')
                            {
                                menus_on = true;
                                field_width = 15;
                                p_fields[num_fields] = new_field(1,field_width,row,col,0,0);
                                field_opts_off(p_fields[num_fields], O_EDIT);
                            }
                            else
                            {
                                field_width = 0;
                                sscanf(&(output_temp->text[j]),"%d",&field_width);
                                p_fields[num_fields] = new_field(1,field_width,row,col,0,0);

                                if (field_width >= 30)
                                    field_opts_off(p_fields[num_fields],O_AUTOSKIP);

                                if (field_width > 9)
                                    i++; /* skip second digit of field width */
                            }
                            set_field_userptr(p_fields[num_fields],NULL);
                            num_fields++;
                            col += field_width-1;

                            bt_len += field_width;

                            body_text = ipr_realloc(body_text,bt_len);
                            for(k=0;k<field_width;k++)
                                body_text[len+k] = ' ';
                            len += field_width;
                        }
                        else
                        {
                            body_text[len] = output_temp->text[j];
                            len++;
                        }

                        col++;
                        j++;

                        if (col > w_pad_max_x)
                            w_pad_max_x = col;
                    }
                }
            }
        }
        else
        {
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

    if (num_fields)
    {
        p_fields = ipr_realloc(p_fields,sizeof(FIELD **) * (num_fields + 1));
        p_fields[num_fields] = NULL;
        p_form = new_form(p_fields);
    }

    /* make the form appear in the pad; not stdscr */
    set_form_win(p_form,w_pad);
    set_form_sub(p_form,w_pad);

    /* calculate the footer size and turn any special characters on*/
    i=0,j=0;
    while(footer[i] != '\0')
    {
        if (footer[i] == '%')
        {
            i++;
            switch (footer[i])
            {
                case 'n':
                    pad_scr_b += 3; /* add 3 lines to the footer */
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
            }
        }

        else
        {
            if (footer[i] == '\n')
                pad_scr_b++;
            more_footer[j++] = footer[i];
        }
        i++;
    }

    /* add null char to end of string */
    more_footer[j] = '\0';

    if (f_on && !enter_on)
        pad_scr_b++;

    /* clear all windows before writing new text to them */
    clear();
    wclear(w_pad);
    if (pages)
        wclear(w_page_header);

    while(1)
    {
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
        if ((stdscr_max_y - pad_scr_t - pad_scr_b + 1) > w_pad_max_y)
        {
            pad_b = w_pad_max_y;
            pad_t = 0;
            y_offscr = false; /*not scrollable*/
            pages = false;
            for (i = 0; i < num_fields; i++)
                field_opts_on(p_fields[i],O_ACTIVE);
        }
        else
        {
            pad_b = stdscr_max_y - pad_scr_t - pad_scr_b - 1;
            y_offscr = true; /*scrollable*/
            if (f_on && max_str_index<=1)
            {
                pages = true;
                w_page_header = subpad(w_pad,header_lines,w_pad_max_x,0,0);
            }
        }
        if (stdscr_max_x > w_pad_max_x)
        {
            pad_r = w_pad_max_x;
            pad_l = 0;
            x_offscr = false;
        }
        else
        {
            pad_r = stdscr_max_x;
            x_offscr = true;
        }

        move(stdscr_max_y-pad_scr_b,0);

        if (enter_on)
        {
            if (num_fields == 0 || menus_on) {
                addstr("\nPress Enter to Continue\n\n");
                ground_cursor = true;
            }
            else
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
        if (f_on && y_offscr)
        {
            addstr(PAGE_DOWN_KEY_LABEL);
            addstr(PAGE_UP_KEY_LABEL);
        }

        post_form(p_form);

        /* add the body of the text to the screen */
        if (pages)
        {
            wmove(w_page_header,0,0);
            waddstr(w_page_header,body_text);
        }
        wmove(w_pad,0,0);
        waddstr(w_pad,body_text);

        if (num_fields)
        {
            set_current_field(p_form,p_fields[active_field]);
            if (active_field == 0)
                form_driver(p_form,REQ_NEXT_FIELD);
            form_driver(p_form,REQ_PREV_FIELD);
        }

        refresh_stdscr = true;

        for(;;)
        {
            if (s_status.index) /* add global status if defined */
            {
                rc = s_status.index;
                s_status.index = 0;
            }
            if (invalid)
            {
                status = catgets(catd,STATUS_SET,INVALID_OPTION_STATUS,"Status Return Error (invalid)");
                invalid = false;
            }
            else if (rc != 0)
            {
                if ((status = strchr(catgets(catd,STATUS_SET,rc,"default"),'%')) == NULL)
                {
                    status = catgets(catd,STATUS_SET,rc,"Status Return Error (rc != 0, w/out %)");
                }
                else if (status[1] == 'd')
                {
                    sprintf(buffer,catgets(catd,STATUS_SET,rc,"Status Return Error (rc != 0 w/ %%d = %d"),s_status.num);
                    status = buffer;
                }
                else if (status[1] == 's')
                {
                    sprintf(buffer,catgets(catd,STATUS_SET,rc,"Status Return Error rc != 0 w/ %%s = %s"),s_status.str);
                    status = buffer;
                }
                rc = 0;
            }
            else
                status = NULL;

            move(stdscr_max_y-1,0);
            clrtoeol(); /* clear last line */
            mvaddstr(stdscr_max_y-1,0,status);
            status = NULL;

            if (f_on && y_offscr)
            {
                if (!is_bottom)
                    mvaddstr(stdscr_max_y-pad_scr_b,stdscr_max_x-8,"More...");
                else
                    mvaddstr(stdscr_max_y-pad_scr_b,stdscr_max_x-8,"       ");

                for (i=0;i<num_fields;i++)
                {
                    if (p_fields[i] != NULL)
                    {
                        field_info(p_fields[i],&h,&w,&t,&l,&o,&b); /* height, width, top, left, offscreen rows, buffer */

                        /* turn all offscreen fields off */
                        if (t>=header_lines+pad_t && t<=pad_b+pad_t)
                        {
                            field_opts_on(p_fields[i],O_ACTIVE);
                            if (form_adjust)
                            {
                                set_current_field(p_form,p_fields[i]);
                                form_adjust = false;
                            }
                        }
                        else
                            field_opts_off(p_fields[i],O_ACTIVE);
                    }
                }
            }  

            if (refresh_stdscr)
            {
                touchwin(stdscr);
                refresh_stdscr = FALSE;
            }
            else
                touchline(stdscr,stdscr_max_y-1,1);

            if (ground_cursor)
                move(0,0);

            refresh();

            if (pages)
            {
                touchwin(w_page_header);
                prefresh(w_page_header, 0, pad_l, pad_scr_t, pad_scr_l, header_lines+1 ,pad_r-1);
                touchwin(w_pad);
                prefresh(w_pad,pad_t+header_lines,pad_l,pad_scr_t+header_lines,pad_scr_l,pad_b+pad_scr_t,pad_r-1);
            }
            else
            {
                touchwin(w_pad);
                prefresh(w_pad,pad_t,pad_l,pad_scr_t,pad_scr_l,pad_b+pad_scr_t,pad_r-1);
            }

            if (ground_cursor) {
                move(stdscr_max_y-1,stdscr_max_x-1);
                refresh();
            }

            ch = wgetch(w_pad); /* pause for user input */

            if (ch == KEY_RESIZE)
                break;

            if (IS_ENTER_KEY(ch))
            {
                char *p_char;

                if (num_fields>1)
                    active_field = field_index(current_field(p_form));

                if (enter_on && num_fields == 0)
                {
                    rc = (CANCEL_FLAG | rc);
                    goto leave;
                }
                else if (enter_on && !menus_on) /* cancel if all fields are empty */
                {
                    form_driver(p_form,REQ_VALIDATION);
                    for (i=0;i<num_fields;i++)
                    {
                        if (strlen(strip_trailing_whitespace(field_buffer(p_fields[i],0))) != 0)
                            break; /* fields are not empty -> continue */
                        if (i == num_fields-1) /* all fields are empty */
                        {
                            rc = (CANCEL_FLAG | rc);
                            goto leave;
                        }
                    }
                }

                if (num_fields > 0)
                {
                    /* input field is always p_fields[0] if only 1 field */
                    p_char = field_buffer(p_fields[0],0);
                }

                invalid = true;

                for (i = 0; i < screen->num_opts; i++)
                {

                    temp = &(screen->options[i]);

                    if ((temp->key == "\n") || ((num_fields > 0)?(strcasecmp(p_char,temp->key) == 0):0))
                    {
                        invalid = false;

                        if (temp->key == "\n" && num_fields > 0) /* store field data to existing i_con (which should already
                                                                  contain pointers) */
                        {
                            i_container *temp_i_con = i_con;
                            form_driver(p_form,REQ_VALIDATION);

                            for (i=num_fields-1;i>=0;i--)
                            {
                                strncpy(temp_i_con->field_data,field_buffer(p_fields[i],0),MAX_FIELD_SIZE);
                                temp_i_con = temp_i_con->next_item;
                            }
                        }

                        if (temp->screen_function == NULL)
                            goto leave; /* continue with function */

                        do 
                            rc = temp->screen_function(i_con);
                        while (rc == REFRESH_SCREEN || rc & REFRESH_FLAG);

                        if ((rc & 0xF000) && !(screen->rc_flags & (rc & 0xF000)))
                            goto leave; /* if screen flags exist on rc and they don't match the screen's flags, return */

                        rc &= ~(EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG); /* strip screen flags from rc */

                        if (screen->rc_flags & REFRESH_FLAG)
                        {
                            s_status.index = rc;
                            rc = (REFRESH_FLAG | rc);
                            goto leave;
                        }
                        break;
                    }
                }

                if (!invalid) 
                {
                    /*clear fields*/
                    form_driver(p_form,REQ_LAST_FIELD);
                    for (i=0;i<num_fields;i++)
                    {
                        form_driver(p_form,REQ_CLR_FIELD);
                        form_driver(p_form,REQ_NEXT_FIELD);
                    }
                    break;
                }
            }

            else if (IS_EXIT_KEY(ch) && e_on)
            {
                rc = (EXIT_FLAG | rc);
                goto leave;
            }
            else if (IS_CANCEL_KEY(ch) && (q_on || screen == &n_main_menu))
            {
                rc = (CANCEL_FLAG | rc);
                goto leave;
            }
            else if ((IS_REFRESH_KEY(ch) && r_on) || (ch == KEY_HOME && r_on))
            {
                rc = REFRESH_SCREEN;
                goto leave;
            }
            else if (IS_PGUP_KEY(ch) && f_on && y_offscr)
            {
                invalid = false;
                if (pad_t>0)
                {
                    if (pages)
                        pad_t -= (pad_b-header_lines+1);
                    else
                        pad_t -= (pad_b+1);
                    rc = PGUP_STATUS;
                }
                else if (f_on && y_offscr)
                    rc = TOP_STATUS;
                else
                    invalid = true;

                if (!invalid)
                {
                    if (pad_t<0)
                        pad_t = 0;

                    is_bottom = false;
                    form_adjust = true;
                    refresh_stdscr = true;
                }

            }
            else if (IS_PGDN_KEY(ch) && f_on && y_offscr)
            {
                invalid = false;
                if ((stdscr_max_y + pad_t) < (w_pad_max_y + pad_scr_t + pad_scr_b))
                {
                    if (pages)
                        pad_t += (pad_b - header_lines + 1);
                    else
                        pad_t += (pad_b + 1);

                    rc = PGDN_STATUS;

                    if (!(stdscr_max_y + pad_t < w_pad_max_y + pad_scr_t + pad_scr_b))
                        is_bottom = true;
                }
                else if (is_bottom)
                    rc = BTM_STATUS;
                else
                    invalid = true;

                if (!invalid)
                {
                    form_adjust = true;
                    refresh_stdscr = true;
                }
            }

            else if (ch == KEY_RIGHT)
            {
                if (x_offscr && stdscr_max_x+pad_l<w_pad_max_x)
                    pad_l++;
                else if (!menus_on)
                    form_driver(p_form, REQ_NEXT_CHAR);
            }
            else if (ch == KEY_LEFT)
            {
                if (pad_l>0)
                    pad_l--;
                else if (!menus_on)
                    form_driver(p_form, REQ_PREV_CHAR);
            }
            else if (ch == KEY_UP)
            {
                if ((y_offscr || num_fields == 0) && !pages)
                {
                    if (pad_t>0)
                    {
                        pad_t--;
                        form_adjust = true;
                        is_bottom = false;
                    }
                }
                else
                    form_driver(p_form, REQ_PREV_FIELD);
            }
            else if (ch == KEY_DOWN)
            {
                if ((y_offscr || num_fields == 0) && !pages)
                {
                    if (y_offscr && stdscr_max_y+pad_t<w_pad_max_y+pad_scr_t+pad_scr_b)
                    {
                        pad_t++;
                        form_adjust = true;
                    }
                    if (!(stdscr_max_y+pad_t<w_pad_max_y+pad_scr_t+pad_scr_b))
                        is_bottom = true;
                }
                else
                    form_driver(p_form, REQ_NEXT_FIELD);
            }

            else if (num_fields == 0)
            {
                char tempstr[2];
                invalid = true;

                tempstr[0] = ch;
                tempstr[1] ='\0';

                for (i = 0; i < screen->num_opts; i++)
                {
                    temp = &(screen->options[i]);

                    if (temp->key)
                    {
                        if (strcasecmp(tempstr,temp->key) == 0)
                        {
                            invalid = false;

                            if (temp->screen_function == NULL)
                                goto leave;

                            do 
                                rc = temp->screen_function(i_con);
                            while (rc == REFRESH_SCREEN || rc & REFRESH_FLAG);

                            if ((rc & 0xF000) && !(screen->rc_flags & (rc & 0xF000)))
                                goto leave;

                            rc &= ~(EXIT_FLAG | CANCEL_FLAG | REFRESH_FLAG);

                            if (screen->rc_flags & REFRESH_FLAG)
                            {
                                rc = (REFRESH_FLAG | rc);
                                goto leave;
                            }		      
                            break;
                        }
                    }
                }
                break;
            }

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
            else if (isascii(ch))
                form_driver(p_form, ch);
        }
    }

    leave:
        s_out->i_con = i_con;
    s_out->rc = rc;
    ipr_free(body_text);
    unpost_form(p_form);
    free_form(p_form);

    for (i=0;i<num_fields;i++)
    {
        if (p_fields[i] != NULL)
            free_field(p_fields[i]);
    }
    delwin(w_pad);
    delwin(w_page_header);
    return s_out;

    mvaddstr(1,0,"inconceivable!");refresh();getch(); /* should never reach this point */
}
  
struct screen_output *screen_driver(s_node *screen, fn_out *output,
                                    i_container *i_con)
{
    return screen_driver_new(screen, output, 0, i_con);
}

void complete_screen_driver(char *title,char *body,char *complete)
{
  int stdscr_max_y,stdscr_max_x,center,len;
  
  getmaxyx(stdscr,stdscr_max_y,stdscr_max_x);
  clear();
  center = stdscr_max_x/2;
  len = strlen(title);

  /* add the title at the center of stdscr */
  mvaddstr(0,(center-len/2)>0?center-len/2:0,title);
  mvaddstr(2,0,body);
  
  len = strlen(complete);
  mvaddstr(stdscr_max_y/2,(center-len/2)>0?center-len/2:0,complete);
  refresh();
  return;
}

char *ipr_list_opts(char *body, char *key, int catalog_set, int catalog_num)
{
    char *string_buf;
    int start_len = 0;

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
    int start_len = 0;
    char *string_buf;

    start_len = strlen(body);

    string_buf = catgets(catd,0,2,"Selection");
    body = ipr_realloc(body, start_len + strlen(string_buf) + 16);
    sprintf(body + start_len, "\n\n%s: %%1", string_buf);
    return body;
}

int main_menu(i_container *i_con)
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
  struct ipr_cmd_status cmd_status;
  char init_message[60];
  int fd, fdev, k, j /*,ch2*/;
  struct ipr_cmd_status_record *p_status_record;
  int num_devs = 0;
  int num_stops = 0;
  int num_starts = 0;
  struct ipr_ioa *p_cur_ioa;
  struct devs_to_init_t *p_cur_dev_to_init;
  struct array_cmd_data *p_cur_raid_cmd;
  u8 ioctl_buffer[IPR_MODE_SENSE_LENGTH], *p_byte;
  struct sense_data_t sense_data;
  u8 cdb[IPR_CCB_CDB_LEN];
  struct ipr_resource_entry *p_resource_entry;
  struct ipr_mode_parm_hdr *p_mode_parm_hdr;
  struct ipr_block_desc *p_block_desc;
  int loop, len, retries;
  struct screen_output *s_out;
  char *string_buf;
  mvaddstr(0,0,"MAIN MENU FUNCTION CALLED");
  
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
      rc = ipr_query_command_status(fd, p_cur_ioa->ioa.p_resource_entry->resource_handle, &cmd_status);

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
		      p_tail_raid_cmd->p_next = (struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
		      p_tail_raid_cmd = p_tail_raid_cmd->p_next;
		    }
		  else
		    {
		      p_head_raid_cmd = p_tail_raid_cmd = (struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
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
		      p_tail_raid_cmd->p_next = (struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
		      p_tail_raid_cmd = p_tail_raid_cmd->p_next;
		    }
		  else
		    {
		      p_head_raid_cmd = p_tail_raid_cmd =
			(struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
		    }

		  p_tail_raid_cmd->p_next = NULL;

		  memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

		  p_tail_raid_cmd->array_id = p_status_record->array_id;
		  p_tail_raid_cmd->p_ipr_ioa = p_cur_ioa;
		  p_tail_raid_cmd->do_cmd = 1;
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

          if (ipr_is_af(p_resource_entry))
          {
              /* ignore if its a volume set */
              if (p_resource_entry->subtype == IPR_SUBTYPE_VOLUME_SET)
                  continue;

              /* Do a query command status */
              rc = ipr_query_command_status(fd, p_resource_entry->resource_handle, &cmd_status);

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
                          p_tail_dev_to_init->p_next = (struct devs_to_init_t *)ipr_malloc(sizeof(struct devs_to_init_t));
                          p_tail_dev_to_init = p_tail_dev_to_init->p_next;
                      }
                      else
                          p_head_dev_to_init = p_tail_dev_to_init = (struct devs_to_init_t *)ipr_malloc(sizeof(struct devs_to_init_t));

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
                      rc = ipr_mode_sense(fd, 0, p_resource_entry->resource_handle, ioctl_buffer);
                      p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

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
                      p_tail_dev_to_init->p_next = (struct devs_to_init_t *)ipr_malloc(sizeof(struct devs_to_init_t));
                      p_tail_dev_to_init = p_tail_dev_to_init->p_next;
                  }
                  else
                      p_head_dev_to_init = p_tail_dev_to_init = (struct devs_to_init_t *)ipr_malloc(sizeof(struct devs_to_init_t));

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
                      }
                  }
              }
              close(fdev);
          }
      }
      close(fd);
    }
  
  p_cur_dev_to_init = p_head_dev_to_init;
  while(p_cur_dev_to_init)
    {
      p_cur_dev_to_init = p_cur_dev_to_init->p_next;
      ipr_free(p_head_dev_to_init);
      p_head_dev_to_init = p_cur_dev_to_init;
    }

  p_cur_raid_cmd = p_head_raid_cmd;
  while(p_cur_raid_cmd)
    {
      p_cur_raid_cmd = p_cur_raid_cmd->p_next;
      ipr_free(p_head_raid_cmd);
      p_head_raid_cmd = p_cur_raid_cmd;
    }

  string_buf = catgets(catd,n_main_menu.text_set,1,"Title");
  n_main_menu.title = ipr_malloc(strlen(string_buf) + 4);
  sprintf(n_main_menu.title, string_buf);

  for (loop = 0; loop < n_main_menu.num_opts; loop++) {
      n_main_menu.body = ipr_list_opts(n_main_menu.body,
                                       n_main_menu.options[loop].key,
                                       n_main_menu.text_set,
                                       n_main_menu.options[loop].cat_num);
  }
  n_main_menu.body = ipr_end_list(n_main_menu.body);

  s_out = screen_driver(&n_main_menu,NULL,NULL);
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

char *disk_status_body_init(int *num_lines)
{
    char *string_buf;
    char *buffer = NULL;
    int cur_len;
    int header_lines = 0;
                  /*   .        .                  .            .                           .          */
                  /*0123456789012345678901234567890123456789012345678901234567890123456789901234567890 */
    char *header = "OPT Vendor/Product ID           Device Name  PCI/SCSI Location           Status";

    string_buf = catgets(catd,n_disk_status.text_set, 2,
                         "Type option, press Enter.");
    buffer = ipr_realloc(buffer, strlen(string_buf) + 4);
    cur_len = sprintf(buffer, "\n%s\n", string_buf);
    header_lines += 2;

    string_buf = catgets(catd,n_disk_status.text_set, 3,
                         "Display hardware resource information details");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
    cur_len += sprintf(buffer + cur_len, "  5=%s\n\n", string_buf);
    header_lines += 2;

    buffer = ipr_realloc(buffer, cur_len + strlen(header) + 8);
    cur_len += sprintf(buffer + cur_len, "%s\n", header);
    header_lines += 1;

    *num_lines = header_lines;
    return buffer;
}

int disk_status(i_container *i_con)
{
    int rc, j;
    int len = 0;
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    struct ipr_resource_entry *p_resource_entry;
    struct screen_output *s_out;
    char *string_buf;
    int header_lines;
    mvaddstr(0,0,"DISK STATUS FUNCTION CALLED");

    rc = RC_SUCCESS;
    i_con = free_i_con(i_con);

    check_current_config(false);
   
    /* Setup screen title */
    string_buf = catgets(catd,n_disk_status.text_set,1,"Display Disk Hardware Status");
    n_disk_status.title = ipr_malloc(strlen(string_buf) + 4);
    sprintf(n_disk_status.title, string_buf);

    n_disk_status.body = disk_status_body_init(&header_lines);

    for(p_cur_ioa = p_head_ipr_ioa;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next)
    {
        if (p_cur_ioa->ioa.p_resource_entry == NULL)
            continue;

        n_disk_status.body = print_device(&p_cur_ioa->ioa,n_disk_status.body,"%1", p_cur_ioa);
        i_con = add_i_con(i_con,"\0",(char *) p_cur_ioa->ioa.p_resource_entry,list);

        num_lines++;

        for (j = 0; j < p_cur_ioa->num_devices; j++)
        {
            p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
            if (p_resource_entry->is_ioa_resource)
            {
                continue;
            }

            n_disk_status.body = print_device(&p_cur_ioa->dev[j],n_disk_status.body, "%1", p_cur_ioa);
            i_con = add_i_con(i_con,"\0", (char *)p_cur_ioa->dev[j].p_resource_entry, list);  

            num_lines++;
        }
    }


    if (num_lines == 0)
    {
       string_buf = catgets(catd,n_disk_status.text_set,4,
                            "No devices found");
       len = strlen(n_disk_status.body);
       n_disk_status.body = ipr_realloc(n_disk_status.body, len +
                                    strlen(string_buf) + 4);
       sprintf(n_disk_status.body + len, "\n%s", string_buf);
    }

    s_out = screen_driver_new(&n_disk_status,NULL,header_lines,i_con);
    ipr_free(n_disk_status.body);
    n_disk_status.body = NULL;
    ipr_free(n_disk_status.title);
    n_disk_status.title = NULL;

    rc = s_out->rc;
    ipr_free(s_out);
    return rc;
}

#define IPR_LINE_FEED 0
void device_details_body(int code, char *field_data)
{
    char *string_buf;
    int cur_len = 0, start_len = 0;
    int str_len, field_data_len;
    char *body;
    int i;

    body = n_device_details.body;
    if (body != NULL)
        start_len = cur_len = strlen(body);

    if (code == IPR_LINE_FEED) {
        body = ipr_realloc(body, cur_len + 2);
        sprintf(body + cur_len, "\n");
    } else {
        string_buf = catgets(catd,n_device_details.text_set,code,"");
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
                    if (cur_len%2)
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
    n_device_details.body = body;
}

int device_details_get_device(i_container *i_con, struct device_detail_struct *p_device)
{
    i_container *temp_i_con;
    int j, dev_num, invalid=0;
    struct ipr_resource_entry *p_resource;
    char *p_char;
    int found = 0;
    struct ipr_ioa *p_cur_ioa;

    if (i_con == NULL)
    {
        return 1;
    }

    for (temp_i_con = i_con; temp_i_con != NULL && !invalid; temp_i_con = temp_i_con->next_item)
    {
        p_resource = (struct ipr_resource_entry *)(temp_i_con->data);
        if (p_resource != NULL)
        {
            p_char = temp_i_con->field_data;

            if (p_char == NULL)
                continue;

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
                            p_device->p_ipr_dev = &p_cur_ioa->ioa;
                            found = 1;
                            break;
                        }
                        for (j = 0; j < p_cur_ioa->num_devices; j++)
                        {
                            if (p_resource == p_cur_ioa->dev[j].p_resource_entry)
                            {
                                p_device->p_ipr_dev = &p_cur_ioa->dev[j];
                                found = 1;
                                break;
                            }
                        }
                        if (found)
                            break;
                    }

                    p_device->p_ioa = p_cur_ioa;
                    dev_num++;
                }
            }
            else if (strcmp(p_char, " ") != 0 && p_char != NULL && strcmp(p_char, "") != 0)
            {
                invalid = 1;
            }
        }
    }

    if (invalid)
    {
        return 15; /* The selection is not valid */
    }

    if (dev_num == 0)
    {
        return 17; /* "Invalid option.  No devices selected." */
    }


    else if (dev_num != 1) 
    {
        return 16; /* "Error.  More than one unit was selected." */
    }

    return 0;
}

int device_details(i_container *i_con)
{
    /*      IOA Hardware Resource Information Details


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
    /*      Disk Unit Hardware Resource Information Details


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

    struct device_detail_struct *p_detail_device;
    char *buffer;
    struct device_detail_struct detail_device;
    struct ipr_ioa *p_cur_ioa;

    int rc;
    char dev_name[50];

    struct ipr_resource_entry *p_resource;
    struct ipr_array2_record *ipr_array2_record;
    struct ipr_ioa *p_ioa;

    struct ipr_supported_arrays *p_supported_arrays;
    struct ipr_array_cap_entry *p_cur_array_cap_entry;
    int fd, i;
    u8 product_id[IPR_PROD_ID_LEN+1];
    u8 vendor_id[IPR_VENDOR_ID_LEN+1];
    u8 plant_code[IPR_VPD_PLANT_CODE_LEN+1];
    u8 part_num[IPR_VPD_PART_NUM_LEN+1];
    int cache_size;
    u8 dram_size[IPR_VPD_DRAM_SIZE_LEN+4];
    u8 cdb[IPR_CCB_CDB_LEN];
    struct ipr_read_cap read_cap;
    struct ipr_read_cap16 read_cap16;
    struct sense_data_t sense_data;
    long double device_capacity; 
    unsigned long long max_user_lba_int;
    char *p_dev_name;
    double lba_divisor;
    char scsi_channel[3], scsi_id[3], scsi_lun[4];
    struct ipr_ioa_vpd ioa_vpd;
    struct ipr_cfc_vpd cfc_vpd;
    struct ipr_dram_vpd dram_vpd;
    struct ipr_inquiry_page0 page0_inq;
    struct ipr_dasd_inquiry_page3 dasd_page3_inq;
    struct ipr_std_inq_data_long std_inq;
    char *p_char;

    buffer = ipr_malloc(100);
    struct screen_output *s_out;
    char *string_buf;
    mvaddstr(0,0,"DEVICE DETAILS FUNCTION CALLED");

    rc = device_details_get_device(i_con, &detail_device);
    if (rc)
        return rc;

    p_detail_device = &detail_device;
    p_resource = p_detail_device->p_ipr_dev->p_resource_entry;
    ipr_array2_record = (struct ipr_array2_record *)p_detail_device->p_ipr_dev->p_qac_entry;
    p_ioa = p_detail_device->p_ioa;

    rc = 0;

    sprintf(scsi_channel, "%d", p_resource->resource_address.bus);
    sprintf(scsi_id, "%d", p_resource->resource_address.target);
    sprintf(scsi_lun, "%d", p_resource->resource_address.lun);

    read_cap.max_user_lba = 0;
    read_cap.block_length = 0;

    if (p_resource->is_ioa_resource)
    {
        string_buf = catgets(catd,n_device_details.text_set,1,"Title");

        n_device_details.title = ipr_malloc(strlen(string_buf) + 4);
        sprintf(n_device_details.title, string_buf);

        memset(&ioa_vpd, 0, sizeof(ioa_vpd));
        memset(&cfc_vpd, 0, sizeof(cfc_vpd));
        memset(&dram_vpd, 0, sizeof(dram_vpd));

        fd = open(p_ioa->ioa.dev_name, O_RDWR | O_NONBLOCK);

        if (fd < 0)
        {
            syslog(LOG_ERR, "Cannot open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
        }
        else
        {
            ipr_inquiry(fd, IPR_IOA_RESOURCE_HANDLE, IPR_STD_INQUIRY,
                        &ioa_vpd, sizeof(ioa_vpd));
            ipr_inquiry(fd, IPR_IOA_RESOURCE_HANDLE, 1,
                        &cfc_vpd, sizeof(cfc_vpd));
            ipr_inquiry(fd, IPR_IOA_RESOURCE_HANDLE, 2,
                        &dram_vpd, sizeof(dram_vpd));
            close(fd);
        }

        ipr_strncpy_0(vendor_id, p_resource->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
        ipr_strncpy_0(product_id, p_resource->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);
        ipr_strncpy_0(plant_code, ioa_vpd.ascii_plant_code, IPR_VPD_PLANT_CODE_LEN);
        ipr_strncpy_0(part_num, ioa_vpd.ascii_part_num, IPR_VPD_PART_NUM_LEN);
        ipr_strncpy_0(buffer, cfc_vpd.cache_size, IPR_VPD_CACHE_SIZE_LEN);
        cache_size = strtoul(buffer, NULL, 16);
        sprintf(buffer,"%d MB", cache_size);
        memcpy(dram_size, dram_vpd.dram_size, IPR_VPD_DRAM_SIZE_LEN);
        sprintf(dram_size + IPR_VPD_DRAM_SIZE_LEN, " MB");

        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(2, vendor_id);
        device_details_body(3, product_id);
        device_details_body(4, p_ioa->driver_version);
        device_details_body(5, p_ioa->firmware_version);
        device_details_body(6, p_ioa->serial_num);
        device_details_body(7, part_num);
        device_details_body(8, plant_code);
        device_details_body(9, buffer);
        device_details_body(10, dram_size);
        device_details_body(11, p_ioa->ioa.dev_name);
        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(12, NULL);
        device_details_body(13, p_ioa->pci_address);
        sprintf(buffer,"%d", p_ioa->host_no);
        device_details_body(14, buffer);
    } else if ((ipr_array2_record) &&
               (ipr_array2_record->common.record_id ==
                IPR_RECORD_ID_ARRAY2_RECORD)) {

        string_buf = catgets(catd,n_device_details.text_set,101,"Title");

        n_device_details.title = ipr_malloc(strlen(string_buf) + 4);
        sprintf(n_device_details.title, string_buf);

        ipr_strncpy_0(vendor_id, p_resource->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);

        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(2, vendor_id);

        p_supported_arrays = p_ioa->p_supported_arrays;
        p_cur_array_cap_entry = (struct ipr_array_cap_entry *)p_supported_arrays->data;
        for (i=0; i<iprtoh16(p_supported_arrays->num_entries); i++)
        {
            if (p_cur_array_cap_entry->prot_level == ipr_array2_record->raid_level)
            {
                sprintf(buffer,"RAID %s", p_cur_array_cap_entry->prot_level_str);
                device_details_body(102, buffer);
                break;
            }

            p_cur_array_cap_entry = (struct ipr_array_cap_entry *)
                ((void *)p_cur_array_cap_entry + iprtoh16(p_supported_arrays->entry_length));
        }

        if (iprtoh16(ipr_array2_record->stripe_size) > 1024)
            sprintf(buffer,"%d M",iprtoh16(ipr_array2_record->stripe_size)/1024);
        else
            sprintf(buffer,"%d k",iprtoh16(ipr_array2_record->stripe_size));

        device_details_body(103, buffer);

        fd = open(p_ioa->ioa.dev_name, O_RDWR);

        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_ioa->ioa.dev_name);
        }
        else
        {
            /* Do a read capacity to determine the capacity */
            rc = ipr_read_capacity_16(fd, p_resource->resource_handle,
                                      &read_cap16);
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

            sprintf(buffer,"%.2Lf GB",device_capacity/lba_divisor);
            device_details_body(104, buffer);
        }
        device_details_body(11, p_detail_device->p_ipr_dev->dev_name);
    } else {
        string_buf = catgets(catd,n_device_details.text_set,201,"Title");

        n_device_details.title = ipr_malloc(strlen(string_buf) + 4);
        sprintf(n_device_details.title, string_buf);

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
            if (p_resource->subtype == IPR_SUBTYPE_GENERIC_SCSI)
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
                rc = ipr_read_capacity(fd, p_resource->resource_handle, &read_cap);
            }

            if (rc != 0)
                syslog(LOG_ERR, "Read capacity to %s failed. %m"IPR_EOL, dev_name);
        }

        ipr_strncpy_0(vendor_id, p_resource->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
        ipr_strncpy_0(product_id, p_resource->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);

        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(2, vendor_id);
        device_details_body(202, product_id);

        if (p_resource->subtype == IPR_SUBTYPE_GENERIC_SCSI) 
        {
            rc = ipr_inquiry(fd, 0,
                             0x03, &dasd_page3_inq, sizeof(dasd_page3_inq));
        }
        else
        {
            rc = ipr_inquiry(fd, p_resource->resource_handle,
                             0x03, &dasd_page3_inq, sizeof(dasd_page3_inq));
        }

        sprintf(buffer, "%X%X%X%X", dasd_page3_inq.release_level[0],
                dasd_page3_inq.release_level[1], dasd_page3_inq.release_level[2],
                dasd_page3_inq.release_level[3]);
        device_details_body(5, buffer);

        if (strcmp(vendor_id,"IBMAS400") == 0) {
            sprintf(buffer, "%X", p_resource->level);
            device_details_body(207, buffer);
        }

        device_details_body(6, p_resource->std_inq_data.serial_num);

        if (iprtoh32(read_cap.block_length) && iprtoh32(read_cap.max_user_lba))
        {
            lba_divisor = (1000*1000*1000)/iprtoh32(read_cap.block_length);
            device_capacity = iprtoh32(read_cap.max_user_lba) + 1;
            sprintf(buffer,"%.2Lf GB", device_capacity/lba_divisor);
            device_details_body(104, buffer);
        }

        if (strlen(p_detail_device->p_ipr_dev->dev_name) > 0)
        {
            device_details_body(11, p_detail_device->p_ipr_dev->dev_name);
        }

        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(12, NULL);
        device_details_body(13, p_ioa->pci_address);

        sprintf(buffer,"%d", p_ioa->host_no);
        device_details_body(203, buffer);

        sprintf(buffer,"%d",p_resource->resource_address.bus);
        device_details_body(204, buffer);

        sprintf(buffer,"%d",p_resource->resource_address.target);
        device_details_body(205, buffer);

        sprintf(buffer,"%d",p_resource->resource_address.lun);
        device_details_body(206, buffer);

        if (fd >= 0) {
            if (p_resource->subtype == IPR_SUBTYPE_GENERIC_SCSI) {
                rc =  ipr_inquiry(fd, 0, 0,
                                  &page0_inq, sizeof(page0_inq));
            } else {
                rc =  ipr_inquiry(fd, p_resource->resource_handle, 0,
                                  &page0_inq, sizeof(page0_inq));
            }

            for (i = 0; (i < page0_inq.page_length) && !rc; i++)
            {
                if (page0_inq.supported_page_codes[i] == 0xC7)  //FIXME!!! what is 0xC7?
                {
                    if (p_resource->subtype == IPR_SUBTYPE_GENERIC_SCSI) {
                        rc =  ipr_inquiry(fd, 0, IPR_STD_INQUIRY,
                                          &std_inq, sizeof(std_inq));
                    } else {
                        rc =  ipr_inquiry(fd, p_resource->resource_handle, IPR_STD_INQUIRY,
                                          &std_inq, sizeof(std_inq));
                    }

                    device_details_body(IPR_LINE_FEED, NULL);
                    device_details_body(208, NULL);

                    ipr_strncpy_0(buffer, std_inq.fru_number, IPR_STD_INQ_FRU_NUM_LEN);
                    device_details_body(209, buffer);

                    ipr_strncpy_0(buffer, std_inq.ec_level, IPR_STD_INQ_EC_LEVEL_LEN);
                    device_details_body(210, buffer);

                    ipr_strncpy_0(buffer, std_inq.part_number, IPR_STD_INQ_PART_NUM_LEN);
                    device_details_body(7, buffer);

                    p_char = (u8 *)&p_resource->std_inq_data;
                    sprintf(buffer, "%02X%02X%02X%02X%02X%02X%02X%02X",
                             p_char[0], p_char[1], p_char[2],
                             p_char[3], p_char[4], p_char[5],
                             p_char[6], p_char[7]);
                    device_details_body(211, buffer);

                    ipr_strncpy_0(buffer, std_inq.z1_term, IPR_STD_INQ_Z1_TERM_LEN);
                    device_details_body(212, buffer);

                    ipr_strncpy_0(buffer, std_inq.z2_term, IPR_STD_INQ_Z2_TERM_LEN);
                    device_details_body(213, buffer);

                    ipr_strncpy_0(buffer, std_inq.z3_term, IPR_STD_INQ_Z3_TERM_LEN);
                    device_details_body(214, buffer);

                    ipr_strncpy_0(buffer, std_inq.z4_term, IPR_STD_INQ_Z4_TERM_LEN);
                    device_details_body(215, buffer);

                    ipr_strncpy_0(buffer, std_inq.z5_term, IPR_STD_INQ_Z5_TERM_LEN);
                    device_details_body(216, buffer);

                    ipr_strncpy_0(buffer, std_inq.z6_term, IPR_STD_INQ_Z6_TERM_LEN);
                    device_details_body(217, buffer);
                    break;
                }
            }
            close (fd);

        }
    }

    s_out = screen_driver(&n_device_details,NULL,i_con);
    ipr_free(n_device_details.title);
    n_device_details.title = NULL;
    ipr_free(n_device_details.body);
    n_device_details.body = NULL;
    rc = s_out->rc;
    ipr_free(s_out);
    ipr_free(buffer);
    return rc;
}

int raid_screen(i_container *i_con)
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

#define IPR_INCLUDE 0
#define IPR_REMOVE  1
#define IPR_ADD_HOT_SPARE 0
#define IPR_RMV_HOT_SPARE 1

    int rc;
    struct array_cmd_data *p_cur_raid_cmd;
    struct screen_output *s_out;
    fn_out *output = init_output();
    char *string_buf;
    int loop;
    mvaddstr(0,0,"RAID SCREEN FUNCTION CALLED");

    p_cur_raid_cmd = p_head_raid_cmd;

    while(p_cur_raid_cmd)
    {
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        ipr_free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }

    string_buf = catgets(catd,n_raid_screen.text_set,1,"Title");
    n_raid_screen.title = ipr_malloc(strlen(string_buf) + 4);
    sprintf(n_raid_screen.title, string_buf);

    for (loop = 0; loop < n_raid_screen.num_opts; loop++) {
        n_raid_screen.body = ipr_list_opts(n_raid_screen.body,
                                           n_raid_screen.options[loop].key,
                                           n_raid_screen.text_set,
                                           n_raid_screen.options[loop].cat_num);
    }
    n_raid_screen.body = ipr_end_list(n_raid_screen.body);

    s_out = screen_driver(&n_raid_screen,output,NULL);
    ipr_free(n_raid_screen.body);
    n_raid_screen.body = NULL;
    ipr_free(n_raid_screen.title);
    n_raid_screen.title = NULL;
    rc = s_out->rc;
    ipr_free(s_out);
    output = free_fn_out(output);
    return rc;
}

char *raid_status_body_init(int *num_lines)
{
    char *buffer = NULL;
    int header_lines = 0;
                  /*            .          .                           .                               */
                  /*0123456789012345678901234567890123456789012345678901234567890123456789901234567890 */
    char *header = "Device Name  RAID Level PCI/SCSI Location           Status";

    buffer = ipr_realloc(buffer, strlen(header) + 8);
    sprintf(buffer, "\n%s\n", header);
    header_lines += 1;

    *num_lines = header_lines;
    return buffer;
}

char *print_parity_info(char *buffer, char *dev_name,
                        char *raid_level, struct ipr_ioa *p_ioa,
                        struct ipr_res_addr *res_addr)
{
    int i, tab_stop;
    int len = strlen(buffer);

    buffer = ipr_realloc(buffer, len + 100);

    len += sprintf(buffer + len, "%-12s %-10s %s.%d", dev_name, raid_level,
                   p_ioa->pci_address, p_ioa->host_no);
    
    tab_stop  = sprintf(buffer + len,"%d:%d:%d ",
                        res_addr->bus,
                        res_addr->target,
                        res_addr->lun);

    len += tab_stop;

    for (i=0;i<12-tab_stop;i++)
        buffer[len+i] = ' ';

    return buffer;
}

int raid_status(i_container *i_con)
{
    /*
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

    int fd, rc, array_num, i, j;
    struct ipr_cmd_status cmd_status;
    struct ipr_array_query_data *array_query_data;
    struct ipr_record_common *p_common_record;
    struct ipr_array2_record *p_array2_record;
    u8 array_id;
    char *buffer = NULL;
    struct ipr_query_res_state res_state;
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    struct ipr_resource_entry *p_resource_entry;
    int is_first;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    struct screen_output *s_out;
    char *string_buf;
    int header_lines;
    char *raid_level;
    struct ipr_supported_arrays *p_supported_arrays;
    struct ipr_array_cap_entry *p_cur_array_cap_entry;
    int found;
    mvaddstr(0,0,"RAID STATUS FUNCTION CALLED");

    rc = RC_SUCCESS;

    array_num = 1;
    check_current_config(false);

    /* Setup screen title */
    string_buf = catgets(catd,n_raid_status.text_set,1,"Display Disk Hardware Status");
    n_raid_status.title = ipr_malloc(strlen(string_buf) + 4);
    sprintf(n_raid_status.title, string_buf);

    buffer = raid_status_body_init(&header_lines);

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

        rc = ipr_query_command_status(fd, p_cur_ioa->ioa.p_resource_entry->resource_handle, &cmd_status);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Query Command status to %s failed. %m"IPR_EOL, p_cur_ioa->dev[j].dev_name);
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
            if (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD)
            {
                p_array2_record = (struct ipr_array2_record *)p_common_record;
                array_id =  p_array2_record->array_id;

                /* Ignore array candidates */
                if (!p_array2_record->established)
                    continue;

                p_supported_arrays = p_cur_ioa->p_supported_arrays;
                p_cur_array_cap_entry = (struct ipr_array_cap_entry *)
                    p_supported_arrays->data;

                for (i=0; i<iprtoh16(p_supported_arrays->num_entries); i++)
                {
                    if (p_array2_record->raid_level == p_cur_array_cap_entry->prot_level)
                    {
                        raid_level = p_cur_array_cap_entry->prot_level_str;
                        break;
                    }
                    p_cur_array_cap_entry = (struct ipr_array_cap_entry *)
                        ((void *)p_cur_array_cap_entry + iprtoh16(p_supported_arrays->entry_length));
                }

                memset(&res_state, 0, sizeof(res_state));

                if (p_array2_record->no_config_entry) {
                    buffer = print_parity_info(buffer, " ", raid_level, p_cur_ioa,
                                               &p_array2_record->last_resource_address);
                    res_state.not_oper = 1;

                } else {
                    found = 0;

                    for (j = 0;
                         j < p_cur_ioa->num_devices;
                         j++)
                    {
                        if (p_cur_ioa->dev[j].p_resource_entry->resource_handle ==
                            p_array2_record->resource_handle)
                        {
                            found = 1;
                            buffer = print_parity_info(buffer,
                                                       p_cur_ioa->dev[j].dev_name,
                                                       raid_level, p_cur_ioa,
                                                       &p_cur_ioa->dev[j].p_resource_entry->resource_address);

                            /* Do a query resource state */
                            rc = ipr_query_resource_state(fd, p_cur_ioa->dev[j].p_resource_entry->resource_handle, &res_state);

                            if (rc != 0)
                            {
                                syslog(LOG_ERR, "Query Resource State to a device (0x%08X) failed. %m"IPR_EOL,
                                       IPR_GET_PHYSICAL_LOCATOR(p_resource_entry->resource_address));
                                res_state.not_oper = 1;
                            }
                            break;
                        }
                    }
                    if (!found)
                        buffer = print_parity_info(buffer, " "," ", p_cur_ioa,
                                                   &p_cur_ioa->ioa.p_resource_entry->resource_address);
                }
                buffer = print_parity_status(buffer, array_id,
                                             &res_state, &cmd_status);
            }
            else
            {
                continue;
            }

            num_lines++;

            for (j = 0;
                 j < p_cur_ioa->num_devices;
                 j++)
            {
                p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

                if ((p_resource_entry->is_array_member) &&
                    (array_id == p_resource_entry->array_id))
                {
                    buffer = print_parity_info(buffer, " ", " ", p_cur_ioa,
                                               &p_resource_entry->resource_address);

                    /* Do a query resource state */
                    rc = ipr_query_resource_state(fd,
                                                  p_resource_entry->resource_handle, &res_state);

                    if (rc != 0)
                    {
                        syslog(LOG_ERR, "Query Resource State to a device (0x%08X) failed. %m"IPR_EOL,
                               IPR_GET_PHYSICAL_LOCATOR(p_resource_entry->resource_address));
                        res_state.not_oper = 1;
                    }

                    buffer = print_parity_status(buffer, array_id, &res_state, &cmd_status);
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
                    buffer = print_parity_info(buffer, p_cur_ioa->ioa.dev_name, " ", p_cur_ioa,
                                               &p_cur_ioa->ioa.p_resource_entry->resource_address);
                    buffer = ipr_realloc(buffer, strlen(buffer + 2));
                    strcat(buffer, "\n");
                    num_lines++;
                }

                buffer = print_parity_info(buffer, " ", " ", p_cur_ioa,
                                           &p_resource_entry->resource_address);

                /* Do a query resource state */
                rc = ipr_query_resource_state(fd, p_resource_entry->resource_handle, &res_state);

                if (rc != 0)
                {
                    syslog(LOG_ERR, "Query Resource State to a device (0x%08X) failed. %m"IPR_EOL,
                           IPR_GET_PHYSICAL_LOCATOR(p_resource_entry->resource_address));
                    res_state.not_oper = 1;
                }

                buffer = print_parity_status(buffer, IPR_INVALID_ARRAY_ID,
                                             &res_state, &cmd_status);

                num_lines++;
            }
        }
        close(fd);
    }

    if (num_lines == 0)
    {
        buffer = ipr_realloc(buffer, 100);
        sprintf(buffer,"\n  (Device parity protection has not been started)");
    }


    n_raid_status.body = buffer;

    s_out = screen_driver_new(&n_raid_status,NULL,header_lines, NULL);
    ipr_free(n_raid_status.body);
    n_raid_status.body = NULL;
    ipr_free(n_raid_status.title);
    n_raid_status.title = NULL;
    rc = s_out->rc;
    ipr_free(s_out);
    ipr_free(out_str);
    return rc;
}

char *print_parity_status(char *buffer,
                          u8 array_id,
                          struct ipr_query_res_state *p_res_state,
                          struct ipr_cmd_status *p_cmd_status)
{
    int len = strlen(buffer);
    struct ipr_cmd_status_record *p_status_record;
    int k;

    buffer = ipr_realloc(buffer, len + 32);

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

    sprintf(buffer + len, "\n");
    return buffer;
}

int raid_stop(i_container *i_con)
{
    /*
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
    int rc, i, array_num;
    struct ipr_record_common *p_common_record;
    struct ipr_array_record *p_array_record;
    u8 array_id;
    char *buffer = NULL;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len;
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"RAID STOP FUNCTION CALLED");

    /* empty the linked list that contains field pointers */
    i_con = free_i_con(i_con);

    rc = RC_SUCCESS;

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
                        p_tail_raid_cmd->p_next = (struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
                        p_tail_raid_cmd = p_tail_raid_cmd->p_next;
                    }
                    else
                    {
                        p_head_raid_cmd = p_tail_raid_cmd = (struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
                    }

                    p_tail_raid_cmd->p_next = NULL;

                    memset(p_tail_raid_cmd, 0, sizeof(struct array_cmd_data));

                    array_id =  p_array_record->array_id;

                    p_tail_raid_cmd->array_id = array_id;
                    p_tail_raid_cmd->array_num = array_num;
                    p_tail_raid_cmd->p_ipr_ioa = p_cur_ioa;
                    p_tail_raid_cmd->p_ipr_device = &p_cur_ioa->dev[i];

                    i_con = add_i_con(i_con,"\0",(char *)p_tail_raid_cmd,list);

                    buffer = print_device(&p_cur_ioa->dev[i],buffer, "%1", p_cur_ioa);

                    buf_size += 150;
                    out_str = realloc(out_str, buf_size);

                    out_str = strcat(out_str,buffer);

                    memset(buffer, 0, strlen(buffer));

                    len = 0;
                    num_lines++;
                    array_num++;
                }
            }
        }
    }

    if (array_num == 1)
        s_out = screen_driver(&n_raid_stop_fail,output,i_con); /*Stop Device Parity Protection Failed*/

    else   
    {
        out_str = strip_trailing_whitespace(out_str);
        output->next = add_fn_out(output->next,1,out_str);

        s_out = screen_driver(&n_raid_stop,output,i_con);
    }

    rc = s_out->rc;
    free(s_out);
    free(out_str);
    output = free_fn_out(output);
    return rc;
}

int confirm_raid_stop(i_container *i_con)
{
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
  struct ipr_device *p_ipr_device;
  struct ipr_resource_entry *p_resource_entry;
  struct ipr_array_record *p_array_record;
  
  struct array_cmd_data *p_cur_raid_cmd;
  struct ipr_device_record *p_device_record;
  struct ipr_resource_flags *p_resource_flags;
  struct ipr_ioa *p_cur_ioa;
  int rc, j;
  int found = 0; /* from raid_stop */
  u16 len=0;
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  char *p_char;    
  char *buffer = NULL;
  u32 num_lines = 0;
  i_container *temp_i_con;
  struct screen_output *s_out;
  fn_out *output = init_output();
  
  mvaddstr(0,0,"CONFIRM RAID STOP FUNCTION CALLED");
  
  found = 0;

  for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
      p_cur_raid_cmd = (struct array_cmd_data *)(temp_i_con->data);
      if (p_cur_raid_cmd != NULL)
	{
	  p_char = temp_i_con->field_data;
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

  if (!found)
    {
      output = free_fn_out(output);
      return INVALID_OPTION_STATUS;
    }
    
  rc = RC_SUCCESS;

  p_cur_raid_cmd = p_head_raid_cmd;
  
  while(p_cur_raid_cmd)
    {
      if (p_cur_raid_cmd->do_cmd)
	{
	  p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
	  p_ipr_device = p_cur_raid_cmd->p_ipr_device;
	    
	  buffer = print_device(p_ipr_device,buffer,"1",p_cur_ioa);
	  
	  buf_size += 150;
	  out_str = realloc(out_str, buf_size);

	  out_str = strcat(out_str,buffer);
	  len = 0;
	  memset(buffer, 0, strlen(buffer));
	  num_lines++;
	  
	  for (j = 0; j < p_cur_ioa->num_devices; j++)
	    {
	      p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;

	      if (p_resource_entry->is_array_member &&
		  (p_cur_raid_cmd->array_id == p_resource_entry->array_id))
		{
		  p_device_record = (struct ipr_device_record *)p_cur_ioa->dev[j].p_qac_entry;
		  p_resource_flags = &p_device_record->resource_flags_to_become;
		    
		  buffer = print_device(&p_cur_ioa->dev[j],buffer,"1",p_cur_ioa);

		  buf_size += 150;
		  out_str = realloc(out_str, buf_size);
		  
		  out_str = strcat(out_str,buffer);
		  len = 0;
		  
		  memset(buffer, 0, strlen(buffer));
		  num_lines++;
		}
	    }
	}
      p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }
  
  output->next = add_fn_out(output->next,1,out_str);
  
  s_out = screen_driver(&n_confirm_raid_stop,output,i_con);
  rc = s_out->rc;
  free(s_out);
  free(out_str);
  output = free_fn_out(output);
  return rc;
}

int do_confirm_raid_stop(i_container *i_con)
{
    struct ipr_device *p_ipr_device;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_ioa *p_cur_ioa;
    int fd, rc;
    int raid_stop_complete();
    int max_y,max_x;
    fn_out *output = init_output();
    mvaddstr(0,0,"DO CONFIRM RAID STOP FUNCTION CALLED");

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
                output = free_fn_out(output);
                return (20 | EXIT_FLAG); /* "Stop Parity Protection failed" */
            }

            if (p_ipr_device->p_qac_entry->record_id == IPR_RECORD_ID_ARRAY2_RECORD)
            {
                fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
                if (fd <= 1)
                {
                    syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    output = free_fn_out(output);
                    return (20 | EXIT_FLAG); /* "Stop Parity Protection failed" */
                }

                getmaxyx(stdscr,max_y,max_x);
                move(max_y-1,0);printw("Operation in progress - please wait");refresh();

                rc = ipr_start_stop_stop(fd, p_ipr_device->p_resource_entry->resource_handle);
                close(fd);

                if (rc != 0)
                {
                    syslog(LOG_ERR, "Stop Start/Stop Unit to %s failed. %m"IPR_EOL, p_ipr_device->dev_name);
                    output = free_fn_out(output);
                    return (20 | EXIT_FLAG); /* "Stop Parity Protection failed" */
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
                syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                output = free_fn_out(output);
                return (20 | EXIT_FLAG); /* "Stop Parity Protection failed" */
            }

            rc = ipr_stop_array_protection(fd, p_cur_ioa->p_qac_data);
            close(fd);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Stop Array Protection to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                output = free_fn_out(output);
                return (20 | EXIT_FLAG); /* "Stop Parity Protection failed" */
            }
        }
    }

    rc = raid_stop_complete();
    output = free_fn_out(output);
    return rc;
}

/* ??? odd screen - draw in function rather than screen_driver ??? */
int raid_stop_complete()
{
  /*
           Stop Device Parity Protection Status

    You selected to stop device parity protection









                   16% Complete
  */

  struct ipr_cmd_status cmd_status;
  struct ipr_cmd_status_record *p_record;
  int done_bad;
  int not_done = 0;
  int rc, j, fd;
  u32 percent_cmplt = 0;
  struct ipr_ioa *p_cur_ioa;
  u32 num_stops = 0;
  struct array_cmd_data *p_cur_raid_cmd;
  struct ipr_record_common *p_common_record;
  struct ipr_resource_entry *p_resource_entry;

  char *title, *body, *complete;
  
  title = catgets(catd,n_raid_stop_complete.text_set,1,"Stop Device Parity Protection Status");
  body = catgets(catd,n_raid_stop_complete.text_set,2,"You selected to stop device parity protection");
  complete = malloc(strlen(catgets(catd,n_raid_stop_complete.text_set,3,"%d%% Complete"))+20);
  
  sprintf(complete,catgets(catd,n_raid_stop_complete.text_set,3,"%d%% Complete"),percent_cmplt);
  
  complete_screen_driver(title,body,complete);
    
  while(1)
    {
      sprintf(complete,catgets(catd,n_raid_stop_complete.text_set,3,"%d%% Complete"),percent_cmplt,0);
      complete_screen_driver(title,body,complete);
      
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


              rc = ipr_query_command_status(fd, p_cur_ioa->ioa.p_resource_entry->resource_handle, &cmd_status);
              close(fd);

              if (rc)
              {
                  syslog(LOG_ERR, "Query Command Status to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                  p_cur_ioa->num_raid_cmds = 0;
                  done_bad = 1;
                  continue;
              }

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
                                  rc = (20 | EXIT_FLAG); /* "Stop Parity Protection failed" */
                              }
                              break;
                          }
                      }
                  }
              }
          }
      }

      if (!not_done)
        {
	  flush_stdscr();

	  if (done_bad)
	    return (20 | EXIT_FLAG); /* "Stop Parity Protection failed" */
	  
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
	  
     	  return (21 | EXIT_FLAG); /* "Stop Parity Protection completed successfully" */;
        }
      not_done = 0;
      sleep(2);
    }
}

int raid_start(i_container *i_con)
{
  int rc, i, array_num;
  struct array_cmd_data *p_cur_raid_cmd;
  struct ipr_record_common *p_common_record;
  struct ipr_array_record *p_array_record;
  u8 array_id;
  char *buffer = NULL;
  int num_lines = 0;
  struct ipr_ioa *p_cur_ioa;
  u32 len;
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"RAID START FUNCTION CALLED");
  /* empty the linked list that contains field pointers */
  i_con = free_i_con(i_con);
  
  rc = RC_SUCCESS;
  
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
		  
		  i_con = add_i_con(i_con,"\0",(char *)p_tail_raid_cmd,list);
		  
		  buffer = print_device(&p_cur_ioa->dev[i],buffer,"%1",p_cur_ioa);
		  
		  buf_size += 150;
		  
		  out_str = realloc(out_str, buf_size);
		  
		  out_str = strcat(out_str,buffer);
		  memset(buffer, 0, strlen(buffer));
		  
		  len = 0;
		  num_lines++;
		  array_num++;
                }
            }
        }
    }

  if (array_num == 1)
    {
      s_out = screen_driver(&n_raid_start_fail,output,i_con); /* Start Device Parity Protection Failed */
      rc = s_out->rc;
      free(s_out);
    }
    

  else
    {
      output->next = add_fn_out(output->next,1,out_str);

      s_out = screen_driver(&n_raid_start,output,i_con);
      rc = s_out->rc;
      free(s_out);
      
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
    }
  
  free(out_str);
  output = free_fn_out(output);
  return rc;
}

int raid_start_loop(i_container *i_con)
{
  int rc;
  int array_num;
  struct ipr_array_record *p_array_record;
  struct array_cmd_data *p_cur_raid_cmd;
  u8 array_id;
  int num_vsets;
  int found = 0;
  char *p_char;
  i_container *temp_i_con;
  fn_out *output = init_output();
  mvaddstr(0,0,"RAID START LOOP FUNCTION CALLED");
  
  found = 0;

  p_cur_raid_cmd = p_head_raid_cmd;
  
  for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
      p_cur_raid_cmd = (struct array_cmd_data *)(temp_i_con->data);
      if (p_cur_raid_cmd != NULL)
	{
	  p_char = temp_i_con->field_data;
	  p_array_record = (struct ipr_array_record *)p_cur_raid_cmd->p_ipr_device->p_qac_entry;
	  
	  if (strcmp(p_char, "1") == 0)
	    {
	      if (p_array_record->common.record_id == IPR_RECORD_ID_ARRAY2_RECORD)
		{
		  rc = RC_SUCCESS;
		  
		  rc = configure_raid_start(temp_i_con);
       		  
		  if (rc > 0)
		    {
		      output = free_fn_out(output);
		      return rc;
		    }
		  
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
      rc = confirm_raid_start(NULL);
      rc |= EXIT_FLAG;
    }
  else
    {
      rc =  INVALID_OPTION_STATUS; /* "Invalid options specified" */
    }

  output = free_fn_out(output);
  return rc;
}

int configure_raid_start(i_container *i_con)
{
    char *p_char;
    int found;
    struct ipr_device *p_ipr_device;
    struct array_cmd_data *p_cur_raid_cmd = (struct array_cmd_data *)(i_con->data);
    int rc, j;
    char *buffer = NULL;
    int num_devs = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_device_record *p_device_record;
    struct ipr_array_query_data *p_qac_data_tmp;
    /*  int configure_raid_parameters(struct array_cmd_data *p_cur_raid_cmd);*/
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    i_container *i_con2 = NULL;
    i_container *temp_i_con = NULL;
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"CONFIGURE RAID START FUNCTION CALLED");

    rc = RC_SUCCESS;

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
            buffer = print_device(&p_cur_ioa->dev[j],buffer,"%1", p_cur_ioa);

            i_con2 = add_i_con(i_con2,"\0",(char *)&p_cur_ioa->dev[j],list);

            buf_size += 150;
            out_str = realloc(out_str, buf_size);

            out_str = strcat(out_str,buffer);

            memset(buffer, 0, strlen(buffer));

            len = 0;
            num_devs++;
        }
    }

    if (num_devs)
    {
        p_qac_data_tmp = malloc(sizeof(struct ipr_array_query_data));
        memcpy(p_qac_data_tmp,
               p_cur_ioa->p_qac_data,
               sizeof(struct ipr_array_query_data));
    }

    output->next = add_fn_out(output->next,1,out_str);

    do
    {
        s_out = screen_driver(&n_configure_raid_start,output,i_con2);
        rc = s_out->rc;

        if (rc > 0)
            return rc;

        i_con2 = s_out->i_con;
        free(s_out);

        found = 0;

        for (temp_i_con = i_con2; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
        {
            p_ipr_device = (struct ipr_device *)temp_i_con->data;

            if (p_ipr_device != NULL)
            {
                p_char = temp_i_con->field_data;
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
            rc = configure_raid_parameters(i_con);

            if (rc &  EXIT_FLAG)
                goto leave;
            if (rc & CANCEL_FLAG)
            {
                /* clear out current selections */
                for (temp_i_con = i_con2; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
                {
                    p_ipr_device = (struct ipr_device *)temp_i_con->data;
                    if (p_ipr_device != NULL)
                    {
                        p_char = temp_i_con->field_data;
                        if (strcmp(p_char, "1") == 0)
                        {
                            p_device_record = (struct ipr_device_record *)p_ipr_device->p_qac_entry;
                            p_device_record->issue_cmd = 0;
                            p_ipr_device->is_start_cand = 0;
                        }
                    }
                }
                rc = REFRESH_FLAG;
                continue;
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
            s_status.index = INVALID_OPTION_STATUS;
            rc = REFRESH_FLAG;
        }
    } while (rc & REFRESH_FLAG);

    leave:
        i_con2 = free_i_con(i_con2);
    free(out_str);
    output = free_fn_out(output);

    if (rc > 0)
    {
        /* restore unmodified image of qac data */
        memcpy(p_cur_ioa->p_qac_data,
               p_qac_data_tmp,
               sizeof(struct ipr_array_query_data));
    }

    free(p_qac_data_tmp);

    return rc;
}

int configure_raid_parameters(i_container *i_con)
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
  FORM *p_form;
  ITEM **raid_item = NULL;
  struct array_cmd_data *p_cur_raid_cmd = (struct array_cmd_data *)(i_con->data);
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
  int max_x,max_y,start_y,start_x;
  
  getmaxyx(stdscr,max_y,max_x);
  getbegyx(stdscr,start_y,start_x);

  /*  move(1,0);printw("max x:%d start x:%d",max_x,start_x);refresh();getch();*/
  
  rc = RC_SUCCESS;
  
  /* Title */
  p_input_fields[0] = new_field(1, max_x - start_x,   /* new field size */ 
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
  
  p_form = new_form(p_input_fields);
  
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

  set_form_win(p_form,stdscr);
  set_form_sub(p_form,stdscr);
  post_form(p_form);
  
  mvaddstr(2, 1, "Default array configurations are shown.  To change");
  mvaddstr(3, 1, "setting hit \"c\" for options menu.  Highlight desired");
  mvaddstr(4, 1, "option then hit Enter");
  mvaddstr(6, 1, "c=Change Setting");
  mvaddstr(8, 0, "Protection Level . . . . . . . . . . . . :");
  mvaddstr(9, 0, "Stripe Size  . . . . . . . . . . . . . . :");
  mvaddstr(max_y - 4, 0, "Press Enter to Continue");
  mvaddstr(max_y - 2, 0, EXIT_KEY_LABEL CANCEL_KEY_LABEL);
  
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
	  
	  if (rc == EXIT_FLAG)
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
  unpost_form(p_form);
  free_form(p_form);
  free_screen(NULL, NULL, p_input_fields);
  
  flush_stdscr();
  return rc;
}

int confirm_raid_start(i_container *i_con)
{
    /*      Confirm Start Device Parity Protection

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

    struct ipr_resource_entry *p_resource_entry;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_resource_flags *p_resource_flags;
    struct ipr_ioa *p_cur_ioa;
    int rc, i, j, fd;
    char *buffer = NULL;
    u32 num_lines = 0;
    struct ipr_passthru_ioctl *ioa_ioctl;
    struct ipr_array_record *p_array_record;
    struct ipr_record_common *p_common_record_saved;
    struct ipr_record_common *p_common_record;
    struct ipr_device_record *p_device_record_saved;
    struct ipr_device_record *p_device_record;
    struct ipr_device *p_ipr_device;
    int found;

    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));

    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"CONFIRM RAID START FUNCTION CALLED");

    rc = RC_SUCCESS;

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

            buffer = print_device(p_ipr_device,buffer,"1",p_cur_ioa);

            buf_size += 100;
            out_str = realloc(out_str, buf_size);
            out_str = strcat(out_str,buffer);
            memset(buffer, 0, strlen(buffer));
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

                    buffer = print_device(&p_cur_ioa->dev[j],buffer,"1",p_cur_ioa);

                    buf_size += 100;
                    out_str = realloc(out_str, buf_size);
                    out_str = strcat(out_str,buffer);
                    memset(buffer, 0, strlen(buffer));
                    num_lines++;
                }
            }
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

    output->next = add_fn_out(output->next,1,strip_trailing_whitespace(out_str));

    s_out = screen_driver(&n_confirm_raid_start,output,NULL);
    rc = s_out->rc;
    free(s_out);
    free(out_str);        

    if (rc > 0)
        return rc;

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
                rc = 19;  /* "Start Parity Protection failed." */
                output = free_fn_out(output);
                syslog(LOG_ERR, "Devices may have changed state. Command cancelled,"
                       " please issue commands again if RAID still desired %s."
                       IPR_EOL, p_cur_ioa->ioa.dev_name);
                return rc;
            }
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

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
                ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_array_query_data));
                ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
                ioa_ioctl->buffer_len = sizeof(struct ipr_array_query_data);
                ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

                ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
                ioa_ioctl->cmd_pkt.write_not_read = 0;
                ioa_ioctl->cmd_pkt.cdb[0] = IPR_QUERY_ARRAY_CONFIG;

                ioa_ioctl->cmd_pkt.cdb[1] = 0x80; /* Prohibit Rebuild Candidate Refresh */

                ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
                ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

                rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

                if (rc != 0)
                {
                    if (errno != EINVAL)
                        syslog(LOG_ERR,"Query Array Config to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    close(fd);
                    free(ioa_ioctl);
                    continue;
                }
                else
                {
                    memcpy(p_cur_raid_cmd->p_qac_data, &ioa_ioctl->buffer[0], sizeof(struct ipr_array_query_data));
                    free(ioa_ioctl);
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

            rc = ipr_start_array_protection(fd, p_cur_ioa->p_qac_data,
                                            p_cur_raid_cmd->stripe_size,
                                            p_cur_raid_cmd->prot_level);
            close(fd);
            if (rc != 0)
            {
                syslog(LOG_ERR, "Start Array Protection to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            }
        }
    }

    rc = raid_start_complete(NULL);
    output = free_fn_out(output);

    return rc;
}


int raid_start_complete(i_container *i_con)
{
  /*
          Start Device Parity Protection Status

   You selected to start device parity protection






                  16% Complete
  */


  struct ipr_cmd_status cmd_status;
  struct ipr_cmd_status_record *p_record;
  int done_bad;
  int not_done = 0;
  int rc, j;
  u32 percent_cmplt = 0;
  struct ipr_ioa *p_cur_ioa;
  u32 num_starts = 0;
  struct array_cmd_data *p_cur_raid_cmd;

  char *title, *body, *complete;
  
  title = catgets(catd,n_raid_start_complete.text_set,1,"Start Device Parity Protection Status");
  body = catgets(catd,n_raid_start_complete.text_set,2,"You selected to start device parity protection");
  complete = malloc(strlen(catgets(catd,n_raid_start_complete.text_set,3,"%d%% Complete"))+20);
  
  while(1)
    {
      sprintf(complete,catgets(catd,n_raid_start_complete.text_set,3,"%d%% Complete"),percent_cmplt);
      complete_screen_driver(title,body,complete);
      
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


              rc = ipr_query_command_status(fd, p_cur_ioa->ioa.p_resource_entry->resource_handle, &cmd_status);
              close(fd);

              if (rc)
              {
                  syslog(LOG_ERR, "Query Command Status to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                  p_cur_ioa->num_raid_cmds = 0;
                  done_bad = 1;
                  continue;
              }

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

      if (!not_done)
        {
	  flush_stdscr();

	  if (done_bad)
	    return 19; /*"Start Parity Protection failed."*/
	  
	  check_current_config(false);
	  
	  return 18; /*"Start Parity Protection completed successfully"*/
        }
      not_done = 0;
      sleep(2);
    }
}

int raid_rebuild(i_container *i_con)
{
    /*                   Rebuild Disk Unit Data

     Type option, press Enter.
     1=Select

     Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
     OPT     ID        ID              Number    Bus    Dev    Bus   ID   Lun
     IBM      DFHSS4W          0024A0B9    05     03      0     3    0


     e=Exit   q=Cancel

     */
    int rc, i, array_num;
    struct ipr_record_common *p_common_record;
    struct ipr_device_record *p_device_record;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_resource_entry *p_resource_entry;
    u8 array_id;
    char *buffer = NULL;
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len = 0;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"RAID REBUILD FUNCTION CALLED");

    rc = RC_SUCCESS;

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

                    i_con = add_i_con(i_con,"\0",(char *)p_tail_raid_cmd,list);

                    buffer = print_device(&p_cur_ioa->ioa,buffer,"%1",p_cur_ioa);

                    buf_size += 150;
                    out_str = realloc(out_str, buf_size);

                    out_str = strcat(out_str,buffer);

                    memset(buffer, 0, strlen(buffer));

                    len = 0;
                    num_lines++;
                    array_num++;
                }
            }
        }
    }

    if (array_num == 1)
        s_out = screen_driver(&n_raid_rebuild_fail,output,i_con);


    else
        screen_driver(&n_raid_rebuild,output,i_con);

    rc = s_out->rc;

    free(s_out);
    free(out_str);
    output = free_fn_out(output);

    p_cur_raid_cmd = p_head_raid_cmd;
    while(p_cur_raid_cmd)
    {
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }

    return rc;
}

int confirm_raid_rebuild(i_container *i_con)
{

    /*                           Confirm Rebuild Disk Unit Data        

     Rebuilding the disk unit data may take several minutes for
     each unit selected.

     Press Enter to confirm having the data rebuilt. 
     Press q=Cancel to return and change your choice.

     Vendor   Product           Serial    PCI    PCI   SCSI  SCSI  SCSI
     Option   ID        ID              Number    Bus    Dev    Bus   ID   Lun
     1     IBM      DFHSS4W          0024A0B9    05     03      0     3    0

     q=Cancel

     */
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_device_record *p_device_record;
    struct ipr_ioa *p_cur_ioa;
    int fd, rc;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    char *buffer = NULL;
    u32 num_lines = 0;
    int found = 0;
    char *p_char;
    i_container *temp_i_con;
    fn_out *output = init_output();  
    mvaddstr(0,0,"CONFIRM RAID REBUILD FUNCTION CALLED");

    found = 0;

    for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
        p_cur_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
        if (p_cur_raid_cmd != NULL)
        {
            p_char = temp_i_con->field_data;

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

    if (!found)
    {
        output = free_fn_out(output);
        return INVALID_OPTION_STATUS;
    }

    p_cur_raid_cmd = p_head_raid_cmd;

    while(p_cur_raid_cmd)
    {
        if (p_cur_raid_cmd->do_cmd)
        {
            p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
            p_device_record = (struct ipr_device_record *)p_cur_raid_cmd->p_ipr_device->p_qac_entry;

            buffer = print_device(&p_cur_ioa->ioa,buffer,"1",p_cur_ioa);

            buf_size += 150;
            out_str = realloc(out_str, buf_size);

            out_str = strcat(out_str,buffer);
            memset(buffer, 0, strlen(buffer));
            num_lines++;
        }
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

    screen_driver(&n_confirm_raid_rebuild,output,i_con);

    for (p_cur_ioa = p_head_ipr_ioa;
         p_cur_ioa != NULL;
         p_cur_ioa = p_cur_ioa->p_next)
    {
        if (p_cur_ioa->num_raid_cmds > 0)
        {
            fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
            if (fd <= 1)
            {
                rc = 29 | EXIT_FLAG; /* "Rebuild failed" */
                syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                return rc;
            }

            rc = ipr_rebuild_device_data(fd, p_cur_ioa->p_qac_data);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Rebuild Disk Unit Data to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                rc = 29 | EXIT_FLAG; /* "Rebuild failed" */
                close(fd);
                return rc;
            }
            close(fd);
        }
    }
    rc = 28 | EXIT_FLAG; /* "Rebuild started, view Parity Status Window for rebuild progress" */
    return rc;
}

int raid_include(i_container *i_con)
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

  int i, j, array_num;
  struct ipr_record_common *p_common_record;
  struct ipr_array_record *p_array_record;
  struct array_cmd_data *p_cur_raid_cmd;
  u8 array_id;
  char *buffer = NULL;
  int num_lines = 0;
  struct ipr_ioa *p_cur_ioa;
  u32 len;
  int include_allowed;
  struct ipr_array2_record *p_array2_record;
  struct ipr_supported_arrays *p_supported_arrays;
  struct ipr_array_cap_entry *p_cur_array_cap_entry;
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  int rc = RC_SUCCESS;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"RAID INCLUDE FUNCTION CALLED");

  i_con = free_i_con(i_con);

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

		  i_con = add_i_con(i_con,"\0",(char *)p_tail_raid_cmd,list);
		  
		  buffer = print_device(&p_cur_ioa->dev[i],buffer,"%1",p_cur_ioa);

		  buf_size += 150;
		  out_str = realloc(out_str, buf_size);
		  
		  out_str = strcat(out_str,buffer);
		  
		  memset(buffer, 0, strlen(buffer));
		  
		  len = 0;
		  num_lines++;
		  array_num++;
                }
            }
        }
    }
  
  if (array_num == 1)
    s_out = screen_driver(&n_raid_include_fail,output,i_con);
  
  else
    {
      out_str = strip_trailing_whitespace(out_str);
      output->next = add_fn_out(output->next,1,out_str);

      s_out = screen_driver(&n_raid_include,output,i_con);
    }

  rc = s_out->rc;
  free(s_out);
  free(out_str);
  output = free_fn_out(output);
  
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
  
  return rc;
}

int configure_raid_include(i_container *i_con)
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

  int fd, i, j, k, array_num;
  struct array_cmd_data *p_cur_raid_cmd;
  struct ipr_passthru_ioctl *ioa_ioctl;
  struct ipr_array_query_data *p_array_query_data;
  struct ipr_record_common *p_common_record;
  struct ipr_device_record *p_device_record;
  struct ipr_resource_entry *p_resource_entry;
  struct ipr_array2_record *p_array2_record;
  char *buffer = NULL;
  int num_lines = 0;
  struct ipr_ioa *p_cur_ioa;
  u32 len = 0;
  int found = 0;
  char *p_char;
  struct ipr_device *p_ipr_device;
  struct ipr_supported_arrays *p_supported_arrays;
  struct ipr_array_cap_entry *p_cur_array_cap_entry;
  u8 min_mult_array_devices = 1;
  u8 raid_level;
  
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  int rc = RC_SUCCESS;
  i_container *temp_i_con;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIGURE RAID INCLUDE FUNCTION CALLED");

  found = 0;
  
  for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
  {
      p_cur_raid_cmd = (struct array_cmd_data *)i_con->data;
      if (p_cur_raid_cmd != NULL)
      {
          p_char = temp_i_con->field_data;
          if (strcmp(p_char, "1") == 0)
          {
              found = 1;
              break;
          }
      }
  }

  if (found != 1)
  {
      output = free_fn_out(output);
      return INVALID_OPTION_STATUS;
  }

  i_con = free_i_con(i_con);

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


      /************new*********/
      /* Get Query Array Config Data */
      p_array_query_data = (struct ipr_array_query_data *)
          malloc(sizeof(struct ipr_array_query_data));

      ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_array_query_data));
      ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
      ioa_ioctl->buffer_len = sizeof(struct ipr_array_query_data);
      ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

      ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
      ioa_ioctl->cmd_pkt.write_not_read = 0;
      ioa_ioctl->cmd_pkt.cdb[0] = IPR_QUERY_ARRAY_CONFIG;

      ioa_ioctl->cmd_pkt.cdb[1] = 0X01; /* Volume Set Include */
      ioa_ioctl->cmd_pkt.cdb[2] = p_cur_raid_cmd->array_id;  

      ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
      ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

      fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);    
      rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);
      close(fd);      

      if (rc != 0)
      {
          if (errno != EINVAL)
          {
              syslog(LOG_ERR, "Query Array Config to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
          }
          free(p_array_query_data);
          free(ioa_ioctl);
      }
      else
      {
          /* re-map the cross reference table with the new qac data */
          memcpy(p_array_query_data, &ioa_ioctl->buffer[0], sizeof(struct ipr_array_query_data));
          free(ioa_ioctl);

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
      free(ioa_ioctl);
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
	  
	  i_con = add_i_con(i_con,"\0",(char *)&p_cur_ioa->dev[j],list);
	  
	  p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
	  
	  buffer = print_device(&p_cur_ioa->dev[j],buffer,"1",p_cur_ioa);
	  
	  buf_size += 150;
	  out_str = realloc(out_str, buf_size);
	  
	  out_str = strcat(out_str,buffer);
	  memset(buffer, 0, strlen(buffer));
	  
	  len = 0;
	  num_lines++;
	  array_num++;
        }
    }

  if  (array_num <= min_mult_array_devices)
    {
      p_ipr_device = p_cur_raid_cmd->p_ipr_device;
      p_common_record = p_ipr_device->p_qac_entry;

      if (!(p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD))
	output->next = add_fn_out(output->next,1,catgets(catd,n_configure_raid_include_fail.text_set,4,"BANG"));
      else
	output->next = add_fn_out(output->next,1,catgets(catd,n_configure_raid_include_fail.text_set,5,"BANG"));

      s_out = screen_driver(&n_configure_raid_include_fail,output,i_con);
      rc = s_out->rc;
      goto leave;
    }

  out_str = strip_trailing_whitespace(out_str);
  output->next = add_fn_out(output->next,1,out_str);

  s_out = screen_driver(&n_configure_raid_include,output,i_con);
  i_con = s_out->i_con;

  if ((rc = s_out->rc) > 0)
    goto leave;
  
  /* first, count devices select to be sure min multiple
     is satisfied */
  found = 0;

  for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
      p_char = temp_i_con->field_data;
      if (strcmp(p_char, "1") == 0)
	found++;
    }
  if (found % min_mult_array_devices != 0)
    {
      output = free_fn_out(output);
      s_status.num = min_mult_array_devices;
      return 25 | REFRESH_FLAG; /* "Error:  number of devices selected must be a multiple of %d" */
    }
  else
    {
      found = 0;
      for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
	{
	  p_ipr_device = (struct ipr_device *)temp_i_con->data;
	  if (p_ipr_device != NULL)
	    {
	      p_char = temp_i_con->field_data;
	      
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
      rc = confirm_raid_include(i_con);
    }
  
 leave:
  free(out_str);
  free(s_out);
  output = free_fn_out(output);
  return rc;
}

int confirm_raid_include(i_container *i_con)
{
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

  struct ipr_resource_entry *p_resource_entry;
  struct array_cmd_data *p_cur_raid_cmd;
  struct ipr_device_record *p_device_record;
  int i, j, found;
  u16 len;
  char *buffer = NULL;
  u32 num_lines = 0;
  struct ipr_array_record *p_array_record;
  struct ipr_array_query_data   *p_qac_data;
  struct ipr_record_common *p_common_record;
  struct ipr_ioa *p_cur_ioa;
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  int rc = RC_SUCCESS;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIRM RAID INCLUDE FUNCTION CALLED");
 
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
                      buffer = print_device(&p_cur_ioa->dev[j],buffer,"1",p_cur_ioa);

		      buf_size += 150;
		      out_str = realloc(out_str, buf_size);
		      out_str = strcat(out_str,buffer);
		      
		      len = 0;
		      memset(buffer, 0, strlen(buffer));
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

  out_str = strip_trailing_whitespace(out_str);
  output->next = add_fn_out(output->next,1,out_str);
  free(out_str);

  s_out = screen_driver(&n_confirm_raid_include,output,i_con);
  rc = s_out->rc;
  free(s_out);

  output = free_fn_out(output);
  return rc;
}


int confirm_raid_include_device(i_container *i_con)
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
  char *buffer = NULL;
  struct array_cmd_data *p_cur_raid_cmd;
  struct ipr_resource_entry *p_resource_entry;
  struct ipr_device_record *p_device_record;
  struct devs_to_init_t *p_cur_dev_to_init;
  u32 i, j, found;
  u32 num_lines = 0;
  u8 num_devs=0;
  struct ipr_ioa *p_cur_ioa;
  struct ipr_array_record *p_array_record;
  struct ipr_array_query_data *p_qac_data;
  struct ipr_record_common *p_common_record;
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  int rc = RC_SUCCESS;
  struct screen_output *s_out;
  fn_out *output = init_output();
  int format_include_cand();
  int dev_include_complete(u8 num_devs);
  mvaddstr(0,0,"CONFIRM RAID INCLUDE FUNCTION CALLED");

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
                      buffer = print_device(&p_cur_ioa->dev[j],buffer,"1",p_cur_ioa);

		      buf_size += 100;
		      out_str = realloc(out_str, buf_size);
		      out_str = strcat(out_str,buffer);
		      memset(buffer, 0, strlen(buffer));
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
  
  out_str = strip_trailing_whitespace(out_str);
  output->next = add_fn_out(output->next,1,out_str);
  free(out_str);
  
  s_out = screen_driver(&n_confirm_raid_include_device,output,i_con);
  rc = s_out->rc;
  free(s_out);

  if (rc > 0)
    {
      output = free_fn_out(output);
      return rc;
    }

  /* now format each of the devices */
  num_devs = format_include_cand();
  
  rc = dev_include_complete(num_devs);
  
  /* free up memory allocated for format */
  p_cur_dev_to_init = p_head_dev_to_init;
  while(p_cur_dev_to_init)
    {
      p_cur_dev_to_init = p_cur_dev_to_init->p_next;
      free(p_head_dev_to_init);
      p_head_dev_to_init = p_cur_dev_to_init;
    }
  
  output = free_fn_out(output);
  return rc;
}

int format_include_cand()
{
    int fd, fdev, pid;
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

                        if (!ipr_is_hidden(p_tail_dev_to_init->p_ipr_device->p_resource_entry))
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
                            rc = ipr_format_unit(fd,
                                                 p_resource_entry->resource_handle);

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
    int done_bad;
    struct ipr_cmd_status cmd_status;
    struct ipr_cmd_status_record *p_record;
    int not_done = 0;
    int rc, i, j;
    struct ipr_passthru_ioctl *ioa_ioctl;
    struct devs_to_init_t *p_cur_dev_to_init;
    u32 percent_cmplt = 0;
    int num_includes = 0;
    int fd, found;
    struct ipr_ioa *p_cur_ioa;
    struct array_cmd_data *p_cur_raid_cmd;
    struct ipr_array_record *p_array_record;
    struct ipr_record_common *p_common_record_saved;
    struct ipr_device_record *p_device_record_saved;
    struct ipr_record_common *p_common_record;
    struct ipr_device_record *p_device_record;

    char *title, *body, *complete;

    title = catgets(catd,n_dev_include_complete.text_set,1,"Initialize and Format Status");
    body = malloc(strlen(catgets(catd,n_dev_include_complete.text_set,2,"You selected to initialize and format a disk unit"""))+20);
    complete = catgets(catd,n_dev_include_complete.text_set,3,"%3d%% Complete");

    sprintf(body,catgets(catd,n_dev_include_complete.text_set,2,"%3d%% Complete"),percent_cmplt,0);

    complete_screen_driver(title,body,complete);

    /* Here we need to sleep for a bit to make sure our children have started
     their formats. There isn't really a good way to know whether or not the formats
     have started, so we just wait for 60 seconds */
    sleep(60);

    while(1)
    {
        sprintf(body,catgets(catd,n_dev_include_complete.text_set,2,"%3d%% Complete"),percent_cmplt,0);
        complete_screen_driver(title,body,complete);

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

                rc = ipr_query_command_status(fd, p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_handle, &cmd_status);

                if (rc != 0)
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

        if (!not_done)
        {
            flush_stdscr();
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
                rc = 26;
                return rc | EXIT_FLAG; /* "Include failed" */
            }

            break;
        }
        not_done = 0;
        sleep(10);
    }

    sprintf(body,catgets(catd,n_dev_include_complete.text_set,2,"BANG %d %d"),100,0);
    complete_screen_driver(title,body,complete);

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
                rc = 26; /* Include failed" */
                syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                continue;
            }

            p_array_record = (struct ipr_array_record *)
                p_cur_raid_cmd->p_ipr_device->p_qac_entry;

            if (p_array_record->common.record_id == IPR_RECORD_ID_ARRAY2_RECORD)
            {
                /* refresh qac data for ARRAY2 */
                ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_array_query_data));
                ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
                ioa_ioctl->buffer_len = sizeof(struct ipr_array_query_data);
                ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

                ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
                ioa_ioctl->cmd_pkt.write_not_read = 0;
                ioa_ioctl->cmd_pkt.cdb[0] = IPR_QUERY_ARRAY_CONFIG;

                ioa_ioctl->cmd_pkt.cdb[1] = 0x01; /* Volume Set Include */
                ioa_ioctl->cmd_pkt.cdb[2] = p_cur_raid_cmd->array_id;
                ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
                ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

                rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

                if (rc != 0)
                {
                    if (errno != EINVAL)
                        syslog(LOG_ERR,"Query Array Config to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    close(fd);
                    free(ioa_ioctl);
                    continue;
                }
                else
                {
                    memcpy(p_cur_ioa->p_qac_data, &ioa_ioctl->buffer[0], sizeof(struct ipr_array_query_data));
                    free(ioa_ioctl);
                }

                /* scan through this raid command to determine which
                 devices should be part of this parity set */
                p_common_record_saved = (struct ipr_record_common *)p_cur_raid_cmd->p_qac_data->data;
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

            rc = ipr_add_array_device(fd, p_cur_ioa->p_qac_data);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Add Array Device to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                rc = 26;
                close(fd);

            }
            close(fd);
        }
    }

    not_done = 0;
    percent_cmplt = 0;

    while(1)
    {
        sprintf(body,catgets(catd,n_dev_include_complete.text_set,2,"BANG %d %d"),100,percent_cmplt);
        complete_screen_driver(title,body,complete);

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

                rc = ipr_query_command_status(fd, p_cur_ioa->ioa.p_resource_entry->resource_handle, &cmd_status);

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
                                    rc = 26; /* "Include failed" */
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (!not_done)
        {
            /* Call ioctl() to re-read partition table to handle change in size */
            for (p_cur_raid_cmd = p_head_raid_cmd;
                 p_cur_raid_cmd != NULL; 
                 p_cur_raid_cmd = p_cur_raid_cmd->p_next)
            {
                if (p_cur_raid_cmd->do_cmd)
                {
                    p_common_record =
                        p_cur_raid_cmd->p_ipr_device->p_qac_entry;
                    if ((p_common_record != NULL) &&
                        (p_common_record->record_id == IPR_RECORD_ID_ARRAY2_RECORD))
                    {
                        fd = open(p_cur_raid_cmd->p_ipr_device->dev_name,
                                  O_RDONLY | O_NONBLOCK);
                        if (fd < 0)
                        {
                            syslog(LOG_ERR,
                                   "Could not open %s to complete Raid Include. %m"IPR_EOL,
                                   p_cur_raid_cmd->p_ipr_device->dev_name);
                        }
                        else
                        {
                            rc = ioctl(fd, BLKRRPART, NULL);
                            if (rc < 0)
                                syslog(LOG_ERR,
                                       "Failure to re-read partition table on %s. %m"IPR_EOL,
                                       p_cur_raid_cmd->p_ipr_device->dev_name);
                        }
                    }
                }
            }

            flush_stdscr();

            if (done_bad)
            {
                rc =  26;
                return rc | EXIT_FLAG; /* "Include failed" */
            }

            check_current_config(false);

            rc = 27; /*  "Include Unit completed successfully" */

            sprintf(body,catgets(catd,n_dev_include_complete.text_set,2,"BANG %d %d"),100,100);
            complete_screen_driver(title,body,complete);

            sleep(2);
            return rc | EXIT_FLAG;
        }
        not_done = 0;
        sleep(2);
    }
}


/* 3's */

int disk_unit_recovery(i_container *i_con)
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

  int rc;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"DISK UNIT RECOVERY FUNCTION CALLED");
  s_out = screen_driver(&n_disk_unit_recovery,output,i_con);
  rc = s_out->rc;
  free(s_out);
  output = free_fn_out(output);
  return rc;
}

int concurrent_add_device(i_container *i_con)
{
  int rc;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"DEVICE CONC ADD FUNCTION CALLED");
  


  /* FIXME */


  s_out = screen_driver(&n_device_concurrent_maintenance,output,i_con);
  rc = s_out->rc;
  free(s_out);
  output = free_fn_out(output);
  return rc;
} 

int concurrent_remove_device(i_container *i_con)
{
  int rc, j;
  int len = 0;
  char *buffer = NULL;
  int num_lines = 0;
  struct ipr_ioa *p_cur_ioa;
  struct ipr_resource_entry *p_resource_entry;
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONCURRENT REMOVE DEVICE FUNCTION CALLED");
  
  rc = RC_SUCCESS;
  i_con = free_i_con(i_con);

  check_current_config(false);
    
  for(p_cur_ioa = p_head_ipr_ioa;
      p_cur_ioa != NULL;
      p_cur_ioa = p_cur_ioa->p_next)
    {
      for (j = 0; j < p_cur_ioa->num_devices; j++)
	{
	  p_resource_entry = p_cur_ioa->dev[j].p_resource_entry;
	  if (p_resource_entry->is_ioa_resource)
	    {
	      continue;
	    }
	  
          buffer = print_device(&p_cur_ioa->dev[j],buffer,"%1",p_cur_ioa);
	  i_con = add_i_con(i_con,"\0", (char *)p_cur_ioa->dev[j].p_resource_entry, list);
	  
	  buf_size += 150;
	  out_str = realloc(out_str, buf_size);
	  
	  out_str = strcat(out_str,buffer);
	  
	  memset(buffer, 0, strlen(buffer));
	  
	  len = 0;
	  num_lines++;
	}
    }
    
  i_con = add_i_con(i_con,"1",NULL,list);


  if (num_lines == 0)
    {
      buffer = realloc(buffer, 32);
      sprintf(buffer,"\nNo devices found");
      out_str = strcat(out_str,buffer);
    }
    
  strip_trailing_whitespace(out_str);
    
  output->next = add_fn_out(output->next,1,out_str);
  
  s_out = screen_driver(&n_concurrent_remove_device,output,i_con);
  rc = s_out->rc;
  free(s_out);
  output = free_fn_out(output);
  return rc;  
}

int issue_concurrent_maintenance(i_container *i_con)
{
  int rc;
  mvaddstr(0,0,"CONCURRENT REMOVE DEVICE FUNCTION CALLED");

  return rc;


}

int device_concurrent_maintenance(i_container *i_con)
{
  int rc;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"DEVICE CONC MAINTENANCE FUNCTION CALLED");
  
  s_out = screen_driver(&n_device_concurrent_maintenance,output,i_con);
  rc = s_out->rc;
  free(s_out);
  output = free_fn_out(output);
  return rc;
}

int reclaim_cache(i_container* i_con)
{
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

  int fd, rc;
  struct ipr_ioa *p_cur_ioa;
  struct ipr_reclaim_query_data *p_reclaim_buffer, *p_cur_reclaim_buffer;
  int num_needed = 0;
  char *buffer = NULL;
  int buf_size = 100;
  char *out_str;
  int ioa_num = 0;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"RECLAIM CACHE FUNCTION CALLED");
    
  out_str = calloc(buf_size,sizeof(char));

  /* empty the linked list that contains field pointers */
  i_con = free_i_con(i_con);

  p_reclaim_buffer = (struct ipr_reclaim_query_data*)malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

  for (p_cur_ioa = p_head_ipr_ioa, p_cur_reclaim_buffer = p_reclaim_buffer; p_cur_ioa != NULL; p_cur_ioa = p_cur_ioa->p_next, p_cur_reclaim_buffer++)
    {
      fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

      if (fd <= 1)
	{
	  syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
	  continue;
	}

      rc = ipr_reclaim_cache_store(fd, IPR_RECLAIM_QUERY, p_cur_reclaim_buffer);

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
	      buffer = print_device(&p_cur_ioa->ioa,buffer,"%1",p_cur_ioa);

	      buf_size += 100;
	      out_str = realloc(out_str, buf_size);
	      out_str = strcat(out_str,buffer);
		
	      memset(buffer, 0, strlen(buffer));
	      ioa_num++;
	      i_con = add_i_con(i_con, "\0",(char *)p_cur_ioa,list);
	    }
	  p_cur_reclaim_buffer++;
	  p_cur_ioa = p_cur_ioa->p_next;
	}
    }
    
  else
    {
      free(out_str);
      output = free_fn_out(output);
      return 38; /* "No Reclaim IOA Cache Storage is necessary" */
    }
    
  output->next = add_fn_out(output->next,1,out_str);
    
  s_out = screen_driver(&n_reclaim_cache,output,i_con);
  rc = s_out->rc;
  free(s_out);
  output = free_fn_out(output);
  free(out_str);
  return rc;
}
 
 
int confirm_reclaim(i_container *i_con)
{
  /*
    Confirm Reclaim IOA Cache Storage

    The disk units that may be affected by the function are displayed.

    ATTENTION: Proceed with this function only if directed to from a
    service procedure.  Data in the IOA cache will be discarded.
    Filsystem corruption may result on the system.

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

  struct ipr_ioa *p_cur_ioa, *p_reclaim_ioa;
  struct ipr_resource_entry *p_resource_entry;
  struct ipr_query_res_state res_state;
  int fd, j, rc;
  char *buffer = NULL;
  i_container* current;
  int ioa_num = 0;
  int buf_size = 150*sizeof(char);
  char *out_str;

  /********************************/
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIRM RECLAIM CACHE FUNCTION CALLED");
  /********************************/

  current = i_con;

  while (current != NULL)
    {
      p_cur_ioa = (struct ipr_ioa *) current->data;
      if (p_cur_ioa != NULL)
	{
	  if (strcmp(current->field_data, "1") == 0)
	    {
	      if (ioa_num)
		{
		  /* Multiple IOAs selected */
		  ioa_num++;
		  break;
		}
	      else if (p_cur_ioa->p_reclaim_data->reclaim_known_needed ||  p_cur_ioa->p_reclaim_data->reclaim_unknown_needed)
		{
		  p_reclaim_ioa = p_cur_ioa;
		  ioa_num++;
		}
	    }
	}
      current = current->next_item;
    }

  if (ioa_num == 0)
    {
      output = free_fn_out(output);
      return 17; /* "Invalid option.  No devices selected." */
    }

  else if (ioa_num > 1)
    {
      output = free_fn_out(output);
      return 16; /* "Error.  More than one unit was selected." */
    }
    
  /* One IOA selected, ready to proceed */

  check_current_config(false);

  fd = open(p_reclaim_ioa->ioa.dev_name, O_RDWR);
  if (fd <= 1)
    {
      close(fd);
      syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
      output = free_fn_out(output);
      return (EXIT_FLAG | 37); /* "Reclaim IOA Cache Storage failed" */
    }

  for (j = 0; j < p_reclaim_ioa->num_devices; j++)
  {
      p_resource_entry = p_reclaim_ioa->dev[j].p_resource_entry;
      if (!IPR_IS_DASD_DEVICE(p_resource_entry->std_inq_data))
          continue;

      /* Do a query resource state to see whether or not the device will be affected by the operation */
      rc = ipr_query_resource_state(fd, p_resource_entry->resource_handle, &res_state);

      if (rc != 0)
      {
          syslog(LOG_ERR, "Query Resource State to a device (0x%08X) failed. %m"IPR_EOL,
                 IPR_GET_PHYSICAL_LOCATOR(p_resource_entry->resource_address));
          res_state.not_oper = 1;
      }

      /* Necessary? We have not set this field. Relates to not_oper?*/
      if (!res_state.read_write_prot)
          continue;

      buffer = print_device(&p_reclaim_ioa->dev[j],buffer,"%1",p_reclaim_ioa);

      buf_size += 150;
      out_str = realloc(out_str, buf_size);
      out_str = strcat(out_str,buffer);

      memset(buffer, 0, strlen(buffer));
  }

  close(fd);

  i_con = free_i_con(i_con);
  /* Save the chosen IOA for later use */
  add_i_con(i_con, "\0", p_reclaim_ioa, list);

  output->next = add_fn_out(output->next,1,out_str);

  s_out = screen_driver(&n_confirm_reclaim,output,i_con);
  rc = s_out->rc;
  free(s_out);
    
  free(out_str);

  if (rc>0)
    {
      output = free_fn_out(output);
      return rc;
    }

  /*
    ATTENTION!!!   ATTENTION!!!   ATTENTION!!!   ATTENTION!!!

    ATTENTION: Proceed with this function only if directed to from a
    service procedure.  Data in the IOA cache will be discarded.
    By proceeding you will be allowing UNKNOWN data loss..
    This data loss may or may not be detected by the host operating system.
    Filesystem corruption may result on the system.

    Press 's' to continue.
    Serial                 Resource
    Option    Number   Type   Model  Name
    1    000564FD   6717    050   /dev/sda
    1    000585F1   6717    050   /dev/sdb
    1    000591FB   6717    050   /dev/sdc
    1    00056880   6717    050   /dev/sdd
    1    000595AD   6717    050   /dev/sde


    q=Cancel    s=Confirm reclaim
  */
  
  mvaddstr(0,0,"CONFIRM RECLAIM WARNING FUNCTION CALLED");
    
  s_out = screen_driver(&n_confirm_reclaim_warning,output,i_con);
  rc = s_out->rc;
  free(s_out);
  output = free_fn_out(output);
  return rc;
}

int reclaim_result(i_container *i_con)
{
    /*
     Reclaim IOA Cache Storage Results

     IOA cache storage reclamation has completed.  Use the number
     of lost sectors to decide whether to restore data from the
     most recent save media or to continue with possible data loss.     

     Number of lost sectors . . . . . . . . . . :       40168

     Press Enter to continue.




     e=Exit   q=Cancel

     */

    int rc, fd;
    struct ipr_ioa *p_reclaim_ioa;
    char* out_str;
    int max_y,max_x;
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"RECLAIM RESULT FUNCTION CALLED");
    int action;

    out_str = calloc (13, sizeof(char));
    p_reclaim_ioa = (struct ipr_ioa *) i_con->data;

    fd = open(p_reclaim_ioa->ioa.dev_name, O_RDWR);
    if (fd <= 1)
    {
        close(fd);
        free(out_str);
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
        output = free_fn_out(output);
        return (EXIT_FLAG | 37); /* "Reclaim IOA Cache Storage failed" */
    }

    action = IPR_RECLAIM_PERFORM;

    if (p_reclaim_ioa->p_reclaim_data->reclaim_unknown_needed)
        action |= IPR_RECLAIM_UNKNOWN_PERM;

    rc = ipr_reclaim_cache_store(fd, action, p_reclaim_ioa->p_reclaim_data);

    if (rc != 0)
    {
        free(out_str);
        syslog(LOG_ERR, "Reclaim to %s failed. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
        close(fd);
        output = free_fn_out(output);
        return (EXIT_FLAG | 37); /* "Reclaim IOA Cache Storage failed" */
    }

    close(fd);

    /* Everything going according to plan. Proceed with reclaim. */

    getmaxyx(stdscr,max_y,max_x);
    move(max_y-1,0);printw("Please wait - reclaim in progress");
    refresh();

    fd = open(p_reclaim_ioa->ioa.dev_name, O_RDWR);

    if (fd <= 1)
    {
        free(out_str);
        close(fd);
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
        output = free_fn_out(output);
        return (EXIT_FLAG | 37); /* "Reclaim IOA Cache Storage failed" */
    }

    memset(p_reclaim_ioa->p_reclaim_data, 0, sizeof(struct ipr_reclaim_query_data));
    rc = ipr_reclaim_cache_store(fd, IPR_RECLAIM_QUERY, p_reclaim_ioa->p_reclaim_data);

    close(fd);

    if (!rc)
    {
        if (p_reclaim_ioa->p_reclaim_data->reclaim_known_performed)
        {
            if (p_reclaim_ioa->p_reclaim_data->num_blocks_needs_multiplier)
            {
                sprintf(out_str, "%12d", iprtoh16(p_reclaim_ioa->p_reclaim_data->num_blocks) *
                        IPR_RECLAIM_NUM_BLOCKS_MULTIPLIER);
            }
            else
            {
                sprintf(out_str, "%12d", iprtoh16(p_reclaim_ioa->p_reclaim_data->num_blocks));
            }
            mvaddstr(8, 0, "Press Enter to continue.");
        }
        else if (p_reclaim_ioa->p_reclaim_data->reclaim_unknown_performed)
        {
            output->cat_offset = 3;
        }
    }

    fd = open(p_reclaim_ioa->ioa.dev_name, O_RDWR);

    if (fd <= 1)
    {
        rc = (EXIT_FLAG | 37);
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
    }
    else
    {
        rc = ipr_reclaim_cache_store(fd, IPR_RECLAIM_RESET, p_reclaim_ioa->p_reclaim_data);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Reclaim reset to %s failed. %m"IPR_EOL, p_reclaim_ioa->ioa.dev_name);
            rc = (EXIT_FLAG | 37);
        }
        close(fd);
    }

    check_current_config(false);


    output->next = add_fn_out(output->next,1,out_str);

    s_out = screen_driver(&n_reclaim_result,output,i_con);
    rc = s_out->rc;
    free(s_out);
    free(out_str);
    output = free_fn_out(output);
    return rc;
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

  int fd, rc, j;
  struct ipr_resource_entry *p_resource_entry;
  struct ipr_device_record *p_device_record;
  int can_init;
  int dev_type;
  int change_size;
  char *buffer = NULL;
  int num_devs = 0;
  struct ipr_ioa *p_cur_ioa;
  u32 len=0;
  struct devs_to_init_t *p_cur_dev_to_init;
  u8 cdb[IPR_CCB_CDB_LEN];
  u8 ioctl_buffer[IOCTL_BUFFER_SIZE];
  struct ipr_mode_parm_hdr *p_mode_parm_hdr;
  struct ipr_block_desc *p_block_desc;
  int status;
  struct sense_data_t sense_data;
  
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));

  struct screen_output *s_out;
  fn_out *output = init_output();  
  mvaddstr(0,0,"CONFIGURE AF DEVICE  FUNCTION CALLED");

  i_con = free_i_con(i_con);

  rc = RC_SUCCESS;
  
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
		  can_init = is_format_allowed(&p_cur_ioa->dev[j]);
		  
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

	      can_init = is_format_allowed(&p_cur_ioa->dev[j]);
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

	      buffer = print_device(&p_cur_ioa->dev[j],buffer,"%1",p_cur_ioa);
	      
	      i_con = add_i_con(i_con,"\0",dev_field,list);

	      buf_size += 100;
	      out_str = realloc(out_str, buf_size);
	      out_str = strcat(out_str,buffer);
	      memset(buffer, 0, strlen(buffer));
	      
	      len = 0;
	      num_devs++;
            }
        }
    }
    
  if (!num_devs)
    {
      if (action_code == IPR_INCLUDE)
	s_out = screen_driver(&n_af_include_fail,output,i_con);
      else
	s_out = screen_driver(&n_af_remove_fail,output,i_con);
    }
  else
    {
      output->next = add_fn_out(output->next,1,out_str);
      s_out = screen_driver(&n_configure_af_device,output,i_con);
    }

  rc = s_out->rc;
  free(s_out);
  free(out_str);
  output = free_fn_out(output);

  p_cur_dev_to_init = p_head_dev_to_init;
  while(p_cur_dev_to_init)
    {
      p_cur_dev_to_init = p_cur_dev_to_init->p_next;
      free(p_head_dev_to_init);
      p_head_dev_to_init = p_cur_dev_to_init;
    }
  
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
/*                    Configure a hot spare device

Select the subsystems which disk units will be configured as hot spares

Type choice, press Enter.
  1=Select subsystem

         Vendor    Product            Serial     PCI    PCI
Option    ID         ID               Number     Bus    Dev
         IBM       2780001           3AFB348C     05     03
         IBM       5703001           A775915E     04     01


e=Exit   q=Cancel    f=PageDn   b=PageUp
*/
  int rc, array_num, i;
  struct ipr_record_common *p_common_record;
  struct ipr_array_record *p_array_record;
  struct ipr_device_record *p_device_record;
  struct array_cmd_data *p_cur_raid_cmd;
  char *buffer = NULL;
  int num_lines = 0;
  struct ipr_ioa *p_cur_ioa;
  u32 len;
  int found = 0;
  char *p_char;
  struct ipr_resource_entry *p_resource_entry;
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  i_container *temp_i_con;
  struct screen_output *s_out;
  fn_out *output = init_output();  
  mvaddstr(0,0,"BUS CONFIG FUNCTION CALLED");
  
  i_con = free_i_con(i_con);

  rc = RC_SUCCESS;

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

	      i_con = add_i_con(i_con,"\0",(char *)p_tail_raid_cmd,list);
	      
	      p_resource_entry = p_cur_ioa->ioa.p_resource_entry;

	      buffer = print_device(&p_cur_ioa->ioa,buffer,"%1",p_cur_ioa);
      
              buf_size += 150;
	      out_str = realloc(out_str, buf_size);
	      
	      out_str = strcat(out_str,buffer);
	      memset(buffer, 0, strlen(buffer));

	      len = 0;
	      num_lines++;
	      array_num++;
	      break;
            }
        }
    }

  if (array_num == 1)
    {
      if (action == IPR_ADD_HOT_SPARE)
	s_out = screen_driver(&n_add_hot_spare_fail,output,i_con);
      else
	s_out = screen_driver(&n_remove_hot_spare_fail,output,i_con);

      rc = EXIT_FLAG;
      goto leave;
    }
  
  if (action == IPR_RMV_HOT_SPARE)
    output->cat_offset = 3;

  output->next = add_fn_out(output->next,1,out_str);
  s_out = screen_driver(&n_hot_spare,output,i_con);
  rc = s_out->rc;
  i_con = s_out->i_con;

  free(s_out);

  if (rc > 0)
    {
      rc = EXIT_FLAG;
      goto leave;
    }
  
  found = 0;
  
  p_cur_raid_cmd = p_head_raid_cmd;
  
  for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
      p_cur_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
      if (p_cur_raid_cmd != NULL)
	{
	  p_char = temp_i_con->field_data;
	  if (strcmp(p_char, "1") == 0)
	    {
	      do
		{
		  rc = select_hot_spare(temp_i_con, action);
		} while (rc & REFRESH_FLAG);
	      
	      if (rc > 0)
		goto leave;
	      
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
  
  if (found)
    {
      /* Go to verification screen */
      rc = confirm_hot_spare(action);
      goto leave;
    }
  else
    {
      rc = REFRESH_FLAG | INVALID_OPTION_STATUS;
    }

 leave:

  if (rc & CANCEL_FLAG)
    {
      s_status.index = (rc & ~CANCEL_FLAG);
      rc = (rc | REFRESH_FLAG) & ~CANCEL_FLAG;
    }
  i_con = free_i_con(i_con);
  free(out_str);
  output = free_fn_out(output); 
  
  p_cur_raid_cmd = p_head_raid_cmd;
  while(p_cur_raid_cmd)
    {
      p_cur_raid_cmd = p_cur_raid_cmd->p_next;
      free(p_head_raid_cmd);
      p_head_raid_cmd = p_cur_raid_cmd;
    }

  return rc;
}

int select_hot_spare(i_container *i_con, int action)
{
  struct array_cmd_data *p_cur_raid_cmd;
  int rc, j, found;
  char *buffer = NULL;
  int num_devs = 0;
  struct ipr_ioa *p_cur_ioa;
  char *p_char;
  u32 len;
  struct ipr_device *p_ipr_device;
  struct ipr_resource_entry *p_resource_entry;
  struct ipr_device_record *p_device_record;
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  i_container *temp_i_con,*i_con2=NULL;
  struct screen_output *s_out;
  fn_out *output = init_output();  
  mvaddstr(0,0,"SELECT HOT SPARE FUNCTION CALLED");
  
  p_cur_raid_cmd = (struct array_cmd_data *) i_con->data;
  
  rc = RC_SUCCESS;
  
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
	  i_con2 = add_i_con(i_con2,"\0",(char *)&p_cur_ioa->dev[j],list);

          buffer = print_device(&p_cur_ioa->dev[j],buffer,"%1",p_cur_ioa);
	  
	  buf_size += 150;
	  out_str = realloc(out_str, buf_size);
	  
	  out_str = strcat(out_str,buffer);
	  
	  memset(buffer, 0, strlen(buffer));

	  len = 0;
	  num_devs++;
        }
    }
  
  if (num_devs)
    {
      output->next = add_fn_out(output->next,1,out_str);
      
      if (action == IPR_RMV_HOT_SPARE)
	output->cat_offset = 3;
      
      s_out = screen_driver(&n_select_hot_spare,output,i_con2);
      rc = s_out->rc;
      i_con2 = s_out->i_con;

      if (rc > 0)
	goto leave;
      
      found = 0;
      
      for (temp_i_con = i_con2; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
	{
	  p_ipr_device = (struct ipr_device *)temp_i_con->data;
	  if (p_ipr_device != NULL)
	    {
	      p_char = temp_i_con->field_data;
	      if (strcmp(p_char, "1") == 0)
		{
		  found = 1;
		  p_device_record = (struct ipr_device_record *)p_ipr_device->p_qac_entry;
		  p_device_record->issue_cmd = 1;
		}
	    }
	}
      
      if (found)
	rc = RC_SUCCESS;
	
      else
	{
	  s_status.index = INVALID_OPTION_STATUS;
	  rc = REFRESH_FLAG;
	}
    }

  else
    {
      rc = 52 | CANCEL_FLAG; /* "No devices available for the selected hot spare operation"  */
    }

 leave:
  i_con2 = free_i_con(i_con2);
  free(s_out);
  free(out_str);
  output = free_fn_out(output);
  return rc;
}

int confirm_hot_spare(int action)
{
 
  /*
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
  struct ipr_resource_entry *p_resource_entry;
  struct array_cmd_data *p_cur_raid_cmd;
  struct ipr_device_record *p_device_record;
  struct ipr_ioa *p_cur_ioa;
  int rc, j;
  char *buffer = NULL;
  u32 num_lines = 0;
  int hot_spare_complete(int action);
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  
  struct screen_output *s_out;
  fn_out *output = init_output();  
  mvaddstr(0,0,"CONFIRM HOT SPARE FUNCTION CALLED");

  rc = RC_SUCCESS;
  
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
                  buffer = print_device(&p_cur_ioa->dev[j],buffer,"1",p_cur_ioa);

		  buf_size += 150;
		  out_str = realloc(out_str, buf_size);
		  
		  out_str = strcat(out_str,buffer);

		  memset(buffer, 0, strlen(buffer));
		  num_lines++;
                }
            }
        }
      p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }

  output->next = add_fn_out(output->next,1,out_str);
  
  if (action == IPR_RMV_HOT_SPARE)
    output->cat_offset = 3;

  s_out = screen_driver(&n_confirm_hot_spare,output,NULL);
  rc = s_out->rc;
  free(s_out);
  
  if(rc > 0)
    goto leave;

  /* verify the devices are not in use */
  p_cur_raid_cmd = p_head_raid_cmd;
  while (p_cur_raid_cmd)
    {
      if (p_cur_raid_cmd->do_cmd)
	{
	  p_cur_ioa = p_cur_raid_cmd->p_ipr_ioa;
	  if (rc != 0)
	    {
	      if (action == IPR_ADD_HOT_SPARE)
		rc = 55 | EXIT_FLAG; /* "Configure a hot spare device failed"  */
	      else if (action == IPR_RMV_HOT_SPARE)
		rc = 56 | EXIT_FLAG; /* "Unconfigure a hot spare device failed"  */
	      
	      syslog(LOG_ERR, "Devices may have changed state. Command cancelled, "
		     "please issue commands again if RAID still desired %s."
		     IPR_EOL, p_cur_ioa->ioa.dev_name);
	      goto leave;
	    }
	}
      p_cur_raid_cmd = p_cur_raid_cmd->p_next;
    }
  
  rc = hot_spare_complete(action);
  
 leave:
  free(out_str);
  output = free_fn_out(output);
  return rc;
}

int hot_spare_complete(int action)
{
    /*
     no screen is displayed for this action 
     */

    int rc;
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
            flush_stdscr();
            fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
            if (fd <= 1)
            {
                if (action == IPR_ADD_HOT_SPARE)
                    rc = 55 | EXIT_FLAG; /* "Configure a hot spare device failed"  */
                else if (action == IPR_RMV_HOT_SPARE)
                    rc = 56 | EXIT_FLAG; /* "Unconfigure a hot spare device failed"  */

                syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                return rc;
            }

            if (action == IPR_ADD_HOT_SPARE)
                rc = ipr_add_hot_spare(fd, p_cur_ioa->p_qac_data);
            else
                rc = ipr_remove_hot_spare(fd, p_cur_ioa->p_qac_data);

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

    flush_stdscr();

    if (action == IPR_ADD_HOT_SPARE)
        rc = 53 | EXIT_FLAG; /* "Enable Device as Hot Spare Completed successfully"  */
    else
        rc = 54 | EXIT_FLAG; /* "Disable Device as Hot Spare Completed successfully"  */

    return rc;
}

int battery_maint(i_container *i_con)
{
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

    int fd, rc;
    struct ipr_reclaim_query_data *p_reclaim_buffer, *p_cur_reclaim_buffer;
    struct ipr_ioa *p_cur_ioa;
    int num_needed = 0;
    int ioa_num = 0;
    char *buffer = NULL;
    int len = 0;
    int buf_size = 100;
    char* out_str = calloc(buf_size,sizeof(char)) ;
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"BATTERY MAINT FUNCTION CALLED");

    i_con = free_i_con(i_con);

    p_reclaim_buffer = (struct ipr_reclaim_query_data*)malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

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

        rc = ipr_reclaim_cache_store(fd, IPR_RECLAIM_QUERY | IPR_RECLAIM_EXTENDED_INFO, p_cur_reclaim_buffer);

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
                buffer = print_device(&p_cur_ioa->ioa,buffer,"%1",p_cur_ioa);

                buf_size += 100;
                out_str = realloc(out_str, buf_size);
                out_str = strcat(out_str,buffer);

                memset(buffer, 0, strlen(buffer));
                len = 0;
                ioa_num++;
                i_con = add_i_con(i_con, "\0",(char *)p_cur_ioa,list); 
            }
            p_cur_reclaim_buffer++;
            p_cur_ioa = p_cur_ioa->p_next;
        }
    }
    else
    {
        output = free_fn_out(output);
        free(out_str);
        return 44; /* No configured resources contain a cache battery pack */
    }

    output->next = add_fn_out(output->next,1,out_str);

    s_out = screen_driver(&n_battery_maint,output,i_con);
    rc = s_out->rc;
    free(s_out);
    free(out_str);
    output = free_fn_out(output);
    return rc;
}

int battery_fork(i_container *i_con)
{
  int rc = 0;
  int force_error = 0;
  struct ipr_ioa *p_cur_ioa;
  char *p_char;

  i_container *temp_i_con;
  mvaddstr(0,0,"CONFIRM FORCE BATTERY ERROR FUNCTION CALLED");

  for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
      p_cur_ioa = (struct ipr_ioa *)temp_i_con->data;
      if (p_cur_ioa != NULL)
	{
	  p_char = temp_i_con->field_data;

	  if (strcmp(p_char, "2") == 0)
	    {
	      force_error++;
	      p_cur_ioa->ioa.is_reclaim_cand = 1;
	    }
	  else if (strcmp(p_char, "5") == 0)
	    {
	      rc = show_battery_info(temp_i_con);/*p_cur_ioa);*/
	      return rc;
   	    }
	  else
	    {
	      p_cur_ioa->ioa.is_reclaim_cand = 0;
	    }
	}
    }

  if (force_error)
    {
      rc = confirm_force_battery_error(NULL);
      return rc;
    }

  return INVALID_OPTION_STATUS;
}

int confirm_force_battery_error(i_container *i_con)
{
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



    e=Exit   q=Cancel    f=PageDn   b=PageUp

  */

  int rc;
  struct ipr_ioa *p_cur_ioa;
  int ioa_num = 0;
  char *buffer = NULL;

  int buf_size = 100;
  char *out_str = calloc(buf_size,sizeof(char));
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIRM FORCE BATTERY ERROR FUNCTION CALLED");

  rc = RC_SUCCESS;

  p_cur_ioa = p_head_ipr_ioa;

  while(p_cur_ioa)
    {
      if ((p_cur_ioa->p_reclaim_data) &&
	  (p_cur_ioa->ioa.is_reclaim_cand))
        {
          buffer = print_device(&p_cur_ioa->ioa,buffer,"2",p_cur_ioa);
	  
	  buf_size += 100;
	  out_str = realloc(out_str, buf_size);
	  out_str = strcat(out_str,buffer);

	  memset(buffer, 0, strlen(buffer));
	  ioa_num++;
	}

      p_cur_ioa = p_cur_ioa->p_next;
    }

  output->next = add_fn_out(output->next,1,out_str);

  s_out = screen_driver(&n_confirm_force_battery_error,output,i_con);
  rc = s_out->rc;
  free(s_out);
  free(out_str);
  output = free_fn_out(output);
  return rc;
}

int force_battery_error(i_container *i_con)
{
    int fd, rc, reclaim_rc;
    struct ipr_ioa *p_cur_ioa;

    struct ipr_reclaim_query_data reclaim_buffer;
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"FORCE BATTERY ERROR FUNCTION CALLEd");

    reclaim_rc = rc = RC_SUCCESS;

    p_cur_ioa = p_head_ipr_ioa; /* Scott's bug fix 1 */

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
                reclaim_rc = 43; /* "Attempting to force the selected battery packs into an error state failed." */
                p_cur_ioa = p_cur_ioa->p_next;
                continue;
            }

            rc = ipr_reclaim_cache_store(fd, IPR_RECLAIM_FORCE_BATTERY_ERROR, &reclaim_buffer);

            close(fd);
            if (rc != 0)
            {
                if (errno != EINVAL)
                {
                    syslog(LOG_ERR,
                           "Reclaim to %s for Battery Maintenance failed. %m"IPR_EOL,
                           p_cur_ioa->ioa.dev_name);
                    reclaim_rc = 43; /* "Attempting to force the selected battery packs into an error state failed." */
                }
            }

            if (reclaim_buffer.action_status !=
                IPR_ACTION_SUCCESSFUL)
            {
                syslog(LOG_ERR,
                       "Battery Force Error to %s failed. %m"IPR_EOL,
                       p_cur_ioa->ioa.dev_name);
                reclaim_rc = 43; /* "Attempting to force the selected battery packs into an error state failed." */
            }
        }
        p_cur_ioa = p_cur_ioa->p_next;
    }

    if (reclaim_rc != RC_SUCCESS)
        return reclaim_rc;

    s_out = screen_driver(&n_force_battery_error,output,i_con);
    rc = s_out->rc;
    free(s_out);
    output = free_fn_out(output);
    return rc;
}

int show_battery_info(i_container *i_con)
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

  struct ipr_ioa *p_ioa = (struct ipr_ioa *) i_con->data;
  int rc;
  struct ipr_reclaim_query_data *p_reclaim_data =
    p_ioa->p_reclaim_data;
  char *buffer;
    
  buffer = malloc(100);
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"SHOW BATTERY INFO FUNCTION CALLED");

  output->next = add_fn_out(output->next,1,p_ioa->ioa.dev_name);
    
  sprintf(buffer,"%-8s", "00000000"); /* FIXME */
  output->next = add_fn_out(output->next,2,buffer);

  sprintf(buffer,"%X-001", p_ioa->ccin);
  output->next = add_fn_out(output->next,3,buffer);

  output->next = add_fn_out(output->next,4,p_ioa->pci_address);
  
  sprintf(buffer,"%d", p_ioa->host_no);
  output->next = add_fn_out(output->next,6,buffer);
  
  switch (p_reclaim_data->rechargeable_battery_type)
    {
    case IPR_BATTERY_TYPE_NICD:
      sprintf(buffer,catgets(catd,n_show_battery_info.text_set,4,"Nickel Cadmium (NiCd)"));
      break;
    case IPR_BATTERY_TYPE_NIMH:
      sprintf(buffer,catgets(catd,n_show_battery_info.text_set,5,"Nickel Metal Hydride (NiMH)"));
      break;
    case IPR_BATTERY_TYPE_LIION:
      sprintf(buffer,catgets(catd,n_show_battery_info.text_set,6,"Lithium Ion (LiIon)"));
      break;
    default:
      sprintf(buffer,catgets(catd,n_show_battery_info.text_set,7,"Unknown"));
      break;
    }

  output->next = add_fn_out(output->next,7,buffer);
	
  switch (p_reclaim_data->rechargeable_battery_error_state)
    {
    case IPR_BATTERY_NO_ERROR_STATE:
      sprintf(buffer,catgets(catd,n_show_battery_info.text_set,8,"No battery warning"));
      break;
    case IPR_BATTERY_WARNING_STATE:
      sprintf(buffer,catgets(catd,n_show_battery_info.text_set,9,"Battery warning issued"));
      break;
    case IPR_BATTERY_ERROR_STATE:
      sprintf(buffer,catgets(catd,n_show_battery_info.text_set,10,"Battery error issued"));
      break;
    default:
      sprintf(buffer,catgets(catd,n_show_battery_info.text_set,7,"Unknown"));
      break;
    }
    
  output->next = add_fn_out(output->next,8,buffer);
    
  sprintf(buffer,"%d",p_reclaim_data->raw_power_on_time);
  output->next = add_fn_out(output->next,9,buffer);

  sprintf(buffer,"%d",p_reclaim_data->adjusted_power_on_time);
  output->next = add_fn_out(output->next,10,buffer);

  sprintf(buffer,"%d",p_reclaim_data->estimated_time_to_battery_warning);
  output->next = add_fn_out(output->next,11,buffer);

  sprintf(buffer,"%d",p_reclaim_data->estimated_time_to_battery_failure);
  output->next = add_fn_out(output->next,12,buffer);

  s_out = screen_driver(&n_show_battery_info,output,i_con);
  rc = s_out->rc;
  free(s_out);
  output = free_fn_out(output);
  free(buffer);
  return rc;
}

/* 4's */

int bus_config(i_container *i_con)
{
    /*      Work with SCSI Bus Configuration

     Select the subsystem to change scsi bus attribute.

     Type choice, press Enter.
     1=change scsi bus attribute

     Vendor    Product            Serial     PCI    PCI
     Option    ID         ID               Number     Bus    Dev
     IBM       2780001           3AFB348C     05     03
     IBM       5703001           A775915E     04     01

     e=Exit   q=Cancel    f=PageDn   b=PageUp
     */
    int rc, config_num;
    struct array_cmd_data *p_cur_raid_cmd;
    char *buffer = NULL;
    int num_lines = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len = 0;
    u8 ioctl_buffer[255];
    struct ipr_mode_parm_hdr *p_mode_parm_header;
    struct ipr_mode_page_28_header *p_modepage_28_header;
    int fd;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));

    struct screen_output *s_out;
    fn_out *output = init_output();  
    mvaddstr(0,0,"BUS CONFIG FUNCTION CALLED");
    rc = RC_SUCCESS;

    i_con = free_i_con(i_con);

    config_num = 0;

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

        /* mode sense page28 to focal point */
        p_mode_parm_header = (struct ipr_mode_parm_hdr *)ioctl_buffer;
        rc = ipr_mode_sense_page_28(fd, p_mode_parm_header);

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

        i_con = add_i_con(i_con,"\0",(char *)p_cur_ioa,list);

        buffer = print_device(&p_cur_ioa->ioa,buffer,"%1",p_cur_ioa);

        buf_size += 150;
        out_str = realloc(out_str, buf_size);

        out_str = strcat(out_str,buffer);
        memset(buffer, 0, strlen(buffer));

        len = 0;
        num_lines++;
        config_num++;
    }

    if (config_num == 0)
    {
        s_out = screen_driver(&n_bus_config_fail,output,i_con);
    }

    else
    {
        output->next = add_fn_out(output->next,1,out_str);
        s_out = screen_driver(&n_bus_config,output,i_con);
    }

    rc = s_out->rc;
    free(s_out);
    free(out_str);
    output = free_fn_out(output);

    p_cur_raid_cmd = p_head_raid_cmd;
    while(p_cur_raid_cmd)
    {
        p_cur_raid_cmd = p_cur_raid_cmd->p_next;
        free(p_head_raid_cmd);
        p_head_raid_cmd = p_cur_raid_cmd;
    }	

    return rc;
}

int change_bus_attr(i_container *i_con)
{
    /*
     Change SCSI Bus Configuration

     Current Bus configurations are shown.  To change
     setting hit "c" for options menu.  Hightlight desired
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
    struct ipr_ioa *p_cur_ioa;
    int rc, j, i;
    FORM *p_form;
    FORM *p_fields_form;
    FIELD **p_input_fields, *p_cur_field;
    FIELD *p_title_fields[3];
    WINDOW *p_field_pad;
    int cur_field_index;
    char buffer[100];
    int input_field_index = 0;
    int num_lines = 0;
    u8 ioctl_buffer0[255];
    u8 ioctl_buffer1[255];
    u8 ioctl_buffer2[255];
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    int fd;
    struct ipr_page_28 *p_page_28_sense;
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
    i_container *temp_i_con;
    int found = 0;
    char *p_char;
    int bus_attr_menu(struct ipr_ioa *p_cur_ioa,
                      struct ipr_page_28 *p_page_28_cur,
                      struct ipr_page_28 *p_page_28_chg,
                      struct ipr_page_28 *p_page_28_dflt,
                      struct ipr_page_28 *p_page_28_ipr,
                      int cur_field_index,
                      int offset);

    mvaddstr(0,0,"CHANGE BUS ATTR FUNCTION CALLED");

    int max_x,max_y,start_y,start_x;

    getmaxyx(stdscr,max_y,max_x);
    getbegyx(stdscr,start_y,start_x);

    rc = RC_SUCCESS;

    found = 0;

    for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
        p_cur_ioa = (struct ipr_ioa *)temp_i_con->data;
        if (p_cur_ioa != NULL)
        {
            p_char = temp_i_con->field_data;
            if (strcmp(p_char, "1") == 0)
            {
                found = 1;
                break;
            }	
        }
    }

    if (!found)
        return INVALID_OPTION_STATUS;

    /* zero out user page 28 page, this data is used to indicate
     which fields the user changed */
    memset(&page_28_ipr, 0, sizeof(struct ipr_page_28));

    fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
        return 46 | EXIT_FLAG; /* "Change SCSI Bus configurations failed" */
    }

    memset(ioctl_buffer0,0,sizeof(ioctl_buffer0));
    memset(ioctl_buffer1,0,sizeof(ioctl_buffer1));
    memset(ioctl_buffer2,0,sizeof(ioctl_buffer2));

    /* mode sense page28 to get current parms */
    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer0;
    rc = ipr_mode_sense_page_28(fd, p_mode_parm_hdr);

    if (rc != 0)
    {
        close(fd);
        syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
               p_cur_ioa->ioa.dev_name);
        return 46 | EXIT_FLAG; /* "Change SCSI Bus configurations failed" */	
    }

    p_page_28_sense = (struct ipr_page_28 *)p_mode_parm_hdr;
    p_page_28_cur = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer1;
    p_page_28_chg = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer2;
    p_page_28_dflt = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* determine changable and default values */
    for (j = 0;
         j < p_page_28_cur->page_hdr.num_dev_entries;
         j++)
    {
        p_page_28_chg->attr[j].qas_capability = 0;
        p_page_28_chg->attr[j].scsi_id = 0; //FIXME!!! need to allow dart (by
                                            // vend/prod & subsystem id)
        p_page_28_chg->attr[j].bus_width = 1;
        p_page_28_chg->attr[j].max_xfer_rate = 1;

        p_page_28_dflt->attr[j].qas_capability = 0;
        p_page_28_dflt->attr[j].scsi_id = p_page_28_cur->attr[j].scsi_id;
        p_page_28_dflt->attr[j].bus_width = 16;
        p_page_28_dflt->attr[j].max_xfer_rate = ipr_max_xfer_rate(fd, p_page_28_cur->attr[j].res_addr.bus);
    }

    close(fd);

    /* Title */
    p_title_fields[0] =
        new_field(1, max_x - start_x,   /* new field size */ 
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

    p_form = new_form(p_title_fields);

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
    set_form_sub(p_fields_form, p_field_pad); /*???*/
    set_current_field(p_fields_form, p_input_fields[0]);
    for (i = 0; i < input_field_index; i++)
    {
        field_opts_off(p_input_fields[i], O_EDIT);
    }

    form_opts_off(p_fields_form, O_BS_OVERLOAD);

    flush_stdscr();
    clear();

    set_form_win(p_form,stdscr);
    set_form_sub(p_form,stdscr);

    post_form(p_form);
    post_form(p_fields_form);

    field_start_row = DSP_FIELD_START_ROW;
    field_end_row = max_y - 4;
    field_num_rows = field_end_row - field_start_row + 1;

    mvaddstr(2, 0, "Current Bus configurations are shown.  To change");
    mvaddstr(3, 0, "setting hit \"c\" for options menu.  Hightlight desired");
    mvaddstr(4, 0, "option then hit Enter");
    mvaddstr(6, 0, "c=Change Setting");
    if (field_num_rows < form_rows)
    {
        mvaddstr(field_end_row, 43, "More...");
        mvaddstr(max_y - 2, 25, "f=PageDn");
        field_end_row--;
        field_num_rows--;
    }
    mvaddstr(max_y - 4, 0, "Press Enter to Continue");
    mvaddstr(max_y - 2, 0, EXIT_KEY_LABEL CANCEL_KEY_LABEL);

    memset(buffer, 0, 100);
    sprintf(buffer,"%s",
            p_cur_ioa->ioa.dev_name);
    mvaddstr(8, 0, buffer);

    refresh();
    pad_start_row = 0;
    pnoutrefresh(p_field_pad,
                 pad_start_row, 0,
                 field_start_row, 0,
                 field_end_row,
                 max_x);
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
                 max_x);
        doupdate();
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

                if (rc == EXIT_FLAG)
                    goto leave;
            }
            else
            {
                clear();
                set_field_buffer(p_title_fields[0],
                                 0,
                                 "Processing Change SCSI Bus Configuration");
                mvaddstr(max_y - 2, 1, "Please wait for next screen");
                doupdate();
                refresh();

                /* issue mode select and reset if necessary */
                fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
                if (fd <= 1)
                {
                    syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
                    return 46 | EXIT_FLAG; /* "Change SCSI Bus configurations failed" */
                }

                rc = ipr_mode_select_page_28(fd, p_page_28_sense);

                if (rc != 0)
                {
                    close(fd);
                    syslog(LOG_ERR, "Mode Select to %s failed. %m"IPR_EOL,
                           p_cur_ioa->ioa.dev_name);
                    return 46 | EXIT_FLAG; /* "Change SCSI Bus configurations failed" */
                }

                ipr_save_page_28(p_cur_ioa,
                                 p_page_28_cur,
                                 p_page_28_chg,
                                 &page_28_ipr);

                if (is_reset_req)
                {
                    rc = ioctl(fd, IPR_IOCTL_RESET_IOA);
                }

                close(fd);
                rc = 45 | EXIT_FLAG;
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
                mvaddstr(2, 0, "Confirming this action will result in the following configuration to");
                mvaddstr(3, 0, "become active.");
                if (is_reset_req)
                {
                    mvaddstr(3, 0, "ATTENTION:  This action requires a reset of the Host Adapter.  Confirming");
                    mvaddstr(4, 0, "this action may affect system performance while processing changes.");
                }
                mvaddstr(6, 0, "Press c=Confirm Change SCSI Bus Configration ");
                if (field_num_rows < form_rows)
                {
                    mvaddstr(field_end_row + 1, 43, "More...");
                    mvaddstr(max_y - 2, 25, "f=PageDn");
                }
                mvaddstr(max_y - 2, 0, "c=Confirm   " EXIT_KEY_LABEL CANCEL_KEY_LABEL);

                memset(buffer, 0, 100);
                sprintf(buffer,"%s",
                        p_cur_ioa->ioa.dev_name);
                mvaddstr(8, 0, buffer);

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
                    mvaddstr(max_y - 2, 25, "        ");
                }
                mvaddstr(max_y - 2, 36, "b=PageUp");
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
                    mvaddstr(max_y - 2, 36, "        ");
                }
                form_adjust = 1;
                mvaddstr(max_y - 2, 25, "f=PageDn");
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

    flush_stdscr();
    return rc;
}

int init_device(i_container *i_con)
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
    int fd, rc, j;
    struct ipr_query_res_state res_state;
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_device_record *p_device_record;
    int can_init;
    int dev_type;
    char *buffer = NULL;
    int num_devs = 0;
    struct ipr_ioa *p_cur_ioa;
    u32 len=0;
    struct devs_to_init_t *p_cur_dev_to_init;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"INIT DEVICE FUNCTION CALLED");

    rc = RC_SUCCESS;

    i_con = free_i_con(i_con);

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
                        can_init = is_format_allowed(&p_cur_ioa->dev[j]);
                    }
                }
                else
                {
                    /* Do a query resource state */
                    rc = ipr_query_resource_state(fd, p_resource_entry->resource_handle, &res_state);

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
                        can_init = is_format_allowed(&p_cur_ioa->dev[j]);
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
                can_init = is_format_allowed(&p_cur_ioa->dev[j]);
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

                buffer = print_device(&p_cur_ioa->dev[j], buffer,"%1",p_cur_ioa);
                i_con = add_i_con(i_con,"\0",(char *)&p_cur_ioa->dev[j],list);

                buf_size += 150*sizeof(char);
                out_str = realloc(out_str, buf_size);
                out_str = strcat(out_str,buffer);

                memset(buffer, 0, strlen(buffer));

                len = 0;
                num_devs++;
            }
        }
        close(fd);
    }


    if (!num_devs)
    {
        free(out_str);
        output = free_fn_out(output);
        return 49; /* "No units available for initialize and format" */
    }

    output->next = add_fn_out(output->next,1,out_str);
    s_out = screen_driver(&n_init_device,output,i_con);
    rc = s_out->rc;
    free(s_out);

    p_cur_dev_to_init = p_head_dev_to_init;
    while(p_cur_dev_to_init)
    {
        p_cur_dev_to_init = p_cur_dev_to_init->p_next;
        free(p_head_dev_to_init);
        p_head_dev_to_init = p_cur_dev_to_init;
    }

    free(out_str);
    output = free_fn_out(output);
    return rc;
}

int confirm_init_device(i_container *i_con)
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
  int found;
  char  *p_char;
  char *buffer = NULL;
  struct devs_to_init_t *p_cur_dev_to_init;
  u32 num_devs = 0;
  int rc = RC_SUCCESS;
  i_container *temp_i_con, *i_num_devs;
  int buf_size = 150;
  char *out_str = calloc(buf_size,sizeof(char));
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIRM INIT DEVICE FUNCTION CALLED");
  found = 0;
  
  temp_i_con = copy_i_con(i_con);

  p_cur_dev_to_init = p_head_dev_to_init;
  
  for (; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
      if (temp_i_con->data != NULL)
	{
	  p_char = strip_trailing_whitespace(temp_i_con->field_data);
	  
	  if (strcmp(p_char, "1") == 0)
	    {
	      found = 1;
	      p_cur_dev_to_init->do_init = 1;
	    }
	  
	  p_cur_dev_to_init = p_cur_dev_to_init->p_next;
	}
    }

  if (!found)
    {
      free_fn_out(output);
      return INVALID_OPTION_STATUS;
    }

  p_cur_dev_to_init = p_head_dev_to_init;

  while(p_cur_dev_to_init)
    {
      if (p_cur_dev_to_init->do_init)
        {
          buffer = print_device(p_cur_dev_to_init->p_ipr_device,buffer,"1",p_cur_dev_to_init->p_ioa);
	  	  
	  buf_size += 150*sizeof(char);
	  out_str = realloc(out_str, buf_size);
	  out_str = strcat(out_str,buffer);
	  
	  memset(buffer, 0, strlen(buffer));
	  num_devs++;
        }
      p_cur_dev_to_init = p_cur_dev_to_init->p_next;
    }

  /*  i_num_devs = free_i_con(i_num_devs);*/
  i_num_devs = add_i_con(i_num_devs,"\0",&num_devs,list);
  
  output->next = add_fn_out(output->next,1,out_str);
  s_out = screen_driver(&n_confirm_init_device,output,i_num_devs);
  rc = s_out->rc;
  free(s_out);
  
  free(out_str);
  output = free_fn_out(output);
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
                    userptr[menu_index] = 16000;
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
  int max_y,max_x;

  getmaxyx(stdscr,max_y,max_x);
    
  p_menu = new_menu(menu_item);
  scale_menu(p_menu, &menu_rows, &menu_cols);
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
	  rc = EXIT_FLAG;
	  break;
	}
      else if (IS_CANCEL_KEY(ch))
	{
	  rc = CANCEL_FLAG;
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

int send_dev_inits(i_container *i_con) /*(u8 num_devs)*/
{
    u8 num_devs = (u32) i_con->data; /* passed from i_num_devs in confirm_init_device */
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
    int max_y, max_x;
    int length;

    int dev_init_complete(u8 num_devs);

    getmaxyx(stdscr,max_y,max_x);
    mvaddstr(max_y-1,0,catgets(catd,n_dev_init_complete.text_set,4,"Please wait for the next screen."));
    refresh();

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

            rc = ipr_query_resource_state(fd, p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_handle, &res_state);

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

            if (!ipr_is_hidden(p_cur_dev_to_init->p_ipr_device->p_resource_entry))
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
            rc = ipr_mode_sense(fd, 0x0a, p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_handle, ioctl_buffer);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                iprlog_location(p_cur_dev_to_init->p_ioa);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            /* Mode sense - changeable parms */
            rc = ipr_mode_sense(fd, 0x4a, p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_handle, ioctl_buffer2);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                iprlog_location(p_cur_dev_to_init->p_ioa);
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

            length = p_mode_parm_hdr->length + 1;
            p_mode_parm_hdr->length = 0;
            p_mode_parm_hdr->medium_type = 0;
            p_mode_parm_hdr->device_spec_parms = 0;
            p_control_mode_page->header.parms_saveable = 0;
            rc = ipr_mode_select(fd, p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_handle, ioctl_buffer, length);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Select %s to disable qerr failed. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                iprlog_location(p_cur_dev_to_init->p_ioa);
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

            rc = ipr_mode_select(fd, p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_handle, ioctl_buffer, sizeof(struct ipr_block_desc) +
                sizeof(struct ipr_mode_parm_hdr));

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Select to %s failed. %m"IPR_EOL,
                       p_cur_dev_to_init->p_ipr_device->dev_name);
                iprlog_location(p_cur_dev_to_init->p_ioa);
                close(fd);
                p_cur_dev_to_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            pid = fork();
            if (!pid)
            {
                /* Issue the format. Failure will be detected by query command status */
                rc = ipr_format_unit(fd, p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_handle);
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
                iprlog_location(p_cur_dev_to_init->p_ioa);
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
                iprlog_location(p_cur_dev_to_init->p_ioa);
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
                iprlog_location(p_cur_dev_to_init->p_ioa);
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
                iprlog_location(p_cur_dev_to_init->p_ioa);
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
                iprlog_location(p_cur_dev_to_init->p_ioa);
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
        return 51 | EXIT_FLAG; /* "Initialize and format failed" */;
}

int dev_init_complete(u8 num_devs)
{
    int done_bad;
    struct ipr_cmd_status cmd_status;
    struct ipr_cmd_status_record *p_record;
    int not_done = 0;
    int rc;
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

    char *title, *body, *complete;

    title = catgets(catd,n_dev_init_complete.text_set,1,"BOOM");
    body = catgets(catd,n_dev_init_complete.text_set,2,"BANG");
    complete = malloc(strlen(catgets(catd,n_dev_init_complete.text_set,3,"BANG %d"))+20);

    sprintf(complete,catgets(catd,n_dev_init_complete.text_set,3,"BANG %d"),percent_cmplt);

    while(1)
    {
        sprintf(complete,catgets(catd,n_dev_init_complete.text_set,3,"BANG %d"),percent_cmplt,0);
        complete_screen_driver(title,body,complete);

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

                rc = ipr_query_command_status(fd, p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_handle, &cmd_status);
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
                        fd = open(p_ioa->ioa.dev_name, O_RDWR);

                        if (fd < 0)
                        {
                            syslog(LOG_ERR, "Cannot open %s. %m\n", p_ioa->ioa.dev_name);
                        }
                        else
                        {
                            rc = ipr_mode_sense(fd, 0, p_cur_dev_to_init->p_ipr_device->p_resource_entry->resource_handle, ioctl_buffer);
                            
                            close(fd);

                            if (rc != 0)
                            {
                                syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
                                       p_cur_dev_to_init->p_ipr_device->dev_name);
                            }
                            else
                            {
                                p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
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

            flush_stdscr();

            if (done_bad)
                return 51 | EXIT_FLAG; /* "Initialize and format failed" */;
                return 50 | EXIT_FLAG; /* "Initialize and format completed successfully" */
        }
        not_done = 0;
        sleep(10);
    }


}

/* 5's */
#define MAX_CMD_LENGTH 1000

int log_menu(i_container *i_con)
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
  
  char cmnd[MAX_CMD_LENGTH];
  int rc;
  
  struct stat file_stat;
  char *buffer;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"LOG MENU FUNCTION CALLED");

  sprintf(cmnd,"%s/boot.msg",log_root_dir);
  rc = stat(cmnd, &file_stat);

  if (rc) /* file does not exist - do not display 8th option */
    output->cat_offset=3;
  else
    output->cat_offset=0;

  buffer = malloc(40);
  sprintf(buffer,"%s",editor);

  if (strchr(buffer,' '))
    *strchr(buffer,' ')='\0';

  output->next = add_fn_out(output->next,1,buffer);
    
  s_out = screen_driver(&n_log_menu,output,i_con);
  rc = s_out->rc;
  free(s_out);
  free(buffer);
  output = free_fn_out(output);
  return rc;
}


int ibm_storage_log_tail(i_container *i_con)
{
  char cmnd[MAX_CMD_LENGTH];
  int rc, len;
  mvaddstr(0,0,"IBM STORAGE LOG TAIL FUNCTION CALLED");

  def_prog_mode();
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
    
  if ((rc != 0) && (rc != 127))
    {
      s_status.num = rc;
      return 65; /* "Editor returned %d. Try setting the default editor" */
    }
  else
    return 1; /* return with no status */
}

int ibm_storage_log(i_container *i_con)
{
  char cmnd[MAX_CMD_LENGTH];
  int rc, i;
  struct dirent **log_files;
  struct dirent **pp_dirent;
  int num_dir_entries;
  int len;

  mvaddstr(0,0,"IBM STORAGE LOG FUNCTION CALLED");

  def_prog_mode();

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

  if (num_dir_entries)
    free(*log_files);

  if ((rc != 0) && (rc != 127))
    {
      s_status.num = rc;
      return 65; /* "Editor returned %d. Try setting the default editor" */
    }
  else
    return 1; /* return with no status */
}

int kernel_log(i_container *i_con)
{
  char cmnd[MAX_CMD_LENGTH];
  int rc, i;
  struct dirent **log_files;
  struct dirent **pp_dirent;
  int num_dir_entries;
  int len;

  mvaddstr(0,0,"KERNEL LOG FUNCTION CALLED");

  def_prog_mode();
  
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
  if (num_dir_entries)
    free(*log_files);

  if ((rc != 0) && (rc != 127))
    {
      s_status.num = rc;
      return 65; /* "Editor returned %d. Try setting the default editor" */
    }
  else
    return 1; /* return with no status */
}

int iprconfig_log(i_container *i_con)
{
  char cmnd[MAX_CMD_LENGTH];
  int rc, i;
  struct dirent **log_files;
  struct dirent **pp_dirent;
  int num_dir_entries;
  int len;

  mvaddstr(0,0,"IPRCONFIG LOG FUNCTION CALLED");
                        
  def_prog_mode();
                        
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
  if (num_dir_entries)
    free(*log_files);

  if ((rc != 0) && (rc != 127))
    {
      s_status.num = rc;
      return 65; /* "Editor returned %d. Try setting the default editor" */
    }
  else
    return 1; /* return with no status */
}

int kernel_root(i_container *i_con)
{
  int rc;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"KERNEL ROOT FUNCTION CALLED");

  i_con = free_i_con(i_con);
  i_con = add_i_con(i_con,"",NULL,list); /* i_con to return field data */
  
  output->next = add_fn_out(output->next,1,log_root_dir);
  
  s_out = screen_driver(&n_kernel_root,output,i_con);
  rc = s_out->rc;
  free(s_out);
  output = free_fn_out(output);
  return rc;
}

int confirm_kernel_root(i_container *i_con)
{
  int rc;
  DIR *p_dir;
  char *p_char;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIRM KERNEL ROOT FUNCTION CALLED");
  
  p_char = i_con->field_data;
  p_dir = opendir(p_char);

  if (p_dir == NULL)
    {
      output = free_fn_out(output);
      return 59; /* "Invalid directory" */
    }
  else
    {
      closedir(p_dir);
      if (strcmp(log_root_dir, p_char) == 0)
	{
	  output = free_fn_out(output);
	  return 61; /*"Root directory unchanged"*/
	}
    }

  output->next = add_fn_out(output->next,1,i_con->field_data);
  
  s_out = screen_driver(&n_confirm_kernel_root,output,i_con);
  rc = s_out->rc;
  
  if (rc == 0)
    {
      strcpy(log_root_dir, i_con->field_data);
      s_status.str = log_root_dir;
      rc = (EXIT_FLAG | 60); /*"Root directory changed to %s"*/
    }

  else
    rc = (EXIT_FLAG | 61); /*"Root directory unchanged"*/
  
  free(s_out);
  output = free_fn_out(output);
  return rc;
}

int set_default_editor(i_container *i_con)
{
  int rc = 0;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"SET DEFAULT EDITOR FUNCTION CALLED");
  
  i_con = free_i_con(i_con);
  i_con = add_i_con(i_con,"",NULL,list);
  
  output->next = add_fn_out(output->next,1,editor);
  
  s_out = screen_driver(&n_set_default_editor,output,i_con);
  rc = s_out->rc;
  free(s_out);
  output = free_fn_out(output);
  return rc;
}

int confirm_set_default_editor(i_container *i_con)
{
  int rc;
  char *p_char;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIRM SET DEFAULT EDITOR FUNCTION CALLED");

  p_char = i_con->field_data;
  if (strcmp(editor, p_char) == 0)
    {
      output = free_fn_out(output);
      return 63; /*"Editor unchanged"*/
    }

  output->next = add_fn_out(output->next,1,i_con->field_data);
  
  s_out = screen_driver(&n_confirm_set_default_editor,output,i_con);
  rc = s_out->rc;
  if (rc == 0)
    {
      strcpy(editor, i_con->field_data);
      s_status.str = editor;
      rc = (EXIT_FLAG | 62); /*"Editor changed to %s"*/
    }

  else
    rc = (EXIT_FLAG | 63); /*"Editor unchanged"*/
  
  free(s_out);
  output = free_fn_out(output);
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

  if ((rc != 0) && (rc != 127))
    {
      s_status.num = rc;
      return 65; /* "Editor returned %d. Try setting the default editor" */
    }
  else
    return 1; /* return with no status */
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

int driver_config(i_container *i_con)
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
    int rc;
    FORM *p_fields_form;
    FIELD *p_input_fields[6], *p_cur_field;
    ITEM **menu_item;
    int cur_field_index;
    int fd;
    struct ipr_driver_cfg driver_cfg;
    int  ch;
    int  i;
    int form_adjust = 0;
    char debug_level_str[4];
    int *userptr;
    int *retptr;
    struct text_str
    {
        char line[4];
    } *menu_str;
    int max_x,max_y,start_y,start_x;
    
    getmaxyx(stdscr,max_y,max_x);
    getbegyx(stdscr,start_y,start_x);
    
    rc = RC_SUCCESS;

    fd = open(p_head_ipr_ioa->ioa.dev_name, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_head_ipr_ioa->ioa.dev_name);
        return 58 | EXIT_FLAG; /* "Change Device Driver Configurations failed" */
    }

    rc = ioctl(fd, IPR_IOCTL_READ_DRIVER_CFG, &driver_cfg);
    close(fd);

    if (rc != 0)
    {
        syslog(LOG_ERR, "Read driver configuration failed. %m"IPR_EOL);
        return 58 | EXIT_FLAG; /* "Change Device Driver Configurations failed" */;
    }

    /* Title */
    p_input_fields[0] =
        new_field(1, max_x - start_x, 
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

    p_input_fields[5] = NULL;

    p_fields_form = new_form(p_input_fields);

    set_current_field(p_fields_form, p_input_fields[2]);

    form_opts_off(p_fields_form, O_BS_OVERLOAD);

    flush_stdscr();
    clear();

    set_form_win(p_fields_form,stdscr);
    set_form_sub(p_fields_form,stdscr);

    post_form(p_fields_form);

    mvaddstr(2, 1, "Current Driver configurations are shown.  To change");
    mvaddstr(3, 1, "setting hit \"c\" for options menu.  Highlight desired");
    mvaddstr(4, 1, "option then hit Enter");
    mvaddstr(6, 1, "c=Change Setting");
    mvaddstr(max_y - 3, 0, "Press Enter to Enable Changes");
    mvaddstr(max_y - 1, 0 , EXIT_KEY_LABEL CANCEL_KEY_LABEL);

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
	    if (rc == RC_EXIT)
                goto leave;
        }
        else if (IS_ENTER_KEY(ch))
        {
	  fd = open(p_head_ipr_ioa->ioa.dev_name, O_RDWR);
	  if (fd <= 1)
	    {
	      syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_head_ipr_ioa->ioa.dev_name);
	      return 58 | EXIT_FLAG; /* "Change Device Driver Configurations failed" */
	    }
	  
	  rc = ioctl(fd, IPR_IOCTL_WRITE_DRIVER_CFG, &driver_cfg);
	  close(fd);
	  
	    if (rc != 0)
	    {
		syslog(LOG_ERR, "Write driver configuration failed. %m"IPR_EOL);
		return 58 | EXIT_FLAG; /* "Change Device Driver Configurations failed" */
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
    
    if (rc == RC_SUCCESS)
      rc = 57 | EXIT_FLAG; /* "Change Device Driver Configurations completed successfully" */

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
  while(*p_tmp == ' ' || *p_tmp == '\n')
    p_tmp--;
  p_tmp++;
  *p_tmp = '\0';
  return p_str;
}

char *print_device(struct ipr_device *p_ipr_dev, char *body, char *option, struct ipr_ioa *p_cur_ioa)
{
    u16 len = 0;
    struct ipr_resource_entry *p_resource_entry = p_ipr_dev->p_resource_entry;
    int i, fd, rc;
    struct ipr_query_res_state res_state;
    u8 ioctl_buffer[255];
    int status;
    int format_req = 0;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    struct ipr_block_desc *p_block_desc;
    struct sense_data_t sense_data;
    u8 product_id[IPR_PROD_ID_LEN+1];
    u8 vendor_id[IPR_VENDOR_ID_LEN+1];
    char *dev_name = p_ipr_dev->dev_name;

    ipr_strncpy_0(vendor_id, p_resource_entry->std_inq_data.vpids.vendor_id, IPR_VENDOR_ID_LEN);
    ipr_strncpy_0(product_id, p_resource_entry->std_inq_data.vpids.product_id, IPR_PROD_ID_LEN);

    if (body)
        len = strlen(body);
    body = ipr_realloc(body, len + 100);

    len += sprintf(body + len, " %s  %-8s %-18s %-12s %s.%d/",
                   option,
                   vendor_id,
                   product_id,
                   dev_name,
                   p_cur_ioa->pci_address,
                   p_cur_ioa->host_no);

    if (p_resource_entry->is_ioa_resource)
    {
        len += sprintf(body + len,"            ");

        if (p_cur_ioa->ioa_dead)
            len += sprintf(body + len, "Not Operational\n");
        else if (p_cur_ioa->nr_ioa_microcode)
        { 
            len += sprintf(body + len, "Not Ready\n");
        } 
        else
        {
            len += sprintf(body + len, "Operational\n");
        }
    }
    else
    {
        int tab_stop  = sprintf(body + len,"%d:%d:%d ",
                                p_resource_entry->resource_address.bus,
                                p_resource_entry->resource_address.target,
                                p_resource_entry->resource_address.lun);

        len += tab_stop;

        for (i=0;i<12-tab_stop;i++)
            body[len+i] = ' ';

        len += 12-tab_stop;

        if (ipr_is_af(p_resource_entry))
        {
            fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);

            /* Do a query resource state */
            rc = ipr_query_resource_state(fd, p_resource_entry->resource_handle, &res_state);

            if (rc != 0)
            {
                syslog(LOG_ERR,"Query Resource State failed. %m"IPR_EOL);
                iprlog_location(p_cur_ioa);
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

            fd = open(p_ipr_dev->gen_name, O_RDONLY);

            if (fd <= 1)
            {
                if (strlen(p_ipr_dev->gen_name) != 0)
                    syslog(LOG_ERR, "Could not open file %s. %m"IPR_EOL, p_ipr_dev->gen_name);
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
                    syslog(LOG_ERR, "Could not send mode sense to %s. %m"IPR_EOL, p_ipr_dev->gen_name);
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

        if (res_state.not_oper)
            sprintf(body + len, "Not Operational\n");
        else if (res_state.not_ready ||
                 (res_state.read_write_prot && (iprtoh32(res_state.failing_dev_ioasc) == 0x02040200u)))
            sprintf(body + len, "Not ready\n");
        else if ((res_state.read_write_prot) || (is_rw_protected(p_ipr_dev)))
            sprintf(body + len, "R/W Protected\n");
        else if (res_state.prot_dev_failed)
            sprintf(body + len, "DPY/Failed\n");
        else if (res_state.prot_suspended)
            sprintf(body + len, "DPY/Unprotected\n");
        else if (res_state.prot_resuming)
            sprintf(body + len, "DPY/Rebuilding\n");
        else if (res_state.degraded_oper)
            sprintf(body + len, "Perf Degraded\n");
        else if (res_state.service_req)
            sprintf(body + len, "Redundant Hw Fail\n");
        else if (format_req)
            sprintf(body + len, "Format Required\n");
        else
        {
            if (p_resource_entry->is_array_member)
                sprintf(body + len, "DPY/Active\n");
            else if (p_resource_entry->is_hot_spare)
                sprintf(body + len, "HS/Active\n");
            else
                sprintf(body + len, "Operational\n");
        }
    }
    return body;
}

int is_format_allowed(struct ipr_device *p_ipr_dev)
{
  return 1;
}

int is_rw_protected(struct ipr_device *p_ipr_dev)
{
  return p_ipr_dev->p_resource_entry->is_compressed;
}

/*
  Print device into buffer for a selection list 
*/
u16 print_dev_pty(struct ipr_resource_entry *p_resource_entry, char *p_buf, char *dev_name)
{
  u16 len = 0;

  len += sprintf(p_buf + len, "%8s  ", "00000000"); /* FIXME */
  
  len += sprintf(p_buf + len, "%-17s", dev_name);
  len += sprintf(p_buf + len, "%02d     %02d", 0, 0); /* FIXME */

		 /*p_resource_entry->pci_bus_number,
		   p_resource_entry->pci_slot);*/
  
  if (ipr_is_af(p_resource_entry))
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

  return len;
}

/*
  Print device into buffer for a selection list 
*/
u16 print_dev_pty2(struct ipr_resource_entry *p_resource_entry, char *p_buf, char *dev_name)
{
  u16 len = 0;

  len += sprintf(p_buf + len, "%8s  ", "00000000");
  
  len += sprintf(p_buf + len, "%-16s", dev_name);
  len += sprintf(p_buf + len, "%02d     %02d", 0, 0);

		 /*p_resource_entry->pci_bus_number,
		   p_resource_entry->pci_slot);*/
  
  if (ipr_is_af(p_resource_entry))
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
	  model += 5;
	  break;
        }
    }
  return model;
}

void evaluate_device(struct ipr_device *p_ipr_device, struct ipr_ioa *p_ioa, int change_size)
{
    struct ipr_resource_entry *p_resource_entry;
    struct ipr_res_addr resource_address;
    struct ipr_resource_table resource_table;
    int j, rc, fd, device_converted;
    int timeout = 60;
    struct ipr_query_config_ioctl *query_ioctl;

    /* send evaluate device down */
    p_resource_entry = p_ipr_device->p_resource_entry;

    fd = open(p_ioa->ioa.dev_name, O_RDWR);
    if (fd < 0)
    {
        syslog(LOG_ERR, "Cannot open %s. %m\n", p_ioa->ioa.dev_name);
        return;
    }

    ipr_evaluate_device(fd, p_resource_entry->resource_handle);

    resource_address = p_resource_entry->resource_address;
    device_converted = 0;

    while (!device_converted && timeout)
    {
        /* issue query ioa config here */
        query_ioctl = calloc(1,sizeof(struct ipr_query_config_ioctl) + sizeof(struct ipr_resource_table));
        query_ioctl->buffer_len = sizeof(struct ipr_resource_table);

        rc = ioctl(fd, IPR_IOCTL_QUERY_CONFIGURATION, query_ioctl);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Query IOA Config to %s failed. %m"IPR_EOL, p_ioa->ioa.dev_name);
            close(fd);
            return;
        }

        else 
        {
            memcpy(&resource_table,&query_ioctl->buffer[0], sizeof(struct ipr_resource_table));
        }

        free(query_ioctl);

        for (j = 0; j <resource_table.hdr.num_entries; j++)
        {
            p_resource_entry = &resource_table.dev[j];
            if ((memcmp(&resource_address,
                        &p_resource_entry->resource_address,
                        sizeof(resource_address)) == 0) &&
                (((ipr_is_hidden(p_resource_entry)) &&
                  (change_size == IPR_522_BLOCK)) ||
                 ((!ipr_is_hidden(p_resource_entry)) &&
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

i_container *free_i_con(i_container *x_con)
{
  i_container *temp_i_con;

  if (x_con == NULL)
    return x_con;

  temp_i_con = x_con->next_item;

  while (temp_i_con != NULL)
    {
      free(x_con);
      x_con = temp_i_con;
      temp_i_con = x_con->next_item;
    }
  free(x_con);
  x_con = NULL;
  return x_con;
}

fn_out *free_fn_out(fn_out *out)
{
  fn_out *temp_fn_out;
  
  if (out == NULL)
    return out;
  
  temp_fn_out = out->next;
  
  while (temp_fn_out != NULL)
    {
      if (out->text)
	free(out->text);
      free(out);
      out = temp_fn_out;
      temp_fn_out = out->next;
    }
  
  if (out->text)
    free(out->text);
  free(out);
  out = NULL;
  return out;
}

i_container *add_i_con(i_container *x_con,char *f,void *d,i_type t)
{  
  i_container *new_i_con;
  new_i_con = malloc(sizeof(i_container));
  
  new_i_con->next_item = x_con;  /* a reference to the next i_con item */
  strncpy(new_i_con->field_data,f,MAX_FIELD_SIZE+1);     /* used to hold data entered into user-entry fields */
  new_i_con->data = d;           /* a pointer to the device information represented by the i_con */
  new_i_con->type = t;           /* the type of container it is: list or menu */
  new_i_con->field_data[strlen(f)+1] = '\0';
  
  return new_i_con;
}

fn_out *init_output()
{
  fn_out *output = malloc(sizeof(fn_out));

  output->next = NULL;
  output->index = 0;
  output->cat_offset = 0;
  output->text = NULL;
  
  return output;
}

fn_out *add_fn_out(fn_out *output,int i,char *text)
{
  fn_out *new_fn_out = malloc(sizeof(fn_out));
  new_fn_out->next = output;
  new_fn_out->index = i;
  new_fn_out->text = malloc((strlen(text)+1));
  sprintf(new_fn_out->text,"%s",text);

  return new_fn_out;
}

i_container *copy_i_con(i_container *x_con)
{
  i_container *temp_i_con = x_con;
  i_container *new_i_con = NULL;

  while (temp_i_con != NULL)
    {
      new_i_con = add_i_con(new_i_con,temp_i_con->field_data,temp_i_con->data,temp_i_con->type);
      temp_i_con = temp_i_con->next_item;
    }

    return new_i_con;
}
