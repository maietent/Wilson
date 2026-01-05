#include "ata.h"
#include "fat32.h"
#include "klog.h"
#include "memory.h"
#include "string.h"

static fat32_boot_sector_t boot_sector;
static uint32_t fat_start_sector;
static uint32_t data_start_sector;
static uint32_t root_dir_cluster;
static uint32_t current_dir_cluster;
static uint32_t partition_offset = 0;
static uint8_t sector_buffer[512];
static bool fat32_initialized = false;

#define MAX_PATH_LENGTH 256
static char current_path[MAX_PATH_LENGTH] = "/";

static bool read_sector(uint32_t lba, uint8_t* buffer)
{
    return ata_read_sector(partition_offset + lba, buffer);
}

static bool write_sector(uint32_t lba, uint8_t* buffer)
{
    return ata_write_sector(partition_offset + lba, buffer);
}

static uint32_t cluster_to_sector(uint32_t cluster)
{
    return data_start_sector + (cluster - 2) * boot_sector.sectors_per_cluster;
}

static uint32_t fat_read_entry(uint32_t cluster)
{
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;

    if (!read_sector(fat_sector, sector_buffer))
    {
        return 0;
    }

    uint32_t entry = *((uint32_t*)&sector_buffer[entry_offset]) & 0x0FFFFFFF;
    return entry;
}

static bool fat_write_entry(uint32_t cluster, uint32_t value)
{
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;

    if (!read_sector(fat_sector, sector_buffer))
    {
        return false;
    }

    uint32_t* entry = (uint32_t*)&sector_buffer[entry_offset];
    *entry = (*entry & 0xF0000000) | (value & 0x0FFFFFFF);

    if (!write_sector(fat_sector, sector_buffer))
    {
        return false;
    }

    if (boot_sector.fat_count > 1)
    {
        uint32_t fat2_sector = fat_sector + boot_sector.sectors_per_fat_32;
        write_sector(fat2_sector, sector_buffer);
    }

    return true;
}

static uint32_t fat_find_free_cluster(void)
{
    uint32_t total_clusters = boot_sector.total_sectors_32 / boot_sector.sectors_per_cluster;

    for (uint32_t cluster = 2; cluster < total_clusters; cluster++)
    {
        uint32_t entry = fat_read_entry(cluster);
        if (entry == FAT32_FREE_CLUSTER)
        {
            return cluster;
        }
    }

    return 0;
}

static void name_to_fat32(const char* name, char* fat_name)
{
    memset(fat_name, ' ', 11);

    int i = 0, j = 0;
    while (name[i] && name[i] != '.' && j < 8)
    {
        fat_name[j++] = (name[i] >= 'a' && name[i] <= 'z') ? name[i] - 32 : name[i];
        i++;
    }

    if (name[i] == '.')
    {
        i++;
        j = 8;
        while (name[i] && j < 11)
        {
            fat_name[j++] = (name[i] >= 'a' && name[i] <= 'z') ? name[i] - 32 : name[i];
            i++;
        }
    }
}

static void fat32_to_name(const char* fat_name, char* name)
{
    int i, j = 0;

    for (i = 0; i < 8 && fat_name[i] != ' '; i++)
    {
        name[j++] = fat_name[i];
    }

    if (fat_name[8] != ' ')
    {
        name[j++] = '.';
        for (i = 8; i < 11 && fat_name[i] != ' '; i++)
        {
            name[j++] = fat_name[i];
        }
    }

    name[j] = '\0';
}

static bool fat_names_match(const char* fat_name1, const char* fat_name2)
{
    for (int i = 0; i < 11; i++)
    {
        char c1 = fat_name1[i];
        char c2 = fat_name2[i];

        if (c1 >= 'a' && c1 <= 'z')
        {
            c1 -= 32;
        }
        if (c2 >= 'a' && c2 <= 'z')
        {
            c2 -= 32;
        }

        if (c1 != c2)
        {
            return false;
        }
    }
    return true;
}

