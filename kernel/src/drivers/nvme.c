#include "arch/asm.h"
#include "drivers/device-numbers.h"
#include "drivers/pci.h"
#include "drivers/tty.h"
#include "drivers/nvmepart.h"
#include "fs/vfs.h"
#include "lib/cstd.h"
#include "kernel/scheduler.h"
#include "mm/kmem.h"
#include "mm/map.h"
#include "mm/page.h"
#include "nvme.h"

#define NVME_QUEUE_SIZE 64
#define NVME_QUEUE_SIZE_MINUS_ONE (NVME_QUEUE_SIZE - 1)
#define NVME_IOSQ_ID 1
#define NVME_IOCQ_ID 1

uint16_t num_nvme_devices = 0;
struct nvme_device nvme_devices[MAX_NVME_DEVICES];

struct nvme_sq_entry {
    uint32_t command;
    uint32_t nsid;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t metadata1;
    uint32_t metadata2;
    uint32_t data1;
    uint32_t data2;
    uint32_t data3;
    uint32_t data4;
    uint32_t suffix1;
    uint32_t suffix2;
    uint32_t suffix3;
    uint32_t suffix4;
    uint32_t suffix5;
    uint32_t suffix6;
} __attribute__((packed));

struct nvme_cq_entry {
    uint32_t data1;
    uint32_t reserved;
    uint16_t sq_head_pointer;
    uint16_t sq_identifier;
    uint16_t command_identifier;
    uint16_t status_field;
} __attribute__((packed));

uint16_t start_command(struct nvme_device *dev) {
    uint16_t command_id = 0;
    while(dev->command_statuses[command_id] != NVME_COMMAND_FREE){
        command_id++;
        if (command_id == NVME_MAX_COMMANDS) {
            command_id = 0;
            // task_yield();
        }
    }
    dev->command_statuses[command_id] = NVME_COMMAND_IN_FLIGHT; // free -> in flight
    return command_id;
}

void finish_command(struct nvme_device *dev, uint16_t command_id) {
    if (command_id > NVME_MAX_COMMANDS) {
        printk(u8p("Cannot finish invalid command_id: "));
        printk_uint16(command_id);
        printk(u8p("\n"));
    }
    dev->command_statuses[command_id] = NVME_COMMAND_COMPLETED; // in flight -> completed
}

// Must be called in task context
void await_command_finish(struct nvme_device *dev, uint16_t command_id) {
    while (dev->command_statuses[command_id] == NVME_COMMAND_IN_FLIGHT) {
        task_yield();
    }
}


void free_command(struct nvme_device *dev, uint16_t command_id) {
    dev->command_statuses[command_id] = NVME_COMMAND_FREE; // in progress -> completed
}

// Must be called in boot context
void nvme_probe_1(struct pci_device *pci_device) {
    uint16_t device_number = num_nvme_devices++;
    struct nvme_device *nvme_device = &nvme_devices[device_number];
    nvme_device->pci_device = pci_device;

    uint8_t name_buffer[6];
    strcpy(name_buffer, u8p("nvmeX"));
    name_buffer[4] = '0' + device_number; // TODO: proper name logic for >9 devices
    vfs_mknod(
        vfs_dev_dir_inode,
        name_buffer,
        DEVICE_NVME,
        device_number,
        &nvme_device_fops,
        nvme_device
    );

    uint64_t num_device_pages = pci_device->mmio_size >> 12;

    // Map device memory
    pci_device->mmio_virt_base = (uint64_t)dpage_alloc(num_device_pages);
    for (uint64_t i = 0; i < num_device_pages; i++) {
        set_page_mapping(
            (void*)read_cr3() + hhdm_offset,
            (void*)pci_device->mmio_virt_base + i * PAGE_SIZE,
            (void*)pci_device->mmio_phys_base + i * PAGE_SIZE,
            true
        );
    }
    // Flush TLB
    asm volatile (
        "movq %%cr3, %%rax\n"
        "movq %%rax, %%cr3" : : : "%rax"
    );
}

