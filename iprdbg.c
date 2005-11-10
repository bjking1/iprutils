/**
  * IBM IPR adapter debug utility
  *
  * (C) Copyright 2003
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprdbg.c,v 1.18 2005/11/10 19:28:20 brking Exp $
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
#include <curses.h>
#include <term.h>

#define IPR_MAX_FLIT_ENTRIES 59
#define IPR_FLIT_TIMESTAMP_LEN 12
#define IPR_FLIT_FILENAME_LEN 184

char *tool_name = "iprdbg";

enum iprdbg_cmd {
	IPRDBG_READ = IPR_IOA_DEBUG_READ_IOA_MEM,
	IPRDBG_WRITE = IPR_IOA_DEBUG_WRITE_IOA_MEM,
	IPRDBG_FLIT = IPR_IOA_DEBUG_READ_FLIT
};

struct ipr_bus_speeds {
	u8 speed;
	char *description;
};

struct ipr_bus_speeds bus_speeds[] = {
	{0, "Async"},
	{1, "10MB/s"},
	{2, "20MB/s"},
	{3, "40MB/s"},
	{4, "80MB/s"},
	{5, "160MB/s (DT)"},
	{13, "160MB/s (packetized)"},
	{14, "320MB/s (packetized)"}
};

struct ipr_flit_entry {
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
	u8 timestamp[IPR_FLIT_TIMESTAMP_LEN];
	u8 filename[IPR_FLIT_FILENAME_LEN];
};

struct ipr_flit {
	u32 ioa_ucode_img_rel_lvl;
	u32 num_entries;
	struct ipr_flit_entry flit_entry[IPR_MAX_FLIT_ENTRIES];
};              

#define logtofile(...) {if (outfile) {fprintf(outfile, __VA_ARGS__);}}
#define iprprint(...) {printf(__VA_ARGS__); if (outfile) {fprintf(outfile, __VA_ARGS__);}}
#define iprloginfo(...) {syslog(LOG_INFO, __VA_ARGS__); if (outfile) {fprintf(outfile, __VA_ARGS__);}}

static FILE *outfile;

static int debug_ioctl(struct ipr_ioa *ioa, enum iprdbg_cmd cmd, int ioa_adx, int mask,
		       int *buffer, int len)
{
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc, fd;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd < 0) {
		syslog(LOG_ERR, "Error opening %s. %m\n", ioa->ioa.gen_name);
		return fd;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_IOA_DEBUG;
	cdb[1] = cmd;
	cdb[2] = (ioa_adx >> 24) & 0xff;
	cdb[3] = (ioa_adx >> 16) & 0xff;
	cdb[4] = (ioa_adx >> 8) & 0xff;
	cdb[5] = ioa_adx & 0xff;
	cdb[6] = (mask >> 24) & 0xff;
	cdb[7] = (mask >> 16) & 0xff;
	cdb[8] = (mask >> 8) & 0xff;
	cdb[9] = mask & 0xff;
	cdb[10] = (len >> 24) & 0xff;
	cdb[11] = (len >> 16) & 0xff;
	cdb[12] = (len >> 8) & 0xff;
	cdb[13] = len & 0xff;

	if (cmd == IPRDBG_WRITE) {
		rc = sg_ioctl_noretry(fd, cdb, buffer,
				      len, SG_DXFER_TO_DEV,
				      &sense_data, IPR_INTERNAL_TIMEOUT);
	} else {
		rc = sg_ioctl_noretry(fd, cdb, buffer,
				      len, SG_DXFER_FROM_DEV,
				      &sense_data, IPR_INTERNAL_TIMEOUT);
	}

	if (rc != 0)
		fprintf(stderr, "Debug IOCTL failed. %d\n", errno);
	close(fd);

	return rc;
}

static int format_flit(struct ipr_flit *flit)
{
	int num_entries = ntohl(flit->num_entries) - 1;
	struct ipr_flit_entry *flit_entry;

	signed int len;
	u8 filename[IPR_FLIT_FILENAME_LEN+1];
	u8 time[IPR_FLIT_TIMESTAMP_LEN+1];
	u8 buf1[IPR_FLIT_FILENAME_LEN+1];
	u8 buf2[IPR_FLIT_FILENAME_LEN+1];
	u8 lid_name[IPR_FLIT_FILENAME_LEN+1];
	u8 path[IPR_FLIT_FILENAME_LEN+1];
	int num_args, i;

	for (i = 0, flit_entry = flit->flit_entry;
	     (i < num_entries) && (*flit_entry->timestamp);
	     flit_entry++, i++) {
		snprintf(time, IPR_FLIT_TIMESTAMP_LEN, flit_entry->timestamp);
		time[IPR_FLIT_TIMESTAMP_LEN] = '\0';

		snprintf(filename, IPR_FLIT_FILENAME_LEN, flit_entry->filename);
		filename[IPR_FLIT_FILENAME_LEN] = '\0';

		num_args = sscanf(filename, "%184s " "%184s " "%184s "
				  "%184s ", buf1, buf2, lid_name, path);

		if (num_args != 4) {
			syslog(LOG_ERR, "Cannot parse flit\n");
			return 1;
		}

		len = strlen(path);

		iprprint("LID Name:   %s  (%8X)\n", lid_name, ntohl(flit_entry->load_name));
		iprprint("LID Path:   %s\n", path);
		iprprint("Time Stamp: %s\n", time);
		iprprint("LID Path:   %s\n", lid_name);
		iprprint("Text Segment:  Address %08X,  Length %8X\n",
			 ntohl(flit_entry->text_ptr), ntohl(flit_entry->text_len));
		iprprint("Data Segment:  Address %08X,  Length %8X\n",
			 ntohl(flit_entry->data_ptr), ntohl(flit_entry->data_len));
		iprprint("BSS Segment:   Address %08X,  Length %8X\n",
			 ntohl(flit_entry->bss_ptr), ntohl(flit_entry->bss_len));
		iprprint("\n");
	}

	return 0;
} 

static char *term;
static char *k_up;
static char *k_down;
static unsigned char hindex;
static char *history[256];

static void ipr_fgets(char *buf, int size, FILE *stream)
{
	int i, ch = 0;

	for (i = 0; ch != EOF && i < (size - 1); i++) {
		ch = fgetc(stream);
		if (ch == '\n')
			break;

		buf[i] = ch;

		if (!strcmp(buf, k_up)) {
			rewind(stdin);
			if (history[hindex]) {
				fprintf(stdout, "%s", history[--hindex]);
				strcpy(buf, history[hindex]);
				i = strlen(buf) + 1;
			}
		} else if (!strcmp(buf, k_down)) {
			rewind(stdin);
			if (history[hindex]) {
				fprintf(stdout, "%s", history[++hindex]);
				strcpy(buf, history[hindex]);
				i = strlen(buf) + 1;
			}
		}
	}

	buf[i] = '\0';

	if (history[hindex])
		free(history[hindex]);
	history[hindex] = malloc(strlen(buf) + 2);
	strcpy(history[hindex++], buf);
	strcat(buf, "\n");
}

static void dump_data(unsigned int adx, unsigned int *buf, int len)
{
	char ascii_buffer[17];
	int i, k, j;

	for (i = 0; i < (len / 4);) {
		iprprint("%08X: ", adx+(i*4));

		memset(ascii_buffer, '\0', sizeof(ascii_buffer));

		for (j = 0; i < (len / 4) && j < 4; j++, i++) {
			iprprint("%08X ", ntohl(buf[i]));
			memcpy(&ascii_buffer[j*4], &buf[i], 4);
			for (k = 0; k < 4; k++) {
				if (!isprint(ascii_buffer[(j*4)+k]))
					ascii_buffer[(j*4)+k] = '.';
			}
		}

		for (;j < 4; j++) {
			iprprint("         ");
			strncat(ascii_buffer, "....", sizeof(ascii_buffer)-1);
		}

		iprprint("   |%s|\n", ascii_buffer);
	}
}

static int mw4(struct ipr_ioa *ioa, int argc, char *argv[])
{
	unsigned int *buf;
	unsigned int adx;
	unsigned int write_buf[16];
	int rc, i;

	if ((adx % 4) != 0) {
		iprprint("Address must be 4 byte aligned\n");
		return -EINVAL;
	}

	adx = strtoul(argv[0], NULL, 16);

	/* Byte swap everything */
	for (i = 0; i < argc-2; i++)
		write_buf[i] = htonl(argv[i]);

	return debug_ioctl(ioa, IPRDBG_WRITE, adx, 0, write_buf, (num_args-2)*4);
}

