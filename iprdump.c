/******************************************************************/
/* Linux IBM SIS microcode update utility                         */
/* Description: IBM Storage IOA Interface Specification (SIS)     */
/*              Linux  microcode update utility                   */
/*                                                                */
/* (C) Copyright 2000, 2001                                       */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprdump.c,v 1.1 2003/10/22 22:21:02 manderso Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/mman.h>

#define SISDUMP_SIZE 0x01000000 /* 16M dump */
#define SISDUMP_DIR "/var/log/"
#define MAX_DUMP_FILES 4

int main(int argc, char *argv[])
{
    int rc = 0;
    struct ibmsis_ioctl_cmd_type2 *ioa_cmd;
    DIR *dump_dir;
    struct dirent *dir_entry;
    char *p_temp;
    int fd, f_dump, str_len;
    char usr_dir[50], sisdump_fn[60];
    char temp_string[80];
    u32 new_dumpID = 0, used_dumpID;
    int index, len, ioa_num;
    struct sis_ioa *p_cur_ioa;
    int files_queue[MAX_DUMP_FILES];
    int delete_file, j, min_dumpID;
    int file_queue_index;
    int signals;

    struct dump_header
    {
        u32 total_length;
        u32 num_elems;
        u32 first_entry_offset;
    } *dump_header;
    struct dump_entry_header
    {
        u32 length;
        u32 id;
#define IBMSIS_DUMP_TEXT_ID     3
        char data[0];
    } *dump_entry_header;

    if (fork())
    {
        return 0;
    }

    openlog("sisdump",
            LOG_PERROR | /* Print error to stderr as well */
            LOG_PID |    /* Include the PID with each error */
            LOG_CONS,    /* Write to system console if there is an error
                          sending to system logger */
            LOG_USER);

    if (argc > 1)
    {
        if (strcmp(argv[1], "--version") == 0)
        {
            printf("sisdump: %s"IBMSIS_EOL, IPR_VERSION_STR);
            exit(1);
        }
        else if (strcmp(argv[1], "-d") == 0)
        {
            strcpy(usr_dir,argv[2]);
            str_len = strlen(usr_dir);
            if ((str_len < 50) && (usr_dir[str_len] != '/'))
            {
                usr_dir[str_len + 1] = '/';
                usr_dir[str_len + 2] = '\0';
            }
        }
        else
        {
            printf("Usage: sisdump [options]"IBMSIS_EOL);
            printf("  Options: --version    Print sisdump version"IBMSIS_EOL);
            printf("           -d <usr_dir>"IBMSIS_EOL);
            exit(1);
        }
    }
    else
    {
        strcpy(usr_dir,SISDUMP_DIR);
    }

    dump_dir = opendir(usr_dir);

    for (j = 0; j < MAX_DUMP_FILES; j++)
    {
        files_queue[j] = 0;
    }

    /* Scan the assigned dump directory for any dump that may already be there
     and assign any new dump files created to one higher than the highest
     numbered file */
    file_queue_index = 0;
    for (dir_entry = readdir(dump_dir),
         delete_file = 0;
         dir_entry != NULL;
         dir_entry = readdir(dump_dir))
    {
        p_temp = strstr(dir_entry->d_name, "ibmsis_D");
        if (p_temp)
        {
            used_dumpID = strtoul(&dir_entry->d_name[8], NULL, 10);
            if (used_dumpID >= new_dumpID)
            {
                new_dumpID = used_dumpID + 1;
            }

            files_queue[file_queue_index++] = used_dumpID;
            if ((file_queue_index == MAX_DUMP_FILES) || delete_file)
            {
                if (file_queue_index == MAX_DUMP_FILES)
                {
                    delete_file = 1;
                    file_queue_index = 0;
                }

                min_dumpID = used_dumpID;
                for (j=0; j < MAX_DUMP_FILES; j++)
                {
                    if (files_queue[j] < min_dumpID)
                    {
                        min_dumpID = files_queue[j];
                        files_queue[j] = files_queue[file_queue_index];
                        files_queue[file_queue_index] = min_dumpID;
                    }
                }

                sprintf(temp_string,"rm %sibmsis_D%d",SISDUMP_DIR, files_queue[file_queue_index]);
                system(temp_string);
            }
        }
    }

    tool_init("sisdump");

    signals = sigmask(SIGINT) | sigmask(SIGQUIT) | sigmask(SIGTERM);

    sigblock(~signals);

    /* Loop for all attached adapters */
    for (p_cur_ioa = p_head_sis_ioa, ioa_num = 0;
         ioa_num < num_ioas;
         p_cur_ioa = p_cur_ioa->p_next, ioa_num++)
    {
        ioa_cmd = malloc(sizeof(struct ibmsis_ioctl_cmd_type2) + SISDUMP_SIZE);

        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR | O_NONBLOCK);

        if (fd < 0)
        {
            syslog(LOG_ERR, "Cannot open %s. %m"IBMSIS_EOL, p_cur_ioa->ioa.dev_name);
            continue;
        }

        while (1) {
            memset(ioa_cmd, 0, sizeof(struct ibmsis_ioctl_cmd_type2) + SISDUMP_SIZE);

            ioa_cmd->buffer_len = SISDUMP_SIZE;
            ioa_cmd->cdb[0] = IBMSIS_DUMP_IOA;

            ioa_cmd->type = IBMSIS_IOCTL_TYPE_2;
            ioa_cmd->driver_cmd = 1;

            rc = ioctl(fd, IBMSIS_IOCTL_SEND_COMMAND, ioa_cmd);

            if (rc != 0)
            {
                if (errno == EINVAL)
                    syslog(LOG_ERR, "sisdump version incompatible with current driver"IBMSIS_EOL);
                break;
            }
            else
            {
                sprintf(sisdump_fn,"%sibmsis_D%d",usr_dir,new_dumpID);
                f_dump = creat(sisdump_fn, S_IRUSR);
                if (f_dump < 0)
                {
                    syslog(LOG_ERR, "Cannot open %s. %m"IBMSIS_EOL, sisdump_fn);
                    break;
                }

                /* get length of data obtained from within buffer */
                dump_header = (struct dump_header *)ioa_cmd->buffer;
                write(f_dump, ioa_cmd->buffer, sistoh32(dump_header->total_length));
                close(f_dump);

                /* parse through buffer and find the IBMSIS_DUMP_TEXT_ID
                 information */
                dump_entry_header =
                    (struct dump_entry_header *)&ioa_cmd->buffer[sistoh32(dump_header->first_entry_offset)];

                while (sistoh32(dump_entry_header->id) != IBMSIS_DUMP_TEXT_ID)
                {
                    dump_entry_header = (struct dump_entry_header *)
                        ((unsigned long)dump_entry_header +
                         sizeof(struct dump_entry_header) +
                         sistoh32(dump_entry_header->length));
                }

                syslog(LOG_ERR,"*********************************************************"IBMSIS_EOL);
                syslog(LOG_ERR, "Dump of ibmsis IOA has completed to file:  %s"IBMSIS_EOL, sisdump_fn);
                if (sistoh32(dump_entry_header->id) == IBMSIS_DUMP_TEXT_ID)
                {
                    index = 0;
                    while (dump_entry_header->data[index] != 0)
                    {
                        len = 0;
                        while ((dump_entry_header->data[index] != '\n') &&
                               (dump_entry_header->data[index] != 0))
                        {
                            temp_string[len++] = dump_entry_header->data[index++];
                        }
                        temp_string[len] = 0;
                        syslog(LOG_ERR,"%s"IBMSIS_EOL,temp_string);
                        index++;
                    }
                }
                syslog(LOG_ERR,"*********************************************************"IBMSIS_EOL);

                /* removed old file if necessary */
                files_queue[file_queue_index++] = new_dumpID;
                if ((file_queue_index == MAX_DUMP_FILES) || delete_file)
                {
                    if (file_queue_index == MAX_DUMP_FILES)
                    {
                        delete_file = 1;
                        file_queue_index = 0;
                    }

                    min_dumpID = new_dumpID;
                    for (j=0; j < MAX_DUMP_FILES; j++)
                    {
                        if (files_queue[j] < min_dumpID)
                        {
                            min_dumpID = files_queue[j];
                            files_queue[j] = files_queue[file_queue_index];
                            files_queue[file_queue_index] = min_dumpID;
                        }
                    }

                    sprintf(temp_string,"rm %sibmsis_D%d",SISDUMP_DIR, files_queue[file_queue_index]);
                    system(temp_string);
                }

                new_dumpID++;
            }
        }
        free(ioa_cmd);
        close(fd);
    }
    return rc;
}


