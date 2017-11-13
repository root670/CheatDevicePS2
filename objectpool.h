#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

/*
 * Object Pool Allocator
 * 
 * Allocate from pools of memory for specific object types.
 * This works well for allocating many objects of the same type with
 * the assumption that we can enforce a maximum amount for each object type
 * and that few of the objects will be deleted. This allows for fast
 * allocation, easy bookkeeping, and minimal fragmentation of the pool's memory.
 */

#include "cheats.h"

typedef enum {
    OBJECTPOOLTYPE_GAME,
    OBJECTPOOLTYPE_CHEAT
} objectPoolType_t;

typedef struct objectPool objectPool_t;

// Returns pointer for storing an object
void* objectPoolAllocate(objectPoolType_t type);
// Releases memory associated with an object
int objectPoolRelease(objectPoolType_t type, void *ptr);

#endif // OBJECTPOOL_H
