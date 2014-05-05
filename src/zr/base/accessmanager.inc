/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          accessmanager.inc
 *  Type:          Base
 *  Description:   Manages client access to project modules.
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
 * Provides the plugin a way to know if the access manager is included in the project.
 */
#define ACCESS_MANAGER

/**
 * Access cvars.
 */
new Handle:g_hCvarRoot;

// **********************************************
//                 Forwards
// **********************************************

/**
 * Plugin is loading.
 */
AccessMgr_OnPluginStart()
{
    // Create cvars.
    g_hCvarRoot = Project_CreateConVar("access_root", "z", "Admin flag(s) required for root access to the plugin.  Changing this requires a server restart.");
}

/**
 * Plugin is ending.
 */
AccessMgr_OnPluginEnd()
{
}

/**
 * A module was just registered.  This is being called after a module identifier has been assigned.
 * 
 * @param module        The module identifer of the registered module.
 */
stock AccessMgr_OnModuleRegistered(Module:module)
{
    // Create a cvar to control access flags for this module.
    
    decl String:cvarname[16 + 16];  // 16 for the module's shortname and 16 for the rest.
    AccessMgr_ModuleToCvarName(module, cvarname, sizeof(cvarname));
    
    new String:strCvarRootFlags[32];
    AccessMgr_CvarRoot(strCvarRootFlags, sizeof(strCvarRootFlags));
    
    // Create a module-specific cvar with cvar project_access_root as the default value.
    CreateConVar(cvarname, strCvarRootFlags, "Admin flag(s) required to access this module.");
}

// **********************************************
//                Public API
// **********************************************

/**
 * Check if a client has access to a specified module.
 * Client has automatic access if they have the root flag(s).
 * 
 * @param client    The client index.
 * @param module    The module identifier to check if the client has access to.
 */
stock bool:AccessMgr_HasAccess(client, Module:module = INVALID_MODULE)
{
    // Console always has access.
    if (client == SERVER_INDEX)
        return true;
    
    // Root access always returns true.
    if (AccessMgr_HasRootAccess(client))
        return true;
    
    if (module == INVALID_MODULE)
        return false;
    
    // If client is in the module's access group, then they have access.
    if (AccessMgr_HasModuleAccessGroup(client, module))
        return true;
    
    // If client has the flag(s) defined in the module access cvar, then they have access.
    if (AccessMgr_HasModuleAccessCvar(client, module))
        return true;
    
    return false;
}

// **********************************************
//   Private API (For base project files only)
// **********************************************

/**
 * Check if a client has the root access flag(s) as defined by the project_access_root cvar.
 * 
 * @param client    The client index.
 */
stock bool:AccessMgr_HasRootAccess(client)
{
    static AdminFlag:AdminFlags[32];
    static flag_count = 0;
    
    if (flag_count == 0) // Basically, flag_count is "saved" so if it's non-zero, then save some work and skip the next part.
    {
        new String:strCvarRootFlags[sizeof(AdminFlags)];
        AccessMgr_CvarRoot(strCvarRootFlags, sizeof(strCvarRootFlags));
        
        // Loop through each char in the string.
        new AdminFlag:flag;
        new length = strlen(strCvarRootFlags);
        for (new cindex = 0; cindex < length; cindex++)
        {
            if (!FindFlagByChar(strCvarRootFlags[cindex], flag))
            {
                LogError("[Access Manager] Invalid char (%c) in cvar project_access_root.", strCvarRootFlags[cindex]);
                continue;
            }
            
            AdminFlags[flag_count] = flag;
            flag_count++;
        }
    }
    
    // If the cvar isn't defined, no one has root access.
    if (flag_count == 0)
        return false;
    
    // Check each flag, if the client doesn't have one of these flags, then they don't have root.
    for (new findex = 0; findex < flag_count; findex++)
    {
        if (!AccessMgr_GetAdminFlag(GetUserAdmin(client), AdminFlags[findex]))
            return false;
    }
    
    return true;
}

/**
 * Check if a client has access to a module via groups.
 * 
 * @param client    The client index.
 * @param module    The module to check if client has access to.
 */
stock bool:AccessMgr_HasModuleAccessGroup(client, Module:module)
{
    decl String:group[16 + 16]; // 16 for module's shortname and 16 for the prefix.
    AccessMgr_ModuleToGroupName(module, group, sizeof(group));
    
    // If client is in the group, then they have access.
    if (AccessMgr_ClientInGroup(client, FindAdmGroup(group)))
        return true;
    
    return false;
}

