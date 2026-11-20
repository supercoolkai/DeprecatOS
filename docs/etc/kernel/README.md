# Kernel-only utilities

## overview
This is a list of utilities usable only in kernel (most likely because of ring barriers)

## utilities

### `hexPrinter`
prints a given `uint32_t` in hex form, when printing it to the framebuffer it's printed in a given `color` 

### `kprintf`
a basic kprintf implementation, uses ANSI escape codes for colors and prints to both serial and framebuffer

### `portio`
a port IO utility, supports reading/writing `uint8_t`s, `uint16_t`s, and `uint32_t`s
