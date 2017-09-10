/*
 * Hash Table
 */

 #ifndef HASH_H
 #define HASH_H

 typedef struct hashTable {
    unsigned int *hashes;
    void **values;
    unsigned int size;
 } hashTable_t;

// Create new hash table able to safely fit numEntries hashes
hashTable_t *hashNewTable(int numEntries);
// Free hash table
void hashDestroyTable(hashTable_t *table);
// Get pointer to data associated with hash
void *hashFind(hashTable_t *table, unsigned int hash);
// Add hash-pointer pair to table
void hashAdd(hashTable_t *table, void *ptr, unsigned int hash);
// Get hash value for key
unsigned int hashFunction(void *key, int len);

#endif