// Must be called in task context
void nvme_probe_2(struct nvme_device *nvme_device) {
    struct pci_device *pci_device = nvme_device->pci_device;

    uint32_t dword_4 = pci_config_read_dword(
        pci_device->bus,
        pci_device->slot,
        pci_device->function,
        4
    );
    // Enable memory-space access
    dword_4 |= (1 << 1);
    // Enable bus mastering
    dword_4 |= (1 << 2);
    // Enable interrupts
    dword_4 &= ~(1 << 10);
    pci_config_write_dword(
        pci_device->bus,
        pci_device->slot,
        pci_device->function,
        4,
        dword_4
    );

    void* admin_submission_queue = kpage_alloc(1);
    void* admin_completion_queue = kpage_alloc(1);
    void* io_submission_queue = kpage_alloc(1);
    void* io_completion_queue = kpage_alloc(1);
    void* admin_result_buffer = kpage_alloc(1);
    
    memset(admin_submission_queue, 0, PAGE_SIZE);
    memset(admin_completion_queue, 0, PAGE_SIZE);
    memset(io_submission_queue, 0, PAGE_SIZE);
    memset(io_completion_queue, 0, PAGE_SIZE);
    memset(admin_result_buffer, 0, PAGE_SIZE);

    // Reset controller
    *((uint32_t*)(pci_device->mmio_virt_base + 0x14)) = *((uint32_t*)(pci_device->mmio_virt_base + 0x14)) & 0xFFFFFFFE;

    // Wait for ready bit to become 0
    while (*((uint32_t*)(pci_device->mmio_virt_base + 0x1C)) & 0x1);

    // Write ASQB
    *((uint64_t*)(pci_device->mmio_virt_base + 0x28)) = (uint64_t)(admin_submission_queue - hhdm_offset);

    // Write ACQB
    *((uint64_t*)(pci_device->mmio_virt_base + 0x30)) = (uint64_t)(admin_completion_queue - hhdm_offset);

    // Write AQA
    *((uint32_t*)(pci_device->mmio_virt_base + 0x24)) = ((uint32_t)NVME_QUEUE_SIZE_MINUS_ONE << 16) | (uint32_t)NVME_QUEUE_SIZE_MINUS_ONE;

    // Enable controlller
    *((uint32_t*)(pci_device->mmio_virt_base + 0x14)) = *((uint32_t*)(pci_device->mmio_virt_base + 0x14)) | NVME_IOSQ_ID;

    // Wait for ready bit to become set
    while (!*((uint32_t*)(pci_device->mmio_virt_base + 0x1C))) {
        task_yield();
    };

    // TODO: Read CAP.DSTRD?
    // uint8_t dstrd_exponent = *((uint32_t*)(pci_device->mmio_virt_base + 0x0)) & 0xF;

    uint32_t version = *((uint32_t*)(pci_device->mmio_virt_base + 0x8));

    nvme_device->pci_device = pci_device;
    nvme_device->major_version = version >> 16;
    nvme_device->minor_version = version >> 8;
    nvme_device->dstrd_exponent = 0; // TODO: read CAP.DSTRD
    nvme_device->asq = admin_submission_queue;
    nvme_device->asq_tail = 0;
    nvme_device->asq_head = 0;
    nvme_device->acq = admin_completion_queue;
    nvme_device->acq_head = 0;
    nvme_device->acq_phase = 0;
    nvme_device->iosq = io_submission_queue;
    nvme_device->iosq_tail = 0;
    nvme_device->iosq_head = 0;
    nvme_device->iocq = io_completion_queue;
    nvme_device->iocq_head = 0;
    nvme_device->iocq_phase = 0;
    nvme_device->admin_result_buffer = admin_result_buffer;
    memset(nvme_device->command_statuses, NVME_COMMAND_FREE, NVME_MAX_COMMANDS);

    nvme_probe_contents(nvme_device);
}

