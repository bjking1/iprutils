/**
  * IBM IPR adapter initialization utility
  *
  * (C) Copyright 2004
  * International Business Machines Corporation and others.
  * All Rights Reserved.
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprinit.c,v 1.10 2004/02/27 20:21:38 bjking1 Exp $
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

int runtime;

#define GPDD_TCQ_DEPTH 64
#define AF_DISK_TCQ_DEPTH 64

static int mode_select(struct ipr_dev *dev, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = MODE_SELECT;
	cdb[1] = 0x11;
	cdb[4] = length;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_INTERNAL_TIMEOUT);

	if (rc)
		scsi_cmd_err(dev, &sense_data, "Mode Select", rc);

	close(fd);
	return rc;
}

static int setup_page0x01(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages, ch_mode_pages;
	struct ipr_rw_err_mode_page *page, *ch_page;
	int len;

	memset(&mode_pages, 0, sizeof(mode_pages));
	memset(&ch_mode_pages, 0, sizeof(ch_mode_pages));

	if (ipr_mode_sense(dev, 0x01, &mode_pages))
		return -EIO;
	if (ipr_mode_sense(dev, 0x41, &ch_mode_pages))
		return -EIO;

	page = (struct ipr_rw_err_mode_page *)(((u8 *)&mode_pages) +
					       mode_pages.hdr.block_desc_len +
					       sizeof(mode_pages.hdr));
	ch_page = (struct ipr_rw_err_mode_page *)(((u8 *)&ch_mode_pages) +
						  ch_mode_pages.hdr.block_desc_len +
						  sizeof(ch_mode_pages.hdr));

	IPR_SET_MODE(ch_page->awre, page->awre, 1);
	IPR_SET_MODE(ch_page->arre, page->arre, 1);

	page->awre = 1;
	page->arre = 1;

	if (page->awre != 1)
		goto error;
	if (page->arre != 1)
		goto error;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (mode_select(dev, &mode_pages, len)) {
error:
		syslog(LOG_ERR, "Failed to setup mode page 0x01 for %s.\n",
		       dev->scsi_dev_data->sysfs_device_name);
		return -EIO;
	}
	return 0;
}

static int page0x0a_setup(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_control_mode_page *page;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev, 0x0A, &mode_pages))
		return -EIO;

	page = (struct ipr_control_mode_page *)(((u8 *)&mode_pages) +
						mode_pages.hdr.block_desc_len +
						sizeof(mode_pages.hdr));

	if (page->queue_algorithm_modifier != 1)
		return 0;
	if (page->qerr != 1)
		return 0;
	if (page->dque != 0)
		return 0;

	return 1;
}

static int setup_page0x0a(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages, ch_mode_pages;
	struct ipr_control_mode_page *page, *ch_page;
	int len;

	memset(&mode_pages, 0, sizeof(mode_pages));
	memset(&ch_mode_pages, 0, sizeof(ch_mode_pages));

	if (ipr_mode_sense(dev, 0x0A, &mode_pages))
		return -EIO;
	if (ipr_mode_sense(dev, 0x4A, &ch_mode_pages))
		return -EIO;

	page = (struct ipr_control_mode_page *)(((u8 *)&mode_pages) +
						mode_pages.hdr.block_desc_len +
						sizeof(mode_pages.hdr));
	ch_page = (struct ipr_control_mode_page *)(((u8 *)&ch_mode_pages) +
						   ch_mode_pages.hdr.block_desc_len +
						   sizeof(ch_mode_pages.hdr));

	IPR_SET_MODE(ch_page->queue_algorithm_modifier,
		     page->queue_algorithm_modifier, 1);
	IPR_SET_MODE(ch_page->qerr, page->qerr, 1);
	IPR_SET_MODE(ch_page->dque, page->dque, 0);

	if (page->queue_algorithm_modifier != 1)
		return -EIO;
	if (page->qerr != 1)
		return -EIO;
	if (page->dque != 0)
		return -EIO;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (mode_select(dev, &mode_pages, len)) {
		syslog(LOG_ERR, "Failed to setup mode page 0x0A for %s.\n",
		       dev->scsi_dev_data->sysfs_device_name);
		return -EIO;
	}
	return 0;
}

static int page0x20_setup(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev, 0x20, &mode_pages))
		return 0;

	page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
					     mode_pages.hdr.block_desc_len +
					     sizeof(mode_pages.hdr));

	return (page->max_tcq_depth == AF_DISK_TCQ_DEPTH);
}

static int ipr_setup_page0x20(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;
	int len;

	if (!ipr_is_af_dasd_device(dev))
		return 0;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev, 0x20, &mode_pages))
		return -EIO;

	page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
					     mode_pages.hdr.block_desc_len +
					     sizeof(mode_pages.hdr));

	page->max_tcq_depth = AF_DISK_TCQ_DEPTH;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (ipr_mode_select(dev, &mode_pages, len))
		return -EIO;

	return 0;
}

/*
 * VSETs:
 * 1. Adjust queue depth based on number of devices
 *
 */
