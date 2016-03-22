/*
 * Hash Table
 */

 #ifndef HASH_H
 #define HASH_H

 typedef struct hashTable {
    unsigned int *keyHashes;
    void **values;
    int numEntries;
 } hashTable_t;

// Create new hash table with numEntries maximum entries
hashTable_t *hashNewTable(int numEntries);
// Get pointer to data associated with keyHash
 void *hashFind(hashTable_t *table, unsigned int keyHash);
 // Add key-pointer pair to table
 void hashAdd(hashTable_t *table, void *ptr, unsigned int keyHash);
// Get hash value for key
unsigned int hashFunction(void *key, int len);
 #endif
