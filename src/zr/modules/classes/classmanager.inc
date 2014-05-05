/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          classmanager.inc
 *  Type:          Module
 *  Description:   Manages player classes.
 *
 *  Copyright (C) 2009-2011  Greyscale, Richard Helgeby
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================================
 */

/**
 * This module's identifier.
 */
static Module:g_moduleClassMgr;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ClassMgr_GetIdentifier() { return g_moduleClassMgr; }

/**
 * @section Event ID variables with documentation.
 */

/**
 * Class data clear request.
 *
 * @param classIndex    Class index to clear, or -1 to clear all classes.
 * OnClassClear(classIndex)
 */
new ProjectEvent:g_EvOnClassClear;

/**
 * A class attribute is loading and ready to be cached.
 *
 * @param classIndex    Class index.
 * @param kv            Handle to keyvalue tree, ready to read attribute value.
 * @param attribute     Name of the current attribute.
 * @param className     Name of the current class (section name in keyvalue tree).
 * OnClassAttribLoad(classIndex, Handle:kv, const String:attribute[], const String:className[])
 */
new ProjectEvent:g_EvOnClassAttribLoad;

/**
 * Class manager sent an validation request. Attribute modules do validation on
 * all their attributes, and log errors if any.
 *
 * @param classIndex    Class index.
 * @param kv            Handle to keyvalue tree, ready to read attribute value.
 * @param attribute     Name of the current attribute.
 *
 * @return              Attribute module returns Plugin_Handled on validation error,
 *                      or Plugin_Continue if ok.
 * Action:OnClassValidate(classIndex)
 */
new ProjectEvent:g_EvOnClassValidate;

/**
 * All classes are loaded now. Attribute modules should now make a copy of their array
 * so the original values can be kept.
 *
 * @param classCount    Number of classes loaded.
 * Action:OnClassAllLoaded(classCount)
 */
new ProjectEvent:g_EvOnClassAllLoaded;

/**
 * Preloads player info before player preferences are loaded. The class manger
 * sets initial selected class indexes. Attribute modules may initialize players too.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 * OnClassPlayerPreload(client, classIndex)
 */
new ProjectEvent:g_EvOnClassPlayerPreload;

/**
 * Loads player info with player preferences (from cookies). The class manger
 * sets new selected class indexes according to player preferences. Attribute modules
 * may initialize players with their preferences too.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 * OnClassPlayerLoad(client, classIndex)
 */
new ProjectEvent:g_EvOnClassPlayerLoad;

/**
 * Class attributes are applied. Attribute modules should apply their own attributes
 * on the player now.
 *
 * @param client        Client index.
 * OnClassApply(client)
 */
new ProjectEvent:g_EvOnClassApply;

/**
 * The player selected a new class.
 *
 * @param client        Client index.
 * @param classIndex    Selected class.
 * OnClassSelected(client, classIndex)
 */
new ProjectEvent:g_EvOnClassSelected;

/**
 * A player was infected. This event is fired AFTER class attributes are applied.
 * Attribute modules should use this event and not the infection module's event when
 * initializing zombie specific features.
 *
 * @param client        Client index.
 * @param attacker      Attacker client index.
 * @param motherZombie  Specifies whether the attacker was a mother zombie.
 * OnClassPlayerInfect(client, attacker, bool:motherZombie)
 */
new ProjectEvent:g_EvOnClassPlayerInfect;

/**
 * A player was turned back into a human. This event is fired AFTER class attributes
 * are applied. Attribute modules should use this event and not the infection module's
 * event when initializing human specific features.
 *
 * @param client        Client index.
 * OnClassPlayerHuman(client)
 */
new ProjectEvent:g_EvOnClassPlayerHuman;

/**
 * @endsection
 */

/**
 * The path to this module's class config file, relative to the path defined in gamerules.txt.
 */
#define CLASSES_CONFIG_FILE "classes.txt"

/**
 * Cvar handles.
 */
new Handle:g_hCvarClassesMenuSpawn;
new Handle:g_hCvarClassesMenuJoin;
new Handle:g_hCvarClassesRandom;
new Handle:g_hCvarClassesChangeTimelimit;
new Handle:g_hCvarClassesSave;
new Handle:g_hCvarClassesDefaultZombie;
//new Handle:g_hCvarClassesDefaultMZombie;
new Handle:g_hCvarClassesDefaultHuman;
new Handle:g_hCvarClassesZombieSelect;
new Handle:g_hCvarClassesHumanSelect;

#define CLASS_MAX               48
#define CLASS_NAME_LEN          64
#define CLASS_STRING_LEN        256
#define CLASS_LIST_SEPARATOR    ","

/**
 * Class cache types. Specifies which data array to use.
 */
enum ClassCacheType
{
    ClassCache_Original,        /** Points to original data array. Data in this array is never changed once loaded. */
    ClassCache_Modified,        /** Points to writable data array. Data in this array may be modified through console commands. */
    ClassCache_Player           /** Points to player cache array. Data in this array is always valid and ready to read/modify. */
}

/**
 * Class teams.
 */
enum ClassTeam
{
    ClassTeam_Invalid = -1,     /** Not valid in a class! */
    ClassTeam_Zombies,
    ClassTeam_Humans,
    ClassTeam_All               /** Not valid in a class! It's only used by filters. */
}

/**
 * Authorization modes for classes.
 */
enum ClsGeneric_AuthModes
{
    ClsGeneric_InvalidMode = -1,
    ClsGeneric_Disabled,            // No authorization.
    ClsGeneric_Flag,                // Player must have a flag.
    ClsGeneric_Group,               // Player must be in a group.
    ClsGeneric_Either,              // Player must have either a flag or be in a group.
    ClsGeneric_Both,                // Player must both have a flag and be in a group.
    ClsGeneric_All                  // Player must have all flags and be in all groups.
}

/**
 * Number of classes loaded.
 */
new g_ClassCount;

/**
 * Function for outside files to use to return number of classes loaded.
 */
stock ClassMgr_GetClassCount() { return g_ClassCount; }

/**
 * Specifies whether classes are loaded.
 */
static bool:g_ClassLoaded = false;

