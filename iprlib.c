/******************************************************************/
/* Linux IBM IPR utility library                                  */
/* Description: IBM Storage IOA Interface Specification (IPR)     */
/*              Linux utility	library                             */
/*                                                                */
/* (C) Copyright 2000, 2001                                       */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprlib.c,v 1.3 2004/01/13 21:13:32 manderso Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

struct ipr_array_query_data *p_ipr_array_query_data = NULL;
struct ipr_resource_table *p_res_table = NULL;
u32 num_ioas = 0;
struct ipr_ioa *p_head_ipr_ioa = NULL;
struct ipr_ioa *p_last_ipr_ioa = NULL;
int driver_major = 0;
int driver_minor = 0;

/* This table includes both unsupported 522 disks and disks that support 
 being formatted to 522, but require a minimum microcode level. The disks
 that require a minimum level of microcode will be marked by the 
 supported_with_min_ucode_level flag set to true. */
struct unsupported_af_dasd unsupported_af[] =
{
    {
        /* 00 Mako & Hammerhead */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"DRVS            "},
        compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: false,
        min_ucode_level: {"    "},
        min_ucode_mask: {0, 0, 0, 0}
    },
    {
        /* 01 Swordfish */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"DRHS            "},
        compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: false,
        min_ucode_level: {"    "},
        min_ucode_mask: {0, 0, 0, 0}
    },
    {
        /* 02 Neptune */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"DNES            "},
        compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: false,
        min_ucode_level: {"    "},
        min_ucode_mask: {0, 0, 0, 0}
    },
    {
        /* 03 Thornback, Stingray, & Manta */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"DMVS            "},
        compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: false,
        min_ucode_level: {"    "},
        min_ucode_mask: {0, 0, 0, 0}
    },
    {
        /* 04 Discovery I */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"DDYS            "},
        compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: false,
        min_ucode_level: {"    "},
        min_ucode_mask: {0, 0, 0, 0}
    },
    {
        /* 05 US73 (Discovery II) - fixme - are we supporting this drive? */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"IC35L     D2    "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"S5DE"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 06 Apollo (Ext. Scallop 9GB) - LID: SC09 */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST318305L       "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"SC09"},
        lid_mask: {1, 1, 1, 1},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C749"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 07 Apollo (Ext. Scallop 18GB) - LID: SC18 */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST318305L       "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"SC18"},
        lid_mask: {1, 1, 1, 1},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C709"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 08 Apollo (Ext. Scallop 36GB) - LID: SC36 */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST336605L       "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"SC36"},
        lid_mask: {1, 1, 1, 1},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C709"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 09 Apollo (Ext. Scallop 73GB) - LID: SC73 */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST373405L       "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"SC73"},
        lid_mask: {1, 1, 1, 1},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C709"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 10 Apollo (non-external 9GB) - LID: AT1x */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST318305L       "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"AT1 "},
        lid_mask: {1, 1, 1, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C54E"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 11 Apollo (non-external 18GB) - LID: AT0x */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST318305L       "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"AT0 "},
        lid_mask: {1, 1, 1, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C50E"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 12 Apollo (non-external 36GB) - LID: AT0x */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST336605L       "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"AT0 "},
        lid_mask: {1, 1, 1, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C50E"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 13 Apollo (non-external 73GB) - LID: AT0x */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST373405L       "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"AT0 "},
        lid_mask: {1, 1, 1, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C50E"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 14 Odyssey (18, 36, 73GB) */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST3   53L       "},
        compare_product_id_byte: {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C51A"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 15 Odyssey (146+GB) - fixme - not in drive list; remove from table? */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST3    53L      "},
        compare_product_id_byte: {1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C51A"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 16 Gemini (18, 36, 73GB) */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST3   07L       "},
        compare_product_id_byte: {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C50F"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 17 Gemini (146+GB) */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"ST3    07L      "},
        compare_product_id_byte: {1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"C50F"},
        min_ucode_mask: {1, 1, 1, 1}
    },
    {
        /* 18 Daytona */
        vendor_id: {"IBM     "},
        compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
        product_id: {"IC35L     DY    "},
        compare_product_id_byte: {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
        lid: {"    "},
        lid_mask: {0, 0, 0, 0},
        supported_with_min_ucode_level: true,
        min_ucode_level: {"S28C"},
        min_ucode_mask: {1, 1, 1, 1}
    }
};

