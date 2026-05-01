#include "fat32.h"
#include "ata.h"
#include <stdint.h>
#include <stddef.h>

typedef struct __attribute__((packed)) {
    uint8_t  jmp[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;   
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;        
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} bpb_t;

typedef struct __attribute__((packed)) {
    char     name[8];
    char     ext[3];
    uint8_t  attr;
    uint8_t  reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_hi;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_lo;
    uint32_t file_size;
} dir_entry_t;

#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LFN        0x0F  

#define FAT32_EOC       0x0FFFFFF8  
#define FAT32_FREE      0x00000000
#define FAT32_BAD       0x0FFFFFF7

static int      mounted       = 0;
static int      ata_drive     = 0;
static uint64_t part_lba      = 0;  

static uint32_t bytes_per_sector;
static uint32_t sectors_per_cluster;
static uint32_t reserved_sectors;
static uint32_t num_fats;
static uint32_t fat_size;            
static uint32_t root_cluster;
static uint32_t total_sectors;
static uint32_t data_start_sector;  
static uint32_t cluster_count;

#define SECTOR_BUF_SIZE 4096
static uint8_t sector_buf[SECTOR_BUF_SIZE];
static uint8_t fat_buf[SECTOR_BUF_SIZE];

static int fat_strlen(const char *s) {
    int i = 0; while (s[i]) i++; return i;
}
static void fat_memcpy(void *dst, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t*)dst; const uint8_t *s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}
static void fat_memset(void *dst, uint8_t val, uint32_t n) {
    uint8_t *d = (uint8_t*)dst;
    for (uint32_t i = 0; i < n; i++) d[i] = val;
}
static int fat_toupper(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}
static int fat_strncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        int ca = fat_toupper(a[i]), cb = fat_toupper(b[i]);
        if (ca != cb) return ca - cb;
        if (!ca) return 0;
    }
    return 0;
}

static uint64_t cluster_to_lba(uint32_t cluster) {
    return part_lba + data_start_sector + (uint64_t)(cluster - 2) * sectors_per_cluster;
}

static int read_sector(uint64_t lba, void *buf) {
    return ata_read(ata_drive, lba, 1, buf);
}

static int write_sector(uint64_t lba, const void *buf) {
    return ata_write(ata_drive, lba, 1, buf);
}

static uint32_t fat_get_next(uint32_t cluster) {
    uint32_t fat_offset  = cluster * 4;
    uint32_t fat_sector  = reserved_sectors + (fat_offset / bytes_per_sector);
    uint32_t fat_ofs_in  = fat_offset % bytes_per_sector;

    if (read_sector(part_lba + fat_sector, fat_buf) != 0) return 0;
    uint32_t val = *(uint32_t*)(fat_buf + fat_ofs_in);
    return val & 0x0FFFFFFF;
}

static int fat_set(uint32_t cluster, uint32_t value) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = reserved_sectors + (fat_offset / bytes_per_sector);
    uint32_t fat_ofs_in = fat_offset % bytes_per_sector;

    for (uint32_t f = 0; f < num_fats; f++) {
        uint64_t lba = part_lba + fat_sector + f * fat_size;
        if (read_sector(lba, fat_buf) != 0) return -1;
        *(uint32_t*)(fat_buf + fat_ofs_in) = (value & 0x0FFFFFFF);
        if (write_sector(lba, fat_buf) != 0) return -1;
    }
    return 0;
}

static uint32_t fat_alloc_cluster(void) {
    for (uint32_t c = 2; c < cluster_count + 2; c++) {
        uint32_t next = fat_get_next(c);
        if (next == FAT32_FREE) {
            if (fat_set(c, 0x0FFFFFFF) != 0) return 0;
            return c;
        }
    }
    return 0; 
}

