/**
  * IBM IPR adapter initialization utility
  *
  * (C) Copyright 2004
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprinit.c,v 1.19 2005/02/23 20:57:11 bjking1 Exp $
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

	tool_init();
	check_current_config(false);
	for_each_ioa(ioa)
		ipr_init_ioa(ioa);
}

static void scsi_dev_kevent(char *buf, struct ipr_dev *(*find_dev)(char *))
{
	struct ipr_dev *dev;
	char *name;

	name = strrchr(buf, '/');
	if (!name) {
		syslog_dbg("Failed to handle %s kevent\n", buf);
		return;
	}

	name++;
	tool_init();
	check_current_config(false);
	dev = find_dev(name);

	if (!dev) {
		syslog_dbg("Failed to find ipr dev %s for iprinit\n", name);
		return;
	}

	ipr_init_dev(dev);
}

static void scsi_host_kevent(char *buf)
{
	struct ipr_ioa *ioa;
	char *c;
	int host;

	c = strrchr(buf, '/');
	if (!c) {
		syslog_dbg("Failed to handle %s kevent\n", buf);
		return;
	}

	c += strlen("/host");

	host = strtoul(c, NULL, 10);
	tool_init();
	check_current_config(false);
	ioa = find_ioa(host);

	if (!ioa) {
		syslog_dbg("Failed to find ipr ioa %d for iprinit\n", host);
		return;
	}

	ipr_init_ioa(ioa);
}

static void kevent_handler(char *buf)
{
	polling_mode = 0;

	if (!strncmp(buf, "change@/class/scsi_host", 23))
		scsi_host_kevent(buf);
	else if (!strncmp(buf, "add@/class/scsi_generic", 23))
		scsi_dev_kevent(buf, find_gen_dev);
	else if (!strncmp(buf, "add@/block/sd", 13))
		scsi_dev_kevent(buf, find_blk_dev);
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
