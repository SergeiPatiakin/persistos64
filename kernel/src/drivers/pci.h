#ifndef PCI_H
#define PCI_H
#include <stdint.h>

struct pci_device {
    uint8_t bus;
    uint8_t slot;
    uint8_t function;

    uint16_t vendor_id;
    uint16_t device_id;
    
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;

    
    uint64_t mmio_phys_base; // Physical
    uint32_t mmio_size;
    uint64_t mmio_virt_base; // Virtual
    
    uint8_t interrupt_line;
};

void pci_probe();
uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);

#define MAX_PCI_DEVICES 1024
extern struct pci_device pci_devices[MAX_PCI_DEVICES];
extern uint16_t num_pci_devices;

#endif
