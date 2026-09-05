Hello!! this is my current passion project, DeprecatOS. 

Any documentation about any specific features are in docs/

All `#define` macros are shown in `docs/terms/definedTerms.md`

## features

### boot & core
- x86-32 kernel, boot via GRUB multiboot from an ISO. built for QEMU, never tested on real hardware.
- exception handling with kernel panic

### memory management
- physical frame allocator
- paging / virtual memory
- kernel heap allocator
- memory map parsing (bootloader)

### processes & userland
- ring 3 userland with TSS setup
- round-robin scheduler with a pqueue
- syscall interface (for full list see `docs/userland/syscall.md`)

### filesystem
- read-only ext2 support (for full feature list see `docs/fs/ext2/README.md`)

### drivers
- ATA disk driver (for full feature list see docs/drivers/disk/ata)
- framebuffer text output (8x16 font)
- PIT timer (ticks)
- PS/2 keyboard driver
- serial port driver

### shell
- standalone shell with crt0 and syscall stubs (for full command list see docs/userland/shell.md)

### util
- ring buffer
- hex printer

## **warnings:**
the *cat/ls/cd* shell command may throw off the timer by a few milliseconds.

don't actually run this on real hardware: the ata driver does raw PIO sector writes with no partition awareness (for now). it CAN harm your machine until further notice.

*mkdisk.sh* destroys disk.img, so whenever u run it it wipes all your changes from inside the OS 

this is a **hobby OS**, expect crashes, incomplete features, and broken changes w/o notice

## dependencies (for running on emulators):

- gcc -m32 (no cross-compiler)
- cmake
- grub-mkrescue
- xorriso
- qemu-system-x86_64
- e2fsprogs (with mke2fs -d support)

## btw:
`firmware/` contains OVMF UEFI firmware from [TianoCore EDK2](https://github.com/tianocore/edk2), licensed under BSD-2-Clause-Patent.
