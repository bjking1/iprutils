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
 * $Header: /cvsroot/iprdd/iprutils/iprlib.c,v 1.9 2004/01/30 23:41:21 manderso Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

struct ipr_array_query_data *ipr_qac_data = NULL;
struct scsi_dev_data *scsi_dev_table = NULL;
u32 num_ioas = 0;
struct ipr_ioa *ipr_ioa_head = NULL;
struct ipr_ioa *ipr_ioa_tail = NULL;

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
    int ccin;
    struct ipr_ioa *ipr_ioa;
    struct sysfs_driver *sysfs_ipr_driver;
    struct dlist *ipr_devs;
    char *pci_address;

    struct sysfs_class *sysfs_host_class;
    struct sysfs_class *sysfs_device_class;
    struct dlist *class_devices;
    struct sysfs_class_device *class_device;
    struct sysfs_device *sysfs_device_device;
    struct sysfs_device *sysfs_host_device;
    struct sysfs_device *sysfs_pci_device;

    struct sysfs_attribute *sysfs_model_attr;

    /* Find all the IPR IOAs attached and save away vital info about them */
    sysfs_ipr_driver = sysfs_open_driver("ipr", "pci");
    if (sysfs_ipr_driver == NULL) {
        syslog(LOG_ERR,"Unable to located ipr driver data: %m\n");
        return;
    }

    ipr_devs = sysfs_get_driver_devices(sysfs_ipr_driver);

    if (ipr_devs != NULL) {
        dlist_for_each_data(ipr_devs, pci_address, char) {

            ipr_ioa = (struct ipr_ioa*)malloc(sizeof(struct ipr_ioa));
            memset(ipr_ioa,0,sizeof(struct ipr_ioa));

            /* PCI address */
            strcpy(ipr_ioa->pci_address, pci_address);

            /* driver version */
            strcpy(ipr_ioa->driver_version,"2");  //FIXME pick up version from modinfo?

            sysfs_host_class = sysfs_open_class("scsi_host");
            class_devices = sysfs_get_class_devices(sysfs_host_class);
            dlist_for_each_data(class_devices, class_device, struct sysfs_class_device) {
                sysfs_host_device = sysfs_get_classdev_device(class_device);
                sysfs_pci_device = sysfs_get_device_parent(sysfs_host_device);

                if (strcmp(pci_address, sysfs_pci_device->name) == 0) {
                    strcpy(ipr_ioa->host_name, sysfs_host_device->name);
                    sscanf(ipr_ioa->host_name, "host%d", &ipr_ioa->host_num);
                    break;
                }
            }
            sysfs_close_class(sysfs_host_class);

            sysfs_device_class = sysfs_open_class("scsi_device");
            class_devices = sysfs_get_class_devices(sysfs_device_class);
            dlist_for_each_data(class_devices, class_device, struct sysfs_class_device) {
                sysfs_device_device = sysfs_get_classdev_device(class_device);
                sysfs_host_device = sysfs_get_device_parent(sysfs_device_device);

                if ((strcmp(ipr_ioa->host_name, sysfs_host_device->name) == 0) && 
                    (strstr(sysfs_device_device->name, ":255:255:255"))) {

                    sysfs_model_attr = sysfs_get_device_attr(sysfs_device_device, "model");
                    sscanf(sysfs_model_attr->value, "%4X", &ccin);
                    ipr_ioa->ccin = ccin;
                    break;
                }
            }
            sysfs_close_class(sysfs_device_class);

            ipr_ioa->next = NULL;
            ipr_ioa->num_raid_cmds = 0;

            if (ipr_ioa_tail)
            {
                ipr_ioa_tail->next = ipr_ioa;
                ipr_ioa_tail = ipr_ioa;
            }
            else
            {
                ipr_ioa_tail = ipr_ioa_head = ipr_ioa;
            }

            num_ioas++;
        }
    }

    sysfs_close_driver(sysfs_ipr_driver);

    return;
}

/* Try to dynamically add this device */
int scan_device(struct scsi_dev_data *scsi_dev_data)
{
    char cmd[200];

    if (!scsi_dev_data)
        return -1;

    sprintf(cmd, "echo \"scsi add-single-device %d %d %d %d\" > /proc/scsi/scsi",
            scsi_dev_data->host,
            scsi_dev_data->channel,
            scsi_dev_data->id,
            scsi_dev_data->lun);
    system(cmd);
    return 0;
}

/* Try to dynamically remove this device */
int remove_device(struct scsi_dev_data *scsi_dev_data)
{
    char cmd[200];

    if (!scsi_dev_data)
        return -1;

    sprintf(cmd, "echo \"scsi remove-single-device %d %d %d %d\" > /proc/scsi/scsi",
            scsi_dev_data->host,
            scsi_dev_data->channel,
            scsi_dev_data->id,
            scsi_dev_data->lun);
    system(cmd);
    return 0;
}

void ipr_reset_adapter(struct ipr_ioa *ioa)
{
    struct sysfs_class_device *class_device;
    struct sysfs_attribute *attr;

    class_device = sysfs_open_class_device("scsi_host",
                                           ioa->host_name);
    attr = sysfs_get_classdev_attr(class_device,
                                    "reset_host");
    sysfs_write_attribute(attr, "1", 1);
    sysfs_close_class_device(class_device);
}

int ipr_query_array_config(char *file, bool allow_rebuild_refresh,
                           bool set_array_id, int array_id, void *buff)
{
    int fd;
    int length = sizeof(struct ipr_array_query_data);
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = IPR_QUERY_ARRAY_CONFIG;
    
    if (set_array_id) {
        cdb[1] = 0x01;
        cdb[2] = array_id;
    }
    else if (allow_rebuild_refresh)
        cdb[1] = 0;
    else
        cdb[1] = 0x80; /* Prohibit Rebuild Candidate Refresh */

    cdb[7] = (length & 0xff00) >> 8;
    cdb[8] = length & 0xff;

    rc = sg_ioctl(fd, cdb, buff,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_ARRAY_CMD_TIMEOUT);

    if (rc != 0)
    {
        if (errno != EINVAL)
            syslog(LOG_ERR,"Query Array Config to %s failed. %m\n", file);
    }

    close(fd);
    return rc;
}

