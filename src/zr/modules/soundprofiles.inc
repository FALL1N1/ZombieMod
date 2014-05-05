/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          soundprofiles.inc
 *  Type:          Module
 *  Description:   Manages sound profiles.
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
 * TODO
 * this needs priority in OnMapStart over any module using it in their configs.
 */

// Include sound profile interface.
#include "zr/interfaces/soundprofiles.interface"

/**
 * The path to this module's sound profiles config file, relative to the path defined in gamerules.txt.
 */
#define SNDPROFS_CONFIG_FILE "soundprofiles.txt"

/**
 * This module's identifier.
 */
new Module:g_moduleSndProfs;

/**
 * Handle to sound profile cache.
 */
new Handle:g_hSndProfsCache;

/**
 * Register this module.
 */
SndProfs_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Sound Profiles");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "soundprofiles");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Manages sound profiles");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleSndProfs = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleSndProfs, "Event_OnEventsRegister",          "SndProfs_OnEventsRegister");
    
    // Register config file(s) that this module will use.
    ConfigMgr_Register(g_moduleSndProfs, "SndProfs_OnConfigReload", "");
}

/**
 * Register all events here.
 */
public SndProfs_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleSndProfs, "Event_OnPluginEnd",               "SndProfs_OnPluginEnd");
    EventMgr_RegisterEvent(g_moduleSndProfs, "Event_OnMyModuleEnable",          "SndProfs_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleSndProfs, "Event_OnMyModuleDisable",         "SndProfs_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleSndProfs, "Event_OnMapStart",                "SndProfs_OnMapStart");
}

/**
 * Set up support for functions from the sound profile interface.
 */
static stock SndProfs_ImplementFuncs()
{
    // Implement the infection interface.
    Interface_Implement(g_ISndProfsGetNumProfiles, GetFunctionByName(GetMyHandle(), "SndProfs_GetNumProfiles"));
    Interface_Implement(g_ISndProfsGetProfileSound, GetFunctionByName(GetMyHandle(), "SndProfs_GetProfileSound"));
    Interface_Implement(g_ISndProfsGetRandProfileSound, GetFunctionByName(GetMyHandle(), "SndProfs_GetRandProfileSound"));
}

/**
 * Set up support for functions from the sound profile interface.
 */
static stock SndProfs_ReleaseFuncs()
{
    // Implement the infection interface.
    Interface_Release(g_ISndProfsGetNumProfiles);
    Interface_Release(g_ISndProfsGetProfileSound);
    Interface_Release(g_ISndProfsGetRandProfileSound);
}

/**
 * Plugin is loading.
 */
SndProfs_OnPluginStart()
{
    // Register the module.
    SndProfs_Register();
    
    // Implement interface functions.
    SndProfs_ImplementFuncs();
    
    // Create array.
    g_hSndProfsCache = CreateArray();
}

/**
 * Plugin is ending.
 */
