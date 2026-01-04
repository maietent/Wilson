#pragma once

#include "std.h"

#define ATA_PRIMARY_BASE      0x1F0
#define ATA_PRIMARY_CTRL      0x3F6
#define ATA_PRIMARY_IRQ       14

#define ATA_SR_BSY    0x80
#define ATA_SR_DRDY   0x40
#define ATA_SR_DRQ    0x08
#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30

void ata_init();
bool ata_read_sector(uint32_t lba, uint8_t *buffer);
bool ata_write_sector(uint32_t lba, const uint8_t *buffer);
void ata_handler();
