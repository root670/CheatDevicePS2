#include "objectpool.h"
#include "cheats.h"
#include <stdlib.h>
#include <stdio.h>

#define POOLSIZE_GAME     MAX_GAMES * sizeof(cheatsGame_t)
#define POOLSIZE_CHEAT    MAX_CHEATS * sizeof(cheatsCheat_t)
#define POOLSIZE_CODE     MAX_CODES * sizeof(u64)

static int initialized = 0;

typedef struct freeList {
    void *ptr;
    struct freeList *next;
} freeList_t;

struct objectPool {
    unsigned int capacity;
    unsigned int objectSize;
    void *memory;
    void *tail;
    freeList_t *freeList;
};

static objectPool_t pools[2];

int initialize()
{
    // Initialize game object pool
    pools[OBJECTPOOLTYPE_GAME].capacity = POOLSIZE_GAME;
    pools[OBJECTPOOLTYPE_GAME].objectSize = sizeof(cheatsGame_t);
    pools[OBJECTPOOLTYPE_GAME].memory = calloc(1, POOLSIZE_GAME);
    if(!pools[OBJECTPOOLTYPE_GAME].memory)
    {
        printf("objectPool initialization failed (cheatsGame_t)!\n");
        return 0;
    }
    pools[OBJECTPOOLTYPE_GAME].tail = pools[OBJECTPOOLTYPE_GAME].memory;
    pools[OBJECTPOOLTYPE_GAME].freeList = NULL;

    // Initialize cheat object pool
    pools[OBJECTPOOLTYPE_CHEAT].capacity = POOLSIZE_CHEAT;
    pools[OBJECTPOOLTYPE_CHEAT].objectSize = sizeof(cheatsCheat_t);
    pools[OBJECTPOOLTYPE_CHEAT].memory = calloc(1, POOLSIZE_CHEAT);
    if(!pools[OBJECTPOOLTYPE_CHEAT].memory)
    {
        printf("objectPool initialization failed (cheatsCheat_t)!\n");
        return 0;
    }
    pools[OBJECTPOOLTYPE_CHEAT].tail = pools[OBJECTPOOLTYPE_CHEAT].memory;
    pools[OBJECTPOOLTYPE_CHEAT].freeList = NULL;

    initialized = 1;
    return 1;
}

void* objectPoolAllocate(objectPoolType_t type)
{
    void *ret;

    if(!initialized)
    {
        if(!initialize())
            return NULL;
    }

    if(type != OBJECTPOOLTYPE_GAME && type != OBJECTPOOLTYPE_CHEAT)
        return NULL;

    if(pools[type].freeList != NULL)
    {
        // Get memory from freeList
        ret = pools[type].freeList->ptr;

        freeList_t *next = pools[type].freeList->next;
        free(pools[type].freeList);
        pools[type].freeList = next;
    }
    else if(pools[type].tail < (pools[type].memory + pools[type].capacity))
    {
        // Get memory from tail
        ret = pools[type].tail;
        pools[type].tail += pools[type].objectSize;
    }
    else
    {
        // Pool is full!
        ret = NULL;
    }

    return ret;
}

int objectPoolRelease(objectPoolType_t type, void *ptr)
{
    if(!initialized)
        return 0;

    if(type != OBJECTPOOLTYPE_GAME && type != OBJECTPOOLTYPE_CHEAT)
        return 0;

    if((ptr >= pools[type].memory) && (ptr < pools[type].tail))
    {
        memset(ptr, 0, pools[type].objectSize);
        
        printf("objectPoolRelease: Adding %08X to freelist\n", ptr);
        freeList_t *temp = malloc(sizeof(freeList_t));
        temp->ptr = ptr;
        temp->next = pools[type].freeList;
        pools[type].freeList = temp;
    }

    return 1;
}