// Called in task context
void nvme_probe_contents(struct nvme_device *dev) {
    // If ASQ is full, wait until there is an empty slot
    while ((dev->asq_tail + 1) % NVME_QUEUE_SIZE == dev->asq_head) {
        // task_yield();
    }
    
    // Enqueue 'identify controller' command
    uint16_t command_id = start_command(dev);

    uint64_t admin_result_buffer_phys = (uint64_t)(dev->admin_result_buffer - hhdm_offset);

    struct nvme_sq_entry* tail_asq_entry = dev->asq + 64 * dev->asq_tail;
    tail_asq_entry->command = 0x06 | (command_id << 16); // identify
    tail_asq_entry->nsid = 0;
    tail_asq_entry->reserved1 = 0;
    tail_asq_entry->reserved2 = 0;
    tail_asq_entry->metadata1 = 0;
    tail_asq_entry->metadata2 = 0;
    tail_asq_entry->data1 = admin_result_buffer_phys;
    tail_asq_entry->data2 = admin_result_buffer_phys >> 32;
    tail_asq_entry->data3 = 0;
    tail_asq_entry->data4 = 0;
    tail_asq_entry->suffix1 = 0x1; // identify controller
    tail_asq_entry->suffix2 = 0;
    tail_asq_entry->suffix3 = 0;
    tail_asq_entry->suffix4 = 0;
    tail_asq_entry->suffix5 = 0;
    tail_asq_entry->suffix6 = 0;
    dev->asq_tail = (dev->asq_tail + 1) % NVME_QUEUE_SIZE;

    uint8_t acq_head = dev->acq_head;

    // Write Submission Queue 0 Tail Doorbell
    *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x1000)) = dev->asq_tail;

    await_command_finish(dev, command_id);
    free_command(dev, command_id);
    
    // Verify the identify controller command was successful
    uint16_t identify_command_status = ((struct nvme_cq_entry*)(dev->acq))[acq_head].status_field >> 1;
    if (identify_command_status != 0) {
        printk(u8p("Identify controller command error: "));
        printk_uint16(identify_command_status);
        printk(u8p("\n"));
        return;
    }

    // Verify that this is an I/O controller
    uint8_t cntrltype = *((uint8_t*)(dev->admin_result_buffer) + 111);
    if (cntrltype > 1) {
        printk(u8p("Bad controller type: "));
        printk_uint8(cntrltype);
        printk(u8p("\n"));
        return;
    }

    // Process Identify Controller data structure
    uint8_t sqes_byte = *((uint8_t*)(dev->admin_result_buffer) + 512);
    uint8_t max_sqes_exponent = sqes_byte >> 4;
    (void) max_sqes_exponent;
    uint8_t min_sqes_exponent = sqes_byte & 0xF;
    uint8_t sqes_exponent = min_sqes_exponent; // Choose min length
    
    uint8_t cqes_byte = *((uint8_t*)(dev->admin_result_buffer) + 513);
    uint8_t max_cqes_exponent = cqes_byte >> 4;
    (void) max_cqes_exponent;
    uint8_t min_cqes_exponent = cqes_byte & 0xF;
    uint8_t cqes_exponent = min_cqes_exponent; // Choose min length

    // Write cqes_exponent and sqes_exponent to CC.IOCQES, CC.IOSQES
    uint32_t cc = *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x14));
    cc &= 0xFF00FFFF;
    cc |= ((uint32_t)cqes_exponent << 20);
    cc |= ((uint32_t)sqes_exponent << 16);
    *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x14)) = cc;

    // Enqueue 'identify namespace' command
    command_id = start_command(dev);
    tail_asq_entry = dev->asq + 64 * dev->asq_tail;
    tail_asq_entry->command = 0x06 | (command_id << 16); // identify
    tail_asq_entry->nsid = 1;
    tail_asq_entry->reserved1 = 0;
    tail_asq_entry->reserved2 = 0;
    tail_asq_entry->metadata1 = 0;
    tail_asq_entry->metadata2 = 0;
    tail_asq_entry->data1 = admin_result_buffer_phys;
    tail_asq_entry->data2 = admin_result_buffer_phys >> 32;
    tail_asq_entry->data3 = 0;
    tail_asq_entry->data4 = 0;
    tail_asq_entry->suffix1 = 0x0; // identify namespace
    tail_asq_entry->suffix2 = 0;
    tail_asq_entry->suffix3 = 0;
    tail_asq_entry->suffix4 = 0;
    tail_asq_entry->suffix5 = 0;
    tail_asq_entry->suffix6 = 0;
    dev->asq_tail = (dev->asq_tail + 1) % NVME_QUEUE_SIZE;

    acq_head = dev->acq_head;

    // Write Submission Queue 0 Tail Doorbell
    *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x1000)) = dev->asq_tail;

    await_command_finish(dev, command_id);
    free_command(dev, command_id);
    
    // Verify the identify namespace command was successful
    uint16_t identify_ns_command_status = ((struct nvme_cq_entry*)(dev->acq))[acq_head].status_field >> 1;
    if (identify_ns_command_status != 0) {
        printk(u8p("Identify namespace command error: "));
        printk_uint16(identify_ns_command_status);
        printk(u8p("\n"));
        return;
    }
    
    // Read namespace size
    uint64_t ins_nsze = *((uint64_t*)(dev->admin_result_buffer) + 0);
    dev->lba_count = ins_nsze;
    // Read LBA exponent
    uint8_t ins_flbas = *((uint8_t*)(dev->admin_result_buffer) + 26) & 0xF;
    uint32_t ins_lbaf = *((uint32_t*)((uint8_t*)(dev->admin_result_buffer) + 128 + 4 * ins_flbas));
    dev->lbads_exponent = (ins_lbaf >> 16) & 0xFF;
    // Read metadata size
    dev->metadata_size = (ins_lbaf & 0xFFFF);

    // If ASQ is full, wait until there is an empty slot
    while ((dev->asq_tail + 1) % NVME_QUEUE_SIZE == dev->asq_head) {
        task_yield();
    }

    uint64_t iocq_phys = (uint64_t)(dev->iocq - hhdm_offset);

    // Enqueue 'create completion queue' command
    command_id = start_command(dev);
    tail_asq_entry = dev->asq + 64 * dev->asq_tail;
    tail_asq_entry->command = 0x05 | (command_id << 16); // create completion queue
    tail_asq_entry->nsid = 0;
    tail_asq_entry->reserved1 = 0;
    tail_asq_entry->reserved2 = 0;
    tail_asq_entry->metadata1 = 0;
    tail_asq_entry->metadata2 = 0;
    tail_asq_entry->data1 = iocq_phys;
    tail_asq_entry->data2 = iocq_phys >> 32;
    tail_asq_entry->data3 = 0;
    tail_asq_entry->data4 = 0;
    tail_asq_entry->suffix1 = (NVME_QUEUE_SIZE_MINUS_ONE << 16) | NVME_IOCQ_ID;
    tail_asq_entry->suffix2 = 0x3;
    tail_asq_entry->suffix3 = 0;
    tail_asq_entry->suffix4 = 0;
    tail_asq_entry->suffix5 = 0;
    tail_asq_entry->suffix6 = 0;
    dev->asq_tail = (dev->asq_tail + 1) % NVME_QUEUE_SIZE;
    
    acq_head = dev->acq_head;

    // Write Submission Queue 0 Tail Doorbell
    *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x1000)) = dev->asq_tail;

    await_command_finish(dev, command_id);
    free_command(dev, command_id);

    // Verify the CreateIocq command was successful
    uint16_t create_iocq_command_status = ((struct nvme_cq_entry*)(dev->acq))[acq_head].status_field >> 1;
    if (create_iocq_command_status != 0) {
        printk(u8p("Create IOCQ command error: "));
        printk_uint16(create_iocq_command_status);
        printk(u8p("\n"));
        return;
    }

    // If ASQ is full, wait until there is an empty slot
    while ((dev->asq_tail + 1) % NVME_QUEUE_SIZE == dev->asq_head) {
        task_yield();
    }

    uint64_t iosq_phys = (uint64_t)(dev->iosq - hhdm_offset);

    // Enqueue 'create submission queue' command
    command_id = start_command(dev);
    tail_asq_entry = dev->asq + 64 * dev->asq_tail;
    tail_asq_entry->command = 0x01 | (command_id << 16); // create submission queue
    tail_asq_entry->nsid = 0;
    tail_asq_entry->reserved1 = 0;
    tail_asq_entry->reserved2 = 0;
    tail_asq_entry->metadata1 = 0;
    tail_asq_entry->metadata2 = 0;
    tail_asq_entry->data1 = iosq_phys;
    tail_asq_entry->data2 = iosq_phys >> 32;
    tail_asq_entry->data3 = 0;
    tail_asq_entry->data4 = 0;
    tail_asq_entry->suffix1 = (NVME_QUEUE_SIZE_MINUS_ONE << 16) | NVME_IOSQ_ID;
    tail_asq_entry->suffix2 = (NVME_IOCQ_ID << 16) | 0x1;
    tail_asq_entry->suffix3 = 0;
    tail_asq_entry->suffix4 = 0;
    tail_asq_entry->suffix5 = 0;
    tail_asq_entry->suffix6 = 0;
    dev->asq_tail = (dev->asq_tail + 1) % NVME_QUEUE_SIZE;
    
    acq_head = dev->acq_head;

    // Write Submission Queue 0 Tail Doorbell
    *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x1000)) = dev->asq_tail;

    await_command_finish(dev, command_id);
    free_command(dev, command_id);

    // Verify the CreateIocq command was successful
    uint16_t create_iosq_command_status = ((struct nvme_cq_entry*)(dev->acq))[acq_head].status_field >> 1;
    if (create_iosq_command_status != 0) {
        printk(u8p("Create IOSQ command error: "));
        printk_uint16(create_iosq_command_status);
        printk(u8p("\n"));
        return;
    }
    // Find partitions on device
    nvmepart_probe(dev);
}

