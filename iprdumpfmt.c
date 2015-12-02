/**
 * IBM IPR dump formatting utility
 *
 * (C) Copyright 2015
 * International Business Machines Corporation and others.
 * All Rights Reserved. This program and the accompanying
 * materials are made available under the terms of the
 * Common Public License v1.0 which accompanies this distribution.
 **/

#include <endian.h>
#include <getopt.h>
#include "iprlib.h"

#define EYE_CATCHER_BE 0xC5D4E3F2
#define EYE_CATCHER_LE 0xF2E3D4C5

#define OS_LINUX 0x4C4E5558

#define DRV_IPR2 0x49505232

#define TYPE_ASCII 0x41534349
#define TYPE_BIN   0x42494E41
#define TYPE_MR32  0x4D523332
#define TYPE_MR64  0x4D523634

#define ID_IOA_DUMP     0x494F4131
#define ID_IOA_LOC      0x4C4F4341
#define ID_DRV_TRACE    0x54524143
#define ID_DRV_VER      0x44525652
#define ID_DRV_TYPE     0x54595045
#define ID_DRV_CTRL_BLK 0x494F4342
#define ID_DRV_PEND_OPS 0x414F5053

#define IPR_DUMP_STATUS_SUCCESS      0
#define IPR_DUMP_STATUS_QUAL_SUCCESS 2
#define IPR_DUMP_STATUS_FAILED       0xFFFFFFFF

