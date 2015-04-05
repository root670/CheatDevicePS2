/*
 * Storage Manager
 * Simple storage abstraction supporting mc, disc, and mass. Up to 128 files can
 * be opened at a time. Files must be opened using storageOpenFile() before
 * calling storageGetFileSize() or storageGetBinaryFileContents().
 */

#ifndef STORAGE_H
#define STORAGE_H
#include <tamtypes.h>

// Load modules needed for using the storage manager.
int initStorageMan();
// TODO: Close all open file handles, free resources, etc.
int killStorageMan();

// TODO: Check if internal harddrive is availible.
int storageHDDIsPresent();
// TODO: Check if Cheat Device's partition is installed.
int storageHDDPartitionPresent();
// TODO: Install Cheat Device's harddrive partition.
int storageHDDCreatePartition();

// Open file to be used by storage manager.
int storageOpenFile(const char* path, const char* mode);
// Close a file used by storage manager
int storageCloseFile(const char* path);

// Get file size of file opened by storage manager.
int storageGetFileSize(const char* path);

// Get the contents of a file opened by storage manager. Must pass an allocated
// buffer at least as large as the file. IMPORTANT: buffer must be freed by
// caller!
int storageGetBinaryFileContents(const char* path, u8 *buff);
int storageGetTextFileContents(const char* path, char *buff);

#endif