void tool_init(char *tool_name)
{
  /* FIXME - host_no is the name of the proc file, dev_name = /dev/ipr* where * = minor number */
    DIR *proc_dir;
    struct dirent *dir_entry;
    char line[100], temp[100], proc_fname[100], *p_char, *p_temp, *p_temp2;
    int ccin;
    struct ipr_ioa *p_ipr_ioa;
    int main_menu();
    int rc, minor_num, major_num;
    dev_t major_minor;
    struct stat file_stat;

    /* Find all the IPR IOAs attached and save away vital info about them */
    proc_dir = opendir("/proc/scsi/ipr");

    if (proc_dir != NULL)
    {
        for (dir_entry = readdir(proc_dir);
             dir_entry != NULL;
             dir_entry = readdir(proc_dir))
        {
            if (dir_entry->d_name[0] == '.')
                continue;

            sprintf(proc_fname, "/proc/scsi/ipr/%s", dir_entry->d_name);

            /* Driver Version */
            if (get_proc_string(proc_fname, "Driver Version:", line) == 0)
            {
                exit_on_error("%s Unable to extract Driver Version"IPR_EOL,tool_name);
            }

            p_ipr_ioa = (struct ipr_ioa*)malloc(sizeof(struct ipr_ioa));
            memset(p_ipr_ioa,0,sizeof(struct ipr_ioa));
            p_temp = &line[16];

            p_temp2 = strchr(p_temp, '\n');
            if (p_temp2)
                *p_temp2 = '\0';
            strcpy(p_ipr_ioa->driver_version, p_temp);

            if ((driver_major = get_major_version(proc_fname)) == -1)
            {
                exit_on_error("%s Invalid Major Driver Version entry"IPR_EOL,tool_name);
            }

            if ((driver_minor = get_minor_version(proc_fname)) == -1)
            {
                exit_on_error("%s Invalid Minor Driver Version entry"IPR_EOL,tool_name);
            }

            if (driver_major != IPR_MAJOR_RELEASE)
            {
                sprintf(temp,"%s incompatible with current driver"IPR_EOL,tool_name);
                exit_on_error("%sDriver Supported Version: %d, %s version: %d"IPR_EOL,
                              temp, driver_major, tool_name, IPR_MAJOR_RELEASE);
            }

            /* Get the CCIN */
            if (get_proc_string(proc_fname, "IBM", line) == 0)
            {
                exit_on_error("%s: %s Unable to extract CCIN in driver %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }

            rc = sscanf(line, "IBM %x", &ccin);

            if (rc < 1)
            {
                exit_on_error("%s: %s Invalid CCIN entry in driver %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }

            /* Firmware Version */
            if (get_proc_string(proc_fname, "Firmware Version:", line) == 0)
            {
                exit_on_error("%s: %s Unable to extract Firmware Version %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }

            p_temp = &line[18];

            p_temp2 = strchr(p_temp, '\n');
            if (p_temp2)
                *p_temp2 = '\0';
            strcpy(p_ipr_ioa->firmware_version, p_temp);

            /* Generate the resource name */
            sprintf(p_ipr_ioa->ioa.dev_name, "/dev/ipr%d", driver_minor);

            strcpy(p_ipr_ioa->ioa.gen_name, p_ipr_ioa->ioa.dev_name);
            p_ipr_ioa->ccin = ccin;

            p_ipr_ioa->p_next = NULL;
            p_ipr_ioa->num_raid_cmds = 0;

            p_ipr_ioa->host_num = strtoul(dir_entry->d_name, NULL, 10);

            /* get major number here */
            if (get_proc_string(proc_fname, "Major Number:", line) == 0)
            {
                exit_on_error("%s: %s Unable to extract Major Number in driver %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }
            else
            {
                rc = sscanf(line, "Major Number: %d", &major_num);

                if (rc < 1)
                {
                    exit_on_error("%s: %s Invalid Major Number in driver %s"IPR_EOL,
                                  tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
                }
            }

            /* if /dev file exists verify major number */
            rc = stat(p_ipr_ioa->ioa.dev_name, &file_stat);
            if (rc || (MAJOR(file_stat.st_rdev) != major_num))
            {
                if (!rc)
                {
                    /* remove all ipr* files with old major number */
                    sprintf(temp,"rm %s",p_ipr_ioa->ioa.dev_name);
                    p_char = strstr(temp, "ipr");
                    p_char[6] = '*';
                    p_char[7] = '\0';
                    system(temp);
                }
                p_char = strstr(p_ipr_ioa->ioa.dev_name, "ipr");
                p_char += 6;
                minor_num = strtoul(p_char, NULL, 10);
                major_minor = MKDEV( major_num, minor_num);

                mknod(p_ipr_ioa->ioa.dev_name, S_IFCHR|S_IRUSR|S_IWUSR, major_minor);
            }

            /* PCI address */
            if (get_proc_string(proc_fname, "PCI Address:", line) == 0)
            {
                exit_on_error("%s: %s Unable to extract PCI Address from driver %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }

            rc = sscanf(line, "PCI Address: %s", p_ipr_ioa->pci_address);

            if (rc < 1)
            {
                exit_on_error("%s: %s Invalid PCI Address in driver %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }

            /* Get the serial number */
            if (get_proc_string(proc_fname, "Serial Number:", line) == 0)
            {
                exit_on_error("%s: %s Unable to extract Serial Number from driver %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }

            rc = sscanf(line, "Serial Number: %s", p_ipr_ioa->serial_num);

            if (rc < 1)
            {
                exit_on_error("%s: %s Invalid Serial Number in driver %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }

            /* Part Number */
            if (get_proc_string(proc_fname, "Card Part Number:", line) == 0)
            {
                exit_on_error("%s: %s Unable to extrace Card Part Number in driver %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }

            rc = sscanf(line, "Card Part Number: %s", p_ipr_ioa->part_num);

            if (rc < 1)
            {
                exit_on_error("%s: %s Invalid Card Part Number in driver %s"IPR_EOL,
                              tool_name, IPR_VERSION_STR, p_ipr_ioa->driver_version);
            }

            if (p_last_ipr_ioa)
            {
                p_last_ipr_ioa->p_next = p_ipr_ioa;
                p_last_ipr_ioa = p_ipr_ioa;
            }
            else
            {
                p_last_ipr_ioa = p_head_ipr_ioa = p_ipr_ioa;
            }
            num_ioas++;
        }
    }

    closedir(proc_dir);

    return;
}

int get_proc_string(char *proc_file_name, char *label, char *buffer)
{
	FILE *proc_file;
	char line[100];
	char old_line[100];
	int lines_different;

	memset(old_line, 0, 100);
	memset(line, 0, 100);
	proc_file = fopen(proc_file_name, "r");

	if (proc_file == NULL)
	{
            exit_on_error("Fatal Error! /proc entry unreadable"IPR_EOL);
	}

	do
	{
		strcpy(old_line, line);
		fgets(line, 100, proc_file);
		lines_different = strcmp(line,old_line);
	}while(strncmp(label, line, strlen(label)) && (line[0] != '\0') && lines_different);

	strncpy(buffer, line, strlen(line));
	fclose(proc_file);

	return ((line[0] != '\0') && lines_different);
}

/* Try to dynamically add this device */
int scan_device(struct ipr_res_addr resource_addr, u32 host_num)
{
    char cmd[200];

    sprintf(cmd, "echo \"scsi add-single-device %d %d %d %d\" > /proc/scsi/scsi",
            host_num,
            resource_addr.bus,
            resource_addr.target,
            resource_addr.lun);
    system(cmd);
    return 0;
}

/* Try to dynamically remove this device */
int remove_device(struct ipr_res_addr resource_addr, struct ipr_ioa *p_ioa)
{
    char cmd[200];

    sprintf(cmd, "echo \"scsi remove-single-device %d %d %d %d\" > /proc/scsi/scsi",
            p_ioa->host_num,
            resource_addr.bus,
            resource_addr.target,
            resource_addr.lun);
    system(cmd);
    return 0;
}

int ipr_start_array(int fd, struct ipr_array_query_data *p_qac_data,
                    int stripe_size, int prot_level, int hot_spare)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;
    int length = iprtoh16(p_qac_data->resp_len);

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + length);
    memcpy(&ioa_ioctl->buffer[0], (void *)p_qac_data, length);

    ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
    ioa_ioctl->buffer_len = length;
    ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
    ioa_ioctl->cmd_pkt.write_not_read = 1;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_START_ARRAY_PROTECTION;
    if (hot_spare)
        ioa_ioctl->cmd_pkt.cdb[1] = 0x01;

    ioa_ioctl->cmd_pkt.cdb[4] = (u8)(stripe_size >> 8);
    ioa_ioctl->cmd_pkt.cdb[5] = (u8)(stripe_size);
    ioa_ioctl->cmd_pkt.cdb[6] = prot_level;
    ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
    ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    free(ioa_ioctl);
    return rc;
}

int ipr_start_array_protection(int fd, struct ipr_array_query_data *p_qac_data,
                               int stripe_size, int prot_level)
{
    return ipr_start_array(fd, p_qac_data, stripe_size, prot_level, 0);
}

int ipr_add_hot_spare(int fd, struct ipr_array_query_data *p_qac_data)
{
    return ipr_start_array(fd, p_qac_data, 0, 0, 1);
}

int ipr_stop_array(int fd, struct ipr_array_query_data *p_qac_data, int hot_spare)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;
    int length = iprtoh16(p_qac_data->resp_len);

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + length);
    memcpy(&ioa_ioctl->buffer[0], (void *)p_qac_data, length);

    ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
    ioa_ioctl->buffer_len = length;
    ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
    ioa_ioctl->cmd_pkt.write_not_read = 1;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_STOP_ARRAY_PROTECTION;
    if (hot_spare)
        ioa_ioctl->cmd_pkt.cdb[1] = 0x01;

    ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
    ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    free(ioa_ioctl);
    return rc;
}

int ipr_stop_array_protection(int fd, struct ipr_array_query_data *p_qac_data)
{
    return ipr_stop_array(fd, p_qac_data, 0);
}

int ipr_remove_hot_spare(int fd, struct ipr_array_query_data *p_qac_data)
{
    return ipr_stop_array(fd, p_qac_data, 1);
}

int ipr_rebuild_device_data(int fd, struct ipr_array_query_data *p_qac_data)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;
    int length = iprtoh16(p_qac_data->resp_len);

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + length);
    memcpy(&ioa_ioctl->buffer[0], (void *)p_qac_data, length);

    ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
    ioa_ioctl->buffer_len = length;
    ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
    ioa_ioctl->cmd_pkt.write_not_read = 1;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_REBUILD_DEVICE_DATA;

    ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
    ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    free(ioa_ioctl);
    return rc;
}

int ipr_add_array_device(int fd, struct ipr_array_query_data *p_qac_data)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;
    int length = iprtoh16(p_qac_data->resp_len);

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + length);
    memcpy(&ioa_ioctl->buffer[0], (void *)p_qac_data, length);

    ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
    ioa_ioctl->buffer_len = length;
    ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
    ioa_ioctl->cmd_pkt.write_not_read = 1;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_ADD_ARRAY_DEVICE;

    ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
    ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    free(ioa_ioctl);
    return rc;
}

int ipr_query_command_status(int fd, u32 res_handle, void *buff)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_cmd_status));
    ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = sizeof(struct ipr_cmd_status);

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
    ioa_ioctl->cmd_pkt.write_not_read = 0;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_QUERY_COMMAND_STATUS;

    ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
    ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    memcpy(buff, &ioa_ioctl->buffer[0], sizeof(struct ipr_cmd_status));
    free(ioa_ioctl);

    return rc;
}

int ipr_query_resource_state(int fd, u32 res_handle, void *buff)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_query_res_state));
    ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = sizeof(struct ipr_query_res_state);

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
    ioa_ioctl->cmd_pkt.write_not_read = 0;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_QUERY_RESOURCE_STATE;

    ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
    ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    memcpy(buff, &ioa_ioctl->buffer[0], sizeof(struct ipr_query_res_state));
    free(ioa_ioctl);

    return rc;
}

int ipr_mode_sense(int fd, u8 page, u32 res_handle, void *buff)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + IPR_MODE_SENSE_LENGTH);
    ioa_ioctl->timeout_in_sec = IPR_INTERNAL_DEV_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = IPR_MODE_SENSE_LENGTH;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 0;
    ioa_ioctl->cmd_pkt.cdb[0] = MODE_SENSE;

    ioa_ioctl->cmd_pkt.cdb[2] = page;
    ioa_ioctl->cmd_pkt.cdb[4] = ioa_ioctl->buffer_len;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    memcpy(buff, &ioa_ioctl->buffer[0], IPR_MODE_SENSE_LENGTH);
    free(ioa_ioctl);

    return rc;
}

int ipr_mode_select(int fd, u32 res_handle, void *buff, int length)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + length);
    memcpy(&ioa_ioctl->buffer[0], buff, length);

    ioa_ioctl->timeout_in_sec = IPR_INTERNAL_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = length;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 1;
    ioa_ioctl->cmd_pkt.cdb[0] = MODE_SELECT;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);
    free(ioa_ioctl);

    return rc;
}

int ipr_read_capacity_16(int fd, u32 res_handle, void *buff)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_read_cap16));
    ioa_ioctl->timeout_in_sec = IPR_INTERNAL_DEV_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = sizeof(struct ipr_read_cap16);

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 0;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_SERVICE_ACTION_IN;
    ioa_ioctl->cmd_pkt.cdb[1] = IPR_READ_CAPACITY_16;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    memcpy(buff, &ioa_ioctl->buffer[0], sizeof(struct ipr_read_cap16));
    free(ioa_ioctl);

    return rc;
}

int ipr_read_capacity(int fd, u32 res_handle, void *buff)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_read_cap));
    ioa_ioctl->timeout_in_sec = IPR_INTERNAL_DEV_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = sizeof(struct ipr_read_cap);

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 0;
    ioa_ioctl->cmd_pkt.cdb[0] = READ_CAPACITY;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    memcpy(buff, &ioa_ioctl->buffer[0], sizeof(struct ipr_read_cap));
    free(ioa_ioctl);

    return rc;
}

