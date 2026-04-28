#include "kmalloc.h"
#define HEAP_START 0x200000
#define HEAP_SIZE 0x100000
#define MAGIC 0xAB
typedef struct block{uint32_t size;uint8_t free,magic;struct block*next;}block_t;
static block_t*heap=(block_t*)HEAP_START;
void kmalloc_init(){heap->size=HEAP_SIZE-sizeof(block_t);heap->free=1;heap->magic=MAGIC;heap->next=0;}
void*kmalloc(uint32_t size){
    block_t*cur=heap;
    while(cur){
        if(cur->free&&cur->magic==MAGIC&&cur->size>=size){
            if(cur->size>size+sizeof(block_t)+4){
                block_t*s=(block_t*)((uint8_t*)cur+sizeof(block_t)+size);
                s->size=cur->size-size-sizeof(block_t);s->free=1;s->magic=MAGIC;s->next=cur->next;
                cur->next=s;cur->size=size;
            }
            cur->free=0;return(void*)((uint8_t*)cur+sizeof(block_t));
        }
        cur=cur->next;
    }
    return 0;
}
void kfree(void*ptr){
    if(!ptr)return;
    block_t*cur=(block_t*)((uint8_t*)ptr-sizeof(block_t));
    if(cur->magic!=MAGIC)return;
    cur->free=1;
    while(cur->next&&cur->next->free&&cur->next->magic==MAGIC){
        cur->size+=sizeof(block_t)+cur->next->size;cur->next=cur->next->next;
    }
}