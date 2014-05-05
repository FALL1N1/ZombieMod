/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          downloader.inc
 *  Type:          Module
 *  Description:   Adds listed files and directories in a file to the server's download list.
 *  Note:          Once a file is added to the table it can not be removed.
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
 * The path to this module's sound profiles config file, relative to the path defined in gamerules.txt.
 */
#define DLR_CONFIG_FILE "downloads.txt"

/**
 * This module's identifier.
 */
new Module:g_moduleDLR;

/**
 * Register this module.
 */
DLR_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Downloader");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "downloader");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Adds listed files and directories in a file to the server's download list.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleDLR = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleDLR, "Event_OnEventsRegister",               "DLR_OnEventsRegister");
    
    // Register config file(s) that this module will use.
    //ConfigMgr_Register(g_moduleDLR, "DLR_OnConfigReload", "");
}

/**
 * Register all events here.
 */
public DLR_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleDLR, "Event_OnMyModuleEnable",               "DLR_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleDLR, "Event_OnMapStart",                     "DLR_OnMapStart");
}

/**
 * Plugin is loading.
 */
DLR_OnPluginStart()
{
    // Register the module.
    DLR_Register();
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:DLR_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    DLR_Cache();
}

/**
 * The map has started.
 */
public DLR_OnMapStart()
{
    DLR_Cache();
}

/**
 * Called when a registered config file (by this module) is manually reloaded.
 */
public DLR_OnConfigReload(configindex)
{
    DLR_Cache();
}

/**
 * Cache data from file.
 * 
 * @return      True if cached successfully, false if the file couldn't be found or no usable data was inside.
 */
bool:DLR_Cache()
{
    // Get the config path from the gamerules module.
    decl String:configfile[PLATFORM_MAX_PATH];
    GameRules_GetConfigPath(DLR_CONFIG_FILE, configfile);
    
    if (ConfigMgr_ValidateFile(configfile))
        ConfigMgr_WriteString(g_moduleDLR, CM_CONFIGINDEX_FIRST, ConfigData_Path, CM_DATA_PATH, configfile);
    else
    {
        LogMgr_Print(g_moduleDLR, LogType_Fatal_Module, "Config Validation", "Invalid config file path defined in gamerules: \"%s\".  Disabling module.", configfile);
        return false;
    }
    
    new count;
    new Handle:hDLRData = ConfigMgr_CacheFile(g_moduleDLR, CM_CONFIGINDEX_FIRST, PLATFORM_MAX_PATH, count);
    if (hDLRData == INVALID_HANDLE)
    {
        LogMgr_Print(g_moduleDLR, LogType_Fatal_Module, "Downloads file", "Unexpected error loading data from downloads file: \"%s\"", configfile);
        return false;
    }
    
    // Process the cached data.
    new downloadvalidcount;
    new downloadinvalidcount;
    
    decl String:downloadpath[PLATFORM_MAX_PATH];
    decl String:dir[PLATFORM_MAX_PATH];
    decl String:filename[32];
    new bool:regex;
    new Handle:hDir;
    decl String:iter_filename[32];
    new FileType:filetype;
    decl String:fullpath[PLATFORM_MAX_PATH];
    
    for (new i = 0; i < count; i++)
    {
        // Get download path
        GetArrayString(hDLRData, i, downloadpath, sizeof(downloadpath));
        
        // If file exists, then add to the downloads table and increment valid count.
        if (FileExists(downloadpath))
        {
            downloadvalidcount++;
            AddFileToDownloadsTable(downloadpath);
            continue;
        }
        
        regex = DLR_HasWildcard(downloadpath);
        
        // Queue all files in the directory.
        hDir = OpenDirectory(downloadpath);
        if (hDir != INVALID_HANDLE)
            strcopy(dir, sizeof(dir), downloadpath);
        else
        {
            // Break this dir up into a path and the file it refers to.
            DLR_GetFileFromPath(downloadpath, dir, sizeof(dir), filename, sizeof(filename));
            hDir = OpenDirectory(dir);
            if (hDir == INVALID_HANDLE || !regex)
            {
                downloadinvalidcount++;
                LogMgr_Print(g_moduleDLR, LogType_Error, "File Parsing", "Could not find file(s) for line: \"%s\"", downloadpath);
                continue;
            }
        }
        
        // Iterate through every file in the folder, ignoring folder names. (no recursive folder support)
        while (ReadDirEntry(hDir, iter_filename, sizeof(iter_filename), filetype))
        {
            if (filetype != FileType_File)
                continue;
            
            // If the file has regex in it and it doesn't pass, then this file should not be queued.
            if (regex && !DLR_WildcardParse(filename, iter_filename))
                continue;
            
            Format(fullpath, sizeof(fullpath), "%s/%s", dir, iter_filename);
            if (FileExists(fullpath))
            {
                downloadvalidcount++;
                AddFileToDownloadsTable(fullpath);
            }
            else
            {
                downloadinvalidcount++;
                LogMgr_Print(g_moduleDLR, LogType_Error, "File Iteration", "File iteration failure: \"%s\"  Contact plugin support.", fullpath);
            }
        }
    }
    
    LogMgr_Print(g_moduleDLR, LogType_Normal, "Downloader Report", "Downloads queued: %d | Invalid entries: %d", downloadvalidcount, downloadinvalidcount);
    
    CloseHandle(hDLRData);
    return true;
}

