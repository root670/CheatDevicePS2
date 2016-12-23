/*
 * Utilities
 * Miscellaneous helper functions.
 */
#ifndef UTIL_H
#define UTIL_H

// Reset the IOP and load initial modules.
void loadModules();

// Change the menu state in response to gamepad input.
void handlePad();

// Draw a simple text menu, handle gamepad input, then return the index of the chosen menu item.
int displayPromptMenu(char **items, int numItems, const char *header);
// Draw an error message in a promp box.
int displayError(const char *error);
// Draw menu of options related to current menu
void displayContextMenu(int menuID);
// Get user input for a string
int displayInputMenu(char *dstStr, int dstLen, const char *initialStr, const char *prompt);

// Replace illegal (reserved) characters in str with replacement. valid must point to a char array as large as str.
void replaceIllegalChars(const char *str, char* valid, char replacement);
// Remove trailing whitespace from str.
char *rtrim(char *str);

unsigned long mycrc32(unsigned long inCrc32, const void *buf, long bufLen);

#endif
