/**
  * IBM IPR adapter dump daemon
  *
  * (C) Copyright 2000, 2004
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprdump.c,v 1.19 2008/11/20 01:20:20 wboyer Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/mman.h>

#define IPRDUMP_SIZE (80 * 1024 * 1024)
#define MAX_DUMP_FILES 4
#define TOOL_NAME "iprdump"
#define DUMP_PREFIX TOOL_NAME"."

char *tool_name = TOOL_NAME;

struct dump_header {
	u32 eye_catcher;
#define IPR_DUMP_EYE_CATCHER 0xC5D4E3F2
	u32 total_length;
	u32 num_elems;
	u32 first_entry_offset;
	u32 status;
} *dump_header;

struct dump_entry_header {
	u32 length;
	u32 id;
#define IPR_DUMP_TEXT_ID     3
	char data[0];
};

struct dump {
	struct dump_header hdr;
	u8 data[IPRDUMP_SIZE-sizeof(struct dump_header)];
};

static struct dump dump;
static char usr_dir[100];
static char *enable = "1\n";
static char *disable = "0\n";

/**
 * enable_dump -
 * @ioa:		ipr_ioa struct
 *
 * Returns:
 *   nothing
 **/
static void enable_dump(struct ipr_ioa *ioa)
{
	int rc;

	rc = ipr_write_host_attr(ioa, "dump", enable, strlen(enable));
	if (rc != strlen(enable)) {
		ioa_err(ioa, "Failed to enable dump. rc=%d. %m\n", rc);
		return;
	}
}

/**
 * disable_dump -
 * @ioa:		ipr_ioa struct
 *
 * Returns:
 *   nothing
 **/
static void disable_dump(struct ipr_ioa *ioa)
{
	int rc;

	rc = ipr_write_host_attr(ioa, "dump", disable, strlen(disable));
	if (rc != strlen(disable)) {
		ioa_err(ioa, "Failed to disable dump. rc=%d. %m\n", rc);
		return;
	}
}

/**
 * read_dump -
 * @ioa:		ipr_ioa struct
 *
 * Returns:
 *   number of items read
 **/
static int read_dump(struct ipr_ioa *ioa)
{
	int count = 0;
	char path[PATH_MAX];
	FILE *file;

	sprintf(path, "/sys/class/scsi_host/%s/%s", ioa->host_name, "dump");
	file = fopen(path, "r");
	count = fread(&dump, 1 , sizeof(dump), file);
	fclose(file);

	return (count < 0) ? 0: count;
}

/**
 * select_dump_file -
 * @dirent:		dirent struct
 *
 * Returns:
 *   1 if the string is found, else 0
 **/
static int select_dump_file(const struct dirent *dirent)
{
	if (strstr(dirent->d_name, DUMP_PREFIX))
		return 1;
	return 0;
}

/**
 * dump_sort -
 * @a:		void buffer
 * @b:		void buffer
 *
 * Returns:
 *
 **/
static int dump_sort(const struct dirent **dumpa, const struct dirent **dumpb)
{
	int numa, numb;

	sscanf((*dumpa)->d_name, DUMP_PREFIX"%d", &numa);
	sscanf((*dumpb)->d_name, DUMP_PREFIX"%d", &numb);

	if (numa < MAX_DUMP_FILES && numb >= (100 - MAX_DUMP_FILES))
		return 1;
	if (numb < MAX_DUMP_FILES && numa >= (100 - MAX_DUMP_FILES))
		return -1;
	return alphasort(dumpa, dumpb);
}

/**
 * cleanup_old_dumps -
 *
 * Returns:
 *   Nothing
 **/
static void cleanup_old_dumps()
{
	struct dirent **dirent;
	char fname[100];
	int rc, i;

	rc = scandir(usr_dir, &dirent, select_dump_file, dump_sort);
	if (rc > 0) {
		for (i = 0 ; i < (rc - MAX_DUMP_FILES); i++) {
			sprintf(fname, "%s%s", usr_dir, dirent[i]->d_name);
			if (remove(fname)) {
				syslog(LOG_ERR, "Delete of %s%s failed. %m\n",
				       usr_dir, dirent[i]->d_name);
			}
		}

		free(*dirent);
	}
}

/**
 * get_dump_fname -
 * @fname:		file name
 *
 * Returns:
 *   -EIO on error, else 0
 **/