static bool find_dir_entry(uint32_t dir_cluster, const char* name, fat32_dir_entry_t* entry, uint32_t* entry_sector, uint32_t* entry_offset)
{
    char fat_name[11];
    name_to_fat32(name, fat_name);

    uint32_t cluster = dir_cluster;

    while (cluster < FAT32_EOC)
    {
        uint32_t sector = cluster_to_sector(cluster);

        for (uint8_t s = 0; s < boot_sector.sectors_per_cluster; s++)
        {
            if (!read_sector(sector + s, sector_buffer))
            {
                return false;
            }

            for (uint16_t offset = 0; offset < 512; offset += 32)
            {
                fat32_dir_entry_t* ent = (fat32_dir_entry_t*)&sector_buffer[offset];

                if (ent->name[0] == 0x00)
                {
                    return false;
                }

                if (ent->name[0] == 0xE5)
                {
                    continue;
                }

                if (ent->attributes == FAT32_ATTR_LFN)
                {
                    continue;
                }

                if (fat_names_match(ent->name, fat_name))
                {
                    memcpy(entry, ent, sizeof(fat32_dir_entry_t));
                    if (entry_sector)
                    {
                        *entry_sector = sector + s;
                    }
                    if (entry_offset)
                    {
                        *entry_offset = offset;
                    }
                    return true;
                }
            }
        }

        cluster = fat_read_entry(cluster);
    }

    return false;
}

static bool create_dir_entry(uint32_t dir_cluster, const char* name, uint32_t first_cluster, uint32_t size, uint8_t attr)
{
    char fat_name[11];
    name_to_fat32(name, fat_name);

    uint32_t cluster = dir_cluster;

    while (cluster < FAT32_EOC)
    {
        uint32_t sector = cluster_to_sector(cluster);

        for (uint8_t s = 0; s < boot_sector.sectors_per_cluster; s++)
        {
            if (!read_sector(sector + s, sector_buffer))
            {
                return false;
            }

            for (uint16_t offset = 0; offset < 512; offset += 32)
            {
                fat32_dir_entry_t* ent = (fat32_dir_entry_t*)&sector_buffer[offset];

                if (ent->name[0] == 0x00 || ent->name[0] == 0xE5)
                {
                    memcpy(ent->name, fat_name, 11);
                    ent->attributes = attr;
                    ent->first_cluster_high = (first_cluster >> 16) & 0xFFFF;
                    ent->first_cluster_low = first_cluster & 0xFFFF;
                    ent->file_size = size;

                    return write_sector(sector + s, sector_buffer);
                }
            }
        }

        uint32_t next = fat_read_entry(cluster);
        if (next >= FAT32_EOC)
        {
            uint32_t new_cluster = fat_find_free_cluster();
            if (new_cluster == 0)
            {
                return false;
            }

            fat_write_entry(cluster, new_cluster);
            fat_write_entry(new_cluster, FAT32_EOC);

            memset(sector_buffer, 0, 512);
            uint32_t new_sector = cluster_to_sector(new_cluster);
            for (uint8_t s = 0; s < boot_sector.sectors_per_cluster; s++)
            {
                write_sector(new_sector + s, sector_buffer);
            }

            cluster = new_cluster;
        }
        else
        {
            cluster = next;
        }
    }

    return false;
}

static bool resolve_path(const char* path, uint32_t* out_cluster, char* out_filename)
{
    if (!path || !*path)
    {
        *out_cluster = current_dir_cluster;
        if (out_filename)
        {
            out_filename[0] = '\0';
        }
        return true;
    }

    uint32_t cluster = (path[0] == '/') ? root_dir_cluster : current_dir_cluster;

    if (path[0] == '/')
    {
        path++;
    }

    if (!*path)
    {
        *out_cluster = cluster;
        if (out_filename)
        {
            out_filename[0] = '\0';
        }
        return true;
    }

    char component[256];
    int comp_idx = 0;

    while (*path)
    {
        if (*path == '/' || *(path + 1) == '\0')
        {
            if (*(path + 1) == '\0' && *path != '/')
            {
                component[comp_idx++] = *path;
            }

            component[comp_idx] = '\0';

            if (comp_idx > 0)
            {
                bool is_last = (*(path + 1) == '\0') || (*path != '/' && *(path + 1) == '\0');

                if (is_last && out_filename)
                {
                    *out_cluster = cluster;
                    strcpy(out_filename, component);
                    return true;
                }

                fat32_dir_entry_t entry;
                if (!find_dir_entry(cluster, component, &entry, NULL, NULL))
                {
                    return false;
                }

                if (!(entry.attributes & FAT32_ATTR_DIRECTORY))
                {
                    return false;
                }

                cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
            }

            comp_idx = 0;
            if (*path == '/')
            {
                path++;
            }
        }
        else
        {
            if (comp_idx < 255)
            {
                component[comp_idx++] = *path;
            }
            path++;
        }
    }

    *out_cluster = cluster;
    if (out_filename)
    {
        out_filename[0] = '\0';
    }
    return true;
}

