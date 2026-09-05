# **ATA Driver**

## **purpose**
direct read and write capabilities for ATA ports (PIO currently, move to DMA soon)
poll ports
soon to be more (i think)

## **important ports**
   #### drive select IO port: 0x1F6
   #### Sectorcount: 0x1F2 
   #### LBAlo, LBAmid, LBAhi: 0x1F3-0x1F5 
   #### Command IO Port / status port: 0x1F7
   #### Data port: 0x1F0

## **global variables**
#### **identify_values**: a table containing all the values returned from *ata_identify*
#### **supports_read48**: a bool value containing the value of whether or not the hardware supports `ata_read/write48` 
#### **lba_sector_count**: a count of the amount of sectors and valid LBAs there are. used for bounds checks

## **function analysis**
### `poll()`
polls the port till it either returns an error or is ready for rw actions

### `wait_bsy_clear()`
basically *poll* but it only waits for BSY not DRQ

### `wait_100_ns()`
self explanatory, waits 100 nanoseconds

### `ata_identify()`
readies the ports and makes sure that the hardware is compatible with the ATA driver. also returns the 256 uint16_t values returned from the data port and puts it into **identify_values** 

### `ata_read48()`
reads **count** sectors from drive **drive** and LBA **lba**

### `ata_write48()`
writes **count** sectors to drive **drive** and LBA **lba**

### `ata_init()`
calls `ata_identify` for both ports, and initializes global variables **lba_sector_count** and **supports_read48**