static const struct {
	const u8 op;
	const char *name;
} scsi_cmnds[] = {
	{ 0xc1, "IPR_CANCEL_REQUEST" },
	{ 0xc2, "IPR_QUERY_RSRC_STATE" },
	{ 0xc3, "IPR_RESET_DEVICE" },
	{ 0xc4, "IPR_ID_HOST_RR_Q" },
	{ 0xc5, "IPR_QUERY_IOA_CONFIG" },
	{ 0xce, "IPR_CANCEL_ALL_REQUESTS" },
	{ 0xcf, "IPR_HOST_CONTROLLED_ASYNC" },
	{ 0xfb, "IPR_SET_SUPPORTED_DEVICES" },
	{ 0xf7, "IPR_IOA_SHUTDOWN" },

	{ 0x00, "TEST_UNIT_READY" },
	{ 0x01, "REZERO_UNIT" },
	{ 0x03, "REQUEST_SENSE" },
	{ 0x04, "FORMAT_UNIT" },
	{ 0x05, "READ_BLOCK_LIMITS" },
	{ 0x07, "REASSIGN_BLOCKS" },
	{ 0x07, "INITIALIZE_ELEMENT_STATUS" },
	{ 0x08, "READ_6" },
	{ 0x0a, "WRITE_6" },
	{ 0x0b, "SEEK_6" },
	{ 0x0f, "READ_REVERSE" },
	{ 0x10, "WRITE_FILEMARKS" },
	{ 0x11, "SPACE" },
	{ 0x12, "INQUIRY" },
	{ 0x14, "RECOVER_BUFFERED_DATA" },
	{ 0x15, "MODE_SELECT" },
	{ 0x16, "RESERVE" },
	{ 0x17, "RELEASE" },
	{ 0x18, "COPY" },
	{ 0x19, "ERASE" },
	{ 0x1a, "MODE_SENSE" },
	{ 0x1b, "START_STOP" },
	{ 0x1c, "RECEIVE_DIAGNOSTIC" },
	{ 0x1d, "SEND_DIAGNOSTIC" },
	{ 0x1e, "ALLOW_MEDIUM_REMOVAL" },
	{ 0x23, "READ_FORMAT_CAPACITIES" },
	{ 0x24, "SET_WINDOW" },
	{ 0x25, "READ_CAPACITY" },
	{ 0x28, "READ_10" },
	{ 0x2a, "WRITE_10" },
	{ 0x2b, "SEEK_10" },
	{ 0x2b, "POSITION_TO_ELEMENT" },
	{ 0x2e, "WRITE_VERIFY" },
	{ 0x2f, "VERIFY" },
	{ 0x30, "SEARCH_HIGH" },
	{ 0x31, "SEARCH_EQUAL" },
	{ 0x32, "SEARCH_LOW" },
	{ 0x33, "SET_LIMITS" },
	{ 0x34, "PRE_FETCH" },
	{ 0x34, "READ_POSITION" },
	{ 0x35, "SYNCHRONIZE_CACHE" },
	{ 0x36, "LOCK_UNLOCK_CACHE" },
	{ 0x37, "READ_DEFECT_DATA" },
	{ 0x38, "MEDIUM_SCAN" },
	{ 0x39, "COMPARE" },
	{ 0x3a, "COPY_VERIFY" },
	{ 0x3b, "WRITE_BUFFER" },
	{ 0x3c, "READ_BUFFER" },
	{ 0x3d, "UPDATE_BLOCK" },
	{ 0x3e, "READ_LONG" },
	{ 0x3f, "WRITE_LONG" },
	{ 0x40, "CHANGE_DEFINITION" },
	{ 0x41, "WRITE_SAME" },
	{ 0x42, "UNMAP" },
	{ 0x43, "READ_TOC" },
	{ 0x44, "READ_HEADER" },
	{ 0x4a, "GET_EVENT_STATUS_NOTIFICATION" },
	{ 0x4c, "LOG_SELECT" },
	{ 0x4d, "LOG_SENSE" },
	{ 0x53, "XDWRITEREAD_10" },
	{ 0x55, "MODE_SELECT_10" },
	{ 0x56, "RESERVE_10" },
	{ 0x57, "RELEASE_10" },
	{ 0x5a, "MODE_SENSE_10" },
	{ 0x5e, "PERSISTENT_RESERVE_IN" },
	{ 0x5f, "PERSISTENT_RESERVE_OUT" },
	{ 0x7f, "VARIABLE_LENGTH_CMD" },
	{ 0xa0, "REPORT_LUNS" },
	{ 0xa2, "SECURITY_PROTOCOL_IN" },
	{ 0xa3, "MAINTENANCE_IN" },
	{ 0xa4, "MAINTENANCE_OUT" },
	{ 0xa5, "MOVE_MEDIUM" },
	{ 0xa6, "EXCHANGE_MEDIUM" },
	{ 0xa8, "READ_12" },
	{ 0xa9, "SERVICE_ACTION_OUT_12" },
	{ 0xaa, "WRITE_12" },
	{ 0xab, "SERVICE_ACTION_IN_12" },
	{ 0xae, "WRITE_VERIFY_12" },
	{ 0xaf, "VERIFY_12" },
	{ 0xb0, "SEARCH_HIGH_12" },
	{ 0xb1, "SEARCH_EQUAL_12" },
	{ 0xb2, "SEARCH_LOW_12" },
	{ 0xb5, "SECURITY_PROTOCOL_OUT" },
	{ 0xb8, "READ_ELEMENT_STATUS" },
	{ 0xb6, "SEND_VOLUME_TAG" },
	{ 0xea, "WRITE_LONG_2" },
	{ 0x83, "EXTENDED_COPY" },
	{ 0x84, "RECEIVE_COPY_RESULTS" },
	{ 0x86, "ACCESS_CONTROL_IN" },
	{ 0x87, "ACCESS_CONTROL_OUT" },
	{ 0x88, "READ_16" },
	{ 0x89, "COMPARE_AND_WRITE" },
	{ 0x8a, "WRITE_16" },
	{ 0x8c, "READ_ATTRIBUTE" },
	{ 0x8d, "WRITE_ATTRIBUTE" },
	{ 0x8f, "VERIFY_16" },
	{ 0x91, "SYNCHRONIZE_CACHE_16" },
	{ 0x93, "WRITE_SAME_16" },
	{ 0x9d, "SERVICE_ACTION_BIDIRECTIONAL" },
	{ 0x9e, "SERVICE_ACTION_IN_16" },
	{ 0x9f, "SERVICE_ACTION_OUT_16" }
};

struct ipr_dump_header {
	u32 eye_catcher;
	u32 len;
	u32 num_entries;
	u32 first_entry_offset;
	u32 status;
	u32 os;
	u32 driver_name;
};

struct ipr_dump_entry_header {
	u32 eye_catcher;
	u32 len; /* does not include entry header */
	u32 num_elems;
	u32 offset; /* offset to data from beginning of dump file */
	u32 data_type;
	u32 id;
	u32 status;
};

struct ipr_trace_entry {
	u32 time;

	u8 op_code; /* SCSI opcode */
	u8 ata_op_code;
	u8 type;
#define IPR_TRACE_START			0x00
#define IPR_TRACE_FINISH		0xff
	u8 cmd_index;

	u32 res_handle; /* stored as BE */
	union {
		u32 ioasc;
		u32 add_data;
		u32 res_addr;
	} u;
};

static void *dump_map;    /* ipr dump memory map */
static FILE *out_fp;      /* output report file pointer */
static int verbose_trace; /* flag for verbose trace printing */

