/**
  * IBM IPR adapter shutdown utility
  *
  * (C) Copyright 2005
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/debug/iprshutdown.c,v 1.2 2005/12/16 17:04:41 brking Exp $
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

char *tool_name = "iprshutdown";

static int ipr_ioa_flush(struct ipr_ioa *ioa)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = 0xF7;
	cdb[1] = 0x40;

	rc = sg_ioctl(fd, cdb, ioa->qac_data,
		      0, SG_DXFER_TO_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, "IOA Shutdown", rc);

	close(fd);
	return rc;
}

int main(int argc, char *argv[])
{
	struct ipr_ioa *ioa;

	tool_init(1);
	check_current_config(false);
	for_each_ioa(ioa) {
		if (!strcmp(ioa->ioa.gen_name, argv[1])) {
			ipr_ioa_flush(ioa);
			return 0;
		}
	}

	return -EINVAL;
}