/**
 * Check if a client has access to a module via the project_access_<moduleshortname> cvar.
 * 
 * @param client    The client index.
 * @param module    The module to check if client has access to.
 */
stock bool:AccessMgr_HasModuleAccessCvar(client, Module:module)
{
    new AdminFlag:AdminFlags[32];
    new flag_count = 0;
    
    decl String:cvarname[16 + 16];
    AccessMgr_ModuleToCvarName(module, cvarname, sizeof(cvarname));
    
    new String:strCvarModuleFlags[sizeof(AdminFlags)];
    GetConVarString(FindConVar(cvarname), strCvarModuleFlags, sizeof(strCvarModuleFlags));
    
    // Loop through each char in the string.
    new AdminFlag:flag;
    new length = strlen(strCvarModuleFlags);
    for (new cindex = 0; cindex < length; cindex++)
    {
        if (!FindFlagByChar(strCvarModuleFlags[cindex], flag))
            continue;
        
        AdminFlags[flag_count] = flag;
        flag_count++;
    }
    
    // If the cvar isn't defined, no one has module access.
    if (flag_count == 0)
        return false;
    
    // Check each flag, if the client doesn't have one of these flags, then they don't have root.
    for (new findex = 0; findex < flag_count; findex++)
    {
        if (!AccessMgr_GetAdminFlag(GetUserAdmin(client), AdminFlags[findex]))
            return false;
    }
    
    return true;
}

/**
 * Wrapper function to check both real and effective access modes.
 * 
 * See native GetAdminFlag, but both AdmAccessMode options are checked.
 */
stock bool:AccessMgr_GetAdminFlag(AdminId:adminid, AdminFlag:flag)
{
    return bool:(GetAdminFlag(adminid, flag, Access_Real) ||
            GetAdminFlag(adminid, flag, Access_Effective));
}

/**
 * Checks if a client is in a specified group.
 * 
 * @param client    The client index.
 * @param groupid   The group id to check.
 * 
 * @return          True if the client is in the group, false if not.
 */
stock bool:AccessMgr_ClientInGroup(client, GroupId:groupid)
{
    if (groupid == INVALID_GROUP_ID)
        return false;
    
    decl String:testgroup[16 + 16];     // See 'group'
    new GroupId:testgroupid;
    new count = GetAdminGroupCount(GetUserAdmin(client));
    for (new gindex; gindex < count; gindex++)
    {
        testgroupid = GetAdminGroup(GetUserAdmin(client), gindex, testgroup, sizeof(testgroup));
        if (testgroupid == INVALID_GROUP_ID)
        {
            LogError("[Access Manager] Encountered unexpected error while checking client %N's admin groups.", client);
            return false;
        }
        
        // If the module's group (found earlier) matches this group from the client's group table, then they have access.
        if (groupid == testgroupid)
            return true;
    }
    
    // Not in the specified group.
    return false;
}

/**
 * Take a module identifier and output its group name.
 * 
 * @param module    The module identifier to return group name for.
 * @param group     The group name associated with this module.
 * @param maxlen    The max length of the output string.
 */
stock AccessMgr_ModuleToGroupName(Module:module, String:group[], maxlen)
{
    decl String:moduleshortname[MM_DATA_SHORTNAME];
    ModuleMgr_ReadString(module, ModuleData_ShortName, moduleshortname, sizeof(moduleshortname));
    
    // Format output.
    Format(group, maxlen, AM_FORMAT_GROUP);
}

/**
 * Take a module identifier and output its access cvar name.
 * 
 * @param module    The module identifier to return group name for.
 * @param cvar      The access cvar name associated with this module.
 * @param maxlen    The max length of the output string.
 */
stock AccessMgr_ModuleToCvarName(Module:module, String:cvar[], maxlen)
{
    decl String:moduleshortname[MM_DATA_SHORTNAME];
    ModuleMgr_ReadString(module, ModuleData_ShortName, moduleshortname, sizeof(moduleshortname));
    
    Format(cvar, maxlen, AM_FORMAT_CVAR);
}

/**
 * These are stock functions to return the value of any of the access manager's cvars.
 * This allows other base components/modules to read access cvars.
 */

stock AccessMgr_CvarRoot(String:strCvarRoot[], maxlen)
{
    GetConVarString(g_hCvarRoot, strCvarRoot, maxlen);
}
