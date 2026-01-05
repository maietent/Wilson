#pragma once

#include "std.h"

typedef struct {
    uint8_t  jump[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_dir_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} __attribute__((packed)) fat32_boot_sector_t;

typedef struct {
    char     name[11];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_modified_time;
    uint16_t last_modified_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;

#define FAT32_ATTR_READ_ONLY 0x01
#define FAT32_ATTR_HIDDEN    0x02
#define FAT32_ATTR_SYSTEM    0x04
#define FAT32_ATTR_VOLUME_ID 0x08
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE   0x20
#define FAT32_ATTR_LFN       0x0F

#define FAT32_EOC            0x0FFFFFFF
#define FAT32_BAD_CLUSTER    0x0FFFFFF7
#define FAT32_FREE_CLUSTER   0x00000000

typedef struct {
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t file_size;
    uint32_t position;
    uint8_t  attributes;
    bool     is_open;
} fat32_file_t;

bool fat32_init(void);
bool fat32_open(const char *path, fat32_file_t *file);
bool fat32_create(const char *path, fat32_file_t *file);
int  fat32_read(fat32_file_t *file, uint8_t *buffer, uint32_t size);
int  fat32_write(fat32_file_t *file, const uint8_t *buffer, uint32_t size);
bool fat32_close(fat32_file_t *file);
bool fat32_seek(fat32_file_t *file, uint32_t position);
bool fat32_delete(const char *path);
bool fat32_list_dir(const char *path, void (*callback)(const char *name, uint32_t size, uint8_t attr));
uint32_t fat32_get_current_dir(void);
bool fat32_set_current_dir(uint32_t cluster);
bool fat32_change_dir(const char *path);
