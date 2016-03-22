#include <stdio.h>
#include <tamtypes.h>
#include "hash.h"

hashTable_t *hashNewTable(int numEntries)
{
    hashTable_t *table;

    table = calloc(1, sizeof(hashTable_t));
    table->keyHashes = calloc(numEntries, sizeof(unsigned int));
    table->values = calloc(numEntries, sizeof(void*));
    table->numEntries = numEntries;

    return table;
}

// http://www.eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
unsigned int hashFunction(void *key, int len)
{
    unsigned char *p = key;
    unsigned int h = 0;
    int i;

    for (i = 0; i < len; i++)
    {
        h += p[i];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h;
}

void *hashFind(hashTable_t *table, unsigned int keyHash)
{
    unsigned int i;

    i = keyHash % table->numEntries;

    while(table->keyHashes[i] != keyHash)
    {
        if(table->keyHashes[i] == 0) // not in table
            return NULL;
        else if(i == table->numEntries - 1) // last entry in table
            i = 0;
        else // try next entry
            i++;
    }

    return table->values[i];
}

void hashAdd(hashTable_t *table, void *ptr, unsigned int keyHash)
{
    unsigned int i;

    i = keyHash % table->numEntries;

    while(table->keyHashes[i] != 0)
    {
        if(i == table->numEntries - 1) // last entry in table
            i = 0;
        else // try next entry
            i++;
    }

    table->keyHashes[i] = keyHash;
    table->values[i] = ptr;
}
