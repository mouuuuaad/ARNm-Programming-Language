/*
 * ARNm Runtime - Memory Management (ARC)
 */

#ifndef ARNM_MEMORY_H
#define ARNM_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

/* ============================================================
 * Object Header
 * ============================================================
 * Every ARC-managed object has this header.
 */

typedef void (*ArnmDestructor)(void*);

typedef struct {
    atomic_uint_fast32_t    refcount;   /* Reference count */
    ArnmDestructor          dtor;       /* Destructor function */
    size_t                  size;       /* Object size */
} ArnmObjectHeader;

/* ============================================================
 * ARC API
 * ============================================================ */

/* Allocate a new ARC-managed object */
void* arnm_arc_alloc(size_t size, ArnmDestructor dtor);

/* Increment reference count */
void arnm_arc_retain(void* obj);

/* Decrement reference count (frees if zero) */
void arnm_arc_release(void* obj);

/* Get current reference count */
uint32_t arnm_arc_refcount(void* obj);

/* ============================================================
 * Memory Pool (for small allocations)
 * ============================================================ */

typedef struct MemoryPool MemoryPool;

/* Create memory pool with given block size */
MemoryPool* pool_create(size_t block_size, size_t initial_blocks);

/* Destroy pool and free all blocks */
void pool_destroy(MemoryPool* pool);

/* Allocate from pool */
void* pool_alloc(MemoryPool* pool);

/* Return to pool */
void pool_free(MemoryPool* pool, void* ptr);

/* ============================================================
 * Stack Allocation
 * ============================================================ */

/* Allocate a process stack with guard pages */
void* stack_alloc(size_t size);

/* Free a process stack */
void stack_free(void* stack, size_t size);

#endif /* ARNM_MEMORY_H */
