#include "ata.h"
#include <stdint.h>
#include <stddef.h>

#define ATA_BUS_PRIMARY    0
#define ATA_BUS_SECONDARY  1

static const uint16_t ata_base[2] = { 0x1F0, 0x170 };
static const uint16_t ata_ctrl[2] = { 0x3F6, 0x376 };

#define ATA_REG_DATA        0
#define ATA_REG_ERROR       1
#define ATA_REG_FEATURES    1
#define ATA_REG_SECCOUNT0   2
#define ATA_REG_LBA0        3
#define ATA_REG_LBA1        4
#define ATA_REG_LBA2        5
#define ATA_REG_HDDEVSEL    6
#define ATA_REG_COMMAND     7
#define ATA_REG_STATUS      7

#define ATA_SR_BSY  0x80  
#define ATA_SR_DRDY 0x40  
#define ATA_SR_DRQ  0x08  
#define ATA_SR_ERR  0x01  


#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_IDENTIFY        0xEC

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0,%1" :: "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile("inb %1,%0" : "=a"(val) : "Nd"(port));
    return val;
}
static inline uint16_t inw(uint16_t port) {
    uint16_t val;
    asm volatile("inw %1,%0" : "=a"(val) : "Nd"(port));
    return val;
}
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0,%1" :: "a"(val), "Nd"(port));
}

static ata_drive_t drives[ATA_MAX_DRIVES];
static int         drive_count = 0;

static void ata_delay(uint8_t bus) {
    inb(ata_ctrl[bus]);
    inb(ata_ctrl[bus]);
    inb(ata_ctrl[bus]);
    inb(ata_ctrl[bus]);
}

static uint8_t ata_wait_bsy(uint8_t bus) {
    int timeout = 100000;
    uint8_t status;
    do {
        status = inb(ata_base[bus] + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) return status;
    } while (--timeout);
    return 0xFF; 
}

static uint8_t ata_wait_drq(uint8_t bus) {
    int timeout = 100000;
    uint8_t status;
    do {
        status = inb(ata_base[bus] + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) return status;
        if (status & ATA_SR_DRQ) return status;
    } while (--timeout);
    return 0xFF;
}

static void ata_copy_string(char *dst, uint16_t *src, int word_count) {
    for (int i = 0; i < word_count; i++) {
        dst[i * 2]     = (char)(src[i] >> 8);
        dst[i * 2 + 1] = (char)(src[i] & 0xFF);
    }
    dst[word_count * 2] = '\0';
    int len = word_count * 2;
    while (len > 0 && dst[len - 1] == ' ') { dst[len - 1] = '\0'; len--; }
}

static int ata_identify(uint8_t bus, uint8_t drive_sel, ata_drive_t *out) {
    uint16_t base = ata_base[bus];

    outb(base + ATA_REG_HDDEVSEL, drive_sel ? 0xB0 : 0xA0);
    ata_delay(bus);

    outb(base + ATA_REG_SECCOUNT0, 0);
    outb(base + ATA_REG_LBA0, 0);
    outb(base + ATA_REG_LBA1, 0);
    outb(base + ATA_REG_LBA2, 0);

    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay(bus);

    uint8_t status = inb(base + ATA_REG_STATUS);
    if (status == 0) return 0;

    status = ata_wait_bsy(bus);
    if (status == 0xFF) return 0;

    uint8_t lba1 = inb(base + ATA_REG_LBA1);
    uint8_t lba2 = inb(base + ATA_REG_LBA2);
    if (lba1 != 0 || lba2 != 0) return 0; 

    status = ata_wait_drq(bus);
    if (status & ATA_SR_ERR) return 0;
    if (!(status & ATA_SR_DRQ)) return 0;

    uint16_t identify[256];
    for (int i = 0; i < 256; i++) identify[i] = inw(base + ATA_REG_DATA);

    out->present = 1;
    out->bus     = bus;
    out->drive   = drive_sel;


    out->lba48 = (identify[83] & (1 << 10)) ? 1 : 0;

    if (out->lba48) {
        out->sector_count = ((uint64_t)identify[103] << 48) |
                            ((uint64_t)identify[102] << 32) |
                            ((uint64_t)identify[101] << 16) |
                            ((uint64_t)identify[100]);
    } else {
        out->sector_count = ((uint32_t)identify[61] << 16) | identify[60];
    }

    ata_copy_string(out->model, &identify[27], 20);

    return 1;
}


int ata_init(void) {
    drive_count = 0;

    for (int bus = 0; bus < 2; bus++) {
        outb(ata_ctrl[bus], 0x04); 

        for (volatile int i = 0; i < 1000; i++) asm volatile("pause");
        outb(ata_ctrl[bus], 0x00); 
        ata_delay(bus);

        for (int drv = 0; drv < 2; drv++) {
            ata_drive_t *d = &drives[drive_count];
            for (int i = 0; i < (int)sizeof(ata_drive_t); i++)
                ((uint8_t*)d)[i] = 0;

            if (ata_identify(bus, drv, d)) {
                drive_count++;
                if (drive_count >= ATA_MAX_DRIVES) goto done;
            }
        }
    }
done:
    return drive_count;
}

