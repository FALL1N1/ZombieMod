/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          gamerules.inc
 *  Type:          Module
 *  Description:   Game rule controller.
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
new Module:g_moduleGameRules;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:GameRules_GetIdentifier() { return g_moduleGameRules; }

/**
 * Custom events.
 */
new ProjectEvent:g_EvOnMyCoreShutdown;
new ProjectEvent:g_EvOnMyCoreActivate;

/**
 * Location of gamerules config files.
 */
#define GAMERULES_FILE "configs/zr/gamerules.txt"
#define GAMERULES_MAPS_FILE "configs/zr/gamerules_maps.txt"

/**
 * Available game rule actions.
 */
enum GRuleModuleCmds
{
    GRuleModuleCmd_Invalid = -1,
    GRuleModuleCmd_On,
    GRuleModuleCmd_ForceOn,
    GRuleModuleCmd_Off,
    GRuleModuleCmd_ForceOff
}

/**
 * Array/string sizes for the enum members below.
 */
#define GAMERULES_DATA_NAME 64

/**
 * Array handle for game rulesets.
 */
new Handle:g_hGameRules;

/**
 * The ruleset that will be applied on map start.
 */
new String:g_strNextRuleSet[GAMERULES_DATA_NAME];

/**
 * Game rule data structure.
 */
enum GameRuleSet
{
    String:GameRuleSet_Name[GAMERULES_DATA_NAME],       /** Name of ruleset. */
    Module:GameRuleSet_Core,                            /** The root module for the GRS. */
    Handle:GameRuleSet_Configs,                         /** File paths that point to config files for the game ruleset. */
    Handle:GameRuleSet_ModuleCmds,                      /** Module cmds for the GRS. */
}

/**
 * Cache for the current ruleset that's loaded.
 */
new g_RuleSetCache[GameRuleSet];

/**
 * Readability macros for reading data from the game ruleset cache.
 */
#define GRULESET_NAME               g_RuleSetCache[GameRuleSet_Name]
#define GRULESET_CORE               g_RuleSetCache[GameRuleSet_Core]
#define GRULESET_CONFIGTRIE         g_RuleSetCache[GameRuleSet_Configs]
#define GRULESET_MODULECMDTRIE      g_RuleSetCache[GameRuleSet_ModuleCmds]

/**
 * The number of game rulesets that are configured in the game rules config file.
 */
new g_iGameRuleCount;

/**
 * The currently applied game ruleset.
 */
new g_iCurGameRuleSet = -1;

/**
 * Handle to trie that stores a game mode for a map.
 */
new Handle:g_hGameRulesMaps;

/**
 * Register this module.
 */
GameRules_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Game Rules");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "gamerules");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Manipulates modules and configs to create unique game modes via rulesets.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleGameRules = ModuleMgr_Register(moduledata);
    
    // Create events.
    g_EvOnMyCoreShutdown = EventMgr_CreateEvent("Event_OnMyCoreShutdown");
    g_EvOnMyCoreActivate = EventMgr_CreateEvent("Event_OnMyCoreActivate");
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleGameRules, "Event_OnEventsRegister",         "GameRules_OnEventsRegister");
    
    // Register config file(s) that this module will use.
    ConfigMgr_Register(g_moduleGameRules, "GameRules_OnRuleSetConfigReload", "");
    ConfigMgr_Register(g_moduleGameRules, "GameRules_OnMapRulesCfgReload", "");
}

/**
 * Register all events here.
 */
public GameRules_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleGameRules, "Event_OnPluginEnd",              "GameRules_OnPluginEnd");
    EventMgr_RegisterEvent(g_moduleGameRules, "Event_OnEventsReady",            "GameRules_OnEventsReady");
    EventMgr_RegisterEvent(g_moduleGameRules, "Event_OnModuleEnable",           "GameRules_OnModuleEnable");
    EventMgr_RegisterEvent(g_moduleGameRules, "Event_OnModuleDisable",          "GameRules_OnModuleDisable");
    EventMgr_RegisterEvent(g_moduleGameRules, "Event_OnMyModuleDisable",        "GameRules_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleGameRules, "Event_OnMapStart",               "GameRules_OnMapStart");
    EventMgr_RegisterEvent(g_moduleGameRules, "Event_OnAutoConfigsBuffered",    "GameRules_OnAutoConfigsBuffered");
}

