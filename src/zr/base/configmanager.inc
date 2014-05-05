/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          config.inc
 *  Type:          Base
 *  Description:   Handles all project configs.
 *
 *  Copyright (C) 2009-2011  Greyscale
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
 * Provides the plugin a way to know if the config manager is included in the project.
 */
#define CONFIG_MANAGER

// ---------------
//     Public
// ---------------

/**
 * The max string lengths of config data variables.
 */
#define CM_DATA_PATH 128

/**
 * All data stored for each config file.
 */
enum ConfigData
{
    Function:ConfigData_ReloadFunc,         /** This function is called when the config should be reloaded. */
    String:ConfigData_Path[CM_DATA_PATH]    /** The path relative to the sourcemod/ folder to the config file. */
}

/**
 * The max number of config files a single module can maintain.
 */
#define CM_MODULE_MAX_CONFIGS 3

/**
 * Default defines for each config file index.
 */
#define CM_CONFIGINDEX_FIRST    0
#define CM_CONFIGINDEX_SECOND   1
#define CM_CONFIGINDEX_THIRD    2

/**
 * Config registration errors.
 */
enum ConfigRegistrationResults
{
    RegResult_Success = 0,          /** Config file registered successfully. */
    RegResult_LimitExceeded = -1,   /** The limit on configs/module has been exceeded. */
    RegResult_Invalid = -2,         /** Non-existant config file. */
    RegResult_Duplicate = -3        /** This config file is already registered. */
}

/**
 * Tell the KV cacher what to do.
 */
enum KvCache
{
    KvCache_Continue,   /** Let the cacheing continue. */
    KvCache_Ignore,     /** Pretend this section never existed.  Won't be counted in the return value either. */
    KvCache_Hault       /** Stop cacheing early.  The section currently being cached will be counted. */
}

// ---------------
//     Private
// ---------------

/**
 * Dummy array used as a way to count the cells required to store module event data.
 */
stock g_DummyConfigData[ConfigData];

/**
 * Defines the block of data in the module data arrays that contains config path data.
 */
#define CONFIG_DATA(%1) g_iCMAllocatedIndexes[%1]

/**
 * Array to store the indexes of the allocated space in the module data arrays for the config manager.
 */
new g_iCMAllocatedIndexes[CM_MODULE_MAX_CONFIGS];

// **********************************************
//                 Forwards
// **********************************************

/**
 * Plugin is loading.
 */
ConfigMgr_OnPluginStart()
{
    // Allocate 1 index for the data we want to store for each module.
    ModuleMgr_Allocate(CM_MODULE_MAX_CONFIGS, g_iCMAllocatedIndexes);
    
    #if defined PROJECT_BASE_CMD
        // Register base cmds.
        ConfigMgr_RegisterCmds();
    #else
        // Create config commands.
        Project_RegServerCmd("reload_config", ConfigMgr_ReloadCfgCommand, "Reloads a module's config files.  Usage: <prefix>_reload_config <moduleshortname> [module2] ...\nRelated command(s): <prefix>_reload_config_all");
        Project_RegServerCmd("reload_config_all", ConfigMgr_ReloadCfgAllCommand, "Reloads all modules' config files.\nRelated command(s): <prefix>_reload_config");
    #endif
}

/**
 * Plugin is ending.
 */
ConfigMgr_OnPluginEnd()
{
}

/**
 * A module was just registered.  This is being called before the module has been assigned a module identifier.
 * 
 * @param adtModule The adt array of the module being registered.
 */
stock ConfigMgr_OnModuleRegister(Handle:adtModule)
{
    // Push the config data arrays for a module.
    // This is being pushed into our allocated space for config path data.
    new moduleconfigs[ConfigData];
    moduleconfigs[ConfigData_ReloadFunc] = INVALID_FUNCTION; // Initialize to INVALID_FUNCTION.
    for (new pindex = 0; pindex < CM_MODULE_MAX_CONFIGS; pindex++)
        PushArrayArray(adtModule, moduleconfigs[0], sizeof(moduleconfigs));
}