void nvme_handle_interrupt(uint8_t interrupt_line) {
    (void) interrupt_line;
    // printk("interrupt 0x");
    // printk_uint8(interrupt_line);
    // printk("\n");

    for (int i = 0; i < num_nvme_devices; i++) {
        struct nvme_device *dev = &nvme_devices[i];
        // if (dev->pci_device->interrupt_line != interrupt_line) {
        //   // Interrupt couldn't have come from this device. Nothing to do
        //   continue;
        // }
    
        // Interrupt could have come from this device
        
        // Check if any unacknowledged entries in the admin completion queue
        bool acq_doorbell_write_needed = false;
        while(true) {
            struct nvme_cq_entry* head_acq_entry = (struct nvme_cq_entry*)(dev->acq) + dev->acq_head;
            bool head_acq_entry_phase = head_acq_entry->status_field & 0x1;
            if (head_acq_entry_phase == dev->acq_phase) {
                break;
            }
        
            // TODO: copy the whole acq somewhere, otherwise it could be overwritten
            finish_command(dev, head_acq_entry->command_identifier);
            dev->acq_head += 1;
            dev->asq_head = head_acq_entry->sq_head_pointer;
            acq_doorbell_write_needed = true;
            if (dev->acq_head == NVME_QUEUE_SIZE) {
                // Wrap around
                dev->acq_head = 0;
                dev->acq_phase = !dev->acq_phase;
            }
        }

        // Write Completion Queue 0 Head Doorbell
        if (acq_doorbell_write_needed) {
            *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x1000 + 1 * (4 << dev->dstrd_exponent))) = dev->acq_head;
        }

        // Check if any unacknowledged entries in the IO completion queue
        bool iocq_doorbell_write_needed = false;
        while(true) {
            struct nvme_cq_entry* head_iocq_entry = (struct nvme_cq_entry*)(dev->iocq) + dev->iocq_head;
            bool head_iocq_entry_phase = head_iocq_entry->status_field & 0x1;
            if (head_iocq_entry_phase == dev->iocq_phase) {
                break;
            }

            // TODO: copy the whole iocq somewhere, otherwise it could be overwritten
            finish_command(dev, head_iocq_entry->command_identifier);
            dev->iocq_head += 1;
            dev->iosq_head = head_iocq_entry->sq_head_pointer;
            iocq_doorbell_write_needed = true;
            if (dev->iocq_head == NVME_QUEUE_SIZE) {
                // Wrap around
                dev->iocq_head = 0;
                dev->iocq_phase = !dev->iocq_phase;
            }
        }

        // Write Completion Queue 1 Head Doorbell
        if (iocq_doorbell_write_needed) {
            *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x1000 + (3 * (4 << dev->dstrd_exponent)))) = dev->iocq_head;
        }
    }
}

