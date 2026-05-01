#ifndef ATA_H
#define ATA_H
#include <stdint.h>

typedef struct {
    uint8_t  present;       
    uint8_t  bus;           
    uint8_t  drive;         
    uint8_t  lba48;         
    uint64_t sector_count; 
    char     model[41];    
} ata_drive_t;

#define ATA_MAX_DRIVES 4

int ata_init(void);

ata_drive_t *ata_get_drive(int index);

int ata_read(int drive_index, uint64_t lba, uint32_t count, void *buf);

int ata_write(int drive_index, uint64_t lba, uint32_t count, const void *buf);

#endif