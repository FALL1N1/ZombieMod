/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          logmanager.inc
 *  Type:          Base
 *  Description:   Manages project logging.
 *
 *  Copyright (C) 2009-2011  Greyscale & Richard Helgeby
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
 * Provides the plugin a way to know if the log manager is included in the project.
 */
#define LOG_MANAGER

// ---------------
//     Public
// ---------------

/**
 * Log format types.
 */
enum LogTypes
{
    LogType_Normal,         // Normal log message.  Printed in SourceMod logs.
    LogType_Error,          // Error message.  Printed in SourceMod error logs.
    LogType_Fatal_Module,   // Error message.  Disables the module that prints it.
    LogType_Fatal_Plugin,   // Error message.  Kills the entire plugin.
    LogType_Debug           // Debug message.  Normal log message, but only printed if debugging is enabled.
}

// ---------------
//     Private
// ---------------

/**
 * The max number of cells needed for the log manager's allocated index.
 */
#define LM_DATA_CELL_COUNT 1
// Log manager only needs 1 cell for a boolean value.

/**
 * Defines the block of data in the module data arrays that contains module whitelist data.
 */
#define LOG_DATA_WHITELIST g_iLMAllocatedIndexes[0]

/**
 * Log message max lengths.
 */
#define LOG_MAX_LENGTH_FILE 2048
#define LOG_MAX_LENGTH_CHAT 192

/**
 * Array to store the index of the allocated space in the module data arrays for the log manager.
 */
new g_iLMAllocatedIndexes[1];

/**
 * Log cvars.
 */
new Handle:g_hCvarLog;
new Handle:g_hCvarLogDebug;
new Handle:g_hCvarLogWhitelist;
new Handle:g_hCvarLogPrintAdmins;
new Handle:g_hCvarLogPrintPublic;

// **********************************************
//                 Forwards
// **********************************************

/**
 * Plugin is loading.
 */
LogMgr_OnPluginStart()
{
    // Allocate 1 index for the data we want to store for each module.
    ModuleMgr_Allocate(1, g_iLMAllocatedIndexes);
    
    // Create log cvars.
    g_hCvarLog =               Project_CreateConVar("log", "1", "Enable logging in the plugin.  Error messages will always be logged.");
    g_hCvarLogDebug =          Project_CreateConVar("log_debug", "0", "Show debug logging.  This will just create more detailed logs.");
    g_hCvarLogWhitelist =      Project_CreateConVar("log_whitelist", "0", "Only modules added to the whitelist (project_log_whitelist_add) will be logged.");
    g_hCvarLogPrintPublic =    Project_CreateConVar("log_print_public", "0", "Logs will be printed to public chat in addition to log files.");
    
    // This cvar needs the translations manager to filter out the non-admins and the access manager to determine what "admin" is.
    #if defined TRANSLATIONS_MANAGER && defined ACCESS_MANAGER
        g_hCvarLogPrintAdmins = Project_CreateConVar("log_print_admins", "0", "Logs will be printed to admin chat in addition to log files.");
    #endif
    
    #if defined PROJECT_BASE_CMD
        // Register base cmds.
        LogMgr_RegisterCmds();
    #else
        // Create log commands.
        Project_RegServerCmd("log_whitelist_add", LogMgr_WhitelistAddCommand, "Adds one or more modules to the whitelist.  Usage: <prefix>_log_whitelist_add <moduleshortname [module2] ...");
        Project_RegServerCmd("log_whitelist_rem", LogMgr_WhitelistRemCommand, "Removes one or more modules from the whitelist.  Usage: <prefix>_log_whitelist_remove <moduleshortname [module2] ...");
    #endif
}

/**
 * Plugin is ending.
 */
LogMgr_OnPluginEnd()
{
}

/**
 * A module was just registered.  This is being called before the module has been assigned a module identifier.
 * 
 * @param adtModule The adt array of the module being registered.
 */
stock LogMgr_OnModuleRegister(Handle:adtModule)
{
    // Push the 'false' into our allocated space to signify if this module is on the whitelist or not.
    PushArrayCell(adtModule, false);
}

/**
 * Base command is printing a module's info.
 * Print the module data allocated by the log manager.
 * Note: |stock| tag will stop this function from being compiled if the base command is disabled.
 * 
 * @param client    The client index the text is being printed to.
 * @param module    The module to print info for.
 */
stock LogMgr_OnPrintModuleInfo(client, Module:module)
{
    // Translate a bool into a text phrase.
    decl String:strWhiteList[8];
    new bool:bWhiteList = LogMgr_IsModuleOnWhitelist(module);
    
    #if defined TRANSLATIONS_MANAGER
        TransMgr_BoolToPhrase(client, bWhiteList, BoolPhrase_YesNo, strWhiteList, sizeof(strWhiteList));
    #else
        IntToString(_:bWhiteList, strWhiteList, sizeof(strWhiteList));
    #endif
    
    // Print the module event info.
    PrintToServer("%T", "LogMgr modules info", LANG_SERVER, strWhiteList, PROJECT_CVAR_PREFIX);
}

