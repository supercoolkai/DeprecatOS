# Filesystem

## overview
overall a simple filesystem, with only ext2 RO support. 

this will be substantially larger once VFS support is established and i have multiple filesystems. 

### `block`
a simple block reading/writing device which uses the ATA driver (`src/drivers/disk/ata.c`) for the majority of its functions.

### `ext2`
currently the only filesystem available. RO support only. 
