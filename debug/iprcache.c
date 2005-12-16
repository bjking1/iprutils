/**
  * IBM IPR adapter drive write caching utility
  *
  * (C) Copyright 2005
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/debug/iprcache.c,v 1.2 2005/12/16 17:04:41 brking Exp $
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

char *tool_name = "iprcache";

struct ipr_caching_mode_page {
	/* Mode page 0x08 */
	struct ipr_mode_page_hdr hdr;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 ic:1;
	u8 abpf:1;
	u8 cap:1;
	u8 disc:1;
	u8 size:1;
	u8 wce:1;
	u8 mf:1;
	u8 rcd:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 rcd:1;
	u8 mf:1;
	u8 wce:1;
	u8 size:1;
	u8 disc:1;
	u8 cap:1;
	u8 abpf:1;
	u8 ic:1;
#endif

	u8 data[17];
};

static int set_caching_page(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_caching_mode_page *page;
	int rc;
	int len;

	rc = ipr_mode_sense(dev, 8, &mode_pages);
	if (rc)
		return rc;

	page = (struct ipr_caching_mode_page *) (((u8 *)&mode_pages) +
						 mode_pages.hdr.block_desc_len +
						 sizeof(mode_pages.hdr));

	page->wce = 0;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	rc = ipr_mode_select(dev, &mode_pages, len);

	return rc;
}

int main(int argc, char *argv[])
{
	struct ipr_dev *dev;

	openlog("iprcache", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
	tool_init(1);
	check_current_config(false);
	dev = find_gen_dev(argv[1]);
	if (!dev)
		dev = find_blk_dev(argv[1]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", argv[1]);
		return -EINVAL;
	}

	return set_caching_page(dev);
}
