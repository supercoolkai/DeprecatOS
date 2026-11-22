# Drivers


## structs

### `FBChar` in `kernel/drivers/fb/fbController.h`:
primitive framebuffer character pairing a char with a color

### `ata_drive` in `kernel/drivers/disk/ata.h`:
contains necessary info about an ata drive

### `pci_triple` in `kernel/drivers/disk/pci/pci.c`
a triple representing a pci config, used mostly in `read_pci_config()`

### `drive_mbr` in `kernel/drivers/disk/partition.h`
mimics the MBR drive entry format

## enums
### `KeyCode` in `kernel/drivers/keyboard/keyboard.h`:
primitive key enum, contains all keys on standard keyboard.

## overview
a bucket of software which communicates directly to the hardware

### `serialController`
a driver to write and receive characters from serial port `COM1`

### `fbController`
a primitive framebuffer with rows and columns adapting with screen resolution, uses the font bitmap included in `kernel/drivers/fb/font8x16.bin`

### `keyboard`
a driver which maps keyboard input with the corresponding keycode, adding it to the ringbuffer

### `timer`
a global timer which uses the `irq0_stub` interrupt to increment the tick counter every millisecond

### `ata`
a direct IO driver with the ATA port (if compatible, currently the only option). currently PIO but moving to DMA soon

### `pci`
a pci config space reader, used by the ata driver to find the primary and secondary channel bases

### `partition`
reads the MBR partition table of a drive, used exclusively by `blockController.c` to fill `fs_entries` 
