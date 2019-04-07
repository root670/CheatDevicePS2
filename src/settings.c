#include <stdio.h>
#include <string.h>

#include "libraries/ini.h"
#include "settings.h"
#include "util.h"
#include "menus.h"
#include "graphics.h"
#include "cheats.h"

typedef struct settings
{
    char *databasePath;
    char *bootPaths[5];
} settings_t;

static int initialized = 0;
static settings_t settings;
static char *diskBootStr = "==Disc==";
static int settingsChanged = 0;

#ifdef _DTL_T10000
static char *settingsPath = "host:CheatDevicePS2.ini";
#else
static char *settingsPath = "CheatDevicePS2.ini";
#endif

static char *defaultDatabase = "CheatDevicePS2.cdb";
static char *defaultBootPaths[] = {
    "mass:BOOT/BOOT.ELF",
    "mass:BOOT/ESR.ELF",
    "mc0:BOOT/BOOT.ELF",
    "mc1:BOOT/BOOT.ELF",
    "rom:OSDSYS"
};

static const char *HELP_TICKER = \
    "{CROSS} Boot     "
    "{SQUARE} Options     "
    "{CIRCLE} Main Menu";

static void getINIString(struct ini_info *ini, char **dst, const char *keyName, const char *defaultValue)
{
    if(!dst || !keyName || !defaultValue)
        return;

    const char *value = (ini) ? ini_get(ini, "CheatDevicePS2", keyName) : NULL;
    if(value)
        *dst = strdup(value);
    else
        *dst = strdup(defaultValue);

    printf("getINIString: %s = %s\n", keyName, *dst);
}

int initSettings()
{
    if(initialized)
        return 0;

    printf("\n ** Initializing Settings **\n");

    struct ini_info *ini = ini_load(settingsPath);

    getINIString(ini, &settings.databasePath, "database", defaultDatabase);
    getINIString(ini, &settings.bootPaths[0], "boot1", defaultBootPaths[0]);
    getINIString(ini, &settings.bootPaths[1], "boot2", defaultBootPaths[1]);
    getINIString(ini, &settings.bootPaths[2], "boot3", defaultBootPaths[2]);
    getINIString(ini, &settings.bootPaths[3], "boot4", defaultBootPaths[3]);
    getINIString(ini, &settings.bootPaths[4], "boot5", defaultBootPaths[4]);

    if(ini)
        ini_free(ini);

    initialized = 1;
    return 1;
}

int killSettings()
{
    if(!initialized)
        return 0;

    printf(" ** Killing Settings Manager **\n");

    free(settings.databasePath);
    free(settings.bootPaths[0]);
    free(settings.bootPaths[1]);
    free(settings.bootPaths[2]);
    free(settings.bootPaths[3]);
    free(settings.bootPaths[4]);

    return 1;
}

int settingsSave()
{
    if(!initialized || !settingsChanged)
        return 0;

    FILE *iniFile = fopen(settingsPath, "w");
    if(iniFile < 0)
    {
        printf("Error saving %s\n", settingsPath);
        return 0;
    }

    fputs("[CheatDevicePS2]\n", iniFile);
    fprintf(iniFile, "database = %s\n", settings.databasePath);
    fprintf(iniFile, "boot1 = %s\n", settings.bootPaths[0]);
    fprintf(iniFile, "boot2 = %s\n", settings.bootPaths[1]);
    fprintf(iniFile, "boot3 = %s\n", settings.bootPaths[2]);
    fprintf(iniFile, "boot4 = %s\n", settings.bootPaths[3]);
    fprintf(iniFile, "boot5 = %s\n", settings.bootPaths[4]);

    fclose(iniFile);

    return 1;
}

char* settingsGetDatabasePath()
{
    if(!initialized)
        return NULL;

    return settings.databasePath;
}

void settingsSetDatabasePath(const char *path)
{
    if(!initialized || !path)
        return;

    free(settings.databasePath);
    settings.databasePath = strdup(path);
    settingsChanged = 1;
}

char** settingsGetBootPaths(int *numPaths)
{
    if(!initialized)
        return NULL;

    *numPaths = 5;
    return settings.bootPaths;
}

void settingsLoadBootMenu()
{
    if(!initialized)
        return;

    int numPaths;
    char **paths = settingsGetBootPaths(&numPaths);
    
    menuItem_t *items = calloc(numPaths + 1, sizeof(menuItem_t));
    
    /* Disc boot; default option */
    items[0].type = MENU_ITEM_NORMAL;
    items[0].text = diskBootStr;
    menuInsertItem(&items[0]);
    
    int i;
    for(i = 1; i < numPaths + 1; i++)
    {
        items[i].type = MENU_ITEM_NORMAL;
        items[i].text = paths[i-1];
        items[i].extra = (void *)&paths[i-1];
        menuInsertItem(&items[i]);
    }

    menuSetDecorationsCB(cheatsDrawStats);
    menuSetHelpTickerText(HELP_TICKER);
}

void settingsRenameBootPath()
{
    if(!initialized)
        return;

    if(menuGetActive() != MENU_BOOT)
        return;

    char **bootPath = (char **)menuGetActiveItemExtra();
    if(!bootPath)
        return;

    char newPath[80];
    if(displayTextEditMenu(newPath, sizeof(newPath), menuGetActiveItemText(), "Edit Boot Path") == 0)
        return;

    free(*bootPath);
    *bootPath = strdup(newPath);
    menuRenameActiveItem(newPath);
    menuRemoveAllItems();
    settingsLoadBootMenu();
    settingsChanged = 1;
}
