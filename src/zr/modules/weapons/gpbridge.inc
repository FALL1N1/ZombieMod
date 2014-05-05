/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          weaponammo.inc
 *  Type:          Core 
 *  Description:   API for all weaponammo-related functions.
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
new Module:g_moduleGPBridge;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:GPBridge_GetIdentifier() { return g_moduleGPBridge; }

/**
 * Grenade Pack plugin information.
 */
#define GRENADEPACK_FILE_NAME   "grenadepack.smx"
#define GRENADEPACK_CVAR_LIMIT  "gp_limit"

/**
 * Variable to store if Grenade Pack is loaded and running.
 */
new bool:g_bGrenadePackLoaded;

/**
 * Register this module.
 */
GPBridge_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Grenade Pack Bridge");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "gpbridge");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Allows Zombie:Reloaded to support servers using the Grenade Pack plugin.");
    moduledata[ModuleData_Dependencies][0] = ZMarket_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleGPBridge = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleGPBridge, "Event_OnEventsRegister",      "GPBridge_OnEventsRegister");
}

/**
 * Register all events here.
 */
public GPBridge_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleGPBridge, "Event_OnAllPluginsLoaded",    "GPBridge_OnAllPluginsLoaded");
}

/**
 * Plugin is loading.
 */
GPBridge_OnPluginStart()
{
    // Register the module.
    GPBridge_Register();
}

/**
 * All plugins have loaded.
 */
public GPBridge_OnAllPluginsLoaded()
{
    // Check if "Grenade Pack" is loaded
    g_bGrenadePackLoaded = GPBridge_CheckIfLoaded();
    if (g_bGrenadePackLoaded)
        LogMgr_Print(g_moduleGPBridge, LogType_Normal, "Grenade Pack Support", "Active plugin file \"%s\" has been detected.", GRENADEPACK_FILE_NAME);
}

/**
 * Checks the value of g_bGrenadePackLoaded.
 * 
 * @return  True if Grenade Pack is loaded on the saerver, false if not.
 */
stock bool:GPBridge_IsLoaded()
{
    return g_bGrenadePackLoaded;
}

/**
 * Checks if the Grenade Pack plugin is loaded on the server.
 * 
 * @return  True if Grenade Pack is loaded on the saerver, false if not.
 */
stock bool:GPBridge_CheckIfLoaded()
{
    new Handle:hPlugins = GetPluginIterator();
    new Handle:hPlugin;
    decl String:strPlugin[64];
    
    // Loop through all the plugins in the plugins/ directory.
    while (MorePlugins(hPlugins))
    {
        // Read the next plugin.
        hPlugin = ReadPlugin(hPlugins);
        
        GetPluginFilename(hPlugin, strPlugin, sizeof(strPlugin));
        if (StrEqual(strPlugin, GRENADEPACK_FILE_NAME, false))
        {
            CloseHandle(hPlugins);
            return (GetPluginStatus(hPlugin) == Plugin_Running);
        }
    }
    
    CloseHandle(hPlugins);
    return false;
}

/**
 * This function should be called whenever the plugin wants to know how many HEGrenades a client can hold
 * if GPBridge_IsLoaded returns 'true.'
 */
stock GPBridge_Intercept()
{
    static Handle:hGPLimit;
    if (hGPLimit == INVALID_HANDLE)
        hGPLimit = FindConVar("gp_limit");
    
    return GetConVarInt(hGPLimit);
}