int ipr_reclaim_cache_store(int fd, int action, void *buff)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    struct ipr_reclaim_cache_ioctl *reclaim_ioctl;
    int rc;

    if ((action & IPR_RECLAIM_PERFORM) == IPR_RECLAIM_PERFORM)
    {
        reclaim_ioctl = calloc(1,sizeof(struct ipr_reclaim_cache_ioctl) + sizeof(struct ipr_reclaim_query_data));
        reclaim_ioctl->buffer_len = sizeof(struct ipr_reclaim_query_data);
  
        rc = ioctl(fd, IPR_IOCTL_RECLAIM_CACHE, ioa_ioctl);

        memcpy(buff, &reclaim_ioctl->buffer[0], sizeof(struct ipr_reclaim_query_data));
    }
    else
    {
        ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_reclaim_query_data));
        ioa_ioctl->timeout_in_sec = IPR_INTERNAL_TIMEOUT;
        ioa_ioctl->buffer_len = sizeof(struct ipr_reclaim_query_data);
        ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

        ioa_ioctl->cmd_pkt.request_type =IPR_RQTYPE_IOACMD;
        ioa_ioctl->cmd_pkt.write_not_read = 0;
        ioa_ioctl->cmd_pkt.cdb[0] = IPR_RECLAIM_CACHE_STORE;
        ioa_ioctl->cmd_pkt.cdb[1] = (u8)action;

        rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

        memcpy(buff, &ioa_ioctl->buffer[0], sizeof(struct ipr_reclaim_query_data));
    }
    free(ioa_ioctl);
    return rc;
}

int ipr_start_stop_stop(int fd, u32 res_handle)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl));
    ioa_ioctl->timeout_in_sec = IPR_INTERNAL_DEV_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = 0;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 0;
    ioa_ioctl->cmd_pkt.cdb[0] = START_STOP;
    ioa_ioctl->cmd_pkt.cdb[4] = IPR_START_STOP_STOP;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    free(ioa_ioctl);
    return rc;
}

int ipr_format_unit(int fd, u32 res_handle)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;
    u8 *p_defect_list_hdr;

    ioa_ioctl = calloc(1, sizeof(struct ipr_passthru_ioctl) + IPR_DEFECT_LIST_HDR_LEN);
    ioa_ioctl->timeout_in_sec = IPR_FORMAT_UNIT_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = IPR_DEFECT_LIST_HDR_LEN;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 1;
    ioa_ioctl->cmd_pkt.cdb[0] = FORMAT_UNIT;
    ioa_ioctl->cmd_pkt.cdb[1] = IPR_FORMAT_DATA;
    ioa_ioctl->cmd_pkt.cdb[4] = 1;

    p_defect_list_hdr = &ioa_ioctl->buffer[0];
    p_defect_list_hdr[1] = IPR_FORMAT_IMMED;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    free(ioa_ioctl);
    return rc;
}

int ipr_evaluate_device(int fd, u32 res_handle)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;
    u32 resource_handle;

    resource_handle = iprtoh32(res_handle);

    ioa_ioctl = calloc(1, sizeof(struct ipr_passthru_ioctl));
    ioa_ioctl->timeout_in_sec = IPR_EVALUATE_DEVICE_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = 0;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_EVALUATE_DEVICE;
    ioa_ioctl->cmd_pkt.cdb[2] = (u8)(resource_handle >> 24);
    ioa_ioctl->cmd_pkt.cdb[3] = (u8)(resource_handle >> 16);
    ioa_ioctl->cmd_pkt.cdb[4] = (u8)(resource_handle >> 8);
    ioa_ioctl->cmd_pkt.cdb[5] = (u8)(resource_handle);

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    free(ioa_ioctl);
    return rc;
}

int ipr_mode_sense_page_28(int fd, void *buff)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_pagewh_28));
    ioa_ioctl->timeout_in_sec = IPR_INTERNAL_TIMEOUT;
    ioa_ioctl->buffer_len = sizeof(struct ipr_pagewh_28);
    ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 0;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_MODE_SENSE;
    ioa_ioctl->cmd_pkt.cdb[2] = 0x28;
    ioa_ioctl->cmd_pkt.cdb[4] = ioa_ioctl->buffer_len;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    memcpy(buff, &ioa_ioctl->buffer[0], sizeof(struct ipr_pagewh_28));

    free(ioa_ioctl);
    return rc;
}

int ipr_mode_select_page_28(int fd, void *buff)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;
    struct ipr_mode_parm_hdr *p_mode_parm_header;
    u8 length;

    p_mode_parm_header = (struct ipr_mode_parm_hdr *)buff; 
    length = p_mode_parm_header->length + 1;
    p_mode_parm_header->length = 0;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + length);
    memcpy(&ioa_ioctl->buffer[0], buff, length);

    ioa_ioctl->timeout_in_sec = IPR_INTERNAL_TIMEOUT;
    ioa_ioctl->buffer_len = length;
    ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;
    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 1;
    ioa_ioctl->cmd_pkt.cdb[0] = IPR_MODE_SELECT;
    ioa_ioctl->cmd_pkt.cdb[1] = 0x11;
    ioa_ioctl->cmd_pkt.cdb[4] = length;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    //FIXME!!! update driver as well.

    free(ioa_ioctl);
    return rc;
}

u32 ipr_max_xfer_rate(int fd, int bus)
{
    struct ipr_bus_attributes *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_bus_attributes));
    ioa_ioctl->bus = bus;

    rc = ioctl(fd, IPR_IOCTL_GET_BUS_CAPABILITIES, ioa_ioctl);

    free(ioa_ioctl);
    return htoipr32(ioa_ioctl->max_xfer_rate);
}

int ipr_inquiry(int fd, u32 res_handle, u8 page, void *buff, u8 length)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    struct sense_data_t sense_data;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + length);
    ioa_ioctl->timeout_in_sec = IPR_INTERNAL_DEV_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = length;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 0;
    ioa_ioctl->cmd_pkt.cdb[0] = INQUIRY;
    if (page != IPR_STD_INQUIRY)
    {
        ioa_ioctl->cmd_pkt.cdb[1] = 0x01;
        ioa_ioctl->cmd_pkt.cdb[2] = page;
    }
    ioa_ioctl->cmd_pkt.cdb[4] = length;

    if (res_handle) {
        rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);
        memcpy(buff, &ioa_ioctl->buffer[0], length);
    } else {
        rc = sg_ioctl(fd, &ioa_ioctl->cmd_pkt.cdb[0], buff,
                      length, SG_DXFER_FROM_DEV,
                      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);
    }

    free(ioa_ioctl);
    return rc;
}

int ipr_write_buffer(int fd, void *buff, u32 length)
{
    struct ipr_ucode_download_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_ucode_download_ioctl) + length);
    memcpy(&ioa_ioctl->buffer[0], buff, length);
    ioa_ioctl->buffer_len = length;

    rc = ioctl(fd, IPR_IOCTL_UCODE_DOWNLOAD, ioa_ioctl);

    free(ioa_ioctl);
    return rc;
}

int ipr_dev_write_buffer(int fd, u32 res_handle, void *buff, u32 length)
{
    struct ipr_passthru_ioctl *ioa_ioctl;
    int rc;

    ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + length);
    memcpy(&ioa_ioctl->buffer[0], buff, length);

    ioa_ioctl->timeout_in_sec = IPR_WRITE_BUFFER_TIMEOUT;
    ioa_ioctl->res_handle = res_handle;
    ioa_ioctl->buffer_len = length;

    ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_SCSICDB;
    ioa_ioctl->cmd_pkt.write_not_read = 1;
    ioa_ioctl->cmd_pkt.cdb[0] = WRITE_BUFFER;
    ioa_ioctl->cmd_pkt.cdb[1] = 5;
    ioa_ioctl->cmd_pkt.cdb[6] = (ioa_ioctl->buffer_len & 0xff0000) >> 16;
    ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0x00ff00) >> 8;
    ioa_ioctl->cmd_pkt.cdb[8]= ioa_ioctl->buffer_len & 0x0000ff;

    rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

    free(ioa_ioctl);
    return rc;
}