// Must be called from task context
void nvme_readpage(struct nvme_device *dev, uint32_t lba, void* result_page) {
    // If IOSQ is full, wait until there is an empty slot
    while ((dev->iosq_tail + 1) % NVME_QUEUE_SIZE == dev->iosq_head) {
        task_yield();
    }
  
    struct nvme_sq_entry *tail_iosq_entry;
    uint64_t result_page_phys = (uint64_t)(result_page - hhdm_offset);
  
    // Enqueue 'read' command
    uint16_t command_id = start_command(dev);
    tail_iosq_entry = dev->iosq + 64 * dev->iosq_tail;
    tail_iosq_entry->command = 0x02 | (command_id << 16); // read
    tail_iosq_entry->nsid = 1;
    tail_iosq_entry->reserved1 = 0;
    tail_iosq_entry->reserved2 = 0;
    tail_iosq_entry->metadata1 = 0;
    tail_iosq_entry->metadata2 = 0;
    tail_iosq_entry->data1 = result_page_phys;
    tail_iosq_entry->data2 = result_page_phys >> 32;
    tail_iosq_entry->data3 = 0;
    tail_iosq_entry->data4 = 0;
    tail_iosq_entry->suffix1 = lba;
    tail_iosq_entry->suffix2 = 0; // Upper 32 bits of LBA
    tail_iosq_entry->suffix3 = 4095 >> (dev->lbads_exponent); // LR, FUA, PRINFO, NLB
    tail_iosq_entry->suffix4 = 0; // Dataset management
    tail_iosq_entry->suffix5 = 0; // EILBRT
    tail_iosq_entry->suffix6 = 0; // ELBATM
    dev->iosq_tail = (dev->iosq_tail + 1) % NVME_QUEUE_SIZE;
  
    // Write Submission Queue 1 Tail Doorbell
    uint8_t iocq_head = dev->iocq_head;
    *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x1000 + (2 * (4 << dev->dstrd_exponent)))) = dev->iosq_tail;
  
    await_command_finish(dev, command_id);
    free_command(dev, command_id);
  
    // Verify the read command was successful
    uint16_t read_command_status = ((struct nvme_cq_entry*)(dev->iocq))[iocq_head].status_field >> 1;
    if (read_command_status != 0) {
        printk(u8p("Read command error: "));
        printk_uint16(read_command_status);
        printk(u8p("\n"));
        return;
    }
}