int fat32_mount(int drive_index, uint64_t lba_start) {
    mounted   = 0;
    ata_drive = drive_index;
    part_lba  = lba_start;

    if (read_sector(part_lba, sector_buf) != 0) return -1;

    bpb_t *bpb = (bpb_t*)sector_buf;

    if (bpb->bytes_per_sector == 0 || bpb->sectors_per_cluster == 0) return -1;
    if (sector_buf[510] != 0x55 || sector_buf[511] != 0xAA) return -1;
    if (bpb->root_entry_count != 0) return -1;

    bytes_per_sector    = bpb->bytes_per_sector;
    sectors_per_cluster = bpb->sectors_per_cluster;
    reserved_sectors    = bpb->reserved_sectors;
    num_fats            = bpb->num_fats;
    fat_size            = bpb->fat_size_32;
    root_cluster        = bpb->root_cluster;
    total_sectors       = bpb->total_sectors_32 ? bpb->total_sectors_32 : bpb->total_sectors_16;

    data_start_sector = reserved_sectors + num_fats * fat_size;
    cluster_count     = (total_sectors - data_start_sector) / sectors_per_cluster;

    mounted = 1;
    return 0;
}

static void iter_dir(uint32_t start_cluster,
                     int (*cb)(dir_entry_t *e, void *ctx), void *ctx) {
    uint32_t cluster = start_cluster;

    while (cluster >= 2 && cluster < FAT32_EOC) {
        uint64_t lba = cluster_to_lba(cluster);

        for (uint32_t s = 0; s < sectors_per_cluster; s++) {
            if (read_sector(lba + s, sector_buf) != 0) return;

            dir_entry_t *entries = (dir_entry_t*)sector_buf;
            int per_sector = bytes_per_sector / sizeof(dir_entry_t);

            for (int i = 0; i < per_sector; i++) {
                dir_entry_t *e = &entries[i];
                if (e->name[0] == 0x00) return;
                if ((uint8_t)e->name[0] == 0xE5) continue; 
                if (e->attr == ATTR_LFN) continue; 
                if (e->attr & ATTR_VOLUME_ID) continue;
                if (cb(e, ctx)) return;
            }
        }

        cluster = fat_get_next(cluster);
    }
}

static void format_83(const dir_entry_t *e, char *out) {
    int i = 0, j = 0;
    for (j = 7; j >= 0 && e->name[j] == ' '; j--);
    for (int k = 0; k <= j; k++) out[i++] = e->name[k];
    if (e->ext[0] != ' ') {
        out[i++] = '.';
        for (j = 2; j >= 0 && e->ext[j] == ' '; j--);
        for (int k = 0; k <= j; k++) out[i++] = e->ext[k];
    }
    out[i] = '\0';
}

typedef struct { void (*cb)(const char*, uint32_t, int); } ls_ctx_t;

static int ls_cb(dir_entry_t *e, void *ctx) {
    ls_ctx_t *c = (ls_ctx_t*)ctx;
    char name[13];
    format_83(e, name);
    int is_dir = (e->attr & ATTR_DIRECTORY) ? 1 : 0;
    c->cb(name, e->file_size, is_dir);
    return 0;
}

void fat32_ls(void (*callback)(const char *name, uint32_t size, int is_dir)) {
    if (!mounted) return;
    ls_ctx_t ctx = { callback };
    iter_dir(root_cluster, ls_cb, &ctx);
}

typedef struct {
    const char *target;
    void (*cb)(const uint8_t*, uint32_t);
    int found;
    int bytes_read;
} cat_ctx_t;