int ipr_start_array(char *file, struct ipr_array_query_data *qac_data,
                    int stripe_size, int prot_level, int hot_spare)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = iprtoh16(qac_data->resp_len);

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = IPR_START_ARRAY_PROTECTION;
    if (hot_spare)
        cdb[1] = 0x01;

    cdb[4] = (u8)(stripe_size >> 8);
    cdb[5] = (u8)(stripe_size);
    cdb[6] = prot_level;
    cdb[7] = (length & 0xff00) >> 8;
    cdb[8] = length & 0xff;

    rc = sg_ioctl(fd, cdb, qac_data,
                  length, SG_DXFER_TO_DEV,
                  &sense_data, IPR_ARRAY_CMD_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Start Array Protection to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_start_array_protection(char *file, struct ipr_array_query_data *qac_data,
                               int stripe_size, int prot_level)
{
    return ipr_start_array(file, qac_data, stripe_size, prot_level, 0);
}

int ipr_add_hot_spare(char *file, struct ipr_array_query_data *qac_data)
{
    return ipr_start_array(file, qac_data, 0, 0, 1);
}

int ipr_stop_array(char *file, struct ipr_array_query_data *qac_data, int hot_spare)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = iprtoh16(qac_data->resp_len);

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = IPR_STOP_ARRAY_PROTECTION;
    if (hot_spare)
        cdb[1] = 0x01;

    cdb[7] = (length & 0xff00) >> 8;
    cdb[8] = length & 0xff;

    rc = sg_ioctl(fd, cdb, qac_data,
                  length, SG_DXFER_TO_DEV,
                  &sense_data, IPR_ARRAY_CMD_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Stop Array Protection to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_stop_array_protection(char *file, struct ipr_array_query_data *qac_data)
{
    return ipr_stop_array(file, qac_data, 0);
}

int ipr_remove_hot_spare(char *file, struct ipr_array_query_data *qac_data)
{
    return ipr_stop_array(file, qac_data, 1);
}

int ipr_rebuild_device_data(char *file, struct ipr_array_query_data *qac_data)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = iprtoh16(qac_data->resp_len);

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = IPR_REBUILD_DEVICE_DATA;
    cdb[7] = (length & 0xff00) >> 8;
    cdb[8] = length & 0xff;

    rc = sg_ioctl(fd, cdb, qac_data,
                  length, SG_DXFER_TO_DEV,
                  &sense_data, IPR_ARRAY_CMD_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Rebuild Device Data to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_add_array_device(char *file, struct ipr_array_query_data *qac_data)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = iprtoh16(qac_data->resp_len);

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = IPR_ADD_ARRAY_DEVICE;
    cdb[7] = (length & 0xff00) >> 8;
    cdb[8] = length & 0xff;

    rc = sg_ioctl(fd, cdb, qac_data,
                  length, SG_DXFER_TO_DEV,
                  &sense_data, IPR_ARRAY_CMD_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Add Array Device to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_query_command_status(char *file, void *buff)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = sizeof(struct ipr_cmd_status);

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = IPR_QUERY_COMMAND_STATUS;
    cdb[7] = (length & 0xff00) >> 8;
    cdb[8] = length & 0xff;

    rc = sg_ioctl(fd, cdb, buff,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_ARRAY_CMD_TIMEOUT);


    if ((rc != 0) && (errno != EINVAL))
        syslog(LOG_ERR, "Query Command Status to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_query_resource_state(char *file, void *buff)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = sizeof(struct ipr_query_res_state);

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = IPR_QUERY_RESOURCE_STATE;
    cdb[7] = (length & 0xff00) >> 8;
    cdb[8] = length & 0xff;

    rc = sg_ioctl(fd, cdb, buff,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_ARRAY_CMD_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Query Resource State to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_mode_sense(char *file, u8 page, void *buff)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc, loop;
    u8 length = IPR_MODE_SENSE_LENGTH;
    char *hex;
    char buffer[128];

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = MODE_SENSE;
    cdb[2] = page;
    cdb[4] = length;

    rc = sg_ioctl(fd, cdb, buff,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

    if (rc != 0) {
        syslog(LOG_ERR, "Mode Sense to %s failed. %m\n", file);

        if (rc == CHECK_CONDITION) {
            hex = (u8 *)&sense_data;

            length = 0;
            for (loop = 0; loop < 18; loop++)
                length += sprintf(buffer + length,"%.2x ", hex[loop + 8]);

            syslog(LOG_ERR, "sense data = %s\n", buffer);
        }
    }

    close(fd);
    return rc;
}

int ipr_mode_select(char *file, void *buff, int length)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = MODE_SELECT;
    cdb[1] = 0x10; /* PF = 1, SP = 0 */
    cdb[4] = length;

    rc = sg_ioctl(fd, cdb, buff,
                  length, SG_DXFER_TO_DEV,
                  &sense_data, IPR_INTERNAL_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Mode Select to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_reset_device(char *file)
{
    int fd;
    int rc;
    int arg;

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    arg = SG_SCSI_RESET_DEVICE;
    rc = ioctl(fd, SG_SCSI_RESET, &arg);

    if (rc != 0)
        syslog(LOG_ERR, "Reset Device to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_re_read_partition(char *file)
{
    int fd;
    int rc;

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    rc = ioctl(fd, BLKRRPART, NULL);

    if (rc != 0)
        syslog(LOG_ERR, "Re-read partition table to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_read_capacity(char *file, void *buff)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = sizeof(struct ipr_read_cap);

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = READ_CAPACITY;

    rc = sg_ioctl(fd, cdb, buff,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Read Capacity to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_read_capacity_16(char *file, void *buff)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = sizeof(struct ipr_read_cap16);

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = IPR_SERVICE_ACTION_IN;
    cdb[1] = IPR_READ_CAPACITY_16;

    rc = sg_ioctl(fd, cdb, buff,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Read Capacity to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_reclaim_cache_store(struct ipr_ioa *ioa, int action, void *buff)
{
    char *file = ioa->ioa.gen_name;
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = sizeof(struct ipr_reclaim_query_data);

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = IPR_RECLAIM_CACHE_STORE;
    cdb[1] = (u8)action;

    rc = sg_ioctl(fd, cdb, buff,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

    if (rc != 0) {
        if (errno != EINVAL) {
            syslog(LOG_ERR, "Reclaim Cache Store to %s failed. %m\n", file);
        }
    }
    else if ((action & IPR_RECLAIM_PERFORM) == IPR_RECLAIM_PERFORM)
        ipr_reset_adapter(ioa);

    close(fd);
    return rc;
}

int ipr_start_stop_start(char *file)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = 0;

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = START_STOP;
    cdb[4] = IPR_START_STOP_START;

    rc = sg_ioctl(fd, cdb, NULL,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Start Stop Start to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_start_stop_stop(char *file)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    int length = 0;

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = START_STOP;
    cdb[4] = IPR_START_STOP_STOP;

    rc = sg_ioctl(fd, cdb, NULL,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Start Stop Stop to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_test_unit_ready(char *file,
                        struct sense_data_t *sense_data)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    int rc;
    int length = 0;

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = TEST_UNIT_READY;

    rc = sg_ioctl(fd, cdb, NULL,
                  length, SG_DXFER_FROM_DEV,
                  sense_data, IPR_INTERNAL_DEV_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Test Unit Ready to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_format_unit(char *file)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc, loop;
    u8 *defect_list_hdr;
    int length = IPR_DEFECT_LIST_HDR_LEN;
    char *hex;
    char buffer[128];

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    defect_list_hdr = calloc(1, IPR_DEFECT_LIST_HDR_LEN);

    cdb[0] = FORMAT_UNIT;
    cdb[1] = IPR_FORMAT_DATA; /* lun = 0, fmtdata = 1, cmplst = 0, defect list format = 0 */
    cdb[4] = 1;

    defect_list_hdr[1] = IPR_FORMAT_IMMED; /* FOV = 0, DPRY = 0, DCRT = 0, STPF = 0, IP = 0, DSP = 0, Immed = 1, VS = 0 */

    rc = sg_ioctl(fd, cdb, defect_list_hdr,
                  length, SG_DXFER_TO_DEV,
                  &sense_data, IPR_FORMAT_UNIT_TIMEOUT);

    free(defect_list_hdr);

    if (rc != 0) {
        syslog(LOG_ERR, "Format Unit to %s failed. %m\n", file);
        hex = (u8 *)&sense_data;

        length = 0;
        for (loop = 0; loop < 18; loop++)
            length += sprintf(buffer + length,"%.2x ", hex[loop + 8]);

        syslog(LOG_ERR, "sense data = %s\n", buffer);
    }

    close(fd);
    return rc;
}

int ipr_evaluate_device(char *file, u32 res_handle)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;
    u32 resource_handle;
    int length = 0;

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    resource_handle = iprtoh32(res_handle);

    cdb[0] = IPR_EVALUATE_DEVICE;
    cdb[2] = (u8)(resource_handle >> 24);
    cdb[3] = (u8)(resource_handle >> 16);
    cdb[4] = (u8)(resource_handle >> 8);
    cdb[5] = (u8)(resource_handle);

    rc = sg_ioctl(fd, cdb, NULL,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_EVALUATE_DEVICE_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Evaluate Device to %s failed. %m\n", file);

    close(fd);
    return rc;
}

int ipr_inquiry(char *file, u8 page, void *buff, u8 length)
{
    int fd;
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int rc;

    fd = open(file, O_RDWR);
    if (fd <= 1)
    {
        syslog(LOG_ERR, "Could not open %s. %m\n",file);
        return errno;
    }

    memset(cdb, 0, IPR_CCB_CDB_LEN);

    cdb[0] = INQUIRY;
    if (page != IPR_STD_INQUIRY)
    {
        cdb[1] = 0x01;
        cdb[2] = page;
    }
    cdb[4] = length;

    rc = sg_ioctl(fd, cdb, buff,
                  length, SG_DXFER_FROM_DEV,
                  &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

    if (rc != 0)
        syslog(LOG_ERR, "Evaluate Device to %s failed. %m\n", file);

    close(fd);
    return rc;
}

#define IPR_MAX_XFER 0x8000
const int cdb_size[] ={6, 10, 10, 0, 16, 12, 16, 16};
int sg_ioctl(int fd, u8 cdb[IPR_CCB_CDB_LEN],
             void *data, u32 xfer_len, u32 data_direction,
             struct sense_data_t *sense_data,
             u32 timeout_in_sec)
{
    int rc;
    sg_io_hdr_t io_hdr_t;
    sg_iovec_t *iovec;
    int iovec_count;
    int i;
    int buff_len, segment_size;
    void *dxferp;

    /* check if scatter gather should be used */
    if (xfer_len > IPR_MAX_XFER)
    {
        iovec_count = xfer_len/IPR_MAX_XFER + 1;
        iovec = malloc(iovec_count * sizeof(sg_iovec_t));

        buff_len = xfer_len;
        segment_size = IPR_MAX_XFER;

        for (i = 0; (i < iovec_count) && (buff_len != 0); i++)
        {
            iovec[i].iov_base = malloc(IPR_MAX_XFER);
            memcpy(iovec[i].iov_base, data + IPR_MAX_XFER * i, IPR_MAX_XFER);
            iovec[i].iov_len = segment_size;

            buff_len -= segment_size;
            if (buff_len < segment_size)
                segment_size = buff_len;
        }
        iovec_count = i;
        dxferp = (void *)iovec;
    }
    else
    {
        iovec_count = 0;
        dxferp = data;
    }

    for (i = 0; i < 2; i++)
    {
        memset(&io_hdr_t, 0, sizeof(io_hdr_t));
        memset(sense_data, 0, sizeof(struct sense_data_t));

        io_hdr_t.interface_id = 'S';
        io_hdr_t.cmd_len = cdb_size[(cdb[0] >> 5) & 0x7];
        io_hdr_t.iovec_count = iovec_count;
        io_hdr_t.flags = 0;
        io_hdr_t.pack_id = 0;
        io_hdr_t.usr_ptr = 0;
        io_hdr_t.sbp = (unsigned char *)sense_data;
        io_hdr_t.mx_sb_len = sizeof(struct sense_data_t);
        io_hdr_t.timeout = timeout_in_sec * 1000;
        io_hdr_t.cmdp = cdb;
        io_hdr_t.dxfer_direction = data_direction;
        io_hdr_t.dxfer_len = xfer_len;
        io_hdr_t.dxferp = dxferp;

        rc = ioctl(fd, SG_IO, &io_hdr_t);

        if ((rc == 0) && (io_hdr_t.masked_status == CHECK_CONDITION))
            rc = CHECK_CONDITION;

        if ((rc != CHECK_CONDITION) || (sense_data->sense_key != UNIT_ATTENTION))
            break;
    }

    if (iovec_count)
    {
        for (i = 0; i < iovec_count; i++)
            free(iovec[i].iov_base);
        free(iovec);
    }

    return rc;
};

#define MAX_SG_DEVS 255  //FIXME!!!
#define MAX_SD_DEVS 255
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

void get_sg_ioctl_data(struct sg_map_info *sg_map_info, int num_devs)
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
        temp = &sg_map_info[num_sg_devices];

        memset(sg_map_info[num_sg_devices].gen_name, 0, sizeof(sg_map_info[num_sg_devices].gen_name));
        make_dev_name(sg_map_info[num_sg_devices].gen_name, "/dev/sg", sg_index, 1);
        fd = open(sg_map_info[num_sg_devices].gen_name, O_RDONLY | O_NONBLOCK);

        if (fd < 0)
        {
            if (errno == ENOENT)
            {
                mknod(sg_map_info[num_sg_devices].gen_name,
                      S_IFCHR|0660,
                      makedev(21,sg_index));

                /* change permissions and owner to be identical to
                 /dev/sg0's */
                stat("/dev/sg0", &sda_stat);
                chown(sg_map_info[num_sg_devices].gen_name,
                      sda_stat.st_uid,
                      sda_stat.st_gid);
                chmod(sg_map_info[num_sg_devices].gen_name,
                      sda_stat.st_mode);

                fd = open(sg_map_info[num_sg_devices].gen_name, O_RDONLY | O_NONBLOCK);
                if (fd < 0)
                    continue;
            }
            else
                continue;
        }

        rc = ioctl(fd, SG_GET_SCSI_ID, &sg_map_info[num_sg_devices].sg_dat);

        close(fd);
        if ((rc >= 0) &&
            (sg_map_info[num_sg_devices].sg_dat.scsi_type == IPR_TYPE_DISK))
        {
            temp_max_sd_devs = 0;

            memset(sg_map_info[num_sg_devices].dev_name, 0, sizeof(sg_map_info[num_sg_devices].dev_name));
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

                if ((sg_map_info[num_sg_devices].sg_dat.host_no == ((dev_idlun.dev_id >> 24) & 0xFF)) &&
                    (sg_map_info[num_sg_devices].sg_dat.channel == ((dev_idlun.dev_id >> 16) & 0xFF)) &&
                    (sg_map_info[num_sg_devices].sg_dat.scsi_id == (dev_idlun.dev_id & 0xFF)) &&
                    (sg_map_info[num_sg_devices].sg_dat.lun     == ((dev_idlun.dev_id >>  8) & 0xFF)))
                {
                    strcpy(sg_map_info[num_sg_devices].dev_name, tmp_dev_name);
                    temp_max_sd_devs = max_sd_devs;
                    used_sd[sd_index] = 1;
                    break;
                }
            }
            max_sd_devs = temp_max_sd_devs;
            num_sg_devices++;
        } else if (rc >= 0) {
            strcpy(sg_map_info[num_sg_devices].dev_name, "");
            num_sg_devices++;
        }
    }
    return;
}

int get_scsi_dev_data(struct scsi_dev_data **scsi_dev_ref)
{
    int num_devs = 0;
    struct scsi_dev_data *scsi_dev_data;
    struct scsi_dev_data *scsi_dev_base = *scsi_dev_ref;
    struct sysfs_class *sysfs_device_class;
    struct dlist *class_devices;
    struct sysfs_class_device *class_device;
    struct sysfs_device *sysfs_device_device;
    struct sysfs_attribute *sysfs_attr;


    sysfs_device_class = sysfs_open_class("scsi_device");
    class_devices = sysfs_get_class_devices(sysfs_device_class);
    dlist_for_each_data(class_devices, class_device, struct sysfs_class_device) {

        scsi_dev_base = realloc(scsi_dev_base,
                                sizeof(struct scsi_dev_data) * (num_devs + 1));
        scsi_dev_data = &scsi_dev_base[num_devs];

        sysfs_device_device = sysfs_get_classdev_device(class_device);
        if (!sysfs_device_device) {
            *scsi_dev_ref = scsi_dev_base;
            return -1;
        }

        sscanf(sysfs_device_device->name,"%d:%d:%d:%d",
               &scsi_dev_data->host,
               &scsi_dev_data->channel,
               &scsi_dev_data->id,
               &scsi_dev_data->lun);

        sprintf(scsi_dev_data->sysfs_device_name,
                sysfs_device_device->name);

        sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "type");
        if (sysfs_attr)
            sscanf(sysfs_attr->value, "%d", &scsi_dev_data->type);

//FIXME WHERE TO GET OPENS!!!
/*
        sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "opens");
        sscanf(sysfs_attr->value "%d", &scsi_dev_data->opens);
*/      scsi_dev_data->opens = 0;

        sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "online");
        if (sysfs_attr)
            sscanf(sysfs_attr->value, "%d", &scsi_dev_data->online);

        sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "queue_depth");
        if (sysfs_attr)
            sscanf(sysfs_attr->value, "%d", &scsi_dev_data->qdepth);

        sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "adapter_handle");
        if (sysfs_attr)
            sscanf(sysfs_attr->value, "%d", &scsi_dev_data->handle);

        sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "vendor");
        if (sysfs_attr)
            ipr_strncpy_0(scsi_dev_data->vendor_id, sysfs_attr->value, IPR_VENDOR_ID_LEN);

        sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "model");
        if (sysfs_attr)
            ipr_strncpy_0(scsi_dev_data->product_id, sysfs_attr->value, IPR_PROD_ID_LEN);

        num_devs++;
    }
    sysfs_close_class(sysfs_device_class);

    *scsi_dev_ref = scsi_dev_base;
    return num_devs;
}

void get_ioa_name(struct ipr_ioa *cur_ioa,
                  struct sg_map_info *sg_map_info,
                  int num_sg_devices)
{
    int i;
    int host_no;

    sscanf(cur_ioa->host_name, "host%d", &host_no);

    for (i = 0; i < num_sg_devices; i++, sg_map_info++) {
        if ((sg_map_info->sg_dat.host_no == host_no)   &&
            (sg_map_info->sg_dat.channel == 255) &&
            (sg_map_info->sg_dat.scsi_id == 255) &&
            (sg_map_info->sg_dat.lun     == 255) &&
            (sg_map_info->sg_dat.scsi_type == IPR_TYPE_ADAPTER)) {

            strcpy(cur_ioa->ioa.dev_name, sg_map_info->dev_name);
            strcpy(cur_ioa->ioa.gen_name, sg_map_info->gen_name);
        }
    }
}

void check_current_config(bool allow_rebuild_refresh)
{
    struct sg_map_info *sg_map_info;
    struct scsi_dev_data *scsi_dev_data;
    int  num_sg_devices, rc, device_count, j, k;
    struct ipr_ioa *cur_ioa;
    struct ipr_array_query_data  *cur_qac_data;
    struct ipr_common_record *common_record;
    struct ipr_dev_record *device_record;
    struct ipr_array_record *array_record;
    int *qac_entry_ref;

    if (ipr_qac_data == NULL)
    {
        ipr_qac_data =
            (struct ipr_array_query_data *)
            malloc(sizeof(struct ipr_array_query_data) * num_ioas);
    }

    /* Get sg data via sg proc file system */
    num_sg_devices = get_scsi_dev_data(&scsi_dev_table);

    sg_map_info = (struct sg_map_info *)malloc(sizeof(struct sg_map_info) * num_sg_devices);
    get_sg_ioctl_data(sg_map_info, num_sg_devices);

    for(cur_ioa = ipr_ioa_head,
        cur_qac_data = ipr_qac_data;
        cur_ioa != NULL;
        cur_ioa = cur_ioa->next,
        cur_qac_data++)
    {
        get_ioa_name(cur_ioa, sg_map_info, num_sg_devices);

        cur_ioa->num_devices = 0;

        /* Get Query Array Config Data */
        rc = ipr_query_array_config(cur_ioa->ioa.gen_name,
                                    allow_rebuild_refresh,
                                    0, 0, cur_qac_data);

        if (rc != 0)
            cur_qac_data->num_records = 0;

        cur_ioa->qac_data = cur_qac_data;

        device_count = 0;
        memset(cur_ioa->dev, 0, IPR_MAX_IOA_DEVICES * sizeof(struct ipr_dev));
        qac_entry_ref = calloc(1, sizeof(int) * cur_qac_data->num_records);

        /* now assemble data pertaining to each individual device */
        for (j = 0, scsi_dev_data = scsi_dev_table;
             j < num_sg_devices; j++, scsi_dev_data++)
        {
            if (scsi_dev_data->host != cur_ioa->host_num)
                continue;

            if ((scsi_dev_data->type == IPR_TYPE_DISK) ||
                (scsi_dev_data->type == IPR_TYPE_AF_DISK))
            {
                cur_ioa->dev[device_count].ioa = cur_ioa;
                cur_ioa->dev[device_count].scsi_dev_data = scsi_dev_data;
                cur_ioa->dev[device_count].qac_entry = NULL;
                strcpy(cur_ioa->dev[device_count].dev_name, "");
                strcpy(cur_ioa->dev[device_count].gen_name, "");

                /* find array config data matching resource entry */
                common_record = (struct ipr_common_record *)cur_qac_data->data;

                for (k = 0; k < cur_qac_data->num_records; k++)
                {
                    if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) {
                        device_record = (struct ipr_dev_record *)common_record;
                        if ((device_record->resource_addr.bus    == scsi_dev_data->channel) &&
                            (device_record->resource_addr.target == scsi_dev_data->id) &&
                            (device_record->resource_addr.lun    == scsi_dev_data->lun))
                        {
                            cur_ioa->dev[device_count].qac_entry = common_record;
                            qac_entry_ref[k]++;
                            break;
                        }
                    } else if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) {
                        array_record = (struct ipr_array_record *)common_record;
                        if ((array_record->resource_addr.bus    == scsi_dev_data->channel) &&
                            (array_record->resource_addr.target == scsi_dev_data->id) &&
                            (array_record->resource_addr.lun    == scsi_dev_data->lun))
                        {
                            cur_ioa->dev[device_count].qac_entry = common_record;
                            qac_entry_ref[k]++;
                            break;
                        }
                    }                    

                    common_record = (struct ipr_common_record *)
                        ((unsigned long)common_record + iprtoh16(common_record->record_len));
                }

                /* find sg_map_info matching resource entry */
                for (k = 0; k < num_sg_devices; k++)
                {
                    if ((scsi_dev_data->host    == sg_map_info[k].sg_dat.host_no) &&
                        (scsi_dev_data->channel == sg_map_info[k].sg_dat.channel) &&
                        (scsi_dev_data->id      == sg_map_info[k].sg_dat.scsi_id) &&
                        (scsi_dev_data->lun     == sg_map_info[k].sg_dat.lun))
                    {
                        /* copy device name to main structure */
                        strcpy(cur_ioa->dev[device_count].dev_name, sg_map_info[k].dev_name);
                        strcpy(cur_ioa->dev[device_count].gen_name, sg_map_info[k].gen_name);

                        break;
                    }
                }

                device_count++;
            }
            else if (scsi_dev_data->type == IPR_TYPE_ADAPTER)
            {
                cur_ioa->ioa.ioa = cur_ioa;
                cur_ioa->ioa.scsi_dev_data = scsi_dev_data;
            }
        }

        /* now scan qac device and array entries to see which ones have
         not been referenced */
        common_record = (struct ipr_common_record *)cur_qac_data->data;

        for (k = 0; k < cur_qac_data->num_records; k++) {
            if (qac_entry_ref[k] > 1)
                syslog(LOG_ERR,
                       "Query Array Config entry referenced more than once\n");

            if (common_record->record_id == IPR_RECORD_ID_SUPPORTED_ARRAYS) {
                cur_ioa->supported_arrays = (struct ipr_supported_arrays *)common_record;
                continue;
            }

            if (qac_entry_ref[k])
                continue;

            if ((common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) ||
                (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD)) {

                /* add phantom qac entry to ioa device list */
                cur_ioa->dev[device_count].scsi_dev_data = NULL;
                cur_ioa->dev[device_count].qac_entry = common_record;

                strcpy(cur_ioa->dev[device_count].dev_name, "");
                strcpy(cur_ioa->dev[device_count].gen_name, "");
                device_count++;
            }

            common_record = (struct ipr_common_record *)
                ((unsigned long)common_record + iprtoh16(common_record->record_len));
        }

        cur_ioa->num_devices = device_count;
    }
    free(sg_map_info);
}

int num_device_opens(int host_num, int channel, int id, int lun)
{
    struct scsi_dev_data *scsi_dev_base;
    int opens = 0;
    int k, num_sg_devices;

    /* Get sg data via sg proc file system */
    num_sg_devices = get_scsi_dev_data(&scsi_dev_base);

    /* find usage counts in scsi_dev_data */
    for (k=0; k < num_sg_devices; k++)
    {
        if ((host_num == scsi_dev_base[k].host) &&
            (channel  == scsi_dev_base[k].channel) &&
            (id       == scsi_dev_base[k].id) &&
            (lun      == scsi_dev_base[k].lun))
        {
            opens = scsi_dev_base[k].opens;
            break;
        }
    }

    free(scsi_dev_base);
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
	syslog(LOG_ERR, "Could not open %s. %m\n", file_name);
	return;
    }

    fprintf(fd,"# DO NOT EDIT! Software generated configuration file for\n");
    fprintf(fd,"# ipr SCSI device subsystem\n\n");
    fprintf(fd,"# Use iprconfig to configure\n");
    fprintf(fd,"# See \"man iprconfig\" for more information\n\n");
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
        syslog(LOG_ERR, "Could not open %s. %m\n", temp_file_name);
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

                        fprintf(temp_fd,"%s %s\n",field,value);
                    }
                }

                if (strstr(line, field))
                {
                    if (update)
                        sprintf(line,"%s %s\n",field,value);

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
            fprintf(temp_fd,"\n%s\n", category);
        }

        if (!update)
            fprintf(temp_fd, "# ");

        fprintf(temp_fd,"%s %s\n", field, value);
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
                        sprintf(value,"%s\n",str_ptr);
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

void ipr_save_page_28(struct ipr_ioa *ioa,
                      struct ipr_page_28 *page_28_cur,
                      struct ipr_page_28 *page_28_chg,
                      struct ipr_page_28 *page_28_ipr)
{
    char file_name[64];
    char category[16];
    char value_str[16];
    void ipr_config_file_hdr(char *file_name);
    int i;
    int update_entry;

    /* gets Host Address to create file_name */
    sprintf(file_name,"%x_%s",
            ioa->ccin, ioa->pci_address);

    for (i = 0;
         i < page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        if (page_28_chg->attr[i].qas_capability)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    page_28_cur->attr[i].qas_capability);

            if (page_28_ipr->attr[i].qas_capability)
                update_entry = 1;
            else
                update_entry = 0;

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_QAS_CAPABILITY,
                                  value_str,
                                  update_entry);
        }

        if (page_28_chg->attr[i].scsi_id)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    page_28_cur->attr[i].scsi_id);

            if (page_28_ipr->attr[i].scsi_id)
                update_entry = 1;
            else
                update_entry = 0;

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_HOST_SCSI_ID,
                                  value_str,
                                  update_entry);
        }

        if (page_28_chg->attr[i].bus_width)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    page_28_cur->attr[i].bus_width);

            if (page_28_ipr->attr[i].bus_width)
                update_entry = 1;
            else
                update_entry = 0;

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_BUS_WIDTH,
                                  value_str,
                                  update_entry);
        }

        if (page_28_chg->attr[i].max_xfer_rate)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    (iprtoh32(page_28_cur->attr[i].max_xfer_rate) *
                     (page_28_cur->attr[i].bus_width / 8))/10);

            if (page_28_ipr->attr[i].max_xfer_rate)
                update_entry = 1;
            else
                update_entry = 0;

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_MAX_XFER_RATE,
                                  value_str,
                                  update_entry);
        }

        if (page_28_chg->attr[i].min_time_delay)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    page_28_cur->attr[i].res_addr.bus);
            sprintf(value_str,"%d",
                    page_28_cur->attr[i].min_time_delay);

            ipr_config_file_entry(file_name,
                                  category,
                                  IPR_MIN_TIME_DELAY,
                                  value_str,
                                  0);
        }
    }
}