bool fat32_init(void)
{
    uint8_t mbr[512];
    if (!ata_read_sector(0, mbr))
    {
        klogf("FAT32: Failed to read MBR\n");
        return false;
    }

    if (mbr[510] != 0x55 || mbr[511] != 0xAA)
    {
        klogf("FAT32: Invalid MBR signature (0x%02X%02X)\n", mbr[511], mbr[510]);
        return false;
    }

    uint8_t* part_entry = &mbr[0x1BE];
    uint8_t part_type = part_entry[4];

    if (part_type != 0x0B && part_type != 0x0C && part_type != 0x83)
    {
        klogf("FAT32: No FAT32 partition found (type: 0x%02X)\n", part_type);
        partition_offset = 0;
    }
    else
    {
        partition_offset = *(uint32_t*)&part_entry[8];
    }

    uint8_t temp_buffer[512];
    if (!ata_read_sector(partition_offset, temp_buffer))
    {
        return false;
    }

    memcpy(&boot_sector, temp_buffer, sizeof(fat32_boot_sector_t));

    if (temp_buffer[510] != 0x55 || temp_buffer[511] != 0xAA)
    {
        klogf("FAT32: Invalid boot signature (0x%02X%02X)\n", temp_buffer[511], temp_buffer[510]);
        return false;
    }

    if (boot_sector.bytes_per_sector != 512)
    {
        klogf("FAT32: Unsupported sector size\n");
        return false;
    }

    if (memcmp(boot_sector.fs_type, "FAT32   ", 8) != 0 && memcmp(boot_sector.fs_type, "FAT32", 5) != 0)
    {
        klogf("FAT32: Not a FAT32 filesystem\n");
        return false;
    }

    if (boot_sector.root_cluster < 2)
    {
        klogf("FAT32: Invalid root cluster\n");
        return false;
    }

    fat_start_sector = boot_sector.reserved_sectors;
    data_start_sector = fat_start_sector + (boot_sector.fat_count * boot_sector.sectors_per_fat_32);
    root_dir_cluster = boot_sector.root_cluster;
    current_dir_cluster = root_dir_cluster;
    strcpy(current_path, "/");
    fat32_initialized = true;
    return true;
}

bool fat32_open(const char* path, fat32_file_t* file)
{
    char filename[256];
    uint32_t dir_cluster;

    if (!resolve_path(path, &dir_cluster, filename))
    {
        return false;
    }

    fat32_dir_entry_t entry;
    if (!find_dir_entry(dir_cluster, filename, &entry, NULL, NULL))
    {
        return false;
    }

    file->first_cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
    file->current_cluster = file->first_cluster;
    file->file_size = entry.file_size;
    file->position = 0;
    file->attributes = entry.attributes;
    file->is_open = true;

    return true;
}

bool fat32_create(const char* path, fat32_file_t* file)
{
    char filename[256];
    uint32_t dir_cluster;

    if (!resolve_path(path, &dir_cluster, filename))
    {
        return false;
    }

    uint32_t new_cluster = fat_find_free_cluster();
    if (new_cluster == 0)
    {
        return false;
    }

    fat_write_entry(new_cluster, FAT32_EOC);

    if (!create_dir_entry(dir_cluster, filename, new_cluster, 0, FAT32_ATTR_ARCHIVE))
    {
        return false;
    }

    file->first_cluster = new_cluster;
    file->current_cluster = new_cluster;
    file->file_size = 0;
    file->position = 0;
    file->attributes = FAT32_ATTR_ARCHIVE;
    file->is_open = true;

    return true;
}

int fat32_read(fat32_file_t* file, uint8_t* buffer, uint32_t size)
{
    if (!file->is_open)
    {
        return -1;
    }

    if (file->position >= file->file_size)
    {
        return 0;
    }

    if (file->position + size > file->file_size)
    {
        size = file->file_size - file->position;
    }

    uint32_t bytes_read = 0;
    uint32_t cluster_size = boot_sector.sectors_per_cluster * 512;

    while (bytes_read < size && file->current_cluster < FAT32_EOC)
    {
        uint32_t cluster_offset = file->position % cluster_size;
        uint32_t sector_in_cluster = cluster_offset / 512;
        uint32_t offset_in_sector = cluster_offset % 512;

        uint32_t sector = cluster_to_sector(file->current_cluster) + sector_in_cluster;

        if (!read_sector(sector, sector_buffer))
        {
            return -1;
        }

        uint32_t to_copy = 512 - offset_in_sector;
        if (to_copy > size - bytes_read)
        {
            to_copy = size - bytes_read;
        }

        memcpy(buffer + bytes_read, sector_buffer + offset_in_sector, to_copy);

        bytes_read += to_copy;
        file->position += to_copy;

        if ((file->position % cluster_size) == 0)
        {
            file->current_cluster = fat_read_entry(file->current_cluster);
        }
    }

    return bytes_read;
}

