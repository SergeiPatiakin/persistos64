// Driver for NVME device partitions
#include <stdint.h>
#include <stdbool.h>
#include "drivers/device-numbers.h"
#include "drivers/nvme.h"
#include "drivers/tty.h"
#include "fs/vfs.h"
#include "mm/kmem.h"
#include "nvmepart.h"

uint16_t num_nvmepart_devices = 0;
struct nvmepart_device nvmepart_devices[MAX_NVMEPART_DEVICES];

// On disk structure
struct gpt_partition_entry {
    uint32_t partition_type_guid[4];
    uint32_t partition_guid[4];
    uint32_t first_lba_low;
    uint32_t first_lba_high;
    uint32_t last_lba_low;
    uint32_t last_lba_high;
    uint8_t attributes[8];
    uint8_t partition_name_utf16[72];
} __attribute__((packed));

// disk_start_buffer holds first 5 pages of disk content
void nvmepart_gpt_probe(struct nvme_device *dev, void *disk_start_buffer) {
    uint32_t num_partition_entries = *((uint32_t*)(disk_start_buffer + 0x250));
    uint32_t partition_entry_size = *((uint32_t*)(disk_start_buffer + 0x254));
    for (uint32_t pnum = 0; pnum < num_partition_entries; pnum++) {
        // Offset of partition entry from start of disk
        uint32_t partition_entry_offset = 0x400 + pnum * partition_entry_size;
        if (partition_entry_size + partition_entry_offset > 4096 * 5) {
            printk("GPT partition entries go past disk_start_buffer\n");
            return;
        }
        struct gpt_partition_entry *pe = disk_start_buffer + partition_entry_offset;
        if (
            pe->partition_type_guid[0] == 0 &&
            pe->partition_type_guid[1] == 0 &&
            pe->partition_type_guid[2] == 0 &&
            pe->partition_type_guid[3] == 0
        ) {
            // Unused GPT entry
            continue;
        }
        if (pe->first_lba_high != 0) {
            printk("GPT partition start exceeds 2TB\n");
            return;
        }
        if (pe->last_lba_high != 0) {
            printk("GPT partition end exceeds 2TB\n");
            return;
        }
        uint16_t device_number = num_nvmepart_devices++;
        struct nvmepart_device *partition = &nvmepart_devices[device_number];
        partition->nvme_device = dev;
        for (int i = 0; i < 4; i++) {
            partition->partition_type_guid[i] = pe->partition_type_guid[i];
        }
        for (int i = 0; i < 4; i++) {
            partition->partition_guid[i] = pe->partition_guid[i];
        }
        partition->first_lba = pe->first_lba_low;
        partition->last_lba = pe->last_lba_low;
        memset(partition->partition_name, 0, 37);
        // Convert UTF-16 to ASCII
        for (int i = 0; i < 36; i++) {
            partition->partition_name[i] = pe->partition_name_utf16[2*i];
            if (partition->partition_name[i] == 0) {
            break;
            }
        }
        // Add to VFS
        uint8_t name_buffer[8];
        strcpy(name_buffer, u8p("nvmeXpY"));
        name_buffer[4] = '0' + (dev - nvme_devices); // TODO: proper name logic for >9 devices
        name_buffer[6] = '1' + pnum; // TODO: proper name logic for >9 partitions
        vfs_mknod(
            vfs_dev_dir_inode,
            name_buffer,
            DEVICE_NVMEPART,
            device_number,
            &nvmepart_device_ops,
            partition
        );
    }
}

void nvmepart_mbr_probe(struct nvme_device *dev, void *disk_start_buffer) {
    for (uint32_t pnum = 0; pnum < 4; pnum++) {
        uint8_t status = *((uint8_t*)(disk_start_buffer + 0x1BE + 16 * pnum));
        if (!(status & 0x80)) {
            printk("mbr_part status=%1X inactive\n", status);
            continue;
        }
        uint32_t first_lba;
        uint32_t num_sectors;
        memcpy(&first_lba, disk_start_buffer + 0x1BE + 16 * pnum + 0x08, 4);
        memcpy(&num_sectors, disk_start_buffer + 0x1BE + 16 * pnum + 0x0C, 4);
        // printk("mbr_part status=%1X first_lba=%4X num_sectors=%4X\n", status, first_lba, num_sectors);
        uint16_t device_number = num_nvmepart_devices++;
        struct nvmepart_device *partition = &nvmepart_devices[device_number];
        partition->nvme_device = dev;
        for (int i = 0; i < 4; i++) {
            partition->partition_type_guid[i] = 0;
        }
        for (int i = 0; i < 4; i++) {
            // Dummy GUID
            partition->partition_guid[i] = i == 3 ? device_number : 0;
        }
        partition->first_lba = first_lba;
        partition->last_lba = first_lba + num_sectors - 1;
        memset(partition->partition_name, 0, 37);
        for (int i = 0; i < 36; i++) {
            partition->partition_name[i] = 0;
        }
        // Add to VFS
        uint8_t name_buffer[8];
        strcpy(name_buffer, u8p("nvmeXpY"));
        name_buffer[4] = '0' + (dev - nvme_devices); // TODO: proper name logic for >9 devices
        name_buffer[6] = '1' + pnum; // TODO: proper name logic for >9 partitions
        vfs_mknod(
            vfs_dev_dir_inode,
            name_buffer,
            DEVICE_NVMEPART,
            device_number,
            &nvmepart_device_ops,
            partition
        );
    }
}

void nvmepart_probe(struct nvme_device *dev) {
    void *disk_start_buffer = kpage_alloc(5);
    nvme_readpage(dev, 0, disk_start_buffer);
    nvme_readpage(dev, 8 * 1, disk_start_buffer + 4096 * 1); // Hack: what if block size is different to 512?
    nvme_readpage(dev, 8 * 2, disk_start_buffer + 4096 * 2);
    nvme_readpage(dev, 8 * 3, disk_start_buffer + 4096 * 3);
    nvme_readpage(dev, 8 * 4, disk_start_buffer + 4096 * 4);
    if (memcmp(disk_start_buffer + 0x200, u8p("EFI PART"), 8) == 0) {
        nvmepart_gpt_probe(dev, disk_start_buffer);
    } else if (memcmp(disk_start_buffer + 0x1FE, u8p("\x55\xAA"), 2) == 0) {
        nvmepart_mbr_probe(dev, disk_start_buffer);
    } else {
        printk("No partition table found on nvme device 0x%1X\n", dev - nvme_devices);
        return;
    }

}

ssize_t nvmepart_read(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    struct nvmepart_device *partition = (struct nvmepart_device*)dev;
    uint64_t nvme_device_offset = offset + (partition->first_lba << partition->nvme_device->lbads_exponent);
    uint64_t readable_length = ((partition->last_lba + 1) << partition->nvme_device->lbads_exponent) - nvme_device_offset;
    if (length > readable_length) {
        length = readable_length;
    }
    return nvme_read(partition->nvme_device, buffer, nvme_device_offset, length);
}

ssize_t nvmepart_write(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    struct nvmepart_device *partition = (struct nvmepart_device*)dev;
    uint64_t nvme_device_offset = offset + (partition->first_lba << partition->nvme_device->lbads_exponent);
    uint64_t writable_length = ((partition->last_lba + 1) << partition->nvme_device->lbads_exponent) - nvme_device_offset;
    if (length > writable_length) {
        length = writable_length;
    }
    return nvme_write(partition->nvme_device, buffer, nvme_device_offset, length);
}

struct device_operations nvmepart_device_ops = {
    .read = nvmepart_read,
    .write = nvmepart_write,
};