/**
 * All modules and events have been registered by this point.  Event priority can be changed here.
 */
public GameRules_OnEventsReady()
{
    // It's important that GameRules_OnMapStart fires before that of any core modules.
    
    // Game mode configs come before map configs.
    EventMgr_GivePriority("Event_OnAutoConfigsBuffered", g_moduleGameRules, MapConfig_GetIdentifier());
}

/**
 * Plugin is loading.
 */
GameRules_OnPluginStart()
{
    // Register the module.
    GameRules_Register();
    
    // Create commands.
    Project_RegConsoleCmd("nextruleset", GameRules_NextRuleSet, "Set the ruleset to be applied for next map.  Usage: <prefix>_nextruleset <ruleset>");
    Project_RegConsoleCmd("setmapruleset", GameRules_SetMapRuleSet, "Define a given ruleset/game mode to be loaded for a map.  Usage: <prefix>_setmapruleset <map> <ruleset>");
    
    // Create array.
    g_hGameRules = CreateArray(sizeof(g_RuleSetCache));
    
    // Create map trie.
    g_hGameRulesMaps = CreateTrie();
}

/**
 * Plugin is ending.
 */
public GameRules_OnPluginEnd()
{
    // Clean up data.
    GameRules_CleanCache();
    CloseHandle(g_hGameRules);
    CloseHandle(g_hGameRulesMaps);
}

/**
 * A module has been enabled.
 * 
 * @return      Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:GameRules_OnModuleEnable(Module:module)
{
    // Ignore this module.
    if (module == g_moduleGameRules)
        return Plugin_Continue;
    
    // If no game ruleset is applied yet, then let it be enabled.
    if (g_iCurGameRuleSet == -1)
        return Plugin_Continue;
    
    // Don't allow a root module that isn't defined by the current game mode be enabled.
    if (ModuleMgr_ReadCell(module, ModuleData_Root) && module != GRULESET_CORE)
        return Plugin_Handled;
    
    // If this module is forced off then don't let it enable.
    decl String:moduleshortname[MM_DATA_SHORTNAME];
    ModuleMgr_ReadString(module, ModuleData_ShortName, moduleshortname, sizeof(moduleshortname));
    if (GameRules_GetModuleCmd(moduleshortname) == GRuleModuleCmd_ForceOff)
        return Plugin_Handled;
    
    return Plugin_Continue;
}

/**
 * A module has been disabled.
 * 
 * @return      Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:GameRules_OnModuleDisable(Module:module)
{
    // Ignore this module, it's already handled in GameRules_OnMyModuleDisable
    if (module == g_moduleGameRules)
        return Plugin_Continue;
    
    // If no game ruleset is applied yet, then let it be disabled.
    if (g_iCurGameRuleSet == -1)
        return Plugin_Continue;
    
    // Don't allow a root module defined by the current game mode be disabled.
    if (module == GRULESET_CORE)
        return Plugin_Handled;
    
    // If this module is forced on then don't let it disable.
    decl String:moduleshortname[MM_DATA_SHORTNAME];
    ModuleMgr_ReadString(module, ModuleData_ShortName, moduleshortname, sizeof(moduleshortname));
    if (GameRules_GetModuleCmd(moduleshortname) == GRuleModuleCmd_ForceOn)
        return Plugin_Handled;
    
    return Plugin_Continue;
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:GameRules_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    Format(refusalmsg, maxlen, "%T", "Game Rules refuse disable", LANG_SERVER);
    return Plugin_Handled;
}

/**
 * Goes through all rulesets and destroys the handles.
 */
