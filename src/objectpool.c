#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "objectpool.h"
#include "cheats.h"

#define POOLSIZE_GAME     MAX_GAMES * sizeof(cheatsGame_t)
#define POOLSIZE_CHEAT    MAX_CHEATS * sizeof(cheatsCheat_t)

static int initialized = 0;
static unsigned char gameMemory[POOLSIZE_GAME];
static unsigned char cheatMemory[POOLSIZE_CHEAT];

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
    pools[OBJECTPOOLTYPE_GAME].memory = gameMemory;
    pools[OBJECTPOOLTYPE_GAME].tail = pools[OBJECTPOOLTYPE_GAME].memory;
    pools[OBJECTPOOLTYPE_GAME].freeList = NULL;
    memset(gameMemory, 0, POOLSIZE_GAME);

    // Initialize cheat object pool
    pools[OBJECTPOOLTYPE_CHEAT].capacity = POOLSIZE_CHEAT;
    pools[OBJECTPOOLTYPE_CHEAT].objectSize = sizeof(cheatsCheat_t);
    pools[OBJECTPOOLTYPE_CHEAT].memory = cheatMemory;
    pools[OBJECTPOOLTYPE_CHEAT].tail = pools[OBJECTPOOLTYPE_CHEAT].memory;
    pools[OBJECTPOOLTYPE_CHEAT].freeList = NULL;
    memset(cheatMemory, 0, POOLSIZE_CHEAT);

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
        
        printf("objectPoolRelease: Adding %p to freelist\n", ptr);
        freeList_t *temp = malloc(sizeof(freeList_t));
        temp->ptr = ptr;
        temp->next = pools[type].freeList;
        pools[type].freeList = temp;
    }

    return 1;
}

int objectPoolKill()
{
    if(!initialized)
        return 0;

    while(pools[OBJECTPOOLTYPE_GAME].freeList != NULL)
    {
        freeList_t *next = pools[OBJECTPOOLTYPE_GAME].freeList->next;
        free(pools[OBJECTPOOLTYPE_GAME].freeList);
        pools[OBJECTPOOLTYPE_GAME].freeList = next;
    }
    
    while(pools[OBJECTPOOLTYPE_CHEAT].freeList != NULL)
    {
        freeList_t *next = pools[OBJECTPOOLTYPE_CHEAT].freeList->next;
        free(pools[OBJECTPOOLTYPE_CHEAT].freeList);
        pools[OBJECTPOOLTYPE_CHEAT].freeList = next;
    }

    memset(pools, 0, sizeof(objectPool_t) * 2);

    initialized = 0;
    return 1;
}