int fat32_write(fat32_file_t* file, const uint8_t* buffer, uint32_t size)
{
    if (!file->is_open)
    {
        return -1;
    }

    uint32_t bytes_written = 0;
    uint32_t cluster_size = boot_sector.sectors_per_cluster * 512;

    while (bytes_written < size)
    {
        if (file->current_cluster >= FAT32_EOC)
        {
            uint32_t new_cluster = fat_find_free_cluster();
            if (new_cluster == 0)
            {
                break;
            }

            fat_write_entry(new_cluster, FAT32_EOC);

            if (file->position == 0)
            {
                file->first_cluster = new_cluster;
            }
            else
            {
                uint32_t prev_cluster = file->first_cluster;
                while (fat_read_entry(prev_cluster) < FAT32_EOC)
                {
                    prev_cluster = fat_read_entry(prev_cluster);
                }
                fat_write_entry(prev_cluster, new_cluster);
            }

            file->current_cluster = new_cluster;
        }

        uint32_t cluster_offset = file->position % cluster_size;
        uint32_t sector_in_cluster = cluster_offset / 512;
        uint32_t offset_in_sector = cluster_offset % 512;

        uint32_t sector = cluster_to_sector(file->current_cluster) + sector_in_cluster;

        if (offset_in_sector != 0 || size - bytes_written < 512)
        {
            if (!read_sector(sector, sector_buffer))
            {
                return -1;
            }
        }

        uint32_t to_copy = 512 - offset_in_sector;
        if (to_copy > size - bytes_written)
        {
            to_copy = size - bytes_written;
        }

        memcpy(sector_buffer + offset_in_sector, buffer + bytes_written, to_copy);

        if (!write_sector(sector, sector_buffer))
        {
            return -1;
        }

        bytes_written += to_copy;
        file->position += to_copy;

        if (file->position > file->file_size)
        {
            file->file_size = file->position;
        }

        if ((file->position % cluster_size) == 0)
        {
            uint32_t next = fat_read_entry(file->current_cluster);
            if (next >= FAT32_EOC)
            {
                file->current_cluster = FAT32_EOC;
            }
            else
            {
                file->current_cluster = next;
            }
        }
    }

    return bytes_written;
}

bool fat32_close(fat32_file_t* file)
{
    if (!file->is_open)
    {
        return false;
    }

    fat32_dir_entry_t entry;
    uint32_t entry_sector, entry_offset;

    const char* path = "";
    if (find_dir_entry(root_dir_cluster, path, &entry, &entry_sector, &entry_offset))
    {
        if (!read_sector(entry_sector, sector_buffer))
        {
            return false;
        }

        fat32_dir_entry_t* ent = (fat32_dir_entry_t*)&sector_buffer[entry_offset];
        ent->file_size = file->file_size;

        write_sector(entry_sector, sector_buffer);
    }

    file->is_open = false;
    return true;
}

bool fat32_seek(fat32_file_t* file, uint32_t position)
{
    if (!file->is_open || position > file->file_size)
    {
        return false;
    }

    file->position = position;
    file->current_cluster = file->first_cluster;

    uint32_t cluster_size = boot_sector.sectors_per_cluster * 512;
    uint32_t clusters_to_skip = position / cluster_size;

    for (uint32_t i = 0; i < clusters_to_skip; i++)
    {
        file->current_cluster = fat_read_entry(file->current_cluster);
        if (file->current_cluster >= FAT32_EOC)
        {
            return false;
        }
    }

    return true;
}

bool fat32_delete(const char* path)
{
    char filename[256];
    uint32_t dir_cluster;

    if (!resolve_path(path, &dir_cluster, filename))
    {
        return false;
    }

    fat32_dir_entry_t entry;
    uint32_t entry_sector, entry_offset;

    if (!find_dir_entry(dir_cluster, filename, &entry, &entry_sector, &entry_offset))
    {
        return false;
    }

    uint32_t cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;

    while (cluster < FAT32_EOC)
    {
        uint32_t next = fat_read_entry(cluster);
        fat_write_entry(cluster, FAT32_FREE_CLUSTER);
        cluster = next;
    }

    if (!read_sector(entry_sector, sector_buffer))
    {
        return false;
    }

    sector_buffer[entry_offset] = 0xE5;

    return write_sector(entry_sector, sector_buffer);
}