/**
 * Breaks a path up into a folder structure and file.
 *                   
 * @param path              The full path (file not required, 'file' will be set to null.)
 * @param folder_struct     The folder structure.
 * @param maxlen            The max length of the folder structure.
 * @param file              The file at the end of the path, null if none existed.
 * @param maxlen2           The max length of the file name.
 */
static stock DLR_GetFileFromPath(const String:path[], String:folder_struct[], maxlen, String:file[], maxlen2)
{
    // This isn't a path, just a filename, so copy it to the part2 output string and nullify part1.
    if (StrContains(path, "/") == -1 && StrContains(path, "\\") == -1)
    {
        folder_struct[0] = '\0';
        strcopy(file, maxlen2, path);
        return;
    }
    
    // Get the position of the last "/" or "\" in the string.
    new breakpoint = FindCharInString(path, '/', true);
    if (breakpoint == -1)
        FindCharInString(path, '\\', true);
    
    strcopy(folder_struct, breakpoint+1, path);
    strcopy(file, maxlen, path[breakpoint+1]);
}

/**
 * Parses a given input to see if it meets the condition specified with simplified regular expressions.
 * 
 * @param condition     The form the input needs to match. (ex. condition: filename.*  input: filename.ext = pass)
 * @param input         The input to check against the condition.
 * 
 * @return              True if the input passes the given condition, false if not.
 */
static stock bool:DLR_WildcardParse(const String:condition[], const String:input[])
{
    // Prepare the regular expression.
    decl String:regex[64];
    DLR_ConvertToRegex(condition, regex, sizeof(regex));
    
    return (SimpleRegexMatch(input, regex, PCRE_CASELESS) > 0);
}

/**
 * Takes a (custom) simplified regular expression and returns a valid regular expression.
 * 
 * @param input     The input string to be modified.
 * @param regex     The regular expression to prepare.
 * @param maxlen    Max length of the output.
 */
static stock DLR_ConvertToRegex(const String:input[], String:regex[], maxlen)
{
    strcopy(regex, maxlen, input);
    
    // All of these regex meta characters are not considered in our simplified version.
    ReplaceString(regex, maxlen, "(", "\\(");
    ReplaceString(regex, maxlen, ")", "\\)");
    ReplaceString(regex, maxlen, "{", "\\{");
    ReplaceString(regex, maxlen, "}", "\\}");
    ReplaceString(regex, maxlen, "$", "\\$");
    ReplaceString(regex, maxlen, "+", "\\+");
    ReplaceString(regex, maxlen, ".", "\\.");
    
    // Replace simple wildcard characters with regex wildcard formatting.
    ReplaceString(regex, maxlen, "*", ".*");
    ReplaceString(regex, maxlen, "?", ".");
    
    // Adding a '$' to the end of the line in regular expressions ensures the end matches exactly.
    // Without this, regex would match "The cat in the hat" with "The cat in the hat!!"
    // That would accept ".mdl.ztmp" when putting ".mdl".
    StrCat(regex, maxlen, "$");
}

/**
 * Checks if a string contains any wildcard characters.
 * 
 * @param input     The input string to be checked.
 */
static stock bool:DLR_HasWildcard(const String:input[])
{
    return (StrContains(input, "*") >= 0 ||
             StrContains(input, "?") >= 0 ||
             StrContains(input, "[") >= 0 ||
             StrContains(input, "]") >= 0);
}
