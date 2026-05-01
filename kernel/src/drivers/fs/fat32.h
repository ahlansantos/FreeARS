#ifndef FAT32_H
#define FAT32_H
#include <stdint.h>

int fat32_mount(int drive_index, uint64_t lba_start);

void fat32_ls(void (*callback)(const char *name, uint32_t size, int is_dir));

int fat32_cat(const char *filename,
              void (*callback)(const uint8_t *data, uint32_t len));

int fat32_write_file(const char *filename, const uint8_t *data, uint32_t len);

#endif