static stock GameRules_CleanCache()
{
    // Cleanup tries in gamerules data.
    new gameRuleSet[GameRuleSet];
    for (new ruleSet = 0; ruleSet < g_iGameRuleCount; ruleSet++)
    {
        GetArrayArray(g_hGameRules, ruleSet, gameRuleSet[0], sizeof(gameRuleSet));
        
        if (gameRuleSet[GameRuleSet_Configs] != INVALID_HANDLE)
            CloseHandle(gameRuleSet[GameRuleSet_Configs]);
        
        if (gameRuleSet[GameRuleSet_ModuleCmds] != INVALID_HANDLE)
            CloseHandle(gameRuleSet[GameRuleSet_ModuleCmds]);
    }
}

/**
 * Loops through each section of the keyvalues tree.
 * 
 * @param kv            The keyvalues handle of the config file. (Don't close this)
 * @param sectionindex  The index of the current keyvalue section, starting from 0.
 * @param sectionname   The name of the current keyvalue section.
 * 
 * @return              See enum KvCache.
 */
public KvCache:GameRules_Cache(Handle:kv, sectionindex, const String:sectionname[])
{
    new gameRuleSet[GameRuleSet];
    
    // Section name = game ruleset name.
    strcopy(gameRuleSet[GameRuleSet_Name], GAMERULES_DATA_NAME, sectionname);
    
    // Store data from known key names.
    
    // Get value and validate for "core"
    decl String:rootmodulename[MM_DATA_SHORTNAME];
    KvGetString(kv, "core", rootmodulename, sizeof(rootmodulename));
    new Module:rootmodule = ModuleMgr_FindByString(ModuleData_ShortName, rootmodulename);
    if (rootmodule != INVALID_MODULE)
    {
        gameRuleSet[GameRuleSet_Core] = rootmodule;
    }
    else
    {
        gameRuleSet[GameRuleSet_Core] = INVALID_MODULE;
        LogMgr_Print(g_moduleGameRules, LogType_Error, "Config Validation", "No core specified for game ruleset \"%s\"!", sectionname);
    }
    
    // Go down a level and store config path data.
    gameRuleSet[GameRuleSet_Configs] = CreateTrie();
    if (KvJumpToKey(kv, "configs"))
    {
        decl String:name[MM_DATA_SHORTNAME];
        decl String:strFilePath[PLATFORM_MAX_PATH];
        if (KvGotoFirstSubKey(kv, false))
        {
            do
            {
                KvGetSectionName(kv, name, sizeof(name));
                KvGoBack(kv);
                KvGetString(kv, name, strFilePath, sizeof(strFilePath));
                KvJumpToKey(kv, name);
                
                if (!SetTrieString(gameRuleSet[GameRuleSet_Configs], name, strFilePath, false))
                {
                    LogMgr_Print(g_moduleGameRules, LogType_Error, "Config Validation", "Duplicate entry in ruleset \"%s\" section \"configs\": \"%s\".  The plugin had to break, meaning config paths defined after this one are ignored.", sectionname, name);
                    break;
                }
            } while (KvGotoNextKey(kv, false));
            KvGoBack(kv);   // Go back to 2nd level.
        }
        
        // Go back to 1st level.
        KvGoBack(kv);
    }
    else
    {
        LogMgr_Print(g_moduleGameRules, LogType_Normal, "Config Validation", "Warning: Ruleset \"%s\" is missing the \"config\" section.  See gamerules config file.", sectionname);
    }
    
    // Go down a level and store module command data.
    gameRuleSet[GameRuleSet_ModuleCmds] = CreateTrie();
    if (KvJumpToKey(kv, "modulecmds"))
    {
        decl String:moduleshortname[MM_DATA_SHORTNAME];
        decl String:strModuleCmd[64];
        new Module:module;
        new GRuleModuleCmds:modulecmd;
        if (KvGotoFirstSubKey(kv, false))
        {
            do
            {
                KvGetSectionName(kv, moduleshortname, sizeof(moduleshortname));
                KvGoBack(kv);
                KvGetString(kv, moduleshortname, strModuleCmd, sizeof(strModuleCmd));
                KvJumpToKey(kv, moduleshortname);
                
                // Get the module identifier.
                module =  ModuleMgr_FindByString(ModuleData_ShortName, moduleshortname);
                
                // Check if module is valid.
                if (module != INVALID_MODULE)
                {
                    // Validate and set rule action.
                    modulecmd = GameRules_ModuleCmdToSymbol(strModuleCmd);
                    if (modulecmd != GRuleModuleCmd_Invalid)
                    {
                        if (!SetTrieValue(gameRuleSet[GameRuleSet_ModuleCmds], moduleshortname, modulecmd, false))
                            LogMgr_Print(g_moduleGameRules, LogType_Error, "Config Validation", "Duplicate entry in ruleset \"%s\" section \"modulecmds\": \"%s\"", sectionname, moduleshortname);
                    }
                    else
                    {
                        LogMgr_Print(g_moduleGameRules, LogType_Error, "Config Validation", "Invalid module command in ruleset \"%s\": \"%s\".", sectionname, strModuleCmd);
                    }
                }
                else
                {
                    LogMgr_Print(g_moduleGameRules, LogType_Error, "Config Validation", "Invalid module in rule \"%s\": \"%s\".", sectionname, moduleshortname);
                }
                
            } while (KvGotoNextKey(kv, false));
            KvGoBack(kv);   // Go back to 2nd level.
        }
        KvGoBack(kv); // Go back to 1st level.
    }
    
    // Push all this collected data to an array.
    PushArrayArray(g_hGameRules, gameRuleSet[0], sizeof(gameRuleSet));
    
    return KvCache_Continue;
}

