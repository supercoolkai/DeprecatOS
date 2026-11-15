#include "drivers/disk/ata.h"
#include "drivers/disk/pci/pci.h"
#include "kprintf/kprintf.h"
#include "colors/colors.h"
#include "drivers/fb/fbController.h"
#include "exceptions/exceptions.h"
#include <stdint.h>
#include "portio/portio.h"
#include <stdbool.h>
#include "hex/hexPrinter.h"
#include "errors.h"

struct ata_drive master_drive;
struct ata_drive slave_drive;

bool has_slave;

uint16_t primary_cmd_base;
uint16_t primary_ctrl_base;
uint16_t secondary_cmd_base;
uint16_t secondary_ctrl_base;

static void poll(bool is_secondary){
  uint16_t base = is_secondary ? secondary_cmd_base : primary_cmd_base;
  uint8_t test = inb(base + STATUS_OFF);

  for (int i = 0; i < MAX_POLL_REPEAT; i++) {
    // err case
    if ((test & DF_BIT) || (test & ERR_BIT)) {
      uint32_t err = (uint32_t) inb(base + ERR_OFF);
      print_hex(err, RED);
      panic("KERNEL PANIC: POLL ABORTED");
    }

    // regular case
    if ((!(test & BSY_BIT)) && (test & DRQ_BIT)) {
      return;
    }

    test = inb(base + STATUS_OFF);
  }

  panic("KERNEL PANIC: POLL UNRESPONSIVE");
}

// cases where waiting for only bsy is the only option
static void wait_bsy_clear(bool is_secondary) {
  uint16_t base = is_secondary ? secondary_cmd_base : primary_cmd_base;
  for (int i = 0; i < MAX_POLL_REPEAT; i ++) {
    if (!(inb(base + STATUS_OFF) & BSY_BIT)) {
      return;
    }
  }

  panic("KERNEL PANIC: BSY CLEAR UNRESPONSIVE");
}

static void wait_100_ns(bool is_secondary) {
  uint16_t base = is_secondary ? secondary_ctrl_base : primary_ctrl_base;
  inb(base + ALT_STATUS_OFF);
}

static bool ata_identify_secondary(uint8_t drive)
{
  // send cmd for master/slave drive
  outb(secondary_cmd_base + DRIVE_SELECT_IO_OFF, drive);
  
  // settle
  for (int i = 0; i < 4; i++)
    wait_100_ns(true);

  // zero sectorcount and all lba ports
  outb(secondary_cmd_base + SECTOR_COUNT_OFF, 0);
  outb(secondary_cmd_base + LBA_LO_OFF, 0);
  outb(secondary_cmd_base + LBA_MID_OFF, 0);
  outb(secondary_cmd_base + LBA_HI_OFF, 0);
  
  // send idetify command
  outb(secondary_cmd_base + COMMAND_IO_OFF, IDENTIFY_CMD);

  uint8_t identity = inb(secondary_cmd_base + STATUS_OFF);
  if (identity == 0) {
    if (drive == ATA_MASTER) {
      panic("\nKERNEL PANIC: NO SECONDARY OR PRIMARY MASTER DRIVE");
    }
    kprintf(KPRINTF_RED "\nWARNING: NO SECONDARY OR PRIMARY SLAVE DRIVE!!");
    return false;
  }
  if (identity == 0xFF) {
    if (drive == ATA_MASTER) {
      panic("\nKERNEL PANIC: FLOATING BUS ON BOTH MASTER DRIVES");
    }

    kprintf(KPRINTF_RED "\nWARNING: FLOATING BUS ON BOTH SLAVE DRIVES" KPRINTF_RESET);
    return false;
  }
  
  wait_bsy_clear(true);

  // check for incompatible hardware (ATAPI/SATA)
  if (inb(secondary_cmd_base + LBA_MID_OFF) != 0 || inb(secondary_cmd_base+ LBA_HI_OFF) != 0) {
    if (drive == ATA_MASTER) {
      panic("KERNEL PANIC: INCOMPATIBLE HARDWARE ON BOTH MASTER DRIVES");
    }

    kprintf(KPRINTF_RED "\nWARNING: INCOMPATBILE HARDWARE ON BOTH SLAVE DRIVES" KPRINTF_RESET);
    return false;
  }

  poll(true);

  // simple output
  for (int i = 0; i < IDENTIFY_VALUE_LENGTH; i++) {
    uint16_t val = inw(secondary_cmd_base + DATA_OFF);
    if (drive == ATA_MASTER) {
      master_drive.identify_values[i] = val;
    }
    else{
      slave_drive.identify_values[i] = val;
    }
  }

  return true;
}

