# Serial Controller

## overview
communicates with the serial port to both read and write from it. especially important for running in the emulator because the console **is** the serial port in QEMU.

## function analysis

### `void serial_init(void)`
do a quick command sequence with the port registers (almost 1:1 code to OSDev so you can look at their example init script and it'll explain most of what this does). the sequence does this (in the same order):
 - disables all interrupts
 - enable DLAB 
 - set divisor lo byte to the first byte of a computed value (`uint16_t divisor`)
 - set divisor hi byte to the last byte of `divisor`
 - enable 8 bits with no parity and one stop bit
 - enable FIFO, and clear them with 14 byte threshold
 - asserts DTR and RTS (instead of 0x0F, which asserts that alongside OUT1 + OUT2).
do note that this does **NOT** test the serial chip like the OSDev wiki does it.

### `uint8_t serial_receive_char(void)`
checks `COM1 + 5` at bit 0 to check whether there is anything to receive. if so, receives a `uint8_t` from `COM1`, if it returns `0` then there is no byte available.

### `void serial_receive(uint8_t *buffer, uint32_t size)`
calls `serial_receive_char()` `size` times and returns each result to `buffer`.

### `void serial_write_char(char c)`
waits until the port is ready by checking `COM1 + 5` at bit 5, then sends `c` as a `uint8_t` to `COM1`.

### `void serial_write_string(const char *str)`
calls `serial_write_char()` for each `char` in `str`
