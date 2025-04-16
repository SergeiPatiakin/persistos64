#ifndef NVME_H
#define NVME_H
#include "lib/cstd.h"

struct pci_device;

ssize_t nvme_write(void *dev, uint8_t* buffer, uint64_t offset, size_t length);
ssize_t nvme_read(void *dev, uint8_t* buffer, uint64_t offset, size_t length);

extern struct file_operations nvme_device_fops;

// Maximum number of parallel commands in NVME_COMMAND_IN_FLIGHT and NVME_COMMAND_COMPLETED states
#define NVME_MAX_COMMANDS 128

#define NVME_COMMAND_FREE 0x91
#define NVME_COMMAND_IN_FLIGHT 0x92
#define NVME_COMMAND_COMPLETED 0x93

struct nvme_device {
    struct pci_device *pci_device;
    uint16_t major_version;
    uint8_t minor_version;
    
    uint8_t dstrd_exponent;
    uint8_t lbads_exponent; // lbads_exponent == 9 => formatted LBA size is 512
    uint16_t metadata_size; // today this is unused
    uint64_t lba_count; // NVME namespace consists of LBA 0 through LBA (lba_count-1)

    void *asq;
    uint8_t asq_tail;
    uint8_t asq_head;
    
    void *acq;
    uint8_t acq_head;
    bool acq_phase; // Phase of "old" ACQ entries. New entries should have inverted phase

    void *iosq;
    uint8_t iosq_tail;
    uint8_t iosq_head;

    void *iocq;
    uint8_t iocq_head;
    bool iocq_phase; // Phase of "old" IOCQ entries. New entries should have inverted phase

    uint8_t command_statuses[NVME_MAX_COMMANDS];

    void *admin_result_buffer;
};

#define MAX_NVME_DEVICES 10
extern uint16_t num_nvme_devices;
extern struct nvme_device nvme_devices[MAX_NVME_DEVICES];

void nvme_probe_1(struct pci_device *pci_device);
void nvme_probe_2(struct nvme_device *nvme_device);
void nvme_probe_contents(struct nvme_device *dev);
void nvme_handle_interrupt(uint8_t interrupt_line);
void nvme_readpage(struct nvme_device *dev, uint32_t lba, void* result_page);

#endif
