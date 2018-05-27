/*
 * Settings Manager
 * Reads settings from an INI file. All settings are stored in a file named
 * "CheatDevicePS2.ini", which must be in the same folder as the Cheat Device
 * ELF.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

// Open settings file and read settings. Fallback values are used if file was
// not found or is missing entries.
int initSettings();
// Free allocated strings.
int killSettings();

// Save setting file with current settings.
int settingsSave();

// Get database path string.
char* settingsGetDatabasePath();
// Set database path string.
void settingsSetDatabasePath();
// Get string array containing numPaths boot paths.
char** settingsGetBootPaths(int *numPaths);
void settingsLoadBootMenu();

// Rename currently selected boot path.
void settingsRenameBootPath();
// Draw help text for the boot menu.
void settingsDrawBootMenuTicker();

#endif
