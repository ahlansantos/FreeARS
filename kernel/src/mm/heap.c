#include "heap.h"
#include "pmm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Cada bloco tem um header antes dos dados do usuário
typedef struct block {
    size_t        size;  // tamanho dos dados (sem o header)
    bool          used;
    struct block *next;
} block_t;

static block_t *free_list = NULL;

// Aloca uma nova página e a converte para endereço virtual via HHDM
static block_t *heap_new_page(void) {
    void *phys = pmm_alloc_page();
    if (!phys) return NULL;

    // pmm_alloc_page() retorna endereço físico —
    // precisamos do virtual para poder escrever nele
    block_t *blk = (block_t *)pmm_phys_to_virt((uint64_t)phys);
    blk->size = PAGE_SIZE - sizeof(block_t);
    blk->used = false;
    blk->next = NULL;
    return blk;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Alinha para 8 bytes
    size = (size + 7) & ~(size_t)7;

    // Inicializa a heap na primeira chamada
    if (!free_list) {
        free_list = heap_new_page();
        if (!free_list) return NULL;
    }

    // Procura um bloco livre grande o suficiente (first-fit)
    block_t *prev = NULL;
    block_t *curr = free_list;

    while (curr) {
        if (!curr->used && curr->size >= size) {
            // Tenta dividir o bloco se sobrar espaço para outro header + dados
            if (curr->size >= size + sizeof(block_t) + 8) {
                block_t *split = (block_t *)((uint8_t *)curr + sizeof(block_t) + size);
                split->size = curr->size - size - sizeof(block_t);
                split->used = false;
                split->next = curr->next;

                curr->size = size;
                curr->next = split;
            }

            curr->used = true;
            return (void *)((uint8_t *)curr + sizeof(block_t));
        }
        prev = curr;
        curr = curr->next;
    }

    // Sem bloco disponível — aloca nova página
    block_t *new_blk = heap_new_page();
    if (!new_blk) return NULL;

    if (prev) prev->next = new_blk;
    else      free_list  = new_blk;

    // Divide se possível
    if (new_blk->size >= size + sizeof(block_t) + 8) {
        block_t *split = (block_t *)((uint8_t *)new_blk + sizeof(block_t) + size);
        split->size = new_blk->size - size - sizeof(block_t);
        split->used = false;
        split->next = new_blk->next;

        new_blk->size = size;
        new_blk->next = split;
    }

    new_blk->used = true;
    return (void *)((uint8_t *)new_blk + sizeof(block_t));
}

void kfree(void *ptr) {
    if (!ptr) return;

    block_t *blk = (block_t *)((uint8_t *)ptr - sizeof(block_t));
    blk->used = false;

    // Coalescência: junta blocos livres contíguos (evita fragmentação)
    block_t *curr = free_list;
    while (curr && curr->next) {
        if (!curr->used && !curr->next->used) {
            curr->size += sizeof(block_t) + curr->next->size;
            curr->next  = curr->next->next;
            // Não avança — tenta fundir de novo com o novo próximo
        } else {
            curr = curr->next;
        }
    }
}