#define IPR_MAX_XFER 0x8000
int sg_ioctl(int fd, u8 cdb[IPR_CCB_CDB_LEN],
             void *p_data, u32 xfer_len, u32 data_direction,
             struct sense_data_t *p_sense_data,
             u32 timeout_in_sec)
{
    int rc;
    sg_io_hdr_t io_hdr_t;
    sg_iovec_t *p_iovec;
    int iovec_count;
    int i;
    int buff_len, segment_size;
    void *dxferp;

    /* check if scatter gather should be used */
    if (xfer_len > IPR_MAX_XFER)
    {
        iovec_count = xfer_len/IPR_MAX_XFER + 1;
        p_iovec = malloc(iovec_count * sizeof(sg_iovec_t));

        buff_len = xfer_len;
        segment_size = IPR_MAX_XFER;

        for (i = 0; (i < iovec_count) && (buff_len != 0); i++)
        {
            
            p_iovec[i].iov_base = malloc(IPR_MAX_XFER);
            memcpy(p_iovec[i].iov_base, p_data + IPR_MAX_XFER * i, IPR_MAX_XFER);
            p_iovec[i].iov_len = segment_size;

            buff_len -= segment_size;
            if (buff_len < segment_size)
                segment_size = buff_len;
        }
        iovec_count = i;
        dxferp = (void *)p_iovec;
    }
    else
    {
        iovec_count = 0;
        dxferp = p_data;
    }

    for (i = 0; i < 2; i++)
    {
        memset(&io_hdr_t, 0, sizeof(io_hdr_t));
        memset(p_sense_data, 0, sizeof(struct sense_data_t));

        io_hdr_t.interface_id = 'S';
        if (cdb[0] == WRITE_BUFFER)
            io_hdr_t.cmd_len = 10;
        else
            io_hdr_t.cmd_len = 6;
        io_hdr_t.iovec_count = iovec_count;
        io_hdr_t.flags = 0;
        io_hdr_t.pack_id = 0;
        io_hdr_t.usr_ptr = 0;
        io_hdr_t.sbp = (unsigned char *)p_sense_data;
        io_hdr_t.mx_sb_len = sizeof(struct sense_data_t);
        io_hdr_t.timeout = timeout_in_sec * 1000;
        io_hdr_t.cmdp = cdb;
        io_hdr_t.dxfer_direction = data_direction;
        io_hdr_t.dxfer_len = xfer_len;
        io_hdr_t.dxferp = dxferp;

        rc = ioctl(fd, SG_IO, &io_hdr_t);

        if ((rc == 0) && (io_hdr_t.masked_status == CHECK_CONDITION))
            rc = CHECK_CONDITION;

        if ((rc != CHECK_CONDITION) || (p_sense_data->sense_key != UNIT_ATTENTION))
            break;
    }

    if (iovec_count)
    {
        for (i = 0; i < iovec_count; i++)
            free(p_iovec[i].iov_base);
        free(p_iovec);
    }

    return rc;
};

/* Will return -1 on error */
int get_major_version(char *proc_file_name)
{
	char line[100];
	char *p_temp, *p_temp2;

	if (get_proc_string(proc_file_name, "Driver Version:", line) == 0)
		return -1;

	p_temp = &line[16];

	p_temp2 = strchr(p_temp, '\n');
	if (p_temp2)
		*p_temp2 = '\0';

	p_temp2 = strchr(p_temp, '.');
	*p_temp2 = '\0';

	return strtoul(p_temp, NULL, 10);
}

/* Will return -1 on error */
int get_minor_version(char *proc_file_name)
{
	char line[100];
	char *p_temp, *p_temp2;

	if (get_proc_string(proc_file_name, "Driver Version:", line) == 0)
		return -1;

	p_temp = &line[18];

	p_temp2 = strchr(p_temp, '\n');
	if (p_temp2)
		*p_temp2 = '\0';

	p_temp2 = strchr(p_temp, '.');
	*p_temp2 = '\0';

	return strtoul(p_temp, NULL, 10);
}

#define MAX_SG_DEVS 128
#define MAX_SD_DEVS 128

static void make_dev_name(char * fname, const char * leadin, int k, 
                          int do_numeric)
{
    char buff[64];
    int  big,little;

    strcpy(fname, leadin ? leadin : "/dev/sg");
    if (do_numeric) {
        sprintf(buff, "%d", k);
        strcat(fname, buff);
    }
    else {
        if (k < 26) {
            buff[0] = 'a' + (char)k;
            buff[1] = '\0';
            strcat(fname, buff);
        }
        else if (k <= 255) { /* assumes sequence goes x,y,z,aa,ab,ac etc */
            big    = k/26;
            little = k - (26 * big);
            big    = big - 1;

            buff[0] = 'a' + (char)big;
            buff[1] = 'a' + (char)little;
            buff[2] = '\0';
            strcat(fname, buff);
        }
        else
            strcat(fname, "xxxx");
    }
}

/* returns 0 on success, 1 on failure  to find major and minor
 number for the given device name */
int get_sd_proc_data(char *sd_name, int *major_num, int *minor_num)
{
    FILE *proc_file;
    char proc_file_name[] = "/proc/partitions";
    int rc;
    int local_major_num, local_minor_num;
    char line[100];
    char old_line[100];
    int lines_different;

    proc_file = fopen(proc_file_name, "r");

    if (proc_file == NULL)
        return 1;
    do
    {
        strcpy(old_line,line);
        fgets(line, 100, proc_file);
        lines_different = strcmp(line,old_line);
        if (strstr(line, sd_name))
        {
            sscanf(line,"%d %d",
                   &local_major_num,
                   &local_minor_num);
            *major_num = local_major_num;
            *minor_num = local_minor_num;
            fclose(proc_file);
            return 0;
        }
    }while ((rc != EOF) && (lines_different));

    fclose(proc_file);
    return 1;
}

void get_sg_ioctl_data(struct sg_map_info *p_sg_map_info, int num_devs)
{
    int sg_index, sd_index, fd, rc;
    int num_sg_devices = 0;
    struct sg_map_info *temp;
    char tmp_dev_name[64];
    struct
    {
        u32 dev_id;
        u32 unique_id;
    } dev_idlun;
    int sd_major, sd_minor;
    char *sd_name;
    struct stat sda_stat;
    int max_sd_devs = MAX_SD_DEVS;
    int temp_max_sd_devs;
    int used_sd[MAX_SD_DEVS];

    memset(used_sd, 0, sizeof(used_sd));

    for (sg_index = 0;
         (sg_index < MAX_SG_DEVS) && (num_sg_devices < num_devs);
         sg_index++)
    {
        temp = &p_sg_map_info[num_sg_devices];

        memset(p_sg_map_info[num_sg_devices].gen_name, 0, sizeof(p_sg_map_info[num_sg_devices].gen_name));
        make_dev_name(p_sg_map_info[num_sg_devices].gen_name, "/dev/sg", sg_index, 1);
        fd = open(p_sg_map_info[num_sg_devices].gen_name, O_RDONLY | O_NONBLOCK);

        if (fd < 0)
        {
            if (errno == ENOENT)
            {
                mknod(p_sg_map_info[num_sg_devices].gen_name,
                      S_IFCHR|0660,
                      makedev(21,sg_index));

                /* change permissions and owner to be identical to
                 /dev/sg0's */
                stat("/dev/sg0", &sda_stat);
                chown(p_sg_map_info[num_sg_devices].gen_name,
                      sda_stat.st_uid,
                      sda_stat.st_gid);
                chmod(p_sg_map_info[num_sg_devices].gen_name,
                      sda_stat.st_mode);

                fd = open(p_sg_map_info[num_sg_devices].gen_name, O_RDONLY | O_NONBLOCK);
                if (fd < 0)
                    continue;
            }
            else
                continue;
        }

        rc = ioctl(fd, SG_GET_SCSI_ID, &p_sg_map_info[num_sg_devices].sg_dat);

        close(fd);
        if ((rc >= 0) &&
            (p_sg_map_info[num_sg_devices].sg_dat.scsi_type == TYPE_DISK))
        {
            temp_max_sd_devs = 0;

            memset(p_sg_map_info[num_sg_devices].dev_name, 0, sizeof(p_sg_map_info[num_sg_devices].dev_name));
            for (sd_index = 0; sd_index < max_sd_devs; sd_index++)
            {
                if (used_sd[sd_index])
                    continue;

                memset(tmp_dev_name, 0, sizeof(tmp_dev_name));
                make_dev_name(tmp_dev_name, "/dev/sd", sd_index, 0);

                fd = open(tmp_dev_name, O_RDONLY | O_NONBLOCK);
                if (fd < 0)
                {
                    if (errno == ENOENT)
                    {
                        /* find major and minor number from /proc/partitions */
                        sd_name = strstr(tmp_dev_name, "sd");
                        rc = get_sd_proc_data(sd_name, &sd_major, &sd_minor);

                        if (rc)
                            continue;

                        mknod(tmp_dev_name,
                              S_IFBLK|0660,
                              makedev(sd_major,sd_minor));

                        /* change permissions and owner to be identical to
                         /dev/sda's */
                        stat("/dev/sda", &sda_stat);
                        chown(tmp_dev_name,
                              sda_stat.st_uid,
                              sda_stat.st_gid);
                        chmod(tmp_dev_name,
                              sda_stat.st_mode);

                        fd = open(tmp_dev_name, O_RDONLY | O_NONBLOCK);
                        if (fd < 0)
                            continue;
                    }
                    else
                        continue;
                }
                temp_max_sd_devs = sd_index + 1;

                rc = ioctl(fd, SCSI_IOCTL_GET_IDLUN, &dev_idlun);
                close(fd);
                if (rc < 0)
                {
                    continue;
                }

                if ((p_sg_map_info[num_sg_devices].sg_dat.host_no == ((dev_idlun.dev_id >> 24) & 0xFF)) &&
                    (p_sg_map_info[num_sg_devices].sg_dat.channel == ((dev_idlun.dev_id >> 16) & 0xFF)) &&
                    (p_sg_map_info[num_sg_devices].sg_dat.scsi_id == (dev_idlun.dev_id & 0xFF)) &&
                    (p_sg_map_info[num_sg_devices].sg_dat.lun     == ((dev_idlun.dev_id >>  8) & 0xFF)))
                {
                    strcpy(p_sg_map_info[num_sg_devices].dev_name, tmp_dev_name);
                    temp_max_sd_devs = max_sd_devs;
                    used_sd[sd_index] = 1;
                    break;
                }
            }
            max_sd_devs = temp_max_sd_devs;
            num_sg_devices++;
        }
    }
    return;
}

