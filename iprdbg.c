/******************************************************************/
/* Linux IBM SIS IOA debug utility                                */
/* Description: IBM Storage IOA Interface Specification (SIS)     */
/*              Linux debug utility                               */
/*                                                                */
/* (C) Copyright 2003                                             */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprdbg.c,v 1.1.1.1 2003/10/22 22:21:02 manderso Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define IBMSIS_MAX_FLIT_ENTRIES 59
#define IBMSIS_FLIT_TIMESTAMP_LEN 12
#define IBMSIS_FLIT_FILENAME_LEN 184

enum sisdbg_cmd
{
    SISDBG_READ = IBMSIS_IOA_DEBUG_READ_IOA_MEM,
    SISDBG_WRITE = IBMSIS_IOA_DEBUG_WRITE_IOA_MEM,
    SISDBG_FLIT = IBMSIS_IOA_DEBUG_READ_FLIT
};

struct ibmsis_bus_speeds
{
    u8 speed;
    char *description;
};

struct ibmsis_bus_speeds bus_speeds[] =
{
    {0, "Async"},
    {1, "10MB/s"},
    {2, "20MB/s"},
    {3, "40MB/s"},
    {4, "80MB/s"},
    {5, "160MB/s (DT)"},
    {13, "160MB/s (packetized)"},
    {14, "320MB/s (packetized)"}
};

struct ibmsis_flit_entry
{
    u32 flags;
    u32 load_name;
    u32 text_ptr;
    u32 text_len;

    u32 data_ptr;
    u32 data_len;
    u32 bss_ptr;
    u32 bss_len;
    u32 prg_parms;
    u32 lid_flags;
    u8 timestamp[IBMSIS_FLIT_TIMESTAMP_LEN];
    u8 filename[IBMSIS_FLIT_FILENAME_LEN];
};

struct ibmsis_flit
{
    u32 ioa_ucode_img_rel_lvl;
    u32 num_entries;
    struct ibmsis_flit_entry flit_entry[IBMSIS_MAX_FLIT_ENTRIES];
};              

int debug_ioctl(int fd, enum sisdbg_cmd cmd, int ioa_adx, int mask,
                int *buffer, int len);

int format_flit(struct ibmsis_flit *p_flit);

#define logtofile(...) {if (outfile) {fprintf(outfile, __VA_ARGS__);}}
#define sisprint(...) {printf(__VA_ARGS__); if (outfile) {fprintf(outfile, __VA_ARGS__);}}
#define sisloginfo(...) {syslog(LOG_INFO, __VA_ARGS__); if (outfile) {fprintf(outfile, __VA_ARGS__);}}

FILE *outfile;

