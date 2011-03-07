/**
  * IBM IPR adapter performance analysis utility
  *
  * (C) Copyright 2005
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/debug/iprperf.c,v 1.1 2005/09/26 20:19:36 brking Exp $
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

char *tool_name = "iprperf";

struct log_sense {
	u8 page_code;
	u8 reserved;
	u16 len;
	u8 reserved2[3];
	u8 parm_len;
	u16 num_seeks_0;
	u16 num_seeks_gt_2_3;
	u16 num_seeks_gt_1_3;
	u16 num_seeks_gt_1_6;
	u16 num_seeks_gt_1_12;
	u16 num_seeks_gt_0;
	u8 reserved3[4];
	u16 num_dev_read_buf_overruns;
	u16 num_dev_write_buf_overruns;
	u32 num_dev_cache_read_hits;
	u32 num_dev_cache_partial_read_hits;
	u32 num_dev_cache_write_hits;
	u32 num_dev_cache_fast_writes;
	u32 reserved4[2];
	u32 num_device_read_ops;
	u32 num_device_write_ops;
	u32 num_ioa_cache_read_hits;
	u32 num_ioa_cache_partial_read_hits;
	u32 num_ioa_cache_write_hits;
	u32 num_ioa_cache_fast_writes;
	u32 num_ioa_emul_read_cache_hits;
	u32 ioa_idle_loop_count[2];
	u32 ioa_idle_loop_count_value;
	u8 ioa_idle_count_value_units;
	u8 reserved5[3];
};

static int get_counters(struct ipr_dev *dev, struct log_sense *log)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);
	memset(log, 0, sizeof(*log));

	cdb[0] = LOG_SENSE;
	cdb[2] = 0x70;
	cdb[7] = sizeof(*log) >> 8;
	cdb[8] = sizeof(*log) & 0xff;

	rc = sg_ioctl(fd, cdb, log,
		      sizeof(*log), SG_DXFER_FROM_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Log Sense", rc);

	close(fd);
	return rc;
}

static void print_counters(struct ipr_dev *dev, struct log_sense *log)
{
	printf("Number of seeks  = zero length           : %d\n", ntohs(log->num_seeks_0));
	printf("Number of seeks >= 2/3 disk              : %d\n", ntohs(log->num_seeks_gt_2_3));
	printf("Number of seeks >= 1/3  and < 2/3 disk   : %d\n", ntohs(log->num_seeks_gt_1_3));
	printf("Number of seeks >= 1/6  and < 1/3 disk   : %d\n", ntohs(log->num_seeks_gt_1_6));
	printf("Number of seeks >= 1/12 and < 1/6 disk   : %d\n", ntohs(log->num_seeks_gt_1_12));
	printf("Number of seeks >  0    and < 1/12 disk  : %d\n", ntohs(log->num_seeks_gt_0));
	printf("Number of device read buffer overruns    : %d\n", ntohs(log->num_dev_read_buf_overruns));
	printf("Number of device write buffer underruns  : %d\n", ntohs(log->num_dev_write_buf_overruns));
	printf("Number of device cache read hits         : %d\n", ntohl(log->num_dev_cache_read_hits));
	printf("Number of device cache partial read hits : %d\n", ntohl(log->num_dev_cache_partial_read_hits));
	printf("Number of device cache write hits        : %d\n", ntohl(log->num_dev_cache_write_hits));
	printf("Number of device cache fast writes       : %d\n", ntohl(log->num_dev_cache_fast_writes));

	if (ipr_is_gscsi(dev))
		return;

	printf("\n");
	printf("Number of device read ops                : %d\n", ntohl(log->num_device_read_ops));
	printf("Number of device write ops               : %d\n", ntohl(log->num_device_write_ops));
	printf("Number of IOA cache read hits            : %d\n", ntohl(log->num_ioa_cache_read_hits));
	printf("Number of IOA cache partial read hits    : %d\n", ntohl(log->num_ioa_cache_partial_read_hits));
	printf("Number of IOA cache write hits           : %d\n", ntohl(log->num_ioa_cache_write_hits));
	printf("Number of IOA cache fast writes          : %d\n", ntohl(log->num_ioa_cache_fast_writes));
	printf("Number of IOA emulated read cache hits   : %d\n", ntohl(log->num_ioa_emul_read_cache_hits));
	printf("IOA Idle loop count                      : %X%08X\n",
	       ntohl(log->ioa_idle_loop_count[0]), ntohl(log->ioa_idle_loop_count[1]));
	printf("IOA Idle loop count value                : %X\n", ntohl(log->ioa_idle_loop_count_value));
	printf("IOA Idle count value units               : %d\n", log->ioa_idle_count_value_units);
}

int main(int argc, char *argv[])
{
	struct ipr_dev *dev;
	struct log_sense log;
	int rc;

	openlog("iprperf", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
	tool_init(1);
	check_current_config(false);
	dev = find_gen_dev(argv[1]);
	if (!dev)
		dev = find_blk_dev(argv[1]);

	if (!dev) {
		fprintf(stderr, "Cannot find %s\n", argv[1]);
		return -EINVAL;
	}

	rc = get_counters(dev, &log);
	if (rc)
		return rc;

	print_counters(dev, &log);
	return 0;
}