int get_sg_proc_data(struct sg_proc_info *p_sg_proc_info)
{
    FILE *proc_file;
    char proc_file_name[] = "/proc/scsi/sg/devices";
    int rc;
    int num_devs = 0;

    proc_file = fopen(proc_file_name, "r");

    if (proc_file == NULL)
    {
        system("modprobe sg");
        proc_file = fopen(proc_file_name, "r");
    }

    if (proc_file == NULL)
        return 0;
    do
    {
        rc = fscanf(proc_file,"%d %d %d %d %d %d %d %d %d",
                    &p_sg_proc_info->host,  &p_sg_proc_info->channel,
                    &p_sg_proc_info->id,    &p_sg_proc_info->lun,
                    &p_sg_proc_info->type,  &p_sg_proc_info->opens,
                    &p_sg_proc_info->qdepth,&p_sg_proc_info->busy,
                    &p_sg_proc_info->online);
        if (rc > 0)
        {
            if (p_sg_proc_info->type == TYPE_DISK)
            {
                p_sg_proc_info++;
                num_devs++;
            }
        }
    } while(rc != EOF);

    fclose(proc_file);
    return num_devs;
}

void check_current_config(bool allow_rebuild_refresh)
{
    struct sg_map_info *p_sg_map_info;
    struct sg_proc_info *p_sg_proc_info;
    int  num_sg_devices, fd, rc, device_count, j, k;
    struct ipr_ioa *p_cur_ioa;
    struct ipr_passthru_ioctl *ioa_ioctl;
    struct ipr_query_config_ioctl *query_ioctl;
    struct ipr_array_query_data  *p_cur_array_query_data;
    struct ipr_resource_table *p_cur_res_table;
    struct ipr_resource_entry res_entry;
    struct ipr_record_common *p_common_record;
    struct ipr_device_record *p_device_record;
    struct ipr_array_record *p_array_record;
    int found, tried_scsi_add, tried_everything;

    /* Allocate memory on init */
    if (p_res_table == NULL)
    {
        p_res_table =
            (struct ipr_resource_table *)
            malloc((sizeof(struct ipr_resource_table) +
                    sizeof(struct ipr_resource_entry) *
                    IPR_MAX_PHYSICAL_DEVS) * num_ioas);
    }

    if (p_ipr_array_query_data == NULL)
    {
        p_ipr_array_query_data =
            (struct ipr_array_query_data *)
            malloc(sizeof(struct ipr_array_query_data) * num_ioas);
    }

    p_sg_map_info = (struct sg_map_info *)malloc(sizeof(struct sg_map_info) * MAX_SG_DEVS);
    p_sg_proc_info = (struct sg_proc_info *)malloc(sizeof(struct sg_proc_info) * MAX_SG_DEVS);

    /* Get sg data via sg proc file system */
    num_sg_devices = get_sg_proc_data(p_sg_proc_info);
    get_sg_ioctl_data(p_sg_map_info, num_sg_devices);

    for(p_cur_ioa = p_head_ipr_ioa,
        p_cur_res_table = p_res_table,
        p_cur_array_query_data = p_ipr_array_query_data;
        p_cur_ioa != NULL;
        p_cur_ioa = p_cur_ioa->p_next,
        p_cur_res_table++,
        p_cur_array_query_data++)
    {
        p_cur_ioa->num_devices = 0;

        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
        if (fd <= 1)
        {
            syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL,
                   p_cur_ioa->ioa.dev_name);
            p_cur_ioa->num_devices = 0;
            continue;
        }

        /* Get Query Array Config Data */
        ioa_ioctl = calloc(1,sizeof(struct ipr_passthru_ioctl) + sizeof(struct ipr_array_query_data));
        ioa_ioctl->timeout_in_sec = IPR_ARRAY_CMD_TIMEOUT;
        ioa_ioctl->buffer_len = sizeof(struct ipr_array_query_data);
        ioa_ioctl->res_handle = IPR_IOA_RESOURCE_HANDLE;

        ioa_ioctl->cmd_pkt.request_type = IPR_RQTYPE_IOACMD;
        ioa_ioctl->cmd_pkt.write_not_read = 0;
        ioa_ioctl->cmd_pkt.cdb[0] = IPR_QUERY_ARRAY_CONFIG;

        if (allow_rebuild_refresh)
            ioa_ioctl->cmd_pkt.cdb[1] = 0;
        else
            ioa_ioctl->cmd_pkt.cdb[1] = 0x80; /* Prohibit Rebuild Candidate Refresh */

        ioa_ioctl->cmd_pkt.cdb[7] = (ioa_ioctl->buffer_len & 0xff00) >> 8;
        ioa_ioctl->cmd_pkt.cdb[8] = ioa_ioctl->buffer_len & 0xff;

        rc = ioctl(fd, IPR_IOCTL_PASSTHRU, ioa_ioctl);

        if (rc != 0)
        {
            if (errno != EINVAL)
                syslog(LOG_ERR,"Query Array Config to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);

            p_cur_array_query_data->num_records = 0;
        }
        else 
        {
            memcpy(p_cur_array_query_data,&ioa_ioctl->buffer[0], sizeof(struct ipr_array_query_data));
        }

        free(ioa_ioctl);

        p_cur_ioa->p_qac_data = p_cur_array_query_data;

        /* Get Query IOA Config Data */
        query_ioctl = calloc(1,sizeof(struct ipr_query_config_ioctl) + sizeof(struct ipr_resource_table));
        query_ioctl->buffer_len = sizeof(struct ipr_resource_table);

        rc = ioctl(fd, IPR_IOCTL_QUERY_CONFIGURATION, query_ioctl);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Query IOA Config to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            close(fd);
            continue;
        }

        else 
        {
            memcpy(p_cur_res_table,&query_ioctl->buffer[0], sizeof(struct ipr_resource_table));
        }

        free(query_ioctl);

        p_cur_ioa->p_resource_table = p_cur_res_table;
        close(fd);

        device_count = 0;
        p_cur_ioa->dev = (struct ipr_device *)
            realloc(p_cur_ioa->dev,
                    p_cur_res_table->hdr.num_entries * sizeof(struct ipr_device));
        memset(p_cur_ioa->dev, 0, p_cur_res_table->hdr.num_entries * sizeof(struct ipr_device));

        /* now assemble data pertaining to each individual device */
        for (j = 0; j < p_cur_res_table->hdr.num_entries; j++)
        {
            if ((IPR_IS_DASD_DEVICE(p_cur_res_table->dev[j].std_inq_data))
                && !p_cur_res_table->dev[j].is_ioa_resource)
            {
                p_cur_ioa->dev[device_count].p_resource_entry = &p_cur_res_table->dev[j];
                p_cur_ioa->dev[device_count].p_qac_entry = NULL;
                strcpy(p_cur_ioa->dev[device_count].dev_name, "");
                strcpy(p_cur_ioa->dev[device_count].gen_name, "");
                p_cur_ioa->dev[device_count].opens = 0;

                res_entry = p_cur_res_table->dev[j];

                /* find array config data matching resource entry */
                p_common_record = (struct ipr_record_common *)p_cur_array_query_data->data;
                for (k = 0; k < p_cur_array_query_data->num_records; k++)
                {
                    p_device_record = (struct ipr_device_record *)p_common_record;
                    if (p_device_record->resource_handle == res_entry.resource_handle)
                    {
                        p_cur_ioa->dev[device_count].p_qac_entry = p_common_record;
                        break;
                    }

                    p_common_record = (struct ipr_record_common *)
                        ((unsigned long)p_common_record + iprtoh16(p_common_record->record_len));
                }

                /* find sg_map_info matching resource entry */
                found = 0;
                tried_scsi_add = 0;
                tried_everything = 0;
                while (!found && !tried_everything && !ipr_is_hidden(&res_entry))
                {
                    for (k = 0; k < num_sg_devices; k++)
                    {
                        if ((p_cur_ioa->host_num               == p_sg_map_info[k].sg_dat.host_no) &&
                            (res_entry.resource_address.bus    == p_sg_map_info[k].sg_dat.channel) &&
                            (res_entry.resource_address.target == p_sg_map_info[k].sg_dat.scsi_id) &&
                            (res_entry.resource_address.lun    == p_sg_map_info[k].sg_dat.lun))
                        {
                            /* copy device name to main structure */
                            strcpy(p_cur_ioa->dev[device_count].dev_name, p_sg_map_info[k].dev_name);
                            strcpy(p_cur_ioa->dev[device_count].gen_name, p_sg_map_info[k].gen_name);

                            found = 1;
                            break;
                        }
                    }

                    if (!found)
                    {
                        if (!tried_scsi_add)
                        {
                            /* try adding scsi device then rechecking */
                            remove_device(res_entry.resource_address, p_cur_ioa);
                            scan_device(res_entry.resource_address,
                                        p_cur_ioa->host_num);
                            /* sleep(1);  give mid-layer a moment to process */

                            /* Re-Get sg data via SG_GET_SCSI_ID ioctl 
                             get_sg_ioctl_data(p_sg_map_info, num_sg_devices); */

                            tried_scsi_add = 1;
                        }
                        else
                        {
                            tried_everything = 1;

                            /* log error 
                             syslog(LOG_ERR,"Device not found host %d, bus %d, target %d, lun %d",
                             p_cur_ioa->host_num,
                             res_entry.resource_address.bus,
                             res_entry.resource_address.target,
                             res_entry.resource_address.lun); */
                        }
                    }
                }

                /* find usage counts in sg_proc_info */
                found = 0;
                for (k=0; k < num_sg_devices; k++)
                {
                    if ((p_cur_ioa->host_num               == p_sg_proc_info[k].host) &&
                        (res_entry.resource_address.bus    == p_sg_proc_info[k].channel) &&
                        (res_entry.resource_address.target == p_sg_proc_info[k].id) &&
                        (res_entry.resource_address.lun    == p_sg_proc_info[k].lun))
                    {
                        p_cur_ioa->dev[device_count].opens = p_sg_proc_info[k].opens;
                        found = 1;
                        break;
                    }
                }
                device_count++;
            }
            else if (p_cur_res_table->dev[j].is_ioa_resource)
            {
                p_cur_ioa->ioa.p_resource_entry = &p_cur_res_table->dev[j];
            }
        }

        /* now go through query array data inserting Array Record Type 1 and 2
         data putting the IOA as the resource owner */
        p_array_record = (struct ipr_array_record *)p_cur_array_query_data->data;
        for (k = 0; k < p_cur_array_query_data->num_records; k++)
        {
            if ((p_array_record->common.record_id == IPR_RECORD_ID_ARRAY_RECORD) ||
                ((p_array_record->common.record_id == IPR_RECORD_ID_ARRAY2_RECORD) &&
                 (p_array_record->start_cand)))
            {
                strcpy(p_cur_ioa->dev[device_count].dev_name, p_cur_ioa->ioa.dev_name);
                strcpy(p_cur_ioa->dev[device_count].gen_name, p_cur_ioa->ioa.gen_name);
                p_cur_ioa->dev[device_count].p_resource_entry = p_cur_ioa->ioa.p_resource_entry;
                p_cur_ioa->dev[device_count].p_qac_entry = (struct ipr_record_common *)p_array_record;
                p_cur_ioa->dev[device_count].opens = 0;
                device_count++;
            }
            else if (p_array_record->common.record_id == IPR_RECORD_ID_SUPPORTED_ARRAYS)
            {
                p_cur_ioa->p_supported_arrays = (struct ipr_supported_arrays *)p_array_record;
            }

            p_array_record = (struct ipr_array_record *)
                ((unsigned long)p_array_record + iprtoh16(p_array_record->common.record_len));
        }

        p_cur_ioa->num_devices = device_count;
    }
    free(p_sg_map_info);
    free(p_sg_proc_info);
}