static int __mr4(struct ipr_ioa *ioa, unsigned int adx, int len)
{
	unsigned int *buf;
	int rc;

	if ((adx % 4) != 0) {
		iprprint("Length must be 4 byte multiple\n");
		return -EINVAL;
	}

	buf = calloc(len/4, 4);

	if (!buf) {
		syslog(LOG_ERR, "Memory allocation error\n");
		return -ENOMEM;
	}

	rc = debug_ioctl(ioa, IPRDBG_READ, adx, 0, buf, len);

	if (!rc)
		dump_data(adx, buf, len);

	free(buf);
	return rc;
}

static int mr4(struct ipr_ioa *ioa, int argc, char *argv[])
{
	unsigned int adx;
	int len = 4;

	adx = strtoul(argv[0], NULL, 16);
	if (argc == 2)
		len = strtoul(argv[1], NULL, 16);
	return __mr4(ioa, adx, len);
}

static const struct {
	char *cmd;
	int min_args;
	int max_args;
	int (*func)(struct ipr_ioa *,int argc, char *argv[]);
} command [] = {
	{ "mr4",			1, 2, mr4 },
	{ "mw4",			2, 17, mw4 },
	{ "bm4",			1, 1, mw4 },
	{ "flit",			0, 0, flit },
};

static int main_cmd_line(int argc, char *argv[])
{
	int i;
	struct ipr_dev *dev;
	int num_args = argc - 3;

	dev = find_dev(argv[argc-1]);

	if (!dev) {
		fprintf(stderr, "Invalid IOA specified: %s\n", argv[argc-1]);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(command); i++) {
		if (strcmp(cmd, command[i].cmd) != 0)
			continue;

		if (num_args < command[i].min_args) {
			fprintf(stderr, "You must specify a device\n");
			return -EINVAL;
		}

		if (num_args > command[i].max_args) {
			fprintf(stderr, "Too many arguments specified.\n");
			return -EINVAL;
		}

		rc = command[i].func(dev->ioa, num_args, &argv[2]);
		return rc;
	}
}

