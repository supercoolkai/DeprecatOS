# ATA driver

## ata_drive struct format
src `kernel/drivers/disk/ata.h`

a collection of information about an ata drive

| off | field | type |
|---|---|---|
| 0 | `is_secondary` | `bool` |
| 1 | `supports_read48` | `bool` |
| 2 | `identify_magic` | `uint8_t` |
| 3 | `lba_sector_count` | `uint64_t` |
| 4 | `identify_values` | `uint16_t[IDENTIFY_VALUE_LENGTH]` |

---

## offset for important ports on command base
based off of the `#define`s and information in `kernel/drivers/disk/ata.h` and `kernel/drivers/disk/pci/pci.h`

| off | port name | read or write |
| --- | --- | --- |
| 0 | data port | read and write |
| 1 | error port | read |
| 2 | sector count port | write |
| 3 | lba lo port | write |
| 4 | lba mid port | write (read during identify) |
| 5 | lba hi port | write (read during identify) |
| 6 | drive select port | write |
| 7 | command port | write |
| 7 | status port | read |

---

## offset for important ports on control base
based off of the `#define`s and information in `kernel/drivers/disk/ata.h` and `kernel/drivers/disk/pci/pci.h`

| off | port name | read or write |
| --- | --- | --- |
| 2 | alternate status port | read |
| 2 | device control port | write |

---

## **global variables**
#### `struct ata_drive master_drive`:
the master drive's `ata_drive` struct, filled in by `ata_init()`

#### `struct ata_drive slave_drive`:
the slave drive's `ata_drive` struct, only meaningful if `has_slave` is `true`

#### `bool has_slave`:
whether `ata_identify` succeeded on the slave drive during `ata_init()`

#### `uint16_t primary_cmd_base`, `uint16_t primary_ctrl_base`, `uint16_t secondary_cmd_base`, `uint16_t secondary_ctrl_base`:
the four port bases handed out by `pci_get_bases()`, every port offset in the tables above is added to one of these

## overview
direct read and write capabilities for ATA ports (PIO currently, move to DMA soon)
poll ports
now reworked to have every port just be a base + offset pair for non-legacy devices


## **function analysis**
### `static void poll(bool is_secondary)`
polls the port till it either returns an error or is ready for rw actions

### `static void wait_bsy_clear(bool is_secondary)`
basically `poll()` but it only waits for BSY not DRQ

### `static void wait_100_ns(bool is_secondary)`
self explanatory, waits 100 nanoseconds

### `static bool ata_identify_secondary(uint8_t drive)`
on drive `drive`, readies the ports and makes sure that the hardware is compatible with the ATA driver. also stores the 256 uint16_t values returned from the data port into the `identify_values` field of whichever `ata_drive` struct `drive` belongs to (`master_drive` or `slave_drive`). for every port uses `secondary_cmd_base` rather than `primary_cmd_base`. on any error it either stays silent on slave or panics on master

### `bool ata_identify(uint8_t drive, bool *is_secondary)`
on drive `drive`, readies the ports and makes sure that the hardware is compatible with the ATA driver. also stores the 256 uint16_t values returned from the data port into the `identify_values` field of whichever `ata_drive` struct `drive` belongs to (`master_drive` or `slave_drive`). for every port uses `primary_cmd_base` rather than `secondary_cmd_base`. on any error it tries the secondary version using `ata_identify_secondary()` and turns `*is_secondary` to `true`

### `void ata_read48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf)`
reads `count` sectors from drive `drive` and LBA `lba` returns these values to `buf`

### `void ata_write48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf)`
writes `count` sectors from `buf` to drive `drive` and LBA `lba`.

### `void ata_read48_part(uint8_t drive, uint64_t base_lba, uint64_t part_len, uint64_t lba, uint16_t count, uint16_t *buf)`
calls `ata_read48()` but with partition awareness, partition's lba is specified with `base_lba` and the lba within the partition is still `lba`

### `void ata_write48_part(uint8_t drive, uint64_t base_lba, uint64_t part_len, uint64_t lba, uint16_t count, uint16_t *buf)`
calls `ata_write48()` but with partition awareness, partition's lba is specified with `base_lba` and the lba within the partition is still `lba`


### `void ata_flush_drives(void)`
runs the flush command on the master and slave drive (if it exists)

### `void ata_init(void)`
calls `pci_get_bases()` on the global base variables, then calls `ata_identify` for both drives, and initializes their corresponding structs' `supports_read48` and `lba_sector_count`