// **********************************************
//                Public API
// **********************************************

/**
 * Print a formatted message to logs depending on log settings.
 * 
 * @param module        The module sending the log.
 * @param logtype       The type of the log being processed.
 * @param description   Short description of the log, like a function name.
 * @param text          The log message.
 * @param ...           Formatting parameters.
 */
stock LogMgr_Print(Module:module, LogTypes:logtype, const String:description[], const String:text[], any:...)
{
    // If the module is disabled, then don't print the log.
    if (ModuleMgr_IsDisabled(module))
        return;
    
    // If the log manager is disabled, and this is a normal log, then don't print the log.
    if (!GetConVarBool(g_hCvarLog) && logtype == LogType_Normal)
        return;
    
    // If debugging is disabled and this is a debug log, then don't print it.
    if (!GetConVarBool(g_hCvarLogDebug) && logtype == LogType_Debug)
        return;
    
    // Check if whitelist is enabled.
    if (GetConVarBool(g_hCvarLogWhitelist))
    {
        // Check if the module is on the whitelist.
        if (!LogMgr_IsModuleOnWhitelist(module))
            return;
    }
    
    // Format extra parameters into the log buffer.
    decl String:logbuffer[LOG_MAX_LENGTH_FILE];
    VFormat(logbuffer, sizeof(logbuffer), text, 5);
    
    // Get the module's full name.
    decl String:modulefullname[MM_DATA_FULLNAME];
    ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
    
    // Format the text string.
    Format(logbuffer, sizeof(logbuffer), LM_FORMAT);
    
    // Format other parameters onto the log text.
    switch (logtype)
    {
        // Normal log message.
        case LogType_Normal, LogType_Debug:
        {
            LogMessage(logbuffer);
        }
        // Log an error message.
        case LogType_Error:
        {
            LogError(logbuffer);
        }
        // Log an error message and disable the module.
        case LogType_Fatal_Module:
        {
            LogError(logbuffer);
            ModuleMgr_Disable(module);
        }
        // Log an error message and kill the plugin.
        case LogType_Fatal_Plugin:
        {
            SetFailState(logbuffer);
        }
    }
    
    #if defined TRANSLATIONS_MANAGER && defined ACCESS_MANAGER
        // Print log to all in-game admins if the cvar is enabled.
        if (GetConVarBool(g_hCvarLogPrintAdmins))
        {
            // Print text to admins.
            TransMgr_PrintTextAll(false, true, MsgFormat_None, MsgType_Chat, INVALID_MODULE, true, logbuffer);
        }
    #endif
    
    // Print log to all in-game clients if the cvar is enabled.
    if (GetConVarBool(g_hCvarLogPrintPublic))
    {
        PrintToChatAll(logbuffer);
    }
}

// **********************************************
//   Private API (For base project files only)
// **********************************************

/**
 * Check if the specified module is on the whitelist
 * 
 * @param module        The module to check.
 * 
 * @return              True if enabled, false otherwise. 
 */
stock bool:LogMgr_IsModuleOnWhitelist(Module:module)
{
    return bool:GetArrayCell(ModuleMgr_GetModuleArray(module), LOG_DATA_WHITELIST);
}

/**
 * Change if a module is enabled on the whitelist.
 * 
 * @param module        The module to change.
 * @param enable        True to add the module to the whitelist, false to remove.
 */
stock LogMgr_WhitelistSet(Module:module, bool:enable)
{
    SetArrayCell(ModuleMgr_GetModuleArray(module), LOG_DATA_WHITELIST, _:enable);
}

/**
 * Adds a module to the whitelist.
 *
 * @param module    The module being add to whitelist.
 * 
 * @return          True if added, false if it's already in the whitelist.
 */
stock bool:LogMgr_WhitelistAdd(Module:module)
{
    // Check if the module isn't already is listed.
    if (!LogMgr_IsModuleOnWhitelist(module))
    {
        // Add module to the whitelist.
        LogMgr_WhitelistSet(module, true);
        return true;
    }
    
    return false;
}

/**
 * Removes a module from the whitelist.
 *
 * @param module    The module being removed from the whitelist.
 * 
 * @return          True if removed, false if it's not on the whitelist.
 */
stock bool:LogMgr_WhitelistRem(Module:module)
{
    // Check if the module isn't already is listed.
    if (LogMgr_IsModuleOnWhitelist(module))
    {
        // Remove module from the whitelist.
        LogMgr_WhitelistSet(module, false);
        return true;
    }
    
    return false;
}

// If there is a base cmd then make sub-commands in it.
#if defined PROJECT_BASE_CMD

/**
 * Registers all default sub-commands.
 */
stock LogMgr_RegisterCmds()
{
    // Create directions.
    new String:g_dirBlank[1][16] = {""};
    new String:dirLog[1][16] = {"log"};
    
    // Server commands.
    BaseCmd_Register(CommandFor_Server, g_dirBlank, 0, "log");
    BaseCmd_Register(CommandFor_Server, dirLog, sizeof(dirLog), "whitelist_add", "LogMgr_BaseCmdWLAdd");
    BaseCmd_Register(CommandFor_Server, dirLog, sizeof(dirLog), "whitelist_rem", "LogMgr_BaseCmdWLRem");
}

