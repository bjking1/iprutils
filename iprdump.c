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
 * $Header: /cvsroot/iprdd/iprutils/iprdump.c,v 1.13 2004/12/10 20:22:43 bjking1 Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/mman.h>

#define IPRDUMP_SIZE (8 * 1024 * 1024)
#define IPRDUMP_DIR "/var/log/"
#define MAX_DUMP_FILES 4
#define DUMP_PREFIX "iprdump."

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

static void enable_dump(struct ipr_ioa *ioa)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;
	int rc, fd;

	class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
	if (!class_device) {
		ioa_err(ioa, "Failed to open class device. %m\n");
		return;
	}

	attr = sysfs_get_classdev_attr(class_device, "dump");
	if (!attr) {
		ioa_dbg(ioa, "Failed to get class attribute. %m\n");
		sysfs_close_class_device(class_device);
		return;
	}

	fd = open(attr->path, O_RDWR);
	if (fd < 0) {
		if (errno != ENOENT)
			ioa_err(ioa, "Failed to open dump attribute. %m\n");
		sysfs_close_class_device(class_device);
		return;
	}

	rc = write(fd, enable, strlen(enable));
	if (rc != strlen(enable)) {
		ioa_err(ioa, "Failed to enable dump. rc=%d. %m\n", rc);
		close(fd);
		sysfs_close_class_device(class_device);
		return;
	}

	close(fd);
	sysfs_close_class_device(class_device);
}

static void disable_dump(struct ipr_ioa *ioa)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;
	int rc, fd;

	class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
	if (!class_device) {
		ioa_err(ioa, "Failed to open class device. %m\n");
		return;
	}

	attr = sysfs_get_classdev_attr(class_device, "dump");
	if (!attr) {
		ioa_dbg(ioa, "Failed to get class attribute. %m\n");
		sysfs_close_class_device(class_device);
		return;
	}

	fd = open(attr->path, O_RDWR);
	if (fd < 0) {
		ioa_err(ioa, "Failed to open dump attribute. %m\n");
		sysfs_close_class_device(class_device);
		return;
	}

	rc = write(fd, disable, strlen(disable));
	if (rc != strlen(disable)) {
		ioa_err(ioa, "Failed to disable dump. rc=%d. %m\n", rc);
		sysfs_close_class_device(class_device);
		return;
	}

	sysfs_close_class_device(class_device);
}

static int read_dump(struct ipr_ioa *ioa)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;
	int count = 0;
	FILE *file;

	class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
	if (!class_device) {
		ioa_err(ioa, "Failed to open class device. %m\n");
		return -EIO;
	}

	attr = sysfs_get_classdev_attr(class_device, "dump");
	if (!attr) {
		ioa_dbg(ioa, "Failed to open dump attribute. %m\n");
		sysfs_close_class_device(class_device);
		return -EIO;

	}

	file = fopen(attr->path, "r");
	if (!file) {
		ioa_err(ioa, "Failed to open sysfs dump file. %m\n");
		sysfs_close_class_device(class_device);
		return -EIO;
	}

	count = fread(&dump, 1, sizeof(dump), file);
	fclose(file);
	sysfs_close_class_device(class_device);
	return count;
}

static int select_dump_file(const struct dirent *dirent)
{
	if (strstr(dirent->d_name, "ipr"))
		return 1;
	return 0;
}

static void cleanup_old_dumps()
{
	struct dirent **dirent;
	char fname[100];
	int rc, i;

	rc = scandir(usr_dir, &dirent, select_dump_file, alphasort);
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

static int get_dump_fname(char *fname)
{
	struct dirent **dirent;
	int rc;
	char *tmp;
	int dump_id = 0;

	cleanup_old_dumps();
	rc = scandir(usr_dir, &dirent, select_dump_file, alphasort);
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

static void handle_signal(int signal)
{
	struct ipr_ioa *ioa;

	tool_init("iprdump");
	for_each_ioa(ioa)
		disable_dump(ioa);
	exit(0);
}

int main(int argc, char *argv[])
{
	struct ipr_ioa *ioa;
	int count, len, i;

	openlog("iprdump", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
	strcpy(usr_dir, IPRDUMP_DIR);

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--version") == 0) {
			printf("iprdump: %s\n", IPR_VERSION_STR);
			exit(1);
		} else if (strcmp(argv[i], "--debug") == 0) {
			ipr_debug = 1;
		} else if (strcmp(argv[i], "-d") == 0) {
			strcpy(usr_dir,argv[++i]);
			len = strlen(usr_dir);
			if (len < sizeof(usr_dir) && usr_dir[len] != '/') {
				usr_dir[len + 1] = '/';
				usr_dir[len + 2] = '\0';
			}
		} else {
			printf("Usage: iprdump [options]\n");
			printf("  Options: --version    Print iprdump version\n");
			printf("           --debug      Print extra debugging information\n");
			printf("           -d <usr_dir>\n");
			exit(1);
		}
	}

	ipr_daemonize();

	signal(SIGINT, handle_signal);
	signal(SIGQUIT, handle_signal);
	signal(SIGTERM, handle_signal);

	while (1) {
		tool_init("iprdump");

		for_each_ioa(ioa)
			enable_dump(ioa);

		for_each_ioa(ioa) {
			count = read_dump(ioa);
			if (count <= 0)
				continue;
			write_dump(ioa, count);
			disable_dump(ioa);
			enable_dump(ioa);
		}
		sleep(60);
	};

	return 0;
}