ssize_t nvme_read(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    struct nvme_device *nvme_device = dev;
    void *temp_page = kpage_alloc(1);
  
    // Avoid overflow
    int64_t bytes_readable = (nvme_device->lba_count << nvme_device->lbads_exponent) - offset;
    if (bytes_readable < 0) {
        bytes_readable = 0;
    }
    if ((int64_t)length > bytes_readable) {
         length = bytes_readable;
    }
  
    size_t bytes_read = 0;
    while (bytes_read < length) {
        size_t bytes_remaining = length - bytes_read;
        uint32_t lba = offset >> 12 << (12 - nvme_device->lbads_exponent);
        nvme_readpage(nvme_device, lba, temp_page);
        
        size_t chunk_page_offset = offset % 4096;
        size_t chunk_length = 4096 - chunk_page_offset;
        if (chunk_length > bytes_remaining) {
            chunk_length = bytes_remaining;
        }
    
        memcpy(buffer, temp_page + chunk_page_offset, chunk_length);
    
        buffer += chunk_length;
        offset += chunk_length;
        bytes_read += chunk_length;
    }
    kpage_free(temp_page, 1);
    return bytes_read;
}

void nvme_writepage(struct nvme_device *dev, uint32_t lba, void* content_page) {
    // If IOSQ is full, wait until there is an empty slot
    while ((dev->iosq_tail + 1) % NVME_QUEUE_SIZE == dev->iosq_head) {
        task_yield();
    }
  
    struct nvme_sq_entry *tail_iosq_entry;
    uint64_t result_page_phys = (uint64_t)(content_page - hhdm_offset);
  
    // Enqueue 'read' command
    uint16_t command_id = start_command(dev);
    tail_iosq_entry = dev->iosq + 64 * dev->iosq_tail;
    tail_iosq_entry->command = 0x01 | (command_id << 16); // write
    tail_iosq_entry->nsid = 1;
    tail_iosq_entry->reserved1 = 0;
    tail_iosq_entry->reserved2 = 0;
    tail_iosq_entry->metadata1 = 0;
    tail_iosq_entry->metadata2 = 0;
    tail_iosq_entry->data1 = result_page_phys;
    tail_iosq_entry->data2 = result_page_phys >> 32;
    tail_iosq_entry->data3 = 0;
    tail_iosq_entry->data4 = 0;
    tail_iosq_entry->suffix1 = lba;
    tail_iosq_entry->suffix2 = 0; // Upper 32 bits of LBA
    tail_iosq_entry->suffix3 = 4095 >> (dev->lbads_exponent); // LR, FUA, PRINFO, NLB
    tail_iosq_entry->suffix4 = 0; // Dataset management
    tail_iosq_entry->suffix5 = 0; // EILBRT
    tail_iosq_entry->suffix6 = 0; // ELBATM
    dev->iosq_tail = (dev->iosq_tail + 1) % NVME_QUEUE_SIZE;
  
    // Write Submission Queue 1 Tail Doorbell
    uint8_t iocq_head = dev->iocq_head;
    *((uint32_t*)(dev->pci_device->mmio_virt_base + 0x1000 + (2 * (4 << dev->dstrd_exponent)))) = dev->iosq_tail;
  
    await_command_finish(dev, command_id);
    free_command(dev, command_id);
  
    // Verify the write command was successful
    uint16_t write_command_status = ((struct nvme_cq_entry*)(dev->iocq))[iocq_head].status_field >> 1;
    if (write_command_status != 0) {
        printk("Write command error: ");
        printk_uint16(write_command_status);
        printk("\n");
        return;
    }
}