int num_device_opens(int host_num, int channel, int id, int lun)
{
    struct sg_proc_info *p_sg_proc_info;
    int opens = 0;
    int k, num_sg_devices;

    p_sg_proc_info = (struct sg_proc_info *)malloc(sizeof(struct sg_proc_info) * MAX_SG_DEVS);

    /* Get sg data via sg proc file system */
    num_sg_devices = get_sg_proc_data(p_sg_proc_info);

    /* find usage counts in sg_proc_info */
    for (k=0; k < num_sg_devices; k++)
    {
        if ((host_num == p_sg_proc_info[k].host) &&
            (channel  == p_sg_proc_info[k].channel) &&
            (id       == p_sg_proc_info[k].id) &&
            (lun      == p_sg_proc_info[k].lun))
        {
            opens = p_sg_proc_info[k].opens;
            break;
        }
    }

    free(p_sg_proc_info);
    return opens;
}

void exit_on_error(char *s, ...)
{
    va_list args;
    char usr_str[256];

    va_start(args, s);
    vsprintf(usr_str, s, args);
    va_end(args);

    closelog();
    openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_ERR,"%s",usr_str);
    closelog();
    openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);

    exit(1);
}

void ipr_config_file_hdr(char *file_name)
{
    FILE *fd;
    char cmd_str[64];

    /* check if file currently exists */
    fd = fopen(file_name, "r");
    if (fd)
    {
	fclose(fd);
	return;
    }

    /* be sure directory is present */
    sprintf(cmd_str,"install -d %s",IPR_CONFIG_DIR);
    system(cmd_str);

    /* otherwise, create new file */
    fd = fopen(file_name, "w");
    if (fd == NULL)
    {
	syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, file_name);
	return;
    }

    fprintf(fd,"# DO NOT EDIT! Software generated configuration file for"IPR_EOL"# ipr SCSI device subsystem"IPR_EOL);
    fprintf(fd, IPR_EOL);
    fprintf(fd,"# Use iprconfig to configure"IPR_EOL);
    fprintf(fd,"# See \"man iprconfig\" for more information"IPR_EOL);
    fprintf(fd, IPR_EOL);
    fclose(fd);
}

void  ipr_config_file_entry(char *usr_file_name,
                            char *category,
                            char *field,
                            char *value,
                            int update)
{
    FILE *fd, *temp_fd;
    char file_name[64];
    char temp_file_name[64];
    char line[64];
    int category_found = 0;
    int field_found = 0;

    sprintf(file_name,"%s%s",IPR_CONFIG_DIR, usr_file_name);

    /* verify common header comments */
    ipr_config_file_hdr(file_name);

    fd = fopen(file_name, "r");

    if (fd == NULL)
        return;

    sprintf(temp_file_name,"%s.1",file_name);
    temp_fd = fopen(temp_file_name, "w");
    if (temp_fd == NULL)
    {
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, temp_file_name);
        return;
    }

    while (NULL != fgets(line,64,fd))
    {
        if ((strstr(line, category)) &&
            (line[0] != '#'))
        {
            category_found = 1;
        }
        else
        {
            if (category_found)
            {
                if (line[0] == '[')
                {
                    category_found = 0;
                    if (!field_found)
                    {
                        if (!update)
                            fprintf(temp_fd, "# ");

                        fprintf(temp_fd,"%s %s"IPR_EOL,field,value);
                    }
                }

                if (strstr(line, field))
                {
                    if (update)
                        sprintf(line,"%s %s"IPR_EOL,field,value);

                    field_found = 1;
                }
            }
        }

        fputs(line, temp_fd);
    } 

    if (!field_found)
    {
        if (!category_found)
        {
            fprintf(temp_fd,IPR_EOL"%s"IPR_EOL, category);
        }

        if (!update)
            fprintf(temp_fd, "# ");

        fprintf(temp_fd,"%s %s"IPR_EOL, field, value);
    }

    sprintf(line,"mv %s %s", temp_file_name, file_name);
    system(line);

    fclose(fd);
    fclose(temp_fd);
}

int  ipr_config_file_read(char *usr_file_name,
                          char *category,
                          char *field,
                          char *value)
{
    FILE *fd;
    char file_name[64];
    char line[64];
    char *str_ptr;
    int category_found;
    int rc = RC_FAILED;

    sprintf(file_name,"%s%s",IPR_CONFIG_DIR, usr_file_name);

    fd = fopen(file_name, "r");
    if (fd == NULL)
    {
        return RC_FAILED;
    }

    while (NULL != fgets(line,64,fd))
    {
        if (line[0] != '#')
        {
            if (strstr(line, category))
            {
                category_found = 1;
            }
            else
            {
                if (category_found)
                {
                    if (line[0] == '[')
                    {
                        category_found = 0;
                    }

                    if ((str_ptr = strstr(line, field)))
                    {
                        str_ptr += strlen(field);
                        while (str_ptr[0] == ' ')
                            str_ptr++;
                        sprintf(value,"%s"IPR_EOL,str_ptr);
                        rc = RC_SUCCESS;
                        break;
                    }
                }
            }
        }
    } 

    fclose(fd);
    return rc;
}

int ipr_config_file_valid(char *usr_file_name)
{
    FILE *fd;
    char file_name[64];

    sprintf(file_name,"%s%s",IPR_CONFIG_DIR, usr_file_name);

    fd = fopen(file_name, "r");
    if (fd == NULL)
    {
        return RC_FAILED;
    }

    fclose(fd);
    return RC_SUCCESS;
}

void ipr_save_page_28(struct ipr_ioa *p_ioa,
                      struct ipr_page_28 *p_page_28_cur,
                      struct ipr_page_28 *p_page_28_chg,
                      struct ipr_page_28 *p_page_28_ipr)
{
    char file_name[64];
    char category[16];
    char value_str[16];
    void ipr_config_file_hdr(char *file_name);
    int i;
    int update_entry;

    /* gets Host Address to create file_name */
    sprintf(file_name,"%x_%s",
            p_ioa->ccin, p_ioa->pci_address);

    for (i = 0;
         i < p_page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        if (p_page_28_chg->attr[i].qas_capability)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    p_page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    p_page_28_cur->attr[i].qas_capability);

