#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

/*
 * Object Pool Allocator
 * 
 * Allocate from pools of memory for specific object types.
 * Good for allocating many objects of the same type with
 * the assumption that we can enforce a maximum amount for each type
 * and that few of the objects will be deleted. This allows for fast
 * allocation, easy bookkeeping, and zero fragmentation of the pool's memory.
 */

#include "cheats.h"

typedef enum {
    OBJECTPOOLTYPE_GAME,
    OBJECTPOOLTYPE_CHEAT
} objectPoolType_t;

typedef struct objectPool objectPool_t;

/*
 * ========== Initialize ==========
 *      allocate memory
 *      tail = memory
 *      set capacity
 *      set objectSize
 * 
 * 
 * ========== Allocate ==========
 * If the freeList is not empty:
 *      Return first freeList->ptr;
 *      increment freeList
 *      free old freelist pointer
 * Else if(tail <= memory + (capacity * objectSize))
 *      Return tail
 *      tail++
 * Else
 *      return NULL (failure)
 * 
 * 
 * ========== Release ==========
 * If memory <= ptr <= tail
 *      allocate new freelist
 *      newFreeList->ptr = ptr
 *      newFreeList->next = freeList
 *      freeList = newFreeList
 * Else
 *      return 0 (failure)
 */

// Returns pointer for storing an object
void* objectPoolAllocate(objectPoolType_t type);
// Releases memory associated with an object
int objectPoolRelease(objectPoolType_t type, void *ptr);

#endif // OBJECTPOOL_H