/**
 * Function for outside files to use to return if classes are loaded.
 */
stock bool:ClassMgr_IsClassesLoaded() { return g_ClassLoaded; }

/**
 * Selected classes.
 */
new g_ClassSelected[MAXPLAYERS + 1][ClassTeam];      // Currently active class.
new g_ClassNextClass[MAXPLAYERS + 1][ClassTeam];     // Class to use next spawn, if any.

/**
 * Cache of default class indexes.
 */
static ClassDefaultClass[ClassTeam];

/**
 * Specifies whether a player is allowed to change class with instant effect.
 * This is only used on human classes, and in combination with the
 * zr_classes_change_timelimit cvar, but could be used other places too. The
 * class menu will automatically check this setting and apply attributes if set
 * to true.
 */
new bool:g_ClassAllowInstantChange[MAXPLAYERS + 1];

/**
 * Timers for instant class change.
 */
new Handle:g_hClassInstantChange[MAXPLAYERS + 1];

/**
 * Specifies whether a player's next spawn will be the first one on the map.
 */
new bool:g_ClassFirstSpawn[MAXPLAYERS + 1];

/**
 * Stores whether the player has manually changed class since connecting.
 */
new bool:g_ClassChangedByPlayer[MAXPLAYERS + 1][ClassTeam];

/**
 * Trie handle to class name index.
 */
new Handle:g_hClassNameIndex;

/**
 * Cookies for saving selected classes when players disconnect.
 */
new Handle:g_hClassZombieCookie;
new Handle:g_hClassHumanCookie;

/**
 * @section Flags for special classes.
 */
#define CLASS_FLAG_ADMIN_ONLY       (1<<0)  /** Class is usable by admins only. */
#define CLASS_FLAG_MOTHER_ZOMBIE    (1<<1)  /** Class is usable by mother zombies only. */

/** A combination of special class flags. Used to exclude special classes. */
#define CLASS_SPECIALFLAGS           CLASS_FLAG_ADMIN_ONLY + CLASS_FLAG_MOTHER_ZOMBIE
/**
 * @endsection
 */

/**
 * Structure for class filter settings passed to various functions.
 */
enum ClassFilter
{
    bool:ClassFilter_IgnoreEnabled,     /** Ignore whether the class is disabled or not. */
    ClassFilter_RequireFlags,           /** Flags the classes must have set. */
    ClassFilter_DenyFlags,              /** Flags the classes cannot have set. */
    ClassFilter_Client                  /** The client authorize. Use 0 to ignore authorization, and negative to exclude classes with authorization. */
}

/**
 * Empty filter structure.
 */
stock g_ClassNoFilter[ClassFilter];

/**
 * Filter structure for excluding special classes.
 */
stock g_ClassNoSpecialClasses[ClassFilter] = {false, 0, CLASS_SPECIALFLAGS, -1};

/**
 * Handle to main class menu (menulib menu handle).
 */
new Handle:g_ClassMenuMain;


// Libraries.
#include "zr/libraries/authlib"
#include "zr/libraries/cookielib"
#include "zr/libraries/menulib"
#include "zr/libraries/dynmenulib"

// Utility and logic includes.
#include "zr/modules/classes/attributeregister"
#include "zr/modules/classes/nextindexes"
#include "zr/modules/classes/instantchange"
#include "zr/modules/classes/classfilter"
#include "zr/modules/classes/classmenus"

// Attribute module includes
#include "zr/modules/classes/genericattributes"

// Game-specific attribute modules.
#if defined PROJECT_GAME_CSS
    //#include "zr/modules/classes/model"
#endif


/**
 * Register this module.
 */
ClassMgr_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Class Manager");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "classmanager");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Manages player classes.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleClassMgr = ModuleMgr_Register(moduledata);
    
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnEventsRegister",      "ClassMgr_OnEventsRegister");
    
    // Register config file that this module will use.
    ConfigMgr_Register(g_moduleClassMgr, "ClassMgr_OnConfigReload", "");
    
    // Create events.
    g_EvOnClassClear =              EventMgr_CreateEvent("Event_OnClassClear");
    g_EvOnClassAttribLoad =         EventMgr_CreateEvent("Event_OnClassAttribLoad");
    g_EvOnClassValidate =           EventMgr_CreateEvent("Event_OnClassValidate");
    g_EvOnClassAllLoaded =          EventMgr_CreateEvent("Event_OnClassAllLoaded");
    g_EvOnClassPlayerPreload =      EventMgr_CreateEvent("Event_OnClassPlayerPreload");
    g_EvOnClassPlayerLoad =         EventMgr_CreateEvent("Event_OnClassPlayerLoad");
    g_EvOnClassApply =              EventMgr_CreateEvent("Event_OnClassApply");
    g_EvOnClassSelected =           EventMgr_CreateEvent("Event_OnClassSelected");
    g_EvOnClassPlayerInfect =       EventMgr_CreateEvent("Event_OnClassPlayerInfect");
    g_EvOnClassPlayerHuman =        EventMgr_CreateEvent("Event_OnClassPlayerHuman");
    
    // Prepare attribute register.
    ClassAttribReg_Create();
}

/**
 * Register all events here.
 */