#define TARGET_TO_HOST32(value, eye_catcher) \
	(eye_catcher == EYE_CATCHER_BE ? \
	 be32toh(value) : \
	 le32toh(value))

/**
 * get_scsi_command_literal -
 * @value:		SCSI opcode
 *
 * Returns:
 *   SCSI command string
 **/
static const char* get_scsi_command_literal(u32 value)
{
	u32 i = 0;

	for (i = 0; i < ARRAY_SIZE(scsi_cmnds); ++i) {
		if (value == scsi_cmnds[i].op)
			return scsi_cmnds[i].name;
	}
	return NULL;
}

/**
 * read_dump_header -
 * @hdr:		ipr dump header pointer
 *
 * Returns:
 *   nothing
 **/
static void read_dump_header(struct ipr_dump_header *hdr)
{
	struct ipr_dump_header *raw_hdr = dump_map;

	hdr->eye_catcher = htobe32(raw_hdr->eye_catcher);

	/* convert to correct endianness */
	hdr->len = TARGET_TO_HOST32(raw_hdr->len, hdr->eye_catcher);
	hdr->num_entries = TARGET_TO_HOST32(raw_hdr->num_entries,
					    hdr->eye_catcher);
	hdr->first_entry_offset = TARGET_TO_HOST32(raw_hdr->first_entry_offset,
						   hdr->eye_catcher);
	hdr->status = TARGET_TO_HOST32(raw_hdr->status, hdr->eye_catcher);
	hdr->os = TARGET_TO_HOST32(raw_hdr->os, hdr->eye_catcher);
	hdr->driver_name = TARGET_TO_HOST32(raw_hdr->driver_name,
					    hdr->eye_catcher);
}

/**
 * print_dump_header -
 * @hdr:		ipr dump header pointer
 *
 * Returns:
 *   nothing
 **/
static void print_dump_header(struct ipr_dump_header *hdr)
{
	fprintf(out_fp, "IPR Adapter Dump Report\n\n");
	switch (hdr->eye_catcher) {
	case EYE_CATCHER_BE:
		fprintf(out_fp, "Big Endian format: ");
		break;
	case EYE_CATCHER_LE:
		fprintf(out_fp, "Little Endian format: ");
		break;
	default:
		fprintf(out_fp, "Unknown dump format: ");
	}
	fprintf(out_fp, "Eye Catcher is 0x%08X\n", hdr->eye_catcher);

	fprintf(out_fp, "Total length: %d bytes\n", hdr->len);
	fprintf(out_fp, "Number of dump entries: %d\n", hdr->num_entries);
	fprintf(out_fp, "Offset to first entry: 0x%08X bytes\n",
		hdr->first_entry_offset);
	fprintf(out_fp, "Dump status: 0x%08X ", hdr->status);
	switch (hdr->status) {
	case IPR_DUMP_STATUS_SUCCESS:
		fprintf(out_fp, "(SUCCESS)\n");
		break;
	case IPR_DUMP_STATUS_QUAL_SUCCESS:
		fprintf(out_fp, "(QUAL_SUCCESS)\n");
		break;
	case IPR_DUMP_STATUS_FAILED:
		fprintf(out_fp, "(FAILED)\n");
		break;
	default:
		fprintf(out_fp, "(UNKNOWN)\n");
	}

	fprintf(out_fp, "Operating System: ");
	switch (hdr->os) {
	case OS_LINUX:
		fprintf(out_fp, "Linux\n");
		break;
	default:
		fprintf(out_fp, "unknown\n");
	}

	fprintf(out_fp, "Driver: ");
	switch (hdr->driver_name) {
	case DRV_IPR2:
		fprintf(out_fp, "ipr2\n");
		break;
	default:
		fprintf(out_fp, "unknown\n");
	}
}

/**
 * read_entry_header -
 * @hdr:		ipr dump entry header pointer
 * @offset:		offset to entry data
 *
 * Returns:
 *   offset to next entry header
 **/
static u32 read_entry_header(struct ipr_dump_entry_header *hdr, u32 offset)
{
	struct ipr_dump_entry_header *raw_hdr = dump_map+offset;
	hdr->eye_catcher = htobe32(raw_hdr->eye_catcher);

	/* convert to correct endianness */
	hdr->len = TARGET_TO_HOST32(raw_hdr->len, hdr->eye_catcher);
	hdr->num_elems = TARGET_TO_HOST32(raw_hdr->num_elems, hdr->eye_catcher);
	hdr->offset = TARGET_TO_HOST32(raw_hdr->offset, hdr->eye_catcher);
	hdr->data_type = TARGET_TO_HOST32(raw_hdr->data_type, hdr->eye_catcher);
	hdr->id = TARGET_TO_HOST32(raw_hdr->id, hdr->eye_catcher);
	hdr->status = TARGET_TO_HOST32(raw_hdr->status, hdr->eye_catcher);
	hdr->offset += offset;

	return (hdr->offset + hdr->len);
}

