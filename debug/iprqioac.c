/**
  * IBM IPR adapter shutdown utility
  *
  * (C) Copyright 2005
  * International Business Machines Corporation and others.
  * All Rights Reserved. This program and the accompanying
  * materials are made available under the terms of the
  * Common Public License v1.0 which accompanies this distribution.
  *
  */

/*
 * $Header: /cvsroot/iprdd/iprutils/debug/iprqioac.c,v 1.1 2005/12/16 17:04:41 brking Exp $
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

char *tool_name = "iprqioac";
#define IPR_QUERY_IOA_CONFIG				0xC5


struct ipr_config_table_entry {
	u8 service_level;
	u8 array_id;
	u8 flags;
#define IPR_IS_IOA_RESOURCE	0x80
#define IPR_IS_ARRAY_MEMBER 0x20
#define IPR_IS_HOT_SPARE	0x10

	u8 rsvd_subtype;
#define IPR_RES_SUBTYPE(res) (((res)->cfgte.rsvd_subtype) & 0x0f)
#define IPR_SUBTYPE_AF_DASD			0
#define IPR_SUBTYPE_GENERIC_SCSI	1
#define IPR_SUBTYPE_VOLUME_SET		2

	struct ipr_res_addr res_addr;
	u32 res_handle;
	u32 reserved4[2];
	struct ipr_std_inq_data std_inq_data;
}__attribute__ ((packed, aligned (4)));

struct ipr_config_table_hdr {
	u8 num_entries;
	u8 flags;
#define IPR_UCODE_DOWNLOAD_REQ	0x10
	u16 reserved;
}__attribute__((packed, aligned (4)));

struct ipr_config_table {
	struct ipr_config_table_hdr hdr;
	struct ipr_config_table_entry dev[192];
}__attribute__((packed, aligned (4)));

static void dump_config_table(struct ipr_config_table *cfg)
{
	int i;
	u32 *buf = (u32 *)cfg;

	for (i = 0; i < sizeof(*cfg); i += 16, buf += 4) {
		if (!buf[0] && !buf[1] && !buf[2] && !buf[3])
			continue;
		printf("[%04X] %08X %08X %08X %08X\n",
		       i, ntohl(buf[0]), ntohl(buf[1]),
		       ntohl(buf[2]), ntohl(buf[3]));
	}
}

static int ipr_ioa_flush(struct ipr_ioa *ioa)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	struct ipr_config_table cfg;
	int rc;

	memset(&cfg, 0, sizeof(cfg));

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_QUERY_IOA_CONFIG;
	cdb[7] = (sizeof(cfg) >> 8) & 0xff;
	cdb[8] = sizeof(cfg) & 0xff;

	rc = sg_ioctl(fd, cdb, &cfg,
		      sizeof(cfg), SG_DXFER_FROM_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, IPR_QUERY_IOA_CONFIG, rc);
	else
		dump_config_table(&cfg);

	close(fd);
	return rc;
}

int main(int argc, char *argv[])
{
	struct ipr_ioa *ioa;

	tool_init(1);
	check_current_config(false);
	for_each_ioa(ioa) {
		if (!strcmp(ioa->ioa.gen_name, argv[1])) {
			ipr_ioa_flush(ioa);
			return 0;
		}
	}

	return -EINVAL;
}
