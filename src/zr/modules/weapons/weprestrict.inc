/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *  Dependencies: weapons.inc
 *
 *  File:          restrict.inc
 *  Type:          Module
 *  Description:   Weapon restriction system.
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

// Include libraries.
#include "zr/libraries/menulib"

/**
 * This module's identifier.
 */
new Module:g_moduleWepRestrict;

/**
 * Weapon restriction's button index in ZAdmin
 */
new g_iWepRestrictZAdminButton;

/**
 * Handle of the last menu lib handle before the weapon restriction menus.
 */
new Handle:g_hWepRestrictLastMenu[MAXPLAYERS + 1];

/**
 * Query results returned when (un)restricting a weapon.
 */
enum RestrictQuery
{
    Query_Successful,   /** (Un)restrict was successful. */
    Query_Stopped,      /** (Un)restrict was stopped because action was redundant. */
    Query_Locked,       /** (Un)restrict was stopped because the weapon is marked "untoggleable." */
    Query_Invalid,      /** (Un)restrict failed because invalid info was given. */
}

/**
 * Register this module.
 */
WepRestrict_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Weapon Restriction");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "weprestrict");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Weapon restriction system.");
    moduledata[ModuleData_Dependencies][0] = Weapons_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleWepRestrict = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleWepRestrict, "Event_OnEventsRegister",       "WepRestrict_OnEventsRegister");
}

/**
 * Register all events here.
 */
public WepRestrict_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleWepRestrict, "Event_OnZAdminCreated",        "WepRestrict_OnZAdminCreated");
    EventMgr_RegisterEvent(g_moduleWepRestrict, "Event_OnMyModuleEnable",       "WepRestrict_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleWepRestrict, "Event_OnMyModuleDisable",      "WepRestrict_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleWepRestrict, "Hook_WeaponCanUse",            "WepRestrict_CanUse");
}

/**
 * Plugin is loading.
 */
WepRestrict_OnPluginStart()
{
    // Register the module (sub-module of weapons)
    WepRestrict_Register();
    
    #if defined PROJECT_GAME_CSS
        // Listen for buy commands.
        AddCommandListener(WepRestrict_BuyListener, "buy");
        AddCommandListener(WepRestrict_BuyListener, "autobuy");
        AddCommandListener(WepRestrict_BuyListener, "rebuy");
        
        // Create weapon admin commands.
        Project_RegConsoleCmd("restrict", Command_Restrict, "Restricts a weapon or a weapon type. Usage: zr_restrict <weapon|weapon type> [weapon2|weapontype2] ...");
        Project_RegConsoleCmd("unrestrict", Command_Unrestrict, "Unrestricts a weapon or a weapon type. Usage: zr_unrestrict <weapon|weapon type> [weapon2|weapontype2] ...");
    #endif
}

/**
 * Called when ZAdmin is created.
 * 
 * @param hMenu     Handle to ZAdmin.
 */
public WepRestrict_OnZAdminCreated(Handle:hMenu)
{
    // Add button to the menu.
    g_iWepRestrictZAdminButton = MenuLib_AddMenuBtnEx(hMenu, "Weapons zadmin button", "0", true, ITEMDRAW_DEFAULT, WepRestrict_SendRestrictMenu, BtnNextMenu_None, "");
}

/**
 * Menu callback
 * Called when a certain button in a menu is pressed.  The menu action is always MenuAction_Select.
 * 
 * @param menu      The menulib menu handle.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public WepRestrict_SendRestrictMenu(Handle:hMenu, client, slot)
{
    g_hWepRestrictLastMenu[client] = hMenu;
    WepMenus_Types(client);
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:WepRestrict_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // Enable button in ZAdmin menu.
    new Handle:hZAdmin = MenuLib_FindMenuById("zadmin");
    if (hZAdmin != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZAdmin, g_iWepRestrictZAdminButton, MenuBtn_Style, ITEMDRAW_DEFAULT);
    
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
public Action:WepRestrict_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    // Disable button in ZAdmin menu.
    new Handle:hZAdmin = MenuLib_FindMenuById("zadmin");
    if (hZAdmin != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZAdmin, g_iWepRestrictZAdminButton, MenuBtn_Style, ITEMDRAW_DISABLED);
    
    #if defined PROJECT_GAME_CSS
        // Stop listening for buy commands.
        RemoveCommandListener(WepRestrict_BuyListener, "buy");
        RemoveCommandListener(WepRestrict_BuyListener, "autobuy");
        RemoveCommandListener(WepRestrict_BuyListener, "rebuy");
    #endif
    
    return Plugin_Continue;
}

/**
 * Command listener for the buy, autorebuy, and rebuy commands.
 * Used to block use of this command under certain conditions.
 * 
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:WepRestrict_BuyListener(client, const String:command[], argc)
{
    // Not a client using the command.
    if (client <= 0)
        return Plugin_Continue;
    
    if (!IsClientInGame(client))
        return Plugin_Continue;
    
    // Don't bother blocking them if they aren't in a buyzone.
    if (!WepLib_InBuyZone(client))
        return Plugin_Continue;
    
    decl String:weapon[64];
    GetCmdArg(1, weapon, sizeof(weapon));
    
    decl String:weapondisp[WEPCONFIG_NAME];
    new windex = Weapons_ClsNameToDisplay(weapon, weapondisp, sizeof(weapondisp), true);
    
    // The weapon has no data.
    if (windex == -1)
        return Plugin_Continue;
    
    // Stop the command if the weapon is restricted.
    if (WepRestrict_IsRestricted(windex))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "Weapon is restricted", weapondisp);
        return Plugin_Handled;
    }
    
    return Plugin_Continue;
}

/**
 * Restricts (or unrestricts) a given weapon or weapon type.
 * 
 * @param restrict      True to restrict, false to unrestrict.
 * @param target        Weapon or weapon type to restrict/unrestrict.
 * @param single        True if a single weapon is being restricted, false if weapon type.
 * @param returntarget  The proper targetname. (same as param 'target' if invalid)
 * @param maxlen        The max length of the returned target name.
 * 
 * @return              Query result. (See enum RestrictQuery)
 */