/**
 * read_trace_data -
 * @offset:		offset to entry data
 * @eye_catcher:	ipr entry eye catcher
 *
 * Returns:
 *   ipr trace entry structure
 **/
static struct ipr_trace_entry read_trace_data(u32 offset, u32 eye_catcher)
{
	struct ipr_trace_entry trace;
	struct ipr_trace_entry *raw_trace = dump_map+offset;

        /* always stored as BE */
	trace.res_handle = htobe32(raw_trace->res_handle);

	/* convert to correct endianness */
	trace.time = TARGET_TO_HOST32(raw_trace->time, eye_catcher);
	trace.u.add_data = TARGET_TO_HOST32(raw_trace->u.add_data, eye_catcher);

	/* copy remaining trace fields */
	trace.op_code = raw_trace->op_code;
	trace.ata_op_code = raw_trace->ata_op_code;
	trace.type = raw_trace->type;
	trace.cmd_index = raw_trace->cmd_index;

	return trace;
}

#define JIFF_TO_USEC(x) x * 1000 * 1000 / sysconf(_SC_CLK_TCK)
/**
 * print_trace -
 * @hdr:		ipr entry header pointer
 *
 * Returns:
 *   nothing
 **/
static void print_trace(struct ipr_dump_entry_header *hdr)
{
	u32 i, min_pos, min_time, num_entries, offset;
	struct ipr_trace_entry *raw_trace;
	struct ipr_trace_entry t;
	const char *cmnd;

	num_entries = hdr->len / sizeof(*raw_trace);
	raw_trace = dump_map + hdr->offset;
	min_pos = 0;
	min_time = TARGET_TO_HOST32(raw_trace->time, hdr->eye_catcher);

	fprintf(out_fp, "\t entries found: %d\n\n", num_entries);
	for (i = 1; i < num_entries; ++i) {
		raw_trace = dump_map + hdr->offset + i*sizeof(*raw_trace);
		if (TARGET_TO_HOST32(raw_trace->time, hdr->eye_catcher) <
		    min_time) {
			min_time = TARGET_TO_HOST32(raw_trace->time,
						    hdr->eye_catcher);
			min_pos = i;
		}
	}
	for (i = 0; i < num_entries; ++i) {
		offset = hdr->offset +
			((min_pos+i)%num_entries) * sizeof(*raw_trace);
		t = read_trace_data(offset, hdr->eye_catcher);
		fprintf(out_fp, "\t %08X %02X%02X%02X%02X %08X %08X",
			t.time,	t.op_code, t.ata_op_code, t.type, t.cmd_index,
			t.res_handle, t.u.add_data);
		if (verbose_trace) {
			t.time -= min_time;
			fprintf(out_fp, "\t|\t%+10lld\xC2\xB5s\t",
				(long long unsigned)JIFF_TO_USEC(t.time));
			switch(t.type) {
			case IPR_TRACE_START:
				fprintf(out_fp, "START\t");
				break;
			case IPR_TRACE_FINISH:
				fprintf(out_fp, "FINISH\t");
				break;
			default:
				fprintf(out_fp, "\t");
			}
			cmnd = get_scsi_command_literal(t.op_code);
			if (cmnd)
				fprintf(out_fp, "%s", cmnd);

		}
		fprintf(out_fp, "\n");
	}
}

/**
 * dump_entry_data -
 * @hdr:		ipr dump entry header pointer
 *
 * Returns:
 *   nothing
 **/