public ClassMgr_OnEventsRegister()
{
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnAllPluginsLoaded",      "ClassMgr_OnAllPluginsLoaded");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnPluginEnd",             "ClassMgr_OnPluginEnd");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnAllModulesLoaded",      "ClassMgr_OnAllModulesLoaded");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnEventsReady",           "ClassMgr_OnEventsReady");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnModuleEnable",          "ClassMgr_OnModuleEnable");
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnMyModuleEnable",          "ClassMgr_OnMyModuleEnable");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnModuleDisable",         "ClassMgr_OnModuleDisable");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnMyModuleDisable",       "ClassMgr_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnMapStart",                "ClassMgr_OnMapStart");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnMapEnd",                "ClassMgr_OnMapEnd");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnAutoConfigsBuffered",   "ClassMgr_OnAutoCfgBuffered");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnConfigsExecuted",       "ClassMgr_OnConfigsExecuted");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnClientPutInServer",     "ClassMgr_OnClientPutInServer");
    //EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnClientDisconnect",      "ClassMgr_OnClientDisconnect");
    
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnClientInfected",          "ClassMgr_OnClientInfected");
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnClientHuman",             "ClassMgr_OnClientHuman");
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnClientReady",             "ClassMgr_OnClientReady");
    
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_OnClassSelected",           "ClassMgr_OnClassSelected");
    
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_PlayerSpawn",           "ClassMgr_PlayerSpawn");
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_PlayerSpawnPost",       "ClassMgr_PlayerSpawnPost");
    EventMgr_RegisterEvent(g_moduleClassMgr, "Event_PlayerDeath",           "ClassMgr_PlayerDeath");
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:ClassMgr_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // Don't let the module load unless the configs cached successfully.
    if (!g_ClassLoaded && !ClassMgr_LoadClasses())
    {
        Format(refusalmsg, maxlen, "%T", LANG_SERVER, "ClassMgr refuse enable");
        return Plugin_Handled;
    }
    
    return Plugin_Continue;
}

/**
 * Loads classes from file.
 */
bool:ClassMgr_LoadClasses()
{
    // TODO: Reorganize so it will also do validation here.
    
    // Get the config path from the gamerules module.
    decl String:configfile[PLATFORM_MAX_PATH];
    GameRules_GetConfigPath(CLASSES_CONFIG_FILE, configfile);
    
    if (ConfigMgr_ValidateFile(configfile))
        ConfigMgr_WriteString(g_moduleClassMgr, CM_CONFIGINDEX_FIRST, ConfigData_Path, CM_DATA_PATH, configfile);
    else
    {
        LogMgr_Print(g_moduleClassMgr, LogType_Fatal_Module, "Config Validation", "Invalid config file path defined in gamerules: \"%s\".  Disabling module.", configfile);
        return false;
    }
    
    // Reset loaded-state.
    g_ClassLoaded = false;
    
    // Prepare class name index. Create or clear.
    ClassMgr_PrepareNameIndex();
    
    // Log loading-message.
    LogMgr_Print(g_moduleClassMgr, LogType_Debug, "Config Loading", "Loading classes from file \"%s\".", configfile);
    
    // Parse class file.
    g_ClassCount = ConfigMgr_CacheKv(g_moduleClassMgr, CM_CONFIGINDEX_FIRST, "ClassMgr_LoadClass");
    
    // Log loaded-message.
    LogMgr_Print(g_moduleClassMgr, LogType_Debug, "Config Loading", "%d class(es) loaded.", g_ClassCount);
    
    // Check if there are no classes.
    if (g_ClassCount == 0)
    {
        LogMgr_Print(g_moduleClassMgr, LogType_Fatal_Module, "Config Validation", "Error: No usable data found in class config file: %s", configfile);
        return false;
    }
    
    g_ClassLoaded = true;
    return true;
    
}

/**
 * Loops through each class in the class config.
 * 
 * @param kv            The keyvalues handle of the config file. (Don't close this)
 * @param sectionindex  The index of the current keyvalue section, starting from 0.
 * @param sectionname   The name of the current keyvalue section.
 * 
 * @return              See enum KvCache.
 */
public KvCache:ClassMgr_LoadClass(Handle:kv, sectionindex, const String:sectionname[])
{
    decl String:attributeName[CLASS_NAME_LEN];
    
    // Check if maximum number of classes is reached.
    if (sectionindex == CLASS_MAX)
    {
        LogMgr_Print(g_moduleClassMgr, LogType_Error, "Config Validation", "Warning: Maximum number of classes reached (%d), ignoring classes from class \"%s\" (%d).", CLASS_MAX, sectionname, sectionindex);
        return KvCache_Hault;
    }
    
    KvGotoFirstSubKey(kv, false);
    do
    {
        // Get attribute name.
        KvGetSectionName(kv, attributeName, sizeof(attributeName));
        
        // Go back to the attribute again so it can read the value.
        KvGoBack(kv);
        
        // Get the module responsible for this attribute.
        new Module:attributeModule = ClassAttribReg_GetAttrib(attributeName);
        
        // Validate module.
        if (attributeModule != INVALID_MODULE)
        {
            // Prepare event data.
            static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell, DataType_String, DataType_String};
            decl any:eventdata[sizeof(eventdatatypes)][CLASS_NAME_LEN];
            eventdata[0][0] = sectionindex;
            eventdata[1][0] = kv;
            strcopy(eventdata[2], CLASS_NAME_LEN, attributeName);
            strcopy(eventdata[3], CLASS_NAME_LEN, sectionname);
            new Module:eventFilter[2];
            eventFilter[0] = attributeModule;
            eventFilter[1] = INVALID_MODULE;
            
            // Send the load event to the attribute module only.
            EventMgr_Forward(g_EvOnClassAttribLoad, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes, eventFilter);
        }
        else
        {
            // Attribute wasn't registered. This is either caused by a typo in
            // the config or when a module isn't loaded.
            LogMgr_Print(g_moduleClassMgr, LogType_Error, "Config Validation", "Warning: Ignoring invalid class attribute in class \"%s\" (%d): %s", sectionname, sectionindex, attributeName);
        }
        
        // Jump back to the attribute again so the parser will find the next attribute.
        KvJumpToKey(kv, attributeName);
    } while (KvGotoNextKey(kv, false));
    
    // Go one level up (to the class section level).
    KvGoBack(kv);
    
    return KvCache_Continue;
}

/**
 * Plugin is loading.
 */
