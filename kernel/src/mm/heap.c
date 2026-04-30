#include "heap.h"
#include "pmm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct block {
    size_t size;
    bool used;
    struct block* next;
} block_t;

static block_t* free_list = NULL;

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = (size + 7) & ~7;
    
    if (!free_list) {
        void* page = pmm_alloc_page();
        if (!page) return NULL;
        free_list = page;
        free_list->size = 4096 - sizeof(block_t);
        free_list->used = false;
        free_list->next = NULL;
    }
    
    block_t* prev = NULL;
    block_t* curr = free_list;
    
    while (curr) {
        if (!curr->used && curr->size >= size) {
            curr->used = true;
            return (void*)((uint8_t*)curr + sizeof(block_t));
        }
        prev = curr;
        curr = curr->next;
    }
    
    void* page = pmm_alloc_page();
    if (!page) return NULL;
    
    block_t* new_block = page;
    new_block->size = 4096 - sizeof(block_t);
    new_block->used = true;
    new_block->next = NULL;
    
    if (prev) prev->next = new_block;
    else free_list = new_block;
    
    return (void*)((uint8_t*)new_block + sizeof(block_t));
}

void kfree(void* ptr) {
    if (!ptr) return;
    block_t* block = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    block->used = false;
}