static void init_vset_dev(struct ipr_dev *dev)
{
	struct ipr_query_res_state res_state;
	char q_depth[10];
	char cur_depth[100];
	int depth;

	memset(&res_state, 0, sizeof(res_state));

	if (!ipr_query_resource_state(dev, &res_state)) {
		depth = res_state.vset.num_devices_in_vset * 4;
		snprintf(q_depth, sizeof(q_depth), "%d", depth);
		q_depth[sizeof(q_depth)-1] = '\0';
		if (ipr_read_dev_attr(dev, "queue_depth", cur_depth))
			return;
		if (!strcmp(cur_depth, q_depth))
			return;
		if (ipr_write_dev_attr(dev, "queue_depth", q_depth))
			return;
	}
}

/*
 * GPDD DASD:
 * 1. Setup Mode Page 0x0A for TCQing.
 * 2. Enable TCQing
 * 3. Adjust queue depth
 *
 */
static void init_gpdd_dev(struct ipr_dev *dev)
{
	struct ipr_disk_attr attr;

	if (runtime && page0x0a_setup(dev))
		return;
	if (setup_page0x0a(dev)) {
		syslog_dbg(LOG_DEBUG, "Failed to enable TCQing for %s.\n",
			   dev->scsi_dev_data->sysfs_device_name);
		return;
	}
	if (ipr_get_dev_attr(dev, &attr))
		return;

	attr.queue_depth = GPDD_TCQ_DEPTH;
	attr.tcq_enabled = 1;

	if (ipr_modify_dev_attr(dev, &attr))
		return;
	if (ipr_set_dev_attr(dev, &attr, 0))
		return;
}

/*
 * AF DASD:
 * 1. Setup mode pages (pages 0x01, 0x0A, 0x20)
 * 2. Send set supported devices
 * 3. Set DASD timeouts
 */
static void init_af_dev(struct ipr_dev *dev)
{
	struct ipr_disk_attr attr;

	if (runtime && page0x20_setup(dev) && page0x0a_setup(dev))
		return;
	if (ipr_set_dasd_timeouts(dev))
		return;
	if (setup_page0x01(dev))
		return;
	if (setup_page0x0a(dev))
		return;
	if (ipr_setup_page0x20(dev))
		return;
	if (enable_af(dev))
		return;
	if (ipr_get_dev_attr(dev, &attr))
		return;

	attr.format_timeout = IPR_FORMAT_UNIT_TIMEOUT;

	if (ipr_modify_dev_attr(dev, &attr))
		return;
	if (ipr_set_dev_attr(dev, &attr, 0))
		return;
}

/*
 * IOA:
 * 1. Load saved adapter configuration
 */
static void init_ioa_dev(struct ipr_dev *dev)
{
	struct ipr_scsi_buses buses;

	if (ipr_get_bus_attr(dev->ioa, &buses))
		return;
	ipr_modify_bus_attr(dev->ioa, &buses);
	if (ipr_set_bus_attr(dev->ioa, &buses, 0))
		return;
}

static void init_dev(struct ipr_dev *dev)
{
	if (!dev->scsi_dev_data)
		return;

	switch (dev->scsi_dev_data->type) {
	case TYPE_DISK:
		if (ipr_is_volume_set(dev))
			init_vset_dev(dev);
		else
			init_gpdd_dev(dev);
		break;
	case IPR_TYPE_AF_DISK:
		init_af_dev(dev);
		break;
	default:
		break;
	};
}

static void init_ioa(struct ipr_ioa *ioa)
{
	int i = 0;

	for (i = 0; i < ioa->num_devices; i++)
		init_dev(&ioa->dev[i]);

	init_ioa_dev(&ioa->ioa);
}

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
			init_ioa(ioa);
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
