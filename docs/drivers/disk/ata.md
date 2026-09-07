#ATA driver

## **important ports**
   #### drive select IO port: 0x1F6
   #### Sectorcount: 0x1F2 
   #### LBAlo, LBAmid, LBAhi: 0x1F3-0x1F5 
   #### Command IO Port / status port: 0x1F7
   #### Data port: 0x1F0

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
soon to be more (i think)


## **function analysis**
### `void poll(void)`
polls the port till it either returns an error or is ready for rw actions

### `static void wait_bsy_clear(void)`
basically *poll* but it only waits for BSY not DRQ

### `static void wait_100_ns(void)`
self explanatory, waits 100 nanoseconds

### `bool ata_identify(uint8_t drive)`
on drive `drive`, readies the ports and makes sure that the hardware is compatible with the ATA driver. also returns the 256 uint16_t values returned from the data port and puts it into global table `identify_values` 

### `void ata_read48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf)`
reads `count` sectors from drive `drive` and LBA `lba` returns these values to `buf`

### `void ata_write48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf)`
writes `count` sectors from `buf` to drive `drive` and LBA `lba`.

### `void ata_init(void)`
calls `ata_identify` for both ports, and initializes global variables **lba_sector_count** and **supports_read48**