/**
 * Base command is printing a module's info.
 * Print the module data allocated by the config manager.
 * Note: |stock| tag will stop this function from being compiled if the base command is disabled.
 * 
 * @param client    The client index the text is being printed to.
 * @param module    The module to print info for.
 */
stock ConfigMgr_OnPrintModuleInfo(client, Module:module)
{
    decl String:configpaths[CM_MODULE_MAX_CONFIGS][CM_DATA_PATH];
    ConfigMgr_ReadStrings(module, ConfigData_Path, configpaths, sizeof(configpaths[]));
    
    new String:configpathlist[CM_MODULE_MAX_CONFIGS * CM_DATA_PATH];
    for (new configindex = 0; configindex < CM_MODULE_MAX_CONFIGS; configindex++)
    {
        if (configpaths[configindex][0] == 0)
            continue;
        
        Format(configpathlist, sizeof(configpathlist), "%s\n    %s", configpathlist, configpaths[configindex]);
    }
    
    // Print all the data stored in the module manager itself.
    PrintToServer("%T", "ConfigMgr modules info", LANG_SERVER, configpathlist);
}

// **********************************************
//                Public API
// **********************************************

/**
 * Config reload function prototype:
 * 
 * @param configindex   The module's config index that's being reloaded.
 * 
 * public:ReloadFunc(configindex)
 */

/**
 * Register a config file.
 * 
 * @param module            The module identifier.
 * @param configreloadfunc  This name of the function that will be called when the config file is being reloaded.
 *                          Set this as a null string to disable the reload function.
 *                          Prototype is right above this comment block.
 * @param configpath        The path to the config file relative to sourcemod/.  "" to bypass validation.  See ConfigMgr_ValidateFile for later validation.
 * @param configindex       Index of the module's config file. See CM_CONFIGINDEX_* defines.
 * 
 * @return                  RegResult_Success on success.  See enum ConfigRegistrationResults for other results.
 */
stock ConfigRegistrationResults:ConfigMgr_Register(Module:module, const String:configreloadfunc[], const String:configpath[], &configindex = -1)
{
    decl String:modulefullname[MM_DATA_FULLNAME];
    ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
    
    // Check if the module has registered the max number of config files.
    configindex = ConfigMgr_FindNextIndex(module);
    if (configindex == -1)
    {
        LogError("[Config Manager] Module \"%s\" can't register more than %d config files!", modulefullname, CM_MODULE_MAX_CONFIGS);
        return RegResult_LimitExceeded;
    }
    
    // Bypass validation by using an empty path.
    if (configpath[0] != 0)
    {
        new bool:validated = ConfigMgr_ValidateFile(configpath);
        if (!validated)
            LogError("[Config Manager] Module \"%s\" trying to register a non-existant config file! (%s)", modulefullname, configpath);
        
        // Check if this path is already registered.
        if (ConfigMgr_PathToIndex(module, configpath) > -1)
        {
            LogError("[Config Manager] Module \"%s\" can't register a config file (%s) more than once!", modulefullname, configpath);
            return RegResult_Duplicate;
        }
    }
    
    // Write the config data to the module data arrays.
    ConfigMgr_WriteCell(module, configindex, ConfigData_ReloadFunc, GetFunctionByName(GetMyHandle(), configreloadfunc));
    ConfigMgr_WriteString(module, configindex, ConfigData_Path, CM_DATA_PATH, configpath);
    
    return RegResult_Success;
}

/**
 * Validates a config file's existence.
 * 
 * @param configpath    The path to the config file.
 */
stock bool:ConfigMgr_ValidateFile(const String:configpath[])
{
    // Check if the file is valid.
    decl String:fullconfigpath[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, fullconfigpath, sizeof(fullconfigpath), configpath);
    
    return FileExists(fullconfigpath);
}

