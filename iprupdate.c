/******************************************************************/
/* Linux IBM IPR microcode update utility                         */
/* Description: IBM Storage IOA Interface Specification (IPR)     */
/*              Linux microcode update utility                    */
/*                                                                */
/* (C) Copyright 2000, 2001                                       */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprupdate.c,v 1.2 2003/10/23 01:50:54 bjking1 Exp $
 */

#include <unistd.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <sys/stat.h>

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/mman.h>

struct image_header
{
    u32 header_length;
    u32 lid_table_offset;
    u8 major_release;
    u8 card_type;
    u8 minor_release[2];
#define LINUX_HEADER_RESERVED_BYTES 20
    u8 reserved[LINUX_HEADER_RESERVED_BYTES];
    char eyecatcher[16];        /* IBMAS400 CCIN */
    u32 num_lids;
    struct ipr_software_inq_lid_info lid[1];
};

int open_dev(struct ipr_ioa *p_ioa, struct ipr_device *p_device);

int main(int argc, char *argv[])
{
    int rc = 0;
    struct stat ucode_stats;
    struct ipr_ioctl_cmd_internal ioa_cmd;
    struct ipr_inquiry_page0 page0_inq;
    struct ipr_inquiry_page3 page3_inq;
    struct ipr_inquiry_page_cx page_cx_inq;
    int num_ioa_lids = 0;
    int num_binary_lids = 0;
    char ucode_file[50];
    int do_download, ucode_fd, dasd_ucode_fd;
    struct image_header *p_image_hdr;
    struct ipr_dasd_ucode_header *p_dasd_image_hdr;
    u32 updated = 0;
    int fd, dev_fd, i, j, num_entries, ioa_num;
    struct ipr_software_inq_lid_info *p_lid_info;
    u32 force_download, force_devs, force_ioas;
    struct ipr_dasd_inquiry_page3 dasd_page3_inq;
    struct ipr_ioa *p_cur_ioa;
    char *p_dev_name;
    struct ipr_device *p_device;
    char config_file_name[64];
    u8 cdb[IPR_CCB_CDB_LEN];
    struct sense_data_t sense_data;
    int image_offset;
    void *p_dasd_image;
    int image_size;
    int device_update_blocked;
    int u320_disabled;

    struct software_inq_lid_info
    {
        u32  load_id;
        u32  timestamp[3];
        u32  found;
    };

    struct software_inq_lid_info ioa_lid_info[50];

    openlog("iprupdate",
            LOG_PERROR | /* Print error to stderr as well */
            LOG_PID |    /* Include the PID with each error */
            LOG_CONS,    /* Write to system console if there is an error
                          sending to system logger */
            LOG_USER);

    force_download = force_devs = force_ioas = 0;

    if (argc > 1)
    {
        if (strcmp(argv[1], "--force") == 0)
            force_download = 1;
        else if (strcmp(argv[1], "--version") == 0)
        {
            printf("iprupdate: %s"IPR_EOL, IPR_VERSION_STR);
            exit(1);
        }
        else if (strcmp(argv[1], "--force-devs") == 0)
        {
            force_devs = 1;
        }
        else if (strcmp(argv[1], "--force-ioas") == 0)
        {
            force_ioas = 1;
        }
        else
        {
            printf("Usage: iprdate [options]"IPR_EOL);
            printf("  Options: --version    Print iprupdate version"IPR_EOL);
            exit(1);
        }
    }

    tool_init("iprupdate");

    check_current_config(false);

    /* Loop for all attached adapters */
    for (p_cur_ioa = p_head_ipr_ioa, ioa_num = 0;
         ioa_num < num_ioas;
         p_cur_ioa = p_cur_ioa->p_next, ioa_num++)
    {
        fd = open(p_cur_ioa->ioa.dev_name, O_RDWR | O_NONBLOCK);

        if (fd < 0)
        {
            syslog(LOG_ERR, "Cannot open %s. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            continue;
        }

        sprintf(ucode_file, "/etc/microcode/ibmsis%X.img", p_cur_ioa->ccin);

        /* Do a page 0 inquiry to the adapter to get the supported page codes */
        memset(&ioa_cmd, 0, sizeof(ioa_cmd));
        ioa_cmd.buffer = &page0_inq;
        ioa_cmd.buffer_len = sizeof(struct ipr_inquiry_page0);
        ioa_cmd.cdb[1] = 0x01;
        ioa_cmd.cdb[2] = 0x00;
        ioa_cmd.read_not_write = 1;

        rc = ipr_ioctl(fd, INQUIRY, &ioa_cmd);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Inquiry to controller %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            close(fd);
            continue;
        }

        /* Starting at page code 0xC0, the first software VPD page,
         issue inquiries to the adapter for the supported pages */
        for (i = 0, num_ioa_lids = 0; (page0_inq.supported_page_codes[i] != 0) &&
            (i < page0_inq.page_length); i++)
        {
            if (page0_inq.supported_page_codes[i] < 0xc0)
                continue;

            memset(&ioa_cmd, 0, sizeof(ioa_cmd));
            ioa_cmd.buffer = &page_cx_inq;
            ioa_cmd.buffer_len = sizeof(struct ipr_inquiry_page_cx);
            ioa_cmd.cdb[1] = 0x01;
            ioa_cmd.cdb[2] = page0_inq.supported_page_codes[i];
            ioa_cmd.read_not_write = 1;

            rc = ipr_ioctl(fd, INQUIRY, &ioa_cmd);

            if (rc != 0)
            {
                syslog(LOG_ERR, "Inquiry to controller %s failed. %m"IPR_EOL,
                       p_cur_ioa->ioa.dev_name);
                continue;
            }

            num_entries = (page_cx_inq.page_length - 4) /
                sizeof(struct ipr_software_inq_lid_info);

            /* Build up a data structure describing all the IOA LIDs
             the adapter currently has loaded */
            for (j = 0;  j < num_entries; j++)
            {
                ioa_lid_info[num_ioa_lids].load_id =
                    page_cx_inq.lidinfo[j].load_id;

                ioa_lid_info[num_ioa_lids].timestamp[0] =
                    page_cx_inq.lidinfo[j].timestamp[0];

                ioa_lid_info[num_ioa_lids].timestamp[1] =
                    page_cx_inq.lidinfo[j].timestamp[1];

                ioa_lid_info[num_ioa_lids].timestamp[2] =
                    page_cx_inq.lidinfo[j].timestamp[2];

                num_ioa_lids++;
            }
        }

        /* Issue a page 3 inquiry to get the versioning information */
        memset(&ioa_cmd, 0, sizeof(ioa_cmd));
        ioa_cmd.buffer = &page3_inq;
        ioa_cmd.buffer_len = sizeof(struct ipr_inquiry_page3);
        ioa_cmd.cdb[1] = 0x01;
        ioa_cmd.cdb[2] = 0x03;
        ioa_cmd.read_not_write = 1;

        rc = ipr_ioctl(fd, INQUIRY, &ioa_cmd);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Inquiry to controller %s failed. %m"IPR_EOL,
                   p_cur_ioa->ioa.dev_name);
            close(fd);
            continue;
        }

        do_download = 0;

        /* Parse the binary file */

        ucode_fd = open(ucode_file, O_RDONLY);

        if (ucode_fd < 0)
        {
            syslog(LOG_ERR, "Could not open firmware file %s. %m"IPR_EOL, ucode_file);
            close(fd);
            continue;
        }

        /* Get the size of the microcode image */
        rc = fstat(ucode_fd, &ucode_stats);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Error accessing IOA firmware file: %s. %m"IPR_EOL, ucode_file);
            close(fd);
            close(ucode_fd);
            continue;
        }

        /* Map the image in memory */
        p_image_hdr = mmap(NULL, ucode_stats.st_size,
                           PROT_READ, MAP_SHARED, ucode_fd, 0);

        if (p_dasd_image_hdr == MAP_FAILED)
        {
            syslog(LOG_ERR, "Error mapping IOA firmware file: %s. %m"IPR_EOL, ucode_file);
            close(fd);
            close(ucode_fd);
            continue;
        }

        p_lid_info = (struct ipr_software_inq_lid_info *)
            ((unsigned long)p_image_hdr + iprtoh32(p_image_hdr->lid_table_offset) + 4);
        num_binary_lids = iprtoh32(*((u32*)p_lid_info - 1));

        /* Compare the two */
        for (i = 0; (i < num_binary_lids) && (do_download == 0); i++, p_lid_info++)
        {
            int found = 0;

            /* Loop through the LIDs loaded on the IOA to find a match */
            for (j = 0; j < num_ioa_lids; j++)
            {
                if (ioa_lid_info[j].load_id == p_lid_info->load_id)
                {
                    found = 1;
                    if (memcmp(p_lid_info->timestamp,
                               ioa_lid_info[j].timestamp,
                               12) != 0)
                    {
                        do_download = 1;
                        break;
                    }
                }
            }
            if (found == 0)
                do_download = 1;
        }

        if ((page3_inq.major_release != p_image_hdr->major_release) ||
            (page3_inq.minor_release[0] != p_image_hdr->minor_release[0]) ||
            (page3_inq.minor_release[1] != p_image_hdr->minor_release[1]))
        {
            do_download = 1;
        }

        u320_disabled = 0;
        
        /* check for mode page 28 changes before adapter reset */
        sprintf(config_file_name,"%x_%x", p_cur_ioa->ccin, p_cur_ioa->host_addr);
        if (RC_SUCCESS == ipr_config_file_valid(config_file_name))
        {
            ipr_set_page_28(p_cur_ioa, IPR_LIMITED_CONFIG,
                            do_download | force_download | force_ioas);
        }
        else
        {
            ipr_set_page_28_init(p_cur_ioa, IPR_LIMITED_CONFIG);
        }

        if ((do_download == 1) ||
            (force_download == 1) ||
            (force_ioas == 1))
        {
            syslog(LOG_NOTICE, "Updating IOA firmware on %s"IPR_EOL, p_cur_ioa->ioa.dev_name);

            /* Do the update if the two are different */
            /* Might want to fork off a process here so we can start
             on the next adapter. Would have to remember the child's PID
             so we could wait on it when we were done, though. This would also
             greatly complicate the load source case */

            memset(&ioa_cmd, 0, sizeof(ioa_cmd));
            ioa_cmd.buffer = (void *)((unsigned long)p_image_hdr + iprtoh32(p_image_hdr->header_length));
            ioa_cmd.buffer_len = ucode_stats.st_size - iprtoh32(p_image_hdr->header_length);
            ioa_cmd.read_not_write = 0;

            /* The write buffer here takes care of both the IOA shutdown and the
             reset of the adapter */

            rc = ipr_ioctl(fd, WRITE_BUFFER, &ioa_cmd);

            if (rc != 0)
                syslog(LOG_ERR, "Write buffer to %s failed. %m"IPR_EOL, p_cur_ioa->ioa.dev_name);
            else
            {
                /* Write buffer was successful. Now we need to see if all our devices
                 are available. The IOA may have been in braindead prior to the ucode
                 download, in which case the upper layer scsi device drivers do not
                 know anything about our devices. */

                check_current_config(false);

                for (j = 0; j < p_cur_ioa->num_devices; j++)
                {
                    /* If not a DASD, ignore */
                    if (!IPR_IS_DASD_DEVICE(p_cur_ioa->dev[j].p_resource_entry->std_inq_data))
                        continue;

                    /* See if we can open device */
                    dev_fd = open(p_cur_ioa->dev[j].dev_name, O_RDWR);

                    if ((dev_fd <= 1) && ((errno == ENOENT) || (errno == ENXIO)))
                    {
                        scan_device(p_cur_ioa->dev[j].p_resource_entry->resource_address,
                                    p_cur_ioa->host_num);
                    }
                    else
                        close(dev_fd);
                }
                updated = 1;
            }
        }

        /* Check DASD microcode */
        for (i = 0; i < p_cur_ioa->num_devices; i++)
        {
            p_device = &p_cur_ioa->dev[i];

            /* If not a DASD, ignore */
            if (!IPR_IS_DASD_DEVICE(p_cur_ioa->dev[i].p_resource_entry->std_inq_data) ||
                ((p_device->p_resource_entry->subtype != IPR_SUBTYPE_AF_DASD) &&
                 (p_device->p_resource_entry->subtype != IPR_SUBTYPE_GENERIC_SCSI)))
                continue;

            if (p_device->p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI)
                p_dev_name = p_device->gen_name;
            else
                p_dev_name = p_cur_ioa->ioa.dev_name;

            /* Do a page 3 inquiry */
            dev_fd = open_dev(p_cur_ioa, p_device);

            if (dev_fd > 1)
            {
                if (p_device->p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI) 
                {
                    memset(&dasd_page3_inq, 0, sizeof(dasd_page3_inq));
                    memset(cdb, 0, IPR_CCB_CDB_LEN);

                    /* Issue page 3 inquiry */
                    cdb[0] = INQUIRY;
                    cdb[1] = 0x01; /* EVPD */
                    cdb[2] = 0x03; /* Page 3 */
                    cdb[4] = sizeof(dasd_page3_inq);

                    rc = sg_ioctl(dev_fd, cdb, &dasd_page3_inq,
                                  sizeof(dasd_page3_inq),
                                  SG_DXFER_FROM_DEV, &sense_data, 30);

                    if (rc == CHECK_CONDITION)
                    {
                        syslog(LOG_ERR, "Page 3 inquiry to %02X%02X%02X%02X failed: %x %x %x"IPR_EOL,
                               p_device->p_resource_entry->host_no,
                               p_device->p_resource_entry->resource_address.bus,
                               p_device->p_resource_entry->resource_address.target,
                               p_device->p_resource_entry->resource_address.lun,
                               sense_data.sense_key & 0x0F,
                               sense_data.add_sense_code,
                               sense_data.add_sense_code_qual);
                        close(dev_fd);
                        continue;
                    }
                }
                else
                {
                    memset(&ioa_cmd, 0, sizeof(ioa_cmd));

                    ioa_cmd.buffer = &dasd_page3_inq;
                    ioa_cmd.buffer_len = sizeof(dasd_page3_inq);
                    ioa_cmd.cdb[1] = 0x01;
                    ioa_cmd.cdb[2] = 0x03;
                    ioa_cmd.read_not_write = 1;
                    ioa_cmd.device_cmd = 1;
                    ioa_cmd.resource_address = p_device->p_resource_entry->resource_address;

                    rc = ipr_ioctl(dev_fd, INQUIRY, &ioa_cmd);
                }

                if (rc != 0)
                {
                    syslog(LOG_ERR, "Inquiry to %02X%02X%02X%02X failed. %m"IPR_EOL,
                           p_device->p_resource_entry->host_no,
                           p_device->p_resource_entry->resource_address.bus,
                           p_device->p_resource_entry->resource_address.target,
                           p_device->p_resource_entry->resource_address.lun);
                    close(dev_fd);
                    continue;
                }

                device_update_blocked = 0;

                /* check if vendor id is IBMAS400 */
                if (memcmp(p_device->p_resource_entry->std_inq_data.vpids.vendor_id, "IBMAS400", 8) == 0)
                {
                    sprintf(ucode_file, "/etc/microcode/device/ibmsis%02X%02X%02X%02X.img",
                            dasd_page3_inq.load_id[0], dasd_page3_inq.load_id[1],
                            dasd_page3_inq.load_id[2],
                            dasd_page3_inq.load_id[3]);
                    image_offset = 0;
                }
                else if (memcmp(p_device->p_resource_entry->std_inq_data.vpids.vendor_id, "IBM     ", 8) == 0)
                {
                    if (p_device->opens)
                    {
                        device_update_blocked = 1;
                    }

                    sprintf(ucode_file, "/etc/microcode/device/%.7s.%02X%02X%02X%02X",
                            p_device->p_resource_entry->std_inq_data.vpids.product_id,
                            dasd_page3_inq.load_id[0], dasd_page3_inq.load_id[1],
                            dasd_page3_inq.load_id[2],
                            dasd_page3_inq.load_id[3]);
                    image_offset = sizeof(struct ipr_dasd_ucode_header);
                }
                else if ((platform == IPR_ISERIES) || (platform == IPR_PSERIES))
                {
                    close(dev_fd);
                    continue;
                }
                else
                {
                    close(dev_fd);
                    continue;
                }

                do_download = 0;

                /* Parse the binary file */

                dasd_ucode_fd = open(ucode_file, O_RDONLY);

                if (dasd_ucode_fd < 0)
                {
                    syslog_dbg(LOG_ERR, "Could not open firmware file %s. %m"IPR_EOL, ucode_file);
                    close(dev_fd);
                    continue;
                }

                /* Get the size of the microcode image */
                rc = fstat(dasd_ucode_fd, &ucode_stats);

                if (rc != 0)
                {
                    syslog(LOG_ERR, "Error accessing firmware file: %s. %m"IPR_EOL, ucode_file);
                    close(dev_fd);
                    close(dasd_ucode_fd);
                    continue;
                }

                /* Map the image in memory */
                p_dasd_image_hdr = mmap(NULL, ucode_stats.st_size,
                                        PROT_READ, MAP_SHARED, dasd_ucode_fd, 0);

                if (p_dasd_image_hdr == MAP_FAILED)
                {
                    syslog(LOG_ERR, "Error reading firmware file: %s. %m"IPR_EOL, ucode_file);
                    close(dev_fd);
                    close(dasd_ucode_fd);
                    continue;
                }

                if ((memcmp(p_dasd_image_hdr->load_id, dasd_page3_inq.load_id, 4)) &&
                    (memcmp(p_device->p_resource_entry->std_inq_data.vpids.vendor_id, "IBMAS400", 8) == 0))
                {
                    syslog(LOG_ERR, "Firmware file corrupt: %s. %m"IPR_EOL, ucode_file);
                    close(dev_fd);
                    close(dasd_ucode_fd);
                    munmap(p_dasd_image_hdr, ucode_stats.st_size);
                    continue;
                }

                if ((memcmp(p_dasd_image_hdr->modification_level,
                            dasd_page3_inq.release_level, 4) > 0) ||
                    (force_download == 1) ||
                    (force_devs == 1))
                {
                    if (device_update_blocked)
                    {
                        if (is_af_blocked(&p_cur_ioa->dev[j], 1))
                        {
                            u320_disabled |= (1 << p_device->p_resource_entry->resource_address.bus) |
                                IPR_SAVE_LIMITED_CONFIG;
                        }

                        syslog(LOG_ERR,"Device update to %02X%02X%02X%02X blocked, device in use."IPR_EOL,
                               p_device->p_resource_entry->host_no,
                               p_device->p_resource_entry->resource_address.bus,
                               p_device->p_resource_entry->resource_address.target,
                               p_device->p_resource_entry->resource_address.lun);
                        continue;
                    }

                    if ((p_device->p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI) &&
                        (memcmp(p_device->p_resource_entry->std_inq_data.vpids.vendor_id, "IBMAS400", 8) == 0))
                    {
                        /* updating AS400 drives which are 512 formatted is not a supported operation */
                        continue;
                    }

                    if (isprint(p_dasd_image_hdr->modification_level[0]) &&
                        isprint(p_dasd_image_hdr->modification_level[1]) &&
                        isprint(p_dasd_image_hdr->modification_level[2]) &&
                        isprint(p_dasd_image_hdr->modification_level[3]) &&
                        isprint(dasd_page3_inq.release_level[0]) &&
                        isprint(dasd_page3_inq.release_level[1]) &&
                        isprint(dasd_page3_inq.release_level[2]) &&
                        isprint(dasd_page3_inq.release_level[3]))
                    {
                        syslog(LOG_NOTICE, "Updating DASD firmware on %02X%02X%02X%02X, using file %s from version %c%c%c%c to %c%c%c%c"IPR_EOL,
                               p_device->p_resource_entry->host_no,
                               p_device->p_resource_entry->resource_address.bus,
                               p_device->p_resource_entry->resource_address.target,
                               p_device->p_resource_entry->resource_address.lun,
                               ucode_file,
                               dasd_page3_inq.release_level[0],
                               dasd_page3_inq.release_level[1],
                               dasd_page3_inq.release_level[2],
                               dasd_page3_inq.release_level[3],
                               p_dasd_image_hdr->modification_level[0],
                               p_dasd_image_hdr->modification_level[1],
                               p_dasd_image_hdr->modification_level[2],
                               p_dasd_image_hdr->modification_level[3]);
                    }
                    else
                    {
                        syslog(LOG_NOTICE, "Updating DASD firmware on %02X%02X%02X%02X, using file %s"IPR_EOL,
                               p_device->p_resource_entry->host_no,
                               p_device->p_resource_entry->resource_address.bus,
                               p_device->p_resource_entry->resource_address.target,
                               p_device->p_resource_entry->resource_address.lun,
                               ucode_file);
                    }

                    p_dasd_image = (void *)p_dasd_image_hdr + image_offset;
                    image_size = ucode_stats.st_size - image_offset;

                    /* Do the update if the firmware file is newer than the firmware loaded on the DASD */
                    if (p_device->p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI) 
                    {
                        memset(cdb, 0, IPR_CCB_CDB_LEN);

                        /* Issue mode sense to get the block size */
                        cdb[0] = WRITE_BUFFER;
                        cdb[1] = 5; /* Mode 5 */
                        cdb[6] = (image_size & 0xff0000) >> 16;
                        cdb[7] = (image_size & 0x00ff00) >> 8;
                        cdb[8] = image_size & 0x0000ff;

                        rc = sg_ioctl(dev_fd, cdb,
                                      p_dasd_image, image_size, SG_DXFER_TO_DEV,
                                      &sense_data, 120);            
                    }
                    else
                    {
                        memset(&ioa_cmd, 0, sizeof(ioa_cmd));
                        ioa_cmd.buffer = (void *)p_dasd_image;
                        ioa_cmd.buffer_len = image_size;
                        ioa_cmd.read_not_write = 0;
                        ioa_cmd.device_cmd = 1;
                        ioa_cmd.resource_address = p_device->p_resource_entry->resource_address;

                        rc = ipr_ioctl(dev_fd, WRITE_BUFFER, &ioa_cmd);
                    }

                    if (rc != 0)
                        syslog(LOG_ERR, "Write buffer to %02X%02X%02X%02X failed. %m"IPR_EOL,
                               p_device->p_resource_entry->host_no,
                               p_device->p_resource_entry->resource_address.bus,
                               p_device->p_resource_entry->resource_address.target,
                               p_device->p_resource_entry->resource_address.lun);
                }

                close(dev_fd);
                close(dasd_ucode_fd);

                if (munmap(p_dasd_image_hdr, ucode_stats.st_size))
                    syslog(LOG_ERR, "munmap failed. %m"IPR_EOL);
            }
            else
            {
                syslog(LOG_ERR, "Cannot open device %s. Retry failed. %m"IPR_EOL, p_dev_name);
                continue;
            }
        }

        munmap(p_image_hdr, ucode_stats.st_size);
        close (ucode_fd);
        close (fd);

        if (RC_SUCCESS == ipr_config_file_valid(config_file_name))
        {
            ipr_set_page_28(p_cur_ioa, u320_disabled,
                            do_download | force_download | force_ioas);
        }
        else
        {
            ipr_set_page_28_init(p_cur_ioa, u320_disabled);
        }
    }

    return rc;
}

int open_dev(struct ipr_ioa *p_ioa, struct ipr_device *p_device)
{
    int dev_fd;

    if (p_device->p_resource_entry->subtype == IPR_SUBTYPE_GENERIC_SCSI)
    {
        dev_fd = open(p_device->gen_name, O_RDWR);

        if ((dev_fd <= 1) && ((errno == ENOENT) || (errno == ENXIO)))
        {
            syslog(LOG_ERR, "Cannot open device %s. %m"IPR_EOL, p_device->gen_name);
            syslog(LOG_ERR, "Rescanning SCSI bus for device"IPR_EOL);
            scan_device(p_device->p_resource_entry->resource_address, p_ioa->host_num);

            dev_fd = open(p_device->gen_name, O_RDWR);
        }
    }
    else
        dev_fd = open(p_ioa->ioa.dev_name, O_RDWR);

    return dev_fd;
}