/**
 * Base sub-command: "log whitelist_add"
 * Add one or modules to the whitelist.
 * 
 * @param client    The client using the command.
 * @param hParams   An ADT array with all the leftover arguments in the command-string.  Don't close this!
 * @param argc      The number of leftover arguments. (GetArraySize(hParams))
 */
public LogMgr_BaseCmdWLAdd(client, Handle:hParams, argc)
{
    // Check if no arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "LogMgr basecmd log whitelist_add usage", LANG_SERVER, PROJECT_CMD_PREFIX, PROJECT_CMD_PREFIX);
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
        
        decl String:modulefullname[MM_DATA_FULLNAME];
        ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
        
        if (LogMgr_WhitelistAdd(module))
            Project_PrintToServer("%T", "LogMgr cmd whitelist_add", LANG_SERVER, modulefullname);
        else
            Project_PrintToServer("%T", "LogMgr cmd whitelist_add fail", LANG_SERVER, modulefullname);
    }
    
    // Say that we handled the command so the game doesn't see it and print "Unknown command"
    return;
}

/**
 * Base sub-command: "log whitelist_rem"
 * Remove one or modules from the whitelist.
 * 
 * @param client    The client using the command.
 * @param hParams   An ADT array with all the leftover arguments in the command-string.  Don't close this!
 * @param argc      The number of leftover arguments. (GetArraySize(hParams))
 */
public LogMgr_BaseCmdWLRem(client, Handle:hParams, argc)
{
    // Check if no arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "LogMgr basecmd log whitelist_rem usage", LANG_SERVER, PROJECT_CMD_PREFIX, PROJECT_CMD_PREFIX);
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
        
        decl String:modulefullname[MM_DATA_FULLNAME];
        ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
        
        if (LogMgr_WhitelistRem(module))
            Project_PrintToServer("%T", "LogMgr cmd whitelist_rem", LANG_SERVER, modulefullname);
        else
            Project_PrintToServer("%T", "LogMgr cmd whitelist_rem fail", LANG_SERVER, modulefullname);
    }
    
    // Say that we handled the command so the game doesn't see it and print "Unknown command"
    return;
}

// If not then use regular console commands.
#else

/**
 * Command callback: <prefix>_log_whitelist_add
 * Add one or modules to the whitelist.
 * 
 * @param argc      The number of arguments that the server sent with the command.
 */
public Action:LogMgr_WhitelistAddCommand(argc)
{
    // Check if no arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "LogMgr cmd whitelist_add usage", LANG_SERVER, PROJECT_CMD_PREFIX, PROJECT_CMD_PREFIX);
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
        
        decl String:modulefullname[MM_DATA_FULLNAME];
        ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
        
        if (LogMgr_WhitelistAdd(module))
            Project_PrintToServer("%T", "LogMgr cmd whitelist_add", LANG_SERVER, modulefullname);
        else
            Project_PrintToServer("%T", "LogMgr cmd whitelist_add fail", LANG_SERVER, modulefullname);
    }
    
    // Say that we handled the command so the game doesn't see it and print "Unknown command"
    return Plugin_Handled;
}

/**
 * Command callback: <prefix>_log_whitelist_remove
 * Remove one or modules from the whitelist.
 * 
 * @param argc      The number of arguments that the server sent with the command.
 */
public Action:LogMgr_WhitelistRemCommand(argc)
{
    // Check if no arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "LogMgr cmd whitelist_rem usage", LANG_SERVER, PROJECT_CMD_PREFIX, PROJECT_CMD_PREFIX);
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
        
        decl String:modulefullname[MM_DATA_FULLNAME];
        ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
        
        if (LogMgr_WhitelistRem(module))
            Project_PrintToServer("%T", "LogMgr cmd whitelist_rem", LANG_SERVER, modulefullname);
        else
            Project_PrintToServer("%T", "LogMgr cmd whitelist_rem fail", LANG_SERVER, modulefullname);
    }
    
    // Say that we handled the command so the game doesn't see it and print "Unknown command"
    return Plugin_Handled;
}

#endif

/**
 * These are stock functions to return the value of any of the log manager's cvars.
 * This allows other base components/modules to read log cvars.
 */

stock bool:LogMgr_CvarLog()
{
    return GetConVarBool(g_hCvarLog);
}

stock bool:LogMgr_CvarDebug()
{
    return GetConVarBool(g_hCvarLogDebug);
}

stock bool:LogMgr_CvarWhitelist()
{
    return GetConVarBool(g_hCvarLogWhitelist);
}

stock bool:LogMgr_CvarPrintAdmins()
{
    return GetConVarBool(g_hCvarLogPrintAdmins);
}

stock bool:LogMgr_CvarPrintPublic()
{
    return GetConVarBool(g_hCvarLogPrintPublic);
}