int main(int argc, char *argv[])
{
    int num_args, address, prev_address = 0;
    int length, prev_length = 0;
    int ioa_num;
    int rc, fd, i, j, k, bus, num_buses;
    int arg[16], write_buf[16], bus_speed;
    int *buffer;
    char cmd[100], prev_cmd[100], cmd_line[1000];
    struct sis_ioa *p_cur_ioa;
    char ascii_buffer[17];
    struct ibmsis_flit flit;

    openlog("sisdbg",
            LOG_PERROR | /* Print error to stderr as well */
            LOG_PID |    /* Include the PID with each error */
            LOG_CONS,    /* Write to system console if there is an error
                          sending to system logger */
            LOG_USER);

    if (argc != 2)
    {
        syslog(LOG_ERR, "Usage: sisdbg /dev/ibmsis0\n");
        return -EINVAL;
    }

    outfile = fopen(".sisdbglog", "a");

    tool_init("sisdbg");

    for (p_cur_ioa = p_head_sis_ioa, ioa_num = 0;
         ioa_num < num_ioas;
         p_cur_ioa = p_cur_ioa->p_next, ioa_num++)
    {
        if (!strcmp(p_cur_ioa->ioa.dev_name, argv[1]))
            break;
    }

    if (p_cur_ioa == NULL)
    {
        syslog(LOG_ERR, "Invalid device name %s\n", argv[1]);
        return -EINVAL;
    }

    fd = open(argv[1], O_RDWR);

    if (fd < 0)
    {
        syslog(LOG_ERR, "Error opening %s. %m\n", argv[1]);
        return -1;
    }

    closelog();
    openlog("sisdbg", LOG_PID, LOG_USER);
    sisloginfo("sisdbg on %s started\n", argv[1]);
    closelog();
    openlog("sisdbg", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);

    printf("\nATTENTION: This utility is for adapter developers only! Use at your own risk!\n");
    printf("\nUse \"quit\" to exit the program.\n\n");

    memset(cmd, 0, sizeof(cmd));
    memset(prev_cmd, 0, sizeof(prev_cmd));

    while(1)
    {
        printf("SISDB(%X)> ", p_cur_ioa->ccin);

        memset(cmd, 0, sizeof(cmd));
        memset(cmd_line, 0, sizeof(cmd_line));
        memset(arg, 0, sizeof(arg));
        address = 0;

        fgets(cmd_line, 999, stdin);

        logtofile("\n%s\n", cmd_line);

        num_args = sscanf(cmd_line, "%80s %x %x %x %x %x %x %x %x "
                          "%x %x %x %x %x %x %x %x %x\n",
                          cmd, &address, &arg[0],
                          &arg[1], &arg[2], &arg[3],
                          &arg[4], &arg[5], &arg[6],
                          &arg[7], &arg[8], &arg[9],
                          &arg[10], &arg[11], &arg[12],
                          &arg[13], &arg[14], &arg[15]);

        if ((cmd[0] == '\0') && !strncmp("mr4", prev_cmd, 3))
        {
            strcpy(cmd, prev_cmd);
            address = prev_address + prev_length;
            num_args = 3;
            arg[0] = prev_length;
        }

        if (!strcmp(cmd, "speeds"))
        {
            length = 64;
            buffer = calloc(length/4, 4);

            if (buffer == NULL)
            {
                syslog(LOG_ERR, "Memory allocation error\n");
                continue;
            }

            if ((p_cur_ioa->ccin == 0x2780) ||
                (p_cur_ioa->ccin == 0x2757))
            {
                num_buses = 4;
            }
            else
            {
                num_buses = 2;
            }

            for (bus = 0, address = 0xF0016380;
                 bus < num_buses;
                 bus++)
            {
                rc = debug_ioctl(fd, SISDBG_READ, address, 0,
                                 buffer, length);

                if (!rc)
                {
                    sisprint("\nBus %d speeds:\n", bus);

                    for (i = 0; i < 16; i++)
                    {
                        bus_speed = sistoh32(buffer[i]) & 0x0000000f;

                        for (j = 0; j < sizeof(bus_speeds)/sizeof(struct ibmsis_bus_speeds); j++)
                        {
                            if (bus_speed == bus_speeds[j].speed)
                            {
                                sisprint("Target %d: %s\n", i, bus_speeds[j].description);
                            }
                        }
                    }
                }

                switch(bus)
                {
                    case 0:
                        address = 0xF001e380;
                        break;
                    case 1:
                        address = 0xF4016380;
                        break;
                    case 2:
                        address = 0xF401E380;
                        break;
                };
            }

            free(buffer);
        }
        else if (!strcmp(cmd, "mr4"))
        {
            if (num_args < 2)
            {
                sisprint("Invalid syntax\n");
                continue;
            }
            else if (num_args == 2)
            {
                length = 4;
            }
            else if ((length % 4) != 0)
            {
                sisprint("Length must be 4 byte multiple\n");
                continue;
            }
            else
                length = arg[0];

            buffer = calloc(length/4, 4);

            if (buffer == NULL)
            {
                syslog(LOG_ERR, "Memory allocation error\n");
                continue;
            }

            rc = debug_ioctl(fd, SISDBG_READ, address, 0,
                             buffer, length);

            for (i = 0; (rc == 0) && (i < (length / 4));)
            {
                sisprint("%08X: ", address+(i*4));

                memset(ascii_buffer, '\0', sizeof(ascii_buffer));

                for (j = 0;
                     (i < (length / 4)) && (j < 4);
                     j++, i++)
                {
                    sisprint("%08X ", sistoh32(buffer[i]));
                    memcpy(&ascii_buffer[j*4], &buffer[i], 4);
                    for (k = 0; k < 4; k++)
                    {
                        if (!isprint(ascii_buffer[(j*4)+k]))
                            ascii_buffer[(j*4)+k] = '.';
                    }
                }

                for (;j < 4; j++)
                {
                    sisprint("         ");
                    strncat(ascii_buffer, "....", sizeof(ascii_buffer)-1);
                }

                sisprint("   |%s|\n", ascii_buffer);
            }

            free(buffer);

            prev_address = address;
            prev_length = length;
        }
        else if (!strcmp(cmd, "mw4"))
        {
            if (num_args < 3)
            {
                sisprint("Invalid syntax\n");
                continue;
            }

            /* Byte swap everything */
            for (i = 0; i < num_args-2; i++)
                write_buf[i] = htosis32(arg[i]);

            debug_ioctl(fd, SISDBG_WRITE, address, 0,
                        write_buf, (num_args-2)*4);
        }
        else if (!strcmp(cmd, "bm4"))
        {
            if (num_args < 3)
            {
                sisprint("Invalid syntax\n");
                continue;
            }

            /* Byte swap the data */
            write_buf[0] = htosis32(arg[0]);

            debug_ioctl(fd, SISDBG_WRITE, address, arg[1],
                        write_buf, 4);
        }
        else if (!strcmp(cmd, "flit"))
        {
            rc = debug_ioctl(fd, SISDBG_FLIT, 0, 0,
                             (int *)&flit, sizeof(flit));

            if (rc != 0)
                continue;

            format_flit(&flit);
        }
        else if (!strcmp(cmd, "help"))
        {
            printf("\n");
            printf("mr4 address length                  "
                   "- Read memory\n");
            printf("mw4 address data                    "
                   "- Write memory\n");
            printf("bm4 address data preserve_mask      "
                   "- Modify bits set to 0 in preserve_mask\n");
            printf("speeds                              "
                   "- Current bus speeds for each device\n");
            printf("flit                                "
                   "- Format the flit\n");
            printf("exit                                "
                   "- Exit the program\n\n");
            continue;
        }
        else if (!strcmp(cmd, "exit") || !strcmp(cmd, "quit") ||
                 !strcmp(cmd, "q") || !strcmp(cmd, "x"))
        {
            closelog();
            openlog("sisdbg", LOG_PID, LOG_USER);
            sisloginfo("sisdbg on %s exited\n", argv[1]);
            closelog();
            return 0;
        }
        else
        {
            sisprint("Invalid command %s. Use \"help\" for a list "
                   "of valid commands\n", cmd);
            continue;
        }

        strcpy(prev_cmd, cmd);
    }
    
    return 0;
}

