/*
 * ARNm Runtime - Memory Management
 */

#include "../include/memory.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* ============================================================
 * ARC Implementation
 * ============================================================ */

/* Get header from object pointer */
static inline ArnmObjectHeader* get_header(void* obj) {
    return (ArnmObjectHeader*)((char*)obj - sizeof(ArnmObjectHeader));
}

/* Get object from header pointer */
static inline void* get_object(ArnmObjectHeader* header) {
    return (char*)header + sizeof(ArnmObjectHeader);
}

void* arnm_arc_alloc(size_t size, ArnmDestructor dtor) {
    size_t total = sizeof(ArnmObjectHeader) + size;
    
    ArnmObjectHeader* header = (ArnmObjectHeader*)malloc(total);
    if (!header) return NULL;
    
    atomic_init(&header->refcount, 1);
    header->dtor = dtor;
    header->size = size;
    
    void* obj = get_object(header);
    memset(obj, 0, size);
    return obj;
}

void arnm_arc_retain(void* obj) {
    if (!obj) return;
    
    ArnmObjectHeader* header = get_header(obj);
    atomic_fetch_add(&header->refcount, 1);
}

void arnm_arc_release(void* obj) {
    if (!obj) return;
    
    ArnmObjectHeader* header = get_header(obj);
    uint32_t old = atomic_fetch_sub(&header->refcount, 1);
    
    if (old == 1) {
        /* Refcount hit zero - destroy object */
        if (header->dtor) {
            header->dtor(obj);
        }
        free(header);
    }
}

uint32_t arnm_arc_refcount(void* obj) {
    if (!obj) return 0;
    
    ArnmObjectHeader* header = get_header(obj);
    return atomic_load(&header->refcount);
}

/* ============================================================
 * Memory Pool Implementation
 * ============================================================ */

typedef struct PoolBlock {
    struct PoolBlock* next;
} PoolBlock;

struct MemoryPool {
    PoolBlock*  free_list;
    size_t      block_size;
    size_t      allocated;
};

MemoryPool* pool_create(size_t block_size, size_t initial_blocks) {
    /* Ensure block size is at least pointer-sized */
    if (block_size < sizeof(PoolBlock)) {
        block_size = sizeof(PoolBlock);
    }
    
    MemoryPool* pool = (MemoryPool*)malloc(sizeof(MemoryPool));
    if (!pool) return NULL;
    
    pool->free_list = NULL;
    pool->block_size = block_size;
    pool->allocated = 0;
    
    /* Pre-allocate initial blocks */
    for (size_t i = 0; i < initial_blocks; i++) {
        PoolBlock* block = (PoolBlock*)malloc(block_size);
        if (block) {
            block->next = pool->free_list;
            pool->free_list = block;
        }
    }
    
    return pool;
}

void pool_destroy(MemoryPool* pool) {
    if (!pool) return;
    
    PoolBlock* block = pool->free_list;
    while (block) {
        PoolBlock* next = block->next;
        free(block);
        block = next;
    }
    
    free(pool);
}

void* pool_alloc(MemoryPool* pool) {
    if (!pool) return NULL;
    
    if (pool->free_list) {
        PoolBlock* block = pool->free_list;
        pool->free_list = block->next;
        pool->allocated++;
        return block;
    }
    
    /* Allocate new block */
    void* ptr = malloc(pool->block_size);
    if (ptr) {
        pool->allocated++;
    }
    return ptr;
}

void pool_free(MemoryPool* pool, void* ptr) {
    if (!pool || !ptr) return;
    
    PoolBlock* block = (PoolBlock*)ptr;
    block->next = pool->free_list;
    pool->free_list = block;
    pool->allocated--;
}

/* ============================================================
 * Stack Allocation
 * ============================================================ */

#define PAGE_SIZE 4096

void* stack_alloc(size_t size) {
    /* Round up to page size */
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    /* Add guard page */
    size_t total_size = size + PAGE_SIZE;
    
    void* mem = mmap(NULL, total_size, 
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1, 0);
    
    if (mem == MAP_FAILED) {
        return NULL;
    }
    
    /* Make the first page a guard page (no access) */
    mprotect(mem, PAGE_SIZE, PROT_NONE);
    
    /* Return pointer to usable stack area (above guard page) */
    return (char*)mem + PAGE_SIZE;
}

void stack_free(void* stack, size_t size) {
    if (!stack) return;
    
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    size_t total_size = size + PAGE_SIZE;
    
    /* Get original mmap base (before guard page) */
    void* base = (char*)stack - PAGE_SIZE;
    munmap(base, total_size);
}
