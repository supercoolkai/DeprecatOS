#include "drivers/disk/ata.h"
#include "drivers/fb/fbController.h"
#include "drivers/serial/serialController.h"
#include "exceptions/exceptions.h"
#include <stdint.h>
#include "portio.h"
#include <stdbool.h>
#include "util/hex/hexPrinter.h"



static uint16_t identify_values[256];
static bool supports_read48;
static uint64_t lba_sector_count;

/* important ports:
  * drive select IO port: 0x1F6
  * Sectorcount: 0x1F2
  * LBAlo, LBAmid, LBAhi: 0x1F3-0x1F5
  * Command IO Port / status port: 0x1F7
  * Data port: 0x1F0
*/


static void poll(void){
  uint8_t test = inb(0x1F7);

  for (int i = 0; i < MAX_POLL_REPEAT; i++) {
    // err case
    if ((test & (1 << 5)) || (test & 1)) {
      uint32_t err = (uint32_t) inb(0x1F1);
      print_hex(err, RED);
      panic("KERNEL PANIC: POLL ABORTED");
    }

    // regular case
    if ((!(test & (1 << 7))) && (test & (1 << 3))) {
      return;
    }

    test = inb(0x1F7);
  }

  panic("KERNEL PANIC: POLL UNRESPONSIVE");
}

// cases where waiting for only bsy is the only option
static void wait_bsy_clear(void) {
  for (int i = 0; i < MAX_POLL_REPEAT; i ++) {
    if (!(inb(0x1F7) & (1 << 7))) {
      return;
    }
  }

  panic("KERNEL PANIC: BSY CLEAR UNRESPONSIVE");
}

static void wait_100_ns(void) {
  inb(0x3F6);
}
bool ata_identify(uint8_t drive) {
  // send cmd for master/slave drive
  outb(0x1F6, drive);
  
  // settle
  for (int i = 0; i < 4; i++)
    wait_100_ns();

  // zero sectorcount and all lba ports
  outb(0x1F2, 0);
  outb(0x1F3, 0);
  outb(0x1F4, 0);
  outb(0x1F5, 0);
  
  // send idetify command
  outb(0x1F7, 0xEC);

  uint8_t identity = inb(0x1F7);

  if (identity == 0 || identity == 0xFF) {
    if (drive == ATA_MASTER) {
      panic("KERNEL PANIC: FLOATING BUS ON MASTER DRIVE");
    }

    fb_draw_string("\nWARNING: FLOATING BUS ON SLAVE DRIVE", RED);
    return false;
  }
  
  wait_bsy_clear();

  // check for incompatible hardware (ATAPI/SATA)
  if (inb(0x1F4) != 0 || inb(0x1F5) != 0) {
    if (drive == ATA_MASTER) {
      panic("KERNEL PANIC: INCOMPATIBLE HARDWARE ON MASTER DRIVE");
    }

    fb_draw_string("\nWARNING: INCOMPATBILE HARDWARE ON SLAVE DRIVE", RED);
    return false;
  }

  poll();

  // simple output
  for (int i = 0; i < 256; i++) {
    uint16_t val = inw(0x1F0);
    if (drive == 0xA0) {
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

  if (drive == 0xA0)
    outb(0x1F6, 0x40);
  else
    outb(0x1F6, 0x50);
  
  // register navigation
  outb(0x1F2, count >> 8);
  outb(0x1F3, lba >> 24);
  outb(0x1F4, lba >> 32);
  outb(0x1F5, lba >> 40);
  outb(0x1F2, count & 0xFF);
  outb(0x1F3, lba & 0xFF);
  outb(0x1F4, lba >> 8);
  outb(0x1F5, lba >> 16);
  outb(0x1F7, 0x24);
  
  // nav through all sectors sent and output to buf
  for (int i = 0; i < count; i++) {
    poll();
    for (int j = 0; j < 256; j++) {
      uint16_t val = inw(0x1F0);
      buf[i * 256 + j] = val;
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

  if (drive == 0xA0)
    outb(0x1F6, 0x40);
  else
    outb(0x1F6, 0x50);
  
  // register navigation
  outb(0x1F2, count >> 8);
  outb(0x1F3, lba >> 24);
  outb(0x1F4, lba >> 32);
  outb(0x1F5, lba >> 40);
  outb(0x1F2, count & 0xFF);
  outb(0x1F3, lba & 0xFF);
  outb(0x1F4, lba >> 8);
  outb(0x1F5, lba >> 16);
  outb(0x1F7, 0x34);

  // nav through all sectors and output from buf to there
  for (int i = 0 ;i < count; i++){
    poll();
    for (int j = 0; j < 256; j++) {
      outw(0x1F0, buf[i * 256 + j]);
    }

    for (int k = 0; k < 4; k++)
        wait_100_ns();
  }
  outb(0x1F7, 0xEA);
  wait_bsy_clear();
}


void ata_init(void)
{
  // identification, warn if slave no exist
  ata_identify(0xA0);
  bool b = ata_identify(0xB0);

  if (!b) {
    fb_draw_string("\nIdentification failed for the slave driver. Highly recommended to reboot and try again if you are sure this is not a hardware issue. Initialization has continued without.\n", YELLOW);  
  }
  
  // get lba sector count for bounds check
  lba_sector_count = (uint64_t)identify_values[100]
                   | (uint64_t)identify_values[101] << 16
                   | (uint64_t)identify_values[102] << 32
                   | (uint64_t)identify_values[103] << 48;
  
  // if no support then panic because no bueno 
  supports_read48 = (identify_values[83] & (1 << 10));

  if (!supports_read48) 
    panic("KERNEL PANIC: INCOMPATBILE HARDWARE, NO 48-BIT PIO SUPPORT");
}