static int term_init()
{
	char *term_type;
	int rc;

	term_type = getenv ("TERM");
	if (!term_type) {
		fprintf(stderr, "Cannot determine terminal type. Is TERM set?\n");
		return -EIO;
	}

	rc = tgetent (term, term_type);
	if (rc < 0) {
		fprintf(stderr, "Cannot determine terminal type\n");
		return -EIO;
	}

	k_up = tgetstr ("ku", NULL);
	k_down = tgetstr ("kd", NULL);
	return 0;
}

static struct ipr_ioa *select_ioa()
{
	int i, num_args, ioa_num = 0;
	struct ipr_ioa *ioa;
	char cmd_line[1000];

	if (num_ioas > 1) {
		printf("\nSelect adapter to debug:\n");
		i = 1;
		for_each_ioa(ioa)
			printf("%d. IBM %X: Location: %s\n",
			       i++, ioa->ccin, ioa->pci_address);

		printf("%d to quit\n", i);
		printf("\nSelection: ");
		fgets(cmd_line, 999, stdin);
		num_args = sscanf(cmd_line, "%d\n", &ioa_num);

		if (ioa_num == i)
			return NULL;
		if (num_args != 1 || ioa_num > num_ioas)
			return NULL;

		for (ioa = ipr_ioa_head, i = 1; i < ioa_num; ioa = ioa->next, i++) {}
	} else
		ioa = ipr_ioa_head;

	return ioa;
}

