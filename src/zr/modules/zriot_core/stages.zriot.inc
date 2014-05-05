/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          stages.zriot.inc
 *  Type:          Module
 *  Description:   Handles and applies stage configs.
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
 * The path to the stages config file, relative to the path defined in gamerules.txt.
 */
#define STAGES_CONFIG_FILE "stages.txt"

/**
 * This module's identifier.
 */
new Module:g_moduleZRiotStages;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ZRiotStages_GetIdentifier() { return g_moduleZRiotStages; }

/**
 * Cvar handles.
 */
new Handle:g_hCvarZRiotStagesRegress;

/**
 * Max string length values.
 */
#define STAGE_DATA_CFG PLATFORM_MAX_PATH
#define STAGE_DATA_MSGS 192

/**
 * What each index in the ZStage_Messages array means.
 */
#define STAGE_MSGS_GLOBAL_START 0
#define STAGE_MSGS_GLOBAL_ZWIN 1
#define STAGE_MSGS_GLOBAL_HWIN 2
#define STAGE_MSGS_ZOMBIE_START 3
#define STAGE_MSGS_ZOMBIE_ZWIN 4
#define STAGE_MSGS_ZOMBIE_HWIN 5
#define STAGE_MSGS_HUMAN_START 6
#define STAGE_MSGS_HUMAN_ZWIN 7
#define STAGE_MSGS_HUMAN_HWIN 8

/**
 * Data for a stage.
 */
enum ZRiotStage
{
    Handle:ZStage_Messages,             // Messages that are printed for each stage at round start or when a round is won.
    ZStage_Count,                       // Number of zombies to kill for this stage.
    bool:ZStage_Respawn,                // Should zombies continuously spawn with force end round when last zombie is killed
                                        // or will it stop respawning when the number of bots in-game is <= number of zombies to kill.
    String:ZStage_Cfg[STAGE_DATA_CFG],  // Path to config file relative to cfg/ folder.
    ZStage_ZombiePlayerMax,             // Max number of players to allow on zombie team.
    ZStage_ZombieAliveMax,              // Max number of live zombies at any moment.
    Handle:ZStage_ZombieAttributes,     // A trie containing attribute modifiers for zombies.
    ZStage_HumanPlayerMax,              // ^^ for humans.
    ZStage_HumanAliveMax,               //
    Handle:ZStage_HumanAttributes       //
}

/**
 * Array to store all stage data.
 */
new Handle:g_hZRiotStageData;

/**
 * The number of stages that are defined in the stages config file.
 */
new g_iZRiotStageCount;

/**
 * Variable/function set to check/set if zombies are present.
 */
new g_iCurStageIndex;
stock ZRiotStages_GetCurrentStage() { return g_iCurStageIndex; }
stock ZRiotStages_SetCurrentStage(stage) { g_iCurStageIndex = stage; }

/**
 * Register this module.
 */
ZRiotStages_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZRiot Stages");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "zriotstages");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Handles and applies stage configs.");
    moduledata[ModuleData_Dependencies][0] = ZRiot_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleZRiotStages = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZRiotStages, "Event_OnEventsRegister",        "ZRiotStages_OnEventsRegister");
    
    // Register config file(s) that this module will use.
    ConfigMgr_Register(g_moduleZRiotStages, "ZRiotStages_OnConfigReload", "");
}

/**
 * Register all events here.
 */
public ZRiotStages_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleZRiotStages, "Event_OnPluginEnd",            "ZRiotStages_OnPluginEnd");
    EventMgr_RegisterEvent(g_moduleZRiotStages, "Event_OnMyModuleEnable",       "ZRiotStages_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleZRiotStages, "Event_OnMyModuleDisable",      "ZRiotStages_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleZRiotStages, "Event_OnMapStart",             "ZRiotStages_OnMapStart");
    
    EventMgr_RegisterEvent(g_moduleZRiotStages, "Event_RoundStart",              "ZRiotStages_RoundStart");
    //EventMgr_RegisterEvent(g_moduleZRiotStages, "Event_RoundFreezeEnd",          "ZRiotStages_RoundFreezeEnd");
    EventMgr_RegisterEvent(g_moduleZRiotStages, "Event_RoundEnd",                "ZRiotStages_RoundEnd");
}