static int cat_find_cb(dir_entry_t *e, void *ctx) {
    cat_ctx_t *c = (cat_ctx_t*)ctx;
    char name[13];
    format_83(e, name);
    if (fat_strncmp(name, c->target, 12) != 0) return 0;
    if (e->attr & ATTR_DIRECTORY) return 0;

    c->found = 1;
    uint32_t cluster = ((uint32_t)e->cluster_hi << 16) | e->cluster_lo;
    uint32_t remaining = e->file_size;
    c->bytes_read = remaining;

    while (cluster >= 2 && cluster < FAT32_EOC && remaining > 0) {
        uint64_t lba = cluster_to_lba(cluster);
        for (uint32_t s = 0; s < sectors_per_cluster && remaining > 0; s++) {
            if (read_sector(lba + s, sector_buf) != 0) return 1;
            uint32_t chunk = remaining > bytes_per_sector ? bytes_per_sector : remaining;
            c->cb(sector_buf, chunk);
            remaining -= chunk;
        }
        cluster = fat_get_next(cluster);
    }
    return 1;
}

int fat32_cat(const char *filename,
              void (*callback)(const uint8_t *data, uint32_t len)) {
    if (!mounted) return -1;
    cat_ctx_t ctx = { filename, callback, 0, 0 };
    iter_dir(root_cluster, cat_find_cb, &ctx);
    return ctx.found ? ctx.bytes_read : -1;
}

static void to_83(const char *name, char out_name[8], char out_ext[3]) {
    fat_memset(out_name, ' ', 8);
    fat_memset(out_ext,  ' ', 3);

    int dot = -1;
    int len = fat_strlen(name);
    for (int i = 0; i < len; i++) if (name[i] == '.') dot = i;

    int base_len = (dot >= 0) ? dot : len;
    if (base_len > 8) base_len = 8;
    for (int i = 0; i < base_len; i++)
        out_name[i] = (char)fat_toupper(name[i]);

    if (dot >= 0) {
        int ext_len = len - dot - 1;
        if (ext_len > 3) ext_len = 3;
        for (int i = 0; i < ext_len; i++)
            out_ext[i] = (char)fat_toupper(name[dot + 1 + i]);
    }
}

typedef struct {
    char target_name[8];
    char target_ext[3];
    uint64_t entry_lba;
    int      entry_idx;
    int      found;
} find_ctx_t;

typedef struct {
    char name83[8];
    char ext83[3];
    uint64_t out_lba;
    int      out_idx;
    int      found;
    uint32_t first_cluster;
    uint32_t file_size;
} findentry_ctx_t;

static int find_entry(const char *filename,
                      uint64_t *out_lba, int *out_offset,
                      uint32_t *first_cluster, uint32_t *file_size) {
    char n83[8], e83[3];
    to_83(filename, n83, e83);

    uint32_t cluster = root_cluster;
    while (cluster >= 2 && cluster < FAT32_EOC) {
        uint64_t lba = cluster_to_lba(cluster);
        for (uint32_t s = 0; s < sectors_per_cluster; s++) {
            if (read_sector(lba + s, sector_buf) != 0) return 0;
            dir_entry_t *entries = (dir_entry_t*)sector_buf;
            int per_sector = bytes_per_sector / sizeof(dir_entry_t);
            for (int i = 0; i < per_sector; i++) {
                dir_entry_t *e = &entries[i];
                if (e->name[0] == 0x00) return 0;
                if ((uint8_t)e->name[0] == 0xE5) continue;
                if (e->attr == ATTR_LFN) continue;
                if (e->attr & ATTR_VOLUME_ID) continue;
                if (fat_strncmp(e->name, n83, 8) == 0 &&
                    fat_strncmp(e->ext,  e83,  3) == 0) {
                    *out_lba      = lba + s;
                    *out_offset   = i;
                    *first_cluster= ((uint32_t)e->cluster_hi << 16) | e->cluster_lo;
                    *file_size    = e->file_size;
                    return 1;
                }
            }
        }
        cluster = fat_get_next(cluster);
    }
    return 0;
}

