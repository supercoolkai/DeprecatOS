# Partition module

## drive_mbr struct format
src `kernel/drivers/disk/partition.h`

just a class mimicing the MBR drive entry format.

| off | field | type |
| --- | --- | --- |
| 0 | `unused` | `uint8_t[446]` |
| 1 | `part_entry_1` | `uint8_t[16]` |
| 2 | `part_entry_2` | `uint8_t[16]` |
| 3 | `part_entry_3` | `uint8_t[16]` |
| 4 | `part_entry_4` | `uint8_t[16]` |
| 5 | `partition_magic_1` | `uint8_t` |
| 6 | `partition_magic_2` | `uint8_t` |

 > Btw you would think this would be padded but it's not because it doesnt really matter

---

## overview
a one-function/struct module, used exclusively in `blockController.c` to initialize the `fs_entries`.

## function analysis

### `struct drive_mbr *read_mbr(struct ata_drive drive)`
rejects all drives which don't support read48 (as we havent added 28-bit support yet), and reads the `drive`'s first sector into the `mbr_buf` via `ata_read48()`. then returns `mbr_buf` casted to `struct drive_mbr *`
