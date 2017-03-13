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
int initSettingsMan();
// Free allocated strings.
int killSettingsMan();

// Save setting file with current settings.
int settingsSave();

// Get database path string.
const char* settingsGetDatabasePath();
// Get string array containing numPaths boot paths.
const char **settingsGetBootPaths(int *numPaths);

#endif