static void dump_entry_data(struct ipr_dump_entry_header *hdr)
{
	u32 i;
	char *s;

	switch (hdr->id) {
	case ID_IOA_LOC:
		fprintf(out_fp, "IOA Location:\n");
		s = dump_map+hdr->offset;
		fprintf(out_fp, "\t %.*s\n\n", hdr->len, s);
		break;
	case ID_DRV_VER:
		fprintf(out_fp, "Driver Version:\n");
		s = dump_map+hdr->offset;
		fprintf(out_fp, "\t %.*s\n\n", hdr->len, s);
		break;
	case ID_DRV_TRACE:
		fprintf(out_fp, "Driver Trace Entries:\n");
		print_trace(hdr);
		fprintf(out_fp, "\n");
		break;
	case ID_DRV_TYPE:
		fprintf(out_fp, "Driver Type:\n");
		i = TARGET_TO_HOST32(*(int*)(dump_map+hdr->offset),
				     hdr->eye_catcher);
		fprintf(out_fp, "\t CCIN: 0x%04X\n", i);
		i = TARGET_TO_HOST32(*(int*)(dump_map+hdr->offset+sizeof(i)),
				     hdr->eye_catcher);
		fprintf(out_fp,"\t Adapter firmware Version: "
			" %02X%02X%02X%02X\n\n",
			i >> 24,                /* major release */
			(i & 0x00FF0000) >> 16, /* card type */
			(i & 0x0000FF00) >> 8,  /* minor release 0 */
			i & 0x000000FF);        /* minor release 1 */
		break;
	case ID_IOA_DUMP:
		/* skip this one, not very useful for driver debugging */
		break;
	case ID_DRV_CTRL_BLK:
	case ID_DRV_PEND_OPS:
		/* not implemented yet */
		fprintf(stderr,
			"Warning: found IPR Control Block or IPR Pending Ops!\n"
			"These are not implemented yet. Skipping...\n");
		break;
	default:
		fprintf(stderr,
			"Warning: did not recognize data identification "
			"(id 0x%08X). Skipping...\n\n", hdr->id);
	}
}

/**
 * fsize -
 * @filename:		ipr dump file name
 *
 * Returns:
 *   ipr dump file size
 **/
off_t fsize(const char *filename)
{
	struct stat st;

	if (stat(filename, &st) == 0)
		return st.st_size;

	return -1;
}

/**
 * print_usage -
 *
 * Returns:
 *   nothing
 **/
static void print_usage()
{
	fprintf(stdout,
		"Usage: iprdumpfmt [options] dump_file\n"
		"  Options:\n"
		"\t-h --help\tPrint this message\n"
		"\t-o --output\tSpecify an output file\n"
		"\t-v --verbose\tPrint verbose trace data\n");
}

static struct option long_options[] = {
	{"output", required_argument, 0, 'o'},
	{"help", no_argument, 0, 'h'},
	{"verbose", no_argument, &verbose_trace, 1},
	{0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
	u32 i, offset;
	int in_fd, filesize;
	struct ipr_dump_header hdr;
	struct ipr_dump_entry_header entry_hdr;
	int c, option_index = 0;
	char out_filename[512] = {'\0'};

	while ((c = getopt_long(argc, argv, "hvo:", long_options,
				&option_index)) != -1) {
		switch (c) {
		case 0:
			break;
		case 'o':
			strncpy(out_filename, optarg, sizeof(out_filename) - 1);
			break;
		case '?':
		case 'h':
			print_usage();
			return 0;
		case 'v':
			verbose_trace = 1;
			break;
		default:
			return -1;
		}
	}

	if (argc - optind < 1) {
		/* needs at least a dump file */
		print_usage();
		return -1;
	}

	in_fd = open(argv[optind], O_RDONLY);
	if (in_fd < 0) {
		int rc = errno;
		fprintf(stderr, "Could not open dump file %s: %s\n",
			argv[optind], strerror(errno));
		return rc;
	}

	if (out_filename[0] == '\0')
		snprintf(out_filename, sizeof(out_filename), "%s.report",
			 argv[optind]);
	out_fp = fopen(out_filename, "w");
	if (out_fp == NULL) {
		int rc = errno;
		fprintf(stderr, "Could not open output file %s: %s\n",
			out_filename, strerror(errno));
		return rc;
	}

	filesize = fsize(argv[optind]);
	if (filesize < 0) {
		int rc = errno;
		fprintf(stderr, "Could not determine size of %s: %s\n",
			argv[optind], strerror(errno));
		return rc;
	}

	dump_map = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, in_fd, 0);
	if (dump_map == MAP_FAILED) {
		int rc = errno;
		fprintf(stderr, "Could not map dump file to memory: %s\n",
			strerror(errno));
		return rc;
	}

	read_dump_header(&hdr);
	print_dump_header(&hdr);
	fprintf(out_fp, "\n");

	offset = hdr.first_entry_offset;
	for (i = 0; i < hdr.num_entries; ++i) {
		offset = read_entry_header(&entry_hdr, offset);
		dump_entry_data(&entry_hdr);
	}

	return 0;
}