/**
 * This function only works for the following KV format:
 * "root"
 * {  
 *     "sectionname"
 *     {
 *         "option1"   "anyvalue"
 *         "option2"   "anyvalue"
 *         ...
 *     }
 *     "sectionname2"
 *     {
 *         "option1"   "anyvalue"
 *         "option2"   "anyvalue"
 *         ...
 *     }
 * }
 * This function will simply loop through each section name and then
 * call the function given to give it a chance to cache each option within it.
 * 
 * Cache function prototype:
 * 
 * @param kv            The keyvalues handle of the config file. (Don't close this)
 * @param sectionindex  The index of the current keyvalue section, starting from 0.
 * @param sectionname   The name of the current keyvalue section.
 * 
 * @return              See enum KvCache.
 *  
 * public KvCache:funcCache(Handle:kv, sectionindex, const String:sectionname[])
 *  
 * Note: SM has plans to remove the need for 'public' on this private function.
 * 
 * @param module        The module identifier to get the config info from.
 * @param configindex   Index of the module's config file. See CM_CONFIGINDEX_* defines.
 * @param funcCache     The name of the function that will be doing the caching. (see prototype above)
 * 
 * @return              The number of sections in the keyvalue file.  
 */
stock ConfigMgr_CacheKv(Module:module, configindex, const String:funcCache[])
{
    decl String:configpath[CM_DATA_PATH];
    ConfigMgr_ReadString(module, configindex, ConfigData_Path, configpath, sizeof(configpath));
    
    decl String:fullconfigpath[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, fullconfigpath, sizeof(fullconfigpath), configpath);
    
    new Handle:kv = CreateKeyValues("");
    if (!FileToKeyValues(kv, fullconfigpath))
    {
        LogError("[Config Manager] Keyvalue config file \"%s\" couldn't be loaded.", fullconfigpath);
        return 0;
    }
    
    KvRewind(kv);
    if (!KvGotoFirstSubKey(kv))
        return 0;
    
    new count;
    decl String:sectionname[64];
    do
    {
        // Get the sectionname to push to the cache func.
        KvGetSectionName(kv, sectionname, sizeof(sectionname));
        
        // Forward to the cache function.
        Call_StartFunction(GetMyHandle(), GetFunctionByName(GetMyHandle(), funcCache));
        
        // Push parameters.
        Call_PushCell(kv);
        Call_PushCell(count);
        Call_PushString(sectionname);
        
        // Finish
        new KvCache:result;
        Call_Finish(result);
        
        count++;
        
        switch (result)
        {
            case KvCache_Ignore: { count--; continue; }
            case KvCache_Hault: break;
        }
    } while (KvGotoNextKey(kv));
    
    CloseHandle(kv);
    
    return count;
}

/**
 * Caches each line of a text file into an adt array.
 * 
 * @param module        The module identifier.
 * @param configindex   Index of the module's config file. See CM_CONFIGINDEX_* defines.
 * @param maxlen        The max length of each string.
 * @param count         This will be set to the number of lines cached.
 * 
 * @return              A handle to a newly created adt array.  INVALID_HANDLE on failure.
 *                      Don't forget to close this when you're done!
 */
stock Handle:ConfigMgr_CacheFile(Module:module, configindex, maxlen, &count = 0)
{
    decl String:configpath[CM_DATA_PATH];
    ConfigMgr_ReadString(module, configindex, ConfigData_Path, configpath, sizeof(configpath));
    
    decl String:fullconfigpath[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, fullconfigpath, sizeof(fullconfigpath), configpath);
    
    new Handle:hConfig = OpenFile(fullconfigpath, "r");
    if (hConfig == INVALID_HANDLE)
    {
        LogError("[Config Manager] Config file \"%s\" couldn't be loaded.", fullconfigpath);
        return INVALID_HANDLE;
    }
    
    // Create the array we are going to return.
    new Handle:hCache = CreateArray(maxlen);
    
    decl String:line[512];
    count = 0;
    while(ReadFileLine(hConfig, line, sizeof(line)))
    {
        // Cut out comments at the end of a line.
        if (StrContains(line, "//") > -1)
        {
            SplitString(line, "//", line, maxlen);
        }
        TrimString(line);
        
        if (!line[0])
            continue;
        
        // Push line into array.
        PushArrayString(hCache, line);
        count++;
    }
    
    // We're done this file, so we can destory it from memory. 
    CloseHandle(hConfig);
    
    // Return the handle to the cache.
    return hCache;
}

