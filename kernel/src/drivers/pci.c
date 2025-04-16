#include <stdint.h>
#include "pci.h"
#include "arch/asm.h"
#include "drivers/nvme.h"
#include "drivers/tty.h"
#include "mm/kmem.h"
#include "mm/map.h"

struct pci_device pci_devices[MAX_PCI_DEVICES];
uint16_t num_pci_devices = 0;

// Offset must be multiple of 4
uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
                (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    // Write out the address
    outl(0xCF8, address);
    // Read in the data
    return inl(0xCFC);
}

// Offset must be multiple of 4
void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
                (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    // Write out the address
    outl(0xCF8, address);
    // write out the data
    return outl(0xCFC, value);
}

void pci_probe() {
    for (uint16_t bus = 0; bus < 255; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t function = 0; function < 8; function++) {
                uint32_t dword_0 = pci_config_read_dword(bus, slot, function, 0);
                uint16_t vendor_id = dword_0;
                uint16_t device_id = dword_0 >> 16;
                if (vendor_id != 0xFFFF) {
                    uint32_t dword_8 = pci_config_read_dword(bus, slot, function, 8);
                    uint8_t class_code = dword_8 >> 24;
                    uint8_t subclass = dword_8 >> 16;
                    uint8_t prog_if = dword_8 >> 8;
                    // Raw value read from register
                    uint32_t bar0_readout = pci_config_read_dword(bus, slot, function, 16);
                    uint32_t bar1_readout = pci_config_read_dword(bus, slot, function, 20);
                    // PCI base address
                    uint8_t bar_type = (bar0_readout & 0x7) >> 1;
                    uint64_t mmio_phys_base;
                    switch (bar_type) {
                        case 0:
                            mmio_phys_base = bar0_readout & 0xFFFFFFF0;
                            break;
                        case 2:
                            mmio_phys_base = (((uint64_t)bar1_readout) << 32) + (bar0_readout & 0xFFFFFFF0);
                            break;
                        default:
                            printk(u8p("Unsupported BAR type\n"));
                            continue; 
                    }

                    uint32_t mmio_size = 0;

                    if (mmio_phys_base) {
                        // Write all ones
                        pci_config_write_dword(bus, slot, function, 16, 0xFFFFFFFF);
                        uint32_t bar0_negative_mask = pci_config_read_dword(bus, slot, function, 16) & 0xFFFFF000;
                        // Restore original value
                        pci_config_write_dword(bus, slot, function, 16, bar0_readout);
                        mmio_size = ~bar0_negative_mask + 1;
                    }
                    uint32_t dword_3C = pci_config_read_dword(bus, slot, function, 0x3C);

                    struct pci_device *dev = &pci_devices[num_pci_devices];
                    
                    dev->bus = bus;
                    dev->slot = slot;
                    dev->function = function;
                    
                    dev->vendor_id = vendor_id;
                    dev->device_id = device_id;

                    dev->class_code = class_code;
                    dev->subclass = subclass;
                    dev->prog_if = prog_if;
                    
                    dev->mmio_phys_base = mmio_phys_base;
                    dev->mmio_size = mmio_size;
                    dev->mmio_virt_base = 0;
                    dev->interrupt_line = dword_3C;
                    
                    num_pci_devices++;

                    if (dev->class_code == 1 && dev->subclass == 8) {
                        // NVME device
                        nvme_probe_1(dev);
                    }
                    if (num_pci_devices == MAX_PCI_DEVICES) {
                        return;
                    }
                }
            }
        }
    }
}
