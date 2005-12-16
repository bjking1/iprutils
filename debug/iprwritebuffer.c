/**
  * IBM IPR adapter low level drive microcode update utility
  *
  * (C) Copyright 2005
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/debug/iprwritebuffer.c,v 1.2 2005/12/16 17:04:41 brking Exp $
 */

#include <unistd.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>
#include <sys/stat.h>

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/mman.h>

char *tool_name = "iprwritebuffer";

static int ipr_update_ucode(struct ipr_dev *dev, char *fname)
{
	struct stat ucode_stats;
	int fd, rc;
	char *buffer;

	fd = open(fname, O_RDONLY);

	if (fd < 0) {
		syslog_dbg("Could not open firmware file %s.\n", fname);
		return fd;
	}

	rc = fstat(fd, &ucode_stats);

	if (rc != 0) {
		syslog(LOG_ERR, "Failed to stat firmware file: %s.\n", fname);
		close(fd);
		return rc;
	}

	buffer = mmap(NULL, ucode_stats.st_size,
		      PROT_READ, MAP_SHARED, fd, 0);

	rc = ipr_write_buffer(dev, buffer, ucode_stats.st_size);
	ipr_init_dev(dev);
	close(fd);

	return rc;
}

int main(int argc, char *argv[])
{
	struct ipr_dev *dev;

	if (argc != 3) {
		fprintf(stderr, "Usage iprwritebuffer /dev/sgX ucode_image.bin\n");
		return -EINVAL;
	}

	tool_init(1);
	check_current_config(false);
	dev = find_blk_dev(argv[1]);
	if (!dev)
		dev = find_gen_dev(argv[1]);
	if (!dev) {
		fprintf(stderr, "Cannot find device: %s\n", argv[1]);
		return -EINVAL;
	}

	return ipr_update_ucode(dev, argv[2]);
}
