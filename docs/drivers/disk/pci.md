# PCI module

## pci_triple struct format
src `kernel/drivers/disk/pci/pci.c`

| offset | field | type | 
| --- | --- | --- |
| 0 | bus_n | `uint8_t` | 
| 1 | device |`uint8_t` | 
| 2 | fn | `uint8_t` |

## important ports
based off the `#define`s in `kernel/drivers/disk/pci/pci.h`

| port | port name | read or write |
| --- | --- | --- |
| `0xCF8` | config address port | write |
| `0xCFC` | config data port | read |

---

## legacy ATA bases
based off the `#define`s in `kernel/drivers/disk/pci/pci.h`. handed to the ata driver by `pci_get_bases()` when no IDE controller is found or when the channel's native bit in prog if is clear

| port | base name |
| --- | --- |
| `0x1F0` | primary command base |
| `0x3F4` | primary control base |
| `0x170` | secondary command base |
| `0x374` | secondary control base |

---

## overview
a pci module which is used in the ata driver to get the primary and secondary bases

## function analysis
### `static uint32_t read_pci_config(uint8_t bus_n, uint8_t device, uint8_t fn, uint8_t offset)`
reads the dword at bus `bus_n`, device `device`, function `fn`, and offset `offset`

### `static uint8_t read_header_type(uint8_t bus_n, uint8_t device, uint8_t fn)`
a dedicated function which returns the header byte at bus `bus_n`, device `device`, and function `fn`

### `static pci_triple pci_find_by_class(uint8_t bus_n, uint8_t class, uint8_t subclass, bool *found)`
returns the `pci_triple` location of the class/subclass pair given, through all buses connected to `bus_n`

### `void pci_get_bases(uint16_t *primary_command_out, uint16_t *primary_control_out, uint16_t *secondary_command_out, uint16_t *secondary_control_out)`
fills out the given parameters using `read_pci_config` by getting the BARs 


