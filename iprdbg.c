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
 * $Header: /cvsroot/iprdd/iprutils/iprdbg.c,v 1.26 2006/09/08 16:26:01 brking Exp $
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
#include <time.h>

#define IPR_MAX_FLIT_ENTRIES 59
#define IPR_FLIT_TIMESTAMP_LEN 12
#define IPR_FLIT_FILENAME_LEN 184
#define IPRDBG_CONF "/etc/iprdbg.conf"
#define IPRDBG_LOG "/var/log/iprdbg"
#define SEPARATOR "--------------------------------------------------------------------------------"

char *tool_name = "iprdbg";

enum iprdbg_cmd {
	IPRDBG_READ = IPR_IOA_DEBUG_READ_IOA_MEM,
	IPRDBG_WRITE = IPR_IOA_DEBUG_WRITE_IOA_MEM,
	IPRDBG_FLIT = IPR_IOA_DEBUG_READ_FLIT,
	IPRDBG_EDF = IPR_IOA_DEBUG_ENABLE_DBG_FUNC,
	IPRDBG_DDF = IPR_IOA_DEBUG_DISABLE_DBG_FUNC
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

struct dbg_macro {
	char *cmd;
};

static struct dbg_macro *macro;
static int num_macros;
static unsigned int last_adx;
static unsigned int last_len;
static unsigned int last_data;
static char time_str[100];

static int getcurtime(char *buf, int max)
{
	time_t cur_time, rc;
	struct tm *cur_tm;
	int len;

	buf[0] = '\0';
	rc = time(&cur_time);
	if (rc == ((time_t)-1))
		return -EIO;

	cur_tm = localtime(&cur_time);
	if (!cur_tm)
		return -EIO;
	len = strftime(buf, max, "%b %d %T", cur_tm);
	return len;
}

#define __logtofile(...) {if (outfile) {fprintf(outfile, __VA_ARGS__);}}
#define logtofile(fmt, ...) \
do { \
if (outfile) { \
     getcurtime(time_str, sizeof(time_str)); \
     __logtofile("%s: "fmt, time_str, ##__VA_ARGS__); \
} \
} while (0)

#define iprprint(...) {printf(__VA_ARGS__); logtofile(__VA_ARGS__);}
#define __iprprint(...) {printf(__VA_ARGS__); __logtofile(__VA_ARGS__);}
#define ipr_err(...) {fprintf(stderr, __VA_ARGS__); logtofile(__VA_ARGS__);}
#define iprloginfo(...) {syslog(LOG_INFO, __VA_ARGS__); logtofile(__VA_ARGS__);}

static FILE *outfile;

static int debug_ioctl(struct ipr_ioa *ioa, enum iprdbg_cmd cmd, int ioa_adx, int mask,
		       unsigned int *buffer, int len)
{
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc, fd;
	u32 direction = SG_DXFER_NONE;

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

	if (cmd == IPRDBG_WRITE)
		direction = SG_DXFER_TO_DEV;
	else if (len > 0)
		direction = SG_DXFER_FROM_DEV;

	rc = sg_ioctl_noretry(fd, cdb, buffer,
			      len, direction,
			      &sense_data, IPR_INTERNAL_TIMEOUT);

	if (rc != 0)
		ipr_err("Debug IOCTL failed. %d\n", errno);
	close(fd);

	return rc;
}

static int format_flit(struct ipr_flit *flit)
{
	int num_entries = ntohl(flit->num_entries) - 1;
	struct ipr_flit_entry *flit_entry;

	signed int len;
	char filename[IPR_FLIT_FILENAME_LEN+1];
	char time[IPR_FLIT_TIMESTAMP_LEN+1];
	char buf1[IPR_FLIT_FILENAME_LEN+1];
	char buf2[IPR_FLIT_FILENAME_LEN+1];
	char lid_name[IPR_FLIT_FILENAME_LEN+1];
	char path[IPR_FLIT_FILENAME_LEN+1];
	int num_args, i;

	for (i = 0, flit_entry = flit->flit_entry;
	     (i < num_entries) && (*flit_entry->timestamp);
	     flit_entry++, i++) {
		snprintf(time, IPR_FLIT_TIMESTAMP_LEN, "%s", (char *)flit_entry->timestamp);
		time[IPR_FLIT_TIMESTAMP_LEN] = '\0';

		snprintf(filename, IPR_FLIT_FILENAME_LEN, "%s", (char *)flit_entry->filename);
		filename[IPR_FLIT_FILENAME_LEN] = '\0';

		num_args = sscanf(filename, "%184s " "%184s " "%184s "
				  "%184s ", buf1, buf2, lid_name, path);

		if (num_args != 4) {
			ipr_err("Cannot parse flit\n");
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
		__iprprint("%s\n", SEPARATOR);
	}

	return 0;
} 

static void ipr_fgets(char *buf, int size, FILE *stream)
{
	int i, ch = 0;

	for (i = 0; ch != EOF && i < (size - 1); i++) {
		ch = fgetc(stream);
		if (ch == '\n')
			break;

		buf[i] = ch;
	}

	buf[i] = '\0';
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
			__iprprint("%08X ", ntohl(buf[i]));
			memcpy(&ascii_buffer[j*4], &buf[i], 4);
			for (k = 0; k < 4; k++) {
				if (!isprint(ascii_buffer[(j*4)+k]))
					ascii_buffer[(j*4)+k] = '.';
			}
		}

		for (;j < 4; j++) {
			__iprprint("         ");
			strncat(ascii_buffer, "....", sizeof(ascii_buffer)-1);
		}

		__iprprint("   |%s|\n", ascii_buffer);
	}
}

static int flit(struct ipr_ioa *ioa, int argc, char *argv[])
{
	struct ipr_flit flit;
	int rc;

	rc = debug_ioctl(ioa, IPRDBG_FLIT, 0, 0, (unsigned int *)&flit, sizeof(flit));

	if (!rc)
		format_flit(&flit);

	return rc;
}

static int speeds(struct ipr_ioa *ioa, int argc, char *argv[])
{
	unsigned int length = 64;
	unsigned int *buffer = calloc(length/4, 4);
	unsigned int adx, bus_speed;
	int num_buses, bus, i, j, rc;

	if (!buffer) {
		ipr_err("Memory allocation error\n");
		return -ENOMEM;
	}

	if ((ioa->ccin == 0x2780) ||
	    (ioa->ccin == 0x2757))
		num_buses = 4;
	else
		num_buses = 2;

	for (bus = 0, adx = 0xF0016380; bus < num_buses; bus++) {
		rc = debug_ioctl(ioa, IPRDBG_READ, adx, 0, buffer, length);

		if (!rc) {
			__iprprint("%s\n", SEPARATOR);
			iprprint("Bus %d speeds:\n", bus);

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
			adx = 0xF001e380;
			break;
		case 1:
			adx = 0xF4016380;
			break;
		case 2:
			adx = 0xF401E380;
			break;
		};
	}

	free(buffer);
	return 0;
}

static int eddf(struct ipr_ioa *ioa, enum iprdbg_cmd cmd, int argc, char *argv[])
{
	unsigned int parm1 = strtoul(argv[0], NULL, 16);
	unsigned int parm2 = 0;

	if (argc == 2)
		parm2 = strtoul(argv[1], NULL, 16);
	return debug_ioctl(ioa, cmd, parm1, parm2, NULL, 0);
}

static int edf(struct ipr_ioa *ioa, int argc, char *argv[])
{
	return eddf(ioa, IPRDBG_EDF, argc, argv);
}

static int ddf(struct ipr_ioa *ioa, int argc, char *argv[])
{
	return eddf(ioa, IPRDBG_DDF, argc, argv);
}

static int raw_cmd(struct ipr_ioa *ioa, int argc, char *argv[])
{
	int i, rc;
	unsigned int parm[4] = { 0 };
	unsigned int *buf = NULL;

	for (i = 0; i < argc; i++)
		parm[i] = strtoul(argv[i], NULL, 16);

	if (argc == 4 && parm[3])
		buf = calloc(parm[3]/4, 4);

	rc = debug_ioctl(ioa, parm[0], parm[1], parm[2], buf, parm[3]);

	if (!rc && buf) {
		dump_data(0, buf, parm[3]);
		last_data = ntohl(buf[0]);
	}

	free(buf);
	return rc;
}

static unsigned int get_adx(char *arg)
{
	if (arg[0] == '@') {
		if (arg[1] == '+')
			return last_data + strtoul(&arg[2], NULL, 16);
		else if (arg[1] == '-')
			return last_data - strtoul(&arg[2], NULL, 16);
		else
			return last_data;
	}

	return strtoul(arg, NULL, 16);
}

static int bm4(struct ipr_ioa *ioa, int argc, char *argv[])
{
	unsigned int adx;
	unsigned int write_buf = htonl(strtoul(argv[1], NULL, 16));
	unsigned int mask = strtoul(argv[2], NULL, 16);

	adx = get_adx(argv[0]);

	if ((adx % 4) != 0) {
		ipr_err("Address must be 4 byte aligned\n");
		return -EINVAL;
	}

	return debug_ioctl(ioa, IPRDBG_WRITE, adx, mask, &write_buf, 4);
}

static int mw4(struct ipr_ioa *ioa, int argc, char *argv[])
{
	int i;
	unsigned int write_buf[16];
	unsigned int adx;

	adx = get_adx(argv[0]);

	if ((adx % 4) != 0) {
		iprprint("Address must be 4 byte aligned\n");
		return -EINVAL;
	}

	for (i = 0; i < (argc-1); i++)
		write_buf[i] = htonl(strtoul(argv[i+1], NULL, 16));

	return debug_ioctl(ioa, IPRDBG_WRITE, adx, 0, write_buf, (argc-1)*4);
}

static int __mr4(struct ipr_ioa *ioa, unsigned int adx, int len)
{
	unsigned int *buf;
	int rc;

	if ((adx % 4) != 0) {
		ipr_err("Address must be 4 byte aligned\n");
		return -EINVAL;
	}

	if ((len % 4) != 0) {
		ipr_err("Length must be a 4 byte multiple\n");
		return -EINVAL;
	}

	buf = calloc(len/4, 4);

	if (!buf) {
		ipr_err("Memory allocation error\n");
		return -ENOMEM;
	}

	last_len = len;
	last_adx = adx;
	rc = debug_ioctl(ioa, IPRDBG_READ, adx, 0, buf, len);

	if (!rc) {
		dump_data(adx, buf, len);
		last_data = ntohl(buf[0]);
	}

	free(buf);
	return rc;
}

static int mr4(struct ipr_ioa *ioa, int argc, char *argv[])
{
	unsigned int adx;
	int len = 4;

	if (argc == 0) {
		if (!last_len) {
			ipr_err("Not enough parameters specified\n");
			return -EINVAL;
		}
		adx = last_adx + last_len;
		len = last_len;
	} else {
		adx = get_adx(argv[0]);

		if (argc == 2)
			len = strtoul(argv[1], NULL, 16);
	}

	return __mr4(ioa, adx, len);
}

static int help(struct ipr_ioa *ioa, int argc, char *argv[])
{
	printf("\n");
	printf("mr4 address length                  "
	       "- Read memory\n");
	printf("mw4 address data                    "
	       "- Write memory\n");
	printf("bm4 address data preserve_mask      "
	       "- Modify bits set to 0 in preserve_mask\n");
	printf("edf parm1 parm2                     "
	       "- Enable debug function\n");
	printf("ddf parm1 parm2                     "
	       "- Disable debug function\n");
	printf("raw cmd [parm1] [parm2] [length]    "
	       "- Manually execute IOA debug command\n");
	printf("speeds                              "
	       "- Current bus speeds for each device\n");
	printf("flit                                "
	       "- Format the flit\n");
	printf("macros                              "
	       "- List currently loaded macros\n");
	printf("exit                                "
	       "- Exit the program\n\n");
	return 0;
}

static int quit(struct ipr_ioa *ioa, int argc, char *argv[])
{
	closelog();
	openlog("iprdbg", LOG_PID, LOG_USER);
	iprloginfo("iprdbg on %s exited\n", ioa->pci_address);
	closelog();
	exit(0);
	return 0;
}

static int macros(struct ipr_ioa *ioa, int argc, char *argv[])
{
	int i;

	if (num_macros == 0)
		printf("No macros loaded. Place macros in "IPRDBG_CONF"\n");

	for (i = 0; i < num_macros; i++)
		printf("%s\n", macro[i].cmd);

	return 0;
}

static const struct {
	char *cmd;
	int min_args;
	int max_args;
	int (*func)(struct ipr_ioa *,int argc, char *argv[]);
} command [] = {
	{ "mr4",		0, 2,		mr4 },
	{ "mw4",		2, 17,	mw4 },
	{ "bm4",		3, 3,		bm4 },
	{ "edf",		1, 2,		edf },
	{ "ddf",		1, 2,		ddf },
	{ "raw",		1, 4,		raw_cmd },
	{ "flit",		0, 0,		flit },
	{ "speeds",		0, 0,		speeds },
	{ "help",		0, 0,		help },
	{ "?",		0, 0,		help },
	{ "quit",		0, 0,		quit },
	{ "q",		0, 0,		quit },
	{ "exit",		0, 0,		quit },
	{ "x",		0, 0,		quit },
	{ "macros",		0, 0,		macros },
};

static int last_cmd = -1;

static int exec_cmd(struct ipr_ioa *ioa, int cmd, int argc, char *argv[])
{
	int num_args = argc - 1;

	if (num_args < command[cmd].min_args) {
		ipr_err("Not enough arguments specified.\n");
		return -EINVAL;
	}

	if (num_args > command[cmd].max_args) {
		ipr_err("Too many arguments specified.\n");
		return -EINVAL;
	}

	if (last_cmd != cmd)
		last_len = 0;
	last_cmd = cmd;
	return command[cmd].func(ioa, num_args, &argv[1]);
}

static int cmd_to_args(char *cmd, int *rargc, char ***rargv)
{
	char *p;
	char **argv = NULL;
	int len = 0;
	int total_len = strlen(cmd);

	if (cmd[total_len-1] == '\n') {
		cmd[total_len-1] = '\0';
		total_len--;
	}

	if (*cmd == '\0')
		return 0;

	for (p = strrchr(cmd, ' '); p; p = strrchr(cmd, ' '))
		*p = '\0';

	for (p = strrchr(cmd, '\t'); p; p = strrchr(cmd, '\t'))
		*p = '\0';

	for (p = cmd, len = 0; p < cmd + total_len; p += (strlen(p) + 1), len++) {
		if (strlen(p) == 0) {
			len--;
			continue;
		}
		argv = realloc(argv, sizeof(char *)*(len+1));
		argv[len] = p;
	}

	*rargv = argv;
	*rargc = len;
	return len;
}

static int exec_macro(struct ipr_ioa *ioa, char *cmd);

static int __parse_cmd(struct ipr_ioa *ioa, int argc, char *argv[])
{
	int i;

	for (i = 0; i < ARRAY_SIZE(command); i++) {
		if (strcasecmp(argv[0], command[i].cmd) != 0)
			continue;

		return exec_cmd(ioa, i, argc, argv);
	}

	for (i = 0; i < num_macros; i++) {
		if (strncasecmp(argv[0], macro[i].cmd, strlen(argv[0])) != 0)
			continue;

		return exec_macro(ioa, macro[i].cmd);
	}

	iprprint("Invalid command %s. Use \"help\" for a list "
		 "of valid commands\n", argv[0]);

	return -EINVAL;
}

static int parse_cmd(struct ipr_ioa *ioa, int argc, char *argv[])
{
	int i, rc = 0;
	int index = 0;
	char *end;

	for (i = 0; i < argc; i++) {
		end = strchr(argv[i], ':');
		if (!end)
			end = strchr(argv[i], ';');
		if (!end)
			continue;

		if (strlen(argv[i]) == 1) {
			rc = __parse_cmd(ioa, i - index, &argv[index]);
		} else {
			*end = '\0';
			rc = __parse_cmd(ioa, (i + 1) - index, &argv[index]);
		}

		index = i + 1;

		if (rc)
			return rc;
	}

	return __parse_cmd(ioa, i - index, &argv[index]);
}

static int exec_macro(struct ipr_ioa *ioa, char *cmd)
{
	int argc;
	char **argv = NULL;
	char *copy = malloc(strlen(cmd) + 1);
	int rc = -EINVAL;

	if (!copy)
		return -ENOMEM;

	strcpy(copy, cmd);

	if (!cmd_to_args(copy, &argc, &argv))
		goto out_free_argv;

	rc = parse_cmd(ioa, argc - 1, &argv[1]);

	free(copy);
out_free_argv:
	free(argv);
	return rc;
}

static int main_cmd_line(int argc, char *argv[])
{
	struct ipr_dev *dev;

	if (argc < 3) {
		ipr_err("Invalid option\n");
		return -EINVAL;
	}

	dev = find_dev(argv[argc-1]);

	if (!dev) {
		ipr_err("Invalid IOA specified: %s\n", argv[argc-1]);
		return -EINVAL;
	}

	return parse_cmd(dev->ioa, argc-2, &argv[1]);
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
			printf("%d. IBM %X: Location: %s %s\n",
			       i++, ioa->ccin, ioa->pci_address, ioa->ioa.gen_name);

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

static int exec_shell_cmd(struct ipr_ioa *ioa, char *cmd)
{
	char **argv = NULL;
	int argc, rc;

	if (!cmd_to_args(cmd, &argc, &argv))
		return mr4(ioa, 0, NULL);

	rc = parse_cmd(ioa, argc, argv);
	free(argv);
	return rc;
}

static void add_new_macro(char *cmd)
{
	char *buf = malloc(strlen(cmd) + 1);

	if (!buf)
		return;

	macro = realloc(macro, (num_macros + 1) * sizeof(struct dbg_macro));

	if (!macro) {
		free(buf);
		return;
	}

	strcpy(buf, cmd);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	macro[num_macros++].cmd = buf;
}

static void load_config()
{
	char buf[2000];
	FILE *config = fopen(IPRDBG_CONF, "r");

	if (!config)
		return;

	while (!feof(config)) {
		if (!fgets(buf, sizeof(buf), config))
			break;
		if (buf[0] == '#')
			continue;
		add_new_macro(buf);
	}

	fclose(config);
}

int main(int argc, char *argv[])
{
	char cmd_line[1000];
	struct ipr_ioa *ioa;

	openlog("iprdbg",
		LOG_PERROR | /* Print error to stderr as well */
		LOG_PID |    /* Include the PID with each error */
		LOG_CONS,    /* Write to system console if there is an error
			      sending to system logger */
		LOG_USER);

	outfile = fopen(IPRDBG_LOG, "a");
	if (!outfile)
		outfile = fopen(".iprdbglog", "a");
	tool_init(0);
	check_current_config(false);

	load_config();
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

	while(1) {
		printf("IPRDB(%X)> ", ioa->ccin);
		ipr_fgets(cmd_line, 999, stdin);
		__logtofile("%s\n", SEPARATOR);
		logtofile("%s", cmd_line);
		exec_shell_cmd(ioa, cmd_line);
	}

	return 0;
}
