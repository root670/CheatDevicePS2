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

// Get pointer to data associated with hash. Returns NULL if the hash doesn't
// exist in the table.
void *hashFind(hashTable_t *table, unsigned int hash);
// Add hash-pointer pair to table
void hashAdd(hashTable_t *table, void *ptr, unsigned int hash);

// Get value associated with hash. Returns -1 if the hash doesn't exist in the
// table.
int hashFindValue(hashTable_t *table, unsigned int hash);
// Add hash-value pair to table
void hashAddValue(hashTable_t *table, int value, unsigned int hash);

// Get hash value for key
unsigned int hashFunction(const void *key, int len);

#endif
