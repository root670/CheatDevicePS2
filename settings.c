#include "settings.h"
#include "libraries/ini.h"
#include "util.h"
#include "menus.h"
#include <string.h>
#include <stdio.h>

typedef struct
{
    char *databasePath;
    char *bootPaths[5];
} settings_t;

static int initialized = 0;
static struct ini_info *ini;
settings_t settings;
static char *diskBootStr = "==Disk==";

int initSettings()
{
    if(!initialized)
    {
        printf("\n ** Initializing Settings **\n");

        #ifdef _DTL_T10000
        ini = ini_load("host:CheatDevicePS2.ini");
        #else
        ini = ini_load("CheatDevicePS2.ini");
        #endif
        if(!ini)
        {
            printf("Error opening CheatDevicePS2.ini\n");
            // Use fallback values
            settings.databasePath = strdup("CheatDatabase.cdb");
            settings.bootPaths[0] = strdup("mc0:/BOOT/BOOT.ELF");
            settings.bootPaths[1] = strdup("mc1:/BOOT/BOOT.ELF");
            settings.bootPaths[2] = strdup("mass:BOOT/BOOT.ELF");
            settings.bootPaths[3] = strdup("rom0:OSDSYS");
            settings.bootPaths[4] = strdup("FASTBOOT");
            
            initialized = 1;
            return 1;
        }
        
        if(ini_get(ini, "CheatDevicePS2", "database"))
            settings.databasePath = strdup(ini_get(ini, "CheatDevicePS2", "database"));
        else
            settings.databasePath = strdup("CheatDevicePS2.cdb");
            
        if(ini_get(ini, "CheatDevicePS2", "boot1"))
            settings.bootPaths[0] = strdup(ini_get(ini, "CheatDevicePS2", "boot1"));
        else
            settings.bootPaths[0] = strdup("mc0:/BOOT/BOOT.ELF");
            
        if(ini_get(ini, "CheatDevicePS2", "boot2"))
            settings.bootPaths[1] = strdup(ini_get(ini, "CheatDevicePS2", "boot2"));
        else
            settings.bootPaths[1] = strdup("mc1:/BOOT/BOOT.ELF");
            
        if(ini_get(ini, "CheatDevicePS2", "boot3"))
            settings.bootPaths[2] = strdup(ini_get(ini, "CheatDevicePS2", "boot3"));
        else
            settings.bootPaths[2] = strdup("mass:/BOOT/BOOT.ELF");
        
        if(ini_get(ini, "CheatDevicePS2", "boot4"))
            settings.bootPaths[3] = strdup(ini_get(ini, "CheatDevicePS2", "boot4"));
        else
            settings.bootPaths[3] = strdup("rom0:OSDSYS");
        
        if(ini_get(ini, "CheatDevicePS2", "boot5"))
            settings.bootPaths[4] = strdup(ini_get(ini, "CheatDevicePS2", "boot5"));
        else
            settings.bootPaths[4] = strdup("FASTBOOT");
        
        printf("Database path: %s\n", settings.databasePath);
        printf("Boot path 1: %s\n", settings.bootPaths[0]);
        printf("Boot path 2: %s\n", settings.bootPaths[1]);
        printf("Boot path 3: %s\n", settings.bootPaths[2]);
        printf("Boot path 4: %s\n", settings.bootPaths[3]);
        printf("Boot path 5: %s\n", settings.bootPaths[4]);
        
        ini_free(ini);
        initialized = 1;
        return 1;
    }

    return 0;
}

int killSettings()
{
    if(initialized)
    {
        printf(" ** Killing Settings Manager **\n");
        free(settings.databasePath);
        free(settings.bootPaths[0]);
        free(settings.bootPaths[1]);
        free(settings.bootPaths[2]);
        free(settings.bootPaths[3]);
        free(settings.bootPaths[4]);
        return 1;
    }

    return 0;
}

int settingsSave()
{
    if(initialized)
    {
        FILE *iniFile;

        #ifdef _DTL_T10000
        iniFile = fopen("CheatDevicePS2.ini", "r");
        #else
        iniFile = fopen("host:CheatDevicePS2.ini", "r");
        #endif

        if(!iniFile)
        {
            printf("Error saving CheatDevicePS2.ini\n");
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

    return 0;
}

const char* settingsGetDatabasePath(char* property)
{
    if(initialized)
    {
        return settings.databasePath;
    }

    return NULL;
}

const char **settingsGetBootPaths(int *numPaths)
{
    if(initialized)
    {
        *numPaths = 5;
        return (const char**) settings.bootPaths;
    }
    return NULL;
}

void settingsLoadBootMenu()
{
    const char **paths;
    int numPaths;
    
    paths = settingsGetBootPaths(&numPaths);
    
    menuItem_t *items = calloc(numPaths + 1, sizeof(menuItem_t));
    /* Disc boot; default option */
    items[0].type = NORMAL;
    items[0].text = diskBootStr;
    menuInsertItem(&items[0]);
    
    int i;
    for(i = 1; i < numPaths + 1; i++)
    {
        items[i].type = NORMAL;
        items[i].text = strdup(paths[i-1]);
        items[i].extra = (void *)&paths[i-1];
        menuInsertItem(&items[i]);
    }
}

void settingsRenameBootPath()
{
    char newPath[80];

    if(initialized)
    {
        if(menuGetActive() != BOOTMENU)
            return;

        char **bootPath = (char **)menuGetActiveItemExtra();
        if(!bootPath)
            return;
        
        if(displayInputMenu(newPath, sizeof(newPath), menuGetActiveItemText(), "Edit Boot Path") == 0)
            return;

        free(*bootPath);
        bootPath = strdup(newPath);
        menuRenameActiveItem(newPath);
        menuRemoveAllItems();
        settingsLoadBootMenu();
    }
}
