/**
  * IBM IPR adapter initialization utility
  *
  * (C) Copyright 2005
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/debug/iprluns.c,v 1.1 2005/12/16 17:04:41 brking Exp $
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

char *tool_name = "iprluns";

static int ipr_report_luns(struct ipr_dev *dev)
{
	u8 cdb[IPR_CCB_CDB_LEN];
	int fd, rc, i, len;
	char buf[100];
	char *name = dev->gen_name;
	struct sense_data_t sense_data;

	if (strlen(dev->dev_name))
		name = dev->dev_name;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = REPORT_LUNS;
	cdb[9] = 100;

	rc = sg_ioctl(fd, cdb, buf,
		      100, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	close(fd);

	if (rc)
		scsi_err(dev, "Report LUNS failed. %m\n");

	len = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

	syslog(LOG_ERR, "Total report luns len: %d\n", len);

	return rc;
}

int main(int argc, char *argv[])
{
	struct ipr_dev *dev;

	tool_init(1);
	check_current_config(false);
	dev = find_blk_dev(argv[1]);
	if (!dev)
		dev = find_gen_dev(argv[1]);
	if (!dev) {
		fprintf(stderr, "Cannot find device: %s\n", argv[1]);
		return -EINVAL;
	}

	return ipr_report_luns(dev);
}