int debug_ioctl(int fd, enum sisdbg_cmd cmd, int ioa_adx, int mask,
                int *buffer, int len)
{
    struct ibmsis_ioctl_cmd_internal ioa_cmd;
    int rc;

    memset(&ioa_cmd, 0, sizeof(struct ibmsis_ioctl_cmd_internal));

    ioa_cmd.buffer = buffer;
    ioa_cmd.buffer_len = len;
    ioa_cmd.cdb[0] = IBMSIS_IOA_DEBUG;
    ioa_cmd.cdb[1] = cmd;
    ioa_cmd.cdb[2] = (ioa_adx >> 24) & 0xff;
    ioa_cmd.cdb[3] = (ioa_adx >> 16) & 0xff;
    ioa_cmd.cdb[4] = (ioa_adx >> 8) & 0xff;
    ioa_cmd.cdb[5] = ioa_adx & 0xff;
    ioa_cmd.cdb[6] = (mask >> 24) & 0xff;
    ioa_cmd.cdb[7] = (mask >> 16) & 0xff;
    ioa_cmd.cdb[8] = (mask >> 8) & 0xff;
    ioa_cmd.cdb[9] = mask & 0xff;
    ioa_cmd.cdb[10] = (len >> 24) & 0xff;
    ioa_cmd.cdb[11] = (len >> 16) & 0xff;
    ioa_cmd.cdb[12] = (len >> 8) & 0xff;
    ioa_cmd.cdb[13] = len & 0xff;

    if (cmd != SISDBG_WRITE)
        ioa_cmd.read_not_write = 1;

    rc = sis_ioctl(fd, IBMSIS_IOA_DEBUG, &ioa_cmd);

    if (rc != 0)
        fprintf(stderr, "Debug IOCTL failed. %d\n", errno);

    return rc;
}

int format_flit(struct ibmsis_flit *p_flit)
{
    int num_entries = sistoh32(p_flit->num_entries) - 1;
    struct ibmsis_flit_entry *p_flit_entry;

    signed int path_length;
    u8 filename[IBMSIS_FLIT_FILENAME_LEN+1];
    u8 time[IBMSIS_FLIT_TIMESTAMP_LEN+1];
    u8 buf1[IBMSIS_FLIT_FILENAME_LEN+1];
    u8 buf2[IBMSIS_FLIT_FILENAME_LEN+1];
    u8 lid_name[IBMSIS_FLIT_FILENAME_LEN+1];
    u8 path[IBMSIS_FLIT_FILENAME_LEN+1];
    int num_args, i;

    for (i = 0, p_flit_entry = p_flit->flit_entry;
         (i < num_entries) && (*p_flit_entry->timestamp);
         p_flit_entry++, i++)
    {
        snprintf(time, IBMSIS_FLIT_TIMESTAMP_LEN, p_flit_entry->timestamp);
        time[IBMSIS_FLIT_TIMESTAMP_LEN] = '\0';

        snprintf(filename, IBMSIS_FLIT_FILENAME_LEN, p_flit_entry->filename);
        filename[IBMSIS_FLIT_FILENAME_LEN] = '\0';

        num_args = sscanf(filename, "%184s "
                          "%184s "
                          "%184s "
                          "%184s ",
                          buf1, buf2, lid_name, path);

        if (num_args != 4)
        {
            syslog(LOG_ERR, "Cannot parse flit\n");
            return 1;
        }

        path_length = strlen(path);

        sisprint("LID Name:   %s  (%8X)\n", lid_name, sistoh32(p_flit_entry->load_name));
        sisprint("Time Stamp: %s\n", time);
        sisprint("LID Path:   %s\n", lid_name);
        sisprint("Text Segment:  Address %08X,  Length %8X\n",
               sistoh32(p_flit_entry->text_ptr), sistoh32(p_flit_entry->text_len));
        sisprint("Data Segment:  Address %08X,  Length %8X\n",
               sistoh32(p_flit_entry->data_ptr), sistoh32(p_flit_entry->data_len));
        sisprint("BSS Segment:   Address %08X,  Length %8X\n",
               sistoh32(p_flit_entry->bss_ptr), sistoh32(p_flit_entry->bss_len));
        sisprint("\n");
    }

    return 0;
} 
