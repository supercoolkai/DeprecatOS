#include "drivers/disk/pci/pci.h"
#include "portio/portio.h"
#include "hex/hexPrinter.h"
#include "colors/colors.h"
#include "kprintf/kprintf.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint8_t bus_n;
  uint8_t device;
  uint8_t fn;
} pci_triple;

static uint32_t read_pci_config(uint8_t bus_n, uint8_t device, uint8_t fn, uint8_t offset)
{
  uint32_t dword = PCI_ENABLE_BIT |
    (bus_n << 16) |
    ((device & ((1 << 5) - 1)) << 11) | 
    ((fn & ((1 << 3) - 1)) << 8) |
    (offset & ~((1 << 2) - 1));
  
  outl(PCI_IO_PORT, dword);

  return inl(PCI_DATA_PORT);
}

static uint8_t read_header_type(uint8_t bus_n, uint8_t device, uint8_t fn)
{
  return (uint8_t)(read_pci_config(bus_n, device, fn, PCI_HEADER_TYPE_BYTE_OFFSET) >> (8 * PCI_HEADER_TYPE_BYTE));
}

static pci_triple pci_find_by_class(uint8_t bus_n, uint8_t class, uint8_t subclass, bool *found)
{
  for (uint8_t device = 0; device < PCI_DEVICES_PER_BUS; device++) {
    uint16_t vendor_check = (uint16_t) read_pci_config(bus_n, device, 0, PCI_VENDOR_CHECK_OFFSET);

    if (vendor_check == PCI_NONEXISTENT_FUNCTION)
      continue;

    uint8_t fn_count = (read_header_type(bus_n, device, 0) & PCI_DEVICE_MULTIFUNCTION_BIT) ? PCI_FUNCTIONS_PER_DEVICE : 1;

    for (uint8_t fn = 0; fn < fn_count; fn++) {
      vendor_check = (uint16_t) read_pci_config(bus_n, device, fn, PCI_VENDOR_CHECK_OFFSET);

      if (vendor_check == PCI_NONEXISTENT_FUNCTION)
        continue;

      uint32_t test_dword = read_pci_config(bus_n, device, fn, PCI_CLASS_OFFSET);

      if ((uint8_t)(test_dword >> (8 * PCI_CLASS_BYTE)) == class && (uint8_t)(test_dword >> (8 * PCI_SUBCLASS_BYTE)) == subclass) {
        pci_triple return_triple;
        return_triple.bus_n = bus_n;
        return_triple.device = device;
        return_triple.fn = fn;

        *found = true;

        return return_triple;
      }

      uint8_t header_byte = read_header_type(bus_n, device, fn);

      if ((header_byte & ~PCI_DEVICE_MULTIFUNCTION_BIT) == PCI_HEADER_TYPE_BRIDGE) {
        uint8_t secondary_bus_n = (uint8_t)(read_pci_config(bus_n, device, fn, PCI_SECONDARY_BUS_N_OFFSET) >> (8 * PCI_SECONDARY_BUS_N_BYTE));
        pci_triple bridged_triple = pci_find_by_class(secondary_bus_n, class, subclass, found);

        if (*found)
          return bridged_triple;
      }
    }
  }

  *found = false;
  return (pci_triple) {0,0,0};
}

void pci_get_bases(uint16_t *primary_command_out, uint16_t *primary_control_out, uint16_t *secondary_command_out, uint16_t *secondary_control_out)
{
  bool found;
  pci_triple triple = pci_find_by_class(0, 0x01, 0x01, &found);

  if (!found) {
    *primary_command_out = LEGACY_PRIMARY_COMMAND;
    *primary_control_out = LEGACY_PRIMARY_CONTROL;
    *secondary_command_out = LEGACY_SECONDARY_COMMAND;
    *secondary_control_out = LEGACY_SECONDARY_CONTROL;

    return;
  }

  uint8_t prog_if = (uint8_t)(read_pci_config(triple.bus_n, triple.device, triple.fn, PCI_CLASS_OFFSET) >> (8 * PCI_PROG_IF_BYTE));

  if (!(prog_if & PCI_PRIMARY_NATIVE_BIT)){
    *primary_command_out = LEGACY_PRIMARY_COMMAND;
    *primary_control_out = LEGACY_PRIMARY_CONTROL;
  }
  else{
    *primary_command_out = (uint16_t)(read_pci_config(triple.bus_n, triple.device, triple.fn, PCI_PRIMARY_COMMAND_BAR) & ~((1 << 2) - 1));
    *primary_control_out = (uint16_t)(read_pci_config(triple.bus_n, triple.device, triple.fn, PCI_PRIMARY_CONTROL_BAR) & ~((1 << 2) - 1));
  }

  if (!(prog_if & PCI_SECONDARY_NATIVE_BIT)){
    *secondary_command_out = LEGACY_SECONDARY_COMMAND;
    *secondary_control_out = LEGACY_SECONDARY_CONTROL;
  }
  else{
    *secondary_command_out = (uint16_t)(read_pci_config(triple.bus_n, triple.device, triple.fn, PCI_SECONDARY_COMMAND_BAR) & ~((1 << 2) - 1));
    *secondary_control_out = (uint16_t)(read_pci_config(triple.bus_n, triple.device, triple.fn, PCI_SECONDARY_CONTROL_BAR) & ~((1 << 2) - 1));
  }
}
