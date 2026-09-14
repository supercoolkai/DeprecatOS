#include "drivers/disk/ata.h"
#include "util/kprintf/kprintf.h"
#include "drivers/fb/fbController.h"
#include "exceptions/exceptions.h"
#include <stdint.h>
#include "portio.h"
#include <stdbool.h>
#include "util/hex/hexPrinter.h"



static uint16_t identify_values[IDENTIFY_VALUE_LENGTH];
static bool supports_read48;
static uint64_t lba_sector_count;

static void poll(void){
  uint8_t test = inb(STATUS_PORT);

  for (int i = 0; i < MAX_POLL_REPEAT; i++) {
    // err case
    if ((test & DF_BIT) || (test & ERR_BIT)) {
      uint32_t err = (uint32_t) inb(ERR_PORT);
      print_hex(err, RED);
      panic("KERNEL PANIC: POLL ABORTED");
    }

    // regular case
    if ((!(test & BSY_BIT)) && (test & DRQ_BIT)) {
      return;
    }

    test = inb(STATUS_PORT);
  }

  panic("KERNEL PANIC: POLL UNRESPONSIVE");
}

// cases where waiting for only bsy is the only option
static void wait_bsy_clear(void) {
  for (int i = 0; i < MAX_POLL_REPEAT; i ++) {
    if (!(inb(STATUS_PORT) & BSY_BIT)) {
      return;
    }
  }

  panic("KERNEL PANIC: BSY CLEAR UNRESPONSIVE");
}

static void wait_100_ns(void) {
  inb(ALT_STATUS_PORT);
}
bool ata_identify(uint8_t drive) {
  // send cmd for master/slave drive
  outb(DRIVE_SELECT_IO_PORT, drive);
  
  // settle
  for (int i = 0; i < 4; i++)
    wait_100_ns();

  // zero sectorcount and all lba ports
  outb(SECTOR_COUNT_PORT, 0);
  outb(LBA_LO_PORT, 0);
  outb(LBA_MID_PORT, 0);
  outb(LBA_HI_PORT, 0);
  
  // send idetify command
  outb(COMMAND_IO_PORT, IDENTIFY_CMD);

  uint8_t identity = inb(STATUS_PORT);

  if (identity == 0 || identity == 0xFF) {
    if (drive == ATA_MASTER) {
      panic("KERNEL PANIC: FLOATING BUS ON MASTER DRIVE");
    }

    kprintf(KPRINTF_RED "\nWARNING: FLOATING BUS ON SLAVE DRIVE" KPRINTF_RESET);
    return false;
  }
  
  wait_bsy_clear();

  // check for incompatible hardware (ATAPI/SATA)
  if (inb(LBA_MID_PORT) != 0 || inb(LBA_HI_PORT) != 0) {
    if (drive == ATA_MASTER) {
      panic("KERNEL PANIC: INCOMPATIBLE HARDWARE ON MASTER DRIVE");
    }

    kprintf(KPRINTF_RED "\nWARNING: INCOMPATBILE HARDWARE ON SLAVE DRIVE" KPRINTF_RESET);
    return false;
  }

  poll();

  // simple output
  for (int i = 0; i < IDENTIFY_VALUE_LENGTH; i++) {
    uint16_t val = inw(DATA_PORT);
    if (drive == ATA_MASTER) {
      identify_values[i] = val;
    }
  }

  return true;
}

void ata_read48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf)
{
  if (lba + count > lba_sector_count) {
    panic("KERNEL PANIC: LBA OUT OF BOUNDS");
  }

  wait_bsy_clear();

  if (drive == ATA_MASTER)
    outb(DRIVE_SELECT_IO_PORT, LBA48_MASTER_DRIVE);
  else
    outb(DRIVE_SELECT_IO_PORT, LBA48_SLAVE_DRIVE);

  // register navigation
  outb(SECTOR_COUNT_PORT, count >> 8);
  outb(LBA_LO_PORT, lba >> 24);
  outb(LBA_MID_PORT, lba >> 32);
  outb(LBA_HI_PORT, lba >> 40);
  outb(SECTOR_COUNT_PORT, count & 0xFF);
  outb(LBA_LO_PORT, lba & 0xFF);
  outb(LBA_MID_PORT, lba >> 8);
  outb(LBA_HI_PORT, lba >> 16);
  outb(COMMAND_IO_PORT, READ_EXT);
  
  // nav through all sectors sent and output to buf
  for (int i = 0; i < count; i++) {
    poll();
    for (int j = 0; j < WORDS_PER_SECTOR; j++) {
      uint16_t val = inw(DATA_PORT);
      buf[i * WORDS_PER_SECTOR + j] = val;
    }
    for (int k = 0; k < 4; k++)
        wait_100_ns();
  }
}

void ata_write48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf)
{
  if (lba + count > lba_sector_count) {
    panic("KERNEL PANIC: LBA OUT OF BOUNDS");
  }

  wait_bsy_clear();

  if (drive == ATA_MASTER)
    outb(DRIVE_SELECT_IO_PORT, LBA48_MASTER_DRIVE);
  else
    outb(DRIVE_SELECT_IO_PORT, LBA48_SLAVE_DRIVE);

  // register navigation
  outb(SECTOR_COUNT_PORT, count >> 8);
  outb(LBA_LO_PORT, lba >> 24);
  outb(LBA_MID_PORT, lba >> 32);
  outb(LBA_HI_PORT, lba >> 40);
  outb(SECTOR_COUNT_PORT, count & 0xFF);
  outb(LBA_LO_PORT, lba & 0xFF);
  outb(LBA_MID_PORT, lba >> 8);
  outb(LBA_HI_PORT, lba >> 16);
  outb(COMMAND_IO_PORT, WRITE_EXT);

  // nav through all sectors and output from buf to there
  for (int i = 0 ;i < count; i++){
    poll();
    for (int j = 0; j < WORDS_PER_SECTOR; j++) {
      outw(DATA_PORT, buf[i * WORDS_PER_SECTOR + j]);
    }

    for (int k = 0; k < 4; k++)
        wait_100_ns();
  }
  outb(COMMAND_IO_PORT, FLUSH_EXT_CMD);
  wait_bsy_clear();
}


void ata_init(void)
{
  // identification, warn if slave no exist
  ata_identify(ATA_MASTER);
  bool b = ata_identify(ATA_SLAVE);

  if (!b) {
    kprintf(KPRINTF_YELLOW "\nIdentification failed for the slave driver. Highly recommended to reboot and try again if you are sure this is not a hardware issue. Initialization has continued without.\n" KPRINTF_RESET);
  }
  
  // get lba sector count for bounds check
  lba_sector_count = (uint64_t)identify_values[LBA_SECTOR_COUNT_LO_IND]
                   | (uint64_t)identify_values[LBA_SECTOR_COUNT_MID_LO_IND] << 16
                   | (uint64_t)identify_values[LBA_SECTOR_COUNT_MID_HI_IND] << 32
                   | (uint64_t)identify_values[LBA_SECTOR_COUNT_HI_IND] << 48;
  
  // if no support then panic because no bueno 
  supports_read48 = (identify_values[SUPPORTED_CMDS_IND] & READ48_SUPPORT_BIT);

  if (!supports_read48) 
    panic("KERNEL PANIC: INCOMPATBILE HARDWARE, NO 48-BIT PIO SUPPORT");
}
