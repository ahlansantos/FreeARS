#include "pmm.h"
#include <stddef.h>
#include <stdint.h>

static uint64_t hhdm_off    = 0;
static uint8_t *bitmap      = NULL;
static uint64_t total_pages = 0;
static uint64_t free_pages  = 0;

// ---------- helpers do bitmap ----------

static inline void bm_set(uint64_t page)   { bitmap[page / 8] |=  (1u << (page % 8)); }
static inline void bm_clear(uint64_t page) { bitmap[page / 8] &= ~(1u << (page % 8)); }
static inline int  bm_get(uint64_t page)   { return (bitmap[page / 8] >> (page % 8)) & 1; }

// ---------- serial debug (sem depender do main.c) ----------

static void pmm_serial_print(const char *s) {
    for (int i = 0; s[i]; i++) {
        uint8_t lsr;
        do { asm volatile("inb %1,%0" : "=a"(lsr) : "Nd"((uint16_t)(0x3F8 + 5))); } while (!(lsr & 0x20));
        asm volatile("outb %0,%1" :: "a"((uint8_t)s[i]), "Nd"((uint16_t)0x3F8));
    }
}
static void pmm_serial_hex(uint64_t n) {
    const char *h = "0123456789ABCDEF";
    char buf[19]; buf[0] = '0'; buf[1] = 'x';
    for (int i = 0; i < 16; i++) buf[2 + i] = h[(n >> (60 - i * 4)) & 0xF];
    buf[18] = 0;
    pmm_serial_print(buf);
}

// ---------- API pública ----------

uint64_t pmm_phys_to_virt(uint64_t phys) {
    return phys + hhdm_off;
}

void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_offset) {
    hhdm_off = hhdm_offset;

    // 1. Descobre o endereço físico mais alto entre regiões USABLE.
    //    Ignorar reserved/MMIO evita bitmap gigante por causa de buracos de PCI.
    pmm_serial_print("=== memmap ===\n");
    uint64_t highest = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        pmm_serial_print("  base="); pmm_serial_hex(e->base);
        pmm_serial_print(" len=");   pmm_serial_hex(e->length);
        pmm_serial_print(" type=");  pmm_serial_hex(e->type);
        pmm_serial_print("\n");
        if (e->type == LIMINE_MEMMAP_USABLE) {
            uint64_t end = e->base + e->length;
            if (end > highest) highest = end;
        }
    }
    pmm_serial_print("  highest_usable="); pmm_serial_hex(highest); pmm_serial_print("\n");

    total_pages = highest / PAGE_SIZE;
    uint64_t bitmap_bytes = (total_pages + 7) / 8;

    pmm_serial_print("  total_pages="); pmm_serial_hex(total_pages);
    pmm_serial_print(" bitmap_bytes="); pmm_serial_hex(bitmap_bytes);
    pmm_serial_print("\n");

    // 2. Acha a primeira região USABLE >= 1MB grande o suficiente pro bitmap.
    bitmap = NULL;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE
                && e->base >= 0x100000
                && e->length >= bitmap_bytes) {
            bitmap = (uint8_t *)(e->base + hhdm_off);
            pmm_serial_print("  bitmap -> phys="); pmm_serial_hex(e->base);
            pmm_serial_print(" bytes="); pmm_serial_hex(bitmap_bytes);
            pmm_serial_print("\n");
            break;
        }
    }

    if (!bitmap) {
        pmm_serial_print("  ERROR: bitmap NULL, nenhuma regiao adequada!\n");
        return;
    }

    // 3. Marca tudo como ocupado (bit 1 = ocupado)
    for (uint64_t i = 0; i < total_pages; i++)
        bm_set(i);

    // 4. Libera apenas as páginas USABLE do memmap
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE) continue;
        uint64_t start = e->base / PAGE_SIZE;
        uint64_t count = e->length / PAGE_SIZE;
        for (uint64_t p = 0; p < count; p++) {
            bm_clear(start + p);
            free_pages++;
        }
    }

    // 5. Reserva as páginas que o próprio bitmap ocupa
    uint64_t bm_phys  = (uint64_t)bitmap - hhdm_off;
    uint64_t bm_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = 0; p < bm_pages; p++) {
        uint64_t pg = bm_phys / PAGE_SIZE + p;
        if (!bm_get(pg)) {
            bm_set(pg);
            free_pages--;
        }
    }

    pmm_serial_print("  free_pages="); pmm_serial_hex(free_pages); pmm_serial_print("\n");
}

// Retorna endereço FÍSICO — use pmm_phys_to_virt() antes de escrever.
// Começa em 1MB para nunca retornar páginas de firmware/BIOS.
void *pmm_alloc_page(void) {
    uint64_t start = 0x100000 / PAGE_SIZE;
    for (uint64_t i = start; i < total_pages; i++) {
        if (!bm_get(i)) {
            bm_set(i);
            free_pages--;
            return (void *)(i * PAGE_SIZE);
        }
    }
    return NULL; // OOM
}

void pmm_free_page(void *phys) {
    uint64_t page = (uint64_t)phys / PAGE_SIZE;
    if (page < total_pages && bm_get(page)) {
        bm_clear(page);
        free_pages++;
    }
}

uint64_t pmm_get_free_page_count(void) {
    return free_pages;
}