stock RestrictQuery:WepRestrict_Restrict(bool:restrict, const String:target[], &bool:single = true, String:returntarget[], maxlen)
{
    // Copy 'target' to 'returntarget' to be possibly changed later.
    strcopy(returntarget, maxlen, target);
    
    // Find index of the given weapon type.
    new wtindex = Weapons_TypeNameToIndex(returntarget);
    
    // Single weapon.
    if (wtindex == -1)
    {
        single = true;
        
        new windex = Weapons_NameToIndex(returntarget);
        
        // This is neither a valid weapon type nor weapon name.
        if (windex == -1)
            return Query_Invalid;
        
        // Return proper weapon name.
        Weapons_ReadString(windex, WepConfig_Name, returntarget, maxlen);
        
        // Check if the weapon is toggleable.
        if (!Weapons_ReadCell(windex, WepConfig_Toggleable))
            return Query_Locked;
        
        // This is a redundant restriction.
        if (WepRestrict_IsRestricted(windex) == restrict)
            return Query_Stopped;
        
        // Set weapon restriction.
        WepRestrict_SetRestriction(windex, restrict);
        return Query_Successful;
    }
    // Weapon type.
    else
    {
        single = false;
        
        // Check if weapon restriction is redundant.
        if (WepRestrict_AreWeaponsUniform(wtindex, restrict))
            return Query_Stopped;
        
        // Get all weapons in the given type.
        new Handle:hWeapons;
        new count = Weapons_GetWeaponsOfType(wtindex, hWeapons);
        
        // Return proper weapon type name.
        Weapons_ReadWTypeName(wtindex, returntarget, maxlen);
        
        new windex;
        for (new weparrayindex = 0; weparrayindex < count; weparrayindex++)
        {
            // Get weapon index.
            windex = GetArrayCell(hWeapons, weparrayindex);
            
            // Don't change untoggleable weapons.
            if (!Weapons_ReadCell(windex, WepConfig_Toggleable))
                continue;
            
            WepRestrict_SetRestriction(windex, restrict);
        }
        
        CloseHandle(hWeapons);
        
        return Query_Successful;
    }
}

/**
 * An array of translated responses to restriction queries.
 */
static String:g_strRestrictionResponses[RestrictQuery][2][2][32] = 
{
    {   { "Unrestrict weapon type", "Restrict weapon type" },
        { "Unrestrict weapon", "Restrict weapon" } },
    
    {   { "Unrestrict weapon type stopped", "Restrict weapon type stopped" },
        { "Unrestrict weapon stopped", "Restrict weapon stopped" }    },
    
    {   { "Restrict weapon untoggleable", "" },
        { "", "" }    },
    
    {   { "Weapon invalid", "" },
        { "", "" }    }
};

/**
 * Print weapon (un)restriction query result to client(s).
 * 
 * @param client    The client to print response to. (0 for all clients)
 * @param query     The query result.
 * @param single    True if a single weapon is being restricted.
 * @param restrict  True if the query was to restrict/unrestrict a weapon.
 * @param target    The target to be restricted/unrestricted.
 */
stock WepRestrict_PrintQueryResponse(client, RestrictQuery:query, bool:single, bool:restrict, const String:target[])
{
    // Translations only exist for these queries when single & restrict are false(0).
    if (query == Query_Locked || query == Query_Invalid)
    {
        single = false;
        restrict = false;
    }
    
    if (client > 0 && client <= MaxClients)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, g_strRestrictionResponses[query][_:single][_:restrict], target);
    }
    else
    {
        TransMgr_PrintTextAll(true, false, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, g_strRestrictionResponses[query][_:single][_:restrict], target);
    }
}

/**
 * Gets the restricted status on a weapon.
 * 
 * @param index     The weapon cache index.
 * @param restrict  True to restrict the weapon, false to unrestrict.
 */
stock WepRestrict_SetRestriction(index, bool:restrict)
{
    Weapons_WriteCell(index, WepConfig_Restricted, restrict);
}

