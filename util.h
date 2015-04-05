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

unsigned long crc32(unsigned long inCrc32, const void *buf, long bufLen);

#endif
