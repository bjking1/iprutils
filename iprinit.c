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

static int init_all()
{
	struct ipr_ioa *ioa;
	int rc = tool_init(1);

	if (rc && rc != -ENXIO) {
		if (!daemonize)
			syslog(LOG_ERR, "Error initializing adapters. Perhaps run with sudo?\n");
	}
	check_current_config(false);
	for_each_ioa(ioa) {
		if (!strlen(ioa->ioa.gen_name))
			rc |= -EAGAIN;
		ipr_init_ioa(ioa);
	}
	return rc;
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
	int i, rc, delay_secs, wait;

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

	rc = check_sg_module();

	if (daemonize) {
		ipr_daemonize();

		if (!rc)
			rc = init_all();

		if (rc) {
			delay_secs = 2;
			sleep(delay_secs);

			for (wait = 0; rc && wait < 300; wait += delay_secs) {
				rc = check_sg_module();
				sleep(delay_secs);
				if (!rc)
					rc = init_all();
			}

			if (rc)
				syslog(LOG_ERR, "Timeout reached. Ensure the sg module is loaded, then "
				       "run iprinit manually to ensure all "
				       "ipr RAID adapters are running optimally\n");
		}
		return handle_events(poll_ioas, 60, kevent_handler);
	} else if (!rc)
		rc = init_all();

	if (rc)
		syslog(LOG_ERR, "iprinit failed. Ensure the sg module is loaded, then "
		       "run iprinit again to ensure all "
		       "ipr RAID adapters are running optimally\n");

	return rc;
}