            if (p_page_28_ipr->attr[i].qas_capability)
                update_entry = 1;
            else
                update_entry = 0;

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_QAS_CAPABILITY,
                                  value_str,
                                  update_entry);
        }

        if (p_page_28_chg->attr[i].scsi_id)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    p_page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    p_page_28_cur->attr[i].scsi_id);

            if (p_page_28_ipr->attr[i].scsi_id)
                update_entry = 1;
            else
                update_entry = 0;

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_HOST_SCSI_ID,
                                  value_str,
                                  update_entry);
        }

        if (p_page_28_chg->attr[i].bus_width)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    p_page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    p_page_28_cur->attr[i].bus_width);

            if (p_page_28_ipr->attr[i].bus_width)
                update_entry = 1;
            else
                update_entry = 0;

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_BUS_WIDTH,
                                  value_str,
                                  update_entry);
        }

        if (p_page_28_chg->attr[i].max_xfer_rate)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    p_page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    (iprtoh32(p_page_28_cur->attr[i].max_xfer_rate) *
                     (p_page_28_cur->attr[i].bus_width / 8))/10);

            if (p_page_28_ipr->attr[i].max_xfer_rate)
                update_entry = 1;
            else
                update_entry = 0;

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_MAX_XFER_RATE,
                                  value_str,
                                  update_entry);
        }

        if (p_page_28_chg->attr[i].min_time_delay)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    p_page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    p_page_28_cur->attr[i].min_time_delay);

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_MIN_TIME_DELAY,
                                  value_str,
                                  0);
        }
    }
}

void ipr_set_page_28(struct ipr_ioa *p_cur_ioa,
                     int limited_config,
                     int reset_scheduled)
{
    int rc, i;
    char current_mode_settings[255];
    char changeable_mode_settings[255];
    char default_mode_settings[255];
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    int fd;
    struct ipr_pagewh_28 *p_page_28_sense;
    struct ipr_page_28 *p_page_28_cur, *p_page_28_chg, *p_page_28_dflt;
    int is_reset_req = 0;
    char value_str[64];
    char file_name[64];
    char category[16];
    int new_value;
    u32 max_xfer_rate;

    memset(current_mode_settings,0,sizeof(current_mode_settings));
    memset(changeable_mode_settings,0,sizeof(changeable_mode_settings));
    memset(default_mode_settings,0,sizeof(default_mode_settings));

    /* mode sense page28 to get current parms */
    fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
        return;
    }

    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)current_mode_settings;
    rc = ipr_mode_sense_page_28(fd, p_mode_parm_hdr);

    if (rc != 0)
    {
        close(fd);
        syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
               p_cur_ioa->ioa.dev_name);
        return;
    }

    p_page_28_sense = (struct ipr_pagewh_28 *)p_mode_parm_hdr;
    p_page_28_cur = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* now issue mode sense to get changeable parms */
    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)changeable_mode_settings;
    p_page_28_chg = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* Now issue mode sense to get default parms */
    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)default_mode_settings;
    p_page_28_dflt = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* determine changable and default values */
    for (i = 0;
         i < p_page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        p_page_28_chg->attr[i].qas_capability = 0;
        p_page_28_chg->attr[i].scsi_id = 0; //FIXME!!! need to allow dart (by
                                            // vend/prod & subsystem id)
        p_page_28_chg->attr[i].bus_width = 1;
        p_page_28_chg->attr[i].max_xfer_rate = 1;

        p_page_28_dflt->attr[i].qas_capability = 0;
        p_page_28_dflt->attr[i].scsi_id = p_page_28_cur->attr[i].scsi_id;
        p_page_28_dflt->attr[i].bus_width = 16;
        p_page_28_dflt->attr[i].max_xfer_rate = ipr_max_xfer_rate(fd, p_page_28_cur->attr[i].res_addr.bus);
    }

    /* extract data from saved file to send in mode select */
    sprintf(file_name,"%x_%s",
            p_cur_ioa->ccin, p_cur_ioa->pci_address);

    for (i = 0;
         i < p_page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        if (p_page_28_chg->attr[i].qas_capability)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    p_page_28_cur->attr[i].res_addr.bus);

            rc = ipr_config_file_read(file_name,
                                      category,
                                      IPR_QAS_CAPABILITY,
                                      value_str);
            if (rc == RC_SUCCESS)
            {
                sscanf(value_str,"%d", &new_value);
                p_page_28_cur->attr[i].qas_capability = new_value;
            }
        }

        if (p_page_28_chg->attr[i].scsi_id)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    p_page_28_cur->attr[i].res_addr.bus);

            rc = ipr_config_file_read(file_name,
                                      category,
                                      IPR_HOST_SCSI_ID,
                                      value_str);
            if (rc == RC_SUCCESS)
            {
                sscanf(value_str,"%d", &new_value);
                if (p_page_28_cur->attr[i].scsi_id != new_value)
                {
                    is_reset_req = 1;
                    p_page_28_cur->attr[i].scsi_id = new_value;
                }
            }
        }

        if (p_page_28_chg->attr[i].bus_width)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    p_page_28_cur->attr[i].res_addr.bus);

            rc = ipr_config_file_read(file_name,
                                      category,
                                      IPR_BUS_WIDTH,
                                      value_str);
            if (rc == RC_SUCCESS)
            {
                sscanf(value_str,"%d", &new_value);
                p_page_28_cur->attr[i].bus_width = new_value;
            }
        }

        if (p_page_28_chg->attr[i].max_xfer_rate)
        {
            if (limited_config & (1 << p_page_28_cur->attr[i].res_addr.bus))
            {
                max_xfer_rate = ((iprtoh32(p_page_28_cur->attr[i].max_xfer_rate)) *
                     p_page_28_cur->attr[i].bus_width)/(10 * 8);

                if (IPR_LIMITED_MAX_XFER_RATE < max_xfer_rate)
                {
                    p_page_28_cur->attr[i].max_xfer_rate =
                        htoipr32(IPR_LIMITED_MAX_XFER_RATE * 10 /
                                 (p_page_28_cur->attr[i].bus_width / 8));
                }

                if (limited_config & IPR_SAVE_LIMITED_CONFIG)
                {
                    /* update config file to indicate present restriction */
                    sprintf(category,"[%s%d]",
                            IPR_CATEGORY_BUS,
                            p_page_28_cur->attr[i].res_addr.bus);
                    sprintf(value_str,"%d",
                            (iprtoh32(p_page_28_cur->attr[i].max_xfer_rate) *
                             (p_page_28_cur->attr[i].bus_width / 8))/10);

                    ipr_config_file_entry(file_name,
                                          category,
                                          IPR_MAX_XFER_RATE,
                                          value_str,
                                          1);
                }
            }
            else
            {
                sprintf(category,"[%s%d]",
                        IPR_CATEGORY_BUS,
                        p_page_28_cur->attr[i].res_addr.bus);

                rc = ipr_config_file_read(file_name,
                                          category,
                                          IPR_MAX_XFER_RATE,
                                          value_str);

                if (rc == RC_SUCCESS)
                {
                    sscanf(value_str,"%d", &new_value);

                    max_xfer_rate = ((iprtoh32(p_page_28_dflt->attr[i].max_xfer_rate)) *
                                     p_page_28_cur->attr[i].bus_width)/(10 * 8);

                    if (new_value <= max_xfer_rate)
                    {
                        p_page_28_cur->attr[i].max_xfer_rate =
                            htoipr32(new_value * 10 /
                                     (p_page_28_cur->attr[i].bus_width / 8));
                    }
                    else
                    {
                        p_page_28_cur->attr[i].max_xfer_rate =
                            p_page_28_dflt->attr[i].max_xfer_rate;

                        /* update config file to indicate present restriction */
                        sprintf(category,"[%s%d]",
                                IPR_CATEGORY_BUS,
                                p_page_28_cur->attr[i].res_addr.bus);
                        sprintf(value_str,"%d",
                                (iprtoh32(p_page_28_cur->attr[i].max_xfer_rate) *
                                 (p_page_28_cur->attr[i].bus_width / 8))/10);

                        ipr_config_file_entry(file_name,
                                              category,
                                              IPR_MAX_XFER_RATE,
                                              value_str,
                                              1);
                    }
                }
                else
                {
                    p_page_28_cur->attr[i].max_xfer_rate =
                        p_page_28_dflt->attr[i].max_xfer_rate;
                }
            }
        }

        if (p_page_28_chg->attr[i].min_time_delay)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    p_page_28_cur->attr[i].res_addr.bus);

            rc = ipr_config_file_read(file_name,
                                      category,
                                      IPR_MIN_TIME_DELAY,
                                      value_str);
            if (rc == RC_SUCCESS)
            {
                sscanf(value_str,"%d", &new_value);
                p_page_28_cur->attr[i].min_time_delay = new_value;
            }
            else
            {
                p_page_28_cur->attr[i].min_time_delay = IPR_INIT_SPINUP_DELAY;
            }
        }
    }

    /* issue mode sense and reset if necessary */
    rc = ipr_mode_select_page_28(fd, p_page_28_sense);

    if (rc != 0)
    {
        close(fd);
        syslog(LOG_ERR, "Mode Select to %s failed. %m"IPR_EOL,
               p_cur_ioa->ioa.dev_name);
        return;
    }

    if ((is_reset_req) &&
        (!reset_scheduled))
    {
        rc = ioctl(fd, IPR_IOCTL_RESET_IOA);
    }

    close(fd);
}