/**
 * Re-cache all game rules data from disk.
 */
GameRules_CacheGameRulesData()
{
    if (ConfigMgr_ValidateFile(GAMERULES_FILE))
    {
        ConfigMgr_WriteString(g_moduleGameRules, CM_CONFIGINDEX_FIRST, ConfigData_Path, CM_DATA_PATH, GAMERULES_FILE);
    }
    else
    {
        LogMgr_Print(g_moduleGameRules, LogType_Fatal_Plugin, "Config Validation", "Error: Missing critical gamerules config file: \"%s\"", GAMERULES_FILE);
        return;
    }
    
    GameRules_CleanCache();
    ClearArray(g_hGameRules);
    g_iGameRuleCount = ConfigMgr_CacheKv(g_moduleGameRules, CM_CONFIGINDEX_FIRST, "GameRules_Cache");
    
    // There were no game rulesets configured.
    if (g_iGameRuleCount == 0)
        LogMgr_Print(g_moduleGameRules, LogType_Fatal_Module, "Config Validation", "Error: No usable data found in game rules config file: \"%s\"", GAMERULES_FILE);
}

/**
 * Re-cache all map game rules data from disk.
 */
GameRules_CacheMapRulesData()
{
    if (ConfigMgr_ValidateFile(GAMERULES_MAPS_FILE))
    {
        ConfigMgr_WriteString(g_moduleGameRules, CM_CONFIGINDEX_SECOND, ConfigData_Path, CM_DATA_PATH, GAMERULES_MAPS_FILE);
    }
    else
    {
        LogMgr_Print(g_moduleGameRules, LogType_Error, "Config Validation", "Warning: Missing map game rules config file: \"%s\"", GAMERULES_MAPS_FILE);
        return;
    }
    
    // Get data from file.
    //ClearTrie(g_hGameRulesMaps);
    new count;
    decl String:line[256];
    new Handle:hMapRules = ConfigMgr_CacheFile(g_moduleGameRules, CM_CONFIGINDEX_SECOND, sizeof(line), count);
    
    // Transfer and validate this data.
    decl String:map[128], String:gamemode[128];
    new breakpoint;
    for (new i = 0; i < count; i++)
    {
        GetArrayString(hMapRules, i, line, sizeof(line));
        breakpoint = BreakString(line, map, sizeof(map));
        if (breakpoint == -1)
        {
            LogMgr_Print(g_moduleGameRules, LogType_Error, "Config Validation", "Error: Invalid map rule (%s) in map game rules config file: \"%s\"", line, GAMERULES_MAPS_FILE);
            continue;
        }
        BreakString(line[breakpoint], gamemode, sizeof(gamemode));
        if (GameRules_FindRuleSet(gamemode) == -1)
        {
            LogMgr_Print(g_moduleGameRules, LogType_Error, "Config Validation", "Error: Invalid rule set/game mode (%s) in map game rules config file: \"%s\"", gamemode, GAMERULES_MAPS_FILE);
            continue;
        }
        SetTrieString(g_hGameRulesMaps, map, gamemode);
    }
}

