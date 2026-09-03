#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <stdbool.h>

#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0
#define MAX_POLL_REPEAT 1000000
#define READ_48_AMT 512

bool ata_identify(uint8_t drive);
void ata_read48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf);
void ata_write48(uint8_t drive, uint64_t lba, uint16_t count, uint16_t *buf);
void ata_init(void);

#endif
