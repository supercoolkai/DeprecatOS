# Drivers


## structs

### `FBChar` in `src/drivers/fb/fbController.h`:
primitive framebuffer character pairing a char with a color

## enums
### `KeyCode` in `src/drivers/keyboard/keyboard.h`:
primitive key enum, contains all keys on standard keyboard.

## overview
a bucket of software which communicates directly to the hardware

### `serialController`
a driver to write and receive characters from serial port `COM1`

### `fbController`
a primitive framebuffer with rows and columns adapting with screen resolution, uses the font bitmap included in `src/drivers/fb/font8x16.bin`

### `keyboard`
a driver which maps keyboard input with the corresponding keycode, adding it to the ringbuffer

### `timer`
a global timer which uses the `irq0_stub` interrupt to increment the tick counter every millisecond

### `ata`
a direct IO driver with the ATA port (if compatible, currently the only option). currently PIO but moving to DMA soon 