ClassMgr_OnPluginStart()
{
    // Register the module.
    ClassMgr_Register();
    
    // Create cvars.
    g_hCvarClassesMenuSpawn =       Project_CreateConVar("classes_menu_spawn",              "0",                "Re-display class selection menu every spawn.");
    g_hCvarClassesMenuJoin =        Project_CreateConVar("classes_menu_join",               "0",                "Display class selection menu when a player spawn for the first time.");
    g_hCvarClassesRandom =          Project_CreateConVar("classes_random",                  "0",                "Player is assigned a random class every spawn. [Override: <prefix>_classes_default_*]");
    g_hCvarClassesChangeTimelimit = Project_CreateConVar("classes_change_timelimit",        "20",               "Time limit to change human class with instant change after spawning. Time is in seconds. Use 0 or negative to disable.");
    g_hCvarClassesSave =            Project_CreateConVar("classes_save",                    "1",                "Save players' class selections in server cookies and restore when connecting. [Overrides: <prefix>_classes_default_*]");
    g_hCvarClassesDefaultZombie =   Project_CreateConVar("classes_default_zombie",          "random",           "Name of zombie class that should be assigned to players on connect. [\"random\" = Random zombie class | \"\" = Class config default]");
    //g_hCvarClassesDefaultMZombie =  Project_CreateConVar("classes_default_mother_zombie",   "motherzombies",    "Zombie class assigned to mother zombies. [\"motherzombies\" = Random mother zombie class | \"random\" = Random regular zombie class | \"disabled\" = Don't change class on mother zombies]");
    g_hCvarClassesDefaultHuman =    Project_CreateConVar("classes_default_human",           "random",           "Name of human class that should be assigned to players on connect. [\"random\" = Random human class | \"\" = Class config default]");
    //g_hCvarClassesDefaultAdmin =    Project_CreateConVar("classes_default_admin",           "default",          "(Not implemented!) Admin-only class assigned to admins on connect, if any. [\"default\" = Default human class | \"random\" = Random admin-only class]");
    g_hCvarClassesZombieSelect =    Project_CreateConVar("classes_zombie_select",           "1",                "Allow players to select zombie classes.");
    g_hCvarClassesHumanSelect =     Project_CreateConVar("classes_human_select",            "1",                "Allow players to select human classes.");
    
    // Setup cookies.
    ClassMgr_PrepareCookies();
    
    // Forward plugin start event to attribute modules.
    ClsGeneric_OnPluginStart();
    
    // Game dependent attribute modules. Some modules should work for multiple
    // games, but CS:S is the only game we build for at the momemt.
    #if defined PROJECT_GAME_CSS
        //ClsModel_OnPluginStart();
    #endif
}


/****************
 *    EVENTS    *
 ****************/

/**
 * The map has started.
 */