/**
 * Checks if a weapon is restricted.
 * 
 * @param index     The weapon cache index.
 * @return          True if weapon is restricted, false if not.
 */
stock bool:WepRestrict_IsRestricted(index)
{
    // If the module is disabled, nothing is restricted.
    if (ModuleMgr_IsDisabled(g_moduleWepRestrict))
        return false;
    
    return bool:Weapons_ReadCell(index, WepConfig_Restricted);
}

/**
 * Used to check if all weapons of a type are restricted.
 * 
 * @param index         The weapon type index.
 * @param restricted    The restriction to match to each weapon for uniformness.  Returns false if any weapons differ from this.
 * 
 * @return              True if all weapons of the given type are uniformly restricted or not, false if not.
 */
stock bool:WepRestrict_AreWeaponsUniform(index, bool:restricted)
{
    new windex;
    
    new Handle:hWeapons;
    new count = Weapons_GetWeaponsOfType(index, hWeapons);
    for (new weparrayindex = 0; weparrayindex < count; weparrayindex++)
    {
        // Get weapon index from the returned array.
        windex = GetArrayCell(hWeapons, weparrayindex);
        
        // Ignore weapons whose restrictions can't be changed.
        if (!Weapons_ReadCell(windex, WepConfig_Toggleable))
            continue;
        
        // Check if the restriction doesn't match the given.
        if (WepRestrict_IsRestricted(windex) != restricted)
            return false;
    }
    
    CloseHandle(hWeapons);
    
    return true;
}

/**
 * Called when a client is trying to pick up a weapon.
 * 
 * @param client    The client index.
 * @param weapon    The weapon index.
 * 
 * @return          Hook action.  See include/core.inc.
 */
public Action:WepRestrict_CanUse(client, weapon)
{
    new String:weaponentity[64];
    GetEdictClassname(weapon, weaponentity, sizeof(weaponentity));
    
    // Knife should never be restricted.
    // TODO: All weapons should be restricted for ghost players, or defined by
    //       their class restrictions.
    if (StrEqual(weaponentity, "weapon_knife"))
        return Plugin_Continue;
    
    decl String:weapondisp[WEPCONFIG_NAME];
    new windex = Weapons_ClsNameToDisplay(weaponentity, weapondisp, sizeof(weapondisp));
    
    // If the weapon isn't configured then we shouldn't interfere.
    if (windex == -1)
        return Plugin_Continue;
    
    // If weapon is restricted, then stop.
    if (WepRestrict_IsRestricted(windex))
        return Plugin_Handled;
    
    return Plugin_Continue;
}

/**
 * Command callbacks.
 */

/**
 * Command callback (zr_restrict)
 * Restricts a weapon or group
 *   
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:Command_Restrict(client, argc)
{
    // Check if the module is disabled.
    if (ModuleMgr_IsDisabled(g_moduleWepRestrict))
        return Plugin_Continue;
    
    // Check if client has access.
    if (!AccessMgr_HasAccess(client, g_moduleWepRestrict))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No access to command");
        return Plugin_Handled;
    }
    
    // Check if enough arguments were given.
    if (argc < 1)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Weapons command restrict syntax", PROJECT_CMD_PREFIX);
        return Plugin_Handled;
    }
    
    decl String:target[64], String:returntarget[WEPCONFIG_NAME];
    new bool:single;
    new RestrictQuery:query;
    
    new args = GetCmdArgs();
    for (new arg = 1; arg <= args; arg++)
    {
        // Get target to restrict.
        GetCmdArg(arg, target, sizeof(target));
        
        // Query restrict on this target, and get a result back.
        query = WepRestrict_Restrict(true, target, single, returntarget, sizeof(returntarget));
        
        // Print response to client(s).
        WepRestrict_PrintQueryResponse((query != Query_Successful) ? client : 0, query, single, true, returntarget);
 
    }
    
    return Plugin_Handled;
}

/**
 * Command callback (zr_unrestrict)
 * Unrestricts a weapon or group
 *   
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:Command_Unrestrict(client, argc)
{
    // Check if the module is disabled.
    if (ModuleMgr_IsDisabled(g_moduleWepRestrict))
        return Plugin_Continue;
    
    // Check if client has access.
    if (!AccessMgr_HasAccess(client, g_moduleWepRestrict))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No access to command");
        return Plugin_Handled;
    }
    
    // Check if enough arguments were given.
    if (argc < 1)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Weapons command unrestrict syntax", PROJECT_CMD_PREFIX);
        return Plugin_Handled;
    }
    
    decl String:target[64], String:returntarget[WEPCONFIG_NAME];
    new bool:single;
    new RestrictQuery:query;
    
    new args = GetCmdArgs();
    for (new arg = 1; arg <= args; arg++)
    {
        // Get target to restrict.
        GetCmdArg(arg, target, sizeof(target));
        
        // Query restrict on this target, and get a result back.
        query = WepRestrict_Restrict(false, target, single, returntarget, sizeof(returntarget));
        
        // Print response to client(s).
        WepRestrict_PrintQueryResponse((query != Query_Successful) ? client : 0, query, single, false, returntarget);
    }
    
    return Plugin_Handled;
}