ssize_t nvme_write(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    struct nvme_device *nvme_device = dev;
    void *temp_page = kpage_alloc(1);

    // Avoid overflow
    int64_t bytes_writeable = (nvme_device->lba_count << nvme_device->lbads_exponent) - offset;
    if (bytes_writeable < 0) {
        bytes_writeable = 0;
    }
    if (length > bytes_writeable) {
        length = bytes_writeable;
    }

    size_t bytes_written = 0;
    while (bytes_written < length) {
        size_t bytes_remaining = length - bytes_written;
        uint32_t lba = offset >> 12 << (12 - nvme_device->lbads_exponent);
        nvme_readpage(nvme_device, lba, temp_page);
        
        size_t chunk_page_offset = offset % 4096;
        size_t chunk_length = 4096 - chunk_page_offset;
        if (chunk_length > bytes_remaining) {
        chunk_length = bytes_remaining;
        }

        memcpy(temp_page + chunk_page_offset, buffer, chunk_length);
        nvme_writepage(nvme_device, lba, temp_page);

        buffer += chunk_length;
        offset += chunk_length;
        bytes_written += chunk_length;
    }
    kpage_free(temp_page, 1);
    return bytes_written;
}

struct file_operations nvme_device_fops = {
    .read = nvme_read,
    .write = nvme_write,
};
