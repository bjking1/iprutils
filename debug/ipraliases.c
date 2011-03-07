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
 * $Header: /cvsroot/iprdd/iprutils/debug/ipraliases.c,v 1.1 2006/05/22 22:41:22 brking Exp $
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

static int ipr_get_aliases(struct ipr_dev *dev)
{
	int rc, i;
	u32 *data;
	struct ipr_res_addr_aliases aliases;

	rc = ipr_query_res_addr_aliases(dev->ioa, &(dev->res_addr[0]), &aliases);

	if (!rc) {
		data = (u32 *)&aliases;
		fprintf(stdout: "Aliases: %08X %08X %08X\n", ntohl(data[0]), ntohl(data[1]),
			ntohl(data[2]));
	}

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

	return ipr_get_aliases(dev);
}
