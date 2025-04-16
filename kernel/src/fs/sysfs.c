#include <stdbool.h>
#include <stddef.h>
#include "sysfs.h"
#include "arch/asm.h"
#include "drivers/pci.h"
#include "drivers/nvme.h"
#include "drivers/tty.h"
#include "lib/cstd.h"
#include "mm/kmem.h"
#include "mm/slab.h"

#define SYSFS_MOUNT_NOT_IMPLEMENTED 3

struct superblock sysfs_superblock;
struct inode sysfs_root_inode;
struct dentry sysfs_pciinfo_dentry;
struct inode sysfs_pciinfo_inode;
struct dentry sysfs_meminfo_dentry;
struct inode sysfs_meminfo_inode;
struct dentry sysfs_nvme_dentry;
struct inode sysfs_nvme_inode;

ssize_t sysfs_mount(struct inode *device_inode, struct dentry *mountpoint_dentry) {
    (void) device_inode;
    mountpoint_dentry->mounted_inode = &sysfs_root_inode;
    return 0;
}

void sysfs_init() {
    sysfs_superblock.device = NULL;
    sysfs_superblock.device_fops = NULL;
    sysfs_superblock.ops = &sysfs_superblock_ops;
    sysfs_superblock.private = NULL;

    sysfs_root_inode.type = INODE_DIRECTORY;
    sysfs_root_inode.superblock = &sysfs_superblock;
    init_list(&sysfs_root_inode.dentry_lh);
    sysfs_root_inode.private = NULL;

    sysfs_pciinfo_inode.type = INODE_REGULAR_FILE;
    sysfs_pciinfo_inode.file_length = 10;
    sysfs_pciinfo_inode.superblock = &sysfs_superblock;

    strcpy(sysfs_pciinfo_dentry.name, u8p("pciinfo"));
    list_add_tail(&sysfs_pciinfo_dentry.dentry_le, &sysfs_root_inode.dentry_lh);
    sysfs_pciinfo_dentry.inode = &sysfs_pciinfo_inode;

    sysfs_meminfo_inode.type = INODE_REGULAR_FILE;
    sysfs_meminfo_inode.file_length = 10;
    sysfs_meminfo_inode.superblock = &sysfs_superblock;

    strcpy(sysfs_meminfo_dentry.name, u8p("meminfo"));
    list_add_tail(&sysfs_meminfo_dentry.dentry_le, &sysfs_root_inode.dentry_lh);
    sysfs_meminfo_dentry.inode = &sysfs_meminfo_inode;

    sysfs_nvme_inode.type = INODE_REGULAR_FILE;
    sysfs_nvme_inode.file_length = 10;
    sysfs_nvme_inode.superblock = &sysfs_superblock;

    strcpy(sysfs_nvme_dentry.name, u8p("nvme"));
    list_add_tail(&sysfs_nvme_dentry.dentry_le, &sysfs_root_inode.dentry_lh);
    sysfs_nvme_dentry.inode = &sysfs_nvme_inode;

    struct vfs_lookup_result sys_resolve_result;
    vfs_resolve(u8p("sys"), &sys_resolve_result);
    if (sys_resolve_result.status != VFS_RESOLVE_SUCCESS_EXISTS) {
        halt_forever();
    }
    sysfs_mount(NULL, sys_resolve_result.dentry);
}

void sysfs_lookup(struct inode *file_inode, struct dentry *dentry) {
    (void) file_inode;
    (void) dentry;
}

ssize_t sysfs_create_file_inode(
    struct inode *parent_dir,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    (void) parent_dir;
    (void) out_inode;
    (void) out_dentry;
    return -1;
}

// out_dentry should have initialized name
ssize_t sysfs_create_dir_inode(
    struct inode *parent_dir,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    (void) parent_dir;
    (void) out_inode;
    (void) out_dentry;
    return -1;
}

// out_dentry should have initialized name
ssize_t sysfs_create_dev_inode(
    struct inode *parent_dir,
    uint16_t device_type,
    uint16_t device_number,
    struct file_operations *device_fops,
    void *device,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    (void) parent_dir;
    (void) device_type;
    (void) device_number;
    (void) device_fops;
    (void) device;
    (void) out_inode;
    (void) out_dentry;
    return -1;
}

// Mutates destination and destination_length
void safe_copy_pal(
    void **destination,
    size_t* destination_length,
    uint8_t *source,
    size_t source_length
) {
    size_t bytes_to_copy = *destination_length < source_length ? *destination_length : source_length;
    memcpy(*destination, source, bytes_to_copy);
    *destination += bytes_to_copy;
    *destination_length -= bytes_to_copy;
}

