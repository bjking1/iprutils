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
 * $Header: /cvsroot/iprdd/iprutils/iprinit.c,v 1.17.2.1 2004/10/25 22:31:43 bjking1 Exp $
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

int main(int argc, char *argv[])
{
	struct ipr_ioa *ioa;
	int daemonize = 0;
	int daemonized = 0;
	int i;

	ipr_sg_required = 1;

	openlog("iprinit", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--version") == 0) {
			printf("iprinit: %s\n", IPR_VERSION_STR);
			return 0;
		} else if (strcmp(argv[i], "--daemon") == 0) {
			daemonize = 1;
		} else if (strcmp(argv[i], "--debug") == 0) {
			ipr_debug = 1;
		} else if (strcmp(argv[i], "--force") == 0) {
			ipr_force = 1;
		} else {
			printf("Usage: iprinit [options]\n");
			printf("  Options: --version [--daemon]   Print iprinit version\n");
			return -EINVAL;
		}
	}

	do {
		tool_init("iprinit");
		check_current_config(false);
		for (ioa = ipr_ioa_head; ioa; ioa = ioa->next)
			ipr_init_ioa(ioa);
		runtime = 1;
		if (daemonize) {
			if (!daemonized) {
				daemonized = 1;
				ipr_daemonize();
			}
			sleep(60);
		}
	} while(daemonize);

	return 0;
}