bool fat32_list_dir(const char* path, void (*callback)(const char* name, uint32_t size, uint8_t attr))
{
    uint32_t cluster;

    if (!path || !*path)
    {
        cluster = current_dir_cluster;
    }
    else
    {
        char unused[256];
        if (!resolve_path(path, &cluster, unused))
        {
            return false;
        }
    }

    while (cluster < FAT32_EOC)
    {
        uint32_t sector = cluster_to_sector(cluster);

        for (uint8_t s = 0; s < boot_sector.sectors_per_cluster; s++)
        {
            if (!read_sector(sector + s, sector_buffer))
            {
                return false;
            }

            for (uint16_t offset = 0; offset < 512; offset += 32)
            {
                fat32_dir_entry_t* ent = (fat32_dir_entry_t*)&sector_buffer[offset];

                if (ent->name[0] == 0x00)
                {
                    return true;
                }

                if (ent->name[0] == 0xE5 || ent->attributes == FAT32_ATTR_LFN)
                {
                    continue;
                }

                if (ent->attributes & FAT32_ATTR_VOLUME_ID)
                {
                    continue;
                }

                char name[13];
                fat32_to_name(ent->name, name);
                callback(name, ent->file_size, ent->attributes);
            }
        }

        cluster = fat_read_entry(cluster);
    }

    return true;
}

uint32_t fat32_get_current_dir(void)
{
    return current_dir_cluster;
}

bool fat32_set_current_dir(uint32_t cluster)
{
    current_dir_cluster = cluster;
    return true;
}

const char* fat32_get_current_path(void)
{
    return current_path;
}

bool fat32_change_dir(const char* path)
{
    if (strcmp(path, "/") == 0)
    {
        current_dir_cluster = root_dir_cluster;
        strcpy(current_path, "/");
        return true;
    }

    uint32_t start_cluster;
    char temp_path[MAX_PATH_LENGTH];

    if (path[0] == '/')
    {
        start_cluster = root_dir_cluster;
        strcpy(temp_path, "/");
        path++;
    }
    else
    {
        start_cluster = current_dir_cluster;
        strcpy(temp_path, current_path);
    }

    if (strcmp(path, "..") == 0)
    {
        if (strcmp(temp_path, "/") == 0)
        {
            return true;
        }

        int len = strlen(temp_path);
        if (len > 1)
        {
            if (temp_path[len - 1] == '/')
            {
                len--;
            }

            while (len > 0 && temp_path[len] != '/')
            {
                len--;
            }

            if (len == 0)
            {
                len = 1;
            }

            temp_path[len] = '\0';
        }

        if (strcmp(temp_path, "/") == 0)
        {
            current_dir_cluster = root_dir_cluster;
            strcpy(current_path, "/");
        }
        else
        {
            uint32_t cluster = root_dir_cluster;
            const char* p = temp_path + 1;
            char component[256];
            int comp_idx = 0;

            while (*p)
            {
                if (*p == '/')
                {
                    if (comp_idx > 0)
                    {
                        component[comp_idx] = '\0';
                        fat32_dir_entry_t entry;
                        if (!find_dir_entry(cluster, component, &entry, NULL, NULL))
                        {
                            return false;
                        }

                        if (!(entry.attributes & FAT32_ATTR_DIRECTORY))
                        {
                            return false;
                        }

                        cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
                        comp_idx = 0;
                    }
                    p++;
                }
                else
                {
                    if (comp_idx < 255)
                    {
                        component[comp_idx++] = *p;
                    }
                    p++;
                }
            }

            if (comp_idx > 0)
            {
                component[comp_idx] = '\0';
                fat32_dir_entry_t entry;
                if (!find_dir_entry(cluster, component, &entry, NULL, NULL))
                {
                    return false;
                }

                if (!(entry.attributes & FAT32_ATTR_DIRECTORY))
                {
                    return false;
                }

                cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
            }

            current_dir_cluster = cluster;
            strcpy(current_path, temp_path);
        }

        return true;
    }

    fat32_dir_entry_t entry;
    if (!find_dir_entry(start_cluster, path, &entry, NULL, NULL))
    {
        return false;
    }

    if (!(entry.attributes & FAT32_ATTR_DIRECTORY))
    {
        return false;
    }

    current_dir_cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;

    if (strcmp(temp_path, "/") == 0 && temp_path[1] == '\0')
    {
        strcpy(current_path, "/");
        strcat(current_path, path);
    }
    else
    {
        strcpy(current_path, temp_path);
        if (current_path[strlen(current_path) - 1] != '/')
        {
            strcat(current_path, "/");
        }
        strcat(current_path, path);
    }

    return true;
}