/**
 * Plugin is loading.
 */
ZRiotStages_OnPluginStart()
{
    // Register the module.
    ZRiotStages_Register();
    
    // Create cvars.
    g_hCvarZRiotStagesRegress = CreateConVar("zriot_stages_regress", "1", "Whether or not to go to the previous stage if humans lose the round.");
    
    // Create array.
    g_hZRiotStageData = CreateArray(sizeof(g_RuleSetCache));
}

/**
 * Plugin is ending.
 */
public ZRiotStages_OnPluginEnd()
{
    // Clean up data.
    ZRiotStages_CleanCache();
    CloseHandle(g_hZRiotStageData);
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:ZRiotStages_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // Create array.
    g_hZRiotStageData = CreateArray(sizeof(g_RuleSetCache));
    
    // Don't let the module load unless the stages data is cached successfully.
    if (!ZRiotStages_CacheData())
    {
        decl String:configfile[PLATFORM_MAX_PATH];
        GameRules_GetConfigPath(STAGES_CONFIG_FILE, configfile);
        Format(refusalmsg, maxlen, "%T", "ZRiot stages refuse enable", LANG_SERVER, configfile);
        return Plugin_Handled;
    }
    
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
public Action:ZRiotStages_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    ZRiotStages_CleanCache();
    CloseHandle(g_hZRiotStageData);
}

/**
 * The map has started.
 */
public ZRiotStages_OnMapStart()
{
    g_iCurStageIndex = 0;   // Reset current stage.
    if (!ZRiotStages_CacheData())
        LogMgr_Print(g_moduleZRiotStages, LogType_Error, "Config Validation", "Error loading stage data, there will be no stages in-game!");
}

// ---------------------------------
//     Data storage functions
// ---------------------------------

/**
 * Check if the stage config has ever been cached.
 * 
 * @return  True if it has, false if not.
 */
stock bool:ZRiotStages_IsConfigCached()
{
    return (g_iZRiotStageCount > 0);
}

static stock ZRiotStages_CleanCache()
{
    // Cleanup tries in stage data.
    new stagedata[ZRiotStage];
    for (new sindex = 0; sindex < g_iZRiotStageCount; sindex++)
    {
        GetArrayArray(g_hZRiotStageData, sindex, stagedata[0], sizeof(stagedata));
        
        if (stagedata[ZStage_ZombieAttributes] != INVALID_HANDLE)
            CloseHandle(stagedata[ZStage_ZombieAttributes]);
        
        if (stagedata[ZStage_HumanAttributes] != INVALID_HANDLE)
            CloseHandle(stagedata[ZStage_HumanAttributes]);
        
        if (stagedata[ZStage_Messages] != INVALID_HANDLE)
            CloseHandle(stagedata[ZStage_Messages]);
    }
}

/**
 * Private function used by ZRiotStages_Cache to avoid duplicate code.
 */
