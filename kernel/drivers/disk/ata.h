#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <stdbool.h>

#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0
#define MAX_POLL_REPEAT 100000000
#define DRIVE_SELECT_IO_OFF 0x06
#define SECTOR_COUNT_OFF 0x02
#define LBA_LO_OFF 0x03
#define LBA_MID_OFF 0x04
#define LBA_HI_OFF 0x05
#define COMMAND_IO_OFF 0x07
#define STATUS_OFF COMMAND_IO_OFF
#define DATA_OFF 0x00
#define ERR_OFF 0x01

// NOTE: these offsets are added to control base, not cmd base
#define ALT_STATUS_OFF 0x02
#define DEVICE_CONTROL_OFF ALT_STATUS_OFF

#define BSY_BIT (1 << 7)
#define DRQ_BIT (1 << 3)
#define DF_BIT (1 << 5)
#define ERR_BIT 1
#define READ48_SUPPORT_BIT (1 << 10)
#define NIEN_BIT 0x02

#define READ_EXT 0x24
#define WRITE_EXT 0x34

#define IDENTIFY_VALUE_LENGTH 256

#define SUPPORTED_CMDS_IND 83

#define BYTES_PER_SECTOR 512
#define WORDS_PER_SECTOR (BYTES_PER_SECTOR / 2)

#define IDENTIFY_CMD 0xEC
#define FLUSH_EXT_CMD 0xEA
#define FLUSH_CACHE_CMD 0xE7
#define LBA48_MASTER_DRIVE 0x40
#define LBA48_SLAVE_DRIVE 0x50

#define LBA_SECTOR_COUNT_LO_IND 100
#define LBA_SECTOR_COUNT_MID_LO_IND 101
#define LBA_SECTOR_COUNT_MID_HI_IND 102
#define LBA_SECTOR_COUNT_HI_IND 103

struct ata_drive {
  bool is_secondary;
  bool supports_read48;
  uint8_t identify_magic;
  uint64_t lba_sector_count;

  uint16_t identify_values[IDENTIFY_VALUE_LENGTH];
};

extern struct ata_drive master_drive;
extern struct ata_drive slave_drive;
extern bool has_slave;

bool ata_identify(uint8_t drive, bool *is_secondary);
void ata_read48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf);
void ata_write48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf);
void ata_read48_part(uint8_t drive, uint64_t base_lba, uint64_t part_len, uint64_t lba, uint16_t count, uint16_t *buf);
void ata_write48_part(uint8_t drive, uint64_t base_lba, uint64_t part_len, uint64_t lba, uint16_t count, uint16_t *buf);
void ata_flush_drives(void);
void ata_init(void);

#endif