/**
 * Check if game rules has ever been cached.
 * 
 * @return  True if it has, false if not.
 */
stock bool:GameRules_IsConfigCached()
{
    return (g_iGameRuleCount > 0);
}

/**
 * The map has started.
 */
public GameRules_OnMapStart()
{
    new bool:wasCached = GameRules_IsConfigCached();
    
    GameRules_CacheGameRulesData();
    GameRules_CacheMapRulesData();
    
    // The config hasn't been cached yet, so cache it and load the first ruleset.
    if (!wasCached)
    {
        GameRules_LoadRuleSet();
        LogMgr_Print(g_moduleGameRules, LogType_Normal, "Ruleset", "Applied default ruleset: \"%s\"", GRULESET_NAME);
        return;
    }
    
    // If this global variable is non-null, then use this as the ruleset.
    if (g_strNextRuleSet[0])
    {
        if (GameRules_FindRuleSet(g_strNextRuleSet) != -1)
        {
            GameRules_LoadRuleSet(g_strNextRuleSet);
            LogMgr_Print(g_moduleGameRules, LogType_Normal, "Ruleset", "Applied ruleset \"%s\" as defined by zr_nextruleset.", GRULESET_NAME);
        }
        else
        {
            GameRules_LoadRuleSet();
            LogMgr_Print(g_moduleGameRules, LogType_Normal, "Ruleset", "Invalid ruleset defined in zr_nextruleset (%s), falling back on default ruleset: \"%s\"", g_strNextRuleSet, GRULESET_NAME);
        }
        g_strNextRuleSet[0] = '\0';
        return;
    }
    
    // Next check if a game mode is defined for the current map.
    decl String:map[128];
    GetCurrentMap(map, sizeof(map));
    
    decl String:ruleset[128];
    if (GetTrieString(g_hGameRulesMaps, map, ruleset, sizeof(ruleset)))
    {
        if (!StrEqual(GRULESET_NAME, ruleset, false))
        {
            GameRules_LoadRuleSet(ruleset);
            LogMgr_Print(g_moduleGameRules, LogType_Normal, "Ruleset", "Applied ruleset \"%s\" as defined by current map.", GRULESET_NAME);
            return;
        }
    }
    
    // Transfer the cached data to another static array cache that holds the current game mode.
    // And if this game mode no longer exists, load the default.
    new rulesetindex = GameRules_FindRuleSet(GRULESET_NAME);
    if (rulesetindex != -1)
        GameRules_UpdateCache(rulesetindex);
    else
    {
        GameRules_LoadRuleSet();
        LogMgr_Print(g_moduleGameRules, LogType_Normal, "Ruleset", "The current ruleset was removed from the configs, falling back on default ruleset: \"%s\"", GRULESET_NAME);
        return;
    }
}

/**
 * This is called before OnConfigsExecuted but any time after OnMapStart.
 * Per-map settings should be set here. 
 */
public GameRules_OnAutoConfigsBuffered()
{
    // Execute this ruleset's cfg file.
    decl String:cfgfile[PLATFORM_MAX_PATH];
    GameRules_GetCvarConfigPath(cfgfile);
    if (cfgfile[0])
    {
        ServerCommand("exec %s", cfgfile);
        LogMgr_Print(g_moduleGameRules, LogType_Debug, "Game Rule Configs", "Executed ruleset cfg file: %s", cfgfile);
    }
}

/**
 * Called when a registered config file (by this module) is manually.
 */
public GameRules_OnRuleSetConfigReload(configindex)
{
    TransMgr_PrintText(SERVER_INDEX, MsgFormat_Plugin, MsgType_Reply, g_moduleGameRules, false, "Game Rules config reload", GAMERULES_FILE);
}

/**
 * Called when a registered config file (by this module) is manually.
 */