static stock Handle:ZRiotStages_CacheAttributes(Handle:kv, const String:stage[], const String:team[])
{
    new Handle:attributes = CreateTrie();
    if (KvJumpToKey(kv, "attributes"))
    {
        decl String:attribute[64];
        decl String:modifier[8];
        if (KvGotoFirstSubKey(kv, false))
        {
            do
            {
                KvGetSectionName(kv, attribute, sizeof(attribute));
                KvGoBack(kv);
                KvGetString(kv, attribute, modifier, sizeof(modifier));
                KvJumpToKey(kv, attribute);
                
                if (!SetTrieString(attributes, attribute, modifier, false))
                {
                    LogMgr_Print(g_moduleZRiotStages, LogType_Error, "Config Validation", "Duplicate attribute modifier for stage \"%s\" in \"%s\" section.  The plugin had to break, meaning attributes defined after this one are ignored.", stage, team);
                    break;
                }
            } while (KvGotoNextKey(kv, false));
            KvGoBack(kv);
        }
        KvGoBack(kv);
    }
    return attributes;
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
public KvCache:ZRiotStages_Cache(Handle:kv, sectionindex, const String:sectionname[])
{
    // Stores all data about a single stage.
    new stagedata[ZRiotStage];
    
    // Non-team-specific data.
    decl String:buffer[STAGE_DATA_MSGS];
    
    // Store messages.
    stagedata[ZStage_Messages] = CreateArray(STAGE_DATA_MSGS, 9);
    KvGetString(kv, "roundstart", buffer, sizeof(buffer));
    SetArrayString(stagedata[ZStage_Messages], STAGE_MSGS_GLOBAL_START, buffer);
    KvGetString(kv, "zombiewin", buffer, sizeof(buffer));
    SetArrayString(stagedata[ZStage_Messages], STAGE_MSGS_GLOBAL_ZWIN, buffer);
    KvGetString(kv, "humanwin", buffer, sizeof(buffer));
    SetArrayString(stagedata[ZStage_Messages], STAGE_MSGS_GLOBAL_HWIN, buffer);
    
    stagedata[ZStage_Count] = KvGetNum(kv, "count");
    stagedata[ZStage_Respawn] = bool:TransMgr_KvPhraseToBoolEx(kv, BoolPhrase_YesNo, "respawn", "yes");
    KvGetString(kv, "cfg", stagedata[ZStage_Cfg], sizeof(stagedata[ZStage_Cfg]));
    
    // Zombie data.
    if (KvJumpToKey(kv, "zombies"))
    {
        // Store messages.
        KvGetString(kv, "roundstart", buffer, sizeof(buffer));
        SetArrayString(stagedata[ZStage_Messages], STAGE_MSGS_ZOMBIE_START, buffer);
        KvGetString(kv, "zombiewin", buffer, sizeof(buffer));
        SetArrayString(stagedata[ZStage_Messages], STAGE_MSGS_ZOMBIE_ZWIN, buffer);
        KvGetString(kv, "humanwin", buffer, sizeof(buffer));
        SetArrayString(stagedata[ZStage_Messages], STAGE_MSGS_ZOMBIE_HWIN, buffer);
        
        stagedata[ZStage_ZombiePlayerMax] = KvGetNum(kv, "playermax");
        stagedata[ZStage_ZombieAliveMax] = KvGetNum(kv, "alivemax");
        stagedata[ZStage_ZombieAttributes] = ZRiotStages_CacheAttributes(kv, sectionname, "zombies");
        KvGoBack(kv);  // Jump back to previous level.
    }
    else
    {
        LogMgr_Print(g_moduleZRiotStages, LogType_Error, "Config Validation", "Missing \"zombies\" section for stage \"%s\".  This stage will be disregarded until fixed.", sectionname);
        return KvCache_Ignore;
    }
    
    // Human data.
    if (KvJumpToKey(kv, "humans"))
    {
        // Store messages.
        KvGetString(kv, "roundstart", buffer, sizeof(buffer));
        SetArrayString(stagedata[ZStage_Messages], STAGE_MSGS_HUMAN_START, buffer);
        KvGetString(kv, "zombiewin", buffer, sizeof(buffer));
        SetArrayString(stagedata[ZStage_Messages], STAGE_MSGS_HUMAN_ZWIN, buffer);
        KvGetString(kv, "humanwin", buffer, sizeof(buffer));
        SetArrayString(stagedata[ZStage_Messages], STAGE_MSGS_HUMAN_HWIN, buffer);
        
        stagedata[ZStage_HumanPlayerMax] = KvGetNum(kv, "playermax");
        stagedata[ZStage_HumanAliveMax] = KvGetNum(kv, "alivemax");
        stagedata[ZStage_HumanAttributes] = ZRiotStages_CacheAttributes(kv, sectionname, "humans");
        KvGoBack(kv);  // Jump back to previous level.
    }
    else
    {
        LogMgr_Print(g_moduleZRiotStages, LogType_Error, "Config Validation", "Missing \"humans\" section for stage \"%s\".  This stage will be disregarded until fixed.", sectionname);
        return KvCache_Ignore;
    }
    
    // Store in global array.
    PushArrayArray(g_hZRiotStageData, stagedata[0]);
    
    return KvCache_Continue;
}

/**
 * Re-cache all stages data from disk.
 * 
 * @return      True if cached successfully, false if the file couldn't be found or no usable data was inside.
 */
bool:ZRiotStages_CacheData()
{
    // Get the config path from the gamerules module.
    decl String:configfile[PLATFORM_MAX_PATH];
    GameRules_GetConfigPath(STAGES_CONFIG_FILE, configfile);
    
    if (ConfigMgr_ValidateFile(configfile))
        ConfigMgr_WriteString(g_moduleZRiotStages, CM_CONFIGINDEX_FIRST, ConfigData_Path, CM_DATA_PATH, configfile);
    else
    {
        LogMgr_Print(g_moduleZRiotStages, LogType_Fatal_Module, "Config Validation", "Invalid config file path defined in gamerules: \"%s\".  Disabling module.", configfile);
        return false;
    }
    
    ZRiotStages_CleanCache();
    ClearArray(g_hZRiotStageData);
    g_iZRiotStageCount = ConfigMgr_CacheKv(g_moduleZRiotStages, CM_CONFIGINDEX_FIRST, "ZRiotStages_Cache");
    
    // There were no stages defined.
    if (g_iZRiotStageCount == 0)
    {
        LogMgr_Print(g_moduleZRiotStages, LogType_Fatal_Module, "Config Validation", "No usable data found in stages config file: %s", configfile);
        return false;
    }
    
    return true;
}

/**
 * Get a cell from stage data.  (bools, ints, handles)
 * 
 * @param stageindex    Index the stage data is at.
 * @param data          The data you want to return.  See enum ZRiotStage.
 * 
 * @return              A cell of data.
 */
stock Handle:ZRiotStages_GetDataMessages(stageindex) { return Handle:ZRiotStages_GetDataCell(stageindex, ZStage_Messages); }
stock ZRiotStages_GetDataCount(stageindex) { return ZRiotStages_GetDataCell(stageindex, ZStage_Count); }
stock bool:ZRiotStages_GetDataRespawn(stageindex) { return bool:ZRiotStages_GetDataCell(stageindex, ZStage_Respawn); }
stock ZRiotStages_GetDataZPlayerMax(stageindex) { return ZRiotStages_GetDataCell(stageindex, ZStage_ZombiePlayerMax); }
stock ZRiotStages_GetDataZAliveMax(stageindex) { return ZRiotStages_GetDataCell(stageindex, ZStage_ZombieAliveMax); }
stock Handle:ZRiotStages_GetDataZAttributes(stageindex) { return Handle:ZRiotStages_GetDataCell(stageindex, ZStage_ZombieAttributes); }
stock ZRiotStages_GetDataHPlayerMax(stageindex) { return ZRiotStages_GetDataCell(stageindex, ZStage_HumanPlayerMax); }
stock ZRiotStages_GetDataHAliveMax(stageindex) { return ZRiotStages_GetDataCell(stageindex, ZStage_HumanAliveMax); }
stock Handle:ZRiotStages_GetDataHAttributes(stageindex) { return Handle:ZRiotStages_GetDataCell(stageindex, ZStage_HumanAttributes); }
stock ZRiotStages_GetDataCell(stageindex, ZRiotStage:data)
{
    new stagedata[ZRiotStage];
    GetArrayArray(g_hZRiotStageData, stageindex, stagedata[0], sizeof(stagedata));
    
    return _:stagedata[data];
}

/**
 * Get a string from stage data.
 * 
 * @param stageindex    Index the stage data is at.
 * @param data          The data you want to return.  Must be String type.  See enum ZRiotStage.
 * @param output        The string to set the output string in.
 * @param maxlen        Max length of the output.
 */
stock ZRiotStages_GetDataString(stageindex, ZRiotStage:data, String:output[], maxlen)
{
    new stagedata[ZRiotStage];
    GetArrayArray(g_hZRiotStageData, stageindex, stagedata[0], sizeof(stagedata));
    
    strcopy(output, maxlen, String:stagedata[data]);
}

// ---------------------------------
//              Events
// ---------------------------------

/**
 * Round has started.
 */
public ZRiotStages_RoundStart()
{
    // Print round start message(s).
    ZRiotStages_PrintStageMessage(STAGE_MSGS_GLOBAL_START);
    ZRiotStages_PrintStageMessage(STAGE_MSGS_ZOMBIE_START);
    ZRiotStages_PrintStageMessage(STAGE_MSGS_HUMAN_START);
    
    // Get cfg file path and execute if it's not empty.
    decl String:cfg[STAGE_DATA_CFG];
    ZRiotStages_GetDataString(g_iCurStageIndex, ZStage_Cfg, cfg, sizeof(cfg));
    if (cfg[0])
        ServerCommand("exec %s", cfg);
}

/**
 * Called when round has ended.
 *
 * @param winner        Winner team.
 */
public ZRiotStages_RoundEnd(VTeam:winner)
{
    if (winner == VTeam_Zombie) // Zombies win
    {
        // Print round win message(s).
        ZRiotStages_PrintStageMessage(STAGE_MSGS_GLOBAL_ZWIN);
        ZRiotStages_PrintStageMessage(STAGE_MSGS_ZOMBIE_ZWIN);
        ZRiotStages_PrintStageMessage(STAGE_MSGS_HUMAN_ZWIN);
        
        // Check if regression is enabled.
        if (GetConVarBool(g_hCvarZRiotStagesRegress))
            if (g_iCurStageIndex > 0) g_iCurStageIndex--;
    }
    else if (winner == VTeam_Human) // Humans win
    {
        // Print round win message(s).
        ZRiotStages_PrintStageMessage(STAGE_MSGS_GLOBAL_HWIN);
        ZRiotStages_PrintStageMessage(STAGE_MSGS_ZOMBIE_HWIN);
        ZRiotStages_PrintStageMessage(STAGE_MSGS_HUMAN_HWIN);
        
        // Check if they beat the final stage.
        if (g_iCurStageIndex < g_iZRiotStageCount - 1)
            g_iCurStageIndex++;
        else // Humans beat the final stage.
        {
            // TODO
        }
    }
}

// ---------------------------------
//            Utilities
// ---------------------------------

/**
 * Prints a message to the proper clients depending on message index.
 * 
 * @param index     ZStage_Messages array index.
 */
static stock ZRiotStages_PrintStageMessage(index)
{
    // Get the message array handle.
    new Handle:hMessages = ZRiotStages_GetDataMessages(g_iCurStageIndex);
    decl String:message[STAGE_DATA_MSGS];
    GetArrayString(hMessages, index, message, sizeof(message));
    if (message[0])
    {
        // Print to the right audience.
        if (index >=0 && index <= 2)
        {
            TransMgr_PrintTextAll(true, false, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, true, message);
        }
        else if (index >=3 && index <= 5)
        {
            for(new client = 1; client <= MaxClients; client++)
            {
                if (IsClientInGame(client) && TeamMgr_GetClientTeam(client) == VTeam_Zombie)
                    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, true, message);
            }
        }
        else if (index >=6 && index <= 8)
        {
            for(new client = 1; client <= MaxClients; client++)
            {
                if (IsClientInGame(client) && TeamMgr_GetClientTeam(client) == VTeam_Human)
                    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, true, message);
            }
        }
    }
}
