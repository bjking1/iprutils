/******************************************************************/
/* Linux IBM IPR initialization utility                           */
/* Description: IBM Power RAID initialization utility             */
/*                                                                */
/* (C) Copyright 2004                                             */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprinit.c,v 1.4 2004/02/17 16:15:56 bjking1 Exp $
 */

#include <unistd.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>
#include <sys/stat.h>
#include <libiberty.h>

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/mman.h>

#define GPDD_TCQ_DEPTH "64"
#define AF_DISK_TCQ_DEPTH 64

/* xxx HUP signal handler. Called by rc.d script on reload/restart
 called by iprconfig at appropriate times. */

/* daemonize and loop through all AF DASD, looking for read/write timeouts
 to be different than we expect. If so, re-initialize the device */

struct ipr_dasd_timeout_record {
	u8 op_code;
	u8 reserved;
	u16 timeout;
};

static const struct ipr_dasd_timeout_record ipr_dasd_timeouts[] = {
	{TEST_UNIT_READY, 0, __constant_cpu_to_be16(30)},
	{REQUEST_SENSE, 0, __constant_cpu_to_be16(30)},
	{INQUIRY, 0, __constant_cpu_to_be16(30)},
	{MODE_SELECT, 0, __constant_cpu_to_be16(30)},
	{MODE_SENSE, 0, __constant_cpu_to_be16(30)},
	{READ_CAPACITY, 0, __constant_cpu_to_be16(30)},
	{READ_10, 0, __constant_cpu_to_be16(30)},
	{WRITE_10, 0, __constant_cpu_to_be16(30)},
	{WRITE_VERIFY, 0, __constant_cpu_to_be16(30)},
	{FORMAT_UNIT, 0, __constant_cpu_to_be16(2 * 60 * 60)},
	{REASSIGN_BLOCKS, 0, __constant_cpu_to_be16(10 * 60)},
	{START_STOP, 0, __constant_cpu_to_be16(2 * 60)},
	{SEND_DIAGNOSTIC, 0, __constant_cpu_to_be16(5 * 60)},
	{VERIFY, 0, __constant_cpu_to_be16(5 * 60)},
	{WRITE_BUFFER, 0, __constant_cpu_to_be16(5 * 60)},
	{WRITE_SAME, 0, __constant_cpu_to_be16(4 * 60 * 60)},
	{LOG_SENSE, 0, __constant_cpu_to_be16(30)},
	{REPORT_LUNS, 0, __constant_cpu_to_be16(30)},
	{SKIP_READ, 0, __constant_cpu_to_be16(30)},
	{SKIP_WRITE, 0, __constant_cpu_to_be16(30)}
};

struct ipr_dasd_timeouts {
	u32 length;
	struct ipr_dasd_timeout_record record[ARRAY_SIZE(ipr_dasd_timeouts)];
};

static int set_dasd_timeouts(struct ipr_dev *dev)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	struct ipr_dasd_timeouts timeouts;

	/* xxx - customizable format timeout? */
	fd = open(dev->gen_name, O_RDWR);

	if (fd <= 1) {
		syslog(LOG_ERR, "Could not open %s. %m\n",
		       dev->ioa->ioa.gen_name);
		return errno;
	}

	memcpy(timeouts.record, ipr_dasd_timeouts, sizeof(ipr_dasd_timeouts));
	timeouts.length = htonl(sizeof(timeouts));

	memset(cdb, 0, IPR_CCB_CDB_LEN);
	cdb[0] = SET_DASD_TIMEOUTS;
	cdb[7] = (sizeof(timeouts) >> 8) & 0xff;
	cdb[8] = sizeof(timeouts) & 0xff;

	rc = sg_ioctl(fd, cdb, &timeouts,
		      sizeof(timeouts), SG_DXFER_TO_DEV,
		      &sense_data, SET_DASD_TIMEOUTS_TIMEOUT);

	if (rc != 0)
		syslog(LOG_ERR, "Set DASD timeouts to %s failed.\n",
		       dev->scsi_dev_data->sysfs_device_name);

	close(fd);
	return rc;
}

static int set_supported_devs(struct ipr_dev *dev,
		       struct ipr_std_inq_data *std_inq)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	struct ipr_supported_device supported_dev;

	fd = open(dev->ioa->ioa.gen_name, O_RDWR);

	if (fd <= 1) {
		syslog(LOG_ERR, "Could not open %s. %m\n",
		       dev->ioa->ioa.gen_name);
		return errno;
	}

	memset(&supported_dev, 0, sizeof(struct ipr_supported_device));
	memcpy(&supported_dev.vpids, &std_inq->vpids, sizeof(std_inq->vpids));
	supported_dev.num_records = 1;
	supported_dev.data_length = htons(sizeof(supported_dev));
	supported_dev.reserved = 0;

	memset(cdb, 0, IPR_CCB_CDB_LEN);
	cdb[0] = SET_SUPPORTED_DEVICES;
	cdb[7] = (sizeof(supported_dev) >> 8) & 0xff;
	cdb[8] = sizeof(supported_dev) & 0xff;

	rc = sg_ioctl(fd, cdb, &supported_dev,
		      sizeof(supported_dev), SG_DXFER_TO_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		syslog(LOG_ERR, "Set supported devices to %s failed.\n",
		       dev->scsi_dev_data->sysfs_device_name);

	close(fd);
	return rc;
}