public GameRules_OnMapRulesCfgReload(configindex)
{
    GameRules_CacheMapRulesData();
}

/**
 * Returns the cfg file path from gamerules cache.
 * 
 * @param path  The output buffer.
 * 
 * @return      True if got successfully, false if it doesn't exist.
 */
public bool:GameRules_GetCvarConfigPath(String:path[PLATFORM_MAX_PATH])
{
    return GetTrieString(GRULESET_CONFIGTRIE, "cvars", path, sizeof(path));
}

/**
 * Reads the cfg folder path from gamerules cache and appends the filename given.
 * 
 * @param file  The name of the file in ZR config folder.
 * @param path  The output buffer.
 * 
 * @return      True if got successfully, false if it doesn't exist.
 */
public bool:GameRules_GetConfigPath(const String:file[], String:path[PLATFORM_MAX_PATH])
{
    if (!GetTrieString(GRULESET_CONFIGTRIE, "cfgfolder", path, sizeof(path)))
        return false;
    
    decl String:fullpath[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, fullpath, sizeof(fullpath), path);
    
    decl String:mapname[256];
    decl String:mapcfgpath[PLATFORM_MAX_PATH];
    GetCurrentMap(mapname, sizeof(mapname));
    
    // First priority: check if a folder with the same name as the map exists with this config file in it.
    Format(mapcfgpath, sizeof(mapcfgpath), "%s%s/%s", fullpath, mapname, file);
    if (FileExists(mapcfgpath))
    {
        Format(path, sizeof(path), "%s%s/%s", path, mapname, file);
        return true;
    }
    
    // Second priority: check partial map name matches.
    new Handle:hDir = OpenDirectory(fullpath);
    decl String:foldername[256];
    new FileType:filetype;
    while (ReadDirEntry(hDir, foldername, sizeof(foldername), filetype))
    {
        if (filetype != FileType_Directory)
            continue;
        
        // Check if a folder partially matches the current map.
        if (StrContains(mapname, foldername, false) > -1)
        {
            Format(mapcfgpath, sizeof(mapcfgpath), "%s%s/%s", fullpath, foldername, file);
            if (FileExists(mapcfgpath))
            {
                Format(path, sizeof(path), "%s%s/%s", path, foldername, file);
                return true;
            }
        }
    }
    
    // Third priority: return where default config should be.  If it's not there the caller should handle it.
    Format(path, sizeof(path), "%s%s", path, file);
    return true;
}

/**
 * Returns the module cmd associated with the given module name in the game ruleset under the "modulecmd" section.
 * 
 * @param name  The name of the module.
 * 
 * @return      The module cmd.
 */
stock GRuleModuleCmds:GameRules_GetModuleCmd(const String:moduleshortname[])
{
    new GRuleModuleCmds:value = GRuleModuleCmd_Invalid;
    GetTrieValue(GRULESET_MODULECMDTRIE, moduleshortname, _:value);
    return value;
}

/**
 * Converts a string value to a module cmd symbol.
 * 
 * @param modulecmd     String value to convert to the appropriate GRuleModuleCmds symbol.
 * 
 * @return              A member of enum GRuleModuleCmds, GRuleModuleCmd_Invalid if no symbol matches.
 */
stock GRuleModuleCmds:GameRules_ModuleCmdToSymbol(const String:modulecmd[])
{
    if (StrEqual(modulecmd, "on", false))
    {
        return GRuleModuleCmd_On;
    }
    else if (StrEqual(modulecmd, "force_on", false))
    {
        return GRuleModuleCmd_ForceOn;
    }
    else if (StrEqual(modulecmd, "off", false))
    {
        return GRuleModuleCmd_Off;
    }
    else if (StrEqual(modulecmd, "force_off", false))
    {
        return GRuleModuleCmd_ForceOff;
    }
    
    return GRuleModuleCmd_Invalid;
}

/**
 * Loads a named ruleset.
 * 
 * @param ruleset   The ruleset as listed in gamerules config file.  Pass a null string to load first ruleset in the config file.
 * 
 * @return          The rule set index.  
 */