static int find_free_slot(uint64_t *out_lba, int *out_offset) {
    uint32_t cluster = root_cluster;
    while (cluster >= 2 && cluster < FAT32_EOC) {
        uint64_t lba = cluster_to_lba(cluster);
        for (uint32_t s = 0; s < sectors_per_cluster; s++) {
            if (read_sector(lba + s, sector_buf) != 0) return -1;
            dir_entry_t *entries = (dir_entry_t*)sector_buf;
            int per_sector = bytes_per_sector / sizeof(dir_entry_t);
            for (int i = 0; i < per_sector; i++) {
                uint8_t first = (uint8_t)entries[i].name[0];
                if (first == 0x00 || first == 0xE5) {
                    *out_lba    = lba + s;
                    *out_offset = i;
                    return 0;
                }
            }
        }
        cluster = fat_get_next(cluster);
    }
    return -1;
}

static void free_chain(uint32_t start) {
    uint32_t c = start;
    while (c >= 2 && c < FAT32_EOC) {
        uint32_t next = fat_get_next(c);
        fat_set(c, FAT32_FREE);
        c = next;
    }
}

#define FAT_DATE_2024 ((2024-1980) << 9 | 1 << 5 | 1)
#define FAT_TIME_0000 0

int fat32_write_file(const char *filename, const uint8_t *data, uint32_t len) {
    if (!mounted) return -1;

    char n83[8], e83[3];
    to_83(filename, n83, e83);

    uint64_t entry_lba;
    int      entry_off;
    uint32_t old_cluster, old_size;
    int exists = find_entry(filename, &entry_lba, &entry_off, &old_cluster, &old_size);

    if (exists && old_cluster >= 2) free_chain(old_cluster);

    uint32_t bytes_per_cluster = bytes_per_sector * sectors_per_cluster;
    uint32_t clusters_needed = (len + bytes_per_cluster - 1) / bytes_per_cluster;
    if (clusters_needed == 0) clusters_needed = 1;

    uint32_t first_cluster = 0;
    uint32_t prev_cluster  = 0;

    for (uint32_t c = 0; c < clusters_needed; c++) {
        uint32_t nc = fat_alloc_cluster();
        if (nc == 0) return -1; 

        if (prev_cluster != 0) {
            fat_set(prev_cluster, nc);
        } else {
            first_cluster = nc;
        }
        prev_cluster = nc;

        uint64_t lba  = cluster_to_lba(nc);
        uint32_t written = c * bytes_per_cluster;

        for (uint32_t s = 0; s < sectors_per_cluster; s++) {
            fat_memset(sector_buf, 0, bytes_per_sector);
            uint32_t sector_off = written + s * bytes_per_sector;
            if (sector_off < len) {
                uint32_t chunk = len - sector_off;
                if (chunk > bytes_per_sector) chunk = bytes_per_sector;
                fat_memcpy(sector_buf, data + sector_off, chunk);
            }
            if (write_sector(lba + s, sector_buf) != 0) return -1;
        }
    }

    dir_entry_t entry;
    fat_memset(&entry, 0, sizeof(entry));
    fat_memcpy(entry.name, n83, 8);
    fat_memcpy(entry.ext,  e83, 3);
    entry.attr        = ATTR_ARCHIVE;
    entry.cluster_hi  = (uint16_t)(first_cluster >> 16);
    entry.cluster_lo  = (uint16_t)(first_cluster & 0xFFFF);
    entry.file_size   = len;
    entry.write_date  = FAT_DATE_2024;
    entry.write_time  = FAT_TIME_0000;
    entry.create_date = FAT_DATE_2024;
    entry.create_time = FAT_TIME_0000;
    entry.access_date = FAT_DATE_2024;

    if (!exists) {
        if (find_free_slot(&entry_lba, &entry_off) != 0) return -1;
    }

    if (read_sector(entry_lba, sector_buf) != 0) return -1;
    dir_entry_t *entries = (dir_entry_t*)sector_buf;
    fat_memcpy(&entries[entry_off], &entry, sizeof(dir_entry_t));
    if (write_sector(entry_lba, sector_buf) != 0) return -1;

    return 0;
}