static int enable_af(struct ipr_dev *dev)
{
	struct ipr_std_inq_data std_inq;

	if (ipr_inquiry(dev->gen_name, IPR_STD_INQUIRY,
			&std_inq, sizeof(std_inq)))
		return -EIO;

	if (set_supported_devs(dev, &std_inq))
		return -EIO;
	return 0;
}

static int mode_select(char *file, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	fd = open(file, O_RDWR);
	if (fd <= 1) {
		syslog(LOG_ERR, "Could not open %s. %m\n",file);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = MODE_SELECT;
	cdb[1] = 0x11;
	cdb[4] = length;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_INTERNAL_TIMEOUT);

	/* xxx additional parm to enable dumping sense data? */
	close(fd);
	return rc;
}

static int setup_page0x01(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_rw_err_mode_page *page;
	int len;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev->gen_name, 0x01, &mode_pages))
		return -EIO;

	page = (struct ipr_rw_err_mode_page *)(((u8 *)&mode_pages) +
					       mode_pages.hdr.block_desc_len +
					       sizeof(mode_pages.hdr));

	page->awre = 1;
	page->arre = 1;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (mode_select(dev->gen_name, &mode_pages, len)) {
		syslog(LOG_ERR, "Failed to setup mode page 0x01 for %s.\n",
		       dev->scsi_dev_data->sysfs_device_name);
		return -EIO;
	}
	return 0;
}

static int setup_page0x0a(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_control_mode_page *page;
	int len;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev->gen_name, 0x0A, &mode_pages))
		return -EIO;

	page = (struct ipr_control_mode_page *)(((u8 *)&mode_pages) +
						mode_pages.hdr.block_desc_len +
						sizeof(mode_pages.hdr));

	page->queue_algorithm_modifier = 1;
	page->qerr = 1;
	page->dque = 0;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (mode_select(dev->gen_name, &mode_pages, len)) {
		syslog(LOG_ERR, "Failed to setup mode page 0x0A for %s.\n",
		       dev->scsi_dev_data->sysfs_device_name);
		return -EIO;
	}
	return 0;
}

static int setup_page0x20(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;
	int len;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev->gen_name, 0x20, &mode_pages))
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

	if (mode_select(dev->gen_name, &mode_pages, len)) {
		syslog(LOG_ERR, "Failed to setup mode page 0x20 for %s.\n",
		       dev->scsi_dev_data->sysfs_device_name);
		return -EIO;
	}
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

	memset(&res_state, 0, sizeof(res_state));

	if (!ipr_query_resource_state(dev->gen_name, &res_state)) {
		snprintf(q_depth, sizeof(q_depth), "%d",
			 (res_state.vset.num_devices_in_vset * 4));
		q_depth[sizeof(q_depth)-1] = '\0';
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
	/* xxx check changeable parms for all mode pages */
	if (setup_page0x0a(dev)) {
		syslog(LOG_ERR, "Failed to enable TCQing for %s.\n",
		       dev->scsi_dev_data->sysfs_device_name);
		return;
	}
	if (ipr_write_dev_attr(dev, "tcq_enable", "1"))
		return;
	if (ipr_write_dev_attr(dev, "queue_depth", GPDD_TCQ_DEPTH))
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
	if (set_dasd_timeouts(dev))
		return;
	if (setup_page0x01(dev))
		return;
	if (setup_page0x0a(dev))
		return;
	if (setup_page0x20(dev))
		return;
	if (enable_af(dev))
		return;
}

/*
 * IOA:
 * 1. Load saved adapter configuration
 */
static void init_ioa_dev(struct ipr_dev *dev)
{
	char fname[64];

	sprintf(fname,"%x_%s", dev->ioa->ccin, dev->ioa->pci_address);
	if (RC_SUCCESS == ipr_config_file_valid(fname))
		ipr_set_page_28(dev->ioa, IPR_NORMAL_CONFIG, 0);
	else
		ipr_set_page_28_init(dev->ioa, IPR_NORMAL_CONFIG);


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

	/*
	 * 1. Initialize all devices
	 * 2. Load configuration from cfg file
	 *
	 */
}

int main(int argc, char *argv[])
{
	struct ipr_ioa *ioa;

	openlog("iprinit", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);

	if (argc > 1) {
		if (strcmp(argv[1], "--version") == 0) {
			printf("iprinit: %s\n", IPR_VERSION_STR);
			return 0;
		} else {
			printf("Usage: iprinit [options]\n");
			printf("  Options: --version    Print iprinit version\n");
			return -EINVAL;
		}
	}

	tool_init("iprinit");
	check_current_config(false);

	for (ioa = ipr_ioa_head; ioa; ioa = ioa->next)
		init_ioa(ioa);

	/* xxx daemonize so that new devices get setup properly? */
	return 0;
}