// **********************************************
//   Private API (For base project files only)
// **********************************************

/**
 * Reloads all of a module's config files.
 * 
 * @param module    The module identifier of the module whose configs to reload.
 */
stock ConfigMgr_ReloadConfigs(Module:module)
{
    new Function:reloadfuncs[CM_MODULE_MAX_CONFIGS];
    new String:configpaths[CM_MODULE_MAX_CONFIGS][CM_DATA_PATH];
    
    // Get the config data for this module into arrays.
    ConfigMgr_ReadCells(module, ConfigData_ReloadFunc, reloadfuncs);
    ConfigMgr_ReadStrings(module, ConfigData_Path, configpaths, CM_DATA_PATH);
    
    // Call the reload func stored in the module's config data for each of its config files.
    for (new configindex = 0; configindex < CM_MODULE_MAX_CONFIGS; configindex++)
    {
        if (reloadfuncs[configindex] == INVALID_FUNCTION)
            continue;
        
        Call_StartFunction(GetMyHandle(), reloadfuncs[configindex]);
        Call_PushCell(configindex);
        Call_Finish();
    }
}

/**
 * Finds the next available index to store a config path.
 * 
 * @param module    The module identifier.
 * 
 * @return          The next available index.  -1 if none are available.
 */
stock ConfigMgr_FindNextIndex(Module:module)
{
    new Function:reloadfuncs[CM_MODULE_MAX_CONFIGS];
    ConfigMgr_ReadCells(module, ConfigData_ReloadFunc, reloadfuncs);
    
    // Loop through each config path index.
    for (new cpindex = 0; cpindex < sizeof(reloadfuncs); cpindex++)
    {
        if (reloadfuncs[cpindex] == INVALID_FUNCTION)
            return cpindex;
    }
    
    return -1;
}

/**
 * Returns the config path index given the path.
 * 
 * @param module    The module to get the path index for.
 * @param path      The path to the config file. (relative to sourcemod)
 * 
 * @return      The config path index.  -1 if the path doesn't exist.
 */
stock ConfigMgr_PathToIndex(Module:module, const String:configpath[])
{
    new String:configpaths[CM_MODULE_MAX_CONFIGS][CM_DATA_PATH];
    ConfigMgr_ReadStrings(module, ConfigData_Path, configpaths, CM_DATA_PATH);
    
    // Loop through each config path index.
    for (new cpindex = 0; cpindex < sizeof(configpaths); cpindex++)
    {
        if (StrEqual(configpath, configpaths[cpindex], false))
            return cpindex;
    }
    
    return -1;
}

/**
 * Config data reader that returns all available config data.
 * 
 * @param module        The module whose config data to read.
 * @param configindex   Index of the module's config file. See CM_CONFIGINDEX_* defines.
 * @param configdata    Output array for all config data.  See enum ConfigData.
 */
stock ConfigMgr_ReadAll(Module:module, configindex, configdata[ConfigData])
{
    GetArrayArray(ModuleMgr_GetModuleArray(module), CONFIG_DATA(configindex), configdata[0], sizeof(configdata));
}

/**
 * Module config data reader for any data type except strings.
 * 
 * @param module        The module whose config cell data to read.
 * @param configindex   Index of the module's config file. See CM_CONFIGINDEX_* defines.
 * @param data          The data to get the value of.  See enum ConfigData.
 * 
 * @return          The value of the desired module config data.
 */
stock ConfigMgr_ReadCell(Module:module, configindex, ConfigData:data)
{
    new configdata[ConfigData];
    ConfigMgr_ReadAll(module, configindex, configdata);
    
    // Return the value.
    return _:configdata[data];
}

/**
 * Module config data reader for reading all non-string-typed values for every config index.
 * The size of the array is equal to CM_MODULE_MAX_CONFIGS.
 * 
 * @param module    The module whose config string data to read.
 * @param data      The data to get the values of.  See enum ConfigData.
 * @param output    Output array for the data read.
 */
