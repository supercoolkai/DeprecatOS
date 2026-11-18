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
| 4 | `identify_values` | `uint16_t[IDENTIFY_VALUES_LENGTH]` |

---

## offset for important ports on command base
based off of the `#define`s and information in `kernel/drivers/disk/ata.h` and `kernel/drivers/disk/pci/pci.h`

| off | port name | read or write |
| --- | --- | --- |
| 0 | data port | read |
| 1 | error port | read |
| 2 | sector count port | read |
| 3 | lba lo port | read |
| 4 | lba mid port | read |
| 5 | lba hi port | read |
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
#### `static uint16_t identify_values[256]`: 
a table containing all the values returned from `ata_identify`

#### `static bool supports_read48`:
a bool value containing the value of whether or not the hardware supports `ata_read/write48` 
#### `static uint64_t lba_sector_count`:
a count of the amount of sectors and valid LBAs there are. used for bounds checks

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
on drive `drive`, readies the ports and makes sure that the hardware is compatible with the ATA driver. also returns the 256 uint16_t values returned from the data port and puts it into global table `identify_values`. for every port uses `secondary_cmd_base` rather than `primary_cmd_base`. on any error it either stays silent on slave or panics on master

### `bool ata_identify(uint8_t drive, bool *is_secondary)`
on drive `drive`, readies the ports and makes sure that the hardware is compatible with the ATA driver. also returns the 256 uint16_t values returned from the data port and puts it into global table `identify_values`. for every port uses `primary_cmd_base` rather than `secondary_cmd_base`. on any error it tries the secondary version using `ata_identify_secondary()` and turns `*is_secondary` to `true`

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