stock GameRules_LoadRuleSet(const String:ruleset[] = "")
{
    // Find the index of the named ruleset.  -1 is returned if invalid.
    new rulesetindex = ruleset[0] ? GameRules_FindRuleSet(ruleset) : 0;
    
    // Use the first ruleset if this one is invalid.
    if (rulesetindex == -1)
    {
        new gameRuleSet[GameRuleSet];
        GetArrayArray(g_hGameRules, 0, gameRuleSet[0], sizeof(gameRuleSet));
        rulesetindex = GameRules_FindRuleSet(gameRuleSet[GameRuleSet_Name]);
        
        if (ruleset[0])
            LogMgr_Print(g_moduleGameRules, LogType_Error, "Config Validation", "Invalid ruleset name \"%s\", falling back to first defined ruleset \"%s\".", ruleset, gameRuleSet[GameRuleSet_Name]);
    }
    GameRules_ApplyRuleSet(rulesetindex);
    return rulesetindex;
}

/**
 * Gets the ruleset with the specified name.
 *
 * @param name  Name of ruleset.
 *  
 * @return      ruleset index if found, -1 otherwise.
 */
stock GameRules_FindRuleSet(const String:name[])
{
    new gameRuleSet[GameRuleSet];
    for (new ruleSet = 0; ruleSet < g_iGameRuleCount; ruleSet++)
    {
        GetArrayArray(g_hGameRules, ruleSet, gameRuleSet[0], sizeof(gameRuleSet));
        if (StrEqual(gameRuleSet[GameRuleSet_Name], name, false))
            return ruleSet;
    }
    
    return -1;
}

/**
 * Finds a module in a rule set.
 *
 * @param ruleset   The game ruleset index to look for module in.
 * @param module    The module to look for.
 * 
 * @return          Index that module lies in.
 */
stock GameRules_FindModuleInRuleSet(ruleset, Module:module)
{
    for (new moduleindex = 0; moduleindex < GRULESET_NUMMODULES; moduleindex++)
    {
        if (module == GRULESET_MODULE(moduleindex))
            return moduleindex;
    }
    
    return -1;
}

/**
 * Get the name of a rule set given its index.
 * 
 * @param ruleset   The index of the ruleset to get name of.
 * @param name      The name of the ruleset. (output)
 * @param maxlen    The max size of the output.
 * 
 * @return          True if found, false if not found.
 */
stock GameRules_GetRuleSetName(ruleset, String:name[], maxlen)
{
    new gameRuleSet[GameRuleSet];
    GetArrayArray(g_hGameRules, ruleset, gameRuleSet[0], sizeof(gameRuleSet));
    strcopy(name, maxlen, gameRuleSet[GameRuleSet_Name]);
}

/**
 * Applies a ruleset and sets the module state (enable or disable them)
 * according to the ruleset.
 *
 * @param ruleSet   Index of ruleset to apply.
 */
stock GameRules_ApplyRuleSet(ruleSet)
{
    // Call a shutdown event on the current game mode and store the name for later.
    new any:eventdata[sizeof(g_CommonEventData1)][1];
    if (g_iCurGameRuleSet != -1 && GRULESET_CORE != INVALID_MODULE)
    {
        eventdata[0][0] = GRULESET_CORE;
        EventMgr_Forward(g_EvOnMyCoreShutdown, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    }
    
    GameRules_UpdateCache(ruleSet);
    g_iCurGameRuleSet = ruleSet;    // Hiding behind this line above in case ruleSet is invalid.
    
    // Disable all other cores.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        if (ModuleMgr_ReadCell(Module:moduleindex, ModuleData_Root) && Module:moduleindex != GRULESET_CORE)
            ModuleMgr_Disable(Module:moduleindex);
    }
    
    // Enable the core's root module.
    if (GRULESET_CORE != INVALID_MODULE)
        ModuleMgr_Enable(GRULESET_CORE);
    
    // Loop through each module and apply its action.
    // Loop through all the modules.
    decl String:moduleshortname[MM_DATA_SHORTNAME];
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // Get module's short name to look up.
        ModuleMgr_ReadString(Module:moduleindex, ModuleData_ShortName, moduleshortname, sizeof(moduleshortname));
        
        // Apply action.
        switch (GameRules_GetModuleCmd(moduleshortname))
        {
            case GRuleModuleCmd_On:         ModuleMgr_Enable(Module:moduleindex);
            case GRuleModuleCmd_ForceOn:    ModuleMgr_Enable(Module:moduleindex);
            case GRuleModuleCmd_Off:        ModuleMgr_Disable(Module:moduleindex);
            case GRuleModuleCmd_ForceOff:   ModuleMgr_Disable(Module:moduleindex);
        }
    }
    
    // Send this event only to the module being affected.
    if (GRULESET_CORE != INVALID_MODULE)
    {
        eventdata[0][0] = GRULESET_CORE;
        EventMgr_Forward(g_EvOnMyCoreActivate, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    }
}