void ipr_set_page_28_init(struct ipr_ioa *p_cur_ioa,
                          int limited_config)
{
    int rc, i;
    char current_mode_settings[255];
    char changeable_mode_settings[255];
    char default_mode_settings[255];
    struct ipr_mode_parm_hdr *p_mode_parm_hdr;
    int fd;
    int issue_mode_select = 0;
    struct ipr_pagewh_28 *p_page_28_sense;
    struct ipr_page_28 *p_page_28_cur, *p_page_28_chg, *p_page_28_dflt;
    struct ipr_page_28 page_28_ipr;
    u32 max_xfer_rate;

    memset(current_mode_settings,0,sizeof(current_mode_settings));
    memset(changeable_mode_settings,0,sizeof(changeable_mode_settings));
    memset(default_mode_settings,0,sizeof(default_mode_settings));

    fd = open(p_cur_ioa->ioa.dev_name, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
        return;
    }

    /* Mode sense page28 to get current parms */
    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)current_mode_settings;
    rc = ipr_mode_sense_page_28(fd, p_mode_parm_hdr);

    if (rc != 0)
    {
        close(fd);
        syslog(LOG_ERR, "Mode Sense to %s failed. %m"IPR_EOL,
               p_cur_ioa->ioa.dev_name);
        return;
    }

    p_page_28_sense = (struct ipr_pagewh_28 *)p_mode_parm_hdr;
    p_page_28_cur = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* Now issue mode sense to get changeable parms */
    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)changeable_mode_settings;
    p_page_28_chg = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* Now issue mode sense to get default parms */
    p_mode_parm_hdr = (struct ipr_mode_parm_hdr *)default_mode_settings;
    p_page_28_dflt = (struct ipr_page_28 *)
        (((u8 *)(p_mode_parm_hdr+1)) + p_mode_parm_hdr->block_desc_len);

    /* determine changable and default values */
    for (i = 0;
         i < p_page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        p_page_28_chg->attr[i].qas_capability = 0;
        p_page_28_chg->attr[i].scsi_id = 0; //FIXME!!! need to allow dart (by
                                            // vend/prod & subsystem id)
        p_page_28_chg->attr[i].bus_width = 1;
        p_page_28_chg->attr[i].max_xfer_rate = 1;

        p_page_28_dflt->attr[i].qas_capability = 0;
        p_page_28_dflt->attr[i].scsi_id = p_page_28_cur->attr[i].scsi_id;
        p_page_28_dflt->attr[i].bus_width = 16;
        p_page_28_dflt->attr[i].max_xfer_rate = ipr_max_xfer_rate(fd, p_page_28_cur->attr[i].res_addr.bus);
    }

    for (i = 0;
         i < p_page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        if (p_page_28_chg->attr[i].min_time_delay)
            p_page_28_cur->attr[i].min_time_delay = IPR_INIT_SPINUP_DELAY;

        if ((limited_config & (1 << p_page_28_cur->attr[i].res_addr.bus)))
        {
            max_xfer_rate = ((iprtoh32(p_page_28_cur->attr[i].max_xfer_rate)) *
                             p_page_28_cur->attr[i].bus_width)/(10 * 8);

            if (IPR_LIMITED_MAX_XFER_RATE < max_xfer_rate)
            {
                p_page_28_cur->attr[i].max_xfer_rate =
                    htoipr32(IPR_LIMITED_MAX_XFER_RATE * 10 /
                             (p_page_28_cur->attr[i].bus_width / 8));
            }
        }
        else
        {
            p_page_28_cur->attr[i].max_xfer_rate =
                p_page_28_dflt->attr[i].max_xfer_rate;
        }

        issue_mode_select++;
    }

    /* Issue mode select */
    if (issue_mode_select)
    {
        rc = ipr_mode_select_page_28(fd, p_page_28_sense);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Mode Select to %s failed. %m"IPR_EOL,
                   p_cur_ioa->ioa.dev_name);
        }

        /* Zero out user page 28 page, this data is used to indicate
         that no values are being changed, this routine is called
         to create and initialize the file if it does not already
         exist */
        memset(&page_28_ipr, 0, sizeof(struct ipr_pagewh_28));

        ipr_save_page_28(p_cur_ioa, p_page_28_cur, p_page_28_chg,
                         &page_28_ipr);
    }

    close(fd);
}

void iprlog_location(struct ipr_ioa *p_ioa)
{
  syslog(LOG_ERR, 
	 "  PCI Address: %s, Host num: %d",
	 p_ioa->pci_address, p_ioa->host_no); /* FIXME */
}

bool is_af_blocked(struct ipr_device *p_ipr_device, int silent)
{
    int i, j, rc, fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    struct ipr_std_inq_data std_inq_data;
    struct ipr_dasd_inquiry_page3 dasd_page3_inq;
    u32 ros_rcv_ram_rsvd, min_ucode_level;

    fd = open(p_ipr_device->gen_name, O_RDWR);

    if (fd != -1)
    {
        /* Zero out inquiry data */
        memset(&std_inq_data, 0, sizeof(std_inq_data));
        memset(cdb, 0, IPR_CCB_CDB_LEN);

        cdb[0] = INQUIRY;
        cdb[4] = sizeof(std_inq_data);

        rc = sg_ioctl(fd, cdb,
                      &std_inq_data, sizeof(std_inq_data),
                      SG_DXFER_FROM_DEV,
                      &sense_data, 30);

        if (rc != 0)
        {
            syslog(LOG_ERR,"standard inquiry failed to %s.",
                   p_ipr_device->gen_name);
            close(fd);
            return false;
        }

        memset(&dasd_page3_inq, 0, sizeof(dasd_page3_inq));
        memset(cdb, 0, IPR_CCB_CDB_LEN);

        /* Issue page 3 inquiry */
        cdb[0] = INQUIRY;
        cdb[1] = 0x01; /* EVPD */
        cdb[2] = 0x03; /* Page 3 */
        cdb[4] = sizeof(dasd_page3_inq);

        rc = sg_ioctl(fd, cdb, &dasd_page3_inq,
                      sizeof(dasd_page3_inq),
                      SG_DXFER_FROM_DEV, &sense_data, 30);

        close(fd);
        if (rc != 0)
        {
            syslog(LOG_ERR,"Page 3 inquiry failed to %s.", p_ipr_device->dev_name);
            return false;
        }

        /* Is this device in the table? */
        for (i=0;
             i < sizeof(unsupported_af)/
                 sizeof(struct unsupported_af_dasd);
             i++)
        {
            /* check vendor id */
            for (j = 0; j < IPR_VENDOR_ID_LEN; j++)
            {
                if (unsupported_af[i].compare_vendor_id_byte[j])
                {
                    if (unsupported_af[i].vendor_id[j] !=
                        std_inq_data.vpids.vendor_id[j])
                    {
                        /* not a match */
                        break;
                    }
                }
            }

            if (j != IPR_VENDOR_ID_LEN)
                continue;

            /* check product ID */
            for (j = 0; j < IPR_PROD_ID_LEN; j++)
            {
                if (unsupported_af[i].compare_product_id_byte[j])
                {
                    if (unsupported_af[i].product_id[j] !=
                        std_inq_data.vpids.product_id[j])
                    {
                        /* not a match */
                        break;
                    }
                }
            }

            if (j != IPR_PROD_ID_LEN)
                continue;

            /* Compare LID Level */
            for (j = 0; j < 4; j++)
            {
                if (unsupported_af[i].lid_mask[j])
                {
                    if (unsupported_af[i].lid[j] !=
                        dasd_page3_inq.load_id[j])
                    {
                        /* not a match */
                        break;
                    }
                }
            }

            if (j != 4)
                continue;

            /* If we make it this far, we have a match into the table.  Now,
             determine if we need a certain level of microcode or if this
             disk is not supported all together. */
            if (unsupported_af[i].supported_with_min_ucode_level)
            {
                /* Check microcode level in std inquiry data */
                /* If it's less than the minimum required level, we will
                 tell the user to upgrade microcode. */
                min_ucode_level = 0;
                ros_rcv_ram_rsvd = 0;

                for (j = 0; j < 4; j++)
                {
                    if (unsupported_af[i].min_ucode_mask[j])
                    {
                        min_ucode_level = (min_ucode_level << 8) |
                            unsupported_af[i].min_ucode_level[j];
                        ros_rcv_ram_rsvd = (ros_rcv_ram_rsvd << 8) |
                            std_inq_data.ros_rsvd_ram_rsvd[j];
                    }
                }

                if (min_ucode_level > ros_rcv_ram_rsvd)
                {
                    if (!silent)
                        syslog(LOG_ERR,"Disk %s needs updated microcode "
                               "before transitioning to 522 bytes/sector "
                               "format.", p_ipr_device->dev_name);
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else /* disk is not supported at all */
            {
                if (!silent)
                    syslog(LOG_ERR,"Disk %s not supported for transition "
                           "to 522 bytes/sector format.", p_ipr_device->dev_name);
                return true;
            }
        }   
    }
    return false;
}