int main(int argc, char *argv[])
{
	int num_args, address, prev_address = 0;
	int length, prev_length = 0;
	int ioa_num = 0;
	int rc, i, j, k, bus, num_buses;
	int arg[16], write_buf[16], bus_speed;
	int *buffer;
	char cmd[100], prev_cmd[100], cmd_line[1000];
	struct ipr_ioa *ioa;
	char ascii_buffer[17];
	struct ipr_flit flit;

	if ((rc = term_init()))
		return rc;

	openlog("iprdbg",
		LOG_PERROR | /* Print error to stderr as well */
		LOG_PID |    /* Include the PID with each error */
		LOG_CONS,    /* Write to system console if there is an error
			      sending to system logger */
		LOG_USER);

	outfile = fopen(".iprdbglog", "a");
	tool_init(0);
	check_current_config(false);

	if (argc > 1)
		return main_cmd_line(argc, argv);

	ioa = select_ioa();
	if (!ioa)
		return -ENXIO;

	closelog();
	openlog("iprdbg", LOG_PID, LOG_USER);
	iprloginfo("iprdbg on %s started\n", ioa->pci_address);
	closelog();
	openlog("iprdbg", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);

	printf("\nATTENTION: This utility is for adapter developers only! "
	       "Use at your own risk!\n");
	printf("\nUse \"quit\" to exit the program.\n\n");

	memset(cmd, 0, sizeof(cmd));
	memset(prev_cmd, 0, sizeof(prev_cmd));

	while(1) {
		printf("IPRDB(%X)> ", ioa->ccin);

		memset(cmd, 0, sizeof(cmd));
		memset(cmd_line, 0, sizeof(cmd_line));
		memset(arg, 0, sizeof(arg));
		address = 0;

		ipr_fgets(cmd_line, 999, stdin);

		logtofile("\n%s\n", cmd_line);

		num_args = sscanf(cmd_line, "%80s %x %x %x %x %x %x %x %x "
				  "%x %x %x %x %x %x %x %x %x\n",
				  cmd, &address, &arg[0],
				  &arg[1], &arg[2], &arg[3],
				  &arg[4], &arg[5], &arg[6],
				  &arg[7], &arg[8], &arg[9],
				  &arg[10], &arg[11], &arg[12],
				  &arg[13], &arg[14], &arg[15]);

		if ((cmd[0] == '\0') && !strncmp("mr4", prev_cmd, 3)) {
			strcpy(cmd, prev_cmd);
			address = prev_address + prev_length;
			num_args = 3;
			arg[0] = prev_length;
		}

		if (!strcmp(cmd, "speeds")) {
			length = 64;
			buffer = calloc(length/4, 4);

			if (buffer == NULL) {
				syslog(LOG_ERR, "Memory allocation error\n");
				continue;
			}

			if ((ioa->ccin == 0x2780) ||
			    (ioa->ccin == 0x2757))
				num_buses = 4;
			else
				num_buses = 2;

			for (bus = 0, address = 0xF0016380; bus < num_buses; bus++) {
				rc = debug_ioctl(ioa, IPRDBG_READ, address, 0,
						 buffer, length);

				if (!rc) {
					iprprint("\nBus %d speeds:\n", bus);

					for (i = 0; i < 16; i++) {
						bus_speed = ntohl(buffer[i]) & 0x0000000f;

						for (j = 0; j < sizeof(bus_speeds)/sizeof(struct ipr_bus_speeds); j++) {
							if (bus_speed == bus_speeds[j].speed)
								iprprint("Target %d: %s\n", i, bus_speeds[j].description);
						}
					}
				}

				switch(bus) {
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
		} else if (!strcmp(cmd, "mr4")) {
			if (num_args < 2) {
				iprprint("Invalid syntax\n");
				continue;
			} else if (num_args == 2) {
				length = 4;
			} else if ((arg[0] % 4) != 0) {
				iprprint("Length must be 4 byte multiple\n");
				continue;
			} else
				length = arg[0];

			__mr4(ioa, address, length);

			prev_address = address;
			prev_length = length;
		} else if (!strcmp(cmd, "mw4")) {
			if (num_args < 3) {
				iprprint("Invalid syntax\n");
				continue;
			}

			__mw4(ioa, num_args, arg);
			/* Byte swap everything */
			for (i = 0; i < num_args-2; i++)
				write_buf[i] = htonl(arg[i]);

			debug_ioctl(ioa, IPRDBG_WRITE, address, 0,
				    write_buf, (num_args-2)*4);
		} else if (!strcmp(cmd, "bm4")) {
			if (num_args < 3) {
				iprprint("Invalid syntax\n");
				continue;
			}

			/* Byte swap the data */
			write_buf[0] = htonl(arg[0]);

			debug_ioctl(ioa, IPRDBG_WRITE, address, arg[1],
				    write_buf, 4);
		} else if (!strcmp(cmd, "flit")) {
			rc = debug_ioctl(ioa, IPRDBG_FLIT, 0, 0,
					 (int *)&flit, sizeof(flit));

			if (rc != 0)
				continue;

			format_flit(&flit);
		} else if (!strcmp(cmd, "help")) {
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
		} else if (!strcmp(cmd, "exit") || !strcmp(cmd, "quit") ||
			   !strcmp(cmd, "q") || !strcmp(cmd, "x")) {
			closelog();
			openlog("iprdbg", LOG_PID, LOG_USER);
			iprloginfo("iprdbg on %s exited\n", ioa->pci_address);
			closelog();
			return 0;
		} else {
			iprprint("Invalid command %s. Use \"help\" for a list "
				 "of valid commands\n", cmd);
			continue;
		}

		strcpy(prev_cmd, cmd);
	}

	return 0;
}
