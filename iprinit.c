/**
  * IBM IPR adapter initialization utility
  *
  * (C) Copyright 2004
  * International Business Machines Corporation and others.
  * All Rights Reserved.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprinit.c,v 1.12 2004/03/04 22:52:06 bjking1 Exp $
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

	openlog("iprinit", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);

	if (argc > 1) {
		if (strcmp(argv[1], "--version") == 0) {
			printf("iprinit: %s\n", IPR_VERSION_STR);
			return 0;
		} else if (strcmp(argv[1], "--daemon") == 0) {
			daemonize = 1;
		} else {
			printf("Usage: iprinit [options]\n");
			printf("  Options: --version    Print iprinit version\n");
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
				if (fork())
					return 0;
			}
			sleep(60);
		}
	} while(daemonize);

	return 0;
}
