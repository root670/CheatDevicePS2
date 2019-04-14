#include "hash.h"
#include <stdio.h>
#include <tamtypes.h>
#include <floatlib.h>

hashTable_t *hashNewTable(int numEntries)
{
    hashTable_t *table;

    table = calloc(1, sizeof(hashTable_t));
    table->size = ceilf(numEntries * 1.6);
    table->hashes = calloc(table->size, sizeof(unsigned int));
    table->values = calloc(table->size, sizeof(void*));

    return table;
}

void hashDestroyTable(hashTable_t *table)
{
    if(!table)
        return;

    if(table->hashes)
        free(table->hashes);
    if(table->values)
        free(table->values);

    free(table);
}

// http://www.eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
unsigned int hashFunction(const void *key, int len)
{
    const unsigned char *p = key;
    unsigned int h = 0;
    int i;

    if(!key)
        return 0;

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

void *hashFind(hashTable_t *table, unsigned int hash)
{
    unsigned int i;

    if(!table)
        return NULL;

    i = hash % table->size;

    while(table->hashes[i] != hash)
    {
        if(table->hashes[i] == 0) // not in table
            return NULL;
        else if(i == table->size - 1) // last entry in table
            i = 0;
        else // try next entry
            i++;
    }

    return table->values[i];
}

void hashAdd(hashTable_t *table, void *ptr, unsigned int hash)
{
    unsigned int i;

    i = hash % table->size;

    while(table->hashes[i] != 0)
    {
        if(i == table->size - 1) // last entry in table
            i = 0;
        else // try next entry
            i++;
    }

    table->hashes[i] = hash;
    table->values[i] = ptr;
}
