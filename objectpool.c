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

void initialize()
{
    // Initialize cheatsGame_t pool
    pools[OBJECTPOOLTYPE_GAME].capacity = POOLSIZE_GAME;
    pools[OBJECTPOOLTYPE_GAME].objectSize = sizeof(cheatsGame_t);
    pools[OBJECTPOOLTYPE_GAME].memory = calloc(1, POOLSIZE_GAME);
    if(!pools[OBJECTPOOLTYPE_GAME].memory)
    {
        printf("objectPool initialization failed!\n");
        return;
    }
    pools[OBJECTPOOLTYPE_GAME].tail = pools[OBJECTPOOLTYPE_GAME].memory;
    pools[OBJECTPOOLTYPE_GAME].freeList = NULL;

    // Initialize cheatsCheat_t pool
    pools[OBJECTPOOLTYPE_CHEAT].capacity = POOLSIZE_GAME;
    pools[OBJECTPOOLTYPE_CHEAT].objectSize = sizeof(cheatsGame_t);
    pools[OBJECTPOOLTYPE_CHEAT].memory = calloc(1, POOLSIZE_GAME);
    if(!pools[OBJECTPOOLTYPE_CHEAT].memory)
    {
        printf("objectPool initialization failed!\n");
        return;
    }
    pools[OBJECTPOOLTYPE_CHEAT].tail = pools[OBJECTPOOLTYPE_CHEAT].memory;
    pools[OBJECTPOOLTYPE_CHEAT].freeList = NULL;

    initialized = 1;
}

void* objectPoolAllocate(objectPoolType_t type)
{
    if(!initialized)
        initialize();

    if(type != OBJECTPOOLTYPE_GAME && type != OBJECTPOOLTYPE_CHEAT)
        return NULL;

    if(pools[type].freeList != NULL)
    {
        printf("objectPoolAllocate: Getting %08X from freeList\n", pools[type].freeList->ptr);
        void *ret = pools[type].freeList->ptr;
        freeList_t *next = pools[type].freeList->next;
        free(pools[type].freeList);
        pools[type].freeList = next;
        return ret;
    }
    else if(pools[type].tail <= (pools[type].memory + pools[type].capacity))
    {
        printf("objectPoolAllocate: Getting %08X from tail\n", pools[type].tail);
        void *ret = pools[type].tail;
        pools[type].tail++;
        return ret;
    }
    else
    {
        printf("objectPoolAllocate: Pool is full!!!");
        return NULL;
    }
}

int objectPoolRelease(objectPoolType_t type, void *ptr)
{
    if(!initialized)
        return 0;

    if(type != OBJECTPOOLTYPE_GAME && type != OBJECTPOOLTYPE_CHEAT)
        return 0;

    if((ptr >= pools[type].memory) && (pools[type].tail < ptr))
    {
        printf("objectPoolRelease: Adding %08X to freelist\n", ptr);
        freeList_t *temp = malloc(sizeof(freeList_t));
        temp->ptr = ptr;
        temp->next = pools[type].freeList;
        pools[type].freeList = temp;
    }

    return 1;
}