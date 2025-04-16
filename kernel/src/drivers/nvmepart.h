#ifndef NVMEPART_H
#define NVMEPART_H

#define MAX_NVMEPART_DEVICES 64

struct nvme_device;

extern struct file_operations nvmepart_device_fops;

struct nvmepart_device {
    struct nvme_device *nvme_device;
    uint32_t partition_type_guid[4];
    uint32_t partition_guid[4];
    uint64_t first_lba;
    uint64_t last_lba; // Inclusive
    uint8_t partition_name[37];
};

void nvmepart_probe(struct nvme_device *dev);

#endif