void ipr_set_page_28(struct ipr_ioa *cur_ioa,
                     int limited_config,
                     int reset_scheduled)
{
    int rc, i;
    char current_mode_settings[255];
    char changeable_mode_settings[255];
    char default_mode_settings[255];
    struct ipr_mode_parm_hdr *mode_parm_hdr;
    struct ipr_pagewh_28 *page_28_sense;
    struct ipr_page_28 *page_28_cur, *page_28_chg, *page_28_dflt;
    int is_reset_req = 0;
    char value_str[64];
    char file_name[64];
    char category[16];
    int new_value;
    u32 max_xfer_rate;
    int length;

    memset(current_mode_settings,0,sizeof(current_mode_settings));
    memset(changeable_mode_settings,0,sizeof(changeable_mode_settings));
    memset(default_mode_settings,0,sizeof(default_mode_settings));

    /* mode sense page28 to get current parms */
    mode_parm_hdr = (struct ipr_mode_parm_hdr *)current_mode_settings;
    rc = ipr_mode_sense(cur_ioa->ioa.gen_name, 0x28, mode_parm_hdr);

    if (rc != 0)
        return;

    page_28_sense = (struct ipr_pagewh_28 *)mode_parm_hdr;
    page_28_cur = (struct ipr_page_28 *)
        (((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

    /* now issue mode sense to get changeable parms */
    mode_parm_hdr = (struct ipr_mode_parm_hdr *)changeable_mode_settings;
    page_28_chg = (struct ipr_page_28 *)
        (((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

    /* Now issue mode sense to get default parms */
    mode_parm_hdr = (struct ipr_mode_parm_hdr *)default_mode_settings;
    page_28_dflt = (struct ipr_page_28 *)
        (((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

    /* determine changable and default values */
    for (i = 0;
         i < page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        page_28_chg->attr[i].qas_capability = 0;
        page_28_chg->attr[i].scsi_id = 0; //FIXME!!! need to allow dart (by
                                            // vend/prod & subsystem id)
        page_28_chg->attr[i].bus_width = 1;
        page_28_chg->attr[i].max_xfer_rate = 1;

        page_28_dflt->attr[i].qas_capability = 0;
        page_28_dflt->attr[i].scsi_id = page_28_cur->attr[i].scsi_id;
        page_28_dflt->attr[i].bus_width = 16;
        page_28_dflt->attr[i].max_xfer_rate = 320;  //FIXME!!!  create backplane table check
    }

    /* extract data from saved file to send in mode select */
    sprintf(file_name,"%x_%s",
            cur_ioa->ccin, cur_ioa->pci_address);

    for (i = 0;
         i < page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        if (page_28_chg->attr[i].qas_capability)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    page_28_cur->attr[i].res_addr.bus);

            rc = ipr_config_file_read(file_name,
                                      category,
                                      IPR_QAS_CAPABILITY,
                                      value_str);
            if (rc == RC_SUCCESS)
            {
                sscanf(value_str,"%d", &new_value);
                page_28_cur->attr[i].qas_capability = new_value;
            }
        }

        if (page_28_chg->attr[i].scsi_id)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    page_28_cur->attr[i].res_addr.bus);

            rc = ipr_config_file_read(file_name,
                                      category,
                                      IPR_HOST_SCSI_ID,
                                      value_str);
            if (rc == RC_SUCCESS)
            {
                sscanf(value_str,"%d", &new_value);
                if (page_28_cur->attr[i].scsi_id != new_value)
                {
                    is_reset_req = 1;
                    page_28_cur->attr[i].scsi_id = new_value;
                }
            }
        }

        if (page_28_chg->attr[i].bus_width)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    page_28_cur->attr[i].res_addr.bus);

            rc = ipr_config_file_read(file_name,
                                      category,
                                      IPR_BUS_WIDTH,
                                      value_str);
            if (rc == RC_SUCCESS)
            {
                sscanf(value_str,"%d", &new_value);
                page_28_cur->attr[i].bus_width = new_value;
            }
        }

        if (page_28_chg->attr[i].max_xfer_rate)
        {
            if (limited_config & (1 << page_28_cur->attr[i].res_addr.bus))
            {
                max_xfer_rate = ((iprtoh32(page_28_cur->attr[i].max_xfer_rate)) *
                     page_28_cur->attr[i].bus_width)/(10 * 8);

                if (IPR_LIMITED_MAX_XFER_RATE < max_xfer_rate)
                {
                    page_28_cur->attr[i].max_xfer_rate =
                        htoipr32(IPR_LIMITED_MAX_XFER_RATE * 10 /
                                 (page_28_cur->attr[i].bus_width / 8));
                }

                if (limited_config & IPR_SAVE_LIMITED_CONFIG)
                {
                    /* update config file to indicate present restriction */
                    sprintf(category,"[%s%d]",
                            IPR_CATEGORY_BUS,
                            page_28_cur->attr[i].res_addr.bus);
                    sprintf(value_str,"%d",
                            (iprtoh32(page_28_cur->attr[i].max_xfer_rate) *
                             (page_28_cur->attr[i].bus_width / 8))/10);

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
                        page_28_cur->attr[i].res_addr.bus);

                rc = ipr_config_file_read(file_name,
                                          category,
                                          IPR_MAX_XFER_RATE,
                                          value_str);

                if (rc == RC_SUCCESS)
                {
                    sscanf(value_str,"%d", &new_value);

                    max_xfer_rate = ((iprtoh32(page_28_dflt->attr[i].max_xfer_rate)) *
                                     page_28_cur->attr[i].bus_width)/(10 * 8);

                    if (new_value <= max_xfer_rate)
                    {
                        page_28_cur->attr[i].max_xfer_rate =
                            htoipr32(new_value * 10 /
                                     (page_28_cur->attr[i].bus_width / 8));
                    }
                    else
                    {
                        page_28_cur->attr[i].max_xfer_rate =
                            page_28_dflt->attr[i].max_xfer_rate;

                        /* update config file to indicate present restriction */
                        sprintf(category,"[%s%d]",
                                IPR_CATEGORY_BUS,
                                page_28_cur->attr[i].res_addr.bus);
                        sprintf(value_str,"%d",
                                (iprtoh32(page_28_cur->attr[i].max_xfer_rate) *
                                 (page_28_cur->attr[i].bus_width / 8))/10);

                        ipr_config_file_entry(file_name,
                                              category,
                                              IPR_MAX_XFER_RATE,
                                              value_str,
                                              1);
                    }
                }
                else
                {
                    page_28_cur->attr[i].max_xfer_rate =
                        page_28_dflt->attr[i].max_xfer_rate;
                }
            }
        }

        if (page_28_chg->attr[i].min_time_delay)
        {
            sprintf(category,"[%s%d]",
                    IPR_CATEGORY_BUS,
                    page_28_cur->attr[i].res_addr.bus);

            rc = ipr_config_file_read(file_name,
                                      category,
                                      IPR_MIN_TIME_DELAY,
                                      value_str);
            if (rc == RC_SUCCESS)
            {
                sscanf(value_str,"%d", &new_value);
                page_28_cur->attr[i].min_time_delay = new_value;
            }
            else
            {
                page_28_cur->attr[i].min_time_delay = IPR_INIT_SPINUP_DELAY;
            }
        }
    }

    /* issue mode sense and reset if necessary */
    mode_parm_hdr = (struct ipr_mode_parm_hdr *)page_28_sense; 
    length = mode_parm_hdr->length + 1;
    mode_parm_hdr->length = 0;

    rc = ipr_mode_select(cur_ioa->ioa.gen_name, page_28_sense, length);

    if (rc != 0)
        return;

    if ((is_reset_req) &&
        (!reset_scheduled))
        ipr_reset_adapter(cur_ioa);
}

void ipr_set_page_28_init(struct ipr_ioa *cur_ioa,
                          int limited_config)
{
    int rc, i;
    char current_mode_settings[255];
    char changeable_mode_settings[255];
    char default_mode_settings[255];
    struct ipr_mode_parm_hdr *mode_parm_hdr;
    int issue_mode_select = 0;
    struct ipr_pagewh_28 *page_28_sense;
    struct ipr_page_28 *page_28_cur, *page_28_chg, *page_28_dflt;
    struct ipr_page_28 page_28_ipr;
    u32 max_xfer_rate;
    int length;

    memset(current_mode_settings,0,sizeof(current_mode_settings));
    memset(changeable_mode_settings,0,sizeof(changeable_mode_settings));
    memset(default_mode_settings,0,sizeof(default_mode_settings));

    /* Mode sense page28 to get current parms */
    mode_parm_hdr = (struct ipr_mode_parm_hdr *)current_mode_settings;
    rc = ipr_mode_sense(cur_ioa->ioa.gen_name, 0x28, mode_parm_hdr);

    if (rc != 0)
        return;

    page_28_sense = (struct ipr_pagewh_28 *)mode_parm_hdr;
    page_28_cur = (struct ipr_page_28 *)
        (((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

    /* Now issue mode sense to get changeable parms */
    mode_parm_hdr = (struct ipr_mode_parm_hdr *)changeable_mode_settings;
    page_28_chg = (struct ipr_page_28 *)
        (((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

    /* Now issue mode sense to get default parms */
    mode_parm_hdr = (struct ipr_mode_parm_hdr *)default_mode_settings;
    page_28_dflt = (struct ipr_page_28 *)
        (((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

    /* determine changable and default values */
    for (i = 0;
         i < page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        page_28_chg->attr[i].qas_capability = 0;
        page_28_chg->attr[i].scsi_id = 0; //FIXME!!! need to allow dart (by
                                            // vend/prod & subsystem id)
        page_28_chg->attr[i].bus_width = 1;
        page_28_chg->attr[i].max_xfer_rate = 1;

        page_28_dflt->attr[i].qas_capability = 0;
        page_28_dflt->attr[i].scsi_id = page_28_cur->attr[i].scsi_id;
        page_28_dflt->attr[i].bus_width = 16;
        page_28_dflt->attr[i].max_xfer_rate = 320;  //FIXME  check backplane table
    }

    for (i = 0;
         i < page_28_cur->page_hdr.num_dev_entries;
         i++)
    {
        if (page_28_chg->attr[i].min_time_delay)
            page_28_cur->attr[i].min_time_delay = IPR_INIT_SPINUP_DELAY;

        if ((limited_config & (1 << page_28_cur->attr[i].res_addr.bus)))
        {
            max_xfer_rate = ((iprtoh32(page_28_cur->attr[i].max_xfer_rate)) *
                             page_28_cur->attr[i].bus_width)/(10 * 8);

            if (IPR_LIMITED_MAX_XFER_RATE < max_xfer_rate)
            {
                page_28_cur->attr[i].max_xfer_rate =
                    htoipr32(IPR_LIMITED_MAX_XFER_RATE * 10 /
                             (page_28_cur->attr[i].bus_width / 8));
            }
        }
        else
        {
            page_28_cur->attr[i].max_xfer_rate =
                page_28_dflt->attr[i].max_xfer_rate;
        }

        issue_mode_select++;
    }

    /* Issue mode select */
    if (issue_mode_select)
    {
        mode_parm_hdr = (struct ipr_mode_parm_hdr *)page_28_sense; 
        length = mode_parm_hdr->length + 1;
        mode_parm_hdr->length = 0;

        rc = ipr_mode_select(cur_ioa->ioa.gen_name, page_28_sense, length);

        /* Zero out user page 28 page, this data is used to indicate
         that no values are being changed, this routine is called
         to create and initialize the file if it does not already
         exist */
        memset(&page_28_ipr, 0, sizeof(struct ipr_pagewh_28));

        ipr_save_page_28(cur_ioa, page_28_cur, page_28_chg,
                         &page_28_ipr);
    }
}

void iprlog_location(struct ipr_ioa *ioa)
{
  syslog(LOG_ERR, 
	 "  PCI Address: %s, Host num: %d",
	 ioa->pci_address, ioa->host_num); /* FIXME */
}

bool is_af_blocked(struct ipr_dev *ipr_dev, int silent)
{
    int i, j, rc;
    struct ipr_std_inq_data std_inq_data;
    struct ipr_dasd_inquiry_page3 dasd_page3_inq;
    u32 ros_rcv_ram_rsvd, min_ucode_level;

    /* Zero out inquiry data */
    memset(&std_inq_data, 0, sizeof(std_inq_data));
    rc = ipr_inquiry(ipr_dev->gen_name, IPR_STD_INQUIRY,
                     &std_inq_data, sizeof(std_inq_data));

    if (rc != 0)
        return false;

    /* Issue page 3 inquiry */
    memset(&dasd_page3_inq, 0, sizeof(dasd_page3_inq));
    rc = ipr_inquiry(ipr_dev->gen_name, 0x03,
                     &dasd_page3_inq, sizeof(dasd_page3_inq));

    if (rc != 0)
        return false;

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
                           "format.", ipr_dev->gen_name);
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
                       "to 522 bytes/sector format.", ipr_dev->gen_name);
            return true;
        }
    }   
    return false;
}