bool ata_identify(uint8_t drive, bool *is_secondary) {
  // send cmd for master/slave drive
  outb(primary_cmd_base + DRIVE_SELECT_IO_OFF, drive);
  
  // settle
  for (int i = 0; i < 4; i++)
    wait_100_ns(false);

  // zero sectorcount and all lba ports
  outb(primary_cmd_base + SECTOR_COUNT_OFF, 0);
  outb(primary_cmd_base + LBA_LO_OFF, 0);
  outb(primary_cmd_base + LBA_MID_OFF, 0);
  outb(primary_cmd_base + LBA_HI_OFF, 0);
  
  // send idetify command
  outb(primary_cmd_base + COMMAND_IO_OFF, IDENTIFY_CMD);
        
  uint8_t identity = inb(primary_cmd_base + STATUS_OFF);
  if (identity == 0) {
    *is_secondary = true;
    return ata_identify_secondary(drive);
  }
  if (identity == 0xFF) {
    *is_secondary = true;
    return ata_identify_secondary(drive);
  }
  
  wait_bsy_clear(false);

  // check for incompatible hardware (ATAPI/SATA)
  if (inb(primary_cmd_base + LBA_MID_OFF) != 0 || inb(primary_cmd_base + LBA_HI_OFF) != 0) {
    *is_secondary = true;
    return ata_identify_secondary(drive);
  }

  poll(false);

  // simple output
  for (int i = 0; i < IDENTIFY_VALUE_LENGTH; i++) {
    uint16_t val = inw(primary_cmd_base + DATA_OFF);
    if (drive == ATA_MASTER) {
      master_drive.identify_values[i] = val;
    }
    else{
      slave_drive.identify_values[i] = val;
    }
  }

  return true;
}

void ata_read48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf)
{
  uint64_t lba_sector_count;
  uint16_t base;
  if (drive == ATA_MASTER){
    lba_sector_count = master_drive.lba_sector_count;
    base = master_drive.is_secondary ? secondary_cmd_base : primary_cmd_base;
  }
  else if (drive == ATA_SLAVE){
    lba_sector_count = slave_drive.lba_sector_count;
    base = slave_drive.is_secondary ? secondary_cmd_base : primary_cmd_base;
  }
  else{
    return;
  }
  if (lba + count > lba_sector_count) {
    panic("KERNEL PANIC: LBA OUT OF BOUNDS");
  }

  wait_bsy_clear(base != primary_cmd_base);

  if (drive == ATA_MASTER)
    outb(base + DRIVE_SELECT_IO_OFF, LBA48_MASTER_DRIVE);
  else
    outb(base + DRIVE_SELECT_IO_OFF, LBA48_SLAVE_DRIVE);

  // register navigation
  outb(base + SECTOR_COUNT_OFF, count >> 8);
  outb(base + LBA_LO_OFF, lba >> 24);
  outb(base + LBA_MID_OFF, lba >> 32);
  outb(base + LBA_HI_OFF, lba >> 40);
  outb(base + SECTOR_COUNT_OFF, count & 0xFF);
  outb(base + LBA_LO_OFF, lba & 0xFF);
  outb(base + LBA_MID_OFF, lba >> 8);
  outb(base + LBA_HI_OFF, lba >> 16);
  outb(base + COMMAND_IO_OFF, READ_EXT);
  for (int k = 0; k < 4; k++)
    wait_100_ns(base != primary_cmd_base);
  
  // nav through all sectors sent and output to buf
  for (int i = 0; i < count; i++) {
    poll(base != primary_cmd_base);
    for (int j = 0; j < WORDS_PER_SECTOR; j++) {
      uint16_t val = inw(base + DATA_OFF);
      buf[i * WORDS_PER_SECTOR + j] = val;
    }
    for (int k = 0; k < 4; k++)
        wait_100_ns(base != primary_cmd_base);
  }
}

void ata_write48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf)
{
  uint64_t lba_sector_count;
  uint16_t base;
  if (drive == ATA_MASTER){
    lba_sector_count = master_drive.lba_sector_count;
    base = master_drive.is_secondary ? secondary_cmd_base : primary_cmd_base;
  }
  else if (drive == ATA_SLAVE){
    lba_sector_count = slave_drive.lba_sector_count;
    base = slave_drive.is_secondary ? secondary_cmd_base : primary_cmd_base;
  }
  else{
    return;
  }
  if (lba + count > lba_sector_count) {
    panic("KERNEL PANIC: LBA OUT OF BOUNDS");
  }

  wait_bsy_clear(base != primary_cmd_base);

  if (drive == ATA_MASTER)
    outb(base + DRIVE_SELECT_IO_OFF, LBA48_MASTER_DRIVE);
  else
    outb(base + DRIVE_SELECT_IO_OFF, LBA48_SLAVE_DRIVE);

  // register navigation
  outb(base + SECTOR_COUNT_OFF, count >> 8);
  outb(base + LBA_LO_OFF, lba >> 24);
  outb(base + LBA_MID_OFF, lba >> 32);
  outb(base + LBA_HI_OFF, lba >> 40);
  outb(base + SECTOR_COUNT_OFF, count & 0xFF);
  outb(base + LBA_LO_OFF, lba & 0xFF);
  outb(base + LBA_MID_OFF, lba >> 8);
  outb(base + LBA_HI_OFF, lba >> 16);
  outb(base + COMMAND_IO_OFF, WRITE_EXT);

  // nav through all sectors and output from buf to there
  for (int i = 0 ;i < count; i++){
    poll(base != primary_cmd_base);
    for (int j = 0; j < WORDS_PER_SECTOR; j++) {
      outw(base + DATA_OFF, buf[i * WORDS_PER_SECTOR + j]);
    }

    for (int k = 0; k < 4; k++)
        wait_100_ns(base != primary_cmd_base);
  }
  wait_bsy_clear(base != primary_cmd_base);
  outb(base + COMMAND_IO_OFF, FLUSH_EXT_CMD);

  for (int k = 0; k < 4; k++)
    wait_100_ns(base != primary_cmd_base);

  wait_bsy_clear(base != primary_cmd_base);
}

