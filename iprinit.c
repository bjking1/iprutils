/**
  * IBM IPR adapter initialization utility
  *
  * (C) Copyright 2004, 2008
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprinit.c,v 1.22 2008/11/20 01:20:20 wboyer Exp $
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

char *tool_name = "iprinit";

static void init_all()
{
	struct ipr_ioa *ioa;
	int rc = tool_init(1);

	if (rc && rc != -ENXIO) {
		if (daemonize)
			syslog(LOG_ERR, "Run iprinit manually to ensure all ipr RAID adapters are running optimally.\n");
		else
			syslog(LOG_ERR, "Error initializing adapters. Perhaps run with sudo?\n");
	}
	check_current_config(false);
	for_each_ioa(ioa)
		ipr_init_ioa(ioa);
}

static void kevent_handler(char *buf)
{
	polling_mode = 0;

	if (!strncmp(buf, "change@/class/scsi_host", 23) ||
	    (!strncmp(buf, "change@/devices/pci", 19) && strstr(buf, "scsi_host")))
		scsi_host_kevent(buf, ipr_init_ioa);
	else if (!strncmp(buf, "add@/class/scsi_generic", 23) ||
		 (!strncmp(buf, "add@/devices/pci", 16) && strstr(buf, "scsi_generic")))
		scsi_dev_kevent(buf, find_gen_dev, ipr_init_dev);
	else if (!strncmp(buf, "add@/block/sd", 13))
		scsi_dev_kevent(buf, find_blk_dev, ipr_init_dev);
}

static void poll_ioas()
{
	polling_mode = 1;
	init_all();
}

int main(int argc, char *argv[])
{
	int i;

	ipr_sg_required = 1;

	openlog("iprinit", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);

	for (i = 1; i < argc; i++) {
		if (parse_option(argv[i]))
			continue;
		else {
			printf("Usage: iprinit [options]\n");
			printf("  Options: --version [--daemon]   Print iprinit version\n");
			return -EINVAL;
		}
	}

	init_all();

	if (daemonize) {
		ipr_daemonize();
		return handle_events(poll_ioas, 60, kevent_handler);
	}

	return 0;
}
