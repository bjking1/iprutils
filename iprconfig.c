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
  u32                          array_num;
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

struct devs_to_init_t *dev_init_head;
struct devs_to_init_t *dev_init_tail;
struct array_cmd_data *raid_cmd_head;
struct array_cmd_data *raid_cmd_tail;
static char            dev_field[] = "dev";
char                   log_root_dir[200];
char                   editor[200];
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

i_container *free_i_con(i_container *x_con);
fn_out      *free_fn_out(fn_out *out);
i_container *add_i_con(i_container *x_con,
                       char *f,
                       void *d,i_type t);
fn_out      *init_output();
fn_out      *add_fn_out(fn_out *output,int i,
                        char *text);
i_container *copy_i_con(i_container *x_con);

struct special_status {
    int   index;
    int   num;
    char *str;
} s_status;

struct screen_output *screen_driver(s_node *screen,
                                    fn_out *output,
                                    i_container *i_con);
char *strip_trailing_whitespace(char *str);
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

int main(int argc, char *argv[])
{
    int  next_editor, next_dir, next_platform, i;
    char parm_editor[200], parm_dir[200];

    ipr_ioa_head = ipr_ioa_tail = NULL;

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
        next_platform = 0;
        for (i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "-e") == 0)
                next_editor = 1;
            else if (strcmp(argv[i], "-k") == 0)
                next_dir = 1;
            else if (strcmp(argv[i], "--version") == 0)
            {
                printf("iprconfig: %s\n", IPR_VERSION_STR);
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
                printf("Usage: iprconfig [options]\n");
                printf("  Options: -e name    Default editor for viewing error logs\n");
                printf("           -k dir     Kernel messages root directory\n");
                printf("           --version  Print iprconfig version\n");
                printf("  Use quotes around parameters with spaces\n");
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

    WINDOW *w_pad,*w_page_header;              /* windows to hold text */
    FIELD **fields = NULL;                     /* field list for forms */
    FORM *form = NULL;                       /* form for input fields */

    fn_out *output_temp;                       /* help store function input */
    int rc = 0;                            /* the status of the screen */
    char *status;                            /* displays screen operations */
    char buffer[100];                       /* needed for special status strings */
    bool invalid = false;                   /* invalid status display */

    bool t_on=false,e_on=false,q_on=false,r_on=false,f_on=false,enter_on=false,menus_on=false; /* control user input handling */
    bool ground_cursor=false;

    int stdscr_max_y,stdscr_max_x;
    int w_pad_max_y=0,w_pad_max_x=0;       /* limits of the windows */
    int pad_l=0,pad_r,pad_t=0,pad_b;
    int pad_scr_t=2,pad_scr_b=2,pad_scr_l=0; /* position of the pad */
    int center,len=0,ch;
    int i=0,j,k,row=0,col=0,bt_len=0;      /* text positioning */
    int field_width,num_fields=0;          /* form positioning */
    int h,w,t,l,o,b;                       /* form querying */

    int active_field = 0;                  /* return to same active field after a quit */
    bool pages = false,is_bottom = false;   /* control page up/down in multi-page screens */
    bool form_adjust = false;               /* correct cursor position in multi-page screens */
    int str_index,max_str_index=0;         /* text positioning in multi-page screens */
    bool refresh_stdscr = true;
    bool x_offscr,y_offscr;                 /* scrolling windows */

    struct screen_output *s_out;
    struct screen_opts   *temp;                 /* reference to another screen */

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
    body_text = calloc(bt_len + 1, sizeof(char));

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
            fields = ipr_realloc(fields,sizeof(FIELD **) * (num_fields + 1));
            i++;

            if (body[i] == 'm')
            {
                menus_on = true;
                field_width = 15;
                fields[num_fields] = new_field(1,field_width,row,col,0,0);
                field_opts_off(fields[num_fields], O_EDIT);
            }
            else
            {
                field_width = 0;
                sscanf(&(body[i]),"%d",&field_width);

                fields[num_fields] = new_field(1,field_width,row,col,0,0);

                if (field_width >= 30)
                    field_opts_off(fields[num_fields],O_AUTOSKIP);

                if (field_width > 9)
                    i++; /* skip second digit of field index */
            }
            set_field_userptr(fields[num_fields],NULL);
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
                            fields = ipr_realloc(fields,sizeof(FIELD **) * (num_fields + 1));
                            j++; 

                            if (output_temp->text[j] == 'm')
                            {
                                menus_on = true;
                                field_width = 15;
                                fields[num_fields] = new_field(1,field_width,row,col,0,0);
                                field_opts_off(fields[num_fields], O_EDIT);
                            }
                            else
                            {
                                field_width = 0;
                                sscanf(&(output_temp->text[j]),"%d",&field_width);
                                fields[num_fields] = new_field(1,field_width,row,col,0,0);

                                if (field_width >= 30)
                                    field_opts_off(fields[num_fields],O_AUTOSKIP);

                                if (field_width > 9)
                                    i++; /* skip second digit of field width */
                            }
                            set_field_userptr(fields[num_fields],NULL);
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
        fields = ipr_realloc(fields,sizeof(FIELD **) * (num_fields + 1));
        fields[num_fields] = NULL;
        form = new_form(fields);
    }

    /* make the form appear in the pad; not stdscr */
    set_form_win(form,w_pad);
    set_form_sub(form,w_pad);

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
                case 't':
                    t_on = true; /* toggle page data */
                    active_field = toggle_field;
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
                field_opts_on(fields[i],O_ACTIVE);
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
        if (t_on)
            addstr(TOGGLE_KEY_LABEL);
        if (f_on && y_offscr)
        {
            addstr(PAGE_DOWN_KEY_LABEL);
            addstr(PAGE_UP_KEY_LABEL);
        }

        post_form(form);

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
            set_current_field(form,fields[active_field]);
            if (active_field == 0)
                form_driver(form,REQ_NEXT_FIELD);
            form_driver(form,REQ_PREV_FIELD);
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
                status = catgets(catd,STATUS_SET,
                                 INVALID_OPTION_STATUS,"Status Return Error (invalid)");
                invalid = false;
            }
            else if (rc != 0)
            {
                if ((status = strchr(catgets(catd, STATUS_SET,
                                             rc,"default"),'%')) == NULL)
                {
                    status = catgets(catd, STATUS_SET,
                                     rc,"Status Return Error (rc != 0, w/out %)");
                }
                else if (status[1] == 'd')
                {
                    sprintf(buffer,catgets(catd,STATUS_SET,
                                           rc,"Status Return Error (rc != 0 w/ %%d = %d"),s_status.num);
                    status = buffer;
                }
                else if (status[1] == 's')
                {
                    sprintf(buffer,catgets(catd,STATUS_SET,
                                           rc,"Status Return Error rc != 0 w/ %%s = %s"),s_status.str);
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
                    if (fields[i] != NULL)
                    {
                        field_info(fields[i],&h,&w,&t,&l,&o,&b); /* height, width, top, left, offscreen rows, buffer */

                        /* turn all offscreen fields off */
                        if (t>=header_lines+pad_t && t<=pad_b+pad_t)
                        {
                            field_opts_on(fields[i],O_ACTIVE);
                            if (form_adjust)
                            {
                                if (!t_on || !toggle_field)
                                    set_current_field(form,fields[i]);
                                form_adjust = false;
                            }
                        }
                        else
                            field_opts_off(fields[i],O_ACTIVE);
                    }
                }
            }  

            if (t_on && toggle_field &&
                ((field_opts(fields[toggle_field]) & O_ACTIVE) != O_ACTIVE)) {

               ch = KEY_NPAGE;
            } else {

                toggle_field = 0;

                if (refresh_stdscr)
                {
                    touchwin(stdscr);
                    refresh_stdscr = FALSE;
                }
                else
                    touchline(stdscr,stdscr_max_y - 1,1);

                if (ground_cursor)
                    move(0,0);

                refresh();

                if (pages)
                {
                    touchwin(w_page_header);
                    prefresh(w_page_header, 0, pad_l,
                             pad_scr_t, pad_scr_l,
                             header_lines + 1 ,pad_r - 1);
                    touchwin(w_pad);
                    prefresh(w_pad, pad_t + header_lines,
                             pad_l, pad_scr_t + header_lines,
                             pad_scr_l, pad_b + pad_scr_t, pad_r - 1);
                }
                else
                {
                    touchwin(w_pad);
                    prefresh(w_pad, pad_t, pad_l,
                             pad_scr_t, pad_scr_l,
                             pad_b + pad_scr_t, pad_r - 1);
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
                char *input;

                if (num_fields > 1)
                    active_field = field_index(current_field(form));

                if (enter_on && num_fields == 0) {

                    rc = (CANCEL_FLAG | rc);
                    goto leave;
                } else if (enter_on && !menus_on) {

                    /* cancel if all fields are empty */
                    form_driver(form,REQ_VALIDATION);
                    for (i = 0; i < num_fields; i++) {

                        if (strlen(strip_trailing_whitespace(field_buffer(fields[i],0))) != 0)
                            /* fields are not empty -> continue */
                            break;

                        if (i == num_fields - 1) {
                            /* all fields are empty */
                            rc = (CANCEL_FLAG | rc);
                            goto leave;
                        }
                    }
                }

                if (num_fields > 0) {

                    /* input field is always fields[0] if only 1 field */
                    input = field_buffer(fields[0],0);
                }

                invalid = true;

                for (i = 0; i < screen->num_opts; i++) {

                    temp = &(screen->options[i]);

                    if ((temp->key == "\n") ||
                        ((num_fields > 0)?(strcasecmp(input,temp->key) == 0):0)) {

                        invalid = false;

                        if (temp->key == "\n" && num_fields > 0) {

                            /* store field data to existing i_con (which should already
                             contain pointers) */
                            i_container *temp_i_con = i_con;
                            form_driver(form,REQ_VALIDATION);

                            for (i = num_fields - 1; i >= 0; i--) {

                                strncpy(temp_i_con->field_data,field_buffer(fields[i],0),MAX_FIELD_SIZE);
                                temp_i_con = temp_i_con->next_item;
                            }
                        }

                        if (temp->screen_function == NULL)
                            /* continue with function */
                            goto leave; 

                        do 
                            rc = temp->screen_function(i_con);
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

                if (!invalid) 
                {
                    /*clear fields*/
                    form_driver(form,REQ_LAST_FIELD);
                    for (i=0;i<num_fields;i++)
                    {
                        form_driver(form,REQ_CLR_FIELD);
                        form_driver(form,REQ_NEXT_FIELD);
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
            else if (IS_TOGGLE_KEY(ch) && t_on)
            {
                rc = TOGGLE_SCREEN;
                if (num_fields>1)
                    toggle_field = field_index(current_field(form)) + 1;
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
                    form_driver(form, REQ_NEXT_CHAR);
            }
            else if (ch == KEY_LEFT)
            {
                if (pad_l>0)
                    pad_l--;
                else if (!menus_on)
                    form_driver(form, REQ_PREV_CHAR);
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
                    form_driver(form, REQ_PREV_FIELD);
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
                    form_driver(form, REQ_NEXT_FIELD);
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
            else if (isascii(ch))
                form_driver(form, ch);
        }
    }

    leave:
        s_out->i_con = i_con;
    s_out->rc = rc;
    ipr_free(body_text);
    unpost_form(form);
    free_form(form);

    for (i=0;i<num_fields;i++)
    {
        if (fields[i] != NULL)
            free_field(fields[i]);
    }
    delwin(w_pad);
    delwin(w_page_header);
    return s_out;
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
    char init_message[60];
    int k, j;
    struct ipr_cmd_status_record *status_record;
    int num_devs = 0;
    int num_starts = 0;
    struct ipr_ioa *cur_ioa;
    struct devs_to_init_t *cur_dev_init;
    struct array_cmd_data *cur_raid_cmd;
    u8 ioctl_buffer[IPR_MODE_SENSE_LENGTH];
    struct sense_data_t sense_data;
    struct scsi_dev_data *scsi_dev_data;
    struct ipr_mode_parm_hdr *mode_parm_hdr;
    struct ipr_block_desc *block_desc;
    int loop, retries;
    struct screen_output         *s_out;
    char *string_buf;
    mvaddstr(0,0,"MAIN MENU FUNCTION CALLED");

    init_message[0] = '\0';

    check_current_config(false);

    for(cur_ioa = ipr_ioa_head;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next)
    {
        cur_ioa->num_raid_cmds = 0;

        /* Do a query command status to see if any long running array commands
         are currently running */
        rc = ipr_query_command_status(cur_ioa->ioa.gen_name,
                                      &cmd_status);

        if (rc != 0)
            continue;

        status_record = &cmd_status.record[0];

        for (k = 0; k < cmd_status.num_records; k++, status_record++)
        {
            if ((status_record->status & IPR_CMD_STATUS_IN_PROGRESS))
            {
                if (status_record->command_code == IPR_START_ARRAY_PROTECTION)
                {
                    cur_ioa->num_raid_cmds++;
                    num_starts++;

                    if (raid_cmd_head)
                    {
                        raid_cmd_tail->next = (struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
                        raid_cmd_tail = raid_cmd_tail->next;
                    }
                    else
                    {
                        raid_cmd_head = raid_cmd_tail = (struct array_cmd_data *)ipr_malloc(sizeof(struct array_cmd_data));
                    }

                    raid_cmd_tail->next = NULL;

                    memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

                    raid_cmd_tail->array_id = status_record->array_id;
                    raid_cmd_tail->ipr_ioa = cur_ioa;
                    raid_cmd_tail->do_cmd = 1;
                }
            }
        }

        /* Do a query command status for all devices to check for formats in
         progress */
        for (j = 0; j < cur_ioa->num_devices; j++)
        {
            scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;

            if ((scsi_dev_data == NULL) ||
                (scsi_dev_data->type == IPR_TYPE_ADAPTER))
                continue;

            if (ipr_is_af(&cur_ioa->dev[j]))
            {
                /* Send Test Unit Ready to start device if its a volume set */
                if (ipr_is_volume_set(&cur_ioa->dev[j])) {
                    ipr_test_unit_ready(cur_ioa->dev[j].gen_name, &sense_data);
                    continue;
                }

                /* Do a query command status */
                rc = ipr_query_command_status(cur_ioa->dev[j].gen_name, &cmd_status);

                if (rc != 0)
                    continue;

                if (cmd_status.num_records == 0)
                {
                    /* Do nothing */
                }
                else
                {
                    if (cmd_status.record->status == IPR_CMD_STATUS_IN_PROGRESS)
                    {
                        if (dev_init_head)
                        {
                            dev_init_tail->next = (struct devs_to_init_t *)ipr_malloc(sizeof(struct devs_to_init_t));
                            dev_init_tail = dev_init_tail->next;
                        }
                        else
                            dev_init_head = dev_init_tail = (struct devs_to_init_t *)ipr_malloc(sizeof(struct devs_to_init_t));

                        memset(dev_init_tail, 0, sizeof(struct devs_to_init_t));

                        dev_init_tail->ioa = cur_ioa;
                        dev_init_tail->dev_type = IPR_AF_DASD_DEVICE;
                        dev_init_tail->ipr_dev = &cur_ioa->dev[j];
                        dev_init_tail->do_init = 1;

                        num_devs++;
                    }
                    else if (cmd_status.record->status == IPR_CMD_STATUS_SUCCESSFUL)
                    {
                        /* Check if evaluate device capabilities is necessary */
                        /* Issue mode sense */
                        rc = ipr_mode_sense(cur_ioa->dev[j].gen_name, 0, ioctl_buffer);

                        if (rc == 0) {
                            mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

                            if (mode_parm_hdr->block_desc_len > 0)
                            {
                                block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

                                if((block_desc->block_length[1] == 0x02) && 
                                   (block_desc->block_length[2] == 0x00))
                                {
                                    /* Send evaluate device capabilities */
                                    remove_device(cur_ioa->dev[j].scsi_dev_data);
                                    evaluate_device(&cur_ioa->dev[j], cur_ioa, IPR_512_BLOCK);
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
                    rc = ipr_test_unit_ready(cur_ioa->dev[j].gen_name, &sense_data);

                if ((rc == CHECK_CONDITION) &&
                    ((sense_data.error_code & 0x7F) == 0x70) &&
                    ((sense_data.sense_key & 0x0F) == 0x02) &&  /* NOT READY */
                    (sense_data.add_sense_code == 0x04) &&      /* LOGICAL UNIT NOT READY */
                    (sense_data.add_sense_code_qual == 0x02))   /* INIT CMD REQ */
                {
                    rc = ipr_start_stop_start(cur_ioa->dev[j].gen_name);

                    if (retries == 0) {
                        retries++;
                        goto redo_tur;
                    }
                }
                else if ((rc == CHECK_CONDITION) &&
                         ((sense_data.error_code & 0x7F) == 0x70) &&
                         ((sense_data.sense_key & 0x0F) == 0x02) &&  /* NOT READY */
                         (sense_data.add_sense_code == 0x04) &&      /* LOGICAL UNIT NOT READY */
                         (sense_data.add_sense_code_qual == 0x04))   /* FORMAT IN PROGRESS */
                {
                    if (dev_init_head)
                    {
                        dev_init_tail->next = (struct devs_to_init_t *)ipr_malloc(sizeof(struct devs_to_init_t));
                        dev_init_tail = dev_init_tail->next;
                    }
                    else
                        dev_init_head = dev_init_tail = (struct devs_to_init_t *)ipr_malloc(sizeof(struct devs_to_init_t));

                    memset(dev_init_tail, 0, sizeof(struct devs_to_init_t));

                    dev_init_tail->ioa = cur_ioa;
                    dev_init_tail->dev_type = IPR_JBOD_DASD_DEVICE;
                    dev_init_tail->ipr_dev = &cur_ioa->dev[j];

                    dev_init_tail->do_init = 1;

                    num_devs++;
                }
                else if (rc != CHECK_CONDITION)
                {
                    /* Check if evaluate device capabilities needs to be issued */

                    /* Issue mode sense to get the block size */
                    rc = ipr_mode_sense(cur_ioa->dev[j].gen_name,
                                        0x0a, &ioctl_buffer);

                    if (rc == 0) {
                        mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

                        if (mode_parm_hdr->block_desc_len > 0)
                        {
                            block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

                            if ((!(block_desc->block_length[1] == 0x02) ||
                                 !(block_desc->block_length[2] == 0x00)) &&
                                (cur_ioa->qac_data->num_records != 0))
                            {
                                /* send evaluate device */
                                evaluate_device(&cur_ioa->dev[j], cur_ioa, IPR_522_BLOCK);
                            }
                        }
                    }
                }
            }
        }
    }

    cur_dev_init = dev_init_head;
    while(cur_dev_init)
    {
        cur_dev_init = cur_dev_init->next;
        ipr_free(dev_init_head);
        dev_init_head = cur_dev_init;
    }

    cur_raid_cmd = raid_cmd_head;
    while(cur_raid_cmd)
    {
        cur_raid_cmd = cur_raid_cmd->next;
        ipr_free(raid_cmd_head);
        raid_cmd_head = cur_raid_cmd;
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

    s_out = screen_driver_new(&n_main_menu,NULL,0,NULL);
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

char *disk_status_body_init(int *num_lines, int type)
{
    char *string_buf;
    char *buffer = NULL;
    int cur_len;
    int header_lines = 0;

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
    struct ipr_query_res_state res_state;
    mvaddstr(0,0,"DISK STATUS FUNCTION CALLED");

    rc = RC_SUCCESS;
    i_con = free_i_con(i_con);

    check_current_config(false);
   
    /* Setup screen title */
    string_buf = catgets(catd,n_disk_status.text_set,1,"Display Disk Hardware Status");
    n_disk_status.title = ipr_malloc(strlen(string_buf) + 4);
    sprintf(n_disk_status.title, string_buf);

    for (k=0; k<2; k++)
        buffer[k] = disk_status_body_init(&header_lines, k);

    for(cur_ioa = ipr_ioa_head;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next)
    {
        if (cur_ioa->ioa.scsi_dev_data == NULL)
            continue;

        for (k=0; k<2; k++)
            buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"%1", cur_ioa, k);

        i_con = add_i_con(i_con,"\0",(char *)&cur_ioa->ioa,list);

        num_lines++;

        /* print JBOD and non-member AF devices*/
        for (j = 0; j < cur_ioa->num_devices; j++)
        {
            scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
            if ((scsi_dev_data == NULL) ||
                (scsi_dev_data->type == IPR_TYPE_ADAPTER) ||
                (ipr_is_hot_spare(&cur_ioa->dev[j])) ||
                (ipr_is_volume_set(&cur_ioa->dev[j])) ||
                (ipr_is_array_member(&cur_ioa->dev[j])))

                continue;

            for (k=0; k<2; k++)
                buffer[k] = print_device(&cur_ioa->dev[j],buffer[k], "%1", cur_ioa, k);

            i_con = add_i_con(i_con,"\0", (char *)&cur_ioa->dev[j], list);  

            num_lines++;
        }

        /* print Hot Spare devices*/
        for (j = 0; j < cur_ioa->num_devices; j++)
        {
            scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
            if ((scsi_dev_data == NULL ) ||
                (!ipr_is_hot_spare(&cur_ioa->dev[j])))

                continue;

            for (k=0; k<2; k++)
                buffer[k] = print_device(&cur_ioa->dev[j],buffer[k], "%1", cur_ioa, k);

            i_con = add_i_con(i_con,"\0", (char *)&cur_ioa->dev[j], list);  

            num_lines++;
        }

        /* print volume set and associated devices*/
        for (j = 0; j < cur_ioa->num_devices; j++)
        {
            if (!ipr_is_volume_set(&cur_ioa->dev[j]))
                continue;

            array_record = (struct ipr_array_record *)
                cur_ioa->dev[j].qac_entry;

            if (array_record->start_cand)
                continue;

            /* query resource state to acquire protection level string */
            rc = ipr_query_resource_state(cur_ioa->dev[j].gen_name,
                                          &res_state);
            strncpy(cur_ioa->dev[j].prot_level_str,
                    res_state.vset.prot_level_str, 8);

            for (k=0; k<2; k++)
                buffer[k] = print_device(&cur_ioa->dev[j],buffer[k], "%1", cur_ioa, k);

            i_con = add_i_con(i_con,"\0",
                              (char *)&cur_ioa->dev[j],
                              list);

            dev_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
            array_id = dev_record->array_id;
            num_lines++;

            for (i = 0; i < cur_ioa->num_devices; i++)
            {
                dev_record = (struct ipr_dev_record *)cur_ioa->dev[i].qac_entry;

                if (!(ipr_is_array_member(&cur_ioa->dev[i])) ||
                     (ipr_is_volume_set(&cur_ioa->dev[i])) ||
                     ((dev_record != NULL) &&
                      (dev_record->array_id != array_id)))
                {
                    continue;
                }

                strncpy(cur_ioa->dev[i].prot_level_str,
                        res_state.vset.prot_level_str, 8);

                for (k=0; k<2; k++)
                    buffer[k] = print_device(&cur_ioa->dev[i],buffer[k], "%1", cur_ioa, k);

                i_con = add_i_con(i_con,"\0", (char *)&cur_ioa->dev[i], list);
                num_lines++;
            }
        }
    }


    if (num_lines == 0)
    {
       string_buf = catgets(catd,n_disk_status.text_set,4,
                            "No devices found");
       len = strlen(buffer[i]);
       buffer[i] = ipr_realloc(buffer[i], len +
                                    strlen(string_buf) + 4);
       sprintf(buffer[i] + len, "\n%s", string_buf);
    }

    toggle_field = 0;

    do {
        n_disk_status.body = buffer[toggle&1];
        s_out = screen_driver_new(&n_disk_status,NULL,header_lines,i_con);
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
    n_device_details.body = body;
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

    for (temp_i_con = i_con;
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
        (scsi_dev_data->type == IPR_TYPE_ADAPTER))
    {
        string_buf = catgets(catd,n_device_details.text_set,1,"Title");

        n_device_details.title = ipr_malloc(strlen(string_buf) + 4);
        sprintf(n_device_details.title, string_buf);

        memset(&ioa_vpd, 0, sizeof(ioa_vpd));
        memset(&cfc_vpd, 0, sizeof(cfc_vpd));
        memset(&dram_vpd, 0, sizeof(dram_vpd));
        memset(&page3_inq, 0, sizeof(page3_inq));

        ipr_inquiry(device->gen_name, IPR_STD_INQUIRY,
                    &ioa_vpd, sizeof(ioa_vpd));
        ipr_inquiry(device->gen_name, 1,
                    &cfc_vpd, sizeof(cfc_vpd));
        ipr_inquiry(device->gen_name, 2,
                    &dram_vpd, sizeof(dram_vpd));
        ipr_inquiry(device->gen_name, 3,
                    &page3_inq, sizeof(page3_inq));

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

        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(2, vendor_id);
        device_details_body(3, product_id);
        device_details_body(4, device->ioa->driver_version);
        device_details_body(5, firmware_version);
        device_details_body(6, serial_num);
        device_details_body(7, part_num);
        device_details_body(8, plant_code);
        device_details_body(9, buffer);
        device_details_body(10, dram_size);
        device_details_body(11, device->gen_name);
        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(12, NULL);
        device_details_body(13, device->ioa->pci_address);
        sprintf(buffer,"%d", device->ioa->host_num);
        device_details_body(14, buffer);
    } else if ((array_record) &&
               (array_record->common.record_id ==
                IPR_RECORD_ID_ARRAY_RECORD)) {

        string_buf = catgets(catd,n_device_details.text_set,101,"Title");

        n_device_details.title = ipr_malloc(strlen(string_buf) + 4);
        sprintf(n_device_details.title, string_buf);

        ipr_strncpy_0(vendor_id,
                      array_record->vendor_id,
                      IPR_VENDOR_ID_LEN);

        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(2, vendor_id);

        sprintf(buffer,"RAID %s", device->prot_level_str);
        device_details_body(102, buffer);

        if (ntohs(array_record->stripe_size) > 1024)
            sprintf(buffer,"%d M",
                    ntohs(array_record->stripe_size)/1024);
        else
            sprintf(buffer,"%d k",
                    ntohs(array_record->stripe_size));

        device_details_body(103, buffer);
        
        /* Do a read capacity to determine the capacity */
        rc = ipr_read_capacity_16(device->gen_name,
                                  &read_cap16);

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
            device_details_body(104, buffer);
        }
        device_details_body(11, device->dev_name);

        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(12, NULL);
        device_details_body(13, device->ioa->pci_address);

        sprintf(buffer,"%d", device->ioa->host_num);
        device_details_body(203, buffer);

        sprintf(buffer,"%d",scsi_channel);
        device_details_body(204, buffer);

        sprintf(buffer,"%d",scsi_id);
        device_details_body(205, buffer);

        sprintf(buffer,"%d",scsi_lun);
        device_details_body(206, buffer);
    } else {
        string_buf = catgets(catd,n_device_details.text_set,201,"Title");

        n_device_details.title = ipr_malloc(strlen(string_buf) + 4);
        sprintf(n_device_details.title, string_buf);

        memset(&read_cap, 0, sizeof(read_cap));
        rc = ipr_read_capacity(device->gen_name, &read_cap);

        rc = ipr_inquiry(device->gen_name,
                         0x03, &dasd_page3_inq, sizeof(dasd_page3_inq));
        rc = ipr_inquiry(device->gen_name,
                         IPR_STD_INQUIRY, &std_inq, sizeof(std_inq));

        ipr_strncpy_0(vendor_id,
                      std_inq.std_inq_data.vpids.vendor_id,
                      IPR_VENDOR_ID_LEN);
        ipr_strncpy_0(product_id,
                      std_inq.std_inq_data.vpids.product_id,
                      IPR_PROD_ID_LEN);

        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(2, vendor_id);
        device_details_body(202, product_id);

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
            
        device_details_body(5, buffer);

        if (strcmp(vendor_id,"IBMAS400") == 0) {
            /* The service level on IBMAS400 dasd is located
             at byte 107 in the STD Inq data */
            hex = (char *)&std_inq;
            sprintf(buffer, "%X",
                    hex[IPR_STD_INQ_AS400_SERV_LVL_OFF]);
            device_details_body(207, buffer);
        }

        ipr_strncpy_0(serial_num,
                      std_inq.std_inq_data.serial_num,
                      IPR_SERIAL_NUM_LEN);
        device_details_body(6, serial_num);

        if (ntohl(read_cap.block_length) &&
            ntohl(read_cap.max_user_lba))  {

            lba_divisor = (1000*1000*1000) /
                ntohl(read_cap.block_length);

            device_capacity = ntohl(read_cap.max_user_lba) + 1;
            sprintf(buffer,"%.2Lf GB",
                    device_capacity/lba_divisor);

            device_details_body(104, buffer);
        }

        if (strlen(device->dev_name) > 0)
        {
            device_details_body(11, device->dev_name);
        }

        device_details_body(IPR_LINE_FEED, NULL);
        device_details_body(12, NULL);
        device_details_body(13, device->ioa->pci_address);

        sprintf(buffer,"%d", device->ioa->host_num);
        device_details_body(203, buffer);

        sprintf(buffer,"%d",scsi_channel);
        device_details_body(204, buffer);

        sprintf(buffer,"%d",scsi_id);
        device_details_body(205, buffer);

        sprintf(buffer,"%d",scsi_lun);
        device_details_body(206, buffer);

        rc =  ipr_inquiry(device->gen_name,
                          0, &page0_inq, sizeof(page0_inq));

        for (i = 0; (i < page0_inq.page_length) && !rc; i++)
        {
            if (page0_inq.supported_page_codes[i] == 0xC7)  //FIXME 0xC7 is SCSD, do further research to find what needs to be tested for this affirmation.
            {
                device_details_body(IPR_LINE_FEED, NULL);
                device_details_body(208, NULL);

                ipr_strncpy_0(buffer,
                              std_inq.fru_number,
                              IPR_STD_INQ_FRU_NUM_LEN);
                device_details_body(209, buffer);

                ipr_strncpy_0(buffer,
                              std_inq.ec_level,
                              IPR_STD_INQ_EC_LEVEL_LEN);
                device_details_body(210, buffer);

                ipr_strncpy_0(buffer,
                              std_inq.part_number,
                              IPR_STD_INQ_PART_NUM_LEN);
                device_details_body(7, buffer);

                hex = (u8 *)&std_inq.std_inq_data;
                sprintf(buffer, "%02X%02X%02X%02X%02X%02X%02X%02X",
                        hex[0], hex[1], hex[2],
                        hex[3], hex[4], hex[5],
                        hex[6], hex[7]);
                device_details_body(211, buffer);

                ipr_strncpy_0(buffer,
                              std_inq.z1_term,
                              IPR_STD_INQ_Z1_TERM_LEN);
                device_details_body(212, buffer);

                ipr_strncpy_0(buffer,
                              std_inq.z2_term,
                              IPR_STD_INQ_Z2_TERM_LEN);
                device_details_body(213, buffer);

                ipr_strncpy_0(buffer,
                              std_inq.z3_term,
                              IPR_STD_INQ_Z3_TERM_LEN);
                device_details_body(214, buffer);

                ipr_strncpy_0(buffer,
                              std_inq.z4_term,
                              IPR_STD_INQ_Z4_TERM_LEN);
                device_details_body(215, buffer);

                ipr_strncpy_0(buffer,
                              std_inq.z5_term,
                              IPR_STD_INQ_Z5_TERM_LEN);
                device_details_body(216, buffer);

                ipr_strncpy_0(buffer,
                              std_inq.z6_term,
                              IPR_STD_INQ_Z6_TERM_LEN);
                device_details_body(217, buffer);
                break;
            }
        }
    }
   
    s_out = screen_driver_new(&n_device_details,NULL,0,i_con);
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
    fn_out *output = init_output();
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

char *raid_status_body_init(int *num_lines, int type)
{
    char *string_buf;
    char *buffer = NULL;
    int cur_len;
    int header_lines = 0;

    string_buf = catgets(catd,n_raid_status.text_set, 2,
                         "Type option, press Enter.");
    buffer = ipr_realloc(buffer, strlen(string_buf) + 4);
    cur_len = sprintf(buffer, "\n%s\n", string_buf);
    header_lines += 2;

    string_buf = catgets(catd,n_raid_status.text_set, 3,
                         "Display hardware resource information details");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
    cur_len += sprintf(buffer + cur_len, "  5=%s\n\n", string_buf);
    header_lines += 2;

    buffer = status_header(buffer, &header_lines, type);

    *num_lines = header_lines;
    return buffer;
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
    struct ipr_query_res_state res_state;
    mvaddstr(0,0,"DISK STATUS FUNCTION CALLED");

    rc = RC_SUCCESS;
    i_con = free_i_con(i_con);

    check_current_config(false);
   
    /* Setup screen title */
    string_buf = catgets(catd,n_raid_status.text_set,1,"Display Disk Array Status");
    n_raid_status.title = ipr_malloc(strlen(string_buf) + 4);
    sprintf(n_raid_status.title, string_buf);

    for (k=0; k<2; k++)
        buffer[k] = raid_status_body_init(&header_lines, k);

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

            i_con = add_i_con(i_con,"\0", (char *)&cur_ioa->dev[j], list);  

            num_lines++;
        }

        /* print volume set and associated devices*/
        for (j = 0; j < cur_ioa->num_devices; j++)
        {
            scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
            if (!ipr_is_volume_set(&cur_ioa->dev[j]))
                continue;

            array_record = (struct ipr_array_record *)
                cur_ioa->dev[j].qac_entry;

            if (array_record->start_cand)
                continue;

            /* query resource state to acquire protection level string */
            rc = ipr_query_resource_state(cur_ioa->dev[j].gen_name,
                                          &res_state);
            strncpy(cur_ioa->dev[j].prot_level_str,
                    res_state.vset.prot_level_str, 8);

            for (k=0; k<2; k++)
                buffer[k] = print_device(&cur_ioa->dev[j],buffer[k], "%1", cur_ioa, k);

            i_con = add_i_con(i_con,"\0",
                              (char *)&cur_ioa->dev[j],
                              list);

            dev_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
            array_id = dev_record->array_id;
            num_lines++;

            for (i = 0; i < cur_ioa->num_devices; i++)
            {
                scsi_dev_data = cur_ioa->dev[i].scsi_dev_data;
                dev_record = (struct ipr_dev_record *)cur_ioa->dev[i].qac_entry;

                if (!(ipr_is_array_member(&cur_ioa->dev[i])) ||
                     (ipr_is_volume_set(&cur_ioa->dev[i])) ||
                     ((dev_record != NULL) &&
                      (dev_record->array_id != array_id)))
                {
                    continue;
                }

                strncpy(cur_ioa->dev[i].prot_level_str,
                        res_state.vset.prot_level_str, 8);

                for (k=0; k<2; k++)
                    buffer[k] = print_device(&cur_ioa->dev[i],buffer[k], "%1", cur_ioa, k);

                i_con = add_i_con(i_con,"\0", (char *)&cur_ioa->dev[i], list);
                num_lines++;
            }
        }
    }


    if (num_lines == 0)
    {
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
        s_out = screen_driver_new(&n_raid_status,NULL,header_lines,i_con);
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
            if (line_num)
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
        if (line_num)
            num_lines++;
    }

    *line_num = *line_num + num_lines;
    return body;
}

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

char *raid_stop_body_init(int *num_lines, int type)
{
    char *string_buf;
    char *buffer = NULL;
    int cur_len;
    int header_lines = 0;

    string_buf = catgets(catd,n_raid_stop.text_set, 2,
                         "Select the subsystems to delete a disk array.");
    buffer = ipr_realloc(buffer, strlen(string_buf) + 4);
    cur_len = sprintf(buffer, "%s\n", string_buf);
    header_lines += 1;

    string_buf = catgets(catd,n_raid_stop.text_set, 3,
                         "Type choice, press Enter.");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 4);
    cur_len += sprintf(buffer + cur_len, "\n%s\n", string_buf);
    header_lines += 2;

    string_buf = catgets(catd,n_raid_stop.text_set, 4,
                         "delete a disk array");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
    cur_len += sprintf(buffer + cur_len, "  1=%s\n\n", string_buf);
    header_lines += 2;

    buffer = status_header(buffer, &header_lines, type);

    *num_lines = header_lines;
    return buffer;
}

int raid_stop(i_container *i_con)
{
    int rc, i, k, array_num;
    struct ipr_common_record *common_record;
    struct ipr_array_record *array_record;
    char *buffer[2];
    struct ipr_ioa *cur_ioa;
    char *string_buf;
    int header_lines;
    struct ipr_query_res_state res_state;
    int toggle = 1;
    struct screen_output *s_out;
    mvaddstr(0,0,"RAID STOP FUNCTION CALLED");

    /* empty the linked list that contains field pointers */
    i_con = free_i_con(i_con);

    rc = RC_SUCCESS;

    array_num = 1;

    check_current_config(false);

    /* Setup screen title */
    string_buf = catgets(catd,n_raid_stop.text_set,1,"Delete a Disk Array");
    n_raid_stop.title = ipr_malloc(strlen(string_buf) + 4);
    sprintf(n_raid_stop.title, string_buf);

    for (k=0; k<2; k++)
        buffer[k] = raid_stop_body_init(&header_lines, k);

    for(cur_ioa = ipr_ioa_head;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next)
    {
        cur_ioa->num_raid_cmds = 0;

        for (i = 0;
             i < cur_ioa->num_devices;
             i++)
        {
            common_record = cur_ioa->dev[i].qac_entry;

            if ((common_record != NULL) &&
                (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD))
            {
                array_record = (struct ipr_array_record *)common_record;

                if (array_record->stop_cand)
                {
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
                    raid_cmd_tail->array_num = array_num;
                    raid_cmd_tail->ipr_ioa = cur_ioa;
                    raid_cmd_tail->ipr_dev = &cur_ioa->dev[i];

                    i_con = add_i_con(i_con,"\0",(char *)raid_cmd_tail,list);

                    rc = ipr_query_resource_state(cur_ioa->dev[i].gen_name,
                                                  &res_state);
                    strncpy(cur_ioa->dev[i].prot_level_str,
                            res_state.vset.prot_level_str, 8);

                    for (k=0; k<2; k++)
                        buffer[k] = print_device(&cur_ioa->dev[i],buffer[k], "%1", cur_ioa, k);

                    array_num++;
                }
            }
        }
    }

    if (array_num == 1) {
        /* Stop Device Parity Protection Failed */
        setup_fail_screen(&n_raid_stop_fail, 3, 7);

        s_out = screen_driver_new(&n_raid_stop_fail,NULL,header_lines,i_con);

        free(n_raid_stop_fail.title);
        n_raid_stop_fail.title = NULL;
        free(n_raid_stop_fail.body);
        n_raid_stop_fail.body = NULL;
    } else {
        toggle_field = 0;

        do {
            n_raid_stop.body = buffer[toggle&1];
            s_out = screen_driver_new(&n_raid_stop,NULL,header_lines,i_con);
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

char *confirm_raid_stop_body_init(int *num_lines, int type)
{
    char *string_buf;
    char *buffer = NULL;
    int cur_len;
    int header_lines = 0;

    string_buf = catgets(catd,n_confirm_raid_stop.text_set, 2,
                         "ATTENTION: Disk units connected to these subsystems "
                         "will not be protected after you confirm your choice.");
    buffer = add_line_to_body(buffer, string_buf, "  ", &header_lines);
    cur_len = strlen(buffer);

    string_buf = catgets(catd,n_confirm_raid_stop.text_set, 3,
                         "Press Enter to continue.");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 4);
    cur_len += sprintf(buffer + cur_len, "\n%s\n", string_buf);
    header_lines += 2;

    string_buf = catgets(catd,n_confirm_raid_stop.text_set, 4,
                         "Cancel to return and change your choice.");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
    cur_len += sprintf(buffer + cur_len, "  q=%s\n\n", string_buf);
    header_lines += 2;

    buffer = status_header(buffer, &header_lines, type);

    *num_lines = header_lines;
    return buffer;
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
        buffer[k] = confirm_raid_stop_body_init(&header_lines, k);

    for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
        cur_raid_cmd = (struct array_cmd_data *)(temp_i_con->data);
        if (cur_raid_cmd != NULL)
        {
            input = temp_i_con->field_data;
            if (strcmp(input, "1") == 0)
            {
                found = 1;
                cur_raid_cmd->do_cmd = 1;

                array_record = (struct ipr_array_record *)cur_raid_cmd->ipr_dev->qac_entry;
                if (!array_record->issue_cmd)
                {
                    array_record->issue_cmd = 1;

                    /* known_zeroed means do not preserve
                     user data on stop */
                    array_record->known_zeroed = 1;
                    cur_raid_cmd->ipr_ioa->num_raid_cmds++;
                }
            }
            else
            {
                cur_raid_cmd->do_cmd = 0;
                array_record = (struct ipr_array_record *)cur_raid_cmd->ipr_dev->qac_entry;

                if (array_record->issue_cmd)
                    array_record->issue_cmd = 0;
            }
        }
    }

    if (!found)
        return INVALID_OPTION_STATUS;

    rc = RC_SUCCESS;

    cur_raid_cmd = raid_cmd_head;

    while(cur_raid_cmd)
    {
        if (cur_raid_cmd->do_cmd)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;
            ipr_dev = cur_raid_cmd->ipr_dev;

            for (k=0; k<2; k++)
                buffer[k] = print_device(ipr_dev,buffer[k],"1",cur_ioa, k);

            for (j = 0; j < cur_ioa->num_devices; j++)
            {
                dev_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;

                if ((ipr_is_array_member(&cur_ioa->dev[j])) &&
                    (dev_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
                    (dev_record->array_id == cur_raid_cmd->array_id))
                {
                    for (k=0; k<2; k++)
                        buffer[k] = print_device(&cur_ioa->dev[j],buffer[k],"1",cur_ioa,k);
                }
            }
        }
        cur_raid_cmd = cur_raid_cmd->next;
    }

    toggle_field = 0;
    
    do {
        n_confirm_raid_stop.body = buffer[toggle&1];
        s_out = screen_driver_new(&n_confirm_raid_stop,NULL,header_lines,i_con);
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
    while(cur_raid_cmd)
    {
        if (cur_raid_cmd->do_cmd)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;
            ipr_dev = cur_raid_cmd->ipr_dev;

            rc = is_array_in_use(cur_ioa, cur_raid_cmd->array_id);
            if (rc != 0)  {

                syslog(LOG_ERR,
                       "Array %s is currently in use and cannot be deleted.\n",
                       ipr_dev->dev_name);
                return (20 | EXIT_FLAG);
            }

            if (ipr_dev->qac_entry->record_id == IPR_RECORD_ID_ARRAY_RECORD)
            {
                getmaxyx(stdscr,max_y,max_x);
                move(max_y-1,0);printw("Operation in progress - please wait");refresh();

                rc = ipr_start_stop_stop(ipr_dev->gen_name);

                if (rc != 0)
                    return (20 | EXIT_FLAG);

                rc = ipr_stop_array_protection(cur_ioa->ioa.gen_name,
                                               cur_ioa->qac_data);
                if (rc != 0)
                    return (20 | EXIT_FLAG);
            }
        }
        cur_raid_cmd = cur_raid_cmd->next;
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

    while(1)
    {
        done_bad = 0;
        not_done = 0;

        for (cur_raid_cmd = raid_cmd_head;
             cur_raid_cmd != NULL; 
             cur_raid_cmd = cur_raid_cmd->next)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;

            rc = ipr_query_command_status(cur_ioa->ioa.gen_name, &cmd_status);

            if (rc) {
                cur_ioa->num_raid_cmds = 0;
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
                    remove_device(scsi_dev_data);
                    scan_device(scsi_dev_data);
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

char *raid_start_body_init(int *num_lines, int type)
{
    char *string_buf;
    char *buffer = NULL;
    int cur_len;
    int header_lines = 0;

    string_buf = catgets(catd,n_raid_start.text_set, 2,
                         "Select the subsystems to create a disk array.");
    buffer = ipr_realloc(buffer, strlen(string_buf) + 4);
    cur_len = sprintf(buffer, "%s\n", string_buf);
    header_lines += 1;

    string_buf = catgets(catd,n_raid_start.text_set, 3,
                         "Type choice, press Enter.");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 4);
    cur_len += sprintf(buffer + cur_len, "\n%s\n", string_buf);
    header_lines += 2;

    string_buf = catgets(catd,n_raid_start.text_set, 4,
                         "create a disk array");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
    cur_len += sprintf(buffer + cur_len, "  1=%s\n\n", string_buf);
    header_lines += 2;

    buffer = status_header(buffer, &header_lines, type);

    *num_lines = header_lines;
    return buffer;
}

int raid_start(i_container *i_con)
{
    int rc, k, array_num;
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

    array_num = 1;

    check_current_config(false);

    /* Setup screen title */
    string_buf = catgets(catd,n_raid_start.text_set,1,"Create a Disk Array");
    n_raid_start.title = ipr_malloc(strlen(string_buf) + 4);
    sprintf(n_raid_start.title, string_buf);

    for (k=0; k<2; k++)
        buffer[k] = raid_start_body_init(&header_lines, k);

    for(cur_ioa = ipr_ioa_head;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next)
    {
        cur_ioa->num_raid_cmds = 0;
        array_record = cur_ioa->start_array_qac_entry;

        if (array_record != NULL)
        {
            if (raid_cmd_head)
            {
                raid_cmd_tail->next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                raid_cmd_tail = raid_cmd_tail->next;
            }
            else
            {
                raid_cmd_head = raid_cmd_tail = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
            }

            memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

            raid_cmd_tail->array_id = array_record->array_id;
            raid_cmd_tail->ipr_ioa = cur_ioa;
            raid_cmd_tail->ipr_dev = &cur_ioa->ioa;

            i_con = add_i_con(i_con,"\0",(char *)raid_cmd_tail,list);

            for (k=0; k<2; k++)
                buffer[k] = print_device(&cur_ioa->ioa,buffer[k],"%1",
                                         cur_ioa, k);

            array_num++;
        }
    }

    if (array_num == 1) {
        /* Start Device Parity Protection Failed */
        setup_fail_screen(&n_raid_start_fail, 3, 7);

        s_out = screen_driver_new(&n_raid_start_fail,NULL,header_lines,i_con);

        free(n_raid_start_fail.title);
        n_raid_start_fail.title = NULL;
        free(n_raid_start_fail.body);
        n_raid_start_fail.body = NULL;

        rc = s_out->rc;
        free(s_out);
    } else {
        toggle_field = 0;

        do {
            n_raid_start.body = buffer[toggle&1];
            s_out = screen_driver_new(&n_raid_start,NULL,header_lines,i_con);
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

    for (temp_i_con = i_con;
         temp_i_con != NULL;
         temp_i_con = temp_i_con->next_item) {

        cur_raid_cmd = (struct array_cmd_data *)(temp_i_con->data);
        if (cur_raid_cmd != NULL)  {

            input = temp_i_con->field_data;

            if (strcmp(input, "1") == 0) {

                rc = configure_raid_start(temp_i_con);

                if (RC_SUCCESS == rc)
                {
                    found = 1;
                    cur_raid_cmd->do_cmd = 1;
                    cur_raid_cmd->ipr_ioa->num_raid_cmds++;
                } else
                    return rc;
            } else
                cur_raid_cmd->do_cmd = 0;
        }
    }

    if (found != 0)
    {
        /* Go to verification screen */
        rc = confirm_raid_start(NULL);
        rc |= EXIT_FLAG;
    } else
        rc =  INVALID_OPTION_STATUS; /* "Invalid options specified" */

    return rc;
}

char *configure_raid_start_body_init(int *num_lines, int type)
{
    char *string_buf;
    char *buffer = NULL;
    int cur_len;
    int header_lines = 0;

    string_buf = catgets(catd,n_configure_raid_start.text_set, 2,
                         "Type option, press Enter.");
    buffer = ipr_realloc(buffer, strlen(string_buf) + 4);
    cur_len = sprintf(buffer, "\n%s\n", string_buf);
    header_lines += 2;

    string_buf = catgets(catd,n_configure_raid_start.text_set, 3,
                         "Select");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
    cur_len += sprintf(buffer + cur_len, "  1=%s\n\n", string_buf);
    header_lines += 2;

    buffer = status_header(buffer, &header_lines, type);

    *num_lines = header_lines;
    return buffer;
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
        buffer[k] = configure_raid_start_body_init(&header_lines, k);

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

            i_con2 = add_i_con(i_con2,"\0",(char *)&cur_ioa->dev[j],list);
        }
    }

    do
    {
        toggle_field = 0;

        do {
            n_configure_raid_start.body = buffer[toggle&1];
            s_out = screen_driver_new(&n_configure_raid_start,NULL,header_lines,i_con2);
            toggle++;
        } while (s_out->rc == TOGGLE_SCREEN);

        rc = s_out->rc;

        if (rc > 0)
            goto leave;

        i_con2 = s_out->i_con;
        free(s_out);

        found = 0;

        for (temp_i_con = i_con2;
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
            rc = configure_raid_parameters(i_con);

            if ((rc &  EXIT_FLAG) ||
                !(rc & CANCEL_FLAG))
                goto leave;

            /* User selected Cancel, clear out current selections
             for redisplay */
            for (temp_i_con = i_con2;
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

char *confirm_raid_start_body_init(int *num_lines, int type)
{
    char *string_buf;
    char *buffer = NULL;
    int cur_len;
    int header_lines = 0;

    string_buf = catgets(catd,n_confirm_raid_start.text_set, 2,
                         "ATTENTION: Data will not be preserved and "
                         "may be lost on selected disk units when "
                         "parity protection is enabled.");
    buffer = add_line_to_body(buffer, string_buf, "  ", &header_lines);
    cur_len = strlen(buffer);

    string_buf = catgets(catd,n_confirm_raid_start.text_set, 3,
                         "Press Enter to continue.");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 4);
    cur_len += sprintf(buffer + cur_len, "\n%s\n", string_buf);
    header_lines += 2;

    string_buf = catgets(catd,n_confirm_raid_start.text_set, 4,
                         "Cancel to return and change your choice.");
    buffer = ipr_realloc(buffer, cur_len + strlen(string_buf) + 8);
    cur_len += sprintf(buffer + cur_len, "  q=%s\n\n", string_buf);
    header_lines += 2;

    buffer = status_header(buffer, &header_lines, type);

    *num_lines = header_lines;
    return buffer;
}

int confirm_raid_start(i_container *i_con)
{
    struct array_cmd_data *cur_raid_cmd;
    struct ipr_ioa *cur_ioa;
    int rc, i, j, k;
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
        buffer[k] = confirm_raid_start_body_init(&header_lines, k);

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
        n_configure_raid_start.body = buffer[toggle&1];
        s_out = screen_driver_new(&n_confirm_raid_start,NULL,header_lines,NULL);
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
    while (cur_raid_cmd)
    {
        if (cur_raid_cmd->do_cmd)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;
            rc = is_array_in_use(cur_ioa, cur_raid_cmd->array_id);
            if (rc != 0)
            {
                rc = 19;  /* "Start Parity Protection failed." */
                syslog(LOG_ERR, "Devices may have changed state. Command cancelled,"
                       " please issue commands again if RAID still desired %s.\n",
                       cur_ioa->ioa.gen_name);
                return rc;
            }
        }
        cur_raid_cmd = cur_raid_cmd->next;
    }

    /* now issue the start array command */
    for (cur_raid_cmd = raid_cmd_head;
         cur_raid_cmd != NULL; 
         cur_raid_cmd = cur_raid_cmd->next)
    {
        if (cur_raid_cmd->do_cmd == 0)
            continue;

        cur_ioa = cur_raid_cmd->ipr_ioa;
        rc = ipr_start_array_protection(cur_ioa->ioa.gen_name,
                                        cur_ioa->qac_data,
                                        cur_raid_cmd->stripe_size,
                                        cur_raid_cmd->prot_level);

    }

    rc = raid_start_complete(NULL);

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
  struct ipr_cmd_status_record *status_record;
  int done_bad;
  int not_done = 0;
  int rc, j;
  u32 percent_cmplt = 0;
  struct ipr_ioa *cur_ioa;
  u32 num_starts = 0;
  struct array_cmd_data *cur_raid_cmd;

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

      for (cur_ioa = ipr_ioa_head;
	   cur_ioa != NULL;
	   cur_ioa = cur_ioa->next)
      {
          if (cur_ioa->num_raid_cmds > 0) {

              rc = ipr_query_command_status(cur_ioa->ioa.gen_name, &cmd_status);

              if (rc)   {
                  cur_ioa->num_raid_cmds = 0;
                  done_bad = 1;
                  continue;
              }

              status_record = cmd_status.record;

              for (j=0; j < cmd_status.num_records; j++, status_record++)
              {
                  if (status_record->command_code == IPR_START_ARRAY_PROTECTION)
                  {
                      for (cur_raid_cmd = raid_cmd_head;
                           cur_raid_cmd != NULL; 
                           cur_raid_cmd = cur_raid_cmd->next)
                      {
                          if ((cur_raid_cmd->ipr_ioa == cur_ioa) &&
                              (cur_raid_cmd->array_id == status_record->array_id))
                          {
                              num_starts++;
                              if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS)
                              {
                                  if (status_record->percent_complete < percent_cmplt)
                                      percent_cmplt = status_record->percent_complete;
                                  not_done = 1;
                              }
                              else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL)
                              {
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
    struct ipr_common_record *common_record;
    struct ipr_dev_record *device_record;
    struct array_cmd_data *cur_raid_cmd;
    u8 array_id;
    char *buffer = NULL;
    int num_lines = 0;
    struct ipr_ioa *cur_ioa;
    u32 len = 0;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"RAID REBUILD FUNCTION CALLED");

    rc = RC_SUCCESS;

    array_num = 1;

    check_current_config(true);

    for(cur_ioa = ipr_ioa_head;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next)
    {
        cur_ioa->num_raid_cmds = 0;

        for (i = 0;
             i < cur_ioa->num_devices;
             i++)
        {
            common_record = cur_ioa->dev[i].qac_entry;

            if ((common_record != NULL) &&
                (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD))
            {
                device_record = (struct ipr_dev_record *)common_record;

                if (device_record->rebuild_cand)
                {
                    /* now find matching resource to display device data */
                    if (raid_cmd_head)
                    {
                        raid_cmd_tail->next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                        raid_cmd_tail = raid_cmd_tail->next;
                    }
                    else
                    {
                        raid_cmd_head = raid_cmd_tail = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                    }

                    raid_cmd_tail->next = NULL;

                    memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

                    array_id =  device_record->array_id;

                    raid_cmd_tail->array_id = array_id;
                    raid_cmd_tail->array_num = array_num;
                    raid_cmd_tail->ipr_ioa = cur_ioa;
                    raid_cmd_tail->ipr_dev = &cur_ioa->dev[i];

                    i_con = add_i_con(i_con,"\0",(char *)raid_cmd_tail,list);

                    buffer = print_device(&cur_ioa->ioa,buffer,"%1",cur_ioa, 0);

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

    cur_raid_cmd = raid_cmd_head;
    while(cur_raid_cmd)
    {
        cur_raid_cmd = cur_raid_cmd->next;
        free(raid_cmd_head);
        raid_cmd_head = cur_raid_cmd;
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
    struct array_cmd_data *cur_raid_cmd;
    struct ipr_dev_record *device_record;
    struct ipr_ioa *cur_ioa;
    int rc;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    char *buffer = NULL;
    u32 num_lines = 0;
    int found = 0;
    char *input;
    i_container *temp_i_con;
    fn_out *output = init_output();  
    mvaddstr(0,0,"CONFIRM RAID REBUILD FUNCTION CALLED");

    found = 0;

    for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
        cur_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
        if (cur_raid_cmd != NULL)
        {
            input = temp_i_con->field_data;

            if (strcmp(input, "1") == 0)
            {
                found = 1;
                cur_raid_cmd->do_cmd = 1;

                device_record = (struct ipr_dev_record *)cur_raid_cmd->ipr_dev->qac_entry;
                if (!device_record->issue_cmd)
                {
                    device_record->issue_cmd = 1;
                    cur_raid_cmd->ipr_ioa->num_raid_cmds++;
                }
            }
            else
            {
                cur_raid_cmd->do_cmd = 0;

                device_record = (struct ipr_dev_record *)cur_raid_cmd->ipr_dev->qac_entry;
                if (device_record->issue_cmd)
                {
                    device_record->issue_cmd = 0;
                    cur_raid_cmd->ipr_ioa->num_raid_cmds--;
                }
            }
        }
    }

    if (!found)
    {
        output = free_fn_out(output);
        return INVALID_OPTION_STATUS;
    }

    cur_raid_cmd = raid_cmd_head;

    while(cur_raid_cmd)
    {
        if (cur_raid_cmd->do_cmd)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;
            device_record = (struct ipr_dev_record *)cur_raid_cmd->ipr_dev->qac_entry;

            buffer = print_device(&cur_ioa->ioa,buffer,"1",cur_ioa, 0);

            buf_size += 150;
            out_str = realloc(out_str, buf_size);

            out_str = strcat(out_str,buffer);
            memset(buffer, 0, strlen(buffer));
            num_lines++;
        }
        cur_raid_cmd = cur_raid_cmd->next;
    }

    screen_driver(&n_confirm_raid_rebuild,output,i_con);

    for (cur_ioa = ipr_ioa_head;
         cur_ioa != NULL;
         cur_ioa = cur_ioa->next)
    {
        if (cur_ioa->num_raid_cmds > 0)
        {
            rc = ipr_rebuild_device_data(cur_ioa->ioa.gen_name,
                                         cur_ioa->qac_data);

            if (rc != 0)
            {
                /* Rebuild failed */
                rc = 29 | EXIT_FLAG;
                return rc;
            }
        }
    }
    /* Rebuild started, view Parity Status Window for rebuild progress */
    rc = 28 | EXIT_FLAG; 
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
    struct ipr_common_record *common_record;
    struct ipr_array_record *array_record;
    struct array_cmd_data *cur_raid_cmd;
    u8 array_id;
    char *buffer = NULL;
    int num_lines = 0;
    struct ipr_ioa *cur_ioa;
    u32 len;
    int include_allowed;
    struct ipr_supported_arrays *supported_arrays;
    struct ipr_array_cap_entry *cur_array_cap_entry;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    int rc = RC_SUCCESS;
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"RAID INCLUDE FUNCTION CALLED");

    i_con = free_i_con(i_con);

    array_num = 1;

    check_current_config(false);

    for(cur_ioa = ipr_ioa_head;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next)
    {
        cur_ioa->num_raid_cmds = 0;

        for (i = 0;
             i < cur_ioa->num_devices;
             i++)
        {
            common_record = cur_ioa->dev[i].qac_entry;

            if ((common_record != NULL) &&
                (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD))
            {
                array_record = (struct ipr_array_record *)common_record;

                supported_arrays = cur_ioa->supported_arrays;
                cur_array_cap_entry =
                    (struct ipr_array_cap_entry *)supported_arrays->data;
                include_allowed = 0;
                for (j = 0;
                     j < ntohs(supported_arrays->num_entries);
                     j++)
                {
                    if (cur_array_cap_entry->prot_level ==
                        array_record->raid_level)
                    {
                        if (cur_array_cap_entry->include_allowed)
                            include_allowed = 1;
                        break;
                    }

                    cur_array_cap_entry = (struct ipr_array_cap_entry *)
                        ((void *)cur_array_cap_entry +
                         ntohs(supported_arrays->entry_length));
                }

                if (!include_allowed)
                    continue;

                array_record = (struct ipr_array_record *)common_record;

                if (array_record->established)
                {
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

                    raid_cmd_tail->next = NULL;

                    memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

                    array_id =  array_record->array_id;

                    raid_cmd_tail->array_id = array_id;
                    raid_cmd_tail->array_num = array_num;
                    raid_cmd_tail->ipr_ioa = cur_ioa;
                    raid_cmd_tail->ipr_dev = &cur_ioa->dev[i];
                    raid_cmd_tail->qac_data = NULL;

                    i_con = add_i_con(i_con,"\0",(char *)raid_cmd_tail,list);

                    buffer = print_device(&cur_ioa->dev[i],buffer,"%1",cur_ioa, 0);

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

    cur_raid_cmd = raid_cmd_head;
    while(cur_raid_cmd)
    {
        if (cur_raid_cmd->qac_data)
        {
            free(cur_raid_cmd->qac_data);
            cur_raid_cmd->qac_data = NULL;
        }
        cur_raid_cmd = cur_raid_cmd->next;
        free(raid_cmd_head);
        raid_cmd_head = cur_raid_cmd;
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

    int i, j, k, array_num;
    struct array_cmd_data *cur_raid_cmd;
    struct ipr_array_query_data *qac_data;
    struct ipr_common_record *common_record;
    struct ipr_dev_record *device_record;
    struct scsi_dev_data *scsi_dev_data;
    struct ipr_array_record *array_record;
    char *buffer = NULL;
    int num_lines = 0;
    struct ipr_ioa *cur_ioa;
    u32 len = 0;
    int found = 0;
    char *input;
    struct ipr_dev *ipr_dev;
    struct ipr_supported_arrays *supported_arrays;
    struct ipr_array_cap_entry *cur_array_cap_entry;
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
        cur_raid_cmd = (struct array_cmd_data *)i_con->data;
        if (cur_raid_cmd != NULL)
        {
            input = temp_i_con->field_data;
            if (strcmp(input, "1") == 0)
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

    cur_ioa = cur_raid_cmd->ipr_ioa;
    ipr_dev = cur_raid_cmd->ipr_dev;
    common_record = ipr_dev->qac_entry;

    if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD)
    {
        /* save off raid level, this information will be used to find
         minimum multiple */
        array_record = (struct ipr_array_record *)common_record;
        raid_level = array_record->raid_level;


        /* Get Query Array Config Data */
        rc = ipr_query_array_config(cur_ioa->ioa.gen_name, 0, 1,
                                    cur_raid_cmd->array_id,
                                    qac_data);

        if (rc != 0)
            free(qac_data);
        else  {
            cur_raid_cmd->qac_data = qac_data;
            common_record = (struct ipr_common_record *)qac_data->data;
            for (k = 0; k < qac_data->num_records; k++)
            {
                if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD)
                {
                    for (j = 0; j < cur_ioa->num_devices; j++)
                    {
                        scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
                        device_record = (struct ipr_dev_record *)common_record;

                        if (scsi_dev_data == NULL)
                            continue;

                        if (scsi_dev_data->handle == device_record->resource_handle)
                        {
                            cur_ioa->dev[j].qac_entry = common_record;
                        }
                    }
                }

                /* find minimum multiple for the raid level being used */
                if (common_record->record_id == IPR_RECORD_ID_SUPPORTED_ARRAYS)
                {
                    supported_arrays = (struct ipr_supported_arrays *)common_record;
                    cur_array_cap_entry = (struct ipr_array_cap_entry *)supported_arrays->data;

                    for (i=0; i<ntohs(supported_arrays->num_entries); i++)
                    {
                        if (cur_array_cap_entry->prot_level == raid_level)
                        {
                            min_mult_array_devices = cur_array_cap_entry->min_mult_array_devices;
                        }
                        cur_array_cap_entry = (struct ipr_array_cap_entry *)
                            ((void *)cur_array_cap_entry + supported_arrays->entry_length);
                    }
                }
                common_record = (struct ipr_common_record *)
                    ((unsigned long)common_record + ntohs(common_record->record_len));
            }
        }
    }

    for (j = 0; j < cur_ioa->num_devices; j++)
    {
        device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;

        if (device_record == NULL)
            continue;

        if (device_record->include_cand)
        {
            if ((cur_ioa->dev[j].scsi_dev_data) &&
                (cur_ioa->dev[j].scsi_dev_data->opens != 0))
            {
                syslog(LOG_ERR,"Include Device not allowed, device in use - %s\n",
                       cur_ioa->dev[j].gen_name); 
                continue;
            }

            i_con = add_i_con(i_con,"\0",(char *)&cur_ioa->dev[j],list);

            buffer = print_device(&cur_ioa->dev[j],buffer,"1",cur_ioa, 0);

            buf_size += 150;
            out_str = realloc(out_str, buf_size);

            out_str = strcat(out_str,buffer);
            memset(buffer, 0, strlen(buffer));

            len = 0;
            num_lines++;
            array_num++;
        }
    }

    if (array_num <= min_mult_array_devices)
    {
        ipr_dev = cur_raid_cmd->ipr_dev;
        common_record = ipr_dev->qac_entry;

        if (!(common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD))
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
        input = temp_i_con->field_data;
        if (strcmp(input, "1") == 0)
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
            ipr_dev = (struct ipr_dev *)temp_i_con->data;
            if (ipr_dev != NULL)
            {
                input = temp_i_con->field_data;

                if (strcmp(input, "1") == 0)
                {
                    found = 1;
                    cur_raid_cmd->do_cmd = 1;

                    device_record = (struct ipr_dev_record *)ipr_dev->qac_entry;
                    if (!device_record->issue_cmd)
                    {
                        device_record->issue_cmd = 1;
                        device_record->known_zeroed = 1;
                        cur_raid_cmd->ipr_ioa->num_raid_cmds++;
                    }
                }
                else
                {
                    device_record = (struct ipr_dev_record *)ipr_dev->qac_entry;
                    if (device_record->issue_cmd)
                    {
                        device_record->issue_cmd = 0;
                        device_record->known_zeroed = 0;
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

    struct scsi_dev_data *scsi_dev_data;
    struct array_cmd_data *cur_raid_cmd;
    struct ipr_dev_record *device_record;
    int i, j, found;
    u16 len;
    char *buffer = NULL;
    u32 num_lines = 0;
    struct ipr_array_record *array_record;
    struct ipr_array_query_data *qac_data;
    struct ipr_common_record *common_record;
    struct ipr_ioa *cur_ioa;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    int rc = RC_SUCCESS;
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"CONFIRM RAID INCLUDE FUNCTION CALLED");

    cur_raid_cmd = raid_cmd_head;

    while(cur_raid_cmd)
    {
        if (cur_raid_cmd->do_cmd)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;

            array_record = (struct ipr_array_record *)
                cur_raid_cmd->ipr_dev->qac_entry;

            if (array_record->common.record_id ==
                IPR_RECORD_ID_ARRAY_RECORD)
            {
                qac_data = cur_raid_cmd->qac_data;
            }
            else
            {
                qac_data = cur_ioa->qac_data;
            }

            common_record = (struct ipr_common_record *)
                qac_data->data;

            for (i = 0; i < qac_data->num_records; i++)
            {
                device_record = (struct ipr_dev_record *)common_record;
                if (device_record->issue_cmd)
                {
                    found = 0;
                    for (j = 0; j < cur_ioa->num_devices; j++)
                    {
                        scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
                        if (scsi_dev_data == NULL)
                            continue;

                        if (scsi_dev_data->handle == device_record->resource_handle)
                        {
                            found = 1;
                            break;
                        }
                    }

                    if (found)
                    {
                        buffer = print_device(&cur_ioa->dev[j],buffer,"1",cur_ioa, 0);

                        buf_size += 150;
                        out_str = realloc(out_str, buf_size);
                        out_str = strcat(out_str,buffer);

                        len = 0;
                        memset(buffer, 0, strlen(buffer));
                        num_lines++;
                    }
                }

                common_record = (struct ipr_common_record *)
                    ((unsigned long)common_record +
                     ntohs(common_record->record_len));
            }
        }
        cur_raid_cmd = cur_raid_cmd->next;
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
    struct array_cmd_data *cur_raid_cmd;
    struct scsi_dev_data *scsi_dev_data;
    struct ipr_dev_record *device_record;
    struct devs_to_init_t *cur_dev_init;
    u32 i, j, found;
    u32 num_lines = 0;
    u8 num_devs=0;
    struct ipr_ioa *cur_ioa;
    struct ipr_array_record *array_record;
    struct ipr_array_query_data *qac_data;
    struct ipr_common_record *common_record;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    int rc = RC_SUCCESS;
    struct screen_output *s_out;
    fn_out *output = init_output();
    int format_include_cand();
    int dev_include_complete(u8 num_devs);
    mvaddstr(0,0,"CONFIRM RAID INCLUDE FUNCTION CALLED");

    cur_raid_cmd = raid_cmd_head;

    while(cur_raid_cmd)
    {
        if (cur_raid_cmd->do_cmd)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;

            array_record = (struct ipr_array_record *)
                cur_raid_cmd->ipr_dev->qac_entry;

            if (array_record->common.record_id ==
                IPR_RECORD_ID_ARRAY_RECORD)
            {
                qac_data = cur_raid_cmd->qac_data;
            }
            else
            {
                qac_data = cur_ioa->qac_data;
            }

            common_record = (struct ipr_common_record *)
                qac_data->data;

            for (i = 0; i < qac_data->num_records; i++)
            {
                device_record = (struct ipr_dev_record *)common_record;
                if (device_record->issue_cmd)
                {
                    found = 0;
                    for (j = 0; j < cur_ioa->num_devices; j++)
                    {
                        scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
                        if (scsi_dev_data == NULL)
                            continue;

                        if (scsi_dev_data->handle ==
                            device_record->resource_handle)
                        {
                            found = 1;
                            break;
                        }
                    }

                    if (found)
                    {
                        buffer = print_device(&cur_ioa->dev[j],buffer,"1",cur_ioa, 0);

                        buf_size += 100;
                        out_str = realloc(out_str, buf_size);
                        out_str = strcat(out_str,buffer);
                        memset(buffer, 0, strlen(buffer));
                        num_lines++;
                    }
                }

                common_record = (struct ipr_common_record *)
                    ((unsigned long)common_record +
                     ntohs(common_record->record_len));
            }
        }
        cur_raid_cmd = cur_raid_cmd->next;
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
    cur_dev_init = dev_init_head;
    while(cur_dev_init)
    {
        cur_dev_init = cur_dev_init->next;
        free(dev_init_head);
        dev_init_head = cur_dev_init;
    }

    output = free_fn_out(output);
    return rc;
}

int format_include_cand()
{
    struct scsi_dev_data *scsi_dev_data;
    struct array_cmd_data *cur_raid_cmd;
    struct ipr_ioa *cur_ioa;
    int num_devs = 0;
    int rc = 0;
    struct ipr_array_record *array_record;
    struct ipr_array_query_data *qac_data;
    struct ipr_common_record *common_record;
    struct ipr_dev_record *device_record;
    int i, j, found, opens;

    cur_raid_cmd = raid_cmd_head;
    while (cur_raid_cmd)
    {
        if (cur_raid_cmd->do_cmd)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;

            array_record = (struct ipr_array_record *)
                cur_raid_cmd->ipr_dev->qac_entry;

            if (array_record->common.record_id ==
                IPR_RECORD_ID_ARRAY_RECORD)
            {
                qac_data = cur_raid_cmd->qac_data;
            }
            else
            {
                qac_data = cur_ioa->qac_data;
            }

            common_record = (struct ipr_common_record *)
                qac_data->data;

            for (i = 0; i < qac_data->num_records; i++)
            {
                device_record = (struct ipr_dev_record *)common_record;
                if (device_record->issue_cmd)
                {
                    found = 0;
                    for (j = 0; j < cur_ioa->num_devices; j++)
                    {
                        scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
                        if (scsi_dev_data == NULL)
                            continue;

                        if (scsi_dev_data->handle ==
                            device_record->resource_handle)
                        {
                            found = 1;
                            break;
                        }
                    }

                    if (found)
                    {
                        /* get current "opens" data for this device to determine conditions to continue */
                        opens = num_device_opens(scsi_dev_data->host,
                                                 scsi_dev_data->channel,
                                                 scsi_dev_data->id,
                                                 scsi_dev_data->lun);
                        if (opens != 0)
                        {
                            syslog(LOG_ERR,"Include Device not allowed, device in use - %s\n",
                                   cur_ioa->dev[j].gen_name);
                            device_record->issue_cmd = 0;
                            continue;
                        }

                        if (dev_init_head)
                        {
                            dev_init_tail->next = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));
                            dev_init_tail = dev_init_tail->next;
                        }
                        else
                            dev_init_head = dev_init_tail = (struct devs_to_init_t *)malloc(sizeof(struct devs_to_init_t));

                        memset(dev_init_tail, 0, sizeof(struct devs_to_init_t));
                        dev_init_tail->ioa = cur_ioa;
                        dev_init_tail->ipr_dev = &cur_ioa->dev[j];
                        dev_init_tail->do_init = 1;

                        /* Issue the format. Failure will be detected by query command status */
                        rc = ipr_format_unit(dev_init_tail->ipr_dev->gen_name);  //FIXME  Mandatory lock?

                        num_devs++;
                    }
                }
                common_record = (struct ipr_common_record *)
                    ((unsigned long)common_record +
                     ntohs(common_record->record_len));
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
    int rc, i, j;
    struct devs_to_init_t *cur_dev_init;
    u32 percent_cmplt = 0;
    int num_includes = 0;
    int found;
    struct ipr_ioa *cur_ioa;
    struct array_cmd_data *cur_raid_cmd;
    struct ipr_array_record *array_record;
    struct ipr_common_record *common_record_saved;
    struct ipr_dev_record *device_record_saved;
    struct ipr_common_record *common_record;
    struct ipr_dev_record *device_record;

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

        for (cur_dev_init = dev_init_head;
             cur_dev_init != NULL;
             cur_dev_init = cur_dev_init->next)
        {
            if (cur_dev_init->do_init) {
                rc = ipr_query_command_status(cur_dev_init->ipr_dev->gen_name,
                                              &cmd_status);

                if (rc != 0) {
                    cur_dev_init->cmplt = 100;
                    continue;
                }

                if (cmd_status.num_records == 0)
                    cur_dev_init->cmplt = 100;
                else
                {
                    status_record = cmd_status.record;
                    if (status_record->status == IPR_CMD_STATUS_SUCCESSFUL)
                    {
                        cur_dev_init->cmplt = 100;
                    }
                    else if (status_record->status == IPR_CMD_STATUS_FAILED)
                    {
                        cur_dev_init->cmplt = 100;
                        done_bad = 1;
                    }
                    else
                    {
                        cur_dev_init->cmplt = status_record->percent_complete;
                        if (cur_dev_init->cmplt < percent_cmplt)
                            percent_cmplt = cur_dev_init->cmplt;
                        not_done = 1;
                    }
                }
            }
        }

        if (!not_done)
        {
            flush_stdscr();

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
    for (cur_raid_cmd = raid_cmd_head;
         cur_raid_cmd != NULL; 
         cur_raid_cmd = cur_raid_cmd->next)
    {
        if (cur_raid_cmd->do_cmd == 0)
            continue;

        cur_ioa = cur_raid_cmd->ipr_ioa;

        if (cur_ioa->num_raid_cmds > 0)
        {
            array_record = (struct ipr_array_record *)
                cur_raid_cmd->ipr_dev->qac_entry;

            if (array_record->common.record_id == IPR_RECORD_ID_ARRAY_RECORD)
            {
                /* refresh qac data for ARRAY */
                rc = ipr_query_array_config(cur_ioa->ioa.gen_name, 0, 1,
                                            cur_raid_cmd->array_id,
                                            cur_ioa->qac_data);

                if (rc != 0)
                    continue;

                /* scan through this raid command to determine which
                 devices should be part of this parity set */
                common_record_saved = (struct ipr_common_record *)cur_raid_cmd->qac_data->data;
                for (i = 0; i < cur_raid_cmd->qac_data->num_records; i++)
                {
                    device_record_saved = (struct ipr_dev_record *)common_record_saved;
                    if (device_record_saved->issue_cmd)
                    {
                        common_record = (struct ipr_common_record *)
                            cur_ioa->qac_data->data;
                        found = 0;
                        for (j = 0; j < cur_ioa->qac_data->num_records; j++)
                        {
                            device_record = (struct ipr_dev_record *)common_record;
                            if (device_record->resource_handle ==
                                device_record_saved->resource_handle)
                            {
                                device_record->issue_cmd = 1;
                                device_record->known_zeroed = 1;
                                found = 1;
                                break;
                            }
                            common_record = (struct ipr_common_record *)
                                ((unsigned long)common_record +
                                 ntohs(common_record->record_len));
                        }
                        if (!found)
                        {
                            syslog(LOG_ERR,"Resource not found to include device to %s failed\n", cur_ioa->ioa.gen_name);
                            break;
                        }
                    }
                    common_record_saved = (struct ipr_common_record *)
                        ((unsigned long)common_record_saved +
                         ntohs(common_record_saved->record_len));
                }

                if (!found)
                    continue;
            }

            rc = ipr_add_array_device(cur_ioa->ioa.gen_name,
                                      cur_ioa->qac_data);

            if (rc != 0)
                rc = 26;
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

        for (cur_ioa = ipr_ioa_head;
             cur_ioa != NULL;
             cur_ioa = cur_ioa->next)
        {
            if (cur_ioa->num_raid_cmds > 0) {
                rc = ipr_query_command_status(cur_ioa->ioa.gen_name,
                                              &cmd_status);

                if (rc) {
                    cur_ioa->num_raid_cmds = 0;
                    done_bad = 1;
                    continue;
                }

                status_record = cmd_status.record;

                for (j=0; j < cmd_status.num_records; j++, status_record++)
                {
                    if (status_record->command_code == IPR_ADD_ARRAY_DEVICE)
                    {
                        for (cur_raid_cmd = raid_cmd_head;
                             cur_raid_cmd != NULL; 
                             cur_raid_cmd = cur_raid_cmd->next)
                        {
                            if ((cur_raid_cmd->ipr_ioa = cur_ioa) &&
                                (cur_raid_cmd->array_id == status_record->array_id))
                            {
                                num_includes++;
                                if (status_record->status == IPR_CMD_STATUS_IN_PROGRESS)
                                {
                                    if (status_record->percent_complete < percent_cmplt)
                                        percent_cmplt = status_record->percent_complete;
                                    not_done = 1;
                                }
                                else if (status_record->status != IPR_CMD_STATUS_SUCCESSFUL)
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
            for (cur_raid_cmd = raid_cmd_head;
                 cur_raid_cmd != NULL; 
                 cur_raid_cmd = cur_raid_cmd->next)
            {
                if (cur_raid_cmd->do_cmd)
                {
                    common_record =
                        cur_raid_cmd->ipr_dev->qac_entry;
                    if ((common_record != NULL) &&
                        (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD))
                    {
                        rc = ipr_re_read_partition(cur_raid_cmd->ipr_dev->gen_name);
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
    struct ipr_ioa *cur_ioa;
    struct scsi_dev_data *scsi_dev_data;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"CONCURRENT REMOVE DEVICE FUNCTION CALLED");

    rc = RC_SUCCESS;
    i_con = free_i_con(i_con);

    check_current_config(false);

    for(cur_ioa = ipr_ioa_head;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next)
    {
        for (j = 0; j < cur_ioa->num_devices; j++)
        {
            scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
            if ((scsi_dev_data == NULL) ||
                (scsi_dev_data->type == IPR_TYPE_ADAPTER))
                continue;

            buffer = print_device(&cur_ioa->dev[j],buffer,"%1",cur_ioa, 0);
            i_con = add_i_con(i_con,"\0", (char *)cur_ioa->dev[j].scsi_dev_data, list);

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

    int rc;
    struct ipr_ioa *cur_ioa;
    struct ipr_reclaim_query_data *reclaim_buffer;
    struct ipr_reclaim_query_data *cur_reclaim_buffer;
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

    reclaim_buffer = (struct ipr_reclaim_query_data*)malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

    for (cur_ioa = ipr_ioa_head, cur_reclaim_buffer = reclaim_buffer; cur_ioa != NULL; cur_ioa = cur_ioa->next, cur_reclaim_buffer++)
    {
        rc = ipr_reclaim_cache_store(cur_ioa,
                                     IPR_RECLAIM_QUERY,
                                     cur_reclaim_buffer);

        if (rc != 0) {
            cur_ioa->reclaim_data = NULL;
            continue;
        }

        cur_ioa->reclaim_data = cur_reclaim_buffer;

        if (cur_reclaim_buffer->reclaim_known_needed ||
            cur_reclaim_buffer->reclaim_unknown_needed)
        {
            num_needed++;
        }
    }

    if (num_needed)
    {
        cur_ioa = ipr_ioa_head;

        while(cur_ioa)
        {
            if ((cur_ioa->reclaim_data) &&
                (cur_ioa->reclaim_data->reclaim_known_needed ||
                 cur_ioa->reclaim_data->reclaim_unknown_needed))
            {
                buffer = print_device(&cur_ioa->ioa,buffer,"%1",cur_ioa, 0);

                buf_size += 100;
                out_str = realloc(out_str, buf_size);
                out_str = strcat(out_str,buffer);

                memset(buffer, 0, strlen(buffer));
                ioa_num++;
                i_con = add_i_con(i_con, "\0",(char *)cur_ioa,list);
            }
            cur_reclaim_buffer++;
            cur_ioa = cur_ioa->next;
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

  struct ipr_ioa *cur_ioa, *reclaim_ioa;
  struct scsi_dev_data *scsi_dev_data;
  struct ipr_query_res_state res_state;
  int j, rc;
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
      cur_ioa = (struct ipr_ioa *) current->data;
      if (cur_ioa != NULL)
	{
	  if (strcmp(current->field_data, "1") == 0)
	    {
	      if (ioa_num)
		{
		  /* Multiple IOAs selected */
		  ioa_num++;
		  break;
		}
	      else if (cur_ioa->reclaim_data->reclaim_known_needed ||  cur_ioa->reclaim_data->reclaim_unknown_needed)
		{
		  reclaim_ioa = cur_ioa;
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

  for (j = 0; j < reclaim_ioa->num_devices; j++)
  {
      scsi_dev_data = reclaim_ioa->dev[j].scsi_dev_data;
      if ((scsi_dev_data == NULL) ||
          (scsi_dev_data->type != TYPE_DISK))  //FIXME correct check?
          continue;

      /* Do a query resource state to see whether or not the device will be affected by the operation */
      rc = ipr_query_resource_state(reclaim_ioa->dev[j].gen_name, &res_state);

      if (rc != 0)
          res_state.not_oper = 1;

      /* Necessary? We have not set this field. Relates to not_oper?*/
      if (!res_state.read_write_prot)
          continue;

      buffer = print_device(&reclaim_ioa->dev[j],buffer,"%1",reclaim_ioa, 0);

      buf_size += 150;
      out_str = realloc(out_str, buf_size);
      out_str = strcat(out_str,buffer);

      memset(buffer, 0, strlen(buffer));
  }

  i_con = free_i_con(i_con);
  /* Save the chosen IOA for later use */
  add_i_con(i_con, "\0", reclaim_ioa, list);

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

    int rc;
    struct ipr_ioa *reclaim_ioa;
    char* out_str;
    int max_y,max_x;
    struct screen_output *s_out;
    int action;
    fn_out *output = init_output();

    mvaddstr(0,0,"RECLAIM RESULT FUNCTION CALLED");
    out_str = calloc (13, sizeof(char));
    reclaim_ioa = (struct ipr_ioa *) i_con->data;

    action = IPR_RECLAIM_PERFORM;

    if (reclaim_ioa->reclaim_data->reclaim_unknown_needed)
        action |= IPR_RECLAIM_UNKNOWN_PERM;

    rc = ipr_reclaim_cache_store(reclaim_ioa,
                                 action,
                                 reclaim_ioa->reclaim_data);

    if (rc != 0) {
        free(out_str);
        output = free_fn_out(output);
        return (EXIT_FLAG | 37); /* "Reclaim IOA Cache Storage failed" */
    }

    /* Everything going according to plan. Proceed with reclaim. */
    getmaxyx(stdscr,max_y,max_x);
    move(max_y-1,0);printw("Please wait - reclaim in progress");
    refresh();

    memset(reclaim_ioa->reclaim_data, 0, sizeof(struct ipr_reclaim_query_data));
    rc = ipr_reclaim_cache_store(reclaim_ioa,
                                 IPR_RECLAIM_QUERY,
                                 reclaim_ioa->reclaim_data);

    if (rc == 0)
    {
        if (reclaim_ioa->reclaim_data->reclaim_known_performed)
        {
            if (reclaim_ioa->reclaim_data->num_blocks_needs_multiplier)
            {
                sprintf(out_str, "%12d", ntohs(reclaim_ioa->reclaim_data->num_blocks) *
                        IPR_RECLAIM_NUM_BLOCKS_MULTIPLIER);
            }
            else
            {
                sprintf(out_str, "%12d", ntohs(reclaim_ioa->reclaim_data->num_blocks));
            }
            mvaddstr(8, 0, "Press Enter to continue.");
        }
        else if (reclaim_ioa->reclaim_data->reclaim_unknown_performed)
        {
            output->cat_offset = 3;
        }
    }

    rc = ipr_reclaim_cache_store(reclaim_ioa,
                                 IPR_RECLAIM_RESET,
                                 reclaim_ioa->reclaim_data);

    if (rc != 0)
        rc = (EXIT_FLAG | 37);

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

    int rc, j;
    struct scsi_dev_data *scsi_dev_data;
    struct ipr_dev_record *device_record;
    int can_init;
    int dev_type;
    int change_size;
    char *buffer = NULL;
    int num_devs = 0;
    struct ipr_ioa *cur_ioa;
    u32 len=0;
    struct devs_to_init_t *cur_dev_init;
    u8 ioctl_buffer[IOCTL_BUFFER_SIZE];
    struct ipr_mode_parm_hdr *mode_parm_hdr;
    struct ipr_block_desc *block_desc;
    int status;
    u8 length;

    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));

    struct screen_output *s_out;
    fn_out *output = init_output();  
    mvaddstr(0,0,"CONFIGURE AF DEVICE  FUNCTION CALLED");

    i_con = free_i_con(i_con);

    rc = RC_SUCCESS;

    check_current_config(false);

    for (cur_ioa = ipr_ioa_head;
         cur_ioa != NULL;
         cur_ioa = cur_ioa->next)
    {
        if ((cur_ioa->qac_data) &&
            (cur_ioa->qac_data->num_records == 0))
            continue;

        for (j = 0; j < cur_ioa->num_devices; j++)
        {
            can_init = 1;
            scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;

            /* If not a DASD, disallow format */
            if ((scsi_dev_data == NULL) ||
                (scsi_dev_data->type != TYPE_DISK) ||
                (ipr_is_hot_spare(&cur_ioa->dev[j])))
                continue;

            /* If Advanced Function DASD */
            if (ipr_is_af_dasd_device(&cur_ioa->dev[j]))
            {
                if (action_code == IPR_INCLUDE)
                    continue;

                device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
                if (!device_record ||
                    (device_record->common.record_id != IPR_RECORD_ID_DEVICE_RECORD))
                {
                    continue;
                }

                dev_type = IPR_AF_DASD_DEVICE;
                change_size  = IPR_512_BLOCK;

                /* We allow the user to format the drive if nobody is using it */
                if (cur_ioa->dev[j].scsi_dev_data->opens != 0)
                {
                    syslog(LOG_ERR, "Format not allowed to %s, device in use\n", cur_ioa->dev[j].gen_name); 
                    continue;
                }

                if (ipr_is_array_member(&cur_ioa->dev[j]))
                {
                    if (device_record->no_cfgte_vol)
                        can_init = 1;
                    else
                        can_init = 0;
                }
                else
                {
                    can_init = is_format_allowed(&cur_ioa->dev[j]);

                }
            }
            else if (scsi_dev_data->type == TYPE_DISK)
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
                if (cur_ioa->dev[j].scsi_dev_data->opens != 0)
                {
                    syslog(LOG_ERR, "Format not allowed to %s, device in use\n", cur_ioa->dev[j].gen_name); 
                    continue;
                }

                if (is_af_blocked(&cur_ioa->dev[j], 0))
                {
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

                status = ipr_mode_select(cur_ioa->dev[j].gen_name,
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

                    status = ipr_mode_select(cur_ioa->dev[j].gen_name,
                                             &ioctl_buffer, length);

                    if (status)
                        continue;
                }

                can_init = is_format_allowed(&cur_ioa->dev[j]);
            }
            else
                continue;

            if (can_init)
            {
                if (dev_init_head)
                {
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

                buffer = print_device(&cur_ioa->dev[j],buffer,"%1",cur_ioa, 0);

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

    cur_dev_init = dev_init_head;
    while(cur_dev_init)
    {
        cur_dev_init = cur_dev_init->next;
        free(dev_init_head);
        dev_init_head = cur_dev_init;
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
    struct ipr_common_record *common_record;
    struct ipr_array_record *array_record;
    struct ipr_dev_record *device_record;
    struct array_cmd_data *cur_raid_cmd;
    char *buffer = NULL;
    int num_lines = 0;
    struct ipr_ioa *cur_ioa;
    u32 len;
    int found = 0;
    char *input;
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

    for(cur_ioa = ipr_ioa_head;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next)
    {
        cur_ioa->num_raid_cmds = 0;

        for (i = 0;
             i < cur_ioa->num_devices;
             i++)
        {
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
                 (action == IPR_RMV_HOT_SPARE)))
            {
                if (raid_cmd_head)
                {
                    raid_cmd_tail->next = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                    raid_cmd_tail = raid_cmd_tail->next;
                }
                else
                {
                    raid_cmd_head = raid_cmd_tail = (struct array_cmd_data *)malloc(sizeof(struct array_cmd_data));
                }

                raid_cmd_tail->next = NULL;

                memset(raid_cmd_tail, 0, sizeof(struct array_cmd_data));

                raid_cmd_tail->array_id = 0;
                raid_cmd_tail->array_num = array_num;
                raid_cmd_tail->ipr_ioa = cur_ioa;
                raid_cmd_tail->ipr_dev = NULL;

                i_con = add_i_con(i_con,"\0",(char *)raid_cmd_tail,list);

                buffer = print_device(&cur_ioa->ioa,buffer,"%1",cur_ioa, 0);

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

    cur_raid_cmd = raid_cmd_head;

    for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
        cur_raid_cmd = (struct array_cmd_data *)temp_i_con->data;
        if (cur_raid_cmd != NULL)
        {
            input = temp_i_con->field_data;
            if (strcmp(input, "1") == 0)
            {
                do
                {
                    rc = select_hot_spare(temp_i_con, action);
                } while (rc & REFRESH_FLAG);

                if (rc > 0)
                    goto leave;

                found = 1;
                if (!cur_raid_cmd->do_cmd)
                {
                    cur_raid_cmd->do_cmd = 1;
                    cur_raid_cmd->ipr_ioa->num_raid_cmds++;
                }
            }
            else
            {
                if (cur_raid_cmd->do_cmd)
                {
                    cur_raid_cmd->do_cmd = 0;
                    cur_raid_cmd->ipr_ioa->num_raid_cmds--;
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

    cur_raid_cmd = raid_cmd_head;
    while(cur_raid_cmd)
    {
        cur_raid_cmd = cur_raid_cmd->next;
        free(raid_cmd_head);
        raid_cmd_head = cur_raid_cmd;
    }

    return rc;
}

int select_hot_spare(i_container *i_con, int action)
{
    struct array_cmd_data *cur_raid_cmd;
    int rc, j, found;
    char *buffer = NULL;
    int num_devs = 0;
    struct ipr_ioa *cur_ioa;
    char *input;
    u32 len;
    struct ipr_dev *ipr_dev;
    struct scsi_dev_data *scsi_dev_data;
    struct ipr_dev_record *device_record;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    i_container *temp_i_con,*i_con2=NULL;
    struct screen_output *s_out;
    fn_out *output = init_output();  
    mvaddstr(0,0,"SELECT HOT SPARE FUNCTION CALLED");

    cur_raid_cmd = (struct array_cmd_data *) i_con->data;

    rc = RC_SUCCESS;

    cur_ioa = cur_raid_cmd->ipr_ioa;

    for (j = 0; j < cur_ioa->num_devices; j++)
    {
        scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;
        device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;

        if ((scsi_dev_data == NULL) ||
            (device_record == NULL))
            continue;

        if ((scsi_dev_data->type != IPR_TYPE_ADAPTER) &&
            (device_record != NULL) &&
            (device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
            (((device_record->add_hot_spare_cand) && (action == IPR_ADD_HOT_SPARE)) ||
             ((device_record->rmv_hot_spare_cand) && (action == IPR_RMV_HOT_SPARE))))
        {
            i_con2 = add_i_con(i_con2,"\0",(char *)&cur_ioa->dev[j],list);

            buffer = print_device(&cur_ioa->dev[j],buffer,"%1",cur_ioa, 0);

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
            ipr_dev = (struct ipr_dev *)temp_i_con->data;
            if (ipr_dev != NULL)
            {
                input = temp_i_con->field_data;
                if (strcmp(input, "1") == 0)
                {
                    found = 1;
                    device_record = (struct ipr_dev_record *)ipr_dev->qac_entry;
                    device_record->issue_cmd = 1;
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
    struct array_cmd_data *cur_raid_cmd;
    struct ipr_dev_record *device_record;
    struct ipr_ioa *cur_ioa;
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

    cur_raid_cmd = raid_cmd_head;

    while(cur_raid_cmd)
    {
        if (cur_raid_cmd->do_cmd)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;

            for (j = 0; j < cur_ioa->num_devices; j++)
            {
                device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;

                if ((device_record != NULL) &&
                    (device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD) &&
                    (device_record->issue_cmd))
                {
                    buffer = print_device(&cur_ioa->dev[j],buffer,"1",cur_ioa, 0);

                    buf_size += 150;
                    out_str = realloc(out_str, buf_size);

                    out_str = strcat(out_str,buffer);

                    memset(buffer, 0, strlen(buffer));
                    num_lines++;
                }
            }
        }
        cur_raid_cmd = cur_raid_cmd->next;
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
    cur_raid_cmd = raid_cmd_head;
    while (cur_raid_cmd)
    {
        if (cur_raid_cmd->do_cmd)
        {
            cur_ioa = cur_raid_cmd->ipr_ioa;
            if (rc != 0)
            {
                if (action == IPR_ADD_HOT_SPARE)
                    rc = 55 | EXIT_FLAG; /* "Configure a hot spare device failed"  */
                else if (action == IPR_RMV_HOT_SPARE)
                    rc = 56 | EXIT_FLAG; /* "Unconfigure a hot spare device failed"  */

                syslog(LOG_ERR, "Devices may have changed state. Command cancelled, "
                       "please issue commands again if RAID still desired %s.\n",
                       cur_ioa->ioa.gen_name);
                goto leave;
            }
        }
        cur_raid_cmd = cur_raid_cmd->next;
    }

    rc = hot_spare_complete(action);

    leave:
        free(out_str);
    output = free_fn_out(output);
    return rc;
}

int hot_spare_complete(int action)
{
    int rc;
    struct ipr_ioa *cur_ioa;
    struct array_cmd_data *cur_raid_cmd;

    /* now issue the start array command with "known to be zero" */
    for (cur_raid_cmd = raid_cmd_head;
         cur_raid_cmd != NULL; 
         cur_raid_cmd = cur_raid_cmd->next)
    {
        if (cur_raid_cmd->do_cmd == 0)
            continue;

        cur_ioa = cur_raid_cmd->ipr_ioa;

        if (cur_ioa->num_raid_cmds > 0)
        {
            flush_stdscr();

            if (action == IPR_ADD_HOT_SPARE)
                rc = ipr_add_hot_spare(cur_ioa->ioa.gen_name,
                                       cur_ioa->qac_data);
            else
                rc = ipr_remove_hot_spare(cur_ioa->ioa.gen_name,
                                          cur_ioa->qac_data);

            if (rc != 0)
            {
                rc = RC_FAILED;
                return rc;
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

    int rc;
    struct ipr_reclaim_query_data *reclaim_buffer, *cur_reclaim_buffer;
    struct ipr_ioa *cur_ioa;
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

    reclaim_buffer = (struct ipr_reclaim_query_data*)malloc(sizeof(struct ipr_reclaim_query_data) * num_ioas);

    for (cur_ioa = ipr_ioa_head, cur_reclaim_buffer = reclaim_buffer;
         cur_ioa != NULL;
         cur_ioa = cur_ioa->next, cur_reclaim_buffer++)
    {
        rc = ipr_reclaim_cache_store(cur_ioa,
                                     IPR_RECLAIM_QUERY | IPR_RECLAIM_EXTENDED_INFO,
                                     cur_reclaim_buffer);

        if (rc != 0) {
             cur_ioa->reclaim_data = NULL;
            continue;
        }

        cur_ioa->reclaim_data = cur_reclaim_buffer;

        if (cur_reclaim_buffer->rechargeable_battery_type)
        {
            num_needed++;
        }
    }

    if (num_needed)
    {
        cur_ioa = ipr_ioa_head;

        while(cur_ioa)
        {
            if ((cur_ioa->reclaim_data) &&
                (cur_ioa->reclaim_data->rechargeable_battery_type))
            {
                buffer = print_device(&cur_ioa->ioa,buffer,"%1",cur_ioa, 0);

                buf_size += 100;
                out_str = realloc(out_str, buf_size);
                out_str = strcat(out_str,buffer);

                memset(buffer, 0, strlen(buffer));
                len = 0;
                ioa_num++;
                i_con = add_i_con(i_con, "\0",(char *)cur_ioa,list); 
            }
            cur_reclaim_buffer++;
            cur_ioa = cur_ioa->next;
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
  struct ipr_ioa *cur_ioa;
  char *input;

  i_container *temp_i_con;
  mvaddstr(0,0,"CONFIRM FORCE BATTERY ERROR FUNCTION CALLED");

  for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
      cur_ioa = (struct ipr_ioa *)temp_i_con->data;
      if (cur_ioa != NULL)
	{
	  input = temp_i_con->field_data;

	  if (strcmp(input, "2") == 0)
	    {
	      force_error++;
	      cur_ioa->ioa.is_reclaim_cand = 1;
	    }
	  else if (strcmp(input, "5") == 0)
	    {
	      rc = show_battery_info(temp_i_con);/*cur_ioa);*/
	      return rc;
   	    }
	  else
	    {
	      cur_ioa->ioa.is_reclaim_cand = 0;
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
  struct ipr_ioa *cur_ioa;
  int ioa_num = 0;
  char *buffer = NULL;

  int buf_size = 100;
  char *out_str = calloc(buf_size,sizeof(char));
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIRM FORCE BATTERY ERROR FUNCTION CALLED");

  rc = RC_SUCCESS;

  cur_ioa = ipr_ioa_head;

  while(cur_ioa)
    {
      if ((cur_ioa->reclaim_data) &&
	  (cur_ioa->ioa.is_reclaim_cand))
        {
          buffer = print_device(&cur_ioa->ioa,buffer,"2",cur_ioa, 0);
	  
	  buf_size += 100;
	  out_str = realloc(out_str, buf_size);
	  out_str = strcat(out_str,buffer);

	  memset(buffer, 0, strlen(buffer));
	  ioa_num++;
	}

      cur_ioa = cur_ioa->next;
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
    int rc, reclaim_rc;
    struct ipr_ioa *cur_ioa;

    struct ipr_reclaim_query_data reclaim_buffer;
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"FORCE BATTERY ERROR FUNCTION CALLEd");

    reclaim_rc = rc = RC_SUCCESS;

    cur_ioa = ipr_ioa_head; /* Scott's bug fix 1 */

    while(cur_ioa)
    {
        if ((cur_ioa->reclaim_data) &&
            (cur_ioa->ioa.is_reclaim_cand))
        {
            rc = ipr_reclaim_cache_store(cur_ioa,
                                         IPR_RECLAIM_FORCE_BATTERY_ERROR,
                                         &reclaim_buffer);

            if (rc != 0)
                /* "Attempting to force the selected battery packs into an
                 error state failed." */
                reclaim_rc = 43;

            if (reclaim_buffer.action_status !=
                IPR_ACTION_SUCCESSFUL)
            {
                /* "Attempting to force the selected battery packs into an
                 error state failed." */
                syslog(LOG_ERR,
                       "Battery Force Error to %s failed. %m\n",
                       cur_ioa->ioa.gen_name);
                reclaim_rc = 43; 
            }
        }
        cur_ioa = cur_ioa->next;
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

  struct ipr_ioa *ioa = (struct ipr_ioa *) i_con->data;
  int rc;
  struct ipr_reclaim_query_data *reclaim_data =
    ioa->reclaim_data;
  char *buffer;
    
  struct screen_output *s_out;
  fn_out *output = init_output();
  buffer = malloc(100);
  mvaddstr(0,0,"SHOW BATTERY INFO FUNCTION CALLED");

  output->next = add_fn_out(output->next,1,ioa->ioa.gen_name);
    
  sprintf(buffer,"%-8s", "00000000"); /* FIXME */
  output->next = add_fn_out(output->next,2,buffer);

  sprintf(buffer,"%X", ioa->ccin);
  output->next = add_fn_out(output->next,3,buffer);

  output->next = add_fn_out(output->next,4,ioa->pci_address);
  
  sprintf(buffer,"%d", ioa->host_num);
  output->next = add_fn_out(output->next,6,buffer);
  
  switch (reclaim_data->rechargeable_battery_type)
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
	
  switch (reclaim_data->rechargeable_battery_error_state)
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
    
  sprintf(buffer,"%d",reclaim_data->raw_power_on_time);
  output->next = add_fn_out(output->next,9,buffer);

  sprintf(buffer,"%d",reclaim_data->adjusted_power_on_time);
  output->next = add_fn_out(output->next,10,buffer);

  sprintf(buffer,"%d",reclaim_data->estimated_time_to_battery_warning);
  output->next = add_fn_out(output->next,11,buffer);

  sprintf(buffer,"%d",reclaim_data->estimated_time_to_battery_failure);
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
    struct array_cmd_data *cur_raid_cmd;
    char *buffer = NULL;
    struct ipr_ioa *cur_ioa;
    u8 ioctl_buffer[255];
    struct ipr_mode_parm_hdr *mode_parm_hdr;
    struct ipr_mode_page_28_header *modepage_28_hdr;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));

    struct screen_output *s_out;
    fn_out *output = init_output();  
    mvaddstr(0,0,"BUS CONFIG FUNCTION CALLED");
    rc = RC_SUCCESS;

    i_con = free_i_con(i_con);

    config_num = 0;

    check_current_config(false);

    for(cur_ioa = ipr_ioa_head;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next)
    {
        /* mode sense page28 to focal point */
        mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
        rc = ipr_mode_sense(cur_ioa->ioa.gen_name, 0x28, mode_parm_hdr);

        if (rc != 0)
            continue;

        modepage_28_hdr = (struct ipr_mode_page_28_header *) (mode_parm_hdr + 1);

        if (modepage_28_hdr->num_dev_entries == 0)
            continue;

        i_con = add_i_con(i_con,"\0",(char *)cur_ioa,list);

        buffer = print_device(&cur_ioa->ioa,buffer,"%1",cur_ioa, 0);

        buf_size += 150;
        out_str = realloc(out_str, buf_size);

        out_str = strcat(out_str,buffer);
        memset(buffer, 0, strlen(buffer));

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

    cur_raid_cmd = raid_cmd_head;
    while(cur_raid_cmd)
    {
        cur_raid_cmd = cur_raid_cmd->next;
        free(raid_cmd_head);
        raid_cmd_head = cur_raid_cmd;
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
    struct ipr_ioa *cur_ioa;
    int rc, j, i;
    FORM *form;
    FORM *fields_form;
    FIELD **input_fields, *cur_field;
    FIELD *title_fields[3];
    WINDOW *field_pad;
    int cur_field_index;
    char buffer[100];
    int input_field_index = 0;
    int num_lines = 0;
    u8 ioctl_buffer0[255];
    u8 ioctl_buffer1[255];
    u8 ioctl_buffer2[255];
    struct ipr_mode_parm_hdr *mode_parm_hdr;
    struct ipr_page_28 *page_28_sense;
    struct ipr_page_28 *page_28_cur, *page_28_chg, *page_28_dflt;
    struct ipr_page_28 page_28_ipr;
    struct scsi_dev_data *scsi_dev_data;
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
    char *input;
    int length;
    int max_x,max_y,start_y,start_x;
    int bus_attr_menu(struct ipr_ioa *cur_ioa,
                      struct ipr_page_28 *page_28_cur,
                      struct ipr_page_28 *page_28_chg,
                      struct ipr_page_28 *page_28_dflt,
                      struct ipr_page_28 *page_28_ipr,
                      int cur_field_index,
                      int offset);

    mvaddstr(0,0,"CHANGE BUS ATTR FUNCTION CALLED");


    getmaxyx(stdscr,max_y,max_x);
    getbegyx(stdscr,start_y,start_x);

    rc = RC_SUCCESS;

    found = 0;

    for (temp_i_con = i_con; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
        cur_ioa = (struct ipr_ioa *)temp_i_con->data;
        if (cur_ioa != NULL)
        {
            input = temp_i_con->field_data;
            if (strcmp(input, "1") == 0)
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

    memset(ioctl_buffer0,0,sizeof(ioctl_buffer0));
    memset(ioctl_buffer1,0,sizeof(ioctl_buffer1));
    memset(ioctl_buffer2,0,sizeof(ioctl_buffer2));

    /* mode sense page28 to get current parms */
    mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer0;
    rc = ipr_mode_sense(cur_ioa->ioa.gen_name, 0x28, mode_parm_hdr);

    if (rc != 0)
        return 46 | EXIT_FLAG; /* "Change SCSI Bus configurations failed" */	

    page_28_sense = (struct ipr_page_28 *)mode_parm_hdr;
    page_28_cur = (struct ipr_page_28 *)
        (((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

    mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer1;
    page_28_chg = (struct ipr_page_28 *)
        (((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

    mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer2;
    page_28_dflt = (struct ipr_page_28 *)
        (((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

    /* determine changable and default values */
    for (j = 0;
         j < page_28_cur->page_hdr.num_dev_entries;
         j++)
    {
        page_28_chg->attr[j].qas_capability = 0;
        page_28_chg->attr[j].scsi_id = 0; //FIXME!!! need to allow dart (by
                                            // vend/prod & subsystem id)
        page_28_chg->attr[j].bus_width = 1;
        page_28_chg->attr[j].max_xfer_rate = 1;

        page_28_dflt->attr[j].qas_capability = 0;
        page_28_dflt->attr[j].scsi_id = page_28_cur->attr[j].scsi_id;
        page_28_dflt->attr[j].bus_width = 16;
        page_28_dflt->attr[j].max_xfer_rate = 320;  //FIXME!!! check backplane
    }

    /* Title */
    title_fields[0] =
        new_field(1, max_x - start_x,   /* new field size */ 
                  0, 0,       /* upper left corner */
                  0,           /* number of offscreen rows */
                  0);               /* number of working buffers */

    title_fields[1] =
        new_field(1, 1,   /* new field size */ 
                  1, 0,       /* upper left corner */
                  0,           /* number of offscreen rows */
                  0);               /* number of working buffers */

    title_fields[2] = NULL;

    set_field_just(title_fields[0], JUSTIFY_CENTER);

    set_field_buffer(title_fields[0],        /* field to alter */
                     0,          /* number of buffer to alter */
                     "Change SCSI Bus Configuration");  /* string value to set     */

    field_opts_off(title_fields[0], O_ACTIVE);

    form = new_form(title_fields);

    input_fields = malloc(sizeof(FIELD *));

    /* Loop for each device bus entry */
    for (j = 0,
         num_lines = 0;
         j < page_28_cur->page_hdr.num_dev_entries;
         j++)
    {
        input_fields = realloc(input_fields,
                                 sizeof(FIELD *) * (input_field_index + 1));
        input_fields[input_field_index] =
            new_field(1, 6,
                      num_lines, 2,
                      0, 0);

        field_opts_off(input_fields[input_field_index], O_ACTIVE);
        memset(buffer, 0, 100);
        sprintf(buffer,"BUS %d",
                page_28_cur->attr[j].res_addr.bus);

        set_field_buffer(input_fields[input_field_index++],
                         0,
                         buffer);

        num_lines++;
        if (page_28_chg->attr[j].qas_capability)
        {
            /* options are ENABLED(10)/DISABLED(01) */
            input_fields = realloc(input_fields,
                                     sizeof(FIELD *) * (input_field_index + 2));
            input_fields[input_field_index] =
                new_field(1, 38,
                          num_lines, 4,
                          0, 0);

            field_opts_off(input_fields[input_field_index], O_ACTIVE);
            set_field_buffer(input_fields[input_field_index++],
                             0,
                             "QAS Capability . . . . . . . . . . . :");

            input_fields[input_field_index++] =
                new_field(1, 9,   /* new field size */ 
                          num_lines, 44,       /*  */
                          0,           /* number of offscreen rows */
                          0);          /* number of working buffers */
            num_lines++;
        }
        if (page_28_chg->attr[j].scsi_id)
        {
            /* options are based on current scsi ids currently in use
             value must be 15 or less */
            input_fields = realloc(input_fields,
                                     sizeof(FIELD *) * (input_field_index + 2));
            input_fields[input_field_index] =
                new_field(1, 38,
                          num_lines, 4,
                          0, 0);

            field_opts_off(input_fields[input_field_index], O_ACTIVE);
            set_field_buffer(input_fields[input_field_index++],
                             0,
                             "Host SCSI ID . . . . . . . . . . . . :");

            input_fields[input_field_index++] =
                new_field(1, 3,   /* new field size */ 
                          num_lines, 44,       /*  */
                          0,           /* number of offscreen rows */
                          0);          /* number of working buffers */
            num_lines++;
        }
        if (page_28_chg->attr[j].bus_width)
        {
            /* check if 8 bit bus is allowable with current configuration before
             enabling option */
            for (i=0;
                 i < cur_ioa->num_devices;
                 i++)
            {
                scsi_dev_data = cur_ioa->dev[i].scsi_dev_data;

                if ((scsi_dev_data) &&
                    (scsi_dev_data->id & 0xF8) &&
                    (page_28_cur->attr[j].res_addr.bus ==  scsi_dev_data->channel))
                {
                    page_28_chg->attr[j].bus_width = 0;
                    break;
                }
            }
        }
        if (page_28_chg->attr[j].bus_width)
        {
            /* options for "Wide Enabled" (bus width), No(8) or Yes(16) bits wide */
            input_fields = realloc(input_fields,
                                     sizeof(FIELD *) * (input_field_index + 2));
            input_fields[input_field_index] =
                new_field(1, 38,
                          num_lines, 4,
                          0, 0);

            field_opts_off(input_fields[input_field_index], O_ACTIVE);
            set_field_buffer(input_fields[input_field_index++],
                             0,
                             "Wide Enabled . . . . . . . . . . . . :");

            input_fields[input_field_index++] =
                new_field(1, 4,   /* new field size */ 
                          num_lines, 44,       /*  */
                          0,           /* number of offscreen rows */
                          0);          /* number of working buffers */
            num_lines++;
        }
        if (page_28_chg->attr[j].max_xfer_rate)
        {
            /* options are 5, 10, 20, 40, 80, 160, 320 */
            input_fields = realloc(input_fields,
                                     sizeof(FIELD *) * (input_field_index + 2));
            input_fields[input_field_index] =
                new_field(1, 38,
                          num_lines, 4,
                          0, 0);

            field_opts_off(input_fields[input_field_index], O_ACTIVE);
            set_field_buffer(input_fields[input_field_index++],
                             0,
                             "Maximum Bus Throughput . . . . . . . :");

            input_fields[input_field_index++] =
                new_field(1, 9,   /* new field size */ 
                          num_lines, 44,       /*  */
                          0,           /* number of offscreen rows */
                          0);          /* number of working buffers */
            num_lines++;
        }
    }

    input_fields[input_field_index] = NULL;

    fields_form = new_form(input_fields);
    scale_form(fields_form, &form_rows, &form_cols);
    field_pad = newpad(form_rows, form_cols);
    set_form_win(fields_form, field_pad);
    set_form_sub(fields_form, field_pad); /*???*/
    set_current_field(fields_form, input_fields[0]);
    for (i = 0; i < input_field_index; i++)
    {
        field_opts_off(input_fields[i], O_EDIT);
    }

    form_opts_off(fields_form, O_BS_OVERLOAD);

    flush_stdscr();
    clear();

    set_form_win(form,stdscr);
    set_form_sub(form,stdscr);

    post_form(form);
    post_form(fields_form);

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
            cur_ioa->ioa.gen_name);
    mvaddstr(8, 0, buffer);

    refresh();
    pad_start_row = 0;
    pnoutrefresh(field_pad,
                 pad_start_row, 0,
                 field_start_row, 0,
                 field_end_row,
                 max_x);
    doupdate();

    form_driver(fields_form,
                REQ_FIRST_FIELD);

    while(1)
    {
        /* Loop for each device bus entry */
        input_field_index = 0;
        for (j = 0,
             num_lines = 0;
             j < page_28_cur->page_hdr.num_dev_entries;
             j++)
        {
            num_lines++;
            input_field_index++;
            if (page_28_chg->attr[j].qas_capability)
            {
                input_field_index++;
                if ((num_lines >= pad_start_row) &&
                    (num_lines < (pad_start_row + field_num_rows)))
                {
                    field_opts_on(input_fields[input_field_index],
                                  O_ACTIVE);
                }
                else
                {
                    field_opts_off(input_fields[input_field_index],
                                   O_ACTIVE);
                }
                if (page_28_cur->attr[j].qas_capability ==
                    IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL)
                {
                    if (page_28_ipr.attr[j].qas_capability)
                        set_field_buffer(input_fields[input_field_index++],
                                         0,
                                         "Disable");
                    else
                        set_field_buffer(input_fields[input_field_index++],
                                         0,
                                         "Disabled");
                }
                else
                {
                    if (page_28_ipr.attr[j].qas_capability)
                        set_field_buffer(input_fields[input_field_index++],
                                         0,
                                         "Enable");
                    else
                        set_field_buffer(input_fields[input_field_index++],
                                         0,
                                         "Enabled");
                }
                num_lines++;
            }
            if (page_28_chg->attr[j].scsi_id)
            {
                input_field_index++;
                if ((num_lines >= pad_start_row) &&
                    (num_lines < (pad_start_row + field_num_rows)))
                {
                    field_opts_on(input_fields[input_field_index],
                                  O_ACTIVE);
                }
                else
                {
                    field_opts_off(input_fields[input_field_index],
                                   O_ACTIVE);
                }
                sprintf(scsi_id_str[j],"%d",page_28_cur->attr[j].scsi_id);
                set_field_buffer(input_fields[input_field_index++],
                                 0,
                                 scsi_id_str[j]);
                num_lines++;
            }
            if (page_28_chg->attr[j].bus_width)
            {
                input_field_index++;
                if ((num_lines >= pad_start_row) &&
                    (num_lines < (pad_start_row + field_num_rows)))
                {
                    field_opts_on(input_fields[input_field_index],
                                  O_ACTIVE);
                }
                else
                {
                    field_opts_off(input_fields[input_field_index],
                                   O_ACTIVE);
                }
                if (page_28_cur->attr[j].bus_width == 16)
                {
                    set_field_buffer(input_fields[input_field_index++],
                                     0,
                                     "Yes");
                }
                else
                {
                    set_field_buffer(input_fields[input_field_index++],
                                     0,
                                     "No");
                }
                num_lines++;
            }
            if (page_28_chg->attr[j].max_xfer_rate)
            {
                input_field_index++;
                if ((num_lines >= pad_start_row) &&
                    (num_lines < (pad_start_row + field_num_rows)))
                {
                    field_opts_on(input_fields[input_field_index],
                                  O_ACTIVE);
                }
                else
                {
                    field_opts_off(input_fields[input_field_index],
                                   O_ACTIVE);
                }
                sprintf(max_xfer_rate_str[j],"%d MB/s",
                        (ntohl(page_28_cur->attr[j].max_xfer_rate) *
                         page_28_cur->attr[j].bus_width)/(10 * 8));
                set_field_buffer(input_fields[input_field_index++],
                                 0,
                                 max_xfer_rate_str[j]);
                num_lines++;
            }
        }

        if (form_adjust)
        {
            form_driver(fields_form,
                        REQ_NEXT_FIELD);
            form_adjust = 0;
            refresh();
        }

        prefresh(field_pad,
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
                cur_field = current_field(fields_form);
                cur_field_index = field_index(cur_field);

                rc = bus_attr_menu(cur_ioa,
                                   page_28_cur,
                                   page_28_chg,
                                   page_28_dflt,
                                   &page_28_ipr,
                                   cur_field_index,
                                   pad_start_row);

                if (rc == EXIT_FLAG)
                    goto leave;
            }
            else
            {
                clear();
                set_field_buffer(title_fields[0],
                                 0,
                                 "Processing Change SCSI Bus Configuration");
                mvaddstr(max_y - 2, 1, "Please wait for next screen");
                doupdate();
                refresh();

                /* issue mode select and reset if necessary */
                mode_parm_hdr = (struct ipr_mode_parm_hdr *)page_28_sense; 
                length = mode_parm_hdr->length + 1;
                mode_parm_hdr->length = 0;

                rc = ipr_mode_select(cur_ioa->ioa.gen_name, page_28_sense, length);

                if (rc != 0)
                    return 46 | EXIT_FLAG; /* "Change SCSI Bus configurations failed" */

                ipr_save_page_28(cur_ioa,
                                 page_28_cur,
                                 page_28_chg,
                                 &page_28_ipr);

                if (is_reset_req)
                {
                    ipr_reset_adapter(cur_ioa);
                }

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
                     j < page_28_cur->page_hdr.num_dev_entries;
                     j++)
                {
                    if (page_28_ipr.attr[j].scsi_id)
                        is_reset_req = 1;
                }

                set_field_buffer(title_fields[0],
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
                        cur_ioa->ioa.gen_name);
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
            form_driver(fields_form, REQ_NEXT_FIELD);
        else if (ch == KEY_UP)
            form_driver(fields_form, REQ_PREV_FIELD);
        else if (ch == KEY_DOWN)
            form_driver(fields_form, REQ_NEXT_FIELD);
    }

    leave:

        free_form(form);
    free_screen(NULL, NULL, input_fields);

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
    int rc, j;
    struct ipr_query_res_state res_state;
    struct scsi_dev_data *scsi_dev_data;
    struct ipr_dev_record *device_record;
    int can_init;
    int dev_type;
    char *buffer = NULL;
    int num_devs = 0;
    struct ipr_ioa *cur_ioa;
    u32 len=0;
    struct devs_to_init_t *cur_dev_init;
    int buf_size = 150;
    char *out_str = calloc(buf_size,sizeof(char));
    struct screen_output *s_out;
    fn_out *output = init_output();
    mvaddstr(0,0,"INIT DEVICE FUNCTION CALLED");

    rc = RC_SUCCESS;

    i_con = free_i_con(i_con);

    check_current_config(false);

    for (cur_ioa = ipr_ioa_head;
         cur_ioa != NULL;
         cur_ioa = cur_ioa->next)
    {
        for (j = 0; j < cur_ioa->num_devices; j++)
        {
            can_init = 1;
            scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;

            /* If not a DASD, disallow format */
            if ((scsi_dev_data == NULL) ||
                (scsi_dev_data->type != TYPE_DISK) ||
                (ipr_is_hot_spare(&cur_ioa->dev[j])))
                continue;

            /* If Advanced Function DASD */
            if (ipr_is_af_dasd_device(&cur_ioa->dev[j]))
            {
                dev_type = IPR_AF_DASD_DEVICE;

                device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
                if (device_record &&
                    (device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD))
                {
                    /* We allow the user to format the drive if nobody is using it */
                    if (cur_ioa->dev[j].scsi_dev_data->opens != 0)
                    {
                        syslog(LOG_ERR,
                               "Format not allowed to %s, device in use\n",
                               cur_ioa->dev[j].gen_name); 
                        continue;
                    }

                    if (ipr_is_array_member(&cur_ioa->dev[j]))
                    {
                        if (device_record->no_cfgte_vol)
                            can_init = 1;
                        else
                            can_init = 0;
                    }
                    else
                    {
                        can_init = is_format_allowed(&cur_ioa->dev[j]);
                    }
                }
                else
                {
                    /* Do a query resource state */
                    rc = ipr_query_resource_state(cur_ioa->dev[j].gen_name,
                                                  &res_state);

                    if (rc != 0)
                        continue;

                    /* We allow the user to format the drive if 
                     the device is read write protected. If the drive fails, then is replaced
                     concurrently it will be read write protected, but the system may still
                     have a use count for it. We need to allow the format to get the device
                     into a state where the system can use it */
                    if ((cur_ioa->dev[j].scsi_dev_data->opens != 0) && (!res_state.read_write_prot))
                    {
                        syslog(LOG_ERR, "Format not allowed to %s, device in use\n", cur_ioa->dev[j].gen_name); 
                        continue;
                    }

                    if (ipr_is_array_member(&cur_ioa->dev[j]))
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
                        can_init = is_format_allowed(&cur_ioa->dev[j]);
                    }
                }
            }
            else if (scsi_dev_data->type == TYPE_DISK)
            {
                /* We allow the user to format the drive if nobody is using it */
                if (cur_ioa->dev[j].scsi_dev_data->opens != 0)
                {
                    syslog(LOG_ERR,
                           "Format not allowed to %s, device in use\n",
                           cur_ioa->dev[j].gen_name); 
                    continue;
                }

                dev_type = IPR_JBOD_DASD_DEVICE;
                can_init = is_format_allowed(&cur_ioa->dev[j]);
            }
            else
                continue;

            if (can_init)
            {
                if (dev_init_head)
                {
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

                buffer = print_device(&cur_ioa->dev[j], buffer,"%1",cur_ioa, 0);
                i_con = add_i_con(i_con,"\0",(char *)&cur_ioa->dev[j],list);

                buf_size += 150*sizeof(char);
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
        free(out_str);
        output = free_fn_out(output);
        return 49; /* "No units available for initialize and format" */
    }

    output->next = add_fn_out(output->next,1,out_str);
    s_out = screen_driver(&n_init_device,output,i_con);
    rc = s_out->rc;
    free(s_out);

    cur_dev_init = dev_init_head;
    while(cur_dev_init)
    {
        cur_dev_init = cur_dev_init->next;
        free(dev_init_head);
        dev_init_head = cur_dev_init;
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
  char  *input;
  char *buffer = NULL;
  struct devs_to_init_t *cur_dev_init;
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

  cur_dev_init = dev_init_head;
  
  for (; temp_i_con != NULL; temp_i_con = temp_i_con->next_item)
    {
      if (temp_i_con->data != NULL)
	{
	  input = strip_trailing_whitespace(temp_i_con->field_data);
	  
	  if (strcmp(input, "1") == 0)
	    {
	      found = 1;
	      cur_dev_init->do_init = 1;
	    }
	  
	  cur_dev_init = cur_dev_init->next;
	}
    }

  if (!found)
    {
      free_fn_out(output);
      return INVALID_OPTION_STATUS;
    }

  cur_dev_init = dev_init_head;

  while(cur_dev_init)
    {
      if (cur_dev_init->do_init)
        {
          buffer = print_device(cur_dev_init->ipr_dev,buffer,"1",cur_dev_init->ioa, 0);
	  	  
	  buf_size += 150*sizeof(char);
	  out_str = realloc(out_str, buf_size);
	  out_str = strcat(out_str,buffer);
	  
	  memset(buffer, 0, strlen(buffer));
	  num_devs++;
        }
      cur_dev_init = cur_dev_init->next;
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
int bus_attr_menu(struct ipr_ioa *cur_ioa,
                  struct ipr_page_28 *page_28_cur,  
                  struct ipr_page_28 *page_28_chg,
                  struct ipr_page_28 *page_28_dflt,
                  struct ipr_page_28 *page_28_ipr,
                  int cur_field_index,
                  int offset)
{
    int input_field_index;
    int start_row;
    int i, j, scsi_id, found;
    int num_menu_items;
    int menu_index;
    ITEM **menu_item = NULL;
    struct scsi_dev_data *scsi_dev_data;
    int *userptr;
    int *retptr;
    int rc = RC_SUCCESS;

    /* Loop for each device bus entry */
    input_field_index = 1;
    start_row = DSP_FIELD_START_ROW - offset;
    for (j = 0;
         j < page_28_cur->page_hdr.num_dev_entries;
         j++)
    {
        /* start_row represents the row in which the top most
         menu item will be placed.  Ideally this will be on the
         same line as the field in which the menu is opened for*/
        start_row++;
        input_field_index += 1;
        if ((page_28_chg->attr[j].qas_capability) &&
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
                (page_28_cur->attr[j].qas_capability != *retptr))
            {
                page_28_cur->attr[j].qas_capability = *retptr;
                page_28_ipr->attr[j].qas_capability =
                    page_28_chg->attr[j].qas_capability;
            }
            i=0;
            while (menu_item[i] != NULL)
                free_item(menu_item[i++]);
            free(menu_item);
            free(userptr);
            menu_item = NULL;
            break;
        }
        else if (page_28_chg->attr[j].qas_capability)
        {
            input_field_index += 2;
            start_row++;
        }

        if ((page_28_chg->attr[j].scsi_id) &&
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
                     i < cur_ioa->num_devices;
                     i++)
                {
                    scsi_dev_data = cur_ioa->dev[i].scsi_dev_data;
                    if (scsi_dev_data == NULL)
                        continue;

                    if ((scsi_id == scsi_dev_data->id) &&
                        (page_28_cur->attr[j].res_addr.bus == scsi_dev_data->channel))
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
                (page_28_cur->attr[j].scsi_id != *retptr))
            {
                page_28_cur->attr[j].scsi_id = *retptr;
                page_28_ipr->attr[j].scsi_id =
                    page_28_chg->attr[j].scsi_id;
            }
            i=0;
            while (menu_item[i] != NULL)
                free_item(menu_item[i++]);
            free(menu_item);
            free(userptr);
            menu_item = NULL;
            break;
        }
        else if (page_28_chg->attr[j].scsi_id)
        {
            input_field_index += 2;
            start_row++;
        }

        if ((page_28_chg->attr[j].bus_width) &&
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
            if (page_28_dflt->attr[j].bus_width == 16)
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
                (page_28_cur->attr[j].bus_width != *retptr))
            {
                page_28_cur->attr[j].bus_width = *retptr;
                page_28_ipr->attr[j].bus_width =
                    page_28_chg->attr[j].bus_width;
            }
            i=0;
            while (menu_item[i] != NULL)
                free_item(menu_item[i++]);
            free(menu_item);
            free(userptr);
            menu_item = NULL;
            break;
        }
        else if (page_28_chg->attr[j].bus_width)
        {
            input_field_index += 2;
            start_row++;
        }

        if ((page_28_chg->attr[j].max_xfer_rate) &&
            (input_field_index == cur_field_index))
        {
            num_menu_items = 7;
            menu_item = malloc(sizeof(ITEM **) * (num_menu_items + 1));
            userptr = malloc(sizeof(int) * num_menu_items);

            menu_index = 0;
            switch (((ntohl(page_28_dflt->attr[j].max_xfer_rate)) *
                     page_28_cur->attr[j].bus_width)/(10 * 8))
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
                (ntohl(page_28_cur->attr[j].max_xfer_rate) !=
                 ((*retptr) * 8)/page_28_cur->attr[j].bus_width))
            {
                page_28_cur->attr[j].max_xfer_rate =
                    htonl(((*retptr) * 8)/page_28_cur->attr[j].bus_width);
                page_28_ipr->attr[j].max_xfer_rate =
                    page_28_chg->attr[j].max_xfer_rate;
            }
            i=0;
            while (menu_item[i] != NULL)
                free_item(menu_item[i++]);
            free(menu_item);
            free(userptr);
            menu_item = NULL;
            break;
        }
        else if (page_28_chg->attr[j].max_xfer_rate)
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
    if (use_box2)
    {
        wclear(box2_win);
        wrefresh(box2_win);
        delwin(box2_win);
    }

    return rc;
}

int send_dev_inits(i_container *i_con) /*(u8 num_devs)*/
{
    u8 num_devs = (u32) i_con->data; /* passed from i_num_devs in confirm_init_device */
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
    u8 remaining_devs = num_devs;
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

            rc = ipr_query_resource_state(cur_dev_init->ipr_dev->gen_name,
                                         &res_state);

            if (rc != 0) {
                cur_dev_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            scsi_dev_data = cur_dev_init->ipr_dev->scsi_dev_data;
            if (scsi_dev_data == NULL) {
                cur_dev_init->do_init = 0;
                remaining_devs--;
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
                remaining_devs--;
                continue;
            }

            /* Mode sense */
            rc = ipr_mode_sense(cur_dev_init->ipr_dev->gen_name, 0x0a, ioctl_buffer);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Sense to %s failed. %m\n",
                       cur_dev_init->ipr_dev->gen_name);
                iprlog_location(cur_dev_init->ioa);
                cur_dev_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            /* Mode sense - changeable parms */
            rc = ipr_mode_sense(cur_dev_init->ipr_dev->gen_name, 0x4a, ioctl_buffer2);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Sense to %s failed. %m\n",
                       cur_dev_init->ipr_dev->gen_name);
                iprlog_location(cur_dev_init->ioa);
                cur_dev_init->do_init = 0;
                remaining_devs--;
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
            rc = ipr_mode_select(cur_dev_init->ipr_dev->gen_name, ioctl_buffer, length);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Mode Select %s to disable qerr failed. %m\n",
                       cur_dev_init->ipr_dev->gen_name);
                iprlog_location(cur_dev_init->ioa);
                cur_dev_init->do_init = 0;
                remaining_devs--;
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

            rc = ipr_mode_select(cur_dev_init->ipr_dev->gen_name,
                                 ioctl_buffer,
                                 sizeof(struct ipr_block_desc) +
                                 sizeof(struct ipr_mode_parm_hdr));

            if (rc != 0)
            {
                cur_dev_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            /* Issue the format. Failure will be detected by query command status */
            rc = ipr_format_unit(cur_dev_init->ipr_dev->gen_name);  //FIXME  Mandatory lock?
        }
        else if ((cur_dev_init->do_init) &&
                 (cur_dev_init->dev_type == IPR_JBOD_DASD_DEVICE))
        {
            scsi_dev_data = cur_dev_init->ipr_dev->scsi_dev_data;
            if (scsi_dev_data == NULL) {
                cur_dev_init->do_init = 0;
                remaining_devs--;
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
                remaining_devs--;
                continue;
            }

            /* Issue mode sense to get the control mode page */
            status = ipr_mode_sense(cur_dev_init->ipr_dev->gen_name,
                                    0x0a, &ioctl_buffer);

            if (status)
            {
                cur_dev_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            /* Issue mode sense to get the control mode page */
            status = ipr_mode_sense(cur_dev_init->ipr_dev->gen_name,
                                    0x4a, &ioctl_buffer2);

            if (status)
            {
                cur_dev_init->do_init = 0;
                remaining_devs--;
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

            status = ipr_mode_select(cur_dev_init->ipr_dev->gen_name,
                                     &ioctl_buffer, length);

            if (status)
            {
                cur_dev_init->do_init = 0;
                remaining_devs--;
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

            status = ipr_mode_select(cur_dev_init->ipr_dev->gen_name,
                                     &ioctl_buffer, length);

            if (status)
            {
                cur_dev_init->do_init = 0;
                remaining_devs--;
                continue;
            }

            /* Issue format */
            status = ipr_format_unit(cur_dev_init->ipr_dev->gen_name);  //FIXME  Mandatory lock?    

            if (status)
            {
                /* Send a device reset to cleanup any old state */
                rc = ipr_reset_device(cur_dev_init->ipr_dev->gen_name);

                cur_dev_init->do_init = 0;
                remaining_devs--;
                continue;
            }
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

        for (cur_dev_init = dev_init_head;
             cur_dev_init != NULL;
             cur_dev_init = cur_dev_init->next)
        {
            if ((cur_dev_init->do_init) &&
                (cur_dev_init->dev_type == IPR_AF_DASD_DEVICE))
            {
                rc = ipr_query_command_status(cur_dev_init->ipr_dev->gen_name,
                                              &cmd_status);
                if (rc) {
                    cur_dev_init->cmplt = 100;
                    continue;
                }

                if (cmd_status.num_records == 0)
                    cur_dev_init->cmplt = 100;
                else
                {
                    status_record = cmd_status.record;
                    if (status_record->status == IPR_CMD_STATUS_SUCCESSFUL)
                    {
                        cur_dev_init->cmplt = 100;
                    }
                    else if (status_record->status == IPR_CMD_STATUS_FAILED)
                    {
                        cur_dev_init->cmplt = 100;
                        done_bad = 1;
                    }
                    else
                    {
                        cur_dev_init->cmplt = status_record->percent_complete;
                        if (status_record->percent_complete < percent_cmplt)
                            percent_cmplt = status_record->percent_complete;
                        not_done = 1;
                    }
                }
            }
            else if ((cur_dev_init->do_init) &&
                     (cur_dev_init->dev_type == IPR_JBOD_DASD_DEVICE))
            {
                /* Send Test Unit Ready to find percent complete in sense data. */
                rc = ipr_test_unit_ready(cur_dev_init->ipr_dev->gen_name,
                                         &sense_data);

                if (rc < 0)
                    continue;

                if ((rc == CHECK_CONDITION) &&
                    ((sense_data.error_code & 0x7F) == 0x70) &&
                    ((sense_data.sense_key & 0x0F) == 0x02))
                {
                    cur_dev_init->cmplt = ((int)sense_data.sense_key_spec[1]*100)/0x100;
                    if (cur_dev_init->cmplt < percent_cmplt)
                        percent_cmplt = cur_dev_init->cmplt;
                    not_done = 1;
                }
                else
                {
                    cur_dev_init->cmplt = 100;
                }
            }
        }

        if (!not_done)
        {
            cur_dev_init = dev_init_head;
            while(cur_dev_init)
            {
                if (cur_dev_init->do_init)
                {
                    ioa = cur_dev_init->ioa;

                    if (cur_dev_init->dev_type == IPR_AF_DASD_DEVICE)
                    {
                        /* check if evaluate device is necessary */
                        /* issue mode sense */
                        rc = ipr_mode_sense(cur_dev_init->ipr_dev->gen_name, 0, ioctl_buffer);

                        if (rc == 0)
                        {
                            mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;
                            if (mode_parm_hdr->block_desc_len > 0)
                            {
                                block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

                                if((block_desc->block_length[1] == 0x02) && 
                                   (block_desc->block_length[2] == 0x00))
                                {
                                    cur_dev_init->change_size = IPR_512_BLOCK;
                                }
                            }
                        }
                    }
                    else if (cur_dev_init->dev_type == IPR_JBOD_DASD_DEVICE)
                    {
                        /* Check if evaluate device capabilities needs to be issued */
                        memset(ioctl_buffer, 0, 255);

                        /* Issue mode sense to get the block size */
                        rc = ipr_mode_sense(cur_dev_init->ipr_dev->gen_name,
                                            0x00, mode_parm_hdr);

                        if (rc == 0) {
                            mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

                            if (mode_parm_hdr->block_desc_len > 0)
                            {
                                block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

                                if ((block_desc->block_length[1] != 0x02) ||
                                    (block_desc->block_length[2] != 0x00))
                                {
                                    cur_dev_init->change_size = IPR_522_BLOCK;
                                    remove_device(cur_dev_init->ipr_dev->scsi_dev_data);
                                }
                            }
                        }
                    }

                    if (cur_dev_init->change_size != 0)
                    {
                        /* send evaluate device down */
                        evaluate_device(cur_dev_init->ipr_dev,
                                        ioa, cur_dev_init->change_size);
                    }
                }
                cur_dev_init = cur_dev_init->next;
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
      syslog(LOG_ERR, "Error encountered concatenating log files...\n");
      syslog(LOG_ERR, "Using failsafe editor...\n");
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
    }
  else
    {
      for (i = 0; i < num_dir_entries; i++)
	{
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
  DIR *dir;
  char *input;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIRM KERNEL ROOT FUNCTION CALLED");
  
  input = i_con->field_data;
  dir = opendir(input);

  if (dir == NULL)
    {
      output = free_fn_out(output);
      return 59; /* "Invalid directory" */
    }
  else
    {
      closedir(dir);
      if (strcmp(log_root_dir, input) == 0)
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
  char *input;
  struct screen_output *s_out;
  fn_out *output = init_output();
  mvaddstr(0,0,"CONFIRM SET DEFAULT EDITOR FUNCTION CALLED");

  input = i_con->field_data;
  if (strcmp(editor, input) == 0)
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

int driver_config(i_container *i_con)
{
#if 0
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
    FORM *fields_form;
    FIELD *input_fields[6], *cur_field;
    ITEM **menu_item;
    int cur_field_index;
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

    rc = ipr_read_driver_cfg(&driver_cfg);

    if (rc != 0)
        return 58 | EXIT_FLAG; /* "Change Device Driver Configurations failed" */;

    /* Title */
    input_fields[0] =
        new_field(1, max_x - start_x, 
                  0, 0, 0, 0);
    set_field_just(input_fields[0], JUSTIFY_CENTER);
    set_field_buffer(input_fields[0], 0,
                     "Change Driver Configuration");
    field_opts_off(input_fields[0], O_ACTIVE);

    input_fields[1] = new_field(1, 38, DSP_FIELD_START_ROW, 4,
                                  0, 0);
    field_opts_off(input_fields[1], O_ACTIVE | O_EDIT);
    set_field_buffer(input_fields[1], 0,
                     "Verbosity  . . . . . . . . . . . . . :");

    input_fields[DEBUG_INDEX] = new_field(1, 3, DSP_FIELD_START_ROW, 44,
                                            0, 0);
    field_opts_off(input_fields[DEBUG_INDEX], O_EDIT);

    input_fields[5] = NULL;

    fields_form = new_form(input_fields);

    set_current_field(fields_form, input_fields[2]);

    form_opts_off(fields_form, O_BS_OVERLOAD);

    flush_stdscr();
    clear();

    set_form_win(fields_form,stdscr);
    set_form_sub(fields_form,stdscr);

    post_form(fields_form);

    mvaddstr(2, 1, "Current Driver configurations are shown.  To change");
    mvaddstr(3, 1, "setting hit \"c\" for options menu.  Highlight desired");
    mvaddstr(4, 1, "option then hit Enter");
    mvaddstr(6, 1, "c=Change Setting");
    mvaddstr(max_y - 3, 0, "Press Enter to Enable Changes");
    mvaddstr(max_y - 1, 0 , EXIT_KEY_LABEL CANCEL_KEY_LABEL);

    refresh();
    doupdate();

    form_driver(fields_form,
                REQ_FIRST_FIELD);

    while(1)
    {
        if (driver_cfg.debug_level)
            sprintf(debug_level_str,"%d",driver_cfg.debug_level);
        else
            sprintf(debug_level_str,"OFF");

        set_field_buffer(input_fields[DEBUG_INDEX],
                         0,
                         debug_level_str);

        if (form_adjust)
        {
            form_driver(fields_form,
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
            cur_field = current_field(fields_form);
            cur_field_index = field_index(cur_field);
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
            rc = ipr_write_driver_cfg(&driver_cfg);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Write driver configuration failed. %m\n");
                return 58 | EXIT_FLAG; /* "Change Device Driver Configurations failed" */
            }
            goto leave;
        }
        else if (ch == '\t')
            form_driver(fields_form, REQ_NEXT_FIELD);
        else if (ch == KEY_UP)
            form_driver(fields_form, REQ_PREV_FIELD);
        else if (ch == KEY_DOWN)
            form_driver(fields_form, REQ_NEXT_FIELD);
    }

    leave:

        free_form(fields_form);
    free_screen(NULL, NULL, input_fields);

    if (rc == RC_SUCCESS)
        rc = 57 | EXIT_FLAG; /* "Change Device Driver Configurations completed successfully" */

    flush_stdscr();
    return rc;
#endif
    return RC_EXIT;
}

char *strip_trailing_whitespace(char *str)
{
  int len;
  char *tmp;

  len = strlen(str);

  if (len == 0)
    return str;

  tmp = str + len - 1;
  while(*tmp == ' ' || *tmp == '\n')
    tmp--;
  tmp++;
  *tmp = '\0';
  return str;
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
        (scsi_dev_data->type == IPR_TYPE_ADAPTER))
    {
        if (type == 0)
            len += sprintf(body + len,"            %-8s %-16s ",
                           scsi_dev_data->vendor_id,
                           scsi_dev_data->product_id);
        else
            len += sprintf(body + len,"            %-25s ","SCSI Adapter");

        if (cur_ioa->ioa_dead)
            len += sprintf(body + len, "Not Operational\n");
        else if (cur_ioa->nr_ioa_microcode)
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
            }
            else if (ipr_is_array_member(ipr_dev)) {
                sprintf(raid_str,"  RAID %s Array Member",
                        ipr_dev->prot_level_str);
                len += sprintf(body + len, "%-25s ", raid_str);

            }
            else if (ipr_is_af_dasd_device(ipr_dev))
                len += sprintf(body + len, "%-25s ", "Advanced Function Disk");
            else
                len += sprintf(body + len, "%-25s ", "Physical Disk");
        }

        if (ipr_is_af(ipr_dev))
        {
            memset(&res_state, 0, sizeof(res_state));

            /* Do a query resource state */
            rc = ipr_query_resource_state(ipr_dev->gen_name, &res_state);

            if (rc != 0)
                res_state.not_oper = 1;
        }
        else /* JBOD */
        {
            memset(&res_state, 0, sizeof(res_state));

            format_req = 0;

            /* Issue mode sense/mode select to determine if device needs to be formatted */
            status = ipr_mode_sense(ipr_dev->gen_name, 0x0a, &ioctl_buffer);

            if (status)
                res_state.not_oper = 1;
            else {
                mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

                if (mode_parm_hdr->block_desc_len > 0)
                {
                    block_desc = (struct ipr_block_desc *)(mode_parm_hdr+1);

                    if ((block_desc->block_length[1] != 0x02) ||
                        (block_desc->block_length[2] != 0x00))
                    {
                        format_req = 1;
                    }
                    else
                    {
                        /* check device with test unit ready */
                        rc = ipr_test_unit_ready(ipr_dev->gen_name, &sense_data);

                        if ((rc < 0) || (rc == CHECK_CONDITION))
                            format_req = 1;
                    }
                }
            }
        }

        if (is_start_cand)
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
  return 0;  //FIXME
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

    ipr_evaluate_device(ioa->ioa.gen_name, scsi_dev_data->handle);

    res_addr.bus = scsi_dev_data->channel;
    res_addr.target = scsi_dev_data->id;
    res_addr.lun = scsi_dev_data->lun;

    device_converted = 0;

    while (!device_converted && timeout)
    {
        //FIXME how to determine evaluate device completed?

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