/**
 * Updates the cache given a ruleset index.
 * 
 * @ruleset The ruleset index.
 */
GameRules_UpdateCache(ruleset)
{
    GetArrayArray(g_hGameRules, ruleset, g_RuleSetCache[0], sizeof(g_RuleSetCache));
}

/**
 * Command callback (zr_nextruleset)
 * Set the ruleset to be applied for next map.
 * 
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:GameRules_NextRuleSet(client, argc)
{
    // Check if client has access.
    if (!AccessMgr_HasAccess(client, g_moduleGameRules))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No access to command");
        return Plugin_Handled;
    }
    
    // If not enough arguments given, then stop.
    if (argc < 1)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Game Rules nextruleset command syntax", PROJECT_CMD_PREFIX);
        return Plugin_Handled;
    }
    
    // Validate the input.
    decl String:arg1[GAMERULES_DATA_NAME];
    GetCmdArg(1, arg1, sizeof(arg1));
    if (GameRules_FindRuleSet(arg1) != -1)
    {
        strcopy(g_strNextRuleSet, sizeof(g_strNextRuleSet), arg1);
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleGameRules, false, "Game Rules next ruleset", arg1);
        LogMgr_Print(g_moduleGameRules, LogType_Normal, "zr_nextruleset", "%N queued rule set \"%s\" to be loaded with the next map.", client, arg1);
    }
    else
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleGameRules, false, "Game Rules invalid ruleset", arg1);
        LogMgr_Print(g_moduleGameRules, LogType_Normal, "zr_nextruleset", "%N attempted to set zr_nextruleset to an invalid value: \"%s\"", client, arg1);
    }
    
    return Plugin_Handled;
}

/**
 * Command callback (zr_setmapruleset)
 * Define a given ruleset/game mode to be loaded for a map.
 * 
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:GameRules_SetMapRuleSet(client, argc)
{
    // Check if client has access.
    if (!AccessMgr_HasAccess(client, g_moduleGameRules))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No access to command");
        return Plugin_Handled;
    }
    
    // If not enough arguments given, then stop.
    if (argc < 1)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Game Rules setmapruleset command syntax", PROJECT_CMD_PREFIX);
        return Plugin_Handled;
    }
    
    // Get and validate the input.
    decl String:arg1[128], String:arg2[GAMERULES_DATA_NAME];
    GetCmdArg(1, arg1, sizeof(arg1));
    GetCmdArg(2, arg2, sizeof(arg2));
    if (GameRules_FindRuleSet(arg2) != -1)
    {
        SetTrieString(g_hGameRulesMaps, arg1, arg2);
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleGameRules, false, "Game Rules set map ruleset", arg2, arg1);
        LogMgr_Print(g_moduleGameRules, LogType_Normal, "zr_setmapruleset", "%N set map \"%s\" to load ruleset \"%s\"", client, arg1, arg2);
    }
    else
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleGameRules, false, "Game Rules invalid ruleset", arg2);
        LogMgr_Print(g_moduleGameRules, LogType_Normal, "zr_setmapruleset", "%N attempted to define an invalid game mode (%s) for map: \"%s\"", client, arg2, arg1);
    }
    
    return Plugin_Handled;
}
