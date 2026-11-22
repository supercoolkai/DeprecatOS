# Docs index

every page in `docs/`, grouped the same way as the source tree. each directory's `README.md` is the overview for that module, the other pages go function-by-function.

all `#define` macros across the whole tree are in [definedTerms.md](definedTerms.md).

## core
- [gdt.md](gdt.md) — global descriptor table, selectors, the TSS and `esp0`
- [idt.md](idt.md) — interrupt descriptor table, gate types, which vectors are wired
- [exceptions.md](exceptions.md) — CPU exception names and the kernel panic path

## memory (`kernel/memory/`)
- [memory/README.md](memory/README.md) — key addresses and the overall layout
- [memory/mmap.md](memory/mmap.md) — multiboot info struct and the memory map walk
- [memory/frameAlloc.md](memory/frameAlloc.md) — physical frame bitmap allocator
- [memory/paging.md](memory/paging.md) — page directory/table setup, flag bits
- [memory/heap.md](memory/heap.md) — kernel heap (`kmalloc` / `kfree`) and its block header

## processes (`kernel/process/`)
- [process/README.md](process/README.md) — the `Process` struct and module overview
- [process/queue.md](process/queue.md) — the round-robin process queue
- [process/scheduler.md](process/scheduler.md) — forged task frames, context switch, process creation

## drivers (`kernel/drivers/`)
- [drivers/README.md](drivers/README.md) — struct list and one-liners for every driver
- [drivers/framebuffer.md](drivers/framebuffer.md) — text framebuffer, `FBChar`, the color palette
- [drivers/keyboard.md](drivers/keyboard.md) — PS/2 keyboard, keycodes, the ring buffer feed
- [drivers/serial.md](drivers/serial.md) — `COM1` read/write
- [drivers/timer.md](drivers/timer.md) — PIT tick counter and the RTC epoch
- [drivers/disk/ata.md](drivers/disk/ata.md) — ATA PIO driver, `ata_drive`, port layout
- [drivers/disk/pci.md](drivers/disk/pci.md) — PCI config space reads, finding the ATA channel bases
- [drivers/disk/partition.md](drivers/disk/partition.md) — MBR partition table reader

## filesystem (`kernel/fs/`)
- [fs/README.md](fs/README.md) — module overview
- [fs/block.md](fs/block.md) — `fs_info`, block-level reads/writes over ATA
- [fs/mount.md](fs/mount.md) — mounting partitions under `/mnt/<label>`, the superblock valid bit
- [fs/ext2/README.md](fs/ext2/README.md) — superblock, block group descriptor, inode and dirent formats
- [fs/ext2/bitmap.md](fs/ext2/bitmap.md) — inode/block bitmap allocation
- [fs/ext2/directory.md](fs/ext2/directory.md) — path lookup, directory insert/remove, links

## userland (`kernel/userland/` and `user/`)
- [userland/README.md](userland/README.md) — the kernel side of userland
- [userland/userland.md](userland/userland.md) — mapping the user stack, code and misc pages
- [userland/userlandAsm.md](userland/userlandAsm.md) — the assembly entry into ring 3
- [userland/syscall.md](userland/syscall.md) — the full syscall table and every handler
- [userland/user/README.md](userland/user/README.md) — the actual userland: app, shell, sys
- [userland/user/app/README.md](userland/user/app/README.md) — the `App` table and the app controller
- [userland/user/app/depsh.md](userland/user/app/depsh.md) — every depsh shell command and its usage
- [userland/user/app/dinv.md](userland/user/app/dinv.md) — the dinv modal text editor

## utilities (`etc/`)
- [etc/kernel/README.md](etc/kernel/README.md) — kernel-only utilities overview
- [etc/kernel/kprintf.md](etc/kernel/kprintf.md) — `kprintf`, ANSI color escapes
- [etc/kernel/hexPrinter.md](etc/kernel/hexPrinter.md) — hex printing to framebuffer and serial
- [etc/kernel/portio.md](etc/kernel/portio.md) — `inb` / `outb` style port IO wrappers
- [etc/lib/README.md](etc/lib/README.md) — shared kernel + userland libraries, signatures and one-liners
- [etc/lib/ringBuffer.md](etc/lib/ringBuffer.md) — the 256-byte ring buffer