static int get_dump_fname(char *fname)
{
	struct dirent **dirent;
	int rc;
	char *tmp;
	int dump_id = 0;

	cleanup_old_dumps();
	rc = scandir(usr_dir, &dirent, select_dump_file, dump_sort);
	if (rc > 0) {
		rc--;
		tmp = strstr(dirent[rc]->d_name, DUMP_PREFIX);
		if (tmp) {
			tmp += strlen(DUMP_PREFIX);
			dump_id = strtoul(tmp, NULL, 10) + 1;
			if (dump_id > 99)
				dump_id = 0;
		} else {
			free(*dirent);
			fname[0] = '\0';
			return -EIO;
		}

		free(*dirent);
	}

	sprintf(fname, "%s%02d", DUMP_PREFIX, dump_id);
	return 0;
}

/**
 * write_dump -
 * @ioa:		ipr_ioa struct
 * @count:		size to write
 *
 * Returns:
 *   Nothing
 **/
static void write_dump(struct ipr_ioa *ioa, int count)
{
	int f_dump;
	char dump_file[100], dump_path[100];

	if (get_dump_fname(dump_file))
		return;

	sprintf(dump_path, "%s%s", usr_dir, dump_file);
	f_dump = creat(dump_path, S_IRUSR);
	if (f_dump < 0) {
		syslog(LOG_ERR, "Cannot open %s. %m\n", dump_path);
		return;
	}

	write(f_dump, &dump, count);
	close(f_dump);

	ioa_err(ioa, "Dump of ipr IOA has completed to file: %s\n", dump_path);
}

/**
 * handle_signal -
 * @signal:		signal value
 *
 * Returns:
 *   Nothing
 **/
static void handle_signal(int signal)
{
	struct ipr_ioa *ioa;

	tool_init(0);
	for_each_ioa(ioa)
		disable_dump(ioa);
	exit(0);
}

/**
 * select_dump_file -
 * @ioa:		ipr_ioa struct
 *
 * Returns:
 *   Nothing
 **/
static void dump_ioa(struct ipr_ioa *ioa)
{
	int count;

	count = read_dump(ioa);
	if (count <= 0)
		return;
	write_dump(ioa, count);
	disable_dump(ioa);
	enable_dump(ioa);
}

/**
 * poll_for_dump -
 *
 * Returns:
 *   Nothing
 **/
static void poll_for_dump()
{
	struct ipr_ioa *ioa;

	tool_init(0);

	for_each_ioa(ioa)
		enable_dump(ioa);
	for_each_ioa(ioa)
		dump_ioa(ioa);
}

/**
 * kevent_handler -
 * @buf:		character buffer
 *
 * Returns:
 *   Nothing
 **/
static void kevent_handler(char *buf)
{
	struct ipr_ioa *ioa;
	int host, i;
	char *index;

	if (!strncmp(buf, "change@/class/scsi_host", 23)) {
		host = strtoul(&buf[28], NULL, 10);
	} else if (!strncmp(buf, "change@/devices/pci", 19) && strstr(buf, "scsi_host")) {
		index = strrchr(buf, '/');
		host = strtoul(index + 5, NULL, 10);
	} else
		return;

	tool_init(0);
	ioa = find_ioa(host);
	if (!ioa) {
		syslog(LOG_ERR, "Failed to find ipr ioa %d for iprdump\n", host);
		return;
	}

	enable_dump(ioa);
	dump_ioa(ioa);
}

/**
 * main -
 * @argc:		argument count
 * @argv:		argument string
 *
 * Returns:
 *   return value of handle_events() call
 **/
int main(int argc, char *argv[])
{
	struct ipr_ioa *ioa;
	int len, i;

	openlog(TOOL_NAME, LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
	strcpy(usr_dir, IPRDUMP_DIR);

	for (i = 1; i < argc; i++) {
		if (parse_option(argv[i]))
			continue;
		if (strcmp(argv[i], "-d") == 0) {
			strcpy(usr_dir,argv[++i]);
			len = strlen(usr_dir);
			if (len < sizeof(usr_dir) && usr_dir[len] != '/') {
				usr_dir[len + 1] = '/';
				usr_dir[len + 2] = '\0';
			}
		} else {
			printf("Usage: "TOOL_NAME" [options]\n");
			printf("  Options: --version    Print iprdump version\n");
			printf("           --daemon     Run as a daemon\n");
			printf("           --debug      Print extra debugging information\n");
			printf("           -d <usr_dir>\n");
			exit(1);
		}
	}

	if (daemonize)
		ipr_daemonize();

	signal(SIGINT, handle_signal);
	signal(SIGQUIT, handle_signal);
	signal(SIGTERM, handle_signal);

	tool_init(0);
	for_each_ioa(ioa)
		enable_dump(ioa);

	return handle_events(poll_for_dump, 60, kevent_handler);
}