ata_drive_t *ata_get_drive(int index) {
    if (index < 0 || index >= drive_count) return (void*)0;
    return &drives[index];
}

int ata_read(int drive_index, uint64_t lba, uint32_t count, void *buf) {
    if (drive_index < 0 || drive_index >= drive_count) return -1;
    ata_drive_t *d   = &drives[drive_index];
    uint16_t     base = ata_base[d->bus];
    uint16_t    *dst  = (uint16_t *)buf;

    while (count > 0) {
        uint32_t sectors = count > 256 ? 256 : count;


        if (ata_wait_bsy(d->bus) == 0xFF) return -1;

        if (d->lba48) {

            outb(base + ATA_REG_HDDEVSEL, 0x40 | (d->drive << 4));
            ata_delay(d->bus);

            outb(base + ATA_REG_SECCOUNT0, (sectors >> 8) & 0xFF);
            outb(base + ATA_REG_LBA0,  (lba >> 24) & 0xFF);
            outb(base + ATA_REG_LBA1,  (lba >> 32) & 0xFF);
            outb(base + ATA_REG_LBA2,  (lba >> 40) & 0xFF);

            outb(base + ATA_REG_SECCOUNT0, sectors & 0xFF);
            outb(base + ATA_REG_LBA0,  lba & 0xFF);
            outb(base + ATA_REG_LBA1,  (lba >> 8)  & 0xFF);
            outb(base + ATA_REG_LBA2,  (lba >> 16) & 0xFF);
            outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO_EXT);
        } else {
            outb(base + ATA_REG_HDDEVSEL, 0xE0 | (d->drive << 4) | ((lba >> 24) & 0x0F));
            ata_delay(d->bus);
            outb(base + ATA_REG_SECCOUNT0, sectors & 0xFF);
            outb(base + ATA_REG_LBA0,  lba & 0xFF);
            outb(base + ATA_REG_LBA1,  (lba >> 8)  & 0xFF);
            outb(base + ATA_REG_LBA2,  (lba >> 16) & 0xFF);
            outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
        }

        for (uint32_t s = 0; s < sectors; s++) {
            uint8_t status = ata_wait_drq(d->bus);
            if (status & ATA_SR_ERR) return -1;
            if (!(status & ATA_SR_DRQ)) return -1;
            for (int w = 0; w < 256; w++) *dst++ = inw(base + ATA_REG_DATA);
            ata_delay(d->bus);
        }

        lba   += sectors;
        count -= sectors;
    }
    return 0;
}

int ata_write(int drive_index, uint64_t lba, uint32_t count, const void *buf) {
    if (drive_index < 0 || drive_index >= drive_count) return -1;
    ata_drive_t    *d    = &drives[drive_index];
    uint16_t        base = ata_base[d->bus];
    const uint16_t *src  = (const uint16_t *)buf;

    while (count > 0) {
        uint32_t sectors = count > 256 ? 256 : count;

        if (ata_wait_bsy(d->bus) == 0xFF) return -1;

        if (d->lba48) {
            outb(base + ATA_REG_HDDEVSEL, 0x40 | (d->drive << 4));
            ata_delay(d->bus);
            outb(base + ATA_REG_SECCOUNT0, (sectors >> 8) & 0xFF);
            outb(base + ATA_REG_LBA0,  (lba >> 24) & 0xFF);
            outb(base + ATA_REG_LBA1,  (lba >> 32) & 0xFF);
            outb(base + ATA_REG_LBA2,  (lba >> 40) & 0xFF);
            outb(base + ATA_REG_SECCOUNT0, sectors & 0xFF);
            outb(base + ATA_REG_LBA0,  lba & 0xFF);
            outb(base + ATA_REG_LBA1,  (lba >> 8)  & 0xFF);
            outb(base + ATA_REG_LBA2,  (lba >> 16) & 0xFF);
            outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO_EXT);
        } else {
            outb(base + ATA_REG_HDDEVSEL, 0xE0 | (d->drive << 4) | ((lba >> 24) & 0x0F));
            ata_delay(d->bus);
            outb(base + ATA_REG_SECCOUNT0, sectors & 0xFF);
            outb(base + ATA_REG_LBA0,  lba & 0xFF);
            outb(base + ATA_REG_LBA1,  (lba >> 8)  & 0xFF);
            outb(base + ATA_REG_LBA2,  (lba >> 16) & 0xFF);
            outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
        }

        for (uint32_t s = 0; s < sectors; s++) {
            uint8_t status = ata_wait_drq(d->bus);
            if (status & ATA_SR_ERR) return -1;
            if (!(status & ATA_SR_DRQ)) return -1;
            for (int w = 0; w < 256; w++) outw(base + ATA_REG_DATA, *src++);
            ata_delay(d->bus);
        }

        if (d->lba48)
            outb(base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH_EXT);
        else
            outb(base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        ata_wait_bsy(d->bus);

        lba   += sectors;
        count -= sectors;
    }
    return 0;
}