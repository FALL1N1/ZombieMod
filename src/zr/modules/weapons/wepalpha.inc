/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *  Dependencies: weapons.inc
 *
 *  File:          wepalpha.inc
 *  Type:          Module
 *  Description:   Weapon alpha functions, and alpha updating on drop/pickup.
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
#include "zr/libraries/zrwrappers"

/**
 * This module's identifier.
 */
new Module:g_moduleWepAlpha;

/**
 * Register this module.
 */
WepAlpha_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Weapon Alpha Updater");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "wepalpha");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Updates weapon alpha when a player's alpha is changed.");
    moduledata[ModuleData_Dependencies][0] = Weapons_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleWepAlpha = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleWepAlpha, "Event_OnEventsRegister",      "WepAlpha_OnEventsRegister");
}

/**
 * Register all events here.
 */
public WepAlpha_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleWepAlpha, "Event_OnClientAlphaChanged",      "WepAlpha_OnClientAlphaChanged");
    EventMgr_RegisterEvent(g_moduleWepAlpha, "Hook_WeaponEquip",                "WepAlpha_WeaponEquip");
    EventMgr_RegisterEvent(g_moduleWepAlpha, "Hook_WeaponDrop",                 "WepAlpha_WeaponDrop");
}

/**
 * Plugin is loading.
 */
WepAlpha_OnPluginStart()
{
    // Register the module (sub-module of weapons)
    WepAlpha_Register();
}

/**
 * A client's alpha has changed.
 * 
 * @param client    The client index.
 * @param newalpha  The client's new alpha. 
 */
public WepAlpha_OnClientAlphaChanged(client, newalpha)
{
    // Set new alpha on weapons.
    WepAlpha_Apply(client, newalpha);
}

/**
 * Called when a client equips a weapon.
 * 
 * @param client    The client index.
 * @param weapon    The weapon index.
 * 
 * @return          Hook action.  See include/core.inc.
 */
public WepAlpha_WeaponEquip(client, weapon)
{
    // If weapon isn't a valid entity, then stop.
    if (!IsValidEntity(weapon))
        return;
    
    // Get the client's color.
    new color[4];
    OffsLib_GetEntityColor(client, color);
    
    // Set new alpha on weapon.
    SetEntityRenderMode(weapon, RENDER_TRANSALPHA);
    Wrapper_SetEntRenderClr(weapon, 255, 255, 255, color[3]);
}

/**
 * Called when a client drops a weapon.
 * 
 * @param client    The client index.
 * @param weapon    The weapon index.
 * 
 * @return          Hook action.  See include/core.inc.
 */
public WepAlpha_WeaponDrop(client, weapon)
{
    // If weapon isn't a valid entity, then stop.
    if (!IsValidEntity(weapon))
        return;
    
    // Reset alpha to default on the dropped weapon.
    SetEntityRenderMode(weapon, RENDER_TRANSALPHA);
    Wrapper_SetEntRenderClr(weapon, 255, 255, 255, 255);
}

/**
 * Sets an alpha on all of a client's weapons.
 * 
 * @param client    The client index.
 * @param alpha     The alpha to set the weapons to.
 */
stock WepAlpha_Apply(client, alpha)
{
    // Get client's list of weapons.
    new weapons[WepLib_Slots];
    WepLib_GetWeapons(client, weapons);
    
    // Loop through array slots and set alpha.
    // x = weapon slot.
    for (new sindex = 0; sindex < WEPLIB_SLOT_COUNT; sindex++)
    {
        // Check if there is a weapon in the slot.
        if (weapons[sindex] == -1)
            continue;
        
        SetEntityRenderMode(weapons[sindex], RENDER_TRANSALPHA);
        Wrapper_SetEntRenderClr(weapons[sindex], 255, 255, 255, alpha);
    }
}
