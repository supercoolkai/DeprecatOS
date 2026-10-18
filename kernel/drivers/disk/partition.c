#include "drivers/disk/partition.h"
#include "memory/heap/kernelHeap.h"
#include "drivers/disk/ata.h"
#include <stdint.h>
#include <stddef.h>

// NOTE: If the buf leaks its lowk on the caller
struct drive_mbr *read_mbr(struct ata_drive drive)
{
  if (!drive.supports_read48){
    return NULL;
  }
  
  uint16_t *mbr_buf = kmalloc(MBR_SIZE);

  ata_read48(drive.identify_magic, 0, 1, mbr_buf);

  return (struct drive_mbr *) mbr_buf;
}