public SndProfs_OnPluginEnd()
{
    // Cleanup data stored by the sound effects module.
    SndProfs_CleanCache();
    CloseHandle(g_hSndProfsCache);
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:SndProfs_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // Implement interface functions.
    SndProfs_ImplementFuncs();
    
    // Create array.
    g_hSndProfsCache = CreateArray();
    
    // Don't let the module load unless the sound profiles are cached successfully.
    if (!SndProfs_CacheProfiles())
    {
        Format(refusalmsg, maxlen, "%T", "Sound profiles refuse enable", LANG_SERVER);
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
public Action:SndProfs_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    // These functions are not longer supported by this module.
    SndProfs_ReleaseFuncs();
    
    // Cleanup data stored by the sound effects module.
    SndProfs_CleanCache();
    CloseHandle(g_hSndProfsCache);
}

/**
 * The map has started.
 */
public SndProfs_OnMapStart()
{
    SndProfs_CacheProfiles();
}

/**
 * Called when a registered config file (by this module) is manually reloaded.
 */
public SndProfs_OnConfigReload(configindex)
{
    SndProfs_CacheProfiles();
}

/**
 * Destroys all profile data from the cache.
 */
static stock SndProfs_CleanCache()
{
    // Cleanup data stored by the sound effects module.
    new count = GetArraySize(g_hSndProfsCache);
    for (new i = 0; i < count; i++)
        CloseHandle(GetArrayCell(g_hSndProfsCache, i));
}

/**
 * Loops through each section of the keyvalues tree.
 * Note: ADT array in 'g_moduleSndProfs' should be cleared before using this.
 * 
 * @param kv            The keyvalues handle of the config file. (Don't close this)
 * @param sectionindex  The index of the current keyvalue section, starting from 0.
 * @param sectionname   The name of the current keyvalue section.
 * 
 * @return              See enum KvCache.
 */
public KvCache:SndProfs_Cache(Handle:kv, sectionindex, const String:sectionname[])
{
    if (!KvGotoFirstSubKey(kv, false))
    {
        LogMgr_Print(g_moduleSndProfs, LogType_Error, "Config Validation", "Empty or invalid sound profile: \"%s\"", sectionname);
        return KvCache_Ignore;
    }
    
    new Handle:hSoundProfile = CreateArray(PLATFORM_MAX_PATH);
    PushArrayString(hSoundProfile, sectionname);
    
    decl String:path[PLATFORM_MAX_PATH];
    decl String:fullpath[PLATFORM_MAX_PATH];
    do
    {
        // Get sound file reference.
        KvGetString(kv, "path", path, sizeof(path));
        
        Format(fullpath, sizeof(fullpath), "sound/%s", path);
        if (FileExists(fullpath, true))
        {
            PushArrayString(hSoundProfile, path);
            AddFileToDownloadsTable(fullpath);
        }
        else
        {
            LogMgr_Print(g_moduleSndProfs, LogType_Error, "Config Validation", "Invalid sound file specified in sound profile \"%s\": \"%s\"", sectionname, path);
            continue;
        }
    } while (KvGotoNextKey(kv, false));
    KvGoBack(kv);
    
    PushArrayCell(g_hSndProfsCache, hSoundProfile);
    return KvCache_Continue;
}

/**
 * Re-cache all sound profiles from disk.
 * 
 * @return      True if cached successfully, false if the file couldn't be found or no usable data was inside.
 */
bool:SndProfs_CacheProfiles()
{
    // Get the config path from the gamerules module.
    decl String:configfile[PLATFORM_MAX_PATH];
    GameRules_GetConfigPath(SNDPROFS_CONFIG_FILE, configfile);
    
    if (ConfigMgr_ValidateFile(configfile))
        ConfigMgr_WriteString(g_moduleSndProfs, CM_CONFIGINDEX_FIRST, ConfigData_Path, CM_DATA_PATH, configfile);
    else
    {
        LogMgr_Print(g_moduleSndProfs, LogType_Fatal_Module, "Config Validation", "Invalid config file path defined in gamerules: \"%s\".  Disabling module.", configfile);
        return false;
    }
    
    SndProfs_CleanCache();
    ClearArray(g_hSndProfsCache);
    ConfigMgr_CacheKv(g_moduleSndProfs, CM_CONFIGINDEX_FIRST, "SndProfs_Cache");
    
    return true;
}

/**
 * Get how many sound profiles exist.
 *
 * @return  Number of sound profiles, 0 if they haven't been loaded yet.
 */
public SndProfs_GetNumProfiles()
{
    if (g_hSndProfsCache != INVALID_HANDLE)
        return GetArraySize(g_hSndProfsCache);
    return 0;
}

/**
 * Find a profile index based off its name.
 * 
 * @param profile   The name of the profile
 * 
 * @return          The index this profile is at in the array.  -1 if doesn't exist.
 */
static stock SndProfs_GetProfileIndex(const String:profile[])
{
    decl String:profilename[64];
    new count = GetArraySize(g_hSndProfsCache);
    for (new i = 0; i < count; i++)
    {
        GetArrayString(GetArrayCell(g_hSndProfsCache, i), 0, profilename, sizeof(profilename)); // Get the name of this profile at index 0.
        if (StrEqual(profile, profilename, false))
            return i;
    }
    return -1;
}

/**
 * Get a sound from a given profile and index.
 * 
 * @param profile   Sound profile get sound from.
 * @param index     The index of the sound.
 * @param sound     Output buffer for the sound path.
 * @param maxlen    Max length of the buffer.
 * 
 * @return          True if found, false otherwise.
 */
public bool:SndProfs_GetProfileSound(const String:profile[], index, String:sound[], maxlen)
{
    new pindex = SndProfs_GetProfileIndex(profile);
    if (pindex == -1)
        return false;
    
    new Handle:hSoundProfile = GetArrayCell(g_hSndProfsCache, pindex);
    if (index < 0 || index >= GetArraySize(hSoundProfile)-1)
        return false;
    
    GetArrayString(hSoundProfile, index, sound, maxlen);
    return true;
}

/**
 * Get a random sound from a profile.
 * 
 * @param profile   Profile to get random sound from.
 * @param sound     Output buffer for the sound path.
 * @param maxlen    Max length of the buffer.
 * 
 * @return          True if succeeded, false otherwrise.
 */
public bool:SndProfs_GetRandProfileSound(const String:profile[], String:sound[], maxlen)
{
    new pindex = SndProfs_GetProfileIndex(profile);
    if (pindex == -1)
        return false;
    
    new Handle:hSoundProfile = GetArrayCell(g_hSndProfsCache, pindex);
    new randsindex = Math_GetRandomInt(1, GetArraySize(hSoundProfile)-1);
    GetArrayString(hSoundProfile, randsindex, sound, maxlen);
    return true;
}