stock ConfigMgr_ReadCells(Module:module, ConfigData:data, any:output[CM_MODULE_MAX_CONFIGS])
{
    // Loop through each config index.
    for (new configindex = 0; configindex < CM_MODULE_MAX_CONFIGS; configindex++)
        output[configindex] = ConfigMgr_ReadCell(module, configindex, data);
}

/**
 * Module config data reader for any string typed values.
 * 
 * @param module        The module whose config string data to read.
 * @param data          The data to get the value of.  See enum ConfigData.
 * @param configindex   Index of the module's config file. See CM_CONFIGINDEX_* defines.
 * @param output        Output variable for the data read.
 * @param maxlen        The max length of the output string.
 */
stock ConfigMgr_ReadString(Module:module, configindex, ConfigData:data, String:output[], maxlen)
{
    new configdata[ConfigData];
    ConfigMgr_ReadAll(module, configindex, configdata);
    
    // Copy full name to output
    strcopy(output, maxlen, String:configdata[data]);
}

/**
 * Module config data reader for reading all string-typed values for every config index.
 * The size of the array is equal to CM_MODULE_MAX_CONFIGS.
 * 
 * @param module    The module whose config string data to read.
 * @param data      The data to get the values of.  See enum ConfigData.
 * @param output    Output string array for the data read.
 * @param maxlen    The max length of the output array strings.
 */
stock ConfigMgr_ReadStrings(Module:module, ConfigData:data, String:output[CM_MODULE_MAX_CONFIGS][], maxlen)
{
    // Loop through each config index.
    for (new configindex = 0; configindex < sizeof(output); configindex++)
        ConfigMgr_ReadString(module, configindex, data, output[configindex], maxlen);
}

/**
 * Module config data writer that overwrites all config data for a module with the given data.
 * 
 * @param module        The module whose config data to write.
 * @param configindex   Index of the module's config file. See CM_CONFIGINDEX_* defines.
 * @param configdata    New config data to replace the old config data.  See enum ConfigData.
 */
stock ConfigMgr_WriteAll(Module:module, configindex, configdata[ConfigData])
{
    SetArrayArray(ModuleMgr_GetModuleArray(module), CONFIG_DATA(configindex), configdata[0], sizeof(configdata));
}

/**
 * Module config data writer that writes a specified non-string data value.
 * 
 * @param module        The module whose config cell data to write.
 * @param configindex   Index of the module's config file. See CM_CONFIGINDEX_* defines.
 * @param data          Data to write new value to.  See enum ConfigData.
 * @param value         Any cell value to write as the new data.
 */
stock ConfigMgr_WriteCell(Module:module, configindex, ConfigData:data, any:value)
{
    // Read all the module's config data.
    new configdata[ConfigData];
    ConfigMgr_ReadAll(module, configindex, configdata);
    
    // Change the value of the specified module config data.
    configdata[data] = value;
    
    // Overwrite the old array with the modified one.
    ConfigMgr_WriteAll(module, configindex, configdata);
}

/**
 * Module config data writer that writes a specified string data value.
 * 
 * @param module        The module whose config string data to write.
 * @param configindex   Index of the module's config file. See CM_CONFIGINDEX_* defines.
 * @param data          Data to write new string to.  See enum ConfigData.
 * @param maxlen        The max length of the data value.  See enum ConfigData.
 * @param value         A string to write as the new data value.
 */
stock ConfigMgr_WriteString(Module:module, configindex, ConfigData:data, maxlen, const String:value[])
{
    // Read all the module's config data.
    new configdata[ConfigData];
    ConfigMgr_ReadAll(module, configindex, configdata);
    
    // Change the value of the specified module config data.
    strcopy(String:configdata[data], maxlen, value);
    
    // Overwrite the old array with the modified one.
    ConfigMgr_WriteAll(module, configindex, configdata);
}

// If there is a base cmd then make sub-commands in it.
#if defined PROJECT_BASE_CMD

/**
 * Registers all default sub-commands.
 */