// Mutates destination and destination_length
void safe_copy_string(
    void **destination,
    size_t* destination_length,
    uint8_t *source
) {
    safe_copy_pal(destination, destination_length, source, strlen(source));
}

ssize_t sysfs_read(struct file *filp, void *buffer, size_t length) {
    if (filp->offset > 0) {
        // Don't support non-zero ofset for simplicity
        return 0;
    }

    void *destination = buffer;
    size_t destination_length = length;

    if(filp->inode == &sysfs_pciinfo_inode) {
        for (int i = 0; i < num_pci_devices; i++) {
            struct pci_device *dev = &pci_devices[i];
            uint8_t num_string_buffer[17];
            safe_copy_string(&destination, &destination_length, u8p("Pci("));
            sprintf_uint8(dev->bus, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(":"));
            sprintf_uint8(dev->slot, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p("."));
            sprintf_uint8(dev->function, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(" d="));
            sprintf_uint16(dev->vendor_id, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(":"));
            sprintf_uint16(dev->device_id, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(" c="));
            sprintf_uint8(dev->class_code, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(":"));
            sprintf_uint8(dev->subclass, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(":"));
            sprintf_uint8(dev->prog_if, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(" mb="));
            sprintf_uint64(dev->mmio_phys_base, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(" ms="));
            sprintf_uint32(dev->mmio_size, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(" bb="));
            sprintf_uint64(dev->mmio_virt_base, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(" il="));
            sprintf_uint8(dev->interrupt_line, num_string_buffer);
            safe_copy_string(&destination, &destination_length, num_string_buffer);
            safe_copy_string(&destination, &destination_length, u8p(")\n"));
        }
    } else if(filp->inode == &sysfs_nvme_inode) {
        for (int i = 0; i < num_nvme_devices; i++) {
            struct nvme_device *dev = &nvme_devices[i];
			safe_copy_string(&destination, &destination_length, u8p("Nvme("));
			uint8_t num_string_buffer[17];
			sprintf_uint8(dev->pci_device->bus, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(":"));
			sprintf_uint8(dev->pci_device->slot, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p("."));
			sprintf_uint8(dev->pci_device->function, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" addr="));
			sprintf_uint32(dev->pci_device->mmio_phys_base, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" ver="));
			sprintf_uint8(dev->major_version, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p("."));
			sprintf_uint8(dev->minor_version, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" asq_tail="));
			sprintf_uint8(dev->asq_tail, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" asq_head="));
			sprintf_uint8(dev->asq_head, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" acq_head="));
			sprintf_uint8(dev->acq_head, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" iosq_tail="));
			sprintf_uint8(dev->iosq_tail, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" iosq_head="));
			sprintf_uint8(dev->iosq_head, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" iocq_head="));
			sprintf_uint8(dev->iocq_head, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" lba_count="));
			sprintf_uint64(dev->lba_count, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(" lbads_exp="));
			sprintf_uint8(dev->lbads_exponent, num_string_buffer);
			safe_copy_string(&destination, &destination_length, num_string_buffer);
			safe_copy_string(&destination, &destination_length, u8p(")\n"));
        }
    } else if (filp->inode == &sysfs_meminfo_inode) {
        uint8_t num_string_buffer[11];
        safe_copy_string(&destination, &destination_length, u8p("total_memory_kib = "));
        sprintf_dec(kmem_total_pages * 4, num_string_buffer, 0, 0);
        safe_copy_string(&destination, &destination_length, num_string_buffer);
        safe_copy_string(&destination, &destination_length, u8p("\nused_memory_kib = "));
        sprintf_dec(kmem_used_pages * 4, num_string_buffer, 0, 0);
        safe_copy_string(&destination, &destination_length, num_string_buffer);
        safe_copy_string(&destination, &destination_length, u8p("\n"));
    } else {
        panic(u8p("Unknown sysfs inode"));
    }

    filp->offset = destination - buffer;
    return filp->offset;
}

ssize_t sysfs_write(struct file *filp, void *buf, size_t length) {
    (void) filp;
    (void) buf;
    (void) length;
    return -1;
}

ssize_t sysfs_set_size(struct file *filp, size_t size) {
    (void) filp;
    (void) size;
    return -1;
}

struct filesystem_ops sysfs_superblock_ops = {
    .mount = sysfs_mount,
    .lookup = sysfs_lookup,
    .create_file_inode = sysfs_create_file_inode,
    .create_dir_inode = sysfs_create_dir_inode,
    .create_dev_inode = sysfs_create_dev_inode,
    .read = sysfs_read,
    .write = sysfs_write,
    .set_size = sysfs_set_size,
};