public ClassMgr_OnMapStart()
{
    new eventdata[1][1];
    new Action:result;
    
    // Clear all classes.
    eventdata[0][0] = -1;
    EventMgr_Forward(g_EvOnClassClear, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    
    // Load classes from file.
    ClassMgr_LoadClasses();
    
    LogMgr_Print(g_moduleClassMgr, LogType_Debug, "Config Validation", "Validating loaded classes.");
    
    // Loop through all classes and send validation event. Read final result from event manager.
    for (new classIndex = 0; classIndex < g_ClassCount; classIndex++)
    {
        // Only validate enabled classes.
        if (ClsGeneric_IsEnabled(classIndex, ClassCache_Original))
        {
            eventdata[0][0] = classIndex;
            result = EventMgr_Forward(g_EvOnClassValidate, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
            
            // Check for validation errors.
            if (result == Plugin_Handled)
            {
                // Disable class.
                ClsGeneric_SetEnabled(classIndex, false);
            }
        }
    }
    
    // Validate class minimum reqirements.
    if (!ClsMgr_ValidateRequirements())
    {
        LogMgr_Print(g_moduleClassMgr, LogType_Fatal_Module, "Config Validation", "Error: The class configuration doesn't match the minimum requirements.");
    }
    
    // Send all classes loaded event (so modules can copy their cache).
    eventdata[0][0] = g_ClassCount;
    EventMgr_Forward(g_EvOnClassAllLoaded, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
}

/**
 * All plugins have loaded.
 */
public ClassMgr_OnAllPluginsLoaded()
{
}

/**
 * Plugin is ending.
 */
public ClassMgr_OnPluginEnd()
{
}

/**
 * All modules and events have been registered by this point.  Event priority can be changed here.
 */
public ClassMgr_OnEventsReady()
{
}

/**
 * All modules have been registered.
 */
public ClassMgr_OnAllModulesLoaded()
{
}

/**
 * A module has been enabled.
 * 
 * @return      Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:ClassMgr_OnModuleEnable(Module:module)
{
}

/**
 * A module has been disabled.
 * 
 * @return      Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:ClassMgr_OnModuleDisable(Module:module)
{
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @return      Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:ClassMgr_OnMyModuleDisable()
{
    // TODO: Disable classes.
    //       Unhook console and chat commands.
    //      Kill any adt arrays that you use.  I don't think you use any for storage though.
    // Unhooking commands normally means adding a check if IsModuleEnabled in the callback and returning Plugin_Continue if it's not.  (See other modules)
}

/**
 * Called when a registered config file (by this module) is manually.
 */
public ClassMgr_OnConfigReload(configindex)
{
    // Forward event.
    ClassMgr_OnMapStart();
    
    // Re-validate class selection indexes. If team mismach, config default
    // will be used on next spawn.
    ClassMgr_ValidateSelClasses();
}

/**
 * Client has joined the server.
 * 
 * @param client    The client index.
 */
public ClassMgr_OnClientPutInServer(client)
{
    // Not spawned yet.
    g_ClassFirstSpawn[client] = true;
    
    // Classes aren't changed manually yet.
    g_ClassChangedByPlayer[client][ClassTeam_Humans] = false;
    g_ClassChangedByPlayer[client][ClassTeam_Zombies] = false;
    
    // Reset instant class change.
    ClassMgr_ResetInstantChange(client);
    
    // Reset next-indexes.
    ClassMgr_ResetNextIndexes(client);
    
    new eventdata[2][1];
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell};
    
    // Cookies may be loaded late. Preload some clean classes.
    
    // Find clean human class. Send preload event.
    new human = ClassMgr_GetDefaultSpawnClass(ClassTeam_Humans, g_ClassNoSpecialClasses);
    if (ClassMgr_IsValidIndex(human))
    {
        // Send event OnClassPlayerPreload.
        eventdata[0][0] = client;
        eventdata[1][0] = human;
        EventMgr_Forward(g_EvOnClassPlayerPreload, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
    }
    
    // Find clean zombie class.
    new zombie = ClassMgr_GetDefaultSpawnClass(ClassTeam_Zombies, g_ClassNoSpecialClasses);
    
    // Set selected class indexes.
    g_ClassSelected[client][ClassTeam_Humans] = human;
    g_ClassSelected[client][ClassTeam_Zombies] = zombie;
}

/**
 * Client is disconnecting from the server.
 * 
 * @param client    The client index.
 */
public ClassMgr_OnClientDisconnect(client)
{
    // Reset instant class change.
    ClassMgr_ResetInstantChange(client);
}

/**
 * Client has spawned.
 * 
 * @param client    The client index.
 */
public ClassMgr_PlayerSpawn(client)
{
    // TODO: Display menu (below).
    
    // Stop if classes aren't loaded.
    if (!g_ClassLoaded)
    {
        return;
    }
    
    // Stop if not on a team. The spawn event is also triggered when clients
    // connect and spawn as spectators right before they're presented the team
    // selection menu.
    if (!Util_IsClientOnTeam(client))
    {
        return;
    }
    
    // Restore next-indexes.
    ClassMgr_ResetNextIndexes(client);
    
    // Get config and steam id.
    new bool:randomClass = GetConVarBool(g_hCvarClassesRandom);
    decl String:steamId[32];
    steamId[0] = 0;
    GetClientAuthString(client, steamId, sizeof(steamId));
    
    // Get random class if enabled in config. Always get random class for bots.
    if (randomClass || StrEqual(steamId, "BOT"))
    {
        // Setup filtering
        // ---------------
        new filter[ClassFilter];
        
        // Exclude special class flags like mother zombies and admin classes.
        filter[ClassFilter_DenyFlags] = CLASS_SPECIALFLAGS;
                    
        // Allow admin classes if admin.
        filter[ClassFilter_DenyFlags] -= Auth_IsClientAdmin(client) ? CLASS_FLAG_ADMIN_ONLY : 0;
        
        // Specify client for checking group permissions.
        filter[ClassFilter_Client] = client;
        
        // Get classes
        // -----------
        
        decl String:className[CLASS_NAME_LEN];
        className[0] = 0;
        
        // Get random classes for each type.
        new randomZombie = ClassMgr_GetRandomClass(ClassTeam_Zombies, filter, ClassCache_Modified);
        new randomHuman = ClassMgr_GetRandomClass(ClassTeam_Humans, filter, ClassCache_Modified);
        
        // Set selected zombie class index.
        g_ClassSelected[client][ClassTeam_Zombies] = randomZombie;
        
        // Get zombie class name and inform player about class assignment.
        ClsGeneric_GetDisplayName(randomZombie, className, sizeof(className), ClassCache_Modified);
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "Classes random assignment", className);
        
        // Set selected human class index.
        g_ClassSelected[client][ClassTeam_Humans] = randomHuman;
        
        // Get human class name and inform player about class assignment.
        className[0] = 0;
        ClsGeneric_GetDisplayName(randomHuman, className, sizeof(className), ClassCache_Modified);
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "Classes random assignment", className);
    }
    
    // Display class menu configs according to config.
    new bool:menuSpawn = GetConVarBool(g_hCvarClassesMenuSpawn);
    new bool:menuJoin = GetConVarBool(g_hCvarClassesMenuJoin);
    if (menuSpawn || (menuJoin && g_ClassFirstSpawn[client]))
    {
        // TODO: Display class menu.
    }
    
    // Send OnClassPlayerLoad event with active class index.
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell};
    decl any:eventdata[sizeof(eventdatatypes)][1];
    eventdata[0][0] = client;
    eventdata[1][0] = ClassMgr_GetActiveIndex(client);
    EventMgr_Forward(g_EvOnClassPlayerLoad, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
    
    // Setup instant class change if enabled.
    ClassMgr_CheckInstantChange(client);
    
    // No longer the first spawn.
    g_ClassFirstSpawn[client] = false;
}

/**
 * Client has spawned (delayed event).
 * 
 * @param client    The client index.
 */
public ClassMgr_PlayerSpawnPost(client)
{
    // Note: The OnPlayerHuman will be fired automatically by the core on spawn.
    //       Applying class attributes here are no longer needed.
    
    /*
    // Stop if classes aren't loaded.
    if (!g_ClassLoaded)
    {
        return;
    }
    
    // Stop if not on a team (spawning as spectator when connecting).
    if (!Util_IsClientOnTeam(client))
    {
        return;
    }
    
    // Send OnClassApply event.
    decl any:eventdata[sizeof(g_CommonDataType2)][1];
    eventdata[0][0] = client;
    EventMgr_Forward(g_EvOnClassApply, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    */
}

/**
 * Client has been killed.
 * 
 * @param victim    The index of the killed client.
 * @param attacker  The killer of the victim.
 * @param weapon    The weapon classname used to kill the victim. (No weapon_ prefix)
 * @param headshot  True if the death was by headshot, false if not.
 */
public ClassMgr_PlayerDeath(victim, attacker, const String:weapon[], bool:headshot)
{
    // Reset instant class change.
    ClassMgr_ResetInstantChange(victim);
}

/**
 * Client is connected, admin checked and cookies are loaded.
 *
 * @param client    Client index.
 */
public ClassMgr_OnClientReady(client)
{
    // Current classes selected are pure and public classes. Load the player's
    // saved classes from cookies - unless the player already selected a class.
    
    // Setup filter
    // ------------
    new filter[ClassFilter];
    
    // No mother zombies.
    filter[ClassFilter_DenyFlags] = CLASS_FLAG_MOTHER_ZOMBIE;
    
    // Disallow admin classes if not admin.
    filter[ClassFilter_DenyFlags] += !Auth_IsClientAdmin(client) ? CLASS_FLAG_ADMIN_ONLY : 0;
    
    // Do class authorization check.
    filter[ClassFilter_Client] = client;
    
    
    // Load classes
    // ------------
    ClassMgr_LoadInitialClass(client, ClassTeam_Humans, filter);
    ClassMgr_LoadInitialClass(client, ClassTeam_Zombies, filter, false);    // Don't send event.
}

/**
 * Client was infected.
 *
 * @param client            Client index.
 * @param attacker          Attacker index, if any.
 * @param motherZombie      Infected as mother zombie.
 */
public ClassMgr_OnClientInfected(client, attacker, bool:motherZombie)
{
    // Reset instant class change.
    ClassMgr_ResetInstantChange(client);
    
    // Get active index.
    
    // If mother zombie, get class according to config.
    
    // Set slected zombie class to active index.
    
    // Restore next-indexes.
    ClassMgr_ResetNextIndexes(client);
    
    // Send OnClassApply event.
    
    // Send OnClassClientInfected event.
    
}

/**
 * Client was turned back into a human.
 *
 * @param client            Client index.
 */
public ClassMgr_OnClientHuman(client)
{
    
}

/**
 * The player selected a new class.
 *
 * @param client        Client index.
 * @param classIndex    Selected class.
 */
public ClassMgr_OnClassSelected(client, classIndex)
{
    PrintToChat(client, "DEBUG: You selected class %d.", classIndex);
    
    // Get class team.
    
    // Check if client is playing on that team, and is allowed to do instant
    // change.
    
    // If not allowed:
    // Set next index.
    // Notify player.
    
    // If change is allowed:
    // Update selected index.
    // Apply attributes if player is alive (send event).
    // Notify player.
    
    // Save new class index in cookies if save is enabled.
}

/******************************
 *    INTERNAL TOOLS/LOGIC    *
 ******************************/

/**
 * Validates the minimum class requirements.
 * Note: This function will log errors, but not stop the plugin.
 *
 * @param cacheDefaults     Optional. Cache default class indexes.
 *                          Note: Writes -1 to cache if not found!
 *
 * @return                  True if valid, false otherwise.
 */
static bool:ClsMgr_ValidateRequirements(bool:cacheDefaults = false)
{
    if (g_ClassCount == 0)
    {
        if (cacheDefaults)
        {
            ClassDefaultClass[ClassTeam_Zombies] = -1;
            ClassDefaultClass[ClassTeam_Humans] = -1;
        }
        return false;
    }
    
    new bool:failed = false;
    
    // Find a clean default zombie class, log error if not found.
    new zombie = ClassMgr_GetDefaultClass(ClassTeam_Zombies, g_ClassNoSpecialClasses, ClassCache_Original);
    if (zombie < 0)
    {
        LogMgr_Print(g_moduleClassMgr, LogType_Error, "Config Validation", "Error: Unable to find a clean default zombie class. Check class config. (Error code: %d)", zombie);
        if (cacheDefaults)
        {
            ClassDefaultClass[ClassTeam_Zombies] = -1;
        }
        failed = true;
    }
    else
    {
        // Cache default class index.
        if (cacheDefaults)
        {
            ClassDefaultClass[ClassTeam_Zombies] = zombie;
        }
    }
    
    // Find a clean default human class, log error if not found.
    new human = ClassMgr_GetDefaultClass(ClassTeam_Humans, g_ClassNoSpecialClasses, ClassCache_Original);
    if (human < 0)
    {
        LogMgr_Print(g_moduleClassMgr, LogType_Error, "Config Validation", "Error: Unable to find a clean default human class. Check class config. (Error code: %d)", human);
        if (cacheDefaults)
        {
            ClassDefaultClass[ClassTeam_Humans] = -1;
        }
        failed = true;
    }
    else
    {
        // Cache default class index.
        if (cacheDefaults)
        {
            ClassDefaultClass[ClassTeam_Humans] = human;
        }
    }
    
    return !failed;
}

/**
 * Validates and fix a client's selected classes. If invalid, use config default
 * for next spawn.
 *
 * @param client    Optional. Client index. Use -1 to validate all players.
 */
static ClassMgr_ValidateSelClasses(client = -1)
{
    new start = 1;
    new end = MaxClients;
    
    if (client > 0)
    {
        // Do client only.
        start = client;
        end = client;
    }
    
    // Loop through players.
    for (new i = start; i <= end; i++)
    {
        new zombie = g_ClassSelected[i][ClassTeam_Zombies];
        new human = g_ClassSelected[i][ClassTeam_Humans];
        new nextZombie = g_ClassNextClass[i][ClassTeam_Zombies];
        new nextHuman = g_ClassNextClass[i][ClassTeam_Humans];
        
        // Check if current or next zombie class is not a zombie.
        if (nextZombie >= 0 && ClsGeneric_GetTeam(nextZombie, ClassCache_Modified) != ClassTeam_Zombies ||
            ClsGeneric_GetTeam(zombie, ClassCache_Modified) != ClassTeam_Zombies)
        {
            // Set next class to be config default.
            g_ClassNextClass[i][ClassTeam_Zombies] = ClassDefaultClass[ClassTeam_Zombies];
        }
        
        // Check if current or next human class is not a human.
        if (nextHuman >= 0 && ClsGeneric_GetTeam(nextHuman, ClassCache_Modified) != ClassTeam_Humans ||
            ClsGeneric_GetTeam(human, ClassCache_Modified) != ClassTeam_Humans)
        {
            // Set next class to be config default.
            g_ClassNextClass[i][ClassTeam_Humans] = ClassDefaultClass[ClassTeam_Humans];
        }
    }
}

/**
 * Prepares the class name index.
 */
static ClassMgr_PrepareNameIndex()
{
    // Prepare trie.
    if (g_hClassNameIndex == INVALID_HANDLE)
    {
        g_hClassNameIndex = CreateTrie();
    }
    else
    {
        ClearTrie(g_hClassNameIndex);
    }
}

/**
 * Creates cookies if they don't exist.
 */
static ClassMgr_PrepareCookies()
{
    if (g_hClassHumanCookie == INVALID_HANDLE)
    {
        g_hClassHumanCookie = RegClientCookie("zr_humanclass", "The last human class selected.", CookieAccess_Protected);
    }
    
    if (g_hClassZombieCookie == INVALID_HANDLE)
    {
        g_hClassZombieCookie = RegClientCookie("zr_zombieclass", "The last zombie class selected.", CookieAccess_Protected);
    }
}

/**
 * Closes open cookie handles.
 */
static stock ClassMgr_CloseCookies()
{
    if (g_hClassHumanCookie != INVALID_HANDLE)
    {
        CloseHandle(g_hClassHumanCookie);
        g_hClassHumanCookie = INVALID_HANDLE;
    }
    
    if (g_hClassZombieCookie != INVALID_HANDLE)
    {
        CloseHandle(g_hClassZombieCookie);
        g_hClassZombieCookie = INVALID_HANDLE;
    }
}

/**
 * Wrapper to get class cookie for a certain team.
 *
 * @param team      Team to get.
 * @return          Cookie handle. INVALID_HANDLE if invalid team or not
 *                  initialized.
 */
static Handle:ClassMgr_GetCookie(ClassTeam:team)
{
    switch (team)
    {
        case ClassTeam_Humans:
        {
            return g_hClassHumanCookie;
        }
        case ClassTeam_Zombies:
        {
            return g_hClassZombieCookie;
        }
    }
    return INVALID_HANDLE;
}

/**
 * Loads a class from cookies (or config default) to a player when connected.
 *
 * @param client        Client index.
 * @param team          Team to load.
 * @param filter        Class filter settings.
 * @param sendEvent     Optional. Send the Event_OnClassPlayerLoad event.
 */
static ClassMgr_LoadInitialClass(client, ClassTeam:team, filter[ClassFilter], bool:sendEvent = true)
{
    // Check if the player already slected a class for this team.
    if (g_ClassChangedByPlayer[client][team])
    {
        // Abort loading. Another class is already loaded.
        return;
    }
    
    new bool:loadCookies = GetConVarBool(g_hCvarClassesSave);
    new classIndex = -1;
    
    // Load saved class from cookies if enabled.
    if (loadCookies)
    {
        // Get saved class index in cookie.
        classIndex = CookieLib_GetInt(client, ClassMgr_GetCookie(team)) - 1;
        
        // Check if valid and team in cookie matches with team in class config.
        if (ClassMgr_IsValidIndex(classIndex) && !ClsGeneric_TeamEqual(team, classIndex, ClassCache_Modified))
        {
            // Team doesn't match anymore (class config may have changed).
            // Invalidate class so another one is choosen.
            classIndex = -1;
        }
    }
    
    // Validate index.
    if (!ClassMgr_IsValidIndex(classIndex))
    {
        // Fall back on default class specified in cvar.
        classIndex = ClassMgr_GetDefaultSpawnClass(team, filter);
    }
    
    // Validate again.
    if (!ClassMgr_IsValidIndex(classIndex))
    {
        // Fall back on config default.
        classIndex = ClassMgr_GetDefaultClass(team, filter);
        
        // Validate once more.
        if (!ClassMgr_IsValidIndex(classIndex))
        {
            // Failed to fall back on config default. Nothing else to do.
            // This should never happen because of class requirement validation.
            decl String:strTeam[16];
            ClassMgr_TeamToString(team, strTeam, sizeof(strTeam));
            LogMgr_Print(g_moduleClassMgr, LogType_Error, "Config Validation", "Error: Unable to find a clean default class for team: %s. Check class config file.", strTeam);
            return;
        }
    }
    
    // Mark class as selected.
    g_ClassSelected[client][team] = classIndex;
    
    // Send event if enabled.
    if (sendEvent)
    {
        static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell};
        new eventdata[2][1];
        
        // Send event OnClassPlayerLoad.
        eventdata[0][0] = client;
        eventdata[1][0] = classIndex;
        EventMgr_Forward(g_EvOnClassPlayerLoad, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
    }
}


/*********************************
 *    CLASS MANAGER UTILITIES    *
 *********************************/

/**
 * Returns whether the specified class valid.
 *
 * @param classIndex    The class index to validate.
 *
 * @return              True if the class exist, false otherwise.
 */
stock bool:ClassMgr_IsValidIndex(classIndex)
{
    if (classIndex >= 0 && classIndex < g_ClassCount)
    {
        return true;
    }
    return false;
}

/**
 * Compares the specified team with a class' team.
 *
 * @param index     Index of the class in a class cache or a client index,
 *                  depending on the cache type specified.
 * @param team      The team to compare with the class.
 * @param cache     Optional. Specifies what class cache to read from. If player
 *                  cache is used 'index' will be used as a client index.
 *                  Default is modified cache.
 *
 * @return          True if equal, false otherwise.
 */
stock bool:ClassMgr_TeamEqual(index, ClassTeam:team, ClassCacheType:cache = ClassCache_Modified)
{
    return ClsGeneric_GetTeam(index, cache) == team;
}

/**
 * Gets the currently active class index that the player is using.
 *
 * @param client    The client index.
 *
 * @return          The active class index. -1 if a spectactor or not alive.
 */
stock ClassMgr_GetActiveIndex(client)
{
    // Check if alive.
    if (!IsPlayerAlive(client))
    {
        return -1;
    }
    
    // Check if on a valid team.
    if (!Util_IsClientOnTeam(client))
    {
        return -1;
    }
    
    // Check if player is human or zombie.
    // TODO: Handle ghost team.
    new ClassTeam:team;
    if (TeamMgr_IsClientHuman(client))
    {
        team = ClassTeam_Humans;
    }
    else
    {
        team = ClassTeam_Zombies;
    }
    
    // Return the active class for the current team.
    return g_ClassSelected[client][team];
}

/**
 * Gets the next class that will be used next spawn, for the current team.
 *
 * @param client    Client index.
 *
 * @return          Class index or -1 if no next class set for the current team.
 */
stock ClassMgr_GetNextClassIndex(client)
{
    if (TLib_IsClientHuman(client))
    {
        return g_ClassNextClass[client][ClassTeam_Humans];
    }
    else
    {
        return g_ClassNextClass[client][ClassTeam_Zombies];
    }
}

/**
 * Gets the next class name that will be used next spawn, for the current team.
 *
 * @param client    Client index.
 * @param buffer    Destination string buffer.
 * @param maxlen    Size of buffer.
 *
 * @return          Number of cells written.
 */
stock ClassMgr_GetNextClassName(client, String:buffer, maxlen)
{
    new nextClass = ClassMgr_GetNextClassIndex(client);
    
    if (ClassMgr_IsValidIndex(nextClass))
    {
        return ClsGeneric_GetDisplayName(nextClass, buffer, maxlen);
    }
    
    return 0;
}

/**
 * Decides whether a class selection menu should be enabled. The decision is
 * based on zr_class_allow_* console variables.
 *
 * @param team      Optional. Team to match. Default is all.
 *
 * @return          True if allowed, false otherwise.
 */
stock bool:ClassMgr_AllowSelection(client, ClassTeam:team = ClassTeam_All)
{
    // Get selection settings.
    new bool:zombie = GetConVarBool(g_hCvarClassesZombieSelect);
    new bool:human = GetConVarBool(g_hCvarClassesHumanSelect);
    
    // Check if a team id is specified.
    if (team != ClassTeamNew_All)
    {
        // Check team and return the corresponding selection setting.
        switch (team)
        {
            case ClassTeamNew_Zombies:
            {
                return zombie;
            }
            case ClassTeamNew_Humans:
            {
                return human;
            }
        }
        
        // Team ID didn't match.
        return false;
    }
    else
    {
        // Check zombie and human.
        return zombie || human;
    }
}

/**
 * Returns whether a string is a reserwed keyword.
 *
 * Note: The string is not trimmed when checked.
 *
 * @return      True if reserved, false otherwise.
 */
stock bool:ClassMgr_IsReservedKeyword(const String:str[])
{
    if (ClassMgr_StringToTeam(str) != ClassTeam_Invalid)
    {
        // It's a team name.
        return true;
    }
    
    if (ClsGeneric_StringToAuthMode(str) != ClsGeneric_InvalidMode)
    {
        // It's a auth mode.
        return true;
    }
    
    return false;
}

/**
 * Logs a generic invalid attribute warning.
 *
 * @param module        Module responsible for the attribute.
 * @param attribute     Name of attribute that failed validation.
 * @param className     Name of the class.
 * @param classIndex    Class index.
 */
stock ClassMgr_LogAttribErrGeneric(Module:module, const String:attribute[], const String:className[], classIndex)
{
    LogMgr_Print(module, LogType_Error, "Config Validation", "Warning: Invalid \"%s\" in class \"%s\" (%d).", attribute, className, classIndex);
}

/**
 * Logs a invalid attribute warning, for string attributes.
 *
 * @param module        Module responsible for the attribute.
 * @param attribute     Name of attribute that failed validation.
 * @param className     Name of the class.
 * @param classIndex    Class index.
 * @param value         Attribute value (string).
 */
stock ClassMgr_LogAttribErrString(Module:module, const String:attribute[], const String:className[], classIndex, const String:value[])
{
    LogMgr_Print(module, LogType_Error, "Config Validation", "Warning: Invalid \"%s\" in class \"%s\" (%d): \"%s\"", attribute, className, classIndex, value);
}

/**
 * Logs a invalid attribute warning, for cell attributes.
 *
 * @param module        Module responsible for the attribute.
 * @param attribute     Name of attribute that failed validation.
 * @param className     Name of the class.
 * @param classIndex    Class index.
 * @param value         Attribute value (cell).
 */
stock ClassMgr_LogAttribErrCell(Module:module, const String:attribute[], const String:className[], classIndex, value)
{
    LogMgr_Print(module, LogType_Error, "Config Validation", "Warning: Invalid \"%s\" in class \"%s\" (%d): %d", attribute, className, classIndex, value);
}

/**
 * Logs a invalid attribute warning, for float attributes.
 *
 * @param module        Module responsible for the attribute.
 * @param attribute     Name of attribute that failed validation.
 * @param className     Name of the class.
 * @param classIndex    Class index.
 * @param value         Attribute value (float).
 */
stock ClassMgr_LogAttribErrFloat(Module:module, const String:attribute[], const String:className[], classIndex, Float:value)
{
    LogMgr_Print(module, LogType_Error, "Config Validation", "Warning: Invalid \"%s\" in class \"%s\" (%d): %0.2f", attribute, className, classIndex, value);
}


/******************************
 *    CONVERSION FUNCTIONS    *
 ******************************/

/**
 * Converts the specified string to a class team.
 *
 * @param team      Team string to convert.
 *
 * @return          Team if successful, otherwise ClassTeam_Invalid.
 */
ClassTeam:ClassMgr_StringToTeam(const String:team[])
{
    if (StrEqual(team, "zombies", false))
    {
        return ClassTeam_Zombies;
    }
    else if (StrEqual(team, "humans", false))
    {
        return ClassTeam_Humans;
    }
    else if (StrEqual(team, "all", false))
    {
        return ClassTeam_All;
    }
    
    return ClassTeam_Invalid;
}

/**
 * Converts the specified team to a string.
 *
 * @param team      Team to convert.
 * @param buffer    Destination string buffer.
 * @param maxlen    Size of buffer.
 *
 * @return          Number of cells written.
 */
ClassMgr_TeamToString(ClassTeam:team, String:buffer[], maxlen)
{
    switch (team)
    {
        case ClassTeam_Zombies: return strcopy(buffer, maxlen, "zombies");
        case ClassTeam_Humans: return strcopy(buffer, maxlen, "humans");
        case ClassTeam_All: return strcopy(buffer, maxlen, "all");
        case ClassTeam_Invalid: return strcopy(buffer, maxlen, "INVALID_TEAM");
    }
    
    return 0;
}

/**
 * Converts the specified class authorization mdoe to a authorization mode for
 * authlib.
 *
 * @param classAuthMode     Class authorization mdoe to convert.
 *
 * @return                  Authlib mode. Or -1 on invalid mode.
 */
Auth_Modes:ClassMgr_ClsAuthModeToAuthMode(ClsGeneric_AuthModes:classAuthMode)
{
    switch (classAuthMode)
    {
        case ClsGeneric_Either: return Auth_Either;
        case ClsGeneric_Both: return Auth_Both;
        case ClsGeneric_All: return Auth_All;
    }

    return Auth_Modes:-1;
}