stock ConfigMgr_RegisterCmds()
{
    // Create directions.
    new String:g_dirBlank[1][16] = {""};
    new String:dirConfig[1][16] = {"config"};
    
    // Server commands.
    BaseCmd_Register(CommandFor_Server, g_dirBlank, 0, "config");
    BaseCmd_Register(CommandFor_Server, dirConfig, sizeof(dirConfig), "reload", "ConfigMgr_BaseCmdReload");
    BaseCmd_Register(CommandFor_Server, dirConfig, sizeof(dirConfig), "reload_all", "ConfigMgr_BaseCmdReloadAll");
}

/**
 * Base sub-command: "config reload <moduleID>"
 * Reloads a module's config files.
 * 
 * @param client    The client using the command.
 * @param hParams   An ADT array with all the leftover arguments in the command-string.  Don't close this!
 * @param argc      The number of leftover arguments. (GetArraySize(hParams))
 */
public ConfigMgr_BaseCmdReload(client, Handle:hParams, argc)
{
    // Check if no arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "ConfigMgr basecmd reload usage", LANG_SERVER, PROJECT_CMD_PREFIX, PROJECT_CMD_PREFIX);
        return;
    }
    
    decl String:strModuleID[MM_DATA_SHORTNAME];
    new Module:module;
    
    // Loop through each argument.
    for (new arg = 0; arg < argc; arg++)
    {
        // Get argument string.
        GetArrayString(hParams, arg, strModuleID, sizeof(strModuleID));
        module = ModuleMgr_FindByID(StringToInt(strModuleID), strModuleID);
        
        if (module == INVALID_MODULE)
        {
            Project_PrintToServer("%T", "ModuleMgr module invalid", LANG_SERVER, strModuleID);
            continue;
        }
        
        // Reload the module's configs.
        ConfigMgr_ReloadConfigs(module);
    }
}

/**
 * Base sub-command: "config reload_all"
 * Reloads all modules' config files.
 * 
 * @param client    The client using the command.
 */
public ConfigMgr_BaseCmdReloadAll(client)
{
    // Loop through all the modules.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // If the module is disabled, then stop this iteration.
        if (ModuleMgr_IsDisabled(Module:moduleindex))
            continue;
        
        // Reload the module's configs.
        ConfigMgr_ReloadConfigs(Module:moduleindex);
    }
}

// If not then use regular console commands.
#else

/**
 * Command callback: <prefix>_config_reload
 * Reloads a module's config files.
 * 
 * @param argc      The number of arguments that the server sent with the command.
 */
public Action:ConfigMgr_ReloadCfgCommand(argc)
{
    // Check if no arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "ConfigMgr cmd reloadconfig usage", LANG_SERVER, PROJECT_CMD_PREFIX, PROJECT_CMD_PREFIX);
        return Plugin_Handled;
    }
    
    decl String:strModuleID[MM_DATA_SHORTNAME];
    new Module:module;
    
    // Loop through each argument.
    for (new arg = 1; arg <= argc; arg++)
    {
        // Get argument string.
        GetCmdArg(arg, strModuleID, sizeof(strModuleID));
        module = ModuleMgr_FindByID(StringToInt(strModuleID), strModuleID);
        
        if (module == INVALID_MODULE)
        {
            Project_PrintToServer("%T", "ModuleMgr module invalid", LANG_SERVER, strModuleID);
            continue;
        }
        
        // Reload the module's configs.
        ConfigMgr_ReloadConfigs(module);
    }
    
    // Say that we handled the command so the game doesn't see it and print "Unknown command"
    return Plugin_Handled;
}

/**
 * Command callback: <prefix>_config_reload_all
 * Reload all modules' config files.
 * 
 * @param argc      The number of arguments that the server sent with the command.
 */
public Action:ConfigMgr_ReloadCfgAllCommand(argc)
{
    new Module:module;
    
    // Loop through all the modules.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // Read moduleindex as a Module type.
        module = Module:moduleindex;
        
        // If the module is disabled, then stop this iteration.
        if (ModuleMgr_IsDisabled(module))
            continue;
        
        // Reload the module's configs.
        ConfigMgr_ReloadConfigs(module);
    }
}

#endif
