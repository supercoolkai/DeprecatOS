# Filesystem

## overview
overall a simple filesystem, with only ext2 RW support. 

this will be substantially larger once VFS support is established and i have multiple filesystems. 

### `block`
a simple block reading/writing device which uses the ATA driver (`kernel/drivers/disk/ata.c`) for the majority of its functions.

### `ext2`
currently the only filesystem available. RW support. 

### `mount`
mounts and unmounts the partitions found in `fs_entries` under `/mnt/<label>`, and sets/clears the valid bit in their superblocks on the way in and out (see `docs/fs/mount.md`)