void ata_read48_part(uint8_t drive, uint64_t base_lba, uint64_t part_len, uint64_t lba, uint16_t count, uint16_t *buf)
{
  if (lba + count > part_len) {
    panic("KERNEL PANIC: LBA OUT OF PARTITION BOUNDS");
  }

  ata_read48(drive, base_lba + lba, count, buf);
}

void ata_write48_part(uint8_t drive, uint64_t base_lba, uint64_t part_len, uint64_t lba, uint16_t count, uint16_t *buf)
{
  if (lba + count > part_len) {
    panic("KERNEL PANIC: LBA OUT OF PARTITION BOUNDS");
  }

  ata_write48(drive, base_lba + lba, count, buf);
}

void ata_flush_drives(void)
{ 
  uint16_t base = master_drive.is_secondary ? secondary_cmd_base : primary_cmd_base;
  outb(base + DRIVE_SELECT_IO_OFF, LBA48_MASTER_DRIVE);
  outb(base + COMMAND_IO_OFF, FLUSH_CACHE_CMD);

  for (int k = 0; k < 4; k++)
    wait_100_ns(master_drive.is_secondary);

  wait_bsy_clear(master_drive.is_secondary);

  if (has_slave){ 
    base = slave_drive.is_secondary ? secondary_cmd_base : primary_cmd_base;
    outb(base + DRIVE_SELECT_IO_OFF, LBA48_SLAVE_DRIVE);
    outb(base + COMMAND_IO_OFF, FLUSH_CACHE_CMD);

    for (int k = 0; k < 4; k++)
      wait_100_ns(slave_drive.is_secondary);

    wait_bsy_clear(slave_drive.is_secondary);
  }
}

void ata_init(void)
{
  pci_get_bases(&primary_cmd_base, &primary_ctrl_base, &secondary_cmd_base, &secondary_ctrl_base);

  master_drive.identify_magic = ATA_MASTER;
  slave_drive.identify_magic = ATA_SLAVE;

  // identification, warn if slave no exist
  ata_identify(master_drive.identify_magic, &master_drive.is_secondary);
  bool b = ata_identify(slave_drive.identify_magic, &slave_drive.is_secondary);

  has_slave = true;

  if (!b) {
    has_slave = false;
    kprintf(KPRINTF_YELLOW "\nIdentification failed for the slave driver. Highly recommended to reboot and try again if you are sure this is not a hardware issue. Initialization has continued without.\n" KPRINTF_RESET);
  }

  outb(primary_ctrl_base + DEVICE_CONTROL_OFF, NIEN_BIT);
  
  // get lba sector count for bounds check
  master_drive.lba_sector_count = (uint64_t)master_drive.identify_values[LBA_SECTOR_COUNT_LO_IND]
                   | (uint64_t)master_drive.identify_values[LBA_SECTOR_COUNT_MID_LO_IND] << 16
                   | (uint64_t)master_drive.identify_values[LBA_SECTOR_COUNT_MID_HI_IND] << 32
                   | (uint64_t)master_drive.identify_values[LBA_SECTOR_COUNT_HI_IND] << 48;
  
  // if no support then panic because no bueno 
  master_drive.supports_read48 = (master_drive.identify_values[SUPPORTED_CMDS_IND] & READ48_SUPPORT_BIT);
  
  if (b){
    // get lba sector count for bounds check
    slave_drive.lba_sector_count = (uint64_t)slave_drive.identify_values[LBA_SECTOR_COUNT_LO_IND]
                    | (uint64_t)slave_drive.identify_values[LBA_SECTOR_COUNT_MID_LO_IND] << 16
                    | (uint64_t)slave_drive.identify_values[LBA_SECTOR_COUNT_MID_HI_IND] << 32
                    | (uint64_t)slave_drive.identify_values[LBA_SECTOR_COUNT_HI_IND] << 48;
    
    // if no support then panic because no bueno 
    slave_drive.supports_read48 = (slave_drive.identify_values[SUPPORTED_CMDS_IND] & READ48_SUPPORT_BIT);
  }

  if (!master_drive.supports_read48) 
    panic("KERNEL PANIC: INCOMPATBILE HARDWARE, NO 48-BIT PIO SUPPORT");
  
  if (b && !slave_drive.supports_read48){
    kprintf(KPRINTF_YELLOW "\nSlave drive does not support read48, unusable." KPRINTF_RESET);
    slave_drive.lba_sector_count = 0;
  }
}
