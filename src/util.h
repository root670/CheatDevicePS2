/*
 * Utilities
 * Miscellaneous helper functions.
 */
 
#ifndef UTIL_H
#define UTIL_H

#include <tamtypes.h>

#define READ_8(address) \
*((unsigned char *)address)

#define READ_16(address) ( \
*((unsigned char *)address + 0) | \
(*((unsigned char *)address + 1) << 8))

#define READ_32(address) ( \
(*((unsigned char *)address + 0) << 0) | \
(*((unsigned char *)address + 1) << 8) | \
(*((unsigned char *)address + 2) << 16) | \
(*((unsigned char *)address + 3) << 24))

#define WRITE_8(address, value) \
*((unsigned char *)address) = (value & 0xff);

#define WRITE_16(address, value) \
*((unsigned char *)address + 0) = (value & 0x00ff); \
*((unsigned char *)address + 1) = (value & 0xff00) >> 8;

#define WRITE_32(address, value) \
*((unsigned char *)address + 0) = (value & 0x000000ff); \
*((unsigned char *)address + 1) = (value & 0x0000ff00) >> 8; \
*((unsigned char *)address + 2) = (value & 0x00ff0000) >> 16; \
*((unsigned char *)address + 3) = (value & 0xff000000) >> 24;

// Reset the IOP and load initial modules.
void loadModules();

// Change the menu state in response to gamepad input.
void handlePad();

// Draw a simple text menu and handle gamepad input.
// Returns the index of the chosen menu item or -1 if the user exited the menu by pressing circle.
int displayPromptMenu(const char **items, int numItems, const char *header);
// Draw an error message in a prompt box.
int displayError(const char *error);
// Get user input for a string
int displayTextEditMenu(char *dstStr, int dstLen, const char *initialStr, const char *prompt);
// Get user input for an existing code line.
int displayExistingCodeEditMenu(u64 *code);
// Get user input for a new code line
int displayNewCodeEditMenu(u64 *code);

// Get number of lines in str.
int getNumLines(const char *str);
// Replace illegal (reserved) characters in str with replacement. valid must point to a char array as large as str.
void replaceIllegalChars(const char *str, char* valid, char replacement);
// Remove trailing whitespace from str.
char* rtrim(char *str);
// Get file extension from filename. Returns null if filename doesn't have an extension.
const char* getFileExtension(const char *filename);
// Get file path without extension.
char* getFileBasename(const char *filename);

unsigned long mycrc32(unsigned long inCrc32, const void *buf, long bufLen);

#endif
