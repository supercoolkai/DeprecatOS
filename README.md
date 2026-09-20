Hello!! this is my current passion project, DeprecatOS. 

Any documentation about any specific features are in docs/

All `#define` macros are shown in `docs/definedTerms.md`

## kernel design
i try to make this project what i call an "ultra-monolithic kernel". an ultra-monolithic kernel is an extreme approach to the standard monolithic kernel architecture, where although many popular kernel (like BSD, Linux, etc) are technically monolithic, they are highly modular. i dont like that, so in this architecture, everything that *can* be (reasonably) implemented kernel-side *is* kernel-side.

## overview

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
- syscall interface

### filesystem
- read-only ext2 support
- RW ext2 support in-progress

### drivers
- ATA disk driver (PIO for now, DMA soon trust me)
- framebuffer text output (8x16 font)
- PIT timer (ticks)
- PS/2 keyboard driver
- serial port driver

### interrupts
- IDT management 

### shell
- standalone shell with crt0 and syscall stubs (for full command list see docs/userland/shell.md)

### util
- ring buffer
- hex printer
- kwrite (primitive version)

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

## how to setup filesystem
the filesystem (updated with *mkdish.sh*) uses a directory called `disk_root` as its root directory. since it's not in this git repository, you must create it yourself using:
`mkdir -p disk_root` 
while in the project's main directory. then, you can add files of your choice onto it. afterwards, finally run:
`./mkdisk.sh` and `./rebuild.sh`

## btw:
`firmware/` contains OVMF UEFI firmware from [TianoCore EDK2](https://github.com/tianocore/edk2), licensed under BSD-2-Clause-Patent.
